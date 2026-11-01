"""Package rebuilt native tracing apps with the protected dense Liger reference."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import zipfile
from package_blueprints import runtime
from package_clean_project import LAUNCHER
from setup_tracing_project import ROOT
OUT=ROOT/'Tracing'

def package(folder,platform):
    folder=Path(folder)
    files,metadata=runtime(folder,platform)
    manifest=json.loads((OUT/'Validation/BuildManifest.json').read_text())
    assert metadata['sha256']==manifest['runtimes'][platform]['sha256'], 'Runtime does not match newly validated tracing build'
    native=(OUT/'Validation/NativeTracing.txt').read_text()
    match=re.search(r'PASS (\d+) surface tracing checks',native)
    assert match and int(match[1])>=92 and 'FAIL' not in native
    editing=(OUT/'Validation/NativeEditing.txt').read_text()
    match=re.search(r'PASS (\d+) curve editing checks',editing)
    assert match and int(match[1])>=50 and 'FAIL' not in editing
    for check in ('Startup','Drawing','Edge','Quad','Control','Editing','TopologyDelete'):
        report=(OUT/f'Validation/{check}.txt').read_text()
        assert '🔴' not in report and 'Expectations:' in report and 'failed' not in report.lower()
    binary=(folder/metadata['executable']).read_bytes()
    assert b'Surface snap###SurfaceTrace' in binary and b'Surface trace supports Curve' in binary
    assert b'only clicked controls are saved' in binary
    # GCC may synthesise std::string command labels from vector constants.
    # This ImGui const-char label stays contiguous in both ELF and PE builds.
    assert b'Subdivide (Ctrl+Shift+D)###CurveSubdivide' in binary
    payload={k:v for k,v in files.items() if k==f"Runtime/{metadata['executable']}" or k.startswith(('Runtime/EngineContent/','Runtime/Notices/'))}
    payload['Liger.solidarc']=(OUT/'Workspace/Liger.solidarc').read_bytes()
    payload['Runtime/SolidArcWorkspace.toml']=(OUT/'Workspace/SolidArcWorkspace.toml').read_bytes()
    payload['Read me.md']=(OUT/'README.md').read_bytes()
    if platform=='Windows':
        payload['Start Liger.bat']=LAUNCHER.replace('\n','\r\n').encode('ascii')
    else:
        payload['Start Liger.sh']=b'''#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$ROOT/Runtime"
exec "$ROOT/Runtime/SolidArcWorkspace" --options "$ROOT/Runtime/SolidArcWorkspace.toml" --open "$ROOT/Liger.solidarc" "$@"
'''
    target=OUT/'Packages'/f'Project Liger Surface Tracing {platform}.zip'
    target.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
        for name,data in sorted(payload.items()):
            entry=zipfile.ZipInfo('Project Liger/'+name)
            entry.compress_type=zipfile.ZIP_DEFLATED
            mode=0o100755 if name.endswith('.sh') or name=='Runtime/SolidArcWorkspace' else 0o100644
            entry.external_attr=mode<<16
            archive.writestr(entry,data,compresslevel=9)
    with zipfile.ZipFile(target) as archive:
        assert archive.testzip() is None
        for name,data in payload.items(): assert archive.read('Project Liger/'+name)==data
        assert {n.removeprefix('Project Liger/').split('/')[0] for n in archive.namelist()}=={'Liger.solidarc','Runtime','Read me.md',f'Start Liger.{"bat" if platform=="Windows" else "sh"}'}
    print(f'PASS {platform}: current tracing executable, protected project, one launcher, {metadata["svg_icons"]} SVGs, {metadata["font_faces"]} fonts; all CRCs / bytes match')
    print(f'{hashlib.sha256(target.read_bytes()).hexdigest()}  {target.name}')
    return target

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--windows',type=Path,required=True)
    parser.add_argument('--linux',type=Path,required=True)
    args=parser.parse_args()
    package(args.windows,'Windows'); package(args.linux,'Linux')
