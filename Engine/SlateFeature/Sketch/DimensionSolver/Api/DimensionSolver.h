//============================================================================================================================================
//                                                       DIMENSIONSOLVER.H
//============================================================================================================================================
// 🧩 Driving-dimension evaluation and bounded edit application for the two-dimensional sketch editor.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "SlateFeature/Sketch/SketchStructure/Api/SketchStructure.h"

#include <cstdint>

namespace Slate
{

enum class DimensionDisposition : std::uint32_t
{
    NotRequested = 0u,
    InvalidSketch = 1u,
    UnsupportedDimension = 2u,
    Produced = 3u
};

DimensionDisposition EvaluateDimensions(const SketchStructure& Declared);
Outcome<double> ResolveDimensionValue(const SketchStructure& Declared,
                                      DimensionName Subject);
Outcome<bool> ResolveDimensionConflict(const SketchStructure& Declared,
                                       DimensionName Subject);
Outcome<bool> ApplyDimensions(SketchStructure& Declared);
Outcome<bool> ApplyDimension(SketchStructure& Declared,
                             DimensionName Subject);

} // namespace Slate
