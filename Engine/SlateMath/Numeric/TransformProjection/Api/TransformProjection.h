//============================================================================================================================================
//                                                          TRANSFORMPROJECTION.H
//============================================================================================================================================
// 🧩 Decomposed transforms, their composition, and the rebasing that precedes every narrowing to 32-bit.

#pragma once

#include "Contract/PrecisionContract.h"
#include "Contract/ToleranceContract.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                               POSITIONS AND ROTATIONS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 A position in document space or view-relative space, at 64-bit.
/// note  Scene extents exceed 32-bit relative precision, which is why document space is never 32-bit.
/// tag   nonallocating, nonthrowing
struct DocumentPosition
{
    double  PositionX = 0.0;   // [mm] - along the document x axis
    double  PositionY = 0.0;   // [mm] - along the document y axis
    double  PositionZ = 0.0;   // [mm] - along the document z axis
};

/// 🧩 A position after rebasing, narrowed for the device.
/// note  🔴 Only ever produced by Rebase. A 32-bit position that did not pass through it is jitter with a
///       plausible-looking cause.
/// tag   nonallocating, nonthrowing
struct DevicePosition
{
    float  PositionX = 0.0f;   // [mm] - relative to the view origin
    float  PositionY = 0.0f;   // [mm] - relative to the view origin
    float  PositionZ = 0.0f;   // [mm] - relative to the view origin
};

/// 🧩 A rotation as a unit quaternion.
/// note  📐 Compounding quaternions renormalises and multiplying matrices does not, which is what lets the
///       containment relation in `12` compound to unbounded depth without drift.
/// tag   nonallocating, nonthrowing
struct RotationQuaternion
{
    double  ImaginaryX = 0.0;   // [-] - 𝑖 coefficient
    double  ImaginaryY = 0.0;   // [-] - 𝑗 coefficient
    double  ImaginaryZ = 0.0;   // [-] - 𝑘 coefficient
    double  Real       = 1.0;   // [-] - scalar coefficient; identity rotation as declared
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE TRANSFORM
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 A transform stored decomposed — never as a general matrix.
/// note  Matrix form is derived at the point of use and never stored back. `Project` derives it; nothing
///       caches it, because a cached matrix is a second representation that drifts from the first.
/// tag   nonallocating, nonthrowing
struct DecomposedTransform
{
    DocumentPosition    Translation = {};                     // [mm] - the origin this transform places
    RotationQuaternion  Rotation    = {};                     // [-]  - unit quaternion
    double              ScaleX      = 1.0;                    // [-]  - along the local x axis
    double              ScaleY      = 1.0;                    // [-]  - along the local y axis
    double              ScaleZ      = 1.0;                    // [-]  - along the local z axis
};

/// 🧩 A transform as sixteen coefficients, column-major, derived for one use and discarded.
/// tag   nonallocating, nonthrowing
struct ProjectedTransform
{
    double  Coefficient[16] = { 1.0, 0.0, 0.0, 0.0,
                                0.0, 1.0, 0.0, 0.0,
                                0.0, 0.0, 1.0, 0.0,
                                0.0, 0.0, 0.0, 1.0 };   // [-] - column-major
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     COMPOUNDING
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Compounds two rotations and renormalises the result.
/// in    OuterRotation  [-]  applied second
/// in    InnerRotation  [-]  applied first
/// out   Compounded     [-]  a unit quaternion
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
RotationQuaternion Compound(RotationQuaternion OuterRotation, RotationQuaternion InnerRotation);
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded);

/// 🧩 Compounds two decomposed transforms without ever forming a matrix.
/// in    OuterTransform [-]  applied second — the containing transform
/// in    InnerTransform [-]  applied first — the contained transform
/// out   Compounded     [-]  decomposed, ready to compound again
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
DecomposedTransform Compound(const DecomposedTransform& OuterTransform,
                             const DecomposedTransform& InnerTransform);
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded);

/// 🧩 Derives the matrix form of a decomposed transform at the point of use.
/// in    Source     [-]  the decomposed transform
/// out   Projected  [-]  column-major coefficients; never stored back into Source
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
ProjectedTransform Project(const DecomposedTransform& Source);
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded);

//------------------------------------------------------------------------------------------------------------------------
//                                                       REBASING
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Rebases a document position to the view origin and narrows it for the device.
/// in    Subject    [mm]  a position in document space
/// in    ViewOrigin [mm]  the current camera position, in document space
/// out   Rebased    [mm]  relative to the view origin, at 32-bit
/// note  🔴 The subtraction happens in 64-bit, before the narrowing. Every position crossing into
///       `SlateCompute` passes through here; `02` §8 gates it and the failure it prevents reads as a
///       driver defect rather than as the arithmetic it is.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
DevicePosition Rebase(DocumentPosition Subject, DocumentPosition ViewOrigin);
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded);

}   // namespace Slate
