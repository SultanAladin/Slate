//============================================================================================================================================
//                                                           THEMESPECIFICATION.H
//============================================================================================================================================
// 🧩 The one palette and the one set of layout extents every panel reads — resolved once a tick, held by nobody else.

#pragma once

#include "Contract/OutcomeContract.h"
#include "SlateMath/Numeric/ColourProjection/Api/ColourProjection.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     ONE THEME COLOUR
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One interface colour: a coordinate carrying the space it is a coordinate in, plus the fraction it covers.
/// note  🔴 `36` §1 and `14` §5 together: a packed integer would be a bare triple whose space nothing states, and a
///        colour whose space nothing states is the shape a transfer function is later added to "fix". Held as a
///        `ColourSpecification` in the display space so the absence of any transfer inside `SlateUI` stays checkable.
/// note  ⚠️ Coverage is not part of `ColourSpecification` and never becomes part of it. Coverage is a compositing
///        fraction over the interface's own recording, not a property of the colour — `36` carries no alpha for
///        exactly that reason.
/// tag   contract, nonallocating, nonthrowing
struct ThemeColour
{
    ColourSpecification  Coordinate = {};    // [-] - in DisplaySpaceIdentity, at that space's declared transfer
    double               Coverage   = 1.0;   // [-] - fraction of the surface beneath it this colour covers
};

/// 🧩 Quantises one theme colour to the eight-bit-per-channel code the vendor draw surface accepts.
/// in    Colour  [-]  the colour, coordinates read in the display space
/// out   Code    [-]  packed as red in the low byte, then green, blue and coverage
/// note  🔴 The only place a theme colour becomes a bare integer, and it is a quantisation and not a projection.
///        Coordinates outside the unit interval are bounded here rather than wrapping, because a wrapped code is a
///        colour the artist sees as a wrong hue rather than as a clipped one.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
std::uint32_t Quantize(const ThemeColour& Colour);

