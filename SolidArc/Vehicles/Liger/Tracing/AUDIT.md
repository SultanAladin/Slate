# Surface tracing, spline editing and component deletion — final audit

Date: 2 November 2026. Incremental to the point-only tracing release `30a57de`.

**Enter still saves only the controls you clicked.** Subdivision is a separate,
explicit command. The protected dense reference and clean factory project are
unchanged; no assistant-authored vehicle geometry is substituted.

| Requested area | Status | Implementation and evidence |
|---|---|---|
| Loft default normals | 🟢 | New open lofts / networks align with the nearest locked reference; without one, open sections use a consistent dominant direction and closed-section tubes use radial outside. Tests cover reversed picking order, persistence, a downward-facing locked reference and outward tube normals. Section order is preserved; existing lofts are not silently reoriented. |
| Curve snapping before surface | 🟢 | Ctrl prioritises authored curves, then visible CAD edges within 12 px, then the surface. Consumed construction profiles do not steal authored-curve priority. Authored overlays remain snappable behind the reference; section clipping is still respected. Native and actual UI checks pass. |
| Delete spline points | 🟢 | Mode 1 → select controls → X/Delete reconnects only the remaining controls. Unselected XYZ values survive. Minimum-control and invalid-selection refusals leave the object untouched. Ordinary sketch spline controls also delete locally. Undo/redo and save/load tested. |
| Select spline spans | 🟢 | Mode 2 picks either a control-cage edge or its corresponding Greville curve interval. Shift adds / Ctrl removes spans on the same spline; selected intervals highlight. Endpoint controls support transforms. Actual selection and deletion/subdivision tested. |
| Subdivide | 🟢 | Ctrl+Shift+D or Feature → Subdivide inserts exact NURBS knots. Selected spans, adjacent selected controls, or the whole selected spline are supported. Native 1,001-sample comparisons verify shape preservation, including after XYZ pole edits; undo/redo and persistence pass. |
| Delete only selected spans | 🟢 | Exact cropping retains the unselected intervals, splitting disconnected pieces when needed. Native tests compare both retained pieces against the original curve. Dependent-feature references are protected by an atomic refusal. |
| CAD face/edge/vertex deletion | 🟢 | Component selection is handled before parent selection or history focus, with **no parent-deletion fallback**. Actual box face deletion leaves one five-face sheet; undo restores six faces. Actual body-vertex and edge picks test safe local-dissolve refusals and retain the six-face parent. Arbitrary invalid CAD corner deletion is not claimed. |
| Gizmo regression | 🟢 | Component hit targets take pointer priority without suppressing whole-curve gizmos. W81 retains **960/960** planar/view cases and **34/34** SpaceCurve/pole cases. Numeric XYZ editing, continued lofts and all prior CAD gates pass. |
| Point-only authoring | 🟢 | All **92** existing native tracing checks pass, including exact placed-control counts/XYZ and no automatic span projection. Prior actual drawing, forms, closure, XYZ, undo/redo/save and quad-view scripts pass. |
| New native suite | 🟢 | **50 editing checks + 1 exported topology fixture = 51**. Includes exact subdivision/cropping, deletion/persistence, normals, curve priority and section clipping. `Validation/NativeEditing.txt`. |
| Actual Vulkan interaction | 🟢 | **115/115 expectations**: Editing 19, TopologyDelete 18, Startup 11, Drawing 35, Edge 6, Quad 9, Control 17. Linux/SwiftShader, 1280×680, complete Runtime resources and paths containing spaces. Final reports and screenshots included. |
| CAD / Python / viewport | 🟢 | **95 CAD gates**, focused W81/W82 rerun, Vulkan viewport regression and **40 Python tests** with native certifier integration. Original STEP fixtures are used, not missing-file substitutes. |
| Rebuilt packages | 🟢 | Linux X11 and Windows x64 binaries rebuilt/installed. Each ZIP contains one `Project Liger` folder, project, guide, one launcher and complete resources: **310 SVGs / 66 SolidArc icons / 5 fonts**, notices included. Runtime SHA, executable architecture/dependencies, all ZIP CRCs and payload bytes checked. |
| Reproducible source delivery | 🟢 | `SolidArc-Spline-Editing.patch` applies after the tracing and point-only patches. Forward/reverse applicability and byte equality with the compiled Host mirrors are checked. Existing ISO-boundary and precision importer patches are both verified present. |
| Windows interactive execution | ⚠️ | Cross-build, PE/dependencies, resources, launcher and ZIP integrity checked. No Windows/Wine runtime is available here; **Windows interactive execution is not claimed**. |

## Usage and limits

- **1**: spline controls / body vertices. **2**: spline spans / CAD edges.
  **3**: faces. **4**: whole bodies. **5**: general selection.
- **X/Delete** acts on selected components first. Invalid CAD dissolves refuse
  rather than deleting the owner. Use object/general mode for whole objects.
- **Ctrl+Shift+D**: explicit, shape-preserving subdivision. Deleting a control
  intentionally reshapes/reconnects the remaining controls; subdivision does not.
- Intermediate NURBS/Bezier controls need not lie on the curve. A span is the
  interval between adjacent Greville parameters, not an interpolating segment.
- Newly drawn curves remain ordinary editable curves, not live shrinkwrapped
  curves. Spans and later edits may leave the reference. No whole-span fitting
  or automatic extra poles has been reintroduced.
- Open sheets have no unique mathematical outside. The default orientation uses
  the reference/canonical rules above; **Alt+N** can still flip a sheet explicitly.
- The factory project remains **three locked open reference sheets, 306 faces /
  906 edges / 595 vertices / 40,160 poles**, with original defects. This is not a
  repaired watertight vehicle. Registration remains nose −Y / right +X / up +Z.

Extract into a **new folder** to protect your work. On Windows use **Start
Liger.bat**; the standalone EXE alone opens a blank workspace. Linux requires
Vulkan and an X11 desktop; use `bash "Start Liger.sh"`.

## Evidence and provenance

`Validation/BuildManifest.json` records current runtime hashes, dependencies,
patch hashes and test counts. `Validation/{NativeEditing,NativeTracing,Editing,
TopologyDelete,Startup,Drawing,Edge,Quad,Control,GizmoLoftRegression,
DocumentRegression,ViewportRegression,PythonTests,PatchValidation,Packaging}.txt`
contains the checks. `Proofs` includes current-build point deletion, selected
span, span deletion, authored-curve snap and box-component captures/snapshots,
as well as refreshed prior workspace/drawing/closure/edge/quad proofs.

`Validation/Drawing1600.txt` is retained historical evidence from the previous
release, **not counted** in this release's 115 expectations. The topology fixture
hides its consumed construction profile so mode 1 tests a real CAD vertex, not
an overlapping sketch point. Command refusals do not add refused feature steps;
the topology script checks both the refusal status and unchanged body/faces.

Desktop host: `SultanAladin/Frontier` `65cf0f131a6afe8bdcd722c006b10cfab0e1aafd`.
Geometry base: `unassignedinbox/Frontier`
`82ed251267ed8457ca3530d8228b91555572c25d`, with the existing overlays/importer
patches, then `SolidArc-Surface-Tracing.patch`,
`SolidArc-Surface-Point-Only.patch`, and `SolidArc-Spline-Editing.patch`.
Linux: GCC 12.2 / X11. Windows: Zig 0.14.1, x86_64-windows-gnu, static C++ runtime.
Final incremental build/install logs are included. `SHA256SUMS` covers the final
packages and audit artifacts.
