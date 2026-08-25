//============================================================================================================================================
//                                                       WORKSPACESCENEPASS.CPP
//============================================================================================================================================

#include "SlateVulkan/Device/WorkspaceScenePass/Api/WorkspaceScenePass.h"

namespace Slate
{

WorkspaceScenePass::~WorkspaceScenePass()
{
    Reclaim();
}

Outcome<bool> WorkspaceScenePass::ConstructWorkspaceScenePass(const VulkanExchange& Exchange,
                                                              const DiagnosticExtension& Naming,
                                                              ShaderCodec& Streams,
                                                              VkFormat ColourFormat)
{
    (void)Streams;
    if (DeviceEdge != nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "a scene pass construction already stands" });
    if (Exchange.ActiveDevice() == VK_NULL_HANDLE || Exchange.GraphicsQueue() == VK_NULL_HANDLE)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no device is active" });
    DeviceEdge = &Exchange;
    NamingEdge = &Naming;
    TargetFormat = ColourFormat;
    return Outcome<bool>::Result(true);
}

void WorkspaceScenePass::Upload(const WorkspaceSceneTriangle* Triangles, std::uint32_t TriangleCount)
{
    UploadedTriangles.clear();
    if (Triangles == nullptr || TriangleCount == 0u)
        return;
    UploadedTriangles.assign(Triangles, Triangles + TriangleCount);
}

void WorkspaceScenePass::Record(VkCommandBuffer Command, const WorkspaceSceneProjection& Projection,
                                float ClipX0, float ClipY0, float ClipX1, float ClipY1)
{
    (void)Command;
    (void)Projection;
    (void)ClipX0;
    (void)ClipY0;
    (void)ClipX1;
    (void)ClipY1;
    // The MVP keeps the scene upload model and pass boundary separate from CAD/overlay. Pipeline creation follows
    // the same seam as WorkspaceCadPass once the scene shaders land, while hosts retain the CPU fallback meanwhile.
}

void WorkspaceScenePass::Reclaim()
{
    UploadedTriangles.clear();
    DeviceEdge = nullptr;
    NamingEdge = nullptr;
    TargetFormat = VK_FORMAT_UNDEFINED;
}

} // namespace Slate
