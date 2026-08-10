//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Corner run assembly and the seal that closes it.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/TopologyStructure/Source
%layer      SlateDocument
%sources    1
%symbols    24
%annotated  0/24
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S TopologyStructure.cpp | 185 lines | 5748aa93 | 24 sym | Corner run assembly and the seal that closes it.

//------------------------------------------------------------------------------------------------------------------------
//                                                 DECLARATIONS AND SEAL
//------------------------------------------------------------------------------------------------------------------------

F TopologyStructure::DeclarePositions          | TopologyStructure.cpp | 15-23   | - | - | ?
    in    Arriving  const std::vector<DocumentPosition>&  [-]  ?
    out   -         Outcome<bool>                         [-]  ?

F TopologyStructure::DeclareFace               | TopologyStructure.cpp | 25-56   | - | - | ?
    in    CornerVertices  const std::vector<std::uint32_t>&  [-]  ?
    out   -               Outcome<bool>                      [-]  ?

F TopologyStructure::DeclareCoordinates        | TopologyStructure.cpp | 58-69   | - | - | ?
    in    Arriving  const std::vector<DomainCoordinate>&  [-]  ?
    out   -         Outcome<bool>                         [-]  ?

F TopologyStructure::DeclarePerpendiculars     | TopologyStructure.cpp | 71-82   | - | - | ?
    in    Arriving  const std::vector<SurfaceDirection>&  [-]  ?
    out   -         Outcome<bool>                         [-]  ?

F TopologyStructure::DeclareTangentBases       | TopologyStructure.cpp | 84-95   | - | - | ?
    in    Arriving  const std::vector<TangentBasis>&  [-]  ?
    out   -         Outcome<bool>                     [-]  ?

F TopologyStructure::DeclareMaterialEnrollment | TopologyStructure.cpp | 97-108  | - | - | ?
    in    Arriving  const std::vector<std::uint32_t>&  [-]  ?
    out   -         Outcome<bool>                      [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE SEAL
//------------------------------------------------------------------------------------------------------------------------

F TopologyStructure::Seal                      | TopologyStructure.cpp | 114-131 | - | - | ?
    out   -  Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

F TopologyStructure::Sealed                    | TopologyStructure.cpp | 137     | - | - | ?
    out   -  bool  [-]  ?

F TopologyStructure::Revision                  | TopologyStructure.cpp | 138     | - | - | ?
    out   -  std::uint64_t  [-]  ?

F TopologyStructure::VertexCount               | TopologyStructure.cpp | 140-143 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F TopologyStructure::FaceCount                 | TopologyStructure.cpp | 145-148 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F TopologyStructure::CornerCount               | TopologyStructure.cpp | 150-153 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F TopologyStructure::FaceFirstCorner           | TopologyStructure.cpp | 155-158 | - | - | ?
    in    FaceOrdinal  std::uint32_t  [-]  ?
    out   -            std::uint32_t  [-]  ?

F TopologyStructure::FaceCornerCount           | TopologyStructure.cpp | 160-163 | - | - | ?
    in    FaceOrdinal  std::uint32_t  [-]  ?
    out   -            std::uint32_t  [-]  ?

F TopologyStructure::CornerVertex              | TopologyStructure.cpp | 165-168 | - | - | ?
    in    CornerOrdinal  std::uint32_t  [-]  ?
    out   -              std::uint32_t  [-]  ?

F TopologyStructure::CornerFace                | TopologyStructure.cpp | 170-173 | - | - | ?
    in    CornerOrdinal  std::uint32_t  [-]  ?
    out   -              std::uint32_t  [-]  ?

F TopologyStructure::Positions                 | TopologyStructure.cpp | 175     | - | - | ?
    out   -  const std::vector<DocumentPosition>&  [-]  ?

F TopologyStructure::Coordinates               | TopologyStructure.cpp | 176     | - | - | ?
    out   -  const std::vector<DomainCoordinate>&  [-]  ?

F TopologyStructure::Perpendiculars            | TopologyStructure.cpp | 177     | - | - | ?
    out   -  const std::vector<SurfaceDirection>&  [-]  ?

F TopologyStructure::TangentBases              | TopologyStructure.cpp | 178     | - | - | ?
    out   -  const std::vector<TangentBasis>&  [-]  ?

F TopologyStructure::MaterialEnrollment        | TopologyStructure.cpp | 179     | - | - | ?
    out   -  const std::vector<std::uint32_t>&  [-]  ?

F TopologyStructure::CoordinatesSupplied       | TopologyStructure.cpp | 181     | - | - | ?
    out   -  bool  [-]  ?

F TopologyStructure::PerpendicularsSupplied    | TopologyStructure.cpp | 182     | - | - | ?
    out   -  bool  [-]  ?

F TopologyStructure::TangentBasesSupplied      | TopologyStructure.cpp | 183     | - | - | ?
    out   -  bool  [-]  ?
