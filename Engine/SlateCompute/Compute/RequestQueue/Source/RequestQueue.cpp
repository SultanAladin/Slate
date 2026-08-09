//============================================================================================================================================
//                                                            REQUESTQUEUE.CPP
//============================================================================================================================================
// 🧩 Coalescing by cell, the cyclic rotation slots, and the readback that is exactly one depth behind.

#include "SlateCompute/Compute/RequestQueue/Api/RequestQueue.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE ARRIVAL ORDER
//------------------------------------------------------------------------------------------------------------------------

void PageQueue::Admit(const CellDemand& Arriving)
{
    // 📝 Searched newest first. A sample walking a surface demands the same cell for a run of adjacent pixels,
    //    so the coalescing terminates on its first comparison for the case that dominates.
    for (std::size_t Ordinal = ArrivalOrder.size(); Ordinal-- > 0u;)
    {
        CellDemand& Held = ArrivalOrder[Ordinal];

        if (Held.SurfaceOrdinal == Arriving.SurfaceOrdinal && Held.CellOrdinal == Arriving.CellOrdinal)
        {
            Held.OccurrenceCount += Arriving.OccurrenceCount;
            return;
        }
    }

    if (ArrivalOrder.size() >= ArrivalCeiling)
    {
        ++DiscardedDemands;
        return;
    }

    ArrivalOrder.push_back(Arriving);
}

const std::vector<CellDemand>& PageQueue::Arrivals() const { return ArrivalOrder; }

void PageQueue::Reclaim()
{
    ArrivalOrder.clear();
    DiscardedDemands = 0u;
}

std::uint32_t PageQueue::ArrivalCount() const
{
    return static_cast<std::uint32_t>(ArrivalOrder.size());
}

std::uint32_t PageQueue::DiscardedCount() const { return DiscardedDemands; }

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE DEMANDS
//------------------------------------------------------------------------------------------------------------------------

void RequestQueue::Demand(std::uint32_t SurfaceOrdinal, std::uint32_t CellOrdinal, std::uint64_t RotationOrdinal)
{
    CellDemand Arriving;
    Arriving.SurfaceOrdinal = SurfaceOrdinal;
    Arriving.CellOrdinal    = CellOrdinal;

    RotationSlots[RotationOrdinal % SlotCount].Admit(Arriving);
    ++RecordedDemands;
}

PageQueue& RequestQueue::SlotAt(std::uint64_t RotationOrdinal)
{
    return RotationSlots[RotationOrdinal % SlotCount];
}

const PageQueue& RequestQueue::SlotAt(std::uint64_t RotationOrdinal) const
{
    return RotationSlots[RotationOrdinal % SlotCount];
}

std::uint64_t RequestQueue::RecordedCount() const { return RecordedDemands; }

std::uint64_t RequestQueue::DiscardedCount() const
{
    std::uint64_t Discarded = 0u;

    for (const PageQueue& Held : RotationSlots)
        Discarded += Held.DiscardedCount();

    return Discarded;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE READBACK
//------------------------------------------------------------------------------------------------------------------------

Outcome<const PageQueue*> ReturnIndex::Drain(RequestQueue& Requesting, std::uint64_t RotationOrdinal)
{
    // 📝 The first rotations of a session have nothing recorded a depth ago. Refusing is honest: the caller
    //    promotes nothing, and the coarsest levels are permanently resident so every sample still resolves.
    if (RotationOrdinal < RecordingRotationDepth)
    {
        return Outcome<const PageQueue*>::Refuse(
            { RefusalReason::ExtentExhausted, "the readback latency has not yet elapsed" });
    }

    if (DrainStanding && RotationOrdinal <= LastDrained)
    {
        return Outcome<const PageQueue*>::Refuse(
            { RefusalReason::HostDenied, "this rotation has already been drained" });
    }

    // 🔴 Exactly one depth behind. `20` §2.1 ②'s latency is not an approximation of the device's readback — it
    //    **is** the readback, and a drain that read the current slot would present demands the device has not
    //    finished writing.
    PageQueue& Drained = Requesting.SlotAt(RotationOrdinal - RecordingRotationDepth);

    LastDrained   = RotationOrdinal;
    DrainStanding = true;
    ++DrainCount;

    return Outcome<const PageQueue*>::Deliver(&Drained);
}

std::uint64_t ReturnIndex::DrainedRotation() const { return LastDrained; }
std::uint64_t ReturnIndex::DrainedCount() const    { return DrainCount;  }

}   // namespace Slate
