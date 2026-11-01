#!/usr/bin/env python3
"""Conforming display tessellation of exact STEP nets + shared rail topology.

The B-rep has T-junctions: independently sampled rectangular face grids make
false visible cracks. Every owner here uses the SAME canonical rail vertices;
constrained UV triangulation connects those boundaries to interior samples.
Only positions evaluated from the actual CAD are used, never smooth display
normals. Geometry is serialized gzip-compressed for the comparison viewer.
"""
import argparse
import gzip
import json
import struct
from pathlib import Path
from collections import defaultdict
import numpy as np
import triangle
from scipy.interpolate import BSpline
from fit_macros import ROOT, evaluate
from fair_surfaces import PARTS, read_nets, mesh


def rail_samples(rail, tolerance=1.5e-5, max_length=.012):
    curve = BSpline(np.array(rail['K']), np.array(rail['P']), 3)
    values = set(np.unique(rail['K']).tolist())
    def split(a, b, depth=0):
        pa, pb = curve([a, b]); t = np.array([a+(b-a)*f for f in (.25, .5, .75)])
        actual = curve(t); chord = pa+(pb-pa)*np.array([.25, .5, .75])[:, None]
        if depth < 16 and (np.max(np.linalg.norm(actual-chord, axis=1)) > tolerance or np.linalg.norm(pb-pa) > max_length):
            mid = (a+b)/2; values.add(mid); split(a, mid, depth+1); split(mid, b, depth+1)
    initial = sorted(values)
    for a, b in zip(initial[:-1], initial[1:]):
        split(a, b)
    t = np.array(sorted(values))
    return t, curve(t)


def side_uv(side, t):
    return [np.c_[t, t*0], np.c_[t*0+1, t], np.c_[1-t, t*0+1], np.c_[t*0, 1-t]][side]


def patch_mesh(net, patch, samples):
    uv = []; exact = []; spans = []; owner_records = []
    for side, refs in enumerate(patch['sides']):
        for ref in refs:
            t, xyz = samples[ref['rail']]
            if ref['reverse']:
                t, xyz = 1-t[::-1], xyz[::-1]
            parameter = ref['start']+(ref['end']-ref['start'])*t
            edge_uv = side_uv(side, parameter)
            start = len(uv)
            uv.extend(edge_uv[:-1]); exact.extend(xyz[:-1])
            spans.append((start, len(edge_uv)-1))
            owner_records.append((ref['rail'], ref['reverse'], start, len(edge_uv)))
    boundary_count = len(uv)
    # Boundary UVs form one CCW rectangle. Each segment is a shared-rail chord.
    segments = np.c_[np.arange(boundary_count), np.roll(np.arange(boundary_count), -1)]
    nu, nv = net['P'].shape[:2]
    us = np.unique(np.r_[np.linspace(0, 1, max(12, min(64, nu*2))), net['ku']])
    vs = np.unique(np.r_[np.linspace(0, 1, max(12, min(64, nv*2))), net['kv']])
    u, v = np.meshgrid(us[(us > 1e-12) & (us < 1-1e-12)], vs[(vs > 1e-12) & (vs < 1-1e-12)], indexing='ij')
    interior = np.c_[u.ravel(), v.ravel()]
    uv = np.r_[np.array(uv), interior]
    triangulation = triangle.triangulate(dict(vertices=uv, segments=segments), 'pQ')
    if len(triangulation['vertices']) != len(uv):
        raise ValueError('Unexpected boundary Steiner vertices; cannot silently break conformity')
    if not np.array_equal(triangulation['vertices'], uv):
        raise ValueError('Unexpected Triangle vertex renumbering')
    tri = triangulation['triangles']
    q = evaluate(**net, uv=uv)
    # Same analytic rail, same samples, bit-identical float32 endpoints per owner.
    # Surface/rail agreement is checked before this assignment, not concealed.
    mismatch = float(np.max(np.linalg.norm(q[:boundary_count]-np.array(exact), axis=1)))
    if mismatch > 1e-9:
        raise ValueError(f'CAD surface does not match its shared rails: {mismatch} m')
    q[:boundary_count] = exact
    P, ku, kv = net['P'], net['ku'], net['kv']
    bu, bv = BSpline(ku, np.eye(nu), 3), BSpline(kv, np.eye(nv), 3)
    du = np.einsum('ni,nj,ijc->nc', bu(uv[:, 0], nu=1), bv(uv[:, 1]), P, optimize=True)
    dv = np.einsum('ni,nj,ijc->nc', bu(uv[:, 0]), bv(uv[:, 1], nu=1), P, optimize=True)
    normal = np.cross(du, dv); normal /= np.maximum(np.linalg.norm(normal, axis=1)[:, None], 1e-20)
    # Verify that no boundary segment was skipped by the constrained mesh.
    edges = {tuple(sorted(e)) for t in tri for e in [(t[0], t[1]), (t[1], t[2]), (t[2], t[0])]}
    if any(tuple(sorted(e)) not in edges for e in segments):
        raise ValueError('Missing constrained boundary segment')
    records = []
    for rail, reverse, start, count in owner_records:
        indices = (np.arange(count)+start) % boundary_count
        values = q[indices]
        records.append((rail, values[::-1] if reverse else values))
    return q, normal, tri, q[segments], records, mismatch


