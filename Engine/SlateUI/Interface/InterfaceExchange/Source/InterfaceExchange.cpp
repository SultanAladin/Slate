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
    // 📝 ImGui 19281 replaced VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER with two separate
    //    descriptor types. The pool must carry both:
    //      - SAMPLED_IMAGE  : one set per texture registered via ImGui_ImplVulkan_AddTexture().
    //      - SAMPLER        : one set per built-in sampler (linear + nearest = 2 minimum).
    //    InterfaceSamplerCapacity matches IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE (2)
    //    without pulling imgui_impl_vulkan.h into this translation unit.
    constexpr std::uint32_t InterfaceDescriptorCapacity = 64u;   // [-] sampled-image sets
    constexpr std::uint32_t InterfaceSamplerCapacity    = 2u;    // [-] sampler sets (linear + nearest)
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

    if (Arriving.MinimumDisplayImageCount < 2u ||
        Arriving.DisplayImageCount < Arriving.MinimumDisplayImageCount)
    {
        return Deliver<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "the display image counts are inconsistent" });
    }

    Attached = Arriving;

    // 📝 ImGui 19281 allocates SAMPLED_IMAGE sets for textures and SAMPLER sets for its two
    //    built-in samplers. A pool that carries only COMBINED_IMAGE_SAMPLER has neither type and
    //    the first allocation fails with VK_ERROR_OUT_OF_POOL_MEMORY at validation time.
    const VkDescriptorPoolSize DescriptorExtent[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, InterfaceDescriptorCapacity },
        { VK_DESCRIPTOR_TYPE_SAMPLER,       InterfaceSamplerCapacity    },
    };

    VkDescriptorPoolCreateInfo DescriptorDeclaration = {};
    DescriptorDeclaration.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorDeclaration.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    DescriptorDeclaration.maxSets       = InterfaceDescriptorCapacity + InterfaceSamplerCapacity;
    DescriptorDeclaration.poolSizeCount = 2u;
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

    // 🔴 Docking, enabled here and nowhere else. The submodule stands on ImGui's `docking` branch and the
    //    whole feature is inert until this flag is raised — the branch carries the code, not the behaviour.
    // 📝 ⚠️ Multi-viewport is deliberately NOT raised. It hands panels to the window manager as real OS
    //    windows, each with its own swapchain, and every swapchain in this process belongs to
    //    `DisplayScheduler` against one surface. Raising it would stand a second, unowned chain outside
    //    `HostLifecycle`'s five lifetimes and outside its resize and rebuild paths both.
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

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
    VendorAttachment.MinImageCount                                  = Attached.MinimumDisplayImageCount;
    VendorAttachment.ImageCount                                     = Attached.DisplayImageCount;
    VendorAttachment.UseDynamicRendering                            = true;
    VendorAttachment.PipelineInfoMain.PipelineRenderingCreateInfo   = RecordingDeclaration;

    if (!ImGui_ImplVulkan_Init(&VendorAttachment))
    {
        Reclaim();
        return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "the vendor attachment declined" });
    }

    VendorAttached = true;

    // 🔴 The retained workspace seat, re-applied. `ImGui::CreateContext` above begins at the vendor's
    //    defaults and `StyleColorsDark` overwrites the lot — so a style seated once at bring-up was lost on
    //    every device rebuild, and the trapezoidal tabs reverted to stock rectangles.
    if (StyleSeated)
        Disregard(SeatWorkspaceStyle(SeatedMeasure, SeatedInk));

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

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE WORKSPACE STYLE
//------------------------------------------------------------------------------------------------------------------------

namespace
{

/// 🧩 One Slate ink as the vendor's four unit ordinates.
/// cost  ✔️
ImVec4 Vendor(InkOrdinate Ink)
{
    return ImVec4(static_cast<float>(Ink.Red)     / 255.0f,
                  static_cast<float>(Ink.Green)   / 255.0f,
                  static_cast<float>(Ink.Blue)    / 255.0f,
                  static_cast<float>(Ink.Opacity) / 255.0f);
}

}   // namespace

