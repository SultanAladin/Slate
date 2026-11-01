"""Package the refined CAD/static evidence; verify archive CRC and SHA256 layers."""
from pathlib import Path
import hashlib
import zipfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'Refined'
README = '''LIGER / TARGETED NURBS REFINEMENT / 01 NOV 2026

Open CAD/Liger_Refined_CAD.step in a STEP surface-model capable CAD application,
or CAD/Liger_Refined_CAD.solidarc in a compatible rebuilt SolidArc.
306 faces in three OPEN SHEET parts, not a closed manufacturing solid / Class-A.

Changes: three strip interfaces per half removed, two arch patches faired,
whole roof/glass-frame insert translated DOWN 1 mm to clear protrusion.
Main-shell outer rails retained. Some tight trailing arch returns remain.
REPORT.md distinguishes completed improvements from remaining limitations.

No new native GUI binary or interactive viewer is included in this ZIP.
Old W95 application executables were NOT rebuilt. Native audit uses Frontier
82ed251267ed8457ca3530d8228b91555572c25d plus the W95 overlay and BOTH
NURBS importer patches shipped in Tools/Patches/. Rebuild for that native path.
The deliberate real-gap fixture is test input, NOT production CAD.

The interactive comparison viewer requires the complete repository's original,
Faired and Refined folders. Clone/download the repository and run
python Tools/serve_refined.py 8080 from SolidArc/Vehicles/Liger.

Full report, viewer and reproducible tools:
https://github.com/SultanAladin/Slate/tree/arena/4f335fa8-slate/SolidArc/Vehicles/Liger/Refined
'''


def main():
    report = (OUT/'README.md').read_text().replace('../Tools/Patches/', 'Tools/Patches/').replace('../../../Patches/', 'Tools/Patches/')
    items = {'README.txt': README.encode(), 'REPORT.md': report.encode(),
             'refinement_report.json': (OUT/'refinement_report.json').read_bytes()}
    for p in (OUT/'CAD').iterdir():
        if p.is_file():
            items[p.relative_to(OUT).as_posix()] = p.read_bytes()
    for p in (OUT/'Proofs').iterdir():
        if p.suffix in ('.png', '.txt', '.json'):
            items[p.relative_to(OUT).as_posix()] = p.read_bytes()
    for name in ['NurbsIsoBoundaryImport.patch', 'NurbsBoundaryPrecisionProjection.patch']:
        items['Tools/Patches/'+name] = (ROOT/'Tools/Patches'/name).read_bytes()
    items['Tools/Patches/SolidArc-Booleans-Variables-Modifiers.patch'] = (ROOT.parents[1]/'Patches/SolidArc-Booleans-Variables-Modifiers.patch').read_bytes()
    items['SHA256SUMS'] = ''.join(f'{hashlib.sha256(data).hexdigest()}  {name}\n' for name, data in sorted(items.items())).encode()
    archive = OUT/'Liger-Refined-CAD-and-Proofs.zip'
    with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, data in sorted(items.items()):
            z.writestr('Liger-Refined/'+name, data)
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        for line in z.read('Liger-Refined/SHA256SUMS').decode().splitlines():
            sha, name = line.split('  ', 1)
            assert hashlib.sha256(z.read('Liger-Refined/'+name)).hexdigest() == sha
    files = [p for p in OUT.rglob('*') if p.is_file() and p.name != 'SHA256SUMS'
             and not {'node_modules', '__pycache__'}.intersection(p.parts)]
    (OUT/'SHA256SUMS').write_text(''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(OUT).as_posix()}\n' for p in sorted(files)))
    for line in (OUT/'SHA256SUMS').read_text().splitlines():
        sha, name = line.split('  ', 1)
        assert hashlib.sha256((OUT/name).read_bytes()).hexdigest() == sha
    print(f'ZIP {archive.stat().st_size:,} bytes; {len(items)-1} payload files; {len(files)} review files')
    print('Archive CRC, internal SHA256 and external review SHA256 checks passed')


if __name__ == '__main__':
    main()
