"""Validate the exact release ZIP against the current runtime inputs."""
from pathlib import Path, PurePosixPath
import hashlib, json, zipfile
ROOT=Path(__file__).resolve().parents[1]
archive=ROOT/'release/UndergodCard-DxLib-Windows-x64.zip'
with zipfile.ZipFile(archive) as bundle:
    assert bundle.testzip() is None,'ZIP CRC failure'
    files={item.filename for item in bundle.infolist() if not item.is_dir()}
    assert all(not PurePosixPath(name).is_absolute() and '..' not in PurePosixPath(name).parts for name in files)
    manifest=json.loads(bundle.read('assets/manifest.json'))
    runtime={item['path']:item['sha256'] for item in manifest['files']+manifest['audio_files']}
    for name,sha in runtime.items():assert hashlib.sha256(bundle.read(name)).hexdigest()==sha,name
    expected=set(runtime)|{'assets/manifest.json','assets/provenance.json','assets/pastel/STYLE.md','UndergodCard.exe','README.txt','licenses/nlohmann-json-MIT.txt','licenses/DxLib-notice.txt','licenses/THIRD_PARTY.md'}
    expected|={p.relative_to(ROOT).as_posix() for p in (ROOT/'data').glob('*.json')}
    assert files==expected,dict(extra=sorted(files-expected),missing=sorted(expected-files))
    for name in files:
        if name.startswith(('assets/','data/')):assert bundle.read(name)==(ROOT/name).read_bytes(),name
    exe=bundle.read('UndergodCard.exe');assert exe[:2]==b'MZ' and exe==(ROOT/'bin/Release/UndergodCard.exe').read_bytes()
    guide=bundle.read('README.txt').decode('utf-8');assert 'v1.2.0-preview' in guide and 'profile.json' in guide and 'Attack' in guide
    report=dict(files=len(files),pngs=len(manifest['files']),wavs=len(manifest['audio_files']),data_files=len(list((ROOT/'data').glob('*.json'))),crc=True,exact_manifest=True,exe_sha256=hashlib.sha256(exe).hexdigest(),zip_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(),zip_bytes=archive.stat().st_size)
    (ROOT/'artifacts/package-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PACKAGE '+json.dumps(report))
