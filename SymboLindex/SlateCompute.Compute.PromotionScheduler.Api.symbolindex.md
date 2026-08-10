//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Two independent budgets, spent per rotation, and the ordering eviction follows when they cannot be met.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/PromotionScheduler/Api
%layer      SlateCompute
%sources    1
%symbols    24
%annotated  15/24
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S PromotionScheduler.h | 208 lines | 62ea16c8 | 24 sym | Two independent budgets, spent per rotation, and the ordering eviction follows when they cannot be met.

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE TWO MEASURES
//------------------------------------------------------------------------------------------------------------------------

T PromotionBudget                     | PromotionScheduler.h | 28-32   | nonallocating,nonthrowing     | -  | What one rotation may spend promoting tiles. alone does not bound analytic resolution, because an analytically resolved tile transfers nothing — it is produced on the device from a source description. A surface carrying many analytic layers can therefore consume unbounded device time while the transfer measure truthfully reports zero. Exceeding either measure defers, and deferral is normal operation rather than an error — `86` §5.
    has   TransferBytes    std::uint64_t  [-]  ?
    has   EvaluationUnits  std::uint64_t  [-]  ?
    by    Source/PromotionScheduler.cpp
    note  🔴 `20` §2.2: promotion is bounded by **two independent measures**, not by tile count. Transfer volume
    note  ⚠️ An unbounded promotion burst on a camera cut produces a stall exactly where it is most visible.

T PromotionCost                       | PromotionScheduler.h | 39-43   | nonallocating,nonthrowing     | -  | What promoting one tile would cost, against both measures at once. transfer for the first and evaluation for the second, and both must fit — which is why the two are carried together rather than resolved to whichever dominates.
    has   TransferBytes    std::uint64_t  [-]  ?
    has   EvaluationUnits  std::uint64_t  [-]  ?
    by    Api/SurfaceTileSpace.h, Source/PromotionScheduler.cpp, Source/SurfaceTileSpace.cpp
    note  📝 A tile is rarely one or the other. A surface with painted layers beneath a placed decal charges

V EvaluationUnitsPerEntry             | PromotionScheduler.h | 49      | -                             | -  | ?
    by    Source/AnalyticProjection.cpp, Source/PromotionScheduler.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                  WHAT A PROMOTION DID
//------------------------------------------------------------------------------------------------------------------------

E PromotionDisposition                | PromotionScheduler.h | 57-64   | contract                      | -  | How one considered promotion ended.
    has   Promoted          PromotionDisposition  [-]  ?
    has   ReResolved        PromotionDisposition  [-]  ?
    has   AlreadyResident   PromotionDisposition  [-]  ?
    has   Deferred          PromotionDisposition  [-]  ?
    has   DispositionCount  PromotionDisposition  [-]  ?
    by    Api/SurfaceTileSpace.h, Source/SurfaceTileSpace.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                   EVICTION ORDERING
//------------------------------------------------------------------------------------------------------------------------

T EvictionCandidate                   | PromotionScheduler.h | 72-78   | nonallocating,nonthrowing     | -  | One resident cell offered as an eviction candidate.
    has   CellOrdinal  std::uint32_t  [-]  ?
    has   Level        std::uint32_t  [-]  ?
    has   DemandedAt   std::uint64_t  [-]  ?
    has   PromotedAt   std::uint64_t  [-]  ?
    by    Source/PromotionScheduler.cpp, Source/SurfaceTileSpace.cpp

E EvictionOrdering                    | PromotionScheduler.h | 86-91   | contract                      | -  | Which ordering eviction follows. edge to `46` that `20` does not declare, and `00` §11 gates that a declared edge is a real read, so acquiring one to answer a tuning question would be paying a build-order cost for a constant. Least recently demanded ships; the row stays open.
    has   LeastRecentlyDemanded  EvictionOrdering  [-]  ?
    has   FinestLevelFirst       EvictionOrdering  [-]  ?
    has   OrderingCount          EvictionOrdering  [-]  ?
    by    Source/PromotionScheduler.cpp
    note  🚧 `20` §6 carries this as open — least-recent, or distance from the camera. The second would need an

