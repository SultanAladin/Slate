# Unified Geometry Rendering and Physical Material Plan

## Status and authority

This is the forward implementation plan for getting imported geometry onto the Slate viewport and for the physical-material contract that will follow it.

It supersedes the material inventory and render-order recommendations in `GeometryWorkspaceAndMaterialProcessingPlan.md` where they conflict. The existing document remains useful for geometry intake, selection, gizmos, and layer-processing detail.

The implementation priority is explicit:

1. finish geometry scene registration and the Vulkan rendering pipeline;
2. render real imported geometry with a fixed white dielectric;
3. add wire and selection proof;
4. only then resume material image/layer processing.

Material layer resolution, image processing, dirty tiles, viewport material binding, and export packing are **on hold**. Planning the durable material schema now prevents the geometry renderer from baking in an eight-channel or opaque-only assumption.

## Decisions

### Rendering decisions

- Use Slate's existing `DepthSurface` and `VisibilityIndex` targets.
- Use a visibility-buffer renderer for opaque and cutout geometry.
- Start with hardware rasterization. Do **not** implement software micro-rasterization in the first geometry renderer.
- Keep geometry graphics programs within the classic render-construct architecture owned by `AttachmentIndex`.
- Do not add surface geometry to `WorkspaceOverlayPass`.
- Shade only the winning visible fragment into `RadianceSurface`.
- Rebase document FP64 positions before narrowing to GPU FP32.
- Keep stable source-face and owner mappings through triangulation, upload, visibility, and picking.
- Treat transmissive glass as a later, explicit render path; do not pretend alpha blending is physical transmission.

### Material decisions

- Use an OpenPBR-inspired layered physical surface as Slate's neutral authoring target, not Unreal-specific serialized material data.
- Preserve familiar metallic/roughness controls while also supporting physical interface controls such as F0, F90, IOR, attenuation, and thickness.
- Use EON for the optional rough-diffuse response. EON is a diffuse BRDF, not the fuzz lobe.
- Allow coat, fuzz, anisotropy, subsurface, thin film, and transmission to coexist. They must not be mutually exclusive material selections.
- Separate sampleable material signals from structural declarations such as thin-walled, two-sided, coverage mode, and parameterization mode.
- Compile only the features a material consumes. A shader must not sample every declared signal.
- Keep imported parameterizations losslessly enough to export or re-resolve them; generate one renderer-facing closure representation from them.

## What exists today

Slate currently declares twenty material channels:

1. Albedo Colour
2. Metallic
3. Roughness
4. Normal-Incidence Reflectance
5. Surface Orientation
6. Ambient Occlusion
7. Emission
8. Opacity
9. Anisotropy
10. Anisotropy Direction
11. Clear Coat
12. Clear-Coat Roughness
13. Clear-Coat Orientation
14. Sheen Colour
15. Sheen Roughness
16. Subsurface Colour
17. Subsurface Thickness
18. Transmission
19. Refraction Ratio
20. Displacement

This is a useful first inventory, but it is **not the complete physical-material contract**.

Important gaps are:

- independent diffuse roughness for EON;
- explicit specular weight and tint;
- coloured F0 and F90 control;
- independent tangent direction and optional second specular lobe;
- coat colour/transmittance, IOR, thickness/absorption, and darkening policy;
- explicit fuzz weight and orientation;
- transmission colour, attenuation depth, scattering, phase anisotropy, and dispersion;
- separate geometric thickness and thin-walled declaration;
- subsurface mean-free-path/radius and phase anisotropy;
- thin-film weight, thickness, and IOR;
- physical emission intensity;
- distinct opacity/coverage/transmission semantics.

The current `ReflectanceSelection` is also too restrictive. Selecting `ClearCoated`, `Anisotropic`, `Cloth`, or `Transmissive` as mutually exclusive choices prevents ordinary combinations such as anisotropic coated metal, fuzzy coated plastic, or coated glass.

