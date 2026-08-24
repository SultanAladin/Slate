#include "SlateGeometry/Topology/SolidStructure/Api/SolidStructure.h"

namespace Slate
{

bool SolidStructure::Declared() const
{
    if (!Identity.Declared())
        return false;

    return (VertexCount == 0u || Vertices != nullptr)
        && (EdgeCount == 0u || Edges != nullptr)
        && (FaceCount == 0u || Faces != nullptr);
}

} // namespace Slate
