//============================================================================================================================================
//                                                             OUTLINERPANEL.H
//============================================================================================================================================
// 🧩 Presents RowSequence through RankIndex and writes intent back — holding no relation of its own.

#pragma once

#include "Contract/PrecisionContract.h"
#include "SlateDocument/Document/OutlinerSequence/Api/OutlinerSequence.h"
#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/WorkspaceSpace.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                               THE SEARCH ENTRY EXTENT
//------------------------------------------------------------------------------------------------------------------------

// 📝 The sought text is held as a fixed extent rather than a growing string because the interface writes into
//    it directly every tick. A name longer than this narrows on its first sixty-three characters and is then
//    confirmed exactly against the whole name, so the extent bounds the entry and never the answer.
inline constexpr std::uint32_t NameSearchExtent = 64u;   // [-] - characters the search entry accepts, terminator included

//------------------------------------------------------------------------------------------------------------------------
//                                                  WHAT THE PANEL CARRIES
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What the panel carries between ticks — presentation only, and the caller owns all of it.
/// note  🔴 `12` §7 and `14` §6: the rows are read through `RankIndex` and the relations are never read here.
///        Only the counted span the artist can see is touched, and the visible position is a row ordinal
///        resolved by count rather than a pixel offset the panel remembers on its own.
/// note  🔴 Every mutation leaves through `OutlinerSequence::Declare` and is applied at the next tick's ①.
///        A panel that mutated the relations where the click arrived would apply against a linearisation that
///        is halfway rebuilt, and would bypass the sequence that undoes it.
/// note  ⚠️ What is held here is what `14` §4.1 places beside the document — the visible offset, the search
///        entry, and the drag in flight. None of it is a transaction and none of it is scrubbed.
/// note  ⚠️ The anchor is carried as an **identity** and not as an ordinal, unlike `LayerPanelCarry`'s chosen
///        position. A row's ordinal changes whenever an enclosure above it collapses, which is the one event
///        this carry exists to absorb; a stale ordinal would move the view on a collapse made elsewhere.
/// tag   owning
struct OutlinerPanelCarry
{
    float             VisibleOffset                 = 0.0f;   // [px] - top of the presented span
    TextCarry         Sought                        = {};     // [-]  - the search entry, wired and not decorative
    std::uint32_t     ConfirmedCount                = 0u;     // [-]  - names the last narrowing confirmed
    std::uint32_t     VisibleAnchor                 = 0u;     // [-]  - counted ordinal at the top of the span
    std::uint32_t     CountedWhenAnchored           = 0u;     // [-]  - counted total the anchor was observed at
    std::uint32_t     RowsPresented                 = 0u;     // [-]  - rows the last presentation touched
    OccupantIdentity  AnchoredOccupant              = {};     // [-]  - who the span is anchored on, not where
    OccupantIdentity  DraggedOccupant               = {};     // [-]  - what a reorder drag took hold of
    OccupantIdentity  LandingOccupant               = {};     // [-]  - the enclosure a release would declare
    bool              ReorderOpen                   = false;  // [-]  - a reorder drag is running
};

//------------------------------------------------------------------------------------------------------------------------
//                                                   WHAT IT PRESENTS AGAINST
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What the panel presents against — the sequence it reads and declares into, and the carry it writes.
/// note  🔴 `14` §1's gate: the panel stores neither relation nor row. A presented outliner holding its own row
///        list drifts from the linearisation the moment an occupant is retired, and the artist sees a row for
///        something that no longer exists.
/// tag   nonallocating, nonthrowing
struct OutlinerPanelContext
{
    OutlinerSequence*    Outliner = nullptr;   // [-] - `12` owns it; intent declared into it, never applied here
    OutlinerPanelCarry*  Carry    = nullptr;   // [-] - the workspace's own storage
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Presents one tick of the outliner into the rectangle the desk resolved, and declares whatever was asked.
/// in    Theme          [-]   read for the palette and the band extents; never held
/// in    Area           [px]  the interior the desk resolved for this panel, honoured exactly
/// in    PresentContext [-]   an `OutlinerPanelContext*`; a null context presents an empty state
/// post  every declared intent sits in the pending run; nothing was applied here
/// note  🔴 Matches `PanelPresentRoutine` exactly, so a workspace declares it into `PanelIndex` and the desk
///        never learns what a row, an enclosure or a subset is.
/// note  🔴 The rectangle is the desk's answer and this panel asks no second one. A panel that opened a window
///        of its own would paint at the depth its band carried before a drag rather than the one it carries
///        now, and the two answers disagree the first tick the band is moved.
/// note  ⚠️ Nothing here refuses. `14` §7 puts the interface context's existence on the desk that calls this,
///        and a refusal raised per panel per tick would append to the register on a path that cannot arise.
/// cost  🚩
/// tag   api, nonthrowing
void PresentOutlinerPanel(const ThemeSpecification&  Theme,
                          const WorkspaceRectangle&  Area,
                          void*                      PresentContext);

// 📐 Ordinals, counts and identities are Exact integers. Rectangles, offsets and row pitches are Bounded. The
//    component claims Bounded, per `00` §3's transitivity rule.
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded, PrecisionGuarantee::Exact);

}   // namespace Slate
