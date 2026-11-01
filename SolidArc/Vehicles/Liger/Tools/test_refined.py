"""Regression tests for the delivered targeted refinement and shared-edge display.

Optional native checks: CERTIFIER=/path/to/CadCertifier python -m unittest ...
"""
import gzip
import json
import os
from pathlib import Path
import re
import subprocess
from tempfile import TemporaryDirectory
import unittest
import numpy as np
from fit_macros import ROOT
from fair_surfaces import PARTS, read_nets
from tessellate_shared import rail_samples, patch_mesh, write_binary, load_binary


class RefinedTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = ROOT/'Refined'
        cls.layout = json.loads((cls.root/'CAD/patch_layout.json').read_text())
        cls.report = json.loads((cls.root/'refinement_report.json').read_text())
        cls.nets = {w: {n: read_nets((ROOT/'Faired' if w == 'before' else cls.root)/'CAD'/f'{n}.step')
                        for n in PARTS} for w in ('before', 'after')}

    def test_count_and_complete_source_provenance(self):
        self.assertEqual(sum(len(n) for n in self.nets['after'].values())*2, 306)
        for name in PARTS:
            ids = [i for p in self.layout['after'][name]['patches'] for i in p['source_ids']]
            self.assertEqual(sorted(ids), list(range(len(self.nets['before'][name]))))
        self.assertEqual({tuple(x['source_ids']) for x in self.report['merged']}, {(1, 10), (3, 8), (59, 84)})

    def test_main_shell_exterior_rails_and_unchanged_surfaces(self):
        before = self.layout['before'][PARTS[0]]; after = self.layout['after'][PARTS[0]]
        old_rails = {tuple(r['key']): r for r in before['rails']}
        new_rails = {tuple(r['key']): r for r in after['rails']}
        self.assertEqual(len(set(old_rails)-set(new_rails)), 3)
        self.assertFalse(set(new_rails)-set(old_rails))
        for key, rail in new_rails.items():
            np.testing.assert_array_equal(rail['K'], old_rails[key]['K'])
            np.testing.assert_allclose(rail['P'], old_rails[key]['P'], atol=2e-15, rtol=0)
        for net, p in zip(self.nets['after'][PARTS[0]], after['patches']):
            if len(p['source_ids']) == 1 and p['id'] not in (27, 42):
                old = self.nets['before'][PARTS[0]][p['id']]
                for k in ['P', 'ku', 'kv']:
                    np.testing.assert_allclose(net[k], old[k], atol=2e-15, rtol=0)

    def test_insert_is_rigid_one_mm_translation(self):
        for old, new in zip(self.nets['before'][PARTS[2]], self.nets['after'][PARTS[2]]):
            np.testing.assert_allclose(new['P'], old['P']+np.array([0, 0, -.001]), atol=2e-15, rtol=0)
            for k in ['ku', 'kv']:
                np.testing.assert_array_equal(old[k], new[k])
        for old, new in zip(self.nets['before'][PARTS[1]], self.nets['after'][PARTS[1]]):
            np.testing.assert_allclose(old['P'], new['P'], atol=2e-15, rtol=0)

    def test_bounded_poles_and_rims_for_arch_fairing(self):
        top = self.layout['after'][PARTS[0]]['patches']
        for pid in [27, 42]:
            old = self.nets['before'][PARTS[0]][pid]
            index = next(i for i, p in enumerate(top) if p['id'] == pid)
            new = self.nets['after'][PARTS[0]][index]
            self.assertLessEqual(np.max(abs(new['P']-old['P'])), .002000000001)
            np.testing.assert_array_equal(old['P'][[0, -1]], new['P'][[0, -1]])
            np.testing.assert_array_equal(old['P'][:, [0, -1]], new['P'][:, [0, -1]])
        for row in self.report['faired']:
            self.assertGreater(row['solver_status'], 0)
            self.assertEqual(row['dense_orientation_screen']['after_opposed'], 0)

    def test_sampled_change_and_clearance_gates(self):
        for row in self.report['merged']+self.report['faired']:
            self.assertLess(row['shape_change']['sampled_max_mm'], 5)
            self.assertLess(row['shape_change']['rms_mm'], 1.5)
        self.assertGreater(self.report['clearance']['before']['maximum_insert_above_skin_mm'], .4)
        self.assertGreater(self.report['clearance']['after']['minimum_vertical_clearance_mm'], .4)
        seam = json.loads((self.root/'Proofs/SeamNormalScreen.json').read_text())
        self.assertEqual(sum(r['removed'] for r in seam), 3)
        for r in seam:
            if r['after'] and any(i in (27, 42) for i in r['after']['patches']):
                self.assertLessEqual(r['after']['p95_degrees'], r['before']['p95_degrees']+.25)

    def plane(self):
        g = np.linspace(0, 1, 4); u, v = np.meshgrid(g, g, indexing='ij')
        P = np.stack([u, v, u*0], axis=-1); K = np.r_[np.zeros(4), np.ones(4)]
        curves = [P[:, 0], P[-1], P[:, -1], P[0]]
        rails = [dict(P=p.tolist(), K=K.tolist()) for p in curves]
        patch = dict(sides=[[dict(rail=i, reverse=i >= 2, start=0., end=1.)] for i in range(4)])
        return dict(P=P, ku=K, kv=K), patch, rails

    def test_constrained_plane_mesh_and_analytic_normals(self):
        net, patch, rails = self.plane()
        q, n, tri, lines, records, gap = patch_mesh(net, patch, [rail_samples(r) for r in rails])
        self.assertLess(gap, 1e-12)
        np.testing.assert_allclose(q[:, 2], 0, atol=1e-15)
        np.testing.assert_allclose(n, np.tile([0, 0, 1], (len(n), 1)), atol=1e-14)
        a, b, c = [q[tri[:, i]] for i in range(3)]
        self.assertTrue(np.all(np.cross(b-a, c-a)[:, 2] > 0))
        self.assertEqual(len(set(tri.ravel())), len(q))

    def test_bad_rail_is_not_hidden_by_boundary_vertex_override(self):
        net, patch, rails = self.plane()
        rails[0]['P'][1][2] = .001
        with self.assertRaisesRegex(ValueError, 'does not match'):
            patch_mesh(net, patch, [rail_samples(r) for r in rails])

    def test_binary_round_trip_and_conformance_audit(self):
        net, patch, rails = self.plane()
        p, n, t, lines, _, _ = patch_mesh(net, patch, [rail_samples(r) for r in rails])
        D = dict(pos=p, nor=n, tri=t, lines=lines, error_mm=np.zeros(len(p)),
                 change_mm=np.ones(len(p)), face=np.zeros(len(p)), part=np.zeros(len(p)))
        with TemporaryDirectory() as d:
            path = Path(d)/'geometry.bin.gz'; write_binary(path, D); loaded = load_binary(path)
            for k in D:
                np.testing.assert_allclose(D[k], loaded[k], atol=1e-7)
        audit = json.loads((self.root/'Proofs/TessellationAudit.json').read_text())
        for state in audit.values():
            self.assertEqual(state['maximum_shared_vertex_gap_mm'], 0.)
            self.assertGreater(state['seam_owner_comparisons'], 300)
            self.assertLess(state['maximum_surface_rail_gap_mm'], .000001)

    def test_native_embeds_exact_step_and_combined_surface_count(self):
        text = (self.root/'CAD/Liger_Refined_CAD.solidarc').read_text()
        all_nets = read_nets(self.root/'CAD/Liger_Refined_CAD.step', half=False)
        self.assertEqual(len(all_nets), 306)
        cursor = 0
        for name in PARTS:
            self.assertEqual(text.count((self.root/'CAD'/f'{name}.step').read_text()), 1)
            individual = read_nets(self.root/'CAD'/f'{name}.step', half=False)
            for a, b in zip(all_nets[cursor:cursor+len(individual)], individual):
                np.testing.assert_array_equal(a['P'], b['P'])
            cursor += len(individual)

    @unittest.skipUnless(os.environ.get('CERTIFIER'), 'Set CERTIFIER for native regression')
    def test_native_zero_tolerant_edges_and_real_gap_control(self):
        def run(path):
            return subprocess.run([os.environ['CERTIFIER'], str(path)], capture_output=True, text=True, check=True).stdout
        valid = run(self.root/'CAD/Liger_Refined_CAD.step')
        self.assertIn('tolerantEdges=0 tolerantVerts=0', valid)
        self.assertLess(float(re.search(r'largestGap_mm=([\deE.+-]+)', valid)[1]), .000001)
        invalid = run(ROOT/'Tools/Fixtures/ExpectedGapControl.step')
        self.assertIn('tolerantEdges=1', invalid)
        self.assertGreater(float(re.search(r'largestGap_mm=([\deE.+-]+)', invalid)[1]), .43)


if __name__ == '__main__':
    unittest.main(verbosity=2)
