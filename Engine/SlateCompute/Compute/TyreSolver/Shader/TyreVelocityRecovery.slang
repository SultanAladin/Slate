//============================================================================================================================================
//                                                        TYREVELOCITYRECOVERY.SLANG
//============================================================================================================================================
// 🧩 ⑤ of the substep — velocities recovered from the corrected displacements and damped toward the rim's rigid motion.

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
    const TyreMotionRecord   Motion  = Motions[Particle];

    Real32 RigidX = 0.0f;
    Real32 RigidY = 0.0f;
    Real32 RigidZ = 0.0f;
    RigidPointVelocity(Hub, Current.X, Current.Y, Current.Z, RigidX, RigidY, RigidZ);

    TyreVelocityRecord Velocity = Velocities[Particle];

    if (Current.InverseMass <= 0.0f)
    {
        Velocity.VelocityX = RigidX;
        Velocity.VelocityY = RigidY;
        Velocity.VelocityZ = RigidZ;
    }
    else
    {
        RecoverVelocity(Current.X, Current.Y, Current.Z,
                        Motion.PreviousX, Motion.PreviousY, Motion.PreviousZ,
                        RigidX, RigidY, RigidZ,
                        Hub.DeformationDamping, Step.SubstepSeconds,
                        Velocity.VelocityX, Velocity.VelocityY, Velocity.VelocityZ);
    }

    Velocities[Particle] = Velocity;

    // 📝 The next substep starts here. Written at the end of this one rather than at the start of the next so
    //    that the prediction, which reads neighbours' starting positions, reads a span it never writes.
    TyreMotionRecord Started = Motion;
    Started.PreviousX  = Current.X;
    Started.PreviousY  = Current.Y;
    Started.PreviousZ  = Current.Z;
    Motions[Particle]  = Started;
}

}   // namespace Slate
