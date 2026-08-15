//============================================================================================================================================
//                                                          INTERFACEEXCHANGE.CPP
//============================================================================================================================================
// 🧩 The only translation unit in the engine that includes ImGui.

#include "SlateUI/Interface/InterfaceExchange/Api/InterfaceExchange.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  DESCRIPTOR EXTENT
//------------------------------------------------------------------------------------------------------------------------

// 📝 The interface's own descriptor extent, sized here rather than shared with `06`'s `DescriptorIndex`.
//    The interface allocates for its own imagery and for nothing else, so a shared extent would couple two
//    lifetimes that are reclaimed at different moments.
namespace
{
    constexpr std::uint32_t InterfaceDescriptorCapacity = 64u;   // [-] - sampled images the interface holds
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

InterfaceExchange::~InterfaceExchange()
{
    Reclaim();
}

Deliver<bool> InterfaceExchange::Construct(const InterfaceAttachment& Arriving)
{
    if (ContextSlot != nullptr)
        return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "the interface context already exists" });

    if (Arriving.Instance         == VK_NULL_HANDLE ||
        Arriving.ScoredDevice     == VK_NULL_HANDLE ||
        Arriving.ActiveDevice     == VK_NULL_HANDLE ||
        Arriving.GraphicsQueue    == VK_NULL_HANDLE ||
        Arriving.NativeWindowSlot == nullptr)
    {
        return Deliver<bool>::Refuse(
            { RefusalReason::CapabilityAbsent, "a required device or window handle was absent" });
    }

    if (Arriving.ColourTargetFormat == VK_FORMAT_UNDEFINED)
    {
        return Deliver<bool>::Refuse(
            { RefusalReason::CapabilityAbsent, "no colour target format was declared" });
    }

    Attached = Arriving;

    const VkDescriptorPoolSize DescriptorExtent[] =
    {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, InterfaceDescriptorCapacity }
    };

    VkDescriptorPoolCreateInfo DescriptorDeclaration = {};
    DescriptorDeclaration.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorDeclaration.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    DescriptorDeclaration.maxSets       = InterfaceDescriptorCapacity;
    DescriptorDeclaration.poolSizeCount = 1u;
    DescriptorDeclaration.pPoolSizes    = DescriptorExtent;

    if (vkCreateDescriptorPool(Attached.ActiveDevice, &DescriptorDeclaration, nullptr, &DescriptorSlot) != VK_SUCCESS)
    {
        Attached = {};
        return Deliver<bool>::Refuse({ RefusalReason::ExtentExhausted, "the interface descriptor extent was refused" });
    }

    IMGUI_CHECKVERSION();
    ImGuiContext* ConstructedContext = ImGui::CreateContext();

    if (ConstructedContext == nullptr)
    {
        vkDestroyDescriptorPool(Attached.ActiveDevice, DescriptorSlot, nullptr);
        DescriptorSlot = VK_NULL_HANDLE;
        Attached       = {};
        return Deliver<bool>::Refuse({ RefusalReason::ExtentExhausted, "the interface context was not constructed" });
    }

    ContextSlot = static_cast<void*>(ConstructedContext);

    ImGui::StyleColorsDark();

    // 📝 The window system attachment installs its own callbacks. `04`'s `InputExchange` keeps its arrival
    //    stamps regardless: the interface reads the accumulated window condition, and the stroke path reads
    //    the stamped arrival ordering. They observe the same device through two surfaces that never merge.
    if (!ImGui_ImplGlfw_InitForVulkan(static_cast<GLFWwindow*>(Attached.NativeWindowSlot), true))
    {
        Reclaim();
        return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "the window system attachment declined" });
    }

    WindowAttached = true;

    VkFormat DeclaredColourFormat = Attached.ColourTargetFormat;

    VkPipelineRenderingCreateInfoKHR RecordingDeclaration = {};
    RecordingDeclaration.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    RecordingDeclaration.colorAttachmentCount    = 1u;
    RecordingDeclaration.pColorAttachmentFormats = &DeclaredColourFormat;

    ImGui_ImplVulkan_InitInfo VendorAttachment = {};
    VendorAttachment.ApiVersion                                     = VK_API_VERSION_1_3;
    VendorAttachment.Instance                                       = Attached.Instance;
    VendorAttachment.PhysicalDevice                                 = Attached.ScoredDevice;
    VendorAttachment.Device                                         = Attached.ActiveDevice;
    VendorAttachment.QueueFamily                                    = Attached.GraphicsFamilyOrdinal;
    VendorAttachment.Queue                                          = Attached.GraphicsQueue;
    VendorAttachment.DescriptorPool                                 = DescriptorSlot;
    VendorAttachment.MinImageCount                                  = Attached.RotationDepth;
    VendorAttachment.ImageCount                                     = Attached.RotationDepth;
    VendorAttachment.UseDynamicRendering                            = true;
    VendorAttachment.PipelineInfoMain.PipelineRenderingCreateInfo   = RecordingDeclaration;

    if (!ImGui_ImplVulkan_Init(&VendorAttachment))
    {
        Reclaim();
        return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "the vendor attachment declined" });
    }

    VendorAttached = true;

    return Deliver<bool>::Deliver(true);
}

