//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Paths, whole streams and the write-verify-replace sequence over the host file system.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Platform/FileInterchange/Source
%layer      SlateMath
%sources    1
%symbols    11
%annotated  0/11
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S FileInterchange.cpp | 489 lines | a41f2c6f | 11 sym | Paths, whole streams and the write-verify-replace sequence over the host file system.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

K WIN32_LEAN_AND_MEAN               | FileInterchange.cpp | 11      | - | - | ?
    by    Source/ClipboardExchange.cpp, Source/CodeInterchange.cpp, Source/InputExchange.cpp, Source/PlatformInterchange.cpp, Source/StorageExchange.cpp, Source/TickSequence.cpp

K NOMINMAX                          | FileInterchange.cpp | 14      | - | - | ?
    by    Source/ClipboardExchange.cpp, Source/CodeInterchange.cpp, Source/InputExchange.cpp, Source/PlatformInterchange.cpp, Source/StorageExchange.cpp, Source/TickSequence.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                     PATH SPELLING
//------------------------------------------------------------------------------------------------------------------------

F Widen                             | FileInterchange.cpp | 39-56   | - | - | ?
    in    Narrow  const std::string&  [-]  ?
    out   -       std::wstring        [-]  ?
    by    Source/ClipboardExchange.cpp, Source/CodeInterchange.cpp, Source/SpatialSubdivision.cpp, Source/StorageExchange.cpp

F ProjectRevision                   | FileInterchange.cpp | 62-68   | - | - | ?
    in    Reported  const FILETIME&  [-]  ?
    out   -         std::uint64_t    [-]  ?

F StagedPath                        | FileInterchange.cpp | 75-78   | - | - | ?
    in    Path  const std::string&  [-]  ?
    out   -     std::string         [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   WHAT A PATH NAMES
//------------------------------------------------------------------------------------------------------------------------

F FileInterchange::Resolve          | FileInterchange.cpp | 86-161  | - | - | ?
    in    Path  const std::string&   [-]  ?
    out   -     Deliver<PathReport>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE READ
//------------------------------------------------------------------------------------------------------------------------

F FileInterchange::ReadStream       | FileInterchange.cpp | 167-245 | - | - | ?
    in    Path  const std::string&                  [-]  ?
    out   -     Deliver<std::vector<std::uint8_t>>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                              WRITE, VERIFY, THEN REPLACE
//------------------------------------------------------------------------------------------------------------------------

F FileInterchange::WriteStream      | FileInterchange.cpp | 251-364 | - | - | ?
    in    Path     const std::string&                [-]  ?
    in    Content  const std::vector<std::uint8_t>&  [-]  ?
    out   -        Deliver<bool>                     [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      DIRECTORIES
//------------------------------------------------------------------------------------------------------------------------

F FileInterchange::DeclareDirectory | FileInterchange.cpp | 370-417 | - | - | ?
    in    Path  const std::string&  [-]  ?
    out   -     Deliver<bool>       [-]  ?

F FileInterchange::Reclaim          | FileInterchange.cpp | 419-445 | - | - | ?
    in    Path  const std::string&  [-]  ?
    out   -     Deliver<bool>       [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     PATH ASSEMBLY
//------------------------------------------------------------------------------------------------------------------------

F FileInterchange::Append           | FileInterchange.cpp | 451-487 | - | - | ?
    in    Leading   const std::string&  [-]  ?
    in    Trailing  const std::string&  [-]  ?
    out   -         std::string         [-]  ?
