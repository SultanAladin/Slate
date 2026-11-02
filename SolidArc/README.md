# SolidArc CAD Audit, Features & Binary Packages

> **Latest — surface-attached cutters:** [Cutter placement fix and rebuilt apps](Vehicles/Liger/CutterFix/README.md). Knife profiles attach to the first-clicked surface in orthographic and perspective views; one standard outline + depth workflow replaces duplicate basic cutter entries. The global curve-selection/Delete fixes are included. [Audit](Vehicles/Liger/CutterFix/AUDIT.md) · [Downloads](Vehicles/Liger/CutterFix/Packages/).

> **Latest — Liger surface tracing:** [dedicated protected NURBS workspace + rebuilt Windows/Linux apps](Vehicles/Liger/Tracing/README.md). Draw editable 3D curves directly on the reference, Ctrl-snap to model edges, and edit in XYZ. [Downloads](Vehicles/Liger/Tracing/Packages/) · [audit](Vehicles/Liger/Tracing/AUDIT.md). Earlier packages below are preserved.

> **Liger reference-workflow correction (2026-11-01):** the newly rebuilt apps and aligned scratch project are in [Vehicles/Liger/Blueprints](Vehicles/Liger/Blueprints/README.md). These fix named views, side-view sketch planes and add faint/dashed guides plus a compact Offset pill. Original W95 packages below are preserved, not replaced.

> **Latest Liger scratch-modelling setup:** [high-resolution schematics + `Liger.solidarc`](Vehicles/Liger/Blueprints/README.md), with **new Windows and Linux/X11 runtime bundles** under [`Vehicles/Liger/Blueprints/Packages/`](Vehicles/Liger/Blueprints/Packages/). These include the NURBS importer fixes and registered-reference view/filtering support. The historical W95 packages below are preserved unchanged.

All deliverables for `unassignedinbox/Frontier` (`arena/d90f0539-frontier`, base `82ed2512`) are staged in this folder
on session branch `arena/4f335fa8-slate` so they appear directly in the Arena Workspace viewer and on GitHub.

---

## 1. Pre-Built Release Binaries

| Platform    | Package                                    | Contents                                                                              |
|-------------|--------------------------------------------|---------------------------------------------------------------------------------------|
| Windows x64 | `Package/SolidArc-Windows-x64.zip` (4.20M) | Standalone `SolidArcWorkspace.exe`, `SolidArcWorkspace.toml`, 5 DM Sans fonts, 310 SVG icons (incl. all 66 `SolidArc/*.svg`) |
| Windows x64 | `Package/Frontier-Windows-x64.zip` (7.65M) | `Frontier.exe` + `SolidArcWorkspace.exe`, 5 DM Sans fonts, 310 SVG icons (incl. all 66 `SolidArc/*.svg`) |
| Linux x64   | `Package/SolidArc-Linux-x64.zip` (4.85M)   | Standalone `SolidArcWorkspace`, `SolidArcWorkspace.toml`, 5 DM Sans fonts, 310 SVG icons (incl. all 66 `SolidArc/*.svg`) |

Direct download URLs on GitHub (`arena/4f335fa8-slate`):
- `https://raw.githubusercontent.com/SultanAladin/Slate/arena/4f335fa8-slate/SolidArc/Package/SolidArc-Windows-x64.zip`
- `https://raw.githubusercontent.com/SultanAladin/Slate/arena/4f335fa8-slate/SolidArc/Package/Frontier-Windows-x64.zip`
- `https://raw.githubusercontent.com/SultanAladin/Slate/arena/4f335fa8-slate/SolidArc/Package/SolidArc-Linux-x64.zip`
- `https://raw.githubusercontent.com/SultanAladin/Slate/arena/4f335fa8-slate/SolidArc/Patches/SolidArc-Booleans-Variables-Modifiers.patch`

---

## 2. Applying Source Changes to `unassignedinbox/Frontier`

Apply the unified patch cleanly onto `unassignedinbox/Frontier` branch `arena/d90f0539-frontier` (`82ed2512`):

