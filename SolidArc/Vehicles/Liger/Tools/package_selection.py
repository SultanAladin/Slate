"""Package rebuilt global-selection runtimes only after the current regressions pass."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import zipfile
from package_blueprints import runtime
from package_clean_project import LAUNCHER

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'SelectionFix'
SOURCE = ROOT.parents[2] / 'Liger(progress).solidarc'
SOURCE_SHA = '90344c326b32e467b8298851eb8b1df2a2dba1ce879becd3c7a12303b252ec70'
SCRIPTS = ('SelectionSketch', 'SelectionViewport', 'SelectionOutliner', 'SelectionCage',
           'CageWorkflow', 'SurfaceOffsetWorkflow', 'CageProject', 'SurfaceTraceStartup',
           'SurfaceTraceDrawing', 'SurfaceTraceEdge', 'SurfaceTraceQuad', 'SurfaceTraceControl',
           'SurfaceTraceEditing', 'SurfaceTraceTopologyDelete')

def sha(data):
    return hashlib.sha256(data).hexdigest()

def validate():
    v = OUT / 'Validation'
    for name, marker in [('NativeSelection', 'PASS 17 selection and batch deletion checks'),
                         ('NativeCage', 'PASS 49 cage tool checks'),
                         ('NativeEditing', 'PASS 51'), ('NativeTracing', 'PASS 92')]:
        text = (v / f'{name}.txt').read_text()
        assert marker in text and 'FAIL' not in text, name
    for name in SCRIPTS:
        text = (v / f'{name}.txt').read_text()
        match = re.search(r'Expectations: (\d+) / (\d+) passed', text)
        assert match and match[1] == match[2] and '🔴' not in text, name
    document = (v / 'DocumentRegression.txt').read_text()
    assert document.count('🟢') >= 869 and '🔴' not in document
    gizmo = (v / 'GizmoRegression.txt').read_text()
    assert 'Perspective gizmo drag moved' in gizmo and '🔴' not in gizmo
    python = (v / 'PythonTests.txt').read_text()
    assert 'Ran 6 tests' in python and '\nOK\n' in python

def package(folder, platform):
    files, metadata = runtime(Path(folder), platform)
    project = SOURCE.read_bytes()
    assert sha(project) == SOURCE_SHA, 'Never rewrite the supplied cage'
    binary = files[f"Runtime/{metadata['executable']}"]
    for marker in (b'CurveOffsetGuide', b'Live surface distance', b'Auto-split 3D crossings###ToolAutoSplit',
                   b'Delete: select a component or a whole figure first'):
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
    payload['SHA256SUMS'] = ''.join(f'{sha(data)}  {name}\n' for name, data in sorted(payload.items())).encode()
    target = OUT / 'Packages' / f'Project Liger Selection Fix {platform}.zip'
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
    print(f'PASS {platform}: exact supplied cage, rebuilt binary, archive CRCs/payload hashes, {metadata["svg_icons"]} SVGs / {metadata["font_faces"]} fonts')
    return metadata

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--windows', type=Path, required=True)
    parser.add_argument('--linux', type=Path, required=True)
    args = parser.parse_args()
    validate()
    manifest = {p: package(f, p) for p, f in [('Windows', args.windows), ('Linux', args.linux)]}
    (OUT / 'Validation/RuntimeManifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
