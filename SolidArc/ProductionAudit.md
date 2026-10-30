# SolidArc Production Audit, Boolean Acceleration & Modifier Stack Report

## 1. Executive Summary

- **Target repository:** `unassignedinbox/Frontier` (branch `arena/d90f0539-frontier`, base `82ed2512`).
- **Document & feature validation (`W1`–`W80`):** **800 🟢 / 0 🔴** (`Results/DocumentValidation.md`).
- **Stage 3 new feature & performance gates (`W72`–`W80`):** **19 🟢 / 0 🔴** in `3.82 s`.
- **Standalone GeometryProcessing kernel CTest suites:** **31 / 31 🟢** (`SurfaceGeometry`, `ModelPartition`, `Tessellation`, `Intersection`, `Containment`, `Boolean`, `AlgorithmStress`, `Construction`, `BodyCut`, `Sketch`, `SketchTrim`, `Sweep`, `Feature`, `Blend`, `RollingBall`, `FaceOperation`, `Replay`, `Measure`, `Assembly`, `Drawing`, `Nurbs`, `Setback`, `Continuity`, `FreeformBlend`, `Exchange`, `PatternStress`, and supporting suites).
- **Standalone Windows & Linux Release packages:** Cross-built cleanly via `CrossBuildWindows.sh` (Clang 22.1.8 `x86_64-windows-gnu`) and native Linux Release, with dereferenced SVG icon packaging verified (`66` `EngineContent/Icons/SolidArc/*.svg` and `310` total SVG icons in every standalone zip).

---

## 2. Production Audit & Subsystem Completeness Matrix

Legend: Status `🟢` pass·production-ready / `🔴` blocking defect · Cost/Risk `✔️` low · `🚩` medium · `🔴` high (cost rises left to right).

