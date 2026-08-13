//============================================================================================================================================
//                                                            CONTROLLAYOUT.CPP
//============================================================================================================================================
// 🧩 The row split, the shared shapes, and the stroke alphabet every control paints its chrome from.

#include "SlateUI/Interface/ControlPanel/Source/ControlInterior.h"

#include <cmath>
#include <cstdio>

namespace Slate
{
namespace ControlInterior
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     CODES AND GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

ImU32 Coded(const ThemeColour& Colour)
{
    return static_cast<ImU32>(Quantize(Colour));
}

ImVec2 Corner(const WorkspaceRectangle& Area)
{
    return ImVec2(Area.PositionX, Area.PositionY);
}

ImVec2 Opposite(const WorkspaceRectangle& Area)
{
    return ImVec2(Area.PositionX + Area.Width, Area.PositionY + Area.Height);
}

ImVec2 Centre(const WorkspaceRectangle& Area)
{
    return ImVec2(Area.PositionX + Area.Width * 0.5f, Area.PositionY + Area.Height * 0.5f);
}

WorkspaceRectangle Inset(const WorkspaceRectangle& Area, float Margin)
{
    WorkspaceRectangle Reduced;

    Reduced.PositionX = Area.PositionX + Margin;
    Reduced.PositionY = Area.PositionY + Margin;
    Reduced.Width     = Area.Width  - Margin * 2.0f;
    Reduced.Height    = Area.Height - Margin * 2.0f;

    if (Reduced.Width  < 0.0f) Reduced.Width  = 0.0f;
    if (Reduced.Height < 0.0f) Reduced.Height = 0.0f;

    return Reduced;
}

WorkspaceRectangle CentredBand(const WorkspaceRectangle& Area, float Height)
{
    WorkspaceRectangle Band = Area;

    Band.Height    = Height < Area.Height ? Height : Area.Height;
    Band.PositionY = Area.PositionY + (Area.Height - Band.Height) * 0.5f;

    return Band;
}

WorkspaceRectangle LeftSlice(const WorkspaceRectangle& Area, float Width)
{
    WorkspaceRectangle Slice = Area;

    Slice.Width = Width < Area.Width ? Width : Area.Width;

    return Slice;
}

WorkspaceRectangle RightSlice(const WorkspaceRectangle& Area, float Width)
{
    WorkspaceRectangle Slice = Area;

    Slice.Width     = Width < Area.Width ? Width : Area.Width;
    Slice.PositionX = Area.PositionX + Area.Width - Slice.Width;

    return Slice;
}

WorkspaceRectangle SquareIn(const WorkspaceRectangle& Area, float Edge)
{
    WorkspaceRectangle Square;

    Square.Width     = Edge;
    Square.Height    = Edge;
    Square.PositionX = Area.PositionX + (Area.Width  - Edge) * 0.5f;
    Square.PositionY = Area.PositionY + (Area.Height - Edge) * 0.5f;

    return Square;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE POINTER
//------------------------------------------------------------------------------------------------------------------------

PointerReading ResolvePointer()
{
    const ImGuiIO& Exchange = ImGui::GetIO();

    PointerReading Reading;

    Reading.PositionX    = Exchange.MousePos.x;
    Reading.PositionY    = Exchange.MousePos.y;
    Reading.TravelX      = Exchange.MouseDelta.x;
    Reading.PressBegan   = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    Reading.PressHeld    = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    Reading.PressEnded   = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    Reading.PressDoubled = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

    return Reading;
}

bool PointerCovers(const PointerReading& Pointer, const WorkspaceRectangle& Area)
{
    return RectangleCovers(Area, Pointer.PositionX, Pointer.PositionY);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      SHARED PAINTING
//------------------------------------------------------------------------------------------------------------------------

// 📝 🔴 The foreground recording, and never a window's own. `WorkspaceSpace` paints every trapezoid there and the
//    bracket opens no window around the desk, so a control recording anywhere else is painted underneath the
//    chrome that contains it.
ImDrawList* Recording()
{
    return ImGui::GetForegroundDrawList();
}

void PaintFill(const WorkspaceRectangle& Area, const ThemeColour& Colour, float Rounding)
{
    if (Area.Width <= 0.0f || Area.Height <= 0.0f)
        return;

    // 📝 A rounding beyond half the shorter side is the reference's `999px` idiom for "fully rounded". It is
    //    bounded here rather than passed through, because the vendor's arc generator degenerates above that.
    const float Shorter  = Area.Width < Area.Height ? Area.Width : Area.Height;
    const float Bounded  = Rounding > Shorter * 0.5f ? Shorter * 0.5f : Rounding;

    Recording()->AddRectFilled(Corner(Area), Opposite(Area), Coded(Colour), Bounded);
}

void PaintOutline(const WorkspaceRectangle& Area, const ThemeColour& Colour, float Rounding, float Thickness)
{
    if (Area.Width <= 0.0f || Area.Height <= 0.0f)
        return;

    const float Shorter = Area.Width < Area.Height ? Area.Width : Area.Height;
    const float Bounded = Rounding > Shorter * 0.5f ? Shorter * 0.5f : Rounding;

    Recording()->AddRect(Corner(Area), Opposite(Area), Coded(Colour), Bounded, 0, Thickness);
}

void PaintDisc(float CentreX, float CentreY, float Radius, const ThemeColour& Colour)
{
    if (Radius <= 0.0f)
        return;

    Recording()->AddCircleFilled(ImVec2(CentreX, CentreY), Radius, Coded(Colour));
}

void PaintCaption(const WorkspaceRectangle&  Area,
                  const char*                Caption,
                  const ThemeColour&         Colour,
                  float                      HorizontalAlignment,
                  float                      VerticalAlignment,
                  float                      FontScale)
{
    if (Caption == nullptr || Caption[0] == '\0' || Area.Width <= 0.0f)
        return;

    const float  BaseSize    = ImGui::GetFontSize();
    const float  ScaledSize  = BaseSize * (FontScale > 0.0f ? FontScale : 1.0f);
    const ImVec2 Measurement = ImGui::GetFont()->CalcTextSizeA(ScaledSize, FLT_MAX, 0.0f, Caption);

    const float PlacedX = Area.PositionX + (Area.Width  - Measurement.x) * HorizontalAlignment;
    const float PlacedY = Area.PositionY + (Area.Height - Measurement.y) * VerticalAlignment;

    // 📝 Clipped rather than ellipsised. The reference ellipsises a label column, and that is the caller's
    //    concern: a control that shortened its own caption would decide for a panel how much of a name matters.
    Recording()->PushClipRect(Corner(Area), Opposite(Area), true);
    Recording()->AddText(ImGui::GetFont(), ScaledSize, ImVec2(PlacedX, PlacedY), Coded(Colour), Caption);
    Recording()->PopClipRect();
}

void PrintReading(char* Destination, std::uint32_t DestinationExtent, double Reading, std::uint32_t Decimals)
{
    if (Destination == nullptr || DestinationExtent == 0u)
        return;

    const int Requested = static_cast<int>(Decimals > 9u ? 9u : Decimals);

    std::snprintf(Destination, static_cast<std::size_t>(DestinationExtent), "%.*f", Requested, Reading);
}

WorkspaceRectangle PaintValueBox(const ThemeSpecification&  Theme,
                                 const WorkspaceRectangle&  Area,
                                 const char*                CapCaption,
                                 float                      CapWidth,
                                 bool                       CapLeading,
                                 const char*                Readout,
                                 bool                       Focused)
{
    const LayoutExtents& Extents = Theme.Extents;

    PaintFill(Area, Theme.Palette.ValueNumberSegment, Extents.EntryRounding);

    WorkspaceRectangle ReadoutArea = Area;

    if (CapWidth > 0.0f && CapCaption != nullptr)
    {
        const WorkspaceRectangle Cap = CapLeading ? LeftSlice(Area, CapWidth) : RightSlice(Area, CapWidth);

        // 📝 The cap is filled only where it is an axis cap. The reference's unit cap carries no fill at all —
        //    it is the same black as the centre with muted lettering — so a fill here would invent a segment
        //    the reference does not draw.
        if (CapLeading)
        {
            PaintFill(Cap, Theme.Palette.ValueSideSegment, Extents.EntryRounding);
            ReadoutArea.PositionX += CapWidth;
        }

        ReadoutArea.Width -= CapWidth;

        PaintCaption(Cap, CapCaption, Theme.Palette.TextMuted, 0.5f, 0.5f, Extents.SegmentFontScale);
    }

    if (Focused)
        PaintOutline(Area, Theme.Palette.SelectionMarker, Extents.EntryRounding, Extents.BorderThickness * 1.5f);

    // 📝 The readout is right-aligned against a right inset of half the padding, which is the reference's 6 px
    //    against its 12 px leading inset. Tabular figures are not available through the vendor's font path, so a
    //    changing digit shifts the run — visible only on a drag, and the alternative is a second font atlas.
    WorkspaceRectangle Printed = ReadoutArea;

    Printed.Width -= Extents.PanelPadding * 0.5f;

    PaintCaption(Printed, Readout, Theme.Palette.ValueText, 1.0f, 0.5f, Extents.NumericFontScale);

    return ReadoutArea;
}

}   // namespace ControlInterior


//------------------------------------------------------------------------------------------------------------------------
//                                                       THE ROW SPLIT
//------------------------------------------------------------------------------------------------------------------------

ControlRowSplit ResolveControlRow(const ThemeSpecification& Theme, const WorkspaceRectangle& Area)
{
    const LayoutExtents& Extents = Theme.Extents;

    ControlRowSplit Split;

    const float Fixed = Extents.LabelColumnWidth + Extents.LabelColumnGap + Extents.ValueColumnWidth;

    float LabelWidth = Extents.LabelColumnWidth;

    // 📝 Below the fixed pair the row falls back to a fraction, so a narrow docked panel keeps a grabbable field
    //    instead of a field clipped to nothing.
    if (Area.Width < Fixed)
        LabelWidth = Area.Width * Extents.LabelColumnRatio;

    Split.LabelArea           = Area;
    Split.LabelArea.Width     = LabelWidth;

    Split.FieldArea           = Area;
    Split.FieldArea.PositionX = Area.PositionX + LabelWidth + Extents.LabelColumnGap;
    Split.FieldArea.Width     = Area.Width - LabelWidth - Extents.LabelColumnGap;

    if (Split.FieldArea.Width < 0.0f)
        Split.FieldArea.Width = 0.0f;

    return Split;
}

void PresentControlLabel(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, const char* Caption)
{
    ControlInterior::PaintCaption(Area, Caption, Theme.Palette.TextMuted, 0.0f, 0.5f, 1.0f);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                     THE STROKE ALPHABET
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📝 Every stroke is authored inside the unit square about the origin and mapped onto the square the caller named,
//    so one alphabet serves a 14 px row glyph and a 26 px header button without a second set of coordinates.
struct StrokeMapping
{
    float  CentreX  = 0.0f;   // [px]
    float  CentreY  = 0.0f;   // [px]
    float  HalfEdge = 0.0f;   // [px]
    float  Cosine   = 1.0f;   // [-]
    float  Sine     = 0.0f;   // [-]
};

ImVec2 Placed(const StrokeMapping& Mapping, float UnitX, float UnitY)
{
    const float TurnedX = UnitX * Mapping.Cosine - UnitY * Mapping.Sine;
    const float TurnedY = UnitX * Mapping.Sine   + UnitY * Mapping.Cosine;

    return ImVec2(Mapping.CentreX + TurnedX * Mapping.HalfEdge,
                  Mapping.CentreY + TurnedY * Mapping.HalfEdge);
}

void StrokeRun(const StrokeMapping&  Mapping,
               const float*          UnitCoordinates,
               std::uint32_t         PointCount,
               ImU32                 Code,
               float                 Thickness,
               bool                  Closed)
{
    ImDrawList* Marking = ControlInterior::Recording();

    for (std::uint32_t Ordinal = 0u; Ordinal + 1u < PointCount; ++Ordinal)
    {
        Marking->AddLine(Placed(Mapping, UnitCoordinates[Ordinal * 2u],      UnitCoordinates[Ordinal * 2u + 1u]),
                         Placed(Mapping, UnitCoordinates[Ordinal * 2u + 2u], UnitCoordinates[Ordinal * 2u + 3u]),
                         Code, Thickness);
    }

    if (Closed && PointCount >= 2u)
    {
        Marking->AddLine(Placed(Mapping, UnitCoordinates[(PointCount - 1u) * 2u],
                                         UnitCoordinates[(PointCount - 1u) * 2u + 1u]),
                         Placed(Mapping, UnitCoordinates[0], UnitCoordinates[1]),
                         Code, Thickness);
    }
}

}   // namespace

void PresentControlStroke(const WorkspaceRectangle& Area,
                          ControlStroke             Stroke,
                          const ThemeColour&        Colour,
                          float                     Thickness,
                          float                     Rotation)
{
    if (Stroke == ControlStroke::None || Area.Width <= 0.0f || Area.Height <= 0.0f)
        return;

    const float Shorter = Area.Width < Area.Height ? Area.Width : Area.Height;

    StrokeMapping Mapping;

    Mapping.CentreX  = Area.PositionX + Area.Width  * 0.5f;
    Mapping.CentreY  = Area.PositionY + Area.Height * 0.5f;
    Mapping.HalfEdge = Shorter * 0.5f;
    Mapping.Cosine   = std::cos(Rotation);
    Mapping.Sine     = std::sin(Rotation);

    const ImU32 Code    = ControlInterior::Coded(Colour);
    ImDrawList* Marking = ControlInterior::Recording();

    switch (Stroke)
    {
    case ControlStroke::Twisty:
    case ControlStroke::Chevron:
    {
        static const float Run[] = { -0.28f, -0.46f,  0.26f, 0.0f, -0.28f, 0.46f };
        StrokeRun(Mapping, Run, 3u, Code, Thickness, false);
        break;
    }

    case ControlStroke::Caret:
    {
        static const float Run[] = { -0.42f, -0.20f, 0.0f, 0.24f, 0.42f, -0.20f };
        StrokeRun(Mapping, Run, 3u, Code, Thickness, false);
        break;
    }

    case ControlStroke::Plus:
    {
        Marking->AddLine(Placed(Mapping, -0.52f, 0.0f), Placed(Mapping, 0.52f, 0.0f), Code, Thickness);
        Marking->AddLine(Placed(Mapping, 0.0f, -0.52f), Placed(Mapping, 0.0f, 0.52f), Code, Thickness);
        break;
    }

    case ControlStroke::Cross:
    {
        Marking->AddLine(Placed(Mapping, -0.40f, -0.40f), Placed(Mapping, 0.40f, 0.40f), Code, Thickness);
        Marking->AddLine(Placed(Mapping, -0.40f,  0.40f), Placed(Mapping, 0.40f, -0.40f), Code, Thickness);
        break;
    }

    case ControlStroke::Check:
    {
        static const float Run[] = { -0.44f, 0.04f, -0.14f, 0.36f, 0.46f, -0.34f };
        StrokeRun(Mapping, Run, 3u, Code, Thickness, false);
        break;
    }

    case ControlStroke::Eye:
    {
        // 📝 Two arcs meeting at the corners rather than an ellipse: the reference's eye is a lens, and an
        //    ellipse reads as a circle at fourteen pixels.
        Marking->PathClear();
        Marking->PathLineTo(Placed(Mapping, -0.56f, 0.0f));
        Marking->PathBezierQuadraticCurveTo(Placed(Mapping, 0.0f, -0.62f), Placed(Mapping, 0.56f, 0.0f));
        Marking->PathStroke(Code, 0, Thickness);
        Marking->PathClear();
        Marking->PathLineTo(Placed(Mapping, -0.56f, 0.0f));
        Marking->PathBezierQuadraticCurveTo(Placed(Mapping, 0.0f, 0.62f), Placed(Mapping, 0.56f, 0.0f));
        Marking->PathStroke(Code, 0, Thickness);
        Marking->AddCircle(Placed(Mapping, 0.0f, 0.0f), Mapping.HalfEdge * 0.20f, Code, 0, Thickness);
        break;
    }

    case ControlStroke::Trash:
    {
        static const float Body[] = { -0.34f, -0.24f, -0.26f, 0.50f, 0.26f, 0.50f, 0.34f, -0.24f };
        StrokeRun(Mapping, Body, 4u, Code, Thickness, false);
        Marking->AddLine(Placed(Mapping, -0.48f, -0.24f), Placed(Mapping, 0.48f, -0.24f), Code, Thickness);
        Marking->AddLine(Placed(Mapping, -0.16f, -0.24f), Placed(Mapping, -0.16f, -0.44f), Code, Thickness);
        Marking->AddLine(Placed(Mapping,  0.16f, -0.24f), Placed(Mapping,  0.16f, -0.44f), Code, Thickness);
        Marking->AddLine(Placed(Mapping, -0.16f, -0.44f), Placed(Mapping,  0.16f, -0.44f), Code, Thickness);
        break;
    }

    case ControlStroke::Search:
    {
        Marking->AddCircle(Placed(Mapping, -0.12f, -0.12f), Mapping.HalfEdge * 0.36f, Code, 0, Thickness);
        Marking->AddLine(Placed(Mapping, 0.16f, 0.16f), Placed(Mapping, 0.50f, 0.50f), Code, Thickness);
        break;
    }

    case ControlStroke::Cog:
    {
        // 📝 Eight teeth as radial spurs about a ring. A toothed outline at this size is indistinguishable from
        //    spurs and costs four times the segments.
        Marking->AddCircle(Placed(Mapping, 0.0f, 0.0f), Mapping.HalfEdge * 0.30f, Code, 0, Thickness);

        for (std::uint32_t Tooth = 0u; Tooth < 8u; ++Tooth)
        {
            const float Bearing = static_cast<float>(Tooth) * 0.7853981634f;
            const float SpurX   = std::cos(Bearing);
            const float SpurY   = std::sin(Bearing);

            Marking->AddLine(Placed(Mapping, SpurX * 0.34f, SpurY * 0.34f),
                             Placed(Mapping, SpurX * 0.54f, SpurY * 0.54f), Code, Thickness);
        }
        break;
    }

    case ControlStroke::Image:
    {
        static const float Border[] = { -0.48f, -0.40f, 0.48f, -0.40f, 0.48f, 0.40f, -0.48f, 0.40f };
        StrokeRun(Mapping, Border, 4u, Code, Thickness, true);
        static const float Ridge[] = { -0.48f, 0.24f, -0.14f, -0.10f, 0.12f, 0.16f, 0.30f, -0.02f, 0.48f, 0.24f };
        StrokeRun(Mapping, Ridge, 5u, Code, Thickness, false);
        Marking->AddCircleFilled(Placed(Mapping, 0.22f, -0.20f), Mapping.HalfEdge * 0.10f, Code);
        break;
    }

    case ControlStroke::Brush:
    {
        static const float Handle[] = { 0.46f, -0.46f, -0.08f, 0.08f };
        StrokeRun(Mapping, Handle, 2u, Code, Thickness, false);
        static const float Ferrule[] = { -0.30f, -0.10f, 0.10f, 0.30f };
        StrokeRun(Mapping, Ferrule, 2u, Code, Thickness * 2.0f, false);
        static const float Bristle[] = { -0.44f, 0.16f, -0.16f, 0.44f };
        StrokeRun(Mapping, Bristle, 2u, Code, Thickness, false);
        break;
    }

    case ControlStroke::Reload:
    {
        Marking->PathClear();
        Marking->PathArcTo(Placed(Mapping, 0.0f, 0.0f), Mapping.HalfEdge * 0.44f, 0.6f, 5.4f, 20);
        Marking->PathStroke(Code, 0, Thickness);
        static const float Arrow[] = { 0.16f, -0.50f, 0.44f, -0.30f, 0.20f, -0.06f };
        StrokeRun(Mapping, Arrow, 3u, Code, Thickness, false);
        break;
    }

    case ControlStroke::Circle:
    {
        Marking->AddCircleFilled(Placed(Mapping, 0.0f, 0.0f), Mapping.HalfEdge * 0.52f, Code);
        break;
    }

    case ControlStroke::Grip:
    {
        for (std::uint32_t Rung = 0u; Rung < 3u; ++Rung)
        {
            const float Offset = -0.10f + static_cast<float>(Rung) * 0.24f;

            Marking->AddLine(Placed(Mapping, 0.54f, Offset), Placed(Mapping, Offset, 0.54f), Code, Thickness);
        }
        break;
    }

    default:
        break;
    }
}

}   // namespace Slate