The current layer `ChannelMask` is 32 bits. The durable channel inventory exceeds that capacity and must move to a named `ChannelSet` before new channels are added.

## Unified physical surface

### Structural form

The general surface is reduced to this ordered physical structure:

```text
ambient medium
    ↓
optional fuzz layer
    ↓
optional dielectric coat with absorption
    ↓
optional thin film at the base interface
    ↓
metal / glossy-diffuse / subsurface / translucent base mixture
    ↓
interior medium for thick transmissive geometry
```

Horizontal mixing remains a masked mix between complete surface descriptions. Vertical layering means light traverses physically ordered layers. These are not equivalent operations and must remain distinct.

### Structural declarations, not texture channels

The following are material declarations and are selected once per material or partition:

```text
SurfaceClosureSelection
  PhysicalSurface | Unlit
  reserved later: Hair | Eye | Water | ParticipatingMedium

CoverageSelection
  Opaque | Cutout | Blended

WallSelection
  Thick | ThinWalled

SidednessSelection
  Front | TwoSided

InterfaceParameterization
  RefractionRatio | DirectF0

DiffuseResponse
  Lambert | EON
```

`PhysicalSurface` is feature-composable. It replaces the current mutually exclusive Standard/Anisotropic/ClearCoated/Cloth/Subsurface/Transmissive split.

Coverage, transmission, and volume are different:

- **coverage** says whether geometry exists at a sample;
- **opacity** may blend a surface for an artistic path;
- **transmission** sends light through an existing interface;
- **volume thickness** determines the distance through an absorbing/scattering medium.

### Authoring signals

The durable authoring inventory is grouped below. Every signal can be constant or source-bound where meaningful. Signal presence does not force runtime sampling.

#### Geometry and occupancy

| Signal | Type | Default | Meaning |
|---|---:|---:|---|
| AlbedoColour | RGB reflectance | 0.8 | diffuse albedo or metallic reflectance input |
| SurfaceOrientation | direction | neutral | base shading normal |
| SurfaceTangent | direction | geometry tangent | anisotropy orientation frame |
| AmbientOcclusion | scalar | 1 | indirect-light occlusion only |
| Opacity | scalar | 1 | coverage/blended-surface amount according to `CoverageSelection` |
| Displacement | distance | 0 | signed geometric or reconstruction displacement |
| GeometryThickness | distance | geometry-derived | explicit thickness texture/approximation for transmission and thin surfaces |

#### Base interface and diffuse response

| Signal | Type | Default | Meaning |
|---|---:|---:|---|
| Metallic | scalar | 0 | dielectric-to-conductor mixture |
| SpecularWeight | scalar | 1 | dielectric interface response weight |
| SpecularColour | RGB reflectance | white | tint applied to dielectric interface response |
| SpecularRoughness | scalar | 0.3 | primary microfacet roughness |
| DiffuseRoughness | scalar | 0 | EON rough-diffuse roughness |
| NormalIncidenceReflectance | RGB reflectance | derived | F0; authoritative only in `DirectF0` mode |
| GrazingReflectance | RGB reflectance | white | F90 artistic/edge response |
| RefractionRatio | scalar ratio | 1.5 | IOR; authoritative in `RefractionRatio` mode |
| SecondSpecularRoughness | scalar | primary | optional second lobe roughness |
| SecondSpecularWeight | scalar | 0 | optional second lobe mixture |

For a dielectric in physical-IOR mode:

```text
F0 = ((ior - 1) / (ior + 1))² × SpecularWeight × SpecularColour
```

The direct-F0 and IOR forms must not both be authoritative. Both may be retained for round-trip source records, but one declared parameterization produces the renderer-facing F0.

#### Anisotropy

| Signal | Type | Default | Meaning |
|---|---:|---:|---|
| Anisotropy | scalar | 0 | primary-lobe elongation |
| AnisotropyDirection | scalar/angle | 0 | rotation around the shading normal |

The resolved tangent frame comes from geometry tangent plus anisotropy direction. A direction map may override the tangent signal without changing geometry topology.

