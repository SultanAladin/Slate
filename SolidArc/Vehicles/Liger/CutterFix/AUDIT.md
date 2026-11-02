# Surface cutter placement / de-duplication — completion audit

**2026-11-02 · Incremental base: Slate `ff06f9161cc9f525eb3b81e6729fd9f79d4f4f6b`.**

## Completion matrix

| Status | Requirement | Implementation / evidence |
|---|---|---|
| ✅ | Orthographic cuts start on the surface, not the world grid | Knife profiles resolve the first-hit planar face or curved-face tangent plane independently of ordinary drawing's Orient to faces preference. Six axis views tested on a box translated away from every origin grid plane. |
| ✅ | Cut from the surface inward | Negative face-normal depth retained; native tests verify exact Boolean volume against the actually clicked outline at typed depth 0.25 on every box side, in orthographic and perspective views. |
| ✅ | Shared placement for profile and shaped cutters | Shaped cutters use the same forced face resolution; a Ctrl point snap no longer bypasses the face frame/target into XY. Face-only picking and a one-pixel adjacent-face fallback handle exact/quantized boundaries. |
| ✅ | Surface tracing cannot hijack cutter placement | Knife profiles do not use retopology tracing, even with tracing enabled. The tracing popup is not offered during Knife. Global tracing / face-orientation preferences are preserved; stale temporary axis locks are reset for a new cutter. |
| ✅ | Remove redundant basic cutter entries | Rectangle + depth / Circle + depth / Polygon + depth are the canonical box / cylinder / prism cuts. Duplicate Block / Cylinder / Prism entries removed from both toolbar menu and inspector. Ordinary Construct primitives and internal compatibility paths remain. |
| ✅ | Keep distinct capabilities | Cone, Sphere, Torus, Wedge and Tube remain; Imprint only, Objects and compound-library cutters remain separate. Kept standalone basic cutter bodies are still possible through Construct + Objects / Keep cutter, rather than the removed primitive-specific duplicate entry. |
| ✅ | Preserve deliberate ordinary drawing behavior | Non-Knife drawing still respects disabled face orientation and its chosen grid. Normal planar arcs, point-only tracing, geodesic links, selection/Delete and component-delete safety regressions rerun. |
| ✅ | Preserve supplied project | Original file and both packaged copies have the supplied SHA-256. No assistant-created cage/surface/loft edits: 3 imports, 23 curves, 200 saved controls, 3 prior pole edits. Tests use disposable fixtures/copies. |
| ✅ | Rebuilt native apps and full assets | Release Linux x86-64 ELF and Windows x86-64 PE builds/installations completed. Packages include one Project Liger folder, launcher, supplied project, executable, 310 SVGs (66 SolidArc), 5 fonts, notices and SHA256SUMS. |
| ⚠️ | Platform execution coverage | Linux UI exercised through native ImGui and real headless Vulkan / SwiftShader. Windows cross-built and package/PE/dependency-checked, **not interactively executed**. Desktop X11 startup was not tested here. |

## Fresh validation

| Status | Suite | Result | Evidence |
|---|---|---|---|
| ✅ | Surface cutter native UI / geometry checks | **112 passed**: all six box faces × both projection types; inward volume, face retention, preference preservation, undo, curved tangent, shaped placement, Ctrl boundary snaps, menu de-duplication and ordinary drawing | [NativeCutterPlacement](Validation/NativeCutterPlacement.txt) |
| ✅ | Rendered orthographic cutter workflow | **22 / 22 expectations**: tracing enabled, surface plane at X=5 (not origin), typed inward pocket, commit, volume bounds and undo | [CutterSurface](Validation/CutterSurface.txt) |
| ✅ | Selection / cage / editing / tracing native regressions | **17 + 49 + 51 + 92 passed** | [Selection](Validation/NativeSelection.txt), [Cage](Validation/NativeCage.txt), [Editing](Validation/NativeEditing.txt), [Tracing](Validation/NativeTracing.txt) |
| ✅ | Four selection and ten existing Vulkan scripts | **247 / 247 expectations** | Selection*, CageWorkflow, SurfaceOffsetWorkflow, CageProject and SurfaceTrace* logs in Validation |
| ✅ | Full CAD regression | **869 green table rows / 0 red**, exit 0 | [DocumentRegression](Validation/DocumentRegression.txt) |
| ✅ | Focused gizmo / dimension regression | **10 green / 0 red**, including precision/snapping-compatible Ctrl-gizmo drag | [GizmoRegression](Validation/GizmoRegression.txt) |
| ✅ | Python workspace / tracing regression | **6 passed** | [PythonTests](Validation/PythonTests.txt) |
| ✅ | Build provenance | **79 / 79** Slate overlay files match the final compiled Host source tree; Release builds/installations exit 0 | [SourceManifest](Validation/SourceManifest.json), Linux/Windows Build, FinalBuild and Install logs |
| ✅ | Release integrity | Exact supplied project, current binary markers, full resources, ZIP CRCs and payload hashes checked | [PackageChecks](Validation/PackageChecks.txt), [RuntimeManifest](Validation/RuntimeManifest.json) |

