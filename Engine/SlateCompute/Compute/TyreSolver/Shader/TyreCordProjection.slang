//============================================================================================================================================
//                                                         TYRECORDPROJECTION.SLANG
//============================================================================================================================================
// 🧩 ② of the substep — one colour of cords projected in parallel, no two of which share a particle.

#include "SlateCompute/Compute/TyreSolver/Shader/TyreDeclarations.slang"

// 📝 🔴 Gauss–Seidel without atomics. The host sorts the cords into colours such that no two cords of one colour
//    touch the same particle, and the recording dispatches one colour at a time with a barrier between. Each
//    invocation therefore owns both of its particles outright for the duration of the dispatch and writes them
//    back plainly — which is what lets the device form converge at the host form's rate rather than at the
//    halved rate of a Jacobi sweep with averaged corrections.

namespace Slate
{

[shader("compute")]
[numthreads(128, 1, 1)]
void main(Unsigned32x3 Dispatched : SV_DispatchThreadID)
{
    if (Dispatched.x >= Step.CordCount)
        return;

    const Unsigned32     CordIndex = Step.CordOffset + Dispatched.x;
    const TyreCordRecord Cord      = Cords[CordIndex];

    TyreParticleRecord A = Particles[Cord.ParticleA];
    TyreParticleRecord B = Particles[Cord.ParticleB];

    // 📝 λ starts at zero on the substep's first sweep and carries across the sweeps of one substep only —
    //    which is XPBD as published. The tension a cord holds at rest is in its strain, not in a carried λ: see
    //    the pre-stress note in `TyreSolver.cpp`.
    Real32 Multiplier = Step.IterationIndex == 0u ? 0.0f : CordMultipliers[CordIndex];

    Real32 CorrectionAX = 0.0f;
    Real32 CorrectionAY = 0.0f;
    Real32 CorrectionAZ = 0.0f;
    Real32 CorrectionBX = 0.0f;
    Real32 CorrectionBY = 0.0f;
    Real32 CorrectionBZ = 0.0f;

    ProjectCord(A.X, A.Y, A.Z, A.InverseMass,
                B.X, B.Y, B.Z, B.InverseMass,
                Cord.RestLength, Cord.Compliance, Step.SubstepSeconds,
                Multiplier,
                CorrectionAX, CorrectionAY, CorrectionAZ,
                CorrectionBX, CorrectionBY, CorrectionBZ);

    CordMultipliers[CordIndex] = Multiplier;

    if (A.InverseMass > 0.0f)
    {
        A.X += CorrectionAX;
        A.Y += CorrectionAY;
        A.Z += CorrectionAZ;
        Particles[Cord.ParticleA] = A;
    }

    if (B.InverseMass > 0.0f)
    {
        B.X += CorrectionBX;
        B.Y += CorrectionBY;
        B.Z += CorrectionBZ;
        Particles[Cord.ParticleB] = B;
    }
}

}   // namespace Slate