#### Coat

| Signal | Type | Default | Meaning |
|---|---:|---:|---|
| CoatWeight | scalar | 0 | coat coverage |
| CoatColour | RGB transmittance | white | coat absorption/transmittance tint |
| CoatRoughness | scalar | 0 | coat interface roughness |
| CoatRefractionRatio | scalar ratio | 1.6 | coat IOR |
| CoatThickness | distance | 0 | physical authoring thickness when supplied |
| CoatAttenuationDistance | distance | infinite | distance associated with coat colour |
| CoatDarkening | scalar | 1 | compensation/inter-reflection policy weight |
| CoatOrientation | direction | inherited | independent coat normal |

`CoatThickness` is retained because Slate must support physical coating workflows. A renderer may reduce thickness and attenuation to an equivalent view-dependent transmittance when its real-time tier cannot track the coat medium exactly.

#### Fuzz

| Signal | Type | Default | Meaning |
|---|---:|---:|---|
| FuzzWeight | scalar | 0 | microfibre-layer coverage |
| FuzzColour | RGB reflectance | white | fibre scattering colour |
| FuzzRoughness | scalar | 0.5 | microflake/fibre distribution |
| FuzzOrientation | direction | inherited | optional fuzz normal |

Fuzz is a layer above coat. It is not a synonym for sheen colour and not the EON rough-diffuse response.

#### Transmission and glass

| Signal | Type | Default | Meaning |
|---|---:|---:|---|
| TransmissionWeight | scalar | 0 | specular transmission amount |
| TransmissionColour | RGB transmittance | white | transmitted-light tint |
| TransmissionDepth | distance | 0 | reference attenuation depth |
| TransmissionScatter | RGB coefficient | black | interior scattering colour/coefficient |
| TransmissionScatterAnisotropy | scalar | 0 | phase anisotropy, -1 to 1 |
| DispersionScale | scalar | 0 | chromatic-dispersion amount |
| DispersionAbbeNumber | scalar | 20 | dispersion parameter |

Thick glass uses closed geometry, IOR, geometric path length, absorption/attenuation, and optional scattering. Thin-walled glass uses the thin-wall declaration and does not invent a closed interior.

#### Subsurface

| Signal | Type | Default | Meaning |
|---|---:|---:|---|
| SubsurfaceWeight | scalar | 0 | base-to-subsurface mixture |
| SubsurfaceColour | RGB reflectance | 0.8 | scattering albedo |
| SubsurfaceRadius | RGB distance | 1 | mean travel/radius by wavelength band |
| SubsurfaceRadiusScale | scalar | 1 | global radius multiplier |
| SubsurfaceAnisotropy | scalar | 0 | phase anisotropy, -1 to 1 |

The existing single `SubsurfaceThickness` cannot represent both path length and RGB mean free path. Geometry thickness and scattering radius are separate declarations.

#### Thin film

| Signal | Type | Default | Meaning |
|---|---:|---:|---|
| ThinFilmWeight | scalar | 0 | thin-film coverage |
| ThinFilmThickness | micrometres | 0.5 | optical film thickness |
| ThinFilmRefractionRatio | scalar ratio | 1.4 | film IOR |

#### Emission

| Signal | Type | Default | Meaning |
|---|---:|---:|---|
| EmissionColour | RGB emission | white | spectral/colour shape |
| EmissionLuminance | luminance | 0 | physical emitted intensity |

This yields 48 sampleable semantic signals. The number is not a promise that a material samples 48 textures. Ordinary metallic/roughness geometry consumes approximately six to ten resolved values, and the compiler emits a bounded feature variant.

### Channel storage changes required later

Before material implementation resumes:

1. Replace `std::uint32_t ChannelMask` with a named, fixed-capacity `ChannelSet` of at least 64 bits.
2. Add a non-colour three-component measure for radii/coefficients so RGB-shaped data is not colour-space converted.
3. Replace mutually exclusive `ReflectanceSelection` with structural declarations plus a compiled feature set.
4. Retain imported source parameterization separately from the resolved renderer closure.
5. Generate compact per-material GPU records containing only the active feature tier and resource handles.
6. Keep material/partition feature selection uniform; allow weights and values to vary per texel.

