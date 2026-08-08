# 02 — NumericFoundation

`Layer1_Numeric` is the only place in Slate where a numerical guarantee originates. Everything above it inherits
guarantees; nothing above it may strengthen one. That inversion — guarantees flow up and weaken, never up and
strengthen — is what makes the precision tier system enforceable rather than aspirational.

This document specifies the scalar and geometric vocabulary, the exact predicates the visibility spine and the
selection outline both depend on, the solvers and integrators, and the parity mechanism that proves the CPU form
and the shader form of a computation agree.

## Position In The Sequence

| Field       | Value                                                                      |
|-------------|-----------------------------------------------------------------------------|
| Unit        | `SlateMath.lib`                                                             |
| Layer       | `Layer1_Numeric`                                                            |
| Upstream    | `00` — tier system, parity mechanism, link partition                        |
| Downstream  | Every document in the series without exception                              |
| Unblocks    | Any computation at all; `Contract/` becomes usable                          |

## 1. Scope

In scope — scalar policy, transforms, orientation and intersection predicates, tolerance arithmetic, linear and
constraint solving, quadrature and time integration, spectral and colour projection, sampling patterns.

Out of scope — anything that touches a device, a file, a window or a document. `Layer1_Numeric` has no include
path to `SlateVulkan`, `SlateDocument` or `SlateUI`, and the link partition makes that structural.

## 2. Scalar Policy

| Use                                   | Representation | Tier | Reason                                            |
|---------------------------------------|----------------|------|----------------------------------------------------|
| Occupant position in document space   | 64-bit real    | B    | Scene extents exceed 32-bit relative precision     |
| Transform composition                 | 64-bit real    | B    | Composition depth is unbounded in the relation     |
| Orientation and intersection predicates | exact        | A    | Sign correctness is topological, not approximate   |
| Position handed to the device         | 32-bit real    | B    | Rebased to view origin first — see §3.2            |
| Shading, atmosphere, reflection       | 16-bit real    | D    | Perceptual; no numeric guarantee is claimed        |
| Occupant and surface identity         | 32-bit unsigned| A    | An identity that rounds is not an identity         |

📝 The rebasing in §3.2 is what lets device-side arithmetic be 32-bit without the scene being limited to 32-bit
extents. Skipping it produces jitter that looks like a driver defect and is not one.

## 3. Geometric Vocabulary

### 3.1 Transforms

A transform is stored decomposed — translation, rotation as a unit quaternion, and a scale triple — never as a
general matrix. Decomposed storage is what makes the kinematic nesting relation in `12` composable without drift,
because composing quaternions renormalises and composing matrices does not.

Matrix form is derived at the point of use and never stored back. `Project` derives it; nothing caches it.

### 3.2 Spaces and the rebasing rule

| Space              | Origin                        | Scalar | Who works in it                     |
|--------------------|-------------------------------|--------|--------------------------------------|
| Document space     | Document origin               | 64-bit | `SlateDocument`, the outliner, tools |
| View-relative space| Current camera position       | 64-bit | Camera assembly, culling             |
| Device space       | Camera position, rebased      | 32-bit | Everything in `SlateCompute`         |
| Surface space      | One paintable surface's domain| 32-bit | `20`, `22`, `24`                     |

🔴 Every position crossing into `SlateCompute` is rebased to the view origin first. This is a single subtraction
performed in 64-bit before the narrowing conversion, and it is not optional.

### 3.3 Tolerances

Tolerances are declared in `Contract/`, named by what they compare, and never written as literals at a call site.
A tolerance is scale-relative — expressed against the extent of the operand, not as an absolute distance — because
an absolute tolerance is correct at exactly one scene scale.

🔴 `Contract/` also holds every **constant two units both read** — `PhysicalTileApron`, packed capacities,
ceilings. `00` §2 gives the reason: a constant declared in one unit and read from another is an Upstream edge
that no traversal can see, and one of them survived long enough to reopen conflict 13 from the other side.

## 4. Exact Predicates — Tier A

Four predicates carry Tier A. They are the foundation of every topological decision in the engine, and they are
the reason `16` and `26` can agree about which surface a pixel resolved to.

| Predicate                | Question answered                                    | Consumed by     |
|--------------------------|------------------------------------------------------|------------------|
| `OrientationClassifier`  | Sign of an orientation determinant                   | `16`, `24`, `26` |
| `IncircleClassifier`     | Is a point inside a circumscribed circle             | `68`, `52`       |
| `IntersectionClassifier` | Do two extents genuinely intersect, and where        | `16`, `26`       |
| `ContainmentClassifier`  | Is a point strictly inside, on, or outside a boundary| `12`, `26`       |

Each is implemented with a floating-point fast path and an exact fallback taken only when the fast path's error
bound cannot exclude zero. The fallback is a correctness requirement, not an optimisation to be removed: a
predicate that is *usually* exact provides no topological guarantee at all.