/// 🧩 Returns one theme colour at a declared coverage, its coordinate untouched.
/// in    Colour            [-]  the colour to re-cover
/// in    DeclaredCoverage  [-]  the fraction, bounded to the unit interval
/// note  What a hover tint or a disabled row is — the same colour at less coverage, never a second literal.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
ThemeColour Attenuate(ThemeColour Colour, double DeclaredCoverage);

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE PALETTE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every colour the interface draws with, named by what it is drawn on rather than by its hue.
/// note  🔴 A second palette — light, high contrast — is one more value of this struct and never a parallel set of
///        literals. A component that spells a colour of its own is the defect this struct exists to prevent.
/// tag   owning
struct ThemePalette
{
    ThemeColour  DeskBackground      = {};   // [-] - behind every panel, where nothing is docked
    ThemeColour  PanelBackground     = {};   // [-] - a panel body
    ThemeColour  PanelHeader         = {};   // [-] - a section header strip
    ThemeColour  PanelBorder         = {};   // [-] - panel outlines and separators
    ThemeColour  ControlBackground   = {};   // [-] - a slider track or entry field
    ThemeColour  ControlHovered      = {};   // [-] - a control under the pointer
    ThemeColour  ControlActive       = {};   // [-] - a control while it is being dragged
    ThemeColour  TileBackground      = {};   // [-] - a card, thumbnail or layer tile face
    ThemeColour  TileHovered         = {};   // [-] - that tile under the pointer
    ThemeColour  RowHovered          = {};   // [-] - the wash a list row takes under the pointer
    ThemeColour  AccentPrimary       = {};   // [-] - selection, active tab, slider grab
    ThemeColour  AccentSubtle        = {};   // [-] - the accent at hover coverage
    ThemeColour  DangerPrimary       = {};   // [-] - a destructive control: discard, delete, refusal
    ThemeColour  SelectionMarker     = {};   // [-] - the marker beside a chosen entry
    ThemeColour  TextPrimary         = {};   // [-] - a label
    ThemeColour  TextMuted           = {};   // [-] - a secondary or refused label
    ThemeColour  TextOnAccent        = {};   // [-] - a label drawn over an accent fill
    ThemeColour  ValueNumberSegment  = {};   // [-] - the centre segment of a numeric entry
    ThemeColour  ValueSideSegment    = {};   // [-] - the axis and unit segments either side of it
    ThemeColour  ValueOutline        = {};   // [-] - the hairline around a numeric entry
    ThemeColour  ValueText           = {};   // [-] - the numeric readout itself
    ThemeColour  SliderTrack         = {};   // [-] - the untravelled portion of a slider
    ThemeColour  SliderFill          = {};   // [-] - the travelled portion
    ThemeColour  SliderKnob          = {};   // [-] - the grab
    ThemeColour  KnobText            = {};   // [-] - a label drawn over the grab
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE LAYOUT EXTENTS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every spacing, height and radius the interface lays out with, in interface pixels.
/// note  📝 Held apart from the palette so a density profile is one value of this struct and re-tinting is one value
///        of the other. Changing the density must not require an artist to re-author a colour.
/// note  ⚠️ These are interface pixels and not device pixels. `14` §8's high-density question is open, and the scale
///        that answers it is folded in once by `ResolveActiveTheme` rather than multiplied at each read site.
/// tag   owning
struct LayoutExtents
{
    float  PanelPadding         =   8.0f;   // [px] - inner margin of a panel body
    float  ControlSpacing       =   6.0f;   // [px] - vertical gap between stacked controls
    float  CardGap              =   6.0f;   // [px] - gap between two stacked cards
    float  ControlHeight        =  22.0f;   // [px] - one single-line control
    float  RowHeight            =  32.0f;   // [px] - one outliner, tree or mask row
    float  LayerRowHeight       =  44.0f;   // [px] - one layer stack row, both halves of the split
    float  RevisionCardHeight   =  44.0f;   // [px] - one revision card, before its fold opens
    float  PanelHeaderHeight    =  46.0f;   // [px] - the header band, universal across every panel
    float  PanelFooterHeight    =  26.0f;   // [px] - the footer band carrying counts
    float  SectionHeaderHeight  =  29.0f;   // [px] - one accordion section header
    float  ViewportBandTop      =  46.0f;   // [px] - the band above the tab strip, across the whole display
    float  ViewportBandBottom   =  26.0f;   // [px] - the band below the desk, across the whole display
    float  TabStripHeight       =  31.0f;   // [px] - the trapezoid tab strip
    float  TabSlant             =  10.0f;   // [px] - horizontal run of a trapezoid's sloped edge
    float  TabInset             =   6.0f;   // [px] - inset from a tab's edge to its caption
    float  TabUnderline         =   2.0f;   // [px] - the active tab's underline thickness
    float  GutterThickness      =   4.0f;   // [px] - the draggable band between two halves of a division
    float  GlyphButtonEdge      =  26.0f;   // [px] - the square a header glyph button occupies
    float  OverlayRowHeight     =  26.0f;   // [px] - one row of a hand-rolled foreground overlay
    float  IndentWidth          =  15.0f;   // [px] - per-depth indent in a tree presentation
    float  CornerRounding       =  12.0f;   // [px] - panel and control corner radius
    float  BorderThickness      =   1.0f;   // [px] - panel outline and separator thickness
    float  LabelColumnWidth     =  88.0f;   // [px] - the label column of a control row
    float  ValueColumnWidth     =  92.0f;   // [px] - the value column beside it
    float  LabelColumnGap       =  10.0f;   // [px] - between the label column and the field beside it
    float  LabelColumnRatio     =   0.40f;  // [-]  - label fraction where the row is too narrow for the pair
    float  EntryRowHeight       =  30.0f;   // [px] - one numeric entry, slider or dropdown row
    float  EntryRounding        = 999.0f;   // [px] - numeric entry radius; beyond half the height is fully rounded
    float  SideSegmentWidth     =  30.0f;   // [px] - one axis or unit segment inside a numeric entry
    float  AxisSegmentWidth     =  24.0f;   // [px] - the axis cap of a vector component, narrower than a unit cap
    float  NumericEntryWidth    =  78.0f;   // [px] - the fixed value box beside a slider
    float  SliderTrackHeight    =  19.0f;   // [px] - the rounded track a slider knob travels
    float  SliderKnobEdge       =  21.0f;   // [px] - the knob's diameter, deliberately over the track's height
    float  SwitchWidth          =  50.0f;   // [px] - a boolean entry's travel
    float  SwitchHeight         =  32.0f;   // [px]
    float  SwitchNubEdge        =  24.0f;   // [px] - the nub, inset four each side
    float  PillRounding         =   9.0f;   // [px] - a selection pill's corner, squarer than a panel's
    float  SegmentRowHeight     =  26.0f;   // [px] - one segment row pill
    float  DropdownHeight       =  26.0f;   // [px] - the closed head of a dropdown
    float  DropdownCaretWidth   =  26.0f;   // [px] - the caret cap at its right
    float  ColourCircleEdge     =  18.0f;   // [px] - the swatch disc inside a colour bar
    float  GlyphButtonSmallEdge =  20.0f;   // [px] - the square a row-level glyph button occupies
    float  NumericFontScale     =   1.15f;  // [-]  - enlargement of the numeric readout
    float  SegmentFontScale     =   0.95f;  // [-]  - scale of the axis and unit segment glyphs
    float  GlyphEdge            =  14.0f;   // [px] - the square a row glyph is drawn into
    float  CarouselTravel       =   0.30f;  // [s]  - the slide a content carousel takes to cross one pane
};

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE THEME
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The palette and the extents together — what every present call is handed by const reference.
/// note  🔴 `14` §4.1 places this beside the document. No transaction records it and undo never steps through it.
/// tag   owning
struct ThemeSpecification
{
    ThemePalette   Palette  = {};   // [-] - the active colours
    LayoutExtents  Extents  = {};   // [-] - the active spacing, already scaled
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE RESOLUTIONS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The built-in dark palette.
/// note  The one translation unit in which a colour literal is permitted. Every other spelling of a colour in
///        `SlateUI` reads a field of the value this returns.
/// cost  🚩
/// tag   api, nonthrowing
ThemePalette DeclaredDarkPalette();

/// 🧩 The built-in layout extents, before any density scale is folded in.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
LayoutExtents DeclaredDefaultExtents();

/// 🧩 Assembles the active theme and folds the declared density scale into the extents that carry pixels.
/// in    DeclaredScale  [-]  the density multiplier; refused at zero or below rather than silently corrected
/// out   Outcome        [-]  refuses with ContentUnsupported for a scale at or below zero
/// note  🔴 The scale is folded in **here**, once, so every downstream read is already a final pixel extent. A scale
///        multiplied at each read site is a scale one site forgets, and the artist sees one control that ignores it.
/// note  📝 Ratios and font scales are dimensionless and are not folded — scaling `LabelColumnRatio` past one would
///        give a label the whole row and leave the control nowhere to sit.
/// cost  🚩
/// tag   api, nonthrowing
Outcome<ThemeSpecification> ResolveActiveTheme(float DeclaredScale);

/// 🧩 Mirrors the theme into the interface library's own style, so a control drawn without the theme still matches.
/// in    Theme    [-]  the resolved theme
/// out   Outcome  [-]  refuses with HostDenied when no interface context is current
/// pre   InterfaceExchange::Construct delivered
/// note  ⚠️ A convenience and never the route. A component that reads the style back instead of reading the theme
///        has made the vendor's global state the source of truth, which is the erosion `14` opens by describing.
/// cost  🚩
/// tag   api, nonthrowing
Outcome<bool> Enforce(const ThemeSpecification& Theme);

}   // namespace Slate
