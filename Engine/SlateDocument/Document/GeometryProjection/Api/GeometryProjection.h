//============================================================================================================================================
//                                                       GEOMETRYPROJECTION.H
//============================================================================================================================================
// 🧩 Exact-solid projection into the standing polygon document topology. This adapter belongs outside
//    SlateGeometry so the exact kernel remains independent of the current polygon, paint and render pipeline.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "SlateDocument/Document/TopologyStructure/Api/TopologyStructure.h"
#include "SlateGeometry/Discrete/TessellationSpecification/Api/TessellationSpecification.h"
#include "SlateGeometry/Topology/SolidStructure/Api/SolidStructure.h"

namespace Slate
{

Outcome<bool> ProjectSolid(const SolidStructure& Exact,
                           const TessellationSpecification& Requested,
                           TopologyStructure& Projected);

} // namespace Slate
