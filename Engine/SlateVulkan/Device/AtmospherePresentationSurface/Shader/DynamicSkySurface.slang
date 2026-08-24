//============================================================================================================================================
//                                                        DYNAMICSKYSURFACE.SLANG
//============================================================================================================================================
// 🧩 Interactive FP32 atmospheric presentation pass for the continuous editor & game sky dome.
// 📐 Implements physical Rayleigh-Mie scattering, Chappuis ozone absorption, and smooth astronomical twilight gates.

struct SkyPush
{
    float SunElevationDegrees;
    float SunAzimuthDegrees;
    float SunIlluminance;
    float SunTemperatureKelvin;
    float SunAngularRadiusDegrees;
    float SunDiscIntensity;
    float SkyIntensity;
    float ExposureCompensation;
    float GroundAlbedo;
    float RayleighDensity;
    float RayleighScaleHeightKilometres;
    float MieDensity;
    float MieScaleHeightKilometres;
    float MieAsymmetry;
    float OzoneDensity;
    float CameraAltitudeKilometres;
    uint Quality;
};

[[vk::push_constant]] ConstantBuffer<SkyPush> Sky;
[[vk::binding(0, 0)]] RWTexture2D<float4> SkyOutput;

static const float Pi = 3.14159265358979323846f;

// 📝 Planckian blackbody color approximation for direct solar disc temperature.
float3 TemperatureColour(float Kelvin)
{
    float t = clamp(Kelvin, 1000.0f, 12000.0f) / 100.0f;
    float r = t <= 66.0f ? 1.0f : clamp(1.2929362f * pow(t - 60.0f, -0.1332048f), 0.0f, 1.0f);
    float g = t <= 66.0f ? clamp(0.3900816f * log(t) - 0.6318414f, 0.0f, 1.0f)
                         : clamp(1.1298909f * pow(t - 60.0f, -0.0755148f), 0.0f, 1.0f);
    float b = t >= 66.0f ? 1.0f : (t <= 19.0f ? 0.0f : clamp(0.5432068f * log(t - 10.0f) - 1.1962541f, 0.0f, 1.0f));
    return float3(r, g, b);
}

