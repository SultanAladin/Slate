#!/usr/bin/env python3
"""Matched before/after rasters of actual evaluated cubic nets, no generated art."""
import argparse
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from fit_macros import ROOT
from render_proofs import raster


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--revision', type=Path, default=ROOT/'Faired')
    args = ap.parse_args()
    before = np.load(ROOT/'Proofs/inspection_geometry.npz')
    after = np.load(args.revision/'Proofs/inspection_geometry.npz')
    font = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    big = ImageFont.truetype(font, 29); small = ImageFont.truetype(font, 17)
    tiny = ImageFont.truetype(font, 14)
    specs = [
        ('01_Faired_Shaded', 'AFTER / FAIRED NURBS', after, 0, False),
        ('02_Bonnet_Before_Zebra', 'BONNET / BEFORE / ZEBRA', before, 3, True),
        ('03_Bonnet_After_Zebra', 'BONNET / AFTER / ZEBRA', after, 3, True),
        ('04_Before_Zebra', 'BEFORE / ZEBRA', before, 3, False),
        ('05_After_Zebra', 'AFTER / ZEBRA', after, 3, False),
        ('06_Shape_Change', 'SHAPE CHANGE / ORIGINAL CAD TO FAIRED', after, 2, False),
        ('07_Source_Deviation', 'DISTANCE TO POLYGON SOURCE / AFTER', after, 2, False),
        ('08_Bonnet_Before_Shaded', 'BONNET / BEFORE / SHADED', before, 0, True),
        ('09_Bonnet_After_Shaded', 'BONNET / AFTER / SHADED', after, 0, True),
    ]
    W, H = 1600, 960
    for name, title, D, mode, close in specs:
        pos, nor, tri = D['pos'], D['nor'], D['tri']
        base = before['pos']
        if close:
            # Identical clipping, camera, center, scale and lighting for both.
            visible = (base[:, 0] > .07) & (base[:, 2] > .72)
            tri = tri[np.all(visible[tri], axis=1)]
            framing = base[visible]
            eye = (6, -9, 14)
        else:
            framing = base; eye = (7, -9, 5)
        center = (framing.min(0)+framing.max(0))/2
        forward = np.array(eye, dtype=float); forward /= np.linalg.norm(forward)
        right = np.cross([0, 0, 1], forward); right /= np.linalg.norm(right)
        up = np.cross(forward, right); R = np.stack([right, up, forward], axis=1)
        p = (framing-center)@R
        scale = min((W-160)/np.ptp(p[:, 0]), (H-240)/np.ptp(p[:, 1]))
        def project(q):
            p = (q-center)@R
            p[..., 0] = p[..., 0]*scale+W/2
            p[..., 1] = H/2+48-p[..., 1]*scale
            return p
        error = D['change_mm']/4 if name == '06_Shape_Change' else D['error_mm']
        image, _ = raster(project(pos), nor@R, tri, D['face'], D['part'], error, 4 if mode == 2 else mode, W, H)
        im = Image.fromarray(image); draw = ImageDraw.Draw(im)
        draw.text((48, 26), 'LIGER / '+title, font=big, fill='#edf4fa')
        draw.text((49, 75), '312 cubic NURBS faces · fixed patch rims · original design creases protected', font=small, fill='#9fb1c7')
        draw.line((48, 111, 1552, 111), fill='#2b3d51')
        note = 'Same camera / lighting / zebra function. Remaining seam irregularities are not G1/G2 certified.'
        if name == '06_Shape_Change':
            note = 'Blue 0 mm → green 4 mm → red ≥8 mm. New sample → nearest original CAD patch triangle; not UV displacement.'
        elif mode == 2:
            note = 'Blue 0 mm → green 1 mm → red ≥2 mm; magenta >5 mm. Source distance is NOT a smoothness score.'
        elif mode == 0:
            note = 'Shaded view directly from edited NURBS positions and derivatives; no cosmetic normal smoothing.'
        draw.text((48, 888), 'COMPUTED NURBS PROOF / 01 NOV 2026 / FAIRED REVIEW', font=small, fill='#b6cadc')
        draw.text((48, 921), note, font=tiny, fill='#8298af')
        im.save(args.revision/'Proofs'/f'{name}.png')
        print(name, flush=True)


if __name__ == '__main__':
    main()
