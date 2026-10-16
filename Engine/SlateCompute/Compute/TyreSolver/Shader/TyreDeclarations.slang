//============================================================================================================================================
//                                                          TYREDECLARATIONS.SLANG
//============================================================================================================================================
// 🧩 The one descriptor set every tyre entry point reads, and the lattice arithmetic they share.

#pragma once

#include "Shared/TyreConstraintProjection.slang.h"

// 📝 🔴 Declared once and included by all six entry points, for `AtmosphereUniform.slang`'s reason: a set spelled
//    separately per entry point is the same layout written five times, and a slot inserted into four of them is
//    not a compilation failure — it is one entry point reading another's span.

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE DESCRIPTOR SET
//------------------------------------------------------------------------------------------------------------------------

[[vk::binding(0, 0)]] RWStructuredBuffer<TyreParticleRecord>  Particles;              // [-] - positions, one per particle
[[vk::binding(1, 0)]] RWStructuredBuffer<TyreMotionRecord>    Motions;                // [-] - substep start and λₙ
[[vk::binding(2, 0)]] RWStructuredBuffer<TyreVelocityRecord>  Velocities;             // [-] - velocities and ‖λₜ‖
[[vk::binding(3, 0)]] StructuredBuffer<TyreRestRecord>        Rests;                  // [-] - rim-frame rest, tyre index
[[vk::binding(4, 0)]] RWStructuredBuffer<TyreGradientRecord>  Gradients;              // [-] - the three shape gradients
[[vk::binding(5, 0)]] StructuredBuffer<TyreCordRecord>        Cords;                  // [-] - every cord, colour-sorted
[[vk::binding(6, 0)]] RWStructuredBuffer<Real32>              CordMultipliers;        // [-] - λ per cord, this substep
[[vk::binding(7, 0)]] StructuredBuffer<TyreHubRecord>         Hubs;                   // [-] - one per tyre, this frame
[[vk::binding(8, 0)]] RWStructuredBuffer<Atomic<Signed32>>    Accumulators;           // [-] - TyreAccumulatorStride per tyre
[[vk::binding(9, 0)]] StructuredBuffer<TyreShapeRecord>       ShapeRows;              // [-] - ShapeRowLimit per tyre
[[vk::binding(10, 0)]] RWStructuredBuffer<Atomic<Signed32>>   ShapeSums;              // [-] - ShapeSumStride per tyre
[[vk::binding(11, 0)]] RWStructuredBuffer<Real32>             ShapeMultipliers;       // [-] - ShapeRowLimit per tyre; λ per row
[[vk::binding(12, 0)]] RWStructuredBuffer<Real32>             ShapeAdvances;          // [-] - ShapeRowLimit per tyre; Δλ per row
[[vk::binding(13, 0)]] RWStructuredBuffer<Real32>             ShapeMatrix;            // [-] - ShapeMatrixStride per tyre; factor scratch

// 📝 🔴 The two integer spans are declared `Atomic<Signed32>` rather than reached through `InterlockedAdd`, because
//    the WebGPU target admits no atomic on a plain integer; the spelling below is the one every target accepts.
//    The step block is a push constant on Vulkan and, on WebGPU, a uniform the browser preview binds at slot 14.
#if defined(SLATE_WEBGPU_TOOLCHAIN)
[[vk::binding(14, 0)]] ConstantBuffer<TyreStepPush>             Step;                   // [-] - one block per dispatch
#else
[[vk::push_constant]] ConstantBuffer<TyreStepPush>            Step;                   // [-] - pushed before every dispatch
#endif

// 📝 `TyreGroupExtent` of one hundred and twenty-eight comes from the shared header, since the host sizes the shape
//    solve's dispatch to it. A tyre of sixteen rings by sixty-four segments is 1024 particles, so eight groups per
//    tyre, and a cord colour of a few hundred is two or three. Larger groups leave most of the device idle on a
//    four-tyre run; smaller ones pay the dispatch's fixed cost more often.

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE LATTICE POSITION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Splits one particle index into the tyre, the ring and the segment it names.
/// cost  ✔️
/// tag   nonallocating, nonthrowing
void ResolveLatticePosition(Unsigned32 Particle, out Unsigned32 Tyre, out Unsigned32 Ring, out Unsigned32 Segment)
{
    const Unsigned32 PerTyre = Step.Rings * Step.Segments;

    Tyre    = Particle / PerTyre;
    Ring    = (Particle - Tyre * PerTyre) / Step.Segments;
    Segment = Particle - Tyre * PerTyre - Ring * Step.Segments;
}

/// 🧩 The fraction of the frame this substep ends at, for the rim pose interpolation.
/// cost  ✔️
/// tag   nonallocating, nonthrowing
Real32 SubstepFraction()
{
    return Real32(Step.SubstepIndex + 1u) / Real32(Step.SubstepCount);
}

/// 🧩 The live order of the shape block — the volume, the rings and the meridians.
/// cost  ✔️
/// tag   nonallocating, nonthrowing
Unsigned32 ShapeOrder()
{
    return 1u + Step.Rings + Step.Segments;
}

/// 🧩 The first particle of a tyre — the one invocation per tyre that writes the tyre's own scalars.
/// cost  ✔️
/// tag   nonallocating, nonthrowing
bool TyreLeadingParticle(Unsigned32 Ring, Unsigned32 Segment)
{
    return Ring == 0u && Segment == 0u;
}

}   // namespace Slate
