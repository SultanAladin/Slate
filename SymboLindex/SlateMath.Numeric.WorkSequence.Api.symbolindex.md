//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The only thread creation in the repository — declared work, immutable inputs, results applied on the tick.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Numeric/WorkSequence/Api
%layer      SlateMath
%sources    1
%symbols    39
%annotated  30/39
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S WorkSequence.h | 368 lines | befd4e22 | 39 sym | The only thread creation in the repository — declared work, immutable inputs, results applied on the tick.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

V AbsentWork                           | WorkSequence.h | 27      | -                             | -  | ?
    by    Source/WorkSequence.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                        PRIORITY
//------------------------------------------------------------------------------------------------------------------------

E WorkPriority                         | WorkSequence.h | 37-43   | contract                      | -  | How urgently declared work is wanted, and therefore what it may starve. whole-document export are both long solves, and `34` §4 forbids the export occupying every worker.
    has   Interactive    WorkPriority  [-]  ?
    has   Background     WorkPriority  [-]  ?
    has   Deferred       WorkPriority  [-]  ?
    has   PriorityCount  WorkPriority  [-]  ?
    by    Source/ConsoleHost.cpp, Source/WorkSequence.cpp
    note  🔴 At least one worker is reserved for Interactive. A residency promotion under the cursor and a

E WorkConclusion                       | WorkSequence.h | 49-55   | contract                      | -  | How one declaration ended. superseded cancellation ordinary operation and a withdrawn one the requester's own decision.
    has   Delivered   WorkConclusion  [-]  ?
    has   Withdrawn   WorkConclusion  [-]  ?
    has   Superseded  WorkConclusion  [-]  ?
    has   Refused     WorkConclusion  [-]  ?
    by    Source/ConsoleHost.cpp, Source/WorkSequence.cpp
    note  ⚠️ Withdrawn and Superseded are both cancellations and are reported apart, because `86` §5 rules a

//------------------------------------------------------------------------------------------------------------------------
//                                                      CANCELLATION
//------------------------------------------------------------------------------------------------------------------------

T WorkCancellation                     | WorkSequence.h | 66-77   | nonallocating,nonthrowing     | -  | What a resolution reads at each of its declared cancellation points. cancelled declaration still runs to its next declared point and releases what it holds — a worker that is simply never joined leaks its inputs, proportional to how often the artist changes their mind.
    has   WithdrawalSlot  const std::atomic<bool>*  [-]  ?
    by    Api/ChartPartition.h, Source/ChartPartition.cpp, Source/ConsoleHost.cpp, Source/WorkSequence.cpp
    note  🔴 Cancellation is cooperative and observed only where the declaration says it is. `34` §5: a

F WorkCancellation::WithdrawalDeclared | WorkSequence.h | 73-76   | api,nonallocating,nonthrowing | ✔️ | Whether the requester has withdrawn this declaration.
    out   -  bool  [-]  ?
    by    Source/ChartPartition.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                        PROGRESS
//------------------------------------------------------------------------------------------------------------------------

T WorkProgress                         | WorkSequence.h | 89-151  | owning                        | -  | What a long solve reports while it runs. own rate would contend with the tick for the very state the tick is presenting. and read once per tick, so the contention it could suffer never arises.
    has   ResolvedFraction  std::atomic<double>         [-]  ?
    has   ResolvedParts     std::atomic<std::uint64_t>  [-]  ?
    has   SpannedParts      std::atomic<std::uint64_t>  [-]  ?
    by    Api/ChartPartition.h, Source/ChartPartition.cpp, Source/ConsoleHost.cpp, Source/WorkSequence.cpp
    note  🔴 Written by the resolution and **sampled** by the tick — `34` §7. A solve that pushed progress at its
    note  📝 The real reading is atomic and may not be lock-free on every host. It is written at declared points

F WorkProgress::DeclareFraction        | WorkSequence.h | 101-105 | api,nonallocating,nonthrowing | ✔️ | Declares the resolved fraction, clamped to the closed unit interval.
    in    Resolved  double  [-]  zero at the beginning, one at the end
    out   -         void    [-]  ?

