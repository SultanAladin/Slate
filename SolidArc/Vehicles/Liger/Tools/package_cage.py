"""Package the tested native cage tools and the byte-identical supplied Liger cage."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import zipfile
from package_blueprints import runtime
from package_clean_project import LAUNCHER

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'CageTools'
SOURCE = ROOT.parents[2] / 'Liger(progress).solidarc'
SOURCE_SHA = '90344c326b32e467b8298851eb8b1df2a2dba1ce879becd3c7a12303b252ec70'

def sha(data):
    return hashlib.sha256(data).hexdigest()

def package(folder, platform):
    files, metadata = runtime(Path(folder), platform)
    project = SOURCE.read_bytes()
    assert sha(project) == SOURCE_SHA, 'Never rewrite the supplied cage'
    checks = (OUT / 'Validation/NativeCage.txt').read_text()
    assert 'PASS 49 cage tool checks' in checks and 'FAIL' not in checks
    for name in ('Cage', 'Offset', 'CageProject', 'Startup', 'Drawing', 'Edge', 'Quad', 'Control', 'Editing', 'TopologyDelete'):
        text = (OUT / f'Validation/{name}.txt').read_text()
        match = re.search(r'Expectations: (\d+) / (\d+) passed', text)
        assert match and match[1] == match[2] and '🔴' not in text, name
    binary = files[f"Runtime/{metadata['executable']}"]
    for marker in (b'CurveOffsetGuide', b'Live surface distance', b'Auto-split 3D crossings###ToolAutoSplit'):
        assert marker in binary, f'Stale executable: {marker!r}'
    payload = {k: v for k, v in files.items() if k == f"Runtime/{metadata['executable']}" or k.startswith(('Runtime/EngineContent/', 'Runtime/Notices/'))}
    payload['Liger.solidarc'] = project
    payload['Runtime/SolidArcWorkspace.toml'] = (ROOT / 'Tracing/Workspace/SolidArcWorkspace.toml').read_bytes()
    payload['Read me.md'] = (OUT / 'README.md').read_bytes()
    if platform == 'Windows':
        payload['Start Liger.bat'] = LAUNCHER.replace('\n', '\r\n').encode('ascii')
    else:
        payload['Start Liger.sh'] = b'''#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$ROOT/Runtime"
exec "$ROOT/Runtime/SolidArcWorkspace" --options "$ROOT/Runtime/SolidArcWorkspace.toml" --open "$ROOT/Liger.solidarc" "$@"
'''
    target = OUT / 'Packages' / f'Project Liger Curve Tools {platform}.zip'
    target.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(target, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(payload.items()):
            entry = zipfile.ZipInfo('Project Liger/' + name)
            entry.compress_type = zipfile.ZIP_DEFLATED
            entry.external_attr = (0o100755 if name.endswith('.sh') or name == 'Runtime/SolidArcWorkspace' else 0o100644) << 16
            archive.writestr(entry, data, compresslevel=9)
    with zipfile.ZipFile(target) as archive:
        assert archive.testzip() is None
        for name, data in payload.items():
            assert archive.read('Project Liger/' + name) == data
        assert all(n.startswith('Project Liger/') for n in archive.namelist())
        assert sha(archive.read('Project Liger/Liger.solidarc')) == SOURCE_SHA
    metadata.update(package=target.name, package_sha256=sha(target.read_bytes()), package_bytes=target.stat().st_size,
                    project_sha256=SOURCE_SHA, payload_files=len(payload))
    print(f'PASS {platform}: exact supplied cage, current native binary, one Project Liger folder, {metadata["svg_icons"]} SVGs, {metadata["font_faces"]} font files; all archive CRCs/bytes match')
    return metadata

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--windows', type=Path, required=True)
    parser.add_argument('--linux', type=Path, required=True)
    args = parser.parse_args()
    manifest = {p: package(f, p) for p, f in [('Windows', args.windows), ('Linux', args.linux)]}
    (OUT / 'Validation/RuntimeManifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
