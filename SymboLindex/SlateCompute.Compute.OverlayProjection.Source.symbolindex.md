//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Two declarations that differ in exactly one behaviour, presence read straight out of `76`, and nothing that could ever be picked.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/OverlayProjection/Source
%layer      SlateCompute
%sources    1
%symbols    10
%annotated  0/10
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S OverlayProjection.cpp | 218 lines | 82bfee4a | 10 sym | Two declarations that differ in exactly one behaviour, presence read straight out of `76`, and nothing that could ever be picked.

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DECLARATION
//------------------------------------------------------------------------------------------------------------------------

V DepthTestedIdentity                  | OverlayProjection.cpp | 18      | - | - | ?

V DepthFreeIdentity                    | OverlayProjection.cpp | 19      | - | - | ?

V OverlayOrigin                        | OverlayProjection.cpp | 20      | - | - | ?

F OverlayDeclarable                    | OverlayProjection.cpp | 22-25   | - | - | ?
    in    Presented  OverlaySubject  [-]  ?
    out   -          constexpr bool  [-]  ?

F OverlayProjection::Declare           | OverlayProjection.cpp | 29-73   | - | - | ?
    in    Presented  OverlaySubject               [-]  ?
    in    Declaring  const OverlaySpecification&  [-]  ?
    out   -          Deliver<bool>                [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RECORDINGS
//------------------------------------------------------------------------------------------------------------------------

F OverlayProjection::Contribute        | OverlayProjection.cpp | 79-126  | - | - | ?
    in    Schedule  RenderSchedule&  [-]  ?
    out   -         Deliver<bool>    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE PRESENCE
//------------------------------------------------------------------------------------------------------------------------

F OverlayProjection::OverlayStanding   | OverlayProjection.cpp | 132-144 | - | - | ?
    in    Tooling    const ToolSequence&  [-]  ?
    in    Presented  OverlaySubject       [-]  ?
    out   -          bool                 [-]  ?

F OverlayProjection::RecordingOccupied | OverlayProjection.cpp | 146-161 | - | - | ?
    in    Tooling    const ToolSequence&  [-]  ?
    in    Behaviour  DepthSubject         [-]  ?
    out   -          bool                 [-]  ?

F OverlayProjection::Specification     | OverlayProjection.cpp | 163-180 | - | - | ?
    in    Presented  OverlaySubject                        [-]  ?
    out   -          Deliver<const OverlaySpecification*>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE REPORTING
//------------------------------------------------------------------------------------------------------------------------

F OverlayProjection::Report            | OverlayProjection.cpp | 186-216 | - | - | ?
    in    Tooling   const ToolSequence&  [-]  ?
    in    Measured  MeasureIndex&        [-]  ?
    in    Sampled   TickPoint            [-]  ?
    out   -         void                 [-]  ?
