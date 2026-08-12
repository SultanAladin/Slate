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
    ThemeColour  AccentPrimary       = {};   // [-] - selection, active tab, slider grab
    ThemeColour  AccentSubtle        = {};   // [-] - the accent at hover coverage
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
    float  PanelPadding      =  8.0f;    // [px] - inner margin of a panel body
    float  ControlSpacing    =  6.0f;    // [px] - vertical gap between stacked controls
    float  ControlHeight     = 22.0f;    // [px] - one single-line control
    float  RowHeight         = 20.0f;    // [px] - one outliner or list row
    float  IndentWidth       = 14.0f;    // [px] - per-depth indent in a tree presentation
    float  CornerRounding    =  4.0f;    // [px] - panel and control corner radius
    float  BorderThickness   =  1.0f;    // [px] - panel outline and separator thickness
    float  LabelColumnRatio  =  0.40f;   // [-]  - fraction of a control row given to its label
    float  EntryRowHeight    = 30.0f;    // [px] - one numeric entry, slider or dropdown row
    float  EntryRounding     = 999.0f;   // [px] - numeric entry radius; beyond half the height is fully rounded
    float  SideSegmentWidth  = 30.0f;    // [px] - one axis or unit segment inside a numeric entry
    float  NumericFontScale  =  1.15f;   // [-]  - enlargement of the numeric readout
    float  SegmentFontScale  =  0.95f;   // [-]  - scale of the axis and unit segment glyphs
    float  GlyphEdge         = 18.0f;    // [px] - the square a row glyph is drawn into
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
