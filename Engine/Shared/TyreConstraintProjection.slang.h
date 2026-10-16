//============================================================================================================================================
//                                                    TYRECONSTRAINTPROJECTION.SLANG.H
//============================================================================================================================================
// 🧩 Extended position-based tyre carcass — cord, shape-block and ground-contact projections, one source for both toolchains.

#pragma once

#include "Shared/ToolchainInterchange.slang.h"

// 📐 🔴 WHY A CARCASS AND NOT A SPRING. A tyre is a pressurised membrane. Its vertical stiffness comes from the
//    inflation pressure acting over the contact patch, and only secondarily from the cords; a radial-spring
//    suspension model reproduces the deflection curve but cannot produce a patch, a sidewall bulge, a belt that
//    flattens under load or a lateral force that lags the slip angle. The projections below are the XPBD form
//    (Macklin, Müller, Chentanez 2016) of exactly four physical things: a cord of stiffness k between two
//    carcass particles, the gas law acting through the enclosed volume, the hoop and meridional tensions of
//    the membrane as a whole, and a unilateral plane with Coulomb friction. Nothing here is a spring in the
//    suspension sense and nothing is a vertex shader displacement.
//
// 📐 XPBD in one line. For a constraint C(x) with compliance α (the reciprocal stiffness) and a step Δt, each
//    projection advances the Lagrange multiplier λ by
//
//        Δλ = (−C − α̃ λ) / (Σᵢ wᵢ ‖∇ᵢC‖² + α̃),       α̃ = α / Δt²,
//        Δxᵢ = wᵢ Δλ ∇ᵢC,
//
//    with wᵢ the inverse mass. The multiplier carries the constraint force: f = λ ∇C / Δt². That last identity is
//    what makes the contact load **recoverable** from the device without a second computation — the solver already
//    holds it, and reading the load back is reading λ back.
//
// 📝 🔴 Every routine is scalar, single precision, and reads nothing but its arguments. Single precision is the
//    device's native rate and the whole point of running this on the device; double here would run at a
//    thirty-second of the rate on consumer hardware and buy nothing a tyre can show. `ParityRunner` compares the
//    two toolchains' forms at the Bounded guarantee, not the Exact one, because the device's fused multiply-add
//    fusion is permitted to differ from the host's.
//
// ⚠️ Lengths are metres, masses kilograms, time seconds and forces newtons throughout. The fixed-point encodings
//    at the end carry their own scale and are the only place a magnitude is anything but SI.

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE RECORDS
//------------------------------------------------------------------------------------------------------------------------

// 📝 🔴 Every record is four 32-bit scalars, so it is sixteen bytes and sixteen-aligned under std430, under
//    scalar layout and on the host alike. A three-component record would be twelve bytes on the host and
//    sixteen on the device under one of the layout rules, and the host would read every record after the first
//    at a drifting offset. The fourth scalar is never padding — each carries something the step needs.

/// 🧩 One carcass particle's current position and inverse mass.
/// note  🔴 An inverse mass of zero is a **bead** particle — kinematic, carried by the rim and never projected.
///        Testing for zero is the one branch every projection shares.
/// tag   guarantee, nonallocating, nonthrowing
struct TyreParticleRecord
{
    Real32  X;             // [m]    - current position
    Real32  Y;             // [m]
    Real32  Z;             // [m]
    Real32  InverseMass;   // [1/kg] - zero for a bead particle
};

/// 🧩 One particle's position at the start of the substep and its accumulated normal contact multiplier.
/// tag   guarantee, nonallocating, nonthrowing
struct TyreMotionRecord
{
    Real32  PreviousX;           // [m]    - where the substep began
    Real32  PreviousY;           // [m]
    Real32  PreviousZ;           // [m]
    Real32  ContactMultiplier;   // [kg·m] - λ of the ground contact this substep; zero while airborne
};

/// 🧩 One particle's velocity; the fourth scalar carries the tangential friction multiplier for the load readback.
/// tag   guarantee, nonallocating, nonthrowing
struct TyreVelocityRecord
{
    Real32  VelocityX;            // [m/s]
    Real32  VelocityY;            // [m/s]
    Real32  VelocityZ;            // [m/s]
    Real32  FrictionMultiplier;   // [kg·m] - ‖λ‖ of the tangential correction this substep
};