void InterfaceExchange::Reclaim()
{
    if (ContextSlot == nullptr)
        return;

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));

    if (Attached.ActiveDevice != VK_NULL_HANDLE)
        vkDeviceWaitIdle(Attached.ActiveDevice);

    // 📝 🔴 Each shutdown is gated on its own attachment having stood. The vendor shutdown asserts on a
    //    backend that was never initialised, so the failure path out of Construct used to abort the
    //    process instead of reporting the refusal it had already built.
    if (VendorAttached)
        ImGui_ImplVulkan_Shutdown();

    if (WindowAttached)
        ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext(static_cast<ImGuiContext*>(ContextSlot));

    if (DescriptorSlot != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(Attached.ActiveDevice, DescriptorSlot, nullptr);
        DescriptorSlot = VK_NULL_HANDLE;
    }

    ContextSlot      = nullptr;
    TickOpen         = false;
    ContentAssembled = false;
    WindowAttached   = false;
    VendorAttached   = false;
    Attached         = {};
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE TICK
//------------------------------------------------------------------------------------------------------------------------

Deliver<bool> InterfaceExchange::Advance()
{
    if (ContextSlot == nullptr)
        return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "no interface context is constructed" });

    if (TickOpen)
        return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "a tick is already open" });

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    TickOpen         = true;
    ContentAssembled = false;

    return Deliver<bool>::Deliver(true);
}

Deliver<bool> InterfaceExchange::Seal()
{
    if (!TickOpen)
        return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "no tick is open" });

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));
    ImGui::Render();

    TickOpen         = false;
    ContentAssembled = true;

    return Deliver<bool>::Deliver(true);
}

Deliver<bool> InterfaceExchange::Abandon()
{
    if (!TickOpen)
        return Deliver<bool>::Deliver(true);

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));
    ImGui::EndFrame();

    TickOpen         = false;
    ContentAssembled = false;

    return Deliver<bool>::Deliver(true);
}

Deliver<bool> InterfaceExchange::Renegotiate(std::uint32_t RotationDepth)
{
    if (ContextSlot == nullptr || !VendorAttached)
        return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "no vendor attachment stands" });

    if (RotationDepth < 2u)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "a rotation depth below two" });

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));
    ImGui_ImplVulkan_SetMinImageCount(RotationDepth);

    Attached.RotationDepth = RotationDepth;

    return Deliver<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECORDING
//------------------------------------------------------------------------------------------------------------------------

Deliver<bool> InterfaceExchange::Record(VkCommandBuffer CommandRecording)
{
    if (!ContentAssembled)
        return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "nothing has been sealed for recording" });

    if (CommandRecording == VK_NULL_HANDLE)
        return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "no command recording was supplied" });

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));

    ImDrawData* AssembledContent = ImGui::GetDrawData();

    if (AssembledContent == nullptr)
        return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "the sealed content was not assembled" });

    // 📝 A display extent of zero — a minimised window — assembles content that covers nothing. Recording it
    //    is legal and costs a recorded nothing; skipping it here keeps the recording out of the rotation's
    //    measured duration entirely.
    if (AssembledContent->DisplaySize.x <= 0.0f || AssembledContent->DisplaySize.y <= 0.0f)
        return Deliver<bool>::Deliver(true);

    ImGui_ImplVulkan_RenderDrawData(AssembledContent, CommandRecording);

    return Deliver<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       CAPTURE
//------------------------------------------------------------------------------------------------------------------------

// 📝 The capture reads go through SetCurrentContext rather than reaching into the context directly. Only
//    `imgui_internal.h` defines ImGuiContext; `imgui.h` forward-declares it, so a member read here would not
//    compile without dragging the internal header into the seam. The accessor route reads the same two bits.
bool InterfaceExchange::PointerCaptured() const
{
    if (ContextSlot == nullptr)
        return false;

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));

    return ImGui::GetIO().WantCaptureMouse;
}

bool InterfaceExchange::KeyboardCaptured() const
{
    if (ContextSlot == nullptr)
        return false;

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));

    return ImGui::GetIO().WantCaptureKeyboard;
}

}   // namespace Slate
