//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 `62` — cutout resolved at `16` and never here, transmissive collected into a bounded sorted column, and amended back to front.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/TransmissionSequence/Api
%layer      SlateCompute
%sources    1
%symbols    18
%annotated  16/18
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S TransmissionSequence.h | 258 lines | d01ca991 | 18 sym | `62` — cutout resolved at `16` and never here, transmissive collected into a bounded sorted column, and amended back to front.

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE THREE BEHAVIOURS
//------------------------------------------------------------------------------------------------------------------------

E TransmissionBehaviour                      | TransmissionSequence.h | 37-43   | contract                      | -  | Which of `62` §2's three rows an occupant's material takes. two is the defect that makes foliage cost what glass costs: a leaf card is opaque with a hole in it, and resolving it as transparent puts every leaf into the sorted column below, so a tree becomes the most expensive object in the workspace for no visual gain whatever. `74` and occluded by `60` with no special case in any of them. That is the whole reason the classification lives at `16` rather than here.
    has   Opaque          TransmissionBehaviour  [-]  ?
    has   Cutout          TransmissionBehaviour  [-]  ?
    has   Transmissive    TransmissionBehaviour  [-]  ?
    has   BehaviourCount  TransmissionBehaviour  [-]  ?
    by    Source/ConsoleHost.cpp, Source/TransmissionSequence.cpp
    note  🔴 Cutout is **not** transmission and is resolved at `16`, at visibility time — `62` §2. Conflating the
    note  🔴 Because a cutout occupant writes `VisibilityIndex`, it is shaded by `18`, outlined by `26`, picked by

F BehaviourOf                                | TransmissionSequence.h | 53      | api,nonallocating,nonthrowing | ✔️ | Which behaviour one declared material takes. answer to a question the reflectance selection and the cutout enrolment already answer, and the two disagree the moment an artist switches a material from transmissive to standard.
    in    Declared  const MaterialSpecification&  [-]  the material
    out   -         Behaviour                     [-]  derived from the declaration; never authored beside it
    by    Source/ConsoleHost.cpp, Source/TransmissionSequence.cpp
    note  🔴 Derived rather than declared as a fourth property. A behaviour declared separately is a second

F CoverageResolved                           | TransmissionSequence.h | 65      | api,nonallocating,nonthrowing | ✔️ | Whether one cutout coverage reading is present at a pixel. makes one artist's foliage disappear while another's grows a halo, and neither can correct it. the classification and `16` owns the resolution; the split is `62` §2's whole content.
    in    Declared  const MaterialSpecification&  [-]  the material, for its own threshold
    in    Coverage  double                        [-]  channel 8 as sampled
    out   -         Present                       [-]  true where the surface is there at all
    by    Source/ConsoleHost.cpp, Source/TransmissionSequence.cpp
    note  🔴 The threshold is **per material** and never global — `62` §2. A single threshold across a document
    note  ⚠️ Declared here and consumed by `16` §3.1, which is where the test is actually performed. `62` owns

//------------------------------------------------------------------------------------------------------------------------
//                                                      ONE FRAGMENT
//------------------------------------------------------------------------------------------------------------------------

T TransmissionFragment                       | TransmissionSequence.h | 73-78   | nonallocating,nonthrowing     | -  | One transmissive fragment, as `TransmissionIndex` carries it.
    has   DepthKey     std::uint32_t  [-]  ?
    has   SurfaceWord  std::uint32_t  [-]  ?
    has   Depth        double         [-]  ?
    by    Source/ConsoleHost.cpp, Source/TransmissionSequence.cpp

T TransmissionColumn                         | TransmissionSequence.h | 88-93   | nonallocating,nonthrowing     | -  | One pixel's bounded, depth-sorted transmissive column. amendment dominates, so discarding the farthest is the correct direction and is what `ProjectTransmissionSlot` already encodes. authored and cannot see, and a column that discards silently is one whose depth ceiling nobody can measure against their own scene.
    has   Held            TransmissionFragment[TransmissionDepth]  [-]  ?
    has   HeldCount       std::uint32_t                            [-]  ?
    has   TruncatedCount  std::uint32_t                            [-]  ?
    by    Source/ConsoleHost.cpp, Source/TransmissionSequence.cpp
    note  🔴 Nearest first, and the overflow leaves the **end** — `62` §3.1. The nearest surface is the one whose
    note  ⚠️ The truncation is **counted** and reported through `86`. A discarded fragment is content the artist

//------------------------------------------------------------------------------------------------------------------------
//                                                  WHAT ONE FRAGMENT IS
//------------------------------------------------------------------------------------------------------------------------