F PrecedesInEviction                  | PromotionScheduler.h | 99      | api,nonallocating,nonthrowing | ✔️ | Whether the first candidate is evicted before the second. ordering that left two candidates incomparable would evict whichever the walk reached, and the walk order is a storage detail the artist would then be able to feel.
    in    Declared  EvictionOrdering          [-]  ?
    in    Earlier   const EvictionCandidate&  [-]  ?
    in    Later     const EvictionCandidate&  [-]  ?
    out   -         bool                      [-]  ?
    by    Source/PromotionScheduler.cpp, Source/SurfaceTileSpace.cpp
    note  📐 A total order over the candidates, so the choice does not depend on which was offered first. An

//------------------------------------------------------------------------------------------------------------------------
//                                                    COST ESTIMATION
//------------------------------------------------------------------------------------------------------------------------

F Estimate                            | PromotionScheduler.h | 121     | api,nonthrowing               | 🚩 | What promoting one tile of a surface would cost, from what its layer sequence holds. source. That is the whole reason the depot pays for itself: it converts an evaluation cost that the second budget bounds tightly into a transfer cost that the first bounds loosely. test a cell against. It is therefore conservative: an entry covering a tenth of the surface is charged against every tile. Sharpening it needs a coverage extent per entry, which is `56` §10's third open row read from this side.
    in    Sequence            const SurfaceLayerSequence&  [-]  the surface's content — `56`
    in    TileBytes           std::uint64_t                [B]  what one stored tile occupies, apron included
    in    DepotArtefactValid  bool                         [-]  a depot artefact stands for this tile
    out   -                   Costing                      [-]  both measures
    by    Source/PromotionScheduler.cpp
    note  🔴 A valid depot artefact short-circuits to **pure transfer** — `20` §2.1's second reconstruction
    note  🚧 Estimated at the sequence level rather than per cell, because `56` carries no per-entry extent to

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SCHEDULER
//------------------------------------------------------------------------------------------------------------------------

T PromotionScheduler                  | PromotionScheduler.h | 135-206 | owning                        | -  | One rotation's promotion budget, spent in arrival order, and the ordering eviction follows. reconstructed and never touches one. `20` §2.1 ⑤'s three reconstruction sources belong to `56`, the depot and `70`, and a scheduler that reached for them would be a scheduler with an opinion about content.
    has   DeclaredBudget    PromotionBudget   [-]  ?
    has   RemainingBudget   PromotionBudget   [-]  ?
    has   DeclaredOrder     EvictionOrdering  [-]  ?
    has   RotationOpened    std::uint64_t     [-]  ?
    has   PromotedThis      std::uint32_t     [-]  ?
    has   DeferredThis      std::uint32_t     [-]  ?
    has   PromotedSession   std::uint64_t     [-]  ?
    has   DeferredSession   std::uint64_t     [-]  ?
    has   RotationStanding  bool              [-]  ?
    by    Api/SurfaceTileSpace.h, Source/PromotionScheduler.cpp, Source/SurfaceTileSpace.cpp
    note  🔴 The scheduler decides **whether** and **what to evict**; it does not decide how a tile is

F PromotionScheduler::DeclareBudget   | PromotionScheduler.h | 145     | api,nonthrowing               | ✔️ | Declares what one rotation may spend. content genuinely has no evaluation to bound, and refusing it would force a fictitious number.
    in    Declaring  const PromotionBudget&  [-]  ?
    out   -          Outcome                 [-]  refuses with ContentUnsupported when both measures are zero
    by    Source/PromotionScheduler.cpp
    note  A budget of zero on one measure alone is admitted deliberately: a document with no analytic

F PromotionScheduler::DeclareOrdering | PromotionScheduler.h | 150     | api,nonallocating,nonthrowing | ✔️ | Declares which ordering eviction follows.
    in    Declaring  EvictionOrdering  [-]  ?
    out   -          void              [-]  ?
    by    Source/PromotionScheduler.cpp

