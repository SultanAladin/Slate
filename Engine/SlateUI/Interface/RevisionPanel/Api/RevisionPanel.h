//============================================================================================================================================
//                                                            REVISIONPANEL.H
//============================================================================================================================================
// 🧩 The transaction sequence made visible and scrubbable — every row read from `RevisionSequence`, nothing held here.

#pragma once

#include "Contract/OutcomeContract.h"
#include "Contract/PrecisionContract.h"
#include "SlateDocument/Document/RevisionSequence/Api/RevisionSequence.h"
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
inline constexpr std::uint32_t RevisionFoldCapacity = 64u;   // [-] - rows whose fold state is remembered at once

/// 🧩 What the panel carries between ticks — presentation only, and the caller owns all of it.
/// note  🔴 `84` §1 and `14` §4.1: nothing here is a transaction and nothing here is scrubbed. The fold states,
///        the visible offset and the drag are layout, and layout inside the document would make opening a fold an
///        undoable edit. The carry sits beside the sequence and never inside it.
/// note  ⚠️ `SavedPosition` arrives from `48` and is the one presented fact that is not a transaction — `84` §5.
///        `RevisionSequence` does not carry it, so a caller that never declares it presents every row as unsaved,
///        which is the safe direction: an artist told their work is unsaved loses nothing by saving twice.
/// tag   owning
struct RevisionPanelCarry
{
    float          VisibleOffset                     = 0.0f;    // [px] - top of the presented span
    std::uint64_t  SavedPosition                     = 0u;      // [-]  - from `48`; rows past it are unsaved
    bool           SavedPositionDeclared             = false;   // [-]  - false presents every row as unsaved
    bool           FoldOpen[RevisionFoldCapacity]    = {};      // [-]  - which rows have their fold open
    bool           ScrubDragOpen                     = false;   // [-]  - a scrub drag is running
    std::uint64_t  ScrubDragOrigin                   = 0u;      // [-]  - the position the drag opened at
    bool           DiscardPromptOpen                 = false;   // [-]  - the confirmation is presenting
    std::uint64_t  DiscardPromptCount                = 0u;      // [-]  - what it names, resolved when it opened
    bool           DiscardConfirmed                  = false;   // [-]  - the artist accepted; the caller clears it
};

/// 🧩 What the panel presents against — the sequence it reads and the carry it writes.
/// note  🔴 Both are pointers the workspace owns. The panel stores neither, which is `84` §1's gate: a presented
///        sequence holding its own row list drifts from the sequence the moment a transaction merges.
/// tag   nonallocating, nonthrowing
struct RevisionPanelContext
{
    RevisionSequence*    Sequence = nullptr;   // [-] - `10` owns it; read, and scrubbed through its own calls
    RevisionPanelCarry*  Carry    = nullptr;   // [-] - the workspace's own storage
};

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE DESTRUCTIVE FACT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 How many transactions an edit made at the current position would discard.
/// in    Sequence  [-]  read and never amended
/// out   Count     [-]  zero when the position is at the end of the sequence
/// note  🔴 `84` §3.1: this is presented **before** the discard, never reported after. Losing thirty transactions
///        is the most destructive thing this panel can do, and the tempting implementation discards first and
///        prints a count second — by which time the artist's only remaining choice is to accept it.
/// note  ⚠️ A caller admits an edit while this is non-zero only after `DiscardConfirmed` stood. The panel cannot
///        enforce that, because the edit does not arrive through the panel; the gate is the caller's and this
///        call is what makes it cheap to hold.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
std::uint64_t DiscardCountStanding(const RevisionSequence& Sequence);

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE SCRUBBING
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Moves the scrub position to one place in the sequence, replaying every intervening transaction in order.
/// in    Sequence  [-]  scrubbed through `Retreat` and `Advance` alone
/// in    Arriving  [-]  the position sought; bounded to the committed count
/// out   Outcome   [-]  refuses with ContentUnsupported when a replay along the way refused, naming where
/// post  the position is the sought one, or the furthest one every replay along the way delivered
/// note  🔴 `84` §3: every intervening transaction is replayed and nothing is skipped. A jump that restored a
///        snapshot would arrive at a state the sequence cannot be scrubbed back out of, because the inverses
///        between here and there were never run.
/// note  🔴 A scrub is **not itself a transaction** — `84` §3 and `10` §2.4. Nothing here opens one.
/// cost  🚩
/// tag   api, nonthrowing
Outcome<bool> ScrubToPosition(RevisionSequence& Sequence, std::uint64_t Arriving);

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Presents one tick of the revision sequence into the rectangle the desk resolved for it.
/// in    Theme           [-]   read by const reference; no colour or extent is spelled in this component
/// in    Area            [px]  the interior the panel layer handed it, header band included
/// in    PresentContext  [-]   a `RevisionPanelContext*`; a null context or a null sequence presents an empty state
/// note  🔴 Matches `PanelPresentRoutine` exactly so a workspace declares it into `PanelIndex` and the desk never
///        learns what a revision is. The opaque context is the whole reason one desk serves every workspace.
/// note  ⚠️ Merged transactions present as one row for free — `RevisionSequence` merges at `Seal`, so presenting
///        `Committed()` directly satisfies `84` §2's requirement without this component knowing merging exists.
/// note  ⚠️ No row is presented for an open transaction, for selection, for tool state or for panel layout —
///        `84` §4. This component reads `Committed()` and nothing else, so none of them can appear.
/// cost  🚩
/// tag   api, nonthrowing
void PresentRevisionPanel(const ThemeSpecification&  Theme,
                          const WorkspaceRectangle&  Area,
                          void*                      PresentContext);

// 📐 Positions, ordinals and counts are Exact integers — `84` §6's table has no Tier B row at all, and nothing
//    here interpolates a position. The Bounded entries are the rectangles the desk handed in, and those alone.
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded, PrecisionGuarantee::Exact);

}   // namespace Slate
