//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 `78` — the grip layout, the screen-space grasp, and the four drag solves each grip resolves against.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/SpatialManipulator/Source
%layer      SlateDocument
%sources    1
%symbols    46
%annotated  0/46
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S SpatialManipulator.cpp | 1179 lines | f044ce46 | 46 sym | `78` — the grip layout, the screen-space grasp, and the four drag solves each grip resolves against.

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE SPAN ARITHMETIC
//------------------------------------------------------------------------------------------------------------------------

T DirectionSpan                      | SpatialManipulator.cpp | 25-30     | - | - | ?
    has   SpanX  double  [-]  ?
    has   SpanY  double  [-]  ?
    has   SpanZ  double  [-]  ?

V PiConstant                         | SpatialManipulator.cpp | 32        | - | - | ?

V DegreesToRadians                   | SpatialManipulator.cpp | 33        | - | - | ?

V ParallelEpsilon                    | SpatialManipulator.cpp | 34        | - | - | ?

F SpanDot                            | SpatialManipulator.cpp | 36-39     | - | - | ?
    in    First   const DirectionSpan&  [-]  ?
    in    Second  const DirectionSpan&  [-]  ?
    out   -       double                [-]  ?

F SpanCross                          | SpatialManipulator.cpp | 41-48     | - | - | ?
    in    First   const DirectionSpan&  [-]  ?
    in    Second  const DirectionSpan&  [-]  ?
    out   -       DirectionSpan         [-]  ?

F SpanScaled                         | SpatialManipulator.cpp | 50-57     | - | - | ?
    in    Source  const DirectionSpan&  [-]  ?
    in    Factor  double                [-]  ?
    out   -       DirectionSpan         [-]  ?

F SpanSum                            | SpatialManipulator.cpp | 59-66     | - | - | ?
    in    First   const DirectionSpan&  [-]  ?
    in    Second  const DirectionSpan&  [-]  ?
    out   -       DirectionSpan         [-]  ?

F NormaliseSpan                      | SpatialManipulator.cpp | 68-76     | - | - | ?
    in    Source  const DirectionSpan&  [-]  ?
    out   -       DirectionSpan         [-]  ?

F RotateSpan                         | SpatialManipulator.cpp | 81-96     | - | - | ?
    in    Rotation  RotationQuaternion    [-]  ?
    in    Source    const DirectionSpan&  [-]  ?
    out   -         DirectionSpan         [-]  ?
    by    Source/CameraProjection.cpp, Source/DecalProjection.cpp, Source/OcclusionProjection.cpp, Source/PointerIntersection.cpp, Source/SpatialSubdivision.cpp

F RotationAbout                      | SpatialManipulator.cpp | 98-110    | - | - | ?
    in    Axis     const DirectionSpan&  [-]  ?
    in    Radians  double                [-]  ?
    out   -        RotationQuaternion    [-]  ?
    by    Source/CameraProjection.cpp

F ProjectBasisRotation               | SpatialManipulator.cpp | 115-159   | - | - | ?
    in    FirstImage   const DirectionSpan&  [-]  ?
    in    SecondImage  const DirectionSpan&  [-]  ?
    in    ThirdImage   const DirectionSpan&  [-]  ?
    out   -            RotationQuaternion    [-]  ?

F Quantise                           | SpatialManipulator.cpp | 161-167   | - | - | ?
    in    Measured   double  [-]  ?
    in    Increment  double  [-]  ?
    out   -          double  [-]  ?
    by    Source/TopologyConditioning.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE MANIPULATOR BASIS
//------------------------------------------------------------------------------------------------------------------------

T ManipulatorBasis                   | SpatialManipulator.cpp | 178-183   | - | - | ?
    has   AxisSpan        DirectionSpan[3]  [-]  ?
    has   FirstPairSpan   DirectionSpan[3]  [-]  ?
    has   SecondPairSpan  DirectionSpan[3]  [-]  ?

F ProjectBasis                       | SpatialManipulator.cpp | 185-200   | - | - | ?
    in    Orientation  RotationQuaternion  [-]  ?
    out   -            ManipulatorBasis    [-]  ?

F LocalBasis                         | SpatialManipulator.cpp | 205-208   | - | - | ?
    out   -  ManipulatorBasis  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE GRIP COLOURS
//------------------------------------------------------------------------------------------------------------------------

F DeclareOverlayColour               | SpatialManipulator.cpp | 217-225   | - | - | ?
    in    RedByte    double               [-]  ?
    in    GreenByte  double               [-]  ?
    in    BlueByte   double               [-]  ?
    out   -          ColourSpecification  [-]  ?

