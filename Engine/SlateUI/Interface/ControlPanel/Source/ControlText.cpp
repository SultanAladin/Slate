//============================================================================================================================================
//                                                             CONTROLTEXT.CPP
//============================================================================================================================================
// 🧩 The three text controls and the caret accumulator beneath them — a field, a bare inline edit, and a path with its browse cap.

#include "SlateUI/Interface/ControlPanel/Source/ControlInterior.h"

namespace Slate
{

using namespace ControlInterior;

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE CARET ACCUMULATOR
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What one tick of an open edit produced.
enum class EditProgress : std::uint32_t
{
    Continuing = 0u,   // [-] - the edit is still open
    Amended    = 1u,   // [-] - still open, and the carry changed this tick
    Sealed     = 2u,   // [-] - accepted; the carry is what the owner should take
    Abandoned  = 3u    // [-] - discarded; the owner's text is untouched
};

// 📝 🔴 A hand-rolled accumulator rather than the vendor's own entry, for the same reason `WorkspaceStrip.cpp`'s
//    rename is one: the vendor's needs a window and nothing here opens one. This is that accumulator with the caret
//    `TextCarry` carries and the rename record does not, which is what `3` owed over `2c`.

/// 🧩 Inserts one printable character at the caret and advances it.
bool AdmitCharacter(TextCarry& Carry, char Arrived)
{
    if (Carry.CarryExtent + 1u >= ControlTextExtent)
        return false;

    if (Carry.CaretPosition > Carry.CarryExtent)
        Carry.CaretPosition = Carry.CarryExtent;

    for (std::uint32_t Ordinal = Carry.CarryExtent; Ordinal > Carry.CaretPosition; --Ordinal)
        Carry.Carried[Ordinal] = Carry.Carried[Ordinal - 1u];

    Carry.Carried[Carry.CaretPosition] = Arrived;

    ++Carry.CarryExtent;
    ++Carry.CaretPosition;

    Carry.Carried[Carry.CarryExtent] = '\0';

    return true;
}

/// 🧩 Removes one character at a named position and draws the tail back over it.
bool WithdrawCharacter(TextCarry& Carry, std::uint32_t Position)
{
    if (Position >= Carry.CarryExtent)
        return false;

    for (std::uint32_t Ordinal = Position; Ordinal + 1u < Carry.CarryExtent; ++Ordinal)
        Carry.Carried[Ordinal] = Carry.Carried[Ordinal + 1u];

    --Carry.CarryExtent;

    Carry.Carried[Carry.CarryExtent] = '\0';

    return true;
}

EditProgress AdvanceTextCarry(TextCarry& Carry)
{
    const ImGuiIO& Arriving = ImGui::GetIO();

    // 📝 🔴 The keyboard is claimed for as long as the edit stands. Without this the host reads the same keys as its
    //    own shortcuts, and the defect presents as typing a layer's name also invoking whatever those letters are
    //    bound to — destructively, where one of them is a discard.
    ImGui::SetNextFrameWantCaptureKeyboard(true);

    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        return EditProgress::Abandoned;

    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
        return EditProgress::Sealed;

    bool Amended = false;

    if (Carry.CaretPosition > Carry.CarryExtent)
        Carry.CaretPosition = Carry.CarryExtent;

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true) && Carry.CaretPosition > 0u)
        --Carry.CaretPosition;

    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true) && Carry.CaretPosition < Carry.CarryExtent)
        ++Carry.CaretPosition;

    if (ImGui::IsKeyPressed(ImGuiKey_Home, false))
        Carry.CaretPosition = 0u;

    if (ImGui::IsKeyPressed(ImGuiKey_End, false))
        Carry.CaretPosition = Carry.CarryExtent;

    if (ImGui::IsKeyPressed(ImGuiKey_Backspace, true) && Carry.CaretPosition > 0u)
    {
        --Carry.CaretPosition;
        Amended = WithdrawCharacter(Carry, Carry.CaretPosition) || Amended;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete, true))
        Amended = WithdrawCharacter(Carry, Carry.CaretPosition) || Amended;

    for (int Ordinal = 0; Ordinal < Arriving.InputQueueCharacters.Size; ++Ordinal)
    {
        const ImWchar Arrived = Arriving.InputQueueCharacters[Ordinal];

        // 📝 Only the printable single-byte range is accepted. A caption is not a document, and a control character
        //    in one presents as a blank the artist can neither see nor delete.
        if (Arrived < 0x20 || Arrived > 0x7E)
            continue;

        if (!AdmitCharacter(Carry, static_cast<char>(Arrived)))
            break;

        Amended = true;
    }

    return Amended ? EditProgress::Amended : EditProgress::Continuing;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE SHARED FIELD
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The advance of a carry's leading characters, which is where its caret stands.
float CaretAdvance(const TextCarry& Carry)
{
    if (Carry.CaretPosition == 0u)
        return 0.0f;

    const char* const Leading = Carry.Carried;
    const char* const Ending  = Carry.Carried + Carry.CaretPosition;

    return ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0.0f, Leading, Ending).x;
}

