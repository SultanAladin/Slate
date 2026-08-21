//============================================================================================================================================
//                                                         OVERLAYFRAGMENT.SLANG
//============================================================================================================================================
// 🧩 The overlay pass's fragment stage: writes the primitive's straight-alpha
//    colour whole, with NO tone mapping and NO premultiplication — the pipeline
//    blends `src_alpha / one_minus_src_alpha`, which is what keeps a vivid gizmo
//    colour vivid over a bright sky (the premultiplied interface blend washed
//    low-alpha lines out). The colour the vertex stage expanded is display-
//    referred, exactly as the interface's own is; nothing here scales it.

#include "Shared/Prelude.slang.h"

namespace Slate
{

struct OverlayVertexOutput
{
    Real32x4   Position : SV_Position;
    Real32x4   Colour   : COLOR0;
};

// 📐 The entry is named `main`: slangc emits the name the [shader(...)] attribute carries, and the
//    engine's ShaderCodec::Stage always binds the "main" entry of a lowered module.
[shader("fragment")]
Real32x4 main(OverlayVertexOutput Input) : SV_Target
{
    return Input.Colour;
}

} // namespace Slate
