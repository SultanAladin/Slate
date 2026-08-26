# White Tea Editor Integration Plan

## 1. Establish the source baseline

- Work from the checked-out Slate source and record the active commit.
- Trace `EditorHost`, `PaintHost`, and `ParametricSketchHost` before changing host composition.
- Trace Codex activation, Scene Directory registration, texture layer registration, geometry intake,
  atmosphere, grid, drawers, previews, and gizmo paths.
- Treat existing Slate implementations and `References/Gizmo.html` as authoritative.
- Do not create substitute shaders, suns, controls, drawers, overlays, gizmos, or mock scene content.

## 2. Unify host composition

- Move common host composition into one shared application path.
- Use the requested host compile definitions for `PaintHost`, `EditorHost`, and `ParametricSketcher`.
- Keep `EditorHost` as the complete surface.
- Keep `PaintHost` and `ParametricSketchHost` as standalone feature selections of the same path.
- Remove `ParametricWorkspaceBridge` and CAD-specific duplicate control paths only after their responsibilities
  have been migrated and verified in the shared path.

## 3. Make WhiteTeaService Codex activation atomic

- Open and decode `EngineContent/WhiteTeaService.codex`.
- Resolve and validate all environment entries and all six geometry entries.
- Resolve and validate `EngineContent/MaterialArchives/WhiteDielectric.pigment`.
- Register the actual scene entries in Scene Directory.
- Register exactly one decoded White Dielectric material layer in the shared layer stack.
- Feed every activated geometry entry into the existing geometry intake and rendering path.
- Commit scene rows, material layer, and geometry only as one successful activation.
- Print success only after the geometry is available to viewport submission.
- Print a precise refusal when decoding, material validation, geometry intake, residency, or submission fails.

## 4. Reuse authoritative rendering

- Remove or bypass PaintHost-only fake sun and shader paths.
- Route every host through the existing atmosphere presentation, sun, grid, overlay, material, and geometry
  implementations.
- Keep the existing White Dielectric material as the first render material.

## 5. Preserve the grid through drawer interaction

- Capture the viewport leaf extent independently of drawer interiors.
- Keep grid world scale and projection unchanged while Control Centre or Content Browser opens.
- Use drawer scissoring and occlusion instead of resizing or rebuilding the grid.
- Never suppress the grid merely because a drawer is open.

## 6. Make parametric authoring top-plane based

- Resolve parametric pointer input against the top/grid plane in both perspective and orthographic viewing.
- Keep camera projection separate from sketch authoring projection.
- Draw active parametric geometry on the same grid plane used by completed geometry.

## 7. Unify the reference gizmo

- Use the exact geometry, colours, proportions, and handle arrangement from `References/Gizmo.html`.
- Share one gizmo renderer and hit-test path across all hosts.
- Select translate, rotate, or scale handles from the active tool.
- Keep gizmo size in screen space with a fixed pixel range and no world-distance expansion.

## 8. Preserve completed geometry and live previews

- Trace the disappearance threshold and remove any fixed-count, stale-reference, or per-frame clearing defect.
- Keep completed shapes in authoritative sketch/workspace records.
- Render a live preview for every supported drawing tool using the same geometry path as completion.
- Keep preview geometry transient until confirmation.

## 9. Debug scene presentation and camera

- Keep the White Tea Service as the only scene content rendered by the debug viewport.
- Use the existing viewport debug menu and existing `WorkspaceScenePass`; do not add another scene shader.
- Expose only these temporary debug modes: `Lit` (default), `SourceWire`, and `TriangulatedWire`.
- Ensure the default mode is `Lit`; source-wire and triangulated-wire are opt-in selections only.
- Use the existing editor camera from Scene Directory and scene settings for the scene projection.
- Keep the camera row and its authored pose connected to the same camera component used for rendering.

## 10. Verification

- Search the final source for duplicate host shaders, suns, grids, controls, drawers, overlays, and gizmos.
- Verify Codex activation commits Scene Directory, one material layer, and geometry intake together.
- Verify the viewport visibly contains the White Tea Service, atmosphere, sun, and grid.
- Verify drawer opening does not resize or remove the grid.
- Verify sketch authoring in both camera modes.
- Verify translate, rotate, and scale gizmos are screen-space sized and mode-correct.
- Verify completed shapes remain visible beyond the previous disappearance threshold.
- Verify live previews for every existing drawing tool.
- Ask before running the repository build or validation executables.
