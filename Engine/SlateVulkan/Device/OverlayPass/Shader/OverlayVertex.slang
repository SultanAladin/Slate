//============================================================================================================================================
//                                                           OVERLAYVERTEX.SLANG
//============================================================================================================================================
// 🧩 The overlay pass's vertex stage: expands every CPU-side primitive into its
//    screen-space triangles ON THE GPU, so the CPU never tessellates.
//
//    One vertex buffer holds every record, in three regions:
//      - lines:      { x0, y0, x1, y1, thickness, a, r, g, b }   (two points per line)
//      - dots:       { x, y, radius, padding, a, r, g, b }       (one point per dot)
//      - triangles:  { x0, y0, x1, y1, x2, y2, a, r, g, b }      (three points)
//    The push constant's `Mode` selects the region: 0 lines, 1 dots, 2 triangles.
//    A line or dot draws 4 × count vertices (the four corners of the expanded
//    quad, chosen by `SV_VertexID % 4`); a triangle draws 3 × count
//    (`SV_VertexID % 3`). The screen-space expansion is the whole point: the CPU
//    record is two points per line and one per dot, so a dense lattice or a
//    high-poly wireframe never reaches the CPU's tessellator.

#include "Shared/Prelude.slang.h"
#include "Shared/OverlayTransform.slang.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE RECORD SHAPES
//------------------------------------------------------------------------------------------------------------------------

// 📐 One CPU record per line: the two endpoints, the thickness, and the straight-alpha colour.
struct LineRecord
{
    Real32x2   Start;       // [px]
    Real32x2   End;         // [px]
    Real32     Thickness;   // [px]
    Real32     Padding[3];  // [-] - alignment to 48 bytes
    Real32x4   Colour;      // [-] - straight alpha, 0..1
};

// 📐 One CPU record per dot: the centre, the radius, and the colour.
struct DotRecord
{
    Real32x2   Centre;      // [px]
    Real32     Radius;      // [px]
    Real32     Padding;     // [-] - alignment to 24 bytes
    Real32x4   Colour;      // [-] - straight alpha, 0..1
};

// 📐 One CPU record per triangle: the three vertices and the colour.
struct TriangleRecord
{
    Real32x2   First;       // [px]
    Real32x2   Second;      // [px]
    Real32x2   Third;       // [px]
    Real32x2   Padding;     // [-] - alignment to 32 bytes
    Real32x4   Colour;      // [-] - straight alpha, 0..1
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PUSH CONSTANT
//------------------------------------------------------------------------------------------------------------------------

// 📐 What one draw reads: which region, and the display size for the screen -> NDC transform.
//    🔴 Mode 0 and 1 draw 4 vertices per record (corner = VertexOrdinal % 4), mode 2 draws 3
//       (corner = VertexOrdinal % 3). The record ordinal is VertexOrdinal / 4 (or / 3).
struct OverlayPushConstant
{
    Unsigned32 Mode;       // [-] - 0 lines, 1 dots, 2 triangles
    Real32     DisplayWidth;   // [px]
    Real32     DisplayHeight;  // [px]
    Real32     Padding;        // [-] - 16-byte alignment
};

[[vk::push_constant]]
ConstantBuffer<OverlayPushConstant> Push;

// 📝 The three record regions of the one vertex buffer, bound as structured pools.
StructuredBuffer<LineRecord>     LinePool;      // [-] - region 0
StructuredBuffer<DotRecord>      DotPool;       // [-] - region 1
StructuredBuffer<TriangleRecord> TrianglePool;  // [-] - region 2

struct OverlayVertexOutput
{
    Real32x4   Position : SV_Position;
    Real32x4   Colour   : COLOR0;
};

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE ENTRY POINT
//------------------------------------------------------------------------------------------------------------------------

// 📐 The entry is named `main`: slangc emits the name the [shader(...)] attribute carries, and the
//    engine's ShaderCodec::Stage always binds the "main" entry of a lowered module.
// 🔴 Every corner is computed from `SV_VertexID` alone — no per-vertex attributes exist. The draw
//    counts ARE the record counts multiplied by the corner count, so the CPU record stays minimal.
[shader("vertex")]
OverlayVertexOutput main(Unsigned32 VertexOrdinal : SV_VertexID)
{
    OverlayVertexOutput Output;

    // 📐 The viewport transform, SHARED with the harness: `OverlayNdcX/Y` live in
    //    `Shared/OverlayTransform.slang.h`, compiled by both the shader toolchain and the host, and
    //    the harness asserts the convention (screen y = 0 lands on NDC +1 — the framebuffer's top
    //    row, exactly as the interface's own vertex shader maps it). 🔴 The previous spelling
    //    `−2y/h − 1` mapped every positive screen y below NDC −1, clipping the whole overlay — the
    //    "grid not showing" defect, which shipped repeatedly because the harness's CPU twin
    //    rasterized in screen space and never exercised this math.
    Real32x2 Position = Real32x2(0.0, 0.0);
    Real32x4 Colour   = Real32x4(1.0, 1.0, 1.0, 1.0);

    if (Push.Mode == 0u)
    {
        // 📐 The line's four corners are the endpoints widened by half the thickness along the
        //    segment's normal. The normal is computed here, keeping the CPU record at four floats.
        const Unsigned32 Record = VertexOrdinal / 4u;
        const Unsigned32 Corner = VertexOrdinal % 4u;

        const LineRecord Line = LinePool[Record];
        const Real32x2 Direction = Line.End - Line.Start;
        const Real32    Length    = sqrt(dot(Direction, Direction));
        const Real32x2 Normal    = (Length > 0.0001)
                                 ? Real32x2(-Direction.y / Length, Direction.x / Length)
                                 : Real32x2(0.0, 1.0);

        const Real32 Half = Line.Thickness * 0.5;
        Position = (Corner < 2u) ? Line.Start : Line.End;
        const Real32 Side = ((Corner == 1u) || (Corner == 3u)) ? 1.0 : -1.0;
        Position += Normal * (Half * Side);
        Colour    = Line.Colour;
    }
    else if (Push.Mode == 1u)
    {
        // 📐 The dot's four corners are the centre widened by the radius along both axes.
        const Unsigned32 Record = VertexOrdinal / 4u;
        const Unsigned32 Corner = VertexOrdinal % 4u;

        const DotRecord Dot = DotPool[Record];
        const Real32 X = ((Corner == 1u) || (Corner == 3u)) ?  1.0 : -1.0;
        const Real32 Y = (Corner >= 2u)                       ?  1.0 : -1.0;

        Position = Dot.Centre + Real32x2(X, Y) * Dot.Radius;
        Colour   = Dot.Colour;
    }
    else
    {
        // 📐 The triangle's three vertices pass through untouched.
        const Unsigned32 Record = VertexOrdinal / 3u;
        const Unsigned32 Corner = VertexOrdinal % 3u;

        const TriangleRecord Triangle = TrianglePool[Record];
        Position = (Corner == 0u) ? Triangle.First
                 : (Corner == 1u) ? Triangle.Second : Triangle.Third;
        Colour   = Triangle.Colour;
    }

    Output.Position = Real32x4(OverlayNdcX(Position.x, Push.DisplayWidth),
                               OverlayNdcY(Position.y, Push.DisplayHeight),
                               0.0, 1.0);
    Output.Colour   = Colour;

    return Output;
}

} // namespace Slate
