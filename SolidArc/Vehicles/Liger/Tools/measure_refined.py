#!/usr/bin/env python3
"""Independent sampled shape-change, seam-normal and insert-clearance checks."""
import argparse
import json
from collections import defaultdict
from pathlib import Path
import numpy as np
from fit_macros import ROOT, evaluate
from fair_surfaces import PARTS, read_nets, mesh, geometry, grid, dense_normal_screen
from tessellate_shared import load_binary, side_uv


def distance_to_region(old, new):
    import bpy  # noqa: F401
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree
    xyz = []; triangles = []; offset = 0
    for p in old:
        q, t = mesh(p, 151); xyz.extend(q); triangles.extend(t+offset); offset += len(q)
    xyz = np.array(xyz)
    tree = BVHTree.FromPolygons(xyz.tolist(), np.array(triangles).tolist(), all_triangles=True)
    q, t = mesh(new, 151)
    target = BVHTree.FromPolygons(q.tolist(), t.tolist(), all_triangles=True)
    forward = np.array([tree.find_nearest(Vector(p))[3]*1000 for p in q[::2]])
    reverse = np.array([target.find_nearest(Vector(p))[3]*1000 for p in xyz[::2]])
    d = np.r_[forward, reverse]
    return dict(rms_mm=float(np.sqrt(np.mean(d*d))), p95_mm=float(np.percentile(d, 95)),
                sampled_max_mm=float(max(d)), samples=len(d),
                method='bidirectional sample vertices to 151x151 triangulated regions; not a Hausdorff bound')


def seam_screen(layout, nets):
    states = {}
    for which in ('before', 'after'):
        top = layout[which][PARTS[0]]; owners = defaultdict(list)
        for net, patch in zip(nets[which][PARTS[0]], top['patches']):
            for side, refs in enumerate(patch['sides']):
                for ref in refs:
                    key = tuple(top['rails'][ref['rail']]['key']); t = np.linspace(.05, .95, 31)
                    s = 1-t if ref['reverse'] else t
                    uv = side_uv(side, ref['start']+(ref['end']-ref['start'])*s)
                    n = geometry(net, uv)[1]; owners[key].append((patch['id'], n))
        values = {}
        for key, items in owners.items():
            if len(items) != 2:
                continue
            angles = np.degrees(np.arccos(np.clip(np.sum(items[0][1]*items[1][1], axis=1), -1, 1)))
            values[key] = dict(patches=[i[0] for i in items], median_degrees=float(np.median(angles)),
                               p95_degrees=float(np.percentile(angles, 95)))
        states[which] = values
    changed = {1, 3, 8, 10, 27, 42, 59, 84}
    return [dict(rail=list(k), before=b, after=states['after'].get(k), removed=k not in states['after'])
            for k, b in states['before'].items() if any(i in changed for i in b['patches'])]


def clearance(D, frame):
    import bpy  # noqa: F401
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree
    tri = D['tri'][D['part'][D['tri'][:, 0]] == 0]
    tree = BVHTree.FromPolygons(D['pos'].tolist(), tri.tolist(), all_triangles=True)
    points = evaluate(**frame, uv=grid(101)); residual = []
    for q in points:
        hit = tree.ray_cast(Vector([q[0], q[1], 2]), Vector([0, 0, -1]))
        if hit[0] is not None and abs(hit[0].z-q[2]) < .02 and abs(hit[1].z) > .4:
            residual.append((q[2]-hit[0].z)*1000)
    if len(residual) < .99*len(points):
        raise ValueError('Insufficient skin coverage for clearance sample')
    return dict(frame_patch_id=0, half_model=True, samples=len(residual), total_samples=len(points),
                maximum_insert_above_skin_mm=float(max(residual)),
                minimum_vertical_clearance_mm=float(-max(residual)),
                method='101x101 insert samples, vertical rays to display triangulation of main skin; not global collision certification')


def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--revision', type=Path, default=ROOT/'Refined')
    args = ap.parse_args(); out = args.revision
    layout = json.loads((out/'CAD/patch_layout.json').read_text())
    nets = {which: {name: read_nets((ROOT/'Faired' if which == 'before' else out)/'CAD'/f'{name}.step')
                    for name in PARTS} for which in ('before', 'after')}
    report = json.loads((out/'refinement_report.json').read_text())
    for row in report['merged']+report['faired']:
        pid = row.get('result_id', row.get('id'))
        index = next(i for i, p in enumerate(layout['after'][PARTS[0]]['patches']) if p['id'] == pid)
        new = nets['after'][PARTS[0]][index]
        ids = row.get('source_ids', [pid]); old = [nets['before'][PARTS[0]][i] for i in ids]
        row['shape_change'] = distance_to_region(old, new)
        if len(ids) == 1:
            row['dense_orientation_screen'] = dense_normal_screen(old[0], new, 1)
        if row['shape_change']['sampled_max_mm'] > 5:
            raise ValueError(f'Region {pid} exceeds 5 mm sampled-change gate')
        print('REGION', pid, row['shape_change'], flush=True)
    report['clearance'] = {which: clearance(load_binary(out/'Proofs'/f'{which}.bin.gz'), nets[which][PARTS[2]][0])
                           for which in ('before', 'after')}
    if report['clearance']['after']['minimum_vertical_clearance_mm'] < .4:
        raise ValueError('Insert correction did not achieve the sampled clearance gate')
    (out/'refinement_report.json').write_text(json.dumps(report, indent=2)+'\n')
    (out/'Proofs/SeamNormalScreen.json').write_text(json.dumps(seam_screen(layout, nets), indent=2)+'\n')
    print('CLEARANCE', report['clearance'], flush=True)


if __name__ == '__main__':
    main()
