//============================================================================================================================================
//                                                            CONTROLCHOICE.CPP
//============================================================================================================================================
// 🧩 The four choosing controls — a crossing nub, a run of exclusive pills, a dropped list, and a row of independent switches.

#include "SlateUI/Interface/ControlPanel/Source/ControlInterior.h"

namespace Slate
{

using namespace ControlInterior;

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       SHARED SHAPES
//------------------------------------------------------------------------------------------------------------------------

// 📝 The reference's pills are laid out by the browser from their text. Nothing here measures text for free, so a
//    pill's width is its caption's advance plus the reference's twelve pixels of inset at either end.
constexpr float PillCaptionInset = 12.0f;

float PillWidth(const char* Caption)
{
    const ImVec2 Measurement = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0.0f, Caption);

    return Measurement.x + PillCaptionInset * 2.0f;
}

/// 🧩 One pill: the face, the hover wash and the caption, in whichever of the two conditions it stands.
void PaintPill(const ThemeSpecification&  Theme,
               const WorkspaceRectangle&  Area,
               const char*                Caption,
               bool                       Chosen,
               bool                       Covered)
{
    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    const ThemeColour Face = Chosen  ? Palette.AccentPrimary
                                     : (Covered ? Palette.ControlHovered : Palette.ControlBackground);
    const ThemeColour Ink  = Chosen  ? Palette.TextOnAccent
                                     : (Covered ? Palette.TextPrimary : Palette.TextMuted);

    PaintFill(Area, Face, Extents.PillRounding);
    PaintCaption(Area, Caption, Ink, 0.5f, 0.5f, Extents.SegmentFontScale);
}

/// 🧩 Whether one ordinal names a choice in a run at all.
bool OrdinalAdmitted(std::uint32_t Ordinal, std::uint32_t ChoiceCount)
{
    return ChoiceCount > 0u && Ordinal < ChoiceCount;
}

}   // namespace


//------------------------------------------------------------------------------------------------------------------------
//                                                     THE BOOLEAN ENTRY
//------------------------------------------------------------------------------------------------------------------------

