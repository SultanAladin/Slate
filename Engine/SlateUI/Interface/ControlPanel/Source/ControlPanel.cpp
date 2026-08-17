//============================================================================================================================================
//                                                            CONTROLPANEL.CPP
//============================================================================================================================================
// 🧩 Reference inspector controls recorded from fixed figures and arbitrated through one interaction index.

#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   REFERENCE APPEARANCE
//------------------------------------------------------------------------------------------------------------------------

namespace
{

constexpr InkOrdinate PanelGround     = Covering(0x101012u);
constexpr InkOrdinate FieldGround     = Covering(0x0A0A0Bu);
constexpr InkOrdinate TileGround      = Covering(0x1D1D21u);
constexpr InkOrdinate TileRoused      = Covering(0x26262Bu);
constexpr InkOrdinate HairInk         = Partial(0xFFFFFFu, 0.06);
constexpr InkOrdinate StrongHairInk   = Partial(0xFFFFFFu, 0.10);
constexpr InkOrdinate AccentInk       = Covering(0x4A90E2u);
constexpr InkOrdinate AccentSoftInk   = Partial(0xFFFFFFu, 0.12);
constexpr InkOrdinate TrackTakenInk   = Covering(0x8A8A8Eu);
constexpr InkOrdinate PrimaryInk      = Covering(0xECECF0u);
constexpr InkOrdinate MutedInk        = Covering(0x7B7B82u);
constexpr InkOrdinate FaintInk        = Covering(0x55555Du);
constexpr InkOrdinate WhiteInk        = Covering(0xFFFFFFu);
constexpr InkOrdinate AbsentInk       = Covering(0x303036u);

constexpr float ReferenceText         = 12.0f;   // [px] - default ImGui typeface presentation size
constexpr float SmallText             = 10.5f;   // [px] - metadata and counts
constexpr float ControlRadius         = 8.0f;    // [px] - tile and segment corner radius
constexpr double RouseDuration        = 120.0;   // [ms] - reference transition-colors duration
constexpr double TakeDuration         = 160.0;   // [ms] - switch and selection transition duration

constexpr float CentredAcross(const PlaneExtent& Extent, float PointSize)
{
    return Extent.LeastAcross + (Extent.SpanAcross() - PointSize) * 0.5f;
}

void UnsignedRun(char* Delivered, std::uint32_t Extent, std::uint32_t Reading)
{
    if (Delivered == nullptr || Extent < 2u)
        return;

    char          Reversed[12] = {};
    std::uint32_t Count        = 0u;

    do
    {
        Reversed[Count++] = static_cast<char>('0' + Reading % 10u);
        Reading /= 10u;
    }
    while (Reading > 0u && Count < 11u);

    std::uint32_t Written = 0u;

    while (Count > 0u && Written + 1u < Extent)
        Delivered[Written++] = Reversed[--Count];

    Delivered[Written] = '\0';
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                       CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Deliver<bool> ControlPanel::Construct(InteractionIndex&              ArrivingInteraction,
                                      RecordingSurface&              ArrivingRecording,
                                      const AppearanceSpecification& ArrivingAppearance)
{
    if (Interaction != nullptr)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "a control panel construction already stands" });

    Interaction = &ArrivingInteraction;
    Recording   = &ArrivingRecording;
    Appearance  = &ArrivingAppearance;

    return Deliver<bool>::Deliver(true);
}

void ControlPanel::Advance(const PointerCondition& Arriving, double Elapsed)
{
    if (Interaction == nullptr)
        return;

    Arrived = Arriving;
    static_cast<void>(Elapsed);
}