F PromotionScheduler::OpenRotation    | PromotionScheduler.h | 160     | api,nonthrowing               | ✔️ | Opens one rotation, restoring the whole budget. its unspent remainder forward would let a still workspace bank several rotations of promotion and spend them all on the rotation the artist finally moved — which is precisely the stall the budget exists to prevent.
    in    RotationOrdinal  std::uint64_t  [-]  ?
    out   -                Outcome        [-]  refuses with HostDenied when the rotation is not later than the one last opened
    by    Source/PromotionScheduler.cpp
    note  🔴 The budget is per rotation and is restored here rather than accumulated. A budget that carried

F PromotionScheduler::Admits          | PromotionScheduler.h | 165     | api,nonallocating,nonthrowing | ✔️ | Whether a cost fits what remains of this rotation.
    in    Costing  const PromotionCost&  [-]  ?
    out   -        bool                  [-]  ?
    by    Source/PromotionScheduler.cpp, Source/SurfaceTileSpace.cpp

F PromotionScheduler::Charge          | PromotionScheduler.h | 172     | api,nonthrowing               | ✔️ | Charges a cost against what remains.
    in    Costing  const PromotionCost&  [-]  ?
    out   -        Outcome               [-]  refuses with ExtentExhausted when it does not fit
    post  a refused charge spends nothing
    by    Source/PromotionScheduler.cpp, Source/SurfaceTileSpace.cpp

F PromotionScheduler::DeferOne        | PromotionScheduler.h | 179     | api,nonallocating,nonthrowing | ✔️ | Records that one promotion was deferred — `86` §5's measure row for `20` §2.2. register that appended one per deferred tile per rotation is a register nobody reads.
    out   -  void  [-]  ?
    by    Source/PromotionScheduler.cpp, Source/SurfaceTileSpace.cpp
    note  🔴 A measure and **not** a report. `86` §5: deferral against budget is ordinary operation, and a

F PromotionScheduler::PromoteOne      | PromotionScheduler.h | 184     | api,nonallocating,nonthrowing | ✔️ | Records that one promotion was admitted.
    out   -  void  [-]  ?
    by    Source/PromotionScheduler.cpp, Source/SurfaceTileSpace.cpp

F PromotionScheduler::Ordering        | PromotionScheduler.h | 186     | -                             | -  | ?
    out   -  EvictionOrdering  [-]  ?
    by    Api/CommandSequence.h, Api/DomainSpace.h, Source/CommandSequence.cpp, Source/DomainSpace.cpp, Source/PromotionScheduler.cpp, Source/SurfaceTileSpace.cpp

F PromotionScheduler::Declared        | PromotionScheduler.h | 187     | -                             | -  | ?
    out   -  const PromotionBudget&  [-]  ?
    by    Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/DecalProjection.h, Api/DescriptorIndex.h, (+45 more)

F PromotionScheduler::Remaining       | PromotionScheduler.h | 188     | -                             | -  | ?
    out   -  const PromotionBudget&  [-]  ?
    by    Shared/SampleProjection.slang.h, Source/AssetInterchange.cpp, Source/CurveSolver.cpp, Source/PromotionScheduler.cpp, Source/RowSequence.cpp, Source/SurfaceTileSpace.cpp

F PromotionScheduler::OpenedRotation  | PromotionScheduler.h | 189     | -                             | -  | ?
    out   -  std::uint64_t  [-]  ?
    by    Source/PromotionScheduler.cpp

F PromotionScheduler::PromotedCount   | PromotionScheduler.h | 190     | -                             | -  | ?
    out   -  std::uint32_t  [-]  ?
    by    Source/PromotionScheduler.cpp, Source/SurfaceTileSpace.cpp

F PromotionScheduler::DeferredCount   | PromotionScheduler.h | 191     | -                             | -  | ?
    out   -  std::uint32_t  [-]  ?
    by    Api/ImpressionSequence.h, Source/ConsoleHost.cpp, Source/ImpressionSequence.cpp, Source/PromotionScheduler.cpp, Source/SurfaceTileSpace.cpp

F PromotionScheduler::PromotedTotal   | PromotionScheduler.h | 192     | -                             | -  | ?
    out   -  std::uint64_t  [-]  ?
    by    Source/PromotionScheduler.cpp

F PromotionScheduler::DeferredTotal   | PromotionScheduler.h | 193     | -                             | -  | ?
    out   -  std::uint64_t  [-]  ?
    by    Source/PromotionScheduler.cpp
