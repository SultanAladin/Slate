"""Package the Liger reference project and explicitly supplied rebuilt runtimes.
Archives contain their own payload hashes; the review has a separate SHA256SUMS.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
import struct
import subprocess
import zipfile
import sys
from importlib.metadata import version

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'Blueprints'

def digest(data):return hashlib.sha256(data).hexdigest()


def archive(path, items):
    items=dict(items)
    items['SHA256SUMS']=''.join(f'{digest(data)}  {name}\n' for name,data in sorted(items.items())).encode()
    with zipfile.ZipFile(path,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for name,data in sorted(items.items()):
            info=zipfile.ZipInfo('Liger-Reference/'+name)
            info.compress_type=zipfile.ZIP_DEFLATED
            info.external_attr=(0o100755 if name.endswith('.sh') or name=='Runtime/SolidArcWorkspace' else 0o100644)<<16
            z.writestr(info,data,compresslevel=9)
    with zipfile.ZipFile(path) as z:
        assert z.testzip() is None
        for line in z.read('Liger-Reference/SHA256SUMS').decode().splitlines():
            sha,name=line.split('  ',1)
            assert digest(z.read('Liger-Reference/'+name))==sha
    return dict(bytes=path.stat().st_size,sha256=digest(path.read_bytes()),payload_files=len(items)-1)


def runtime(folder, platform):
    exe=folder/('SolidArcWorkspace.exe' if platform=='Windows' else 'SolidArcWorkspace')
    data=exe.read_bytes()
    if platform=='Windows':
        pe=struct.unpack_from('<I',data,0x3c)[0]
        assert data[:2]==b'MZ' and data[pe:pe+4]==b'PE\0\0'
        assert struct.unpack_from('<H',data,pe+4)[0]==0x8664
        dependencies=subprocess.check_output(['objdump','-p',str(exe)],text=True)
        dependencies=[s.split('DLL Name:',1)[1].strip() for s in dependencies.splitlines() if 'DLL Name:' in s]
    else:
        assert data[:5]==b'\x7fELF\x02' and struct.unpack_from('<H',data,18)[0]==62
        dependencies=subprocess.check_output(['ldd',str(exe)],text=True)
        assert 'not found' not in dependencies
        dependencies=[re.sub(r'\s*\(0x[0-9a-f]+\)', '', line).strip() for line in dependencies.splitlines()]
    svgs=list((folder/'EngineContent/Icons').rglob('*.svg'))
    solidarc=list((folder/'EngineContent/Icons/SolidArc').glob('*.svg'))
    fonts=list((folder/'EngineContent/FontArchives').rglob('*.ttf'))
    assert len(svgs)>=310 and len(solidarc)>=66 and len(fonts)>=5
    assert all(p.stat().st_size>0 for p in svgs)
    files={'Runtime/'+p.relative_to(folder).as_posix():p.read_bytes() for p in folder.rglob('*') if p.is_file()}
    return files,dict(executable=exe.name,sha256=digest(data),executable_bytes=len(data),
                      svg_icons=len(svgs),solidarc_icons=len(solidarc),font_faces=len(fonts),dependencies=dependencies)


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--windows',type=Path,required=True,help='Installed rebuilt Windows runtime directory')
    ap.add_argument('--linux',type=Path,required=True,help='Installed rebuilt Linux/X11 runtime directory')
    args=ap.parse_args()
    # Refuse to label incomplete/failed native checks as a released package.
    assert '123 passed; 0 failed' in (OUT/'Validation/NativeProjectTests.txt').read_text()
    regression=(OUT/'Validation/DocumentRegression.txt').read_text()
    assert regression.count('🟢')>=869 and '🔴' not in regression
    for name,count in [('QuadViewport',6),('TopViewport',7)]:
        assert f'Expectations: {count} / {count} passed' in (OUT/f'Validation/{name}.txt').read_text()
    for log in ['InteractionTests.txt','Interaction1800Tests.txt']:
        assert 'Expectations: 35 / 35 passed' in (OUT/'Validation'/log).read_text()
    for view in ['Top','Bottom','Left','Right','Front','Rear']:
        assert 'Expectations: 5 / 5 passed' in (OUT/'Validation'/f'NamedViews-{view}.txt').read_text()
    python_log=(OUT/'Validation/PythonTests.txt').read_text()
    assert 'Ran 34 tests' in python_log and '\nOK\n' in python_log and 'skipped' not in python_log
    assert 'JavaScript errors / failed HTTP responses: 0' in (OUT/'Validation/BrowserTests.txt').read_text()
    manifest=dict(python_environment=dict(python=sys.version.split()[0],packages={p:version(p) for p in ['numpy','scipy','pillow','numba','triangle']}),delivery='native-view-and-drawing-plane-correction-v2',reference_axes='nose -Y / native right +X / up +Z',vehicle_source='ea9cb133220aa171b6002703ff7bb88ced5673f2',
        source_base='unassignedinbox/Frontier 82ed251267ed8457ca3530d8228b91555572c25d',
        desktop_host='SultanAladin/Frontier 65cf0f1',
        compilers=dict(Windows='Zig 0.14.1 / x86_64-windows-gnu / static C++ runtime',Linux='GCC 12.2 / Debian 12 / GLFW X11'),
        reference_filter='Alpha-aware STB mip chain with trilinear minification; base resolution unchanged',
        tests=dict(python=34,native_reference_assertions=123,document_regressions=regression.count('🟢'),vulkan_assertions=113,
                   reference_minification=json.loads((OUT/'Validation/MinificationCheck.json').read_text())),
        windows_interactive_tested=False,patches={},runtimes={})
    patches=[ROOT.parents[1]/'Patches/SolidArc-Booleans-Variables-Modifiers.patch',
             ROOT/'Tools/Patches/NurbsIsoBoundaryImport.patch',ROOT/'Tools/Patches/NurbsBoundaryPrecisionProjection.patch',
             ROOT.parents[1]/'Patches/SolidArc-Reference-Startup-Views.patch',
             ROOT.parents[1]/'Patches/SolidArc-Reference-Interaction-Corrections.patch']
    for p in patches:manifest['patches'][p.name]=digest(p.read_bytes())
    manifest['interface_source_sha256']=digest((ROOT.parents[1]/'Source/UI/SolidArc/Interface/SolidArcInterface.cpp').read_bytes())
    manifest['script_source_sha256']=digest((ROOT.parents[1]/'Source/UI/SolidArc/Interface/SolidArcScript.cpp').read_bytes())
    runtimes={}
    for platform,folder in [('Windows',args.windows),('Linux',args.linux)]:
        runtimes[platform],manifest['runtimes'][platform]=runtime(folder,platform)
    (OUT/'Validation/BuildManifest.json').write_text(json.dumps(manifest,indent=2))
    items={p.relative_to(OUT).as_posix():p.read_bytes() for p in OUT.rglob('*') if p.is_file()
           and p.name!='SHA256SUMS' and not {'Packages','Runtime','__pycache__'}.intersection(p.relative_to(OUT).parts)
           and ('Recovery' not in p.parts or p.name=='.gitkeep')}
    items['REPOSITORY.txt']=b'https://github.com/SultanAladin/Slate/tree/arena/4f335fa8-slate/SolidArc/Vehicles/Liger/Blueprints\nFull reproducible tooling is in the parent Tools folder of the repository.\n'
    packages=OUT/'Packages';packages.mkdir(exist_ok=True)
    result={'Liger-Reference-Project.zip':archive(packages/'Liger-Reference-Project.zip',items)}
    for platform in ['Windows','Linux']:
        name=f'SolidArc-Liger-{platform}-x64.zip'
        result[name]=archive(packages/name,dict(items,**runtimes[platform]))
    (packages/'manifest.json').write_text(json.dumps(result,indent=2))
    files=[p for p in OUT.rglob('*') if p.is_file() and p.name!='SHA256SUMS'
           and not {'Runtime','__pycache__'}.intersection(p.relative_to(OUT).parts)
           and ('Recovery' not in p.parts or p.name=='.gitkeep')]
    (OUT/'SHA256SUMS').write_text(''.join(f'{digest(p.read_bytes())}  {p.relative_to(OUT).as_posix()}\n' for p in sorted(files)))
    print(json.dumps(result,indent=2))
    print('Archive CRCs, all internal payload hashes, executable formats and full SVG/font sets verified.')


if __name__=='__main__':main()
