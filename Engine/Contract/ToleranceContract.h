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
inline constexpr std::uint32_t MaximumWorkingEdge        = 8192u; // [texel] - largest working extent per edge
inline constexpr std::uint32_t TransmissionDepth         = 4u;   // [-]     - packed pairs per pixel, sorted
inline constexpr std::uint32_t DirectOcclusionCapacity   = 4u;   // [-]     - illuminants an RGBA8 target holds
inline constexpr std::uint32_t RecordingRotationDepth    = 2u;   // [-]     - cyclic recording slots — 🚧 open
inline constexpr std::uint32_t DisplayExtentCeiling      = 16384u; // [px]  - largest display extent claimed
inline constexpr std::uint32_t SubPixelSequenceLength    = 8u;   // [-]     - rotations before the offsets repeat
inline constexpr std::uint32_t IlluminantReachCapacity   = 16u;  // [-]     - illuminants one partition may carry
inline constexpr std::uint32_t TilingNestingCeiling       = 1u;  // [-]     - levels a tiling may nest — `54` §3

// 📝 🔴 The three resident atmosphere extents are read by `08` §2's shared-target table in `SlateVulkan` and by
//    `28` in `SlateCompute`. Two units, one set of numbers, so `00` §2 places them here without exception —
//    `08` §2 previously carried them as comments beside the table and `28` §1 as prose, which is the same
//    number written twice in two units and is exactly the disguised edge conflict 30 was recorded to remove.
inline constexpr std::uint32_t TransmittanceExtentAlong    = 256u;   // [px] - altitude
inline constexpr std::uint32_t TransmittanceExtentAcross   = 64u;    // [px] - sun zenith angle
inline constexpr std::uint32_t MultiScatterExtentAlong     = 32u;    // [px] - altitude
inline constexpr std::uint32_t MultiScatterExtentAcross    = 32u;    // [px] - sun zenith angle
inline constexpr std::uint32_t SkyViewExtentAlong          = 192u;   // [px] - view azimuth
inline constexpr std::uint32_t SkyViewExtentAcross         = 108u;   // [px] - view zenith
inline constexpr std::uint32_t AtmosphereComponentCount    = 4u;     // [-]  - RGBA
inline constexpr std::uint32_t AtmosphereComponentBytes    = 2u;     // [B]  - half precision; Tier D by definition

inline constexpr std::uint64_t AtmosphereResidentBytes =
    static_cast<std::uint64_t>(TransmittanceExtentAlong) * TransmittanceExtentAcross
        * AtmosphereComponentCount * AtmosphereComponentBytes
  + static_cast<std::uint64_t>(MultiScatterExtentAlong)  * MultiScatterExtentAcross
        * AtmosphereComponentCount * AtmosphereComponentBytes
  + static_cast<std::uint64_t>(SkyViewExtentAlong)       * SkyViewExtentAcross
        * AtmosphereComponentCount * AtmosphereComponentBytes;   // [B]

// 🔴 `28` §7's first gate, as a build failure rather than as a review remark. The figure was arithmetically
//    wrong once already — `00` §10 conflict 42 records 217 KB against a true 298 KiB — and prose review is
//    what failed to catch it. 128 + 8 + 162 = 298.
static_assert(AtmosphereResidentBytes == 298ull * 1024ull,
              "The three resident atmosphere surfaces must total the declared 298 KiB.");

// 📝 🔴 `54` §3 bounds nesting at one level and `70` resolves what that bound admits, so two units read the
//    number and `00` §2 places it here. A weave whose thread is itself a weave is where the complexity artists
//    want lives; a second level makes resolution cost unbounded, and `20` §2.2's evaluation-cost budget cannot
//    bound what it cannot predict.

// 📝 🚧 `64` §9 leaves the offset sequence length open and it blocks convergence quality alone. It is declared
//    here rather than in `64` because `46` applies the offset, `64` accumulates across it and `82` replays it —
//    three units reading one number, which `00` §2 places in `Contract/` without exception.

// 📝 🔴 `44` §5's reach capacity is read by `44` and by `60` §3.1, which truncates again at the narrower packed
//    capacity of `DirectOcclusionSurface`. Two units, one number, so `00` §2 places it here. Exceeding it is a
//    truncation reported through `86`, never a silent drop.

// 📝 🔴 `MaximumWorkingEdge` is `20` §1's largest working extent per edge, and `68` §5 sizes its inter-chart gap
//    against it together with the apron. Two units read it, so `00` §2 places it here rather than in either —
//    which is precisely the edge conflict 30 was recorded to remove, closed at the declaration rather than by
//    a comment promising the read is "only a constant".

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE DEPTH CONVENTION
//------------------------------------------------------------------------------------------------------------------------

