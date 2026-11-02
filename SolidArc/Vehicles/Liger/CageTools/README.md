# Project Liger — native curve tools

## Open your project

Extract the **entire ZIP**. Keep its single **Project Liger** folder together. On Windows run **Start Liger.bat**; on Linux run **Start Liger.sh** (a Vulkan graphics driver and X11 desktop are required). The Runtime folder includes the executable, all 310 SVG icons and the packaged fonts/notices. The adjacent project also opens through the executable's installed workspace settings.

`Liger.solidarc` is a byte-for-byte copy of your supplied `Liger(progress).solidarc`: **3 reference imports, 23 curves, 200 saved authoring controls, 3 existing pole edits, no lofts**. Its SHA-256 is `90344c326b32e467b8298851eb8b1df2a2dba1ce879becd3c7a12303b252ec70`. No joints, offsets, surfaces or wheel arches have been added to your saved cage. Save your own edited version normally.

## Leave surface tracing / draw a wheel arc

1. **Esc** cancels the active drawing gesture without adding geometry.
2. For ordinary planar tools, turn **Surface** off in the bottom toolbar or turn off **Surface snap** in the inspector. You can also turn it off in the small **Drawing options** panel *before placing the first point*.
3. **Arc (3 points)** is always **planar**: it does not drape onto CAD faces, even while the general Surface preference remains enabled for later NURBS/Line tools.
4. Choose a named side view (**Numpad 3**, right side) and use **YZ (side)** in Drawing options. Set **Depth (m)** to the desired X station for that wheel well. XY is the top plane; XZ is the front plane. Planes/depth cannot change halfway through a gesture.
5. Click the arc's start, end, then a point on the arc. It remains an editable analytic arc, not a surface-fitted spline.

**Snap to curves** is a separate automatic authored-curve attraction option. Switch it off for independent small details near existing controls; otherwise endpoints within 8 pixels and curves within 12 pixels may attract the click. In surface tracing, **Ctrl** prioritizes authored curves, then model edges, ahead of a face hit. Ordinary Ctrl sketch snapping remains available. An off-plane arc snap is projected into its chosen plane and labelled accordingly; it does not tilt/drape the arc.

## Whole curves versus spans

- **6 / curved-line rail button:** select an entire curve. Shift-click adds another curve **in click order**. Ctrl-click removes. Box selection and Ctrl+A operate on whole curves in this mode; locked references are not selected.
- **2:** select spline spans / CAD edges.
- **1:** select controls / vertices.
- Switching selection mode clears stale spline-span/control picks. **Alt+A** clears the object selection. If a transform gizmo overlaps a curve, deselect first or choose a point away from the gizmo to pick the curve.

## Keep the second curve X metres from the first

1. In mode **6**, select the **guide first**, then Shift-select the **target**. The target must be an existing native 3D spline, not a planar analytic sketch arc.
2. The inspector's **Live surface distance** card shows the order; **Swap guide / target** reverses it.
3. Choose the correct **Reference** body/sheet. The first locked reference is initially suggested; select another if the guide belongs to a different sheet.
4. Enter **Along surface (m)** and **Tolerance (m)**. Press **Link / update distance**.
5. The target chooses the desired side and approximate extent. Editing the guide regenerates the linked target; the relation survives save/load and undo/redo. An earlier target can be safely reordered after its guide unless that would introduce a dependency cycle.
6. Select the linked target and **Detach (keep curve)** before editing its resulting controls or spans independently.

This measures **intrinsic distance over a triangle mesh of the selected reference**, not ambient XYZ distance. Triangle-interior fast marching builds an iso-distance contour; progressive remeshing checks bidirectional contour convergence. Output is an editable degree-one NURBS/polyline representation of that mesh contour.

**Accuracy/limits:** this is a numerical mesh approximation, **not an exact analytic CAD-surface offset or a certified error bound against the continuous surface**. Guide points are seeded onto that reference within four times the entered tolerance. Refinement, disconnected/non-manifold contours and resource limits can refuse an operation rather than substitute a straight-line offset. Failed control/whole-curve edits roll back if a live dependent refuses. Target extent anchors follow guide translation through its midpoint; large rotations/bends can need re-linking to choose a new branch/extent. Guide/reference must precede the target; derived-target transformations/control edits require detachment. No loft is created.

## Optional automatic cuts

During NURBS drawing, use **Drawing options → Auto-split 3D crossings**. It is **off by default**. **Joint tolerance (m)** controls genuine 3D proximity, not projected/screen crossings. Finish the curve normally: drawing and splitting are **one undo transaction**.

For the existing cage, use **Split existing cage crossings**. This is an explicit edit; undo restores all original curves. Exact B-spline subcurves retain the original curve shape except that joints within the selected tolerance share their midpoint. Original labels are kept on the first piece. Referenced/linked curves that cannot safely be cut refuse atomically; locked curves are skipped.

Automatic cutting supports native **non-rational 3D splines**. It deliberately does **not** split analytic sketch arcs, overlaps, or tangential contacts. Close screen crossings with more than the chosen XYZ tolerance are not joined. Drawing options can be moved, closed using its X, or hidden/reopened using **Drawing options** in the inspector.

Without an explicit offset, subdivision or enabled splitting, finishing a NURBS curve still saves **only the user-placed controls at their snapped XYZ positions**. Enter does not add fitting points.

## What was found in your rough cage

A top-view numerical inspection found 31 projected crossings: 18 within 1 mm (including already shared endpoints) and **13 with larger 3D separation**. Examples by native curve label:

| Curves | Separation at the top-view crossing |
|---|---:|
| Curve#15 / Curve#18 | 1.906 mm |
| Curve#11 / Curve#13 | 2.664 mm |
| Curve#12 / Curve#13 | 3.797 mm |
| Curve#4 / Curve#9 | 4.551 mm |
| Curve#6 / Curve#7 | 18.659 mm |

A 1 mm-tolerance cut on an in-memory test copy produced **43 pieces**, and undo restored the exact serialized 23-curve document. **The packaged project is still your untouched 23-curve file.** A visual crossing alone does not prove a shared 3D joint; use explicit authored-curve snapping when repairing a point.

See the repository's `CageTools/AUDIT.md`, `Validation` and `Proofs` for the tested scope and limitations. Windows is cross-built; interactive Windows execution is not claimed.