F AxisColour                         | SpatialManipulator.cpp | 227-232   | - | - | ?
    in    AxisOrdinal  std::uint32_t        [-]  ?
    out   -            ColourSpecification  [-]  ?

F PlaneColour                        | SpatialManipulator.cpp | 234-239   | - | - | ?
    in    AxisOrdinal  std::uint32_t        [-]  ?
    out   -            ColourSpecification  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE INTERSECTIONS
//------------------------------------------------------------------------------------------------------------------------

F IntersectCapsule                   | SpatialManipulator.cpp | 248-311   | - | - | ?
    in    RayOrigin     const DirectionSpan&  [-]  ?
    in    RayDirection  const DirectionSpan&  [-]  ?
    in    SpanNear      const DirectionSpan&  [-]  ?
    in    SpanFar       const DirectionSpan&  [-]  ?
    in    HalfExtent    double                [-]  ?
    in    RayParameter  double&               [-]  ?
    out   -             bool                  [-]  ?

F SolveAxisParameter                 | SpatialManipulator.cpp | 316-340   | - | - | ?
    in    RayOrigin      const DirectionSpan&  [-]  ?
    in    RayDirection   const DirectionSpan&  [-]  ?
    in    LineOrigin     const DirectionSpan&  [-]  ?
    in    LineDirection  const DirectionSpan&  [-]  ?
    in    LineParameter  double&               [-]  ?
    out   -              bool                  [-]  ?

F SolvePlanePoint                    | SpatialManipulator.cpp | 342-367   | - | - | ?
    in    RayOrigin           const DirectionSpan&  [-]  ?
    in    RayDirection        const DirectionSpan&  [-]  ?
    in    PlaneOrigin         const DirectionSpan&  [-]  ?
    in    PlanePerpendicular  const DirectionSpan&  [-]  ?
    in    Met                 DirectionSpan&        [-]  ?
    out   -                   bool                  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE CONVERSIONS
//------------------------------------------------------------------------------------------------------------------------

F SpanOfPosition                     | SpatialManipulator.cpp | 373-380   | - | - | ?
    in    Position  DocumentPosition  [-]  ?
    out   -         DirectionSpan     [-]  ?

F PositionOfSpan                     | SpatialManipulator.cpp | 382-389   | - | - | ?
    in    Span  const DirectionSpan&  [-]  ?
    out   -     DocumentPosition      [-]  ?