/// 🧩 One particle's rest position in the rim's own frame, and the tyre it belongs to.
/// note  📝 The tyre index is carried as a float because the record is four floats; it is exact below 2²⁴ and
///        a run of sixteen million tyres is not a run this engine will see.
/// tag   guarantee, nonallocating, nonthrowing
struct TyreRestRecord
{
    Real32  LocalX;      // [m] - rim frame; the rim axis is the local Z axis
    Real32  LocalY;      // [m]
    Real32  LocalZ;      // [m]
    Real32  TyreIndex;   // [-] - which tyre's rim carries it
};

/// 🧩 One particle's gradients of the three shape constraints it belongs to, and the pieces of each it owns.
/// note  📝 Three records' worth, forty-eight bytes, still a multiple of sixteen. The volume gradient is the
///        area-weighted outward normal; the ring and meridian gradients are differences of unit tangents, which
///        point inward along the curvature and are the direction a hoop under tension pulls.
/// tag   guarantee, nonallocating, nonthrowing
struct TyreGradientRecord
{
    Real32  VolumeX;        // [m²] - ∇ᵢV
    Real32  VolumeY;        // [m²]
    Real32  VolumeZ;        // [m²]
    Real32  OwnedVolume;    // [m³] - the two triangles of the quad this particle is the first corner of
    Real32  RingX;          // [-]  - ∇ᵢ of its ring's circumference
    Real32  RingY;          // [-]
    Real32  RingZ;          // [-]
    Real32  RingEdge;       // [m]  - the hoop edge from this particle to the next segment
    Real32  MeridianX;      // [-]  - ∇ᵢ of its meridian's bead-to-bead length
    Real32  MeridianY;      // [-]
    Real32  MeridianZ;      // [-]
    Real32  MeridianEdge;   // [m]  - the edge from this particle to the next ring; zero on the last ring
};

/// 🧩 One row of a tyre's shape block — the rest value and compliance of the volume, one ring or one meridian.
/// note  📝 Row 0 is the volume, rows 1…Rings the rings, rows Rings+1…Rings+Segments the meridians.
/// tag   guarantee, nonallocating, nonthrowing
struct TyreShapeRecord
{
    Real32  RestValue;    // [m³|m]    - the volume target, or the circumference or meridian length at rest
    Real32  Compliance;   // [m⁵/N|m/N] - α of the row
    Real32  Reserved0;    // [-]
    Real32  Reserved1;    // [-]
};

/// 🧩 One cord — a distance constraint between two carcass particles with its own compliance.
/// tag   guarantee, nonallocating, nonthrowing
struct TyreCordRecord
{
    Unsigned32  ParticleA;    // [-]   - the first particle
    Unsigned32  ParticleB;    // [-]   - the second
    Real32      RestLength;   // [m]   - at the inflated rest shape
    Real32      Compliance;   // [m/N] - α, the reciprocal cord stiffness
};

/// 🧩 One rim's pose at the start and the end of the frame, and the velocities the particles are damped toward.
/// note  🔴 Two poses and not one. The substeps interpolate between them, so a rim spinning at ninety radians
///        per second carries its bead particles through eight intermediate attitudes rather than one jump —
///        and the friction projection, which reads the per-substep displacement, sees a rolling contact
///        rather than a sliding one.
/// tag   guarantee, nonallocating, nonthrowing
struct TyreHubRecord
{
    Real32  BeginX;               // [m]     - rim centre at the frame's start
    Real32  BeginY;               // [m]
    Real32  BeginZ;               // [m]
    Real32  BeginRotationX;       // [-]     - unit quaternion, vector part
    Real32  BeginRotationY;       // [-]
    Real32  BeginRotationZ;       // [-]
    Real32  BeginRotationW;       // [-]     - scalar part
    Real32  EndX;                 // [m]     - rim centre at the frame's end
    Real32  EndY;                 // [m]
    Real32  EndZ;                 // [m]
    Real32  EndRotationX;         // [-]
    Real32  EndRotationY;         // [-]
    Real32  EndRotationZ;         // [-]
    Real32  EndRotationW;         // [-]
    Real32  LinearVelocityX;      // [m/s]   - of the rim centre over the frame
    Real32  LinearVelocityY;      // [m/s]
    Real32  LinearVelocityZ;      // [m/s]
    Real32  AngularVelocityX;     // [rad/s] - of the rim over the frame
    Real32  AngularVelocityY;     // [rad/s]
    Real32  AngularVelocityZ;     // [rad/s]
    Real32  RestVolume;           // [m³]    - V₀, at which the gas stands at the gauge pressure
    Real32  GaugePressure;        // [Pa]    - p₀ above ambient; what the volume row's target encodes
    Real32  InflationCompliance;  // [m⁵/N]  - V₀ / (γ p_abs); the gas law's stiffness about V₀
    Real32  GroundHeight;         // [m]     - the plane's Y; the plane normal is +Y
    Real32  StaticFriction;       // [-]     - μₛ
    Real32  KineticFriction;      // [-]     - μₖ
    Real32  DeformationDamping;   // [1/s]   - damping of velocity relative to the rigid rim motion
    Real32  GravityY;             // [m/s²]  - signed; −9.81 for a ground at the bottom
    Real32  ContactOffset;        // [m]     - tread thickness; the particle contacts this far above its position
    Real32  AdiabaticIndex;       // [-]     - γ of the gas; 1.4 for air
    Real32  AmbientPressure;      // [Pa]    - absolute, outside the carcass
    Real32  Reserved;             // [-]     - thirty-two scalars; one hundred and twenty-eight bytes
};