F WorkProgress::DeclareCount           | WorkSequence.h | 112-119 | api,nonallocating,nonthrowing | ✔️ | Declares the resolved count out of the spanned count, and the fraction they imply.
    in    Resolved  std::uint64_t  [-]  parts completed
    in    Spanned   std::uint64_t  [-]  parts the solve holds; zero declares the span unknown
    out   -         void           [-]  ?
    by    Api/ReportSequence.h, Source/ChartPartition.cpp, Source/ConsoleHost.cpp, Source/DisplayProjection.cpp, Source/OcclusionProjection.cpp, Source/OverlayProjection.cpp, (+6 more)

F WorkProgress::Fraction               | WorkSequence.h | 124     | api,nonallocating,nonthrowing | ✔️ | The resolved fraction as last declared.
    out   -  double  [-]  ?
    by    Shader/MultiScatterSurface.slang, Source/AtmosphereIntegrator.cpp, Source/BrushSpecification.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlLayout.cpp, (+7 more)

F WorkProgress::ResolvedCount          | WorkSequence.h | 129     | api,nonallocating,nonthrowing | ✔️ | The resolved count as last declared.
    out   -  std::uint64_t  [-]  ?
    by    Api/ImpressionSequence.h, Api/ShaderCodec.h, Api/SpecularProjection.h, Api/SurfaceDepot.h, Api/UvSurfaceDepot.h, Source/ConsoleHost.cpp, (+6 more)

F WorkProgress::SpannedCount           | WorkSequence.h | 134     | api,nonallocating,nonthrowing | ✔️ | The spanned count as last declared; zero declares the span unknown.
    out   -  std::uint64_t  [-]  ?
    by    Api/DocumentSession.h, Api/IlluminantPopulation.h, Api/OcclusionProjection.h, Api/PopulationIndex.h, Api/PrimitiveStructure.h, Api/SceneStructure.h, (+10 more)

