# Liger — surface-point tracing (no extra curve points)

The app now snaps **only the controls you place**. Enter saves exactly those
controls and their snapped XYZ coordinates. It does **not** project whole spans,
fit the curve to the reference, or insert more control points. A three-click
NURBS has three editable poles, including after save/load.

## Start

Extract the entire ZIP into a **new folder**, preserving your previous work.
On Windows open **Start Liger.bat**. On Linux run `bash "Start Liger.sh"`
(X11 and Vulkan required). Each package has one `Project Liger` folder with
the project, guide, launcher and complete `Runtime` resources.

The three dense NURBS reference sheets open locked. Surface snap and XYZ editing
are enabled. The reference is unchanged: 306 faces / 906 edges, three open sheets,
not a repaired watertight vehicle. Nose −Y, right +X, up +Z.

## Draw

1. Choose a view: Numpad 1 front, 3 right, 7 top, 0 isometric; 5 toggles projection.
   Middle-drag orbits, right-drag pans, wheel zooms.
2. **Shift+S** starts a control-point curve, default **NURBS** (`3`).
   **Bezier** (`2`) is also available. Click on the reference to place each control.
   Interpolating Hermite/cubic modes are excluded in Surface mode because their
   conversion generates extra tangent poles. They remain available with Surface off.
3. **Ctrl** snaps to the nearest visible body edge within 12 pixels; otherwise it
   continues to snap to the surface. Hidden edge display does not disable snapping.
4. **Enter / right-click** finishes the curve with just your placed controls.
   **Ctrl+Z** drops a point while drawing; **Esc** cancels. To close a loop, explicitly
   click the first control after placing at least three distinct controls. That
   closing click is the last control; there is no automatic closure insertion.
5. **Shift+A** draws a straight line / polyline with only the placed vertices.
   **Arc** uses start, end, then a point on the arc; it remains an ordinary analytic
   arc on the plane defined by those three world points, not a draped NURBS. Its
   standard editing handles are centre/start/end, not fitted surface poles.
6. Draw sections / rails, select in order and use **L** for the native loft workflow.

**Only controls snap. Spans may leave the reference or bridge gaps.** NURBS and
Bezier are control-point curves: intermediate controls need not lie on the curve
itself. Neither spans nor subsequent edits are constrained to the surface.
Preview tessellation vertices are display-only and are never saved as controls.
Previously created fitted curves are not automatically modified or simplified.

## XYZ editing and persistence

All three gizmo axes remain available, including in cardinal views. Orbit if an
axis points straight at you, or use **G → X/Y/Z → type distance → Enter**. Esc
cancels. Mode `1` exposes editable poles / sketch points. Native undo, redo and
Ctrl+S preserve the controls and knots. Planar line endpoints can lift into 3D;
individual analytic arc points retain their plane (move the whole arc instead).

## Build and verification

Apply the existing overlays and `SolidArc-Surface-Tracing.patch` described in the
previous release, then **`SolidArc-Surface-Point-Only.patch`**. Both patches are in
`SolidArc/Patches`; the latter is incremental to the former. Current native sources
are mirrored in `SolidArc/Source`. Host `65cf0f1`, geometry `82ed2512`.

Native tests include exact control count/XYZ, commit and save/load for NURBS and
Bezier with 2, 3, 4, 8 and 16 points. Actual Vulkan scripts cover drawing, XYZ,
undo/redo/save, edge snapping and split views. See `AUDIT.md` for final results.

Windows x64 is cross-built with Zig 0.14.1; native geometry and Vulkan interaction
checks run on Linux/SwiftShader. **Windows execution itself is not verified in
this sandbox**; PE architecture, resources, launcher and archive integrity are checked.
