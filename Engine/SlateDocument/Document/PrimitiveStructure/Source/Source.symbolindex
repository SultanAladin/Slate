//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Parametric polygon generation — the closed set of solids every authored surface and every manipulator grip is built from.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/PrimitiveStructure/Source
%layer      SlateDocument
%sources    1
%symbols    21
%annotated  0/21
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S PrimitiveStructure.cpp | 732 lines | 9d306a90 | 21 sym | Parametric polygon generation — the closed set of solids every authored surface and every manipulator grip is built from.

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE ACCUMULATOR
//------------------------------------------------------------------------------------------------------------------------

T GeneratedSurface              | PrimitiveStructure.cpp | 24-77   | - | - | ?
    has   Positions       std::vector<DocumentPosition>            [-]  ?
    has   Perpendiculars  std::vector<SurfaceDirection>            [-]  ?
    has   Coordinates     std::vector<DomainCoordinate>            [-]  ?
    has   Faces           std::vector<std::vector<std::uint32_t>>  [-]  ?

F GeneratedSurface::Emit        | PrimitiveStructure.cpp | 31-65   | - | - | ?
    in    PositionAlong    double         [-]  ?
    in    PositionUp       double         [-]  ?
    in    PositionAcross   double         [-]  ?
    in    DirectionAlong   double         [-]  ?
    in    DirectionUp      double         [-]  ?
    in    DirectionAcross  double         [-]  ?
    in    DomainAlong      double         [-]  ?
    in    DomainAcross     double         [-]  ?
    out   -                std::uint32_t  [-]  ?
    by    Api/ImpressionSequence.h, Source/ImpressionSequence.cpp

F GeneratedSurface::EmitFace    | PrimitiveStructure.cpp | 67-70   | - | - | ?
    in    FirstCorner   std::uint32_t  [-]  ?
    in    SecondCorner  std::uint32_t  [-]  ?
    in    ThirdCorner   std::uint32_t  [-]  ?
    out   -             void           [-]  ?

F GeneratedSurface::EmitFace    | PrimitiveStructure.cpp | 72-76   | - | - | ?
    in    FirstCorner   std::uint32_t  [-]  ?
    in    SecondCorner  std::uint32_t  [-]  ?
    in    ThirdCorner   std::uint32_t  [-]  ?
    in    FourthCorner  std::uint32_t  [-]  ?
    out   -             void           [-]  ?

V Turn                          | PrimitiveStructure.cpp | 79      | - | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE BOX
//------------------------------------------------------------------------------------------------------------------------

F GenerateBox                   | PrimitiveStructure.cpp | 88-145  | - | - | ?
    in    Declaring   const PrimitiveSpecification&  [-]  ?
    in    Generating  GeneratedSurface&              [-]  ?
    out   -           void                           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE SPHERE
//------------------------------------------------------------------------------------------------------------------------

F GenerateSphere                | PrimitiveStructure.cpp | 151-211 | - | - | ?
    in    Declaring   const PrimitiveSpecification&  [-]  ?
    in    Generating  GeneratedSurface&              [-]  ?
    out   -           void                           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                               THE SURFACE OF REVOLUTION
//------------------------------------------------------------------------------------------------------------------------

F GenerateRevolution            | PrimitiveStructure.cpp | 220-333 | - | - | ?
    in    Declaring      const PrimitiveSpecification&  [-]  ?
    in    Generating     GeneratedSurface&              [-]  ?
    in    UpperFraction  double                         [-]  ?
    out   -              void                           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE TORUS
//------------------------------------------------------------------------------------------------------------------------

F GenerateTorus                 | PrimitiveStructure.cpp | 339-386 | - | - | ?
    in    Declaring   const PrimitiveSpecification&  [-]  ?
    in    Generating  GeneratedSurface&              [-]  ?
    out   -           void                           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE PLANE
//------------------------------------------------------------------------------------------------------------------------

F GeneratePlane                 | PrimitiveStructure.cpp | 392-426 | - | - | ?
    in    Declaring   const PrimitiveSpecification&  [-]  ?
    in    Generating  GeneratedSurface&              [-]  ?
    out   -           void                           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE ANNULAR SECTOR
//------------------------------------------------------------------------------------------------------------------------

F GenerateAnnularSector         | PrimitiveStructure.cpp | 436-468 | - | - | ?
    in    Declaring   const PrimitiveSpecification&  [-]  ?
    in    Generating  GeneratedSurface&              [-]  ?
    out   -           void                           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE GENERATION
//------------------------------------------------------------------------------------------------------------------------

F GeneratePrimitive             | PrimitiveStructure.cpp | 476-551 | - | - | ?
    in    Declaring  const PrimitiveSpecification&  [-]  ?
    in    Generated  TopologyStructure&             [-]  ?
    out   -          Outcome<bool>                  [-]  ?
    by    Api/PrimitiveStructure.h

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE EXTENT
//------------------------------------------------------------------------------------------------------------------------

F ProjectPrimitiveExtent        | PrimitiveStructure.cpp | 557-592 | - | - | ?
    in    Declaring  const PrimitiveSpecification&  [-]  ?
    in    Least      DocumentPosition&              [-]  ?
    in    Greatest   DocumentPosition&              [-]  ?
    out   -          void                           [-]  ?
    by    Api/PrimitiveStructure.h

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DECLARATIONS
//------------------------------------------------------------------------------------------------------------------------

F PrimitiveIndex::Declare       | PrimitiveStructure.cpp | 598-636 | - | - | ?
    in    Declaring  const PrimitiveSpecification&  [-]  ?
    out   -          Outcome<std::uint32_t>         [-]  ?

F PrimitiveIndex::Amend         | PrimitiveStructure.cpp | 638-674 | - | - | ?
    in    PrimitiveOrdinal  std::uint32_t                  [-]  ?
    in    Amending          const PrimitiveSpecification&  [-]  ?
    out   -                 Outcome<bool>                  [-]  ?

F PrimitiveIndex::Resolve       | PrimitiveStructure.cpp | 676-685 | - | - | ?
    in    PrimitiveOrdinal  std::uint32_t                           [-]  ?
    out   -                 Outcome<const PrimitiveSpecification*>  [-]  ?

F PrimitiveIndex::Withdraw      | PrimitiveStructure.cpp | 687-701 | - | - | ?
    in    PrimitiveOrdinal  std::uint32_t  [-]  ?
    out   -                 Outcome<bool>  [-]  ?

F PrimitiveIndex::Revision      | PrimitiveStructure.cpp | 703-711 | - | - | ?
    in    PrimitiveOrdinal  std::uint32_t  [-]  ?
    out   -                 std::uint64_t  [-]  ?

F PrimitiveIndex::DeclaredCount | PrimitiveStructure.cpp | 713-716 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F PrimitiveIndex::SpannedCount  | PrimitiveStructure.cpp | 718-721 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F PrimitiveIndex::Reclaim       | PrimitiveStructure.cpp | 723-730 | - | - | ?
    out   -  void  [-]  ?
