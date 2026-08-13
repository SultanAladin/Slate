//============================================================================================================================================
//                                                              PANELINDEX.H
//============================================================================================================================================
// 🧩 The slot ledger a workspace fills at activation and the desk walks each tick — the host names no concrete panel.

#pragma once

#include "Contract/OutcomeContract.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/WorkspaceSpace.h"

#include <cstdint>

namespace Slate
{

// 📝 🔴 `WorkspacePanelSide` is declared once, in `WorkspaceSpace.h`, beside the `WorkspacePanelBox` that anchors
//    against it. A second identical declaration here compiled only while nothing included both headers; the first
//    workspace that declares panels and addresses the desk includes both, and a redefinition is not an ODR
//    allowance the way two identical class definitions in two translation units are — it is an error at the
//    second declaration. A panel names a side and never a rectangle either way.

//------------------------------------------------------------------------------------------------------------------------
//                                                      ONE DECLARED PANEL
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The routine one panel presents through, handed the resolved theme, the rectangle it was given, and the
///     context its workspace threads in.
/// note  🔴 `14` §1: the theme arrives by const reference so no panel can spell a colour or an extent of its own,
///        and the context is opaque so one shared outliner routine serves every workspace without naming any.
/// note  🔴 The rectangle is a parameter and never something the panel resolves for itself. `2e` places a panel in
///        a band whose depth the artist drags, so a panel that read a rectangle from anywhere else would paint at
///        the extent the band had before the drag — and the defect presents as a panel that lags its own frame.
/// tag   contract
using PanelPresentRoutine = void (*)(const ThemeSpecification& Theme,
                                     const WorkspaceRectangle& Area,
                                     void*                     PresentContext);

/// 🧩 One panel as a workspace declares it — what it is called, where it sits, and how it presents.
/// note  🔴 The context is the workspace's own storage and this ledger never owns it. A slot that owned what its
///        panel presents would be the `14` §1 defect arriving through the ledger rather than through the panel.
/// note  ⚠️ Both text pointers name static storage. Nothing here is copied, so a declaration built from a local
///        buffer leaves the ledger pointing at a dead extent the first time the declaring scope closes.
/// tag   contract, nonallocating, nonthrowing
struct PanelSlot
{
    const char*          PanelIdentifier = nullptr;                          // [-] - stable, unique; never empty
    const char*          PanelTitle      = nullptr;                          // [-] - what the header band shows
    WorkspacePanelSide   DeclaredSide    = WorkspacePanelSide::Floating;     // [-] - the side, never a rectangle
    PanelPresentRoutine  Present         = nullptr;                          // [-] - one tick of the panel body
    void*                PresentContext  = nullptr;                          // [-] - the workspace's own storage
};

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE LEDGER
//------------------------------------------------------------------------------------------------------------------------

// 📝 A workspace declares a handful of panels, not an open-ended population, so the ledger is a fixed extent and
//    never allocates. `00` forbids new/delete outside an extent slicer and this is neither.
inline constexpr std::uint32_t PanelSlotCapacity = 16u;   // [-] - declared panels one workspace may hold

/// 🧩 Every panel one workspace declared, in declaration order.
/// note  Declaration order is presentation order within a side, which is the only ordering the ledger carries.
/// tag   owning
struct PanelIndex
{
    PanelSlot      DeclaredSlots[PanelSlotCapacity] = {};    // [-] - the declarations, in the order they arrived
    std::uint32_t  DeclaredCount                    = 0u;    // [-] - how many of them are occupied
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     LEDGER OPERATIONS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Returns the ledger to empty, releasing every declaration it held.
/// in    Ledger  [-]  the ledger to empty
/// post  DeclaredCount is zero and no slot names a context
/// note  What a workspace calls at deactivation, and before rebuilding its panel set. The contexts are the
///       workspace's own and are untouched — this releases the ledger's claim, never the storage behind it.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
void ReclaimPanelIndex(PanelIndex& Ledger);

/// 🧩 Appends one panel declaration to the ledger.
/// in    Ledger     [-]  the ledger to declare into
/// in    Declaring  [-]  the slot, its text pointers retained and never copied
/// out   Outcome    [-]  refuses with ContentUnsupported when the slot names no identifier or no present routine,
///                       and with ExtentExhausted when the ledger already holds PanelSlotCapacity declarations
/// post  a delivered declaration resolves for its side; a refused one leaves the ledger exactly as it was
/// note  🔴 Frontier's equivalent ignored the seventeenth registration silently, and the panel that vanished was
///        indistinguishable from one whose present routine never drew. A refusal names which failure occurred.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
Outcome<bool> DeclarePanel(PanelIndex& Ledger, const PanelSlot& Declaring);

/// 🧩 Resolves the first panel declared for one side.
/// in    Ledger        [-]  the ledger to read
/// in    ResolvedSide  [-]  the side sought
/// out   Outcome       [-]  refuses with ContentUnsupported when no declaration names that side
/// note  First by declaration order, which is why order is the ledger's only ordering. A side sharing two panels
///        is resolved by `WorkspaceSpace` walking the ledger itself rather than by this call returning both.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
Outcome<PanelSlot> ResolvePanelForSide(const PanelIndex& Ledger, WorkspacePanelSide ResolvedSide);

/// 🧩 How many declarations the ledger holds.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
std::uint32_t DeclaredPanelCount(const PanelIndex& Ledger);

}   // namespace Slate
