//============================================================================================================================================
//                                                          TYRESHAPEGATHER.SLANG
//============================================================================================================================================
// 🧩 ③ of the substep — every particle's volume, ring and meridian gradients, and the shape block's sums per tyre.

#include "SlateCompute/Compute/TyreSolver/Shader/TyreDeclarations.slang"

// 📝 🔴 The shape constraints are **global**: the volume, each ring's circumference and each meridian's length span
//    a whole tyre and are read by every particle of it. That is the one thing a particle-parallel solver cannot
//    do in a single dispatch, and it is why the shape step is three — this entry point reduces, `TyreShapeSolve`
//    factors, `TyreContactProjection` applies. The reduction lands in fixed-point integers through the device's
//    atomic add, which is order-independent and therefore bit-reproducible frame to frame.
//
// 📝 No triangle span. The lattice is structured, so each particle's six incident triangles and four lattice
//    neighbours are arithmetic on its ring and segment, and `ResolveIncidentCorner` and `LatticeIndex` are the
//    places that arithmetic is written — the host form reads the same routines.

namespace Slate
{

[shader("compute")]
[numthreads(128, 1, 1)]
void main(Unsigned32x3 Dispatched : SV_DispatchThreadID)
{
    const Unsigned32 Particle = Dispatched.x;

    if (Particle >= Step.ParticleCount)
        return;

    Unsigned32 Tyre    = 0u;
    Unsigned32 Ring    = 0u;
    Unsigned32 Segment = 0u;
    ResolveLatticePosition(Particle, Tyre, Ring, Segment);

    const TyreHubRecord Hub = Hubs[Tyre];

    Real32 CentreX   = 0.0f;
    Real32 CentreY   = 0.0f;
    Real32 CentreZ   = 0.0f;
    Real32 RotationX = 0.0f;
    Real32 RotationY = 0.0f;
    Real32 RotationZ = 0.0f;
    Real32 RotationW = 1.0f;
    InterpolateHubPose(Hub, SubstepFraction(), CentreX, CentreY, CentreZ, RotationX, RotationY, RotationZ, RotationW);

    const TyreParticleRecord Own = Particles[Particle];

    const Real32 OwnX = Own.X - CentreX;
    const Real32 OwnY = Own.Y - CentreY;
    const Real32 OwnZ = Own.Z - CentreZ;

    TyreGradientRecord Gradient;
    Gradient.VolumeX      = 0.0f;
    Gradient.VolumeY      = 0.0f;
    Gradient.VolumeZ      = 0.0f;
    Gradient.OwnedVolume  = 0.0f;
    Gradient.RingX        = 0.0f;
    Gradient.RingY        = 0.0f;
    Gradient.RingZ        = 0.0f;
    Gradient.RingEdge     = 0.0f;
    Gradient.MeridianX    = 0.0f;
    Gradient.MeridianY    = 0.0f;
    Gradient.MeridianZ    = 0.0f;
    Gradient.MeridianEdge = 0.0f;

    for (Unsigned32 Triangle = 0u; Triangle < 6u; ++Triangle)
    {
        Signed32 RingB    = 0;
        Signed32 SegmentB = 0;
        Signed32 RingC    = 0;
        Signed32 SegmentC = 0;
        ResolveIncidentCorner(Triangle, RingB, SegmentB, RingC, SegmentC);

        const Signed32 AbsoluteB = Signed32(Ring) + RingB;
        const Signed32 AbsoluteC = Signed32(Ring) + RingC;

        if (AbsoluteB < 0 || AbsoluteC < 0 || AbsoluteB >= Signed32(Step.Rings) || AbsoluteC >= Signed32(Step.Rings))
            continue;

        const TyreParticleRecord B = Particles[LatticeIndex(Tyre, AbsoluteB, Signed32(Segment) + SegmentB, Step.Rings, Step.Segments)];
        const TyreParticleRecord C = Particles[LatticeIndex(Tyre, AbsoluteC, Signed32(Segment) + SegmentC, Step.Rings, Step.Segments)];

        const Real32 BX = B.X - CentreX;
        const Real32 BY = B.Y - CentreY;
        const Real32 BZ = B.Z - CentreZ;
        const Real32 CX = C.X - CentreX;
        const Real32 CY = C.Y - CentreY;
        const Real32 CZ = C.Z - CentreZ;

        TriangleVolumeGradient(BX, BY, BZ, CX, CY, CZ, Gradient.VolumeX, Gradient.VolumeY, Gradient.VolumeZ);

        // 📝 The two triangles of the quad this particle leads are the two it owns for the volume sum, so every
        //    triangle of the carcass is counted exactly once across the dispatch.
        if (Triangle < 2u)
            Gradient.OwnedVolume += TriangleSweptVolume(OwnX, OwnY, OwnZ, BX, BY, BZ, CX, CY, CZ);
    }

    // 📝 The ring row: both hoop neighbours; the edge to the next segment is the one this particle owns.
    {
        const TyreParticleRecord Before = Particles[LatticeIndex(Tyre, Signed32(Ring), Signed32(Segment) - 1, Step.Rings, Step.Segments)];
        const TyreParticleRecord After  = Particles[LatticeIndex(Tyre, Signed32(Ring), Signed32(Segment) + 1, Step.Rings, Step.Segments)];

        EdgeLengthGradient(Own.X, Own.Y, Own.Z, Before.X, Before.Y, Before.Z, Gradient.RingX, Gradient.RingY, Gradient.RingZ);
        Gradient.RingEdge = EdgeLengthGradient(Own.X, Own.Y, Own.Z, After.X, After.Y, After.Z,
                                               Gradient.RingX, Gradient.RingY, Gradient.RingZ);
    }

    // 📝 The meridian row: the ring below and the ring above where they exist; the edge upward is owned.
    if (Ring > 0u)
    {
        const TyreParticleRecord Below = Particles[LatticeIndex(Tyre, Signed32(Ring) - 1, Signed32(Segment), Step.Rings, Step.Segments)];
        EdgeLengthGradient(Own.X, Own.Y, Own.Z, Below.X, Below.Y, Below.Z, Gradient.MeridianX, Gradient.MeridianY, Gradient.MeridianZ);
    }

    if (Ring + 1u < Step.Rings)
    {
        const TyreParticleRecord Above = Particles[LatticeIndex(Tyre, Signed32(Ring) + 1, Signed32(Segment), Step.Rings, Step.Segments)];
        Gradient.MeridianEdge = EdgeLengthGradient(Own.X, Own.Y, Own.Z, Above.X, Above.Y, Above.Z,
                                                   Gradient.MeridianX, Gradient.MeridianY, Gradient.MeridianZ);
    }

    Gradients[Particle] = Gradient;

    // 📝 The block's sums. Row 0 is the volume, row 1 + Ring this ring, row 1 + Rings + Segment this meridian;
    //    an entry is stored once, at (lower row, higher column), and the solve mirrors it.
    const Unsigned32 RingRow     = 1u + Ring;
    const Unsigned32 MeridianRow = 1u + Step.Rings + Segment;
    const Real32     W           = Own.InverseMass;
    const Unsigned32 Base        = Tyre * ShapeSumStride;
    const Unsigned32 Values      = Base + ShapeMatrixStride;

    const Real32 VX = Gradient.VolumeX,   VY = Gradient.VolumeY,   VZ = Gradient.VolumeZ;
    const Real32 RX = Gradient.RingX,     RY = Gradient.RingY,     RZ = Gradient.RingZ;
    const Real32 MX = Gradient.MeridianX, MY = Gradient.MeridianY, MZ = Gradient.MeridianZ;

    const Unsigned32 VolumeVolume     = Base + 0u * ShapeRowLimit + 0u;
    const Unsigned32 RingRing         = Base + RingRow * ShapeRowLimit + RingRow;
    const Unsigned32 MeridianMeridian = Base + MeridianRow * ShapeRowLimit + MeridianRow;
    const Unsigned32 VolumeRing       = Base + 0u * ShapeRowLimit + RingRow;
    const Unsigned32 VolumeMeridian   = Base + 0u * ShapeRowLimit + MeridianRow;
    const Unsigned32 RingMeridian     = Base + RingRow * ShapeRowLimit + MeridianRow;

    const Real32 DiagonalScale = ShapeEntryScale(RingRow, RingRow);   // 📝 every length–length entry shares it
    const Real32 CrossScale    = ShapeEntryScale(0u, RingRow);        // 📝 every volume–length entry shares it

    InterlockedAdd(ShapeSums[VolumeVolume],     EncodeFixed(WeightedDot(W, VX, VY, VZ, VX, VY, VZ), ShapeEntryScale(0u, 0u)));
    InterlockedAdd(ShapeSums[RingRing],         EncodeFixed(WeightedDot(W, RX, RY, RZ, RX, RY, RZ), DiagonalScale));
    InterlockedAdd(ShapeSums[MeridianMeridian], EncodeFixed(WeightedDot(W, MX, MY, MZ, MX, MY, MZ), DiagonalScale));
    InterlockedAdd(ShapeSums[VolumeRing],       EncodeFixed(WeightedDot(W, VX, VY, VZ, RX, RY, RZ), CrossScale));
    InterlockedAdd(ShapeSums[VolumeMeridian],   EncodeFixed(WeightedDot(W, VX, VY, VZ, MX, MY, MZ), CrossScale));
    InterlockedAdd(ShapeSums[RingMeridian],     EncodeFixed(WeightedDot(W, RX, RY, RZ, MX, MY, MZ), DiagonalScale));

    InterlockedAdd(ShapeSums[Values + 0u],          EncodeFixed(Gradient.OwnedVolume,  ShapeValueScale(0u)));
    InterlockedAdd(ShapeSums[Values + RingRow],     EncodeFixed(Gradient.RingEdge,     ShapeValueScale(RingRow)));
    InterlockedAdd(ShapeSums[Values + MeridianRow], EncodeFixed(Gradient.MeridianEdge, ShapeValueScale(MeridianRow)));
}

}   // namespace Slate
