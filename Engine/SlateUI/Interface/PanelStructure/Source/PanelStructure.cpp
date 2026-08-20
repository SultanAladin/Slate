//============================================================================================================================================
//                                                         PANELSTRUCTURE.CPP
//============================================================================================================================================
// 🧩 Division, withdrawal, assignment and proportional resizing of a bounded workspace partition.

#include "SlateUI/Interface/PanelStructure/Api/PanelStructure.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

void PanelStructure::Construct(PanelSubject InitialSubject)
{
    Reset();
    Records[RootOrdinal].Occupied = true;
    Records[RootOrdinal].Subject  = InitialSubject;
}

void PanelStructure::Reset()
{
    for (std::uint32_t Ordinal = 0u; Ordinal < RecordCeiling; ++Ordinal)
        Records[Ordinal] = PanelRecord{};
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        DIVISION
//------------------------------------------------------------------------------------------------------------------------

std::uint32_t PanelStructure::TakeVacant()
{
    for (std::uint32_t Ordinal = 1u; Ordinal < RecordCeiling; ++Ordinal)
    {
        if (!Records[Ordinal].Occupied)
            return Ordinal;
    }

    return RecordCeiling;
}

Outcome<bool> PanelStructure::Divide(std::uint32_t LeafOrdinal,
                                     PanelDivisionAxis Axis,
                                     PanelDivisionSide VacantSide)
{
    if (LeafOrdinal >= RecordCeiling || !Records[LeafOrdinal].Occupied || Records[LeafOrdinal].Divided)
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "that ordinal names no leaf panel" });

    const std::uint32_t FirstSlot = TakeVacant();
    if (FirstSlot >= RecordCeiling)
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "no two panel slots remain" });

    Records[FirstSlot].Occupied = true;
    const std::uint32_t SecondSlot = TakeVacant();
    Records[FirstSlot].Occupied = false;

    if (SecondSlot >= RecordCeiling)
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "no two panel slots remain" });

    const PanelSubject DepartingSubject = Records[LeafOrdinal].Subject;
    const bool VacantTop = VacantSide == PanelDivisionSide::Minimum;

    Records[FirstSlot] = PanelRecord{ true, false,
                                      VacantTop ? PanelSubject::Vacant : DepartingSubject };
    Records[SecondSlot] = PanelRecord{ true, false,
                                       VacantTop ? DepartingSubject : PanelSubject::Vacant };

    PanelRecord& Divided = Records[LeafOrdinal];
    Divided.Divided       = true;
    Divided.Subject       = PanelSubject::Vacant;
    Divided.Axis          = Axis;
    Divided.MinimumFraction = 0.5f;
    Divided.Minimum  = FirstSlot;
    Divided.Maximum   = SecondSlot;

    return Outcome<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       WITHDRAWAL
//------------------------------------------------------------------------------------------------------------------------

bool PanelStructure::Encloses(std::uint32_t BranchOrdinal,
                              std::uint32_t SeekingOrdinal,
                              std::uint32_t& EnclosingOrdinal,
                              bool& MinimumSide) const
{
    if (BranchOrdinal >= RecordCeiling || !Records[BranchOrdinal].Occupied || !Records[BranchOrdinal].Divided)
        return false;

    const PanelRecord& Branch = Records[BranchOrdinal];
    if (Branch.Minimum == SeekingOrdinal || Branch.Maximum == SeekingOrdinal)
    {
        EnclosingOrdinal = BranchOrdinal;
        MinimumSide        = Branch.Minimum == SeekingOrdinal;
        return true;
    }

    return Encloses(Branch.Minimum, SeekingOrdinal, EnclosingOrdinal, MinimumSide) ||
           Encloses(Branch.Maximum, SeekingOrdinal, EnclosingOrdinal, MinimumSide);
}

Outcome<bool> PanelStructure::Withdraw(std::uint32_t LeafOrdinal)
{
    if (LeafOrdinal >= RecordCeiling || !Records[LeafOrdinal].Occupied || Records[LeafOrdinal].Divided)
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "that ordinal names no leaf panel" });

    if (LeafOrdinal == RootOrdinal)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the sole panel cannot be withdrawn" });

    std::uint32_t EnclosingOrdinal = RecordCeiling;
    bool          MinimumSide        = false;
    if (!Encloses(RootOrdinal, LeafOrdinal, EnclosingOrdinal, MinimumSide))
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "the leaf has no enclosing division" });

    const PanelRecord Enclosing = Records[EnclosingOrdinal];
    const std::uint32_t PromotedOrdinal = MinimumSide ? Enclosing.Maximum : Enclosing.Minimum;
    const PanelRecord Promoted = Records[PromotedOrdinal];

    Records[EnclosingOrdinal] = Promoted;
    Records[LeafOrdinal]      = PanelRecord{};
    Records[PromotedOrdinal]  = PanelRecord{};

    return Outcome<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        EDITING
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> PanelStructure::Assign(std::uint32_t LeafOrdinal, PanelSubject Subject)
{
    if (LeafOrdinal >= RecordCeiling || !Records[LeafOrdinal].Occupied || Records[LeafOrdinal].Divided)
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "that ordinal names no leaf panel" });

    if (Subject >= PanelSubject::SubjectCount)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "that panel subject is unsupported" });

    Records[LeafOrdinal].Subject = Subject;
    return Outcome<bool>::Result(true);
}

Outcome<bool> PanelStructure::Proportion(std::uint32_t DivisionOrdinal, float MinimumFraction)
{
    if (DivisionOrdinal >= RecordCeiling || !Records[DivisionOrdinal].Occupied ||
        !Records[DivisionOrdinal].Divided)
    {
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "that ordinal names no panel division" });
    }

    Records[DivisionOrdinal].MinimumFraction = (MinimumFraction < 0.05f) ? 0.05f
                                                   : (MinimumFraction > 0.95f) ? 0.95f
                                                                                 : MinimumFraction;
    return Outcome<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        READINGS
//------------------------------------------------------------------------------------------------------------------------

Outcome<PanelRecord> PanelStructure::Current(std::uint32_t Ordinal) const
{
    if (Ordinal >= RecordCeiling || !Records[Ordinal].Occupied)
        return Outcome<PanelRecord>::Refuse({ RefusalReason::IdentityStale, "that panel ordinal is unoccupied" });

    return Outcome<PanelRecord>::Result(Records[Ordinal]);
}

bool PanelStructure::RemovalAccepted() const
{
    const PanelRecord& Root = Records[RootOrdinal];
    return Root.Occupied && Root.Divided;
}

}   // namespace Slate
