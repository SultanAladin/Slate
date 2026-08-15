//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Adaptive subdivision, endpoint arc parameterisation, and bevelled offsetting.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Numeric/CurveSolver/Source
%layer      SlateMath
%sources    1
%symbols    8
%annotated  0/8
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S CurveSolver.cpp | 321 lines | d5b6654e | 8 sym | Adaptive subdivision, endpoint arc parameterisation, and bevelled offsetting.

//------------------------------------------------------------------------------------------------------------------------
//                                                      SUBDIVISION
//------------------------------------------------------------------------------------------------------------------------

V SubdivisionCeiling | CurveSolver.cpp | 25      | - | - | ?
    by    Api/ChartPartition.h, Api/PrimitiveStructure.h, Source/ChartPartition.cpp

F ChordDeviation     | CurveSolver.cpp | 27-44   | - | - | ?
    in    Origin    PlanarPosition  [-]  ?
    in    Control   PlanarPosition  [-]  ?
    in    Terminus  PlanarPosition  [-]  ?
    out   -         double          [-]  ?

F Interpolate        | CurveSolver.cpp | 46-53   | - | - | ?
    in    Earlier   PlanarPosition  [-]  ?
    in    Later     PlanarPosition  [-]  ?
    in    Fraction  double          [-]  ?
    out   -         PlanarPosition  [-]  ?

F SubdivideCubic     | CurveSolver.cpp | 55-84   | - | - | ?
    in    Origin         PlanarPosition                [-]  ?
    in    FirstControl   PlanarPosition                [-]  ?
    in    SecondControl  PlanarPosition                [-]  ?
    in    Terminus       PlanarPosition                [-]  ?
    in    Tolerance      double                        [-]  ?
    in    Remaining      std::uint32_t                 [-]  ?
    in    Appending      std::vector<PlanarPosition>&  [-]  ?
    out   -              void                          [-]  ?

F FlattenArc         | CurveSolver.cpp | 86-193  | - | - | ?
    in    Origin     PlanarPosition                [-]  ?
    in    Segment    const PathSegment&            [-]  ?
    in    Tolerance  double                        [-]  ?
    in    Appending  std::vector<PlanarPosition>&  [-]  ?
    out   -          void                          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       FLATTENING
//------------------------------------------------------------------------------------------------------------------------

F Flatten            | CurveSolver.cpp | 201-241 | - | - | ?
    in    Origin     PlanarPosition                [-]  ?
    in    Segment    const PathSegment&            [-]  ?
    in    Tolerance  double                        [-]  ?
    in    Appending  std::vector<PlanarPosition>&  [-]  ?
    out   -          void                          [-]  ?
    by    Api/CurveSolver.h, Api/VectorInterchange.h, Source/AnalyticProjection.cpp, Source/ConsoleHost.cpp, Source/VectorInterchange.cpp

F Flatten            | CurveSolver.cpp | 243-260 | - | - | ?
    in    Origin     PlanarPosition                   [-]  ?
    in    Segments   const std::vector<PathSegment>&  [-]  ?
    in    Tolerance  double                           [-]  ?
    out   -          std::vector<PlanarPosition>      [-]  ?
    by    Api/CurveSolver.h, Api/VectorInterchange.h, Source/AnalyticProjection.cpp, Source/ConsoleHost.cpp, Source/VectorInterchange.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                       OFFSETTING
//------------------------------------------------------------------------------------------------------------------------

F OffsetOutline      | CurveSolver.cpp | 266-319 | - | - | ?
    in    Traversed  const std::vector<PlanarPosition>&    [-]  ?
    in    HalfWidth  double                                [-]  ?
    in    ClosedRun  bool                                  [-]  ?
    out   -          Deliver<std::vector<PlanarPosition>>  [-]  ?
    by    Api/CurveSolver.h, Source/ConsoleHost.cpp
