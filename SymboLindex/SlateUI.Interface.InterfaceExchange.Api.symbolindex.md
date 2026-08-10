//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The one seam the interface library crosses — device handles in, recorded commands out, no ImGui spelling.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateUI/Interface/InterfaceExchange/Api
%layer      SlateUI
%sources    1
%symbols    10
%annotated  9/10
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S InterfaceExchange.h | 116 lines | ace19393 | 10 sym | The one seam the interface library crosses — device handles in, recorded commands out, no ImGui spelling.

//------------------------------------------------------------------------------------------------------------------------
//                                             WHAT THE INTERFACE ATTACHES TO
//------------------------------------------------------------------------------------------------------------------------

T InterfaceAttachment                   | InterfaceExchange.h | 28-38  | nonallocating,nonthrowing     | -  | Every device handle the interface library needs, supplied once at bring-up. is an ImGui spelling: the whole point of the seam is that a host including this header links the interface without acquiring ImGui's declarations. `00` §2.2 makes a host that includes `imgui.h` a defect, and a defect that cannot be spelled cannot be committed.
    has   Instance               VkInstance        [-]  ?
    has   ScoredDevice           VkPhysicalDevice  [-]  ?
    has   ActiveDevice           VkDevice          [-]  ?
    has   GraphicsQueue          VkQueue           [-]  ?
    has   GraphicsFamilyOrdinal  std::uint32_t     [-]  ?
    has   ColourTargetFormat     VkFormat          [-]  ?
    has   RotationDepth          std::uint32_t     [-]  ?
    has   NativeWindowSlot       void*             [-]  ?
    by    Source/InterfaceExchange.cpp
    note  🔴 Vendor spellings are verbatim here because this is the vendor surface. Nothing in this struct

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE INTERFACE SEAM
//------------------------------------------------------------------------------------------------------------------------

T InterfaceExchange                     | InterfaceExchange.h | 50-114 | owning                        | -  | Holds the interface context and the two vendor attachments that feed it. source file. `00` §2.2: exactly one copy of ImGui exists, compiled inside `SlateUI`. projection, so nothing recorded by this component is ever tone-mapped a second time.
    has   Attached          InterfaceAttachment  [-]  ?
    has   DescriptorSlot    VkDescriptorPool     [-]  ?
    has   ContextSlot       void*                [-]  ?
    has   TickOpen          bool                 [-]  ?
    has   ContentAssembled  bool                 [-]  ?
    by    Source/InterfaceExchange.cpp
    note  🔴 This is the only component in the engine that names ImGui, and it names it only inside its
    note  ⚠️ Everything recorded here is display-referred. `08` §3.1 places the interface after the tone

F InterfaceExchange::~InterfaceExchange | InterfaceExchange.h | 57     | destructor                    | -  | ?

F InterfaceExchange::Construct          | InterfaceExchange.h | 68     | api,nonthrowing               | 🔴 | Constructs the interface context over the supplied device handles. HostDenied when the vendor attachment declines `VK_KHR_dynamic_rendering` or a device at Vulkan 1.3. Construct refuses rather than recording into a target the device never agreed to.
    in    Arriving  const InterfaceAttachment&  [-]  the device handles and the window the interface reads from
    out   -         Outcome                     [-]  refuses with CapabilityAbsent when any required handle is absent, and with
    by    Api/AnalyticProjection.h, Api/AtmosphereIntegrator.h, Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CameraProjection.h, Api/CommandSequence.h, (+46 more)
    note  🚧 Recording is declared against dynamic rendering, so `06`'s bring-up must negotiate

F InterfaceExchange::Reclaim            | InterfaceExchange.h | 73     | api,nonthrowing               | 🚩 | Destroys the interface context and both vendor attachments.
    out   -  void  [-]  ?
    by    Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CommandSequence.h, Api/CycleScheduler.h, Api/DepthReduction.h, Api/DescriptorIndex.h, (+49 more)

F InterfaceExchange::Advance            | InterfaceExchange.h | 79     | api,nonthrowing               | ✔️ | Opens one interface tick and reads the window system's accumulated condition.
    out   -  Outcome  [-]  refuses when no context is constructed, or when a tick is already open
    by    Api/CycleScheduler.h, Api/OutlinerSequence.h, Api/RevisionSequence.h, Api/SelectionSequence.h, Api/TickSequence.h, Api/VectorInterchange.h, (+11 more)

F InterfaceExchange::Seal               | InterfaceExchange.h | 86     | api,nonthrowing               | 🚩 | Closes the open tick and assembles its command content, ready to record.
    out   -  Outcome  [-]  refuses when no tick is open
    post  the assembled content stays valid until the next Advance
    by    Api/CameraProjection.h, Api/DecalProjection.h, Api/ImpressionSequence.h, Api/RevisionSequence.h, Api/SelectionSequence.h, Api/TopologyStructure.h, (+12 more)

F InterfaceExchange::Record             | InterfaceExchange.h | 94     | api,nonthrowing               | 🚩 | Records the assembled content into a command recording of the current rotation slot.
    in    CommandRecording  VkCommandBuffer  [-]  a recording already inside a dynamic rendering scope over DisplaySurface
    out   -                 Outcome          [-]  refuses when nothing has been sealed since the last Advance
    pre   Seal delivered
    by    Api/InputExchange.h, Api/IntakeIndex.h, Api/VisibilityRaster.h, Source/AssetInterchange.cpp, Source/ConsoleHost.cpp, Source/InputExchange.cpp, (+4 more)

F InterfaceExchange::PointerCaptured    | InterfaceExchange.h | 99     | api,nonallocating,nonthrowing | ✔️ | Whether the interface has taken the pointer, so that `22` must not treat it as a canvas stroke.
    out   -  bool  [-]  ?
    by    Source/InterfaceExchange.cpp

F InterfaceExchange::KeyboardCaptured   | InterfaceExchange.h | 104    | api,nonallocating,nonthrowing | ✔️ | Whether the interface has taken text entry, so that no shortcut consumes the same key.
    out   -  bool  [-]  ?
    by    Source/InterfaceExchange.cpp
