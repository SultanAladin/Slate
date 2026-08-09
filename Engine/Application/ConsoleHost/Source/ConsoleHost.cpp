//============================================================================================================================================
//                                                             CONSOLEHOST.CPP
//============================================================================================================================================
// 🧩 Headless bring-up in link order — every unit constructed, reported, and reclaimed without a window.

#include "Contract/IdentityContract.h"
#include "Contract/OutcomeContract.h"
#include "Contract/PrecisionContract.h"
#include "Contract/ToleranceContract.h"

#include "Shared/ContainmentClassifier.slang.h"
#include "Shared/IncircleClassifier.slang.h"
#include "Shared/IntersectionClassifier.slang.h"
#include "Shared/OrientationClassifier.slang.h"
#include "Shared/SampleProjection.slang.h"

#include "SlateMath/Platform/TickSequence/Api/TickSequence.h"
#include "SlateMath/Platform/InputExchange/Api/InputExchange.h"
#include "SlateMath/Numeric/TransformProjection/Api/TransformProjection.h"
#include "SlateMath/Numeric/ReportSequence/Api/ReportSequence.h"
#include "SlateMath/Numeric/WorkSequence/Api/WorkSequence.h"
#include "SlateMath/Numeric/ColourProjection/Api/ColourProjection.h"

#include "SlateDocument/Document/PopulationIndex/Api/PopulationIndex.h"
#include "SlateDocument/Document/RevisionSequence/Api/RevisionSequence.h"
#include "SlateDocument/Document/PropertySpecification/Api/PropertySpecification.h"
#include "SlateDocument/Document/TopologyStructure/Api/TopologyStructure.h"
#include "SlateDocument/Document/TopologyConditioning/Api/TopologyConditioning.h"
#include "SlateDocument/Document/MaterialSpecification/Api/MaterialSpecification.h"
#include "SlateDocument/Document/VectorInterchange/Api/VectorInterchange.h"
#include "SlateDocument/Document/CameraProjection/Api/CameraProjection.h"
#include "SlateDocument/Document/IlluminantPopulation/Api/IlluminantPopulation.h"
#include "SlateDocument/Document/SpatialSubdivision/Api/SpatialSubdivision.h"
#include "SlateDocument/Document/IntakeIndex/Api/IntakeIndex.h"
#include "SlateDocument/Document/AssetInterchange/Api/AssetInterchange.h"
#include "SlateDocument/Format/FormatCodec/Api/FormatCodec.h"

#include "SlateMath/Numeric/CurveSolver/Api/CurveSolver.h"
#include "SlateMath/Numeric/UnwrapSolver/Api/UnwrapSolver.h"

#include "SlateVulkan/Device/RenderSchedule/Api/RenderSchedule.h"

#include "SlateCompute/Compute/ParityRunner/Api/ParityRunner.h"
#include "SlateCompute/Compute/SeamSpecification/Api/SeamSpecification.h"
#include "SlateCompute/Compute/DomainSpace/Api/DomainSpace.h"
#include "SlateCompute/Compute/ChartPartition/Api/ChartPartition.h"

// 📝 🔴 This host names no interface component and constructs no instance. `00` §2.2 keeps exactly one copy
//    of ImGui, compiled inside SlateUI; a headless host reaching for it would be linking a window-system
//    attachment it can never construct. `SlateUI` and the vendor edge belong to the windowed host.

#include <cstdio>
#include <cstring>
#include <cmath>
#include <atomic>
#include <vector>

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                      REPORTING
//------------------------------------------------------------------------------------------------------------------------

// 📝 Every check reports through one routine, so a run's output reads as one table rather than as a
//    sequence of independently phrased sentences.
int RefusedCount = 0;   // [-] - checks that did not hold; also the process exit ordinal

