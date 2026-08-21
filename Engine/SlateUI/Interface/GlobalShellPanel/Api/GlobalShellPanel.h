//============================================================================================================================================
//                                                          GLOBALSHELLPANEL.H
//============================================================================================================================================
// 🧩 The global-interface reference shell — its top bar, its Options rail, its weave viewport and its two-slide inspector.
//
// 🔴 THIS IS THE VALIDATION PROTOTYPE. It presents the whole reference sheet —
//    the options rail, the texture-paint layer stack, the CAD drafting modes and
//    the fullscreen two-slide inspector — and it is recorded ONLY by
//    InterfaceValidationHost, whose viewport stays BLACK. The editor host must
//    never record this panel, and no sky is ever drawn in its viewport: the
//    editor's sky lives in the viewport LEAF via SceneDirectoryPanel
//    (see AgenticInstuctions/EDITOR-AND-VALIDATION.md).

#pragma once

#include "Contract/DeliveryContract.h"
#include "Contract/PrecisionContract.h"
#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "SlateUI/Interface/ComponentSpecification/Api/ComponentSpecification.h"
#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"
#include "SlateUI/Interface/InteractionIndex/Api/InteractionIndex.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"
#include "SlateUI/Interface/SceneDirectoryPanel/Api/SceneDirectoryContract.h"
#include "SlateUI/Interface/MotionIntegrator/Api/MotionIntegrator.h"
#include "SlateUI/Interface/SymbolSpecification/Api/SymbolSpecification.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE REFERENCE INKS
//------------------------------------------------------------------------------------------------------------------------

// 📝 `ShellColour` now lives in `ThemeProfile.h`, beside every other colour the interface draws
//    with. It moved so the appearance file can reach it: a token run declared in a panel header is one the
//    Control Centre cannot theme. The spellings are unchanged.

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE REFERENCE EXTENTS
//------------------------------------------------------------------------------------------------------------------------

inline constexpr double CarouselTravelOver = 300.0;   // [ms] - duration-300 on the inspector strip

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

/// 🧩 What one paint layer is, which decides its accent dot and its summary run.
/// note  📐 The reference's `KINDS` record from `components/TexturePaint.tsx`, in its own ordinal order. It
///        spells the discriminator `kind`, which is a banned spelling; `ChannelPropertyPanel` in the same
///        file already calls the same property `Classification`, so the reference's own second name is used.
/// tag   contract
enum class LayerClassification : std::uint32_t
{
    Paint               = 0u,   // [-] - accepts brush strokes
    Material            = 1u,   // [-] - fills entire UV
    Generator           = 2u,   // [-] - procedural over the atlas
    ClassificationCount = 3u    // [-] - the closed count, never a classification
};

/// 🧩 Which half of a layer row the artist has taken — the reference's `activeTarget`.
/// tag   contract
enum class LayerTarget : std::uint32_t
{
    Layer = 0u,   // [-] - the left half; its blend, opacity and channels
    Mask  = 1u    // [-] - the right half; its strength, inversion and source
};

/// 🧩 The tint one classification carries, from the reference's own `KINDS` record.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
ThemeToken ClassificationTint(LayerClassification Classified);

/// 🧩 The run naming one classification, as the reference presents it.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* ClassificationText(LayerClassification Classified);

/// 🧩 One row of the Layer Stack, exactly as the reference's `mockLayers` declares it.
/// note  📝 Held flat and borrowed for the tick, on the same terms as `EntityRow`. The reference keeps its
///        channel names in a JavaScript array; a bounded run of borrowed pointers records identically and
///        keeps the panel allocation free.
/// tag   contract, nonallocating, nonthrowing
struct LayerRow
{
    static constexpr std::uint32_t ChannelCeiling = 6u;   // [-] - the reference declares at most four

