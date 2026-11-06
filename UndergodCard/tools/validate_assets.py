"""Check runtime asset references, hashes and WAV data using the standard library."""
from pathlib import Path
import hashlib, json, struct, wave
ROOT=Path(__file__).resolve().parents[1]

def main():
    manifest=json.loads((ROOT/'assets/manifest.json').read_text(encoding='utf-8'))
    actual={p.relative_to(ROOT).as_posix() for p in (ROOT/'assets/pastel').rglob('*.png')}
    recorded={item['path'].replace('\\','/') for item in manifest['files']}
    assert len(actual)==manifest['runtime_pngs'] and actual==recorded,'Image manifest does not match assets'
    for item in manifest['files']:
        path=ROOT/item['path'];data=path.read_bytes()
        assert hashlib.sha256(data).hexdigest()==item['sha256'],path
        assert data[:8]==b'\x89PNG\r\n\x1a\n' and list(struct.unpack('>II',data[16:24]))==item['size'],path
        assert data[24:26]==b'\x08\x06',(path,'PNG must be RGBA8')
    def read(name):return json.loads((ROOT/'data'/name).read_text(encoding='utf-8'))
    cards=read('cards.json');assert len(cards)==50
    for card in cards:assert f'assets/pastel/cards/{card["id"]}.png' in actual,card['id']
    boss_cards=read('boss_cards.json');boss_ids={e['id'] for e in read('enemies.json') if e['role']=='boss'}
    assert {deck['boss'] for deck in boss_cards}==boss_ids
    boss_art=set()
    for deck in boss_cards:
        assert len(deck['cards'])==6
        for card in deck['cards']:
            name=f"assets/pastel/boss_cards/{card['id']}.png"
            assert card['id'].startswith('boss_'+deck['boss']+'_') and name in actual and name not in boss_art
            boss_art.add(name)
    for kind in ['relics','potions']:
        for item in read(kind+'.json'):assert f'assets/pastel/{kind}/{item["id"]}.png' in actual,item['id']
    for character in read('classes.json'):
        for pose in ['idle','attack','hit','defend','ult_throw']:
            for frame in range(4):assert f'assets/pastel/characters/char_{character["sprite"]}_{pose}_{frame}.png' in actual
    for enemy in read('enemies.json'):
        assert f'assets/pastel/characters/char_{enemy["sprite"]}_idle_0.png' in actual,enemy['id']
        if enemy['role']=='boss':
            for pose in ['idle','attack']:
                for frame in range(4):assert f'assets/pastel/characters/char_{enemy["sprite"]}_{pose}_{frame}.png' in actual
    animations=read('skill_animations.json');assert len(animations['bodies'])==18 and len(animations['effects'])==17
    profiles=animations['profiles'];assert len(profiles)==98
    for card in cards:assert card['id'] in profiles
    for deck in boss_cards:
        for card in deck['cards']:assert card['id'] in profiles
    by_path={item['path']:item for item in manifest['files']}
    for body in animations['bodies']:
        paths=[f'assets/pastel/characters/char_{body["actor"]}_{body["pose"]}_{n}.png' for n in range(4)]
        assert all(p in actual for p in paths) and len({by_path[p]['sha256'] for p in paths})==4
        assert 0<body['emitterX']<512 and 0<body['emitterY']<512
    for effect in animations['effects']:
        paths=[f'assets/pastel/skill_fx/{effect}_{n}.png' for n in range(4)]
        assert all(p in actual for p in paths) and len({by_path[p]['sha256'] for p in paths})==4
        for path in paths:
            x,y,right,bottom=by_path[path]['bounds'];assert 0<x<right<512 and 0<y<bottom<512
    art=json.loads((ROOT/'assets/provenance.json').read_text(encoding='utf-8'))['skill_animations']['groups']
    assert len(art)==35 and all(not group['edge_touch_frames'] and group['unique_frames']==4 for group in art)
    hashes={item['path']:item['sha256'] for item in manifest['audio_files']}
    audio=read('audio.json');assert len(audio)==12
    for item in audio:
        path=ROOT/item['file'];assert hashes[item['file']]==hashlib.sha256(path.read_bytes()).hexdigest(),path
        with wave.open(str(path),'rb') as wav:
            assert wav.getnchannels()==1 and wav.getsampwidth()==2 and wav.getframerate()==22050,path
            raw=wav.readframes(wav.getnframes());samples=struct.unpack('<'+'h'*(len(raw)//2),raw)
            peak=max(abs(value) for value in samples);assert 0<peak<32767,(path,peak)
    report=dict(pngs=len(actual),cards=len(cards),boss_cards=len(boss_art),boss_decks=len(boss_cards),skill_profiles=len(profiles),body_animations=len(animations['bodies']),fx_animations=len(animations['effects']),new_frames=140,relics=len(read('relics.json')),potions=len(read('potions.json')),audio=len(audio),hashes=True,references=True,pcm=True)
    target=ROOT/'artifacts/asset-validation.json';target.parent.mkdir(parents=True,exist_ok=True);target.write_text(json.dumps(report,indent=2)+'\n')
    print('ASSETS '+json.dumps(report))

if __name__=='__main__':main()
