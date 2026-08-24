//============================================================================================================================================
//                                                     MESHINSTANCECOMPONENT.H
//============================================================================================================================================

#pragma once

#include "Foundation/Identity.h"

#include <cstdint>
#include <vector>

namespace Slate
{

struct MeshInstanceComponent
{
    GeometryIdentity Geometry = {};
    bool Visible = true;
    bool CastShadows = true;
    bool ReceiveShadows = true;
};

struct MaterialAssignmentComponent
{
    std::vector<std::uint32_t> MaterialBySlot = {};
};

struct SourceProvenanceComponent
{
    std::uint64_t ContentHash = 0u;
    std::uint32_t FormatIdentity = 0u;
    bool UnitAssumed = false;
};

} // namespace Slate