    const char*          Naming        = "";                          // [-] - borrowed; outlives the tick
    LayerClassification  Classified    = LayerClassification::Paint;   // [-] - kind
    const char*          Blend         = "Normal";                     // [-] - borrowed; the blend mode's name
    std::uint32_t        Opacity       = 100u;                         // [%] - 0…100
    std::uint32_t        PaintHue      = 0xF97316u;                    // [-] - the swatch, `paint`
    std::uint32_t        TagHue        = 0xEAB308u;                    // [-] - the spine and its badge, `tag`
    bool                 MaskDeclared  = false;                        // [-] - mask.enabled
    std::uint32_t        MaskStrength  = 100u;                         // [%] - mask.strength
    bool                 MaskInverted  = false;                        // [-] - mask.invert
    const char*          Channels[ChannelCeiling] = {};                // [-] - borrowed; the channel pills
    std::uint32_t        ChannelCount  = 0u;                           // [-] - how many are declared
};

/// 🧩 Every measured property one outline row carries, which the metadata pane states and the property
///    cards write back through.
/// note  📐 `RecordProfile` from `lib/store.tsx`, transcribed field for field. The reference declares each
///        member optionally and tests for its presence before stating a row; an optional member is a
///        rejected spelling here, so each conditional member is paired with the flag that declares it.
/// note  🔴 Lives beside the artist's other conditions in `ShellContext` and NOT in `EntityRow`, because
///        the inspector writes to it. `EntityRow` is the borrowed description of what a row IS; this is the
///        mutable record of what the artist has made it.
/// tag   contract, nonallocating, nonthrowing
struct EntityProfile
{
    double         Position[3]        = { 0.0, 0.0, 0.0 };   // [mm]  - profile.Position
    double         Rotation[3]        = { 0.0, 0.0, 0.0 };   // [deg] - profile.Rotation
    double         Magnification[3]   = { 1.0, 1.0, 1.0 };   // [-]   - profile.Scale
    std::uint32_t  Albedo[4]          = { 200u, 200u, 205u, 255u };   // [-] - profile.Albedo, 0…255
    double         Roughness          = 0.50;   // [-]  - profile.Roughness, 0 … 1
    double         Metalness          = 0.00;   // [-]  - profile.Metalness, 0 … 1
    std::uint32_t  ShadingTaken       = 0u;     // [-]  - profile.ShadingMode, into the three shading runs
    double         Radius             = 0.00;   // [mm] - profile.Radius
    double         Height             = 0.00;   // [mm] - profile.Height
    double         ExtrudeDepth       = 0.00;   // [mm] - profile.ExtrudeDepth
    std::uint32_t  CurveTally         = 0u;     // [-]  - profile.CurveTally
    bool           FullyConstrained   = false;  // [-]  - profile.FullyConstrained
    bool           RadiusDeclared     = false;  // [-]  - whether the Radius row is stated at all
    bool           HeightDeclared     = false;  // [-]  - whether the Height row is stated at all
    bool           DepthDeclared      = false;  // [-]  - whether the Depth row is stated at all
    bool           CurvesDeclared     = false;  // [-]  - whether the Curves row is stated at all
    bool           ConstraintDeclared = false;  // [-]  - whether the Status row is stated at all
    bool           AlbedoDeclared     = true;   // [-]  - whether the Albedo swatch row is stated at all
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT THE HOST OWNS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every datum the shell presents, owned by the host and written through by the panel.
/// note  🔴 `14` §1: the panel presents what it is handed and retains none of it. Every condition the artist
///        can alter lives here, so the host — and only the host — is the home of the shell's content.
/// tag   contract, nonallocating, nonthrowing
struct ShellContext
{
    static constexpr std::uint32_t EntityCeiling    = 16u;   // [-] - the reference declares fourteen
    static constexpr std::uint32_t RetentionCeiling = 48u;   // [-] - the retention run, terminator included
    static constexpr std::uint32_t LayerCeiling     = 12u;   // [-] - the reference declares four
    static constexpr std::uint32_t CardCeiling      =  4u;   // [-] - the reference states at most four

