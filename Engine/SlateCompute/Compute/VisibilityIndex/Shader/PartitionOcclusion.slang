//============================================================================================================================================
//                                                        PARTITIONOCCLUSION.SLANG
//============================================================================================================================================
// 🧩 ① and ③ of `16` §2 — one partition tested against the reduction, and the survivors compacted into the draw's own ordinals.

#include "SlateCompute/Compute/VisibilityIndex/Shader/OcclusionUniform.slang"

// 📝 🔴 One entry point for **both** phases. What distinguishes them is which reduction the chain holds — the
//    previous rotation's in ① and this rotation's in ③ — and which partitions are dispatched. Neither is a
//    property of the arithmetic, so a second entry point would be this one copied with its comparison intact,
//    and the copy is what eventually disagrees about the direction of a reversed comparison.
//
// 🔴 The survivors are **compacted**, not marked. `16` §2 rasterises what passes and the draw is recorded from
//    the device, so what the phase must produce is a contiguous run of surviving ordinals and a count — a marked
//    span would still need a host readback to know how far to draw, which is the wait the two phases exist to
//    avoid.
//
// 🔴 What is compacted is the surviving **triangles** and not the surviving partitions. A partition carries
//    `PartitionTriangleFloor` to `PartitionTriangleCeiling` triangles, so a span of partition ordinals is one the
//    draw cannot read by corner ordinal — three corners of a partition are three corners of its first triangle
//    and the remaining hundred-odd are unreachable. Each surviving partition therefore writes its own triangle
//    run out, and the corner count the record names is three times what was written.

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

// ⚠️ Declared in the set order the host declares them — the uniform at nought, then the classified partitions,
//    the level extents, the chain, the surviving triangles and the record the draw is issued from.
//
// 📝 The level extents arrive as a span rather than as uniform members because the chain carries up to
//    `ReductionLevelCeiling` of them and a uniform block sized to the ceiling would be mostly unread. The span
//    carries three ordinals per level — where it begins and how far it runs on both axes.
ConstantBuffer<OcclusionUniform>      Testing             ;
StructuredBuffer<TestedPartition>     Classified          ;
StructuredBuffer<Unsigned32>          LevelExtents        ;
StructuredBuffer<Real32>              ReducedChain        ;
RWStructuredBuffer<Unsigned32>        SurvivingTriangles  ;
RWStructuredBuffer<Unsigned32>        IssuedRecord        ;

// 📝 Three ordinals per level, in the order the host writes them. Spelled once here because both reads below
//    index against it, and a stride disagreeing with the host's write reads one level's offset as another's
//    extent — which selects a level that exists and reads it at the wrong stride.
static const Unsigned32 LevelExtentStride = 3u;   // [-] - offset, extent along, extent across

