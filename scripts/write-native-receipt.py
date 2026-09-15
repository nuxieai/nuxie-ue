#!/usr/bin/env python3
"""Record prepared native artifacts and their exact source inputs."""
from pathlib import Path
from native_receipt import digest, inputs as source_inputs
import json
import sys
root = Path(__file__).resolve().parent.parent
platform = sys.argv[1]
if platform not in ('ios', 'android'): raise SystemExit('Expected ios or android')
inputs = source_inputs(root, platform)
if platform == 'ios':
    artifacts = sorted((root / 'ThirdParty/IOS/lib').glob('*/NuxieUnrealBridge.embeddedframework.zip'))
    if len(artifacts) != 2: raise SystemExit('Both iOS device and simulator archives are required')
    receipt = root / 'ThirdParty/IOS/lib/receipt.json'
else:
    artifacts = [root / 'ThirdParty/Android/lib/nuxie-unreal-bridge.aar']
    artifacts += sorted(p for p in (root / 'ThirdParty/Android/maven').rglob('*') if p.is_file())
    if len(artifacts) < 2: raise SystemExit('Pinned Android Maven artifacts are required')
    receipt = root / 'ThirdParty/Android/lib/receipt.json'
metadata = {}
if platform == 'ios':
    if len(sys.argv) != 3 or sys.argv[2] not in ('Debug', 'Release'): raise SystemExit('Specify the actual iOS build configuration: Debug or Release')
    metadata['configuration'] = sys.argv[2]
receipt.write_text(json.dumps({'contract': 1, **metadata, 'inputs': {str(p.relative_to(root)): digest(p) for p in inputs},
    'artifacts': {str(p.relative_to(root)): digest(p) for p in artifacts}}, indent=2) + '\n')
print('Recorded', receipt.relative_to(root))
