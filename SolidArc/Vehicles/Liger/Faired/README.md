# Liger — boundary-preserving NURBS smoothing revision

**01 November 2026 · 28 half-body patches / 56 mirrored faces faired · still 312 cubic NURBS faces.**

This edits the actual CAD pole nets, not just display normals. The [original review](../README.md), its CAD, source, proofs and ZIP remain available unchanged. This revision lives entirely in `Faired/`.

## Inspect / download

- [Native SolidArc document](CAD/Liger_Macro_CAD.solidarc) · [combined STEP](CAD/Liger_Macro_CAD.step).
- [CAD + static proofs ZIP](Liger-Faired-CAD-and-Proofs.zip). This ZIP does **not** contain the interactive viewer; use the repository/preview for that.
- Browser comparison: run `python Tools/serve_faired.py 8080` from the parent **Liger** directory. Open the printed/live preview, or `http://localhost:8080/` on your own machine.
- In the viewer, **Show original CAD (before)** swaps actual positions/normals at the same camera. Use zebra or shaded mode. Downloads always contain the **after** revision.
- **Shape-change map** measures the faired CAD against its original CAD patch, not matching UV coordinates. **Distance to polygon source** is a separate metric and is **not a smoothness score**.
- [Bonnet zebra before](Proofs/02_Bonnet_Before_Zebra.png) / [after](Proofs/03_Bonnet_After_Zebra.png) · [shaded before](Proofs/08_Bonnet_Before_Shaded.png) / [after](Proofs/09_Bonnet_After_Shaded.png).
- [Whole-body zebra before](Proofs/04_Before_Zebra.png) / [after](Proofs/05_After_Zebra.png) · [shape-change map](Proofs/06_Shape_Change.png) · [source distance](Proofs/07_Source_Deviation.png) · [interactive screenshot](Proofs/10_Interactive_Inspector.png).

All images are deterministic geometry renders or a browser screenshot of evaluated NURBS, **not AI-generated vehicle images and not native GUI screenshots**.

## What was changed

Broad, graph-like main-shell patches received a smoother physical height field and a rebuilt interior parameterization. Their final cubic nets are fitted to height **and physical first/second derivatives**, avoiding the rippled normals left by position-only fitting. The particularly rough bonnet-surround patch **21** uses physical-area weighting because its original parameterization folds and oversamples the edge.

- **All patch boundary poles and all knot vectors are exactly unchanged.** Shared B-rep rail curves, edge entities, openings, authored crease locations and connectivity are preserved.
- 28 right-half patches plus their mirrors changed; the other **256 faces** did not. The front-cowl and roof/glass-frame STEP files are byte-identical to their originals.
- No added patches, poles, invented thickness, closed underside, wheels or interior.
- Candidates need at least 30% reduction in the matched-position p95 curvature-variation screen with no more than a 10% median increase (patch 50 has a 1.8% median increase; all others improve). 13 broad candidates failed the conservative screen or graph-projection test and were left unchanged. Smaller/complex/strongly turning patches are not automatically rebuilt.
- Retaining a rim curve does **not** retain or certify its tangent plane. First interior pole rows may change. Seam blending is not globally re-engineered in this pass.

### Bonnet-surround patch 21 — measured result

IDs here are **zero-based**, matching STEP face suffix `_021` and the comparison viewer.

| Check | Before | After |
|---|---:|---:|
| p95 absolute mean curvature, matched positions | 125.545 /m | 17.217 /m |
| Median curvature-variation screen | 879.540 /m² | 173.970 /m² |
| p95 curvature-variation screen | 76,273.721 /m² | 1,168.405 /m² |
| Opposed Z-normal samples, dense interior/knot screen | 460 | 0 |
| Boundary-curve/pole displacement | — | **0 mm** |

The p95 curvature-variation screen falls **98.47%** on this patch. Curvature is compared at **3,343 matched physical projected positions**, not blindly at matching old/new UVs. A physical-neighbour finite-difference screen estimates curvature variation; it is not a G2 certificate. The dense normal check is a separate **41,949-point** grid including both sides of interior knots.

Sampled, bidirectional new↔original CAD distance on this patch: **2.634 mm RMS / 6.408 mm p95 / 9.270 mm maximum**. Other edited patches have a largest sampled change of **2.518 mm**. Distances use vertices against a 151×151 triangulation in each direction, **not a certified Hausdorff bound**. Same-UV displacement is deliberately not used because the interior parameterization changed.

## Audit / completeness

