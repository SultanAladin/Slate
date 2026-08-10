//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The reserved interactive worker, cooperative cancellation, and conclusions ordered by declaration.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Numeric/WorkSequence/Source
%layer      SlateMath
%sources    1
%symbols    23
%annotated  0/23
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S WorkSequence.cpp | 533 lines | 29b66aab | 23 sym | The reserved interactive worker, cooperative cancellation, and conclusions ordered by declaration.

//------------------------------------------------------------------------------------------------------------------------
//                                                       ONE RECORD
//------------------------------------------------------------------------------------------------------------------------

T WorkSequence                  | WorkSequence.cpp | 19-29   | -          | - | ?
    has   Declared           WorkDeclaration    [-]  ?
    has   Progressed         WorkProgress       [-]  ?
    has   WithdrawalPosed    std::atomic<bool>  [-]  ?
    has   DeclaredOrdinal    std::uint64_t      [-]  ?
    has   SlotGeneration     std::uint32_t      [-]  ?
    has   ResolutionOpen     bool               [-]  ?
    has   SupersessionPosed  bool               [-]  ?
    has   Occupied           bool               [-]  ?
    by    Api/WorkSequence.h, Source/ConsoleHost.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                       ONE QUEUE
//------------------------------------------------------------------------------------------------------------------------

F WorkQueue::Admit              | WorkSequence.cpp | 35-39   | -          | - | ?
    in    RecordOrdinal  std::uint32_t  [-]  ?
    out   -              void           [-]  ?

F WorkQueue::Claim              | WorkSequence.cpp | 41-68   | -          | - | ?
    out   -  Outcome<std::uint32_t>  [-]  ?

F WorkQueue::Withdraw           | WorkSequence.cpp | 70-84   | -          | - | ?
    in    RecordOrdinal  std::uint32_t  [-]  ?
    out   -              void           [-]  ?

F WorkQueue::PendingCount       | WorkSequence.cpp | 86-89   | -          | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F WorkSequence::Construct       | WorkSequence.cpp | 100-135 | -          | - | ?
    in    RequestedWorkers  std::uint32_t        [-]  ?
    in    HostTimeline      const TickSequence&  [-]  ?
    in    ReportingInto     ReportSequence&      [-]  ?
    out   -                 Outcome<bool>        [-]  ?

F WorkSequence::~WorkSequence   | WorkSequence.cpp | 137-140 | destructor | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE WORKERS
//------------------------------------------------------------------------------------------------------------------------

F WorkSequence::Claimable       | WorkSequence.cpp | 146-163 | -          | - | ?
    in    WorkerOrdinal  std::uint32_t  [-]  ?
    out   -              bool           [-]  ?

F WorkSequence::Claim           | WorkSequence.cpp | 165-181 | -          | - | ?
    in    WorkerOrdinal  std::uint32_t  [-]  ?
    out   -              std::uint32_t  [-]  ?

F WorkSequence::Serve           | WorkSequence.cpp | 183-229 | -          | - | ?
    in    WorkerOrdinal  std::uint32_t  [-]  ?
    out   -              void           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      DECLARATION
//------------------------------------------------------------------------------------------------------------------------

F WorkSequence::Declare         | WorkSequence.cpp | 235-286 | -          | - | ?
    in    Arriving  const WorkDeclaration&  [-]  ?
    out   -         Outcome<WorkIdentity>   [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      CANCELLATION
//------------------------------------------------------------------------------------------------------------------------

F WorkSequence::Resolved        | WorkSequence.cpp | 292-303 | -          | - | ?
    in    Subject  WorkIdentity   [-]  ?
    out   -        std::uint32_t  [-]  ?

F WorkSequence::Cancel          | WorkSequence.cpp | 305-329 | -          | - | ?
    in    Subject            WorkIdentity   [-]  ?
    in    SupersessionPosed  bool           [-]  ?
    out   -                  Outcome<bool>  [-]  ?

F WorkSequence::Withdraw        | WorkSequence.cpp | 331-334 | -          | - | ?
    in    Subject  WorkIdentity   [-]  ?
    out   -        Outcome<bool>  [-]  ?

F WorkSequence::Supersede       | WorkSequence.cpp | 336-339 | -          | - | ?
    in    Subject  WorkIdentity   [-]  ?
    out   -        Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       CONCLUSION
//------------------------------------------------------------------------------------------------------------------------

F WorkSequence::Seal            | WorkSequence.cpp | 345-397 | -          | - | ?
    in    RecordOrdinal  std::uint32_t         [-]  ?
    in    Resolved_      const Outcome<bool>&  [-]  ?
    out   -              void                  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        DRAINING
//------------------------------------------------------------------------------------------------------------------------

F WorkSequence::Drain           | WorkSequence.cpp | 403-421 | -          | - | ?
    out   -  const std::vector<WorkCompletion>&  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

F WorkSequence::Progress        | WorkSequence.cpp | 427-437 | -          | - | ?
    in    Subject  WorkIdentity     [-]  ?
    out   -        Outcome<double>  [-]  ?

F WorkSequence::ProgressCount   | WorkSequence.cpp | 439-452 | -          | - | ?
    in    Subject  WorkIdentity            [-]  ?
    out   -        Outcome<std::uint64_t>  [-]  ?

F WorkSequence::WorkerCount     | WorkSequence.cpp | 454-458 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F WorkSequence::OccupiedWorkers | WorkSequence.cpp | 460-464 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F WorkSequence::PendingCount    | WorkSequence.cpp | 466-476 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F WorkSequence::Reclaim         | WorkSequence.cpp | 482-531 | -          | - | ?
    out   -  void  [-]  ?
