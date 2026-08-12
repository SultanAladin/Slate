//============================================================================================================================================
//                                                             GLYPHDEPOT.CPP
//============================================================================================================================================
// 🧩 Drives the vector rasteriser and the staging transfer — the one file in which a glyph becomes a vendor texture identity.

#include "SlateUI/Interface/GlyphDepot/Api/GlyphDepot.h"

#include "backends/imgui_impl_vulkan.h"
#include "thorvg.h"

#include <cstring>
#include <memory>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE VECTOR RASTERISER
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📝 The rasteriser is process-wide and reference counted by the vendor already. A second count is held here so that
//    Construct can report the first failure as its own refusal rather than as a later resolution returning nothing.
int RasteriserClaims = 0;   // [-] - depots currently holding the vector engine up

constexpr std::uint32_t RasterEdgeCeiling = 512u;   // [px] - the largest square a declaration may ask for

/// 🧩 One rasterised glyph as texels, before any device object exists.
/// note  Straight-coverage ABGR8888S out of the rasteriser maps one to one onto VK_FORMAT_R8G8B8A8_UNORM, so
///        nothing between here and the upload reorders a component.
struct RasterisedGlyph
{
    std::vector<std::uint32_t>  Texels          = {};      // [-]  - EdgePixels squared, straight coverage
    std::uint32_t               EdgePixels      = 0u;      // [px] - the square edge actually produced
    bool                        RasterDelivered = false;   // [-]  - the whole chain succeeded
};

bool ConstructRasteriser()
{
    if (RasteriserClaims > 0)
    {
        ++RasteriserClaims;
        return true;
    }

    // 📝 A worker count of zero asks the vendor to choose one for the software engine.
    if (tvg::Initializer::init(0u) != tvg::Result::Success)
        return false;

    RasteriserClaims = 1;
    return true;
}

void ReclaimRasteriser()
{
    if (RasteriserClaims <= 0)
        return;

    --RasteriserClaims;

    if (RasteriserClaims == 0)
        tvg::Initializer::term();
}

