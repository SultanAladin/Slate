//============================================================================================================================================
//                                                              TYRESOLVER.CPP
//============================================================================================================================================
// 🧩 Lattice construction, cord colouring, and the host form of the frame in the device's own order.

#include "SlateCompute/Compute/TyreSolver/Api/TyreSolver.h"

#include "Foundation/NumericTolerance.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE SPECIFICATION
//------------------------------------------------------------------------------------------------------------------------

Deliver<bool> TyreSpecification::Validate() const
{
    if (RimRadius <= 0.0 || OuterRadius <= RimRadius)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "the tyre has no section above the rim" });

    if (SectionWidth <= 0.0 || BeadWidth <= 0.0 || BeadWidth >= SectionWidth)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "the beads must sit inside the section width" });

    if (CrownFlattening <= 0.0 || CrownFlattening > 1.0)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "the crown flattening lies in (0, 1]" });

    if (CarcassMass <= 0.0 || MassScale <= 0.0 || InflationPressure <= 0.0 || AmbientPressure <= 0.0 || AdiabaticIndex <= 1.0)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "the carcass has no mass or no gas" });

    if (BeltStiffness <= 0.0 || SidewallHoopStiffness <= 0.0 || PlyStiffness <= 0.0 ||
        ShearStiffness <= 0.0 || BendStiffness <= 0.0)
    {
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "a cord of zero stiffness is no cord" });
    }

    if (StaticFriction < 0.0 || KineticFriction < 0.0 || KineticFriction > StaticFriction)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "kinetic friction may not exceed static" });

    if (DeformationDamping < 0.0 || TreadThickness < 0.0)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "damping and tread thickness are non-negative" });

    if (Rings < 4u || Segments < 8u || (Segments % 2u) != 0u)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "the lattice needs four rings and an even segment count" });

    return Deliver<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE LATTICE
//------------------------------------------------------------------------------------------------------------------------

std::uint32_t TyreLattice::ParticleCount() const
{
    return static_cast<std::uint32_t>(Particles.size());
}

std::uint32_t TyreLattice::ShapeOrder() const
{
    return 1u + Rings + Segments;
}

std::uint32_t TyreLattice::ParticlesPerTyre() const
{
    return Rings * Segments;
}

namespace
{

/// 🧩 The rim-frame rest position of one lattice sample.
void ResolveRestPosition(const TyreSpecification& Declared, std::uint32_t Ring, std::uint32_t Segment, double Winding,
                         double& LocalX, double& LocalY, double& LocalZ)
{
    const double HalfWidth   = 0.5 * Declared.SectionWidth;
    const double BeadCosine  = Declared.BeadWidth / Declared.SectionWidth;
    const double BeadAngle   = std::acos(BeadCosine);
    const double Fraction    = static_cast<double>(Ring) / static_cast<double>(Declared.Rings - 1u);
    const double Angle       = BeadAngle + (Pi - 2.0 * BeadAngle) * Fraction;
    const double BeadSine    = std::sin(BeadAngle);
    const double Raised      = (std::sin(Angle) - BeadSine) / (1.0 - BeadSine);
    const double Section     = Raised > 0.0 ? std::pow(Raised, Declared.CrownFlattening) : 0.0;
    const double Radius      = Declared.RimRadius + (Declared.OuterRadius - Declared.RimRadius) * Section;
    const double Azimuth     = Winding * 2.0 * Pi * static_cast<double>(Segment) / static_cast<double>(Declared.Segments);

    LocalX = Radius * std::cos(Azimuth);
    LocalY = Radius * std::sin(Azimuth);
    LocalZ = HalfWidth * std::cos(Angle);
}

/// 🧩 The signed carcass volume of one tyre about its rim centre, through the shared triangle sweep.
double SweptVolume(const TyreLattice& Lattice, std::uint32_t Tyre, const TyreHubRecord& Hub)
{
    double Total = 0.0;

    for (std::uint32_t Ring = 0u; Ring + 1u < Lattice.Rings; ++Ring)
    {
        for (std::uint32_t Segment = 0u; Segment < Lattice.Segments; ++Segment)
        {
            const TyreParticleRecord& Own = Lattice.Particles[LatticeIndex(Tyre, static_cast<Signed32>(Ring),
                                                                           static_cast<Signed32>(Segment),
                                                                           Lattice.Rings, Lattice.Segments)];

            for (Unsigned32 Triangle = 0u; Triangle < 2u; ++Triangle)
            {
                Signed32 RingB = 0, SegmentB = 0, RingC = 0, SegmentC = 0;
                ResolveIncidentCorner(Triangle, RingB, SegmentB, RingC, SegmentC);

                const TyreParticleRecord& B = Lattice.Particles[LatticeIndex(Tyre, static_cast<Signed32>(Ring) + RingB,
                                                                             static_cast<Signed32>(Segment) + SegmentB,
                                                                             Lattice.Rings, Lattice.Segments)];
                const TyreParticleRecord& C = Lattice.Particles[LatticeIndex(Tyre, static_cast<Signed32>(Ring) + RingC,
                                                                             static_cast<Signed32>(Segment) + SegmentC,
                                                                             Lattice.Rings, Lattice.Segments)];

                Total += TriangleSweptVolume(Own.X - Hub.EndX, Own.Y - Hub.EndY, Own.Z - Hub.EndZ,
                                             B.X - Hub.EndX, B.Y - Hub.EndY, B.Z - Hub.EndZ,
                                             C.X - Hub.EndX, C.Y - Hub.EndY, C.Z - Hub.EndZ);
            }
        }
    }

    return Total;
}

double Distance(const TyreParticleRecord& A, const TyreParticleRecord& B)
{
    const double DX = static_cast<double>(A.X) - static_cast<double>(B.X);
    const double DY = static_cast<double>(A.Y) - static_cast<double>(B.Y);
    const double DZ = static_cast<double>(A.Z) - static_cast<double>(B.Z);

    return std::sqrt(DX * DX + DY * DY + DZ * DZ);
}

/// 🧩 Which family a cord belongs to, and the parity that colours it within the family.
enum class CordFamily : std::uint32_t
{
    Circumferential = 0u,   // [-] - (k,s)–(k,s+1)
    Meridional      = 1u,   // [-] - (k,s)–(k+1,s)
    Diagonal        = 2u,   // [-] - (k,s)–(k+1,s+1)
    Antidiagonal    = 3u,   // [-] - (k,s+1)–(k+1,s)
    Bending         = 4u,   // [-] - (k,s)–(k+2,s)
    FamilyCount     = 5u
};

/// 🧩 Appends every cord of one family and parity across every tyre, delivering the colour it fills.
CordColour AppendCordColour(TyreLattice& Lattice, const TyreSpecification& Declared, CordFamily Family, std::uint32_t Parity)
{
    CordColour Colour;
    Colour.Offset = static_cast<std::uint32_t>(Lattice.Cords.size());

    const std::uint32_t Rings    = Lattice.Rings;
    const std::uint32_t Segments = Lattice.Segments;

    for (std::uint32_t Tyre = 0u; Tyre < Lattice.TyreCount; ++Tyre)
    {
        for (std::uint32_t Ring = 0u; Ring < Rings; ++Ring)
        {
            for (std::uint32_t Segment = 0u; Segment < Segments; ++Segment)
            {
                Signed32 RingB    = static_cast<Signed32>(Ring);
                Signed32 SegmentA = static_cast<Signed32>(Segment);
                Signed32 SegmentB = static_cast<Signed32>(Segment);
                double   Stiffness = 0.0;
                bool     Coloured  = false;

                switch (Family)
                {
                    case CordFamily::Circumferential:
                        SegmentB += 1;
                        Coloured  = (Segment % 2u) == Parity;
                        break;

                    case CordFamily::Meridional:
                        RingB    += 1;
                        Coloured  = (Ring % 2u) == Parity;
                        break;

                    case CordFamily::Diagonal:
                        RingB    += 1;
                        SegmentB += 1;
                        Coloured  = (Ring % 2u) == Parity;
                        break;

                    case CordFamily::Antidiagonal:
                        SegmentA += 1;
                        RingB    += 1;
                        Coloured  = (Ring % 2u) == Parity;
                        break;

                    case CordFamily::Bending:
                        RingB    += 2;
                        Coloured  = ((Ring / 2u) % 2u) == Parity;
                        break;

                    default:
                        break;
                }

                if (!Coloured || RingB >= static_cast<Signed32>(Rings))
                    continue;

                const std::uint32_t IndexA = LatticeIndex(Tyre, static_cast<Signed32>(Ring), SegmentA, Rings, Segments);
                const std::uint32_t IndexB = LatticeIndex(Tyre, RingB, SegmentB, Rings, Segments);

                const TyreParticleRecord& A = Lattice.Particles[IndexA];
                const TyreParticleRecord& B = Lattice.Particles[IndexB];

                // 📝 A cord between two beads constrains nothing the rim does not already fix.
                if (A.InverseMass <= 0.0f && B.InverseMass <= 0.0f)
                    continue;

                // 📐 EA per unit width times the lattice spacing across the cord, over the rest length. The
                //    circumferential spacing is the arc at this ring; the meridional spacing is the profile step.
                const std::uint32_t RingAbove = Ring + 1u < Rings ? Ring + 1u : Ring - 1u;
                const double Circumferential  = Distance(A, Lattice.Particles[LatticeIndex(Tyre, static_cast<Signed32>(Ring),
                                                                                            static_cast<Signed32>(Segment) + 1,
                                                                                            Rings, Segments)]);
                const double Meridional       = Distance(A, Lattice.Particles[LatticeIndex(Tyre, static_cast<Signed32>(RingAbove),
                                                                                            static_cast<Signed32>(Segment),
                                                                                            Rings, Segments)]);
                const double RestLength       = Distance(A, B);

                // 📝 A crown ring is one standing in the outer fifteen percent of the section height — the belt
                //    package — and its hoop cords carry the belt stiffness; every other ring's hoop is sidewall rubber.
                const double Height    = std::sqrt(static_cast<double>(A.X) * A.X + static_cast<double>(A.Y) * A.Y);
                const bool   CrownRing = Height >= Declared.RimRadius + 0.85 * (Declared.OuterRadius - Declared.RimRadius);

                switch (Family)
                {
                    case CordFamily::Circumferential:
                        Stiffness = (CrownRing ? Declared.BeltStiffness : Declared.SidewallHoopStiffness) * Meridional / RestLength;
                        break;

                    case CordFamily::Meridional:
                        Stiffness = Declared.PlyStiffness * Circumferential / RestLength;
                        break;

                    case CordFamily::Diagonal:
                    case CordFamily::Antidiagonal:
                        Stiffness = Declared.ShearStiffness * std::sqrt(Circumferential * Meridional) / RestLength;
                        break;

                    case CordFamily::Bending:
                        Stiffness = Declared.BendStiffness * Circumferential / RestLength;
                        break;

                    default:
                        break;
                }

                if (RestLength <= 0.0 || Stiffness <= 0.0)
                    continue;

                TyreCordRecord Cord;
                Cord.ParticleA  = IndexA;
                Cord.ParticleB  = IndexB;
                Cord.RestLength = static_cast<Real32>(RestLength);
                Cord.Compliance = static_cast<Real32>(1.0 / Stiffness);
                Lattice.Cords.push_back(Cord);

                if (Lattice.Spacing <= 0.0 || RestLength < Lattice.Spacing)
                    Lattice.Spacing = RestLength;
            }
        }
    }

    Colour.Count = static_cast<std::uint32_t>(Lattice.Cords.size()) - Colour.Offset;
    return Colour;
}

/// 🧩 Finds the pre-tension the inflated rest shape carries and writes it into the rest lengths; defined with the host form.
void PrestressLattice(TyreLattice& Lattice);

}   // namespace

