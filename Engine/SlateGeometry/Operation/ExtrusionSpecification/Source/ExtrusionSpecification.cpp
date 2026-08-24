#include "SlateGeometry/Operation/ExtrusionSpecification/Api/ExtrusionSpecification.h"

namespace Slate
{

bool ExtrusionSpecification::Declared() const
{
    return SourceProfile.Declared() && Direction.Declared() && Distance.Declared()
        && Distance.Numerator != 0;
}

ExtrusionResult EvaluateExtrusion(const ExtrusionSpecification& Specification)
{
    if (!Specification.Declared())
        return { {}, ExtrusionDisposition::InvalidSpecification };

    // The declaration seam exists before an exact modeller does. No approximate substitute is permitted here.
    return { {}, ExtrusionDisposition::ImplementationAbsent };
}

} // namespace Slate
