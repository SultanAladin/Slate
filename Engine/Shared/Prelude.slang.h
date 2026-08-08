//============================================================================================================================================
//                                                             PRELUDE.SLANG.H
//============================================================================================================================================
// 🧩 Supplies the type, parameter-direction and intrinsic spellings each toolchain lacks.

#pragma once

// 📝 Everything under Shared/ is compiled once by the C++ toolchain and once by the shader toolchain. This
//    file is the only place the two spellings are reconciled. A Shared/ entry point that reaches for a
//    toolchain-specific spelling directly has stopped being shared source, and ParityRunner cannot cover it.

#if defined(SLATE_SHADER_TOOLCHAIN)

//------------------------------------------------------------------------------------------------------------------------
//                                                   SHADER TOOLCHAIN
//------------------------------------------------------------------------------------------------------------------------

#define SLATE_SHARED
#define SLATE_OUT(TypeName)                            out TypeName
#define SLATE_INOUT(TypeName)                          inout TypeName
#define SLATE_INOUT_SPAN(TypeName, SpanName, Capacity) inout TypeName SpanName[Capacity]

typedef double Real64;
typedef float  Real32;
typedef int    Signed32;

Real64 Magnitude(Real64 Operand)
{
    return abs(Operand);
}

#else

//------------------------------------------------------------------------------------------------------------------------
//                                                    HOST TOOLCHAIN
//------------------------------------------------------------------------------------------------------------------------

#include <cmath>
#include <cstdint>

#define SLATE_SHARED                                   inline
#define SLATE_OUT(TypeName)                            TypeName&
#define SLATE_INOUT(TypeName)                          TypeName&
#define SLATE_INOUT_SPAN(TypeName, SpanName, Capacity) TypeName (&SpanName)[Capacity]

using Real64   = double;
using Real32   = float;
using Signed32 = std::int32_t;

SLATE_SHARED Real64 Magnitude(Real64 Operand)
{
    return std::fabs(Operand);
}

#endif
