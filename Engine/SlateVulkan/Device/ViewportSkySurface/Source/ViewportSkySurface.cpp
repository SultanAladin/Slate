//============================================================================================================================================
//                                                        VIEWPORTSKYSURFACE.CPP
//============================================================================================================================================

#include "SlateVulkan/Device/ViewportSkySurface/Api/ViewportSkySurface.h"

#include <cstring>

namespace Slate
{

ViewportSkySurface::~ViewportSkySurface()
{
    Reclaim();
}

Outcome<bool> ViewportSkySurface::Construct(const VulkanExchange& Exchange, const DiagnosticExtension& Naming)
{
    if (DeviceEdge != nullptr)
        return Outcome<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "a sky surface construction already stands" });

    const VkDevice Active = Exchange.ActiveDevice();

    if (Active == VK_NULL_HANDLE || Exchange.GraphicsQueue() == VK_NULL_HANDLE)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no device is active" });

    DeviceEdge = &Exchange;
    NamingEdge = &Naming;
    ExtentWidth  = SkyWidth;
    ExtentHeight = SkyHeight;

    // 🔴 Scored before anything is created, on the same grounds as `ImageSpace::Reserve`: a vendor error
    //    at vkCreateImage names the call and not the format, and the refusal then has no operand.
    VkFormatProperties FormatDeclaration = {};
    vkGetPhysicalDeviceFormatProperties(Exchange.ScoredDevice(), VK_FORMAT_R8G8B8A8_UNORM,
                                        &FormatDeclaration);

    if ((FormatDeclaration.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) == 0u)
        return Outcome<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "the device declines the sky format for an optimal image" });

    VkImageCreateInfo ImageDeclaration = {};
    ImageDeclaration.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageDeclaration.imageType         = VK_IMAGE_TYPE_2D;
    ImageDeclaration.format            = VK_FORMAT_R8G8B8A8_UNORM;
    ImageDeclaration.extent.width      = ExtentWidth;
    ImageDeclaration.extent.height     = ExtentHeight;
    ImageDeclaration.extent.depth      = 1u;
    ImageDeclaration.mipLevels         = 1u;
    ImageDeclaration.arrayLayers       = 1u;
    ImageDeclaration.samples           = VK_SAMPLE_COUNT_1_BIT;
    ImageDeclaration.tiling            = VK_IMAGE_TILING_OPTIMAL;
    ImageDeclaration.usage             = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ImageDeclaration.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    ImageDeclaration.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(Active, &ImageDeclaration, nullptr, &ImageSlot) != VK_SUCCESS)
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the sky image was rejected" });

    VkMemoryRequirements MemoryRequirements = {};
    vkGetImageMemoryRequirements(Active, ImageSlot, &MemoryRequirements);

    VkMemoryAllocateInfo Allocation = {};
    Allocation.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    Allocation.allocationSize  = MemoryRequirements.size;

    VkPhysicalDeviceMemoryProperties MemoryProperties = {};
    vkGetPhysicalDeviceMemoryProperties(Exchange.ScoredDevice(), &MemoryProperties);

    bool MemoryTypeFound = false;
    for (std::uint32_t Ordinal = 0u; Ordinal < MemoryProperties.memoryTypeCount; ++Ordinal)
    {
        const VkMemoryType& Candidate = MemoryProperties.memoryTypes[Ordinal];
        if ((MemoryRequirements.memoryTypeBits & (1u << Ordinal)) != 0u &&
            (Candidate.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0u)
        {
            Allocation.memoryTypeIndex = Ordinal;
            MemoryTypeFound = true;
            break;
        }
    }

    if (!MemoryTypeFound || vkAllocateMemory(Active, &Allocation, nullptr, &ImageMemory) != VK_SUCCESS)
    {
        vkDestroyImage(Active, ImageSlot, nullptr);
        ImageSlot = VK_NULL_HANDLE;
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the sky image memory was rejected" });
    }

    if (vkBindImageMemory(Active, ImageSlot, ImageMemory, 0u) != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the sky image memory would not bind" });
    }

    VkImageViewCreateInfo ViewDeclaration = {};
    ViewDeclaration.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ViewDeclaration.image            = ImageSlot;
    ViewDeclaration.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    ViewDeclaration.format           = VK_FORMAT_R8G8B8A8_UNORM;
    ViewDeclaration.components.r     = VK_COMPONENT_SWIZZLE_IDENTITY;
    ViewDeclaration.components.g     = VK_COMPONENT_SWIZZLE_IDENTITY;
    ViewDeclaration.components.b     = VK_COMPONENT_SWIZZLE_IDENTITY;
    ViewDeclaration.components.a     = VK_COMPONENT_SWIZZLE_IDENTITY;
    ViewDeclaration.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    ViewDeclaration.subresourceRange.baseMipLevel   = 0u;
    ViewDeclaration.subresourceRange.levelCount     = 1u;
    ViewDeclaration.subresourceRange.baseArrayLayer = 0u;
    ViewDeclaration.subresourceRange.layerCount     = 1u;

    if (vkCreateImageView(Active, &ViewDeclaration, nullptr, &ImageViewSlot) != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the sky image view was rejected" });
    }

    VkSamplerCreateInfo SamplerDeclaration = {};
    SamplerDeclaration.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerDeclaration.magFilter               = VK_FILTER_LINEAR;
    SamplerDeclaration.minFilter               = VK_FILTER_LINEAR;
    SamplerDeclaration.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    // 🔴 The U axis wraps (REPEAT): the dome's azimuth is periodic, and the viewport mesh spans the
    //    seam whenever the fly camera's yaw crosses ±180° — a clamped sampler would smear the edge
    //    texels across the seam as a stretched band. The V axis genuinely ends at the zenith and the
    //    nadir, so it clamps.
    SamplerDeclaration.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerDeclaration.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerDeclaration.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    SamplerDeclaration.mipLodBias              = 0.0f;
    SamplerDeclaration.anisotropyEnable        = VK_FALSE;
    SamplerDeclaration.compareEnable           = VK_FALSE;
    SamplerDeclaration.minLod                  = 0.0f;
    SamplerDeclaration.maxLod                  = 0.0f;
    SamplerDeclaration.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    SamplerDeclaration.unnormalizedCoordinates = VK_FALSE;

    if (vkCreateSampler(Active, &SamplerDeclaration, nullptr, &SamplerSlot) != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the sky sampler was rejected" });
    }

    // 📝 The staging extent is host-visible and host-coherent: the upload writes it directly and the
    //    copy reads what was written, with no flush or invalidate to forget.
    VkBufferCreateInfo StagingDeclaration = {};
    StagingDeclaration.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    StagingDeclaration.size        = static_cast<VkDeviceSize>(ExtentWidth) * ExtentHeight * 4u;
    StagingDeclaration.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    StagingDeclaration.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(Active, &StagingDeclaration, nullptr, &StagingSlot) != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the sky staging extent was rejected" });
    }

    VkMemoryRequirements StagingRequirements = {};
    vkGetBufferMemoryRequirements(Active, StagingSlot, &StagingRequirements);

    VkMemoryAllocateInfo StagingAllocation = {};
    StagingAllocation.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    StagingAllocation.allocationSize = StagingRequirements.size;

    bool StagingTypeFound = false;
    for (std::uint32_t Ordinal = 0u; Ordinal < MemoryProperties.memoryTypeCount; ++Ordinal)
    {
        const VkMemoryType& Candidate = MemoryProperties.memoryTypes[Ordinal];
        if ((StagingRequirements.memoryTypeBits & (1u << Ordinal)) != 0u &&
            (Candidate.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0u &&
            (Candidate.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0u)
        {
            StagingAllocation.memoryTypeIndex = Ordinal;
            StagingTypeFound = true;
            break;
        }
    }

    if (!StagingTypeFound ||
        vkAllocateMemory(Active, &StagingAllocation, nullptr, &StagingMemory) != VK_SUCCESS ||
        vkBindBufferMemory(Active, StagingSlot, StagingMemory, 0u) != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the sky staging memory was rejected" });
    }

    VkCommandPoolCreateInfo PoolDeclaration = {};
    PoolDeclaration.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    PoolDeclaration.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    PoolDeclaration.queueFamilyIndex = Exchange.Capability().GraphicsFamilyOrdinal;

    if (vkCreateCommandPool(Active, &PoolDeclaration, nullptr, &UploadPool) != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the sky upload pool was rejected" });
    }

    VkFenceCreateInfo FenceDeclaration = {};
    FenceDeclaration.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    FenceDeclaration.flags = VK_FENCE_CREATE_SIGNALED_BIT;   // [-] - the first wait returns immediately

    if (vkCreateFence(Active, &FenceDeclaration, nullptr, &UploadFence) != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the sky upload fence was rejected" });
    }

    if (Naming.Declare(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<std::uint64_t>(ImageSlot), "ViewportSkySurface") &&
        Naming.Declare(VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<std::uint64_t>(ImageViewSlot),
                       "ViewportSkySurface.View") &&
        Naming.Declare(VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<std::uint64_t>(SamplerSlot),
                       "ViewportSkySurface.Sampler") &&
        Naming.Declare(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<std::uint64_t>(StagingSlot),
                       "ViewportSkySurface.Staging"))
    {
        // 📝 Every object named; nothing else to do with the outcome.
    }

    return Outcome<bool>::Result(true);
}

Outcome<bool> ViewportSkySurface::Upload(const void* Pixels)
{
    if (DeviceEdge == nullptr || ImageSlot == VK_NULL_HANDLE)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no sky surface construction stands" });

    if (Pixels == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no sky pixels were supplied" });

    const VkDevice Active = DeviceEdge->ActiveDevice();

    // 🔴 The fence stands signalled from construction; the first upload waits on it immediately. Every
    //    later upload waits for its own previous submission before rewriting the staging extent.
    if (vkWaitForFences(Active, 1u, &UploadFence, VK_TRUE, 1000000000ull) != VK_SUCCESS)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the previous sky upload never completed" });

    void* Mapped = nullptr;
    if (vkMapMemory(Active, StagingMemory, 0u, VK_WHOLE_SIZE, 0u, &Mapped) != VK_SUCCESS)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the sky staging extent would not map" });

    std::memcpy(Mapped, Pixels, static_cast<std::size_t>(ExtentWidth) * ExtentHeight * 4u);
    vkUnmapMemory(Active, StagingMemory);

    vkResetFences(Active, 1u, &UploadFence);

    VkCommandBufferAllocateInfo Allocation = {};
    Allocation.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    Allocation.commandPool        = UploadPool;
    Allocation.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    Allocation.commandBufferCount = 1u;

    VkCommandBuffer Upload = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(Active, &Allocation, &Upload) != VK_SUCCESS)
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the sky upload command was rejected" });

    VkCommandBufferBeginInfo Begin = {};
    Begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    Begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    const VkResult Began = vkBeginCommandBuffer(Upload, &Begin);

    if (Began == VK_SUCCESS)
    {
        // 🔴 The texture stands UNDEFINED until its first upload, and SHADER_READ_ONLY_OPTIMAL after
        //    every one — so the first barrier discards contents and every later one preserves them.
        VkImageMemoryBarrier ToTransfer = {};
        ToTransfer.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        ToTransfer.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        ToTransfer.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ToTransfer.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ToTransfer.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        ToTransfer.image                           = ImageSlot;
        ToTransfer.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ToTransfer.subresourceRange.baseMipLevel   = 0u;
        ToTransfer.subresourceRange.levelCount     = 1u;
        ToTransfer.subresourceRange.baseArrayLayer = 0u;
        ToTransfer.subresourceRange.layerCount     = 1u;

        vkCmdPipelineBarrier(Upload,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0u, 0u, nullptr, 0u, nullptr, 1u, &ToTransfer);

        VkBufferImageCopy Copy = {};
        Copy.bufferOffset                    = 0u;
        Copy.bufferRowLength                 = 0u;   // [-] - tightly packed
        Copy.bufferImageHeight               = 0u;
        Copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        Copy.imageSubresource.mipLevel       = 0u;
        Copy.imageSubresource.baseArrayLayer = 0u;
        Copy.imageSubresource.layerCount     = 1u;
        Copy.imageOffset                     = { 0, 0, 0 };
        Copy.imageExtent.width               = ExtentWidth;
        Copy.imageExtent.height              = ExtentHeight;
        Copy.imageExtent.depth               = 1u;

        vkCmdCopyBufferToImage(Upload, StagingSlot, ImageSlot, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1u, &Copy);

        VkImageMemoryBarrier ToRead = ToTransfer;
        ToRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        ToRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vkCmdPipelineBarrier(Upload,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0u, 0u, nullptr, 0u, nullptr, 1u, &ToRead);

        if (vkEndCommandBuffer(Upload) != VK_SUCCESS)
        {
            vkFreeCommandBuffers(Active, UploadPool, 1u, &Upload);
            return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the sky upload command would not close" });
        }

        VkSubmitInfo Submit = {};
        Submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        Submit.commandBufferCount = 1u;
        Submit.pCommandBuffers    = &Upload;

        // 🔴 Submitted on the graphics queue and fenced. The queue is idle between frames here, so the
        //    submission is ordered before the next frame's presentation.
        const VkResult Submitted = vkQueueSubmit(DeviceEdge->GraphicsQueue(), 1u, &Submit, UploadFence);

        if (Submitted != VK_SUCCESS)
        {
            vkFreeCommandBuffers(Active, UploadPool, 1u, &Upload);
            return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the sky upload would not submit" });
        }

        // 🔴 The fence is awaited BEFORE the command buffer is freed. A command buffer still executing
        //    when `vkFreeCommandBuffers` runs is a use-after-free the validation layer reports at the
        //    destroy rather than at the submission — and the upload is rare (at most once per
        //    environment change), so a synchronous wait costs nothing a frame ever notices.
        if (vkWaitForFences(Active, 1u, &UploadFence, VK_TRUE, 1000000000ull) != VK_SUCCESS)
        {
            vkFreeCommandBuffers(Active, UploadPool, 1u, &Upload);
            return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the sky upload never completed" });
        }
    }
    else
    {
        vkFreeCommandBuffers(Active, UploadPool, 1u, &Upload);
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the sky upload command would not begin" });
    }

    vkFreeCommandBuffers(Active, UploadPool, 1u, &Upload);

    return Outcome<bool>::Result(true);
}

void ViewportSkySurface::Reclaim()
{
    if (DeviceEdge == nullptr)
        return;

    const VkDevice Active = DeviceEdge->ActiveDevice();

    // 🔴 A device that was already reclaimed has no valid handle left — the host reclaims this surface
    //    BEFORE it reclaims the device, so the only way to reach here with a dead device is a caller
    //    that forgot the order. Clearing the edges without touching the driver is the safe landing: the
    //    driver objects were already destroyed with the device.
    if (Active == VK_NULL_HANDLE)
    {
        DeviceEdge    = nullptr;
        NamingEdge    = nullptr;
        ImageSlot     = VK_NULL_HANDLE;
        ImageMemory   = VK_NULL_HANDLE;
        ImageViewSlot = VK_NULL_HANDLE;
        SamplerSlot   = VK_NULL_HANDLE;
        StagingSlot   = VK_NULL_HANDLE;
        StagingMemory = VK_NULL_HANDLE;
        UploadPool    = VK_NULL_HANDLE;
        UploadFence   = VK_NULL_HANDLE;
        ExtentWidth   = 0u;
        ExtentHeight  = 0u;
        return;
    }

    // 🔴 The upload is awaited before anything is destroyed: a submission still reading the staging
    //    extent or the image is a use-after-free the validation layer names only at the destroy.
    if (UploadFence != VK_NULL_HANDLE)
        static_cast<void>(vkWaitForFences(Active, 1u, &UploadFence, VK_TRUE, 1000000000ull));

    if (UploadFence != VK_NULL_HANDLE)
        vkDestroyFence(Active, UploadFence, nullptr);
    if (UploadPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(Active, UploadPool, nullptr);
    if (StagingMemory != VK_NULL_HANDLE)
        vkFreeMemory(Active, StagingMemory, nullptr);
    if (StagingSlot != VK_NULL_HANDLE)
        vkDestroyBuffer(Active, StagingSlot, nullptr);
    if (SamplerSlot != VK_NULL_HANDLE)
        vkDestroySampler(Active, SamplerSlot, nullptr);
    if (ImageViewSlot != VK_NULL_HANDLE)
        vkDestroyImageView(Active, ImageViewSlot, nullptr);
    if (ImageMemory != VK_NULL_HANDLE)
        vkFreeMemory(Active, ImageMemory, nullptr);
    if (ImageSlot != VK_NULL_HANDLE)
        vkDestroyImage(Active, ImageSlot, nullptr);

    DeviceEdge    = nullptr;
    NamingEdge    = nullptr;
    ImageSlot     = VK_NULL_HANDLE;
    ImageMemory   = VK_NULL_HANDLE;
    ImageViewSlot = VK_NULL_HANDLE;
    SamplerSlot   = VK_NULL_HANDLE;
    StagingSlot   = VK_NULL_HANDLE;
    StagingMemory = VK_NULL_HANDLE;
    UploadPool    = VK_NULL_HANDLE;
    UploadFence   = VK_NULL_HANDLE;
    ExtentWidth   = 0u;
    ExtentHeight  = 0u;
}

} // namespace Slate
