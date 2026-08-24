// Exact surface declarations owned by SlateGeometry.
#pragma once
#include "SlateGeometry/Geometry/CurveSpecification/Api/CurveSpecification.h"
#include <cstdint>
namespace Slate
{
enum class SurfaceKind : std::uint32_t { Plane, Cylinder, Cone, Sphere, Torus, Bezier, Nurbs };
struct SurfaceSpecification
{
    SurfaceKind Kind = SurfaceKind::Plane;
    std::uint64_t Identity = 0u;
    bool Exact = true;
};
} // namespace Slate
