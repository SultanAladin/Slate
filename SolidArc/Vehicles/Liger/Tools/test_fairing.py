"""Fairing regression + delivered-file invariants: python Tools/test_fairing.py."""
import json
import re
import unittest
import numpy as np
from fit_macros import ROOT, knots
from fair_surfaces import PARTS, read_nets, graph_fair, grid, geometry, dense_normal_screen
from export_cad import Writer


class FairingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = json.loads((ROOT/'Faired/fairing_report.json').read_text())

    def plane(self):
        k = knots(9)
        g = np.array([np.mean(k[i+1:i+4]) for i in range(9)])
        u, v = np.meshgrid(g, g, indexing='ij')
        return dict(P=np.stack([u, v, np.zeros_like(u)], axis=-1), ku=k, kv=k)

    def test_plane_is_invariant(self):
        p = self.plane(); q = graph_fair(p, 2)
        np.testing.assert_allclose(p['P'], q['P'], atol=1e-12)
        xyz, normal, h, _, _ = geometry(q, grid(17, .02))
        self.assertLess(abs(h).max(), 1e-10)
        self.assertGreater(normal[:, 2].min(), .999999)

    def test_interior_noise_changes_without_moving_boundary(self):
        p = self.plane(); p['P'][1:-1, 1:-1, 2] = np.random.default_rng(42).normal(0, .001, (7, 7))
        q = graph_fair(p, 2)
        self.assertFalse(np.array_equal(p['P'], q['P']))
        np.testing.assert_array_equal(p['P'][[0, -1]], q['P'][[0, -1]])
        np.testing.assert_array_equal(p['P'][:, [0, -1]], q['P'][:, [0, -1]])
        np.testing.assert_array_equal(p['ku'], q['ku'])
        np.testing.assert_array_equal(p['kv'], q['kv'])

    def test_restricted_step_reader_round_trip(self):
        w = Writer(); p = self.plane(); w.surface(**p)
        from tempfile import TemporaryDirectory
        from pathlib import Path
        with TemporaryDirectory() as d:
            path = Path(d)/'surface.step'; path.write_text('\n'.join(w.lines))
            q = read_nets(path, half=False)[0]
            np.testing.assert_allclose(q['P'], p['P'], atol=1e-15)
            path.write_text(path.read_text().replace("('',3,3,", "('',2,3,"))
            with self.assertRaises(ValueError):
                read_nets(path, half=False)

    def test_brep_and_curve_entities_are_unchanged(self):
        for name in PARTS + ['Liger_Macro_CAD']:
            before = (ROOT/'CAD'/f'{name}.step').read_text()
            after = (ROOT/'Faired/CAD'/f'{name}.step').read_text()
            def without_points(s):
                return re.sub(r"#\d+=CARTESIAN_POINT\('',\([^()]*\)\);", '', s)
            self.assertEqual(without_points(before), without_points(after))
            entities = dict(re.findall(r'#(\d+)=(.*?);', before))
            new_entities = dict(re.findall(r'#(\d+)=(.*?);', after))
            for body in entities.values():
                if body.startswith('B_SPLINE_CURVE_WITH_KNOTS'):
                    for number in re.findall(r'#(\d+)', body):
                        self.assertEqual(entities[number], new_entities[number])

    def test_every_delivered_rim_knot_and_unchanged_surface(self):
        changed = {(r['part'], r['half_patch']) for r in self.report['changed']}
        count = 0
        for name in PARTS:
            before = read_nets(ROOT/'CAD'/f'{name}.step')
            after = read_nets(ROOT/'Faired/CAD'/f'{name}.step')
            self.assertEqual(len(before), len(after))
            for i, (p, q) in enumerate(zip(before, after)):
                for k in ['ku', 'kv']:
                    np.testing.assert_array_equal(p[k], q[k])
                np.testing.assert_array_equal(p['P'][[0, -1]], q['P'][[0, -1]])
                np.testing.assert_array_equal(p['P'][:, [0, -1]], q['P'][:, [0, -1]])
                if (name, i) not in changed:
                    np.testing.assert_array_equal(p['P'], q['P'])
                else:
                    self.assertFalse(np.array_equal(p['P'], q['P'])); count += 1
        self.assertEqual(count, len(changed))
        for name in PARTS[1:]:
            self.assertEqual((ROOT/'CAD'/f'{name}.step').read_bytes(), (ROOT/'Faired/CAD'/f'{name}.step').read_bytes())

    def test_combined_step_matches_individual_parts_and_mirrors(self):
        combined = read_nets(ROOT/'Faired/CAD/Liger_Macro_CAD.step', half=False)
        expected = []
        for name in PARTS:
            expected.extend(read_nets(ROOT/'Faired/CAD'/f'{name}.step', half=False))
        self.assertEqual(len(combined), 312)
        for a, b in zip(combined, expected):
            np.testing.assert_allclose(a['P'], b['P'], rtol=0, atol=2e-15)
        for name in PARTS:
            paired = read_nets(ROOT/'Faired/CAD'/f'{name}.step', half=False)
            n = len(paired)//2
            for a, b in zip(paired[:n], paired[n:]):
                np.testing.assert_allclose(a['P']*np.array([1, -1, 1]), b['P'], rtol=0, atol=2e-15)

    def test_fairing_thresholds_and_dense_screen(self):
        before = read_nets(ROOT/'CAD/Body_Main_Shell.step')
        after = read_nets(ROOT/'Faired/CAD/Body_Main_Shell.step')
        for r in self.report['changed']:
            i = r['half_patch']
            self.assertGreaterEqual(r['roughness_p95_reduction_percent'], 30)
            self.assertLessEqual(r['curvature']['after']['curvature_gradient_median_per_m2'], r['curvature']['before']['curvature_gradient_median_per_m2']*1.10)
            self.assertLessEqual(r['shape_change']['sampled_max_mm'], 10 if i == 21 else 5)
            self.assertLessEqual(r['shape_change']['rms_mm'], 3)
            screen = dense_normal_screen(before[i], after[i], r['axis'])
            self.assertLessEqual(screen['after_opposed'], screen['before_opposed'])
            if i == 21:
                self.assertEqual(screen['after_opposed'], 0)

    def test_native_document_embeds_exact_delivered_steps(self):
        text = (ROOT/'Faired/CAD/Liger_Macro_CAD.solidarc').read_text()
        for name in PARTS:
            self.assertEqual(text.count((ROOT/'Faired/CAD'/f'{name}.step').read_text()), 1)
        self.assertEqual(text.count('Kind = 18'), 3)

    def test_bonnet_rebuild_is_reproducible(self):
        p = read_nets(ROOT/'CAD/Body_Main_Shell.step')[21]
        target = read_nets(ROOT/'Faired/CAD/Body_Main_Shell.step')[21]
        q = graph_fair(p, 2, 12, area_weighted=True)
        np.testing.assert_allclose(q['P'], target['P'], rtol=0, atol=1e-10)


if __name__ == '__main__':
    unittest.main(verbosity=2)
