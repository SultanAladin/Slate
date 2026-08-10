//============================================================================================================================================
//                                                             SHADER.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The one uniform block every atmosphere entry point reads, and its widening into the shared medium profile.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/AtmosphereIntegrator/Shader
%layer      SlateCompute
%sources    5
%symbols    29
%annotated  7/29
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S AtmosphereUniform.slang    | 107 lines | f413601f | 3 sym  | The one uniform block every atmosphere entry point reads, and its widening into the shared medium profile.
S MultiScatterSurface.slang  | 204 lines | d4bb7b9c | 10 sym | ② of `28` §2 — the isotropic multiple-scattering series, closed, reading ① at 32 × 32.
S SkyRadiance.slang          | 129 lines | afca44a5 | 3 sym  | The sky along one view direction, with the analytic solar disc — what `18` §5.1's unoccupied class reads.
S SkyViewSurface.slang       | 190 lines | fa4663fc | 9 sym  | ③ of `28` §2 — single scattering phase-weighted against ①, plus ②'s isotropic term, at 192 × 108.
S TransmittanceSurface.slang | 105 lines | fbe42a29 | 4 sym  | ① of `28` §2 — the optical depth to the atmosphere boundary, exponentiated, at 256 × 64.

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE UNIFORM BLOCK
//------------------------------------------------------------------------------------------------------------------------

T MediumUniform              | AtmosphereUniform.slang    | 32-53  | contract                  | -  | The medium, the sun and the camera altitude, as the host uploads them. of the three declared layout rules. A vector member here would pad to sixteen bytes under std140 and to its own width elsewhere, and the host mirror would agree with exactly one of them.
    has   PlanetRadius             Real32  [-]  ?
    has   AtmosphereThickness      Real32  [-]  ?
    has   RayleighScaleHeight      Real32  [-]  ?
    has   MieScaleHeight           Real32  [-]  ?
    has   OzoneCentreAltitude      Real32  [-]  ?
    has   OzoneHalfWidth           Real32  [-]  ?
    has   MieAsymmetry             Real32  [-]  ?
    has   MieScattering            Real32  [-]  ?
    has   MieExtinction            Real32  [-]  ?
    has   RayleighScatteringRed    Real32  [-]  ?
    has   RayleighScatteringGreen  Real32  [-]  ?
    has   RayleighScatteringBlue   Real32  [-]  ?
    has   OzoneAbsorptionRed       Real32  [-]  ?
    has   OzoneAbsorptionGreen     Real32  [-]  ?
    has   OzoneAbsorptionBlue      Real32  [-]  ?
    has   SunDirectionX            Real32  [-]  ?
    has   SunDirectionY            Real32  [-]  ?
    has   SunDirectionZ            Real32  [-]  ?
    has   CameraAltitude           Real32  [-]  ?
    by    Shader/MultiScatterSurface.slang, Shader/SkyViewSurface.slang, Shader/TransmittanceSurface.slang
    note  📝 Every member is scalar and ordered widest-first so the layout carries no implicit padding under any

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE WIDENING
//------------------------------------------------------------------------------------------------------------------------

F ResolveMediumProfile       | AtmosphereUniform.slang    | 65-90  | nonallocating,nonthrowing | ✔️ | Widens the uploaded block into the profile `Shared/` computes against. second derivation of what the host already resolved spectrally through `02` §5, and the two would agree only until one of them was amended.
    in    Uploaded  MediumUniform  [-]  ?
    out   -         MediumProfile  [-]  ?
    by    Shader/MultiScatterSurface.slang, Shader/SkyViewSurface.slang, Shader/TransmittanceSurface.slang
    note  🔴 One assignment per member and nothing derived. A widening that recomputed a coefficient would be a

F ProjectTexelCoordinate     | AtmosphereUniform.slang    | 98-105 | nonallocating,nonthrowing | ✔️ | The coordinate of one texel centre on a surface of a declared extent. samples at texel corners and a lookup that filters between texel centres disagree by half a texel everywhere — uniformly, so nothing looks broken and the horizon simply sits in the wrong place.
    in    Along             Unsigned32         [-]  ?
    in    Across            Unsigned32         [-]  ?
    in    ExtentAlong       Unsigned32         [-]  ?
    in    ExtentAcross      Unsigned32         [-]  ?
    in    CoordinateAlong   SLATE_OUT(Real64)  [-]  ?
    in    CoordinateAcross  SLATE_OUT(Real64)  [-]  ?
    out   -                 void               [-]  ?
    by    Shader/MultiScatterSurface.slang, Shader/SkyViewSurface.slang, Shader/TransmittanceSurface.slang
    note  📐 The half-texel offset is what makes the bake and the lookup the same mapping. A bake that placed its

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DECLARATIONS
//------------------------------------------------------------------------------------------------------------------------