F WorkProgress::Reclaim                | WorkSequence.h | 139-144 | api,nonallocating,nonthrowing | ✔️ | Returns every reading to its beginning, for a reused record.
    out   -  void  [-]  ?
    by    Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CodeInterchange.h, Api/CommandSequence.h, Api/CycleScheduler.h, Api/DepthReduction.h, (+75 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                     DECLARED WORK
//------------------------------------------------------------------------------------------------------------------------

T WorkDeclaration                      | WorkSequence.h | 166-174 | owning                        | -  | One declaration of work to be resolved off the tick. document, the tick's state, or anything in `76`. The requester captures what the work needs at declaration and hands it over, which is the rule that makes every lock here unnecessary. it on the tick after `Drain` delivers it — `34` §3. bounded worker count, waiting is a deadlock that appears only under load, on someone else's machine.
    has   Origin            const char*   [-]  ?
    has   Priority          WorkPriority  [-]  ?
    has   ProgressReported  bool          [-]  ?
    by    Source/ConsoleHost.cpp, Source/WorkSequence.cpp
    note  🔴 `34` §2: the resolution reads inputs that are **immutable for its whole run**. It may not read the
    note  🔴 The resolution mutates nothing and commits nothing. It returns an Deliver, and the requester applies
    note  ⚠️ There is deliberately no field naming another declaration. `34` §4 forbids waiting on one: with a

T WorkCompletion                       | WorkSequence.h | 178-186 | nonallocating,nonthrowing     | -  | One concluded declaration, crossing back to the tick.
    has   Declared         WorkIdentity    [-]  ?
    has   Origin           const char*     [-]  ?
    has   Concluded        WorkConclusion  [-]  ?
    has   Declining        Refusal         [-]  ?
    has   DeclaredOrdinal  std::uint64_t   [-]  ?
    has   Sealed           TickPoint       [-]  ?
    by    Source/ConsoleHost.cpp, Source/WorkSequence.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                       ONE QUEUE
//------------------------------------------------------------------------------------------------------------------------

T WorkQueue                            | WorkSequence.h | 196-226 | owning                        | -  | Pending declarations at one priority level, claimed in declaration order. withdrawal costs a write instead of a shift of everything behind it.
    has   PendingOrder  std::vector<std::uint32_t>  [-]  ?
    has   ClaimOrdinal  std::size_t                 [-]  ?
    has   PendingHeld   std::uint32_t               [-]  ?
    by    Source/WorkSequence.cpp
    note  A withdrawn declaration is struck from the order rather than erased from the middle of it, so a

F WorkQueue::Admit                     | WorkSequence.h | 203     | api,nonthrowing               | 🚩 | Admits one record ordinal at the end of the order.
    in    RecordOrdinal  std::uint32_t  [-]  ?
    out   -              void           [-]  ?
    by    Api/PointerIntersection.h, Api/RequestQueue.h, Api/SceneStructure.h, Api/SpatialSubdivision.h, Api/UvSurfaceDepot.h, Source/ConsoleHost.cpp, (+7 more)

F WorkQueue::Claim                     | WorkSequence.h | 209     | api,nonallocating,nonthrowing | ✔️ | Claims the earliest pending record ordinal.
    out   -  Deliver  [-]  refuses with ExtentExhausted when nothing is pending
    by    Api/ByteSpace.h, Api/DescriptorIndex.h, Api/ImageSpace.h, Api/RenderSchedule.h, Api/SpanSpace.h, Api/StrokeSpace.h, (+14 more)

F WorkQueue::Withdraw                  | WorkSequence.h | 214     | api,nonallocating,nonthrowing | ✔️ | Strikes one record ordinal from the order without claiming it.
    in    RecordOrdinal  std::uint32_t  [-]  ?
    out   -              void           [-]  ?
    by    Api/DecalProjection.h, Api/GlyphDepot.h, Api/IlluminantPopulation.h, Api/PointerIntersection.h, Api/PopulationIndex.h, Api/PrimitiveStructure.h, (+16 more)

F WorkQueue::PendingCount              | WorkSequence.h | 219     | api,nonallocating,nonthrowing | ✔️ | How many declarations are pending here.
    out   -  std::uint32_t  [-]  ?
    by    Api/ImpressionSequence.h, Api/StorageExchange.h, Source/ConsoleHost.cpp, Source/ImpressionSequence.cpp, Source/StorageExchange.cpp, Source/WorkSequence.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE SEQUENCE
//------------------------------------------------------------------------------------------------------------------------

T WorkSequence                         | WorkSequence.h | 239-366 | owning                        | -  | The workers, their lifetime, and the dispatch order over three priority levels. `50`, `68`, `20`, `24`, `70` and `82` is declared into this and applied by its requester on the tick. delivers completions ordered by declaration ordinal for exactly that reason — an application order that followed completion order would make the same inputs produce two documents on two machines.
    has   WorkerCeiling               static constexpr std::uint32_t            [-]  ?
    has   InteractiveReservedOrdinal  static constexpr std::uint32_t            [-]  ?
    has   Workers                     std::vector<std::thread>                  [-]  ?
    has   Records                     std::vector<std::unique_ptr<WorkRecord>>  [-]  ?
    has   ReleasedOrdinals            std::vector<std::uint32_t>                [-]  ?
    has   PendingByPriority           WorkQueue[PrioritySpan]                   [-]  ?
    has   SealedCompletions           std::vector<WorkCompletion>               [-]  ?
    has   DrainedCompletions          std::vector<WorkCompletion>               [-]  ?
    has   Timeline                    const TickSequence*                       [-]  ?
    has   Reporting                   ReportSequence*                           [-]  ?
    has   WorkGuard                   mutable std::mutex                        [-]  ?
    has   ArrivalDeclared             std::condition_variable                   [-]  ?
    has   DeclaredCount               std::uint64_t                             [-]  ?
    has   SpannedWorkers              std::uint32_t                             [-]  ?
    has   OccupiedWorkerCount         std::uint32_t                             [-]  ?
    has   TeardownDeclared            bool                                      [-]  ?
    by    Source/ConsoleHost.cpp, Source/WorkSequence.cpp
    note  🔴 `34` §8: no thread is created anywhere in the repository except here. Every long solve in `38`,
    note  🔴 `34` §6: a result must not depend on how many workers ran it or in what order they finished. `Drain`

F WorkSequence::WorkSequence           | WorkSequence.h | 245     | constructor                   | -  | ?
    by    Source/ConsoleHost.cpp, Source/WorkSequence.cpp

F WorkSequence::~WorkSequence          | WorkSequence.h | 248     | destructor                    | -  | ?

F WorkSequence::Construct              | WorkSequence.h | 261     | api,nonthrowing               | 🔴 | Constructs the workers, once, at bring-up. at bring-up. Nothing here decides how many workers a host should run — `34` §4 does, from a reading this only asks for.
    in    RequestedWorkers  std::uint32_t        [-]  workers wanted; zero derives the count from the host
    in    HostTimeline      const TickSequence&  [-]  the process's one timeline, for conclusion stamps
    in    Reporting         ReportSequence&      [-]  where `34` §5's failures are appended
    out   -                 Deliver              [-]  refuses with HostDenied when workers already stand
    post  the worker count is fixed and recorded, so `HardwareMetrics` can attribute a measurement to it
    by    Api/AnalyticProjection.h, Api/AtmosphereIntegrator.h, Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CameraProjection.h, Api/CommandSequence.h, (+62 more)
    note  📝 A zero request reads the count from `04`'s `PlatformInterchange`, which reports the host once

F WorkSequence::Declare                | WorkSequence.h | 271     | api,nonthrowing               | 🚩 | Declares one unit of work, to be resolved by a worker. declaration carries no resolution claims it; nothing about the calling thread decides when.
    in    Arriving  const WorkDeclaration&  [-]  the declaration, its inputs already captured
    out   -         Deliver                 [-]  refuses with HostDenied when no worker stands, and with ContentUnsupported when the
    by    Api/AttachmentIndex.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/DiagnosticExtension.h, (+65 more)
    note  Declaring is not spawning. The declaration takes its place in its priority's order and a worker

F WorkSequence::Withdraw               | WorkSequence.h | 279     | api,nonthrowing               | ✔️ | Withdraws one declaration, because the requester no longer wants it.
    in    Subject  WorkIdentity  [-]  the identity Declare issued
    out   -        Deliver       [-]  refuses with IdentityStale when the declaration has already concluded
    post  the declaration concludes as Withdrawn and produces no result — `34` §5
    by    Api/DecalProjection.h, Api/GlyphDepot.h, Api/IlluminantPopulation.h, Api/PointerIntersection.h, Api/PopulationIndex.h, Api/PrimitiveStructure.h, (+16 more)

F WorkSequence::Supersede              | WorkSequence.h | 287     | api,nonthrowing               | ✔️ | Withdraws one declaration because a newer one replaces it. superseded cancellation ordinary operation, so nothing is appended to the register for it.
    in    Subject  WorkIdentity  [-]  ?
    out   -        Deliver       [-]  refuses with IdentityStale when the declaration has already concluded
    by    Api/SurfaceDepot.h, Source/SurfaceDepot.cpp, Source/WorkSequence.cpp
    note  Reported apart from a withdrawal so the requester can tell the two apart. `86` §5 rules a

F WorkSequence::Drain                  | WorkSequence.h | 303     | api,nonthrowing               | 🚩 | Delivers every conclusion recorded since the last drain, in declaration order. delivered as soon as it is recorded, so an earlier declaration still resolving does not hold a later one back. Holding it back would make a `Background` export block every `Interactive` promotion declared after it, which is the starvation `34` §4 forbids outright. index, never by completion. Two independent declarations read disjoint immutable inputs, so the order their results are applied in carries no information and cannot make two machines differ. a worker applying its own result would linearise against `RevisionSequence` from a thread that does not observe the tick's ordering, which `12` invariant 10 forbids.
    out   -  Completions  [-]  ordered by declaration ordinal within the drain, never by finishing order
    by    Api/RequestQueue.h, Api/StorageExchange.h, Api/WindowInterchange.h, Source/ConsoleHost.cpp, Source/RequestQueue.cpp, Source/StorageExchange.cpp, (+2 more)
    note  🔴 The ordering is **within one drain** and is not a global prefix across drains. A conclusion is
    note  📝 `34` §6's determinism rule binds the parts of **one** split solve — recombined by declared
    note  🔴 Called on the tick and nowhere else. The requester applies each result here — `34` §3 — because

F WorkSequence::Progress               | WorkSequence.h | 309     | api,nonthrowing               | ✔️ | One declaration's resolved fraction.
    in    Subject  WorkIdentity  [-]  ?
    out   -        Deliver       [-]  refuses with IdentityStale once the declaration has concluded
    by    Source/BrushSpecification.cpp, Source/ConsoleHost.cpp, Source/WorkSequence.cpp

F WorkSequence::ProgressCount          | WorkSequence.h | 315     | api,nonthrowing               | ✔️ | One declaration's resolved count.
    in    Subject  WorkIdentity  [-]  ?
    out   -        Deliver       [-]  refuses with IdentityStale once the declaration has concluded
    by    Source/WorkSequence.cpp

F WorkSequence::WorkerCount            | WorkSequence.h | 320     | api,nonallocating,nonthrowing | ✔️ | How many workers stand.
    out   -  std::uint32_t  [-]  ?
    by    Source/ConsoleHost.cpp, Source/WorkSequence.cpp

F WorkSequence::OccupiedWorkers        | WorkSequence.h | 325     | api,nonallocating,nonthrowing | ✔️ | How many workers are resolving something now.
    out   -  std::uint32_t  [-]  ?
    by    Source/WorkSequence.cpp

F WorkSequence::PendingCount           | WorkSequence.h | 330     | api,nonallocating,nonthrowing | ✔️ | How many declarations are pending across every priority.
    out   -  std::uint32_t  [-]  ?
    by    Api/ImpressionSequence.h, Api/StorageExchange.h, Source/ConsoleHost.cpp, Source/ImpressionSequence.cpp, Source/StorageExchange.cpp, Source/WorkSequence.cpp

F WorkSequence::Reclaim                | WorkSequence.h | 336     | api,nonthrowing               | 🔴 | Withdraws everything pending, joins every worker, and returns the sequence to its unconstructed state.
    out   -  void  [-]  ?
    post  every pending declaration has concluded as Withdrawn and is drainable
    by    Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CodeInterchange.h, Api/CommandSequence.h, Api/CycleScheduler.h, Api/DepthReduction.h, (+75 more)

F WorkSequence::Serve                  | WorkSequence.h | 345     | -                             | -  | ?
    in    WorkerOrdinal  std::uint32_t  [-]  ?
    out   -              void           [-]  ?
    by    Source/WorkSequence.cpp

F WorkSequence::Claimable              | WorkSequence.h | 346     | -                             | -  | ?
    in    WorkerOrdinal  std::uint32_t  [-]  ?
    out   -              bool           [-]  ?
    by    Source/WorkSequence.cpp

F WorkSequence::Claim                  | WorkSequence.h | 347     | -                             | -  | ?
    in    WorkerOrdinal  std::uint32_t  [-]  ?
    out   -              std::uint32_t  [-]  ?
    by    Api/ByteSpace.h, Api/DescriptorIndex.h, Api/ImageSpace.h, Api/RenderSchedule.h, Api/SpanSpace.h, Api/StrokeSpace.h, (+14 more)

F WorkSequence::Seal                   | WorkSequence.h | 348     | -                             | -  | ?
    in    RecordOrdinal  std::uint32_t         [-]  ?
    in    Resolved       const Deliver<bool>&  [-]  ?
    out   -              void                  [-]  ?
    by    Api/CameraProjection.h, Api/DecalProjection.h, Api/DocumentSession.h, Api/EmissionSequence.h, Api/ImpressionSequence.h, Api/InterfaceExchange.h, (+19 more)

F WorkSequence::Cancel                 | WorkSequence.h | 349     | -                             | -  | ?
    in    Subject            WorkIdentity   [-]  ?
    in    SupersessionPosed  bool           [-]  ?
    out   -                  Deliver<bool>  [-]  ?
    by    Source/WorkSequence.cpp

F WorkSequence::Resolved               | WorkSequence.h | 350     | -                             | -  | ?
    in    Subject  WorkIdentity   [-]  ?
    out   -        std::uint32_t  [-]  ?
    by    Api/ImpressionSequence.h, Api/PointerIntersection.h, Api/ReferenceIndex.h, Api/ReflectanceIntegrator.h, Api/SceneStructure.h, Api/SpatialSubdivision.h, (+40 more)
