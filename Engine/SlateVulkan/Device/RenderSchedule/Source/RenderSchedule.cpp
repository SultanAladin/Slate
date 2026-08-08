//============================================================================================================================================
//                                                            RENDERSCHEDULE.CPP
//============================================================================================================================================
// 🧩 Contribution gating and the ordering derived from declared reads and writes.

#include "SlateVulkan/Device/RenderSchedule/Api/RenderSchedule.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  TARGET DECLARATION
//------------------------------------------------------------------------------------------------------------------------

// 📝 Extent relation per target, declared here once. A resize recreates every display-relative and
//    fraction-of-display target and no absolute one — `06` §4.1 ④ depends on this table being total.
namespace
{
    constexpr ExtentRelation RelationOf[static_cast<std::size_t>(SharedTarget::TargetCount)] =
    {
        ExtentRelation::DisplayRelative,    // DepthSurface
        ExtentRelation::DisplayRelative,    // VisibilityIndex
        ExtentRelation::DisplayRelative,    // OccupancySurface
        ExtentRelation::DisplayRelative,    // MotionSurface
        ExtentRelation::FractionOfDisplay,  // OcclusionSurface
        ExtentRelation::DisplayRelative,    // DirectOcclusionSurface
        ExtentRelation::DisplayRelative,    // TransmissionIndex
        ExtentRelation::DisplayRelative,    // RadianceSurface
        ExtentRelation::FractionOfDisplay,  // ReflectionSurface
        ExtentRelation::DisplayRelative,    // AccumulationSurface
        ExtentRelation::DisplayRelative,    // DisplaySurface
        ExtentRelation::DisplayRelative,    // OutlineSurface
        ExtentRelation::Absolute,           // TransmittanceSurface
        ExtentRelation::Absolute,           // MultiScatterSurface
        ExtentRelation::Absolute            // SkyViewSurface
    };

    constexpr std::size_t TargetSpan = static_cast<std::size_t>(SharedTarget::TargetCount);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     CONTRIBUTION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> RenderSchedule::Contribute(const DeclaredRecording& Arriving)
{
    if (OrderingFixed)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the ordering is already fixed" });

    // 📝 🔴 A capability requirement with no substitution is rejected here rather than discovered at the
    //    recording site. The substitution is a design decision belonging to the contributing document.
    if (Arriving.CapabilityRequired && (Arriving.Substitution == nullptr || Arriving.Substitution[0] == '\0'))
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::CapabilityAbsent, "a capability is required with no declared substitution" });
    }

    for (const SharedTarget Produced : Arriving.Produces)
    {
        const std::size_t TargetOrdinal = static_cast<std::size_t>(Produced);

        if (TargetOrdinal >= TargetSpan)
            return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such shared target" });

        // 📝 One producing recording per target. An amender declares itself in Amends and takes its place
        //    in the ordered amendment list instead — `08` §2's Amended by column.
        if (ProducerOf[TargetOrdinal].IdentityDeclared())
        {
            return Outcome<bool>::Refuse(
                { RefusalReason::HostDenied, "the target already declares a producing recording" });
        }

        ProducerOf[TargetOrdinal].SlotOrdinal    = static_cast<std::uint32_t>(ContributedOrder.size());
        ProducerOf[TargetOrdinal].SlotGeneration = 1u;
    }

    ContributedOrder.push_back(Arriving);
    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       ORDERING
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> RenderSchedule::Fix()
{
    if (OrderingFixed)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the ordering is already fixed" });

    OrderedRecordings.clear();
    OrderedRecordings.reserve(ContributedOrder.size());

    std::vector<bool> Placed(ContributedOrder.size(), false);
    std::vector<bool> Available(TargetSpan, false);

    // 📝 The order is derived rather than authored: a recording is placed once every target it reads is
    //    either produced already or produced by nothing at all. Scene-referred recordings are exhausted
    //    before any display-referred one is placed, which is the tone line in `08` §3.1.
    for (int DisplayPhase = 0; DisplayPhase < 2; ++DisplayPhase)
    {
        const bool PlacingDisplayReferred = DisplayPhase == 1;
        bool       Advanced               = true;

        while (Advanced)
        {
            Advanced = false;

            for (std::size_t Ordinal = 0u; Ordinal < ContributedOrder.size(); ++Ordinal)
            {
                if (Placed[Ordinal])
                    continue;

                const DeclaredRecording& Candidate = ContributedOrder[Ordinal];

                if (Candidate.DisplayReferred != PlacingDisplayReferred)
                    continue;

                bool ReadsSatisfied = true;

                for (const SharedTarget Consumed : Candidate.Reads)
                {
                    const std::size_t TargetOrdinal = static_cast<std::size_t>(Consumed);

                    if (ProducerOf[TargetOrdinal].IdentityDeclared() && !Available[TargetOrdinal])
                    {
                        ReadsSatisfied = false;
                        break;
                    }
                }

                if (!ReadsSatisfied)
                    continue;

                OrderedRecordings.push_back(Candidate);
                Placed[Ordinal] = true;
                Advanced        = true;

                for (const SharedTarget Produced : Candidate.Produces)
                    Available[static_cast<std::size_t>(Produced)] = true;
            }
        }
    }

    if (OrderedRecordings.size() != ContributedOrder.size())
    {
        // 📝 A recording that never became placeable reads a target whose producer reads it back. The
        //    orderer reports it here rather than emitting an ordering that silently drops the recording.
        OrderedRecordings.clear();
        return Outcome<bool>::Refuse(
            { RefusalReason::HostDenied, "a recording reads a target no ordering makes available" });
    }

    OrderingFixed = true;
    return Outcome<bool>::Deliver(true);
}

const std::vector<DeclaredRecording>& RenderSchedule::Ordered() const
{
    return OrderedRecordings;
}

}   // namespace Slate
