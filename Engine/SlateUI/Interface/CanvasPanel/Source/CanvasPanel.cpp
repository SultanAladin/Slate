//============================================================================================================================================
//                                                             CANVASPANEL.CPP
//============================================================================================================================================
// 🧩 One viewport's tick — the band arithmetic, the three hand-rolled overlays, and the quads that frame a canvas nothing has drawn into yet.

#include "SlateUI/Interface/CanvasPanel/Api/CanvasPanel.h"

#include "imgui.h"

#include <cstdio>
#include <cstring>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   CODES AND SHAPES
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📝 🚧 The five helpers below are spelled a second time here rather than shared with the desk's own. `WorkspaceStripInternal.h`
//    is a Source-only header of another component and including it across that seam is what it exists to prevent, so the choice
//    is one small copy or a public vendor-naming header — and `14` §7 bars the second outright. The closing move is an internal
//    recording header shared by `SlateUI`'s Source folders, and it is owed the moment a third component needs these.

// 📝 The quantised code and the vendor's packed colour agree channel for channel, so this is a re-spelling and not a
//    conversion. `Quantize` stays the one place a theme colour becomes an integer.
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

WorkspaceRectangle SquareAt(float PositionX, float PositionY, float Edge)
{
    WorkspaceRectangle Square;

    Square.PositionX = PositionX;
    Square.PositionY = PositionY;
    Square.Width     = Edge;
    Square.Height    = Edge;

    return Square;
}

