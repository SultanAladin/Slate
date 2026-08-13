//============================================================================================================================================
//                                                            CHANNELPANEL.H
//============================================================================================================================================
// 🧩 One material's twenty channels made visible — every row read from `MaterialIndex`, nothing held here.

#pragma once

#include "Contract/PrecisionContract.h"
#include "SlateDocument/Document/MaterialSpecification/Api/MaterialSpecification.h"
#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/WorkspaceSpace.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT THE PANEL CARRIES
//------------------------------------------------------------------------------------------------------------------------

// 📝 The refusal notice's extent. A refusal's text is static and short — `10` §2.2 requires it to name which bound
//    was exceeded — so a fixed extent carries any of them and one tick of presentation allocates nothing.
inline constexpr std::uint32_t ChannelNoticeExtent = 128u;   // [-] - characters a presented refusal keeps

// 📝 The three sections the twenty channels fall into. Fixed so the fold carry is an extent and not a population.
inline constexpr std::uint32_t ChannelSectionCount = 3u;     // [-] - consumed, retained, undeclared

/// 🧩 What the panel carries between ticks — presentation only, and the caller owns all of it.
/// note  🔴 `14` §4.1 and `42` §5: nothing here is a channel, a selection or a threshold. The folds, the offset,
///        the open picker and the notice are layout, and layout inside the document would make opening a section
///        an undoable edit.
/// note  ⚠️ The notice is the text of the **last refusal**, kept so the artist reads why a write did not land.
///        It is not a copy of anything the material holds — a refused write leaves the material exactly as it
///        was, so there is nothing here that the document could drift from.
/// tag   owning
struct ChannelPanelCarry
{
    float          VisibleOffset                          = 0.0f;    // [px] - top of the presented span
    bool           SectionOpen[ChannelSectionCount]       = { true, true, false };
                                                                     // [-]  - consumed open, undeclared closed
    bool           PickerOpen[static_cast<std::size_t>(ChannelSubject::ChannelCount)] = {};
                                                                     // [-]  - which colour rows have a picker down
    DropdownCarry  SelectionCarry                         = {};      // [-]  - the reflectance dropdown's own
    char           Notice[ChannelNoticeExtent]            = {};      // [-]  - the last refusal, verbatim
    bool           NoticeDeclared                         = false;   // [-]  - false presents no notice band
    std::uint32_t  PresentedTicks                         = 0u;      // [-]  - the panel's own; a dropdown reads it
};

/// 🧩 What the panel presents against — the ledger it reads and the carry it writes.
/// note  🔴 `14` §1's gate: the panel stores neither, and it does not store the material either. It resolves the
///        ordinal against the ledger every tick, so a material withdrawn beneath it presents as absent rather
///        than as a stale copy of channels that no longer exist.
/// note  ⚠️ `MaterialSpecification`'s API is presented **against** and never copied. Where a channel cannot be
///        amended through a call the ledger offers, the row presents the reading and refuses the edit — it does
///        not hold a local value and write it back later.
/// tag   nonallocating, nonthrowing
struct ChannelPanelContext
{
    MaterialIndex*      Materials       = nullptr;   // [-] - `42` owns it; amended through `Amend` alone
    std::uint32_t       MaterialOrdinal = 0u;        // [-] - which material; resolved fresh every tick
    ChannelPanelCarry*  Carry           = nullptr;   // [-] - the workspace's own storage
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT A ROW READS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The caption one channel is presented under.
/// note  🔴 Read from the declared subject and never derived from the value — the same rule `86` §4.1 applies to
///        a report class. `18` §2's order is the presented order, so the artist reads the channels in the order
///        the document declares them.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* CaptionOfChannel(ChannelSubject Channel);

/// 🧩 The caption one reflectance selection is presented under.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* CaptionOfReflectance(ReflectanceSelection Selected);

/// 🧩 The caption one channel source is presented under.
/// note  ⚠️ Spelled apart from `LayerPanel`'s `CaptionOfSource` deliberately. The two enumerations are unrelated —
///        one is where a channel's value comes from and the other is what a layer's content is — and one name
///        over both would read as a shared vocabulary they do not have.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* CaptionOfChannelSource(ChannelSource Source);

/// 🧩 The caption one channel measure is presented under.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* CaptionOfChannelMeasure(ChannelMeasure Measured);

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Presents one tick of one material's channels into the rectangle the desk resolved for it.
/// in    Theme           [-]   read by const reference; no colour or extent is spelled in this component
/// in    Area            [px]  the interior the panel layer handed it, header band included
/// in    PresentContext  [-]   a `ChannelPanelContext*`; a null context or an unresolved ordinal presents empty
/// note  🔴 Matches `PanelPresentRoutine` exactly so a workspace declares it into `PanelIndex` and the desk never
///        learns what a channel is.
/// note  🔴 `42` §5: a channel declared for a reflectance the material no longer selects is **retained**, and it
///        is presented in its own section rather than hidden. Hiding it is what makes an artist believe that
///        switching a selection destroyed their work, and they then avoid switching at all.
/// note  ⚠️ Whether a channel is sampled is read from `ChannelSampled` and whether it is converted from
///        `ChannelConverted`. Neither is re-derived here — `18` §9's inventory and `36` §4's declaration are the
///        material's answers, and a panel that recomputed them could disagree with the dispatch that reads them.
/// cost  🚩
/// tag   api, nonthrowing
void PresentChannelPanel(const ThemeSpecification&  Theme,
                         const WorkspaceRectangle&  Area,
                         void*                      PresentContext);

// 📐 Channel ordinals, selections, sources and measures are Exact integers. Constant magnitudes and coordinates
//    are presented at the tier `42` declared for them and are never re-derived here; the Bounded entries are the
//    rectangles the desk handed in.
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded, PrecisionGuarantee::Exact);

}   // namespace Slate