    bool           InspectorDocked = false;                        // [-] - isDocked; the reference begins undocked
    bool           MenuOpened      = false;                        // [-] - menuOpen
    bool           InspectorShown  = false;                        // [-] - showInspector

    // 📝 Which caption each of the two strips carries. Slide one strips the directory against the taken
    //    row's own card; slide two strips the property cards against the revision spine. Tab walks the
    //    four of them in one cycle, so both live beside the slide they belong to.
    std::uint32_t  OutlineTab      = 0u;                           // [-] - 0 Outliner, 1 Context Menu
    std::uint32_t  InspectorTab    = 0u;                           // [-] - 0 Properties, 1 History

    // 📝 The floating context card the per-row kebab raises. `ContextRaised` is the row it was raised
    //    from and `EntityCeiling` is the closed count standing for "no card"; the two ordinates are where
    //    the kebab was, before the reference's own clamp against the trailing edge is applied.
    std::uint32_t  ContextRaised   = EntityCeiling;                // [-] - contextMenu.id
    float          ContextX    = 0.0f;                         // [px] - contextMenu.x
    float          ContextY   = 0.0f;                         // [px] - contextMenu.y
    WorkspaceMode  Mode            = WorkspaceMode::WorldEditor;   // [-] - the reference begins on 'game'
    std::uint32_t  EntityTaken     = 2u;                           // [-] - activeGameId, applied at 'g_03'
    char           EntityRetention[RetentionCeiling] = {};         // [-] - filterText

    // 📝 The disclosure conditions the reference's own graph declares: the level, Lighting, Environment and
    //    Systems arrive expanded and nothing else does.
    bool  EntityExpanded[EntityCeiling] = { true, true, false, false, false, false,
                                            true, false, false, false, true, false, false, false };
    bool  EntityPresent[EntityCeiling]  = { true, true, true, true, true, true,
                                            true, true, true, true, true, true, true, true };

    // 📝 What the metadata pane states and the property cards write back through. Applied at the reference's
    //    own defaults and amended by the artist; the panel reads and writes it and retains none of it.
    EntityProfile  EntityProfiles[EntityCeiling] = {};

    // 📐 `collapsedCards` and `collapsedHistory`. The reference applies both empty, so every card and every
    //    revision group arrives disclosed and the artist folds what they do not want.
    bool  CardFolded[CardCeiling]        = {};
    bool  RevisionFolded[EntityCeiling]  = {};

    // 📝 The Layer Stack's own conditions. `activeLayerId` is an ordinal here rather than the reference's
    //    minted identifier, on the same terms as `EntityTaken`; the reference applies it on its first layer.
    std::uint32_t  LayerTaken   = 0u;                      // [-] - activeLayerId
    LayerTarget    TargetTaken  = LayerTarget::Layer;      // [-] - activeTarget

    // 🔴 The reference applies `expandedIds` to `[activeLayerId]` — the taken layer alone arrives unfolded.
    bool  LayerUnfolded[LayerCeiling] = { true };          // [-] - expandedIds
    bool  LayerShown[LayerCeiling]    = { true, true, false, true, true, true,
                                          true, true, true, true, true, true };   // [-] - shown

    // 📝 The editor's environment. `EnvironmentPresented` gates every environment branch in the shell, so
    //    the validation host (which never sets it) renders byte-identically; the editor sets it and the
    //    viewport draws the sun and sky from `Environment` while the inspector edits the same values.
    bool                       EnvironmentPresented = false;   // [-] - the editor's sun/sky/atmosphere UI
    EnvironmentConfiguration   Environment          = {};      // [-] - host-owned; the inspector writes it
    RevisionDemand             RevisionDemandSlot   = {};      // [-] - one drag-end history demand

    // 📝 The GPU sky texture the viewport draws, uploaded by the host. Opaque on purpose: the shell
    //    names no vendor, so the identity is an integer the recording surface resolves.
    std::uintptr_t             SkyTextureIdentity  = 0u;      // [-] - zero draws the stylised fallback

