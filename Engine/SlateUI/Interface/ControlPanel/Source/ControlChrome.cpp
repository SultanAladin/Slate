//============================================================================================================================================
//                                                            CONTROLCHROME.CPP
//============================================================================================================================================
// 🧩 The five remaining primitives — a colour bar and its coordinate tracks, a menu pill, a glyph square, an accordion header, and a slide.

#include "SlateUI/Interface/ControlPanel/Source/ControlInterior.h"

#include <cstdio>

namespace Slate
{

using namespace ControlInterior;

namespace
{

// 📝 A quarter turn. The twisty's chevron is authored pointing along the positive axis, so nought presents a closed
//    section and this presents an open one.
constexpr float QuarterTurn = 1.5707963268f;

/// 🧩 One coordinate printed as the eight-bit code an artist reads it as.
std::uint32_t Coded255(double Coordinate)
{
    const double Bounded255 = Bounded(Coordinate, 0.0, 1.0) * 255.0;

    return static_cast<std::uint32_t>(Bounded255 + 0.5);
}

}   // namespace


//------------------------------------------------------------------------------------------------------------------------
//                                                      THE COLOUR ENTRY
//------------------------------------------------------------------------------------------------------------------------

Outcome<ControlInteraction> PresentColourEntry(const ThemeSpecification&  Theme,
                                               const WorkspaceRectangle&  Area,
                                               const char*                Caption,
                                               ThemeColour&               Carried,
                                               bool&                      PickerOpen)
{
    const LayoutExtents&  Extents = Theme.Extents;
    const ThemePalette&   Palette = Theme.Palette;
    const ControlRowSplit Split   = ResolveControlRow(Theme, Area);

    if (Split.FieldArea.Width < Extents.ColourCircleEdge + Extents.DropdownCaretWidth + Extents.PanelPadding * 2.0f)
    {
        return Outcome<ControlInteraction>::Refuse(
            { RefusalReason::ExtentExhausted, "the field cannot carry a swatch and a caret" });
    }

    PresentControlLabel(Theme, Split.LabelArea, Caption);

    const PointerReading Pointer = ResolvePointer();

    WorkspaceRectangle Bar = Split.FieldArea;

    Bar.Height = Extents.EntryRowHeight;

    ControlInteraction Interaction = ResolvePress(Bar);

    if (Interaction.EditSealed)
        PickerOpen = !PickerOpen;

    // -- the bar -----------------------------------------------------------------------------------------------------
    const WorkspaceRectangle Cap = RightSlice(Bar, Extents.DropdownCaretWidth);

    PaintFill(Bar, Palette.ValueNumberSegment, Extents.EntryRounding);
    PaintFill(Cap, PointerCovers(Pointer, Bar) ? Palette.ControlHovered : Palette.ValueSideSegment,
              Extents.EntryRounding);

    // 📝 🔴 The swatch is painted with the carried colour itself and never with a converted one. `36` §1 and `14` §5
    //    together: the coordinate keeps its declared space across the whole edit, and `Quantize` inside `PaintDisc` is
    //    a quantisation to the draw surface's code — not a projection, and not a second transfer.
    const float SwatchRadius = Extents.ColourCircleEdge * 0.5f;
    const float SwatchX      = Bar.PositionX + Extents.PanelPadding * 1.5f + SwatchRadius;
    const float SwatchY      = Bar.PositionY + Bar.Height * 0.5f;

    PaintDisc(SwatchX, SwatchY, SwatchRadius, Carried);

    char Readout[ReadoutExtent] = {};

    // 📝 The three codes and the coverage, which is the reference's `r, g, b` followed by a dimmed alpha. Printed and
    //    never written back: `PrintReading`'s own note applies — the readout presents the reading and is not a second
    //    authority over it, so an artist who never touches this control loses no precision to what it shows.
    char Coverage[ReadoutExtent] = {};

    PrintReading(Coverage, ReadoutExtent, Carried.Coverage, 2u);

    std::snprintf(Readout, static_cast<std::size_t>(ReadoutExtent), "%u, %u, %u  %s",
                  Coded255(Carried.Coordinate.RedCoordinate),
                  Coded255(Carried.Coordinate.GreenCoordinate),
                  Coded255(Carried.Coordinate.BlueCoordinate),
                  Coverage);

    WorkspaceRectangle Printed = Bar;

    Printed.PositionX += Extents.PanelPadding * 1.5f + Extents.ColourCircleEdge + Extents.PanelPadding;
    Printed.Width     -= Extents.PanelPadding * 2.5f + Extents.ColourCircleEdge + Extents.DropdownCaretWidth;

    PaintCaption(Printed, Readout, Palette.ValueText, 0.0f, 0.5f, Extents.SegmentFontScale);

    PresentControlStroke(SquareIn(Cap, Extents.GlyphEdge * 0.8f), ControlStroke::Caret, Palette.TextMuted,
                         Extents.BorderThickness * 1.6f, PickerOpen ? 3.14159265f : 0.0f);

    if (!PickerOpen)
        return Outcome<ControlInteraction>::Deliver(Interaction);

    // -- the four coordinate tracks ----------------------------------------------------------------------------------
    // 📝 🔴 Four tracks in the colour's own space, and deliberately **not** the reference's saturation/value box and
    //    hue bar. A hue edit is a round trip through a second model, and this control is forbidden from spelling a
    //    transfer at all — `14` §5 places the interface after the display projection, so a conversion here would be
    //    the second transfer the whole arrangement exists to prevent. Four tracks amend the coordinate in place.
    double* const Coordinates[4] =
    {
        &Carried.Coordinate.RedCoordinate,
        &Carried.Coordinate.GreenCoordinate,
        &Carried.Coordinate.BlueCoordinate,
        &Carried.Coverage
    };

    static const char* const CoordinateCaptions[4] = { "R", "G", "B", "A" };

    WorkspaceRectangle Picker = Split.FieldArea;

    Picker.PositionY = Bar.PositionY + Bar.Height + Extents.ControlSpacing;
    Picker.Height    = Extents.SwitchHeight * 4.0f + Extents.ControlSpacing * 3.0f
                     + Extents.PanelPadding * 2.0f;

    PaintFill(Picker, Palette.ControlBackground, Extents.CornerRounding);

    for (std::uint32_t Ordinal = 0u; Ordinal < 4u; ++Ordinal)
    {
        WorkspaceRectangle TrackRow;

        TrackRow.PositionX = Picker.PositionX + Extents.PanelPadding;
        TrackRow.PositionY = Picker.PositionY + Extents.PanelPadding
                           + static_cast<float>(Ordinal) * (Extents.SwitchHeight + Extents.ControlSpacing);
        TrackRow.Width     = Picker.Width - Extents.PanelPadding * 2.0f;
        TrackRow.Height    = Extents.SwitchHeight;

        const WorkspaceRectangle Letter = LeftSlice(TrackRow, Extents.AxisSegmentWidth);

        PaintCaption(Letter, CoordinateCaptions[Ordinal], Palette.TextMuted, 0.5f, 0.5f, Extents.SegmentFontScale);

        WorkspaceRectangle TrackArea = TrackRow;

        TrackArea.PositionX += Extents.AxisSegmentWidth;
        TrackArea.Width     -= Extents.AxisSegmentWidth;

        // 📝 🔴 Each track claims the active identity, because a coordinate drag must survive the pointer leaving the
        //    track it started on. `ResolveTrack` is the same hold the numeric entries use, keyed by the address of the
        //    coordinate itself — which the caller owns and which is therefore stable across ticks.
        const TrackHold Held = ResolveTrack(TrackArea, static_cast<const void*>(Coordinates[Ordinal]));

        if (Held.Interaction.PointerOver) Interaction.PointerOver = true;
        if (Held.Interaction.EditOpened)  Interaction.EditOpened  = true;
        if (Held.Interaction.EditSealed)  Interaction.EditSealed  = true;

        if (Held.HoldOpen)
        {
            const double Proposed = static_cast<double>(Held.Fraction);

            if (Proposed != *Coordinates[Ordinal])
            {
                *Coordinates[Ordinal]    = Proposed;
                Interaction.EditDeclared = true;
            }
        }

        PaintTrack(Theme, TrackArea, static_cast<float>(Bounded(*Coordinates[Ordinal], 0.0, 1.0)), true, Held.HoldOpen);
    }

    return Outcome<ControlInteraction>::Deliver(Interaction);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                       THE MENU PILL
//------------------------------------------------------------------------------------------------------------------------

Outcome<ControlInteraction> PresentMenuPill(const ThemeSpecification&  Theme,
                                            const WorkspaceRectangle&  Area,
                                            const char*                Caption,
                                            bool                       Highlighted)
{
    const LayoutExtents& Extents = Theme.Extents;
    const ThemePalette&  Palette = Theme.Palette;
    const PointerReading Pointer = ResolvePointer();

    const WorkspaceRectangle Pill = CentredBand(Area, Extents.SegmentRowHeight);

    const ControlInteraction Interaction = ResolvePress(Pill);
    const bool               Covered     = PointerCovers(Pointer, Pill);

    if (Highlighted || Covered)
        PaintFill(Pill, Palette.AccentSubtle, Extents.PillRounding);

    PaintCaption(Pill, Caption, Highlighted || Covered ? Palette.TextPrimary : Palette.TextMuted,
                 0.5f, 0.5f, 1.0f);

    return Outcome<ControlInteraction>::Deliver(Interaction);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                      THE GLYPH BUTTON
//------------------------------------------------------------------------------------------------------------------------

Outcome<ControlInteraction> PresentGlyphButton(const ThemeSpecification&  Theme,
                                               const WorkspaceRectangle&  Area,
                                               ControlStroke              Stroke,
                                               std::uint64_t              DepotSlot,
                                               bool                       Highlighted)
{
    const LayoutExtents& Extents = Theme.Extents;
    const ThemePalette&  Palette = Theme.Palette;
    const PointerReading Pointer = ResolvePointer();

    if (Area.Width <= 0.0f || Area.Height <= 0.0f)
        return Outcome<ControlInteraction>::Refuse({ RefusalReason::ExtentExhausted, "the button has no area" });

    // 📝 The square is taken from the rectangle the caller named rather than from an extent, so one entry point serves
    //    both the twenty pixel row button and the twenty-six pixel header button without a second signature.
    const float              Shorter = Area.Width < Area.Height ? Area.Width : Area.Height;
    const WorkspaceRectangle Square  = SquareIn(Area, Shorter);

    const ControlInteraction Interaction = ResolvePress(Square);
    const bool               Covered     = PointerCovers(Pointer, Square);

    if (Highlighted)
        PaintFill(Square, Palette.AccentSubtle, Extents.PillRounding * 0.7f);
    else if (Covered)
        PaintFill(Square, Palette.ControlHovered, Extents.PillRounding * 0.7f);

    const WorkspaceRectangle Interior = Inset(Square, Extents.PanelPadding * 0.5f);

    if (DepotSlot != 0u)
    {
        // 📝 🔴 The slot is the depot's own integer and is handed straight back to the vendor. `ImTextureRef` is
        //    constructible from a texture identity and a texture identity is this integer widened, so the conversion
        //    is the vendor's own — no vendor spelling reaches `ControlPanel.h`, which is what `14` §7 requires.
        Recording()->AddImage(static_cast<ImTextureID>(DepotSlot), Corner(Interior), Opposite(Interior));
    }
    else
    {
        // 📝 🔴 The fallback is not a convenience. A depot that reclaimed a tier mid-session must not leave a panel
        //    painting nothing at all: a missing icon that still answers a press is recoverable, an invisible one is
        //    not, and the artist reports the second as "the button stopped working".
        PresentControlStroke(Interior, Stroke, Covered || Highlighted ? Palette.TextPrimary : Palette.TextMuted,
                             Extents.BorderThickness * 1.5f, 0.0f);
    }

    return Outcome<ControlInteraction>::Deliver(Interaction);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SECTION HEADER
//------------------------------------------------------------------------------------------------------------------------

Outcome<ControlInteraction> PresentSectionHeader(const ThemeSpecification&  Theme,
                                                 const WorkspaceRectangle&  Area,
                                                 const char*                Caption,
                                                 bool&                      SectionOpen,
                                                 const char*                Trailing)
{
    const LayoutExtents& Extents = Theme.Extents;
    const ThemePalette&  Palette = Theme.Palette;
    const PointerReading Pointer = ResolvePointer();

    const WorkspaceRectangle Band = CentredBand(Area, Extents.SectionHeaderHeight);

    ControlInteraction Interaction = ResolvePress(Band);

    if (Interaction.EditSealed)
    {
        SectionOpen              = !SectionOpen;
        Interaction.EditDeclared = true;
    }

    const bool Covered = PointerCovers(Pointer, Band);

    PaintFill(Band, Covered ? Palette.TileHovered : Palette.PanelHeader, Extents.PillRounding * 0.7f);

    WorkspaceRectangle Hairline;

    Hairline.PositionX = Band.PositionX;
    Hairline.PositionY = Band.PositionY + Band.Height - Extents.BorderThickness;
    Hairline.Width     = Band.Width;
    Hairline.Height    = Extents.BorderThickness;

    PaintFill(Hairline, Palette.PanelBorder, 0.0f);

    // 📝 The twisty turns a quarter and is not a second glyph. One rotated chevron is what the reference animates, and
    //    two authored arrows would be two shapes to keep matching.
    WorkspaceRectangle Twisty = Band;

    Twisty.PositionX += Extents.PanelPadding * 0.75f;
    Twisty.Width      = Extents.GlyphEdge;

    PresentControlStroke(SquareIn(Twisty, Extents.GlyphEdge * 0.7f), ControlStroke::Twisty, Palette.TextMuted,
                         Extents.BorderThickness * 1.6f, SectionOpen ? QuarterTurn : 0.0f);

    WorkspaceRectangle Lettering = Band;

    Lettering.PositionX += Extents.PanelPadding * 0.75f + Extents.GlyphEdge + Extents.PanelPadding * 0.5f;
    Lettering.Width     -= Extents.PanelPadding * 1.25f + Extents.GlyphEdge + Extents.PanelPadding * 0.5f;

    PaintCaption(Lettering, Caption, Palette.TextPrimary, 0.0f, 0.5f, 1.0f);

    if (Trailing != nullptr && Trailing[0] != '\0')
    {
        WorkspaceRectangle Counted = Lettering;

        Counted.Width -= Extents.PanelPadding * 0.5f;

        PaintCaption(Counted, Trailing, Palette.TextMuted, 1.0f, 0.5f, Extents.SegmentFontScale);
    }

    return Outcome<ControlInteraction>::Deliver(Interaction);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                     THE CONTENT CAROUSEL
//------------------------------------------------------------------------------------------------------------------------

Outcome<float> AdvanceContentCarousel(const ThemeSpecification&  Theme,
                                      CarouselCarry&             Carry,
                                      float                      ElapsedInterval)
{
    const float Travel = Theme.Extents.CarouselTravel;

    if (!(Travel > 0.0f))
    {
        return Outcome<float>::Refuse(
            { RefusalReason::ContentUnsupported, "the theme declares no carousel travel to slide across" });
    }

    if (ElapsedInterval < 0.0f)
        return Outcome<float>::Refuse({ RefusalReason::ContentUnsupported, "the elapsed interval ran backwards" });

    // 📝 At rest the two panes agree and there is nothing to advance. A caller that has not asked for a new pane pays
    //    one comparison a tick for the whole mechanism.
    if (Carry.PresentedPane == Carry.ArrivingPane)
    {
        Carry.Travelled = 1.0f;
        return Outcome<float>::Deliver(0.0f);
    }

    if (Carry.Travelled >= 1.0f)
        Carry.Travelled = 0.0f;

    Carry.Travelled += ElapsedInterval / Travel;

    if (Carry.Travelled >= 1.0f)
    {
        Carry.Travelled     = 1.0f;
        Carry.PresentedPane = Carry.ArrivingPane;

        return Outcome<float>::Deliver(0.0f);
    }

    // 📝 The reference's `cubic-bezier(.5,.05,.2,1)` as a cubic ease-out, which is the same settle to within a pixel
    //    over three hundred milliseconds and costs one multiply rather than a solve per tick.
    const float Remaining = 1.0f - Carry.Travelled;
    const float Eased     = 1.0f - Remaining * Remaining * Remaining;

    // 📝 🔴 Returned as a **fraction of one pane** and not as a pixel offset. The header places the clipping with the
    //    caller, and a pane's width is the caller's body — which this control is never handed. The caller multiplies
    //    by its own body width. Negative where the arriving pane sits after the presented one, positive where it sits
    //    before, so the two panes are painted at the offset and at the offset plus or minus one width.
    const float Direction = Carry.ArrivingPane > Carry.PresentedPane ? -1.0f : 1.0f;

    return Outcome<float>::Deliver(Direction * (1.0f - Eased));
}

}   // namespace Slate
