//============================================================================================================================================
//                                                            WORKSPACESTRIP.CPP
//============================================================================================================================================
// 🧩 The trapezoid strip every leaf and window carries — foreground quads, hand-rolled overlays, and inline rename.

#include "WorkspaceStripInternal.h"

#include <cstdio>
#include <cstring>
#include <vector>

namespace Slate
{

// 📝 The shared strip geometry is used on nearly every line below. Named once here rather than qualified at each
//    use, which is the only reason this using-declaration exists in a translation unit.
using namespace StripInterior;

// 📝 The geometry below is shared with `WorkspaceDrag.cpp` rather than private to this file. A floating window
//    carries the same trapezoids as a leaf, and one implementation is the whole reason the internal header exists.
namespace StripInterior
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    CODES AND STROKES
//------------------------------------------------------------------------------------------------------------------------

// 📝 The quantised code and the vendor's packed colour agree channel for channel — red in the low byte through
//    coverage in the high one — so this is a re-spelling and not a conversion. `Quantize` stays the one place a
//    theme colour becomes an integer.
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

// 📝 The sloped edges run inward at the top, so a strip of tabs reads as overlapping cards rather than as a row of
//    rectangles. The slant is taken from the theme rather than spelled here — a density change moves it with the rest.
void PaintTrapezoid(ImDrawList* Recording, const WorkspaceRectangle& Area, float Slant, ImU32 Code)
{
    const float Bounded = Slant * 2.0f < Area.Width ? Slant : Area.Width * 0.5f;

    const ImVec2 Outline[4] =
    {
        ImVec2(Area.PositionX,                          Area.PositionY + Area.Height),
        ImVec2(Area.PositionX + Bounded,                Area.PositionY),
        ImVec2(Area.PositionX + Area.Width - Bounded,   Area.PositionY),
        ImVec2(Area.PositionX + Area.Width,             Area.PositionY + Area.Height)
    };

    Recording->AddConvexPolyFilled(Outline, 4, Code);
}

// 📝 🔴 Procedural strokes and not depot glyphs, deliberately. `GlyphDepot::Resolve` delivers an opaque handle and
//    nothing outside `GlyphDepot.cpp` can turn one into something paintable, so a chrome tier declared today would
//    resolve to a handle this file cannot use. Frontier's own dock falls back to strokes for the same reason when
//    its registry is null. The three below are the whole chrome vocabulary the strip needs.
void PaintPlusStroke(ImDrawList* Recording, float CentreX, float CentreY, float Edge, ImU32 Code, float Thickness)
{
    const float Reach = Edge * 0.5f;

    Recording->AddLine(ImVec2(CentreX - Reach, CentreY), ImVec2(CentreX + Reach, CentreY), Code, Thickness);
    Recording->AddLine(ImVec2(CentreX, CentreY - Reach), ImVec2(CentreX, CentreY + Reach), Code, Thickness);
}

void PaintChevronStroke(ImDrawList* Recording, float CentreX, float CentreY, float Edge, ImU32 Code, float Thickness)
{
    const float Reach = Edge * 0.5f;
    const float Rise  = Edge * 0.28f;

    Recording->AddLine(ImVec2(CentreX - Reach, CentreY - Rise), ImVec2(CentreX, CentreY + Rise), Code, Thickness);
    Recording->AddLine(ImVec2(CentreX, CentreY + Rise), ImVec2(CentreX + Reach, CentreY - Rise), Code, Thickness);
}

void PaintCrossStroke(ImDrawList* Recording, float CentreX, float CentreY, float Edge, ImU32 Code, float Thickness)
{
    const float Reach = Edge * 0.5f;

    Recording->AddLine(ImVec2(CentreX - Reach, CentreY - Reach), ImVec2(CentreX + Reach, CentreY + Reach),
                       Code, Thickness);
    Recording->AddLine(ImVec2(CentreX + Reach, CentreY - Reach), ImVec2(CentreX - Reach, CentreY + Reach),
                       Code, Thickness);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     STRIP GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

constexpr float MinimumTabExtent = 96.0f;    // [px] - narrower than this a caption cannot be read at all
constexpr float MaximumTabExtent = 220.0f;   // [px] - wider than this one tab crowds out every other

float ResolveTabExtent(const LayoutExtents& Extents, const char* Title)
{
    const ImVec2 Measured  = ImGui::CalcTextSize(Title);
    const float  Requested = Measured.x + Extents.TabInset * 4.0f + Extents.TabSlant * 2.0f;

    return Requested < MinimumTabExtent ? MinimumTabExtent
                                        : (Requested > MaximumTabExtent ? MaximumTabExtent : Requested);
}

WorkspaceRectangle StripOf(const WorkspaceRectangle& Area, float StripHeight)
{
    WorkspaceRectangle Strip = Area;
    Strip.Height = Area.Height < StripHeight ? Area.Height : StripHeight;

    return Strip;
}

WorkspaceRectangle BodyOf(const WorkspaceRectangle& Area, float StripHeight)
{
    WorkspaceRectangle Body = Area;

    Body.PositionY += StripHeight;
    Body.Height     = Area.Height - StripHeight;

    if (Body.Height < 0.0f)
        Body.Height = 0.0f;

    return Body;
}

WorkspaceRectangle SquareAt(float PositionX, float PositionY, float Edge)
{
    WorkspaceRectangle Square;

    Square.PositionX = PositionX;
    Square.PositionY = PositionY;
    Square.Width     = Edge;
    Square.Height    = Edge;

    return Square;
}

WorkspaceRectangle AreaOf(const WorkspaceFloatingWindow& Window)
{
    WorkspaceRectangle Area;

    Area.PositionX = Window.PositionX;
    Area.PositionY = Window.PositionY;
    Area.Width     = Window.Width;
    Area.Height    = Window.Height;

    return Area;
}

// 📝 Three rules stepping in from the corner. The grip is a target and not an ornament, so it is painted at the
//    muted text colour rather than at a border colour the artist reads as an edge.
void PaintGripStroke(ImDrawList* Recording, const WorkspaceRectangle& Grip, ImU32 Code, float Thickness)
{
    for (std::uint32_t Ordinal = 1u; Ordinal <= 3u; ++Ordinal)
    {
        const float Stepped = Grip.Width * (0.25f * static_cast<float>(Ordinal));

        Recording->AddLine(ImVec2(Grip.PositionX + Grip.Width,           Grip.PositionY + Grip.Height - Stepped),
                           ImVec2(Grip.PositionX + Grip.Width - Stepped, Grip.PositionY + Grip.Height),
                           Code, Thickness);
    }
}

}   // namespace StripInterior

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE INLINE RENAME
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What one tick of an open rename produced.
enum class RenameProgress : std::uint32_t
{
    Continuing = 0u,   // [-] - the edit is still open
    Sealed     = 1u,   // [-] - accepted; the carry is the new title
    Abandoned  = 2u    // [-] - discarded; the title is untouched
};

// 📝 A caret-less accumulator rather than the vendor's own entry: the vendor's needs a window, and nothing here
//    opens one. `3`'s InlineTextEditor replaces this and the record it edits does not change when it does.
RenameProgress AdvanceRename(WorkspaceRenameRecord& Renaming)
{
    const ImGuiIO& Arriving = ImGui::GetIO();

    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
        return RenameProgress::Abandoned;

    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
        return RenameProgress::Sealed;

    if (ImGui::IsKeyPressed(ImGuiKey_Backspace, true) && Renaming.CarryExtent > 0u)
    {
        --Renaming.CarryExtent;
        Renaming.Carry[Renaming.CarryExtent] = '\0';
    }

    for (int Ordinal = 0; Ordinal < Arriving.InputQueueCharacters.Size; ++Ordinal)
    {
        const ImWchar Arrived = Arriving.InputQueueCharacters[Ordinal];

        // 📝 Only the printable single-byte range is accepted. A title is a caption and not a document, and a
        //    control character in one presents as a blank the artist cannot see or delete.
        if (Arrived < 0x20 || Arrived > 0x7E)
            continue;

        if (Renaming.CarryExtent + 1u >= WorkspaceTitleExtent)
            break;

        Renaming.Carry[Renaming.CarryExtent] = static_cast<char>(Arrived);
        ++Renaming.CarryExtent;
        Renaming.Carry[Renaming.CarryExtent] = '\0';
    }

    return RenameProgress::Continuing;
}

// 📝 DeferredIntent now lives in the internal header: a floating window's strip records the same intents a leaf's
//    does, and two copies of the record would be two places a new intent has to be added.

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    ONE STRIP AND BODY
//------------------------------------------------------------------------------------------------------------------------

namespace StripInterior
{

void PresentOccupantStrip(const ThemeSpecification&                     Theme,
                          WorkspaceSpace&                               Space,
                          const StripCarrier&                           Carrier,
                          const std::vector<WorkspaceDocumentIdentity>& Occupants,
                          WorkspaceDocumentIdentity                     Active,
                          DeferredIntent&                               Arriving,
                          bool&                                         PointerConsumed)
{
    const LayoutExtents& Extents   = Theme.Extents;
    const ThemePalette&  Palette   = Theme.Palette;
    ImDrawList*          Recording = ImGui::GetForegroundDrawList();
    const ImGuiIO&       Pointing  = ImGui::GetIO();

    const WorkspaceRectangle Strip = StripOf(Carrier.Area, Extents.TabStripHeight);
    const WorkspaceRectangle Body  = BodyOf(Carrier.Area, Extents.TabStripHeight);

    Recording->AddRectFilled(Corner(Body), Opposite(Body), Coded(Palette.PanelBackground), Extents.CornerRounding);
    Recording->AddRectFilled(Corner(Strip), Opposite(Strip), Coded(Palette.DeskBackground));

    const float PointerX = Pointing.MousePos.x;
    const float PointerY = Pointing.MousePos.y;

    // -- the (V) panel overlay button, at the far left ---------------------------------------------------------------
    const WorkspaceRectangle PanelButton =
        SquareAt(Strip.PositionX + Extents.TabInset,
                 Strip.PositionY + (Strip.Height - Extents.GlyphButtonEdge) * 0.5f,
                 Extents.GlyphButtonEdge);

    const bool PanelButtonCovered = RectangleCovers(PanelButton, PointerX, PointerY);

    if (PanelButtonCovered)
    {
        Recording->AddRectFilled(Corner(PanelButton), Opposite(PanelButton), Coded(Palette.ControlHovered),
                                 Extents.CornerRounding);
    }

    PaintChevronStroke(Recording,
                       PanelButton.PositionX + PanelButton.Width * 0.5f,
                       PanelButton.PositionY + PanelButton.Height * 0.5f,
                       Extents.GlyphEdge * 0.7f,
                       Coded(Palette.TextMuted),
                       Extents.TabUnderline * 0.75f);

    // -- the (+) minting button, at the far right --------------------------------------------------------------------
    const WorkspaceRectangle MintButton =
        SquareAt(Strip.PositionX + Strip.Width - Extents.TabInset - Extents.GlyphButtonEdge,
                 Strip.PositionY + (Strip.Height - Extents.GlyphButtonEdge) * 0.5f,
                 Extents.GlyphButtonEdge);

    const bool MintButtonCovered = RectangleCovers(MintButton, PointerX, PointerY);

    if (MintButtonCovered)
    {
        Recording->AddRectFilled(Corner(MintButton), Opposite(MintButton), Coded(Palette.ControlHovered),
                                 Extents.CornerRounding);
    }

    PaintPlusStroke(Recording,
                    MintButton.PositionX + MintButton.Width * 0.5f,
                    MintButton.PositionY + MintButton.Height * 0.5f,
                    Extents.GlyphEdge * 0.7f,
                    Coded(Palette.TextMuted),
                    Extents.TabUnderline * 0.75f);

    // -- the trapezoids between them ---------------------------------------------------------------------------------
    float Travelled = PanelButton.PositionX + PanelButton.Width + Extents.TabInset;

    const float TabCeiling = MintButton.PositionX - Extents.TabInset;

    Recording->PushClipRect(ImVec2(Travelled, Strip.PositionY), ImVec2(TabCeiling, Strip.PositionY + Strip.Height),
                            true);

    for (std::size_t Position = 0u; Position < Occupants.size(); ++Position)
    {
        const WorkspaceDocumentIdentity Occupant = Occupants[Position];

        const Outcome<const WorkspaceDocument*> Resolved = ResolveDocument(Space, Occupant);

        if (!Resolved.ContentPresent)
            continue;

        const WorkspaceDocument* Standing = Resolved.Resolve();

        const bool  Renaming   = Space.Renaming.RenameOpen && Space.Renaming.Subject == Occupant;
        const char* Carried    = Renaming ? Space.Renaming.Carry : Standing->Title;
        const float TabExtent  = ResolveTabExtent(Extents, Carried);

        WorkspaceRectangle Tab;
        Tab.PositionX = Travelled;
        Tab.PositionY = Strip.PositionY;
        Tab.Width     = TabExtent;
        Tab.Height    = Strip.Height;

        const bool Presented = Active == Occupant;
        const bool Covered   = RectangleCovers(Tab, PointerX, PointerY) && PointerX < TabCeiling;
        const bool HeldNow   = Space.Dragging.Mode == WorkspaceDragMode::Reorder
                            && Space.Dragging.HeldDocument == Occupant;

        // 📝 A tab being reordered is named by the subtle accent while it slides, so the artist can see which of
        //    two adjacent tabs is the one following the pointer.
        const ThemeColour Face = HeldNow    ? Palette.AccentSubtle
                               : (Presented ? Palette.ControlActive
                                            : (Covered ? Palette.ControlHovered : Palette.PanelHeader));

        PaintTrapezoid(Recording, Tab, Extents.TabSlant, Coded(Face));

        // 📝 The active tab is named by a 2 px underline rather than by a brighter face, which is what the
        //    reference does: a face bright enough to read as active at a glance also reads as a hovered tab.
        if (Presented)
        {
            Recording->AddRectFilled(
                ImVec2(Tab.PositionX + Extents.TabSlant, Tab.PositionY + Tab.Height - Extents.TabUnderline),
                ImVec2(Tab.PositionX + Tab.Width - Extents.TabSlant, Tab.PositionY + Tab.Height),
                Coded(Palette.AccentPrimary));
        }

        const ImVec2 Measured = ImGui::CalcTextSize(Carried);
        const ImVec2 Caption  = ImVec2(Tab.PositionX + Extents.TabSlant + Extents.TabInset,
                                       Tab.PositionY + (Tab.Height - Measured.y) * 0.5f);

        Recording->AddText(Caption, Coded(Presented ? Palette.TextPrimary : Palette.TextMuted), Carried);

        if (Renaming)
        {
            // 📝 The caret is a rule at the end of the carry rather than a blinking cell. Nothing here holds a
            //    duration, and a caret that blinks would need one that the desk would then have to carry.
            Recording->AddLine(ImVec2(Caption.x + Measured.x + 1.0f, Caption.y),
                               ImVec2(Caption.x + Measured.x + 1.0f, Caption.y + Measured.y),
                               Coded(Palette.AccentPrimary), Extents.TabUnderline * 0.5f);
        }

        // -- the tab's own (x) -------------------------------------------------------------------------------------
        const WorkspaceRectangle Withdrawal =
            SquareAt(Tab.PositionX + Tab.Width - Extents.TabSlant - Extents.GlyphEdge,
                     Tab.PositionY + (Tab.Height - Extents.GlyphEdge) * 0.5f,
                     Extents.GlyphEdge);

        const bool WithdrawalCovered = RectangleCovers(Withdrawal, PointerX, PointerY);

        if (Covered || Presented)
        {
            PaintCrossStroke(Recording,
                             Withdrawal.PositionX + Withdrawal.Width * 0.5f,
                             Withdrawal.PositionY + Withdrawal.Height * 0.5f,
                             Extents.GlyphEdge * 0.42f,
                             Coded(WithdrawalCovered ? Palette.DangerPrimary : Palette.TextMuted),
                             Extents.TabUnderline * 0.6f);
        }

        // -- a held tab crossing this one declares the swap ----------------------------------------------------------
        if (Space.Dragging.Mode == WorkspaceDragMode::Reorder && Covered && !HeldNow
         && Space.Dragging.HeldDocument.IdentityDeclared())
        {
            Arriving.ReorderDeclared = true;
            Arriving.ReorderSubject  = Space.Dragging.HeldDocument;
            Arriving.ReorderLink     = Carrier.Link;
            Arriving.ReorderWindow   = Carrier.Window;
            Arriving.ReorderPosition = static_cast<std::uint32_t>(Position);
        }

        // -- what the pointer asked of this tab --------------------------------------------------------------------
        if (!PointerConsumed && Covered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            PointerConsumed = true;

            if (WithdrawalCovered)
            {
                Arriving.WithdrawDeclared = true;
                Arriving.WithdrawSubject  = Occupant;
            }
            else
            {
                // 📝 🔴 The click activates. The press is only recorded here — `2d` turns it into a tear once the
                //    pointer travels past TearThreshold while still held, so a plain click never tears a tab out.
                Arriving.ActivateDeclared = true;
                Arriving.ActivateSubject  = Occupant;
                Arriving.ActivateLink     = Carrier.Link;
                Arriving.ActivateWindow   = Carrier.Window;

                Space.Dragging.PendingDocument = Occupant;
                Space.Dragging.PendingPressX   = PointerX;
                Space.Dragging.PendingPressY   = PointerY;
                Space.Dragging.PendingTabLeft  = Tab.PositionX;
            }
        }

        if (!PointerConsumed && Covered && !WithdrawalCovered
         && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            PointerConsumed = true;

            Space.Renaming.RenameOpen  = true;
            Space.Renaming.Subject     = Occupant;
            Space.Renaming.CarryExtent = static_cast<std::uint32_t>(std::strlen(Standing->Title));

            std::snprintf(Space.Renaming.Carry, WorkspaceTitleExtent, "%s", Standing->Title);
        }

        Travelled += TabExtent + Extents.TabInset;

        if (Travelled > TabCeiling)
            break;
    }

    Recording->PopClipRect();

    // -- an unfilled body prints what it is for ----------------------------------------------------------------------
    if (Occupants.empty())
    {
        const char*  Unfilled = "empty";
        const ImVec2 Measured = ImGui::CalcTextSize(Unfilled);

        Recording->AddText(ImVec2(Body.PositionX + (Body.Width - Measured.x) * 0.5f,
                                  Body.PositionY + (Body.Height - Measured.y) * 0.5f),
                           Coded(Palette.TextMuted), Unfilled);
    }

    // -- the two overlays open from these buttons ----------------------------------------------------------------------
    if (!PointerConsumed && MintButtonCovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        PointerConsumed = true;

        Space.MintingOverlay.OverlayOpen   = true;
        Space.MintingOverlay.OpenedTick    = Space.PresentedTicks;
        Space.MintingOverlay.AnchorX       = MintButton.PositionX;
        Space.MintingOverlay.AnchorY       = MintButton.PositionY + MintButton.Height;
        Space.MintingOverlay.TargetLink    = Carrier.Link;
        Space.MintingOverlay.TargetWindow  = Carrier.Window;
    }

    if (!PointerConsumed && PanelButtonCovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        PointerConsumed = true;

        Space.PanelOverlay.OverlayOpen    = true;
        Space.PanelOverlay.OpenedTick     = Space.PresentedTicks;
        Space.PanelOverlay.AnchorX        = PanelButton.PositionX;
        Space.PanelOverlay.AnchorY        = PanelButton.PositionY + PanelButton.Height;
        Space.PanelOverlay.TargetLink     = Carrier.Link;
        Space.PanelOverlay.TargetDocument = Active;
    }
}

}   // namespace StripInterior

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE GUTTERS
//------------------------------------------------------------------------------------------------------------------------

// 📝 The gutter is painted from the division's own cached rectangle, which `ResolveLayout` wrote this tick. Painting
//    it from the two halves instead would leave a hairline wherever a ratio landed off a whole pixel.
void PaintGutters(const ThemeSpecification& Theme, WorkspaceSpace& Space, bool& PointerConsumed)
{
    ImDrawList*    Recording = ImGui::GetForegroundDrawList();
    const ImGuiIO& Pointing  = ImGui::GetIO();

    std::int32_t Pressed = -1;

    Traverse(Space.Partitions, Space.RootLink,
             [&](std::int32_t Link, const WorkspacePartition<WorkspaceDocumentIdentity>& Standing)
             {
                 if (Standing.LeafDeclared || !Standing.LayoutResolved)
                     return;

                 const bool Covered =
                     RectangleCovers(Standing.Gutter, Pointing.MousePos.x, Pointing.MousePos.y);

                 Recording->AddRectFilled(Corner(Standing.Gutter), Opposite(Standing.Gutter),
                                          Coded(Covered ? Theme.Palette.AccentSubtle : Theme.Palette.PanelBorder));

                 if (Covered && !PointerConsumed && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                     Pressed = Link;
             });

    // 📝 Recorded after the traversal rather than inside it. The traversal is walking the pool by reference and
    //    the record it writes into is the desk's own, which is the one thing the walk must not see change.
    if (Pressed >= 0)
    {
        Space.Dragging.Mode     = WorkspaceDragMode::Partition;
        Space.Dragging.HeldLink = Pressed;

        PointerConsumed = true;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE OVERLAYS
//------------------------------------------------------------------------------------------------------------------------

// 📝 One overlay is a stack of rows on the foreground list, dismissed by any press that lands outside it — except on
//    the tick it opened, whose press is the one that opened it.
void PresentMintingOverlay(const ThemeSpecification& Theme,
                           WorkspaceSpace&           Space,
                           DeferredIntent&           Arriving,
                           bool&                     PointerConsumed)
{
    if (!Space.MintingOverlay.OverlayOpen)
        return;

    const LayoutExtents& Extents   = Theme.Extents;
    const ThemePalette&  Palette   = Theme.Palette;
    ImDrawList*          Recording = ImGui::GetForegroundDrawList();
    const ImGuiIO&       Pointing  = ImGui::GetIO();

    float Widest = 0.0f;

    for (const WorkspaceDocumentSpecification& Offered : Space.Catalogue)
    {
        const ImVec2 Measured = ImGui::CalcTextSize(Offered.Label);
        Widest = Measured.x > Widest ? Measured.x : Widest;
    }

    WorkspaceRectangle Overlay;
    Overlay.PositionX = Space.MintingOverlay.AnchorX;
    Overlay.PositionY = Space.MintingOverlay.AnchorY;
    Overlay.Width     = Widest + Extents.PanelPadding * 4.0f;
    Overlay.Height    = Extents.OverlayRowHeight * static_cast<float>(Space.Catalogue.size())
                      + Extents.PanelPadding * 2.0f;

    // 📝 The overlay is nudged back inside the display rather than being clipped at its edge. An entry the artist
    //    cannot reach is worse than one that opened a few pixels from where the button sits.
    const ImVec2 Displayed = ImGui::GetIO().DisplaySize;

    if (Overlay.PositionX + Overlay.Width > Displayed.x)
        Overlay.PositionX = Displayed.x - Overlay.Width;

    Recording->AddRectFilled(Corner(Overlay), Opposite(Overlay), Coded(Palette.PanelBackground),
                             Extents.CornerRounding);
    Recording->AddRect(Corner(Overlay), Opposite(Overlay), Coded(Palette.PanelBorder), Extents.CornerRounding,
                       0, Extents.BorderThickness);

    float Travelled = Overlay.PositionY + Extents.PanelPadding;

    for (std::size_t Ordinal = 0u; Ordinal < Space.Catalogue.size(); ++Ordinal)
    {
        WorkspaceRectangle Row;
        Row.PositionX = Overlay.PositionX + Extents.PanelPadding;
        Row.PositionY = Travelled;
        Row.Width     = Overlay.Width - Extents.PanelPadding * 2.0f;
        Row.Height    = Extents.OverlayRowHeight;

        const bool Covered = RectangleCovers(Row, Pointing.MousePos.x, Pointing.MousePos.y);

        if (Covered)
        {
            Recording->AddRectFilled(Corner(Row), Opposite(Row), Coded(Palette.RowHovered),
                                     Extents.CornerRounding * 0.5f);
        }

        const ImVec2 Measured = ImGui::CalcTextSize(Space.Catalogue[Ordinal].Label);

        Recording->AddText(ImVec2(Row.PositionX + Extents.PanelPadding,
                                  Row.PositionY + (Row.Height - Measured.y) * 0.5f),
                           Coded(Palette.TextPrimary), Space.Catalogue[Ordinal].Label);

        if (Covered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            PointerConsumed = true;

            Arriving.MintDeclared = true;
            Arriving.MintOrdinal  = static_cast<std::uint32_t>(Ordinal);
            Arriving.MintLink     = Space.MintingOverlay.TargetLink;

            Space.MintingOverlay.OverlayOpen = false;
        }

        Travelled += Extents.OverlayRowHeight;
    }

    const bool Outside = !RectangleCovers(Overlay, Pointing.MousePos.x, Pointing.MousePos.y);

    if (Outside && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
     && Space.MintingOverlay.OpenedTick != Space.PresentedTicks)
    {
        Space.MintingOverlay.OverlayOpen = false;
    }

    if (!Outside)
        PointerConsumed = true;
}

// 📝 🚧 The (V) overlay offers one row, and choosing it declares a panel box into the active document. `2e` is what
//    presents the box; until then the declaration is visible as the document's PanelBoxes extent alone.
void PresentPanelOverlay(const ThemeSpecification& Theme, WorkspaceSpace& Space, bool& PointerConsumed)
{
    if (!Space.PanelOverlay.OverlayOpen)
        return;

    const LayoutExtents& Extents   = Theme.Extents;
    const ThemePalette&  Palette   = Theme.Palette;
    ImDrawList*          Recording = ImGui::GetForegroundDrawList();
    const ImGuiIO&       Pointing  = ImGui::GetIO();

    const char* Offered = "Panel box";

    WorkspaceRectangle Overlay;
    Overlay.PositionX = Space.PanelOverlay.AnchorX;
    Overlay.PositionY = Space.PanelOverlay.AnchorY;
    Overlay.Width     = ImGui::CalcTextSize(Offered).x + Extents.PanelPadding * 4.0f;
    Overlay.Height    = Extents.OverlayRowHeight + Extents.PanelPadding * 2.0f;

    Recording->AddRectFilled(Corner(Overlay), Opposite(Overlay), Coded(Palette.PanelBackground),
                             Extents.CornerRounding);
    Recording->AddRect(Corner(Overlay), Opposite(Overlay), Coded(Palette.PanelBorder), Extents.CornerRounding,
                       0, Extents.BorderThickness);

    WorkspaceRectangle Row;
    Row.PositionX = Overlay.PositionX + Extents.PanelPadding;
    Row.PositionY = Overlay.PositionY + Extents.PanelPadding;
    Row.Width     = Overlay.Width - Extents.PanelPadding * 2.0f;
    Row.Height    = Extents.OverlayRowHeight;

    const bool Covered = RectangleCovers(Row, Pointing.MousePos.x, Pointing.MousePos.y);

    if (Covered)
    {
        Recording->AddRectFilled(Corner(Row), Opposite(Row), Coded(Palette.RowHovered),
                                 Extents.CornerRounding * 0.5f);
    }

    const ImVec2 Measured = ImGui::CalcTextSize(Offered);

    Recording->AddText(ImVec2(Row.PositionX + Extents.PanelPadding,
                              Row.PositionY + (Row.Height - Measured.y) * 0.5f),
                       Coded(Palette.TextPrimary), Offered);

    if (Covered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        PointerConsumed = true;

        const Outcome<WorkspaceDocument*> Amended = AmendDocument(Space, Space.PanelOverlay.TargetDocument);

        if (Amended.ContentPresent)
        {
            WorkspaceDocument* Standing = Amended.Resolve();

            ++Standing->MintedPanels;

            WorkspacePanelBox Declared;
            Declared.Identity.SlotOrdinal    = Standing->MintedPanels;
            Declared.Identity.SlotGeneration = 1u;
            Declared.OffsetX                 = Extents.PanelPadding * 4.0f;
            Declared.OffsetY                 = Extents.PanelPadding * 4.0f;

            std::snprintf(Declared.Title, WorkspaceTitleExtent, "Panel %u", Standing->MintedPanels);

            Standing->PanelBoxes.push_back(Declared);
        }

        Space.PanelOverlay.OverlayOpen = false;
    }

    const bool Outside = !RectangleCovers(Overlay, Pointing.MousePos.x, Pointing.MousePos.y);

    if (Outside && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
     && Space.PanelOverlay.OpenedTick != Space.PresentedTicks)
    {
        Space.PanelOverlay.OverlayOpen = false;
    }

    if (!Outside)
        PointerConsumed = true;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

void PresentWorkspaceSpace(const ThemeSpecification& Theme, WorkspaceSpace& Space, WorkspaceRectangle DeskArea)
{
    ++Space.PresentedTicks;

    // 🔴 Layout first, before a single pointer test. A gutter dragged against last tick's rectangles moves to where
    //    the pointer was rather than to where it is, and the drag reads as sticky.
    ResolveSpaceLayout(Space, DeskArea, Theme.Extents.GutterThickness);

    ImDrawList* Recording = ImGui::GetForegroundDrawList();

    Recording->AddRectFilled(Corner(DeskArea), Opposite(DeskArea), Coded(Theme.Palette.DeskBackground));

    DeferredIntent Arriving;
    bool           PointerConsumed = false;

    // 🔴 The drag advances before anything is painted. A landing applied here re-divides the desk and re-resolves
    //    the layout, so every rectangle the rest of the tick paints and tests against is the one that now holds.
    AdvanceWorkspaceDrag(Theme, Space, DeskArea, PointerConsumed);

    // 📝 The leaves are collected before any of them is presented. Presenting inside the traversal would run the
    //    pool's own recursion while an intent applied against it could grow the pool.
    std::vector<std::int32_t> Standing;

    Traverse(Space.Partitions, Space.RootLink,
             [&Standing](std::int32_t Link, const WorkspacePartition<WorkspaceDocumentIdentity>& Visiting)
             {
                 if (Visiting.LeafDeclared && Visiting.LayoutResolved)
                     Standing.push_back(Link);
             });

    // 📝 The overlays resolve their input first because they are the topmost thing painted. A strip that consumed
    //    the press first would activate a tab underneath the overlay the artist was aiming at.
    PresentMintingOverlay(Theme, Space, Arriving, PointerConsumed);
    PresentPanelOverlay(Theme, Space, PointerConsumed);

    // 📝 🔴 A leaf under a floating window resolves no input. The windows are painted after the leaves so they sit
    //    above them, so the leaves cannot simply be told the pointer was consumed — they are told separately, by a
    //    coverage test taken before either is presented.
    const bool Occluded = LocateWindowCovering(Space, ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y) != 0u;

    for (const std::int32_t Link : Standing)
    {
        StripCarrier Carrier;

        Carrier.Area = Space.Partitions[static_cast<std::size_t>(Link)].Area;
        Carrier.Link = Link;

        bool Blocked = PointerConsumed || Occluded;

        PresentOccupantStrip(Theme, Space, Carrier,
                             Space.Partitions[static_cast<std::size_t>(Link)].Occupants,
                             Space.Partitions[static_cast<std::size_t>(Link)].ActiveOccupant,
                             Arriving, Blocked);

        if (Blocked && !Occluded)
            PointerConsumed = true;
    }

    PresentFloatingWindows(Theme, Space, Arriving, PointerConsumed);

    PaintGutters(Theme, Space, PointerConsumed);

    // -- the open rename, sealed or abandoned ---------------------------------------------------------------------------
    if (Space.Renaming.RenameOpen)
    {
        const RenameProgress Progressed = AdvanceRename(Space.Renaming);

        if (Progressed == RenameProgress::Sealed)
        {
            const Outcome<WorkspaceDocument*> Amended = AmendDocument(Space, Space.Renaming.Subject);

            // 📝 An empty carry is an abandonment and not a title. A tab with no caption cannot be aimed at again.
            if (Amended.ContentPresent && Space.Renaming.CarryExtent > 0u)
                std::snprintf(Amended.Resolve()->Title, WorkspaceTitleExtent, "%s", Space.Renaming.Carry);

            Space.Renaming = WorkspaceRenameRecord{};
        }
        else if (Progressed == RenameProgress::Abandoned)
        {
            Space.Renaming = WorkspaceRenameRecord{};
        }
    }

    // -- the structural amendments, applied once the traversal is over ----------------------------------------------------
    if (Arriving.ActivateDeclared && Arriving.ActivateLink >= 0)
    {
        WorkspacePartition<WorkspaceDocumentIdentity>& Leaf =
            Space.Partitions[static_cast<std::size_t>(Arriving.ActivateLink)];

        Leaf.ActiveOccupant = Arriving.ActivateSubject;

        // 📝 An activation closes an open rename on a different document rather than leaving two carets standing.
        if (Space.Renaming.RenameOpen && Space.Renaming.Subject != Arriving.ActivateSubject)
            Space.Renaming = WorkspaceRenameRecord{};
    }
    else if (Arriving.ActivateDeclared && Arriving.ActivateWindow != 0u)
    {
        const Outcome<WorkspaceFloatingWindow*> Amended = AmendFloatingWindow(Space, Arriving.ActivateWindow);

        if (Amended.ContentPresent)
            Amended.Resolve()->ActiveDocument = Arriving.ActivateSubject;
    }

    if (Arriving.ReorderDeclared)
    {
        ReorderOccupant(Space, Arriving.ReorderSubject, Arriving.ReorderLink, Arriving.ReorderWindow,
                        Arriving.ReorderPosition);
    }

    // 📝 The raise is a rotation of the list and not a re-mint. A window raised by being rebuilt would lose the
    //    ordering of the documents inside it, which is the ordering its own strip presents.
    if (Arriving.RaiseDeclared)
    {
        for (std::size_t Ordinal = 0u; Ordinal < Space.Floating.size(); ++Ordinal)
        {
            if (Space.Floating[Ordinal].Identifier != Arriving.RaiseWindow)
                continue;

            const WorkspaceFloatingWindow Raised = Space.Floating[Ordinal];

            Space.Floating.erase(Space.Floating.begin() + static_cast<std::ptrdiff_t>(Ordinal));
            Space.Floating.push_back(Raised);
            break;
        }
    }

    if (Arriving.WithdrawDeclared)
        WithdrawDocument(Space, Arriving.WithdrawSubject);

    if (Arriving.MintDeclared)
        DeclareDocument(Space, Arriving.MintOrdinal, Arriving.MintLink);

    // 🔴 The preview is painted last, above every strip and window. Painted with the leaves it would be occluded
    //    by the very floating window the artist is dragging, which is the one thing that must stay visible.
    PaintDragPreview(Theme, Space);

    // 📝 A press released anywhere clears the pending press. `2d` reads it while the button is still held and turns
    //    it into a tear past the threshold; leaving it standing after release would tear on the next press instead.
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        Space.Dragging.PendingDocument = WorkspaceDocumentIdentity{};
}

}   // namespace Slate
