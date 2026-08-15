//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The load, the verification performed before any table is read, and the unload taken on every refusing path.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Platform/CodeInterchange/Source
%layer      SlateMath
%sources    1
%symbols    14
%annotated  0/14
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S CodeInterchange.cpp | 281 lines | 6b6f2258 | 14 sym | The load, the verification performed before any table is read, and the unload taken on every refusing path.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

K WIN32_LEAN_AND_MEAN               | CodeInterchange.cpp | 13      | -          | - | ?
    by    Source/ClipboardExchange.cpp, Source/FileInterchange.cpp, Source/InputExchange.cpp, Source/PlatformInterchange.cpp, Source/StorageExchange.cpp, Source/TickSequence.cpp

K NOMINMAX                          | CodeInterchange.cpp | 16      | -          | - | ?
    by    Source/ClipboardExchange.cpp, Source/FileInterchange.cpp, Source/InputExchange.cpp, Source/PlatformInterchange.cpp, Source/StorageExchange.cpp, Source/TickSequence.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE HOST EDGE
//------------------------------------------------------------------------------------------------------------------------

V AcquisitionEntry                  | CodeInterchange.cpp | 35      | -          | - | ?

F Widen                             | CodeInterchange.cpp | 39-56   | -          | - | ?
    in    Narrow  const std::string&  [-]  ?
    out   -       std::wstring        [-]  ?
    by    Source/ClipboardExchange.cpp, Source/FileInterchange.cpp, Source/SpatialSubdivision.cpp, Source/StorageExchange.cpp

F LoadModule                        | CodeInterchange.cpp | 60-81   | -          | - | ?
    in    ModulePath  const std::string&  [-]  ?
    out   -           void*               [-]  ?

F UnloadModule                      | CodeInterchange.cpp | 83-93   | -          | - | ?
    in    HostToken  void*  [-]  ?
    out   -          void   [-]  ?

F ResolveAcquisition                | CodeInterchange.cpp | 95-114  | -          | - | ?
    in    HostToken  void*                    [-]  ?
    out   -          SlateAcquireModuleEntry  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE ACQUISITION
//------------------------------------------------------------------------------------------------------------------------

F CodeInterchange::Acquire          | CodeInterchange.cpp | 122-214 | -          | - | ?
    in    ModulePath  const std::string&      [-]  ?
    in    Required    ForeignRequirement      [-]  ?
    out   -           Deliver<std::uint32_t>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

F CodeInterchange::EntryTable       | CodeInterchange.cpp | 220-226 | -          | - | ?
    in    ModuleOrdinal  std::uint32_t         [-]  ?
    out   -              Deliver<const void*>  [-]  ?

F CodeInterchange::Report           | CodeInterchange.cpp | 228-237 | -          | - | ?
    in    ModuleOrdinal  std::uint32_t                      [-]  ?
    out   -              Deliver<const SlateModuleReport*>  [-]  ?

F CodeInterchange::StandingCount    | CodeInterchange.cpp | 239-250 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F CodeInterchange::Release          | CodeInterchange.cpp | 256-268 | -          | - | ?
    in    ModuleOrdinal  std::uint32_t  [-]  ?
    out   -              void           [-]  ?

F CodeInterchange::Reclaim          | CodeInterchange.cpp | 270-274 | -          | - | ?
    out   -  void  [-]  ?

F CodeInterchange::~CodeInterchange | CodeInterchange.cpp | 276-279 | destructor | - | ?
