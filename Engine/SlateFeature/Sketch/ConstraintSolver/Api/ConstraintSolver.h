//============================================================================================================================================
//                                                        CONSTRAINTSOLVER.H
//============================================================================================================================================
// 🧩 Sketch-constraint evaluation and bounded solve application for the two-dimensional editor.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "SlateFeature/Sketch/SketchStructure/Api/SketchStructure.h"

#include <cstdint>

namespace Slate
{

enum class ConstraintDisposition : std::uint32_t
{
    NotRequested = 0u,
    InvalidSketch = 1u,
    UnsupportedConstraint = 2u,
    ConflictingConstraint = 3u,
    RepeatedConstraint = 4u,
    Produced = 5u
};

ConstraintDisposition EvaluateConstraints(const SketchStructure& Declared);
Outcome<bool> ResolveConstraintConflict(const SketchStructure& Declared,
                                        ConstraintName Subject);
Outcome<bool> ApplyConstraints(SketchStructure& Declared);
Outcome<bool> ApplyConstraint(SketchStructure& Declared,
                              ConstraintName Subject);

} // namespace Slate
