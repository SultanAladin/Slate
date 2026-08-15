//============================================================================================================================================
//                                                             EDITORHOST.CPP
//============================================================================================================================================
// 🧩 The combined editor — Vulkan bring-up, the tick loop, and the two drawers.

#include "Contract/DeliveryContract.h"
#include "Contract/ToleranceContract.h"
#include "SlateMath/Platform/TickSequence/Api/TickSequence.h"
#include "SlateMath/Platform/WindowInterchange/Api/WindowInterchange.h"
#include "SlateUI/Interface/ViewportSequence/Api/ViewportSequence.h"
#include "SlateVulkan/Device/CommandSequence/Api/CommandSequence.h"
#include "SlateVulkan/Device/CycleScheduler/Api/CycleScheduler.h"
#include "SlateVulkan/Device/DiagnosticExtension/Api/DiagnosticExtension.h"
#include "SlateVulkan/Device/DisplayScheduler/Api/DisplayScheduler.h"
#include "SlateVulkan/Device/VendorClassifier/Api/VendorClassifier.h"
#include "SlateVulkan/Device/VulkanExchange/Api/VulkanExchange.h"
#include "SlateVulkan/Device/WindowExchange/Api/WindowExchange.h"

#include <cstdio>

//------------------------------------------------------------------------------------------------------------------------
//                                                        FIGURES
//------------------------------------------------------------------------------------------------------------------------