Totals: **321 native feature checks; 269 Vulkan expectations**. CAD suite and focused gizmo results are reported separately, without treating aggregate table rows as independent feature assertions.

Native volume checks use the actual authored rectangle dimensions after integer-pixel pointer projection; they do not pretend a scripted world click can land between physical pixels. The comparison to Boolean removed volume retains a 1e-6 tolerance. A boundary-snap development failure led to the face-only / adjacent-pixel placement correction; final tests include that case.

## Native proof captures

- [Unified menu: no duplicate basic 3D entries](Proofs/Unified_Surface_Cutter_Menu.png)
- [Orthographic face-attached inward pocket preview](Proofs/Ortho_Surface_Inward_Pocket_Preview.png)
- [Committed pocket](Proofs/Ortho_Surface_Inward_Pocket.png)
- [Undo restores uncut body](Proofs/Ortho_Surface_Cut_Undo.png)

These are application captures, not mockups. The translated box is a disposable validation fixture, **not added to the supplied Liger project**.

## Scope / usage limits

- A curved-face cutter is planar in the tangent plane at its first hit and extrudes along that fixed normal. It is **not** a surface-draped profile or a conformal constant-depth offset.
- Empty-space starts retain the previous grid fallback. A one-pixel boundary neighborhood is a picking tolerance, not an unlimited nearest-surface search. Hidden/locked geometry safety and Boolean refusals remain in force.
- Same-owner component picks/deletion remain intentional components. Previous global curve Shift-selection and Outliner Delete fixes are included unchanged.
- Live compound/object Boolean workflows remain; the standard outline + depth entry does not newly expose primitive-only Keep cutter or create a standalone kept body automatically. Use ordinary Construct + Objects when that is required.
- Windows and Linux desktop packages need a working Vulkan driver; Linux needs X11. SwiftShader was used for testing only and is not bundled as the desktop driver.

## Reproduction / provenance

Host: `SultanAladin/Frontier` commit `65cf0f131a6afe8bdcd722c006b10cfab0e1aafd`.
Geometry/UI base: `unassignedinbox/Frontier` commit `82ed251267ed8457ca3530d8228b91555572c25d`.
Overlay `SolidArc/Source`, apply the two precision patches in `Tools/Patches`, use the native integration and `Tools/BlueprintValidation.cmake`. Configure Release / GLFW X11 for Linux and Zig 0.14.1 `x86_64-windows-gnu` for Windows. Source hashes and test-tool hashes are recorded in the manifest.

`NativeCutterPlacement.patch` is incremental against **Slate ff06f91**, not a standalone patch against upstream Frontier. Build target `LigerCutterPlacementValidation`; run it with an optional output `.solidarc` path to export the disposable translated-box fixture. Run `Tools/CutterSurfaceCheck.txt` with SolidArcCapture against that fixture, using the packaged tracing settings. The existing selection/cage/tracing fixtures and scripts are also rerun on the final binaries.

Release command from Liger: `python3 Tools/package_cutter.py --windows <installed-windows> --linux <installed-linux>`. The packager refuses incomplete native/UI/CAD/Python results or a changed supplied project.

Original/project SHA-256: `90344c326b32e467b8298851eb8b1df2a2dba1ce879becd3c7a12303b252ec70`.

Runtime and archive SHA-256 values are listed in [RuntimeManifest](Validation/RuntimeManifest.json), and every ZIP contains its own payload `SHA256SUMS`.

## Package hashes

### Windows

- ZIP: `Project Liger Cutter Fix Windows.zip`
- ZIP SHA-256: `fcd6c19ffd2bcdf03bd3f5c7807163beb07092a93272d7190ce308c15b771c47`
- Runtime SHA-256: `7734a40ffe544358520bedc141a5f93bbd7899ab784ab888a01ded535d3ebbd4`

### Linux

- ZIP: `Project Liger Cutter Fix Linux.zip`
- ZIP SHA-256: `24802b06c1a5215c3f2adbe187544d5bae7af59c0ac9867366c838a2860ee4c6`
- Runtime SHA-256: `86679f3fc2fab9d7ee8c1dc9dc51db2abc44f493f914fb27cbc879adaa72eabd`
