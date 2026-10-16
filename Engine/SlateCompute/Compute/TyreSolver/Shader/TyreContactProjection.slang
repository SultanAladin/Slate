//============================================================================================================================================
//                                                        TYRECONTACTPROJECTION.SLANG
//============================================================================================================================================
// 🧩 ④ of the substep — the shape block's correction applied, the ground contact projected, and the contact load summed.

#include "SlateCompute/Compute/TyreSolver/Shader/TyreDeclarations.slang"

// 📝 🔴 THIS IS WHERE THE LOAD COMES BACK. The ground contact's multiplier λₙ **is** the normal impulse, and
//    λₙ / Δt² is the force; the friction correction divided by the inverse mass is the tangential impulse the
//    same way. Both are summed per tyre into fixed-point integers here, during the projection that produced
//    them, so recovering the wheel load costs the recording one sixteen-integer copy per tyre and no second
//    dispatch. The host reads that copy back `RecordingSlotCount` frames later — see `TyreDeviceExchange`.

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

    const TyreHubRecord Hub  = Hubs[Tyre];
    const Unsigned32    Base = Tyre * TyreAccumulatorStride;

    TyreParticleRecord Current = Particles[Particle];

    if (Current.InverseMass <= 0.0f)
        return;

    // 📝 The shape block's advance, three rows per particle: its tyre's volume, its ring and its meridian.
    const TyreGradientRecord Gradient        = Gradients[Particle];
    const Unsigned32         Rows            = Tyre * ShapeRowLimit;
    const Real32             VolumeAdvance   = ShapeAdvances[Rows];
    const Real32             RingAdvance     = ShapeAdvances[Rows + 1u + Ring];
    const Real32             MeridianAdvance = ShapeAdvances[Rows + 1u + Step.Rings + Segment];

    const Real32 ShapedX = Current.InverseMass * (VolumeAdvance * Gradient.VolumeX + RingAdvance * Gradient.RingX
                                                + MeridianAdvance * Gradient.MeridianX);
    const Real32 ShapedY = Current.InverseMass * (VolumeAdvance * Gradient.VolumeY + RingAdvance * Gradient.RingY
                                                + MeridianAdvance * Gradient.MeridianY);
    const Real32 ShapedZ = Current.InverseMass * (VolumeAdvance * Gradient.VolumeZ + RingAdvance * Gradient.RingZ
                                                + MeridianAdvance * Gradient.MeridianZ);

    Current.X += ShapedX;
    Current.Y += ShapedY;
    Current.Z += ShapedZ;

    TyreMotionRecord   Motion   = Motions[Particle];
    TyreVelocityRecord Velocity = Velocities[Particle];

    const Real32 Penetration = Hub.GroundHeight + Hub.ContactOffset - Current.Y;

    Real32 CorrectionX        = 0.0f;
    Real32 CorrectionY        = 0.0f;
    Real32 CorrectionZ        = 0.0f;
    Real32 NormalMultiplier   = 0.0f;
    Real32 FrictionMultiplier = 0.0f;

    ProjectGroundContact(Current.X, Current.Y, Current.Z, Current.InverseMass,
                         Motion.PreviousX, Motion.PreviousZ,
                         Hub.GroundHeight, Hub.ContactOffset,
                         Hub.StaticFriction, Hub.KineticFriction,
                         CorrectionX, CorrectionY, CorrectionZ,
                         NormalMultiplier, FrictionMultiplier);

    Current.X += CorrectionX;
    Current.Y += CorrectionY;
    Current.Z += CorrectionZ;

    Particles[Particle] = Current;

    if (NormalMultiplier <= 0.0f)
        return;

    Motion.ContactMultiplier    += NormalMultiplier;
    Velocity.FrictionMultiplier += FrictionMultiplier;
    Motions[Particle]            = Motion;
    Velocities[Particle]         = Velocity;

    // 📐 f = λ / Δt² per substep; the frame's load is the mean over its substeps, so each substep contributes
    //    its force over the substep count. The friction force is the tangential correction read as an impulse.
    const Real32 Share  = 1.0f / Real32(Step.SubstepCount);
    const Real32 ForceX = ForceOfMultiplier(CorrectionX / Current.InverseMass, Step.SubstepSeconds) * Share;
    const Real32 ForceY = ForceOfMultiplier(NormalMultiplier, Step.SubstepSeconds) * Share;
    const Real32 ForceZ = ForceOfMultiplier(CorrectionZ / Current.InverseMass, Step.SubstepSeconds) * Share;

    // 📝 The arm is to the rim centre **at this substep**, not at the frame's end; at ten metres a second the
    //    two differ by seventeen centimetres on the first substep, which is half the radius.
    Real32 CentreX   = 0.0f;
    Real32 CentreY   = 0.0f;
    Real32 CentreZ   = 0.0f;
    Real32 RotationX = 0.0f;
    Real32 RotationY = 0.0f;
    Real32 RotationZ = 0.0f;
    Real32 RotationW = 1.0f;
    InterpolateHubPose(Hub, SubstepFraction(), CentreX, CentreY, CentreZ, RotationX, RotationY, RotationZ, RotationW);

    const Real32 ArmX = Current.X - CentreX;
    const Real32 ArmY = Current.Y - CentreY;
    const Real32 ArmZ = Current.Z - CentreZ;

    InterlockedAdd(Accumulators[Base + AccumulatorForceX],  EncodeFixed(ForceX, LoadFixedScale));
    InterlockedAdd(Accumulators[Base + AccumulatorForceY],  EncodeFixed(ForceY, LoadFixedScale));
    InterlockedAdd(Accumulators[Base + AccumulatorForceZ],  EncodeFixed(ForceZ, LoadFixedScale));
    InterlockedAdd(Accumulators[Base + AccumulatorTorqueX], EncodeFixed(ArmY * ForceZ - ArmZ * ForceY, LoadFixedScale));
    InterlockedAdd(Accumulators[Base + AccumulatorTorqueY], EncodeFixed(ArmZ * ForceX - ArmX * ForceZ, LoadFixedScale));
    InterlockedAdd(Accumulators[Base + AccumulatorTorqueZ], EncodeFixed(ArmX * ForceY - ArmY * ForceX, LoadFixedScale));
    InterlockedAdd(Accumulators[Base + AccumulatorContactCount], 1);
    InterlockedMin(Accumulators[Base + AccumulatorPatchLeading],  EncodeFixed(Current.X, LengthFixedScale));
    InterlockedMax(Accumulators[Base + AccumulatorPatchTrailing], EncodeFixed(Current.X, LengthFixedScale));
    InterlockedMin(Accumulators[Base + AccumulatorPatchInner],    EncodeFixed(Current.Z, LengthFixedScale));
    InterlockedMax(Accumulators[Base + AccumulatorPatchOuter],    EncodeFixed(Current.Z, LengthFixedScale));
    InterlockedMax(Accumulators[Base + AccumulatorPenetration],   EncodeFixed(Penetration, LengthFixedScale));
}

}   // namespace Slate
