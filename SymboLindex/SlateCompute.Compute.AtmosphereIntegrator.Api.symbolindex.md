//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Three resident lookup surfaces replacing per-pixel marching — and the only source of environmental light in Slate.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/AtmosphereIntegrator/Api
%layer      SlateCompute
%sources    1
%symbols    45
%annotated  26/45
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S AtmosphereIntegrator.h | 389 lines | e9e87277 | 45 sym | Three resident lookup surfaces replacing per-pixel marching — and the only source of environmental light in Slate.

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE MEDIUM
//------------------------------------------------------------------------------------------------------------------------

T MediumSpecification                             | AtmosphereIntegrator.h | 33-57   | nonallocating,nonthrowing     | -  | The three components of `28` §3, each with its own density profile and scattering behaviour. what produces the blue of twilight rather than the grey the other two components alone give, and omitting it is the most common reason an atmosphere looks wrong only at low sun angles. a document that edits them edits this specification and the surfaces rebuild per §4's first two rows.
    has   PlanetRadius            double  [-]  ?
    has   AtmosphereThickness     double  [-]  ?
    has   RayleighScaleHeight     double  [-]  ?
    has   MieScaleHeight          double  [-]  ?
    has   MieScattering           double  [-]  ?
    has   MieExtinction           double  [-]  ?
    has   MieAsymmetry            double  [-]  ?
    has   OzoneCentreAltitude     double  [-]  ?
    has   OzoneHalfWidth          double  [-]  ?
    has   OzonePeakAbsorption     double  [-]  ?
    has   RefractiveIndex         double  [-]  ?
    has   MolecularConcentration  double  [-]  ?
    has   Depolarisation          double  [-]  ?
    by    Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp
    note  🔴 Ozone absorbs **without scattering** and has no scattering coefficient at all — not a zero one. It is
    note  🚧 `28` §8 leaves artist-editability open. The values below are Earth's and are declared, not assumed:

F MediumSpecification::Validate                   | AtmosphereIntegrator.h | 56      | api,nonallocating,nonthrowing | ✔️ | Whether the medium describes an atmosphere that can be integrated at all. ozone half width, and for an asymmetry outside the open interval about the origin extent, and the phase magnitude diverges along it.
    out   -  Outcome  [-]  refuses with ContentUnsupported for a non-positive radius, thickness, scale height or
    by    Api/AssetInterchange.h, Api/IlluminantPopulation.h, Api/PropertySpecification.h, Api/TilingSpecification.h, Source/AssetInterchange.cpp, Source/AtmosphereIntegrator.cpp, (+4 more)
    note  📐 An asymmetry reaching unity collapses the forward-scattering lobe onto a direction of zero solid

T MediumCoefficient                               | AtmosphereIntegrator.h | 64-71   | nonallocating,nonthrowing     | -  | The medium's coefficients resolved into the working space, per component. wavelength-neutral by declaration and needs no projection, which is why it is one magnitude rather than three — writing it as three equal ones would invite a later edit to make them differ.
    has   RayleighScattering   double[3]  [-]  ?
    has   OzoneAbsorption      double[3]  [-]  ?
    has   MieScattering        double     [-]  ?
    has   MieExtinction        double     [-]  ?
    has   CoefficientResolved  bool       [-]  ?
    by    Source/AtmosphereIntegrator.cpp
    note  🔴 Rayleigh and ozone are resolved **spectrally**, through `02` §5's `SpectralProjection`. Mie is

F Resolve                                         | AtmosphereIntegrator.h | 87      | api,nonthrowing               | 🔴 | Resolves a medium's spectral coefficients into a declared working space. and depolarisation rather than transcribed as three magnitudes. Transcribed, the three are correct for exactly one set of primaries and one working space, and a document declaring a wider space would get a sky whose blue is the old space's blue reinterpreted. The Chappuis band has no closed form, and saying so here is what stops a later reader from assuming the same first-principles standing for both.
    in    Declared  const MediumSpecification&       [-]  the medium
    in    Working   const ColourSpaceSpecification&  [-]  the space the coefficients are expressed in
    in    Rule      const QuadratureRule&            [-]  a derived rule, integrated over the declared wavelength interval
    out   -         Outcome                          [-]  carries the medium's own refusal, and `02` §5's where the projection declines
    by    Api/AttachmentIndex.h, Api/BrushSpecification.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/DocumentSession.h, Api/FileInterchange.h, (+94 more)
    note  📐 The Rayleigh coefficient is **derived** from the medium's refractive index, molecular concentration
    note  📝 The ozone absorption is a **fit to measured absorption** rather than a derivation, unlike Rayleigh.

