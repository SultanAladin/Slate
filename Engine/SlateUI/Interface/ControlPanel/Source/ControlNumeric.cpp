//============================================================================================================================================
//                                                           CONTROLNUMERIC.CPP
//============================================================================================================================================
// 🧩 The three numeric entries — a bounded slider, an unbounded scalar drag, and three components side by side.

#include "SlateUI/Interface/ControlPanel/Source/ControlInterior.h"

#include <cmath>

namespace Slate
{

using namespace ControlInterior;

namespace
{

// 📝 The readout buffer. Sixteen digits of a double plus a sign, a point and a terminator do not reach this, and
//    a fixed extent keeps every control on this file's path allocation-free.
constexpr std::uint32_t ReadoutExtent = 40u;

double Bounded(double Reading, double Floor, double Ceiling)
{
    return Reading < Floor ? Floor : (Reading > Ceiling ? Ceiling : Reading);
}

/// 🧩 One grabbable track: reports the interaction and, while held, the fraction the pointer names.
/// note  🔴 The hold is identified by the vendor's active identity and not by "the pointer is down over me". A
///        drag that left the track would otherwise stop amending the moment it did, which is the defect where a
///        slider drops the reading if the artist's hand strays a few pixels below the row.
struct TrackHold
{
    ControlInteraction  Interaction = {};
    bool                HoldOpen    = false;   // [-] - this track owns the pointer
    float               Fraction    = 0.0f;    // [-] - where along it the pointer sits, bounded
};

TrackHold ResolveTrack(const WorkspaceRectangle& Area, const void* Anchor)
{
    const PointerReading Pointer = ResolvePointer();
    const ImGuiID        Claim   = ImGui::GetID(Anchor);

    TrackHold Held;

    Held.Interaction.PointerOver = PointerCovers(Pointer, Area);

    if (Held.Interaction.PointerOver && Pointer.PressBegan && ImGui::GetActiveID() == 0u)
    {
        ImGui::SetActiveID(Claim, ImGui::GetCurrentWindowRead());
        Held.Interaction.EditOpened = true;
    }

    if (ImGui::GetActiveID() == Claim)
    {
        Held.HoldOpen = true;

        // 📝 🔴 The claim is renewed every tick it is held. The vendor drops an active identity that no item
        //    marked alive during the previous tick, and marking alive is something its own item bracket does —
        //    which this control has none of, because it never enters one. Without the renewal the hold survives
        //    exactly one tick past the press, and the defect presents as a drag that amends the reading once and
        //    then goes dead under a pointer that is still down.
        ImGui::KeepAliveID(Claim);

        if (Pointer.PressEnded || !Pointer.PressHeld)
        {
            ImGui::ClearActiveID();
            Held.HoldOpen                = false;
            Held.Interaction.EditSealed  = true;
        }
    }

    const float Span = Area.Width > 0.0f ? Area.Width : 1.0f;

    Held.Fraction = (Pointer.PositionX - Area.PositionX) / Span;
    Held.Fraction = Held.Fraction < 0.0f ? 0.0f : (Held.Fraction > 1.0f ? 1.0f : Held.Fraction);

    return Held;
}

/// 🧩 Paints the track, its travelled fill and its knob at a declared fraction.
void PaintTrack(const ThemeSpecification&  Theme,
                const WorkspaceRectangle&  Area,
                float                      Fraction,
                bool                       FillTravelled,
                bool                       Held)
{
    const LayoutExtents& Extents = Theme.Extents;

    const WorkspaceRectangle Track = CentredBand(Area, Extents.SliderTrackHeight);

    PaintFill(Track, Theme.Palette.SliderTrack, Extents.EntryRounding);

    if (FillTravelled && Fraction > 0.0f)
    {
        WorkspaceRectangle Travelled = Track;

        Travelled.Width = Track.Width * Fraction;

        PaintFill(Travelled, Theme.Palette.SliderFill, Extents.EntryRounding);
    }

    // 📝 The knob is inset by its own radius at both ends so that it sits inside the track at nought and at one
    //    rather than half outside it. The reference lets it overhang; the reference is a rounded div and this is
    //    a quad on a recording, where the overhang reads as a knob that has come loose.
    const float Radius  = Extents.SliderKnobEdge * 0.5f;
    const float Travel  = Track.Width - Radius * 2.0f;
    const float KnobX   = Track.PositionX + Radius + (Travel > 0.0f ? Travel * Fraction : 0.0f);
    const float KnobY   = Track.PositionY + Track.Height * 0.5f;

    PaintDisc(KnobX, KnobY, Radius, Theme.Palette.SliderKnob);

    if (Held)
        PaintDisc(KnobX, KnobY, Radius + Theme.Extents.BorderThickness * 3.0f,
                  Attenuate(Theme.Palette.SelectionMarker, 0.12));
}

}   // namespace


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