// 📝 Each rasterisation allocates its own picture and its own software canvas, so two of them share no state and
//    the order declarations arrive in cannot change what any one of them produces.
RasterisedGlyph Rasterise(const char* VectorSource, std::uint32_t SourceExtent, std::uint32_t RasterEdge)
{
    RasterisedGlyph Produced;

    if (RasteriserClaims <= 0 || VectorSource == nullptr || SourceExtent == 0u || RasterEdge == 0u)
        return Produced;

    const std::uint32_t BoundedEdge = RasterEdge > RasterEdgeCeiling ? RasterEdgeCeiling : RasterEdge;

    tvg::Picture* GlyphPicture = tvg::Picture::gen();

    if (GlyphPicture == nullptr)
        return Produced;

    if (GlyphPicture->load(VectorSource, SourceExtent, "svg", nullptr, true) != tvg::Result::Success)
    {
        tvg::Paint::rel(GlyphPicture);
        return Produced;
    }

    // 📝 The picture is sized to the requested square regardless of the extent its own document declares, so a
    //    glyph authored at 24 and one authored at 32 land on the same texel grid.
    GlyphPicture->size(static_cast<float>(BoundedEdge), static_cast<float>(BoundedEdge));

    Produced.Texels.assign(static_cast<std::size_t>(BoundedEdge) * static_cast<std::size_t>(BoundedEdge), 0u);

    // 📝 The canvas carries a public destructor where a paint does not, so it is held in a unique reference that
    //    releases it on every path below. A paint added to a canvas is owned by the canvas from that point on.
    std::unique_ptr<tvg::SwCanvas> RasterCanvas(tvg::SwCanvas::gen());

    if (RasterCanvas == nullptr)
    {
        tvg::Paint::rel(GlyphPicture);
        Produced.Texels.clear();
        return Produced;
    }

    if (RasterCanvas->target(Produced.Texels.data(), BoundedEdge, BoundedEdge, BoundedEdge,
                             tvg::ColorSpace::ABGR8888S) != tvg::Result::Success)
    {
        tvg::Paint::rel(GlyphPicture);
        Produced.Texels.clear();
        return Produced;
    }

    if (RasterCanvas->add(GlyphPicture) != tvg::Result::Success)
    {
        tvg::Paint::rel(GlyphPicture);
        Produced.Texels.clear();
        return Produced;
    }

    if (RasterCanvas->draw(true) != tvg::Result::Success || RasterCanvas->sync() != tvg::Result::Success)
    {
        Produced.Texels.clear();
        return Produced;
    }

    Produced.EdgePixels      = BoundedEdge;
    Produced.RasterDelivered = true;

    return Produced;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DEVICE TRANSFER
//------------------------------------------------------------------------------------------------------------------------

// 📝 The first admitted extent carrying every required property. Identical to how the rest of the engine selects one,
//    so a glyph and a surface never land in differently-propertied storage for reasons nothing states.
std::uint32_t ResolveExtentOrdinal(VkPhysicalDevice      ScoredDevice,
                                   std::uint32_t         AdmittedOrdinals,
                                   VkMemoryPropertyFlags RequiredProperties,
                                   bool&                 OrdinalLocated)
{
    VkPhysicalDeviceMemoryProperties Properties = {};
    vkGetPhysicalDeviceMemoryProperties(ScoredDevice, &Properties);

    for (std::uint32_t Ordinal = 0u; Ordinal < Properties.memoryTypeCount; ++Ordinal)
    {
        const bool Admitted = (AdmittedOrdinals & (1u << Ordinal)) != 0u;
        const bool Carried  = (Properties.memoryTypes[Ordinal].propertyFlags & RequiredProperties)
                            == RequiredProperties;

        if (Admitted && Carried)
        {
            OrdinalLocated = true;
            return Ordinal;
        }
    }

    OrdinalLocated = false;
    return 0u;
}

bool ClaimStagingSource(VkPhysicalDevice  ScoredDevice,
                        VkDevice          ActiveDevice,
                        VkDeviceSize      ByteExtent,
                        VkBuffer&         StagingSource,
                        VkDeviceMemory&   StagingExtent)
{
    StagingSource = VK_NULL_HANDLE;
    StagingExtent = VK_NULL_HANDLE;

    VkBufferCreateInfo SourceInformation = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    SourceInformation.size        = ByteExtent;
    SourceInformation.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    SourceInformation.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(ActiveDevice, &SourceInformation, nullptr, &StagingSource) != VK_SUCCESS)
    {
        StagingSource = VK_NULL_HANDLE;
        return false;
    }

    VkMemoryRequirements Required = {};
    vkGetBufferMemoryRequirements(ActiveDevice, StagingSource, &Required);

    bool                OrdinalLocated = false;
    const std::uint32_t ExtentOrdinal  = ResolveExtentOrdinal(ScoredDevice,
                                                              Required.memoryTypeBits,
                                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                              OrdinalLocated);

    if (!OrdinalLocated)
    {
        vkDestroyBuffer(ActiveDevice, StagingSource, nullptr);
        StagingSource = VK_NULL_HANDLE;
        return false;
    }

    VkMemoryAllocateInfo ExtentInformation = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ExtentInformation.allocationSize  = Required.size;
    ExtentInformation.memoryTypeIndex = ExtentOrdinal;

    if (vkAllocateMemory(ActiveDevice, &ExtentInformation, nullptr, &StagingExtent) != VK_SUCCESS
     || vkBindBufferMemory(ActiveDevice, StagingSource, StagingExtent, 0) != VK_SUCCESS)
    {
        if (StagingExtent != VK_NULL_HANDLE)
            vkFreeMemory(ActiveDevice, StagingExtent, nullptr);

        vkDestroyBuffer(ActiveDevice, StagingSource, nullptr);
        StagingSource = VK_NULL_HANDLE;
        StagingExtent = VK_NULL_HANDLE;
        return false;
    }

    return true;
}

