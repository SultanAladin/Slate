//============================================================================================================================================
//                                                           DIAGNOSTICPANEL.H
//============================================================================================================================================
// 🧩 The register made visible — reports grouped by their declared class, measures presented at their producer's tier.

#pragma once

#include "Contract/OutcomeContract.h"
#include "Contract/PrecisionContract.h"
#include "SlateMath/Numeric/ReportSequence/Api/ReportSequence.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/WorkspaceSpace.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT A CLASS WEIGHS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 How much of the artist's attention one report class is worth.
/// note  🔴 `86` §5: five of the seven classes describe normal operation, and a panel presenting all seven as
///        problems teaches the artist to ignore it. `Refused` and `Failed` are problems; `Terminated` is the
///        ambiguous row `02` §5 leaves ambiguous on purpose; the rest are information.
/// tag   contract
enum class ReportWeight : std::uint32_t
{
    Information = 0u,   // [-] - the engine did something and is saying so
    Ambiguous   = 1u,   // [-] - a ceiling was reached; the result is the best available, not wrong
    Problem     = 2u    // [-] - nothing was produced, or nothing completed
};

/// 🧩 The weight one declared class carries.
/// in    Declared  [-]  the disposition the reporting mechanism declared
/// note  🔴 Resolved from the declared class alone and never from the report's text — `86` §4.1. An inferred
///        weight is a presentation that disagrees with the document that made the promise.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
ReportWeight WeightOf(ReportDisposition Declared);

/// 🧩 Where one class sits in the presented order, lowest first.
/// note  ⚠️ `56` §3.1's resampling report is an `Amended`, and it is the one operation in the engine that
///        resamples authored content. `Amended` therefore ranks above every other informational class, so it
///        can never sit at the same weight as a residency total — which is a `Measured`, presented last. The
///        elevation is by class and not by matching the origin text, so nothing here infers anything.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
std::uint32_t PresentedRankOf(ReportDisposition Declared);

/// 🧩 The caption one class is presented under.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* CaptionOf(ReportDisposition Declared);

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT THE PANEL CARRIES
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What the panel carries between ticks — presentation only, and the caller owns all of it.
/// note  🔴 `86` §10 and `14` §1: nothing here is a copy of either structure. A panel holding its own copy
///        presents a residency total from the rotation before last, which reads as a measure that has stopped
///        moving rather than as a panel that has stopped reading.
/// tag   owning
struct DiagnosticPanelCarry
{
    float  VisibleOffset                                        = 0.0f;   // [px] - top of the presented span
    bool   ClassOpen[static_cast<std::size_t>(ReportDisposition::DispositionCount)] = {};
                                                                          // [-]  - which class sections are open
    bool   MeasuresOpen                                         = true;   // [-]  - the measure section's own fold
    bool   InformationDeclared                                  = true;   // [-]  - informational classes presented
};

/// 🧩 What the panel presents against — the two structures it reads and the carry it writes.
/// note  🔴 Both structures live in `SlateMath` — `86` §3's link partition, not a preference. Neither is owned
///        here and neither is copied here.
/// tag   nonallocating, nonthrowing
struct DiagnosticPanelContext
{
    const ReportSequence*  Reports  = nullptr;   // [-] - `SlateMath` owns it; read under its own guard
    const MeasureIndex*    Measures = nullptr;   // [-] - sampled by the tick, never pushed
    DiagnosticPanelCarry*  Carry    = nullptr;   // [-] - the workspace's own storage
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Presents one tick of the register into the rectangle the desk resolved for it.
/// in    PresentContext  [-]  a `DiagnosticPanelContext*`; a null context presents an empty state
/// note  🔴 Matches `PanelPresentRoutine` exactly, so a workspace declares it into `PanelIndex` and the desk
///        never learns what a report is.
/// note  ⚠️ A recurring report presents as one entry carrying its count — the register coalesced it at append,
///        so presenting `Retained()` directly satisfies `86` §6 without this component knowing coalescing exists.
/// note  ⚠️ A measure is presented at the tier its producer declared: a `Counted` prints as an integer and a
///        `Magnitude` as a real, and neither is ever converted into the other. `86` §9 — recomputing a measure
///        for presentation lets the panel disagree with the mechanism that produced it.
/// cost  🚩
/// tag   api, nonthrowing
void PresentDiagnosticPanel(const ThemeSpecification&  Theme,
                            const WorkspaceRectangle&  Area,
                            void*                      PresentContext);

// 📐 Ordinals, counts, occurrence counts and byte extents are Exact integers. A magnitude is presented at
//    whatever tier its producer declared and is never re-derived here, so this component claims nothing about it.
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded, PrecisionGuarantee::Exact);

}   // namespace Slate
