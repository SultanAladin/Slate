//============================================================================================================================================
//                                                          SELECTIONSEQUENCE.CPP
//============================================================================================================================================
// 🧩 Sealing, traversal, and the restoration that pairs a document scrub with the selection it served.

#include "SlateDocument/Document/SelectionSequence/Api/SelectionSequence.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       SEALING
//------------------------------------------------------------------------------------------------------------------------

void SelectionSequence::Seal(const std::vector<OwnerIdentity>& Selected, std::uint64_t RevisionOrdinal)
{
    // 📝 Sealing after a backward traversal truncates what stood ahead, exactly as the document sequence
    //    does. Leaving it would let a forward traversal reach a selection the artist has since replaced.
    if (TraversalOrdinal < CommittedOrder.size())
        CommittedOrder.resize(static_cast<std::size_t>(TraversalOrdinal));

    CommittedSelection Incoming;
    Incoming.SelectedOwners = Selected;
    Incoming.RevisionOrdinal   = RevisionOrdinal;

    CommittedOrder.push_back(Incoming);

    CurrentSelection = Selected;
    TraversalOrdinal  = CommittedOrder.size();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      TRAVERSAL
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> SelectionSequence::Retreat()
{
    if (TraversalOrdinal == 0u)
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the traversal is at the beginning" });

    --TraversalOrdinal;

    if (TraversalOrdinal == 0u)
        CurrentSelection.clear();
    else
        CurrentSelection = CommittedOrder[static_cast<std::size_t>(TraversalOrdinal) - 1u].SelectedOwners;

    return Outcome<bool>::Result(true);
}

Outcome<bool> SelectionSequence::Advance()
{
    if (TraversalOrdinal >= CommittedOrder.size())
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the traversal is at the end" });

    CurrentSelection = CommittedOrder[static_cast<std::size_t>(TraversalOrdinal)].SelectedOwners;
    ++TraversalOrdinal;

    return Outcome<bool>::Result(true);
}

Outcome<bool> SelectionSequence::RestoreAt(std::uint64_t RevisionOrdinal)
{
    // 📝 The most recent selection sealed at or before the arrived-at revision. Searched backwards because a
    //    scrub arrives at a revision the artist selected against several times, and the last one is theirs.
    for (std::size_t Ordinal = CommittedOrder.size(); Ordinal-- > 0u;)
    {
        if (CommittedOrder[Ordinal].RevisionOrdinal > RevisionOrdinal)
            continue;

        CurrentSelection = CommittedOrder[Ordinal].SelectedOwners;
        TraversalOrdinal  = Ordinal + 1u;

        return Outcome<bool>::Result(true);
    }

    return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "no selection was sealed at that revision" });
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

const std::vector<OwnerIdentity>& SelectionSequence::Current() const
{
    return CurrentSelection;
}

const std::vector<CommittedSelection>& SelectionSequence::Committed() const
{
    return CommittedOrder;
}

std::uint64_t SelectionSequence::TraversalPosition() const
{
    return TraversalOrdinal;
}

}   // namespace Slate
