# Shared host viewport work

Required order. Editor is the superset. Paint and any sketch host reuse the same
viewport, sun, lattice, menus, and gizmo. No substitute renderer.

## Done on this checkout

1. Keep the analytic lattice at a fixed world pose when drawers open.
   Drawers may occlude the leaf; they must not hide the whole lattice or
   change cell size, fade radius, or frustum tangents.
2. Withhold the GPU overlay only while a footer or chrome menu stands, so
   lattice lines cannot paint through the menu.
3. Drive overlay projection from the viewport Persp / Ortho footer toggle.
4. Restrict the debug shading menu to lit, source wire, and triangulated wire.

## Remaining

- One shared host path selected with host compile definitions.
- White Tea Service activation: scene rows, one White Dielectric layer,
  geometry residency, truthful console.
- Shared CAD authoring, snapping, previews, and packet presentation in Editor.
- Reference gizmo from `References/Gizmo.html` as the only transform gizmo.