/// 🧩 What the recording pushes before every dispatch — the lattice extents, the substep, and the cord run.
/// tag   guarantee, nonallocating, nonthrowing
struct TyreStepPush
{
    Unsigned32  ParticleCount;    // [-] - across every tyre
    Unsigned32  Rings;            // [-] - meridional samples bead to bead, beads included
    Unsigned32  Segments;         // [-] - samples around the circumference; even
    Unsigned32  TyreCount;        // [-]
    Real32      SubstepSeconds;   // [s] - Δt of one substep
    Unsigned32  SubstepIndex;     // [-] - which substep of the frame
    Unsigned32  SubstepCount;     // [-] - substeps per frame
    Unsigned32  IterationIndex;   // [-] - which projection sweep within the substep
    Unsigned32  PassIndex;        // [-] - SubstepIndex × iterations + IterationIndex; its parity selects a slot
    Unsigned32  CordOffset;       // [-] - first cord of the colour being projected
    Unsigned32  CordCount;        // [-] - cords in that colour
};

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE ACCUMULATORS
//------------------------------------------------------------------------------------------------------------------------

// 📝 🔴 The per-tyre sums are integers because the device's atomic add is integer-only under glsl_450, and a
//    float atomic is an extension `06` has not negotiated. A fixed-point sum is also **order-independent**, which
//    a float sum across workgroups is not — the volume read on one frame is bit-identical to the volume read on
//    the next for the same positions, and the parity comparison can hold at all.

// 📝 One hundred and twenty-eight invocations per group, shared because the shape solve is one group per tyre and
//    the host that records the dispatch must size it to the same figure the kernel's barrier assumes.
SLATE_CONSTANT Unsigned32 TyreGroupExtent           = 128u;   // [-] - invocations per workgroup, every tyre entry point
SLATE_CONSTANT Unsigned32 TyreAccumulatorStride     = 16u;    // [-] - integers per tyre
SLATE_CONSTANT Unsigned32 AccumulatorForceX         = 0u;     // [-] - summed over the frame's substeps
SLATE_CONSTANT Unsigned32 AccumulatorForceY         = 1u;
SLATE_CONSTANT Unsigned32 AccumulatorForceZ         = 2u;
SLATE_CONSTANT Unsigned32 AccumulatorTorqueX        = 3u;     // [-] - about the rim centre
SLATE_CONSTANT Unsigned32 AccumulatorTorqueY        = 4u;
SLATE_CONSTANT Unsigned32 AccumulatorTorqueZ        = 5u;
SLATE_CONSTANT Unsigned32 AccumulatorContactCount   = 6u;     // [-] - particle-substeps in contact
SLATE_CONSTANT Unsigned32 AccumulatorPatchLeading   = 7u;     // [-] - min of contact X, fixed point
SLATE_CONSTANT Unsigned32 AccumulatorPatchTrailing  = 8u;     // [-] - max of contact X, fixed point
SLATE_CONSTANT Unsigned32 AccumulatorPatchInner     = 9u;     // [-] - min of contact Z, fixed point
SLATE_CONSTANT Unsigned32 AccumulatorPatchOuter     = 10u;    // [-] - max of contact Z, fixed point
SLATE_CONSTANT Unsigned32 AccumulatorPenetration    = 11u;    // [-] - max incursion before projection, fixed point

SLATE_CONSTANT Real32 VolumeFixedScale   = 5.0e8f;    // [1/m³] - 2 nm³ resolution; saturates past four cubic metres
SLATE_CONSTANT Real32 LoadFixedScale     = 4096.0f;   // [1/N] - quarter-millinewton resolution; saturates past 500 kN
SLATE_CONSTANT Real32 LengthFixedScale   = 1.0e6f;    // [1/m] - micrometre resolution; saturates past two kilometres

