//============================================================================================================================================
//                                                   EXTRUSIONSPECIFICATION.H
//============================================================================================================================================
// 🧩 Exact extrusion input and result declarations. No polygon generation or document mutation is performed here.

#pragma once

#include "SlateGeometry/Topology/SolidStructure/Api/SolidStructure.h"

namespace Slate
{

/// 🧩 Exact 3D direction. It is intentionally unnormalised: normalisation is explicit kernel work.
struct ExactDirection3
{
    RationalScalar X{};
    RationalScalar Y{};
    RationalScalar Z{};

    constexpr bool Declared() const
    {
        return X.Declared() && Y.Declared() && Z.Declared()
            && (X.Numerator != 0 || Y.Numerator != 0 || Z.Numerator != 0);
    }
};

/// 🧩 A future exact feature request formed from a closed planar curve profile.
struct ExtrusionSpecification
{
    CurveName SourceProfile{};
    ExactDirection3 Direction{};
    RationalScalar Distance{};
    bool CapFirst = true;
    bool CapLast = true;

    bool Declared() const;
};

enum class ExtrusionDisposition : std::uint32_t
{
    NotRequested,
    InvalidSpecification,
    ImplementationAbsent,
    Produced
};

/// 🧩 Explicit result shape for eventual extrusion execution.
struct ExtrusionResult
{
    SolidName ProducedSolid{};
    ExtrusionDisposition Disposition = ExtrusionDisposition::NotRequested;
};

/// 🧩 Validates an exact request and reports that execution is not yet implemented.
ExtrusionResult EvaluateExtrusion(const ExtrusionSpecification& Specification);

} // namespace Slate
