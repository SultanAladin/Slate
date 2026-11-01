# Liger — targeted upper-surround, sill and arch refinement

**01 November 2026 · 306 cubic NURBS faces · based on the previous `Faired` review (`5ba6188`).**

The upper hoop/section is the **canopy or roof surround**. The model also has a separate **roof/glass-frame insert** under the outer cover. These are not all the same CAD part.

This revision addresses several causes of the highlighted roughness: narrow strip interfaces, local curvature/parameterization problems, a slightly protruding insert, and non-conforming display sampling. **Not every remaining tight arch return has been rebuilt.**

## Download / inspect

- **[Native SolidArc document](CAD/Liger_Refined_CAD.solidarc)** · **[combined STEP](CAD/Liger_Refined_CAD.step)**.
- [CAD + static proofs ZIP](Liger-Refined-CAD-and-Proofs.zip). The ZIP does not include the interactive viewer or a native application executable.
- Live/local comparison: from the parent **Liger** directory run `python Tools/serve_refined.py 8080`. Open the live preview, or `http://localhost:8080/` on your own machine.
- Viewer: shaded, zebra, patch layout, source distance and shape change; before/after toggle; **Upper surround / Sill / Front arch** focus buttons; part visibility, source overlay, picking and downloads.
- Downloads always contain **AFTER**. The before toggle displays the **previous faired CAD**, not the first conversion.

### Matched proofs

| Region | Before | After |
|---|---|---|
| Whole body | [Shaded](Proofs/01_Before_Shaded.png) / [zebra](Proofs/03_Before_Zebra.png) | [Shaded](Proofs/02_After_Shaded.png) / [zebra](Proofs/04_After_Zebra.png) |
| Upper surround | [Zebra](Proofs/05_Upper_Before.png) | [Zebra](Proofs/06_Upper_After.png) |
| Sill / lower side | [Shaded](Proofs/07_Sill_Before.png) | [Shaded](Proofs/08_Sill_After.png) |
| Front arch | [Zebra](Proofs/09_Arch_Before.png) | [Zebra](Proofs/10_Arch_After.png) |
| Canopy insert protrusion | [Shaded close-up](Proofs/14_Canopy_Before.png) | [After 1 mm correction](Proofs/15_Canopy_After.png) |

Also: [shape-change map](Proofs/11_Change_Map.png), [new patch layout](Proofs/12_Patch_Layout.png), [browser screenshot](Proofs/13_Interactive_Inspector.png).

**Both sides of these comparisons use the same conforming shared-edge tessellation, lighting and analytic NURBS normals.** Thus the CAD comparison does not count the display-mesh repair as a surface-design improvement. These are computed renders/browser inspection, not AI-generated pictures or native GUI screenshots.

## Actual CAD changes

IDs are zero-based and retain their original meanings in the inspector/STEP labels.

1. **Upper main-shell surround: merge patches 59 + 84 → 59.** Remove the narrow internal strip interface; fit one fair interior to the retained outer rail curves.
2. **Sill transition: merge 1 + 10 → 1.** Remove another narrow ridge/strip interface.
3. **Lower-side transition: merge 3 + 8 → 3.** Replace the two-chart interior with a boundary-preserving 3D Coons surface, retaining the wrapped return rather than incorrectly forcing it to be a planar height graph.
4. **Front-arch patches 27 and 42:** bounded physical-curvature fairing, with fixed rim poles and a penalty on changes to the original rim tangent planes. Pole components move by at most 2 mm; actual sampled shape changes are much smaller.
5. **Roof/glass-frame insert:** translate the entire part **1 mm downward**, including all its surface nets and B-rep curves. Its shape is unchanged. This is an assembly-clearance correction, not smoothing or hiding the part.

Three internal interfaces per half disappear: **312 → 306 faces**, **912 → 906 edges** overall. All **main-shell exterior rail curves** are retained; no new perimeter curve is fitted. The front cowl is geometrically unchanged. The insert's outlines intentionally move down 1 mm with the part.

This is not a universal pole-count reduction: compatibility with the preserved curved rims gives **40,160 poles**, versus 39,292 previously. The improvement is removal of problematic internal interfaces and fairer geometry, not a claim that fewer poles always produce a better surface.

### Shape change versus previous faired CAD

