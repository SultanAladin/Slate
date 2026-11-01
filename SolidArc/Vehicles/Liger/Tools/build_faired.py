#!/usr/bin/env python3
"""Apply accepted pole nets without rebuilding/changing a single B-rep edge.

Uses the delivered review export as a read-only baseline. Generates a separate
Faired/ revision and direct-NURBS display proofs; no Blender fitting cache needed.
"""
import argparse
import copy
from collections import Counter
import json
import re
import struct
from pathlib import Path
import numpy as np
from scipy.interpolate import BSpline
from fit_macros import ROOT, evaluate
from fair_surfaces import PARTS, read_nets, mesh, dense_normal_screen


def replace_nets(text, nets):
    """Replace unique interior point entities, retaining every edge and knot."""
    surfaces = re.findall(r'B_SPLINE_SURFACE_WITH_KNOTS\((.*?)\);', text)
    if len(surfaces) != len(nets):
        raise ValueError('Surface count changed')
    edits = {}
    counts = Counter(map(int, re.findall(r"#(\d+)", text)))
    points = {int(n): np.fromstring(x, sep=",")/1000 for n, x in
              re.findall(r"#(\d+)=CARTESIAN_POINT\('',\(([^()]*)\)\);", text)}
    for s, net in zip(surfaces, nets):
        rows = re.search(r"^'',3,3,(\(.*?\)),\.UNSPECIFIED\.", s)[1]
        ids = np.array([list(map(int, re.findall(r'#(\d+)', r)))
                        for r in re.findall(r'\(([^()]+)\)', rows)])
        P = net['P']
        if ids.shape != P.shape[:2]:
            raise ValueError('Pole count changed')
        if np.max(abs(np.array([[points[int(i)] for i in row] for row in ids])-P)) < 1e-14:
            continue
        for i in range(1, len(ids)-1):
            for j in range(1, ids.shape[1]-1):
                number = int(ids[i, j])
                # Exporter emits unique surface pole entities: never edit a rail.
                if counts[number] != 2:
                    raise ValueError('Surface pole entity is shared')
                edits[number] = P[i, j]
    def repl(m):
        number = int(m[1])
        if number not in edits:
            return m[0]
        xyz = ','.join(format(float(x*1000), '.16g') for x in edits[number])
        return f"#{number}=CARTESIAN_POINT('',({xyz}));"
    return re.sub(r"#(\d+)=CARTESIAN_POINT\('',\([^()]*\)\);", repl, text)


def full_nets(nets):
    return nets + [dict(P=n['P']*np.array([1, -1, 1]), ku=n['ku'], kv=n['kv']) for n in nets]


def export(nets_dir, output):
    (output/'CAD').mkdir(parents=True, exist_ok=True)
    report = json.loads((nets_dir/'fairing_report.json').read_text())
    by_part = {}; changed = {}; all_full = []; replacements = []
    for name in PARTS:
        baseline = read_nets(ROOT/'CAD'/f'{name}.step')
        nets = copy.deepcopy(baseline)
        ids = [item['half_patch'] for item in report['changed'] if item['part'] == name]
        for i in ids:
            d = np.load(nets_dir/f'{name}_{i:03d}.npz')
            nets[i] = {k: d[k] for k in ('P', 'ku', 'kv')}
            old, new = baseline[i], nets[i]
            screen = dense_normal_screen(old, new, next(r['axis'] for r in report['changed'] if r['part'] == name and r['half_patch'] == i))
            if screen['after_opposed'] > screen['before_opposed']:
                raise ValueError(f'New dense normal-screen defects on patch {i}')
            next(r for r in report['changed'] if r['part'] == name and r['half_patch'] == i)['dense_normal_screen'] = screen
            for k in ('ku', 'kv'):
                assert np.array_equal(old[k], new[k])
            assert np.array_equal(old['P'][[0, -1]], new['P'][[0, -1]])
            assert np.array_equal(old['P'][:, [0, -1]], new['P'][:, [0, -1]])
        by_part[name] = (baseline, nets); changed[name] = set(ids)
        old_text = (ROOT/'CAD'/f'{name}.step').read_text()
        if ids:
            # Only changed surface point entities are edited, not unchanged nets.
            new_text = replace_nets(old_text, full_nets(nets))
        else:
            new_text = old_text
        (output/'CAD'/f'{name}.step').write_text(new_text)
        replacements.append((old_text, new_text))
        all_full.extend(full_nets(nets))
        reconstructed = read_nets(output/'CAD'/f'{name}.step')
        for a, b in zip(nets, reconstructed):
            np.testing.assert_allclose(a['P'], b['P'], atol=2e-15, rtol=0)
    combined = replace_nets((ROOT/'CAD/Liger_Macro_CAD.step').read_text(), all_full)
    (output/'CAD/Liger_Macro_CAD.step').write_text(combined)
    native = (ROOT/'CAD/Liger_Macro_CAD.solidarc').read_text()
    for old_text, new_text in replacements:
        assert native.count(old_text) == 1
        native = native.replace(old_text, new_text)
    native = native.replace('crease-preserving macro CAD (review build)', 'faired NURBS CAD (review build)')
    (output/'CAD/Liger_Macro_CAD.solidarc').write_text(native)
    (output/'Proofs').mkdir(exist_ok=True)
    (output/'Proofs/DenseNormalScreen.json').write_text(json.dumps([dict(half_patch=r['half_patch'], axis=r['axis'], **r['dense_normal_screen']) for r in report['changed']], indent=2)+'\n')
    (output/'fairing_report.json').write_text(json.dumps(report, indent=2)+'\n')
    top = json.loads((ROOT/'CAD/metrics.json').read_text())
    # Historical fitting metrics do not describe the intentionally faired surface.
    topology = [{k: v for k, v in p.items() if k in (
        'name', 'faces', 'control_poles', 'master_rails_half', 'sewn_edges', 'open_edges',
        'nonmanifold_edges', 'open_shells', 'source_nonmanifold_branches_detached', 'boundary_max_mm')}
        for p in top['parts']]
    (output/'CAD/metrics.json').write_text(json.dumps(dict(parts=topology,
        note='Topology retained; boundary geometry unchanged. Current source distances: ../Proofs/reverse_deviation.json. Shape changes: ../fairing_report.json.'), indent=2)+'\n')
    return by_part, changed


