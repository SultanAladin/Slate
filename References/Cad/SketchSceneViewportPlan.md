# Sketch Scene Viewport and Editor Panel Integration Plan

## Source-of-truth decisions

- `PaintHost` is standalone for paint-focused documents such as `.pigment`.
- `ParametricSketchHost` is standalone for sketch-focused documents such as `.sketch` and sketch-contained scene references.
- `EditorHost` is the combined host. It may expose paint and sketch panels together when the opened document/profile supports them.
- `WorkspaceCodex` (`.codex`) is the general binary workspace container.
- `SketchCodex` (`.sketch`) carries constraint sketches and construction geometry.
- `PigmentCodex` (`.pigment`) carries paint layers and paint-channel content.
- Do not put texture-paint layer-stack logic in `ParametricSketchHost`.
- Do not put CAD/sketch logic in `PaintHost`.
- Use compile/profile gates for combined-editor composition once host roles are split further.

## Panel naming and hierarchy

The UI has two separate hierarchies:

1. `Scene Directory`
   - scene-facing hierarchy;
   - objects, lights, cameras, folders, imported mesh placements, and references to CAD records;
   - formerly shown as `Outliner`.

2. `Sketch Directory` / `Parametric Directory`
   - exact CAD/sketch hierarchy;
   - points, curves, closed profiles, surfaces, solids, dimensions, constraints, operations, and CAD folders;
   - backed by `WorkspaceRecordStructure` and `WorkspaceDirectoryProjection`.

The construction catalogue panel should be exposed as:

- standalone sketch host header: `Construction Catalogue` where appropriate;
- editor panel/dropdown title: `Parametric Tools`.

Panel dropdown target names:

- `Scene Directory`
- `Sketch Directory`
- `Parametric Tools`
- `Layer Stack` only for paint-capable/editor contexts
- `3D Viewport`
- `UV Editor` where still applicable

## CAD references in Scene Directory

CAD records are not duplicated into the scene hierarchy.
Scene Directory rows for CAD-owned geometry are references back to `WorkspaceRecordName`.

Initial automatic references:

- `ClosedProfile`
- `ThinSurface`
- `Solid`

Do not automatically reference:

- `Point`
- `OpenCurve`
- `Dimension`
- `Constraint`
- `Pattern`
- `Mirror`
- CAD-only folders

Reference behavior:

- rename from Sketch Directory updates Scene Directory projection;
- rename from Scene Directory routes to the CAD record;
- visibility and lock should route to the authoritative CAD record first;
- viewport edits of CAD references route through exact sketch/CAD editing paths.

## Imported mesh-file scene placement

There are no standalone OBJ scene files in `EngineContent` for the current target.
The current visible scene proof is `WhiteTeaService.codex`, a binary workspace container.

Rules:

- do not expose fake catalogue entries such as `Hangar_Interior.fbx`;
- remove hardcoded reference/demo entries from the runtime content catalogue;
- keep scene-loadable runtime content focused on `WhiteTeaService.codex` until real files are added;
- keep source OBJ generation files out of `EngineContent` after the codex proof exists;
- scratch/generated interchange sources may live outside `EngineContent` when needed by agent tooling.

## Unified viewport rule

There is one viewport/camera space in the sketch host and in the sketch-capable editor layout.
It renders, in order:

1. scene polygon geometry from the opened binary workspace/profiles;
2. CAD/sketch geometry;
3. grid, guides, selection, and gizmo overlay;
4. UI shell, drawers, and deferred popups.

Implementation may use specialized internal passes inside the same viewport/frame:

- scene mesh pass;
- CAD pass;
- overlay pass.

The forbidden architecture is separate CAD and scene viewports/worlds for the same sketch workspace.

## Implementation task list

### Phase 1 — panel names and Sketch Directory subject

Status: started in implementation.

- Rename the displayed `Outliner` panel to `Scene Directory`.
- Rename the displayed `Construction` panel to `Parametric Tools`.
- Add `PanelSubject::SketchDirectory`.
- Add chooser/dropdown entries for `Sketch Directory`.
- In `ParametricSketchHost`, make the default left panel `SketchDirectory` instead of `Outliner`.
- Route `SketchDirectory` to `ParametricWorkspacePanel`.
- Reserve `Outliner`/`Scene Directory` for scene-reference rows.

### Phase 2 — editor host sketch panels

Status: started in implementation with editor dropdown routing and empty-panel hosting.

- Add `ParametricWorkspacePanel` and `ParametricToolsPanel` to `EditorHost`.
- Add sketch directory and tool contexts to `EditorHost`.
- Route editor leaves:
  - `Scene Directory` -> `SceneDirectoryPanel`;
  - `Sketch Directory` -> `ParametricWorkspacePanel`;
  - `Parametric Tools` -> `ParametricToolsPanel`.
- Keep paint-specific `Layer Stack` routing out of `ParametricSketchHost`.

### Phase 3 — runtime content catalogue cleanup

- Remove fake hardcoded entries from `ContentBrowserReferenceCatalog.inc`.
- Expose `WhiteTeaService.codex` as the scene-loadable runtime proof.
- Stop advertising OBJ scene loading in the sketch host until a real mesh-placement path exists.
- Keep the binary `.codex` as the shipped proof.

### Phase 4 — Sketch-scene references

- Add host-side scene reference records for sketch workspaces.
- Project renderable CAD records into Scene Directory as references.
- Keep exact CAD ownership in `WorkspaceRecordStructure`.

### Phase 5 — binary scene loading into unified viewport

- Load `WhiteTeaService.codex` through the existing codex activation path.
- Register scene rows in Scene Directory.
- Render codex scene geometry and CAD geometry in the same viewport/camera path.

### Phase 6 — selection sync

- Scene Directory CAD reference selection selects the underlying CAD record.
- Sketch Directory CAD selection highlights matching Scene Directory references when present.
- Scene geometry selection selects Scene Directory rows.
- CAD point/control selection remains exact and CPU-authoritative.
