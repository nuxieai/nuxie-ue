#!/usr/bin/env python3
"""Execute emitted commands with recorded tools; check platform ownership."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent / 'ci'))
from pipeline import pipeline

ROOT = Path(__file__).resolve().parents[1]


class PipelineTests(unittest.TestCase):
    def test_default_graph_retains_full_original_readiness(self):
        expected = json.loads((ROOT / '.buildkite/lanes.json').read_text())
        self.assertEqual(pipeline({}), expected)
        self.assertEqual(len(expected['steps']), 2)

    def execute(self, step):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'scripts/bazel').mkdir(parents=True)
            (root / 'scripts/bazel/ci-cache.sh').write_text('export NUXIE_BAZEL_BATCH=1\n')
            recorder = root / 'record.py'
            recorder.write_text('import json, os, sys\nwith open(os.environ["CALLS"], "a") as log: log.write(json.dumps(sys.argv[1:]) + "\\n")\n')
            calls = root / 'calls.jsonl'
            sdkmanager = root / 'android/cmdline-tools/latest/bin/sdkmanager'
            sdkmanager.parent.mkdir(parents=True)
            sdkmanager.write_text('#!' + sys.executable + '\n' + recorder.read_text())
            sdkmanager.chmod(0o755)
            functions = 'record() { "$REAL_PYTHON" "$RECORDER" "$@"; }\npython3() { record python3 "$@"; }\nbash() { record bash "$@"; }\n'
            subprocess.run(['/bin/bash', '-c', functions + step['command']], cwd=root,
                           env={**os.environ, 'REAL_PYTHON': sys.executable, 'RECORDER': str(recorder), 'CALLS': str(calls), 'ANDROID_HOME': str(root / 'android')},
                           check=True, timeout=10)
            return [json.loads(line) for line in calls.read_text().splitlines()]

    def test_split_executes_complete_inventory_once_on_owning_platform(self):
        graph = pipeline({'NUXIE_CI_RUNNER_POLICY_VERSION': '2', 'NUXIE_SDK_SPLIT_LANES': '1'})
        steps = graph['steps']
        self.assertEqual(len(steps), 3)
        calls = [self.execute(step) for step in steps]
        self.assertEqual(calls[0], [
            ['python3', '-B', '-m', 'unittest', 'discover', '-s', 'scripts/bazel', '-p', 'test_*.py'],
            ['python3', '-B', '-m', 'unittest', 'discover', '-s', 'scripts/ci', '-p', 'test_*.py'],
            ['python3', '-B', 'scripts/test-buildkite-pipeline.py'],
            ['python3', 'scripts/bazel/sdk.py', 'test'],
        ])
        self.assertEqual(calls[1], [
            ['python3', 'scripts/ci/android-toolchain.py'],
            ['python3', 'scripts/bazel/sdk.py', 'test-android'],
            ['python3', 'scripts/prepare-android.py'],
        ])
        self.assertEqual(calls[2], [
            ['python3', 'scripts/bazel/ios_simulator.py', '--minimum-os', '17.0', '--', 'python3', 'scripts/bazel/sdk.py', 'test-ios'],
            ['bash', 'ThirdParty/IOS/scripts/build-framework.sh'],
        ])
        for step, platform in zip(steps, ('linux', 'linux', 'macos')):
            self.assertEqual(step['agents']['nuxie-platform'], platform)
            self.assertEqual(step['agents']['nuxie-resources'], 'mac-64g-v2' if platform == 'macos' else 'linux-14g-v2')
            self.assertEqual(step['concurrency'], 1)
            self.assertEqual(step['concurrency_group'], 'nuxie/physical/mac-m2' if platform == 'macos' else 'nuxie/physical/proxmox-heavy')
            self.assertEqual(step['env']['NUXIE_BAZEL_REMOTE_CACHE_MODE'], 'off')
        self.assertEqual(steps[0]['env']['NUXIE_CI_RUNNER_PROFILE'], 'linux-portable')
        self.assertEqual(steps[1]['env']['NUXIE_CI_RUNNER_PROFILE'], 'linux-native')
        self.assertEqual(steps[2]['env']['NUXIE_CI_RUNNER_PROFILE'], 'macos-apple')

    def test_engine_release_check_executes_full_gate_and_packager_on_serialized_mac(self):
        for split in ('0', '1'):
            steps = pipeline({'NUXIE_CI_RUNNER_POLICY_VERSION': '2',
                              'NUXIE_SDK_SPLIT_LANES': split,
                              'NUXIE_SDK_ENGINE_CHECK': '1'})['steps']
            apple = steps[-1]
            self.assertEqual(apple['agents']['nuxie-platform'], 'macos')
            self.assertEqual(apple['concurrency_group'], 'nuxie/physical/mac-m2')
            self.assertEqual(self.execute(apple)[-2:], [
                ['python3', 'scripts/bazel/ios_simulator.py', '--minimum-os', '17.0', '--', 'python3', 'scripts/check.py'],
                ['python3', 'scripts/package.py', '--output', 'dist/release-package'],
            ])
            self.assertIn('dist/check-report/**/*', apple['artifact_paths'])
            self.assertIn('dist/release-package/**/*', apple['artifact_paths'])
        with self.assertRaises(ValueError):
            pipeline({'NUXIE_SDK_ENGINE_CHECK': 'yes'})

    def test_half_activated_or_unknown_settings_fail_closed(self):
        for environment in ({'NUXIE_SDK_SPLIT_LANES': '1'}, {'NUXIE_CI_RUNNER_POLICY_VERSION': '3'},
                            {'NUXIE_SDK_SPLIT_LANES': 'yes'}, {'NUXIE_CI_RUNNER_TOPOLOGY': 'unknown'},
                            {'NUXIE_CI_RUNNER_POLICY_VERSION': '2', 'NUXIE_CI_RUNNER_TOPOLOGY': 'fallback'}):
            with self.subTest(environment=environment), self.assertRaises(ValueError):
                pipeline(environment)


if __name__ == '__main__':
    unittest.main()
