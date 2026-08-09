//============================================================================================================================================
//                                                       ATMOSPHEREINTEGRATOR.CPP
//============================================================================================================================================
// 🧩 The three surfaces in construction order, the spectral coefficients behind them, and the convolution derived on rebuild.

#include "SlateCompute/Compute/AtmosphereIntegrator/Api/AtmosphereIntegrator.h"

#include "Shared/SampleProjection.slang.h"

#include <cmath>
#include <cstring>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   HALF PRECISION
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📝 🚧 File-local because `28` is the only component that stores half precision today. `06` will need the same
//    pair the moment it claims an RGBA16F target it writes from the host, and this moves to `SlateMath` then —
//    two units reading one conversion is `00` §2's rule for constants applied to arithmetic.
std::uint16_t EncodeHalf(float Magnitude)
{
    std::uint32_t Bits = 0u;
    std::memcpy(&Bits, &Magnitude, sizeof(Bits));

    const std::uint32_t Signum   = (Bits >> 16) & 0x8000u;
    std::int32_t        Exponent = static_cast<std::int32_t>((Bits >> 23) & 0xFFu) - 112;   // 127 − 15
    std::uint32_t       Mantissa = Bits & 0x7FFFFFu;

    // 📝 Subnormals flush to zero rather than encoding. A radiance below the half-precision subnormal floor is
    //    below anything `66`'s tone projection can distinguish from black, and encoding it costs a branch at
    //    every texel to preserve a distinction no display carries.
    if (Exponent <= 0)
        return static_cast<std::uint16_t>(Signum);

    if (Exponent >= 31)
        return static_cast<std::uint16_t>(Signum | 0x7BFFu);   // the largest finite half, never an infinity

    // 📐 Round to nearest, ties to even. Truncating instead biases every encoded magnitude downward, and a
    //    transmittance surface biased downward is an atmosphere that is uniformly and inexplicably too dark.
    const std::uint32_t Rounded = Mantissa + 0x0FFFu + ((Mantissa >> 13) & 1u);

    if ((Rounded & 0x800000u) != 0u)
    {
        ++Exponent;

        if (Exponent >= 31)
            return static_cast<std::uint16_t>(Signum | 0x7BFFu);

        return static_cast<std::uint16_t>(Signum | (static_cast<std::uint32_t>(Exponent) << 10));
    }

    return static_cast<std::uint16_t>(Signum | (static_cast<std::uint32_t>(Exponent) << 10) | (Rounded >> 13));
}

float DecodeHalf(std::uint16_t Encoded)
{
    const std::uint32_t Signum   = static_cast<std::uint32_t>(Encoded & 0x8000u) << 16;
    const std::uint32_t Exponent = (Encoded >> 10) & 0x1Fu;
    const std::uint32_t Mantissa = Encoded & 0x3FFu;

    if (Exponent == 0u)
    {
        // 📐 A subnormal half is Mantissa × 2⁻²⁴. Encode never produces one, but a surface uploaded and read back
        //    by a device might, so decoding admits them rather than reporting them as zero.
        const float Subnormal = static_cast<float>(Mantissa) * 5.960464477539063e-8f;

        return (Signum != 0u) ? -Subnormal : Subnormal;
    }

    std::uint32_t Bits = 0u;

    if (Exponent == 31u)
        Bits = Signum | 0x7F800000u | (Mantissa << 13);
    else
        Bits = Signum | ((Exponent + 112u) << 23) | (Mantissa << 13);

    float Magnitude = 0.0f;
    std::memcpy(&Magnitude, &Bits, sizeof(Magnitude));

    return Magnitude;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  GEOMETRY HELPERS
//------------------------------------------------------------------------------------------------------------------------

double Bounded(double Magnitude, double Lower, double Upper)
{
    return Magnitude < Lower ? Lower : (Magnitude > Upper ? Upper : Magnitude);
}

// 📐 The distance from a radius along a zenith cosine to a sphere of a declared radius. The discriminant is
//    written as r²(μ²−1) + R² rather than as the expanded quadratic, because the expanded form subtracts two
//    nearly equal magnitudes at grazing angles and loses every significant digit exactly where the horizon is.
double DistanceToSphere(double Radius, double ZenithCosine, double SphereRadius)
{
    const double Discriminant = Radius * Radius * (ZenithCosine * ZenithCosine - 1.0) + SphereRadius * SphereRadius;

    if (Discriminant < 0.0)
        return -1.0;

    const double Root = std::sqrt(Discriminant);

    // 📝 The nearer intersection where the ray descends, the further where it climbs. A ray that climbs has no
    //    nearer intersection with the planet at all, which is what the negative branch below tests for.
    const double Nearer = -Radius * ZenithCosine - Root;
    const double Further = -Radius * ZenithCosine + Root;

    if (Nearer >= 0.0)
        return Nearer;

    return Further >= 0.0 ? Further : -1.0;
}

// 📐 The radius reached after advancing a declared distance along a ray, from the cosine rule on the triangle
//    centre–origin–arrival: r'² = r² + d² + 2rμd. Advancing a position and taking its length instead would carry
//    three coordinates through the march to recover one number that depends on none of them separately.
double AdvanceRadius(double Radius, double ZenithCosine, double Distance)
{
    const double Squared = Radius * Radius + Distance * Distance + 2.0 * Radius * ZenithCosine * Distance;

    return std::sqrt(Squared > 0.0 ? Squared : 0.0);
}

bool GroundReached(double Radius, double ZenithCosine, double PlanetRadius)
{
    if (ZenithCosine >= 0.0)
        return false;

    return Radius * Radius * (ZenithCosine * ZenithCosine - 1.0) + PlanetRadius * PlanetRadius >= 0.0;
}

// 📐 Rayleigh's phase, 3(1+cos²θ)/16π. Near-isotropic with a shallow pair of lobes forward and back, which is
//    why the daytime sky is bright in every direction rather than only around the sun.
double RayleighPhase(double ScatterCosine)
{
    return 3.0 * (1.0 + ScatterCosine * ScatterCosine) / (16.0 * Pi);
}

// 📐 Cornette–Shanks, forward-biased by the declared asymmetry. It is what puts the bright halo around the sun
//    and what makes a hazy day's horizon brighter than its zenith.
// 🔴 This is the **same** phase `Shared/AtmosphereProjection.slang.h` spells for the device, and it must stay the
//    same one. Henyey–Greenstein stood here previously, which is a defensible phase and the wrong one to hold
//    beside a device form that carries the Rayleigh-like (1+cos²θ) numerator: the two agree to within a few per
//    cent away from the sun and diverge across the solar halo, so a host-against-device comparison passes on
//    almost every sample it draws and fails only on the population the surface exists to represent.
double MiePhase(double ScatterCosine, double Asymmetry)
{
    const double Squared     = Asymmetry * Asymmetry;
    const double Numerator   = 3.0 * (1.0 - Squared) * (1.0 + ScatterCosine * ScatterCosine);
    const double Base        = 1.0 + Squared - 2.0 * Asymmetry * ScatterCosine;
    const double Held        = Base > 1.0e-4 ? Base : 1.0e-4;
    const double Denominator = 8.0 * Pi * (2.0 + Squared) * Held * std::sqrt(Held);

    return Numerator / Denominator;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                  MEDIUM VALIDATION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> MediumSpecification::Validate() const
{
    if (PlanetRadius <= 0.0 || AtmosphereThickness <= 0.0)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the planet or its atmosphere has no extent" });

    if (RayleighScaleHeight <= 0.0 || MieScaleHeight <= 0.0)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "a scale height of zero has no profile" });

    if (OzoneHalfWidth <= 0.0)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the ozone tent has no width" });

    if (MieAsymmetry <= -1.0 || MieAsymmetry >= 1.0)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the asymmetry collapses the phase lobe" });

    if (MieExtinction < MieScattering)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "a component cannot scatter more than it extinguishes" });
    }

    if (MolecularConcentration <= 0.0 || RefractiveIndex <= 1.0)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the medium is not a refracting gas" });

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                              THE SPECTRAL COEFFICIENTS
//------------------------------------------------------------------------------------------------------------------------

