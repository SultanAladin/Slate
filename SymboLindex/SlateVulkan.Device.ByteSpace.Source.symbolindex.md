//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Residency scoring, the first-fit slice, and the coalescing release that keeps an extent from fragmenting away.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/ByteSpace/Source
%layer      SlateVulkan
%sources    1
%symbols    12
%annotated  0/12
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ByteSpace.cpp | 398 lines | df21d451 | 12 sym | Residency scoring, the first-fit slice, and the coalescing release that keeps an extent from fragmenting away.

//------------------------------------------------------------------------------------------------------------------------
//                                                  ALIGNMENT ARITHMETIC
//------------------------------------------------------------------------------------------------------------------------

F PowerOfTwo                   | ByteSpace.cpp | 20-23   | -          | - | ?
    in    Candidate  VkDeviceSize    [-]  ?
    out   -          constexpr bool  [-]  ?

F RaiseToAlignment             | ByteSpace.cpp | 25-28   | -          | - | ?
    in    Offset     VkDeviceSize            [-]  ?
    in    Alignment  VkDeviceSize            [-]  ?
    out   -          constexpr VkDeviceSize  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F ByteSpace::Construct         | ByteSpace.cpp | 35-57   | -          | - | ?
    in    Exchange  const VulkanExchange&       [-]  ?
    in    Naming    const DiagnosticExtension&  [-]  ?
    out   -         Outcome<bool>               [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   RESIDENCY SCORING
//------------------------------------------------------------------------------------------------------------------------

F ByteSpace::ClassifyResidency | ByteSpace.cpp | 63-91   | -          | - | ?
    in    Residency  ExtentResidency         [-]  ?
    out   -          Outcome<std::uint32_t>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   EXTENT ACQUISITION
//------------------------------------------------------------------------------------------------------------------------

F ByteSpace::ConstructExtent   | ByteSpace.cpp | 97-171  | -          | - | ?
    in    Residency   ExtentResidency         [-]  ?
    in    LeastBytes  VkDeviceSize            [-]  ?
    out   -           Outcome<std::uint32_t>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE SLICE
//------------------------------------------------------------------------------------------------------------------------

F ByteSpace::Claim             | ByteSpace.cpp | 177-278 | -          | - | ?
    in    RequestedBytes  VkDeviceSize        [-]  ?
    in    ByteAlignment   VkDeviceSize        [-]  ?
    in    Residency       ExtentResidency     [-]  ?
    in    Standing        ClaimStanding       [-]  ?
    out   -               Outcome<ByteClaim>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F ByteSpace::Release           | ByteSpace.cpp | 284-330 | -          | - | ?
    in    Claimed  const ByteClaim&  [-]  ?
    out   -        void              [-]  ?

F ByteSpace::Reclaim           | ByteSpace.cpp | 332-356 | -          | - | ?
    out   -  void  [-]  ?

F ByteSpace::~ByteSpace        | ByteSpace.cpp | 358-361 | destructor | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS HELD
//------------------------------------------------------------------------------------------------------------------------

F ByteSpace::ClaimedBytes      | ByteSpace.cpp | 367-378 | -          | - | ?
    in    Residency  ExtentResidency  [-]  ?
    out   -          VkDeviceSize     [-]  ?

F ByteSpace::BackingBytes      | ByteSpace.cpp | 380-391 | -          | - | ?
    in    Residency  ExtentResidency  [-]  ?
    out   -          VkDeviceSize     [-]  ?

F ByteSpace::ExtentCount       | ByteSpace.cpp | 393-396 | -          | - | ?
    out   -  std::uint32_t  [-]  ?
