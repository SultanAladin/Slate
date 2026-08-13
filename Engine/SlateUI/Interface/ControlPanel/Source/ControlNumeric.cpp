//============================================================================================================================================
//                                                           CONTROLNUMERIC.CPP
//============================================================================================================================================
// 🧩 The three numeric entries — a bounded slider, an unbounded scalar drag, and three components side by side.

#include "SlateUI/Interface/ControlPanel/Source/ControlInterior.h"

#include <cmath>

namespace Slate
{

using namespace ControlInterior;

// 📝 The track, the press and the readout buffer now live in `ControlInterior`. A boolean entry's nub and a colour
//    coordinate's bar are the same track this file drags, and two copies of it were two places every amendment to
//    the knob's inset had to land.


//------------------------------------------------------------------------------------------------------------------------
//                                                       THE VALUE SLIDER
//------------------------------------------------------------------------------------------------------------------------

Outcome<ControlInteraction> PresentValueSlider(const ThemeSpecification&  Theme,
                                               const WorkspaceRectangle&  Area,
                                               const char*                Caption,
                                               double&                    Carried,
                                               double                     Floor,
                                               double                     Ceiling,
                                               const char*                Unit,
                                               std::uint32_t              Decimals)
{
    if (!(Ceiling > Floor))
    {
        return Outcome<ControlInteraction>::Refuse(
            { RefusalReason::ContentUnsupported, "the slider's ceiling does not exceed its floor" });
    }

    const LayoutExtents&  Extents = Theme.Extents;
    const ControlRowSplit Split   = ResolveControlRow(Theme, Area);

    const float BoxWidth = Extents.NumericEntryWidth;
    const float Gap      = Extents.ControlSpacing;

    if (Split.FieldArea.Width < BoxWidth + Gap + Extents.SliderKnobEdge * 2.0f)
    {
        return Outcome<ControlInteraction>::Refuse(
            { RefusalReason::ExtentExhausted, "the field cannot carry a value box and a grabbable track" });
    }

    PresentControlLabel(Theme, Split.LabelArea, Caption);

    const WorkspaceRectangle Row = CentredBand(Split.FieldArea, Extents.SwitchHeight);

    WorkspaceRectangle BoxArea = LeftSlice(Row, BoxWidth);

    WorkspaceRectangle TrackArea    = Row;
    TrackArea.PositionX            += BoxWidth + Gap;
    TrackArea.Width                -= BoxWidth + Gap;

    const TrackHold Held = ResolveTrack(TrackArea, static_cast<const void*>(&Carried));

    ControlInteraction Interaction = Held.Interaction;

    if (Held.HoldOpen || Interaction.EditOpened)
    {
        const double Proposed = Floor + static_cast<double>(Held.Fraction) * (Ceiling - Floor);
        const double Amended  = Decimals == 0u ? std::round(Proposed) : Proposed;

        if (Amended != Carried)
        {
            Carried                  = Bounded(Amended, Floor, Ceiling);
            Interaction.EditDeclared = true;
        }
    }

    const double Reading  = Bounded(Carried, Floor, Ceiling);
    const float  Fraction = static_cast<float>((Reading - Floor) / (Ceiling - Floor));

    char Readout[ReadoutExtent] = {};

    PrintReading(Readout, ReadoutExtent, Reading, Decimals);

    PaintValueBox(Theme, BoxArea, Unit, Extents.SideSegmentWidth, false, Readout, false);
    PaintTrack(Theme, TrackArea, Fraction, true, Held.HoldOpen);

    return Outcome<ControlInteraction>::Deliver(Interaction);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                      THE SCALAR ENTRY
//------------------------------------------------------------------------------------------------------------------------

Outcome<ControlInteraction> PresentScalarEntry(const ThemeSpecification&  Theme,
                                               const WorkspaceRectangle&  Area,
                                               const char*                Caption,
                                               double&                    Carried,
                                               double                     Step,
                                               const char*                Unit,
                                               std::uint32_t              Decimals)
{
    const LayoutExtents&  Extents = Theme.Extents;
    const ControlRowSplit Split   = ResolveControlRow(Theme, Area);

    const float BoxWidth = Extents.NumericEntryWidth;
    const float Gap      = Extents.ControlSpacing;

    if (Split.FieldArea.Width < BoxWidth + Gap + Extents.SliderKnobEdge * 2.0f)
    {
        return Outcome<ControlInteraction>::Refuse(
            { RefusalReason::ExtentExhausted, "the field cannot carry a value box and a grabbable track" });
    }

    PresentControlLabel(Theme, Split.LabelArea, Caption);

    const WorkspaceRectangle Row     = CentredBand(Split.FieldArea, Extents.SwitchHeight);
    const WorkspaceRectangle BoxArea = LeftSlice(Row, BoxWidth);

    WorkspaceRectangle TrackArea = Row;

    TrackArea.PositionX += BoxWidth + Gap;
    TrackArea.Width     -= BoxWidth + Gap;

    const PointerReading Pointer = ResolvePointer();
    const TrackHold      Held    = ResolveTrack(TrackArea, static_cast<const void*>(&Carried));

    ControlInteraction Interaction = Held.Interaction;

    // 📝 🔴 Travel is accumulated, never mapped. This control has no span, so the pointer's absolute position
    //    means nothing to it — only how far the pointer moved since the previous tick does.
    if (Held.HoldOpen && Pointer.TravelX != 0.0f)
    {
        const double Proposed = Carried + static_cast<double>(Pointer.TravelX) * Step;

        Carried                  = Decimals == 0u ? std::round(Proposed) : Proposed;
        Interaction.EditDeclared = true;
    }

    char Readout[ReadoutExtent] = {};

    PrintReading(Readout, ReadoutExtent, Carried, Decimals);

    PaintValueBox(Theme, BoxArea, Unit, Extents.SideSegmentWidth, false, Readout, false);
    PaintTrack(Theme, TrackArea, 0.5f, false, Held.HoldOpen);

    return Outcome<ControlInteraction>::Deliver(Interaction);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                      THE VECTOR ENTRY
//------------------------------------------------------------------------------------------------------------------------

Outcome<ControlInteraction> PresentVectorEntry(const ThemeSpecification&  Theme,
                                               const WorkspaceRectangle&  Area,
                                               const char*                Caption,
                                               double                     Carried[3],
                                               double                     Step,
                                               std::uint32_t              Decimals)
{
    if (Carried == nullptr)
        return Outcome<ControlInteraction>::Refuse({ RefusalReason::ContentUnsupported, "no components were named" });

    const LayoutExtents&  Extents = Theme.Extents;
    const ControlRowSplit Split   = ResolveControlRow(Theme, Area);

    const float Gap           = Extents.ControlSpacing * 0.5f;
    const float ComponentSpan = (Split.FieldArea.Width - Gap * 2.0f) / 3.0f;

    if (ComponentSpan < Extents.AxisSegmentWidth * 2.0f)
    {
        return Outcome<ControlInteraction>::Refuse(
            { RefusalReason::ExtentExhausted, "the field cannot carry three components" });
    }

    PresentControlLabel(Theme, Split.LabelArea, Caption);

    static const char* const AxisCaptions[3] = { "X", "Y", "Z" };

    const PointerReading     Pointer = ResolvePointer();
    const WorkspaceRectangle Row     = CentredBand(Split.FieldArea, Extents.SwitchHeight);

    ControlInteraction Interaction;

    for (std::uint32_t Component = 0u; Component < 3u; ++Component)
    {
        WorkspaceRectangle BoxArea = Row;

        BoxArea.PositionX = Row.PositionX + static_cast<float>(Component) * (ComponentSpan + Gap);
        BoxArea.Width     = ComponentSpan;

        const TrackHold Held = ResolveTrack(BoxArea, static_cast<const void*>(&Carried[Component]));

        if (Held.Interaction.PointerOver) Interaction.PointerOver = true;
        if (Held.Interaction.EditOpened)  Interaction.EditOpened  = true;
        if (Held.Interaction.EditSealed)  Interaction.EditSealed  = true;

        if (Held.HoldOpen && Pointer.TravelX != 0.0f)
        {
            const double Proposed = Carried[Component] + static_cast<double>(Pointer.TravelX) * Step;

            Carried[Component]       = Decimals == 0u ? std::round(Proposed) : Proposed;
            Interaction.EditDeclared = true;
        }

        char Readout[ReadoutExtent] = {};

        PrintReading(Readout, ReadoutExtent, Carried[Component], Decimals);

        PaintValueBox(Theme, BoxArea, AxisCaptions[Component], Extents.AxisSegmentWidth, true, Readout, false);
    }

    return Outcome<ControlInteraction>::Deliver(Interaction);
}

}   // namespace Slate
