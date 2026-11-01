"""Create clean Project Liger downloads, optionally with the Windows app and one launcher."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import tomllib
import zipfile

ROOT = Path(__file__).resolve().parents[1]
BLUEPRINTS = ROOT / 'Blueprints'
OUTPUT = BLUEPRINTS / 'Packages' / 'Project Liger.zip'


def build_clean_project(source=BLUEPRINTS, output=OUTPUT):
    original = (source / 'Liger.solidarc').read_text(encoding='utf-8')
    project = re.sub(r'(?m)^Path = "References/([^"/]+\.png)"$', r'Path = "\1"', original)
    document = tomllib.loads(project)
    assert not document.get('Steps'), 'Keep the scratch-model project empty'
    planes = document['ImagePlanes']
    assert len(planes) == 6
    payload = {'Liger.solidarc': project.encode('utf-8')}
    for plane in planes:
        name = plane['Path']
        assert name == Path(name).name and name.endswith('.png')
        payload[name] = (source / 'References' / name).read_bytes()
    assert set(payload) == {'Liger.solidarc', 'Top.png', 'Bottom.png', 'Left.png',
                            'Right.png', 'Front.png', 'Rear.png'}
    # All calibration and image flags remain identical; only relative paths change.
    previous = tomllib.loads(original)
    for before, after in zip(previous['ImagePlanes'], planes):
        assert {k: v for k, v in before.items() if k != 'Path'} == {
            k: v for k, v in after.items() if k != 'Path'}
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted(payload.items()):
            entry = zipfile.ZipInfo('Project Liger/' + name)
            entry.compress_type = zipfile.ZIP_DEFLATED
            entry.external_attr = 0o100644 << 16
            archive.writestr(entry, data, compresslevel=9)
    with zipfile.ZipFile(output) as archive:
        assert archive.testzip() is None
        assert len(archive.namelist()) == 7
        for name, data in payload.items():
            assert archive.read('Project Liger/' + name) == data
    print(f'PASS: one Project Liger folder, exactly 7 files; all six images unchanged')
    print(f'PASS: relative paths, calibration, empty feature tree and ZIP CRCs verified')
    print(f'{hashlib.sha256(output.read_bytes()).hexdigest()}  {output.name}')
    return output


LAUNCHER = r'''@echo off
setlocal EnableExtensions DisableDelayedExpansion
if not exist "%~dp0Liger.solidarc" goto MissingProject
if not exist "%~dp0Runtime\SolidArcWorkspace.exe" goto MissingRuntime
pushd "%~dp0Runtime"
if errorlevel 1 goto MissingRuntime
"%~dp0Runtime\SolidArcWorkspace.exe" --options "%~dp0Runtime\SolidArcWorkspace.toml" --open "%~dp0Liger.solidarc"
set "Result=%ERRORLEVEL%"
popd
if not "%Result%"=="0" (
  echo SolidArc exited with error %Result%. Check your Vulkan graphics driver.
  pause
)
exit /b %Result%
:MissingProject
echo Liger.solidarc is missing. Extract the entire ZIP before starting.
pause
exit /b 1
:MissingRuntime
echo The Runtime folder is missing or inaccessible. Extract the entire ZIP before starting.
pause
exit /b 1
'''


def build_windows_bundle(folder, source=BLUEPRINTS, output=None):
    from package_blueprints import runtime
    folder = Path(folder)
    # Use the tested corrected executable, not one of the historical app packages.
    files, metadata = runtime(folder, 'Windows')
    expected = json.loads((source / 'Validation/BuildManifest.json').read_text())
    assert metadata['sha256'] == expected['runtimes']['Windows']['sha256']
    clean = build_clean_project(source)
    with zipfile.ZipFile(clean) as archive:
        payload = {n.removeprefix('Project Liger/'): archive.read(n) for n in archive.namelist()}
    # All app support data stays under Runtime, not alongside the seven project files.
    payload.update({name: data for name, data in files.items()
                    if name == 'Runtime/SolidArcWorkspace.exe'
                    or name.startswith(('Runtime/EngineContent/', 'Runtime/Notices/'))})
    options = (source / 'SolidArcWorkspace.toml').read_text().replace(
        'Width = 1800', 'Width = 1280').replace('Height = 1100', 'Height = 680').replace(
        'DocumentPath = "Liger.solidarc"', 'DocumentPath = "../Liger.solidarc"')
    payload['Runtime/SolidArcWorkspace.toml'] = options.encode('utf-8')
    payload['Start Liger.bat'] = LAUNCHER.replace('\n', '\r\n').encode('ascii')
    output = output or source / 'Packages/Project Liger Windows.zip'
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted(payload.items()):
            entry = zipfile.ZipInfo('Project Liger/' + name)
            entry.compress_type = zipfile.ZIP_DEFLATED
            entry.external_attr = 0o100644 << 16
            archive.writestr(entry, data, compresslevel=9)
    with zipfile.ZipFile(output) as archive:
        assert archive.testzip() is None
        for name, data in payload.items():
            assert archive.read('Project Liger/' + name) == data
        top = {n.removeprefix('Project Liger/').split('/')[0] for n in archive.namelist()}
        assert top == {'Liger.solidarc', 'Top.png', 'Bottom.png', 'Left.png', 'Right.png',
                       'Front.png', 'Rear.png', 'Runtime', 'Start Liger.bat'}
        assert len([n for n in archive.namelist() if n.endswith('.bat')]) == 1
    print('PASS: project + six images + one batch file + Runtime; no samples or reports')
    print(f"PASS: Windows x64 executable, {metadata['svg_icons']} SVG icons, {metadata['font_faces']} fonts, all payload bytes and CRCs")
    print(f'{hashlib.sha256(output.read_bytes()).hexdigest()}  {output.name}')
    return output


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--windows', type=Path, help='Include this installed Windows runtime and one batch launcher')
    args = parser.parse_args()
    if args.windows:
        build_windows_bundle(args.windows)
    else:
        build_clean_project()
