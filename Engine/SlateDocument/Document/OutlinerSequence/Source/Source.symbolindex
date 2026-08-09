//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The seven steps in order, every mutation a transaction, and the retirement cascade as one of them.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/OutlinerSequence/Source
%layer      SlateDocument
%sources    1
%symbols    23
%annotated  0/23
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S OutlinerSequence.cpp | 534 lines | dcda05db | 23 sym | The seven steps in order, every mutation a transaction, and the retirement cascade as one of them.

//------------------------------------------------------------------------------------------------------------------------
//                                                       ENROLMENT
//------------------------------------------------------------------------------------------------------------------------

F OutlinerSequence::Enrol           | OutlinerSequence.cpp | 15-49   | - | - | ?
    in    DeclaredName  const std::string&         [-]  ?
    out   -             Outcome<OccupantIdentity>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    DECLARED INTENT
//------------------------------------------------------------------------------------------------------------------------

F OutlinerSequence::Declare         | OutlinerSequence.cpp | 55-65   | - | - | ?
    in    Arriving  const DeclaredIntent&  [-]  ?
    out   -         Outcome<bool>          [-]  ?

F OutlinerSequence::Reject          | OutlinerSequence.cpp | 67-74   | - | - | ?
    in    Refused    const DeclaredIntent&  [-]  ?
    in    Declining  const Refusal&         [-]  ?
    out   -          void                   [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     SUBSET INTENT
//------------------------------------------------------------------------------------------------------------------------

F OutlinerSequence::EnrolSelection  | OutlinerSequence.cpp | 82-95   | - | - | ?
    in    Standing  const std::vector<OccupantIdentity>&  [-]  ?
    out   -         Outcome<bool>                         [-]  ?

F OutlinerSequence::ApplySelection  | OutlinerSequence.cpp | 97-113  | - | - | ?
    in    Standing  const std::vector<OccupantIdentity>&  [-]  ?
    in    SealedAt  std::uint64_t                         [-]  ?
    out   -         Outcome<bool>                         [-]  ?

F OutlinerSequence::ApplySubset     | OutlinerSequence.cpp | 115-176 | - | - | ?
    in    Applying   const DeclaredIntent&  [-]  ?
    in    Addressed  SubsetSubject          [-]  ?
    in    SealedAt   std::uint64_t          [-]  ?
    out   -          Outcome<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    NARROWING INTENT
//------------------------------------------------------------------------------------------------------------------------

F OutlinerSequence::ApplyNarrowing  | OutlinerSequence.cpp | 182-194 | - | - | ?
    in    Applying  const DeclaredIntent&  [-]  ?
    out   -         Outcome<bool>          [-]  ?

F OutlinerSequence::DeriveNarrowing | OutlinerSequence.cpp | 196-204 | - | - | ?
    out   -  Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       SCRUBBING
//------------------------------------------------------------------------------------------------------------------------

F OutlinerSequence::Retreat         | OutlinerSequence.cpp | 210-231 | - | - | ?
    in    SealedAt  std::uint64_t  [-]  ?
    out   -         Outcome<bool>  [-]  ?

F OutlinerSequence::Advance         | OutlinerSequence.cpp | 233-251 | - | - | ?
    in    SealedAt  std::uint64_t  [-]  ?
    out   -         Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE RETIREMENT CASCADE
//------------------------------------------------------------------------------------------------------------------------

F OutlinerSequence::RetireCascade   | OutlinerSequence.cpp | 257-284 | - | - | ?
    in    Applying  const DeclaredIntent&  [-]  ?
    in    SealedAt  std::uint64_t          [-]  ?
    out   -         Outcome<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    APPLYING INTENT
//------------------------------------------------------------------------------------------------------------------------

F OutlinerSequence::ApplyIntent     | OutlinerSequence.cpp | 290-375 | - | - | ?
    in    Applying  const DeclaredIntent&  [-]  ?
    in    SealedAt  std::uint64_t          [-]  ?
    out   -         Outcome<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE TICK ORDER
//------------------------------------------------------------------------------------------------------------------------

F OutlinerSequence::Reconcile       | OutlinerSequence.cpp | 381-472 | - | - | ?
    in    SealedAt  std::uint64_t  [-]  ?
    out   -         Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

F OutlinerSequence::Sequenced       | OutlinerSequence.cpp | 478-481 | - | - | ?
    out   -  const RowSequence&  [-]  ?

F OutlinerSequence::Enrollments     | OutlinerSequence.cpp | 483-486 | - | - | ?
    out   -  const EnrollmentIndex&  [-]  ?

F OutlinerSequence::Names           | OutlinerSequence.cpp | 488-491 | - | - | ?
    out   -  const TrigramIndex&  [-]  ?

F OutlinerSequence::Relations       | OutlinerSequence.cpp | 493-496 | - | - | ?
    out   -  const SceneStructure&  [-]  ?

F OutlinerSequence::Revisions       | OutlinerSequence.cpp | 498-501 | - | - | ?
    out   -  const RevisionSequence&  [-]  ?

F OutlinerSequence::Selections      | OutlinerSequence.cpp | 503-506 | - | - | ?
    out   -  const SelectionSequence&  [-]  ?

F OutlinerSequence::Sought          | OutlinerSequence.cpp | 508-511 | - | - | ?
    out   -  const std::string&  [-]  ?

F OutlinerSequence::Rejected        | OutlinerSequence.cpp | 513-516 | - | - | ?
    out   -  const std::vector<RejectedIntent>&  [-]  ?

F OutlinerSequence::ReclaimRejected | OutlinerSequence.cpp | 518-521 | - | - | ?
    out   -  void  [-]  ?

F OutlinerSequence::InvariantsHeld  | OutlinerSequence.cpp | 523-532 | - | - | ?
    out   -  bool  [-]  ?
