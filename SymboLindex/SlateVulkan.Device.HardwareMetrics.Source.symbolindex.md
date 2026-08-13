//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The timestamp extent, the recorded pair around each declared span, and the readback that reports or refuses.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/HardwareMetrics/Source
%layer      SlateVulkan
%sources    1
%symbols    13
%annotated  0/13
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S HardwareMetrics.cpp | 329 lines | 9605d816 | 13 sym | The timestamp extent, the recorded pair around each declared span, and the readback that reports or refuses.

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F HardwareMetrics::Construct          | HardwareMetrics.cpp | 17-55   | -          | - | ?
    in    Exchange  const VulkanExchange&  [-]  ?
    out   -         Outcome<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DECLARATION
//------------------------------------------------------------------------------------------------------------------------

F HardwareMetrics::TimestampOrdinalOf | HardwareMetrics.cpp | 61-64   | -          | - | ?
    in    RotationSlot  std::uint32_t  [-]  ?
    in    SpanOrdinal   std::uint32_t  [-]  ?
    out   -             std::uint32_t  [-]  ?

F HardwareMetrics::Declare            | HardwareMetrics.cpp | 66-96   | -          | - | ?
    in    SpanName  const char*             [-]  ?
    out   -         Outcome<std::uint32_t>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RECORDING
//------------------------------------------------------------------------------------------------------------------------

F HardwareMetrics::Clear              | HardwareMetrics.cpp | 102-119 | -          | - | ?
    in    Recorded      VkCommandBuffer  [-]  ?
    in    RotationSlot  std::uint32_t    [-]  ?
    out   -             Outcome<bool>    [-]  ?

F HardwareMetrics::Open               | HardwareMetrics.cpp | 121-153 | -          | - | ?
    in    Recorded      VkCommandBuffer  [-]  ?
    in    RotationSlot  std::uint32_t    [-]  ?
    in    SpanOrdinal   std::uint32_t    [-]  ?
    out   -             Outcome<bool>    [-]  ?

F HardwareMetrics::Close              | HardwareMetrics.cpp | 155-190 | -          | - | ?
    in    Recorded      VkCommandBuffer  [-]  ?
    in    RotationSlot  std::uint32_t    [-]  ?
    in    SpanOrdinal   std::uint32_t    [-]  ?
    out   -             Outcome<bool>    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE READBACK
//------------------------------------------------------------------------------------------------------------------------

F HardwareMetrics::Resolve            | HardwareMetrics.cpp | 196-267 | -          | - | ?
    in    RotationSlot    std::uint32_t  [-]  ?
    in    CompletedCount  std::uint64_t  [-]  ?
    out   -               Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE READINGS
//------------------------------------------------------------------------------------------------------------------------

F HardwareMetrics::Standing           | HardwareMetrics.cpp | 273-279 | -          | - | ?
    in    SpanOrdinal  std::uint32_t          [-]  ?
    out   -            Outcome<MeasuredSpan>  [-]  ?

F HardwareMetrics::Report             | HardwareMetrics.cpp | 281-293 | -          | - | ?
    in    Sampled  MeasureIndex&  [-]  ?
    in    Arrival  TickPoint      [-]  ?
    out   -        void           [-]  ?

F HardwareMetrics::Measuring          | HardwareMetrics.cpp | 295-298 | -          | - | ?
    out   -  bool  [-]  ?

F HardwareMetrics::DeclaredCount      | HardwareMetrics.cpp | 300-303 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F HardwareMetrics::Reclaim            | HardwareMetrics.cpp | 309-322 | -          | - | ?
    out   -  void  [-]  ?

F HardwareMetrics::~HardwareMetrics   | HardwareMetrics.cpp | 324-327 | destructor | - | ?