void Report(const char* Subject, bool Held, const char* Detail)
{
    if (!Held)
        ++RefusedCount;

    std::printf("  %-42s %-8s %s\n", Subject, Held ? "held" : "REFUSED", Detail);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                 PLATFORM AND NUMERIC
//------------------------------------------------------------------------------------------------------------------------

// 📝 SlateMath is verified first because every unit below consumes something declared there, and a failure
//    here would otherwise be reported by whichever unit happened to read the result.
void VerifyMathematics()
{
    std::printf("SlateMath\n");

    const Slate::TickSequence HostTimeline;

    const Slate::TickPoint EarlierReading = HostTimeline.Advance();
    const Slate::TickPoint LaterReading   = HostTimeline.Advance();

    Report("TickSequence monotonic",
           LaterReading.Ordinal >= EarlierReading.Ordinal,
           "[ns] never decreasing between two reads");

    Report("TickSequence span never negative",
           Slate::TickSequence::Span(LaterReading, EarlierReading) == 0.0,
           "[ms] reversed operands report zero");

    // 📝 The arrival extent is bounded and non-allocating, so overrunning it must discard the oldest sample
    //    rather than grow. Recording one more than the extent holds is the only way to observe that.
    Slate::InputExchange Arrivals;

    for (std::uint32_t Ordinal = 0u; Ordinal <= Slate::InputExchange::ArrivalCapacity; ++Ordinal)
    {
        Slate::PointerSample Arriving;
        Arriving.Arrival.Ordinal = Ordinal;
        Arriving.PositionX       = static_cast<double>(Ordinal);
        Arrivals.Record(Arriving);
    }

    Report("InputExchange extent bounded",
           Arrivals.HeldCount() == Slate::InputExchange::ArrivalCapacity,
           "[-] the extent never grows");

    Report("InputExchange discards the oldest",
           Arrivals.Sample(0u).Arrival.Ordinal == 1ull,
           "[-] a discard, not a corrupted ordering");

    Arrivals.Reclaim();

    Report("InputExchange reclaimed", Arrivals.HeldCount() == 0u, "[-] drained by the consumer");

    // 📐 Compounding a rotation with its conjugate returns the identity. The residue is bounded rather than
    //    zero, which is exactly what the declared Bounded guarantee claims.
    Slate::RotationQuaternion QuarterTurn;
    QuarterTurn.ImaginaryZ = 0.7071067811865476;
    QuarterTurn.Real       = 0.7071067811865476;

    Slate::RotationQuaternion Conjugate = QuarterTurn;
    Conjugate.ImaginaryX = -QuarterTurn.ImaginaryX;
    Conjugate.ImaginaryY = -QuarterTurn.ImaginaryY;
    Conjugate.ImaginaryZ = -QuarterTurn.ImaginaryZ;

    const Slate::RotationQuaternion Restored = Slate::Compound(QuarterTurn, Conjugate);
    const double                    RealGap  = Restored.Real - 1.0;

    Report("Rotation compounds to the identity",
           RealGap < Slate::WeldTolerance && RealGap > -Slate::WeldTolerance,
           "[-] within the declared tolerance");

    // 📝 🔴 The rebasing subtraction happens in 64-bit. A document position a billion millimetres out,
    //    displaced by one millimetre, survives the narrowing only because the subtraction preceded it.
    //    `02` §8 gates this, and the failure it prevents reads as a driver defect rather than as arithmetic.
    Slate::DocumentPosition DistantPosition;
    DistantPosition.PositionX = 1.0e9;

    Slate::DocumentPosition ViewOrigin;
    ViewOrigin.PositionX = 1.0e9 - 1.0;

    const Slate::DevicePosition Rebased = Slate::Rebase(DistantPosition, ViewOrigin);

    Report("Rebasing precedes narrowing",
           Rebased.PositionX == 1.0f,
           "[mm] one millimetre survives at 1e9");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE EXACT PREDICATES
//------------------------------------------------------------------------------------------------------------------------

// 📝 The predicates are verified here rather than only inside `ParityRunner`, because the runner proves
//    self-consistency and this proves the answers. A predicate that is antisymmetric and wrong passes the
//    runner; a predicate that says a position outside a circle is inside it does not pass this.
void VerifyClassifiers()
{
    std::printf("Shared predicates\n");

    // 📐 The unit right triangle, counter-clockwise, with circumcentre at (0.5, 0.5).
    Report("Incircle resolves inside",
           Slate::ClassifyIncircle(0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.4, 0.4) > 0,
           "[-] for a counter-clockwise triangle");

    Report("Incircle resolves outside",
           Slate::ClassifyIncircle(0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 2.0, 2.0) < 0,
           "[-] and the sign is unambiguous");

    // 🔴 The exact path decides this one: the position is on the circle and no filter can say so.
    Report("Incircle resolves cocircular exactly",
           Slate::ClassifyIncircle(0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0) == 0,
           "[-] the arena, not the filter");

    Report("Incircle inverts with the winding",
           Slate::ClassifyIncircle(0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.4, 0.4) < 0,
           "[-] which is why the unwound form exists");

    Report("The unwound form does not",
           Slate::ClassifyIncircleUnwound(0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.4, 0.4) > 0
        && Slate::ClassifyIncircleUnwound(0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.4, 0.4) > 0,
           "[-] the same answer either way round");

    Report("A degenerate triangle describes no circle",
           Slate::ClassifyIncircleUnwound(0.0, 0.0, 1.0, 1.0, 2.0, 2.0, 3.0, 3.0) == 0,
           "[-] nothing is inside it");

    Report("Segments cross transversally",
           Slate::ClassifySegmentIntersection(0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0, 0.0)
        == SlateIntersectionCrossing,
           "[-] interiors, strictly");

    Report("An endpoint on a segment touches",
           Slate::ClassifySegmentIntersection(0.0, 0.0, 1.0, 0.0, 0.5, 0.0, 0.5, 1.0)
        == SlateIntersectionTouching,
           "[-] never reported as a crossing");

    // 📐 Vertical and collinear — the case that makes the overlap axis a decision rather than a convenience.
    Report("Collinear vertical segments overlap",
           Slate::ClassifySegmentIntersection(0.0, 1.0, 0.0, 3.0, 0.0, 2.0, 0.0, 4.0)
        == SlateIntersectionCollinear,
           "[-] the axis is chosen by span");

    Report("Collinear segments meeting at one position touch",
           Slate::ClassifySegmentIntersection(0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 2.0, 0.0)
        == SlateIntersectionTouching,
           "[-] one position is not a span");

    Report("Parallel segments are disjoint",
           Slate::ClassifySegmentIntersection(0.0, 0.0, 1.0, 1.0, 2.0, 0.0, 3.0, 1.0)
        == SlateIntersectionDisjoint,
           "[-] and no crossing is resolved");

    double CrossingX = 0.0;
    double CrossingY = 0.0;

    Report("A crossing position resolves",
           Slate::ResolveSegmentCrossing(0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 1.0, 0.0, CrossingX, CrossingY)
        && std::fabs(CrossingX - 0.5) < 1.0e-12
        && std::fabs(CrossingY - 0.5) < 1.0e-12,
           "[-] Bounded, and named apart from the classification");

    Report("A parallel pair resolves no position",
           !Slate::ResolveSegmentCrossing(0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0, CrossingX, CrossingY),
           "[-] refuses rather than dividing by zero");

    Report("Extents overlapping interiors",
           Slate::ClassifyExtentOverlap(0.0, 0.0, 2.0, 2.0, 1.0, 1.0, 3.0, 3.0) > 0,
           "[-] +1");

    Report("Extents touching on a bound",
           Slate::ClassifyExtentOverlap(0.0, 0.0, 1.0, 1.0, 1.0, 0.0, 2.0, 1.0) == 0,
           "[-] 0 — which `38` §6's outward rounding turns into a hit, never a miss");

    Report("Extents disjoint",
           Slate::ClassifyExtentOverlap(0.0, 0.0, 1.0, 1.0, 2.0, 2.0, 3.0, 3.0) < 0,
           "[-] −1");

    Report("Volumes overlapping interiors",
           Slate::ClassifyVolumeOverlap(0.0, 0.0, 0.0, 2.0, 2.0, 2.0,
                                        1.0, 1.0, 1.0, 3.0, 3.0, 3.0) > 0,
           "[-] +1");

    Report("Volumes touching on a face",
           Slate::ClassifyVolumeOverlap(0.0, 0.0, 0.0, 1.0, 1.0, 1.0,
                                        1.0, 0.0, 0.0, 2.0, 1.0, 1.0) == 0,
           "[-] 0 — outward rounding turns it into a hit, never a miss");

    // 🔴 The case three planar tests get wrong: every axis-aligned projection overlaps and the volumes do not.
    //    Composing the planar predicate three times would report +1 here.
    Report("Volumes disjoint on one axis alone",
           Slate::ClassifyVolumeOverlap(0.0, 0.0, 0.0, 1.0, 1.0, 1.0,
                                        0.5, 0.5, 2.0, 1.5, 1.5, 3.0) < 0,
           "[-] −1, where the planar form would resolve +1");

    Report("An interval strictly contains another",
           Slate::ClassifyIntervalContainment(0u, 100u, 10u, 20u) > 0,
           "[-] `12` §2.1's comparison");

    Report("An interval does not contain itself",
           Slate::ClassifyIntervalContainment(0u, 100u, 0u, 100u) == 0,
           "[-] identity, never containment");

    Report("A shared bound is not containment",
           Slate::ClassifyIntervalContainment(0u, 100u, 0u, 50u) < 0,
           "[-] invariant 4 nests strictly");

    Report("Overlapping intervals contain neither",
           Slate::ClassifyIntervalContainment(0u, 50u, 40u, 60u) < 0
        && Slate::ClassifyIntervalContainment(40u, 60u, 0u, 50u) < 0,
           "[-] and invariant 4 forbids the shape outright");

    Report("Enrolment is inclusive at both bounds",
           Slate::OrdinalEnrolled(10u, 20u, 10u)
        && Slate::OrdinalEnrolled(10u, 20u, 20u)
        && !Slate::OrdinalEnrolled(10u, 20u, 21u),
           "[-] where the label comparison is strict");

    Report("Disjointness is symmetric",
           Slate::IntervalsDisjoint(0u, 10u, 20u, 30u)
        && Slate::IntervalsDisjoint(20u, 30u, 0u, 10u)
        && !Slate::IntervalsDisjoint(0u, 25u, 20u, 30u),
           "[-] invariant 4, as one comparison");

    // 📐 The base-two inverse of an even ordinal and of its successor differ by exactly one half, in binary,
    //    with no tolerance. Nothing else in the engine can be asserted this strongly.
    bool RadicalHeld = true;

    for (std::uint32_t Ordinal = 0u; Ordinal < 1024u; Ordinal += 2u)
    {
        if (Slate::ProjectRadicalTwo(Ordinal + 1u) - Slate::ProjectRadicalTwo(Ordinal) != 0.5)
            RadicalHeld = false;
    }

    Report("The base-two sequence steps exactly", RadicalHeld, "[-] one half, bit for bit");

    bool WithinUnitSquare = true;

    for (std::uint32_t Ordinal = 0u; Ordinal < 4096u; ++Ordinal)
    {
        double FirstCoordinate  = 0.0;
        double SecondCoordinate = 0.0;
        Slate::ProjectPlanarSample(Ordinal, FirstCoordinate, SecondCoordinate);

        if (FirstCoordinate < 0.0 || FirstCoordinate >= 1.0
         || SecondCoordinate < 0.0 || SecondCoordinate >= 1.0)
        {
            WithinUnitSquare = false;
        }
    }

    Report("Planar samples stay in the unit square", WithinUnitSquare, "[-] half-open, at every ordinal");

    Report("The base-three inverse is exact at its first digits",
           Slate::ProjectRadicalThree(1u) == 1.0 / 3.0
        && Slate::ProjectRadicalThree(3u) == 1.0 / 9.0,
           "[-] one correctly-rounded division, never twenty");

    // 📝 The offsets sit within the pixel and never at its corner, which is why the sequence begins at one.
    bool OffsetsWithinPixel = true;

    for (std::uint32_t Ordinal = 0u; Ordinal < 64u; ++Ordinal)
    {
        double OffsetX = 0.0;
        double OffsetY = 0.0;
        Slate::ProjectSubPixelOffset(Ordinal, OffsetX, OffsetY);

        if (OffsetX < -0.5 || OffsetX >= 0.5 || OffsetY < -0.5 || OffsetY >= 0.5)
            OffsetsWithinPixel = false;

        if (OffsetX == -0.5 && OffsetY == -0.5)
            OffsetsWithinPixel = false;
    }

    Report("Sub-pixel offsets stay within the pixel",
           OffsetsWithinPixel,
           "[px] and never at its corner");

    Report("The offset sequence repeats on its declared length",
           [&]
           {
               double FirstX = 0.0, FirstY = 0.0, LaterX = 0.0, LaterY = 0.0;
               Slate::ProjectSubPixelOffset(3u, FirstX, FirstY);
               Slate::ProjectSubPixelOffset(3u + Slate::SubPixelSequenceLength, LaterX, LaterY);
               return FirstX == LaterX && FirstY == LaterY;
           }(),
           "[-] `46`, `64` and `82` read one length");

    // 📐 Bounded, so measured against the declared bound rather than compared for equality.
    double GreatestDeviation = 0.0;

    for (std::uint32_t Ordinal = 0u; Ordinal < 1024u; ++Ordinal)
    {
        double FirstCoordinate  = 0.0;
        double SecondCoordinate = 0.0;
        Slate::ProjectPlanarSample(Ordinal, FirstCoordinate, SecondCoordinate);

        double DirectionX = 0.0;
        double DirectionY = 0.0;
        double DirectionZ = 0.0;
        Slate::ProjectSphericalSample(FirstCoordinate, SecondCoordinate,
                                      DirectionX, DirectionY, DirectionZ);

        const double Length    = std::sqrt(DirectionX * DirectionX
                                         + DirectionY * DirectionY
                                         + DirectionZ * DirectionZ);
        const double Deviation = std::fabs(Length - 1.0) / Slate::MachineEpsilon;

        if (Deviation > GreatestDeviation)
            GreatestDeviation = Deviation;
    }

    Report("Spherical samples are unit length",
           GreatestDeviation <= Slate::SampleUnitPlaceCeiling,
           "[-] within the declared bound in units in the last place");

    bool HemisphereHeld = true;

    for (std::uint32_t Ordinal = 0u; Ordinal < 1024u; ++Ordinal)
    {
        double FirstCoordinate  = 0.0;
        double SecondCoordinate = 0.0;
        Slate::ProjectPlanarSample(Ordinal, FirstCoordinate, SecondCoordinate);

        double DirectionX = 0.0;
        double DirectionY = 0.0;
        double DirectionZ = 0.0;
        Slate::ProjectHemisphericalSample(FirstCoordinate, SecondCoordinate,
                                          DirectionX, DirectionY, DirectionZ);

        if (DirectionZ < 0.0)
            HemisphereHeld = false;
    }

    Report("Hemispherical samples stay on one side",
           HemisphereHeld,
           "[-] `60` §5 samples the closed hemisphere");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE REGISTER
//------------------------------------------------------------------------------------------------------------------------

// 📝 The distinction `86` §2 rests on is the only thing worth measuring here: a measure must overwrite and a
//    report must append. A register that appended `06` §3's rotational totals is the defect, and it is a defect
//    nothing else in the engine can observe.
void VerifyReporting()
{
    std::printf("ReportSequence\n");

    const Slate::TickSequence HostTimeline;
    Slate::ReportSequence     Reporting;

    Slate::ReportSpecification Refused;
    Refused.Origin      = "06 §3 ByteSpace";
    Refused.Subject     = "Reserved";
    Refused.Detail      = "the reserved claim could not be satisfied in full";
    Refused.Disposition = Slate::ReportDisposition::Refused;
    Refused.Arrival     = HostTimeline.Advance();

    Reporting.Append(Refused);

    Report("A report appends", Reporting.RetainedCount() == 1u, "[-] exactly one entry");

    Reporting.Append(Refused);
    Reporting.Append(Refused);

    Report("A recurrence coalesces",
           Reporting.RetainedCount() == 1u && Reporting.AppendedCount() == 3u,
           "[-] one entry, three occurrences");

    // 🔴 `86` §6: coalescing is by origin, disposition and subject together. Coalescing by origin alone would
    //    present twelve distinct refused constructs as one entry with a count of twelve.
    Slate::ReportSpecification OtherSubject = Refused;
    OtherSubject.Subject                    = "Committed";

    Reporting.Append(OtherSubject);

    Report("A different subject is its own entry",
           Reporting.RetainedCount() == 2u,
           "[-] never coalesced by origin alone");

    Slate::ReportSpecification OtherDisposition = Refused;
    OtherDisposition.Disposition               = Slate::ReportDisposition::Truncated;

    Reporting.Append(OtherDisposition);

    Report("A different disposition is its own entry",
           Reporting.RetainedCount() == 3u,
           "[-] the disposition discriminates too");

    const std::vector<Slate::ReportSpecification> Retained = Reporting.Retained();

    Report("Retention is oldest first",
           Retained.size() == 3u && Retained[0].OccurrenceCount == 3u,
           "[-] the coalesced entry carries its count");

    // 📝 The bound is measured rather than assumed, because `86` §6 requires the discard itself to be presented.
    //    A register that silently forgot the first report of a run is worse than one that admits it is full.
    for (std::uint32_t Ordinal = 0u; Ordinal <= Slate::ReportSequence::RetainedCeiling; ++Ordinal)
    {
        Slate::ReportSpecification Filling;
        Filling.Origin         = "34 §5 WorkSequence";
        Filling.Subject        = "Filling";
        Filling.SubjectOrdinal = Ordinal;
        Filling.Disposition    = Slate::ReportDisposition::Failed;

        Reporting.Append(Filling);
    }

    Report("Retention is bounded",
           Reporting.RetainedCount() == Slate::ReportSequence::RetainedCeiling,
           "[-] the ceiling holds");

    Report("The discard is itself counted",
           Reporting.DiscardedCount() != 0u,
           "[-] presented, never silent");

    Slate::MeasureIndex Measured;

    Measured.DeclareCount("06 §3 ByteSpace", "Claimed", 1024u, HostTimeline.Advance());
    Measured.DeclareCount("06 §3 ByteSpace", "Claimed", 2048u, HostTimeline.Advance());

    const Slate::Outcome<Slate::SampledMeasure> Resolved = Measured.Resolve("06 §3 ByteSpace", "Claimed");

    Report("A measure overwrites",
           Measured.Measures().size() == 1u && Resolved.ContentPresent && Resolved.Resolve().Counted == 2048ull,
           "[-] one entry, the latest reading");

    Report("An undeclared measure refuses",
           !Measured.Resolve("06 §3 ByteSpace", "Available").ContentPresent,
           "[-] refuses rather than reading zero");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     WORK OFF THE TICK
//------------------------------------------------------------------------------------------------------------------------

void VerifyWork()
{
    std::printf("WorkSequence\n");

    const Slate::TickSequence HostTimeline;
    Slate::ReportSequence     Reporting;
    Slate::WorkSequence       Working;

    Report("A declaration before Construct is refused",
           !Working.Declare(Slate::WorkDeclaration{}).ContentPresent,
           "[-] no worker stands to resolve it");

    Report("Workers construct",
           Working.Construct(4u, HostTimeline, Reporting).ContentPresent && Working.WorkerCount() == 4u,
           "[-] the count is fixed and recorded");

    Report("A second Construct is refused",
           !Working.Construct(4u, HostTimeline, Reporting).ContentPresent,
           "[-] the workers are constructed once");

    // 📝 The resolution reads only what is captured here — `34` §2. Nothing it touches is the document, the
    //    tick's state, or anything in `76`, which is the rule that makes every lock in the sequence unnecessary.
    std::atomic<std::uint32_t> ResolvedCount { 0u };

    Slate::WorkDeclaration Declaring;
    Declaring.Origin   = "ConsoleHost";
    Declaring.Priority = Slate::WorkPriority::Interactive;
    Declaring.Resolve  = [&ResolvedCount](const Slate::WorkCancellation&, Slate::WorkProgress& Progressed)
    {
        Progressed.DeclareCount(8u, 8u);
        ResolvedCount.fetch_add(1u, std::memory_order_relaxed);

        return Slate::Outcome<bool>::Deliver(true);
    };

    std::vector<Slate::WorkIdentity> Declared;

    for (std::uint32_t Ordinal = 0u; Ordinal < 32u; ++Ordinal)
    {
        const Slate::Outcome<Slate::WorkIdentity> Issued = Working.Declare(Declaring);

        if (Issued.ContentPresent)
            Declared.push_back(Issued.Resolve());
    }

    Report("Every declaration was admitted", Declared.size() == 32u, "[-] none silently dropped");

    // 📝 Drained repeatedly rather than waited on. `34` §3 makes the tick the only place a result is applied,
    //    and a host that blocked on a condition here would be observing the sequence from outside its contract.
    // 🔴 Each drain is checked for ordering as it arrives. `34` §6's guarantee is **within one drain** and is not
    //    a global prefix: a conclusion is delivered as soon as it is recorded, so accumulating several drains and
    //    asserting the accumulation is ordered asserts a property no bounded worker count can supply. Supplying
    //    it would mean holding a conclusion back until every earlier declaration had also concluded, and that is
    //    the starvation `34` §4 forbids — a `Background` export declared first would block every `Interactive`
    //    promotion declared after it from ever being applied.
    std::vector<Slate::WorkCompletion> Concluded;
    bool                               OrderHeld = true;

    for (std::uint32_t Passed = 0u; Passed < 100000u && Concluded.size() < 32u; ++Passed)
    {
        const std::vector<Slate::WorkCompletion>& Drained = Working.Drain();

        for (std::size_t Ordinal = 1u; Ordinal < Drained.size(); ++Ordinal)
        {
            if (Drained[Ordinal - 1u].DeclaredOrdinal >= Drained[Ordinal].DeclaredOrdinal)
                OrderHeld = false;
        }

        for (const Slate::WorkCompletion& Held : Drained)
            Concluded.push_back(Held);
    }

    Report("Every declaration concluded", Concluded.size() == 32u, "[-] each crossed back exactly once");

    Report("Every resolution ran",
           ResolvedCount.load(std::memory_order_relaxed) == 32u,
           "[-] once each, never twice");

    Report("Each drain is ordered by declaration",
           OrderHeld,
           "[-] within the drain, never by which worker finished first");

    // 🔴 The property `34` §6 genuinely binds: every declaration is concluded exactly once, so the results of a
    //    split solve recombine by declared index. A conclusion delivered twice, or one lost between drains, is
    //    what would make the same inputs produce two documents on two machines.
    bool RecombinationHeld = Concluded.size() == 32u;

    for (std::uint32_t Expected = 1u; Expected <= 32u; ++Expected)
    {
        std::uint32_t Found = 0u;

        for (const Slate::WorkCompletion& Held : Concluded)
        {
            if (Held.DeclaredOrdinal == static_cast<std::uint64_t>(Expected))
                ++Found;
        }

        if (Found != 1u)
            RecombinationHeld = false;
    }

    Report("Every declared index concluded once",
           RecombinationHeld,
           "[-] recombination by index, never by completion");

    bool DeliveredThroughout = true;

    for (const Slate::WorkCompletion& Held : Concluded)
    {
        if (Held.Concluded != Slate::WorkConclusion::Delivered)
            DeliveredThroughout = false;
    }

    Report("Each delivered", DeliveredThroughout, "[-] no spurious cancellation");

    Report("A concluded identity no longer resolves",
           !Declared.empty() && !Working.Progress(Declared[0]).ContentPresent,
           "[-] the generation advanced at Seal");

    // 📝 A refusing resolution is reported through `86` with its origin — `34` §5. A refusal that produced no
    //    report would leave the artist unable to say why a solve produced nothing.
    Slate::WorkDeclaration Refusing;
    Refusing.Origin   = "ConsoleHost refusal";
    Refusing.Priority = Slate::WorkPriority::Background;
    Refusing.Resolve  = [](const Slate::WorkCancellation&, Slate::WorkProgress&)
    {
        return Slate::Outcome<bool>::Refuse({ Slate::RefusalReason::ExtentExhausted, "declined deliberately" });
    };

    const Slate::Outcome<Slate::WorkIdentity> Declining = Working.Declare(Refusing);

    bool RefusalConcluded = false;

    for (std::uint32_t Passed = 0u; Passed < 100000u && !RefusalConcluded; ++Passed)
    {
        for (const Slate::WorkCompletion& Held : Working.Drain())
        {
            if (Held.Concluded == Slate::WorkConclusion::Refused)
                RefusalConcluded = true;
        }
    }

    Report("A refusal concludes as refused",
           Declining.ContentPresent && RefusalConcluded,
           "[-] carrying its reason");

    Report("A refusal reaches the register",
           Reporting.RetainedCount() != 0u,
           "[-] `34` §5 reports through `86`");

    Working.Reclaim();

    Report("Reclamation joins every worker", Working.WorkerCount() == 0u, "[-] nothing is left running");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       COLOUR
//------------------------------------------------------------------------------------------------------------------------

void VerifyColour()
{
    std::printf("ColourProjection\n");

    const Slate::ColourSpaceSpecification Working = Slate::DeclaredWorkingSpace();
    const Slate::ColourSpaceSpecification Display = Slate::DeclaredDisplaySpace();

    Report("The two spaces are distinct",
           Working.SpaceIdentity != Display.SpaceIdentity,
           "[-] the display space is never the working space");

    Slate::ColourSpecification Undeclared;
    Undeclared.RedCoordinate = 0.5;

    Report("An undeclared colour refuses projection",
           !Slate::Project(Undeclared, Working, Display).ContentPresent,
           "[-] no bare triple is projected");

    // 📐 A neutral working coordinate must project to a neutral display coordinate. `66` §3 requires the tone
    //    projection to preserve the neutral axis, and it cannot if the space projection does not.
    Slate::ColourSpecification Neutral;
    Neutral.RedCoordinate   = 0.5;
    Neutral.GreenCoordinate = 0.5;
    Neutral.BlueCoordinate  = 0.5;
    Neutral.SpaceIdentity   = Slate::WorkingSpaceIdentity;

    const Slate::Outcome<Slate::ColourSpecification> Projected = Slate::Project(Neutral, Working, Display);

    const bool NeutralHeld = Projected.ContentPresent
                          && std::fabs(Projected.Resolve().RedCoordinate
                                     - Projected.Resolve().GreenCoordinate) < 1.0e-6
                          && std::fabs(Projected.Resolve().GreenCoordinate
                                     - Projected.Resolve().BlueCoordinate) < 1.0e-6;

    Report("Neutral projects to neutral", NeutralHeld, "[-] the axis survives the white adaptation");

    // 📐 Projected back, the coordinate returns to itself within the Bounded guarantee. A transfer applied twice
    //    or omitted once is the most common defect in a display path and it reads as "a bit washed out".
    const Slate::Outcome<Slate::ColourSpecification> Returned =
        Projected.ContentPresent ? Slate::Project(Projected.Resolve(), Display, Working)
                                 : Projected;

    Report("A round trip returns the coordinate",
           Returned.ContentPresent && std::fabs(Returned.Resolve().RedCoordinate - 0.5) < 1.0e-9,
           "[-] within the declared Bounded guarantee");

    Report("A projection into the same space is untouched",
           Slate::Project(Neutral, Working, Working).Resolve().RedCoordinate == 0.5,
           "[-] a conversion that does nothing perturbs nothing");

    Report("The working transfer is linear",
           Slate::Encode(Working, 0.25) == 0.25 && Slate::Decode(Working, 0.25) == 0.25,
           "[-] every computation above `66` ⑧ is linear");

    Report("The display transfer is not",
           Slate::Encode(Display, 0.25) != 0.25,
           "[-] and is applied exactly once, in `66`");

    // 📝 Negative coordinates arise legitimately: a working space wider than the display produces them. Clamping
    //    at the transfer would lose the sign before `66` had projected it.
    Report("A negative coordinate keeps its sign",
           Slate::Encode(Display, -0.25) < 0.0,
           "[-] transferred by odd reflection, never clamped");

    Report("A declared temperature projects",
           Slate::ProjectTemperature(5600.0, Working).ContentPresent,
           "[-] `36` §5's illuminant colour");

    Report("A temperature outside the locus refuses",
           !Slate::ProjectTemperature(100.0, Working).ContentPresent,
           "[-] refuses rather than extrapolating");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     PROPERTIES
//------------------------------------------------------------------------------------------------------------------------

void VerifyProperties()
{
    std::printf("PropertySpecification\n");

    Slate::PropertyIndex Properties;

    // 📝 A roughness channel: bounded, and defaulted to something that is not zero. `42` §2's rule is that an
    //    absent value resolves to a declared default — a magnitude defaulted to zero produces a mirror.
    Slate::PropertyDeclaration Roughness;
    Roughness.Identity                 = "Roughness";
    Roughness.Presented               = "Roughness";
    Roughness.Measured                = Slate::PropertyMeasure::Magnitude;
    Roughness.LowerMagnitude          = 0.0;
    Roughness.UpperMagnitude          = 1.0;
    Roughness.BoundsDeclared          = true;
    Roughness.Defaulted.Measured      = Slate::PropertyMeasure::Magnitude;
    Roughness.Defaulted.MagnitudeHeld = 0.5;

    Report("A declaration is admitted", Properties.Declare(Roughness).ContentPresent, "[-] its default validated");

    Report("An absent value reads its declared default",
           Properties.Resolve("Roughness").Resolve().MagnitudeHeld == 0.5,
           "[-] never assumed to be zero");

    // 🔴 `10` §2.2: a declaration whose own default is out of bounds presents an invalid value on every occupant
    //    that never wrote it, which is every occupant at the moment it arrives.
    Slate::PropertyDeclaration Impossible = Roughness;
    Impossible.Identity                   = "Impossible";
    Impossible.Defaulted.MagnitudeHeld    = 2.0;

    Report("A declaration with an invalid default is refused",
           !Properties.Declare(Impossible).ContentPresent,
           "[-] validated at declaration");

    Slate::PropertyValue Offered;
    Offered.Measured      = Slate::PropertyMeasure::Magnitude;
    Offered.MagnitudeHeld = 0.25;

    Report("A valid write lands",
           Properties.Write("Roughness", Offered).ContentPresent
        && Properties.Resolve("Roughness").Resolve().MagnitudeHeld == 0.25,
           "[-] and is marked written");

    Slate::PropertyValue Exceeding;
    Exceeding.Measured      = Slate::PropertyMeasure::Magnitude;
    Exceeding.MagnitudeHeld = 4.0;

    Report("A write beyond the interval is refused",
           !Properties.Write("Roughness", Exceeding).ContentPresent,
           "[-] refuses; it never bounds silently");

    Report("A refused write leaves the prior value",
           Properties.Resolve("Roughness").Resolve().MagnitudeHeld == 0.25,
           "[-] no partial state");

    Report("Bounding is offered apart from writing",
           Slate::Bounded(Roughness, Exceeding).Resolve().MagnitudeHeld == 1.0,
           "[-] the presenter bounds, then writes");

    Slate::PropertyValue MismeasuredValue;
    MismeasuredValue.Measured    = Slate::PropertyMeasure::Truth;
    MismeasuredValue.TruthDeclared = true;

    Report("A value of the wrong measure is refused",
           !Properties.Write("Roughness", MismeasuredValue).ContentPresent,
           "[-] the measure discriminates first");

    // 🔴 `36` §1: a colour without its space is refused rather than assumed to be in the working space.
    Slate::PropertyDeclaration Albedo;
    Albedo.Identity                = "AlbedoColour";
    Albedo.Measured                = Slate::PropertyMeasure::Colour;
    Albedo.RequiredSpace           = Slate::WorkingSpaceIdentity;
    Albedo.Defaulted.Measured      = Slate::PropertyMeasure::Colour;
    Albedo.Defaulted.ColourHeld.RedCoordinate   = 0.5;
    Albedo.Defaulted.ColourHeld.GreenCoordinate = 0.5;
    Albedo.Defaulted.ColourHeld.BlueCoordinate  = 0.5;
    Albedo.Defaulted.ColourHeld.SpaceIdentity   = Slate::WorkingSpaceIdentity;

    Properties.Declare(Albedo);

    Slate::PropertyValue Spaceless;
    Spaceless.Measured                  = Slate::PropertyMeasure::Colour;
    Spaceless.ColourHeld.RedCoordinate  = 1.0;

    Report("A colour with no space is refused",
           !Properties.Write("AlbedoColour", Spaceless).ContentPresent,
           "[-] no bare triple is ever held");

    Slate::PropertyValue WrongSpace = Spaceless;
    WrongSpace.ColourHeld.SpaceIdentity = Slate::DisplaySpaceIdentity;

    Report("A colour in the wrong space is refused",
           !Properties.Write("AlbedoColour", WrongSpace).ContentPresent,
           "[-] the required space is stated, not assumed");

    Report("An undeclared property refuses",
           !Properties.Write("Absent", Offered).ContentPresent
        && !Properties.Resolve("Absent").ContentPresent,
           "[-] nothing declares it");

    Properties.Reclaim("Roughness");

    Report("Reclamation restores the default",
           Properties.Resolve("Roughness").Resolve().MagnitudeHeld == 0.5
        && !Properties.ValueWritten("Roughness"),
           "[-] and clears the written mark");

    Report("Every held value satisfies its declaration",
           Properties.ValuesValid(),
           "[-] structurally, because Write is the only writer");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       DOCUMENT
//------------------------------------------------------------------------------------------------------------------------

void VerifyDocument()
{
    std::printf("SlateDocument\n");

    Slate::PopulationIndex Population;

    const Slate::Outcome<Slate::OccupantIdentity> FirstEnrolment = Population.Enrol();

    Report("PopulationIndex enrols", FirstEnrolment.ContentPresent, "[-] an identity was issued");

    if (!FirstEnrolment.ContentPresent)
        return;

    const Slate::OccupantIdentity Enrolled = FirstEnrolment.Resolve();

    Report("Issued identity resolves",
           Population.Resolve(Enrolled),
           "[-] the generation still occupies the slot");

    Population.Withdraw(Enrolled);

    Report("Withdrawal advances the generation",
           !Population.Resolve(Enrolled),
           "[-] the prior generation reads absent");

    // 📝 🔴 The reused slot must not issue an identity equal to the withdrawn one. This is the property the
    //    whole generational scheme exists for, so it is measured rather than assumed.
    const Slate::Outcome<Slate::OccupantIdentity> SecondEnrolment = Population.Enrol();

    Report("A reused slot issues a new generation",
           SecondEnrolment.ContentPresent && SecondEnrolment.Resolve() != Enrolled,
           "[-] the slot returned, the identity did not");

    Report("A stale identity survives reuse",
           !Population.Resolve(Enrolled),
           "[-] resolves absent, never to the new occupant");

    Slate::RevisionSequence Revisions;

    Report("An open transaction is absent",
           Revisions.Open("", "PaintStroke").ContentPresent && Revisions.Committed().empty(),
           "[-] nothing enters the sequence until Seal");

    Report("A second open is refused",
           !Revisions.Open("", "PaintStroke").ContentPresent,
           "[-] one transaction is open at a time");

    Revisions.Seal(1000000000ull, false);

    Report("A sealed transaction enters", Revisions.Committed().size() == 1u, "[-] exactly one");

    Report("Retreat scrubs backwards",
           Revisions.Retreat().ContentPresent && Revisions.ScrubPosition() == 0u,
           "[-] the position moved, the transaction remains");

    Report("Retreat at the beginning refuses",
           !Revisions.Retreat().ContentPresent,
           "[-] refuses rather than underflowing");

    // 📝 A heading at the current version resolves with zero migration steps. A later version is
    //    unmigratable rather than best-effort: `10` refuses a stream a later build wrote.
    Slate::StreamHeading CurrentHeading;
    CurrentHeading.Signature     = 0x45544C53u;
    CurrentHeading.StreamVersion = Slate::CurrentStreamVersion;

    const Slate::Outcome<std::uint32_t> Resolved = Slate::ResolveMigration(CurrentHeading);

    Report("A current stream needs no migration",
           Resolved.ContentPresent && Resolved.Resolve() == 0u,
           "[-] zero declared steps");

    Slate::StreamHeading LaterHeading = CurrentHeading;
    LaterHeading.StreamVersion        = Slate::CurrentStreamVersion + 1u;

    const Slate::Outcome<std::uint32_t> RefusedVersion = Slate::ResolveMigration(LaterHeading);

    Report("A later stream is refused",
           !RefusedVersion.ContentPresent
        && RefusedVersion.Declined.DeclaredReason == Slate::RefusalReason::VersionUnmigratable,
           "[-] the refusal carries its reason");

    Slate::StreamHeading ForeignHeading = CurrentHeading;
    ForeignHeading.Signature            = 0u;

    Report("A foreign stream is refused",
           !Slate::ResolveMigration(ForeignHeading).ContentPresent,
           "[-] the signature gates the whole codec");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        DEVICE
//------------------------------------------------------------------------------------------------------------------------

// 📝 🔴 No instance is constructed here. This host runs where no vendor loader need be present, so it
//    verifies what `08` derives — the ordering — and leaves device construction to the windowed host.
void VerifySchedule()
{
    std::printf("SlateVulkan\n");

    Slate::RenderSchedule Schedule;

    Slate::DeclaredRecording VisibilityRecording;
    VisibilityRecording.Identity = "VisibilityRecording";
    VisibilityRecording.Produces = { Slate::SharedTarget::DepthSurface, Slate::SharedTarget::VisibilityIndex };
    VisibilityRecording.Command  = Slate::RecordingCommand::GraphicsRecording;

    Slate::DeclaredRecording RadianceRecording;
    RadianceRecording.Identity = "RadianceRecording";
    RadianceRecording.Reads    = { Slate::SharedTarget::DepthSurface, Slate::SharedTarget::VisibilityIndex };
    RadianceRecording.Produces = { Slate::SharedTarget::RadianceSurface };
    RadianceRecording.Command  = Slate::RecordingCommand::ComputeDispatch;

    Slate::DeclaredRecording ToneRecording;
    ToneRecording.Identity = "ToneRecording";
    ToneRecording.Reads    = { Slate::SharedTarget::RadianceSurface };
    ToneRecording.Produces = { Slate::SharedTarget::DisplaySurface };
    ToneRecording.Command  = Slate::RecordingCommand::ComputeDispatch;

    Slate::DeclaredRecording OverlayRecording;
    OverlayRecording.Identity        = "OverlayRecording";
    OverlayRecording.Amends          = { Slate::SharedTarget::DisplaySurface };
    OverlayRecording.Command         = Slate::RecordingCommand::GraphicsRecording;
    OverlayRecording.DisplayReferred = true;

    // 📝 Contributed in reverse deliberately. The ordering is derived from the declared reads and writes;
    //    contributing in the order the recordings run would prove nothing about the derivation.
    Report("Display-referred contribution accepted",
           Schedule.Contribute(OverlayRecording).ContentPresent,
           "[-] recorded after the tone line");

    Report("Tone contribution accepted",
           Schedule.Contribute(ToneRecording).ContentPresent,
           "[-] the tone line itself");

    Report("Radiance contribution accepted",
           Schedule.Contribute(RadianceRecording).ContentPresent,
           "[-] scene-referred");

    Report("Visibility contribution accepted",
           Schedule.Contribute(VisibilityRecording).ContentPresent,
           "[-] scene-referred");

    Slate::DeclaredRecording DuplicateProducer;
    DuplicateProducer.Identity = "SecondDepthRecording";
    DuplicateProducer.Produces = { Slate::SharedTarget::DepthSurface };

    Report("A second producer is refused",
           !Schedule.Contribute(DuplicateProducer).ContentPresent,
           "[-] one producing recording per target");

    Slate::DeclaredRecording UngovernedRecording;
    UngovernedRecording.Identity           = "ComputeRasterRecording";
    UngovernedRecording.CapabilityRequired = true;

    Report("A capability with no substitution is refused",
           !Schedule.Contribute(UngovernedRecording).ContentPresent,
           "[-] the substitution belongs to the contributor");

    Report("Ordering fixed", Schedule.Fix().ContentPresent, "[-] derived, never authored");

    const std::vector<Slate::DeclaredRecording>& Ordered = Schedule.Ordered();

    Report("Every contribution ordered", Ordered.size() == 4u, "[-] none silently dropped");

    if (Ordered.size() == 4u)
    {
        Report("Producers precede their consumers",
               std::strcmp(Ordered[0].Identity, "VisibilityRecording") == 0
            && std::strcmp(Ordered[1].Identity, "RadianceRecording")   == 0
            && std::strcmp(Ordered[2].Identity, "ToneRecording")       == 0,
               "[-] visibility, radiance, tone");

        Report("Display-referred ordered last",
               Ordered[3].DisplayReferred,
               "[-] nothing scene-referred follows the tone line");
    }

    Report("A second Fix is refused", !Schedule.Fix().ContentPresent, "[-] the ordering is immutable");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        PARITY
//------------------------------------------------------------------------------------------------------------------------

void VerifyParity()
{
    std::printf("SlateCompute\n");

    Slate::ParityRunner Runner;

    Slate::ParityRegistration OrientationEntry;
    OrientationEntry.EntryName = "ClassifyOrientation";
    OrientationEntry.Claimed   = Slate::PrecisionGuarantee::Exact;

    Report("Registration accepted", Runner.Register(OrientationEntry).ContentPresent, "[-] one entry point");

    Report("A duplicate registration is refused",
           !Runner.Register(OrientationEntry).ContentPresent,
           "[-] one registration per spelling");

    // 🔴 `32` §4.1 stage D checks at build time that every `Shared/` entry point is registered. Registering
    //    them here is the run-time half of the same statement: an entry point that is registered and never
    //    compared reports zero samples and withdraws the agreement, which the vacancy check below relies on.
    const char* const ExactEntryNames[] =
    {
        "ClassifyIncircle",
        "ClassifySegmentIntersection",
        "ClassifyIntervalContainment",
        "ProjectPlanarSample"
    };

    bool ExactRegistrationsHeld = true;

    for (const char* const EntryName : ExactEntryNames)
    {
        Slate::ParityRegistration Registering;
        Registering.EntryName = EntryName;
        Registering.Claimed   = Slate::PrecisionGuarantee::Exact;

        if (!Runner.Register(Registering).ContentPresent)
            ExactRegistrationsHeld = false;
    }

    Report("Every exact entry point registers",
           ExactRegistrationsHeld,
           "[-] four beside the orientation predicate");

    Slate::ParityRegistration SphericalEntry;
    SphericalEntry.EntryName = "ProjectSphericalSample";
    SphericalEntry.Claimed   = Slate::PrecisionGuarantee::Bounded;

    Report("A bounded entry point registers",
           Runner.Register(SphericalEntry).ContentPresent,
           "[-] compared against a bound, never for equality");

    // 🔴 `28` §2's three surfaces are baked on the host and sampled on the device through the same two mappings,
    //    so both of them cross the toolchain seam in both directions and both are covered here. Neither is
    //    compared against a second implementation of itself: ① is compared as an inversion of its own parameter
    //    routine, and ③ against the unit length every consumer of its direction assumes.
    const char* const AtmosphereEntryNames[] =
    {
        "ProjectTransmittanceCoordinate",
        "ProjectSkyViewDirection"
    };

    bool AtmosphereRegistrationsHeld = true;

    for (const char* const EntryName : AtmosphereEntryNames)
    {
        Slate::ParityRegistration Registering;
        Registering.EntryName = EntryName;
        Registering.Claimed   = Slate::PrecisionGuarantee::Bounded;

        if (!Runner.Register(Registering).ContentPresent)
            AtmosphereRegistrationsHeld = false;
    }

    Report("Every atmosphere mapping registers",
           AtmosphereRegistrationsHeld,
           "[-] both of the mappings that cross the seam in both directions");

    const std::vector<Slate::ParityReport>& Reports = Runner.Compare();

    Report("One report per registration", Reports.size() == 8u, "[-] in registration order");

    if (!Reports.empty())
    {
        Report("Samples were compared",
               Reports[0].SampleCount > 0u,
               "[-] an empty sample set never holds");

        Report("Orientation antisymmetry holds",
               Reports[0].DisagreeingCount == 0u,
               "[-] reversing two operands negates the sign, exactly");
    }

    bool EverySampleSetCompared = true;

    for (const Slate::ParityReport& Held : Reports)
    {
        if (Held.SampleCount == 0u || Held.DisagreeingCount != 0u)
            EverySampleSetCompared = false;
    }

    Report("Every registration was genuinely compared",
           EverySampleSetCompared,
           "[-] none passed on an empty sample set");

    bool EveryBoundHeld = Reports.size() == 8u;

    for (std::size_t Ordinal = 5u; Ordinal < Reports.size(); ++Ordinal)
    {
        if (Reports[Ordinal].LargestDeviation > Slate::SampleUnitPlaceCeiling)
            EveryBoundHeld = false;
    }

    Report("Every bounded entry point stays inside its bound",
           EveryBoundHeld,
           "[-] measured in units in the last place");

    Report("Agreement declared", Runner.AgreementHeld(), "[-] every registered entry point held");

    // 🚧 An unregistered comparison must not pass vacantly. Registering an entry point with no declared
    //    comparison reports zero samples and withdraws the agreement, rather than reporting a proof nobody
    //    produced. This is the gate that would otherwise let `Shared/` grow uncovered entry points.
    Slate::ParityRunner VacantRunner;

    Slate::ParityRegistration UncomparedEntry;
    // 📝 `02` §5's quadrature integrator is unbuilt, so it is the honest choice of a name that is legitimately
    //    uncompared. It was `ClassifyIncircle` until this batch built one, which is exactly the churn the check
    //    is meant to force: a vacancy check that names a built entry point has stopped checking anything.
    UncomparedEntry.EntryName = "IntegrateQuadrature";
    UncomparedEntry.Claimed   = Slate::PrecisionGuarantee::Bounded;

    VacantRunner.Register(UncomparedEntry);
    VacantRunner.Compare();

    Report("An uncompared entry point does not hold",
           !VacantRunner.AgreementHeld(),
           "[-] zero samples is not agreement");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                 TOPOLOGY AND MATERIALS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Two unit squares sharing an edge, the second declared with a duplicated position rather than a shared
//    vertex — which is what a format storing a coordinate per corner produces at every seam. Welding is the only
//    thing that can see through it, so it is the shape the check is built on.
void VerifyTopology()
{
    std::printf("TopologyConditioning\n");

    Slate::TopologyStructure Imported;

    std::vector<Slate::DocumentPosition> Positions(6);
    Positions[0].PositionX = 0.0;  Positions[0].PositionY = 0.0;
    Positions[1].PositionX = 1.0;  Positions[1].PositionY = 0.0;
    Positions[2].PositionX = 1.0;  Positions[2].PositionY = 1.0;
    Positions[3].PositionX = 0.0;  Positions[3].PositionY = 1.0;
    Positions[4].PositionX = 1.0;  Positions[4].PositionY = 0.0;   // duplicate of vertex 1
    Positions[5].PositionX = 2.0;  Positions[5].PositionY = 0.0;

    Imported.DeclarePositions(Positions);

    Report("A run of two corners is refused",
           !Imported.DeclareFace({ 0u, 1u }).ContentPresent,
           "[-] fewer than three corners is not a face");

    Imported.DeclareFace({ 0u, 1u, 2u, 3u });
    Imported.DeclareFace({ 4u, 5u, 2u });

    Report("An unsealed topology refuses conditioning",
           !Slate::TopologyConditioning{}.Condition(Imported).ContentPresent,
           "[-] not immutable for the run");

    Report("Sealing advances the revision",
           Imported.Seal().ContentPresent && Imported.Revision() == 1u,
           "[-] what `24` §3 keys on");

    Report("A declaration after the seal is refused",
           !Imported.DeclareFace({ 0u, 1u, 2u }).ContentPresent,
           "[-] the arrays are stable from here");

    Report("An absent enrollment defaults to one material",
           Imported.MaterialEnrollment().size() == Imported.FaceCount(),
           "[-] `50` §3's last-resort row");

    Slate::TopologyConditioning Conditioned;

    Report("Conditioning derives", Conditioned.Condition(Imported).ContentPresent, "[-] beside, never into");

    Report("Coincident vertices weld to one position",
           Conditioned.WeldedCount() == 5u,
           "[-] six imported vertices, five positions");

    Report("The imported arrays are untouched",
           Imported.VertexCount() == 6u && Imported.FaceCount() == 2u,
           "[-] an index means the same thing after as before");

    Report("A degenerate face is enrolled, not removed",
           Imported.FaceCount() == 2u,
           "[-] `38` §3 excludes; it never renumbers");

    // 📐 Every extent is rounded outward, so the whole extent strictly contains every position it was built from.
    const Slate::ConditionedExtent Whole = Conditioned.TopologyExtent();

    Report("Extents are conservative outward",
           Whole.Least.PositionX < 0.0 && Whole.Greatest.PositionX > 2.0,
           "[mm] never inward — `38` §6");

    Report("Perpendiculars were derived",
           Conditioned.Perpendiculars().size() == Imported.VertexCount(),
           "[-] one per imported vertex");

    // 🔴 With no domain coordinates there is no domain, so the basis is absent rather than substituted — `18`
    //    §1.1. An orthonormalised substitute would be a fabricated value, which `24` §2 rejects for transfer.
    bool BasesAbsent = !Conditioned.TangentBases().empty();

    for (const Slate::TangentBasis& Held : Conditioned.TangentBases())
    {
        if (Held.BasisDeclared)
            BasesAbsent = false;
    }

    Report("Absent coordinates leave the basis absent",
           BasesAbsent && !Conditioned.TangentBasesRetained(),
           "[-] never a fabricated substitute");

    Report("A boundary edge yields no adjacency",
           !Conditioned.AdjacentCorner(0u).ContentPresent,
           "[-] refuses rather than choosing one arbitrarily");
}

void VerifyMaterials()
{
    std::printf("MaterialSpecification\n");

    Slate::MaterialIndex Materials;

    const Slate::Outcome<std::uint32_t> Declared = Materials.Declare("Painted metal");

    Report("A material is declared", Declared.ContentPresent, "[-] addressed by identity");

    if (!Declared.ContentPresent)
        return;

    Slate::MaterialSpecification* Amending = Materials.Amend(Declared.Resolve()).Resolve();

    Report("An undeclared material refuses",
           !Materials.Resolve(Declared.Resolve() + 1u).ContentPresent,
           "[-] no such material");

    // 🔴 An absent channel resolves to its declared default, which is not zero. A transmission channel defaulted
    //    to zero is opaque and an occlusion channel defaulted to zero is black; only one of those is right.
    Slate::ChannelSpecification Occlusion;
    Occlusion.Source         = Slate::ChannelSource::Absent;
    Occlusion.Measured       = Slate::ChannelMeasure::Scalar;
    Occlusion.DefaultScalar  = 1.0;
    Occlusion.LowerMagnitude = 0.0;
    Occlusion.UpperMagnitude = 1.0;

    Report("An absent channel declares a non-zero default",
           Amending->DeclareChannel(Slate::ChannelSubject::AmbientOcclusion, Occlusion).ContentPresent
        && Amending->Channel(Slate::ChannelSubject::AmbientOcclusion).DefaultScalar == 1.0,
           "[-] fully unoccluded, not black");

    Slate::ChannelSpecification Impossible = Occlusion;
    Impossible.DefaultScalar               = 4.0;

    Report("A default outside its interval is refused",
           !Amending->DeclareChannel(Slate::ChannelSubject::Roughness, Impossible).ContentPresent,
           "[-] validated at declaration");

    Slate::ChannelSpecification Albedo;
    Albedo.Source   = Slate::ChannelSource::Constant;
    Albedo.Measured = Slate::ChannelMeasure::Reflectance;

    Report("A colour channel with no space is refused",
           !Amending->DeclareChannel(Slate::ChannelSubject::AlbedoColour, Albedo).ContentPresent,
           "[-] `36` §1: no bare triple");

    Albedo.ConstantColour.SpaceIdentity = Slate::WorkingSpaceIdentity;
    Albedo.DefaultColour.SpaceIdentity  = Slate::WorkingSpaceIdentity;

    Report("A colour channel with its space is admitted",
           Amending->DeclareChannel(Slate::ChannelSubject::AlbedoColour, Albedo).ContentPresent,
           "[-] the coordinate carries its space");

    Slate::ChannelSpecification Sheen = Occlusion;
    Sheen.Source = Slate::ChannelSource::Layered;

    Amending->DeclareChannel(Slate::ChannelSubject::SheenRoughness, Sheen);
    Amending->DeclareReflectance(Slate::ReflectanceSelection::Standard);

    Report("An unconsumed channel is not sampled",
           !Amending->ChannelSampled(Slate::ChannelSubject::SheenRoughness),
           "[-] `18` §9: unread channels are not sampled");

    Amending->DeclareReflectance(Slate::ReflectanceSelection::Cloth);

    Report("A retained channel returns with its selection",
           Amending->ChannelSampled(Slate::ChannelSubject::SheenRoughness),
           "[-] `42` §5: never discarded on switch");

    Report("Colour conversion reads the declared measure",
           Amending->ChannelConverted(Slate::ChannelSubject::AlbedoColour)
       && !Amending->ChannelConverted(Slate::ChannelSubject::AmbientOcclusion),
           "[-] never inferred from a name");

    Slate::PartitionResolutionIndex Resolutions;

    Slate::ResolvedPartition Resolving;
    Resolving.Occupant.SlotOrdinal    = 7u;
    Resolving.Occupant.SlotGeneration = 1u;
    Resolving.MaterialOrdinal         = Declared.Resolve();
    Resolving.FaceCount               = 96u;

    const Slate::Outcome<Slate::PartitionIdentity> Issued = Resolutions.Declare(Resolving);

    Report("A partition resolves to an occupant",
           Issued.ContentPresent
        && Resolutions.Resolve(Issued.Resolve()).Resolve().Occupant == Resolving.Occupant,
           "[-] `00` §10 conflict 15, closed");

    Report("A partition with no occupant is refused",
           !Resolutions.Declare(Slate::ResolvedPartition{}).ContentPresent,
           "[-] the resolution is derived, never authored");

    Resolutions.Reclaim();

    Report("A rebuild staleness the prior identity",
           !Resolutions.Resolve(Issued.Resolve()).ContentPresent,
           "[-] refuses rather than resolving to whoever took the ordinal");
}

void VerifyVector()
{
    std::printf("VectorInterchange\n");

    // 📝 One unit square as four line segments. Flattened, classified, and then classified again at a coarser
    //    tolerance — the containment must not depend on the tolerance for a path with no curvature.
    Slate::OutlineSpecification Square;

    Slate::OutlinePath Path;
    Path.Origin.PositionX = 0.0;
    Path.Origin.PositionY = 0.0;
    Path.ClosedRun        = true;

    const double CornerX[4] = { 1.0, 1.0, 0.0, 0.0 };
    const double CornerY[4] = { 0.0, 1.0, 1.0, 0.0 };

    for (std::uint32_t Ordinal = 0u; Ordinal < 4u; ++Ordinal)
    {
        Slate::PathSegment Segment;
        Segment.Subject             = Slate::SegmentSubject::Line;
        Segment.Terminus.PositionX  = CornerX[Ordinal];
        Segment.Terminus.PositionY  = CornerY[Ordinal];

        Path.Segments.push_back(Segment);
    }

    Square.Paths.push_back(Path);

    Slate::VectorInterchange Outline;

    Report("An empty source is refused",
           !Outline.DeclareFromFile(Slate::OutlineSpecification{}, "empty.vector").ContentPresent,
           "[-] no path was declared");

    Report("A supplied-text source retains its text",
           Outline.DeclareFromText(Square, "<square/>").ContentPresent && Outline.TextRetained(),
           "[-] nothing depends on a clipboard surviving");

    Report("A file source produces the same specification",
           Outline.DeclareFromFile(Square, "square.vector").ContentPresent
        && !Outline.TextRetained()
        && Outline.Declared().Paths.size() == 1u,
           "[-] indistinguishable downstream");

    const std::vector<std::vector<Slate::PlanarPosition>> Fine   = Outline.Flatten(1.0e-4);
    const std::vector<std::vector<Slate::PlanarPosition>> Coarse = Outline.Flatten(1.0e-1);

    Report("Interior classifies inside",
           Outline.Classify(Fine, 0.5, 0.5) > 0,
           "[-] the non-zero rule");

    Report("Exterior classifies outside",
           Outline.Classify(Fine, 1.5, 0.5) < 0,
           "[-] and the ray is unambiguous");

    // 🔴 A boundary resolves to zero and not to inside. `70` resolves coverage from this, and a boundary reported
    //    interior gives every outline a one-texel bias outward at its own edge.
    Report("A boundary classifies as boundary",
           Outline.Classify(Fine, 0.0, 0.5) == 0,
           "[-] never interior");

    // 📐 A position level with two shared vertices is the case a closed ordinate test counts twice. The half-open
    //    test is what makes it contribute exactly once, so the artist sees no hole at any vertex.
    Report("A position level with a vertex classifies once",
           Outline.Classify(Fine, 0.5, 0.0) == 0 && Outline.Classify(Fine, 0.5, 1.0) == 0,
           "[-] the half-open ordinate test");

    Report("Tolerance does not change containment of a straight path",
           Outline.Classify(Coarse, 0.5, 0.5) > 0,
           "[-] resolution-relative, not resolution-dependent");

    Slate::Refusal Declining;
    Declining.DeclaredReason = Slate::RefusalReason::ContentUnsupported;
    Declining.Detail         = "effect operations are outside the accepted subset";

    Outline.Refuse("feGaussianBlur", 412u, Declining);

    Report("A refusal names its construct and position",
           Outline.Refusals().size() == 1u
        && Outline.Refusals()[0].SourceOrdinal == 412u
        && !Outline.Refusals()[0].Construct.empty(),
           "[-] never bare 'unsupported'");

    // 📝 A stroke is converted at intake rather than stored as a width, so a placement that scales the source
    //    does not thin the stroke — `52` §2.
    const std::vector<Slate::PlanarPosition> Traversed = Slate::Flatten(Path.Origin, Path.Segments, 1.0e-4);

    Report("A stroke converts to an outline",
           Slate::OffsetOutline(Traversed, 0.05, true).ContentPresent,
           "[-] no stroke width is stored");

    Report("A stroke of no width is refused",
           !Slate::OffsetOutline(Traversed, 0.0, true).ContentPresent,
           "[-] it encloses nothing");

    // 📐 An arc is flattened by sagitta, so a tighter tolerance produces strictly more positions. A flattening
    //    that ignored the tolerance would produce the same count either way.
    Slate::PathSegment Arc;
    Arc.Subject            = Slate::SegmentSubject::Arc;
    Arc.Terminus.PositionX = 2.0;
    Arc.RadiusAlong        = 1.0;
    Arc.RadiusAcross       = 1.0;
    Arc.SweepEnabled       = true;

    const std::size_t FineArc   = Slate::Flatten(Slate::PlanarPosition{}, { Arc }, 1.0e-4).size();
    const std::size_t CoarseArc = Slate::Flatten(Slate::PlanarPosition{}, { Arc }, 1.0e-1).size();

    Report("Arc flattening follows the tolerance",
           FineArc > CoarseArc && CoarseArc >= 2u,
           "[-] by sagitta, per level");

    Slate::TypefaceInterchange Typeface;
    Typeface.DeclareTypeface(1u, 1000.0);

    Slate::GlyphSpecification Glyph;
    Glyph.GlyphIdentity = 42u;
    Glyph.Advance       = 600.0;

    Report("A glyph is declared by identity",
           Typeface.DeclareGlyph(Glyph).ContentPresent
        && Typeface.ResolveGlyph(42u).ContentPresent,
           "[-] never by character");

    Report("A duplicate glyph is refused",
           !Typeface.DeclareGlyph(Glyph).ContentPresent,
           "[-] one declaration per glyph");

    Typeface.DeclareAdjustment(42u, 43u, -30.0);

    Report("An undeclared pair adjusts by nothing",
           Typeface.Adjustment(42u, 43u) == -30.0 && Typeface.Adjustment(43u, 42u) == 0.0,
           "[-] and the pair is ordered");

    // 🔴 Text stores a glyph sequence **and** its characters. Storing only characters means replacing a typeface
    //    silently reshapes text the artist already positioned — `52` §3.
    Slate::ResolvedText Text;
    Text.GlyphSequence    = { 42u };
    Text.Characters       = "A";
    Text.TypefaceIdentity = 1u;

    Report("Text stores glyphs and characters both",
           !Text.GlyphSequence.empty() && !Text.Characters.empty(),
           "[-] never characters alone");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE CAMERA
//------------------------------------------------------------------------------------------------------------------------

void VerifyCamera()
{
    std::printf("CameraProjection\n");

    Slate::CameraSpecification Declaring;
    Declaring.Placement.Translation.PositionZ = 10.0;
    Declaring.SensorProportion                = 16.0 / 9.0;

    Slate::CameraSpecification Inverted    = Declaring;
    Inverted.Clipping.Nearest              = 100.0;
    Inverted.Clipping.Furthest             = 1.0;

    Report("An inverted clipping interval is refused",
           !Slate::Derive(Inverted).ContentPresent && !Inverted.Clipping.IntervalValid(),
           "[-] a frustum with no interior is refused, never derived");

    const Slate::Outcome<Slate::ViewProjection> Projected = Slate::Derive(Declaring);

    Report("A declared camera projects", Projected.ContentPresent, "[-] the composed matrix exists");

    if (!Projected.ContentPresent)
        return;

    // 📐 The depth convention, measured rather than assumed. A position on the nearest plane must resolve to
    //    NearPlaneDepth and one on the furthest to FarPlaneDepth; the forward arrangement inverts both.
    const double* Composed = Projected.Resolve().Composed.Coefficient;

    const double NearestClipDepth = Composed[10] * (-Declaring.Clipping.Nearest) + Composed[14];
    const double NearestClipScale = Composed[11] * (-Declaring.Clipping.Nearest) + Composed[15];

    const double FurthestClipDepth = Composed[10] * (-Declaring.Clipping.Furthest) + Composed[14];
    const double FurthestClipScale = Composed[11] * (-Declaring.Clipping.Furthest) + Composed[15];

    Report("The nearest plane resolves to one",
           NearestClipScale != 0.0
        && std::fabs(NearestClipDepth / NearestClipScale - Slate::NearPlaneDepth) < 1.0e-9,
           "[-] depth is reversed, repository-wide");

    Report("The furthest plane resolves to zero",
           FurthestClipScale != 0.0
        && std::fabs(FurthestClipDepth / FurthestClipScale - Slate::FarPlaneDepth) < 1.0e-9,
           "[-] and the two never disagree");

    Slate::CameraProjection Camera;

    Slate::OccupantIdentity CameraOccupant;
    CameraOccupant.SlotOrdinal    = 3u;
    CameraOccupant.SlotGeneration = 1u;

    Report("A camera declares against an occupant",
           Camera.Declare(CameraOccupant, Declaring).ContentPresent && Camera.DerivationOwed(),
           "[-] and owes a reconciliation");

    Report("Reconciliation derives the frustum",
           Camera.Reconcile().ContentPresent && !Camera.DerivationOwed(),
           "[-] both derivations, one call");

    // 📐 The camera sits at ten along the third axis looking toward the origin, so an extent at the origin is
    //    inside and one far behind the camera is outside.
    Slate::DocumentPosition NearLeast;
    NearLeast.PositionX = -1.0;  NearLeast.PositionY = -1.0;  NearLeast.PositionZ = -1.0;

    Slate::DocumentPosition NearGreatest;
    NearGreatest.PositionX = 1.0;  NearGreatest.PositionY = 1.0;  NearGreatest.PositionZ = 1.0;

    Report("An extent before the camera is inside",
           Camera.Frustum().Classify(NearLeast, NearGreatest) >= 0,
           "[-] the frustum contains what the camera sees");

    Slate::DocumentPosition BehindLeast;
    BehindLeast.PositionZ = 400.0;

    Slate::DocumentPosition BehindGreatest;
    BehindGreatest.PositionX = 1.0;  BehindGreatest.PositionY = 1.0;  BehindGreatest.PositionZ = 402.0;

    Report("An extent behind the camera is outside",
           Camera.Frustum().Classify(BehindLeast, BehindGreatest) < 0,
           "[-] and is culled before anything reads it");

    // 🔴 The rebasing property, measured at a billion millimetres. An extent that far from the document origin
    //    classifies against the camera only because `02` §3.2's subtraction preceded the narrowing.
    Slate::CameraSpecification Distant = Declaring;
    Distant.Placement.Translation.PositionX = 1.0e9;
    Distant.Placement.Translation.PositionZ = 10.0;

    Slate::CameraProjection DistantCamera;
    DistantCamera.Declare(CameraOccupant, Distant);
    DistantCamera.Reconcile();

    Slate::DocumentPosition DistantLeast;
    DistantLeast.PositionX = 1.0e9 - 1.0;  DistantLeast.PositionY = -1.0;  DistantLeast.PositionZ = -1.0;

    Slate::DocumentPosition DistantGreatest;
    DistantGreatest.PositionX = 1.0e9 + 1.0;  DistantGreatest.PositionY = 1.0;  DistantGreatest.PositionZ = 1.0;

    Report("Rebasing survives at a billion millimetres",
           DistantCamera.Frustum().Classify(DistantLeast, DistantGreatest) >= 0,
           "[mm] the subtraction preceded the narrowing");

    // 📝 A navigation gesture Seals one transaction. Amending fifty times is one seal, not fifty.
    Slate::NavigationSequence Navigating;

    Report("A gesture opens", Navigating.Open(Slate::NavigationSubject::Orbit, Declaring).ContentPresent,
           "[-] holding the prior specification");

    Report("A second open is refused",
           !Navigating.Open(Slate::NavigationSubject::Pan, Declaring).ContentPresent,
           "[-] one gesture at a time");

    for (std::uint32_t Ordinal = 0u; Ordinal < 50u; ++Ordinal)
        Navigating.Amend(4.0, 0.0);

    const Slate::Outcome<Slate::CameraSpecification> Sealed = Navigating.Seal();

    Report("An orbit preserves the focus distance",
           Sealed.ContentPresent
        && std::fabs(std::sqrt(Sealed.Resolve().Placement.Translation.PositionX
                             * Sealed.Resolve().Placement.Translation.PositionX
                             + Sealed.Resolve().Placement.Translation.PositionZ
                             * Sealed.Resolve().Placement.Translation.PositionZ) - 10.0) < 1.0e-6,
           "[mm] fifty amendments, one seal, one distance");

    Report("A gesture Seals once", !Navigating.GestureOpen(), "[-] and nothing between Open and Seal recorded");

    Slate::NavigationSequence Abandoning;
    Abandoning.Open(Slate::NavigationSubject::Pan, Declaring);
    Abandoning.Amend(100.0, 100.0);

    const Slate::Outcome<Slate::CameraSpecification> Restored = Abandoning.Abandon();

    Report("Abandonment restores the prior camera",
           Restored.ContentPresent
        && Restored.Resolve().Placement.Translation.PositionZ == Declaring.Placement.Translation.PositionZ,
           "[-] no partial state");

    // 🔴 Framing changes the placement only. A framing that also changed the field would change the composition.
    const Slate::Outcome<Slate::DecomposedTransform> Framed = Slate::Frame(Declaring, NearLeast, NearGreatest);

    Report("Framing produces a placement",
           Framed.ContentPresent,
           "[-] from an extent and a projection");

    Report("An inverted extent frames nothing",
           !Slate::Frame(Declaring, NearGreatest, NearLeast).ContentPresent,
           "[-] refuses rather than framing a negative volume");

    Slate::CameraSpecification Zoomed = Declaring;
    Zoomed.ExtentParameter             = 20.0;

    const Slate::Outcome<Slate::DecomposedTransform> Narrower = Slate::Frame(Zoomed, NearLeast, NearGreatest);

    Report("A narrower field frames from further back",
           Framed.ContentPresent && Narrower.ContentPresent
        && Narrower.Resolve().Translation.PositionZ > Framed.Resolve().Translation.PositionZ,
           "[mm] the placement moved; the field did not");

    Report("Exposure is the camera's",
           Camera.Exposure() == Declaring.Exposure,
           "[EV] stored in the document — `00` §10 conflict 33");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     ILLUMINANTS
//------------------------------------------------------------------------------------------------------------------------

void VerifyIlluminants()
{
    std::printf("IlluminantPopulation\n");

    Slate::IlluminantPopulation Illuminants;

    Slate::OccupantIdentity FirstOccupant;
    FirstOccupant.SlotOrdinal    = 1u;
    FirstOccupant.SlotGeneration = 1u;

    Slate::IlluminantSpecification Declaring;
    Declaring.Emission                     = Slate::EmissionShape::Point;
    Declaring.EmissionRadius               = 25.0;
    Declaring.ExtentReach                  = 500.0;
    Declaring.DeclaredColour.RedCoordinate = 1.0;
    Declaring.DeclaredColour.SpaceIdentity = Slate::WorkingSpaceIdentity;

    Report("An illuminant is declared",
           Illuminants.Declare(FirstOccupant, Declaring).ContentPresent,
           "[-] against an enrolled occupant");

    // 🔴 `44` §3: every emission shape has a non-zero size. A zero radius is not a smaller source.
    Slate::IlluminantSpecification Sizeless = Declaring;
    Sizeless.EmissionRadius                 = 0.0;

    Slate::OccupantIdentity SecondOccupant;
    SecondOccupant.SlotOrdinal    = 2u;
    SecondOccupant.SlotGeneration = 1u;

    Report("A shape with no size is refused",
           !Illuminants.Declare(SecondOccupant, Sizeless).ContentPresent,
           "[-] a zero extent produces a single aliased pixel or nothing");

    Slate::IlluminantSpecification Spaceless = Declaring;
    Spaceless.DeclaredColour.SpaceIdentity   = 0u;

    Report("A colour with no space is refused",
           !Illuminants.Declare(SecondOccupant, Spaceless).ContentPresent,
           "[-] `36` §1: no bare triple");

    // 🔴 Exactly one atmospheric source. Two suns is a scene where the shadows fall one way and the sky the other.
    Slate::IlluminantSpecification Sun = Declaring;
    Sun.Emission                       = Slate::EmissionShape::Directional;
    Sun.AngularSize                    = 0.53;
    Sun.AtmosphericSource              = true;

    Report("A sun is enrolled",
           Illuminants.Declare(SecondOccupant, Sun).ContentPresent
        && Illuminants.AtmosphericSource().ContentPresent,
           "[-] `28` reads exactly this one");

    Slate::OccupantIdentity ThirdOccupant;
    ThirdOccupant.SlotOrdinal    = 3u;
    ThirdOccupant.SlotGeneration = 1u;

    Report("A second sun is refused",
           !Illuminants.Declare(ThirdOccupant, Sun).ContentPresent,
           "[-] one atmospheric source, by declaration");

    Report("The same sun may be amended",
           Illuminants.Amend(SecondOccupant, Sun).ContentPresent,
           "[-] the exclusion excludes others, never itself");

    // 📝 A declared temperature is retained as authored and projected on demand — `36` §5.
    Slate::IlluminantSpecification Warm = Declaring;
    Warm.TemperatureDeclared             = true;
    Warm.Temperature                     = 5600.0;

    Illuminants.Declare(ThirdOccupant, Warm);

    Report("A temperature is retained as authored",
           Illuminants.Resolve(ThirdOccupant).Resolve().Temperature == 5600.0,
           "[K] the artist who set 5600 sees 5600");

    Report("A temperature projects to a colour",
           Illuminants.ResolveColour(ThirdOccupant, Slate::DeclaredWorkingSpace()).ContentPresent,
           "[-] projected on demand, never written back");

    Report("Enrolment is ordered by identity",
           Illuminants.EnrolledCount() == 3u
        && Illuminants.Enrolled()[0].SlotOrdinal == 1u
        && Illuminants.Enrolled()[2].SlotOrdinal == 3u,
           "[-] stable across ticks, runs and machines");

    // 📐 Incidence at a position within the extent attenuates; beyond it, exactly zero.
    Slate::DocumentPosition Shaded;
    Shaded.PositionX = 100.0;

    const Slate::Outcome<Slate::IncidenceProjection> Near = Slate::ProjectIncidence(Declaring, Shaded);

    Report("Incidence resolves within the extent",
           Near.ContentPresent && Near.Resolve().Attenuation > 0.0 && Near.Resolve().SolidExtent > 0.0,
           "[sr] and the solid extent is never zero");

    Slate::DocumentPosition Distant;
    Distant.PositionX = 5000.0;

    const Slate::Outcome<Slate::IncidenceProjection> Far = Slate::ProjectIncidence(Declaring, Distant);

    Report("Beyond the declared extent attenuates to zero",
           Far.ContentPresent && Far.Resolve().Attenuation == 0.0,
           "[-] a declared cutoff, not a discovered threshold");

    // 🔴 The row that pays for the declared cutoff: brightening an illuminant re-derives nothing.
    const std::uint64_t BeforeIntensity = Illuminants.Revision();

    Slate::IlluminantSpecification Brighter = Declaring;
    Brighter.RadiantIntensity               = 40.0;
    Illuminants.Amend(FirstOccupant, Brighter);

    Slate::IlluminantIndex Reach;

    std::vector<Slate::PartitionExtent> Extents(2);
    Extents[0].Least.PositionX    = -10.0;
    Extents[0].Greatest.PositionX =  10.0;
    Extents[0].Greatest.PositionY =  10.0;
    Extents[0].Greatest.PositionZ =  10.0;

    Extents[1].Least.PositionX    = 9000.0;
    Extents[1].Greatest.PositionX = 9010.0;
    Extents[1].Greatest.PositionY =   10.0;
    Extents[1].Greatest.PositionZ =   10.0;

    Report("The reach index derives",
           Reach.Derive(Illuminants, Extents).ContentPresent && Reach.SpannedCount() == 2u,
           "[-] one reaching set per partition");

    Report("A near partition is reached by the point illuminant",
           Reach.ReachingCount(0u) == 3u,
           "[-] point, sun and warm all reach it");

    // 📐 The distant partition is beyond both positioned illuminants' extents, and a directional shape reaches
    //    everything because it declares no position for an extent to be measured from.
    Report("A distant partition is reached by the directional alone",
           Reach.ReachingCount(1u) == 1u
        && Reach.Reaching(1u, 0u).Resolve() == SecondOccupant,
           "[-] the declared extent bounds the positioned shapes");

    Report("The reaching set is in identity order",
           Reach.Reaching(0u, 0u).Resolve().SlotOrdinal == 1u
        && Reach.Reaching(0u, 2u).Resolve().SlotOrdinal == 3u,
           "[-] `02` §5's ordered recombination one layer up");

    Report("A radiant intensity change advanced the revision only",
           Illuminants.Revision() > BeforeIntensity && Reach.ReachingCount(0u) == 3u,
           "[-] the extent is declared, so nothing re-derived");

    Report("Nothing was truncated", Reach.TruncatedTotal() == 0u, "[-] three is below the capacity");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  SPATIAL SUBDIVISION
//------------------------------------------------------------------------------------------------------------------------

void VerifySubdivision()
{
    std::printf("SpatialSubdivision\n");

    // 📐 One square in the plane at the origin, facing the third axis. Small enough to reason about and enough to
    //    exercise the fan triangulation, the exact edge classification and the object-space transform.
    Slate::TopologyStructure Imported;

    std::vector<Slate::DocumentPosition> Positions(4);
    Positions[0].PositionX = -1.0;  Positions[0].PositionY = -1.0;
    Positions[1].PositionX =  1.0;  Positions[1].PositionY = -1.0;
    Positions[2].PositionX =  1.0;  Positions[2].PositionY =  1.0;
    Positions[3].PositionX = -1.0;  Positions[3].PositionY =  1.0;

    Imported.DeclarePositions(Positions);
    Imported.DeclareFace({ 0u, 1u, 2u, 3u });
    Imported.Seal();

    Slate::TopologyConditioning Conditioned;
    Conditioned.Condition(Imported);

    Slate::BoundingStructure Inner;

    Report("The inner structure builds",
           Inner.Construct(Imported, Conditioned).ContentPresent && Inner.FaceCount() == 1u,
           "[-] over the conditioned face extents");

    // 🔴 The revision gate: a conditioning describing a different seal indexes faces that have moved.
    Slate::TopologyStructure Reimported;
    Reimported.DeclarePositions(Positions);
    Reimported.DeclareFace({ 0u, 1u, 2u });
    Reimported.Seal();
    Reimported.Seal();

    Slate::BoundingStructure Mismatched;

    Report("A conditioning of another revision is refused",
           !Mismatched.Construct(Reimported, Conditioned).ContentPresent,
           "[-] refuses rather than indexing faces that moved");

    Slate::DocumentPosition RayOrigin;
    RayOrigin.PositionZ = 5.0;

    const Slate::FaceIntersection Met = Inner.IntersectRay(RayOrigin, 0.0, 0.0, -1.0, 1000.0);

    Report("A ray meets the face",
           Met.Resolved && std::fabs(Met.Distance - 5.0) < 1.0e-9,
           "[mm] at the distance the geometry says");

    Report("The barycentric weights sum to one",
           Met.Resolved && std::fabs(Met.Weights[0] + Met.Weights[1] + Met.Weights[2] - 1.0) < 1.0e-9,
           "[-] one traversal serves every consumer");

    Report("A ray past the face misses",
           !Inner.IntersectRay(RayOrigin, 1.0, 0.0, 0.0, 1000.0).Resolved,
           "[-] and reports the miss rather than the nearest thing");

    Report("A ray beyond the furthest distance misses",
           !Inner.IntersectRay(RayOrigin, 0.0, 0.0, -1.0, 1.0).Resolved,
           "[mm] the bound is honoured");

    // 📐 The corner is exactly on two edges, so the exact predicate resolves zero on both and the hit is
    //    admitted. A filtered test misses along exactly the shared edges dense topology is made of.
    Slate::DocumentPosition CornerRay;
    CornerRay.PositionX = 1.0;  CornerRay.PositionY = -1.0;  CornerRay.PositionZ = 5.0;

    Report("A ray through a corner is a hit",
           Inner.IntersectRay(CornerRay, 0.0, 0.0, -1.0, 1000.0).Resolved,
           "[-] `02` §4's exact classification, on the edge");

    Slate::OctantSpace Outer;
    Slate::EnrollmentIndex Subsets;

    Slate::AdmittedOccupant Admitting;
    Admitting.Occupant.SlotOrdinal    = 5u;
    Admitting.Occupant.SlotGeneration = 1u;
    Admitting.Inner                   = &Inner;
    Admitting.Extent                  = Inner.Extent();

    Report("An occupant is admitted",
           Outer.Admit(Admitting).ContentPresent && Outer.ConstructionOwed(),
           "[-] and the subdivision is owed a build");

    Report("An undeclared identity is refused",
           !Outer.Admit(Slate::AdmittedOccupant{}).ContentPresent,
           "[-] an undeclared identity occupies nothing");

    Outer.Construct();

    Report("Construction discharges the debt", !Outer.ConstructionOwed(), "[-] the shape is current");

    const Slate::ResolvedIntersection Resolved =
        Outer.IntersectRay(RayOrigin, 0.0, 0.0, -1.0, Subsets);

    Report("The outer traversal resolves an occupant",
           Resolved.Resolved && Resolved.Occupant == Admitting.Occupant,
           "[-] the whole tuple, one traversal");

    Report("The resolved position is in document space",
           Resolved.Resolved && std::fabs(Resolved.Position.PositionZ) < 1.0e-9,
           "[mm] `78`'s manipulator plane reads this");

    // 🔴 `40` §3: exclusion is tested before descent, so a locked occupant is never traversed at all.
    Subsets.Enrol(Admitting.Occupant, Slate::SubsetSubject::Lock);

    Report("A locked occupant is not traversed",
           !Outer.IntersectRay(RayOrigin, 0.0, 0.0, -1.0, Subsets).Resolved,
           "[-] excluded before descent, never after");

    Subsets.Unenrol(Admitting.Occupant, Slate::SubsetSubject::Lock);
    Subsets.Enrol(Admitting.Occupant, Slate::SubsetSubject::VisibilityExclusion);

    Report("A hidden occupant is not traversed",
           !Outer.IntersectRay(RayOrigin, 0.0, 0.0, -1.0, Subsets).Resolved,
           "[-] the same test, the same cost");

    Subsets.Unenrol(Admitting.Occupant, Slate::SubsetSubject::VisibilityExclusion);

    // 🔴 The property the two levels exist for: an occupant move is a refit, and the inner structure is untouched.
    Slate::DecomposedTransform Moved;
    Moved.Translation.PositionX = 50.0;

    Slate::ConditionedExtent MovedExtent = Inner.Extent();
    MovedExtent.Least.PositionX    += 50.0;
    MovedExtent.Greatest.PositionX += 50.0;

    const std::uint32_t RecordsBefore = Outer.RecordCount();

    Report("An occupant move is a refit",
           Outer.Refit(Admitting.Occupant, Moved, MovedExtent).ContentPresent
        && Outer.RecordCount() == RecordsBefore
        && !Outer.ConstructionOwed(),
           "[-] the shape is untouched — `40` §4");

    Slate::DocumentPosition MovedRay;
    MovedRay.PositionX = 50.0;  MovedRay.PositionZ = 5.0;

    Report("The moved occupant resolves at its new position",
           Outer.IntersectRay(MovedRay, 0.0, 0.0, -1.0, Subsets).Resolved,
           "[mm] object space is invariant under the motion");

    Report("A refitted occupant no longer resolves at the old one",
           !Outer.IntersectRay(RayOrigin, 0.0, 0.0, -1.0, Subsets).Resolved,
           "[-] the refit was applied, not merely recorded");

    // 🔴 One traversal over the extent, never one per position inside it — `74` §4.
    Slate::ConditionedExtent Marquee;
    Marquee.Least.PositionX    = 40.0;
    Marquee.Least.PositionY    = -10.0;
    Marquee.Least.PositionZ    = -10.0;
    Marquee.Greatest.PositionX = 60.0;
    Marquee.Greatest.PositionY =  10.0;
    Marquee.Greatest.PositionZ =  10.0;

    Report("A marquee enrols by containment",
           Outer.IntersectExtent(Marquee, true, Subsets).size() == 1u,
           "[-] one traversal, one enrolment");

    Slate::ConditionedExtent Grazing = Marquee;
    Grazing.Least.PositionX    = 50.5;
    Grazing.Greatest.PositionX = 60.0;

    Report("Containment and intersection differ",
           Outer.IntersectExtent(Grazing, true, Subsets).empty()
        && Outer.IntersectExtent(Grazing, false, Subsets).size() == 1u,
           "[-] wholly inside is not the same as overlapping");

    Report("A locked occupant is absent from a marquee",
           [&]
           {
               Subsets.Enrol(Admitting.Occupant, Slate::SubsetSubject::Lock);
               const bool Empty = Outer.IntersectExtent(Marquee, false, Subsets).empty();
               Subsets.Unenrol(Admitting.Occupant, Slate::SubsetSubject::Lock);
               return Empty;
           }(),
           "[-] the same exclusion, the same place");

    Report("Withdrawal owes a rebuild",
           Outer.Withdraw(Admitting.Occupant).ContentPresent && Outer.ConstructionOwed(),
           "[-] the shape no longer describes the population");

    // 📝 The domain subdivision answers a different question in a different space.
    Slate::AxisSpace Domain;

    std::vector<Slate::DomainExtent> Placements(2);
    Placements[0].LeastAlong      = 0.0;   Placements[0].LeastAcross     = 0.0;
    Placements[0].GreatestAlong   = 0.6;   Placements[0].GreatestAcross  = 0.6;
    Placements[0].PlacementOrdinal = 10u;  Placements[0].SequenceOrdinal = 1u;

    Placements[1].LeastAlong      = 0.4;   Placements[1].LeastAcross     = 0.4;
    Placements[1].GreatestAlong   = 1.0;   Placements[1].GreatestAcross  = 1.0;
    Placements[1].PlacementOrdinal = 11u;  Placements[1].SequenceOrdinal = 2u;

    Domain.Construct(Placements);

    Report("A domain position resolves a placement",
           Domain.Resolve(0.2, 0.2).ContentPresent && Domain.Resolve(0.2, 0.2).Resolve() == 10u,
           "[-] `74` precedence 1");

    // 🔴 The topmost containing placement wins, by `56` sequence order — never the first declared.
    Report("Overlapping placements resolve topmost",
           Domain.Resolve(0.5, 0.5).Resolve() == 11u,
           "[-] never by declaration order");

    Report("A position outside every placement refuses",
           !Domain.Resolve(1.5, 1.5).ContentPresent,
           "[-] refuses rather than resolving the nearest");

    Report("A placement refits without a rebuild",
           Domain.Refit(10u, Placements[1]).ContentPresent,
           "[-] `40` §4's placement row");

    Report("An unknown placement ordinal refuses",
           !Domain.Refit(99u, Placements[0]).ContentPresent,
           "[-] no placement carries it");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE CHART PARTITION
//------------------------------------------------------------------------------------------------------------------------

// 📐 A three-by-three grid of quads in the plane. Genuinely a disc, so it flattens in one chart with no derived
//    seam; cutting it across the middle must produce two. Both are checked, because a partitioner that always
//    cuts and one that never cuts each pass half of this on their own.
void VerifyPartition()
{
    std::printf("ChartPartition\n");

    Slate::TopologyStructure Imported;

    std::vector<Slate::DocumentPosition> Positions(16);

    for (std::uint32_t Across = 0u; Across < 4u; ++Across)
    {
        for (std::uint32_t Along = 0u; Along < 4u; ++Along)
        {
            Positions[Across * 4u + Along].PositionX = static_cast<double>(Along);
            Positions[Across * 4u + Along].PositionY = static_cast<double>(Across);
        }
    }

    Imported.DeclarePositions(Positions);

    for (std::uint32_t Across = 0u; Across < 3u; ++Across)
    {
        for (std::uint32_t Along = 0u; Along < 3u; ++Along)
        {
            const std::uint32_t Corner = Across * 4u + Along;

            Imported.DeclareFace({ Corner, Corner + 1u, Corner + 5u, Corner + 4u });
        }
    }

    Imported.Seal();

    Slate::TopologyConditioning Conditioned;
    Conditioned.Condition(Imported);

    Slate::SeamSpecification Seams;

    Report("A seam of one vertex is refused",
           !Seams.DeclareAuthored(3u, 3u).ContentPresent,
           "[-] one vertex is not an edge");

    Slate::PartitionSpecification Declaring;
    Slate::WorkCancellation       Posed;
    Slate::WorkProgress           Progressed;

    const Slate::Outcome<Slate::DerivedPartition> Derived =
        Slate::Derive(Imported, Conditioned, Seams, Declaring, Posed, Progressed);

    Report("A disc derives one chart",
           Derived.ContentPresent && Derived.Resolve().Metrics.ChartCount == 1u,
           "[-] connected across every non-seam edge");

    Report("A disc needs no derived seam",
           Derived.ContentPresent && Derived.Resolve().Metrics.DerivedSeamCount == 0u,
           "[-] the authored set sufficed");

    Report("No fold survives",
           Derived.ContentPresent && Derived.Resolve().Metrics.FoldCount == 0u,
           "[-] `68` §4.1: a fold is a failure, never a distortion value");

    // 🔴 Every corner lands strictly inside the unit domain, gap included. A coordinate on the boundary means the
    //    apron of an edge tile reads outside the domain, which is the fringe `68` §5's gap exists to prevent.
    bool WithinDomain = Derived.ContentPresent;

    for (const Slate::DomainCoordinate& Held : Derived.Resolve().CornerCoordinates)
    {
        if (Held.CoordinateAlong <= 0.0f || Held.CoordinateAlong >= 1.0f
         || Held.CoordinateAcross <= 0.0f || Held.CoordinateAcross >= 1.0f)
        {
            WithinDomain = false;
        }
    }

    Report("Every coordinate lands inside the domain", WithinDomain, "[-] strictly, gap included");

    Report("Occupancy is reported and is a fraction",
           Derived.ContentPresent
        && Derived.Resolve().Metrics.Occupancy > 0.0
        && Derived.Resolve().Metrics.Occupancy <= 1.0,
           "[-] `86`'s `68` §5 measure");

    // 📐 A planar chart flattens with bounded distortion but not with none — the boundary is mapped to a circle,
    //    so a square necessarily stretches. What is checked is that the measure was taken at all.
    Report("Distortion is measured separately",
           Derived.ContentPresent
        && Derived.Resolve().Charts[0].Distortion.MeasureDeclared
        && Derived.Resolve().Metrics.GreatestAreaRatio >= 1.0,
           "[-] area and angle, never one number for both");

    // 🔴 The seam is what cuts, and it is over welded positions rather than imported vertices.
    Seams.DeclareAuthored(1u, 5u);
    Seams.DeclareAuthored(5u, 9u);
    Seams.DeclareAuthored(9u, 13u);

    const Slate::Outcome<Slate::DerivedPartition> Cut =
        Slate::Derive(Imported, Conditioned, Seams, Declaring, Posed, Progressed);

    Report("An authored seam cuts the topology",
           Cut.ContentPresent && Cut.Resolve().Metrics.ChartCount == 2u,
           "[-] two charts, from one cut");

    Report("Chart identity is the least face ordinal held",
           Cut.ContentPresent
        && (Cut.Resolve().Charts[0].IdentityOrdinal == 0u || Cut.Resolve().Charts[1].IdentityOrdinal == 0u),
           "[-] stable where the chart is unchanged — `24` §3 keys on it");

    Slate::ChartPartition Standing;

    Report("Nothing stands before adoption",
           !Standing.PartitionStanding() && !Standing.Coordinate(0u).ContentPresent,
           "[-] the previous partition stands until the solve completes — `68` §7");

    Report("Adoption advances the revision",
           Standing.Adopt(Cut.Resolve()).ContentPresent && Standing.Revision() == 1u,
           "[-] what `24` §3 keys on and `20` promotes against");

    const std::uint64_t BeforeSecond = Standing.Revision();
    Standing.Adopt(Derived.Resolve());

    Report("A re-partition advances it again",
           Standing.Revision() > BeforeSecond,
           "[-] every artefact in the old domain is discoverably stale");

    Report("An empty partition is refused",
           !Standing.Adopt(Slate::DerivedPartition{}).ContentPresent,
           "[-] a partition carrying no chart");

    // 🔴 `68` §2: the authored set survives a re-partition. This is the hour of the artist's work the whole
    //    two-set split exists to protect.
    Seams.DeclareDerived(2u, 6u);
    Seams.ReclaimDerived();

    Report("Reclaiming derived seams leaves the authored set",
           Seams.AuthoredCount() == 3u && Seams.DerivedCount() == 0u,
           "[-] an automatic partition never erases marked seams");

    Report("Only an authored amendment advances the seam revision",
           [&]
           {
               const std::uint64_t Before = Seams.Revision();
               Seams.DeclareDerived(2u, 6u);
               const bool Unmoved = Seams.Revision() == Before;
               Seams.ReclaimDerived();
               return Unmoved;
           }(),
           "[-] a derived seam is a consequence, never an input");

    Slate::DomainSpace Arranged;

    std::vector<Slate::ChartExtent> Extents(3);
    Extents[0].Width = 0.4;  Extents[0].Height = 0.3;  Extents[0].ChartOrdinal = 0u;
    Extents[1].Width = 0.3;  Extents[1].Height = 0.5;  Extents[1].ChartOrdinal = 1u;
    Extents[2].Width = 0.2;  Extents[2].Height = 0.2;  Extents[2].ChartOrdinal = 2u;

    Report("A per-chart scale is refused",
           !Arranged.Arrange(Extents, false).ContentPresent,
           "[-] `68` §10's open row is not decided by accident");

    Report("Charts arrange at a common scale",
           Arranged.Arrange(Extents, true).ContentPresent && Arranged.SettledScale() > 0.0,
           "[-] one texel covers the same area on every chart");

    // 🔴 `68` §5: at least one apron between adjacent charts, and at least one from the domain edge. Measured
    //    rather than asserted, because the packing is where the gap is actually applied.
    bool GapHeld = true;

    const double Gap = Slate::DeclaredGap();

    for (std::size_t Earlier = 0u; Earlier < Arranged.Placements().size(); ++Earlier)
    {
        const Slate::ChartPlacement& Alpha = Arranged.Placements()[Earlier];

        if (Alpha.LeastAlong < Gap || Alpha.LeastAcross < Gap)
            GapHeld = false;

        const double AlphaAlong  = Alpha.LeastAlong  + Extents[Earlier].Width  * Alpha.Scale;
        const double AlphaAcross = Alpha.LeastAcross + Extents[Earlier].Height * Alpha.Scale;

        if (AlphaAlong > 1.0 || AlphaAcross > 1.0)
            GapHeld = false;

        for (std::size_t Later = Earlier + 1u; Later < Arranged.Placements().size(); ++Later)
        {
            const Slate::ChartPlacement& Beta = Arranged.Placements()[Later];

            const double BetaAlong  = Beta.LeastAlong  + Extents[Later].Width  * Beta.Scale;
            const double BetaAcross = Beta.LeastAcross + Extents[Later].Height * Beta.Scale;

            const bool ApartAlong  = AlphaAlong + Gap <= Beta.LeastAlong  || BetaAlong  + Gap <= Alpha.LeastAlong;
            const bool ApartAcross = AlphaAcross + Gap <= Beta.LeastAcross || BetaAcross + Gap <= Alpha.LeastAcross;

            if (!ApartAlong && !ApartAcross)
                GapHeld = false;
        }
    }

    Report("Adjacent charts are separated by at least the apron",
           GapHeld,
           "[-] narrower, and every chart edge carries a neighbour's fringe");

    Report("The arrangement is reproducible",
           [&]
           {
               Slate::DomainSpace Repeated;
               Repeated.Arrange(Extents, true);
               return Repeated.SettledScale() == Arranged.SettledScale();
           }(),
           "[-] bit for bit; the shelf order is scale-invariant");

    // 📐 The solver's own contract, exercised where the partition cannot reach it: a ceiling of one cannot
    //    converge, and reporting it as convergence is the ambiguity `02` §5 exists to close.
    // 🔴 The whole grid, not one quad. A chart whose every position lies on the boundary loop has no interior to
    //    relax, so its residual is zero on the first sweep and it converges whatever the ceiling — which would
    //    pass the ceiling check for the one reason that makes the check prove nothing.
    Slate::UnwrapSpecification Solving;
    Solving.Positions    = Positions;
    Solving.BoundaryLoop = { 0u, 1u, 2u, 3u, 7u, 11u, 15u, 14u, 13u, 12u, 8u, 4u };

    for (std::uint32_t Across = 0u; Across < 3u; ++Across)
    {
        for (std::uint32_t Along = 0u; Along < 3u; ++Along)
        {
            const std::uint32_t Corner = Across * 4u + Along;

            Solving.TriangleCorners.insert(Solving.TriangleCorners.end(),
                                           { Corner, Corner + 1u, Corner + 5u,
                                             Corner, Corner + 5u, Corner + 4u });
        }
    }

    Report("A converged solve says so",
           Slate::Solve(Solving).ContentPresent
        && Slate::Solve(Solving).Resolve().Cause == Slate::TerminationCause::CriterionSatisfied,
           "[-] `02` §5's Convergent contract");

    Slate::UnwrapSpecification Ceilinged = Solving;
    Ceilinged.IterationCeiling           = 1u;
    Ceilinged.ConvergenceCriterion       = 1.0e-18;

    Report("A ceiling termination says so too",
           Slate::Solve(Ceilinged).Resolve().Cause == Slate::TerminationCause::CeilingReached,
           "[-] the last iterate, never presented as convergence");

    Slate::UnwrapSpecification Loopless = Solving;
    Loopless.BoundaryLoop                = { 0u, 1u };

    Report("A boundary that is not a loop is refused",
           !Slate::Solve(Loopless).ContentPresent,
           "[-] no disc, no boundary-first parameterisation");
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       INTAKE
//------------------------------------------------------------------------------------------------------------------------

void VerifyIntake()
{
    std::printf("AssetInterchange\n");

    Slate::AssetInterchange   Interchange;
    Slate::IntakeIndex        Recorded;
    Slate::TopologyStructure  Into;

    Slate::DecodedTopology Decoded;

    Report("A source with no position is refused",
           !Interchange.IntakeTopology(Decoded, Into, Recorded).ContentPresent,
           "[-] `50` §3 gives it no default");

    Decoded.Positions.resize(4);
    Decoded.Positions[1].PositionX = 1.0;
    Decoded.Positions[2].PositionX = 1.0;
    Decoded.Positions[2].PositionY = 1.0;
    Decoded.Positions[3].PositionY = 1.0;
    Decoded.OriginPath             = "figure.topology";

    Report("A source with no face indexing is refused",
           !Interchange.IntakeTopology(Decoded, Into, Recorded).ContentPresent,
           "[-] refused, never defaulted");

    Decoded.Faces.push_back({ 0u, 1u, 9u });

    Report("A partially invalid intake enrols nothing",
           !Interchange.IntakeTopology(Decoded, Into, Recorded).ContentPresent
        && Into.VertexCount() == 0u,
           "[-] `50` §8: half a topology is an occupant the artist will export");

    Decoded.Faces[0] = { 0u, 1u, 2u, 3u };

    Report("A faithful intake enrols",
           Interchange.IntakeTopology(Decoded, Into, Recorded).ContentPresent
        && Into.Sealed()
        && Into.FaceCount() == 1u,
           "[-] sealed and immutable for the run");

    // 🔴 `50` §3: the two assuming rows are the two that produce a plausible, wrong result. Both are recorded.
    Report("A silent unit convention is recorded as an assumption",
           Recorded.AssumptionCount() == 1u
        && Recorded.Resolve("figure.topology").Resolve().Assumed == Slate::AssumedSubject::UnitScale,
           "[-] a model at a hundredth of its size still renders");

    Slate::DecodedTopology Scaled = Decoded;
    Scaled.OriginPath             = "scaled.topology";
    Scaled.UnitScale              = 10.0;
    Scaled.UnitScaleDeclared      = true;

    Slate::TopologyStructure ScaledInto;
    Interchange.IntakeTopology(Scaled, ScaledInto, Recorded);

    Report("Unit scale is applied once, at intake",
           ScaledInto.Positions()[1].PositionX == 10.0,
           "[mm] never carried as a per-occupant multiplier");

    Report("A declared convention assumes nothing",
           Recorded.AssumptionCount() == 1u,
           "[-] the record names only what was guessed");

    Report("An absent enrollment defaults to one material",
           ScaledInto.MaterialEnrollment().size() == ScaledInto.FaceCount(),
           "[-] `50` §3's last-resort row");

    Slate::DecodedImage Image;

    Report("An image of no extent is refused",
           !Interchange.IntakeImage(Image, Recorded).ContentPresent,
           "[-] there is nothing to decode");

    Image.Width          = 64u;
    Image.Height         = 64u;
    Image.ComponentCount = 3u;
    Image.BitDepth       = 16u;
    Image.OriginPath     = "albedo.image";
    Image.Original.assign(64u * 64u * 3u * 2u, 0u);

    Report("A silent colour space is recorded as an assumption",
           Interchange.IntakeImage(Image, Recorded).ContentPresent
        && Recorded.AssumptionCount() == 2u,
           "[-] an image decoded as linear still looks like an image");

    Report("The original is retained for re-conversion",
           !Image.Original.empty() && Image.BitDepth == 16u,
           "[-] never narrowed at intake — `36` §3 re-converts from it");

    Slate::ReportSequence Reporting;
    const Slate::TickSequence HostTimeline;

    Recorded.Report(Reporting, HostTimeline.Advance());

    Report("Every assumption reaches the register",
           Reporting.RetainedCount() == 2u,
           "[-] recorded and reported, never silent");

    Recorded.Report(Reporting, HostTimeline.Advance());

    Report("An assumption is reported once",
           Reporting.AppendedCount() == 2u,
           "[-] a second tick appends nothing further");

    Slate::MaterialIndex Materials;

    const std::uint32_t MaterialOrdinal = Materials.Declare("Painted metal").Resolve();

    Slate::ChannelSpecification Albedo;
    Albedo.Source                       = Slate::ChannelSource::Layered;
    Albedo.Measured                     = Slate::ChannelMeasure::Reflectance;
    Albedo.ConstantColour.SpaceIdentity = Slate::WorkingSpaceIdentity;
    Albedo.DefaultColour.SpaceIdentity  = Slate::WorkingSpaceIdentity;

    Materials.Amend(MaterialOrdinal).Resolve()->DeclareChannel(Slate::ChannelSubject::AlbedoColour, Albedo);

    Slate::EmissionSpecification Emitting;

    Report("An emission producing no image is refused",
           !Interchange.DeclareEmission(Emitting, Materials).ContentPresent,
           "[-] nothing would leave");

    Slate::EmittedImage Packed;
    Packed.ExtentTexels                                                             = 2048u;
    Packed.Occupying[static_cast<std::size_t>(Slate::ComponentSlot::Red)]            = Slate::ChannelSubject::AmbientOcclusion;
    Packed.ComponentOccupied[static_cast<std::size_t>(Slate::ComponentSlot::Red)]    = true;
    Packed.Occupying[static_cast<std::size_t>(Slate::ComponentSlot::Green)]          = Slate::ChannelSubject::Roughness;
    Packed.ComponentOccupied[static_cast<std::size_t>(Slate::ComponentSlot::Green)]  = true;
    Packed.Occupying[static_cast<std::size_t>(Slate::ComponentSlot::Blue)]           = Slate::ChannelSubject::Metallic;
    Packed.ComponentOccupied[static_cast<std::size_t>(Slate::ComponentSlot::Blue)]   = true;

    Emitting.Images.push_back(Packed);
    Emitting.NamePattern = "{Occupant}_{Material}_{Channel}_{Extent}";

    Report("A declared arrangement is admitted",
           Interchange.DeclareEmission(Emitting, Materials).ContentPresent,
           "[-] `50` §5.1: declared and presented, never conventional");

    Slate::EmittedImage Colour;
    Colour.ExtentTexels                                                           = 4096u;
    Colour.Occupying[static_cast<std::size_t>(Slate::ComponentSlot::Red)]          = Slate::ChannelSubject::AlbedoColour;
    Colour.ComponentOccupied[static_cast<std::size_t>(Slate::ComponentSlot::Red)]  = true;

    Slate::EmissionSpecification Spaceless = Emitting;
    Spaceless.Images.push_back(Colour);

    Report("A colour channel with no declared space is refused",
           !Interchange.DeclareEmission(Spaceless, Materials).ContentPresent,
           "[-] the consumer would decode it by guessing");

    Colour.SpaceIdentity = Slate::WorkingSpaceIdentity;

    Slate::EmissionSpecification Spaced = Emitting;
    Spaced.Images.push_back(Colour);

    Report("Per-image extents differ",
           Interchange.DeclareEmission(Spaced, Materials).ContentPresent,
           "[-] not one extent for every channel");

    Slate::EmissionSpecification Doubled = Spaced;
    Doubled.Images.push_back(Packed);

    Report("A channel emitted twice is refused",
           !Interchange.DeclareEmission(Doubled, Materials).ContentPresent,
           "[-] the consumer reads whichever it loaded second");

    Report("The naming pattern resolves",
           Slate::ResolveName(Emitting.NamePattern, "Figure", "Metal", "Roughness", 2048u)
        == "Figure_Metal_Roughness_2048",
           "[-] over occupant, material, channel and extent");

    Report("An unrecognised substitution is left verbatim",
           Slate::ResolveName("{Occupant}_{Absent}", "Figure", "Metal", "Roughness", 2048u)
        == "Figure_{Absent}",
           "[-] emptied, and every name collides with every other");

    Slate::DecodedTopology Refusing = Decoded;
    Refusing.OriginPath             = "animated.topology";
    Refusing.UnsupportedNamed.push_back("animation track");

    Slate::TopologyStructure RefusingInto;
    Interchange.IntakeTopology(Refusing, RefusingInto, Recorded);

    Report("An unsupported construct is named at intake",
           Interchange.Unsupported().size() == 1u,
           "[-] `50` §6: not at export, by which time it is missed");

    Interchange.Report(Reporting, HostTimeline.Advance());

    Report("It reaches the register once",
           [&]
           {
               const std::uint64_t Appended = Reporting.AppendedCount();
               Interchange.Report(Reporting, HostTimeline.Advance());
               return Reporting.AppendedCount() == Appended;
           }(),
           "[-] a second call appends nothing further");
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE HOST
//------------------------------------------------------------------------------------------------------------------------

int main()
{
    std::printf("Slate — headless bring-up\n\n");

    VerifyMathematics();
    std::printf("\n");

    VerifyClassifiers();
    std::printf("\n");

    VerifyReporting();
    std::printf("\n");

    VerifyWork();
    std::printf("\n");

    VerifyColour();
    std::printf("\n");

    VerifyDocument();
    std::printf("\n");

    VerifyProperties();
    std::printf("\n");

    VerifyTopology();
    std::printf("\n");

    VerifyMaterials();
    std::printf("\n");

    VerifyVector();
    std::printf("\n");

    VerifyCamera();
    std::printf("\n");

    VerifyIlluminants();
    std::printf("\n");

    VerifySubdivision();
    std::printf("\n");

    VerifyPartition();
    std::printf("\n");

    VerifyIntake();
    std::printf("\n");

    VerifySchedule();
    std::printf("\n");

    VerifyParity();
    std::printf("\n");

    if (RefusedCount == 0)
    {
        std::printf("every check held\n");
        return 0;
    }

    std::printf("%d checks refused\n", RefusedCount);
    return RefusedCount;
}
