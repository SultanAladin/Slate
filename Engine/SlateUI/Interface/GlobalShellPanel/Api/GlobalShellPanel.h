//============================================================================================================================================
//                                                          GLOBALSHELLPANEL.H
//============================================================================================================================================
// 🧩 The global-interface reference shell — its top bar, its Options rail, its weave viewport and its two-slide inspector.

#pragma once

#include "Contract/DeliveryContract.h"
#include "Contract/PrecisionContract.h"
#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"
#include "SlateUI/Interface/InteractionIndex/Api/InteractionIndex.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"
#include "SlateUI/Interface/MotionIntegrator/Api/MotionIntegrator.h"
#include "SlateUI/Interface/SymbolSpecification/Api/SymbolSpecification.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE REFERENCE INKS
//------------------------------------------------------------------------------------------------------------------------

// 📐 The custom properties `app/globals.css` declares, transcribed as packed literals. Each is named for the
//    responsibility the reference gives it, with the CSS spelling stated beside it so the two can be compared
//    without opening the sheet. Nothing here is derived — every one is a literal from the source.
inline constexpr std::uint32_t ShellDesk         = 0x0A0A0Bu;   // [-] - --desk
inline constexpr std::uint32_t ShellMenu         = 0x17171Au;   // [-] - --menu
inline constexpr std::uint32_t ShellMenuLower    = 0x101012u;   // [-] - --menu-2
inline constexpr std::uint32_t ShellRailTaken    = 0x232327u;   // [-] - --rail-sel, and --row-sel
inline constexpr std::uint32_t ShellTile         = 0x1D1D21u;   // [-] - --tile
inline constexpr std::uint32_t ShellTileRoused   = 0x26262Bu;   // [-] - --tile-hi
inline constexpr std::uint32_t ShellAccent       = 0x4A90E2u;   // [-] - --accent
inline constexpr std::uint32_t ShellInkPrimary   = 0xECECF0u;   // [-] - --ink
inline constexpr std::uint32_t ShellInkMuted     = 0x7B7B82u;   // [-] - --muted
inline constexpr std::uint32_t ShellInkFaint     = 0x55555Du;   // [-] - --faint
inline constexpr std::uint32_t ShellValueUnit    = 0x33333Au;   // [-] - --value-unit
inline constexpr std::uint32_t ShellHairline     = 0xFFFFFFu;   // [-] - --hair and --hair-strong, by coverage
inline constexpr std::uint32_t ShellEntityAccent = 0x3B82F6u;   // [-] - the outliner's own rail, bg-[#3b82f6]
inline constexpr std::uint32_t ShellEntityTaken  = 0x1E40AFu;   // [-] - bg-[#1e40af33]