Outcome<MediumCoefficient> Resolve(const MediumSpecification&      Declared,
                                   const ColourSpaceSpecification& Working,
                                   const QuadratureRule&           Rule)
{
    const Outcome<bool> Validated = Declared.Validate();

    if (!Validated.ContentPresent)
        return Outcome<MediumCoefficient>::Refuse(Validated.Declined);

    if (!Rule.Derived())
        return Outcome<MediumCoefficient>::Refuse({ RefusalReason::ContentUnsupported, "the rule is not derived" });

    // 📐 β(λ) = 8π³(n²−1)² / (3Nλ⁴) × (6+3p)/(6−7p). The King correction factor on the right accounts for the
    //    molecules not being spherically symmetric; omitting it understates the coefficient by about six percent,
    //    which is a sky that is very slightly too dark and entirely the right colour — so nobody finds it.
    const double IndexTerm  = Declared.RefractiveIndex * Declared.RefractiveIndex - 1.0;
    const double KingFactor = (6.0 + 3.0 * Declared.Depolarisation) / (6.0 - 7.0 * Declared.Depolarisation);
    const double Numerator  = 8.0 * Pi * Pi * Pi * IndexTerm * IndexTerm * KingFactor;

    const Outcome<TristimulusCoordinate> Rayleigh = ProjectSpectrum(
        Rule,
        [&](double Wavelength)
        {
            const double Metres  = Wavelength * 1.0e-9;
            const double Quartic = Metres * Metres * Metres * Metres;

            return Numerator / (3.0 * Declared.MolecularConcentration * Quartic);
        });

    if (!Rayleigh.ContentPresent)
        return Outcome<MediumCoefficient>::Refuse(Rayleigh.Declined);

    // 📐 The Chappuis band, as a two-lobe fit peaking near six hundred nanometres. 🔴 This is a fit to measured
    //    absorption and not a derivation: ozone's cross-section has no closed form, and the note on `Resolve`
    //    exists so that nobody later reads it as having the same standing as the Rayleigh expression above.
    const Outcome<TristimulusCoordinate> Ozone = ProjectSpectrum(
        Rule,
        [&](double Wavelength)
        {
            const double Principal = (Wavelength - 602.0) / 78.0;
            const double Secondary = (Wavelength - 505.0) / 52.0;

            const double Shape = std::exp(-0.5 * Principal * Principal)
                               + 0.42 * std::exp(-0.5 * Secondary * Secondary);

            return Declared.OzonePeakAbsorption * Shape;
        });

    if (!Ozone.ContentPresent)
        return Outcome<MediumCoefficient>::Refuse(Ozone.Declined);

    const Outcome<ColourSpecification> RayleighWorking =
        ProjectTristimulus(Rayleigh.Resolve().MagnitudeX,
                           Rayleigh.Resolve().MagnitudeY,
                           Rayleigh.Resolve().MagnitudeZ,
                           Working);

    const Outcome<ColourSpecification> OzoneWorking =
        ProjectTristimulus(Ozone.Resolve().MagnitudeX,
                           Ozone.Resolve().MagnitudeY,
                           Ozone.Resolve().MagnitudeZ,
                           Working);

    if (!RayleighWorking.ContentPresent)
        return Outcome<MediumCoefficient>::Refuse(RayleighWorking.Declined);

    if (!OzoneWorking.ContentPresent)
        return Outcome<MediumCoefficient>::Refuse(OzoneWorking.Declined);

    MediumCoefficient Resolved;

    // 📝 An extinction coefficient is never negative. A wide working space can carry a negative coordinate
    //    legitimately — `36` §7 transfers negatives rather than clamping them — but a negative extinction is a
    //    medium that amplifies light along a path, and no tone projection recovers from it.
    Resolved.RayleighScattering[0] = RayleighWorking.Resolve().RedCoordinate   > 0.0
                                   ? RayleighWorking.Resolve().RedCoordinate   : 0.0;
    Resolved.RayleighScattering[1] = RayleighWorking.Resolve().GreenCoordinate > 0.0
                                   ? RayleighWorking.Resolve().GreenCoordinate : 0.0;
    Resolved.RayleighScattering[2] = RayleighWorking.Resolve().BlueCoordinate  > 0.0
                                   ? RayleighWorking.Resolve().BlueCoordinate  : 0.0;

    Resolved.OzoneAbsorption[0] = OzoneWorking.Resolve().RedCoordinate   > 0.0
                                ? OzoneWorking.Resolve().RedCoordinate   : 0.0;
    Resolved.OzoneAbsorption[1] = OzoneWorking.Resolve().GreenCoordinate > 0.0
                                ? OzoneWorking.Resolve().GreenCoordinate : 0.0;
    Resolved.OzoneAbsorption[2] = OzoneWorking.Resolve().BlueCoordinate  > 0.0
                                ? OzoneWorking.Resolve().BlueCoordinate  : 0.0;

    Resolved.MieScattering       = Declared.MieScattering;
    Resolved.MieExtinction       = Declared.MieExtinction;
    Resolved.CoefficientResolved = true;

    return Outcome<MediumCoefficient>::Deliver(Resolved);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                 ONE RESIDENT SURFACE
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> ResidentSurface::Construct(std::uint32_t ExtentAlong_, std::uint32_t ExtentAcross_)
{
    if (ExtentAlong_ == 0u || ExtentAcross_ == 0u)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "a surface of no extent" });

    SpannedAlong  = ExtentAlong_;
    SpannedAcross = ExtentAcross_;

    Encoded.assign(static_cast<std::size_t>(ExtentAlong_) * ExtentAcross_ * AtmosphereComponentCount, 0u);

    return Outcome<bool>::Deliver(true);
}

void ResidentSurface::Write(std::uint32_t Along, std::uint32_t Across, double Red, double Green, double Blue)
{
    if (Along >= SpannedAlong || Across >= SpannedAcross)
        return;

    const std::size_t Writing = (static_cast<std::size_t>(Across) * SpannedAlong + Along)
                              * AtmosphereComponentCount;

    Encoded[Writing]      = EncodeHalf(static_cast<float>(Red));
    Encoded[Writing + 1u] = EncodeHalf(static_cast<float>(Green));
    Encoded[Writing + 2u] = EncodeHalf(static_cast<float>(Blue));
    Encoded[Writing + 3u] = EncodeHalf(1.0f);
}

