//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Host timeline over the operating system's monotonic counter.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Platform/TickSequence/Source
%layer      SlateMath
%sources    1
%symbols    6
%annotated  0/6
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S TickSequence.cpp | 125 lines | 6c2c938b | 6 sym | Host timeline over the operating system's monotonic counter.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

K WIN32_LEAN_AND_MEAN        | TickSequence.cpp | 14      | - | - | ?
    by    Source/ClipboardExchange.cpp, Source/CodeInterchange.cpp, Source/FileInterchange.cpp, Source/InputExchange.cpp, Source/PlatformInterchange.cpp, Source/StorageExchange.cpp

K NOMINMAX                   | TickSequence.cpp | 17      | - | - | ?
    by    Source/ClipboardExchange.cpp, Source/CodeInterchange.cpp, Source/FileInterchange.cpp, Source/InputExchange.cpp, Source/PlatformInterchange.cpp, Source/StorageExchange.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F TickSequence::TickSequence | TickSequence.cpp | 31-53   | - | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        ADVANCE
//------------------------------------------------------------------------------------------------------------------------

F TickSequence::Advance      | TickSequence.cpp | 59-83   | - | - | ?
    out   -  TickPoint  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     A HOST READING
//------------------------------------------------------------------------------------------------------------------------

F TickSequence::Project      | TickSequence.cpp | 89-111  | - | - | ?
    in    HostCount  std::uint64_t  [-]  ?
    out   -          TickPoint      [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                          SPAN
//------------------------------------------------------------------------------------------------------------------------

F TickSequence::Span         | TickSequence.cpp | 117-123 | - | - | ?
    in    Earlier  TickPoint  [-]  ?
    in    Later    TickPoint  [-]  ?
    out   -        double     [-]  ?
