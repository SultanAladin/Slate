// Exact modelling declaration. Execution is intentionally deferred from this structural pass.
#pragma once
#include "SlateGeometry/Topology/SolidStructure/Api/SolidStructure.h"
#include <cstdint>
namespace Slate
{
struct ChamferSolver
{
    SolidName Input{};
    double Parameter = 0.0;
    bool Requested = false;
};
} // namespace Slate
