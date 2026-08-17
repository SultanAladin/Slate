//============================================================================================================================================
//                                                             CONTROLPANEL.H
//============================================================================================================================================
// 🧩 Reusable inspector controls transcribed from the global-interface reference, with no presented datum owned.

#pragma once

#include "Contract/DeliveryContract.h"
#include "Contract/PrecisionContract.h"
#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "SlateUI/Interface/InteractionIndex/Api/InteractionIndex.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"
#include "SlateUI/Interface/SymbolSpecification/Api/SymbolSpecification.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  CONTROL DECLARATIONS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One labelled switch whose track and nub reproduce `.switch` and `.nub` from the reference.
/// tag   contract, nonallocating, nonthrowing
struct SwitchDeclaration
{
    const char*  Caption = "";   // [-] - borrowed; outlives the tick
};

/// 🧩 One mutually exclusive caption sequence.
/// tag   contract, nonallocating, nonthrowing
struct SegmentDeclaration
{
    const char* const*  Captions    = nullptr;   // [-] - borrowed; outlives the tick
    std::uint32_t       CaptionCount = 0u;       // [-] - captions presented
};

/// 🧩 One tab strip whose selected caption carries the accent underline.
/// tag   contract, nonallocating, nonthrowing
struct TabDeclaration
{
    const char* const*  Captions     = nullptr;   // [-] - borrowed; outlives the tick
    std::uint32_t       CaptionCount = 0u;        // [-] - captions presented
};

/// 🧩 The two inspector pages carried horizontally beneath a two-caption tab strip.
/// tag   contract, nonallocating, nonthrowing
struct CarouselDeclaration
{
    const char* const*  LeadingRuns   = nullptr;   // [-] - borrowed property summaries
    std::uint32_t       LeadingCount  = 0u;        // [-] - rows on the leading page
    const char* const*  TrailingRuns  = nullptr;   // [-] - borrowed revision summaries
    std::uint32_t       TrailingCount = 0u;        // [-] - rows on the trailing page
};

/// 🧩 One folding card header and the count displayed at its trailing edge.
/// tag   contract, nonallocating, nonthrowing
struct FoldDeclaration
{
    const char*         Caption     = "";        // [-] - borrowed; outlives the tick
    const char* const*  BodyRuns    = nullptr;   // [-] - borrowed rows revealed by the fold
    std::uint32_t       BodyCount   = 0u;        // [-] - rows held by the card
};

/// 🧩 One caption field and the menu rows it discloses beneath itself.
/// tag   contract, nonallocating, nonthrowing
struct DropdownDeclaration
{
    const char*         Caption     = "";        // [-] - leading field label
    const char* const*  Options     = nullptr;   // [-] - borrowed; outlives the tick
    std::uint32_t       OptionCount = 0u;        // [-] - menu rows presented
};

/// 🧩 One linearised outline row, including its depth and visibility condition.
/// tag   contract, nonallocating, nonthrowing
struct OutlineDeclaration
{
    const char*    Caption          = "";    // [-] - borrowed; outlives the tick
    std::uint32_t  Depth            = 0u;    // [-] - indentation steps from the root
    std::uint32_t  EnclosedCount    = 0u;    // [-] - zero presents no disclosure mark
    bool           ExpansionEnabled = true;  // [-] - whether enclosed rows are presented
    bool           PresenceEnabled  = true;  // [-] - whether the eye action presents presence
};

/// 🧩 One revision marker with its description and secondary run.
/// tag   contract, nonallocating, nonthrowing
struct RevisionDeclaration
{
    const char*  Description = "";   // [-] - borrowed; outlives the tick
    const char*  Secondary   = "";   // [-] - borrowed; outlives the tick
    const char*  TimeRun     = "";   // [-] - borrowed; outlives the tick
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE PANEL
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Presents the shared controls used by drafting, painting and world-editor inspectors.
/// note  🔴 Every artist-visible datum arrives by reference and is written through that reference. The panel
///       retains only borrowed recording facilities; the host or application owns every selected condition.
/// tag   owning
class ControlPanel
{
public:

