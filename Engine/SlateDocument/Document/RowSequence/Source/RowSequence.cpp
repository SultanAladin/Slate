//============================================================================================================================================
//                                                             ROWSEQUENCE.CPP
//============================================================================================================================================
// 🧩 The depth-first walk, and the binary-indexed counts that answer both scroll questions.

#include "SlateDocument/Document/RowSequence/Api/RowSequence.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   COUNTED ORDERING
//------------------------------------------------------------------------------------------------------------------------

void RankIndex::Construct(std::uint32_t RowCount)
{
    SpannedRows = RowCount;
    CountedRows = RowCount;

    RowCounted.assign(RowCount, true);
    RunCounts.assign(static_cast<std::size_t>(RowCount) + 1u, 0u);

    // 📐 Built in one pass rather than by RowCount insertions: each stored word takes the count of the run
    //    ending at its ordinal, then hands that run's total to the word that contains it.
    for (std::uint32_t Ordinal = 1u; Ordinal <= RowCount; ++Ordinal)
    {
        RunCounts[Ordinal] += 1u;

        const std::uint32_t Containing = Ordinal + (Ordinal & (~Ordinal + 1u));

        if (Containing <= RowCount)
            RunCounts[Containing] += RunCounts[Ordinal];
    }
}

void RankIndex::Declare(std::uint32_t RowOrdinal, bool CountedEnabled)
{
    if (RowOrdinal >= SpannedRows)
        return;

    if (RowCounted[RowOrdinal] == CountedEnabled)
        return;

    RowCounted[RowOrdinal] = CountedEnabled;

    const std::uint32_t Adjustment = CountedEnabled ? 1u : 0u;

    for (std::uint32_t Ordinal = RowOrdinal + 1u;
         Ordinal <= SpannedRows;
         Ordinal += Ordinal & (~Ordinal + 1u))
    {
        if (Adjustment == 1u)
            ++RunCounts[Ordinal];
        else
            --RunCounts[Ordinal];
    }

    if (CountedEnabled)
        ++CountedRows;
    else
        --CountedRows;
}

std::uint32_t RankIndex::CountedBefore(std::uint32_t RowOrdinal) const
{
    std::uint32_t Bounded = RowOrdinal > SpannedRows ? SpannedRows : RowOrdinal;
    std::uint32_t Counted = 0u;

    while (Bounded != 0u)
    {
        Counted += RunCounts[Bounded];
        Bounded -= Bounded & (~Bounded + 1u);
    }

    return Counted;
}

Outcome<std::uint32_t> RankIndex::RowAtVisible(std::uint32_t VisibleOrdinal) const
{
    if (VisibleOrdinal >= CountedRows)
    {
        return Outcome<std::uint32_t>::Refuse(
            { RefusalReason::ExtentExhausted, "the visible position lies past the last counted row" });
    }

    // 📐 Descends the runs from the widest, taking each whose count the target still exceeds. The ordinal
    //    accumulated is the last row whose prefix count is not greater than the target.
    std::uint32_t Stride    = 1u;
    std::uint32_t Remaining = VisibleOrdinal;
    std::uint32_t Located   = 0u;

    while (Stride * 2u <= SpannedRows)
        Stride *= 2u;

    for (; Stride != 0u; Stride /= 2u)
    {
        const std::uint32_t Candidate = Located + Stride;

        if (Candidate <= SpannedRows && RunCounts[Candidate] <= Remaining)
        {
            Remaining -= RunCounts[Candidate];
            Located    = Candidate;
        }
    }

    return Outcome<std::uint32_t>::Result(Located);
}

Outcome<std::uint32_t> RankIndex::VisibleOfRow(std::uint32_t RowOrdinal) const
{
    if (RowOrdinal >= SpannedRows)
        return Outcome<std::uint32_t>::Refuse({ RefusalReason::ExtentExhausted, "the row lies outside the span" });

    if (!RowCounted[RowOrdinal])
    {
        return Outcome<std::uint32_t>::Refuse(
            { RefusalReason::ExtentExhausted, "the row is collapsed or narrowed out of the count" });
    }

    return Outcome<std::uint32_t>::Result(CountedBefore(RowOrdinal));
}

