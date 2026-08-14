//============================================================================================================================================
//                                                             ASSETPANEL.CPP
//============================================================================================================================================
// 🧩 Six bands, three narrowings and two empty states — every control live before one asset has been offered.

#include "SlateUI/Interface/AssetPanel/Api/AssetPanel.h"

#include <cstdio>
#include <cstring>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE CONTENT TABLE
//------------------------------------------------------------------------------------------------------------------------

const char* CaptionOf(AssetContent Declared)
{
    switch (Declared)
    {
        case AssetContent::Surface:  return "Surfaces";
        case AssetContent::Material: return "Materials";
        case AssetContent::Brush:    return "Brushes";
        case AssetContent::Geometry: return "Geometry";
        case AssetContent::Document: return "Documents";
        default:                     return "Undeclared";
    }
}

ControlStroke StrokeOf(AssetContent Declared)
{
    switch (Declared)
    {
        case AssetContent::Surface:  return ControlStroke::Image;
        case AssetContent::Brush:    return ControlStroke::Brush;
        case AssetContent::Material: return ControlStroke::Circle;
        case AssetContent::Geometry: return ControlStroke::Cog;
        case AssetContent::Document: return ControlStroke::Check;
        default:                     return ControlStroke::None;
    }
}

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE LOCAL EXTENTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 What the theme does not name, because nothing outside this panel has an opinion on it. Everything the theme
//    does name is read from `LayoutExtents` — a panel spelling `PanelPadding` of its own is the drift
//    `ThemeSpecification` exists to prevent.
constexpr float TileCaptionBand  = 34.0f;   // [px] - the two caption lines beneath a tile's face
constexpr float TileFloor        = 64.0f;   // [px] - the smallest tile edge the track offers
constexpr float TileCeiling      = 168.0f;  // [px] - the largest
constexpr float ColumnFloor      = 0.14f;   // [-]  - the folder column's narrowest share of the body
constexpr float ColumnCeiling    = 0.55f;   // [-]  - its widest
constexpr float DividerReach     = 4.0f;    // [px] - the grab band either side of the divider
constexpr float TileTrackWidth   = 96.0f;   // [px] - the footer's hand-rolled tile track
constexpr float TileTrackFloor   = 40.0f;   // [px] - the narrowest track still worth grabbing
constexpr float OrderingWidth    = 118.0f;  // [px] - the ordering dropdown's head

// 📝 The ordering choices, spelled once. `PresentDropdown` retains no run, but a run rebuilt per call site is a run
//    two call sites can disagree about — and the disagreement presents as a dropdown whose chosen caption is not
//    the one the area ordered by.
const char* const OfferedOrderings[4] = { "Caption", "Content", "Extent", "Declared" };

// 📝 🔴 The six narrowings in `AssetContent`'s own declaration order, so `ContentDeclared[Ordinal]` and
//    `NarrowingCaptions[Ordinal]` name the same content by construction. A second ordering here is the defect
//    where pressing "Brushes" narrows away the geometry.
const char* const NarrowingCaptions[6] = { "Surf", "Matl", "Brsh", "Geom", "Doc", "Und" };

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE LOCAL GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

WorkspaceRectangle BandOf(const WorkspaceRectangle& Area, float PositionY, float Height)
{
    WorkspaceRectangle Band;

    Band.PositionX = Area.PositionX;
    Band.PositionY = PositionY;
    Band.Width     = Area.Width;
    Band.Height    = Height > 0.0f ? Height : 0.0f;

    return Band;
}

WorkspaceRectangle InsetBy(const WorkspaceRectangle& Area, float Inset)
{
    WorkspaceRectangle Narrowed;

    Narrowed.PositionX = Area.PositionX + Inset;
    Narrowed.PositionY = Area.PositionY + Inset;
    Narrowed.Width     = Area.Width  - Inset * 2.0f;
    Narrowed.Height    = Area.Height - Inset * 2.0f;

    if (Narrowed.Width  < 0.0f) Narrowed.Width  = 0.0f;
    if (Narrowed.Height < 0.0f) Narrowed.Height = 0.0f;

    return Narrowed;
}

WorkspaceRectangle LeftOf(const WorkspaceRectangle& Area, float Width)
{
    WorkspaceRectangle Leading = Area;

    Leading.Width = Width < Area.Width ? Width : Area.Width;

    if (Leading.Width < 0.0f)
        Leading.Width = 0.0f;

    return Leading;
}

WorkspaceRectangle RightOf(const WorkspaceRectangle& Area, float Width)
{
    const float Bounded = Width < Area.Width ? Width : Area.Width;

    WorkspaceRectangle Trailing = Area;

    Trailing.PositionX = Area.PositionX + Area.Width - (Bounded > 0.0f ? Bounded : 0.0f);
    Trailing.Width     = Bounded > 0.0f ? Bounded : 0.0f;

    return Trailing;
}