| Item | Status | Evidence / limitation |
|---|:---:|---|
| Actual NURBS CAD edited and exported | 🟢 | 56 changed faces; 312 total, 39,292 poles unchanged in count |
| Boundary curves, knots, B-rep connectivity | 🟢 | Exact pole/knot and rail-entity checks; unchanged edges/loops |
| Mirror symmetry and combined/individual agreement | 🟢 | Regression checks on all delivered surface nets |
| Native regeneration | 🟢 | 3 bodies, **0 refused** |
| Native `AuditBody` | 🟢 | All 3 bodies sound; no non-manifold edges |
| Existing importer tolerance behavior | 🟢 | Same as baseline: 4 tolerant edges, 8 tolerant vertices, largest reported gap **0.0132246 mm** |
| Python regression suite | 🟢 | **14 tests passed**, original + smoothing/export tests |
| Browser inspector | 🟢 | 5 display modes, before/after, picking, source overlay, downloads, mobile; zero JS/shader errors or failed HTTP responses |
| Matched-camera before/after and change map | 🟢 | Actual NURBS position/derivative evaluations; no cosmetic normal smoothing |
| Every remaining rough area repaired | 🟡 | Selective pass only; 13 broad candidates kept and other complex patches excluded |
| Near-rim orientation screening | 🟡 | Across edited halves: 6,966→195 opposed-axis samples out of 863,066; residuals on patches **9 / 81 / 99** |
| Cross-patch G1/G2, Class-A, self-intersection certificate | 🟡 | **Not certified**; visible seam/rim irregularities remain |
| Closed manufacturing solid | 🟡 | **No** — the same 3 exterior sheet parts and original detached rear branch remain |
| Standalone native GUI binaries | 🟡 | **Not rebuilt**; this is a CAD revision, not a new application release |

Kernel evidence: [KernelValidation.txt](Proofs/KernelValidation.txt). Regression evidence: [PipelineTests.txt](Proofs/PipelineTests.txt), [BrowserValidation.txt](Proofs/BrowserValidation.txt), [dense normal screen](Proofs/DenseNormalScreen.json), [per-patch measurements and skipped reasons](fairing_report.json).

The 195 residual opposed-axis samples are predominantly near protected complex rims. This screening concerns projection/orientation, not proof of 3D self-intersection. It would be misleading to describe the whole car as fold-free or Class-A.

### Native topology retained

| Part | Faces | Edges | Sewn / free / non-manifold |
|---|---:|---:|---:|
| Main shell | 288 | 831 | 721 / 110 / 0 |
| Front cowl | 14 | 45 | 17 / 28 / 0 |
| Roof / glass frame | 10 | 36 | 12 / 24 / 0 |

The **same NURBS iso-boundary importer fix** as the original review is required: [NurbsIsoBoundaryImport.patch](../Tools/Patches/NurbsIsoBoundaryImport.patch). Old W95 GUI packages were not rebuilt and can still refuse the main shell. Validation uses Frontier commit `82ed251267ed8457ca3530d8228b91555572c25d` with the shipped W95 overlay and that importer fix; no tolerance inflation or audit bypasses were added for smoothing.

## Source accuracy versus smoothness

The intent is to reject polygon noise, not trace it more tightly. Main-shell sampled CAD→source RMS therefore rises from **0.064 mm to 0.404 mm**; the new full mirrored grid has **2.048 mm p99 / 7.933 mm maximum** distance to the triangulated L3 reference. These are explicit design departures, concentrated around the repaired bonnet surround and other edited interiors. A warmer source heatmap does **not** contradict smoother curvature.

Cowl and frame are geometrically unchanged; current grid metrics are in [reverse_deviation.json](Proofs/reverse_deviation.json). These sampled reverse distances are not maximum-tolerance or manufacturing certificates.

## Reproduce

Use the dependency versions in `../Tools/requirements.txt`. `bpy`/BVHTree is needed for nearest-triangle distance screens and proofs. Run from the parent **Liger** directory, keeping the baseline `CAD/`, `Source/` and `Proofs/` alongside `Faired/`:

```sh
python Tools/fair_surfaces.py --output /tmp/liger-faired-nets
python Tools/build_faired.py --nets /tmp/liger-faired-nets
python Tools/render_faired.py
python -m unittest discover -s Tools -p 'test_*.py' -v
python Tools/package_faired.py
python Tools/serve_faired.py 8080
```

Intermediate accepted nets and fitting caches need not be committed; the delivered STEP stores the actual edited cubic nets. `inspection_geometry.npz` is only derived display data, and `geometry.bin` is a triangle display, not the CAD authority.

Rebuild/test the comparison viewer with `cd Faired/Tools && npm ci && npm run build && npm test`, with the server running. The browser test uses `playwright-core` and pinned serverless Chromium; a normal Chromium-compatible Linux runtime/library set is required. `LIGER_PROOFS_URL` can point the test at another served URL.

Per-patch quantitative results follow in [fairing_report.json](fairing_report.json). New deliverable integrity is recorded in `SHA256SUMS`; the original baseline checksum file still verifies separately.