    // 📝 The sky's own camera, declared by the host each tick it regenerates. The dome is
    //    direction-indexed, and the viewport crops it to this camera's field of view — so the sun
    //    stays in frame at any viewport aspect.
    SkyViewCamera              ViewportSkyCamera   = {};      // [-] - the dome crop the viewport draws
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

    /// 🧩 Exactly how many control identities `Construct` claims, stated where they are claimed.
    /// note  🔴 A host counted these by hand and the count went stale the first time the shell grew a
    ///        button; the ledger then rejected whichever panel was constructed after it, at bring-up, with
    ///        a message naming the wrong panel. The arithmetic lives beside the registrations it describes so
    ///        the two can only disagree by an edit that touches both.
    static constexpr std::uint32_t RegistrationDemand =
          14u                                       // [-] - the chrome: dock, three modes, retention,
                                                    //       veil, the stack's two, the call and five actions
        + 13u                                       // [-] - both strips, six tints, the clear, Rename,
                                                    //       Delete, the context veil and the Back call
        + ShellContext::CardCeiling               // [-] - one fold per property card
        + ShellContext::EntityCeiling             // [-] - one fold per grouped revision header
        + ShellContext::EntityCeiling * 4u        // [-] - contact, disclosure, presence and kebab per row
        + ShellContext::LayerCeiling  * 6u        // [-] - two halves and four actions per layer row
        + 6u;                                      // [-] - the six environment slider rows

    GlobalShellPanel()                                   = default;
    GlobalShellPanel(const GlobalShellPanel&)            = delete;
    GlobalShellPanel& operator=(const GlobalShellPanel&) = delete;
    ~GlobalShellPanel()                                  = default;

    /// 🧩 Borrows the recording facilities and registers every identity and interpolant the shell needs.
    /// out   Result  [-]  refuses with ContentUnsupported when a construction already stands, and with
    ///                     ExtentExhausted when the ledger or the integrator declines an registration
    /// cost  🚩
    /// tag   api, nonthrowing
    Outcome<bool> Construct(InteractionIndex&              Interaction,
                            MotionIntegrator&              Integrator,
                            RecordingSurface&              Surface,
                            const ThemeProfile& Resolved);

    /// 🧩 Samples the contact for this tick, after the tick owner has advanced the shared ledger once.
    /// note  🔴 This does not advance the ledger; several panels share it and the tick owner advances it once.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Advance(const PointerCondition& Sampled, double Elapsed);

    /// 🧩 Re-applies every scaled extent after the appearance was resolved against a new display extent.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Reapply(const ThemeProfile& Resolved);

    /// 🧩 Applies the reference's Tab and Escape rules to the summoning conditions the host owns.
    /// in    Summoned   [-]  Tab arrived this tick and no text field held it
    /// in    Dismissed  [-]  Escape arrived this tick
    /// in    Reversed   [-]  Shift stood with the Tab, so the cycle is walked the other way
    /// out   Altered    [-]  true when any presentation condition moved, so the host may re-record
    /// note  📐 The reference's own order, from `app/page.tsx`, extended by the two tab strips the summoned
    ///        card carries. Undocked, one whole cycle is: ① open the card, ② Outliner to Context Menu,
    ///        ③ travel to slide two, ④ Properties to History, ⑤ wrap back to slide one's Outliner. Docked,
    ///        the card is always open, so ① and ⑤ are not steps and the cycle is the middle three.
    /// note  📐 Escape is unchanged: the slide closes first and the menu second, and only while undocked.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    bool AdvanceSummoning(ShellContext& Applied, bool Summoned, bool Dismissed, bool Reversed = false);

