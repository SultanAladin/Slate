//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 `10` §1 — vector streams translated into `52`'s accepted subset, with every refusal named and positioned.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Format/VectorCodec/Source
%layer      SlateDocument
%sources    1
%symbols    13
%annotated  8/13
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S VectorCodec.cpp | 629 lines | 8446dcdb | 13 sym | `10` §1 — vector streams translated into `52`'s accepted subset, with every refusal named and positioned.

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE REFUSED SET
//------------------------------------------------------------------------------------------------------------------------

T RefusedElement    | VectorCodec.cpp | 25-30   | - | - | ?
    has   Spelling  const char*    [-]  ?
    has   Reason    RefusalReason  [-]  ?
    has   Detail    const char*    [-]  ?

V StrokedDetail     | VectorCodec.cpp | 48      | - | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE SCANNING
//------------------------------------------------------------------------------------------------------------------------

F Whitespace        | VectorCodec.cpp | 54-57   | - | - | ?
    in    Carried  char  [-]  ?
    out   -        bool  [-]  ?

F SkipSeparators    | VectorCodec.cpp | 60-65   | - | - | Advances past every separator, so a run of commands may be spaced however the source spaced it.
    in    Reading  const std::string&  [-]  ?
    in    Ordinal  std::size_t         [-]  ?
    out   -        std::size_t         [-]  ?

F ReadOrdinate      | VectorCodec.cpp | 71-117  | - | - | Reads one real from the path data, reporting whether one was there to read. without separators — "1.5.5" is two numbers — and a conversion that consumed as much as it could would take both. The scan below stops at the second decimal point, which is what the grammar means.
    in    Reading   const std::string&  [-]  ?
    in    Ordinal   std::size_t&        [-]  ?
    in    Produced  double&             [-]  ?
    out   -         bool                [-]  ?
    note  📝 Read here rather than through the standard conversions because a path's numbers run together

F ReadFlag          | VectorCodec.cpp | 120-134 | - | - | Reads one flag — a single digit, which the arc grammar writes without a separator after it.
    in    Reading   const std::string&  [-]  ?
    in    Ordinal   std::size_t&        [-]  ?
    in    Produced  bool&               [-]  ?
    out   -         bool                [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      ONE PATH RUN
//------------------------------------------------------------------------------------------------------------------------

T PathReading       | VectorCodec.cpp | 141-148 | - | - | Holds the state one run of path data is translated against — where it is, and what it last curved with.
    has   Position            PlanarPosition  [-]  ?
    has   Beginning           PlanarPosition  [-]  ?
    has   LastControl         PlanarPosition  [-]  ?
    has   CubicPreceding      bool            [-]  ?
    has   QuadraticPreceding  bool            [-]  ?

F TranslatePathData | VectorCodec.cpp | 154-447 | - | - | Translates one `d` attribute into the closed and open paths it declares. interior is on, and the artist reads that as the fill having moved rather than the path having been altered by something they cannot see.
    in    PathData   const std::string&         [-]  ?
    in    Rule       FillRule                   [-]  ?
    in    Appending  std::vector<OutlinePath>&  [-]  ?
    out   -          void                       [-]  ?
    note  🔴 A subpath that never closes stays open. `52` §1: closing it silently moves which side of it the

//------------------------------------------------------------------------------------------------------------------------
//                                                ELEMENTS AND ATTRIBUTES
//------------------------------------------------------------------------------------------------------------------------

F AttributeValue    | VectorCodec.cpp | 454-484 | - | - | Reads one attribute's value out of an element's own text, empty where the element declares none.
    in    Element    const std::string&  [-]  ?
    in    Attribute  const char*         [-]  ?
    out   -          std::string         [-]  ?

F ElementNamed      | VectorCodec.cpp | 487-500 | - | - | Whether an element's opening tag names one spelling.
    in    Element   const std::string&  [-]  ?
    in    Spelling  const char*         [-]  ?
    out   -         bool                [-]  ?

F TranslateSource   | VectorCodec.cpp | 503-580 | - | - | Translates one whole vector source, whichever route it arrived by.
    in    Source  const std::string&       [-]  ?
    out   -       Deliver<DecodedOutline>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE TRANSLATION
//------------------------------------------------------------------------------------------------------------------------

F Translate         | VectorCodec.cpp | 588-606 | - | - | ?
    in    Stream      const std::vector<std::uint8_t>&  [-]  ?
    in    OriginPath  const std::string&                [-]  ?
    out   -           Deliver<DecodedOutline>           [-]  ?
    by    Api/ImageCodec.h, Api/SpatialManipulator.h, Api/TopologyCodec.h, Api/TypefaceCodec.h, Api/VectorCodec.h, Source/ImageCodec.cpp, (+3 more)

F TranslateText     | VectorCodec.cpp | 608-627 | - | - | ?
    in    SourceText  const std::string&       [-]  ?
    out   -           Deliver<DecodedOutline>  [-]  ?
    by    Api/VectorCodec.h
