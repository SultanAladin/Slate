//============================================================================================================================================
//                                                             ENTRYPANEL.CPP
//============================================================================================================================================
// 🧩 Six accordion sections over the whole of `ControlPanel` — measured once, laid out once, and every refusal counted rather than raised.

#include "SlateUI/Interface/EntryPanel/Api/EntryPanel.h"

#include <cstdio>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SECTION TABLE
//------------------------------------------------------------------------------------------------------------------------

const char* CaptionOf(EntrySection Declared)
{
    switch (Declared)
    {
        case EntrySection::Numeric:  return "Numeric entries";
        case EntrySection::Choice:   return "Choice entries";
        case EntrySection::Text:     return "Text entries";
        case EntrySection::Colour:   return "Colour";
        case EntrySection::Chrome:   return "Chrome";
        case EntrySection::Carousel: return "Carousel";
        default:                     return "Section";
    }
}

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE OFFERED RUNS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Spelled once each. A run rebuilt at its call site is a run the measuring pass and the presenting pass can
//    disagree about, and the disagreement presents as a section whose last control is unreachable.
const char* const OfferedBlends[EntryBlendCount]      = { "Normal", "Multiply", "Screen", "Overlay" };
const char* const OfferedChannels[EntryChannelCount]  = { "R", "G", "B", "A" };
const char* const OfferedSampling[EntrySamplingCount] = { "Nearest", "Linear", "Cubic", "Lanczos" };
const char* const OfferedPills[EntryPillCount]        = { "File", "Edit", "Layer", "View" };
const char* const OfferedPanes[EntryPaneCount]        = { "Properties", "History" };

// 📝 The chrome strokes the glyph band offers, and the one place their count is spelled. `ControlStroke` carries
//    sixteen and a band presenting all of them at `GlyphButtonEdge` needs 416 px — wider than any docked column —
//    so the band offers the six a panel header actually uses.
constexpr std::uint32_t OfferedGlyphCount = 6u;