// 📝 🔴 THE SHAPE BLOCK. The volume, the rings and the meridians are **global** constraints — each spans a whole
//    tyre — and they are solved **together**, as one small dense system per tyre, rather than one after another.
//    Solved in sequence the volume constraint would push the carcass out by the whole gauge-pressure offset
//    and the rings would pull it back in, every sweep, a centimetre each way that never settles; solved
//    together the pressure and the hoop tension that balances it are found in the same step, and what moves
//    is only the difference from the balance. The block is
//
//        (J M⁻¹ Jᵀ + α̃) Δλ = −C − α̃ λ,       rows: volume, Rings rings, Segments meridians,
//
//    symmetric positive definite, of order 1 + Rings + Segments, and its entries are sums over particles
//    accumulated here. Its ring–ring and meridian–meridian blocks are diagonal — no two rings share a particle —
//    and each ring–meridian entry is one particle's, so no atomic is contended by more than a few invocations.
//
// 📝 Three fixed-point scales, by the row types an entry joins, because the entries span six orders of
//    magnitude: Σ w‖∇V‖² is of order 10⁻² m⁴/kg, a ring's Σ w‖∇C‖² of order 10² /kg, and a cross term between.

SLATE_CONSTANT Unsigned32 ShapeRowLimit          = 96u;                     // [-] - 1 + Rings + Segments at most
SLATE_CONSTANT Unsigned32 ShapeSumStride         = ShapeRowLimit * (ShapeRowLimit + 1u);   // [-] - integers per tyre: matrix, then values
SLATE_CONSTANT Unsigned32 ShapeMatrixStride      = ShapeRowLimit * ShapeRowLimit;          // [-] - floats per tyre of factor scratch

SLATE_CONSTANT Real32 ShapeVolumeDiagonalScale   = 4.0e9f;      // [kg/m⁴] - the volume–volume entry; saturates past 0.5
SLATE_CONSTANT Real32 ShapeVolumeCrossScale      = 2.68435456e8f;   // [kg/m²] - 2²⁸; a volume–length entry; saturates past 8
SLATE_CONSTANT Real32 ShapeLengthScale           = 1048576.0f;  // [kg]     - 2²⁰; a length–length entry; saturates past 2047

/// 🧩 The fixed-point scale of one shape matrix entry, from the two rows it joins.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED Real32 ShapeEntryScale(Unsigned32 Row, Unsigned32 Column)
{
    if (Row == 0u && Column == 0u)
        return ShapeVolumeDiagonalScale;

    if (Row == 0u || Column == 0u)
        return ShapeVolumeCrossScale;

    return ShapeLengthScale;
}

/// 🧩 The fixed-point scale of one shape right-hand-side entry — the volume's, or a length's.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED Real32 ShapeValueScale(Unsigned32 Row)
{
    return Row == 0u ? VolumeFixedScale : LengthFixedScale;
}

/// 🧩 Encodes a magnitude into a saturating fixed-point integer at a declared scale.
/// note  📝 Saturation rather than wrap: a sum that overflows reads as the largest representable force, which is
///        visibly wrong, where a wrapped one reads as a plausible force of the wrong sign.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED Signed32 EncodeFixed(Real32 Magnitude, Real32 Scale)
{
    const Real32 Scaled = Magnitude * Scale;

    if (Scaled >= 2147483520.0f)
        return 2147483520;

    if (Scaled <= -2147483520.0f)
        return -2147483520;

    return Signed32(Scaled);
}

