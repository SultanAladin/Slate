//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 A coordinate and the space it is a coordinate in — never a bare triple, and never an assumed encoding.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Numeric/ColourProjection/Api
%layer      SlateMath
%sources    1
%symbols    23
%annotated  14/23
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ColourProjection.h | 210 lines | 401282be | 23 sym | A coordinate and the space it is a coordinate in — never a bare triple, and never an assumed encoding.

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE TRANSFERS
//------------------------------------------------------------------------------------------------------------------------

E TransferSubject                         | ColourProjection.h | 26-32   | contract                      | -  | The encoding transfer a colour space applies between its linear light and its stored code. `Linear` here carries no transfer, which is what lets a working space be wide and linear while a display space is neither.
    has   Linear         TransferSubject  [-]  ?
    has   Companded      TransferSubject  [-]  ?
    has   PureExponent   TransferSubject  [-]  ?
    has   TransferCount  TransferSubject  [-]  ?
    by    Source/ColourProjection.cpp, Source/DisplayProjection.cpp
    note  🔴 `66` §4 applies the output transfer exactly once in the whole engine. A space that declares

//------------------------------------------------------------------------------------------------------------------------
//                                                       ONE SPACE
//------------------------------------------------------------------------------------------------------------------------

T ColourSpaceSpecification                | ColourProjection.h | 45-62   | nonallocating,nonthrowing     | -  | One colour space: three primaries, a white point, and the transfer it stores its code through. rather than declared as a matrix. A stored matrix is a second representation of the primaries and it drifts from them the moment either is amended. differ are never treated as one because their chromaticities happened to agree to six places.
    has   SpaceIdentity     std::uint32_t    [-]  ?
    has   RedX              double           [-]  ?
    has   RedY              double           [-]  ?
    has   GreenX            double           [-]  ?
    has   GreenY            double           [-]  ?
    has   BlueX             double           [-]  ?
    has   BlueY             double           [-]  ?
    has   WhiteX            double           [-]  ?
    has   WhiteY            double           [-]  ?
    has   Transfer          TransferSubject  [-]  ?
    has   TransferExponent  double           [-]  ?
    by    Api/AtmosphereIntegrator.h, Api/DisplayProjection.h, Api/IlluminantPopulation.h, Source/AtmosphereIntegrator.cpp, Source/ColourProjection.cpp, Source/ConsoleHost.cpp, (+2 more)
    note  📐 Primaries and white are chromaticities, so the projection between two spaces is derived from these
    note  🔴 Every space carries an identity, and the identity is what `36` §7 compares at Exact. Two spaces that

F ColourSpaceSpecification::SpaceDeclared | ColourProjection.h | 61      | -                             | ✔️ | Whether this specification names a space at all.
    out   -  constexpr bool  [-]  ?
    by    Api/AssetInterchange.h, Source/AssetInterchange.cpp, Source/ColourProjection.cpp, Source/DisplayProjection.cpp, Source/ImageCodec.cpp

V WorkingSpaceIdentity                    | ColourProjection.h | 66      | -                             | -  | ?
    by    Source/AssetInterchange.cpp, Source/ConsoleHost.cpp

V DisplaySpaceIdentity                    | ColourProjection.h | 67      | -                             | -  | ?
    by    Source/ConsoleHost.cpp, Source/IntersectionOutline.cpp, Source/OverlayProjection.cpp, Source/SpatialManipulator.cpp, Source/ThemeSpecification.cpp

F DeclaredWorkingSpace                    | ColourProjection.h | 74-89   | api,nonallocating,nonthrowing | ✔️ | The wide linear working space a document is created with. wide-gamut set; `36` §9 leaves which set open and this is a constant, not a shape.
    out   -  constexpr ColourSpaceSpecification  [-]  ?
    by    Api/DisplayProjection.h, Source/ConsoleHost.cpp
    note  Wide enough that a saturated illuminant does not clip on entry — `36` §2. The primaries are the common

