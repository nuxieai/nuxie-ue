#!/usr/bin/env python3
"""Local confidence gate: compile real code and execute behavior tests; never source-substring checks."""
from pathlib import Path
import json
import os
import subprocess

root = Path(__file__).resolve().parent.parent
engine = Path(os.environ.get('UNREAL_ENGINE_ROOT', '/Users/Shared/Epic Games/UE_5.8'))
editor = engine / 'Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor'
build = engine / 'Engine/Build/BatchFiles/Mac/Build.sh'
project = root / 'Examples/NuxieLab/NuxieLab.uproject'
if not build.is_file() or not editor.is_file():
    raise SystemExit('Set UNREAL_ENGINE_ROOT to an installed UE 5.8 editor. Engine validation cannot be skipped.')
version = json.loads((engine / 'Engine/Build/Build.version').read_text())
if (version['MajorVersion'], version['MinorVersion']) != (5, 8):
    raise SystemExit('This release targets UE 5.8. Use the documented toolchain.')

def run(args, cwd=root, env=None):
    subprocess.run([str(arg) for arg in args], cwd=cwd, env=env, check=True)

run(['python3', root / 'scripts/bazel/sdk.py', 'test'])
run(['python3', root / 'scripts/link-example.py'])
run([build, 'NuxieLabEditor', 'Mac', 'Development', '-Project=' + str(project), '-architecture=arm64', '-NoHotReload', '-NoUBA'])
report = root / 'dist/check-report'
report.mkdir(parents=True, exist_ok=True)
index = report / 'index.json'
if index.exists(): index.unlink()
run([editor, project, '-Unattended', '-nowrite', '-NullRHI', '-NoSound', '-ExecCmds=Automation RunTests Nuxie.', '-TestExit=Automation Test Queue Empty', '-ReportExportPath=' + str(report), '-stdout'])
result = json.loads(index.read_text(encoding='utf-8-sig'))
required = {'Nuxie.Contract.DeferredBudget', 'Nuxie.Contract.SessionLifecycle', 'Nuxie.Contract.ValuesAndReceipts'}
passed = {test.get('fullTestPath') for test in result.get('tests', []) if test.get('state') == 'Success'}
if result.get('failed') or result.get('notRun') or not required.issubset(passed):
    raise SystemExit('Unreal automation did not pass every Nuxie test.')
run(['python3', root / 'scripts/bazel/sdk.py', 'test-android'])
run(['python3', root / 'scripts/prepare-android.py'])
run(['python3', root / 'scripts/bazel/sdk.py', 'test-ios'])
print('Nuxie: portable C++, Unreal editor, Android bridge, and iOS bridge checks passed.')
