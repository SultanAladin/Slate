//============================================================================================================================================
//                                                          THEMESPECIFICATION.CPP
//============================================================================================================================================
// 🧩 The one translation unit in which an interface colour literal and a layout extent literal are permitted.

#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"

#include "imgui.h"

#include <cmath>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   COORDINATE ASSEMBLY
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📝 The palette below is authored in the eight-bit code an artist reads off a design surface, and is carried into a
//    display-space coordinate here. The division is by 255 and not by 256: the code 255 is the space's unity, and
//    dividing by 256 leaves white sitting a fraction below it, which is visible where white text meets a white fill.
constexpr double CodeCeiling = 255.0;   // [-] - the largest eight-bit code, and the coordinate of unity

ThemeColour AuthoredColour(int RedCode, int GreenCode, int BlueCode, int CoverageCode)
{
    ThemeColour Authored;

    Authored.Coordinate.RedCoordinate   = static_cast<double>(RedCode)   / CodeCeiling;
    Authored.Coordinate.GreenCoordinate = static_cast<double>(GreenCode) / CodeCeiling;
    Authored.Coordinate.BlueCoordinate  = static_cast<double>(BlueCode)  / CodeCeiling;
    Authored.Coordinate.SpaceIdentity   = DisplaySpaceIdentity;
    Authored.Coverage                   = static_cast<double>(CoverageCode) / CodeCeiling;

    return Authored;
}

// 📝 One coordinate to one eight-bit code, bounded rather than wrapped. The bound is applied before the multiply so
//    that a coordinate far outside the interval cannot overflow the conversion before it is ever compared.
std::uint32_t QuantizeCoordinate(double Coordinate)
{
    const double Bounded = Coordinate < 0.0 ? 0.0 : (Coordinate > 1.0 ? 1.0 : Coordinate);

    return static_cast<std::uint32_t>(std::lround(Bounded * CodeCeiling)) & 0xFFu;
}

}   // namespace

std::uint32_t Quantize(const ThemeColour& Colour)
{
    const std::uint32_t RedCode      = QuantizeCoordinate(Colour.Coordinate.RedCoordinate);
    const std::uint32_t GreenCode    = QuantizeCoordinate(Colour.Coordinate.GreenCoordinate);
    const std::uint32_t BlueCode     = QuantizeCoordinate(Colour.Coordinate.BlueCoordinate);
    const std::uint32_t CoverageCode = QuantizeCoordinate(Colour.Coverage);

    return RedCode | (GreenCode << 8u) | (BlueCode << 16u) | (CoverageCode << 24u);
}

