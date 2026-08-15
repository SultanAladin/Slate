//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The loader resolution, the attached sink, the arrival that appends, and the per-object name.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/DiagnosticExtension/Source
%layer      SlateVulkan
%sources    1
%symbols    8
%annotated  0/8
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S DiagnosticExtension.cpp | 234 lines | 240de5bf | 8 sym | The loader resolution, the attached sink, the arrival that appends, and the per-object name.

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F DiagnosticExtension::Construct            | DiagnosticExtension.cpp | 17-79   | -          | - | ?
    in    Exchange  const VulkanExchange&  [-]  ?
    in    Register  ReportSequence&        [-]  ?
    in    Timeline  const TickSequence&    [-]  ?
    out   -         Deliver<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE ARRIVAL
//------------------------------------------------------------------------------------------------------------------------

F DiagnosticExtension::Arrival              | DiagnosticExtension.cpp | 85-131  | -          | - | ?
    in    Severity    VkDebugUtilsMessageSeverityFlagBitsEXT       [-]  ?
    in    Reported    VkDebugUtilsMessageTypeFlagsEXT              [-]  ?
    in    Arriving    const VkDebugUtilsMessengerCallbackDataEXT*  [-]  ?
    in    Forwarding  void*                                        [-]  ?
    out   -           VkBool32                                     [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE OBJECT NAME
//------------------------------------------------------------------------------------------------------------------------

F DiagnosticExtension::Declare              | DiagnosticExtension.cpp | 137-161 | -          | - | ?
    in    Subject       VkObjectType   [-]  ?
    in    VendorHandle  std::uint64_t  [-]  ?
    in    DeclaredName  const char*    [-]  ?
    out   -             Deliver<bool>  [-]  ?

F DiagnosticExtension::Declare              | DiagnosticExtension.cpp | 163-189 | -          | - | ?
    in    Subject         VkObjectType   [-]  ?
    in    VendorHandle    std::uint64_t  [-]  ?
    in    DeclaredPrefix  const char*    [-]  ?
    in    Ordinal         std::uint32_t  [-]  ?
    out   -               Deliver<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE READINGS
//------------------------------------------------------------------------------------------------------------------------

F DiagnosticExtension::Negotiated           | DiagnosticExtension.cpp | 195-198 | -          | - | ?
    out   -  bool  [-]  ?

F DiagnosticExtension::ArrivalCount         | DiagnosticExtension.cpp | 200-203 | -          | - | ?
    out   -  std::uint64_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F DiagnosticExtension::Reclaim              | DiagnosticExtension.cpp | 209-227 | -          | - | ?
    out   -  void  [-]  ?

F DiagnosticExtension::~DiagnosticExtension | DiagnosticExtension.cpp | 229-232 | destructor | - | ?
