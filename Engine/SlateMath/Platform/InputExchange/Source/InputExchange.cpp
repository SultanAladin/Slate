//============================================================================================================================================
//                                                            INPUTEXCHANGE.CPP
//============================================================================================================================================
// 🧩 Bounded cyclic arrival ordering over pointer samples.

#include "SlateMath/Platform/InputExchange/Api/InputExchange.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       ARRIVAL
//------------------------------------------------------------------------------------------------------------------------

void InputExchange::Record(const PointerSample& Arriving)
{
    const std::uint32_t WriteOrdinal = (OldestOrdinal + OccupiedCount) % ArrivalCapacity;
    ArrivalOrder[WriteOrdinal]       = Arriving;

    if (OccupiedCount == ArrivalCapacity)
    {
        // 📝 The extent is full, so the write above overwrote the oldest sample. Advancing the oldest
        //    ordinal is what makes that overwrite a discard rather than a corruption of the ordering.
        OldestOrdinal = (OldestOrdinal + 1u) % ArrivalCapacity;
    }
    else
    {
        ++OccupiedCount;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        DRAIN
//------------------------------------------------------------------------------------------------------------------------

const PointerSample& InputExchange::Sample(std::uint32_t ArrivalOrdinal) const
{
    return ArrivalOrder[(OldestOrdinal + ArrivalOrdinal) % ArrivalCapacity];
}

std::uint32_t InputExchange::HeldCount() const
{
    return OccupiedCount;
}

void InputExchange::Reclaim()
{
    OldestOrdinal = 0u;
    OccupiedCount = 0u;
}

}   // namespace Slate
