//============================================================================================================================================
//                                                          OCCLUSIONUNIFORM.SLANG
//============================================================================================================================================
// 🧩 The two uniform blocks `16` §2's device half reads, the record one classified partition arrives as, and the two-by-two reading.

#pragma once

#include "Shared/Prelude.slang.h"
#include "Contract/ToleranceContract.h"

// 📝 🔴 Declared once, here, and included by the reduction entry point and the occlusion entry point both. The
//    reduction writes the chain and the occlusion reads it, so the two agree on where a level begins and how far
//    it runs; a level offset derived separately at each is the same arithmetic written twice, and the occlusion
//    then reads the reduction one level along and rejects whatever the coarser level happened to hold.
//
// 🔴 The chain is a **span** and not an image with reduction levels. `DepthReduction` halves by rounding **up**
//    and the vendor's own level extents halve by rounding down, so nine texels reduce to five here and to four
//    there. A chain claimed as an image would therefore disagree with the host's level count and with every
//    extent below the first odd ordinate — and the disagreement appears as a partition tested against a level
//    whose extent the host thinks is one texel wider than the device wrote.
//
// ⚠️ This file spells no entry point attribute **anywhere**, comments included. The build searches each `.slang`
//    for that attribute's opening to decide what is a translation, so a file naming it in prose is handed to the
//    toolchain and refused for exporting nothing.

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT IS REDUCED
//------------------------------------------------------------------------------------------------------------------------

// 📝 One workgroup edge, shared by both entry points. Sixty-four invocations is one or two vendor subgroups on
//    every implementation `06` scores, and the reduction is bound by its reads rather than by its arithmetic.
static const Unsigned32 OcclusionGroupEdge = 8u;   // [-] - invocations per workgroup edge

/// 🧩 Which level of the chain one reduction dispatch writes, and where the two levels it touches begin.
/// note  🔴 The **finest** level is written from the depth target and every coarser one from the level above it,
///        so the dispatch that writes level nought reads an image and the rest read the span. `SourceFromTarget`
///        is what distinguishes them, and it is declared rather than derived from `WrittenLevel` being nought —
///        the derivation holds only while the chain's finest level is also the display extent, which is a
///        condition this block cannot see.
/// note  📐 Both extents are carried because the source's is not derivable from the written one. Halving rounds
///        **up**, so a written extent of five admits a source of nine or of ten and the two read different
///        texels at the last column.
/// tag   contract
struct ReductionUniform
{
    Unsigned32  WrittenLevel     ;   // [-]     - which level the dispatch writes
    Unsigned32  WrittenOffset    ;   // [texel] - where that level begins in the chain span
    Unsigned32  WrittenAlong     ;   // [texel] - its extent
    Unsigned32  WrittenAcross    ;   // [texel]
    Unsigned32  SourceOffset     ;   // [texel] - where the level above it begins; unread when the target is read
    Unsigned32  SourceAlong      ;   // [texel] - the source's extent
    Unsigned32  SourceAcross     ;   // [texel]
    Unsigned32  SourceFromTarget ;   // [-]     - non-zero for the finest level, which reads the depth target
};

