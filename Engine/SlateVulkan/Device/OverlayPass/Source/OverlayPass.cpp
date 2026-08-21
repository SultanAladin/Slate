//============================================================================================================================================
//                                                               OVERLAYPASS.CPP
//============================================================================================================================================

#include "SlateVulkan/Device/OverlayPass/Api/OverlayPass.h"

namespace Slate
{

namespace
{

// 📐 The shader's record shapes, mirroring `OverlayVertex.slang` exactly — the byte offsets are the
//    contract, so each record is written here and read there, and the alignment assertions below are
//    what keep the two from drifting.
struct LineRecord
{
    float X0, Y0;             // [px]
    float X1, Y1;             // [px]
    float Thickness;          // [px]
    float Padding[3];         // [-] - alignment to 48 bytes
    float R, G, B, A;         // [-] - straight alpha, 0..1
};

struct DotRecord
{
    float X, Y;               // [px]
    float Radius;             // [px]
    float Padding;            // [-] - alignment
    float R, G, B, A;         // [-] - straight alpha, 0..1
};

struct TriangleRecord
{
    float X0, Y0;             // [px]
    float X1, Y1;             // [px]
    float X2, Y2;             // [px]
    float P0, P1;             // [-] - alignment
    float R, G, B, A;         // [-] - straight alpha, 0..1
};

static_assert(sizeof(LineRecord)     % 16u == 0u, "the line record must align to 16 bytes");
static_assert(sizeof(DotRecord)      % 16u == 0u, "the dot record must align to 16 bytes");
static_assert(sizeof(TriangleRecord) % 16u == 0u, "the triangle record must align to 16 bytes");

constexpr std::uint32_t OverlayPushConstantBytes = 16u;   // [B] - mode, width, height, padding

}   // namespace

OverlayPass::~OverlayPass()
{
    Reclaim();
}

Outcome<bool> OverlayPass::Construct(const VulkanExchange&      Exchange,
                                     const DiagnosticExtension& Naming,
                                     ShaderCodec&               Streams,
                                     VkFormat                   ColourFormat)
{
    if (DeviceEdge != nullptr)
        return Outcome<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "an overlay pass construction already stands" });

    const VkDevice Active = Exchange.ActiveDevice();

    if (Active == VK_NULL_HANDLE || Exchange.GraphicsQueue() == VK_NULL_HANDLE)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no device is active" });

    DeviceEdge = &Exchange;
    NamingEdge = &Naming;

    LineBytes     = LineCapacity     * static_cast<std::uint32_t>(sizeof(LineRecord));
    DotBytes      = DotCapacity      * static_cast<std::uint32_t>(sizeof(DotRecord));
    TriangleBytes = TriangleCapacity * static_cast<std::uint32_t>(sizeof(TriangleRecord));

    // ① The two stages, resolved through the shader codec's own SPIR-V reading. A stream the build
    //    never lowered (the sandbox) refuses here, and the pass simply does not stand — the host
    //    reports it and runs without the overlay.
    const Outcome<std::uint32_t> VertexModule =
        Streams.Resolve("SlateVulkan", "OverlayVertex");

    if (!VertexModule.Resolved)
    {
        Reclaim();
        return Outcome<bool>::Refuse(VertexModule.Error);
    }

    const Outcome<std::uint32_t> FragmentModule =
        Streams.Resolve("SlateVulkan", "OverlayFragment");

    if (!FragmentModule.Resolved)
    {
        Reclaim();
        return Outcome<bool>::Refuse(FragmentModule.Error);
    }

    const Outcome<VkPipelineShaderStageCreateInfo> VertexRead =
        Streams.Stage(VertexModule.Resolve(), VK_SHADER_STAGE_VERTEX_BIT, {});

    if (!VertexRead.Resolved)
    {
        Reclaim();
        return Outcome<bool>::Refuse(VertexRead.Error);
    }

    const Outcome<VkPipelineShaderStageCreateInfo> FragmentRead =
        Streams.Stage(FragmentModule.Resolve(), VK_SHADER_STAGE_FRAGMENT_BIT, {});

    if (!FragmentRead.Resolved)
    {
        Reclaim();
        return Outcome<bool>::Refuse(FragmentRead.Error);
    }

    // ② The descriptor set layout: the three record regions, read by the vertex stage as structured
    //    pools. No uniform buffer exists — the viewport size rides the push constant.
    VkDescriptorSetLayoutBinding Bindings[3] = {};

    for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
    {
        Bindings[Ordinal].binding            = Ordinal;
        Bindings[Ordinal].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        Bindings[Ordinal].descriptorCount    = 1u;
        Bindings[Ordinal].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
        Bindings[Ordinal].pImmutableSamplers = nullptr;
    }

    VkDescriptorSetLayoutCreateInfo LayoutDeclaration = {};
    LayoutDeclaration.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    LayoutDeclaration.bindingCount = 3u;
    LayoutDeclaration.pBindings    = Bindings;

    if (vkCreateDescriptorSetLayout(Active, &LayoutDeclaration, nullptr, &OverlayLayout) != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the overlay layout was rejected" });
    }

    VkDescriptorPoolSize PoolSize = {};
    PoolSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    PoolSize.descriptorCount = 3u;

    VkDescriptorPoolCreateInfo PoolDeclaration = {};
    PoolDeclaration.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolDeclaration.maxSets       = 1u;
    PoolDeclaration.poolSizeCount = 1u;
    PoolDeclaration.pPoolSizes    = &PoolSize;

    if (vkCreateDescriptorPool(Active, &PoolDeclaration, nullptr, &OverlayPool) != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the overlay pool was rejected" });
    }

    VkDescriptorSetAllocateInfo SetDeclaration = {};
    SetDeclaration.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    SetDeclaration.descriptorPool     = OverlayPool;
    SetDeclaration.descriptorSetCount = 1u;
    SetDeclaration.pSetLayouts        = &OverlayLayout;

    if (vkAllocateDescriptorSets(Active, &SetDeclaration, &OverlaySet) != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the overlay set was rejected" });
    }

    // ③ The one vertex buffer, host-visible and coherent, mapped once — the upload is a memcpy.
    VkBufferCreateInfo BufferDeclaration = {};
    BufferDeclaration.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferDeclaration.size        = static_cast<VkDeviceSize>(LineBytes) + DotBytes + TriangleBytes;
    BufferDeclaration.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    BufferDeclaration.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(Active, &BufferDeclaration, nullptr, &VertexBuffer) != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the overlay extent was rejected" });
    }

    VkMemoryRequirements MemoryRequirements = {};
    vkGetBufferMemoryRequirements(Active, VertexBuffer, &MemoryRequirements);

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
            (Candidate.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0u &&
            (Candidate.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0u)
        {
            Allocation.memoryTypeIndex = Ordinal;
            MemoryTypeFound = true;
            break;
        }
    }

    if (!MemoryTypeFound || vkAllocateMemory(Active, &Allocation, nullptr, &VertexMemory) != VK_SUCCESS ||
        vkBindBufferMemory(Active, VertexBuffer, VertexMemory, 0u) != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the overlay memory was rejected" });
    }

    const VkResult Mapped =
        vkMapMemory(Active, VertexMemory, 0u, VK_WHOLE_SIZE, 0u,
                    reinterpret_cast<void**>(&MappedSlot));

    if (Mapped != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the overlay extent would not map" });
    }

    // ④ The set writes the three regions of the one buffer.
    VkDescriptorBufferInfo Region[3] = {};

    Region[0].buffer = VertexBuffer;
    Region[0].offset = 0u;
    Region[0].range  = LineBytes;
    Region[1].buffer = VertexBuffer;
    Region[1].offset = LineBytes;
    Region[1].range  = DotBytes;
    Region[2].buffer = VertexBuffer;
    Region[2].offset = LineBytes + DotBytes;
    Region[2].range  = TriangleBytes;

    VkWriteDescriptorSet Writes[3] = {};

    for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
    {
        Writes[Ordinal].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        Writes[Ordinal].dstSet           = OverlaySet;
        Writes[Ordinal].dstBinding       = Ordinal;
        Writes[Ordinal].descriptorCount  = 1u;
        Writes[Ordinal].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        Writes[Ordinal].pBufferInfo      = &Region[Ordinal];
    }

    vkUpdateDescriptorSets(Active, 3u, Writes, 0u, nullptr);

    // ⑤ The pipeline layout: the set and the 16-byte push constant (mode + display size).
    VkPushConstantRange PushRange = {};
    PushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    PushRange.offset     = 0u;
    PushRange.size       = OverlayPushConstantBytes;

    VkPipelineLayoutCreateInfo LayoutDeclaration2 = {};
    LayoutDeclaration2.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    LayoutDeclaration2.setLayoutCount         = 1u;
    LayoutDeclaration2.pSetLayouts            = &OverlayLayout;
    LayoutDeclaration2.pushConstantRangeCount = 1u;
    LayoutDeclaration2.pPushConstantRanges    = &PushRange;

    if (vkCreatePipelineLayout(Active, &LayoutDeclaration2, nullptr, &OverlayPipelineLayout) != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the overlay program layout was rejected" });
    }

    // ⑥ The program — dynamic rendering, so the colour attachment rides the pipeline-rendering info
    //    and no render construct stands. Straight-alpha blend; no depth; no cull.
    VkPipelineRenderingCreateInfo Rendering = {};
    Rendering.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    Rendering.colorAttachmentCount    = 1u;
    Rendering.pColorAttachmentFormats = &ColourFormat;

    VkPipelineShaderStageCreateInfo Stages[2] = { VertexRead.Resolve(), FragmentRead.Resolve() };

    VkPipelineVertexInputStateCreateInfo VertexInput = {};
    VertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    // 🔴 No vertex input is declared and none may be: every vertex is synthesised from `SV_VertexID`
    //    and the structured pools, so an input declaration would be a layout the draw never satisfies.

    VkPipelineInputAssemblyStateCreateInfo Assembly = {};
    Assembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    Assembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    Assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo ViewportState = {};
    ViewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportState.viewportCount = 1u;
    ViewportState.scissorCount  = 1u;

    VkPipelineRasterizationStateCreateInfo Raster = {};
    Raster.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    Raster.depthClampEnable        = VK_FALSE;
    Raster.rasterizerDiscardEnable = VK_FALSE;
    Raster.polygonMode             = VK_POLYGON_MODE_FILL;
    Raster.cullMode                = VK_CULL_MODE_NONE;
    Raster.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    Raster.depthBiasEnable         = VK_FALSE;
    Raster.lineWidth               = 1.0f;

    VkPipelineMultisampleStateCreateInfo Multisample = {};
    Multisample.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    Multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState Blend = {};
    Blend.blendEnable         = VK_TRUE;
    // 🔴 Straight alpha — `src_alpha / one_minus_src_alpha` — never the premultiplied
    //    `one / one_minus_src_alpha` the interface uses. The premultiplied read is what washed a
    //    low-alpha line out over a bright sky; straight alpha keeps the hue at the declared coverage.
    Blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    Blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    Blend.colorBlendOp        = VK_BLEND_OP_ADD;
    Blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    Blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    Blend.alphaBlendOp        = VK_BLEND_OP_ADD;
    Blend.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                             | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo ColourBlend = {};
    ColourBlend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColourBlend.attachmentCount = 1u;
    ColourBlend.pAttachments    = &Blend;

    VkDynamicState Dynamic[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo DynamicState = {};
    DynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicState.dynamicStateCount = 2u;
    DynamicState.pDynamicStates    = Dynamic;

    VkGraphicsPipelineCreateInfo PipelineDeclaration = {};
    PipelineDeclaration.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineDeclaration.pNext               = &Rendering;
    PipelineDeclaration.stageCount          = 2u;
    PipelineDeclaration.pStages             = Stages;
    PipelineDeclaration.pVertexInputState   = &VertexInput;
    PipelineDeclaration.pInputAssemblyState = &Assembly;
    PipelineDeclaration.pViewportState      = &ViewportState;
    PipelineDeclaration.pRasterizationState = &Raster;
    PipelineDeclaration.pMultisampleState   = &Multisample;
    PipelineDeclaration.pColorBlendState    = &ColourBlend;
    PipelineDeclaration.pDynamicState       = &DynamicState;
    PipelineDeclaration.layout              = OverlayPipelineLayout;
    PipelineDeclaration.renderPass          = VK_NULL_HANDLE;   // [-] - dynamic rendering declares no construct
    PipelineDeclaration.subpass             = 0u;

    const VkResult Constructed =
        vkCreateGraphicsPipelines(Active, VK_NULL_HANDLE, 1u, &PipelineDeclaration, nullptr,
                                  &OverlayPipeline);

    if (Constructed != VK_SUCCESS)
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the overlay program was rejected" });
    }

    if (Naming.Declare(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<std::uint64_t>(OverlayPipeline),
                       "OverlayPass.Pipeline") &&
        Naming.Declare(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<std::uint64_t>(VertexBuffer),
                       "OverlayPass.Geometry"))
    {
        // 📝 Every object named; nothing else to do with the outcome.
    }

    return Outcome<bool>::Result(true);
}

