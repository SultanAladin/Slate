//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 One recording per rotation slot — where commands are written, and the ordered surrender of them to the queue.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/CommandSequence/Api
%layer      SlateVulkan
%sources    1
%symbols    11
%annotated  9/11
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S CommandSequence.h | 144 lines | 5a6d9520 | 11 sym | One recording per rotation slot — where commands are written, and the ordered surrender of them to the queue.

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE ORDERING POINTS
//------------------------------------------------------------------------------------------------------------------------

T SurrenderOrdering                   | CommandSequence.h | 31-37   | nonallocating,nonthrowing     | -  | What one surrender to the queue waits on and what it signals. that waits at the top of the ordering serialises against a point it only needs before it writes colour, and the display stall that produces reads as a device too slow for the extent.
    has   Awaited       VkSemaphore           [-]  ?
    has   AwaitedStage  VkPipelineStageFlags  [-]  ?
    has   Signalled     VkSemaphore           [-]  ?
    has   Completion    VkFence               [-]  ?
    by    Source/CommandSequence.cpp
    note  🔴 The awaited stage is declared alongside the awaited point rather than fixed here. A recording

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RECORDINGS
//------------------------------------------------------------------------------------------------------------------------

T CommandSequence                     | CommandSequence.h | 51-142  | owning                        | -  | The rotation-deep recordings every contributing document writes its commands into. rather than a queue arbitration. `08` §3's diagram is therefore the submission order verbatim, and nothing here reorders what `RenderSchedule::Ordered` fixed. vendor a per-recording allocator it must then keep, and `06` §7 sizes every per-recording resource against the depth precisely so the whole slot can be reset at once.
    has   CompletionCeilingNanoseconds  static constexpr std::uint64_t  [-]  ?
    has   DeviceEdge                    const VulkanExchange*           [-]  ?
    has   NamingEdge                    const DiagnosticExtension*      [-]  ?
    has   Slots                         std::vector<RecordingSlot>      [-]  ?
    has   ImmediateExtent               VkCommandPool                   [-]  ?
    by    Source/CommandSequence.cpp
    note  🔴 `06` §2.1 settles one graphics queue, so ordering between recordings is their order of surrender
    note  ⚠️ One primary recording per rotation slot, reset whole. Resetting an individual recording costs the

F CommandSequence::~CommandSequence   | CommandSequence.h | 58      | destructor                    | -  | ?

F CommandSequence::Construct          | CommandSequence.h | 71      | api,nonthrowing               | 🚩 | Constructs the per-slot recording extents and the one primary recording each holds. device declines an extent or a recording; refused in full driver's text needs to say — a report against an unnamed recording cannot distinguish the slot being written from the one the device is still executing, and that pair is the whole rotation.
    in    Exchange  const VulkanExchange&       [-]  the created device; borrowed and outlives this component
    in    Naming    const DiagnosticExtension&  [-]  names every extent and every recording; borrowed and outlives this component
    out   -         Outcome                     [-]  refuses with CapabilityAbsent when no device is active, ExtentExhausted when the
    post  `RecordingRotationDepth` recordings stand, none of them open
    by    Api/AnalyticProjection.h, Api/AtmosphereIntegrator.h, Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CameraProjection.h, Api/CycleScheduler.h, (+62 more)
    note  🔴 `06` §7's diagnostic-name gate. Each recording is named by its rotation slot, which is what the

F CommandSequence::Open               | CommandSequence.h | 81      | api,nonthrowing               | 🚩 | Resets one rotation slot's recording extent and opens its recording for writing. and HostDenied when the device declines the reset or the open
    in    RotationSlot  std::uint32_t  [-]  below `RecordingRotationDepth`
    out   -             Outcome        [-]  the opened recording; refuses with ContentUnsupported for an excessive slot
    pre   🔴 `CycleScheduler::Await` delivered for this slot — the device no longer reads it
    post  the slot is open; Surrender closes it
    by    Api/CameraProjection.h, Api/DecalProjection.h, Api/DocumentSession.h, Api/EmissionSequence.h, Api/HardwareMetrics.h, Api/ImpressionSequence.h, (+20 more)

F CommandSequence::Recording          | CommandSequence.h | 87      | api,nonallocating,nonthrowing | ✔️ | The recording one rotation slot holds, for a document contributing commands to an open slot.
    in    RotationSlot  std::uint32_t  [-]  ?
    out   -             Outcome        [-]  refuses with ContentUnsupported for an excessive slot or a slot that is not open
    by    Api/DomainSpace.h, Source/AssetInterchange.cpp, Source/CommandSequence.cpp, Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, (+9 more)

F CommandSequence::Surrender          | CommandSequence.h | 101     | api,nonthrowing               | 🚩 | Closes one rotation slot's recording and surrenders it to the one graphics queue. the device declines the close or the surrender, and DeviceLost when the device was lost; the slot is closed and nothing is destroyed either way — a component clearing an ordering point it does not own is one that clears it at the wrong moment for every other reader of it.
    in    RotationSlot  std::uint32_t             [-]  below `RecordingRotationDepth`
    in    Ordering      const SurrenderOrdering&  [-]  what the surrender waits on and signals; any member may be null
    out   -             Outcome                   [-]  refuses with ContentUnsupported for a slot that is not open, HostDenied when
    post  the slot is closed and executing; the completion is signalled when it finishes
    by    Api/AttachmentIndex.h, Api/DisplayScheduler.h, Api/RenderSchedule.h, Api/VisibilityRaster.h, Source/AttachmentIndex.cpp, Source/CommandSequence.cpp, (+3 more)
    note  🔴 The completion is cleared by `CycleScheduler::Arm` immediately before this call and never here

F CommandSequence::OpenImmediate      | CommandSequence.h | 109     | api,nonthrowing               | 🚩 | Opens a recording outside the rotation, for the one-off transfers bring-up records. a rotation's — an immediate wait inside a rotation is the whole device serialised on the host.
    out   -  Outcome  [-]  refuses with ExtentExhausted when the device declines the recording
    by    Source/CommandSequence.cpp
    note  ⚠️ Surrendered and awaited immediately by `SurrenderImmediate`. This is bring-up's path and never

F CommandSequence::SurrenderImmediate | CommandSequence.h | 117     | api,nonthrowing               | 🔴 | Closes an immediate recording, surrenders it, waits for it, and returns it. DeviceLost when the device was lost; the recording is returned either way
    in    Recorded  VkCommandBuffer  [-]  a recording OpenImmediate delivered
    out   -         Outcome          [-]  refuses with HostDenied when the device declines or does not complete, and with
    by    Source/CommandSequence.cpp

F CommandSequence::Reclaim            | CommandSequence.h | 123     | api,nonthrowing               | 🚩 | Destroys every recording and every extent.
    out   -  void  [-]  ?
    pre   the device is idle
    by    Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CodeInterchange.h, Api/CycleScheduler.h, Api/DepthReduction.h, Api/DescriptorIndex.h, (+75 more)

T CommandSequence::RecordingSlot      | CommandSequence.h | 127-132 | -                             | -  | ?
    has   RecordingExtent  VkCommandPool    [-]  ?
    has   Primary          VkCommandBuffer  [-]  ?
    has   SlotOpen         bool             [-]  ?
    by    Source/CommandSequence.cpp
