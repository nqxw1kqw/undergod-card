"""Import ImageGen sheets with the sprite skill processor. Does not generate art."""
from pathlib import Path
import argparse, hashlib, json, shutil, subprocess, sys
from PIL import Image, ImageDraw
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / 'artifacts/art-v1'
ASSETS = ROOT / 'assets/pastel'

def read(name):
    return json.loads((ROOT / 'data' / name).read_text(encoding='utf-8'))

def export(source, destination, feet=False):
    image = Image.open(source).convert('RGBA')
    pixels=np.array(image)
    # Remove magenta spill on extracted alpha edges, preserving purple clothes.
    keyed=(pixels[:,:,0]>160)&(pixels[:,:,2]>150)&(pixels[:,:,1]<95)&(np.abs(pixels[:,:,0].astype(int)-pixels[:,:,2].astype(int))<75)
    pixels[keyed,3]=0
    image=Image.fromarray(pixels)
    if feet and image.getbbox():
        # Translate alpha only; every actor shares the original chick feet anchor.
        bottom = image.getbbox()[3]
        canvas = Image.new('RGBA', image.size)
        canvas.paste(image, (0, 456-bottom))
        image = canvas
    destination.parent.mkdir(parents=True, exist_ok=True)
    image.save(destination)

def normalize_actors():
    report={}
    def body_height(image):
        rows=(np.array(image.getchannel('A'))>128).sum(axis=1)
        visible=np.flatnonzero(rows>=max(24,int(rows.max()*.20)))
        return int(visible[-1]-visible[0]+1)
    for actor in ['mage','ninja','frog_king','snake','hawk','fox','undergod']:
        actions={}
        for action in ['idle','attack','hit','defend','ult_throw']:
            paths=list((ASSETS/'characters').glob(f'char_{actor}_{action}_*.png'))
            if not paths: continue
            images=[Image.open(p).convert('RGBA') for p in paths]
            height=float(np.median([body_height(im) for im in images]))
            width=max(im.getbbox()[2]-im.getbbox()[0] for im in images)
            full_height=max(im.getbbox()[3]-im.getbbox()[1] for im in images)
            actions[action]=(paths,images,height,min(488/width,444/full_height)*height)
        target=min(actions['idle'][2],min(item[3] for item in actions.values()))
        report[actor]={'body_height':round(target,2),'actions':{}}
        for action,(paths,images,height,_) in actions.items():
            factor=target/height;heights=[]
            for path,im in zip(paths,images):
                im=im.crop(im.getbbox());im=im.resize((round(im.width*factor),round(im.height*factor)),Image.Resampling.LANCZOS)
                canvas=Image.new('RGBA',(512,512));canvas.paste(im,((512-im.width)//2,456-im.height));canvas.save(path);heights.append(body_height(canvas))
            report[actor]['actions'][action]=heights
    (WORK/'normalization.json').write_text(json.dumps(report,indent=2)+'\n')

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--processor', type=Path, default=Path.home()/'.codex/skills/generate2dsprite/scripts/generate2dsprite.py')
    args = parser.parse_args()
    specs = json.loads((WORK/'inputs.json').read_text(encoding='utf-8'))
    cards = read('cards.json')
    relics = read('relics.json')
    potions = read('potions.json')
    groups = {
        'card_atlas_0': (4,4,256,[f'cards/{c["id"]}.png' for c in cards[4:20]]),
        'card_atlas_1': (4,4,256,[f'cards/{c["id"]}.png' for c in cards[20:36]]),
        'card_atlas_2': (4,4,256,[f'cards/{c["id"]}.png' for c in cards[36:50]]+['vfx/feather.png','vfx/kunai.png']),
        'enemies_idle': (3,3,512,[f'characters/char_{name}_idle_0.png' for name in ['froglet','toad','reed_frog','moss_toad','marsh_guard','poison_frog','elite_frog','elder_toad','tadpole']]),
        'relic_atlas_0': (4,4,256,[f'relics/{r["id"]}.png' for r in relics[:16]]),
        'relic_atlas_1': (3,3,256,['relics/war_drum.png']+[f'potions/{p["id"]}.png' for p in potions]),
    }
    for actor in ['mage','ninja','frog_king','snake','hawk','fox','undergod']:
        for action in ['idle','attack','hit','defend','ult_throw']:
            groups[f'{actor}_{action}']=(2,2,512,[f'characters/char_{actor}_{action}_{n}.png' for n in range(4)])
    for key,spec in specs.items():
        if key not in groups: continue
        rows,cols,size,targets = groups[key]
        out=WORK/'processed'/key
        raw=WORK/'raw'/f'{key}.png'; raw.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(spec['path'],raw)
        if spec.get('prompt'): (raw.with_suffix('.prompt.txt')).write_text(spec['prompt'],encoding='utf-8')
        processor_input=raw
        if key=='relic_atlas_0':
            source=Image.open(raw).convert('RGB');grid=Image.new('RGB',(1536,1536),(255,0,255))
            xs=[0,313,627,940,source.width];ys=[0,313,615,945,source.height]
            for row in range(4):
                for col in range(4):
                    cell=source.crop((xs[col],ys[row],xs[col+1],ys[row+1]))
                    grid.paste(cell,(col*384+(384-cell.width)//2,row*384+(384-cell.height)//2))
            processor_input=raw.with_name('relic_atlas_0-aligned-grid.png');grid.save(processor_input)
        if key=='undergod_attack':
            source=Image.open(raw).convert('RGB');grid=Image.new('RGB',(1536,1536),(255,0,255))
            xs=[0,600,source.width];ys=[0,source.height//2,source.height]
            for row in range(2):
                for col in range(2):
                    cell=source.crop((xs[col],ys[row],xs[col+1],ys[row+1]))
                    grid.paste(cell,(col*768+(768-cell.width)//2,row*768+(768-cell.height)//2))
            processor_input=raw.with_name('undergod_attack-aligned-grid.png');grid.save(processor_input)
        if key=='enemies_idle':
            # The generated atlas has unequal row gutters. Repack intact cells
            # before uniform grid extraction; the helmet crosses the 2/3 line.
            source=Image.open(raw).convert('RGB'); grid=Image.new('RGB',(1536,1536),(255,0,255))
            boundaries=[0,400,790,source.height]
            for row in range(3):
                for col in range(3):
                    cell=source.crop((col*418,boundaries[row],(col+1)*418,boundaries[row+1]))
                    grid.paste(cell,(col*512+(512-cell.width)//2,row*512+(512-cell.height)//2))
            processor_input=raw.with_name('enemies_idle-aligned-grid.png');grid.save(processor_input)
        if not (out/'pipeline-meta.json').exists():
            command=[sys.executable,str(args.processor),'process','--input',str(processor_input),'--target','asset','--mode','sheet','--output-dir',str(out),'--rows',str(rows),'--cols',str(cols),'--cell-size',str(size),'--fit-scale','.74' if size==512 else '.82','--align','feet' if size==512 else 'center','--component-mode','largest' if size==512 else 'all','--min-component-area','20','--reject-edge-touch']
            if rows==cols==2: command+=['--shared-scale']
            subprocess.run(command,check=True,stdout=subprocess.DEVNULL)
        meta=json.loads((out/'pipeline-meta.json').read_text())
        if meta['edge_touch_frames']: raise ValueError(f'{key}: clipped raw cells')
        for label,target in zip(meta['frame_labels'],targets): export(out/f'{label}.png',ASSETS/target,size==512)
        print(f'{key}: {len(targets)} imported')
    normalize_actors()
    files=[]
    for path in sorted(ASSETS.rglob('*.png')):
        with Image.open(path) as im:
            assert im.mode=='RGBA' and im.getbbox(),path
            assert im.size in [(256,256),(512,512)],(path,im.size)
            alpha=im.getchannel('A'); assert alpha.getextrema()==(0,255),path
            if 'characters' in path.parts:
                box=im.getbbox(); assert box[0]>0 and box[1]>0 and box[2]<512 and box[3]<=457,(path,box)
            files.append(dict(path=path.relative_to(ROOT).as_posix(),size=list(im.size),sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    audio=[dict(path=p.relative_to(ROOT).as_posix(),sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in sorted((ROOT/'assets/audio').glob('*.wav'))]
    (ROOT/'assets/manifest.json').write_text(json.dumps(dict(player='chick',runtime_pngs=len(files),files=files,audio_files=audio),indent=2)+'\n',encoding='utf-8')
    provenance=[dict(group=key,source_file=Path(spec['path']).name,source_sha256=hashlib.sha256((WORK/'raw'/f'{key}.png').read_bytes()).hexdigest(),prompt=spec.get('prompt',''),exports=groups[key][3]) for key,spec in specs.items() if key in groups]
    (ROOT/'assets/provenance.json').write_text(json.dumps(dict(generator='built-in ImageGen',processor='generate2dsprite.py process; grid extraction, chroma key, alpha spill cleanup, shared body scale, feet y=456',groups=provenance),ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    # Contact sheet is a QA artifact, never a runtime asset.
    thumbs=Image.new('RGB',(960,((len(files)+9)//10)*112),(30,34,38)); draw=ImageDraw.Draw(thumbs)
    for n,item in enumerate(files):
        im=Image.open(ROOT/item['path']).convert('RGBA'); im.thumbnail((92,88));x=(n%10)*96;y=(n//10)*112
        thumbs.paste(im,(x+(96-im.width)//2,y),im);draw.text((x,y+90),Path(item['path']).stem[:15],fill='white')
    thumbs.save(WORK/'contact.png')
    print(f'Manifest: {len(files)} PNG, {len(audio)} WAV')

if __name__=='__main__': main()
