//============================================================================================================================================
//                                                            WORKSPACEPANEL.CPP
//============================================================================================================================================
// 🧩 The in-body panel layer — docked bands beneath, floating boxes above, and the three drags each of them carries.

#include "WorkspaceStripInternal.h"

#include <cstring>

namespace Slate
{

// 📝 The shared strip geometry is used on nearly every line below. Named once here rather than qualified at each
//    use, which is the only reason this using-declaration exists in a translation unit.
using namespace StripInterior;

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   WHAT A BOX RESOLVES TO
//------------------------------------------------------------------------------------------------------------------------

// 📝 One box by identity. The placement carries identities and never positions, so every read of a box goes through
//    this rather than through the position the placement happened to be written at.
const WorkspacePanelBox* BoxOf(const WorkspaceDocument& Standing, WorkspacePanelIdentity Subject)
{
    if (!Subject.IdentityDeclared())
        return nullptr;

    for (const WorkspacePanelBox& Box : Standing.PanelBoxes)
    {
        if (Box.SlotOccupied && Box.Identity == Subject)
            return &Box;
    }

    return nullptr;
}

// 📝 🔴 The ledger is matched by identifier and never by ordinal. A workspace rebuilds its ledger at activation and
//    the boxes on the desk outlive that rebuild, so a box holding a ledger position would present whatever panel
//    happened to land at that position the second time around.
const PanelSlot* SlotFor(const PanelIndex* Panels, const char* DeclaredIdentifier)
{
    if (Panels == nullptr || DeclaredIdentifier == nullptr || DeclaredIdentifier[0] == '\0')
        return nullptr;

    for (std::uint32_t Ordinal = 0u; Ordinal < Panels->DeclaredCount && Ordinal < PanelSlotCapacity; ++Ordinal)
    {
        const PanelSlot& Declared = Panels->DeclaredSlots[Ordinal];

        if (Declared.PanelIdentifier != nullptr && std::strcmp(Declared.PanelIdentifier, DeclaredIdentifier) == 0)
            return &Declared;
    }

    return nullptr;
}

// 📝 The header band a box carries, bounded to a third of the box so a box resized to the minimum extent still
//    presents a body rather than presenting nothing but its own caption.
WorkspaceRectangle HeaderOf(const WorkspaceRectangle& Area, float HeaderHeight)
{
    WorkspaceRectangle Header = Area;

    Header.Height = Area.Height * 0.34f < HeaderHeight ? Area.Height * 0.34f : HeaderHeight;

    return Header;
}

WorkspaceRectangle InteriorOf(const WorkspaceRectangle& Area, float HeaderHeight, float Padding)
{
    const WorkspaceRectangle Header = HeaderOf(Area, HeaderHeight);

    WorkspaceRectangle Interior;

    Interior.PositionX = Area.PositionX + Padding;
    Interior.PositionY = Area.PositionY + Header.Height;
    Interior.Width     = Area.Width  - Padding * 2.0f;
    Interior.Height    = Area.Height - Header.Height - Padding;

    if (Interior.Width  < 0.0f) Interior.Width  = 0.0f;
    if (Interior.Height < 0.0f) Interior.Height = 0.0f;

    return Interior;
}

// 📝 🔴 The edges under the pointer as one bitmask, so a corner is two edges held at once and never a fifth case.
//    Frontier spells its corners apart from its edges and the two disagree by a pixel at every corner as a result.
std::uint32_t EdgesUnder(const WorkspaceRectangle& Area, float PointerX, float PointerY)
{
    if (!RectangleCovers(Area, PointerX, PointerY))
        return 0u;

    std::uint32_t Held = 0u;

    if (PointerX - Area.PositionX                  <= PanelHandleReach) Held |= PanelEdgeLeft;
    if (Area.PositionX + Area.Width  - PointerX    <= PanelHandleReach) Held |= PanelEdgeRight;
    if (PointerY - Area.PositionY                  <= PanelHandleReach) Held |= PanelEdgeTop;
    if (Area.PositionY + Area.Height - PointerY    <= PanelHandleReach) Held |= PanelEdgeBottom;

    return Held;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       ONE PANEL BOX
//------------------------------------------------------------------------------------------------------------------------

// 📝 One implementation for a docked band and for a floating overlay. The two differ in three things — the rounding,
//    the grips they carry and whether a press raises them — and every other line of the two would otherwise be a
//    copy, which is the copy a chrome amendment lands in only one of.
void PresentOnePanelBox(const ThemeSpecification&      Theme,
                        WorkspaceSpace&                Space,
                        WorkspaceDocumentIdentity      Presented,
                        const WorkspacePanelPlacement& Placement,
                        const PanelIndex*              Panels,
                        bool                           Overlaid,
                        bool                           InputOpen,
                        DeferredIntent&                Arriving,
                        bool&                          PointerConsumed)
{
    const Outcome<const WorkspaceDocument*> Resolved = ResolveDocument(Space, Presented);

    if (!Resolved.ContentPresent)
        return;

    const WorkspacePanelBox* Box = BoxOf(*Resolved.Resolve(), Placement.Identity);

    if (Box == nullptr)
        return;

    const LayoutExtents& Extents   = Theme.Extents;
    const ThemePalette&  Palette   = Theme.Palette;
    ImDrawList*          Recording = ImGui::GetForegroundDrawList();
    const ImGuiIO&       Pointing  = ImGui::GetIO();

    const float PointerX = Pointing.MousePos.x;
    const float PointerY = Pointing.MousePos.y;

    const WorkspaceRectangle& Area = Placement.Area;

    if (Area.Width <= 1.0f || Area.Height <= 1.0f)
        return;

    // 📝 A docked band is flush with the bands beside it and takes no rounding. A rounded band leaves four wedges of
    //    desk showing at every junction, which reads as a gap the artist tries to drag something into.
    const float Rounding = Overlaid ? Extents.CornerRounding : 0.0f;

    const WorkspaceRectangle Header   = HeaderOf(Area, Extents.PanelHeaderHeight);
    const WorkspaceRectangle Interior = InteriorOf(Area, Extents.PanelHeaderHeight, Extents.PanelPadding);

    Recording->AddRectFilled(Corner(Area), Opposite(Area), Coded(Palette.PanelBackground), Rounding);
    Recording->AddRectFilled(Corner(Header), Opposite(Header), Coded(Palette.PanelHeader), Rounding,
                             ImDrawFlags_RoundCornersTop);
    Recording->AddRect(Corner(Area), Opposite(Area), Coded(Palette.PanelBorder), Rounding,
                       0, Extents.BorderThickness);

    // -- the caption and the box's own (x) --------------------------------------------------------------------------
    const ImVec2 Measured = ImGui::CalcTextSize(Box->Title);

    Recording->AddText(ImVec2(Header.PositionX + Extents.PanelPadding,
                              Header.PositionY + (Header.Height - Measured.y) * 0.5f),
                       Coded(Palette.TextPrimary), Box->Title);

    const WorkspaceRectangle Withdrawal =
        SquareAt(Header.PositionX + Header.Width - Extents.PanelPadding - Extents.GlyphEdge,
                 Header.PositionY + (Header.Height - Extents.GlyphEdge) * 0.5f,
                 Extents.GlyphEdge);

    const bool WithdrawalCovered = RectangleCovers(Withdrawal, PointerX, PointerY);

    PaintCrossStroke(Recording,
                     Withdrawal.PositionX + Withdrawal.Width  * 0.5f,
                     Withdrawal.PositionY + Withdrawal.Height * 0.5f,
                     Extents.GlyphEdge * 0.42f,
                     Coded(WithdrawalCovered ? Palette.DangerPrimary : Palette.TextMuted),
                     Extents.TabUnderline * 0.6f);

    // -- what the panel itself presents, inside the rectangle it was given ------------------------------------------
    const PanelSlot* Named = SlotFor(Panels, Box->DeclaredIdentifier);

    if (Named != nullptr && Named->Present != nullptr && Interior.Width > 1.0f && Interior.Height > 1.0f)
    {
        // 🔴 The interior is handed over as a parameter. A panel that resolved its own rectangle would paint at the
        //    depth this band carried before the drag rather than at the one it carries now.
        Named->Present(Theme, Interior, Named->PresentContext);
    }
    else if (Box->DeclaredIdentifier != nullptr && Interior.Height > 1.0f)
    {
        // 📝 ⚠️ A box whose ledger slot does not resolve prints the identifier it names rather than nothing at all,
        //    which is what makes an arrangement of panels testable before one concrete panel has been written.
        const ImVec2 Printed = ImGui::CalcTextSize(Box->DeclaredIdentifier);

        Recording->AddText(ImVec2(Interior.PositionX + (Interior.Width  - Printed.x) * 0.5f,
                                  Interior.PositionY + (Interior.Height - Printed.y) * 0.5f),
                           Coded(Palette.TextMuted), Box->DeclaredIdentifier);
    }

    // -- the grips a docked band carries, and the resize rules a floating box carries -------------------------------
    const bool DepthCovered = Placement.DepthGripDeclared
                           && RectangleCovers(Placement.DepthGrip, PointerX, PointerY);
    const bool ShareCovered = Placement.ShareGripDeclared
                           && RectangleCovers(Placement.ShareGrip, PointerX, PointerY);

    if (Placement.DepthGripDeclared)
    {
        Recording->AddRectFilled(Corner(Placement.DepthGrip), Opposite(Placement.DepthGrip),
                                 Coded(DepthCovered ? Palette.AccentSubtle : Palette.PanelBorder));
    }

    if (Placement.ShareGripDeclared)
    {
        Recording->AddRectFilled(Corner(Placement.ShareGrip), Opposite(Placement.ShareGrip),
                                 Coded(ShareCovered ? Palette.AccentSubtle : Palette.PanelBorder));
    }

    const std::uint32_t Edges = Overlaid ? EdgesUnder(Area, PointerX, PointerY) : 0u;

    if (Overlaid)
    {
        PaintGripStroke(Recording,
                        SquareAt(Area.PositionX + Area.Width  - Extents.GlyphButtonEdge,
                                 Area.PositionY + Area.Height - Extents.GlyphButtonEdge,
                                 Extents.GlyphButtonEdge),
                        Coded(Palette.TextMuted), Extents.BorderThickness);
    }

    // -- what the pointer asked of this box -------------------------------------------------------------------------
    if (!InputOpen || PointerConsumed || !ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        return;

    const bool Covered = RectangleCovers(Area, PointerX, PointerY) || DepthCovered || ShareCovered;

    if (!Covered)
        return;

    if (WithdrawalCovered)
    {
        Arriving.PanelWithdrawDeclared = true;
        Arriving.PanelWithdrawBody     = Presented;
        Arriving.PanelWithdrawSubject  = Placement.Identity;

        PointerConsumed = true;

        return;
    }

    WorkspaceDragRecord& Held = Space.Dragging;

    // 📝 The record is filled in one place for all three modes, and the mode alone says which of the fields the
    //    advance will read. Filling it per mode is where a mode ends up reading a field no branch wrote.
    Held.Origin        = WorkspaceDragOrigin::Panel;
    Held.HeldBody      = Presented;
    Held.HeldPanel     = Placement.Identity;
    Held.HeldPanelArea = Area;
    Held.HeldSide      = Box->Side;
    Held.HeldEdges     = Edges;
    Held.ShareHeld     = false;
    Held.GrabOffsetX   = PointerX - Area.PositionX;
    Held.GrabOffsetY   = PointerY - Area.PositionY;

    // 🔴 The two grips are tested before the header. A left band's depth grip runs the whole height of the box and
    //    therefore crosses the header's right end; testing the header first would make that corner un-draggable.
    if (DepthCovered || ShareCovered)
    {
        Held.Mode      = WorkspaceDragMode::PanelBandResize;
        Held.ShareHeld = ShareCovered && !DepthCovered;

        PointerConsumed = true;
    }
    else if (Overlaid && Edges != 0u)
    {
        Held.Mode = WorkspaceDragMode::PanelResize;

        PointerConsumed = true;
    }
    else if (RectangleCovers(Header, PointerX, PointerY))
    {
        Held.Mode = WorkspaceDragMode::PanelBox;

        PointerConsumed = true;
    }
    else
    {
        // 📝 A press on the body of a box is not a drag. It is consumed all the same, so it does not reach the
        //    document underneath and activate a tab the artist cannot see behind the panel.
        Held.Mode = WorkspaceDragMode::None;

        PointerConsumed = true;
    }

    if (Overlaid)
    {
        Arriving.PanelRaiseDeclared = true;
        Arriving.PanelRaiseBody     = Presented;
        Arriving.PanelRaiseSubject  = Placement.Identity;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE THREE PANEL DRAGS
//------------------------------------------------------------------------------------------------------------------------

void AdvancePanelMove(WorkspaceDragRecord&      Held,
                      WorkspacePanelBox&        Moving,
                      const WorkspaceRectangle& Body,
                      float                     PointerX,
                      float                     PointerY)
{
    // 📝 Only a floating box follows the pointer. A docked band held by its header shows a landing preview and stays
    //    where it is until the release, because a band that left its side mid-drag re-lays out everything beside it.
    if (Moving.Side == WorkspacePanelSide::Floating)
    {
        Moving.OffsetX = PointerX - Held.GrabOffsetX - Body.PositionX;
        Moving.OffsetY = PointerY - Held.GrabOffsetY - Body.PositionY;
    }

    WorkspaceRectangle Preview;

    Held.PreviewSide = ResolvePanelLanding(Body, PointerX, PointerY, Preview);
    Held.PreviewArea = Preview;
}

void AdvancePanelResize(const WorkspaceDragRecord& Held,
                        WorkspacePanelBox&         Sized,
                        const WorkspaceRectangle&  Body,
                        float                      PointerX,
                        float                      PointerY)
{
    // 📝 Resolved from the rectangle the box carried when the press landed, never from the rectangle it carried last
    //    tick. Accumulating one tick onto the next lets a bounded edge drift by the amount it was bounded by.
    WorkspaceRectangle Resized = Held.HeldPanelArea;

    if ((Held.HeldEdges & PanelEdgeLeft) != 0u)
    {
        const float Trailing = Resized.PositionX + Resized.Width;
        float       Leading  = PointerX;

        if (Trailing - Leading < PanelMinimumExtent)
            Leading = Trailing - PanelMinimumExtent;

        Resized.PositionX = Leading;
        Resized.Width     = Trailing - Leading;
    }

    if ((Held.HeldEdges & PanelEdgeRight) != 0u)
    {
        const float Asked = PointerX - Resized.PositionX;

        Resized.Width = Asked < PanelMinimumExtent ? PanelMinimumExtent : Asked;
    }

    if ((Held.HeldEdges & PanelEdgeTop) != 0u)
    {
        const float Trailing = Resized.PositionY + Resized.Height;
        float       Leading  = PointerY;

        if (Trailing - Leading < PanelMinimumExtent)
            Leading = Trailing - PanelMinimumExtent;

        Resized.PositionY = Leading;
        Resized.Height    = Trailing - Leading;
    }

    if ((Held.HeldEdges & PanelEdgeBottom) != 0u)
    {
        const float Asked = PointerY - Resized.PositionY;

        Resized.Height = Asked < PanelMinimumExtent ? PanelMinimumExtent : Asked;
    }

    Sized.OffsetX = Resized.PositionX - Body.PositionX;
    Sized.OffsetY = Resized.PositionY - Body.PositionY;
    Sized.Width   = Resized.Width;
    Sized.Height  = Resized.Height;
}

// 📝 The depth is written to every box on the side rather than to the one being dragged. The band is one band and the
//    placement takes its depth from the first box on the side, so writing one box would move the band for that box
//    alone and the artist would see a band that only follows the pointer when the topmost panel is the one grabbed.
void AdvanceBandDepth(WorkspaceDocument&        Standing,
                      WorkspacePanelSide        Side,
                      const WorkspaceRectangle& Body,
                      float                     PointerX,
                      float                     PointerY)
{
    if (Body.Width <= 1.0f || Body.Height <= 1.0f)
        return;

    float Asked = 0.28f;

    switch (Side)
    {
        case WorkspacePanelSide::Left:
            Asked = (PointerX - Body.PositionX) / Body.Width;
            break;

        case WorkspacePanelSide::Right:
            Asked = (Body.PositionX + Body.Width - PointerX) / Body.Width;
            break;

        case WorkspacePanelSide::Top:
            Asked = (PointerY - Body.PositionY) / Body.Height;
            break;

        case WorkspacePanelSide::Bottom:
            Asked = (Body.PositionY + Body.Height - PointerY) / Body.Height;
            break;

        default:
            return;
    }

    const float Bounded = Asked < PanelDockFloor ? PanelDockFloor
                                                 : (Asked > PanelDockCeiling ? PanelDockCeiling : Asked);

    for (WorkspacePanelBox& Box : Standing.PanelBoxes)
    {
        if (Box.SlotOccupied && Box.Side == Side)
            Box.DockExtent = Bounded;
    }
}

// 📝 🔴 The two shares either side of the grip are re-split and their sum is preserved, so every other panel on the
//    band keeps the extent it had. Writing one share alone changes the sum the placement normalises against, and the
//    band's other panels then move while the artist is dragging a divider that has nothing to do with them.
void AdvanceBandShare(WorkspaceDocument&            Standing,
                      const WorkspaceBodyPlacement& Placed,
                      WorkspacePanelIdentity        Subject,
                      WorkspacePanelSide            Side,
                      float                         PointerX,
                      float                         PointerY)
{
    std::size_t Leading  = Standing.PanelBoxes.size();
    std::size_t Trailing = Standing.PanelBoxes.size();

    for (std::size_t Ordinal = 0u; Ordinal < Standing.PanelBoxes.size(); ++Ordinal)
    {
        const WorkspacePanelBox& Box = Standing.PanelBoxes[Ordinal];

        if (!Box.SlotOccupied || Box.Side != Side)
            continue;

        if (Leading == Standing.PanelBoxes.size())
        {
            if (Box.Identity == Subject)
                Leading = Ordinal;

            continue;
        }

        Trailing = Ordinal;
        break;
    }

    if (Leading == Standing.PanelBoxes.size() || Trailing == Standing.PanelBoxes.size())
        return;

    const WorkspacePanelIdentity Following = Standing.PanelBoxes[Trailing].Identity;

    WorkspaceRectangle LeadingArea  = {};
    WorkspaceRectangle TrailingArea = {};

    for (std::uint32_t Ordinal = 0u; Ordinal < Placed.DockedCount; ++Ordinal)
    {
        if (Placed.Docked[Ordinal].Identity == Subject)   LeadingArea  = Placed.Docked[Ordinal].Area;
        if (Placed.Docked[Ordinal].Identity == Following) TrailingArea = Placed.Docked[Ordinal].Area;
    }

    const bool  Across = Side == WorkspacePanelSide::Left || Side == WorkspacePanelSide::Right;
    const float Origin = Across ? LeadingArea.PositionY : LeadingArea.PositionX;
    const float Span   = Across ? LeadingArea.Height + TrailingArea.Height
                                : LeadingArea.Width  + TrailingArea.Width;

    if (Span <= 1.0f)
        return;

    const float Asked   = ((Across ? PointerY : PointerX) - Origin) / Span;
    const float Bounded = Asked < 0.15f ? 0.15f : (Asked > 0.85f ? 0.85f : Asked);

    const float Combined = Standing.PanelBoxes[Leading].SlotFraction + Standing.PanelBoxes[Trailing].SlotFraction;

    Standing.PanelBoxes[Leading].SlotFraction  = Combined * Bounded;
    Standing.PanelBoxes[Trailing].SlotFraction = Combined * (1.0f - Bounded);
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PANEL LAYER
//------------------------------------------------------------------------------------------------------------------------

namespace StripInterior
{

bool BodyCarrying(const WorkspaceSpace&     Space,
                  WorkspaceDocumentIdentity Subject,
                  float                     StripHeight,
                  WorkspaceRectangle&       Resolved)
{
    if (!Subject.IdentityDeclared())
        return false;

    const std::int32_t Carrying = LocateLeafCarrying(Space, Subject);

    if (Carrying >= 0 && Space.Partitions[static_cast<std::size_t>(Carrying)].LayoutResolved)
    {
        Resolved = BodyOf(Space.Partitions[static_cast<std::size_t>(Carrying)].Area, StripHeight);

        return true;
    }

    const std::uint32_t Window = LocateWindowCarrying(Space, Subject);

    if (Window == 0u)
        return false;

    for (const WorkspaceFloatingWindow& Standing : Space.Floating)
    {
        if (Standing.Identifier == Window)
        {
            Resolved = BodyOf(AreaOf(Standing), StripHeight);

            return true;
        }
    }

    return false;
}

void PresentPanelLayer(const ThemeSpecification&  Theme,
                       WorkspaceSpace&            Space,
                       WorkspaceDocumentIdentity  Presented,
                       WorkspaceRectangle         Body,
                       const PanelIndex*          Panels,
                       DeferredIntent&            Arriving,
                       bool&                      PointerConsumed)
{
    const Outcome<const WorkspaceDocument*> Resolved = ResolveDocument(Space, Presented);

    if (!Resolved.ContentPresent || Body.Width <= 1.0f || Body.Height <= 1.0f)
        return;

    // 📝 The placement is taken by value before anything is painted. It names identities rather than positions, so a
    //    withdrawal deferred to the end of the tick cannot leave this walk holding a record nothing occupies.
    const WorkspaceBodyPlacement Placed =
        ResolveBodyPlacement(*Resolved.Resolve(), Body, Theme.Extents.GutterThickness);

    if (Placed.DockedCount == 0u && Placed.OverlaidCount == 0u)
        return;

    const ImGuiIO& Pointing = ImGui::GetIO();

    // 📝 🔴 The topmost floating box under the pointer is resolved before any of them is presented. Docked bands
    //    resolve input only when no floating box covers the pointer, because the floating run paints over them and a
    //    band that took the press would be a band the artist reached through the panel sitting on top of it.
    std::uint32_t Covering = Placed.OverlaidCount;

    for (std::uint32_t Ordinal = Placed.OverlaidCount; Ordinal > 0u; --Ordinal)
    {
        if (RectangleCovers(Placed.Overlaid[Ordinal - 1u].Area, Pointing.MousePos.x, Pointing.MousePos.y))
        {
            Covering = Ordinal - 1u;
            break;
        }
    }

    const bool DockedOpen = Covering == Placed.OverlaidCount;

    for (std::uint32_t Ordinal = 0u; Ordinal < Placed.DockedCount; ++Ordinal)
    {
        PresentOnePanelBox(Theme, Space, Presented, Placed.Docked[Ordinal], Panels,
                           false, DockedOpen, Arriving, PointerConsumed);
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < Placed.OverlaidCount; ++Ordinal)
    {
        PresentOnePanelBox(Theme, Space, Presented, Placed.Overlaid[Ordinal], Panels,
                           true, Ordinal == Covering, Arriving, PointerConsumed);
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PANEL ADVANCE
//------------------------------------------------------------------------------------------------------------------------

void AdvancePanelDrag(const ThemeSpecification& Theme,
                      WorkspaceSpace&           Space,
                      float                     PointerX,
                      float                     PointerY)
{
    WorkspaceDragRecord& Held = Space.Dragging;

    if (Held.Mode != WorkspaceDragMode::PanelBox
     && Held.Mode != WorkspaceDragMode::PanelResize
     && Held.Mode != WorkspaceDragMode::PanelBandResize)
    {
        return;
    }

    WorkspaceRectangle Body;

    // 🔴 The body is re-resolved every tick rather than carried in the record. A gutter dragged elsewhere on the desk
    //    moves the body under the panel, and a carried rectangle would place the panel against where the body was.
    if (!BodyCarrying(Space, Held.HeldBody, Theme.Extents.TabStripHeight, Body))
    {
        Held.Mode = WorkspaceDragMode::None;

        return;
    }

    const Outcome<WorkspaceDocument*> Amended = AmendDocument(Space, Held.HeldBody);

    if (!Amended.ContentPresent)
    {
        Held.Mode = WorkspaceDragMode::None;

        return;
    }

    WorkspaceDocument* Standing = Amended.Resolve();

    const Outcome<WorkspacePanelBox*> Addressed = AmendPanelBox(*Standing, Held.HeldPanel);

    if (!Addressed.ContentPresent)
    {
        Held.Mode = WorkspaceDragMode::None;

        return;
    }

    switch (Held.Mode)
    {
        case WorkspaceDragMode::PanelBox:
            AdvancePanelMove(Held, *Addressed.Resolve(), Body, PointerX, PointerY);
            break;

        case WorkspaceDragMode::PanelResize:
            AdvancePanelResize(Held, *Addressed.Resolve(), Body, PointerX, PointerY);
            break;

        case WorkspaceDragMode::PanelBandResize:
            if (Held.ShareHeld)
            {
                // 📝 The share needs the two rectangles either side of the grip, which only the placement carries.
                //    Recomputed here rather than held: it is pure geometry over the record this tick already wrote.
                AdvanceBandShare(*Standing,
                                 ResolveBodyPlacement(*Standing, Body, Theme.Extents.GutterThickness),
                                 Held.HeldPanel, Held.HeldSide, PointerX, PointerY);
            }
            else
            {
                AdvanceBandDepth(*Standing, Held.HeldSide, Body, PointerX, PointerY);
            }
            break;

        default:
            break;
    }
}

void SealPanelDrag(const ThemeSpecification& Theme, WorkspaceSpace& Space)
{
    WorkspaceDragRecord& Held = Space.Dragging;

    // 📝 Only a move lands anything. A resize and a band drag amended the record on every tick of the drag, so their
    //    release has nothing left to apply — which is also why abandoning one is not possible and never was.
    if (Held.Mode == WorkspaceDragMode::PanelBox && Held.HeldPanel.IdentityDeclared())
    {
        WorkspaceRectangle Body;

        if (BodyCarrying(Space, Held.HeldBody, Theme.Extents.TabStripHeight, Body))
        {
            const Outcome<WorkspaceDocument*> Amended = AmendDocument(Space, Held.HeldBody);

            if (Amended.ContentPresent)
            {
                const ImGuiIO& Pointing = ImGui::GetIO();

                // 📝 The release is offset by the grab, so a box that lands floating arrives where it was seen and
                //    not with its own corner under the pointer.
                DockPanelBox(*Amended.Resolve(), Held.HeldPanel, Held.PreviewSide, Body,
                             Pointing.MousePos.x - Held.GrabOffsetX,
                             Pointing.MousePos.y - Held.GrabOffsetY);
            }
        }
    }

    // 📝 The whole record is returned to rest rather than having its mode cleared. A preview left standing paints an
    //    accent wash over a landing no drag is asking for any more.
    Space.Dragging = WorkspaceDragRecord{};
}

}   // namespace StripInterior
}   // namespace Slate
