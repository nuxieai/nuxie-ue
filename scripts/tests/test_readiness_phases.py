import importlib.util
from pathlib import Path
import sys
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'scripts'))
spec=importlib.util.spec_from_file_location('ue_readiness',ROOT/'scripts/check.py')
check=importlib.util.module_from_spec(spec);spec.loader.exec_module(check)

class ReadinessPhaseTests(unittest.TestCase):
    def test_portable_cpp_checks_do_not_open_engine_or_mobile_toolchains(self):
        calls=[]
        with patch.object(check,'run',side_effect=calls.append),patch.object(check,'engine',side_effect=AssertionError('engine started')):
            check.check('portable')
        self.assertEqual(calls,[['python3',ROOT/'scripts/bazel/sdk.py','test']])

    def test_all_phases_keep_cpp_android_ios_and_engine_oracles(self):
        calls=[]
        with patch.object(check,'run',side_effect=calls.append),patch.object(check,'engine',side_effect=lambda:calls.append(['engine'])):
            check.check()
        self.assertEqual(calls,[['python3',ROOT/'scripts/bazel/sdk.py','test'],['python3',ROOT/'scripts/bazel/sdk.py','test-android'],['python3',ROOT/'scripts/prepare-android.py'],['python3',ROOT/'scripts/bazel/sdk.py','test-ios'],['engine']])
