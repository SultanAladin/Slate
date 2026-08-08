//============================================================================================================================================
//                                                         TOPOLOGYSTRUCTURE.CPP
//============================================================================================================================================
// 🧩 Corner run assembly and the seal that closes it.

#include "SlateDocument/Document/TopologyStructure/Api/TopologyStructure.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                               ------------------ DECLARATION ------------------
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> TopologyStructure::DeclarePositions(const std::vector<DocumentPosition>& Arriving)
{
    if (SealDeclared)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the topology is sealed" });

    VertexPositions = Arriving;

    return Outcome<bool>::Deliver(true);
}

Outcome<bool> TopologyStructure::DeclareFace(const std::vector<std::uint32_t>& CornerVertices)
{
    if (SealDeclared)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the topology is sealed" });

    if (CornerVertices.size() < 3u)
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "fewer than three corners is not a face" });

    for (const std::uint32_t VertexOrdinal : CornerVertices)
    {
        if (VertexOrdinal >= VertexPositions.size())
        {
            return Outcome<bool>::Refuse(
                { RefusalReason::ContentUnsupported, "a corner addresses a vertex that was not declared" });
        }
    }

    const std::uint32_t FaceOrdinal = static_cast<std::uint32_t>(FaceFirstCorners.size());

    FaceFirstCorners.push_back(static_cast<std::uint32_t>(CornerVertexOrdinals.size()));
    FaceCornerCounts.push_back(static_cast<std::uint32_t>(CornerVertices.size()));

    // 📝 A repeated ordinal within one run is admitted deliberately. `38` §3 enrolls it as degenerate and
    //    excludes it downstream; refusing it here would repair the artist's file, which `50` §2 ① forbids.
    for (const std::uint32_t VertexOrdinal : CornerVertices)
    {
        CornerVertexOrdinals.push_back(VertexOrdinal);
        CornerFaceOrdinals.push_back(FaceOrdinal);
    }

    return Outcome<bool>::Deliver(true);
}

Outcome<bool> TopologyStructure::DeclareCoordinates(const std::vector<DomainCoordinate>& Arriving)
{
    if (SealDeclared)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the topology is sealed" });

    if (Arriving.size() != CornerVertexOrdinals.size())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "one coordinate per corner is required" });

    CornerCoordinates = Arriving;

    return Outcome<bool>::Deliver(true);
}

Outcome<bool> TopologyStructure::DeclarePerpendiculars(const std::vector<SurfaceDirection>& Arriving)
{
    if (SealDeclared)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the topology is sealed" });

    if (Arriving.size() != VertexPositions.size())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "one perpendicular per vertex is required" });

    VertexPerpendiculars = Arriving;

    return Outcome<bool>::Deliver(true);
}

Outcome<bool> TopologyStructure::DeclareTangentBases(const std::vector<TangentBasis>& Arriving)
{
    if (SealDeclared)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the topology is sealed" });

    if (Arriving.size() != VertexPositions.size())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "one basis per vertex is required" });

    VertexTangentBases = Arriving;

    return Outcome<bool>::Deliver(true);
}

Outcome<bool> TopologyStructure::DeclareMaterialEnrollment(const std::vector<std::uint32_t>& Arriving)
{
    if (SealDeclared)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the topology is sealed" });

    if (Arriving.size() != FaceFirstCorners.size())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "one enrollment per face is required" });

    FaceMaterialOrdinals = Arriving;

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE SEAL
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> TopologyStructure::Seal()
{
    if (SealDeclared)
        return Outcome<bool>::Deliver(true);

    if (FaceFirstCorners.empty())
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "a topology with no face is not paintable" });

    // 📝 One material for the whole occupant where the source declared none — `50` §3's last-resort row. It is a
    //    default rather than an assumption, so nothing is reported: the artist assigns materials afterwards.
    if (FaceMaterialOrdinals.empty())
        FaceMaterialOrdinals.assign(FaceFirstCorners.size(), 0u);

    ++SealedRevision;
    SealDeclared = true;

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

bool          TopologyStructure::Sealed() const      { return SealDeclared;   }
std::uint64_t TopologyStructure::Revision() const    { return SealedRevision; }

std::uint32_t TopologyStructure::VertexCount() const
{
    return static_cast<std::uint32_t>(VertexPositions.size());
}

std::uint32_t TopologyStructure::FaceCount() const
{
    return static_cast<std::uint32_t>(FaceFirstCorners.size());
}

std::uint32_t TopologyStructure::CornerCount() const
{
    return static_cast<std::uint32_t>(CornerVertexOrdinals.size());
}

std::uint32_t TopologyStructure::FaceFirstCorner(std::uint32_t FaceOrdinal) const
{
    return FaceOrdinal < FaceFirstCorners.size() ? FaceFirstCorners[FaceOrdinal] : 0u;
}

std::uint32_t TopologyStructure::FaceCornerCount(std::uint32_t FaceOrdinal) const
{
    return FaceOrdinal < FaceCornerCounts.size() ? FaceCornerCounts[FaceOrdinal] : 0u;
}

std::uint32_t TopologyStructure::CornerVertex(std::uint32_t CornerOrdinal) const
{
    return CornerOrdinal < CornerVertexOrdinals.size() ? CornerVertexOrdinals[CornerOrdinal] : 0u;
}

std::uint32_t TopologyStructure::CornerFace(std::uint32_t CornerOrdinal) const
{
    return CornerOrdinal < CornerFaceOrdinals.size() ? CornerFaceOrdinals[CornerOrdinal] : 0u;
}

const std::vector<DocumentPosition>& TopologyStructure::Positions() const          { return VertexPositions;      }
const std::vector<DomainCoordinate>& TopologyStructure::Coordinates() const        { return CornerCoordinates;    }
const std::vector<SurfaceDirection>& TopologyStructure::Perpendiculars() const     { return VertexPerpendiculars; }
const std::vector<TangentBasis>&     TopologyStructure::TangentBases() const       { return VertexTangentBases;   }
const std::vector<std::uint32_t>&    TopologyStructure::MaterialEnrollment() const { return FaceMaterialOrdinals; }

bool TopologyStructure::CoordinatesSupplied() const    { return !CornerCoordinates.empty();    }
bool TopologyStructure::PerpendicularsSupplied() const { return !VertexPerpendiculars.empty(); }
bool TopologyStructure::TangentBasesSupplied() const   { return !VertexTangentBases.empty();   }

}   // namespace Slate
