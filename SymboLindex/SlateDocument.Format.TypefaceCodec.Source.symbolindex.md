//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 `10` §1 — typeface streams translated into glyph outlines and the metrics that position them.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Format/TypefaceCodec/Source
%layer      SlateDocument
%sources    1
%symbols    7
%annotated  2/7
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S TypefaceCodec.cpp | 245 lines | e0e679d8 | 7 sym | `10` §1 — typeface streams translated into glyph outlines and the metrics that position them.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

K STBTT_STATIC                | TypefaceCodec.cpp | 13      | - | - | ?

K STB_TRUETYPE_IMPLEMENTATION | TypefaceCodec.cpp | 14      | - | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE UNIT RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

V AssumedUnitsPerEm           | TypefaceCodec.cpp | 31      | - | - | ?

F ResolveUnitsPerEm           | TypefaceCodec.cpp | 34-41   | - | - | Reads a typeface's units per em from its heading, falling back where the heading does not carry one.
    in    Reading  const stbtt_fontinfo&  [-]  ?
    out   -        double                 [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    ONE GLYPH SHAPE
//------------------------------------------------------------------------------------------------------------------------

F TranslateShape              | TypefaceCodec.cpp | 51-108  | - | - | Converts one glyph's contour run into the path run `52` §2 accepts, in the typeface's own units. contour is closed by construction — an open one would have no interior — so the fill rule below is NonZero for every path, which is the rule outlines are authored under.
    in    Contours      const stbtt_vertex*        [-]  ?
    in    ContourCount  int                        [-]  ?
    in    Appending     std::vector<OutlinePath>&  [-]  ?
    out   -             void                       [-]  ?
    note  🔴 A contour is closed on the move that begins the next one, and on the end of the run. A typeface

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE TRANSLATION
//------------------------------------------------------------------------------------------------------------------------

F Translate                   | TypefaceCodec.cpp | 116-207 | - | - | ?
    in    Stream        const std::vector<std::uint8_t>&  [-]  ?
    in    GlyphCeiling  std::uint32_t                     [-]  ?
    out   -             Outcome<DecodedTypeface>          [-]  ?
    by    Api/ImageCodec.h, Api/SpatialManipulator.h, Api/TopologyCodec.h, Api/TypefaceCodec.h, Api/VectorCodec.h, Source/ImageCodec.cpp, (+3 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE SUBSTITUTION
//------------------------------------------------------------------------------------------------------------------------

F ResolveCodepoint            | TypefaceCodec.cpp | 213-243 | - | - | ?
    in    Stream     const std::vector<std::uint8_t>&  [-]  ?
    in    Codepoint  std::uint32_t                     [-]  ?
    out   -          Outcome<std::uint32_t>            [-]  ?
    by    Api/TypefaceCodec.h