void OverlayPass::Upload(const OverlayGeometry& Overlay)
{
    if (DeviceEdge == nullptr || MappedSlot == nullptr)
        return;

    // 📐 The conversion is a straight field-for-field copy: the panel's 24-byte line, 16-byte dot and
    //    28-byte triangle become the shader's aligned 40/32/48-byte records. The alignment fills are
    //    written zero so a driver that reads them reads defined bytes.
    std::uint8_t* Cursor = MappedSlot;

    for (std::uint32_t Ordinal = 0u; Ordinal < Overlay.LineCount && Ordinal < LineCapacity; ++Ordinal)
    {
        const OverlayLine& Source = Overlay.Lines[Ordinal];
        LineRecord& Written = *reinterpret_cast<LineRecord*>(Cursor);

        Written.X0 = Source.X0;
        Written.Y0 = Source.Y0;
        Written.X1 = Source.X1;
        Written.Y1 = Source.Y1;
        Written.Thickness = Source.Thickness;
        Written.Padding[0] = 0.0f;
        Written.Padding[1] = 0.0f;
        Written.Padding[2] = 0.0f;
        Written.R = static_cast<float>((Source.Packed >> 16u) & 0xFFu) / 255.0f;
        Written.G = static_cast<float>((Source.Packed >> 8u)  & 0xFFu) / 255.0f;
        Written.B = static_cast<float>((Source.Packed >> 0u)  & 0xFFu) / 255.0f;
        Written.A = static_cast<float>((Source.Packed >> 24u) & 0xFFu) / 255.0f;

        Cursor += sizeof(LineRecord);
    }

    Cursor = MappedSlot + LineBytes;

    for (std::uint32_t Ordinal = 0u; Ordinal < Overlay.DotCount && Ordinal < DotCapacity; ++Ordinal)
    {
        const OverlayDot& Source = Overlay.Dots[Ordinal];
        DotRecord& Written = *reinterpret_cast<DotRecord*>(Cursor);

        Written.X = Source.X;
        Written.Y = Source.Y;
        Written.Radius = Source.Radius;
        Written.Padding = 0.0f;
        Written.R = static_cast<float>((Source.Packed >> 16u) & 0xFFu) / 255.0f;
        Written.G = static_cast<float>((Source.Packed >> 8u)  & 0xFFu) / 255.0f;
        Written.B = static_cast<float>((Source.Packed >> 0u)  & 0xFFu) / 255.0f;
        Written.A = static_cast<float>((Source.Packed >> 24u) & 0xFFu) / 255.0f;

        Cursor += sizeof(DotRecord);
    }

    Cursor = MappedSlot + LineBytes + DotBytes;

    for (std::uint32_t Ordinal = 0u; Ordinal < Overlay.TriangleCount && Ordinal < TriangleCapacity; ++Ordinal)
    {
        const OverlayTriangle& Source = Overlay.Triangles[Ordinal];
        TriangleRecord& Written = *reinterpret_cast<TriangleRecord*>(Cursor);

        Written.X0 = Source.X0;
        Written.Y0 = Source.Y0;
        Written.X1 = Source.X1;
        Written.Y1 = Source.Y1;
        Written.X2 = Source.X2;
        Written.Y2 = Source.Y2;
        Written.P0 = 0.0f;
        Written.P1 = 0.0f;
        Written.R = static_cast<float>((Source.Packed >> 16u) & 0xFFu) / 255.0f;
        Written.G = static_cast<float>((Source.Packed >> 8u)  & 0xFFu) / 255.0f;
        Written.B = static_cast<float>((Source.Packed >> 0u)  & 0xFFu) / 255.0f;
        Written.A = static_cast<float>((Source.Packed >> 24u) & 0xFFu) / 255.0f;

        Cursor += sizeof(TriangleRecord);
    }

    OverlayLineCount     = Overlay.LineCount;
    OverlayDotCount      = Overlay.DotCount;
    OverlayTriangleCount = Overlay.TriangleCount;
}