void ResidentSurface::Sample(double CoordinateAlong, double CoordinateAcross,
                             double& Red, double& Green, double& Blue) const
{
    Red   = 0.0;
    Green = 0.0;
    Blue  = 0.0;

    if (Encoded.empty())
        return;

    const double SpanAlong  = static_cast<double>(SpannedAlong);
    const double SpanAcross = static_cast<double>(SpannedAcross);

    double TexelAlong  = Bounded(CoordinateAlong,  0.0, 1.0) * SpanAlong  - 0.5;
    double TexelAcross = Bounded(CoordinateAcross, 0.0, 1.0) * SpanAcross - 0.5;

    TexelAlong  = Bounded(TexelAlong,  0.0, SpanAlong  - 1.0);
    TexelAcross = Bounded(TexelAcross, 0.0, SpanAcross - 1.0);

    const std::uint32_t LeastAlong  = static_cast<std::uint32_t>(TexelAlong);
    const std::uint32_t LeastAcross = static_cast<std::uint32_t>(TexelAcross);

    const std::uint32_t NextAlong  = LeastAlong  + 1u < SpannedAlong  ? LeastAlong  + 1u : LeastAlong;
    const std::uint32_t NextAcross = LeastAcross + 1u < SpannedAcross ? LeastAcross + 1u : LeastAcross;

    const double FractionAlong  = TexelAlong  - static_cast<double>(LeastAlong);
    const double FractionAcross = TexelAcross - static_cast<double>(LeastAcross);

    const std::size_t LowerLeft  = (static_cast<std::size_t>(LeastAcross) * SpannedAlong + LeastAlong)
                                 * AtmosphereComponentCount;
    const std::size_t LowerRight = (static_cast<std::size_t>(LeastAcross) * SpannedAlong + NextAlong)
                                 * AtmosphereComponentCount;
    const std::size_t UpperLeft  = (static_cast<std::size_t>(NextAcross)  * SpannedAlong + LeastAlong)
                                 * AtmosphereComponentCount;
    const std::size_t UpperRight = (static_cast<std::size_t>(NextAcross)  * SpannedAlong + NextAlong)
                                 * AtmosphereComponentCount;

    double* Resolved[3] = { &Red, &Green, &Blue };

    for (std::uint32_t Component = 0u; Component < 3u; ++Component)
    {
        const double Lower = static_cast<double>(DecodeHalf(Encoded[LowerLeft  + Component])) * (1.0 - FractionAlong)
                           + static_cast<double>(DecodeHalf(Encoded[LowerRight + Component])) * FractionAlong;

        const double Upper = static_cast<double>(DecodeHalf(Encoded[UpperLeft  + Component])) * (1.0 - FractionAlong)
                           + static_cast<double>(DecodeHalf(Encoded[UpperRight + Component])) * FractionAlong;

        *Resolved[Component] = Lower * (1.0 - FractionAcross) + Upper * FractionAcross;
    }
}

const std::vector<std::uint16_t>& ResidentSurface::Texels() const { return Encoded; }

std::uint64_t ResidentSurface::ResidentBytes() const
{
    return static_cast<std::uint64_t>(Encoded.size()) * AtmosphereComponentBytes;
}

std::uint32_t ResidentSurface::ExtentAlong() const        { return SpannedAlong;  }
std::uint32_t ResidentSurface::ExtentAcross() const       { return SpannedAcross; }
bool          ResidentSurface::SurfaceConstructed() const { return !Encoded.empty(); }

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE AMBIENT TERM
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📐 The nine real second-order harmonic basis magnitudes at one direction, in the conventional ordering.
void HarmonicBasis(double DirectionX, double DirectionY, double DirectionZ, double Basis[9])
{
    Basis[0] = 0.282095;
    Basis[1] = 0.488603 * DirectionY;
    Basis[2] = 0.488603 * DirectionZ;
    Basis[3] = 0.488603 * DirectionX;
    Basis[4] = 1.092548 * DirectionX * DirectionY;
    Basis[5] = 1.092548 * DirectionY * DirectionZ;
    Basis[6] = 0.315392 * (3.0 * DirectionZ * DirectionZ - 1.0);
    Basis[7] = 1.092548 * DirectionX * DirectionZ;
    Basis[8] = 0.546274 * (DirectionX * DirectionX - DirectionY * DirectionY);
}

// 📐 The cosine lobe's own harmonic expansion, band by band: π at the zeroth, 2π/3 at the first, π/4 at the
//    second. The convolution is a multiply per band, which is the whole reason a harmonic basis is used here
//    rather than a directional surface.
constexpr double CosineLobe[3] = { 3.141592653589793, 2.0943951023931953, 0.7853981633974483 };

}   // namespace

void IrradianceProjection::Evaluate(double DirectionX, double DirectionY, double DirectionZ,
                                    double& Red, double& Green, double& Blue) const
{
    double Basis[9] = {};
    HarmonicBasis(DirectionX, DirectionY, DirectionZ, Basis);

    double Accumulated[3] = { 0.0, 0.0, 0.0 };

    for (std::uint32_t Harmonic = 0u; Harmonic < 9u; ++Harmonic)
    {
        for (std::uint32_t Component = 0u; Component < 3u; ++Component)
            Accumulated[Component] += Coefficient[Harmonic][Component] * Basis[Harmonic];
    }

    Red   = Accumulated[0] > 0.0 ? Accumulated[0] : 0.0;
    Green = Accumulated[1] > 0.0 ? Accumulated[1] : 0.0;
    Blue  = Accumulated[2] > 0.0 ? Accumulated[2] : 0.0;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    DECLARATION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> AtmosphereIntegrator::DeclareMedium(const MediumSpecification& Declaring)
{
    const Outcome<bool> Validated = Declaring.Validate();

    if (!Validated.ContentPresent)
        return Validated;

    DeclaredMedium = Declaring;
    MediumDeclared = true;

    // 🔴 A medium amendment owes all three. ③ reads ① and ②, so rebuilding it alone against coefficients they
    //    were not built from is a sky-view surface describing an atmosphere that no longer exists.
    MediumOwed  = true;
    SkyViewOwed = true;

    return Outcome<bool>::Deliver(true);
}

Outcome<bool> AtmosphereIntegrator::DeclareSun(double DirectionX, double DirectionY, double DirectionZ)
{
    const double Length = std::sqrt(DirectionX * DirectionX + DirectionY * DirectionY + DirectionZ * DirectionZ);

    if (Length <= 0.0)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "a direction of no length" });

    SunDirectionX = DirectionX / Length;
    SunDirectionY = DirectionY / Length;
    SunDirectionZ = DirectionZ / Length;

    // 🔴 `28` §4: "materially" is a declared threshold and not a strict inequality. A sun that advances by a
    //    hundredth of a degree per rotation would otherwise rebuild ③ on every rotation of a still workspace,
    //    which makes the precomputed surface an expensive way to compute what it was meant to precompute.
    const double Alignment = Bounded(SunDirectionX * BuiltSunX
                                   + SunDirectionY * BuiltSunY
                                   + SunDirectionZ * BuiltSunZ, -1.0, 1.0);

    if (std::acos(Alignment) > SunDirectionMateriality)
        SkyViewOwed = true;

    return Outcome<bool>::Deliver(true);
}

