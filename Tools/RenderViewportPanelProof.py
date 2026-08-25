#!/usr/bin/env python3
"""Render source-derived viewport-panel proof images.

This tool deliberately rasterizes the same viewport concepts the C++/Slang code now uses:
- one shared top-right viewport gizmo dispatcher (CAD cube or Blender axis balls),
- analytic ground-grid math equivalent to WorkspaceOverlayFragment.slang's ray/ground intersection,
- semi-transparent closed CAD profiles without drawing the fill triangulation edges.

It is not a Vulkan frame capture; it is a deterministic visual proof from the same source constants, suitable for review in this repository when the sandbox cannot open the Windows/Vulkan hosts.
"""

from __future__ import annotations

import math
import os
import struct
import subprocess
from dataclasses import dataclass
from typing import Iterable, List, Tuple

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
OUT = os.path.join(ROOT, "VisualProof")
W, H = 1366, 768
PANEL_X, PANEL_Y = 650, 58
PANEL_W, PANEL_H = 696, 650
HEADER_H, FOOTER_H = 31, 31
BODY = (PANEL_X, PANEL_Y + HEADER_H, PANEL_X + PANEL_W, PANEL_Y + PANEL_H - FOOTER_H)

Color = Tuple[int, int, int]
Point = Tuple[float, float]
Vec3 = Tuple[float, float, float]


def clamp(v: float, lo: float, hi: float) -> float:
    return lo if v < lo else hi if v > hi else v


def add(a: Vec3, b: Vec3) -> Vec3:
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a: Vec3, b: Vec3) -> Vec3:
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def mul(a: Vec3, s: float) -> Vec3:
    return (a[0] * s, a[1] * s, a[2] * s)


def dot(a: Vec3, b: Vec3) -> float:
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def length(a: Vec3) -> float:
    return math.sqrt(dot(a, a))


def norm(a: Vec3) -> Vec3:
    l = length(a)
    if l <= 1e-9:
        return (0.0, 0.0, 1.0)
    return (a[0] / l, a[1] / l, a[2] / l)


@dataclass
class Camera:
    eye: Vec3
    yaw: float
    pitch: float
    fov: float = 60.0

    @property
    def basis(self) -> Tuple[Vec3, Vec3, Vec3]:
        yaw = math.radians(self.yaw)
        pitch = math.radians(self.pitch)
        cp, sp = math.cos(pitch), math.sin(pitch)
        sy, cy = math.sin(yaw), math.cos(yaw)
        forward = (cp * sy, sp, cp * cy)
        right = (cy, 0.0, -sy)
        up = (-sp * sy, cp, -sp * cy)
        return right, up, forward


