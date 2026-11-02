# Liger cage tools — completeness / correctness audit

Date: **2026-11-02**. Branch: `arena/4f335fa8-slate`.

## Delivery status and scope

| Status | Requirement | Evidence / result |
|---|---|---|
| 🟢 | Preserve the supplied rough cage; do not loft | Packaged project is byte-identical to `Liger(progress).solidarc`, SHA-256 `90344c326b32e467b8298851eb8b1df2a2dba1ce879becd3c7a12303b252ec70`: 3 imports, 23 native curves, 200 saved controls and 3 pre-existing pole edits. No patches/lofts added. |
| 🟢 | Planar wheel arcs, independent of surface snapping | Actual supplied cage opened in the native Vulkan app; YZ side-plane analytic arc drawn at X = 0.8 m, then undone. `Validation/CageProject.txt`: 19/19. |
| 🟢 | Clear surface-tracing exit and small drawing menu | Esc cancels; Surface toolbar / inspector / pre-first-point tool checkbox exits tracing. Arc itself is always planar. Small movable/closable menu exposes surface snap, curve snap, auto-cut, plane/depth. `Validation/Cage.txt`: 25/25. |
| 🟢 | Whole curves separate from spans / controls | Mode 6 / rail button, ordered Shift selection, Ctrl removal, box selection and mode-specific Ctrl+A. Mode 2 remains spans; mode 1 controls. Stale span/pole picks cleared on keyboard/rail mode changes. Actual cage Ctrl+A selects 23 curves, zero reference bodies. |
| 🟢 | Live surface-distance relationship | First selected curve is guide, second target; reference/distance/tolerance, swap/update and detach controls. Native plane/cylinder tests distinguish intrinsic distance from a chord. UI link, guide translation, regeneration and undo: `Validation/Offset.txt`, 16/16. Native tests cover codec persistence, history reordering, protected derived controls and larger guide translation past old anchors. |
| 🟡 | Continuous-surface precision / large deformation | Implemented as a **mesh-geodesic approximation**, not an exact CAD-surface offset. Adaptive mesh-contour convergence is not a certified continuous-surface error bound. Target extents follow midpoint translation, not a persisted rotating frame; large bends/rotations can require re-linking. Resource/geometry failures refuse rather than silently use XYZ distance. |
| 🟢 | Optional true-3D curve cutting | Default off; exact B-spline subcurves, explicit XYZ joint tolerance, one transaction for drawing + cutting. Native depth-separated crossing test does not cut. Actual supplied-cage dry run: 23→43 pieces; undo restores exact serialized source. Native non-rational 3D splines only; analytic sketch arcs/overlaps/tangencies are deliberately excluded. |
| 🟢 | Inspect failed-looking cage joints | `Validation/CageTopCrossings.tsv`: 31 top-view intersections, 18 within 1 mm (including already shared endpoints), 13 with larger actual 3D separation. These 13 must not be auto-cut on screen position alone. The delivered project was not repaired/replaced. |
| 🟢 | Point-only authoring remains intact | 92 native surface-tracing checks, including N explicit clicks→N identical XYZ controls, plus 51 curve-editing checks. Optional explicit offset, subdivision and cutting may generate additional geometry by design. |
| 🟢 | Native regression coverage | 49 cage-tool checks; 92 tracing checks; 51 editing checks; full document validation: 869 green rows, zero red. Six deterministic project/launcher Python tests pass. |
| 🟢 | Native GPU coverage | Ten scripted native Vulkan workflows: **175/175 expectations**. SwiftShader / Linux X11, 4× MSAA; no browser mock or screenshots substituted for implementation. |
| 🟢 | Native source / reproduction | Full current source overlay in `SolidArc/Source`; incremental `Tools/Patches/NativeCageTools.patch` reverse-applies cleanly against the final build host. Source hashes recorded; build-host files byte-match the overlay. |
| 🟢 | Standalone packaging | Windows x64 and Linux x64 native applications, byte-identical supplied cage, full 310 SVGs (66 SolidArc), five packaged font files and third-party notices. One `Project Liger` folder and one platform launcher. ZIP CRC and every payload byte verified by `Tools/package_cage.py`. |
| 🟡 | Windows runtime validation | PE x64 cross-build / imports and package checks only. **No interactive Windows/Wine execution claimed.** Interactive native tests ran on Linux. |