F SLATE_DECLARES_PRECISION                        | AtmosphereIntegrator.h | 90      | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, Api/ChartPartition.h, (+50 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                  ONE RESIDENT SURFACE
//------------------------------------------------------------------------------------------------------------------------

T ResidentSurface                                 | AtmosphereIntegrator.h | 104-156 | owning                        | -  | One precomputed lookup surface — sampled by coordinate, resident on the device, rebuilt on a declared condition. coordinate with filtering between their texels, which is what `Surface` means everywhere else in the series and is not what a lookup indexed by an ordinal would be. aspirational. All three are Tier D — perceptual output, no numeric guarantee — so half precision is correct by definition rather than by concession.
    has   Encoded        std::vector<std::uint16_t>  [-]  ?
    has   SpannedAlong   std::uint32_t               [-]  ?
    has   SpannedAcross  std::uint32_t               [-]  ?
    has   WrapAlong      bool                        [-]  ?
    by    Source/AtmosphereIntegrator.cpp
    note  ⚠️ `Table` is banned by `00` §8, and the substitution is not a euphemism: these are sampled by
    note  🔴 Stored at half precision, which is what makes `28` §1's declared byte totals true rather than

F ResidentSurface::Construct                      | AtmosphereIntegrator.h | 116     | api,nonthrowing               | 🚩 | Claims the surface at a declared extent, every texel zero. altitude and sun zenith, both genuinely bounded — a wrapped sample at ③'s zenith would read the horizon while standing at the pole, which appears as a bright ring directly overhead.
    in    ExtentAlong        std::uint32_t  [-]  ?
    in    ExtentAcross       std::uint32_t  [-]  ?
    in    WrapAlongDeclared  bool           [-]  the first axis is periodic and its filter wraps rather than clamps
    out   -                  Outcome        [-]  refuses with ContentUnsupported for a zero extent on either axis
    by    Api/AnalyticProjection.h, Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CameraProjection.h, Api/CommandSequence.h, Api/CycleScheduler.h, (+62 more)
    note  🔴 Wrapping is declared per surface because only ③'s azimuth is periodic. ①'s and ②'s axes are

F ResidentSurface::Write                          | AtmosphereIntegrator.h | 124     | api,nonallocating,nonthrowing | ✔️ | Writes one texel's three components; the fourth is written as unity. three-component format would be a fourth format for the device to negotiate — which `06` §2.1's one-queue, explicit-descriptor spine has no appetite for.
    in    Along   std::uint32_t  [-]  ?
    in    Across  std::uint32_t  [-]  ?
    in    Red     double         [-]  ?
    in    Green   double         [-]  ?
    in    Blue    double         [-]  ?
    out   -       void           [-]  ?
    by    Api/PropertySpecification.h, Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp, Source/PropertyPanel.cpp, Source/PropertySpecification.cpp
    note  📝 The fourth component is claimed and unused. `08` §2 declares the format RGBA16F and a

F ResidentSurface::Sample                         | AtmosphereIntegrator.h | 134     | api,nonallocating,nonthrowing | 🚩 | Samples the surface bilinearly at a declared coordinate; the axis declared periodic wraps, the other clamps. periodic. The zenith axis genuinely ends at the zenith, and a wrapped sample there reads the horizon — which appears as a bright ring directly overhead.
    in    CoordinateAlong   double   [-]  the closed unit interval; outside it the sample clamps, or wraps where constructed
    in    CoordinateAcross  double   [-]  likewise
    in    Red               double&  [-]  ?
    in    Green             double&  [-]  ?
    in    Blue              double&  [-]  ?
    out   -                 void     [-]  ?
    by    Api/InputExchange.h, Api/ReflectanceIntegrator.h, Api/SurfaceTileSpace.h, Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp, Source/ImpressionSequence.cpp, (+4 more)
    note  🔴 Wrapping is declared per surface by `Construct`'s `WrapAlongDeclared`, and only ③'s azimuth is

F ResidentSurface::Texels                         | AtmosphereIntegrator.h | 139     | api,nonallocating,nonthrowing | ✔️ | The encoded texels, for whoever uploads them.
    out   -  const std::vector<std::uint16_t>&  [-]  ?
    by    Api/AnalyticProjection.h, Api/ClipboardExchange.h, Api/EmissionSequence.h, Api/SurfaceLayerSequence.h, Source/AnalyticProjection.cpp, Source/AtmosphereIntegrator.cpp, (+6 more)

F ResidentSurface::ResidentBytes                  | AtmosphereIntegrator.h | 144     | api,nonallocating,nonthrowing | ✔️ | What the surface occupies once resident.
    out   -  std::uint64_t  [-]  ?
    by    Api/ReflectanceIntegrator.h, Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp, Source/ReflectanceIntegrator.cpp

F ResidentSurface::ExtentAlong                    | AtmosphereIntegrator.h | 146     | -                             | -  | ?
    out   -  std::uint32_t  [-]  ?
    by    Api/DepthReduction.h, Api/ReflectanceIntegrator.h, Shader/AtmosphereUniform.slang, Source/AtmosphereIntegrator.cpp, Source/DepthReduction.cpp, Source/OcclusionScheduler.cpp, (+1 more)

F ResidentSurface::ExtentAcross                   | AtmosphereIntegrator.h | 147     | -                             | -  | ?
    out   -  std::uint32_t  [-]  ?
    by    Api/DepthReduction.h, Api/ReflectanceIntegrator.h, Shader/AtmosphereUniform.slang, Source/AtmosphereIntegrator.cpp, Source/DepthReduction.cpp, Source/OcclusionScheduler.cpp, (+1 more)

F ResidentSurface::SurfaceConstructed             | AtmosphereIntegrator.h | 148     | -                             | -  | ?
    out   -  bool  [-]  ?
    by    Source/AtmosphereIntegrator.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE AMBIENT TERM
//------------------------------------------------------------------------------------------------------------------------

T IrradianceProjection                            | AtmosphereIntegrator.h | 173-189 | nonallocating,nonthrowing     | -  | The sky's radiance projected onto a second-order harmonic basis and convolved against the cosine lobe. integral evaluated per shaded pixel is the thing the whole precomputation exists to avoid, and it would be evaluated at every pixel of every rotation rather than at every rebuild. product "for probes"; no probe population exists in Slate, so the product is retained and retargeted to `18` §5's diffuse ambient. `Probe` is banned and nothing here spells it. lobe's own harmonic expansion has less than a percent of its energy above the second order, so a higher order would carry coefficients that the convolution multiplies by very nearly nothing.
    has   Coefficient  double[9][3]  [-]  ?
    by    Source/AtmosphereIntegrator.cpp
    note  🔴 `28` §5: the convolution is derived when `SkyViewSurface` rebuilds, **never per pixel**. A hemisphere
    note  ⚠️ `00` §5.1's third substitution point, made concrete. The donor documents describe an irradiance
    note  📐 Nine coefficients is the standard order for a diffuse convolution and is not a budget: the cosine

F IrradianceProjection::Evaluate                  | AtmosphereIntegrator.h | 187     | api,nonallocating,nonthrowing | ✔️ | Evaluates the irradiance arriving at a surface of a declared orientation. against a bright horizon. Clamped at zero because a negative irradiance is not a dim surface, it is a surface that subtracts light from whatever else reaches it.
    in    DirectionX  double          [-]  the surface's outward orientation, unit length
    in    DirectionY  double          [-]  ?
    in    DirectionZ  double          [-]  ?
    in    Red         double&         [-]  ?
    in    Green       double&         [-]  ?
    in    Blue        double&         [-]  ?
    in    DirectionY  -               [-]  ?
    in    DirectionZ  -               [-]  ?
    out   -           Red/Green/Blue  [-]  never negative; the reconstruction is clamped at zero
    by    Api/QuadratureIntegrator.h, Api/SpectralProjection.h, Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp, Source/ReflectanceIntegrator.cpp
    note  ⚠️ A truncated harmonic reconstruction rings, and the ring goes negative where the sky is dark

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE INTEGRATOR
//------------------------------------------------------------------------------------------------------------------------

T AtmosphereIntegrator                            | AtmosphereIntegrator.h | 208-381 | owning                        | -  | The three resident surfaces, their construction order, and the conditions each rebuilds on. against a ① that does not stand is refused rather than performed against zeros — a sky-view surface built against an absent transmittance is uniformly black and reads as a device failure. enrolled as the atmospheric source and that `28` reads it; the requester resolves that enrolment on the tick and hands the direction over. Declaring the edge instead would put `44` in this document's Upstream and move `28` from `00` §9.1's stratum 4 to stratum 5, and `00` §11 gates that a declared edge is a real read — so the choice is between a false edge and a supplied parameter. multiple-scattering term near the horizon; `28` §3 declares three medium components and a fourth is not this document's to invent. Recorded as an open row rather than added quietly.
    has   MultiScatterDirectionCount  static constexpr std::uint32_t  [-]  ?
    has   MultiScatterStepCount       static constexpr std::uint32_t  [-]  ?
    has   SkyViewStepCount            static constexpr std::uint32_t  [-]  ?
    has   IrradianceSampleCount       static constexpr std::uint32_t  [-]  ?
    has   DeclaredMedium              MediumSpecification             [-]  ?
    has   ResolvedCoefficient         MediumCoefficient               [-]  ?
    has   ShapedProfile               MediumProfile                   [-]  ?
    has   TransmittanceSurface        ResidentSurface                 [-]  ?
    has   MultiScatterSurface         ResidentSurface                 [-]  ?
    has   SkyViewSurface              ResidentSurface                 [-]  ?
    has   ConvolvedIrradiance         IrradianceProjection            [-]  ?
    has   ConstantFloor               ColourSpecification             [-]  ?
    has   SunDirectionX               double                          [-]  ?
    has   SunDirectionY               double                          [-]  ?
    has   SunDirectionZ               double                          [-]  ?
    has   BuiltSunX                   double                          [-]  ?
    has   BuiltSunY                   double                          [-]  ?
    has   BuiltSunZ                   double                          [-]  ?
    has   CameraAltitude              double                          [-]  ?
    has   BuiltAltitude               double                          [-]  ?
    has   MediumRebuilds              std::uint32_t                   [-]  ?
    has   SkyViewRebuilds             std::uint32_t                   [-]  ?
    has   MediumDeclared              bool                            [-]  ?
    has   MediumOwed                  bool                            [-]  ?
    has   SkyViewOwed                 bool                            [-]  ?
    has   PresenceDeclared            bool                            [-]  ?
    has   FloorDeclared               bool                            [-]  ?
    by    Api/ReflectanceIntegrator.h, Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp, Source/ReflectanceIntegrator.cpp
    note  🔴 `28` §2: strictly ordered ① ② ③, and each surface reads only the surfaces before it. Constructing ③
    note  🔴 The sun direction is **supplied**, not read from `44`. `44` §8 gates that exactly one illuminant is
    note  ⚠️ There is no ground albedo. Hillaire's formulation includes one and it materially brightens the

F AtmosphereIntegrator::DeclareMedium             | AtmosphereIntegrator.h | 223     | api,nonthrowing               | ✔️ | Declares the medium, which owes all three surfaces a rebuild.
    in    Declaring  const MediumSpecification&  [-]  ?
    out   -          Outcome                     [-]  carries the medium's own refusal
    by    Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp

F AtmosphereIntegrator::DeclareSun                | AtmosphereIntegrator.h | 233     | api,nonthrowing               | ✔️ | Declares the direction toward the atmospheric source, as `44`'s enrolled illuminant reports it.
    in    DirectionX  double   [-]  toward the sun; normalised here, so an unnormalised direction is admitted
    in    DirectionY  double   [-]  the local zenith is the second axis, matching `46`'s upward convention
    in    DirectionZ  double   [-]  ?
    in    DirectionZ  -        [-]  ?
    out   -           Outcome  [-]  refuses with ContentUnsupported for a direction of no length
    post  🔴 the sky-view surface is owed a rebuild only when the direction moved **materially** — `28` §4
    by    Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp

F AtmosphereIntegrator::DeclareCameraAltitude     | AtmosphereIntegrator.h | 240     | api,nonthrowing               | ✔️ | Declares the camera's altitude above the planet surface.
    in    Altitude  double   [-]  ?
    out   -         Outcome  [-]  refuses with ContentUnsupported for an altitude outside the declared atmosphere
    post  the sky-view surface is owed a rebuild only when the altitude changed materially
    by    Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp

F AtmosphereIntegrator::DeclareAtmospherePresence | AtmosphereIntegrator.h | 248     | api,nonallocating,nonthrowing | ✔️ | Declares whether the atmosphere is present at all. to the same. Both reach that fallback through `SampleSkyView` below, so neither carries a branch of its own and the two cannot come to disagree about what "disabled" looks like.
    in    PresenceEnabled  bool  [-]  ?
    out   -                void  [-]  ?
    by    Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp
    note  🔴 `28` §7's last gate: with the atmosphere disabled, `18` falls back to the constant floor and `30`

F AtmosphereIntegrator::DeclareConstantFloor      | AtmosphereIntegrator.h | 256     | api,nonthrowing               | ✔️ | Declares the constant floor the disabled atmosphere resolves to. structural. It is declared by the caller here rather than chosen, which is what keeps the row open.
    in    Declaring  const ColourSpecification&  [-]  ?
    out   -          Outcome                     [-]  refuses with ContentUnsupported for a colour declaring no space — `36` §1
    by    Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp
    note  🚧 `18` §10 carries the floor's magnitude as an open row and records that it blocks nothing

F AtmosphereIntegrator::Rebuild                   | AtmosphereIntegrator.h | 269     | api,nonthrowing               | 🔴 | Rebuilds whatever the declared conditions owe, in construction order, and nothing else. derived, and carries `Resolve`'s refusal where the spectral projection declines both. A rebuild owed on ③ alone runs ③ alone, which is the whole reason the sun may move without the medium being re-integrated.
    in    Working  const ColourSpaceSpecification&  [-]  the space the radiance is expressed in
    in    Rule     const QuadratureRule&            [-]  a derived rule; the optical depths are integrated against it
    out   -        Outcome                          [-]  refuses with ContentUnsupported before a medium is declared or before the rule is
    post  🔴 with nothing owed, nothing is rebuilt and nothing is recorded — `28` §4
    by    Api/OcclusionProjection.h, Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp, Source/OcclusionProjection.cpp
    note  🔴 The construction order is ① transmittance, ② multiple scattering reading ①, ③ sky-view reading

F AtmosphereIntegrator::RebuildOwed               | AtmosphereIntegrator.h | 276     | api,nonallocating,nonthrowing | ✔️ | Whether anything is owed a rebuild. what the schedule's contributor reads to decide.
    out   -  bool  [-]  ?
    by    Api/OcclusionProjection.h, Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp, Source/OcclusionProjection.cpp
    note  🔴 `28` §4: `28` is conditional in `08` §3. When nothing changed, it records nothing — and this is

F AtmosphereIntegrator::SampleSkyView             | AtmosphereIntegrator.h | 290     | api,nonthrowing               | 🚩 | Samples sky-view radiance along a view direction — `18` §5's specular ambient and `30`'s fallback. surface stands `30` §3 both name the floor as the second of exactly two sources, and a refusal here would make each of them write the fallback again.
    in    DirectionX  double          [-]  the view direction, unit length, in the local frame
    in    DirectionY  double          [-]  ?
    in    DirectionZ  double          [-]  ?
    in    Red         double&         [-]  ?
    in    Green       double&         [-]  ?
    in    Blue        double&         [-]  ?
    in    DirectionY  -               [-]  ?
    in    DirectionZ  -               [-]  ?
    out   -           Red/Green/Blue  [-]  radiance, in the working space
    out   -           Outcome         [-]  refuses with ContentUnsupported when the atmosphere is enabled and no sky-view
    by    Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp, Source/ReflectanceIntegrator.cpp
    note  🔴 With the atmosphere disabled this delivers the constant floor rather than refusing. `18` §5 and

F AtmosphereIntegrator::SampleTransmittance       | AtmosphereIntegrator.h | 297     | api,nonthrowing               | 🚩 | Samples transmittance from a declared altitude along a declared zenith cosine, to the atmosphere boundary.
    in    Altitude      double   [-]  ?
    in    ZenithCosine  double   [-]  ?
    in    Red           double&  [-]  ?
    in    Green         double&  [-]  ?
    in    Blue          double&  [-]  ?
    out   -             Outcome  [-]  refuses with ContentUnsupported before ① stands
    by    Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp

F AtmosphereIntegrator::AerialTransmittance       | AtmosphereIntegrator.h | 313     | api,nonthrowing               | 🔴 | Transmittance over a bounded distance along a view ray — `28` §6's aerial perspective. and the ratio trick that recovers a bounded segment from it loses its conditioning near the horizon, which is exactly where distant geometry sits. `28` §8 leaves whether aerial perspective earns a resident surface of its own open, and integrating here is what keeps that row open rather than answering it by accident. as unnaturally crisp against a correct sky — `28` §6.
    in    Altitude    double                 [m]  where the ray begins
    in    DirectionX  double                 [-]  ?
    in    DirectionY  double                 [-]  ?
    in    DirectionZ  double                 [-]  ?
    in    Distance    double                 [m]  how far along it the surface sits
    in    Rule        const QuadratureRule&  [-]  ?
    in    Red         double&                [-]  ?
    in    Green       double&                [-]  ?
    in    Blue        double&                [-]  ?
    out   -           Outcome                [-]  refuses with ContentUnsupported before a medium is declared or before the rule is derived
    by    Source/AtmosphereIntegrator.cpp
    note  🔴 Integrated directly rather than read from ①. ① holds transmittance **to the atmosphere boundary**
    note  ⚠️ This applies to scene surfaces in `18` and not only to the sky. Without it distant geometry reads

F AtmosphereIntegrator::Irradiance                | AtmosphereIntegrator.h | 322     | api,nonallocating,nonthrowing | ✔️ | The cosine-convolved irradiance, derived at the last sky-view rebuild — `18` §5's diffuse ambient.
    out   -  const IrradianceProjection&  [-]  ?
    by    Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp, Source/ReflectanceIntegrator.cpp

F AtmosphereIntegrator::Transmittance             | AtmosphereIntegrator.h | 324     | -                             | -  | ?
    out   -  const ResidentSurface&  [-]  ?
    by    Source/AtmosphereIntegrator.cpp

F AtmosphereIntegrator::MultiScatter              | AtmosphereIntegrator.h | 325     | -                             | -  | ?
    out   -  const ResidentSurface&  [-]  ?
    by    Source/AtmosphereIntegrator.cpp

F AtmosphereIntegrator::SkyView                   | AtmosphereIntegrator.h | 326     | -                             | -  | ?
    out   -  const ResidentSurface&  [-]  ?
    by    Source/AtmosphereIntegrator.cpp

F AtmosphereIntegrator::ResidentBytes             | AtmosphereIntegrator.h | 331     | api,nonallocating,nonthrowing | ✔️ | What all three occupy once resident — `28` §7's first gate, measured rather than asserted.
    out   -  std::uint64_t  [-]  ?
    by    Api/ReflectanceIntegrator.h, Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp, Source/ReflectanceIntegrator.cpp

F AtmosphereIntegrator::MediumRebuildCount        | AtmosphereIntegrator.h | 338     | api,nonallocating,nonthrowing | ✔️ | How many times each surface has been rebuilt this session. for. A transmittance count that tracks the rotation count is the defect §4 exists to prevent.
    out   -  std::uint32_t  [-]  ?
    by    Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp
    note  📝 Presented so that `28` §4's "almost never" and "occasionally" are measurable rather than hoped

F AtmosphereIntegrator::SkyViewRebuildCount       | AtmosphereIntegrator.h | 339     | -                             | -  | ?
    out   -  std::uint32_t  [-]  ?
    by    Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp

F AtmosphereIntegrator::Medium                    | AtmosphereIntegrator.h | 341     | -                             | -  | ?
    out   -  const MediumSpecification&  [-]  ?
    by    Shader/MultiScatterSurface.slang, Shader/SkyViewSurface.slang, Shader/TransmittanceSurface.slang, Shared/AtmosphereProjection.slang.h, Source/AtmosphereIntegrator.cpp

F AtmosphereIntegrator::Coefficient               | AtmosphereIntegrator.h | 342     | -                             | -  | ?
    out   -  const MediumCoefficient&  [-]  ?
    by    Api/TransformProjection.h, Source/AtmosphereIntegrator.cpp, Source/CameraProjection.cpp, Source/ColourProjection.cpp, Source/ConsoleHost.cpp, Source/PartitionClassifier.cpp, (+3 more)

F AtmosphereIntegrator::AtmospherePresent         | AtmosphereIntegrator.h | 343     | -                             | -  | ?
    out   -  bool  [-]  ?
    by    Source/AtmosphereIntegrator.cpp

F AtmosphereIntegrator::ShapeProfile              | AtmosphereIntegrator.h | 347     | -                             | -  | ?
    out   -  void  [-]  ?
    by    Source/AtmosphereIntegrator.cpp

F AtmosphereIntegrator::BuildTransmittance        | AtmosphereIntegrator.h | 348     | -                             | -  | ?
    in    Rule  const QuadratureRule&  [-]  ?
    out   -     Outcome<bool>          [-]  ?
    by    Source/AtmosphereIntegrator.cpp

F AtmosphereIntegrator::BuildMultiScatter         | AtmosphereIntegrator.h | 349     | -                             | -  | ?
    out   -  Outcome<bool>  [-]  ?
    by    Source/AtmosphereIntegrator.cpp

F AtmosphereIntegrator::BuildSkyView              | AtmosphereIntegrator.h | 350     | -                             | -  | ?
    out   -  Outcome<bool>  [-]  ?
    by    Source/AtmosphereIntegrator.cpp

F AtmosphereIntegrator::DeriveIrradiance          | AtmosphereIntegrator.h | 351     | -                             | -  | ?
    out   -  void  [-]  ?
    by    Source/AtmosphereIntegrator.cpp

F AtmosphereIntegrator::TransmittanceAt           | AtmosphereIntegrator.h | 353     | -                             | -  | ?
    in    Radius        double   [-]  ?
    in    ZenithCosine  double   [-]  ?
    in    Red           double&  [-]  ?
    in    Green         double&  [-]  ?
    in    Blue          double&  [-]  ?
    out   -             void     [-]  ?
    by    Source/AtmosphereIntegrator.cpp

F AtmosphereIntegrator::MultiScatterAt            | AtmosphereIntegrator.h | 355     | -                             | -  | ?
    in    Radius           double   [-]  ?
    in    SunZenithCosine  double   [-]  ?
    in    Red              double&  [-]  ?
    in    Green            double&  [-]  ?
    in    Blue             double&  [-]  ?
    out   -                void     [-]  ?
    by    Source/AtmosphereIntegrator.cpp

F SLATE_DECLARES_PRECISION                        | AtmosphereIntegrator.h | 385     | -                             | -  | ?
    in    Perceptual  PrecisionGuarantee::  [-]  ?
    in    Perceptual  PrecisionGuarantee::  [-]  ?
    in    Bounded     PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, Api/ChartPartition.h, (+50 more)
