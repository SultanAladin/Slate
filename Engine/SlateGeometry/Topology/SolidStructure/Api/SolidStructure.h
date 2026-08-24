//============================================================================================================================================
//                                                      SOLIDSTRUCTURE.H
//============================================================================================================================================
// 🧩 Exact B-rep naming and incidence declarations. This is intentionally separate from SlateDocument's polygon topology.

#pragma once

#include "SlateGeometry/Geometry/SurfaceSpecification/Api/SurfaceSpecification.h"

#include <cstdint>

namespace Slate
{

struct VertexName { std::uint64_t Value = 0u; constexpr bool Declared() const { return Value != 0u; } };
struct EdgeName   { std::uint64_t Value = 0u; constexpr bool Declared() const { return Value != 0u; } };
struct FaceName   { std::uint64_t Value = 0u; constexpr bool Declared() const { return Value != 0u; } };
struct SolidName  { std::uint64_t Value = 0u; constexpr bool Declared() const { return Value != 0u; } };

/// 🧩 An exact topological vertex and its exact Cartesian position.
struct ExactVertexStructure
{
    VertexName Identity{};
    ExactPoint3 Position{};
};

/// 🧩 An oriented edge relation. Curve direction is stated rather than inferred from storage order.
struct ExactEdgeStructure
{
    EdgeName Identity{};
    VertexName FirstVertex{};
    VertexName LastVertex{};
    CurveName SupportingCurve{};
    bool SameSense = true;
};

/// 🧩 A face paired with its supporting surface and its loop-edge references.
struct ExactFaceStructure
{
    FaceName Identity{};
    SurfaceName SupportingSurface{};
    const EdgeName* LoopEdges = nullptr;
    std::uint32_t LoopEdgeCount = 0u;
    bool SameSense = true;
};

/// 🧩 A non-owning view of exact solid incidence. Allocation and revisions remain above this kernel layer.
struct SolidStructure
{
    SolidName Identity{};
    const ExactVertexStructure* Vertices = nullptr;
    const ExactEdgeStructure* Edges = nullptr;
    const ExactFaceStructure* Faces = nullptr;
    std::uint32_t VertexCount = 0u;
    std::uint32_t EdgeCount = 0u;
    std::uint32_t FaceCount = 0u;

    bool Declared() const;
};

} // namespace Slate
