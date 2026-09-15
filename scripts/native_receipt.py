"""Shared inventory and freshness validation for prepared native artifacts."""
from pathlib import Path
import hashlib


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def inputs(root, platform):
    paths = [root / 'NATIVE-PINS.json', root / 'scripts/write-native-receipt.py', root / 'scripts/native_receipt.py']
    if platform == 'ios':
        paths += [root / 'ThirdParty/IOS/Package.swift', root / 'ThirdParty/IOS/scripts/build-framework.sh']
        paths += sorted((root / 'ThirdParty/IOS/Sources').rglob('*.swift'))
    elif platform == 'android':
        paths += sorted(p for p in (root / 'ThirdParty/Android/bridge/src/main').rglob('*') if p.is_file())
        paths += [root / p for p in ('ThirdParty/Android/Nuxie_APL.xml', 'scripts/prepare-android.py',
                  'ThirdParty/Android/bridge/build.gradle.kts', 'ThirdParty/Android/settings.gradle.kts')]
    else:
        raise ValueError('Expected ios or android')
    return paths


def validate(root, platform, receipt):
    expected = {str(path.relative_to(root)) for path in inputs(root, platform)}
    if expected != set(receipt['inputs']):
        raise ValueError('Native source inventory changed; prepare ' + platform + ' again')
    for section in ('inputs', 'artifacts'):
        for relative, expected_hash in receipt[section].items():
            path = (root / relative).resolve()
            if not path.is_relative_to(root.resolve()) or not path.is_file():
                raise ValueError('Missing native input/artifact: ' + relative)
            if digest(path) != expected_hash:
                raise ValueError('Rebuild stale native input/artifact: ' + relative)
