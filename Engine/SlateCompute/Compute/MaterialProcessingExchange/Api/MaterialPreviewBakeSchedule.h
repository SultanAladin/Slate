//============================================================================================================================================
//                                                  MATERIALPREVIEWBAKESCHEDULE.H
//============================================================================================================================================
// 🧩 Immutable bake work derived from a compiled material snapshot. Device command recording consumes this later.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "SlateCompute/Compute/MaterialProcessingExchange/Api/MaterialPreviewAtlas.h"
#include "SlateCompute/Compute/MaterialProcessingExchange/Api/MaterialProcessingExchange.h"

#include <cstdint>

namespace Slate
{

/// 🧩 One exact-at-submission preview bake. It borrows no mutable document state.
struct MaterialPreviewBakeJob
{
    MaterialPreviewTile Tile = {};
    PhysicalSurfacePacket Physical = {};
    std::uint64_t Fingerprint = 0u;
};

/// 🧩 Forms a GPU-bake-ready job only for a stale tile and a resolved physical packet.
class MaterialPreviewBakeSchedule
{
public:
    Outcome<MaterialPreviewBakeJob> Schedule(const MaterialPreviewTile& Tile,
                                             const MaterialProcessingSnapshot& Snapshot) const;
};

} // namespace Slate
