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

// 📐 What the reference's own transitions run at. `.row` states `transition:background .12s`, and the
//    popup and the card both state `.16s`.
static constexpr double RouseOver = 120.0;   // [ms]
static constexpr double TakeOver  = 160.0;   // [ms]

// 📐 `.stack` scrolls three lines a notch, which at a 45 px row is what the reference's own wheel gives.
static constexpr float NotchAcross = 48.0f;   // [px]

// 📐 A contact that has travelled beyond this is a carry and never a take — `GestureTolerance` states the
//    same six pixels for the same reason, and the reference's HTML drag has the window system's own.
static constexpr float CarryFloor = 6.0f;   // [px]

// 📐 `renderHistory()`'s own columns and heights. The card is `min-height: 44px`; the two fixed columns to
//    its left are 32 px of medallion and 15 px of spine, which puts the spine's centre at 39 and the card's
//    leading edge at 47. The fold is the comment field and the value field over the author line.
static constexpr float RevisionCardAcross = 44.0f;   // [px] - one folded card
static constexpr float RevisionFoldAcross = 96.0f;   // [px] - author line, comment field, value field
static constexpr float RevisionGapAcross  =  4.0f;   // [px] - `pb-[4px]`
static constexpr float RevisionLeadAlong  = 55.0f;   // [px] - 32 + 15 + `pl-[8px]`
static constexpr float RevisionSpineAlong = 39.0f;   // [px] - 32 + 15/2, the spine's own centre

//------------------------------------------------------------------------------------------------------------------------
//                                                        CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::Reseat(const AppearanceSpecification& Resolved)
{
    Tinted = Resolved.LayerStack;
}

Deliver<bool> LayerStackPanel::Construct(InteractionIndex& Interaction, RecordingSurface& Recording)
{
    if (Ledger != nullptr)
    {
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported,
                                       "the layer stack panel is already constructed" });
    }

    Ledger  = &Interaction;
    Surface = &Recording;

    // 🔴 Every identity claimed here and none inside a tick. The three runs are claimed in one pass so a
    //    refusal partway through retires the whole construction rather than leaving half a panel enrolled.
    const auto Claim = [&](ControlIdentity* Written, std::uint32_t Count) -> Deliver<bool>
    {
        for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
        {
            const Deliver<ControlIdentity> Issued = Interaction.Enrol();

            if (!Issued.ContentPresent)
            {
                Reset();
                return Deliver<bool>::Refuse(Issued.Declined);
            }

            Written[Ordinal] = Issued.Resolve();
        }

        return Deliver<bool>::Deliver(true);
    };

    if (const auto Verdict = Claim(RowCells, RowCeiling * CellsPerRow); !Verdict.ContentPresent)
        return Verdict;

    if (const auto Verdict = Claim(ChromeCells, ChromeCeiling); !Verdict.ContentPresent)
        return Verdict;

    if (const auto Verdict = Claim(PopupEntries, PopupEntryCeiling); !Verdict.ContentPresent)
        return Verdict;

    if (const auto Verdict = Claim(RevisionCells, RevisionCellCeiling); !Verdict.ContentPresent)
        return Verdict;

    return Deliver<bool>::Deliver(true);
}

void LayerStackPanel::Advance(const PointerCondition& Contact, double)
{
    Sampled = Contact;
}

void LayerStackPanel::Reset()
{
    Ledger  = nullptr;
    Surface = nullptr;
    Sampled = {};

    for (ControlIdentity& Claimed : RowCells)
        Claimed = {};

    for (ControlIdentity& Claimed : ChromeCells)
        Claimed = {};

    for (ControlIdentity& Claimed : PopupEntries)
        Claimed = {};

    for (ControlIdentity& Claimed : RevisionCells)
        Claimed = {};
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      ONE ARBITRATION
//------------------------------------------------------------------------------------------------------------------------

bool LayerStackPanel::Roused(const PlaneExtent& Extent) const
{
    if (Surface == nullptr)
        return false;

    // 📐 A confined extent is what actually decides: a row scrolled past the stack's edge still encloses
    //    the pointer arithmetically, and rousing it would light a row nobody can see.
    if (Surface->Excluded(Extent))
        return false;

    return Extent.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);
}

bool LayerStackPanel::Pressed(ControlIdentity Claimed, const PlaneExtent& Extent,
                              LayerStackOrdinates& Seated, const char* Tooltip)
{
    if (Ledger == nullptr)
        return false;

    const bool Over = Roused(Extent);

    // 📐 `[data-tip]` — the tooltip follows the rouse and is recorded in the deferred sweep, seated at the
    //    roused control's own upper edge exactly as the reference's `getBoundingClientRect` places it.
    if (Over && Tooltip != nullptr)
    {
        Seated.Tooltip       = Tooltip;
        Seated.TooltipAlong  = (Extent.LeastAlong + Extent.MostAlong) * 0.5f;
        Seated.TooltipAcross = Extent.LeastAcross;
    }

    // 🔴 A standing popup outranks every row beneath it. Without this the same contact that dismisses a
    //    menu also presses whatever the menu was covering.
    if (Over && Sampled.ContactArrived && !Ledger->AnyDisclosed())
        Ledger->Seize(Claimed, ControlPart::Body);

    Ledger->DeclareRoused(Claimed, Over, RouseOver);

    return Over && Ledger->Released(Claimed);
}