const ControlStroke OfferedGlyphs[OfferedGlyphCount] =
{
    ControlStroke::Plus,
    ControlStroke::Trash,
    ControlStroke::Eye,
    ControlStroke::Cog,
    ControlStroke::Reload,
    ControlStroke::Search
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE LOCAL EXTENTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 What the theme does not name, because nothing outside this panel has an opinion on it. Every pitch below is
//    built from `LayoutExtents` and never from a literal of its own — a panel spelling `ControlSpacing` for itself
//    is the drift `ThemeSpecification` exists to prevent.
constexpr float CarouselBodyHeight = 84.0f;   // [px] - the two-pane body the slide crosses
constexpr float SectionInset       = 4.0f;    // [px] - the accordion body's inset from the panel's own padding

float RowPitch(const LayoutExtents& Extents)      { return Extents.EntryRowHeight   + Extents.ControlSpacing; }
float SwitchPitch(const LayoutExtents& Extents)   { return Extents.SwitchHeight     + Extents.ControlSpacing; }
float SegmentPitch(const LayoutExtents& Extents)  { return Extents.SegmentRowHeight + Extents.ControlSpacing; }
float DropdownPitch(const LayoutExtents& Extents) { return Extents.DropdownHeight   + Extents.ControlSpacing; }
float GlyphPitch(const LayoutExtents& Extents)    { return Extents.GlyphButtonEdge  + Extents.ControlSpacing; }

// 📝 🔴 The exclusive pill run wraps, so it is allotted two rows and not one. `PresentSelectionEntry` lays its
//    pills from the field's top edge and wraps downward without asking how much room it has — an allotment of one
//    row leaves a wrapped second row painted over whatever the accordion stacked beneath it.
float SelectionPitch(const LayoutExtents& Extents)
{
    return Extents.SegmentRowHeight * 2.0f + Extents.ControlSpacing * 2.0f;
}

// 📝 🔴 The picker's own extent, spelled exactly as `PresentColourEntry` builds it. A colour entry drops four
//    coordinate tracks **below** the rectangle it was handed, so a section that did not reserve this would have
//    the tracks painted over the section beneath — and pressing one would drag a track the artist cannot see.
float PickerExtent(const LayoutExtents& Extents)
{
    return Extents.SwitchHeight * 4.0f + Extents.ControlSpacing * 3.0f + Extents.PanelPadding * 2.0f;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE LOCAL GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

WorkspaceRectangle RowAt(const WorkspaceRectangle& Body, float PositionY, float Height)
{
    WorkspaceRectangle Row;

    Row.PositionX = Body.PositionX;
    Row.PositionY = PositionY;
    Row.Width     = Body.Width;
    Row.Height    = Height > 0.0f ? Height : 0.0f;

    return Row;
}

WorkspaceRectangle InsetBy(const WorkspaceRectangle& Area, float Horizontal)
{
    WorkspaceRectangle Narrowed = Area;

    Narrowed.PositionX += Horizontal;
    Narrowed.Width     -= Horizontal * 2.0f;

    if (Narrowed.Width < 0.0f)
        Narrowed.Width = 0.0f;

    return Narrowed;
}

WorkspaceRectangle RightOf(const WorkspaceRectangle& Area, float Width)
{
    const float Taken = Width < Area.Width ? Width : Area.Width;

    WorkspaceRectangle Trailing = Area;

    Trailing.PositionX = Area.PositionX + Area.Width - (Taken > 0.0f ? Taken : 0.0f);
    Trailing.Width     = Taken > 0.0f ? Taken : 0.0f;

    return Trailing;
}

WorkspaceRectangle CentredWithin(const WorkspaceRectangle& Area, float Width, float Height)
{
    WorkspaceRectangle Centred;

    Centred.Width     = Width  < Area.Width  ? Width  : Area.Width;
    Centred.Height    = Height < Area.Height ? Height : Area.Height;
    Centred.PositionX = Area.PositionX + (Area.Width  - Centred.Width)  * 0.5f;
    Centred.PositionY = Area.PositionY + (Area.Height - Centred.Height) * 0.5f;

    return Centred;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE WALK
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What one section's walk carries — where it is stacking, and what refused along the way.
/// note  🔴 The refusal count is accumulated here and written back once, after the walk. A count written into the
///        specification mid-walk is a count the footer reads while it is still growing, and the footer then prints
///        a different number every tick for a panel nobody is touching.
struct SectionWalk
{
    float          Travelled = 0.0f;   // [px] - the top of the next row
    std::uint32_t  Refused   = 0u;     // [-]  - controls that declined for want of extent
};

// 📝 A row is laid out whether or not it is visible, because the offset arithmetic is what places the row after
//    it. Only the painting is skipped, and only where the row falls wholly outside the clip.
bool RowVisible(const WorkspaceRectangle& Body, const WorkspaceRectangle& Row)
{
    return Row.PositionY + Row.Height >= Body.PositionY && Row.PositionY <= Body.PositionY + Body.Height;
}

void Counted(SectionWalk& Walk, const Outcome<ControlInteraction>& Reported)
{
    if (!Reported.ContentPresent)
        ++Walk.Refused;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE NUMERIC SECTION
//------------------------------------------------------------------------------------------------------------------------

void PresentNumericSection(const ThemeSpecification&  Theme,
                           const WorkspaceRectangle&  Body,
                           EntrySpecification&        Standing,
                           SectionWalk&               Walk)
{
    const LayoutExtents& Extents = Theme.Extents;
    const float          Pitch   = RowPitch(Extents);

    // -- the three bounded sliders -----------------------------------------------------------------------------------
    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.EntryRowHeight);

        Walk.Travelled += Pitch;

        if (RowVisible(Body, Row))
            Counted(Walk, PresentValueSlider(Theme, Row, "Coverage", Standing.Coverage, 0.0, 1.0, "\xC2\xB7", 2u));
    }

    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.EntryRowHeight);

        Walk.Travelled += Pitch;

        if (RowVisible(Body, Row))
            Counted(Walk, PresentValueSlider(Theme, Row, "Extent", Standing.StrokeExtent, 1.0, 512.0, "px", 0u));
    }

    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.EntryRowHeight);

        Walk.Travelled += Pitch;

        if (RowVisible(Body, Row))
            Counted(Walk, PresentValueSlider(Theme, Row, "Hardness", Standing.Hardness, 0.0, 1.0, "\xC2\xB7", 2u));
    }

    // -- the two unbounded scalars -----------------------------------------------------------------------------------
    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.EntryRowHeight);

        Walk.Travelled += Pitch;

        if (RowVisible(Body, Row))
            Counted(Walk, PresentScalarEntry(Theme, Row, "Offset", Standing.Displacement, 0.25, "px", 2u));
    }

    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.EntryRowHeight);

        Walk.Travelled += Pitch;

        if (RowVisible(Body, Row))
            Counted(Walk, PresentScalarEntry(Theme, Row, "Turn", Standing.Turn, 0.5, "\xC2\xB0", 1u));
    }

    // -- the three components ----------------------------------------------------------------------------------------
    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.EntryRowHeight);

        Walk.Travelled += Pitch;

        if (RowVisible(Body, Row))
            Counted(Walk, PresentVectorEntry(Theme, Row, "Placement", Standing.Placement, 0.25, 2u));
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE CHOICE SECTION
//------------------------------------------------------------------------------------------------------------------------

