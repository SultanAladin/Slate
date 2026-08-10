//============================================================================================================================================
//                                                             ENGINE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Root symbol index — every source folder in Engine, one line each.

%format    symbolindex 1.0
%scope     root
%path      Engine
%layers    8
%folders   135
%symbols   2479
%protocol  root → layer → folder; never open a source file the folder index already answers

//------------------------------------------------------------------------------------------------------------------------
//                                                      APPLICATION
//------------------------------------------------------------------------------------------------------------------------

I Source | Application | Engine/Application/ConsoleHost/Source/Source.symbolindex | 1 src | 24 sym | Headless bring-up in link order — every unit constructed, reported, and reclaimed without a window.

//------------------------------------------------------------------------------------------------------------------------
//                                                        CONTRACT
//------------------------------------------------------------------------------------------------------------------------

I Contract | Contract | Engine/Contract/Contract.symbolindex | 6 src | 79 sym | The five combination behaviours `22` §3 declares — read by impressions, by layer entries and by cell content alike.

//------------------------------------------------------------------------------------------------------------------------
//                                                         SHARED
//------------------------------------------------------------------------------------------------------------------------

I Shared | Shared | Engine/Shared/Shared.symbolindex | 9 src | 93 sym | The scattering medium, the phase functions and the three surface parameterisations — one source, both toolchains.

//------------------------------------------------------------------------------------------------------------------------
//                                                      SLATECOMPUTE
//------------------------------------------------------------------------------------------------------------------------

