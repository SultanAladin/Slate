#!/usr/bin/env python3
"""Boundary-preserving graph fairing of this export's nonrational cubic nets.

Not a general STEP reader or Class-A surfacer. Coons reparameterization removes
folded interior XY maps; a coarse physical height field rejects polygon noise.
The final cubic is fitted to height AND physical first/second derivatives.
All four original boundary control curves and knot vectors stay byte-identical.
Only candidates passing common-position curvature and bidirectional mesh-distance
screens are emitted. Those sampled screens are NOT self-intersection certificates.
"""
from pathlib import Path
import argparse
import json
import re
import numpy as np
from scipy.interpolate import BSpline
from scipy.sparse import csr_matrix, diags, vstack, eye
from scipy.sparse.linalg import spsolve
from scipy.spatial import cKDTree
from fit_macros import ROOT, knots as open_knots, basis, evaluate
PARTS = ["Body_Main_Shell", "Body_Front_Cowl", "Body_Roof_Glass_Frame"]


def read_nets(path, half=True):
    """Read only the explicit cubic surfaces emitted by export_cad.py; mm -> m."""
    text = Path(path).read_text()
    points = {int(i): np.fromstring(s, sep=',') / 1000 for i, s in
              re.findall(r'#(\d+)=CARTESIAN_POINT\(\'\',\(([^()]*)\)\);', text)}
    nets = []
    for body in re.findall(r'B_SPLINE_SURFACE_WITH_KNOTS\((.*?)\);', text):
        fields, depth, start = [], 0, 0
        for i, c in enumerate(body):
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
            elif c == ',' and depth == 0:
                fields.append(body[start:i]); start = i + 1
        fields.append(body[start:])
        if fields[1:3] != ['3', '3']:
            raise ValueError('Only cubic exported surfaces are supported')
        rows = [list(map(int, re.findall(r'#(\d+)', r)))
                for r in re.findall(r'\(([^()]+)\)', fields[3])]
        P = np.array([[points[i] for i in row] for row in rows])
        knots = []
        for im, ik in [(8, 10), (9, 11)]:
            mult = np.fromstring(fields[im].strip('()'), sep=',', dtype=int)
            values = np.fromstring(fields[ik].strip('()'), sep=',')
            knots.append(np.repeat(values, mult))
        ku, kv = knots
        if P.shape != (len(ku)-4, len(kv)-4, 3):
            raise ValueError('Pole/knot mismatch')
        nets.append(dict(P=P, ku=ku, kv=kv))
    if not nets or (half and len(nets) % 2):
        raise ValueError('Expected paired right/left surfaces')
    return nets[:len(nets)//2] if half else nets


def row_tensor(a, b):
    return np.einsum('ni,nj->nij', a, b).reshape(len(a), -1)


def operators(ku, kv, uv):
    nu, nv = len(ku)-4, len(kv)-4
    bu, bv = BSpline(ku, np.eye(nu), 3), BSpline(kv, np.eye(nv), 3)
    u = [bu(uv[:, 0], nu=i) for i in range(3)]
    v = [bv(uv[:, 1], nu=i) for i in range(3)]
    return [csr_matrix(row_tensor(u[i], v[j])) for i, j in
            [(0, 0), (1, 0), (0, 1), (2, 0), (1, 1), (0, 2)]]


def grid(n, inset=0):
    u, v = np.meshgrid(np.linspace(inset, 1-inset, n),
                       np.linspace(inset, 1-inset, n), indexing='ij')
    return np.c_[u.ravel(), v.ravel()]


def geometry(net, uv):
    P = net['P'].reshape(-1, 3)
    q, u, v, uu, uvv, vv = [o@P for o in operators(net['ku'], net['kv'], uv)]
    normal = np.cross(u, v)
    magnitude = np.linalg.norm(normal, axis=1)
    normal /= np.maximum(magnitude[:, None], 1e-15)
    E = np.einsum('ij,ij->i', u, u)
    F = np.einsum('ij,ij->i', u, v)
    G = np.einsum('ij,ij->i', v, v)
    e, f, g = [np.einsum('ij,ij->i', normal, x) for x in (uu, uvv, vv)]
    H = (e*G - 2*f*F + g*E) / np.maximum(2*(E*G-F*F), 1e-24)
    return q, normal, H, u, v


def curvature(P, ku, kv):
    """Legacy UV-grid screen. Use common_position_curvature for comparisons."""
    q, normals, H, u, v = geometry(dict(P=P, ku=ku, kv=kv), grid(61, .025))
    h = H.reshape(61, 61)
    ds = [np.linalg.norm(t, axis=1).reshape(61, 61)*(.95/60) for t in (u, v)]
    grad = np.hypot(np.gradient(h, axis=0)/np.maximum(ds[0], 1e-12),
                    np.gradient(h, axis=1)/np.maximum(ds[1], 1e-12))
    return dict(curvature_gradient_median_per_m2=float(np.median(abs(grad))),
                curvature_gradient_p95_per_m2=float(np.percentile(abs(grad), 95)),
                mean_curvature_abs_p95_per_m=float(np.percentile(abs(H), 95)))


def graph_fair(net, axis, n=12, area_weighted=False):
    """Replace only the patch interior; fit a smooth height graph in physical axes."""
    P, ku, kv = net['P'], net['ku'], net['kv']
    axes = [a for a in range(3) if a != axis] + [axis]
    R = P[:, :, axes].copy()
    nu, nv = R.shape[:2]
    gu = np.array([np.mean(ku[i+1:i+4]) for i in range(nu)])
    gv = np.array([np.mean(kv[i+1:i+4]) for i in range(nv)])
    u, v = gu[:, None, None], gv[None, :, None]
    # Exact-boundary Coons XY, not a convex interpolation of the old bad map.
    coons = ((1-v)*R[:, 0:1] + v*R[:, -1:] +
             (1-u)*R[0:1, :] + u*R[-1:, :] -
             ((1-u)*(1-v)*R[0, 0] + (1-u)*v*R[0, -1] +
              u*(1-v)*R[-1, 0] + u*v*R[-1, -1]))
    Q = R.copy(); Q[1:-1, 1:-1, :2] = coons[1:-1, 1:-1, :2]
    B, U, V, UU, UV, VV = operators(ku, kv, grid(81))
    q = Q.reshape(-1, 3)
    qu, qv, quu, quv, qvv = [o@q for o in (U, V, UU, UV, VV)]
    # Refuse maps that cannot describe this patch as a single-valued graph.
    det = qu[:, 0]*qv[:, 1] - qu[:, 1]*qv[:, 0]
    inside = np.all((grid(81) > .015) & (grid(81) < .985), axis=1)
    sign = np.sign(np.median(det[inside]))
    if np.min(det[inside]*sign) <= 1e-9:
        raise ValueError('Coons graph projection folds or degenerates')
    xyz = B@R.reshape(-1, 3)
    low, high = xyz[:, :2].min(axis=0), xyz[:, :2].max(axis=0)
    span = high-low
    if min(span) < .02:
        raise ValueError('Too narrow for height-field fairing')
    local = np.clip((xyz[:, :2]-low)/span, 0, 1)
    kx, ky = open_knots(n), open_knots(n)
    A = row_tensor(basis(local[:, 0], kx).toarray(), basis(local[:, 1], ky).toarray())
    # Dimensionless coarse-domain Laplacian; no fit to polygon micro-noise.
    D = csr_matrix(np.diff(np.eye(n), 2, axis=0))
    from scipy.sparse import kron
    L = vstack([kron(D, eye(n)), kron(eye(n), D)]).tocsr()
    A = csr_matrix(A)
    if area_weighted:
        # Folded bonnet UV strongly oversamples its narrow edge. Physical area
        # weighting prevents that edge from biasing the broad height field.
        pu, pv = U@R.reshape(-1, 3), V@R.reshape(-1, 3)
        area = np.linalg.norm(np.cross(pu, pv), axis=1)
        W = diags(area/max(np.mean(area), 1e-15))
        C = spsolve((A.T@W@A + eye(n*n)*1e-6).tocsc(), A.T@W@xyz[:, 2])
    else:
        C = spsolve((A.T@A + .05*L.T@L + eye(n*n)*1e-9).tocsc(), A.T@xyz[:, 2])
    local = np.clip(((B@q)[:, :2]-low)/span, 0, 1)
    bx, by = BSpline(kx, np.eye(n), 3), BSpline(ky, np.eye(n), 3)
    def g(i, j):
        return row_tensor(bx(local[:, 0], nu=i), by(local[:, 1], nu=j))@C / (span[0]**i * span[1]**j)
    target, gx, gy, gxx, gxy, gyy = [g(i, j) for i, j in [(0, 0), (1, 0), (0, 1), (2, 0), (1, 1), (0, 2)]]
    lu = np.maximum(np.linalg.norm(qu[:, :2], axis=1), .01)
    lv = np.maximum(np.linalg.norm(qv[:, :2], axis=1), .01)
    tu = gx*qu[:, 0] + gy*qu[:, 1]
    tv = gx*qv[:, 0] + gy*qv[:, 1]
    tuu = gxx*qu[:, 0]**2 + 2*gxy*qu[:, 0]*qu[:, 1] + gyy*qu[:, 1]**2 + gx*quu[:, 0] + gy*quu[:, 1]
    tuv = gxx*qu[:, 0]*qv[:, 0] + gxy*(qu[:, 0]*qv[:, 1]+qu[:, 1]*qv[:, 0]) + gyy*qu[:, 1]*qv[:, 1] + gx*quv[:, 0] + gy*quv[:, 1]
    tvv = gxx*qv[:, 0]**2 + 2*gxy*qv[:, 0]*qv[:, 1] + gyy*qv[:, 1]**2 + gx*qvv[:, 0] + gy*qvv[:, 1]
    # Hermite/Sobolev projection: matching height alone leaves rippled normals.
    DB = vstack((B, diags(.05/lu)@U, diags(.05/lv)@V,
                 diags(.003/lu**2)@UU, diags(.003/(lu*lv))@UV,
                 diags(.003/lv**2)@VV)).tocsr()
    target = np.r_[target, .05*tu/lu, .05*tv/lv, .003*tuu/lu**2,
                   .003*tuv/(lu*lv), .003*tvv/lv**2]
    free = np.zeros((nu, nv), bool); free[1:-1, 1:-1] = True
    ids = np.flatnonzero(free)
    A = DB[:, ids]
    z = spsolve((A.T@A + eye(len(ids))*1e-8).tocsc(), A.T@(target-DB@q[:, 2]))
    zfield = Q[:, :, 2].copy().ravel(); zfield[ids] += z
    Q[:, :, 2] = zfield.reshape(nu, nv)
    out = Q[:, :, np.argsort(axes)]
    # No round-trip changes whatsoever to protected edge poles/parameters.
    out[[0, -1], :, :] = P[[0, -1], :, :]
    out[:, [0, -1], :] = P[:, [0, -1], :]
    if not np.all(np.isfinite(out)):
        raise ValueError('Non-finite fit')
    return dict(P=out, ku=ku.copy(), kv=kv.copy())


def common_position_curvature(old, new, axis):
    """Compare invariant mean curvature at matched physical projected positions.

    Old UV is the common sampling measure for BOTH. New UV is inverted by XY
    Newton iteration; unmatched/edge/fold points excluded explicitly. Gradient
    screen uses the same physical neighbour spacing for both fields.
    """
    uv = grid(61, .025)
    q, normal, h0, _, _ = geometry(old, uv)
    xy = [a for a in range(3) if a != axis]
    seed = grid(91)
    seed_xyz = evaluate(**new, uv=seed)
    inverse = seed[cKDTree(seed_xyz[:, xy]).query(q[:, xy])[1]].copy()
    for _ in range(12):
        B, U, V = operators(new['ku'], new['kv'], inverse)[:3]
        P = new['P'].reshape(-1, 3)
        err = (B@P)[:, xy]-q[:, xy]
        u, v = (U@P)[:, xy], (V@P)[:, xy]
        det = u[:, 0]*v[:, 1]-u[:, 1]*v[:, 0]
        den = np.where(abs(det) > 1e-12, det, 1e-12)
        delta = np.c_[(err[:, 0]*v[:, 1]-err[:, 1]*v[:, 0])/den,
                       (u[:, 0]*err[:, 1]-u[:, 1]*err[:, 0])/den]
        inverse = np.clip(inverse-np.clip(delta, -.1, .1), 0, 1)
    q1, n1, h1, _, _ = geometry(new, inverse)
    sign = np.sign(np.median(normal[:, axis]))
    valid = ((np.linalg.norm((q1-q)[:, xy], axis=1) < 1e-5) &
             (normal[:, axis]*sign > .15) & (n1[:, axis]*sign > .15))
    valid &= np.all((inverse > .005) & (inverse < .995), axis=1)
    # Avoid finite-difference stencils that cross an excluded fold/trim point.
    mask = valid.reshape(61, 61).copy()
    m = mask.copy(); m[[0, -1], :] = False; m[:, [0, -1]] = False
    m[1:-1, 1:-1] &= (mask[:-2, 1:-1] & mask[2:, 1:-1] & mask[1:-1, :-2] & mask[1:-1, 2:])
    if m.sum() < 1000:
        raise ValueError('Insufficient common physical-position coverage')
    spacing = [np.maximum(np.linalg.norm(np.gradient(q.reshape(61, 61, 3), axis=i), axis=2), 1e-10) for i in (0, 1)]
    def summary(h):
        h = h.reshape(61, 61)
        grad = np.hypot(np.gradient(h, axis=0)/spacing[0], np.gradient(h, axis=1)/spacing[1])
        return dict(mean_curvature_abs_p95_per_m=float(np.percentile(abs(h[m]), 95)),
                    curvature_gradient_median_per_m2=float(np.median(grad[m])),
                    curvature_gradient_p95_per_m2=float(np.percentile(grad[m], 95)))
    return dict(before=summary(h0), after=summary(h1), matched_samples=int(m.sum()),
                total_samples=len(uv), before_opposed_axis_normals=int(np.sum(normal[:, axis]*sign <= 0)),
                after_opposed_axis_normals=int(np.sum(n1[:, axis]*sign <= 0)))


def mesh(net, n=151):
    xyz = evaluate(**net, uv=grid(n))
    ix = np.arange(n*n).reshape(n, n)
    tris = np.stack([ix[:-1, :-1], ix[1:, :-1], ix[1:, 1:],
                     ix[:-1, :-1], ix[1:, 1:], ix[:-1, 1:]], axis=-1).reshape(-1, 3)
    return xyz, tris


def dense_normal_screen(old, new, axis):
    """151x151 + both sides of every knot; includes near-rim residual defects.

    This screens graph projection/orientation, NOT 3D self-intersections.
    Protected complex rims can leave a small opposed-axis strip near the edge.
    """
    def samples(k):
        t = np.unique(k[(k > .001) & (k < .999)])
        return np.unique(np.r_[np.linspace(.001, .999, 151), t-1e-8, t+1e-8])
    u, v = np.meshgrid(samples(new['ku']), samples(new['kv']), indexing='ij')
    uv = np.c_[u.ravel(), v.ravel()]
    def jac(p):
        P, ku, kv = p['P'], p['ku'], p['kv']
        bu, bv = BSpline(ku, np.eye(len(ku)-4), 3), BSpline(kv, np.eye(len(kv)-4), 3)
        du = np.einsum('ni,nj,ijc->nc', bu(uv[:, 0], nu=1), bv(uv[:, 1]), P, optimize=True)
        dv = np.einsum('ni,nj,ijc->nc', bu(uv[:, 0]), bv(uv[:, 1], nu=1), P, optimize=True)
        return np.cross(du, dv)[:, axis]
    j0, j1 = jac(old), jac(new)
    sign = np.sign(np.median(j0))
    return dict(samples=len(uv), before_opposed=int(np.sum(j0*sign < 0)),
                after_opposed=int(np.sum(j1*sign < 0)),
                minimum_signed_projected_jacobian=float(np.min(j1*sign)))


def distance_screen(old, new):
    # Import bpy first: standalone mathutils.BVHTree otherwise may crash.
    import bpy  # noqa: F401
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree
    a, tri = mesh(old); b, _ = mesh(new)
    ta = BVHTree.FromPolygons(a.tolist(), tri.tolist(), all_triangles=True)
    tb = BVHTree.FromPolygons(b.tolist(), tri.tolist(), all_triangles=True)
    def nearest(points, tree):
        return np.array([tree.find_nearest(Vector(p))[3]*1000 for p in points])
    forward, reverse = nearest(b[::2], ta), nearest(a[::2], tb)
    distances = np.r_[forward, reverse]
    return dict(rms_mm=float(np.sqrt(np.mean(distances**2))),
                p95_mm=float(np.percentile(distances, 95)),
                sampled_max_mm=float(max(distances)), samples=len(distances),
                method='bidirectional vertices to 151x151 triangulated patch; sampled, not Hausdorff bound')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--patches', help='comma-separated Shell macro IDs; default broad candidates')
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    requested = set(map(int, args.patches.split(','))) if args.patches else None
    report = dict(method='Exact-boundary Coons graph + physical Hermite fairing', changed=[], unchanged=[])
    for part in PARTS:
        nets = read_nets(ROOT/'CAD'/f'{part}.step')
        for i, old in enumerate(nets):
            reason = ''
            if part != 'Body_Main_Shell' or (requested is not None and i not in requested):
                continue
            pos, normals, _, du, dv = geometry(old, grid(35, .02))
            area = float(np.mean(np.linalg.norm(np.cross(du, dv), axis=1)))
            if area < .035:
                continue
            axis = int(np.argmax(abs(normals).mean(axis=0)))
            if i == 21:
                axis = 2
            if i != 21 and abs(normals[:, axis]).mean() < .8:
                continue
            chosen = None
            for n in [12, 16, 20]:
                try:
                    new = graph_fair(old, axis, n, area_weighted=(i == 21))
                    cm = common_position_curvature(old, new, axis)
                    bef, aft = cm['before'], cm['after']
                    gain = 1-aft['curvature_gradient_p95_per_m2']/max(bef['curvature_gradient_p95_per_m2'], 1e-9)
                    if gain < .30 or aft['curvature_gradient_median_per_m2'] > bef['curvature_gradient_median_per_m2']*1.10:
                        raise ValueError('Less than 30% roughness reduction or worse median')
                    if cm['after_opposed_axis_normals'] > cm['before_opposed_axis_normals']:
                        raise ValueError('New opposed-axis normals')
                    ds = distance_screen(old, new)
                    limit = 10 if i == 21 else 5
                    if ds['sampled_max_mm'] > limit or ds['rms_mm'] > 3:
                        reason = f'sampled shape change exceeds {limit}mm max / 3mm RMS'; continue
                    chosen = new
                    entry = dict(part=part, half_patch=i, axis=axis, coarse_controls=n, area_weighted=(i == 21),
                                 area_screen_m2=area, curvature=cm, shape_change=ds,
                                 roughness_p95_reduction_percent=gain*100,
                                 boundary_poles_max_change_mm=0.0)
                    break
                except ValueError as e:
                    reason = str(e)
                    if 'Coons' in reason or 'narrow' in reason:
                        break
            if chosen is None:
                report['unchanged'].append(dict(part=part, half_patch=i, reason=reason))
                print('KEEP', i, reason, flush=True)
            else:
                np.savez_compressed(args.output/f'{part}_{i:03d}.npz', **chosen)
                report['changed'].append(entry)
                print('FAIR', i, f"{gain*100:.1f}%", ds, flush=True)
    (args.output/'fairing_report.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