def write_binary(path, D):
    header = struct.pack('<III', len(D['pos']), D['tri'].size, len(D['lines']))
    packed = np.c_[D['pos'], D['nor'], D['error_mm'], D['face'], D['part'], D['change_mm']].astype('<f4')
    data = header+packed.tobytes()+D['tri'].astype('<u4').tobytes()+D['lines'].astype('<f4').tobytes()
    path.write_bytes(gzip.compress(data, compresslevel=9, mtime=0))


def load_binary(path):
    data = gzip.decompress(Path(path).read_bytes()); nv, ni, nl = struct.unpack_from('<III', data)
    packed = np.frombuffer(data, '<f4', count=nv*10, offset=12).reshape(nv, 10)
    off = 12+nv*40; tri = np.frombuffer(data, '<u4', count=ni, offset=off).reshape(-1, 3)
    lines = np.frombuffer(data, '<f4', count=nl*6, offset=off+ni*4).reshape(-1, 2, 3)
    return dict(pos=packed[:, :3], nor=packed[:, 3:6], error_mm=packed[:, 6],
                face=packed[:, 7], part=packed[:, 8], change_mm=packed[:, 9], tri=tri, lines=lines)


def make_revision(which, layout, out, baseline_nets):
    import bpy  # noqa: F401
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree
    fields = defaultdict(list); meta = dict(faces=[], revision=which); offset = 0; face = 0
    audit = dict(parts=[], seam_owner_comparisons=0, maximum_shared_vertex_gap_mm=0., maximum_surface_rail_gap_mm=0.)
    for part, name in enumerate(PARTS):
        nets = baseline_nets[name] if which == 'before' else read_nets(out/'CAD'/f'{name}.step')
        topology = layout[which][name]
        if len(nets) != len(topology['patches']):
            raise ValueError('STEP/layout face count mismatch')
        samples = [rail_samples(r) for r in topology['rails']]
        source = np.load(ROOT/'Source'/f'{name}_reference.npz')['S']
        rv = source.reshape(-1, 3); ix = np.arange(len(rv)).reshape(-1, 5, 5)
        rt = np.stack([ix[:, :-1, :-1], ix[:, 1:, :-1], ix[:, 1:, 1:],
                       ix[:, :-1, :-1], ix[:, 1:, 1:], ix[:, :-1, 1:]], axis=-1).reshape(-1, 3)
        tree = BVHTree.FromPolygons(rv.tolist(), rt.tolist(), all_triangles=True)
        owners = defaultdict(list); part_errors = []
        for net, patch in zip(nets, topology['patches']):
            xyz, normal, tri, lines, records, gap = patch_mesh(net, patch, samples)
            audit['maximum_surface_rail_gap_mm'] = max(audit['maximum_surface_rail_gap_mm'], gap*1000)
            for rail, values in records:
                # The intentionally detached rear branch keeps its own namespace.
                if name == PARTS[0] and patch['id'] == 137:
                    continue
                for previous in owners[rail]:
                    if previous.shape != values.shape:
                        raise ValueError('Shared-rail sample count mismatch')
                    delta = np.max(np.linalg.norm(previous.astype('float32')-values.astype('float32'), axis=1))*1000
                    audit['maximum_shared_vertex_gap_mm'] = max(audit['maximum_shared_vertex_gap_mm'], float(delta))
                    audit['seam_owner_comparisons'] += 1
                owners[rail].append(values)
            err = np.array([tree.find_nearest(Vector(p))[3]*1000 for p in xyz])
            part_errors.extend(err)
            change = np.zeros(len(xyz))
            if which == 'after' and (name == PARTS[2] or len(patch['source_ids']) > 1 or (name == PARTS[0] and patch['id'] in (27, 42))):
                oldxyz = []; oldtri = []; start = 0
                for pid in patch['source_ids']:
                    q, t = mesh(baseline_nets[name][pid], 151)
                    oldxyz.extend(q); oldtri.extend(t+start); start += len(q)
                oldtree = BVHTree.FromPolygons(np.array(oldxyz).tolist(), np.array(oldtri).tolist(), all_triangles=True)
                change = np.array([oldtree.find_nearest(Vector(p))[3]*1000 for p in xyz])
            for mirrored in (False, True):
                sign = np.array([1, -1 if mirrored else 1, 1])
                fields['pos'].append(xyz*sign); fields['nor'].append(normal*sign)
                fields['tri'].append((tri[:, ::-1] if mirrored else tri)+offset)
                fields['lines'].append(lines*sign)
                fields['error_mm'].append(err); fields['change_mm'].append(change)
                fields['face'].append(np.full(len(xyz), face)); fields['part'].append(np.full(len(xyz), part))
                meta['faces'].append(dict(name=name, patch=patch['id'], source_ids=patch['source_ids'], mirror=mirrored,
                    poles=list(net['P'].shape[:2]), rms=float(np.sqrt(np.mean(err**2))), max=float(err.max())))
                offset += len(xyz); face += 1
        d = np.array(part_errors)
        audit['parts'].append(dict(name=name, half_vertices=len(d), rms_source_mm=float(np.sqrt(np.mean(d*d))),
                                  p99_source_mm=float(np.percentile(d, 99)), max_source_mm=float(max(d))))
        print(which, name, 'vertices half', len(d), flush=True)
    if audit['maximum_shared_vertex_gap_mm'] != 0:
        raise ValueError('Non-conforming boundary samples')
    D = {k: np.concatenate(v) for k, v in fields.items()}
    write_binary(out/'Proofs'/f'{which}.bin.gz', D)
    (out/'Proofs'/f'{which}_metadata.json').write_text(json.dumps(meta, separators=(',', ':'))+'\n')
    return audit


def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--revision', type=Path, default=ROOT/'Refined')
    args = ap.parse_args(); out = args.revision; (out/'Proofs').mkdir(exist_ok=True)
    layout = json.loads((out/'CAD/patch_layout.json').read_text())
    before = {n: read_nets(ROOT/'Faired/CAD'/f'{n}.step') for n in PARTS}
    audits = {which: make_revision(which, layout, out, before) for which in ('before', 'after')}
    (out/'Proofs/TessellationAudit.json').write_text(json.dumps(audits, indent=2)+'\n')


if __name__ == '__main__':
    main()
