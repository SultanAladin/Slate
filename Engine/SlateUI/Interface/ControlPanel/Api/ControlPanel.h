//============================================================================================================================================
//                                                             CONTROLPANEL.H
//============================================================================================================================================
// 🧩 Every primitive control as one immediate-mode free function over a rectangle — no widget owns anything, and the theme arrives by reference.

#pragma once

#include "Contract/OutcomeContract.h"
#include "Contract/PrecisionContract.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/WorkspaceSpace.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT A CONTROL REPORTS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What one control did this tick — the pointer over it, and the three points of an edit's lifecycle.
/// note  🔴 `10` §2.4: a drag is **one** transaction and not one per tick. A caller opens on `EditOpened`, amends
///        on every `EditDeclared`, and seals on `EditSealed`. A control that reported only "the reading changed"
///        would leave the caller unable to tell a drag from a run of separate edits, and the defect presents as a
///        revision row per pixel of pointer travel — which is exactly `84`'s merge requirement failing.
/// note  ⚠️ `EditSealed` arrives on the tick the pointer is released and carries no further amendment with it.
///        `EditDeclared` may be false on that tick; sealing is not conditional on it.
/// tag   contract, nonallocating, nonthrowing
struct ControlInteraction
{
    bool  PointerOver    = false;   // [-] - the pointer covers the control's rectangle
    bool  EditOpened     = false;   // [-] - the first tick of a held edit
    bool  EditDeclared   = false;   // [-] - the carried reading was amended this tick
    bool  EditSealed     = false;   // [-] - the hold ended this tick
    bool  EditAbandoned  = false;   // [-] - the edit was discarded rather than sealed
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE STROKE ALPHABET
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The chrome glyphs a control paints from line and arc primitives when no uploaded glyph is named.
/// note  📝 Flat single-colour chrome is a dozen strokes, and a stroke costs no upload, no descriptor and no
///        rasteriser. A panel that wants authored art names a depot slot instead and the stroke is not consulted.
/// tag   contract
enum class ControlStroke : std::uint32_t
{
    None    =  0u,   // [-] - paint nothing; the caller is filling the square itself
    Twisty  =  1u,   // [-] - a disclosure chevron, rotated by the caller's openness
    Chevron =  2u,   // [-] - a bare chevron, never rotated
    Caret   =  3u,   // [-] - the downward cap of a dropdown
    Plus    =  4u,   // [-] - add
    Cross   =  5u,   // [-] - dismiss, close, withdraw
    Check   =  6u,   // [-] - a satisfied condition
    Eye     =  7u,   // [-] - visibility
    Trash   =  8u,   // [-] - discard
    Search  =  9u,   // [-] - a filter entry's cap
    Cog     = 10u,   // [-] - configuration
    Image   = 11u,   // [-] - a surface or thumbnail placeholder
    Brush   = 12u,   // [-] - the painting discipline
    Reload  = 13u,   // [-] - resample, refresh, revert
    Circle  = 14u,   // [-] - a filled disc, the subject dot
    Grip    = 15u    // [-] - the corner hatching of a resize handle
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE ROW SPLIT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One control row divided into its label column and the field beside it.
/// tag   contract, nonallocating, nonthrowing
struct ControlRowSplit
{
    WorkspaceRectangle  LabelArea = {};   // [px] - the caption, vertically centred, clipped with an ellipsis
    WorkspaceRectangle  FieldArea = {};   // [px] - everything the control itself occupies
};

/// 🧩 Divides one row into remix's 88 px label column and the field that takes the remainder.
/// in    Theme  [-]   read for the two column extents and the gap between them
/// in    Area   [px]  the whole row
/// out   Split  [px]  the two rectangles, never overlapping
/// note  ⚠️ Below `LabelColumnWidth + LabelColumnGap + ValueColumnWidth` the fixed split cannot hold, and the row
///        falls back to `LabelColumnRatio`. Frontier clipped the field to nothing instead, and the defect presents
///        as a docked-narrow panel whose sliders are all zero pixels wide and cannot be grabbed at all.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
ControlRowSplit ResolveControlRow(const ThemeSpecification& Theme, const WorkspaceRectangle& Area);

/// 🧩 Paints one row's caption in the muted text colour, clipped to its column.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
void PresentControlLabel(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, const char* Caption);

/// 🧩 Paints one stroke centred in a square, at the coverage and thickness the caller declares.
/// in    Rotation  [rad]  applied about the square's centre; a twisty opens at a quarter turn
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
void PresentControlStroke(const WorkspaceRectangle& Area,
                          ControlStroke             Stroke,
                          const ThemeColour&        Colour,
                          float                     Thickness,
                          float                     Rotation);

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE NUMERIC ENTRIES
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 A label, a fixed 78 px value box, and a track whose knob sits at the reading's fraction of its span.
/// in    Theme     [-]   read and never held
/// in    Area      [px]  the whole row, label included
/// in    Caption   [-]   the label column's text
/// in    Carried   [-]   the reading, amended in place when the artist drags or types
/// in    Floor     [-]   the low end of the span
/// in    Ceiling   [-]   the high end
/// in    Unit      [-]   the unit cap's text; the reference prints a middot where a quantity is dimensionless
/// in    Decimals  [-]   digits after the point in the readout; zero rounds the reading to an integer
/// out   Outcome   [-]   refuses with ContentUnsupported when the ceiling does not exceed the floor, and with
///                       ExtentExhausted when the field cannot carry the value box and a grabbable track
/// note  🔴 The refusal on an empty span is the point. A slider over `Floor == Ceiling` divides by zero to place
///        its knob; clamping the span silently instead gives the artist a control that looks live and is not.
/// cost  🚩
/// tag   api, nonthrowing
Outcome<ControlInteraction> PresentValueSlider(const ThemeSpecification&  Theme,
                                               const WorkspaceRectangle&  Area,
                                               const char*                Caption,
                                               double&                    Carried,
                                               double                     Floor,
                                               double                     Ceiling,
                                               const char*                Unit,
                                               std::uint32_t              Decimals);

/// 🧩 An unbounded scalar: the same value box, and a centre-knobbed track that accumulates travel rather than
///     mapping position, so the reading may leave any span.
/// in    Step     [-]  reading amended per pixel of horizontal travel
/// out   Outcome  [-]  refuses with ExtentExhausted when the field cannot carry both parts
/// note  ⚠️ The knob is painted at the track's centre always and never moves. It is a grab surface and not a
///        readout — a knob that travelled would imply a span this control does not have.
/// cost  🚩
/// tag   api, nonthrowing
Outcome<ControlInteraction> PresentScalarEntry(const ThemeSpecification&  Theme,
                                               const WorkspaceRectangle&  Area,
                                               const char*                Caption,
                                               double&                    Carried,
                                               double                     Step,
                                               const char*                Unit,
                                               std::uint32_t              Decimals);

/// 🧩 Three value boxes side by side, each capped with its axis letter, each dragging its own component.
/// in    Carried  [-]  three components, amended in place
/// out   Outcome  [-]  refuses with ExtentExhausted when the field cannot carry three boxes
/// cost  🚩
/// tag   api, nonthrowing
Outcome<ControlInteraction> PresentVectorEntry(const ThemeSpecification&  Theme,
                                               const WorkspaceRectangle&  Area,
                                               const char*                Caption,
                                               double                     Carried[3],
                                               double                     Step,
                                               std::uint32_t              Decimals);

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE CHOICE ENTRIES
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 A 50×32 travel whose nub crosses on a click.
/// note  The nub's travel is not animated here. `14` §4.1 places animation carry beside the caller, and a control
///        holding its own would be retained state — which this file has none of by construction.
/// cost  ✔️
/// tag   api, nonthrowing
Outcome<ControlInteraction> PresentBooleanEntry(const ThemeSpecification&  Theme,
                                                const WorkspaceRectangle&  Area,
                                                const char*                Caption,
                                                bool&                      Carried);

/// 🧩 A wrapping run of pills, the chosen one filled with the accent and lettered in the on-accent colour.
/// in    Choices        [-]  the captions, read and never held
/// in    ChoiceCount    [-]  how many
/// in    CarriedOrdinal [-]  which is chosen, amended in place
/// out   Outcome        [-]  refuses with ContentUnsupported for no choices or an ordinal outside them
/// cost  🚩
/// tag   api, nonthrowing
Outcome<ControlInteraction> PresentSelectionEntry(const ThemeSpecification&  Theme,
                                                  const WorkspaceRectangle&  Area,
                                                  const char*                Caption,
                                                  const char* const*         Choices,
                                                  std::uint32_t              ChoiceCount,
                                                  std::uint32_t&             CarriedOrdinal);

/// 🧩 What a dropdown carries between ticks — its openness and the anchor its list drops from.
/// note  🔴 Owned by the caller and never by the control. Two dropdowns sharing one carry are two dropdowns that
///        open together, which is the defect that follows from a control holding openness of its own.
/// tag   owning
struct DropdownCarry
{
    bool           ListOpen     = false;   // [-]  - the list is presenting
    std::uint32_t  OpenedTick   = 0u;      // [-]  - so the press that opened it does not dismiss it
    float          AnchorX      = 0.0f;    // [px] - the list's top-left
    float          AnchorY      = 0.0f;    // [px]
};

/// 🧩 A 26 px head with a caret cap, and a hand-rolled list beneath it.
/// in    PresentedTick  [-]  the desk's own presentation count, compared against the carry's opening tick
/// out   Outcome        [-]  refuses with ContentUnsupported for no choices or an ordinal outside them
/// note  🔴 The list is painted on the foreground recording and is **not** a vendor popup, for the same reason the
///        tab overlays are not: every trapezoid is already on that recording, and a popup sits beneath it.
/// cost  🚩
/// tag   api, nonthrowing
Outcome<ControlInteraction> PresentDropdown(const ThemeSpecification&  Theme,
                                            const WorkspaceRectangle&  Area,
                                            const char* const*         Choices,
                                            std::uint32_t              ChoiceCount,
                                            std::uint32_t&             CarriedOrdinal,
                                            DropdownCarry&             Carry,
                                            std::uint32_t              PresentedTick);

/// 🧩 A 26 px pill row — the reference's `.seg`, filled when it is on.
/// note  Distinct from a selection entry: a segment row is a run of independent switches, not one exclusive
///        choice, so each pill carries its own boolean and pressing one does not clear the others.
/// cost  🚩
/// tag   api, nonthrowing
Outcome<ControlInteraction> PresentSegmentRow(const ThemeSpecification&  Theme,
                                              const WorkspaceRectangle&  Area,
                                              const char* const*         Captions,
                                              bool*                      Carried,
                                              std::uint32_t              SegmentCount);

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE TEXT ENTRIES
//------------------------------------------------------------------------------------------------------------------------

// 📝 The extent one text carry accepts. Matched to `WorkspaceTitleExtent` so an inline rename and a text entry
//    can share a carry without either truncating what the other accepted.
inline constexpr std::uint32_t ControlTextExtent = WorkspaceTitleExtent;

/// 🧩 One open text edit — the edited text, never the committed one.
/// note  🔴 Sealing writes the carry back to whatever owns the text; abandoning discards it. The owner's text is
///        never written during the edit, so an abandoned rename leaves no trace and a refused seal leaves none.
/// tag   owning
struct TextCarry
{
    bool           EditOpen                      = false;   // [-] - an edit is running
    char           Carried[ControlTextExtent]    = {};      // [-] - the edited text, terminated
    std::uint32_t  CarryExtent                   = 0u;      // [-] - characters held, terminator excluded
    std::uint32_t  CaretPosition                 = 0u;      // [-] - where typed characters arrive
};

/// 🧩 A rounded field the artist types into, opened by a click and sealed by Enter or by a press elsewhere.
/// out   Outcome  [-]  refuses with ExtentExhausted when the field is narrower than one glyph
/// cost  🚩
/// tag   api, nonthrowing
Outcome<ControlInteraction> PresentTextEntry(const ThemeSpecification&  Theme,
                                             const WorkspaceRectangle&  Area,
                                             const char*                Caption,
                                             TextCarry&                 Carry,
                                             const char*                Placeholder);

/// 🧩 A text edit with no field of its own, painted over whatever it is renaming.
/// note  What a tab's double-click rename rides on. It paints a caret and an accent hairline and nothing else, so
///        the trapezoid beneath it stays visible and the artist can see what is being renamed.
/// cost  🚩
/// tag   api, nonthrowing
Outcome<ControlInteraction> PresentInlineTextEditor(const ThemeSpecification&  Theme,
                                                    const WorkspaceRectangle&  Area,
                                                    TextCarry&                 Carry);

/// 🧩 A path field and the round browse cap beside it.
/// out   Outcome  [-]  the interaction; `EditSealed` on the browse cap means the caller should open a chooser
/// note  ⚠️ This control never touches the file system. `04`'s interchange owns that, and a control that opened a
///        chooser itself would be a panel holding what it presents.
/// cost  🚩
/// tag   api, nonthrowing
Outcome<ControlInteraction> PresentPathEntry(const ThemeSpecification&  Theme,
                                             const WorkspaceRectangle&  Area,
                                             const char*                Caption,
                                             TextCarry&                 Carry,
                                             bool&                      BrowseDeclared);

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE REMAINING PRIMITIVES
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 A fully rounded bar carrying a swatch disc, the coordinate printed, and a caret cap.
/// in    Carried  [-]  the colour, amended in place; its space is carried through untouched
/// note  🔴 `36` §1: the coordinate keeps its declared space across the edit. This control never projects and
///        never spells a transfer — `14` §5 places the interface after the display projection, and a control that
///        converted here would be the second transfer the whole arrangement exists to prevent.
/// cost  🚩
/// tag   api, nonthrowing
Outcome<ControlInteraction> PresentColourEntry(const ThemeSpecification&  Theme,
                                               const WorkspaceRectangle&  Area,
                                               const char*                Caption,
                                               ThemeColour&               Carried,
                                               bool&                      PickerOpen);

/// 🧩 A caption pill that reads as pressable — what a menu band is a run of.
/// cost  ✔️
/// tag   api, nonthrowing
Outcome<ControlInteraction> PresentMenuPill(const ThemeSpecification&  Theme,
                                            const WorkspaceRectangle&  Area,
                                            const char*                Caption,
                                            bool                       Highlighted);

/// 🧩 A square glyph that answers a press — the uploaded glyph when a depot slot is named, the stroke otherwise.
/// in    DepotSlot  [-]  an opaque `GlyphHandle::DepotSlot`; zero falls back to the stroke
/// in    Stroke     [-]  what is painted when no slot is named, or when the named one has been reclaimed
/// note  🔴 The fallback is not a convenience. A depot that reclaimed a tier mid-session must not leave a panel
///        painting nothing at all — a missing icon that still answers a press is recoverable, an invisible button
///        is not. The stroke is therefore always supplied, even where a slot is expected to resolve.
/// note  ⚠️ The slot crosses as a bare integer so that no vendor spelling enters this header. `GlyphDepot.h`
///        already documents it as an integer whose meaning only its own source knows.
/// cost  ✔️
/// tag   api, nonthrowing
Outcome<ControlInteraction> PresentGlyphButton(const ThemeSpecification&  Theme,
                                               const WorkspaceRectangle&  Area,
                                               ControlStroke              Stroke,
                                               std::uint64_t              DepotSlot,
                                               bool                       Highlighted);

/// 🧩 A 29 px accordion header with a twisty that turns a quarter as it opens.
/// in    SectionOpen  [-]  amended in place by a press anywhere on the header
/// cost  ✔️
/// tag   api, nonthrowing
Outcome<ControlInteraction> PresentSectionHeader(const ThemeSpecification&  Theme,
                                                 const WorkspaceRectangle&  Area,
                                                 const char*                Caption,
                                                 bool&                      SectionOpen,
                                                 const char*                Trailing);

/// 🧩 What a carousel carries between ticks — which pane is presented and how far the slide has travelled.
/// tag   owning
struct CarouselCarry
{
    std::uint32_t  PresentedPane = 0u;     // [-] - the pane the caller should fill this tick
    std::uint32_t  ArrivingPane  = 0u;     // [-] - where the slide is heading; equal when at rest
    float          Travelled     = 1.0f;   // [-] - the slide's completion, one at rest
};

/// 🧩 Advances a carousel's slide and reports the horizontal offset each pane paints at.
/// in    ElapsedInterval  [s]   since the previous tick, from the tick's own clock
/// out   Outcome          [px]  the offset to add to the presented pane's origin; the arriving pane sits one
///                              body width further along in the direction of travel
/// note  📝 `84`'s Properties/History pair rides this. The offset is returned rather than applied so that the
///        caller clips its own body — a carousel that clipped for its caller would need to know what a pane is.
/// cost  ✔️
/// tag   api, nonthrowing
Outcome<float> AdvanceContentCarousel(const ThemeSpecification&  Theme,
                                      CarouselCarry&             Carry,
                                      float                      ElapsedInterval);

// 📐 Captions, ordinals and choice counts are Exact. Rectangles, readings, fractions and travel are Bounded.
//    The component claims Bounded, per `00` §3's transitivity rule.
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded, PrecisionGuarantee::Exact);

}   // namespace Slate
