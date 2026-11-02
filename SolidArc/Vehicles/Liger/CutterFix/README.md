# Project Liger — surface-attached cutters

## What changed

**Knife cutters now attach to the first-clicked surface in orthographic and perspective views**, independently of the normal drawing tool's **Orient to faces** preference. A planar face supplies its plane; a curved face supplies its tangent plane at the hit. The outline stays in that plane after the first click. Cut depth runs **inward** along its normal, not from an unrelated world-grid plane.

The general **Surface tracing** preference no longer blocks Rectangle/Circle/Polygon cutters or turns a cutter line into a retopology trace. Your tracing and face-orientation preferences are preserved when you leave Knife. A previous drawing's temporary axis lock does not pin a new cutter to its old grid.

## One cutter workflow, without duplicate basic shapes

Open **Knife (Shift+K)** or its bottom-toolbar dropdown:

- **Surface cutters:** Rectangle, Circle, Polygon, Slot, Line and Polyline. Draw the outline on the surface, then choose depth. Rectangle + depth is the box cutter; Circle + depth is the cylindrical cutter; Polygon + depth is the prismatic cutter. The duplicate Block / Cylinder / Prism entries have been removed from both cutter menus, **not** from the ordinary Construct tools.
- **Shaped cutters:** Cone, Sphere, Torus, Wedge and Tube retain their distinct geometry, with the same automatic surface placement.
- **Imprint only:** splits a face without removing volume. This is intentionally separate from a pocket/cut.
- **Objects / Boolean library:** existing-object cuts and multi-stage library cutters remain available, including their live/editable workflows. If you need to keep a standalone box/cylinder/prism cutter body, make it with ordinary Construct and use Objects / Keep cutter; the standard outline + depth entry does not expose the old primitive-only keep-body option.

For a normal pocket: choose **Rectangle**, click its first corner on the surface, then the opposite corner. Move along the face normal or type a depth, then **Enter** to apply. **W** cuts inward, **Q** adds outward, **T** cuts through. **Esc** cancels; **Ctrl+Z** undoes. Keep the target visible so the first click actually hits its surface; an empty-space start retains the existing grid fallback. Locked geometry remains protected.

On a curved surface this is a **planar tangent-plane cutter extruded along one fixed normal**, not a conformal profile draped over the surface or a constant-depth surface offset. The Boolean removes the intersecting volume.

## Open your project

Extract the entire ZIP and keep its single **Project Liger** folder together. On Windows run **Start Liger.bat**; on Linux run **Start Liger.sh**. The Runtime folder contains the rebuilt native app, full SVG icons, fonts and notices. Linux requires X11 and Vulkan; Windows requires a compatible Vulkan driver.

`Liger.solidarc` is a byte-identical copy of your supplied `Liger(progress).solidarc`: **3 reference imports, 23 curves, 200 saved controls, 3 existing pole edits, no lofts or added replacement geometry**. SHA-256: `90344c326b32e467b8298851eb8b1df2a2dba1ce879becd3c7a12303b252ec70`. Tests modify temporary fixtures/copies only.

The previous global Shift-selection and Outliner Delete fixes are included. Same-owner component selection/deletion remains local; use **6** or an Outliner row for an explicit whole curve. Your saved project is not automatically lofted or repaired.

See **CutterFix/AUDIT.md** in the repository for the completion matrix, fresh test logs, proof captures and build hashes. Windows is cross-built; interactive Windows execution is not claimed.
