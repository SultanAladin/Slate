//============================================================================================================================================
//                                                           REVISIONSEQUENCE.CPP
//============================================================================================================================================
// 🧩 The drag lifecycle, declared merging, and scrubbing in both directions.

#include "SlateDocument/Document/RevisionSequence/Api/RevisionSequence.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       OPENING
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> RevisionSequence::Open(const std::string& Description, const std::string& OperationName)
{
    if (OpenDeclared)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "a transaction is already open" });

    OpenTransaction               = {};
    OpenTransaction.Description   = Description;
    OpenTransaction.OperationName = OperationName;
    OpenDeclared                  = true;

    return Outcome<bool>::Result(true);
}

void RevisionSequence::Abandon()
{
    OpenTransaction = {};
    OpenDeclared    = false;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       SEALING
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> RevisionSequence::Seal(std::uint64_t SealedAt, bool MergeDeclared)
{
    if (!OpenDeclared)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "no transaction is open" });

    // 📝 Sealing after a retreat discards the transactions the artist scrubbed past. They are not reachable
    //    again, and keeping them would present two futures from one position.
    if (ScrubOrdinal < CommittedOrder.size())
        CommittedOrder.resize(static_cast<std::size_t>(ScrubOrdinal));

    OpenTransaction.SealedAt      = SealedAt;
    OpenTransaction.MergeDeclared = MergeDeclared;

    // 📝 Merging is declared per operation and never inferred. An operation that merges when the artist
    //    expected two steps is as wrong as one that does not merge when they expected one.
    const bool MergeReachable = MergeDeclared
                             && !CommittedOrder.empty()
                             && CommittedOrder.back().MergeDeclared
                             && CommittedOrder.back().OperationName == OpenTransaction.OperationName
                             && (SealedAt - CommittedOrder.back().SealedAt) <= MergeInterval;

    if (MergeReachable)
    {
        // 📝 The merged transaction keeps the earlier inverse and the later forward, so undoing it returns
        //    to the content that stood before the first of the two.
        CommittedOrder.back().ForwardOrdinal = OpenTransaction.ForwardOrdinal;
        CommittedOrder.back().SealedAt       = SealedAt;
    }
    else
    {
        CommittedOrder.push_back(OpenTransaction);
    }

    ScrubOrdinal    = CommittedOrder.size();
    OpenTransaction = {};
    OpenDeclared    = false;

    return Outcome<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      SCRUBBING
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> RevisionSequence::Retreat()
{
    if (ScrubOrdinal == 0u)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the scrub position is at the beginning" });

    --ScrubOrdinal;
    return Outcome<bool>::Result(true);
}

Outcome<bool> RevisionSequence::Advance()
{
    if (ScrubOrdinal >= CommittedOrder.size())
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the scrub position is at the end" });

    ++ScrubOrdinal;
    return Outcome<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

const std::vector<CommittedTransaction>& RevisionSequence::Committed() const
{
    return CommittedOrder;
}

std::uint64_t RevisionSequence::ScrubPosition() const
{
    return ScrubOrdinal;
}

bool RevisionSequence::TransactionOpen() const
{
    return OpenDeclared;
}

}   // namespace Slate
