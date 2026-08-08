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

#include "SlateDocument/Document/PopulationIndex/Api/PopulationIndex.h"
#include "SlateDocument/Document/RevisionSequence/Api/RevisionSequence.h"
#include "SlateDocument/Format/FormatCodec/Api/FormatCodec.h"

#include "SlateVulkan/Device/RenderSchedule/Api/RenderSchedule.h"

#include "SlateCompute/Compute/ParityRunner/Api/ParityRunner.h"

// 📝 🔴 This host names no interface component and constructs no instance. `00` §2.2 keeps exactly one copy
//    of ImGui, compiled inside SlateUI; a headless host reaching for it would be linking a window-system
//    attachment it can never construct. `SlateUI` and the vendor edge belong to the windowed host.

#include <cstdio>
#include <cstring>
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

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE HOST
//------------------------------------------------------------------------------------------------------------------------

int main()
{
    std::printf("Slate — headless bring-up\n\n");

    VerifyMathematics();
    std::printf("\n");

    VerifyDocument();
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
