//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The claim, the host write, the recorded transfer and the release of every linear device extent the engine holds.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/SpanSpace/Source
%layer      SlateVulkan
%sources    1
%symbols    11
%annotated  0/11
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S SpanSpace.cpp | 301 lines | 3f6c0515 | 11 sym | The claim, the host write, the recorded transfer and the release of every linear device extent the engine holds.

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F SpanSpace::~SpanSpace   | SpanSpace.cpp | 17-20   | destructor | - | ?

F SpanSpace::Construct    | SpanSpace.cpp | 22-31   | -          | - | ?
    in    Exchange      const VulkanExchange&  [-]  ?
    in    BackingSpace  ByteSpace&             [-]  ?
    out   -             Outcome<bool>          [-]  ?

F SpanSpace::UsageOf      | SpanSpace.cpp | 33-60   | -          | - | ?
    in    Intent  SpanIntent          [-]  ?
    out   -       VkBufferUsageFlags  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE CLAIM
//------------------------------------------------------------------------------------------------------------------------

F SpanSpace::Claim        | SpanSpace.cpp | 66-155  | -          | - | ?
    in    Declared  const SpanShape&    [-]  ?
    out   -         Outcome<SpanClaim>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE WRITES
//------------------------------------------------------------------------------------------------------------------------

F SpanSpace::Amend        | SpanSpace.cpp | 161-188 | -          | - | ?
    in    SpanOrdinal    std::uint32_t  [-]  ?
    in    Arriving       const void*    [-]  ?
    in    ArrivingBytes  VkDeviceSize   [-]  ?
    in    ByteOffset     VkDeviceSize   [-]  ?
    out   -              Outcome<bool>  [-]  ?

F SpanSpace::Transfer     | SpanSpace.cpp | 190-222 | -          | - | ?
    in    Recorded       VkCommandBuffer  [-]  ?
    in    SourceOrdinal  std::uint32_t    [-]  ?
    in    TargetOrdinal  std::uint32_t    [-]  ?
    in    TransferBytes  VkDeviceSize     [-]  ?
    out   -              Outcome<bool>    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE READS
//------------------------------------------------------------------------------------------------------------------------

F SpanSpace::Standing     | SpanSpace.cpp | 228-242 | -          | - | ?
    in    SpanOrdinal  std::uint32_t       [-]  ?
    out   -            Outcome<SpanClaim>  [-]  ?

F SpanSpace::ClaimedCount | SpanSpace.cpp | 244-255 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F SpanSpace::ClaimedBytes | SpanSpace.cpp | 257-268 | -          | - | ?
    out   -  VkDeviceSize  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F SpanSpace::Release      | SpanSpace.cpp | 274-291 | -          | - | ?
    in    SpanOrdinal  std::uint32_t  [-]  ?
    out   -            void           [-]  ?

F SpanSpace::Reclaim      | SpanSpace.cpp | 293-299 | -          | - | ?
    out   -  void  [-]  ?
