//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 `30` — screen-space reflection over depth already resolved and radiance already shaded, composed so that nothing is counted twice.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/SpecularProjection/Api
%layer      SlateCompute
%sources    1
%symbols    15
%annotated  11/15
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S SpecularProjection.h | 281 lines | cc404995 | 15 sym | `30` — screen-space reflection over depth already resolved and radiance already shaded, composed so that nothing is counted twice.

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT IS DECLARED
//------------------------------------------------------------------------------------------------------------------------

T ReflectionSpecification             | SpecularProjection.h | 29-37   | nonallocating,nonthrowing     | -  | What the trace is bounded by. here rather than in `Contract/` because no second unit reads one — `00` §2's rule, applied to a number that is a measurement waiting to happen rather than an agreement.
    has   MarchCeiling      std::uint32_t  [-]  ?
    has   RefineCount       std::uint32_t  [-]  ?
    has   ThicknessBound    double         [-]  ?
    has   RoughnessCeiling  double         [-]  ?
    has   ExtentDivisor     std::uint32_t  [-]  ?
    has   JitterDeclared    bool           [-]  ?
    by    Source/ConsoleHost.cpp, Source/SpecularProjection.cpp
    note  🚧 Every figure below is one of `30` §7's open rows and each blocks tuning alone. They are declared

T TracedReflection                    | SpecularProjection.h | 44-50   | nonallocating,nonthrowing     | -  | One traced result and the weight it carries into the composite. pointing away from the camera. `30` §3's table is four rows and one result, which is what lets the march terminate anywhere it likes.
    has   Component   double[3]      [-]  ?
    has   Weight      double         [-]  ?
    has   StepsTaken  std::uint32_t  [-]  ?
    has   Resolved    bool           [-]  ?
    by    Source/ConsoleHost.cpp, Source/SpecularProjection.cpp
    note  🔴 `Weight` is nothing on **every** failure — off the extent, past the ceiling, behind a surface, or

T ReflectionMetrics                   | SpecularProjection.h | 56-62   | nonallocating,nonthrowing     | -  | What `30` reports through `86`. as `30` §3 designed it, and reporting one would mean the register is never quiet.
    has   TracedCount    std::uint32_t  [-]  ?
    has   ResolvedCount  std::uint32_t  [-]  ?
    has   SkippedCount   std::uint32_t  [-]  ?
    has   StepsTaken     std::uint64_t  [-]  ?
    by    Source/SpecularProjection.cpp
    note  🔴 Every row overwrites. `86` §5 rules a failed trace ordinary operation — it is the mechanism working

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PROJECTION
//------------------------------------------------------------------------------------------------------------------------

T SpecularProjection                  | SpecularProjection.h | 79-179  | owning                        | -  | `30` — the amending recording, the march, and the composite that subtracts before it adds. `62` and `30` amend it in that order, and `08` §2's amendment list is what makes that legal and ordered. `08` §6 previously gated "no shared target is produced by two recordings" while this section wrote back into a target `18` produced — recorded as `00` §10 conflict 26 and closed by the ordinal. a transmissive occupant shows that occupant. It reads nothing display-referred, so a selection outline appearing inside a mirror is impossible by ordering rather than by a test. needs was already resolved by `16` and shaded by `18`.
    has   AmendmentOrdinal  static constexpr std::uint32_t  [-]  ?
    has   Specification     ReflectionSpecification         [-]  ?
    has   Reported          ReflectionMetrics               [-]  ?
    by    Source/ConsoleHost.cpp, Source/SpecularProjection.cpp
    note  🔴 `30` §5: this is an **amending** recording and not a producing one. `18` produces `RadianceSurface`;
    note  🔴 `30` §5: the trace reads `RadianceSurface` at the hit, so it reads `62`'s amendment — a reflection of
    note  🔴 No geometry is submitted and no second visibility resolution occurs — `30` §6. Everything the trace

F SpecularProjection::Declare         | SpecularProjection.h | 95      | api,nonthrowing               | ✔️ | Declares what the trace is bounded by. thickness, and a divisor that is not two `60`'s ambient term refuses one: `08` §2 claims `ReflectionSurface` at half extent, and admitting a third would declare the extent in two places that could disagree.
    in    Declaring  const ReflectionSpecification&  [-]  ?
    out   -          Deliver                         [-]  refuses with ContentUnsupported for a march ceiling of nothing, a non-positive
    by    Api/AttachmentIndex.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/DiagnosticExtension.h, (+65 more)
    note  ⚠️ The divisor is refused above two rather than admitted as a quality setting, for the reason

F SpecularProjection::Contribute      | SpecularProjection.h | 104     | api,nonthrowing               | ✔️ | Contributes `08` §3 ⑥'s recording. pre-added contribution and the resolved weight, which is what makes the composite expressible at all — a target carrying the trace result instead would leave nothing to subtract.
    in    Schedule  RenderSchedule&  [-]  ?
    out   -         Deliver          [-]  refuses with whatever the schedule refused
    by    Api/DisplayProjection.h, Api/IntersectionOutline.h, Api/OcclusionProjection.h, Api/OverlayProjection.h, Api/ReflectanceIntegrator.h, Api/RenderSchedule.h, (+13 more)
    note  📝 Produces `ReflectionSurface` and amends `RadianceSurface`. The produced target carries `18`'s