Deliver<TyreLattice> ConstructTyreLattice(const TyreSpecification& Declared, std::uint32_t TyreCount)
{
    const Deliver<bool> Validated = Declared.Validate();

    if (!Validated.Resolved)
        return Deliver<TyreLattice>::Refuse(Validated.Error);

    if (TyreCount == 0u)
        return Deliver<TyreLattice>::Refuse({ RefusalReason::ContentUnsupported, "a lattice of no tyres" });

    TyreLattice Lattice;
    Lattice.Rings     = Declared.Rings;
    Lattice.Segments  = Declared.Segments;
    Lattice.TyreCount = TyreCount;

    const std::uint32_t PerTyre   = Declared.Rings * Declared.Segments;
    const std::uint32_t Total     = PerTyre * TyreCount;
    const std::uint32_t FreeCount = (Declared.Rings - 2u) * Declared.Segments;
    const Real32        Inverse   = static_cast<Real32>(static_cast<double>(FreeCount) / (Declared.CarcassMass * Declared.MassScale));

    Lattice.Particles.resize(Total);
    Lattice.Motions.resize(Total);
    Lattice.Velocities.resize(Total);
    Lattice.Rests.resize(Total);
    Lattice.Gradients.resize(Total);
    Lattice.Hubs.resize(TyreCount);
    Lattice.Accumulators.assign(static_cast<std::size_t>(TyreCount) * TyreAccumulatorStride, 0);
    Lattice.ShapeRows.assign(static_cast<std::size_t>(TyreCount) * ShapeRowLimit, TyreShapeRecord{});
    Lattice.ShapeSums.assign(static_cast<std::size_t>(TyreCount) * ShapeSumStride, 0);
    Lattice.ShapeMultipliers.assign(static_cast<std::size_t>(TyreCount) * ShapeRowLimit, 0.0f);
    Lattice.ShapeAdvances.assign(static_cast<std::size_t>(TyreCount) * ShapeRowLimit, 0.0f);
    Lattice.ShapeMatrix.assign(static_cast<std::size_t>(TyreCount) * ShapeMatrixStride, 0.0f);

    if (Lattice.ShapeOrder() > ShapeRowLimit)
        return Deliver<TyreLattice>::Refuse({ RefusalReason::ExtentExhausted, "rings plus segments exceed the shape block's order" });

    // 📝 Two windings are tried and the one sweeping a positive volume is kept, so the inflation gradient points
    //    outward whatever hand the profile was written in. Guessing the hand and asserting it would be the same
    //    code with a worse failure.
    for (std::uint32_t Attempt = 0u; Attempt < 2u; ++Attempt)
    {
        const double Winding = Attempt == 0u ? 1.0 : -1.0;

        for (std::uint32_t Tyre = 0u; Tyre < TyreCount; ++Tyre)
        {
            for (std::uint32_t Ring = 0u; Ring < Declared.Rings; ++Ring)
            {
                for (std::uint32_t Segment = 0u; Segment < Declared.Segments; ++Segment)
                {
                    const std::uint32_t Index = LatticeIndex(Tyre, static_cast<Signed32>(Ring), static_cast<Signed32>(Segment),
                                                             Declared.Rings, Declared.Segments);

                    double LocalX = 0.0, LocalY = 0.0, LocalZ = 0.0;
                    ResolveRestPosition(Declared, Ring, Segment, Winding, LocalX, LocalY, LocalZ);

                    const bool Bead = Ring == 0u || Ring + 1u == Declared.Rings;

                    TyreParticleRecord& Particle = Lattice.Particles[Index];
                    Particle.X           = static_cast<Real32>(LocalX);
                    Particle.Y           = static_cast<Real32>(LocalY);
                    Particle.Z           = static_cast<Real32>(LocalZ);
                    Particle.InverseMass = Bead ? 0.0f : Inverse;

                    TyreRestRecord& Rest = Lattice.Rests[Index];
                    Rest.LocalX    = Particle.X;
                    Rest.LocalY    = Particle.Y;
                    Rest.LocalZ    = Particle.Z;
                    Rest.TyreIndex = static_cast<Real32>(Tyre);

                    Lattice.Motions[Index]    = TyreMotionRecord{ Particle.X, Particle.Y, Particle.Z, 0.0f };
                    Lattice.Velocities[Index] = TyreVelocityRecord{ 0.0f, 0.0f, 0.0f, 0.0f };
                    Lattice.Gradients[Index]  = TyreGradientRecord{};
                }
            }
        }

        TyreHubRecord Origin = {};
        Origin.BeginRotationW = 1.0f;
        Origin.EndRotationW   = 1.0f;

        Lattice.RestVolume = SweptVolume(Lattice, 0u, Origin);

        if (Lattice.RestVolume > 0.0)
            break;
    }

    if (Lattice.RestVolume <= 0.0)
        return Deliver<TyreLattice>::Refuse({ RefusalReason::ContentUnsupported, "the carcass sweeps no volume" });

    for (std::uint32_t Tyre = 0u; Tyre < TyreCount; ++Tyre)
    {
        TyreHubRecord& Hub = Lattice.Hubs[Tyre];
        Hub                     = {};
        Hub.BeginRotationW      = 1.0f;
        Hub.EndRotationW        = 1.0f;
        Hub.RestVolume          = static_cast<Real32>(Lattice.RestVolume);
        Hub.GroundHeight        = -1.0e6f;
        Hub.StaticFriction      = static_cast<Real32>(Declared.StaticFriction);
        Hub.KineticFriction     = static_cast<Real32>(Declared.KineticFriction);
        Hub.DeformationDamping  = static_cast<Real32>(Declared.DeformationDamping);
        Hub.GravityY            = -9.81f;
        Hub.ContactOffset       = static_cast<Real32>(Declared.TreadThickness);
        Hub.AdiabaticIndex      = static_cast<Real32>(Declared.AdiabaticIndex);
        Hub.AmbientPressure     = static_cast<Real32>(Declared.AmbientPressure);

        // 📐 Each ring row's compliance is its hoop cords' in series — Σ L/(EA) around the ring — and each
        //    meridian row's is its ply cords' in series from bead to bead. The rows therefore carry exactly the
        //    stiffness the cords declare for the uniform mode, and the local cords add theirs on top of it.
        TyreShapeRecord* Rows = &Lattice.ShapeRows[static_cast<std::size_t>(Tyre) * ShapeRowLimit];

        for (std::uint32_t Ring = 0u; Ring < Declared.Rings; ++Ring)
        {
            const std::uint32_t RingAbove = Ring + 1u < Declared.Rings ? Ring + 1u : Ring - 1u;
            double Circumference = 0.0, Compliance = 0.0;

            for (std::uint32_t Segment = 0u; Segment < Declared.Segments; ++Segment)
            {
                const TyreParticleRecord& A    = Lattice.Particles[LatticeIndex(Tyre, Ring, Segment, Declared.Rings, Declared.Segments)];
                const TyreParticleRecord& Next = Lattice.Particles[LatticeIndex(Tyre, Ring, Segment + 1, Declared.Rings, Declared.Segments)];
                const TyreParticleRecord& Up   = Lattice.Particles[LatticeIndex(Tyre, RingAbove, Segment, Declared.Rings, Declared.Segments)];

                const double Height    = std::sqrt(static_cast<double>(A.X) * A.X + static_cast<double>(A.Y) * A.Y);
                const bool   CrownRing = Height >= Declared.RimRadius + 0.85 * (Declared.OuterRadius - Declared.RimRadius);
                const double Edge      = Distance(A, Next);

                Circumference += Edge;
                Compliance    += Edge / ((CrownRing ? Declared.BeltStiffness : Declared.SidewallHoopStiffness) * Distance(A, Up));
            }

            Rows[1u + Ring].RestValue  = static_cast<Real32>(Circumference);
            Rows[1u + Ring].Compliance = static_cast<Real32>(Compliance);
        }

        for (std::uint32_t Segment = 0u; Segment < Declared.Segments; ++Segment)
        {
            double Length = 0.0, Compliance = 0.0;

            for (std::uint32_t Ring = 0u; Ring + 1u < Declared.Rings; ++Ring)
            {
                const TyreParticleRecord& A    = Lattice.Particles[LatticeIndex(Tyre, Ring, Segment, Declared.Rings, Declared.Segments)];
                const TyreParticleRecord& Up   = Lattice.Particles[LatticeIndex(Tyre, Ring + 1u, Segment, Declared.Rings, Declared.Segments)];
                const TyreParticleRecord& Next = Lattice.Particles[LatticeIndex(Tyre, Ring, Segment + 1, Declared.Rings, Declared.Segments)];
                const double              Edge = Distance(A, Up);

                Length     += Edge;
                Compliance += Edge / (Declared.PlyStiffness * Distance(A, Next));
            }

            Rows[1u + Declared.Rings + Segment].RestValue  = static_cast<Real32>(Length);
            Rows[1u + Declared.Rings + Segment].Compliance = static_cast<Real32>(Compliance);
        }
    }

    // 📝 Colour order is family-major so that the stiffest families are swept first within a substep; the
    //    bending family, the softest, is swept last and corrects what the others left.
    for (std::uint32_t Family = 0u; Family < static_cast<std::uint32_t>(CordFamily::FamilyCount); ++Family)
    {
        for (std::uint32_t Parity = 0u; Parity < 2u; ++Parity)
        {
            const CordColour Colour = AppendCordColour(Lattice, Declared, static_cast<CordFamily>(Family), Parity);

            if (Colour.Count > 0u)
                Lattice.Colours.push_back(Colour);
        }
    }

    Lattice.CordMultipliers.assign(Lattice.Cords.size(), 0.0f);

    for (std::uint32_t Tyre = 0u; Tyre < TyreCount; ++Tyre)
    {
        const Deliver<bool> Inflated = DeclareInflation(Lattice, Tyre, Declared.InflationPressure);

        if (!Inflated.Resolved)
            return Deliver<TyreLattice>::Refuse(Inflated.Error);
    }

    PrestressLattice(Lattice);

    return Deliver<TyreLattice>::Result(Lattice);
}

