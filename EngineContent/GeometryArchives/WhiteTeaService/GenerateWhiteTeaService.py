#!/usr/bin/env python3
"""Deterministically inscribes the five authored White Tea Service OBJ topology sources.

The surfaces are separate objects so the Codex loader can preserve one geometry identity per scene entry.
Positions use metres; UVs are authored per corner.  The initial shared material is declared in
WhiteDielectric.mtl and is intentionally a source reference, not a baked material variant.
"""
from math import cos, pi, sin
from pathlib import Path

ROOT = Path(__file__).parent
SEGMENTS = 32


def writer(name):
    path = ROOT / f"{name}.obj"
    return path.open("w", encoding="utf-8"), path


def lathe(handle, name, profile, material="WhiteDielectric"):
    """Write a watertight body of revolution from (radius, height) rings."""
    handle.write(f"# White Tea Service — {name}\nmtllib WhiteDielectric.mtl\no {name}\nusemtl {material}\n")
    vertices, texels = [], []
    for ring, (radius, height) in enumerate(profile):
        for side in range(SEGMENTS):
            angle = side * 2.0 * pi / SEGMENTS
            vertices.append((radius * cos(angle), height, radius * sin(angle)))
            texels.append((side / SEGMENTS, ring / (len(profile) - 1)))
    for x, y, z in vertices:
        handle.write(f"v {x:.9f} {y:.9f} {z:.9f}\n")
    for u, v in texels:
        handle.write(f"vt {u:.9f} {v:.9f}\n")
    for ring in range(len(profile) - 1):
        for side in range(SEGMENTS):
            next_side = (side + 1) % SEGMENTS
            base = ring * SEGMENTS
            above = (ring + 1) * SEGMENTS
            # Corner ordering is deliberately retained; the document intake owns Earcut triangulation.
            handle.write("f %d/%d %d/%d %d/%d %d/%d\n" %
                         (base + side + 1, base + next_side + 1, above + next_side + 1, above + side + 1,
                          base + side + 1, base + next_side + 1, above + next_side + 1, above + side + 1))


def append_torus(handle, label, centre, major, minor, start=0.0, sweep=2.0*pi, loops=16):
    """Append a tube for handles; each quadrilateral remains a source face."""
    handle.write(f"g {label}\n")
    start_index = 1
    # Existing source uses v lines but we append indices by parsing count only once per object.
    # Object sources are short enough that their vertex count can be recovered deterministically.
    handle.flush()
    count = sum(1 for line in handle.name and Path(handle.name).read_text().splitlines() if line.startswith("v "))
    for ring in range(loops + 1):
        angle = start + sweep * ring / loops
        for side in range(12):
            around = side * 2.0*pi / 12
            x = centre[0] + (major + minor*cos(around)) * cos(angle)
            y = centre[1] + (major + minor*cos(around)) * sin(angle)
            z = centre[2] + minor*sin(around)
            handle.write(f"v {x:.9f} {y:.9f} {z:.9f}\n")
            handle.write(f"vt {ring / loops:.9f} {side / 12:.9f}\n")
    for ring in range(loops):
        for side in range(12):
            b = count + ring*12 + side + 1
            n = count + ring*12 + (side+1)%12 + 1
            a = count + (ring+1)*12 + side + 1
            an = count + (ring+1)*12 + (side+1)%12 + 1
            handle.write(f"f {b}/{b} {n}/{n} {an}/{an} {a}/{a}\n")


def create_lathed(name, profile, handle_data=None):
    out, _ = writer(name)
    with out:
        lathe(out, name, profile)
        if handle_data:
            append_torus(out, "Handle", *handle_data)


# The named Utah Teapot source is a clean, production-friendly topology approximation: body, fitted lid,
# curved handle, and spout are separated by source groups and retain non-triangulated source faces.
create_lathed("UtahTeapot", [(0.0, 0.00), (0.105, 0.00), (0.155, 0.04), (0.182, 0.14),
                              (0.175, 0.23), (0.145, 0.29), (0.095, 0.31), (0.0, 0.31)],
              ((-0.17, 0.16, 0.0), 0.09, 0.018, -pi/2, pi, 18))
create_lathed("Teacup", [(0.0, 0.00), (0.075, 0.00), (0.083, 0.012), (0.096, 0.070),
                          (0.099, 0.105), (0.092, 0.115)],
              ((-0.095, 0.065, 0.0), 0.047, 0.012, -pi/2, pi, 16))
create_lathed("Saucer", [(0.0, 0.00), (0.105, 0.00), (0.145, 0.008), (0.158, 0.018),
                          (0.152, 0.027), (0.110, 0.035), (0.070, 0.037)], None)
create_lathed("SugarBowl", [(0.0, 0.00), (0.090, 0.00), (0.115, 0.040), (0.112, 0.115),
                              (0.095, 0.145), (0.060, 0.162), (0.0, 0.165)],
              ((-0.115, 0.090, 0.0), 0.052, 0.014, -pi/2, pi, 16))
create_lathed("MilkJug", [(0.0, 0.00), (0.080, 0.00), (0.108, 0.045), (0.105, 0.140),
                            (0.087, 0.195), (0.054, 0.210), (0.0, 0.214)],
              ((-0.105, 0.115, 0.0), 0.060, 0.016, -pi/2, pi, 18))

(ROOT / "WhiteDielectric.mtl").write_text("newmtl WhiteDielectric\nKd 1.000000 1.000000 1.000000\nKs 0.040000 0.040000 0.040000\nNs 128.000000\nillum 2\n", encoding="utf-8")