ThemeColour Attenuate(ThemeColour Colour, double DeclaredCoverage)
{
    Colour.Coverage = DeclaredCoverage < 0.0 ? 0.0 : (DeclaredCoverage > 1.0 ? 1.0 : DeclaredCoverage);

    return Colour;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE DARK PALETTE
//------------------------------------------------------------------------------------------------------------------------

ThemePalette DeclaredDarkPalette()
{
    ThemePalette Palette;

    // 📝 A near-black ramp rather than a grey one: the desk is the space's zero, and the panel, control and hovered
    //    tones are four steps above it. Steps this small only separate at all because nothing above them is
    //    tone-mapped a second time — `14` §5 places the interface after the display projection, so what is authored
    //    here is what reaches the surface.
    Palette.DeskBackground     = AuthoredColour(  0,   0,   0, 255);
    Palette.PanelBackground    = AuthoredColour( 14,  14,  14, 255);
    Palette.PanelHeader        = AuthoredColour( 14,  14,  14, 255);
    Palette.PanelBorder        = AuthoredColour( 28,  28,  28, 255);
    Palette.ControlBackground  = AuthoredColour( 20,  20,  20, 255);
    Palette.ControlHovered     = AuthoredColour( 26,  26,  26, 255);
    Palette.ControlActive      = AuthoredColour( 31,  31,  31, 255);

    // 📝 A tile face is a control face by another name, so it takes the control's authored coordinate rather than a
    //    second literal identical to it. #141414 and #1a1a1a are already spelled above, bit for bit. Naming them
    //    apart lets a later density or contrast profile separate them; assigning them here keeps exactly one
    //    authored value until one actually does.
    Palette.TileBackground     = Palette.ControlBackground;
    Palette.TileHovered        = Palette.ControlHovered;

    // 📝 The row wash is white at a coverage no eight-bit code carries — 0.045 × 255 is 11.5, so quantising it
    //    would round the wash to one side and the reference row would read a shade off. Attenuate takes the
    //    fraction directly, which is what it exists for.
    Palette.RowHovered         = Attenuate(AuthoredColour(255, 255, 255, 255), 0.045);

    // 📝 The accent is near-white rather than a hue, so that a selected row reads as selected on a surface whose own
    //    content is arbitrarily coloured. A saturated accent competes with whatever the artist is painting.
    Palette.AccentPrimary      = AuthoredColour(232, 232, 232, 255);
    Palette.AccentSubtle       = AuthoredColour(255, 255, 255,  31);
    Palette.SelectionMarker    = AuthoredColour( 74, 144, 226, 255);

    // 📝 🔴 The one hue in the palette, and it is reserved for a destructive control. `84` §3.1's discard is the
    //    reason it exists: the artist must be able to tell that control from every other one at a glance, and a
    //    near-white accent cannot carry that distinction.
    Palette.DangerPrimary      = AuthoredColour(224,  90,  90, 255);

    Palette.TextPrimary        = AuthoredColour(237, 237, 237, 255);
    Palette.TextMuted          = AuthoredColour(138, 138, 138, 255);
    Palette.TextOnAccent       = AuthoredColour( 17,  17,  17, 255);

    // 📝 The numeric entry is three segments — a grey axis cap, a black centre holding the number, a grey unit cap.
    //    The centre is the space's zero so the readout reads as an editable field rather than as a label.
    Palette.ValueNumberSegment = AuthoredColour(  0,   0,   0, 255);
    Palette.ValueSideSegment   = AuthoredColour( 20,  20,  20, 255);
    Palette.ValueOutline       = AuthoredColour(255, 255, 255,  56);
    Palette.ValueText          = AuthoredColour(255, 255, 255, 255);

    Palette.SliderTrack        = AuthoredColour( 20,  20,  20, 255);
    Palette.SliderFill         = AuthoredColour( 90,  90,  90, 255);
    Palette.SliderKnob         = AuthoredColour(255, 255, 255, 255);
    Palette.KnobText           = AuthoredColour( 17,  17,  17, 255);

    return Palette;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DEFAULT EXTENTS
//------------------------------------------------------------------------------------------------------------------------

LayoutExtents DeclaredDefaultExtents()
{
    // 📝 The member initialisers carry the authored extents. Returning a default-constructed value here rather than
    //    re-listing them keeps one copy of every number: a second list in this file would drift from the header's
    //    the first time either was amended, and the header's is what a reader consults.
    return LayoutExtents{};
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE ACTIVE THEME
//------------------------------------------------------------------------------------------------------------------------

Outcome<ThemeSpecification> ResolveActiveTheme(float DeclaredScale)
{
    if (!(DeclaredScale > 0.0f))
    {
        return Outcome<ThemeSpecification>::Refuse(
            { RefusalReason::ContentUnsupported, "the declared interface scale is not above zero" });
    }

    ThemeSpecification Resolved;

    Resolved.Palette = DeclaredDarkPalette();
    Resolved.Extents = DeclaredDefaultExtents();

    // 📝 Exactly the extents carrying pixels are scaled. The ratios and the font scales are dimensionless and a
    //    density change must leave them alone — scaling LabelColumnRatio past one gives the label the whole row.
    Resolved.Extents.PanelPadding         *= DeclaredScale;
    Resolved.Extents.ControlSpacing       *= DeclaredScale;
    Resolved.Extents.CardGap              *= DeclaredScale;
    Resolved.Extents.ControlHeight        *= DeclaredScale;
    Resolved.Extents.RowHeight            *= DeclaredScale;
    Resolved.Extents.LayerRowHeight       *= DeclaredScale;
    Resolved.Extents.RevisionCardHeight   *= DeclaredScale;
    Resolved.Extents.PanelHeaderHeight    *= DeclaredScale;
    Resolved.Extents.PanelFooterHeight    *= DeclaredScale;
    Resolved.Extents.SectionHeaderHeight  *= DeclaredScale;
    Resolved.Extents.ViewportBandBottom   *= DeclaredScale;
    Resolved.Extents.TabStripHeight       *= DeclaredScale;
    Resolved.Extents.TabSlant             *= DeclaredScale;
    Resolved.Extents.TabInset             *= DeclaredScale;
    Resolved.Extents.TabUnderline         *= DeclaredScale;
    Resolved.Extents.GutterThickness      *= DeclaredScale;
    Resolved.Extents.GlyphButtonEdge      *= DeclaredScale;
    Resolved.Extents.OverlayRowHeight     *= DeclaredScale;
    Resolved.Extents.IndentWidth          *= DeclaredScale;
    Resolved.Extents.CornerRounding       *= DeclaredScale;
    Resolved.Extents.BorderThickness      *= DeclaredScale;
    Resolved.Extents.LabelColumnWidth     *= DeclaredScale;
    Resolved.Extents.ValueColumnWidth     *= DeclaredScale;
    Resolved.Extents.LabelColumnGap       *= DeclaredScale;
    Resolved.Extents.EntryRowHeight       *= DeclaredScale;
    Resolved.Extents.SideSegmentWidth     *= DeclaredScale;
    Resolved.Extents.AxisSegmentWidth     *= DeclaredScale;
    Resolved.Extents.NumericEntryWidth    *= DeclaredScale;
    Resolved.Extents.SliderTrackHeight    *= DeclaredScale;
    Resolved.Extents.SliderKnobEdge       *= DeclaredScale;
    Resolved.Extents.SwitchWidth          *= DeclaredScale;
    Resolved.Extents.SwitchHeight         *= DeclaredScale;
    Resolved.Extents.SwitchNubEdge        *= DeclaredScale;
    Resolved.Extents.PillRounding         *= DeclaredScale;
    Resolved.Extents.SegmentRowHeight     *= DeclaredScale;
    Resolved.Extents.DropdownHeight       *= DeclaredScale;
    Resolved.Extents.DropdownCaretWidth   *= DeclaredScale;
    Resolved.Extents.ColourCircleEdge     *= DeclaredScale;
    Resolved.Extents.GlyphButtonSmallEdge *= DeclaredScale;
    Resolved.Extents.GlyphEdge            *= DeclaredScale;

    // 📝 The drawer's extents scale with everything else, and so do its two velocity thresholds. The pointer's own
    //    delta arrives in the coordinate space the interface paints in, which is the space the scale enlarges — a
    //    flick threshold left unscaled would need twice the hand movement at twice the density to read as a flick.
    Resolved.Extents.DrawerGripWidth      *= DeclaredScale;
    Resolved.Extents.DrawerGripHeight     *= DeclaredScale;
    Resolved.Extents.DrawerFinalizeOffset *= DeclaredScale;
    Resolved.Extents.DrawerFlickVelocity  *= DeclaredScale;
    Resolved.Extents.DrawerBodyClearance  *= DeclaredScale;
    Resolved.Extents.DrawerContentWidth   *= DeclaredScale;
    Resolved.Extents.DrawerContentInset   *= DeclaredScale;
    Resolved.Extents.DrawerContentPadding *= DeclaredScale;
    Resolved.Extents.DrawerFrameMinimum   *= DeclaredScale;
    Resolved.Extents.DrawerRevealCeiling  *= DeclaredScale;
    Resolved.Extents.SlideOffsetRest      *= DeclaredScale;
    Resolved.Extents.SlideVelocityRest    *= DeclaredScale;

    // 📝 🔴 The spring's own constants are **not** scaled and neither are the two scrim coverages, the reveal
    //    fraction, the curve subdivision or the velocity blend. Stiffness is [1/s²] and damping [1/s]: scaling them
    //    would retune the settle at every density, and a coverage or a fraction scaled past one has no meaning at all.

    // 📝 EntryRounding is deliberately not scaled. It is declared far beyond half of any row height so that the
    //    entry is fully rounded at every density; scaling it would only move it further beyond a bound it already
    //    exceeds, and the vendor bounds it to half the height when it draws.

    // 📝 ⚠️ CarouselTravel is seconds and is not scaled either. A density change must not slow an animation down —
    //    the two are unrelated quantities, and the only thing they share is this struct.

    return Outcome<ThemeSpecification>::Deliver(Resolved);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE STYLE MIRROR
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📝 The vendor's style holds a straight-coverage quadruple in the same display space the palette is authored in, so
//    this is a re-layout and not a projection. `14` §5 forbids a transfer here and none is applied.
ImVec4 StyleCoordinate(const ThemeColour& Colour)
{
    return ImVec4(static_cast<float>(Colour.Coordinate.RedCoordinate),
                  static_cast<float>(Colour.Coordinate.GreenCoordinate),
                  static_cast<float>(Colour.Coordinate.BlueCoordinate),
                  static_cast<float>(Colour.Coverage));
}

}   // namespace

Outcome<bool> Enforce(const ThemeSpecification& Theme)
{
    if (ImGui::GetCurrentContext() == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "no interface context is current" });

    ImGuiStyle& Styling = ImGui::GetStyle();

    Styling.WindowRounding    = Theme.Extents.CornerRounding;
    Styling.ChildRounding     = Theme.Extents.CornerRounding;
    Styling.FrameRounding     = Theme.Extents.CornerRounding;
    Styling.PopupRounding     = Theme.Extents.CornerRounding;
    Styling.ScrollbarRounding = Theme.Extents.CornerRounding;
    Styling.GrabRounding      = Theme.Extents.CornerRounding;
    Styling.TabRounding       = Theme.Extents.CornerRounding;

    Styling.WindowBorderSize  = Theme.Extents.BorderThickness;
    Styling.ChildBorderSize   = Theme.Extents.BorderThickness;
    Styling.FrameBorderSize   = 0.0f;
    Styling.PopupBorderSize   = Theme.Extents.BorderThickness;

    Styling.WindowPadding     = ImVec2(Theme.Extents.PanelPadding,   Theme.Extents.PanelPadding);
    Styling.FramePadding      = ImVec2(Theme.Extents.PanelPadding,   Theme.Extents.ControlSpacing * 0.5f);
    Styling.ItemSpacing       = ImVec2(Theme.Extents.ControlSpacing, Theme.Extents.ControlSpacing);
    Styling.ItemInnerSpacing  = ImVec2(Theme.Extents.ControlSpacing, Theme.Extents.ControlSpacing * 0.5f);
    Styling.IndentSpacing     = Theme.Extents.IndentWidth;

    const ThemePalette& Palette = Theme.Palette;

    Styling.Colors[ImGuiCol_WindowBg]             = StyleCoordinate(Palette.PanelBackground);
    Styling.Colors[ImGuiCol_ChildBg]              = StyleCoordinate(Palette.PanelBackground);
    Styling.Colors[ImGuiCol_PopupBg]              = StyleCoordinate(Palette.PanelBackground);
    Styling.Colors[ImGuiCol_Border]               = StyleCoordinate(Palette.PanelBorder);
    Styling.Colors[ImGuiCol_Separator]            = StyleCoordinate(Palette.PanelBorder);
    Styling.Colors[ImGuiCol_FrameBg]              = StyleCoordinate(Palette.ControlBackground);
    Styling.Colors[ImGuiCol_FrameBgHovered]       = StyleCoordinate(Palette.ControlHovered);
    Styling.Colors[ImGuiCol_FrameBgActive]        = StyleCoordinate(Palette.ControlActive);
    Styling.Colors[ImGuiCol_TitleBg]              = StyleCoordinate(Palette.PanelHeader);
    Styling.Colors[ImGuiCol_TitleBgActive]        = StyleCoordinate(Palette.PanelHeader);
    Styling.Colors[ImGuiCol_TitleBgCollapsed]     = StyleCoordinate(Palette.PanelHeader);
    Styling.Colors[ImGuiCol_MenuBarBg]            = StyleCoordinate(Palette.PanelHeader);
    Styling.Colors[ImGuiCol_Header]               = StyleCoordinate(Palette.ControlActive);
    Styling.Colors[ImGuiCol_HeaderHovered]        = StyleCoordinate(Palette.ControlHovered);
    Styling.Colors[ImGuiCol_HeaderActive]         = StyleCoordinate(Palette.AccentSubtle);
    Styling.Colors[ImGuiCol_Button]               = StyleCoordinate(Palette.ControlBackground);
    Styling.Colors[ImGuiCol_ButtonHovered]        = StyleCoordinate(Palette.ControlHovered);
    Styling.Colors[ImGuiCol_ButtonActive]         = StyleCoordinate(Palette.ControlActive);
    Styling.Colors[ImGuiCol_SliderGrab]           = StyleCoordinate(Palette.SliderKnob);
    Styling.Colors[ImGuiCol_SliderGrabActive]     = StyleCoordinate(Palette.AccentPrimary);
    Styling.Colors[ImGuiCol_CheckMark]            = StyleCoordinate(Palette.AccentPrimary);
    Styling.Colors[ImGuiCol_Text]                 = StyleCoordinate(Palette.TextPrimary);
    Styling.Colors[ImGuiCol_TextDisabled]         = StyleCoordinate(Palette.TextMuted);
    Styling.Colors[ImGuiCol_ScrollbarBg]          = StyleCoordinate(Palette.PanelBackground);
    Styling.Colors[ImGuiCol_ScrollbarGrab]        = StyleCoordinate(Palette.ControlBackground);
    Styling.Colors[ImGuiCol_ScrollbarGrabHovered] = StyleCoordinate(Palette.ControlHovered);
    Styling.Colors[ImGuiCol_ScrollbarGrabActive]  = StyleCoordinate(Palette.ControlActive);
    Styling.Colors[ImGuiCol_Tab]                  = StyleCoordinate(Palette.PanelHeader);
    Styling.Colors[ImGuiCol_TabHovered]           = StyleCoordinate(Palette.ControlHovered);
    Styling.Colors[ImGuiCol_TabSelected]          = StyleCoordinate(Palette.ControlActive);

    return Outcome<bool>::Deliver(true);
}

}   // namespace Slate