### Runtime feature tiers

The renderer should classify compiled materials by actual requirements:

```text
Tier 0 — Unlit
Tier 1 — Opaque standard: albedo, F0, roughness, normal, AO, emission
Tier 2 — Extended opaque: anisotropy, EON diffuse, second lobe, coat, fuzz, thin film
Tier 3 — Subsurface
Tier 4 — Transmission / glass
Tier 5 — specialised closures reserved for hair, eye, water, participating media
```

The tier is a runtime compilation result, not authored material identity. Quality settings may simplify a tier, but must report the simplification and preserve document parameters.

## Geometry rendering pipeline

### Stage 1 — neutral scene registration

Deliver:

- `DecodedScene` with hierarchy-ready entity recipes;
- caller-supplied auxiliary inputs for MTL and later external glTF payloads;
- outliner capacity preflight;
- one atomic registration transaction;
- stable owner-to-geometry-instance, transform, material-assignment, and source-record mappings.

Gate:

- a multi-owner import either registers every owner/component/relation or registers nothing;
- population exhaustion is caught before mutation;
- identities remain generation checked.

### Stage 2 — GPU geometry resources

Extend `GeometryRenderingExchange` from CPU packets to disposable device resources:

```text
GeometryRenderingResource
  position reservation
  packed normal/tangent reservation
  UV/colour reservation when present
  triangle-index reservation
  triangle-to-source-face reservation
  source-wire reservation
  revision keys
  generation-checked rendering identity
```

Rules:

- allocate through `ByteSpace`;
- no dedicated hidden allocator;
- stage copies through the existing command/cycle ownership;
- retire only after the last consuming cycle completes;
- re-upload only streams whose revision changed;
- CPU source topology remains authoritative.

### Stage 3 — origin rebasing and instance records

Document positions remain FP64. For each viewport/camera origin:

```text
rebasedFP64 = documentPositionFP64 - viewportOriginFP64
uploadedFP32 = checked_narrow(rebasedFP64)
```

Never narrow before subtraction.

Per-instance data contains:

```text
world transform relative to viewport origin
normal transform
geometry rendering identity
fixed material identity initially
stable frame owner index
visibility and selection flags
```

The first implementation may rebuild a small instance buffer each cycle. It must not rewrite immutable geometry positions for camera motion.

### Stage 4 — geometry visibility construct

Declare a dedicated classic render construct through `AttachmentIndex`:

```text
colour: VisibilityIndex (R32G32_UINT)
depth:  DepthSurface (D32_SFLOAT or supported equivalent)
```

Visibility payload:

```text
x = frame-local instance index
y = render triangle index
```

The frame-local instance table resolves to stable owner and geometry identities. The geometry rendering packet resolves triangle to stable source/editable face.

Adopt reverse-Z only as one complete camera/depth decision. If selected, projection, clear value, compare operation, reconstruction, grid, picking, and wire depth tests change together.

First pass constraints:

- hardware indexed-triangle rasterization;
- opaque geometry only;
- back-face policy comes from the instance/material declaration;
- no texture sampling in visibility;
- cutout waits until direct opacity binding exists;
- one simple PSO family, not one visibility PSO per material.

### Stage 5 — fixed white dielectric shading

Add a compute material-resolve recording:

```text
reads:  VisibilityIndex + DepthSurface + geometry/instance buffers + light/sky state
writes: RadianceSurface
```

For the first visible milestone use fixed values:

```text
AlbedoColour = white
Metallic = 0
F0 = 0.04
SpecularRoughness = 0.5
AmbientOcclusion = 1
Emission = 0
```

The resolve reconstructs or fetches:

- triangle vertices;
- barycentrics;
- interpolated world position;
- normal and tangent frame;
- camera vector;
- derivatives required for later texture sampling.

