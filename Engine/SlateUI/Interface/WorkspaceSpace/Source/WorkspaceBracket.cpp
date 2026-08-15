//============================================================================================================================================
//                                                          WORKSPACEBRACKET.CPP
//============================================================================================================================================
// 🧩 The one call an application makes per tick — workspace strip, desk, bottom band, all on the foreground recording.

#include "SlateUI/Interface/WorkspaceSpace/Source/WorkspaceStripInternal.h"

#include "SlateUI/Interface/DrawerPanel/Api/DrawerPanel.h"

namespace Slate
{

using namespace StripInterior;

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE FOOTER BAND
//------------------------------------------------------------------------------------------------------------------------

// 📝 The band is a filled strip across the whole display with one hairline along the edge it meets the desk on and
//    one caption at its leading edge. Only the footer remains: the display's top row is the workspace strip itself,
//    because a caption band above it printed the standing workspace's name a second time, once as a trapezoid the
//    artist can press and once as a rectangle that does nothing.
void PaintViewportBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Area,
                       const char*                Caption)
{
    if (Area.Height <= 0.0f || Area.Width <= 0.0f)
        return;

    const LayoutExtents& Extents   = Theme.Extents;
    const ThemePalette&  Palette   = Theme.Palette;
    ImDrawList*          Recording = ImGui::GetForegroundDrawList();

    Recording->AddRectFilled(Corner(Area), Opposite(Area), Coded(Palette.PanelHeader));

    Recording->AddLine(ImVec2(Area.PositionX, Area.PositionY),
                       ImVec2(Area.PositionX + Area.Width, Area.PositionY),
                       Coded(Palette.PanelBorder), Extents.BorderThickness);

    if (Caption == nullptr || Caption[0] == '\0')
        return;

    const ImVec2 Measured = ImGui::CalcTextSize(Caption);

    Recording->AddText(ImVec2(Area.PositionX + Extents.PanelPadding * 2.0f,
                              Area.PositionY + (Area.Height - Measured.y) * 0.5f),
                       Coded(Palette.TextMuted), Caption);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE EDGE DRAWERS
//------------------------------------------------------------------------------------------------------------------------

// 📝 🔴 Whether either declared drawer is holding the pointer, asked **before** the desk resolves anything. A drawer
//    owns no vendor window, so a panel sitting under the open sheet would otherwise answer the same press — the desk
//    would hover a row the artist cannot see, and a click meant for a control inside the drawer would activate a tab
//    beneath it as well. This is the whole of what "the drawers are above every panel" means mechanically.
bool DrawersCapturingPointer(const ThemeSpecification&  Theme,
                             const DrawerIndex*         Drawers,
                             float                      DisplayWidth,
                             float                      DisplayHeight)
{
    if (Drawers == nullptr)
        return false;

    const LayoutExtents& Extents = Theme.Extents;

    if (Drawers->BottomDrawer.Drawer != nullptr
     && DrawerCapturingPointer(*Drawers->BottomDrawer.Drawer, Extents, DisplayWidth, DisplayHeight))
    {
        return true;
    }

    return Drawers->TopDrawer.Drawer != nullptr
        && DrawerCapturingPointer(*Drawers->TopDrawer.Drawer, Extents, DisplayWidth, DisplayHeight);
}

// 📝 What the pointer carried before a drawer took it, held only across the calls that paint beneath the drawer.
// 📝 🔴 The wheel and the three button rows are suspended and **not** the position. A desk that also lost the cursor
//    would report every row unhovered and repaint its whole strip the tick a drawer opened; the source suspends
//    exactly these five, and the hover a panel resolves beneath an open sheet is invisible under it anyway.
struct PointerSuspension
{
    float  Wheel           = 0.0f;      // [-] - vertical wheel travel this tick
    float  WheelSideways   = 0.0f;      // [-] - horizontal
    bool   Pressing[5]     = {};        // [-] - the button is down
    bool   PressBegun[5]   = {};        // [-] - it went down this tick
    bool   PressEnded[5]   = {};        // [-] - it came up this tick
    bool   Suspended       = false;     // [-] - the rows above are worth restoring
};

// 📝 🔴 Blinds the pointer for the calls that paint beneath an open drawer, exactly as the source does. The desk is
//    still presented — a desk skipped outright would vanish from behind a sheet that only covers part of it — but it
//    is presented against a pointer that presses nothing. This is what "the drawers are above every panel" means
//    mechanically: a drawer owns no vendor window, so nothing else stops the panel under the cursor answering the
//    same press, and the sheet would drag while a tab beneath it activated.
void SuspendPointer(PointerSuspension& Suspension)
{
    ImGuiIO& Pointing = ImGui::GetIO();

    Pointing.WantCaptureMouse = true;

    Suspension.Wheel         = Pointing.MouseWheel;
    Suspension.WheelSideways = Pointing.MouseWheelH;
    Pointing.MouseWheel      = 0.0f;
    Pointing.MouseWheelH     = 0.0f;

    for (int Button = 0; Button < 5; ++Button)
    {
        Suspension.Pressing[Button]   = Pointing.MouseDown[Button];
        Suspension.PressBegun[Button] = Pointing.MouseClicked[Button];
        Suspension.PressEnded[Button] = Pointing.MouseReleased[Button];

        Pointing.MouseDown[Button]     = false;
        Pointing.MouseClicked[Button]  = false;
        Pointing.MouseReleased[Button] = false;
    }

    Suspension.Suspended = true;
}

// 📝 🔴 And hands it back before the drawer itself paints, so the notch reads the true press to drag on and the body's
//    own controls read the true clicks to act on. The pass beneath already ran blind; restoring here shields it and
//    nothing else. Omitting this is the defect where an open drawer stops answering its own controls.
void RestorePointer(const PointerSuspension& Suspension)
{
    if (!Suspension.Suspended)
        return;

    ImGuiIO& Pointing = ImGui::GetIO();

    Pointing.MouseWheel  = Suspension.Wheel;
    Pointing.MouseWheelH = Suspension.WheelSideways;

    for (int Button = 0; Button < 5; ++Button)
    {
        Pointing.MouseDown[Button]     = Suspension.Pressing[Button];
        Pointing.MouseClicked[Button]  = Suspension.PressBegun[Button];
        Pointing.MouseReleased[Button] = Suspension.PressEnded[Button];
    }
}

// 📝 The bottom drawer is presented first and the top one over it, so where both are dragged fully open the control
//    centre is the one the artist reaches. That ordering is the same one the two reveals already imply — only the top
//    drawer covers the whole display, so only it can be the sheet in front.
bool PresentEdgeDrawers(const ThemeSpecification&  Theme,
                        const DrawerIndex*         Drawers,
                        float                      DisplayWidth,
                        float                      DisplayHeight)
{
    if (Drawers == nullptr)
        return false;

    bool Consumed = false;

    if (Drawers->BottomDrawer.Drawer != nullptr)
    {
        Consumed = PresentDrawer(Theme, *Drawers->BottomDrawer.Drawer, DisplayWidth, DisplayHeight,
                                 Drawers->BottomDrawer.Body, Drawers->BottomDrawer.BodyContext) || Consumed;
    }

    if (Drawers->TopDrawer.Drawer != nullptr)
    {
        Consumed = PresentDrawer(Theme, *Drawers->TopDrawer.Drawer, DisplayWidth, DisplayHeight,
                                 Drawers->TopDrawer.Body, Drawers->TopDrawer.BodyContext) || Consumed;
    }

    return Consumed;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE WORKSPACE STRIP
//------------------------------------------------------------------------------------------------------------------------

std::uint32_t ConstructWorkspaceTabStrip(const ThemeSpecification&  Theme,
                                         WorkspaceRectangle         StripArea,
                                         const char* const*         Captions,
                                         std::uint32_t              Count,
                                         std::uint32_t              ActiveOrdinal,
                                         bool&                      PointerConsumed)
{
    if (Captions == nullptr || Count == 0u || StripArea.Height <= 0.0f || StripArea.Width <= 0.0f)
        return AbsentWorkspaceChoice;

    const LayoutExtents& Extents   = Theme.Extents;
    const ThemePalette&  Palette   = Theme.Palette;
    ImDrawList*          Recording = ImGui::GetForegroundDrawList();
    const ImGuiIO&       Pointing  = ImGui::GetIO();

    Recording->AddRectFilled(Corner(StripArea), Opposite(StripArea), Coded(Palette.DeskBackground));

    std::uint32_t Chosen    = AbsentWorkspaceChoice;
    float         Travelled = StripArea.PositionX;

    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
    {
        const char* Caption = Captions[Ordinal] != nullptr ? Captions[Ordinal] : "Workspace";

        WorkspaceRectangle Trapezoid;
        Trapezoid.PositionX = Travelled;
        Trapezoid.PositionY = StripArea.PositionY;
        Trapezoid.Width     = ImGui::CalcTextSize(Caption).x + Extents.TabInset * 2.0f + Extents.TabSlant * 2.0f;
        Trapezoid.Height    = StripArea.Height;

        // 📝 The strip stops rather than painting a trapezoid that leaves the display. A caption the artist cannot
        //    read is worse than one that is absent, because an unreachable tab still resolves presses.
        if (Trapezoid.PositionX + Trapezoid.Width > StripArea.PositionX + StripArea.Width)
            break;

        const bool Standing = Ordinal == ActiveOrdinal;
        const bool Covered  = RectangleCovers(Trapezoid, Pointing.MousePos.x, Pointing.MousePos.y);

        ThemeColour Face = Standing ? Palette.PanelBackground : Palette.DeskBackground;

        if (!Standing && Covered)
            Face = Palette.RowHovered;

        PaintTrapezoid(Recording, Trapezoid, Extents.TabSlant, Coded(Face));

        const ImVec2 Measured = ImGui::CalcTextSize(Caption);

        Recording->AddText(ImVec2(Trapezoid.PositionX + Extents.TabSlant + Extents.TabInset,
                                  Trapezoid.PositionY + (Trapezoid.Height - Measured.y) * 0.5f),
                           Coded(Standing ? Palette.TextPrimary : Palette.TextMuted), Caption);

        if (Standing)
        {
            const float UnderlineY = Trapezoid.PositionY + Trapezoid.Height - Extents.TabUnderline;

            Recording->AddRectFilled(ImVec2(Trapezoid.PositionX + Extents.TabSlant, UnderlineY),
                                     ImVec2(Trapezoid.PositionX + Trapezoid.Width - Extents.TabSlant,
                                            UnderlineY + Extents.TabUnderline),
                                     Coded(Palette.AccentPrimary));
        }

        if (Covered)
        {
            PointerConsumed = true;

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                Chosen = Ordinal;
        }

        // 📝 The trapezoids overlap by one slant so their sloped edges meet rather than leaving a wedge of desk
        //    between every pair. Frontier's strip does the same and it is what makes the row read as one band.
        Travelled += Trapezoid.Width - Extents.TabSlant;
    }

    if (RectangleCovers(StripArea, Pointing.MousePos.x, Pointing.MousePos.y))
        PointerConsumed = true;

    return Chosen;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE DEPLOYMENT BRACKET
//------------------------------------------------------------------------------------------------------------------------

DeploymentReport PresentDeploymentBracket(const ThemeSpecification&  Theme,
                                          WorkspaceSpace&            Space,
                                          const PanelIndex*          Panels,
                                          const DrawerIndex*         Drawers,
                                          const char* const*         Captions,
                                          std::uint32_t              Count,
                                          std::uint32_t              ActiveOrdinal,
                                          const char*                BottomBandCaption,
                                          float                      DisplayWidth,
                                          float                      DisplayHeight)
{
    const LayoutExtents& Extents = Theme.Extents;

    DeploymentReport Reported;

    // 🔴 The style is mirrored once per tick, here, and never inside a panel. `14` §7: a panel that enforced the
    //    theme itself would leave whichever panel presented last deciding what the next tick's controls look like.
    Enforce(Theme);

    // 📐 The workspace strip is the display's first row. Nothing is reserved above it — the trapezoid already names
    //    the standing workspace, and a caption band repeating that name cost a row of desk for no readable gain.
    // 📐 🔴 A single registered workspace is presented with no roster row at all. The row would carry exactly one
    //    trapezoid that cannot be switched away from, and it would sit directly above the desk leaf's own document
    //    strip — two stacked strips reading as a workspace nested inside a workspace. Worse, the roster row is the
    //    one an artist meets first and it is the one that does the least: `ConstructWorkspaceTabStrip` resolves a
    //    choice and nothing else, so it declares no panel, begins no drag and answers no `(+)`. Suppressing it hands
    //    its row to the desk, whose `PresentOccupantStrip` carries the panel list, the mint, the drag and the tear.
    const bool RosterPresented = Count > 1u;

    WorkspaceRectangle StripArea;
    StripArea.PositionX = 0.0f;
    StripArea.PositionY = 0.0f;
    StripArea.Width     = DisplayWidth;
    StripArea.Height    = RosterPresented ? Extents.TabStripHeight : 0.0f;

    WorkspaceRectangle BottomBand;
    BottomBand.PositionX = 0.0f;
    BottomBand.PositionY = DisplayHeight - Extents.ViewportBandBottom;
    BottomBand.Width     = DisplayWidth;
    BottomBand.Height    = Extents.ViewportBandBottom;

    // 📐 The desk takes what the two fixed rows leave. Bounded at zero rather than allowed negative: a negative
    //    height inverts every coverage test in `RectangleCovers`, so a minimised window would resolve presses
    //    against rectangles that cover the whole display instead of none of it.
    const float DeskTop    = StripArea.PositionY + StripArea.Height;
    const float DeskBottom = BottomBand.PositionY;

    WorkspaceRectangle DeskArea;
    DeskArea.PositionX = 0.0f;
    DeskArea.PositionY = DeskTop;
    DeskArea.Width     = DisplayWidth > 0.0f ? DisplayWidth : 0.0f;
    DeskArea.Height    = DeskBottom > DeskTop ? DeskBottom - DeskTop : 0.0f;

    Reported.DeskArea = DeskArea;

    // 📝 The strip resolves its input before the desk does, and the desk is presented afterwards, so a press on a
    //    workspace trapezoid never reaches the document tabs beneath it. The two strips sit one above the other
    //    and a press that resolved twice would activate a document while switching workspace.
    // 📝 The suppressed roster is skipped outright rather than left to refuse on its own zero height. It reports no
    //    choice either way, but a call that paints nothing still takes the pointer on its last coverage test — and
    //    a zero-height rectangle at the display's top edge covers the row of desk immediately beneath it.
    // 🔴 Asked here, before one rectangle beneath a drawer has resolved anything. A drawer opened over the desk hides
    //    the strip, the gutters and the panel bodies, and none of them know it: they own vendor windows only through
    //    the foreground recording, which arbitrates nothing. Consulted after the desk, the claim would arrive one call
    //    too late — the tab under the cursor would already have taken the press.
    const bool DrawersHolding = DrawersCapturingPointer(Theme, Drawers, DisplayWidth, DisplayHeight);

    PointerSuspension Suspended;

    if (DrawersHolding)
        SuspendPointer(Suspended);

    if (RosterPresented)
    {
        Reported.WorkspaceChoice = ConstructWorkspaceTabStrip(Theme, StripArea, Captions, Count, ActiveOrdinal,
                                                              Reported.PointerConsumed);
    }

    if (DeskArea.Height > 0.0f)
        PresentWorkspaceSpace(Theme, Space, DeskArea, Panels);

    PaintViewportBand(Theme, BottomBand, BottomBandCaption);

    // 📝 The band takes the pointer as much as the desk does. A press on the footer that fell through to the
    //    canvas beneath would start a stroke the artist aimed at a status readout.
    const ImGuiIO& Pointing = ImGui::GetIO();

    if (RectangleCovers(BottomBand, Pointing.MousePos.x, Pointing.MousePos.y)
     || RectangleCovers(DeskArea, Pointing.MousePos.x, Pointing.MousePos.y))
    {
        Reported.PointerConsumed = true;
    }

    RestorePointer(Suspended);

    // 🔴 And painted last, over the band as much as over the desk. The two halves are not interchangeable: the claim
    //    is read before anything beneath resolves, the quads are recorded after everything beneath is drawn. Painting
    //    early would put the sheet under the footer strip; claiming late would put the press under the tabs.
    if (PresentEdgeDrawers(Theme, Drawers, DisplayWidth, DisplayHeight) || DrawersHolding)
        Reported.PointerConsumed = true;

    return Reported;
}

}   // namespace Slate
