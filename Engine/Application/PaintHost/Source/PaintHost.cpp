//============================================================================================================================================
//                                                             PAINTHOST.CPP
//============================================================================================================================================
// 🧩 The painting application — lifetime and tick only, with every device concern held by HostLifecycle.

#include "Contract/DeliveryContract.h"
#include "SlateUI/Interface/ViewportSequence/Api/ViewportSequence.h"
#include "SlateUI/Interface/WorkspacePanel/Api/WorkspaceIndex.h"
#include "SlateVulkan/Device/HostLifecycle/Api/HostLifecycle.h"

#include <cstdio>

//------------------------------------------------------------------------------------------------------------------------
//                                                          FIGURES
//------------------------------------------------------------------------------------------------------------------------

namespace
{

using namespace Slate;

constexpr std::uint32_t InitialWidth  = 1280u;   // [px]
constexpr std::uint32_t InitialHeight = 720u;    // [px]

constexpr const char* WindowTitle = "Slate \u2014 Paint";
constexpr const char* HostName    = "PaintHost";

// 📝 The workspace ground the interface is recorded over. Stated here because it is the one visual decision
//    this host makes; everything else it presents belongs to a panel.
constexpr float WorkspaceGround[4] = { 0.06f, 0.06f, 0.08f, 1.0f };   // [-]

/// 🧩 Copies the device handles across the layer seam into the attachment the interface declares.
/// note  🔴 `SlateVulkan` cannot name `InterfaceAttachment` — it lives one layer above — so `HostLifecycle`
///        offers the same handles as `DeviceOffering` and the host performs the copy. The copy IS the seam:
///        it happens in the one translation unit that is allowed to see both sides.
InterfaceAttachment Attach(const DeviceOffering& Offered)
{
    InterfaceAttachment Arriving = {};

    Arriving.Instance                 = Offered.Instance;
    Arriving.ScoredDevice             = Offered.ScoredDevice;
    Arriving.ActiveDevice             = Offered.ActiveDevice;
    Arriving.GraphicsQueue            = Offered.GraphicsQueue;
    Arriving.GraphicsFamilyOrdinal    = Offered.GraphicsFamilyOrdinal;
    Arriving.ColourTargetFormat       = Offered.ColourTargetFormat;
    Arriving.MinimumDisplayImageCount = Offered.MinimumDisplayImageCount;
    Arriving.DisplayImageCount        = Offered.DisplayImageCount;
    Arriving.NativeWindowSlot         = Offered.NativeWindowSlot;

    return Arriving;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                            MAIN
//------------------------------------------------------------------------------------------------------------------------

int main()
{
    using namespace Slate;

    // ① The five lifetimes — window, instance, surface, diagnostic, device, chain, slots, recordings.
    HostDeclaration Declared;
    Declared.Naming        = HostName;
    Declared.WindowCaption = WindowTitle;
    Declared.InitialWidth  = InitialWidth;
    Declared.InitialHeight = InitialHeight;
    Declared.Pacing        = LatencyIntent::SteadyPacing;

#ifdef SLATE_DEBUG
    Declared.DiagnosticRequested = true;
#endif

    HostLifecycle Lifetime;

    if (!Lifetime.Construct(Declared).ContentPresent)
        return 1;

    // ② The viewport sequence — springs, drawers, and the assembled recording.
    ViewportSequence Viewport;

    DrawerDeclaration NorthDrawer;
    NorthDrawer.Caption       = "ControlCentre";
    NorthDrawer.TongueSubject = SymbolSubject::PulseTrace;
    NorthDrawer.PoseCount     = 2u;

    DrawerDeclaration SouthDrawer;
    SouthDrawer.Caption       = "AssetBrowser";
    SouthDrawer.TongueSubject = SymbolSubject::FolderClosed;
    SouthDrawer.PoseCount     = 3u;

    if (!Viewport.Construct(Attach(Lifetime.Offering()), NorthDrawer, SouthDrawer).ContentPresent)
    {
        std::printf("%s \u2014 the viewport sequence was refused\n", HostName);
        return 1;
    }

    // ─────────────────────────────────────────────────────────────────────────────────────────────────────
    //                                                       THE TICK LOOP
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────

    // 📝 🔴 Every refusal is resolved inside Await, before a display image is acquired. A tick that reports
    //    Recording has a recording open and cannot be abandoned; a tick that reports Withdrawn has opened
    //    nothing. That is what makes the `continue` below safe — the arrangement this host previously got
    //    wrong five times over, returning to the top of the loop with a command buffer still recording.

    // ③ The workspaces this host opens. 🔴 The LEDGER owns them and the panel presents them — `14` §1
    //    forbids a panel from holding what it displays, and separating the two is the whole reason there
    //    are two components here rather than one.
    // 📝 The subject this host opens by default, named once so the startup enrolment and the strip's `+`
    //    cannot disagree about what a new workspace is.
    constexpr WorkspaceSubject DefaultSubject = WorkspaceSubject::Painting;

    WorkspaceIndex Workspaces;
    WorkspacePanel Workspace;

    if (!Workspace.Construct(Viewport.Surface(), Viewport.Appearance()).ContentPresent)
    {
        std::printf("%s \u2014 the workspace panel was refused\n", HostName);
        return 1;
    }

    // 📝 One workspace open by default, of the subject this host is for. A host that opened none would show
    //    the vacant run on first launch, which reads as a failure rather than as a fresh start.
    if (!Workspaces.Enrol(DefaultSubject).ContentPresent)
    {
        std::printf("%s \u2014 the default workspace could not be opened\n", HostName);
        return 1;
    }

    // 🔴 The sheet's tab figures seated into the vendor's style, including the four `Patches/` adds. They
    //    default to 0.0f, at which a patched build draws stock rectangular tabs — so this call is what
    //    turns the trapezoid on.
    if (!Viewport.Seam().SeatWorkspaceStyle(Viewport.Appearance().WorkspaceMeasure,
                                                 Viewport.Appearance().Workspace).ContentPresent)
    {
        std::printf("%s \u2014 the workspace style was not seated\n", HostName);
    }

    std::printf("%s \u2014 opened %s\n", HostName, Workspaces.ActiveTitle());

    while (Lifetime.Standing())
    {
        const TickPass Pass = Lifetime.Await(WorkspaceGround);

        if (Pass.Standing == TickStanding::Closed)
            break;

        // 🔴 Phase one of a device rebuild. The device STILL STANDS here, so this is the one moment the
        //    interface can release its descriptor pool, font atlas and pipelines against a live handle.
        //    Reclaiming after the rebuild idles a device the vendor has already destroyed, which the
        //    loader reports as VUID-vkDeviceWaitIdle-device-parameter.
        if (Pass.DeviceRetiring)
        {
            Viewport.Reclaim();
            continue;
        }

        // ③·i 🔴 The DEVICE was rebuilt, so every device handle the interface holds names an object the
        //      vendor has returned — its font atlas, its descriptor sets, its pipelines. Renegotiating the
        //      image counts would restate figures against a device that no longer exists, so the interface
        //      is reclaimed and reconstructed against the handles the rebuilt device offers.
        //      Tested before DisplayRecovered because a device rebuild raises both.
        if (Lifetime.DeviceRecovered())
        {
            // 📝 Not reclaimed here: the retiring tick above already did it, while the device lived.
            if (!Viewport.Construct(Attach(Lifetime.Offering()), NorthDrawer, SouthDrawer).ContentPresent)
            {
                std::printf("%s \u2014 the interface could not be rebuilt on the recovered device\n", HostName);
                break;
            }

            // 📝 The display recovery this rebuild also raised is consumed here. The reconstruction above
            //    already took the counts the new chain holds, and renegotiating them again would restate
            //    what was just constructed.
            static_cast<void>(Lifetime.DisplayRecovered());
        }

        // ③ The chain was re-established. The interface is told the counts it now holds, exactly once.
        else if (Lifetime.DisplayRecovered())
        {
            const DeviceOffering Offered = Lifetime.Offering();
            // 🔴 Read, not discarded. An interface still holding the previous image counts records
            //    against a chain depth that no longer exists, and the vendor reports that as a
            //    descriptor mismatch several ticks later rather than as the resize that caused it.
            if (!Viewport.Renegotiate(Offered.MinimumDisplayImageCount, Offered.DisplayImageCount))
            {
                std::printf("%s \u2014 the interface declined the restated image counts\n", HostName);
            }
        }

        if (Pass.Standing != TickStanding::Recording)
            continue;

        // ④ Build the interface tick. A refusal here abandons the tick's content, and the recording is
        //    still surrendered — an empty rendering scope presents the cleared ground, which is correct
        //    and is what the artist sees for one tick.
        if (Viewport.Advance(Pass.ElapsedMilliseconds).ContentPresent)
        {
            // 🔴 The workspace is recorded FIRST and the drawers over it. One background draw list, so
            //    the order of recording IS the z-order — and the previous arrangement recorded the
            //    workspace after `RecordDrawers`, which painted the whole surface over the control
            //    centre and the asset browser.
            const PlaneExtent Whole = Spanning(0.0f, 0.0f,
                                               static_cast<float>(Pass.ExtentAlong),
                                               static_cast<float>(Pass.ExtentAcross));

            Disregard(Workspace.Record(Whole, Workspaces.ActiveTitle()));

            // 📝 The vendor's patched tab bar, seated on the strip the panel measured. The trapezoid is
            //    PatchA's; nothing in Slate draws a tab.
            // 🔴 Recorded even at a zero count, so the `+` stands on an empty strip. An artist who closed
            //    the last workspace must have a way back, which an early return here would deny them.
            const std::uint32_t OpenCount = Workspaces.OpenCount();

            const char* Titles[WorkspaceIndex::WorkspaceCeiling] = {};

            for (std::uint32_t Ordinal = 0u; Ordinal < OpenCount; ++Ordinal)
                Titles[Ordinal] = Workspaces.Standing(Ordinal).Resolve().Titled;

            std::uint32_t Chosen    = OpenCount;
            std::uint32_t Closed    = OpenCount;
            bool          Enrolling = false;

            Viewport.Seam().RecordWorkspaceTabs(Workspace.Strip(), Titles, OpenCount,
                                                Workspaces.ActiveOrdinal(), Chosen, Closed, Enrolling);

            // 🔴 Acted on AFTER the strip is recorded, never inside it. Enrolling or withdrawing mid-sweep
            //    edits the set the vendor's tab bar is walking, which invalidates the position it holds.
            if (Chosen < OpenCount)
                Disregard(Workspaces.Present(Chosen));

            if (Closed < OpenCount)
                Disregard(Workspaces.Withdraw(Closed));

            if (Enrolling)
                Disregard(Workspaces.Enrol(DefaultSubject));

            // 📝 The dock space over the body, so a panel dragged loose has somewhere to dock to.
            Viewport.Seam().RecordDockSpace(Workspace.Body());

            // 📝 The drawers last, so they sit ABOVE the workspace as the sheet lays them.
            Viewport.RecordDrawers();
            Viewport.DrawerPanels();

            if (Viewport.SealPanels().ContentPresent)
            {
                // 🔴 Read. A refused Record presents the cleared ground with nothing on it, which is
                //    indistinguishable from a panel that drew nothing, so the refusal is named here.
                if (!Viewport.Record(Pass.Recording))
                {
                    std::printf("%s \u2014 the interface content was not recorded\n", HostName);
                }
            }
            else
            {
                Disregard(Viewport.Abandon());
            }
        }
        else
        {
            Disregard(Viewport.Abandon());
        }

        // ⑤ Close the scope, submit, present, advance. A refused present re-establishes the chain rather
        //    than ending the loop.
        if (!Lifetime.Surrender().ContentPresent)
            break;
    }

    // ─────────────────────────────────────────────────────────────────────────────────────────────────────
    //                                                      RECLAMATION
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────

    // 📝 The viewport is retired before the lifetimes it was constructed over. HostLifecycle idles the
    //    device inside Reclaim, so nothing here needs to.
    // 🔴 Read before Reclaim. The register is Device lifetime, and a reclaimed device has emptied it.
    const std::uint32_t Serious = Lifetime.StateDiagnostics();

    Workspace.Reset();
    Workspaces.Reset();
    Viewport.Reclaim();
    Lifetime.Reclaim();

    std::printf("%s \u2014 exited cleanly\n", HostName);

    // 🔴 Returned rather than only stated. A validation run needs an exit code, so that a serious arrival
    //    fails whatever invoked the host instead of scrolling past in a console nobody reads.
    return (Serious == 0u) ? 0 : 1;
}
