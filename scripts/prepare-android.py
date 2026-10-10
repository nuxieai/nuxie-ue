#!/usr/bin/env python3
"""Prepare the direct Bazel bridge and verified pinned Maven artifacts for Unreal."""
from pathlib import Path
import shutil
import subprocess
import sys
root = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(root / 'scripts/bazel'))
from sdk import android_build, copy_maven
artifact, manifest = android_build()
copy_maven(manifest, root / 'ThirdParty/Android/maven')
output = root / 'ThirdParty/Android/lib/nuxie-unreal-bridge.aar'
output.parent.mkdir(parents=True, exist_ok=True)
shutil.copyfile(artifact, output)
subprocess.run([sys.executable, str(root / 'scripts/write-native-receipt.py'), 'android'], check=True)
