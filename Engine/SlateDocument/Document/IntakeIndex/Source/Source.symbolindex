//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Arrival-ordered records, and the once-only report of every assumption among them.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/IntakeIndex/Source
%layer      SlateDocument
%sources    1
%symbols    6
%annotated  0/6
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S IntakeIndex.cpp | 81 lines | 2653acfc | 6 sym | Arrival-ordered records, and the once-only report of every assumption among them.

//------------------------------------------------------------------------------------------------------------------------
//                                                       RECORDING
//------------------------------------------------------------------------------------------------------------------------

F IntakeIndex::Record          | IntakeIndex.cpp | 15-22 | - | - | ?
    in    Arriving  const IntakeRecord&  [-]  ?
    out   -         void                 [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE REPORT
//------------------------------------------------------------------------------------------------------------------------

F IntakeIndex::Report          | IntakeIndex.cpp | 28-59 | - | - | ?
    in    Reporting  ReportSequence&  [-]  ?
    in    Sampled    TickPoint        [-]  ?
    out   -          void             [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

F IntakeIndex::Records         | IntakeIndex.cpp | 65    | - | - | ?
    out   -  const std::vector<IntakeRecord>&  [-]  ?

F IntakeIndex::Resolve         | IntakeIndex.cpp | 67-76 | - | - | ?
    in    OriginPath  const std::string&     [-]  ?
    out   -           Outcome<IntakeRecord>  [-]  ?

F IntakeIndex::AssumptionCount | IntakeIndex.cpp | 78    | - | - | ?
    out   -  std::uint32_t  [-]  ?

F IntakeIndex::RecordedCount   | IntakeIndex.cpp | 79    | - | - | ?
    out   -  std::uint32_t  [-]  ?
