//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Drives the vector rasteriser and the staging transfer — the one file in which a glyph becomes a vendor texture identity.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateUI/Interface/GlyphDepot/Source
%layer      SlateUI
%sources    1
%symbols    24
%annotated  1/24
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S GlyphDepot.cpp | 762 lines | 77c8adb0 | 24 sym | Drives the vector rasteriser and the staging transfer — the one file in which a glyph becomes a vendor texture identity.

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE VECTOR RASTERISER
//------------------------------------------------------------------------------------------------------------------------

V RasteriserClaims               | GlyphDepot.cpp | 26      | -          | - | ?

V RasterEdgeCeiling              | GlyphDepot.cpp | 28      | -          | - | ?

T RasterisedGlyph                | GlyphDepot.cpp | 33-38   | -          | - | One rasterised glyph as texels, before any device object exists. nothing between here and the upload reorders a component.
    has   Texels           std::vector<std::uint32_t>  [-]  ?
    has   EdgePixels       std::uint32_t               [-]  ?
    has   RasterDelivered  bool                        [-]  ?
    note  Straight-coverage ABGR8888S out of the rasteriser maps one to one onto VK_FORMAT_R8G8B8A8_UNORM, so

F ConstructRasteriser            | GlyphDepot.cpp | 40-54   | -          | - | ?
    out   -  bool  [-]  ?

F ReclaimRasteriser              | GlyphDepot.cpp | 56-65   | -          | - | ?
    out   -  void  [-]  ?

F Rasterise                      | GlyphDepot.cpp | 69-131  | -          | - | ?
    in    VectorSource  const char*      [-]  ?
    in    SourceExtent  std::uint32_t    [-]  ?
    in    RasterEdge    std::uint32_t    [-]  ?
    out   -             RasterisedGlyph  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE DEVICE TRANSFER
//------------------------------------------------------------------------------------------------------------------------

F ResolveExtentOrdinal           | GlyphDepot.cpp | 139-162 | -          | - | ?
    in    ScoredDevice        VkPhysicalDevice       [-]  ?
    in    AdmittedOrdinals    std::uint32_t          [-]  ?
    in    RequiredProperties  VkMemoryPropertyFlags  [-]  ?
    in    OrdinalLocated      bool&                  [-]  ?
    out   -                   std::uint32_t          [-]  ?

F ClaimStagingSource             | GlyphDepot.cpp | 164-218 | -          | - | ?
    in    ScoredDevice   VkPhysicalDevice  [-]  ?
    in    ActiveDevice   VkDevice          [-]  ?
    in    ByteExtent     VkDeviceSize      [-]  ?
    in    StagingSource  VkBuffer&         [-]  ?
    in    StagingExtent  VkDeviceMemory&   [-]  ?
    out   -              bool              [-]  ?

F TransferIntoImage              | GlyphDepot.cpp | 223-325 | -          | - | ?
    in    ScoredDevice    VkPhysicalDevice  [-]  ?
    in    ActiveDevice    VkDevice          [-]  ?
    in    GraphicsQueue   VkQueue           [-]  ?
    in    CommandSlot     VkCommandPool     [-]  ?
    in    TargetImage     VkImage           [-]  ?
    in    EdgePixels      std::uint32_t     [-]  ?
    in    ArrivingTexels  const void*       [-]  ?
    in    ByteExtent      VkDeviceSize      [-]  ?
    out   -               bool              [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                 BRING-UP AND TEARDOWN
//------------------------------------------------------------------------------------------------------------------------

F GlyphDepot::~GlyphDepot        | GlyphDepot.cpp | 333-336 | destructor | - | ?

F GlyphDepot::Construct          | GlyphDepot.cpp | 338-358 | -          | - | ?
    in    Arriving  const GlyphAttachment&  [-]  ?
    out   -         Deliver<bool>           [-]  ?

F GlyphDepot::Reclaim            | GlyphDepot.cpp | 360-398 | -          | - | ?
    out   -  void  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    CONTENT IDENTITY
//------------------------------------------------------------------------------------------------------------------------

F GlyphDepot::ContentHash        | GlyphDepot.cpp | 404-426 | -          | - | ?
    in    VectorSource  const char*    [-]  ?
    in    SourceExtent  std::uint32_t  [-]  ?
    in    RasterEdge    std::uint32_t  [-]  ?
    out   -             std::uint64_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE UPLOAD
//------------------------------------------------------------------------------------------------------------------------

F GlyphDepot::Upload             | GlyphDepot.cpp | 432-573 | -          | - | ?
    in    Declaring   const GlyphDeclaration&  [-]  ?
    in    RasterEdge  std::uint32_t            [-]  ?
    out   -           Deliver<std::uint64_t>   [-]  ?

F GlyphDepot::Withdraw           | GlyphDepot.cpp | 575-606 | -          | - | ?
    in    ContentIdentity  std::uint64_t  [-]  ?
    out   -                void           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  TIERS AND RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

F GlyphDepot::Declare            | GlyphDepot.cpp | 612-659 | -          | - | ?
    in    Declaring  const GlyphTier&  [-]  ?
    out   -          Deliver<bool>     [-]  ?

F GlyphDepot::Release            | GlyphDepot.cpp | 661-691 | -          | - | ?
    in    Releasing  const GlyphTier&  [-]  ?
    out   -          Deliver<bool>     [-]  ?

F GlyphDepot::Resolve            | GlyphDepot.cpp | 693-709 | -          | - | ?
    in    GlyphKey  const std::string&    [-]  ?
    out   -         Deliver<GlyphHandle>  [-]  ?

F GlyphDepot::GlyphHeld          | GlyphDepot.cpp | 711-714 | -          | - | ?
    in    GlyphKey  const std::string&  [-]  ?
    out   -         bool                [-]  ?

F GlyphDepot::ResolveTextureSlot | GlyphDepot.cpp | 716-732 | -          | - | ?
    in    Presenting  GlyphHandle             [-]  ?
    out   -           Deliver<std::uint64_t>  [-]  ?

F GlyphDepot::DeclaredEdge       | GlyphDepot.cpp | 734-737 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F GlyphDepot::DeclareEdge        | GlyphDepot.cpp | 739-750 | -          | - | ?
    in    RasterEdge  std::uint32_t  [-]  ?
    out   -           Deliver<bool>  [-]  ?

F GlyphDepot::UploadedCount      | GlyphDepot.cpp | 752-755 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F GlyphDepot::KeyCount           | GlyphDepot.cpp | 757-760 | -          | - | ?
    out   -  std::uint32_t  [-]  ?
