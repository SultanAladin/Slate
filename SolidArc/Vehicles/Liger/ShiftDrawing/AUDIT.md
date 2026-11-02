# Global Shift straight drawing — completion / correctness audit

**2026-11-02 · Incremental base: Slate `c327c4181caf238f52e10eb7e2e65bdcfa44c34f`.**

## Completion matrix

| Status | Requirement | Implementation / evidence |
|---|---|---|
| ✅ | Hold Shift for exact straight alignment | Shared post-snap placement projects the pointer onto the nearest drawing-plane U/V axis through the previous click; the other coordinate is exactly equal, not merely an angle hint. |
| ✅ | Global Line / Polyline behavior | Every polyline segment uses its previous point. Ordinary drawing, Knife / Imprint profiles and click-drag Line use the same viewport placement. All three principal sketch planes, both projections and signed directions tested. |
| ✅ | Arc end alignment without collapsing the arc | Shift constrains only the second endpoint. Third/bulge point stays free while Shift is held. Vertical alignment preserves the first plane coordinate, horizontal alignment the second. Slot centre alignment has the same stage-aware exclusion for width. |
| ✅ | Correct side / face planes | Uses the session plane, including a tilted clicked face, not a hard-coded XY screen projection. Surface-traced Line instead uses nearest world XYZ through its last 3D point. |
| ✅ | Non-sticky behavior and snap precedence | Release returns to free placement. Explicit picker locks win; Shift wins incompatible automatic/Ctrl point snaps. Off-axis first-point closure magnet cannot break the Shift constraint; collinear closure remains available. |
| ✅ | Visible, understandable feedback | Temporary coloured dashed guide, visible axis glyph / HUD label and stage-aware hints. No persistent picker lock is set. Surface-line HUD warns that straight world axes may leave the surface. |
| ✅ | Leave freeform and unrelated Shift actions unchanged | Freeform curves, rectangle diagonals, circle/solid sizing excluded. Existing fine adjustment, Shift multi-selection and deliberate sub-element deletion regressions preserved. |
| ✅ | Preserve exact supplied cage | Original and both ZIP copies retain supplied SHA-256; no lofts, authored surface patches, replacement curves or control fitting. Tests use disposable fixtures. |
| ✅ | Rebuilt standalone packages | Fresh Release Linux x86-64 and Windows x86-64 builds/installations; one Project Liger folder, launcher, exact supplied project, 310 SVGs, 5 fonts, notices and payload SHA256SUMS. |
| ⚠️ | Platform execution limits | Native ImGui and real headless Vulkan / SwiftShader exercised on Linux. Windows cross-built / PE and package-checked, not interactively executed. Desktop X11 startup not exercised here. |

## Fresh validation

| Status | Suite | Result | Evidence |
|---|---|---|---|
| ✅ | New native Shift drawing checks | **185 passed**: 3 planes × 2 projections × signed directions; exact arc/slot stages, polyline references/release/closure, freeform exclusions, Ctrl/explicit-lock precedence, tilted face, Knife/Imprint, click-drag and surface XYZ / exact two controls | [NativeShiftDrawing](Validation/NativeShiftDrawing.txt) |
| ✅ | New rendered Shift workflow | **31 / 31 expectations**, 4 native captures: aligned arc chord, free bulge, independent polyline segment axes, unconstrained freeform curve | [ShiftDrawing](Validation/ShiftDrawing.txt) |
| ✅ | Preserved native feature regressions | **112 cutter + 17 selection + 49 cage + 51 editing + 92 tracing = 321 passed** | NativeCutterPlacement / NativeSelection / NativeCage / NativeEditing / NativeTracing logs in Validation |
| ✅ | Preserved rendered workflows | **269 / 269 expectations**, 15 scripts: surface cutters; selection in sketch / viewport / Outliner / cage; cage and surface-distance workflow; existing surface tracing/editing/deletion | Corresponding logs in Validation |
| ✅ | Full CAD regression | **869 green table rows / 0 red**, exit 0 | [DocumentRegression](Validation/DocumentRegression.txt) |
| ✅ | Focused gizmo / dimension regression | **10 green / 0 red**, exit 0 | [GizmoRegression](Validation/GizmoRegression.txt) |
| ✅ | Python workspace / tracing regression | **6 passed** | [PythonTests](Validation/PythonTests.txt) |
| ✅ | Build provenance | **79 / 79** overlay files match the compiled Host tree; fresh Linux / Windows build and install exit 0 | [SourceManifest](Validation/SourceManifest.json), Build / Install logs |
| ✅ | Release integrity | Fresh binary markers and hashes, exact original project, complete resources, archive CRCs and every payload hash checked | [PackageChecks](Validation/PackageChecks.txt), [RuntimeManifest](Validation/RuntimeManifest.json) |

