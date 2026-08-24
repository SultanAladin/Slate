//============================================================================================================================================
//                                                     CURVESPECIFICATION.H
//============================================================================================================================================
// 🧩 Exact curve authority for SlateGeometry. This is geometric meaning only: it owns neither document identity
//    allocation nor a render-ready polyline.

#pragma once

#include <cstdint>

namespace Slate
{

/// 🧩 An exact scalar represented as a signed rational value.
/// note  A denominator of zero is undeclared. Normalisation is deliberately an operation concern, not a hidden
///       mutation performed while a declaration is merely inspected.
struct RationalScalar
{
    std::int64_t Numerator   = 0;
    std::int64_t Denominator = 1;

    constexpr bool Declared() const { return Denominator != 0; }
};

/// 🧩 A Cartesian coordinate composed of exact scalar components.
struct ExactPoint2
{
    RationalScalar X{};
    RationalScalar Y{};
};

/// 🧩 A Cartesian coordinate composed of exact scalar components.
struct ExactPoint3
{
    RationalScalar X{};
    RationalScalar Y{};
    RationalScalar Z{};
};

/// 🧩 An ordered rational parameter interval.
struct CurveParameterInterval
{
    RationalScalar First{};
    RationalScalar Last{};

    constexpr bool Declared() const { return First.Declared() && Last.Declared(); }
};

/// 🧩 Stable kernel name for a curve. Zero is never an authored curve.
struct CurveName
{
    std::uint64_t Value = 0u;
    constexpr bool Declared() const { return Value != 0u; }
};

enum class CurveKind : std::uint32_t
{
    Line,
    Circle,
    Ellipse,
    Bezier,
    Nurbs
};

/// 🧩 Immutable semantic description of one exact curve.
/// note  Control data is intentionally referenced rather than owned. Ownership arrives later through a kernel
///       model, never through a document/session container hidden inside this value type.
struct CurveSpecification
{
    CurveName                     Identity{};
    CurveKind                     Kind = CurveKind::Line;
    CurveParameterInterval        ParameterRange{};
    const ExactPoint3*            ControlPoints = nullptr;
    const RationalScalar*         Weights = nullptr;
    std::uint32_t                 ControlPointCount = 0u;
    std::uint32_t                 Degree = 1u;

    bool Declared() const;
};

} // namespace Slate
