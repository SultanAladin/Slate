// Exact curve declarations owned by SlateGeometry, not document or render state.
#pragma once
#include <cstdint>
namespace Slate
{
enum class CurveKind : std::uint32_t { Line, Circle, Ellipse, Bezier, Nurbs };
struct ExactPoint2 { double X = 0.0; double Y = 0.0; };
struct ExactPoint3 { double X = 0.0; double Y = 0.0; double Z = 0.0; };
struct CurveSpecification
{
    CurveKind Kind = CurveKind::Line;
    std::uint64_t Identity = 0u;
    bool Exact = true;
};
} // namespace Slate