/// 🧩 Resolves one field's opening, typing and sealing, and paints the text and the caret inside it.
/// in    Frame      [px]  what the pointer is resolved against; the frame's own fill is the caller's
/// in    Placement  [px]  where the text is laid, already inset from that frame
/// in    Settled    [-]   what to present while no edit is open; the carry itself where none is named
ControlInteraction AdvanceField(const ThemeSpecification&  Theme,
                               const WorkspaceRectangle&  Frame,
                               const WorkspaceRectangle&  Placement,
                               TextCarry&                 Carry,
                               const char*                Placeholder,
                               const char*                Settled)
{
    const LayoutExtents& Extents = Theme.Extents;
    const ThemePalette&  Palette = Theme.Palette;
    const PointerReading Pointer = ResolvePointer();

    ControlInteraction Interaction;

    const ControlInteraction Pressed = ResolvePress(Frame);

    Interaction.PointerOver = Pressed.PointerOver;

    if (Pressed.EditSealed && !Carry.EditOpen)
    {
        Carry.EditOpen          = true;
        Carry.CaretPosition     = Carry.CarryExtent;
        Interaction.EditOpened  = true;
    }
    else if (Carry.EditOpen)
    {
        const EditProgress Advanced = AdvanceTextCarry(Carry);

        if (Advanced == EditProgress::Amended)
            Interaction.EditDeclared = true;

        if (Advanced == EditProgress::Sealed)
        {
            Carry.EditOpen         = false;
            Interaction.EditSealed = true;
        }

        if (Advanced == EditProgress::Abandoned)
        {
            Carry.EditOpen            = false;
            Interaction.EditAbandoned = true;
        }

        // 📝 A press outside the frame seals rather than abandons, which is the reference's blur behaviour. Escape is
        //    the only route that discards, so an artist who clicks away does not silently lose what was typed.
        if (Carry.EditOpen && Pointer.PressBegan && !PointerCovers(Pointer, Frame))
        {
            Carry.EditOpen         = false;
            Interaction.EditSealed = true;
        }
    }

    // -- the text and its caret --------------------------------------------------------------------------------------
    const bool        Editing   = Carry.EditOpen;
    const char* const Presented = Editing ? Carry.Carried : (Settled != nullptr ? Settled : Carry.Carried);
    const bool        Empty     = Presented == nullptr || Presented[0] == '\0';

    if (Empty && !Editing && Placeholder != nullptr)
        PaintCaption(Placement, Placeholder, Palette.TextMuted, 0.0f, 0.5f, 1.0f);
    else
        PaintCaption(Placement, Presented, Palette.ValueText, 0.0f, 0.5f, 1.0f);

    if (Editing)
    {
        WorkspaceRectangle Caret;

        Caret.PositionX = Placement.PositionX + CaretAdvance(Carry);
        Caret.PositionY = Placement.PositionY + Extents.BorderThickness * 2.0f;
        Caret.Width     = Extents.BorderThickness;
        Caret.Height    = Placement.Height - Extents.BorderThickness * 4.0f;

        // 📝 The caret does not blink. A blink needs a tick clock this control is not handed, and a steady caret is
        //    the one thing about it an artist never has to wait to see.
        PaintFill(Caret, Palette.SelectionMarker, 0.0f);
    }

    return Interaction;
}

}   // namespace


