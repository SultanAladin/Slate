// Declares an exact-to-discrete request; it owns no GPU or render cache.
#pragma once
#include "SlateGeometry/Topology/SolidStructure/Api/SolidStructure.h"
#include <cstdint>
namespace Slate
{
struct TessellationSpecification
{
    SolidName Source{};
    double ChordTolerance = 1.0e-4;
    double AngleTolerance = 1.0e-2;
};
} // namespace Slate