    /// 🧩 Records the whole shell — bar, rail, viewport, docked inspector, veil and summoned card.
    /// in    Extent      [px] the display's full drawable extent
    /// in    Rows        [-]  the outliner's linearised entities, borrowed for the tick
    /// in    RowCount    [-]  how many of them; clamped to the applied ceiling
    /// in    Layers      [-]  the Layer Stack's rows, borrowed for the tick; absent presents an empty stack
    /// in    LayerCount  [-]  how many of them; clamped to the applied ceiling
    /// out   Result     [-]  refuses with CapabilityAbsent when no construction stands or no tick is adopted
    /// note  📐 Which pair of panes the inspector presents is decided by `Applied.Mode`, exactly as the
    ///       reference's two ternaries in `app/page.tsx` decide it for each of its two slides.
    /// cost  🔴
    /// tag   api, nonthrowing
    Outcome<bool> Record(const PlaneExtent&     Extent,
                         ShellContext&        Applied,
                         const EntityRow*       Rows,
                         std::uint32_t          RowCount,
                         const LayerRow*        Layers        = nullptr,
                         std::uint32_t          LayerCount    = 0u,
                         const EntityRevision*  Revisions     = nullptr,
                         std::uint32_t          RevisionCount = 0u);

    /// 🧩 Whether the shell's own chrome stands over a display coordinate.
    /// note  Consulted by a host before it treats a contact as a viewport stroke.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    bool Occluding(float X, float Y) const;

    /// 🧩 Returns the panel to its unconstructed condition.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Reset();

private:

    static constexpr std::uint32_t RowCeiling   = ShellContext::EntityCeiling;
    static constexpr std::uint32_t LayerCeiling = ShellContext::LayerCeiling;
    static constexpr std::uint32_t CardCeiling  = ShellContext::CardCeiling;

    void RecordTopBar(const PlaneExtent& Extent, const ShellContext& Applied);
    void RecordOptionsRail(const PlaneExtent& Extent, ShellContext& Applied);
    void RecordViewport(const PlaneExtent& Extent, const ShellContext& Applied);
    void RecordInspector(const PlaneExtent& Extent, ShellContext& Applied,
                         const EntityRow* Rows, std::uint32_t RowCount,
                         const LayerRow* Layers, std::uint32_t LayerCount,
                         const EntityRevision* Revisions, std::uint32_t RevisionCount);
    void RecordOutliner(const PlaneExtent& Extent, ShellContext& Applied,
                        const EntityRow* Rows, std::uint32_t RowCount);
    void RecordComponents(const PlaneExtent& Extent, ShellContext& Applied,
                          const EntityRow* Rows, std::uint32_t RowCount,
                          const EntityRevision* Revisions, std::uint32_t RevisionCount);

    /// 🧩 The property cards for whatever the directory has taken — slide two's leading page.
    void RecordPropertyCards(const PlaneExtent& Extent, ShellContext& Applied,
                             const EntityRow* Rows, std::uint32_t RowCount);

    /// 🧩 The revision spine — slide two's trailing page, grouped by the row each revision stands against.
    void RecordRevisionSpine(const PlaneExtent& Extent, ShellContext& Applied,
                             const EntityRow* Rows, std::uint32_t RowCount,
                             const EntityRevision* Revisions, std::uint32_t RevisionCount);

    /// 🧩 Slide one's trailing pane — the reference's `MetadataPane`, hero, stats, call and actions.
    /// note  📐 `components/MetadataPane.tsx`. It presents what the directory has taken and offers the five
    ///        inline actions; the call beneath the stats is the pointer-driven twin of Tab.
    void RecordMetadata(const PlaneExtent& Extent, ShellContext& Applied,
                        const EntityRow* Rows, std::uint32_t RowCount);

    /// 🧩 One hairline stat row — a muted key at the leading edge and a value at the trailing one.
    /// out   Consumed  [px] what the row spanned, so the caller may advance its own cursor
    float RecordStatRow(const PlaneExtent& Extent, const char* Caption, const char* Reading);

