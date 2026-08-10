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

S ByteSpace.cpp | 386 lines | e690575d | 12 sym | Residency scoring, the first-fit slice, and the coalescing release that keeps an extent from fragmenting away.

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

F ByteSpace::Construct         | ByteSpace.cpp | 35-56   | -          | - | ?
    in    Exchange  const VulkanExchange&  [-]  ?
    out   -         Outcome<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   RESIDENCY SCORING
//------------------------------------------------------------------------------------------------------------------------

F ByteSpace::ClassifyResidency | ByteSpace.cpp | 62-90   | -          | - | ?
    in    Residency  ExtentResidency         [-]  ?
    out   -          Outcome<std::uint32_t>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   EXTENT ACQUISITION
//------------------------------------------------------------------------------------------------------------------------

F ByteSpace::ConstructExtent   | ByteSpace.cpp | 96-159  | -          | - | ?
    in    Residency   ExtentResidency         [-]  ?
    in    LeastBytes  VkDeviceSize            [-]  ?
    out   -           Outcome<std::uint32_t>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE SLICE
//------------------------------------------------------------------------------------------------------------------------

F ByteSpace::Claim             | ByteSpace.cpp | 165-266 | -          | - | ?
    in    RequestedBytes  VkDeviceSize        [-]  ?
    in    ByteAlignment   VkDeviceSize        [-]  ?
    in    Residency       ExtentResidency     [-]  ?
    in    Standing        ClaimStanding       [-]  ?
    out   -               Outcome<ByteClaim>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F ByteSpace::Release           | ByteSpace.cpp | 272-318 | -          | - | ?
    in    Claimed  const ByteClaim&  [-]  ?
    out   -        void              [-]  ?

F ByteSpace::Reclaim           | ByteSpace.cpp | 320-344 | -          | - | ?
    out   -  void  [-]  ?

F ByteSpace::~ByteSpace        | ByteSpace.cpp | 346-349 | destructor | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS HELD
//------------------------------------------------------------------------------------------------------------------------

F ByteSpace::ClaimedBytes      | ByteSpace.cpp | 355-366 | -          | - | ?
    in    Residency  ExtentResidency  [-]  ?
    out   -          VkDeviceSize     [-]  ?

F ByteSpace::BackingBytes      | ByteSpace.cpp | 368-379 | -          | - | ?
    in    Residency  ExtentResidency  [-]  ?
    out   -          VkDeviceSize     [-]  ?

F ByteSpace::ExtentCount       | ByteSpace.cpp | 381-384 | -          | - | ?
    out   -  std::uint32_t  [-]  ?