```bash
git checkout arena/d90f0539-frontier
git apply /path/to/SolidArc/Patches/SolidArc-Booleans-Variables-Modifiers.patch
cp /path/to/SolidArc/Package/SolidArc-Windows-x64.zip Exhibits/SolidArc/Package/SolidArc-Windows-x64.zip
```

Every modified and newly created C++ source file is also mirrored verbatim under `SolidArc/Source/` at its exact
`Frontier` repository path:
- `Source/Engine/GeometryProcessing/` — Boolean acceleration, carrier preservation, `BodyModifiers.{h,cpp}`,
  parametric variables, edge groups, modifier stack, `ModifierCheckpoint` replay cache, shared-endpoint V-lofts,
  singular `u`-rail transpose sewing, 3-curve triangular Coons patches (`LoftTriangularLoop`), Möbius loop untwist,
  3-way T-fin sheet fallback, twisted-NURBS `RollingBall` spine damping & non-planar `ChartCarrier` SSI acceleration
  (`MeasureRollingBallDistanceNear` + tangential Newton guard + `RegenerationOutcome::Audits`), and multi-pass vertex
  `ReshapeBodyVertices` across bilinear `(1,1)` & general trimmed NURBS faces.
- `Source/UI/SolidArc/` — Non-stretching multi-stage Compound Cutters (`Countersink`, `Counterbore`, `Stepped hole`,
  `Hex socket`, `Stepped slot`, `Stepped pocket`), Solid + Semitransparent Realtime-Editable Live Booleans (`CutterDepthPipeline`
  & `CutterGhostPipeline`), Top/Bottom/Front/Back/Left/Right/3D gizmo alignment across all 2D & 3D primitives and
  sketch/SpaceCurve pole handles (`SolidArcGizmoTarget::Poles`), continued lofting from curves/edges/sheets
  (`LoftSolidArcSelection`), `Ctrl+I` Isolate & `Ctrl+A` Select All hotkeys (`Ctrl+Shift+I` Import), Outliner bottom
  `"Construct geometry"` card removal, associative CAD Dimension Lines (`Linear`, `Ø`, `R`, `°`, `W×D×H`, `dX`/`dY`/`dZ` +
  `[[Dimensions]]` TOML), Windows Explorer `Shift+Click` `[A..Z]` & `Ctrl+Click` Outliner/Viewport multi-select sync,
  continuous 3-decade `SolidArcGrid.slang` shader + SPIR-V archive, FOV-aligned 3D UV-plane `ImGui::GetFont()` Orientation
  Cube, Blender-like `G`/`R`/`S` modal transforms (single-axis gizmo isolation + 3D axis guide line + mouse & numeric editing),
  Blender Numpad camera navigation (`1`/`3`/`7`/`0`/`9`/`5`/`2`/`4`/`6`/`8`/`+`/`-`/`.`), docked `2`/`3`/`4`-pane Multi-View Split
  Viewport (`RecordPanes` + `@ViewportSplit` + draggable internal splitters), jitter-free `0.0001 px` Gizmo zoom scaling across
  wheel/shortcut/modal/drag frames, full interactive CAD Dimension Tool (`D` / `@SketchDimension`) with `X`/`Y`/`Z` 3D world-axis
  dimension line sliding, Cutter-style placed-dimension editing card (`Plane`/`X`/`Y`/`Z`, `Flip`, `Note`, live `X`/`Y`/`Z` re-slide),
  and top-header `@DimensionTool` (`"Dim"`) viewport visibility toggle, Variables card, Edge Weights card,
  Blender-style Modifiers stack UI, 32-entry scene mesh LRU cache, standalone icon directory resolution, and `.solidarc` TOML v1 serialization.
- `Source/Experimental/SolidArc/` — Standalone CMake icon dereferencing + `CrossBuildWindows.sh` compressed packaging &
  icon-count verification.
- `Source/Exhibits/` — `SolidArcDocumentValidation.cpp` (`W72`–`W95`), `SolidArcCapture.cpp`, walkthrough scripts, and
  updated standalone kernel checks.

---

## 3. Audit & Validation Reports

- `ProductionAudit.md` — Full CAD completeness/correctness audit report with emoji matrix and root-cause analysis.
- `Results/DocumentValidation.md` — Complete `W1`–`W95` run (**869 🟢 / 0 🔴**).