I Api    | SlateCompute | Engine/SlateCompute/Compute/AnalyticProjection/Api/Api.symbolindex         | 1 src | 19 sym  | `70` — resolution-free sources resolved into a tile at promotion, at that tile's own reduction level.
I Source | SlateCompute | Engine/SlateCompute/Compute/AnalyticProjection/Source/Source.symbolindex   | 1 src | 17 sym  | The four sources resolved at a domain position, the sequence composed over them, and the tile walk that flattens once.
I Api    | SlateCompute | Engine/SlateCompute/Compute/AtmosphereIntegrator/Api/Api.symbolindex       | 1 src | 45 sym  | Three resident lookup surfaces replacing per-pixel marching — and the only source of environmental light in Slate.
I Shader | SlateCompute | Engine/SlateCompute/Compute/AtmosphereIntegrator/Shader/Shader.symbolindex | 5 src | 29 sym  | The one uniform block every atmosphere entry point reads, and its widening into the shared medium profile.
I Source | SlateCompute | Engine/SlateCompute/Compute/AtmosphereIntegrator/Source/Source.symbolindex | 1 src | 42 sym  | The three surfaces in construction order, the spectral coefficients behind them, and the convolution derived on rebuild.
I Api    | SlateCompute | Engine/SlateCompute/Compute/ChartPartition/Api/Api.symbolindex             | 1 src | 13 sym  | The parametric domain every paintable surface addresses — cut, flattened, arranged, and measured.
I Source | SlateCompute | Engine/SlateCompute/Compute/ChartPartition/Source/Source.symbolindex       | 1 src | 15 sym  | Seam-bounded flood fill, boundary chaining, exact fold classification, and subdivision that terminates.
I Api    | SlateCompute | Engine/SlateCompute/Compute/DomainSpace/Api/Api.symbolindex                | 1 src | 10 sym  | Charts arranged into the unit domain at a common scale, separated by at least one apron.
I Source | SlateCompute | Engine/SlateCompute/Compute/DomainSpace/Source/Source.symbolindex          | 1 src | 6 sym   | Scale-invariant shelf ordering, bisection to the common scale, and the occupancy it reports.
I Api    | SlateCompute | Engine/SlateCompute/Compute/ImpressionSequence/Api/Api.symbolindex         | 1 src | 26 sym  | A stroke as ordered impressions against the parametric domain — resampled by arrival, deferred never dropped, sealed once.
I Source | SlateCompute | Engine/SlateCompute/Compute/ImpressionSequence/Source/Source.symbolindex   | 1 src | 25 sym  | Domain resampling from arrival stamps, the deferral that never coarsens, and the accumulation applied once.
I Api    | SlateCompute | Engine/SlateCompute/Compute/ParityRunner/Api/Api.symbolindex               | 1 src | 6 sym   | Proves the host form and the shader form of a Shared/ entry point agree at the declared guarantee.
I Source | SlateCompute | Engine/SlateCompute/Compute/ParityRunner/Source/Source.symbolindex         | 1 src | 7 sym   | Registration and comparison over the common sample set.
I Api    | SlateCompute | Engine/SlateCompute/Compute/PromotionScheduler/Api/Api.symbolindex         | 1 src | 24 sym  | Two independent budgets, spent per rotation, and the ordering eviction follows when they cannot be met.
I Source | SlateCompute | Engine/SlateCompute/Compute/PromotionScheduler/Source/Source.symbolindex   | 1 src | 17 sym  | The two measures charged apart, the total eviction order, and the cost read from `56`'s entries.
I Api    | SlateCompute | Engine/SlateCompute/Compute/RequestQueue/Api/Api.symbolindex               | 1 src | 17 sym  | Device-written cell demands, drained with latency — the readback half named apart, and the arrival order it yields.
I Source | SlateCompute | Engine/SlateCompute/Compute/RequestQueue/Source/Source.symbolindex         | 1 src | 13 sym  | Coalescing by cell, the cyclic rotation slots, and the readback that is exactly one depth behind.
I Api    | SlateCompute | Engine/SlateCompute/Compute/SeamSpecification/Api/Api.symbolindex          | 1 src | 13 sym  | Where the topology is cut — authored seams that survive every re-partition, derived seams that do not.
I Source | SlateCompute | Engine/SlateCompute/Compute/SeamSpecification/Source/Source.symbolindex    | 1 src | 11 sym  | Two separately stored sets, and the reclamation that reaches only one of them.
I Api    | SlateCompute | Engine/SlateCompute/Compute/StrokeSpace/Api/Api.symbolindex                | 1 src | 12 sym  | The bounded extent one stroke accumulates into — coverage per texel, per touched cell, applied once at Seal.
I Source | SlateCompute | Engine/SlateCompute/Compute/StrokeSpace/Source/Source.symbolindex          | 1 src | 9 sym   | Sparse tile claiming over the dense cell index, and the commutative coverage accumulation.
I Api    | SlateCompute | Engine/SlateCompute/Compute/SurfaceDepot/Api/Api.symbolindex               | 1 src | 15 sym  | Derived, evictable, reconstructible artefacts keyed by content — and nothing authored, ever.
I Source | SlateCompute | Engine/SlateCompute/Compute/SurfaceDepot/Source/Source.symbolindex         | 1 src | 11 sym  | The reconstructibility refusal, key resolution, and least-recently-resolved eviction.
I Api    | SlateCompute | Engine/SlateCompute/Compute/SurfaceTileSpace/Api/Api.symbolindex           | 1 src | 40 sym  | Resolution-independent paintable surfaces — the cell subdivision, its residency, and the sample that never stalls.
I Source | SlateCompute | Engine/SlateCompute/Compute/SurfaceTileSpace/Source/Source.symbolindex     | 1 src | 28 sym  | The coarsening walk that never stalls, the revision comparison, and promotion that evicts only what it may.
I Api    | SlateCompute | Engine/SlateCompute/Compute/TileSpace/Api/Api.symbolindex                  | 1 src | 16 sym  | Physical tile extents, sliced and reclaimed — slot ledger and byte offsets, and never a texel.
I Source | SlateCompute | Engine/SlateCompute/Compute/TileSpace/Source/Source.symbolindex            | 1 src | 12 sym  | Claim, release into quarantine, and the reclamation deferred by the rotation depth.
I Api    | SlateCompute | Engine/SlateCompute/Compute/VisibilityIndex/Api/Api.symbolindex            | 6 src | 103 sym | The hierarchical minimum `16` §2 ② reduces one rotation's depth into, and which level a projected extent is tested at.
I Shader | SlateCompute | Engine/SlateCompute/Compute/VisibilityIndex/Shader/Shader.symbolindex      | 6 src | 29 sym  | `16` §2 ② — one dispatch per level, reducing the depth target and then the chain into its own coarser levels.
I Source | SlateCompute | Engine/SlateCompute/Compute/VisibilityIndex/Source/Source.symbolindex      | 6 src | 76 sym  | Halving a display extent into a level chain, and the integer logarithm that picks the level one partition is tested at.