/// 🧩 The inverse of EncodeFixed.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED Real32 DecodeFixed(Signed32 Encoded, Real32 Scale)
{
    return Real32(Encoded) / Scale;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE LATTICE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The particle index of a lattice position, wrapping around the circumference and never across the beads.
/// in    Ring     [-]  meridional index; the caller keeps it inside [0, Rings)
/// in    Segment  [-]  circumferential index; any integer, reduced modulo Segments
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED Unsigned32 LatticeIndex(Unsigned32 TyreIndex, Signed32 Ring, Signed32 Segment,
                                     Unsigned32 Rings, Unsigned32 Segments)
{
    const Signed32 Wrapped = ((Segment % Signed32(Segments)) + Signed32(Segments)) % Signed32(Segments);

    return TyreIndex * Rings * Segments + Unsigned32(Ring) * Segments + Unsigned32(Wrapped);
}

/// 🧩 The two further corners of one of the six triangles incident to a lattice particle, as ring and segment offsets.
/// in    Triangle  [-]  which of the six, in [0, 6)
/// note  📐 The quad with corners (k,s) (k+1,s) (k+1,s+1) (k,s+1) is split along (k,s)–(k+1,s+1). Every
///        triangle is wound so that, read from the particle, the corners run the same way round — which is what
///        makes Σ b × c the gradient of a **signed** volume rather than of six unrelated areas. A triangle whose
///        offsets would leave the ring interval does not exist and the caller skips it.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED void ResolveIncidentCorner(Unsigned32 Triangle,
                                        SLATE_OUT(Signed32) RingB, SLATE_OUT(Signed32) SegmentB,
                                        SLATE_OUT(Signed32) RingC, SLATE_OUT(Signed32) SegmentC)
{
    if (Triangle == 0u)      { RingB =  1; SegmentB =  0; RingC =  1; SegmentC =  1; }
    else if (Triangle == 1u) { RingB =  1; SegmentB =  1; RingC =  0; SegmentC =  1; }
    else if (Triangle == 2u) { RingB =  0; SegmentB =  1; RingC = -1; SegmentC =  0; }
    else if (Triangle == 3u) { RingB = -1; SegmentB =  0; RingC = -1; SegmentC = -1; }
    else if (Triangle == 4u) { RingB = -1; SegmentB = -1; RingC =  0; SegmentC = -1; }
    else                     { RingB =  0; SegmentB = -1; RingC =  1; SegmentC =  0; }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE RIM FRAME
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Rotates a vector by a unit quaternion.
/// note  📐 v' = v + 2 q_v × (q_v × v + q_w v). Two cross products rather than a matrix the caller would have to
///        hold for every particle.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED void RotateByQuaternion(Real32 QuaternionX, Real32 QuaternionY, Real32 QuaternionZ, Real32 QuaternionW,
                                     Real32 X, Real32 Y, Real32 Z,
                                     SLATE_OUT(Real32) RotatedX, SLATE_OUT(Real32) RotatedY, SLATE_OUT(Real32) RotatedZ)
{
    const Real32 CrossX = QuaternionY * Z - QuaternionZ * Y + QuaternionW * X;
    const Real32 CrossY = QuaternionZ * X - QuaternionX * Z + QuaternionW * Y;
    const Real32 CrossZ = QuaternionX * Y - QuaternionY * X + QuaternionW * Z;

    RotatedX = X + 2.0f * (QuaternionY * CrossZ - QuaternionZ * CrossY);
    RotatedY = Y + 2.0f * (QuaternionZ * CrossX - QuaternionX * CrossZ);
    RotatedZ = Z + 2.0f * (QuaternionX * CrossY - QuaternionY * CrossX);
}

/// 🧩 The rim's pose at a fraction of the frame — position interpolated, attitude normalised-lerped.
/// in    Fraction  [-]  in [0, 1]; the substep's end over the frame
/// note  📐 Normalised lerp and not slerp. One frame of rim rotation is a few degrees at most, where the two
///        agree to well below a particle's spacing, and nlerp has no branch at the antipode.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED void InterpolateHubPose(TyreHubRecord Hub, Real32 Fraction,
                                     SLATE_OUT(Real32) CentreX, SLATE_OUT(Real32) CentreY, SLATE_OUT(Real32) CentreZ,
                                     SLATE_OUT(Real32) RotationX, SLATE_OUT(Real32) RotationY,
                                     SLATE_OUT(Real32) RotationZ, SLATE_OUT(Real32) RotationW)
{
    CentreX = Hub.BeginX + (Hub.EndX - Hub.BeginX) * Fraction;
    CentreY = Hub.BeginY + (Hub.EndY - Hub.BeginY) * Fraction;
    CentreZ = Hub.BeginZ + (Hub.EndZ - Hub.BeginZ) * Fraction;

    // 📝 The shorter arc. Two quaternions of opposite sign are one attitude, and interpolating between them
    //    through the origin would collapse the rim for the middle substeps.
    const Real32 Agreement = Hub.BeginRotationX * Hub.EndRotationX + Hub.BeginRotationY * Hub.EndRotationY
                           + Hub.BeginRotationZ * Hub.EndRotationZ + Hub.BeginRotationW * Hub.EndRotationW;
    const Real32 Sign      = Agreement < 0.0f ? -1.0f : 1.0f;

    Real32 MixedX = Hub.BeginRotationX + (Sign * Hub.EndRotationX - Hub.BeginRotationX) * Fraction;
    Real32 MixedY = Hub.BeginRotationY + (Sign * Hub.EndRotationY - Hub.BeginRotationY) * Fraction;
    Real32 MixedZ = Hub.BeginRotationZ + (Sign * Hub.EndRotationZ - Hub.BeginRotationZ) * Fraction;
    Real32 MixedW = Hub.BeginRotationW + (Sign * Hub.EndRotationW - Hub.BeginRotationW) * Fraction;

    const Real32 Length = Real32(SquareRoot(Real64(MixedX * MixedX + MixedY * MixedY + MixedZ * MixedZ + MixedW * MixedW)));
    const Real32 Scale  = Length > 1.0e-12f ? 1.0f / Length : 1.0f;

    RotationX = MixedX * Scale;
    RotationY = MixedY * Scale;
    RotationZ = MixedZ * Scale;
    RotationW = MixedW * Scale;
}

