//============================================================================================================================================
//                                                        WORKSPACESTRIPINTERNAL.H
//============================================================================================================================================
// 🧩 What the desk's own translation units share — the strip, its geometry, and the intent a tick defers.

#pragma once

#include "SlateUI/Interface/WorkspaceSpace/Api/PanelIndex.h"
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

    // 📝 A panel withdrawal is deferred with the rest. Releasing the record inside the walk that is presenting it
    //    would leave the placement the walk is iterating naming a record nothing occupies.
    bool                       PanelWithdrawDeclared = false;   // [-] - a panel box's (x) was pressed
    WorkspaceDocumentIdentity  PanelWithdrawBody     = {};      // [-] - the body holding it
    WorkspacePanelIdentity     PanelWithdrawSubject  = {};      // [-] - which box

    bool                       PanelRaiseDeclared    = false;   // [-] - a floating panel box was pressed
    WorkspaceDocumentIdentity  PanelRaiseBody        = {};      // [-] - the body holding it
    WorkspacePanelIdentity     PanelRaiseSubject     = {};      // [-] - which box comes to the front
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
/// in    Panels  [-]  the active workspace's ledger, threaded through to each window's own panel layer, or null
/// note  🔴 A torn-out window carries the same body a leaf does, so it carries the same panel layer. Presenting
///        panels only in leaves would make tearing a tab out of the desk silently discard its panel arrangement.
void PresentFloatingWindows(const ThemeSpecification& Theme,
                            WorkspaceSpace&           Space,
                            const PanelIndex*         Panels,
                            DeferredIntent&           Arriving,
                            bool&                     PointerConsumed);

/// 🧩 The topmost floating window covering a point, or zero.
std::uint32_t LocateWindowCovering(const WorkspaceSpace& Space, float PointerX, float PointerY);

/// 🧩 Washes the resolved landing in the accent, above everything else the tick painted.
/// note  🔴 A preview and never an application. The desk is rearranged on release, so a drag abandoned by dragging
///        back out of every band leaves the desk exactly as it was found.
void PaintDragPreview(const ThemeSpecification& Theme, const WorkspaceSpace& Space);

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PANEL LAYER
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Presents one body's panel boxes — docked beneath, floating above — and resolves the input each one carries.
/// in    Presented  [-]   the document whose body this is; read here, amended only through the deferred intent
/// in    Body       [px]  the body rectangle, beneath the leaf's own tab strip
/// in    Panels     [-]   the active workspace's ledger, or null where none is declared
/// note  🔴 A panel's own present routine is handed the interior rectangle and nothing else. A routine that resolved
///        its own extent would paint at the depth the band carried before the drag rather than the one it carries.
/// note  ⚠️ The centre remainder is not painted here. It is what the document's own content occupies, and a fill
///        recorded over it would sit on top of whatever the workspace draws into the body.
void PresentPanelLayer(const ThemeSpecification&  Theme,
                       WorkspaceSpace&            Space,
                       WorkspaceDocumentIdentity  Presented,
                       WorkspaceRectangle         Body,
                       const PanelIndex*          Panels,
                       DeferredIntent&            Arriving,
                       bool&                      PointerConsumed);

/// 🧩 One tick of a panel move, a panel resize or a band re-share, and the landing a release would take.
/// note  🔴 The three panel modes are advanced here rather than in `AdvanceWorkspaceDrag`'s switch, because each
///        needs the body rectangle the box lives in and the desk's own drag record carries only the document.
void AdvancePanelDrag(const ThemeSpecification& Theme,
                      WorkspaceSpace&           Space,
                      float                     PointerX,
                      float                     PointerY);

/// 🧩 Applies a released panel drag, docking the held box to whichever side the preview named.
/// note  Only a move lands anything. A resize and a band drag amended the box on every tick they ran, so their
///        release has nothing left to apply.
void SealPanelDrag(const ThemeSpecification& Theme, WorkspaceSpace& Space);

/// 🧩 The body rectangle of one document, wherever it sits, or false when nothing carries it.
bool BodyCarrying(const WorkspaceSpace&     Space,
                  WorkspaceDocumentIdentity Subject,
                  float                     StripHeight,
                  WorkspaceRectangle&       Resolved);

}   // namespace StripInterior
}   // namespace Slate