//------------------------------------------------------------------------------------------------------------------------
//                                                     SLATEDOCUMENT
//------------------------------------------------------------------------------------------------------------------------

I Api    | SlateDocument | Engine/SlateDocument/Document/AssetInterchange/Api/Api.symbolindex            | 1 src | 17 sym | Topology and imagery in, painted channels out — one contract, and intake that never repairs.
I Source | SlateDocument | Engine/SlateDocument/Document/AssetInterchange/Source/Source.symbolindex      | 1 src | 10 sym | Faithful enrolment, unit scale applied once, and the emission validated before anything is resolved.
I Api    | SlateDocument | Engine/SlateDocument/Document/BrushSpecification/Api/Api.symbolindex          | 1 src | 41 sym | The brush every stroke in `22` is resolved against — a shape, a spacing, a channel set, and dynamics that read input.
I Source | SlateDocument | Engine/SlateDocument/Document/BrushSpecification/Source/Source.symbolindex    | 1 src | 26 sym | Declared progressions, the fallback an absent axis takes, and the stroke-seeded sequence both sides share.
I Api    | SlateDocument | Engine/SlateDocument/Document/CameraProjection/Api/Api.symbolindex            | 1 src | 39 sym | Where the viewer is and how a document position becomes a display position — one answer, read by twelve documents.
I Source | SlateDocument | Engine/SlateDocument/Document/CameraProjection/Source/Source.symbolindex      | 1 src | 27 sym | Reversed-depth projection derivation, plane extraction, the gesture lifecycle, and the framing solve.
I Api    | SlateDocument | Engine/SlateDocument/Document/DecalProjection/Api/Api.symbolindex             | 1 src | 26 sym | Placed content — a source, a transform stored against the surface, and the extent it covers in the domain.
I Source | SlateDocument | Engine/SlateDocument/Document/DecalProjection/Source/Source.symbolindex       | 1 src | 21 sym | The inverse of a decomposed placing transform, the two extent derivations, and the drag that records nothing until release.
I Api    | SlateDocument | Engine/SlateDocument/Document/EnrollmentIndex/Api/Api.symbolindex             | 1 src | 14 sym | Which slots are enrolled in a named subset, compressed by interval rather than stored per occupant.
I Source | SlateDocument | Engine/SlateDocument/Document/EnrollmentIndex/Source/Source.symbolindex       | 1 src | 11 sym | Interval merging, division, and the exclusion refusal that precedes every write.
I Api    | SlateDocument | Engine/SlateDocument/Document/IlluminantPopulation/Api/Api.symbolindex        | 1 src | 28 sym | The illuminants `18` integrates — every one an occupant, every one with a size, every extent declared not derived.
I Source | SlateDocument | Engine/SlateDocument/Document/IlluminantPopulation/Source/Source.symbolindex  | 1 src | 23 sym | Size validation, the single atmospheric source, incidence projection, and the reach index.
I Api    | SlateDocument | Engine/SlateDocument/Document/IntakeIndex/Api/Api.symbolindex                 | 1 src | 9 sym  | What arrived, from where, and what was assumed about it — never an assumption made silently.
I Source | SlateDocument | Engine/SlateDocument/Document/IntakeIndex/Source/Source.symbolindex           | 1 src | 6 sym  | Arrival-ordered records, and the once-only report of every assumption among them.
I Api    | SlateDocument | Engine/SlateDocument/Document/MaterialSpecification/Api/Api.symbolindex       | 1 src | 32 sym | What a surface's channels are, where each value comes from, and which partition resolves to which occupant.
I Source | SlateDocument | Engine/SlateDocument/Document/MaterialSpecification/Source/Source.symbolindex | 1 src | 23 sym | The channel inventory per reflectance, declaration validation, and the partition resolution.
I Api    | SlateDocument | Engine/SlateDocument/Document/OutlinerSequence/Api/Api.symbolindex            | 1 src | 27 sym | The fixed tick order over both relations, the linearisation, the subsets and the name search.
I Source | SlateDocument | Engine/SlateDocument/Document/OutlinerSequence/Source/Source.symbolindex      | 1 src | 23 sym | The seven steps in order, every mutation a transaction, and the retirement cascade as one of them.
I Api    | SlateDocument | Engine/SlateDocument/Document/PointerIntersection/Api/Api.symbolindex         | 1 src | 14 sym | What is under the pointer — resolved on the host, every sample, as one tuple and never as several answers.
I Source | SlateDocument | Engine/SlateDocument/Document/PointerIntersection/Source/Source.symbolindex   | 1 src | 10 sym | The unprojection, the one traversal that resolves the whole tuple, and the marquee as a narrower camera.
I Api    | SlateDocument | Engine/SlateDocument/Document/PopulationIndex/Api/Api.symbolindex             | 1 src | 10 sym | Generationally versioned slot ledger — the population every occupant of the document sits inside.
I Source | SlateDocument | Engine/SlateDocument/Document/PopulationIndex/Source/Source.symbolindex       | 1 src | 8 sym  | Slot issuance, withdrawal and generational resolution.
I Api    | SlateDocument | Engine/SlateDocument/Document/PropertySpecification/Api/Api.symbolindex       | 1 src | 15 sym | Typed, named, validated property declarations — validation part of the declaration, never a later step.
I Source | SlateDocument | Engine/SlateDocument/Document/PropertySpecification/Source/Source.symbolindex | 1 src | 11 sym | The validation each measure declares, the bounding offered beside it, and the write that refuses.
I Api    | SlateDocument | Engine/SlateDocument/Document/RevisionSequence/Api/Api.symbolindex            | 1 src | 11 sym | Ordered, scrubbable sequence of committed transactions, with the drag lifecycle every edit uses.
I Source | SlateDocument | Engine/SlateDocument/Document/RevisionSequence/Source/Source.symbolindex      | 1 src | 8 sym  | The drag lifecycle, declared merging, and scrubbing in both directions.
I Api    | SlateDocument | Engine/SlateDocument/Document/RowSequence/Api/Api.symbolindex                 | 1 src | 18 sym | Depth-first linearisation of the enclosure relation, and the counted ordering that scrolls it.
I Source | SlateDocument | Engine/SlateDocument/Document/RowSequence/Source/Source.symbolindex           | 1 src | 17 sym | The depth-first walk, and the binary-indexed counts that answer both scroll questions.
I Api    | SlateDocument | Engine/SlateDocument/Document/SceneStructure/Api/Api.symbolindex              | 1 src | 40 sym | The two nesting relations over the population — organisational enclosure and kinematic attachment, apart.
I Source | SlateDocument | Engine/SlateDocument/Document/SceneStructure/Source/Source.symbolindex        | 1 src | 28 sym | Enclosure ordering, gapped label assignment and repair, and downward attachment compounding.
I Api    | SlateDocument | Engine/SlateDocument/Document/SelectionSequence/Api/Api.symbolindex           | 1 src | 9 sym  | Selection ordering, revised in its own sequence, session-scoped, restored alongside the transaction it served.
I Source | SlateDocument | Engine/SlateDocument/Document/SelectionSequence/Source/Source.symbolindex     | 1 src | 7 sym  | Sealing, traversal, and the restoration that pairs a document scrub with the selection it served.
I Api    | SlateDocument | Engine/SlateDocument/Document/SpatialSubdivision/Api/Api.symbolindex          | 1 src | 40 sym | Three subdivisions, two levels, one traversal — what lets `74` answer the pointer on the host, every sample.
I Source | SlateDocument | Engine/SlateDocument/Document/SpatialSubdivision/Source/Source.symbolindex    | 1 src | 38 sym | Octant division, nearest-first descent, exact face classification, and refit without rebuild.
I Api    | SlateDocument | Engine/SlateDocument/Document/SurfaceLayerSequence/Api/Api.symbolindex        | 1 src | 32 sym | The ordered content of one surface — the single source of truth a resident tile is a projection of.
I Source | SlateDocument | Engine/SlateDocument/Document/SurfaceLayerSequence/Source/Source.symbolindex  | 1 src | 24 sym | Ordering by position alone, amendments bounded by what they touched, and the one resampling that is reported.
I Api    | SlateDocument | Engine/SlateDocument/Document/TilingSpecification/Api/Api.symbolindex         | 1 src | 25 sym | A repeating pattern as plane symmetry plus cell content — declared, deterministic, and resolving nothing.
I Source | SlateDocument | Engine/SlateDocument/Document/TilingSpecification/Source/Source.symbolindex   | 1 src | 16 sym | Lattice validation, the per-cell variation that is a permutation rather than a sample, and the nesting bound.
I Api    | SlateDocument | Engine/SlateDocument/Document/ToolSequence/Api/Api.symbolindex                | 1 src | 37 sym | Everything the application holds that is not the document and not a panel's own layout — with exactly one owner.
I Source | SlateDocument | Engine/SlateDocument/Document/ToolSequence/Source/Source.symbolindex          | 1 src | 27 sym | Tool declaration, the arbitration both units ask, and the capture that persists for a whole drag.
I Api    | SlateDocument | Engine/SlateDocument/Document/TopologyConditioning/Api/Api.symbolindex        | 1 src | 24 sym | Derived companions to imported topology — adjacency, welding, orientation, bases and extents. Never a mutation.
I Source | SlateDocument | Engine/SlateDocument/Document/TopologyConditioning/Source/Source.symbolindex  | 1 src | 26 sym | Lattice welding, corner adjacency, orientation consistency, and conservative extents.
I Api    | SlateDocument | Engine/SlateDocument/Document/TopologyStructure/Api/Api.symbolindex           | 1 src | 28 sym | Polygon topology exactly as it arrived — sealed once, never mutated, and never repaired.
I Source | SlateDocument | Engine/SlateDocument/Document/TopologyStructure/Source/Source.symbolindex     | 1 src | 24 sym | Corner run assembly and the seal that closes it.
I Api    | SlateDocument | Engine/SlateDocument/Document/TrigramIndex/Api/Api.symbolindex                | 1 src | 10 sym | Name search that narrows by trigram and then confirms exactly — approximate index, exact answer.
I Source | SlateDocument | Engine/SlateDocument/Document/TrigramIndex/Source/Source.symbolindex          | 1 src | 8 sym  | Trigram folding and entry, the rarest-run narrowing, and the exact confirmation over it.
I Api    | SlateDocument | Engine/SlateDocument/Document/VectorInterchange/Api/Api.symbolindex           | 1 src | 25 sym | Vector outlines and typeface outlines as one thing — the accepted subset, and every refusal positioned.
I Source | SlateDocument | Engine/SlateDocument/Document/VectorInterchange/Source/Source.symbolindex     | 1 src | 16 sym | Declaration by either route, flattening at a supplied tolerance, and classification per declared rule.
I Api    | SlateDocument | Engine/SlateDocument/Format/FormatCodec/Api/Api.symbolindex                   | 1 src | 4 sym  | Versioned document stream layout and its declared migrations — never a conditional inside a reader.
I Source | SlateDocument | Engine/SlateDocument/Format/FormatCodec/Source/Source.symbolindex             | 1 src | 2 sym  | Migration chain resolution over the declared version transformations.

