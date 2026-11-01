"""Package only Liger.solidarc and its six images in one flat Project Liger folder."""
from pathlib import Path
import hashlib
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


if __name__ == '__main__':
    build_clean_project()
