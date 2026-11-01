"""Deterministic workspace / rigid registration / launcher regressions (no CAD stubs)."""
import re
import tempfile
import tomllib
import unittest
from pathlib import Path
from setup_tracing_project import ROOT, setup
from package_clean_project import LAUNCHER

class TracingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source=tomllib.loads((ROOT/'Refined/CAD/Liger_Refined_CAD.solidarc').read_text())
        cls.project=tomllib.loads((ROOT/'Tracing/Workspace/Liger.solidarc').read_text())
        cls.options=tomllib.loads((ROOT/'Tracing/Workspace/SolidArcWorkspace.toml').read_text())

    def test_reference_only_no_authored_curves(self):
        self.assertEqual(len(self.project['Steps']),3)
        self.assertTrue(all(s['Kind']==18 for s in self.project['Steps']))
        self.assertEqual([s['Label'] for s in self.project['Steps']],[s['Label'] for s in self.source['Steps']])

    def test_reference_protected_not_hidden(self):
        self.assertEqual(set(self.project['Appearance']['Locked']),{s['Label'] for s in self.source['Steps']})
        self.assertFalse(self.project['Appearance'].get('Hidden'))

    def test_rigid_rotation_every_point_and_direction(self):
        expression=re.compile(r"(?:CARTESIAN_POINT|DIRECTION)\('[^']*',\(([^,()]+),([^,()]+),([^,()]+)\)\)")
        total=0
        for old,new in zip(self.source['Steps'],self.project['Steps']):
            self.assertEqual(old['ExchangeUnit'],new['ExchangeUnit'])
            before=expression.findall(old['Exchange']);after=expression.findall(new['Exchange'])
            self.assertEqual(len(before),len(after))
            # Topology, knots, weights, names, units and patch connectivity unchanged.
            self.assertEqual(expression.sub('RIGID_ENTITY',old['Exchange']),expression.sub('RIGID_ENTITY',new['Exchange']))
            for a,b in zip(before,after):
                x,y,z=map(float,a); q=tuple(map(float,b))
                self.assertEqual(q,(y,-x,z))
            total+=len(before)
        self.assertGreater(total,47000)

    def test_surface_xyz_and_single_view_are_default(self):
        self.assertEqual(self.options['Tracing'],{'Surface':True,'FullXYZ':True,'SourceEdges':False})
        self.assertEqual(self.options['Viewport']['InitialLayout'],'Single')
        self.assertEqual(self.options['Files']['DocumentPath'],'../Liger.solidarc')

    def test_build_is_deterministic(self):
        with tempfile.TemporaryDirectory() as directory:
            setup(directory)
            for name in ['Liger.solidarc','SolidArcWorkspace.toml']:
                self.assertEqual((Path(directory)/name).read_bytes(),(ROOT/'Tracing/Workspace'/name).read_bytes())

    def test_windows_launcher_explicit_open_and_quoted_paths(self):
        self.assertIn('DisableDelayedExpansion',LAUNCHER)
        self.assertIn('--open "%~dp0Liger.solidarc"',LAUNCHER)
        self.assertIn('--options "%~dp0Runtime\\SolidArcWorkspace.toml"',LAUNCHER)
        self.assertIn('pushd "%~dp0Runtime"',LAUNCHER)
        self.assertEqual(LAUNCHER.count('SolidArcWorkspace.exe" --options'),1)

if __name__=='__main__':unittest.main()
