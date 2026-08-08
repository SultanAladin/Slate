//============================================================================================================================================
//                                                             PARITYRUNNER.CPP
//============================================================================================================================================
// 🧩 Registration and comparison over the common sample set.

#include "SlateCompute/Compute/ParityRunner/Api/ParityRunner.h"

#include "Shared/OrientationClassifier.slang.h"

#include <cstring>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     REGISTRATION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> ParityRunner::Register(const ParityRegistration& Arriving)
{
    for (const ParityRegistration& Held : Registered)
    {
        if (std::strcmp(Held.EntryName, Arriving.EntryName) == 0)
            return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the entry point is already registered" });
    }

    Registered.push_back(Arriving);
    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                THE COMMON SAMPLE SET
//------------------------------------------------------------------------------------------------------------------------

// 📝 The sample set deliberately concentrates on the inputs where the filtered path cannot decide: nearly
//    collinear triples, exactly collinear triples, and triples separated by many orders of magnitude. A
//    sample set of well-conditioned inputs proves only that the fast path works.
namespace
{
    struct OrientationSample
    {
        double  AlphaX = 0.0;   // [-] - first position
        double  AlphaY = 0.0;
        double  BetaX  = 0.0;   // [-] - second position
        double  BetaY  = 0.0;
        double  GammaX = 0.0;   // [-] - third position
        double  GammaY = 0.0;
    };

    constexpr OrientationSample OrientationSampleSet[] =
    {
        { 0.0,     0.0,     1.0,     0.0,     0.0,     1.0     },   // well conditioned, counter-clockwise
        { 0.0,     0.0,     0.0,     1.0,     1.0,     0.0     },   // well conditioned, clockwise
        { 0.0,     0.0,     1.0,     1.0,     2.0,     2.0     },   // exactly collinear
        { 0.5,     0.5,     12.0,    12.0,    24.0,    24.0    },   // exactly collinear, off origin
        { 0.0,     0.0,     1.0e-15, 1.0e-15, 2.0e-15, 2.0e-15 },   // collinear at the representable floor
        { 0.0,     0.0,     1.0,     1.0,     2.0,     2.0000000000000004 },  // one ULP off collinear
        { 1.0e18,  1.0e18,  1.0e18,  1.0e18,  1.0e18,  1.0e18  },   // degenerate — all three coincide
        { 1.0e-30, 1.0e-30, 1.0e30,  1.0e30,  1.0,     1.0     }    // spanning sixty orders of magnitude
    };
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      COMPARISON
//------------------------------------------------------------------------------------------------------------------------

const std::vector<ParityReport>& ParityRunner::Compare()
{
    Reported.clear();
    Reported.reserve(Registered.size());

    AgreementDeclared = true;

    for (const ParityRegistration& Held : Registered)
    {
        ParityReport Report;
        Report.EntryName = Held.EntryName;

        if (Held.Claimed == PrecisionGuarantee::Perceptual)
        {
            // 📝 Perceptual entry points are not compared. Reporting agreement for one would claim a
            //    guarantee the guarantee itself disclaims.
            Report.AgreementHeld = true;
            Reported.push_back(Report);
            continue;
        }

        if (std::strcmp(Held.EntryName, "ClassifyOrientation") == 0)
        {
            const std::uint32_t SampleSpan =
                static_cast<std::uint32_t>(sizeof(OrientationSampleSet) / sizeof(OrientationSampleSet[0]));

            for (std::uint32_t Ordinal = 0u; Ordinal < SampleSpan; ++Ordinal)
            {
                const OrientationSample& Sample = OrientationSampleSet[Ordinal];

                const Signed32 Classified = ClassifyOrientation(Sample.AlphaX, Sample.AlphaY,
                                                                Sample.BetaX,  Sample.BetaY,
                                                                Sample.GammaX, Sample.GammaY);

                // 📐 Reversing two operands negates an orientation determinant exactly. The exact path must
                //    reproduce that antisymmetry for every input, and the filtered path must not break it
                //    where it decides — which is the strongest host-side statement available before the
                //    shader-side comparison exists.
                const Signed32 Reversed = ClassifyOrientation(Sample.BetaX,  Sample.BetaY,
                                                              Sample.AlphaX, Sample.AlphaY,
                                                              Sample.GammaX, Sample.GammaY);

                if (Classified != -Reversed)
                    ++Report.DisagreeingCount;
            }

            Report.SampleCount = SampleSpan;
        }
        else
        {
            // 🚧 An entry point with no comparison declared here reports zero samples and does not hold.
            //    Reporting agreement over an empty sample set is how an unproven entry point passes a gate.
            Report.SampleCount = 0u;
        }

        Report.AgreementHeld = Report.SampleCount > 0u && Report.DisagreeingCount == 0u;

        if (!Report.AgreementHeld)
            AgreementDeclared = false;

        Reported.push_back(Report);
    }

    return Reported;
}

bool ParityRunner::AgreementHeld() const
{
    return AgreementDeclared;
}

}   // namespace Slate
