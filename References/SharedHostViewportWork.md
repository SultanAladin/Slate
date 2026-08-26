# Shared host viewport work

Required order. Editor is the superset. Paint and any sketch host reuse the same
viewport, sun, lattice, menus, and gizmo. No substitute renderer.

## Done on this checkout

1. Keep the analytic lattice at a fixed world pose when drawers open.
   Drawers may occlude the leaf by scissor; they must not hide the whole
   lattice or change cell size, fade radius, or frustum tangents.
2. Overlay always records. Interface submits Beneath lists, then overlay,
   then the foreground chrome list (drawers and deferred menus).
3. Drive overlay projection from the viewport Persp / Ortho footer toggle.
4. Restrict the debug shading menu to lit, source wire, and triangulated wire.
5. Viewport gizmo uses `78` screen-space proportions (axis fraction of leaf
   height) at the taken entity origin.

## Remaining

- One shared host path selected with host compile definitions.
- White Tea Service activation: scene rows, one White Dielectric layer,
  geometry residency, truthful console. No WhiteTea sources on this tree.
- Shared CAD authoring, snapping, previews, and packet presentation in Editor.
  No Parametric sources on this tree.
