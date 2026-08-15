//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Coalescing, the bounded cyclic retention, and the overwriting measure index beside it.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Numeric/ReportSequence/Source
%layer      SlateMath
%sources    1
%symbols    14
%annotated  0/14
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ReportSequence.cpp | 223 lines | 9d223dea | 14 sym | Coalescing, the bounded cyclic retention, and the overwriting measure index beside it.

//------------------------------------------------------------------------------------------------------------------------
//                                                       COALESCING
//------------------------------------------------------------------------------------------------------------------------

F TextAgrees                     | ReportSequence.cpp | 22-31   | - | - | ?
    in    Left   const char*  [-]  ?
    in    Right  const char*  [-]  ?
    out   -      bool         [-]  ?

F Coalesces                      | ReportSequence.cpp | 36-45   | - | - | ?
    in    Held      const ReportSpecification&  [-]  ?
    in    Arriving  const ReportSpecification&  [-]  ?
    out   -         bool                        [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       APPENDING
//------------------------------------------------------------------------------------------------------------------------

F ReportSequence::Append         | ReportSequence.cpp | 53-94   | - | - | ?
    in    Arriving  const ReportSpecification&  [-]  ?
    out   -         void                        [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

F ReportSequence::Retained       | ReportSequence.cpp | 100-111 | - | - | ?
    out   -  std::vector<ReportSpecification>  [-]  ?

F ReportSequence::RetainedCount  | ReportSequence.cpp | 113-117 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F ReportSequence::AppendedCount  | ReportSequence.cpp | 119-123 | - | - | ?
    out   -  std::uint64_t  [-]  ?

F ReportSequence::DiscardedCount | ReportSequence.cpp | 125-129 | - | - | ?
    out   -  std::uint64_t  [-]  ?

F ReportSequence::Reclaim        | ReportSequence.cpp | 131-140 | - | - | ?
    out   -  void  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE MEASURES
//------------------------------------------------------------------------------------------------------------------------

F MeasureIndex::Located          | ReportSequence.cpp | 146-158 | - | - | ?
    in    Origin    const char*  [-]  ?
    in    Measured  const char*  [-]  ?
    out   -         std::size_t  [-]  ?

F MeasureIndex::DeclareCount     | ReportSequence.cpp | 160-178 | - | - | ?
    in    Origin    const char*    [-]  ?
    in    Measured  const char*    [-]  ?
    in    Counted   std::uint64_t  [-]  ?
    in    Sampled   TickPoint      [-]  ?
    out   -         void           [-]  ?

F MeasureIndex::DeclareMagnitude | ReportSequence.cpp | 180-198 | - | - | ?
    in    Origin     const char*  [-]  ?
    in    Measured   const char*  [-]  ?
    in    Magnitude  double       [-]  ?
    in    Sampled    TickPoint    [-]  ?
    out   -          void         [-]  ?

F MeasureIndex::Measures         | ReportSequence.cpp | 200-203 | - | - | ?
    out   -  const std::vector<SampledMeasure>&  [-]  ?

F MeasureIndex::Resolve          | ReportSequence.cpp | 205-216 | - | - | ?
    in    Origin    const char*              [-]  ?
    in    Measured  const char*              [-]  ?
    out   -         Deliver<SampledMeasure>  [-]  ?

F MeasureIndex::Reclaim          | ReportSequence.cpp | 218-221 | - | - | ?
    out   -  void  [-]  ?
