"""Reference-sheet scale, orientation, project and occlusion regressions."""
import json
from pathlib import Path
import tomllib
import unittest
import numpy as np
from PIL import Image
from fit_macros import ROOT
from build_blueprints import visible_lines


class BlueprintTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root=ROOT/'Blueprints'
        cls.meta=json.loads((cls.root/'registration.json').read_text())
        cls.project=tomllib.loads((cls.root/'Liger.solidarc').read_text())

    def test_six_views_and_empty_modelling_tree(self):
        self.assertEqual({r['name'] for r in self.meta['views']},{'Top','Bottom','Left','Right','Front','Rear'})
        self.assertFalse(self.project.get('Steps'))
        self.assertEqual(len(self.project['ImagePlanes']),6)
        for p in self.project['ImagePlanes']:
            self.assertTrue(p['Visible'] and p['Locked'] and p['FrontOnly'])
            self.assertTrue((self.root/p['Path']).is_file())

    def test_no_perspective_or_mirrored_camera_frames(self):
        for v in self.meta['views']:
            M=np.array([v['screen_right'],v['screen_up'],v['eye']])
            np.testing.assert_array_equal(M@M.T,np.eye(3))
            self.assertEqual(np.linalg.det(M),1)
        v={r['name']:r for r in self.meta['views']}
        self.assertEqual(v['Left']['screen_right'][0],-1)
        self.assertEqual(v['Right']['screen_right'][0],1)
        self.assertEqual(v['Front']['eye'],[1,0,0])

    def test_registered_pixel_to_world_scale(self):
        for v,p in zip(self.meta['views'],self.project['ImagePlanes']):
            W,H=v['pixels'];self.assertEqual(v['pixels_per_metre'],1500)
            self.assertAlmostEqual(np.linalg.norm(p['Right'])/W,1/1500,places=14)
            self.assertAlmostEqual(np.linalg.norm(p['Down'])/H,1/1500,places=14)
            E=np.array(v['eye']); R=np.array(v['screen_right']); U=np.array(v['screen_up'])
            for s,t in [(0,0),(.17,.81),(1,1)]:
                q=np.array(p['Corner'])+s*np.array(p['Right'])+t*np.array(p['Down'])
                self.assertAlmostEqual(q@E,v['plane_depth'],places=12)
                self.assertAlmostEqual((q@R-np.array(p['Corner'])@R)*1500,s*W,places=9)
                self.assertAlmostEqual((np.array(p['Corner'])@U-q@U)*1500,t*H,places=9)
            self.assertLess(v['plane_depth'],v['near_depth'])

    def test_high_resolution_transparent_registered_images(self):
        for v in self.meta['views']:
            with Image.open(self.root/v['image']) as image:
                self.assertEqual(image.size,tuple(v['pixels']))
                self.assertEqual(image.mode,'RGBA')
                alpha=np.array(image.getchannel('A'))
                self.assertGreater(np.count_nonzero(alpha),1000)
                self.assertLess(np.count_nonzero(alpha),alpha.size//8)
            with Image.open(self.root/'Schematics'/f"Liger_{v['name']}.png") as sheet:
                self.assertEqual(sheet.size,(v['pixels'][0]+160,v['pixels'][1]+350))

    def test_launchers_keep_all_views_in_the_same_project(self):
        options=tomllib.loads((self.root/'SolidArcWorkspace.toml').read_text())
        self.assertEqual(options['Viewport']['InitialLayout'],'Quad')
        for v in self.meta['views']:
            name=v['name'];opts=tomllib.loads((self.root/'Views'/f'{name}.toml').read_text())
            self.assertEqual(opts['Viewport']['InitialView'],v['native_preset'])
            self.assertEqual(opts['Viewport']['InitialLayout'],'Single')
            self.assertIn('--open "Liger.solidarc"',(self.root/'Views'/f'Start_{name}.bat').read_text())
            self.assertIn('/Liger.solidarc',(self.root/'Views'/f'Start_{name}.sh').read_text())
        self.assertFalse(list((self.root/'Views').glob('*.solidarc')))

    def test_buried_curve_is_not_shown_through_skin(self):
        depth=np.zeros((12,12),np.float32);segments=np.array([[0,1]],np.int32);kinds=np.array([2],np.uint8)
        for z,expected in [(0,True),(-.0005,False),(-.01,False)]:
            p=np.array([[2.,3.,z],[9.,3.,z]])
            self.assertEqual(bool(np.any(visible_lines(p,segments,kinds,depth))),expected)

    def test_visibility_uses_interpolated_depth_on_sloped_skin(self):
        y,x=np.mgrid[:12,:12];depth=(x*.003+y*.002).astype(np.float32)
        points=np.array([[2.25,3.5,2.25*.003+3.5*.002],[8.25,3.5,8.25*.003+3.5*.002]])
        lines=visible_lines(points,np.array([[0,1]],np.int32),np.array([2],np.uint8),depth)
        self.assertGreater(np.count_nonzero(lines),4)

    def test_project_does_not_embed_old_cad(self):
        self.assertNotIn('Exchange',self.project)
        self.assertNotIn('ISO-10303-21',(self.root/'Liger.solidarc').read_text())
        self.assertEqual(self.meta['source_commit'],'ea9cb133220aa171b6002703ff7bb88ced5673f2')


if __name__=='__main__':unittest.main(verbosity=2)
