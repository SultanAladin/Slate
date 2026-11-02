# Global selection / Delete — completion audit

**2026-11-02 · Slate incremental base `e625d6026b7467f25245473f169282ff2f409a84`.**

## Requested fixes

| Status | Requirement | Implemented / verified |
|---|---|---|
| ✅ | Shift-click multiple curves in ordinary viewport use | Additive whole-curve picks in Any/whole-curve modes; cross-owner span/control picks upgrade to whole curves rather than replacing the old owner. Native 3D splines and planar sketch controls tested. Same-owner component additions remain component selections. |
| ✅ | Curves underneath transform handles remain selectable | Modifier curve hits take priority over an idle gizmo/dimension handle. Existing active drags and unobstructed Ctrl/Shift gizmo drags remain available. |
| ✅ | Outliner selections delete without changing mode manually | Single, Shift-range and Ctrl row picks clear stale components and choose whole-figure mode. Delete/X use actual whole-curve selection rather than rejecting an old viewport mode. |
| ✅ | Delete all supplied cage curves | Real Outliner selection / Ctrl+A / Delete removes all 23 curves, leaves all 3 locked reference imports; Ctrl+Z restores all 23. Native test additionally compares exact serialized undo state. |
| ✅ | Safe batch deletion | Copies/deduplicates the possibly aliased live selection, validates all members including locks before mutation, retires selected dependents before their inputs, keeps inputs needed by unselected downstream features. One undo for a curve-only batch. |
| ✅ | Deliberate component deletion remains local | Point/span/face/edge/vertex operations still run first and return even on refusal; no silent owner deletion as a component-error fallback. |
| ✅ | Supplied saved project untouched | Byte-identical supplied project in both ZIPs: 3 imports / 23 curves / 200 authoring controls / 3 existing pole edits. No new surfaces, lofts or replacement geometry. |
| ✅ | Full native packages | Windows x86-64 PE + Linux x86-64 ELF rebuilt and installed; single Project Liger folder, launcher, project, runtime, 310 SVGs (66 SolidArc), 5 font files, notices and payload SHA256SUMS. |
| ⚠️ | Interactive platform coverage | Linux UI rendered/exercised headlessly on real Vulkan via SwiftShader. Windows cross-build and PE/dependency/package checks passed; interactive Windows and desktop/X11 launching were **not** tested here. |

## Fresh regression evidence

| Status | Suite | Result | Evidence |
|---|---|---|---|
| ✅ | Native selection / batch deletion | 17 checks | [NativeSelection](Validation/NativeSelection.txt) |
| ✅ | Existing cage / editing / tracing | 49 + 51 + 92 checks | [Cage](Validation/NativeCage.txt), [Editing](Validation/NativeEditing.txt), [Tracing](Validation/NativeTracing.txt) |
| ✅ | New Vulkan UI regressions | 72 / 72 expectations across 4 scripts | [Viewport:25](Validation/SelectionViewport.txt), [Outliner:21](Validation/SelectionOutliner.txt), [Cage:14](Validation/SelectionCage.txt), [Sketch:12](Validation/SelectionSketch.txt) |
| ✅ | Existing Vulkan workflows | 175 / 175 expectations across 10 scripts | CageWorkflow, SurfaceOffsetWorkflow, CageProject and seven SurfaceTrace logs in Validation |
| ✅ | Full native CAD suite | 869 green table rows / 0 red; process exit 0 | [DocumentRegression](Validation/DocumentRegression.txt) |
| ✅ | Focused gizmo / dimensions rerun | 10 green checks / 0 red, including Ctrl-drag in perspective | [GizmoRegression](Validation/GizmoRegression.txt) |
| ✅ | Python workspace / tracing tests | 6 passed | [PythonTests](Validation/PythonTests.txt) |
| ✅ | Build provenance | All 79 Slate overlay files match the compiled Host tree; Release builds / installs exit 0 | [SourceManifest](Validation/SourceManifest.json), Linux/Windows Build, FinalBuild and Install logs |
| ✅ | ZIP integrity / exact payload | Both ZIP CRCs and every payload byte verified; source-project hash checked; Linux dependencies resolve | [PackageChecks](Validation/PackageChecks.txt), [RuntimeManifest](Validation/RuntimeManifest.json) |

