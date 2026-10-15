//============================================================================================================================================
//                                                        DISPLAYRECOMBINATION.WGSL
//============================================================================================================================================
// 🧩 Recombines direct, indirect diffuse, glossy reflection and emission per pixel, or substitutes an inspection signal.

// 📝 The indirect term arrives demodulated (irradiance E) and is modulated here by the diffuse albedo, so texture-free
//    surfaces keep their edges through the wavelet. Specular indirect uses the split-sum response F0·A + B over the
//    traced reflection; past the reflection cutoff it falls back to E/π, Lumen's rough-reflection reuse. A clear coat
//    reflects the same radiance with its own response and attenuates the layer beneath by its Fresnel transmittance.
//
//    A second target always receives the diffuse exitance ρ/π · (E_sun · max(n·l, 0) · V + E) + emission — exactly what a
//    surface card stores, here with shadow-map visibility and per-pixel irradiance. The next frame's screen traces read
//    it, so it is written whatever the inspection selector shows and never carries an inspection signal.

//! include "FrameDescriptor.wgsl"
//! include "SurfaceReflectance.wgsl"
//! include "PackingCodec.wgsl"

@group(0) @binding(1) var<storage, read> Materials : array<MaterialRecord>;
@group(0) @binding(2) var SurfaceAttributes        : texture_2d<f32>;
@group(0) @binding(3) var SurfaceDepth             : texture_depth_2d;
@group(0) @binding(4) var DirectRadiance           : texture_2d<f32>;
@group(0) @binding(5) var IndirectIrradiance       : texture_2d<f32>;
@group(0) @binding(6) var ReflectionRadiance       : texture_2d<f32>;
@group(0) @binding(7) var ResampledIrradiance      : texture_2d<f32>;
@group(0) @binding(8) var InspectionRadiance       : texture_2d<f32>;
@group(0) @binding(9) var CardFinal                : texture_2d<f32>;
@group(0) @binding(10) var RecombinedTarget        : texture_storage_2d<rgba16float, write>;
@group(0) @binding(11) var ExitanceTarget          : texture_storage_2d<rgba16float, write>;

const InspectionFinal      : u32 = 0u;
const InspectionDirect     : u32 = 1u;
const InspectionDiffuse    : u32 = 2u;
const InspectionIrradiance : u32 = 3u;
const InspectionResampled  : u32 = 4u;
const InspectionReflection : u32 = 5u;
const InspectionDistance   : u32 = 6u;
const InspectionCards      : u32 = 7u;
const InspectionNormal     : u32 = 8u;
const InspectionAlbedo     : u32 = 9u;

// 📝 The two inspections that replace the whole viewport, background included; every other selector passes through.
fn ResolveViewportSubstitute(Pixel: vec2i, Colour: vec3f) -> vec3f
{
    if (Frame.InspectionMode == InspectionDistance)
    {
        return textureLoad(InspectionRadiance, Pixel, 0).rgb;
    }
    if (Frame.InspectionMode == InspectionCards)
    {
        let Extent = vec2f(textureDimensions(CardFinal));
        let Scale = min(Frame.ScreenExtent.x, Frame.ScreenExtent.y) / Extent.x;
        let Texel = vec2i(vec2f(Pixel) / Scale);
        if (all(Texel < vec2i(Extent)))
        {
            return textureLoad(CardFinal, Texel, 0).rgb;
        }
        return vec3f(0.04);
    }
    return Colour;
}

