//============================================================================================================================================
//                                                             EDITORHOST.CPP
//============================================================================================================================================
// 🧩 The combined editor — every workspace subject in one host, with every device concern held by HostLifecycle.
//
// 🔴 LAYOUT RULE, READ BEFORE EDITING THIS HOST. The editor's layout is:
//      workspace windows (WorkspacePanel + WorkspaceIndex + the vendor dock)
//        → splittable panels (EditorPanel + PanelStructure: viewport | UV |
//          outliner | properties leaves, each with chrome and a footer)
//          → leaf content (SceneDirectoryPanel: the sky in a viewport leaf,
//            the outliner | details column in an outliner leaf, the
//            properties | history pages in a properties leaf).
//    `GlobalShellPanel` is the VALIDATION PROTOTYPE (the full reference sheet:
//    options rail, texture-paint layer stack, CAD drafting, fullscreen
//    inspector). It is recorded ONLY by InterfaceValidationHost. NEVER record
//    it here, and never port its rail/layer-stack/fullscreen strip into this
//    host — that mistake was made once and reverted. The validation viewport
//    stays black; the editor's sky lives in the viewport LEAF.

#include "Contract/DeliveryContract.h"
#include "Application/EditorHost/Api/CameraRig.h"
#include "Application/EditorHost/Api/SkyImage.h"
#include "SlateCompute/Compute/AtmosphereIntegrator/Api/AtmosphereIntegrator.h"
#include "SlateUI/Interface/ContentBrowserPanel/Api/ContentBrowserPanel.h"
#include "SlateUI/Interface/ControlCentrePanel/Api/ControlCentrePanel.h"
#include "SlateUI/Interface/ThemeInterchange/Api/ThemeInterchange.h"
#include "SlateUI/Interface/EditorPanel/Api/EditorPanel.h"
#include "SlateUI/Interface/SceneDirectoryPanel/Api/SceneDirectoryPanel.h"
#include "SlateUI/Interface/ViewportSequence/Api/ViewportSequence.h"
#include "SlateUI/Interface/WorkspacePanel/Api/WorkspaceIndex.h"
#include "SlateVulkan/Device/HostLifecycle/Api/HostLifecycle.h"
#include "SlateVulkan/Device/ViewportSkySurface/Api/ViewportSkySurface.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <cstring>

//------------------------------------------------------------------------------------------------------------------------
//                                                          FIGURES
//------------------------------------------------------------------------------------------------------------------------

