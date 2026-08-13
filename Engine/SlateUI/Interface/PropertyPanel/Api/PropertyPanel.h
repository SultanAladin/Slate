//============================================================================================================================================
//                                                            PROPERTYPANEL.H
//============================================================================================================================================
// 🧩 Any declared property presented without the panel knowing which — one row per declaration, read from `PropertyIndex`.

#pragma once

#include "Contract/PrecisionContract.h"
#include "SlateDocument/Document/PropertySpecification/Api/PropertySpecification.h"
#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/WorkspaceSpace.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT THE PANEL CARRIES
//------------------------------------------------------------------------------------------------------------------------

// 📝 A body presents a few dozen properties at once, not a population. Bounded so the carry is a fixed extent and
//    one tick of presentation allocates nothing.
inline constexpr std::uint32_t PropertyRowCapacity = 64u;    // [-] - rows whose carries are remembered at once
inline constexpr std::uint32_t PropertyNoticeExtent = 128u;  // [-] - characters a presented refusal keeps

/// 🧩 One property's edit lifecycle, reported to whoever owns the transaction.
/// note  🔴 `10` §2.4: a drag is **one** transaction and not one per tick. This panel does not open transactions —
///        it is a presentation — so it reports the three points and the caller brackets the run. A panel that
///        opened its own would be a second authority on what an edit is, and `76`'s tool parameters would then be
///        recorded twice.
/// tag   contract, nonallocating, nonthrowing
struct PropertyEditReport
{
    bool           EditOpened   = false;   // [-] - the first tick of a held edit
    bool           EditSealed   = false;   // [-] - the hold ended this tick
    bool           ValueWritten = false;   // [-] - a write landed this tick
    std::uint32_t  RowOrdinal   = 0u;      // [-] - which declaration, in declaration order
};

/// 🧩 What the panel carries between ticks — presentation only, and the caller owns all of it.
/// note  🔴 `14` §4.1: nothing here is a value or a declaration. The folds, the offset, the text carries and the
///        open dropdowns are layout, and layout inside the document would make opening a dropdown an undoable
///        edit.
/// note  ⚠️ The text and dropdown carries are addressed by **row ordinal**, so a carry that outlives a
///        re-declaration names a different property than it did. Presentation only — a stale carry presents the
///        wrong caret position for one tick and writes nothing, because a write is keyed by identity and not by
///        ordinal.
/// tag   owning
struct PropertyPanelCarry
{
    float               VisibleOffset                       = 0.0f;    // [px] - top of the presented span
    TextCarry           TextCarries[PropertyRowCapacity]    = {};      // [-]  - open text and path edits
    DropdownCarry       ChoiceCarries[PropertyRowCapacity]  = {};      // [-]  - open enrolment dropdowns
    bool                PickerOpen[PropertyRowCapacity]     = {};      // [-]  - which colour rows have a picker
    char                Notice[PropertyNoticeExtent]        = {};      // [-]  - the last refusal, verbatim
    bool                NoticeDeclared                      = false;   // [-]  - false presents no notice band
    std::uint32_t       PresentedTicks                      = 0u;      // [-]  - the panel's own; dropdowns read it
    PropertyEditReport  Reported                            = {};      // [-]  - this tick's lifecycle, for the caller
};

/// 🧩 What the panel presents against — the declarations it reads and the carry it writes.
/// note  🔴 `14` §1's gate: the panel stores neither, and it holds no value of its own. Every reading comes from
///        `Resolve` on the tick it is presented, so a value written by a tool while the panel is open presents
///        immediately rather than on the next click.
/// tag   nonallocating, nonthrowing
struct PropertyPanelContext
{
    PropertyIndex*       Declarations = nullptr;   // [-] - `10` owns it; written through `Write` alone
    PropertyPanelCarry*  Carry        = nullptr;   // [-] - the workspace's own storage
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT A ROW READS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The caption one measure is presented under.
/// note  ⚠️ Spelled apart from `ChannelPanel`'s measure caption. `PropertyMeasure` and `ChannelMeasure` are
///        unrelated enumerations and one name over both would read as a shared vocabulary they do not have.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* CaptionOfPropertyMeasure(PropertyMeasure Measured);

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Presents one tick of the declarations into the rectangle the desk resolved for it.
/// in    Theme           [-]   read by const reference; no colour or extent is spelled in this component
/// in    Area            [px]  the interior the panel layer handed it, header band included
/// in    PresentContext  [-]   a `PropertyPanelContext*`; a null context presents an empty state
/// note  🔴 Matches `PanelPresentRoutine` exactly so a workspace declares it into `PanelIndex` and the desk never
///        learns what a property is.
/// note  🔴 `76` §4's whole point: the panel presents whatever is declared and names no property. A tool that
///        adds a parameter is presented without this file being edited, and a tool presented by hand-written
///        panel code is a tool the panel must be edited to add.
/// note  🔴 A row **bounds and then writes** — `PropertySpecification.h` requires exactly that order, because a
///        write refuses rather than correcting. A row that wrote first would present a refusal on every drag that
///        touched an endpoint.
/// note  ⚠️ An `Occupant` measure is presented as a reading and offers no editor. Choosing an occupant is a
///        picking interaction `74` owns; a control invented here could only offer an ordinal the artist cannot
///        map to anything they can see.
/// cost  🚩
/// tag   api, nonthrowing
void PresentPropertyPanel(const ThemeSpecification&  Theme,
                          const WorkspaceRectangle&  Area,
                          void*                      PresentContext);

// 📐 Ordinals, counts, enrolments and identities are Exact integers. A magnitude is presented at the tier its
//    declaration carries and is never re-derived here; the Bounded entries are the rectangles the desk handed in.
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded, PrecisionGuarantee::Exact);

}   // namespace Slate
