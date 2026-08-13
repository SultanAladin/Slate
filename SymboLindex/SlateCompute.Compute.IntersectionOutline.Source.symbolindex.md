//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Two indexed lookups, one interval comparison, a scalar coverage — and the refusal that stops an occluded outline being merely dimmer.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/IntersectionOutline/Source
%layer      SlateCompute
%sources    1
%symbols    11
%annotated  0/11
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S IntersectionOutline.cpp | 200 lines | 5ed021be | 11 sym | Two indexed lookups, one interval comparison, a scalar coverage — and the refusal that stops an occluded outline being merely dimmer.

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DECLARATION
//------------------------------------------------------------------------------------------------------------------------

V OutlineRecordingIdentity               | IntersectionOutline.cpp | 20      | - | - | ?

V DistinctColourDeparture                | IntersectionOutline.cpp | 24      | - | - | ?

F IntersectionOutline::Declare           | IntersectionOutline.cpp | 28-75   | - | - | ?
    in    Outlining_  const OutlineSpecification&  [-]  ?
    out   -           Outcome<bool>                [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RECORDING
//------------------------------------------------------------------------------------------------------------------------

F IntersectionOutline::Contribute        | IntersectionOutline.cpp | 81-109  | - | - | ?
    in    Schedule  RenderSchedule&  [-]  ?
    out   -         Outcome<bool>    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE ENROLMENT
//------------------------------------------------------------------------------------------------------------------------

F IntersectionOutline::ClassifyEnrolment | IntersectionOutline.cpp | 115-134 | - | - | ?
    in    Written      VisibilityWord                   [-]  ?
    in    Visibility   const VisibilityIndex&           [-]  ?
    in    Resolutions  const PartitionResolutionIndex&  [-]  ?
    in    Enrollments  const EnrollmentIndex&           [-]  ?
    out   -            Outcome<bool>                    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE COVERAGE
//------------------------------------------------------------------------------------------------------------------------

F IntersectionOutline::ProjectCoverage   | IntersectionOutline.cpp | 140-151 | - | - | ?
    in    DivergenceExtent  double  [-]  ?
    out   -                 double  [-]  ?

F IntersectionOutline::ClassifyOcclusion | IntersectionOutline.cpp | 153-156 | - | - | ?
    in    OutlineDepth   double  [-]  ?
    in    RecordedDepth  double  [-]  ?
    out   -              bool    [-]  ?

F IntersectionOutline::DashStanding      | IntersectionOutline.cpp | 158-169 | - | - | ?
    in    AlongOrdinate   double  [-]  ?
    in    AcrossOrdinate  double  [-]  ?
    out   -               bool    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE COLOUR
//------------------------------------------------------------------------------------------------------------------------

F IntersectionOutline::OutlineColour     | IntersectionOutline.cpp | 175-186 | - | - | ?
    in    Occluded  bool                          [-]  ?
    out   -         Outcome<ColourSpecification>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE REPORTING
//------------------------------------------------------------------------------------------------------------------------

F IntersectionOutline::Report            | IntersectionOutline.cpp | 192-196 | - | - | ?
    in    Measured  MeasureIndex&  [-]  ?
    in    Sampled   TickPoint      [-]  ?
    out   -         void           [-]  ?

F IntersectionOutline::Outline           | IntersectionOutline.cpp | 198     | - | - | ?
    out   -  const OutlineSpecification&  [-]  ?
