//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Size validation, the single atmospheric source, incidence projection, and the reach index.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/IlluminantPopulation/Source
%layer      SlateDocument
%sources    1
%symbols    23
%annotated  0/23
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S IlluminantPopulation.cpp | 490 lines | 36583b50 | 23 sym | Size validation, the single atmospheric source, incidence projection, and the reach index.

//------------------------------------------------------------------------------------------------------------------------
//                                                     IDENTITY ORDER
//------------------------------------------------------------------------------------------------------------------------

F PrecedesInIdentity                      | IlluminantPopulation.cpp | 24-30   | - | - | ?
    in    Earlier  OccupantIdentity  [-]  ?
    in    Later    OccupantIdentity  [-]  ?
    out   -        bool              [-]  ?
    by    Source/PointerIntersection.cpp

F EmissionDirection                       | IlluminantPopulation.cpp | 32-51   | - | - | ?
    in    Rotation  RotationQuaternion  [-]  ?
    in    OutX      double&             [-]  ?
    in    OutY      double&             [-]  ?
    in    OutZ      double&             [-]  ?
    out   -         void                [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       VALIDATION
//------------------------------------------------------------------------------------------------------------------------

F IlluminantPopulation::Validate          | IlluminantPopulation.cpp | 59-133  | - | - | ?
    in    Declaring  const IlluminantSpecification&  [-]  ?
    in    Subject    OccupantIdentity                [-]  ?
    out   -          Deliver<bool>                   [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      DECLARATION
//------------------------------------------------------------------------------------------------------------------------

F IlluminantPopulation::Located           | IlluminantPopulation.cpp | 139-155 | - | - | ?
    in    Subject  OccupantIdentity  [-]  ?
    out   -        std::size_t       [-]  ?

F IlluminantPopulation::Declare           | IlluminantPopulation.cpp | 157-182 | - | - | ?
    in    Subject    OccupantIdentity                [-]  ?
    in    Declaring  const IlluminantSpecification&  [-]  ?
    out   -          Deliver<bool>                   [-]  ?

F IlluminantPopulation::Amend             | IlluminantPopulation.cpp | 184-200 | - | - | ?
    in    Subject   OccupantIdentity                [-]  ?
    in    Amending  const IlluminantSpecification&  [-]  ?
    out   -         Deliver<bool>                   [-]  ?

F IlluminantPopulation::Withdraw          | IlluminantPopulation.cpp | 202-215 | - | - | ?
    in    Subject  OccupantIdentity  [-]  ?
    out   -        Deliver<bool>     [-]  ?

F IlluminantPopulation::Resolve           | IlluminantPopulation.cpp | 217-228 | - | - | ?
    in    Subject  OccupantIdentity                  [-]  ?
    out   -        Deliver<IlluminantSpecification>  [-]  ?

F IlluminantPopulation::ResolveColour     | IlluminantPopulation.cpp | 230-252 | - | - | ?
    in    Subject  OccupantIdentity                 [-]  ?
    in    Working  const ColourSpaceSpecification&  [-]  ?
    out   -        Deliver<ColourSpecification>     [-]  ?

F IlluminantPopulation::AtmosphericSource | IlluminantPopulation.cpp | 254-264 | - | - | ?
    out   -  Deliver<OccupantIdentity>  [-]  ?

F IlluminantPopulation::Enrolled          | IlluminantPopulation.cpp | 266     | - | - | ?
    out   -  const std::vector<OccupantIdentity>&  [-]  ?

F IlluminantPopulation::Revision          | IlluminantPopulation.cpp | 267     | - | - | ?
    out   -  std::uint64_t  [-]  ?

F IlluminantPopulation::EnrolledCount     | IlluminantPopulation.cpp | 269-272 | - | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       INCIDENCE
//------------------------------------------------------------------------------------------------------------------------

F ProjectIncidence                        | IlluminantPopulation.cpp | 278-374 | - | - | ?
    in    Declared  const IlluminantSpecification&  [-]  ?
    in    Shaded    DocumentPosition                [-]  ?
    out   -         Deliver<IncidenceProjection>    [-]  ?
    by    Api/IlluminantPopulation.h, Source/ConsoleHost.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE REACH INDEX
//------------------------------------------------------------------------------------------------------------------------

F ReachesExtent                           | IlluminantPopulation.cpp | 385-397 | - | - | ?
    in    Declared  const IlluminantSpecification&  [-]  ?
    in    Extent    PartitionExtent                 [-]  ?
    out   -         bool                            [-]  ?

F IlluminantIndex::Derive                 | IlluminantPopulation.cpp | 401-419 | - | - | ?
    in    Illuminants  const IlluminantPopulation&          [-]  ?
    in    Extents      const std::vector<PartitionExtent>&  [-]  ?
    out   -            Deliver<bool>                        [-]  ?

F IlluminantIndex::DerivePartition        | IlluminantPopulation.cpp | 421-459 | - | - | ?
    in    Illuminants       const IlluminantPopulation&  [-]  ?
    in    PartitionOrdinal  std::uint32_t                [-]  ?
    in    Extent            PartitionExtent              [-]  ?
    out   -                 Deliver<bool>                [-]  ?

F IlluminantIndex::ReachingCount          | IlluminantPopulation.cpp | 461-467 | - | - | ?
    in    PartitionOrdinal  std::uint32_t  [-]  ?
    out   -                 std::uint32_t  [-]  ?

F IlluminantIndex::Reaching               | IlluminantPopulation.cpp | 469-475 | - | - | ?
    in    PartitionOrdinal  std::uint32_t              [-]  ?
    in    ReachOrdinal      std::uint32_t              [-]  ?
    out   -                 Deliver<OccupantIdentity>  [-]  ?

F IlluminantIndex::TruncatedCount         | IlluminantPopulation.cpp | 477-480 | - | - | ?
    in    PartitionOrdinal  std::uint32_t  [-]  ?
    out   -                 std::uint32_t  [-]  ?

F IlluminantIndex::TruncatedTotal         | IlluminantPopulation.cpp | 482     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F IlluminantIndex::DescribedRevision      | IlluminantPopulation.cpp | 483     | - | - | ?
    out   -  std::uint64_t  [-]  ?

F IlluminantIndex::SpannedCount           | IlluminantPopulation.cpp | 485-488 | - | - | ?
    out   -  std::uint32_t  [-]  ?