V MultiScatterDirectionCount | MultiScatterSurface.slang  | 28     | -                         | -  | ?
    by    Api/AtmosphereIntegrator.h, Source/AtmosphereIntegrator.cpp

V MultiScatterStepCount      | MultiScatterSurface.slang  | 29     | -                         | -  | ?
    by    Api/AtmosphereIntegrator.h, Source/AtmosphereIntegrator.cpp

V TransferCeiling            | MultiScatterSurface.slang  | 33     | -                         | -  | ?

V TransferFloor              | MultiScatterSurface.slang  | 34     | -                         | -  | ?

V Medium                     | MultiScatterSurface.slang  | 36     | -                         | -  | ?
    by    Api/AtmosphereIntegrator.h, Shader/SkyViewSurface.slang, Shader/TransmittanceSurface.slang, Shared/AtmosphereProjection.slang.h, Source/AtmosphereIntegrator.cpp

V TransmittanceSurface       | MultiScatterSurface.slang  | 37     | -                         | -  | ?
    by    Api/AtmosphereIntegrator.h, Api/RenderSchedule.h, Shader/SkyRadiance.slang, Shader/SkyViewSurface.slang, Source/AtmosphereIntegrator.cpp

V SurfaceSampler             | MultiScatterSurface.slang  | 38     | -                         | -  | ?
    by    Shader/SkyRadiance.slang, Shader/SkyViewSurface.slang

V MultiScatterOutput         | MultiScatterSurface.slang  | 39     | -                         | -  | ?

V SolarLimbFraction          | SkyRadiance.slang          | 39     | -                         | -  | ?

V SolarSurfaceDeparture      | SkyRadiance.slang          | 44     | -                         | -  | ?

V SkyViewStepCount           | SkyViewSurface.slang       | 27     | -                         | -  | ?
    by    Api/AtmosphereIntegrator.h, Source/AtmosphereIntegrator.cpp

V Medium                     | SkyViewSurface.slang       | 29     | -                         | -  | ?
    by    Api/AtmosphereIntegrator.h, Shader/MultiScatterSurface.slang, Shader/TransmittanceSurface.slang, Shared/AtmosphereProjection.slang.h, Source/AtmosphereIntegrator.cpp

V TransmittanceSurface       | SkyViewSurface.slang       | 30     | -                         | -  | ?
    by    Api/AtmosphereIntegrator.h, Api/RenderSchedule.h, Shader/MultiScatterSurface.slang, Shader/SkyRadiance.slang, Source/AtmosphereIntegrator.cpp

V MultiScatterSurface        | SkyViewSurface.slang       | 31     | -                         | -  | ?
    by    Api/AtmosphereIntegrator.h, Api/RenderSchedule.h, Source/AtmosphereIntegrator.cpp

V SurfaceSampler             | SkyViewSurface.slang       | 32     | -                         | -  | ?
    by    Shader/MultiScatterSurface.slang, Shader/SkyRadiance.slang

V SkyViewOutput              | SkyViewSurface.slang       | 33     | -                         | -  | ?

V TransmittanceStepCount     | TransmittanceSurface.slang | 26     | -                         | -  | ?

V Medium                     | TransmittanceSurface.slang | 28     | -                         | -  | ?
    by    Api/AtmosphereIntegrator.h, Shader/MultiScatterSurface.slang, Shader/SkyViewSurface.slang, Shared/AtmosphereProjection.slang.h, Source/AtmosphereIntegrator.cpp

V TransmittanceOutput        | TransmittanceSurface.slang | 29     | -                         | -  | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE LOOKUP
//------------------------------------------------------------------------------------------------------------------------

