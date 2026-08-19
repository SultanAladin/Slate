//============================================================================================================================================
//                                                             PAINTHOST.CPP
//============================================================================================================================================
// 🧩 The painting application — lifetime and tick only, with every device concern held by HostLifecycle.

#include "Contract/DeliveryContract.h"
#include "SlateUI/Interface/ContentBrowserPanel/Api/ContentBrowserPanel.h"
#include "SlateUI/Interface/ControlCentrePanel/Api/ControlCentrePanel.h"
#include "SlateUI/Interface/ThemeInterchange/Api/ThemeInterchange.h"
#include "SlateUI/Interface/EditorPanel/Api/EditorPanel.h"
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
//------------------------------------------------------------------------------------------------------------------------
//                                                      THE THREE CEILINGS
//------------------------------------------------------------------------------------------------------------------------

// 🔴 Seating the content browser in the south drawer crossed all three of the budgets that have each, at
//    least once, taken a host down with no window and no log line. They are asserted here so a fourth panel
//    cannot repeat any of them silently.

// ① EASED INTERPOLANTS. `InteractionIndex::Enrol` draws two fades per control, and the integrator's supply
//    is shared by every ledger in the process — the browser's private ledger does not get its own pool.
constexpr std::uint32_t EasesPerControl = 2u;
constexpr std::uint32_t CentreControls  = ControlCentrePanel::ControlCapacity;
constexpr std::uint32_t BrowserControls = ContentBrowserPanel::EnrolmentDemand;
constexpr std::uint32_t EditorControls  = PanelStructure::RecordCeiling * EditorPanel::ControlsPerRecord;
constexpr std::uint32_t BareEases       = 9u + 1u;   // [-] - the Control Centre's own motions

constexpr std::uint32_t DemandedEases =
    ((CentreControls + BrowserControls + EditorControls) * EasesPerControl) + BareEases;

static_assert(DemandedEases <= MotionIntegrator::EaseCapacity,
              "this host's panels demand more eased interpolants than the integrator holds — the panel "
              "constructed last is refused mid-enrolment and the host exits before its first frame; raise "
              "MotionIntegrator::EaseCapacity or reduce a panel's control count");

// ② LEDGER SLOTS. Counted per ledger, not per host. The browser is the only occupant of its own
//    `BrowserLedger`, so only its demand is weighed here; the Control Centre answers to its private index.
static_assert(BrowserControls <= InteractionIndex::ControlCapacity,
              "the content browser enrols more controls than one InteractionIndex holds — Construct is "
              "refused with \"no further control slot\" and the south drawer opens onto blank ground");

// ③ AUTOMATIC STORAGE. A Windows thread is given one megabyte and a refusal here is not a refusal at all:
//    the guard page is touched in the prologue, so the process dies before a statement can report anything.
//    Linux hands out eight megabytes, which is exactly why no gate here can catch it.
constexpr std::size_t WindowsThreadStack = 1048576u;                  // [B] - the shipped linker default
constexpr std::size_t AutomaticCeiling   = WindowsThreadStack / 4u;   // [B] - a quarter, leaving room to call

static_assert(sizeof(WorkspaceIndex) + sizeof(WorkspacePanel) + sizeof(EditorPanel)
              + (sizeof(PanelStructure)       * WorkspaceIndex::WorkspaceCeiling)
              + (sizeof(EditorPanelOrdinates) * WorkspaceIndex::WorkspaceCeiling)
              + sizeof(ControlCentrePanel)  + sizeof(ControlCentreOrdinates)
              + sizeof(InteractionIndex)    + sizeof(ContentBrowserPanel)
              + sizeof(ContentBrowserOrdinates) + sizeof(ContentLibrary) <= AutomaticCeiling,
              "this host's automatic UI members no longer fit a quarter of a Windows thread stack — the "
              "prologue's stack probe will fault before main runs a statement and the host will exit with "
              "no window and no log line; move the largest member to static storage");

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

