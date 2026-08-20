//============================================================================================================================================
//                                                           ENROLLMENTINDEX.CPP
//============================================================================================================================================
// 🧩 Interval merging, division, and the exclusion refusal that precedes every write.

#include "SlateDocument/Document/RegistrationIndex/Api/RegistrationIndex.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   INTERVAL SEARCH
//------------------------------------------------------------------------------------------------------------------------

// 📝 The first run whose last ordinal is not below the subject. Every registration, withdrawal and test starts
//    here, so it is one routine rather than three loops that must agree.
static std::size_t LocateInterval(const std::vector<RegisteredInterval>& Runs, std::uint32_t SlotOrdinal)
{
    std::size_t Lower = 0u;
    std::size_t Upper = Runs.size();

    while (Lower < Upper)
    {
        const std::size_t Middle = Lower + (Upper - Lower) / 2u;

        if (Runs[Middle].LastOrdinal < SlotOrdinal)
            Lower = Middle + 1u;
        else
            Upper = Middle;
    }

    return Lower;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                 INTERVAL ENROLMENT
//------------------------------------------------------------------------------------------------------------------------

// 📝 The merging body Register used to hold inline, lifted out so that `38`'s degeneracy registration reads the same
//    code rather than a second copy of the same reasoning.
bool RegisterInterval(std::vector<RegisteredInterval>& Runs, std::uint32_t Ordinal)
{
    const std::size_t Located = LocateInterval(Runs, Ordinal);

    if (Located < Runs.size() && Runs[Located].FirstOrdinal <= Ordinal)
        return false;

    const bool AbutsBelow = Located != 0u && Runs[Located - 1u].LastOrdinal + 1u == Ordinal;
    const bool AbutsAbove = Located < Runs.size() && Runs[Located].FirstOrdinal == Ordinal + 1u;

    if (AbutsBelow && AbutsAbove)
    {
        Runs[Located - 1u].LastOrdinal = Runs[Located].LastOrdinal;
        Runs.erase(Runs.begin() + static_cast<std::ptrdiff_t>(Located));
    }
    else if (AbutsBelow)
    {
        Runs[Located - 1u].LastOrdinal = Ordinal;
    }
    else if (AbutsAbove)
    {
        Runs[Located].FirstOrdinal = Ordinal;
    }
    else
    {
        RegisteredInterval Incoming;
        Incoming.FirstOrdinal = Ordinal;
        Incoming.LastOrdinal  = Ordinal;

        Runs.insert(Runs.begin() + static_cast<std::ptrdiff_t>(Located), Incoming);
    }

    return true;
}

bool IntervalRegistered(const std::vector<RegisteredInterval>& Runs, std::uint32_t Ordinal)
{
    const std::size_t Located = LocateInterval(Runs, Ordinal);

    return Located < Runs.size() && Runs[Located].FirstOrdinal <= Ordinal;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      ENROLMENT
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> RegistrationIndex::Register(OwnerIdentity Subject, SubsetSubject RegisteredSubset)
{
    if (!Subject.IdentityDeclared())
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "an undeclared identity registers in nothing" });

    // 🔴 Decided before anything is written. A rejected registration leaves no partial state, which is what
    //    lets the caller abandon its transaction rather than repair it.
    for (std::uint32_t Ordinal = 0u; Ordinal < static_cast<std::uint32_t>(SubsetSubject::SubsetCount); ++Ordinal)
    {
        const SubsetSubject Current = static_cast<SubsetSubject>(Ordinal);

        if (SubsetsExclusive(RegisteredSubset, Current) && Registered(Subject, Current))
        {
            return Outcome<bool>::Refuse(
                { RefusalReason::ContentUnsupported, "a mutually exclusive subset already holds the owner" });
        }
    }

    std::vector<RegisteredInterval>& Runs        = SubsetIntervals[static_cast<std::size_t>(RegisteredSubset)];

    // 📝 Extending an abutting run keeps the storage at one run per contiguous span. Two runs that touch
    //    carry no fact the merged run does not, and every later comparison pays for the extra entry.
    if (RegisterInterval(Runs, Subject.SlotOrdinal))
        ++SubsetCounts[static_cast<std::size_t>(RegisteredSubset)];

    return Outcome<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      WITHDRAWAL
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> RegistrationIndex::Unenrol(OwnerIdentity Subject, SubsetSubject RegisteredSubset)
{
    if (!Subject.IdentityDeclared())
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "an undeclared identity registers in nothing" });

    std::vector<RegisteredInterval>& Runs        = SubsetIntervals[static_cast<std::size_t>(RegisteredSubset)];
    const std::uint32_t            SlotOrdinal = Subject.SlotOrdinal;
    const std::size_t              Located     = LocateInterval(Runs, SlotOrdinal);

    if (Located >= Runs.size() || Runs[Located].FirstOrdinal > SlotOrdinal)
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "the owner was not registered here" });

    const RegisteredInterval Held = Runs[Located];

    if (Held.FirstOrdinal == SlotOrdinal && Held.LastOrdinal == SlotOrdinal)
    {
        Runs.erase(Runs.begin() + static_cast<std::ptrdiff_t>(Located));
    }
    else if (Held.FirstOrdinal == SlotOrdinal)
    {
        Runs[Located].FirstOrdinal = SlotOrdinal + 1u;
    }
    else if (Held.LastOrdinal == SlotOrdinal)
    {
        Runs[Located].LastOrdinal = SlotOrdinal - 1u;
    }
    else
    {
        // 📝 A withdrawal from the interior divides one run into two. This is the only operation that grows
        //    the run count, and it grows it by exactly one.
        Runs[Located].LastOrdinal = SlotOrdinal - 1u;

        RegisteredInterval Upper;
        Upper.FirstOrdinal = SlotOrdinal + 1u;
        Upper.LastOrdinal  = Held.LastOrdinal;

        Runs.insert(Runs.begin() + static_cast<std::ptrdiff_t>(Located) + 1, Upper);
    }

    --SubsetCounts[static_cast<std::size_t>(RegisteredSubset)];

    return Outcome<bool>::Result(true);
}

