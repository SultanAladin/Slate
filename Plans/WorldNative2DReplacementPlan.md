# World-native 2D replacement plan

## Goal
Replace the remaining sketch-basis 2D authoring path with a world-native workplane path, while keeping `WorldSketchStructure` as the live authoring authority and preserving the sketch document only as a compatibility mirror for records, revisions, outliner state, and legacy dimension fallback.

## Immediate implementation scope
1. Stop deriving draw-plane authority from `SketchStructure`.
2. Make active `WorkplaneCatalogue` state the input/preview/commit plane for draw tools.
3. Let orthographic `XY/XZ/YZ` views switch the active standing workplane even after geometry already exists.
4. Keep rendering and selection world-backed.
5. Keep legacy sketch fallback paths resynchronising world state.
6. Preserve proof coverage for placement, bridge, drawing, rendering, and workplane behaviour.

## Code plan
### 1. Resolve workplane-native draw basis
- Add a reusable helper to turn `Workplane` into a `SpatialBasis`.
- Use that basis in the editor viewport draw path instead of `ResolveSketchBasis(Sketch)`.

### 2. Make world-backed placement commit take explicit workplane support
- Change `CommitPlacementWorldBacked(...)` so the caller passes the active `Workplane`.
- Use that explicit workplane to author support metadata and mirrored profile planes.
- Stop inferring support for new geometry from the sketch’s one global plane.

### 3. Keep sketch as compatibility only
- Only seed `Sketch.DeclarePlane(...)` when a sketch still has no declared plane.
- Do not require active plane changes to rewrite the sketch’s global plane once geometry already exists.
- Continue mirroring sketch-only fallback mutations back into persistent world state.

### 4. Improve sketch->world remirroring
- When rebuilding world state from sketch compatibility data, prefer each profile’s own stored plane for its member curves.
- Fall back to the sketch plane only for geometry with no profile-owned plane.
- This keeps fallback remirroring from collapsing all profile support back onto one legacy plane.

### 5. Orthographic plane/grid behaviour
- Always allow standing-plane activation from exact orthographic views now that world geometry is authoritative.
- Show the analytic grid only in exact orthographic plane views.
- Keep perspective free of forced workplane-grid presentation.

## Acceptance targets for this implementation
- Draw preview and placement land on the active workplane rather than the sketch basis.
- World-backed draw commit stores the active workplane’s support frame directly.
- Existing world geometry does not move when the active workplane changes.
- Switching to exact `XY`, `XZ`, or `YZ` ortho changes where new drawing lands, even after prior geometry exists.
- Sketch compatibility paths still sync back into persistent world state.
- Proofs stay green.

## Next-phase scope: semantic/workspace boundary and vocabulary

The immediate workplane phase leaves one concrete follow-up, rather than a broad removal of sketch compatibility: the world-sketch semantic kernel must stay in `SlateShape`, while camera- and viewport-dependent services stay in `SlateWorkspace`, and the provisional `WorldDraft` vocabulary must be retired. This phase is limited to that boundary and its proof/build wiring.

1. Rename the world authoring units, APIs, symbols, and owned proofs from `WorldDraft` to `WorldSketch`.
2. Split world picking into camera-free semantic placements and pivots in `SlateShape`, plus screen-space ray picking in `SlateWorkspace`.
3. Move world editing back to `SlateShape` because it consumes only semantic world picks; keep rendering projection, screen picking, interaction, transforms, and the bridge in `SlateWorkspace`.
4. Keep `SketchSnap` shape-only by accepting an explicit `SketchPlane`; resolve the active workspace `Workplane` at the interaction call site.
5. Re-aim structural and build-budget proofs at the renamed, partition-correct paths.

### Next-phase acceptance
- `VerifyPartition`, `VerifyNaming`, and `ValidateHostBuildBudgets` pass.
- Shape units include no workspace headers, and screen picking is the only world-picking unit that consumes camera projection.
- The renamed world-sketch proofs compile their shape and workspace seams independently and remain green.

## Active-plane compatibility handoff

The next authority-reduction phase is now concrete: the two remaining compatibility-oriented viewport paths must accept the active world plane explicitly instead of deriving a new placement or overlay basis from the sketch document.

