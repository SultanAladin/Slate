#!/usr/bin/env python3
"""Targeted strip removal and jet-controlled fairing, based on the Faired review.

Reconstructs the source rail topology (not the expensive original fit). Removes
only three explicit internal strip interfaces. No exterior rail is refitted.
Rejected experiments are not exported. See Refined/README.md for scope/limits.
"""
import argparse
import copy
import json
import pickle
from pathlib import Path
import numpy as np
from scipy.sparse import diags, eye, hstack, vstack, kron
from scipy.optimize import lsq_linear
from fit_macros import ROOT, Fitter, Layout, edge
from fair_surfaces import PARTS, read_nets, graph_fair, curvature, operators, grid
from export_cad import Writer

GROUPS = [(59, 84), (1, 10), (3, 8)]


def reconstruct(name):
    L = Layout(np.load(ROOT/'Source'/f'{name}_reference.npz')).construct()
    F = Fitter(L)
    nets = read_nets(ROOT/'Faired/CAD'/f'{name}.step')
    if len(nets) != len(L.regs):
        raise ValueError('Source layout no longer matches delivered baseline')
    for i, sides in enumerate(L.sides):
        F.sideinfo.append([F.side(s) for s in sides])
        F.patches.append(dict(**nets[i], id=i, reg=np.array(L.regs[i]), sides=sides, source_ids=[i]))
    return F


def coons(net):
    P, ku, kv = net['P'], net['ku'], net['kv']
    u = np.array([np.mean(ku[i+1:i+4]) for i in range(len(ku)-4)])[:, None, None]
    v = np.array([np.mean(kv[i+1:i+4]) for i in range(len(kv)-4)])[None, :, None]
    Q = ((1-v)*P[:, 0:1]+v*P[:, -1:]+(1-u)*P[0:1]+u*P[-1:] -
         ((1-u)*(1-v)*P[0, 0]+(1-u)*v*P[0, -1]+u*(1-v)*P[-1, 0]+u*v*P[-1, -1]))
    Q[[0, -1]] = P[[0, -1]]; Q[:, [0, -1]] = P[:, [0, -1]]
    return dict(P=Q, ku=ku, kv=kv)


def bounded_fair(net, length=.02, limit=.002, jet_weight=2.):
    """Physical mean-curvature penalty with box-bounded poles and rim jet control.

    All rim poles fixed. Penalize changes to the original rim tangent PLANES,
    rather than using uncontrolled per-face normal smoothing. Full convergence,
    curvature and orientation/seam screening are required separately.
    """
    P, ku, kv = net['P'], net['ku'], net['kv']
    nu, nv = P.shape[:2]
    B, U, V, UU, UV, VV = operators(ku, kv, grid(43, .005))
    xyz = P.reshape(-1, 3); u, v = U@xyz, V@xyz
    normal = np.cross(u, v); area = np.linalg.norm(normal, axis=1)
    normal /= np.maximum(area[:, None], 1e-12)
    E, F, G = (u*u).sum(1), (u*v).sum(1), (v*v).sum(1)
    determinant = np.maximum(E*G-F*F, np.median(E*G-F*F)*1e-4)
    D = diags(G/determinant)@UU-diags(2*F/determinant)@UV+diags(E/determinant)@VV
    free = np.ones((nu, nv), bool); free[[0, -1]] = False; free[:, [0, -1]] = False
    ids = np.flatnonzero(free)
    W = diags(np.sqrt(area/max(np.mean(area), 1e-12)))
    A = hstack([diags(normal[:, c])@D[:, ids] for c in range(3)]).tocsr()
    current = np.sum(normal*(D@xyz), axis=1)
    position = kron(eye(3), W@B[:, ids])
    t = np.linspace(.002, .998, 65)
    rim = np.r_[np.c_[t, t*0], np.c_[t*0+1, t], np.c_[t, t*0+1], np.c_[t*0, t]]
    _, DU, DV = operators(ku, kv, rim)[:3]
    n = np.cross(DU@xyz, DV@xyz); n /= np.maximum(np.linalg.norm(n, axis=1)[:, None], 1e-12)
    cross_derivative = vstack([DV[:65], DU[65:130], DV[130:195], DU[195:]]).tocsr()
    speed = np.maximum(np.linalg.norm(cross_derivative@xyz, axis=1), .001)
    jets = hstack([diags(n[:, c]/speed)@cross_derivative[:, ids] for c in range(3)]).tocsr()*jet_weight
    ridge = eye(len(ids)*3)*.03
    A = vstack([position, length**2*W@A, ridge, jets]).toarray()
    rhs = np.r_[np.zeros(position.shape[0]), -length**2*(W@current),
                np.zeros(len(ids)*3+jets.shape[0])]
    result = lsq_linear(A, rhs, bounds=(-limit, limit), lsq_solver='exact', tol=1e-9, max_iter=100)
    if result.status <= 0:
        raise ValueError('Fairing solve did not converge')
    delta = result.x.reshape(3, -1).T
    Q = P.copy(); Q.reshape(-1, 3)[ids] += delta
    return dict(P=Q, ku=ku, kv=kv), dict(solver_status=int(result.status),
        optimality=float(result.optimality), max_pole_change_mm=float(np.linalg.norm(delta, axis=1).max()*1000),
        position_component_bound_mm=limit*1000, curvature_length_mm=length*1000, rim_jet_weight=jet_weight)


