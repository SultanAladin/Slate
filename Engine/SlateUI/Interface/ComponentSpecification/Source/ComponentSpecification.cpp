//============================================================================================================================================
//                                                         COMPONENTSPECIFICATION.CPP
//============================================================================================================================================
// 🧩 The eight controls, arranged and recorded from the sheet's own figures, arbitrated against the ledger's one grab.

#include "SlateUI/Interface/ComponentSpecification/Api/ComponentSpecification.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     SHARED ARITHMETIC
//------------------------------------------------------------------------------------------------------------------------

namespace
{

constexpr double HoverDuration    = 200.0;   // [ms] - the sheet's transition-colors on a hover
constexpr double DiscloseDuration = 150.0;   // [ms] - the accordion and the chevron's turn
constexpr double TooltipDuration  = 300.0;   // [ms] - duration-300 on the tooltip's opacity

/// 🧩 Interpolates between two ordinates by a fraction already clamped to the unit interval.
/// cost  ✔️
constexpr float Between(float Previous, float Incoming, float Fraction)
{
    return Previous + (Incoming - Previous) * Fraction;
}

/// 🧩 Blends two colours by a fraction, component by component, in the display-referred encoding they are in.
/// note  ⚠️ Blended where they are declared — display-referred — and not in a linear space. `08` §3.1 places
///       the interface after the tone projection, and a fade that linearised would disagree with the browser
///       the sheet was measured in, which interpolates sRGB ordinates exactly as this does.
/// cost  ✔️
constexpr std::uint8_t BlendCoordinate(std::uint8_t Previous, std::uint8_t Incoming, float Fraction)
{
    return static_cast<std::uint8_t>(
        Between(static_cast<float>(Previous), static_cast<float>(Incoming), Fraction) + 0.5f);
}

constexpr ThemeToken Blend(ThemeToken Previous, ThemeToken Incoming, float Fraction)
{
    const float Held = (Fraction < 0.0f) ? 0.0f : (Fraction > 1.0f) ? 1.0f : Fraction;

    return ThemeToken{ BlendCoordinate(Previous.Red,     Incoming.Red,     Held),
                        BlendCoordinate(Previous.Green,   Incoming.Green,   Held),
                        BlendCoordinate(Previous.Blue,    Incoming.Blue,    Held),
                        BlendCoordinate(Previous.Opacity, Incoming.Opacity, Held) };
}

/// 🧩 Restates an colour at a fraction of its own coverage — what a fading tooltip records with.
/// cost  ✔️
constexpr ThemeToken Faded(ThemeToken Declared, float Fraction)
{
    const float Held = (Fraction < 0.0f) ? 0.0f : (Fraction > 1.0f) ? 1.0f : Fraction;

    ThemeToken Restated = Declared;
    Restated.Opacity     = static_cast<std::uint8_t>(static_cast<float>(Declared.Opacity) * Held + 0.5f);

    return Restated;
}

/// 🧩 Holds an coordinate inside a stated interval.
/// cost  ✔️
constexpr double Held(double Coordinate, double Minimum, double Maximum)
{
    return (Coordinate < Minimum) ? Minimum : (Coordinate > Maximum) ? Maximum : Coordinate;
}

/// 🧩 The extent one run occupies, centred across a stated extent at its own leading.
/// note  📐 The vendor places a run by its upper edge, and every extent the sheet states is centred within
///        its row. Deriving the upper edge here rather than at nineteen call sites is what keeps a run from
///        sitting a pixel high in one control and correct in the next.
/// cost  ✔️
constexpr float CentredY(const PlaneExtent& Extent, float PointSize)
{
    return Extent.MinimumY + (Extent.Height() - PointSize) * 0.5f;
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

    unsigned long long Magnitude = Negative ? static_cast<unsigned long long>(-(Reading + 1)) + 1ull
                                            : static_cast<unsigned long long>(Reading);

    if (Magnitude == 0ull)
    {
        Reversed[Digits++] = '0';
    }

    while (Magnitude > 0ull && Digits < 19u)
    {
        Reversed[Digits++] = static_cast<char>('0' + (Magnitude % 10ull));
        Magnitude /= 10ull;
    }

    std::uint32_t Written = 0u;

    if (Negative && Written + 1u < StagingExtent)
        Staging[Written++] = '-';

    while (Digits > 0u && Written + 1u < StagingExtent)
        Staging[Written++] = Reversed[--Digits];

    Staging[Written] = '\0';
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE TWO PROJECTIONS
//------------------------------------------------------------------------------------------------------------------------

double MagnitudeFraction(double Coordinate, double Minimum, double Maximum)
{
    const double Span = Maximum - Minimum;

    if (Span <= 0.0)
        return 0.0;

    return Held((Coordinate - Minimum) / Span, 0.0, 1.0);
}

double RotationDegrees(double Previous, double TravelX, double DegreesPerPixel)
{
    // 📐 The sheet: `rotationValue = startValRot - (deltaX / 10)`. Stated as a rate so that the reduction
    //    factor cannot silently change how far a drag turns the dial.
    return Previous - TravelX * DegreesPerPixel;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> ComponentSpecification::Construct(InteractionIndex&              IncomingLedger,
                                      RecordingSurface&              IncomingSurface,
                                      const ThemeProfile& IncomingAppearance)
{
    if (Ledger != nullptr)
    {
        return Outcome<bool>::Refuse(Refusal{ RefusalReason::ContentUnsupported,
                                              "ComponentSpecification is already constructed" });
    }

    Ledger     = &IncomingLedger;
    Surface    = &IncomingSurface;
    Appearance = &IncomingAppearance;

    return Outcome<bool>::Result(true);
}

void ComponentSpecification::Advance(const PointerCondition& Incoming, double Elapsed)
{
    if (Ledger == nullptr)
        return;

    Ledger->Advance(Incoming, Elapsed);
    Sample(Incoming);
}

void ComponentSpecification::Sample(const PointerCondition& Incoming)
{
    if (Ledger == nullptr)
        return;

    Sampled       = Incoming;
    DeferredCount = 0u;
    Current      = RedrawMark::Quiet;
    ContactHeldByPanel = Incoming.ContactHeld && ContactHeldByPanel;
}

void ComponentSpecification::FoldMark(RedrawMark Incoming)
{
    Current = Dearer(Current, Incoming);
}

bool ComponentSpecification::ContactTaken() const
{
    return ContactHeldByPanel;
}

RedrawMark ComponentSpecification::CurrentMark() const
{
    return Current;
}

void ComponentSpecification::Reset()
{
    Ledger             = nullptr;
    Surface            = nullptr;
    Appearance         = nullptr;
    Sampled            = {};
    DeferredCount      = 0u;
    Current           = RedrawMark::Quiet;
    ContactHeldByPanel = false;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                          THE CARD
//------------------------------------------------------------------------------------------------------------------------

CardArrangement ComponentSpecification::ArrangeCard(float X, float Y, float Width,
                                          const float* RowExtents, std::uint32_t RowCount) const
{
    CardArrangement Arranged;

    if (Appearance == nullptr)
        return Arranged;

    const ControlMetric& Measure = Appearance->ControlMeasure;

    float Interior = 0.0f;

    for (std::uint32_t Ordinal = 0u; Ordinal < RowCount; ++Ordinal)
    {
        if (RowExtents != nullptr)
            Interior += RowExtents[Ordinal];

        if (Ordinal + 1u < RowCount)
            Interior += Measure.CardRowGap;
    }

    Arranged.Enclosure = Spanning(X, Y, Width, Interior + Measure.CardPad * 2.0f);
    Arranged.Interior  = Spanning(X + Measure.CardPad, Y + Measure.CardPad,
                                  Width - Measure.CardPad * 2.0f, Interior);
    Arranged.RowGap    = Measure.CardRowGap;

    return Arranged;
}

void ComponentSpecification::RecordCard(const CardArrangement& Arranged)
{
    if (Surface == nullptr || Appearance == nullptr)
        return;

    const ControlColour&    Colour     = Appearance->Control;
    const ControlMetric& Measure = Appearance->ControlMeasure;

    Surface->Ground(Arranged.Enclosure, Colour.CardGround, Measure.CardRadius, CornerAll);
    Surface->Edge(Arranged.Enclosure, Colour.CardEdge, Measure.CardEdgeWeight, Measure.CardRadius, CornerAll);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE SELECTION FIELD
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ComponentSpecification::SelectionField(ControlIdentity Target, const PlaneExtent& Row,
                                            const SelectionDeclaration& Declared, std::uint32_t& TakenOrdinal)
{
    ControlVerdict Reported;

    if (Ledger == nullptr || Surface == nullptr || Appearance == nullptr || !Ledger->Resolves(Target))
        return Reported;

    const ControlColour&    Colour     = Appearance->Control;
    const ControlMetric& Measure = Appearance->ControlMeasure;

    // ① The row divides into a leading label and the field that fills what remains.
    //    🔴 With `CaptionInside` the label strip is not reserved at all. Callers
    //       that give this row less width than LabelX + RowGapX otherwise get a
    //       field with MinimumX past MaximumX.
    const PlaneExtent Label = Declared.CaptionInside
                            ? Spanning(Row.MinimumX, Row.MinimumY, 0.0f, Row.Height())
                            : Spanning(Row.MinimumX, Row.MinimumY, Measure.LabelX, Row.Height());
    const float       FieldX = Declared.CaptionInside
                             ? Row.MinimumX
                             : Label.MaximumX + Measure.RowGapX;
    const PlaneExtent Field = PlaneExtent{ FieldX, Row.MinimumY, Row.MaximumX,
                                           Row.MinimumY + Measure.FieldHeight };
    const PlaneExtent Cell  = PlaneExtent{ Field.MaximumX - Measure.ChevronCellX, Field.MinimumY,
                                           Field.MaximumX, Field.MaximumY };

    // ② Arbitration. 🔴 The open menu is tested **before** the field, because it is recorded above it: a
    //    contact inside the menu's extent addresses the option under it and never the field it hangs from.
    //    Testing the field first is the defect where taking the last option instead re-toggles the menu.
    const bool OverField = Field.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool OverCell  = Cell.Encloses(Sampled.PositionX, Sampled.PositionY);

    const bool StoodOpen = Ledger->Disclosed(Target);
    const PlaneExtent Menu = StoodOpen ? MenuEnclosure(Field, Declared.OptionCount) : PlaneExtent{};
    const bool OverMenu  = StoodOpen && Menu.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Ledger->DeclareHovered(Target, OverCell, HoverDuration))
        FoldMark(RedrawMark::Recolour);

    if (Sampled.ContactPressed && (OverField || OverMenu))
    {
        if (Ledger->Grab(Target, OverMenu  ? ControlPart::Option
                                 : OverCell  ? ControlPart::Chevron
                                             : ControlPart::Body))
            ContactHeldByPanel = true;
    }

    if (Ledger->Released(Target))
    {
        if (StoodOpen && OverMenu)
        {
            // ③ An option was taken. The datum is written through the caller's own reference, here, in the
            //    call that presents it — the panel never holds it between two ticks.
            const std::uint32_t Chosen = OptionUnder(Field, Declared.OptionCount);

            if (Chosen < Declared.OptionCount && Chosen != TakenOrdinal)
            {
                TakenOrdinal             = Chosen;
                Reported.ReadingAltered = true;
            }

            Ledger->Withdraw();
            FoldMark(RedrawMark::Rearrange);
        }
        else if (OverField)
        {
            // 🔴 A tap on the field toggles the menu. Disclosing through the ledger is what closes whichever
            //    other menu stood open, so no call site has to remember to.
            if (StoodOpen)
                Ledger->Withdraw();
            else
                Ledger->Disclose(Target);

            FoldMark(RedrawMark::Rearrange);
        }
    }

    // 📝 A contact that arrived outside both the field and its open menu withdraws it — the dismissal every
    //    menu the sheet declares performs, and the reason the test is on arrival rather than on release.
    if (StoodOpen && Sampled.ContactPressed && !OverField && !OverMenu)
    {
        Ledger->Withdraw();
        FoldMark(RedrawMark::Rearrange);
    }

    const bool  Disclosed = Ledger->Disclosed(Target);
    const float Hovered    = Ledger->HoveredFraction(Target);

    // ③ The label, then the field's two grounds.
    if (!Declared.CaptionInside)
    {
        Surface->TextRun(Label.MinimumX, CentredY(Label, Measure.LabelText), Colour.LabelQuiet,
                         Declared.Caption, Measure.LabelText, 0.0f, false);
    }

    const float Radius = Field.Height() * 0.5f;

    Surface->Ground(Field, Colour.FieldGround, Radius, CornerAll);
    Surface->Ground(Cell, Blend(Colour.CellGround, Colour.CellGroundHovered, Hovered), Radius,
                    CornerTrailingUpper | CornerTrailingLower);
    Surface->Edge(Field, Colour.CardEdge, Measure.CardEdgeWeight, Radius, CornerAll);

    // ④ The taken option's caption, inside the black field.
    const char* OptionCaption = (Declared.Options != nullptr && TakenOrdinal < Declared.OptionCount)
                          ? Declared.Options[TakenOrdinal]
                          : "";

    Surface->TextRunTruncated(Field.MinimumX + Measure.FieldPadX,
                              CentredY(Field, Measure.RowText),
                              Cell.MinimumX - Field.MinimumX - Measure.FieldPadX * 2.0f,
                              Colour.FieldColour, OptionCaption, Measure.RowText, false);

    // ⑤ The chevron, turned a half turn while the menu stands open. The sheet rotates it; the figure roster
    //    holds one chevron, so the turned pose is recorded as the upward figure rather than as a rotation
    //    the recording surface does not offer.
    const PlaneExtent Chevron = Spanning(Cell.MinimumX + (Cell.Width() - Measure.ChevronSymbol) * 0.5f,
                                         Cell.MinimumY + (Cell.Height() - Measure.ChevronSymbol) * 0.5f,
                                         Measure.ChevronSymbol, Measure.ChevronSymbol);

    Surface->Stroke(SymbolSubject::ChevronDown, Chevron, Colour.CellColour);

    // ⑥ The menu is deferred so that it records above every row beneath it.
    if (Disclosed && Declared.OptionCount > 0u && DeferredCount < DeferredCeiling)
    {
        DeferredRecording& Holding = Deferred[DeferredCount++];

        Holding.Target     = Target;
        Holding.Anchor      = Field;
        Holding.Options     = Declared.Options;
        Holding.OptionCount = Declared.OptionCount;
        Holding.TakenOption = TakenOrdinal;
        Holding.Menu        = true;
    }

    Reported.ContactTaken = Ledger->Holding(Target);
    Reported.Mark         = Current;

    return Reported;
}

PlaneExtent ComponentSpecification::MenuEnclosure(const PlaneExtent& Field, std::uint32_t OptionCount) const
{
    if (Appearance == nullptr || OptionCount == 0u)
        return PlaneExtent{};

    const ControlMetric& Measure = Appearance->ControlMeasure;

    const float OptionHeight = Measure.RowText * 1.5f + Measure.OptionPadY * 2.0f;
    const float Interior     = OptionHeight * static_cast<float>(OptionCount)
                             + Measure.MenuGapY * static_cast<float>(OptionCount - 1u);

    return Spanning(Field.MinimumX, Field.MaximumY + Measure.MenuLift,
                    Field.Width(), Interior + Measure.MenuPad * 2.0f);
}

std::uint32_t ComponentSpecification::OptionUnder(const PlaneExtent& Field, std::uint32_t OptionCount) const
{
    if (Appearance == nullptr || OptionCount == 0u)
        return OptionCount;

    const ControlMetric& Measure = Appearance->ControlMeasure;
    const PlaneExtent    Menu    = MenuEnclosure(Field, OptionCount);

    const float OptionHeight = Measure.RowText * 1.5f + Measure.OptionPadY * 2.0f;
    float       Cursor       = Menu.MinimumY + Measure.MenuPad;

    for (std::uint32_t Ordinal = 0u; Ordinal < OptionCount; ++Ordinal)
    {
        const PlaneExtent Option = Spanning(Menu.MinimumX + Measure.MenuPad, Cursor,
                                            Menu.Width() - Measure.MenuPad * 2.0f, OptionHeight);

        if (Option.Encloses(Sampled.PositionX, Sampled.PositionY))
            return Ordinal;

        Cursor += OptionHeight + Measure.MenuGapY;
    }

    return OptionCount;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE MAGNITUDE ROW
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ComponentSpecification::MagnitudeRow(ControlIdentity Target, const PlaneExtent& Row,
                                          const MagnitudeDeclaration& Declared, double& Coordinate,
                                          bool ReadoutTrailing)
{
    ControlVerdict Reported;

    if (Ledger == nullptr || Surface == nullptr || Appearance == nullptr || !Ledger->Resolves(Target))
        return Reported;

    const ControlColour&    Colour     = Appearance->Control;
    const ControlMetric& Measure = Appearance->ControlMeasure;

    // ① The row is a label, a readout pill, and a slider — the sheet spaces them with one gap each.
    PlaneExtent Label;
    PlaneExtent Readout;
    PlaneExtent Track;

    if (ReadoutTrailing)
    {
        Readout = Spanning(Row.MaximumX - Measure.ReadoutX, Row.MinimumY,
                           Measure.ReadoutX, Measure.FieldHeight);
        Track = Spanning(Row.MinimumX,
                         Row.MinimumY + (Row.Height() - Measure.SliderHeight) * 0.5f,
                         Readout.MinimumX - Row.MinimumX - Measure.RowGapX,
                         Measure.SliderHeight);
    }
    else
    {
        Label = Spanning(Row.MinimumX, Row.MinimumY, Measure.LabelX, Row.Height());
        Readout = Spanning(Label.MaximumX + Measure.RowGapX, Row.MinimumY,
                           Measure.ReadoutX, Measure.FieldHeight);
        Track = Spanning(Row.MaximumX - Measure.SliderX,
                         Row.MinimumY + (Row.Height() - Measure.SliderHeight) * 0.5f,
                         Measure.SliderX, Measure.SliderHeight);
    }

    const PlaneExtent UnitCell = PlaneExtent{ Readout.MaximumX - Measure.UnitCellX, Readout.MinimumY,
                                              Readout.MaximumX, Readout.MaximumY };

    // ② Arbitration. The thumb's centre may travel only between the two ends of the track's inner run, so
    //    the fraction is projected onto that inner run and never onto the track's whole extent.
    const float Radius     = Measure.ThumbExtent * 0.5f;
    const float TravelLeft = Track.MinimumX + Radius;
    const float TravelRight  = Track.MaximumX  - Radius;
    const float TravelWidth  = (TravelRight > TravelLeft) ? (TravelRight - TravelLeft) : 0.0f;

    const bool OverTrack = Track.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Ledger->DeclareHovered(Target, OverTrack, HoverDuration))
        FoldMark(RedrawMark::Recolour);

    if (Sampled.ContactPressed && OverTrack)
    {
        if (Ledger->Grab(Target, ControlPart::Thumb))
        {
            ContactHeldByPanel = true;
            Ledger->RecordInitial(Target, static_cast<float>(Coordinate));
        }
    }

    // 📐 While held, the reading is projected from the pointer's absolute abscissa against the track — not
    //    accumulated from per-tick travel, which drifts by a pixel for every tick the pointer spent outside
    //    the extent and never returns to a round figure at either end.
    if (Ledger->Holding(Target) && TravelWidth > 0.0f)
    {
        const double Fraction = Held((static_cast<double>(Sampled.PositionX) -
                                      static_cast<double>(TravelLeft)) / static_cast<double>(TravelWidth),
                                     0.0, 1.0);
        const double Projected = Declared.Minimum +
                                 Fraction * (Declared.Maximum - Declared.Minimum);

        if (Projected != Coordinate)
        {
            Coordinate                 = Projected;
            Reported.ReadingAltered = true;
            FoldMark(RedrawMark::Rerecord);
        }
    }

    const double Fraction = MagnitudeFraction(Coordinate, Declared.Minimum, Declared.Maximum);

    // ③ The label, absent when the readout trails a full-width slider.
    if (!ReadoutTrailing)
        Surface->TextRun(Label.MinimumX, CentredY(Label, Measure.LabelText), Colour.LabelQuiet,
                         Declared.Caption, Measure.LabelText, 0.0f, false);

    // ④ The readout pill — a black value cell and a raised unit cell, rounded at the ends only.
    const float PillRadius = Readout.Height() * 0.5f;

    Surface->Ground(Readout, Colour.FieldGround, PillRadius, CornerAll);
    Surface->Ground(UnitCell, Colour.CellGround, PillRadius, CornerTrailingUpper | CornerTrailingLower);
    Surface->Edge(Readout, Colour.CardEdge, Measure.CardEdgeWeight, PillRadius, CornerAll);

    char Reading[24] = {};
    IntegralRun(Reading, 24u, static_cast<long long>(Coordinate + (Coordinate < 0.0 ? -0.5 : 0.5)));

    const float ValueX = Readout.MinimumX;
    const float ValueSpan  = UnitCell.MinimumX - Readout.MinimumX;
    const float ReadingRun = Surface->MeasureRun(Reading, Measure.ReadoutText, Measure.ReadoutTracking);

    Surface->TextRun(ValueX + (ValueSpan - ReadingRun) * 0.5f,
                     CentredY(Readout, Measure.ReadoutText),
                     Colour.FieldColour, Reading, Measure.ReadoutText, Measure.ReadoutTracking, true);

    const float UnitRun = Surface->MeasureRun(Declared.UnitGlyph, Measure.UnitText, 0.0f);

    Surface->TextRun(UnitCell.MinimumX + (UnitCell.Width() - UnitRun) * 0.5f,
                     CentredY(UnitCell, Measure.UnitText),
                     Colour.UnitColour, Declared.UnitGlyph, Measure.UnitText, 0.0f, false);

    // ⑤ The track — taken below the fraction, quiet above it, and the thumb centred on the division.
    const float TrackRadius = Track.Height() * 0.5f;
    const float Division    = TravelLeft + TravelWidth * static_cast<float>(Fraction);

    Surface->Ground(Track, Colour.TrackQuiet, TrackRadius, CornerAll);

    if (Division > Track.MinimumX)
    {
        const PlaneExtent Taken = PlaneExtent{ Track.MinimumX, Track.MinimumY,
                                               Division, Track.MaximumY };
        Surface->Ground(Taken, Colour.TrackTaken, TrackRadius, CornerLeadingUpper | CornerLeadingLower);
    }

    Surface->Edge(Track, Colour.TrackEdge, Measure.CardEdgeWeight, TrackRadius, CornerAll);
    Surface->Medallion(Division, Track.MinimumY + Track.Height() * 0.5f, Radius, Colour.ThumbGround);

    Reported.ContactTaken = Ledger->Holding(Target);
    Reported.Mark         = Current;

    return Reported;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE ROTATION RULER
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ComponentSpecification::VectorRow(ControlIdentity Target, const PlaneExtent& Row,
                                                 const VectorDeclaration& Declared, double* Coordinates)
{
    ControlVerdict Reported;

    if (Ledger == nullptr || Surface == nullptr || Appearance == nullptr ||
        Coordinates == nullptr || !Ledger->Resolves(Target))
        return Reported;

    const ControlColour& Colour  = Appearance->Control;
    const ControlMetric& Measure = Appearance->ControlMeasure;

    // ① The row divides into a leading label and one readout that fills the rest.
    const PlaneExtent Label = Spanning(Row.MinimumX, Row.MinimumY, Measure.LabelX, Row.Height());
    const PlaneExtent Readout{ Label.MaximumX + Measure.RowGapX, Row.MinimumY,
                               Row.MaximumX, Row.MinimumY + Measure.FieldHeight };

    if (Readout.Width() <= 0.0f)
        return Reported;

    // 📐 One unit cell at the trailing end, exactly as the scalar pill has, and the
    //    remainder split three ways. The axes therefore share the unit rather than
    //    repeating it, which is what makes this one control and not three.
    const PlaneExtent UnitCell{ Readout.MaximumX - Measure.UnitCellX, Readout.MinimumY,
                                Readout.MaximumX, Readout.MaximumY };
    const float AxisSpan = (UnitCell.MinimumX - Readout.MinimumX) / 3.0f;

    // ② Arbitration. Each axis is its own drag zone; the identity is shared, so the
    //    held axis is remembered as the part rather than as a separate control.
    const float PointerX = Sampled.PositionX;
    const float PointerY = Sampled.PositionY;

    std::int32_t OverAxis = -1;
    for (std::int32_t Axis = 0; Axis < 3; ++Axis)
    {
        const PlaneExtent Cell{ Readout.MinimumX + AxisSpan * static_cast<float>(Axis),
                                Readout.MinimumY,
                                Readout.MinimumX + AxisSpan * static_cast<float>(Axis + 1),
                                Readout.MaximumY };
        if (Cell.Encloses(PointerX, PointerY))
            OverAxis = Axis;
    }

    if (Ledger->DeclareHovered(Target, OverAxis >= 0, HoverDuration))
        FoldMark(RedrawMark::Recolour);

    if (Sampled.ContactPressed && OverAxis >= 0)
    {
        if (Ledger->Grab(Target, ControlPart::Thumb))
        {
            ContactHeldByPanel = true;
            Ledger->RecordInitial(Target, static_cast<float>(OverAxis));
        }
    }

    // 📐 Dragged horizontally, a cell scrubs its own axis.
    // 📝 MagnitudeRow projects from the pointer's ABSOLUTE abscissa because its
    //    track has two ends to project between, and it notes that accumulating
    //    per-tick travel drifts. A transform axis has no track and no ends — a
    //    position is unbounded — so there is nothing to project onto and the
    //    accumulation is the reading rather than an approximation of it. The
    //    drift that comment warns about cannot arise here for the same reason:
    //    there is no round figure at either end to fail to return to.
    if (Ledger->Holding(Target) && AxisSpan > 0.0f)
    {
        const Outcome<float> Recorded = Ledger->InitialReading(Target);
        const std::int32_t   Held = Recorded.Resolved
                                  ? static_cast<std::int32_t>(Recorded.Resolve() + 0.5f) : -1;
        if (Held >= 0 && Held < 3)
        {
            const double Span      = Declared.Maximum - Declared.Minimum;
            const double Projected = Coordinates[Held] +
                                     static_cast<double>(Sampled.TravelX) * Span /
                                     static_cast<double>(AxisSpan) * 0.001;

            const double Bounded = (Projected < Declared.Minimum) ? Declared.Minimum
                                 : (Projected > Declared.Maximum) ? Declared.Maximum : Projected;

            if (Bounded != Coordinates[Held])
            {
                Coordinates[Held]      = Bounded;
                Reported.ReadingAltered = true;
                FoldMark(RedrawMark::Rerecord);
            }
        }
    }

    // ③ The label.
    Surface->TextRun(Label.MinimumX, CentredY(Label, Measure.LabelText), Colour.LabelQuiet,
                     Declared.Caption, Measure.LabelText, 0.0f, false);

    // ④ The readout: one pill, three value cells, one raised unit cell.
    const float PillRadius = Readout.Height() * 0.5f;

    Surface->Ground(Readout, Colour.FieldGround, PillRadius, CornerAll);
    Surface->Ground(UnitCell, Colour.CellGround, PillRadius, CornerTrailingUpper | CornerTrailingLower);
    Surface->Edge(Readout, Colour.CardEdge, Measure.CardEdgeWeight, PillRadius, CornerAll);

    for (std::int32_t Axis = 0; Axis < 3; ++Axis)
    {
        const float CellLead = Readout.MinimumX + AxisSpan * static_cast<float>(Axis);

        // 📐 A hairline between the axes, so three readings do not read as one run.
        if (Axis > 0)
            Surface->Ground(Spanning(CellLead, Readout.MinimumY + 4.0f, 1.0f, Readout.Height() - 8.0f),
                            Colour.CardEdge, 0.0f, CornerNone);

        const char* AxisRun = (Declared.AxisRuns[Axis] != nullptr) ? Declared.AxisRuns[Axis] : "";
        const float AxisWide = Surface->MeasureRun(AxisRun, Measure.UnitText, 0.0f);

        Surface->TextRun(CellLead + 8.0f, CentredY(Readout, Measure.UnitText),
                         Colour.UnitColour, AxisRun, Measure.UnitText, 0.0f, false);

        char Reading[24] = {};
        IntegralRun(Reading, 24u,
                    static_cast<long long>(Coordinates[Axis] +
                                           (Coordinates[Axis] < 0.0 ? -0.5 : 0.5)));

        const float ReadingRun = Surface->MeasureRun(Reading, Measure.ReadoutText,
                                                     Measure.ReadoutTracking);
        const float ReadingLead = CellLead + 8.0f + AxisWide;
        const float ReadingRoom = AxisSpan - 8.0f - AxisWide - 8.0f;

        Surface->TextRun(ReadingLead + (ReadingRoom - ReadingRun) * 0.5f,
                         CentredY(Readout, Measure.ReadoutText),
                         Colour.FieldColour, Reading, Measure.ReadoutText,
                         Measure.ReadoutTracking, true);
    }

    const float UnitRun = Surface->MeasureRun(Declared.UnitGlyph, Measure.UnitText, 0.0f);

    Surface->TextRun(UnitCell.MinimumX + (UnitCell.Width() - UnitRun) * 0.5f,
                     CentredY(UnitCell, Measure.UnitText),
                     Colour.UnitColour, Declared.UnitGlyph, Measure.UnitText, 0.0f, false);

    return Reported;
}

ControlVerdict ComponentSpecification::RotationRuler(ControlIdentity Target, const PlaneExtent& Row,
                                           const RulerDeclaration& Declared, double& Degrees)
{
    ControlVerdict Reported;

    if (Ledger == nullptr || Surface == nullptr || Appearance == nullptr || !Ledger->Resolves(Target))
        return Reported;

    const ControlColour&    Colour     = Appearance->Control;
    const ControlMetric& Measure = Appearance->ControlMeasure;

    // ① The ruler stacks: a label and readout above, the tick strip beneath them.
    const PlaneExtent Head    = Spanning(Row.MinimumX, Row.MinimumY, Row.Width(), Measure.FieldHeight);
    const PlaneExtent Label   = Spanning(Head.MinimumX, Head.MinimumY, Measure.LabelX, Head.Height());
    const PlaneExtent Readout = Spanning(Head.MaximumX - Measure.ReadoutX, Head.MinimumY,
                                         Measure.ReadoutX, Measure.FieldHeight);
    const PlaneExtent UnitCell = PlaneExtent{ Readout.MaximumX - Measure.UnitCellX, Readout.MinimumY,
                                              Readout.MaximumX, Readout.MaximumY };
    const PlaneExtent Strip   = Spanning(Row.MinimumX, Head.MaximumY + Measure.CardRowGap * 0.5f,
                                         Row.Width(), Measure.RulerHeight);

    // ② Arbitration. The reading departs from where it stood when the contact arrived, so a drag that
    //    leaves and re-enters the strip resumes from the same origin rather than jumping.
    const bool OverStrip = Strip.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Sampled.ContactPressed && OverStrip)
    {
        if (Ledger->Grab(Target, ControlPart::Strip))
        {
            ContactHeldByPanel = true;
            Ledger->RecordInitial(Target, static_cast<float>(Degrees));
        }
    }

    if (Ledger->Holding(Target) && Ledger->HeldPart(Target) == ControlPart::Strip)
    {
        const Outcome<float> Previous = Ledger->InitialReading(Target);

        if (Previous.Resolved)
        {
            const double Travel   = static_cast<double>(Sampled.PositionX) -
                                    static_cast<double>(Ledger->OriginX());
            const double Turned   = RotationDegrees(static_cast<double>(Previous.Resolve()), Travel,
                                                    static_cast<double>(Measure.RulerDegreesPerPixel));

            if (Turned != Degrees)
            {
                Degrees                  = Turned;
                Reported.ReadingAltered = true;
                FoldMark(RedrawMark::Rerecord);
            }
        }
    }

    // ③ The label and the readout pill.
    Surface->TextRun(Label.MinimumX, CentredY(Label, Measure.LabelText), Colour.LabelQuiet,
                     Declared.Caption, Measure.LabelText, 0.0f, false);

    const float PillRadius = Readout.Height() * 0.5f;

    Surface->Ground(Readout, Colour.FieldGround, PillRadius, CornerAll);
    Surface->Ground(UnitCell, Colour.CellGround, PillRadius, CornerTrailingUpper | CornerTrailingLower);
    Surface->Edge(Readout, Colour.CardEdge, Measure.CardEdgeWeight, PillRadius, CornerAll);

    const long long Rounded = static_cast<long long>(Degrees + (Degrees < 0.0 ? -0.5 : 0.5));

    char Reading[24] = {};
    IntegralRun(Reading, 24u, Rounded);

    const float ValueSpan  = UnitCell.MinimumX - Readout.MinimumX;
    const float ReadingRun = Surface->MeasureRun(Reading, Measure.ReadoutText, Measure.ReadoutTracking);

    Surface->TextRun(Readout.MinimumX + (ValueSpan - ReadingRun) * 0.5f,
                     CentredY(Readout, Measure.ReadoutText),
                     Colour.FieldColour, Reading, Measure.ReadoutText, Measure.ReadoutTracking, true);

    const float UnitRun = Surface->MeasureRun(Declared.UnitGlyph, Measure.UnitText, 0.0f);

    Surface->TextRun(UnitCell.MinimumX + (UnitCell.Width() - UnitRun) * 0.5f,
                     CentredY(UnitCell, Measure.UnitText),
                     Colour.UnitColour, Declared.UnitGlyph, Measure.UnitText, 0.0f, false);

    // ④ The strip's ground, and every tick inside it, confined so nothing escapes the rounded extent.
    Surface->Ground(Strip, Colour.RulerGround, Measure.RulerRadius, CornerAll);
    Surface->Edge(Strip, Colour.CardEdge, Measure.CardEdgeWeight, Measure.RulerRadius, CornerAll);
    Surface->Confine(Strip);

    const float CentreX  = Strip.MinimumX + Strip.Width() * 0.5f;
    const float CentreY = Strip.MinimumY + Strip.Height() * 0.5f;
    const long long Centred  = Rounded;

    for (long long Tick = Centred - static_cast<long long>(Measure.TickReach);
         Tick <= Centred + static_cast<long long>(Measure.TickReach); ++Tick)
    {
        // 📐 The sheet places tick i at `i * TICK_SPACING` and then translates the whole strip by
        //    `-rotationValue * TICK_SPACING`. The two compose to this, which is what keeps the strip
        //    continuous while the reading is fractional.
        const float TickX = CentreX +
                                (static_cast<float>(Tick) - static_cast<float>(Degrees)) * Measure.TickSpacing;

        if (TickX < Strip.MinimumX - Measure.TickWeight ||
            TickX > Strip.MaximumX  + Measure.TickWeight)
            continue;

        const long long Absolute = (Tick < 0) ? -Tick : Tick;
        const bool      Major    = (Absolute % 10) == 0;
        const bool      Medium   = !Major && (Absolute % 5) == 0;

        const float       TickY = Major ? Measure.TickMajorHeight
                                     : Medium ? Measure.TickMediumHeight
                                              : Measure.TickMinorHeight;
        const ThemeToken TickColour    = Major ? Colour.TickMajor : Medium ? Colour.TickMedium : Colour.TickMinor;

        const PlaneExtent Mark = Spanning(TickX - Measure.TickWeight * 0.5f,
                                          CentreY - TickY * 0.5f,
                                          Measure.TickWeight, TickY);

        Surface->Ground(Mark, TickColour, Measure.TickWeight * 0.5f, CornerAll);

        if (!Major)
            continue;

        char Caption[24] = {};
        IntegralRun(Caption, 22u, Tick);

        // 📝 The degree sign is appended as its own two UTF-8 octets rather than spelled in the literal, so
        //    the run stays ASCII-safe at every call site that measures it.
        std::uint32_t Written = 0u;
        while (Caption[Written] != '\0' && Written < 20u) ++Written;
        Caption[Written++] = '\xC2';
        Caption[Written++] = '\xB0';
        Caption[Written]   = '\0';

        const float CaptionRun = Surface->MeasureRun(Caption, Measure.TickCaptionText, 0.0f);

        Surface->TextRun(TickX - CaptionRun * 0.5f, CentreY + Measure.TickCaptionLift,
                         Colour.TickCaption, Caption, Measure.TickCaptionText, 0.0f, true);
    }

    // ⑤ The sheet's mask — transparent, opaque at a fifth, opaque to four fifths, transparent. Two X
    //    scrims over the ruler's own ground reproduce it; the middle three fifths need nothing recorded.
    const ThemeToken Opaque      = Colour.CardGround;
    const ThemeToken Transparent = Faded(Colour.CardGround, 0.0f);
    const float       FadeX   = Strip.Width() * 0.2f;

    Surface->Scrim(PlaneExtent{ Strip.MinimumX, Strip.MinimumY,
                                Strip.MinimumX + FadeX, Strip.MaximumY },
                   Opaque, Transparent, ScrimAxis::X);

    Surface->Scrim(PlaneExtent{ Strip.MaximumX - FadeX, Strip.MinimumY,
                                Strip.MaximumX, Strip.MaximumY },
                   Transparent, Opaque, ScrimAxis::X);

    // ⑥ The fixed centre pointer, recorded last so the fade never touches it.
    const PlaneExtent Pointer = Spanning(CentreX - Measure.PointerWeight * 0.5f,
                                         CentreY - Measure.PointerY * 0.5f,
                                         Measure.PointerWeight, Measure.PointerY);

    Surface->Ground(Pointer, Colour.RulerPointer, Measure.PointerWeight * 0.5f, CornerAll);
    Surface->Medallion(CentreX, Strip.MinimumY + Measure.PointerDotLift,
                       Measure.PointerDot * 0.5f, Colour.RulerPointer);

    Surface->Release();

    Reported.ContactTaken = Ledger->Holding(Target);
    Reported.Mark         = Current;

    return Reported;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE TOGGLE ROW
//------------------------------------------------------------------------------------------------------------------------

// 📐 The pill's proportions, stated once and matching ControlPanel::SwitchTrack:
//    the nub is 11/16 of the track's radius and grows 1/16 more while roused,
//    which is exactly the reference 32 px track's 11 -> 12 px nub.
void ComponentSpecification::SwitchTrack(ControlIdentity Target, const PlaneExtent& Extent, bool Taken,
                                         ThemeToken TrackTaken, ThemeToken TrackQuiet, ThemeToken Nub)
{
    if (Ledger == nullptr || Surface == nullptr)
        return;

    Ledger->DeclareTaken(Target, Taken, HoverDuration);

    const float TakenFraction = Ledger->TakenFraction(Target);
    const float HoverFraction = Ledger->HoveredFraction(Target);

    const float Radius    = Extent.Height() * 0.5f;
    const float NubQuiet  = Radius * (11.0f / 16.0f);
    const float NubRoused = Radius * (12.0f / 16.0f);

    Surface->Ground(Extent, Blend(TrackQuiet, TrackTaken, TakenFraction), Radius, CornerAll);

    const float NubX      = Between(Extent.MinimumX + Radius, Extent.MaximumX - Radius, TakenFraction);
    const float NubRadius = Between(NubQuiet, NubRoused, HoverFraction);

    Surface->Medallion(NubX, Extent.MinimumY + Radius, NubRadius, Nub);
}

ControlVerdict ComponentSpecification::ToggleRow(ControlIdentity Target, const PlaneExtent& Row,
                                       const ToggleDeclaration& Declared, bool& Taken)
{
    ControlVerdict Reported;

    if (Ledger == nullptr || Surface == nullptr || Appearance == nullptr || !Ledger->Resolves(Target))
        return Reported;

    const ControlColour&    Colour     = Appearance->Control;
    const ControlMetric& Measure = Appearance->ControlMeasure;

    const bool OverRow = Row.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Ledger->DeclareHovered(Target, OverRow, HoverDuration))
        FoldMark(RedrawMark::Recolour);

    if (Sampled.ContactPressed && OverRow)
    {
        if (Ledger->Grab(Target, ControlPart::Body))
            ContactHeldByPanel = true;
    }

    if (Ledger->Released(Target) && OverRow)
    {
        Taken                    = !Taken;
        Reported.ReadingAltered = true;
        FoldMark(RedrawMark::Rerecord);
    }

    if (Ledger->DeclareTaken(Target, Taken, HoverDuration))
        FoldMark(RedrawMark::Recolour);

    const float Hovered = Ledger->HoveredFraction(Target);
    const float Held   = Ledger->TakenFraction(Target);

    // ① The ring — quiet, hovered, or taken. The sheet fades the border colour and scales the dot.
    const PlaneExtent Ring = Spanning(Row.MinimumX + Measure.ToggleRowPadX,
                                      Row.MinimumY + (Row.Height() - Measure.RingExtent) * 0.5f,
                                      Measure.RingExtent, Measure.RingExtent);

    const ThemeToken Quiet   = Blend(Colour.RingQuiet, Colour.RingHovered, Hovered);
    const ThemeToken RingColour = Blend(Quiet, Colour.RingTaken, Held);

    Surface->Edge(Ring, RingColour, Measure.RingWeight, Measure.RingExtent * 0.5f, CornerAll);

    // ② The dot scales from nothing to its full extent, which is what `scale-0` to `scale-100` states.
    const float DotRadius = Measure.RingDotExtent * 0.5f * Held;

    if (DotRadius > 0.0f)
    {
        Surface->Medallion(Ring.MinimumX + Ring.Width() * 0.5f,
                           Ring.MinimumY + Ring.Height() * 0.5f,
                           DotRadius, Colour.RingDot);
    }

    // ③ The label, fading between its three declared colours.
    const ThemeToken QuietLabel = Blend(Colour.LabelQuiet, Colour.LabelHovered, Hovered);
    const ThemeToken LabelColour   = Blend(QuietLabel, Colour.LabelTaken, Held);

    Surface->TextRun(Ring.MaximumX + Measure.ToggleGapX, CentredY(Row, Measure.RowText),
                     LabelColour, Declared.Caption, Measure.RowText, 0.0f, false);

    Reported.ContactTaken = Ledger->Holding(Target);
    Reported.Mark         = Current;

    return Reported;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE MULTI-SELECT ROW
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ComponentSpecification::SubsetRow(ControlIdentity Target, const PlaneExtent& Row,
                                       const SubsetDeclaration& Declared, bool& Registered)
{
    ControlVerdict Reported;

    if (Ledger == nullptr || Surface == nullptr || Appearance == nullptr || !Ledger->Resolves(Target))
        return Reported;

    const ControlColour&    Colour     = Appearance->Control;
    const ControlMetric& Measure = Appearance->ControlMeasure;

    const bool OverRow = Row.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Ledger->DeclareHovered(Target, OverRow, HoverDuration))
        FoldMark(RedrawMark::Recolour);

    if (Sampled.ContactPressed && OverRow)
    {
        if (Ledger->Grab(Target, ControlPart::Body))
            ContactHeldByPanel = true;
    }

    if (Ledger->Released(Target) && OverRow)
    {
        Registered                 = !Registered;
        Reported.ReadingAltered = true;
        FoldMark(RedrawMark::Rerecord);
    }

    if (Ledger->DeclareTaken(Target, Registered, HoverDuration))
        FoldMark(RedrawMark::Recolour);

    const float Hovered = Ledger->HoveredFraction(Target);
    const float Held   = Ledger->TakenFraction(Target);

    // ① The ground. The sheet gives a quiet row no ground at all, a hovered one #222222, and a taken one
    //    #2a2a2a — and the taken ground wins over the hovered one, which is why it is blended last.
    const ThemeToken QuietGround = Blend(Colour.RowGroundQuiet, Colour.RowGroundHovered, Hovered);
    const ThemeToken RowGround   = Blend(QuietGround, Colour.RowGroundTaken, Held);

    // 🔴 Square corners. The sheet states `rounded-none` on this row and rounds every other control; a
    //    radius here would be the one place the transcription quietly improved on the source.
    Surface->Ground(Row, RowGround, 0.0f, CornerNone);

    // ② The rail, which the sheet grows from nothing along the row's whole extent across.
    const PlaneExtent Rail = Spanning(Row.MinimumX, Row.MinimumY, Measure.SubsetRailX, Row.Height());

    Surface->Ground(Rail, Blend(Colour.RowRailQuiet, Colour.RowRailTaken, Held), 0.0f, CornerNone);

    // ③ The label.
    const ThemeToken LabelColour = Blend(Colour.LabelQuiet, Colour.LabelTaken, Held);

    Surface->TextRun(Row.MinimumX + Measure.SubsetRowPadX, CentredY(Row, Measure.RowText),
                     LabelColour, Declared.Caption, Measure.RowText, 0.0f, false);

    Reported.ContactTaken = Ledger->Holding(Target);
    Reported.Mark         = Current;

    return Reported;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE MAGNITUDE STOPS
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ComponentSpecification::MagnitudeStops(ControlIdentity Target, const PlaneExtent& Row,
                                            const StopDeclaration& Declared, std::uint32_t& TakenOrdinal)
{
    ControlVerdict Reported;

    if (Ledger == nullptr || Surface == nullptr || Appearance == nullptr || !Ledger->Resolves(Target))
        return Reported;

    if (Declared.StopCount < 2u || Declared.StopCount > StopCeiling || Declared.Stops == nullptr)
        return Reported;

    const ControlColour&    Colour     = Appearance->Control;
    const ControlMetric& Measure = Appearance->ControlMeasure;

    const PlaneExtent Label = Spanning(Row.MinimumX, Row.MinimumY, Measure.LabelX, Row.Height());
    const float       StripLeft = Label.MaximumX + Measure.RowGapX + Measure.StopStripPadLeading;
    const float       StripRight  = Row.MaximumX - Measure.StopStripPadTrailing;
    const float       StripWidth  = (StripRight > StripLeft) ? (StripRight - StripLeft) : 0.0f;
    const float       CentreY = Row.MinimumY + Row.Height() * 0.5f;

    // 📐 The sheet spaces its stops with `justify-between`, which places the first and the last against the
    //    two ends and divides the remainder evenly. With one stop there is no remainder to divide, which is
    //    why the declaration refuses a count below two rather than dividing by zero here.
    const float Division = StripWidth / static_cast<float>(Declared.StopCount - 1u);

    Surface->TextRun(Label.MinimumX, CentredY(Label, Measure.LabelText), Colour.LabelQuiet,
                     Declared.Caption, Measure.LabelText, 0.0f, false);

    std::uint32_t HoveredOrdinal = Declared.StopCount;

    for (std::uint32_t Ordinal = 0u; Ordinal < Declared.StopCount; ++Ordinal)
    {
        const float  StopX = StripLeft + Division * static_cast<float>(Ordinal);
        const bool   TakenStop = Ordinal == TakenOrdinal;
        const float  Extent    = TakenStop ? Measure.StopTakenExtent : Measure.StopQuietExtent;
        const float  Reach     = Extent * 0.5f;

        const PlaneExtent Stop = Spanning(StopX - Reach, CentreY - Reach, Extent, Extent);

        if (Stop.Encloses(Sampled.PositionX, Sampled.PositionY))
            HoveredOrdinal = Ordinal;
    }

    // ① One hover fade serves the whole strip, because the sheet roues exactly one stop at a time.
    if (Ledger->DeclareHovered(Target, HoveredOrdinal < Declared.StopCount, HoverDuration))
        FoldMark(RedrawMark::Recolour);

    if (Sampled.ContactPressed && HoveredOrdinal < Declared.StopCount)
    {
        if (Ledger->Grab(Target, ControlPart::Body))
            ContactHeldByPanel = true;
    }

    if (Ledger->Released(Target) && HoveredOrdinal < Declared.StopCount && HoveredOrdinal != TakenOrdinal)
    {
        TakenOrdinal             = HoveredOrdinal;
        Reported.ReadingAltered = true;
        FoldMark(RedrawMark::Rearrange);
    }

    const float Hovered = Ledger->HoveredFraction(Target);

    // ② Every stop, the taken one grown and carrying its letter.
    for (std::uint32_t Ordinal = 0u; Ordinal < Declared.StopCount; ++Ordinal)
    {
        const float StopX = StripLeft + Division * static_cast<float>(Ordinal);
        const bool  TakenStop = Ordinal == TakenOrdinal;
        const float Extent    = TakenStop ? Measure.StopTakenExtent : Measure.StopQuietExtent;

        const ThemeToken Quiet = (Ordinal == HoveredOrdinal) ? Blend(Colour.StopQuiet, Colour.StopHovered, Hovered)
                                                             : Colour.StopQuiet;
        const ThemeToken StopColour = TakenStop ? Colour.StopTaken : Quiet;

        Surface->Medallion(StopX, CentreY, Extent * 0.5f, StopColour);

        if (!TakenStop)
            continue;

        const char* Letter = Declared.Stops[Ordinal];

        if (Letter == nullptr || Letter[0] == '\0')
            continue;

        const float LetterRun = Surface->MeasureRun(Letter, Measure.RowText, 0.0f);

        Surface->TextRun(StopX - LetterRun * 0.5f, CentreY - Measure.RowText * 0.5f,
                         Colour.StopTakenColour, Letter, Measure.RowText, 0.0f, true);
    }

    Reported.ContactTaken = Ledger->Holding(Target);
    Reported.Mark         = Current;

    return Reported;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE TOOLTIP TRIGGER
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ComponentSpecification::TooltipTrigger(ControlIdentity Target, const PlaneExtent& Trigger,
                                            const TooltipDeclaration& Declared)
{
    ControlVerdict Reported;

    if (Ledger == nullptr || Surface == nullptr || Appearance == nullptr || !Ledger->Resolves(Target))
        return Reported;

    const ControlColour&    Colour     = Appearance->Control;
    const ControlMetric& Measure = Appearance->ControlMeasure;

    const bool Light   = Declared.Appearance == TooltipAppearance::Light;
    const bool OverIt  = Trigger.Encloses(Sampled.PositionX, Sampled.PositionY);

    // 📝 The card is disclosed by rest rather than by tap — `group-hover` — so this is the one control that
    //    declares its take fade from the pointer's presence and never from a release.
    if (Ledger->DeclareHovered(Target, OverIt, HoverDuration))
        FoldMark(RedrawMark::Recolour);

    if (Ledger->DeclareTaken(Target, OverIt, TooltipDuration))
        FoldMark(RedrawMark::Recolour);

    if (Sampled.ContactPressed && OverIt)
    {
        if (Ledger->Grab(Target, ControlPart::Body))
            ContactHeldByPanel = true;
    }

    const float Hovered    = Ledger->HoveredFraction(Target);
    const float Disclosed = Ledger->TakenFraction(Target);
    const bool  Grabbed    = Ledger->Holding(Target);

    // ① The sheet scales the trigger to 1.05 on hover and 0.95 while it is held. Both are recorded as an
    //    inset of the declared extent, because the recording surface places extents and does not transform.
    const float Scaled  = Grabbed ? 0.95f : Between(1.0f, 1.05f, Hovered);
    const float Inset   = Trigger.Width() * (1.0f - Scaled) * 0.5f;

    const PlaneExtent Grown = PlaneExtent{ Trigger.MinimumX  + Inset, Trigger.MinimumY + Inset,
                                           Trigger.MaximumX   - Inset, Trigger.MaximumY  - Inset };

    Surface->Ground(Grown, Light ? Colour.TriggerLightGround : Colour.TriggerDarkGround,
                    Measure.TriggerRadius, CornerAll);

    const PlaneExtent Figure = Spanning(Grown.MinimumX + (Grown.Width() - Measure.TriggerSymbol) * 0.5f,
                                        Grown.MinimumY + (Grown.Height() - Measure.TriggerSymbol) * 0.5f,
                                        Measure.TriggerSymbol, Measure.TriggerSymbol);

    Surface->Stroke(Declared.Figure, Figure, Light ? Colour.TriggerLightColour : Colour.TriggerDarkColour);

    // ② The card is deferred while any of it is visible, so it records above every later control.
    if (Disclosed > 0.0f && DeferredCount < DeferredCeiling)
    {
        DeferredRecording& Holding = Deferred[DeferredCount++];

        Holding.Target    = Target;
        Holding.Anchor     = Trigger;
        Holding.Title      = Declared.Title;
        Holding.Body       = Declared.Body;
        Holding.Appearance = Declared.Appearance;
        Holding.Menu       = false;
    }

    Reported.ContactTaken = Grabbed;
    Reported.Mark         = Current;

    return Reported;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE DEFERRED SWEEP
//------------------------------------------------------------------------------------------------------------------------

void ComponentSpecification::RecordMenu(const DeferredRecording& Holding)
{
    const ControlColour&    Colour     = Appearance->Control;
    const ControlMetric& Measure = Appearance->ControlMeasure;

    const PlaneExtent Menu = MenuEnclosure(Holding.Anchor, Holding.OptionCount);

    Surface->Ground(Menu, Colour.MenuGround, Measure.MenuRadius, CornerAll);
    Surface->Edge(Menu, Colour.MenuEdge, Measure.CardEdgeWeight, Measure.MenuRadius, CornerAll);

    const float OptionHeight = Measure.RowText * 1.5f + Measure.OptionPadY * 2.0f;
    float       Cursor       = Menu.MinimumY + Measure.MenuPad;

    for (std::uint32_t Ordinal = 0u; Ordinal < Holding.OptionCount; ++Ordinal)
    {
        const PlaneExtent Option = Spanning(Menu.MinimumX + Measure.MenuPad, Cursor,
                                            Menu.Width() - Measure.MenuPad * 2.0f, OptionHeight);

        const bool Over = Option.Encloses(Sampled.PositionX, Sampled.PositionY);

        // 📝 The option's hover is read from the pointer directly rather than from an registered fade. Every
        //    option would otherwise need its own identity, and the sheet fades an option in 150 ms — below
        //    the threshold at which the absence of a fade reads as a defect rather than as immediacy.
        if (Over)
        {
            Surface->Ground(Option, Colour.OptionGroundHovered, Option.Height() * 0.5f, CornerAll);
        }

        const char* Caption = (Holding.Options != nullptr) ? Holding.Options[Ordinal] : "";
        const bool  Taken   = Ordinal == Holding.TakenOption;

        Surface->TextRunTruncated(Option.MinimumX + Measure.OptionPadX,
                                  CentredY(Option, Measure.RowText),
                                  Option.Width() - Measure.OptionPadX * 2.0f,
                                  (Over || Taken) ? Colour.OptionColourHovered : Colour.OptionColour,
                                  Caption, Measure.RowText, false);

        Cursor += OptionHeight + Measure.MenuGapY;
    }
}

void ComponentSpecification::RecordTooltip(const DeferredRecording& Holding)
{
    const ControlColour&    Colour     = Appearance->Control;
    const ControlMetric& Measure = Appearance->ControlMeasure;

    const float Disclosed = Ledger->TakenFraction(Holding.Target);

    if (Disclosed <= 0.0f)
        return;

    const bool Light = Holding.Appearance == TooltipAppearance::Light;

    const ThemeToken Ground = Faded(Light ? Colour.TooltipLightGround : Colour.TooltipDarkGround, Disclosed);
    const ThemeToken Title  = Faded(Light ? Colour.TooltipLightTitle  : Colour.TooltipDarkTitle,  Disclosed);
    const ThemeToken Body   = Faded(Light ? Colour.TooltipLightBody   : Colour.TooltipDarkBody,   Disclosed);

    // ① The body is wrapped first, because the card's extent across follows from how many lines it took.
    const float Interior = Measure.TooltipX - Measure.TooltipPad * 2.0f;

    const char*   LineFirst[WrapCeiling] = {};
    std::uint32_t LineExtent[WrapCeiling] = {};
    std::uint32_t Lines = 0u;

    const char* Sweeping = (Holding.Body != nullptr) ? Holding.Body : "";

    while (*Sweeping != '\0' && Lines < WrapCeiling)
    {
        const char*   LineStart = Sweeping;
        const char*   LastBreak = nullptr;
        std::uint32_t Taken     = 0u;

        while (Sweeping[Taken] != '\0')
        {
            if (Sweeping[Taken] == ' ')
            {
                char          Probe[256] = {};
                std::uint32_t Copied     = (Taken < 255u) ? Taken : 255u;

                for (std::uint32_t Ordinal = 0u; Ordinal < Copied; ++Ordinal)
                    Probe[Ordinal] = LineStart[Ordinal];

                Probe[Copied] = '\0';

                if (Surface->MeasureRun(Probe, Measure.TooltipBodyText, 0.0f) > Interior)
                    break;

                LastBreak = Sweeping + Taken;
            }

            ++Taken;
        }

        const char* Breaking = (LastBreak != nullptr && Sweeping[Taken] != '\0') ? LastBreak
                                                                                 : Sweeping + Taken;

        LineFirst[Lines]  = LineStart;
        LineExtent[Lines] = static_cast<std::uint32_t>(Breaking - LineStart);
        ++Lines;

        Sweeping = (*Breaking == ' ') ? Breaking + 1 : Breaking;
    }

    const float BodyHeight  = Measure.TooltipBodyLeading * static_cast<float>(Lines);
    const float CardHeight  = Measure.TooltipPad * 2.0f + Measure.TooltipTitleText * 1.5f
                            + Measure.TooltipTitleGap + BodyHeight;

    // ② The card hangs above its trigger by the declared lift, aligned to the trigger's leading edge.
    const PlaneExtent Card = Spanning(Holding.Anchor.MinimumX,
                                      Holding.Anchor.MinimumY - Measure.TooltipLift - CardHeight,
                                      Measure.TooltipX, CardHeight);

    Surface->Ground(Card, Ground, Measure.TooltipRadius, CornerAll);

    // ③ The arrow — a square turned a quarter turn, tucked under the card's lower edge. Recorded as four
    //    corners rather than as a rotated ground, because the recording surface places extents and the
    //    sheet's own arrow is a rotated div whose rounded corners are hidden behind the card regardless.
    const float ArrowReach  = Measure.TooltipArrowExtent * 0.5f;
    const float ArrowCentre = Card.MinimumX + Measure.TooltipArrowX;
    const float ArrowY = Card.MaximumY - Measure.TooltipArrowScolour;

    const float Corners[8] = { ArrowCentre,              ArrowY - ArrowReach,
                               ArrowCentre + ArrowReach, ArrowY,
                               ArrowCentre,              ArrowY + ArrowReach,
                               ArrowCentre - ArrowReach, ArrowY };

    Surface->Tongue(Corners, 4u, Ground);

    // ④ The title, then every wrapped line of the body.
    Surface->TextRun(Card.MinimumX + Measure.TooltipPad, Card.MinimumY + Measure.TooltipPad,
                     Title, Holding.Title, Measure.TooltipTitleText, 0.0f, true);

    float Cursor = Card.MinimumY + Measure.TooltipPad + Measure.TooltipTitleText * 1.5f
                 + Measure.TooltipTitleGap;

    for (std::uint32_t Ordinal = 0u; Ordinal < Lines; ++Ordinal)
    {
        char          Staging[256] = {};
        std::uint32_t Copied       = (LineExtent[Ordinal] < 255u) ? LineExtent[Ordinal] : 255u;

        for (std::uint32_t Glyph = 0u; Glyph < Copied; ++Glyph)
            Staging[Glyph] = LineFirst[Ordinal][Glyph];

        Staging[Copied] = '\0';

        Surface->TextRun(Card.MinimumX + Measure.TooltipPad, Cursor, Body,
                         Staging, Measure.TooltipBodyText, 0.0f, false);

        Cursor += Measure.TooltipBodyLeading;
    }
}

void ComponentSpecification::RecordDeferred()
{
    if (Surface == nullptr || Appearance == nullptr || Ledger == nullptr)
        return;

    for (std::uint32_t Ordinal = 0u; Ordinal < DeferredCount; ++Ordinal)
    {
        const DeferredRecording& Holding = Deferred[Ordinal];

        if (Holding.Menu)
            RecordMenu(Holding);
        else
            RecordTooltip(Holding);
    }

    DeferredCount = 0u;
}

}   // namespace Slate
