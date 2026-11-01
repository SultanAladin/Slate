# Liger — dedicated surface-tracing workspace

**New native SolidArc functionality**, not a mesh drawing or a pre-modelled replacement.
The Windows / Linux packages contain one `Project Liger` folder: `Liger.solidarc`,
a short guide, one platform launcher, and the self-contained app under `Runtime`.

## Start

Extract the **whole** ZIP. On Windows double-click **Start Liger.bat** (not the EXE).
It passes the project and options explicitly, including from paths with spaces.
On Linux run `bash "Start Liger.sh"` (X11 desktop and a working Vulkan driver required).

The three dense NURBS reference sheets open **locked**, in a single perspective
viewport, with **Surface snap** and **XYZ editing** enabled. The source has 306
faces / 906 edges, not a watertight repaired vehicle. Wheels, interior and underside
have not been invented. Existing shape defects / holes are intentionally preserved.
A proper rigid rotation matches the previous blueprints: nose −Y, right +X, up +Z.

## Draw your spline network

1. Choose a useful view (Numpad 1 front / 3 right / 7 top / 0 isometric).
   Middle-drag orbits; right-drag pans; wheel zooms. Numpad 5 toggles projection.
2. **Shift+S** starts a curve. Click **on the reference surface**. No modifier is
   needed. `1` Hermite, `2` Bézier, `3` NURBS, `4` cubic spline select the seed form.
3. **Hold Ctrl** to target the nearest **visible model edge / curve within 12 px**.
   This is an exact body edge, not a faceted display-mesh edge or point/centre snap.
   Away from an edge, Ctrl continues to snap to the surface.
4. **Enter / right-click** finishes an open section. **Ctrl+Z** drops the last click
   while drawing; **Esc** cancels without changing the document. Click the first
   control to close a spline loop. Finish one section before changing the view. If an existing control becomes hidden,
   the trace refuses instead of moving that control onto the newly visible front.
5. Draw separate sparse sections / rails, then select them in loft order and **L**
   for the existing native loft workflow. The reference sheets stay separate.

**Arc (3 points)** uses start, end, then a point on the arc. **Shift+A** draws a
surface-draped line / polyline. The complete seed is projected, then fitted to a
native editable **3D NURBS**. A circular arc generally cannot remain an exact circle
on a doubly curved sheet: its draped result is a NURBS, not a falsely labelled exact
arc. Turn **Surface** off to use the ordinary analytic planar arc / circle tools.

### Surface check and limitations

Drawing points land on exact CAD carriers, refined from visible tessellation hits.
Entire seed spans are projected, not merely the clicked controls. The finished fit
gets an independent **257-sample** surface-distance check at **0.2 mm**; this is a
sampled check, not a proof of a globally surface-bound p-curve. A hole, silhouette,
invalid projection or fit failure refuses the commit, with the drawing still open.
Add controls, change view **between sections**, or split the section rather than
bridging empty space. Very long / complicated curves may need shorter sections
(65 controls and 64 fit spans maximum).

Toggle **Reference edges** to see the dense patch network. Edges remain available
for Ctrl snaps when their display is off. **Surface** in the compact viewport pill
switches this workflow on/off; the inspector has the same toggle and help.

## Move curves in XYZ

**XYZ editing** keeps all three gizmo axes available, including in cardinal views.
An axis pointing straight at you cannot be usefully dragged from that view: orbit,
or select the curve / point and use **G → X/Y/Z → type a distance → Enter**.
Escape cancels the transform. Mode `1` exposes curve poles / sketch points.
Planar **line endpoints can now lift out of plane** into real 3D curves; spline
points already could. Individual analytic arc/circle points retain their plane;
move the whole analytic shape or use a traced NURBS for spatial editing.

Free XYZ edits are **not a live shrinkwrap constraint**: moving a pole can take the
curve off the reference. Surface checking happens when a trace is created. Undo,
redo, Ctrl+S and native save/load preserve the real poles and knots.

## Source / build

Use the existing Frontier `82ed2512` + W95 / importer / reference-startup /
reference-interaction overlays documented in `../Blueprints/README.md`, then apply
`../../../Patches/SolidArc-Surface-Tracing.patch`. Updated sources are mirrored
under `SolidArc/Source`. Desktop host: SultanAladin/Frontier `65cf0f1`.
`Tools/SurfaceTraceValidation.cpp` supplies native projection / persistence /
editing tests; the four `SurfaceTrace*Check.txt` scripts exercise the actual UI.

Windows x64 binaries are cross-built with Zig 0.14.1. Native geometry and Vulkan
interaction checks run on Linux with SwiftShader. **Windows execution itself is
not verified in this sandbox** (no Windows or Wine); PE architecture, resources,
launch command, archive integrity and executable hashes are checked.