@compute @workgroup_size(8, 8)
fn ResolveRecombination(@builtin(global_invocation_id) Invocation: vec3u)
{
    let Pixel = vec2i(Invocation.xy);
    if (any(vec2f(Pixel) >= Frame.ScreenExtent))
    {
        return;
    }
    let Direct = textureLoad(DirectRadiance, Pixel, 0);
    let Depth = textureLoad(SurfaceDepth, Pixel, 0);
    if (Depth <= 0.0)
    {
        textureStore(ExitanceTarget, Pixel, vec4f(0.0));
        textureStore(RecombinedTarget, Pixel, vec4f(ResolveViewportSubstitute(Pixel, Direct.rgb), 1.0));
        return;
    }
    let Attributes = textureLoad(SurfaceAttributes, Pixel, 0);
    let Normal = DecodeOctahedral(Attributes.xy);
    let Surface = ProjectSurfaceSample(Materials[u32(Attributes.z + 0.5)]);
    let Position = ReconstructWorldPosition(vec2f(Pixel) + 0.5, Depth);
    let View = normalize(Frame.CameraPosition - Position);
    let NdotV = max(dot(Normal, View), 1.0e-3);

    var Irradiance = vec3f(0.0);
    if (FeatureEnabled(Frame, FeatureIndirect))
    {
        Irradiance = textureLoad(IndirectIrradiance, Pixel, 0).rgb * Frame.IndirectScale;
    }
    let Diffuse = ResolveDiffuseAlbedo(Surface) * InversePi * Irradiance;
    let Emission = Surface.Emission * Frame.EmissionScale;

    // 📝 Direct.w is the sun visibility the direct pass resolved for this pixel.
    let SunIrradiance = Frame.SunIlluminance * max(dot(Normal, Frame.SunDirection), 0.0) * Direct.w;
    let Exitance = ResolveDiffuseAlbedo(Surface) * InversePi * (SunIrradiance + Irradiance) + Emission;
    textureStore(ExitanceTarget, Pixel, vec4f(min(Exitance, vec3f(60000.0)), 1.0));

    let GlossRoughness = select(Surface.Roughness, Surface.CoatRoughness, Surface.CoatWeight > 0.0);
    let Reflection = textureLoad(ReflectionRadiance, Pixel, 0);
    var Glossy = Irradiance * InversePi;
    if (FeatureEnabled(Frame, FeatureReflection) && FeatureEnabled(Frame, FeatureIndirect))
    {
        Glossy = mix(Reflection.rgb * Frame.IndirectScale, Glossy, smoothstep(0.35, 0.5, GlossRoughness));
    }
    let LayerResponse = ResolveEnvironmentResponse(NdotV, Surface.Roughness);
    let LayerGlossy = select(Glossy, mix(Glossy, Irradiance * InversePi, smoothstep(0.3, 0.5, Surface.Roughness)), Surface.CoatWeight > 0.0);
    var Indirect = Diffuse + LayerGlossy * (ResolveSpecularColour(Surface) * LayerResponse.x + LayerResponse.y);
    var Specular = LayerGlossy * (ResolveSpecularColour(Surface) * LayerResponse.x + LayerResponse.y);
    if (Surface.CoatWeight > 0.0)
    {
        let CoatResponse = ResolveEnvironmentResponse(NdotV, Surface.CoatRoughness);
        let CoatReflectance = (0.04 * CoatResponse.x + CoatResponse.y) * Surface.CoatWeight;
        Indirect = Indirect * (1.0 - CoatReflectance) + Glossy * CoatReflectance;
        Specular = Specular * (1.0 - CoatReflectance) + Glossy * CoatReflectance;
    }

    var Colour = Direct.rgb + Indirect + Emission;
    switch (Frame.InspectionMode)
    {
        case 1u: { Colour = Direct.rgb + Emission; }
        case 2u: { Colour = Diffuse; }
        case 3u: { Colour = Irradiance * InversePi; }
        case 4u:
        {
            let TracePixel = min(Pixel / i32(Frame.TraceScale), vec2i(Frame.TraceExtent) - vec2i(1));
            Colour = textureLoad(ResampledIrradiance, TracePixel, 0).rgb * InversePi;
        }
        case 5u: { Colour = Specular; }
        case 8u: { Colour = Normal * 0.5 + 0.5; }
        case 9u: { Colour = Surface.Albedo; }
        default: { }
    }
    textureStore(RecombinedTarget, Pixel, vec4f(ResolveViewportSubstitute(Pixel, Colour), 1.0));
}
