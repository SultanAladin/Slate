"""Dedicated, protected dense-NURBS reference workspace. No authored vehicle curves."""
from pathlib import Path
import re
import tomllib
ROOT = Path(__file__).resolve().parents[1]

def setup(destination=ROOT/'Tracing/Workspace'):
    destination=Path(destination); destination.mkdir(parents=True,exist_ok=True)
    source=(ROOT/'Refined/CAD/Liger_Refined_CAD.solidarc').read_text()
    # Same proper rigid rotation as the registered blueprint workflow: row p @ R
    # = (y,-x,z), nose -Y, right +X, up +Z. Never scale / alter poles independently.
    pattern=re.compile(r"((?:CARTESIAN_POINT|DIRECTION)\('[^']*',\()([^,()]+),([^,()]+),([^,()]+)(\)\))")
    def rotate(m):
        x,y,z=(float(m[i]) for i in (2,3,4))
        return m[1]+f'{y:.17g},{-x:.17g},{z:.17g}'+m[5]
    project,count=pattern.subn(rotate,source)
    assert count>1000
    project=re.sub(r'^Title = .*$', 'Title = "Liger — surface tracing workspace"',project,count=1,flags=re.M)
    project=re.sub(r'^Description = .*$', 'Description = "Protected dense NURBS reference. Draw editable 3D curves on its surface; Ctrl snaps to model edges. Native orientation: nose -Y, right +X, up +Z. Three open sheets, not a repaired solid."',project,count=1,flags=re.M)
    project+='\n[Appearance]\nBodyMatcap = "SatinPlastic"\nLocked = ["LigerPart1", "LigerPart2", "LigerPart3"]\n'
    parsed=tomllib.loads(project)
    assert len(parsed['Steps'])==3 and all(s['Kind']==18 for s in parsed['Steps'])
    (destination/'Liger.solidarc').write_text(project)
    options='''[Window]
Width = 1280
Height = 680
[Viewport]
Grid = false
InitialView = "Isometric"
InitialLayout = "Single"
Orthographic = false
[Tracing]
Surface = true
FullXYZ = true
SourceEdges = false
[Interface]
FontSize = 15.0
[Files]
DocumentPath = "../Liger.solidarc"
RecoveryPath = "Recovery/Liger.solidarc"
'''
    (destination/'SolidArcWorkspace.toml').write_text(options)
    print(f'PASS protected 3-sheet workspace, {count} rigidly rotated point/direction entities; no new vehicle geometry')
    return destination
if __name__=='__main__': setup()
