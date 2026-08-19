//============================================================================================================================================
//                                                          LAYERSTACKPANEL.H
//============================================================================================================================================
// 🧩 Records the texture-paint layer stack, the channel property panel and the mask property panel exactly as their references present them.

#pragma once

#include "Contract/DeliveryContract.h"
#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "SlateUI/Interface/InteractionIndex/Api/InteractionIndex.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"
#include "SlateUI/Interface/LayerStackSpecification/Api/LayerStackSpecification.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE SEATED INKS
//------------------------------------------------------------------------------------------------------------------------

// 📝 `LayerStackInk` now lives in `AppearanceSpecification.h`, beside every other ink the interface draws
//    with. It moved so the appearance file can reach it: a token run declared in a panel header is one the
//    Control Centre cannot theme. The spellings are unchanged.

/// 🧩 Every length `LayerstackV1` states, at the artist's own scale.
/// tag   contract, nonallocating, nonthrowing
struct LayerStackMetric
{
    float  PanelAlong       = 392.0f;   // [px] - --w, the panel's own extent
    float  HeadAcross       =  40.0f;   // [px] - .head, 11px over and 9px under an 20px run
    float  HeadPadAlong     =  14.0f;   // [px] - .head padding-left
    float  ToolsAcross      =  44.0f;   // [px] - .tools, 8px over and under a 28px field
    float  ToolsPadAlong    =  10.0f;   // [px] - .tools padding
    float  SearchAcross     =  28.0f;   // [px] - .search height
    float  RowAcross        =  45.0f;   // [px] - .row min-height
    float  MaskRowAcross    =  37.0f;   // [px] - .row.msk min-height
    float  RowPadAlong      =  10.0f;   // [px] - .row padding-left
    float  RowGapAlong      =   8.0f;   // [px] - .row gap
    float  RowStepAlong     =  27.0f;   // [px] - .kids margin-left 15 + padding-left 12
    float  MaskLeadAlong    =  26.0f;   // [px] - .attach padding-left
    float  TagAlong         =   3.0f;   // [px] - .tag width
    float  DiscloseAlong    =  14.0f;   // [px] - .tw width
    float  ActionExtent     =  23.0f;   // [px] - .ico.sm
    float  ButtonExtent     =  28.0f;   // [px] - .ico
    float  ThumbExtent      =  35.0f;   // [px] - .thumb
    float  ThumbMini        =  27.0f;   // [px] - .thumb.mini
    float  BadgeExtent      =  15.0f;   // [px] - .thumb .badge
    float  ColumnsLeast     = 580.0f;   // [px] - the extent at which `wide` seats the columns
    float  BlendColumnAlong = 118.0f;   // [px] - .col-blend width
    float  OpacityColumnAlong = 110.0f; // [px] - .col-op width
    float  OpacityReadAlong =  32.0f;   // [px] - .opn width
    float  ColumnGapAlong   =   7.0f;   // [px] - .col-op gap
    float  MiniAcross       =   4.0f;   // [px] - .mini height
    float  ChipAcross       =  18.0f;   // [px] - .chip height
    float  StackPadAlong    =   8.0f;   // [px] - .stack padding
    float  StackPadAcross   =   6.0f;   // [px] - .stack padding-top
    float  ScrollAlong      =  10.0f;   // [px] - .stack::-webkit-scrollbar width
    float  SectionAcross    =  30.0f;   // [px] - .sech, 8px over and under a 14px run
    float  CardPadAlong     =  12.0f;   // [px] - .cbody padding
    float  FieldAcross      =  26.0f;   // [px] - one folded property row
    float  FootAcross       =  96.0f;   // [px] - .foot, crumb over properties over the action bar
    float  FootPadAlong     =  10.0f;   // [px] - .foot padding
    float  RadiusStandard   =  10.0f;   // [px] - --r
    float  RadiusSmall      =   7.0f;   // [px] - --r-s
    float  RadiusPill       = 999.0f;   // [px] - --pill
    float  RunHead          =  11.5f;   // [px] - .head h1
    float  RunRow           =  12.5f;   // [px] - .name
    float  RunSub           =  10.5f;   // [px] - .sub
    float  RunFine          =  10.0f;   // [px] - .count, .chip
    float  RunSection       =   9.5f;   // [px] - .sech
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT THE HOST OWNS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Which popup, if any, the stack has standing.
/// note  🔴 One popup at a time, which `InteractionIndex` already enforces for the CONTACT. This states
///        *which kind* stands, because the ledger records only that one is disclosed and the panel must
///        know whether to draw a blend run, a colour wheel or a layer menu into it.
/// tag   contract
enum class StackPopup : std::uint32_t
{
    Absent      = 0u,   // [-] - nothing stands open
    Addition    = 1u,   // [-] - the tools `+`; the seven declarations and their chords
    BlendMode   = 2u,   // [-] - the footer pill; the twenty-nine blend runs
    LayerMenu   = 3u,   // [-] - one entry's own menu, from its ellipsis or a secondary contact
    MaskMenu    = 4u,   // [-] - one mask's menu — source, generator, invert, clear
    EffectMenu  = 5u,   // [-] - the effect run a card's `+` offers
    ColourWheel = 6u,   // [-] - the hue ring, its luminance run and its hexadecimal field
    PopupCount  = 7u    // [-] - the closed count, never a popup
};

/// 🧩 What one drop would do to the arrangement — the reference's `dropM`.
/// tag   contract
enum class DropIntent : std::uint32_t
{
    Absent   = 0u,   // [-] - the carried entry stands over nothing droppable
    Prior    = 1u,   // [-] - lands before the destination
    Trailing = 2u,   // [-] - lands after it
    Enclosed = 3u    // [-] - lands inside it; only over a folder, and only across its middle third
};

/// 🧩 Everything the stack retains between ticks, which the host owns and the panel amends.
/// note  🔴 Held by the HOST and not by the panel, on `14` §1's terms: a panel stores none of what it
///        presents. Every reading here is interaction — where the stack is scrolled, what is roused, what
///        is being carried — and none of it is a datum the artist edits.
/// tag   contract, nonallocating, nonthrowing
struct LayerStackOrdinates
{
    static constexpr std::uint32_t RetentionCeiling = 48u;   // [-] - characters the search run retains
    static constexpr std::uint32_t NamingCeiling    = LayerStackCeiling::NamingCeiling;

