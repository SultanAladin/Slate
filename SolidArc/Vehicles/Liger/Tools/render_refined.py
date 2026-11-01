#!/usr/bin/env python3
"""Matched-camera proofs using conforming tessellation for BOTH CAD revisions."""
import argparse
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from fit_macros import ROOT
from render_proofs import raster, lines_overlay
from tessellate_shared import load_binary


def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--revision', type=Path, default=ROOT/'Refined')
    args = ap.parse_args(); out = args.revision/'Proofs'
    before, after = [load_binary(out/f'{w}.bin.gz') for w in ['before', 'after']]
    font = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    big = ImageFont.truetype(font, 29); small = ImageFont.truetype(font, 17); tiny = ImageFont.truetype(font, 14)
    specs = [('01_Before_Shaded', before, 0, 'full'), ('02_After_Shaded', after, 0, 'full'),
             ('03_Before_Zebra', before, 3, 'full'), ('04_After_Zebra', after, 3, 'full'),
             ('05_Upper_Before', before, 3, 'upper'), ('06_Upper_After', after, 3, 'upper'),
             ('07_Sill_Before', before, 0, 'sill'), ('08_Sill_After', after, 0, 'sill'),
             ('09_Arch_Before', before, 3, 'arch'), ('10_Arch_After', after, 3, 'arch'),
             ('11_Change_Map', after, 4, 'full'), ('12_Patch_Layout', after, 1, 'full'),
             ('14_Canopy_Before', before, 0, 'canopy'), ('15_Canopy_After', after, 0, 'canopy')]
    def region(pos, key):
        if key == 'canopy': return (pos[:, 0] > .65) & (pos[:, 0] < 1.35) & (pos[:, 2] > 1.0) & (abs(pos[:, 1]) < .7)
        if key == 'upper': return (pos[:, 0] < .23) & (pos[:, 2] > .83)
        if key == 'sill': return (pos[:, 0] > -.75) & (pos[:, 0] < 1.45) & (pos[:, 1] < -.99) & (pos[:, 2] < .79)
        if key == 'arch': return (pos[:, 0] > 1.95) & (pos[:, 1] < -.63) & (pos[:, 2] < .85)
        return np.ones(len(pos), bool)
    W, H = 1600, 960
    for name, D, mode, key in specs:
        # Focus the camera, never remove triangles at an arbitrary ROI boundary.
        tri = D['tri']
        framing = before['pos'][region(before['pos'], key)]
        center = (framing.min(0)+framing.max(0))/2
        eye = [7, -9, 5] if key == 'full' else [6, -9, 7] if key == 'canopy' else [-4, -9, 7] if key == 'upper' else [1, -10, 2] if key == 'sill' else [5, -10, 4]
        forward = np.array(eye, float); forward /= np.linalg.norm(forward)
        right = np.cross([0, 0, 1], forward); right /= np.linalg.norm(right)
        up = np.cross(forward, right); R = np.stack([right, up, forward], axis=1)
        p = (framing-center)@R
        scale = min((W-160)/np.ptp(p[:, 0]), (H-240)/np.ptp(p[:, 1]))
        def project(q):
            p = (q-center)@R; p[..., 0] = p[..., 0]*scale+W/2; p[..., 1] = H/2+20-p[..., 1]*scale
            return p
        error = D['change_mm']/2.5 if name == '11_Change_Map' else D['error_mm']
        image, depth = raster(project(D['pos']), D['nor']@R, tri, D['face'], D['part'], error, mode, W, H)
        if mode == 1:
            image = lines_overlay(image, depth, project(D['lines']))
        # A rectangular viewport crop protects labels without creating false
        # saw-toothed 'sheet edges' from per-triangle world-region filtering.
        image[:120] = [15, 22, 33]; image[875:] = [15, 22, 33]
        image[:, :32] = [15, 22, 33]; image[:, -32:] = [15, 22, 33]
        im = Image.fromarray(image); draw = ImageDraw.Draw(im)
        if key != 'full': draw.rectangle((32, 120, W-33, 874), outline='#233146')
        draw.text((48, 26), 'LIGER / '+name[3:].replace('_', ' ').upper(), font=big, fill='#edf4fa')
        draw.text((49, 75), '306 faces after / 312 before · shared-edge tessellation on BOTH · real NURBS derivatives', font=small, fill='#9fb1c7')
        draw.line((48, 111, 1552, 111), fill='#2b3d51')
        draw.text((48, 888), 'COMPUTED GEOMETRY / TARGETED STRIP + ARCH REFINEMENT / 01 NOV 2026', font=small, fill='#b6cadc')
        note = 'Matched camera and shading. Remaining return/crease irregularities are not Class-A or G1/G2 certified.'
        if key != 'full': note = 'Camera crop, no geometry cut. ' + note
        if mode == 4: note = 'Blue 0 mm → green 2.5 mm → red ≥5 mm. Sample to nearest original CAD region; not same-UV movement.'
        if mode == 1: note = 'Selected internal strip interfaces removed; openings and exterior rail curves retained.'
        draw.text((48, 921), note, font=tiny, fill='#8298af')
        im.save(out/f'{name}.png'); print(name, flush=True)


if __name__ == '__main__':
    main()
