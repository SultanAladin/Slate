# Surface-point-only tracing correction — delivery audit

## Requested correction

**Only user-placed controls snap to the reference and become editable points.**
The earlier whole-span projection / adaptive fitting behavior is removed from
both the Surface preview and commit paths. Enter does not add poles to follow
the body. The reference, existing user geometry and historical packages are not
rewritten. Previously fitted curves remain unchanged; this correction applies
to newly drawn curves.

| Area | Status | Verification / behavior |
|---|---|---|
| No extra points on Enter | 🟢 | Actual three-click NURBS commits with **3 points / 3 poles**; actual two-click line and quad-view curve have **2**. Snapshot and script evidence included. |
| Exact snapped controls | 🟢 | Native NURBS and Bezier tests for 2, 3, 4, 8 and 16 controls verify identical XYZ values and counts; commit and save/load retain the counts. No sampling/fitting creates saved controls. |
| Span behavior | 🟢 | Ordinary control-point spans, deliberately **not surface-constrained**. A sphere test verifies that the span leaves the sphere while its placed controls remain unchanged. Preview tessellation is display-only. |
| Spline forms | 🟢 | Surface mode defaults to NURBS (3), also supports Bezier (2). Hermite/cubic interpolation is excluded here to prevent extra tangent poles; ordinary non-Surface tools retain these forms. Actual keyboard refusal and Bezier creation tested. |
| Explicit loop closure | 🟢 | Click the first point as the explicit final placement. Three distinct controls plus the closing click produce exactly **4** poles, not additional generated closure poles. Native persistence and actual UI tested. The automatic Close button is disabled in Surface mode. |
| Line / Arc | 🟢 | Line is an exact degree-1 curve through only its placed vertices. Three-point Arc stays analytic on its three world points' plane, using normal centre/start/end editing handles, not an adaptively draped NURBS. Actual UI creation tested. |
| Surface / Ctrl edge snapping | 🟢 | Existing exact CAD-carrier placement and Ctrl nearest-visible-body-edge snapping retained. Surface misses still do not fall back to the construction plane. Startup / edge scripts pass. |
| XYZ / undo / redo / save | 🟢 | Actual G → Z numeric edit, all three gizmo axes, undo/redo and Ctrl+S pass. Native editable poles / knots and existing line-endpoint lifting retained. |
| Split views | 🟢 | Four-pane preview shares cached world geometry; two clicks commit with exactly two poles. |
| Reference and workspace | 🟢 | Clean factory project has only three locked imported sheets, 306 faces / 906 edges, no authored curves. Deterministic workspace and all 47,267 rigid point/direction transforms rechecked. |
| Native and CAD regressions | 🟢 | **92 native tracing checks**, including 50 point-only checks and 42 retained snapping/geometry/persistence/loft/legacy-fitter regressions. **95 CAD gates** pass with complete runtime resources; Vulkan viewport regression passes. |
| Actual UI checks | 🟢 | **78 / 78 expectations**: Startup 11, Drawing 35, Edge 6, Quad 9, Control 17. Final Linux build, 1280 × 680, Runtime cwd and project paths containing spaces. |
| Python regression | 🟢 | **40 tests**, no skips, including rebuilt native certifier integration. |
| Rebuilt distribution | 🟢 | Linux and Windows x64 rebuilt and installed; both ZIPs checked byte-for-byte, CRCs, runtime SHA, launcher paths, 310 SVGs / 66 SolidArc icons / five font faces. |
| Source patch | 🟢 | `SolidArc-Surface-Point-Only.patch` applies after the previous tracing patch; forward and reverse applicability checked. Native source mirrors match the compiled Host. |
| Windows execution | ⚠️ | Windows cross-build / PE / resources / launcher structurally checked. **No Windows/Wine runtime is available here; Windows interactive execution is not claimed.** |

## Evidence

- `Validation/NativeTracing.txt`: current 92 checks. Retained standalone projection
  utility tests are compatibility regressions, **not the active authoring path**.
  `SolidArcInterface.cpp` no longer calls `ProjectSolidArcTrace` for preview or commit.
- `Validation/{Startup,Drawing,Edge,Quad,Control}.txt`: actual final-build Vulkan
  interaction reports. Control includes form restriction, Bezier count and explicit
  loop closure; Drawing includes NURBS count, XYZ, undo/redo/save, Arc and Line.
- `Proofs/Curve_XYZ.png`: actual three-control curve; corresponding capture snapshot
  records `Points: 3`, `Poles: 3`. `Proofs/Closed_Control_Curve.png` shows explicit closure.
- `Validation/DocumentRegression.txt`, `ViewportRegression.txt`, `PythonTests.txt`:
  current regressions. CAD tests run from the complete Runtime resource directory;
  the icon gate verifies 66 non-empty SVGs.
- `Validation/{LinuxBuild,WindowsBuild,LinuxInstall,WindowsInstall}.txt`,
  `BuildManifest.json`, `Packaging.txt` and `SHA256SUMS`: build and delivery provenance.

## Limits and usage

NURBS / Bezier intermediate controls need not lie on the curve. Spans may leave
the surface or bridge gaps; this is intentional, not a failed conformance check.
There is no live shrinkwrap after edits. Analytic arc points retain their plane;
move the whole arc in XYZ or use a control-point NURBS for spatial pole editing.
The reference remains three open sheets with its original defects, not a completed
watertight vehicle. Existing curves with extra poles are not silently simplified.

Extract into a **new folder** to avoid overwriting your work. Windows users open
**Start Liger.bat**; running the EXE alone still opens a blank document. Vulkan
is required; Linux additionally requires an X11 desktop.

## Build provenance

Desktop host `SultanAladin/Frontier` `65cf0f1`; geometry base
`unassignedinbox/Frontier` `82ed2512` with the previously documented overlays and
`SolidArc-Surface-Tracing.patch`, then the incremental point-only correction.
Windows: Zig 0.14.1, x86_64-windows-gnu, static C++ runtime.
Linux: GCC 12.2 / X11; Vulkan validation uses SwiftShader.