    float          StackOffset   = 0.0f;    // [px] - how far the stack is scrolled
    float          StackSpan     = 0.0f;    // [px] - the recorded extent, resolved each tick
    std::uint32_t  Hovered       = LayerStackCeiling::AbsentOrdinal;   // [-] - which entry is roused
    bool           HoveredMask   = false;   // [-] - whether it is that entry's mask row
    bool           ContactPrior  = false;   // [-] - the previous tick's contact, for edge detection

    // 📐 `$('#q')` — the search run, and whether it holds text entry.
    char           Retention[RetentionCeiling] = {};   // [-] - what the artist typed
    bool           RetentionRoused = false;            // [-] - the field has the keyboard

    // 📐 `renameRow` — the in-place naming field, which is the only other text entry the panel has.
    std::uint32_t  Renaming        = LayerStackCeiling::AbsentOrdinal;   // [-] - which entry is renaming
    char           RenamingRun[NamingCeiling] = {};                      // [-] - what stands typed

    // 📐 The reference's `drag`, `dropT` and `dropM`.
    std::uint32_t  Carried      = LayerStackCeiling::AbsentOrdinal;   // [-] - which entry is being dragged
    std::uint32_t  Destination  = LayerStackCeiling::AbsentOrdinal;   // [-] - what it stands over
    DropIntent     Intent       = DropIntent::Absent;                 // [-] - what the drop would do
    float          CarryOrigin  = 0.0f;   // [px] - where the contact arrived, to separate a drag from a tap

    // 📐 `show()` / `hide()` — one popup, its anchor and whichever entry it addresses.
    StackPopup     Popup          = StackPopup::Absent;

    // 🔴 The ANCHOR and the resolved SEAT are separate. The anchor is the trailing edge of whatever opened
    //    the popup and never moves; the seat is where the card actually landed after being clamped inside
    //    the display. Folding the two into one reading made every tick re-clamp against the previous
    //    tick's clamp, walking the card across the display — and, worse, made the veil test the wrong
    //    extent, so the contact that opened a popup's own entry dismissed it instead.
    float          PopupAlong     = 0.0f;    // [px] - the anchor's trailing edge
    float          PopupAcross    = 0.0f;    // [px] - the anchor's lower edge
    float          PopupSeatAlong  = 0.0f;   // [px] - where the card landed
    float          PopupSeatAcross = 0.0f;   // [px]
    float          PopupSeatSpan   = 0.0f;   // [px] - how tall it was recorded