T TransmissionSpecification                  | TransmissionSequence.h | 103-110 | nonallocating,nonthrowing     | -  | What a transmissive occupant declares, read from `42` and added to by nothing. unamended, and the reflectance selection that consumes them is `18` §3's Transmissive.
    has   Opacity          double     [-]  ?
    has   Transmission     double     [-]  ?
    has   RefractionRatio  double     [-]  ?
    has   TintComponent    double[3]  [-]  ?
    has   Roughness        double     [-]  ?
    by    Source/TransmissionSequence.cpp
    note  🔴 `62` §4: this document **adds no channel**. Every field below is one of `18` §2's twenty, read

F DeclaredTransmission                       | TransmissionSequence.h | 120     | api,nonallocating,nonthrowing | ✔️ | Reads one fragment's transmission specification out of an already-resolved channel set. once at that position. Resampling here would walk `56`'s layer sequence a second time for the one surface in the scene most likely to be layered.
    in    Resolved  const ResolvedChannelSet&  [-]  `18`'s resolution at the fragment's own domain position
    out   -         Declared                   [-]  the five channels, unamended
    by    Source/TransmissionSequence.cpp
    note  📝 Taken from the resolved set rather than resampled, because `18` §8 already resolved the whole set

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE METRICS
//------------------------------------------------------------------------------------------------------------------------

T TransmissionMetrics                        | TransmissionSequence.h | 130-136 | nonallocating,nonthrowing     | -  | What `62` reports through `86`. appended once per rotation would bury the one truncation the artist did not expect.
    has   OccupantCount          std::uint32_t  [-]  ?
    has   GreatestColumnDepth    std::uint32_t  [-]  ?
    has   TruncatedThisRotation  std::uint32_t  [-]  ?
    has   TruncatedTotal         std::uint64_t  [-]  ?
    by    Source/TransmissionSequence.cpp
    note  🔴 The truncation **appends** and every count **overwrites** — `86` §2. A per-rotation occupant count

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE SEQUENCE
//------------------------------------------------------------------------------------------------------------------------

T TransmissionSequence                       | TransmissionSequence.h | 154-248 | owning                        | -  | `62` — the two recordings, the sorted collection, and the back-to-front amendment of `RadianceSurface`. transmissive occupant that wrote depth would occlude what is behind it in `16`, and the surface it exists to reveal would never be shaded at all. is deliberate: `60` §3's projections resolve topology, so a transmissive occupant enrolled in one would cast the shadow of a solid object, while a cutout occupant is topology with a coverage test and casts correctly with no special case at all. reads and writes and does not yet order two amenders of one target, so both declare an amendment ordinal and the schedule prefers the lower — the diff below closes that.
    has   CollectAmendmentOrdinal  static constexpr std::uint32_t  [-]  ?
    has   ResolveAmendmentOrdinal  static constexpr std::uint32_t  [-]  ?
    has   Reported                 TransmissionMetrics             [-]  ?
    by    Source/ConsoleHost.cpp, Source/TransmissionSequence.cpp
    note  🔴 `62` §6: **nothing here writes** `VisibilityIndex`, `DepthSurface` or `OccupancySurface`. A
    note  🔴 A transmissive occupant casts **no** occlusion and a cutout occupant does — `62` §5. The asymmetry
    note  ⚠️ 🚧 Ordering between this and `30` is `08` §2's amendment list. `RenderSchedule` orders by declared

F TransmissionSequence::ContributeCollection | TransmissionSequence.h | 170     | api,nonthrowing               | ✔️ | Contributes ⑤·i — the collection that writes `TransmissionIndex` and no depth. sorted insertion, so no depth write is performed and the opaque resolution `16` produced stands untouched.
    in    Schedule  RenderSchedule&  [-]  ?
    out   -         Deliver          [-]  refuses with whatever the schedule refused
    by    Source/TransmissionSequence.cpp
    note  🔴 Produces `TransmissionIndex` and amends nothing. Every fragment is inserted with an atomic

F TransmissionSequence::ContributeResolution | TransmissionSequence.h | 176     | api,nonthrowing               | ✔️ | Contributes ⑤·ii — the resolution that amends `RadianceSurface` back to front.
    in    Schedule  RenderSchedule&  [-]  ?
    out   -         Deliver          [-]  refuses with whatever the schedule refused
    by    Source/TransmissionSequence.cpp

F TransmissionSequence::Insert               | TransmissionSequence.h | 190     | api,nonallocating,nonthrowing | ✔️ | Inserts one fragment into one pixel's column, in depth order. and resolving it would amend a pixel it does not reach. preview through it and `00` §11 gates the agreement, which is why the comparison itself lives in `Shared/` rather than here.
    in    Column       TransmissionColumn&          [-]  the column, amended in place
    in    Arriving     const TransmissionFragment&  [-]  the fragment
    in    OpaqueDepth  double                       [-]  `16`'s resolved depth at this pixel, reversed
    out   -            Admitted                     [-]  false where the fragment was discarded
    by    Source/ConsoleHost.cpp, Source/TransmissionSequence.cpp
    note  🔴 A transmissive surface **behind the opaque depth is discarded** — `62` §3. It is not visible,
    note  📝 The host form of the device's atomic sorted insertion, and the same ordering. `82` §5 resolves a

