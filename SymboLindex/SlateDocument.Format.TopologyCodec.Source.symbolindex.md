//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 `10` §1 — polygon streams translated exactly as the file wrote them, n-gons and degeneracies included.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Format/TopologyCodec/Source
%layer      SlateDocument
%sources    1
%symbols    9
%annotated  1/9
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S TopologyCodec.cpp | 315 lines | 375955b9 | 9 sym | `10` §1 — polygon streams translated exactly as the file wrote them, n-gons and degeneracies included.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

K FAST_OBJ_IMPLEMENTATION | TopologyCodec.cpp | 12      | - | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE STREAM READ
//------------------------------------------------------------------------------------------------------------------------

T StreamReading           | TopologyCodec.cpp | 32-37   | - | - | ?
    has   Stream    const std::vector<std::uint8_t>*  [-]  ?
    has   Consumed  std::size_t                       [-]  ?
    has   Occupied  bool                              [-]  ?

F StreamOpen              | TopologyCodec.cpp | 39-51   | - | - | ?
    in    Path     const char*  [-]  ?
    in    Reading  void*        [-]  ?
    out   -        void*        [-]  ?

F StreamClose             | TopologyCodec.cpp | 53-60   | - | - | ?
    in    Stream   void*  [-]  ?
    in    Reading  void*  [-]  ?
    out   -        void   [-]  ?

F StreamRead              | TopologyCodec.cpp | 62-80   | - | - | ?
    in    Stream   void*        [-]  ?
    in    Landing  void*        [-]  ?
    in    Wanted   std::size_t  [-]  ?
    in    Reading  void*        [-]  ?
    out   -        std::size_t  [-]  ?

F StreamSpanned           | TopologyCodec.cpp | 82-91   | - | - | ?
    in    Stream   void*          [-]  ?
    in    Reading  void*          [-]  ?
    out   -        unsigned long  [-]  ?
    by    Api/StorageExchange.h, Source/StorageExchange.cpp

F SuffixMatches           | TopologyCodec.cpp | 94-115  | - | - | Whether an origin's suffix matches one declared spelling, compared without regard to case.
    in    OriginPath  const std::string&  [-]  ?
    in    Suffix      const char*         [-]  ?
    out   -           bool                [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE CLASSIFICATION
//------------------------------------------------------------------------------------------------------------------------

F ClassifyContent         | TopologyCodec.cpp | 123-128 | - | - | ?
    in    OriginPath  const std::string&      [-]  ?
    out   -           TopologyContentSubject  [-]  ?
    by    Api/ImageCodec.h, Api/TopologyCodec.h, Source/ImageCodec.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE TRANSLATION
//------------------------------------------------------------------------------------------------------------------------

F Translate               | TopologyCodec.cpp | 134-313 | - | - | ?
    in    Stream      const std::vector<std::uint8_t>&  [-]  ?
    in    OriginPath  const std::string&                [-]  ?
    out   -           Deliver<DecodedTopology>          [-]  ?
    by    Api/ImageCodec.h, Api/SpatialManipulator.h, Api/TopologyCodec.h, Api/TypefaceCodec.h, Api/VectorCodec.h, Source/ImageCodec.cpp, (+3 more)
