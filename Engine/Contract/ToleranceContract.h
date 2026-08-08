//============================================================================================================================================
//                                                           TOLERANCECONTRACT.H
//============================================================================================================================================
// 🧩 Every tolerance and every capacity two units both read. Depends on nothing; depended on by everything.

#pragma once

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                CROSS-UNIT CAPACITIES
//------------------------------------------------------------------------------------------------------------------------

// 📝 A number declared inside one unit and read by another is a dependency edge wearing a disguise. Each
//    capacity below is read by at least two units, which is exactly why it is declared here and not there.
inline constexpr std::uint32_t PhysicalTileApron         = 4u;   // [texel] - border carried by a resident tile
inline constexpr std::uint32_t TransmissionDepth         = 4u;   // [-]     - packed pairs per pixel, sorted
inline constexpr std::uint32_t DirectOcclusionCapacity   = 4u;   // [-]     - illuminants an RGBA8 target holds
inline constexpr std::uint32_t RecordingRotationDepth    = 2u;   // [-]     - cyclic recording slots — 🚧 open
inline constexpr std::uint32_t DisplayExtentCeiling      = 16384u; // [px]  - largest display extent claimed

//------------------------------------------------------------------------------------------------------------------------
//                                                      TOLERANCES
//------------------------------------------------------------------------------------------------------------------------

// 📝 A tolerance is scale-relative, expressed against the extent of the operand rather than as an absolute
//    distance, because an absolute tolerance is correct at exactly one scene scale.
inline constexpr double WeldTolerance          = 1.0e-5;   // [-] - below this two positions are one position
inline constexpr double CollinearityTolerance  = 1.0e-9;   // [-] - filtered only; the exact path decides
inline constexpr double QuaternionRenormalise  = 1.0e-12;  // [-] - below this a rotation is left untouched

//------------------------------------------------------------------------------------------------------------------------
//                                      FLOATING-POINT CONSTANTS OF THE EXACT PATH
//------------------------------------------------------------------------------------------------------------------------

// 📐 ε is the unit roundoff of the 64-bit representation, 2⁻⁵³. The orientation filter constant is
//    (3 + 16ε)ε, which is the bound below which the sign of the filtered determinant cannot be trusted.
inline constexpr double MachineEpsilon         = 1.1102230246251565e-16;                         // [-] - ε
inline constexpr double OrientationErrorFactor = (3.0 + 16.0 * MachineEpsilon) * MachineEpsilon;  // [-]
inline constexpr double ExpansionSplitter      = 134217729.0;                                    // [-] - 2²⁷+1

}   // namespace Slate