It shades only a valid winning visibility record. Background pixels preserve the analytic sky. This milestone uses the directional sun and does not wait for material processing.

### Stage 6 — wire presentations

Implement two distinct modes:

- **Triangulated wire:** every raster triangle edge, including derived diagonals.
- **Source-topology wire:** only retained source/editable polygon edges.

Both depth-test against `DepthSurface`. Use screen-space edge quads or barycentric coverage rather than relying on inconsistent wide hardware lines.

### Stage 7 — visible selection

Use visibility and depth for object and face picking. Preserve the chain:

```text
screen pixel
  → frame instance + render triangle
  → stable owner + geometry identity + source/editable face
  → persistent SelectionSet
```

Selection highlighting must remain visible after pointer release and must still work after viewport tabs slide.

Edge and vertex selection follow after object/face selection. Region selection uses bounded GPU reduction, not a full visibility-image readback.

### Stage 8 — cutout and transparent geometry

After opaque geometry is proven:

- add cutout visibility with an explicit opacity source and threshold;
- keep blended/transmissive surfaces out of the single-hit opaque visibility assumption;
- render glass after opaque radiance is available;
- use `TransmissionIndex`/depth layers or another explicitly scheduled transparent path;
- support reflection plus refraction, not alpha-only glass;
- start with thin glass, then closed-volume attenuation, then rough transmission/dispersion;
- treat order-independent transparency as a measured design choice, not an automatic requirement.

## Visibility buffer versus micro-rasterization

### Visibility buffer: use now

A visibility buffer is valuable even with ordinary hardware rasterization:

- opaque rasterization is largely independent of material complexity;
- hidden fragments do not run expensive material shading;
- the winning fragment is shaded once per pixel;
- depth-correct picking receives stable primitive identity;
- the target is much smaller than a wide G-buffer;
- material classification and shading can be scheduled after visibility;
- ray queries and raster hits can eventually feed a similar closure resolver.

Costs that must be accepted:

- attributes and barycentrics are reconstructed manually;
- texture derivatives need explicit reconstruction;
- geometry and texture resources need indexed/bindless-style access;
- alpha-tested and transparent surfaces need special paths;
- MSAA is less straightforward than a classic forward pass.

Slate already declares `VisibilityIndex`, so this is the correct first architecture.

### Software micro-rasterization: defer

Software micro-rasterization is useful when a scene contains enormous quantities of triangles near or below pixel size. It can avoid poor 2×2 pixel-quad utilisation, integrate tightly with cluster culling and cluster LOD, reduce CPU draw submission, and write the same visibility representation as the hardware path.

It is **not** automatically faster for normal game geometry, editable modelling meshes, low-poly objects, skinned/deformed content, masked foliage, or transparent surfaces. It also does not create the main visibility-buffer benefits by itself; those already come from visibility-first rendering.

Implementing it well requires a much larger system:

- offline/runtime cluster construction;
- hierarchical cluster LOD and error metrics;
- geometry page streaming and residency;
- GPU instance/cluster culling and indirect dispatch;
- conservative subpixel coverage and depth atomics;
- a hybrid hardware/software raster decision;
- attribute reconstruction across cluster encodings;
- masked-material and deformation policies;
- stable selection/source mappings across generated LOD clusters;
- extensive per-GPU profiling.

Decision:

```text
Now:   HardwareRaster → VisibilityIndex
Later: HardwareRaster ─┐
                       ├→ shared VisibilityIndex → shared shading/picking
       ClusterMicroRaster┘
```

A future `ClusterMicroRaster` path is justified only when captured Slate workloads show that subpixel triangle density or draw submission is a dominant cost. It is most relevant to dense static scans, CAD tessellation, sculpted assets, and high-detail game environments. It is not a prerequisite for rendering geometry or for using a visibility buffer.

## Implementation sequence and proof gates

### Rendering milestone R1 — atomic scene intake

