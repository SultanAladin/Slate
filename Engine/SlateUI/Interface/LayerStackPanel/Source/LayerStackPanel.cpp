//============================================================================================================================================
//                                                         LAYERSTACKPANEL.CPP
//============================================================================================================================================
// 🧩 Records the layer stack, the channel panel and the mask panel, primitive for primitive against `References/LayerstackV1.html`.

#include "SlateUI/Interface/LayerStackPanel/Api/LayerStackPanel.h"

#include <cstdio>
#include <cstring>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       SMALL HELPERS
//------------------------------------------------------------------------------------------------------------------------

// 📐 The reference tracks its small capitals at .14em on the header and .1em on a section head.
static constexpr float TrackingHead    = 0.14f;
static constexpr float TrackingSection = 0.10f;

static PlaneExtent Squared(float CentreAlong, float CentreAcross, float Extent)
{
    const float Half = Extent * 0.5f;
    return PlaneExtent{ CentreAlong - Half, CentreAcross - Half, CentreAlong + Half, CentreAcross + Half };
}

static PlaneExtent Inset(const PlaneExtent& Extent, float Along, float Across)
{
    return PlaneExtent{ Extent.LeastAlong + Along, Extent.LeastAcross + Across,
                        Extent.MostAlong  - Along, Extent.MostAcross  - Across };
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Deliver<bool> LayerStackPanel::Construct(RecordingSurface& Recording)
{
    Surface = &Recording;
    return Deliver<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      SHARED FRAGMENTS
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordMeter(const PlaneExtent& Extent, std::uint32_t Reading, InkOrdinate Ink)
{
    // 📐 `.mini` — a 4px trough at .12 coverage with the reading filled over it and rounded to a pill.
    Surface->Ground(Extent, Partial(0xFFFFFFu, 0.12), Scaled.MiniAcross * 0.5f);

    const float Fraction = (Reading > 100u) ? 1.0f : static_cast<float>(Reading) * 0.01f;

    if (Fraction > 0.0f)
    {
        PlaneExtent Filled = Extent;
        Filled.MostAlong   = Extent.LeastAlong + Extent.SpanAlong() * Fraction;
        Surface->Ground(Filled, Ink, Scaled.MiniAcross * 0.5f);
    }
}

void LayerStackPanel::RecordChip(const PlaneExtent& Extent, const char* Caption, InkOrdinate Ink, bool Solid)
{
    // 📐 `.chip` — an 18px pill, either a .07-coverage ground with a stroke or a solid tint carrying black text.
    const float Radius = Extent.SpanAcross() * 0.5f;

    if (Solid)
    {
        Surface->Ground(Extent, Ink, Radius);
    }
    else
    {
        Surface->Ground(Extent, Partial(0xFFFFFFu, 0.07), Radius);
        Surface->Edge(Extent, Tinted.Stroke, 1.0f, Radius);
    }

    const InkOrdinate Written = Solid ? Covering(0x000000u) : Ink;
    const float       Along   = Extent.LeastAlong + (Extent.SpanAlong() -
                                Surface->MeasureRun(Caption, Scaled.RunFine, 0.04f)) * 0.5f;

    Surface->TextRun(Along, Extent.LeastAcross + (Extent.SpanAcross() - Surface->RunAcross(Scaled.RunFine)) * 0.5f,
                     Written, Caption, Scaled.RunFine, 0.04f, true);
}

void LayerStackPanel::RecordSectionHead(const PlaneExtent& Extent, const char* Caption, const char* Reading,
                                        bool Opened)
{
    // 📐 `.sech` — a chevron, a tracked small-capital caption and an optional trailing reading.
    const float Middle = (Extent.LeastAcross + Extent.MostAcross) * 0.5f;

    Surface->Stroke(Opened ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                    Squared(Extent.LeastAlong + 6.0f, Middle, 11.0f), Tinted.Faint);

    Surface->TextRunCapitalised(Extent.LeastAlong + 18.0f, Middle - Surface->RunAcross(Scaled.RunSection) * 0.5f,
                                Tinted.Secondary, Caption, Scaled.RunSection, TrackingSection, true);

    if (Reading != nullptr && Reading[0] != '\0')
    {
        const float Along = Extent.MostAlong - Surface->MeasureRun(Reading, Scaled.RunFine);
        Surface->TextRun(Along, Middle - Surface->RunAcross(Scaled.RunFine) * 0.5f,
                         Tinted.Faint, Reading, Scaled.RunFine);
    }
}

float LayerStackPanel::RecordReadingRow(const PlaneExtent& Extent, const char* Caption, const char* Reading)
{
    // 📐 `.d` — a faint caption on the leading edge and its reading on the trailing one.
    const float Middle   = Extent.LeastAcross + Scaled.FieldAcross * 0.5f;
    const float Baseline = Middle - Surface->RunAcross(Scaled.RunSub) * 0.5f;

    Surface->TextRun(Extent.LeastAlong, Baseline, Tinted.Faint, Caption, Scaled.RunSub);

    const float Along = Extent.MostAlong - Surface->MeasureRun(Reading, Scaled.RunSub);
    Surface->TextRunTruncated(Along, Baseline, Extent.SpanAlong() * 0.6f, Tinted.Primary,
                              Reading, Scaled.RunSub, true);

    return Extent.LeastAcross + Scaled.FieldAcross;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                          ONE ROW
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordEntryRow(const PlaneExtent& Extent, const LayerArrangement& Arrangement,
                                     std::uint32_t Ordinal, bool Taken, bool Hovered)
{
    const LayerEntry& Entry  = Arrangement.Entries[Ordinal];
    const bool        Folder = (Entry.Content == LayerContent::Folder);
    const float       Middle = (Extent.LeastAcross + Extent.MostAcross) * 0.5f;

    // ① The ground, which the reference tints by taken, then hovered, then standing.
    // 📐 `/* ── ROW (square) ── */` — the entry row carries no radius at all.
    const InkOrdinate Ground = Taken ? Tinted.RowTaken : (Hovered ? Tinted.RowHovered : Tinted.Row);
    Surface->Ground(Extent, Ground, 0.0f);

    if (Taken)
        Surface->Edge(Extent, Tinted.StrokeStrong, 1.0f, 0.0f);

    // ② `.tag` — the colour tag, a 3px spine down the leading edge.
    PlaneExtent Tag = Extent;
    Tag.MostAlong   = Extent.LeastAlong + Scaled.TagAlong;
    Surface->Ground(Tag, Covering(Entry.ColourTag), Scaled.TagAlong * 0.5f);

    float Along = Extent.LeastAlong + Scaled.RowPadAlong;

    // ③ `.tw` — the disclosure twisty, drawn only on a folder.
    if (Folder)
        Surface->Stroke(Entry.Opened ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                        Squared(Along + Scaled.DiscloseAlong * 0.5f, Middle, 12.0f), Tinted.Secondary);

    Along += Scaled.DiscloseAlong + Scaled.RowGapAlong * 0.5f;

    // ④ The eye, dimmed while the entry is hidden.
    Surface->Stroke(Entry.Shown ? SymbolSubject::EyeOpen : SymbolSubject::EyeClosed,
                    Squared(Along + Scaled.ActionExtent * 0.5f, Middle, 12.5f),
                    Entry.Shown ? Tinted.Secondary : Tinted.Faint);

    Along += Scaled.ActionExtent + Scaled.RowGapAlong;

    // ⑤ `.thumb` — the preview disc and its classification badge.
    // 📐 `.disc` states `border-radius:0` and a 1px inset ring; the fill is the whole square.
    const PlaneExtent Thumb = Squared(Along + Scaled.ThumbExtent * 0.5f, Middle, Scaled.ThumbExtent);
    Surface->Ground(Thumb, ContentTint(Entry.Content), 0.0f);
    Surface->Edge(Thumb, Tinted.StrokeStrong, 1.0f, 0.0f);

    // 📐 `.badge` — 15px square hung 3px past the trailing and lower edges, on --r-s.
    const PlaneExtent Badge = Spanning(Thumb.MostAlong - Scaled.BadgeExtent + 3.0f,
                                       Thumb.MostAcross - Scaled.BadgeExtent + 3.0f,
                                       Scaled.BadgeExtent, Scaled.BadgeExtent);
    Surface->Ground(Badge, Covering(0x000000u), Scaled.RadiusSmall);
    Surface->Edge(Badge, Tinted.StrokeStrong, 1.0f, Scaled.RadiusSmall);

    const char* const BadgeRun = ContentBadge(Entry.Content);
    Surface->TextRun(Badge.LeastAlong + (Scaled.BadgeExtent - Surface->MeasureRun(BadgeRun, 9.0f)) * 0.5f,
                     Badge.LeastAcross + (Scaled.BadgeExtent - Surface->RunAcross(9.0f)) * 0.5f,
                     Tinted.Secondary, BadgeRun, 9.0f, 0.0f, true);

    Along += Scaled.ThumbExtent + Scaled.RowGapAlong;

    // ⑥ `.chips` — measured before the meta, because `.meta{flex:1}` yields to whatever the chips take.
    struct ChipDeclaration { char Caption[16]; InkOrdinate Ink; bool Solid; };
    ChipDeclaration Declared[5] = {};
    std::uint32_t   ChipCount   = 0u;

    const auto Declare = [&](const char* Caption, InkOrdinate Ink, bool Solid)
    {
        if (ChipCount >= 5u)
            return;
        std::snprintf(Declared[ChipCount].Caption, sizeof Declared[ChipCount].Caption, "%s", Caption);
        Declared[ChipCount].Ink   = Ink;
        Declared[ChipCount].Solid = Solid;
        ++ChipCount;
    };

    // 📝 The reference orders them 3D, L, MASK, N FX, n/8 CH, leading to trailing.
    if (Entry.Content == LayerContent::Decal)
        Declare("3D", Tinted.Accent, true);

    if (Entry.Secured)
        Declare("L", Tinted.Danger, false);

    if (Entry.Mask.Declared)
        Declare("MASK", Tinted.Caution, false);

    if (Entry.EffectCount > 0u)
    {
        char Counted[16] = {};
        std::snprintf(Counted, sizeof Counted, "%u FX", Entry.EffectCount);
        Declare(Counted, Tinted.Affirm, false);
    }

    if (!Folder)
    {
        const std::uint32_t Enabled = ChannelsEnabled(Entry);

        if (Enabled != LayerStackCeiling::Channels)
        {
            char Counted[16] = {};
            std::snprintf(Counted, sizeof Counted, "%u/8 CH", Enabled);
            Declare(Counted, Tinted.Secondary, false);
        }
    }

    float ChipsAlong = 0.0f;

    for (std::uint32_t Chip = 0u; Chip < ChipCount; ++Chip)
        ChipsAlong += Surface->MeasureRun(Declared[Chip].Caption, Scaled.RunFine, 0.04f) + 14.0f + 3.0f;

    // ⑦ `body.wide .col` — the two columns seat only once the panel reaches 580px.
    const bool  Columns     = Extent.SpanAlong() >= Scaled.ColumnsLeast;
    const float ColumnsSpan = Columns ? Scaled.BlendColumnAlong + Scaled.OpacityColumnAlong : 0.0f;
    const float ChipsSeat   = Extent.MostAlong - Scaled.ActionExtent * 2.0f - ChipsAlong;
    const float ColumnsSeat = ChipsSeat - ColumnsSpan;

    // ⑧ `.meta` — the naming over its reading line, both truncated to whatever extent is left.
    const float MetaCeiling = ColumnsSeat - Along - Scaled.RowGapAlong;

    if (MetaCeiling > 8.0f)
    {
        Surface->TextRunTruncated(Along, Middle - Surface->RunAcross(Scaled.RunRow) - 1.0f, MetaCeiling,
                                  Entry.Shown ? Tinted.Primary : Tinted.Faint, Entry.Naming,
                                  Scaled.RunRow, true);

        char Reading[96] = {};

        // 📝 `rowSub` reads the resolution and format once the columns carry blend and opacity instead.
        if (Folder)
            std::snprintf(Reading, sizeof Reading, "%u items  %s",
                          EnclosedCount(Arrangement, Ordinal), Entry.Blend);
        else if (Columns)
            std::snprintf(Reading, sizeof Reading, "%s  %upx  %s",
                          ContentNaming(Entry.Content), Entry.Resolution, Entry.Format);
        else
            std::snprintf(Reading, sizeof Reading, "%s  %s  %u%%",
                          ContentNaming(Entry.Content), Entry.Blend, Entry.Opacity);

        Surface->TextRunTruncated(Along, Middle + 1.0f, MetaCeiling, Tinted.Faint, Reading, Scaled.RunSub);
    }

    if (Columns)
    {
        // ⑨ `.col-blend` — an em dash on a folder, its blend otherwise.
        Surface->TextRunTruncated(ColumnsSeat, Middle - Surface->RunAcross(Scaled.RunSub) * 0.5f,
                                  Scaled.BlendColumnAlong - 6.0f, Tinted.Secondary,
                                  Folder ? "\xE2\x80\x94" : Entry.Blend, Scaled.RunSub);

        // ⑩ `.col-op` — the meter takes the slack, the reading its stated 32px on the trailing edge.
        const float MeterSeat  = ColumnsSeat + Scaled.BlendColumnAlong;
        const float MeterAlong = Scaled.OpacityColumnAlong - Scaled.OpacityReadAlong - Scaled.ColumnGapAlong;
        RecordMeter(Spanning(MeterSeat, Middle - Scaled.MiniAcross * 0.5f, MeterAlong, Scaled.MiniAcross),
                    Entry.Opacity, Tinted.Accent);

        char Percent[8] = {};
        std::snprintf(Percent, sizeof Percent, "%u%%", Entry.Opacity);
        Surface->TextRun(MeterSeat + Scaled.OpacityColumnAlong - Surface->MeasureRun(Percent, Scaled.RunSub),
                         Middle - Surface->RunAcross(Scaled.RunSub) * 0.5f,
                         Tinted.Secondary, Percent, Scaled.RunSub);
    }

    // ⑪ The chips themselves, seated leading to trailing across the run just measured.
    float ChipSeat = ChipsSeat;

    for (std::uint32_t Chip = 0u; Chip < ChipCount; ++Chip)
    {
        const float ChipAlong = Surface->MeasureRun(Declared[Chip].Caption, Scaled.RunFine, 0.04f) + 14.0f;
        RecordChip(Spanning(ChipSeat, Middle - Scaled.ChipAcross * 0.5f, ChipAlong, Scaled.ChipAcross),
                   Declared[Chip].Caption, Declared[Chip].Ink, Declared[Chip].Solid);
        ChipSeat += ChipAlong + 3.0f;
    }

    // ⑩ The unfold chevron and the menu, both on the trailing edge.
    Surface->Stroke(Entry.Unfolded ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                    Squared(Extent.MostAlong - Scaled.ActionExtent * 1.5f, Middle, 12.0f), Tinted.Faint);

    Surface->Medallion(Extent.MostAlong - Scaled.ActionExtent * 0.5f, Middle - 4.0f, 1.3f, Tinted.Faint);
    Surface->Medallion(Extent.MostAlong - Scaled.ActionExtent * 0.5f, Middle,        1.3f, Tinted.Faint);
    Surface->Medallion(Extent.MostAlong - Scaled.ActionExtent * 0.5f, Middle + 4.0f, 1.3f, Tinted.Faint);
}

void LayerStackPanel::RecordMaskRow(const PlaneExtent& Extent, const LayerEntry& Entry, bool Taken, bool Hovered)
{
    // 📐 `.row.msk` — attached beneath its entry, indented, shorter, and without a colour tag or twisty.
    const MaskOrdinate& Mask = Entry.Mask;
    const float      Middle = (Extent.LeastAcross + Extent.MostAcross) * 0.5f;

    const InkOrdinate Ground = Taken ? Tinted.RowTaken : (Hovered ? Tinted.RowHovered : Tinted.Detail);
    Surface->Ground(Extent, Ground, 0.0f);
    Surface->Edge(Extent, Taken ? Tinted.StrokeStrong : Tinted.Stroke, 1.0f, 0.0f);

    float Along = Extent.LeastAlong + Scaled.RowPadAlong;

    Surface->Stroke(Mask.Shown ? SymbolSubject::EyeOpen : SymbolSubject::EyeClosed,
                    Squared(Along + Scaled.ActionExtent * 0.5f, Middle, 11.5f),
                    Mask.Shown ? Tinted.Secondary : Tinted.Faint);

    Along += Scaled.ActionExtent + Scaled.RowGapAlong;

    // 📐 The mini preview, which the reference inverts in place when the mask is inverted.
    const PlaneExtent Thumb = Squared(Along + Scaled.ThumbMini * 0.5f, Middle, Scaled.ThumbMini);
    Surface->Ground(Thumb, Mask.Inverted ? Covering(0x2A2A2Au) : Covering(0xC8C8C8u), 0.0f);
    Surface->Edge(Thumb, Tinted.StrokeStrong, 1.0f, 0.0f);

    Along += Scaled.ThumbMini + Scaled.RowGapAlong;

    Surface->TextRun(Along, Middle - Surface->RunAcross(Scaled.RunSub) - 1.0f, Tinted.Secondary,
                     "Mask", Scaled.RunSub, 0.0f, true);

    char Reading[96] = {};
    const char* Source = (Mask.Source == MaskSource::Generator && Mask.Generator != nullptr)
                       ? Mask.Generator : SourceNaming(Mask.Source);

    if (Mask.Inverted)
        std::snprintf(Reading, sizeof Reading, "%s  Gray 8  %u%%  INV", Source, Mask.Density);
    else
        std::snprintf(Reading, sizeof Reading, "%s  Gray 8  %u%%", Source, Mask.Density);

    // 📐 `.chips` — the mask row declares at most the effect count.
    char  Counted[16] = {};
    float ChipsAlong  = 0.0f;

    if (Mask.EffectCount > 0u)
    {
        std::snprintf(Counted, sizeof Counted, "%u FX", Mask.EffectCount);
        ChipsAlong = Surface->MeasureRun(Counted, Scaled.RunFine, 0.04f) + 14.0f + 3.0f;
    }

    const bool  Columns     = Extent.SpanAlong() >= Scaled.ColumnsLeast;
    const float ColumnsSpan = Columns ? Scaled.BlendColumnAlong + Scaled.OpacityColumnAlong : 0.0f;
    const float ChipsSeat   = Extent.MostAlong - Scaled.ActionExtent * 2.0f - ChipsAlong;
    const float ColumnsSeat = ChipsSeat - ColumnsSpan;
    const float MetaCeiling = ColumnsSeat - Along - Scaled.RowGapAlong;

    if (MetaCeiling > 8.0f)
        Surface->TextRunTruncated(Along, Middle + 1.0f, MetaCeiling, Tinted.Faint, Reading, Scaled.RunFine);

    if (Columns)
    {
        // 📐 `clips <naming>` in the blend column, truncated at fourteen characters by the reference.
        char Clips[48] = {};
        std::snprintf(Clips, sizeof Clips, "clips %.14s", Entry.Naming);
        Surface->TextRunTruncated(ColumnsSeat, Middle - Surface->RunAcross(Scaled.RunFine) * 0.5f,
                                  Scaled.BlendColumnAlong - 6.0f, Tinted.Faint, Clips, Scaled.RunFine);

        const float MeterSeat  = ColumnsSeat + Scaled.BlendColumnAlong;
        const float MeterAlong = Scaled.OpacityColumnAlong - Scaled.OpacityReadAlong - Scaled.ColumnGapAlong;
        RecordMeter(Spanning(MeterSeat, Middle - Scaled.MiniAcross * 0.5f, MeterAlong, Scaled.MiniAcross),
                    Mask.Density, Tinted.Secondary);

        char Percent[8] = {};
        std::snprintf(Percent, sizeof Percent, "%u%%", Mask.Density);
        Surface->TextRun(MeterSeat + Scaled.OpacityColumnAlong - Surface->MeasureRun(Percent, Scaled.RunSub),
                         Middle - Surface->RunAcross(Scaled.RunSub) * 0.5f,
                         Tinted.Secondary, Percent, Scaled.RunSub);
    }

    if (Mask.EffectCount > 0u)
        RecordChip(Spanning(ChipsSeat, Middle - Scaled.ChipAcross * 0.5f, ChipsAlong - 3.0f, Scaled.ChipAcross),
                   Counted, Tinted.Affirm, false);

    Surface->Stroke(Mask.Unfolded ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                    Squared(Extent.MostAlong - Scaled.ActionExtent * 1.5f, Middle, 11.0f), Tinted.Faint);

    Surface->Medallion(Extent.MostAlong - Scaled.ActionExtent * 0.5f, Middle - 3.5f, 1.2f, Tinted.Faint);
    Surface->Medallion(Extent.MostAlong - Scaled.ActionExtent * 0.5f, Middle,        1.2f, Tinted.Faint);
    Surface->Medallion(Extent.MostAlong - Scaled.ActionExtent * 0.5f, Middle + 3.5f, 1.2f, Tinted.Faint);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE STACK
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordStack(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                                  LayerStackOrdinates& Seated)
{
    if (Surface == nullptr || !Surface->Recording())
        return;

    Surface->Ground(Extent, Tinted.Panel);
    Surface->Confine(Extent);

    // ① `.head` — the caption and the entry count.
    const PlaneExtent Head = Spanning(Extent.LeastAlong, Extent.LeastAcross, Extent.SpanAlong(), Scaled.HeadAcross);
    Surface->Ground(Head, Tinted.PanelRaised);
    Surface->Edge(PlaneExtent{ Head.LeastAlong, Head.MostAcross - 1.0f, Head.MostAlong, Head.MostAcross },
                  Tinted.Stroke);

    Surface->TextRunCapitalised(Head.LeastAlong + Scaled.HeadPadAlong,
                                Head.LeastAcross + (Scaled.HeadAcross - Surface->RunAcross(Scaled.RunHead)) * 0.5f,
                                Tinted.Secondary, "Layers", Scaled.RunHead, TrackingHead, true);

    char Counted[24] = {};
    std::snprintf(Counted, sizeof Counted, "%u", Arrangement.EntryCount);

    const float CountAlong = Surface->MeasureRun(Counted, Scaled.RunFine) + 18.0f;
    RecordChip(Spanning(Head.MostAlong - Scaled.HeadPadAlong - CountAlong,
                        Head.LeastAcross + (Scaled.HeadAcross - 20.0f) * 0.5f, CountAlong, 20.0f),
               Counted, Tinted.Secondary, false);

    // ② `.tools` — the search field and its trailing actions.
    const PlaneExtent Tools = Spanning(Extent.LeastAlong, Head.MostAcross, Extent.SpanAlong(), Scaled.ToolsAcross);
    Surface->Edge(PlaneExtent{ Tools.LeastAlong, Tools.MostAcross - 1.0f, Tools.MostAlong, Tools.MostAcross },
                  Tinted.Stroke);

    const float       ActionsAlong = Scaled.ButtonExtent * 3.0f + 10.0f;
    const PlaneExtent Search       = Spanning(Tools.LeastAlong + Scaled.ToolsPadAlong,
                                              Tools.LeastAcross + (Scaled.ToolsAcross - Scaled.SearchAcross) * 0.5f,
                                              Tools.SpanAlong() - Scaled.ToolsPadAlong * 2.0f - ActionsAlong,
                                              Scaled.SearchAcross);

    Surface->Ground(Search, Covering(0x000000u), Scaled.SearchAcross * 0.5f);
    Surface->Edge(Search, Tinted.Stroke, 1.0f, Scaled.SearchAcross * 0.5f);
    Surface->Stroke(SymbolSubject::MagnifierLens,
                    Squared(Search.LeastAlong + 17.0f, (Search.LeastAcross + Search.MostAcross) * 0.5f, 13.0f),
                    Tinted.Faint);
    Surface->TextRun(Search.LeastAlong + 28.0f,
                     (Search.LeastAcross + Search.MostAcross) * 0.5f - Surface->RunAcross(12.0f) * 0.5f,
                     Tinted.Faint, "Search layers", 12.0f);

    const float ActionMiddle = (Tools.LeastAcross + Tools.MostAcross) * 0.5f;

    Surface->Stroke(SymbolSubject::PlusCross,
                    Squared(Tools.MostAlong - Scaled.ToolsPadAlong - Scaled.ButtonExtent * 2.5f,
                            ActionMiddle, 15.0f), Tinted.Secondary);
    Surface->Stroke(SymbolSubject::LayerMerge,
                    Squared(Tools.MostAlong - Scaled.ToolsPadAlong - Scaled.ButtonExtent * 1.5f,
                            ActionMiddle, 15.0f), Tinted.Secondary);
    Surface->Stroke(SymbolSubject::TrashBin,
                    Squared(Tools.MostAlong - Scaled.ToolsPadAlong - Scaled.ButtonExtent * 0.5f,
                            ActionMiddle, 15.0f), Tinted.Secondary);

    // ③ `.stack` — the scrolling run of rows, confined so an overflowing row is clipped and not drawn over
    //    the footer.
    const PlaneExtent Foot  = Spanning(Extent.LeastAlong, Extent.MostAcross - Scaled.FootAcross,
                                       Extent.SpanAlong(), Scaled.FootAcross);
    const PlaneExtent Stack = PlaneExtent{ Extent.LeastAlong, Tools.MostAcross,
                                           Extent.MostAlong,  Foot.LeastAcross };

    Surface->Confine(Stack);

    const float RowAlong  = Stack.SpanAlong() - Scaled.StackPadAlong * 2.0f - Scaled.ScrollAlong;
    float       Across    = Stack.LeastAcross + Scaled.StackPadAcross - Seated.StackOffset;
    const float Pointer   = Surface->Pointer().PositionAcross;
    const float PointerAt = Surface->Pointer().PositionAlong;

    std::uint32_t Hovered     = 0xFFFFFFFFu;
    bool          HoveredMask = false;

    for (std::uint32_t Ordinal = 0u; Ordinal < Arrangement.EntryCount; ++Ordinal)
    {
        if (!EntryPresented(Arrangement, Ordinal))
            continue;

        const LayerEntry& Entry  = Arrangement.Entries[Ordinal];
        const float       Indent = static_cast<float>(Entry.Depth) * Scaled.RowStepAlong;

        const PlaneExtent Row = Spanning(Stack.LeastAlong + Scaled.StackPadAlong + Indent, Across,
                                         RowAlong - Indent, Scaled.RowAcross);

        if (!Surface->Excluded(Row))
        {
            const bool Over = Row.Encloses(PointerAt, Pointer) && Stack.Encloses(PointerAt, Pointer);

            if (Over)
            {
                Hovered     = Ordinal;
                HoveredMask = false;
            }

            RecordEntryRow(Row, Arrangement,  Ordinal,
                           Arrangement.Taken == Ordinal && Arrangement.TakenHalf == LayerTaken::Layer, Over);
        }

        Across += Scaled.RowAcross + 4.0f;

        // 📐 `.attach` — the mask row, drawn immediately beneath its entry and indented past the thumb.
        if (Entry.Mask.Declared)
        {
            const PlaneExtent MaskRow = Spanning(Stack.LeastAlong + Scaled.StackPadAlong + Indent +
                                                 Scaled.MaskLeadAlong, Across,
                                                 RowAlong - Indent - Scaled.MaskLeadAlong, Scaled.MaskRowAcross);

            if (!Surface->Excluded(MaskRow))
            {
                const bool Over = MaskRow.Encloses(PointerAt, Pointer) && Stack.Encloses(PointerAt, Pointer);

                if (Over)
                {
                    Hovered     = Ordinal;
                    HoveredMask = true;
                }

                RecordMaskRow(MaskRow, Entry,
                              Arrangement.Taken == Ordinal && Arrangement.TakenHalf == LayerTaken::Mask, Over);
            }

            Across += Scaled.MaskRowAcross + 4.0f;
        }
    }

    Seated.StackSpan   = (Across + Seated.StackOffset) - (Stack.LeastAcross + Scaled.StackPadAcross);
    Seated.Hovered     = Hovered;
    Seated.HoveredMask = HoveredMask;

    Surface->Release();

    // ④ The scroll bar, recorded only when the run overflows its extent.
    const float Visible = Stack.SpanAcross();

    if (Seated.StackSpan > Visible && Visible > 0.0f)
    {
        const float Fraction = Visible / Seated.StackSpan;
        const float ThumbAcross = (Visible * Fraction < 28.0f) ? 28.0f : Visible * Fraction;
        const float Travel      = Visible - ThumbAcross;
        const float Ceiling     = Seated.StackSpan - Visible;
        const float Advanced    = (Ceiling > 0.0f) ? (Seated.StackOffset / Ceiling) : 0.0f;

        Surface->Ground(Spanning(Stack.MostAlong - Scaled.ScrollAlong + 3.0f,
                                 Stack.LeastAcross + Travel * Advanced, 4.0f, ThumbAcross),
                        Partial(0xFFFFFFu, 0.15), 2.0f);
    }

    // ⑤ The wheel, which the stack answers itself because the seam carries no scrolling primitive.
    if (Stack.Encloses(PointerAt, Pointer))
    {
        Seated.StackOffset -= Surface->Pointer().WheelAcross * 48.0f;

        const float Ceiling = (Seated.StackSpan > Visible) ? (Seated.StackSpan - Visible) : 0.0f;

        if (Seated.StackOffset < 0.0f)      Seated.StackOffset = 0.0f;
        if (Seated.StackOffset > Ceiling)   Seated.StackOffset = Ceiling;
    }

    // ⑥ What the artist takes, resolved on the contact's arriving edge.
    if (Surface->Pointer().ContactArrived && Hovered < Arrangement.EntryCount)
    {
        Arrangement.Taken     = Hovered;
        Arrangement.TakenHalf = HoveredMask ? LayerTaken::Mask : LayerTaken::Layer;
    }

    // ⑦ `.foot` — the breadcrumb over the taken entry's blend and opacity.
    Surface->Ground(Foot, Tinted.PanelRaised);
    Surface->Edge(PlaneExtent{ Foot.LeastAlong, Foot.LeastAcross, Foot.MostAlong, Foot.LeastAcross + 1.0f },
                  Tinted.Stroke);

    if (Arrangement.Taken < Arrangement.EntryCount)
    {
        const LayerEntry& Taken = Arrangement.Entries[Arrangement.Taken];

        char Crumb[128] = {};
        std::snprintf(Crumb, sizeof Crumb, "%s  /  %s%s", ContentNaming(Taken.Content), Taken.Naming,
                      Arrangement.TakenHalf == LayerTaken::Mask ? "  /  Mask" : "");

        Surface->TextRunTruncated(Foot.LeastAlong + Scaled.FootPadAlong, Foot.LeastAcross + 9.0f,
                                  Foot.SpanAlong() - Scaled.FootPadAlong * 2.0f, Tinted.Faint,
                                  Crumb, Scaled.RunFine);

        // 📐 `.blend` — the pill that opens the blend menu, capped at 52% of the footer.
        const float       BlendAlong = Foot.SpanAlong() * 0.52f;
        const PlaneExtent Blend      = Spanning(Foot.LeastAlong + Scaled.FootPadAlong,
                                                Foot.LeastAcross + 26.0f, BlendAlong, 27.0f);

        Surface->Ground(Blend, Partial(0xFFFFFFu, 0.06), 13.5f);
        Surface->Edge(Blend, Tinted.Stroke, 1.0f, 13.5f);
        Surface->TextRunTruncated(Blend.LeastAlong + 13.0f,
                                  Blend.LeastAcross + (27.0f - Surface->RunAcross(11.0f)) * 0.5f,
                                  BlendAlong - 32.0f, Tinted.Primary, Taken.Blend, 11.0f, true);
        Surface->Stroke(SymbolSubject::ChevronDown,
                        Squared(Blend.MostAlong - 12.0f, Blend.LeastAcross + 13.5f, 11.0f), Tinted.Faint);

        // 📐 `.op` — the opacity meter that fills the footer's trailing half.
        const float MeterLeast = Blend.MostAlong + 10.0f;
        const float MeterMost  = Foot.MostAlong - Scaled.FootPadAlong - 34.0f;

        if (MeterMost > MeterLeast)
            RecordMeter(PlaneExtent{ MeterLeast, Blend.LeastAcross + 12.0f, MeterMost,
                                     Blend.LeastAcross + 15.0f }, Taken.Opacity, Tinted.Accent);

        char Percent[8] = {};
        std::snprintf(Percent, sizeof Percent, "%u%%", Taken.Opacity);
        Surface->TextRun(Foot.MostAlong - Scaled.FootPadAlong -
                         Surface->MeasureRun(Percent, Scaled.RunSub),
                         Blend.LeastAcross + 13.5f - Surface->RunAcross(Scaled.RunSub) * 0.5f,
                         Tinted.Secondary, Percent, Scaled.RunSub, 0.0f, true);
    }

    Surface->Release();
    Seated.ContactPrior = Surface->Pointer().ContactHeld;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE CHANNEL PROPERTIES
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordChannelProperties(const PlaneExtent& Extent, const LayerArrangement& Arrangement)
{
    if (Surface == nullptr || !Surface->Recording())
        return;

    Surface->Ground(Extent, Tinted.Panel);
    Surface->Confine(Extent);

    const LayerEntry& Entry = Arrangement.Entries[
        (Arrangement.Taken < Arrangement.EntryCount) ? Arrangement.Taken : 0u];

    // ① The head — the naming, its reading and the classification medallion.
    const PlaneExtent Head = Spanning(Extent.LeastAlong, Extent.LeastAcross, Extent.SpanAlong(), 52.0f);
    Surface->Ground(Head, Tinted.PanelRaised);
    Surface->Edge(PlaneExtent{ Head.LeastAlong, Head.MostAcross - 1.0f, Head.MostAlong, Head.MostAcross },
                  Tinted.Stroke);

    const PlaneExtent Medallion = Squared(Head.LeastAlong + 24.0f, Head.LeastAcross + 26.0f, 26.0f);
    Surface->Ground(Medallion, ContentTint(Entry.Content), Scaled.RadiusSmall);
    Surface->Stroke(SymbolSubject::ChannelSelect, Inset(Medallion, 6.0f, 6.0f), Covering(0x000000u));

    Surface->TextRunTruncated(Head.LeastAlong + 46.0f, Head.LeastAcross + 11.0f,
                              Head.SpanAlong() - 60.0f, Tinted.Primary, Entry.Naming, 13.0f, true);

    char Reading[96] = {};
    std::snprintf(Reading, sizeof Reading, "%s  %upx  %s",
                  ContentNaming(Entry.Content), Entry.Resolution, Entry.Format);
    Surface->TextRun(Head.LeastAlong + 46.0f, Head.LeastAcross + 28.0f, Tinted.Faint, Reading, Scaled.RunFine);

    // ② The chips region — one chip per enabled channel, tinted with its own hue.
    float Across = Head.MostAcross;

    const PlaneExtent ChipsHead = Spanning(Extent.LeastAlong, Across, Extent.SpanAlong(), Scaled.SectionAcross);
    char ChipsReading[24] = {};
    std::snprintf(ChipsReading, sizeof ChipsReading, "%u", ChannelsEnabled(Entry));
    RecordSectionHead(Inset(ChipsHead, Scaled.CardPadAlong, 0.0f), "Channels", ChipsReading, true);
    Across += Scaled.SectionAcross;

    float ChipAlong = Extent.LeastAlong + Scaled.CardPadAlong;

    for (std::uint32_t Channel = 0u; Channel < LayerStackCeiling::Channels; ++Channel)
    {
        if (!Entry.Channels[Channel].Enabled)
            continue;

        const char* Caption = ChannelNaming()[Channel];
        const float Along   = Surface->MeasureRun(Caption, Scaled.RunFine) + 26.0f;

        if (ChipAlong + Along > Extent.MostAlong - Scaled.CardPadAlong)
        {
            ChipAlong = Extent.LeastAlong + Scaled.CardPadAlong;
            Across   += 22.0f;
        }

        const PlaneExtent Chip = Spanning(ChipAlong, Across, Along, 18.0f);
        Surface->Ground(Chip, Partial(0xFFFFFFu, 0.05), 9.0f);
        Surface->Edge(Chip, Tinted.Stroke, 1.0f, 9.0f);
        Surface->Medallion(Chip.LeastAlong + 10.0f, Across + 9.0f, 3.5f, ChannelTint(Channel));
        Surface->TextRun(Chip.LeastAlong + 18.0f, Across + 9.0f - Surface->RunAcross(Scaled.RunFine) * 0.5f,
                         Tinted.Secondary, Caption, Scaled.RunFine);

        ChipAlong += Along + 5.0f;
    }

    Across += 30.0f;

    // ③ One panel per channel — a dot, its naming, its blend and its opacity meter.
    const PlaneExtent BlendingHead = Spanning(Extent.LeastAlong, Across, Extent.SpanAlong(), Scaled.SectionAcross);
    RecordSectionHead(Inset(BlendingHead, Scaled.CardPadAlong, 0.0f), "Channel Blending", nullptr, true);
    Across += Scaled.SectionAcross + 2.0f;

    for (std::uint32_t Channel = 0u; Channel < LayerStackCeiling::Channels; ++Channel)
    {
        const ChannelOrdinate& Reading8 = Entry.Channels[Channel];
        const PlaneExtent      Row      = Spanning(Extent.LeastAlong + Scaled.CardPadAlong, Across,
                                                   Extent.SpanAlong() - Scaled.CardPadAlong * 2.0f, 28.0f);

        if (Surface->Excluded(Row))
        {
            Across += 30.0f;
            continue;
        }

        Surface->Ground(Row, Reading8.Enabled ? Tinted.Row : Tinted.Detail, Scaled.RadiusSmall);

        const float Middle = Row.LeastAcross + 14.0f;
        Surface->Medallion(Row.LeastAlong + 12.0f, Middle, 4.0f,
                           Reading8.Enabled ? ChannelTint(Channel) : Tinted.Faint);

        Surface->TextRunTruncated(Row.LeastAlong + 22.0f, Middle - Surface->RunAcross(Scaled.RunSub) * 0.5f,
                                  108.0f, Reading8.Enabled ? Tinted.Primary : Tinted.Faint,
                                  ChannelNaming()[Channel], Scaled.RunSub, true);

        Surface->TextRunTruncated(Row.LeastAlong + 132.0f, Middle - Surface->RunAcross(Scaled.RunFine) * 0.5f,
                                  Row.SpanAlong() - 132.0f - 74.0f, Tinted.Secondary,
                                  Reading8.Blend, Scaled.RunFine);

        RecordMeter(PlaneExtent{ Row.MostAlong - 66.0f, Middle - 1.5f, Row.MostAlong - 30.0f, Middle + 1.5f },
                    Reading8.Opacity, Reading8.Enabled ? ChannelTint(Channel) : Tinted.Faint);

        char Percent[8] = {};
        std::snprintf(Percent, sizeof Percent, "%u%%", Reading8.Opacity);
        Surface->TextRun(Row.MostAlong - 26.0f, Middle - Surface->RunAcross(Scaled.RunFine) * 0.5f,
                         Tinted.Faint, Percent, Scaled.RunFine);

        Across += 30.0f;
    }

    // ④ The foot — how many channels stand against how many atlases the arrangement covers.
    const PlaneExtent Foot = Spanning(Extent.LeastAlong, Extent.MostAcross - 26.0f, Extent.SpanAlong(), 26.0f);
    Surface->Ground(Foot, Tinted.PanelRaised);
    Surface->Edge(PlaneExtent{ Foot.LeastAlong, Foot.LeastAcross, Foot.MostAlong, Foot.LeastAcross + 1.0f },
                  Tinted.Stroke);

    char Footing[48] = {};
    std::snprintf(Footing, sizeof Footing, "%u channels  \xC2\xB7  %u atlases",
                  ChannelsEnabled(Entry), LayerStackCeiling::AtlasTotal);
    Surface->TextRun(Foot.LeastAlong + Scaled.FootPadAlong,
                     Foot.LeastAcross + (26.0f - Surface->RunAcross(Scaled.RunFine)) * 0.5f,
                     Tinted.Faint, Footing, Scaled.RunFine);

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE MASK PROPERTIES
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordMaskProperties(const PlaneExtent& Extent, const LayerArrangement& Arrangement)
{
    if (Surface == nullptr || !Surface->Recording())
        return;

    Surface->Ground(Extent, Tinted.Panel);
    Surface->Confine(Extent);

    const LayerEntry& Entry = Arrangement.Entries[
        (Arrangement.Taken < Arrangement.EntryCount) ? Arrangement.Taken : 0u];
    const MaskOrdinate&  Mask  = Entry.Mask;

    // ① The head.
    const PlaneExtent Head = Spanning(Extent.LeastAlong, Extent.LeastAcross, Extent.SpanAlong(), 52.0f);
    Surface->Ground(Head, Tinted.PanelRaised);
    Surface->Edge(PlaneExtent{ Head.LeastAlong, Head.MostAcross - 1.0f, Head.MostAlong, Head.MostAcross },
                  Tinted.Stroke);

    const PlaneExtent Medallion = Squared(Head.LeastAlong + 24.0f, Head.LeastAcross + 26.0f, 26.0f);
    Surface->Ground(Medallion, Mask.Inverted ? Covering(0x2A2A2Au) : Covering(0xC8C8C8u), Scaled.RadiusSmall);
    Surface->Stroke(SymbolSubject::MaskStencil, Inset(Medallion, 6.0f, 6.0f), Covering(0x000000u));

    Surface->TextRun(Head.LeastAlong + 46.0f, Head.LeastAcross + 11.0f, Tinted.Primary, "Mask", 13.0f, 0.0f, true);

    char Reading[96] = {};
    std::snprintf(Reading, sizeof Reading, "clips %.18s", Entry.Naming);
    Surface->TextRun(Head.LeastAlong + 46.0f, Head.LeastAcross + 28.0f, Tinted.Faint, Reading, Scaled.RunFine);

    if (!Mask.Declared)
    {
        Surface->TextRun(Extent.LeastAlong + Scaled.CardPadAlong, Head.MostAcross + 18.0f, Tinted.Faint,
                         "This layer carries no mask.", Scaled.RunSub);
        Surface->Release();
        return;
    }

    float Across = Head.MostAcross + 2.0f;

    // ② Source — what the mask reads from, and the two switches beside it.
    const PlaneExtent SourceHead = Spanning(Extent.LeastAlong, Across, Extent.SpanAlong(), Scaled.SectionAcross);
    RecordSectionHead(Inset(SourceHead, Scaled.CardPadAlong, 0.0f), "Source",
                      SourceNaming(Mask.Source), true);
    Across += Scaled.SectionAcross;

    const PlaneExtent Body = PlaneExtent{ Extent.LeastAlong + Scaled.CardPadAlong, Across,
                                          Extent.MostAlong  - Scaled.CardPadAlong, Across + Scaled.FieldAcross };

    Across = RecordReadingRow(Body, "Source", SourceNaming(Mask.Source));

    if (Mask.Generator != nullptr)
        Across = RecordReadingRow(PlaneExtent{ Body.LeastAlong, Across, Body.MostAlong,
                                               Across + Scaled.FieldAcross }, "Generator", Mask.Generator);

    char Density[8] = {};
    std::snprintf(Density, sizeof Density, "%u%%", Mask.Density);
    Across = RecordReadingRow(PlaneExtent{ Body.LeastAlong, Across, Body.MostAlong,
                                           Across + Scaled.FieldAcross }, "Density", Density);
    Across = RecordReadingRow(PlaneExtent{ Body.LeastAlong, Across, Body.MostAlong,
                                           Across + Scaled.FieldAcross }, "Blend", Mask.Blend);

    // 📐 The invert switch — a 26×14 pill whose knob sits on whichever side the reading names.
    {
        const float       Middle = Across + Scaled.FieldAcross * 0.5f;
        const PlaneExtent Switch = Spanning(Body.MostAlong - 26.0f, Middle - 7.0f, 26.0f, 14.0f);

        Surface->TextRun(Body.LeastAlong, Middle - Surface->RunAcross(Scaled.RunSub) * 0.5f,
                         Tinted.Faint, "Invert", Scaled.RunSub);
        Surface->Ground(Switch, Mask.Inverted ? Tinted.Accent : Partial(0xFFFFFFu, 0.09), 7.0f);
        Surface->Medallion(Mask.Inverted ? Switch.MostAlong - 6.0f : Switch.LeastAlong + 6.0f,
                           Middle, 5.0f, Mask.Inverted ? Covering(0x000000u) : Tinted.Secondary);

        Across += Scaled.FieldAcross;
    }

    // ③ Parameters — every reading the source declares, each as a caption, a meter and its reading.
    if (Mask.ParameterCount > 0u)
    {
        const PlaneExtent ParameterHead = Spanning(Extent.LeastAlong, Across + 4.0f, Extent.SpanAlong(),
                                                   Scaled.SectionAcross);
        char ParameterReading[16] = {};
        std::snprintf(ParameterReading, sizeof ParameterReading, "%u", Mask.ParameterCount);
        RecordSectionHead(Inset(ParameterHead, Scaled.CardPadAlong, 0.0f), "Parameters",
                          ParameterReading, true);
        Across += Scaled.SectionAcross + 4.0f;

        for (std::uint32_t Ordinal = 0u; Ordinal < Mask.ParameterCount; ++Ordinal)
        {
            const ParameterOrdinate& Parameter = Mask.Parameters[Ordinal];
            const float              Middle    = Across + Scaled.FieldAcross * 0.5f;

            Surface->TextRunTruncated(Body.LeastAlong, Middle - Surface->RunAcross(Scaled.RunSub) * 0.5f,
                                      96.0f, Tinted.Faint, Parameter.Naming, Scaled.RunSub);

            if (Parameter.Selected != nullptr)
            {
                Surface->TextRunTruncated(Body.LeastAlong + 104.0f,
                                          Middle - Surface->RunAcross(Scaled.RunSub) * 0.5f,
                                          Body.SpanAlong() - 104.0f, Tinted.Primary,
                                          Parameter.Selected, Scaled.RunSub, true);
            }
            else if (Parameter.Toggling)
            {
                const PlaneExtent Switch = Spanning(Body.MostAlong - 26.0f, Middle - 7.0f, 26.0f, 14.0f);
                const bool        Standing = Parameter.Standing > 0.5;

                Surface->Ground(Switch, Standing ? Tinted.Accent : Partial(0xFFFFFFu, 0.09), 7.0f);
                Surface->Medallion(Standing ? Switch.MostAlong - 6.0f : Switch.LeastAlong + 6.0f,
                                   Middle, 5.0f, Standing ? Covering(0x000000u) : Tinted.Secondary);
            }
            else
            {
                const double Span     = Parameter.Most - Parameter.Least;
                const double Fraction = (Span > 0.0) ? ((Parameter.Standing - Parameter.Least) / Span) : 0.0;

                RecordMeter(PlaneExtent{ Body.LeastAlong + 104.0f, Middle - 1.5f,
                                         Body.MostAlong - 44.0f, Middle + 1.5f },
                            static_cast<std::uint32_t>(Fraction * 100.0), Tinted.Accent);

                char Written[24] = {};
                std::snprintf(Written, sizeof Written, "%.0f%s", Parameter.Standing, Parameter.Unit);
                Surface->TextRun(Body.MostAlong - Surface->MeasureRun(Written, Scaled.RunFine),
                                 Middle - Surface->RunAcross(Scaled.RunFine) * 0.5f,
                                 Tinted.Secondary, Written, Scaled.RunFine);
            }

            Across += Scaled.FieldAcross;
        }
    }

    // ④ Mesh Map Inputs — one chip per map, marked by whether its transfer stands.
    if (Mask.MeshMapCount > 0u)
    {
        const PlaneExtent MapHead = Spanning(Extent.LeastAlong, Across + 4.0f, Extent.SpanAlong(),
                                             Scaled.SectionAcross);
        RecordSectionHead(Inset(MapHead, Scaled.CardPadAlong, 0.0f), "Mesh Map Inputs", nullptr, true);
        Across += Scaled.SectionAcross + 4.0f;

        float ChipAlong  = Body.LeastAlong;
        bool  AnyAbsent  = false;

        for (std::uint32_t Ordinal = 0u; Ordinal < Mask.MeshMapCount; ++Ordinal)
        {
            const bool  Transferred = Mask.MeshMapTransferred[Ordinal];
            const char* Caption     = Mask.MeshMaps[Ordinal];
            const float Along       = Surface->MeasureRun(Caption, Scaled.RunFine) + 24.0f;

            if (!Transferred)
                AnyAbsent = true;

            if (ChipAlong + Along > Body.MostAlong)
            {
                ChipAlong = Body.LeastAlong;
                Across   += 22.0f;
            }

            const PlaneExtent Chip = Spanning(ChipAlong, Across, Along, 18.0f);
            Surface->Ground(Chip, Partial(0xFFFFFFu, 0.05), 9.0f);
            Surface->Edge(Chip, Transferred ? Tinted.Stroke : Partial(0xFF6B63u, 0.35), 1.0f, 9.0f);
            Surface->TextRun(Chip.LeastAlong + 8.0f, Across + 9.0f - Surface->RunAcross(Scaled.RunFine) * 0.5f,
                             Transferred ? Tinted.Secondary : Tinted.Danger, Caption, Scaled.RunFine);
            Surface->Stroke(Transferred ? SymbolSubject::CubeSolid : SymbolSubject::PlusCross,
                            Squared(Chip.MostAlong - 9.0f, Across + 9.0f, 8.0f),
                            Transferred ? Tinted.Affirm : Tinted.Danger,
                            Transferred ? 0.0f : 0.785398f);

            ChipAlong += Along + 5.0f;
        }

        Across += 24.0f;

        if (AnyAbsent)
        {
            Surface->TextRun(Body.LeastAlong, Across, Tinted.Danger, "Transfer missing", Scaled.RunFine);
            Across += 18.0f;
        }
    }

    // ⑤ Applies To Channels — the eight chips, dimmed where the mask does not reach.
    const PlaneExtent AppliesHead = Spanning(Extent.LeastAlong, Across + 4.0f, Extent.SpanAlong(),
                                             Scaled.SectionAcross);
    RecordSectionHead(Inset(AppliesHead, Scaled.CardPadAlong, 0.0f), "Applies To Channels", nullptr, true);
    Across += Scaled.SectionAcross + 4.0f;

    float AppliesAlong = Body.LeastAlong;

    for (std::uint32_t Channel = 0u; Channel < LayerStackCeiling::Channels; ++Channel)
    {
        const bool  Reached = Mask.ChannelApplied[Channel];
        const char* Caption = ChannelNaming()[Channel];
        const float Along   = Surface->MeasureRun(Caption, Scaled.RunFine) + 16.0f;

        if (AppliesAlong + Along > Body.MostAlong)
        {
            AppliesAlong = Body.LeastAlong;
            Across      += 22.0f;
        }

        const PlaneExtent Chip = Spanning(AppliesAlong, Across, Along, 18.0f);
        Surface->Ground(Chip, Reached ? Partial(0xFFFFFFu, 0.90) : Tinted.Detail, 9.0f);
        Surface->Edge(Chip, Tinted.Stroke, 1.0f, 9.0f);
        // 📐 `.chip.tog.on{background:rgba(255,255,255,.9);color:#000}` — a seated chip inverts its run.
        Surface->TextRun(Chip.LeastAlong + 8.0f, Across + 9.0f - Surface->RunAcross(Scaled.RunFine) * 0.5f,
                         Reached ? Covering(0x000000u) : Tinted.Faint, Caption, Scaled.RunFine);

        AppliesAlong += Along + 5.0f;
    }

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE REVISIONS
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordRevisions(const PlaneExtent& Extent)
{
    if (Surface == nullptr || !Surface->Recording())
        return;

    Surface->Ground(Extent, Tinted.Panel);
    Surface->Confine(Extent);

    const PlaneExtent Head = Spanning(Extent.LeastAlong, Extent.LeastAcross, Extent.SpanAlong(),
                                      Scaled.HeadAcross);
    Surface->Ground(Head, Tinted.PanelRaised);
    Surface->Edge(PlaneExtent{ Head.LeastAlong, Head.MostAcross - 1.0f, Head.MostAlong, Head.MostAcross },
                  Tinted.Stroke);

    // 📝 The caption the reference draws, which is not the spelling the identifiers carry.
    Surface->TextRunCapitalised(Head.LeastAlong + Scaled.HeadPadAlong,
                                Head.LeastAcross + (Scaled.HeadAcross - Surface->RunAcross(Scaled.RunHead)) * 0.5f,
                                Tinted.Secondary, "History", Scaled.RunHead, TrackingHead, true);

    const RevisionOrdinate* Revisions = nullptr;
    std::uint32_t           Count     = 0u;
    SeatReferenceRevisions(Revisions, Count);

    float Across = Head.MostAcross + Scaled.StackPadAcross;

    if (Count == 0u || Revisions == nullptr)
    {
        Surface->TextRun(Extent.LeastAlong + Scaled.CardPadAlong, Across + 14.0f, Tinted.Faint,
                         "No revisions recorded.", Scaled.RunSub);
        Surface->Release();
        return;
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
    {
        const RevisionOrdinate& Revision = Revisions[Ordinal];
        const PlaneExtent       Row      = Spanning(Extent.LeastAlong + Scaled.StackPadAlong, Across,
                                                    Extent.SpanAlong() - Scaled.StackPadAlong * 2.0f, 40.0f);

        if (Surface->Excluded(Row))
        {
            Across += 44.0f;
            continue;
        }

        Surface->Ground(Row, Tinted.Row, Scaled.RadiusSmall);

        // 📐 The standing revision is the newest, which the reference marks with an accent spine.
        if (Ordinal == 0u)
        {
            PlaneExtent Spine = Row;
            Spine.MostAlong   = Row.LeastAlong + 3.0f;
            Surface->Ground(Spine, Tinted.Accent, 1.5f);
        }

        Surface->TextRunTruncated(Row.LeastAlong + 12.0f, Row.LeastAcross + 7.0f,
                                  Row.SpanAlong() - 24.0f, Tinted.Primary,
                                  Revision.Naming, Scaled.RunSub, true);

        Surface->TextRun(Row.LeastAlong + 12.0f, Row.LeastAcross + 22.0f, Tinted.Faint,
                         Revision.Moment, Scaled.RunFine);

        if (Revision.Detail != nullptr && Revision.Detail[0] != '\0')
        {
            const float Along = Row.MostAlong - 12.0f - Surface->MeasureRun(Revision.Detail, Scaled.RunFine);
            Surface->TextRun(Along, Row.LeastAcross + 22.0f, Tinted.Secondary, Revision.Detail, Scaled.RunFine);
        }

        Across += 44.0f;
    }

    Surface->Release();
}

}   // namespace Slate
