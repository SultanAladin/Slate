//============================================================================================================================================
//                                                         VIEWPORTSKYSURFACE.H
//============================================================================================================================================
// 🧩 One device-side RGBA8 sky texture the interface draws in the editor viewport.
//
//    The texture is absolute-sized (independent of the display), written by the
//    host's CPU atmosphere evaluation, and presented through the interface's
//    sampled-image path (`ImGui_ImplVulkan_AddTexture` + `ImDrawList::AddImage`).
//    It is the only GPU object the editor's sky owns: no compute dispatch, no
//    render pass, no per-frame upload — a staging copy on change, a sampled quad
//    every frame. That is what makes it resize- and rebuild-safe: the display
//    extent never touches it, and a device loss recreates it whole.

#pragma once

#include "Contract/DeliveryContract.h"
#include "SlateVulkan/Device/DiagnosticExtension/Api/DiagnosticExtension.h"
#include "SlateVulkan/Device/VulkanExchange/Api/VulkanExchange.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Slate
{

/// 🧩 Owns the sky texture's image, view, sampler, staging buffer and upload command pool.
/// tag   owning, nonallocating, nonthrowing
class ViewportSkySurface
{
public:

    static constexpr std::uint32_t SkyWidth  = 2048u;  // [px] - the sky's own resolution (2x the viewport)
    static constexpr std::uint32_t SkyHeight = 1152u;  // [px] - 16:9, matching the editor viewport; independent of the display

    ViewportSkySurface()                              = default;
    ViewportSkySurface(const ViewportSkySurface&)     = delete;
    ViewportSkySurface& operator=(const ViewportSkySurface&) = delete;
    ~ViewportSkySurface();

    /// 🧩 Creates the image, its view, the sampler and the staging extent against the active device.
    /// in    Exchange  [-]  the created device and its queue; borrowed and outlives this component
    /// in    Naming    [-]  names every object; borrowed and outlives this component
    /// out   Result   [-]  refuses with CapabilityAbsent when no device is active, and with
    ///                     ContentUnsupported when the device declines the format or an extent
    /// post  the texture stands in `VK_IMAGE_LAYOUT_UNDEFINED`; the first `Upload` transitions it
    /// cost  🔴
    /// tag   api, nonthrowing
    Outcome<bool> Construct(const VulkanExchange& Exchange, const DiagnosticExtension& Naming);

    /// 🧩 Uploads one RGBA8 frame into the texture, outside any frame's recording.
    /// in    Pixels  [-]  `SkyWidth * SkyHeight * 4` bytes, row-major, top row first
    /// out   Result  [-]  refuses with CapabilityAbsent before Construct
    /// note  🔴 A one-shot submission, exactly as the interface's own font upload runs: staging copy on
    ///        the graphics queue, fenced, and the texture left in `SHADER_READ_ONLY_OPTIMAL`. It never
    ///        runs inside a frame — the interface records inside a dynamic rendering scope, where a copy
    ///        command is invalid — so the host calls it between frames, at most once per environment
    ///        change.
    /// cost  🔴
    /// tag   api, nonthrowing
    Outcome<bool> Upload(const void* Pixels);

    /// 🧩 The image view the interface's sampled-image path reads.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    VkImageView View() const { return ImageViewSlot; }

    /// 🧩 The sampler the interface's sampled-image path reads with.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    VkSampler Sampler() const { return SamplerSlot; }

    std::uint32_t Width() const  { return ExtentWidth; }
    std::uint32_t Height() const { return ExtentHeight; }

    /// 🧩 Destroys every object and forgets the device handles, ahead of a device rebuild.
    /// cost  🔴
    /// tag   api, nonthrowing
    void Reclaim();

private:

    const VulkanExchange*  DeviceEdge    = nullptr;   // [-] - borrowed; never owned
    const DiagnosticExtension* NamingEdge = nullptr;   // [-] - borrowed; never owned

    VkImage        ImageSlot      = VK_NULL_HANDLE;   // [-] - the sky texture
    VkDeviceMemory ImageMemory    = VK_NULL_HANDLE;   // [-]
    VkImageView    ImageViewSlot  = VK_NULL_HANDLE;   // [-]
    VkSampler      SamplerSlot    = VK_NULL_HANDLE;   // [-]
    VkBuffer       StagingSlot    = VK_NULL_HANDLE;   // [-] - the upload extent
    VkDeviceMemory StagingMemory  = VK_NULL_HANDLE;   // [-]
    VkCommandPool  UploadPool     = VK_NULL_HANDLE;   // [-] - one-shot uploads
    VkFence        UploadFence    = VK_NULL_HANDLE;   // [-] - the upload's completion
    std::uint32_t  ExtentWidth    = 0u;               // [px]
    std::uint32_t  ExtentHeight   = 0u;               // [px]
};

} // namespace Slate