/// 🧩 What the occlusion test needs of the chain and of the recording it is testing for.
/// note  🔴 `LevelCount` is read to bound the level a partition's own extent selects. The host selects it and
///        the device clamps to this count rather than trusting it, because a level ordinal past the chain's end
///        addresses whatever the span holds beyond it — which is the next enrolment's finest level.
/// note  🔴 `TriangleCeiling` is what the compaction writes below and never past. The atomic hands back an entry
///        whether or not the span reaches it, so a run of survivors longer than the claim was sized for writes
///        outside the span — and the vendor's own bound on a storage write is undefined behaviour rather than a
///        refusal. The host sizes the claim to the residency's whole triangle run, so reaching this is the
///        arithmetic disagreeing rather than a scene being large.
/// tag   contract
struct OcclusionUniform
{
    Unsigned32  ClassifiedCount ;   // [-]  - partitions the dispatch tests
    Unsigned32  LevelCount      ;   // [-]  - levels the chain carries
    Unsigned32  DisplayAlong    ;   // [px] - the extent the finest level covers
    Unsigned32  DisplayAcross   ;   // [px]
    Unsigned32  TriangleCeiling ;   // [-]  - entries the surviving run can hold
    Unsigned32  Unoccupied0     ;   // [-]  - pads the block to an even word count; never read
    Unsigned32  Unoccupied1     ;   // [-]
    Unsigned32  Unoccupied2     ;   // [-]
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  ONE TESTED PARTITION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One classified partition as the device reads it — where it projects, how far, and how near it reaches.
/// note  🔴 The mirror of `ClassifiedPartition`, member for member and width for width. The host classifies
///        against the frustum and the cone and this is what crosses; a member inserted on one side alone is a
///        span the device walks at the wrong stride, and every ordinal past the first then belongs to another
///        partition.
/// note  📝 `Standing` arrives as the host wrote it and is **not** read by the occlusion test. What the test
///        writes is the survival word below, so a partition the host already excluded is never dispatched at
///        all — the standing rides along for the readback `86` reports rather than for the comparison.
/// note  🔴 The **triangle run** is carried because the compaction writes one entry per surviving triangle and
///        not one per surviving partition. A partition carries `PartitionTriangleFloor` to
///        `PartitionTriangleCeiling` of them — and more for a lone wide face — so a record naming the partition
///        alone leaves the corner count to be recovered from a triangle count nothing on that path holds.
/// note  📐 `FirstTriangle` addresses the residency's own fanned run and never the document's. The occlusion
///        dispatch is per residency, because the draw it records is: one residency is one placement, one uniform
///        and one position span, and a compaction laid across two of them would draw the second at the first's
///        placement.
/// tag   contract
struct TestedPartition
{
    Unsigned32  PartitionOrdinal ;   // [-]     - within the enrolment
    Unsigned32  Standing         ;   // [-]     - as the host classified it
    Unsigned32  FirstTriangle    ;   // [-]     - into the residency's fanned triangle run
    Unsigned32  TriangleCount    ;   // [-]     - triangles of that run the partition spans
    Unsigned32  OriginAlong      ;   // [texel] - the first display texel it covers
    Unsigned32  OriginAcross     ;   // [texel]
    Unsigned32  ProjectedAlong   ;   // [texel] - conservative outward
    Unsigned32  ProjectedAcross  ;   // [texel]
    Real32      NearestDepth     ;   // [-]     - reversed; the greatest ordinate of its eight corners
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE CHAIN READING
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Where one texel of one level sits in the chain span.
/// in    LevelOffset  [texel]  where the level begins
/// in    LevelAlong   [texel]  the level's first extent, which is its stride
/// in    Along        [texel]  the texel, already inside the level
/// in    Across       [texel]
/// out   Entry        [-]      the span entry that texel occupies
/// note  📐 Row-major with the level's own extent as the stride, which is what makes a level contiguous and its
///         offset one addition. The stride is the level's extent rather than the display's — a chain strided by
///         the display would leave every level but the finest holding one row of depth and the rest unwritten.
/// cost  ✔️
/// tag   nonallocating, nonthrowing
Unsigned32 ProjectChainEntry(Unsigned32 LevelOffset, Unsigned32 LevelAlong, Unsigned32 Along, Unsigned32 Across)
{
    return LevelOffset + Across * LevelAlong + Along;
}

/// 🧩 The conservative depth over the two-by-two reading one partition is tested against.
/// in    Reduced      [-]      the chain, as the reduction wrote it
/// in    LevelOffset  [texel]  where the selected level begins
/// in    LevelAlong   [texel]  the selected level's extent
/// in    LevelAcross  [texel]
/// in    Along        [texel]  the partition's origin, carried into the selected level
/// in    Across       [texel]
/// out   Reached      [-]      the least ordinate across the four texels, which is the furthest of them
/// note  📐 🔴 The **minimum**, because depth is reversed — `NearPlaneDepth` is unity and `FarPlaneDepth` is
///         nought, so the least ordinate over a region is its furthest point. A partition nearer than that
///         cannot be hidden by anything already recorded there. Taking the maximum instead is correct wherever
///         the surface is flat and rejects visible geometry at every silhouette, which reaches the artist as an
///         object flickering along its own edge rather than as a comparison written the wrong way round.
/// note  ⚠️ The reading is clamped into the level rather than wrapped. A partition whose origin sits on the
///         level's last column reads that column twice, which is conservative; wrapping would read the first
///         column of the next row and compare against depth recorded at the opposite edge of the display.
/// cost  ✔️
/// tag   nonallocating, nonthrowing
Real32 ResolveReducedDepth(StructuredBuffer<Real32>  Reduced,
                           Unsigned32                LevelOffset,
                           Unsigned32                LevelAlong,
                           Unsigned32                LevelAcross,
                           Unsigned32                Along,
                           Unsigned32                Across)
{
    const Unsigned32 FirstAlong  = Along  < LevelAlong  ? Along  : LevelAlong  - 1u;
    const Unsigned32 FirstAcross = Across < LevelAcross ? Across : LevelAcross - 1u;

    const Unsigned32 NextAlong  = FirstAlong  + 1u < LevelAlong  ? FirstAlong  + 1u : FirstAlong;
    const Unsigned32 NextAcross = FirstAcross + 1u < LevelAcross ? FirstAcross + 1u : FirstAcross;

    const Real32 Corner00 = Reduced[ProjectChainEntry(LevelOffset, LevelAlong, FirstAlong, FirstAcross)];
    const Real32 Corner10 = Reduced[ProjectChainEntry(LevelOffset, LevelAlong, NextAlong,  FirstAcross)];
    const Real32 Corner01 = Reduced[ProjectChainEntry(LevelOffset, LevelAlong, FirstAlong, NextAcross)];
    const Real32 Corner11 = Reduced[ProjectChainEntry(LevelOffset, LevelAlong, NextAlong,  NextAcross)];

    const Real32 LeastLower = Corner00 < Corner10 ? Corner00 : Corner10;
    const Real32 LeastUpper = Corner01 < Corner11 ? Corner01 : Corner11;

    return LeastLower < LeastUpper ? LeastLower : LeastUpper;
}

}   // namespace Slate
