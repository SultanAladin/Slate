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
#define SLATE_CONSTEXPR
#define SLATE_OUT(TypeName)                            out TypeName
#define SLATE_INOUT(TypeName)                          inout TypeName
#define SLATE_INOUT_SPAN(TypeName, SpanName, Capacity) inout TypeName SpanName[Capacity]

typedef double   Real64;
typedef float    Real32;
typedef int      Signed32;
typedef uint     Unsigned32;
typedef uint64_t Unsigned64;

// 📝 The two component spellings a resident surface is declared and sampled through, and the **only** ones. They
//    are device-side alone and deliberately have no host counterpart: everything under `Shared/` is scalar
//    throughout, so a host form would be a spelling nothing in shared source is permitted to reach for. A surface
//    of four components must say so at its declaration — `Texture2D<Real32>` samples one component and silently
//    delivers red where three were meant, which reads as a monochrome sky rather than as a mistake.
typedef float2   Real32x2;
typedef float4   Real32x4;

Real64 Magnitude(Real64 Operand)
{
    return abs(Operand);
}

Real64 SquareRoot(Real64 Operand)
{
    return sqrt(Operand);
}

Real64 Sine(Real64 Operand)
{
    return sin(Operand);
}

Real64 Cosine(Real64 Operand)
{
    return cos(Operand);
}

Real64 Exponential(Real64 Operand)
{
    return exp(Operand);
}

Real64 ArcCosine(Real64 Operand)
{
    return acos(Operand);
}

Real64 ArcTangentQuadrant(Real64 Numerator, Real64 Denominator)
{
    return atan2(Numerator, Denominator);
}

Real64 BoundedMagnitude(Real64 Operand, Real64 Lower, Real64 Upper)
{
    return Operand < Lower ? Lower : (Operand > Upper ? Upper : Operand);
}

Unsigned32 ReversedBits(Unsigned32 Operand)
{
    return reversebits(Operand);
}

#else

//------------------------------------------------------------------------------------------------------------------------
//                                                    HOST TOOLCHAIN
//------------------------------------------------------------------------------------------------------------------------

#include <cmath>
#include <cstdint>

#define SLATE_SHARED                                   inline
#define SLATE_CONSTEXPR                                constexpr
#define SLATE_OUT(TypeName)                            TypeName&
#define SLATE_INOUT(TypeName)                          TypeName&
#define SLATE_INOUT_SPAN(TypeName, SpanName, Capacity) TypeName (&SpanName)[Capacity]

using Real64     = double;
using Real32     = float;
using Signed32   = std::int32_t;
using Unsigned32 = std::uint32_t;
using Unsigned64 = std::uint64_t;

SLATE_SHARED Real64 Magnitude(Real64 Operand)
{
    return std::fabs(Operand);
}

SLATE_SHARED Real64 SquareRoot(Real64 Operand)
{
    return std::sqrt(Operand);
}

SLATE_SHARED Real64 Sine(Real64 Operand)
{
    return std::sin(Operand);
}

SLATE_SHARED Real64 Cosine(Real64 Operand)
{
    return std::cos(Operand);
}

SLATE_SHARED Real64 Exponential(Real64 Operand)
{
    return std::exp(Operand);
}

SLATE_SHARED Real64 ArcCosine(Real64 Operand)
{
    return std::acos(Operand);
}

SLATE_SHARED Real64 ArcTangentQuadrant(Real64 Numerator, Real64 Denominator)
{
    return std::atan2(Numerator, Denominator);
}

// 📝 The bound is spelled here rather than reached for per toolchain because the shader form's `clamp` and the
//    host's nested conditional disagree on a non-finite operand, and a zenith cosine arriving as one is exactly
//    the input a parity comparison is meant to catch rather than to silently normalise.
SLATE_SHARED Real64 BoundedMagnitude(Real64 Operand, Real64 Lower, Real64 Upper)
{
    return Operand < Lower ? Lower : (Operand > Upper ? Upper : Operand);
}

// 📝 The shader toolchain supplies a bit reversal and the host does not, which is exactly the divergence this
//    file exists to absorb. The host form reverses in five constant-width exchanges rather than in a loop, so
//    both forms are branch-free and neither can diverge on the ordinal it is given.
SLATE_SHARED Unsigned32 ReversedBits(Unsigned32 Operand)
{
    Operand = ((Operand & 0x55555555u) << 1)  | ((Operand & 0xAAAAAAAAu) >> 1);
    Operand = ((Operand & 0x33333333u) << 2)  | ((Operand & 0xCCCCCCCCu) >> 2);
    Operand = ((Operand & 0x0F0F0F0Fu) << 4)  | ((Operand & 0xF0F0F0F0u) >> 4);
    Operand = ((Operand & 0x00FF00FFu) << 8)  | ((Operand & 0xFF00FF00u) >> 8);

    return (Operand << 16) | (Operand >> 16);
}

#endif