Deliver<bool> InterfaceExchange::SeatWorkspaceStyle(const WorkspaceMetric& Measure, const WorkspaceInk& Tinted)
{
    // 🔴 Retained BEFORE the context is tested, so a seat asked for while no context stands is applied by
    //    the next Construct rather than silently dropped.
    SeatedMeasure = Measure;
    SeatedInk     = Tinted;
    StyleSeated   = true;

    if (ContextSlot == nullptr)
        return Deliver<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no interface context stands" });

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));

    ImGuiStyle& Seated = ImGui::GetStyle();

    // 🔴 The four members `Patches/` adds. Each defaults to 0.0f, at which a patched build rasterises
    //    byte-identically to an unpatched one — so seating them here is what turns the sheet's trapezoid on.
    Seated.TabSlant       = Measure.TabSlant;
    Seated.TabOverlap     = Measure.TabOverlap;
    Seated.TabHeight      = Measure.TabAcross;
    Seated.TabStripPadTop = Measure.StripPadTop;

    // ⚠️ Coupled with TabOverlap: the sheet's 38 px padding exists to clear the slant plus the overlap.
    Seated.FramePadding.x = Measure.TabPadAlong;

    // 📝 The sheet rounds nothing and strokes nothing. Both configurable, both zero here, which is what
    //    `DockWorkspace.html` states — `roundCorners` is off and no tab carries a border.
    Seated.TabRounding   = Measure.TabRadius;
    Seated.TabBorderSize = Measure.TabEdgeWeight;

    Seated.Colors[ImGuiCol_Tab]               = Vendor(Tinted.TabQuiet);
    Seated.Colors[ImGuiCol_TabHovered]        = Vendor(Tinted.TabRoused);
    Seated.Colors[ImGuiCol_TabSelected]       = Vendor(Tinted.TabTaken);
    Seated.Colors[ImGuiCol_TabDimmed]         = Vendor(Tinted.TabQuiet);
    Seated.Colors[ImGuiCol_TabDimmedSelected] = Vendor(Tinted.TabTaken);
    Seated.Colors[ImGuiCol_Text]              = Vendor(Tinted.TabInkTaken);

    return Deliver<bool>::Deliver(true);
}

void InterfaceExchange::RecordWorkspaceTabs(const PlaneExtent&  Extent,
                                            const char* const*  Titles,
                                            std::uint32_t       Count,
                                            std::uint32_t       Active,
                                            std::uint32_t&      Chosen,
                                            std::uint32_t&      Closed,
                                            bool&               Enrolling)
{
    // 📝 Both outputs are settled before any early return. `Count` means "none", which no valid
    //    ordinal equals.
    Chosen    = Count;
    Closed    = Count;
    Enrolling = false;

    // 🔴 A zero count is NOT an early return. The `+` must be recorded on an empty strip too, or an artist
    //    who closed the last workspace is left in a state with no way out.
    if (ContextSlot == nullptr || !TickOpen)
        return;

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));

    // 🔴 A bare window seated exactly on the strip the panel measured. The tab bar has to live in a
    //    window: the vendor owns tab layout, hover and drag arbitration, and a bar recorded into a plain
    //    draw list gets none of it.
    ImGui::SetNextWindowPos(ImVec2(Extent.LeastAlong, Extent.LeastAcross));
    ImGui::SetNextWindowSize(ImVec2(Extent.SpanAlong(), Extent.SpanAcross()));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    const ImGuiWindowFlags Bare = ImGuiWindowFlags_NoTitleBar
                                | ImGuiWindowFlags_NoResize
                                | ImGuiWindowFlags_NoMove
                                | ImGuiWindowFlags_NoScrollbar
                                | ImGuiWindowFlags_NoScrollWithMouse
                                | ImGuiWindowFlags_NoSavedSettings
                                | ImGuiWindowFlags_NoBringToFrontOnFocus
                                | ImGuiWindowFlags_NoBackground;

    if (ImGui::Begin("SlateWorkspaceStrip", nullptr, Bare))
    {
        if (ImGui::BeginTabBar("SlateWorkspaceTabs", ImGuiTabBarFlags_Reorderable
                                                   | ImGuiTabBarFlags_FittingPolicyScroll))
        {
            for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
            {
                if (Titles[Ordinal] == nullptr)
                    continue;

                // 📝 The vendor is TOLD which tab is presented rather than left to remember. The ledger
                //    owns that decision, and a bar keeping its own would disagree the first time anything
                //    but a click changed the active workspace.
                ImGuiTabItemFlags Seated = ImGuiTabItemFlags_None;

                if (Ordinal == Active)
                    Seated |= ImGuiTabItemFlags_SetSelected;

                bool Standing = true;

                if (ImGui::BeginTabItem(Titles[Ordinal], &Standing, Seated))
                {
                    Chosen = Ordinal;
                    ImGui::EndTabItem();
                }

                // ⚠️ Reported, never acted on here. Withdrawing inside this sweep would edit the set the
                //    tab bar is walking; the caller withdraws it once the strip is sealed.
                if (!Standing)
                    Closed = Ordinal;
            }

            // 📝 The sheet's `addBtn`, recorded as a trailing tab-bar button so the vendor lays it after the
            //    last tab and it scrolls with the strip. `TabItemButton` is not a tab: it carries no
            //    selection and `ImGuiTabItemFlags_Button` keeps PatchA's slant off it, which is what makes
            //    it read as a control rather than as an empty workspace.
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
                Enrolling = true;

            ImGui::EndTabBar();
        }
    }

    ImGui::End();

    ImGui::PopStyleVar(2);
}