Outcome<ControlInteraction> PresentBooleanEntry(const ThemeSpecification&  Theme,
                                                const WorkspaceRectangle&  Area,
                                                const char*                Caption,
                                                bool&                      Carried)
{
    const LayoutExtents&  Extents = Theme.Extents;
    const ThemePalette&   Palette = Theme.Palette;
    const ControlRowSplit Split   = ResolveControlRow(Theme, Area);

    PresentControlLabel(Theme, Split.LabelArea, Caption);

    WorkspaceRectangle Travel = CentredBand(Split.FieldArea, Extents.SwitchHeight);

    Travel.Width = Extents.SwitchWidth < Split.FieldArea.Width ? Extents.SwitchWidth : Split.FieldArea.Width;

    ControlInteraction Interaction = ResolvePress(Travel);

    if (Interaction.EditSealed)
    {
        Carried                  = !Carried;
        Interaction.EditDeclared = true;
    }

    // 📝 The travel is fully rounded rather than at the reference's sixteen pixels, which is the same shape: half of
    //    a thirty-two pixel height is sixteen, and `PaintFill` bounds the radius to exactly that.
    PaintFill(Travel, Carried ? Palette.SliderFill : Palette.ControlBackground, Extents.EntryRounding);

    // 📝 🔴 The nub is placed from the carried reading and never interpolated. `14` §4.1 puts animation carry beside
    //    the caller, and a nub that eased here would be retained state in a file that has none by construction.
    const float Inset  = (Extents.SwitchHeight - Extents.SwitchNubEdge) * 0.5f;
    const float Radius = Extents.SwitchNubEdge * 0.5f;
    const float Closed = Travel.PositionX + Inset + Radius;
    const float Opened = Travel.PositionX + Travel.Width - Inset - Radius;

    PaintDisc(Carried ? Opened : Closed, Travel.PositionY + Travel.Height * 0.5f, Radius, Palette.SliderKnob);

    return Outcome<ControlInteraction>::Deliver(Interaction);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                    THE SELECTION ENTRY
//------------------------------------------------------------------------------------------------------------------------

Outcome<ControlInteraction> PresentSelectionEntry(const ThemeSpecification&  Theme,
                                                  const WorkspaceRectangle&  Area,
                                                  const char*                Caption,
                                                  const char* const*         Choices,
                                                  std::uint32_t              ChoiceCount,
                                                  std::uint32_t&             CarriedOrdinal)
{
    if (Choices == nullptr || ChoiceCount == 0u)
        return Outcome<ControlInteraction>::Refuse({ RefusalReason::ContentUnsupported, "no choices were named" });

    if (!OrdinalAdmitted(CarriedOrdinal, ChoiceCount))
    {
        return Outcome<ControlInteraction>::Refuse(
            { RefusalReason::ContentUnsupported, "the carried ordinal names no choice in the run" });
    }

    const LayoutExtents&  Extents = Theme.Extents;
    const ControlRowSplit Split   = ResolveControlRow(Theme, Area);

    PresentControlLabel(Theme, Split.LabelArea, Caption);

    const PointerReading Pointer = ResolvePointer();

    ControlInteraction Interaction;

    // 📝 The run wraps, exactly as the reference's `flex-wrap` does. A row too narrow for the widest pill still
    //    presents it — clipped by its own caption's clip rectangle — rather than dropping it out of the run.
    float PlacedX = Split.FieldArea.PositionX;
    float PlacedY = Split.FieldArea.PositionY;

    for (std::uint32_t Ordinal = 0u; Ordinal < ChoiceCount; ++Ordinal)
    {
        const char* const Choice = Choices[Ordinal] != nullptr ? Choices[Ordinal] : "";
        const float       Width  = PillWidth(Choice);

        if (PlacedX > Split.FieldArea.PositionX &&
            PlacedX + Width > Split.FieldArea.PositionX + Split.FieldArea.Width)
        {
            PlacedX  = Split.FieldArea.PositionX;
            PlacedY += Extents.SegmentRowHeight + Extents.ControlSpacing;
        }

        WorkspaceRectangle Pill;

        Pill.PositionX = PlacedX;
        Pill.PositionY = PlacedY;
        Pill.Width     = Width;
        Pill.Height    = Extents.SegmentRowHeight;

        const ControlInteraction Pressed = ResolvePress(Pill);

        if (Pressed.PointerOver)
            Interaction.PointerOver = true;

        if (Pressed.EditSealed)
        {
            // 📝 The seal is reported for a press on the pill that already stands, and only the amendment is not. A
            //    caller opening a transaction on the seal would otherwise see none for a re-press, and a re-press is
            //    how an artist confirms a choice they are unsure of.
            if (CarriedOrdinal != Ordinal)
            {
                CarriedOrdinal           = Ordinal;
                Interaction.EditDeclared = true;
            }

            Interaction.EditSealed = true;
        }

        PaintPill(Theme, Pill, Choice, CarriedOrdinal == Ordinal, PointerCovers(Pointer, Pill));

        PlacedX += Width + Extents.ControlSpacing;
    }

    return Outcome<ControlInteraction>::Deliver(Interaction);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                       THE DROPDOWN
//------------------------------------------------------------------------------------------------------------------------

Outcome<ControlInteraction> PresentDropdown(const ThemeSpecification&  Theme,
                                            const WorkspaceRectangle&  Area,
                                            const char* const*         Choices,
                                            std::uint32_t              ChoiceCount,
                                            std::uint32_t&             CarriedOrdinal,
                                            DropdownCarry&             Carry,
                                            std::uint32_t              PresentedTick)
{
    if (Choices == nullptr || ChoiceCount == 0u)
        return Outcome<ControlInteraction>::Refuse({ RefusalReason::ContentUnsupported, "no choices were named" });

    if (!OrdinalAdmitted(CarriedOrdinal, ChoiceCount))
    {
        return Outcome<ControlInteraction>::Refuse(
            { RefusalReason::ContentUnsupported, "the carried ordinal names no choice in the list" });
    }

    const LayoutExtents& Extents = Theme.Extents;
    const ThemePalette&  Palette = Theme.Palette;
    const PointerReading Pointer = ResolvePointer();

    const WorkspaceRectangle Head = CentredBand(Area, Extents.DropdownHeight);

    ControlInteraction Interaction = ResolvePress(Head);

    if (Interaction.EditSealed)
    {
        Carry.ListOpen   = !Carry.ListOpen;
        Carry.OpenedTick = PresentedTick;
        Carry.AnchorX    = Head.PositionX;
        Carry.AnchorY    = Head.PositionY + Head.Height + Extents.BorderThickness * 4.0f;
    }

    // -- the head ----------------------------------------------------------------------------------------------------
    const WorkspaceRectangle Cap = RightSlice(Head, Extents.DropdownCaretWidth);

    PaintFill(Head, Palette.ValueNumberSegment, Extents.EntryRounding);
    PaintFill(Cap, PointerCovers(Pointer, Head) ? Palette.TileHovered : Palette.ValueSideSegment,
              Extents.EntryRounding);

    WorkspaceRectangle Chosen = Head;

    Chosen.PositionX += Extents.PanelPadding;
    Chosen.Width     -= Extents.PanelPadding + Extents.DropdownCaretWidth;

    PaintCaption(Chosen, Choices[CarriedOrdinal] != nullptr ? Choices[CarriedOrdinal] : "",
                 Palette.ValueText, 0.0f, 0.5f, Extents.SegmentFontScale);

    // 📝 The caret turns a half rather than the reference's hundred and eighty degrees of `rotate`, which is the
    //    same turn: a caret is symmetric about its own vertical, so a half is what inverts it.
    PresentControlStroke(SquareIn(Cap, Extents.GlyphEdge * 0.8f), ControlStroke::Caret, Palette.TextMuted,
                         Extents.BorderThickness * 1.6f, Carry.ListOpen ? 3.14159265f : 0.0f);

    if (!Carry.ListOpen)
        return Outcome<ControlInteraction>::Deliver(Interaction);

    // -- the dropped list --------------------------------------------------------------------------------------------
    // 📝 ⚠️ Painted in call order on one recording, so a control presented after this one lands over the list. The
    //    reference has a stacking order and a recording has none. The remedy is a deferred overlay record —
    //    `WorkspaceOverlayRecord` in `WorkspaceSpace.h` — and not a second recording invented here.
    WorkspaceRectangle List;

    List.PositionX = Carry.AnchorX;
    List.PositionY = Carry.AnchorY;
    List.Width     = Head.Width;
    List.Height    = static_cast<float>(ChoiceCount) * Extents.OverlayRowHeight
                   + Extents.BorderThickness * 8.0f;

    PaintFill(List, Palette.PanelHeader, Extents.PillRounding);
    PaintOutline(List, Palette.PanelBorder, Extents.PillRounding, Extents.BorderThickness);

    bool ListCovered = PointerCovers(Pointer, List);

    for (std::uint32_t Ordinal = 0u; Ordinal < ChoiceCount; ++Ordinal)
    {
        WorkspaceRectangle Row;

        Row.PositionX = List.PositionX + Extents.BorderThickness * 4.0f;
        Row.PositionY = List.PositionY + Extents.BorderThickness * 4.0f
                      + static_cast<float>(Ordinal) * Extents.OverlayRowHeight;
        Row.Width     = List.Width - Extents.BorderThickness * 8.0f;
        Row.Height    = Extents.OverlayRowHeight;

        const bool               RowCovered = PointerCovers(Pointer, Row);
        const ControlInteraction Pressed    = ResolvePress(Row);

        if (RowCovered)
        {
            PaintFill(Row, Palette.RowHovered, 0.0f);

            // 📝 The four pixel leading bar the reference paints on a hovered entry. Its indent is not ported: a
            //    caption that shifts under the pointer costs a re-measure per tick and reads as a jitter here.
            PaintFill(LeftSlice(Row, Extents.GutterThickness), Palette.SelectionMarker, 0.0f);
        }

        WorkspaceRectangle Lettering = Row;

        Lettering.PositionX += Extents.PanelPadding;
        Lettering.Width     -= Extents.PanelPadding * 2.0f + Extents.GlyphEdge;

        PaintCaption(Lettering, Choices[Ordinal] != nullptr ? Choices[Ordinal] : "",
                     RowCovered ? Palette.TextPrimary : Palette.TextMuted, 0.0f, 0.5f, Extents.SegmentFontScale);

        // 📝 The marker dot the reference draws as a radio: an outline always, filled where the entry is the chosen
        //    one, so the list states which choice stands rather than only which one the pointer is over.
        const WorkspaceRectangle Marker = SquareIn(RightSlice(Row, Extents.GlyphEdge), Extents.GlyphEdge * 0.7f);
        const float              Radius = Marker.Width * 0.5f;

        if (CarriedOrdinal == Ordinal)
            PaintDisc(Marker.PositionX + Radius, Marker.PositionY + Radius, Radius, Palette.SelectionMarker);
        else
            PaintOutline(Marker, Palette.PanelBorder, Extents.EntryRounding, Extents.BorderThickness * 1.4f);

        if (Pressed.EditSealed)
        {
            if (CarriedOrdinal != Ordinal)
            {
                CarriedOrdinal           = Ordinal;
                Interaction.EditDeclared = true;
            }

            Interaction.EditSealed = true;
            Carry.ListOpen         = false;
            ListCovered            = true;
        }
    }

    // 📝 🔴 A press that lands on neither the head nor the list dismisses it — but not on the tick the head opened
    //    it, which is what the carry's opening tick is for. Without that comparison the press that opens the list is
    //    also the press that is outside it once the head has been resolved, and the list closes on the tick it opens.
    if (Pointer.PressBegan && !ListCovered && !PointerCovers(Pointer, Head) && PresentedTick != Carry.OpenedTick)
    {
        Carry.ListOpen             = false;
        Interaction.EditAbandoned  = true;
    }

    return Outcome<ControlInteraction>::Deliver(Interaction);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                      THE SEGMENT ROW
//------------------------------------------------------------------------------------------------------------------------

Outcome<ControlInteraction> PresentSegmentRow(const ThemeSpecification&  Theme,
                                              const WorkspaceRectangle&  Area,
                                              const char* const*         Captions,
                                              bool*                      Carried,
                                              std::uint32_t              SegmentCount)
{
    if (Captions == nullptr || Carried == nullptr || SegmentCount == 0u)
        return Outcome<ControlInteraction>::Refuse({ RefusalReason::ContentUnsupported, "no segments were named" });

    const LayoutExtents& Extents = Theme.Extents;
    const ThemePalette&  Palette = Theme.Palette;
    const PointerReading Pointer = ResolvePointer();

    const WorkspaceRectangle Bar = CentredBand(Area, Extents.SegmentRowHeight);

    if (Bar.Width / static_cast<float>(SegmentCount) < Extents.GlyphEdge)
    {
        return Outcome<ControlInteraction>::Refuse(
            { RefusalReason::ExtentExhausted, "the row cannot carry one glyph per segment" });
    }

    PaintFill(Bar, Palette.TileBackground, Extents.EntryRounding);

    ControlInteraction Interaction;

    const float CellWidth = Bar.Width / static_cast<float>(SegmentCount);

    for (std::uint32_t Ordinal = 0u; Ordinal < SegmentCount; ++Ordinal)
    {
        WorkspaceRectangle Cell = Bar;

        Cell.PositionX = Bar.PositionX + static_cast<float>(Ordinal) * CellWidth;
        Cell.Width     = CellWidth;

        const bool               Covered = PointerCovers(Pointer, Cell);
        const ControlInteraction Pressed = ResolvePress(Cell);

        if (Pressed.PointerOver)
            Interaction.PointerOver = true;

        // 📝 🔴 Each segment carries its own reading and pressing one clears none of the others. This is what
        //    distinguishes the row from a selection entry, and reading it as exclusive is the defect where an
        //    artist enabling a second channel silently disables the first.
        if (Pressed.EditSealed)
        {
            Carried[Ordinal]          = !Carried[Ordinal];
            Interaction.EditDeclared  = true;
            Interaction.EditSealed    = true;
        }

        // 📝 The end cells take the bar's rounding on their outer corners only, so the run reads as one bar and not
        //    as a row of pills. The vendor's corner flags are what make that a single fill rather than a mask.
        if (Carried[Ordinal] || Covered)
        {
            ImDrawFlags Corners = ImDrawFlags_RoundCornersNone;

            if (Ordinal == 0u)                  Corners = ImDrawFlags_RoundCornersLeft;
            if (Ordinal + 1u == SegmentCount)   Corners = Ordinal == 0u ? ImDrawFlags_RoundCornersAll
                                                                       : ImDrawFlags_RoundCornersRight;

            const float Shorter  = Cell.Width < Cell.Height ? Cell.Width : Cell.Height;
            const float Rounding = Shorter * 0.5f;

            Recording()->AddRectFilled(Corner(Cell), Opposite(Cell),
                                       Coded(Carried[Ordinal] ? Palette.AccentPrimary : Palette.TileHovered),
                                       Rounding, Corners);
        }

        PaintCaption(Cell, Captions[Ordinal] != nullptr ? Captions[Ordinal] : "",
                     Carried[Ordinal] ? Palette.TextOnAccent : (Covered ? Palette.TextPrimary : Palette.TextMuted),
                     0.5f, 0.5f, Extents.SegmentFontScale);
    }

    PaintOutline(Bar, Palette.PanelBorder, Extents.EntryRounding, Extents.BorderThickness);

    return Outcome<ControlInteraction>::Deliver(Interaction);
}

}   // namespace Slate
