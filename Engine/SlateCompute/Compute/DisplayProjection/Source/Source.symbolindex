//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Exposure first, compression second, primaries third, transfer once — and the refusal that stops one monitor being every monitor.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/DisplayProjection/Source
%layer      SlateCompute
%sources    1
%symbols    11
%annotated  0/11
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S DisplayProjection.cpp | 233 lines | 9cc71399 | 11 sym | Exposure first, compression second, primaries third, transfer once — and the refusal that stops one monitor being every monitor.

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DECLARATION
//------------------------------------------------------------------------------------------------------------------------

V DisplayRecordingIdentity           | DisplayProjection.cpp | 20      | - | - | ?

V MiddleGreyLuminance                | DisplayProjection.cpp | 25      | - | - | ?

F DisplayProjection::Declare         | DisplayProjection.cpp | 29-73   | - | - | ?
    in    Exposing_  const ExposureSpecification&  [-]  ?
    in    Toning_    const ToneSpecification&      [-]  ?
    in    Encoding_  const EncodeSpecification&    [-]  ?
    out   -          Deliver<bool>                 [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RECORDING
//------------------------------------------------------------------------------------------------------------------------

F DisplayProjection::Contribute      | DisplayProjection.cpp | 79-99   | - | - | ?
    in    Schedule  RenderSchedule&  [-]  ?
    out   -         Deliver<bool>    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE METERING
//------------------------------------------------------------------------------------------------------------------------

F DisplayProjection::AdvanceMetering | DisplayProjection.cpp | 105-144 | - | - | ?
    in    ReducedLuminance  double         [-]  ?
    in    ElapsedSeconds    double         [-]  ?
    out   -                 Deliver<bool>  [-]  ?

F DisplayProjection::ExposureScale   | DisplayProjection.cpp | 146-153 | - | - | ?
    out   -  double  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PROJECTION
//------------------------------------------------------------------------------------------------------------------------

F DisplayProjection::Project         | DisplayProjection.cpp | 159-211 | - | - | ?
    in    Accumulated  const ColourSpecification&    [-]  ?
    out   -            Deliver<ColourSpecification>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE REPORTING
//------------------------------------------------------------------------------------------------------------------------

F DisplayProjection::Report          | DisplayProjection.cpp | 217-227 | - | - | ?
    in    Measured  MeasureIndex&  [-]  ?
    in    Sampled   TickPoint      [-]  ?
    out   -         void           [-]  ?

F DisplayProjection::Exposure        | DisplayProjection.cpp | 229     | - | - | ?
    out   -  const ExposureSpecification&  [-]  ?

F DisplayProjection::Tone            | DisplayProjection.cpp | 230     | - | - | ?
    out   -  const ToneSpecification&  [-]  ?

F DisplayProjection::Encoding        | DisplayProjection.cpp | 231     | - | - | ?
    out   -  const EncodeSpecification&  [-]  ?