Outcome<bool> AtmosphereIntegrator::DeclareCameraAltitude(double Altitude)
{
    if (!MediumDeclared)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no medium is declared" });

    if (Altitude < 0.0 || Altitude > DeclaredMedium.AtmosphereThickness)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "the altitude lies outside the declared atmosphere" });
    }

    CameraAltitude = Altitude;

    if (BuiltAltitude < 0.0 || std::fabs(CameraAltitude - BuiltAltitude) > CameraAltitudeMateriality)
        SkyViewOwed = true;

    return Outcome<bool>::Deliver(true);
}

void AtmosphereIntegrator::DeclareAtmospherePresence(bool PresenceEnabled)
{
    PresenceDeclared = PresenceEnabled;
}

Outcome<bool> AtmosphereIntegrator::DeclareConstantFloor(const ColourSpecification& Declaring)
{
    if (!Declaring.ColourDeclared())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the floor declares no colour space" });

    ConstantFloor = Declaring;
    FloorDeclared = true;

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE MEDIUM PROFILES
//------------------------------------------------------------------------------------------------------------------------

void AtmosphereIntegrator::Extinction(double Altitude, double& Red, double& Green, double& Blue) const
{
    const double RayleighDensity = std::exp(-Altitude / DeclaredMedium.RayleighScaleHeight);
    const double MieDensity      = std::exp(-Altitude / DeclaredMedium.MieScaleHeight);

    // 📐 The ozone tent: unity at its centre, falling linearly to nothing at the declared half width and staying
    //    there. A tent rather than an exponential because ozone is a layer with a maximum aloft, not a gas that
    //    settles — which is exactly why it colours twilight and the other two do not.
    const double Departure   = std::fabs(Altitude - DeclaredMedium.OzoneCentreAltitude);
    const double OzoneDensity = Departure >= DeclaredMedium.OzoneHalfWidth
                              ? 0.0
                              : 1.0 - Departure / DeclaredMedium.OzoneHalfWidth;

    const double MieTerm = ResolvedCoefficient.MieExtinction * MieDensity;

    Red   = ResolvedCoefficient.RayleighScattering[0] * RayleighDensity
          + ResolvedCoefficient.OzoneAbsorption[0]    * OzoneDensity + MieTerm;
    Green = ResolvedCoefficient.RayleighScattering[1] * RayleighDensity
          + ResolvedCoefficient.OzoneAbsorption[1]    * OzoneDensity + MieTerm;
    Blue  = ResolvedCoefficient.RayleighScattering[2] * RayleighDensity
          + ResolvedCoefficient.OzoneAbsorption[2]    * OzoneDensity + MieTerm;
}

void AtmosphereIntegrator::Scattering(double Altitude,
                                      double& RayleighRed, double& RayleighGreen, double& RayleighBlue,
                                      double& Mie) const
{
    const double RayleighDensity = std::exp(-Altitude / DeclaredMedium.RayleighScaleHeight);
    const double MieDensity      = std::exp(-Altitude / DeclaredMedium.MieScaleHeight);

    RayleighRed   = ResolvedCoefficient.RayleighScattering[0] * RayleighDensity;
    RayleighGreen = ResolvedCoefficient.RayleighScattering[1] * RayleighDensity;
    RayleighBlue  = ResolvedCoefficient.RayleighScattering[2] * RayleighDensity;

    // 🔴 Ozone contributes nothing here. `28` §3 and §7: ozone absorbs **without scattering**, and a component
    //    that appeared in this routine would be one that brightens the sky it is meant to tint.
    Mie = ResolvedCoefficient.MieScattering * MieDensity;
}

//------------------------------------------------------------------------------------------------------------------------
//                                              ① THE TRANSMITTANCE SURFACE
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📐 The mapping ① is baked through, and it is the **same routine** the later lookups invert. The zenith cosine
//    spans the first axis linearly and the altitude the second, which is the donor formulation's own arrangement:
//    the first axis is the wider of the two precisely because the transmittance gradient across the horizon is
//    the steep one. A mapping written twice — once to bake and once to read — is a surface sampled half a texel
//    from where it was written, and the seam appears as a horizon that sits slightly wrong at one altitude only.
void TransmittanceCoordinate(double Radius, double ZenithCosine,
                             double PlanetRadius, double AtmosphereThickness,
                             double& CoordinateAlong, double& CoordinateAcross)
{
    const double Altitude = Radius - PlanetRadius;

    CoordinateAlong  = Bounded(0.5 * (ZenithCosine + 1.0), 0.0, 1.0);
    CoordinateAcross = AtmosphereThickness > 0.0 ? Bounded(Altitude / AtmosphereThickness, 0.0, 1.0) : 0.0;
}

// 📝 The inverse, read by the bake to recover what each texel centre stands for. Declared beside the forward
//    mapping so that an amendment to either is made with the other in view.
void TransmittanceParameter(double CoordinateAlong, double CoordinateAcross,
                            double PlanetRadius, double AtmosphereThickness,
                            double& Radius, double& ZenithCosine)
{
    Radius       = PlanetRadius + CoordinateAcross * AtmosphereThickness;
    ZenithCosine = 2.0 * CoordinateAlong - 1.0;
}

// 📐 One march step's ∫₀ᵈ S·e^{−σₑs} ds, which is S(1 − e^{−σₑd})/σₑ. 🔴 The vanishing-extinction limit S·d is
//    spelled out rather than guarded by a floor under the divisor: an extinction coefficient at the top of the
//    atmosphere is itself of the order any such floor would be, so a floor does not protect the division — it
//    replaces the answer with a different one wherever the air is thin, which is most of the ray.
double StepIntegral(double ScatteringMagnitude, double ExtinctionMagnitude,
                    double StepTransmittance, double StepSize)
{
    if (ExtinctionMagnitude <= 0.0)
        return ScatteringMagnitude * StepSize;

    return ScatteringMagnitude * (1.0 - StepTransmittance) / ExtinctionMagnitude;
}

}   // namespace