Each also exists in `Shared/`, compiles under both toolchains, and is proven equal by `ParityRunner`. A Tier A
predicate that disagrees between CPU and device is the defect class that produces cracks along surface seams and
selection outlines that flicker at silhouettes.

## 5. Solvers And Integrators

| Component                | Mechanism                                        | Tier | Consumed by |
|--------------------------|--------------------------------------------------|------|--------------|
| `LinearSolver`           | Dense and sparse factorisation                   | B    | `24`, `68`   |
| `UnwrapSolver`           | Boundary-first parameterisation                  | C    | `68`         |
| `QuadratureIntegrator`   | Definite integral approximation over a domain    | B    | `18`, `28`   |
| `TimeIntegrator`         | Fixed-step accumulation with an interpolant      | C    | `64`         |
| `ColourProjection`       | Transfer between colour spaces, and its inverse  | B    | `36`, `18`, `66` |
| `WhiteProjection`        | Chromatic adaptation between white points        | B    | `36`, `44`   |
| `TransferProjection`     | An encoding transfer and its inverse             | B    | `36`, `66`   |
| `SpectralProjection`     | Wavelength-domain to tristimulus                 | B    | `28`         |
| `CurveSolver`            | Planar path evaluation, offsetting, flattening   | B    | `52`         |
| `PlanarClassifier`       | Winding and coverage for closed planar paths     | A    | `52`         |
| `LatticeProjection`      | Periodic plane symmetry — translation and reflection | A | `70`         |

Every Tier C component declares its convergence criterion and its iteration ceiling as part of its contract, and
reports which of the two terminated it. A solver that silently returns its last iterate when the ceiling is hit is
indistinguishable from one that converged, and that ambiguity propagates upward as an unexplained artefact.

⚠️ `TimeIntegrator` was declared here as consumed by `12` and `12` never reads it. Its real consumer is `64`.
Recorded as `00` §10 conflict 21.

⚠️ `ConstraintSolver` is **removed**. It named `12` and `24` as consumers and neither reads it — `12` composes
static transforms and solves nothing, and `24`'s Upstream cites `LinearSolver` and the predicates only. This is
conflict 21's defect a second time, caught by §8's gate on the second pass rather than the first. Recorded as
`00` §10 conflict 41. The three colour rows above are `36`'s components declared here, where the mechanism
lives; `36` §1 declares what they mean and does not re-spell them.

🔴 The last three rows are new and each has exactly one consumer, named. `PlanarClassifier` and
`LatticeProjection` are **Tier A** and therefore parity-proven: a vector outline whose interior test disagrees
between host and device has a different silhouette in a preview than in the resolved surface, and a periodic
lattice that disagrees produces a pattern that does not meet itself across a tile edge.

## 6. Sampling

Sample patterns are shared source, because the device and the host must place samples identically for `18`'s
accumulation and `28`'s integration to agree.

| Pattern                | Property                                    | Consumed by |
|------------------------|---------------------------------------------|--------------|
| Low-discrepancy planar | Uniform coverage, progressive               | `18`, `30`, `60` |
| Spherical              | Solid-angle uniform                         | `28`         |
| Hemispherical, cosine  | Weighted to the cosine lobe                 | `18`, `60`   |
| Sub-pixel offsets      | Deterministic per rotation slot             | `46`, `64`, `82` |

⚠️ The offset row previously named `08` and `18`, neither of which mentions jitter. `46` applies the offset to
the projection, `64` accumulates across the sequence, and `82` replays it for a preview. Conflict 20 recorded
that the offsets had no consumer; they had three, and none of them was listed.

## 7. Parity

Every `Shared/` entry point is registered with `ParityRunner`, which evaluates the C++ and shader forms over a
common sample set and asserts agreement at the declared tier — bit-exact for A, ULP-bounded for B, within the
convergence criterion for C. Tier D entry points are not parity-checked; that is what Tier D means.

An entry point in `Shared/` with no registration is duplicated source that has not diverged yet.

## 8. Gates

- **Gate:** `Layer1_Numeric` compiles with no include path to any unit other than `SlateMath` itself.
- **Gate:** Every exported computation declares a tier.
- **Gate:** No computation claims a tier stronger than its weakest input.
- **Gate:** All four Tier A predicates take an exact fallback and are registered with `ParityRunner`.
- **Gate:** No tolerance literal appears outside `Contract/`.
- **Gate:** Every position narrowing to 32-bit is rebased in 64-bit first.
- **Gate:** Every Tier C component reports its termination cause.
- 🔴 **Gate:** Every component in §5 names at least one consumer that reads it. A component with no consumer is
  removed or its consumer is named — never carried.

## 9. Open

| Open question                                                        | Blocks                        |
|-----------------------------------------------------------------------|--------------------------------|
| Exact-fallback strategy — adaptive expansion or extended precision     | Nothing in design; `16` timing |
| Whether `TimeIntegrator` is needed before `64` ships                   | `64` scheduling only           |
| Whether path flattening tolerance is fixed or resolution-relative      | `52` quality; `70` re-resolves |
