//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Instance construction, device scoring and the one graphics queue.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/VulkanExchange/Source
%layer      SlateVulkan
%sources    1
%symbols    9
%annotated  0/9
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S VulkanExchange.cpp | 163 lines | fa49f080 | 9 sym | Instance construction, device scoring and the one graphics queue.

//------------------------------------------------------------------------------------------------------------------------
//                                                        INSTANCE
//------------------------------------------------------------------------------------------------------------------------

F VulkanExchange::ConstructInstance | VulkanExchange.cpp | 19-60   | -          | - | ?
    in    DiagnosticRequested  bool           [-]  ?
    out   -                    Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                         DEVICE
//------------------------------------------------------------------------------------------------------------------------

F VulkanExchange::ConstructDevice   | VulkanExchange.cpp | 66-122  | -          | - | ?
    in    PresentationSurface  VkSurfaceKHR   [-]  ?
    out   -                    Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F VulkanExchange::ReclaimDevice     | VulkanExchange.cpp | 128-140 | -          | - | ?
    out   -  void  [-]  ?

F VulkanExchange::~VulkanExchange   | VulkanExchange.cpp | 142-151 | destructor | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        HANDLES
//------------------------------------------------------------------------------------------------------------------------

F VulkanExchange::Instance          | VulkanExchange.cpp | 157     | -          | - | ?
    out   -  VkInstance  [-]  ?

F VulkanExchange::ScoredDevice      | VulkanExchange.cpp | 158     | -          | - | ?
    out   -  VkPhysicalDevice  [-]  ?

F VulkanExchange::ActiveDevice      | VulkanExchange.cpp | 159     | -          | - | ?
    out   -  VkDevice  [-]  ?

F VulkanExchange::GraphicsQueue     | VulkanExchange.cpp | 160     | -          | - | ?
    out   -  VkQueue  [-]  ?

F VulkanExchange::Capability        | VulkanExchange.cpp | 161     | -          | - | ?
    out   -  const CapabilitySet&  [-]  ?
