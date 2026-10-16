//============================================================================================================================================
//                                                           TYRESHAPESOLVE.SLANG
//============================================================================================================================================
// 🧩 ③½ of the substep — one workgroup per tyre factors the shape block and delivers Δλ for every row.

#include "SlateCompute/Compute/TyreSolver/Shader/TyreDeclarations.slang"

// 📐 🔴 The block is (J M⁻¹ Jᵀ + α̃) Δλ = −C − α̃ λ over the volume, the rings and the meridians — of order
//    1 + Rings + Segments, eighty-one for the default lattice — and it is solved **directly**, by Cholesky,
//    because solving these three families one after another is the tug of war described in
//    `TyreConstraintProjection.slang.h`. Eighty-one is small: the factor is a third of 81³ multiply-adds,
//    a hundred and eighty thousand, which one workgroup does in the time a particle dispatch takes to launch.
//
// 📝 🔴 One workgroup per tyre, with the matrix in the tyre's own span of `ShapeMatrix` rather than in group
//    shared memory, which `06` does not negotiate beyond the sixteen kilobytes every device guarantees and which
//    a 96-row block would exceed. Each column's pivot is computed by every invocation redundantly — it is a dot
//    product of one row with itself — so no invocation waits on another for it; the column's entries below the
//    pivot are then split across the invocations, and a group barrier separates the columns. The host form in
//    `TyreSolver.cpp` loops the same arithmetic in the same order.

namespace Slate
{

[shader("compute")]
[numthreads(128, 1, 1)]
void main(Unsigned32x3 Grouped : SV_GroupID, Unsigned32x3 Local : SV_GroupThreadID)
{
    const Unsigned32 Tyre = Grouped.x;

    if (Tyre >= Step.TyreCount)
        return;

    const Unsigned32 Order   = ShapeOrder();
    const Unsigned32 Lane    = Local.x;
    const Unsigned32 Sums    = Tyre * ShapeSumStride;
    const Unsigned32 Values  = Sums + ShapeMatrixStride;
    const Unsigned32 Matrix  = Tyre * ShapeMatrixStride;
    const Unsigned32 Rows    = Tyre * ShapeRowLimit;
    const Real32     Substep = Step.SubstepSeconds;

    // 📝 Decode into the lower triangle, mirrored from the upper where the gather stored it, α̃ on the diagonal;
    //    the right-hand side −C − α̃λ into the advances, which the substitution overwrites in place.
    for (Unsigned32 Row = Lane; Row < Order; Row += TyreGroupExtent)
    {
        if (Step.IterationIndex == 0u)
            ShapeMultipliers[Rows + Row] = 0.0f;

        const TyreShapeRecord Record           = ShapeRows[Rows + Row];
        const Real32          ScaledCompliance = Record.Compliance / (Substep * Substep);

        for (Unsigned32 Column = 0u; Column <= Row; ++Column)
        {
            const Real32 Entry = DecodeFixed(ShapeSums[Sums + Column * ShapeRowLimit + Row].load(), ShapeEntryScale(Column, Row));
            ShapeMatrix[Matrix + Row * ShapeRowLimit + Column] = Entry + (Row == Column ? ScaledCompliance : 0.0f);
        }

        const Real32 Value = DecodeFixed(ShapeSums[Values + Row].load(), ShapeValueScale(Row));
        ShapeAdvances[Rows + Row] = -(Value - Record.RestValue) - ScaledCompliance * ShapeMultipliers[Rows + Row];
    }

    // 🔴 All memory, not group memory. The factor's scratch is a storage span, not groupshared, and the group
    //    barrier alone orders only the latter: on SPIR-V it carries workgroup memory semantics, and on WGSL it
    //    lowers to `workgroupBarrier()` with no `storageBarrier()` — a lane reading a column another lane just
    //    wrote would be reading a stale cache line on either target, correctly on one driver.
    AllMemoryBarrierWithGroupSync();

    // 📐 L Lᵀ = A, in place, column by column; a pivot that reads non-positive is a saturated sum and is clamped.
    for (Unsigned32 Column = 0u; Column < Order; ++Column)
    {
        Real32 Pivot = ShapeMatrix[Matrix + Column * ShapeRowLimit + Column];

        for (Unsigned32 Earlier = 0u; Earlier < Column; ++Earlier)
            Pivot -= ShapeMatrix[Matrix + Column * ShapeRowLimit + Earlier] * ShapeMatrix[Matrix + Column * ShapeRowLimit + Earlier];

        Pivot = Real32(SquareRoot(Real64(Pivot > 1.0e-30f ? Pivot : 1.0e-30f)));

        AllMemoryBarrierWithGroupSync();

        if (Lane == 0u)
            ShapeMatrix[Matrix + Column * ShapeRowLimit + Column] = Pivot;

        for (Unsigned32 Row = Column + 1u + Lane; Row < Order; Row += TyreGroupExtent)
        {
            Real32 Sum = ShapeMatrix[Matrix + Row * ShapeRowLimit + Column];

            for (Unsigned32 Earlier = 0u; Earlier < Column; ++Earlier)
                Sum -= ShapeMatrix[Matrix + Row * ShapeRowLimit + Earlier] * ShapeMatrix[Matrix + Column * ShapeRowLimit + Earlier];

            ShapeMatrix[Matrix + Row * ShapeRowLimit + Column] = Sum / Pivot;
        }

        AllMemoryBarrierWithGroupSync();
    }

    // 📐 L y = b forward, Lᵀ Δλ = y backward — sequential by nature, so one invocation does it.
    if (Lane == 0u)
    {
        for (Unsigned32 Row = 0u; Row < Order; ++Row)
        {
            Real32 Sum = ShapeAdvances[Rows + Row];

            for (Unsigned32 Earlier = 0u; Earlier < Row; ++Earlier)
                Sum -= ShapeMatrix[Matrix + Row * ShapeRowLimit + Earlier] * ShapeAdvances[Rows + Earlier];

            ShapeAdvances[Rows + Row] = Sum / ShapeMatrix[Matrix + Row * ShapeRowLimit + Row];
        }

        for (Unsigned32 Back = Order; Back > 0u; --Back)
        {
            const Unsigned32 Row = Back - 1u;
            Real32           Sum = ShapeAdvances[Rows + Row];

            for (Unsigned32 Later = Row + 1u; Later < Order; ++Later)
                Sum -= ShapeMatrix[Matrix + Later * ShapeRowLimit + Row] * ShapeAdvances[Rows + Later];

            ShapeAdvances[Rows + Row] = Sum / ShapeMatrix[Matrix + Row * ShapeRowLimit + Row];
        }
    }

    AllMemoryBarrierWithGroupSync();

    for (Unsigned32 Row = Lane; Row < Order; Row += TyreGroupExtent)
        ShapeMultipliers[Rows + Row] += ShapeAdvances[Rows + Row];

    // 📝 The sums are this workgroup's alone once read, so it zeroes them for the next pass itself.
    for (Unsigned32 Slot = Lane; Slot < ShapeSumStride; Slot += TyreGroupExtent)
        ShapeSums[Sums + Slot].store(0);
}

}   // namespace Slate
