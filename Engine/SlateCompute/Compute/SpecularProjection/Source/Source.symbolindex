//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The half-extent claim, the amending recording ordered after `62`, and the composite that cancels to nothing on every failure.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/SpecularProjection/Source
%layer      SlateCompute
%sources    1
%symbols    9
%annotated  0/9
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S SpecularProjection.cpp | 146 lines | 875a15ca | 9 sym | The half-extent claim, the amending recording ordered after `62`, and the composite that cancels to nothing on every failure.

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DECLARATION
//------------------------------------------------------------------------------------------------------------------------

V ReflectionRecordingIdentity         | SpecularProjection.cpp | 18      | - | - | ?

F SpecularProjection::Declare         | SpecularProjection.cpp | 22-50   | - | - | ?
    in    Declaring  const ReflectionSpecification&  [-]  ?
    out   -          Deliver<bool>                   [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RECORDING
//------------------------------------------------------------------------------------------------------------------------

F SpecularProjection::Contribute      | SpecularProjection.cpp | 56-81   | - | - | ?
    in    Schedule  RenderSchedule&  [-]  ?
    out   -         Deliver<bool>    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE EXTENT
//------------------------------------------------------------------------------------------------------------------------

F SpecularProjection::Resolve         | SpecularProjection.cpp | 87-99   | - | - | ?
    in    DisplayAlong    std::uint32_t   [-]  ?
    in    DisplayAcross   std::uint32_t   [-]  ?
    in    ResolvedAlong   std::uint32_t&  [-]  ?
    in    ResolvedAcross  std::uint32_t&  [-]  ?
    out   -               Deliver<bool>   [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE COMPOSITE
//------------------------------------------------------------------------------------------------------------------------

F SpecularProjection::Compose         | SpecularProjection.cpp | 105-117 | - | - | ?
    in    Standing  const double             [-]  ?
    in    PreAdded  const double             [-]  ?
    in    Traced    const TracedReflection&  [-]  ?
    in    Resolved  double                   [-]  ?
    out   -         void                     [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE REPORTING
//------------------------------------------------------------------------------------------------------------------------

F SpecularProjection::DeclareRotation | SpecularProjection.cpp | 123-130 | - | - | ?
    in    TracedCount    std::uint32_t  [-]  ?
    in    ResolvedCount  std::uint32_t  [-]  ?
    in    SkippedCount   std::uint32_t  [-]  ?
    out   -              void           [-]  ?

F SpecularProjection::Report          | SpecularProjection.cpp | 132-141 | - | - | ?
    in    Measured  MeasureIndex&  [-]  ?
    in    Sampled   TickPoint      [-]  ?
    out   -         void           [-]  ?

F SpecularProjection::Declared        | SpecularProjection.cpp | 143     | - | - | ?
    out   -  const ReflectionSpecification&  [-]  ?

F SpecularProjection::Metrics         | SpecularProjection.cpp | 144     | - | - | ?
    out   -  const ReflectionMetrics&  [-]  ?
