> **Latest native workflow:** [Surface tracing workspace + rebuilt binaries](Tracing/README.md) — protected dense NURBS reference, automatic surface drawing, Ctrl model-edge snaps, editable XYZ curves. [Packages](Tracing/Packages/) · [Audit](Tracing/AUDIT.md).

# Liger — simplified SubD-to-CAD review build

**2026-11-01 · 312 native cubic NURBS faces · 93.11% fewer faces than the previous 4,528-face conversion.**

This is a real fitted CAD surface model, with reproducible geometry proofs. It is **not a Class-A, G1/G2-certified, closed-solid or manufacturing-ready release**. The measured results below supersede the earlier optimistic estimate of 50–90 faces.

## Inspect first

- Interactive orbit/zoom inspector: `Proofs/index.html` (serve over HTTP; do not double-click a `file://` page).
- Run `python Tools/serve_proofs.py 8080`, then open `http://localhost:8080/` on your own machine.
- Inspector modes: shaded, coloured patches, deviation heatmap, zebra, source sample overlay, part visibility and click-to-inspect control-net size / residuals.
- Static views: [front quarter](Proofs/01_Front_Quarter.png), [patch layout](Proofs/02_Patch_Layout.png), [rear quarter](Proofs/03_Rear_Quarter.png), [side](Proofs/04_Side.png), [top](Proofs/05_Top.png), [deviation](Proofs/06_Deviation.png), [zebra](Proofs/07_Zebra.png), [browser screenshot](Proofs/08_Interactive_Inspector.png).
- These images evaluate the fitted NURBS pole nets. They are **not AI-generated pictures and not screenshots of the native SolidArc viewport**. The separate kernel log validates the native document.

## CAD files and compatibility — important

- `CAD/Liger_Macro_CAD.step`: AP214 `SHELL_BASED_SURFACE_MODEL` containing the three exterior sheet parts. Individual part STEP files are alongside it. These contain actual `B_SPLINE_SURFACE_WITH_KNOTS` carriers, shared topological edge instances and open shells, **not a faceted mesh disguised as CAD**.
- `CAD/Liger_Macro_CAD.solidarc`: current TOML document with three named, embedded STEP import features. No external file dependencies are required to regenerate it.
- **The native document requires the included NURBS iso-boundary loader fix.** The previously packaged W95 binaries are **unchanged** and the unpatched reader refuses the main shell (six G2/G3 findings caused by numerical boundary inversion). Do not use those old binaries as evidence that this native document has been successfully opened. Apply `Tools/Patches/NurbsIsoBoundaryImport.patch` to the current overlaid Frontier sources and rebuild the application, or build the included certifier against those sources. This turn supplies the model, proofs and loader patch; it does **not** supply replacement Windows/Linux GUI packages.
- The standard SolidArc GUI STEP import/export path also currently refuses sheet bodies. Open the native `.solidarc` document after rebuilding with the loader fix; use the STEP surface model in another CAD application that supports open sheets. No independent third-party CAD import test was run.

## Measured result

| Part | Faces (old → new) | New poles, full vehicle | Held-out source → CAD RMS | p99 | Worst sample |
|---|---:|---:|---:|---:|---:|
| Main shell | 4,366 → **288** | 35,496 | **0.102 mm** | 0.460 mm | **4.976 mm** |
| Front cowl | 76 → **14** | 2,330 | **0.245 mm** | 0.497 mm | **9.255 mm** |
| Roof / glass frame | 86 → **10** | 1,466 | **0.032 mm** | 0.152 mm | **0.210 mm** |
| Total | **4,528 → 312** | **39,292** | — | — | — |

Source-to-CAD values are local closest-point residuals of held-out checkerboard samples from the level-3 limit-position reference. Fitting uses the complementary samples; each sample remains associated with its macro panel. They are not distance bounds over the continuous limit surface. Duplicate points on adjacent source quads are counted, so these are sample-weighted rather than area-weighted statistics. The source was not sampled at progressively higher levels to establish convergence.

