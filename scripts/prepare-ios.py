#!/usr/bin/env python3
"""Stage direct Bazel XCFramework slices in Unreal's embedded-framework ZIP layout."""
from pathlib import Path
import os
import plistlib
import subprocess
import sys
import tempfile
import zipfile
root = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root / 'scripts/bazel'))
from sdk import ios_xcframework

configuration = os.environ.get('NUXIE_IOS_BUILD_CONFIGURATION', 'Release')
if configuration not in ('Debug', 'Release'):
    raise ValueError('Use Debug or Release for NUXIE_IOS_BUILD_CONFIGURATION')
product = ios_xcframework(configuration)
info = plistlib.loads((product / 'Info.plist').read_bytes())
for item in info['AvailableLibraries']:
    selected = 'simulator' if item.get('SupportedPlatformVariant') == 'simulator' else 'ios'
    framework = product / item['LibraryIdentifier'] / item['LibraryPath']
    if framework.suffix != '.framework' or not (framework / 'Nuxie_Nuxie.bundle').is_dir():
        raise ValueError('The prepared bridge slice must include its native resource bundle')
    destination = root / 'ThirdParty/IOS/lib' / selected
    destination.mkdir(parents=True, exist_ok=True)
    output = destination / 'NuxieUnrealBridge.embeddedframework.zip'
    with tempfile.NamedTemporaryFile(prefix='.framework-', dir=destination, delete=False) as temporary:
        temporary_path = Path(temporary.name)
    try:
        with zipfile.ZipFile(temporary_path, 'w', zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(framework.rglob('*')):
                if path.is_file():
                    if path.is_symlink():
                        raise ValueError('iOS framework slices must not contain symlinks')
                    name = 'NuxieUnrealBridge.embeddedframework/NuxieUnrealBridge.framework/' + path.relative_to(framework).as_posix()
                    entry = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
                    entry.compress_type = zipfile.ZIP_DEFLATED
                    entry.external_attr = 0o100644 << 16
                    archive.writestr(entry, path.read_bytes())
        temporary_path.replace(output)
    finally:
        temporary_path.unlink(missing_ok=True)
subprocess.run([sys.executable, str(root / 'scripts/write-native-receipt.py'), 'ios', configuration], check=True)
