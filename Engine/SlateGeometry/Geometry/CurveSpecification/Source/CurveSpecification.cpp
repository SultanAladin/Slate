#include "SlateGeometry/Geometry/CurveSpecification/Api/CurveSpecification.h"

namespace Slate
{

bool CurveSpecification::Declared() const
{
    if (!Identity.Declared() || !ParameterRange.Declared())
        return false;

    if (Kind == CurveKind::Line || Kind == CurveKind::Circle || Kind == CurveKind::Ellipse)
        return true;

    return ControlPoints != nullptr && ControlPointCount > Degree;
}

} // namespace Slate