namespace
{

using namespace Slate;

constexpr std::uint32_t InitialWidth  = 1280u;   // [px]
constexpr std::uint32_t InitialHeight = 720u;    // [px]

constexpr const char* WindowTitle = "Slate \u2014 Editor";
constexpr const char* HostName    = "EditorHost";

// 📝 The workspace ground the interface is recorded over. Stated here because it is the one visual decision
//    this host makes; everything else it presents belongs to a panel.
//------------------------------------------------------------------------------------------------------------------------
//                                                      THE THREE CEILINGS
//------------------------------------------------------------------------------------------------------------------------

// 🔴 Applying the content browser in the south drawer crossed all three of the budgets that have each, at
//    least once, taken a host down with no window and no log line. They are asserted here so a fourth panel
//    cannot repeat any of them silently.

// ① EASED INTERPOLANTS. `InteractionIndex::Register` draws two fades per control, and the integrator's supply
//    is shared by every ledger in the process — the browser's private ledger does not get its own pool.
constexpr std::uint32_t EasesPerControl = 2u;
constexpr std::uint32_t CentreControls  = ControlCentrePanel::ControlCapacity;
constexpr std::uint32_t BrowserControls = ContentBrowserPanel::RegistrationDemand;
constexpr std::uint32_t EditorControls  = PanelStructure::RecordCeiling * EditorPanel::ControlsPerRecord;
constexpr std::uint32_t SceneControls   = SceneDirectoryPanel::RegistrationDemand;
constexpr std::uint32_t BareEases       = 9u + 1u + 1u;   // [-] - Control Centre motions, shell carousel

constexpr std::uint32_t DemandedEases =
    ((CentreControls + BrowserControls + EditorControls + SceneControls) * EasesPerControl) + BareEases;

static_assert(DemandedEases <= MotionIntegrator::EaseCapacity,
              "this host's panels demand more eased interpolants than the integrator holds — the panel "
              "constructed last is rejected mid-registration and the host exits before its first frame; raise "
              "MotionIntegrator::EaseCapacity or reduce a panel's control count");

// ② LEDGER SLOTS. Counted per ledger, not per host. The browser is the only owner of its own
//    `BrowserLedger`, so only its demand is weighed here; the Control Centre answers to its private index.
static_assert(BrowserControls <= InteractionIndex::ControlCapacity,
              "the content browser registers more controls than one InteractionIndex holds — Construct is "
              "rejected with \"no further control slot\" and the south drawer opens onto blank ground");

static_assert(SceneControls <= InteractionIndex::ControlCapacity,
              "the scene directory registers more controls than one InteractionIndex holds — Construct is "
              "rejected with \"no further control slot\" and the editor opens without its scene directory");

// ③ AUTOMATIC STORAGE. A Windows thread is given one megabyte and a refusal here is not a refusal at all:
//    the guard page is touched in the prologue, so the process dies before a statement can report anything.
//    Linux hands out eight megabytes, which is exactly why no gate here can catch it.
constexpr std::size_t WindowsThreadStack = 1048576u;                  // [B] - the shipped linker default
constexpr std::size_t AutomaticCeiling   = WindowsThreadStack / 4u;   // [B] - a quarter, leaving room to call

static_assert(sizeof(WorkspaceIndex) + sizeof(WorkspacePanel) + sizeof(EditorPanel)
              + (sizeof(PanelStructure)       * WorkspaceIndex::WorkspaceCeiling)
              + (sizeof(EditorPanelConfiguration) * WorkspaceIndex::WorkspaceCeiling)
              + sizeof(ControlCentrePanel)  + sizeof(ControlCentreConfiguration)
              + sizeof(InteractionIndex)    + sizeof(ContentBrowserPanel)
              + sizeof(ContentBrowserConfiguration) + sizeof(ContentLibrary)
              + sizeof(InteractionIndex)    + sizeof(SceneDirectoryPanel)
              + sizeof(SceneDirectoryContext) <= AutomaticCeiling,
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
    InterfaceAttachment Incoming = {};

    Incoming.Instance                 = Offered.Instance;
    Incoming.ScoredDevice             = Offered.ScoredDevice;
    Incoming.ActiveDevice             = Offered.ActiveDevice;
    Incoming.GraphicsQueue            = Offered.GraphicsQueue;
    Incoming.GraphicsFamilyOrdinal    = Offered.GraphicsFamilyOrdinal;
    Incoming.ColourTargetFormat       = Offered.ColourTargetFormat;
    Incoming.MinimumDisplayImageCount = Offered.MinimumDisplayImageCount;
    Incoming.DisplayImageCount        = Offered.DisplayImageCount;
    Incoming.NativeWindowSlot         = Offered.NativeWindowSlot;

    return Incoming;
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

    if (!Lifetime.Construct(Declared).Resolved)
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

    if (!Viewport.Construct(Attach(Lifetime.Offering()), NorthDrawer, SouthDrawer).Resolved)
    {
        std::printf("%s \u2014 the viewport sequence was rejected\n", HostName);
        return 1;
    }

    // ─────────────────────────────────────────────────────────────────────────────────────────────────────
    //                                                       THE TICK LOOP
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────

    // 📝 🔴 Every refusal is resolved inside Await, before a display image is acquired. A tick that reports
    //    Recording has a recording open and cannot be abandoned; a tick that reports Idle has opened
    //    nothing. That is what makes the `continue` below safe — the arrangement this host previously got
    //    wrong five times over, returning to the top of the loop with a command buffer still recording.

    // ③ The workspaces this host opens. 🔴 The LEDGER owns them and the panel presents them — `14` §1
    //    forbids a panel from holding what it displays, and separating the two is the whole reason there
    //    are two components here rather than one.
    // 📝 The subject this host opens by default, named once so the startup registration and the strip's `+`
    //    cannot disagree about what a new workspace is.
    constexpr WorkspaceSubject DefaultSubject = WorkspaceSubject::Vacant;

    WorkspaceIndex          Workspaces;
    WorkspacePanel          Workspace;
    EditorPanel             WorkspacePanels;
    PanelStructure          PanelPartitions[WorkspaceIndex::WorkspaceCeiling];
    EditorPanelConfiguration    PanelConfiguration[WorkspaceIndex::WorkspaceCeiling];
    ControlCentrePanel      ControlCentre;
    ControlCentreConfiguration  ControlCentreValues;
    SceneDirectoryPanel     SceneDirectory;
    SceneDirectoryContext   SceneApplied;
    InteractionIndex        SceneLedger;
    ViewportSkySurface      SkySurface;
    AtmosphereIntegrator    SkyIntegrator;
    CameraRig               FlyRig;
    std::vector<std::uint8_t> SkyPixels;
    SkyCamera               SkyCam;
    EnvironmentConfiguration SkyPrevious;
    bool                    SkyEverGenerated = false;
    std::uintptr_t          SkyTextureIdentity = 0u;
    bool                    SkyRegistered = false;

    // 📐 The editor's scene directory — the sun and sky the viewport renders, registered under the
    //    Lighting grouping. `Sun` and `Sky` are the two appended `EntitySubject` ordinals, so the
    //    inspector's slider cards branch on them while every reference entity keeps its g_NN identity.
    static constexpr EntityRow EditorEntities[7] =
    {
        { "Level_01_City",           EntitySubject::Level,      0u, 0xFFFFFFFFu, 3u },
        { "Lighting",                EntitySubject::Grouping,   1u,  0u,         2u },
        { "Directional Light (Sun)", EntitySubject::Sun,        2u,  1u,         0u },
        { "Sky Atmosphere",          EntitySubject::Sky,        2u,  1u,         0u },
        { "Environment",             EntitySubject::Grouping,   1u,  0u,         1u },
        { "Post Process Volume",     EntitySubject::Actor,      2u,  4u,         0u },
        { "Editor Camera",           EntitySubject::Camera,     1u,  0u,         0u }
    };

    // 📝 The editor's own history run, drained from the shell's one-slot demand at drag end.
    EntityRevision EditorRevisions[8] = {};
    std::uint32_t  EditorRevisionCount = 0u;
    FontLoader                  Fonts;

    InteractionIndex         BrowserLedger;

    // 📝 The south drawer's owner. The library is the HOST's, not the panel's — `14` §1 forbids a panel
    //    from holding what it displays, which is the same separation WorkspaceIndex and WorkspacePanel keep.
    ContentBrowserPanel      ContentBrowser;
    ContentBrowserConfiguration  ContentBrowserApplied;
    ContentLibrary           ContentApplied;

    // 📝 The appearance file sits beside the executable and is read once, before any panel is recorded. A
    //    first run has no file yet, which is the ordinary case and not a fault — the build's own appearance
    //    stands and the first colour the artist changes writes the file.
    const char* const InvokedAs = (ArgumentCount > 0) ? ArgumentValues[0] : "";
    const std::filesystem::path ExecutablePath = InvokedAs[0] != '\0'
                                               ? std::filesystem::absolute(InvokedAs)
                                               : std::filesystem::current_path();
    const std::string FontRoot = (ExecutablePath.parent_path() / "EngineContent" / "FontArchives").string();

    {
        ThemeSelection Recorded;

        if (ThemeInterchange::AdoptBeside(InvokedAs, Recorded))
        {
            ControlCentreValues.Theme       = Recorded.Current;
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
    InscribedSelection.Current   = ControlCentreValues.Theme;
    InscribedSelection.Primary     = ControlCentreValues.Primary;
    InscribedSelection.Secondary   = ControlCentreValues.Secondary;
    InscribedSelection.Information = ControlCentreValues.Information;
    InscribedSelection.Warning     = ControlCentreValues.Warning;
    InscribedSelection.Alert       = ControlCentreValues.Alert;

    // 🔴 Declared BEFORE any panel is constructed. Panels that copy their inks do so out of the appearance the
    //    viewport hands them at Construct, so a selection declared afterwards would leave the first frames
    //    drawn in the transcription's own theme and only correct itself on the artist's first colour change.
    Viewport.Retint(InscribedSelection);

    // 📝 Which dock node the next registered workspace is applied into; zero means the main dock space.
    std::uint32_t  RegisterIntoNode = 0u;

    Viewport.Surface().ApplyFontLoader(Fonts);
    Discard(Fonts.Discover(FontRoot.c_str()));
    // 📝 The family carousel's preview faces are added to the atlas BEFORE the first tick records. Added
    //    during recording instead, the faces would land in an atlas the renderer had already uploaded and
    //    the preview tiles would draw from stale texture data.
    Discard(Fonts.PreparePreviews(1.0f));
    Discard(Fonts.Load(FontRoot.c_str(), Viewport.Appearance().Fonts, 1.0f));
    ControlCentre.SetFontFamilies(Fonts);
    // 📝 Seat the family carousel on the family the appearance names. Without this the carousel opened
    //    on ordinal zero (the alphabetically first family) while the loaded faces were the appearance's
    //    own — and the role strips draw the LOADED family's faces, so the two have to agree at bring-up.
    for (std::uint32_t Ordinal = 0u; Ordinal < Fonts.FamilyCount(); ++Ordinal)
        if (Fonts.FamilyName(Ordinal) != nullptr &&
            std::strcmp(Fonts.FamilyName(Ordinal), Viewport.Appearance().Fonts.Family) == 0)
        {
            ControlCentreValues.Font = Ordinal;
            break;
        }


    if (!Workspace.Construct(Viewport.Surface(), Viewport.Appearance()).Resolved)
    {
        std::printf("%s \u2014 the workspace panel was rejected\n", HostName);
        return 1;
    }

    if (!WorkspacePanels.Construct(Viewport.MotionSource(), Viewport.Surface(), Viewport.Appearance()).Resolved)
    {
        std::printf("%s \u2014 the editor panels were rejected\n", HostName);
        return 1;
    }

    if (!ControlCentre.Construct(Viewport.MotionSource(), Viewport.Surface(), Viewport.Appearance()).Resolved)
    {
        std::printf("%s \u2014 the Control Centre panel was rejected\n", HostName);
        return 1;
    }

    if (!BrowserLedger.Construct(Viewport.MotionSource()).Resolved)
    {
        std::printf("%s \u2014 the content browser ledger was rejected\n", HostName);
        return 1;
    }

    // 📝 The editor's sun and sky arrive presented, so the viewport draws the sky from the very first
    //    frame and the inspector edits the same ordinates.
    SceneApplied.EnvironmentPresented = true;
    SceneApplied.Environment.SunElevation    = 35.0;
    SceneApplied.Environment.SunAzimuth      = 120.0;
    SceneApplied.Environment.SunIntensity    = 4.8;
    SceneApplied.Environment.SunTemperature  = 5500.0;
    SceneApplied.Environment.SkyIntensity    = 1.0;
    SceneApplied.Environment.SkyTurbidity    = 2.0;
    SceneApplied.Environment.AtmosphereDensity = 1.0;
    SceneApplied.Environment.AtmosphereScaleHeight = 1.0;
    SceneApplied.EntityTaken = 2u;   // [-] - the sun, taken at bring-up

    // 📝 The editor camera, registered as the seventh row. Its details' options are the camera's own:
    //    bit 1 is the camera lag, bit 2 the inverted pitch — the lag arrives enabled so the camera
    //    eases out of the gate, and the pitch arrives un-inverted (the standard fly-cam convention).
    SceneApplied.DetailBits[6u] = 2u;
    SceneApplied.CameraSpeed = 50.0;
    FlyRig.YawDegrees   = SceneApplied.Environment.SunAzimuth - 20.0;
    FlyRig.PitchDegrees = 15.0;
    FlyRig.Position[0]  = 0.0;
    FlyRig.Position[1]  = 1.5;
    FlyRig.Position[2]  = 0.0;
    FlyRig.Snap();
    SceneApplied.CameraPosition[0] = 0.0;
    SceneApplied.CameraPosition[1] = 1.5;
    SceneApplied.CameraPosition[2] = 0.0;
    SceneApplied.CameraRotation[0] = FlyRig.YawDegrees;
    SceneApplied.CameraRotation[1] = FlyRig.PitchDegrees;

    if (!SceneLedger.Construct(Viewport.MotionSource()).Resolved)
    {
        std::printf("%s \u2014 the scene directory ledger was rejected\n", HostName);
        return 1;
    }

    if (!SceneDirectory.Construct(SceneLedger, Viewport.MotionSource(), Viewport.Surface(),
                                  Viewport.Appearance()).Resolved)
    {
        std::printf("%s \u2014 the scene directory was rejected\n", HostName);
        return 1;
    }

    // 📝 The viewport's sky: one device texture, uploaded from the CPU atmosphere evaluation. The
    //    texture identity is registered with the interface's own Vulkan backend, so the shell draws
    //    it through the same sampled-image path the font atlas uses.
    if (!SkySurface.Construct(Lifetime.DeviceExchange(), Lifetime.DiagnosticsExtension()).Resolved)
    {
        std::printf("%s \u2014 the viewport sky surface was rejected\n", HostName);
        return 1;
    }

    {
        // 🔴 The interface context is current after Viewport.Construct, and the backend is
        //    initialised — this is the only window in which the texture may be registered. The
        //    registration itself lives in the interface's own translation unit, which is the engine's
        //    single place the vendor is spelled.
        SkyTextureIdentity = Viewport.Surface().RegisterSampledImage(SkySurface.Sampler(), SkySurface.View());
        SkyRegistered = SkyTextureIdentity != 0u;
    }

    // 🔴 The browser carries its OWN ledger, as every panel here does, so its registration cannot exhaust the
    //    Control Centre's. Read — an registration refusal is silent at the call site and a browser that was
    //    rejected records nothing at all, which reads as a drawer that opens onto blank ground.
    if (!ContentBrowser.Construct(BrowserLedger, Viewport.Surface()).Resolved)
    {
        std::printf("%s \u2014 the content browser was rejected\n", HostName);
        return 1;
    }

    // 🔴 The browser takes no appearance at Construct — it is applied here, once the viewport has resolved one.
    ContentBrowser.Reapply(Viewport.Appearance());

    ApplyReferenceContent(ContentApplied);

    // 📝 🔴 The editor opens a VACANT workspace, where the painting host opens a canvas. This is the one
    //    thing that distinguishes the two hosts, and it is the reason there are two: the editor carries
    //    every subject and cannot presume which the artist wants, so it presents a blank one and lets them
    //    say. A host that guessed would open a canvas for someone who came to sketch.
    const Outcome<std::uint32_t> DefaultWorkspace = Workspaces.Register(DefaultSubject);
    if (!DefaultWorkspace.Resolved)
    {
        std::printf("%s \u2014 the default workspace could not be opened\n", HostName);
        return 1;
    }
    PanelPartitions[DefaultWorkspace.Resolve()].Construct(PanelSubject::Viewport);

    // 🔴 The sheet's tab figures applied into the vendor's style, including the four `Patches/` adds. They
    //    default to 0.0f, at which a patched build draws stock rectangular tabs — so this call is what
    //    turns the trapezoid on.
    if (!Viewport.Seam().ApplyWorkspaceStyle(Viewport.Appearance().WorkspaceMeasure,
                                                 Viewport.Appearance().Workspace).Resolved)
    {
        std::printf("%s \u2014 the workspace style was not applied\n", HostName);
    }

    std::printf("%s \u2014 opened %s\n", HostName, Workspaces.ActiveTitle());

    while (Lifetime.Active())
    {
        const TickPass Pass = Lifetime.Await(WorkspaceGround);
        Discard(Fonts.FlushPending());

        if (Pass.Current == TickCondition::Closed)
            break;

        // 🔴 Phase one of a device rebuild. The device STILL STANDS here, so this is the one moment the
        //    interface can release its descriptor pool, font atlas and pipelines against a live handle.
        //    Reclaiming after the rebuild idles a device the vendor has already destroyed, which the
        //    loader reports as VUID-vkDeviceWaitIdle-device-parameter.
        if (Pass.DeviceRetiring)
        {
            Viewport.Reclaim();
            SkySurface.Reclaim();
            SkyRegistered = false;
            SkyTextureIdentity = 0u;
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
            if (!Viewport.Construct(Attach(Lifetime.Offering()), NorthDrawer, SouthDrawer).Resolved)
            {
                std::printf("%s \u2014 the interface could not be rebuilt on the recovered device\n", HostName);
                break;
            }

            // 🔴 The sky texture's image, view and sampler died with the old device, and the interface's
            //    own descriptor pool was rebuilt with it — so the texture is re-created and re-registered
            //    against the fresh backend, exactly as the font atlas is.
            if (SkySurface.Construct(Lifetime.DeviceExchange(), Lifetime.DiagnosticsExtension()).Resolved)
            {
                SkyTextureIdentity = Viewport.Surface().RegisterSampledImage(SkySurface.Sampler(), SkySurface.View());
                SkyRegistered = SkyTextureIdentity != 0u;
                SkyPrevious = {};   // [-] - the next tick regenerates and uploads
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
                std::printf("%s \u2014 the interface rejected the restated image counts\n", HostName);
            }
        }

        if (Pass.Current != TickCondition::Recording)
            continue;

        // ④ Build the interface tick. A refusal here abandons the tick's content, and the recording is
        //    still surrendered — an empty rendering scope presents the cleared ground, which is correct
        //    and is what the artist sees for one tick.
        if (Viewport.Advance(Pass.ElapsedMilliseconds).Resolved)
        {
            // 🔴 The workspace is recorded FIRST and the drawers over it. One background draw list, so
            //    the order of recording IS the z-order — and the previous arrangement recorded the
            //    workspace after `RecordDrawers`, which painted the whole surface over the control
            //    centre and the asset browser.
            const PlaneExtent Whole = Spanning(0.0f, 0.0f,
                                               static_cast<float>(Pass.Width),
                                               static_cast<float>(Pass.Height));

            Discard(Workspace.Record(Whole, Workspaces.ActiveTitle()));

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

            // 📝 The node the previous tick's `+` named, so the workspace it registered is applied into the
            //    strip the artist actually pressed rather than always into the main dock space.
            const std::uint32_t ApplyInto = RegisterIntoNode;

            RegisterIntoNode = 0u;

            WorkspacePanels.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);

            for (std::uint32_t Ordinal = 0u; Ordinal < OpenCount; ++Ordinal)
            {
                const char* Titled = Workspaces.Titled(Ordinal);

                if (Titled == nullptr)
                    continue;

                bool Current = true;

                const PlaneExtent PanelExtent = Viewport.Seam().EnterWorkspaceWindow(
                    Titled, !Workspaces.Applied(Ordinal), ApplyInto, Current);
                Workspaces.Apply(Ordinal);

                if (PanelExtent.Width() > 0.0f && PanelExtent.Height() > 0.0f)
                {
                    Discard(Viewport.Surface().SwitchToWindow());
                    Discard(WorkspacePanels.Record(PanelExtent,
                                                      PanelPartitions[Ordinal],
                                                      PanelConfiguration[Ordinal],
                                                      Ordinal));

                    // 📝 The leaf content — the editor's scene directory inside the workspace's own
                    //    panels. Recorded into the same window the panel chrome was, so it clips and
                    //    orders with it: the sky fills a viewport leaf, the outliner | details fills
                    //    an outliner leaf, and the properties | history fills a properties leaf.
                    //    Panels draw their content only while they exist in the partition; there is
                    //    no fullscreen scene directory in this host (see the header's layout rule).
                    for (std::uint32_t Leaf = 0u; Leaf < WorkspacePanels.LeafCount(); ++Leaf)
                    {
                        const PlaneExtent LeafBody = WorkspacePanels.LeafBody(Leaf);

                        switch (WorkspacePanels.LeafSubject(Leaf))
                        {
                            case PanelSubject::Viewport:
                                SceneDirectory.RecordViewportSky(LeafBody, SceneApplied);
                                break;
                            case PanelSubject::Outliner:
                                SceneDirectory.RecordOutliner(LeafBody, SceneApplied,
                                                              EditorEntities, 7u);
                                break;
                            case PanelSubject::Properties:
                                SceneDirectory.RecordProperties(LeafBody, SceneApplied,
                                                                EditorEntities, 7u,
                                                                EditorRevisions, EditorRevisionCount);
                                break;
                            default:
                                break;
                        }
                    }

                    if (WorkspacePanels.PointerCaptured(Ordinal))
                        Viewport.Seam().WithholdPointer();
                }

                Viewport.Seam().LeaveWorkspaceWindow();
                Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Beneath));

                // ⚠️ Recorded, never acted on inside the sweep. Withdrawing here edits the set being walked.
                if (!Current)
                    Withdrawing = Ordinal;
            }

            if (Withdrawing < OpenCount)
            {
                Discard(Workspaces.Withdraw(Withdrawing));
                WorkspacePanels.WithdrawPresentation(Withdrawing);
                for (std::uint32_t Moving = Withdrawing; Moving + 1u < OpenCount; ++Moving)
                {
                    PanelPartitions[Moving] = PanelPartitions[Moving + 1u];
                    PanelConfiguration[Moving]   = PanelConfiguration[Moving + 1u];
                }
                PanelPartitions[OpenCount - 1u].Reset();
                PanelConfiguration[OpenCount - 1u] = EditorPanelConfiguration{};
            }

            // 📝 The `+`, applied inside the dock node's own tab bar so the vendor lays it after the last
            //    tab — always at the end, by construction rather than by arithmetic.
            std::uint32_t AskingNode = 0u;

            if (Viewport.Seam().RecordWorkspaceAddition(Workspace.Strip(), OpenCount, AskingNode))
            {
                // 🔴 The asking node is carried to the NEXT tick, because the workspace it registers is not
                //    recorded until then. Applying it against the main space instead is what put a new
                //    workspace in the wrong window.
                RegisterIntoNode = AskingNode;
                const Outcome<std::uint32_t> RegisteredWorkspace = Workspaces.Register(DefaultSubject);
                if (RegisteredWorkspace.Resolved)
                    PanelPartitions[RegisteredWorkspace.Resolve()].Construct(PanelSubject::Viewport);
            }

            // 🔴 With nothing open there is no tab bar to seat a `+` in, so the empty shell carries the
            //    invitation itself. `WorkspacePanel` draws "CREATE PANEL" on plain black; a press anywhere
            //    on that ground registers one, which is the way out of a state that otherwise has none.
            if (OpenCount == 0u && Viewport.Seam().VacantPressed(Whole))
            {
                const Outcome<std::uint32_t> RegisteredWorkspace = Workspaces.Register(DefaultSubject);
                if (RegisteredWorkspace.Resolved)
                    PanelPartitions[RegisteredWorkspace.Resolve()].Construct(PanelSubject::Viewport);
            }

            // 📝 The drawers last, so they sit ABOVE the workspace as the sheet lays them.
            // ④·b The scene directory — the shared ledger is advanced here, once, before the panel
            //      samples it; the panel's own Advance only samples, and a second advance would retire
            //      the release before the leaves read it.
            SceneLedger.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);
            SceneDirectory.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);