/// 🧩 Every ink the shell records with, seated once beside the rest of the appearance.
/// note  🔴 A record and not forty call-site literals. The reference states each colour once as a custom
///        property and every rule reads it; a port spelling `Covering(0x17171Au)` at each of the sites it
///        appears could not be compared against the sheet, and one of them would drift unnoticed.
/// tag   contract, nonallocating, nonthrowing
struct ShellInk
{
    InkOrdinate  Desk         = Covering(ShellDesk);            // [-] - the viewport ground
    InkOrdinate  Menu         = Covering(ShellMenu);            // [-] - the outliner and the summoned card
    InkOrdinate  MenuLower    = Covering(ShellMenuLower);       // [-] - the top bar, the rail, the inspector
    InkOrdinate  Tile         = Covering(ShellTile);            // [-] - a quiet mode button
    InkOrdinate  TileRoused   = Covering(ShellTileRoused);      // [-] - hover:bg-[var(--tile-hi)]
    InkOrdinate  RowTaken     = Covering(ShellRailTaken);       // [-] - bg-[var(--row-sel)]
    InkOrdinate  RowRoused    = Partial(ShellHairline, 0.045);  // [-] - --row-hover
    InkOrdinate  Hairline     = Partial(ShellHairline, 0.06);   // [-] - --hair
    InkOrdinate  HairlineFirm = Partial(ShellHairline, 0.10);   // [-] - --hair-strong
    InkOrdinate  Accent       = Covering(ShellAccent);          // [-] - --accent
    InkOrdinate  AccentSoft   = Partial(ShellAccent, 0.13);     // [-] - --accent-soft
    InkOrdinate  EntityAccent = Covering(ShellEntityAccent);    // [-] - the outliner's selection rail
    InkOrdinate  EntityTaken  = Partial(ShellEntityTaken, 0.20);// [-] - bg-[#1e40af33]
    InkOrdinate  Primary      = Covering(ShellInkPrimary);      // [-] - --ink
    InkOrdinate  Muted        = Covering(ShellInkMuted);        // [-] - --muted
    InkOrdinate  Faint        = Covering(ShellInkFaint);        // [-] - --faint
    InkOrdinate  Unit         = Covering(ShellValueUnit);       // [-] - --value-unit, the status separators
    InkOrdinate  Veil         = Partial(0x000000u, 0.30);       // [-] - the summon veil, bg-black/30
    InkOrdinate  WeaveFine    = Partial(ShellHairline, 0.028);  // [-] - the 28 px weave
    InkOrdinate  WeaveCoarse  = Partial(ShellHairline, 0.055);  // [-] - the 140 px weave
    InkOrdinate  Vignette     = Partial(0x000000u, 0.55);       // [-] - the viewport's radial fall-off
    InkOrdinate  Absent       = Partial(0x000000u, 0.00);       // [-] - a quiet ground records nothing
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE REFERENCE EXTENTS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every measured extent the reference states, before the display and artist factors are applied.
/// note  📐 Each figure is the literal from `app/page.tsx` or its component, named rather than repeated. The
///        reference writes them as Tailwind arbitrary values — `h-[36px]`, `w-[220px]` — so the transcription
///        can be checked against the sheet line by line.
/// tag   contract, nonallocating, nonthrowing
struct ShellMetric
{
    float  TopBarAcross    =  36.0f;   // [px] - h-[36px]
    float  TopBarPadAlong  =  14.0f;   // [px] - px-[14px]
    float  OptionsAlong    = 220.0f;   // [px] - w-[220px]
    float  HeaderAcross    =  46.0f;   // [px] - h-[46px], every pane header
    float  HeaderPadAlong  =  10.0f;   // [px] - px-[10px]
    float  RowAcross       =  32.0f;   // [px] - h-[32px], an outline row
    float  RowStepAlong    =  15.0f;   // [px] - depth * 15
    float  RowLeadAlong    =   8.0f;   // [px] - paddingLeft 8 + depth * 15
    float  ChevronExtent   =  15.0f;   // [px] - the w-[15px] disclosure cell
    float  GlyphExtent     =  18.0f;   // [px] - the w-[18px] classification cell
    float  RailAlong       =   3.0f;   // [px] - the w-[3px] selection rail
    float  RailAcross      =  15.0f;   // [px] - h-[15px]
    float  RailOffsetAlong =   7.0f;   // [px] - left-[-7px]
    float  SearchAcross    =  30.0f;   // [px] - h-[30px], the filter field
    float  FooterAcross    =  26.0f;   // [px] - h-[26px]
    float  InspectorAlong  = 700.0f;   // [px] - w-[700px], docked and summoned alike
    float  SummonedAcross  = 400.0f;   // [px] - h-[400px], the summoned card
    float  OutlinerAlong   = 350.0f;   // [px] - grid-cols-[350px_minmax(0,1fr)]
    float  MedallionExtent =  24.0f;   // [px] - the w-6 h-6 header medallion
    float  CardRadius      =  12.0f;   // [px] - --r-tile
    float  MenuRadius      =  18.0f;   // [px] - --r-menu
    float  FieldRadius     =   6.0f;   // [px] - rounded-md
    float  StatusAcross    =  28.0f;   // [px] - the viewport's h-[28px] hint strip
    float  WeaveFineStep   =  28.0f;   // [px] - backgroundSize 28px
    float  WeaveCoarseStep = 140.0f;   // [px] - backgroundSize 140px
    float  ComponentAcross =  31.0f;   // [px] - h-[31px], a component card header
    float  PanePad         =   7.0f;   // [px] - p-[7px]
    float  RunPrimary      =  12.5f;   // [px] - text-[12.5px]
    float  RunSecondary    =  11.5f;   // [px] - text-[11.5px]
    float  RunSmall        =  10.5f;   // [px] - text-[10.5px]
    float  RunFine         =  10.0f;   // [px] - text-[10px]
};

// 📐 The slide travel, which is a duration and not a length and so is never scaled.
inline constexpr double CarouselTravelOver = 300.0;   // [ms] - duration-300 on the inspector strip

/// 🧩 Applies the resolved display and artist factors to every extent the reference declares.
/// in    Factor  [-]  the same product `AppearanceSpecification` applies to its own interface lengths
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
ShellMetric ScaleShellLengths(float Factor);

//------------------------------------------------------------------------------------------------------------------------
//                                                   WHAT THE SHELL PRESENTS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Which of the three workspaces the Options rail has selected.
/// note  The reference's `workspaceMode` union, verbatim and in its own ordinal order.
/// tag   contract
enum class WorkspaceMode : std::uint32_t
{
    Drafting     = 0u,   // [-] - the scene directory and its metadata pane
    TexturePaint = 1u,   // [-] - the layer stack and its channel inspector
    WorldEditor  = 2u,   // [-] - the outliner and its component inspector
    ModeCount    = 3u    // [-] - the closed count, never a mode
};

/// 🧩 What one entity in the World Outliner is, which decides its glyph and its hue.
/// note  🔴 The reference spells this `GameNodeType`, which carries two banned spellings. The discriminating
///        mechanism is the entity's discipline, so it is named for that. Ordinals follow the reference's own
///        declaration order so the two records can be read side by side.
/// tag   contract
enum class EntitySubject : std::uint32_t
{
    Level        = 0u,   // [-] - the level root
    Grouping     = 1u,   // [-] - a folder in the outliner
    Actor        = 2u,   // [-] - a placed prefab
    Camera       = 3u,   // [-] - a view
    Illuminant   = 4u,   // [-] - a light
    Audio        = 5u,   // [-] - an emitter
    Particle     = 6u,   // [-] - a visual effect
    Trigger      = 7u,   // [-] - a volume
    Script       = 8u,   // [-] - a behaviour
    SubjectCount = 9u    // [-] - the closed count, never a subject
};

/// 🧩 The glyph one entity subject is drawn with, from the reference's own `ICONS` record.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
SymbolSubject EntityGlyph(EntitySubject Subject);

/// 🧩 The hue one entity subject carries, from the reference's own `COLORS` record.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
InkOrdinate EntityHue(EntitySubject Subject);

/// 🧩 The run naming one entity subject, as the reference's `capitalize` rule presents it.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* EntityText(EntitySubject Subject);

/// 🧩 One row of the World Outliner, linearised and carrying its own depth.
/// note  📝 The reference holds a nested graph and renders it recursively. A linear sequence carrying depth
///       records identically and needs no recursion inside a tick, which is what keeps the panel allocation
///       free; the enclosing ordinal is retained so disclosure and presence still propagate inward.
/// tag   contract, nonallocating, nonthrowing
struct EntityRow
{
    const char*    Naming           = "";                     // [-] - borrowed; outlives the tick
    EntitySubject  Subject          = EntitySubject::Actor;   // [-] - what it is
    std::uint32_t  Depth            = 0u;                     // [-] - indentation steps from the level
    std::uint32_t  Enclosing        = 0xFFFFFFFFu;            // [-] - the row holding it; absent for the level
    std::uint32_t  EnclosedCount    = 0u;                     // [-] - zero presents no disclosure mark
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT THE HOST OWNS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every datum the shell presents, owned by the host and written through by the panel.
/// note  🔴 `14` §1: the panel presents what it is handed and retains none of it. Every condition the artist
///        can alter lives here, so the host — and only the host — is the home of the shell's content.
/// tag   contract, nonallocating, nonthrowing
struct ShellOrdinates
{
    static constexpr std::uint32_t EntityCeiling    = 16u;   // [-] - the reference declares fourteen
    static constexpr std::uint32_t RetentionCeiling = 48u;   // [-] - the retention run, terminator included