| Edited main-shell region | RMS mm | p95 mm | Sampled max mm |
|---|---:|---:|---:|
| Upper surround 59 + 84 | 0.244 | 0.578 | 0.869 |
| Sill 1 + 10 | 0.216 | 0.444 | 0.884 |
| Lower side 3 + 8 | 1.152 | 2.702 | **4.467** |
| Front arch 27 | 0.057 | 0.138 | 0.196 |
| Front arch 42 | 0.142 | 0.303 | 0.389 |

Distances are **bidirectional samples to 151×151 triangulated old/new regions**, using the union of both old surfaces for a merge. They are not certified Hausdorff bounds and not same-UV displacement. The insert has a separately verified **exact 1 mm rigid translation**. These numbers are relative to `5ba6188`, not cumulative bounds from the original Blender source.

### Insert protrusion / clearance

At 10,201 corresponding samples on the main insert half-patch (frame patch 0; the other side is mirrored), vertical rays against the triangulated main skin found the insert as much as **0.486 mm above** the skin before correction. After the 1 mm move the minimum sampled vertical clearance is **0.514 mm**.

This explains many of the dark fragments on the cover: a second part was poking through, not simply a high patch count. The check is **sampled vertical clearance**, not a global body-body collision certificate, and it does not certify every other insert/frame piece against every vehicle component.

## Display repair — separate from CAD repair

The old proof grids sampled neighbouring face boundaries independently. A valid shared NURBS rail could therefore have different display chords on its two owners. The new tessellator samples each canonical rail once, uses those samples on every owner, and constrained-triangulates each rectangular UV domain.

- **0 mm difference between shared display-boundary positions**, before and after.
- All constrained boundary segments checked; surface-to-rail agreement checked **before** canonical boundary assignment.
- Analytic normals come from each actual NURBS net, not averaged/smoothed mesh normals.
- The intentionally detached rear branch is still detached and is not falsely counted as a sewn interface.
- Matching positions do not imply matching tangent planes or globally smooth reflections.

## Audit / completeness

| Check | Status | Result / limitation |
|---|:---:|---|
| Real NURBS and B-rep exported | 🟢 | 306 faces in 3 open sheet parts |
| Strip simplification | 🟢 | 3 half-body internal interfaces removed, 6 mirrored faces removed |
| Main-shell outer rails | 🟢 | Unchanged geometry; all remaining rails agree with the baseline |
| Insert shape / placement | 🟢 | Shape retained; exactly 1 mm downward translation |
| Native regeneration | 🟢 | **3 bodies, 0 refused** |
| Native body audits | 🟢 | All sound; no non-manifold edges |
| STEP precision, corrected importer | 🟢 | **0 tolerant edges / 0 tolerant vertices**; largest reported residual 5.59×10⁻⁹ mm |
| Real-gap negative control | 🟢 | Deliberate off-surface rail still detected: 0.443 mm, 1 tolerant edge |
| Python / native regressions | 🟢 | **24 tests passed**, including native real-gap control |
| Browser inspection | 🟢 | Both gzip models, 5 modes, revision toggle, zone focus, picking, downloads, mobile; zero JS/shader errors or failed HTTP responses |
| Existing deliverables retained | 🟢 | Both original and previous-faired checksum sets verify |
| Every highlighted tight return fully rebuilt | 🟡 | **No.** Trailing lower wheel-arch returns, notably patches 34 / 46, remain; unsafe rebuild trials were rejected |
| All seam angles improved / G1 / G2 / Class-A | 🟡 | **Not claimed.** Some normal jumps decrease greatly; some increase slightly; residual creases/pinching remain |
| Closed manufacturing solid / global collision proof | 🟡 | **No** — exterior sheets only, with original open boundaries/detached rear branch |
| Standalone GUI binaries | 🟡 | **Not rebuilt**; see importer compatibility below |

The result is an improved, inspectable review—not a claim that all red-marked regions are now perfect. The very tight trailing arch return needs a more deliberate boundary/profile redesign to finish cleanly; this pass does not silently reshape that outer contour. Small canopy crease lines and sharp bumper/arch design returns also remain.