            // 📝 The fly camera: the seam's held keys and look gesture drive the rig, and the lagged
            //    pose becomes the viewport crop. The sky dome is direction-indexed and camera-independent,
            //    so looking around needs no regeneration — the crop alone moves, which is the whole
            //    point of drawing the sky as a dome rather than as one pinhole image.
            {
                const CameraCondition FlyInput = Viewport.Seam().CameraInput();

                CameraSettings FlySettings;
                FlySettings.FlySpeed    = SceneApplied.CameraSpeed;
                FlySettings.LagEnabled  = (SceneApplied.DetailBits[6u] & 2u) != 0u;
                FlySettings.InvertPitch = (SceneApplied.DetailBits[6u] & 4u) != 0u;

                FlyRig.Advance(Pass.ElapsedMilliseconds / 1000.0, FlyInput, FlySettings);

                SceneApplied.ViewportSkyCamera.AzimuthDegrees    = static_cast<float>(FlyRig.LaggedYawDegrees);
                SceneApplied.ViewportSkyCamera.ElevationDegrees  = static_cast<float>(FlyRig.LaggedPitchDegrees);
                SceneApplied.ViewportSkyCamera.FieldOfViewDegrees = 60.0f;
                SceneApplied.CameraPosition[0] = FlyRig.LaggedPosition[0];
                SceneApplied.CameraPosition[1] = FlyRig.LaggedPosition[1];
                SceneApplied.CameraPosition[2] = FlyRig.LaggedPosition[2];
                SceneApplied.CameraRotation[0] = FlyRig.LaggedYawDegrees;
                SceneApplied.CameraRotation[1] = FlyRig.LaggedPitchDegrees;
            }

