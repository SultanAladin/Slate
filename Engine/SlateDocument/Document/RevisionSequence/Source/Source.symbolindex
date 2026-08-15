//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The drag lifecycle, declared merging, and scrubbing in both directions.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/RevisionSequence/Source
%layer      SlateDocument
%sources    1
%symbols    8
%annotated  0/8
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S RevisionSequence.cpp | 119 lines | 8c6dba74 | 8 sym | The drag lifecycle, declared merging, and scrubbing in both directions.

//------------------------------------------------------------------------------------------------------------------------
//                                                        OPENING
//------------------------------------------------------------------------------------------------------------------------

F RevisionSequence::Open            | RevisionSequence.cpp | 15-26   | - | - | ?
    in    Description    const std::string&  [-]  ?
    in    OperationName  const std::string&  [-]  ?
    out   -              Deliver<bool>       [-]  ?

F RevisionSequence::Abandon         | RevisionSequence.cpp | 28-32   | - | - | ?
    out   -  void  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        SEALING
//------------------------------------------------------------------------------------------------------------------------

F RevisionSequence::Seal            | RevisionSequence.cpp | 38-76   | - | - | ?
    in    SealedAt       std::uint64_t  [-]  ?
    in    MergeDeclared  bool           [-]  ?
    out   -              Deliver<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       SCRUBBING
//------------------------------------------------------------------------------------------------------------------------

F RevisionSequence::Retreat         | RevisionSequence.cpp | 82-89   | - | - | ?
    out   -  Deliver<bool>  [-]  ?

F RevisionSequence::Advance         | RevisionSequence.cpp | 91-98   | - | - | ?
    out   -  Deliver<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

F RevisionSequence::Committed       | RevisionSequence.cpp | 104-107 | - | - | ?
    out   -  const std::vector<CommittedTransaction>&  [-]  ?

F RevisionSequence::ScrubPosition   | RevisionSequence.cpp | 109-112 | - | - | ?
    out   -  std::uint64_t  [-]  ?

F RevisionSequence::TransactionOpen | RevisionSequence.cpp | 114-117 | - | - | ?
    out   -  bool  [-]  ?
