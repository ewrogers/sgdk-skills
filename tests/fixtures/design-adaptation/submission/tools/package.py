#!/usr/bin/env python3
"""Package the already-built and verified ROM plus reviewable source."""
import hashlib
import json
import shutil
import zipfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
release = root / 'release'
release.mkdir(exist_ok=True)
shutil.copyfile(root / 'out/rom.bin', release / 'last-service.bin')
files = [root / f for f in ('README.md', 'ENGINEERING.md', 'LICENSE',
                          'LICENSE-SGDK.txt', 'build.sh', '.gitignore')]
for directory in ('src', 'inc', 'tests', 'tools', 'logs'):
    files += sorted(p for p in (root / directory).rglob('*') if p.is_file())
files += [release / 'last-service.bin']
manifest = {
    'name': 'Last Service', 'version': '1.0',
    'toolchain_image': 'ewrogers/sgdk@sha256:1d8dd3f289ef9ed1c7186b68b47a1977bc02b2bcbd72903e7682f6b19ce7d0df',
    'validation': 'Clean release: zero warnings; host tests pass; ROM inspection valid, no findings.',
    'playtesting': 'Not run; caller handles emulator/hardware checks.',
    'files': {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
              for p in files},
}
(release / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
files.append(release / 'manifest.json')
archive = release / 'last-service-1.0.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as package:
    for path in sorted(files):
        info = zipfile.ZipInfo('last-service/' + str(path.relative_to(root)),
                               date_time=(2026, 10, 16, 0, 0, 0))
        info.external_attr = (path.stat().st_mode & 0xffff) << 16
        info.compress_type = zipfile.ZIP_DEFLATED
        package.writestr(info, path.read_bytes())
print(archive)
print('ROM SHA-256:', manifest['files']['release/last-service.bin'])