//------------------------------------------------------------------------------------------------------------------------
//                                                       THE TEXT ENTRY
//------------------------------------------------------------------------------------------------------------------------

Outcome<ControlInteraction> PresentTextEntry(const ThemeSpecification&  Theme,
                                             const WorkspaceRectangle&  Area,
                                             const char*                Caption,
                                             TextCarry&                 Carry,
                                             const char*                Placeholder)
{
    const LayoutExtents&  Extents = Theme.Extents;
    const ThemePalette&   Palette = Theme.Palette;
    const ControlRowSplit Split   = ResolveControlRow(Theme, Area);

    if (Split.FieldArea.Width < Extents.GlyphEdge + Extents.PanelPadding * 2.0f)
    {
        return Outcome<ControlInteraction>::Refuse(
            { RefusalReason::ExtentExhausted, "the field is narrower than one glyph" });
    }

    PresentControlLabel(Theme, Split.LabelArea, Caption);

    const WorkspaceRectangle Frame = CentredBand(Split.FieldArea, Extents.EntryRowHeight);

    // 📝 Seven pixels and not the path field's fully rounded radius. The reference overrides exactly this one corner
    //    radius where the same field carries free text rather than a path, and the squarer field is what tells an
    //    artist the two are different things.
    const float Rounding = Extents.PillRounding * 0.78f;

    PaintFill(Frame, Palette.ValueNumberSegment, Rounding);

    if (Carry.EditOpen)
        PaintOutline(Frame, Palette.SelectionMarker, Rounding, Extents.BorderThickness * 1.5f);

    WorkspaceRectangle Placement = Frame;

    Placement.PositionX += Extents.PanelPadding;
    Placement.Width     -= Extents.PanelPadding * 2.0f;

    const ControlInteraction Interaction = AdvanceField(Theme, Frame, Placement, Carry, Placeholder, nullptr);

    return Outcome<ControlInteraction>::Deliver(Interaction);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                   THE INLINE TEXT EDITOR
//------------------------------------------------------------------------------------------------------------------------

Outcome<ControlInteraction> PresentInlineTextEditor(const ThemeSpecification&  Theme,
                                                    const WorkspaceRectangle&  Area,
                                                    TextCarry&                 Carry)
{
    const LayoutExtents& Extents = Theme.Extents;
    const ThemePalette&  Palette = Theme.Palette;

    if (Area.Width <= 0.0f || Area.Height <= 0.0f)
        return Outcome<ControlInteraction>::Refuse({ RefusalReason::ExtentExhausted, "the edit has no area to sit in" });

    // 📝 🔴 No frame and no fill. What this is painted over — a trapezoid tab, a layer row — has to stay visible, or
    //    the artist cannot see which of several similarly named things is being renamed.
    ControlInteraction Interaction;

    if (!Carry.EditOpen)
    {
        Carry.EditOpen      = true;
        Carry.CaretPosition = Carry.CarryExtent;
        Interaction.EditOpened = true;
    }

    const EditProgress Advanced = AdvanceTextCarry(Carry);

    if (Advanced == EditProgress::Amended)
        Interaction.EditDeclared = true;

    if (Advanced == EditProgress::Sealed)
    {
        Carry.EditOpen         = false;
        Interaction.EditSealed = true;
    }

    if (Advanced == EditProgress::Abandoned)
    {
        Carry.EditOpen            = false;
        Interaction.EditAbandoned = true;
    }

    const PointerReading Pointer = ResolvePointer();

    Interaction.PointerOver = PointerCovers(Pointer, Area);

    // 📝 A press outside seals, matching the field entries. The rename this replaces sealed the same way.
    if (Carry.EditOpen && Pointer.PressBegan && !Interaction.PointerOver)
    {
        Carry.EditOpen         = false;
        Interaction.EditSealed = true;
    }

    PaintCaption(Area, Carry.Carried, Palette.TextPrimary, 0.0f, 0.5f, 1.0f);

    WorkspaceRectangle Caret;

    Caret.PositionX = Area.PositionX + CaretAdvance(Carry);
    Caret.PositionY = Area.PositionY + Extents.BorderThickness * 2.0f;
    Caret.Width     = Extents.BorderThickness;
    Caret.Height    = Area.Height - Extents.BorderThickness * 4.0f;

    PaintFill(Caret, Palette.SelectionMarker, 0.0f);

    WorkspaceRectangle Hairline;

    Hairline.PositionX = Area.PositionX;
    Hairline.PositionY = Area.PositionY + Area.Height - Extents.BorderThickness;
    Hairline.Width     = Area.Width;
    Hairline.Height    = Extents.BorderThickness;

    PaintFill(Hairline, Palette.SelectionMarker, 0.0f);

    return Outcome<ControlInteraction>::Deliver(Interaction);
}


//------------------------------------------------------------------------------------------------------------------------
//                                                       THE PATH ENTRY
//------------------------------------------------------------------------------------------------------------------------

Outcome<ControlInteraction> PresentPathEntry(const ThemeSpecification&  Theme,
                                             const WorkspaceRectangle&  Area,
                                             const char*                Caption,
                                             TextCarry&                 Carry,
                                             bool&                      BrowseDeclared)
{
    const LayoutExtents&  Extents = Theme.Extents;
    const ThemePalette&   Palette = Theme.Palette;
    const ControlRowSplit Split   = ResolveControlRow(Theme, Area);

    BrowseDeclared = false;

    const float CapEdge = Extents.EntryRowHeight;
    const float Gap     = Extents.PanelPadding;

    if (Split.FieldArea.Width < CapEdge + Gap + Extents.GlyphEdge + Extents.PanelPadding * 2.0f)
    {
        return Outcome<ControlInteraction>::Refuse(
            { RefusalReason::ExtentExhausted, "the field cannot carry a path and its browse cap" });
    }

    PresentControlLabel(Theme, Split.LabelArea, Caption);

    const WorkspaceRectangle Row = CentredBand(Split.FieldArea, Extents.EntryRowHeight);

    WorkspaceRectangle Frame = Row;

    Frame.Width -= CapEdge + Gap;

    const WorkspaceRectangle Cap = RightSlice(Row, CapEdge);

    PaintFill(Frame, Palette.ValueNumberSegment, Extents.EntryRounding);

    if (Carry.EditOpen)
        PaintOutline(Frame, Palette.SelectionMarker, Extents.EntryRounding, Extents.BorderThickness * 1.5f);

    WorkspaceRectangle Placement = Frame;

    Placement.PositionX += Extents.PanelPadding * 1.5f;
    Placement.Width     -= Extents.PanelPadding * 3.0f;

    ControlInteraction Interaction = AdvanceField(Theme, Frame, Placement, Carry, nullptr, nullptr);

    // -- the browse cap ----------------------------------------------------------------------------------------------
    const PointerReading     Pointer     = ResolvePointer();
    const bool               CapCovered  = PointerCovers(Pointer, Cap);
    const ControlInteraction CapPressed  = ResolvePress(Cap);

    PaintFill(Cap, CapCovered ? Palette.ControlHovered : Palette.ValueSideSegment, Extents.EntryRounding);

    // 📝 Three dots and not an ellipsis glyph, so the cap reads the same at every font the theme is resolved with.
    const float DotRadius = Extents.BorderThickness * 1.4f;
    const float DotStride = Extents.GlyphEdge * 0.36f;
    const float DotY      = Cap.PositionY + Cap.Height * 0.5f;
    const float DotX      = Cap.PositionX + Cap.Width * 0.5f;

    for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
    {
        PaintDisc(DotX + (static_cast<float>(Ordinal) - 1.0f) * DotStride, DotY, DotRadius,
                  CapCovered ? Palette.TextPrimary : Palette.TextMuted);
    }

    if (CapPressed.PointerOver)
        Interaction.PointerOver = true;

    // 📝 🔴 The cap declares an intent and opens nothing. `04`'s interchange owns the file system, and a control that
    //    reached it here would be a control holding what it presents.
    if (CapPressed.EditSealed)
    {
        BrowseDeclared         = true;
        Interaction.EditSealed = true;
    }

    return Outcome<ControlInteraction>::Deliver(Interaction);
}

}   // namespace Slate
