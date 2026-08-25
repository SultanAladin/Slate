//============================================================================================================================================
//                                                         WORKSPACECADFRAGMENT.SLANG
//============================================================================================================================================
// 🧩 The parametric workspace CAD pass's fragment stage: writes display-referred straight-alpha CAD colour,
//    masking marker quads into discs.

#include "Shared/Prelude.slang.h"

namespace Slate
{

static const Unsigned32 WorkspaceCadSegmentKind = 0u;
static const Unsigned32 WorkspaceCadFillKind    = 1u;
static const Unsigned32 WorkspaceCadMarkerKind  = 2u;

struct WorkspaceCadVertexOutput
{
    Real32x4 Position : SV_Position;
    Real32x4 Colour   : COLOR0;
    Real32x2 Local    : TEXCOORD0;
    Unsigned32 Kind   : TEXCOORD1;
    Unsigned32 Subject : TEXCOORD2;
};

[shader("fragment")]
Real32x4 main(WorkspaceCadVertexOutput Input) : SV_Target
{
    if (Input.Kind == WorkspaceCadMarkerKind)
    {
        const Real32 Radius = dot(Input.Local, Input.Local);
        if (Radius > 1.0)
            discard;
    }

    return Input.Colour;
}

} // namespace Slate