- neutral decoded scene and recipes;
- MTL auxiliary bundle contract;
- outliner preflight and atomic registration;
- stable component mappings.

Proof: hierarchy import success, capacity refusal with zero mutation, stale identity rejection.

### Rendering milestone R2 — upload

- `ByteSpace` reservations;
- rebased FP32 vertex stream;
- index and triangle/source mapping upload;
- generation-safe retirement.

Proof: numerical large-coordinate rebasing, byte ranges, revision-limited updates, retirement after cycle completion.

### Rendering milestone R3 — first actual geometry

- classic depth/visibility construct;
- hardware triangle raster;
- fixed white dielectric compute resolve;
- analytic sky background;
- actual imported OBJ in the real viewport.

Proof:

- actual-panel screenshot, not a mock;
- geometry occludes itself correctly;
- camera motion and FOV/clipping work;
- a large-world-position object remains stable after rebasing;
- render capture confirms visibility and depth targets;
- resize recreates every display-relative target/span correctly.

### Rendering milestone R4 — topology presentations

- shaded, triangulated-wire, and source-wire modes;
- depth-correct overlays;
- mode switch in the real viewport.

Proof: retained quad shows four source edges and five triangulated edges.

### Rendering milestone R5 — object/face selection

- bounded visibility readback;
- stable owner/face resolution;
- persistent highlighted selection;
- tab-slide retest.

Proof: front face wins over occluded face and remains highlighted after release.

### Rendering milestone R6 — transforms and gizmos

- optional transform component;
- instance transform upload;
- depth-aware translate/rotate/scale gizmos;
- undoable edits.

### Rendering milestone R7 — advanced geometry throughput

Only after profiling:

- GPU-driven instance/cluster culling;
- mesh-shader path where available;
- cluster LOD and streaming;
- optional software micro-raster prototype behind the shared visibility contract.

### Material milestone M1 — schema migration

Resume only after R3–R5:

- 64-bit `ChannelSet`;
- composable physical-surface declarations;
- complete signal schema above;
- imported glTF/OpenPBR mapping and diagnostics;
- fixed renderer closure matching the schema.

### Material milestone M2 onward — held work

Still on hold:

- layer/image/analytic source resolution;
- GPU layer blending;
- dirty tiles and transient image lifetime;
- processed-material viewport binding;
- export readback and texture packing.

## Validation requirements

### Material contract tests

When material implementation resumes:

- dielectric F0 derived from IOR matches the declared equation;
- direct F0 and IOR modes never become dual authorities;
- white furnace tests cover base, coat, fuzz, and transmission combinations;
- EON preserves energy over sampled roughness/view/light domains;
- data signals never receive colour transfer functions;
- unused channels are not sampled;
- thin-wall and thick-volume glass produce distinct paths;
- import mappings report unsupported semantics rather than dropping them silently.

### Rendering tests

- visibility IDs reproduce stable owner and source-face mappings;
- rebasing occurs in FP64 before FP32 narrowing;
- depth and visibility clear values are deterministic;
- background/invalid visibility cannot fetch geometry;
- source wire and triangulated wire remain distinct;
- cutout and transmission never silently use opaque semantics;
- device resources retire by completed cycle and generation;
- first visible proof is captured from the actual Slate panel.

## Research basis

The material direction is based on:

- OpenPBR Surface 1.1.1: layered base, thin film, coat, fuzz, transmission, subsurface, physical parameter reference, and furnace testing;
- Unreal Substrate: measured slab/interface concepts, F0/F90, roughness, anisotropy, MFP, fuzz, second roughness, and quality-dependent simplification;
- Khronos glTF PBR: interoperable metallic/roughness plus anisotropy, clearcoat, dispersion, emissive strength, IOR, iridescence, sheen, specular, transmission, and volume;
- EON: energy-preserving rough diffuse reflection with analytical compensation.

These sources guide Slate's neutral contract; they do not make Slate's document schema an Unreal, MaterialX, or glTF serialization.