    /// 🧩 Slide one's leading column — the tab strip and whichever of its two pages stands taken.
    void RecordOutlineColumn(const PlaneExtent& Extent, ShellContext& Applied,
                               const EntityRow* Rows, std::uint32_t RowCount);

    /// 🧩 The Context Menu page — the taken row's options presented as a whole pane.
    void RecordContextPage(const PlaneExtent& Extent, ShellContext& Applied,
                           const EntityRow* Rows, std::uint32_t RowCount);

    /// 🧩 The same options as a card floating over everything, raised by a row's kebab.
    /// note  📐 Recorded last of all, after the veil and the summoned card, because it stands over both.
    void RecordContextOverlay(const PlaneExtent& Extent, ShellContext& Applied,
                              const EntityRow* Rows, std::uint32_t RowCount);

    /// 🧩 The three dots one row carries at its trailing edge, and whether they were tapped.
    bool RecordKebab(const PlaneExtent& Extent, ControlIdentity Target, bool Hovered);

    /// 🧩 One inline action row — a glyph cell, a run, and an optional trailing chord.
    /// in    Target   [-]  the identity the row was registered under
    /// in    Colour       [-]  the run's own colour; the reference states `--colour` for four rows and its alert hue
    ///                      for the fifth, and the hover wash is that same colour at a twelfth of its coverage
    /// in    GlyphColour  [-]  the leading cell's colour, which is `--muted` wherever the run is not alerting
    /// out   Taken     [-]  true on the tick the row resolves a tap
    bool RecordActionRow(const PlaneExtent& Extent, ControlIdentity Target, SymbolSubject Glyph,
                         const char* Caption, const char* Chord, ThemeToken Colour, ThemeToken GlyphColour);

    /// 🧩 Slide one in Texture Paint — the reference's `LayersPane`, header, toolbar, stack and footer.
    void RecordLayerStack(const PlaneExtent& Extent, ShellContext& Applied,
                          const LayerRow* Layers, std::uint32_t LayerCount);

    /// 🧩 One layer row — its spine, its two halves and, when unfolded, its two property columns.
    /// out   Consumed  [px]  what the row actually spanned across, folded or unfolded
    float RecordLayerRow(const PlaneExtent& Extent, ShellContext& Applied, const LayerRow& Current,
                         std::uint32_t Ordinal, std::uint32_t LayerCount, bool Trailing);

    /// 🧩 Slide two in Texture Paint — the reference's `LayerInspectorPane` and the panel it delegates to.
    void RecordLayerInspector(const PlaneExtent& Extent, const ShellContext& Applied,
                              const LayerRow* Layers, std::uint32_t LayerCount);
    void RecordRetentionField(const PlaneExtent& Extent, ShellContext& Applied);
    void RecordPaneHeader(const PlaneExtent& Extent, SymbolSubject Glyph, ThemeToken GlyphColour,
                          ThemeToken MedallionColour, const char* Title, const char* Secondary);

    /// 🧩 Whether a row is presented, given the filter and every enclosure's disclosure.
    bool RowCurrent(const ShellContext& Applied, const EntityRow* Rows,
                      std::uint32_t RowCount, std::uint32_t Ordinal) const;

    InteractionIndex*              Ledger     = nullptr;   // [-] - borrowed; never owned
    MotionIntegrator*              Motion     = nullptr;   // [-] - borrowed; never owned
    RecordingSurface*              Surface    = nullptr;   // [-] - borrowed; never owned
    const ThemeProfile* Appearance = nullptr;   // [-] - borrowed; never owned
    ControlPanel                   Controls   = {};        // [-] - the shared inspector controls

    ShellColour     Tinted  = {};   // [-] - applied once at construction
    ShellMetric  Scaled  = {};   // [-] - re-applied whenever the appearance is resolved again

    PointerCondition  Sampled       = {};      // [-] - this tick's contact
    std::uint32_t     CarouselSlide = 0u;      // [-] - the registered traverse carrying the two slides
    PlaneExtent       InspectorAt   = {};      // [px] - where the inspector was last recorded
    bool              InspectorStood = false;  // [-] - whether it was recorded at all last tick