ControlVerdict ControlPanel::ResolveTap(ControlIdentity Claimed, const PlaneExtent& Extent, bool& Altered)
{
    ControlVerdict Verdict;

    if (Interaction == nullptr || Recording == nullptr || !Interaction->Resolves(Claimed))
        return Verdict;

    const bool Roused = Extent.Encloses(Arrived.PositionAlong, Arrived.PositionAcross);

    if (Roused && Arrived.ContactArrived && !Interaction->AnyDisclosed())
        Interaction->Seize(Claimed, ControlPart::Body);

    if (Interaction->Released(Claimed) && Roused)
    {
        Altered                  = true;
        Verdict.OrdinateAltered = true;
    }

    Interaction->DeclareRoused(Claimed, Roused, RouseDuration);
    Verdict.ContactTaken = Interaction->Holding(Claimed);
    Verdict.Mark         = (Roused || Verdict.ContactTaken) ? RedrawMark::Recolour : RedrawMark::Quiet;

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                          SWITCH
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ControlPanel::SwitchToggle(ControlIdentity Claimed, const PlaneExtent& Extent,
                                          const SwitchDeclaration& Declared, bool& Taken)
{
    bool           Altered = false;
    ControlVerdict Verdict = ResolveTap(Claimed, Extent, Altered);

    if (Altered)
        Taken = !Taken;

    if (Interaction == nullptr || Recording == nullptr)
        return Verdict;

    Interaction->DeclareTaken(Claimed, Taken, TakeDuration);

    const float CaptionAlong = Extent.LeastAlong;
    const float TrackAlong   = Extent.MostAlong - 50.0f;
    const PlaneExtent Track  = Spanning(TrackAlong, Extent.LeastAcross, 50.0f, 32.0f);

    Recording->TextRun(CaptionAlong, CentredAcross(Extent, ReferenceText), MutedInk,
                       Declared.Caption, ReferenceText);
    Recording->Ground(Track, Taken ? TrackTakenInk : FieldGround, 16.0f, CornerAll);
    Recording->Edge(Track, Taken ? TrackTakenInk : HairInk, 1.0f, 16.0f, CornerAll);

    const float NubAlong = Taken ? Track.MostAlong - 16.0f : Track.LeastAlong + 16.0f;
    Recording->Medallion(NubAlong, Track.LeastAcross + 16.0f, 12.0f, WhiteInk);

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    SEGMENTED CHOICE
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ControlPanel::SegmentedChoice(ControlIdentity Claimed, const PlaneExtent& Extent,
                                             const SegmentDeclaration& Declared, std::uint32_t& TakenOrdinal)
{
    ControlVerdict Verdict;

    if (Interaction == nullptr || Recording == nullptr || Declared.Captions == nullptr || Declared.CaptionCount == 0u)
        return Verdict;

    const float SegmentAlong = Extent.SpanAlong() / static_cast<float>(Declared.CaptionCount);
    std::uint32_t RousedOrdinal = Declared.CaptionCount;

    for (std::uint32_t Ordinal = 0u; Ordinal < Declared.CaptionCount; ++Ordinal)
    {
        const PlaneExtent Segment = Spanning(Extent.LeastAlong + SegmentAlong * static_cast<float>(Ordinal),
                                             Extent.LeastAcross, SegmentAlong, Extent.SpanAcross());

        if (Segment.Encloses(Arrived.PositionAlong, Arrived.PositionAcross))
            RousedOrdinal = Ordinal;
    }

    const bool Roused = RousedOrdinal < Declared.CaptionCount;

    if (Roused && Arrived.ContactArrived)
    {
        Interaction->Seize(Claimed, ControlPart::Body);
        Interaction->DepartFrom(Claimed, static_cast<float>(RousedOrdinal));
    }

    if (Interaction->Released(Claimed) && Roused)
    {
        TakenOrdinal             = RousedOrdinal;
        Verdict.OrdinateAltered = true;
    }

    Interaction->DeclareRoused(Claimed, Roused, RouseDuration);
    Interaction->DeclareTaken(Claimed, true, TakeDuration);

    Recording->Ground(Extent, FieldGround, ControlRadius, CornerAll);
    Recording->Edge(Extent, HairInk, 1.0f, ControlRadius, CornerAll);

    for (std::uint32_t Ordinal = 0u; Ordinal < Declared.CaptionCount; ++Ordinal)
    {
        const PlaneExtent Segment = Spanning(Extent.LeastAlong + SegmentAlong * static_cast<float>(Ordinal),
                                             Extent.LeastAcross, SegmentAlong, Extent.SpanAcross());
        const bool Taken = Ordinal == TakenOrdinal;

        if (Taken)
        {
            const PlaneExtent Inset = { Segment.LeastAlong + 3.0f, Segment.LeastAcross + 3.0f,
                                        Segment.MostAlong - 3.0f, Segment.MostAcross - 3.0f };
            Recording->Ground(Inset, AccentSoftInk, 6.0f, CornerAll);
            Recording->Edge(Inset, AccentInk, 1.0f, 6.0f, CornerAll);
        }

        const float RunAlong = Recording->MeasureRun(Declared.Captions[Ordinal], ReferenceText);
        Recording->TextRun(Segment.LeastAlong + (Segment.SpanAlong() - RunAlong) * 0.5f,
                           CentredAcross(Segment, ReferenceText), Taken ? PrimaryInk : MutedInk,
                           Declared.Captions[Ordinal], ReferenceText, 0.0f, Taken);
    }

    Verdict.ContactTaken = Interaction->Holding(Claimed);
    Verdict.Mark         = Roused ? RedrawMark::Recolour : RedrawMark::Quiet;

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                          TAB STRIP
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ControlPanel::TabStrip(ControlIdentity Claimed, const PlaneExtent& Extent,
                                      const TabDeclaration& Declared, std::uint32_t& TakenOrdinal)
{
    ControlVerdict Verdict;

    if (Interaction == nullptr || Recording == nullptr || Declared.Captions == nullptr || Declared.CaptionCount == 0u)
        return Verdict;

    const float TabAlong = Extent.SpanAlong() / static_cast<float>(Declared.CaptionCount);
    std::uint32_t RousedOrdinal = Declared.CaptionCount;

    for (std::uint32_t Ordinal = 0u; Ordinal < Declared.CaptionCount; ++Ordinal)
    {
        const PlaneExtent Tab = Spanning(Extent.LeastAlong + TabAlong * static_cast<float>(Ordinal),
                                         Extent.LeastAcross, TabAlong, Extent.SpanAcross());
        if (Tab.Encloses(Arrived.PositionAlong, Arrived.PositionAcross))
            RousedOrdinal = Ordinal;
    }

    const bool Roused = RousedOrdinal < Declared.CaptionCount;

    if (Roused && Arrived.ContactArrived)
        Interaction->Seize(Claimed, ControlPart::Body);

    if (Interaction->Released(Claimed) && Roused)
    {
        TakenOrdinal             = RousedOrdinal;
        Verdict.OrdinateAltered = true;
    }

    Recording->Ground(Extent, PanelGround, 0.0f, CornerNone);
    Recording->Ground(Spanning(Extent.LeastAlong, Extent.MostAcross - 1.0f, Extent.SpanAlong(), 1.0f), HairInk, 0.0f, CornerNone);

    for (std::uint32_t Ordinal = 0u; Ordinal < Declared.CaptionCount; ++Ordinal)
    {
        const PlaneExtent Tab = Spanning(Extent.LeastAlong + TabAlong * static_cast<float>(Ordinal),
                                         Extent.LeastAcross, TabAlong, Extent.SpanAcross());
        const bool Taken = Ordinal == TakenOrdinal;
        const float RunAlong = Recording->MeasureRun(Declared.Captions[Ordinal], ReferenceText);

        Recording->TextRun(Tab.LeastAlong + (Tab.SpanAlong() - RunAlong) * 0.5f,
                           CentredAcross(Tab, ReferenceText), Taken ? PrimaryInk : MutedInk,
                           Declared.Captions[Ordinal], ReferenceText, 0.0f, Taken);

        if (Taken)
            Recording->Ground(Spanning(Tab.LeastAlong + 6.0f, Tab.MostAcross - 2.0f,
                                       Tab.SpanAlong() - 12.0f, 2.0f), AccentInk, 0.0f, CornerNone);
    }

    Interaction->DeclareRoused(Claimed, Roused, RouseDuration);
    Verdict.ContactTaken = Interaction->Holding(Claimed);
    Verdict.Mark         = Roused ? RedrawMark::Recolour : RedrawMark::Quiet;

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       FOLDING CARD
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ControlPanel::CollapsibleCard(ControlIdentity Claimed, const PlaneExtent& Extent,
                                             const FoldDeclaration& Declared, bool& ExpansionEnabled)
{
    bool           Altered = false;
    ControlVerdict Verdict = ResolveTap(Claimed, Extent, Altered);

    if (Altered)
        ExpansionEnabled = !ExpansionEnabled;

    if (Recording == nullptr)
        return Verdict;

    Recording->Ground(Extent, PanelGround, ControlRadius, CornerAll);
    Recording->Edge(Extent, HairInk, 1.0f, ControlRadius, CornerAll);

    const PlaneExtent SymbolExtent = Spanning(Extent.LeastAlong + 8.0f, Extent.LeastAcross + 8.0f, 14.0f, 14.0f);
    Recording->Stroke(ExpansionEnabled ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                      SymbolExtent, FaintInk);
    Recording->TextRun(Extent.LeastAlong + 30.0f, CentredAcross(Extent, SmallText), MutedInk,
                       Declared.Caption, SmallText, 0.08f, true);

    char CountRun[12] = {};
    UnsignedRun(CountRun, 12u, Declared.Count);
    const float CountAlong = Recording->MeasureRun(CountRun, SmallText);
    Recording->TextRun(Extent.MostAlong - CountAlong - 10.0f, CentredAcross(Extent, SmallText),
                       FaintInk, CountRun, SmallText);

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        OUTLINE ROW
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ControlPanel::OutlineRow(ControlIdentity Claimed, const PlaneExtent& Extent,
                                        const OutlineDeclaration& Declared, bool SelectionExtended,
                                        bool& Selected, bool& PresenceEnabled)
{
    ControlVerdict Verdict;

    if (Interaction == nullptr || Recording == nullptr)
        return Verdict;

    const PlaneExtent PresenceExtent = Spanning(Extent.MostAlong - 27.0f, Extent.LeastAcross + 3.0f, 22.0f, 22.0f);
    const bool PresenceRoused = PresenceExtent.Encloses(Arrived.PositionAlong, Arrived.PositionAcross);
    const bool RowRoused      = Extent.Encloses(Arrived.PositionAlong, Arrived.PositionAcross);

    if (RowRoused && Arrived.ContactArrived)
    {
        Interaction->Seize(Claimed, PresenceRoused ? ControlPart::Chevron : ControlPart::Body);
    }

    if (Interaction->Released(Claimed) && RowRoused)
    {
        if (PresenceRoused)
            PresenceEnabled = !PresenceEnabled;
        else if (SelectionExtended)
            Selected = !Selected;
        else
            Selected = true;

        Verdict.OrdinateAltered = true;
    }

    if (Selected)
    {
        Recording->Ground(Extent, AccentSoftInk, 5.0f, CornerAll);
        Recording->Ground(Spanning(Extent.LeastAlong, Extent.LeastAcross, 2.0f, Extent.SpanAcross()), AccentInk,
                          1.0f, CornerAll);
    }
    else if (RowRoused)
    {
        Recording->Ground(Extent, TileRoused, 5.0f, CornerAll);
    }

    const float Indent = 7.0f + static_cast<float>(Declared.Depth) * 17.0f;

    if (Declared.EnclosedCount > 0u)
    {
        Recording->Stroke(Declared.ExpansionEnabled ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                          Spanning(Extent.LeastAlong + Indent, Extent.LeastAcross + 7.0f, 12.0f, 12.0f), FaintInk);
    }

    Recording->Stroke(SymbolSubject::PlaceholderMark,
                      Spanning(Extent.LeastAlong + Indent + 17.0f, Extent.LeastAcross + 5.0f, 16.0f, 16.0f),
                      Declared.PresenceEnabled ? AccentInk : FaintInk);
    Recording->TextRunTruncated(Extent.LeastAlong + Indent + 39.0f, CentredAcross(Extent, ReferenceText),
                                Extent.MostAlong - 34.0f, Declared.PresenceEnabled ? PrimaryInk : FaintInk,
                                Declared.Caption, ReferenceText, Selected);

    Recording->Edge(PresenceExtent, PresenceRoused ? StrongHairInk : HairInk, 1.0f, 5.0f, CornerAll);
    Recording->Medallion(PresenceExtent.LeastAlong + 11.0f, PresenceExtent.LeastAcross + 11.0f,
                         Declared.PresenceEnabled ? 3.0f : 1.5f,
                         Declared.PresenceEnabled ? MutedInk : AbsentInk);

    Interaction->DeclareRoused(Claimed, RowRoused, RouseDuration);
    Interaction->DeclareTaken(Claimed, Selected, TakeDuration);
    Verdict.ContactTaken = Interaction->Holding(Claimed);
    Verdict.Mark         = RowRoused ? RedrawMark::Recolour : RedrawMark::Quiet;

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      REVISION MARKER
//------------------------------------------------------------------------------------------------------------------------

void ControlPanel::RevisionRow(const PlaneExtent& Extent, const RevisionDeclaration& Declared, bool Taken)
{
    if (Recording == nullptr)
        return;

    const float MarkerAlong = Extent.LeastAlong + 12.0f;
    Recording->Ground(Spanning(MarkerAlong - 0.5f, Extent.LeastAcross, 1.0f, Extent.SpanAcross()), HairInk,
                      0.0f, CornerNone);
    Recording->Medallion(MarkerAlong, Extent.LeastAcross + Extent.SpanAcross() * 0.5f, Taken ? 5.0f : 4.0f,
                         Taken ? AccentInk : FaintInk);

    const PlaneExtent Card = { Extent.LeastAlong + 28.0f, Extent.LeastAcross + 3.0f,
                               Extent.MostAlong, Extent.MostAcross - 3.0f };
    Recording->Ground(Card, Taken ? AccentSoftInk : TileGround, 7.0f, CornerAll);
    Recording->Edge(Card, Taken ? AccentInk : HairInk, 1.0f, 7.0f, CornerAll);
    Recording->TextRunTruncated(Card.LeastAlong + 9.0f, Card.LeastAcross + 7.0f, Card.MostAlong - 58.0f,
                                PrimaryInk, Declared.Description, ReferenceText, true);
    Recording->TextRunTruncated(Card.LeastAlong + 9.0f, Card.LeastAcross + 24.0f, Card.MostAlong - 58.0f,
                                MutedInk, Declared.Secondary, SmallText);

    const float TimeAlong = Recording->MeasureRun(Declared.TimeRun, SmallText);
    Recording->TextRun(Card.MostAlong - TimeAlong - 8.0f, Card.LeastAcross + 7.0f,
                       FaintInk, Declared.TimeRun, SmallText);
}

void ControlPanel::Reset()
{
    Interaction = nullptr;
    Recording   = nullptr;
    Appearance  = nullptr;
    Arrived     = {};
}

}   // namespace Slate