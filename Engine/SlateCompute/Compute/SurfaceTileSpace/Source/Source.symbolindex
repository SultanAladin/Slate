//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The coarsening walk that never stalls, the revision comparison, and promotion that evicts only what it may.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/SurfaceTileSpace/Source
%layer      SlateCompute
%sources    1
%symbols    28
%annotated  0/28
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S SurfaceTileSpace.cpp | 623 lines | c4055d81 | 28 sym | The coarsening walk that never stalls, the revision comparison, and promotion that evicts only what it may.

//------------------------------------------------------------------------------------------------------------------------
//                                                    CELL ADDRESSING
//------------------------------------------------------------------------------------------------------------------------

F OrdinalOf                             | SurfaceTileSpace.cpp | 15-27   | - | - | ?
    in    Addressed  CellAddress             [-]  ?
    out   -          Outcome<std::uint32_t>  [-]  ?
    by    Api/RenderSchedule.h, Api/SurfaceTileSpace.h, Source/ImpressionSequence.cpp, Source/OcclusionScheduler.cpp, Source/RenderSchedule.cpp

F AddressOf                             | SurfaceTileSpace.cpp | 29-51   | - | - | ?
    in    CellOrdinal  std::uint32_t         [-]  ?
    out   -            Outcome<CellAddress>  [-]  ?
    by    Api/SurfaceTileSpace.h, Source/ImpressionSequence.cpp

F OrdinalAt                             | SurfaceTileSpace.cpp | 53-76   | - | - | ?
    in    Level           std::uint32_t           [-]  ?
    in    PositionAlong   double                  [-]  ?
    in    PositionAcross  double                  [-]  ?
    out   -               Outcome<std::uint32_t>  [-]  ?
    by    Api/SurfaceTileSpace.h

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE CELLS
//------------------------------------------------------------------------------------------------------------------------

F CellSpace::Construct                  | SurfaceTileSpace.cpp | 82-99   | - | - | ?
    out   -  void  [-]  ?

F CellSpace::Held                       | SurfaceTileSpace.cpp | 101-107 | - | - | ?
    in    CellOrdinal  std::uint32_t               [-]  ?
    out   -            Outcome<const CellRecord*>  [-]  ?

F CellSpace::Amend                      | SurfaceTileSpace.cpp | 109-115 | - | - | ?
    in    CellOrdinal  std::uint32_t         [-]  ?
    out   -            Outcome<CellRecord*>  [-]  ?

F CellSpace::Records                    | SurfaceTileSpace.cpp | 117     | - | - | ?
    out   -  const std::vector<CellRecord>&  [-]  ?

F CellSpace::ResidentCount              | SurfaceTileSpace.cpp | 119     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F CellSpace::UncommittedCount           | SurfaceTileSpace.cpp | 120     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F CellSpace::DeclareResident            | SurfaceTileSpace.cpp | 122-133 | - | - | ?
    in    CellOrdinal      std::uint32_t  [-]  ?
    in    SlotOrdinal      std::uint32_t  [-]  ?
    in    RotationOrdinal  std::uint64_t  [-]  ?
    out   -                void           [-]  ?

F CellSpace::DeclareAbsent              | SurfaceTileSpace.cpp | 135-146 | - | - | ?
    in    CellOrdinal  std::uint32_t  [-]  ?
    out   -            void           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F SurfaceTileSpace::Construct           | SurfaceTileSpace.cpp | 152-206 | - | - | ?
    in    SurfaceOrdinal_  std::uint32_t  [-]  ?
    in    BytesPerTexel    std::uint32_t  [-]  ?
    in    SlotCeiling      std::uint32_t  [-]  ?
    out   -                Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        SAMPLING
//------------------------------------------------------------------------------------------------------------------------

F SurfaceTileSpace::Sample              | SurfaceTileSpace.cpp | 212-271 | - | - | ?
    in    Level            std::uint32_t         [-]  ?
    in    PositionAlong    double                [-]  ?
    in    PositionAcross   double                [-]  ?
    in    RotationOrdinal  std::uint64_t         [-]  ?
    in    Requesting       RequestQueue&         [-]  ?
    out   -                Outcome<SampledCell>  [-]  ?