void OverlayPass::Record(VkCommandBuffer Command, std::uint32_t Width, std::uint32_t Height)
{
    if (DeviceEdge == nullptr || OverlayPipeline == VK_NULL_HANDLE || Command == VK_NULL_HANDLE ||
        Width == 0u || Height == 0u)
        return;

    const VkDevice Active = DeviceEdge->ActiveDevice();

    vkCmdBindPipeline(Command, VK_PIPELINE_BIND_POINT_GRAPHICS, OverlayPipeline);

    const VkViewport Viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = static_cast<float>(Width),
        .height   = static_cast<float>(Height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    const VkRect2D Scissor = {
        .offset = { 0, 0 },
        .extent = { Width, Height }
    };

    vkCmdSetViewport(Command, 0u, 1u, &Viewport);
    vkCmdSetScissor(Command, 0u, 1u, &Scissor);
    vkCmdBindDescriptorSets(Command, VK_PIPELINE_BIND_POINT_GRAPHICS, OverlayPipelineLayout,
                            0u, 1u, &OverlaySet, 0u, nullptr);

    // 📐 The push constant rides the draw: mode 0 lines, 1 dots, 2 triangles; the display size in the
    //    same block is what the vertex stage transforms against.
    struct PushBlock
    {
        std::uint32_t Mode;
        float         DisplayWidth;
        float         DisplayHeight;
        float         Padding;
    };

    const auto DrawMode = [&](std::uint32_t Mode, std::uint32_t Count, std::uint32_t VerticesPerRecord)
    {
        if (Count == 0u)
            return;

        const PushBlock Push = { Mode, static_cast<float>(Width), static_cast<float>(Height), 0.0f };

        vkCmdPushConstants(Command, OverlayPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0u, OverlayPushConstantBytes, &Push);
        vkCmdDraw(Command, Count * VerticesPerRecord, 1u, 0u, 0u);
    };

    DrawMode(0u, OverlayLineCount, 4u);
    DrawMode(1u, OverlayDotCount, 4u);
    DrawMode(2u, OverlayTriangleCount, 3u);
}

void OverlayPass::Reclaim()
{
    if (DeviceEdge == nullptr)
        return;

    const VkDevice Active = DeviceEdge->ActiveDevice();

    if (Active == VK_NULL_HANDLE)
    {
        DeviceEdge = nullptr;
        NamingEdge = nullptr;
        return;
    }

    if (MappedSlot != nullptr)
    {
        vkUnmapMemory(Active, VertexMemory);
        MappedSlot = nullptr;
    }

    if (OverlayPipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(Active, OverlayPipeline, nullptr);
    if (OverlayPipelineLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(Active, OverlayPipelineLayout, nullptr);
    if (OverlayPool != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(Active, OverlayPool, nullptr);
    if (OverlayLayout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(Active, OverlayLayout, nullptr);
    if (VertexMemory != VK_NULL_HANDLE)
        vkFreeMemory(Active, VertexMemory, nullptr);
    if (VertexBuffer != VK_NULL_HANDLE)
        vkDestroyBuffer(Active, VertexBuffer, nullptr);

    DeviceEdge            = nullptr;
    NamingEdge            = nullptr;
    VertexBuffer          = VK_NULL_HANDLE;
    VertexMemory          = VK_NULL_HANDLE;
    MappedSlot            = nullptr;
    OverlayLayout         = VK_NULL_HANDLE;
    OverlayPool           = VK_NULL_HANDLE;
    OverlaySet            = VK_NULL_HANDLE;
    OverlayPipelineLayout = VK_NULL_HANDLE;
    OverlayPipeline       = VK_NULL_HANDLE;
    LineBytes             = 0u;
    DotBytes              = 0u;
    TriangleBytes         = 0u;
    OverlayLineCount     = 0u;
    OverlayDotCount      = 0u;
    OverlayTriangleCount = 0u;
}

} // namespace Slate
