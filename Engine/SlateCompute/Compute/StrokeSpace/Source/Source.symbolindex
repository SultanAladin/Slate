//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Sparse tile claiming over the dense cell index, and the commutative coverage accumulation.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/StrokeSpace/Source
%layer      SlateCompute
%sources    1
%symbols    9
%annotated  0/9
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S StrokeSpace.cpp | 129 lines | 16243adc | 9 sym | Sparse tile claiming over the dense cell index, and the commutative coverage accumulation.

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F StrokeSpace::Construct         | StrokeSpace.cpp | 15-23   | - | - | ?
    out   -  void  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    CLAIM AND LOCATE
//------------------------------------------------------------------------------------------------------------------------

F StrokeSpace::Claim             | StrokeSpace.cpp | 29-56   | - | - | ?
    in    CellOrdinal  std::uint32_t           [-]  ?
    out   -            Outcome<std::uint32_t>  [-]  ?

F StrokeSpace::Located           | StrokeSpace.cpp | 58-64   | - | - | ?
    in    CellOrdinal  std::uint32_t           [-]  ?
    out   -            Outcome<std::uint32_t>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      ACCUMULATION
//------------------------------------------------------------------------------------------------------------------------

F StrokeSpace::Accumulate        | StrokeSpace.cpp | 70-92   | - | - | ?
    in    TileOrdinal  std::uint32_t  [-]  ?
    in    Along        std::uint32_t  [-]  ?
    in    Across       std::uint32_t  [-]  ?
    in    Arriving     double         [-]  ?
    out   -            void           [-]  ?

F StrokeSpace::Coverage          | StrokeSpace.cpp | 94-100  | - | - | ?
    in    TileOrdinal  std::uint32_t  [-]  ?
    in    Along        std::uint32_t  [-]  ?
    in    Across       std::uint32_t  [-]  ?
    out   -            double         [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

F StrokeSpace::TouchedCells      | StrokeSpace.cpp | 106     | - | - | ?
    out   -  const std::vector<std::uint32_t>&  [-]  ?

F StrokeSpace::ClaimedCount      | StrokeSpace.cpp | 108-111 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F StrokeSpace::TouchedTexelCount | StrokeSpace.cpp | 113     | - | - | ?
    out   -  std::uint64_t  [-]  ?

F StrokeSpace::Reclaim           | StrokeSpace.cpp | 115-127 | - | - | ?
    out   -  void  [-]  ?
