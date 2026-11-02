"""Release current cutter fix only after selection, cage, tracing and cutter regressions pass."""
import argparse
import json
import re
from pathlib import Path
import package_selection as base

base.OUT = base.ROOT / 'CutterFix'

def validate():
    base.validate()
    text = (base.OUT / 'Validation/NativeCutterPlacement.txt').read_text()
    match = re.search(r'PASS (\d+) surface cutter checks', text)
    assert match and int(match[1]) >= 112 and 'FAIL' not in text
    text = (base.OUT / 'Validation/CutterSurface.txt').read_text()
    match = re.search(r'Expectations: (\d+) / (\d+) passed', text)
    assert match and match[1] == match[2] and '🔴' not in text

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--windows', type=Path, required=True)
    parser.add_argument('--linux', type=Path, required=True)
    args = parser.parse_args()
    validate()
    manifest = {}
    for platform, folder in [('Windows', args.windows), ('Linux', args.linux)]:
        executable = folder / ('SolidArcWorkspace.exe' if platform == 'Windows' else 'SolidArcWorkspace')
        assert b'Surface cutters' in executable.read_bytes(), 'Stale pre-cutter-fix executable'
        metadata = base.package(folder, platform)
        old = base.OUT / 'Packages' / metadata['package']
        target = old.with_name(f'Project Liger Cutter Fix {platform}.zip')
        old.replace(target)
        metadata['package'] = target.name
        assert base.sha(target.read_bytes()) == metadata['package_sha256']
        manifest[platform] = metadata
    (base.OUT / 'Validation/RuntimeManifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
