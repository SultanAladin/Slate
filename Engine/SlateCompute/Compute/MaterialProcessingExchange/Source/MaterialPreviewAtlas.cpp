//============================================================================================================================================
//                                                    MATERIALPREVIEWATLAS.CPP
//============================================================================================================================================

#include "SlateCompute/Compute/MaterialProcessingExchange/Api/MaterialPreviewAtlas.h"

namespace Slate
{
Outcome<MaterialPreviewTile> MaterialPreviewAtlas::Reserve(std::uint32_t MaterialIndex, std::uint32_t Revision,
                                                           std::uint64_t Fingerprint)
{
    for (MaterialPreviewTile& Current : Tiles)
    {
        if (!Current.Active || Current.MaterialIndex != MaterialIndex) continue;
        if (Current.Revision != Revision || Current.Fingerprint != Fingerprint)
        {
            Current.Revision = Revision;
            Current.Fingerprint = Fingerprint;
            Current.State = MaterialPreviewState::BakeOwed;
        }
        return Outcome<MaterialPreviewTile>::Result(Current);
    }

    const std::uint64_t Slot = Tiles.size();
    if (Slot > 0xFFFFFFFFu)
    {
        return Outcome<MaterialPreviewTile>::Refuse(
            { RefusalReason::ExtentExhausted, "the material preview atlas tile ordinal cannot be represented" });
    }

    MaterialPreviewTile Produced;
    Produced.MaterialIndex = MaterialIndex;
    Produced.Revision = Revision;
    Produced.AtlasIndex = static_cast<std::uint32_t>(Slot / TilesPerAtlas);
    Produced.TileIndex = static_cast<std::uint32_t>(Slot % TilesPerAtlas);
    Produced.Fingerprint = Fingerprint;
    Produced.State = MaterialPreviewState::BakeOwed;
    Produced.Active = true;
    Tiles.push_back(Produced);
    return Outcome<MaterialPreviewTile>::Result(Produced);
}

Outcome<MaterialPreviewTile> MaterialPreviewAtlas::Resolve(std::uint32_t MaterialIndex) const
{
    for (const MaterialPreviewTile& Current : Tiles)
        if (Current.Active && Current.MaterialIndex == MaterialIndex) return Outcome<MaterialPreviewTile>::Result(Current);

    return Outcome<MaterialPreviewTile>::Refuse(
        { RefusalReason::ContentUnsupported, "the material has no allocated preview atlas tile" });
}

Outcome<MaterialPreviewTile> MaterialPreviewAtlas::MarkBaked(std::uint32_t MaterialIndex,
                                                              std::uint64_t Fingerprint)
{
    for (MaterialPreviewTile& Current : Tiles)
    {
        if (!Current.Active || Current.MaterialIndex != MaterialIndex) continue;
        if (Current.Fingerprint != Fingerprint)
        {
            return Outcome<MaterialPreviewTile>::Refuse(
                { RefusalReason::IdentityStale, "the preview bake completed for an older material revision" });
        }

        Current.BakedFingerprint = Fingerprint;
        Current.State = MaterialPreviewState::Ready;
        return Outcome<MaterialPreviewTile>::Result(Current);
    }

    return Outcome<MaterialPreviewTile>::Refuse(
        { RefusalReason::ContentUnsupported, "the material has no allocated preview atlas tile" });
}

void MaterialPreviewAtlas::Retire(std::uint32_t MaterialIndex)
{
    for (auto Current = Tiles.begin(); Current != Tiles.end(); ++Current)
    {
        if (Current->Active && Current->MaterialIndex == MaterialIndex)
        {
            Current->Active = false;
            return;
        }
    }
}

std::uint32_t MaterialPreviewAtlas::DeclaredCount() const
{
    std::uint32_t Count = 0u;
    for (const MaterialPreviewTile& Current : Tiles) if (Current.Active) ++Count;
    return Count;
}

} // namespace Slate
