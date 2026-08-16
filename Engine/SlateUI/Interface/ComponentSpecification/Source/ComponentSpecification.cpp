//============================================================================================================================================
//                                                            COMPONENTSPECIFICATION.CPP
//============================================================================================================================================
// 🧩 The eight controls, arranged and recorded from the sheet's own figures, arbitrated against the ledger's one seizure.

#include "SlateUI/Interface/ComponentSpecification/Api/ComponentSpecification.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     SHARED ARITHMETIC
//------------------------------------------------------------------------------------------------------------------------

namespace
{

constexpr double RouseDuration    = 200.0;   // [ms] - the sheet's transition-colors on a hover
constexpr double DiscloseDuration = 150.0;   // [ms] - the accordion and the chevron's turn
constexpr double TooltipDuration  = 300.0;   // [ms] - duration-300 on the tooltip's opacity

/// 🧩 Interpolates between two ordinates by a fraction already clamped to the unit interval.
/// cost  ✔️
constexpr float Between(float Departed, float Arriving, float Fraction)
{
    return Departed + (Arriving - Departed) * Fraction;
}

/// 🧩 Blends two inks by a fraction, component by component, in the display-referred encoding they are in.
/// note  ⚠️ Blended where they are declared — display-referred — and not in a linear space. `08` §3.1 places
///       the interface after the tone projection, and a fade that linearised would disagree with the browser
///       the sheet was measured in, which interpolates sRGB ordinates exactly as this does.
/// cost  ✔️
constexpr std::uint8_t BlendOrdinate(std::uint8_t Departed, std::uint8_t Arriving, float Fraction)
{
    return static_cast<std::uint8_t>(
        Between(static_cast<float>(Departed), static_cast<float>(Arriving), Fraction) + 0.5f);
}

constexpr InkOrdinate Blend(InkOrdinate Departed, InkOrdinate Arriving, float Fraction)
{
    const float Held = (Fraction < 0.0f) ? 0.0f : (Fraction > 1.0f) ? 1.0f : Fraction;

    return InkOrdinate{ BlendOrdinate(Departed.Red,     Arriving.Red,     Held),
                        BlendOrdinate(Departed.Green,   Arriving.Green,   Held),
                        BlendOrdinate(Departed.Blue,    Arriving.Blue,    Held),
                        BlendOrdinate(Departed.Opacity, Arriving.Opacity, Held) };
}

/// 🧩 Restates an ink at a fraction of its own coverage — what a fading tooltip records with.
/// cost  ✔️
constexpr InkOrdinate Faded(InkOrdinate Declared, float Fraction)
{
    const float Held = (Fraction < 0.0f) ? 0.0f : (Fraction > 1.0f) ? 1.0f : Fraction;

    InkOrdinate Restated = Declared;
    Restated.Opacity     = static_cast<std::uint8_t>(static_cast<float>(Declared.Opacity) * Held + 0.5f);

    return Restated;
}

/// 🧩 Holds an ordinate inside a stated interval.
/// cost  ✔️
constexpr double Held(double Ordinate, double Least, double Most)
{
    return (Ordinate < Least) ? Least : (Ordinate > Most) ? Most : Ordinate;
}

/// 🧩 The extent one run occupies, centred across a stated extent at its own leading.
/// note  📐 The vendor places a run by its upper edge, and every extent the sheet states is centred within
///        its row. Deriving the upper edge here rather than at nineteen call sites is what keeps a run from
///        sitting a pixel high in one control and correct in the next.
/// cost  ✔️
constexpr float CentredAcross(const PlaneExtent& Extent, float PointSize)
{
    return Extent.LeastAcross + (Extent.SpanAcross() - PointSize) * 0.5f;
}

/// 🧩 Rounds an integral reading to its decimal run, without allocating.
/// in    Staging   [-]  receives the run; at least twelve characters
/// note  Written rather than reached for through a formatting call, because no formatting call in the
///       standard library is both non-allocating and available at this seam.
/// cost  ✔️
void IntegralRun(char* Staging, std::uint32_t StagingExtent, long long Reading)
{
    if (Staging == nullptr || StagingExtent < 2u)
        return;

    char          Reversed[20] = {};
    std::uint32_t Digits       = 0u;
    const bool    Negative     = Reading < 0;

    if (Negative)
        Reading = -Reading;

    do
    {
        Reversed[Digits++] = static_cast<char>('0' + (Reading % 10));
        Reading /= 10;
    }
    while (Reading > 0 && Digits < 19u);

    std::uint32_t Written = 0u;

    if (Negative && Written < StagingExtent - 1u)
        Staging[Written++] = '-';

    for (std::uint32_t Ordinal = Digits; Ordinal > 0u && Written < StagingExtent - 1u; --Ordinal)
        Staging[Written++] = Reversed[Ordinal - 1u];

    Staging[Written] = '\0';
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE FREE PROJECTIONS
//------------------------------------------------------------------------------------------------------------------------

double MagnitudeFraction(double Ordinate, double Least, double Most)
{
    if (Most <= Least)
        return 0.0;

    return Held((Ordinate - Least) / (Most - Least), 0.0, 1.0);
}

double RotationDegrees(double Departed, double TravelAlong, double DegreesPerPixel)
{
    return Departed - TravelAlong * DegreesPerPixel;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Deliver<bool> ComponentSpecification::Construct(InteractionIndex&              Ledger,
                                                RecordingSurface&              Surface,
                                                const AppearanceSpecification& Appearance)
{
    if (this->Ledger != nullptr)
        return Deliver<bool>::Refuse(DeliveryRefusal::ContentUnsupported);

    this->Ledger     = &Ledger;
    this->Surface    = &Surface;
    this->Appearance = &Appearance;

    return Deliver<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     ARBITRATION
//------------------------------------------------------------------------------------------------------------------------

void ComponentSpecification::Advance(const PointerCondition& Arrived, double Elapsed)
{
    this->Arrived        = Arrived;
    DeferredCount        = 0u;
    ContactHeldByPanel   = false;
    Standing             = RedrawMark::Quiet;

    Ledger->Advance(Arrived, Elapsed);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     CARD ARRANGEMENT
//------------------------------------------------------------------------------------------------------------------------

CardArrangement ComponentSpecification::ArrangeCard(float Along, float Across, float ExtentAlong,
                                                    const float* RowExtents, std::uint32_t RowCount) const
{
    const ControlMetric& M = Appearance->ControlMeasure;
    const float InteriorAlong  = Along + M.CardPad;
    const float InteriorAcross = Across + M.CardPad;
    const float InteriorExtent = ExtentAlong - M.CardPad * 2.0f;

    float TotalAcross = 0.0f;

    for (std::uint32_t Ordinal = 0u; Ordinal < RowCount; ++Ordinal)
        TotalAcross += RowExtents[Ordinal];

    if (RowCount > 1u)
        TotalAcross += static_cast<float>(RowCount - 1u) * M.CardRowGap;

    const float CardAcross = TotalAcross + M.CardPad * 2.0f;

    CardArrangement Arranged = {};
    Arranged.Enclosure = Spanning(Along, Across, ExtentAlong, CardAcross);
    Arranged.Interior  = Spanning(InteriorAlong, InteriorAcross, InteriorExtent, TotalAcross);
    Arranged.RowGap    = M.CardRowGap;

    return Arranged;
}

void ComponentSpecification::RecordCard(const CardArrangement& Arranged)
{
    const ControlInk& Ink = Appearance->Control;
    Surface->Ground(Arranged.Enclosure, Ink.CardGround, Appearance->ControlMeasure.CardRadius, CornerAll);
    Surface->Edge(Arranged.Enclosure, Ink.CardEdge, Appearance->ControlMeasure.CardEdgeWeight,
                  Appearance->ControlMeasure.CardRadius, CornerAll);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SELECTION FIELD
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ComponentSpecification::SelectionField(ControlIdentity Claimed, const PlaneExtent& Row,
                                                      const SelectionDeclaration& Declared, std::uint32_t& TakenOrdinal)
{
    const ControlInk&    Ink     = Appearance->Control;
    const ControlMetric& M       = Appearance->ControlMeasure;
    const float          Across  = CentredAcross(Row, M.FieldAcross);

    const PlaneExtent Field = Spanning(Row.LeastAlong, Across, Row.SpanAlong(), M.FieldAcross);
    const PlaneExtent Cell  = Spanning(Field.MostAlong - M.ChevronCellAlong, Across,
                                       M.ChevronCellAlong, M.FieldAcross);

    const bool Roused = Field.Encloses(Arrived.PositionAlong, Arrived.PositionAcross);
    const bool Taken  = Ledger->Holding(Claimed);

    const float Rouse = Ledger->RousedFraction(Claimed);
    const float Take  = Ledger->TakenFraction(Claimed);

    const InkOrdinate FieldInk = Blend(Ink.FieldGround, Ink.FieldGround, Take);

    Surface->Ground(Field, FieldInk, M.RadiusFine, CornerAll);

    const InkOrdinate CellInk = Roused ? Ink.CellGroundRoused : Ink.CellGround;
    Surface->Ground(Cell, CellInk, M.RadiusFine, CornerAll);

    // The label run, centred in the field.
    const float LabelAcross = CentredAcross(Field, M.RowText);
    Surface->TextRunTruncated(Field.LeastAlong + M.FieldPadAlong, LabelAcross,
                              Field.MostAlong - M.ChevronCellAlong - M.FieldPadAlong,
                              Ink.FieldInk, Declared.Options[TakenOrdinal], M.RowText);

    // The chevron, stroked in the cell.
    Stroke(ChevronDown, Spanning(Cell.LeastAlong + (M.ChevronCellAlong - M.ChevronSymbol) * 0.5f,
                                 CentredAcross(Cell, M.ChevronSymbol),
                                 M.ChevronSymbol, M.ChevronSymbol), Ink.CellInk);

    // Interaction.
    Ledger->DeclareRoused(Claimed, Roused, RouseDuration);

    ControlVerdict Verdict = {};
    Verdict.Mark = RedrawMark::Quiet;

    if (Arrived.ContactArrived && Roused)
    {
        if (Taken)
        {
            Ledger->Withdraw();
            Verdict.Mark = RedrawMark::Content;
        }
        else if (Ledger->Seize(Claimed, ControlPart::Chevron))
        {
            Ledger->Disclose(Claimed);
            Verdict.Mark = RedrawMark::Content;
        }
    }

    if (Taken)
    {
        if (Arrived.ContactReleased && Ledger->Released(Claimed))
        {
            Ledger->Withdraw();
            Verdict.Mark = RedrawMark::Content;
        }

        // The open menu is deferred.
        if (DeferredCount < DeferredCeiling)
        {
            Deferred[DeferredCount].Claimed     = Claimed;
            Deferred[DeferredCount].Anchor      = Field;
            Deferred[DeferredCount].Options     = Declared.Options;
            Deferred[DeferredCount].OptionCount = Declared.OptionCount;
            Deferred[DeferredCount].TakenOption = TakenOrdinal;
            Deferred[DeferredCount].Menu        = true;
            ++DeferredCount;
        }
    }

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE MAGNITUDE ROW
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ComponentSpecification::MagnitudeRow(ControlIdentity Claimed, const PlaneExtent& Row,
                                                    const MagnitudeDeclaration& Declared, double& Ordinate)
{
    const ControlInk&    Ink     = Appearance->Control;
    const ControlMetric& M       = Appearance->ControlMeasure;
    const float          Across  = CentredAcross(Row, M.FieldAcross);

    // Parts of the row.
    const PlaneExtent Readout  = Spanning(Row.LeastAlong, Across, M.ReadoutAlong, M.FieldAcross);
    const PlaneExtent UnitCell = Spanning(Readout.MostAlong + M.RowGapAlong, Across, M.UnitCellAlong, M.FieldAcross);
    const PlaneExtent Track    = Spanning(UnitCell.MostAlong + M.RowGapAlong, Across,
                                          Row.SpanAlong() - M.ReadoutAlong - M.UnitCellAlong - M.RowGapAlong * 2.0f,
                                          M.SliderAcross);

    const bool Roused = Row.Encloses(Arrived.PositionAlong, Arrived.PositionAcross);
    const bool Held   = Ledger->Holding(Claimed);

    const float Rouse = Ledger->RousedFraction(Claimed);

    // Readout ground.
    Surface->Ground(Readout, Ink.FieldGround, M.RadiusFine, CornerAll);

    // Readout run.
    char Reading[20] = {};
    IntegralRun(Reading, sizeof(Reading), static_cast<long long>(Ordinate + 0.5));

    const float ReadingAcross = CentredAcross(Readout, M.ReadoutText);
    Surface->TextRun(Readout.LeastAlong + M.FieldPadAlong, ReadingAcross,
                     Ink.FieldInk, Reading, M.ReadoutText, M.ReadoutTracking);

    // Unit cell.
    Surface->Ground(UnitCell, Ink.CellGround, M.RadiusFine, CornerAll);

    const float UnitAcross = CentredAcross(UnitCell, M.UnitText);
    Surface->TextRun(UnitCell.LeastAlong + (M.UnitCellAlong - M.UnitText) * 0.5f, UnitAcross,
                     Ink.UnitInk, Declared.UnitGlyph, M.UnitText);

    // Track.
    const float Fraction = MagnitudeFraction(Ordinate, Declared.LeastOrdinal, Declared.MostOrdinal);
    const float TakenAlong = Track.SpanAlong() * Fraction;

    const PlaneExtent TrackTaken = Spanning(Track.LeastAlong, Track.LeastAcross,
                                            Track.SpanAlong(), Track.SpanAcross());
    const PlaneExtent TrackBelow = Spanning(Track.LeastAlong + TakenAlong, Track.LeastAcross,
                                            Track.SpanAlong() - TakenAlong, Track.SpanAcross());

    Surface->Ground(TrackBelow, Ink.TrackQuiet, M.ThumbExtent * 0.5f, CornerAll);
    Surface->Ground(TrackTaken, Ink.TrackTaken, M.ThumbExtent * 0.5f, CornerAll);
    Surface->Edge(Track, Ink.TrackEdge, 1.0f, M.ThumbExtent * 0.5f, CornerAll);

    // Thumb.
    const float ThumbAlong = Track.LeastAlong + TakenAlong - M.ThumbExtent * 0.5f;
    const float ThumbAcross = CentredAcross(Track, M.ThumbExtent);
    Surface->Medallion(ThumbAlong + M.ThumbExtent * 0.5f, ThumbAcross + M.ThumbExtent * 0.5f,
                       M.ThumbExtent * 0.5f, Ink.ThumbGround);

    // Interaction.
    Ledger->DeclareRoused(Claimed, Roused, RouseDuration);

    ControlVerdict Verdict = {};
    Verdict.Mark = RedrawMark::Quiet;

    if (Arrived.ContactArrived && Roused)
    {
        const PlaneExtent ThumbExtent = Spanning(ThumbAlong, ThumbAcross, M.ThumbExtent, M.ThumbExtent);
        const ControlPart Part = ThumbExtent.Encloses(Arrived.PositionAlong, Arrived.PositionAcross)
                               ? ControlPart::Thumb : ControlPart::Track;

        if (Ledger->Seize(Claimed, Part))
        {
            Ledger->DepartFrom(Claimed, static_cast<float>(Ordinate));
            Verdict.Mark = RedrawMark::Content;
        }
    }

    if (Held && Ledger->Released(Claimed))
    {
        Verdict.Mark = RedrawMark::Content;
    }

    if (Held)
    {
        const Deliver<float> Departed = Ledger->DepartedOrdinate(Claimed);

        if (Departed.ContentPresent)
        {
            const float Travel = Arrived.PositionAlong - Ledger->OriginAlong();
            const double NewOrdinate = Held(Departed.Resolve() + Travel / (Track.SpanAlong() / (Declared.MostOrdinal - Declared.LeastOrdinal)),
                                            Declared.LeastOrdinal, Declared.MostOrdinal);

            if (NewOrdinate != Ordinate)
            {
                Ordinate         = NewOrdinate;
                Verdict.OrdinalAltered = true;
                Verdict.Mark     = RedrawMark::Content;
            }
        }
    }

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE ROTATION RULER
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ComponentSpecification::RotationRuler(ControlIdentity Claimed, const PlaneExtent& Row,
                                                     const RulerDeclaration& Declared, double& Degrees)
{
    const ControlInk&    Ink     = Appearance->Control;
    const ControlMetric& M       = Appearance->ControlMeasure;

    const bool Roused = Row.Encloses(Arrived.PositionAlong, Arrived.PositionAcross);
    const bool Held   = Ledger->Holding(Claimed);

    // The ground.
    Surface->Ground(Row, Ink.RulerGround, M.RulerRadius, CornerAll);
    Surface->Edge(Row, Ink.CardEdge, M.CardEdgeWeight, M.RulerRadius, CornerAll);

    // Ticks: major every tenth, medium every fifth, minor every other.
    const float CentreAlong = Row.LeastAlong + Row.SpanAlong() * 0.5f;
    const float CentreAcross = Row.LeastAcross + Row.SpanAcross() * 0.5f;
    const float Offset = static_cast<float>(static_cast<int>(Degrees / M.RulerDegreesPerPixel)) * M.TickSpacing;

    for (std::int32_t Ordinal = -static_cast<std::int32_t>(M.TickReach); Ordinal <= static_cast<std::int32_t>(M.TickReach); ++Ordinal)
    {
        const float TickAlong = CentreAlong + static_cast<float>(Ordinal) * M.TickSpacing - Offset;

        if (TickAlong < Row.LeastAlong || TickAlong > Row.MostAlong)
            continue;

        const bool IsMajor  = (Ordinal % 10 == 0);
        const bool IsMedium = (!IsMajor && Ordinal % 5 == 0);
        const bool IsMinor  = (!IsMajor && !IsMedium);

        const float TickAcross = IsMajor ? M.TickMajorAcross : IsMedium ? M.TickMediumAcross : M.TickMinorAcross;
        const InkOrdinate TickInk = IsMajor ? Ink.TickMajor : IsMedium ? Ink.TickMedium : Ink.TickMinor;

        const PlaneExtent Tick = Spanning(TickAlong - M.TickWeight * 0.5f,
                                          CentreAcross - TickAcross * 0.5f,
                                          M.TickWeight, TickAcross);
        Surface->Ground(Tick, TickInk);

        if (IsMajor)
        {
            char Caption[12] = {};
            long long DegreesShown = static_cast<long long>(Ordinal) * static_cast<long long>(M.RulerDegreesPerPixel * 10.0 + 0.5);
            IntegralRun(Caption, sizeof(Caption), DegreesShown);

            const float CaptionAcross = CentreAcross + M.TickCaptionLift;
            Surface->TextRun(TickAlong - M.MeasureRun(Caption, M.TickCaptionText) * 0.5f,
                             CaptionAcross, Ink.TickCaption, Caption, M.TickCaptionText);
        }
    }

    // Centre pointer.
    const PlaneExtent Pointer = Spanning(CentreAlong - M.PointerWeight * 0.5f,
                                         CentreAcross - M.PointerAcross * 0.5f,
                                         M.PointerWeight, M.PointerAcross);
    Surface->Ground(Pointer, Ink.RulerPointer);

    // Centre dot.
    Surface->Medallion(CentreAlong, CentreAcross + M.PointerDotLift, M.PointerDot * 0.5f, Ink.RulerPointer);

    // Readout.
    char Reading[20] = {};
    long long DegreesInt = static_cast<long long>(Degrees + 0.5);
    IntegralRun(Reading, sizeof(Reading), DegreesInt);

    const float ReadingAcross = CentredAcross(Row, M.ReadoutText);
    Surface->TextRun(Row.LeastAlong + M.FieldPadAlong, ReadingAcross,
                     Ink.FieldInk, Reading, M.ReadoutText, M.ReadoutTracking);

    // The readout is placed over the left side of the ruler.
    Surface->Ground(Spanning(Row.LeastAlong, Row.LeastAcross, M.ReadoutAlong + M.FieldPadAlong * 2.0f, Row.SpanAlong()),
                    Ink.FieldGround, M.RadiusFine, CornerAll);

    // Interaction.
    Ledger->DeclareRoused(Claimed, Roused, RouseDuration);

    ControlVerdict Verdict = {};
    Verdict.Mark = RedrawMark::Quiet;

    if (Arrived.ContactArrived && Roused)
    {
        if (Ledger->Seize(Claimed, ControlPart::Strip))
        {
            Ledger->DepartFrom(Claimed, static_cast<float>(Degrees));
            Verdict.Mark = RedrawMark::Content;
        }
    }

    if (Held && Ledger->Released(Claimed))
    {
        Verdict.Mark = RedrawMark::Content;
    }

    if (Held)
    {
        const Deliver<float> Departed = Ledger->DepartedOrdinate(Claimed);

        if (Departed.ContentPresent)
        {
            const float Travel = Arrived.PositionAlong - Ledger->OriginAlong();
            const double NewDegrees = RotationDegrees(Departed.Resolve(), Travel, M.RulerDegreesPerPixel);

            if (NewDegrees != Degrees)
            {
                Degrees         = NewDegrees;
                Verdict.OrdinalAltered = true;
                Verdict.Mark     = RedrawMark::Content;
            }
        }
    }

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE TOGGLE ROW
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ComponentSpecification::ToggleRow(ControlIdentity Claimed, const PlaneExtent& Row,
                                                 const ToggleDeclaration& Declared, bool& Taken)
{
    const ControlInk&    Ink     = Appearance->Control;
    const ControlMetric& M       = Appearance->ControlMeasure;
    const float          Across  = CentredAcross(Row, M.ToggleRowAcross);

    const PlaneExtent RingExtent = Spanning(Row.LeastAlong + M.ToggleRowPadAlong,
                                            Across + (M.ToggleRowAcross - M.RingExtent) * 0.5f,
                                            M.RingExtent, M.RingExtent);

    const float LabelAlong = RingExtent.MostAlong + M.ToggleGapAlong;
    const float LabelAcross = CentredAcross(Row, M.RowText);

    const bool Roused = Row.Encloses(Arrived.PositionAlong, Arrived.PositionAcross);
    const float Rouse = Ledger->RousedFraction(Claimed);
    const float Take  = Ledger->TakenFraction(Claimed);

    // Ring.
    const InkOrdinate RingInk = Taken ? Blend(Ink.RingQuiet, Ink.RingTaken, Take)
                                      : Blend(Ink.RingQuiet, Ink.RingRoused, Rouse);
    Surface->Medallion(RingExtent.LeastAlong + M.RingExtent * 0.5f,
                       RingExtent.LeastAcross + M.RingExtent * 0.5f,
                       M.RingExtent * 0.5f - M.RingWeight, RingInk);

    // Dot, when taken.
    if (Take > 0.01f)
    {
        const float DotExtent = M.RingDotExtent * Take;
        Surface->Medallion(RingExtent.LeastAlong + M.RingExtent * 0.5f,
                           RingExtent.LeastAcross + M.RingExtent * 0.5f,
                           DotExtent * 0.5f, Ink.RingDot);
    }

    // Label.
    const InkOrdinate LabelInk = Taken ? Blend(Ink.LabelQuiet, Ink.LabelTaken, Take)
                                       : Blend(Ink.LabelQuiet, Ink.LabelRoused, Rouse);
    Surface->TextRun(LabelAlong, LabelAcross, LabelInk, Declared.Caption, M.RowText);

    // Interaction.
    Ledger->DeclareRoused(Claimed, Roused, RouseDuration);
    Ledger->DeclareTaken(Claimed, Taken, RouseDuration);

    ControlVerdict Verdict = {};
    Verdict.Mark = RedrawMark::Quiet;

    if (Arrived.ContactArrived && Roused && Ledger->Seize(Claimed, ControlPart::Body))
    {
        Taken               = !Taken;
        Verdict.OrdinalAltered = true;
        Verdict.Mark        = RedrawMark::Content;
    }

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SUBSET ROW
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ComponentSpecification::SubsetRow(ControlIdentity Claimed, const PlaneExtent& Row,
                                                 const SubsetDeclaration& Declared, bool& Enrolled)
{
    const ControlInk&    Ink     = Appearance->Control;
    const ControlMetric& M       = Appearance->ControlMeasure;
    const float          Across  = CentredAcross(Row, M.SubsetRowAcross);

    const bool Roused = Row.Encloses(Arrived.PositionAlong, Arrived.PositionAcross);
    const float Rouse = Ledger->RousedFraction(Claimed);
    const float Take  = Ledger->TakenFraction(Claimed);

    // Row ground.
    const InkOrdinate GroundInk = Enrolled ? Ink.RowGroundTaken
                                           : Roused ? Ink.RowGroundRoused : Ink.RowGroundQuiet;
    Surface->Ground(Row, GroundInk);

    // Leading rail.
    const PlaneExtent Rail = Spanning(Row.LeastAlong, Across + (M.SubsetRowAcross - M.RailAcross) * 0.5f,
                                      M.SubsetRailAlong, M.RailAcross);
    const InkOrdinate RailInk = Enrolled ? Ink.RowRailTaken : Ink.RowRailQuiet;
    Surface->Ground(Rail, RailInk);

    // Label.
    const float LabelAlong = Row.LeastAlong + M.SubsetRowPadAlong;
    const float LabelAcrossPos = CentredAcross(Row, M.RowText);
    const InkOrdinate LabelInk = Enrolled ? Ink.LabelTaken : Roused ? Ink.LabelRoused : Ink.LabelQuiet;
    Surface->TextRun(LabelAlong, LabelAcrossPos, LabelInk, Declared.Caption, M.RowText);

    // Interaction.
    Ledger->DeclareRoused(Claimed, Roused, RouseDuration);
    Ledger->DeclareTaken(Claimed, Enrolled, RouseDuration);

    ControlVerdict Verdict = {};
    Verdict.Mark = RedrawMark::Quiet;

    if (Arrived.ContactArrived && Roused && Ledger->Seize(Claimed, ControlPart::Body))
    {
        Enrolled            = !Enrolled;
        Verdict.OrdinalAltered = true;
        Verdict.Mark        = RedrawMark::Content;
    }

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE MAGNITUDE STOPS
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ComponentSpecification::MagnitudeStops(ControlIdentity Claimed, const PlaneExtent& Row,
                                                      const StopDeclaration& Declared, std::uint32_t& TakenOrdinal)
{
    const ControlInk&    Ink     = Appearance->Control;
    const ControlMetric& M       = Appearance->ControlMeasure;
    const float          Across  = CentredAcross(Row, M.StopStripAcross);

    const std::uint32_t Count = (Declared.StopCount < StopCeiling) ? Declared.StopCount : StopCeiling;

    // Laying out the stops: the taken one is larger and placed at the left.
    float CursorAlong = Row.LeastAlong + M.StopStripPadLeading;

    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
    {
        const bool IsTaken = (Ordinal == TakenOrdinal);
        const float Extent = IsTaken ? M.StopTakenExtent : M.StopQuietExtent;
        const float StopAcross = Across + (M.StopStripAcross - Extent) * 0.5f;

        const PlaneExtent Stop = Spanning(CursorAlong, StopAcross, Extent, Extent);
        const bool StopRoused = Stop.Encloses(Arrived.PositionAlong, Arrived.PositionAcross);

        const InkOrdinate StopInk = IsTaken ? Ink.StopTaken : StopRoused ? Ink.StopRoused : Ink.StopQuiet;
        Surface->Medallion(Stop.LeastAlong + Extent * 0.5f, StopAcross + Extent * 0.5f,
                           Extent * 0.5f, StopInk);

        if (IsTaken && Declared.Stops != nullptr && Ordinal < Declared.StopCount)
        {
            const float LetterAcross = StopAcross + (Extent - M.RowText) * 0.5f;
            Surface->TextRun(Stop.LeastAlong + (Extent - M.MeasureRun(Declared.Stops[Ordinal], M.RowText)) * 0.5f,
                             LetterAcross, Ink.StopTakenInk, Declared.Stops[Ordinal], M.RowText);
        }

        CursorAlong += Extent + M.WellGapAcross;
    }

    // Interaction.
    ControlVerdict Verdict = {};
    Verdict.Mark = RedrawMark::Quiet;

    CursorAlong = Row.LeastAlong + M.StopStripPadLeading;

    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
    {
        const float Extent = (Ordinal == TakenOrdinal) ? M.StopTakenExtent : M.StopQuietExtent;
        const PlaneExtent Stop = Spanning(CursorAlong, Across + (M.StopStripAcross - Extent) * 0.5f, Extent, Extent);

        if (Arrived.ContactArrived && Stop.Encloses(Arrived.PositionAlong, Arrived.PositionAcross)
            && Ledger->Seize(Claimed, ControlPart::Body))
        {
            if (Ordinal != TakenOrdinal)
            {
                TakenOrdinal    = Ordinal;
                Verdict.OrdinalAltered = true;
                Verdict.Mark     = RedrawMark::Content;
            }
        }

        CursorAlong += Extent + M.WellGapAcross;
    }

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE TOOLTIP TRIGGER
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ComponentSpecification::TooltipTrigger(ControlIdentity Claimed, const PlaneExtent& Trigger,
                                                      const TooltipDeclaration& Declared)
{
    const ControlInk&    Ink     = Appearance->Control;
    const ControlMetric& M       = Appearance->ControlMeasure;
    const bool IsLight = (Declared.Appearance == TooltipAppearance::Light);

    const InkOrdinate GroundInk = IsLight ? Ink.TriggerLightGround : Ink.TriggerDarkGround;
    const InkOrdinate FigureInk = IsLight ? Ink.TriggerLightInk    : Ink.TriggerDarkInk;

    Surface->Ground(Trigger, GroundInk, M.TriggerRadius, CornerAll);

    // The figure, stroked inside the trigger's square.
    const float SymbolExtent = M.TriggerSymbol;
    const PlaneExtent SymbolSquare = Spanning(
        Trigger.LeastAlong + (Trigger.SpanAlong() - SymbolExtent) * 0.5f,
        Trigger.LeastAcross + (Trigger.SpanAcross() - SymbolExtent) * 0.5f,
        SymbolExtent, SymbolExtent);
    Stroke(Declared.Figure, SymbolSquare, FigureInk);

    // Interaction.
    const bool Roused = Trigger.Encloses(Arrived.PositionAlong, Arrived.PositionAcross);
    Ledger->DeclareRoused(Claimed, Roused, RouseDuration);

    ControlVerdict Verdict = {};
    Verdict.Mark = RedrawMark::Quiet;

    if (Arrived.ContactArrived && Roused)
    {
        if (Ledger->Seize(Claimed, ControlPart::Body))
        {
            Ledger->Disclose(Claimed);
            Verdict.Mark = RedrawMark::Content;
        }
    }

    if (Ledger->Disclosed(Claimed))
    {
        // The tooltip is deferred, so it draws above everything.
        if (DeferredCount < DeferredCeiling)
        {
            Deferred[DeferredCount].Claimed    = Claimed;
            Deferred[DeferredCount].Anchor     = Trigger;
            Deferred[DeferredCount].Title      = Declared.Title;
            Deferred[DeferredCount].Body       = Declared.Body;
            Deferred[DeferredCount].Appearance = Declared.Appearance;
            Deferred[DeferredCount].Menu       = false;
            ++DeferredCount;
        }

        Verdict.Mark = RedrawMark::Content;
    }

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE DEFERRED SWEEP
//------------------------------------------------------------------------------------------------------------------------

void ComponentSpecification::RecordMenu(const DeferredRecording& Deferred)
{
    const ControlInk&    Ink  = Appearance->Control;
    const ControlMetric& M    = Appearance->ControlMeasure;

    const std::uint32_t Count = (Deferred.OptionCount < StopCeiling) ? Deferred.OptionCount : StopCeiling;

    // Menu extent: anchored below the field, lifting by MenuLift.
    const float MenuAlong = Deferred.Anchor.SpanAlong();
    const float OptionAcross = M.OptionPadAcross * 2.0f + M.RowText;
    const float MenuAcross = OptionAcross * Count + M.MenuGapAcross * (Count - 1u) + M.MenuPad * 2.0f;

    const PlaneExtent Menu = Spanning(Deferred.Anchor.LeastAlong,
                                      Deferred.Anchor.MostAcross + M.MenuLift,
                                      MenuAlong, MenuAcross);

    Surface->Ground(Menu, Ink.MenuGround, M.MenuRadius, CornerAll);
    Surface->Edge(Menu, Ink.MenuEdge, M.CardEdgeWeight, M.MenuRadius, CornerAll);

    float CursorAcross = Menu.LeastAcross + M.MenuPad;

    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
    {
        const PlaneExtent Option = Spanning(Menu.LeastAlong + M.OptionPadAlong,
                                            CursorAcross,
                                            Menu.SpanAlong() - M.OptionPadAlong * 2.0f,
                                            OptionAcross);

        const bool OptionRoused = Option.Encloses(Arrived.PositionAlong, Arrived.PositionAcross);
        const bool OptionTaken  = (Ordinal == Deferred.TakenOption);

        if (OptionRoused || OptionTaken)
        {
            Surface->Ground(Option, OptionRoused ? Ink.OptionGroundRoused : Ink.GroupGroundTaken);
        }

        const InkOrdinate OptionInk = OptionRoused ? Ink.OptionInkRoused : Ink.OptionInk;
        Surface->TextRun(Option.LeastAlong, CentredAcross(Option, M.RowText), OptionInk,
                         Deferred.Options[Ordinal], M.RowText);

        if (OptionRoused && Arrived.ContactArrived && Ledger->Seize(Deferred.Claimed, ControlPart::Option))
        {
            // The taken option is written by the caller through the reference the menu holds.
            // Here we just close the menu.
            Ledger->Withdraw();
        }

        CursorAcross += OptionAcross + M.MenuGapAcross;
    }
}

void ComponentSpecification::RecordTooltip(const DeferredRecording& Deferred)
{
    const ControlInk&    Ink  = Appearance->Control;
    const ControlMetric& M    = Appearance->ControlMeasure;

    const bool IsLight = (Deferred.Appearance == TooltipAppearance::Light);

    const InkOrdinate GroundInk = IsLight ? Ink.TooltipLightGround : Ink.TooltipDarkGround;
    const InkOrdinate TitleInk  = IsLight ? Ink.TooltipLightTitle  : Ink.TooltipDarkTitle;
    const InkOrdinate BodyInk   = IsLight ? Ink.TooltipLightBody   : Ink.TooltipDarkBody;

    // The tooltip card, anchored above the trigger.
    const float TooltipAlong = M.TooltipAlong;
    const float TriggerCentre = Deferred.Anchor.LeastAlong + Deferred.Anchor.SpanAlong() * 0.5f;
    const float TooltipLeastAlong = TriggerCentre - TooltipAlong * 0.5f;

    // The body wraps. We estimate line count from the run extent.
    const float BodyExtent = TooltipAlong - M.TooltipPad * 2.0f;
    const float BodyRun = Surface->MeasureRun(Deferred.Body, M.TooltipBodyText);
    const std::uint32_t Lines = (BodyRun > 0.0f) ? static_cast<std::uint32_t>((BodyRun / BodyExtent) + 1.5f) : 1u;
    const std::uint32_t ClampedLines = (Lines < WrapCeiling) ? Lines : WrapCeiling;

    const float TitleAcross = M.TooltipPad;
    const float BodyAcross = TitleAcross + M.TooltipTitleText + M.TooltipTitleGap;
    const float TooltipAcross = BodyAcross + ClampedLines * M.TooltipBodyLeading + M.TooltipPad;

    const PlaneExtent Tooltip = Spanning(TooltipLeastAlong,
                                         Deferred.Anchor.LeastAcross - M.TooltipLift - TooltipAcross,
                                         TooltipAlong, TooltipAcross);

    // Ground, edge.
    Surface->Ground(Tooltip, GroundInk, M.TooltipRadius, CornerAll);
    Surface->Edge(Tooltip, Ink.CardEdge, M.CardEdgeWeight, M.TooltipRadius, CornerAll);

    // Title.
    Surface->TextRun(Tooltip.LeastAlong + M.TooltipPad, Tooltip.LeastAcross + TitleAcross,
                     TitleInk, Deferred.Title, M.TooltipTitleText);

    // Body, wrapped manually.
    {
        const char* Remaining = Deferred.Body;
        float LineAcross = BodyAcross;

        for (std::uint32_t Line = 0u; Line < ClampedLines && Remaining != nullptr && *Remaining != '\0'; ++Line)
        {
            // Find the break point: the last space before the extent.
            const char* Break = Remaining;
            float RunSoFar = 0.0f;

            while (*Break != '\0')
            {
                const char* Next = (*Break == ' ') ? Break + 1 : Break;

                if (*Next == '\0')
                {
                    Break = Next;
                    break;
                }

                float ExtentAtBreak = Surface->MeasureRun(Remaining, M.TooltipBodyText);

                if (ExtentAtBreak > BodyExtent && Break > Remaining)
                    break;

                Break = Next;

                if (*Break == ' ')
                {
                    RunSoFar = Surface->MeasureRun(Remaining, M.TooltipBodyText);

                    if (RunSoFar > BodyExtent)
                        break;
                }
            }

            std::uint32_t Len = 0u;
            const char* p = Remaining;

            while (p < Break && *p != '\0')
            {
                ++Len;
                ++p;
            }

            if (Len > 0u)
            {
                char LineBuffer[256] = {};
                std::uint32_t CopyLen = (Len < 255u) ? Len : 255u;

                for (std::uint32_t C = 0u; C < CopyLen; ++C)
                    LineBuffer[C] = Remaining[C];

                LineBuffer[CopyLen] = '\0';
                Surface->TextRun(Tooltip.LeastAlong + M.TooltipPad, Tooltip.LeastAcross + LineAcross,
                                 BodyInk, LineBuffer, M.TooltipBodyText);
            }

            LineAcross += M.TooltipBodyLeading;
            Remaining = (*Break == ' ') ? Break + 1 : Break;
        }
    }
}

void ComponentSpecification::RecordDeferred()
{
    for (std::uint32_t Ordinal = 0u; Ordinal < DeferredCount; ++Ordinal)
    {
        if (Deferred[Ordinal].Menu)
            RecordMenu(Deferred[Ordinal]);
        else
            RecordTooltip(Deferred[Ordinal]);
    }

    DeferredCount = 0u;
}

void ComponentSpecification::FoldMark(RedrawMark Arriving)
{
    if (static_cast<std::uint32_t>(Arriving) > static_cast<std::uint32_t>(Standing))
        Standing = Arriving;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     QUERIES
//------------------------------------------------------------------------------------------------------------------------

bool ComponentSpecification::ContactTaken() const
{
    return ContactHeldByPanel;
}

RedrawMark ComponentSpecification::StandingMark() const
{
    return Standing;
}

void ComponentSpecification::Reset()
{
    Ledger     = nullptr;
    Surface    = nullptr;
    Appearance = nullptr;
    DeferredCount  = 0u;
    Standing       = RedrawMark::Quiet;
    ContactHeldByPanel = false;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     PRIVATE HELPERS
//------------------------------------------------------------------------------------------------------------------------

PlaneExtent ComponentSpecification::MenuEnclosure(const PlaneExtent& Field, std::uint32_t OptionCount) const
{
    const ControlMetric& M = Appearance->ControlMeasure;
    const float OptionAcross = M.OptionPadAcross * 2.0f + M.RowText;
    const float MenuAcross = OptionAcross * OptionCount + M.MenuGapAcross * (OptionCount - 1u) + M.MenuPad * 2.0f;

    return Spanning(Field.LeastAlong, Field.MostAcross + M.MenuLift, Field.SpanAlong(), MenuAcross);
}

std::uint32_t ComponentSpecification::OptionUnder(const PlaneExtent& Field, std::uint32_t OptionCount) const
{
    const PlaneExtent Menu = MenuEnclosure(Field, OptionCount);

    if (!Menu.Encloses(Arrived.PositionAlong, Arrived.PositionAcross))
        return OptionCount;

    const ControlMetric& M = Appearance->ControlMeasure;
    const float OptionAcross = M.OptionPadAcross * 2.0f + M.RowText;
    const float RelativeAcross = Arrived.PositionAcross - Menu.LeastAcross - M.MenuPad;

    if (RelativeAcross < 0.0f)
        return OptionCount;

    const std::uint32_t Ordinal = static_cast<std::uint32_t>(RelativeAcross / (OptionAcross + M.MenuGapAcross));

    return (Ordinal < OptionCount) ? Ordinal : OptionCount;
}

}   // namespace Slate