bool LayerStackPanel::Dragged(ControlIdentity Claimed, const PlaneExtent& Extent, std::uint32_t& Reading)
{
    if (Ledger == nullptr || Extent.SpanAlong() <= 0.0f)
        return false;

    const bool Over = Roused(Extent);

    if (Over && Sampled.ContactArrived && !Ledger->AnyDisclosed())
    {
        Ledger->Seize(Claimed, ControlPart::Track);
        Ledger->DepartFrom(Claimed, static_cast<float>(Reading));
    }

    Ledger->DeclareRoused(Claimed, Over, RouseOver);

    if (!Ledger->Holding(Claimed))
        return false;

    // 📐 The reading follows the pointer's ABSOLUTE position along the track rather than an accumulated
    //    per-tick delta, which drifts by a pixel for every tick the pointer spent outside the extent.
    const float Fraction = (Sampled.PositionAlong - Extent.LeastAlong) / Extent.SpanAlong();
    const float Clamped  = (Fraction < 0.0f) ? 0.0f : ((Fraction > 1.0f) ? 1.0f : Fraction);
    const auto  Resolved = static_cast<std::uint32_t>(Clamped * 100.0f + 0.5f);

    if (Resolved == Reading)
        return false;

    Reading = Resolved;
    return true;
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
//                                                    THE DROP MARK
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordDropMark(const PlaneExtent& Extent, DropIntent Intent)
{
    // 📐 `.drop-before` and `.drop-after` state a 2px accent rule; `.drop-into` states a ring around the
    //    whole folder row, which is what makes the two readings unmistakable at a glance.
    switch (Intent)
    {
        case DropIntent::Prior:
            Surface->Ground(PlaneExtent{ Extent.LeastAlong, Extent.LeastAcross - 1.0f,
                                         Extent.MostAlong,  Extent.LeastAcross + 1.0f }, Tinted.Accent);
            break;

        case DropIntent::Trailing:
            Surface->Ground(PlaneExtent{ Extent.LeastAlong, Extent.MostAcross - 1.0f,
                                         Extent.MostAlong,  Extent.MostAcross + 1.0f }, Tinted.Accent);
            break;

        case DropIntent::Enclosed:
            Surface->Edge(Extent, Tinted.Accent, 2.0f, Scaled.RadiusSmall);
            break;

        default:
            break;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE POPUP SHELL
//------------------------------------------------------------------------------------------------------------------------

// 📐 `.pop` — a 200px raised card on --r, strokes at .18, and 6px of padding.
static constexpr float PopupAlongLeast  = 208.0f;   // [px]
static constexpr float PopupEntryAcross =  26.0f;   // [px] - one entry
static constexpr float PopupPad         =   6.0f;   // [px]
static constexpr float PopupCaption     =  22.0f;   // [px] - one `<h6>`

PlaneExtent LayerStackPanel::RecordPopupGround(LayerStackOrdinates& Seated, float Along, float Across,
                                               float Span)
{
    const DisplayCondition& Display = Surface->Display();

    // 📐 `show()` seats the card against its anchor and then clamps it inside the display on both axes,
    //    flipping it above the anchor when it would otherwise run past the lower edge.
    const float Extent  = Display.ExtentAlong  > 0.0f ? Display.ExtentAlong  : 1920.0f;
    const float Across0 = Display.ExtentAcross > 0.0f ? Display.ExtentAcross : 1080.0f;

    float Seat = Along - PopupAlongLeast;

    if (Seat < 8.0f)                            Seat = 8.0f;
    if (Seat > Extent - PopupAlongLeast - 8.0f) Seat = Extent - PopupAlongLeast - 8.0f;

    // 📐 `if(y+p.height>innerHeight-8)y=Math.max(8,at.top-p.height-6)` — flipped against the card's OWN
    //    measured height and not against a fixed probe, which left a long run hanging off the lower edge.
    float Upper = Across;

    if (Upper + Span > Across0 - 8.0f)
        Upper = (Across0 - 8.0f - Span > 8.0f) ? (Across0 - 8.0f - Span) : 8.0f;

    Seated.PopupSeatAlong  = Seat;
    Seated.PopupSeatAcross = Upper;

    return PlaneExtent{ Seat, Upper, Seat + PopupAlongLeast, Upper };
}

bool LayerStackPanel::RecordPopupEntry(const PlaneExtent& Extent, const char* Caption, const char* Chord,
                                       bool Marked, bool Dangerous, LayerStackOrdinates& Seated)
{
    if (Surface->Excluded(Extent))
        return false;

    // 📝 A popup entry is arbitrated against the standing disclosure rather than through `Pressed`, which
    //    refuses everything while a popup is open — that refusal is exactly what keeps rows underneath a
    //    menu from answering, and the menu's own entries have to sit on the other side of it.
    const bool Over = Extent.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

    if (Over)
        Surface->Ground(Extent, Partial(0xFFFFFFu, 0.07), Scaled.RadiusSmall);

    const float Middle = (Extent.LeastAcross + Extent.MostAcross) * 0.5f;

    // 📐 The check occupies its 14px cell whether or not it is drawn, so the captions align down the run.
    if (Marked)
        Surface->Stroke(SymbolSubject::ChevronRight, Squared(Extent.LeastAlong + 15.0f, Middle, 10.0f),
                        Tinted.Accent);

    Surface->TextRunTruncated(Extent.LeastAlong + 26.0f, Middle - Surface->RunAcross(Scaled.RunSub) * 0.5f,
                              Extent.SpanAlong() - 34.0f - (Chord != nullptr ? 30.0f : 0.0f),
                              Dangerous ? Tinted.Danger : Tinted.Primary, Caption, Scaled.RunSub);

    if (Chord != nullptr && Chord[0] != '\0')
    {
        Surface->TextRun(Extent.MostAlong - 8.0f - Surface->MeasureRun(Chord, Scaled.RunFine),
                         Middle - Surface->RunAcross(Scaled.RunFine) * 0.5f, Tinted.Faint,
                         Chord, Scaled.RunFine);
    }

    // 📐 A popup resolves on the RELEASE, and only once it has stood for a whole tick — the contact that
    //    opened it is itself a release, and would otherwise pick whatever entry landed under the pointer.
    const bool Taken = Over && Sampled.ContactReleased && Seated.PopupSettled;

    if (Taken)
    {
        Seated.Popup = StackPopup::Absent;

        if (Ledger != nullptr)
            Ledger->Withdraw();
    }

    return Taken;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE CHORDS
//------------------------------------------------------------------------------------------------------------------------

bool LayerStackPanel::AdmitChord(KeySubject Subject, const ModifierCondition& Modifiers,
                                 LayerArrangement& Arrangement, LayerStackOrdinates& Seated,
                                 RevisionSequence& Revisions)
{
    // 🔴 The reference's own first line — `if(e.target.tagName==='INPUT'||…)return`. A chord that reached
    //    the arrangement while the artist was typing would declare a paint layer out of the letter `p`.
    //    The revision card's comment and value fields are `INPUT` and `TEXTAREA` on exactly those grounds.
    if (Seated.RetentionRoused || Seated.Renaming != LayerStackCeiling::AbsentOrdinal ||
        Seated.RevisionField != 0u)
    {
        return false;
    }

    const bool Taken = Arrangement.Taken < Arrangement.EntryCount;

    // 📐 Every amendment records its own revision FIRST, exactly as `snap()` is called ahead of the
    //    mutation and never after it.
    const auto Amend = [&](const char* Naming) { Revisions.Record(Arrangement, Naming); };

    if (Modifiers.Commanded)
    {
        switch (Subject)
        {
            // ① The two revision chords, the only ones that READ the ring rather than record into it.
            case KeySubject::Revert:
                if (Modifiers.Shifted)
                    Revisions.Reinstate(Arrangement);
                else
                    Revisions.Revert(Arrangement);

                return true;

            case KeySubject::DeclareDecal:                      // 📐 ⌘D — duplicate, not "declare a decal"
                if (!Taken) return true;
                Amend("Entry copied");
                if (!DuplicateTaken(Arrangement)) Revisions.Revert(Arrangement);
                return true;

            case KeySubject::DeclareFolder:                     // 📐 ⌘G — group
                if (!Taken) return true;
                Amend("Entries grouped");
                if (!EncloseTaken(Arrangement)) Revisions.Revert(Arrangement);
                return true;

            case KeySubject::StepPrior:                         // 📐 ⌘↑ — carry, not step
                if (!Taken) return true;
                Amend("Entry carried");
                if (!CarryTaken(Arrangement, false)) Revisions.Revert(Arrangement);
                return true;

            case KeySubject::StepNext:                          // 📐 ⌘↓
                if (!Taken) return true;
                Amend("Entry carried");
                if (!CarryTaken(Arrangement, true)) Revisions.Revert(Arrangement);
                return true;

            default:
                // 📐 `else if(m)return` — a commanded chord this panel does not answer is swallowed rather
                //    than falling through to its unmodified reading, which would declare a layer from ⌘P.
                return false;
        }
    }

    switch (Subject)
    {
        // ② The seven declarations.
        case KeySubject::DeclarePaint:
            Amend("Paint layer declared");
            DeclareEntry(Arrangement, LayerContent::Paint, "Paint Layer");
            return true;

        case KeySubject::DeclareFill:
            Amend("Fill layer declared");
            DeclareEntry(Arrangement, LayerContent::Fill, "Fill Layer");
            return true;

        case KeySubject::DeclareAdjustment:
            Amend("Adjustment declared");
            DeclareEntry(Arrangement, LayerContent::Adjustment, "Adjustment");
            return true;

        case KeySubject::DeclareRetention:
            Amend("Retention declared");
            DeclareEntry(Arrangement, LayerContent::Retention, "Filter");
            return true;

        case KeySubject::DeclareDecal:
            Amend("Decal layer declared");
            DeclareEntry(Arrangement, LayerContent::Decal, "Decal Layer");
            return true;

        case KeySubject::DeclarePattern:
            Amend("Pattern layer declared");
            DeclareEntry(Arrangement, LayerContent::Pattern, "Pattern Layer");
            return true;

        case KeySubject::DeclareFolder:
            Amend("Folder declared");
            DeclareEntry(Arrangement, LayerContent::Folder, "New Folder");
            return true;

        // ③ What one taken entry answers.
        case KeySubject::Retire:
            if (!Taken) return true;
            Amend("Entry retired");
            if (!RetireTaken(Arrangement)) Revisions.Revert(Arrangement);
            return true;

        case KeySubject::AttachMask:
            if (!Taken) return true;
            Amend("Mask amended");
            if (!ToggleMask(Arrangement)) Revisions.Revert(Arrangement);
            return true;

        case KeySubject::Secure:
            if (!Taken) return true;
            Amend("Entry secured");
            Arrangement.Entries[Arrangement.Taken].Secured =
                !Arrangement.Entries[Arrangement.Taken].Secured;
            return true;

        case KeySubject::Conceal:
            if (!Taken) return true;
            Amend("Presence amended");
            Arrangement.Entries[Arrangement.Taken].Shown =
                !Arrangement.Entries[Arrangement.Taken].Shown;
            return true;

        // 📐 Soloing and unfolding are interaction, not content, so neither records a revision — the
        //    reference does not `snap()` on either.
        case KeySubject::Solo:
            if (!Taken) return true;
            Arrangement.Soloed = (Arrangement.Soloed == Arrangement.Taken)
                               ? LayerStackCeiling::AbsentOrdinal : Arrangement.Taken;
            return true;

        case KeySubject::Unfold:
        {
            if (!Taken) return true;

            LayerEntry& Entry = Arrangement.Entries[Arrangement.Taken];

            if (Arrangement.TakenHalf == LayerTaken::Mask && Entry.Mask.Declared)
                Entry.Mask.Unfolded = !Entry.Mask.Unfolded;
            else
                Entry.Unfolded = !Entry.Unfolded;

            return true;
        }

        case KeySubject::Rename:
            if (!Taken || Arrangement.TakenHalf == LayerTaken::Mask) return true;
            Seated.Renaming = Arrangement.Taken;
            std::snprintf(Seated.RenamingRun, sizeof Seated.RenamingRun, "%s",
                          Arrangement.Entries[Arrangement.Taken].Naming);
            return true;

        case KeySubject::Seek:
            Seated.RetentionRoused = true;
            return true;

        // ④ Folder disclosure, which the two horizontal arrows drive.
        case KeySubject::Disclose:
        case KeySubject::Withhold:
        {
            if (!Taken) return true;

            LayerEntry& Entry = Arrangement.Entries[Arrangement.Taken];

            if (Entry.Content != LayerContent::Folder)
                return true;

            Entry.Opened = (Subject == KeySubject::Disclose);
            return true;
        }

        // ⑤ Stepping through the presented run, which is what makes the arrows read as a walk.
        case KeySubject::StepPrior:
        case KeySubject::StepNext:
        {
            PresentedHalf Halves[LayerStackCeiling::Entries * 2u];
            const std::uint32_t Count = PresentedHalves(Arrangement, Seated.Retention, Halves,
                                                        LayerStackCeiling::Entries * 2u);

            if (Count == 0u)
                return true;

            std::uint32_t Standing = 0u;

            for (std::uint32_t Walk = 0u; Walk < Count; ++Walk)
                if (Halves[Walk].Ordinal == Arrangement.Taken && Halves[Walk].Half == Arrangement.TakenHalf)
                {
                    Standing = Walk;
                    break;
                }

            // 📐 The walk stops at both ends rather than wrapping, exactly as `flat[i+d]` yields nothing.
            if (Subject == KeySubject::StepPrior && Standing == 0u)
                return true;

            if (Subject == KeySubject::StepNext && Standing + 1u >= Count)
                return true;

            const std::uint32_t Stepped = (Subject == KeySubject::StepNext) ? Standing + 1u : Standing - 1u;

            Arrangement.Taken     = Halves[Stepped].Ordinal;
            Arrangement.TakenHalf = Halves[Stepped].Half;
            return true;
        }

        case KeySubject::Withdraw:
            // 📐 Escape closes the popup first and clears the search run second.
            if (Seated.Popup != StackPopup::Absent)
            {
                Seated.Popup = StackPopup::Absent;
                if (Ledger != nullptr) Ledger->Withdraw();
                return true;
            }

            Seated.Retention[0] = '\0';
            return true;

        default:
            return false;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE STACK
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordStack(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                                  LayerStackOrdinates& Seated, RevisionSequence& Revisions)
{
    if (Surface == nullptr || Ledger == nullptr || !Surface->Recording())
        return;

    // 📝 The tooltip is resolved fresh every tick. A retained one outlives the control it named and hangs
    //    over the panel after the pointer has left it.
    Seated.Tooltip = nullptr;

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

    // 📐 `$('#count').textContent=count()+' · '+maskCount()+'m'` — entries and masks, in one chip.
    std::uint32_t MaskCount = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < Arrangement.EntryCount; ++Ordinal)
        if (Arrangement.Entries[Ordinal].Mask.Declared)
            ++MaskCount;

    char Counted[24] = {};
    std::snprintf(Counted, sizeof Counted, "%u \xC2\xB7 %um", Arrangement.EntryCount, MaskCount);

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

    // 📐 `#q` — the field is a PRIMITIVE and not a vendor widget, so it carries its own rouse and its own
    //    caret. A contact inside it takes the keyboard; a contact anywhere else gives it back.
    if (Pressed(ChromeCells[static_cast<std::uint32_t>(ChromeCell::SearchField)], Search, Seated,
                "Search layers"))
    {
        Seated.RetentionRoused = true;
    }
    else if (Sampled.ContactArrived && !Roused(Search))
    {
        Seated.RetentionRoused = false;
    }

    Surface->Ground(Search, Covering(0x000000u), Scaled.SearchAcross * 0.5f);
    Surface->Edge(Search, Seated.RetentionRoused ? Tinted.StrokeStrong : Tinted.Stroke, 1.0f,
                  Scaled.SearchAcross * 0.5f);
    Surface->Stroke(SymbolSubject::MagnifierLens,
                    Squared(Search.LeastAlong + 17.0f, (Search.LeastAcross + Search.MostAcross) * 0.5f, 13.0f),
                    Tinted.Faint);

    {
        const bool  Written  = Seated.Retention[0] != '\0';
        const float Baseline = (Search.LeastAcross + Search.MostAcross) * 0.5f - Surface->RunAcross(12.0f) * 0.5f;

        Surface->TextRunTruncated(Search.LeastAlong + 28.0f, Baseline, Search.SpanAlong() - 38.0f,
                                  Written ? Tinted.Primary : Tinted.Faint,
                                  Written ? Seated.Retention : "Search layers", 12.0f);

        // 📐 The caret, drawn only while the field holds the keyboard, at the run's trailing edge.
        if (Seated.RetentionRoused)
        {
            const float Caret = Search.LeastAlong + 28.0f +
                                (Written ? Surface->MeasureRun(Seated.Retention, 12.0f) : 0.0f);

            Surface->Ground(Spanning(Caret + 1.0f, Search.LeastAcross + 7.0f, 1.0f,
                                     Scaled.SearchAcross - 14.0f), Tinted.Primary);
        }
    }

    // ③ The three tool actions — add, group, retire.
    const float ActionMiddle = (Tools.LeastAcross + Tools.MostAcross) * 0.5f;

    const PlaneExtent AddButton    = Squared(Tools.MostAlong - Scaled.ToolsPadAlong -
                                             Scaled.ButtonExtent * 2.5f, ActionMiddle, Scaled.ButtonExtent);
    const PlaneExtent FolderButton = Squared(Tools.MostAlong - Scaled.ToolsPadAlong -
                                             Scaled.ButtonExtent * 1.5f, ActionMiddle, Scaled.ButtonExtent);
    const PlaneExtent RetireButton = Squared(Tools.MostAlong - Scaled.ToolsPadAlong -
                                             Scaled.ButtonExtent * 0.5f, ActionMiddle, Scaled.ButtonExtent);

    const auto Action = [&](ChromeCell Cell, const PlaneExtent& Seat, SymbolSubject Figure,
                            const char* Tooltip, InkOrdinate Ink) -> bool
    {
        const bool Taken = Pressed(ChromeCells[static_cast<std::uint32_t>(Cell)], Seat, Seated, Tooltip);

        if (Roused(Seat))
            Surface->Ground(Seat, Partial(0xFFFFFFu, 0.07), Scaled.RadiusSmall);

        Surface->Stroke(Figure, Inset(Seat, 6.5f, 6.5f), Ink);
        return Taken;
    };

    if (Action(ChromeCell::AddButton, AddButton, SymbolSubject::PlusCross, "Add layer", Tinted.Secondary))
    {
        Seated.Popup       = StackPopup::Addition;
        Seated.PopupAlong  = AddButton.MostAlong;
        Seated.PopupAcross = AddButton.MostAcross + 6.0f;
        Seated.PopupOffset = 0.0f;
        Ledger->Disclose(ChromeCells[static_cast<std::uint32_t>(ChromeCell::PopupBody)]);
    }

    if (Action(ChromeCell::FolderButton, FolderButton, SymbolSubject::LayerMerge, "Group", Tinted.Secondary))
    {
        Revisions.Record(Arrangement, "Folder declared");
        DeclareEntry(Arrangement, LayerContent::Folder, "New Folder");
    }

    if (Action(ChromeCell::RetireButton, RetireButton, SymbolSubject::TrashBin, "Delete", Tinted.Secondary))
    {
        Revisions.Record(Arrangement, "Entry retired");

        if (!RetireTaken(Arrangement))
            Revisions.Revert(Arrangement);
    }

    // ④ `.stack` — the scrolling run of rows, confined so an overflowing row is clipped and not drawn over
    //    the footer.
    const PlaneExtent Foot  = Spanning(Extent.LeastAlong, Extent.MostAcross - Scaled.FootAcross,
                                       Extent.SpanAlong(), Scaled.FootAcross);
    const PlaneExtent Stack = PlaneExtent{ Extent.LeastAlong, Tools.MostAcross,
                                           Extent.MostAlong,  Foot.LeastAcross };

    Surface->Confine(Stack);

    const float RowAlong  = Stack.SpanAlong() - Scaled.StackPadAlong * 2.0f - Scaled.ScrollAlong;
    float       Across    = Stack.LeastAcross + Scaled.StackPadAcross - Seated.StackOffset;
    const float Pointer   = Sampled.PositionAcross;
    const float PointerAt = Sampled.PositionAlong;

    std::uint32_t Hovered     = LayerStackCeiling::AbsentOrdinal;
    bool          HoveredMask = false;

    // 📐 What one drop would do, resolved fresh each tick against whatever the carried entry stands over.
    std::uint32_t Destination = LayerStackCeiling::AbsentOrdinal;
    DropIntent    Intent      = DropIntent::Absent;

    const bool Carrying = Seated.Carried < Arrangement.EntryCount;

    // 🔴 A contact that has not travelled the carry floor is still a candidate TAKE, not yet a carry. Only
    //    once it passes the floor does the row dim and a drop resolve — otherwise every ordinary press
    //    scrimmed the row it selected for the one tick the contact was held, which read as the row going
    //    hidden the instant it was clicked.
    const float CarryTravel = (Sampled.PositionAcross > Seated.CarryOrigin)
                            ? (Sampled.PositionAcross - Seated.CarryOrigin)
                            : (Seated.CarryOrigin - Sampled.PositionAcross);
    const bool  Travelled   = Carrying && Sampled.ContactHeld && CarryTravel >= CarryFloor;

    // 🔴 What the PREVIOUS tick resolved, kept before this tick overwrites it. The release tick carries no
    //    held contact, so it resolves no destination of its own — a drop that read only this tick's
    //    reading therefore always found nothing and silently discarded every reorder.
    const std::uint32_t PriorDestination = Seated.Destination;
    const DropIntent    PriorIntent      = Seated.Intent;

    // 📐 One presented row per enrolled run of cells. Beyond `RowCeiling` the rows still record and still
    //    take, on the row body's shared identity — a reduction, not a defect.
    std::uint32_t Presented = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < Arrangement.EntryCount; ++Ordinal)
    {
        // 📐 A retention run opens every folder it reaches into, so it is asked instead of the disclosure.
        const bool Retaining = Seated.Retention[0] != '\0';
        const bool Standing  = Retaining ? EntryRetained(Arrangement, Ordinal, Seated.Retention)
                                         : EntryPresented(Arrangement, Ordinal);

        if (!Standing)
            continue;

        const LayerEntry& Entry  = Arrangement.Entries[Ordinal];
        const float       Indent = static_cast<float>(Entry.Depth) * Scaled.RowStepAlong;
        const std::uint32_t Cells = (Presented < RowCeiling) ? (Presented * CellsPerRow) : 0u;
        const bool          Enrolled = Presented < RowCeiling;

        ++Presented;

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

            // 📐 A carried entry resolves its drop against whichever row it stands over. A folder crossed
            //    through its middle third is entered rather than passed — `y>.32&&y<.68`, verbatim.
            if (Travelled && Over && Seated.Carried != Ordinal &&
                !EntryWithin(Arrangement, Seated.Carried, Ordinal))
            {
                const float Fraction = (Pointer - Row.LeastAcross) / Row.SpanAcross();

                Destination = Ordinal;
                Intent      = (Entry.Content == LayerContent::Folder && Fraction > 0.32f && Fraction < 0.68f)
                            ? DropIntent::Enclosed
                            : ((Fraction < 0.5f) ? DropIntent::Prior : DropIntent::Trailing);
            }

            const bool TakenRow = Arrangement.Taken == Ordinal && Arrangement.TakenHalf == LayerTaken::Layer;

            RecordEntryRow(Row, Arrangement, Ordinal, TakenRow, Over);

            // 📐 A carried entry is drawn at half coverage in place, which is what `.dragging{opacity:.4}`
            //    states, rather than being lifted to the pointer.
            if (Travelled && Seated.Carried == Ordinal)
                Surface->Ground(Row, Partial(0x000000u, 0.55), 0.0f);

            if (Enrolled)
            {
                const float Middle = (Row.LeastAcross + Row.MostAcross) * 0.5f;

                // ⓐ The twisty, on a folder alone.
                if (Entry.Content == LayerContent::Folder)
                {
                    const PlaneExtent Twisty = Squared(Row.LeastAlong + Scaled.RowPadAlong +
                                                       Scaled.DiscloseAlong * 0.5f, Middle, 18.0f);

                    if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::Disclosure)],
                                Twisty, Seated, Entry.Opened ? "Collapse" : "Expand"))
                    {
                        Arrangement.Entries[Ordinal].Opened = !Entry.Opened;
                    }
                }

                // ⓑ The eye. Alternate-clicking it solos, exactly as `if(e.altKey)` branches.
                const PlaneExtent Eye = Squared(Row.LeastAlong + Scaled.RowPadAlong + Scaled.DiscloseAlong +
                                                Scaled.RowGapAlong * 0.5f + Scaled.ActionExtent * 0.5f,
                                                Middle, Scaled.ActionExtent);

                if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::Presence)], Eye, Seated,
                            Entry.Shown ? "Hide \xC2\xB7 Alt = solo" : "Show \xC2\xB7 Alt = solo"))
                {
                    Revisions.Record(Arrangement, "Presence amended");
                    Arrangement.Entries[Ordinal].Shown = !Entry.Shown;
                }

                // ⓒ The card chevron and the ellipsis, on the trailing edge.
                const PlaneExtent Unfolding = Squared(Row.MostAlong - Scaled.ActionExtent * 1.5f, Middle,
                                                      Scaled.ActionExtent);
                const PlaneExtent Menu      = Squared(Row.MostAlong - Scaled.ActionExtent * 0.5f, Middle,
                                                      Scaled.ActionExtent);

                if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::Unfolding)], Unfolding,
                            Seated, Entry.Unfolded ? "Hide details" : "Show details"))
                {
                    Arrangement.Entries[Ordinal].Unfolded = !Entry.Unfolded;
                }

                if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::Menu)], Menu, Seated,
                            "Layer menu"))
                {
                    Arrangement.Taken     = Ordinal;
                    Arrangement.TakenHalf = LayerTaken::Layer;
                    Seated.Popup          = StackPopup::LayerMenu;
                    Seated.PopupSubject   = Ordinal;
                    Seated.PopupOnMask    = false;
                    Seated.PopupAlong     = Menu.MostAlong;
                    Seated.PopupAcross    = Menu.MostAcross + 6.0f;
                    Seated.PopupOffset    = 0.0f;
                    Ledger->Disclose(ChromeCells[static_cast<std::uint32_t>(ChromeCell::PopupBody)]);
                }
            }
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

                if (Enrolled)
                {
                    const float Middle = (MaskRow.LeastAcross + MaskRow.MostAcross) * 0.5f;

                    const PlaneExtent MaskEye = Squared(MaskRow.LeastAlong + Scaled.RowPadAlong +
                                                        Scaled.ActionExtent * 0.5f, Middle, Scaled.ActionExtent);

                    if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::MaskPresence)],
                                MaskEye, Seated, Entry.Mask.Shown ? "Disable mask" : "Enable mask"))
                    {
                        Revisions.Record(Arrangement, "Mask presence amended");
                        Arrangement.Entries[Ordinal].Mask.Shown = !Entry.Mask.Shown;
                    }

                    const PlaneExtent MaskUnfold = Squared(MaskRow.MostAlong - Scaled.ActionExtent * 1.5f,
                                                           Middle, Scaled.ActionExtent);
                    const PlaneExtent MaskMenu   = Squared(MaskRow.MostAlong - Scaled.ActionExtent * 0.5f,
                                                           Middle, Scaled.ActionExtent);

                    if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::MaskUnfold)],
                                MaskUnfold, Seated, "Mask details"))
                    {
                        Arrangement.Entries[Ordinal].Mask.Unfolded = !Entry.Mask.Unfolded;
                    }

                    if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::MaskMenu)],
                                MaskMenu, Seated, "Mask menu"))
                    {
                        Arrangement.Taken     = Ordinal;
                        Arrangement.TakenHalf = LayerTaken::Mask;
                        Seated.Popup          = StackPopup::MaskMenu;
                        Seated.PopupSubject   = Ordinal;
                        Seated.PopupOnMask    = true;
                        Seated.PopupAlong     = MaskMenu.MostAlong;
                        Seated.PopupAcross    = MaskMenu.MostAcross + 6.0f;
                        Seated.PopupOffset    = 0.0f;
                        Ledger->Disclose(ChromeCells[static_cast<std::uint32_t>(ChromeCell::PopupBody)]);
                    }
                }
            }

            Across += Scaled.MaskRowAcross + 4.0f;
        }

        // 📐 The drop rule is recorded AFTER its row so it lies over the row's own ground.
        if (Destination == Ordinal && Intent != DropIntent::Absent)
            RecordDropMark(Row, Intent);
    }

    Seated.StackSpan   = (Across + Seated.StackOffset) - (Stack.LeastAcross + Scaled.StackPadAcross);
    Seated.Hovered     = Hovered;
    Seated.HoveredMask = HoveredMask;
    Seated.Destination = Destination;
    Seated.Intent      = Intent;

    Surface->Release();

    // ⑤ The scroll bar, recorded only when the run overflows its extent, and draggable in place.
    const float Visible = Stack.SpanAcross();
    const float Ceiling = (Seated.StackSpan > Visible) ? (Seated.StackSpan - Visible) : 0.0f;

    if (Ceiling > 0.0f && Visible > 0.0f)
    {
        const float Fraction    = Visible / Seated.StackSpan;
        const float ThumbAcross = (Visible * Fraction < 28.0f) ? 28.0f : Visible * Fraction;
        const float Travel      = Visible - ThumbAcross;
        const float Advanced    = Seated.StackOffset / Ceiling;

        const PlaneExtent Bar   = Spanning(Stack.MostAlong - Scaled.ScrollAlong, Stack.LeastAcross,
                                           Scaled.ScrollAlong, Visible);
        const PlaneExtent Thumb = Spanning(Stack.MostAlong - Scaled.ScrollAlong + 3.0f,
                                           Stack.LeastAcross + Travel * Advanced, 4.0f, ThumbAcross);

        ControlIdentity& Claimed = ChromeCells[static_cast<std::uint32_t>(ChromeCell::ScrollThumb)];

        if (Roused(Bar) && Sampled.ContactArrived && !Ledger->AnyDisclosed())
        {
            Ledger->Seize(Claimed, ControlPart::Thumb);
            Ledger->DepartFrom(Claimed, Seated.StackOffset);
        }

        Ledger->DeclareRoused(Claimed, Roused(Bar), RouseOver);

        // 📐 The bar travels `Travel` pixels while the run travels `Ceiling`, so the pointer's own travel
        //    is scaled by their ratio rather than applied to the offset directly.
        if (Ledger->Holding(Claimed) && Travel > 0.0f)
        {
            const Deliver<float> Departed = Ledger->DepartedOrdinate(Claimed);

            if (Departed.ContentPresent)
            {
                const float Moved = Sampled.PositionAcross - Ledger->OriginAcross();
                Seated.StackOffset = Departed.Resolve() + Moved * (Ceiling / Travel);
            }
        }

        Surface->Ground(Thumb, Partial(0xFFFFFFu, Ledger->Holding(Claimed) ? 0.30 : 0.15), 2.0f);
    }

    // ⑥ The wheel, which the stack answers itself because the seam carries no scrolling primitive.
    if (Stack.Encloses(PointerAt, Pointer) && Seated.Popup == StackPopup::Absent)
        Seated.StackOffset -= Sampled.WheelAcross * NotchAcross;

    if (Seated.StackOffset < 0.0f)      Seated.StackOffset = 0.0f;
    if (Seated.StackOffset > Ceiling)   Seated.StackOffset = Ceiling;

    // ⑦ What the artist takes, and what the artist carries. Both resolve off the SAME contact: a contact
    //    that arrived over a row and travelled beyond the carry floor is a drag, and one that did not is a
    //    take — which is exactly the separation `GestureTolerance` states and the reference gets from the
    //    window system's own drag threshold.
    if (Sampled.ContactArrived && Hovered < Arrangement.EntryCount && !Ledger->AnyDisclosed())
    {
        Arrangement.Taken     = Hovered;
        Arrangement.TakenHalf = HoveredMask ? LayerTaken::Mask : LayerTaken::Layer;

        // 📐 A secured entry refuses to be carried — `draggable="${n.lock?'false':'true'}"`.
        if (!HoveredMask && !Arrangement.Entries[Hovered].Secured)
        {
            Seated.Carried     = Hovered;
            Seated.CarryOrigin = Pointer;
        }
    }

    if (Carrying && !Sampled.ContactHeld)
    {
        // 📐 The drop, resolved against what the last held tick marked. A carry released over nothing
        //    droppable simply ends, which is what `cleanup()` does.
        if (PriorDestination < Arrangement.EntryCount && PriorIntent != DropIntent::Absent)
        {
            Revisions.Record(Arrangement, "Entry carried");

            const bool Moved = CarryEntry(Arrangement, Seated.Carried, PriorDestination,
                                          PriorIntent == DropIntent::Enclosed,
                                          PriorIntent == DropIntent::Trailing);

            if (!Moved)
                Revisions.Revert(Arrangement);
        }

        Seated.Carried     = LayerStackCeiling::AbsentOrdinal;
        Seated.Destination = LayerStackCeiling::AbsentOrdinal;
        Seated.Intent      = DropIntent::Absent;
    }

    // ⑧ `.foot` — the breadcrumb over the taken entry's blend and opacity.
    Surface->Ground(Foot, Tinted.PanelRaised);
    Surface->Edge(PlaneExtent{ Foot.LeastAlong, Foot.LeastAcross, Foot.MostAlong, Foot.LeastAcross + 1.0f },
                  Tinted.Stroke);

    if (Arrangement.Taken < Arrangement.EntryCount)
    {
        LayerEntry& Taken = Arrangement.Entries[Arrangement.Taken];
        const bool  OnMask = Arrangement.TakenHalf == LayerTaken::Mask && Taken.Mask.Declared;

        char Crumb[128] = {};
        std::snprintf(Crumb, sizeof Crumb, "%s  /  %s%s", ContentNaming(Taken.Content), Taken.Naming,
                      OnMask ? "  /  Mask" : "");

        Surface->TextRunTruncated(Foot.LeastAlong + Scaled.FootPadAlong, Foot.LeastAcross + 9.0f,
                                  Foot.SpanAlong() - Scaled.FootPadAlong * 2.0f, Tinted.Faint,
                                  Crumb, Scaled.RunFine);

        // 📐 `.blend` — the pill that opens the blend menu, capped at 52% of the footer.
        const float       BlendAlong = Foot.SpanAlong() * 0.52f;
        const PlaneExtent Blend      = Spanning(Foot.LeastAlong + Scaled.FootPadAlong,
                                                Foot.LeastAcross + 26.0f, BlendAlong, 27.0f);

        if (Pressed(ChromeCells[static_cast<std::uint32_t>(ChromeCell::BlendPill)], Blend, Seated,
                    "Blend mode"))
        {
            Seated.Popup        = StackPopup::BlendMode;
            Seated.PopupSubject = Arrangement.Taken;
            Seated.PopupOnMask  = OnMask;
            Seated.PopupAlong   = Blend.LeastAlong;
            Seated.PopupAcross  = Blend.LeastAcross - 6.0f;
            Seated.PopupOffset  = 0.0f;
            Ledger->Disclose(ChromeCells[static_cast<std::uint32_t>(ChromeCell::PopupBody)]);
        }

        const bool BlendRoused = Roused(Blend);

        Surface->Ground(Blend, Partial(0xFFFFFFu, BlendRoused ? 0.11 : 0.06), 13.5f);
        Surface->Edge(Blend, BlendRoused ? Tinted.StrokeStrong : Tinted.Stroke, 1.0f, 13.5f);
        Surface->TextRunTruncated(Blend.LeastAlong + 13.0f,
                                  Blend.LeastAcross + (27.0f - Surface->RunAcross(11.0f)) * 0.5f,
                                  BlendAlong - 32.0f, Tinted.Primary,
                                  OnMask ? Taken.Mask.Blend : Taken.Blend, 11.0f, true);
        Surface->Stroke(SymbolSubject::ChevronDown,
                        Squared(Blend.MostAlong - 12.0f, Blend.LeastAcross + 13.5f, 11.0f), Tinted.Faint);

        // 📐 `#opac` — the opacity run that fills the footer's trailing half, dragged in place. The mask
        //    half moves the mask's density instead, exactly as `if(selMask&&n.mask)n.mask.den=v`.
        const float MeterLeast = Blend.MostAlong + 10.0f;
        const float MeterMost  = Foot.MostAlong - Scaled.FootPadAlong - 34.0f;

        if (MeterMost > MeterLeast)
        {
            const PlaneExtent Track = PlaneExtent{ MeterLeast, Blend.LeastAcross + 4.0f,
                                                   MeterMost,  Blend.LeastAcross + 23.0f };

            std::uint32_t& Reading = OnMask ? Taken.Mask.Density : Taken.Opacity;
            const auto     Prior   = Reading;

            ControlIdentity& Claimed = ChromeCells[static_cast<std::uint32_t>(ChromeCell::OpacityRun)];

            // 📐 One revision per drag and not one per tick — recorded on the arriving edge alone, which
            //    is what `pointerdown → snap()` states and what keeps the ring from filling in a second.
            if (Roused(Track) && Sampled.ContactArrived && !Ledger->AnyDisclosed())
                Revisions.Record(Arrangement, OnMask ? "Mask density moved" : "Opacity amended");

            if (Dragged(Claimed, Track, Reading) && Reading == Prior)
                Reading = Prior;

            RecordMeter(PlaneExtent{ MeterLeast, Blend.LeastAcross + 12.0f, MeterMost,
                                     Blend.LeastAcross + 15.0f }, Reading, Tinted.Accent);

            // 📐 The thumb, drawn only while the run is roused or held, as `.rng::-webkit-slider-thumb`
            //    is scaled from zero on hover.
            if (Roused(Track) || Ledger->Holding(Claimed))
            {
                const float Seat = MeterLeast + (MeterMost - MeterLeast) *
                                   static_cast<float>(Reading) * 0.01f;
                Surface->Medallion(Seat, Blend.LeastAcross + 13.5f, 5.0f, Tinted.Accent);
            }
        }

        char Percent[8] = {};
        std::snprintf(Percent, sizeof Percent, "%u%%", OnMask ? Taken.Mask.Density : Taken.Opacity);
        Surface->TextRun(Foot.MostAlong - Scaled.FootPadAlong -
                         Surface->MeasureRun(Percent, Scaled.RunSub),
                         Blend.LeastAcross + 13.5f - Surface->RunAcross(Scaled.RunSub) * 0.5f,
                         Tinted.Secondary, Percent, Scaled.RunSub, 0.0f, true);
    }

    Surface->Release();
    Seated.ContactPrior = Sampled.ContactHeld;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE CHANNEL PROPERTIES
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordChannelProperties(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                                              LayerStackOrdinates& Seated, RevisionSequence& Revisions)
{
    if (Surface == nullptr || Ledger == nullptr || !Surface->Recording())
        return;

    Surface->Ground(Extent, Tinted.Panel);
    Surface->Confine(Extent);

    const std::uint32_t Subject = (Arrangement.Taken < Arrangement.EntryCount) ? Arrangement.Taken : 0u;
    LayerEntry&         Entry   = Arrangement.Entries[Subject];

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
        ChannelOrdinate&  Reading8 = Entry.Channels[Channel];
        const PlaneExtent Row      = Spanning(Extent.LeastAlong + Scaled.CardPadAlong, Across,
                                              Extent.SpanAlong() - Scaled.CardPadAlong * 2.0f, 28.0f);

        if (Surface->Excluded(Row))
        {
            Across += 30.0f;
            continue;
        }

        const bool RowRoused = Roused(Row);

        Surface->Ground(Row, RowRoused ? Tinted.RowHovered
                                       : (Reading8.Enabled ? Tinted.Row : Tinted.Detail),
                        Scaled.RadiusSmall);

        const float Middle = Row.LeastAcross + 14.0f;

        // 📐 `[data-cha="on"]` — the dot toggles the channel. Its own 20px cell, not the whole row, so a
        //    contact on the blend run beside it does not silently disable the channel.
        const PlaneExtent Dot = Squared(Row.LeastAlong + 12.0f, Middle, 20.0f);

        if (Pressed(RowCells[Channel * CellsPerRow + static_cast<std::uint32_t>(RowCell::Body)], Dot,
                    Seated, Reading8.Enabled ? "Disable channel" : "Enable channel"))
        {
            Revisions.Record(Arrangement, "Channel amended");
            Reading8.Enabled = !Reading8.Enabled;
        }

        Surface->Medallion(Row.LeastAlong + 12.0f, Middle, 4.0f,
                           Reading8.Enabled ? ChannelTint(Channel) : Tinted.Faint);

        if (Roused(Dot))
            Surface->Medallion(Row.LeastAlong + 12.0f, Middle, 7.5f, Partial(0xFFFFFFu, 0.14));

        Surface->TextRunTruncated(Row.LeastAlong + 22.0f, Middle - Surface->RunAcross(Scaled.RunSub) * 0.5f,
                                  108.0f, Reading8.Enabled ? Tinted.Primary : Tinted.Faint,
                                  ChannelNaming()[Channel], Scaled.RunSub, true);

        // 📐 The blend run opens the same twenty-nine-entry menu the footer pill does, anchored here.
        const PlaneExtent BlendRun = PlaneExtent{ Row.LeastAlong + 128.0f, Row.LeastAcross + 4.0f,
                                                  Row.MostAlong - 74.0f,   Row.MostAcross - 4.0f };

        if (Pressed(RowCells[Channel * CellsPerRow + static_cast<std::uint32_t>(RowCell::Menu)],
                    BlendRun, Seated, "Channel blend"))
        {
            Seated.Popup        = StackPopup::BlendMode;
            Seated.PopupSubject = Subject;
            Seated.PopupOnMask  = false;
            Seated.PopupAlong   = BlendRun.MostAlong;
            Seated.PopupAcross  = BlendRun.MostAcross + 6.0f;
            Seated.PopupOffset  = 0.0f;
            Ledger->Disclose(ChromeCells[static_cast<std::uint32_t>(ChromeCell::PopupBody)]);
        }

        if (Roused(BlendRun))
            Surface->Ground(BlendRun, Partial(0xFFFFFFu, 0.05), Scaled.RadiusSmall);

        Surface->TextRunTruncated(Row.LeastAlong + 132.0f, Middle - Surface->RunAcross(Scaled.RunFine) * 0.5f,
                                  Row.SpanAlong() - 132.0f - 74.0f, Tinted.Secondary,
                                  Reading8.Blend, Scaled.RunFine);

        // 📐 The channel's own opacity, dragged in place — `[data-cha="op"]`.
        {
            const PlaneExtent Track = PlaneExtent{ Row.MostAlong - 66.0f, Row.LeastAcross + 4.0f,
                                                   Row.MostAlong - 30.0f, Row.MostAcross - 4.0f };

            ControlIdentity& Claimed =
                RowCells[Channel * CellsPerRow + static_cast<std::uint32_t>(RowCell::Opacity)];

            if (Roused(Track) && Sampled.ContactArrived && !Ledger->AnyDisclosed())
                Revisions.Record(Arrangement, "Channel opacity amended");

            Dragged(Claimed, Track, Reading8.Opacity);
        }

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

void LayerStackPanel::RecordMaskProperties(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                                           LayerStackOrdinates& Seated, RevisionSequence& Revisions)
{
    if (Surface == nullptr || Ledger == nullptr || !Surface->Recording())
        return;

    Surface->Ground(Extent, Tinted.Panel);
    Surface->Confine(Extent);

    const std::uint32_t  Subject = (Arrangement.Taken < Arrangement.EntryCount) ? Arrangement.Taken : 0u;
    LayerEntry&          Entry   = Arrangement.Entries[Subject];
    MaskOrdinate&        Mask    = Entry.Mask;

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

        if (Pressed(RowCells[static_cast<std::uint32_t>(RowCell::Body)], Switch, Seated,
                    Mask.Inverted ? "Stop inverting" : "Invert"))
        {
            Revisions.Record(Arrangement, "Mask inverted");
            Mask.Inverted = !Mask.Inverted;
        }

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
            ParameterOrdinate& Parameter = Mask.Parameters[Ordinal];
            const float        Middle    = Across + Scaled.FieldAcross * 0.5f;

            // 📐 One enrolled run of cells per parameter, beyond the eight the channel rows take.
            const std::uint32_t Cells = (LayerStackCeiling::Channels + Ordinal < RowCeiling)
                                      ? (LayerStackCeiling::Channels + Ordinal) * CellsPerRow
                                      : 0u;

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

                // 📐 `[data-pt]` — a parameter switch toggles in place and does not re-record the panel.
                if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::Presence)], Switch, Seated))
                {
                    Revisions.Record(Arrangement, "Parameter amended");
                    Parameter.Standing = Standing ? 0.0 : 1.0;
                }

                Surface->Ground(Switch, Standing ? Tinted.Accent : Partial(0xFFFFFFu, 0.09), 7.0f);
                Surface->Medallion(Standing ? Switch.MostAlong - 6.0f : Switch.LeastAlong + 6.0f,
                                   Middle, 5.0f, Standing ? Covering(0x000000u) : Tinted.Secondary);
            }
            else
            {
                const double Span     = Parameter.Most - Parameter.Least;

                // 📐 `[data-pk]` — the range is dragged in its own declared span and not in 0…100, so a
                //    size in pixels or a rotation in degrees reads its own units back.
                {
                    const PlaneExtent Track = PlaneExtent{ Body.LeastAlong + 104.0f, Across + 3.0f,
                                                           Body.MostAlong - 44.0f, Across + Scaled.FieldAcross - 3.0f };

                    auto Reading = (Span > 0.0)
                                 ? static_cast<std::uint32_t>((Parameter.Standing - Parameter.Least) / Span * 100.0)
                                 : 0u;

                    ControlIdentity& Claimed =
                        RowCells[Cells + static_cast<std::uint32_t>(RowCell::Opacity)];

                    if (Roused(Track) && Sampled.ContactArrived && !Ledger->AnyDisclosed())
                        Revisions.Record(Arrangement, "Parameter amended");

                    if (Dragged(Claimed, Track, Reading))
                        Parameter.Standing = Parameter.Least + Span * static_cast<double>(Reading) * 0.01;
                }

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

            // 📐 `data-dact="bake"` — a contact on an untransferred chip transfers that one map. The
            //    reference offers only "transfer all missing"; per-chip is the same command at the
            //    granularity the chip already presents, and the row's own chip is where an artist aims.
            if (Pressed(RowCells[(LayerStackCeiling::Channels + Ordinal) % RowCeiling * CellsPerRow +
                                 static_cast<std::uint32_t>(RowCell::MaskBody)], Chip, Seated,
                        Transferred ? "Transferred" : "Transfer this mesh map"))
            {
                Revisions.Record(Arrangement, "Mesh map transferred");
                Mask.MeshMapTransferred[Ordinal] = !Transferred;
            }

            Surface->Ground(Chip, Partial(0xFFFFFFu, Roused(Chip) ? 0.11 : 0.05), 9.0f);
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

        // 📐 `data-dact="mchan"` — the chip toggles whether the mask reaches that channel.
        if (Pressed(RowCells[Channel * CellsPerRow + static_cast<std::uint32_t>(RowCell::MaskDensity)],
                    Chip, Seated, Reached ? "Stop applying" : "Apply to this channel"))
        {
            Revisions.Record(Arrangement, "Mask channels amended");
            Mask.ChannelApplied[Channel] = !Reached;
        }

        Surface->Ground(Chip, Reached ? Partial(0xFFFFFFu, 0.90) : Tinted.Detail, 9.0f);
        Surface->Edge(Chip, Roused(Chip) ? Tinted.StrokeStrong : Tinted.Stroke, 1.0f, 9.0f);
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

void LayerStackPanel::RecordRevisions(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                                     LayerStackOrdinates& Seated, RevisionSequence& Revisions)
{
    if (Surface == nullptr || !Surface->Recording() || Ledger == nullptr)
        return;

    Surface->Ground(Extent, Tinted.Panel);
    Surface->Confine(Extent);

    // ① The head, which folds the whole run. `renderHistory` gives its group header `cursor-pointer` and
    //    `onClick={() => toggleHistoryCard(token)}`, and rotates the chevron −90° while it stands folded.
    const PlaneExtent Head = Spanning(Extent.LeastAlong, Extent.LeastAcross, Extent.SpanAlong(),
                                      Scaled.HeadAcross);

    const bool HeadRoused = Roused(Head);

    if (Pressed(ChromeCells[static_cast<std::uint32_t>(ChromeCell::RevisionHead)], Head, Seated,
                Seated.RevisionsFolded ? "Show history" : "Hide history"))
    {
        Seated.RevisionsFolded = !Seated.RevisionsFolded;
        Seated.RevisionField   = 0u;
    }

    Surface->Ground(Head, HeadRoused ? Tinted.RowHovered : Tinted.PanelRaised);
    Surface->Edge(PlaneExtent{ Head.LeastAlong, Head.MostAcross - 1.0f, Head.MostAlong, Head.MostAcross },
                  Tinted.Stroke);

    // 📝 The caption the reference draws, which is not the spelling the identifiers carry.
    Surface->TextRunCapitalised(Head.LeastAlong + Scaled.HeadPadAlong,
                                Head.LeastAcross + (Scaled.HeadAcross - Surface->RunAcross(Scaled.RunHead)) * 0.5f,
                                HeadRoused ? Tinted.Primary : Tinted.Secondary, "History", Scaled.RunHead,
                                TrackingHead, true);

    const RevisionOrdinate* Reference = nullptr;
    std::uint32_t           Count     = 0u;
    SeatReferenceRevisions(Reference, Count);

    const std::uint32_t Standing = Revisions.RecordedCount() + Count;

    // 📐 `{tokenRevisions.length} ops` — the count the header carries at its trailing edge, ahead of the
    //    chevron.
    {
        char Reading[24] = {};
        std::snprintf(Reading, sizeof Reading, "%u ops", Standing);

        const float Along = Head.MostAlong - 34.0f - Surface->MeasureRun(Reading, Scaled.RunFine);
        Surface->TextRun(Along, Head.LeastAcross + (Scaled.HeadAcross - Surface->RunAcross(Scaled.RunFine)) * 0.5f,
                         Tinted.Secondary, Reading, Scaled.RunFine);
    }

    Surface->Stroke(Seated.RevisionsFolded ? SymbolSubject::ChevronRight : SymbolSubject::ChevronDown,
                    Squared(Head.MostAlong - 18.0f, (Head.LeastAcross + Head.MostAcross) * 0.5f, 13.0f),
                    Tinted.Faint);

    // ② The two ring actions, which the reference's own inspector does not carry but the arrangement
    //    demands: a recorded run that cannot be walked back is a log, not a history.
    const PlaneExtent Bar = Spanning(Extent.LeastAlong, Head.MostAcross, Extent.SpanAlong(), 34.0f);

    if (!Seated.RevisionsFolded)
    {
        Surface->Ground(Bar, Tinted.Detail);
        Surface->Edge(PlaneExtent{ Bar.LeastAlong, Bar.MostAcross - 1.0f, Bar.MostAlong, Bar.MostAcross },
                      Tinted.Stroke);

        const float Middle = (Bar.LeastAcross + Bar.MostAcross) * 0.5f;

        const PlaneExtent RevertSeat    = Spanning(Bar.LeastAlong + 10.0f, Middle - 11.0f, 78.0f, 22.0f);
        const PlaneExtent ReinstateSeat = Spanning(RevertSeat.MostAlong + 6.0f, Middle - 11.0f, 88.0f, 22.0f);

        const auto RingAction = [&](ChromeCell Cell, const PlaneExtent& Seat, const char* Caption,
                                    bool Offered, const char* Tooltip) -> bool
        {
            const bool Taken = Offered && Pressed(ChromeCells[static_cast<std::uint32_t>(Cell)], Seat,
                                                  Seated, Tooltip);

            const bool Lit = Offered && Roused(Seat);

            Surface->Ground(Seat, Partial(0xFFFFFFu, Lit ? 0.10 : 0.045), Scaled.RadiusSmall);
            Surface->Edge(Seat, Lit ? Tinted.StrokeStrong : Tinted.Stroke, 1.0f, Scaled.RadiusSmall);

            const float Along = Seat.LeastAlong +
                                (Seat.SpanAlong() - Surface->MeasureRun(Caption, Scaled.RunFine)) * 0.5f;

            Surface->TextRun(Along, Seat.LeastAcross + (22.0f - Surface->RunAcross(Scaled.RunFine)) * 0.5f,
                             Offered ? (Lit ? Tinted.Primary : Tinted.Secondary) : Tinted.Faint,
                             Caption, Scaled.RunFine);

            return Taken;
        };

        // 📐 Both read the ring rather than record into it, exactly as ⌘Z and ⇧⌘Z do in `AdmitChord`.
        if (RingAction(ChromeCell::RevertAction, RevertSeat, "Revert",
                       Revisions.RecordedCount() > 0u, "Revert the last amendment"))
        {
            Revisions.Revert(Arrangement);
            Seated.RevisionShown = LayerStackCeiling::AbsentOrdinal;
            Seated.RevisionField = 0u;
        }

        if (RingAction(ChromeCell::ReinstateAction, ReinstateSeat, "Reinstate",
                       Revisions.ReinstatableCount() > 0u, "Reinstate what was reverted"))
        {
            Revisions.Reinstate(Arrangement);
            Seated.RevisionShown = LayerStackCeiling::AbsentOrdinal;
            Seated.RevisionField = 0u;
        }
    }

    // 📐 `gridTemplateRows: isCollapsed ? '0fr' : '1fr'` — a folded run records nothing at all, which is
    //    what an unfolded extent of zero actually amounts to once the transition has settled.
    if (Seated.RevisionsFolded)
    {
        Seated.RevisionSpan = 0.0f;
        Surface->Release();
        return;
    }

    const PlaneExtent Run = Spanning(Extent.LeastAlong, Bar.MostAcross, Extent.SpanAlong(),
                                     Extent.MostAcross - Bar.MostAcross);

    Surface->Confine(Run);

    if (Standing == 0u)
    {
        // 📝 The reference's own empty state, verbatim.
        Surface->TextRunTruncated(Run.LeastAlong + Scaled.CardPadAlong, Run.LeastAcross + 16.0f,
                                  Run.SpanAlong() - Scaled.CardPadAlong * 2.0f, Tinted.Faint,
                                  "No history events found for this selection or its children.", 11.5f);
        Surface->Release();
        Surface->Release();
        Seated.RevisionSpan = 0.0f;
        return;
    }

    float Across = Run.LeastAcross + Scaled.StackPadAcross - Seated.RevisionOffset;

    std::uint32_t Enrolled = 0u;

    // 📐 One card, recorded the same way whether its reading came from the ring or from the seated
    //    reference run. The reference draws both out of one `tokenRevisions.map`, so they share a body.
    const auto RecordCard = [&](std::uint32_t Ordinal, const char* Naming, const char* Moment,
                                const char* Detail, bool Standing2) -> void
    {
        const bool  Shown = Seated.RevisionShown == Ordinal;
        const float Folded = Shown ? RevisionFoldAcross : 0.0f;
        const float Whole  = RevisionCardAcross + Folded;

        const PlaneExtent Card = Spanning(Run.LeastAlong + RevisionLeadAlong, Across,
                                          Run.SpanAlong() - RevisionLeadAlong - Scaled.StackPadAlong,
                                          RevisionCardAcross);

        const PlaneExtent Whole2 = Spanning(Card.LeastAlong, Across, Card.SpanAlong(), Whole);

        // 📐 The bubble and the spine, which the reference seats in two fixed columns of 32 and 15 to the
        //    left of every card and runs continuously between the first card and the last.
        const float Spine = Run.LeastAlong + RevisionSpineAlong;

        if (!Surface->Excluded(Whole2))
        {
            const bool First = Ordinal == 0u;
            const bool Last  = Ordinal + 1u == Standing;

            // 📐 The spine is its own flex column and runs the WHOLE row, the card's trailing padding
            //    included — so the gap between two cards carries spine and not ground. It starts at the
            //    first node and stops at the last, rounded at whichever end it terminates.
            const float SpineLeast = First ? (Across + 19.0f) : Across;
            const float SpineMost  = Last  ? (Across + 19.0f) : (Across + Whole + RevisionGapAcross);

            if (SpineMost > SpineLeast)
            {
                Surface->Ground(Spanning(Spine - 3.0f, SpineLeast, 6.0f, SpineMost - SpineLeast),
                                Partial(0xFFFFFFu, 0.10), (First || Last) ? 3.0f : 0.0f);
            }

            // 📐 `w-[7px] h-[7px] rounded-full bg-white shadow-[0_0_0_3px_var(--menu-2)]` — the node, which
            //    sits on the spine 19px into the card and is ringed by the panel's own ground.
            Surface->Ground(Squared(Spine, Across + 19.0f, 13.0f), Tinted.PanelRaised, 6.5f);
            Surface->Ground(Squared(Spine, Across + 19.0f, 7.0f), Tinted.Accent, 3.5f);

            // 📐 The medallion — `{i.toString().padStart(2,'0')}` in a 25px disc.
            const PlaneExtent Medallion = Squared(Run.LeastAlong + 16.0f, Across + 19.0f, 25.0f);
            Surface->Ground(Medallion, Standing2 ? Tinted.Accent : Partial(0xFFFFFFu, 0.16), 12.5f);

            char Numbered[4] = { static_cast<char>('0' + static_cast<char>((Ordinal / 10u) % 10u)),
                                 static_cast<char>('0' + static_cast<char>(Ordinal % 10u)), '\0', '\0' };

            Surface->TextRun(Medallion.LeastAlong +
                             (25.0f - Surface->MeasureRun(Numbered, Scaled.RunFine)) * 0.5f,
                             Medallion.LeastAcross + (25.0f - Surface->RunAcross(Scaled.RunFine)) * 0.5f,
                             Standing2 ? Tinted.Ground : Tinted.Primary, Numbered, Scaled.RunFine,
                             0.0f, true);
        }

        // 📐 The card itself, pressed to unfold. Only the first `RevisionCeiling` entries carry a cell;
        //    beyond that the pane still draws but no longer arbitrates, which is what a ceiling is for.
        bool Taken = false;

        if (Enrolled < RevisionCellCeiling)
        {
            Taken = Pressed(RevisionCells[Enrolled], Card, Seated,
                            Shown ? "Fold this revision" : "Unfold this revision");
            ++Enrolled;
        }

        if (Taken)
        {
            Seated.RevisionShown = Shown ? LayerStackCeiling::AbsentOrdinal : Ordinal;
            Seated.RevisionField = 0u;
        }

        if (Surface->Excluded(Whole2))
        {
            Across += Whole + RevisionGapAcross;
            return;
        }

        const bool Lit = Roused(Card);

        // 📐 `rounded-t-[8px] border-[var(--accent)] bg-[var(--accent-soft)] border-b-transparent` while
        //    unfolded, and a plain rounded tile otherwise.
        Surface->Ground(Card, Shown ? Partial(0xFFFFFFu, 0.07)
                                    : (Lit ? Tinted.RowHovered : Tinted.Row),
                        8.0f, Shown ? (CornerLeadingUpper | CornerTrailingUpper) : CornerAll);
        Surface->Edge(Card, Shown ? Tinted.StrokeStrong : Tinted.Stroke, 1.0f, 8.0f);

        Surface->TextRunTruncated(Card.LeastAlong + 8.0f, Card.LeastAcross + 8.0f,
                                  Card.SpanAlong() - 76.0f, Tinted.Primary, Naming, 12.5f, true);

        if (Detail != nullptr && Detail[0] != '\0')
        {
            Surface->TextRunTruncated(Card.LeastAlong + 8.0f, Card.LeastAcross + 25.0f,
                                      Card.SpanAlong() - 76.0f, Tinted.Secondary, Detail, 10.0f);
        }

        // 📐 `rev.date.toLocaleTimeString(…)` — the trailing moment, in the reference's own monospaced run.
        if (Moment != nullptr && Moment[0] != '\0')
        {
            const float Along = Card.MostAlong - 26.0f - Surface->MeasureRun(Moment, 10.0f);
            Surface->TextRun(Along, Card.LeastAcross + (RevisionCardAcross - Surface->RunAcross(10.0f)) * 0.5f,
                             Tinted.Faint, Moment, 10.0f);
        }

        // 📐 `${isRevExpanded ? 'rotate-180' : ''}` — the card's own chevron points DOWN at rest and turns
        //    a half circle when it unfolds. It is the run's head that swaps to a right chevron, not a card.
        Surface->Stroke(SymbolSubject::ChevronDown,
                        Squared(Card.MostAlong - 14.0f, (Card.LeastAcross + Card.MostAcross) * 0.5f, 12.0f),
                        Lit ? Tinted.Secondary : Tinted.Faint, Shown ? 3.14159265f : 0.0f);

        if (!Shown)
        {
            Across += Whole + RevisionGapAcross;
            return;
        }

        // ③ The fold — author and date over a Comment field over an optional Value field, exactly as the
        //    reference lays it out inside its `grid-template-rows: 1fr` panel.
        const PlaneExtent Fold = Spanning(Card.LeastAlong, Card.MostAcross - 1.0f, Card.SpanAlong(),
                                          RevisionFoldAcross + 1.0f);

        Surface->Ground(Fold, Partial(0xFFFFFFu, 0.045), 8.0f,
                        CornerTrailingLower | CornerLeadingLower);
        Surface->Edge(Fold, Tinted.StrokeStrong, 1.0f, 8.0f);

        Surface->TextRun(Fold.LeastAlong + 8.0f, Fold.LeastAcross + 8.0f, Tinted.Secondary,
                         "By System", 10.0f);

        if (Moment != nullptr && Moment[0] != '\0')
        {
            const float Along = Fold.MostAlong - 8.0f - Surface->MeasureRun(Moment, 10.0f);
            Surface->TextRun(Along, Fold.LeastAcross + 8.0f, Tinted.Secondary, Moment, 10.0f);
        }

        // 📐 One field, which takes the keyboard on a press and gives it back on a contact anywhere else —
        //    the same arbitration `#q` carries, because a primitive field has no vendor focus to borrow.
        const auto RecordField = [&](std::uint32_t Which, const PlaneExtent& Seat, const char* Caption,
                                     char* Written, const char* Absent) -> void
        {
            const bool Holding = Seated.RevisionField == Which;

            if (Enrolled < RevisionCellCeiling &&
                Pressed(RevisionCells[Enrolled], Seat, Seated, nullptr))
            {
                Seated.RevisionField = Holding ? 0u : Which;
            }
            else if (Sampled.ContactArrived && Holding && !Roused(Seat))
            {
                // 📝 `onBlur` — the reference writes the reading back exactly here and nowhere else.
                Seated.RevisionField = 0u;
            }

            if (Enrolled < RevisionCellCeiling)
                ++Enrolled;

            Surface->Ground(Seat, Tinted.PanelRaised, Scaled.RadiusSmall);
            Surface->Edge(Seat, Holding ? Tinted.StrokeStrong : Tinted.Stroke, 1.0f, Scaled.RadiusSmall);

            Surface->TextRunCapitalised(Seat.LeastAlong + 8.0f, Seat.LeastAcross + 6.0f, Tinted.Faint,
                                        Caption, 9.0f, TrackingSection, true);

            const bool Present = Written[0] != '\0';

            Surface->TextRunTruncated(Seat.LeastAlong + 8.0f, Seat.LeastAcross + 19.0f,
                                      Seat.SpanAlong() - 16.0f, Present ? Tinted.Primary : Tinted.Faint,
                                      Present ? Written : Absent, 11.5f);

            if (Holding)
            {
                const float Caret = Seat.LeastAlong + 8.0f +
                                    (Present ? Surface->MeasureRun(Written, 11.5f) : 0.0f);

                Surface->Ground(Spanning(Caret + 1.0f, Seat.LeastAcross + 18.0f, 1.0f, 13.0f),
                                Tinted.Primary);
            }
        };

        // 📐 The retained run is the field's key as well as its seat, so a card beyond the ceiling folds
        //    into the last retained pair rather than writing past the end of either run.
        const std::uint32_t Held = (Ordinal < LayerStackOrdinates::RevisionCeiling)
                                 ? Ordinal : (LayerStackOrdinates::RevisionCeiling - 1u);

        const PlaneExtent Remark = Spanning(Fold.LeastAlong + 7.0f, Fold.LeastAcross + 24.0f,
                                            Fold.SpanAlong() - 14.0f, 36.0f);

        RecordField(Held * 2u + 1u, Remark, "Comment", Seated.RevisionRemark[Held],
                    "Add a comment...");

        // 📐 `{rev.editValue !== undefined && …}` — the Value field stands only where the revision
        //    actually moved a reading, which is exactly where the card carries a detail.
        if (Detail != nullptr && Detail[0] != '\0')
        {
            const PlaneExtent Reading = Spanning(Fold.LeastAlong + 7.0f, Remark.MostAcross + 6.0f,
                                                 Fold.SpanAlong() - 14.0f, 36.0f);

            RecordField(Held * 2u + 2u, Reading, "Value", Seated.RevisionReading[Held], Detail);
        }

        Across += Whole + RevisionGapAcross;
    };

    std::uint32_t Ordinal = 0u;

    // 📐 What the artist has actually amended this session stands above the seated reference run, newest
    //    first, so the pane reads as one continuous record rather than as two.
    for (std::uint32_t Recorded = 0u; Recorded < Revisions.RecordedCount(); ++Recorded, ++Ordinal)
        RecordCard(Ordinal, Revisions.RevisionNaming(Recorded), "this session", nullptr, Ordinal == 0u);

    for (std::uint32_t Seat = 0u; Seat < Count; ++Seat, ++Ordinal)
    {
        RecordCard(Ordinal, Reference[Seat].Naming, Reference[Seat].Moment, Reference[Seat].Detail,
                   Ordinal == 0u);
    }

    Seated.RevisionSpan = (Across + Seated.RevisionOffset) - (Run.LeastAcross + Scaled.StackPadAcross);

    Surface->Release();

    // ④ The pane's own bar and wheel, on the same terms the stack's are — the seam carries no scrolling
    //    primitive, so a pane that overflows answers for itself.
    const float Visible = Run.SpanAcross();
    const float Ceiling = (Seated.RevisionSpan > Visible) ? (Seated.RevisionSpan - Visible) : 0.0f;

    if (Ceiling > 0.0f && Visible > 0.0f)
    {
        const float Fraction    = Visible / Seated.RevisionSpan;
        const float ThumbAcross = (Visible * Fraction < 28.0f) ? 28.0f : Visible * Fraction;
        const float Travel      = Visible - ThumbAcross;
        const float Advanced    = Seated.RevisionOffset / Ceiling;

        const PlaneExtent Track = Spanning(Run.MostAlong - Scaled.ScrollAlong, Run.LeastAcross,
                                           Scaled.ScrollAlong, Visible);
        const PlaneExtent Thumb = Spanning(Run.MostAlong - Scaled.ScrollAlong + 3.0f,
                                           Run.LeastAcross + Travel * Advanced, 4.0f, ThumbAcross);

        ControlIdentity& Claimed = ChromeCells[static_cast<std::uint32_t>(ChromeCell::RevisionBar)];

        if (Roused(Track) && Sampled.ContactArrived && !Ledger->AnyDisclosed())
        {
            Ledger->Seize(Claimed, ControlPart::Thumb);
            Ledger->DepartFrom(Claimed, Seated.RevisionOffset);
        }

        Ledger->DeclareRoused(Claimed, Roused(Track), RouseOver);

        if (Ledger->Holding(Claimed) && Travel > 0.0f)
        {
            const Deliver<float> Departed = Ledger->DepartedOrdinate(Claimed);

            if (Departed.ContentPresent)
            {
                const float Moved  = Sampled.PositionAcross - Ledger->OriginAcross();
                Seated.RevisionOffset = Departed.Resolve() + Moved * (Ceiling / Travel);
            }
        }

        Surface->Ground(Thumb, Partial(0xFFFFFFu, Ledger->Holding(Claimed) ? 0.30 : 0.15), 2.0f);
    }

    if (Run.Encloses(Sampled.PositionAlong, Sampled.PositionAcross) && Seated.Popup == StackPopup::Absent)
        Seated.RevisionOffset -= Sampled.WheelAcross * NotchAcross;

    if (Seated.RevisionOffset < 0.0f)      Seated.RevisionOffset = 0.0f;
    if (Seated.RevisionOffset > Ceiling)   Seated.RevisionOffset = Ceiling;

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DEFERRED SWEEP
//------------------------------------------------------------------------------------------------------------------------

// 📐 `hex2hsl` / `hsl2hex`, transcribed. The wheel is authored in hue-saturation-luminance because that is
//    what a ring and a radius actually are; the arrangement retains packed sRGB, so the pair converts.
static void SeparateTint(std::uint32_t Packed, float& Hue, float& Saturation, float& Luminance)
{
    const float Red   = static_cast<float>((Packed >> 16) & 0xFFu) / 255.0f;
    const float Green = static_cast<float>((Packed >>  8) & 0xFFu) / 255.0f;
    const float Blue  = static_cast<float>( Packed        & 0xFFu) / 255.0f;

    const float Most  = (Red > Green ? (Red > Blue ? Red : Blue) : (Green > Blue ? Green : Blue));
    const float Least = (Red < Green ? (Red < Blue ? Red : Blue) : (Green < Blue ? Green : Blue));

    Luminance = (Most + Least) * 0.5f;
    Hue       = 0.0f;
    Saturation = 0.0f;

    if (Most != Least)
    {
        const float Span = Most - Least;

        Saturation = (Luminance > 0.5f) ? (Span / (2.0f - Most - Least)) : (Span / (Most + Least));

        if (Most == Red)        Hue = (Green - Blue) / Span + (Green < Blue ? 6.0f : 0.0f);
        else if (Most == Green) Hue = (Blue - Red) / Span + 2.0f;
        else                    Hue = (Red - Green) / Span + 4.0f;

        Hue *= 60.0f;
    }

    Saturation *= 100.0f;
    Luminance  *= 100.0f;
}

static std::uint32_t CombineTint(float Hue, float Saturation, float Luminance)
{
    const float Sat = Saturation * 0.01f;
    const float Lum = Luminance  * 0.01f;
    const float Amplitude = Sat * ((Lum < 1.0f - Lum) ? Lum : (1.0f - Lum));

    const auto Channel = [&](float Offset) -> std::uint32_t
    {
        float Turned = Offset + Hue / 30.0f;

        while (Turned >= 12.0f) Turned -= 12.0f;
        while (Turned <    0.0f) Turned += 12.0f;

        const float Lower   = (Turned - 3.0f < 9.0f - Turned) ? (Turned - 3.0f) : (9.0f - Turned);
        const float Bounded = (Lower < 1.0f) ? Lower : 1.0f;
        const float Written = Lum - Amplitude * ((Bounded > -1.0f) ? Bounded : -1.0f);
        const float Scaled8 = Written * 255.0f + 0.5f;

        return static_cast<std::uint32_t>((Scaled8 < 0.0f) ? 0.0f : ((Scaled8 > 255.0f) ? 255.0f : Scaled8));
    };

    return (Channel(0.0f) << 16) | (Channel(8.0f) << 8) | Channel(4.0f);
}

void LayerStackPanel::RecordDeferred(LayerArrangement& Arrangement, LayerStackOrdinates& Seated,
                                     RevisionSequence& Revisions)
{
    if (Surface == nullptr || Ledger == nullptr || !Surface->Recording())
        return;

    // ① The veil. A contact anywhere outside the standing popup dismisses it, exactly as the reference's
    //    document-level `pointerdown` does — and it dismisses it WITHOUT the contact reaching a row.
    if (Seated.Popup != StackPopup::Absent && Sampled.ContactArrived)
    {
        // 📐 Tested against where the card was actually RECORDED last tick, not against its anchor.
        const PlaneExtent Card = Spanning(Seated.PopupSeatAlong, Seated.PopupSeatAcross,
                                          PopupAlongLeast, Seated.PopupSeatSpan);

        if (!Card.Encloses(Sampled.PositionAlong, Sampled.PositionAcross))
        {
            Seated.Popup = StackPopup::Absent;
            Ledger->Withdraw();
        }
    }

    if (Seated.Popup == StackPopup::Absent)
    {
        Seated.PopupSettled = false;
    }
    else
    {
        // 📝 The card's extent is counted from its declared run BEFORE the ground is recorded, because the
        //    ground has to be laid first — a vendor command list is ordered, so a ground recorded after its
        //    entries would paint over them.

        const std::uint32_t Subject = (Seated.PopupSubject < Arrangement.EntryCount)
                                    ? Seated.PopupSubject : Arrangement.Taken;
        const bool          Present = Subject < Arrangement.EntryCount;

        std::uint32_t Entries  = 0u;
        std::uint32_t Captions = 0u;
        std::uint32_t Rules    = 0u;

        switch (Seated.Popup)
        {
            case StackPopup::Addition:    Entries = 7u;  Captions = 1u; Rules = 2u; break;
            case StackPopup::BlendMode:   Entries = 29u; Captions = 1u; Rules = 0u; break;
            case StackPopup::LayerMenu:   Entries = 9u;  Captions = 2u; Rules = 3u; break;
            case StackPopup::MaskMenu:    Entries = 20u; Captions = 4u; Rules = 3u; break;
            case StackPopup::EffectMenu:  Entries = 14u; Captions = 1u; Rules = 0u; break;
            case StackPopup::ColourWheel: Entries = 0u;  Captions = 1u; Rules = 0u; break;
            default:                                                                break;
        }

        float Measured = PopupPad * 2.0f + static_cast<float>(Entries) * PopupEntryAcross
                       + static_cast<float>(Captions) * PopupCaption + static_cast<float>(Rules) * 8.0f;

        if (Seated.Popup == StackPopup::LayerMenu)
            Measured += 30.0f;   // 📐 the swatch strip

        if (Seated.Popup == StackPopup::ColourWheel)
            Measured += 244.0f;  // 📐 the ring, its luminance run and its hexadecimal row

        // 📐 A run taller than the display scrolls inside its own card rather than running off it, which is
        //    what the twenty-nine blend modes need on a short display.
        const float DisplayAcross = (Surface->Display().ExtentAcross > 0.0f)
                                  ? Surface->Display().ExtentAcross : 1080.0f;
        const float Ceiling       = DisplayAcross - 16.0f;
        const float Standing      = (Measured < Ceiling) ? Measured : ((Ceiling > 80.0f) ? Ceiling : 80.0f);

        const PlaneExtent Anchored = RecordPopupGround(Seated, Seated.PopupAlong, Seated.PopupAcross,
                                                       Standing);
        const PlaneExtent Card     = Spanning(Anchored.LeastAlong, Anchored.LeastAcross,
                                              PopupAlongLeast, Standing);

        Seated.PopupSeatSpan = Standing;

        Surface->Ground(Card, Tinted.PanelRaised, Scaled.RadiusStandard);
        Surface->Edge(Card, Tinted.StrokeStrong, 1.0f, Scaled.RadiusStandard);
        Surface->Confine(Card);

        float Across = Anchored.LeastAcross + PopupPad;

        const auto Caption = [&](const char* Written)
        {
            Surface->TextRunCapitalised(Anchored.LeastAlong + 14.0f,
                                        Across + (PopupCaption - Surface->RunAcross(Scaled.RunSection)) * 0.5f,
                                        Tinted.Faint, Written, Scaled.RunSection, TrackingSection, true);
            Across += PopupCaption;
        };

        const auto Rule = [&]()
        {
            Surface->Ground(PlaneExtent{ Anchored.LeastAlong + 6.0f, Across + 3.0f,
                                         Anchored.MostAlong - 6.0f, Across + 4.0f }, Tinted.Stroke);
            Across += 8.0f;
        };

        const auto Entry = [&](const char* Written, const char* Chord, bool Marked, bool Dangerous) -> bool
        {
            const PlaneExtent Seated0 = Spanning(Anchored.LeastAlong + PopupPad, Across,
                                                 PopupAlongLeast - PopupPad * 2.0f, PopupEntryAcross);
            Across += PopupEntryAcross;
            return RecordPopupEntry(Seated0, Written, Chord, Marked, Dangerous, Seated);
        };

        if (Measured > Standing && Card.Encloses(Sampled.PositionAlong, Sampled.PositionAcross))
        {
            Seated.PopupOffset -= Sampled.WheelAcross * NotchAcross;

            const float Travel = Measured - Standing;

            if (Seated.PopupOffset < 0.0f)    Seated.PopupOffset = 0.0f;
            if (Seated.PopupOffset > Travel)  Seated.PopupOffset = Travel;
        }
        else if (Measured <= Standing)
        {
            Seated.PopupOffset = 0.0f;
        }

        Across -= Seated.PopupOffset;

        switch (Seated.Popup)
        {
            // ⓐ `#btnAdd` — the seven declarations, with their chords.
            case StackPopup::Addition:
            {
                struct Declaration { const char* Naming; const char* Chord; LayerContent Content; };

                static const Declaration Offered[7] =
                {
                    { "Paint layer",       "P",  LayerContent::Paint      },
                    { "Fill layer",        "F",  LayerContent::Fill       },
                    { "Adjustment",        "A",  LayerContent::Adjustment },
                    { "Filter",            "R",  LayerContent::Retention  },
                    { "Decal layer \xC2\xB7 3D", "D", LayerContent::Decal  },
                    { "Pattern layer",     "T",  LayerContent::Pattern    },
                    { "Group",             "G",  LayerContent::Folder     }
                };

                static const char* const Declared[7] =
                {
                    "Paint Layer", "Fill Layer", "Adjustment", "Filter",
                    "Decal Layer", "Pattern Layer", "New Folder"
                };

                Caption("Add");

                for (std::uint32_t Ordinal = 0u; Ordinal < 7u; ++Ordinal)
                {
                    if (Ordinal == 4u || Ordinal == 6u)
                        Rule();

                    if (Entry(Offered[Ordinal].Naming, Offered[Ordinal].Chord, false, false))
                    {
                        Revisions.Record(Arrangement, "Entry declared");
                        DeclareEntry(Arrangement, Offered[Ordinal].Content, Declared[Ordinal]);
                    }
                }

                break;
            }

            // ⓑ `#btnBlend` — the blend run the taken half accepts.
            case StackPopup::BlendMode:
            {
                if (!Present)
                    break;

                LayerEntry& Taken   = Arrangement.Entries[Subject];
                const bool  OnMask  = Seated.PopupOnMask && Taken.Mask.Declared;
                const char* Standing0 = OnMask ? Taken.Mask.Blend : Taken.Blend;

                std::uint32_t       Count   = 0u;
                const char* const*  Offered = BlendNaming(0xFFFFFFFFu, Count);

                Caption("Blend mode");

                for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
                {
                    const bool Marked = std::strcmp(Offered[Ordinal], Standing0) == 0;

                    if (Entry(Offered[Ordinal], nullptr, Marked, false))
                    {
                        Revisions.Record(Arrangement, "Blend restated");

                        if (OnMask) Arrangement.Entries[Subject].Mask.Blend = Offered[Ordinal];
                        else        Arrangement.Entries[Subject].Blend      = Offered[Ordinal];
                    }
                }

                break;
            }

            // ⓒ One entry's own menu.
            case StackPopup::LayerMenu:
            {
                if (!Present)
                    break;

                const LayerEntry& Taken = Arrangement.Entries[Subject];

                Caption(Taken.Naming);

                if (Entry("Rename", "F2", false, false))
                {
                    Seated.Renaming = Subject;
                    std::snprintf(Seated.RenamingRun, sizeof Seated.RenamingRun, "%s", Taken.Naming);
                }

                if (Entry(Taken.Unfolded ? "Hide details" : "Show details", "Space", false, false))
                    Arrangement.Entries[Subject].Unfolded = !Taken.Unfolded;

                if (Entry(Taken.Mask.Declared ? "Remove mask" : "Add mask", "M", false, false))
                {
                    Revisions.Record(Arrangement, "Mask amended");
                    Arrangement.Taken = Subject;

                    if (!ToggleMask(Arrangement))
                        Revisions.Revert(Arrangement);
                }

                if (Entry(Taken.Secured ? "Unlock" : "Lock", "L", false, false))
                {
                    Revisions.Record(Arrangement, "Entry secured");
                    Arrangement.Entries[Subject].Secured = !Taken.Secured;
                }

                if (Entry(Arrangement.Soloed == Subject ? "Clear solo" : "Solo", "S", false, false))
                {
                    Arrangement.Soloed = (Arrangement.Soloed == Subject)
                                       ? LayerStackCeiling::AbsentOrdinal : Subject;
                }

                if (Entry("Duplicate", "Cmd D", false, false))
                {
                    Revisions.Record(Arrangement, "Entry copied");
                    Arrangement.Taken = Subject;

                    if (!DuplicateTaken(Arrangement))
                        Revisions.Revert(Arrangement);
                }

                if (Entry("Group", "Cmd G", false, false))
                {
                    Revisions.Record(Arrangement, "Entries grouped");
                    Arrangement.Taken = Subject;

                    if (!EncloseTaken(Arrangement))
                        Revisions.Revert(Arrangement);
                }

                Rule();
                Caption("Colour tag");

                // 📐 `<div class="swatches">` — ten 18px swatches, the standing one ringed.
                {
                    const std::uint32_t* const Offered = SeatedColourTags();
                    const float Step = (PopupAlongLeast - PopupPad * 2.0f - 12.0f) /
                                       static_cast<float>(LayerStackCeiling::ColourTags);

                    for (std::uint32_t Ordinal = 0u; Ordinal < LayerStackCeiling::ColourTags; ++Ordinal)
                    {
                        const PlaneExtent Swatch = Spanning(Anchored.LeastAlong + PopupPad + 6.0f +
                                                            Step * static_cast<float>(Ordinal),
                                                            Across + 4.0f, Step - 2.0f, 18.0f);

                        Surface->Ground(Swatch, Covering(Offered[Ordinal]), 4.0f);

                        if (Taken.ColourTag == Offered[Ordinal])
                            Surface->Edge(Swatch, Tinted.Accent, 1.5f, 4.0f);

                        if (Swatch.Encloses(Sampled.PositionAlong, Sampled.PositionAcross) &&
                            Sampled.ContactReleased)
                        {
                            Revisions.Record(Arrangement, "Colour tag amended");
                            Arrangement.Entries[Subject].ColourTag = Offered[Ordinal];
                            Seated.Popup = StackPopup::Absent;
                            Ledger->Withdraw();
                        }
                    }

                    Across += 30.0f;
                }

                if (Entry("Custom colour...", nullptr, false, false))
                {
                    SeparateTint(Taken.ColourTag, Seated.WheelHue, Seated.WheelSaturation,
                                 Seated.WheelLuminance);

                    Seated.Popup        = StackPopup::ColourWheel;
                    Seated.PopupSubject = Subject;
                    Seated.PopupOffset  = 0.0f;
                    Ledger->Disclose(ChromeCells[static_cast<std::uint32_t>(ChromeCell::PopupBody)]);
                }

                Rule();

                if (Entry("Delete", "Del", false, true))
                {
                    Revisions.Record(Arrangement, "Entry retired");
                    Arrangement.Taken     = Subject;
                    Arrangement.TakenHalf = LayerTaken::Layer;

                    if (!RetireTaken(Arrangement))
                        Revisions.Revert(Arrangement);
                }

                break;
            }

            // ⓓ One mask's own menu — source, generator, invert, clear.
            case StackPopup::MaskMenu:
            {
                if (!Present || !Arrangement.Entries[Subject].Mask.Declared)
                    break;

                const MaskOrdinate& Mask = Arrangement.Entries[Subject].Mask;

                Caption("Mask");

                if (Entry(Mask.Unfolded ? "Hide details" : "Show details", nullptr, false, false))
                    Arrangement.Entries[Subject].Mask.Unfolded = !Mask.Unfolded;

                if (Entry("Invert", nullptr, Mask.Inverted, false))
                {
                    Revisions.Record(Arrangement, "Mask inverted");
                    Arrangement.Entries[Subject].Mask.Inverted = !Mask.Inverted;
                }

                Rule();
                Caption("Source");

                // 📐 `SRCLIST.filter(s=>s!=='Generator')` — the generator reaches the run below instead.
                for (std::uint32_t Ordinal = 0u; Ordinal < 6u; ++Ordinal)
                {
                    const auto Offered = static_cast<MaskSource>(Ordinal);

                    if (Entry(SourceNaming(Offered), nullptr, Mask.Source == Offered, false))
                    {
                        Revisions.Record(Arrangement, "Mask source restated");
                        Arrangement.Entries[Subject].Mask.Source = Offered;
                    }
                }

                Rule();
                Caption("Generator");

                // 📐 `GENLIST` — the eight the reference declares, in its own order.
                static const char* const Generators[8] =
                {
                    "Curvature", "Ambient Occlusion", "Dirt", "Metal Edge Wear",
                    "Position", "Mask Editor", "Light", "Panel Lines"
                };

                for (const char* const Offered : Generators)
                {
                    const bool Marked = Mask.Source == MaskSource::Generator &&
                                        Mask.Generator != nullptr &&
                                        std::strcmp(Mask.Generator, Offered) == 0;

                    if (Entry(Offered, nullptr, Marked, false))
                    {
                        Revisions.Record(Arrangement, "Mask generator restated");
                        Arrangement.Entries[Subject].Mask.Source    = MaskSource::Generator;
                        Arrangement.Entries[Subject].Mask.Generator = Offered;
                    }
                }

                Rule();

                if (Entry("Clear mask effects", nullptr, false, false))
                {
                    Revisions.Record(Arrangement, "Mask effects cleared");
                    Arrangement.Entries[Subject].Mask.EffectCount = 0u;
                }

                if (Entry("Delete mask", nullptr, false, true))
                {
                    Revisions.Record(Arrangement, "Mask retired");
                    Arrangement.Entries[Subject].Mask = MaskOrdinate{};
                    Arrangement.TakenHalf             = LayerTaken::Layer;
                }

                break;
            }

            // ⓔ The effect run a card's `+` offers.
            case StackPopup::EffectMenu:
            {
                if (!Present)
                    break;

                std::uint32_t      Count   = 0u;
                const char* const* Offered = EffectNaming(Count);

                Caption("Add effect");

                for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
                {
                    if (!Entry(Offered[Ordinal], nullptr, false, false))
                        continue;

                    Revisions.Record(Arrangement, "Effect declared");

                    if (Seated.PopupOnMask && Arrangement.Entries[Subject].Mask.Declared)
                    {
                        MaskOrdinate& Mask = Arrangement.Entries[Subject].Mask;

                        if (Mask.EffectCount < LayerStackCeiling::Effects)
                            Mask.Effects[Mask.EffectCount++] = Offered[Ordinal];
                        else
                            Revisions.Revert(Arrangement);
                    }
                    else
                    {
                        LayerEntry& Taken = Arrangement.Entries[Subject];

                        if (Taken.EffectCount < LayerStackCeiling::Effects)
                            Taken.Effects[Taken.EffectCount++] = Offered[Ordinal];
                        else
                            Revisions.Revert(Arrangement);
                    }
                }

                break;
            }

            // ⓕ The colour wheel — a hue ring, a luminance run and an Apply.
            case StackPopup::ColourWheel:
            {
                Caption("Custom colour");

                // 📐 `.wheel` — a 158px disc whose angle is hue and whose radius, out to `r-9`, is
                //    saturation. It is recorded as concentric arcs of medallions rather than as a texture,
                //    because the recording seam carries no per-pixel primitive and the ring is the only
                //    thing in the panel that needs one.
                const float Ring   = 158.0f;
                const float Radius = Ring * 0.5f;
                const float Centre = Anchored.LeastAlong + PopupAlongLeast * 0.5f;
                const float Middle = Across + Radius + 4.0f;

                Surface->Ground(Squared(Centre, Middle, Ring), Covering(0x0A0A0Au), Radius);

                // 📐 Sixty spokes of ten stops each. Six hundred medallions is what a 158px ring needs to
                //    read as continuous; fewer bands and the saturation axis visibly steps.
                for (std::uint32_t Spoke = 0u; Spoke < 60u; ++Spoke)
                {
                    const float Turn = static_cast<float>(Spoke) * 6.0f;
                    const float Sine = Turn;

                    for (std::uint32_t Band = 0u; Band < 10u; ++Band)
                    {
                        const float Away = (static_cast<float>(Band) + 0.5f) * 0.1f;
                        const float Sat  = Away * 100.0f;
                        const auto  Tint = CombineTint(Sine, Sat, Seated.WheelLuminance);

                        // 📐 sin and cos from the turn, without <cmath>: the ring is sixty fixed spokes, so
                        //    the two are read from a stated quarter-turn run rather than computed.
                        static const float Quarter[16] =
                        {
                            0.0000f, 0.1045f, 0.2079f, 0.3090f, 0.4067f, 0.5000f, 0.5878f, 0.6691f,
                            0.7431f, 0.8090f, 0.8660f, 0.9135f, 0.9511f, 0.9781f, 0.9945f, 1.0000f
                        };

                        const std::uint32_t Step  = Spoke;                       // 6 degrees each
                        const std::uint32_t Ordinal = Step % 60u;
                        const auto Sines = [&](std::uint32_t Which) -> float
                        {
                            const std::uint32_t Folded = Which % 60u;

                            if (Folded <= 15u) return  Quarter[Folded];
                            if (Folded <= 30u) return  Quarter[30u - Folded];
                            if (Folded <= 45u) return -Quarter[Folded - 30u];
                            return                    -Quarter[60u - Folded];
                        };

                        const float Along  = Centre + Sines((Ordinal + 15u) % 60u) * (Radius - 9.0f) * Away;
                        const float Across0 = Middle + Sines(Ordinal) * (Radius - 9.0f) * Away;

                        Surface->Medallion(Along, Across0, 5.0f, Covering(Tint));
                    }
                }

                // 📐 The dot, at the standing hue and saturation.
                {
                    static const float Quarter[16] =
                    {
                        0.0000f, 0.1045f, 0.2079f, 0.3090f, 0.4067f, 0.5000f, 0.5878f, 0.6691f,
                        0.7431f, 0.8090f, 0.8660f, 0.9135f, 0.9511f, 0.9781f, 0.9945f, 1.0000f
                    };

                    const auto Sines = [&](float Degrees) -> float
                    {
                        float Turned = Degrees;
                        while (Turned >= 360.0f) Turned -= 360.0f;
                        while (Turned <    0.0f) Turned += 360.0f;

                        const auto Step = static_cast<std::uint32_t>(Turned / 6.0f) % 60u;

                        if (Step <= 15u) return  Quarter[Step];
                        if (Step <= 30u) return  Quarter[30u - Step];
                        if (Step <= 45u) return -Quarter[Step - 30u];
                        return                  -Quarter[60u - Step];
                    };

                    const float Away  = Seated.WheelSaturation * 0.01f * (Radius - 9.0f);
                    const float Along = Centre + Sines(Seated.WheelHue + 90.0f) * Away;
                    const float Down  = Middle + Sines(Seated.WheelHue) * Away;
                    const auto  Tint  = CombineTint(Seated.WheelHue, Seated.WheelSaturation,
                                                    Seated.WheelLuminance);

                    Surface->Medallion(Along, Down, 7.0f, Covering(0xFFFFFFu));
                    Surface->Medallion(Along, Down, 5.5f, Covering(Tint));

                    // 📐 A contact inside the ring resolves hue from its angle and saturation from its
                    //    radius, and keeps resolving while it is held — which is what makes it a wheel.
                    const PlaneExtent Disc = Squared(Centre, Middle, Ring);

                    if (Disc.Encloses(Sampled.PositionAlong, Sampled.PositionAcross) &&
                        (Sampled.ContactArrived || Sampled.ContactHeld))
                    {
                        const float OffAlong  = Sampled.PositionAlong  - Centre;
                        const float OffAcross = Sampled.PositionAcross - Middle;

                        // 📐 The angle, resolved by walking the same sixty spokes rather than by an
                        //    arc-tangent — the ring has sixty stops, so sixty comparisons resolve it
                        //    exactly and pull in no further dependency.
                        float Nearest = 0.0f;
                        float Closest = 1.0e30f;

                        for (std::uint32_t Step = 0u; Step < 60u; ++Step)
                        {
                            const float Turn      = static_cast<float>(Step) * 6.0f;
                            const float SpokeDown = Sines(Turn);
                            const float SpokeAcross = Sines(Turn + 90.0f);
                            const float Away2     = OffAlong * SpokeAcross + OffAcross * SpokeDown;

                            if (Away2 <= 0.0f)
                                continue;

                            const float Off = (OffAlong - SpokeAcross * Away2) * (OffAlong - SpokeAcross * Away2)
                                            + (OffAcross - SpokeDown * Away2) * (OffAcross - SpokeDown * Away2);

                            if (Off < Closest)
                            {
                                Closest = Off;
                                Nearest = Turn;
                            }
                        }

                        const float Extent2 = OffAlong * OffAlong + OffAcross * OffAcross;
                        float       Reach   = 0.0f;

                        // 📐 The radius, by bisection over the squared extent — the same reason as above.
                        for (float Probe = 0.0f; Probe <= 1.0f; Probe += 0.01f)
                        {
                            const float Span = Probe * (Radius - 9.0f);

                            if (Span * Span <= Extent2)
                                Reach = Probe;
                        }

                        Seated.WheelHue        = Nearest;
                        Seated.WheelSaturation = Reach * 100.0f;
                    }
                }

                Across = Middle + Radius + 10.0f;

                // 📐 `#cwL` — the luminance run, 4…96.
                {
                    const PlaneExtent Track = Spanning(Anchored.LeastAlong + 44.0f, Across,
                                                       PopupAlongLeast - 56.0f, 16.0f);

                    Surface->TextRunCapitalised(Anchored.LeastAlong + 12.0f,
                                                Across + 8.0f - Surface->RunAcross(Scaled.RunSection) * 0.5f,
                                                Tinted.Faint, "Lum", Scaled.RunSection, TrackingSection, true);

                    auto Reading = static_cast<std::uint32_t>(Seated.WheelLuminance);

                    if (Dragged(ChromeCells[static_cast<std::uint32_t>(ChromeCell::WheelLuma)],
                                Track, Reading))
                    {
                        const auto Bounded = (Reading < 4u) ? 4u : ((Reading > 96u) ? 96u : Reading);
                        Seated.WheelLuminance = static_cast<float>(Bounded);
                    }

                    RecordMeter(PlaneExtent{ Track.LeastAlong, Across + 6.5f, Track.MostAlong, Across + 9.5f },
                                static_cast<std::uint32_t>(Seated.WheelLuminance), Tinted.Accent);

                    Across += 26.0f;
                }

                // 📐 `#cwPrev` and `#cwOk` — the resolved tint beside the Apply.
                {
                    const auto Tint = CombineTint(Seated.WheelHue, Seated.WheelSaturation,
                                                  Seated.WheelLuminance);

                    Surface->Ground(Spanning(Anchored.LeastAlong + 12.0f, Across, 28.0f, 24.0f),
                                    Covering(Tint), Scaled.RadiusSmall);

                    char Written[16] = {};
                    std::snprintf(Written, sizeof Written, "#%06X", Tint);
                    Surface->TextRun(Anchored.LeastAlong + 48.0f,
                                     Across + 12.0f - Surface->RunAcross(Scaled.RunSub) * 0.5f,
                                     Tinted.Secondary, Written, Scaled.RunSub);

                    const PlaneExtent Apply = Spanning(Anchored.MostAlong - 74.0f, Across, 62.0f, 24.0f);
                    const bool        Over  = Apply.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

                    Surface->Ground(Apply, Over ? Tinted.Accent : Partial(0xFFFFFFu, 0.09),
                                    Scaled.RadiusSmall);
                    Surface->TextRun(Apply.LeastAlong + 16.0f,
                                     Across + 12.0f - Surface->RunAcross(Scaled.RunSub) * 0.5f,
                                     Over ? Covering(0x000000u) : Tinted.Primary, "Apply", Scaled.RunSub,
                                     0.0f, true);

                    if (Over && Sampled.ContactReleased && Present)
                    {
                        Revisions.Record(Arrangement, "Colour tag amended");
                        Arrangement.Entries[Subject].ColourTag = Tint;
                        Seated.Popup = StackPopup::Absent;
                        Ledger->Withdraw();
                    }

                    Across += 32.0f;
                }

                break;
            }

            default:
                break;
        }

        Surface->Release();

        // 🔴 Set LAST. A popup opened during this very sweep must not resolve one of its own entries under
        //    the same contact — it becomes pickable on the next tick and not before.
        if (Seated.Popup != StackPopup::Absent)
            Seated.PopupSettled = true;
    }

    // ② The tooltip, above even the popup, because it may name one of the popup's own entries.
    if (Seated.Tooltip != nullptr && Seated.Tooltip[0] != '\0' && Seated.Popup == StackPopup::Absent)
    {
        const float Along  = Surface->MeasureRun(Seated.Tooltip, Scaled.RunFine) + 16.0f;
        const float Across = 22.0f;

        float Seat = Seated.TooltipAlong - Along * 0.5f;
        float Over = Seated.TooltipAcross - Across - 6.0f;

        const float ExtentAlong = (Surface->Display().ExtentAlong > 0.0f)
                                ? Surface->Display().ExtentAlong : 1920.0f;

        if (Seat < 8.0f)                     Seat = 8.0f;
        if (Seat > ExtentAlong - Along - 8.0f) Seat = ExtentAlong - Along - 8.0f;
        if (Over < 8.0f)                     Over = Seated.TooltipAcross + 24.0f;

        const PlaneExtent Card = Spanning(Seat, Over, Along, Across);

        Surface->Ground(Card, Covering(0x1A1A1Au), Scaled.RadiusSmall);
        Surface->Edge(Card, Tinted.StrokeStrong, 1.0f, Scaled.RadiusSmall);
        Surface->TextRun(Seat + 8.0f, Over + (Across - Surface->RunAcross(Scaled.RunFine)) * 0.5f,
                         Tinted.Primary, Seated.Tooltip, Scaled.RunFine);
    }
}

}   // namespace Slate