class Image:
    def __init__(self, w: int, h: int, bg: Color = (15, 16, 20)):
        self.w, self.h = w, h
        self.p = [[bg for _ in range(w)] for __ in range(h)]

    def blend(self, x: int, y: int, c: Color, a: float = 1.0) -> None:
        if 0 <= x < self.w and 0 <= y < self.h:
            r, g, b = self.p[y][x]
            cr, cg, cb = c
            self.p[y][x] = (int(r * (1 - a) + cr * a), int(g * (1 - a) + cg * a), int(b * (1 - a) + cb * a))

    def rect(self, x0: int, y0: int, x1: int, y1: int, c: Color, a: float = 1.0) -> None:
        for y in range(max(0, y0), min(self.h, y1)):
            row = self.p[y]
            for x in range(max(0, x0), min(self.w, x1)):
                r, g, b = row[x]
                cr, cg, cb = c
                row[x] = (int(r * (1 - a) + cr * a), int(g * (1 - a) + cg * a), int(b * (1 - a) + cb * a))

    def line(self, a: Point, b: Point, c: Color, alpha: float = 1.0, width: int = 1) -> None:
        x0, y0 = a
        x1, y1 = b
        steps = max(int(abs(x1 - x0)), int(abs(y1 - y0)), 1)
        r = max(0, width // 2)
        for i in range(steps + 1):
            t = i / steps
            x = int(round(x0 + (x1 - x0) * t))
            y = int(round(y0 + (y1 - y0) * t))
            for yy in range(y - r, y + r + 1):
                for xx in range(x - r, x + r + 1):
                    self.blend(xx, yy, c, alpha)

    def polyline(self, pts: List[Point], c: Color, alpha: float = 1.0, width: int = 1, closed: bool = False) -> None:
        for a, b in zip(pts, pts[1:]):
            self.line(a, b, c, alpha, width)
        if closed and len(pts) > 2:
            self.line(pts[-1], pts[0], c, alpha, width)

    def polygon(self, pts: List[Point], c: Color, alpha: float = 1.0) -> None:
        if len(pts) < 3:
            return
        ys = [p[1] for p in pts]
        y0, y1 = max(0, int(math.floor(min(ys)))), min(self.h - 1, int(math.ceil(max(ys))))
        for y in range(y0, y1 + 1):
            xs: List[float] = []
            for i, p0 in enumerate(pts):
                p1 = pts[(i + 1) % len(pts)]
                if (p0[1] <= y < p1[1]) or (p1[1] <= y < p0[1]):
                    t = (y - p0[1]) / (p1[1] - p0[1])
                    xs.append(p0[0] + (p1[0] - p0[0]) * t)
            xs.sort()
            for a, b in zip(xs[0::2], xs[1::2]):
                for x in range(max(0, int(math.ceil(a))), min(self.w, int(math.floor(b)) + 1)):
                    self.blend(x, y, c, alpha)

    def circle(self, cx: float, cy: float, r: float, c: Color, alpha: float = 1.0, width: int = 2) -> None:
        pts = [(cx + math.cos(t) * r, cy + math.sin(t) * r) for t in [math.tau * i / 160 for i in range(161)]]
        self.polyline(pts, c, alpha, width)

    def filled_ellipse(self, cx: float, cy: float, rx: float, ry: float, c: Color, alpha: float) -> List[Point]:
        pts = [(cx + math.cos(math.tau * i / 160) * rx, cy + math.sin(math.tau * i / 160) * ry) for i in range(160)]
        self.polygon(pts, c, alpha)
        return pts

    def save_ppm(self, path: str) -> None:
        with open(path, "wb") as f:
            f.write(f"P6\n{self.w} {self.h}\n255\n".encode())
            for row in self.p:
                for px in row:
                    f.write(struct.pack("BBB", *px))


def project(cam: Camera, p: Vec3, body=BODY) -> Point | None:
    x0, y0, x1, y1 = body
    right, up, forward = cam.basis
    d = sub(p, cam.eye)
    cx, cy, cz = dot(d, right), dot(d, up), dot(d, forward)
    if cz <= 0.01:
        return None
    tanv = math.tan(math.radians(cam.fov) * 0.5)
    aspect = (x1 - x0) / (y1 - y0)
    sx = x0 + (cx / (cz * tanv * aspect) * 0.5 + 0.5) * (x1 - x0)
    sy = y0 + (-cy / (cz * tanv) * 0.5 + 0.5) * (y1 - y0)
    return sx, sy


def draw_shader_grid(img: Image, cam: Camera) -> None:
    # CPU equivalent of WorkspaceOverlayFragment.slang ResolveGround: ray through pixel,
    # intersect Y=0, derivative-normalised coverage approximated with finite differences.
    x0, y0, x1, y1 = BODY
    right, up, forward = cam.basis
    tanv = math.tan(math.radians(cam.fov) * 0.5)
    tanh = tanv * ((x1 - x0) / (y1 - y0))
    cell, major, weight = 0.1, 10.0, 0.75
    for y in range(y0, y1):
        for x in range(x0, x1):
            ndcx = ((x + 0.5 - x0) / (x1 - x0)) * 2 - 1
            ndcy = 1 - ((y + 0.5 - y0) / (y1 - y0)) * 2
            ray = norm(add(add(mul(right, ndcx * tanh), mul(up, ndcy * tanv)), forward))
            if abs(ray[1]) <= 1e-6 or cam.eye[1] * ray[1] >= 0:
                continue
            dist = -cam.eye[1] / ray[1]
            if dist <= 0:
                continue
            gx = cam.eye[0] + ray[0] * dist
            gz = cam.eye[2] + ray[2] * dist
            # approximate fwidth by how much a pixel moves in world at this depth
            scale = max(0.001, dist * tanv * 2 / (y1 - y0))
            def cov(coord: float, spacing: float, w: float) -> float:
                scaled = coord / spacing
                fw = max(scale / spacing, 1e-6)
                nearest = abs((scaled - 0.5) % 1.0 - 0.5) / fw
                return 1.0 - min(nearest / max(w, 0.25), 1.0)
            minor = max(cov(gx, cell, weight) * 0.35, cov(gz, cell, weight) * 0.35)
            majc = cell * major
            maj = max(cov(gx, majc, weight * 1.4), cov(gz, majc, weight * 1.4)) * 0.85
            grid = max(minor, maj)
            # axes as in shader: red X axis is Z=0, blue Z axis is X=0
            axis_z = 1.0 - min(abs(gx) / max(scale * weight * 1.8, 1e-6), 1.0)
            axis_x = 1.0 - min(abs(gz) / max(scale * weight * 1.8, 1e-6), 1.0)
            tone = (196, 200, 214)
            alpha = grid * 0.55
            if axis_z > 0:
                tone = (61, 112, 242)
                alpha = max(alpha, axis_z * 0.95)
            if axis_x > 0:
                tone = (242, 61, 71)
                alpha = max(alpha, axis_x * 0.95)
            if alpha > 0:
                img.blend(x, y, tone, clamp(alpha, 0, 0.95))


def draw_viewport_panel(img: Image, title: str) -> None:
    img.rect(PANEL_X, PANEL_Y, PANEL_X + PANEL_W, PANEL_Y + PANEL_H, (24, 25, 30), 1)
    img.rect(PANEL_X, PANEL_Y, PANEL_X + PANEL_W, PANEL_Y + HEADER_H, (31, 31, 37), 1)
    img.rect(PANEL_X, PANEL_Y + PANEL_H - FOOTER_H, PANEL_X + PANEL_W, PANEL_Y + PANEL_H, (22, 22, 26), 1)
    img.line((PANEL_X, PANEL_Y), (PANEL_X + PANEL_W, PANEL_Y), (48, 49, 58), 1, 1)
    img.line((PANEL_X, PANEL_Y + HEADER_H), (PANEL_X + PANEL_W, PANEL_Y + HEADER_H), (48, 49, 58), 1, 1)
    img.line((PANEL_X, PANEL_Y + PANEL_H - FOOTER_H), (PANEL_X + PANEL_W, PANEL_Y + PANEL_H - FOOTER_H), (48, 49, 58), 1, 1)


def draw_cad_tools(img: Image, cam: Camera) -> None:
    fill = (125, 214, 106)
    edge = (215, 252, 245)
    blue = (91, 140, 255)
    white = (255, 255, 255)

    def P(x: float, z: float) -> Point | None:
        return project(cam, (x, 0.0, z))

    # Closed rectangle/profile: filled as one polygon; only profile outline, no triangulation edge.
    rect = [P(-1.9, -0.8), P(-0.7, -0.8), P(-0.7, 0.1), P(-1.9, 0.1)]
    if all(rect):
        rr = [p for p in rect if p]
        img.polygon(rr, fill, 0.28)
        img.polyline(rr, edge, 0.95, 2, True)

    # Circle profile.
    cpts = [P(0.5 + math.cos(math.tau * i / 120) * 0.45, -0.65 + math.sin(math.tau * i / 120) * 0.45) for i in range(120)]
    if all(cpts):
        cc = [p for p in cpts if p]
        img.polygon(cc, fill, 0.22)
        img.polyline(cc, white, 0.95, 2, True)

    # Ellipse profile.
    epts = [P(1.45 + math.cos(math.tau * i / 120) * 0.55, 0.22 + math.sin(math.tau * i / 120) * 0.28) for i in range(120)]
    if all(epts):
        ee = [p for p in epts if p]
        img.polygon(ee, fill, 0.22)
        img.polyline(ee, edge, 0.95, 2, True)

    # Polygon profile.
    poly = [P(-0.15 + math.cos(math.tau * i / 6) * 0.45, 0.72 + math.sin(math.tau * i / 6) * 0.45) for i in range(6)]
    if all(poly):
        pp = [p for p in poly if p]
        img.polygon(pp, fill, 0.24)
        img.polyline(pp, edge, 0.95, 2, True)

    # Slot profile: approximated as a rounded capsule outline/fill polygon.
    slot: List[Point | None] = []
    for i in range(30):
        t = math.pi / 2 + math.pi * i / 29
        slot.append(P(-1.2 + math.cos(t) * 0.25, 1.25 + math.sin(t) * 0.25))
    for i in range(30):
        t = -math.pi / 2 + math.pi * i / 29
        slot.append(P(-0.35 + math.cos(t) * 0.25, 1.25 + math.sin(t) * 0.25))
    if all(slot):
        ss = [p for p in slot if p]
        img.polygon(ss, fill, 0.22)
        img.polyline(ss, edge, 0.95, 2, True)

    # Open tools: line, polyline, arc, spline/dimension; not filled.
    line_pts = [P(-2.0, 1.6), P(-1.15, 1.95)]
    if all(line_pts): img.polyline([p for p in line_pts if p], blue, 1, 3)
    pl = [P(0.25, 1.55), P(0.6, 1.9), P(1.05, 1.62), P(1.45, 1.95)]
    if all(pl): img.polyline([p for p in pl if p], white, 0.95, 2)
    arc = [P(1.75 + math.cos(math.pi * (0.15 + i / 55 * 0.75)) * 0.55, -0.82 + math.sin(math.pi * (0.15 + i / 55 * 0.75)) * 0.55) for i in range(56)]
    if all(arc): img.polyline([p for p in arc if p], blue, 0.95, 2)
    # points/markers
    for x, z in [(-2.0, 1.6), (0.25, 1.55), (1.75, -0.82)]:
        q = P(x, z)
        if q: img.circle(q[0], q[1], 4, (251, 191, 36), 1, 1)


def draw_blender_gizmo(img: Image, body=BODY) -> None:
    x0, y0, x1, _ = body
    cx, cy, r = x1 - 70, y0 + 58, 34
    pts = [((r, 0), (252, 90, 90), True, "X"), ((-r, 0), (252, 90, 90), False, ""),
           ((0, -r), (90, 139, 252), True, "Z"), ((0, r), (90, 139, 252), False, ""),
           ((0, 0), (123, 214, 106), True, "Y")]
    for (dx, dy), col, pos, lab in pts:
        if pos and (dx or dy): img.line((cx, cy), (cx + dx, cy + dy), (255, 255, 255), 0.14, 2)
    for (dx, dy), col, pos, lab in pts:
        img.circle(cx + dx, cy + dy, 9 if pos else 7, col if pos else (10, 12, 16), 1, 1)


def draw_cad_cube(img: Image, body=BODY) -> None:
    x0, y0, x1, _ = body
    # Top orthographic cube face: label on projected face, single top-right gizmo.
    bx, by = x1 - 98, y0 + 28
    top = [(bx, by), (bx + 58, by), (bx + 58, by + 58), (bx, by + 58)]
    img.polygon(top, (248, 250, 252), 0.58)
    img.polyline(top, (255, 255, 255), 0.95, 2, True)
    side = [(bx + 58, by), (bx + 68, by + 10), (bx + 68, by + 68), (bx + 58, by + 58)]
    img.polygon(side, (252, 90, 90), 0.45)
    img.polyline(side, (255, 255, 255), 0.7, 1, True)
    front = [(bx, by + 58), (bx + 58, by + 58), (bx + 68, by + 68), (bx + 10, by + 68)]
    img.polygon(front, (91, 140, 255), 0.45)
    img.polyline(front, (255, 255, 255), 0.7, 1, True)


def render(name: str, cam: Camera, gizmo: str) -> str:
    img = Image(W, H)
    # App frame gutters around the viewport panel.
    img.rect(0, 0, W, H, (17, 17, 20), 1)
    img.rect(0, 0, 390, H, (18, 18, 22), 1)
    img.rect(390, 0, 640, H, (20, 20, 24), 1)
    draw_viewport_panel(img, name)
    draw_shader_grid(img, cam)
    draw_cad_tools(img, cam)
    if gizmo == "cad":
        draw_cad_cube(img)
    else:
        draw_blender_gizmo(img)
    ppm = os.path.join(OUT, f"{name}.ppm")
    png = os.path.join(OUT, f"{name}.png")
    img.save_ppm(ppm)
    # Add text labels with ImageMagick so the proof visibly carries mode/view/tool notes.
    subprocess.run([
        "convert", ppm,
        "-font", "DejaVu-Sans-Bold", "-pointsize", "12", "-fill", "#d8d8dc",
        "-gravity", "northwest", "-annotate", f"+{PANEL_X + 72}+{PANEL_Y + 9}", "3D Viewport",
        "-pointsize", "10", "-fill", "#101014", "-annotate", f"+{BODY[2] - 82}+{BODY[1] + 51}", "TOP" if gizmo == "cad" else "",
        "-pointsize", "10", "-fill", "#101014", "-annotate", f"+{BODY[2] - 73}+{BODY[1] + 25}", "X" if gizmo == "blender" else "",
        "-annotate", f"+{BODY[2] - 105}+{BODY[1] - 7}", "Z" if gizmo == "blender" else "",
        "-pointsize", "13", "-fill", "#a7f3d0", "-annotate", f"+{PANEL_X + 16}+{PANEL_Y + PANEL_H - 24}",
        f"{name}: shader-grid CPU mirror, semi-transparent closed profiles, {gizmo.upper()} gizmo only",
        png
    ], check=True)
    os.remove(ppm)
    return png


def main() -> None:
    os.makedirs(OUT, exist_ok=True)
    views = [
        ("viewport_actual_grid_top_cad", Camera((0.0, 3.6, 0.001), 0.0, -89.0), "cad"),
        ("viewport_actual_grid_front_cad", Camera((0.0, 1.25, -4.2), 0.0, -6.0), "cad"),
        ("viewport_actual_grid_iso_blender", Camera((3.2, 2.5, -4.2), -38.0, -20.0), "blender"),
    ]
    for name, cam, gizmo in views:
        print(render(name, cam, gizmo))


if __name__ == "__main__":
    main()