    bool           InspectorDocked = false;                        // [-] - isDocked; the reference begins undocked
    bool           MenuOpened      = false;                        // [-] - menuOpen
    bool           InspectorShown  = false;                        // [-] - showInspector
    WorkspaceMode  Mode            = WorkspaceMode::WorldEditor;   // [-] - the reference begins on 'game'
    std::uint32_t  EntityTaken     = 2u;                           // [-] - activeGameId, seated at 'g_03'
    char           EntityRetention[RetentionCeiling] = {};         // [-] - filterText

    // 📝 The disclosure conditions the reference's own graph declares: the level, Lighting, Environment and
    //    Systems arrive expanded and nothing else does.
    bool  EntityExpanded[EntityCeiling] = { true, true, false, false, false, false,
                                            true, false, false, false, true, false, false, false };
    bool  EntityPresent[EntityCeiling]  = { true, true, true, true, true, true,
                                            true, true, true, true, true, true, true, true };
};

//------------------------------------------------------------------------------------------------------------------------
//                                                         THE PANEL
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Records the reference shell exactly as `References/remix-remix-global-ui` presents it.
/// note  🔴 Every primitive is recorded through `RecordingSurface` into ONE command list, in one order. The
///        panel opens no vendor window and records no vendor widget — a vendor widget carries its own draw
///        list, which composites above every shell layer, and that is precisely how a ground painted over
///        the whole display left the panels beneath it legible and still pressable. The veil, the summoned
///        card and everything beneath them therefore stack in the order they are written and in no other.
/// note  ⚠️ The keymap is the host's, because the host owns the seam the keys arrive through.
///        `AdvanceSummoning` is offered so the reference's Tab and Escape rules are stated once, in the
///        order the reference states them, rather than re-derived by each host that presents the shell.
/// tag   owning
class GlobalShellPanel
{
public:

    GlobalShellPanel()                                   = default;
    GlobalShellPanel(const GlobalShellPanel&)            = delete;
    GlobalShellPanel& operator=(const GlobalShellPanel&) = delete;
    ~GlobalShellPanel()                                  = default;

    /// 🧩 Borrows the recording facilities and enrols every identity and interpolant the shell needs.
    /// out   Deliver  [-]  refuses with ContentUnsupported when a construction already stands, and with
    ///                     ExtentExhausted when the ledger or the integrator declines an enrolment
    /// cost  🚩
    /// tag   api, nonthrowing
    Deliver<bool> Construct(InteractionIndex&              Interaction,
                            MotionIntegrator&              Integrator,
                            RecordingSurface&              Surface,
                            const AppearanceSpecification& Resolved);

    /// 🧩 Samples the contact for this tick, after the tick owner has advanced the shared ledger once.
    /// note  🔴 This does not advance the ledger; several panels share it and the tick owner advances it once.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Advance(const PointerCondition& Sampled, double Elapsed);

    /// 🧩 Re-seats every scaled extent after the appearance was resolved against a new display extent.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Reseat(const AppearanceSpecification& Resolved);

    /// 🧩 Applies the reference's Tab and Escape rules to the summoning conditions the host owns.
    /// in    Summoned   [-]  Tab arrived this tick and no text field held it
    /// in    Withdrawn  [-]  Escape arrived this tick
    /// out   Altered    [-]  true when any presentation condition moved, so the host may re-record
    /// note  📐 The reference's own order, from `app/page.tsx`: docked, Tab flips the inspector; undocked,
    ///       the first Tab opens the menu and every later Tab flips the inspector; Escape closes the
    ///       inspector when it stands and otherwise the menu, and only while undocked.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    bool AdvanceSummoning(ShellOrdinates& Seated, bool Summoned, bool Withdrawn);

