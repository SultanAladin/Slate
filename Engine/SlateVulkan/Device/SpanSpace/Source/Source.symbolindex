//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The claim, the host write, the recorded transfer and the release of every linear device extent the engine holds.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/SpanSpace/Source
%layer      SlateVulkan
%sources    1
%symbols    12
%annotated  0/12
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S SpanSpace.cpp | 329 lines | 82b6a6cb | 12 sym | The claim, the host write, the recorded transfer and the release of every linear device extent the engine holds.

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F SpanSpace::~SpanSpace   | SpanSpace.cpp | 17-20   | destructor | - | ?

F SpanSpace::Construct    | SpanSpace.cpp | 22-34   | -          | - | ?
    in    Exchange      const VulkanExchange&       [-]  ?
    in    BackingSpace  ByteSpace&                  [-]  ?
    in    Naming        const DiagnosticExtension&  [-]  ?
    out   -             Deliver<bool>               [-]  ?

F SpanSpace::NameOf       | SpanSpace.cpp | 36-51   | -          | - | ?
    in    Intent  SpanIntent   [-]  ?
    out   -       const char*  [-]  ?

F SpanSpace::UsageOf      | SpanSpace.cpp | 53-80   | -          | - | ?
    in    Intent  SpanIntent          [-]  ?
    out   -       VkBufferUsageFlags  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE CLAIM
//------------------------------------------------------------------------------------------------------------------------

F SpanSpace::Claim        | SpanSpace.cpp | 86-183  | -          | - | ?
    in    Declared  const SpanShape&    [-]  ?
    out   -         Deliver<SpanClaim>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE WRITES
//------------------------------------------------------------------------------------------------------------------------

F SpanSpace::Amend        | SpanSpace.cpp | 189-216 | -          | - | ?
    in    SpanOrdinal    std::uint32_t  [-]  ?
    in    Arriving       const void*    [-]  ?
    in    ArrivingBytes  VkDeviceSize   [-]  ?
    in    ByteOffset     VkDeviceSize   [-]  ?
    out   -              Deliver<bool>  [-]  ?

F SpanSpace::Transfer     | SpanSpace.cpp | 218-250 | -          | - | ?
    in    Recorded       VkCommandBuffer  [-]  ?
    in    SourceOrdinal  std::uint32_t    [-]  ?
    in    TargetOrdinal  std::uint32_t    [-]  ?
    in    TransferBytes  VkDeviceSize     [-]  ?
    out   -              Deliver<bool>    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE READS
//------------------------------------------------------------------------------------------------------------------------

F SpanSpace::Standing     | SpanSpace.cpp | 256-270 | -          | - | ?
    in    SpanOrdinal  std::uint32_t       [-]  ?
    out   -            Deliver<SpanClaim>  [-]  ?

F SpanSpace::ClaimedCount | SpanSpace.cpp | 272-283 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F SpanSpace::ClaimedBytes | SpanSpace.cpp | 285-296 | -          | - | ?
    out   -  VkDeviceSize  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F SpanSpace::Release      | SpanSpace.cpp | 302-319 | -          | - | ?
    in    SpanOrdinal  std::uint32_t  [-]  ?
    out   -            void           [-]  ?

F SpanSpace::Reclaim      | SpanSpace.cpp | 321-327 | -          | - | ?
    out   -  void  [-]  ?
