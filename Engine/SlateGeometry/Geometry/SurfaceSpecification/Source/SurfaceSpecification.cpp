#include "SlateGeometry/Geometry/SurfaceSpecification/Api/SurfaceSpecification.h"

namespace Slate
{

bool SurfaceSpecification::Declared() const
{
    if (!Identity.Declared() || !UParameterRange.Declared() || !VParameterRange.Declared())
        return false;

    return TrimRelationCount == 0u || TrimRelations != nullptr;
}

} // namespace Slate
