//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Declared byte ranges over the host's storage surface, drained in declaration order with the latency each took.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Platform/StorageExchange/Source
%layer      SlateMath
%sources    1
%symbols    10
%annotated  0/10
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S StorageExchange.cpp | 322 lines | 2bb1489b | 10 sym | Declared byte ranges over the host's storage surface, drained in declaration order with the latency each took.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

K WIN32_LEAN_AND_MEAN               | StorageExchange.cpp | 12      | -          | - | ?
    by    Source/ClipboardExchange.cpp, Source/CodeInterchange.cpp, Source/FileInterchange.cpp, Source/InputExchange.cpp, Source/PlatformInterchange.cpp, Source/TickSequence.cpp

K NOMINMAX                          | StorageExchange.cpp | 15      | -          | - | ?
    by    Source/ClipboardExchange.cpp, Source/CodeInterchange.cpp, Source/FileInterchange.cpp, Source/InputExchange.cpp, Source/PlatformInterchange.cpp, Source/TickSequence.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                     PATH SPELLING
//------------------------------------------------------------------------------------------------------------------------

F Widen                             | StorageExchange.cpp | 38-55   | -          | - | ?
    in    Narrow  const std::string&  [-]  ?
    out   -       std::wstring        [-]  ?
    by    Source/ClipboardExchange.cpp, Source/CodeInterchange.cpp, Source/FileInterchange.cpp, Source/SpatialSubdivision.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    OPEN AND RECLAIM
//------------------------------------------------------------------------------------------------------------------------

F StorageExchange::Open             | StorageExchange.cpp | 65-133  | -          | - | ?
    in    Path  const std::string&  [-]  ?
    out   -     Deliver<bool>       [-]  ?

F StorageExchange::Reclaim          | StorageExchange.cpp | 135-152 | -          | - | ?
    out   -  void  [-]  ?

F StorageExchange::~StorageExchange | StorageExchange.cpp | 154-157 | destructor | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    DECLARED RANGES
//------------------------------------------------------------------------------------------------------------------------

F StorageExchange::Declare          | StorageExchange.cpp | 163-190 | -          | - | ?
    in    Wanted  RangeRequest            [-]  ?
    out   -       Deliver<std::uint32_t>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE DRAIN
//------------------------------------------------------------------------------------------------------------------------

F StorageExchange::Drain            | StorageExchange.cpp | 196-306 | -          | - | ?
    out   -  const std::vector<RangeArrival>&  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        READINGS
//------------------------------------------------------------------------------------------------------------------------

F StorageExchange::PendingCount     | StorageExchange.cpp | 312-315 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F StorageExchange::SpannedBytes     | StorageExchange.cpp | 317-320 | -          | - | ?
    out   -  std::uint64_t  [-]  ?
