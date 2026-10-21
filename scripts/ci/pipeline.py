#!/usr/bin/env python3
"""Compose Unreal confidence from independent platform command seams."""
import copy
import json
import os
from pathlib import Path
from cache_policy import cache_environment

ROOT = Path(__file__).resolve().parents[2]


def pipeline(environment=None):
    environment = os.environ if environment is None else environment
    version = environment.get('NUXIE_CI_RUNNER_POLICY_VERSION', '1')
    split = environment.get('NUXIE_SDK_SPLIT_LANES', '0')
    engine_check = environment.get('NUXIE_SDK_ENGINE_CHECK', '0')
    if engine_check not in ('0', '1'):
        raise ValueError('NUXIE_SDK_ENGINE_CHECK must be 0 or 1')
    topology = environment.get('NUXIE_CI_RUNNER_TOPOLOGY', 'final')
    if version not in ('', '1', '2') or split not in ('0', '1') or topology not in ('final', 'fallback'):
        raise ValueError('Unsupported runner policy or SDK split setting')
    if split == '1' and version != '2':
        raise ValueError('SDK split requires installed runner policy v2')
    if version == '2' and topology == 'fallback':
        raise ValueError('SDK native confidence requires the final 14 GiB Linux topology')
    document = json.loads((ROOT / '.buildkite/lanes.json').read_text())
    if split == '1':
        document['steps'][0]['agents']['nuxie-platform'] = 'linux'
        document['steps'][1:] = [
            {
                'label': ':android: Direct Unreal Android bridge',
                'command': '\n'.join([
                    'set -euo pipefail',
                    'source scripts/bazel/ci-cache.sh',
                    'python3 scripts/ci/android-toolchain.py',
                    'python3 scripts/bazel/sdk.py test-android',
                    'python3 scripts/prepare-android.py',
                ]) + '\n',
                'agents': {'queue': 'nuxie-pr', 'nuxie-platform': 'linux'},
                'timeout_in_minutes': 120,
            },
            {
                'label': ':apple: Direct Unreal Apple bridge',
                'command': '\n'.join([
                    'set -euo pipefail',
                    'source scripts/bazel/ci-cache.sh',
                    'python3 scripts/bazel/ios_simulator.py --minimum-os 17.0 -- python3 scripts/bazel/sdk.py test-ios',
                    'bash ThirdParty/IOS/scripts/build-framework.sh',
                ]) + '\n',
                'agents': {'queue': 'nuxie-pr', 'nuxie-platform': 'macos'},
                'timeout_in_minutes': 120,
            },
        ]
    if engine_check == '1':
        apple = document['steps'][-1]
        apple['command'] += '\n'.join([
            'export JAVA_HOME="${JAVA_HOME:-/opt/homebrew/opt/openjdk@21/libexec/openjdk.jdk/Contents/Home}"',
            'export ANDROID_HOME="${ANDROID_HOME:-/opt/homebrew/share/android-commandlinetools}"',
            'export ANDROID_SDK_ROOT="$ANDROID_HOME"',
            'export PATH="$JAVA_HOME/bin:$PATH"',
            'python3 scripts/ci/android-toolchain.py',
            'python3 scripts/bazel/ios_simulator.py --minimum-os 17.0 -- python3 scripts/check.py',
        ]) + '\n'
        apple['command'] += 'python3 scripts/package.py --output dist/release-package\n'
        apple['timeout_in_minutes'] = 240
        apple['artifact_paths'] = list(dict.fromkeys([*apple.get('artifact_paths', []), 'dist/check-report/**/*', 'dist/release-package/**/*']))
    if version == '2':
        document['steps'][0]['command'] = (
            'set -euo pipefail\n'
            "NUXIE_CI_RUNNER_POLICY_VERSION=1 python3 -B -m unittest discover -s scripts/bazel -p 'test_*.py'\n"
            "NUXIE_CI_RUNNER_POLICY_VERSION=1 python3 -B -m unittest discover -s scripts/ci -p 'test_*.py'\n"
            "NUXIE_CI_RUNNER_POLICY_VERSION=1 python3 -B scripts/test-buildkite-pipeline.py\n"
            + document['steps'][0]['command']
        )
        for step in document['steps']:
            platform = step['agents'].get('nuxie-platform', 'linux')
            step['agents']['nuxie-platform'] = platform
            step['agents']['nuxie-resources'] = 'mac-64g-v2' if platform == 'macos' else 'linux-14g-v2'
            step['concurrency'] = 1
            step['concurrency_group'] = 'nuxie/physical/mac-m2' if platform == 'macos' else 'nuxie/physical/proxmox-heavy'
            step['concurrency_method'] = 'eager'
            profile = 'macos-apple' if platform == 'macos' else 'linux-portable' if step is document['steps'][0] else 'linux-native'
            step['env'] = {
                'NUXIE_CI_RUNNER_POLICY_VERSION': '2',
                'NUXIE_CI_RUNNER_PROFILE': profile,
                **cache_environment(environment, profile),
            }
            step['command'] = step['command'].replace('      export NUXIE_BAZEL_JOBS=2\n', '').replace('export NUXIE_BAZEL_JOBS=2\n', '')
            step['artifact_paths'] = list(dict.fromkeys([*step.get('artifact_paths', []), 'output/pipeline/**/*', 'ThirdParty/*/lib/receipt.json']))
    return copy.deepcopy(document)


if __name__ == '__main__':
    print(json.dumps(pipeline()))
