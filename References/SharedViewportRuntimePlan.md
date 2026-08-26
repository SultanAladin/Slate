# Shared Viewport Runtime Plan

## Goal

Make Editor Host, Paint Host, and Parametric Sketch Host use one shared viewport and CAD runtime without merging their standalone entry points or panel-specific features.

## Acceptance criteria

- Editor Host can author the same CAD sketch records as Parametric Sketch Host.
- Paint Host contains no local placeholder grid or decorative sun renderer.
- All hosts use the shared camera frame and projection law.
- The footer perspective/orthographic control drives the active viewport projection.
- Parametric authoring, snapping, grid, completed profiles, overlays, and CAD packet rendering remain on the sketch grid plane.
- Scene/Codex geometry remains rendered through the current scene camera.
- The shared reference orientation gizmo is the only orientation gizmo path.
- Completed CAD geometry is not silently truncated by a low packet capacity.
- Existing panel ordering, popup clipping, drawer behavior, and standalone host entry points remain unchanged.

## Implementation stages

1. **Shared viewport contract**
   - Keep camera pose, projection selection, viewport frame, and world-to-screen conversion in `SharedViewportHostBridge`.
   - Add a common camera-state adapter so Editor and Parametric views cannot drift in yaw, pitch, position, field of view, or projection mode.
   - Route Paint and Editor scene projection through that contract.

2. **Shared CAD workspace state**
   - Extract sketch structure, workspace records, revisions, draft state, selection, and transform state from the Parametric Sketch host into a reusable feature/runtime unit.
   - Keep host-specific panels and entry functions outside the runtime.
   - Status: complete in `SharedCadWorkspaceRuntime.h`; both hosts include the contract.

3. **Shared authoring dispatch**
   - Introduce one dispatch entry point that receives the active tool, pointer, modifiers, grid-plane projection, and shared runtime.
   - Move the existing Parametric Sketch dispatch behind that entry point rather than creating a second Editor implementation.
   - Connect the same entry point from Editor Host and Parametric Sketch Host.
   - Status: in progress; the shared entry point now handles line, polyline, rectangle, circle, three-point arc, center/start/end arc, tangent arc, elliptical arc, Bézier, basis spline, rational spline, Hermite, polygon, and slot records, plus dimension and constraint paths when resolved references are supplied. Grid snapping is now shared by Parametric Sketch and Editor; entity/tangent/intersection snapping and preview rendering still need extraction from the Parametric host.
   - Move grid-plane pointer conversion, snapping, draft preview, commit, selection, and transform dispatch into the shared runtime.
   - Enable the runtime in Editor Host through its existing CAD host feature definition.
   - Preserve Paint Host’s non-CAD feature boundary.

4. **Shared presentation**
   - Move CAD packet projection, profile overlays, constraint glyphs, and validation overlays behind the shared runtime.
   - Use one camera projection mode for both completed and preview CAD geometry.
   - Keep scene/Codex rendering separate and camera-based.

5. **Remove duplicated paths**
   - Delete obsolete host-local camera projection, grid, sun, CAD dispatch, and gizmo render paths only after their shared replacements are wired.
   - Verify source has no duplicate placeholder grid/sun renderer and no custom orientation gizmo.

6. **Validation**
   - Run `git diff --check`.
   - Run the repository sandbox sequence after restoring required submodules/toolchain where available.
   - Run focused source checks for Editor drawing, Paint projection toggling, Parametric top-plane authoring, drawer opening, popup opacity, and high-count CAD packet retention.