WorkspaceRectangle AfterLeft(const WorkspaceRectangle& Area, float Consumed, float Gap)
{
    WorkspaceRectangle Remainder = Area;

    Remainder.PositionX = Area.PositionX + Consumed + Gap;
    Remainder.Width     = Area.Width - Consumed - Gap;

    if (Remainder.Width < 0.0f)
        Remainder.Width = 0.0f;

    return Remainder;
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

float Bounded(float Asked, float Floor, float Ceiling)
{
    return Asked < Floor ? Floor : (Asked > Ceiling ? Ceiling : Asked);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT WAS DEFERRED
//------------------------------------------------------------------------------------------------------------------------

// 📝 🔴 Collected during the walk and applied after it, exactly as the desk's own `DeferredIntent` is. A row that
//    closed its own fold mid-walk would change the run the walk is still stepping through, and the artist meets
//    that as one press collapsing a folder and choosing the entry that slid up under the pointer.
struct EntryIntent
{
    bool           ChoiceDeclared = false;   // [-] - an entry was chosen
    std::uint32_t  ChosenOrdinal  = 0u;      // [-] - which one
    bool           FolderDeclared = false;   // [-] - a folder row was chosen
    std::uint32_t  FolderOrdinal  = 0u;      // [-] - which one
    bool           FoldDeclared   = false;   // [-] - a folder's twisty was pressed
    std::uint32_t  FoldOrdinal    = 0u;      // [-] - which one
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE ORDERING
//------------------------------------------------------------------------------------------------------------------------

// 📝 Two captions compared without a vendor call, so this file names no library beyond the seam. A null caption
//    orders first rather than dereferencing — an entry offered without one is a declarer's slip, not a crash.
int ComparedCaptions(const char* Leading, const char* Trailing)
{
    if (Leading == nullptr) return Trailing == nullptr ? 0 : -1;
    if (Trailing == nullptr) return 1;

    return std::strcmp(Leading, Trailing);
}

// 📝 🔴 Whether `Leading` precedes `Trailing` under the declared ordering. The declared order is the tie-break in
//    every case, which is what makes the presented run stable: an ordering with ties and no tie-break lets two
//    entries swap places between ticks for no reason the artist can see.
bool Precedes(const AssetSpecification& Standing, std::uint32_t Leading, std::uint32_t Trailing)
{
    const AssetEntry& First  = Standing.Offered[Leading];
    const AssetEntry& Second = Standing.Offered[Trailing];

    switch (Standing.Ordering)
    {
        case AssetOrdering::Caption:
        {
            const int Compared = ComparedCaptions(First.Caption, Second.Caption);

            return Compared != 0 ? Compared < 0 : Leading < Trailing;
        }

        case AssetOrdering::Content:
        {
            if (First.Content != Second.Content)
                return static_cast<std::uint32_t>(First.Content) < static_cast<std::uint32_t>(Second.Content);

            const int Compared = ComparedCaptions(First.Caption, Second.Caption);

            return Compared != 0 ? Compared < 0 : Leading < Trailing;
        }

        case AssetOrdering::Extent:
        {
            if (First.ByteExtent != Second.ByteExtent)
                return First.ByteExtent > Second.ByteExtent;

            return Leading < Trailing;
        }

        case AssetOrdering::Declared:
        default:
            return Leading < Trailing;
    }
}

// 📝 🔴 The surviving ordinals are resolved into a fixed run and the offered run is never permuted. A panel that
//    sorted what it was handed would reorder storage its declarer still holds ordinals into, and the defect
//    presents as a host's own reference naming a different asset after the artist changed the ordering.
std::uint32_t ResolvePresentedOrder(const AssetSpecification& Standing, std::uint32_t (&Presented)[AssetEntryCapacity])
{
    std::uint32_t Counted = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < Standing.OfferedCount && Ordinal < AssetEntryCapacity; ++Ordinal)
    {
        if (EntrySurvives(Standing, Ordinal))
            Presented[Counted++] = Ordinal;
    }

    // 📝 An insertion order over a bounded run. The extent is 128 and the comparison is three reads, so the
    //    quadratic term is smaller than one allocation would be — and `00` forbids the allocation either way.
    for (std::uint32_t Ordinal = 1u; Ordinal < Counted; ++Ordinal)
    {
        const std::uint32_t Held = Presented[Ordinal];

        std::uint32_t Position = Ordinal;

        while (Position > 0u && Precedes(Standing, Held, Presented[Position - 1u]))
        {
            Presented[Position] = Presented[Position - 1u];
            --Position;
        }

        Presented[Position] = Held;
    }

    return Counted;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE BYTE READOUT
//------------------------------------------------------------------------------------------------------------------------

// 📝 A byte extent printed at the largest tier it fills, so a 4 MB surface does not read as seven digits the
//    artist has to count. Integer arithmetic throughout — a byte count is Exact and dividing it into a real would
//    make the readout a derived magnitude the panel then has to make a precision claim about.
void CarryByteExtent(char (&Destination)[24], std::uint64_t ByteExtent)
{
    if (ByteExtent >= 1024ull * 1024ull * 1024ull)
    {
        std::snprintf(Destination, sizeof Destination, "%llu.%llu GB",
                      static_cast<unsigned long long>(ByteExtent / (1024ull * 1024ull * 1024ull)),
                      static_cast<unsigned long long>((ByteExtent / (1024ull * 1024ull * 107ull)) % 10ull));
    }
    else if (ByteExtent >= 1024ull * 1024ull)
    {
        std::snprintf(Destination, sizeof Destination, "%llu.%llu MB",
                      static_cast<unsigned long long>(ByteExtent / (1024ull * 1024ull)),
                      static_cast<unsigned long long>((ByteExtent / (1024ull * 107ull)) % 10ull));
    }
    else if (ByteExtent >= 1024ull)
    {
        std::snprintf(Destination, sizeof Destination, "%llu KB",
                      static_cast<unsigned long long>(ByteExtent / 1024ull));
    }
    else
    {
        std::snprintf(Destination, sizeof Destination, "%llu B", static_cast<unsigned long long>(ByteExtent));
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE HEADER BAND
//------------------------------------------------------------------------------------------------------------------------

void PresentHeaderBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       AssetSpecification&        Standing,
                       std::uint32_t              Surviving)
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

    PresentTextRun(Interior, "Assets", Palette.TextPrimary, TextPlacement::Leading, 1.0f);

    // -- the two presentation glyphs, at the trailing edge ----------------------------------------------------------
    WorkspaceRectangle Trailing = RightOf(Interior, Extents.GlyphButtonEdge);
    Trailing = CentredWithin(Trailing, Extents.GlyphButtonEdge, Extents.GlyphButtonEdge);

    const Outcome<ControlInteraction> Reloaded =
        PresentGlyphButton(Theme, Trailing, ControlStroke::Reload, 0u, false);

    // 📝 The reload glyph answers the pointer and declares nothing. Nothing in `SlateUI` resamples content — `04`'s
    //    interchange owns that — so a panel that acted here would be the panel deciding what is on disk.
    (void)Reloaded;

    WorkspaceRectangle Rowed = Trailing;
    Rowed.PositionX -= Extents.GlyphButtonEdge + Extents.ControlSpacing;

    const bool RowsStanding = Standing.Presentation == AssetPresentation::Rows;

    // 📝 The glyph names what the press would switch **to** and not what is standing, which is why a tile
    //    presentation shows the row stroke. A button captioned with the standing choice reads as a readout, and the
    //    artist presses it expecting nothing to happen.
    const Outcome<ControlInteraction> Switched =
        PresentGlyphButton(Theme, Rowed, RowsStanding ? ControlStroke::Image : ControlStroke::Grip, 0u,
                           RowsStanding);

    if (Switched.ContentPresent && Switched.Resolve().EditSealed)
    {
        Standing.Presentation =
            RowsStanding ? AssetPresentation::Tiles : AssetPresentation::Rows;
    }

    // -- the surviving count, between the caption and the glyphs ----------------------------------------------------
    char Counted[48] = {};

    std::snprintf(Counted, sizeof Counted, "%u of %u", Surviving, Standing.OfferedCount);

    WorkspaceRectangle Readout = Interior;
    Readout.Width -= Extents.GlyphButtonEdge * 2.0f + Extents.ControlSpacing * 2.0f;

    if (Readout.Width > 0.0f)
        PresentTextRun(Readout, Counted, Palette.TextMuted, TextPlacement::Trailing, 0.9f);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE SEARCH BAND
//------------------------------------------------------------------------------------------------------------------------

void PresentSearchBand(const ThemeSpecification& Theme, const WorkspaceRectangle& Band, AssetSpecification& Standing)
{
    if (Band.Height <= 1.0f)
        return;

    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    PresentSurfaceFill(Band, Palette.PanelBackground, 0.0f);

    WorkspaceRectangle Interior = Band;
    Interior.PositionX += Extents.PanelPadding;
    Interior.Width     -= Extents.PanelPadding * 2.0f;

    if (Interior.Width <= 1.0f)
        return;

    // 📝 🔴 The cap sits in the label column `PresentTextEntry` reserves and not inside the field. A caption of
    //    `nullptr` suppresses the caption alone — `ResolveControlRow` still divides the row — so a cap painted at
    //    the row's leading edge lands in the column the entry left empty, which is exactly where the reference's
    //    `.search` cap sits relative to its own field.
    const Outcome<ControlInteraction> Narrowed = PresentTextEntry(Theme, Interior, nullptr, Standing.Sought, "Search");

    if (!Narrowed.ContentPresent)
        return;

    WorkspaceRectangle Cap = LeftOf(Interior, Extents.GlyphEdge + Extents.ControlSpacing);
    Cap = CentredWithin(Cap, Extents.GlyphEdge, Extents.GlyphEdge);

    PresentControlStroke(Cap, ControlStroke::Search, Palette.TextMuted, Extents.BorderThickness, 0.0f);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE NARROWING BAND
//------------------------------------------------------------------------------------------------------------------------

void PresentNarrowBand(const ThemeSpecification& Theme, const WorkspaceRectangle& Band, AssetSpecification& Standing)
{
    if (Band.Height <= 1.0f)
        return;

    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    PresentSurfaceFill(Band, Palette.PanelBackground, 0.0f);

    WorkspaceRectangle Interior = Band;
    Interior.PositionX += Extents.PanelPadding;
    Interior.Width     -= Extents.PanelPadding * 2.0f;

    if (Interior.Width <= 1.0f)
        return;

    // 📝 🔴 The ordering head is laid out from the leading edge and the segments take the remainder. Both
    //    primitives here are `CentredBand` controls and reserve no label column, so the arithmetic is the band's
    //    own — which is why this band can carry two controls where the search band cannot carry two.
    WorkspaceRectangle OrderingHead = LeftOf(Interior, OrderingWidth);

    std::uint32_t Chosen = static_cast<std::uint32_t>(Standing.Ordering);

    if (Chosen > 3u)
        Chosen = 0u;

    const Outcome<ControlInteraction> Ordered =
        PresentDropdown(Theme, OrderingHead, OfferedOrderings, 4u, Chosen,
                        Standing.OrderingCarry, Standing.PresentedTicks);

    if (Ordered.ContentPresent && Chosen <= 3u)
        Standing.Ordering = static_cast<AssetOrdering>(Chosen);

    // 📝 🔴 A segment row and not a dropdown. The six narrowings are independent switches and a dropdown reports
    //    one ordinal — read as a choice, enabling brushes would disable surfaces, and the artist meets that as a
    //    narrowing that can never show two contents at once.
    WorkspaceRectangle Segments = AfterLeft(Interior, OrderingWidth, Extents.ControlSpacing);

    if (Segments.Width <= 1.0f)
        return;

    const Outcome<ControlInteraction> Narrowed =
        PresentSegmentRow(Theme, Segments, NarrowingCaptions, Standing.ContentDeclared, 6u);

    // 📝 ⚠️ A refusal is presented and never raised. `PresentSegmentRow` refuses below one glyph per segment, and a
    //    panel docked that narrow is the ordinary case rather than a fault — `14` §7 puts refusal on the desk.
    (void)Narrowed;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE FOLDER COLUMN
//------------------------------------------------------------------------------------------------------------------------

void PresentFolderColumn(const ThemeSpecification&  Theme,
                         const WorkspaceRectangle&  Column,
                         AssetSpecification&        Standing,
                         EntryIntent&               Arriving)
{
    if (Column.Width <= 1.0f || Column.Height <= 1.0f)
        return;

    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    PresentSurfaceFill(Column, Attenuate(Palette.DeskBackground, 0.55), 0.0f);

    if (Standing.FolderCount == 0u)
    {
        PresentTextRun(Column, "no folders", Palette.TextMuted, TextPlacement::Centred, 0.9f);

        return;
    }

    DeclareClip(Column);

    // 📝 The content is measured before it is presented, because the visible offset is bounded against the content
    //    and a bound applied afterwards lags the wheel by one tick.
    std::uint32_t Presentable = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < Standing.FolderCount && Ordinal < AssetFolderCapacity; ++Ordinal)
    {
        // 📝 🔴 A row presents where every shallower fold above it is open. Resolved by walking back from the row
        //    rather than by remembering an occlusion as the walk descends: a remembered one is wrong the first time
        //    two folds close at the same depth, and the artist sees rows of a closed folder still standing.
        bool Occluded = false;

        for (std::uint32_t Above = Ordinal; Above > 0u; --Above)
        {
            const AssetFolder& Enclosing = Standing.Folders[Above - 1u];

            if (Enclosing.Depth < Standing.Folders[Ordinal].Depth && !Enclosing.FolderOpen)
            {
                Occluded = true;
                break;
            }

            if (Enclosing.Depth == 0u)
                break;
        }

        if (!Occluded)
            ++Presentable;
    }

    AdvanceVisibleOffset(Standing.FolderOffset, Column,
                         static_cast<float>(Presentable) * Extents.RowHeight + Extents.PanelPadding);

    float Travelled = Column.PositionY + Extents.PanelPadding * 0.5f - Standing.FolderOffset;

    for (std::uint32_t Ordinal = 0u; Ordinal < Standing.FolderCount && Ordinal < AssetFolderCapacity; ++Ordinal)
    {
        const AssetFolder& Offered = Standing.Folders[Ordinal];

        bool Occluded = false;

        for (std::uint32_t Above = Ordinal; Above > 0u; --Above)
        {
            const AssetFolder& Enclosing = Standing.Folders[Above - 1u];

            if (Enclosing.Depth < Offered.Depth && !Enclosing.FolderOpen)
            {
                Occluded = true;
                break;
            }

            if (Enclosing.Depth == 0u)
                break;
        }

        if (Occluded)
            continue;

        WorkspaceRectangle Row = BandOf(Column, Travelled, Extents.RowHeight);

        Travelled += Extents.RowHeight;

        if (Row.PositionY + Row.Height < Column.PositionY || Row.PositionY > Column.PositionY + Column.Height)
            continue;

        const ControlInteraction Pressed  = ResolveAreaPress(Row);
        const bool               Standing_ = Ordinal == Standing.ChosenFolder;

        if (Standing_)
            PresentSurfaceFill(Row, Palette.AccentSubtle, 0.0f);
        else if (Pressed.PointerOver)
            PresentSurfaceFill(Row, Palette.RowHovered, 0.0f);

        const float Indent = static_cast<float>(Offered.Depth) * Extents.IndentWidth;

        WorkspaceRectangle Twisty = Row;
        Twisty.PositionX += Extents.PanelPadding * 0.5f + Indent;
        Twisty = CentredWithin(LeftOf(Twisty, Extents.GlyphEdge), Extents.GlyphEdge, Extents.GlyphEdge);

        // 📝 🔴 The twisty is tested before the row, and the row's own press is refused where the twisty took it.
        //    Tested the other way a press on the twisty would both open the fold and choose the folder, and the
        //    artist cannot open a folder without also changing what the area presents.
        const ControlInteraction Turned = ResolveAreaPress(Twisty);

        if (Offered.CountedEntries > 0u || Offered.Depth == 0u)
        {
            PresentControlStroke(Twisty, ControlStroke::Twisty,
                                 Turned.PointerOver ? Palette.TextPrimary : Palette.TextMuted,
                                 Extents.BorderThickness,
                                 Offered.FolderOpen ? 1.5707963f : 0.0f);
        }

        WorkspaceRectangle Caption = AfterLeft(Row, Indent + Extents.GlyphEdge + Extents.PanelPadding,
                                               Extents.ControlSpacing);
        Caption.Width -= Extents.PanelPadding * 2.0f;

        PresentTextRun(Caption, Offered.Caption != nullptr ? Offered.Caption : "",
                       Standing_ ? Palette.TextOnAccent : Palette.TextPrimary, TextPlacement::Leading, 0.95f);

        if (Offered.CountedEntries > 0u)
        {
            char Counted[16] = {};

            std::snprintf(Counted, sizeof Counted, "%u", Offered.CountedEntries);

            PresentTextRun(Caption, Counted, Palette.TextMuted, TextPlacement::Trailing, 0.85f);
        }

        if (Turned.EditSealed)
        {
            Arriving.FoldDeclared = true;
            Arriving.FoldOrdinal  = Ordinal;
        }
        else if (Pressed.EditSealed)
        {
            Arriving.FolderDeclared = true;
            Arriving.FolderOrdinal  = Ordinal;
        }
    }

    ReclaimClip();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        ONE TILE
//------------------------------------------------------------------------------------------------------------------------

void PresentOneTile(const ThemeSpecification&  Theme,
                    const WorkspaceRectangle&  Area,
                    const AssetEntry&          Offered,
                    bool                       Chosen,
                    std::uint32_t              Ordinal,
                    EntryIntent&               Arriving)
{
    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    const ControlInteraction Pressed = ResolveAreaPress(Area);

    PresentSurfaceFill(Area, Pressed.PointerOver ? Palette.TileHovered : Palette.TileBackground,
                       Extents.CornerRounding * 0.5f);

    if (Chosen)
        PresentSurfaceOutline(Area, Palette.AccentPrimary, Extents.CornerRounding * 0.5f, Extents.TabUnderline);

    WorkspaceRectangle Face = Area;
    Face.Height = Area.Height - TileCaptionBand;

    if (Face.Height > 1.0f)
    {
        PresentSurfaceFill(InsetBy(Face, Extents.BorderThickness * 2.0f),
                           Attenuate(Palette.DeskBackground, 0.7), Extents.CornerRounding * 0.4f);

        // 📝 ⚠️ The stroke is the placeholder and the uploaded thumbnail is not reached for here. `GlyphDepot`
        //    resolves an authored image and a panel that resolved one itself would be the second component
        //    deciding what a tier holds — the stroke is what a tile presents until the depot answers.
        WorkspaceRectangle Glyph = CentredWithin(Face, Extents.GlyphButtonEdge, Extents.GlyphButtonEdge);

        PresentControlStroke(Glyph, StrokeOf(Offered.Content), Palette.TextMuted, Extents.BorderThickness * 1.5f, 0.0f);
    }

    WorkspaceRectangle Caption;
    Caption.PositionX = Area.PositionX + Extents.PanelPadding * 0.5f;
    Caption.PositionY = Area.PositionY + Area.Height - TileCaptionBand;
    Caption.Width     = Area.Width - Extents.PanelPadding;
    Caption.Height    = TileCaptionBand * 0.5f;

    PresentTextRun(Caption, Offered.Caption != nullptr ? Offered.Caption : "",
                   Chosen ? Palette.TextPrimary : Palette.TextMuted, TextPlacement::Leading, 0.9f);

    WorkspaceRectangle Detail = Caption;
    Detail.PositionY += TileCaptionBand * 0.5f;

    // 📝 The declarer's own detail where it offered one, and the byte extent otherwise. A tile that printed both
    //    would need a third line, and a third line is what makes the smallest offered tile unreadable.
    if (Offered.Detail != nullptr && Offered.Detail[0] != '\0')
    {
        PresentTextRun(Detail, Offered.Detail, Palette.TextMuted, TextPlacement::Leading, 0.82f);
    }
    else
    {
        char Read[24] = {};

        CarryByteExtent(Read, Offered.ByteExtent);

        PresentTextRun(Detail, Read, Palette.TextMuted, TextPlacement::Leading, 0.82f);
    }

    if (Pressed.EditSealed)
    {
        Arriving.ChoiceDeclared = true;
        Arriving.ChosenOrdinal  = Ordinal;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        ONE ROW
//------------------------------------------------------------------------------------------------------------------------

void PresentOneRow(const ThemeSpecification&  Theme,
                   const WorkspaceRectangle&  Area,
                   const AssetEntry&          Offered,
                   bool                       Chosen,
                   std::uint32_t              Ordinal,
                   EntryIntent&               Arriving)
{
    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    const ControlInteraction Pressed = ResolveAreaPress(Area);

    if (Chosen)
        PresentSurfaceFill(Area, Palette.AccentSubtle, Extents.CornerRounding * 0.3f);
    else if (Pressed.PointerOver)
        PresentSurfaceFill(Area, Palette.RowHovered, Extents.CornerRounding * 0.3f);

    WorkspaceRectangle Glyph = LeftOf(Area, Extents.RowHeight);
    Glyph = CentredWithin(Glyph, Extents.GlyphEdge, Extents.GlyphEdge);

    PresentControlStroke(Glyph, StrokeOf(Offered.Content), Palette.TextMuted, Extents.BorderThickness, 0.0f);

    WorkspaceRectangle Caption = AfterLeft(Area, Extents.RowHeight, 0.0f);
    Caption.Width -= Extents.PanelPadding;

    if (Caption.Width <= 1.0f)
        return;

    PresentTextRun(Caption, Offered.Caption != nullptr ? Offered.Caption : "",
                   Chosen ? Palette.TextOnAccent : Palette.TextPrimary, TextPlacement::Leading, 0.95f);

    char Read[24] = {};

    CarryByteExtent(Read, Offered.ByteExtent);

    PresentTextRun(Caption, Read, Palette.TextMuted, TextPlacement::Trailing, 0.85f);

    if (Pressed.EditSealed)
    {
        Arriving.ChoiceDeclared = true;
        Arriving.ChosenOrdinal  = Ordinal;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE ENTRY AREA
//------------------------------------------------------------------------------------------------------------------------

void PresentEntryArea(const ThemeSpecification&  Theme,
                      const WorkspaceRectangle&  Area,
                      AssetSpecification&        Standing,
                      EntryIntent&               Arriving)
{
    if (Area.Width <= 1.0f || Area.Height <= 1.0f)
        return;

    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    PresentSurfaceFill(Area, Palette.PanelBackground, 0.0f);

    std::uint32_t       Presented[AssetEntryCapacity] = {};
    const std::uint32_t Counted                       = ResolvePresentedOrder(Standing, Presented);

    if (Counted == 0u)
    {
        // 📝 🔴 Two different empty states, because they call for two different answers. Nothing offered at all is
        //    a workspace nobody has declared content into; nothing surviving is a narrowing the artist can widen,
        //    and telling them "no assets" when their own search is the cause is a panel that looks broken.
        const char* Said = Standing.OfferedCount == 0u ? "No assets offered" : "Nothing matches this narrowing";

        PresentTextRun(Area, Said, Palette.TextMuted, TextPlacement::Centred, 1.0f);

        return;
    }

    DeclareClip(Area);

    if (Standing.Presentation == AssetPresentation::Rows)
    {
        AdvanceVisibleOffset(Standing.VisibleOffset, Area,
                             static_cast<float>(Counted) * Extents.RowHeight + Extents.PanelPadding * 2.0f);

        float Travelled = Area.PositionY + Extents.PanelPadding - Standing.VisibleOffset;

        for (std::uint32_t Position = 0u; Position < Counted; ++Position)
        {
            WorkspaceRectangle Row = BandOf(Area, Travelled, Extents.RowHeight);
            Row.PositionX += Extents.PanelPadding;
            Row.Width     -= Extents.PanelPadding * 2.0f;

            Travelled += Extents.RowHeight;

            if (Row.PositionY + Row.Height < Area.PositionY || Row.PositionY > Area.PositionY + Area.Height)
                continue;

            const std::uint32_t Ordinal = Presented[Position];

            PresentOneRow(Theme, Row, Standing.Offered[Ordinal],
                          Standing.EntryChosen && Standing.ChosenEntry == Ordinal, Ordinal, Arriving);
        }

        ReclaimClip();

        return;
    }

    // -- the tile lattice ------------------------------------------------------------------------------------------
    const float TileEdge  = Bounded(static_cast<float>(Standing.TileExtent), TileFloor, TileCeiling);
    const float TilePitch = TileEdge + Extents.CardGap;
    const float Usable    = Area.Width - Extents.PanelPadding * 2.0f;

    // 📝 At least one per row. A zero column count divides by zero resolving the row a tile sits in, and a panel
    //    docked narrower than one tile is the ordinary case rather than a refusal.
    std::uint32_t PerRow = Usable > TilePitch ? static_cast<std::uint32_t>(Usable / TilePitch) : 1u;

    if (PerRow == 0u)
        PerRow = 1u;

    const std::uint32_t Rows = (Counted + PerRow - 1u) / PerRow;

    AdvanceVisibleOffset(Standing.VisibleOffset, Area,
                         static_cast<float>(Rows) * TilePitch + Extents.PanelPadding * 2.0f);

    for (std::uint32_t Position = 0u; Position < Counted; ++Position)
    {
        const std::uint32_t Across = Position % PerRow;
        const std::uint32_t Down   = Position / PerRow;

        WorkspaceRectangle Tile;
        Tile.PositionX = Area.PositionX + Extents.PanelPadding + static_cast<float>(Across) * TilePitch;
        Tile.PositionY = Area.PositionY + Extents.PanelPadding + static_cast<float>(Down) * TilePitch
                       - Standing.VisibleOffset;
        Tile.Width     = TileEdge;
        Tile.Height    = TileEdge;

        if (Tile.PositionY + Tile.Height < Area.PositionY || Tile.PositionY > Area.PositionY + Area.Height)
            continue;

        const std::uint32_t Ordinal = Presented[Position];

        PresentOneTile(Theme, Tile, Standing.Offered[Ordinal],
                       Standing.EntryChosen && Standing.ChosenEntry == Ordinal, Ordinal, Arriving);
    }

    ReclaimClip();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE FOOTER BAND
//------------------------------------------------------------------------------------------------------------------------

void PresentFooterBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       AssetSpecification&        Standing,
                       std::uint32_t              Surviving)
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

    // 📝 The chosen entry's own caption where one is chosen, and the counts otherwise. A footer carrying both is a
    //    footer whose left half is unreadable at every width a docked panel actually takes.
    char Said[128] = {};

    if (Standing.EntryChosen && Standing.ChosenEntry < Standing.OfferedCount)
    {
        const AssetEntry& Chosen = Standing.Offered[Standing.ChosenEntry];

        char Read[24] = {};

        CarryByteExtent(Read, Chosen.ByteExtent);

        std::snprintf(Said, sizeof Said, "%s  \xC2\xB7  %s  \xC2\xB7  %s",
                      Chosen.Caption != nullptr ? Chosen.Caption : "",
                      CaptionOf(Chosen.Content), Read);
    }
    else
    {
        std::snprintf(Said, sizeof Said, "%u shown  \xC2\xB7  %u folder%s",
                      Surviving, Standing.FolderCount, Standing.FolderCount == 1u ? "" : "s");
    }

    WorkspaceRectangle Readout = Interior;
    Readout.Width -= TileTrackWidth + Extents.ControlSpacing;

    if (Readout.Width > 0.0f)
        PresentTextRun(Readout, Said, Palette.TextMuted, TextPlacement::Leading, 0.85f);

    // -- the tile track, presented only where tiles are -------------------------------------------------------------
    if (Standing.Presentation != AssetPresentation::Tiles)
        return;

    WorkspaceRectangle Track = RightOf(Interior, TileTrackWidth);
    Track.Height    = Extents.SliderTrackHeight * 0.5f;
    Track.PositionY = Interior.PositionY + (Interior.Height - Track.Height) * 0.5f;

    if (Track.Width < TileTrackFloor)
        return;

    // 📝 🔴 Hand-rolled and not `PresentValueSlider`. Every labelled numeric primitive divides its row through
    //    `ResolveControlRow` first and then refuses below a value box plus a grabbable track — 78 + 6 + 42 px of
    //    field — so a slider in a 26 px footer would need a 316 px row and would be refused at every width a
    //    docked panel actually takes. A footer's readout has no label column to spare, so the track is the whole
    //    control: a fill, a knob, and the same `PointerHeld` follow the divider uses.
    const ControlInteraction Grabbed = ResolveAreaPress(Track);

    if (Grabbed.EditOpened)
        Standing.TileHeld = true;

    if (Standing.TileHeld)
    {
        if (!PointerHeld())
        {
            Standing.TileHeld = false;
        }
        else
        {
            float PointerX = 0.0f;
            float PointerY = 0.0f;

            ResolvePointerPosition(PointerX, PointerY);

            const float Travelled = Bounded((PointerX - Track.PositionX) / Track.Width, 0.0f, 1.0f);

            Standing.TileExtent =
                static_cast<double>(TileFloor) + static_cast<double>(Travelled * (TileCeiling - TileFloor));
        }
    }

    const float Reading  = Bounded(static_cast<float>(Standing.TileExtent), TileFloor, TileCeiling);
    const float Fraction = (Reading - TileFloor) / (TileCeiling - TileFloor);

    PresentSurfaceFill(Track, Palette.SliderTrack, Track.Height * 0.5f);

    WorkspaceRectangle Travelled = Track;
    Travelled.Width = Track.Width * Fraction;

    if (Travelled.Width > 0.0f)
        PresentSurfaceFill(Travelled, Palette.SliderFill, Track.Height * 0.5f);

    WorkspaceRectangle Knob;
    Knob.Width     = Extents.SliderTrackHeight;
    Knob.Height    = Extents.SliderTrackHeight;
    Knob.PositionX = Track.PositionX + Track.Width * Fraction - Knob.Width * 0.5f;
    Knob.PositionY = Track.PositionY + (Track.Height - Knob.Height) * 0.5f;

    PresentSurfaceFill(Knob, Standing.TileHeld || Grabbed.PointerOver ? Palette.AccentPrimary : Palette.SliderKnob,
                       Knob.Width * 0.5f);
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE BAND ARITHMETIC
//------------------------------------------------------------------------------------------------------------------------

AssetBands ResolveAssetBands(const LayoutExtents& Extents, const WorkspaceRectangle& Area, float ColumnFraction)
{
    AssetBands Resolved;

    const float SearchHeight = Extents.EntryRowHeight + Extents.ControlSpacing;
    const float NarrowHeight = Extents.DropdownHeight + Extents.ControlSpacing;

    Resolved.HeaderBand = BandOf(Area, Area.PositionY, Extents.PanelHeaderHeight);

    Resolved.SearchBand = BandOf(Area, Resolved.HeaderBand.PositionY + Resolved.HeaderBand.Height, SearchHeight);

    Resolved.NarrowBand = BandOf(Area, Resolved.SearchBand.PositionY + Resolved.SearchBand.Height, NarrowHeight);

    Resolved.FooterBand = BandOf(Area, Area.PositionY + Area.Height - Extents.PanelFooterHeight,
                                 Extents.PanelFooterHeight);

    const float BodyTop    = Resolved.NarrowBand.PositionY + Resolved.NarrowBand.Height;
    const float BodyBottom = Resolved.FooterBand.PositionY;
    const float BodyHeight = BodyBottom > BodyTop ? BodyBottom - BodyTop : 0.0f;

    const float Share = Bounded(ColumnFraction, ColumnFloor, ColumnCeiling);

    Resolved.FolderColumn.PositionX = Area.PositionX;
    Resolved.FolderColumn.PositionY = BodyTop;
    Resolved.FolderColumn.Width     = Area.Width * Share;
    Resolved.FolderColumn.Height    = BodyHeight;

    Resolved.Divider.PositionX = Resolved.FolderColumn.PositionX + Resolved.FolderColumn.Width;
    Resolved.Divider.PositionY = BodyTop;
    Resolved.Divider.Width     = Extents.GutterThickness;
    Resolved.Divider.Height    = BodyHeight;

    Resolved.EntryArea.PositionX = Resolved.Divider.PositionX + Resolved.Divider.Width;
    Resolved.EntryArea.PositionY = BodyTop;
    Resolved.EntryArea.Width     = Area.PositionX + Area.Width - Resolved.EntryArea.PositionX;
    Resolved.EntryArea.Height    = BodyHeight;

    if (Resolved.EntryArea.Width < 0.0f)
        Resolved.EntryArea.Width = 0.0f;

    return Resolved;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE NARROWING
//------------------------------------------------------------------------------------------------------------------------

bool EntrySurvives(const AssetSpecification& Standing, std::uint32_t Ordinal)
{
    if (Ordinal >= Standing.OfferedCount || Ordinal >= AssetEntryCapacity)
        return false;

    const AssetEntry& Offered = Standing.Offered[Ordinal];

    // 📝 The folder narrowing is skipped where no folder is declared, so an entry offered before any folder is
    //    still presented. A run narrowed against a column that does not exist is a panel presenting nothing while
    //    its footer counts everything.
    if (Standing.FolderCount > 0u && Offered.FolderOrdinal != Standing.ChosenFolder)
        return false;

    const std::uint32_t Content = static_cast<std::uint32_t>(Offered.Content);

    if (Content < 6u && !Standing.ContentDeclared[Content])
        return false;

    if (Standing.Sought.CarryExtent == 0u)
        return true;

    if (Offered.Caption == nullptr)
        return false;

    // 📝 🔴 A caseless containment and not a prefix. An artist typing "rough" expects `MetalRoughness` — and a
    //    prefix match here is the narrowing that appears broken for every asset named by its material first.
    const std::size_t SoughtExtent  = std::strlen(Standing.Sought.Carried);
    const std::size_t CaptionExtent = std::strlen(Offered.Caption);

    if (SoughtExtent == 0u)
        return true;

    if (CaptionExtent < SoughtExtent)
        return false;

    for (std::size_t Start = 0u; Start + SoughtExtent <= CaptionExtent; ++Start)
    {
        std::size_t Matched = 0u;

        while (Matched < SoughtExtent)
        {
            char Carried = Standing.Sought.Carried[Matched];
            char Offered_ = Offered.Caption[Start + Matched];

            if (Carried >= 'A' && Carried <= 'Z')  Carried  = static_cast<char>(Carried  - 'A' + 'a');
            if (Offered_ >= 'A' && Offered_ <= 'Z') Offered_ = static_cast<char>(Offered_ - 'A' + 'a');

            if (Carried != Offered_)
                break;

            ++Matched;
        }

        if (Matched == SoughtExtent)
            return true;
    }

    return false;
}

std::uint32_t SurvivingCount(const AssetSpecification& Standing)
{
    std::uint32_t Counted = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < Standing.OfferedCount && Ordinal < AssetEntryCapacity; ++Ordinal)
        Counted += EntrySurvives(Standing, Ordinal) ? 1u : 0u;

    return Counted;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

void PresentAssets(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, AssetSpecification& Standing)
{
    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    if (Area.Width <= 1.0f || Area.Height <= 1.0f)
        return;

    // 📝 🔴 Advanced once, here, at the top of the tick. Every dropdown carried by this panel compares against it
    //    to tell the press that opened it from the press that dismisses it, and two panels advancing one count
    //    would leave a dropdown that closes on the tick it opened.
    ++Standing.PresentedTicks;

    PresentSurfaceFill(Area, Palette.PanelBackground, Extents.CornerRounding);

    const AssetBands  Bands     = ResolveAssetBands(Extents, Area, Standing.ColumnFraction);
    const std::uint32_t Surviving = SurvivingCount(Standing);

    EntryIntent Arriving;

    PresentHeaderBand(Theme, Bands.HeaderBand, Standing, Surviving);
    PresentSearchBand(Theme, Bands.SearchBand, Standing);
    PresentNarrowBand(Theme, Bands.NarrowBand, Standing);
    PresentFolderColumn(Theme, Bands.FolderColumn, Standing, Arriving);
    PresentEntryArea(Theme, Bands.EntryArea, Standing, Arriving);
    PresentFooterBand(Theme, Bands.FooterBand, Standing, Surviving);

    // -- the divider between the column and the area ---------------------------------------------------------------
    {
        WorkspaceRectangle Reach = Bands.Divider;
        Reach.PositionX -= DividerReach;
        Reach.Width     += DividerReach * 2.0f;

        const ControlInteraction Grabbed = ResolveAreaPress(Reach);

        PresentSurfaceFill(Bands.Divider,
                           Grabbed.PointerOver || Standing.ColumnHeld ? Palette.AccentSubtle : Palette.PanelBorder,
                           0.0f);

        if (Grabbed.EditOpened)
            Standing.ColumnHeld = true;

        // 📝 🔴 The drag is followed through `PointerHeld` and not through the grab's own interaction, because the
        //    pointer leaves the divider on the first pixel of travel. A drag that ended when the pointer left the
        //    rectangle would be a divider that can only be moved four pixels at a time.
        if (Standing.ColumnHeld)
        {
            if (!PointerHeld())
            {
                Standing.ColumnHeld = false;
            }
            else if (Area.Width > 1.0f)
            {
                float PointerX = 0.0f;
                float PointerY = 0.0f;

                ResolvePointerPosition(PointerX, PointerY);

                Standing.ColumnFraction =
                    Bounded((PointerX - Area.PositionX) / Area.Width, ColumnFloor, ColumnCeiling);
            }
        }
    }

    // -- what the walk deferred -------------------------------------------------------------------------------------
    if (Arriving.FoldDeclared && Arriving.FoldOrdinal < AssetFolderCapacity)
    {
        AssetFolder& Turned = Standing.Folders[Arriving.FoldOrdinal];

        Turned.FolderOpen = !Turned.FolderOpen;
    }

    if (Arriving.FolderDeclared && Arriving.FolderOrdinal < Standing.FolderCount)
    {
        Standing.ChosenFolder = Arriving.FolderOrdinal;

        // 📝 🔴 The chosen entry is released when the folder changes rather than carried across it. Carried, the
        //    footer would print an entry the area no longer presents, and the artist would be looking at a caption
        //    for something in a folder they have left.
        Standing.EntryChosen   = false;
        Standing.VisibleOffset = 0.0f;
    }

    if (Arriving.ChoiceDeclared && Arriving.ChosenOrdinal < Standing.OfferedCount)
    {
        // 📝 A second press on the standing entry releases it. Nothing here opens anything — `04`'s interchange
        //    owns opening, and a panel that opened an asset would be the panel deciding what a session contains.
        const bool Held = Standing.EntryChosen && Standing.ChosenEntry == Arriving.ChosenOrdinal;

        Standing.ChosenEntry = Arriving.ChosenOrdinal;
        Standing.EntryChosen = !Held;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PANEL ROUTINE
//------------------------------------------------------------------------------------------------------------------------

void PresentAssetPanel(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, void* PresentContext)
{
    AssetSpecification* Standing = static_cast<AssetSpecification*>(PresentContext);

    if (Standing == nullptr)
    {
        PresentSurfaceFill(Area, Theme.Palette.PanelBackground, Theme.Extents.CornerRounding);
        PresentTextRun(Area, "no assets", Theme.Palette.TextMuted, TextPlacement::Centred, 1.0f);

        return;
    }

    PresentAssets(Theme, Area, *Standing);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE LEDGER SLOT
//------------------------------------------------------------------------------------------------------------------------

PanelSlot ResolveAssetSlot(const char*          PanelIdentifier,
                           const char*          PanelTitle,
                           WorkspacePanelSide   DeclaredSide,
                           AssetSpecification&  Standing)
{
    PanelSlot Declaring;

    Declaring.PanelIdentifier = PanelIdentifier;
    Declaring.PanelTitle      = PanelTitle;
    Declaring.DeclaredSide    = DeclaredSide;
    Declaring.Present         = &PresentAssetPanel;
    Declaring.PresentContext  = &Standing;

    return Declaring;
}

}   // namespace Slate
