//============================================================================================================================================
//                                                        DEPTHREDUCTIONLEVEL.SLANG
//============================================================================================================================================
// 🧩 ② of `16` §2 — one level of the hierarchical minimum, reduced from the depth target or from the level above it.

#include "SlateCompute/Compute/VisibilityIndex/Shader/OcclusionUniform.slang"

// 📝 🔴 One entry point, dispatched once per level, rather than one entry point per level. The level being written
//    arrives in the uniform, so a chain of any depth is walked by the same lowered stream — and `ReductionLevelCeiling`
//    stops being a count of programs that have to exist.
//
// 🔴 The whole reduction is a **minimum** and never a maximum. `Contract/`'s depth is reversed, so the least
//    ordinate across a region is its furthest point and a partition nearer than that cannot be occluded by it.

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

// ⚠️ The three are declared in the set order the host declares them — the uniform at nought, the depth target at
//    one and the chain at two. A span moved here without its declaration moving with it is read as another.
//
// 📝 The depth target is a sampled image rather than a storage one, because `08` §2 claims it as a depth
//    attachment and the vendor admits no storage usage on a depth format. It is read by texel ordinal through
//    `Load`, which needs no sampler and takes no filtering — a filtered read of a reduction is an average, and
//    an averaged conservative bound is not a bound.
ConstantBuffer<ReductionUniform>  Reducing        ;
Texture2D<Real32>                 DepthTarget     ;
RWStructuredBuffer<Real32>        ReducedChain    ;

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE REDUCTION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Reduces one texel of one level from the two-by-two region above it.
/// in    TexelOrdinal  [-]  the dispatch ordinal; ordinals outside the written level write nothing
/// note  📐 Halving rounds **up**, so the source extent is odd wherever the written one doubles past it. The
///         second texel of each pair is therefore clamped rather than read unconditionally: nine texels reduce to
///         five, and the fifth written texel reads the ninth source texel twice instead of reading a tenth that
///         does not exist. Reading past the level is a read of the next level's first row, which is depth from a
///         different extent entirely.
/// note  🔴 A texel the raster never wrote carries the clear, which is `FarPlaneDepth` — nought under the
///         reversed convention. That is the correct identity for a minimum over reversed depth: an unwritten
///         region occludes nothing, and nought is below every ordinate a surface can carry.
/// note  ⚠️ The finest level reads the depth **target** and every coarser level reads the chain. Both paths
///         reduce the same two-by-two region by the same minimum; what differs is only where the four ordinates
///         come from, which is why the two are one entry point rather than two that could drift apart.
/// cost  🚩
/// tag   api, nonallocating, nonthrowing
[shader("compute")]
[numthreads(8, 8, 1)]
void ReduceDepthLevel(Unsigned32x3 TexelOrdinal : SV_DispatchThreadID)
{
    const Unsigned32 Along  = TexelOrdinal.x;
    const Unsigned32 Across = TexelOrdinal.y;

    if (Along >= Reducing.WrittenAlong || Across >= Reducing.WrittenAcross)
    {
        return;
    }

    // 📐 The two-by-two region of the source this texel covers. The second ordinate of each pair is clamped into
    //    the source extent, which is what makes an odd source reduce without reading past its own last column.
    const Unsigned32 FirstAlong  = Along  * 2u;
    const Unsigned32 FirstAcross = Across * 2u;

    const Unsigned32 NextAlong  = FirstAlong  + 1u < Reducing.SourceAlong  ? FirstAlong  + 1u : FirstAlong;
    const Unsigned32 NextAcross = FirstAcross + 1u < Reducing.SourceAcross ? FirstAcross + 1u : FirstAcross;

    Real32 Corner00 = 0.0f;
    Real32 Corner10 = 0.0f;
    Real32 Corner01 = 0.0f;
    Real32 Corner11 = 0.0f;

    if (Reducing.SourceFromTarget != 0u)
    {
        // 📝 A wholly clamped read rather than a bounds test, because the clamping above already holds every
        //    ordinate inside the target — the finest level's extent is the display's and the source is that
        //    same extent.
        Corner00 = DepthTarget.Load(Unsigned32x3(FirstAlong, FirstAcross, 0u)).x;
        Corner10 = DepthTarget.Load(Unsigned32x3(NextAlong,  FirstAcross, 0u)).x;
        Corner01 = DepthTarget.Load(Unsigned32x3(FirstAlong, NextAcross,  0u)).x;
        Corner11 = DepthTarget.Load(Unsigned32x3(NextAlong,  NextAcross,  0u)).x;
    }
    else
    {
        Corner00 = ReducedChain[ProjectChainEntry(Reducing.SourceOffset, Reducing.SourceAlong, FirstAlong, FirstAcross)];
        Corner10 = ReducedChain[ProjectChainEntry(Reducing.SourceOffset, Reducing.SourceAlong, NextAlong,  FirstAcross)];
        Corner01 = ReducedChain[ProjectChainEntry(Reducing.SourceOffset, Reducing.SourceAlong, FirstAlong, NextAcross)];
        Corner11 = ReducedChain[ProjectChainEntry(Reducing.SourceOffset, Reducing.SourceAlong, NextAlong,  NextAcross)];
    }

    const Real32 LeastLower = Corner00 < Corner10 ? Corner00 : Corner10;
    const Real32 LeastUpper = Corner01 < Corner11 ? Corner01 : Corner11;
    const Real32 Reduced    = LeastLower < LeastUpper ? LeastLower : LeastUpper;

    ReducedChain[ProjectChainEntry(Reducing.WrittenOffset, Reducing.WrittenAlong, Along, Across)] = Reduced;
}

}   // namespace Slate
