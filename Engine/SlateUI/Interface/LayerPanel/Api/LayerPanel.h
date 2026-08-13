//============================================================================================================================================
//                                                              LAYERPANEL.H
//============================================================================================================================================
// 🧩 The ordered content of one surface made visible — every row read from `SurfaceLayerSequence`, nothing held here.

#pragma once

#include "Contract/PrecisionContract.h"
#include "SlateDocument/Document/SurfaceLayerSequence/Api/SurfaceLayerSequence.h"
#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/WorkspaceSpace.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT THE PANEL CARRIES
//------------------------------------------------------------------------------------------------------------------------

// 📝 A body presents a handful of open folds at once, not a population. Bounded so the carry is a fixed extent and
//    one tick of presentation allocates nothing.
inline constexpr std::uint32_t LayerFoldCapacity = 64u;   // [-] - rows whose fold state is remembered at once

/// 🧩 What the panel carries between ticks — presentation only, and the caller owns all of it.
/// note  🔴 `56` §4 and `14` §4.1: sequence position is the sequence's answer and never this panel's. Nothing here
///        is an entry, an ordering or a coverage; the folds, the filter, the offset and the drag are layout, and
///        layout inside the document would make opening a fold an undoable edit.
/// note  ⚠️ The chosen row and the drag are carried as **sequence positions** and not as identities, so a carry
///        that outlives an append names a different entry than it did. Presentation only — a stale choice paints
///        the wrong row for one tick and amends nothing, where a stale identity would need a resolution every tick
///        to say the same thing.
/// tag   owning
struct LayerPanelCarry
{
    float          VisibleOffset                  = 0.0f;    // [px] - top of the presented span
    TextCarry      Filter                         = {};      // [-]  - the toolbar entry, wired and not decorative
    bool           FoldOpen[LayerFoldCapacity]    = {};      // [-]  - which rows have their properties open
    std::uint32_t  ChosenPosition                 = 0u;      // [-]  - the sequence position the artist chose
    bool           ChosenDeclared                 = false;   // [-]  - false leaves no row marked
    bool           ReorderOpen                    = false;   // [-]  - a reorder drag is running
    std::uint32_t  ReorderOrigin                  = 0u;      // [-]  - the position the drag took hold of
    std::uint32_t  ReorderLanding                 = 0u;      // [-]  - where a release would put it
};

/// 🧩 What the panel presents against — the sequence it reads and the carry it writes.
/// note  🔴 `14` §1's gate: the panel stores neither. A presented sequence holding its own row list drifts from the
///        sequence the moment an entry is withdrawn, and the artist sees a layer that no longer exists.
/// tag   nonallocating, nonthrowing
struct LayerPanelContext
{
    SurfaceLayerSequence*  Sequence = nullptr;   // [-] - `56` owns it; presence and order amended through its calls
    LayerPanelCarry*       Carry    = nullptr;   // [-] - the workspace's own storage
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT A ROW READS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The caption one content source is presented under.
/// note  🔴 Read from the declared source and never inferred from the entry's content — the same rule `86` §4.1
///        applies to a report class. An inferred caption is a presentation that disagrees with the document.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* CaptionOfSource(LayerContentSource Source);

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Presents one tick of the surface's ordered content into the rectangle the desk resolved for it.
/// in    Theme           [-]   read by const reference; no colour or extent is spelled in this component
/// in    Area            [px]  the interior the panel layer handed it, header band included
/// in    PresentContext  [-]   a `LayerPanelContext*`; a null context or a null sequence presents an empty state
/// note  🔴 Matches `PanelPresentRoutine` exactly so a workspace declares it into `PanelIndex` and the desk never
///        learns what a layer is.
/// note  ⚠️ Rows present topmost first. `Entries()` is bottom first — `56` §4 — so the presented ordinal is the
///        reverse of the sequence position, and the badge counts down the sequence while it counts up the panel.
///        Both numbers exist; the one the artist is shown is the panel's, and the one every call carries is the
///        sequence's.
/// note  📝 The reference layer model carries no group nesting, so no row is indented. `56` §4.1's nested sequences
///        are a distinct thing and are presented as one row, not as a fold — a nested sequence presented as an
///        indented run would let the artist reorder across a boundary the sequence does not have.
/// cost  🚩
/// tag   api, nonthrowing
void PresentLayerPanel(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Area,
                       void*                      PresentContext);

// 📐 Sequence positions, ordinals and counts are Exact integers. Coverage strength is presented at the tier `56`
//    declared for it and is never re-derived here; the Bounded entries are the rectangles the desk handed in.
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded, PrecisionGuarantee::Exact);

}   // namespace Slate