Outcome<bool> AtmosphereIntegrator::BuildTransmittance(const QuadratureRule& Rule)
{
    if (!Rule.Derived())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the rule is not derived" });

    const Outcome<bool> Claimed =
        TransmittanceSurface.Construct(TransmittanceExtentAlong, TransmittanceExtentAcross);

    if (!Claimed.ContentPresent)
        return Claimed;

    const double PlanetRadius     = DeclaredMedium.PlanetRadius;
    const double Thickness        = DeclaredMedium.AtmosphereThickness;
    const double AtmosphereRadius = PlanetRadius + Thickness;

    for (std::uint32_t Across = 0u; Across < TransmittanceExtentAcross; ++Across)
    {
        for (std::uint32_t Along = 0u; Along < TransmittanceExtentAlong; ++Along)
        {
            const double CoordinateAlong  = (static_cast<double>(Along)  + 0.5) / TransmittanceExtentAlong;
            const double CoordinateAcross = (static_cast<double>(Across) + 0.5) / TransmittanceExtentAcross;

            double Radius       = 0.0;
            double ZenithCosine = 0.0;
            TransmittanceParameter(CoordinateAlong, CoordinateAcross, PlanetRadius, Thickness,
                                   Radius, ZenithCosine);

            // 📝 The path runs to the atmosphere boundary and is not shortened at the ground. A ray that descends
            //    through the planet accumulates the whole of the dense lower atmosphere twice and extinguishes to
            //    nothing on its own, which is the answer a ground test would write by hand — and it writes it
            //    continuously rather than as a step the horizon texels straddle.
            const double Distance = DistanceToSphere(Radius, ZenithCosine, AtmosphereRadius);

            double DepthRed   = 0.0;
            double DepthGreen = 0.0;
            double DepthBlue  = 0.0;

            // 🔴 One walk, three components, in ordinal order. `02` §5: three separate integrations would
            //    evaluate the same two density profiles three times and would accumulate in three orders.
            if (Distance > 0.0)
            {
                for (std::uint32_t Ordinal = 0u; Ordinal < Rule.DeclaredCount(); ++Ordinal)
                {
                    double Position  = 0.0;
                    double Weighting = 0.0;

                    if (!Rule.Project(Ordinal, 0.0, Distance, Position, Weighting).ContentPresent)
                        continue;

                    const double SampleRadius = AdvanceRadius(Radius, ZenithCosine, Position);

                    double ExtinctionRed   = 0.0;
                    double ExtinctionGreen = 0.0;
                    double ExtinctionBlue  = 0.0;
                    Extinction(SampleRadius - PlanetRadius, ExtinctionRed, ExtinctionGreen, ExtinctionBlue);

                    DepthRed   += ExtinctionRed   * Weighting;
                    DepthGreen += ExtinctionGreen * Weighting;
                    DepthBlue  += ExtinctionBlue  * Weighting;
                }
            }

            // 📐 Beer–Lambert. Every length here is in metres and every coefficient in reciprocal metres, so the
            //    exponent is dimensionless without a conversion — the donor formulation marches in kilometres and
            //    carries a factor of a thousand at exactly this line, which is the factor to look for first if an
            //    atmosphere ported from it is either opaque or absent.
            TransmittanceSurface.Write(Along, Across,
                                       std::exp(-DepthRed), std::exp(-DepthGreen), std::exp(-DepthBlue));
        }
    }

    return Outcome<bool>::Deliver(true);
}

void AtmosphereIntegrator::TransmittanceAt(double Radius, double ZenithCosine,
                                           double& Red, double& Green, double& Blue) const
{
    double CoordinateAlong  = 0.0;
    double CoordinateAcross = 0.0;

    TransmittanceCoordinate(Radius, ZenithCosine,
                            DeclaredMedium.PlanetRadius, DeclaredMedium.AtmosphereThickness,
                            CoordinateAlong, CoordinateAcross);

    TransmittanceSurface.Sample(CoordinateAlong, CoordinateAcross, Red, Green, Blue);
}

//------------------------------------------------------------------------------------------------------------------------
//                                          ② THE MULTIPLE-SCATTERING SURFACE
//------------------------------------------------------------------------------------------------------------------------

void AtmosphereIntegrator::MultiScatterAt(double Radius, double SunZenithCosine,
                                          double& Red, double& Green, double& Blue) const
{
    const double Altitude = Radius - DeclaredMedium.PlanetRadius;

    const double CoordinateAlong  = Bounded(0.5 * (SunZenithCosine + 1.0), 0.0, 1.0);
    const double CoordinateAcross = DeclaredMedium.AtmosphereThickness > 0.0
                                  ? Bounded(Altitude / DeclaredMedium.AtmosphereThickness, 0.0, 1.0)
                                  : 0.0;

    MultiScatterSurface.Sample(CoordinateAlong, CoordinateAcross, Red, Green, Blue);
}

