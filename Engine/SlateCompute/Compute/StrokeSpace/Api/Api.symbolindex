//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The bounded extent one stroke accumulates into — coverage per texel, per touched cell, applied once at Seal.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/StrokeSpace/Api
%layer      SlateCompute
%sources    1
%symbols    12
%annotated  8/12
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S StrokeSpace.h | 122 lines | 8b8fcb76 | 12 sym | The bounded extent one stroke accumulates into — coverage per texel, per touched cell, applied once at Seal.

//------------------------------------------------------------------------------------------------------------------------
//                                                   ONE COVERAGE TILE
//------------------------------------------------------------------------------------------------------------------------

V CoverageTileTexels             | StrokeSpace.h | 26     | -                             | -  | ?
    by    Source/ConsoleHost.cpp, Source/ImpressionSequence.cpp, Source/StrokeSpace.cpp

V CoverageTileCeiling            | StrokeSpace.h | 34     | -                             | -  | ?
    by    Source/StrokeSpace.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE ACCUMULATION
//------------------------------------------------------------------------------------------------------------------------

T StrokeSpace                    | StrokeSpace.h | 53-120 | owning                        | -  | The coverage one stroke has accumulated, held sparsely by cell and reclaimed whole. Applying per impression lets overlapping impressions within one stroke double-darken at their intersections, which is visible wherever an artist slows down — and slowing down is what an artist does at exactly the places they care about. `a + b(1 − a)`, which is symmetric in its two operands — so the accumulated coverage does not depend on the order the impressions were resolved in. That is what lets `22` §2's deferred impression resolve two rotations later without disturbing the ones around it. the values are constants of the stroke and only the coverage varies per texel. Holding values here would be holding one number per channel per texel to store the same number everywhere.
    has   TileOfCell     std::vector<std::uint32_t>       [-]  ?
    has   ClaimedCells   std::vector<std::uint32_t>       [-]  ?
    has   Claimed        std::vector<std::vector<float>>  [-]  ?
    has   TouchedTexels  std::uint64_t                    [-]  ?
    by    Api/ImpressionSequence.h, Api/PreviewProjection.h, Source/ImpressionSequence.cpp, Source/PreviewProjection.cpp, Source/StrokeSpace.cpp
    note  🔴 `22` §3: a stroke resolves into an accumulation extent **first** and applies once to the surface.
    note  📐 Accumulation is `Over`, whatever combination the brush declares. `CombineCoverage(Over, a, b)` is
    note  🔴 This holds coverage and **no channel value**. `58` §2 declares one shape and a value per channel, so

F StrokeSpace::Construct         | StrokeSpace.h | 61     | api,nonthrowing               | 🚩 | Sizes the sparse index; claims nothing.
    out   -  void  [-]  ?
    post  every cell is unclaimed and the touched count is zero
    by    Api/AnalyticProjection.h, Api/AtmosphereIntegrator.h, Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CameraProjection.h, Api/CommandSequence.h, (+62 more)

F StrokeSpace::Claim             | StrokeSpace.h | 72     | api,nonthrowing               | 🚩 | Claims the coverage tile backing one cell, or resolves the one already claimed. the declared tile ceiling count. It is a guard against a coarser level being painted at with a miscomputed ordinal, and it refuses rather than growing so that a defect there is a refusal instead of an allocation storm.
    in    CellOrdinal  std::uint32_t  [-]  into `20` §1's single ordinal span
    out   -            Outcome        [-]  refuses with ContentUnsupported outside the span, and with ExtentExhausted at
    by    Api/ByteSpace.h, Api/DescriptorIndex.h, Api/ImageSpace.h, Api/RenderSchedule.h, Api/SpanSpace.h, Api/TileSpace.h, (+14 more)
    note  📝 Exhaustion is structurally unreachable at the finest level, where the ceiling equals the cell

F StrokeSpace::Located           | StrokeSpace.h | 78     | api,nonthrowing               | ✔️ | The tile backing one cell, if one is claimed.
    in    CellOrdinal  std::uint32_t  [-]  ?
    out   -            Outcome        [-]  refuses with ExtentExhausted when the cell is untouched
    by    Api/DocumentSession.h, Api/IlluminantPopulation.h, Api/OcclusionProjection.h, Api/PointerIntersection.h, Api/PropertySpecification.h, Api/ReportSequence.h, (+26 more)

F StrokeSpace::Accumulate        | StrokeSpace.h | 89     | api,nonallocating,nonthrowing | ✔️ | Accumulates one impression's coverage at one texel of one claimed tile. impressions overlap, and the excess is invisible in the accumulation and abrupt at the apply.
    in    TileOrdinal  std::uint32_t  [-]   as `Claim` delivered it
    in    Along        std::uint32_t  [px]  within the tile
    in    Across       std::uint32_t  [px]  within the tile
    in    Arriving     double         [-]   the impression's coverage there, in the closed unit interval
    out   -            void           [-]   ?
    by    Api/SampleIntegrator.h, Source/ImpressionSequence.cpp, Source/SampleIntegrator.cpp, Source/StrokeSpace.cpp
    note  🔴 Combined by `Over` and never by addition. Additive accumulation exceeds unity wherever two

F StrokeSpace::Coverage          | StrokeSpace.h | 94     | api,nonallocating,nonthrowing | ✔️ | The coverage standing at one texel.
    in    TileOrdinal  std::uint32_t  [-]  ?
    in    Along        std::uint32_t  [-]  ?
    in    Across       std::uint32_t  [-]  ?
    out   -            double         [-]  ?
    by    Api/AnalyticProjection.h, Api/SurfaceLayerSequence.h, Api/ThemeSpecification.h, Api/TransmissionSequence.h, Source/AnalyticProjection.cpp, Source/ChannelPanel.cpp, (+8 more)

F StrokeSpace::TouchedCells      | StrokeSpace.h | 99     | api,nonallocating,nonthrowing | ✔️ | The cells this stroke has touched, in claim order.
    out   -  const std::vector<std::uint32_t>&  [-]  ?
    by    Api/ImpressionSequence.h, Source/ImpressionSequence.cpp, Source/StrokeSpace.cpp

F StrokeSpace::ClaimedCount      | StrokeSpace.h | 101    | -                             | -  | ?
    out   -  std::uint32_t  [-]  ?
    by    Api/DescriptorIndex.h, Api/ImageSpace.h, Api/SpanSpace.h, Api/TileSpace.h, Source/ConsoleHost.cpp, Source/DescriptorIndex.cpp, (+5 more)

F StrokeSpace::TouchedTexelCount | StrokeSpace.h | 102    | -                             | -  | ?
    out   -  std::uint64_t  [-]  ?
    by    Source/StrokeSpace.cpp

F StrokeSpace::Reclaim           | StrokeSpace.h | 108    | api,nonthrowing               | 🚩 | Discards every claimed tile, keeping the sparse index sized.
    out   -  void  [-]  ?
    by    Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CodeInterchange.h, Api/CommandSequence.h, Api/CycleScheduler.h, Api/DepthReduction.h, (+75 more)
    note  Called at Seal, at Abandon, and once per rotation by a speculative extent — `22` §4.1.
