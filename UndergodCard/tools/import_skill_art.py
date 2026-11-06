"""Extract generated skill sheets; deterministic alpha cleanup and alignment only."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import hashlib, json, shutil, subprocess, sys
import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'artifacts/skill-art'
ASSETS = ROOT / 'assets/pastel'
PROCESSOR = Path.home() / '.codex/skills/generate2dsprite/scripts/generate2dsprite.py'

def body_height(image):
    rows = (np.array(image.getchannel('A')) > 128).sum(axis=1)
    visible = np.flatnonzero(rows >= max(24, int(rows.max() * .20)))
    return int(visible[-1] - visible[0] + 1)

def despill(image, fx):
    p = np.array(image).astype(np.float32)
    rgb = p[:, :, :3]
    matte = np.array([255., 0., 255.])
    distance = np.linalg.norm(rgb - matte, axis=2)
    suspect = (rgb[:, :, 0] > 165) & (rgb[:, :, 2] > 145) & (rgb[:, :, 1] < 170) & (distance < 215) & (p[:, :, 3] > 0)
    if not fx:
        p[suspect & (distance < 155), 3] = 0
        return Image.fromarray(p.astype(np.uint8))
    known = (distance > 225) & (p[:, :, 3] > 128)
    nearest = rgb.copy()
    reached = known.copy()
    for _ in range(24):
        changed = False
        for axis, step in [(0, 1), (0, -1), (1, 1), (1, -1)]:
            adjacent = np.roll(reached, step, axis=axis)
            if axis == 0: adjacent[0 if step > 0 else -1, :] = False
            else: adjacent[:, 0 if step > 0 else -1] = False
            take = suspect & ~reached & adjacent
            if take.any():
                nearest[take] = np.roll(nearest, step, axis=axis)[take]
                reached[take] = True
                changed = True
        if not changed: break
    eligible = suspect & reached
    direction = nearest - matte
    alpha = np.clip(np.sum((rgb - matte) * direction, axis=2) / np.maximum(1, np.sum(direction * direction, axis=2)), .01, 1)
    restored = (rgb - (1-alpha[:, :, None]) * matte) / alpha[:, :, None]
    p[eligible, :3] = np.clip(restored[eligible], 0, 255)
    p[eligible, 3] *= alpha[eligible]
    p[suspect & ~reached & (distance < 170), 3] = 0
    p[p[:, :, 3] < 12, 3] = 0
    return Image.fromarray(p.astype(np.uint8))

def process(job):
    name, body = job['name'], job['kind'] == 'body'
    directory = WORK / name
    directory.mkdir(parents=True, exist_ok=True)
    raw = directory / 'raw.png'
    if not raw.exists(): shutil.copy2(job['source'], raw)
    prompt = directory / 'prompt.txt'
    prompt.write_text(job['prompt'], encoding='utf-8')
    out = directory / 'processed'
    if not (out / 'pipeline-meta.json').exists():
        command = [sys.executable, str(PROCESSOR), 'process', '--input', str(raw), '--target', 'asset', '--mode', name,
                   '--rows', '2', '--cols', '2', '--cell-size', '512', '--fit-scale', '.85', '--align', 'feet' if body else 'center',
                   '--shared-scale', '--component-mode', 'largest' if body else 'all', '--min-component-area', '14',
                   '--trim-border', '0', '--edge-touch-margin', '3', '--reject-edge-touch', '--prompt-file', str(prompt), '--output-dir', str(out)]
        result = subprocess.run(command, capture_output=True, text=True)
        (directory / 'processor.log').write_text(result.stdout + result.stderr, encoding='utf-8')
        if result.returncode: raise RuntimeError(name + ': ' + result.stderr[-1000:] + result.stdout[-1000:])
    meta = json.loads((out / 'pipeline-meta.json').read_text(encoding='utf-8'))
    if meta['edge_touch_frames']: raise ValueError((name, 'clipped raw cell', meta['edge_touch_frames']))
    frames = [despill(Image.open(out / (label+'.png')).convert('RGBA'), not body) for label in meta['frame_labels']]
    report = {'name': name, 'edge_touch_frames': [], 'exports': []}
    if body:
        references = [Image.open(p).convert('RGBA') for p in (ASSETS / 'characters').glob('char_'+job['actor']+'_idle_*.png')]
        target = float(np.median([body_height(im) for im in references]))
        measured = float(np.median([body_height(im) for im in frames]))
        factor = min(target / measured, min(472 / (im.getbbox()[2]-im.getbbox()[0]) for im in frames), min(436 / (im.getbbox()[3]-im.getbbox()[1]) for im in frames))
        normalized = []
        for im in frames:
            crop = im.crop(im.getbbox())
            crop = crop.resize((round(crop.width * factor), round(crop.height * factor)), Image.Resampling.LANCZOS)
            canvas = Image.new('RGBA', (512, 512))
            canvas.paste(crop, ((512-crop.width)//2, 456-crop.height))
            normalized.append(canvas)
        frames = normalized
        report.update(reference_body_height=target, body_heights=[body_height(im) for im in frames])
        report['body_ratio'] = float(np.median(report['body_heights'])) / target
        if report['body_ratio'] < .82: raise ValueError((name, 'body shrink', report['body_ratio']))
        mask = np.array(frames[2].getchannel('A')) > 100
        ys, xs = np.nonzero(mask)
        right = job['actor'] in ('chick', 'mage', 'ninja')
        edge = xs.max() if right else xs.min()
        strip = xs >= edge-8 if right else xs <= edge+8
        report['emitterX'], report['emitterY'] = float(edge), float(np.median(ys[strip]))
    for i, im in enumerate(frames):
        assert im.getbbox() and im.getchannel('A').getextrema() == (0,255), name
        box = im.getbbox()
        assert box[0] > 0 and box[1] > 0 and box[2] < 512 and box[3] < 512, (name, box)
        if body:
            assert box[3] <= 457
            relative = 'characters/char_'+name+'_'+str(i)+'.png'
        else:
            relative = 'skill_fx/'+name+'_'+str(i)+'.png'
            if name.startswith('ward_'): assert im.getpixel((256,256))[3] < 24, (name, 'opaque shield center')
        path = ASSETS / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        im.save(path)
        report['exports'].append(path.relative_to(ROOT).as_posix())
    report['unique_frames'] = len({hashlib.sha256(im.tobytes()).hexdigest() for im in frames})
    assert report['unique_frames'] == 4, (name, 'duplicate animation frames')
    frames[0].save(directory / 'transparent.gif', save_all=True, append_images=frames[1:], duration=150, loop=0, disposal=2)
    report.update(source_file=raw.name, source_sha256=hashlib.sha256(raw.read_bytes()).hexdigest(), prompt=job['prompt'], reference=Path(job.get('reference') or '').name, grid=[2,2])
    (directory / 'qc.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(name + ': imported', flush=True)
    return report

def main():
    WORK.mkdir(parents=True, exist_ok=True)
    jobs = json.loads((ROOT / 'artifacts/skill-art-inputs.json').read_text(encoding='utf-8'))
    with ThreadPoolExecutor(max_workers=4) as pool: reports = list(pool.map(process, jobs))
    data_path = ROOT / 'data/skill_animations.json'
    data = json.loads(data_path.read_text(encoding='utf-8'))
    by_name = {r['name']: r for r in reports}
    for action in data['bodies']:
        report = by_name[action['actor']+'_'+action['pose']]
        action.update(emitterX=report['emitterX'], emitterY=report['emitterY'])
    data_path.write_text(json.dumps(data, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    provenance_path = ROOT / 'assets/provenance.json'
    provenance = json.loads(provenance_path.read_text(encoding='utf-8'))
    provenance['skill_animations'] = {'generator': 'built-in ImageGen', 'processing': 'generate2dsprite extraction, magenta alpha cleanup, shared body normalization, feet anchor y=456', 'groups': reports}
    provenance_path.write_text(json.dumps(provenance, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    manifest_path = ROOT / 'assets/manifest.json'
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    files = []
    for path in sorted(ASSETS.rglob('*.png')):
        with Image.open(path) as im:
            assert im.mode == 'RGBA' and im.getbbox() and (im.size in ((256,256),(512,512)) or path.parent.name=='ui')
            entry=dict(path=path.relative_to(ROOT).as_posix(), size=list(im.size), sha256=hashlib.sha256(path.read_bytes()).hexdigest())
            if path.parent.name=='skill_fx': entry['bounds']=list(im.getbbox())
            files.append(entry)
    manifest.update(runtime_pngs=len(files), files=files)
    manifest_path.write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    (WORK / 'qc.json').write_text(json.dumps(reports, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    sheet = Image.new('RGB', (1080, len(reports)*150), (39,43,49))
    draw = ImageDraw.Draw(sheet)
    for row, report in enumerate(reports):
        draw.text((5,row*150+5),report['name'],fill='white')
        for n,path in enumerate(report['exports']):
            im = Image.open(ROOT/path).convert('RGBA');im.thumbnail((230,125))
            sheet.paste(im,(150+n*230,row*150+18),im)
    sheet.save(WORK / 'contact.png')
    print('Manifest:',len(files),'PNG')

if __name__ == '__main__': main()
