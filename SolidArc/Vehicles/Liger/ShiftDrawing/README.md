# Project Liger — hold Shift for straight drawing

## How to use it

After placing the first point, **hold Shift while placing the next point**:

- **Line / Polyline:** the next segment follows the nearest horizontal or vertical axis of the active drawing plane, measured from the **previous click**. A mostly vertical move keeps the first plane coordinate exactly equal; a mostly horizontal move keeps the second equal. Each polyline segment chooses its own axis.
- **Arc:** hold Shift for the **second endpoint** to align the two ends. The third/bulge point stays free even while Shift is held, so the arc does not collapse. For vertically aligned ends this gives the same X on an XY sketch; side/front views use their actual sketch-plane coordinates.
- **Slot:** Shift aligns the two cap centres; the width point stays free.

A coloured dashed guide and **Shift straight** label show the current axis. Release Shift to draw freely again; no sticky lock is created. This is shared placement for ordinary drawing, click-click / click-drag, and Knife / Imprint line profiles, in orthographic and perspective views. A face-attached sketch uses its face plane rather than assuming world XY.

**Freeform curves remain free.** Rectangle diagonals, circle radii, solid sizing, Shift fine-adjustments and Shift-click selection outside drawing are unchanged. Explicit X/Y/Z/tangent/normal/binormal locks take precedence. Shift overrides an incompatible automatic/Ctrl point snap; release Shift to use that snap instead.

For a **surface-traced Line**, Shift chooses the nearest world X/Y/Z axis through the last clicked 3D point. A true straight axis may leave a curved surface; the HUD explicitly says so. It does not silently reproject the result or add controls. Arcs remain planar, not surface-snapped.

## Open your project

Extract the full ZIP and keep its single **Project Liger** folder together. Windows: run **Start Liger.bat**. Linux: run **Start Liger.sh**. The Runtime folder includes the rebuilt app, all 310 SVG icons, 5 fonts and dependency notices. Windows needs a compatible Vulkan driver; Linux needs X11 and Vulkan.

Both packages include your **unchanged** `Liger(progress).solidarc`, named `Liger.solidarc`: 3 reference imports, 23 curves, 200 saved controls and 3 pre-existing pole edits. **No lofts, surface patches or replacement vehicle geometry were created.** SHA-256: `90344c326b32e467b8298851eb8b1df2a2dba1ce879becd3c7a12303b252ec70`.

The surface-attached inward cutter fix, global Shift-click multi-curve selection, Outliner whole-curve Delete, component-deletion safety and live surface-distance links are retained. Use **6** or an Outliner row for an explicit whole curve.

See [AUDIT.md](AUDIT.md) in the repository for the emoji completion matrix, fresh test results, native proof captures, source provenance and package hashes. Windows is cross-built and package-checked; interactive Windows execution is not claimed.