// 📐 ACES Film Tone Mapping curve for natural HDR highlight compression and rich sky color saturation.
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 index : SV_DispatchThreadID)
{
    uint width, height;
    SkyOutput.GetDimensions(width, height);
    if (index.x >= width || index.y >= height)
        return;

    float2 uv = (float2(index.xy) + 0.5f) / float2(width, height);
    float azimuth = (uv.x - 0.5f) * (2.0f * Pi);
    float elevation = (0.5f - uv.y) * Pi;
    float cosElevation = cos(elevation);
    float3 view = float3(sin(azimuth) * cosElevation, sin(elevation), cos(azimuth) * cosElevation);

    float sunElevation = Sky.SunElevationDegrees * Pi / 180.0f;
    float sunAzimuth = Sky.SunAzimuthDegrees * Pi / 180.0f;
    float3 sun = normalize(float3(sin(sunAzimuth) * cos(sunElevation), sin(sunElevation),
                                  cos(sunAzimuth) * cos(sunElevation)));

    float mu = clamp(dot(view, sun), -1.0f, 1.0f);
    
    // Phase Functions
    float rayleighPhase = 0.0596831f * (1.0f + mu * mu);
    float g = clamp(Sky.MieAsymmetry, -0.95f, 0.95f);
    float mieBase = max(1.0f + g * g - 2.0f * g * mu, 0.0001f);
    float miePhase = 0.1193662f * (1.0f - g * g) * (1.0f + mu * mu) /
                     ((2.0f + g * g) * mieBase * sqrt(mieBase));

    float quality = clamp(float(Sky.Quality), 0.0f, 3.0f);
    float horizonSoftness = lerp(0.04f, 0.015f, quality / 3.0f);
    float aboveHorizon = smoothstep(-horizonSoftness, horizonSoftness * 0.5f, view.y);

    // 📐 Spherical Curved Atmosphere Optical Airmass Lengths (prevents flat plane horizon fog at midday)
    float viewCos = max(view.y, 0.001f);
    float horizonMass = 1.0f / (viewCos + 0.045f * exp(-viewCos * 7.5f));
    
    float sunCos = max(sun.y, -0.20f);
    float solarMass = 1.0f / (max(sunCos, 0.01f) + 0.045f * exp(-max(sunCos, 0.01f) * 7.5f));

    float altitudeFade = exp(-max(Sky.CameraAltitudeKilometres, 0.0f) /
                             max(Sky.RayleighScaleHeightKilometres, 0.1f));
    float rayleigh = Sky.RayleighDensity * altitudeFade * horizonMass;
    float mie = Sky.MieDensity * exp(-max(Sky.CameraAltitudeKilometres, 0.0f) /
                                     max(Sky.MieScaleHeightKilometres, 0.05f)) * horizonMass;

    // 🌐 Physical Twilight & Day-Night Transition Gates
    // Civil twilight: sun.y in [0.0, -0.104] (0° to -6°)
    // Nautical twilight: sun.y in [-0.104, -0.208] (-6° to -12°)
    // Astronomical twilight / Night: sun.y < -0.208 (<-12°)
    float daylight = smoothstep(-0.04f, 0.06f, sun.y);
    float twilightCivil = smoothstep(-0.11f, 0.02f, sun.y) * (1.0f - smoothstep(0.02f, 0.25f, sun.y));
    float twilightNautical = smoothstep(-0.22f, -0.06f, sun.y) * (1.0f - smoothstep(-0.06f, 0.02f, sun.y));
    float nightAmount = 1.0f - smoothstep(-0.22f, -0.04f, sun.y);

    // 🎨 Rayleigh Wavelength Extinction & Solar Transmittance along the path
    float3 rayleighWavelengths = float3(0.18f, 0.42f, 1.0f);
    
    // Physical Chappuis Ozone Band Absorption (peak green/yellow absorption)
    float3 ozoneCoeffs = float3(0.10f, 0.22f, 0.02f);
    float3 sunTransmittance = exp(-solarMass * (rayleighWavelengths * Sky.RayleighDensity * 0.12f + 
                                               ozoneCoeffs * Sky.OzoneDensity * 0.35f));

    // Daytime Atmospheric Scattering
    float3 daylightScatter = rayleighWavelengths * (rayleighPhase * rayleigh * 0.85f) +
                             float3(1.0f, 0.90f, 0.75f) * (miePhase * mie * 0.16f);
    daylightScatter *= sunTransmittance * max(Sky.SunIlluminance, 0.0f) * max(Sky.SkyIntensity, 0.0f) * daylight;

    // 🌅 Sunset / Sunrise Golden-Orange Horizon Glow
    // Concentrated at the horizon and toward the sun azimuth during golden hour and civil twilight.
    float sunsetAzimuthGlow = pow(max(mu, 0.0f), 2.5f);
    float horizonBand = exp(-abs(view.y) * 14.0f);
    float3 sunsetColor = lerp(float3(1.0f, 0.38f, 0.04f), float3(0.92f, 0.18f, 0.08f), clamp(-sun.y * 8.0f, 0.0f, 1.0f));
    float3 sunsetScatter = sunsetColor * horizonBand * (0.35f + 0.65f * sunsetAzimuthGlow) *
                           (twilightCivil * 0.85f + (1.0f - daylight) * 0.4f) *
                           max(Sky.SunIlluminance, 0.0f) * max(Sky.SkyIntensity, 0.0f);

    // 🌌 Pre-Dawn / Nautical Twilight Bluish-Blackish Transition Band
    float nauticalBand = exp(-abs(view.y) * 8.0f) * twilightNautical;
    float3 nauticalScatter = float3(0.02f, 0.06f, 0.18f) * nauticalBand * (0.4f + 0.6f * max(mu, 0.0f));

    // 🌃 Proper Dark Night Sky (starlight & airglow gradient)
    float zenithNorm = clamp(view.y * 0.5f + 0.5f, 0.0f, 1.0f);
    float3 nightSky = lerp(float3(0.0006f, 0.0010f, 0.0022f),
                           float3(0.0015f, 0.0030f, 0.0080f), zenithNorm) * nightAmount;

    // Multiple Scattering Approximation
    float multipleScatter = quality >= 2.0f
                          ? (quality - 1.0f) * 0.04f * exp(-abs(view.y) * 4.0f) * daylight
                          : 0.0f;
    float3 scattered = daylightScatter + sunsetScatter + nauticalScatter + nightSky +
                       float3(0.30f, 0.45f, 0.70f) * multipleScatter;

    // Ground Reflection
    float3 groundDay = float3(0.005f, 0.006f, 0.007f) +
                       Sky.GroundAlbedo * float3(0.015f, 0.014f, 0.012f) * daylight;
    float3 groundNight = float3(0.0004f, 0.0006f, 0.0010f) * nightAmount;
    float3 radiance = lerp(groundDay + groundNight, scattered, aboveHorizon);

    // ☀️ Direct Solar Disc (Strictly masked above horizon to eliminate pre-sunrise white horizon line)
    float discRadius = max(Sky.SunAngularRadiusDegrees, 0.001f) * Pi / 180.0f;
    float discCosine = cos(discRadius);
    float discEdge = lerp(0.25f, 0.65f, quality / 3.0f);
    float disc = smoothstep(discCosine, min(1.0f, discCosine + (1.0f - discCosine) * discEdge), mu);

    // Mask sun disc strictly so it disappears completely when dipping below horizon
    float discAboveHorizon = smoothstep(0.0f, sin(discRadius) * 2.0f, sun.y) * aboveHorizon;
    
    // Solar Disc color attenuated by sun transmittance at sunset
    float3 discColor = TemperatureColour(Sky.SunTemperatureKelvin) * lerp(float3(1.0f, 1.0f, 1.0f), sunTransmittance, 0.75f);
    radiance += discColor * disc * discAboveHorizon *
                Sky.SunDiscIntensity * max(Sky.SunIlluminance, 0.0f) * 2.5f;

    // Exposure & ACES Tone Mapping
    radiance *= exp2(Sky.ExposureCompensation);
    radiance = ACESFilm(radiance);
    radiance = pow(max(radiance, 0.0f), float3(1.0f / 2.2f));

    SkyOutput[index.xy] = float4(radiance, 1.0f);
}
