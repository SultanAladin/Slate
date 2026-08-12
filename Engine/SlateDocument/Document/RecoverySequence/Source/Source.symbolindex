//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 `48` §4 — the per-document journal appended as transactions seal, and the replay that is offered and never applied.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/RecoverySequence/Source
%layer      SlateDocument
%sources    1
%symbols    11
%annotated  0/11
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S RecoverySequence.cpp | 192 lines | a2974247 | 11 sym | `48` §4 — the per-document journal appended as transactions seal, and the replay that is offered and never applied.

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DECLARATION
//------------------------------------------------------------------------------------------------------------------------

F RecoverySequence::DeclareDocument   | RecoverySequence.cpp | 17-29   | - | - | ?
    in    DeclaredDocument  const std::string&  [-]  ?
    in    DeclaredJournal   const std::string&  [-]  ?
    out   -                 Outcome<bool>       [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE APPENDING
//------------------------------------------------------------------------------------------------------------------------

F RecoverySequence::Append            | RecoverySequence.cpp | 35-61   | - | - | ?
    in    Sealing          const CommittedTransaction&  [-]  ?
    in    RevisionOrdinal  std::uint64_t                [-]  ?
    out   -                Outcome<bool>                [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RETIREMENT
//------------------------------------------------------------------------------------------------------------------------

F RecoverySequence::Retire            | RecoverySequence.cpp | 67-88   | - | - | ?
    in    SavedThrough  std::uint64_t  [-]  ?
    out   -             void           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE OFFER
//------------------------------------------------------------------------------------------------------------------------

F RecoverySequence::OfferReplay       | RecoverySequence.cpp | 94-136  | - | - | ?
    in    SavedAt       std::uint64_t  [-]  ?
    in    SavedThrough  std::uint64_t  [-]  ?
    out   -             RecoveryOffer  [-]  ?

F RecoverySequence::DeclareUnreadable | RecoverySequence.cpp | 138-144 | - | - | ?
    in    EntryOrdinal  std::uint32_t  [-]  ?
    out   -             void           [-]  ?

F RecoverySequence::Offered           | RecoverySequence.cpp | 146-159 | - | - | ?
    in    SavedThrough  std::uint64_t              [-]  ?
    out   -             std::vector<JournalEntry>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE READINGS
//------------------------------------------------------------------------------------------------------------------------

F RecoverySequence::Retained          | RecoverySequence.cpp | 165-168 | - | - | ?
    out   -  const std::vector<JournalEntry>&  [-]  ?

F RecoverySequence::DocumentOrigin    | RecoverySequence.cpp | 170-173 | - | - | ?
    out   -  const std::string&  [-]  ?

F RecoverySequence::JournalOrigin     | RecoverySequence.cpp | 175-178 | - | - | ?
    out   -  const std::string&  [-]  ?

F RecoverySequence::DiscardedCount    | RecoverySequence.cpp | 180-183 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F RecoverySequence::Reclaim           | RecoverySequence.cpp | 185-190 | - | - | ?
    out   -  void  [-]  ?
