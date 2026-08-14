//============================================================================================================================================
//                                                       TEXTUREPAINTSPECIFICATION.H
//============================================================================================================================================
// 🧩 The one concrete workspace — six panels declared into a ledger, and the storage every one of them presents against.

#pragma once

#include "Contract/PrecisionContract.h"
#include "SlateUI/Interface/ChannelPanel/Api/ChannelPanel.h"
#include "SlateUI/Interface/DiagnosticPanel/Api/DiagnosticPanel.h"
#include "SlateUI/Interface/LayerPanel/Api/LayerPanel.h"
#include "SlateUI/Interface/OutlinerPanel/Api/OutlinerPanel.h"
#include "SlateUI/Interface/PropertyPanel/Api/PropertyPanel.h"
#include "SlateUI/Interface/RevisionPanel/Api/RevisionPanel.h"
#include "SlateUI/Interface/WorkspaceSequence/Api/WorkspaceSequence.h"

#include <cstdint>

namespace Slate
{

// 📝 `WorkspaceSpecification` is declared in `WorkspaceSequence.h`, which is why the whole bring-up header arrives
//    here. That header is the one an application already includes to construct the host, so nothing reaches a host
//    through this file that was not reaching it anyway — and the alternative, a second declaration of the roster
//    record, is the redefinition `PanelIndex.h` records at its own top.

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE WORKSPACE STORAGE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Everything the texture-painting workspace presents against, owned by the application and never by the engine.
/// note  🔴 `32` §5: a host declares one of these as a stack local and threads its address through
///        `WorkspaceSpecification::WorkspaceContext`. Nothing beneath `SlateUI` learns this record exists, which is
///        what lets `PaintHost` carry one roster entry and `EditorHost` carry every one from the same tree.
/// note  🔴 The seven document pointers are borrowed. This record owns the **carries** and nothing else — `14` §4.1
///        puts layout beside the document, and a workspace that owned a sequence would make closing a workspace a
///        destructive edit rather than a change of what is presented.
/// note  ⚠️ The six panel contexts are members rather than locals because the ledger retains their addresses for
///        as long as the workspace is standing. A context built inside `DeclareTexturePaintPanels` would be a dead
///        extent by the first tick that presented it.
/// note  📝 `Reports` and `Measures` are const because `DiagnosticPanel` reads them and nothing here appends. The
///        register the bring-up writes into is `WorkspaceSequence`'s own, handed in by the host.
/// tag   owning
struct PaintWorkspaceContext
{
    SurfaceLayerSequence*   Layers          = nullptr;   // [-] - `56` owns it; the layer rows read it
    MaterialIndex*          Materials       = nullptr;   // [-] - `42` owns it; the channel rows read it
    PropertyIndex*          Declarations    = nullptr;   // [-] - `10` owns it; the property rows read it
    RevisionSequence*       Revisions       = nullptr;   // [-] - `84` owns it; scrubbed through its own calls
    const ReportSequence*   Reports         = nullptr;   // [-] - `86`'s register, read under its own guard
    const MeasureIndex*     Measures        = nullptr;   // [-] - sampled by the tick, never pushed
    OutlinerSequence*       Outlined        = nullptr;   // [-] - `12` owns it; intent declared into it
    std::uint32_t           MaterialOrdinal = 0u;        // [-] - which material the channel rows resolve

    OutlinerPanelCarry      OutlinerCarry   = {};        // [-] - the anchor, the search entry and the reorder drag
    LayerPanelCarry         LayerCarry      = {};        // [-] - folds, filter, offset and the reorder drag
    ChannelPanelCarry       ChannelCarry    = {};        // [-] - sections, pickers and the last refusal
    PropertyPanelCarry      PropertyCarry   = {};        // [-] - text carries, dropdowns and the edit report
    RevisionPanelCarry      RevisionCarry   = {};        // [-] - folds, the saved position and the discard prompt
    DiagnosticPanelCarry    DiagnosticCarry = {};        // [-] - which class sections are open

    OutlinerPanelContext    DeclaredOutlinerContext   = {};   // [-] - addressed by the ledger, never by a caller
    LayerPanelContext       DeclaredLayerContext      = {};   // [-]
    ChannelPanelContext     DeclaredChannelContext    = {};   // [-]
    PropertyPanelContext    DeclaredPropertyContext   = {};   // [-]
    RevisionPanelContext    DeclaredRevisionContext   = {};   // [-]
    DiagnosticPanelContext  DeclaredDiagnosticContext = {};   // [-]
};

//------------------------------------------------------------------------------------------------------------------------
//                                                THE PANEL DECLARATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Declares the workspace's six panels into the ledger the sequence handed it.
/// in    Ledger           [-]  emptied by the caller before this runs; six declarations arrive
/// in    WorkspaceContext [-]  a `PaintWorkspaceContext*`; a null context declares nothing at all
/// post  the ledger carries one slot per panel, each addressing a context this record owns
/// note  🔴 Matches `WorkspaceDeclareRoutine` exactly, so `WorkspaceSequence` calls it at activation and learns
///        nothing about what a layer, a channel or a revision is.
/// note  🔴 The six panel contexts are rebuilt from the document pointers **on every activation** rather than once
///        at construction. A host that swaps the material ordinal or opens a second document between activations
///        would otherwise present the panels against whatever was standing when the record was first filled.
/// note  ⚠️ A declared side is where the workspace asks a panel to sit and never where the artist put it —
///        `WorkspaceSpace::DeclarePanelBox` lands every box floating regardless.
/// cost  🚩
/// tag   api, nonallocating, nonthrowing
void DeclareTexturePaintPanels(PanelIndex& Ledger, void* WorkspaceContext);

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE WORKSPACE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The roster entry a host registers for texture painting.
/// in    Standing  [-]  the storage, whose address the entry carries; it must outlive the host
/// out   Entry     [-]  caption, stem, discipline and the declaration routine, filled and ready to register
/// note  🔴 `PresentCentre` is left null deliberately. The centre of a painting workspace is the composited image
///        `RenderSchedule` put beneath the interface, and a routine painting over it here would hide the surface
///        the workspace exists to present.
/// note  ⚠️ The returned entry points at `Standing`. Registering an entry built from a temporary is the dangling
///        roster `WorkspaceSpecification` warns about, and it presents as panels drawn against released storage.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
WorkspaceSpecification ResolveTexturePaintWorkspace(PaintWorkspaceContext& Standing);

// 📐 Ordinals, counts and slot positions are Exact integers. Every rectangle the panels are handed is Bounded and
//    none of them is derived here. The component claims Bounded, per `00` §3's transitivity rule.
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded, PrecisionGuarantee::Exact);

}   // namespace Slate
