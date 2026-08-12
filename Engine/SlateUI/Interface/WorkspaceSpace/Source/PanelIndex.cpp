//============================================================================================================================================
//                                                             PANELINDEX.CPP
//============================================================================================================================================
// 🧩 Declaration, release and resolution over a fixed panel ledger — no allocation, no ImGui, no vendor spelling.

#include "SlateUI/Interface/WorkspaceSpace/Api/PanelIndex.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     LEDGER OPERATIONS
//------------------------------------------------------------------------------------------------------------------------

void ReclaimPanelIndex(PanelIndex& Ledger)
{
    // 📝 Every slot is overwritten rather than only the count being cleared. A stale context pointer left behind a
    //    zeroed count is one a later declaration reads through the moment the count passes it again, and the
    //    workspace that owned that context has already deactivated by then.
    for (std::uint32_t SlotOrdinal = 0u; SlotOrdinal < PanelSlotCapacity; ++SlotOrdinal)
        Ledger.DeclaredSlots[SlotOrdinal] = PanelSlot{};

    Ledger.DeclaredCount = 0u;
}

Outcome<bool> DeclarePanel(PanelIndex& Ledger, const PanelSlot& Declaring)
{
    if (Declaring.PanelIdentifier == nullptr || Declaring.PanelIdentifier[0] == '\0')
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "a panel declaration names no identifier" });
    }

    if (Declaring.Present == nullptr)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "a panel declaration names no present routine" });
    }

    if (Ledger.DeclaredCount >= PanelSlotCapacity)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::ExtentExhausted, "the panel ledger already holds its declared capacity" });
    }

    Ledger.DeclaredSlots[Ledger.DeclaredCount] = Declaring;
    ++Ledger.DeclaredCount;

    return Outcome<bool>::Deliver(true);
}

Outcome<PanelSlot> ResolvePanelForSide(const PanelIndex& Ledger, WorkspacePanelSide ResolvedSide)
{
    for (std::uint32_t SlotOrdinal = 0u; SlotOrdinal < Ledger.DeclaredCount; ++SlotOrdinal)
    {
        if (Ledger.DeclaredSlots[SlotOrdinal].DeclaredSide == ResolvedSide)
            return Outcome<PanelSlot>::Deliver(Ledger.DeclaredSlots[SlotOrdinal]);
    }

    return Outcome<PanelSlot>::Refuse(
        { RefusalReason::ContentUnsupported, "no panel is declared for that side" });
}

std::uint32_t DeclaredPanelCount(const PanelIndex& Ledger)
{
    return Ledger.DeclaredCount;
}

}   // namespace Slate