    // 🔴 Whether the popup has stood for at least one whole tick. A popup opened by a RELEASE has its
    //    entries recorded under that same release, and the entry the pointer happens to rest on resolves
    //    immediately — so opening the footer's blend run instantly restated the blend to whatever entry
    //    the pill was sitting under. An entry resolves only once the popup has settled.
    bool           PopupSettled    = false;
    std::uint32_t  PopupSubject   = LayerStackCeiling::AbsentOrdinal;   // [-] - the entry it addresses
    bool           PopupOnMask    = false;   // [-] - whether it addresses that entry's mask
    float          PopupOffset    = 0.0f;    // [px] - how far a long popup is scrolled

    // 📐 `colorWheel` — hue, saturation and luminance, retained while the wheel stands open.
    float          WheelHue        = 0.0f;    // [deg] - 0…360
    float          WheelSaturation = 0.0f;    // [%]   - 0…100
    float          WheelLuminance  = 60.0f;   // [%]   - 4…96, the reference's own clamp

    // 📐 `[data-tip]` — the roused tooltip, which is recorded in the deferred sweep and not in place.
    const char*    Tooltip        = nullptr;   // [-] - borrowed; absent when nothing is roused
    float          TooltipAlong   = 0.0f;      // [px] - the roused control's own centre
    float          TooltipAcross  = 0.0f;      // [px] - its upper edge

    // 📐 `Inspector.tsx` `renderHistory()` — the revision pane's own interaction. The reference retains
    //    `collapsedHistory` per token and `expandedRevisions` per revision; the pane here presents one
    //    token, so the run is folded by a single reading and each entry unfolds on its own.
    static constexpr std::uint32_t RevisionCeiling = 16u;   // [-] - entries carrying enrolled cells
    static constexpr std::uint32_t RemarkCeiling   = 64u;   // [-] - characters one comment retains

    bool           RevisionsFolded = false;   // [-] - `collapsedHistory[token]`, the head's own chevron
    std::uint32_t  RevisionShown   = LayerStackCeiling::AbsentOrdinal;   // [-] - `expandedRevisions[rev.id]`
    float          RevisionOffset  = 0.0f;    // [px] - how far the pane is scrolled
    float          RevisionSpan    = 0.0f;    // [px] - the recorded extent, resolved each tick

    // 📐 The unfolded card's two editors. The reference writes each back on BLUR — `updateRevision(rev.id,
    //    {comment})` — so what is typed lives here until the keyboard leaves the field, and only then is
    //    it the revision's own reading.
    std::uint32_t  RevisionField   = 0u;      // [-] - 0 none, 1 the comment, 2 the value
    char           RevisionRemark[RevisionCeiling][RemarkCeiling] = {};   // [-] - `rev.comment`
    char           RevisionReading[RevisionCeiling][RemarkCeiling] = {};  // [-] - `rev.editValue`
};

//------------------------------------------------------------------------------------------------------------------------
//                                                         THE PANEL
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Records the layer stack and its two property panels exactly as `LayerstackV1` presents them.
/// note  🔴 Every primitive is recorded through `RecordingSurface` in one order; the panel opens no vendor
///        window. The stack scrolls by offsetting its own cursor inside a confined extent, because the
///        recording seam carries a wheel reading but no scrolling primitive of its own.
/// tag   owning
class LayerStackPanel
{
public:

    // 🔴 How many rows carry their own enrolled controls. Beyond this the rows still record and still
    //    respond to the take, but their per-cell actions fall back to the row's own contact — which is a
    //    graceful reduction and not a defect.
    // 🔴 The three runs together claim
    //        16 × 12  +  16  +  32  =  240
    //    slots from the ledger. The reference's own arrangement presents thirteen entries, so sixteen rows
    //    covers it with slack. A run sized past the ceiling refuses at `Construct` and the panel never
    //    records at all.
    // 🔴 The ledger is SHARED with every other panel the host constructs, so the assertion below is
    //    necessary and NOT sufficient — it weighs this panel alone. It once passed at 240 ≤ 256 while the
    //    host had already spent 159 slots on the sheet and the shell, and the layer stack was refused at
    //    bring-up. The host states the whole-host total in its own budget; keep both in step.
    static constexpr std::uint32_t RowCeiling      = 16u;   // [-] - rows carrying enrolled cells
    static constexpr std::uint32_t CellsPerRow     = 12u;   // [-] - see RowCell
    static constexpr std::uint32_t ChromeCeiling   = 20u;   // [-] - the head, the tools, the footer, the revision pane
    static constexpr std::uint32_t PopupEntryCeiling = 32u; // [-] - the longest popup is BLENDS, at 29

