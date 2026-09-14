#!/usr/bin/env python3
"""Link the source plugin for SDK contributors; standalone consumers install a prepared archive."""
from pathlib import Path
root = Path(__file__).resolve().parent.parent
link = root / 'Examples/NuxieLab/Plugins/Nuxie'
link.parent.mkdir(parents=True, exist_ok=True)
if link.is_symlink() and link.resolve() == root:
    pass
elif link.exists() or link.is_symlink():
    raise SystemExit('Preserving an existing example plugin. Remove it yourself or use a fresh example checkout.')
else:
    link.symlink_to('../../..', target_is_directory=True)
print('Source plugin linked:', link)
