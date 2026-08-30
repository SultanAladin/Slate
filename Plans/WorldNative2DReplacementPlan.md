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

The world placement path is now authoritative for ordinary geometry creation. `CommitPlacementWorldBacked` imports a legacy sketch only when the world model is empty, declares new curves and loops in `WorldSketchStructure` first, and then appends the resulting geometry to `SketchStructure` for compatibility records and legacy consumers. World-backed viewport selection follows the same bootstrap-only import rule; normal frames no longer rebuild live world geometry from the sketch mirror. Driving dimensions now follow the same world-first path, with sketch dimensions retained only as mapped compatibility records.

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

## World-native semantic constraints

World constraint authoring now follows the same authority boundary as snapping. `WorldSketchStructure` owns
`WorldConstraintName`, `WorldConstraintSubject`, semantic references, and stored specifications. The
camera-free `WorldSketchConstraintAuthoring` unit builds those specifications from `WorldPick` values, while
`WorldSketchConstraintSolver` evaluates and applies supported relationships directly to world curves and
points. The solver preserves authored support frames while enforcing horizontal and vertical relationships.

The workspace constraint interaction resolves screen picks into semantic `WorldPick` values, declares and
solves the world constraint first, then mirrors only the resulting compatibility `ConstraintSpecification`
and workspace record. `WorldSketchMapping` records the world/compatibility constraint pairing alongside curve
and loop mappings. Existing sketch-only constraint authoring remains available for the deliberate legacy path;
world-backed interaction routes constraint tools through the world authoring seam.

### World-constraint acceptance
- Constraint specifications and identifiers are owned by `WorldSketchStructure`, not by `SketchStructure`.
- Constraint authoring consumes semantic world curve and point picks, without `SketchPick` or camera state.
- Coincident, horizontal, vertical, parallel, perpendicular, equal, tangent, and fixed world relationships
  have declared subjects; the solver applies the supported line/curve cases directly to world geometry.
- Compatibility constraints and workspace records are mirrored after world application, with mapping and
  revision state preserved.
- World constraint authoring and solving do not rebuild or solve through the compatibility sketch.
- `WorldSketchConstraintProof`, world bridge/interaction proofs, strict compilation, partition checks, and
  host-budget checks remain green.

## World-native driving dimensions

Driving dimensions now use the world authority as well. `WorldSketchStructure` stores
`WorldDimensionName`, semantic world references, and dimension specifications. The camera-free
`WorldSketchDimensionAuthoring` unit validates references and targets, while `WorldSketchDimensionSolver`
measures and drives exact world points, controls, curves, radii, and angles using the curve support frame.

World dimension placement resolves its snapped compatibility names back through `WorldSketchMapping`,
declares the world dimension first, applies it to world geometry, and then mirrors a compatibility dimension
for the existing workspace record surface. Text edits follow the same route: the world target is changed and
solved first, the compatibility geometry and target are refreshed second, and one revision records the edit.

### World-dimension acceptance
- Dimension specifications and identifiers are stored and resolved by `WorldSketchStructure`.
- Aligned, horizontal, vertical, radius, diameter, and angle dimensions measure and drive world geometry.
- Support-frame coordinates remain stable when a world dimension changes an endpoint or round control.
- Dimension placement and text editing do not solve through `SketchStructure`.
- Compatibility dimensions, mappings, workspace records, and revisions remain available after world edits.
- `WorldSketchDimensionProof`, world placement/bridge proofs, strict compilation, and host-budget checks remain green.

## World-native transaction boundaries

World-backed constraint and dimension commits now use a single cross-structure transaction. The world
relationship or dimension is declared and solved first, but compatibility geometry, mirrored specification,
workspace record, name allocation, pending selection, and revision history are restored if any later handoff
fails. A mapped world dimension is never retried through the legacy dimension solver after an invalid or
unsupported world edit.

### Transaction acceptance
- Failed world constraint commits restore world geometry, compatibility geometry, mappings, records, names,
  revisions, and pending selection.
- Failed world dimension text edits restore both targets, world geometry, compatibility geometry, and
  revision history.
- Failed world geometry placement restores partial legacy-bootstrap imports and does not consume records,
  names, revisions, or selection.
- World-backed dimension placement refuses on world failure rather than routing through compatibility
  placement or the legacy dimension solver.
- Successful world-first commits still mirror compatibility state and seal exactly one revision.
- Rollback coverage remains green in the world bridge and placement proofs.

## Host partition cleanup

The combined editor now delegates revision snapshots and viewport-look input arbitration to
`SlateWorkspace/Discipline/SketchRevisionHistory` and `ViewportLookInput`. `EditorHost` retains only
lifetime, panel seating, input sampling, and tick orchestration; it no longer defines engine-mechanism
helpers for revision history or keyboard filtering.

### Host-partition acceptance
- `VerifyHostPartition.py` reports zero host definitions, engine-mechanism helpers, and preprocessor-gated
  logic.
- Revision snapshots restore world sketch, compatibility sketch, mappings, records, workplanes, names,
  revisions, and semantic selection as one state.
- WASDEQ filtering remains active only while the viewport look gesture owns the input.
- The editor combined-authoring translation and workspace helpers compile strictly.