// 📝 A procedural stroke and not a depot glyph, for the reason the strip records: `GlyphDepot::Resolve` delivers an opaque
//    handle and nothing outside `GlyphDepot.cpp` can turn one into something paintable, so a chrome tier declared today
//    would resolve to a handle this file cannot use.
void PaintCaretStroke(ImDrawList* Recording, float CentreX, float CentreY, float Edge, ImU32 Code, float Thickness)
{
    const float Reach = Edge * 0.5f;
    const float Rise  = Edge * 0.28f;

    Recording->AddLine(ImVec2(CentreX - Reach, CentreY - Rise), ImVec2(CentreX, CentreY + Rise), Code, Thickness);
    Recording->AddLine(ImVec2(CentreX, CentreY + Rise), ImVec2(CentreX + Reach, CentreY - Rise), Code, Thickness);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE COMPOSED TEXT
//------------------------------------------------------------------------------------------------------------------------

constexpr std::size_t   CaptionExtent    = 96u;   // [-] - characters one composed caption accepts, terminator included
constexpr std::uint32_t OverlayRowCeiling = 8u;   // [-] - rows one overlay may carry into the paint pass

const char* CaptionOf(CanvasProjection Presented)
{
    switch (Presented)
    {
        case CanvasProjection::Orthographic: return "orthographic";

        case CanvasProjection::Perspective:
        default:                             return "perspective";
    }
}

const char* CaptionOf(bool Standing)
{
    return Standing ? "shown" : "hidden";
}

// 📝 Segments are joined rather than formatted into one call, so a readout the artist has switched off costs no separator
//    and the line never opens or closes on a dangling one.
void AppendSegment(char* Carry, std::size_t Extent, const char* Segment)
{
    const std::size_t Held = std::strlen(Carry);

    if (Held + 1u >= Extent)
        return;

    if (Held == 0u)
        std::snprintf(Carry, Extent, "%s", Segment);
    else
        std::snprintf(Carry + Held, Extent - Held, "  ·  %s", Segment);
}

void ComposeExtentCaption(const CanvasSpecification& Canvas, char* Carry, std::size_t Extent)
{
    const Outcome<CanvasExtent> Offered = ResolveCanvasExtent(Canvas.ExtentOrdinal);

    if (!Offered.ContentPresent)
    {
        std::snprintf(Carry, Extent, "no extent declared");

        return;
    }

    std::snprintf(Carry, Extent, "%u × %u", Offered.Resolve().Width, Offered.Resolve().Height);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                THE OVERLAY MECHANISM
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What one overlay's resolution decided, carried to the paint pass at the end of the tick.
struct OverlayReach
{
    bool                OverlayDeclared              = false;   // [-]  - this overlay is standing
    WorkspaceRectangle  Area                         = {};      // [px] - its whole rectangle
    WorkspaceRectangle  Row[OverlayRowCeiling]       = {};      // [px] - one per offered row
    bool                RowCovered[OverlayRowCeiling] = {};     // [-]  - as the resolution found it
    const char*         RowCaption[OverlayRowCeiling] = {};     // [-]  - retained by address for this tick only
    std::uint32_t       RowCount                      = 0u;     // [-]  - rows recorded
};

// 📝 One overlay is a stack of rows on the foreground list, dismissed by any press outside it — except on the tick it
//    opened, whose press is the one that opened it.
// 🔴 Resolution only. The rows this decided are painted by `PaintOverlayReach` after every band has recorded, because one
//    foreground list draws in call order and an overlay recorded here would be buried by the canvas beneath it.
std::int32_t ResolveOverlay(const ThemeSpecification& Theme,
                            CanvasOverlayRecord&      Record,
                            std::uint32_t             PresentedTicks,
                            const char* const*        Captions,
                            std::uint32_t             Count,
                            bool&                     PointerConsumed,
                            OverlayReach&             Reaching)
{
    if (!Record.OverlayOpen || Captions == nullptr || Count == 0u)
        return -1;

    const LayoutExtents& Extents  = Theme.Extents;
    const ImGuiIO&       Pointing = ImGui::GetIO();

    float Widest = 0.0f;

    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
    {
        if (Captions[Ordinal] == nullptr)
            continue;

        const float Measured = ImGui::CalcTextSize(Captions[Ordinal]).x;

        Widest = Measured > Widest ? Measured : Widest;
    }

    WorkspaceRectangle Overlay;
    Overlay.PositionX = Record.AnchorX;
    Overlay.PositionY = Record.AnchorY;
    Overlay.Width     = Widest + Extents.PanelPadding * 4.0f;
    Overlay.Height    = Extents.OverlayRowHeight * static_cast<float>(Count) + Extents.PanelPadding * 2.0f;

    // 📝 Nudged back inside the display on both axes rather than clipped at its edge. A row the artist cannot reach is
    //    worse than one that opened a few pixels from the button that opened it — and on the vertical axis this is what
    //    places the footer's own overlay **above** its band rather than off the bottom of the display.
    if (Overlay.PositionX + Overlay.Width > Pointing.DisplaySize.x)
        Overlay.PositionX = Pointing.DisplaySize.x - Overlay.Width;

    if (Overlay.PositionY + Overlay.Height > Pointing.DisplaySize.y)
        Overlay.PositionY = Pointing.DisplaySize.y - Overlay.Height;

    Reaching.OverlayDeclared = true;
    Reaching.Area            = Overlay;

    std::int32_t Chosen    = -1;
    float        Travelled = Overlay.PositionY + Extents.PanelPadding;

    for (std::uint32_t Ordinal = 0u; Ordinal < Count && Reaching.RowCount < OverlayRowCeiling; ++Ordinal)
    {
        WorkspaceRectangle Row;
        Row.PositionX = Overlay.PositionX + Extents.PanelPadding;
        Row.PositionY = Travelled;
        Row.Width     = Overlay.Width - Extents.PanelPadding * 2.0f;
        Row.Height    = Extents.OverlayRowHeight;

        const bool Covered = RectangleCovers(Row, Pointing.MousePos.x, Pointing.MousePos.y);

        Reaching.Row[Reaching.RowCount]        = Row;
        Reaching.RowCovered[Reaching.RowCount] = Covered;
        Reaching.RowCaption[Reaching.RowCount] = Captions[Ordinal];

        ++Reaching.RowCount;

        if (Covered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            PointerConsumed = true;
            Chosen          = static_cast<std::int32_t>(Ordinal);
        }

        Travelled += Extents.OverlayRowHeight;
    }

    // 📝 A chosen row leaves the overlay standing rather than dismissing it. Every row here toggles or advances one
    //    declaration, so an artist changing two of them in a row would otherwise reopen the same list between them.
    const bool Outside = !RectangleCovers(Overlay, Pointing.MousePos.x, Pointing.MousePos.y);

    if (Outside && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && Record.OpenedTick != PresentedTicks)
    {
        Record.OverlayOpen       = false;
        Reaching.OverlayDeclared = false;
    }

    if (!Outside)
        PointerConsumed = true;

    return Chosen;
}

// 📝 One rectangle, one border and one row per entry, recorded from what the resolution already decided. Nothing here tests
//    the pointer: a second coverage test taken at paint time would drift from the one the input resolved against the moment
//    anything between the two passes moved a band under it.
void PaintOverlayReach(const ThemeSpecification& Theme, const OverlayReach& Reaching)
{
    if (!Reaching.OverlayDeclared)
        return;

    const LayoutExtents& Extents   = Theme.Extents;
    const ThemePalette&  Palette   = Theme.Palette;
    ImDrawList*          Recording = ImGui::GetForegroundDrawList();

    Recording->AddRectFilled(Corner(Reaching.Area), Opposite(Reaching.Area), Coded(Palette.PanelBackground),
                             Extents.CornerRounding);
    Recording->AddRect(Corner(Reaching.Area), Opposite(Reaching.Area), Coded(Palette.PanelBorder),
                       Extents.CornerRounding, 0, Extents.BorderThickness);

    for (std::uint32_t Ordinal = 0u; Ordinal < Reaching.RowCount; ++Ordinal)
    {
        if (Reaching.RowCovered[Ordinal])
        {
            Recording->AddRectFilled(Corner(Reaching.Row[Ordinal]), Opposite(Reaching.Row[Ordinal]),
                                     Coded(Palette.RowHovered), Extents.CornerRounding * 0.5f);
        }

        if (Reaching.RowCaption[Ordinal] == nullptr)
            continue;

        const ImVec2 Measured = ImGui::CalcTextSize(Reaching.RowCaption[Ordinal]);

        Recording->AddText(ImVec2(Reaching.Row[Ordinal].PositionX + Extents.PanelPadding,
                                  Reaching.Row[Ordinal].PositionY
                                      + (Reaching.Row[Ordinal].Height - Measured.y) * 0.5f),
                           Coded(Palette.TextPrimary), Reaching.RowCaption[Ordinal]);
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE GLYPH BUTTON
//------------------------------------------------------------------------------------------------------------------------

// 📝 The `(V)` a band carries, painted and resolved in one place so the three bands cannot drift apart by a pixel. It opens
//    its own record and anchors it where the button sits; the nudge inside `ResolveOverlay` does the rest.
void PresentCaretButton(const ThemeSpecification&  Theme,
                        const WorkspaceRectangle&  Button,
                        CanvasOverlayRecord&       Record,
                        std::uint32_t              PresentedTicks,
                        bool&                      PointerConsumed)
{
    const LayoutExtents& Extents   = Theme.Extents;
    const ThemePalette&  Palette   = Theme.Palette;
    ImDrawList*          Recording = ImGui::GetForegroundDrawList();
    const ImGuiIO&       Pointing  = ImGui::GetIO();

    if (Button.Width <= 0.0f || Button.Height <= 0.0f)
        return;

    const bool Covered = RectangleCovers(Button, Pointing.MousePos.x, Pointing.MousePos.y);

    if (Covered || Record.OverlayOpen)
    {
        Recording->AddRectFilled(Corner(Button), Opposite(Button),
                                 Coded(Record.OverlayOpen ? Palette.ControlActive : Palette.ControlHovered),
                                 Extents.CornerRounding);
    }

    PaintCaretStroke(Recording,
                     Button.PositionX + Button.Width * 0.5f,
                     Button.PositionY + Button.Height * 0.5f,
                     Extents.GlyphEdge * 0.7f,
                     Coded(Record.OverlayOpen ? Palette.TextPrimary : Palette.TextMuted),
                     Extents.TabUnderline * 0.75f);

    if (PointerConsumed || !Covered || !ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        return;

    PointerConsumed = true;

    // 📝 A second press on a standing button closes its own list rather than reopening it. Without this the dismissal and
    //    the opening fight over the same press and the overlay never closes from the button it opened from.
    if (Record.OverlayOpen)
    {
        Record.OverlayOpen = false;

        return;
    }

    Record.OverlayOpen = true;
    Record.OpenedTick  = PresentedTicks;
    Record.AnchorX     = Button.PositionX;
    Record.AnchorY     = Button.PositionY + Button.Height;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE OFFERED EXTENTS
//------------------------------------------------------------------------------------------------------------------------

Outcome<CanvasExtent> ResolveCanvasExtent(std::uint32_t ExtentOrdinal)
{
    // 📝 The one place an image extent literal is permitted in this component, and the roster is closed so a fifth entry is
    //    one row here rather than a second list somewhere a reader has to find.
    static constexpr CanvasExtent Offered[CanvasExtentCount] =
    {
        { 1280u,  720u, "1280 × 720"  },
        { 1920u, 1080u, "1920 × 1080" },
        { 2560u, 1440u, "2560 × 1440" },
        { 3840u, 2160u, "3840 × 2160" }
    };

    if (ExtentOrdinal >= CanvasExtentCount)
    {
        return Outcome<CanvasExtent>::Refuse(
            { RefusalReason::ContentUnsupported, "the extent ordinal lies outside the declared roster" });
    }

    return Outcome<CanvasExtent>::Deliver(Offered[ExtentOrdinal]);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE BAND ARITHMETIC
//------------------------------------------------------------------------------------------------------------------------

CanvasBands ResolveCanvasBands(const LayoutExtents& Extents, WorkspaceRectangle Body)
{
    CanvasBands Resolved;

    if (Body.Width <= 0.0f || Body.Height <= 0.0f)
        return Resolved;

    // 📐 ① the header takes its declared height, or whatever the body has when that is less -------------------------------
    const float HeaderHeight = Extents.PanelHeaderHeight < Body.Height ? Extents.PanelHeaderHeight : Body.Height;

    Resolved.HeaderBand.PositionX = Body.PositionX;
    Resolved.HeaderBand.PositionY = Body.PositionY;
    Resolved.HeaderBand.Width     = Body.Width;
    Resolved.HeaderBand.Height    = HeaderHeight;

    // 📐 ② the footer takes its declared height out of what the header and one gutter left ---------------------------------
    const float BelowHeader = Body.Height - HeaderHeight - Extents.GutterThickness;
    const float Available   = BelowHeader > 0.0f ? BelowHeader : 0.0f;
    const float FooterHeight = Extents.PanelFooterHeight < Available ? Extents.PanelFooterHeight : Available;

    Resolved.FooterBand.PositionX = Body.PositionX;
    Resolved.FooterBand.PositionY = Body.PositionY + Body.Height - FooterHeight;
    Resolved.FooterBand.Width     = Body.Width;
    Resolved.FooterBand.Height    = FooterHeight;

    // 📐 ③ the canvas takes the remainder, one gutter clear of each band ----------------------------------------------------
    const float CanvasTop    = Resolved.HeaderBand.PositionY + HeaderHeight + Extents.GutterThickness;
    const float CanvasBottom = Resolved.FooterBand.PositionY - Extents.GutterThickness;

    Resolved.CanvasArea.PositionX = Body.PositionX;
    Resolved.CanvasArea.PositionY = CanvasTop;
    Resolved.CanvasArea.Width     = Body.Width;
    Resolved.CanvasArea.Height    = CanvasBottom > CanvasTop ? CanvasBottom - CanvasTop : 0.0f;

    return Resolved;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE HEADER BAND
//------------------------------------------------------------------------------------------------------------------------

namespace
{

void PresentHeaderBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       CanvasSpecification&       Canvas,
                       bool&                      PointerConsumed)
{
    if (Band.Width <= 0.0f || Band.Height <= 0.0f)
        return;

    const LayoutExtents& Extents   = Theme.Extents;
    const ThemePalette&  Palette   = Theme.Palette;
    ImDrawList*          Recording = ImGui::GetForegroundDrawList();

    Recording->AddRectFilled(Corner(Band), Opposite(Band), Coded(Palette.PanelHeader), Extents.CornerRounding,
                             ImDrawFlags_RoundCornersTop);

    Recording->AddLine(ImVec2(Band.PositionX, Band.PositionY + Band.Height),
                       ImVec2(Band.PositionX + Band.Width, Band.PositionY + Band.Height),
                       Coded(Palette.PanelBorder), Extents.BorderThickness);

    const char*  Caption  = "Viewport";
    const ImVec2 Measured = ImGui::CalcTextSize(Caption);

    Recording->AddText(ImVec2(Band.PositionX + Extents.PanelPadding * 2.0f,
                              Band.PositionY + (Band.Height - Measured.y) * 0.5f),
                       Coded(Palette.TextPrimary), Caption);

    const WorkspaceRectangle Button =
        SquareAt(Band.PositionX + Band.Width - Extents.TabInset - Extents.GlyphButtonEdge,
                 Band.PositionY + (Band.Height - Extents.GlyphButtonEdge) * 0.5f,
                 Extents.GlyphButtonEdge);

    // 📝 The declarations print as one muted line between the caption and the button, right-aligned against it, so the band
    //    reads as a statement of what the canvas is rather than as a row of controls the artist has to interpret. Every one
    //    of them is amended through the button's own list.
    char Extent[CaptionExtent] = {};
    char Declared[CaptionExtent] = {};

    ComposeExtentCaption(Canvas, Extent, CaptionExtent);

    std::snprintf(Declared, CaptionExtent, "%s  ·  %s  ·  lattice %s",
                  CaptionOf(Canvas.Projection), Extent, CaptionOf(Canvas.LatticeStanding));

    const ImVec2 DeclaredMeasured = ImGui::CalcTextSize(Declared);
    const float  DeclaredLeft     = Button.PositionX - Extents.PanelPadding - DeclaredMeasured.x;

    if (DeclaredLeft > Band.PositionX + Extents.PanelPadding * 2.0f + Measured.x + Extents.PanelPadding)
    {
        Recording->AddText(ImVec2(DeclaredLeft, Band.PositionY + (Band.Height - DeclaredMeasured.y) * 0.5f),
                           Coded(Palette.TextMuted), Declared);
    }

    PresentCaretButton(Theme, Button, Canvas.HeaderOverlay, Canvas.PresentedTicks, PointerConsumed);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE CANVAS AREA
//------------------------------------------------------------------------------------------------------------------------

void PresentCanvasArea(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Area,
                       CanvasSpecification&       Canvas,
                       bool&                      PointerConsumed)
{
    if (Area.Width <= 0.0f || Area.Height <= 0.0f)
        return;

    const LayoutExtents& Extents   = Theme.Extents;
    const ThemePalette&  Palette   = Theme.Palette;
    ImDrawList*          Recording = ImGui::GetForegroundDrawList();

    // 🔴 The desk's own background and not the panel's. This rectangle is where the display recording will land, and the
    //    space's zero is what an unattached canvas must read as — a panel tone here would leave the artist unable to tell a
    //    canvas awaiting an image from a canvas presenting a black one.
    Recording->AddRectFilled(Corner(Area), Opposite(Area), Coded(Palette.DeskBackground), Extents.CornerRounding);
    Recording->AddRect(Corner(Area), Opposite(Area), Coded(Palette.PanelBorder), Extents.CornerRounding,
                       0, Extents.BorderThickness);

    // 📝 🚧 Nothing is drawn into the rectangle and the lattice is not painted either. `08` §3.1 places the display
    //    recording **before** the interface, so what fills this is a target the schedule composites and not a quad recorded
    //    here — a lattice painted on the foreground list would sit on top of the very image it is meant to sit under.
    //    The three overlay declarations are carried and presented, and the recording that honours them is owed.
    char Extent[CaptionExtent] = {};

    ComposeExtentCaption(Canvas, Extent, CaptionExtent);

    const char*  Unfilled        = "no image is attached";
    const ImVec2 ExtentMeasured  = ImGui::CalcTextSize(Extent);
    const ImVec2 UnfilledMeasured = ImGui::CalcTextSize(Unfilled);

    const float CentreX = Area.PositionX + Area.Width * 0.5f;
    const float CentreY = Area.PositionY + Area.Height * 0.5f;

    Recording->AddText(ImVec2(CentreX - ExtentMeasured.x * 0.5f, CentreY - ExtentMeasured.y),
                       Coded(Palette.TextMuted), Extent);
    Recording->AddText(ImVec2(CentreX - UnfilledMeasured.x * 0.5f, CentreY + Extents.ControlSpacing * 0.5f),
                       Coded(Attenuate(Palette.TextMuted, 0.6)), Unfilled);

    // 📝 The canvas carries its own `(V)` at the top-right, inside its rectangle rather than in either band, because what it
    //    offers is what is drawn **over** the image and not what the image is.
    const WorkspaceRectangle Button =
        SquareAt(Area.PositionX + Area.Width - Extents.PanelPadding - Extents.GlyphButtonEdge,
                 Area.PositionY + Extents.PanelPadding,
                 Extents.GlyphButtonEdge);

    if (Area.Height > Extents.GlyphButtonEdge + Extents.PanelPadding * 2.0f)
        PresentCaretButton(Theme, Button, Canvas.CanvasOverlay, Canvas.PresentedTicks, PointerConsumed);

    // 📝 The canvas takes the pointer as much as either band does. A press that fell through to whatever sits beneath the
    //    box would start a stroke the artist aimed at an empty viewport.
    const ImGuiIO& Pointing = ImGui::GetIO();

    if (RectangleCovers(Area, Pointing.MousePos.x, Pointing.MousePos.y))
        PointerConsumed = true;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE FOOTER BAND
//------------------------------------------------------------------------------------------------------------------------

void PresentFooterBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       CanvasSpecification&       Canvas,
                       bool&                      PointerConsumed)
{
    if (Band.Width <= 0.0f || Band.Height <= 0.0f)
        return;

    const LayoutExtents& Extents   = Theme.Extents;
    const ThemePalette&  Palette   = Theme.Palette;
    ImDrawList*          Recording = ImGui::GetForegroundDrawList();

    Recording->AddRectFilled(Corner(Band), Opposite(Band), Coded(Palette.PanelHeader), Extents.CornerRounding,
                             ImDrawFlags_RoundCornersBottom);

    Recording->AddLine(ImVec2(Band.PositionX, Band.PositionY),
                       ImVec2(Band.PositionX + Band.Width, Band.PositionY),
                       Coded(Palette.PanelBorder), Extents.BorderThickness);

    char Readout[CaptionExtent] = {};
    char Segment[CaptionExtent] = {};

    if (Canvas.ExtentReadout)
    {
        ComposeExtentCaption(Canvas, Segment, CaptionExtent);
        AppendSegment(Readout, CaptionExtent, Segment);
    }

    if (Canvas.MagnificationReadout)
    {
        std::snprintf(Segment, CaptionExtent, "%.0f%%", static_cast<double>(Canvas.Magnification));
        AppendSegment(Readout, CaptionExtent, Segment);
    }

    if (Canvas.OffsetReadout)
    {
        std::snprintf(Segment, CaptionExtent, "%.0f, %.0f",
                      static_cast<double>(Canvas.OffsetAlong), static_cast<double>(Canvas.OffsetAcross));
        AppendSegment(Readout, CaptionExtent, Segment);
    }

    if (Canvas.RotationReadout)
    {
        std::snprintf(Segment, CaptionExtent, "%.0f°, %.0f°, %.0f°",
                      static_cast<double>(Canvas.RotationYaw),
                      static_cast<double>(Canvas.RotationPitch),
                      static_cast<double>(Canvas.RotationRoll));
        AppendSegment(Readout, CaptionExtent, Segment);
    }

    if (Readout[0] != '\0')
    {
        const ImVec2 Measured = ImGui::CalcTextSize(Readout);

        Recording->AddText(ImVec2(Band.PositionX + Extents.PanelPadding * 2.0f,
                                  Band.PositionY + (Band.Height - Measured.y) * 0.5f),
                           Coded(Palette.TextMuted), Readout);
    }

    const WorkspaceRectangle Button =
        SquareAt(Band.PositionX + Band.Width - Extents.TabInset - Extents.GlyphButtonSmallEdge,
                 Band.PositionY + (Band.Height - Extents.GlyphButtonSmallEdge) * 0.5f,
                 Extents.GlyphButtonSmallEdge);

    PresentCaretButton(Theme, Button, Canvas.FooterOverlay, Canvas.PresentedTicks, PointerConsumed);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                 WHAT EACH LIST OFFERS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Each row advances or toggles exactly one declaration and the caption is composed from the value it will change, so the
//    list reads as the current state rather than as a menu of verbs.
void ResolveHeaderList(const ThemeSpecification& Theme,
                       CanvasSpecification&      Canvas,
                       bool&                     PointerConsumed,
                       OverlayReach&             Reaching)
{
    char Projection[CaptionExtent] = {};
    char Extent[CaptionExtent]     = {};
    char ExtentRow[CaptionExtent]  = {};

    std::snprintf(Projection, CaptionExtent, "Projection:  %s", CaptionOf(Canvas.Projection));

    ComposeExtentCaption(Canvas, Extent, CaptionExtent);
    std::snprintf(ExtentRow, CaptionExtent, "Extent:  %s", Extent);

    const char* const Rows[2] = { Projection, ExtentRow };

    const std::int32_t Chosen =
        ResolveOverlay(Theme, Canvas.HeaderOverlay, Canvas.PresentedTicks, Rows, 2u, PointerConsumed, Reaching);

    if (Chosen == 0)
    {
        Canvas.Projection = Canvas.Projection == CanvasProjection::Perspective ? CanvasProjection::Orthographic
                                                                               : CanvasProjection::Perspective;
    }
    else if (Chosen == 1)
    {
        Canvas.ExtentOrdinal = (Canvas.ExtentOrdinal + 1u) % CanvasExtentCount;
    }
}

void ResolveCanvasList(const ThemeSpecification& Theme,
                       CanvasSpecification&      Canvas,
                       bool&                     PointerConsumed,
                       OverlayReach&             Reaching)
{
    char Lattice[CaptionExtent]   = {};
    char Axis[CaptionExtent]      = {};
    char Wireframe[CaptionExtent] = {};

    std::snprintf(Lattice,   CaptionExtent, "Lattice:  %s",         CaptionOf(Canvas.LatticeStanding));
    std::snprintf(Axis,      CaptionExtent, "Axis reference:  %s",  CaptionOf(Canvas.AxisStanding));
    std::snprintf(Wireframe, CaptionExtent, "Wireframe:  %s",       CaptionOf(Canvas.WireframeStanding));

    const char* const Rows[3] = { Lattice, Axis, Wireframe };

    const std::int32_t Chosen =
        ResolveOverlay(Theme, Canvas.CanvasOverlay, Canvas.PresentedTicks, Rows, 3u, PointerConsumed, Reaching);

    if (Chosen == 0)
        Canvas.LatticeStanding = !Canvas.LatticeStanding;
    else if (Chosen == 1)
        Canvas.AxisStanding = !Canvas.AxisStanding;
    else if (Chosen == 2)
        Canvas.WireframeStanding = !Canvas.WireframeStanding;
}

void ResolveFooterList(const ThemeSpecification& Theme,
                       CanvasSpecification&      Canvas,
                       bool&                     PointerConsumed,
                       OverlayReach&             Reaching)
{
    char Extent[CaptionExtent]        = {};
    char Magnification[CaptionExtent] = {};
    char Offset[CaptionExtent]        = {};
    char Rotation[CaptionExtent]      = {};

    std::snprintf(Extent,        CaptionExtent, "Extent:  %s",        CaptionOf(Canvas.ExtentReadout));
    std::snprintf(Magnification, CaptionExtent, "Magnification:  %s", CaptionOf(Canvas.MagnificationReadout));
    std::snprintf(Offset,        CaptionExtent, "Offset:  %s",        CaptionOf(Canvas.OffsetReadout));
    std::snprintf(Rotation,      CaptionExtent, "Rotation:  %s",      CaptionOf(Canvas.RotationReadout));

    const char* const Rows[4] = { Extent, Magnification, Offset, Rotation };

    const std::int32_t Chosen =
        ResolveOverlay(Theme, Canvas.FooterOverlay, Canvas.PresentedTicks, Rows, 4u, PointerConsumed, Reaching);

    if (Chosen == 0)
        Canvas.ExtentReadout = !Canvas.ExtentReadout;
    else if (Chosen == 1)
        Canvas.MagnificationReadout = !Canvas.MagnificationReadout;
    else if (Chosen == 2)
        Canvas.OffsetReadout = !Canvas.OffsetReadout;
    else if (Chosen == 3)
        Canvas.RotationReadout = !Canvas.RotationReadout;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

void PresentCanvas(const ThemeSpecification& Theme, WorkspaceRectangle Body, CanvasSpecification& Canvas)
{
    ++Canvas.PresentedTicks;

    const CanvasBands Bands = ResolveCanvasBands(Theme.Extents, Body);

    bool PointerConsumed = false;

    // ① the three lists resolve their input before any band paints ------------------------------------------------------
    OverlayReach HeaderReaching;
    OverlayReach CanvasReaching;
    OverlayReach FooterReaching;

    ResolveHeaderList(Theme, Canvas, PointerConsumed, HeaderReaching);
    ResolveCanvasList(Theme, Canvas, PointerConsumed, CanvasReaching);
    ResolveFooterList(Theme, Canvas, PointerConsumed, FooterReaching);

    // ② the bands paint from the back forward ----------------------------------------------------------------------------
    PresentHeaderBand(Theme, Bands.HeaderBand, Canvas, PointerConsumed);
    PresentCanvasArea(Theme, Bands.CanvasArea, Canvas, PointerConsumed);
    PresentFooterBand(Theme, Bands.FooterBand, Canvas, PointerConsumed);

    // ③ and the lists record above every one of them ----------------------------------------------------------------------
    PaintOverlayReach(Theme, HeaderReaching);
    PaintOverlayReach(Theme, CanvasReaching);
    PaintOverlayReach(Theme, FooterReaching);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE LEDGER SLOT
//------------------------------------------------------------------------------------------------------------------------

void PresentCanvasPanel(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, void* PresentContext)
{
    if (PresentContext == nullptr)
        return;

    PresentCanvas(Theme, Area, *static_cast<CanvasSpecification*>(PresentContext));
}

PanelSlot ResolveCanvasSlot(const char* PanelIdentifier, const char* PanelTitle, CanvasSpecification& Canvas)
{
    PanelSlot Declaring;

    Declaring.PanelIdentifier = PanelIdentifier;
    Declaring.PanelTitle      = PanelTitle;
    Declaring.DeclaredSide    = WorkspacePanelSide::Centre;
    Declaring.Present         = &PresentCanvasPanel;
    Declaring.PresentContext  = &Canvas;

    return Declaring;
}

}   // namespace Slate