F ResolveTransmittance       | MultiScatterSurface.slang  | 50-63  | nonallocating,nonthrowing | 🚩 | ① at a declared radius and zenith cosine. reason the mapping lives in `Shared/` — see the file note there.
    in    Profile       MediumProfile      [-]  ?
    in    Radius        Real64             [-]  ?
    in    ZenithCosine  Real64             [-]  ?
    in    Red           SLATE_OUT(Real64)  [-]  ?
    in    Green         SLATE_OUT(Real64)  [-]  ?
    in    Blue          SLATE_OUT(Real64)  [-]  ?
    out   -             void               [-]  ?
    by    Shader/SkyViewSurface.slang
    note  🔴 Through `Shared/`'s forward mapping, which is the same routine the bake inverted. That is the whole

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE BAKE
//------------------------------------------------------------------------------------------------------------------------

F IntegrateMultiScatter      | MultiScatterSurface.slang  | 83-202 | -                         | -  | ?
    in    SV_DispatchThreadID  Unsigned32x3 TexelOrdinal :  [-]  ?
    out   -                    void                         [-]  ?

F IntegrateSkyView           | SkyViewSurface.slang       | 89-188 | -                         | -  | ?
    in    SV_DispatchThreadID  Unsigned32x3 TexelOrdinal :  [-]  ?
    out   -                    void                         [-]  ?

F IntegrateTransmittance     | TransmittanceSurface.slang | 47-103 | -                         | -  | ?
    in    SV_DispatchThreadID  Unsigned32x3 TexelOrdinal :  [-]  ?
    out   -                    void                         [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

F ResolveSkyRadiance         | SkyRadiance.slang          | 70-127 | nonallocating,nonthrowing | 🚩 | The radiance arriving along a view direction, disc included. two in step. ③ is baked without it and the disc is resolved without it, so one flux applied by whoever reads this scales both together. Applying it to the disc alone — which is where a port of the donor formulation lands, since its disc names an illuminance its sky-view lookup does not — brightens the sun away from the sky immediately behind it, and the seam that produces is at the one place in the image the eye is already looking. subtends about a quarter of a degree; a disc carried through that surface is a texel wide at best and filtered into a smear at worst.
    in    Profile               MediumProfile        [-]    the widened medium, its sun direction already unit in the local frame
    in    DirectionX            Real64               [-]    unit, atmosphere-local, zenith on the second axis
    in    DirectionY            Real64               [-]    ?
    in    DirectionZ            Real64               [-]    ?
    in    SkyViewSurface        Texture2D<Real32x4>  [-]    ③, sampled through `Shared/`'s inverse of the mapping that baked it
    in    TransmittanceSurface  Texture2D<Real32x4>  [-]    ①, for the disc's own attenuation to space
    in    SurfaceSampler        SamplerState         [-]    ?
    in    SolarAngularRadius    Real64               [rad]  half-angle of the disc; a non-positive radius draws no disc — `28` §8's open row
    in    Red                   SLATE_OUT(Real64)    [-]    ?
    in    Green                 SLATE_OUT(Real64)    [-]    ?
    in    Blue                  SLATE_OUT(Real64)    [-]    ?
    in    DirectionY            -                    [-]    ?
    in    DirectionZ            -                    [-]    ?
    out   -                     Red/Green/Blue       [-]    linear radiance, unbounded above unity
    note  🔴 The illuminant's flux is applied to **neither** the sky nor the disc here, and that is what keeps the
    note  📝 The disc is analytic rather than baked. ③ is a hundred and ninety-two texels of azimuth, and the sun

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE LOOKUPS
//------------------------------------------------------------------------------------------------------------------------

F ResolveTransmittance       | SkyViewSurface.slang       | 42-55  | nonallocating,nonthrowing | 🚩 | ① at a declared radius and zenith cosine.
    in    Profile       MediumProfile      [-]  ?
    in    Radius        Real64             [-]  ?
    in    ZenithCosine  Real64             [-]  ?
    in    Red           SLATE_OUT(Real64)  [-]  ?
    in    Green         SLATE_OUT(Real64)  [-]  ?
    in    Blue          SLATE_OUT(Real64)  [-]  ?
    out   -             void               [-]  ?
    by    Shader/MultiScatterSurface.slang

F ResolveMultiScatter        | SkyViewSurface.slang       | 60-73  | nonallocating,nonthrowing | 🚩 | ② at a declared radius and sun zenith cosine.
    in    Profile          MediumProfile      [-]  ?
    in    Radius           Real64             [-]  ?
    in    SunZenithCosine  Real64             [-]  ?
    in    Red              SLATE_OUT(Real64)  [-]  ?
    in    Green            SLATE_OUT(Real64)  [-]  ?
    in    Blue             SLATE_OUT(Real64)  [-]  ?
    out   -                void               [-]  ?