int main(int ArgumentCount, char** ArgumentValues)
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

    WorkspaceIndex          Workspaces;
    WorkspacePanel          Workspace;
    EditorPanel             WorkspacePanels;
    PanelStructure          PanelPartitions[WorkspaceIndex::WorkspaceCeiling];
    EditorPanelOrdinates    PanelOrdinates[WorkspaceIndex::WorkspaceCeiling];
    ControlCentrePanel      ControlCentre;
    ControlCentreOrdinates  ControlCentreValues;

    InteractionIndex         BrowserLedger;

    // 📝 The south drawer's occupant. The library is the HOST's, not the panel's — `14` §1 forbids a panel
    //    from holding what it displays, which is the same separation WorkspaceIndex and WorkspacePanel keep.
    ContentBrowserPanel      ContentBrowser;
    ContentBrowserOrdinates  ContentBrowserSeated;
    ContentLibrary           ContentSeated;

    // 📝 The appearance file sits beside the executable and is read once, before any panel is recorded. A
    //    first run has no file yet, which is the ordinary case and not a fault — the build's own appearance
    //    stands and the first colour the artist changes writes the file.
    const char* const InvokedAs = (ArgumentCount > 0) ? ArgumentValues[0] : "";

    {
        ThemeSelection Recorded;

        if (ThemeInterchange::AdoptBeside(InvokedAs, Recorded))
        {
            ControlCentreValues.Theme       = Recorded.Presented;
            ControlCentreValues.Primary     = Recorded.Primary;
            ControlCentreValues.Secondary   = Recorded.Secondary;
            ControlCentreValues.Information = Recorded.Information;
            ControlCentreValues.Warning     = Recorded.Warning;
            ControlCentreValues.Alert       = Recorded.Alert;
        }
    }

    // 🔴 What was last written, so the file is inscribed when a colour actually changes and not every tick.
    //    A write per frame would rewrite the whole appearance sixty times a second for as long as the
    //    Control Centre is open, which is a disk cost no artist asked for.
    ThemeSelection InscribedSelection;
    InscribedSelection.Presented   = ControlCentreValues.Theme;
    InscribedSelection.Primary     = ControlCentreValues.Primary;
    InscribedSelection.Secondary   = ControlCentreValues.Secondary;
    InscribedSelection.Information = ControlCentreValues.Information;
    InscribedSelection.Warning     = ControlCentreValues.Warning;
    InscribedSelection.Alert       = ControlCentreValues.Alert;

    // 🔴 Declared BEFORE any panel is constructed. Panels that copy their inks do so out of the appearance the
    //    viewport hands them at Construct, so a selection declared afterwards would leave the first frames
    //    drawn in the transcription's own theme and only correct itself on the artist's first colour change.
    Viewport.Retint(InscribedSelection);

    // 📝 Which dock node the next enrolled workspace is seated into; zero means the main dock space.
    std::uint32_t  EnrolIntoNode = 0u;

    if (!Workspace.Construct(Viewport.Surface(), Viewport.Appearance()).ContentPresent)
    {
        std::printf("%s \u2014 the workspace panel was refused\n", HostName);
        return 1;
    }

    if (!WorkspacePanels.Construct(Viewport.MotionSource(), Viewport.Surface(), Viewport.Appearance()).ContentPresent)
    {
        std::printf("%s \u2014 the editor panels were refused\n", HostName);
        return 1;
    }

    if (!ControlCentre.Construct(Viewport.MotionSource(), Viewport.Surface(), Viewport.Appearance()).ContentPresent)
    {
        std::printf("%s \u2014 the Control Centre panel was refused\n", HostName);
        return 1;
    }

    if (!BrowserLedger.Construct(Viewport.MotionSource()).ContentPresent)
    {
        std::printf("%s \u2014 the content browser ledger was refused\n", HostName);
        return 1;
    }

    // 🔴 The browser carries its OWN ledger, as every panel here does, so its enrolment cannot exhaust the
    //    Control Centre's. Read — an enrolment refusal is silent at the call site and a browser that was
    //    refused records nothing at all, which reads as a drawer that opens onto blank ground.
    if (!ContentBrowser.Construct(BrowserLedger, Viewport.Surface()).ContentPresent)
    {
        std::printf("%s \u2014 the content browser was refused\n", HostName);
        return 1;
    }

    // 🔴 The browser takes no appearance at Construct — it is seated here, once the viewport has resolved one.
    ContentBrowser.Reseat(Viewport.Appearance());

    SeatReferenceContent(ContentSeated);

    // 📝 One workspace open by default, of the subject this host is for. A host that opened none would show
    //    the vacant run on first launch, which reads as a failure rather than as a fresh start.
    const Deliver<std::uint32_t> DefaultWorkspace = Workspaces.Enrol(DefaultSubject);
    if (!DefaultWorkspace.ContentPresent)
    {
        std::printf("%s \u2014 the default workspace could not be opened\n", HostName);
        return 1;
    }
    PanelPartitions[DefaultWorkspace.Resolve()].Construct(PanelSubject::Viewport);

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

            // 🔴 The dock space FIRST, over the whole panel. Every workspace below docks into it, and the
            //    vendor draws their tabs with PatchA's trapezoid — which is what makes a tab draggable out
            //    into a floating window. A hand-recorded tab bar cannot be undocked: the vendor's docking
            //    operates on WINDOWS, so a workspace has to be one.
            Viewport.Seam().RecordDockSpace(Whole);

            const std::uint32_t OpenCount = Workspaces.OpenCount();

            // 🔴 Titles are read through `Titled`, which points into the ledger's own storage. The delivered
            //    form copies the entry, so a pointer taken from it dangles at the semicolon — every label
            //    then decayed to the same garbage and ImGui reported four conflicting IDs.
            std::uint32_t Withdrawing = OpenCount;

            // 📝 The node the previous tick's `+` named, so the workspace it enrolled is seated into the
            //    strip the artist actually pressed rather than always into the main dock space.
            const std::uint32_t SeatInto = EnrolIntoNode;

            EnrolIntoNode = 0u;

            WorkspacePanels.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);

            for (std::uint32_t Ordinal = 0u; Ordinal < OpenCount; ++Ordinal)
            {
                const char* Titled = Workspaces.Titled(Ordinal);

                if (Titled == nullptr)
                    continue;

                bool Standing = true;

                const PlaneExtent PanelExtent = Viewport.Seam().EnterWorkspaceWindow(
                    Titled, !Workspaces.Seated(Ordinal), SeatInto, Standing);
                Workspaces.Seat(Ordinal);

                if (PanelExtent.SpanAlong() > 0.0f && PanelExtent.SpanAcross() > 0.0f)
                {
                    Disregard(Viewport.Surface().RelayerWindow());
                    Disregard(WorkspacePanels.Record(PanelExtent,
                                                      PanelPartitions[Ordinal],
                                                      PanelOrdinates[Ordinal],
                                                      Ordinal));
                    if (WorkspacePanels.PointerCaptured(Ordinal))
                        Viewport.Seam().WithholdPointer();
                }

                Viewport.Seam().LeaveWorkspaceWindow();
                Disregard(Viewport.Surface().Relayer(RecordingSurface::ShellLayer::Beneath));

                // ⚠️ Recorded, never acted on inside the sweep. Withdrawing here edits the set being walked.
                if (!Standing)
                    Withdrawing = Ordinal;
            }

            if (Withdrawing < OpenCount)
            {
                Disregard(Workspaces.Withdraw(Withdrawing));
                WorkspacePanels.WithdrawPresentation(Withdrawing);
                for (std::uint32_t Moving = Withdrawing; Moving + 1u < OpenCount; ++Moving)
                {
                    PanelPartitions[Moving] = PanelPartitions[Moving + 1u];
                    PanelOrdinates[Moving]   = PanelOrdinates[Moving + 1u];
                }
                PanelPartitions[OpenCount - 1u].Reset();
                PanelOrdinates[OpenCount - 1u] = EditorPanelOrdinates{};
            }

            // 📝 The `+`, seated inside the dock node's own tab bar so the vendor lays it after the last
            //    tab — always at the end, by construction rather than by arithmetic.
            std::uint32_t AskingNode = 0u;

            if (Viewport.Seam().RecordWorkspaceAddition(Workspace.Strip(), OpenCount, AskingNode))
            {
                // 🔴 The asking node is carried to the NEXT tick, because the workspace it enrols is not
                //    recorded until then. Seating it against the main space instead is what put a new
                //    workspace in the wrong window.
                EnrolIntoNode = AskingNode;
                const Deliver<std::uint32_t> EnrolledWorkspace = Workspaces.Enrol(DefaultSubject);
                if (EnrolledWorkspace.ContentPresent)
                    PanelPartitions[EnrolledWorkspace.Resolve()].Construct(PanelSubject::Viewport);
            }

            // 🔴 With nothing open there is no tab bar to seat a `+` in, so the empty shell carries the
            //    invitation itself. `WorkspacePanel` draws "CREATE PANEL" on plain black; a press anywhere
            //    on that ground enrols one, which is the way out of a state that otherwise has none.
            if (OpenCount == 0u && Viewport.Seam().VacantPressed(Whole))
            {
                const Deliver<std::uint32_t> EnrolledWorkspace = Workspaces.Enrol(DefaultSubject);
                if (EnrolledWorkspace.ContentPresent)
                    PanelPartitions[EnrolledWorkspace.Resolve()].Construct(PanelSubject::Viewport);
            }

            // 📝 The drawers last, so they sit ABOVE the workspace as the sheet lays them.
            Viewport.RecordDrawers();
            Viewport.DrawerPanels();

            // ⑤ The south drawer's browser, recorded before the north drawer's Control Centre so the
            //     Control Centre's own exclusions are the last thing declared and the two cannot disagree.
            //     🔴 The interior is asked for every tick and not cached — the drawer is springing, so the
            //     extent it offers is a different one on almost every tick of an open or a close.
            const PlaneExtent BrowserInterior = Viewport.Drawers().Interior(DrawerBearing::South);

            BrowserLedger.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);
            ContentBrowser.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);

            if (BrowserInterior.SpanAlong() > 0.0f && BrowserInterior.SpanAcross() > 0.0f)
            {
                Disregard(Viewport.Surface().Relayer(RecordingSurface::ShellLayer::Above));
                ContentBrowser.RecordBrowser(BrowserInterior, ContentSeated, ContentBrowserSeated);
                ContentBrowser.RecordDeferred(ContentBrowserSeated);

                // 🔴 Declared every tick or lost. Without it the drawer owns every contact inside its own
                //    body, so taking a record or dragging the lattice slides the drawer instead.
                ContentBrowser.Exclude(Viewport.Drawers(), DrawerBearing::South);
                Disregard(Viewport.Surface().Relayer(RecordingSurface::ShellLayer::Beneath));
            }

            const PlaneExtent ControlInterior = Viewport.Drawers().Interior(DrawerBearing::North);
            ControlCentre.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);
            Disregard(Viewport.Surface().Relayer(RecordingSurface::ShellLayer::Above));
            Disregard(ControlCentre.Record(ControlInterior, ControlCentreValues));

            // 📝 Compared rather than watched. The Control Centre writes the artist's choice straight into the
            //    ordinates, so the change is visible here as a difference and needs no callback to report it.
            {
                ThemeSelection Chosen;
                Chosen.Presented   = ControlCentreValues.Theme;
                Chosen.Primary     = ControlCentreValues.Primary;
                Chosen.Secondary   = ControlCentreValues.Secondary;
                Chosen.Information = ControlCentreValues.Information;
                Chosen.Warning     = ControlCentreValues.Warning;
                Chosen.Alert       = ControlCentreValues.Alert;

                const bool Altered = Chosen.Presented   != InscribedSelection.Presented
                                  || Chosen.Primary     != InscribedSelection.Primary
                                  || Chosen.Secondary   != InscribedSelection.Secondary
                                  || Chosen.Information != InscribedSelection.Information
                                  || Chosen.Warning     != InscribedSelection.Warning
                                  || Chosen.Alert       != InscribedSelection.Alert;

                // 🔴 The record is advanced whether the write was delivered or refused. A read-only folder would
                //    otherwise have every later tick retry the same refused write for the life of the process.
                if (Altered)
                {
                    Disregard(ThemeInterchange::RecordBeside(InvokedAs, Chosen));
                    InscribedSelection = Chosen;

                    // 🔴 Declared to the viewport, which re-anchors the whole appearance on the next tick, and
                    //    then pushed into the two panels that keep their own copy of the inks. The shell reads
                    //    the appearance through its own Reseat, which the viewport already calls.
                    Viewport.Retint(Chosen);
                    ContentBrowser.Reseat(Viewport.Appearance());
                }
            }
            ControlCentre.Exclude(Viewport.Drawers());
            Disregard(Viewport.Surface().Relayer(RecordingSurface::ShellLayer::Beneath));

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

    ControlCentre.Reset();
    WorkspacePanels.Reset();
    for (std::uint32_t Ordinal = 0u; Ordinal < WorkspaceIndex::WorkspaceCeiling; ++Ordinal)
        PanelPartitions[Ordinal].Reset();
    Workspace.Reset();
    Workspaces.Reset();
    Viewport.Reclaim();
    Lifetime.Reclaim();

    std::printf("%s \u2014 exited cleanly\n", HostName);

    // 🔴 Returned rather than only stated. A validation run needs an exit code, so that a serious arrival
    //    fails whatever invoked the host instead of scrolling past in a console nobody reads.
    return (Serious == 0u) ? 0 : 1;
}