/// 🧩 The velocity the rim's rigid motion would give a point — v = v_hub + ω × (x − c).
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED void RigidPointVelocity(TyreHubRecord Hub, Real32 X, Real32 Y, Real32 Z,
                                     SLATE_OUT(Real32) VelocityX, SLATE_OUT(Real32) VelocityY, SLATE_OUT(Real32) VelocityZ)
{
    const Real32 ArmX = X - Hub.EndX;
    const Real32 ArmY = Y - Hub.EndY;
    const Real32 ArmZ = Z - Hub.EndZ;

    VelocityX = Hub.LinearVelocityX + Hub.AngularVelocityY * ArmZ - Hub.AngularVelocityZ * ArmY;
    VelocityY = Hub.LinearVelocityY + Hub.AngularVelocityZ * ArmX - Hub.AngularVelocityX * ArmZ;
    VelocityZ = Hub.LinearVelocityZ + Hub.AngularVelocityX * ArmY - Hub.AngularVelocityY * ArmX;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PREDICTION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Advances one free particle by its velocity and gravity, delivering the predicted position.
/// note  📐 Semi-implicit Euler: v ← v + g Δt, x ← x + v Δt. The projections that follow correct x, and the
///        velocity is recovered from the correction afterwards. The inflation pressure is **not** a force here
///        — it is inside the shape block, where it is balanced against the hoop tension implicitly. As a
///        force it would move a nine-gram carcass particle by seven centimetres a substep.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED void PredictParticle(Real32 X, Real32 Y, Real32 Z,
                                  Real32 VelocityX, Real32 VelocityY, Real32 VelocityZ,
                                  Real32 GravityY, Real32 Substep,
                                  SLATE_OUT(Real32) PredictedX, SLATE_OUT(Real32) PredictedY, SLATE_OUT(Real32) PredictedZ)
{
    PredictedX = X + VelocityX * Substep;
    PredictedY = Y + (VelocityY + GravityY * Substep) * Substep;
    PredictedZ = Z + VelocityZ * Substep;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE CORD PROJECTION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Projects one cord, advancing its multiplier and delivering the correction each end receives.
/// in    Compliance  [m/N]  α; zero is an inextensible cord
/// in    Substep     [s]    Δt of the substep
/// out   Multiplier  [kg·m] λ, advanced in place
/// note  📐 C = ‖xₐ − x_b‖ − L₀, ∇ₐC = n, ∇_bC = −n. The denominator is wₐ + w_b + α/Δt²; a cord between two beads
///        has a zero denominator and is never built, which the lattice construction guarantees rather than this
///        routine testing for.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED void ProjectCord(Real32 AX, Real32 AY, Real32 AZ, Real32 InverseMassA,
                              Real32 BX, Real32 BY, Real32 BZ, Real32 InverseMassB,
                              Real32 RestLength, Real32 Compliance, Real32 Substep,
                              SLATE_INOUT(Real32) Multiplier,
                              SLATE_OUT(Real32) CorrectionAX, SLATE_OUT(Real32) CorrectionAY, SLATE_OUT(Real32) CorrectionAZ,
                              SLATE_OUT(Real32) CorrectionBX, SLATE_OUT(Real32) CorrectionBY, SLATE_OUT(Real32) CorrectionBZ)
{
    const Real32 DeltaX = AX - BX;
    const Real32 DeltaY = AY - BY;
    const Real32 DeltaZ = AZ - BZ;
    const Real32 Length = Real32(SquareRoot(Real64(DeltaX * DeltaX + DeltaY * DeltaY + DeltaZ * DeltaZ)));

    CorrectionAX = 0.0f;
    CorrectionAY = 0.0f;
    CorrectionAZ = 0.0f;
    CorrectionBX = 0.0f;
    CorrectionBY = 0.0f;
    CorrectionBZ = 0.0f;

    if (Length < 1.0e-9f)
        return;

    const Real32 ScaledCompliance = Compliance / (Substep * Substep);
    const Real32 Residual         = Length - RestLength;
    const Real32 Denominator      = InverseMassA + InverseMassB + ScaledCompliance;

    if (Denominator <= 0.0f)
        return;

    const Real32 Advance = (-Residual - ScaledCompliance * Multiplier) / Denominator;
    const Real32 Along   = Advance / Length;

    Multiplier += Advance;

    CorrectionAX =  InverseMassA * Along * DeltaX;
    CorrectionAY =  InverseMassA * Along * DeltaY;
    CorrectionAZ =  InverseMassA * Along * DeltaZ;
    CorrectionBX = -InverseMassB * Along * DeltaX;
    CorrectionBY = -InverseMassB * Along * DeltaY;
    CorrectionBZ = -InverseMassB * Along * DeltaZ;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SHAPE BLOCK
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The signed volume one triangle sweeps from the rim centre — a · (b × c) / 6 with every corner relative to it.
/// note  📐 Relative to the rim centre and not the origin, so that the carcass alone is a closed surface up to
///        the two bead cones, whose volume is rigid with the rim and therefore constant. Relative to the world
///        origin the missing cones would contribute a term that moves with the rim.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED Real32 TriangleSweptVolume(Real32 AX, Real32 AY, Real32 AZ,
                                        Real32 BX, Real32 BY, Real32 BZ,
                                        Real32 CX, Real32 CY, Real32 CZ)
{
    const Real32 CrossX = BY * CZ - BZ * CY;
    const Real32 CrossY = BZ * CX - BX * CZ;
    const Real32 CrossZ = BX * CY - BY * CX;

    return (AX * CrossX + AY * CrossY + AZ * CrossZ) / 6.0f;
}