def make_proofs(by_part, changed, output):
    import bpy  # noqa: F401
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree
    out = output/'Proofs'; out.mkdir(exist_ok=True)
    original = np.load(ROOT/'Proofs/inspection_geometry.npz')
    D = {k: original[k].copy() for k in original.files}
    meta = json.loads((ROOT/'Proofs/viewer_metadata.json').read_text())
    meta['before_faces'] = copy.deepcopy(meta['faces'])
    change = np.zeros(len(D['pos']), dtype=np.float32)
    stats = []
    for part, name in enumerate(PARTS):
        ref = np.load(ROOT/'Source'/f'{name}_reference.npz')['S']
        rv = ref.reshape(-1, 3)
        r = np.arange(len(rv)).reshape(-1, 5, 5)
        rt = np.stack([r[:, :-1, :-1], r[:, 1:, :-1], r[:, 1:, 1:],
                       r[:, :-1, :-1], r[:, 1:, 1:], r[:, :-1, 1:]], axis=-1).reshape(-1, 3)
        tree = BVHTree.FromPolygons(rv.tolist(), rt.tolist(), all_triangles=True)
        old_nets, new_nets = by_part[name]
        for patch, (old, new) in enumerate(zip(old_nets, new_nets)):
            faces = [i for i, f in enumerate(meta['faces']) if f['name'] == name and f['patch'] == patch]
            if patch in changed[name]:
                P, ku, kv = new['P'], new['ku'], new['kv']
                nu = max(14, min(88, (len(ku)-4)*2)); nv = max(14, min(88, (len(kv)-4)*2))
                u, v = np.meshgrid(np.linspace(0, 1, nu), np.linspace(0, 1, nv), indexing='ij')
                uv = np.c_[u.ravel(), v.ravel()]
                pos = evaluate(P, ku, kv, uv)
                bu, bv = BSpline(ku, np.eye(len(ku)-4), 3), BSpline(kv, np.eye(len(kv)-4), 3)
                du = np.einsum('ni,nj,ijc->nc', bu(uv[:, 0], nu=1), bv(uv[:, 1]), P, optimize=True)
                dv = np.einsum('ni,nj,ijc->nc', bu(uv[:, 0]), bv(uv[:, 1], nu=1), P, optimize=True)
                normal = np.cross(du, dv); normal /= np.maximum(np.linalg.norm(normal, axis=1)[:, None], 1e-20)
                err = np.array([tree.find_nearest(Vector(p))[3]*1000 for p in pos])
                xyz, tri = mesh(old, 151)
                previous_tree = BVHTree.FromPolygons(xyz.tolist(), tri.tolist(), all_triangles=True)
                delta = np.array([previous_tree.find_nearest(Vector(p))[3]*1000 for p in pos])
                for face in faces:
                    mask = D['face'] == face
                    assert mask.sum() == len(pos)
                    sign = np.array([1, -1 if meta['faces'][face]['mirror'] else 1, 1])
                    D['pos'][mask] = pos*sign; D['nor'][mask] = normal*sign
                    D['error_mm'][mask] = err; change[mask] = delta
            for face in faces:
                mask = D['face'] == face
                for f, err in [(meta['faces'][face], D['error_mm'][mask]),
                               (meta['before_faces'][face], original['error_mm'][mask])]:
                    f['rms'] = float(np.sqrt(np.mean(err**2))); f['max'] = float(err.max())
                    f['metric'] = 'CAD grid to L3 reference triangle'
                meta['faces'][face]['faired'] = patch in changed[name]
        err = D['error_mm'][D['part'] == part]
        stats.append(dict(name=name, cad_grid_points_full=len(err),
            cad_to_L3_triangle_rms_mm=float(np.sqrt(np.mean(err**2))),
            cad_to_L3_triangle_p99_mm=float(np.percentile(err, 99)),
            cad_to_L3_triangle_max_mm=float(err.max())))
        print('REFERENCE', stats[-1], flush=True)
    # The triangles are a display of the edited pole nets, never the edited CAD.
    np.savez_compressed(out/'inspection_geometry.npz', **D, change_mm=change)
    with (out/'geometry.bin').open('wb') as f:
        f.write(struct.pack('<III', len(D['pos']), D['tri'].size, len(D['lines'])))
        packed = np.c_[D['pos'], D['nor'], D['error_mm'], D['face'], D['part']].astype('<f4')
        f.write(packed.tobytes()); f.write(D['tri'].astype('<u4').tobytes()); f.write(D['lines'].astype('<f4').tobytes())
    change.astype('<f4').tofile(out/'change.bin')
    meta['revision'] = 'Faired NURBS review; before/after share fixed boundaries'
    (out/'viewer_metadata.json').write_text(json.dumps(meta, indent=2)+'\n')
    (out/'reverse_deviation.json').write_text(json.dumps(stats, indent=2)+'\n')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--nets', type=Path, required=True)
    ap.add_argument('--output', type=Path, default=ROOT/'Faired')
    args = ap.parse_args()
    by_part, changed = export(args.nets, args.output)
    make_proofs(by_part, changed, args.output)


if __name__ == '__main__':
    main()
