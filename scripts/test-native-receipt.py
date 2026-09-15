#!/usr/bin/env python3
import tempfile
import unittest
from pathlib import Path
from native_receipt import digest, inputs, validate


class NativeReceiptTests(unittest.TestCase):
    def test_added_deleted_and_modified_sources_require_preparation(self):
        for platform, source in [('ios', 'ThirdParty/IOS/Sources/Bridge.swift'),
                                 ('android', 'ThirdParty/Android/bridge/src/main/Bridge.kt')]:
            with self.subTest(platform=platform), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                original = root / source
                original.parent.mkdir(parents=True)
                original.write_text('original implementation')
                for path in inputs(root, platform):
                    if not path.exists():
                        path.parent.mkdir(parents=True, exist_ok=True)
                        path.write_text('build input')
                artifact = root / 'prepared.bin'
                artifact.write_bytes(b'compiled artifact')
                receipt = {'inputs': {str(p.relative_to(root)): digest(p) for p in inputs(root, platform)},
                           'artifacts': {'prepared.bin': digest(artifact)}}
                validate(root, platform, receipt)
                added = original.with_name('New' + original.name)
                added.write_text('new implementation')
                with self.assertRaisesRegex(ValueError, 'inventory changed'):
                    validate(root, platform, receipt)
                added.unlink()
                original.unlink()
                with self.assertRaisesRegex(ValueError, 'inventory changed'):
                    validate(root, platform, receipt)
                original.write_text('changed implementation')
                with self.assertRaisesRegex(ValueError, 'stale'):
                    validate(root, platform, receipt)
                original.write_text('original implementation')
                artifact.write_bytes(b'changed artifact')
                with self.assertRaisesRegex(ValueError, 'stale'):
                    validate(root, platform, receipt)


if __name__ == '__main__':
    unittest.main()
