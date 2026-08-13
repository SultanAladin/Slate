//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The scored format, the established chain, the ordered arrival, and the surrender back to the display.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/DisplayScheduler/Source
%layer      SlateVulkan
%sources    1
%symbols    15
%annotated  0/15
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S DisplayScheduler.cpp | 466 lines | 5f8b01bc | 15 sym | The scored format, the established chain, the ordered arrival, and the surrender back to the display.

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE SCORING
//------------------------------------------------------------------------------------------------------------------------

F DisplayScheduler::ScoreFormat       | DisplayScheduler.cpp | 15-42   | -          | - | ?
    in    Surface  VkSurfaceKHR                 [-]  ?
    out   -        Outcome<VkSurfaceFormatKHR>  [-]  ?

F DisplayScheduler::ScorePacing       | DisplayScheduler.cpp | 44-74   | -          | - | ?
    in    Surface  VkSurfaceKHR      [-]  ?
    in    Intent   LatencyIntent     [-]  ?
    out   -        VkPresentModeKHR  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F DisplayScheduler::Construct         | DisplayScheduler.cpp | 80-108  | -          | - | ?
    in    Exchange       const VulkanExchange&       [-]  ?
    in    Naming         const DiagnosticExtension&  [-]  ?
    in    Surface        VkSurfaceKHR                [-]  ?
    in    DisplayWidth   std::uint32_t               [-]  ?
    in    DisplayHeight  std::uint32_t               [-]  ?
    in    Intent         LatencyIntent               [-]  ?
    out   -              Outcome<bool>               [-]  ?

F DisplayScheduler::Reclaim           | DisplayScheduler.cpp | 110-123 | -          | - | ?
    in    DisplayWidth   std::uint32_t  [-]  ?
    in    DisplayHeight  std::uint32_t  [-]  ?
    out   -              Outcome<bool>  [-]  ?

F DisplayScheduler::Establish         | DisplayScheduler.cpp | 125-283 | -          | - | ?
    out   -  Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE ARRIVAL
//------------------------------------------------------------------------------------------------------------------------

F DisplayScheduler::Await             | DisplayScheduler.cpp | 289-367 | -          | - | ?
    in    Standing  const RotationSlot&    [-]  ?
    in    Timeline  const TickSequence&    [-]  ?
    out   -         Outcome<ArrivedImage>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SURRENDER
//------------------------------------------------------------------------------------------------------------------------

F DisplayScheduler::Present           | DisplayScheduler.cpp | 373-411 | -          | - | ?
    in    Standing      const RotationSlot&  [-]  ?
    in    ImageOrdinal  std::uint32_t        [-]  ?
    out   -             Outcome<bool>        [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE READINGS
//------------------------------------------------------------------------------------------------------------------------

F DisplayScheduler::Carries           | DisplayScheduler.cpp | 417     | -          | - | ?
    out   -  VkFormat  [-]  ?

F DisplayScheduler::StandingWidth     | DisplayScheduler.cpp | 418     | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F DisplayScheduler::StandingHeight    | DisplayScheduler.cpp | 419     | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F DisplayScheduler::PacedInterval     | DisplayScheduler.cpp | 420     | -          | - | ?
    out   -  double  [-]  ?

F DisplayScheduler::Presented         | DisplayScheduler.cpp | 421     | -          | - | ?
    out   -  std::uint64_t  [-]  ?

F DisplayScheduler::ChainDepth        | DisplayScheduler.cpp | 423-426 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F DisplayScheduler::Surrender         | DisplayScheduler.cpp | 432-459 | -          | - | ?
    out   -  void  [-]  ?

F DisplayScheduler::~DisplayScheduler | DisplayScheduler.cpp | 461-464 | destructor | - | ?
