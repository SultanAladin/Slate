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

/// 🧩 The ground plane at one pixel: the ray/plane intersection, three octaves of
///    lattice, the two world axes, and the distance haze that replaces the CPU
///    lattice's hard far edge.
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
    const Unsigned32 Present = Unsigned32(Push.UpAndPresent.w + 0.5);

    // ① The pixel's ray, through the same pinhole the sky mesh uses.
    const Real32 NdcX = Corner.x * 2.0 - 1.0;
    const Real32 NdcY = 1.0 - Corner.y * 2.0;

    const Real32x3 Ray = normalize(Right * (NdcX * TanH)
                                 + Up    * (NdcY * TanV)
                                 + Forward);

    // ② The ground is Y = 0. A ray that does not descend never meets it, so the
    //    horizon falls out of the arithmetic rather than being a drawn edge.
    // 🔴 This is what the CPU path could not express: it had a FINITE patch, so
    //    its far boundary was a visible line that moved with the eye height —
    //    the reported "the grid tilts when I look up/down" and "Q/E moves the
    //    grid the opposite way". An infinite plane has no boundary to move.
    if (Ray.y >= -0.00001 || Eye.y <= 0.0)
        return Real32x4(0.0, 0.0, 0.0, 0.0);

    const Real32 Distance = -Eye.y / Ray.y;

    if (Distance <= 0.0)
        return Real32x4(0.0, 0.0, 0.0, 0.0);

    const Real32x2 Ground = Real32x2(Eye.x + Ray.x * Distance,
                                     Eye.z + Ray.z * Distance);

    // ③ Three octaves, each eight times the last. The fine one carries the
    //    ground at the artist's feet, the coarse one runs to the horizon, and
    //    the derivative fade hands over between them without a seam.
    Real32 Coverage = 0.0;

    if (Present == 1u || Present == 3u)
    {
        // 🔴 The octaves run FINER than the declared cell, not coarser. At a 1.5 m
        //    eye the whole lower screen sees ground inside 25 m — measured, screen
        //    row 285 of 300 is 2.0 m away — so a finest octave AT the 20 m cell
        //    puts at most one line in the near field and the artist's feet are
        //    bare. 1/8 · 1 · 8 covers 2.5 m to 160 m; 1 · 8 · 64 covered 20 m to
        //    1280 m and left everything near the camera empty.
        Coverage = max(Coverage, OctaveCoverage(Ground, Cell * 0.125, Weight) * 0.45);
        Coverage = max(Coverage, OctaveCoverage(Ground, Cell,         Weight) * 0.75);
        Coverage = max(Coverage, OctaveCoverage(Ground, Cell * 8.0,   Weight) * 1.00);
    }

    // ④ The dotted presentation: a node at each intersection rather than a line.
    if (Present == 2u || Present == 3u)
    {
        const Real32 DotRadius = max(Push.WeightsAndDot.y, 0.25);
        const Real32 Coarse    = Cell * 8.0;
        const Real32x2 Scaled  = Ground / Coarse;
        const Real32x2 Span    = Real32x2(fwidth(Scaled.x), fwidth(Scaled.y));
        const Real32x2 Nearest = abs(frac(Scaled - 0.5) - 0.5) / max(Span, Real32x2(1e-6, 1e-6));
        const Real32   Radial  = length(Nearest);
        const Real32   Node    = 1.0 - min(Radial / DotRadius, 1.0);
        const Real32   Fade    = 1.0 - smoothstep(0.25, 0.5, max(Span.x, Span.y));

        Coverage = max(Coverage, Node * Fade);
    }

    // ⑤ The world axes, in their own hues, drawn wider than a lattice line.
    Real32x3 Tone = Push.LatticeColour.rgb;

    const Real32 AxisZ = LatticeCoverage(Ground.x, 1.0e9, Weight * 1.6);
    const Real32 AxisX = LatticeCoverage(Ground.y, 1.0e9, Weight * 1.6);

    if (AxisZ > Coverage) { Coverage = AxisZ; Tone = Real32x3(0.32, 0.44, 0.92); }
    if (AxisX > Coverage) { Coverage = AxisX; Tone = Real32x3(0.90, 0.28, 0.32); }

    if (Coverage <= 0.0)
        return Real32x4(0.0, 0.0, 0.0, 0.0);

    // ⑥ The distance haze. The lattice is infinite, so without this it would
    //    alias into a shimmering mat at the horizon; fading it out is what makes
    //    an infinite plane read as ground rather than as noise.
    const Real32 Horizon = 1.0 - smoothstep(0.0, 1.0, Distance / (max(Eye.y, 1.0) * 900.0));

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
