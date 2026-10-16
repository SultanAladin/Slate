//============================================================================================================================================
//                                                            TYREPREDICTION.SLANG
//============================================================================================================================================
// 🧩 ① of the substep — beads carried to the rim's interpolated pose, free particles advanced by velocity and gravity.

#include "SlateCompute/Compute/TyreSolver/Shader/TyreDeclarations.slang"

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

    const TyreHubRecord      Hub     = Hubs[Tyre];
    const TyreParticleRecord Current = Particles[Particle];
    const TyreRestRecord     Rest    = Rests[Particle];

    // 📝 The frame's first pass zeroes the tyre's load sums, from the one invocation that owns them; nothing on
    //    the host ever clears this span.
    if (Step.SubstepIndex == 0u && TyreLeadingParticle(Ring, Segment))
    {
        const Unsigned32 Base = Tyre * TyreAccumulatorStride;

        for (Unsigned32 Slot = 0u; Slot < TyreAccumulatorStride; ++Slot)
            Accumulators[Base + Slot].store(0);

        Accumulators[Base + AccumulatorPatchLeading].store(2147483647);
        Accumulators[Base + AccumulatorPatchTrailing].store(-2147483647);
        Accumulators[Base + AccumulatorPatchInner].store(2147483647);
        Accumulators[Base + AccumulatorPatchOuter].store(-2147483647);
    }

    // 📝 The substep's starting positions were written by the previous velocity recovery, not here; only the
    //    contact multiplier is reset.
    Motions[Particle].ContactMultiplier = 0.0f;

    TyreVelocityRecord Velocity = Velocities[Particle];
    Velocity.FrictionMultiplier = 0.0f;

    TyreParticleRecord Predicted = Current;

    if (Current.InverseMass <= 0.0f)
    {
        // 📝 A bead rides the rim. Its position is the rim's interpolated pose applied to its rest position, and
        //    its displacement over the substep is what the friction projection reads as the rolling motion.
        Real32 CentreX   = 0.0f;
        Real32 CentreY   = 0.0f;
        Real32 CentreZ   = 0.0f;
        Real32 RotationX = 0.0f;
        Real32 RotationY = 0.0f;
        Real32 RotationZ = 0.0f;
        Real32 RotationW = 1.0f;
        InterpolateHubPose(Hub, SubstepFraction(), CentreX, CentreY, CentreZ,
                           RotationX, RotationY, RotationZ, RotationW);

        Real32 CarriedX = 0.0f;
        Real32 CarriedY = 0.0f;
        Real32 CarriedZ = 0.0f;
        RotateByQuaternion(RotationX, RotationY, RotationZ, RotationW,
                           Rest.LocalX, Rest.LocalY, Rest.LocalZ, CarriedX, CarriedY, CarriedZ);

        Predicted.X = CentreX + CarriedX;
        Predicted.Y = CentreY + CarriedY;
        Predicted.Z = CentreZ + CarriedZ;
    }
    else
    {
        Real32 PredictedX = 0.0f;
        Real32 PredictedY = 0.0f;
        Real32 PredictedZ = 0.0f;
        PredictParticle(Current.X, Current.Y, Current.Z,
                        Velocity.VelocityX, Velocity.VelocityY, Velocity.VelocityZ,
                        Hub.GravityY, Step.SubstepSeconds,
                        PredictedX, PredictedY, PredictedZ);

        Predicted.X = PredictedX;
        Predicted.Y = PredictedY;
        Predicted.Z = PredictedZ;
    }

    Particles[Particle]  = Predicted;
    Velocities[Particle] = Velocity;
}

}   // namespace Slate