The first development run exposed a double-click/Chain routing regression and a Ctrl-gizmo drag conflict. Both were corrected in production code; **the complete suites above were rerun on the final source**, without removing their assertions. Historical CageTools evidence is not substituted for these new results.

## Proofs

- [All cage curves deleted, three references retained](Proofs/Delete_All_Cage_Curves_Not_References.png)
- [Cage restored by undo](Proofs/Undo_All_Cage_Curves.png)
- [Viewport regression capture](Proofs/Global_Shift_Selection_And_Delete.png)
- [Outliner regression capture](Proofs/Outliner_Range_Delete_Undo.png)
- [Planar sketch regression capture](Proofs/Planar_Sketch_Global_Selection_Delete.png)

These are native application captures, not mockups. Curve-deletion operations were performed only on temporary project copies.

## Build / reproduce

Pinned Host: `SultanAladin/Frontier` at `65cf0f131a6afe8bdcd722c006b10cfab0e1aafd`.
Geometry/UI base: `unassignedinbox/Frontier` at `82ed251267ed8457ca3530d8228b91555572c25d`.
Overlay `SolidArc/Source`, apply the two geometry precision patches in `Tools/Patches`, use the native overlay integration and `Tools/BlueprintValidation.cmake`; configure Release / GLFW X11 for Linux and Zig 0.14.1 `x86_64-windows-gnu` for Windows. The supplied full SVG/font assets are included in each ZIP.

`NativeSelection.patch` is an **incremental patch against Slate e625d60**, not a standalone upstream Frontier patch. Native regression target: `LigerSelectionValidation supplied.solidarc three-curves.solidarc crossing-3d.solidarc crossing-sketch.solidarc`. Run the four `Tools/Selection*Check.txt` scripts through the rebuilt SolidArcCapture with their corresponding generated fixtures (SelectionCage uses a temporary copy of the supplied project). The ten existing scripts, output logs, build logs and source hashes are retained alongside them.

Packaging command: `python3 Tools/package_selection.py --windows <installed-windows> --linux <installed-linux>` from the Liger directory. It refuses incomplete native/UI/CAD/Python results or a changed source project.

## Scope / limitations retained

- Exact overlapping hit targets still use ordinary nearest-hit selection, not a new cycling selector; the Outliner disambiguates them. Modifier key-release-order permutations were not separately tested.
- Same-owner Shift point/span picks intentionally remain component picks. Use 6 or an Outliner row for unambiguous whole-curve selection.
- The earlier live surface distance remains a numerical triangle-mesh geodesic approximation; this selection fix does not change that algorithm or its documented limits.
- Linux desktop packages need an X11 desktop and Vulkan driver; Windows needs a compatible Vulkan driver and normal Windows runtime components. SwiftShader is used for validation only, not included as a replacement desktop graphics driver.

## Deliverable hashes

Supplied and packaged project SHA-256: `90344c326b32e467b8298851eb8b1df2a2dba1ce879becd3c7a12303b252ec70`.

### Windows

[Download Project Liger Selection Fix Windows.zip](Packages/Project%20Liger%20Selection%20Fix%20Windows.zip)

- ZIP SHA-256: `99126973a55bf2bafe1a55492b6ff8ba7f2cc5e557ac15d1043ac9bfab094718`
- Runtime SHA-256: `40e3268e8e2ce8bf187abbf76f3ea12de5afcbbf08e5d45d373234bb881909c9`

### Linux

[Download Project Liger Selection Fix Linux.zip](Packages/Project%20Liger%20Selection%20Fix%20Linux.zip)

- ZIP SHA-256: `7ecc2085c092644326d0ed65d12f82f36e1856c5c0c9f0e0ecbe608d57a16362`
- Runtime SHA-256: `a280a107c11222248187e41d5983d47a03d186fb4eb7d7a521501c8a51bc7e47`
