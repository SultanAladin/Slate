//============================================================================================================================================
//                                                     INTERFACEVALIDATIONHOST.CPP
//============================================================================================================================================
// 🧩 Records the control sheet and reusable global-interface components for direct visual comparison.

#include "Contract/DeliveryContract.h"
#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "SlateUI/Interface/ComponentSpecification/Api/ComponentSpecification.h"
#include "SlateUI/Interface/ControlCentrePanel/Api/ControlCentrePanel.h"
#include "SlateUI/Interface/ThemeInterchange/Api/ThemeInterchange.h"
#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"
#include "SlateUI/Interface/EditorPanel/Api/EditorPanel.h"
#include "SlateUI/Interface/FacetPanel/Api/FacetPanel.h"
#include "SlateUI/Interface/GlobalShellPanel/Api/GlobalShellPanel.h"
#include "SlateUI/Interface/InteractionIndex/Api/InteractionIndex.h"
#include "SlateUI/Interface/InterfaceExchange/Api/InterfaceExchange.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"
#include "SlateUI/Interface/ContentBrowserPanel/Api/ContentBrowserPanel.h"
#include "SlateUI/Interface/LayerStackPanel/Api/LayerStackPanel.h"
#include "SlateUI/Interface/LayerStackSpecification/Api/LayerStackSpecification.h"
#include "SlateUI/Interface/MotionIntegrator/Api/MotionIntegrator.h"
#include "SlateVulkan/Device/HostLifecycle/Api/HostLifecycle.h"

#include <cstdio>
#include <cstring>

//------------------------------------------------------------------------------------------------------------------------
//                                                          FIGURES
//------------------------------------------------------------------------------------------------------------------------