| Status | Subsystem                      | Audit Scope & Key Findings                                                        | Risk | Validation Evidence                                     |
|--------|--------------------------------|-----------------------------------------------------------------------------------|------|---------------------------------------------------------|
| 🟢     | Exact Predicates & Topology    | Radial-edge `ModelPartition`, Euler edits, `T1`–`T6` & `G1`–`G6` topological audit | ✔️   | `ModelPartitionValidation`, `W1`–`W78` (`0 🔴`)         |
| 🟢     | Analytic & NURBS Carriers      | Plane, Cylinder, Cone, Sphere, Torus, RollingBall, Parallel, rational NURBS       | ✔️   | `SurfaceGeometryValidation`, `NurbsValidation` (`🟢`)   |
| 🟢     | Surface Intersection (SSI)     | Added UV AABB trim pre-rejection; eliminates redundant NURBS marching             | ✔️   | `IntersectionValidation` (`41.88 s 🟢`), `W72`          |
| 🟢     | Winding Containment            | Added `SolidContainmentIndex` cached face meshes + 3-ray early majority exit      | ✔️   | `ContainmentValidation` (`67.72 s 🟢`), `W56`           |
| 🟢     | Boolean Engine (`GeneralFuse`) | Disjoint fast paths (`Subtract` clone, `Intersect` empty, `Union` assembly)       | ✔️   | `BooleanValidation` (`41.67 s 🟢`), `W72` (`≤ 15.7 ms`) |
| 🟢     | Carrier Transform Preservation | Non-uniform axis-aligned/orthogonal cylinder & cone stay analytic (not NURBS)     | ✔️   | `W72` (`0` NURBS faces on stretched Round cutter)       |
| 🟢     | Fillet, Chamfer & Setback      | Rolling-ball, variable-radius, mixed-vertex setback; updated stale `S6` check     | ✔️   | `BlendValidation`, `RollingBallValidation`, `Setback`   |
| 🟢     | 2D Sketch Solver & Trim        | Constraints, profiles, spline/conic trim; updated stale `R7` refusal check        | ✔️   | `SketchValidation`, `SketchTrimValidation` (`0.02 s 🟢`)|
| 🟢     | Sweeps, Lofts, Shell & Draft   | Extrude, Revolve, Pipe/Path Sweep, Loft, Shell, Offset, Draft, Split, Mirror      | ✔️   | `SweepValidation`, `FaceOperationValidation`, `W1`–`W71`|
| 🟢     | Parametric Variables           | Named `FeatureParameter` bindings across features (`Combine.Offset`) & modifiers  | ✔️   | `W73` (`ΔV/V = 1.15e-16`), `W75`, TOML v1 round-trip    |
| 🟢     | Multi-Stage Compound Cutters   | 6 non-stretching, zero-duplicate 2-stage cutters (`Ø₁ → D₁ → Ø₂ → D₂`), 0 NURBS   | ✔️   | `W79` (`ΔV/V = 1.50e-13`, `0` NURBS faces, `0` dupes)   |
| 🟢     | Semitransparent Live Booleans  | Solid target + semitransparent (`α = 0.34`, Copper) realtime-editable Live Cutter | ✔️   | `W74`, `W80` (`ΔV/V = 1.03e-14` realtime push & fillet) |
| 🟢     | Blender-Style Modifier Stack   | `Boolean`, `Bevel`, `LinearArray`, `CircularArray`, `CurveArray`, `SurfaceArray`, `CurveBend` | ✔️ | `W74`–`W78` (`ΔV/V ≤ 2.45e-07`, `ModifierCheckpoint`)   |
| 🟢     | Edge Weight Paint & Groups     | Persistent `FeatureEdgeGroup` weights (literal or variable-bound) for `Bevel`     | ✔️   | `W75` (`ΔV/V = 3.35e-16`, variable update `2.0 → 3.0`)  |
| 🟢     | Scene Tessellation & Viewport  | 32-entry `BodyDigest` LRU mesh cache; fixed heavy-modifier `StageGizmoPreview` throttle | ✔️ | `TessellationValidation`, `W31` (`112.8×` clone speedup)|
| 🟢     | Assemblies, Measure & Drawings | Rigid joints/mates, exact clearances/mass properties, HLR 2D SVG/DXF sheets       | ✔️   | `AssemblyValidation`, `MeasureValidation`, `DrawingVal` |
| 🟢     | STEP / STL / `.solidarc` I/O   | AP214/AP203 STEP cache, watertight binary/ASCII STL, `.solidarc` TOML v1 codec    | ✔️   | `ExchangeValidation` (`🟢`), `W73`–`W80` byte-identical |
| 🟢     | Standalone & Browser Packaging | Standalone `SolidArcWorkspace` + `Frontier` zips verified with all 66 SolidArc SVGs| ✔️   | `W80` (`66` SolidArc SVGs / `310` total SVGs in all zips)|

---

## 3. Root-Cause Analysis & Fixes for Slow / Freezing Booleans

### 3.1 Root Causes Identified During Audit (`⏱️` / `🐞`)
- **🐞 Analytic-to-NURBS promotion in Boolean Cutter Library (`BodyTransform.cpp`):** Placing and stretching a KitOps/library cutter (`Round`, `Slot`, `Box`, `Gusset`) applies a non-uniform diagonal scale in the cutter's local frame (`ScaleX ≠ ScaleY` or `ScaleZ ≠ ScaleX`). Previously, `TransformBody` promoted every non-uniformly scaled `CylinderSurface` or `ConeSurface` into a rational tensor-product `NurbsSurface` when `ScaleX ≠ ScaleY`, even when the scale along the cylinder's two radial axes was isotropic and only the axial height stretched (`AxialScale ≠ RadialScale`). Intersecting two promoted rational NURBS surfaces triggered expensive subdivision + marching SSI instead of closed-form quadric SSI.
- **⏱️ Uncached O(N × M) ray-mesh rebuilding in `GeneralFuse` (`WindingNumber.cpp`, `GeneralFuse.cpp`):** Classifying `K` candidate cells rebuilt full face triangulations for `BodyA` and `BodyB` on every `ClassifyPointInSolid` query and always cast all 5 rays even when the first 3 rays agreed unanimously.
- **⏱️ Missing disjoint AABB fast paths in `GeneralFuse` (`GeneralFuse.cpp`):** Moving a Live Boolean cutter outside the target's bounding box still executed full cell decomposition, boundary paving, and winding classification instead of returning an instant clone (`Subtract`), empty result (`Intersect`), or multi-region assembly (`Union`).
- **⏱️ Untrimmed NURBS carrier marching in `SurfaceIntersection` (`SurfaceIntersection.cpp`):** Face-pair SSI marched across full untrimmed carrier domains even when one face's active UV loop bounding box was disjoint from the partner surface's 3D AABB.
- **⏱️ Uncached STEP imports & scene re-tessellation (`FeatureTree.cpp`, `SolidArcScene.cpp`):** Replaying trees containing `ImportStep` re-parsed STEP text on every regeneration, and `BuildSolidArcScene` re-tessellated unchanged bodies when sibling cutters moved.
- **🐞 `StageGizmoPreview` throttle timestamp bug (`SolidArcInterface.cpp`):** `Gizmo.QuietSince` was overwritten on every mouse-delta frame rather than on preview completion, starving continuous gizmo drags on heavy (>50 ms) compound stacks.