F DeclaredDisplaySpace                    | ColourProjection.h | 96-103  | api,nonallocating,nonthrowing | ✔️ | The display space, companded, as a build default until `36` §9's open row is answered. produces an image that is correct on exactly one monitor.
    out   -  constexpr ColourSpaceSpecification  [-]  ?
    by    Api/DisplayProjection.h, Source/ConsoleHost.cpp
    note  🔴 Queried or declared per `36` §9 and **never assumed to be the working space**. Assuming they match

//------------------------------------------------------------------------------------------------------------------------
//                                                       ONE COLOUR
//------------------------------------------------------------------------------------------------------------------------

T ColourSpecification                     | ColourProjection.h | 115-125 | nonallocating,nonthrowing     | -  | A coordinate together with the space it is expressed in. three subsystems will each interpret differently, and all three will look plausible. and clamping here would compress a radiance before `66` had a chance to project it.
    has   RedCoordinate    double         [-]  ?
    has   GreenCoordinate  double         [-]  ?
    has   BlueCoordinate   double         [-]  ?
    has   SpaceIdentity    std::uint32_t  [-]  ?
    by    Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/DisplayProjection.h, Api/IlluminantPopulation.h, Api/IntersectionOutline.h, Api/MaterialSpecification.h, (+18 more)
    note  🔴 `36` §1: there is no bare triple anywhere in Slate. A colour without its space is a number that
    note  Coordinates are held at 64-bit and are not clamped. An emission channel is unbounded above — `18` §2 —

F ColourSpecification::ColourDeclared     | ColourProjection.h | 124     | -                             | ✔️ | Whether this colour names the space it is a coordinate in.
    out   -  constexpr bool  [-]  ?
    by    Api/BrushSpecification.h, Api/VectorInterchange.h, Source/AnalyticProjection.cpp, Source/AtmosphereIntegrator.cpp, Source/BrushSpecification.cpp, Source/ColourProjection.cpp, (+11 more)

F SpacesAgree                             | ColourProjection.h | 131-134 | api,nonallocating,nonthrowing | ✔️ | Whether two colours are expressed in the same space.
    in    LeftColour   ColourSpecification  [-]  ?
    in    RightColour  ColourSpecification  [-]  ?
    out   -            constexpr bool       [-]  ?
    note  An integer comparison at Exact — `36` §7. A mistaken match converts nothing and is therefore silent.

F SLATE_DECLARES_PRECISION                | ColourProjection.h | 135     | -                             | -  | ?
    in    Exact  PrecisionGuarantee::  [-]  ?
    in    Exact  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PROJECTIONS
//------------------------------------------------------------------------------------------------------------------------

F Project                                 | ColourProjection.h | 150     | api,nonallocating,nonthrowing | ✔️ | Projects one colour into a declared space, transfers and white point included. white point, encode the target transfer. Exposing the four apart invites a caller to omit one, and the omission that matters — the transfer — produces an image that is merely "a bit washed out".
    in    Arriving       ColourSpecification              [-]  the colour, carrying the space it is a coordinate in
    in    ArrivingSpace  const ColourSpaceSpecification&  [-]  ?
    in    Target         const ColourSpaceSpecification&  [-]  the space to express it in
    out   -              Outcome                          [-]  refuses with ContentUnsupported when either space is undeclared
    by    Api/DisplayProjection.h, Api/QuadratureIntegrator.h, Api/SpectralProjection.h, Api/TickSequence.h, Api/TransformProjection.h, Api/VisibilityRaster.h, (+12 more)
    note  🔴 The whole conversion in one call: decode the arriving transfer, project the primaries, adapt the

F SLATE_DECLARES_PRECISION                | ColourProjection.h | 153     | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)