Outcome<bool> AtmosphereIntegrator::BuildMultiScatter()
{
    if (!TransmittanceSurface.SurfaceConstructed())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the transmittance surface does not stand" });

    const Outcome<bool> Claimed =
        MultiScatterSurface.Construct(MultiScatterExtentAlong, MultiScatterExtentAcross);

    if (!Claimed.ContentPresent)
        return Claimed;

    const double PlanetRadius     = DeclaredMedium.PlanetRadius;
    const double Thickness        = DeclaredMedium.AtmosphereThickness;
    const double AtmosphereRadius = PlanetRadius + Thickness;
    const double Reciprocal       = 1.0 / static_cast<double>(MultiScatterDirectionCount);

    for (std::uint32_t Across = 0u; Across < MultiScatterExtentAcross; ++Across)
    {
        for (std::uint32_t Along = 0u; Along < MultiScatterExtentAlong; ++Along)
        {
            const double SunZenithCosine = 2.0 * ((static_cast<double>(Along) + 0.5) / MultiScatterExtentAlong)
                                         - 1.0;
            const double StartRadius     = PlanetRadius
                                         + ((static_cast<double>(Across) + 0.5) / MultiScatterExtentAcross)
                                         * Thickness;

            // 📝 The surface is sun-independent: the sun is placed in the plane the zenith cosine names rather
            //    than read from the declaration, which is why ② survives every rotation the sun moves through.
            const double SunAltitudeSine = std::sqrt(std::fmax(0.0, 1.0 - SunZenithCosine * SunZenithCosine));
            const double SunX            = SunAltitudeSine;
            const double SunY            = SunZenithCosine;
            const double SunZ            = 0.0;

            double SecondOrder[3] = { 0.0, 0.0, 0.0 };
            double Transfer[3]    = { 0.0, 0.0, 0.0 };

            for (std::uint32_t Direction = 0u; Direction < MultiScatterDirectionCount; ++Direction)
            {
                double FirstCoordinate  = 0.0;
                double SecondCoordinate = 0.0;
                ProjectPlanarSample(Direction + 1u, FirstCoordinate, SecondCoordinate);

                // 🔴 `02` §6's one sphere sampling, not a second one written here. `SampleProjection` distributes
                //    the zenith about its **third** axis and `28` carries the zenith on its second — `46`'s upward
                //    convention — so the projected direction is renamed onto this component's frame rather than
                //    reused as it arrives.
                double SampleX = 0.0;
                double SampleY = 0.0;
                double SampleZ = 0.0;
                ProjectSphericalSample(FirstCoordinate, SecondCoordinate, SampleX, SampleY, SampleZ);

                const double ViewX           = SampleX;
                const double ViewY           = SampleZ;
                const double ViewZ           = SampleY;
                const double ViewZenithCosine = ViewY;

                const double Distance = GroundReached(StartRadius, ViewZenithCosine, PlanetRadius)
                                      ? DistanceToSphere(StartRadius, ViewZenithCosine, PlanetRadius)
                                      : DistanceToSphere(StartRadius, ViewZenithCosine, AtmosphereRadius);

                if (Distance <= 0.0)
                    continue;

                const double StepSize   = Distance / static_cast<double>(MultiScatterStepCount);
                double       Throughput[3] = { 1.0, 1.0, 1.0 };

                for (std::uint32_t Step = 0u; Step < MultiScatterStepCount; ++Step)
                {
                    const double Position     = (static_cast<double>(Step) + 0.5) * StepSize;
                    const double SampleRadius = AdvanceRadius(StartRadius, ViewZenithCosine, Position);
                    const double SampleAltitude = SampleRadius - PlanetRadius;

                    double ExtinctionComponent[3] = { 0.0, 0.0, 0.0 };
                    Extinction(SampleAltitude,
                               ExtinctionComponent[0], ExtinctionComponent[1], ExtinctionComponent[2]);

                    double RayleighComponent[3] = { 0.0, 0.0, 0.0 };
                    double MieComponent         = 0.0;
                    Scattering(SampleAltitude,
                               RayleighComponent[0], RayleighComponent[1], RayleighComponent[2], MieComponent);

                    // 📐 The sun's zenith cosine at the sample, against the **local** up — which is the sample's
                    //    own position direction and not the starting one. A ray that travels a hundred kilometres
                    //    around a planet of six thousand has turned measurably under itself.
                    const double LocalX = (ViewX * Position) / SampleRadius;
                    const double LocalY = (StartRadius + ViewY * Position) / SampleRadius;
                    const double LocalZ = (ViewZ * Position) / SampleRadius;

                    const double SampleSunCosine = Bounded(LocalX * SunX + LocalY * SunY + LocalZ * SunZ,
                                                           -1.0, 1.0);

                    double SunTransmit[3] = { 0.0, 0.0, 0.0 };
                    TransmittanceAt(SampleRadius, SampleSunCosine,
                                    SunTransmit[0], SunTransmit[1], SunTransmit[2]);

                    for (std::uint32_t Component = 0u; Component < 3u; ++Component)
                    {
                        const double ScatteringComponent = RayleighComponent[Component] + MieComponent;
                        const double StepTransmittance   = std::exp(-ExtinctionComponent[Component] * StepSize);

                        // 📐 The multiple-scattering approximation is **isotropic** — 1/4π in place of either
                        //    phase. Light that has bounced more than once has forgotten which way it came from,
                        //    and carrying a phase through the series is carrying a direction that no longer exists.
                        const double InScatter = ScatteringComponent / (4.0 * Pi);

                        Transfer[Component] += Throughput[Component]
                                             * StepIntegral(ScatteringComponent,
                                                            ExtinctionComponent[Component],
                                                            StepTransmittance, StepSize);

                        SecondOrder[Component] += Throughput[Component] * SunTransmit[Component]
                                                * InScatter * StepSize;

                        Throughput[Component] *= StepTransmittance;
                    }
                }
            }

            double Psi[3] = { 0.0, 0.0, 0.0 };

            for (std::uint32_t Component = 0u; Component < 3u; ++Component)
            {
                const double Order    = SecondOrder[Component] * Reciprocal;
                const double Fraction = Transfer[Component]    * Reciprocal;

                // 📐 Ψ = L₂ₙd / (1 − f_ms) — the closed form of the geometric series Σ L₂ₙd·f_msⁿ. The divisor is
                //    held below unity because a transfer that reaches one is a medium returning every photon it
                //    receives, which the series does not converge for and which no atmosphere is.
                const double Divisor = Fraction < 0.999 ? 1.0 - Fraction : 0.001;

                Psi[Component] = Order / Divisor;
            }

            MultiScatterSurface.Write(Along, Across, Psi[0], Psi[1], Psi[2]);
        }
    }

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                ③ THE SKY-VIEW SURFACE
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📐 The sky-view parameterisation, forward and inverse, declared as a pair for the reason ①'s pair is. The zenith
//    axis is quadratic **about the horizon** rather than linear in either the angle or its cosine: the horizon is
//    where the radiance gradient is steep and where a linear parameterisation spends a handful of texels, which
//    reaches the artist as a banded sunset over a perfectly smooth zenith.
void SkyViewDirection(double CoordinateAlong, double CoordinateAcross,
                      double& DirectionX, double& DirectionY, double& DirectionZ)
{
    const double Azimuth = (CoordinateAlong * 2.0 - 1.0) * Pi;

    double Zenith = 0.0;

    if (CoordinateAcross < 0.5)
    {
        const double Departure = 1.0 - 2.0 * CoordinateAcross;   // [-] - nothing at the horizon, unity at the nadir

        Zenith = Pi * 0.5 + Departure * Departure * Pi * 0.5;
    }
    else
    {
        const double Departure = 2.0 * CoordinateAcross - 1.0;   // [-] - nothing at the horizon, unity at the zenith

        Zenith = Pi * 0.5 - Departure * Departure * Pi * 0.5;
    }

    const double ZenithSine = std::sin(Zenith);

    DirectionX = ZenithSine * std::cos(Azimuth);
    DirectionY = std::cos(Zenith);
    DirectionZ = ZenithSine * std::sin(Azimuth);
}

void SkyViewCoordinate(double DirectionX, double DirectionY, double DirectionZ,
                       double& CoordinateAlong, double& CoordinateAcross)
{
    const double Azimuth = std::atan2(DirectionZ, DirectionX);
    const double Zenith  = std::acos(Bounded(DirectionY, -1.0, 1.0));

    CoordinateAlong = Bounded(Azimuth / (2.0 * Pi) + 0.5, 0.0, 1.0);

    // 📝 The inverse of the quadratic bias above, and it is a square root rather than a second fit of it. The
    //    azimuth is wrapped by this routine because it is periodic; `ResidentSurface::Sample` clamps both of its
    //    axes and could not wrap one of them without being told which.
    if (Zenith > Pi * 0.5)
    {
        const double Departure = std::sqrt(Bounded((Zenith - Pi * 0.5) / (Pi * 0.5), 0.0, 1.0));

        CoordinateAcross = 0.5 * (1.0 - Departure);
    }
    else
    {
        const double Departure = std::sqrt(Bounded((Pi * 0.5 - Zenith) / (Pi * 0.5), 0.0, 1.0));

        CoordinateAcross = 0.5 * (1.0 + Departure);
    }
}

}   // namespace