    // 📐 One cell per revision entry, so an entry unfolds under its own contact rather than under the
    //    pane's. `LayerStackOrdinates::RevisionCeiling` states how many the pane presents.
    static constexpr std::uint32_t RevisionCellCeiling = LayerStackOrdinates::RevisionCeiling;

    static_assert(RowCeiling * CellsPerRow + ChromeCeiling + PopupEntryCeiling + RevisionCellCeiling <=
                  InteractionIndex::ControlCapacity,
                  "the layer stack's enrolments exceed the interaction ledger's capacity — Construct would "
                  "refuse and the panel would record nothing; reduce RowCeiling or raise ControlCapacity");

    LayerStackPanel()                                  = default;
    LayerStackPanel(const LayerStackPanel&)            = delete;
    LayerStackPanel& operator=(const LayerStackPanel&) = delete;
    ~LayerStackPanel()                                 = default;

    /// 🧩 Binds the panel to one recording surface and claims every identity its controls are arbitrated
    ///    under, once, at bring-up.
    /// in    Interaction  [-]  borrowed; the ledger every control is enrolled into
    /// in    Recording    [-]  borrowed; must outlive the panel
    /// out   Deliver      [-]  refuses with ContentUnsupported when a construction already stands, and
    ///                         with whatever the ledger declined when an enrolment was refused
    /// note  🔴 Enrolment happens HERE and never inside a tick. A control enrolled mid-tick receives a
    ///        fresh fade and reads as though the pointer had only just arrived over it, every tick.
    /// cost  🚩
    /// tag   api, nonallocating, nonthrowing
    Deliver<bool> Construct(InteractionIndex& Interaction, RecordingSurface& Recording);

    /// 🧩 Samples this tick's contact, before any extent is recorded against it.
    /// in    Contact  [-]  what `RecordingSurface::Pointer` reported
    /// in    Elapsed  [ms] what the same tick's display condition measured
    /// note  ⚠️ Called once per tick, after `InteractionIndex::Advance` and before `RecordStack`.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Advance(const PointerCondition& Contact, double Elapsed);

    /// 🧩 Applies one arbitrated chord to the arrangement, recording a revision when it amends one.
    /// in    Subject     [-]  which of the closed roster arrived
    /// in    Modifiers   [-]  what stood down alongside it
    /// in    Revisions   [-]  borrowed; the ring an amendment is recorded into ahead of itself
    /// out   Consumed    [-]  true when the chord addressed the stack, so no other panel answers it
    /// note  🔴 Refuses every chord while a text entry stands, exactly as the reference's own guard does
    ///        — `if(e.target.tagName==='INPUT')return`. Without it, typing `p` into the search run
    ///        declares a paint layer.
    /// cost  🚩
    /// tag   api, nonallocating, nonthrowing
    bool AdmitChord(KeySubject Subject, const ModifierCondition& Modifiers,
                    LayerArrangement& Arrangement, LayerStackOrdinates& Seated,
                    RevisionSequence& Revisions);

    /// 🧩 Records the layer stack — header, tools, the nested rows and the footer — and arbitrates every
    ///    contact that lands inside it.
    /// in    Extent       [-]  the pane's own extent
    /// in    Arrangement  [-]  borrowed for the tick; the panel amends what the artist takes and moves
    /// in    Seated       [-]  retained between ticks
    /// in    Revisions    [-]  borrowed; recorded into ahead of every amendment a contact makes
    /// note  ⚠️ The popup and the tooltip are NOT recorded here — they must land above every row, which
    ///        means above rows this call has not recorded yet. Call `RecordDeferred` after the last panel.
    /// cost  🔴
    /// tag   api, nonallocating, nonthrowing
    void RecordStack(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                     LayerStackOrdinates& Seated, RevisionSequence& Revisions);