### 3.2 Measured Performance Results (`W72` & `W1`–`W78`)
- **Worst-case Boolean Library cut + full viewport scene build (`W72`):** **14.9–15.7 ms** across `Box`, `Round`, `Slot`, and `Hex` library cutters (gate budget `≤ 50 ms` — **60+ FPS interactive**).
- **Analytic preservation (`W72`):** Coaxial stretched `Round` cutter produces **0 NURBS faces** (exact `CylinderSurface` + `PlaneSurface` preserved).
- **Preview document clone (`W31`):** **112.8× faster** than full replay (`ModifierCheckpoint` + `RegenerationCache` + `StepImportCache`).
- **Full `W1`–`W30` regression header:** **37.51 s** (faster than **39.44 s** baseline despite richer modifier/parameter digesting).

---

## 4. New Production Architecture Summary

### 4.1 Modular Parametric Variables (`FeatureParameter` + `Combine.Offset`)
- **Kernel (`FeatureTree.{h,cpp}`, `ReplayDigest.cpp`):** Extended `FeatureStep` (`Step.Offset`, `Step.OffsetAxis` for parametric bolt/cutter clearance shifts in `Combine`), `FeatureEdgeGroup::Weight`, and every numeric field on `FeatureModifier` with `FeatureValue` bindings (`Value` + `Parameter` name).
- **Document & UI (`SolidArcDocument.{h,cpp}`, `SolidArcInterface.cpp`, `SolidArcDocumentCodec.cpp`):** Full `Create`, `Set`, `Rename`, `Delete` (cleanly baking last evaluated values on delete so geometry never jumps), Inspector `Variables` card + inline `fx` variable-binding pills on feature/modifier fields, and `.solidarc` TOML v1 persistence.

### 4.2 Semitransparent Realtime-Editable Live Booleans & Multi-Stage Compound Cutters
- **Multi-Stage Compound Cutters (`SolidArcDocument.{h,cpp}`, `SolidArcKnife.{h,cpp}`, `SolidArcInterface.{h,cpp}`):** Removed duplicate 1-stage shapes and corner-to-corner non-uniform stretching (`Aspect`) from the Boolean Library, replacing them with **6 native analytic multi-stage Compound Cutters** (`Countersink`, `Counterbore`, `Stepped hole`, `Hex socket`, `Stepped slot`, `Stepped pocket`) drawn interactively stage-by-stage (`Place centre → Stage 1 Ø → Stage 1 extrude → Stage 2 Ø → Stage 2 extrude`, with `Backspace` stage step-back, `T` Through-All, and `L` Live Boolean). User-saved `.step` cutters scale strictly uniformly (`Aspect = 1.0`).
- **Solid Target + Semitransparent Cutter Viewport (`SolidArcScene.{h,cpp}`, `SolidArcRasterization.{h,cpp}`):** Two-pass scene assembly emits opaque solid bodies in `[0 .. SolidCornerCount)` and `Appearance.LiveCutters` bodies in `[SolidCornerCount .. CornerCount)`, rendered via depth-biased `CutterDepthPipeline` + `CutterGhostPipeline` (`α = 0.34`, `"Copper"` default matcap) plus occluded strokes so coincident pocket walls never z-fight and the semitransparent cutter stays directly selectable by body, face, or submerged edge.
- **Realtime Cutter Editing (`W80`):** Because `ModifierCheckpoint` invalidates whenever any step precedes a `ModifierKind::Boolean` and body keys remain stable across direct edits, moving (`G`/`R`/`S`), Push/Pulling faces (`FaceOperation`), or Beveling edges (`Blend`) directly on the semitransparent cutter updates the carved solid target body in real time.
- **Standalone Icon Packaging (`SolidArc.cmake`, `CrossBuildWindows.sh`, `SolidArcInterface.cpp`):** Fixed `OverlayFrontier.py` symlink omission in standalone packages by dereferencing `EngineContent/Icons/SolidArc/` during CMake install and archive creation (`66` `SolidArc/*.svg` and `310` total SVGs verified in every zip) and resolving `EngineContent/Icons` beside the standalone executable without `./` prefix mismatches.