Outcome<bool> AtmosphereIntegrator::BuildSkyView()
{
    if (!TransmittanceSurface.SurfaceConstructed() || !MultiScatterSurface.SurfaceConstructed())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "① or ② does not stand" });

    const Outcome<bool> Claimed = SkyViewSurface.Construct(SkyViewExtentAlong, SkyViewExtentAcross);

    if (!Claimed.ContentPresent)
        return Claimed;

    const double PlanetRadius     = DeclaredMedium.PlanetRadius;
    const double AtmosphereRadius = PlanetRadius + DeclaredMedium.AtmosphereThickness;
    const double StartRadius      = PlanetRadius + CameraAltitude;

    for (std::uint32_t Across = 0u; Across < SkyViewExtentAcross; ++Across)
    {
        for (std::uint32_t Along = 0u; Along < SkyViewExtentAlong; ++Along)
        {
            const double CoordinateAlong  = (static_cast<double>(Along)  + 0.5) / SkyViewExtentAlong;
            const double CoordinateAcross = (static_cast<double>(Across) + 0.5) / SkyViewExtentAcross;

            double ViewX = 0.0;
            double ViewY = 0.0;
            double ViewZ = 0.0;
            SkyViewDirection(CoordinateAlong, CoordinateAcross, ViewX, ViewY, ViewZ);

            const double ViewZenithCosine = ViewY;

            const double Distance = GroundReached(StartRadius, ViewZenithCosine, PlanetRadius)
                                  ? DistanceToSphere(StartRadius, ViewZenithCosine, PlanetRadius)
                                  : DistanceToSphere(StartRadius, ViewZenithCosine, AtmosphereRadius);

            double Radiance[3] = { 0.0, 0.0, 0.0 };

            if (Distance > 0.0)
            {
                const double ScatterCosine = Bounded(ViewX * SunDirectionX
                                                   + ViewY * SunDirectionY
                                                   + ViewZ * SunDirectionZ, -1.0, 1.0);

                const double RayleighWeighting = RayleighPhase(ScatterCosine);
                const double MieWeighting      = MiePhase(ScatterCosine, DeclaredMedium.MieAsymmetry);

                const double StepSize      = Distance / static_cast<double>(SkyViewStepCount);
                double       Throughput[3] = { 1.0, 1.0, 1.0 };

                for (std::uint32_t Step = 0u; Step < SkyViewStepCount; ++Step)
                {
                    const double Position       = (static_cast<double>(Step) + 0.5) * StepSize;
                    const double SampleRadius   = AdvanceRadius(StartRadius, ViewZenithCosine, Position);
                    const double SampleAltitude = SampleRadius - PlanetRadius;

                    double ExtinctionComponent[3] = { 0.0, 0.0, 0.0 };
                    Extinction(SampleAltitude,
                               ExtinctionComponent[0], ExtinctionComponent[1], ExtinctionComponent[2]);

                    double RayleighComponent[3] = { 0.0, 0.0, 0.0 };
                    double MieComponent         = 0.0;
                    Scattering(SampleAltitude,
                               RayleighComponent[0], RayleighComponent[1], RayleighComponent[2], MieComponent);

                    const double LocalX = (ViewX * Position) / SampleRadius;
                    const double LocalY = (StartRadius + ViewY * Position) / SampleRadius;
                    const double LocalZ = (ViewZ * Position) / SampleRadius;

                    const double SampleSunCosine = Bounded(LocalX * SunDirectionX
                                                         + LocalY * SunDirectionY
                                                         + LocalZ * SunDirectionZ, -1.0, 1.0);

                    double SunTransmit[3] = { 0.0, 0.0, 0.0 };
                    TransmittanceAt(SampleRadius, SampleSunCosine,
                                    SunTransmit[0], SunTransmit[1], SunTransmit[2]);

                    double Multiple[3] = { 0.0, 0.0, 0.0 };
                    MultiScatterAt(SampleRadius, SampleSunCosine, Multiple[0], Multiple[1], Multiple[2]);

                    for (std::uint32_t Component = 0u; Component < 3u; ++Component)
                    {
                        // 🔴 The two components are phase-weighted **separately**. Rayleigh is near-isotropic and
                        //    Mie is sharply forward, so weighting their sum by either one is a sky with the sun's
                        //    halo spread over the whole dome or with no halo at all.
                        const double Single = (RayleighComponent[Component] * RayleighWeighting
                                             + MieComponent                * MieWeighting)
                                            * SunTransmit[Component];

                        const double Multiplied = (RayleighComponent[Component] + MieComponent)
                                                * Multiple[Component];

                        const double InScatter       = Single + Multiplied;
                        const double StepTransmittance = std::exp(-ExtinctionComponent[Component] * StepSize);

                        Radiance[Component] += Throughput[Component]
                                             * StepIntegral(InScatter, ExtinctionComponent[Component],
                                                            StepTransmittance, StepSize);

                        Throughput[Component] *= StepTransmittance;
                    }
                }
            }

            // 📝 🚧 The illuminant's own magnitude is **not** applied here. `44` §8 enrols the atmospheric source
            //    and the requester supplies its direction; its flux scales this radiance uniformly and applying it
            //    at this depth would bake one illuminant's brightness into a surface rebuilt on direction alone.
            SkyViewSurface.Write(Along, Across, Radiance[0], Radiance[1], Radiance[2]);
        }
    }

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                              THE HARMONIC CONVOLUTION
//------------------------------------------------------------------------------------------------------------------------