    /// 🧩 Records the whole shell — bar, rail, viewport, docked inspector, veil and summoned card.
    /// in    Extent    [px] the display's full drawable extent
    /// in    Rows      [-]  the outliner's linearised entities, borrowed for the tick
    /// in    RowCount  [-]  how many of them; clamped to the seated ceiling
    /// out   Deliver   [-]  refuses with CapabilityAbsent when no construction stands or no tick is adopted
    /// cost  🔴
    /// tag   api, nonthrowing
    Deliver<bool> Record(const PlaneExtent&  Extent,
                         ShellOrdinates&     Seated,
                         const EntityRow*    Rows,
                         std::uint32_t       RowCount);

    /// 🧩 Whether the shell's own chrome stands over a display ordinate.
    /// note  Consulted by a host before it treats a contact as a viewport stroke.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    bool Occluding(float Along, float Across) const;

    /// 🧩 Returns the panel to its unconstructed condition.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Reset();

private:

    static constexpr std::uint32_t RowCeiling = ShellOrdinates::EntityCeiling;

    void RecordTopBar(const PlaneExtent& Extent, const ShellOrdinates& Seated);
    void RecordOptionsRail(const PlaneExtent& Extent, ShellOrdinates& Seated);
    void RecordViewport(const PlaneExtent& Extent, const ShellOrdinates& Seated);
    void RecordInspector(const PlaneExtent& Extent, ShellOrdinates& Seated,
                         const EntityRow* Rows, std::uint32_t RowCount);
    void RecordOutliner(const PlaneExtent& Extent, ShellOrdinates& Seated,
                        const EntityRow* Rows, std::uint32_t RowCount);
    void RecordComponents(const PlaneExtent& Extent, const ShellOrdinates& Seated,
                          const EntityRow* Rows, std::uint32_t RowCount);
    void RecordRetentionField(const PlaneExtent& Extent, ShellOrdinates& Seated);
    void RecordPaneHeader(const PlaneExtent& Extent, SymbolSubject Glyph, InkOrdinate GlyphInk,
                          InkOrdinate MedallionInk, const char* Title, const char* Secondary);

    /// 🧩 Whether a row is presented, given the filter and every enclosure's disclosure.
    bool RowPresented(const ShellOrdinates& Seated, const EntityRow* Rows,
                      std::uint32_t RowCount, std::uint32_t Ordinal) const;

    InteractionIndex*              Ledger     = nullptr;   // [-] - borrowed; never owned
    MotionIntegrator*              Motion     = nullptr;   // [-] - borrowed; never owned
    RecordingSurface*              Surface    = nullptr;   // [-] - borrowed; never owned
    const AppearanceSpecification* Appearance = nullptr;   // [-] - borrowed; never owned
    ControlPanel                   Controls   = {};        // [-] - the shared inspector controls

    ShellInk     Tinted  = {};   // [-] - seated once at construction
    ShellMetric  Scaled  = {};   // [-] - re-seated whenever the appearance is resolved again

    PointerCondition  Sampled       = {};      // [-] - this tick's contact
    std::uint32_t     CarouselSlide = 0u;      // [-] - the enrolled traverse carrying the two slides
    PlaneExtent       InspectorAt   = {};      // [px] - where the inspector was last recorded
    bool              InspectorStood = false;  // [-] - whether it was recorded at all last tick

    ControlIdentity  DockSwitch    = {};   // [-] - the Dock Inspector switch
    ControlIdentity  ModeButtons[3] = {};  // [-] - the three workspace buttons
    ControlIdentity  RetentionField   = {};   // [-] - the outliner's retention run
    ControlIdentity  VeilContact   = {};   // [-] - the veil, which dismisses the menu
    ControlIdentity  RowContacts[RowCeiling]    = {};   // [-] - one per outline row
    ControlIdentity  RowDisclosures[RowCeiling] = {};   // [-] - one per disclosure cell
    ControlIdentity  RowPresences[RowCeiling]   = {};   // [-] - one per presence action
};

SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded);

}   // namespace Slate
