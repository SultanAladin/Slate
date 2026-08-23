//============================================================================================================================================
//                                                         OVERLAYFRAGMENT.SLANG
//============================================================================================================================================
// 🧩 The overlay pass's fragment stage: writes the primitive's straight-alpha
//    colour whole, with NO tone mapping and NO premultiplication — the pipeline
//    blends `src_alpha / one_minus_src_alpha`, which is what keeps a vivid gizmo
//    colour vivid over a bright sky (the premultiplied interface blend washed
//    low-alpha lines out). The colour the vertex stage expanded is display-
//    referred, exactly as the interface's own is; nothing here scales it.
//
//    Mode 3 is different in kind: it receives ONE quad and solves the ground
//    lattice analytically, per pixel. See `ResolveGround` below.

#include "Shared/Prelude.slang.h"

namespace Slate
{

struct OverlayVertexOutput
{
    Real32x4   Position : SV_Position;
    Real32x4   Colour   : COLOR0;
};

// 📐 The same push constant the vertex stage declares — Slang requires the
//    layout to agree, and the fragment stage reads the camera terms.
struct OverlayPushConstant
{
    Unsigned32 Mode;
    Real32     DisplayWidth;
    Real32     DisplayHeight;
    Real32     Padding;

    Real32x4   EyeAndCell;
    Real32x4   ForwardAndTanH;
    Real32x4   RightAndTanV;
    Real32x4   UpAndPresent;
    Real32x4   LatticeColour;
    Real32x4   WeightsAndDot;
    Real32x4   LeafRect;
};

[[vk::push_constant]]
ConstantBuffer<OverlayPushConstant> Push;

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE ANALYTIC GROUND
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 How close a coordinate is to a lattice line, in units of that line's own
///    screen-space width — so the answer is a coverage, not a distance, and the
///    line stays one pixel wide however far away it is.
/// note  📐 `fwidth` is the screen-space derivative: it IS the level of detail.
///        The CPU path had to guess a sample step and a density threshold and
///        got both wrong; here the hardware reports the exact figure per pixel.
Real32 LatticeCoverage(Real32 Coordinate, Real32 Cell, Real32 Weight)
{
    const Real32 Scaled  = Coordinate / Cell;
    const Real32 Spacing = fwidth(Scaled);

    if (Spacing <= 0.0)
        return 0.0;

    // 📐 Distance to the nearest whole multiple, in cells, divided by how much a
    //    pixel spans — the classic derivative-normalised grid.
    const Real32 Nearest = abs(frac(Scaled - 0.5) - 0.5) / Spacing;

    return 1.0 - min(Nearest / max(Weight, 0.25), 1.0);
}

/// 🧩 One octave of the lattice, faded out as its lines crowd together.
/// note  📐 An octave whose cells fall below a few pixels apart would be a grey
///        wash; the fade is on the SAME derivative that draws it, so an octave
///        never pops — the defect the CPU path's density threshold produced.
Real32 OctaveCoverage(Real32x2 Ground, Real32 Cell, Real32 Weight)
{
    const Real32 SpanX = fwidth(Ground.x / Cell);
    const Real32 SpanY = fwidth(Ground.y / Cell);
    const Real32 Span  = max(SpanX, SpanY);

    // 📐 Below one cell per two pixels the octave is retired, over a full octave
    //    of travel so the handover between two octaves is continuous.
    const Real32 Fade = 1.0 - smoothstep(0.25, 0.5, Span);

    if (Fade <= 0.0)
        return 0.0;

    const Real32 Along = LatticeCoverage(Ground.x, Cell, Weight);
    const Real32 Cross = LatticeCoverage(Ground.y, Cell, Weight);

    return max(Along, Cross) * Fade;
}

/// 🧩 The ground plane at one pixel: the ray/plane intersection, major and minor
///    subdivisions of lattice, the world axes, and the distance haze.
Real32x4 ResolveGround(Real32x2 Corner)
{
    const Real32x3 Eye     = Push.EyeAndCell.xyz;
    const Real32   Cell    = max(Push.EyeAndCell.w, 0.0001);
    const Real32x3 Forward = Push.ForwardAndTanH.xyz;
    const Real32x3 Right   = Push.RightAndTanV.xyz;
    const Real32x3 Up      = Push.UpAndPresent.xyz;
    const Real32   TanH    = Push.ForwardAndTanH.w;
    const Real32   TanV    = Push.RightAndTanV.w;
    const Real32   Weight  = max(Push.WeightsAndDot.x, 0.25);
    const Real32   DotRadius = max(Push.WeightsAndDot.y, 0.25);
    const Real32   MajorStep = max(Push.WeightsAndDot.z, 2.0);
    const Unsigned32 Present = Unsigned32(Push.UpAndPresent.w + 0.5);

    // ① The pixel's ray, through the same pinhole the sky mesh uses.
    const Real32 NdcX = Corner.x * 2.0 - 1.0;
    const Real32 NdcY = 1.0 - Corner.y * 2.0;

    const Real32x3 Ray = normalize(Right * (NdcX * TanH)
                                 + Up    * (NdcY * TanV)
                                 + Forward);

    // ② The ground is Y = 0. A ray meets the plane from above (Eye.y > 0, Ray.y < 0)
    //    or from below (Eye.y < 0, Ray.y > 0).
    if (abs(Ray.y) <= 1e-6 || (Eye.y * Ray.y >= 0.0))
        return Real32x4(0.0, 0.0, 0.0, 0.0);

    const Real32 Distance = -Eye.y / Ray.y;

    if (Distance <= 0.0)
        return Real32x4(0.0, 0.0, 0.0, 0.0);

    const Real32x2 Ground = Real32x2(Eye.x + Ray.x * Distance,
                                     Eye.z + Ray.z * Distance);

    // ③ Dynamic grid levels: Minor cells, Major cells, and SuperMajor cells.
    const Real32 MinorCell = Cell;
    const Real32 MajorCell = Cell * MajorStep;
    const Real32 SuperCell = MajorCell * MajorStep;

    Real32 Coverage = 0.0;

    if (Present == 1u || Present == 3u)
    {
        const Real32 MinorCov = OctaveCoverage(Ground, MinorCell, Weight);
        const Real32 MajorCov = OctaveCoverage(Ground, MajorCell, Weight * 1.35);
        const Real32 SuperCov = OctaveCoverage(Ground, SuperCell, Weight * 1.70);

        Coverage = max(MinorCov * 0.35, max(MajorCov * 0.75, SuperCov * 1.00));
    }

    // ④ The dotted presentation: nodes at grid intersections.
    if (Present == 2u || Present == 3u)
    {
        const Real32 Coarse    = MajorCell;
        const Real32x2 Scaled  = Ground / Coarse;
        const Real32x2 Span    = Real32x2(fwidth(Scaled.x), fwidth(Scaled.y));
        const Real32x2 Nearest = abs(frac(Scaled - 0.5) - 0.5) / max(Span, Real32x2(1e-6, 1e-6));
        const Real32   Radial  = length(Nearest);
        const Real32   Node    = 1.0 - min(Radial / DotRadius, 1.0);
        const Real32   Fade    = 1.0 - smoothstep(0.25, 0.5, max(Span.x, Span.y));

        const Real32 DotCov = Node * Fade;
        Coverage = max(Coverage, DotCov);
    }

    // ⑤ The world axes, in their own hues, drawn with exact pixel width.
    Real32x3 Tone = Push.LatticeColour.rgb;

    const Real32 SpanX = fwidth(Ground.x);
    const Real32 SpanZ = fwidth(Ground.y);
    const Real32 PixelDistZ = abs(Ground.x) / max(SpanX, 1e-6);
    const Real32 PixelDistX = abs(Ground.y) / max(SpanZ, 1e-6);
    const Real32 AxisZ = 1.0 - min(PixelDistZ / max(Weight * 1.6, 0.25), 1.0);
    const Real32 AxisX = 1.0 - min(PixelDistX / max(Weight * 1.6, 0.25), 1.0);

    if (AxisZ > Coverage) { Coverage = AxisZ; Tone = Real32x3(0.32, 0.44, 0.92); }
    if (AxisX > Coverage) { Coverage = AxisX; Tone = Real32x3(0.90, 0.28, 0.32); }

    if (Coverage <= 0.0)
        return Real32x4(0.0, 0.0, 0.0, 0.0);

    // ⑥ The distance haze fading to the horizon from above or below.
    const Real32 Horizon = 1.0 - smoothstep(0.0, 1.0, Distance / (max(abs(Eye.y), 1.0) * 900.0));

    return Real32x4(Tone, Coverage * Push.LatticeColour.a * Horizon);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE ENTRY POINT
//------------------------------------------------------------------------------------------------------------------------

// 📐 The entry is named `main`: slangc emits the name the [shader(...)] attribute carries, and the
//    engine's ShaderCodec::Stage always binds the "main" entry of a lowered module.
[shader("fragment")]
Real32x4 main(OverlayVertexOutput Input) : SV_Target
{
    if (Push.Mode == 3u)
        return ResolveGround(Input.Colour.xy);

    return Input.Colour;
}

} // namespace Slate