// 📐 🔴 Depth is **reversed** — the nearest plane maps to one and the furthest to zero. `46` §3 declares it a
//    repository-wide convention rather than one document's private choice: `16`'s comparison, `30`'s ray march,
//    `60`'s occlusion comparison and `80` ⑩'s depth test all read it. Reversed depth with a floating-point target
//    distributes precision where perspective takes it away; the forward arrangement spends its precision near the
//    camera, where the perspective divide has already supplied precision for free, and starves the distance.
// ⚠️ One document reversing its own test in isolation produces geometry that **vanishes** rather than geometry
//    that sorts wrongly, which is why the two ordinates are constants and not literals at each comparison.
inline constexpr double NearPlaneDepth = 1.0;   // [-] - clip depth at the nearest plane
inline constexpr double FarPlaneDepth  = 0.0;   // [-] - clip depth at the furthest plane

// 📝 The clip ordinate is inverted for the device's downward-increasing display ordinate. Declared here because
//    `46` derives the projection and `16` reconstructs a position from a pixel; a sign the two spell separately
//    is a sign they will eventually spell differently.
inline constexpr double ClipOrdinateSignum = -1.0;   // [-] - applied to the projection's second row

// 📐 Frustum planes are pushed outward by this fraction of their own distance, matching `38` §6's extents and
//    `40` §6's subdivision. An inward-rounded plane culls geometry the camera can see, and the artist meets it as
//    a surface that disappears along one edge of the display.
inline constexpr double FrustumOutwardMargin = 1.0e-6;   // [-] - relative, with an absolute floor at the origin

//------------------------------------------------------------------------------------------------------------------------
//                                                      TOLERANCES
//------------------------------------------------------------------------------------------------------------------------

// 📝 A tolerance is scale-relative, expressed against the extent of the operand rather than as an absolute
//    distance, because an absolute tolerance is correct at exactly one scene scale.
inline constexpr double WeldTolerance          = 1.0e-5;   // [-] - below this two positions are one position
inline constexpr double ImpressionSpacingFloor = 0.01;     // [-] - finest spacing, as a fraction of the extent
inline constexpr double CollinearityTolerance  = 1.0e-9;   // [-] - filtered only; the exact path decides
inline constexpr double QuaternionRenormalise  = 1.0e-12;  // [-] - below this a rotation is left untouched

// 📐 The Newton criterion the Gauss–Legendre abscissae are derived against, and the ceiling that bounds the
//    derivation. `02` §8 gates that no tolerance literal appears outside `Contract/`, so neither may sit at the
//    derivation site — a criterion written there is one that is tuned there, and a rule derived to two different
//    criteria in two builds integrates to two different numbers.
// 📝 Newton on the Legendre recurrence converges quadratically from the standard initial estimate, so the
//    ceiling is reached only by an implementation defect rather than by a hard input.
inline constexpr double        QuadratureConvergence      = 1.0e-15;   // [-] - Newton step below which it settles
inline constexpr std::uint32_t QuadratureIterationCeiling = 128u;      // [-] - iterations before it stops

// 📐 `28` §4's "materially". A rebuild on any camera movement at all makes a precomputed surface an expensive
//    way to compute what it was meant to precompute; a strict inequality makes every rebuild condition true.
inline constexpr double SunDirectionMateriality = 0.0035;   // [rad] - about a fifth of a degree
inline constexpr double CameraAltitudeMateriality = 10.0;   // [m]

// 📝 🔴 The spacing floor is `58` §5's and is read by `22`, which resamples a path at it and whose impression
//    count it therefore bounds. Two units, one number, so `00` §2 places it here. It is applied **and said** —
//    `58` reports reaching it through `86` rather than coarsening a stroke silently.

//------------------------------------------------------------------------------------------------------------------------
//                                      FLOATING-POINT CONSTANTS OF THE EXACT PATH
//------------------------------------------------------------------------------------------------------------------------

// 📐 ε is the unit roundoff of the 64-bit representation, 2⁻⁵³. The orientation filter constant is
//    (3 + 16ε)ε, which is the bound below which the sign of the filtered determinant cannot be trusted.
inline constexpr double MachineEpsilon         = 1.1102230246251565e-16;                         // [-] - ε
inline constexpr double OrientationErrorFactor = (3.0 + 16.0 * MachineEpsilon) * MachineEpsilon;  // [-]
inline constexpr double IncircleErrorFactor    = (10.0 + 96.0 * MachineEpsilon) * MachineEpsilon; // [-]
inline constexpr double ExpansionSplitter      = 134217729.0;                                    // [-] - 2²⁷+1

//------------------------------------------------------------------------------------------------------------------------
//                                          MATHEMATICAL AND SAMPLING CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 🔴 Declared once, here, for the reason `02` §8 declares tolerances here: a constant transcribed a second
//    time is transcribed to a second precision, and two subsystems that disagree in the sixteenth place produce
//    a seam nobody attributes to a literal.
inline constexpr double Pi = 3.14159265358979323846;   // [-] - the circle constant

// 📐 A Bounded shared entry point is compared against this many units in the last place rather than for
//    equality. A direction that departs from unit length by more has not merely rounded — it has taken a
//    different path through its own arithmetic, which is what parity exists to catch.
inline constexpr double SampleUnitPlaceCeiling = 8.0;   // [-] - units in the last place

}   // namespace Slate