std::uint32_t RankIndex::CountedTotal() const
{
    return CountedRows;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    LINEARISATION
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📝 Appends one enclosure's ordering to the pending run, last owner first. The reversal is what makes a
//    run taken from the end reproduce the declared order.
void AppendReversed(std::vector<std::uint32_t>& Pending,
                    const SceneStructure&       Relations,
                    std::uint32_t               FirstInOrder)
{
    const std::size_t Appended = Pending.size();

    for (std::uint32_t Walking = FirstInOrder; Walking != AbsentSlot; Walking = Relations.NextInOrder(Walking))
        Pending.push_back(Walking);

    std::size_t Lower = Appended;
    std::size_t Upper = Pending.size();

    while (Lower + 1u < Upper)
    {
        --Upper;

        const std::uint32_t Held = Pending[Lower];
        Pending[Lower]           = Pending[Upper];
        Pending[Upper]           = Held;

        ++Lower;
    }
}

}   // namespace

Outcome<bool> RowSequence::Linearize(const SceneStructure& Relations)
{
    // 🔴 `12` §4 puts ④ before ⑤. Linearising against labels a relabel is still owed on produces an order
    //    that is briefly wrong, and briefly wrong here means displayed.
    if (Relations.RelabelOwed())
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::ExtentExhausted, "a label repair is owed before the sequence may be taken" });
    }

    SequencedRows.clear();
    RowOfSlot.assign(Relations.SpannedCount(), AbsentSlot);

    // 📝 🔴 Both per-slot declarations are resized rather than cleared. A collapse and a narrowing are the
    //    artist's standing decisions, and ⑤ runs on every tick — clearing them here would reopen every
    //    collapsed enclosure and widen every narrowing on the tick after the one that declared it.
    SlotCollapsed.resize(Relations.SpannedCount(), false);
    SlotRetained.resize(Relations.SpannedCount(), false);

    // 📝 The walk carries its own pending ordering rather than recursing, so enclosure depth is bounded by
    //    the relation's declared ceiling instead of by whatever call stack the walk happens to be given.
    //    Owners are appended in reverse so they come off the end in the order their enclosure declares
    //    them: depth-first order is the row order, and the two never diverge.
    std::vector<std::uint32_t> Pending;
    Pending.reserve(Relations.SpannedCount());

    AppendReversed(Pending, Relations, Relations.RootFirst());

    while (!Pending.empty())
    {
        const std::uint32_t SlotOrdinal = Pending.back();
        Pending.pop_back();

        SequencedRow Incoming;
        Incoming.Owner         = Relations.OwnerAt(SlotOrdinal);
        Incoming.EnclosureDepth   = Relations.EnclosureDepth(SlotOrdinal);
        Incoming.ExpansionEnabled = !SlotCollapsed[SlotOrdinal];

        std::uint32_t EnclosedCount = 0u;

        for (std::uint32_t Enclosed = Relations.FirstEnclosed(SlotOrdinal);
             Enclosed != AbsentSlot;
             Enclosed = Relations.NextInOrder(Enclosed))
        {
            ++EnclosedCount;
        }

        Incoming.EnclosedCount = EnclosedCount;

        RowOfSlot[SlotOrdinal] = static_cast<std::uint32_t>(SequencedRows.size());
        SequencedRows.push_back(Incoming);

        AppendReversed(Pending, Relations, Relations.FirstEnclosed(SlotOrdinal));
    }

    // 📝 🔴 A slot that holds no row holds no standing decision either. Clearing here is what makes a reused
    //    slot safe: without it a new owner inherits the collapse and the retention of whoever the slot
    //    carried before, and appears collapsed, or retained by a narrowing it was never confirmed against.
    for (std::uint32_t SlotOrdinal = 0u; SlotOrdinal < RowOfSlot.size(); ++SlotOrdinal)
    {
        if (RowOfSlot[SlotOrdinal] != AbsentSlot)
            continue;

        SlotCollapsed[SlotOrdinal] = false;
        SlotRetained[SlotOrdinal]  = false;
    }

    Recount();

    return Outcome<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                               EXPANSION AND NARROWING
//------------------------------------------------------------------------------------------------------------------------

// 📝 No enclosure is collapsed above the row being counted. Spelled as a constant rather than repeated,
//    because it is compared against a depth and every enclosure depth is below the relation's ceiling.
namespace
{

constexpr std::uint32_t NothingCollapsed = 0xFFFFFFFFu;   // [-] - no collapsing enclosure stands above

}   // namespace

