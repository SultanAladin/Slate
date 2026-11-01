"""Package faired CAD + static proofs and verify both archive checksum layers."""
from pathlib import Path
import zipfile, hashlib
root=Path(__file__).resolve().parents[1]/'Faired'
readme='''LIGER / FAIRED NURBS CAD + STATIC PROOFS / 01 NOV 2026

Open CAD/Liger_Macro_CAD.solidarc in a compatible rebuilt SolidArc, or
CAD/Liger_Macro_CAD.step in a STEP surface-model capable CAD application.
Three exterior sheet parts, not a closed manufacturing solid or Class-A model.
56 mirrored faces faired; 312 total. Original boundary curves preserved.

IMPORTANT: old W95 GUI binaries are NOT rebuilt in this package. They may
refuse the main shell. The included NURBS iso-boundary importer patch must
be applied to the W95-overlay source and the application rebuilt, as explained
in REPORT.md and the linked original review. No native GUI executable included.

Proofs/ contains computed before/after shaded and zebra PNGs, actual shape-change
map, source distance map, and validation logs. REPORT.md has the full measured
results and remaining limitations. Source deviation is NOT a smoothness score.

The interactive comparison viewer is NOT in this CAD-only ZIP. To use it,
clone/download the repository, preserving both the original and Faired folders,
then run python Tools/serve_faired.py 8080 from SolidArc/Vehicles/Liger.
Repository revision/report and all reproducible tools:
https://github.com/SultanAladin/Slate/tree/arena/4f335fa8-slate/SolidArc/Vehicles/Liger/Faired
Original review and native rebuild requirements:
https://github.com/SultanAladin/Slate/blob/2473d49/SolidArc/Vehicles/Liger/README.md
'''
items={'README.txt':readme.encode(),'REPORT.md':(root/'README.md').read_bytes(),
       'fairing_report.json':(root/'fairing_report.json').read_bytes(),
       'Tools/Patches/NurbsIsoBoundaryImport.patch':(root.parent/'Tools/Patches/NurbsIsoBoundaryImport.patch').read_bytes()}
for p in (root/'CAD').iterdir():items[p.relative_to(root).as_posix()]=p.read_bytes()
for p in (root/'Proofs').iterdir():
 if p.suffix in ('.png','.txt') or p.name in ('reverse_deviation.json','DenseNormalScreen.json'):
  items[p.relative_to(root).as_posix()]=p.read_bytes()
items['SHA256SUMS']=''.join(f'{hashlib.sha256(data).hexdigest()}  {name}\n' for name,data in sorted(items.items())).encode()
with zipfile.ZipFile(root/'Liger-Faired-CAD-and-Proofs.zip','w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 for name,data in sorted(items.items()):z.writestr('Liger-Faired/'+name,data)
files=[p for p in root.rglob('*') if p.is_file() and p.name!='SHA256SUMS' and 'node_modules' not in p.parts and '__pycache__' not in p.parts]
(root/'SHA256SUMS').write_text(''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(root).as_posix()}\n' for p in sorted(files)))
print('ZIP', (root/'Liger-Faired-CAD-and-Proofs.zip').stat().st_size, 'bytes;',len(files),'checked files')
with zipfile.ZipFile(root/'Liger-Faired-CAD-and-Proofs.zip') as z:
 assert z.testzip() is None
 sums=z.read('Liger-Faired/SHA256SUMS').decode().splitlines()
 for line in sums:
  sha,name=line.split('  ',1);assert hashlib.sha256(z.read('Liger-Faired/'+name)).hexdigest()==sha
 print('Archive CRC + all internal SHA256 checks passed')