// 📝 One transient recording carries the whole transfer: UNDEFINED to TRANSFER_DST, the copy, then TRANSFER_DST to
//    SHADER_READ_ONLY. It is submitted under a fence this waits on, because a declaration happens at activation and
//    not during a tick — `14` §6 forbids the reverse, not this.
bool TransferIntoImage(VkPhysicalDevice  ScoredDevice,
                       VkDevice          ActiveDevice,
                       VkQueue           GraphicsQueue,
                       VkCommandPool     CommandSlot,
                       VkImage           TargetImage,
                       std::uint32_t     EdgePixels,
                       const void*       ArrivingTexels,
                       VkDeviceSize      ByteExtent)
{
    VkBuffer       StagingSource = VK_NULL_HANDLE;
    VkDeviceMemory StagingExtent = VK_NULL_HANDLE;

    if (!ClaimStagingSource(ScoredDevice, ActiveDevice, ByteExtent, StagingSource, StagingExtent))
        return false;

    void* MappedTexels = nullptr;

    if (vkMapMemory(ActiveDevice, StagingExtent, 0, ByteExtent, 0, &MappedTexels) != VK_SUCCESS)
    {
        vkDestroyBuffer(ActiveDevice, StagingSource, nullptr);
        vkFreeMemory(ActiveDevice, StagingExtent, nullptr);
        return false;
    }

    std::memcpy(MappedTexels, ArrivingTexels, static_cast<std::size_t>(ByteExtent));
    vkUnmapMemory(ActiveDevice, StagingExtent);

    VkCommandBufferAllocateInfo RecordingInformation = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    RecordingInformation.commandPool        = CommandSlot;
    RecordingInformation.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    RecordingInformation.commandBufferCount = 1u;

    VkCommandBuffer CopyRecording = VK_NULL_HANDLE;

    if (vkAllocateCommandBuffers(ActiveDevice, &RecordingInformation, &CopyRecording) != VK_SUCCESS)
    {
        vkDestroyBuffer(ActiveDevice, StagingSource, nullptr);
        vkFreeMemory(ActiveDevice, StagingExtent, nullptr);
        return false;
    }

    VkCommandBufferBeginInfo OpenInformation = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    OpenInformation.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(CopyRecording, &OpenInformation);

    VkImageMemoryBarrier ToTransfer = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    ToTransfer.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
    ToTransfer.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ToTransfer.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    ToTransfer.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    ToTransfer.image                       = TargetImage;
    ToTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ToTransfer.subresourceRange.levelCount = 1u;
    ToTransfer.subresourceRange.layerCount = 1u;
    ToTransfer.srcAccessMask               = 0;
    ToTransfer.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(CopyRecording,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &ToTransfer);

    VkBufferImageCopy CopyExtent = {};
    CopyExtent.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    CopyExtent.imageSubresource.layerCount = 1u;
    CopyExtent.imageExtent                 = { EdgePixels, EdgePixels, 1u };

    vkCmdCopyBufferToImage(CopyRecording, StagingSource, TargetImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &CopyExtent);

    VkImageMemoryBarrier ToRead = ToTransfer;
    ToRead.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ToRead.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ToRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    ToRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(CopyRecording,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &ToRead);

    vkEndCommandBuffer(CopyRecording);

    VkFenceCreateInfo FenceInformation = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    VkFence           TransferFence    = VK_NULL_HANDLE;
    vkCreateFence(ActiveDevice, &FenceInformation, nullptr, &TransferFence);

    VkSubmitInfo Submitted = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    Submitted.commandBufferCount = 1u;
    Submitted.pCommandBuffers    = &CopyRecording;

    bool TransferDelivered = true;

    if (vkQueueSubmit(GraphicsQueue, 1, &Submitted, TransferFence) != VK_SUCCESS)
        TransferDelivered = false;
    else
        vkWaitForFences(ActiveDevice, 1, &TransferFence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(ActiveDevice, TransferFence, nullptr);
    vkFreeCommandBuffers(ActiveDevice, CommandSlot, 1, &CopyRecording);
    vkDestroyBuffer(ActiveDevice, StagingSource, nullptr);
    vkFreeMemory(ActiveDevice, StagingExtent, nullptr);

    return TransferDelivered;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                     BRING-UP AND TEARDOWN
//------------------------------------------------------------------------------------------------------------------------

GlyphDepot::~GlyphDepot()
{
    Reclaim();
}

Outcome<bool> GlyphDepot::Construct(const GlyphAttachment& Arriving)
{
    if (Arriving.ScoredDevice  == VK_NULL_HANDLE
     || Arriving.ActiveDevice  == VK_NULL_HANDLE
     || Arriving.GraphicsQueue == VK_NULL_HANDLE)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::CapabilityAbsent, "a device handle the glyph depot requires is absent" });
    }

    if (!ConstructRasteriser())
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::HostDenied, "the vector rasteriser declined to start" });
    }

    Attached           = Arriving;
    RasteriserStanding = true;

    return Outcome<bool>::Deliver(true);
}

