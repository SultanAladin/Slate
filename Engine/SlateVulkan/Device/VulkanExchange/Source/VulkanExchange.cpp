//============================================================================================================================================
//                                                            VULKANEXCHANGE.CPP
//============================================================================================================================================
// 🧩 Instance construction, device scoring and the one graphics queue.

#include "SlateVulkan/Device/VulkanExchange/Api/VulkanExchange.h"

#include "SlateVulkan/Device/VendorClassifier/Api/VendorClassifier.h"

#include <vector>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       INSTANCE
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> VulkanExchange::ConstructInstance(bool DiagnosticRequested)
{
    VkApplicationInfo ApplicationDeclaration = {};
    ApplicationDeclaration.sType             = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ApplicationDeclaration.pApplicationName  = "Slate";
    ApplicationDeclaration.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    ApplicationDeclaration.pEngineName       = "Slate";
    ApplicationDeclaration.engineVersion     = VK_MAKE_VERSION(0, 1, 0);
    ApplicationDeclaration.apiVersion        = VK_API_VERSION_1_3;

    std::vector<const char*> RequestedExtensions;
    RequestedExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if defined(_WIN32)
    RequestedExtensions.push_back("VK_KHR_win32_surface");
#endif

    std::vector<const char*> RequestedLayers;

    if (DiagnosticRequested)
    {
        RequestedExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        RequestedLayers.push_back("VK_LAYER_KHRONOS_validation");
    }

    VkInstanceCreateInfo InstanceDeclaration    = {};
    InstanceDeclaration.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceDeclaration.pApplicationInfo        = &ApplicationDeclaration;
    InstanceDeclaration.enabledExtensionCount   = static_cast<std::uint32_t>(RequestedExtensions.size());
    InstanceDeclaration.ppEnabledExtensionNames = RequestedExtensions.data();
    InstanceDeclaration.enabledLayerCount       = static_cast<std::uint32_t>(RequestedLayers.size());
    InstanceDeclaration.ppEnabledLayerNames     = RequestedLayers.empty() ? nullptr : RequestedLayers.data();

    if (vkCreateInstance(&InstanceDeclaration, nullptr, &InstanceSlot) != VK_SUCCESS)
    {
        InstanceSlot = VK_NULL_HANDLE;
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no Vulkan instance was created" });
    }

    DiagnosticEnabled = DiagnosticRequested;
    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        DEVICE
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> VulkanExchange::ConstructDevice(VkSurfaceKHR PresentationSurface)
{
    std::uint32_t CandidateCount = 0u;
    vkEnumeratePhysicalDevices(InstanceSlot, &CandidateCount, nullptr);

    if (CandidateCount == 0u)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no device was enumerated" });

    std::vector<VkPhysicalDevice> Candidates(CandidateCount);
    vkEnumeratePhysicalDevices(InstanceSlot, &CandidateCount, Candidates.data());

    ScoredCandidate Winner;

    for (const VkPhysicalDevice Candidate : Candidates)
    {
        const ScoredCandidate Contender = Classify(Candidate, PresentationSurface);

        if (Contender.Ranking > Winner.Ranking)
            Winner = Contender;
    }

    if (Winner.Ranking == 0u)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no device both draws and presents" });

    const float QueuePriority = 1.0f;

    VkDeviceQueueCreateInfo QueueDeclaration = {};
    QueueDeclaration.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    QueueDeclaration.queueFamilyIndex        = Winner.Scored.GraphicsFamilyOrdinal;
    QueueDeclaration.queueCount              = 1u;
    QueueDeclaration.pQueuePriorities        = &QueuePriority;

    const char* DeviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo DeviceDeclaration    = {};
    DeviceDeclaration.sType                 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceDeclaration.queueCreateInfoCount  = 1u;
    DeviceDeclaration.pQueueCreateInfos     = &QueueDeclaration;
    DeviceDeclaration.enabledExtensionCount = 1u;
    DeviceDeclaration.ppEnabledExtensionNames = DeviceExtensions;

    if (vkCreateDevice(Winner.Candidate, &DeviceDeclaration, nullptr, &ActiveDeviceSlot) != VK_SUCCESS)
    {
        ActiveDeviceSlot = VK_NULL_HANDLE;
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "the scored device declined creation" });
    }

    // 📝 🔴 The capability set is fixed here and consulted thereafter. Recovery re-scores rather than
    //    reusing it, because a driver update is one cause of loss and the updated driver may score
    //    differently — `06` §4.2 ④.
    ScoredDeviceSlot = Winner.Candidate;
    ScoredCapability = Winner.Scored;

    vkGetDeviceQueue(ActiveDeviceSlot, ScoredCapability.GraphicsFamilyOrdinal, 0u, &GraphicsQueueSlot);

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

void VulkanExchange::ReclaimDevice()
{
    if (ActiveDeviceSlot == VK_NULL_HANDLE)
        return;

    vkDeviceWaitIdle(ActiveDeviceSlot);
    vkDestroyDevice(ActiveDeviceSlot, nullptr);

    ActiveDeviceSlot  = VK_NULL_HANDLE;
    GraphicsQueueSlot = VK_NULL_HANDLE;
    ScoredDeviceSlot  = VK_NULL_HANDLE;
    ScoredCapability  = {};
}

VulkanExchange::~VulkanExchange()
{
    ReclaimDevice();

    if (InstanceSlot != VK_NULL_HANDLE)
    {
        vkDestroyInstance(InstanceSlot, nullptr);
        InstanceSlot = VK_NULL_HANDLE;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       HANDLES
//------------------------------------------------------------------------------------------------------------------------

VkInstance           VulkanExchange::Instance() const      { return InstanceSlot;      }
VkPhysicalDevice     VulkanExchange::ScoredDevice() const  { return ScoredDeviceSlot;  }
VkDevice             VulkanExchange::ActiveDevice() const  { return ActiveDeviceSlot;  }
VkQueue              VulkanExchange::GraphicsQueue() const { return GraphicsQueueSlot; }
const CapabilitySet& VulkanExchange::Capability() const    { return ScoredCapability;  }

}   // namespace Slate
