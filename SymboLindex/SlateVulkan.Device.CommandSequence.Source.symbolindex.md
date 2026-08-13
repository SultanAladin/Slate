//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The per-slot recording extents, the open that resets one whole, and the surrender to the one graphics queue.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/CommandSequence/Source
%layer      SlateVulkan
%sources    1
%symbols    8
%annotated  0/8
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S CommandSequence.cpp | 358 lines | 8a97331f | 8 sym | The per-slot recording extents, the open that resets one whole, and the surrender to the one graphics queue.

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F CommandSequence::Construct          | CommandSequence.cpp | 15-93   | -          | - | ?
    in    Exchange  const VulkanExchange&       [-]  ?
    in    Naming    const DiagnosticExtension&  [-]  ?
    out   -         Outcome<bool>               [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE OPENING
//------------------------------------------------------------------------------------------------------------------------

F CommandSequence::Open               | CommandSequence.cpp | 99-133  | -          | - | ?
    in    RotationSlot  std::uint32_t             [-]  ?
    out   -             Outcome<VkCommandBuffer>  [-]  ?

F CommandSequence::Recording          | CommandSequence.cpp | 135-147 | -          | - | ?
    in    RotationSlot  std::uint32_t             [-]  ?
    out   -             Outcome<VkCommandBuffer>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SURRENDER
//------------------------------------------------------------------------------------------------------------------------

F CommandSequence::Surrender          | CommandSequence.cpp | 153-204 | -          | - | ?
    in    RotationSlot  std::uint32_t             [-]  ?
    in    Ordering      const SurrenderOrdering&  [-]  ?
    out   -             Outcome<bool>             [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  OUTSIDE THE ROTATION
//------------------------------------------------------------------------------------------------------------------------

F CommandSequence::OpenImmediate      | CommandSequence.cpp | 210-247 | -          | - | ?
    out   -  Outcome<VkCommandBuffer>  [-]  ?

F CommandSequence::SurrenderImmediate | CommandSequence.cpp | 249-317 | -          | - | ?
    in    Recorded  VkCommandBuffer  [-]  ?
    out   -         Outcome<bool>    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F CommandSequence::Reclaim            | CommandSequence.cpp | 323-351 | -          | - | ?
    out   -  void  [-]  ?

F CommandSequence::~CommandSequence   | CommandSequence.cpp | 353-356 | destructor | - | ?
