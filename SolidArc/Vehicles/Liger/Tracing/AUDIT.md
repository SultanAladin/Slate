# Surface-tracing delivery audit

## Delivered scope

A native SolidArc tracing workspace over the previously delivered **dense patched
NURBS Liger reference**. Three imported reference sheets are locked and kept separate
from newly drawn curves. No new vehicle contours were authored in the shipped project.
Reference registration is a proper rigid rotation only; topology, knots, weights,
units and connectivity are unchanged. The prior blueprint-only packages are preserved.

| Area | Status | Evidence / behavior |
|---|---|---|
| Dedicated workspace | 🟢 | Opens the 306-face / 906-edge reference with Surface and XYZ enabled, edges hidden, compact single-view layout. |
| Surface drawing | 🟢 | Unmodified pointer hits exact CAD carriers; no construction-plane or grid fallback on misses. Curve, Arc and Line supported. |
| Ctrl edge snapping | 🟢 | Nearest visible **body** curve / edge within 12 px; no endpoint/centre ranking or accidental snap to a free user spline. Otherwise stays on surface. Native and actual Liger UI tests. |
| Whole-span conformance | 🟢 | Full seed projection, bounded NURBS fitting, independent 257-sample check at 0.2 mm. Holes, silhouettes, hidden controls and failed fits refuse atomically. |
| Real editable geometry | 🟢 | Native SpaceCurve poles and knots, not raster marks, meshes or plane-only sketches. Undo/redo, deterministic native save/load and separate-sheet loft checked. |
| XYZ editing | 🟢 | Explicit full-axis gizmo option, actual G → Z numeric movement tested; native pole edit changes all XYZ coordinates. Planar line endpoints can lift into 3D curves. |
| Reference protection | 🟢 | All three source sheets locked; tracing does not edit embedded STEP carriers. All 47,267 point/direction entities checked against the exact rigid registration. |
| Multi-view | 🟢 | Secondary panes render the active pane's cached **world** trace, not different re-projections of the seed. Ordinary split-view behavior retained. |
| Source reproducibility | 🟢 | Incremental native patch forward/reverse checked; changed sources mirrored under `SolidArc/Source`. Build/runtime provenance in `Validation/BuildManifest.json`. |
| Standalone resources | 🟢 | Each runtime includes 310 SVG icons, 66 SolidArc icons and five font faces; archive CRCs, bytes, architecture and executable hashes checked. |
| Windows execution | ⚠️ | Windows x64 cross-build succeeded. No Windows / Wine is available here: **interactive Windows execution is not claimed**. |

## Verification files

**Results:** 42 new native tracing checks; 57 Vulkan interaction expectations
(11 startup + 32 drawing/XYZ/save + 6 Ctrl-edge + 8 quad); 95 existing CAD gates;
40 Python tests without skips. All pass.

- `Validation/NativeTracing.txt`: analytic surface tests for all four spline forms,
  projected arcs, negative cases, original-reference immutability, native commit,
  save/load/undo/redo, XYZ editing, incident-line lifting, a real loft, and real dense
  Liger surface / edge snaps. Sphere fits additionally checked at 4,097 independent
  points, not just their clicked controls.
- `Validation/Drawing.txt`: actual Vulkan pointer/key workflow at **1280 × 680**,
  from a Runtime working directory and a project path containing spaces. Curve,
  Arc, Line, three-axis gizmo, depth movement, undo/redo and Ctrl+S.
- `Validation/Startup.txt`, `Edge.txt`, `Quad.txt`: startup defaults, off-surface
  click rejection, real Ctrl edge snap, four-pane world-space trace preview/commit.
- `Validation/DocumentRegression.txt`: all **95** existing CAD regression gates green.
  The former line-endpoint refusal expectation is deliberately updated to the new
  supported behavior. Its label is copied before appending steps, avoiding an
  invalid reference when the step vector reallocates.
- `Validation/ViewportRegression.txt`: Vulkan rendering regression green on SwiftShader.
- `Validation/PythonTests.txt`: **40 tests, no skips**, including native gap/tolerance
  checks via the rebuilt CAD certifier. `WorkspaceTests.txt` also rechecks the final
  deterministic compact-window workspace configuration.
- `Validation/Packaging.txt`, `BuildManifest.json`, `SHA256SUMS`: deliverable integrity
  and runtime identity. `Proofs/` contains actual native viewport captures.

## Explicit limitations (not hidden behind a green check)

- A draped arc is generally a **3D NURBS**, not an exact circular arc on a doubly
  curved sheet. Ordinary analytic planar tools remain available with Surface off.
- The 0.2 mm conformance check is **sampled**, not a continuum or trimmed p-curve
  proof. There is no persistent live shrinkwrap constraint after free XYZ edits.
- Use shorter visible sections at openings or silhouettes; no invented surface
  bridges holes. Hidden controls after an orbit refuse rather than jumping onto
  the newly visible front. A closed spline needs at least three controls.
- Maximum 65 controls / 64 fitted spans. Source defects remain; the three sheets
  are not represented as a repaired, watertight completed vehicle.
- Individual analytic arc/circle points retain their plane. Move the entire shape
  in XYZ, or use a traced NURBS for free spatial editing.
- Linux needs an X11 desktop and Vulkan driver. Windows needs a working Vulkan
  graphics driver. Launch with the supplied script so the project is explicitly
  opened; running the EXE alone still opens a blank document.

## Build provenance

Geometry source base: `unassignedinbox/Frontier` `82ed2512`, with the prior W95,
NURBS-importer and reference-interaction overlays. Desktop host:
`SultanAladin/Frontier` `65cf0f1`. Apply
`SolidArc/Patches/SolidArc-Surface-Tracing.patch` after those overlays.
Windows: Zig 0.14.1, x86_64-windows-gnu, static C++ runtime.
Linux: GCC 12.2, X11; native Vulkan captures use SwiftShader.
