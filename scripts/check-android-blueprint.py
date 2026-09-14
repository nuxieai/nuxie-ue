#!/usr/bin/env python3
"""Install the unconfigured BlueprintOnly APK and require its actual async error branch."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--serial', required=True)
parser.add_argument('--apk', required=True, type=Path)
args = parser.parse_args()
adb = shutil.which('adb') or str(Path(os.environ.get('ANDROID_HOME', '/opt/homebrew/share/android-commandlinetools')) / 'platform-tools/adb')
package = 'ai.nuxie.unreal.blueprintlab'

def run(*arguments):
    return subprocess.run([adb, '-s', args.serial, *arguments], check=True,
                          capture_output=True, text=True, timeout=120).stdout

run('install', '-r', str(args.apk.resolve()))
run('shell', 'am', 'force-stop', package)
run('shell', 'am', 'start', '-n', package + '/com.epicgames.unreal.GameActivity')
deadline = time.monotonic() + 60
while time.monotonic() < deadline:
    try:
        pid = run('shell', 'pidof', package).split()[0]
    except (subprocess.CalledProcessError, IndexError):
        time.sleep(1)
        continue
    log = run('logcat', '-d', '--pid=' + pid)
    if "module 'Nuxie' could not be found" in log or 'FATAL EXCEPTION' in log or 'Fatal error:' in log:
        raise SystemExit('FAIL: packaged Blueprint consumer could not start its Nuxie module.')
    if 'LogBlueprintUserMessages:' in log and 'Nuxie failed: inspect the typed Error output.' in log:
        print('PASS: packaged Nuxie module loaded and the unconfigured Blueprint async error branch executed.')
        break
    time.sleep(1)
else:
    raise SystemExit('FAIL: Blueprint async error branch did not execute within 60 seconds.')
