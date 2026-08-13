//============================================================================================================================================
//                                                           CONTROLINTERIOR.H
//============================================================================================================================================
// 🧩 What the control translation units share — the recording, the pointer, and the shapes every control is assembled from.

#pragma once

#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"

#include "imgui.h"
#include "imgui_internal.h"

// 📝 🔴 A Source-only header, on the same footing as `WorkspaceStripInternal.h`. Nothing outside
//    `ControlPanel/Source/` includes it, which is what lets it name vendor spellings: `14` §7 bars them from a
//    public header, and this is not one.

// 📝 ⚠️ The internal header is included for the active identity alone — `GetActiveID`, `SetActiveID` and
//    `ClearActiveID` are declared there and nowhere else, and a numeric drag that left its track would stop
//    amending without them. Nothing else in this module reads the vendor's interior state.

namespace Slate
{
namespace ControlInterior
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     CODES AND GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

ImU32   Coded(const ThemeColour& Colour);
ImVec2  Corner(const WorkspaceRectangle& Area);
ImVec2  Opposite(const WorkspaceRectangle& Area);
ImVec2  Centre(const WorkspaceRectangle& Area);

WorkspaceRectangle  Inset(const WorkspaceRectangle& Area, float Margin);
WorkspaceRectangle  CentredBand(const WorkspaceRectangle& Area, float Height);
WorkspaceRectangle  LeftSlice(const WorkspaceRectangle& Area, float Width);
WorkspaceRectangle  RightSlice(const WorkspaceRectangle& Area, float Width);
WorkspaceRectangle  SquareIn(const WorkspaceRectangle& Area, float Edge);

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE POINTER
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The pointer as one tick sees it, read once so every control in a tick agrees on where it is.
struct PointerReading
{
    float  PositionX    = 0.0f;    // [px]
    float  PositionY    = 0.0f;    // [px]
    float  TravelX      = 0.0f;    // [px] - since the previous tick
    bool   PressBegan   = false;   // [-]  - the primary control went down this tick
    bool   PressHeld    = false;   // [-]  - it is down
    bool   PressEnded   = false;   // [-]  - it came up this tick
    bool   PressDoubled = false;   // [-]  - a double press landed this tick
};

PointerReading  ResolvePointer();
bool            PointerCovers(const PointerReading& Pointer, const WorkspaceRectangle& Area);

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE PRESS AND THE TRACK
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 A press with no hold — what a control that answers a click and never drags reports.
/// note  🔴 The release is only honoured where the press **began** over the same rectangle. A control that acted on
///        any release covering it would fire when a drag started elsewhere happened to end over it, and the defect
///        presents as a section collapsing because a slider release landed on its header.
/// note  ⚠️ This claims no active identity. Only a control that must keep amending after the pointer has left its
///        own rectangle needs one, and that control uses `ResolveTrack` instead.
ControlInteraction ResolvePress(const WorkspaceRectangle& Area);

/// 🧩 One grabbable track: reports the interaction and, while held, the fraction the pointer names.
/// note  🔴 The hold is identified by the vendor's active identity and not by "the pointer is down over me". A drag
///        that left the track would otherwise stop amending the moment it did, which is the defect where a slider
///        drops the reading if the artist's hand strays a few pixels below the row.
struct TrackHold
{
    ControlInteraction  Interaction = {};
    bool                HoldOpen    = false;   // [-] - this track owns the pointer
    float               Fraction    = 0.0f;    // [-] - where along it the pointer sits, bounded
};

/// 🧩 Resolves one track's hold, the claim keyed by an address the caller guarantees is stable across ticks.
TrackHold ResolveTrack(const WorkspaceRectangle& Area, const void* Anchor);

/// 🧩 Paints a track, its travelled fill and its knob at a declared fraction.
void PaintTrack(const ThemeSpecification&  Theme,
                const WorkspaceRectangle&  Area,
                float                      Fraction,
                bool                       FillTravelled,
                bool                       Held);

//------------------------------------------------------------------------------------------------------------------------
//                                                      SHARED PAINTING
//------------------------------------------------------------------------------------------------------------------------

ImDrawList* Recording();

void PaintFill(const WorkspaceRectangle& Area, const ThemeColour& Colour, float Rounding);
void PaintOutline(const WorkspaceRectangle& Area, const ThemeColour& Colour, float Rounding, float Thickness);
void PaintDisc(float CentreX, float CentreY, float Radius, const ThemeColour& Colour);

/// 🧩 Text clipped to a rectangle, aligned by two fractions — zero is left or top, one is right or bottom.
void PaintCaption(const WorkspaceRectangle&  Area,
                  const char*                Caption,
                  const ThemeColour&         Colour,
                  float                      HorizontalAlignment,
                  float                      VerticalAlignment,
                  float                      FontScale);

// 📝 The readout buffer every control prints into. Sixteen digits of a double plus a sign, a point and a terminator
//    do not reach this, and a fixed extent keeps every control on these paths allocation-free.
inline constexpr std::uint32_t ReadoutExtent = 40u;

/// 🧩 One reading bounded to a closed interval, the ends included.
double Bounded(double Reading, double Floor, double Ceiling);

/// 🧩 One reading printed to the declared number of decimals, into a caller-owned buffer.
/// note  ⚠️ Printed and never rounded in place. `02`'s tiers make the readout a presentation of the reading and
///        not a second authority over it; rounding the carry to what is shown loses precision the artist never
///        asked to lose.
void PrintReading(char* Destination, std::uint32_t DestinationExtent, double Reading, std::uint32_t Decimals);

/// 🧩 The value box — a black centre carrying the readout, capped by a side segment at one end or the other.
/// in    CapWidth  [px]  zero paints no cap
/// out   the rectangle the readout occupies, for a caller that wants to place a caret in it
WorkspaceRectangle PaintValueBox(const ThemeSpecification&  Theme,
                                 const WorkspaceRectangle&  Area,
                                 const char*                CapCaption,
                                 float                      CapWidth,
                                 bool                       CapLeading,
                                 const char*                Readout,
                                 bool                       Focused);

}   // namespace ControlInterior
}   // namespace Slate
