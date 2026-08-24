//============================================================================================================================================
//                                             GEOMETRYPRESENTATIONEXCHANGE.H
//============================================================================================================================================
// 🧩 Immutable CPU topology into revision-keyed, disposable presentation connectivity.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "Foundation/Identity.h"
#include "SlateDocument/Document/GeometryInterchange/Api/GeometryInterchange.h"

#include <cstdint>
#include <vector>

namespace Slate
{

struct GeometryPresentationVertex
{
    DocumentPosition Position = {};
    SurfaceDirection Perpendicular = {};
    DomainCoordinate Coordinate = {};
    std::uint32_t SourceVertex = 0u;
    std::uint32_t SourceCorner = 0u;
};

struct GeometryPresentationTriangle
{
    std::uint32_t Corners[3] = {};
    std::uint32_t SourceFace = 0u;
    std::uint32_t MaterialIndex = 0u;
};

struct GeometryPresentationSegment
{
    std::uint32_t Corners[2] = {};
};

/// 🧩 One immutable CPU packet from which disposable GPU buffers can be uploaded.
/// note  Positions remain 64-bit here. A device upload must rebase before narrowing them.
struct GeometryPresentationSnapshot
{
    GeometryIdentity Geometry = {};
    std::uint64_t TopologyRevision = 0u;
    std::vector<GeometryPresentationVertex> Vertices = {};
    std::vector<GeometryPresentationTriangle> Triangles = {};
    std::vector<GeometryPresentationSegment> SourceWire = {};
    std::vector<GeometryPresentationSegment> TriangulatedWire = {};
    std::vector<std::uint32_t> UnpresentedFaces = {};
};

/// 🧩 Builds and retains presentation packets by authoritative geometry identity and topology revision.
/// note  Vulkan buffer allocation follows behind this seam; this increment delivers the immutable upload packet.
class GeometryPresentationExchange
{
public:
    Outcome<GeometryPresentationIdentity> Synchronise(const GeometryAssetView& Geometry);
    Outcome<const GeometryPresentationSnapshot*> Resolve(GeometryPresentationIdentity Subject) const;
    Outcome<bool> Retire(GeometryPresentationIdentity Subject);
    void Reclaim();
    std::uint32_t DeclaredCount() const { return OccupiedCount; }

private:
    struct Entry
    {
        GeometryPresentationSnapshot Snapshot = {};
        std::uint32_t Generation = 1u;
        bool Occupied = false;
    };

    std::vector<Entry> Entries = {};
    std::vector<std::uint32_t> ReleasedSlots = {};
    std::uint32_t OccupiedCount = 0u;
};

} // namespace Slate
