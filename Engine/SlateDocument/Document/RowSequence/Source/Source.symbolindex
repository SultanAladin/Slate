//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The depth-first walk, and the binary-indexed counts that answer both scroll questions.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/RowSequence/Source
%layer      SlateDocument
%sources    1
%symbols    17
%annotated  0/17
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S RowSequence.cpp | 396 lines | ab09838a | 17 sym | The depth-first walk, and the binary-indexed counts that answer both scroll questions.

//------------------------------------------------------------------------------------------------------------------------
//                                                    COUNTED ORDERING
//------------------------------------------------------------------------------------------------------------------------

F RankIndex::Construct           | RowSequence.cpp | 15-34   | - | - | ?
    in    RowCount  std::uint32_t  [-]  ?
    out   -         void           [-]  ?

F RankIndex::Declare             | RowSequence.cpp | 36-62   | - | - | ?
    in    RowOrdinal      std::uint32_t  [-]  ?
    in    CountedEnabled  bool           [-]  ?
    out   -               void           [-]  ?

F RankIndex::CountedBefore       | RowSequence.cpp | 64-76   | - | - | ?
    in    RowOrdinal  std::uint32_t  [-]  ?
    out   -           std::uint32_t  [-]  ?

F RankIndex::RowAtVisible        | RowSequence.cpp | 78-107  | - | - | ?
    in    VisibleOrdinal  std::uint32_t           [-]  ?
    out   -               Deliver<std::uint32_t>  [-]  ?

F RankIndex::VisibleOfRow        | RowSequence.cpp | 109-121 | - | - | ?
    in    RowOrdinal  std::uint32_t           [-]  ?
    out   -           Deliver<std::uint32_t>  [-]  ?

F RankIndex::CountedTotal        | RowSequence.cpp | 123-126 | - | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     LINEARISATION
//------------------------------------------------------------------------------------------------------------------------

F AppendReversed                 | RowSequence.cpp | 137-159 | - | - | ?
    in    Pending       std::vector<std::uint32_t>&  [-]  ?
    in    Relations     const SceneStructure&        [-]  ?
    in    FirstInOrder  std::uint32_t                [-]  ?
    out   -             void                         [-]  ?

F RowSequence::Linearize         | RowSequence.cpp | 163-233 | - | - | ?
    in    Relations  const SceneStructure&  [-]  ?
    out   -          Deliver<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                EXPANSION AND NARROWING
//------------------------------------------------------------------------------------------------------------------------

V NothingCollapsed               | RowSequence.cpp | 244     | - | - | ?

F RowSequence::Recount           | RowSequence.cpp | 248-276 | - | - | ?
    out   -  void  [-]  ?

F RowSequence::DeclareExpansion  | RowSequence.cpp | 278-293 | - | - | ?
    in    Subject           OccupantIdentity  [-]  ?
    in    ExpansionEnabled  bool              [-]  ?
    out   -                 Deliver<bool>     [-]  ?

F RowSequence::DeclareNarrowing  | RowSequence.cpp | 295-331 | - | - | ?
    in    Retained           const std::vector<OccupantIdentity>&  [-]  ?
    in    NarrowingDeclared  bool                                  [-]  ?
    out   -                  Deliver<bool>                         [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

F RowSequence::Rows              | RowSequence.cpp | 337-340 | - | - | ?
    out   -  const std::vector<SequencedRow>&  [-]  ?

F RowSequence::Counted           | RowSequence.cpp | 342-345 | - | - | ?
    out   -  const RankIndex&  [-]  ?

F RowSequence::RowOf             | RowSequence.cpp | 347-358 | - | - | ?
    in    Subject  OccupantIdentity        [-]  ?
    out   -        Deliver<std::uint32_t>  [-]  ?

F RowSequence::NarrowingStanding | RowSequence.cpp | 360-363 | - | - | ?
    out   -  bool  [-]  ?

F RowSequence::CountsAgree       | RowSequence.cpp | 365-394 | - | - | ?
    out   -  bool  [-]  ?