void RegistrationIndex::UnenrolEverywhere(OwnerIdentity Subject)
{
    for (std::uint32_t Ordinal = 0u; Ordinal < static_cast<std::uint32_t>(SubsetSubject::SubsetCount); ++Ordinal)
        Discard(Unenrol(Subject, static_cast<SubsetSubject>(Ordinal)));
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

bool RegistrationIndex::Registered(OwnerIdentity Subject, SubsetSubject RegisteredSubset) const
{
    if (!Subject.IdentityDeclared())
        return false;

    return IntervalRegistered(SubsetIntervals[static_cast<std::size_t>(RegisteredSubset)], Subject.SlotOrdinal);
}

const std::vector<RegisteredInterval>& RegistrationIndex::Intervals(SubsetSubject RegisteredSubset) const
{
    return SubsetIntervals[static_cast<std::size_t>(RegisteredSubset)];
}

std::uint32_t RegistrationIndex::RegisteredCount(SubsetSubject RegisteredSubset) const
{
    return SubsetCounts[static_cast<std::size_t>(RegisteredSubset)];
}

void RegistrationIndex::Reclaim(SubsetSubject RegisteredSubset)
{
    SubsetIntervals[static_cast<std::size_t>(RegisteredSubset)].clear();
    SubsetCounts[static_cast<std::size_t>(RegisteredSubset)] = 0u;
}

bool RegistrationIndex::RegistrationsOccupied(const std::vector<std::uint32_t>& Generations) const
{
    for (std::size_t Subset = 0u; Subset < SubsetSpan; ++Subset)
    {
        for (const RegisteredInterval& Run : SubsetIntervals[Subset])
        {
            for (std::uint32_t Ordinal = Run.FirstOrdinal; Ordinal <= Run.LastOrdinal; ++Ordinal)
            {
                if (Ordinal >= Generations.size() || Generations[Ordinal] == 0u)
                    return false;
            }
        }
    }

    return true;
}

}   // namespace Slate
