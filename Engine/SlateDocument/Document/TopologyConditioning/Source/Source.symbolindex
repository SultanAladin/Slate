//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Lattice welding, corner adjacency, orientation consistency, and conservative extents.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/TopologyConditioning/Source
%layer      SlateDocument
%sources    1
%symbols    26
%annotated  0/26
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S TopologyConditioning.cpp | 788 lines | e3c8fbeb | 26 sym | Lattice welding, corner adjacency, orientation consistency, and conservative extents.

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE LATTICE
//------------------------------------------------------------------------------------------------------------------------

V AbsentCorner                               | TopologyConditioning.cpp | 23      | - | - | ?

T LatticeCell                                | TopologyConditioning.cpp | 32-37   | - | - | ?
    has   CellAlong   std::int64_t  [-]  ?
    has   CellAcross  std::int64_t  [-]  ?
    has   CellDeep    std::int64_t  [-]  ?

F Quantise                                   | TopologyConditioning.cpp | 39-47   | - | - | ?
    in    Subject  DocumentPosition  [-]  ?
    in    Spacing  double            [-]  ?
    out   -        LatticeCell       [-]  ?
    by    Source/SpatialManipulator.cpp

F CellOrdinal                                | TopologyConditioning.cpp | 49-58   | - | - | ?
    in    Cell  LatticeCell    [-]  ?
    out   -     std::uint64_t  [-]  ?
    by    Api/PromotionScheduler.h, Api/RequestQueue.h, Api/StrokeSpace.h, Api/SurfaceTileSpace.h, Source/ImpressionSequence.cpp, Source/PromotionScheduler.cpp, (+3 more)

F GreatestSpan                               | TopologyConditioning.cpp | 60-86   | - | - | ?
    in    Positions  const std::vector<DocumentPosition>&  [-]  ?
    out   -          double                                [-]  ?

F Normalise                                  | TopologyConditioning.cpp | 88-102  | - | - | ?
    in    DirectionX  double            [-]  ?
    in    DirectionY  double            [-]  ?
    in    DirectionZ  double            [-]  ?
    out   -           SurfaceDirection  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        WELDING
//------------------------------------------------------------------------------------------------------------------------

F TopologyConditioning::DeriveWelding        | TopologyConditioning.cpp | 110-199 | - | - | ?
    in    Imported  const TopologyStructure&  [-]  ?
    out   -         void                      [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       ADJACENCY
//------------------------------------------------------------------------------------------------------------------------

F TopologyConditioning::DeriveAdjacency      | TopologyConditioning.cpp | 205-306 | - | - | ?
    in    Imported  const TopologyStructure&  [-]  ?
    out   -         void                      [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      ORIENTATION
//------------------------------------------------------------------------------------------------------------------------

F TopologyConditioning::DeriveOrientation    | TopologyConditioning.cpp | 312-436 | - | - | ?
    in    Imported  const TopologyStructure&  [-]  ?
    out   -         void                      [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  DIRECTION DERIVATION
//------------------------------------------------------------------------------------------------------------------------

F TopologyConditioning::DerivePerpendiculars | TopologyConditioning.cpp | 442-506 | - | - | ?
    in    Imported  const TopologyStructure&  [-]  ?
    out   -         void                      [-]  ?

F TopologyConditioning::DeriveTangentBases   | TopologyConditioning.cpp | 508-629 | - | - | ?
    in    Imported  const TopologyStructure&  [-]  ?
    out   -         void                      [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        EXTENTS
//------------------------------------------------------------------------------------------------------------------------

F TopologyConditioning::DeriveExtents        | TopologyConditioning.cpp | 635-696 | - | - | ?
    in    Imported  const TopologyStructure&  [-]  ?
    out   -         void                      [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE CONDITIONING
//------------------------------------------------------------------------------------------------------------------------

F TopologyConditioning::Condition            | TopologyConditioning.cpp | 702-727 | - | - | ?
    in    Imported  const TopologyStructure&  [-]  ?
    out   -         Deliver<bool>             [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

F TopologyConditioning::WeldedPosition       | TopologyConditioning.cpp | 733-742 | - | - | ?
    in    VertexOrdinal  std::uint32_t           [-]  ?
    out   -              Deliver<std::uint32_t>  [-]  ?

F TopologyConditioning::AdjacentCorner       | TopologyConditioning.cpp | 744-756 | - | - | ?
    in    CornerOrdinal  std::uint32_t           [-]  ?
    out   -              Deliver<std::uint32_t>  [-]  ?

F TopologyConditioning::FaceEnrolled         | TopologyConditioning.cpp | 758-761 | - | - | ?
    in    FaceOrdinal  std::uint32_t      [-]  ?
    in    Condition    DegeneracySubject  [-]  ?
    out   -            bool               [-]  ?

F TopologyConditioning::VertexIsolated       | TopologyConditioning.cpp | 763-771 | - | - | ?
    in    VertexOrdinal  std::uint32_t  [-]  ?
    out   -              bool           [-]  ?

F TopologyConditioning::Enrolled             | TopologyConditioning.cpp | 773-776 | - | - | ?
    in    Condition  DegeneracySubject                     [-]  ?
    out   -          const std::vector<EnrolledInterval>&  [-]  ?

F TopologyConditioning::Perpendiculars       | TopologyConditioning.cpp | 778     | - | - | ?
    out   -  const std::vector<SurfaceDirection>&  [-]  ?

F TopologyConditioning::TangentBases         | TopologyConditioning.cpp | 779     | - | - | ?
    out   -  const std::vector<TangentBasis>&  [-]  ?

F TopologyConditioning::FaceExtents          | TopologyConditioning.cpp | 780     | - | - | ?
    out   -  const std::vector<ConditionedExtent>&  [-]  ?

F TopologyConditioning::TopologyExtent       | TopologyConditioning.cpp | 782     | - | - | ?
    out   -  ConditionedExtent  [-]  ?

F TopologyConditioning::WeldedCount          | TopologyConditioning.cpp | 783     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F TopologyConditioning::ConditionedRevision  | TopologyConditioning.cpp | 784     | - | - | ?
    out   -  std::uint64_t  [-]  ?

F TopologyConditioning::TangentBasesRetained | TopologyConditioning.cpp | 785     | - | - | ?
    out   -  bool  [-]  ?

F TopologyConditioning::UnorientedCount      | TopologyConditioning.cpp | 786     | - | - | ?
    out   -  std::uint32_t  [-]  ?