def serialize(F):
    rails = sorted({key for sides in F.sideinfo for _, _, refs, _ in sides for key, _ in refs})
    indices = {k: i for i, k in enumerate(rails)}
    def rail(k):
        P = F.rails[k]['P'].copy()
        # Match Writer.point's symmetry-plane canonicalisation exactly.
        P[np.abs(P[:, 1]) < 1e-7, 1] = 0.
        P[np.abs(P) < 1e-14] = 0.
        return dict(key=list(k), P=P.tolist(), K=F.rails[k]['K'].tolist())
    return dict(rails=[rail(k) for k in rails],
        patches=[dict(id=p['id'], source_ids=p.get('source_ids', [p['id']]),
                      sides=[[dict(rail=indices[k], reverse=bool(rev), start=float(a), end=float(b))
                              for (k, rev), a, b in zip(refs, stops[:-1], stops[1:])]
                             for _, _, refs, stops in ss]) for p, ss in zip(F.patches, F.sideinfo)])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--output', type=Path, default=ROOT/'Refined')
    ap.add_argument('--scratch', type=Path, default=Path('/tmp/liger-refined'))
    args = ap.parse_args(); args.scratch.mkdir(parents=True, exist_ok=True)
    out = args.output; (out/'CAD').mkdir(parents=True, exist_ok=True)
    report = dict(baseline='Faired / 5ba6188', merged=[], faired=[], retained_complex_regions=[34, 46],
                  frame_translation_mm=[0., 0., -1.],
                  note='Main-shell exterior rail curves retained. Three selected internal crease/strip rails removed. Display tessellation is a separate repair.')
    full = Writer(); shape, context = full.context('Liger_Refined_CAD'); models = []; steps = []; stats = []
    layout = dict(before={}, after={})
    for part, name in enumerate(PARTS):
        original = reconstruct(name); F = copy.deepcopy(original)
        layout['before'][name] = serialize(original)
        if part == 0:
            replacements = {}; remove = set()
            for group in GROUPS:
                reg = sum([original.L.regs[i] for i in group], [])
                internal = {e for e, fs in original.L.ef.items() if len(fs) == 2 and set(fs) <= set(reg)}
                sides = original.L.boundary(reg, original.L.cuts-internal)
                if sides is None:
                    raise ValueError(f'Merge {group} is not a simple four-corner disk')
                L = copy.copy(original.L); L.regs = [reg]; L.sides = [sides]
                merged = Fitter(L); merged.corners = original.corners.copy(); merged.rails = copy.deepcopy(original.rails)
                merged.fit()
                p = merged.patches[0]
                initial = {k: p[k] for k in ['P', 'ku', 'kv']}
                q = coons(initial) if group[0] == 3 else graph_fair(initial, 2 if group[0] == 59 else 1, 8, area_weighted=True)
                p.update(q); p['id'] = group[0]; p['source_ids'] = list(group)
                if not set(merged.rails) <= set(original.rails):
                    raise ValueError('Unexpected new perimeter rail')
                replacements[group[0]] = (p, merged.sideinfo[0]); remove.update(group[1:])
                report['merged'].append(dict(source_ids=list(group), result_id=group[0],
                    method='3D Coons fairing' if group[0] == 3 else 'Area-weighted 8x8 physical graph + Hermite projection',
                    new_poles=list(q['P'].shape[:2]), diagnostic_curvature=curvature(**q)))
            F.patches = []; F.sideinfo = []
            for p, ss in zip(original.patches, original.sideinfo):
                if p['id'] in remove:
                    continue
                p, ss = replacements.get(p['id'], (copy.deepcopy(p), ss))
                if p['id'] in [27, 42]:
                    old = {k: p[k] for k in ['P', 'ku', 'kv']}
                    q, solve = bounded_fair(old)
                    p.update(q)
                    report['faired'].append(dict(id=p['id'], method='Bounded physical curvature fairing with rim jet penalty',
                        before=curvature(**old), after=curvature(**q), **solve))
                F.patches.append(p); F.sideinfo.append(ss)
            used = {key for ss in F.sideinfo for _, _, refs, _ in ss for key, _ in refs}
            F.rails = {k: original.rails[k] for k in used}
        if name == 'Body_Roof_Glass_Frame':
            # Assembly clearance: preserve the insert shape exactly, but stop it
            # protruding through the canopy skin. Move rails AND carriers.
            for p in F.patches:
                p['P'][:, :, 2] -= .001
            for rail in F.rails.values():
                rail['P'][:, 2] -= .001
        layout['after'][name] = serialize(F)
        with (args.scratch/f'{name}_before.pkl').open('wb') as s:
            pickle.dump(original, s)
        with (args.scratch/f'{name}_after.pkl').open('wb') as s:
            pickle.dump(F, s)
        w = Writer(); sh, ctx = w.context(name); model, st = w.part(name, F, fit_statistics=False)
        text = w.finish(name, sh, ctx, [model]); (out/'CAD'/f'{name}.step').write_text(text)
        model, _ = full.part(name, F, fit_statistics=False); models.append(model); stats.append(st)
        steps.append(f'[[Steps]]\nKind = 18\nLabel = "LigerPart{part+1}"\nCaption = "{name}"\nExchangeUnit = 1000.0\nExchange = """\n{text}"""\n')
        print(name, st, flush=True)
    (out/'CAD/Liger_Refined_CAD.step').write_text(full.finish('Liger_Refined_CAD', shape, context, models))
    (out/'CAD/Liger_Refined_CAD.solidarc').write_text('Format = "SolidArc"\nFormatVersion = 1\nTitle = "Liger — targeted strip/arch refinement (review)"\nDescription = "Three open sheets; selected strips merged, rims retained. See report for remaining defects."\n\n'+'\n'.join(steps))
    (out/'CAD/metrics.json').write_text(json.dumps(dict(parts=stats), indent=2)+'\n')
    (out/'CAD/patch_layout.json').write_text(json.dumps(layout, separators=(',', ':'))+'\n')
    (out/'refinement_report.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
