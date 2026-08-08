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

Outcome<bool> InterfaceExchange::Construct(const InterfaceAttachment& Arriving)
{
    if (ContextSlot != nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the interface context already exists" });

    if (Arriving.Instance         == VK_NULL_HANDLE ||
        Arriving.ScoredDevice     == VK_NULL_HANDLE ||
        Arriving.ActiveDevice     == VK_NULL_HANDLE ||
        Arriving.GraphicsQueue    == VK_NULL_HANDLE ||
        Arriving.NativeWindowSlot == nullptr)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::CapabilityAbsent, "a required device or window handle was absent" });
    }

    if (Arriving.ColourTargetFormat == VK_FORMAT_UNDEFINED)
    {
        return Outcome<bool>::Refuse(
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
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the interface descriptor extent was refused" });
    }

    IMGUI_CHECKVERSION();
    ImGuiContext* ConstructedContext = ImGui::CreateContext();

    if (ConstructedContext == nullptr)
    {
        vkDestroyDescriptorPool(Attached.ActiveDevice, DescriptorSlot, nullptr);
        DescriptorSlot = VK_NULL_HANDLE;
        Attached       = {};
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the interface context was not constructed" });
    }

    ContextSlot = static_cast<void*>(ConstructedContext);

    ImGui::StyleColorsDark();

    // 📝 The window system attachment installs its own callbacks. `04`'s `InputExchange` keeps its arrival
    //    stamps regardless: the interface reads the accumulated window condition, and the stroke path reads
    //    the stamped arrival ordering. They observe the same device through two surfaces that never merge.
    if (!ImGui_ImplGlfw_InitForVulkan(static_cast<GLFWwindow*>(Attached.NativeWindowSlot), true))
    {
        Reclaim();
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the window system attachment declined" });
    }

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
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the vendor attachment declined" });
    }

    return Outcome<bool>::Deliver(true);
}

void InterfaceExchange::Reclaim()
{
    if (ContextSlot == nullptr)
        return;

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));

    if (Attached.ActiveDevice != VK_NULL_HANDLE)
        vkDeviceWaitIdle(Attached.ActiveDevice);

    ImGui_ImplVulkan_Shutdown();
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
    Attached         = {};
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE TICK
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> InterfaceExchange::Advance()
{
    if (ContextSlot == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "no interface context is constructed" });

    if (TickOpen)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "a tick is already open" });

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    TickOpen         = true;
    ContentAssembled = false;

    return Outcome<bool>::Deliver(true);
}

Outcome<bool> InterfaceExchange::Seal()
{
    if (!TickOpen)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "no tick is open" });

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));
    ImGui::Render();

    TickOpen         = false;
    ContentAssembled = true;

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECORDING
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> InterfaceExchange::Record(VkCommandBuffer CommandRecording)
{
    if (!ContentAssembled)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "nothing has been sealed for recording" });

    if (CommandRecording == VK_NULL_HANDLE)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "no command recording was supplied" });

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));

    ImDrawData* AssembledContent = ImGui::GetDrawData();

    if (AssembledContent == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the sealed content was not assembled" });

    // 📝 A display extent of zero — a minimised window — assembles content that covers nothing. Recording it
    //    is legal and costs a recorded nothing; skipping it here keeps the recording out of the rotation's
    //    measured duration entirely.
    if (AssembledContent->DisplaySize.x <= 0.0f || AssembledContent->DisplaySize.y <= 0.0f)
        return Outcome<bool>::Deliver(true);

    ImGui_ImplVulkan_RenderDrawData(AssembledContent, CommandRecording);

    return Outcome<bool>::Deliver(true);
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