A separate reverse check samples the fitted CAD grid against the nearest triangle of the level-3 reference:

| Part | CAD → L3 reference RMS | p99 | Worst sample |
|---|---:|---:|---:|
| Main shell | 0.064 mm | 0.235 mm | 3.178 mm |
| Front cowl | 0.150 mm | 0.634 mm | 4.351 mm |
| Roof / glass frame | 0.036 mm | 0.145 mm | 0.476 mm |

Raw values: `CAD/metrics.json`, `Proofs/reverse_deviation.json`. Neither test is a certified Hausdorff bound. The heatmap uses the **reverse** check (blue 0 → green 1 → red ≥2 mm; magenta >5 mm). The panel inspector uses the **held-out source** check. Do not confuse these two metrics.

## How simplification actually works

1. **Reference extraction.** Apply the authored mirror *before* subdivision so centreline stencils see both halves. Apply one finite Catmull–Clark step without limit projection, then two steps with limit projection enabled. Private face UVs preserve each 5×5 sample grid. Original coordinates are centimetres; output is metres, +X forward, +Z up, symmetry Y=0.
2. **Topological macro layout.** Mark authored creases ≥0.5 and open/non-manifold boundaries. Trace quad separatrices and merge non-feature interfaces only while their union remains a simple four-corner disk. No Liger-specific X stations are hardcoded. This produces 144/7/5 macros per half, then mirrors them: 288/14/10 faces.
3. **Master rails first.** Solve each shared rail once with adaptive cubic knots. Form full panel sides from these canonical rails. Opposite boundaries are made compatible by *exact knot insertion*, never by independent edge refits. The concatenation points use C0 knots; these are not a claim of tangent continuity.
4. **Constrained interiors.** Positive graph-Laplacian parameterization supplies a fold-screened initial domain. Solve cubic B-spline interiors with fixed boundary poles, regularized pole-net fairing, local closest-point updates and limited knot refinement. Export the actual nets, not merely a rendered approximation.
5. **Conforming B-rep at T-junctions.** Split the corresponding topological edge, not the whole surface. Neighbouring faces reference the same `EDGE_CURVE` instance. Thus fewer faces does not require a face split through every upstream grid intersection.

The master-boundary mismatch before STEP coordinate serialization is <2×10⁻¹² mm at the checked parameters. Tiny mirror-coordinate noise (<0.0001 mm) is snapped at serialization. This is geometric coincidence of retained seams, **not closure of the vehicle**: open boundaries deliberately remain.

## Source defect and topology treatment

One rear-body source rail has **three incident sheets** (a non-manifold junction). Mirroring duplicates that junction. Retain the first two sheet incidences and detach the extra macro face into an independent, coincident sheet on each side. Those two extra branch faces retain their geometry but are not sewn to the main body at the defect. They form one additional mirrored sheet component. No faces were deleted to hide the defect and sewing tolerance was not inflated.

| Part | Connected open shells | Sewn edges | Free edges | Non-manifold edges |
|---|---:|---:|---:|---:|
| Main shell | 2 | 721 | 110 | 0 |
| Front cowl | 4 | 17 | 28 | 0 |
| Roof / glass frame | 2 | 12 | 24 | 0 |

Free edges include source openings, separated sheet components and the detached branch seam. The source roof-frame surface overlaps the body and can z-fight; isolate parts in the inspector. This conversion preserves authored skin rather than inventing thickness or an underside.

## Audit and remaining work

