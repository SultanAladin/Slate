//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 One window surface over three window systems — surrenders the native handle and nothing else.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Platform/WindowInterchange/Api
%layer      SlateMath
%sources    1
%symbols    8
%annotated  7/8
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S WindowInterchange.h | 83 lines | 2e1a2931 | 8 sym | One window surface over three window systems — surrenders the native handle and nothing else.

//------------------------------------------------------------------------------------------------------------------------
//                                                     DISPLAY EXTENT
//------------------------------------------------------------------------------------------------------------------------

T DisplayExtent                         | WindowInterchange.h | 23-27 | nonallocating,nonthrowing     | -  | The extent of a window's drawable area. the intermediates; nothing here queues them.
    has   Width   std::uint32_t  [-]  ?
    has   Height  std::uint32_t  [-]  ?
    by    Source/WindowInterchange.cpp
    note  ⚠️ A drag produces a new extent many times a second. `06` §4.1 takes the extent once and discards

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE WINDOW
//------------------------------------------------------------------------------------------------------------------------

T WindowInterchange                     | WindowInterchange.h | 38-81 | owning                        | -  | A native window over the host window system. is, includes no Vulkan header, and names no presentation chain. `06`'s `WindowExchange` converts the handle; the split is what keeps `SlateMath` device-free.
    has   WindowSlot    void*          [-]  ?
    has   DrawExtent    DisplayExtent  [-]  ?
    has   ClosurePosed  bool           [-]  ?
    by    Source/WindowInterchange.cpp
    note  🔴 This component surrenders the native handle and nothing else. It does not know what a surface

F WindowInterchange::~WindowInterchange | WindowInterchange.h | 45    | destructor                    | -  | ?

F WindowInterchange::Open               | WindowInterchange.h | 53    | api,nonthrowing               | 🚩 | Opens a window of the requested extent and surrenders nothing until it succeeds.
    in    RequestedExtent  DisplayExtent  [px]  the drawable extent asked of the window system
    in    WindowTitle      const char*    [-]   static text; never allocated, never retained beyond the call
    out   -                Outcome        [-]   refuses with HostDenied when the window system declines
    by    Api/CameraProjection.h, Api/CommandSequence.h, Api/DecalProjection.h, Api/DocumentSession.h, Api/EmissionSequence.h, Api/HardwareMetrics.h, (+20 more)

F WindowInterchange::Drain              | WindowInterchange.h | 58    | api,nonthrowing               | ✔️ | Drains the window system's pending messages into this window's recorded condition.
    out   -  void  [-]  ?
    by    Api/RequestQueue.h, Api/StorageExchange.h, Api/WorkSequence.h, Source/ConsoleHost.cpp, Source/RequestQueue.cpp, Source/StorageExchange.cpp, (+2 more)

F WindowInterchange::NativeHandle       | WindowInterchange.h | 64    | api,nonallocating,nonthrowing | ✔️ | The opaque native handle, for `06`'s `WindowExchange` and for nothing else.
    out   -  NativeHandle  [-]  null while no window is open
    by    Api/WindowExchange.h, Source/WindowExchange.cpp, Source/WindowInterchange.cpp

F WindowInterchange::CurrentExtent      | WindowInterchange.h | 69    | api,nonallocating,nonthrowing | ✔️ | The current drawable extent.
    out   -  DisplayExtent  [-]  ?
    by    Source/WindowInterchange.cpp

F WindowInterchange::ClosureRequested   | WindowInterchange.h | 74    | api,nonallocating,nonthrowing | ✔️ | Whether the artist has asked the window system to close this window.
    out   -  bool  [-]  ?
    by    Source/WindowInterchange.cpp