## Exact tested workflows

| Workflow | Expectations |
|---|---:|
| Whole curves / popup / planar analytic Arc / optional cuts | 25 / 25 |
| Ordered live offset / guide edit / undo | 16 / 16 |
| Actual supplied cage / Ctrl+A / YZ wheel arc / undo | 19 / 19 |
| Existing surface-tracing startup | 11 / 11 |
| Existing drawing / XYZ movement / independent planar Arc / Line | 35 / 35 |
| Existing authored-edge priority | 6 / 6 |
| Existing quad viewport | 9 / 9 |
| Existing point-only controls / form restriction | 17 / 17 |
| Existing spline control/span editing | 19 / 19 |
| Existing topology deletion | 18 / 18 |
| **Total** | **175 / 175** |

The independent small-arc regression explicitly disables the new automatic authored-curve attraction to avoid snapping all closely spaced clicks back onto the nearby spline. General Surface preference can remain on for later NURBS/Line gestures while Arc stays planar; this is not surface draping.

## Cage-joint findings

At projected top-view crossings, examples with genuine separation are Curve#15/18 (**1.906 mm**), #11/13 (**2.664 mm**), #12/13 (**3.797 mm**), #4/9 (**4.551 mm**), and #6/7 (**18.659 mm**). Other larger separations are listed in the TSV. These are evaluated curve positions at numerically refined XY crossings, not a claim to enumerate all possible closest points, contacts or intersections in every view.

Cuts use a separate 3D root/proximity test. The default 1 mm threshold does not bridge these example gaps. Existing shared endpoints and interior intersections are distinct: crossing curves were previously independent curve records, not automatically split topology. New automatic authored endpoint/curve snapping is visible and independently switchable; Ctrl snapping gives authored geometry priority before CAD surfaces.

## Build / package provenance

- Desktop host: `SultanAladin/Frontier` **65cf0f1**.
- Geometry base: `unassignedinbox/Frontier` **82ed251267ed8457ca3530d8228b91555572c25d** with the existing precision/import/tracing overlay plus this native cage update.
- Source base in this repository: **8078ca9**. Full overlay: `SolidArc/Source`. Incremental patch: `../Tools/Patches/NativeCageTools.patch` (apply after the previously delivered tracing overlay, or copy the current overlay directly).
- Linux: GCC **12.2.0**, Release, GLFW/X11, Vulkan.
- Windows: Zig **0.14.1**, `x86_64-windows-gnu`, Release, static C++ runtime.
- Build targets: `SolidArc`, `SolidArcCapture`, `SolidArcDocumentValidation`, `LigerSurfaceTraceValidation`, `LigerCurveEditingValidation`, `LigerCageToolsValidation`; verification targets are provided by `Tools/BlueprintValidation.cmake`.
- Native cage test invocation: `LigerCageToolsValidation '<supplied cage>' <cage-fixture> <offset-fixture> <inspection.tsv>`.
- Script sources: `../Tools/CageWorkflowCheck.txt`, `SurfaceOffsetWorkflowCheck.txt`, `CageProjectCheck.txt`, and the existing `SurfaceTrace*Check.txt` scripts.
- `Validation/SourceManifest.json`: source hashes / bases. `RuntimeManifest.json`: executable/archive hashes, dependencies and asset counts. `SHA256SUMS`: all audit, proof, log and package files (excluding itself).

The supplied project is unchanged in both ZIPs. The demonstrative wheel arc and split/offset test fixtures exist only in isolated test documents/screenshots, **not** in the packaged cage. No future loft or vehicle geometry has been substituted for the user's work.
