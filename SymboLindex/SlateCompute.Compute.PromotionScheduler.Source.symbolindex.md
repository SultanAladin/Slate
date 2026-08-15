//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The two measures charged apart, the total eviction order, and the cost read from `56`'s entries.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/PromotionScheduler/Source
%layer      SlateCompute
%sources    1
%symbols    17
%annotated  0/17
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S PromotionScheduler.cpp | 173 lines | 7fdc8427 | 17 sym | The two measures charged apart, the total eviction order, and the cost read from `56`'s entries.

//------------------------------------------------------------------------------------------------------------------------
//                                                   EVICTION ORDERING
//------------------------------------------------------------------------------------------------------------------------

F PrecedesInEviction                  | PromotionScheduler.cpp | 15-37   | - | - | ?
    in    Declared  EvictionOrdering          [-]  ?
    in    Earlier   const EvictionCandidate&  [-]  ?
    in    Later     const EvictionCandidate&  [-]  ?
    out   -         bool                      [-]  ?
    by    Api/PromotionScheduler.h, Source/SurfaceTileSpace.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    COST ESTIMATION
//------------------------------------------------------------------------------------------------------------------------

F Estimate                            | PromotionScheduler.cpp | 43-81   | - | - | ?
    in    Sequence            const SurfaceLayerSequence&  [-]  ?
    in    TileBytes           std::uint64_t                [-]  ?
    in    DepotArtefactValid  bool                         [-]  ?
    out   -                   PromotionCost                [-]  ?
    by    Api/PromotionScheduler.h

//------------------------------------------------------------------------------------------------------------------------
//                                                      DECLARATION
//------------------------------------------------------------------------------------------------------------------------

F PromotionScheduler::DeclareBudget   | PromotionScheduler.cpp | 87-99   | - | - | ?
    in    Declaring  const PromotionBudget&  [-]  ?
    out   -          Deliver<bool>           [-]  ?

F PromotionScheduler::DeclareOrdering | PromotionScheduler.cpp | 101-105 | - | - | ?
    in    Declaring  EvictionOrdering  [-]  ?
    out   -          void              [-]  ?

F PromotionScheduler::OpenRotation    | PromotionScheduler.cpp | 107-122 | - | - | ?
    in    RotationOrdinal  std::uint64_t  [-]  ?
    out   -                Deliver<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE BUDGET
//------------------------------------------------------------------------------------------------------------------------

F PromotionScheduler::Admits          | PromotionScheduler.cpp | 128-135 | - | - | ?
    in    Costing  const PromotionCost&  [-]  ?
    out   -        bool                  [-]  ?

F PromotionScheduler::Charge          | PromotionScheduler.cpp | 137-146 | - | - | ?
    in    Costing  const PromotionCost&  [-]  ?
    out   -        Deliver<bool>         [-]  ?

F PromotionScheduler::DeferOne        | PromotionScheduler.cpp | 148-152 | - | - | ?
    out   -  void  [-]  ?

F PromotionScheduler::PromoteOne      | PromotionScheduler.cpp | 154-158 | - | - | ?
    out   -  void  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

F PromotionScheduler::Ordering        | PromotionScheduler.cpp | 164     | - | - | ?
    out   -  EvictionOrdering  [-]  ?

F PromotionScheduler::Declared        | PromotionScheduler.cpp | 165     | - | - | ?
    out   -  const PromotionBudget&  [-]  ?

F PromotionScheduler::Remaining       | PromotionScheduler.cpp | 166     | - | - | ?
    out   -  const PromotionBudget&  [-]  ?

F PromotionScheduler::OpenedRotation  | PromotionScheduler.cpp | 167     | - | - | ?
    out   -  std::uint64_t  [-]  ?

F PromotionScheduler::PromotedCount   | PromotionScheduler.cpp | 168     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F PromotionScheduler::DeferredCount   | PromotionScheduler.cpp | 169     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F PromotionScheduler::PromotedTotal   | PromotionScheduler.cpp | 170     | - | - | ?
    out   -  std::uint64_t  [-]  ?

F PromotionScheduler::DeferredTotal   | PromotionScheduler.cpp | 171     | - | - | ?
    out   -  std::uint64_t  [-]  ?