//------------------------------------------------------------------------------------------------------------------------
//                                                       SLATEMATH
//------------------------------------------------------------------------------------------------------------------------

I Api    | SlateMath | Engine/SlateMath/Numeric/ColourProjection/Api/Api.symbolindex           | 1 src | 23 sym | A coordinate and the space it is a coordinate in — never a bare triple, and never an assumed encoding.
I Source | SlateMath | Engine/SlateMath/Numeric/ColourProjection/Source/Source.symbolindex     | 1 src | 10 sym | Primaries derived from chromaticities, the transfers, von Kries adaptation, and the Planckian locus.
I Api    | SlateMath | Engine/SlateMath/Numeric/CurveSolver/Api/Api.symbolindex                | 1 src | 7 sym  | Planar path evaluation, flattening to a tolerance, and stroke offsetting — the mechanism `52` resolves with.
I Source | SlateMath | Engine/SlateMath/Numeric/CurveSolver/Source/Source.symbolindex          | 1 src | 8 sym  | Adaptive subdivision, endpoint arc parameterisation, and bevelled offsetting.
I Api    | SlateMath | Engine/SlateMath/Numeric/QuadratureIntegrator/Api/Api.symbolindex       | 1 src | 9 sym  | Definite integral approximation over a declared domain — derived abscissae, ordered accumulation, no transcribed set.
I Source | SlateMath | Engine/SlateMath/Numeric/QuadratureIntegrator/Source/Source.symbolindex | 1 src | 6 sym  | Newton on the Legendre recurrence, solved over half the interval and mirrored onto the other.
I Api    | SlateMath | Engine/SlateMath/Numeric/ReportSequence/Api/Api.symbolindex             | 1 src | 17 sym | The session's reports and its sampled measures — one appended once, one overwritten, and never confused.
I Source | SlateMath | Engine/SlateMath/Numeric/ReportSequence/Source/Source.symbolindex       | 1 src | 14 sym | Coalescing, the bounded cyclic retention, and the overwriting measure index beside it.
I Api    | SlateMath | Engine/SlateMath/Numeric/SpectralProjection/Api/Api.symbolindex         | 1 src | 8 sym  | Wavelength domain to tristimulus — the colour-matching functions, analytic, never three sampled wavelengths.
I Source | SlateMath | Engine/SlateMath/Numeric/SpectralProjection/Source/Source.symbolindex   | 1 src | 3 sym  | The nine-lobe fit, and the normalisation derived from it rather than beside it.
I Api    | SlateMath | Engine/SlateMath/Numeric/TransformProjection/Api/Api.symbolindex        | 1 src | 13 sym | Decomposed transforms, their composition, and the rebasing that precedes every narrowing to 32-bit.
I Source | SlateMath | Engine/SlateMath/Numeric/TransformProjection/Source/Source.symbolindex  | 1 src | 4 sym  | Quaternion composition, matrix derivation, and the 64-bit rebasing subtraction.
I Api    | SlateMath | Engine/SlateMath/Numeric/UnwrapSolver/Api/Api.symbolindex               | 1 src | 6 sym  | Boundary-first parameterisation — Convergent, and held to reporting which criterion terminated it.
I Source | SlateMath | Engine/SlateMath/Numeric/UnwrapSolver/Source/Source.symbolindex         | 1 src | 7 sym  | Chord-length boundary mapping, mean-value interior weights, and relaxation against a declared criterion.
I Api    | SlateMath | Engine/SlateMath/Numeric/WorkSequence/Api/Api.symbolindex               | 1 src | 39 sym | The only thread creation in the repository — declared work, immutable inputs, results applied on the tick.
I Source | SlateMath | Engine/SlateMath/Numeric/WorkSequence/Source/Source.symbolindex         | 1 src | 23 sym | The reserved interactive worker, cooperative cancellation, and conclusions ordered by declaration.
I Api    | SlateMath | Engine/SlateMath/Platform/InputExchange/Api/Api.symbolindex             | 1 src | 7 sym  | Timestamped device samples crossing in, with absent axes distinguishable from zero-valued ones.
I Source | SlateMath | Engine/SlateMath/Platform/InputExchange/Source/Source.symbolindex       | 1 src | 4 sym  | Bounded cyclic arrival ordering over pointer samples.
I Api    | SlateMath | Engine/SlateMath/Platform/TickSequence/Api/Api.symbolindex              | 1 src | 5 sym  | Monotonically increasing ordering points, stamped at arrival and never derived at consumption.
I Source | SlateMath | Engine/SlateMath/Platform/TickSequence/Source/Source.symbolindex        | 1 src | 5 sym  | Host timeline over the operating system's monotonic counter.
I Api    | SlateMath | Engine/SlateMath/Platform/WindowInterchange/Api/Api.symbolindex         | 1 src | 8 sym  | One window surface over three window systems — surrenders the native handle and nothing else.
I Source | SlateMath | Engine/SlateMath/Platform/WindowInterchange/Source/Source.symbolindex   | 1 src | 9 sym  | Windowing over GLFW, linked dynamically through glfw3dll.lib against glfw3.dll.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SLATEUI
//------------------------------------------------------------------------------------------------------------------------

