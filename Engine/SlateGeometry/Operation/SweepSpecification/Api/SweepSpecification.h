//============================================================================================================================================
//                                                      SWEEPSPECIFICATION.H
//============================================================================================================================================

#pragma once

#include "SlateGeometry/Geometry/ProfileSpecification/Api/ProfileSpecification.h"
#include "SlateGeometry/Geometry/CurveSpecification/Api/CurveSpecification.h"

namespace Slate
{

struct SweepSpecification
{
    ProfileName SourceProfile = {};
    CurveName SpineCurve = {};
    bool RigidNormal = true;

    bool Declared() const { return SourceProfile.Assigned() && SpineCurve.Assigned(); }
};

} // namespace Slate
