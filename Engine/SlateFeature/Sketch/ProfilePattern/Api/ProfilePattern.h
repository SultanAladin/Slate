//============================================================================================================================================
//                                                          PROFILEPATTERN.H
//============================================================================================================================================
// 🧩 Duplicate, mirror, repeat and profile-copy verbs for the two-dimensional editor.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "SlateFeature/Sketch/SketchStructure/Api/SketchStructure.h"

#include <vector>

namespace Slate
{

struct PatternResult
{
    std::vector<SketchCurveName> CurveSet = {};
    std::vector<ProfileNameInFeature> ProfileSet = {};
};

Outcome<PatternResult> DuplicateCurves(SketchStructure& Declared,
                                       const std::vector<SketchCurveName>& CurveSet,
                                       const SpatialDirection& Offset);
Outcome<PatternResult> DuplicateProfiles(SketchStructure& Declared,
                                         const std::vector<ProfileNameInFeature>& ProfileSet,
                                         const SpatialDirection& Offset);
Outcome<PatternResult> DuplicateBetween(SketchStructure& Declared,
                                        const std::vector<SketchCurveName>& CurveSet,
                                        const SpatialPoint& StartPoint,
                                        const SpatialPoint& EndPoint);
Outcome<PatternResult> MirrorCurves(SketchStructure& Declared,
                                    const std::vector<SketchCurveName>& CurveSet,
                                    const SpatialPoint& AxisStart,
                                    const SpatialPoint& AxisEnd);
Outcome<PatternResult> MirrorProfiles(SketchStructure& Declared,
                                      const std::vector<ProfileNameInFeature>& ProfileSet,
                                      const SpatialPoint& AxisStart,
                                      const SpatialPoint& AxisEnd);
Outcome<PatternResult> DeclareLinearPattern(SketchStructure& Declared,
                                            const std::vector<SketchCurveName>& CurveSet,
                                            const SpatialDirection& Step,
                                            std::uint32_t Count);
Outcome<PatternResult> DeclareRadialPattern(SketchStructure& Declared,
                                            const std::vector<SketchCurveName>& CurveSet,
                                            const SpatialPoint& Centre,
                                            const SpatialDirection& Axis,
                                            double SweepRadians,
                                            std::uint32_t Count);

} // namespace Slate