// 📝 🔴 The record the indirect draw reads, as the vendor lays `VkDrawIndirectCommand` out: the corner count,
//    the instance count, the first corner and the first instance, in that order and in that width. The count of
//    surviving triangles is not among them — it is the corner count divided by three, which is why the corner
//    count is what the atomic advances and why it advances in multiples of three.
static const Unsigned32 RecordCornerCount   = 0u;   // [-] - corners the draw issues
static const Unsigned32 RecordInstanceCount = 1u;   // [-] - always one
static const Unsigned32 RecordFirstCorner   = 2u;   // [-] - always nought
static const Unsigned32 RecordFirstInstance = 3u;   // [-] - always nought

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PREDICATE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Tests one classified partition against the reduction and compacts it if it survives.
/// in    PartitionEntry  [-]  the dispatch ordinal; ordinals past the classified count test nothing
/// note  📐 🔴 The comparison is `NearestDepth >= ReducedDepth` and the direction is the whole of the mechanism.
///         Depth is reversed, so a partition survives when its nearest corner is at least as near as the
///         furthest surface already recorded where it projects. Written the other way round it admits exactly
///         the partitions that are hidden and rejects the ones that are not, and the image is the far side of
///         every object.
/// note  🔴 The comparison is **not strict**. A partition whose nearest ordinate exactly equals the reduction is
///         admitted, because the reduction is a conservative bound rather than a measurement — equality is where
///         the partition lies precisely on the recorded surface, and rejecting it drops the geometry that is
///         coplanar with what is already there.
/// note  📝 The level is selected from the partition's own projected extent, halving until a two-by-two reading
///         spans it. It is the same integer halving `DepthReduction::LevelOfExtent` performs on the host; it runs
///         here because the extent is the device's operand and a host selection would need the classified span
///         read back before the dispatch that consumes it.
/// note  ⚠️ A partition projecting to nothing survives. It is sub-pixel, `16` §3 routes it to the compute raster,
///         and a reduction read at a zero extent tests it against the depth of a region it does not cover.
/// cost  🚩
/// tag   api, nonallocating, nonthrowing
[shader("compute")]
[numthreads(64, 1, 1)]
void TestPartitionOcclusion(Unsigned32x3 PartitionOrdinal : SV_DispatchThreadID)
{
    const Unsigned32 PartitionEntry = PartitionOrdinal.x;

    if (PartitionEntry >= Testing.ClassifiedCount)
    {
        return;
    }

    const TestedPartition Tested = Classified[PartitionEntry];

    // 📐 The level whose two-by-two reading spans the projected extent, found by the same halving the chain was
    //    built with. Both round up, so the count arrived at is exactly how many texels of that level the extent
    //    spans — which is what makes two by two a terminator rather than an approximation of one.
    Unsigned32 RemainingAlong  = Tested.ProjectedAlong;
    Unsigned32 RemainingAcross = Tested.ProjectedAcross;
    Unsigned32 Selected        = 0u;

    const Unsigned32 Coarsest = Testing.LevelCount - 1u;

    while ((RemainingAlong > 2u || RemainingAcross > 2u) && Selected < Coarsest)
    {
        RemainingAlong  = (RemainingAlong  + 1u) / 2u;
        RemainingAcross = (RemainingAcross + 1u) / 2u;
        Selected       += 1u;
    }

    // 📐 The origin is carried into the selected level by the same halving. Shifting by the level ordinal is the
    //    same arithmetic only where every extent halved evenly; the chain rounds up, so the origin is halved
    //    alongside the extent rather than derived from a shift that disagrees at the first odd level.
    Unsigned32 ReadAlong  = Tested.OriginAlong;
    Unsigned32 ReadAcross = Tested.OriginAcross;

    for (Unsigned32 Walked = 0u; Walked < Selected; ++Walked)
    {
        ReadAlong  = ReadAlong  / 2u;
        ReadAcross = ReadAcross / 2u;
    }

    const Unsigned32 LevelEntry  = Selected * LevelExtentStride;
    const Unsigned32 LevelOffset = LevelExtents[LevelEntry];
    const Unsigned32 LevelAlong  = LevelExtents[LevelEntry + 1u];
    const Unsigned32 LevelAcross = LevelExtents[LevelEntry + 2u];

    Unsigned32 Survives = 1u;

    if (Tested.ProjectedAlong != 0u && Tested.ProjectedAcross != 0u && LevelAlong != 0u && LevelAcross != 0u)
    {
        const Real32 Reduced = ResolveReducedDepth(ReducedChain, LevelOffset, LevelAlong, LevelAcross,
                                                  ReadAlong, ReadAcross);

        Survives = Tested.NearestDepth >= Reduced ? 1u : 0u;
    }

    if (Survives == 0u)
    {
        return;
    }

    // 🔴 The corner count is what the atomic advances, and it advances by the partition's **own** corner span —
    //    three per triangle it carries. Advancing a separate partition count and multiplying afterwards needs a
    //    second dispatch to do the multiplication, and until that dispatch ran the record would name a corner
    //    count of nought, which is a draw that issues nothing.
    //
    // 📐 One atomic per surviving partition and not one per triangle. The run a partition claims is contiguous
    //    because the addition is atomic, so the entries it writes are its own and no two partitions interleave —
    //    which is what lets a triangle be written by an ordinary store below rather than by an atomic each.
    const Unsigned32 ClaimedCorners = Tested.TriangleCount * 3u;

    if (ClaimedCorners == 0u)
    {
        return;
    }

    Unsigned32 Reached = 0u;
    InterlockedAdd(IssuedRecord[RecordCornerCount], ClaimedCorners, Reached);

    const Unsigned32 FirstEntry = Reached / 3u;

    // ⚠️ Bounded against the claim rather than trusted. The atomic hands back an entry past the span's end as
    //    readily as one inside it, and a storage write past a claim is undefined rather than refused — so the run
    //    is dropped where it would not fit. The corner count the record names is then longer than what was
    //    written, which the host refuses at the readback rather than the device drawing entries it never wrote.
    if (FirstEntry + Tested.TriangleCount > Testing.TriangleCeiling)
    {
        return;
    }

    for (Unsigned32 Walked = 0u; Walked < Tested.TriangleCount; ++Walked)
    {
        SurvivingTriangles[FirstEntry + Walked] = Tested.FirstTriangle + Walked;
    }
}

}   // namespace Slate