### 4.3 Blender-Style Modular Modifier Stack & Edge Weight Paint
- **7 Non-Destructive Modifiers (`BodyModifiers.{h,cpp}`, `FeatureTree.{h,cpp}`):**
  1. `Boolean` (`Subtract`, `Union`, `Intersect` + parametric `Offset` along `AxisDirection`).
  2. `Bevel` (`Fillet` / `Chamfer`, `AllEdges` / `ConvexOnly` / `ConcaveOnly` / `EdgeGroup`, multiplied per-edge by `EdgeGroup` weights).
  3. `LinearArray` (1D/2D `CountX × CountY` grid along `StepX`, `StepY` with fast disjoint assembly or `MergeCondition` union).
  4. `CircularArray` (radial & helical `AxialOffset` pattern around `AxisOrigin` + `AxisDirection` over `SweepAngle`).
  5. `CurveArray` (3D rail curve from sketch/wire body using rotation-minimising frames + arc-length table).
  6. `SurfaceArray` (`CountU × CountV` pattern across trimmed UV `FaceDomain`, skipping interior holes, aligned to outward surface normal).
  7. `CurveBend` (`DeformBodyAlongCurve`: certified 3D spatial B-rep deformation mapping analytic & NURBS faces/edges along a 3D rail's rotation-minimising frames via adaptive cubic B-spline `CarrierApproximation`, with `std::fmax` exact-precision tolerance propagation).
- **Stack Controls & Caching:** Full `Add`, `Move Up/Down`, `Mute/Unmute`, `Apply` (collapse prefix to base tree or split disjoint array shells into independent bodies), `Remove`, Undo/Redo checkpoints, `ModifierCheckpoint` base-body cache, and byte-identical `.solidarc` TOML round-trip.

---

## 5. Additional Pre-Release Audit Fixes Applied

- **`DeformBodyAlongCurve` NaN precision propagation (`BodyModifiers.cpp`):** `EdgeProfile::Precision` and `VertexProfile::Precision` default to `ExactPrecision` (`NaN`); replaced `std::max` with `std::fmax` so deformed edges/vertices pass `ModelAudit` `T5` with `0` NaN tolerances.
- **Standalone Kernel Suite Stale Refusals (`SketchTrimValidation.cpp:370`, `SetbackValidation.cpp:199`):** Updated two legacy refusal checks that predated spline curve trimming (`W57`) and flat chamfer corner patches on L-blocks, bringing standalone `KernelSuites` to **31 / 31 Passed**.

---

## 6. Tomorrow's Testing & Packaging Checklist

- **Windows GPU Smoke Pass:** Unzip `Exhibits/SolidArc/Package/SolidArc-Windows-x64.zip` on a Windows 10/11 Vulkan workstation and verify interactive gizmo dragging on Live Boolean cutters, Edge Weight assignment + Bevel modifier, and stacked `CurveBend → LinearArray` modifiers.
- **Optional GPU Capture Walkthroughs:** Run `SolidArcViewportValidation`, `SolidArcBrowserValidation`, and `SolidArcCapture` (`Tests #95`–`#100`) on a host with a hardware Vulkan ICD (skipped cleanly with `exit=77` in this headless container).
