//============================================================================================================================================
//                                                        DYNAMICSKYSURFACE.SLANG
//============================================================================================================================================
// FP32 kilometre-scaled interactive atmosphere presentation. The physical LUT entry points remain the
// reference-quality route; this pass is the continuous editor/game dome and never requires shader FP64.

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
};

[[vk::push_constant]] ConstantBuffer<SkyPush> Sky;
[[vk::binding(0, 0)]] RWTexture2D<float4> SkyOutput;

static const float Pi = 3.14159265358979323846f;

float3 TemperatureColour(float Kelvin)
{
    float t = clamp(Kelvin, 1000.0f, 12000.0f) / 100.0f;
    float r = t <= 66.0f ? 1.0f : clamp(1.2929362f * pow(t - 60.0f, -0.1332048f), 0.0f, 1.0f);
    float g = t <= 66.0f ? clamp(0.3900816f * log(t) - 0.6318414f, 0.0f, 1.0f)
                         : clamp(1.1298909f * pow(t - 60.0f, -0.0755148f), 0.0f, 1.0f);
    float b = t >= 66.0f ? 1.0f : (t <= 19.0f ? 0.0f : clamp(0.5432068f * log(t - 10.0f) - 1.1962541f, 0.0f, 1.0f));
    return float3(r, g, b);
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
    float rayleighPhase = 0.0596831f * (1.0f + mu * mu);
    float g = clamp(Sky.MieAsymmetry, -0.95f, 0.95f);
    float mieBase = max(1.0f + g * g - 2.0f * g * mu, 0.0001f);
    float miePhase = 0.1193662f * (1.0f - g * g) * (1.0f + mu * mu) /
                     ((2.0f + g * g) * mieBase * sqrt(mieBase));

    float aboveHorizon = smoothstep(-0.035f, 0.02f, view.y);
    float horizonMass = 1.0f / max(0.08f, view.y + 0.13f);
    float solarMass = 1.0f / max(0.08f, sun.y + 0.13f);
    float altitudeFade = exp(-max(Sky.CameraAltitudeKilometres, 0.0f) /
                             max(Sky.RayleighScaleHeightKilometres, 0.1f));
    float rayleigh = Sky.RayleighDensity * altitudeFade * horizonMass;
    float mie = Sky.MieDensity * exp(-max(Sky.CameraAltitudeKilometres, 0.0f) /
                                     max(Sky.MieScaleHeightKilometres, 0.05f)) * horizonMass;

    float3 wavelengths = float3(0.34f, 0.58f, 1.0f);
    float3 ozone = exp(-Sky.OzoneDensity * solarMass * float3(0.06f, 0.13f, 0.035f));
    float twilight = smoothstep(-0.18f, 0.08f, sun.y);
    float3 scattered = wavelengths * (rayleighPhase * rayleigh) +
                       float3(1.0f, 0.78f, 0.56f) * (miePhase * mie * 0.18f);
    scattered *= ozone * lerp(float3(0.18f, 0.035f, 0.12f), float3(1.0f, 1.0f, 1.0f), twilight);
    scattered *= max(Sky.SunIlluminance, 0.0f) * 0.75f * max(Sky.SkyIntensity, 0.0f);

    float3 ground = float3(0.018f, 0.021f, 0.025f) + Sky.GroundAlbedo * float3(0.035f, 0.03f, 0.025f);
    float3 radiance = lerp(ground, scattered, aboveHorizon);

    float discCosine = cos(max(Sky.SunAngularRadiusDegrees, 0.001f) * Pi / 180.0f);
    float disc = smoothstep(discCosine, min(1.0f, discCosine + (1.0f - discCosine) * 0.55f), mu);
    radiance += TemperatureColour(Sky.SunTemperatureKelvin) * disc * Sky.SunDiscIntensity *
                max(Sky.SunIlluminance, 0.0f) * 2.5f;

    radiance *= exp2(Sky.ExposureCompensation);
    radiance = radiance / (1.0f + radiance);
    radiance = pow(max(radiance, 0.0f), float3(1.0f / 2.2f));
    SkyOutput[index.xy] = float4(radiance, 1.0f);
}