void GlyphDepot::Reclaim()
{
    if (Attached.ActiveDevice != VK_NULL_HANDLE)
    {
        // 📝 A texture identity a recording still references is one the vendor reads after it was freed, and that
        //    read succeeds often enough to present as a different defect entirely.
        vkDeviceWaitIdle(Attached.ActiveDevice);

        for (auto& Held : HeldGlyphs)
        {
            if (Held.second.DescriptorSlot != VK_NULL_HANDLE)
                ImGui_ImplVulkan_RemoveTexture(Held.second.DescriptorSlot);

            if (Held.second.LinearSampler != VK_NULL_HANDLE)
                vkDestroySampler(Attached.ActiveDevice, Held.second.LinearSampler, nullptr);

            if (Held.second.ColourView != VK_NULL_HANDLE)
                vkDestroyImageView(Attached.ActiveDevice, Held.second.ColourView, nullptr);

            if (Held.second.DeviceImage != VK_NULL_HANDLE)
                vkDestroyImage(Attached.ActiveDevice, Held.second.DeviceImage, nullptr);

            if (Held.second.ImageExtent != VK_NULL_HANDLE)
                vkFreeMemory(Attached.ActiveDevice, Held.second.ImageExtent, nullptr);
        }
    }

    HeldGlyphs.clear();
    KeyedContent.clear();
    TieredKeys.clear();

    if (RasteriserStanding)
    {
        ReclaimRasteriser();
        RasteriserStanding = false;
    }

    Attached = {};
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     CONTENT IDENTITY
//------------------------------------------------------------------------------------------------------------------------

std::uint64_t GlyphDepot::ContentHash(const char*   VectorSource,
                                      std::uint32_t SourceExtent,
                                      std::uint32_t RasterEdge) const
{
    // 📐 FNV-1a over the source bytes and then over the edge, so two keys collapse onto one texture exactly when
    //    they would have uploaded identical texels. Folding the edge in is what keeps a 14 px and a 32 px
    //    declaration of the same art apart.
    std::uint64_t Accumulated = 1469598103934665603ull;

    const auto Fold = [&Accumulated](std::uint8_t Byte)
    {
        Accumulated ^= static_cast<std::uint64_t>(Byte);
        Accumulated *= 1099511628211ull;
    };

    for (std::uint32_t Ordinal = 0u; Ordinal < SourceExtent; ++Ordinal)
        Fold(static_cast<std::uint8_t>(VectorSource[Ordinal]));

    for (std::uint32_t Shift = 0u; Shift < 32u; Shift += 8u)
        Fold(static_cast<std::uint8_t>((RasterEdge >> Shift) & 0xFFu));

    return Accumulated;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE UPLOAD
//------------------------------------------------------------------------------------------------------------------------

Outcome<std::uint64_t> GlyphDepot::Upload(const GlyphDeclaration& Declaring, std::uint32_t RasterEdge)
{
    const std::uint64_t ContentIdentity = ContentHash(Declaring.VectorSource, Declaring.SourceExtent, RasterEdge);

    if (HeldGlyphs.find(ContentIdentity) != HeldGlyphs.end())
        return Outcome<std::uint64_t>::Deliver(ContentIdentity);

    const RasterisedGlyph Produced = Rasterise(Declaring.VectorSource, Declaring.SourceExtent, RasterEdge);

    if (!Produced.RasterDelivered)
    {
        return Outcome<std::uint64_t>::Refuse(
            { RefusalReason::ContentUnsupported, Declaring.GlyphKey });
    }

    UploadedGlyph Uploaded;
    Uploaded.RasterEdge = Produced.EdgePixels;

    VkImageCreateInfo ImageInformation = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    ImageInformation.imageType     = VK_IMAGE_TYPE_2D;
    ImageInformation.format        = VK_FORMAT_R8G8B8A8_UNORM;
    ImageInformation.extent        = { Produced.EdgePixels, Produced.EdgePixels, 1u };
    ImageInformation.mipLevels     = 1u;
    ImageInformation.arrayLayers   = 1u;
    ImageInformation.samples       = VK_SAMPLE_COUNT_1_BIT;
    ImageInformation.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ImageInformation.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ImageInformation.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ImageInformation.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(Attached.ActiveDevice, &ImageInformation, nullptr, &Uploaded.DeviceImage) != VK_SUCCESS)
    {
        return Outcome<std::uint64_t>::Refuse(
            { RefusalReason::ExtentExhausted, Declaring.GlyphKey });
    }

    VkMemoryRequirements Required = {};
    vkGetImageMemoryRequirements(Attached.ActiveDevice, Uploaded.DeviceImage, &Required);

    bool                OrdinalLocated = false;
    const std::uint32_t ExtentOrdinal  = ResolveExtentOrdinal(Attached.ScoredDevice,
                                                              Required.memoryTypeBits,
                                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                              OrdinalLocated);

    VkMemoryAllocateInfo ExtentInformation = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ExtentInformation.allocationSize  = Required.size;
    ExtentInformation.memoryTypeIndex = ExtentOrdinal;

    const bool ExtentClaimed = OrdinalLocated
        && vkAllocateMemory(Attached.ActiveDevice, &ExtentInformation, nullptr, &Uploaded.ImageExtent) == VK_SUCCESS
        && vkBindImageMemory(Attached.ActiveDevice, Uploaded.DeviceImage, Uploaded.ImageExtent, 0) == VK_SUCCESS;

    if (!ExtentClaimed)
    {
        if (Uploaded.ImageExtent != VK_NULL_HANDLE)
            vkFreeMemory(Attached.ActiveDevice, Uploaded.ImageExtent, nullptr);

        vkDestroyImage(Attached.ActiveDevice, Uploaded.DeviceImage, nullptr);

        return Outcome<std::uint64_t>::Refuse(
            { RefusalReason::ExtentExhausted, Declaring.GlyphKey });
    }

    VkImageViewCreateInfo ViewInformation = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    ViewInformation.image                       = Uploaded.DeviceImage;
    ViewInformation.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
    ViewInformation.format                      = VK_FORMAT_R8G8B8A8_UNORM;
    ViewInformation.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ViewInformation.subresourceRange.levelCount = 1u;
    ViewInformation.subresourceRange.layerCount = 1u;

    VkCommandPool           CommandSlot     = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo SlotInformation = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    SlotInformation.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    SlotInformation.queueFamilyIndex = Attached.GraphicsFamilyOrdinal;

    bool Constructed = vkCreateImageView(Attached.ActiveDevice, &ViewInformation, nullptr,
                                         &Uploaded.ColourView) == VK_SUCCESS
                    && vkCreateCommandPool(Attached.ActiveDevice, &SlotInformation, nullptr,
                                           &CommandSlot) == VK_SUCCESS;

    if (Constructed)
    {
        const VkDeviceSize ByteExtent = static_cast<VkDeviceSize>(Produced.EdgePixels)
                                      * static_cast<VkDeviceSize>(Produced.EdgePixels) * 4u;

        Constructed = TransferIntoImage(Attached.ScoredDevice, Attached.ActiveDevice, Attached.GraphicsQueue,
                                        CommandSlot, Uploaded.DeviceImage, Produced.EdgePixels,
                                        Produced.Texels.data(), ByteExtent);
    }

    if (CommandSlot != VK_NULL_HANDLE)
        vkDestroyCommandPool(Attached.ActiveDevice, CommandSlot, nullptr);

    if (Constructed)
    {
        VkSamplerCreateInfo SamplerInformation = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        SamplerInformation.magFilter    = VK_FILTER_LINEAR;
        SamplerInformation.minFilter    = VK_FILTER_LINEAR;
        SamplerInformation.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        SamplerInformation.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        SamplerInformation.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        SamplerInformation.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        SamplerInformation.minLod       = 0.0f;
        SamplerInformation.maxLod       = 1.0f;

        Constructed = vkCreateSampler(Attached.ActiveDevice, &SamplerInformation, nullptr,
                                      &Uploaded.LinearSampler) == VK_SUCCESS;
    }

    if (Constructed)
    {
        // 🔗 The one call that turns a view and a sampler into something the interface can sample. The identity it
        //    returns is carried as an integer from here on, so no vendor spelling reaches a panel.
        Uploaded.DescriptorSlot = ImGui_ImplVulkan_AddTexture(Uploaded.LinearSampler, Uploaded.ColourView,
                                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        Constructed             = Uploaded.DescriptorSlot != VK_NULL_HANDLE;
    }

    if (!Constructed)
    {
        if (Uploaded.LinearSampler != VK_NULL_HANDLE)
            vkDestroySampler(Attached.ActiveDevice, Uploaded.LinearSampler, nullptr);

        if (Uploaded.ColourView != VK_NULL_HANDLE)
            vkDestroyImageView(Attached.ActiveDevice, Uploaded.ColourView, nullptr);

        vkDestroyImage(Attached.ActiveDevice, Uploaded.DeviceImage, nullptr);
        vkFreeMemory(Attached.ActiveDevice, Uploaded.ImageExtent, nullptr);

        return Outcome<std::uint64_t>::Refuse(
            { RefusalReason::ExtentExhausted, Declaring.GlyphKey });
    }

    Uploaded.TextureSlot = static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(Uploaded.DescriptorSlot));
    Uploaded.NamingCount = 0u;

    HeldGlyphs.emplace(ContentIdentity, Uploaded);

    return Outcome<std::uint64_t>::Deliver(ContentIdentity);
}

void GlyphDepot::Withdraw(std::uint64_t ContentIdentity)
{
    const auto Located = HeldGlyphs.find(ContentIdentity);

    if (Located == HeldGlyphs.end())
        return;

    if (Located->second.NamingCount > 0u)
        --Located->second.NamingCount;

    if (Located->second.NamingCount > 0u)
        return;

    vkDeviceWaitIdle(Attached.ActiveDevice);

    if (Located->second.DescriptorSlot != VK_NULL_HANDLE)
        ImGui_ImplVulkan_RemoveTexture(Located->second.DescriptorSlot);

    if (Located->second.LinearSampler != VK_NULL_HANDLE)
        vkDestroySampler(Attached.ActiveDevice, Located->second.LinearSampler, nullptr);

    if (Located->second.ColourView != VK_NULL_HANDLE)
        vkDestroyImageView(Attached.ActiveDevice, Located->second.ColourView, nullptr);

    if (Located->second.DeviceImage != VK_NULL_HANDLE)
        vkDestroyImage(Attached.ActiveDevice, Located->second.DeviceImage, nullptr);

    if (Located->second.ImageExtent != VK_NULL_HANDLE)
        vkFreeMemory(Attached.ActiveDevice, Located->second.ImageExtent, nullptr);

    HeldGlyphs.erase(Located);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    TIERS AND RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> GlyphDepot::Declare(const GlyphTier& Declaring)
{
    if (!RasteriserStanding)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the glyph depot is not constructed" });

    if (Declaring.TierName == nullptr || Declaring.Declarations == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the tier declares nothing" });

    std::vector<std::string>& Claimed = TieredKeys[Declaring.TierName];

    for (std::uint32_t Ordinal = 0u; Ordinal < Declaring.DeclaredCount; ++Ordinal)
    {
        const GlyphDeclaration& Entry = Declaring.Declarations[Ordinal];

        if (Entry.GlyphKey == nullptr || Entry.VectorSource == nullptr || Entry.SourceExtent == 0u)
            return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported,
                                           Entry.GlyphKey != nullptr ? Entry.GlyphKey : Declaring.TierName });

        const std::uint32_t RasterEdge = Entry.RasterEdge != 0u ? Entry.RasterEdge : DefaultEdge;
        const std::string   GlyphKey   = Entry.GlyphKey;

        // 📝 A key already standing under this exact content is a second tier naming the same glyph. It costs one
        //    naming count and no upload, which is the arrangement the two-level indirection exists to produce.
        const std::uint64_t ContentIdentity = ContentHash(Entry.VectorSource, Entry.SourceExtent, RasterEdge);
        const auto          PriorKey        = KeyedContent.find(GlyphKey);

        if (PriorKey != KeyedContent.end() && PriorKey->second == ContentIdentity)
        {
            Claimed.push_back(GlyphKey);
            HeldGlyphs[ContentIdentity].NamingCount += 1u;
            continue;
        }

        const Outcome<std::uint64_t> Uploaded = Upload(Entry, RasterEdge);

        if (!Uploaded.ContentPresent)
            return Outcome<bool>::Refuse(Uploaded.Declined);

        if (PriorKey != KeyedContent.end())
            Withdraw(PriorKey->second);

        HeldGlyphs[Uploaded.Resolve()].NamingCount += 1u;
        KeyedContent[GlyphKey] = Uploaded.Resolve();
        Claimed.push_back(GlyphKey);
    }

    return Outcome<bool>::Deliver(true);
}

Outcome<bool> GlyphDepot::Release(const GlyphTier& Releasing)
{
    if (Releasing.TierName == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "the tier names nothing" });

    const auto Located = TieredKeys.find(Releasing.TierName);

    if (Located == TieredKeys.end())
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, Releasing.TierName });

    for (const std::string& GlyphKey : Located->second)
    {
        const auto Keyed = KeyedContent.find(GlyphKey);

        if (Keyed == KeyedContent.end())
            continue;

        const std::uint64_t ContentIdentity = Keyed->second;

        Withdraw(ContentIdentity);

        // 📝 The key leaves only when nothing holds its content any more. A key another standing tier still names
        //    keeps resolving, which is what makes the chrome tier survive a workspace deactivating.
        if (HeldGlyphs.find(ContentIdentity) == HeldGlyphs.end())
            KeyedContent.erase(Keyed);
    }

    TieredKeys.erase(Located);

    return Outcome<bool>::Deliver(true);
}

