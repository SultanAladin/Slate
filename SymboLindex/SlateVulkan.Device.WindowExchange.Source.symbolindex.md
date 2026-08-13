//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The surface conversion, taken through the window system that produced the handle.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/WindowExchange/Source
%layer      SlateVulkan
%sources    1
%symbols    2
%annotated  0/2
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S WindowExchange.cpp | 50 lines | 8527fdcc | 2 sym | The surface conversion, taken through the window system that produced the handle.

//------------------------------------------------------------------------------------------------------------------------
//                                                       CONVERSION
//------------------------------------------------------------------------------------------------------------------------

F Convert | WindowExchange.cpp | 20-36 | - | - | ?
    in    Instance      VkInstance             [-]  ?
    in    NativeHandle  void*                  [-]  ?
    out   -             Outcome<VkSurfaceKHR>  [-]  ?
    by    Api/WindowExchange.h

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F Reclaim | WindowExchange.cpp | 42-48 | - | - | ?
    in    Instance             VkInstance    [-]  ?
    in    PresentationSurface  VkSurfaceKHR  [-]  ?
    out   -                    void          [-]  ?
    by    Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CodeInterchange.h, Api/CommandSequence.h, Api/CycleScheduler.h, Api/DepthReduction.h, (+75 more)
