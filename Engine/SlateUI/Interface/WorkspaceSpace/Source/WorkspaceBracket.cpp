//============================================================================================================================================
//                                                          WORKSPACEBRACKET.CPP
//============================================================================================================================================
// 🧩 The one call an application makes per tick — top band, workspace strip, desk, bottom band, all on the foreground recording.

#include "SlateUI/Interface/WorkspaceSpace/Source/WorkspaceStripInternal.h"

namespace Slate
{

using namespace StripInterior;

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE BANDS
//------------------------------------------------------------------------------------------------------------------------

// 📝 A band is a filled strip across the whole display with one hairline against the desk and one caption at its
//    leading edge. Both bands are the same shape and differ only in which edge the hairline sits on, so they are
//    one routine taking that edge rather than two that drift apart the first time either is amended.
void PaintViewportBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Area,
                       const char*                Caption,
                       bool                       HairlineBelow)
{
    if (Area.Height <= 0.0f || Area.Width <= 0.0f)
        return;

    const LayoutExtents& Extents   = Theme.Extents;
    const ThemePalette&  Palette   = Theme.Palette;
    ImDrawList*          Recording = ImGui::GetForegroundDrawList();

    Recording->AddRectFilled(Corner(Area), Opposite(Area), Coded(Palette.PanelHeader));

    const float HairlineY = HairlineBelow ? Area.PositionY + Area.Height : Area.PositionY;

    Recording->AddLine(ImVec2(Area.PositionX, HairlineY),
                       ImVec2(Area.PositionX + Area.Width, HairlineY),
                       Coded(Palette.PanelBorder), Extents.BorderThickness);

    if (Caption == nullptr || Caption[0] == '\0')
        return;

    const ImVec2 Measured = ImGui::CalcTextSize(Caption);

    Recording->AddText(ImVec2(Area.PositionX + Extents.PanelPadding * 2.0f,
                              Area.PositionY + (Area.Height - Measured.y) * 0.5f),
                       Coded(Palette.TextMuted), Caption);
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
                                          const char* const*         Captions,
                                          std::uint32_t              Count,
                                          std::uint32_t              ActiveOrdinal,
                                          const char*                TopBandCaption,
                                          const char*                BottomBandCaption,
                                          float                      DisplayWidth,
                                          float                      DisplayHeight)
{
    const LayoutExtents& Extents = Theme.Extents;

    DeploymentReport Reported;

    // 🔴 The style is mirrored once per tick, here, and never inside a panel. `14` §7: a panel that enforced the
    //    theme itself would leave whichever panel presented last deciding what the next tick's controls look like.
    Enforce(Theme);

    WorkspaceRectangle TopBand;
    TopBand.PositionX = 0.0f;
    TopBand.PositionY = 0.0f;
    TopBand.Width     = DisplayWidth;
    TopBand.Height    = Extents.ViewportBandTop;

    WorkspaceRectangle StripArea;
    StripArea.PositionX = 0.0f;
    StripArea.PositionY = TopBand.Height;
    StripArea.Width     = DisplayWidth;
    StripArea.Height    = Extents.TabStripHeight;

    WorkspaceRectangle BottomBand;
    BottomBand.PositionX = 0.0f;
    BottomBand.PositionY = DisplayHeight - Extents.ViewportBandBottom;
    BottomBand.Width     = DisplayWidth;
    BottomBand.Height    = Extents.ViewportBandBottom;

    // 📐 The desk takes what the three fixed rows leave. Bounded at zero rather than allowed negative: a negative
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

    PaintViewportBand(Theme, TopBand, TopBandCaption, true);

    // 📝 The strip resolves its input before the desk does, and the desk is presented afterwards, so a press on a
    //    workspace trapezoid never reaches the document tabs beneath it. The two strips sit one above the other
    //    and a press that resolved twice would activate a document while switching workspace.
    Reported.WorkspaceChoice = ConstructWorkspaceTabStrip(Theme, StripArea, Captions, Count, ActiveOrdinal,
                                                          Reported.PointerConsumed);

    if (DeskArea.Height > 0.0f)
        PresentWorkspaceSpace(Theme, Space, DeskArea, Panels);

    PaintViewportBand(Theme, BottomBand, BottomBandCaption, false);

    // 📝 The bands take the pointer as much as the desk does. A press on the footer that fell through to the
    //    canvas beneath would start a stroke the artist aimed at a status readout.
    const ImGuiIO& Pointing = ImGui::GetIO();

    if (RectangleCovers(TopBand, Pointing.MousePos.x, Pointing.MousePos.y)
     || RectangleCovers(BottomBand, Pointing.MousePos.x, Pointing.MousePos.y)
     || RectangleCovers(DeskArea, Pointing.MousePos.x, Pointing.MousePos.y))
    {
        Reported.PointerConsumed = true;
    }

    return Reported;
}

}   // namespace Slate