F SurfaceTileSpace::SampleGuaranteed    | SurfaceTileSpace.cpp | 273-304 | - | - | ?
    in    PositionAlong   double                [-]  ?
    in    PositionAcross  double                [-]  ?
    out   -               Outcome<SampledCell>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   UNCOMMITTED PAINT
//------------------------------------------------------------------------------------------------------------------------

F SurfaceTileSpace::DeclareUncommitted  | SurfaceTileSpace.cpp | 310-336 | - | - | ?
    in    CellOrdinal          std::uint32_t  [-]  ?
    in    UncommittedDeclared  bool           [-]  ?
    out   -                    Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       PROMOTION
//------------------------------------------------------------------------------------------------------------------------

F SurfaceTileSpace::ClaimOrEvict        | SurfaceTileSpace.cpp | 342-400 | - | - | ?
    in    Scheduling       PromotionScheduler&     [-]  ?
    in    RotationOrdinal  std::uint64_t           [-]  ?
    out   -                Outcome<std::uint32_t>  [-]  ?

F SurfaceTileSpace::Promote             | SurfaceTileSpace.cpp | 402-483 | - | - | ?
    in    CellOrdinal      std::uint32_t                  [-]  ?
    in    Costing          const PromotionCost&           [-]  ?
    in    ContentRevision  std::uint64_t                  [-]  ?
    in    Scheduling       PromotionScheduler&            [-]  ?
    in    RotationOrdinal  std::uint64_t                  [-]  ?
    out   -                Outcome<PromotionDisposition>  [-]  ?

F SurfaceTileSpace::DeclareApronWritten | SurfaceTileSpace.cpp | 485-498 | - | - | ?
    in    CellOrdinal  std::uint32_t  [-]  ?
    out   -            Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  EVICTION AND RECLAIM
//------------------------------------------------------------------------------------------------------------------------

F SurfaceTileSpace::Evict               | SurfaceTileSpace.cpp | 504-533 | - | - | ?
    in    CellOrdinal      std::uint32_t  [-]  ?
    in    RotationOrdinal  std::uint64_t  [-]  ?
    out   -                Outcome<bool>  [-]  ?

F SurfaceTileSpace::Reconcile           | SurfaceTileSpace.cpp | 535-538 | - | - | ?
    in    RotationOrdinal  std::uint64_t  [-]  ?
    out   -                std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE MEASURES
//------------------------------------------------------------------------------------------------------------------------

F SurfaceTileSpace::Report              | SurfaceTileSpace.cpp | 544-562 | - | - | ?
    in    Measured    MeasureIndex&              [-]  ?
    in    Scheduling  const PromotionScheduler&  [-]  ?
    in    Sampled     TickPoint                  [-]  ?
    out   -           void                       [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

F SurfaceTileSpace::Cells               | SurfaceTileSpace.cpp | 568     | - | - | ?
    out   -  const CellSpace&  [-]  ?

F SurfaceTileSpace::Tiles               | SurfaceTileSpace.cpp | 569     | - | - | ?
    out   -  const TileSpace&  [-]  ?

F SurfaceTileSpace::Depot               | SurfaceTileSpace.cpp | 570     | - | - | ?
    out   -  SurfaceDepot&  [-]  ?

F SurfaceTileSpace::Depot               | SurfaceTileSpace.cpp | 571     | - | - | ?
    out   -  const SurfaceDepot&  [-]  ?

F SurfaceTileSpace::SurfaceOrdinal      | SurfaceTileSpace.cpp | 573     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F SurfaceTileSpace::StoredBytesPerTile  | SurfaceTileSpace.cpp | 574     | - | - | ?
    out   -  std::uint64_t  [-]  ?

F SurfaceTileSpace::ResidencyValid      | SurfaceTileSpace.cpp | 576-621 | - | - | ?
    in    RotationOrdinal  std::uint64_t  [-]  ?
    out   -                bool           [-]  ?