Totals: **506 native feature checks; 300 / 300 Vulkan expectations across 16 scripts**. CAD and focused gizmo table rows are reported separately, not counted as independent feature assertions.

During GUI test development, Shift+A was identified as the two-click Segment tool rather than Polyline, and a floating Drawing options window intercepted a target hover. The final workflow selects the actual Construct → Sketch → Polyline entry and hides the popup before that hover. The same geometry/snap assertions then pass; no production workaround or reduced coordinate tolerance was introduced.

## Application proofs

- [Arc endpoints aligned](Proofs/Shift_Arc_Aligned_Endpoints.png)
- [Arc bulge remains free while Shift is held](Proofs/Shift_Arc_Free_Bulge.png)
- [Polyline chooses the next axis from its previous click](Proofs/Shift_Polyline_Per_Segment_Axes.png)
- [Freeform curve remains unconstrained](Proofs/Shift_Leaves_Freeform_Curves_Free.png)

These are actual Vulkan application captures, not generated mockups. The empty drawing fixture is a disposable test project, not a modification of your Liger cage.

## Scope and limits

- Alignment is to sketch-plane U/V, not arbitrary viewport screen axes. On a principal plane these correspond to its two world axes; on a clicked tilted face they follow that face.
- Only arc endpoints / slot centres align. Keeping the third shape point free is intentional. Freeform curves are excluded.
- A surface-traced Line uses exact world XYZ. Straight axis alignment and remaining on an arbitrary curved surface cannot generally both be satisfied; Shift deliberately chooses the axis and warns that it may leave the surface. Only clicked points are saved.
- Explicit axis/tangent/normal/binormal locks take precedence. Shift may reject an incompatible automatic/Ctrl point snap; release Shift when that point matters more than axis alignment.
- Existing surface cutters remain fixed tangent-plane extrusions, not conformal cuts. No new automatic lofts or cage changes are part of this release.

## Reproduction / provenance

Host: `SultanAladin/Frontier` `65cf0f131a6afe8bdcd722c006b10cfab0e1aafd`; geometry/UI base: `unassignedinbox/Frontier` `82ed251267ed8457ca3530d8228b91555572c25d`. Overlay `SolidArc/Source`, retain the two precision patches in `Tools/Patches`, use native integration and `Tools/BlueprintValidation.cmake`. Release Linux GCC 12.2 / GLFW X11; Windows Zig 0.14.1 `x86_64-windows-gnu`.

`NativeShiftDrawing.patch` is incremental against **Slate c327c41**, not a standalone upstream Frontier patch. Build and run `LigerShiftDrawingValidation` from a runtime working directory with EngineContent available. Run `Tools/ShiftDrawingCheck.txt` with `SolidArcCapture` on an empty document with the tracing workspace settings, 1280×680, Vulkan / SwiftShader. Rerun the prior cutter/selection/cage/tracing fixtures and scripts on the same final production code.

Release gate: `python3 Tools/package_shift_drawing.py --windows <installed-windows> --linux <installed-linux>`. It layers the preserved cutter/selection/cage/tracing/CAD/Python guards with the new 185-native / 31-rendered checks and refuses stale binaries or a changed supplied project.

Original/project SHA-256: `90344c326b32e467b8298851eb8b1df2a2dba1ce879becd3c7a12303b252ec70`. Package and runtime hashes are in RuntimeManifest; every ZIP carries SHA256SUMS.

## Package hashes

### Windows

- ZIP: `Project Liger Shift Drawing Windows.zip`
- ZIP SHA-256: `919b2cdacb636087c65db5f0c19149cd0045953143af1c32cb13ddce9412162b`
- Runtime SHA-256: `f15a11ecf10d6b1cffac7b2cd5bd4b2c6bd52e03740eb801b6d3eae984b1b7ed`

### Linux

- ZIP: `Project Liger Shift Drawing Linux.zip`
- ZIP SHA-256: `3bfbe3c3bad48120bf6b47207e0df4d3a2c93c6596cd2e6372dc0682318bf22f`
- Runtime SHA-256: `4482bc9a8b01e2fa8e16664c3f77ecba2aea52f61b5a0562c41e220d7dc5b6fb`