Deliver<bool> PlaceTyre(TyreLattice& Lattice, std::uint32_t TyreIndex, const HubPose& Pose)
{
    if (TyreIndex >= Lattice.TyreCount)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "no tyre stands at that index" });

    const std::uint32_t PerTyre = Lattice.Rings * Lattice.Segments;
    const std::uint32_t First   = TyreIndex * PerTyre;

    const Real32 CentreX = static_cast<Real32>(Pose.X), CentreY = static_cast<Real32>(Pose.Y), CentreZ = static_cast<Real32>(Pose.Z);
    const Real32 QX = static_cast<Real32>(Pose.RotationX), QY = static_cast<Real32>(Pose.RotationY);
    const Real32 QZ = static_cast<Real32>(Pose.RotationZ), QW = static_cast<Real32>(Pose.RotationW);

    for (std::uint32_t Particle = First; Particle < First + PerTyre; ++Particle)
    {
        const TyreRestRecord& Rest = Lattice.Rests[Particle];

        Real32 X = 0.0f, Y = 0.0f, Z = 0.0f;
        RotateByQuaternion(QX, QY, QZ, QW, Rest.LocalX, Rest.LocalY, Rest.LocalZ, X, Y, Z);

        TyreParticleRecord& Current = Lattice.Particles[Particle];
        Current.X = CentreX + X;
        Current.Y = CentreY + Y;
        Current.Z = CentreZ + Z;

        TyreMotionRecord& Motion = Lattice.Motions[Particle];
        Motion.PreviousX         = Current.X;
        Motion.PreviousY         = Current.Y;
        Motion.PreviousZ         = Current.Z;
        Motion.ContactMultiplier = 0.0f;

        Lattice.Velocities[Particle] = TyreVelocityRecord{};
    }

    HubPose Still = Pose;
    Still.LinearVelocityX = Still.LinearVelocityY = Still.LinearVelocityZ = 0.0;
    Still.AngularVelocityX = Still.AngularVelocityY = Still.AngularVelocityZ = 0.0;

    return DeclareHubMotion(Lattice, TyreIndex, Still, Still);
}

Deliver<bool> DeclareInflation(TyreLattice& Lattice, std::uint32_t TyreIndex, double GaugePressure)
{
    if (TyreIndex >= Lattice.TyreCount)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "no tyre stands at that index" });

    if (GaugePressure < 0.0)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "a gauge pressure below ambient" });

    TyreHubRecord& Hub = Lattice.Hubs[TyreIndex];

    // 📐 The gas law about the rest state: α = V₀ / (γ p_abs) is the compliance of the air's own stiffness, and
    //    the gauge pressure p₀ is the constraint force at V₀ — so the volume row's target is V₀ + α p₀, the
    //    volume the gas would reach unresisted, and the rings and meridians hold it back by exactly p₀.
    const double Absolute   = GaugePressure + Hub.AmbientPressure;
    const double Compliance = Lattice.RestVolume / (Hub.AdiabaticIndex * Absolute);

    Hub.GaugePressure       = static_cast<Real32>(GaugePressure);
    Hub.InflationCompliance = static_cast<Real32>(Compliance);

    TyreShapeRecord& Volume = Lattice.ShapeRows[static_cast<std::size_t>(TyreIndex) * ShapeRowLimit];
    Volume.RestValue  = static_cast<Real32>(Lattice.RestVolume + Compliance * GaugePressure);
    Volume.Compliance = static_cast<Real32>(Compliance);

    return Deliver<bool>::Result(true);
}

