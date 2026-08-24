// Exact B-rep naming and incidence declarations. This is distinct from document polygon topology.
#pragma once
#include "SlateGeometry/Geometry/SurfaceSpecification/Api/SurfaceSpecification.h"
#include <cstdint>
namespace Slate
{
struct VertexName { std::uint64_t Value = 0u; };
struct EdgeName { std::uint64_t Value = 0u; };
struct FaceName { std::uint64_t Value = 0u; };
struct SolidName { std::uint64_t Value = 0u; };
struct SolidStructure
{
    SolidName Identity{};
    std::uint32_t VertexCount = 0u;
    std::uint32_t EdgeCount = 0u;
    std::uint32_t FaceCount = 0u;
    bool Exact = true;
};
} // namespace Slate
