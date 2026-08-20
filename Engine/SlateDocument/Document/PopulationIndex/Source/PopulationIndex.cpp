//============================================================================================================================================
//                                                           POPULATIONINDEX.CPP
//============================================================================================================================================
// 🧩 Slot issuance, withdrawal and generational resolution.

#include "SlateDocument/Document/PopulationIndex/Api/PopulationIndex.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                      OCCUPANCY
//------------------------------------------------------------------------------------------------------------------------

void OccupancyIndex::Occupy(std::uint32_t SlotOrdinal)
{
    const std::size_t WordOrdinal = SlotOrdinal / 64u;
    const std::uint64_t BitMask   = 1ull << (SlotOrdinal % 64u);

    if (WordOrdinal >= OccupancyWords.size())
        OccupancyWords.resize(WordOrdinal + 1u, 0ull);

    OccupancyWords[WordOrdinal] |= BitMask;

    if (SlotOrdinal + 1u > SpannedSlots)
        SpannedSlots = SlotOrdinal + 1u;
}

void OccupancyIndex::Release(std::uint32_t SlotOrdinal)
{
    const std::size_t WordOrdinal = SlotOrdinal / 64u;

    if (WordOrdinal >= OccupancyWords.size())
        return;

    OccupancyWords[WordOrdinal] &= ~(1ull << (SlotOrdinal % 64u));
}

bool OccupancyIndex::Occupied(std::uint32_t SlotOrdinal) const
{
    const std::size_t WordOrdinal = SlotOrdinal / 64u;

    if (WordOrdinal >= OccupancyWords.size())
        return false;

    return (OccupancyWords[WordOrdinal] & (1ull << (SlotOrdinal % 64u))) != 0ull;
}

std::uint32_t OccupancyIndex::SpannedCount() const
{
    return SpannedSlots;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      ENROLMENT
//------------------------------------------------------------------------------------------------------------------------

Outcome<OwnerIdentity> PopulationIndex::Register()
{
    std::uint32_t SlotOrdinal = 0u;

    if (!ReleasedOrdinals.empty())
    {
        // 📝 A released slot is reused with its generation already advanced by Withdraw, so the identity
        //    registered here can never equal one registered for the slot's previous owner.
        SlotOrdinal = ReleasedOrdinals.back();
        ReleasedOrdinals.pop_back();
    }
    else
    {
        if (SlotGenerations.size() >= PopulationCeiling)
        {
            return Outcome<OwnerIdentity>::Refuse(
                { RefusalReason::ExtentExhausted, "the population reached its declared ceiling" });
        }

        SlotOrdinal = static_cast<std::uint32_t>(SlotGenerations.size());
        SlotGenerations.push_back(1u);
    }

    Occupancy.Occupy(SlotOrdinal);
    ++OccupiedCount;

    OwnerIdentity Registered;
    Registered.SlotOrdinal    = SlotOrdinal;
    Registered.SlotGeneration = SlotGenerations[SlotOrdinal];

    return Outcome<OwnerIdentity>::Result(Registered);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      WITHDRAWAL
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> PopulationIndex::Withdraw(OwnerIdentity Subject)
{
    if (!Resolve(Subject))
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "the identity no longer resolves" });

    Occupancy.Release(Subject.SlotOrdinal);

    // 📝 The generation advances on withdrawal, not on reuse. Every reference carrying the prior generation
    //    resolves to absent from this point, whether or not the slot is ever occupied again.
    ++SlotGenerations[Subject.SlotOrdinal];

    ReleasedOrdinals.push_back(Subject.SlotOrdinal);
    --OccupiedCount;

    return Outcome<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

bool PopulationIndex::Resolve(OwnerIdentity Subject) const
{
    if (!Subject.IdentityDeclared())
        return false;

    if (Subject.SlotOrdinal >= SlotGenerations.size())
        return false;

    if (!Occupancy.Occupied(Subject.SlotOrdinal))
        return false;

    return SlotGenerations[Subject.SlotOrdinal] == Subject.SlotGeneration;
}

std::uint32_t PopulationIndex::RegisteredCount() const
{
    return OccupiedCount;
}

}   // namespace Slate
