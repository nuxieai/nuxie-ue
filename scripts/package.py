#!/usr/bin/env python3
"""Build a plugin archive from a bounded staging tree, never from example plugin symlinks."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import tempfile
from native_receipt import validate as validate_native_receipt

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser()
parser.add_argument('--platforms', default='IOS+Android+Mac')
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
output = args.output.resolve()
if output.exists(): raise SystemExit('Preserving existing output; choose a new output directory.')
engine = Path(os.environ.get('UNREAL_ENGINE_ROOT', '/Users/Shared/Epic Games/UE_5.8'))
version = json.loads((engine / 'Engine/Build/Build.version').read_text())
if (version['MajorVersion'], version['MinorVersion']) != (5, 8): raise SystemExit('Use the qualified UE 5.8 toolchain.')
pins = json.loads((root / 'NATIVE-PINS.json').read_text())
for platform in ('IOS', 'Android'):
    receipt = json.loads((root / f'ThirdParty/{platform}/lib/receipt.json').read_text())
    if platform == 'IOS' and receipt.get('configuration') != 'Release':
        raise SystemExit('Distribution requires Release iOS artifacts; run bash ThirdParty/IOS/scripts/build-framework.sh without the Debug override.')
    try:
        validate_native_receipt(root, platform.lower(), receipt)
    except ValueError as error:
        raise SystemExit(str(error)) from error
(root / '.native').mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='package-', dir=root / '.native') as temporary:
    stage = Path(temporary) / 'Nuxie'
    stage.mkdir()
    for name in ('Source', 'Config', 'docs'):
        shutil.copytree(root / name, stage / name)
    for name in ('Nuxie.uplugin', 'NATIVE-PINS.json', 'README.md', 'LICENSE'):
        shutil.copy2(root / name, stage / name)
    for name in ('Android/lib', 'Android/maven', 'IOS/lib', 'Notices'):
        shutil.copytree(root / 'ThirdParty' / name, stage / 'ThirdParty' / name)
    shutil.copy2(root / 'ThirdParty/Android/Nuxie_APL.xml', stage / 'ThirdParty/Android/Nuxie_APL.xml')
    # UE 5.8 UBA's Mac linker detour can stall. This scoped supported configuration uses the local toolchain.
    environment = dict(os.environ, UnrealBuildTool_BuildConfiguration__bAllowUBAExecutor='false')
    subprocess.run([str(engine / 'Engine/Build/BatchFiles/RunUAT.sh'), 'BuildPlugin',
        '-Plugin=' + str(stage / 'Nuxie.uplugin'), '-Package=' + str(output),
        '-TargetPlatforms=' + args.platforms, '-Architecture_Mac=arm64', '-Architecture_Android=arm64', '-NoP4'], env=environment, check=True)
# BuildPlugin clears EnabledByDefault; restore explicit opt-in before hashing.
# Otherwise content-only projects can reuse UnrealGame without linking Nuxie.
descriptor_path = output / 'Nuxie.uplugin'
descriptor = json.loads(descriptor_path.read_text())
descriptor['EnabledByDefault'] = json.loads((root / 'Nuxie.uplugin').read_text())['EnabledByDefault']
descriptor_path.write_text(json.dumps(descriptor, indent=2) + '\n')
manifest = {'sdk': pins['version'], 'pins': pins, 'engine': version, 'platforms': args.platforms.split('+'), 'files': {}}
for path in sorted(output.rglob('*')):
    if path.is_file():
        with path.open('rb') as stream: manifest['files'][str(path.relative_to(output))] = hashlib.file_digest(stream, 'sha256').hexdigest()
(output / 'ARTIFACT-MANIFEST.json').write_text(json.dumps(manifest, indent=2) + '\n')
print('Built plugin:', output)