void InterfaceExchange::RecordDockSpace(const PlaneExtent& Extent)
{
    if (ContextSlot == nullptr || !TickOpen)
        return;

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));

    ImGui::SetNextWindowPos(ImVec2(Extent.LeastAlong, Extent.LeastAcross));
    ImGui::SetNextWindowSize(ImVec2(Extent.SpanAlong(), Extent.SpanAcross()));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    const ImGuiWindowFlags Bare = ImGuiWindowFlags_NoTitleBar
                                | ImGuiWindowFlags_NoResize
                                | ImGuiWindowFlags_NoMove
                                | ImGuiWindowFlags_NoScrollbar
                                | ImGuiWindowFlags_NoScrollWithMouse
                                | ImGuiWindowFlags_NoSavedSettings
                                | ImGuiWindowFlags_NoBringToFrontOnFocus
                                | ImGuiWindowFlags_NoNavFocus
                                | ImGuiWindowFlags_NoBackground;

    if (ImGui::Begin("SlateWorkspaceBody", nullptr, Bare))
    {
        // 🔴 `PassthruCentralNode` so the workspace ground shows through where nothing is docked. Without
        //    it the vendor fills the whole node with its own colour and the sheet's OLED body is lost.
        ImGui::DockSpace(ImGui::GetID("SlateDockSpace"), ImVec2(0.0f, 0.0f),
                         ImGuiDockNodeFlags_PassthruCentralNode);
    }

    ImGui::End();

    ImGui::PopStyleVar(2);
}

Deliver<bool> InterfaceExchange::Renegotiate(std::uint32_t MinimumImageCount, std::uint32_t ImageCount)
{
    if (ContextSlot == nullptr || !VendorAttached)
        return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "no vendor attachment stands" });

    if (MinimumImageCount < 2u || ImageCount < MinimumImageCount)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "the display image counts are inconsistent" });

    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ContextSlot));

    // Dear ImGui 1.92.9 exposes runtime renegotiation of the requested minimum. The actual count is supplied
    //    at construction and retained here beside the new chain count; it is never replaced by Slate's slot count.
    ImGui_ImplVulkan_SetMinImageCount(MinimumImageCount);

    Attached.MinimumDisplayImageCount = MinimumImageCount;
    Attached.DisplayImageCount        = ImageCount;

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
    {
        ContentAssembled = false;
        return Deliver<bool>::Deliver(true);
    }

    ImGui_ImplVulkan_RenderDrawData(AssembledContent, CommandRecording);
    ContentAssembled = false;

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
