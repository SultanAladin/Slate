//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The image claim, the one place a layout transition is recorded, and the reclamation that returns both.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/ImageSpace/Source
%layer      SlateVulkan
%sources    1
%symbols    13
%annotated  0/13
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ImageSpace.cpp | 458 lines | d97b1d23 | 13 sym | The image claim, the one place a layout transition is recorded, and the reclamation that returns both.

//------------------------------------------------------------------------------------------------------------------------
//                                                WHAT THE INTENT REQUIRES
//------------------------------------------------------------------------------------------------------------------------

F ImageSpace::UsageOf      | ImageSpace.cpp | 15-37   | -          | - | ?
    in    Intent  ImageIntent        [-]  ?
    out   -       VkImageUsageFlags  [-]  ?

F ImageSpace::AspectOf     | ImageSpace.cpp | 39-44   | -          | - | ?
    in    Intent  ImageIntent         [-]  ?
    out   -       VkImageAspectFlags  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F ImageSpace::Construct    | ImageSpace.cpp | 50-59   | -          | - | ?
    in    Exchange      const VulkanExchange&  [-]  ?
    in    BackingSpace  ByteSpace&             [-]  ?
    out   -             Outcome<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE CLAIM
//------------------------------------------------------------------------------------------------------------------------

F ImageSpace::Claim        | ImageSpace.cpp | 65-203  | -          | - | ?
    in    Declared  const ImageShape&    [-]  ?
    out   -         Outcome<ImageClaim>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE TRANSITION
//------------------------------------------------------------------------------------------------------------------------

F ReachedAt                | ImageSpace.cpp | 214-260 | -          | - | ?
    in    Standing  VkImageLayout          [-]  ?
    in    Stage     VkPipelineStageFlags&  [-]  ?
    in    Access    VkAccessFlags&         [-]  ?
    out   -         void                   [-]  ?

F ImageSpace::Transition   | ImageSpace.cpp | 263-310 | -          | - | ?
    in    Recorded      VkCommandBuffer  [-]  ?
    in    ImageOrdinal  std::uint32_t    [-]  ?
    in    Arriving      VkImageLayout    [-]  ?
    out   -             Outcome<bool>    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT IS CLAIMED
//------------------------------------------------------------------------------------------------------------------------

F ImageSpace::Standing     | ImageSpace.cpp | 316-331 | -          | - | ?
    in    ImageOrdinal  std::uint32_t        [-]  ?
    out   -             Outcome<ImageClaim>  [-]  ?

F ImageSpace::LevelView    | ImageSpace.cpp | 333-375 | -          | - | ?
    in    ImageOrdinal  std::uint32_t         [-]  ?
    in    LevelOrdinal  std::uint32_t         [-]  ?
    out   -             Outcome<VkImageView>  [-]  ?

F ImageSpace::ClaimedCount | ImageSpace.cpp | 377-388 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F ImageSpace::ClaimedBytes | ImageSpace.cpp | 390-401 | -          | - | ?
    out   -  VkDeviceSize  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F ImageSpace::Release      | ImageSpace.cpp | 407-443 | -          | - | ?
    in    ImageOrdinal  std::uint32_t  [-]  ?
    out   -             void           [-]  ?

F ImageSpace::Reclaim      | ImageSpace.cpp | 445-451 | -          | - | ?
    out   -  void  [-]  ?

F ImageSpace::~ImageSpace  | ImageSpace.cpp | 453-456 | destructor | - | ?
