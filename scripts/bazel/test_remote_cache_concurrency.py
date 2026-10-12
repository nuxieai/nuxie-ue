"""Actual cache argv for the bounded proxy connection budget; no network work."""

import importlib.util
import os
from pathlib import Path
import unittest
from unittest.mock import patch


spec = importlib.util.spec_from_file_location("remote_cache", Path(__file__).with_name("remote-cache.py"))
remote_cache = importlib.util.module_from_spec(spec)
spec.loader.exec_module(remote_cache)
PREFIX = "NUXIE_BAZEL_REMOTE_CACHE_"


class RemoteCacheConcurrencyTest(unittest.TestCase):
    def test_each_enabled_mode_emits_one_connection_and_preserves_upload_policy(self):
        for mode in ("read", "write"):
            with self.subTest(mode=mode), patch.dict(os.environ, {
                PREFIX + "MODE": mode,
                PREFIX + "ELIGIBLE": "1",
                PREFIX + "DOMAIN": "sha256:" + "a" * 64,
                PREFIX + "URL": "https://cache.example:8443",
            }, clear=True), patch.object(remote_cache, "ca_file", return_value="/fixture/reviewed-leaf.pem"), \
                    patch.object(remote_cache, "credential", return_value={}) as credential:
                argv = remote_cache.flags()
            self.assertEqual([arg for arg in argv if arg.startswith("--remote_max_connections=")],
                             ["--remote_max_connections=1"])
            self.assertIn("--remote_upload_local_results=" + str(mode == "write").lower(), argv)
            self.assertIn("--remote_cache=https://cache.example:8443", argv)
            credential.assert_called_once_with()

    def test_default_and_explicit_off_do_not_add_connection_or_read_credentials(self):
        for env, expected in (({}, []), ({PREFIX + "MODE": "off"}, ["--remote_cache="])):
            with self.subTest(env=env), patch.dict(os.environ, env, clear=True), \
                    patch.object(remote_cache, "ca_file", side_effect=AssertionError("off must not inspect TLS")), \
                    patch.object(remote_cache, "credential", side_effect=AssertionError("off must not read credentials")):
                self.assertEqual(remote_cache.flags(), expected)


if __name__ == "__main__":
    unittest.main()
