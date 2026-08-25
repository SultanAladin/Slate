//============================================================================================================================================
//                                                           WORKSPACECADVERTEX.SLANG
//============================================================================================================================================
// 🧩 The parametric workspace CAD pass's vertex stage: expands the bounded CPU CAD packet into triangles on the
//    GPU and fits the exact sketch plane into the current viewport leaf.

#include "Shared/Prelude.slang.h"
#include "Shared/OverlayTransform.slang.h"

namespace Slate
{

static const Unsigned32 WorkspaceCadSegmentKind = 0u;
static const Unsigned32 WorkspaceCadFillKind    = 1u;
static const Unsigned32 WorkspaceCadMarkerKind  = 2u;

struct SegmentRecord
{
    Real32x2 Start;
    Real32x2 End;
    Real32 Thickness;
    Real32 Padding[3];
    Real32x4 Colour;
};

struct FillRecord
{
    Real32x2 First;
    Real32x2 Second;
    Real32x2 Third;
    Real32x2 Padding;
    Real32x4 Colour;
};

struct MarkerRecord
{
    Real32x2 Centre;
    Real32 Radius;
    Unsigned32 Subject;
    Real32x4 Colour;
};

struct CadPushConstant
{
    Unsigned32 Kind;
    Real32 DisplayWidth;
    Real32 DisplayHeight;
    Real32 Padding;
    Real32 MinimumAlong;
    Real32 MinimumAcross;
    Real32 MaximumAlong;
    Real32 MaximumAcross;
    Real32x4 ClipRect;
};

[[vk::push_constant]]
ConstantBuffer<CadPushConstant> Push;

StructuredBuffer<SegmentRecord> SegmentPool;
StructuredBuffer<FillRecord> FillPool;
StructuredBuffer<MarkerRecord> MarkerPool;

struct WorkspaceCadVertexOutput
{
    Real32x4 Position : SV_Position;
    Real32x4 Colour   : COLOR0;
    Real32x2 Local    : TEXCOORD0;
    Unsigned32 Kind   : TEXCOORD1;
    Unsigned32 Subject : TEXCOORD2;
};

Unsigned32 QuadCorner(Unsigned32 VertexIndex)
{
    const Unsigned32 Within = VertexIndex % 6u;
    if (Within == 0u) return 0u;
    if (Within == 1u) return 1u;
    if (Within == 2u) return 2u;
    if (Within == 3u) return 2u;
    if (Within == 4u) return 1u;
    return 3u;
}

Real32x2 FitToLeaf(Real32 Along, Real32 Across)
{
    const Real32 Pad = 18.0;
    const Real32 SafeAlong = max(Push.MaximumAlong - Push.MinimumAlong, 1.0);
    const Real32 SafeAcross = max(Push.MaximumAcross - Push.MinimumAcross, 1.0);
    const Real32 LeafWidth = max(Push.ClipRect.z - Push.ClipRect.x, 1.0);
    const Real32 LeafHeight = max(Push.ClipRect.w - Push.ClipRect.y, 1.0);
    const Real32 Scale = min((LeafWidth - Pad * 2.0) / SafeAlong,
                             (LeafHeight - Pad * 2.0) / SafeAcross);
    const Real32 OccupiedX = SafeAlong * Scale;
    const Real32 OccupiedY = SafeAcross * Scale;
    const Real32 OriginX = Push.ClipRect.x + (LeafWidth - OccupiedX) * 0.5;
    const Real32 OriginY = Push.ClipRect.y + (LeafHeight - OccupiedY) * 0.5;

    return Real32x2(OriginX + (Along - Push.MinimumAlong) * Scale,
                    OriginY + OccupiedY - (Across - Push.MinimumAcross) * Scale);
}

[shader("vertex")]
WorkspaceCadVertexOutput main(Unsigned32 VertexIndex : SV_VertexID)
{
    WorkspaceCadVertexOutput Output;
    Output.Position = Real32x4(0.0, 0.0, 0.0, 1.0);
    Output.Colour = Real32x4(1.0, 1.0, 1.0, 1.0);
    Output.Local = Real32x2(0.0, 0.0);
    Output.Kind = Push.Kind;
    Output.Subject = 0u;

    Real32x2 Screen = Real32x2(0.0, 0.0);

    if (Push.Kind == WorkspaceCadFillKind)
    {
        const Unsigned32 Record = VertexIndex / 3u;
        const Unsigned32 Corner = VertexIndex % 3u;
        const FillRecord Triangle = FillPool[Record];
        Screen = Corner == 0u ? FitToLeaf(Triangle.First.x, Triangle.First.y)
               : Corner == 1u ? FitToLeaf(Triangle.Second.x, Triangle.Second.y)
                              : FitToLeaf(Triangle.Third.x, Triangle.Third.y);
        Output.Colour = Triangle.Colour;
    }
    else if (Push.Kind == WorkspaceCadSegmentKind)
    {
        const Unsigned32 Record = VertexIndex / 6u;
        const Unsigned32 Corner = QuadCorner(VertexIndex);
        const SegmentRecord Segment = SegmentPool[Record];
        const Real32x2 Start = FitToLeaf(Segment.Start.x, Segment.Start.y);
        const Real32x2 End = FitToLeaf(Segment.End.x, Segment.End.y);
        const Real32x2 Direction = End - Start;
        const Real32 Length = sqrt(dot(Direction, Direction));
        const Real32x2 Normal = Length > 0.0001 ? Real32x2(-Direction.y / Length, Direction.x / Length)
                                                : Real32x2(0.0, 1.0);
        const Real32 Side = ((Corner == 1u) || (Corner == 3u)) ? 1.0 : -1.0;
        Screen = (Corner < 2u) ? Start : End;
        Screen += Normal * (Segment.Thickness * 0.5 * Side);
        Output.Colour = Segment.Colour;
    }
    else
    {
        const Unsigned32 Record = VertexIndex / 6u;
        const Unsigned32 Corner = QuadCorner(VertexIndex);
        const MarkerRecord Marker = MarkerPool[Record];
        const Real32x2 Centre = FitToLeaf(Marker.Centre.x, Marker.Centre.y);
        const Real32 LocalX = ((Corner == 1u) || (Corner == 3u)) ? 1.0 : -1.0;
        const Real32 LocalY = Corner >= 2u ? 1.0 : -1.0;
        Output.Local = Real32x2(LocalX, LocalY);
        Output.Subject = Marker.Subject;
        Screen = Centre + Output.Local * Marker.Radius;
        Output.Colour = Marker.Colour;
    }

    Output.Position = Real32x4(OverlayNdcX(Screen.x, Push.DisplayWidth),
                               OverlayNdcY(Screen.y, Push.DisplayHeight),
                               0.0, 1.0);
    return Output;
}

} // namespace Slate
