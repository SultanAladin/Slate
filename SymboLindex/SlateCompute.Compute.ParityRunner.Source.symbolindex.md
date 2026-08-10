//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Registration and comparison over the common sample set.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/ParityRunner/Source
%layer      SlateCompute
%sources    1
%symbols    7
%annotated  0/7
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ParityRunner.cpp | 475 lines | 00cc7064 | 7 sym | Registration and comparison over the common sample set.

//------------------------------------------------------------------------------------------------------------------------
//                                                      REGISTRATION
//------------------------------------------------------------------------------------------------------------------------

F ParityRunner::Register      | ParityRunner.cpp | 25-35   | - | - | ?
    in    Arriving  const ParityRegistration&  [-]  ?
    out   -         Outcome<bool>              [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE COMMON SAMPLE SET
//------------------------------------------------------------------------------------------------------------------------

T OrientationSample           | ParityRunner.cpp | 46-54   | - | - | ?
    has   AlphaX  double  [-]  ?
    has   AlphaY  double  [-]  ?
    has   BetaX   double  [-]  ?
    has   BetaY   double  [-]  ?
    has   GammaX  double  [-]  ?
    has   GammaY  double  [-]  ?

T IncircleSample              | ParityRunner.cpp | 71-81   | - | - | ?
    has   AlphaX  double  [-]  ?
    has   AlphaY  double  [-]  ?
    has   BetaX   double  [-]  ?
    has   BetaY   double  [-]  ?
    has   GammaX  double  [-]  ?
    has   GammaY  double  [-]  ?
    has   DeltaX  double  [-]  ?
    has   DeltaY  double  [-]  ?

T SegmentSample               | ParityRunner.cpp | 94-104  | - | - | ?
    has   AlphaX  double  [-]  ?
    has   AlphaY  double  [-]  ?
    has   BetaX   double  [-]  ?
    has   BetaY   double  [-]  ?
    has   GammaX  double  [-]  ?
    has   GammaY  double  [-]  ?
    has   DeltaX  double  [-]  ?
    has   DeltaY  double  [-]  ?

T IntervalSample              | ParityRunner.cpp | 118-124 | - | - | ?
    has   OuterBegin  std::uint64_t  [-]  ?
    has   OuterEnd    std::uint64_t  [-]  ?
    has   InnerBegin  std::uint64_t  [-]  ?
    has   InnerEnd    std::uint64_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       COMPARISON
//------------------------------------------------------------------------------------------------------------------------

F ParityRunner::Compare       | ParityRunner.cpp | 142-468 | - | - | ?
    out   -  const std::vector<ParityReport>&  [-]  ?

F ParityRunner::AgreementHeld | ParityRunner.cpp | 470-473 | - | - | ?
    out   -  bool  [-]  ?