            // 📝 The sky image is regenerated and uploaded only when the environment actually changed
            //    (the sliders write at drag end, so this runs at most once per drag), and the identity
            //    is handed to the shell every tick so the viewport draws the texture.
            if (SkyRegistered && SceneApplied.EnvironmentPresented)
            {
                const bool SkyAltered = !SkyEverGenerated ||
                                        std::memcmp(&SkyPrevious, &SceneApplied.Environment,
                                                    sizeof(EnvironmentConfiguration)) != 0;
                if (SkyAltered)
                {
                    // 📐 The viewport frames the lit side of the scene: the camera turns with the sun,
                    //    which keeps the disc in frame as the artist drags the azimuth slider.
                    SkyCam.AzimuthDegrees = SceneApplied.Environment.SunAzimuth - 20.0;
                    // 📐 The camera's elevation is fixed, so the artist SEES the sun rise and set as the
                    //    elevation slider moves; a camera that rose with the sun would hold the disc at
                    //    one screen height and a drag would read as no change at all. The ground plane
                    //    remains in the lower frame, and the shell's crop shifts to keep a sun above
                    //    the camera's view pinned near the top edge.
                    SkyCam.ElevationDegrees = 15.0;
                    if (GenerateSkyImage(SkyIntegrator, SceneApplied.Environment, SkyCam,
                                         ViewportSkySurface::SkyWidth, ViewportSkySurface::SkyHeight,
                                         SkyPixels).Resolved)
                        static_cast<void>(SkySurface.Upload(SkyPixels.data()));
                    SkyPrevious = SceneApplied.Environment;
                    SkyEverGenerated = true;
                }
                SceneApplied.SkyTextureIdentity = SkyTextureIdentity;
            }
            else
            {
                SceneApplied.SkyTextureIdentity = 0u;
            }