    /// 🧩 Records the channel property panel for whichever entry stands taken.
    /// note  📐 Reached when the taken half is the entry itself — a material, paint, fill or decal.
    /// cost  🔴
    /// tag   api, nonallocating, nonthrowing
    void RecordChannelProperties(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                                 LayerStackOrdinates& Seated, RevisionSequence& Revisions);

    /// 🧩 Records the mask property panel for whichever entry stands taken.
    /// note  📐 Reached when the taken half is the attached mask.
    /// cost  🔴
    /// tag   api, nonallocating, nonthrowing
    void RecordMaskProperties(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                              LayerStackOrdinates& Seated, RevisionSequence& Revisions);

    /// 🧩 Records the revision pane the inspector's second slide pairs with a property panel, and
    ///    arbitrates every contact that lands inside it.
    /// in    Extent       [-]  the pane's own extent
    /// in    Arrangement  [-]  borrowed; a revert or a reinstate restores straight into it
    /// in    Seated       [-]  retained between ticks; the fold, the unfolded entry and the scroll
    /// in    Revisions    [-]  borrowed; what stands recorded is presented above the seated reference run
    /// note  📐 `Inspector.tsx` `renderHistory()`: the head folds the whole run, each entry unfolds its own
    ///        card, and the two actions revert and reinstate through the ring.
    /// cost  🔴
    /// tag   api, nonallocating, nonthrowing
    void RecordRevisions(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                         LayerStackOrdinates& Seated, RevisionSequence& Revisions);

    /// 🧩 Records the standing popup and the roused tooltip, above everything recorded before them.
    /// note  🔴 A popup recorded in place is painted over by the next row the stack records. The reference
    ///        seats its `#pop` as a sibling of the whole panel for exactly this reason.
    /// cost  🔴
    /// tag   api, nonallocating, nonthrowing
    void RecordDeferred(LayerArrangement& Arrangement, LayerStackOrdinates& Seated,
                        RevisionSequence& Revisions);

    /// 🧩 The seated inks, so a host may state them in a proof.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    /// 🧩 Restates the panel's inks from a resolved appearance, so a theme change reaches it.
    /// in    Resolved  [-]  the appearance the host resolved for the chosen theme
    /// note  📐 Colours only. The lengths are the reference's own and a theme must not move one.
    ///        Nothing is borrowed — the inks are copied out, so the caller may let `Resolved` expire.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Reseat(const AppearanceSpecification& Resolved);

    const LayerStackInk& Inked() const { return Tinted; }

    /// 🧩 The seated lengths.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    const LayerStackMetric& Measured() const { return Scaled; }

    /// 🧩 Returns the panel to its constructed condition, retiring every claimed identity.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Reset();

private:

    /// 🧩 Which cell of one row a contact addressed — the reference's own `data-act` roster.
    /// note  📐 Both halves are enrolled per row rather than per half, because a mask row exists only when
    ///        its entry declares one and enrolling on that condition would claim identities inside a tick.
    enum class RowCell : std::uint32_t
    {
        Body        =  0u,   // [-] - the row itself; takes it
        Disclosure  =  1u,   // [-] - `data-act="tw"`, the folder twisty
        Presence    =  2u,   // [-] - `data-act="vis"`, the eye
        Unfolding   =  3u,   // [-] - `data-act="exp"`, the card chevron
        Menu        =  4u,   // [-] - `data-act="more"`, the ellipsis
        MaskBody    =  5u,   // [-] - the mask row
        MaskPresence=  6u,   // [-] - `data-act="mvis"`
        MaskUnfold  =  7u,   // [-] - `data-act="mexp"`
        MaskMenu    =  8u,   // [-] - `data-act="mmore"`
        Opacity     =  9u,   // [-] - the row's own opacity meter, dragged in place
        MaskDensity = 10u,   // [-] - the mask row's density meter
        Reserved    = 11u,   // [-] - held so CellsPerRow stays a stated constant, not a derived one
        CellCount   = 12u
    };