Deliver<bool> DeclareHubMotion(TyreLattice& Lattice, std::uint32_t TyreIndex, const HubPose& Begin, const HubPose& End)
{
    if (TyreIndex >= Lattice.TyreCount)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "no tyre stands at that index" });

    TyreHubRecord& Hub = Lattice.Hubs[TyreIndex];

    Hub.BeginX           = static_cast<Real32>(Begin.X);
    Hub.BeginY           = static_cast<Real32>(Begin.Y);
    Hub.BeginZ           = static_cast<Real32>(Begin.Z);
    Hub.BeginRotationX   = static_cast<Real32>(Begin.RotationX);
    Hub.BeginRotationY   = static_cast<Real32>(Begin.RotationY);
    Hub.BeginRotationZ   = static_cast<Real32>(Begin.RotationZ);
    Hub.BeginRotationW   = static_cast<Real32>(Begin.RotationW);
    Hub.EndX             = static_cast<Real32>(End.X);
    Hub.EndY             = static_cast<Real32>(End.Y);
    Hub.EndZ             = static_cast<Real32>(End.Z);
    Hub.EndRotationX     = static_cast<Real32>(End.RotationX);
    Hub.EndRotationY     = static_cast<Real32>(End.RotationY);
    Hub.EndRotationZ     = static_cast<Real32>(End.RotationZ);
    Hub.EndRotationW     = static_cast<Real32>(End.RotationW);
    Hub.LinearVelocityX  = static_cast<Real32>(End.LinearVelocityX);
    Hub.LinearVelocityY  = static_cast<Real32>(End.LinearVelocityY);
    Hub.LinearVelocityZ  = static_cast<Real32>(End.LinearVelocityZ);
    Hub.AngularVelocityX = static_cast<Real32>(End.AngularVelocityX);
    Hub.AngularVelocityY = static_cast<Real32>(End.AngularVelocityY);
    Hub.AngularVelocityZ = static_cast<Real32>(End.AngularVelocityZ);

    return Deliver<bool>::Result(true);
}

Deliver<bool> DeclareGround(TyreLattice& Lattice, std::uint32_t TyreIndex, double GroundHeight)
{
    if (TyreIndex >= Lattice.TyreCount)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "no tyre stands at that index" });

    Lattice.Hubs[TyreIndex].GroundHeight = static_cast<Real32>(GroundHeight);
    return Deliver<bool>::Result(true);
}

