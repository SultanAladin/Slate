//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The image claim, the one place a layout transition is recorded, and the reclamation that returns both.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/ImageSpace/Source
%layer      SlateVulkan
%sources    1
%symbols    14
%annotated  0/14
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ImageSpace.cpp | 499 lines | e5d98cf2 | 14 sym | The image claim, the one place a layout transition is recorded, and the reclamation that returns both.

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

F ImageSpace::Construct    | ImageSpace.cpp | 50-62   | -          | - | ?
    in    Exchange      const VulkanExchange&       [-]  ?
    in    BackingSpace  ByteSpace&                  [-]  ?
    in    Naming        const DiagnosticExtension&  [-]  ?
    out   -             Outcome<bool>               [-]  ?

F ImageSpace::NameOf       | ImageSpace.cpp | 64-78   | -          | - | ?
    in    Intent  ImageIntent  [-]  ?
    out   -       const char*  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE CLAIM
//------------------------------------------------------------------------------------------------------------------------

F ImageSpace::Claim        | ImageSpace.cpp | 84-235  | -          | - | ?
    in    Declared  const ImageShape&    [-]  ?
    out   -         Outcome<ImageClaim>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE TRANSITION
//------------------------------------------------------------------------------------------------------------------------

F ReachedAt                | ImageSpace.cpp | 246-292 | -          | - | ?
    in    Standing  VkImageLayout          [-]  ?
    in    Stage     VkPipelineStageFlags&  [-]  ?
    in    Access    VkAccessFlags&         [-]  ?
    out   -         void                   [-]  ?

F ImageSpace::Transition   | ImageSpace.cpp | 295-342 | -          | - | ?
    in    Recorded      VkCommandBuffer  [-]  ?
    in    ImageOrdinal  std::uint32_t    [-]  ?
    in    Arriving      VkImageLayout    [-]  ?
    out   -             Outcome<bool>    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT IS CLAIMED
//------------------------------------------------------------------------------------------------------------------------

F ImageSpace::Standing     | ImageSpace.cpp | 348-363 | -          | - | ?
    in    ImageOrdinal  std::uint32_t        [-]  ?
    out   -             Outcome<ImageClaim>  [-]  ?

F ImageSpace::LevelView    | ImageSpace.cpp | 365-416 | -          | - | ?
    in    ImageOrdinal  std::uint32_t         [-]  ?
    in    LevelOrdinal  std::uint32_t         [-]  ?
    out   -             Outcome<VkImageView>  [-]  ?

F ImageSpace::ClaimedCount | ImageSpace.cpp | 418-429 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F ImageSpace::ClaimedBytes | ImageSpace.cpp | 431-442 | -          | - | ?
    out   -  VkDeviceSize  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F ImageSpace::Release      | ImageSpace.cpp | 448-484 | -          | - | ?
    in    ImageOrdinal  std::uint32_t  [-]  ?
    out   -             void           [-]  ?

F ImageSpace::Reclaim      | ImageSpace.cpp | 486-492 | -          | - | ?
    out   -  void  [-]  ?

F ImageSpace::~ImageSpace  | ImageSpace.cpp | 494-497 | destructor | - | ?