F ProjectTristimulus                      | ColourProjection.h | 170     | api,nonallocating,nonthrowing | ✔️ | Projects one tristimulus coordinate into a declared space, encoding its transfer. own; whether it needs adapting is a fact about the spectrum that produced it, which this routine cannot see. `ProjectTemperature` adapts before calling here, because a locus coordinate **is** a white point — and that is the one case where the adaptation is knowable at this depth. space without re-deriving the primaries. `AdaptWhite` already crosses this seam in tristimulus, so nothing new is exposed by it.
    in    TristimulusX  double                           [-]  as `SpectralProjection` produced it
    in    TristimulusY  double                           [-]  ?
    in    TristimulusZ  double                           [-]  ?
    in    Target        const ColourSpaceSpecification&  [-]  the space to express it in
    in    TristimulusY  -                                [-]  ?
    in    TristimulusZ  -                                [-]  ?
    out   -             Outcome                          [-]  refuses with ContentUnsupported for an undeclared or degenerate target space
    by    Source/AtmosphereIntegrator.cpp, Source/ColourProjection.cpp
    note  🔴 No white adaptation is applied. A tristimulus coordinate is absolute and carries no white of its
    note  📝 Declared so that `28` may resolve a spectrally projected extinction coefficient into the working

F SLATE_DECLARES_PRECISION                | ColourProjection.h | 174     | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)

F Encode                                  | ColourProjection.h | 180     | api,nonallocating,nonthrowing | ✔️ | Applies one space's encoding transfer to a linear coordinate.
    in    Space            const ColourSpaceSpecification&  [-]  ?
    in    LinearMagnitude  double                           [-]  linear light; negative magnitudes are transferred by odd reflection
    out   -                double                           [-]  ?
    by    Source/ColourProjection.cpp, Source/ConsoleHost.cpp

F SLATE_DECLARES_PRECISION                | ColourProjection.h | 181     | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)

F Decode                                  | ColourProjection.h | 186     | api,nonallocating,nonthrowing | ✔️ | Removes one space's encoding transfer, returning linear light.
    in    Space       const ColourSpaceSpecification&  [-]  ?
    in    StoredCode  double                           [-]  ?
    out   -           double                           [-]  ?
    by    Source/ColourProjection.cpp, Source/ConsoleHost.cpp

F SLATE_DECLARES_PRECISION                | ColourProjection.h | 187     | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)

F AdaptWhite                              | ColourProjection.h | 194     | api,nonallocating,nonthrowing | ✔️ | Adapts a tristimulus coordinate from one white point to another. shifts hue on every saturated colour, which is visible exactly where an artist notices it.
    in    ArrivingWhiteX  double   [-]  ?
    in    ArrivingWhiteY  double   [-]  ?
    in    TargetWhiteX    double   [-]  ?
    in    TargetWhiteY    double   [-]  ?
    in    TristimulusX    double&  [-]  ?
    in    TristimulusY    double&  [-]  ?
    in    TristimulusZ    double&  [-]  ?
    out   -               void     [-]  ?
    by    Source/ColourProjection.cpp
    note  📐 Von Kries adaptation in a declared cone response space. Adapting by scaling tristimulus directly

F SLATE_DECLARES_PRECISION                | ColourProjection.h | 197     | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)

F ProjectTemperature                      | ColourProjection.h | 206     | api,nonallocating,nonthrowing | ✔️ | Derives a white point coordinate from a declared correlated colour temperature. 5600 expects to see 5600 when they return, and a coordinate cannot be inverted back to it exactly.
    in    Temperature  double                           [K]  1667 to 25000; outside that the locus approximation is refused
    in    Target       const ColourSpaceSpecification&  [-]  ?
    out   -            Outcome                          [-]  refuses with ContentUnsupported outside the declared interval
    by    Source/ColourProjection.cpp, Source/ConsoleHost.cpp, Source/IlluminantPopulation.cpp
    note  🔴 `36` §5: the temperature is retained as the authored value by whoever declared it. An artist who set

F SLATE_DECLARES_PRECISION                | ColourProjection.h | 208     | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)
