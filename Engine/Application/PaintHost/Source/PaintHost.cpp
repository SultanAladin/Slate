//============================================================================================================================================
//                                                              PAINTHOST.CPP
//============================================================================================================================================
// 🧩 The standalone painting executable — Vulkan bring-up, the tick loop, and the two drawers.

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

constexpr const char* WindowTitle = "Slate \u2014 Paint";

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

    const Deliver<bool> WindowOpened = Window.Open({ InitialWidth, InitialHeight }, WindowTitle);
    if (!WindowOpened.ContentPresent)
    {
        std::printf("PaintHost \u2014 the window system declined\n");
        return 1;
    }

    // ③ The Vulkan instance.
    VulkanExchange DeviceEdge;

#ifdef SLATE_DEBUG
    const bool DiagnosticRequested = true;
#else
    const bool DiagnosticRequested = false;
#endif

    const Deliver<bool> InstanceBuilt = DeviceEdge.ConstructInstance(DiagnosticRequested);
    if (!InstanceBuilt.ContentPresent)
    {
        std::printf("PaintHost \u2014 no Vulkan instance could be constructed\n");
        return 1;
    }

    // ④ The presentation surface.
    const Deliver<VkSurfaceKHR> SurfaceConverted = Convert(DeviceEdge.Instance(),
                                                           Window.NativeHandle());
    if (!SurfaceConverted.ContentPresent)
    {
        std::printf("PaintHost \u2014 the presentation surface was refused\n");
        return 1;
    }

    const VkSurfaceKHR PresentationSurface = SurfaceConverted.Resolve();

    // ⑤ The diagnostic extension — attached after the instance, before the device.
    ReportSequence DiagnosticRegister;
    DiagnosticExtension DiagnosticEdge;

    const Deliver<bool> DiagnosticBuilt = DiagnosticEdge.Construct(DeviceEdge, DiagnosticRegister, Timeline);
    if (!DiagnosticBuilt.ContentPresent)
    {
        std::printf("PaintHost \u2014 the diagnostic extension was not negotiated\n");
    }

    // ⑥ The device.
    const Deliver<bool> DeviceBuilt = DeviceEdge.ConstructDevice(PresentationSurface);
    if (!DeviceBuilt.ContentPresent)
    {
        std::printf("PaintHost \u2014 no Vulkan device could be constructed\n");
        return 1;
    }

    // ⑦ The presentation chain.
    DisplayScheduler DisplayChain;

    const Deliver<bool> ChainBuilt = DisplayChain.Construct(DeviceEdge, DiagnosticEdge,
                                                            PresentationSurface,
                                                            InitialWidth, InitialHeight,
                                                            LatencyIntent::SteadyPacing);
    if (!ChainBuilt.ContentPresent)
    {
        std::printf("PaintHost \u2014 the presentation chain was refused\n");
        return 1;
    }

    // ⑧ The cyclic recording slots.
    CycleScheduler Cycle;

    const Deliver<bool> CycleBuilt = Cycle.Construct(DeviceEdge, DiagnosticEdge);
    if (!CycleBuilt.ContentPresent)
    {
        std::printf("PaintHost \u2014 the recording rotation was refused\n");
        return 1;
    }

    // ⑨ The command recording sequence.
    CommandSequence Commands;

    const Deliver<bool> CommandsBuilt = Commands.Construct(DeviceEdge, DiagnosticEdge);
    if (!CommandsBuilt.ContentPresent)
    {
        std::printf("PaintHost \u2014 the command sequence was refused\n");
        return 1;
    }

    // ⑩ The interface attachment.
    InterfaceAttachment InterfaceArriving              = {};
    InterfaceArriving.Instance              = DeviceEdge.Instance();
    InterfaceArriving.ScoredDevice          = DeviceEdge.ScoredDevice();
    InterfaceArriving.ActiveDevice          = DeviceEdge.ActiveDevice();
    InterfaceArriving.GraphicsQueue         = DeviceEdge.GraphicsQueue();
    InterfaceArriving.GraphicsFamilyOrdinal = DeviceEdge.Capability().GraphicsFamilyOrdinal;
    InterfaceArriving.ColourTargetFormat       = DisplayChain.Carries();
    InterfaceArriving.MinimumDisplayImageCount = DisplayChain.MinimumChainImageCount();
    InterfaceArriving.DisplayImageCount        = DisplayChain.ChainImageCount();
    InterfaceArriving.NativeWindowSlot         = Window.NativeHandle();

    // ⑪ The viewport sequence — springs, drawers, and the assembled recording.
    ViewportSequence Viewport;

    DrawerDeclaration NorthDrawer;
    NorthDrawer.Caption       = "ControlCentre";
    NorthDrawer.TongueSubject = SymbolSubject::PulseTrace;
    NorthDrawer.PoseCount     = 2u;

    DrawerDeclaration SouthDrawer;
    SouthDrawer.Caption       = "AssetBrowser";
    SouthDrawer.TongueSubject = SymbolSubject::FolderClosed;
    SouthDrawer.PoseCount     = 3u;

    const Deliver<bool> ViewportBuilt = Viewport.Construct(InterfaceArriving, NorthDrawer, SouthDrawer);
    if (!ViewportBuilt.ContentPresent)
    {
        std::printf("PaintHost \u2014 the viewport sequence was refused\n");
        return 1;
    }

    std::uint64_t DebugTick = 0u;

    // ─────────────────────────────────────────────────────────────────────────────────────────────────────
    //                                                       THE TICK LOOP
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────

    std::printf("PaintHost \u2014 running\n");

    while (!Window.ClosureRequested())
    {
        // ① Drain the window system's events.
        Window.Drain();

        const DisplayExtent Extent = Window.CurrentExtent();
        if (Extent.Width == 0u || Extent.Height == 0u)
            continue;

        // ② Await the next cycle slot.
        const Deliver<bool> SlotReady = Cycle.Await();
        if (!SlotReady.ContentPresent)
        {
            std::printf("PaintHost \u2014 the cycle slot was lost\n");
            break;
        }

        const std::uint32_t SlotOrdinal = Cycle.StandingOrdinal();

        // ③ Await the next display image.
        const Deliver<CycleSlot> Standing = Cycle.Standing();
        if (!Standing.ContentPresent)
            break;

        const Deliver<ArrivedImage> Arrived = DisplayChain.Await(Standing.Resolve(), Timeline);
        if (!Arrived.ContentPresent)
        {
            if (Arrived.Declined.DeclaredReason == RefusalReason::DeviceLost)
            {
                std::printf("PaintHost \u2014 the device was lost\n");
                break;
            }

            continue;
        }

        // ③a The chain was outgrown — re-establish and skip this rotation.
        if (Arrived.Resolve().Reclaimed)
        {
            if (DeviceEdge.ActiveDevice() != VK_NULL_HANDLE)
                vkDeviceWaitIdle(DeviceEdge.ActiveDevice());

            DisplayChain.Reclaim(Extent.Width, Extent.Height);
            Viewport.Renegotiate(DisplayChain.MinimumChainImageCount(), DisplayChain.ChainImageCount());
            continue;
        }

        // ④ Arm the rotation before the submission.
        const Deliver<bool> Armed = Cycle.Arm();
        if (!Armed.ContentPresent)
            break;

        // ⑤ Open the command recording for this cycle slot.
        const Deliver<VkCommandBuffer> Recording = Commands.Open(SlotOrdinal);
        if (!Recording.ContentPresent)
            break;

        // ⑥ Begin a dynamic rendering scope over the display image.
        const VkRenderingAttachmentInfo ColorAttachment = {
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
            .pColorAttachments    = &ColorAttachment
        };

        vkCmdBeginRendering(Recording.Resolve(), &RenderScope);

        // ⑦ Advance the viewport sequence — opens ImGui, adopts the surface, drives the drawers.
        const TickPoint TickNow   = Timeline.Advance();
        const double    ElapsedMs = TickSequence::Span(PreviousTick, TickNow);
        PreviousTick = TickNow;

        const Deliver<bool> Ticked = Viewport.Advance(ElapsedMs);
        if (!Ticked.ContentPresent)
            continue;

        // Debug: log pointer and drawer state every 60 ticks.
        if (++DebugTick % 60u == 1u)
        {
            const PointerCondition& Ptr  = Viewport.Surface().Pointer();
            const DrawerSpace&      D    = Viewport.Drawers();
            const DrawerPose        NP   = D.Pose(DrawerBearing::North);
            const DrawerPose        SP   = D.Pose(DrawerBearing::South);

            std::printf("tick %llu  ptr(%.0f,%.0f) held=%d arrived=%d released=%d | north=%d south=%d moving=%d\n",
                        static_cast<unsigned long long>(DebugTick),
                        static_cast<double>(Ptr.PositionAlong),
                        static_cast<double>(Ptr.PositionAcross),
                        static_cast<int>(Ptr.ContactHeld),
                        static_cast<int>(Ptr.ContactArrived),
                        static_cast<int>(Ptr.ContactReleased),
                        static_cast<int>(NP),
                        static_cast<int>(SP),
                        static_cast<int>(Viewport.Moving()));
        }

        // ⑧ Record the drawer chrome.
        Viewport.RecordDrawers();

        // ⑨ Open the panel recording window — panels draw here through Viewport.Surface().
        Viewport.DrawerPanels();

        // ⑩ Close the panel window and seal the interface tick.
        const Deliver<bool> Sealed = Viewport.SealPanels();
        if (!Sealed.ContentPresent)
            continue;

        // ⑪ Record the assembled interface content into the command buffer.
        const Deliver<bool> InterfaceRecorded = Viewport.Record(Recording.Resolve());

        // ⑫ End the dynamic rendering scope.
        vkCmdEndRendering(Recording.Resolve());

        // ⑬ Surrender the recording to the queue.
        const Deliver<bool> Surrendered = Commands.Surrender(SlotOrdinal, SurrenderOrdering{
            .Awaited      = Standing.Resolve().ImageArrived,
            .AwaitedStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .Signalled    = Standing.Resolve().RecordingDone,
            .Completion   = Standing.Resolve().Completion
        });

        if (!Surrendered.ContentPresent)
            break;

        // ⑭ Present the display image.
        const Deliver<bool> Presented = DisplayChain.Present(Standing.Resolve(), Arrived.Resolve().ImageOrdinal);
        if (!Presented.ContentPresent)
            break;

        // ⑮ Advance the rotation.
        Cycle.Advance();
    }

    // ─────────────────────────────────────────────────────────────────────────────────────────────────────
    //                                                      RECLAMATION
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────

    if (DeviceEdge.ActiveDevice() != VK_NULL_HANDLE)
        vkDeviceWaitIdle(DeviceEdge.ActiveDevice());

    Viewport.Reclaim();
    Commands.Reclaim();
    Cycle.Reclaim();
    DisplayChain.Surrender();
    DiagnosticEdge.Reclaim();
    Reclaim(DeviceEdge.Instance(), PresentationSurface);
    DeviceEdge.ReclaimDevice();

    std::printf("PaintHost \u2014 exited cleanly\n");
    return 0;
}
