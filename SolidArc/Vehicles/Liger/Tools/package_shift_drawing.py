"""Package Shift drawing only after current drawing, cutter and preservation regressions pass."""
import argparse
import json
import re
from pathlib import Path
import package_cutter as cutter

cutter.base.OUT = cutter.base.ROOT / 'ShiftDrawing'

def validate():
    cutter.validate()
    text = (cutter.base.OUT / 'Validation/NativeShiftDrawing.txt').read_text()
    assert 'PASS 185 Shift drawing checks' in text and 'FAIL' not in text
    text = (cutter.base.OUT / 'Validation/ShiftDrawing.txt').read_text()
    match = re.search(r'Expectations: (\d+) / (\d+) passed', text)
    assert match and match[1] == match[2] and int(match[1]) >= 31 and '🔴' not in text

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--windows', type=Path, required=True)
    parser.add_argument('--linux', type=Path, required=True)
    args = parser.parse_args()
    validate()
    manifest = {}
    for platform, folder in [('Windows', args.windows), ('Linux', args.linux)]:
        executable = folder / ('SolidArcWorkspace.exe' if platform == 'Windows' else 'SolidArcWorkspace')
        binary = executable.read_bytes()
        assert b'Shift straight' in binary and b'Surface cutters' in binary, 'Stale executable'
        metadata = cutter.base.package(folder, platform)
        old = cutter.base.OUT / 'Packages' / metadata['package']
        target = old.with_name(f'Project Liger Shift Drawing {platform}.zip')
        old.replace(target)
        metadata['package'] = target.name
        assert cutter.base.sha(target.read_bytes()) == metadata['package_sha256']
        manifest[platform] = metadata
    (cutter.base.OUT / 'Validation/RuntimeManifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