    /// 🧩 Which piece of the panel's own chrome a contact addressed.
    enum class ChromeCell : std::uint32_t
    {
        SearchField  =  0u,   // [-] - `#q`
        AddButton    =  1u,   // [-] - `#btnAdd`
        FolderButton =  2u,   // [-] - `#btnFolder`
        RetireButton =  3u,   // [-] - `#aDel`
        BlendPill    =  4u,   // [-] - `#btnBlend`, the footer
        OpacityRun   =  5u,   // [-] - `#opac`, the footer
        UndoButton   =  6u,   // [-] - `#btnUndo`
        RedoButton   =  7u,   // [-] - `#btnRedo`
        CollapseAll  =  8u,   // [-] - `#btnCollapse`
        ScrollThumb  =  9u,   // [-] - the stack's own bar, dragged
        Veil         = 10u,   // [-] - the whole display, which dismisses a standing popup
        PopupBody    = 11u,   // [-] - the popup's own extent, so a contact inside it is not a dismissal
        WheelRing    = 12u,   // [-] - the colour wheel's hue ring
        WheelLuma    = 13u,   // [-] - its luminance run
        WheelApply   = 14u,   // [-] - its Apply action
        RevisionHead = 15u,   // [-] - the revision pane's own head, which folds the whole run
        RevisionBar  = 16u,   // [-] - the revision pane's scroll bar, dragged in place
        RevertAction = 17u,   // [-] - the pane's revert action
        ReinstateAction = 18u,   // [-] - the pane's reinstate action
        Reserved     = 19u,
        CellCount    = 20u
    };

    /// 🧩 Whether one extent is roused, seizes on arrival and resolves on release — the whole arbitration
    ///    of a press, stated once so no call site re-derives three quarters of it and forgets the fourth.
    /// out   Resolved  [-]  true on the single tick the contact was released inside the extent
    bool Pressed(ControlIdentity Claimed, const PlaneExtent& Extent, LayerStackOrdinates& Seated,
                 const char* Tooltip = nullptr);

    /// 🧩 Whether one extent is roused this tick, without arbitrating a press.
    bool Roused(const PlaneExtent& Extent) const;

    /// 🧩 Drags one 0…100 reading along an extent, seizing it so the drag survives leaving the extent.
    /// out   Altered  [-]  true on any tick the reading moved
    bool Dragged(ControlIdentity Claimed, const PlaneExtent& Extent, std::uint32_t& Reading);

    void RecordEntryRow(const PlaneExtent& Extent, const LayerArrangement& Arrangement,
                        std::uint32_t Ordinal, bool Taken, bool Hovered);

    void RecordMaskRow(const PlaneExtent& Extent, const LayerEntry& Entry, bool Taken, bool Hovered);

    void RecordSectionHead(const PlaneExtent& Extent, const char* Caption, const char* Reading,
                           bool Opened);

    float RecordReadingRow(const PlaneExtent& Extent, const char* Caption, const char* Reading);

    void RecordMeter(const PlaneExtent& Extent, std::uint32_t Reading, InkOrdinate Ink);

    void RecordChip(const PlaneExtent& Extent, const char* Caption, InkOrdinate Ink, bool Solid);

    /// 🧩 Records the drop marker one carried entry would land against — a rule, or a ring around a folder.
    void RecordDropMark(const PlaneExtent& Extent, DropIntent Intent);

    /// 🧩 Records the popup ground and returns the extent its entries are laid into.
    PlaneExtent RecordPopupGround(LayerStackOrdinates& Seated, float Along, float Across, float Span);

    /// 🧩 Records one popup entry and reports whether it was pressed.
    bool RecordPopupEntry(const PlaneExtent& Extent, const char* Caption, const char* Chord,
                          bool Marked, bool Dangerous, LayerStackOrdinates& Seated);

    RecordingSurface*  Surface = nullptr;   // [-] - borrowed
    InteractionIndex*  Ledger  = nullptr;   // [-] - borrowed; never owned
    LayerStackInk      Tinted;              // [-] - the seated inks
    LayerStackMetric   Scaled;              // [-] - the seated lengths
    PointerCondition   Sampled = {};        // [-] - this tick's contact, taken at Advance

    ControlIdentity    RowCells[RowCeiling * CellsPerRow] = {};   // [-] - one per row, one per cell
    ControlIdentity    ChromeCells[ChromeCeiling]         = {};   // [-] - the panel's own chrome
    ControlIdentity    PopupEntries[PopupEntryCeiling]    = {};   // [-] - one per open popup entry
    ControlIdentity    RevisionCells[RevisionCellCeiling] = {};   // [-] - one per revision entry
};

}   // namespace Slate
