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

S CommandSequence.cpp | 311 lines | f2de6185 | 8 sym | The per-slot recording extents, the open that resets one whole, and the surrender to the one graphics queue.

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F CommandSequence::Construct          | CommandSequence.cpp | 15-70   | -          | - | ?
    in    Exchange  const VulkanExchange&  [-]  ?
    out   -         Outcome<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE OPENING
//------------------------------------------------------------------------------------------------------------------------

F CommandSequence::Open               | CommandSequence.cpp | 76-110  | -          | - | ?
    in    RotationSlot  std::uint32_t             [-]  ?
    out   -             Outcome<VkCommandBuffer>  [-]  ?

F CommandSequence::Recording          | CommandSequence.cpp | 112-124 | -          | - | ?
    in    RotationSlot  std::uint32_t             [-]  ?
    out   -             Outcome<VkCommandBuffer>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SURRENDER
//------------------------------------------------------------------------------------------------------------------------

F CommandSequence::Surrender          | CommandSequence.cpp | 130-176 | -          | - | ?
    in    RotationSlot  std::uint32_t             [-]  ?
    in    Ordering      const SurrenderOrdering&  [-]  ?
    out   -             Outcome<bool>             [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  OUTSIDE THE ROTATION
//------------------------------------------------------------------------------------------------------------------------

F CommandSequence::OpenImmediate      | CommandSequence.cpp | 182-213 | -          | - | ?
    out   -  Outcome<VkCommandBuffer>  [-]  ?

F CommandSequence::SurrenderImmediate | CommandSequence.cpp | 215-270 | -          | - | ?
    in    Recorded  VkCommandBuffer  [-]  ?
    out   -         Outcome<bool>    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F CommandSequence::Reclaim            | CommandSequence.cpp | 276-304 | -          | - | ?
    out   -  void  [-]  ?

F CommandSequence::~CommandSequence   | CommandSequence.cpp | 306-309 | destructor | - | ?