1. `PlacementCommit` receives an explicit `SketchPlane` for active fallback commits. Its legacy overload remains available only for callers intentionally operating on the compatibility sketch.
2. Placement declarers and auto-constraint measurement use that supplied plane directly, so stale `SketchStructure::HeldPlane()` state cannot redirect a workplane placement.
3. `SketchViewportOverlay` exposes explicit-`SpatialBasis` entry points for the grid, constraint glyphs, and profile diagnostics. Existing sketch-only wrappers remain as legacy compatibility adapters.
4. The world-backed interaction and bridge paths use the explicit active plane when they reach compatibility commit behavior; world geometry and selection remain authoritative.

### Active-plane acceptance
- A compatibility fallback commit on an active plane writes profile geometry on that plane without rewriting the sketch's remembered global plane.
- Circle, ellipse, polygon, and slot profile declarers retain that active-plane routing rather than falling back to the stored sketch plane.
- An overlay recorded with an explicit active basis changes when that basis changes, without consulting the sketch basis.
- Legacy wrappers remain available, but no active workplane path calls the sketch-basis resolver.
- Placement, bridge, interaction, drawing, and world-sketch proofs stay green.

## Current phase result

The active-plane handoff is implemented. `PlacementCommit` now has an explicit-plane entry point used by drawing fallback and bridge compatibility commits, while the old sketch-plane overload remains available for deliberately legacy callers. Circle, ellipse, polygon, and slot profile declaration also have explicit-plane overloads, and every corresponding placement arm passes its active plane instead of reaching the sketch's stored plane. Grid, constraint-glyph, and profile-area overlay recording likewise accept an explicit `SpatialBasis`; sketch-only wrappers are isolated compatibility adapters.

## World-first authoring handoff

The world placement path is now authoritative for ordinary geometry creation. `CommitPlacementWorldBacked` imports a legacy sketch only when the world model is empty, declares new curves and loops in `WorldSketchStructure` first, and then appends the resulting geometry to `SketchStructure` for compatibility records and legacy consumers. World-backed viewport selection follows the same bootstrap-only import rule; normal frames no longer rebuild live world geometry from the sketch mirror. Dimension text editing remains an explicit compatibility seam until world dimensions are implemented.

### World-first acceptance
- A partial or stale compatibility sketch cannot erase existing world curves or loops during a new placement.
- New world-backed geometry is declared before compatibility sketch geometry.
- World transforms continue to mirror back into the sketch for records, revisions, and legacy consumers.
- The bootstrap import remains available for opening legacy sketch data.
- World-first placement and interaction proofs, structural budget checks, partition checks, and naming checks remain green.

## World-native semantic snapping

The world authoring model now owns semantic snap resolution. `WorldSketchSnap` accepts only a `WorldSketchStructure`, an explicit `WorldPlacementFrame`, and pending world anchors; it does not consult a camera, workplane catalogue, or compatibility `SketchStructure`. The workspace resolves the active `Workplane` into that frame and adapts the returned world names into the compatibility-facing `SealedPlacement` record at the boundary.

### World-snap acceptance
- Endpoints, centres, controls, midpoints, along-curve/perpendicular positions, intersections, tangents, and grid candidates resolve from world geometry.
- Semantic precedence remains stable, with geometry and pending anchors beating the grid.
- Intersections and grid coordinates use the supplied active frame rather than fixed ground-plane axes.
- A pending placement can snap before any compatibility sketch geometry exists.
- World curve/control mappings preserve compatibility selection and transform references when world and sketch names diverge.
- World snap and interaction proofs, strict compilation, partition checks, and host-budget checks remain green.

## Validation plan
- Strict compile:
  - `WorldSketchBridge.cpp`
  - `SketchInteraction.cpp`
  - `EditorHost.cpp` with `-DSLATE_COMBINED_AUTHORING`
- Proofs:
  - `WorldSketchSnapProof`
  - `WorldSketchPlacementCommitProof`
  - `WorldSketchBridgeProof`
  - `WorldSketchInteractionProof`
  - `WorldSketchTransformSessionProof`
  - `WorldSketchEditingProof`
  - `WorldSketchPickingProof`
  - `WorldSketchRenderProof`
  - `WorldSketchFoundationProof`
  - `SketchDrawingProof`
  - `WorkplaneCatalogueProof`