            // 📝 The history demand is drained ONCE per drag — the shell raises it at drag end, the host
            //    appends it to its own run and clears the slot, and no tick in between wrote a revision.
            if (SceneApplied.RevisionDemandSlot.Standing && EditorRevisionCount < 8u)
            {
                EntityRevision& Written = EditorRevisions[EditorRevisionCount++];
                Written.Description = SceneApplied.RevisionDemandSlot.Caption;
                Written.Secondary   = SceneApplied.RevisionDemandSlot.Secondary;
                Written.TimeRun     = "now";
                Written.Author      = "Artist";
                Written.Against     = SceneApplied.RevisionDemandSlot.Against;
                Written.Classified  = RevisionSubject::Parameter;
                SceneApplied.RevisionDemandSlot = {};
            }

            Viewport.RecordDrawers();
            Viewport.DrawerPanels();

            // ⑤ The south drawer's browser, recorded before the north drawer's Control Centre so the
            //     Control Centre's own exclusions are the last thing declared and the two cannot disagree.
            //     🔴 The interior is asked for every tick and not cached — the drawer is springing, so the
            //     extent it offers is a different one on almost every tick of an open or a close.
            const PlaneExtent BrowserInterior = Viewport.Drawers().Interior(DrawerBearing::South);

            BrowserLedger.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);
            ContentBrowser.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);

            if (BrowserInterior.Width() > 0.0f && BrowserInterior.Height() > 0.0f)
            {
                Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Above));
                ContentBrowser.RecordBrowser(BrowserInterior, ContentApplied, ContentBrowserApplied);
                ContentBrowser.RecordDeferred(ContentBrowserApplied);

                // 🔴 Declared every tick or lost. Without it the drawer owns every contact inside its own
                //    body, so taking a record or dragging the lattice slides the drawer instead.
                ContentBrowser.Exclude(Viewport.Drawers(), DrawerBearing::South);
                Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Beneath));
            }

            const PlaneExtent ControlInterior = Viewport.Drawers().Interior(DrawerBearing::North);
            ControlCentre.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);
            // 📝 The artist's per-role weights are declared every tick so the workspace's panels read the
            //    current choice; the viewport re-states them after each resolve.
            Viewport.ApplyTypographyWeights(ControlCentreValues.TypographyWeight);
            Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Above));
            Discard(ControlCentre.Record(ControlInterior, ControlCentreValues));

            // 📝 Compared rather than watched. The Control Centre writes the artist's choice straight into the
            //    ordinates, so the change is visible here as a difference and needs no callback to report it.
            {
                ThemeSelection Chosen;
                Chosen.Current   = ControlCentreValues.Theme;
                Chosen.Primary     = ControlCentreValues.Primary;
                Chosen.Secondary   = ControlCentreValues.Secondary;
                Chosen.Information = ControlCentreValues.Information;
                Chosen.Warning     = ControlCentreValues.Warning;
                Chosen.Alert       = ControlCentreValues.Alert;
                if (ControlCentreValues.Font < Fonts.FamilyCount() && Fonts.FamilyName(ControlCentreValues.Font) != nullptr)
                    std::strncpy(Chosen.FontFamily, Fonts.FamilyName(ControlCentreValues.Font), sizeof(Chosen.FontFamily) - 1u);

                // 🔴 Only the family re-runs the font pipeline. The other members are colours and reach
                //    every panel through the appearance; re-loading fonts for them would re-rasterise
                //    the whole atlas on every theme or colour edit.
                const bool FamilyAltered = std::strcmp(Chosen.FontFamily, InscribedSelection.FontFamily) != 0;
                const bool Altered = Chosen.Current   != InscribedSelection.Current
                                  || Chosen.Primary     != InscribedSelection.Primary
                                  || Chosen.Secondary   != InscribedSelection.Secondary
                                  || Chosen.Information != InscribedSelection.Information
                                  || Chosen.Warning     != InscribedSelection.Warning
                                  || Chosen.Alert       != InscribedSelection.Alert
                                  || std::strcmp(Chosen.FontFamily, InscribedSelection.FontFamily) != 0;

                // 🔴 The record is advanced whether the write was delivered or rejected. A read-only folder would
                //    otherwise have every later tick retry the same rejected write for the life of the process.
                if (Altered)
                {
                    Discard(ThemeInterchange::RecordBeside(InvokedAs, Chosen));
                    InscribedSelection = Chosen;

                    // 🔴 Declared to the viewport, which re-anchors the whole appearance on the next tick, and
                    //    then pushed into the two panels that keep their own copy of the inks. The shell reads
                    //    the appearance through its own Reapply, which the viewport already calls.
                    Viewport.Retint(Chosen);
                    Discard(Viewport.Seam().ApplyWorkspaceStyle(
                        Viewport.Appearance().WorkspaceMeasure,
                        Viewport.Appearance().Workspace));
                    ContentBrowser.Reapply(Viewport.Appearance());
                    if (FamilyAltered)
                        Fonts.RequestLoad(FontRoot.c_str(), Viewport.Appearance().Fonts, 1.0f);
                }
            }
            ControlCentre.Exclude(Viewport.Drawers());
            Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Beneath));

            if (Viewport.SealPanels().Resolved)
            {
                // 🔴 Read. A rejected Record presents the cleared ground with nothing on it, which is
                //    indistinguishable from a panel that drew nothing, so the refusal is named here.
                if (!Viewport.Record(Pass.Recording))
                {
                    std::printf("%s \u2014 the interface content was not recorded\n", HostName);
                }
            }
            else
            {
                Discard(Viewport.Abandon());
            }
        }
        else
        {
            Discard(Viewport.Abandon());
        }

        // ⑤ Close the scope, submit, present, advance. A rejected present re-establishes the chain rather
        //    than ending the loop.
        if (!Lifetime.Complete().Resolved)
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