F CarryToDocument                    | SpatialManipulator.cpp | 391-398   | - | - | ?
    in    Local        const DirectionSpan&  [-]  ?
    in    Origin       DocumentPosition      [-]  ?
    in    Orientation  RotationQuaternion    [-]  ?
    in    UnitExtent   double                [-]  ?
    out   -            DirectionSpan         [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE GRIP SOLIDS
//------------------------------------------------------------------------------------------------------------------------

F DeclareConeGrip                    | SpatialManipulator.cpp | 407-436   | - | - | ?
    in    AxisOrdinal  std::uint32_t         [-]  ?
    in    AxisLocal    const DirectionSpan&  [-]  ?
    in    FirstPair    const DirectionSpan&  [-]  ?
    in    SecondPair   const DirectionSpan&  [-]  ?
    out   -            ManipulationGrip      [-]  ?

F DeclareScaleGrip                   | SpatialManipulator.cpp | 438-466   | - | - | ?
    in    AxisOrdinal  std::uint32_t         [-]  ?
    in    AxisLocal    const DirectionSpan&  [-]  ?
    in    FirstPair    const DirectionSpan&  [-]  ?
    in    SecondPair   const DirectionSpan&  [-]  ?
    out   -            ManipulationGrip      [-]  ?

F DeclarePlaneGrip                   | SpatialManipulator.cpp | 468-503   | - | - | ?
    in    AxisOrdinal  std::uint32_t         [-]  ?
    in    AxisLocal    const DirectionSpan&  [-]  ?
    in    FirstPair    const DirectionSpan&  [-]  ?
    in    SecondPair   const DirectionSpan&  [-]  ?
    out   -            ManipulationGrip      [-]  ?

F DeclareRotationGrip                | SpatialManipulator.cpp | 505-550   | - | - | ?
    in    AxisOrdinal  std::uint32_t         [-]  ?
    in    AxisLocal    const DirectionSpan&  [-]  ?
    in    FirstPair    const DirectionSpan&  [-]  ?
    in    SecondPair   const DirectionSpan&  [-]  ?
    out   -            ManipulationGrip      [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE LAYOUT
//------------------------------------------------------------------------------------------------------------------------

F ManipulationLayout::Layout         | SpatialManipulator.cpp | 558-713   | - | - | ?
    in    Origin       DocumentPosition         [-]  ?
    in    Orientation  RotationQuaternion       [-]  ?
    in    Camera       const CameraProjection&  [-]  ?
    in    Addressing   ManipulatedSubject       [-]  ?
    out   -            Outcome<bool>            [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE GRASP
//------------------------------------------------------------------------------------------------------------------------

F ManipulationLayout::Grasp          | SpatialManipulator.cpp | 719-785   | - | - | ?
    in    Camera         const CameraProjection&  [-]  ?
    in    PointerAlong   double                   [-]  ?
    in    PointerAcross  double                   [-]  ?
    in    DisplayAlong   std::uint32_t            [-]  ?
    in    DisplayAcross  std::uint32_t            [-]  ?
    out   -              Outcome<std::uint32_t>   [-]  ?

F ManipulationLayout::Resolve        | SpatialManipulator.cpp | 787-802   | - | - | ?
    in    GripOrdinal  std::uint32_t                     [-]  ?
    out   -            Outcome<const ManipulationGrip*>  [-]  ?

F ManipulationLayout::Grips          | SpatialManipulator.cpp | 804-807   | - | - | ?
    out   -  const std::vector<ManipulationGrip>&  [-]  ?

F ManipulationLayout::Origin         | SpatialManipulator.cpp | 809-812   | - | - | ?
    out   -  DocumentPosition  [-]  ?

F ManipulationLayout::Orientation    | SpatialManipulator.cpp | 814-817   | - | - | ?
    out   -  RotationQuaternion  [-]  ?

F ManipulationLayout::UnitExtent     | SpatialManipulator.cpp | 819-822   | - | - | ?
    out   -  double  [-]  ?

F ManipulationLayout::LayoutStanding | SpatialManipulator.cpp | 824-827   | - | - | ?
    out   -  bool  [-]  ?

F ManipulationLayout::Reclaim        | SpatialManipulator.cpp | 829-837   | - | - | ?
    out   -  void  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     OPENING A DRAG
//------------------------------------------------------------------------------------------------------------------------

F ManipulationSequence::Open         | SpatialManipulator.cpp | 843-961   | - | - | ?
    in    Grasping       const ManipulationGrip&    [-]  ?
    in    Laid           const ManipulationLayout&  [-]  ?
    in    Camera         const CameraProjection&    [-]  ?
    in    PointerAlong   double                     [-]  ?
    in    PointerAcross  double                     [-]  ?
    in    DisplayAlong   std::uint32_t              [-]  ?
    in    DisplayAcross  std::uint32_t              [-]  ?
    out   -              Outcome<bool>              [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    AMENDING A DRAG
//------------------------------------------------------------------------------------------------------------------------

F ManipulationSequence::Amend        | SpatialManipulator.cpp | 967-1115  | - | - | ?
    in    PointerAlong   double         [-]  ?
    in    PointerAcross  double         [-]  ?
    in    DisplayAlong   std::uint32_t  [-]  ?
    in    DisplayAcross  std::uint32_t  [-]  ?
    in    SnapDeclared   bool           [-]  ?
    out   -              Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     ENDING A DRAG
//------------------------------------------------------------------------------------------------------------------------

F ManipulationSequence::Abandon      | SpatialManipulator.cpp | 1121-1140 | - | - | ?
    out   -  Outcome<ManipulationAmendment>  [-]  ?

F ManipulationSequence::Seal         | SpatialManipulator.cpp | 1142-1157 | - | - | ?
    out   -  Outcome<ManipulationAmendment>  [-]  ?

F ManipulationSequence::Amended      | SpatialManipulator.cpp | 1159-1162 | - | - | ?
    out   -  const ManipulationAmendment&  [-]  ?

F ManipulationSequence::DragOpen     | SpatialManipulator.cpp | 1164-1167 | - | - | ?
    out   -  bool  [-]  ?

F ManipulationSequence::Constrained  | SpatialManipulator.cpp | 1169-1172 | - | - | ?
    out   -  ConstraintSubject  [-]  ?

F ManipulationSequence::Grasped      | SpatialManipulator.cpp | 1174-1177 | - | - | ?
    out   -  const ManipulationGrip&  [-]  ?