namespace
{

constexpr std::uint32_t InitialWidth  = 1280u;   // [px]
constexpr std::uint32_t InitialHeight = 720u;    // [px]

constexpr const char* WindowTitle = "Slate \u2014 Editor";
constexpr const char* HostName    = "EditorHost";

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                           MAIN
//------------------------------------------------------------------------------------------------------------------------

int main()
{
    using namespace Slate;

    // ① The timeline — one per process, constructed once.
    TickSequence Timeline;
    TickPoint    PreviousTick = Timeline.Advance();

    // ② The window.
    WindowInterchange Window;

    if (!Window.Open({ InitialWidth, InitialHeight }, WindowTitle).ContentPresent)
    {
        std::printf("%s \u2014 the window system declined\n", HostName);
        return 1;
    }

    // ③ The Vulkan instance.
    VulkanExchange DeviceEdge;

#ifdef SLATE_DEBUG
    const bool DiagnosticRequested = true;
#else
    const bool DiagnosticRequested = false;
#endif

    if (!DeviceEdge.ConstructInstance(DiagnosticRequested).ContentPresent)
    {
        std::printf("%s \u2014 no Vulkan instance could be constructed\n", HostName);
        return 1;
    }

    // ④ The presentation surface.
    const Deliver<VkSurfaceKHR> SurfaceConverted = Convert(DeviceEdge.Instance(), Window.NativeHandle());
    if (!SurfaceConverted.ContentPresent)
    {
        std::printf("%s \u2014 the presentation surface was refused\n", HostName);
        return 1;
    }

    const VkSurfaceKHR PresentationSurface = SurfaceConverted.Resolve();

    // ⑤ The diagnostic extension — attached after the instance, before the device.
    ReportSequence      DiagnosticRegister;
    DiagnosticExtension DiagnosticEdge;

    if (!DiagnosticEdge.Construct(DeviceEdge, DiagnosticRegister, Timeline).ContentPresent)
        std::printf("%s \u2014 the diagnostic extension was not negotiated\n", HostName);

    // ⑥ The device.
    if (!DeviceEdge.ConstructDevice(PresentationSurface).ContentPresent)
    {
        std::printf("%s \u2014 no Vulkan device could be constructed\n", HostName);
        return 1;
    }

    // ⑦ The presentation chain.
    DisplayScheduler DisplayChain;

    if (!DisplayChain.Construct(DeviceEdge, DiagnosticEdge, PresentationSurface,
                                InitialWidth, InitialHeight, LatencyIntent::SteadyPacing).ContentPresent)
    {
        std::printf("%s \u2014 the presentation chain was refused\n", HostName);
        return 1;
    }

    // ⑧ The cyclic recording slots and the command recording sequence.
    CycleScheduler  Rotation;
    CommandSequence Commands;

    if (!Rotation.Construct(DeviceEdge, DiagnosticEdge).ContentPresent ||
        !Commands.Construct(DeviceEdge, DiagnosticEdge).ContentPresent)
    {
        std::printf("%s \u2014 the recording rotation was refused\n", HostName);
        return 1;
    }

    // ⑨ The interface attachment.
    InterfaceAttachment InterfaceArriving = {};
    InterfaceArriving.Instance              = DeviceEdge.Instance();
    InterfaceArriving.ScoredDevice          = DeviceEdge.ScoredDevice();
    InterfaceArriving.ActiveDevice          = DeviceEdge.ActiveDevice();
    InterfaceArriving.GraphicsQueue         = DeviceEdge.GraphicsQueue();
    InterfaceArriving.GraphicsFamilyOrdinal = DeviceEdge.Capability().GraphicsFamilyOrdinal;
    InterfaceArriving.ColourTargetFormat    = DisplayChain.Carries();
    InterfaceArriving.RotationDepth         = RecordingRotationDepth;
    InterfaceArriving.NativeWindowSlot      = Window.NativeHandle();

    // ⑩ The viewport sequence — springs, drawers, and the assembled recording.
    ViewportSequence Viewport;

    DrawerDeclaration NorthDrawer;
    NorthDrawer.Caption       = "ControlCentre";
    NorthDrawer.TongueSubject = SymbolSubject::PulseTrace;
    NorthDrawer.PoseCount     = 2u;

    DrawerDeclaration SouthDrawer;
    SouthDrawer.Caption       = "AssetBrowser";
    SouthDrawer.TongueSubject = SymbolSubject::FolderClosed;
    SouthDrawer.PoseCount     = 3u;

    if (!Viewport.Construct(InterfaceArriving, NorthDrawer, SouthDrawer).ContentPresent)
    {
        std::printf("%s \u2014 the viewport sequence was refused\n", HostName);
        return 1;
    }

    // ─────────────────────────────────────────────────────────────────────────────────────────────────────
    //                                                       THE TICK LOOP
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────

    // 📝 🔴 The interface tick is built and sealed **before** the display image is acquired and before any
    //    command recording is opened. Nothing between the acquire and the present may return to the top of
    //    the loop, so no path can leave a command buffer in the recording condition or an interface tick
    //    open — which is the whole of the resize crash.

    std::printf("%s \u2014 running\n", HostName);

    while (!Window.ClosureRequested())
    {
        // ① Drain the window system's events.
        Window.Drain();

        // ② A minimised window has no drawable extent. Wait on the window system rather than spinning.
        DisplayExtent Extent = Window.CurrentExtent();

        if (Extent.Width == 0u || Extent.Height == 0u)
        {
            Window.Await();
            continue;
        }

        // ③ Re-establish the presentation chain the moment the extent moves, before anything reads it.
        if (Window.ExtentAltered())
        {
            vkDeviceWaitIdle(DeviceEdge.ActiveDevice());
            DisplayChain.Reclaim(Extent.Width, Extent.Height);
            Viewport.Renegotiate(RecordingRotationDepth);
            Window.AdoptExtent();
        }

        // ④ Await the rotation slot. The fence guards the recording this slot is about to reuse.
        if (!Rotation.Await().ContentPresent)
        {
            std::printf("%s \u2014 the rotation slot was lost\n", HostName);
            break;
        }

        const std::uint32_t         SlotOrdinal = Rotation.StandingOrdinal();
        const Deliver<RotationSlot> Standing    = Rotation.Standing();

        if (!Standing.ContentPresent)
            break;

        // ⑤ Build the interface tick. Every refusal here abandons the tick and costs nothing device-side.
        const TickPoint TickNow   = Timeline.Advance();
        const double    ElapsedMs = TickSequence::Span(PreviousTick, TickNow);
        PreviousTick = TickNow;

        if (!Viewport.Advance(ElapsedMs).ContentPresent)
        {
            Viewport.Abandon();
            continue;
        }

        Viewport.RecordDrawers();
        Viewport.DrawerPanels();

        if (!Viewport.SealPanels().ContentPresent)
        {
            Viewport.Abandon();
            continue;
        }

        // ⑥ Acquire the display image. A chain that was outgrown is re-established and the sealed content
        //    is discarded — it is rebuilt whole next tick and nothing device-side has been opened yet.
        const Deliver<ArrivedImage> Arrived = DisplayChain.Await(Standing.Resolve(), Timeline);

        if (!Arrived.ContentPresent)
        {
            if (Arrived.Declined.DeclaredReason == RefusalReason::DeviceLost)
            {
                std::printf("%s \u2014 the device was lost\n", HostName);
                break;
            }

            continue;
        }

        if (Arrived.Resolve().Reclaimed)
        {
            vkDeviceWaitIdle(DeviceEdge.ActiveDevice());
            DisplayChain.Reclaim(Extent.Width, Extent.Height);
            Viewport.Renegotiate(RecordingRotationDepth);
            Window.AdoptExtent();
            continue;
        }

        // ⑦ Open the recording. From here every path reaches the surrender.
        const Deliver<VkCommandBuffer> Recording = Commands.Open(SlotOrdinal);

        if (!Recording.ContentPresent)
        {
            std::printf("%s \u2014 the command recording was refused\n", HostName);
            break;
        }

        const VkCommandBuffer Assembling = Recording.Resolve();

        const VkRenderingAttachmentInfo ColourAttachment = {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = Arrived.Resolve().WholeView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue  = { .color = { { 0.06f, 0.06f, 0.08f, 1.0f } } }
        };

        const VkRenderingInfo RenderScope = {
            .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea           = { { 0, 0 }, { DisplayChain.StandingWidth(), DisplayChain.StandingHeight() } },
            .layerCount           = 1u,
            .colorAttachmentCount = 1u,
            .pColorAttachments    = &ColourAttachment
        };

        vkCmdBeginRendering(Assembling, &RenderScope);
        Viewport.Record(Assembling);
        vkCmdEndRendering(Assembling);

        // ⑧ Arm the rotation immediately before the surrender. Armed any earlier, a refusal between the two
        //    leaves the fence unsignalled and the next Await never returns.
        if (!Rotation.Arm().ContentPresent)
            break;

        const Deliver<bool> Surrendered = Commands.Surrender(SlotOrdinal, SurrenderOrdering{
            .Awaited      = Standing.Resolve().ImageArrived,
            .AwaitedStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .Signalled    = Standing.Resolve().RecordingDone,
            .Completion   = Standing.Resolve().Completion
        });

        if (!Surrendered.ContentPresent)
            break;

        // ⑨ Present. A refused present is a chain to re-establish, not a reason to leave the loop.
        if (!DisplayChain.Present(Standing.Resolve(), Arrived.Resolve().ImageOrdinal).ContentPresent)
        {
            vkDeviceWaitIdle(DeviceEdge.ActiveDevice());
            DisplayChain.Reclaim(Extent.Width, Extent.Height);
            Viewport.Renegotiate(RecordingRotationDepth);
            Window.AdoptExtent();
        }

        Rotation.Advance();
    }

    // ─────────────────────────────────────────────────────────────────────────────────────────────────────
    //                                                      RECLAMATION
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────

    if (DeviceEdge.ActiveDevice() != VK_NULL_HANDLE)
        vkDeviceWaitIdle(DeviceEdge.ActiveDevice());

    Viewport.Reclaim();
    Commands.Reclaim();
    Rotation.Reclaim();
    DisplayChain.Surrender();
    DiagnosticEdge.Reclaim();
    Reclaim(DeviceEdge.Instance(), PresentationSurface);
    DeviceEdge.ReclaimDevice();

    std::printf("%s \u2014 exited cleanly\n", HostName);
    return 0;
}