    ControlIdentity  DockSwitch    = {};   // [-] - the Dock Inspector switch
    ControlIdentity  ModeButtons[3] = {};  // [-] - the three workspace buttons
    ControlIdentity  RetentionField   = {};   // [-] - the outliner's retention run
    ControlIdentity  VeilContact   = {};   // [-] - the veil, which dismisses the menu
    // 📝 The metadata pane's own contacts. The call is the pointer twin of Tab; the five actions are the
    //    reference's `New record`, `Rename`, `Duplicate`, `Hide` and `Delete`, in that order.
    ControlIdentity  AdvanceCall                = {};   // [-] - the Properties & History call
    ControlIdentity  MetadataActions[5]         = {};   // [-] - the five inline actions

    // 📝 The directory strip and the Context Menu surface it discloses. The six colour discs and the two
    //    actions are registered once and lent to whichever presentation is recording — the page and the
    //    floating card are the same options and can never both take a contact in one tick.
    // 📝 Slide two's own strip, its Back call, and the fold each property card and revision group carries.
    ControlIdentity  InspectorStrip             = {};   // [-] - the Properties / History strip
    ControlIdentity  BackCall                   = {};   // [-] - "Back to scene directory"
    ControlIdentity  CardFolds[CardCeiling]     = {};   // [-] - one per property card
    ControlIdentity  RevisionGroups[RowCeiling] = {};   // [-] - one per grouped revision header

    ControlIdentity  OutlineStrip             = {};   // [-] - the Outliner / Context Menu strip
    ControlIdentity  ContextTints[7]            = {};   // [-] - six hues and the one that clears them
    ControlIdentity  ContextActions[2]          = {};   // [-] - Rename and Delete
    ControlIdentity  ContextVeil                = {};   // [-] - the fixed inset the card dismisses against
    ControlIdentity  RowKebabs[RowCeiling]      = {};   // [-] - one per outline row

    ControlIdentity  RowContacts[RowCeiling]    = {};   // [-] - one per outline row
    ControlIdentity  RowDisclosures[RowCeiling] = {};   // [-] - one per disclosure cell
    ControlIdentity  RowPresences[RowCeiling]   = {};   // [-] - one per presence action

    // 📝 The Layer Stack's own contacts. Its rows carry two takeable halves and three actions apiece, so
    //    they are registered as their own run rather than shared with the outliner's — a shared identity
    //    would let a contact on an outline row hover a layer row that is not even presented this tick.
    ControlIdentity  LayerHalves[LayerCeiling * 2u] = {};   // [-] - the layer half and the mask half
    ControlIdentity  LayerFolds[LayerCeiling]       = {};   // [-] - one per disclosure chevron
    ControlIdentity  LayerPresences[LayerCeiling]   = {};   // [-] - one per eye
    ControlIdentity  LayerMaskEyes[LayerCeiling]    = {};   // [-] - one per mask eye
    ControlIdentity  LayerRetires[LayerCeiling]     = {};   // [-] - one per bin
    ControlIdentity  LayerAdd                       = {};   // [-] - the Add layer button
    ControlIdentity  LayerRetention                 = {};   // [-] - the Filter layers run

    // 📝 The environment slider cards — six rows: sun elevation, azimuth, intensity and temperature, then
    //    sky intensity and atmosphere density. Each carries its own identity so a drag on one does not
    //    hover another, and the drag-end history demand keys off the released identity.
    ControlIdentity  EnvironmentSliders[6]          = {};   // [-] - one per environment slider row

    ComponentSpecification         EnvironmentControls = {};  // [-] - the environment slider rows
    bool                           EnvironmentArmed[6] = {};  // [-] - drag-start latched per slider
    double                         EnvironmentFrom[6]  = {};  // [-] - the value at drag start
};

SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded);

}   // namespace Slate