ContactLoad DecodeContactLoad(const Signed32* Accumulated)
{
    ContactLoad Load;

    if (Accumulated == nullptr)
        return Load;

    Load.ForceX       = DecodeFixed(Accumulated[AccumulatorForceX],  LoadFixedScale);
    Load.ForceY       = DecodeFixed(Accumulated[AccumulatorForceY],  LoadFixedScale);
    Load.ForceZ       = DecodeFixed(Accumulated[AccumulatorForceZ],  LoadFixedScale);
    Load.TorqueX      = DecodeFixed(Accumulated[AccumulatorTorqueX], LoadFixedScale);
    Load.TorqueY      = DecodeFixed(Accumulated[AccumulatorTorqueY], LoadFixedScale);
    Load.TorqueZ      = DecodeFixed(Accumulated[AccumulatorTorqueZ], LoadFixedScale);
    Load.ContactCount = static_cast<std::uint32_t>(Accumulated[AccumulatorContactCount] > 0 ? Accumulated[AccumulatorContactCount] : 0);
    Load.Penetration  = DecodeFixed(Accumulated[AccumulatorPenetration], LengthFixedScale);

    if (Load.ContactCount > 0u)
    {
        Load.PatchLength = DecodeFixed(Accumulated[AccumulatorPatchTrailing] - Accumulated[AccumulatorPatchLeading], LengthFixedScale);
        Load.PatchWidth  = DecodeFixed(Accumulated[AccumulatorPatchOuter] - Accumulated[AccumulatorPatchInner], LengthFixedScale);
    }

    return Load;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE HOST FORM
//------------------------------------------------------------------------------------------------------------------------

namespace
{

/// 🧩 The frame's push constants as the host form carries them between its own passes.
struct HostStep
{
    TyreStepPush  Push     = {};     // [-] - exactly what the device is pushed
    Real32        Fraction = 0.0f;   // [-] - of the frame this substep ends at
};

void ResolvePosition(const TyreLattice& Lattice, std::uint32_t Particle, std::uint32_t& Tyre, std::uint32_t& Ring, std::uint32_t& Segment)
{
    const std::uint32_t PerTyre = Lattice.ParticlesPerTyre();

    Tyre    = Particle / PerTyre;
    Ring    = (Particle - Tyre * PerTyre) / Lattice.Segments;
    Segment = Particle - Tyre * PerTyre - Ring * Lattice.Segments;
}

/// 🧩 ① — the device's `TyrePrediction` entry point, one invocation per particle.
void PredictAll(TyreLattice& Lattice, const HostStep& Step)
{
    for (std::uint32_t Particle = 0u; Particle < Lattice.ParticleCount(); ++Particle)
    {
        std::uint32_t Tyre = 0u, Ring = 0u, Segment = 0u;
        ResolvePosition(Lattice, Particle, Tyre, Ring, Segment);

        const TyreHubRecord&      Hub     = Lattice.Hubs[Tyre];
        const TyreParticleRecord  Current = Lattice.Particles[Particle];
        const TyreRestRecord&     Rest    = Lattice.Rests[Particle];

        if (Step.Push.SubstepIndex == 0u && Ring == 0u && Segment == 0u)
        {
            Signed32* Base = &Lattice.Accumulators[static_cast<std::size_t>(Tyre) * TyreAccumulatorStride];

            for (std::uint32_t Slot = 0u; Slot < TyreAccumulatorStride; ++Slot)
                Base[Slot] = 0;

            Base[AccumulatorPatchLeading]  =  2147483647;
            Base[AccumulatorPatchTrailing] = -2147483647;
            Base[AccumulatorPatchInner]    =  2147483647;
            Base[AccumulatorPatchOuter]    = -2147483647;
        }

        // 📝 The starting positions were written by the previous recovery; only the multiplier is reset here.
        Lattice.Motions[Particle].ContactMultiplier = 0.0f;

        TyreVelocityRecord& Velocity = Lattice.Velocities[Particle];
        Velocity.FrictionMultiplier  = 0.0f;

        TyreParticleRecord& Predicted = Lattice.Particles[Particle];

        if (Current.InverseMass <= 0.0f)
        {
            Real32 CentreX = 0.0f, CentreY = 0.0f, CentreZ = 0.0f;
            Real32 RotationX = 0.0f, RotationY = 0.0f, RotationZ = 0.0f, RotationW = 1.0f;
            InterpolateHubPose(Hub, Step.Fraction, CentreX, CentreY, CentreZ, RotationX, RotationY, RotationZ, RotationW);

            Real32 CarriedX = 0.0f, CarriedY = 0.0f, CarriedZ = 0.0f;
            RotateByQuaternion(RotationX, RotationY, RotationZ, RotationW, Rest.LocalX, Rest.LocalY, Rest.LocalZ,
                               CarriedX, CarriedY, CarriedZ);

            Predicted.X = CentreX + CarriedX;
            Predicted.Y = CentreY + CarriedY;
            Predicted.Z = CentreZ + CarriedZ;
        }
        else
        {
            PredictParticle(Current.X, Current.Y, Current.Z,
                            Velocity.VelocityX, Velocity.VelocityY, Velocity.VelocityZ,
                            Hub.GravityY, Step.Push.SubstepSeconds, Predicted.X, Predicted.Y, Predicted.Z);
        }
    }
}

/// 🧩 ② — the device's `TyreCordProjection`, one colour; delivers the largest correction applied.
double ProjectColour(TyreLattice& Lattice, const HostStep& Step, const CordColour& Colour)
{
    double Largest = 0.0;

    for (std::uint32_t Within = 0u; Within < Colour.Count; ++Within)
    {
        const std::uint32_t   CordIndex = Colour.Offset + Within;
        const TyreCordRecord& Cord      = Lattice.Cords[CordIndex];

        TyreParticleRecord& A = Lattice.Particles[Cord.ParticleA];
        TyreParticleRecord& B = Lattice.Particles[Cord.ParticleB];

        Real32 Multiplier = Step.Push.IterationIndex == 0u ? 0.0f : Lattice.CordMultipliers[CordIndex];

        Real32 AX = 0.0f, AY = 0.0f, AZ = 0.0f, BX = 0.0f, BY = 0.0f, BZ = 0.0f;
        ProjectCord(A.X, A.Y, A.Z, A.InverseMass, B.X, B.Y, B.Z, B.InverseMass,
                    Cord.RestLength, Cord.Compliance, Step.Push.SubstepSeconds, Multiplier,
                    AX, AY, AZ, BX, BY, BZ);

        Lattice.CordMultipliers[CordIndex] = Multiplier;

        if (A.InverseMass > 0.0f)
        {
            A.X += AX;
            A.Y += AY;
            A.Z += AZ;
        }

        if (B.InverseMass > 0.0f)
        {
            B.X += BX;
            B.Y += BY;
            B.Z += BZ;
        }

        const double Moved = std::sqrt(static_cast<double>(AX) * AX + static_cast<double>(AY) * AY + static_cast<double>(AZ) * AZ
                                     + static_cast<double>(BX) * BX + static_cast<double>(BY) * BY + static_cast<double>(BZ) * BZ);

        if (Moved > Largest)
            Largest = Moved;
    }

    return Largest;
}

/// 🧩 ③ — the device's `TyreShapeGather`, one invocation per particle: the three gradients, and the block's sums.
void GatherAll(TyreLattice& Lattice, const HostStep& Step)
{
    for (std::uint32_t Particle = 0u; Particle < Lattice.ParticleCount(); ++Particle)
    {
        std::uint32_t Tyre = 0u, Ring = 0u, Segment = 0u;
        ResolvePosition(Lattice, Particle, Tyre, Ring, Segment);

        const TyreHubRecord& Hub = Lattice.Hubs[Tyre];

        Real32 CentreX = 0.0f, CentreY = 0.0f, CentreZ = 0.0f;
        Real32 RotationX = 0.0f, RotationY = 0.0f, RotationZ = 0.0f, RotationW = 1.0f;
        InterpolateHubPose(Hub, Step.Fraction, CentreX, CentreY, CentreZ, RotationX, RotationY, RotationZ, RotationW);

        const TyreParticleRecord& Own = Lattice.Particles[Particle];

        const Real32 OwnX = Own.X - CentreX;
        const Real32 OwnY = Own.Y - CentreY;
        const Real32 OwnZ = Own.Z - CentreZ;

        TyreGradientRecord Gradient = {};

        for (std::uint32_t Triangle = 0u; Triangle < 6u; ++Triangle)
        {
            Signed32 RingB = 0, SegmentB = 0, RingC = 0, SegmentC = 0;
            ResolveIncidentCorner(Triangle, RingB, SegmentB, RingC, SegmentC);

            const Signed32 AbsoluteB = static_cast<Signed32>(Ring) + RingB;
            const Signed32 AbsoluteC = static_cast<Signed32>(Ring) + RingC;

            if (AbsoluteB < 0 || AbsoluteC < 0 || AbsoluteB >= static_cast<Signed32>(Lattice.Rings) ||
                AbsoluteC >= static_cast<Signed32>(Lattice.Rings))
            {
                continue;
            }

            const TyreParticleRecord& B = Lattice.Particles[LatticeIndex(Tyre, AbsoluteB, static_cast<Signed32>(Segment) + SegmentB,
                                                                         Lattice.Rings, Lattice.Segments)];
            const TyreParticleRecord& C = Lattice.Particles[LatticeIndex(Tyre, AbsoluteC, static_cast<Signed32>(Segment) + SegmentC,
                                                                         Lattice.Rings, Lattice.Segments)];

            const Real32 BX = B.X - CentreX, BY = B.Y - CentreY, BZ = B.Z - CentreZ;
            const Real32 CX = C.X - CentreX, CY = C.Y - CentreY, CZ = C.Z - CentreZ;

            TriangleVolumeGradient(BX, BY, BZ, CX, CY, CZ, Gradient.VolumeX, Gradient.VolumeY, Gradient.VolumeZ);

            if (Triangle < 2u)
                Gradient.OwnedVolume += TriangleSweptVolume(OwnX, OwnY, OwnZ, BX, BY, BZ, CX, CY, CZ);
        }

        const Signed32 R = static_cast<Signed32>(Ring);
        const Signed32 S = static_cast<Signed32>(Segment);

        // 📝 The ring row: both hoop neighbours; the edge to the next segment is the one this particle owns.
        {
            const TyreParticleRecord& Before = Lattice.Particles[LatticeIndex(Tyre, R, S - 1, Lattice.Rings, Lattice.Segments)];
            const TyreParticleRecord& After  = Lattice.Particles[LatticeIndex(Tyre, R, S + 1, Lattice.Rings, Lattice.Segments)];

            EdgeLengthGradient(Own.X, Own.Y, Own.Z, Before.X, Before.Y, Before.Z, Gradient.RingX, Gradient.RingY, Gradient.RingZ);
            Gradient.RingEdge = EdgeLengthGradient(Own.X, Own.Y, Own.Z, After.X, After.Y, After.Z,
                                                   Gradient.RingX, Gradient.RingY, Gradient.RingZ);
        }

        // 📝 The meridian row: the ring below and the ring above where they exist; the edge upward is owned.
        if (Ring > 0u)
        {
            const TyreParticleRecord& Below = Lattice.Particles[LatticeIndex(Tyre, R - 1, S, Lattice.Rings, Lattice.Segments)];
            EdgeLengthGradient(Own.X, Own.Y, Own.Z, Below.X, Below.Y, Below.Z, Gradient.MeridianX, Gradient.MeridianY, Gradient.MeridianZ);
        }

        if (Ring + 1u < Lattice.Rings)
        {
            const TyreParticleRecord& Above = Lattice.Particles[LatticeIndex(Tyre, R + 1, S, Lattice.Rings, Lattice.Segments)];
            Gradient.MeridianEdge = EdgeLengthGradient(Own.X, Own.Y, Own.Z, Above.X, Above.Y, Above.Z,
                                                       Gradient.MeridianX, Gradient.MeridianY, Gradient.MeridianZ);
        }

        Lattice.Gradients[Particle] = Gradient;

        // 📝 The block's sums. Row 0 is the volume, row 1 + Ring this ring, row 1 + Rings + Segment this meridian;
        //    an entry is stored once, at (lower row, higher column), and the solve mirrors it.
        const std::uint32_t RingRow     = 1u + Ring;
        const std::uint32_t MeridianRow = 1u + Lattice.Rings + Segment;
        const Real32        W           = Own.InverseMass;

        Signed32* Sums   = &Lattice.ShapeSums[static_cast<std::size_t>(Tyre) * ShapeSumStride];
        Signed32* Values = Sums + ShapeMatrixStride;

        const Real32 VX = Gradient.VolumeX,   VY = Gradient.VolumeY,   VZ = Gradient.VolumeZ;
        const Real32 RX = Gradient.RingX,     RY = Gradient.RingY,     RZ = Gradient.RingZ;
        const Real32 MX = Gradient.MeridianX, MY = Gradient.MeridianY, MZ = Gradient.MeridianZ;

        const Real32 DiagonalScale = ShapeEntryScale(RingRow, RingRow);
        const Real32 CrossScale    = ShapeEntryScale(0u, RingRow);

        Sums[0u * ShapeRowLimit + 0u]                   += EncodeFixed(WeightedDot(W, VX, VY, VZ, VX, VY, VZ), ShapeEntryScale(0u, 0u));
        Sums[RingRow * ShapeRowLimit + RingRow]         += EncodeFixed(WeightedDot(W, RX, RY, RZ, RX, RY, RZ), DiagonalScale);
        Sums[MeridianRow * ShapeRowLimit + MeridianRow] += EncodeFixed(WeightedDot(W, MX, MY, MZ, MX, MY, MZ), DiagonalScale);
        Sums[0u * ShapeRowLimit + RingRow]              += EncodeFixed(WeightedDot(W, VX, VY, VZ, RX, RY, RZ), CrossScale);
        Sums[0u * ShapeRowLimit + MeridianRow]          += EncodeFixed(WeightedDot(W, VX, VY, VZ, MX, MY, MZ), CrossScale);
        Sums[RingRow * ShapeRowLimit + MeridianRow]     += EncodeFixed(WeightedDot(W, RX, RY, RZ, MX, MY, MZ), DiagonalScale);

        Values[0u]          += EncodeFixed(Gradient.OwnedVolume,  ShapeValueScale(0u));
        Values[RingRow]     += EncodeFixed(Gradient.RingEdge,     ShapeValueScale(RingRow));
        Values[MeridianRow] += EncodeFixed(Gradient.MeridianEdge, ShapeValueScale(MeridianRow));
    }

}

/// 🧩 ③½ — the device's `TyreShapeSolve`, one workgroup per tyre: decode, factor, substitute, deliver Δλ, zero the sums.
void SolveShapeAll(TyreLattice& Lattice, const HostStep& Step)
{
    const std::uint32_t Order   = Lattice.ShapeOrder();
    const Real32        Substep = Step.Push.SubstepSeconds;

    for (std::uint32_t Tyre = 0u; Tyre < Lattice.TyreCount; ++Tyre)
    {
        Signed32*              Sums        = &Lattice.ShapeSums[static_cast<std::size_t>(Tyre) * ShapeSumStride];
        Signed32*              Values      = Sums + ShapeMatrixStride;
        Real32*                Matrix      = &Lattice.ShapeMatrix[static_cast<std::size_t>(Tyre) * ShapeMatrixStride];
        Real32*                Multipliers = &Lattice.ShapeMultipliers[static_cast<std::size_t>(Tyre) * ShapeRowLimit];
        Real32*                Advances    = &Lattice.ShapeAdvances[static_cast<std::size_t>(Tyre) * ShapeRowLimit];
        const TyreShapeRecord* Rows        = &Lattice.ShapeRows[static_cast<std::size_t>(Tyre) * ShapeRowLimit];

        // 📝 Decode into the lower triangle, mirrored from the upper where the gather stored it, α̃ on the diagonal;
        //    the right-hand side −C − α̃λ into the advances, which the substitution overwrites in place.
        for (std::uint32_t Row = 0u; Row < Order; ++Row)
        {
            if (Step.Push.IterationIndex == 0u)
                Multipliers[Row] = 0.0f;

            const Real32 ScaledCompliance = Rows[Row].Compliance / (Substep * Substep);

            for (std::uint32_t Column = 0u; Column <= Row; ++Column)
            {
                const Real32 Entry = DecodeFixed(Sums[Column * ShapeRowLimit + Row], ShapeEntryScale(Column, Row));
                Matrix[Row * ShapeRowLimit + Column] = Entry + (Row == Column ? ScaledCompliance : 0.0f);
            }

            const Real32 Value = DecodeFixed(Values[Row], ShapeValueScale(Row));
            Advances[Row]      = -(Value - Rows[Row].RestValue) - ScaledCompliance * Multipliers[Row];
        }

        // 📐 L Lᵀ = A, in place, column by column; a pivot that reads non-positive is a saturated sum and is clamped.
        for (std::uint32_t Column = 0u; Column < Order; ++Column)
        {
            Real32 Pivot = Matrix[Column * ShapeRowLimit + Column];

            for (std::uint32_t Earlier = 0u; Earlier < Column; ++Earlier)
                Pivot -= Matrix[Column * ShapeRowLimit + Earlier] * Matrix[Column * ShapeRowLimit + Earlier];

            Pivot = static_cast<Real32>(std::sqrt(static_cast<double>(Pivot > 1.0e-30f ? Pivot : 1.0e-30f)));
            Matrix[Column * ShapeRowLimit + Column] = Pivot;

            for (std::uint32_t Row = Column + 1u; Row < Order; ++Row)
            {
                Real32 Sum = Matrix[Row * ShapeRowLimit + Column];

                for (std::uint32_t Earlier = 0u; Earlier < Column; ++Earlier)
                    Sum -= Matrix[Row * ShapeRowLimit + Earlier] * Matrix[Column * ShapeRowLimit + Earlier];

                Matrix[Row * ShapeRowLimit + Column] = Sum / Pivot;
            }
        }

        // 📐 L y = b forward, Lᵀ Δλ = y backward.
        for (std::uint32_t Row = 0u; Row < Order; ++Row)
        {
            Real32 Sum = Advances[Row];

            for (std::uint32_t Earlier = 0u; Earlier < Row; ++Earlier)
                Sum -= Matrix[Row * ShapeRowLimit + Earlier] * Advances[Earlier];

            Advances[Row] = Sum / Matrix[Row * ShapeRowLimit + Row];
        }

        for (std::uint32_t Back = Order; Back > 0u; --Back)
        {
            const std::uint32_t Row = Back - 1u;
            Real32              Sum = Advances[Row];

            for (std::uint32_t Later = Row + 1u; Later < Order; ++Later)
                Sum -= Matrix[Later * ShapeRowLimit + Row] * Advances[Later];

            Advances[Row] = Sum / Matrix[Row * ShapeRowLimit + Row];
        }

        for (std::uint32_t Row = 0u; Row < Order; ++Row)
            Multipliers[Row] += Advances[Row];

        for (std::uint32_t Slot = 0u; Slot < ShapeSumStride; ++Slot)
            Sums[Slot] = 0;
    }
}

/// 🧩 ④ — the device's `TyreContactProjection`, one invocation per particle; delivers the largest correction.
double ProjectContactAll(TyreLattice& Lattice, const HostStep& Step)
{
    double Largest = 0.0;

    for (std::uint32_t Particle = 0u; Particle < Lattice.ParticleCount(); ++Particle)
    {
        std::uint32_t Tyre = 0u, Ring = 0u, Segment = 0u;
        ResolvePosition(Lattice, Particle, Tyre, Ring, Segment);

        TyreParticleRecord& Current = Lattice.Particles[Particle];

        if (Current.InverseMass <= 0.0f)
            continue;

        const TyreHubRecord&      Hub      = Lattice.Hubs[Tyre];
        const TyreGradientRecord& Gradient = Lattice.Gradients[Particle];
        const Real32*             Advances = &Lattice.ShapeAdvances[static_cast<std::size_t>(Tyre) * ShapeRowLimit];

        const Real32 VolumeAdvance   = Advances[0u];
        const Real32 RingAdvance     = Advances[1u + Ring];
        const Real32 MeridianAdvance = Advances[1u + Lattice.Rings + Segment];

        const Real32 InflatedX = Current.InverseMass * (VolumeAdvance * Gradient.VolumeX + RingAdvance * Gradient.RingX
                                                      + MeridianAdvance * Gradient.MeridianX);
        const Real32 InflatedY = Current.InverseMass * (VolumeAdvance * Gradient.VolumeY + RingAdvance * Gradient.RingY
                                                      + MeridianAdvance * Gradient.MeridianY);
        const Real32 InflatedZ = Current.InverseMass * (VolumeAdvance * Gradient.VolumeZ + RingAdvance * Gradient.RingZ
                                                      + MeridianAdvance * Gradient.MeridianZ);

        Current.X += InflatedX;
        Current.Y += InflatedY;
        Current.Z += InflatedZ;

        TyreMotionRecord&   Motion   = Lattice.Motions[Particle];
        TyreVelocityRecord& Velocity = Lattice.Velocities[Particle];

        const Real32 Penetration = Hub.GroundHeight + Hub.ContactOffset - Current.Y;

        Real32 CorrectionX = 0.0f, CorrectionY = 0.0f, CorrectionZ = 0.0f;
        Real32 NormalMultiplier = 0.0f, FrictionMultiplier = 0.0f;

        ProjectGroundContact(Current.X, Current.Y, Current.Z, Current.InverseMass, Motion.PreviousX, Motion.PreviousZ,
                             Hub.GroundHeight, Hub.ContactOffset, Hub.StaticFriction, Hub.KineticFriction,
                             CorrectionX, CorrectionY, CorrectionZ, NormalMultiplier, FrictionMultiplier);

        Current.X += CorrectionX;
        Current.Y += CorrectionY;
        Current.Z += CorrectionZ;

        const double Moved = std::sqrt(static_cast<double>(InflatedX + CorrectionX) * (InflatedX + CorrectionX)
                                     + static_cast<double>(InflatedY + CorrectionY) * (InflatedY + CorrectionY)
                                     + static_cast<double>(InflatedZ + CorrectionZ) * (InflatedZ + CorrectionZ));

        if (Moved > Largest)
            Largest = Moved;

        if (NormalMultiplier <= 0.0f)
            continue;

        Motion.ContactMultiplier    += NormalMultiplier;
        Velocity.FrictionMultiplier += FrictionMultiplier;

        const Real32 Share  = 1.0f / static_cast<Real32>(Step.Push.SubstepCount);
        const Real32 ForceX = ForceOfMultiplier(CorrectionX / Current.InverseMass, Step.Push.SubstepSeconds) * Share;
        const Real32 ForceY = ForceOfMultiplier(NormalMultiplier, Step.Push.SubstepSeconds) * Share;
        const Real32 ForceZ = ForceOfMultiplier(CorrectionZ / Current.InverseMass, Step.Push.SubstepSeconds) * Share;

        // 📝 The arm is to the rim centre **at this substep**, not at the frame's end; at ten metres a second the
        //    two differ by seventeen centimetres on the first substep, which is half the radius.
        Real32 CentreX = 0.0f, CentreY = 0.0f, CentreZ = 0.0f;
        Real32 RotationX = 0.0f, RotationY = 0.0f, RotationZ = 0.0f, RotationW = 1.0f;
        InterpolateHubPose(Hub, Step.Fraction, CentreX, CentreY, CentreZ, RotationX, RotationY, RotationZ, RotationW);

        const Real32 ArmX = Current.X - CentreX;
        const Real32 ArmY = Current.Y - CentreY;
        const Real32 ArmZ = Current.Z - CentreZ;

        Signed32* Base = &Lattice.Accumulators[static_cast<std::size_t>(Tyre) * TyreAccumulatorStride];

        Base[AccumulatorForceX]  += EncodeFixed(ForceX, LoadFixedScale);
        Base[AccumulatorForceY]  += EncodeFixed(ForceY, LoadFixedScale);
        Base[AccumulatorForceZ]  += EncodeFixed(ForceZ, LoadFixedScale);
        Base[AccumulatorTorqueX] += EncodeFixed(ArmY * ForceZ - ArmZ * ForceY, LoadFixedScale);
        Base[AccumulatorTorqueY] += EncodeFixed(ArmZ * ForceX - ArmX * ForceZ, LoadFixedScale);
        Base[AccumulatorTorqueZ] += EncodeFixed(ArmX * ForceY - ArmY * ForceX, LoadFixedScale);
        Base[AccumulatorContactCount] += 1;

        const Signed32 AlongX = EncodeFixed(Current.X, LengthFixedScale);
        const Signed32 AlongZ = EncodeFixed(Current.Z, LengthFixedScale);
        const Signed32 Depth  = EncodeFixed(Penetration, LengthFixedScale);

        if (AlongX < Base[AccumulatorPatchLeading])  Base[AccumulatorPatchLeading]  = AlongX;
        if (AlongX > Base[AccumulatorPatchTrailing]) Base[AccumulatorPatchTrailing] = AlongX;
        if (AlongZ < Base[AccumulatorPatchInner])    Base[AccumulatorPatchInner]    = AlongZ;
        if (AlongZ > Base[AccumulatorPatchOuter])    Base[AccumulatorPatchOuter]    = AlongZ;
        if (Depth  > Base[AccumulatorPenetration])   Base[AccumulatorPenetration]   = Depth;
    }

    return Largest;
}

/// 🧩 ⑤ — the device's `TyreVelocityRecovery`, one invocation per particle.
void RecoverAll(TyreLattice& Lattice, const HostStep& Step)
{
    for (std::uint32_t Particle = 0u; Particle < Lattice.ParticleCount(); ++Particle)
    {
        std::uint32_t Tyre = 0u, Ring = 0u, Segment = 0u;
        ResolvePosition(Lattice, Particle, Tyre, Ring, Segment);

        const TyreHubRecord&      Hub      = Lattice.Hubs[Tyre];
        const TyreParticleRecord& Current  = Lattice.Particles[Particle];
        TyreMotionRecord&         Motion   = Lattice.Motions[Particle];
        TyreVelocityRecord&       Velocity = Lattice.Velocities[Particle];

        Real32 RigidX = 0.0f, RigidY = 0.0f, RigidZ = 0.0f;
        RigidPointVelocity(Hub, Current.X, Current.Y, Current.Z, RigidX, RigidY, RigidZ);

        if (Current.InverseMass <= 0.0f)
        {
            Velocity.VelocityX = RigidX;
            Velocity.VelocityY = RigidY;
            Velocity.VelocityZ = RigidZ;
        }
        else
        {
            RecoverVelocity(Current.X, Current.Y, Current.Z, Motion.PreviousX, Motion.PreviousY, Motion.PreviousZ,
                            RigidX, RigidY, RigidZ, Hub.DeformationDamping, Step.Push.SubstepSeconds,
                            Velocity.VelocityX, Velocity.VelocityY, Velocity.VelocityZ);
        }

        Motion.PreviousX = Current.X;
        Motion.PreviousY = Current.Y;
        Motion.PreviousZ = Current.Z;
    }
}

// 📐 🔴 PRE-STRESS. The moulded shape the lattice is built in is the shape the tyre holds **inflated**, so at that
//    shape the cords, rings and meridians must already carry the tension that balances the pressure. XPBD
//    resets a multiplier every substep and recovers it from strain, so a cord that carries tension at rest must
//    have a natural length shorter than its rest length by exactly that tension times its compliance —
//    otherwise the carcass stretches until the strain supplies the tension, by sixty percent for a sidewall hoop
//    at two bar, and the tyre balloons. The balance is found here as a **linear** problem at the rest shape:
//
//        J M⁻¹ Jᵀ F = −J M⁻¹ f_p,       f_p = p₀ ∇V,      F the force each cord and each row carries,
//
//    swept Gauss–Seidel over the cords with the ring and meridian rows solved as one block per sweep, on a
//    virtual displacement r = M⁻¹ (f_p + Jᵀ F) that goes to zero as the balance is reached. It is the
//    projection's own arithmetic with the gradients frozen at the rest shape and the step taken to infinity,
//    which is the one setting where XPBD's inertial tether m/Δt² is exactly nothing. Natural lengths are then
//    L₀ + α F. The volume row is not in the static block: its force **is** p₀, and at run time its target
//    V₀ + α p₀ delivers exactly that at V₀.
void PrestressLattice(TyreLattice& Lattice)
{
    const std::uint32_t Order    = Lattice.ShapeOrder();
    const std::uint32_t Count    = Lattice.ParticleCount();
    const std::size_t   CordCount = Lattice.Cords.size();

    HostStep Step;
    Step.Push.ParticleCount  = Count;
    Step.Push.Rings          = Lattice.Rings;
    Step.Push.Segments       = Lattice.Segments;
    Step.Push.TyreCount      = Lattice.TyreCount;
    Step.Push.SubstepSeconds = 1.0f;
    Step.Push.SubstepCount   = 1u;
    Step.Fraction            = 1.0f;

    // 📝 The gradients and the block's matrix at the rest shape, once; the factor is kept for every sweep.
    GatherAll(Lattice, Step);

    std::vector<Real32> Factor(static_cast<std::size_t>(Lattice.TyreCount) * ShapeMatrixStride, 0.0f);

    for (std::uint32_t Tyre = 0u; Tyre < Lattice.TyreCount; ++Tyre)
    {
        Signed32* Sums   = &Lattice.ShapeSums[static_cast<std::size_t>(Tyre) * ShapeSumStride];
        Real32*   Matrix = &Factor[static_cast<std::size_t>(Tyre) * ShapeMatrixStride];

        for (std::uint32_t Row = 0u; Row < Order; ++Row)
        {
            for (std::uint32_t Column = 0u; Column <= Row; ++Column)
                Matrix[Row * ShapeRowLimit + Column] = DecodeFixed(Sums[Column * ShapeRowLimit + Row], ShapeEntryScale(Column, Row));

            // 📝 The volume row is decoupled, and a bead ring, which no free particle touches, is regularised.
            if (Row == 0u)
            {
                for (std::uint32_t Column = 0u; Column < Order; ++Column)
                    Matrix[Row * ShapeRowLimit + Column] = Column == Row ? 1.0f : 0.0f;
            }
            else
            {
                Matrix[Row * ShapeRowLimit + 0u] = 0.0f;
                Matrix[Row * ShapeRowLimit + Row] += 1.0e-6f;
            }
        }

        for (std::uint32_t Column = 0u; Column < Order; ++Column)
        {
            Real32 Pivot = Matrix[Column * ShapeRowLimit + Column];

            for (std::uint32_t Earlier = 0u; Earlier < Column; ++Earlier)
                Pivot -= Matrix[Column * ShapeRowLimit + Earlier] * Matrix[Column * ShapeRowLimit + Earlier];

            Pivot = static_cast<Real32>(std::sqrt(static_cast<double>(Pivot > 1.0e-30f ? Pivot : 1.0e-30f)));
            Matrix[Column * ShapeRowLimit + Column] = Pivot;

            for (std::uint32_t Row = Column + 1u; Row < Order; ++Row)
            {
                Real32 Sum = Matrix[Row * ShapeRowLimit + Column];

                for (std::uint32_t Earlier = 0u; Earlier < Column; ++Earlier)
                    Sum -= Matrix[Row * ShapeRowLimit + Earlier] * Matrix[Column * ShapeRowLimit + Earlier];

                Matrix[Row * ShapeRowLimit + Column] = Sum / Pivot;
            }
        }

        for (std::uint32_t Slot = 0u; Slot < ShapeSumStride; ++Slot)
            Sums[Slot] = 0;
    }

    // 📝 r starts as the pressure's own virtual displacement, w p₀ ∇V.
    std::vector<double> ResidualX(Count, 0.0), ResidualY(Count, 0.0), ResidualZ(Count, 0.0);
    std::vector<double> CordForce(CordCount, 0.0);
    std::vector<double> RowForce(static_cast<std::size_t>(Lattice.TyreCount) * ShapeRowLimit, 0.0);
    std::vector<double> RightHand(Order, 0.0);

    for (std::uint32_t Particle = 0u; Particle < Count; ++Particle)
    {
        std::uint32_t Tyre = 0u, Ring = 0u, Segment = 0u;
        ResolvePosition(Lattice, Particle, Tyre, Ring, Segment);

        const double Scale = static_cast<double>(Lattice.Particles[Particle].InverseMass) * Lattice.Hubs[Tyre].GaugePressure;

        ResidualX[Particle] = Scale * Lattice.Gradients[Particle].VolumeX;
        ResidualY[Particle] = Scale * Lattice.Gradients[Particle].VolumeY;
        ResidualZ[Particle] = Scale * Lattice.Gradients[Particle].VolumeZ;
    }

    std::vector<double> DirectionX(CordCount, 0.0), DirectionY(CordCount, 0.0), DirectionZ(CordCount, 0.0);

    for (std::size_t Cord = 0u; Cord < CordCount; ++Cord)
    {
        const TyreParticleRecord& A = Lattice.Particles[Lattice.Cords[Cord].ParticleA];
        const TyreParticleRecord& B = Lattice.Particles[Lattice.Cords[Cord].ParticleB];
        const double              L = Distance(A, B);

        DirectionX[Cord] = (static_cast<double>(A.X) - B.X) / L;
        DirectionY[Cord] = (static_cast<double>(A.Y) - B.Y) / L;
        DirectionZ[Cord] = (static_cast<double>(A.Z) - B.Z) / L;
    }

    double Largest = 0.0;

    for (std::uint32_t Sweep = 0u; Sweep < 4000u; ++Sweep)
    {
        Largest = 0.0;

        for (std::size_t Cord = 0u; Cord < CordCount; ++Cord)
        {
            const TyreCordRecord& Record = Lattice.Cords[Cord];
            const double          WA     = Lattice.Particles[Record.ParticleA].InverseMass;
            const double          WB     = Lattice.Particles[Record.ParticleB].InverseMass;
            const double          Sum    = WA + WB;

            if (Sum <= 0.0)
                continue;

            const double Along = DirectionX[Cord] * (ResidualX[Record.ParticleA] - ResidualX[Record.ParticleB])
                               + DirectionY[Cord] * (ResidualY[Record.ParticleA] - ResidualY[Record.ParticleB])
                               + DirectionZ[Cord] * (ResidualZ[Record.ParticleA] - ResidualZ[Record.ParticleB]);
            const double Advance = -Along / Sum;

            CordForce[Cord] += Advance;
            ResidualX[Record.ParticleA] += WA * Advance * DirectionX[Cord];
            ResidualY[Record.ParticleA] += WA * Advance * DirectionY[Cord];
            ResidualZ[Record.ParticleA] += WA * Advance * DirectionZ[Cord];
            ResidualX[Record.ParticleB] -= WB * Advance * DirectionX[Cord];
            ResidualY[Record.ParticleB] -= WB * Advance * DirectionY[Cord];
            ResidualZ[Record.ParticleB] -= WB * Advance * DirectionZ[Cord];
        }

        for (std::uint32_t Tyre = 0u; Tyre < Lattice.TyreCount; ++Tyre)
        {
            const Real32* Matrix = &Factor[static_cast<std::size_t>(Tyre) * ShapeMatrixStride];
            std::fill(RightHand.begin(), RightHand.end(), 0.0);

            const std::uint32_t First = Tyre * Lattice.ParticlesPerTyre();

            for (std::uint32_t Particle = First; Particle < First + Lattice.ParticlesPerTyre(); ++Particle)
            {
                std::uint32_t Own = 0u, Ring = 0u, Segment = 0u;
                ResolvePosition(Lattice, Particle, Own, Ring, Segment);

                const TyreGradientRecord& G = Lattice.Gradients[Particle];

                const double RX = ResidualX[Particle], RY = ResidualY[Particle], RZ = ResidualZ[Particle];

                RightHand[1u + Ring]                    -= G.RingX * RX + G.RingY * RY + G.RingZ * RZ;
                RightHand[1u + Lattice.Rings + Segment] -= G.MeridianX * RX + G.MeridianY * RY + G.MeridianZ * RZ;
            }

            for (std::uint32_t Row = 0u; Row < Order; ++Row)
            {
                double Sum = RightHand[Row];

                for (std::uint32_t Earlier = 0u; Earlier < Row; ++Earlier)
                    Sum -= Matrix[Row * ShapeRowLimit + Earlier] * RightHand[Earlier];

                RightHand[Row] = Sum / Matrix[Row * ShapeRowLimit + Row];
            }

            for (std::uint32_t Back = Order; Back > 0u; --Back)
            {
                const std::uint32_t Row = Back - 1u;
                double              Sum = RightHand[Row];

                for (std::uint32_t Later = Row + 1u; Later < Order; ++Later)
                    Sum -= Matrix[Later * ShapeRowLimit + Row] * RightHand[Later];

                RightHand[Row] = Sum / Matrix[Row * ShapeRowLimit + Row];
            }

            for (std::uint32_t Row = 1u; Row < Order; ++Row)
                RowForce[static_cast<std::size_t>(Tyre) * ShapeRowLimit + Row] += RightHand[Row];

            for (std::uint32_t Particle = First; Particle < First + Lattice.ParticlesPerTyre(); ++Particle)
            {
                std::uint32_t Own = 0u, Ring = 0u, Segment = 0u;
                ResolvePosition(Lattice, Particle, Own, Ring, Segment);

                const TyreGradientRecord& G        = Lattice.Gradients[Particle];
                const double              W        = Lattice.Particles[Particle].InverseMass;
                const double              RingF    = RightHand[1u + Ring];
                const double              MeridF   = RightHand[1u + Lattice.Rings + Segment];

                ResidualX[Particle] += W * (RingF * G.RingX + MeridF * G.MeridianX);
                ResidualY[Particle] += W * (RingF * G.RingY + MeridF * G.MeridianY);
                ResidualZ[Particle] += W * (RingF * G.RingZ + MeridF * G.MeridianZ);

                const double Size = std::sqrt(ResidualX[Particle] * ResidualX[Particle] + ResidualY[Particle] * ResidualY[Particle]
                                            + ResidualZ[Particle] * ResidualZ[Particle]);

                if (Size > Largest)
                    Largest = Size;
            }
        }

        // 📝 r is a virtual displacement, w times a force; at w = 100 /kg a residual of 10⁻⁴ is a millinewton.
        if (Largest < 1.0e-4)
            break;
    }

    for (std::size_t Cord = 0u; Cord < CordCount; ++Cord)
    {
        TyreCordRecord& Record  = Lattice.Cords[Cord];
        const double    Natural = static_cast<double>(Record.RestLength) + static_cast<double>(Record.Compliance) * CordForce[Cord];
        Record.RestLength       = static_cast<Real32>(Natural);
    }

    for (std::uint32_t Tyre = 0u; Tyre < Lattice.TyreCount; ++Tyre)
    {
        for (std::uint32_t Row = 1u; Row < Order; ++Row)
        {
            const std::size_t Index  = static_cast<std::size_t>(Tyre) * ShapeRowLimit + Row;
            TyreShapeRecord&  Record  = Lattice.ShapeRows[Index];
            const double      Natural = static_cast<double>(Record.RestValue) + static_cast<double>(Record.Compliance) * RowForce[Index];
            Record.RestValue          = static_cast<Real32>(Natural);
        }
    }

    std::fill(Lattice.ShapeAdvances.begin(), Lattice.ShapeAdvances.end(), 0.0f);
}

}   // namespace

ConvergentResult<ContactLoad> SolveTyreFrame(TyreLattice& Lattice, const TyreStepSpecification& Stepping)
{
    ConvergentResult<ContactLoad> Result;

    if (Lattice.TyreCount == 0u || Stepping.SubstepCount == 0u || Stepping.IterationCount == 0u || Stepping.FrameSeconds <= 0.0)
        return Result;

    HostStep Step;
    Step.Push.ParticleCount  = Lattice.ParticleCount();
    Step.Push.Rings          = Lattice.Rings;
    Step.Push.Segments       = Lattice.Segments;
    Step.Push.TyreCount      = Lattice.TyreCount;
    Step.Push.SubstepSeconds = static_cast<Real32>(Stepping.FrameSeconds / static_cast<double>(Stepping.SubstepCount));
    Step.Push.SubstepCount   = Stepping.SubstepCount;

    double Largest = 0.0;

    for (std::uint32_t Substep = 0u; Substep < Stepping.SubstepCount; ++Substep)
    {
        Step.Push.SubstepIndex = Substep;
        Step.Fraction          = static_cast<Real32>(Substep + 1u) / static_cast<Real32>(Stepping.SubstepCount);

        PredictAll(Lattice, Step);

        for (std::uint32_t Iteration = 0u; Iteration < Stepping.IterationCount; ++Iteration)
        {
            Step.Push.IterationIndex = Iteration;
            Step.Push.PassIndex      = Substep * Stepping.IterationCount + Iteration;

            Largest = 0.0;

            for (const CordColour& Colour : Lattice.Colours)
            {
                Step.Push.CordOffset = Colour.Offset;
                Step.Push.CordCount  = Colour.Count;

                const double Moved = ProjectColour(Lattice, Step, Colour);

                if (Moved > Largest)
                    Largest = Moved;
            }

            GatherAll(Lattice, Step);
            SolveShapeAll(Lattice, Step);

            const double Moved = ProjectContactAll(Lattice, Step);

            if (Moved > Largest)
                Largest = Moved;

            ++Result.IterationCount;
        }

        RecoverAll(Lattice, Step);
    }

    Result.Approximation = DecodeContactLoad(Lattice.Accumulators.data());
    Result.ResidualNorm  = Largest;
    Result.Cause         = Largest <= Stepping.Tolerance ? TerminationCause::CriterionSatisfied : TerminationCause::LimitReached;

    return Result;
}

}   // namespace Slate
