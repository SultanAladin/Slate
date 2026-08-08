//============================================================================================================================================
//                                                             VULKANEXCHANGE.H
//============================================================================================================================================
// 🧩 Loader C-ABI, instance and device handles crossing the vendor edge.

#pragma once

#include "Contract/OutcomeContract.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE CAPABILITY SET
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What the created device is capable of, scored once and consulted thereafter.
/// note  🔴 Fixed at device creation and never re-queried. Re-querying at a recording site is how a code
///       path becomes conditional on something that cannot change, and those conditionals never all leave.
/// tag   nonallocating, nonthrowing
struct CapabilitySet
{
    bool           ComputeRasterAvailable   = false;   // [-]  - `16` may take the compute raster path
    bool           HalfPrecisionStore       = false;   // [-]  - `28` and `30` may store at half precision
    bool           TimestampQueryAvailable  = false;   // [-]  - `HardwareMetrics` may measure at all
    std::uint32_t  GraphicsFamilyOrdinal    = 0u;      // [-]  - the one queue family taken
    std::uint64_t  LargestExtentClaim       = 0u;      // [B]  - largest single allocation the device allows
    double         TimestampToMilliseconds  = 0.0;     // [ms] - carried by one timestamp increment
};

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE VENDOR EDGE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Holds the instance, the physical device, the created device and the one graphics queue.
/// note  Vendor spellings are verbatim at this surface — `VkDevice`, `VkQueue`, `VkPhysicalDevice`. Slate's
///       own identifiers wrapping them do not reuse the banned words those spellings contain.
/// tag   owning
class VulkanExchange
{
public:

    VulkanExchange()                                 = default;
    VulkanExchange(const VulkanExchange&)            = delete;
    VulkanExchange& operator=(const VulkanExchange&) = delete;
    ~VulkanExchange();

    /// 🧩 Loads the loader and creates the instance, with the diagnostic capability enabled in Debug only.
    /// in    DiagnosticRequested [-]  true only under SLATE_DEBUG; the caller does not decide otherwise
    /// out   Outcome             [-]  refuses with CapabilityAbsent when no loader or no instance
    /// cost  🔴
    /// tag   api, nonthrowing
    Outcome<bool> ConstructInstance(bool DiagnosticRequested);

    /// 🧩 Enumerates devices, scores them, and creates one with its capability set fixed at creation.
    /// in    PresentationSurface [-]  the surface the device must be able to present to
    /// out   Outcome             [-]  refuses with CapabilityAbsent when no device scores above zero
    /// pre   ConstructInstance delivered
    /// cost  🔴
    /// tag   api, nonthrowing
    Outcome<bool> ConstructDevice(VkSurfaceKHR PresentationSurface);

    /// 🧩 Destroys every device object and retains the instance, for the recovery in `06` §4.2 ③.
    /// cost  🚩
    /// tag   api, nonthrowing
    void ReclaimDevice();

    VkInstance           Instance() const;
    VkPhysicalDevice     ScoredDevice() const;
    VkDevice             ActiveDevice() const;
    VkQueue              GraphicsQueue() const;
    const CapabilitySet& Capability() const;

private:

    VkInstance        InstanceSlot       = VK_NULL_HANDLE;   // [-] - retained across a device loss
    VkPhysicalDevice  ScoredDeviceSlot   = VK_NULL_HANDLE;   // [-] - the winner of VendorClassifier
    VkDevice          ActiveDeviceSlot   = VK_NULL_HANDLE;   // [-] - destroyed and recreated on loss
    VkQueue           GraphicsQueueSlot  = VK_NULL_HANDLE;   // [-] - one queue; transfers ordered inside it
    CapabilitySet     ScoredCapability   = {};               // [-] - re-scored at recovery, never reused
    bool              DiagnosticEnabled  = false;            // [-] - Debug only
};

}   // namespace Slate