    /// 🧩 Borrows the interaction index, recording surface and resolved appearance.
    /// out   Deliver  [-]  refuses when a construction already stands
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    Deliver<bool> Construct(InteractionIndex&              Interaction,
                            RecordingSurface&              Recording,
                            const AppearanceSpecification& Appearance);

    /// 🧩 Samples the contact after the tick owner has advanced the shared interaction index once.
    /// note  🔴 This does not advance the index; several panels share it and the tick owner advances it once.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Advance(const PointerCondition& Arrived, double Elapsed);

    /// 🧩 Presents the reference track-and-nub switch.
    /// cost  🚩
    /// tag   api, nonthrowing
    ControlVerdict SwitchToggle(ControlIdentity Claimed, const PlaneExtent& Extent,
                                const SwitchDeclaration& Declared, bool& Taken);

    /// 🧩 Presents one mutually exclusive sequence and writes the selected ordinal.
    /// cost  🚩
    /// tag   api, nonthrowing
    ControlVerdict SegmentedChoice(ControlIdentity Claimed, const PlaneExtent& Extent,
                                   const SegmentDeclaration& Declared, std::uint32_t& TakenOrdinal);

    /// 🧩 Presents one tab strip and writes the selected ordinal.
    /// cost  🚩
    /// tag   api, nonthrowing
    ControlVerdict TabStrip(ControlIdentity Claimed, const PlaneExtent& Extent,
                            const TabDeclaration& Declared, std::uint32_t& TakenOrdinal);

    /// 🧩 Slides the leading and trailing inspector pages beneath their tab strip.
    /// note  The same identity is passed to TabStrip, so selection and page travel remain one interaction.
    /// cost  🚩
    /// tag   api, nonthrowing
    ControlVerdict CarouselPages(ControlIdentity Claimed, const PlaneExtent& Extent,
                                 const CarouselDeclaration& Declared, std::uint32_t TakenOrdinal);

    /// 🧩 Presents one folding card header and writes its expanded condition.
    /// cost  🚩
    /// tag   api, nonthrowing
    ControlVerdict CollapsibleCard(ControlIdentity Claimed, const PlaneExtent& Extent,
                                   const FoldDeclaration& Declared, bool& ExpansionEnabled);

    /// 🧩 Presents one selection field and an animated menu card beneath it.
    /// cost  🚩
    /// tag   api, nonthrowing
    ControlVerdict DropdownCard(ControlIdentity Claimed, const PlaneExtent& Extent,
                                const DropdownDeclaration& Declared, std::uint32_t& TakenOrdinal);

    /// 🧩 Presents one outline row with additive selection and a visibility action.
    /// note  SelectionExtended controls whether a row press toggles this row without clearing other rows.
    /// cost  🚩
    /// tag   api, nonthrowing
    ControlVerdict OutlineRow(ControlIdentity Claimed, const PlaneExtent& Extent,
                              const OutlineDeclaration& Declared, bool SelectionExtended,
                              bool& ExpansionEnabled, bool& Selected, bool& PresenceEnabled);

    /// 🧩 Presents one revision marker and its two text runs.
    /// cost  ✔️
    /// tag   api, nonthrowing
    void RevisionRow(const PlaneExtent& Extent, const RevisionDeclaration& Declared, bool Taken);

    /// 🧩 Returns the panel to its unconstructed condition.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Reset();

private:

    ControlVerdict ResolveTap(ControlIdentity Claimed, const PlaneExtent& Extent, bool& Altered);

    InteractionIndex*              Interaction = nullptr;   // [-] - borrowed; never owned
    RecordingSurface*              Recording   = nullptr;   // [-] - borrowed; never owned
    const AppearanceSpecification* Appearance  = nullptr;   // [-] - borrowed; never owned
    PointerCondition               Arrived     = {};        // [-] - current tick sample
};

SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded);

}   // namespace Slate
