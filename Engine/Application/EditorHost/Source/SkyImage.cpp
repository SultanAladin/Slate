//============================================================================================================================================
//                                                               SKYIMAGE.CPP
//============================================================================================================================================

#include "Application/EditorHost/Api/SkyImage.h"

#include "SlateMath/Numeric/QuadratureIntegrator/Api/QuadratureIntegrator.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace Slate
{

namespace
{

constexpr double HalfTurn  = 3.14159265358979323846;
constexpr double SunAngularRadius = 0.00465;   // [rad] - the sun's apparent radius, the donor's own figure

} // namespace

Outcome<bool> GenerateSkyImage(AtmosphereIntegrator& Atmosphere,
                               const EnvironmentConfiguration& Environment,
                               const SkyCamera& Camera,
                               std::uint32_t Width,
                               std::uint32_t Height,
                               std::vector<std::uint8_t>& Pixels)
{
    if (Width == 0u || Height == 0u)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "a sky image of zero extent" });

    // 🔴 The medium is Earth's, declared once per generation — cheap and idempotent: the integrator
    //    rebuilds only what changed. The artist's turbidity and density scale the sky's own terms, and
    //    the elevation/azimuth declare the sun's direction (second axis is the local zenith).
    MediumSpecification Earth;
    Earth.RayleighScaleHeight = 8000.0 * std::clamp(Environment.AtmosphereScaleHeight, 0.2, 3.0);
    Earth.MieScaleHeight      = 1200.0 / std::clamp(Environment.AtmosphereDensity, 0.2, 3.0);

    if (!Atmosphere.DeclareMedium(Earth).Resolved)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the sky medium was rejected" });

    const double SunElevation = std::clamp(Environment.SunElevation, 0.0, 90.0) * HalfTurn / 180.0;
    const double SunAzimuth   = Environment.SunAzimuth * HalfTurn / 180.0;
    const double SunCosine    = std::cos(SunElevation);
    const double SunDirectionX = SunCosine * std::sin(SunAzimuth);
    const double SunDirectionY = std::sin(SunElevation);
    const double SunDirectionZ = SunCosine * std::cos(SunAzimuth);

    if (!Atmosphere.DeclareSun(SunDirectionX, SunDirectionY, SunDirectionZ).Resolved)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the sky sun direction was rejected" });

    // 📝 A ground-level camera. The reference camera stands on the surface; the atmosphere's own
    //    thickness and the sky-view surface are both built around the same start radius.
    if (!Atmosphere.DeclareCameraAltitude(1.0).Resolved)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the sky camera altitude was rejected" });

    // 📝 `02` §5's Gauss–Legendre rule, derived on the recurrence — the same rule the ConsoleHost's
    //    atmosphere verification derives before it rebuilds.
    QuadratureRule Rule;
    if (!Rule.Derive(32u).Resolved)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the sky rule would not derive" });

    if (!Atmosphere.Rebuild(DeclaredWorkingSpace(), Rule).Resolved)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the sky surfaces would not rebuild" });

    // 📝 The camera is carried for the viewport's crop; the dome itself is direction-indexed and
    //    camera-independent.
    static_cast<void>(Camera);

    Pixels.resize(static_cast<std::size_t>(Width) * Height * 4u);

    // 📐 The sun's disc test uses the same direct term the GPU shader omits: the sky-view surface
    //    deliberately excludes the illuminant's own flux, so the disc is added here from the
    //    transmittance toward the sun, scaled by the artist's intensity and temperature.
    const double SunTemperature = std::clamp(Environment.SunTemperature, 1000.0, 12000.0);
    const double TemperatureT   = (SunTemperature - 1000.0) / 11000.0;
    double SunRed = 1.0, SunGreen = 1.0, SunBlue = 1.0;
    if (TemperatureT < 0.5)
    {
        SunRed   = 1.0;
        SunGreen = 0.31 + TemperatureT * 2.0 * 0.69;
        SunBlue  = 0.12 + TemperatureT * 2.0 * 0.16;
    }
    else
    {
        const double Upper = (TemperatureT - 0.5) * 2.0;
        SunRed   = 1.0 - Upper * 0.24;
        SunGreen = 1.0 - Upper * 0.12;
        SunBlue  = 0.43 + Upper * 0.57;
    }

    const double SunIntensity = std::clamp(Environment.SunIntensity, 0.0, 10.0);

    double SunTransmitRed = 0.0, SunTransmitGreen = 0.0, SunTransmitBlue = 0.0;
    static_cast<void>(Atmosphere.SampleTransmittance(1.0, std::sin(SunElevation),
                                                     SunTransmitRed, SunTransmitGreen, SunTransmitBlue));

    const double DirectRed   = SunRed   * SunTransmitRed * 0.95;
    const double DirectGreen = SunGreen * SunTransmitGreen * 0.95;
    const double DirectBlue  = SunBlue  * SunTransmitBlue * 0.95;

    for (std::uint32_t Y = 0u; Y < Height; ++Y)
    {
        for (std::uint32_t X = 0u; X < Width; ++X)
        {
            // 📐 The dome, not a pinhole: every texel is a direction, with the azimuth spread across
            //    the width and the elevation down the height (zenith at the top, nadir at the bottom).
            //    The viewport then crops the dome to the camera's field of view — which keeps the sun
            //    in frame at any viewport aspect, where a pinhole image of one direction would crop
            //    the sun out of a narrow viewport.
            const double DirectionAzimuth = (static_cast<double>(X) + 0.5) / static_cast<double>(Width) * 2.0 * HalfTurn
                                          - HalfTurn;
            const double DirectionElevation = (0.5 - (static_cast<double>(Y) + 0.5) / static_cast<double>(Height))
                                            * HalfTurn;

            const double ElevationCosine = std::cos(DirectionElevation);
            const double RayX = ElevationCosine * std::sin(DirectionAzimuth);
            const double RayY = std::sin(DirectionElevation);
            const double RayZ = ElevationCosine * std::cos(DirectionAzimuth);

            double SkyRed = 0.0, SkyGreen = 0.0, SkyBlue = 0.0;
            static_cast<void>(Atmosphere.SampleSkyView(RayX, RayY, RayZ,
                                                       SkyRed, SkyGreen, SkyBlue));
            // 📐 The sun disc: the angular test against the declared sun direction, then the direct
            //    term added on top of the atmosphere's own scattered radiance — all in radiance space,
            //    before the display scale, so the disc saturates to white while the dome stays soft.
            // 🔴 The dot product is against the sun's own direction components, held outside the loop:
            //    a test that mixes the texel's own azimuth and elevation into the sun direction is a
            //    band across the whole sky at the sun's elevation, not a disc.
            const double SunCosineAngle = std::clamp(RayX * SunDirectionX
                                                    + RayY * SunDirectionY
                                                    + RayZ * SunDirectionZ,
                                                    -1.0, 1.0);
            double Red   = SkyRed;
            double Green = SkyGreen;
            double Blue  = SkyBlue;
            // 📐 The disc angle, with a soft edge so a 0.27° sun reads as a sun and not as a single
            //    white texel. The strength is applied AFTER the tone curve, so the disc keeps the
            //    temperature's warm hue instead of saturating to white with the dome.
            const double DiscAngle = std::acos(std::clamp(SunCosineAngle, -1.0, 1.0));
            // 📐 The disc is drawn wider than the 0.27° figure: three times the real angular radius is a
//    disc of a handful of dome texels, and a disc of one texel reads as noise rather than a
//    sun. The editor's sun is an icon of the real one, like the icons everywhere else in the
//    surface, and its radius is declared here where the icon's size is decided.
const double DiscRadius = SunAngularRadius * 8.0;
            const double DiscStrength = (DiscAngle < DiscRadius)
                ? (1.0 - DiscAngle / DiscRadius) * (1.0 - DiscAngle / DiscRadius) : 0.0;

            // 📐 The sky-view surface stores the radiance per unit solar flux — the shader's own
            //    declaration: "the illuminant's own flux is not applied here… belongs to whoever reads
            //    ③". The reader is this generator, and the flux is the artist's sun intensity times a
            //    display-scale constant, with the sky's own intensity multiplying on top.
            // 🔴 A Reinhard-style curve instead of a linear clamp: the LUT rises quadratically toward
            //    the horizon, so a linear scale clips the whole lower sky to white before the horizon
            //    line — the tone curve keeps the gradient visible and only the sun disc reaches unity.
            const double SkyScale = std::clamp(Environment.SunIntensity, 0.0, 10.0) * 8.0
                                  * std::clamp(Environment.SkyIntensity, 0.0, 3.0);
            const auto Tone = [&](double Radiant) -> double
            {
                const double Scaled = Radiant * SkyScale;
                return Scaled / (1.0 + Scaled);
            };
            Red   = Tone(Red);
            Green = Tone(Green);
            Blue  = Tone(Blue);

            // 📐 The sun's own disc is added after the tone curve, in display space, so it keeps the
            //    temperature-mapped warm hue; the direct term's magnitude is the artist's intensity.
            // 🔴 Clamped at unity: the write is an eight-bit channel, and an unclamped value past one
            //    wraps around to garbage rather than clipping.
            Red   = std::min(Red   + DirectRed   * DiscStrength, 1.0);
            Green = std::min(Green + DirectGreen * DiscStrength, 1.0);
            Blue  = std::min(Blue  + DirectBlue  * DiscStrength, 1.0);

            // 📐 The ground plane: beneath the horizon the sky-view surface holds the horizon's glow
            //    fading to nothing; the editor draws a dark ground there, blended AFTER the tone curve
            //    so the ground's own colour is not scaled with the sky's.
            const double GroundMix = std::clamp(-RayY * 900.0, 0.0, 1.0);
            if (GroundMix > 0.0)
            {
                Red   = Red   * (1.0 - GroundMix) + 0.020 * GroundMix;
                Green = Green * (1.0 - GroundMix) + 0.022 * GroundMix;
                Blue  = Blue  * (1.0 - GroundMix) + 0.026 * GroundMix;
            }

            // 📝 A touch of turbidity warms the dome by pulling it toward the horizon's hue.
            const double Turbidity = std::clamp(Environment.SkyTurbidity, 1.0, 10.0);
            const double Warm = (Turbidity - 1.0) / 9.0 * 0.18;
            Red   = std::min(Red   + Warm * (1.0 - Red),   1.0);
            Green = std::min(Green + Warm * 0.5 * (1.0 - Green), 1.0);
            Blue  = std::min(Blue  - Warm * 0.4 * Blue, 1.0);

            const std::size_t Offset = (static_cast<std::size_t>(Y) * Width + X) * 4u;
            Pixels[Offset + 0u] = static_cast<std::uint8_t>(Red   * 255.0 + 0.5);
            Pixels[Offset + 1u] = static_cast<std::uint8_t>(Green * 255.0 + 0.5);
            Pixels[Offset + 2u] = static_cast<std::uint8_t>(Blue  * 255.0 + 0.5);
            Pixels[Offset + 3u] = 255u;
        }
    }

    return Outcome<bool>::Result(true);
}

} // namespace Slate
