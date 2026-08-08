//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Slot issuance, withdrawal and generational resolution.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/PopulationIndex/Source
%layer      SlateDocument
%sources    1
%symbols    8
%annotated  0/8
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S PopulationIndex.cpp | 135 lines | 53f3f6c7 | 8 sym | Slot issuance, withdrawal and generational resolution.

//------------------------------------------------------------------------------------------------------------------------
//                                                       OCCUPANCY
//------------------------------------------------------------------------------------------------------------------------

F OccupancyIndex::Occupy         | PopulationIndex.cpp | 15-27   | - | - | ?
    in    SlotOrdinal  std::uint32_t  [-]  ?
    out   -            void           [-]  ?

F OccupancyIndex::Release        | PopulationIndex.cpp | 29-37   | - | - | ?
    in    SlotOrdinal  std::uint32_t  [-]  ?
    out   -            void           [-]  ?

F OccupancyIndex::Occupied       | PopulationIndex.cpp | 39-47   | - | - | ?
    in    SlotOrdinal  std::uint32_t  [-]  ?
    out   -            bool           [-]  ?

F OccupancyIndex::SpannedCount   | PopulationIndex.cpp | 49-52   | - | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       ENROLMENT
//------------------------------------------------------------------------------------------------------------------------

F PopulationIndex::Enrol         | PopulationIndex.cpp | 58-89   | - | - | ?
    out   -  Outcome<OccupantIdentity>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       WITHDRAWAL
//------------------------------------------------------------------------------------------------------------------------

F PopulationIndex::Withdraw      | PopulationIndex.cpp | 95-110  | - | - | ?
    in    Subject  OccupantIdentity  [-]  ?
    out   -        Outcome<bool>     [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

F PopulationIndex::Resolve       | PopulationIndex.cpp | 116-128 | - | - | ?
    in    Subject  OccupantIdentity  [-]  ?
    out   -        bool              [-]  ?

F PopulationIndex::EnrolledCount | PopulationIndex.cpp | 130-133 | - | - | ?
    out   -  std::uint32_t  [-]  ?