void PresentChoiceSection(const ThemeSpecification&  Theme,
                          const WorkspaceRectangle&  Body,
                          EntrySpecification&        Standing,
                          SectionWalk&               Walk)
{
    const LayoutExtents& Extents = Theme.Extents;

    // -- the crossing nub ---------------------------------------------------------------------------------------------
    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.SwitchHeight);

        Walk.Travelled += SwitchPitch(Extents);

        if (RowVisible(Body, Row))
            Counted(Walk, PresentBooleanEntry(Theme, Row, "Smoothing", Standing.SmoothingEnabled));
    }

    // -- the exclusive pill run ---------------------------------------------------------------------------------------
    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, SelectionPitch(Extents));

        Walk.Travelled += SelectionPitch(Extents);

        if (RowVisible(Body, Row))
        {
            Counted(Walk, PresentSelectionEntry(Theme, Row, "Blend", OfferedBlends, EntryBlendCount,
                                                Standing.ChosenBlend));
        }
    }

    // -- the independent switches -------------------------------------------------------------------------------------
    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.SegmentRowHeight);

        Walk.Travelled += SegmentPitch(Extents);

        // 📝 A segment row and a selection entry sit one above the other on purpose. They are the two controls an
        //    artist most often mistakes for each other, and the pair reads as one exclusive choice above four
        //    independent switches only when both are presented together.
        if (RowVisible(Body, Row))
        {
            Counted(Walk, PresentSegmentRow(Theme, Row, OfferedChannels, Standing.ChannelDeclared,
                                            EntryChannelCount));
        }
    }

    // -- the dropped list ---------------------------------------------------------------------------------------------
    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.DropdownHeight);

        Walk.Travelled += DropdownPitch(Extents);

        // 📝 ⚠️ The dropped list is clipped to the accordion's body, because the body is what the walk declared a
        //    clip over. A list longer than the room beneath its head is therefore cut rather than overhanging the
        //    panel — the remedy `ControlChoice.cpp` names is a deferred overlay record, and inventing a second
        //    recording here would be the seam `14` §7 holds breaking at a panel.
        if (RowVisible(Body, Row))
        {
            Counted(Walk, PresentDropdown(Theme, Row, OfferedSampling, EntrySamplingCount, Standing.ChosenSampling,
                                          Standing.SamplingCarry, Standing.PresentedTicks));
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE TEXT SECTION
//------------------------------------------------------------------------------------------------------------------------

void PresentTextSection(const ThemeSpecification&  Theme,
                        const WorkspaceRectangle&  Body,
                        EntrySpecification&        Standing,
                        SectionWalk&               Walk)
{
    const LayoutExtents& Extents = Theme.Extents;
    const float          Pitch   = RowPitch(Extents);

    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.EntryRowHeight);

        Walk.Travelled += Pitch;

        if (RowVisible(Body, Row))
            Counted(Walk, PresentTextEntry(Theme, Row, "Name", Standing.Named, "untitled"));
    }

    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.EntryRowHeight);

        Walk.Travelled += Pitch;

        if (RowVisible(Body, Row))
        {
            // 📝 🔴 The browse declaration is read and released here and never carried to the next tick. `04`'s
            //    interchange owns the file system, and a panel that retained the intent would be a panel with a
            //    pending file operation nothing ever collects — which presents as a chooser opening a tick late.
            Counted(Walk, PresentPathEntry(Theme, Row, "Location", Standing.Located, Standing.BrowseDeclared));
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE COLOUR SECTION
//------------------------------------------------------------------------------------------------------------------------

void PresentColourSection(const ThemeSpecification&  Theme,
                          const WorkspaceRectangle&  Body,
                          EntrySpecification&        Standing,
                          SectionWalk&               Walk)
{
    const LayoutExtents& Extents = Theme.Extents;

    const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.EntryRowHeight);

    Walk.Travelled += RowPitch(Extents);

    if (Standing.PickerOpen)
        Walk.Travelled += PickerExtent(Extents) + Extents.ControlSpacing;

    // 📝 The whole allotment is tested for visibility and not the bar alone, so an open picker whose bar has
    //    scrolled off the top still presents the tracks the artist is dragging.
    WorkspaceRectangle Allotted = Row;

    if (Standing.PickerOpen)
        Allotted.Height += PickerExtent(Extents) + Extents.ControlSpacing;

    if (!RowVisible(Body, Allotted))
        return;

    Counted(Walk, PresentColourEntry(Theme, Row, "Tint", Standing.Tint, Standing.PickerOpen));
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE CHROME SECTION
//------------------------------------------------------------------------------------------------------------------------

void PresentChromeSection(const ThemeSpecification&  Theme,
                          const WorkspaceRectangle&  Body,
                          EntrySpecification&        Standing,
                          SectionWalk&               Walk)
{
    const LayoutExtents& Extents = Theme.Extents;

    // -- the menu band ------------------------------------------------------------------------------------------------
    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.SegmentRowHeight);

        Walk.Travelled += SegmentPitch(Extents);

        if (RowVisible(Body, Row) && Row.Width > 1.0f)
        {
            const float Cell = Row.Width / static_cast<float>(EntryPillCount);

            for (std::uint32_t Ordinal = 0u; Ordinal < EntryPillCount; ++Ordinal)
            {
                WorkspaceRectangle Pill = Row;

                Pill.PositionX = Row.PositionX + static_cast<float>(Ordinal) * Cell;
                Pill.Width     = Cell;

                const Outcome<ControlInteraction> Pressed =
                    PresentMenuPill(Theme, Pill, OfferedPills[Ordinal], Standing.ChosenPill == Ordinal);

                Counted(Walk, Pressed);

                // 📝 A menu pill highlights and opens nothing. What a real menu band drops is a
                //    `WorkspaceOverlayRecord`, which belongs to whichever host owns the band — a panel that
                //    dropped one here would be a second authority on what the menu contains.
                if (Pressed.ContentPresent && Pressed.Resolve().EditSealed)
                    Standing.ChosenPill = Ordinal;
            }
        }
    }

    // -- the glyph band -----------------------------------------------------------------------------------------------
    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.GlyphButtonEdge);

        Walk.Travelled += GlyphPitch(Extents);

        if (RowVisible(Body, Row) && Row.Width > 1.0f)
        {
            const float Stride = Extents.GlyphButtonEdge + Extents.ControlSpacing;

            for (std::uint32_t Ordinal = 0u; Ordinal < OfferedGlyphCount; ++Ordinal)
            {
                WorkspaceRectangle Square = Row;

                Square.PositionX = Row.PositionX + static_cast<float>(Ordinal) * Stride;
                Square.Width     = Extents.GlyphButtonEdge;

                if (Square.PositionX + Square.Width > Row.PositionX + Row.Width)
                    break;

                // 📝 🔴 Zero names no depot slot, so every button falls back to its stroke. `GlyphDepot` owns
                //    uploaded art and this panel names none — a slot invented here would be a panel deciding what
                //    a tier holds.
                const Outcome<ControlInteraction> Pressed =
                    PresentGlyphButton(Theme, Square, OfferedGlyphs[Ordinal], 0u, Standing.ChosenGlyph == Ordinal);

                Counted(Walk, Pressed);

                if (Pressed.ContentPresent && Pressed.Resolve().EditSealed)
                    Standing.ChosenGlyph = Ordinal;
            }
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE CAROUSEL SECTION
//------------------------------------------------------------------------------------------------------------------------

void PresentCarouselSection(const ThemeSpecification&  Theme,
                            const WorkspaceRectangle&  Body,
                            EntrySpecification&        Standing,
                            SectionWalk&               Walk)
{
    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    // -- the two pane pills -------------------------------------------------------------------------------------------
    {
        const WorkspaceRectangle Row = RowAt(Body, Walk.Travelled, Extents.SegmentRowHeight);

        Walk.Travelled += SegmentPitch(Extents);

        if (RowVisible(Body, Row) && Row.Width > 1.0f)
        {
            const float Cell = Row.Width / static_cast<float>(EntryPaneCount);

            for (std::uint32_t Ordinal = 0u; Ordinal < EntryPaneCount; ++Ordinal)
            {
                WorkspaceRectangle Pill = Row;

                Pill.PositionX = Row.PositionX + static_cast<float>(Ordinal) * Cell;
                Pill.Width     = Cell;

                const Outcome<ControlInteraction> Pressed =
                    PresentMenuPill(Theme, Pill, OfferedPanes[Ordinal], Standing.Slide.ArrivingPane == Ordinal);

                Counted(Walk, Pressed);

                // 📝 🔴 The press names the arriving pane and never the presented one. Writing both would place
                //    the slide at its destination on the tick it began, and the carousel would present as an
                //    instantaneous switch with an animation nobody sees.
                if (Pressed.ContentPresent && Pressed.Resolve().EditSealed)
                    Standing.Slide.ArrivingPane = Ordinal;
            }
        }
    }

    // -- the two panes ------------------------------------------------------------------------------------------------
    const WorkspaceRectangle Pane = RowAt(Body, Walk.Travelled, CarouselBodyHeight);

    Walk.Travelled += CarouselBodyHeight + Extents.ControlSpacing;

    const Outcome<float> Advanced = AdvanceContentCarousel(Theme, Standing.Slide, Standing.DeclaredInterval);

    if (!Advanced.ContentPresent)
    {
        ++Walk.Refused;

        return;
    }

    if (!RowVisible(Body, Pane) || Pane.Width <= 1.0f)
        return;

    PresentSurfaceFill(Pane, Palette.TileBackground, Extents.CornerRounding * 0.5f);

    DeclareClip(Pane);

    // 📝 🔴 The returned fraction settles to zero as the slide completes, so it is the **entering** pane's offset
    //    and the leaving pane sits one body width behind it, against the direction of travel. Read the other way
    //    round the two panes both start a full width off the body, and the section presents as an empty rectangle
    //    for the whole of the slide.
    const float Fraction  = Advanced.Resolve();
    const float Direction = Standing.Slide.ArrivingPane > Standing.Slide.PresentedPane ? -1.0f : 1.0f;

    WorkspaceRectangle Entering = Pane;
    Entering.PositionX = Pane.PositionX + Fraction * Pane.Width;

    WorkspaceRectangle Leaving = Pane;
    Leaving.PositionX = Pane.PositionX + (Fraction - Direction) * Pane.Width;

    const std::uint32_t ArrivingPane  = Standing.Slide.ArrivingPane  < EntryPaneCount
                                      ? Standing.Slide.ArrivingPane  : 0u;
    const std::uint32_t PresentedPane = Standing.Slide.PresentedPane < EntryPaneCount
                                      ? Standing.Slide.PresentedPane : 0u;

    if (PresentedPane != ArrivingPane)
        PresentTextRun(Leaving, OfferedPanes[PresentedPane], Palette.TextMuted, TextPlacement::Centred, 1.1f);

    PresentTextRun(Entering, OfferedPanes[ArrivingPane], Palette.TextPrimary, TextPlacement::Centred, 1.1f);

    ReclaimClip();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE HEADER BAND
//------------------------------------------------------------------------------------------------------------------------

void PresentHeaderBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       EntrySpecification&        Standing)
{
    if (Band.Height <= 1.0f)
        return;

    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    PresentSurfaceFill(Band, Palette.PanelHeader, 0.0f);

    WorkspaceRectangle Rule = Band;
    Rule.PositionY = Band.PositionY + Band.Height - Extents.BorderThickness;
    Rule.Height    = Extents.BorderThickness;

    PresentSurfaceFill(Rule, Palette.PanelBorder, 0.0f);

    WorkspaceRectangle Interior = Band;
    Interior.PositionX += Extents.PanelPadding;
    Interior.Width     -= Extents.PanelPadding * 2.0f;
    Interior.Height     = Band.Height - Extents.BorderThickness;

    PresentTextRun(Interior, "Control Centre", Palette.TextPrimary, TextPlacement::Leading, 1.0f);

    // -- the fold-all glyph, at the trailing edge ---------------------------------------------------------------------
    WorkspaceRectangle Trailing = RightOf(Interior, Extents.GlyphButtonEdge);
    Trailing = CentredWithin(Trailing, Extents.GlyphButtonEdge, Extents.GlyphButtonEdge);

    // 📝 One glyph and not two. Whether the press folds or unfolds is resolved from whether any section is open,
    //    so the button always does the thing the artist can see is available — a pair of buttons leaves one of
    //    them inert at every moment and neither says which.
    bool AnyOpen = false;

    for (std::uint32_t Ordinal = 0u; Ordinal < EntrySectionCount; ++Ordinal)
        AnyOpen = AnyOpen || Standing.SectionOpen[Ordinal];

    const Outcome<ControlInteraction> Folded =
        PresentGlyphButton(Theme, Trailing, ControlStroke::Twisty, 0u, AnyOpen);

    if (Folded.ContentPresent && Folded.Resolve().EditSealed)
    {
        for (std::uint32_t Ordinal = 0u; Ordinal < EntrySectionCount; ++Ordinal)
            Standing.SectionOpen[Ordinal] = !AnyOpen;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE FOOTER BAND
//------------------------------------------------------------------------------------------------------------------------

void PresentFooterBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       const EntrySpecification&  Standing,
                       std::uint32_t              OpenSections)
{
    if (Band.Height <= 1.0f)
        return;

    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    PresentSurfaceFill(Band, Palette.PanelHeader, 0.0f);

    WorkspaceRectangle Rule = Band;
    Rule.Height = Extents.BorderThickness;

    PresentSurfaceFill(Rule, Palette.PanelBorder, 0.0f);

    WorkspaceRectangle Interior = Band;
    Interior.PositionX += Extents.PanelPadding;
    Interior.Width     -= Extents.PanelPadding * 2.0f;

    char Said[96] = {};

    std::snprintf(Said, sizeof Said, "%u of %u open", OpenSections, EntrySectionCount);

    PresentTextRun(Interior, Said, Palette.TextMuted, TextPlacement::Leading, 0.85f);

    // 📝 🔴 The refusals are presented and never raised. A control refuses for want of width on every tick a panel
    //    is docked narrow, and appending that to `86`'s register would fill it from an ordinary layout — `14` §7
    //    puts the refusal on the desk, which is here.
    if (Standing.RefusedControls == 0u)
        return;

    char Refused[64] = {};

    std::snprintf(Refused, sizeof Refused, "%u too narrow", Standing.RefusedControls);

    PresentTextRun(Interior, Refused, Palette.DangerPrimary, TextPlacement::Trailing, 0.85f);
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE SECTION EXTENT
//------------------------------------------------------------------------------------------------------------------------

float SectionExtent(const LayoutExtents& Extents, const EntrySpecification& Standing, EntrySection Declared)
{
    switch (Declared)
    {
        case EntrySection::Numeric:
            return RowPitch(Extents) * 6.0f;

        case EntrySection::Choice:
            return SwitchPitch(Extents) + SelectionPitch(Extents) + SegmentPitch(Extents) + DropdownPitch(Extents);

        case EntrySection::Text:
            return RowPitch(Extents) * 2.0f;

        case EntrySection::Colour:
            return RowPitch(Extents)
                 + (Standing.PickerOpen ? PickerExtent(Extents) + Extents.ControlSpacing : 0.0f);

        case EntrySection::Chrome:
            return SegmentPitch(Extents) + GlyphPitch(Extents);

        case EntrySection::Carousel:
            return SegmentPitch(Extents) + CarouselBodyHeight + Extents.ControlSpacing;

        default:
            return 0.0f;
    }
}

float PresentedExtent(const LayoutExtents& Extents, const EntrySpecification& Standing)
{
    float Measured = Extents.PanelPadding * 2.0f;

    for (std::uint32_t Ordinal = 0u; Ordinal < EntrySectionCount; ++Ordinal)
    {
        Measured += Extents.SectionHeaderHeight + Extents.ControlSpacing;

        if (Standing.SectionOpen[Ordinal])
            Measured += SectionExtent(Extents, Standing, static_cast<EntrySection>(Ordinal));
    }

    return Measured;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

void PresentEntries(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, EntrySpecification& Standing)
{
    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    if (Area.Width <= 1.0f || Area.Height <= 1.0f)
        return;

    // 📝 🔴 Advanced once, here, at the top of the tick. The sampling dropdown compares against this count to tell
    //    the press that opened it from the press that dismisses it, and a count advanced twice in a tick leaves a
    //    list that closes on the tick it opened.
    ++Standing.PresentedTicks;

    // 📝 🔴 The tint is resolved from the palette on the first tick and is the artist's from then on. A tint
    //    re-read every tick would discard the edit the moment the pointer left the track, and the artist meets
    //    that as a colour control that will not hold a colour.
    if (!Standing.TintDeclared)
    {
        Standing.Tint         = Palette.AccentPrimary;
        Standing.TintDeclared = true;
    }

    PresentSurfaceFill(Area, Palette.PanelBackground, Extents.CornerRounding);

    // -- the two bands ------------------------------------------------------------------------------------------------
    WorkspaceRectangle Header = Area;
    Header.Height = Extents.PanelHeaderHeight;

    WorkspaceRectangle Footer = Area;
    Footer.PositionY = Area.PositionY + Area.Height - Extents.PanelFooterHeight;
    Footer.Height    = Extents.PanelFooterHeight;

    PresentHeaderBand(Theme, Header, Standing);

    WorkspaceRectangle Body;
    Body.PositionX = Area.PositionX;
    Body.PositionY = Area.PositionY + Extents.PanelHeaderHeight;
    Body.Width     = Area.Width;
    Body.Height    = Area.Height - Extents.PanelHeaderHeight - Extents.PanelFooterHeight;

    std::uint32_t OpenSections = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < EntrySectionCount; ++Ordinal)
        OpenSections += Standing.SectionOpen[Ordinal] ? 1u : 0u;

    if (Body.Height <= 0.0f)
    {
        PresentFooterBand(Theme, Footer, Standing, OpenSections);

        return;
    }

    DeclareClip(Body);

    // 📝 🔴 Measured before it is presented, and measured by the same call the desk could make for itself. The
    //    visible offset is bounded against the content, and a bound applied after the walk lags the wheel by a
    //    tick — which the artist meets as a list that scrolls one notch behind the pointer.
    AdvanceVisibleOffset(Standing.VisibleOffset, Body, PresentedExtent(Extents, Standing));

    SectionWalk Walk;

    Walk.Travelled = Body.PositionY + Extents.PanelPadding - Standing.VisibleOffset;

    const WorkspaceRectangle Interior = InsetBy(Body, Extents.PanelPadding);

    for (std::uint32_t Ordinal = 0u; Ordinal < EntrySectionCount; ++Ordinal)
    {
        const EntrySection Declared = static_cast<EntrySection>(Ordinal);

        WorkspaceRectangle HeaderRow = RowAt(Interior, Walk.Travelled, Extents.SectionHeaderHeight);

        Walk.Travelled += Extents.SectionHeaderHeight + Extents.ControlSpacing;

        if (RowVisible(Body, HeaderRow))
        {
            PresentSectionHeader(Theme, HeaderRow, CaptionOf(Declared), Standing.SectionOpen[Ordinal], nullptr);
        }

        if (!Standing.SectionOpen[Ordinal])
            continue;

        // 📝 The section body is inset a little further than the headers, so an open fold reads as belonging to
        //    the header above it rather than as a second run of unrelated rows.
        const WorkspaceRectangle SectionBody = InsetBy(Interior, SectionInset);

        switch (Declared)
        {
            case EntrySection::Numeric:  PresentNumericSection(Theme, SectionBody, Standing, Walk);  break;
            case EntrySection::Choice:   PresentChoiceSection(Theme, SectionBody, Standing, Walk);   break;
            case EntrySection::Text:     PresentTextSection(Theme, SectionBody, Standing, Walk);     break;
            case EntrySection::Colour:   PresentColourSection(Theme, SectionBody, Standing, Walk);   break;
            case EntrySection::Chrome:   PresentChromeSection(Theme, SectionBody, Standing, Walk);   break;
            case EntrySection::Carousel: PresentCarouselSection(Theme, SectionBody, Standing, Walk); break;
            default: break;
        }
    }

    if (OpenSections == 0u)
        PresentTextRun(Body, "every section is folded", Palette.TextMuted, TextPlacement::Centred, 1.0f);

    ReclaimClip();

    // 📝 🔴 Written back after the walk and never during it, exactly as the desk's own deferred intents are. A
    //    count amended mid-walk is a count the footer has already read, and the footer then prints a figure that
    //    is one section behind whatever refused.
    Standing.RefusedControls = Walk.Refused;

    PresentFooterBand(Theme, Footer, Standing, OpenSections);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PANEL ROUTINE
//------------------------------------------------------------------------------------------------------------------------

void PresentEntryPanel(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, void* PresentContext)
{
    EntrySpecification* Standing = static_cast<EntrySpecification*>(PresentContext);

    if (Standing == nullptr)
    {
        PresentSurfaceFill(Area, Theme.Palette.PanelBackground, Theme.Extents.CornerRounding);
        PresentTextRun(Area, "no controls", Theme.Palette.TextMuted, TextPlacement::Centred, 1.0f);

        return;
    }

    PresentEntries(Theme, Area, *Standing);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE LEDGER SLOT
//------------------------------------------------------------------------------------------------------------------------

PanelSlot ResolveEntrySlot(const char*          PanelIdentifier,
                           const char*          PanelTitle,
                           WorkspacePanelSide   DeclaredSide,
                           EntrySpecification&  Standing)
{
    PanelSlot Declaring;

    Declaring.PanelIdentifier = PanelIdentifier;
    Declaring.PanelTitle      = PanelTitle;
    Declaring.DeclaredSide    = DeclaredSide;
    Declaring.Present         = &PresentEntryPanel;
    Declaring.PresentContext  = &Standing;

    return Declaring;
}

}   // namespace Slate