namespace
{

using namespace Slate;

constexpr std::uint32_t InitialWidth  = 1280u;   // [px]
constexpr std::uint32_t InitialHeight = 900u;    // [px] - the sheet's six cards do not fit in 720

constexpr const char* WindowTitle = "Slate \u2014 Interface Validation";
constexpr const char* HostName    = "InterfaceValidationHost";

// 📐 🔴 The sheet declares `scale-110` on its own column. That is a property of the reference page and not of
//    the controls, so it is **not** folded into AuthoredReduction — it arrives here, as the artist scale, which
//    is exactly the seam a real application would expose to its own preference.
constexpr double SheetColumnScale = 1.10;   // [-] - scale-110

// 📝 The sheet's own page ground, #050505, as four unit ordinates. HostLifecycle clears the colour target
//    to this before the host records anything over it.
constexpr float PageGroundInk[4] = { 0.0196f, 0.0196f, 0.0196f, 1.0f };   // [-]

/// 🧩 Copies the device handles across the layer seam into the attachment the interface declares.
/// note  🔴 `SlateVulkan` cannot name `InterfaceAttachment` — it lives one layer above — so `HostLifecycle`
///        offers the same handles as `DeviceOffering` and the host performs the copy.
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

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT THE SHEET SEATS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every datum the sheet presents, applied at the value the sheet itself states.
/// note  🔴 The host owns these and the panel does not. `14` §1's gate is visible here as an ordinary struct:
///       every control below is handed a reference into this record and writes through it.
struct ValidationConfiguration
{
    std::uint32_t  SelectionTaken = 0u;      // [-]   - "Entry name"
    double         Degree         = 123.0;   // [deg] - value="123"
    double         Percent        =  85.0;   // [%]   - value="85"
    double         Pixel          = 123.0;   // [px]  - value="123"
    double         Rotation       =   0.0;   // [deg] - rotationValue = 0
    bool           Snapping       = true;    // [-]   - data-checked="true"
    bool           GridLines      = false;   // [-]
    bool           AspectLocked   = false;   // [-]
    bool           EntryOne       = true;    // [-]   - data-checked="true"
    bool           EntryTwo       = false;   // [-]
    bool           EntryThree     = true;    // [-]   - data-checked="true"
    bool           EntryFour      = false;   // [-]
    std::uint32_t  SizeTaken       = 2u;      // [-]   - the taken stop is L
    bool           InspectorDocked = true;    // [-]   - reference switch begins taken
    std::uint32_t  WorkspaceTaken  = 1u;      // [-]   - Texture Paint
    std::uint32_t  InspectorTaken  = 0u;      // [-]   - Properties
    bool           TransformOpen   = true;    // [-]   - folding property card
    std::uint32_t  ShadingTaken    = 0u;      // [-]   - dropdown selection
    PickerColour   Albedo          = { 214u, 216u, 222u, 255u };   // [-] - HSV colour picker
    bool           OutlineExpanded[5] = { true, true, true, true, true };   // [-] - branch disclosure
    bool           OutlineTaken[5]    = { false, true, true, false, false };   // [-] - additive multi-selection
    bool           OutlinePresent[5]  = { true, true, true, false, true };   // [-] - row presence action
    std::uint32_t  OutlineEnclosure[5] = { 5u, 0u, 1u, 1u, 0u };   // [-] - enclosing record; five is root
    std::uint32_t  OutlineOrder[5]     = { 0u, 0u, 0u, 1u, 1u };   // [-] - sibling position
    bool           FacetEnabled[14]    = { true, true, true, true, true, false, false,
                                           true, true, true, true, false, false, false };   // [-] - active filters
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE REFERENCE SHELL'S LEVEL
//------------------------------------------------------------------------------------------------------------------------

// 📐 `initialGameGraph` from `components/GameOutliner.tsx`, linearised in presentation order. The reference's
//    `g_NN` tokens are the ordinals here, so the two can be read against each other row for row.
constexpr EntityRow LevelEntities[14] =
{
    /* g_01 */ { "Level_01_City",           EntitySubject::Level,      0u, 0xFFFFFFFFu, 4u },
    /* g_02 */ { "Lighting",                EntitySubject::Grouping,   1u,  0u,         2u },
    /* g_03 */ { "Directional Light (Sun)", EntitySubject::Illuminant, 2u,  1u,         0u },
    /* g_04 */ { "Sky Atmosphere",          EntitySubject::Illuminant, 2u,  1u,         0u },
    /* g_05 */ { "Player_Start",            EntitySubject::Trigger,    1u,  0u,         0u },
    /* g_06 */ { "Main Camera",             EntitySubject::Camera,     1u,  0u,         0u },
    /* g_07 */ { "Environment",             EntitySubject::Grouping,   1u,  0u,         3u },
    /* g_08 */ { "Building_A_Prefab",       EntitySubject::Actor,      2u,  6u,         0u },
    /* g_09 */ { "Building_B_Prefab",       EntitySubject::Actor,      2u,  6u,         0u },
    /* g_10 */ { "Street_Prop_FireHydrant", EntitySubject::Actor,      2u,  6u,         0u },
    /* g_11 */ { "Systems",                 EntitySubject::Grouping,   1u,  0u,         3u },
    /* g_12 */ { "GameManager",             EntitySubject::Script,     2u, 10u,         0u },
    /* g_13 */ { "Ambient_City_Noise",      EntitySubject::Audio,      2u, 10u,         0u },
    /* g_14 */ { "Dust_Motes_VFX",          EntitySubject::Particle,   2u, 10u,         0u }
};

// 📐 `initialRevisions` from `lib/store.tsx`, linearised against the outline ordinals above. The reference
//    mints a date per revision and formats it at record time; the two runs are stated here already
//    formatted, because a panel that presents a time it also computes owns a datum it should not.
constexpr EntityRevision LevelRevisions[9] =
{
    { "Level created",         "Bracket_Rev4",        "09:12", "A. Marner", 0u, RevisionSubject::Start     },
    { "Lighting group added",  "3 emitters enclosed", "09:40", "A. Marner", 1u, RevisionSubject::Grouped   },
    { "Sun angle relocated",   "Pitch 42.5 deg",      "10:05", "A. Marner", 2u, RevisionSubject::Relocate  },
    { "Intensity raised",      "3.2 to 4.8",          "10:21", "R. Okonjo", 2u, RevisionSubject::Parameter },
    { "Atmosphere authored",   "Rayleigh profile",    "10:44", "R. Okonjo", 3u, RevisionSubject::Feature   },
    { "Start volume placed",   "Player_Start",        "11:02", "A. Marner", 4u, RevisionSubject::Created   },
    { "Camera framing edited", "FOV 60 to 72",        "11:30", "R. Okonjo", 5u, RevisionSubject::Amended   },
    { "Environment grouped",   "3 records enclosed",  "11:55", "A. Marner", 6u, RevisionSubject::Grouped   },
    { "Prefab dropped",        "Building_C removed",  "12:18", "R. Okonjo", 6u, RevisionSubject::Dropped   }
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE INTERPOLANT BUDGET
//------------------------------------------------------------------------------------------------------------------------

// 🔴 Stated here so the ceiling can never silently fall behind the demand again. Every panel below owns a
//    private `InteractionIndex` but they ALL draw from this host's single `MotionIntegrator`, and each
//    registered control costs TWO eased interpolants — a hover fade and a take fade. That doubling is what
//    made the arithmetic surprising: the ledgers were nowhere near their own 256 ceilings while the shared
//    ease pool was already empty. A panel that grows its control count now fails the build here, at the
//    line that states the budget, rather than at run time in whichever panel happens to be constructed last.
constexpr std::uint32_t SheetControls   = 31u;                 // [-] - RegisterEvery
constexpr std::uint32_t FacetControls   = 24u + 2u;            // [-] - FacetPanel::FacetCapacity + 2
constexpr std::uint32_t EditorControls  = 11u * 22u;           // [-] - RecordCeiling * ControlsPerRecord
constexpr std::uint32_t CentreControls  = 192u;                // [-] - ControlCentrePanel::ControlCapacity
constexpr std::uint32_t ShellControls   = GlobalShellPanel::RegistrationDemand;   // [-] - chrome, outline rows,
                                                                              //       layer rows, metadata
constexpr std::uint32_t StackControls   = LayerStackPanel::RegistrationDemand;   // [-] - rows, chrome,
                                                                             //       popups, revisions, card
constexpr std::uint32_t BrowserControls = ContentBrowserPanel::RegistrationDemand;   // [-] - sources, lattice, chrome

constexpr std::uint32_t EasesPerControl = 2u;                  // [-] - InteractionIndex::Register draws both fades
constexpr std::uint32_t BareEases       = 9u + 1u;             // [-] - Control Centre motions, shell carousel

constexpr std::uint32_t DemandedEases =
    ((SheetControls + FacetControls + EditorControls + CentreControls + ShellControls
      + StackControls + BrowserControls) * EasesPerControl)
    + BareEases;

static_assert(DemandedEases <= MotionIntegrator::EaseCapacity,
              "the host's construct chain demands more eased interpolants than the integrator holds — the "
              "panel constructed last will be rejected mid-registration and the window will retire before its "
              "first frame; raise MotionIntegrator::EaseCapacity or reduce a panel's control count");

// 🔴 The eased budget above was necessary but NOT sufficient, and the gap cost a whole bring-up. Ledger
//    SLOTS are a second, separate ceiling: `FacetPanel`, `EditorPanel` and `ControlCentrePanel` each own a
//    PRIVATE `InteractionIndex`, but the sheet, the reference shell and the layer stack all register into the
//    ONE `Ledger` declared below. That shared total is what overflowed — 31 + 128 + 240 = 399 against a
//    ceiling of 256 — so `LayerStack.Construct` was rejected with "no further control slot" and the host
//    printed its refusal and exited 1 before recording a single frame. Only the panels sharing the ledger
//    are counted here; a panel with its own ledger is weighed against its own capacity, not this one.
constexpr std::uint32_t SharedSlots = SheetControls + ShellControls + StackControls + BrowserControls;

static_assert(SharedSlots <= InteractionIndex::ControlCapacity,
              "the panels sharing this host's one InteractionIndex register more controls than it holds — the "
              "panel constructed last is rejected at bring-up and the host exits before its first frame; "
              "raise InteractionIndex::ControlCapacity or reduce a sharing panel's control count");

// 🔴 A THIRD ceiling, and the one that actually killed this host: automatic storage. A Windows thread is
//    given one megabyte, and a refusal here is not a refusal at all — the guard page is touched in the
//    prologue, so the process dies before any statement can report anything. The gates could never catch
//    it because Linux hands out eight megabytes. Anything above the stated fraction of a Windows stack
//    must live in static storage; this assert makes that a build error rather than a silent exit.
constexpr std::size_t WindowsThreadStack = 1048576u;   // [B] - the linker default the host is shipped with
constexpr std::size_t AutomaticCeiling   = WindowsThreadStack / 4u;   // [B] - a quarter, leaving room to call

static_assert(sizeof(MotionIntegrator) + sizeof(InteractionIndex) + sizeof(RecordingSurface) +
              sizeof(LayerStackPanel)  + sizeof(LayerStackContext) +
              sizeof(ContentBrowserPanel) + sizeof(ContentBrowserConfiguration) <= AutomaticCeiling,
              "this host's automatic UI members no longer fit a quarter of a Windows thread stack — the "
              "prologue's stack probe will fault before main runs a statement and the host will exit with "
              "no window and no log line; move the largest member to static storage as LayerArrangement "
              "and RevisionSequence already are");

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE REFERENCE SHELL'S STACK
//------------------------------------------------------------------------------------------------------------------------

// 📐 `mockLayers` from `components/TexturePaint.tsx`, transcribed verbatim and in its own order. The reference
//    mints an `id` per layer; the ordinal is that identity here, on the same terms as the outliner's `g_NN`.
constexpr LayerRow StackLayers[4] =
{
    /* 1 */ { "Edge Wear",  LayerClassification::Paint,     "Multiply",  78u, 0xF97316u, 0xEAB308u,
              true,  92u, false, { "Base Color", "Roughness", "Metallic" },           3u },
    /* 2 */ { "Dirt Pass",  LayerClassification::Material,  "Overlay",   45u, 0x8B5CF6u, 0xEC4899u,
              true, 100u, true,  { "Base Color", "Roughness" },                       2u },
    /* 3 */ { "Scratches",  LayerClassification::Paint,     "Screen",    60u, 0xF97316u, 0x06B6D4u,
              false, 100u, false, { "Base Color", "Bump" },                           2u },
    /* 4 */ { "Base Metal", LayerClassification::Material,  "Normal",   100u, 0x8B5CF6u, 0x3B82F6u,
              false, 100u, false, { "Base Color", "Roughness", "Metallic", "Bump" },  4u }
};

/// 🧩 Every identity the sheet's controls are registered under, claimed once at bring-up.
struct ValidationIdentities
{
    ControlIdentity  Selection    = {};
    ControlIdentity  Degree       = {};
    ControlIdentity  Percent      = {};
    ControlIdentity  Pixel        = {};
    ControlIdentity  Rotation     = {};
    ControlIdentity  Snapping     = {};
    ControlIdentity  GridLines    = {};
    ControlIdentity  AspectLocked = {};
    ControlIdentity  EntryOne     = {};
    ControlIdentity  EntryTwo     = {};
    ControlIdentity  EntryThree   = {};
    ControlIdentity  EntryFour    = {};
    ControlIdentity  Size         = {};
    ControlIdentity  TooltipLight  = {};
    ControlIdentity  TooltipDark   = {};
    ControlIdentity  InspectorDock = {};
    ControlIdentity  WorkspaceMode = {};
    ControlIdentity  InspectorTabs = {};
    ControlIdentity  TransformFold = {};
    ControlIdentity  ShadingMenu   = {};
    ControlIdentity  AlbedoPicker       = {};
    ControlIdentity  OutlineRows[5]     = {};
    ControlIdentity  OutlineExpansion[5] = {};
};

/// 🧩 Reservations every identity the sheet needs, refusing in full rather than in part.
/// out   Result  [-]  refuses with ExtentExhausted when the ledger declines any requested identity
/// note  🔴 A partial registration would leave one control reading another's fade, which draws correctly on the
///       first tick and diverges on the second — the hardest possible shape of defect to attribute.
Outcome<ValidationIdentities> RegisterEvery(InteractionIndex& Ledger)
{
    ValidationIdentities  Target;
    ControlIdentity*      Every[] = {
        &Target.Selection, &Target.Degree,     &Target.Percent,    &Target.Pixel,
        &Target.Rotation,  &Target.Snapping,   &Target.GridLines,  &Target.AspectLocked,
        &Target.EntryOne,  &Target.EntryTwo,   &Target.EntryThree, &Target.EntryFour,
        &Target.Size,      &Target.TooltipLight, &Target.TooltipDark,
        &Target.InspectorDock, &Target.WorkspaceMode, &Target.InspectorTabs, &Target.TransformFold,
        &Target.ShadingMenu, &Target.AlbedoPicker,
        &Target.OutlineRows[0], &Target.OutlineRows[1], &Target.OutlineRows[2],
        &Target.OutlineRows[3], &Target.OutlineRows[4],
        &Target.OutlineExpansion[0], &Target.OutlineExpansion[1], &Target.OutlineExpansion[2],
        &Target.OutlineExpansion[3], &Target.OutlineExpansion[4]
    };

    for (ControlIdentity* Target : Every)
    {
        const Outcome<ControlIdentity> Registered = Ledger.Register();

        if (!Registered.Resolved)
        {
            return Outcome<ValidationIdentities>::Refuse(Registered.Error);
        }

        *Target = Registered.Resolve();
    }

    return Outcome<ValidationIdentities>::Result(Target);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE MEASURE OVERLAY
//------------------------------------------------------------------------------------------------------------------------

#ifdef SLATE_DEBUG

/// 🧩 One control's recorded extent, retained for the overlay to stroke over it.
/// note  🔍 Debug only. Nothing in a shipped build registers, retains or records any of this.
struct MeasuredExtent
{
    const char*  Naming  = "";   // [-] - static text; never allocated
    PlaneExtent  Where   = {};   // [px] - what the control was handed
    float        Target = 0.0f; // [px] - what the sheet declares it should span across, already reduced
};

/// 🧩 Retains what each control was arranged at, so the overlay can compare it against the sheet.
/// note  🔍 The comparison is the point: "exact" is checkable rather than asserted. A control whose extent
///       disagrees with its declared figure by more than half a pixel is reported in the pointer ink.
class MeasureOverlay
{
public:

    static constexpr std::uint32_t MeasuredCeiling = 32u;   // [-] - never allocated

    void Retain(const char* Naming, const PlaneExtent& Where, float Target)
    {
        if (MeasuredCount >= MeasuredCeiling)
            return;

        Measured[MeasuredCount].Naming  = Naming;
        Measured[MeasuredCount].Where   = Where;
        Measured[MeasuredCount].Target = Target;
        ++MeasuredCount;
    }

    void Discard()
    {
        MeasuredCount = 0u;
    }

    /// 🧩 Strokes every retained extent and reports the four factors the appearance was resolved by.
    void Record(RecordingSurface& Surface, const ThemeProfile& Appearance,
                double ArtistScale, float Width, std::uint32_t Disagreeing) const
    {
        const ControlColour&    Colours = Appearance.Control;
        const ControlMetric& Measure = Appearance.ControlMeasure;

        for (std::uint32_t Ordinal = 0u; Ordinal < MeasuredCount; ++Ordinal)
        {
            const MeasuredExtent& Held  = Measured[Ordinal];
            const float           Apart = Held.Where.Height() - Held.Target;
            const bool            Agreed = (Apart < 0.5f && Apart > -0.5f);

            Surface.Edge(Held.Where, Agreed ? Colours.RulerPointer : Colours.StopTaken, 1.0f, 0.0f, CornerNone);
        }

        // 📝 The header states every factor separately, so a wrong extent is attributable to which multiplier
        //    produced it without a debugger. A single product would say only that something is wrong.
        char Reading[192] = {};
        std::snprintf(Reading, sizeof(Reading),
                      "reduction %.2f  density %u  artist %.2f  applied %.3f  extent %.0f  measured %u  apart %u",
                      static_cast<double>(AuthoredReduction),
                      static_cast<unsigned>(Measure.Density),
                      ArtistScale,
                      static_cast<double>(Measure.AppliedFactor),
                      static_cast<double>(Width),
                      static_cast<unsigned>(MeasuredCount),
                      static_cast<unsigned>(Disagreeing));

        Surface.TextRun(12.0f, 12.0f, Colours.RulerPointer, Reading, Measure.RowText, 0.0f, true);
    }

    /// 🧩 How many retained extents disagree with the figure the sheet declares for them.
    std::uint32_t Disagreeing() const
    {
        std::uint32_t Counted = 0u;

        for (std::uint32_t Ordinal = 0u; Ordinal < MeasuredCount; ++Ordinal)
        {
            const float Apart = Measured[Ordinal].Where.Height() - Measured[Ordinal].Target;

            if (Apart >= 0.5f || Apart <= -0.5f)
                ++Counted;
        }

        return Counted;
    }

private:

    MeasuredExtent  Measured[MeasuredCeiling] = {};   // [-] - never allocated
    std::uint32_t   MeasuredCount             = 0u;   // [-]
};

#endif   // SLATE_DEBUG

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

    // 🔴 Requested in EVERY configuration. `Build\Construct.bat` produces Release by default, so gating
    //    this compiled the validation layer out of the binary that is actually run — and every run then
    //    reported itself unwatched, which is the one answer indistinguishable from a clean one.
    Declared.DiagnosticRequested = true;

    HostLifecycle Lifetime;

    if (!Lifetime.Construct(Declared).Resolved)
        return 1;

    // ② 🔴 The interface, the integrator, the ledger and the panel — **not** `ViewportSequence`. The sheet
    //    declares no drawers, and constructing two of them to hold both closed forever would be recording
    //    chrome nothing in the reference has, which is the opposite of what a validation host is for.
    InterfaceExchange Interface;

    if (!Interface.Construct(Attach(Lifetime.Offering())).Resolved)
    {
        std::printf("%s \u2014 the interface context was rejected\n", HostName);
        return 1;
    }

    MotionIntegrator Motion;
    InteractionIndex Ledger;
    RecordingSurface       Surface;
    FontLoader             Fonts;
    ComponentSpecification  Panel;
    ControlPanel             ReferenceControls;
    FacetPanel               Facets;
    EditorPanel              EditorPanels;
    PanelStructure           EditorPartition;
    EditorPanelConfiguration     EditorConfiguration;
    ControlCentrePanel       ControlCentre;
    ControlCentreConfiguration   ControlCentreValues;

    // 📝 The appearance file sits beside the executable and is read once, before any panel is recorded. A
    //    first run has no file yet, which is the ordinary case and not a fault — the build's own appearance
    //    stands and the first colour the artist changes writes the file.
    const char* const InvokedAs = (ArgumentCount > 0) ? ArgumentValues[0] : "";

    // 🔴 Resolve font archives relative to the executable, not the working directory.  Slate is not
    //    always launched from the repository root, so a relative path silently falls back to the ImGui
    //    default font.
    constexpr std::uint32_t FontPathCeiling = 512u;
    char FontArchivesPath[FontPathCeiling] = {};
    {
        const std::size_t Spanned = std::strlen(InvokedAs);
        std::size_t Folder = 0u;
        for (std::size_t Place = Spanned; Place > 0u; --Place)
        {
            const char Letter = InvokedAs[Place - 1u];
            if (Letter == '\\' || Letter == '/') { Folder = Place; break; }
        }
        if (Folder > 0u) std::memcpy(FontArchivesPath, InvokedAs, Folder);
        const char Leaf[] = "EngineContent/FontArchives";
        std::memcpy(FontArchivesPath + Folder, Leaf, sizeof(Leaf));
    }

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
    GlobalShellPanel         ReferenceShell;
    ShellContext           ShellApplied;

    // 📝 The ported `LayerstackV1` pane and the two property panels its inspector pairs with. The
    //    arrangement is applied from the reference once, then the artist amends it through the panel.
    LayerStackPanel          LayerStack;
    LayerStackContext      LayerStackApplied;

    // 📝 The ported `AsstbrowsrBasic` page — the sources aside, the record lattice and the inspector. The
    //    library is applied from the reference's own `ASSETS` run once, before the first tick.
    ContentBrowserPanel      ContentBrowser;
    ContentBrowserConfiguration  ContentBrowserApplied;
    ContentLibrary           ContentApplied;

    // 🔴 `static`, and that is not a style choice. `LayerArrangement` is 157 KB and `RevisionSequence`
    //    retains sixteen whole arrangements against its undo ring, which is 2.5 MB — together they are
    //    2.7 MB of automatic storage. A Windows thread is given ONE megabyte by default, so declaring
    //    these on the stack overflows the guard page in the function prologue: MSVC's `__chkstk` probe
    //    runs before the first statement, so the host dies with no window, no log line and no message —
    //    exactly the silent black console this host presented. Linux's 8 MB default hid the fault
    //    entirely, which is why it survived every gate. Static storage costs the same bytes in .bss,
    //    where their size is a link-time fact rather than a per-thread reservation.
    // 📝 The house rule forbids `new`/`delete` outside an extent slicer, so heap is not the answer here;
    //    the host is a single-instance executable and these three have exactly one lifetime.
    static LayerArrangement  LayerArranged;
    static RevisionSequence  LayerRevisions;

    if (const auto Verdict = Ledger.Construct(Motion); !Verdict.Resolved)
    {
        std::printf("%s \u2014 the interaction ledger was rejected: %s\n", HostName, Verdict.Error.Detail);
        std::fflush(stdout);
        return 1;
    }

    const Outcome<ValidationIdentities> Registered = RegisterEvery(Ledger);

    if (!Registered.Resolved)
    {
        std::printf("%s \u2014 the ledger rejected an registration: %s\n", HostName, Registered.Error.Detail);
        std::fflush(stdout);
        return 1;
    }

    const ValidationIdentities Target = Registered.Resolve();

    // 🔴 Seeded from what was transcribed beside the executable, so gate ⑱ and every sheet above it come up in
//    the recorded theme rather than in the transcription's own and correcting themselves a tick later.
ThemeSelection          Selected   = InscribedSelection;
ThemeProfile Appearance = ResolveTinted(1.0, SheetColumnScale, 0.0f, Selected);
std::strncpy(Appearance.Fonts.Family, Selected.FontFamily, sizeof(Appearance.Fonts.Family) - 1u);
ApplyUserScale(Appearance,
               static_cast<float>(ControlCentreValues.TypographySize[3]) / 14.0f,
               static_cast<float>(ControlCentreValues.Radius) / 24.0f);
ApplyFontWeights(Appearance, ControlCentreValues.TypographyWeight);
    Discard(Interface.ApplyWorkspaceStyle(Appearance.WorkspaceMeasure, Appearance.Workspace));
    Surface.ApplyTypographyScale(Appearance.TextScale);
    Surface.ApplyCornerScale(Appearance.CornerScale);
    Surface.ApplyFontLoader(Fonts);
    Discard(Fonts.Discover(FontArchivesPath));
    // 📝 The family carousel's preview faces are added to the atlas BEFORE the first tick records. Added
    //    during recording instead, the faces would land in an atlas the renderer had already uploaded and
    //    the preview tiles would draw from stale texture data.
    Discard(Fonts.PreparePreviews(1.0f));
    ControlCentre.SetFontFamilies(Fonts);
    Discard(Fonts.Load(FontArchivesPath, Appearance.Fonts, 1.0f));

    // Every construct refusal below is reported WITH its detail and flushed before the return. A refusal
    //    that printed only a headline and left the text in a buffered stdout was invisible: the window is
    //    already open by this point, so the host appeared to "open white and crash" when it had in fact
    //    stated its reason and exited 1. Naming the stage and flushing it is what makes the next one legible.
    const auto Rejected = [](const char* Stage, const Refusal& Rejected) -> int
    {
        std::printf("%s \u2014 %s was rejected: %s\n", HostName, Stage, Rejected.Detail);
        std::fflush(stdout);
        return 1;
    };

    if (const auto Verdict = Panel.Construct(Ledger, Surface, Appearance); !Verdict.Resolved)
        return Rejected("the control panel", Verdict.Error);

    if (const auto Verdict = ReferenceControls.Construct(Ledger, Surface, Appearance); !Verdict.Resolved)
        return Rejected("the reference controls", Verdict.Error);

    if (const auto Verdict = Facets.Construct(Motion, Surface, Appearance); !Verdict.Resolved)
        return Rejected("the facet panel", Verdict.Error);

    if (const auto Verdict = EditorPanels.Construct(Motion, Surface, Appearance); !Verdict.Resolved)
        return Rejected("the editor panels", Verdict.Error);

    EditorPartition.Construct(PanelSubject::Viewport);

    if (const auto Verdict = ControlCentre.Construct(Motion, Surface, Appearance); !Verdict.Resolved)
        return Rejected("the Control Centre panel", Verdict.Error);

    // 🔴 The reference shell is constructed LAST and recorded FIRST. It occupies the whole display, and the
    //    validation sheet is the page that scrolls beneath it — so its registrations are claimed after every
    //    other panel's, and nothing below it can take a contact the shell's own chrome stands over.
    // 🔴 Being last also makes it the first to starve: every earlier panel draws two eased interpolants per
    //    registered control from the ONE integrator, so a ceiling that fits the others exactly refuses here.
    if (const auto Verdict = ReferenceShell.Construct(Ledger, Motion, Surface, Appearance); !Verdict.Resolved)
        return Rejected("the reference shell", Verdict.Error);

    // 📝 The layer stack carries its own inks and lengths from `LayerstackV1` rather than from
    //    ThemeProfile, because the reference states them absolutely — but it shares the one
    //    interaction ledger, so its registrations are counted in the interpolant budget above.
    if (const auto Verdict = LayerStack.Construct(Ledger, Surface, Appearance); !Verdict.Resolved)
        return Rejected("the layer stack", Verdict.Error);

    if (const auto Verdict = ContentBrowser.Construct(Ledger, Surface); !Verdict.Resolved)
        return Rejected("the content browser", Verdict.Error);

    // 📝 The reference's own `ASSETS` run, applied once. The panel amends what the artist takes; it never
    //    amends the run itself, so this is the only write the library ever receives.
    ApplyReferenceContent(ContentApplied);

    // 🔴 The seat is read rather than dropped. A rejected seat leaves the arrangement empty, and an empty
    //    stack draws as a bare pane — indistinguishable from a panel that recorded nothing.
    if (const auto Verdict = ApplyReferenceArrangement(LayerArranged); !Verdict.Resolved)
        return Rejected("the layer arrangement", Verdict.Error);

    // What the sheet applies, and the runs it presents — the sole owner of every datum below.
    ValidationConfiguration Applied;

    const char* SelectionOptions[] = { "Entry name", "Second Entry", "Third Entry" };
    const char* SizeStops[]        = { "S", "M", "L", "XL" };

    SelectionDeclaration Selection;
    Selection.Caption     = "Selection";
    Selection.Options     = SelectionOptions;
    Selection.OptionCount = 3u;

    MagnitudeDeclaration Degree;
    Degree.Caption   = "Degree";
    Degree.UnitGlyph = "\u00B0";

    MagnitudeDeclaration Percent;
    Percent.Caption   = "Percent";
    Percent.UnitGlyph = "%";

    MagnitudeDeclaration Pixel;
    Pixel.Caption   = "Pixel";
    Pixel.UnitGlyph = "px";

    RulerDeclaration Rotation;
    Rotation.Caption   = "Rotation";
    Rotation.UnitGlyph = "\u00B0";

    ToggleDeclaration Snapping;     Snapping.Caption     = "Enable Snapping";
    ToggleDeclaration GridLines;    GridLines.Caption    = "Show Grid Lines";
    ToggleDeclaration AspectLocked; AspectLocked.Caption = "Lock Aspect Ratio";

    SubsetDeclaration EntryOne;   EntryOne.Caption   = "Entry one";
    SubsetDeclaration EntryTwo;   EntryTwo.Caption   = "Entry two";
    SubsetDeclaration EntryThree; EntryThree.Caption = "Entry three";
    SubsetDeclaration EntryFour;  EntryFour.Caption  = "Entry four";

    StopDeclaration Size;
    Size.Caption   = "Size";
    Size.Stops     = SizeStops;
    Size.StopCount = 4u;

    constexpr const char* TooltipBody =
        "Try connecting to another server. In case of a repeated error, please wait, "
        "if nothing happens, try to write a letter to the post office.";

    TooltipDeclaration TooltipLight;
    TooltipLight.Title      = "Tooltip";
    TooltipLight.Body       = TooltipBody;
    TooltipLight.Figure     = SymbolSubject::BulbFilament;
    TooltipLight.Appearance = TooltipAppearance::Light;

    TooltipDeclaration TooltipDark = TooltipLight;
    TooltipDark.Appearance         = TooltipAppearance::Dark;

    const char* WorkspaceCaptions[] = { "Drafting", "Texture Paint", "Game Editor" };
    const char* InspectorCaptions[] = { "Properties", "History" };

    SwitchDeclaration InspectorDock;
    InspectorDock.Caption = "Dock Inspector";

    SegmentDeclaration WorkspaceMode;
    WorkspaceMode.Captions     = WorkspaceCaptions;
    WorkspaceMode.CaptionCount = 3u;

    TabDeclaration InspectorTabs;
    InspectorTabs.Captions     = InspectorCaptions;
    InspectorTabs.CaptionCount = 2u;

    const char* TransformRuns[] = { "Position", "Rotation", "Scale" };
    const char* ShadingOptions[] = { "Smooth", "Faceted", "Flat" };
    const char* PropertyCards[]  = { "Record · 2 fields", "Transform · 3 fields", "Appearance · 4 fields" };
    const char* RevisionCards[]  = { "Set Parameter · 10:42", "Translate SOL_Boss · 10:37", "Created SOL_Boss · 10:31" };

    CarouselDeclaration InspectorCarousel;
    InspectorCarousel.LeadingRuns   = PropertyCards;
    InspectorCarousel.LeadingCount  = 3u;
    InspectorCarousel.TrailingRuns  = RevisionCards;
    InspectorCarousel.TrailingCount = 3u;

    FoldDeclaration TransformFold;
    TransformFold.Caption   = "TRANSFORM";
    TransformFold.BodyRuns  = TransformRuns;
    TransformFold.BodyCount = 3u;

    DropdownDeclaration ShadingMenu;
    ShadingMenu.Caption     = "Shading";
    ShadingMenu.Options     = ShadingOptions;
    ShadingMenu.OptionCount = 3u;

    ColourPickerDeclaration AlbedoPicker;
    AlbedoPicker.Caption = "Albedo";

    const char* FacetOptions[14] = {
        "Base Colour", "Metallic", "Roughness", "Height", "Normal", "Opacity", "Emissive",
        "Ambient Occlusion", "Anisotropy", "Anisotropy Angle", "Clearcoat", "Refraction Index",
        "Sheen", "Subsurface"
    };
    const ThemeToken FacetColours[14] = {
        Covering(0xB87333u), Covering(0x8B5CF6u), Covering(0x3B82F6u), Covering(0x8A8A8Au),
        Covering(0x10B981u), Covering(0x94A3B8u), Covering(0xF59E0Bu), Covering(0x6B7280u),
        Covering(0x22D3EEu), Covering(0x0EA5E9u), Covering(0xE2E8F0u), Covering(0xA78BFAu),
        Covering(0xF472B6u), Covering(0xFB7185u)
    };
    FacetDeclaration FacetCard;
    FacetCard.Caption       = "Filters";
    FacetCard.Options       = FacetOptions;
    FacetCard.Colours          = FacetColours;
    FacetCard.OptionCount   = 14u;
    FacetCard.LockedOrdinal = 0u;

    OutlineDeclaration OutlineRows[5] = {
        { "Part",         0u, 2u, true,  true  },
        { "Bodies",       1u, 3u, true,  true  },
        { "SOL_Boss",     2u, 0u, true,  true  },
        { "SOL_Rib",      2u, 0u, true,  false },
        { "SOL_Housing",  1u, 0u, true,  true  }
    };

    RevisionDeclaration RevisionRows[3] = {
        { "Set Parameter",  "Radius = 6.25 mm", "10:42" },
        { "Translate SOL_Boss", "Moved 4.20 mm", "10:37" },
        { "Created SOL_Boss", "Initial condition", "10:31" }
    };

#ifdef SLATE_DEBUG
    MeasureOverlay Overlay;
    bool           OverlayShown = false;
#endif

    double ArtistScale     = SheetColumnScale;
    float  ResolvedAgainst = 0.0f;

    // 📝 🔴 The sheet is a scrolling page — `py-32` above and below a column that runs past 1000 px at the
    //    reduced scale. A host that recorded it into a fixed window would present the first four cards and
    //    silently lose the last two, which is exactly the kind of disagreement this host exists to catch.
    float ScrollY  = 0.0f;   // [px] - how far the column has been carried upward
    float ColumnMeasured = 0.0f;  // [px] - what the previous tick's column actually occupied

    std::printf("%s \u2014 running\n", HostName);

    // ─────────────────────────────────────────────────────────────────────────────────────────────────────
    //                                                       THE TICK LOOP
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────

    while (Lifetime.Active())
    {
        const TickPass Pass = Lifetime.Await(PageGroundInk);

        if (Pass.Current == TickCondition::Closed)
            break;

        // 🔴 The DEVICE was rebuilt, so every device handle the interface holds names an object the vendor
        //    has returned. The interface alone is reconstructed: the ledger, the panel and the recording
        //    surface hold no device handle, so retiring them would discard interaction state — the grab
        //    an artist is mid-drag on — over a rebuild that did not invalidate any of it.
        //    Tested before DisplayRecovered because a device rebuild raises both.
        if (Lifetime.DeviceRecovered())
        {
            Interface.Reclaim();

            if (!Interface.Construct(Attach(Lifetime.Offering())).Resolved)
            {
                std::printf("%s \u2014 the interface could not be rebuilt on the recovered device\n", HostName);
                break;
            }

            // 📝 The display recovery this rebuild also raised is consumed here; the reconstruction above
            //    already took the counts the new chain holds.
            static_cast<void>(Lifetime.DisplayRecovered());
        }

        // The chain was re-established; the interface is told the counts it now holds, exactly once.
        else if (Lifetime.DisplayRecovered())
        {
            const DeviceOffering Offered = Lifetime.Offering();
            // 🔴 Read, not discarded. An interface still holding the previous image counts records
            //    against a chain depth that no longer exists, and the vendor reports that as a
            //    descriptor mismatch several ticks later rather than as the resize that caused it.
            if (!Interface.Renegotiate(Offered.MinimumDisplayImageCount, Offered.DisplayImageCount))
            {
                std::printf("%s \u2014 the interface rejected the restated image counts\n", HostName);
            }
        }

        if (Pass.Current != TickCondition::Recording)
            continue;

        const double ElapsedMs = Pass.ElapsedMilliseconds;

        // ① Open the interface tick and adopt the surface. 🔴 A refusal here must NOT return to the top
        //    of the loop: Await has already acquired an image and opened a recording, and only Complete
        //    closes them. The tick records nothing and the cleared ground is presented instead.
        bool ContentBuilt = Interface.Advance().Resolved;

        if (ContentBuilt && !Surface.Adopt().Resolved)
        {
            Discard(Interface.Abandon());
            ContentBuilt = false;
        }

        if (ContentBuilt)
        {

        const DisplayCondition& Display = Surface.Display();

        // ② 🔴 Re-resolve only when a factor actually moved. Resolved unconditionally, every extent is
        //    recomputed sixty times a second to the same figures, and the density classification steps
        //    while a drag is live.
        if (Display.Width != ResolvedAgainst)
        {
            Appearance      = ResolveTinted(Display.DisplayScale, ArtistScale, Display.Width, Selected);
            std::strncpy(Appearance.Fonts.Family, Selected.FontFamily, sizeof(Appearance.Fonts.Family) - 1u);
            ApplyUserScale(Appearance,
                           static_cast<float>(ControlCentreValues.TypographySize[3]) / 14.0f,
                           static_cast<float>(ControlCentreValues.Radius) / 24.0f);
ApplyFontWeights(Appearance, ControlCentreValues.TypographyWeight);
            Discard(Interface.ApplyWorkspaceStyle(Appearance.WorkspaceMeasure, Appearance.Workspace));
    Surface.ApplyTypographyScale(Appearance.TextScale);
    Surface.ApplyCornerScale(Appearance.CornerScale);
    Discard(Fonts.Discover(FontArchivesPath));
    Discard(Fonts.PreparePreviews(1.0f));
    ControlCentre.SetFontFamilies(Fonts);
    Fonts.RequestLoad(FontArchivesPath, Appearance.Fonts, 1.0f);
            ResolvedAgainst = Display.Width;

            // 🔴 The shell holds its own scaled extents, so a resolve it is not told about leaves it
            //    arranging at the previous display's figures — every other panel reads the appearance
            //    through the borrowed reference and needs no such call.
            ReferenceShell.Reapply(Appearance);
        }

        // 📝 Re-stated every tick rather than only when the display or the theme moves: the per-role
        //    weights change without either factor moving, and every panel reads the appearance through the
        //    borrowed reference, so the strip's choice must land on the tick it was made.
        ApplyFontWeights(Appearance, ControlCentreValues.TypographyWeight);

        Motion.Advance(ElapsedMs);

        // 🔴 The shell's keymap is applied BEFORE anything is arranged, so a Tab and the arrangement it
        //    causes land in the same tick. Applied after, the artist sees one frame of the old
        //    presentation on every press.
        // 📝 Shift is read from the modifier condition rather than from a second key subject, because the
        //    exchange states Tab once and reports the modifiers standing with it; a `ShiftedSummon`
        //    enumerator would be a second spelling of the same arrival for every host to keep in step.
        static_cast<void>(ReferenceShell.AdvanceSummoning(ShellApplied,
                                                          Interface.KeyPressed(KeySubject::Summon),
                                                          Interface.KeyPressed(KeySubject::Withdraw),
                                                          Interface.Modifiers().Shifted));

        ReferenceShell.Advance(Surface.Pointer(), ElapsedMs);
        Panel.Advance(Surface.Pointer(), ElapsedMs);
        ReferenceControls.Advance(Surface.Pointer(), ElapsedMs);
        Facets.Advance(Surface.Pointer(), ElapsedMs);
        EditorPanels.Advance(Surface.Pointer(), ElapsedMs);
        ControlCentre.Advance(Surface.Pointer(), ElapsedMs);
        LayerStack.Advance(Surface.Pointer(), ElapsedMs);
        ContentBrowser.Advance(Surface.Pointer(), ElapsedMs);

        // 🔴 The layer stack's chords are applied BEFORE anything is arranged, on the same grounds as the
        //    shell's Tab above: applied afterwards, every press shows one frame of the previous
        //    arrangement. The whole roster is swept in one pass so that a chord the stack answers is not
        //    also answered by whatever else is listening for the same key.
        {
            const ModifierCondition Modifiers = Interface.Modifiers();

            // 📝 The search run takes what was typed only while it holds the keyboard, and the panel's own
            //    guard refuses every chord in that condition — so the two can never both consume a key.
            if (LayerStackApplied.RetentionHovered)
            {
                static_cast<void>(Interface.AcceptTyped(LayerStackApplied.Retention,
                                                       LayerStackContext::RetentionCeiling));

                if (Interface.KeyPressed(KeySubject::Retract))
                {
                    std::uint32_t Occupied = 0u;

                    while (Occupied + 1u < LayerStackContext::RetentionCeiling &&
                           LayerStackApplied.Retention[Occupied] != '\0')
                    {
                        ++Occupied;
                    }

                    if (Occupied > 0u)
                        LayerStackApplied.Retention[Occupied - 1u] = '\0';
                }

                if (Interface.KeyPressed(KeySubject::Withdraw))
                {
                    LayerStackApplied.Retention[0]    = '\0';
                    LayerStackApplied.RetentionHovered = false;
                }
            }
            else if (LayerStackApplied.Renaming != LayerStackCeiling::AbsentOrdinal)
            {
                static_cast<void>(Interface.AcceptTyped(LayerStackApplied.RenamingRun,
                                                       LayerStackContext::NamingCeiling));

                if (Interface.KeyPressed(KeySubject::Retract))
                {
                    std::uint32_t Occupied = 0u;

                    while (Occupied + 1u < LayerStackContext::NamingCeiling &&
                           LayerStackApplied.RenamingRun[Occupied] != '\0')
                    {
                        ++Occupied;
                    }

                    if (Occupied > 0u)
                        LayerStackApplied.RenamingRun[Occupied - 1u] = '\0';
                }

                // 📐 `commit(false)` on Escape — the naming is abandoned rather than written.
                if (Interface.KeyPressed(KeySubject::Withdraw))
                    LayerStackApplied.Renaming = LayerStackCeiling::AbsentOrdinal;
            }
            else if (LayerStackApplied.RevisionField != 0u)
            {
                // 📐 The unfolded revision card's comment and value fields, on the same terms as the two
                //    above: the field holding the keyboard consumes what was typed, and no chord reaches
                //    the arrangement while it does. `RevisionField` is `Ordinal * 2 + 1` for the comment
                //    and `+ 2` for the value, so the ordinal and the field both fall out of one reading.
                const std::uint32_t Field   = LayerStackApplied.RevisionField - 1u;
                const std::uint32_t Ordinal = Field / 2u;
                const bool          Reading = (Field % 2u) == 1u;

                if (Ordinal < LayerStackContext::RevisionCeiling)
                {
                    char* Written = Reading ? LayerStackApplied.RevisionReading[Ordinal]
                                            : LayerStackApplied.RevisionRemark[Ordinal];

                    static_cast<void>(Interface.AcceptTyped(Written,
                                                           LayerStackContext::RemarkCeiling));

                    if (Interface.KeyPressed(KeySubject::Retract))
                    {
                        std::uint32_t Occupied = 0u;

                        while (Occupied + 1u < LayerStackContext::RemarkCeiling &&
                               Written[Occupied] != '\0')
                        {
                            ++Occupied;
                        }

                        if (Occupied > 0u)
                            Written[Occupied - 1u] = '\0';
                    }
                }

                if (Interface.KeyPressed(KeySubject::Withdraw))
                    LayerStackApplied.RevisionField = 0u;
            }
            else
            {
                for (std::uint32_t Ordinal = 0u;
                     Ordinal < static_cast<std::uint32_t>(KeySubject::SubjectCount); ++Ordinal)
                {
                    const auto Subject = static_cast<KeySubject>(Ordinal);

                    // 📝 Tab belongs to the shell, which has already consumed it above.
                    if (Subject == KeySubject::Summon || Subject == KeySubject::Retract)
                        continue;

                    if (Interface.KeyPressed(Subject))
                    {
                        static_cast<void>(LayerStack.AcceptChord(Subject, Modifiers, LayerArranged,
                                                                LayerStackApplied, LayerRevisions));
                    }
                }
            }
        }

#ifdef SLATE_DEBUG
        Overlay.Discard();
#endif

        const ControlColour&    Colours = Appearance.Control;
        const ControlMetric& Measure = Appearance.ControlMeasure;

        // ③ The page ground, then the sheet's own column, centred.
        const PlaneExtent Page = Spanning(0.0f, 0.0f, Display.Width, Display.Height);

        Surface.Ground(Page, Colours.PageGround, 0.0f, CornerNone);

        const float ColumnX  = (Measure.ColumnX < Display.Width - Measure.PagePad * 2.0f)
                                 ? Measure.ColumnX
                                 : Display.Width - Measure.PagePad * 2.0f;
        const float ColumnMinimum  = (Display.Width - ColumnX) * 0.5f;

        // 📐 The wheel carries the column, and the travel is held between zero and whatever the column
        //    overruns the display by. Clamped against the **previous** tick's measured extent, because this
        //    tick's is not known until every card has been arranged — and a scroll clamped against a stale
        //    extent by one tick is invisible, where an unclamped one scrolls into empty space forever.
        const float Overrun = (ColumnMeasured > Display.Height)
                            ? (ColumnMeasured - Display.Height) : 0.0f;

        ScrollY -= Surface.Pointer().WheelY * Measure.CardGapY * 2.0f;
        ScrollY  = (ScrollY < 0.0f) ? 0.0f
                      : (ScrollY > Overrun) ? Overrun : ScrollY;

        float Cursor = Measure.PagePad - ScrollY;

        // 📝 Every card below is arranged from its own row extents and then recorded, so the arrangement the
        //    overlay measures and the arrangement the control was handed are the same object by construction.
        const auto AdvanceCard = [&](const float* RowExtents, std::uint32_t RowCount) -> CardArrangement
        {
            const CardArrangement Arranged = Panel.ArrangeCard(ColumnMinimum, Cursor, ColumnX,
                                                               RowExtents, RowCount);
            Panel.RecordCard(Arranged);
            Cursor = Arranged.Enclosure.MaximumY + Measure.CardGapY;
            return Arranged;
        };

        const auto RowAt = [&](const CardArrangement& Card, const float* RowExtents,
                               std::uint32_t Ordinal) -> PlaneExtent
        {
            float X = Card.Interior.MinimumY;

            for (std::uint32_t Passed = 0u; Passed < Ordinal; ++Passed)
                X += RowExtents[Passed] + Card.RowGap;

            return Spanning(Card.Interior.MinimumX, X, Card.Interior.Width(), RowExtents[Ordinal]);
        };

        // ④ Card one — the selection field and the three magnitude rows.
        const float TopRows[4] = { Measure.FieldHeight, Measure.FieldHeight,
                                   Measure.FieldHeight, Measure.FieldHeight };
        const CardArrangement TopCard = AdvanceCard(TopRows, 4u);

        const PlaneExtent SelectionRow = RowAt(TopCard, TopRows, 0u);
        const PlaneExtent DegreeRow    = RowAt(TopCard, TopRows, 1u);
        const PlaneExtent PercentRow   = RowAt(TopCard, TopRows, 2u);
        const PlaneExtent PixelRow     = RowAt(TopCard, TopRows, 3u);

        Panel.SelectionField(Target.Selection, SelectionRow, Selection, Applied.SelectionTaken);
        Panel.MagnitudeRow(Target.Degree,  DegreeRow,  Degree,  Applied.Degree);
        Panel.MagnitudeRow(Target.Percent, PercentRow, Percent, Applied.Percent);
        Panel.MagnitudeRow(Target.Pixel,   PixelRow,   Pixel,   Applied.Pixel);

        // ⑤ Card two — the two tooltip triggers inside their well.
        const float TooltipRows[1] = { Measure.TooltipWellFloor };
        const CardArrangement TooltipCard = AdvanceCard(TooltipRows, 1u);
        const PlaneExtent     TooltipWell = RowAt(TooltipCard, TooltipRows, 0u);

        Surface.Ground(TooltipWell, Colours.WellGround, Measure.TooltipWellRadius, CornerAll);
        Surface.Edge(TooltipWell, Colours.CardEdge, Measure.CardEdgeWeight, Measure.TooltipWellRadius, CornerAll);

        // 📐 The sheet places its two triggers `items-end` with a gap of 32 units between them, and lifts each
        //    by `ml-8`. Both are applied against the well's lower padding, which is what items-end states.
        const float TriggerY = TooltipWell.MaximumY - Measure.TooltipWellInset - Measure.TriggerExtent;
        const float TriggerPair   = Measure.TriggerExtent * 2.0f + Measure.TooltipWellGap;
        const float TriggerTop  = TooltipWell.MinimumX + (TooltipWell.Width() - TriggerPair) * 0.5f;

        const PlaneExtent LightTrigger = Spanning(TriggerTop + Measure.TriggerLeadX, TriggerY,
                                                  Measure.TriggerExtent, Measure.TriggerExtent);
        const PlaneExtent DarkTrigger  = Spanning(TriggerTop + Measure.TriggerExtent + Measure.TooltipWellGap,
                                                  TriggerY, Measure.TriggerExtent, Measure.TriggerExtent);

        Panel.TooltipTrigger(Target.TooltipLight, LightTrigger, TooltipLight);
        Panel.TooltipTrigger(Target.TooltipDark,  DarkTrigger,  TooltipDark);

        // ⑥ Card three — the rotation ruler.
        const float RulerRows[1] = { Measure.FieldHeight + Measure.CardRowGap * 0.5f + Measure.RulerHeight };
        const CardArrangement RulerCard = AdvanceCard(RulerRows, 1u);

        Panel.RotationRuler(Target.Rotation, RowAt(RulerCard, RulerRows, 0u), Rotation, Applied.Rotation);

        // ⑦ Card four — the three toggles inside their well.
        const float ToggleWellHeight = Measure.ToggleRowHeight * 3.0f + Measure.WellGapY * 2.0f
                                     + Measure.WellInset * 2.0f;
        const float ToggleRows[1] = { ToggleWellHeight };
        const CardArrangement ToggleCard = AdvanceCard(ToggleRows, 1u);
        const PlaneExtent     ToggleWell = RowAt(ToggleCard, ToggleRows, 0u);

        Surface.Ground(ToggleWell, Colours.WellGround, Measure.WellRadius, CornerAll);
        Surface.Edge(ToggleWell, Colours.CardEdge, Measure.CardEdgeWeight, Measure.WellRadius, CornerAll);

        const auto WellRow = [&](const PlaneExtent& Well, float RowHeight, std::uint32_t Ordinal) -> PlaneExtent
        {
            return Spanning(Well.MinimumX + Measure.WellInset,
                            Well.MinimumY + Measure.WellInset +
                                static_cast<float>(Ordinal) * (RowHeight + Measure.WellGapY),
                            Well.Width() - Measure.WellInset * 2.0f, RowHeight);
        };

        Panel.ToggleRow(Target.Snapping,     WellRow(ToggleWell, Measure.ToggleRowHeight, 0u),
                        Snapping,     Applied.Snapping);
        Panel.ToggleRow(Target.GridLines,    WellRow(ToggleWell, Measure.ToggleRowHeight, 1u),
                        GridLines,    Applied.GridLines);
        Panel.ToggleRow(Target.AspectLocked, WellRow(ToggleWell, Measure.ToggleRowHeight, 2u),
                        AspectLocked, Applied.AspectLocked);

        // ⑧ Card five — the four multi-select rows inside their well.
        const float SubsetWellHeight = Measure.SubsetRowHeight * 4.0f + Measure.WellGapY * 3.0f
                                     + Measure.WellInset * 2.0f;
        const float SubsetRows[1] = { SubsetWellHeight };
        const CardArrangement SubsetCard = AdvanceCard(SubsetRows, 1u);
        const PlaneExtent     SubsetWell = RowAt(SubsetCard, SubsetRows, 0u);

        Surface.Ground(SubsetWell, Colours.WellGround, Measure.WellRadius, CornerAll);
        Surface.Edge(SubsetWell, Colours.CardEdge, Measure.CardEdgeWeight, Measure.WellRadius, CornerAll);

        Panel.SubsetRow(Target.EntryOne,   WellRow(SubsetWell, Measure.SubsetRowHeight, 0u),
                        EntryOne,   Applied.EntryOne);
        Panel.SubsetRow(Target.EntryTwo,   WellRow(SubsetWell, Measure.SubsetRowHeight, 1u),
                        EntryTwo,   Applied.EntryTwo);
        Panel.SubsetRow(Target.EntryThree, WellRow(SubsetWell, Measure.SubsetRowHeight, 2u),
                        EntryThree, Applied.EntryThree);
        Panel.SubsetRow(Target.EntryFour,  WellRow(SubsetWell, Measure.SubsetRowHeight, 3u),
                        EntryFour,  Applied.EntryFour);

        // ⑨ Card six — the magnitude stops.
        const float StopRows[1] = { Measure.StopStripHeight };
        const CardArrangement StopCard = AdvanceCard(StopRows, 1u);

        Panel.MagnitudeStops(Target.Size, RowAt(StopCard, StopRows, 0u), Size, Applied.SizeTaken);

        // ⑩ The general-purpose filter card — wrapped active chips, removal, clear-all, and the shared dropdown.
        const float FacetY = Facets.MeasureHeight(ColumnX, FacetCard, Applied.FacetEnabled);
        const PlaneExtent FacetExtent = Spanning(ColumnMinimum, Cursor, ColumnX, FacetY);
        Discard(Facets.Record(FacetExtent, FacetCard, Applied.FacetEnabled));
        Cursor = FacetExtent.MaximumY + Measure.CardGapY;

        // ⑪ The global-interface primitives — switch, segmented selector, inspector carousel, fold and dropdown.
        const float ReferenceRows[7] = { 32.0f, 38.0f, 31.0f, 154.0f, 129.0f, 124.0f, 341.0f };
        const CardArrangement ReferenceCard = AdvanceCard(ReferenceRows, 7u);

        ReferenceControls.SwitchToggle(Target.InspectorDock, RowAt(ReferenceCard, ReferenceRows, 0u),
                                       InspectorDock, Applied.InspectorDocked);
        ReferenceControls.SegmentedChoice(Target.WorkspaceMode, RowAt(ReferenceCard, ReferenceRows, 1u),
                                          WorkspaceMode, Applied.WorkspaceTaken);
        ReferenceControls.TabStrip(Target.InspectorTabs, RowAt(ReferenceCard, ReferenceRows, 2u),
                                   InspectorTabs, Applied.InspectorTaken);
        ReferenceControls.CarouselPages(Target.InspectorTabs, RowAt(ReferenceCard, ReferenceRows, 3u),
                                        InspectorCarousel, Applied.InspectorTaken);
        ReferenceControls.CollapsibleCard(Target.TransformFold, RowAt(ReferenceCard, ReferenceRows, 4u),
                                          TransformFold, Applied.TransformOpen);
        ReferenceControls.DropdownCard(Target.ShadingMenu, RowAt(ReferenceCard, ReferenceRows, 5u),
                                       ShadingMenu, Applied.ShadingTaken);
        ReferenceControls.ColourPicker(Target.AlbedoPicker, RowAt(ReferenceCard, ReferenceRows, 6u),
                                       AlbedoPicker, Applied.Albedo);

        // ⑫ One identity-backed outline. A drop's destination is declared here; document ownership stays outside
        //     the panel exactly as it does for selection and visibility.
        for (std::uint32_t RecordOrdinal = 0u; RecordOrdinal < 5u; ++RecordOrdinal)
        {
            OutlineRows[RecordOrdinal].EnclosedCount = 0u;
            if (Applied.OutlineEnclosure[RecordOrdinal] < 5u)
                ++OutlineRows[Applied.OutlineEnclosure[RecordOrdinal]].EnclosedCount;
        }

        std::uint32_t CurrentRecords[5] = {};
        std::uint32_t CurrentCount = 0u;
        const auto LinearizeOutline = [&](auto&& Traverse, std::uint32_t Enclosing, std::uint32_t Depth) -> void
        {
            for (std::uint32_t Position = 0u; Position < 5u; ++Position)
            {
                for (std::uint32_t RecordOrdinal = 0u; RecordOrdinal < 5u; ++RecordOrdinal)
                {
                    if (Applied.OutlineEnclosure[RecordOrdinal] != Enclosing ||
                        Applied.OutlineOrder[RecordOrdinal] != Position)
                        continue;

                    OutlineRows[RecordOrdinal].Depth = Depth;
                    CurrentRecords[CurrentCount++] = RecordOrdinal;
                    Traverse(Traverse, RecordOrdinal, Depth + 1u);
                }
            }
        };
        LinearizeOutline(LinearizeOutline, 5u, 0u);

        float OutlineExpansion[5] = {};
        for (std::uint32_t RecordOrdinal = 0u; RecordOrdinal < 5u; ++RecordOrdinal)
        {
            OutlineExpansion[RecordOrdinal] = ReferenceControls.OutlineExpansion(
                Target.OutlineExpansion[RecordOrdinal], Applied.OutlineExpanded[RecordOrdinal],
                OutlineRows[RecordOrdinal].AnimationEnabled);
        }

        float RowPresence[5] = {};
        float OutlineHeight = 0.0f;
        for (std::uint32_t CurrentOrdinal = 0u; CurrentOrdinal < CurrentCount; ++CurrentOrdinal)
        {
            const std::uint32_t RecordOrdinal = CurrentRecords[CurrentOrdinal];
            float Presence = 1.0f;
            std::uint32_t Enclosing = Applied.OutlineEnclosure[RecordOrdinal];
            std::uint32_t WalkCount = 0u;

            while (Enclosing < 5u && WalkCount++ < 5u)
            {
                Presence *= OutlineExpansion[Enclosing];
                Enclosing = Applied.OutlineEnclosure[Enclosing];
            }

            RowPresence[CurrentOrdinal] = Presence;
            OutlineHeight += 28.0f * Presence;
        }

        std::uint32_t DragSource = 5u;
        const float DragX = Surface.Pointer().PositionX - Ledger.OriginX();
        const float DragY = Surface.Pointer().PositionY - Ledger.OriginY();
        const bool DragTravelled = DragX * DragX + DragY * DragY >= 16.0f;

        if (DragTravelled)
        {
            for (std::uint32_t RecordOrdinal = 0u; RecordOrdinal < 5u; ++RecordOrdinal)
            {
                const bool BodyHeld = Ledger.Holding(Target.OutlineRows[RecordOrdinal]) &&
                                      Ledger.HeldPart(Target.OutlineRows[RecordOrdinal]) == ControlPart::Body;
                const bool BodyReleased = Ledger.Released(Target.OutlineRows[RecordOrdinal]) &&
                                          Ledger.ReleasedControlPart(Target.OutlineRows[RecordOrdinal]) == ControlPart::Body;
                if (BodyHeld || BodyReleased)
                    DragSource = RecordOrdinal;
            }
        }

        const float OutlineRowsHeight[1] = { OutlineHeight };
        const CardArrangement OutlineCard = AdvanceCard(OutlineRowsHeight, 1u);
        float OutlineCursor = OutlineCard.Interior.MinimumY;
        std::uint32_t DropTarget = 5u;
        OutlineDropPlacement DropPlacement = OutlineDropPlacement::Absent;

        for (std::uint32_t CurrentOrdinal = 0u; CurrentOrdinal < CurrentCount; ++CurrentOrdinal)
        {
            const std::uint32_t RecordOrdinal = CurrentRecords[CurrentOrdinal];
            const float Presence = RowPresence[CurrentOrdinal];
            if (Presence <= 0.0f)
                continue;

            const PlaneExtent Row = Spanning(OutlineCard.Interior.MinimumX, OutlineCursor,
                                             OutlineCard.Interior.Width(), 28.0f);
            OutlineDropPlacement RowPlacement = OutlineDropPlacement::Absent;

            if (DragSource < 5u && DragSource != RecordOrdinal &&
                Row.Encloses(Surface.Pointer().PositionX, Surface.Pointer().PositionY))
            {
                const float RowFraction = (Surface.Pointer().PositionY - Row.MinimumY) / Row.Height();
                RowPlacement = (RowFraction < 0.25f) ? OutlineDropPlacement::Before
                             : (RowFraction > 0.75f) ? OutlineDropPlacement::After
                                                     : OutlineDropPlacement::Enclosed;
                DropTarget = RecordOrdinal;
                DropPlacement = RowPlacement;
            }

            const PlaneExtent Revealed = Spanning(Row.MinimumX, Row.MinimumY,
                                                  Row.Width(), 28.0f * Presence);
            Surface.Confine(Revealed);
            ReferenceControls.OutlineRow(Target.OutlineRows[RecordOrdinal], Row, OutlineRows[RecordOrdinal], true,
                                         OutlineExpansion[RecordOrdinal], RowPlacement,
                                         Applied.OutlineExpanded[RecordOrdinal], Applied.OutlineTaken[RecordOrdinal],
                                         Applied.OutlinePresent[RecordOrdinal]);
            Surface.Release();
            OutlineCursor += 28.0f * Presence;
        }

        if (DragSource < 5u && DropTarget < 5u && Ledger.Released(Target.OutlineRows[DragSource]))
        {
            const std::uint32_t ProposedEnclosure = (DropPlacement == OutlineDropPlacement::Enclosed)
                                                   ? DropTarget : Applied.OutlineEnclosure[DropTarget];
            bool CycleDeclared = ProposedEnclosure == DragSource;
            std::uint32_t Walking = ProposedEnclosure;
            std::uint32_t WalkCount = 0u;

            while (!CycleDeclared && Walking < 5u && WalkCount++ < 5u)
            {
                CycleDeclared = Walking == DragSource;
                Walking = Applied.OutlineEnclosure[Walking];
            }

            if (!CycleDeclared)
            {
                const std::uint32_t DepartingEnclosure = Applied.OutlineEnclosure[DragSource];
                const std::uint32_t DepartingOrder = Applied.OutlineOrder[DragSource];
                for (std::uint32_t RecordOrdinal = 0u; RecordOrdinal < 5u; ++RecordOrdinal)
                {
                    if (RecordOrdinal != DragSource && Applied.OutlineEnclosure[RecordOrdinal] == DepartingEnclosure &&
                        Applied.OutlineOrder[RecordOrdinal] > DepartingOrder)
                        --Applied.OutlineOrder[RecordOrdinal];
                }

                std::uint32_t IncomingOrder = 0u;
                if (DropPlacement == OutlineDropPlacement::Enclosed)
                {
                    Applied.OutlineExpanded[DropTarget] = true;
                }
                else
                {
                    IncomingOrder = Applied.OutlineOrder[DropTarget];
                    if (DropPlacement == OutlineDropPlacement::After)
                        ++IncomingOrder;
                }

                for (std::uint32_t RecordOrdinal = 0u; RecordOrdinal < 5u; ++RecordOrdinal)
                {
                    if (RecordOrdinal != DragSource && Applied.OutlineEnclosure[RecordOrdinal] == ProposedEnclosure &&
                        Applied.OutlineOrder[RecordOrdinal] >= IncomingOrder)
                        ++Applied.OutlineOrder[RecordOrdinal];
                }

                Applied.OutlineEnclosure[DragSource] = ProposedEnclosure;
                Applied.OutlineOrder[DragSource] = IncomingOrder;
            }
        }

        // ⑬ The revision timeline, presented from newest to oldest.
        const float RevisionRowExtents[3] = { 54.0f, 54.0f, 54.0f };
        const CardArrangement RevisionCard = AdvanceCard(RevisionRowExtents, 3u);

        for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
        {
            ReferenceControls.RevisionRow(RowAt(RevisionCard, RevisionRowExtents, Ordinal),
                                          RevisionRows[Ordinal], Ordinal == 0u);
        }

        // ⑭ The reusable editor partition stands inside a workspace-sized page. Its leaf headers, footers,
        //     menus, split rails, resizing and withdrawal are live; scene and UV GPU targets remain skeletal.
        const float EditorX = (Display.Width - 32.0f < 1152.0f)
                                ? Display.Width - 32.0f : 1152.0f;
        const float EditorY = (Display.Height - 48.0f > 600.0f)
                                 ? Display.Height - 48.0f : 600.0f;
        const PlaneExtent EditorExtent = Spanning((Display.Width - EditorX) * 0.5f,
                                                  Cursor,
                                                  EditorX,
                                                  EditorY);
        Discard(EditorPanels.Record(EditorExtent, EditorPartition, EditorConfiguration));
        Cursor = EditorExtent.MaximumY + Measure.CardGapY;

        // ⑮ The complete notch Control Centre remains the final full display-sized page.
        const PlaneExtent ControlCentreExtent = Spanning(0.0f, Cursor, Display.Width, Display.Height);
        Discard(ControlCentre.Record(ControlCentreExtent, ControlCentreValues));

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

                // 🔴 The next tick's resolve reads this, and the two panels that keep their own copy of the
                //    inks are reapplied from the appearance this tick already holds.
                Selected = Chosen;
                Appearance = ResolveTinted(Display.DisplayScale, ArtistScale, Display.Width, Selected);
                std::strncpy(Appearance.Fonts.Family, Selected.FontFamily, sizeof(Appearance.Fonts.Family) - 1u);
                ApplyUserScale(Appearance,
                               static_cast<float>(ControlCentreValues.TypographySize[3]) / 14.0f,
                               static_cast<float>(ControlCentreValues.Radius) / 24.0f);
ApplyFontWeights(Appearance, ControlCentreValues.TypographyWeight);
                Discard(Interface.ApplyWorkspaceStyle(Appearance.WorkspaceMeasure, Appearance.Workspace));
    Surface.ApplyTypographyScale(Appearance.TextScale);
    Surface.ApplyCornerScale(Appearance.CornerScale);
    Discard(Fonts.Discover(FontArchivesPath));
    Discard(Fonts.PreparePreviews(1.0f));
    ControlCentre.SetFontFamilies(Fonts);
    Fonts.RequestLoad(FontArchivesPath, Appearance.Fonts, 1.0f);
                ContentBrowser.Reapply(Appearance);
                LayerStack.Reapply(Appearance);
            }
        }
        Cursor = ControlCentreExtent.MaximumY + Measure.CardGapY;

        // ⑯ The ported reference shell, the final full display-sized page. Its filter takes whatever was
        //    typed this tick before it is recorded, so the run the field strokes is the run the artist has
        //    just entered rather than the previous tick's.
        const PlaneExtent ShellExtent = Spanning(0.0f, Cursor, Display.Width, Display.Height);

        static_cast<void>(Interface.AcceptTyped(ShellApplied.EntityRetention,
                                               ShellContext::RetentionCeiling));

        if (Interface.KeyPressed(KeySubject::Retract))
        {
            std::uint32_t Occupied = 0u;

            while (Occupied + 1u < ShellContext::RetentionCeiling &&
                   ShellApplied.EntityRetention[Occupied] != '\0')
            {
                ++Occupied;
            }

            if (Occupied > 0u)
                ShellApplied.EntityRetention[Occupied - 1u] = '\0';
        }

        Discard(ReferenceShell.Record(ShellExtent, ShellApplied, LevelEntities, 14u, StackLayers, 4u,
                                        LevelRevisions, 9u));
        Cursor = ShellExtent.MaximumY + Measure.CardGapY;

        // 📝 The browser's seek run takes what was typed only while its field holds the keyboard, on the
        //    same terms as the shell's filter above — the two guards are exclusive, so no key reaches both.
        if (ContentBrowserApplied.SeekHolding)
        {
            static_cast<void>(Interface.AcceptTyped(ContentBrowserApplied.Seek,
                                                   ContentBrowserConfiguration::SeekCeiling));

            if (Interface.KeyPressed(KeySubject::Retract))
                static_cast<void>(ContentBrowser.RetractTyped(ContentBrowserApplied));

            if (Interface.KeyPressed(KeySubject::Withdraw))
            {
                ContentBrowserApplied.Seek[0]     = '\0';
                ContentBrowserApplied.SeekHolding = false;
            }
        }
        else if (Interface.KeyPressed(KeySubject::Seek))
        {
            // 📐 The `/` chip in the field is not decoration — the reference binds the key to the focus.
            ContentBrowserApplied.SeekHolding = true;
        }

        // ⑰ The ported `LayerstackV1` page: the stack on the leading edge, and beside it the inspector's
        //     second slide — a property panel over the revisions. Which property panel stands is not a
        //     choice the host makes; it follows the taken half, exactly as the reference switches it.
        {
            constexpr float LayerPaneX  = 392.0f;   // [px] - --w, the reference's own panel extent
            constexpr float LayerPageGap    =  16.0f;   // [px]

            // 📐 The property panel is sized to the taller of the two it may present. The mask panel runs
            //    to its mesh-map and channel sections, so a ratio of the stack's extent clips it; the
            //    lengths are stated instead, and the page is as tall as the sum.
            constexpr float PropertyHeight  = 600.0f;   // [px] - the mask panel, its tallest arrangement
            constexpr float RevisionHeight  = 420.0f;   // [px] - the five applied revisions and their head
            constexpr float SlideHeight     = PropertyHeight + LayerPageGap + RevisionHeight;
            constexpr float LayerPageHeight = (SlideHeight > 760.0f) ? SlideHeight : 760.0f;

            const PlaneExtent StackExtent = Spanning(0.0f, Cursor, LayerPaneX, LayerPageHeight);
            LayerStack.RecordStack(StackExtent, LayerArranged, LayerStackApplied, LayerRevisions);

            const float SlideX = LayerPaneX;
            const float SlideTop = StackExtent.MaximumX + LayerPageGap;

            const PlaneExtent PropertyExtent = Spanning(SlideTop, Cursor, SlideX, PropertyHeight);

            // 🔴 A mask taken presents the mask panel and a layer taken the channel panel. The reference
            //    switches on the taken half and never on the content, so a folder taken still reaches here.
            if (LayerArranged.TakenHalf == LayerTaken::Mask)
                LayerStack.RecordMaskProperties(PropertyExtent, LayerArranged, LayerStackApplied,
                                                LayerRevisions);
            else
                LayerStack.RecordChannelProperties(PropertyExtent, LayerArranged, LayerStackApplied,
                                                   LayerRevisions);

            const PlaneExtent RevisionExtent = Spanning(SlideTop,
                                                        PropertyExtent.MaximumY + LayerPageGap,
                                                        SlideX, RevisionHeight);
            LayerStack.RecordRevisions(RevisionExtent, LayerArranged, LayerStackApplied, LayerRevisions);

            Cursor = Cursor + LayerPageHeight + Measure.CardGapY;
        }

        // ⑱ The ported `AsstbrowsrBasic` page: the sources aside, the record lattice between them, and the
        //     inspector on the trailing edge. `h-screen` in the reference, so it is recorded at the whole
        //     display extent exactly as the shell and the Control Centre pages above it are.
        // 🔴 The Three.js preview is deliberately not built. The inspector's preview region states what it
        //     would present instead of standing empty, so the absence reads as withheld and not as failed.
        {
            constexpr float BrowserPageHeight = 720.0f;   // [px] - what the whole browser wants across

            const float BrowserY = (Display.Height > BrowserPageHeight)
                                      ? Display.Height : BrowserPageHeight;

            const PlaneExtent BrowserExtent = Spanning(0.0f, Cursor, Display.Width, BrowserY);

            ContentBrowser.RecordBrowser(BrowserExtent, ContentApplied, ContentBrowserApplied);

            Cursor = BrowserExtent.MaximumY + Measure.CardGapY;
        }

        // 🔴 The deferred sweep — every menu and every tooltip card, above every row recorded above.
        Panel.RecordDeferred();
        Facets.RecordDeferred();
        LayerStack.RecordDeferred(LayerArranged, LayerStackApplied, LayerRevisions);
        ContentBrowser.RecordDeferred(ContentBrowserApplied);

        // 📝 What the page sequence actually occupied, for the next tick's scroll to be held against. The
        //    trailing `py-32` is added so the Control Centre page can be carried clear of the lower edge.
        ColumnMeasured = Cursor + ScrollY + Measure.PagePadY;

#ifdef SLATE_DEBUG
        // 🔍 The overlay retains what the sheet declares each control should span across, and strokes the
        //    disagreement. Recorded last so it sits above even the deferred sweep.
        Overlay.Retain("selection", SelectionRow, Measure.FieldHeight);
        Overlay.Retain("degree",    DegreeRow,    Measure.FieldHeight);
        Overlay.Retain("percent",   PercentRow,   Measure.FieldHeight);
        Overlay.Retain("pixel",     PixelRow,     Measure.FieldHeight);
        Overlay.Retain("light",     LightTrigger, Measure.TriggerExtent);
        Overlay.Retain("dark",      DarkTrigger,  Measure.TriggerExtent);
        Overlay.Retain("toggles",   ToggleWell,   ToggleWellHeight);
        Overlay.Retain("subsets",   SubsetWell,   SubsetWellHeight);

        if (OverlayShown)
            Overlay.Record(Surface, Appearance, ArtistScale, Display.Width, Overlay.Disagreeing());
#endif

            // ⑫ Seal the tick and record it into the recording Await opened.
            // 🔴 The surface is retired at the seal. This host records through it directly rather than
            //    through ViewportSequence, so it performs the retirement ViewportSequence would.
            Surface.Retire();

            if (Interface.Seal().Resolved)
            {
                // 🔴 Read. A rejected Record presents the cleared ground with nothing on it, which is
                //    indistinguishable from a panel that drew nothing, so the refusal is named here.
                if (!Interface.Record(Pass.Recording))
                {
                    std::printf("%s \u2014 the interface content was not recorded\n", HostName);
                }
            }
            else
            {
                Discard(Interface.Abandon());
            }
        }

        // ⑬ Close the scope, submit, present, advance. A rejected present re-establishes the chain rather
        //    than ending the loop, and a tick whose content rejected still presents the cleared ground.
        if (!Lifetime.Complete().Resolved)
            break;
    }

    // ─────────────────────────────────────────────────────────────────────────────────────────────────────
    //                                                      RECLAMATION
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────

    // 📝 The interface content is retired before the lifetimes it was constructed over. HostLifecycle idles
    //    the device inside Reclaim, so nothing here needs to.
    // 🔴 Read before Reclaim. The register is Device lifetime, and a reclaimed device has emptied it.
    const std::uint32_t Serious = Lifetime.StateDiagnostics();

    ReferenceShell.Reset();
    ControlCentre.Reset();
    EditorPanels.Reset();
    EditorPartition.Reset();
    Facets.Reset();
    ReferenceControls.Reset();
    Panel.Reset();
    Ledger.Reset();
    Surface.Reset();
    Interface.Reclaim();
    Lifetime.Reclaim();

    std::printf("%s \u2014 exited cleanly\n", HostName);

    // 🔴 Returned rather than only stated. This is the host a validation run is driven through, so a
    //    serious arrival has to fail the run and not merely appear in it.
    return (Serious == 0u) ? 0 : 1;
}
