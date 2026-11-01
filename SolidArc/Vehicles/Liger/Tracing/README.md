# Liger — surface tracing and spline editing

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
3. **Ctrl** prioritises authored curves within 12 pixels, then visible CAD body
   edges, then the surface. Hidden edge display does not disable edge snapping.
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

## Spline and component editing update

- **1** selects spline control points / body vertices. Select a control, then
  **X** or **Delete** removes that control, not its parent spline. Ordinary sketch
  spline controls also delete locally. At least two controls must remain; invalid
  edits refuse without deleting the object.
- **2** selects native 3D spline spans as well as CAD body edges. Click a control
  polygon edge or the corresponding curve interval. Shift adds spans on the same
  spline; Ctrl removes them. Selected spans highlight in blue.
- **Ctrl+Shift+D**, or **Feature → Subdivide**, inserts exact NURBS knots in the
  selected spans. Select the whole spline to subdivide all spans, or two adjacent
  controls to target their interval. The curve shape is preserved; this is an
  explicit editing command, not automatic extra points on Enter.
- **X/Delete** on selected spans removes only those intervals and retains the
  other exact curve pieces. Spans are defined by adjacent control parameters
  (Greville intervals), since intermediate NURBS poles are not points on the curve.
  A curve used by another feature refuses span deletion rather than destroying
  its dependents; detach those dependents first.
- **3** selects CAD faces; **4** selects whole bodies; **5** is general selection.
  Component deletion takes precedence over parent selections and history focus.
  CAD edge/vertex deletion uses local kernel dissolve; unsupported cases refuse
  without removing the parent. Whole-object deletion remains available in object /
  general selection mode.
- **Ctrl snapping now prioritises authored free curves**, then visible CAD edges,
  then the surface. Authored curves drawn as overlays remain snappable even when
  their ordinary unconstrained span lies slightly inside the reference. Consumed
  construction profiles do not take this priority.
- New open lofts / curve networks align their normals with the nearest locked
  reference. Without a reference, open sections use a consistent dominant-axis
  direction and closed-section tubes use the radial outside. Closed solids keep
  their existing outward orientation. The chosen sheet orientation persists in
  the project without changing the section order. Existing lofts are not silently
  changed; **Alt+N** remains available to flip an existing sheet.

Apply `SolidArc-Spline-Editing.patch` **after** the two preceding tracing patches.
