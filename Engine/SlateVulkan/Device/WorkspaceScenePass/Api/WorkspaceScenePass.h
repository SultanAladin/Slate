//============================================================================================================================================
//                                                        WORKSPACESCENEPASS.H
//============================================================================================================================================
// 🧩 Dedicated scene-mesh GPU pass seam for imported workspace polygon objects. The pass owns scene-triangle
//    upload state separately from the CAD and overlay passes so mesh rendering can evolve independently.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "SlateVulkan/Device/DiagnosticExtension/Api/DiagnosticExtension.h"
#include "SlateVulkan/Device/ShaderCodec/Api/ShaderCodec.h"
#include "SlateVulkan/Device/VulkanExchange/Api/VulkanExchange.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Slate
{

struct WorkspaceSceneProjection
{
    float DisplayWidth = 1.0f;
    float DisplayHeight = 1.0f;
    float ViewProjection[16] = {};
};

struct WorkspaceSceneTriangle
{
    float Position[9] = {};
    float Colour[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    std::uint32_t MaterialSlot = 0u;
};

class WorkspaceScenePass
{
public:
    WorkspaceScenePass() = default;
    WorkspaceScenePass(const WorkspaceScenePass&) = delete;
    WorkspaceScenePass& operator=(const WorkspaceScenePass&) = delete;
    ~WorkspaceScenePass();

    Outcome<bool> ConstructWorkspaceScenePass(const VulkanExchange& Exchange,
                                              const DiagnosticExtension& Naming,
                                              ShaderCodec& Streams,
                                              VkFormat ColourFormat);

    void Upload(const WorkspaceSceneTriangle* Triangles, std::uint32_t TriangleCount);
    void Record(VkCommandBuffer Command, const WorkspaceSceneProjection& Projection,
                float ClipX0, float ClipY0, float ClipX1, float ClipY1);

    bool Standing() const { return DeviceEdge != nullptr; }
    std::uint32_t TriangleCount() const { return static_cast<std::uint32_t>(UploadedTriangles.size()); }

    void Reclaim();

private:
    const VulkanExchange* DeviceEdge = nullptr;
    const DiagnosticExtension* NamingEdge = nullptr;
    VkFormat TargetFormat = VK_FORMAT_UNDEFINED;
    std::vector<WorkspaceSceneTriangle> UploadedTriangles = {};
};

} // namespace Slate
