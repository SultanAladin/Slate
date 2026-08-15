//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Primaries derived from chromaticities, the transfers, von Kries adaptation, and the Planckian locus.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Numeric/ColourProjection/Source
%layer      SlateMath
%sources    1
%symbols    10
%annotated  0/10
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ColourProjection.cpp | 430 lines | 7039a605 | 10 sym | Primaries derived from chromaticities, the transfers, von Kries adaptation, and the Planckian locus.

//------------------------------------------------------------------------------------------------------------------------
//                                                    PRIMARIES TO XYZ
//------------------------------------------------------------------------------------------------------------------------

T TristimulusProjection | ColourProjection.cpp | 23-29   | - | - | ?
    has   Coefficient  double[9]  [-]  ?
    has   Derived      bool       [-]  ?

F Invert                | ColourProjection.cpp | 31-56   | - | - | ?
    in    Forward   const TristimulusProjection&  [-]  ?
    in    Inverted  TristimulusProjection&        [-]  ?
    out   -         bool                          [-]  ?

F Apply                 | ColourProjection.cpp | 58-68   | - | - | ?
    in    Projection  const TristimulusProjection&  [-]  ?
    in    LeftTerm    double                        [-]  ?
    in    MiddleTerm  double                        [-]  ?
    in    RightTerm   double                        [-]  ?
    in    FirstOut    double&                       [-]  ?
    in    SecondOut   double&                       [-]  ?
    in    ThirdOut    double&                       [-]  ?
    out   -           void                          [-]  ?

F DeriveProjection      | ColourProjection.cpp | 77-127  | - | - | ?
    in    Space  const ColourSpaceSpecification&  [-]  ?
    out   -      TristimulusProjection            [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE TRANSFERS
//------------------------------------------------------------------------------------------------------------------------

F Encode                | ColourProjection.cpp | 135-153 | - | - | ?
    in    Space            const ColourSpaceSpecification&  [-]  ?
    in    LinearMagnitude  double                           [-]  ?
    out   -                double                           [-]  ?
    by    Api/ColourProjection.h, Source/ConsoleHost.cpp

F Decode                | ColourProjection.cpp | 155-170 | - | - | ?
    in    Space       const ColourSpaceSpecification&  [-]  ?
    in    StoredCode  double                           [-]  ?
    out   -           double                           [-]  ?
    by    Api/ColourProjection.h, Source/ConsoleHost.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHITE ADAPTATION
//------------------------------------------------------------------------------------------------------------------------

F AdaptWhite            | ColourProjection.cpp | 176-252 | - | - | ?
    in    ArrivingWhiteX  double   [-]  ?
    in    ArrivingWhiteY  double   [-]  ?
    in    TargetWhiteX    double   [-]  ?
    in    TargetWhiteY    double   [-]  ?
    in    TristimulusX    double&  [-]  ?
    in    TristimulusY    double&  [-]  ?
    in    TristimulusZ    double&  [-]  ?
    out   -               void     [-]  ?
    by    Api/ColourProjection.h

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PROJECTION
//------------------------------------------------------------------------------------------------------------------------

F Project               | ColourProjection.cpp | 258-318 | - | - | ?
    in    Arriving       ColourSpecification              [-]  ?
    in    ArrivingSpace  const ColourSpaceSpecification&  [-]  ?
    in    Target         const ColourSpaceSpecification&  [-]  ?
    out   -              Deliver<ColourSpecification>     [-]  ?
    by    Api/ColourProjection.h, Api/DisplayProjection.h, Api/QuadratureIntegrator.h, Api/SpectralProjection.h, Api/TickSequence.h, Api/TransformProjection.h, (+12 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                 TRISTIMULUS TO A SPACE
//------------------------------------------------------------------------------------------------------------------------

F ProjectTristimulus    | ColourProjection.cpp | 324-355 | - | - | ?
    in    TristimulusX  double                           [-]  ?
    in    TristimulusY  double                           [-]  ?
    in    TristimulusZ  double                           [-]  ?
    in    Target        const ColourSpaceSpecification&  [-]  ?
    out   -             Deliver<ColourSpecification>     [-]  ?
    by    Api/ColourProjection.h, Source/AtmosphereIntegrator.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                      TEMPERATURE
//------------------------------------------------------------------------------------------------------------------------

F ProjectTemperature    | ColourProjection.cpp | 361-427 | - | - | ?
    in    Temperature  double                           [-]  ?
    in    Target       const ColourSpaceSpecification&  [-]  ?
    out   -            Deliver<ColourSpecification>     [-]  ?
    by    Api/ColourProjection.h, Source/ConsoleHost.cpp, Source/IlluminantPopulation.cpp
