//============================================================================================================================================
//                                                            WORKSPACEDRAG.CPP
//============================================================================================================================================
// 🧩 The one drag in flight — the press that becomes a tear, the window that follows the pointer, and the landing on release.

#include "WorkspaceStripInternal.h"

#include <cmath>

namespace Slate
{

// 📝 The shared strip geometry is used on nearly every line below. Named once here rather than qualified at each
//    use, which is the only reason this using-declaration exists in a translation unit.
using namespace StripInterior;

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE HELD RECTANGLE
//------------------------------------------------------------------------------------------------------------------------

constexpr float ResizeGripEdge = 16.0f;   // [px] - the square at a window's bottom-right corner

WorkspaceRectangle GripOf(const WorkspaceRectangle& Area)
{
    return SquareAt(Area.PositionX + Area.Width  - ResizeGripEdge,
                    Area.PositionY + Area.Height - ResizeGripEdge,
                    ResizeGripEdge);
}

// 📝 The strip of whatever carries a document, so a press can be told apart from a tear: travel that stays inside
//    the strip it began in is a reorder, and travel that leaves it is a tear.
bool CarrierStripOf(const WorkspaceSpace&     Space,
                    WorkspaceDocumentIdentity Subject,
                    float                     StripHeight,
                    WorkspaceRectangle&       Resolved)
{
    const std::int32_t Carrying = LocateLeafCarrying(Space, Subject);

    if (Carrying >= 0 && Space.Partitions[static_cast<std::size_t>(Carrying)].LayoutResolved)
    {
        Resolved = StripOf(Space.Partitions[static_cast<std::size_t>(Carrying)].Area, StripHeight);

        return true;
    }

    const std::uint32_t Window = LocateWindowCarrying(Space, Subject);

    for (const WorkspaceFloatingWindow& Standing : Space.Floating)
    {
        if (Standing.Identifier == Window && Window != 0u)
        {
            Resolved = StripOf(AreaOf(Standing), StripHeight);

            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE MODES
//------------------------------------------------------------------------------------------------------------------------

// 📝 🔴 Frontier's threshold, ported as summed travel rather than as distance. A click that shifts a pixel in each
//    axis while the button goes down must activate the tab, not tear it out of the desk.
bool TravelledPastThreshold(const WorkspaceDragRecord& Held, float PointerX, float PointerY)
{
    return std::fabs(PointerX - Held.PendingPressX) + std::fabs(PointerY - Held.PendingPressY) > TearThreshold;
}

void OpenTearOrReorder(const ThemeSpecification& Theme,
                       WorkspaceSpace&           Space,
                       WorkspaceRectangle        DeskArea,
                       float                     PointerX,
                       float                     PointerY)
{
    WorkspaceDragRecord& Held = Space.Dragging;

    WorkspaceRectangle Origin;

    const bool OriginKnown = CarrierStripOf(Space, Held.PendingDocument, Theme.Extents.TabStripHeight, Origin);

    if (OriginKnown && RectangleCovers(Origin, PointerX, PointerY))
    {
        Held.Mode         = WorkspaceDragMode::Reorder;
        Held.Origin       = WorkspaceDragOrigin::Tab;
        Held.HeldDocument = Held.PendingDocument;
        Held.HeldLink     = LocateLeafCarrying(Space, Held.PendingDocument);
        Held.HeldWindow   = LocateWindowCarrying(Space, Held.PendingDocument);
    }
    else
    {
        // 📝 The grab offset is measured from where the tab sat when it was pressed, so the torn window arrives
        //    under the pointer at the same place on the caption the artist grabbed. Tearing to the pointer's
        //    own position instead makes every tear jump the window a tab-width to the left.
        const float GrabX = Held.PendingPressX - Held.PendingTabLeft;
        const float GrabY = Theme.Extents.TabStripHeight * 0.5f;

        const Outcome<std::uint32_t> Torn =
            TearDocument(Space, Held.PendingDocument, PointerX - GrabX, PointerY - GrabY);

        if (Torn.ContentPresent)
        {
            Held.Mode         = WorkspaceDragMode::Window;
            Held.Origin       = WorkspaceDragOrigin::Tab;
            Held.HeldDocument = Held.PendingDocument;
            Held.HeldWindow   = Torn.Resolve();
            Held.GrabOffsetX  = GrabX;
            Held.GrabOffsetY  = GrabY;

            // 🔴 The tear reclaimed a leaf, so every rectangle resolved earlier this tick is stale. Re-resolved
            //    here rather than next tick: a pointer test against the old rectangles would find the leaf that
            //    has just left the desk.
            ResolveSpaceLayout(Space, DeskArea, Theme.Extents.GutterThickness);
        }
    }

    Held.PendingDocument = WorkspaceDocumentIdentity{};
}

// 📝 🔴 A reorder promotes to a tear the moment the pointer leaves the strip it opened in, and this branch is what
//    makes a tab tearable at all. The threshold is six pixels and the press lands on a trapezoid, so six pixels of
//    travel from there is still inside the same strip on nearly every drag — a reorder that could not promote is a
//    desk whose tabs slide along their own row and can never be pulled out of it, which is exactly what the artist
//    meets as "the tab will not come off".
// 📝 A lone document in a floating window is **moved with its window** rather than torn into a second one. Tearing
//    it would release the window it is the only occupant of and mint an identical one in its place, and the artist
//    sees that as the window blinking out and back a tab-width away.
void AdvanceReorderDrag(const ThemeSpecification& Theme,
                        WorkspaceSpace&           Space,
                        WorkspaceRectangle        DeskArea,
                        float                     PointerX,
                        float                     PointerY)
{
    WorkspaceDragRecord& Held = Space.Dragging;

    WorkspaceRectangle Origin;

    if (!CarrierStripOf(Space, Held.HeldDocument, Theme.Extents.TabStripHeight, Origin))
        return;

    if (RectangleCovers(Origin, PointerX, PointerY))
        return;

    const std::uint32_t Carrying = LocateWindowCarrying(Space, Held.HeldDocument);

    for (const WorkspaceFloatingWindow& Standing : Space.Floating)
    {
        if (Standing.Identifier != Carrying || Carrying == 0u || Standing.Documents.size() != 1u)
            continue;

        Held.Mode        = WorkspaceDragMode::Window;
        Held.HeldWindow  = Carrying;
        Held.HeldLink    = -1;
        Held.GrabOffsetX = PointerX - Standing.PositionX;
        Held.GrabOffsetY = PointerY - Standing.PositionY;

        return;
    }

    // 📝 The grab is measured from where the tab sat when it was pressed, exactly as the opening tear measures it,
    //    so a promotion and a tear place the arriving window under the pointer at the same point on its caption.
    const float GrabX = Held.PendingPressX - Held.PendingTabLeft;
    const float GrabY = Theme.Extents.TabStripHeight * 0.5f;

    const Outcome<std::uint32_t> Torn =
        TearDocument(Space, Held.HeldDocument, PointerX - GrabX, PointerY - GrabY);

    if (!Torn.ContentPresent)
        return;

    Held.Mode        = WorkspaceDragMode::Window;
    Held.HeldWindow  = Torn.Resolve();
    Held.HeldLink    = -1;
    Held.GrabOffsetX = GrabX;
    Held.GrabOffsetY = GrabY;

    // 🔴 The tear reclaimed a leaf, so every rectangle resolved earlier this tick names a leaf that has left the
    //    desk. Re-resolved here rather than next tick, for the same reason the opening tear re-resolves.
    ResolveSpaceLayout(Space, DeskArea, Theme.Extents.GutterThickness);
}

void AdvanceWindowDrag(const ThemeSpecification& Theme,
                       WorkspaceSpace&           Space,
                       float                     PointerX,
                       float                     PointerY)
{
    WorkspaceDragRecord& Held = Space.Dragging;

    const Outcome<WorkspaceFloatingWindow*> Standing = AmendFloatingWindow(Space, Held.HeldWindow);

    if (!Standing.ContentPresent)
    {
        Held.Mode = WorkspaceDragMode::None;

        return;
    }

    WorkspaceFloatingWindow* Moving = Standing.Resolve();

    Moving->PositionX = PointerX - Held.GrabOffsetX;
    Moving->PositionY = PointerY - Held.GrabOffsetY;

    // 📝 A window moved by its own bar while carrying several documents resolves no landing at all. Only a single
    //    document can be docked, and docking one of a stack would silently leave the rest floating.
    if (!Held.HeldDocument.IdentityDeclared())
        return;

    const WorkspaceDropLanding Landing =
        ResolveDropLanding(Space, Held.Origin, PointerX, PointerY, Theme.Extents.TabStripHeight, Held.HeldWindow);

    Held.PreviewZone   = Landing.Zone;
    Held.PreviewLink   = Landing.Link;
    Held.PreviewWindow = Landing.Window;
    Held.PreviewArea   = Landing.PreviewArea;
}

void AdvanceResizeDrag(WorkspaceSpace& Space, float PointerX, float PointerY)
{
    const Outcome<WorkspaceFloatingWindow*> Standing = AmendFloatingWindow(Space, Space.Dragging.HeldWindow);

    if (!Standing.ContentPresent)
    {
        Space.Dragging.Mode = WorkspaceDragMode::None;

        return;
    }

    WorkspaceFloatingWindow* Sized = Standing.Resolve();

    const float Width  = PointerX - Sized->PositionX;
    const float Height = PointerY - Sized->PositionY;

    Sized->Width  = Width  < WindowMinimumExtent ? WindowMinimumExtent : Width;
    Sized->Height = Height < WindowMinimumExtent ? WindowMinimumExtent : Height;
}

void AdvancePartitionDrag(const ThemeSpecification& Theme,
                          WorkspaceSpace&           Space,
                          WorkspaceRectangle        DeskArea,
                          float                     PointerX,
                          float                     PointerY)
{
    const std::int32_t Held = Space.Dragging.HeldLink;

    if (Held < 0 || static_cast<std::size_t>(Held) >= Space.Partitions.size())
    {
        Space.Dragging.Mode = WorkspaceDragMode::None;

        return;
    }

    WorkspacePartition<WorkspaceDocumentIdentity>& Division = Space.Partitions[static_cast<std::size_t>(Held)];

    if (!Division.SlotOccupied || Division.LeafDeclared || !Division.LayoutResolved)
    {
        Space.Dragging.Mode = WorkspaceDragMode::None;

        return;
    }

    const bool  Across = Division.Axis == WorkspacePartitionAxis::Row;
    const float Span   = Across ? Division.Area.Width : Division.Area.Height;
    const float Along  = Across ? PointerX - Division.Area.PositionX : PointerY - Division.Area.PositionY;

    if (Span <= 1.0f)
        return;

    const float Asked = Along / Span;

    Division.Ratio = Asked < 0.05f ? 0.05f : (Asked > 0.95f ? 0.95f : Asked);

    // 🔴 Re-resolved inside the drag. A gutter whose ratio changed but whose rectangles did not is a gutter the
    //    next pointer test measures against where it used to be, which reads as a drag that lags the pointer.
    ResolveSpaceLayout(Space, DeskArea, Theme.Extents.GutterThickness);
}

void SealDrag(const ThemeSpecification& Theme, WorkspaceSpace& Space, WorkspaceRectangle DeskArea)
{
    WorkspaceDragRecord& Held = Space.Dragging;

    // 🔴 A released panel drag is sealed by the panel layer and returns here without touching the desk's own landing.
    //    A panel lands inside one body and the zones below re-divide the desk, so one seal cannot serve both: the
    //    defect is a panel dropped on a body's left edge splitting the desk instead of docking in the body.
    if (Held.Mode == WorkspaceDragMode::PanelBox
     || Held.Mode == WorkspaceDragMode::PanelResize
     || Held.Mode == WorkspaceDragMode::PanelBandResize)
    {
        SealPanelDrag(Theme, Space);

        return;
    }

    const bool Landed = Held.Mode == WorkspaceDragMode::Window
                     && Held.HeldDocument.IdentityDeclared()
                     && Held.PreviewZone != WorkspaceDropZone::None;

    if (Landed)
    {
        if (Held.PreviewWindow != 0u)
            StackDocumentInWindow(Space, Held.HeldDocument, Held.PreviewWindow);
        else
            DockDocument(Space, Held.HeldDocument, Held.PreviewLink, Held.PreviewZone);

        ResolveSpaceLayout(Space, DeskArea, Theme.Extents.GutterThickness);
    }

    // 📝 The whole record is returned to rest rather than having its mode cleared. A preview left standing paints
    //    an accent wash over a landing no drag is asking for any more.
    Space.Dragging = WorkspaceDragRecord{};
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE ADVANCE
//------------------------------------------------------------------------------------------------------------------------

namespace StripInterior
{

void AdvanceWorkspaceDrag(const ThemeSpecification& Theme,
                          WorkspaceSpace&           Space,
                          WorkspaceRectangle        DeskArea,
                          bool&                     PointerConsumed)
{
    const ImGuiIO& Pointing = ImGui::GetIO();

    const float PointerX = Pointing.MousePos.x;
    const float PointerY = Pointing.MousePos.y;
    const bool  Holding  = ImGui::IsMouseDown(ImGuiMouseButton_Left);

    WorkspaceDragRecord& Held = Space.Dragging;

    // ① a press that has travelled far enough becomes either a reorder or a tear ---------------------------------------
    if (Held.Mode == WorkspaceDragMode::None && Held.PendingDocument.IdentityDeclared() && Holding
     && TravelledPastThreshold(Held, PointerX, PointerY))
    {
        OpenTearOrReorder(Theme, Space, DeskArea, PointerX, PointerY);
    }

    if (Held.Mode == WorkspaceDragMode::None)
        return;

    // ② the release seals it, whatever it was ---------------------------------------------------------------------------
    if (!Holding)
    {
        SealDrag(Theme, Space, DeskArea);

        return;
    }

    // ③ one tick of the drag in flight ------------------------------------------------------------------------------------
    switch (Held.Mode)
    {
        case WorkspaceDragMode::Window:
            AdvanceWindowDrag(Theme, Space, PointerX, PointerY);
            break;

        case WorkspaceDragMode::Resize:
            AdvanceResizeDrag(Space, PointerX, PointerY);
            break;

        case WorkspaceDragMode::Partition:
            AdvancePartitionDrag(Theme, Space, DeskArea, PointerX, PointerY);
            break;

        case WorkspaceDragMode::Reorder:
            // 📝 The swap itself is declared by the strip, which is the one place each trapezoid's rectangle is
            //    known. Recomputing that geometry here would be a second copy of it, drifting from the first. What
            //    is decided here is only whether the reorder is still a reorder — travel that leaves the strip
            //    promotes it to a tear.
            AdvanceReorderDrag(Theme, Space, DeskArea, PointerX, PointerY);
            break;

        // 📝 The three panel modes are advanced by the panel layer, which is the one place a box's body rectangle is
        //    resolved. Advancing them here would need a second copy of that resolution, drifting from the first.
        case WorkspaceDragMode::PanelBox:
        case WorkspaceDragMode::PanelResize:
        case WorkspaceDragMode::PanelBandResize:
            AdvancePanelDrag(Theme, Space, PointerX, PointerY);
            break;

        case WorkspaceDragMode::None:
        default:
            break;
    }

    // 🔴 Capture persists for the whole drag. Re-arbitrating mid-drag is `14` §4.2's defect where a stroke stops
    //    the moment the cursor crosses a floating panel, and it is the same mechanism here.
    PointerConsumed = true;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE FLOATING WINDOWS
//------------------------------------------------------------------------------------------------------------------------

std::uint32_t LocateWindowCovering(const WorkspaceSpace& Space, float PointerX, float PointerY)
{
    for (std::size_t Ordinal = Space.Floating.size(); Ordinal > 0u; --Ordinal)
    {
        if (RectangleCovers(AreaOf(Space.Floating[Ordinal - 1u]), PointerX, PointerY))
            return Space.Floating[Ordinal - 1u].Identifier;
    }

    return 0u;
}

void PresentFloatingWindows(const ThemeSpecification& Theme,
                            WorkspaceSpace&           Space,
                            const PanelIndex*         Panels,
                            DeferredIntent&           Arriving,
                            bool&                     PointerConsumed)
{
    const LayoutExtents& Extents   = Theme.Extents;
    const ThemePalette&  Palette   = Theme.Palette;
    ImDrawList*          Recording = ImGui::GetForegroundDrawList();
    const ImGuiIO&       Pointing  = ImGui::GetIO();

    const float PointerX = Pointing.MousePos.x;
    const float PointerY = Pointing.MousePos.y;

    // 📝 🔴 Painted in list order so the last window lands on top, but only the topmost window under the pointer
    //    resolves input. Iterating in reverse for both would paint the topmost first and bury it under the rest.
    const std::uint32_t Covering = LocateWindowCovering(Space, PointerX, PointerY);

    for (std::size_t Ordinal = 0u; Ordinal < Space.Floating.size(); ++Ordinal)
    {
        const WorkspaceFloatingWindow& Standing = Space.Floating[Ordinal];
        const WorkspaceRectangle       Area     = AreaOf(Standing);

        Recording->AddRectFilled(Corner(Area), Opposite(Area), Coded(Palette.DeskBackground), Extents.CornerRounding);
        Recording->AddRect(Corner(Area), Opposite(Area), Coded(Palette.PanelBorder), Extents.CornerRounding,
                           0, Extents.BorderThickness);

        StripCarrier Carrier;
        Carrier.Area   = Area;
        Carrier.Link   = -1;
        Carrier.Window = Standing.Identifier;

        // 📝 The window's own input is skipped entirely when another window covers the pointer, which is the same
        //    guard a blocked leaf takes. A window presenting under another one still paints; it just cannot be hit.
        bool Blocked = PointerConsumed || Covering != Standing.Identifier;

        PresentOccupantStrip(Theme, Space, Carrier, Standing.Documents, Standing.ActiveDocument, Arriving, Blocked);

        // 📝 🔴 The window's panel layer is presented before its own resize grip is tested, so a panel box sitting over
        //    the bottom-right corner takes the press that lands on it. Testing the grip first would make the corner of
        //    the window reach through whatever panel the artist put there, which is the one press it cannot be.
        if (Standing.ActiveDocument.IdentityDeclared())
        {
            PresentPanelLayer(Theme, Space, Standing.ActiveDocument,
                              BodyOf(Area, Extents.TabStripHeight), Panels, Arriving, Blocked);
        }

        const WorkspaceRectangle Grip = GripOf(Area);

        PaintGripStroke(Recording, Grip, Coded(Palette.TextMuted), Extents.BorderThickness);

        if (Covering != Standing.Identifier || PointerConsumed)
            continue;

        if (!Blocked && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            const WorkspaceRectangle Strip = StripOf(Area, Extents.TabStripHeight);

            if (RectangleCovers(Grip, PointerX, PointerY))
            {
                Space.Dragging.Mode       = WorkspaceDragMode::Resize;
                Space.Dragging.HeldWindow = Standing.Identifier;

                PointerConsumed = true;
            }
            else if (RectangleCovers(Strip, PointerX, PointerY))
            {
                Space.Dragging.Mode        = WorkspaceDragMode::Window;
                Space.Dragging.Origin      = WorkspaceDragOrigin::Tab;
                Space.Dragging.HeldWindow  = Standing.Identifier;
                Space.Dragging.GrabOffsetX = PointerX - Area.PositionX;
                Space.Dragging.GrabOffsetY = PointerY - Area.PositionY;

                // 📝 A window carrying one document can be docked whole; one carrying several can only be moved,
                //    because docking would land one document and abandon the rest.
                Space.Dragging.HeldDocument = Standing.Documents.size() == 1u ? Standing.ActiveDocument
                                                                              : WorkspaceDocumentIdentity{};

                PointerConsumed = true;
            }
        }

        if (RectangleCovers(Area, PointerX, PointerY) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
         && Ordinal + 1u != Space.Floating.size())
        {
            Arriving.RaiseDeclared = true;
            Arriving.RaiseWindow   = Standing.Identifier;
        }

        if (Blocked)
            PointerConsumed = true;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE PREVIEW
//------------------------------------------------------------------------------------------------------------------------

void PaintDragPreview(const ThemeSpecification& Theme, const WorkspaceSpace& Space)
{
    // 📝 🔴 Two records name a preview and the mode says which. A desk landing declares a `PreviewZone`; a panel move
    //    declares a `PreviewSide` and leaves the zone at rest, so gating on the zone alone paints no panel preview at
    //    all — and a panel dragged towards a band would then dock to a side the artist was never shown.
    // 📝 A panel heading for `Floating` washes nothing. That landing's preview rectangle is the whole body, and a body
    //    filled with accent while the box is simply being moved around inside it reads as a dock about to happen.
    const bool Landing = Space.Dragging.Mode == WorkspaceDragMode::PanelBox
                       ? Space.Dragging.PreviewSide != WorkspacePanelSide::Floating
                       : Space.Dragging.PreviewZone != WorkspaceDropZone::None;

    if (!Landing)
        return;

    ImDrawList* Recording = ImGui::GetForegroundDrawList();

    const WorkspaceRectangle& Preview = Space.Dragging.PreviewArea;

    // 📝 One colour at two coverages, never two literals. `Attenuate` is what the palette's own note asks for and
    //    the wash has to be translucent enough that the artist can read the leaf beneath the landing.
    Recording->AddRectFilled(Corner(Preview), Opposite(Preview),
                             Coded(Attenuate(Theme.Palette.AccentPrimary, 0.22)), Theme.Extents.CornerRounding);
    Recording->AddRect(Corner(Preview), Opposite(Preview),
                       Coded(Attenuate(Theme.Palette.AccentPrimary, 0.85)), Theme.Extents.CornerRounding,
                       0, Theme.Extents.TabUnderline);
}

}   // namespace StripInterior
}   // namespace Slate
