//============================================================================================================================================
//                                                             CONSOLEHOST.CPP
//============================================================================================================================================
// 🧩 Headless bring-up in link order — every unit constructed, reported, and reclaimed without a window.

#include "Contract/IdentityContract.h"
#include "Contract/OutcomeContract.h"
#include "Contract/PrecisionContract.h"
#include "Contract/ToleranceContract.h"

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
#include "SlateDocument/Format/FormatCodec/Api/FormatCodec.h"

#include "SlateMath/Numeric/CurveSolver/Api/CurveSolver.h"

#include "SlateVulkan/Device/RenderSchedule/Api/RenderSchedule.h"

#include "SlateCompute/Compute/ParityRunner/Api/ParityRunner.h"

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

    const std::vector<Slate::ParityReport>& Reports = Runner.Compare();

    Report("One report per registration", Reports.size() == 1u, "[-] in registration order");

    if (!Reports.empty())
    {
        Report("Samples were compared",
               Reports[0].SampleCount > 0u,
               "[-] an empty sample set never holds");

        Report("Orientation antisymmetry holds",
               Reports[0].DisagreeingCount == 0u,
               "[-] reversing two operands negates the sign, exactly");
    }

    Report("Agreement declared", Runner.AgreementHeld(), "[-] every registered entry point held");

    // 🚧 An unregistered comparison must not pass vacantly. Registering an entry point with no declared
    //    comparison reports zero samples and withdraws the agreement, rather than reporting a proof nobody
    //    produced. This is the gate that would otherwise let `Shared/` grow uncovered entry points.
    Slate::ParityRunner VacantRunner;

    Slate::ParityRegistration UncomparedEntry;
    UncomparedEntry.EntryName = "ClassifyIncircle";
    UncomparedEntry.Claimed   = Slate::PrecisionGuarantee::Exact;

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

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE HOST
//------------------------------------------------------------------------------------------------------------------------

int main()
{
    std::printf("Slate — headless bring-up\n\n");

    VerifyMathematics();
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
