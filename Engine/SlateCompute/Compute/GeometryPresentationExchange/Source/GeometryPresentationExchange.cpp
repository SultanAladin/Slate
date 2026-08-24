//============================================================================================================================================
//                                           GEOMETRYPRESENTATIONEXCHANGE.CPP
//============================================================================================================================================

#include "SlateCompute/Compute/GeometryPresentationExchange/Api/GeometryPresentationExchange.h"

#include "ExternalPackages/earcut/include/mapbox/earcut.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <unordered_map>

namespace Slate
{
namespace
{
using Point2 = std::array<double, 2u>;
using Polygon2 = std::vector<std::vector<Point2>>;

std::uint64_t EdgeKey(std::uint32_t First, std::uint32_t Second)
{
    if (Second < First) std::swap(First, Second);
    return (static_cast<std::uint64_t>(First) << 32u) | Second;
}

void AddSegment(std::vector<GeometryPresentationSegment>& Into,
                std::unordered_map<std::uint64_t, std::uint32_t>& Declared,
                std::uint32_t FirstCorner, std::uint32_t SecondCorner,
                std::uint32_t FirstVertex, std::uint32_t SecondVertex)
{
    const std::uint64_t Key = EdgeKey(FirstVertex, SecondVertex);
    if (Declared.find(Key) != Declared.end()) return;
    GeometryPresentationSegment Segment;
    Segment.Corners[0] = FirstCorner;
    Segment.Corners[1] = SecondCorner;
    Declared.emplace(Key, static_cast<std::uint32_t>(Into.size()));
    Into.push_back(Segment);
}

bool BuildFace(const TopologyStructure& Topology, std::uint32_t Face,
               GeometryPresentationSnapshot& Snapshot,
               std::unordered_map<std::uint64_t, std::uint32_t>& TriangleEdges)
{
    const std::uint32_t First = Topology.FaceFirstCorner(Face);
    const std::uint32_t Count = Topology.FaceCornerCount(Face);
    if (Count < 3u) return false;

    double Normal[3] = {};
    for (std::uint32_t Index = 0u; Index < Count; ++Index)
    {
        const DocumentPosition& A = Topology.Positions()[Topology.CornerVertex(First + Index)];
        const DocumentPosition& B = Topology.Positions()[Topology.CornerVertex(First + (Index + 1u) % Count)];
        Normal[0] += (A.PositionY - B.PositionY) * (A.PositionZ + B.PositionZ);
        Normal[1] += (A.PositionZ - B.PositionZ) * (A.PositionX + B.PositionX);
        Normal[2] += (A.PositionX - B.PositionX) * (A.PositionY + B.PositionY);
    }
    const double Magnitude = std::abs(Normal[0]) + std::abs(Normal[1]) + std::abs(Normal[2]);
    if (Magnitude <= 1.0e-18) return false;

    std::uint32_t Dropped = 0u;
    if (std::abs(Normal[1]) > std::abs(Normal[Dropped])) Dropped = 1u;
    if (std::abs(Normal[2]) > std::abs(Normal[Dropped])) Dropped = 2u;

    Polygon2 Polygon(1u);
    Polygon[0].reserve(Count);
    for (std::uint32_t Index = 0u; Index < Count; ++Index)
    {
        const DocumentPosition& P = Topology.Positions()[Topology.CornerVertex(First + Index)];
        if (Dropped == 0u) Polygon[0].push_back({ P.PositionY, P.PositionZ });
        else if (Dropped == 1u) Polygon[0].push_back({ P.PositionX, P.PositionZ });
        else Polygon[0].push_back({ P.PositionX, P.PositionY });
    }

    const std::vector<std::uint32_t> Local = mapbox::earcut<std::uint32_t>(Polygon);
    if (Local.size() != static_cast<std::size_t>(Count - 2u) * 3u) return false;

    const std::vector<std::uint32_t>& Materials = Topology.MaterialRegistration();
    const std::uint32_t Material = Face < Materials.size() ? Materials[Face] : 0u;
    for (std::size_t Index = 0u; Index < Local.size(); Index += 3u)
    {
        GeometryPresentationTriangle Triangle;
        Triangle.Corners[0] = First + Local[Index + 0u];
        Triangle.Corners[1] = First + Local[Index + 1u];
        Triangle.Corners[2] = First + Local[Index + 2u];
        Triangle.SourceFace = Face;
        Triangle.MaterialIndex = Material;

        const DocumentPosition& A = Snapshot.Vertices[Triangle.Corners[0]].Position;
        const DocumentPosition& B = Snapshot.Vertices[Triangle.Corners[1]].Position;
        const DocumentPosition& C = Snapshot.Vertices[Triangle.Corners[2]].Position;
        const double AB[3] = { B.PositionX - A.PositionX, B.PositionY - A.PositionY, B.PositionZ - A.PositionZ };
        const double AC[3] = { C.PositionX - A.PositionX, C.PositionY - A.PositionY, C.PositionZ - A.PositionZ };
        const double Cross[3] = { AB[1] * AC[2] - AB[2] * AC[1],
                                  AB[2] * AC[0] - AB[0] * AC[2],
                                  AB[0] * AC[1] - AB[1] * AC[0] };
        if (Cross[0] * Normal[0] + Cross[1] * Normal[1] + Cross[2] * Normal[2] < 0.0)
            std::swap(Triangle.Corners[1], Triangle.Corners[2]);

        Snapshot.Triangles.push_back(Triangle);
        for (std::uint32_t Edge = 0u; Edge < 3u; ++Edge)
        {
            const std::uint32_t C0 = Triangle.Corners[Edge];
            const std::uint32_t C1 = Triangle.Corners[(Edge + 1u) % 3u];
            AddSegment(Snapshot.TriangulatedWire, TriangleEdges, C0, C1,
                       Snapshot.Vertices[C0].SourceVertex, Snapshot.Vertices[C1].SourceVertex);
        }
    }
    return true;
}

Outcome<GeometryPresentationSnapshot> BuildSnapshot(const GeometryAssetView& Geometry)
{
    if (!Geometry.Identity.IdentityDeclared() || Geometry.Topology == nullptr ||
        Geometry.Conditioning == nullptr || !Geometry.Topology->Sealed())
        return Outcome<GeometryPresentationSnapshot>::Refuse(
            { RefusalReason::IdentityStale, "geometry presentation requires one resolved immutable geometry view" });

    const TopologyStructure& Topology = *Geometry.Topology;
    GeometryPresentationSnapshot Built;
    Built.Geometry = Geometry.Identity;
    Built.TopologyRevision = Topology.Revision();
    Built.Vertices.resize(Topology.CornerCount());
    const std::vector<SurfaceDirection>& Perpendiculars = Geometry.Conditioning->Perpendiculars();

    for (std::uint32_t Corner = 0u; Corner < Topology.CornerCount(); ++Corner)
    {
        GeometryPresentationVertex& Vertex = Built.Vertices[Corner];
        Vertex.SourceCorner = Corner;
        Vertex.SourceVertex = Topology.CornerVertex(Corner);
        Vertex.Position = Topology.Positions()[Vertex.SourceVertex];
        if (Vertex.SourceVertex < Perpendiculars.size()) Vertex.Perpendicular = Perpendiculars[Vertex.SourceVertex];
        if (Topology.CoordinatesSupplied() && Corner < Topology.Coordinates().size())
            Vertex.Coordinate = Topology.Coordinates()[Corner];
    }

    std::unordered_map<std::uint64_t, std::uint32_t> SourceEdges;
    std::unordered_map<std::uint64_t, std::uint32_t> TriangleEdges;
    for (std::uint32_t Face = 0u; Face < Topology.FaceCount(); ++Face)
    {
        const std::uint32_t First = Topology.FaceFirstCorner(Face);
        const std::uint32_t Count = Topology.FaceCornerCount(Face);
        for (std::uint32_t Edge = 0u; Edge < Count; ++Edge)
        {
            const std::uint32_t C0 = First + Edge;
            const std::uint32_t C1 = First + (Edge + 1u) % Count;
            AddSegment(Built.SourceWire, SourceEdges, C0, C1,
                       Built.Vertices[C0].SourceVertex, Built.Vertices[C1].SourceVertex);
        }
        if (!BuildFace(Topology, Face, Built, TriangleEdges)) Built.UnpresentedFaces.push_back(Face);
    }
    return Outcome<GeometryPresentationSnapshot>::Result(std::move(Built));
}
}

Outcome<GeometryPresentationIdentity> GeometryPresentationExchange::Synchronise(const GeometryAssetView& Geometry)
{
    if (!Geometry.Identity.IdentityDeclared() || Geometry.Topology == nullptr || Geometry.Conditioning == nullptr)
        return Outcome<GeometryPresentationIdentity>::Refuse(
            { RefusalReason::IdentityStale, "geometry presentation requires one resolved immutable geometry view" });

    for (std::uint32_t Slot = 0u; Slot < Entries.size(); ++Slot)
    {
        Entry& Held = Entries[Slot];
        if (!Held.Occupied || Held.Snapshot.Geometry != Geometry.Identity) continue;
        if (Held.Snapshot.TopologyRevision == Geometry.Topology->Revision())
            return Outcome<GeometryPresentationIdentity>::Result({ Slot, Held.Generation });
        const Outcome<GeometryPresentationSnapshot> Built = BuildSnapshot(Geometry);
        if (!Built.Resolved) return Outcome<GeometryPresentationIdentity>::Refuse(Built.Error);
        Held.Snapshot = Built.Resolve();
        ++Held.Generation;
        if (Held.Generation == 0u) Held.Generation = 1u;
        return Outcome<GeometryPresentationIdentity>::Result({ Slot, Held.Generation });
    }

    const Outcome<GeometryPresentationSnapshot> Built = BuildSnapshot(Geometry);
    if (!Built.Resolved) return Outcome<GeometryPresentationIdentity>::Refuse(Built.Error);
    std::uint32_t Slot = 0u;
    if (!ReleasedSlots.empty()) { Slot = ReleasedSlots.back(); ReleasedSlots.pop_back(); }
    else { Slot = static_cast<std::uint32_t>(Entries.size()); Entries.push_back({}); }
    Entry& Held = Entries[Slot];
    Held.Snapshot = Built.Resolve();
    Held.Occupied = true;
    ++OccupiedCount;
    return Outcome<GeometryPresentationIdentity>::Result({ Slot, Held.Generation });
}

Outcome<const GeometryPresentationSnapshot*> GeometryPresentationExchange::Resolve(GeometryPresentationIdentity Subject) const
{
    if (!Subject.IdentityDeclared() || Subject.SlotIndex >= Entries.size())
        return Outcome<const GeometryPresentationSnapshot*>::Refuse({ RefusalReason::IdentityStale, "the geometry presentation is stale" });
    const Entry& Held = Entries[Subject.SlotIndex];
    if (!Held.Occupied || Held.Generation != Subject.SlotGeneration)
        return Outcome<const GeometryPresentationSnapshot*>::Refuse({ RefusalReason::IdentityStale, "the geometry presentation is stale" });
    return Outcome<const GeometryPresentationSnapshot*>::Result(&Held.Snapshot);
}

Outcome<bool> GeometryPresentationExchange::Retire(GeometryPresentationIdentity Subject)
{
    if (!Subject.IdentityDeclared() || Subject.SlotIndex >= Entries.size())
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "the geometry presentation is stale" });
    Entry& Held = Entries[Subject.SlotIndex];
    if (!Held.Occupied || Held.Generation != Subject.SlotGeneration)
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "the geometry presentation is stale" });
    Held.Snapshot = {};
    Held.Occupied = false;
    ++Held.Generation;
    if (Held.Generation == 0u) Held.Generation = 1u;
    ReleasedSlots.push_back(Subject.SlotIndex);
    --OccupiedCount;
    return Outcome<bool>::Result(true);
}

void GeometryPresentationExchange::Reclaim()
{
    ReleasedSlots.clear();
    for (std::uint32_t Slot = 0u; Slot < Entries.size(); ++Slot)
    {
        Entry& Held = Entries[Slot];
        Held.Snapshot = {};
        Held.Occupied = false;
        ++Held.Generation;
        if (Held.Generation == 0u) Held.Generation = 1u;
        ReleasedSlots.push_back(Slot);
    }
    OccupiedCount = 0u;
}

} // namespace Slate