F SpecularProjection::Resolve         | SpecularProjection.h | 112     | api,nonthrowing               | ✔️ | The extent the trace is resolved at, from one display extent. Rounding down leaves the display's last column with no coarse texel above it.
    in    DisplayAlong    std::uint32_t   [-]  ?
    in    DisplayAcross   std::uint32_t   [-]  ?
    in    ResolvedAlong   std::uint32_t&  [-]  ?
    in    ResolvedAcross  std::uint32_t&  [-]  ?
    out   -               Deliver         [-]  refuses with ContentUnsupported for a display extent of nothing
    by    Api/AtmosphereIntegrator.h, Api/AttachmentIndex.h, Api/BrushSpecification.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/DocumentSession.h, (+94 more)
    note  📐 Rounded **up** on both ordinates, matching `RenderSchedule`'s own fraction-of-display claim.

F SpecularProjection::March           | SpecularProjection.h | 136     | api,nonthrowing               | 🔴 | What the caller must sample and where — one step of the march, in display coordinates. low-frequency at all but mirror roughness, and the extent is where the cost lives. the interval six times resolves it to within a sixty-fourth of one step and marching sixty-four times as far costs sixty-four times as much for the same answer. which samples a resident target — and `82`'s host preview, which samples a resolved one. `00` §11 gates the agreement between the two at Tier B, and one routine is the only way it holds.
    in    OriginAlong   double          [-]  the shading pixel, in the closed unit square
    in    OriginAcross  double          [-]  its second ordinate, likewise
    in    OriginDepth   double          [-]  its reversed depth
    in    StepAlong     double          [-]  the reflected direction projected into display coordinates, per step
    in    StepAcross    double          [-]  its second ordinate, likewise
    in    StepDepth     double          [-]  the reversed-depth change per step
    in    Sampling      const Sampler&  [-]  answers with `DepthSurface` and `RadianceSurface` at a coordinate
    out   -             Traced          [-]  the crossing, or a weight of nothing
    note  🔴 The march is against `DepthSurface` at **half extent** — `30` §2 ③. Reflections are
    note  📝 The crossing is refined by a short binary search rather than by a finer march, because halving
    note  🔴 The sampler is supplied rather than held, so that the same routine serves the device dispatch —

F SpecularProjection::Compose         | SpecularProjection.h | 155     | api,nonallocating,nonthrowing | ✔️ | Applies `30` §1's composite at one pixel. arithmetic in this document that cannot be got slightly wrong without the error being invisible — a double count brightens uniformly and reads as the material being wrong.
    in    Standing  const double             [-]  `RadianceSurface` as `18` and `62` left it
    in    PreAdded  const double             [-]  `ReflectionSurface` RGB — what `18`'s ambient term already contributed
    in    Traced    const TracedReflection&  [-]  the trace's own result and weight
    in    Resolved  double                   [-]  ?
    out   -         Resolved                 [-]  the amended radiance
    by    Source/ConsoleHost.cpp, Source/SpecularProjection.cpp
    note  🔴 Through `Shared/`'s own routine and never written again here. The composite is the one piece of

F SpecularProjection::DeclareRotation | SpecularProjection.h | 163     | api,nonallocating,nonthrowing | ✔️ | Records what one rotation's tracing cost, for `86`.
    in    TracedCount    std::uint32_t  [-]  ?
    in    ResolvedCount  std::uint32_t  [-]  ?
    in    SkippedCount   std::uint32_t  [-]  ?
    out   -              void           [-]  ?
    by    Api/SampleIntegrator.h, Api/TransmissionSequence.h, Source/ConsoleHost.cpp, Source/SampleIntegrator.cpp, Source/SpecularProjection.cpp, Source/TransmissionSequence.cpp

F SpecularProjection::Report          | SpecularProjection.h | 170     | api,nonthrowing               | 🚩 | Declares every measure; appends nothing. `30` §3's design operating and not a fact the artist needs told.
    in    Measured  MeasureIndex&  [-]  ?
    in    Sampled   TickPoint      [-]  ?
    out   -         void           [-]  ?
    by    Api/AssetInterchange.h, Api/BrushSpecification.h, Api/ChartPartition.h, Api/CodeInterchange.h, Api/DisplayProjection.h, Api/HardwareMetrics.h, (+32 more)
    note  🔴 `30` appears in **no** row of `86` §4's register, and this reports accordingly. A failed trace is

F SpecularProjection::Declared        | SpecularProjection.h | 172     | -                             | -  | ?
    out   -  const ReflectionSpecification&  [-]  ?
    by    Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/DecalProjection.h, Api/DescriptorIndex.h, (+82 more)

F SpecularProjection::Metrics         | SpecularProjection.h | 173     | -                             | -  | ?
    out   -  const ReflectionMetrics&  [-]  ?
    by    Api/ChartPartition.h, Api/OcclusionProjection.h, Api/PartitionStructure.h, Api/SampleIntegrator.h, Api/TransmissionSequence.h, Source/ChartPartition.cpp, (+6 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE MARCH
//------------------------------------------------------------------------------------------------------------------------

F SpecularProjection::March           | SpecularProjection.h | 185-272 | -                             | -  | ?
    in    OriginAlong   double            [-]  ?
    in    OriginAcross  double            [-]  ?
    in    OriginDepth   double            [-]  ?
    in    StepAlong     double            [-]  ?
    in    StepAcross    double            [-]  ?
    in    StepDepth     double            [-]  ?
    in    Sampling      const Sampler&    [-]  ?
    out   -             TracedReflection  [-]  ?

F SLATE_DECLARES_PRECISION            | SpecularProjection.h | 276     | -                             | -  | ?
    in    Perceptual  PrecisionGuarantee::  [-]  ?
    in    Perceptual  PrecisionGuarantee::  [-]  ?
    in    Bounded     PrecisionGuarantee::  [-]  ?
    in    Exact       PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)