Outcome<GlyphHandle> GlyphDepot::Resolve(const std::string& GlyphKey) const
{
    const auto Keyed = KeyedContent.find(GlyphKey);

    if (Keyed == KeyedContent.end())
        return Outcome<GlyphHandle>::Refuse({ RefusalReason::ContentUnsupported, "no tier declares that key" });

    const auto Held = HeldGlyphs.find(Keyed->second);

    if (Held == HeldGlyphs.end())
        return Outcome<GlyphHandle>::Refuse({ RefusalReason::ContentUnsupported, "the key names no upload" });

    GlyphHandle Resolved;
    Resolved.DepotSlot = Held->second.TextureSlot;

    return Outcome<GlyphHandle>::Deliver(Resolved);
}

bool GlyphDepot::GlyphHeld(const std::string& GlyphKey) const
{
    return Resolve(GlyphKey).ContentPresent;
}

std::uint32_t GlyphDepot::DeclaredEdge() const
{
    return DefaultEdge;
}

Outcome<bool> GlyphDepot::DeclareEdge(std::uint32_t RasterEdge)
{
    if (RasterEdge == 0u || RasterEdge > RasterEdgeCeiling)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "the declared raster edge is outside the admitted interval" });
    }

    DefaultEdge = RasterEdge;

    return Outcome<bool>::Deliver(true);
}

std::uint32_t GlyphDepot::UploadedCount() const
{
    return static_cast<std::uint32_t>(HeldGlyphs.size());
}

std::uint32_t GlyphDepot::KeyCount() const
{
    return static_cast<std::uint32_t>(KeyedContent.size());
}

}   // namespace Slate
