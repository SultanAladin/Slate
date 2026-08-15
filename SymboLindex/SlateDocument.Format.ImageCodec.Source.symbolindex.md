//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 `10` §1 — image streams translated to the texels the file carried, at the depth it carried them, and nothing else.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Format/ImageCodec/Source
%layer      SlateDocument
%sources    1
%symbols    15
%annotated  2/15
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ImageCodec.cpp | 231 lines | 222027f5 | 15 sym | `10` §1 — image streams translated to the texels the file carried, at the depth it carried them, and nothing else.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

K STB_IMAGE_IMPLEMENTATION   | ImageCodec.cpp | 15      | - | - | ?

K STBI_NO_STDIO              | ImageCodec.cpp | 16      | - | - | ?

K STBI_NO_GIF                | ImageCodec.cpp | 17      | - | - | ?

K STBI_NO_PIC                | ImageCodec.cpp | 18      | - | - | ?

K STBI_NO_PNM                | ImageCodec.cpp | 19      | - | - | ?

K STBI_NO_PSD                | ImageCodec.cpp | 20      | - | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SIGNATURES
//------------------------------------------------------------------------------------------------------------------------

V PortableNetworkSignature   | ImageCodec.cpp | 34      | - | - | ?

V JointPhotographicSignature | ImageCodec.cpp | 35      | - | - | ?

V RadianceSignature          | ImageCodec.cpp | 37      | - | - | ?

V RadianceSignatureShort     | ImageCodec.cpp | 38      | - | - | ?

F LeadingMatches             | ImageCodec.cpp | 41-46   | - | - | Whether the leading bytes begin with one declared signature.
    in    Leading    const std::vector<std::uint8_t>&  [-]  ?
    in    Signature  const std::uint8_t*               [-]  ?
    in    Spanned    std::size_t                       [-]  ?
    out   -          bool                              [-]  ?

F LeadingMatchesText         | ImageCodec.cpp | 49-56   | - | - | Whether the leading bytes begin with one declared textual signature.
    in    Leading    const std::vector<std::uint8_t>&  [-]  ?
    in    Signature  const char*                       [-]  ?
    out   -          bool                              [-]  ?

F TruevisionPlausible        | ImageCodec.cpp | 62-73   | - | - | ?
    in    Leading  const std::vector<std::uint8_t>&  [-]  ?
    out   -        bool                              [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE CLASSIFICATION
//------------------------------------------------------------------------------------------------------------------------

F ClassifyContent            | ImageCodec.cpp | 81-104  | - | - | ?
    in    Leading  const std::vector<std::uint8_t>&  [-]  ?
    out   -        ImageContentSubject               [-]  ?
    by    Api/ImageCodec.h, Api/TopologyCodec.h, Source/TopologyCodec.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE TRANSLATION
//------------------------------------------------------------------------------------------------------------------------

F Translate                  | ImageCodec.cpp | 110-229 | - | - | ?
    in    Stream      const std::vector<std::uint8_t>&  [-]  ?
    in    OriginPath  const std::string&                [-]  ?
    out   -           Deliver<DecodedImage>             [-]  ?
    by    Api/ImageCodec.h, Api/SpatialManipulator.h, Api/TopologyCodec.h, Api/TypefaceCodec.h, Api/VectorCodec.h, Source/SpatialManipulator.cpp, (+3 more)