| Check | Result |
|---|---|
| 🟢 Real cubic NURBS CAD carriers | 312 faces across 3 named sheet parts |
| 🟢 Face-count reduction | 93.11% relative to the 4,528-face conversion |
| 🟢 Patched SolidArc regeneration | 3 bodies, **0 refused steps** |
| 🟢 Patched kernel `AuditBody` | All 3 bodies **sound**, 0 findings |
| 🟢 Boundary topology | 0 non-manifold edges after documented branch detachment |
| 🟢 Small pipeline regressions | 5 / 5 passing |
| 🟢 Curved split-edge/T-junction native fixture | 3 faces, sound, 0 refused steps |
| 🟢 Browser smoke test | Model load, 4 modes, part visibility, camera presets, reference overlay; 0 JS errors |
| 🟡 Accuracy target | All shell/frame macros ≤0.5 mm held-out RMS; **one cowl macro exceeds it** (0.953 mm RMS, 9.255 mm local max) |
| 🟡 Normal screening | Opposite-to-reference triangle normals at 558 / 93,496 shell samples, 64 / 5,560 cowl samples and 29 / 2,816 frame samples. These flag degeneracy/nearest-triangle ambiguity/possible local folds; not a proof of orientation correctness |
| 🟡 Pole complexity | 39,292 total poles; not a proven minimal-pole solution |
| 🟡 Continuity | **No G1/G2 enforcement or certification**; zebra exposes remaining irregularities |
| 🟡 Original 50–90-face estimate | **Not met** while conservatively preserving authored crease topology |
| 🟡 Old packaged GUI binaries | Need loader patch + rebuild; not updated here |
| ⚪ Manufacturing solid / complete car | Not in this result; no wheels, interior, aero extras, thickness or underside |

Kernel evidence: `Proofs/KernelValidation.txt`. Validation is against `unassignedinbox/Frontier` branch `arena/d90f0539-frontier` with the existing W95 SolidArc overlay and the included iso-boundary patch. The fix detects split NURBS iso-boundaries by 1D boundary projection and sampled agreement before choosing an affine parameter line. It does not relax audit tolerances or skip findings. Other edges keep the existing generic projection path. This recognition is sampled, not a symbolic identity certificate.

**Recommended next work after visual review:** repair the cowl outlier and suspect normal regions, then add coupled tangent constraints along genuinely smooth seams. Do not force the count down to 50–90 by discarding sharp details or merely joining dense C0 nets.

## Reproduction

Source: `unassignedinbox/Slate`, branch `arena/01a0fdf5-slate`, `Vehicles/Liger/Source/Liger_named.blend`. The exact source hash and original modifier states are recorded in `Source/reference.json`; the source blend and extracted reference arrays are included.

```sh
python3 -m venv .venv
.venv/bin/pip install -r Tools/requirements.txt
# bpy may require system OpenGL/X11 loader libraries even in headless mode.
.venv/bin/python Tools/extract_reference.py
.venv/bin/python Tools/fit_macros.py
.venv/bin/python Tools/export_cad.py
.venv/bin/python Tools/make_proofs.py
.venv/bin/python Tools/render_proofs.py
.venv/bin/python Tools/test_pipeline.py
python3 Tools/serve_proofs.py 8080
```

The `*_fit.pkl` intermediate caches are ignored. Rebuild them locally before exporting/proof generation. They are not the delivery format. Never load a pickle from an untrusted source.

To rebuild the browser bundle: `cd Tools && npm install && npm run build`. The shipped bundle is self-contained (no CDN); Three.js's MIT notice is alongside it.

To validate in C++ after applying the existing SolidArc overlay to Frontier:

```sh
cd /path/to/Frontier
git apply /path/to/Liger/Tools/Patches/NurbsIsoBoundaryImport.patch
python Experimental/SolidArc/Native/Tooling/PrepareSolidArcDependencies.py
cmake -S /path/to/Liger/Tools -B /tmp/liger-certifier -G Ninja \
  -DFRONTIER_ROOT=/path/to/Frontier -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/liger-certifier --target CadCertifier
/tmp/liger-certifier/CadCertifier /path/to/Liger/CAD/Liger_Macro_CAD.solidarc
```

The optional `--dump-scene <prefix>` invokes native tessellation, which is much slower than the audit on these high-span surfaces. The shipped gallery uses direct NURBS evaluation rather than that optional native tessellator.
