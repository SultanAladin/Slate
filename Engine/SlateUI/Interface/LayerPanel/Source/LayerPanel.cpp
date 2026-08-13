//============================================================================================================================================
//                                                             LAYERPANEL.CPP
//============================================================================================================================================
// 🧩 `TexturePaint.tsx`'s layer stack, band for band — the 46 px header, the spine, the 50/50 split row and the 26 px footer.

#include "SlateUI/Interface/LayerPanel/Api/LayerPanel.h"

#include <cstdio>
#include <cstring>

// 📝 🔴 No vendor spelling appears in this file and none may. Every fill, outline, run of text and clip goes through
//    `ControlPanel`'s painting seam, which is the one component that knows which recording the interface paints on.

namespace Slate
{
namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE REFERENCE GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

// 📝 What `TexturePaint.tsx:38-339` spells that the theme does not. Everything the theme already carries — the 46 px
//    header, the 44 px row, the 26 px footer, the paddings, the roundings — is read from `LayoutExtents` and is not
//    repeated here. These are the four extents that belong to this panel alone.
constexpr float SpineWidth     = 30.0f;   // [px] - the gutter carrying the rail and the index badges
constexpr float SpineRail      =  3.0f;   // [px] - the rail running down the middle of the gutter
constexpr float BadgeEdge      = 20.0f;   // [px] - the circular zero-padded index badge
constexpr float ThumbnailEdge  = 26.0f;   // [px] - the layer thumbnail and the mask tile alike
constexpr float SubjectDotEdge =  4.0f;   // [px] - the dot beside a row's name
constexpr float AddRowHeight   = 28.0f;   // [px] - the full-width add button in the toolbar
constexpr float ToolbarPadding =  7.0f;   // [px] - the toolbar's own inset, narrower than a panel's
constexpr float DropLineHeight =  2.0f;   // [px] - the accent line above the row a release would land on
constexpr float FoldRowHeight  = 26.0f;   // [px] - one row of the folded properties beneath an entry

// 📝 The readout buffer every row prints into. A name, a source caption and a strength do not reach this, and a
//    fixed extent keeps the whole presentation allocation-free.
constexpr std::uint32_t RowTextExtent = 96u;

//------------------------------------------------------------------------------------------------------------------------
//                                                      SMALL GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

WorkspaceRectangle BandOf(const WorkspaceRectangle& Area, float Offset, float Height)
{
    return { Area.PositionX, Area.PositionY + Offset, Area.Width, Height };
}

WorkspaceRectangle InsetBy(const WorkspaceRectangle& Area, float Margin)
{
    return { Area.PositionX + Margin,
             Area.PositionY + Margin,
             Area.Width  - Margin * 2.0f > 0.0f ? Area.Width  - Margin * 2.0f : 0.0f,
             Area.Height - Margin * 2.0f > 0.0f ? Area.Height - Margin * 2.0f : 0.0f };
}

WorkspaceRectangle LeftOf(const WorkspaceRectangle& Area, float Width)
{
    return { Area.PositionX, Area.PositionY, Width < Area.Width ? Width : Area.Width, Area.Height };
}

WorkspaceRectangle RightOf(const WorkspaceRectangle& Area, float Width)
{
    const float Taken = Width < Area.Width ? Width : Area.Width;

    return { Area.PositionX + Area.Width - Taken, Area.PositionY, Taken, Area.Height };
}

WorkspaceRectangle AfterLeft(const WorkspaceRectangle& Area, float Consumed)
{
    const float Taken = Consumed < Area.Width ? Consumed : Area.Width;

    return { Area.PositionX + Taken, Area.PositionY, Area.Width - Taken, Area.Height };
}

WorkspaceRectangle SquareCentred(const WorkspaceRectangle& Area, float Edge)
{
    return { Area.PositionX + (Area.Width  - Edge) * 0.5f,
             Area.PositionY + (Area.Height - Edge) * 0.5f,
             Edge,
             Edge };
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE ROW READINGS
//------------------------------------------------------------------------------------------------------------------------

// 📝 ⚠️ An entry carries no name. `56` §2's four fields are the whole of what a layer declares, and a name is not
//    among them, so the presented name is minted from the panel's own ordinal. A panel that stored a name beside
//    the entry would be storing content outside the document, and the artist's rename would not survive a save.
void PrintRowName(char* Destination, std::uint32_t DestinationExtent, std::uint32_t PresentedOrdinal)
{
    std::snprintf(Destination, DestinationExtent, "Layer %02u", PresentedOrdinal + 1u);
}

// 📝 🚧 The subline reads `{source} · {strength}%`, where the reference reads `{blend} · {opacity}%`. The blend
//    caption needs a routine over `CombineSpecification`, which lives in `Contract/CombineContract.h` and is unread
//    by this component — inventing the enumerators here would be the panel asserting a set the contract owns. The
//    source caption is presented in its place and the substitution is one call wide when the routine arrives.
void PrintRowSubline(char*                     Destination,
                     std::uint32_t             DestinationExtent,
                     const LayerSpecification& Entry)
{
    const double Strength = Entry.Coverage.CoverageDeclared ? Entry.Coverage.UniformStrength : 1.0;
    const double Bounded  = Strength < 0.0 ? 0.0 : (Strength > 1.0 ? 1.0 : Strength);

    std::snprintf(Destination, DestinationExtent, "%s \xC2\xB7 %d%%",
                  CaptionOfSource(Entry.Source),
                  static_cast<int>(Bounded * 100.0 + 0.5));
}

bool RowAdmitted(const LayerPanelCarry& Carry, const char* RowName, const char* Subline)
{
    if (Carry.Filter.CarryExtent == 0u)
        return true;

    // 📝 The reference's filter entry is decorative — it binds to nothing at all. An unwired entry is a defect in
    //    Slate, so it is wired here, over the two runs of text the row actually presents.
    return std::strstr(RowName, Carry.Filter.Carried) != nullptr
        || std::strstr(Subline, Carry.Filter.Carried) != nullptr;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE BANDS
//------------------------------------------------------------------------------------------------------------------------

void PresentHeaderBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       std::uint32_t              EntryCount)
{
    PresentSurfaceFill(Band, Theme.Palette.PanelHeader, 0.0f);
    PresentSurfaceFill({ Band.PositionX, Band.PositionY + Band.Height - Theme.Extents.BorderThickness,
                         Band.Width, Theme.Extents.BorderThickness },
                       Theme.Palette.PanelBorder, 0.0f);

    const WorkspaceRectangle Interior = InsetBy(Band, Theme.Extents.PanelPadding);
    const WorkspaceRectangle GlyphBox = LeftOf(Interior, 24.0f);

    PresentSurfaceFill(GlyphBox, Theme.Palette.TileBackground, Theme.Extents.PillRounding);
    PresentControlStroke(SquareCentred(GlyphBox, Theme.Extents.GlyphEdge),
                         ControlStroke::Image, Theme.Palette.TextPrimary, 1.5f, 0.0f);

    char Counted[RowTextExtent] = {};
    std::snprintf(Counted, RowTextExtent, "%u", EntryCount);

    const float              CountWidth = MeasuredTextExtent(Counted, 1.0f) + Theme.Extents.PanelPadding * 2.0f;
    const WorkspaceRectangle CountBadge = RightOf(Interior, CountWidth);

    PresentSurfaceFill(CountBadge, Theme.Palette.TileBackground, Theme.Extents.EntryRounding);
    PresentTextRun(CountBadge, Counted, Theme.Palette.TextMuted, TextPlacement::Centred, 1.0f);

    WorkspaceRectangle Titles = AfterLeft(Interior, 24.0f + Theme.Extents.ControlSpacing);
    Titles.Width = Titles.Width - CountWidth - Theme.Extents.ControlSpacing;

    // 📝 The title and its subtitle share the band, the title on the upper half and the subtitle beneath it. Two
    //    runs rather than one so the subtitle keeps the muted colour without a second text colour being spelled.
    const WorkspaceRectangle TitleRun    = { Titles.PositionX, Titles.PositionY, Titles.Width, Titles.Height * 0.55f };
    const WorkspaceRectangle SubtitleRun = { Titles.PositionX, Titles.PositionY + Titles.Height * 0.55f,
                                             Titles.Width, Titles.Height * 0.45f };

    PresentTextRun(TitleRun,    "Layers",         Theme.Palette.TextPrimary, TextPlacement::Leading, 1.0f);
    PresentTextRun(SubtitleRun, "Surface content", Theme.Palette.TextMuted,  TextPlacement::Leading, 0.85f);
}

// 🧩 The toolbar: a full-width add row and the filter entry beneath it.
// out  true where the add row was pressed this tick
bool PresentToolbarBand(const ThemeSpecification&  Theme,
                        const WorkspaceRectangle&  Band,
                        LayerPanelCarry&           Carry)
{
    const WorkspaceRectangle Interior = InsetBy(Band, ToolbarPadding);
    const WorkspaceRectangle AddRow   = { Interior.PositionX, Interior.PositionY, Interior.Width, AddRowHeight };

    const ControlInteraction AddPress = ResolveAreaPress(AddRow);

    PresentSurfaceFill(AddRow,
                       AddPress.PointerOver ? Theme.Palette.TileHovered : Theme.Palette.TileBackground,
                       Theme.Extents.PillRounding);
    PresentControlStroke(SquareCentred(LeftOf(AddRow, AddRowHeight), Theme.Extents.GlyphEdge),
                         ControlStroke::Plus, Theme.Palette.TextPrimary, 1.5f, 0.0f);
    PresentTextRun(AddRow, "Add Layer", Theme.Palette.TextPrimary, TextPlacement::Centred, 1.0f);

    const WorkspaceRectangle FilterRow = { Interior.PositionX,
                                           Interior.PositionY + AddRowHeight + Theme.Extents.ControlSpacing,
                                           Interior.Width,
                                           Theme.Extents.SegmentRowHeight };

    // 📝 The filter is a text entry with no label column of its own — the reference gives it the whole toolbar
    //    width and caps it with a search glyph, so the caption is empty and the cap is painted over the field.
    PresentTextEntry(Theme, FilterRow, "", Carry.Filter, "Filter");
    PresentControlStroke(SquareCentred(RightOf(FilterRow, Theme.Extents.SegmentRowHeight), Theme.Extents.GlyphEdge),
                         ControlStroke::Search, Theme.Palette.TextMuted, 1.4f, 0.0f);

    return AddPress.EditSealed;
}

void PresentFooterBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       std::uint32_t              EntryCount,
                       std::uint32_t              PresentedCount,
                       std::uint32_t              MaskedCount)
{
    PresentSurfaceFill(Band, Theme.Palette.PanelHeader, 0.0f);
    PresentSurfaceFill({ Band.PositionX, Band.PositionY, Band.Width, Theme.Extents.BorderThickness },
                       Theme.Palette.PanelBorder, 0.0f);

    char Counted[RowTextExtent] = {};
    std::snprintf(Counted, RowTextExtent, "%u layers \xC2\xB7 %u presented \xC2\xB7 %u masked",
                  EntryCount, PresentedCount, MaskedCount);

    PresentTextRun(InsetBy(Band, Theme.Extents.PanelPadding), Counted,
                   Theme.Palette.TextMuted, TextPlacement::Leading, 0.9f);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE SPINE
//------------------------------------------------------------------------------------------------------------------------

void PresentSpineBadge(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Gutter,
                       std::uint32_t              PresentedOrdinal,
                       bool                       Chosen)
{
    const WorkspaceRectangle Badge = SquareCentred(Gutter, BadgeEdge);

    // 📝 The reference shadows the badge `0 0 0 3px` in the panel's own background so the rail reads as passing
    //    behind it. A shadow is a fill of the background colour at the badge's radius plus three, painted first.
    PresentSurfaceFill(InsetBy(Badge, -3.0f), Theme.Palette.PanelBackground, Theme.Extents.EntryRounding);
    PresentSurfaceFill(Badge,
                       Chosen ? Theme.Palette.AccentPrimary : Theme.Palette.TileBackground,
                       Theme.Extents.EntryRounding);

    char Ordinal[RowTextExtent] = {};
    std::snprintf(Ordinal, RowTextExtent, "%02u", PresentedOrdinal + 1u);

    PresentTextRun(Badge, Ordinal,
                   Chosen ? Theme.Palette.TextOnAccent : Theme.Palette.TextMuted,
                   TextPlacement::Centred, 0.8f);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     ONE SPLIT ROW
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What one row's tick asked the sequence for, applied after the whole run has been walked.
/// note  🔴 Deferred for the same reason `WorkspaceStripInternal.h`'s intent is: `DeclarePresence` and `Reorder`
///        amend the very vector the walk is iterating, and applying inside the walk invalidates it.
struct RowIntent
{
    bool           PresenceDeclared = false;   // [-] - an eye was pressed
    bool           PresenceArriving = false;   // [-] - what it should become
    std::uint32_t  PresenceSubject  = 0u;      // [-] - the sequence position it names
    bool           WithdrawDeclared = false;   // [-] - a row's discard was pressed
    std::uint32_t  WithdrawSubject  = 0u;      // [-] - the sequence position it names
};

void PresentSplitRow(const ThemeSpecification&  Theme,
                     const WorkspaceRectangle&  Row,
                     const LayerSpecification&  Entry,
                     std::uint32_t              SequencePosition,
                     std::uint32_t              PresentedOrdinal,
                     LayerPanelCarry&           Carry,
                     RowIntent&                 Arriving)
{
    const bool Chosen = Carry.ChosenDeclared && Carry.ChosenPosition == SequencePosition;

    const ControlInteraction RowPress = ResolveAreaPress(Row);

    PresentSurfaceFill(Row,
                       Chosen              ? Theme.Palette.AccentSubtle
                     : RowPress.PointerOver ? Theme.Palette.RowHovered
                                            : Theme.Palette.TileBackground,
                       Theme.Extents.PillRounding);

    // 📝 The 50/50 split is the reference's whole layer row: the entry on the left, its coverage on the right, one
    //    hairline between them. The halves are equal by construction and not by a fraction the artist can drag —
    //    a draggable split here would be a second layout authority beside the panel band the row sits in.
    const float              HalfWidth = Row.Width * 0.5f;
    const WorkspaceRectangle LeftHalf  = { Row.PositionX, Row.PositionY, HalfWidth, Row.Height };
    const WorkspaceRectangle RightHalf = { Row.PositionX + HalfWidth, Row.PositionY, HalfWidth, Row.Height };

    PresentSurfaceFill({ Row.PositionX + HalfWidth, Row.PositionY + 6.0f,
                         Theme.Extents.BorderThickness, Row.Height - 12.0f },
                       Theme.Palette.PanelBorder, 0.0f);

    //--------------------------------------------------------------------------------------------------------------
    // the entry half
    //--------------------------------------------------------------------------------------------------------------

    WorkspaceRectangle Walking = InsetBy(LeftHalf, Theme.Extents.ControlSpacing);

    const WorkspaceRectangle TwistyBox = LeftOf(Walking, Theme.Extents.GlyphButtonSmallEdge);
    const bool               FoldStanding =
        PresentedOrdinal < LayerFoldCapacity ? Carry.FoldOpen[PresentedOrdinal] : false;

    if (ResolveAreaPress(TwistyBox).EditSealed && PresentedOrdinal < LayerFoldCapacity)
        Carry.FoldOpen[PresentedOrdinal] = !FoldStanding;

    PresentControlStroke(SquareCentred(TwistyBox, Theme.Extents.GlyphEdge), ControlStroke::Twisty,
                         Theme.Palette.TextMuted, 1.4f, FoldStanding ? 1.5707963f : 0.0f);

    Walking = AfterLeft(Walking, Theme.Extents.GlyphButtonSmallEdge);

    const WorkspaceRectangle EyeBox = LeftOf(Walking, Theme.Extents.GlyphButtonSmallEdge);

    if (ResolveAreaPress(EyeBox).EditSealed)
    {
        Arriving.PresenceDeclared = true;
        Arriving.PresenceArriving = !Entry.PresenceEnabled;
        Arriving.PresenceSubject  = SequencePosition;
    }

    PresentControlStroke(SquareCentred(EyeBox, Theme.Extents.GlyphEdge), ControlStroke::Eye,
                         Entry.PresenceEnabled ? Theme.Palette.TextPrimary : Theme.Palette.TextMuted,
                         1.4f, 0.0f);

    Walking = AfterLeft(Walking, Theme.Extents.GlyphButtonSmallEdge + Theme.Extents.ControlSpacing * 0.5f);

    const WorkspaceRectangle Thumbnail = SquareCentred(LeftOf(Walking, ThumbnailEdge), ThumbnailEdge);

    PresentSurfaceFill(Thumbnail, Theme.Palette.ControlBackground, Theme.Extents.PillRounding);
    PresentControlStroke(SquareCentred(Thumbnail, Theme.Extents.GlyphEdge), ControlStroke::Image,
                         Theme.Palette.TextMuted, 1.2f, 0.0f);

    Walking = AfterLeft(Walking, ThumbnailEdge + Theme.Extents.ControlSpacing);

    char RowName[RowTextExtent] = {};
    char Subline[RowTextExtent] = {};

    PrintRowName(RowName, RowTextExtent, PresentedOrdinal);
    PrintRowSubline(Subline, RowTextExtent, Entry);

    const WorkspaceRectangle DotBox = { Walking.PositionX,
                                        Walking.PositionY + Walking.Height * 0.5f - SubjectDotEdge,
                                        SubjectDotEdge, SubjectDotEdge };

    PresentSurfaceFill(DotBox,
                       Entry.PresenceEnabled ? Theme.Palette.AccentPrimary : Theme.Palette.TextMuted,
                       Theme.Extents.EntryRounding);

    const WorkspaceRectangle NameRun = { Walking.PositionX + SubjectDotEdge + 4.0f,
                                         Walking.PositionY,
                                         Walking.Width - SubjectDotEdge - 4.0f,
                                         Walking.Height * 0.55f };
    const WorkspaceRectangle SubRun  = { Walking.PositionX,
                                         Walking.PositionY + Walking.Height * 0.55f,
                                         Walking.Width,
                                         Walking.Height * 0.45f };

    DeclareClip(LeftHalf);
    PresentTextRun(NameRun, RowName,
                   Entry.PresenceEnabled ? Theme.Palette.TextPrimary : Theme.Palette.TextMuted,
                   TextPlacement::Leading, 1.0f);
    PresentTextRun(SubRun, Subline, Theme.Palette.TextMuted, TextPlacement::Leading, 0.8f);
    ReclaimClip();

    //--------------------------------------------------------------------------------------------------------------
    // the coverage half
    //--------------------------------------------------------------------------------------------------------------

    const WorkspaceRectangle MaskInterior = InsetBy(RightHalf, Theme.Extents.ControlSpacing);

    if (!Entry.Coverage.CoverageDeclared)
    {
        const WorkspaceRectangle Placeholder = SquareCentred(LeftOf(MaskInterior, ThumbnailEdge), ThumbnailEdge);

        PresentSurfaceOutline(Placeholder, Theme.Palette.PanelBorder, Theme.Extents.PillRounding,
                              Theme.Extents.BorderThickness);
        PresentTextRun(AfterLeft(MaskInterior, ThumbnailEdge + Theme.Extents.ControlSpacing),
                       "No Mask", Theme.Palette.TextMuted, TextPlacement::Leading, 0.85f);
    }
    else
    {
        const WorkspaceRectangle MaskEye  = LeftOf(MaskInterior, Theme.Extents.GlyphButtonSmallEdge);
        const WorkspaceRectangle MaskTile = SquareCentred(
            LeftOf(AfterLeft(MaskInterior, Theme.Extents.GlyphButtonSmallEdge), ThumbnailEdge), ThumbnailEdge);

        PresentControlStroke(SquareCentred(MaskEye, Theme.Extents.GlyphEdge), ControlStroke::Eye,
                             Theme.Palette.TextPrimary, 1.4f, 0.0f);

        const double Strength = Entry.Coverage.UniformStrength < 0.0 ? 0.0
                              : (Entry.Coverage.UniformStrength > 1.0 ? 1.0 : Entry.Coverage.UniformStrength);

        // 📝 The reference paints the mask tile at the coverage's own strength, which is a coverage of the surface
        //    beneath and not a second colour. `Attenuate` is the one route to that, so no literal is spelled.
        PresentSurfaceFill(MaskTile, Attenuate(Theme.Palette.TextPrimary, Strength), Theme.Extents.PillRounding);

        char Reading[RowTextExtent] = {};
        std::snprintf(Reading, RowTextExtent, "%d%%", static_cast<int>(Strength * 100.0 + 0.5));

        PresentTextRun(AfterLeft(MaskInterior,
                                 Theme.Extents.GlyphButtonSmallEdge + ThumbnailEdge + Theme.Extents.ControlSpacing),
                       Reading, Theme.Palette.TextMuted, TextPlacement::Leading, 0.85f);

        // 🚧 The reference carries a mask discard here and `SurfaceLayerSequence` carries no route to it —
        //    `CoverageSpecification` is amended as part of an entry and there is no withdrawal call. Presenting a
        //    control that cannot act is worse than presenting none, so the affordance is absent until `56` gains
        //    one. Reported rather than faked with a local copy, which would breach `14` §1 outright.
    }

    const WorkspaceRectangle DiscardBox = RightOf(MaskInterior, Theme.Extents.GlyphButtonSmallEdge);

    if (ResolveAreaPress(DiscardBox).EditSealed)
    {
        Arriving.WithdrawDeclared = true;
        Arriving.WithdrawSubject  = SequencePosition;
    }

    PresentControlStroke(SquareCentred(DiscardBox, Theme.Extents.GlyphEdge), ControlStroke::Trash,
                         Theme.Palette.DangerPrimary, 1.4f, 0.0f);

    //--------------------------------------------------------------------------------------------------------------
    // choosing and taking hold
    //--------------------------------------------------------------------------------------------------------------

    if (RowPress.EditOpened)
    {
        Carry.ChosenDeclared = true;
        Carry.ChosenPosition = SequencePosition;
        Carry.ReorderOpen    = true;
        Carry.ReorderOrigin  = SequencePosition;
        Carry.ReorderLanding = SequencePosition;
    }
}

void PresentFoldedProperties(const ThemeSpecification&  Theme,
                             const WorkspaceRectangle&  Area,
                             const LayerSpecification&  Entry)
{
    PresentSurfaceFill(Area, Theme.Palette.PanelBackground, Theme.Extents.PillRounding);

    char Reading[RowTextExtent] = {};
    std::snprintf(Reading, RowTextExtent, "source %s \xC2\xB7 channels 0x%05X",
                  CaptionOfSource(Entry.Source), Entry.ChannelMask);

    PresentTextRun(InsetBy(Area, Theme.Extents.ControlSpacing), Reading,
                   Theme.Palette.TextMuted, TextPlacement::Leading, 0.8f);
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT A ROW READS
//------------------------------------------------------------------------------------------------------------------------

const char* CaptionOfSource(LayerContentSource Source)
{
    switch (Source)
    {
        case LayerContentSource::PaintedImpressions: return "Painted";
        case LayerContentSource::PlacedContent:      return "Placed";
        case LayerContentSource::Tiling:             return "Tiled";
        case LayerContentSource::AnalyticResolution: return "Resolved";
        case LayerContentSource::NestedSequence:     return "Nested";
        default:                                     return "Unknown";
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

void PresentLayerPanel(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Area,
                       void*                      PresentContext)
{
    LayerPanelContext* Standing = static_cast<LayerPanelContext*>(PresentContext);

    PresentSurfaceFill(Area, Theme.Palette.PanelBackground, Theme.Extents.CornerRounding);

    if (Standing == nullptr || Standing->Sequence == nullptr || Standing->Carry == nullptr)
    {
        PresentTextRun(Area, "No surface", Theme.Palette.TextMuted, TextPlacement::Centred, 1.0f);
        return;
    }

    SurfaceLayerSequence& Sequence = *Standing->Sequence;
    LayerPanelCarry&      Carry    = *Standing->Carry;

    const std::vector<LayerSpecification>& Entries = Sequence.Entries();
    const std::uint32_t                    Counted = static_cast<std::uint32_t>(Entries.size());

    //----------------------------------------------------------------------------------------------------------------
    // the four bands
    //----------------------------------------------------------------------------------------------------------------

    const float ToolbarHeight = ToolbarPadding * 2.0f + AddRowHeight
                              + Theme.Extents.ControlSpacing + Theme.Extents.SegmentRowHeight;

    const WorkspaceRectangle HeaderBand  = BandOf(Area, 0.0f, Theme.Extents.PanelHeaderHeight);
    const WorkspaceRectangle ToolbarBand = BandOf(Area, Theme.Extents.PanelHeaderHeight, ToolbarHeight);
    const WorkspaceRectangle FooterBand  = BandOf(Area, Area.Height - Theme.Extents.PanelFooterHeight,
                                                  Theme.Extents.PanelFooterHeight);
    const WorkspaceRectangle ListBand    = { Area.PositionX,
                                             Area.PositionY + Theme.Extents.PanelHeaderHeight + ToolbarHeight,
                                             Area.Width,
                                             Area.Height - Theme.Extents.PanelHeaderHeight - ToolbarHeight
                                                         - Theme.Extents.PanelFooterHeight };

    std::uint32_t PresentedCount = 0u;
    std::uint32_t MaskedCount    = 0u;

    for (const LayerSpecification& Entry : Entries)
    {
        PresentedCount += Entry.PresenceEnabled ? 1u : 0u;
        MaskedCount    += Entry.Coverage.CoverageDeclared ? 1u : 0u;
    }

    PresentHeaderBand(Theme, HeaderBand, Counted);

    const bool AddDeclared = PresentToolbarBand(Theme, ToolbarBand, Carry);

    if (AddDeclared)
    {
        // 🚧 Minting an entry is `56`'s `Append`, and what it appends — source, channel mask, combination — is a
        //    workspace's declaration and not a panel's. The press is presented and reported through the chosen
        //    position; the workspace that owns the surface performs the append inside its own transaction.
        Carry.ChosenDeclared = true;
        Carry.ChosenPosition = Counted;
    }

    if (ListBand.Height <= 0.0f)
    {
        PresentFooterBand(Theme, FooterBand, Counted, PresentedCount, MaskedCount);
        return;
    }

    //----------------------------------------------------------------------------------------------------------------
    // the scrolled run of rows, topmost first
    //----------------------------------------------------------------------------------------------------------------

    const float RowPitch     = Theme.Extents.LayerRowHeight + Theme.Extents.CardGap;
    float       ContentSpan  = 0.0f;

    for (std::uint32_t Ordinal = 0u; Ordinal < Counted; ++Ordinal)
    {
        const std::uint32_t Position = Counted - 1u - Ordinal;

        char RowName[RowTextExtent] = {};
        char Subline[RowTextExtent] = {};

        PrintRowName(RowName, RowTextExtent, Ordinal);
        PrintRowSubline(Subline, RowTextExtent, Entries[Position]);

        if (!RowAdmitted(Carry, RowName, Subline))
            continue;

        ContentSpan += RowPitch;
        ContentSpan += (Ordinal < LayerFoldCapacity && Carry.FoldOpen[Ordinal]) ? FoldRowHeight : 0.0f;
    }

    AdvanceVisibleOffset(Carry.VisibleOffset, ListBand, ContentSpan);

    RowIntent Arriving = {};

    DeclareClip(ListBand);

    float Walking = ListBand.PositionY - Carry.VisibleOffset;

    for (std::uint32_t Ordinal = 0u; Ordinal < Counted; ++Ordinal)
    {
        const std::uint32_t       Position = Counted - 1u - Ordinal;
        const LayerSpecification& Entry    = Entries[Position];

        char RowName[RowTextExtent] = {};
        char Subline[RowTextExtent] = {};

        PrintRowName(RowName, RowTextExtent, Ordinal);
        PrintRowSubline(Subline, RowTextExtent, Entry);

        if (!RowAdmitted(Carry, RowName, Subline))
            continue;

        const WorkspaceRectangle Gutter = { ListBand.PositionX, Walking, SpineWidth,
                                            Theme.Extents.LayerRowHeight };
        const WorkspaceRectangle Row    = { ListBand.PositionX + SpineWidth,
                                            Walking,
                                            ListBand.Width - SpineWidth - Theme.Extents.PanelPadding,
                                            Theme.Extents.LayerRowHeight };

        // 📝 The rail is painted before the badge so the badge's own shadow reads as the rail passing behind it.
        PresentSurfaceFill({ Gutter.PositionX + (SpineWidth - SpineRail) * 0.5f, Gutter.PositionY,
                             SpineRail, Gutter.Height + Theme.Extents.CardGap },
                           Theme.Palette.PanelBorder, 0.0f);

        PresentSpineBadge(Theme, Gutter, Ordinal,
                          Carry.ChosenDeclared && Carry.ChosenPosition == Position);

        if (Carry.ReorderOpen && Carry.ReorderLanding == Position)
        {
            PresentSurfaceFill({ Row.PositionX, Row.PositionY - DropLineHeight - 1.0f,
                                 Row.Width, DropLineHeight },
                               Theme.Palette.AccentPrimary, 0.0f);
        }

        PresentSplitRow(Theme, Row, Entry, Position, Ordinal, Carry, Arriving);

        Walking += RowPitch;

        if (Ordinal < LayerFoldCapacity && Carry.FoldOpen[Ordinal])
        {
            PresentFoldedProperties(Theme,
                                    { Row.PositionX, Walking - Theme.Extents.CardGap, Row.Width, FoldRowHeight },
                                    Entry);
            Walking += FoldRowHeight;
        }
    }

    ReclaimClip();

    //----------------------------------------------------------------------------------------------------------------
    // the reorder drag, resolved against the pointer and applied on release
    //----------------------------------------------------------------------------------------------------------------

    if (Carry.ReorderOpen)
    {
        float PointerX = 0.0f;
        float PointerY = 0.0f;

        ResolvePointerPosition(PointerX, PointerY);

        const float Travelled = PointerY - (ListBand.PositionY - Carry.VisibleOffset);
        const float Landed    = Travelled / (RowPitch > 0.0f ? RowPitch : 1.0f);

        std::uint32_t PresentedLanding = Landed < 0.0f ? 0u : static_cast<std::uint32_t>(Landed);

        if (PresentedLanding >= Counted && Counted > 0u)
            PresentedLanding = Counted - 1u;

        Carry.ReorderLanding = Counted > 0u ? Counted - 1u - PresentedLanding : 0u;

        if (!PointerHeld())
        {
            if (Carry.ReorderLanding != Carry.ReorderOrigin
             && Carry.ReorderOrigin < Counted)
            {
                // 🔴 One transaction against two sequence positions — `56` §6. The panel asks the sequence to move
                //    the entry and holds no ordering of its own, so the outliner and this panel can never disagree.
                Sequence.Reorder(Entries[Carry.ReorderOrigin].Identity, Carry.ReorderLanding);
            }

            Carry.ReorderOpen = false;
        }
    }

    //----------------------------------------------------------------------------------------------------------------
    // the deferred intent
    //----------------------------------------------------------------------------------------------------------------

    if (Arriving.PresenceDeclared && Arriving.PresenceSubject < Counted)
        Sequence.DeclarePresence(Entries[Arriving.PresenceSubject].Identity, Arriving.PresenceArriving);

    if (Arriving.WithdrawDeclared && Arriving.WithdrawSubject < Counted)
        Sequence.Withdraw(Entries[Arriving.WithdrawSubject].Identity);

    PresentFooterBand(Theme, FooterBand, Counted, PresentedCount, MaskedCount);
}

}   // namespace Slate
