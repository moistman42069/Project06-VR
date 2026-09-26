"""Archive mod source and build dependencies, excluding game data and signing keys."""
import hashlib
import pathlib
import subprocess
import zipfile

root = pathlib.Path(__file__).resolve().parents[1]
version = '0.1.3'
files = set()
for folder in ['src', 'tools', 'tests', 'docs']:
    files.update(p for p in (root / folder).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
for name in ['README.md', 'AGENTS.md', '.gitignore', 'CMakeLists.txt',
             'evidence/apk-identity.json', 'evidence/bindings.json',
             'evidence/bridge-api-audit.json', 'evidence/input-callers.json',
             f'out/p06-quest-{version}-receipt.json', 'out/compatibility-audit.json']:
    files.add(root / name)
for folder in ['vendor/OpenXR-SDK', 'vendor/Dobby-stable']:
    base = root / folder
    names = subprocess.check_output(['git', '-C', str(base), 'ls-files', '-z']).decode().split('\0')
    files.update(base / name for name in names if name and (base / name).is_file())
files.update(p for p in (root / 'vendor/unity-xr-plugin/CommonHeaders').rglob('*') if p.is_file())
files.add(root / 'vendor/unity-xr-plugin/LICENSE')
files.add(root / 'vendor/font8x8/font8x8_basic.h')
archive = root / f'out/p06-quest-{version}-source.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for p in sorted(files):
        z.write(p, 'p06-quest-source/' + p.relative_to(root).as_posix())
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
with archive.open('rb') as stream:
    print(archive)
    print('SHA256:', hashlib.file_digest(stream, 'sha256').hexdigest())
