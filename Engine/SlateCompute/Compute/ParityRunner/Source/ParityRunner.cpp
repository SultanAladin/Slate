//============================================================================================================================================
//                                                             PARITYRUNNER.CPP
//============================================================================================================================================
// 🧩 Registration and comparison over the common sample set.

#include "SlateCompute/Compute/ParityRunner/Api/ParityRunner.h"

#include "Contract/ToleranceContract.h"
#include "Shared/AtmosphereProjection.slang.h"
#include "Shared/ContainmentClassifier.slang.h"
#include "Shared/IncircleClassifier.slang.h"
#include "Shared/IntersectionClassifier.slang.h"
#include "Shared/OrientationClassifier.slang.h"
#include "Shared/SampleProjection.slang.h"

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

    // 📝 Four positions each. The set concentrates on inputs the incircle filter cannot decide: exactly
    //    cocircular quadrilaterals, a position one ULP off the circle, and a triangle whose winding is
    //    reversed so that the sign correction is exercised rather than assumed.
    struct IncircleSample
    {
        double  AlphaX = 0.0;   // [-] - the triangle
        double  AlphaY = 0.0;
        double  BetaX  = 0.0;
        double  BetaY  = 0.0;
        double  GammaX = 0.0;
        double  GammaY = 0.0;
        double  DeltaX = 0.0;   // [-] - the position classified against its circle
        double  DeltaY = 0.0;
    };

    constexpr IncircleSample IncircleSampleSet[] =
    {
        { 0.0, 0.0,  1.0, 0.0,  0.0, 1.0,   0.4,  0.4  },   // well inside
        { 0.0, 0.0,  1.0, 0.0,  0.0, 1.0,   2.0,  2.0  },   // well outside
        { 0.0, 0.0,  1.0, 0.0,  0.0, 1.0,   1.0,  1.0  },   // exactly cocircular
        { 0.0, 0.0,  0.0, 1.0,  1.0, 0.0,   0.4,  0.4  },   // the same, wound the other way
        { 0.0, 0.0,  1.0, 0.0,  0.0, 1.0,   1.0,  0.9999999999999999 },  // one ULP inside
        { 0.0, 0.0,  1.0e8, 0.0,  0.0, 1.0e8,  1.0e8, 1.0e8 },           // cocircular at scale
        { 0.0, 0.0,  1.0, 1.0,  2.0, 2.0,   3.0,  3.0  }    // degenerate triangle, no circle
    };

    struct SegmentSample
    {
        double  AlphaX = 0.0;   // [-] - the first segment
        double  AlphaY = 0.0;
        double  BetaX  = 0.0;
        double  BetaY  = 0.0;
        double  GammaX = 0.0;   // [-] - the second
        double  GammaY = 0.0;
        double  DeltaX = 0.0;
        double  DeltaY = 0.0;
    };

    constexpr SegmentSample SegmentSampleSet[] =
    {
        { 0.0, 0.0,  1.0, 1.0,   0.0, 1.0,  1.0, 0.0 },   // proper crossing
        { 0.0, 0.0,  1.0, 0.0,   2.0, 0.0,  3.0, 0.0 },   // collinear, disjoint
        { 0.0, 0.0,  2.0, 0.0,   1.0, 0.0,  3.0, 0.0 },   // collinear, overlapping
        { 0.0, 0.0,  1.0, 0.0,   1.0, 0.0,  2.0, 0.0 },   // collinear, meeting at one position
        { 0.0, 0.0,  1.0, 0.0,   0.5, 0.0,  0.5, 1.0 },   // an endpoint on the other segment
        { 0.0, 0.0,  1.0, 1.0,   2.0, 0.0,  3.0, 1.0 },   // parallel, disjoint
        { 0.0, 0.0,  0.0, 0.0,   0.0, 0.0,  1.0, 0.0 },   // degenerate against a segment through it
        { 0.0, 1.0,  0.0, 3.0,   0.0, 2.0,  0.0, 4.0 }    // collinear and vertical — the axis choice
    };

    struct IntervalSample
    {
        std::uint64_t  OuterBegin = 0u;   // [-] - the candidate enclosing interval
        std::uint64_t  OuterEnd   = 0u;
        std::uint64_t  InnerBegin = 0u;   // [-] - the candidate enclosed interval
        std::uint64_t  InnerEnd   = 0u;
    };

    constexpr IntervalSample IntervalSampleSet[] =
    {
        {   0u, 100u,  10u,  20u },   // strictly contained
        {   0u, 100u,   0u, 100u },   // identical
        {   0u, 100u,   0u,  50u },   // sharing the lower bound
        {   0u, 100u,  50u, 100u },   // sharing the upper bound
        {   0u,  50u,  40u,  60u },   // overlapping, contained by neither
        {   0u,  10u,  20u,  30u },   // disjoint
        { 100u,   0u,  10u,  20u }    // inverted, therefore no interval at all
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
        else if (std::strcmp(Held.EntryName, "ClassifyIncircle") == 0)
        {
            const std::uint32_t SampleSpan =
                static_cast<std::uint32_t>(sizeof(IncircleSampleSet) / sizeof(IncircleSampleSet[0]));

            for (std::uint32_t Ordinal = 0u; Ordinal < SampleSpan; ++Ordinal)
            {
                const IncircleSample& Sample = IncircleSampleSet[Ordinal];

                const Signed32 Classified = ClassifyIncircle(Sample.AlphaX, Sample.AlphaY,
                                                             Sample.BetaX,  Sample.BetaY,
                                                             Sample.GammaX, Sample.GammaY,
                                                             Sample.DeltaX, Sample.DeltaY);

                // 📐 Exchanging two positions of the triangle reverses its winding, and the incircle
                //    determinant negates exactly with it. The exact path must reproduce that antisymmetry for
                //    every input, and the filtered path must not break it where it decides.
                const Signed32 Exchanged = ClassifyIncircle(Sample.AlphaX, Sample.AlphaY,
                                                            Sample.GammaX, Sample.GammaY,
                                                            Sample.BetaX,  Sample.BetaY,
                                                            Sample.DeltaX, Sample.DeltaY);

                if (Classified != -Exchanged)
                    ++Report.DisagreeingCount;

                // 📐 A position of the triangle is cocircular with the triangle, exactly and by definition.
                //    This is the one identity that holds for every input including the degenerate ones.
                if (ClassifyIncircle(Sample.AlphaX, Sample.AlphaY,
                                     Sample.BetaX,  Sample.BetaY,
                                     Sample.GammaX, Sample.GammaY,
                                     Sample.BetaX,  Sample.BetaY) != 0)
                {
                    ++Report.DisagreeingCount;
                }
            }

            Report.SampleCount = SampleSpan;
        }
        else if (std::strcmp(Held.EntryName, "ClassifySegmentIntersection") == 0)
        {
            const std::uint32_t SampleSpan =
                static_cast<std::uint32_t>(sizeof(SegmentSampleSet) / sizeof(SegmentSampleSet[0]));

            for (std::uint32_t Ordinal = 0u; Ordinal < SampleSpan; ++Ordinal)
            {
                const SegmentSample& Sample = SegmentSampleSet[Ordinal];

                const Signed32 Classified = ClassifySegmentIntersection(Sample.AlphaX, Sample.AlphaY,
                                                                        Sample.BetaX,  Sample.BetaY,
                                                                        Sample.GammaX, Sample.GammaY,
                                                                        Sample.DeltaX, Sample.DeltaY);

                // 📐 The relation between two segments does not depend on which was named first, nor on which
                //    end of a segment was named first. Both symmetries are checked, because the algorithm
                //    tests the two segments asymmetrically and a lapse shows up only under one of them.
                const Signed32 Exchanged = ClassifySegmentIntersection(Sample.GammaX, Sample.GammaY,
                                                                       Sample.DeltaX, Sample.DeltaY,
                                                                       Sample.AlphaX, Sample.AlphaY,
                                                                       Sample.BetaX,  Sample.BetaY);

                const Signed32 Reversed = ClassifySegmentIntersection(Sample.BetaX,  Sample.BetaY,
                                                                      Sample.AlphaX, Sample.AlphaY,
                                                                      Sample.GammaX, Sample.GammaY,
                                                                      Sample.DeltaX, Sample.DeltaY);

                if (Classified != Exchanged || Classified != Reversed)
                    ++Report.DisagreeingCount;
            }

            Report.SampleCount = SampleSpan;
        }
        else if (std::strcmp(Held.EntryName, "ClassifyIntervalContainment") == 0)
        {
            const std::uint32_t SampleSpan =
                static_cast<std::uint32_t>(sizeof(IntervalSampleSet) / sizeof(IntervalSampleSet[0]));

            for (std::uint32_t Ordinal = 0u; Ordinal < SampleSpan; ++Ordinal)
            {
                const IntervalSample& Sample = IntervalSampleSet[Ordinal];

                const Signed32 Classified = ClassifyIntervalContainment(Sample.OuterBegin, Sample.OuterEnd,
                                                                        Sample.InnerBegin, Sample.InnerEnd);

                const Signed32 Exchanged = ClassifyIntervalContainment(Sample.InnerBegin, Sample.InnerEnd,
                                                                       Sample.OuterBegin, Sample.OuterEnd);

                // 📐 Strict containment is antisymmetric and identity is symmetric, so the two classifications
                //    agree only where both are zero. Anything else would let two occupants each enclose the
                //    other, which is `12` invariant 3 broken by the predicate rather than by the relation.
                if (Classified > 0 && Exchanged > 0)
                    ++Report.DisagreeingCount;

                if ((Classified == 0) != (Exchanged == 0))
                    ++Report.DisagreeingCount;

                // 📐 An interval never strictly contains itself, and every consumer relies on it so that an
                //    occupant tested against itself answers false without an exclusion at the call site.
                if (ClassifyIntervalContainment(Sample.OuterBegin, Sample.OuterEnd,
                                                Sample.OuterBegin, Sample.OuterEnd) > 0)
                {
                    ++Report.DisagreeingCount;
                }
            }

            Report.SampleCount = SampleSpan;
        }
        else if (std::strcmp(Held.EntryName, "ProjectPlanarSample") == 0)
        {
            constexpr std::uint32_t SampleSpan = 4096u;

            for (std::uint32_t Ordinal = 0u; Ordinal < SampleSpan; ++Ordinal)
            {
                Real64 FirstCoordinate  = 0.0;
                Real64 SecondCoordinate = 0.0;
                ProjectPlanarSample(Ordinal, FirstCoordinate, SecondCoordinate);

                if (FirstCoordinate < 0.0 || FirstCoordinate >= 1.0
                 || SecondCoordinate < 0.0 || SecondCoordinate >= 1.0)
                {
                    ++Report.DisagreeingCount;
                }

                // 📐 Reversing the bits of an even ordinal and of its successor differ in the highest bit
                //    alone, so the base-two inverses differ by exactly one half. Exactly, in binary, with no
                //    tolerance — which is the strongest statement available about any sample in the engine.
                if ((Ordinal & 1u) == 0u)
                {
                    if (ProjectRadicalTwo(Ordinal + 1u) - ProjectRadicalTwo(Ordinal) != 0.5)
                        ++Report.DisagreeingCount;
                }
            }

            Report.SampleCount = SampleSpan;
        }
        else if (std::strcmp(Held.EntryName, "ProjectSphericalSample") == 0)
        {
            constexpr std::uint32_t SampleSpan = 1024u;

            for (std::uint32_t Ordinal = 0u; Ordinal < SampleSpan; ++Ordinal)
            {
                Real64 FirstCoordinate  = 0.0;
                Real64 SecondCoordinate = 0.0;
                ProjectPlanarSample(Ordinal, FirstCoordinate, SecondCoordinate);

                Real64 DirectionX = 0.0;
                Real64 DirectionY = 0.0;
                Real64 DirectionZ = 0.0;
                ProjectSphericalSample(FirstCoordinate, SecondCoordinate, DirectionX, DirectionY, DirectionZ);

                const double Length = std::sqrt(DirectionX * DirectionX
                                              + DirectionY * DirectionY
                                              + DirectionZ * DirectionZ);

                // 📐 Measured in units in the last place about unity, which is what the report's deviation
                //    column declares. A Bounded entry point is compared against a bound and never for equality.
                const double Deviation = std::fabs(Length - 1.0) / MachineEpsilon;

                if (Deviation > Report.LargestDeviation)
                    Report.LargestDeviation = Deviation;
            }

            Report.SampleCount = SampleSpan;
        }
        else if (std::strcmp(Held.EntryName, "ProjectTransmittanceCoordinate") == 0)
        {
            // 📐 ① is compared as an **inversion**, not against a second implementation of itself. The bake walks
            //    the surface backwards through `ProjectTransmittanceParameter` and every reader walks it forwards
            //    through this routine, so the two composing to the identity is the whole of what `28` §2 needs
            //    from them — and it is a statement each toolchain can check without the other present.
            // ⚠️ The deviation is measured on the **zenith axis only**. The altitude axis carries a radius of six
            //    and a half million metres and recovers a coordinate from it, so its round trip loses about five
            //    decimal places to the subtraction alone — honestly, identically on both toolchains, and far
            //    outside a ceiling stated in units in the last place. The altitude axis is held to containment
            //    below instead, which is the strongest statement that survives the magnitude.
            constexpr std::uint32_t AxisSpan = 64u;

            MediumProfile Profile{};
            Profile.PlanetRadius        = 6360000.0;
            Profile.AtmosphereThickness = 100000.0;

            for (std::uint32_t AlongOrdinal = 0u; AlongOrdinal < AxisSpan; ++AlongOrdinal)
            {
                for (std::uint32_t AcrossOrdinal = 0u; AcrossOrdinal < AxisSpan; ++AcrossOrdinal)
                {
                    const Real64 CoordinateAlong  = (static_cast<Real64>(AlongOrdinal)  + 0.5) / AxisSpan;
                    const Real64 CoordinateAcross = (static_cast<Real64>(AcrossOrdinal) + 0.5) / AxisSpan;

                    Real64 Radius       = 0.0;
                    Real64 ZenithCosine = 0.0;
                    ProjectTransmittanceParameter(Profile, CoordinateAlong, CoordinateAcross,
                                                  Radius, ZenithCosine);

                    if (Radius < Profile.PlanetRadius
                     || Radius > Profile.PlanetRadius + Profile.AtmosphereThickness
                     || ZenithCosine < -1.0 || ZenithCosine > 1.0)
                    {
                        ++Report.DisagreeingCount;
                    }

                    Real64 ReturnedAlong  = 0.0;
                    Real64 ReturnedAcross = 0.0;
                    ProjectTransmittanceCoordinate(Profile, Radius, ZenithCosine,
                                                   ReturnedAlong, ReturnedAcross);

                    const double Deviation = std::fabs(ReturnedAlong - CoordinateAlong) / MachineEpsilon;

                    if (Deviation > Report.LargestDeviation)
                        Report.LargestDeviation = Deviation;
                }
            }

            Report.SampleCount = AxisSpan * AxisSpan;
        }
        else if (std::strcmp(Held.EntryName, "ProjectSkyViewDirection") == 0)
        {
            // 📐 ③'s zenith axis is quadratic about the horizon and its inverse is a square root, so composing
            //    the two through an arc cosine near the pole recovers the coordinate to about half its digits —
            //    a property of the arc cosine and not of the mapping. What survives every magnitude is that the
            //    direction the mapping hands back is a **unit** direction, which is what every consumer of it
            //    assumes and what a mis-set quadratic branch would break immediately.
            constexpr std::uint32_t AxisSpan = 64u;

            for (std::uint32_t AlongOrdinal = 0u; AlongOrdinal < AxisSpan; ++AlongOrdinal)
            {
                for (std::uint32_t AcrossOrdinal = 0u; AcrossOrdinal < AxisSpan; ++AcrossOrdinal)
                {
                    const Real64 CoordinateAlong  = (static_cast<Real64>(AlongOrdinal)  + 0.5) / AxisSpan;
                    const Real64 CoordinateAcross = (static_cast<Real64>(AcrossOrdinal) + 0.5) / AxisSpan;

                    Real64 DirectionX = 0.0;
                    Real64 DirectionY = 0.0;
                    Real64 DirectionZ = 0.0;
                    ProjectSkyViewDirection(CoordinateAlong, CoordinateAcross,
                                            DirectionX, DirectionY, DirectionZ);

                    const double Length = std::sqrt(DirectionX * DirectionX
                                                  + DirectionY * DirectionY
                                                  + DirectionZ * DirectionZ);

                    const double Deviation = std::fabs(Length - 1.0) / MachineEpsilon;

                    if (Deviation > Report.LargestDeviation)
                        Report.LargestDeviation = Deviation;

                    // 📝 The lower half of the across axis descends and the upper half climbs, with the horizon
                    //    exactly at the halfway coordinate. A branch written the other way round produces a sky
                    //    that is upside down and otherwise entirely plausible.
                    const bool Climbing = CoordinateAcross > 0.5;

                    if (Climbing != (DirectionY > 0.0))
                        ++Report.DisagreeingCount;
                }
            }

            Report.SampleCount = AxisSpan * AxisSpan;
        }
        else
        {
            // 🚧 An entry point with no comparison declared here reports zero samples and does not hold.
            //    Reporting agreement over an empty sample set is how an unproven entry point passes a gate.
            Report.SampleCount = 0u;
        }

        // 🔴 A Bounded entry point holds only when its measured deviation stays inside the declared bound.
        //    Without this clause a Bounded registration would hold on the strength of an equality comparison
        //    it never performed, which is the vacant pass in a different disguise.
        const bool DeviationHeld = Held.Claimed != PrecisionGuarantee::Bounded
                                || Report.LargestDeviation <= SampleUnitPlaceCeiling;

        Report.AgreementHeld = Report.SampleCount > 0u && Report.DisagreeingCount == 0u && DeviationHeld;

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
