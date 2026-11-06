"""Extract built-in ImageGen boss icon packs; never generates artwork."""
from pathlib import Path
import argparse, hashlib, json, subprocess, sys
from PIL import Image
import numpy as np

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--inputs', type=Path, required=True)
    parser.add_argument('--processor', type=Path, default=Path.home()/'.codex/skills/generate2dsprite/scripts/generate2dsprite.py')
    args = parser.parse_args()
    inputs = json.loads(args.inputs.read_text(encoding='utf-8'))
    decks = json.loads((ROOT/'data/boss_cards.json').read_text(encoding='utf-8'))
    by_owner = {deck['boss']: deck['cards'] for deck in decks}
    assert {item['boss'] for item in inputs} == set(by_owner)
    provenance, exports = [], []
    for item in inputs:
        owner = item['boss']; source = Path(item['source'])
        work = ROOT/'artifacts/boss-cards'/owner
        work.mkdir(parents=True, exist_ok=True)
        (work/'prompt.txt').write_text(item['prompt'], encoding='utf-8')
        subprocess.run([sys.executable, str(args.processor), 'process', '--input', str(source), '--target', 'asset', '--mode', 'boss_icons', '--rows', '2', '--cols', '3', '--cell-size', '256', '--label-prefix', owner, '--fit-scale', '.80', '--align', 'center', '--shared-scale', '--component-mode', 'all', '--min-component-area', '14', '--trim-border', '0', '--edge-touch-margin', '3', '--reject-edge-touch', '--prompt-file', str(work/'prompt.txt'), '--output-dir', str(work)], check=True, stdout=subprocess.DEVNULL)
        metadata = json.loads((work/'pipeline-meta.json').read_text(encoding='utf-8'))
        assert not metadata['edge_touch_frames'], owner
        files = []
        for index, card in enumerate(by_owner[owner], 1):
            image = Image.open(work/f'{owner}-{index}.png').convert('RGBA')
            pixels = np.array(image)
            rgb = pixels[:, :, :3].astype(np.int32)
            spill = ((rgb - np.array([255, 0, 255]))**2).sum(axis=2) < 100**2
            pixels[spill, 3] = 0
            image = Image.fromarray(pixels)
            bbox = image.getbbox()
            assert bbox and min(bbox[0], bbox[1], 256-bbox[2], 256-bbox[3]) >= 10, (card['id'], bbox)
            destination = ROOT/'assets/pastel/boss_cards'/f"{card['id']}.png"
            exports.append((image, destination)); files.append(destination.relative_to(ROOT).as_posix())
        provenance.append(dict(boss=owner, tool='built-in image_gen', prompt=item['prompt'], references=[Path(item['ref']).name, 'toxin.png'], source=source.name, source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(), grid=[2, 3], exports=files, processing='generate2dsprite.py process; 256px, centered shared scale .80, all components, reject edge touch; remove RGB magenta spill within distance 100'))
    for image, destination in exports:
        destination.parent.mkdir(parents=True, exist_ok=True)
        image.save(destination)
    path = ROOT/'assets/provenance.json'
    recorded = json.loads(path.read_text(encoding='utf-8'))
    recorded['boss_card_decks'] = provenance
    path.write_text(json.dumps(recorded, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    path = ROOT/'assets/manifest.json'
    manifest = json.loads(path.read_text(encoding='utf-8'))
    manifest['files'] = [dict(path=p.relative_to(ROOT).as_posix(), size=list(Image.open(p).size), sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in sorted((ROOT/'assets/pastel').rglob('*.png'))]
    manifest['runtime_pngs'] = len(manifest['files'])
    path.write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(bosses=len(inputs), exported=len(exports), runtime_pngs=manifest['runtime_pngs'], edge_touch=False)))

if __name__ == '__main__':
    main()