void AtmosphereIntegrator::DeriveIrradiance()
{
    ConvolvedIrradiance = IrradianceProjection{};

    if (!SkyViewSurface.SurfaceConstructed())
        return;

    // 📐 The estimator is (4π/N)·Σ L(ω)Y(ω): the sphere's solid angle over the sample count, because the
    //    directions are distributed uniformly in solid angle rather than importance-weighted toward the sky.
    const double SolidAngleShare = 4.0 * Pi / static_cast<double>(IrradianceSampleCount);

    for (std::uint32_t Ordinal = 0u; Ordinal < IrradianceSampleCount; ++Ordinal)
    {
        double FirstCoordinate  = 0.0;
        double SecondCoordinate = 0.0;
        ProjectPlanarSample(Ordinal + 1u, FirstCoordinate, SecondCoordinate);

        double SampleX = 0.0;
        double SampleY = 0.0;
        double SampleZ = 0.0;
        ProjectSphericalSample(FirstCoordinate, SecondCoordinate, SampleX, SampleY, SampleZ);

        const double DirectionX = SampleX;
        const double DirectionY = SampleZ;
        const double DirectionZ = SampleY;

        double CoordinateAlong  = 0.0;
        double CoordinateAcross = 0.0;
        SkyViewCoordinate(DirectionX, DirectionY, DirectionZ, CoordinateAlong, CoordinateAcross);

        double Radiance[3] = { 0.0, 0.0, 0.0 };
        SkyViewSurface.Sample(CoordinateAlong, CoordinateAcross, Radiance[0], Radiance[1], Radiance[2]);

        double Basis[9] = {};
        HarmonicBasis(DirectionX, DirectionY, DirectionZ, Basis);

        for (std::uint32_t Harmonic = 0u; Harmonic < 9u; ++Harmonic)
        {
            for (std::uint32_t Component = 0u; Component < 3u; ++Component)
            {
                ConvolvedIrradiance.Coefficient[Harmonic][Component] +=
                    Radiance[Component] * Basis[Harmonic] * SolidAngleShare;
            }
        }
    }

    // 📐 The convolution itself: one multiply per band, the zeroth against π, the three first-order against 2π/3
    //    and the five second-order against π/4. Convolving after projecting rather than before is what makes the
    //    cosine lobe a constant per band instead of an integral per sample.
    for (std::uint32_t Harmonic = 0u; Harmonic < 9u; ++Harmonic)
    {
        const std::uint32_t Band = Harmonic == 0u ? 0u : (Harmonic < 4u ? 1u : 2u);

        for (std::uint32_t Component = 0u; Component < 3u; ++Component)
            ConvolvedIrradiance.Coefficient[Harmonic][Component] *= CosineLobe[Band];
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE REBUILD
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> AtmosphereIntegrator::Rebuild(const ColourSpaceSpecification& Working, const QuadratureRule& Rule)
{
    if (!MediumDeclared)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no medium is declared" });

    if (!Rule.Derived())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the rule is not derived" });

    // 🔴 `28` §4: with nothing owed, nothing is rebuilt and nothing is recorded. The delivery is not a rebuild of
    //    zero surfaces reported as success — it is the schedule's contributor being told there is no work.
    if (!MediumOwed && !SkyViewOwed)
        return Outcome<bool>::Deliver(true);

    if (MediumOwed)
    {
        const Outcome<MediumCoefficient> Resolved = Resolve(DeclaredMedium, Working, Rule);

        if (!Resolved.ContentPresent)
            return Outcome<bool>::Refuse(Resolved.Declined);

        ResolvedCoefficient = Resolved.Resolve();

        const Outcome<bool> First = BuildTransmittance(Rule);

        if (!First.ContentPresent)
            return First;

        const Outcome<bool> Second = BuildMultiScatter();

        if (!Second.ContentPresent)
            return Second;

        MediumOwed  = false;
        SkyViewOwed = true;
        ++MediumRebuilds;
    }

    if (SkyViewOwed)
    {
        const Outcome<bool> Third = BuildSkyView();

        if (!Third.ContentPresent)
            return Third;

        DeriveIrradiance();

        BuiltSunX     = SunDirectionX;
        BuiltSunY     = SunDirectionY;
        BuiltSunZ     = SunDirectionZ;
        BuiltAltitude = CameraAltitude;
        SkyViewOwed   = false;
        ++SkyViewRebuilds;
    }

    return Outcome<bool>::Deliver(true);
}

bool AtmosphereIntegrator::RebuildOwed() const
{
    return MediumOwed || SkyViewOwed;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE SAMPLED RESULTS
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> AtmosphereIntegrator::SampleSkyView(double DirectionX, double DirectionY, double DirectionZ,
                                                  double& Red, double& Green, double& Blue) const
{
    // 🔴 The disabled atmosphere delivers the floor rather than refusing — `18` §5 and `30` §3 both reach their
    //    second source through this one call, so neither writes the fallback a second time.
    if (!PresenceDeclared)
    {
        Red   = FloorDeclared ? ConstantFloor.RedCoordinate   : 0.0;
        Green = FloorDeclared ? ConstantFloor.GreenCoordinate : 0.0;
        Blue  = FloorDeclared ? ConstantFloor.BlueCoordinate  : 0.0;

        return Outcome<bool>::Deliver(true);
    }

    if (!SkyViewSurface.SurfaceConstructed())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no sky-view surface stands" });

    const double Length = std::sqrt(DirectionX * DirectionX + DirectionY * DirectionY + DirectionZ * DirectionZ);

    if (Length <= 0.0)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "a direction of no length" });

    double CoordinateAlong  = 0.0;
    double CoordinateAcross = 0.0;
    SkyViewCoordinate(DirectionX / Length, DirectionY / Length, DirectionZ / Length,
                      CoordinateAlong, CoordinateAcross);

    SkyViewSurface.Sample(CoordinateAlong, CoordinateAcross, Red, Green, Blue);

    return Outcome<bool>::Deliver(true);
}

Outcome<bool> AtmosphereIntegrator::SampleTransmittance(double Altitude, double ZenithCosine,
                                                        double& Red, double& Green, double& Blue) const
{
    if (!TransmittanceSurface.SurfaceConstructed())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no transmittance surface stands" });

    TransmittanceAt(DeclaredMedium.PlanetRadius + Altitude, Bounded(ZenithCosine, -1.0, 1.0), Red, Green, Blue);

    return Outcome<bool>::Deliver(true);
}

Outcome<bool> AtmosphereIntegrator::AerialTransmittance(double Altitude,
                                                        double DirectionX, double DirectionY, double DirectionZ,
                                                        double Distance,
                                                        const QuadratureRule& Rule,
                                                        double& Red, double& Green, double& Blue) const
{
    Red   = 1.0;
    Green = 1.0;
    Blue  = 1.0;

    if (!MediumDeclared || !ResolvedCoefficient.CoefficientResolved)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no medium is resolved" });

    if (!Rule.Derived())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the rule is not derived" });

    const double Length = std::sqrt(DirectionX * DirectionX + DirectionY * DirectionY + DirectionZ * DirectionZ);

    if (Length <= 0.0)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "a direction of no length" });

    if (Distance <= 0.0)
        return Outcome<bool>::Deliver(true);

    const double Radius       = DeclaredMedium.PlanetRadius + Altitude;
    const double ZenithCosine = Bounded(DirectionY / Length, -1.0, 1.0);

    double DepthRed   = 0.0;
    double DepthGreen = 0.0;
    double DepthBlue  = 0.0;

    for (std::uint32_t Ordinal = 0u; Ordinal < Rule.DeclaredCount(); ++Ordinal)
    {
        double Position  = 0.0;
        double Weighting = 0.0;

        if (!Rule.Project(Ordinal, 0.0, Distance, Position, Weighting).ContentPresent)
            continue;

        const double SampleRadius = AdvanceRadius(Radius, ZenithCosine, Position);

        double ExtinctionRed   = 0.0;
        double ExtinctionGreen = 0.0;
        double ExtinctionBlue  = 0.0;
        Extinction(SampleRadius - DeclaredMedium.PlanetRadius, ExtinctionRed, ExtinctionGreen, ExtinctionBlue);

        DepthRed   += ExtinctionRed   * Weighting;
        DepthGreen += ExtinctionGreen * Weighting;
        DepthBlue  += ExtinctionBlue  * Weighting;
    }

    Red   = std::exp(-DepthRed);
    Green = std::exp(-DepthGreen);
    Blue  = std::exp(-DepthBlue);

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  WHAT IS PRESENTED
//------------------------------------------------------------------------------------------------------------------------

const IrradianceProjection& AtmosphereIntegrator::Irradiance() const     { return ConvolvedIrradiance;  }
const ResidentSurface&      AtmosphereIntegrator::Transmittance() const  { return TransmittanceSurface; }
const ResidentSurface&      AtmosphereIntegrator::MultiScatter() const   { return MultiScatterSurface;  }
const ResidentSurface&      AtmosphereIntegrator::SkyView() const        { return SkyViewSurface;       }

std::uint64_t AtmosphereIntegrator::ResidentBytes() const
{
    return TransmittanceSurface.ResidentBytes()
         + MultiScatterSurface.ResidentBytes()
         + SkyViewSurface.ResidentBytes();
}

std::uint32_t              AtmosphereIntegrator::MediumRebuildCount() const  { return MediumRebuilds;      }
std::uint32_t              AtmosphereIntegrator::SkyViewRebuildCount() const { return SkyViewRebuilds;     }
const MediumSpecification& AtmosphereIntegrator::Medium() const              { return DeclaredMedium;      }
const MediumCoefficient&   AtmosphereIntegrator::Coefficient() const         { return ResolvedCoefficient; }
bool                       AtmosphereIntegrator::AtmospherePresent() const   { return PresenceDeclared;    }

}   // namespace Slate
