//============================================================================================================================================
//                                                           VENDORCLASSIFIER.CPP
//============================================================================================================================================
// 🧩 Enumerated device scored into a capability set and a ranking.

#include "SlateVulkan/Device/VendorClassifier/Api/VendorClassifier.h"

#include <vector>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       SCORING
//------------------------------------------------------------------------------------------------------------------------

ScoredCandidate Classify(VkPhysicalDevice Candidate, VkSurfaceKHR PresentationSurface)
{
    ScoredCandidate Scored;
    Scored.Candidate = Candidate;

    VkPhysicalDeviceProperties CandidateProperties = {};
    vkGetPhysicalDeviceProperties(Candidate, &CandidateProperties);

    VkPhysicalDeviceFeatures CandidateFeatures = {};
    vkGetPhysicalDeviceFeatures(Candidate, &CandidateFeatures);

    // 📝 One queue family must both draw and present. The spine takes a single graphics queue and orders
    //    transfers inside it, so a family that presents but does not draw is of no use to Slate.
    std::uint32_t FamilyCount = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(Candidate, &FamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> FamilyProperties(FamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(Candidate, &FamilyCount, FamilyProperties.data());

    bool          FamilyFound   = false;
    std::uint32_t FamilyOrdinal = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < FamilyCount; ++Ordinal)
    {
        const bool DrawsHere = (FamilyProperties[Ordinal].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u;

        VkBool32 PresentsHere = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(Candidate, Ordinal, PresentationSurface, &PresentsHere);

        if (DrawsHere && PresentsHere == VK_TRUE)
        {
            FamilyFound   = true;
            FamilyOrdinal = Ordinal;
            break;
        }
    }

    if (!FamilyFound)
    {
        // 📝 Ranking zero is unusable rather than poor. Nothing downstream treats it as a fallback.
        Scored.Ranking = 0u;
        return Scored;
    }

    Scored.Scored.GraphicsFamilyOrdinal   = FamilyOrdinal;
    Scored.Scored.ComputeRasterAvailable  = (FamilyProperties[FamilyOrdinal].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0u;
    Scored.Scored.HalfPrecisionStore      = CandidateFeatures.shaderStorageImageExtendedFormats == VK_TRUE;
    Scored.Scored.TimestampQueryAvailable = CandidateProperties.limits.timestampComputeAndGraphics == VK_TRUE;
    Scored.Scored.LargestExtentClaim      = CandidateProperties.limits.maxStorageBufferRange;

    // 📐 The timestamp period is reported in nanoseconds per increment; the metrics surface reports in
    //    milliseconds, so the conversion is folded in once here rather than at every measurement site.
    Scored.Scored.TimestampToMilliseconds = static_cast<double>(CandidateProperties.limits.timestampPeriod) * 1.0e-6;

    std::uint32_t Ranking = 1u;

    if (CandidateProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        Ranking += 1000u;
    else if (CandidateProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        Ranking += 100u;

    if (Scored.Scored.ComputeRasterAvailable)
        Ranking += 50u;

    if (Scored.Scored.HalfPrecisionStore)
        Ranking += 20u;

    if (Scored.Scored.TimestampQueryAvailable)
        Ranking += 10u;

    Scored.Ranking = Ranking;
    return Scored;
}

}   // namespace Slate