I Api    | SlateUI | Engine/SlateUI/Interface/InterfaceExchange/Api/Api.symbolindex       | 1 src | 10 sym | The one seam the interface library crosses — device handles in, recorded commands out, no ImGui spelling.
I Source | SlateUI | Engine/SlateUI/Interface/InterfaceExchange/Source/Source.symbolindex | 1 src | 9 sym  | The only translation unit in the engine that includes ImGui.
I Api    | SlateUI | Engine/SlateUI/Interface/OutlinerPanel/Api/Api.symbolindex           | 1 src | 10 sym | Presents RowSequence through RankIndex and writes intent back — holding no relation of its own.
I Source | SlateUI | Engine/SlateUI/Interface/OutlinerPanel/Source/Source.symbolindex     | 1 src | 15 sym | The counted span presented, and every gesture over it turned into a declared intent.

//------------------------------------------------------------------------------------------------------------------------
//                                                      SLATEVULKAN
//------------------------------------------------------------------------------------------------------------------------

I Api    | SlateVulkan | Engine/SlateVulkan/Device/AttachmentIndex/Api/Api.symbolindex        | 1 src | 17 sym | The classic render constructs `06` §2.1 settled on, declared over the shared targets and re-derived on an extent change.
I Source | SlateVulkan | Engine/SlateVulkan/Device/AttachmentIndex/Source/Source.symbolindex  | 1 src | 11 sym | The construct declared from the claimed formats, the span derived over the claimed views, and the two reclamations.
I Api    | SlateVulkan | Engine/SlateVulkan/Device/ByteSpace/Api/Api.symbolindex              | 1 src | 19 sym | Raw device byte extents, claimed in a few large pieces and sliced into the spans every resource sits in.
I Source | SlateVulkan | Engine/SlateVulkan/Device/ByteSpace/Source/Source.symbolindex        | 1 src | 12 sym | Residency scoring, the first-fit slice, and the coalescing release that keeps an extent from fragmenting away.
I Api    | SlateVulkan | Engine/SlateVulkan/Device/CommandSequence/Api/Api.symbolindex        | 1 src | 11 sym | One recording per rotation slot — where commands are written, and the ordered surrender of them to the queue.
I Source | SlateVulkan | Engine/SlateVulkan/Device/CommandSequence/Source/Source.symbolindex  | 1 src | 8 sym  | The per-slot recording extents, the open that resets one whole, and the surrender to the one graphics queue.
I Api    | SlateVulkan | Engine/SlateVulkan/Device/CycleScheduler/Api/Api.symbolindex         | 1 src | 11 sym | Orders reuse of N cyclic recording slots — the wait that makes a slot writable and the ordinal that names it.
I Source | SlateVulkan | Engine/SlateVulkan/Device/CycleScheduler/Source/Source.symbolindex   | 1 src | 9 sym  | The ordering points of every cyclic slot, the bounded wait that reclaims one, and the advance that cycles them.
I Api    | SlateVulkan | Engine/SlateVulkan/Device/DescriptorIndex/Api/Api.symbolindex        | 1 src | 18 sym | Descriptor set layouts constructed once at bring-up, and explicit sets claimed one per rotation slot.
I Source | SlateVulkan | Engine/SlateVulkan/Device/DescriptorIndex/Source/Source.symbolindex  | 1 src | 12 sym | The layout declaration that closes at bring-up, the extent it is sized against, and the per-rotation write.
I Api    | SlateVulkan | Engine/SlateVulkan/Device/ImageSpace/Api/Api.symbolindex             | 1 src | 18 sym | Device image extents — claimed against a declared shape, viewed once, and carrying the layout each one stands in.
I Source | SlateVulkan | Engine/SlateVulkan/Device/ImageSpace/Source/Source.symbolindex       | 1 src | 13 sym | The image claim, the one place a layout transition is recorded, and the reclamation that returns both.
I Api    | SlateVulkan | Engine/SlateVulkan/Device/ProgramIndex/Api/Api.symbolindex           | 1 src | 15 sym | Graphics and compute programs constructed once at bring-up, against the layouts and modules already declared.
I Source | SlateVulkan | Engine/SlateVulkan/Device/ProgramIndex/Source/Source.symbolindex     | 1 src | 8 sym  | The layout every program reaches through, the two construction routes, and the reclamation that returns both.
I Api    | SlateVulkan | Engine/SlateVulkan/Device/RenderSchedule/Api/Api.symbolindex         | 1 src | 16 sym | What is recorded in a rotation slot, in what order, and against which shared targets.
I Source | SlateVulkan | Engine/SlateVulkan/Device/RenderSchedule/Source/Source.symbolindex   | 1 src | 10 sym | Contribution gating and the ordering derived from declared reads and writes.
I Api    | SlateVulkan | Engine/SlateVulkan/Device/ShaderCodec/Api/Api.symbolindex            | 1 src | 13 sym | Lowered shader streams — read once, verified as SPIR-V, held as vendor modules and specialised at construction.
I Source | SlateVulkan | Engine/SlateVulkan/Device/ShaderCodec/Source/Source.symbolindex      | 1 src | 7 sym  | The whole-file read, the stream verification that refuses before the vendor sees it, and the held specialisation.
I Api    | SlateVulkan | Engine/SlateVulkan/Device/SpanSpace/Api/Api.symbolindex              | 1 src | 17 sym | Device linear extents, each sliced out of ByteSpace and each declaring what the device is permitted to read it as.
I Source | SlateVulkan | Engine/SlateVulkan/Device/SpanSpace/Source/Source.symbolindex        | 1 src | 11 sym | The claim, the host write, the recorded transfer and the release of every linear device extent the engine holds.
I Api    | SlateVulkan | Engine/SlateVulkan/Device/VendorClassifier/Api/Api.symbolindex       | 1 src | 2 sym  | Scores vendor implementations into a capability set, once, at bring-up and at recovery.
I Source | SlateVulkan | Engine/SlateVulkan/Device/VendorClassifier/Source/Source.symbolindex | 1 src | 1 sym  | Enumerated device scored into a capability set and a ranking.
I Api    | SlateVulkan | Engine/SlateVulkan/Device/VulkanExchange/Api/Api.symbolindex         | 1 src | 11 sym | Loader C-ABI, instance and device handles crossing the vendor edge.
I Source | SlateVulkan | Engine/SlateVulkan/Device/VulkanExchange/Source/Source.symbolindex   | 1 src | 9 sym  | Instance construction, device scoring and the one graphics queue.
I Api    | SlateVulkan | Engine/SlateVulkan/Device/WindowExchange/Api/Api.symbolindex         | 1 src | 2 sym  | Native window handle ⇄ VkSurfaceKHR — the one place the window system meets the vendor edge.
I Source | SlateVulkan | Engine/SlateVulkan/Device/WindowExchange/Source/Source.symbolindex   | 1 src | 2 sym  | The surface conversion, taken through the window system that produced the handle.