/// 🧩 The gradient of one triangle's swept volume with respect to its first corner — (b × c) / 6.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED void TriangleVolumeGradient(Real32 BX, Real32 BY, Real32 BZ,
                                         Real32 CX, Real32 CY, Real32 CZ,
                                         SLATE_INOUT(Real32) GradientX, SLATE_INOUT(Real32) GradientY, SLATE_INOUT(Real32) GradientZ)
{
    GradientX += (BY * CZ - BZ * CY) / 6.0f;
    GradientY += (BZ * CX - BX * CZ) / 6.0f;
    GradientZ += (BX * CY - BY * CX) / 6.0f;
}

/// 🧩 One shape block entry's contribution from one particle — w (a · b) for two of its gradients.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED Real32 WeightedDot(Real32 Weight, Real32 AX, Real32 AY, Real32 AZ, Real32 BX, Real32 BY, Real32 BZ)
{
    return Weight * (AX * BX + AY * BY + AZ * BZ);
}

/// 🧩 Adds one edge's contribution to a length constraint's gradient at this particle — the unit vector from the other end.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED Real32 EdgeLengthGradient(Real32 X, Real32 Y, Real32 Z, Real32 OtherX, Real32 OtherY, Real32 OtherZ,
                                       SLATE_INOUT(Real32) GradientX, SLATE_INOUT(Real32) GradientY, SLATE_INOUT(Real32) GradientZ)
{
    const Real32 DeltaX = X - OtherX;
    const Real32 DeltaY = Y - OtherY;
    const Real32 DeltaZ = Z - OtherZ;
    const Real32 Length = Real32(SquareRoot(Real64(DeltaX * DeltaX + DeltaY * DeltaY + DeltaZ * DeltaZ)));

    if (Length < 1.0e-9f)
        return 0.0f;

    GradientX += DeltaX / Length;
    GradientY += DeltaY / Length;
    GradientZ += DeltaZ / Length;

    return Length;
}