The [seam-normal screen](Proofs/SeamNormalScreen.json) records both improvements and regressions. For example, one lower-side transition's p95 normal jump drops from about 39.3° to 5.0°, and the neighbouring return screen drops from 106.4° to 27.1°; neither is a continuity certificate. Patch 42's p95 curvature-variation screen falls about 60.5%, while its median rises about 9%. There is no blanket “all smoothness metrics improved” claim.

Evidence: [native kernel audit](Proofs/KernelValidation.txt), [24 regression tests](Proofs/PipelineTests.txt), [true-gap control](Proofs/TrueGapControl.txt), [browser tests](Proofs/BrowserValidation.txt), [shared-edge audit](Proofs/TessellationAudit.json), [baseline integrity](Proofs/BaselineIntegrity.txt), [region/clearance measurements](refinement_report.json).

### Retained native topology

| Part | Faces | Edges | Sewn / free / non-manifold |
|---|---:|---:|---:|
| Main shell | 282 | 825 | 715 / 110 / 0 |
| Front cowl | 14 | 45 | 17 / 28 / 0 |
| Roof / glass frame | 10 | 36 | 12 / 24 / 0 |

## Importer compatibility and precision fix

The native audit uses Frontier `82ed251267ed8457ca3530d8228b91555572c25d`, the existing [W95 overlay](../../../Patches/SolidArc-Booleans-Variables-Modifiers.patch), and **both** patches:

1. [NurbsIsoBoundaryImport.patch](../Tools/Patches/NurbsIsoBoundaryImport.patch), required by the original review.
2. [NurbsBoundaryPrecisionProjection.patch](../Tools/Patches/NurbsBoundaryPrecisionProjection.patch), added here.

The second fix corrects the STEP precision estimator. An unconstrained projection could report a fictitious 2.576 mm residual on an exact narrow-patch rim. It now also evaluates valid points on the four NURBS boundary iso-curves and retains the smallest **measured spatial residual**. It does not increase tolerances, suppress audits, or declare a rim exact without measuring it. A deliberately off-surface test rail continues to report a real 0.443 mm gap.

Old W95 application ZIPs are unchanged and can still refuse or mis-handle these imported sheets. Rebuild the application with the overlay and both fixes for the tested native path; a compatible STEP surface-model CAD application is another way to inspect the geometry. No new GUI executable is included here.

## Source deviation is not surface quality

Main-shell CAD→source RMS on the **new conforming display sample** is 0.478 mm before / 0.541 mm after; the insert's deliberate move gives about 0.908 mm source RMS. The maximum sampled shell/source distance remains about 7.897 mm, inherited from the previous fairing. Sampling differs from the older independent rectangular grids, so these are screening values, not controlled geometry-only change metrics or global maximum-tolerance guarantees.

The unchanged front cowl now exposes a 5.199 mm maximum at the denser/different sample; this is not a new cowl edit. Use the region-change measurements above to distinguish edited geometry from sampling differences.

## Reproduce

From the parent **Liger** directory, install the pins in `Tools/requirements-refined.txt` (`bpy`, SciPy/NumPy, Pillow/Numba, and Triangle). A working headless `bpy` runtime is needed for nearest-triangle checks. Keep the original and `Faired` folders alongside `Refined`.

```sh
python Tools/refine_regions.py --scratch /tmp/liger-refined
python Tools/tessellate_shared.py
python Tools/render_refined.py
python Tools/measure_refined.py
CERTIFIER=/path/to/CadCertifier python -m unittest discover -s Tools -p 'test_*.py' -v
python Tools/serve_refined.py 8080
```

`CERTIFIER` enables the native regression; build it using the existing `Tools/CMakeLists.txt` with the overlay and both importer patches. `Tools/Fixtures/ExpectedGapControl.step` is intentionally off-surface regression input, **not production CAD**.

Viewer: `cd Refined/Tools && npm ci && npm run build && npm test` with the server running. The pinned serverless Chromium test requires compatible Linux runtime libraries; `LIGER_PROOFS_URL` can override the local test URL. The viewer requires a current browser with `DecompressionStream('gzip')` support.

`CAD/*.step` and the embedded native document are the CAD authority. The gzip files are derived triangle displays. New asset integrity is recorded in `SHA256SUMS`; `Tools/package_refined.py` regenerates the CAD/static-proof ZIP and verifies its internal hashes.
