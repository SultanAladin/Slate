//============================================================================================================================================
//                                                        WORKSPACESTRIPINTERNAL.H
//============================================================================================================================================
// 🧩 What the desk's own translation units share — the strip, its geometry, and the intent a tick defers.

#pragma once

#include "SlateUI/Interface/WorkspaceSpace/Api/WorkspaceSpace.h"

#include "imgui.h"

#include <vector>

// 📝 🔴 A Source-only header. Nothing outside `WorkspaceSpace/Source/` includes it, which is what lets it name
//    vendor spellings freely: `14` §7 bars them from a public header, and this is not one. It exists because a
//    leaf and a floating window carry the same strip, and Frontier's two copies of that strip are two places
//    every trapezoid amendment has to land — with one of them missed each time.

namespace Slate
{
namespace StripInterior
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     CODES AND SHAPES
//------------------------------------------------------------------------------------------------------------------------

ImU32   Coded(const ThemeColour& Colour);
ImVec2  Corner(const WorkspaceRectangle& Area);
ImVec2  Opposite(const WorkspaceRectangle& Area);

WorkspaceRectangle  StripOf(const WorkspaceRectangle& Area, float StripHeight);
WorkspaceRectangle  BodyOf(const WorkspaceRectangle& Area, float StripHeight);
WorkspaceRectangle  SquareAt(float PositionX, float PositionY, float Edge);
WorkspaceRectangle  AreaOf(const WorkspaceFloatingWindow& Window);

void PaintTrapezoid(ImDrawList* Recording, const WorkspaceRectangle& Area, float Slant, ImU32 Code);
void PaintPlusStroke(ImDrawList* Recording, float CentreX, float CentreY, float Edge, ImU32 Code, float Thickness);
void PaintChevronStroke(ImDrawList* Recording, float CentreX, float CentreY, float Edge, ImU32 Code, float Thickness);
void PaintCrossStroke(ImDrawList* Recording, float CentreX, float CentreY, float Edge, ImU32 Code, float Thickness);
void PaintGripStroke(ImDrawList* Recording, const WorkspaceRectangle& Grip, ImU32 Code, float Thickness);

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT CARRIES A STRIP
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The rectangle a strip and its body occupy, and which of the two things owns it.
/// note  Exactly one of the two is set: a leaf names a pool index and no window, a floating window names a key and
///        no index. Both being absent is a carrier nothing can resolve an intent against.
struct StripCarrier
{
    WorkspaceRectangle  Area   = {};   // [px] - strip and body together
    std::int32_t        Link   = -1;   // [-]  - the leaf, or -1 for a window
    std::uint32_t       Window = 0u;   // [-]  - the window key, or 0 for a leaf
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     DEFERRED INTENT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every structural amendment one tick asked for, applied once the whole desk has been traversed.
/// note  🔴 Minting a document, docking one, or raising a window may grow or reorder the pools the traversal is
///        walking. Applying inside the traversal invalidates the very references it is iterating, and the defect
///        presents as a leaf painted at another leaf's rectangle rather than as a crash.
struct DeferredIntent
{
    bool                       MintDeclared     = false;   // [-] - a (+) entry was chosen
    std::uint32_t              MintOrdinal      = 0u;      // [-] - which entry
    std::int32_t               MintLink         = -1;      // [-] - the leaf it lands in

    bool                       WithdrawDeclared = false;   // [-] - a tab's (x) was pressed
    WorkspaceDocumentIdentity  WithdrawSubject  = {};      // [-] - which document

    bool                       ActivateDeclared = false;   // [-] - a tab was clicked
    WorkspaceDocumentIdentity  ActivateSubject  = {};      // [-] - which document
    std::int32_t               ActivateLink     = -1;      // [-] - the leaf carrying it
    std::uint32_t              ActivateWindow   = 0u;      // [-] - the window carrying it

    bool                       ReorderDeclared  = false;   // [-] - a held tab crossed a neighbour
    WorkspaceDocumentIdentity  ReorderSubject   = {};      // [-] - the held document
    std::int32_t               ReorderLink      = -1;      // [-] - the leaf it is reordering within
    std::uint32_t              ReorderWindow    = 0u;      // [-] - the window it is reordering within
    std::uint32_t              ReorderPosition  = 0u;      // [-] - the ordinal it takes

    bool                       RaiseDeclared    = false;   // [-] - a floating window was pressed
    std::uint32_t              RaiseWindow      = 0u;      // [-] - which window comes to the front
};

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE SHARED STRIP
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Paints one carrier's body and trapezoid strip and resolves every pointer intent the strip carries.
/// in    Occupants  [-]  read, never amended; every amendment is deferred
/// in    Active     [-]  the occupant whose body is presented
/// note  🔴 Nothing here mutates the desk's structure. The one field it writes directly is the pending press,
///        which is not structure — it is the record that lets a click activate and only travel tear.
void PresentOccupantStrip(const ThemeSpecification&                     Theme,
                          WorkspaceSpace&                               Space,
                          const StripCarrier&                           Carrier,
                          const std::vector<WorkspaceDocumentIdentity>& Occupants,
                          WorkspaceDocumentIdentity                     Active,
                          DeferredIntent&                               Arriving,
                          bool&                                         PointerConsumed);

//------------------------------------------------------------------------------------------------------------------------
//                                                THE DRAG AND THE WINDOWS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Turns a travelled press into a drag, advances the one in flight, and applies it on release.
/// in    DeskArea  [px]  needed because an applied landing re-divides the desk and the layout must follow it
/// pre   ResolveSpaceLayout ran this tick
/// post  the layout is current again whenever this amended the tree
void AdvanceWorkspaceDrag(const ThemeSpecification& Theme,
                          WorkspaceSpace&           Space,
                          WorkspaceRectangle        DeskArea,
                          bool&                     PointerConsumed);

/// 🧩 Paints every floating window, topmost last, and resolves the input of the topmost one under the pointer.
void PresentFloatingWindows(const ThemeSpecification& Theme,
                            WorkspaceSpace&           Space,
                            DeferredIntent&           Arriving,
                            bool&                     PointerConsumed);

/// 🧩 The topmost floating window covering a point, or zero.
std::uint32_t LocateWindowCovering(const WorkspaceSpace& Space, float PointerX, float PointerY);

/// 🧩 Washes the resolved landing in the accent, above everything else the tick painted.
/// note  🔴 A preview and never an application. The desk is rearranged on release, so a drag abandoned by dragging
///        back out of every band leaves the desk exactly as it was found.
void PaintDragPreview(const ThemeSpecification& Theme, const WorkspaceSpace& Space);

}   // namespace StripInterior
}   // namespace Slate
