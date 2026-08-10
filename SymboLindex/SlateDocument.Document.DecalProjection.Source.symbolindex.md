//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The inverse of a decomposed placing transform, the two extent derivations, and the drag that records nothing until release.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/DecalProjection/Source
%layer      SlateDocument
%sources    1
%symbols    21
%annotated  0/21
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S DecalProjection.cpp | 503 lines | 36b65f9f | 21 sym | The inverse of a decomposed placing transform, the two extent derivations, and the drag that records nothing until release.

//------------------------------------------------------------------------------------------------------------------------
//                                                   TRANSFORM HELPERS
//------------------------------------------------------------------------------------------------------------------------

F Conjugated                         | DecalProjection.cpp | 20-29   | - | - | ?
    in    Subject  RotationQuaternion  [-]  ?
    out   -        RotationQuaternion  [-]  ?
    by    Source/CameraProjection.cpp, Source/SpatialSubdivision.cpp

F RotateSpan                         | DecalProjection.cpp | 33-48   | - | - | ?
    in    Rotation  RotationQuaternion  [-]  ?
    in    SpanX     double              [-]  ?
    in    SpanY     double              [-]  ?
    in    SpanZ     double              [-]  ?
    in    OutX      double&             [-]  ?
    in    OutY      double&             [-]  ?
    in    OutZ      double&             [-]  ?
    out   -         void                [-]  ?
    by    Source/CameraProjection.cpp, Source/PointerIntersection.cpp, Source/SpatialSubdivision.cpp

F ProjectPlanar                      | DecalProjection.cpp | 52-67   | - | - | ?
    in    Placing       const DecomposedTransform&  [-]  ?
    in    SourceAlong   double                      [-]  ?
    in    SourceAcross  double                      [-]  ?
    in    AlongOut      double&                     [-]  ?
    in    AcrossOut     double&                     [-]  ?
    out   -             void                        [-]  ?

F WidenOutward                       | DecalProjection.cpp | 72-78   | - | - | ?
    in    Widening  DomainExtent&  [-]  ?
    out   -         void           [-]  ?

F AdmitPosition                      | DecalProjection.cpp | 80-98   | - | - | ?
    in    Running         DomainExtent&  [-]  ?
    in    Along           double         [-]  ?
    in    Across          double         [-]  ?
    in    FirstAdmission  bool&          [-]  ?
    out   -               void           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                 INTO THE SOURCE SPACE
//------------------------------------------------------------------------------------------------------------------------

F ProjectIntoSource                  | DecalProjection.cpp | 106-140 | - | - | ?
    in    Placed          const PlacementSpecification&  [-]  ?
    in    PositionAlong   double                         [-]  ?
    in    PositionAcross  double                         [-]  ?
    in    SourceAlong     double&                        [-]  ?
    in    SourceAcross    double&                        [-]  ?
    out   -               bool                           [-]  ?
    by    Api/DecalProjection.h, Source/AnalyticProjection.cpp, Source/PointerIntersection.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE DERIVED EXTENT
//------------------------------------------------------------------------------------------------------------------------

F ProjectPlacementExtent             | DecalProjection.cpp | 146-275 | - | - | ?
    in    Placed             const PlacementSpecification&         [-]  ?
    in    PlacementOrdinal   std::uint32_t                         [-]  ?
    in    SequenceOrdinal    std::uint32_t                         [-]  ?
    in    Imported           const TopologyStructure&              [-]  ?
    in    CornerCoordinates  const std::vector<DomainCoordinate>&  [-]  ?
    out   -                  Outcome<DomainExtent>                 [-]  ?
    by    Api/DecalProjection.h

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PLACEMENTS
//------------------------------------------------------------------------------------------------------------------------

F PlacementIndex::Declare            | DecalProjection.cpp | 281-339 | - | - | ?
    in    Declaring  const PlacementSpecification&  [-]  ?
    out   -          Outcome<std::uint32_t>         [-]  ?

F PlacementIndex::Amend              | DecalProjection.cpp | 341-382 | - | - | ?
    in    PlacementOrdinal  std::uint32_t                  [-]  ?
    in    Amending          const PlacementSpecification&  [-]  ?
    out   -                 Outcome<bool>                  [-]  ?

F PlacementIndex::Resolve            | DecalProjection.cpp | 384-393 | - | - | ?
    in    PlacementOrdinal  std::uint32_t                           [-]  ?
    out   -                 Outcome<const PlacementSpecification*>  [-]  ?

F PlacementIndex::Withdraw           | DecalProjection.cpp | 395-411 | - | - | ?
    in    PlacementOrdinal  std::uint32_t  [-]  ?
    out   -                 Outcome<bool>  [-]  ?

F PlacementIndex::Revision           | DecalProjection.cpp | 413-419 | - | - | ?
    in    PlacementOrdinal  std::uint32_t  [-]  ?
    out   -                 std::uint64_t  [-]  ?

F PlacementIndex::DeclaredCount      | DecalProjection.cpp | 421-424 | - | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE POSITIONING DRAG
//------------------------------------------------------------------------------------------------------------------------

F PlacementSequence::Open            | DecalProjection.cpp | 430-444 | - | - | ?
    in    PlacementOrdinal  std::uint32_t                  [-]  ?
    in    Standing          const PlacementSpecification&  [-]  ?
    in    CameraFollowed_   bool                           [-]  ?
    out   -                 Outcome<bool>                  [-]  ?

F PlacementSequence::Amend           | DecalProjection.cpp | 446-454 | - | - | ?
    in    Amending  const DecomposedTransform&  [-]  ?
    out   -         Outcome<bool>               [-]  ?

F PlacementSequence::Abandon         | DecalProjection.cpp | 456-472 | - | - | ?
    out   -  Outcome<PlacementSpecification>  [-]  ?

F PlacementSequence::Seal            | DecalProjection.cpp | 474-496 | - | - | ?
    out   -  Outcome<PlacementSpecification>  [-]  ?

F PlacementSequence::Amended         | DecalProjection.cpp | 498     | - | - | ?
    out   -  const PlacementSpecification&  [-]  ?

F PlacementSequence::Subject         | DecalProjection.cpp | 499     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F PlacementSequence::GestureOpen     | DecalProjection.cpp | 500     | - | - | ?
    out   -  bool  [-]  ?

F PlacementSequence::CameraFollowing | DecalProjection.cpp | 501     | - | - | ?
    out   -  bool  [-]  ?
