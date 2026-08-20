//============================================================================================================================================
//                                                            STROKESPACE.CPP
//============================================================================================================================================
// 🧩 Sparse tile claiming over the dense cell index, and the commutative coverage accumulation.

#include "SlateCompute/Compute/StrokeSpace/Api/StrokeSpace.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

void StrokeSpace::Construct()
{
    TileOfCell.assign(CellOrdinalSpan, AbsentTile);

    ReservedCells.clear();
    Reserved.clear();

    TouchedTexels = 0u;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    CLAIM AND LOCATE
//------------------------------------------------------------------------------------------------------------------------

Outcome<std::uint32_t> StrokeSpace::Reserve(std::uint32_t CellOrdinal)
{
    if (TileOfCell.empty())
        Construct();

    if (CellOrdinal >= CellOrdinalSpan)
        return Outcome<std::uint32_t>::Refuse({ RefusalReason::ContentUnsupported, "no such cell" });

    if (TileOfCell[CellOrdinal] != AbsentTile)
        return Outcome<std::uint32_t>::Result(TileOfCell[CellOrdinal]);

    if (Reserved.size() >= CoverageTileCeiling)
    {
        return Outcome<std::uint32_t>::Refuse(
            { RefusalReason::ExtentExhausted, "the stroke touched more cells than the accumulation holds" });
    }

    const std::uint32_t TileOrdinal = static_cast<std::uint32_t>(Reserved.size());

    // 📝 Zeroed on claim rather than on reclaim. A stroke that touches four cells and is abandoned pays for four
    //    tiles; zeroing at reclaim would pay for whatever the previous stroke touched as well.
    Reserved.push_back(std::vector<float>(static_cast<std::size_t>(CoverageTileTexels) * CoverageTileTexels, 0.0f));
    ReservedCells.push_back(CellOrdinal);

    TileOfCell[CellOrdinal] = TileOrdinal;

    return Outcome<std::uint32_t>::Result(TileOrdinal);
}

Outcome<std::uint32_t> StrokeSpace::Located(std::uint32_t CellOrdinal) const
{
    if (CellOrdinal >= TileOfCell.size() || TileOfCell[CellOrdinal] == AbsentTile)
        return Outcome<std::uint32_t>::Refuse({ RefusalReason::ExtentExhausted, "the stroke has not touched it" });

    return Outcome<std::uint32_t>::Result(TileOfCell[CellOrdinal]);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    ACCUMULATION
//------------------------------------------------------------------------------------------------------------------------

void StrokeSpace::Accumulate(std::uint32_t TileOrdinal, std::uint32_t X, std::uint32_t Y, double Incoming)
{
    if (TileOrdinal >= Reserved.size() || X >= CoverageTileTexels || Y >= CoverageTileTexels)
        return;

    if (Incoming <= 0.0)
        return;

    const std::size_t Writing = static_cast<std::size_t>(Y) * CoverageTileTexels + X;

    const double Current = static_cast<double>(Reserved[TileOrdinal][Writing]);

    if (Current <= 0.0)
        ++TouchedTexels;

    // 🔴 `22` §3's within-stroke rule, and the one line that decides it. `Over` saturates toward unity and is
    //    symmetric in its operands; addition neither saturates nor stays inside the interval the apply reads.
    const double Combined = CombineCoverage(CombineSpecification::Over,
                                            Current,
                                            Incoming > 1.0 ? 1.0 : Incoming);

    Reserved[TileOrdinal][Writing] = static_cast<float>(Combined > 1.0 ? 1.0 : Combined);
}

double StrokeSpace::Coverage(std::uint32_t TileOrdinal, std::uint32_t X, std::uint32_t Y) const
{
    if (TileOrdinal >= Reserved.size() || X >= CoverageTileTexels || Y >= CoverageTileTexels)
        return 0.0;

    return static_cast<double>(Reserved[TileOrdinal][static_cast<std::size_t>(Y) * CoverageTileTexels + X]);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

const std::vector<std::uint32_t>& StrokeSpace::TouchedCells() const { return ReservedCells; }

std::uint32_t StrokeSpace::ReservedCount() const
{
    return static_cast<std::uint32_t>(Reserved.size());
}

std::uint64_t StrokeSpace::TouchedTexelCount() const { return TouchedTexels; }

void StrokeSpace::Reclaim()
{
    for (const std::uint32_t CellOrdinal : ReservedCells)
    {
        if (CellOrdinal < TileOfCell.size())
            TileOfCell[CellOrdinal] = AbsentTile;
    }

    ReservedCells.clear();
    Reserved.clear();

    TouchedTexels = 0u;
}

}   // namespace Slate