F TransmissionSequence::AmendRadiance        | TransmissionSequence.h | 205     | api,nonallocating,nonthrowing | ✔️ | Amends one pixel's standing radiance by one fragment. direct term over `44` §5's reaching set, the same ambient term from `28`, the same models from `18` §3. Nothing about transmission changes how the surface itself is lit; it changes only what survives from behind it.
    in    Behind         const double                                           [-]  what stands behind the fragment, in the working space
    in    Declared       const TransmissionSpecification&                       [-]  the fragment's five channels
    in    Shaded         const double                                           [-]  what `18` resolved for the fragment itself, direct and ambient already summed
    in    ViewCosine     double                                                 [-]  ?
    in    Amended        double                                                 [-]  ?
    in    ViewCosine[-]  the fragment's orientation against the view direction  [-]  ?
    out   -              Amended                                                [-]  the three amended components
    by    Source/TransmissionSequence.cpp
    note  🔴 A transmissive surface is shaded through `18` **exactly as an opaque one is** — `62` §5. The same
    note  🔴 Refraction drives the Fresnel term and the lobe's width and **displaces nothing** — `62` §4.

F TransmissionSequence::Resolve              | TransmissionSequence.h | 222     | api,nonallocating,nonthrowing | 🚩 | Resolves one whole column back to front, amending the radiance behind it. nearest first and read in reverse. Reading it forward composites the near pane under the far one, which reads as two sheets of glass in the wrong order and is not visibly an ordering defect.
    in    Column         const TransmissionColumn&                      [-]  the column, nearest first
    in    Declared       const std::vector<TransmissionSpecification>&  [-]  one specification per held fragment, parallel to the column
    in    Shaded         const std::vector<std::array<double, 3>>&      [-]  one shaded radiance per held fragment, parallel to the column
    in    ViewCosine     const std::vector<double>&                     [-]  ?
    in    Standing       double                                         [-]  what `18` left in `RadianceSurface`, amended in place
    in    ViewCosine[-]  one per held fragment                          [-]  ?
    out   -              void                                           [-]  ?
    by    Api/AtmosphereIntegrator.h, Api/AttachmentIndex.h, Api/BrushSpecification.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/DocumentSession.h, (+94 more)
    note  🔴 Walked from the **last** held entry to the first, which is far to near — the column is stored

F TransmissionSequence::DeclareRotation      | TransmissionSequence.h | 231     | api,nonallocating,nonthrowing | ✔️ | Records what one rotation's collection cost, for `86`.
    in    OccupantCount          std::uint32_t  [-]  ?
    in    GreatestColumnDepth    std::uint32_t  [-]  ?
    in    TruncatedThisRotation  std::uint32_t  [-]  ?
    out   -                      void           [-]  ?
    by    Api/SampleIntegrator.h, Api/SpecularProjection.h, Source/ConsoleHost.cpp, Source/SampleIntegrator.cpp, Source/SpecularProjection.cpp, Source/TransmissionSequence.cpp

F TransmissionSequence::Report               | TransmissionSequence.h | 241     | api,nonthrowing               | 🚩 | Appends `62` §3.1's truncation and declares every measure beside it. subject rather than by the pixel, because a per-pixel subject would present a million entries for one pane of glass and `86` §6's coalescing would then be doing the discarding.
    in    Reporting  ReportSequence&  [-]  ?
    in    Measured   MeasureIndex&    [-]  ?
    in    Sampled    TickPoint        [-]  ?
    out   -          void             [-]  ?
    by    Api/AssetInterchange.h, Api/BrushSpecification.h, Api/ChartPartition.h, Api/CodeInterchange.h, Api/DisplayProjection.h, Api/HardwareMetrics.h, (+32 more)
    note  🔴 The truncation appends and the counts overwrite — `86` §2. Coalesced by the ceiling as its

F TransmissionSequence::Metrics              | TransmissionSequence.h | 243     | -                             | -  | ?
    out   -  const TransmissionMetrics&  [-]  ?
    by    Api/ChartPartition.h, Api/OcclusionProjection.h, Api/PartitionStructure.h, Api/SampleIntegrator.h, Api/SpecularProjection.h, Source/ChartPartition.cpp, (+6 more)

F SLATE_DECLARES_PRECISION                   | TransmissionSequence.h | 253     | -                             | -  | ?
    in    Perceptual  PrecisionGuarantee::  [-]  ?
    in    Perceptual  PrecisionGuarantee::  [-]  ?
    in    Bounded     PrecisionGuarantee::  [-]  ?
    in    Exact       PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)