void RowSequence::Recount()
{
    VisibleOrdering.Construct(static_cast<std::uint32_t>(SequencedRows.size()));

    // 📝 A row leaves the count when it is narrowed, or when any row it sits under is collapsed. The
    //    collapsing depth is carried down the sequence so the whole derivation is one pass over the rows.
    std::uint32_t CollapsedAtDepth = NothingCollapsed;

    for (std::size_t Ordinal = 0u; Ordinal < SequencedRows.size(); ++Ordinal)
    {
        SequencedRow& Held = SequencedRows[Ordinal];

        if (Held.EnclosureDepth <= CollapsedAtDepth)
            CollapsedAtDepth = NothingCollapsed;

        // 📝 A narrowing narrows by retention rather than by exclusion, so an owner registered in nothing
        //    leaves the count while one stands. `12` §10 makes it a subset, and the subset is what is kept.
        const bool Retained = !NarrowingHeld
                           || (Held.Owner.SlotOrdinal < SlotRetained.size()
                            && SlotRetained[Held.Owner.SlotOrdinal]);

        Held.VisibleInCount = CollapsedAtDepth == NothingCollapsed && Retained;

        VisibleOrdering.Declare(static_cast<std::uint32_t>(Ordinal), Held.VisibleInCount);

        if (!Held.ExpansionEnabled && Held.EnclosureDepth < CollapsedAtDepth)
            CollapsedAtDepth = Held.EnclosureDepth;
    }
}

Outcome<bool> RowSequence::DeclareExpansion(OwnerIdentity Subject, bool ExpansionEnabled)
{
    const Outcome<std::uint32_t> Located = RowOf(Subject);

    if (!Located.Resolved)
        return Outcome<bool>::Refuse(Located.Error);

    // 🔴 Declared against the slot as well as the row. The row is rebuilt at every ⑤ and the slot is not, so
    //    holding it on the row alone would reopen the enclosure on the tick after the artist collapsed it.
    SequencedRows[Located.Resolve()].ExpansionEnabled = ExpansionEnabled;
    SlotCollapsed[Subject.SlotOrdinal]                = !ExpansionEnabled;

    Recount();

    return Outcome<bool>::Result(true);
}

Outcome<bool> RowSequence::DeclareNarrowing(const std::vector<OwnerIdentity>& Retained, bool NarrowingDeclared)
{
    // 📝 Withdrawing the narrowing ignores what was retained rather than requiring the whole population to be
    //    handed back. Every row returns to the count, which is the one thing an empty search text means.
    if (!NarrowingDeclared)
    {
        NarrowingHeld = false;

        SlotRetained.assign(SlotRetained.size(), false);

        Recount();

        return Outcome<bool>::Result(true);
    }

    // 📝 Confirmed against the rows before anything is written, so a stale identity refuses the whole
    //    narrowing rather than leaving a subset that retains part of what the search found.
    for (const OwnerIdentity& Confirming : Retained)
    {
        if (!RowOf(Confirming).Resolved)
        {
            return Outcome<bool>::Refuse(
                { RefusalReason::IdentityStale, "a retained owner holds no row in this sequence" });
        }
    }

    SlotRetained.assign(SlotRetained.size(), false);

    for (const OwnerIdentity& Retaining : Retained)
        SlotRetained[Retaining.SlotOrdinal] = true;

    NarrowingHeld = true;

    Recount();

    return Outcome<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

const std::vector<SequencedRow>& RowSequence::Rows() const
{
    return SequencedRows;
}

const RankIndex& RowSequence::Counted() const
{
    return VisibleOrdering;
}

Outcome<std::uint32_t> RowSequence::RowOf(OwnerIdentity Subject) const
{
    if (!Subject.IdentityDeclared() || Subject.SlotOrdinal >= RowOfSlot.size())
        return Outcome<std::uint32_t>::Refuse({ RefusalReason::IdentityStale, "the owner holds no row" });

    const std::uint32_t RowOrdinal = RowOfSlot[Subject.SlotOrdinal];

    if (RowOrdinal == AbsentSlot || SequencedRows[RowOrdinal].Owner != Subject)
        return Outcome<std::uint32_t>::Refuse({ RefusalReason::IdentityStale, "the owner holds no row" });

    return Outcome<std::uint32_t>::Result(RowOrdinal);
}

bool RowSequence::NarrowingCurrent() const
{
    return NarrowingHeld;
}

bool RowSequence::CountsAgree() const
{
    std::uint32_t Counted = 0u;

    for (const SequencedRow& Held : SequencedRows)
    {
        if (Held.VisibleInCount)
            ++Counted;
    }

    if (Counted != VisibleOrdering.CountedTotal())
        return false;

    std::uint32_t Walking = 0u;

    for (std::size_t Ordinal = 0u; Ordinal < SequencedRows.size(); ++Ordinal)
    {
        if (!SequencedRows[Ordinal].VisibleInCount)
            continue;

        const Outcome<std::uint32_t> Located = VisibleOrdering.RowAtVisible(Walking);

        if (!Located.Resolved || Located.Resolve() != static_cast<std::uint32_t>(Ordinal))
            return false;

        ++Walking;
    }

    return true;
}

}   // namespace Slate