// 📐 🔴 THE SHAPE SOLVE is the one piece the two toolchains write separately — `TyreShapeSolve.slang` and the host
//    form in `TyreSolver.cpp` — because it walks a matrix, and a span cannot be passed through this header's
//    scalar interface. Both do the same thing in the same order: decode the fixed-point sums, add α̃ to the
//    diagonal, factor L Lᵀ = A column by column (L_kk = √(A_kk − Σⱼ L_kj²), L_ik = (A_ik − Σⱼ L_ij L_kj) / L_kk),
//    forward- then back-substitute, and deliver Δλ per row. A pivot that reads non-positive is a saturated
//    fixed-point sum; it is clamped to 10⁻³⁰, which makes that row's advance zero rather than infinite.

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE CONTACT PROJECTION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Projects one particle against the ground plane with Coulomb friction, delivering the correction and both multipliers.
/// in    ContactOffset       [m]    the tread stands this far below the particle
/// out   NormalMultiplier    [kg·m] λₙ of this substep; zero when the particle is clear of the ground
/// out   FrictionMultiplier  [kg·m] ‖λₜ‖ of the tangential correction
/// note  📐 The normal constraint is C = y − y_ground − offset ≥ 0 with zero compliance, so Δλₙ = −C / w and the
///        particle is lifted exactly onto the plane. Friction follows Macklin et al. 2014 §6: the tangential
///        displacement since the substep began is removed wholly when it lies inside the static cone
///        μₛ ‖Δxₙ‖, and otherwise scaled back to the kinetic cone μₖ ‖Δxₙ‖.
/// note  🔴 A unilateral constraint's multiplier is **reset every substep** and not carried, because a carried λₙ
///        would pull a particle that has just left the ground back toward it. The load readback sums the
///        per-substep multipliers over the frame instead.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED void ProjectGroundContact(Real32 X, Real32 Y, Real32 Z, Real32 InverseMass,
                                       Real32 PreviousX, Real32 PreviousZ,
                                       Real32 GroundHeight, Real32 ContactOffset,
                                       Real32 StaticFriction, Real32 KineticFriction,
                                       SLATE_OUT(Real32) CorrectionX, SLATE_OUT(Real32) CorrectionY, SLATE_OUT(Real32) CorrectionZ,
                                       SLATE_OUT(Real32) NormalMultiplier, SLATE_OUT(Real32) FrictionMultiplier)
{
    CorrectionX        = 0.0f;
    CorrectionY        = 0.0f;
    CorrectionZ        = 0.0f;
    NormalMultiplier   = 0.0f;
    FrictionMultiplier = 0.0f;

    const Real32 Penetration = Y - ContactOffset - GroundHeight;

    if (Penetration >= 0.0f || InverseMass <= 0.0f)
        return;

    const Real32 Lift = -Penetration;

    CorrectionY      = Lift;
    NormalMultiplier = Lift / InverseMass;

    const Real32 SlideX = X - PreviousX;
    const Real32 SlideZ = Z - PreviousZ;
    const Real32 Slide  = Real32(SquareRoot(Real64(SlideX * SlideX + SlideZ * SlideZ)));

    if (Slide < 1.0e-9f)
        return;

    Real32 Removed = 1.0f;

    if (Slide >= StaticFriction * Lift)
        Removed = KineticFriction * Lift / Slide;

    if (Removed > 1.0f)
        Removed = 1.0f;

    CorrectionX        = -SlideX * Removed;
    CorrectionZ        = -SlideZ * Removed;
    FrictionMultiplier = Slide * Removed / InverseMass;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE VELOCITY RECOVERY
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Recovers a particle's velocity from its corrected displacement and damps it toward the rim's rigid motion.
/// in    Damping  [1/s]  rate at which the deformation velocity decays; zero leaves it undamped
/// note  📐 v = (x − x_prev)/Δt, then v ← v_rigid + (v − v_rigid)(1 − d Δt)⁺. Damping the velocity relative to
///        the rigid rim motion rather than to rest is what lets a wheel spin at ninety radians per second without
///        the damping reading its own rotation as vibration and bleeding it away.
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED void RecoverVelocity(Real32 X, Real32 Y, Real32 Z,
                                  Real32 PreviousX, Real32 PreviousY, Real32 PreviousZ,
                                  Real32 RigidX, Real32 RigidY, Real32 RigidZ,
                                  Real32 Damping, Real32 Substep,
                                  SLATE_OUT(Real32) VelocityX, SLATE_OUT(Real32) VelocityY, SLATE_OUT(Real32) VelocityZ)
{
    const Real32 Inverse = 1.0f / Substep;

    Real32 Retained = 1.0f - Damping * Substep;

    if (Retained < 0.0f)
        Retained = 0.0f;

    VelocityX = RigidX + ((X - PreviousX) * Inverse - RigidX) * Retained;
    VelocityY = RigidY + ((Y - PreviousY) * Inverse - RigidY) * Retained;
    VelocityZ = RigidZ + ((Z - PreviousZ) * Inverse - RigidZ) * Retained;
}

/// 🧩 The force a multiplier carried over one substep — f = λ / Δt².
/// cost  ✔️
/// tag   shared, parity, nonallocating, nonthrowing
SLATE_SHARED Real32 ForceOfMultiplier(Real32 Multiplier, Real32 Substep)
{
    return Multiplier / (Substep * Substep);
}

}   // namespace Slate
