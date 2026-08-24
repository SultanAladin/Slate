//============================================================================================================================================
//                                                    SURFACESPECIFICATION.H
//============================================================================================================================================
// 🧩 Exact surface authority. Trim curves are named kernel relations; this type never owns document rows or GPU data.

#pragma once

#include "SlateGeometry/Geometry/CurveSpecification/Api/CurveSpecification.h"

#include <cstdint>

namespace Slate
{

struct SurfaceName
{
    std::uint64_t Value = 0u;
    constexpr bool Declared() const { return Value != 0u; }
};

enum class SurfaceKind : std::uint32_t
{
    Plane,
    Cylinder,
    Cone,
    Sphere,
    Torus,
    Bezier,
    Nurbs
};

/// 🧩 A curve used to trim a surface, with its orientation in the face loop declared explicitly.
struct SurfaceTrimRelation
{
    CurveName Curve{};
    bool SameSense = true;

    constexpr bool Declared() const { return Curve.Declared(); }
};

/// 🧩 Immutable semantic description of one exact parametric surface.
struct SurfaceSpecification
{
    SurfaceName                   Identity{};
    SurfaceKind                   Kind = SurfaceKind::Plane;
    CurveParameterInterval        UParameterRange{};
    CurveParameterInterval        VParameterRange{};
    const SurfaceTrimRelation*    TrimRelations = nullptr;
    std::uint32_t                 TrimRelationCount = 0u;
    std::uint32_t                 UDegree = 1u;
    std::uint32_t                 VDegree = 1u;

    bool Declared() const;
};

} // namespace Slate
