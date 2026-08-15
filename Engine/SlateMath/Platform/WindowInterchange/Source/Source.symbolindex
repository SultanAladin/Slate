//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Windowing over GLFW, linked dynamically through glfw3dll.lib against glfw3.dll.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Platform/WindowInterchange/Source
%layer      SlateMath
%sources    1
%symbols    9
%annotated  0/9
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S WindowInterchange.cpp | 126 lines | be931b03 | 9 sym | Windowing over GLFW, linked dynamically through glfw3dll.lib against glfw3.dll.

//------------------------------------------------------------------------------------------------------------------------
//                                                 WINDOW SYSTEM LIFETIME
//------------------------------------------------------------------------------------------------------------------------

V OpenWindowCount                       | WindowInterchange.cpp | 23      | -          | - | ?

F AcquireWindowSystem                   | WindowInterchange.cpp | 25-32   | -          | - | ?
    out   -  bool  [-]  ?

F ReleaseWindowSystem                   | WindowInterchange.cpp | 34-43   | -          | - | ?
    out   -  void  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     OPEN AND CLOSE
//------------------------------------------------------------------------------------------------------------------------

F WindowInterchange::Open               | WindowInterchange.cpp | 50-77   | -          | - | ?
    in    RequestedExtent  DisplayExtent  [-]  ?
    in    WindowTitle      const char*    [-]  ?
    out   -                Deliver<bool>  [-]  ?

F WindowInterchange::~WindowInterchange | WindowInterchange.cpp | 79-87   | destructor | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    DRAIN AND REPORT
//------------------------------------------------------------------------------------------------------------------------

F WindowInterchange::Drain              | WindowInterchange.cpp | 93-109  | -          | - | ?
    out   -  void  [-]  ?

F WindowInterchange::NativeHandle       | WindowInterchange.cpp | 111-114 | -          | - | ?
    out   -  void*  [-]  ?

F WindowInterchange::CurrentExtent      | WindowInterchange.cpp | 116-119 | -          | - | ?
    out   -  DisplayExtent  [-]  ?

F WindowInterchange::ClosureRequested   | WindowInterchange.cpp | 121-124 | -          | - | ?
    out   -  bool  [-]  ?
