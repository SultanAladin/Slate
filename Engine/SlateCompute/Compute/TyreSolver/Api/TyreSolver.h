//============================================================================================================================================
//                                                               TYRESOLVER.H
//============================================================================================================================================
// 🧩 The tyre carcass lattice, its cords coloured for parallel projection, and the host form of the XPBD frame.

#pragma once

#include "Foundation/DeliveryGuarantee.h"
#include "Foundation/PrecisionGuarantee.h"
#include "Shared/TyreConstraintProjection.slang.h"

#include <cstdint>
#include <vector>

namespace Slate
{

// 📐 🔴 WHY THIS IS NOT A RADIAL SPRING, AND WHY IT IS NOT A SHADER EITHER. The alternatives were weighed before a
//    line of this was written, against what a driving game needs from a tyre — a contact patch with a length
//    and a width, a sidewall that bulges under load and squirms under slip, a belt that flattens, and a load the
//    vehicle's rigid body can read back every frame. Cost rises left to right: ✔️ low · 🚩 medium · 🔴 high.
//
//    | Mechanism                        | Patch | Bulge | Load back | Device-resident | Cost  | Standing              |
//    |----------------------------------|-------|-------|-----------|-----------------|-------|-----------------------|
//    | Radial spring per ray            | 🔴    | 🔴    | 🟢        | pointless       | ✔️    | rejected by the brief  |
//    | Vertex-shader squash             | 🔴    | 🟢    | 🔴        | 🟢              | ✔️    | rejected by the brief  |
//    | Mass–spring explicit             | 🟢    | 🟢    | 🟢        | 🟢              | 🚩    | stiffness-limited Δt   |
//    | FEM corotational (Neo-Hookean)   | 🟢    | 🟢    | 🟢        | 🚩              | 🔴    | tetrahedra, a solve    |
//    | Projective dynamics              | 🟢    | 🟢    | 🟢        | 🚩              | 🔴    | global factorisation   |
//    | Jolt soft body (host XPBD)       | 🟢    | 🟢    | 🟢        | 🔴              | 🚩    | the host bottleneck    |
//    | XPBD, cords + shape block + contact | 🟢 | 🟢    | 🟢        | 🟢              | 🚩    | this unit              |
//
//    XPBD wins on the one column the brief fixed: every projection is local except the three shape families —
//    the volume, the rings and the meridians — which are one reduction and one eighty-one-row solve per tyre,
//    and the load is the solver's own multiplier read back. Projective dynamics and FEM carry a global linear
//    solve over every particle, which the device does badly at this size; the shape block here is a solve over
//    eighty-one rows, which it does in the time a dispatch takes to launch. The explicit mass–spring needs a step
//    the belt stiffness makes microseconds long. `ExternalPackages/jolt` holds a very good host XPBD soft body;
//    it is the thing this unit moves off the host, and its formulation is close enough that the material
//    numbers transfer.
//
// 📐 🔴 WHAT WAS TRIED AND DOES NOT WORK, so that nobody tries it again. (1) The pressure as a volume target
//    V₀(1 + p₀/γp_abs) swept in turn with the cords: a two-centimetre push and pull every sweep, never settling,
//    the tyre fourteen percent too large. (2) The pressure as an explicit force p₀∇V in the prediction: a
//    nine-gram particle moves seven centimetres a substep under it, and the cords, swept from λ = 0, cannot form
//    the hoop tension that answers it — a ring's radial mode converges a few percent per sweep. (3) Carrying the
//    cord multipliers across substeps as a warm start: the incomplete sweep's displacement goes into the
//    recovered velocity and is applied again next substep by the carried λ, and the pair diverges within a
//    tenth of a second. What works is below: the pressure **inside** a block that is solved together with the
//    hoop and meridional tensions, so the balance is found rather than approached, a static pre-stress so that
//    the rest shape is the inflated shape, and a step short enough that what is left to approach is small.

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE SPECIFICATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One tyre's geometry, construction and inflation — a 225/45 R17 on a 7.5 J rim unless amended.
/// note  📝 Stiffnesses are per metre of lattice width — EA per unit width of the carcass — so that refining the
///        lattice leaves the tyre's behaviour where it was. A per-cord stiffness would make a finer tyre stiffer.
/// note  ⚠️ `Segments` must be even and `Rings` at least four, which `Validate` refuses otherwise: the cord
///        colouring wraps around the circumference by parity, and an odd count would put two cords of one colour
///        on the seam particle.
/// tag   nonallocating, nonthrowing
struct TyreSpecification
{
    double         RimRadius               = 0.2159;      // [m]       - 17 inch rim
    double         OuterRadius             = 0.3172;      // [m]       - rim plus a 45 percent section of 225 mm
    double         SectionWidth            = 0.225;       // [m]       - across the sidewall bulge
    double         BeadWidth               = 0.1905;      // [m]       - 7.5 J rim; the beads sit this far apart
    double         CrownFlattening         = 0.55;        // [-]       - below unity flattens the tread; unity is a semi-ellipse
    double         CarcassMass             = 9.0;         // [kg]      - the real carcass, spread over the free particles
    double         MassScale               = 3.0;         // [-]       - numerical; see the note on the solver's step below
    double         InflationPressure       = 220000.0;    // [Pa]      - gauge; 2.2 bar
    double         AmbientPressure         = 101325.0;    // [Pa]      - absolute, outside the carcass
    double         AdiabaticIndex          = 1.4;         // [-]       - γ of air
    double         BeltStiffness           = 3.0e6;       // [N/m]     - circumferential, crown rings, per metre of width
    double         SidewallHoopStiffness   = 1.0e5;       // [N/m]     - circumferential, sidewall rings, per metre of width
    double         PlyStiffness            = 8.0e5;       // [N/m]     - meridional bead to bead, per metre of width
    double         ShearStiffness          = 5.0e4;       // [N/m]     - the two diagonal families, per metre of width
    double         BendStiffness           = 2.0e4;       // [N/m]     - second-neighbour meridional, per metre of width
    double         StaticFriction          = 1.0;         // [-]       - μₛ against the ground
    double         KineticFriction         = 0.85;        // [-]       - μₖ against the ground
    double         DeformationDamping      = 20.0;        // [1/s]     - of velocity relative to the rim's rigid motion
    double         TreadThickness          = 0.010;       // [m]       - the contact stands this far outside the particle
    std::uint32_t  Rings                   = 16u;         // [-]       - meridional samples, beads included
    std::uint32_t  Segments                = 64u;         // [-]       - circumferential samples; even

    /// 🧩 Whether the specification describes a tyre that can be built.
    /// out   Result  [-]  refuses with ContentUnsupported for a non-positive extent, mass, pressure or stiffness,
    ///                     an outer radius not above the rim, a bead wider than the section, or an odd segment count
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    Deliver<bool> Validate() const;
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE LATTICE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One run of cords no two of which share a particle — projected in one dispatch.
/// tag   nonallocating, nonthrowing
struct CordColour
{
    std::uint32_t  Offset = 0u;   // [-] - first cord of the colour in the sorted cord run
    std::uint32_t  Count  = 0u;   // [-] - cords in the colour
};

/// 🧩 One rim's pose and motion, as the vehicle declares it to the solver.
/// note  📝 The rotation is a unit quaternion taking the rim frame — axis along local Z — into the world. A
///        wheel rolling forward along +X with +Y up spins about −Z.
/// tag   nonallocating, nonthrowing
struct HubPose
{
    double  X                = 0.0;   // [m]     - rim centre
    double  Y                = 0.0;   // [m]
    double  Z                = 0.0;   // [m]
    double  RotationX        = 0.0;   // [-]     - unit quaternion, vector part
    double  RotationY        = 0.0;   // [-]
    double  RotationZ        = 0.0;   // [-]
    double  RotationW        = 1.0;   // [-]     - scalar part
    double  LinearVelocityX  = 0.0;   // [m/s]
    double  LinearVelocityY  = 0.0;   // [m/s]
    double  LinearVelocityZ  = 0.0;   // [m/s]
    double  AngularVelocityX = 0.0;   // [rad/s]
    double  AngularVelocityY = 0.0;   // [rad/s]
    double  AngularVelocityZ = 0.0;   // [rad/s]
};

/// 🧩 What one tyre pressed back with over the frame — the readback the vehicle's rigid body integrates against.
/// note  🔴 These are decoded from the solver's own multipliers, not re-derived from penetration depths. The
///        force is Σ λₙ/Δt² averaged over the substeps; a vehicle applying it at the rim centre together with the
///        torque is applying exactly what the particles felt.
/// note  🔴 HOW THE VEHICLE READS IT FROM THE DEVICE. `TyreDeviceExchange::Load` delivers this `RecordingSlotCount`
///        frames after the hub pose it was measured at was declared — the readback never waits on the device.
///        The vehicle must not integrate a two-frame-old force as if it were current, which rings at the frame
///        rate; it integrates against the secant stiffness k = ForceY / deflection the load implies, semi-
///        implicitly: v′ = (v + Δt(−g + (F₀ + k(y₀ − y))/m)) / (1 + kΔt²/m), with F₀ and y₀ the load and the hub
///        height it was measured at. That is a one-line implicit spring whose stiffness the solver refreshes every
///        frame, and it is what the proof's quarter car settles with — the load lags, the stiffness does not.
/// tag   nonallocating, nonthrowing
struct ContactLoad
{
    double         ForceX        = 0.0;   // [N]   - longitudinal; traction positive forward
    double         ForceY        = 0.0;   // [N]   - normal; the wheel load
    double         ForceZ        = 0.0;   // [N]   - lateral
    double         TorqueX       = 0.0;   // [N·m] - about the rim centre
    double         TorqueY       = 0.0;   // [N·m]
    double         TorqueZ       = 0.0;   // [N·m] - the spin axis; braking or driving reaction
    double         PatchLength   = 0.0;   // [m]   - extent of the contact along X; zero while airborne
    double         PatchWidth    = 0.0;   // [m]   - extent of the contact along Z
    double         Penetration   = 0.0;   // [m]   - deepest incursion before a substep's projection; solver health, not deflection
    std::uint32_t  ContactCount  = 0u;    // [-]   - particle-substeps in contact over the frame
};

/// 🧩 Every span the solver reads and writes, laid out exactly as the device holds it, for one or more tyres.
/// note  🔴 The host form steps these vectors in place and the device form uploads them once and steps its own
///        copies. The two are the same bytes at construction, which is what `ParityRunner`'s comparison of the two
///        forms after a frame is a comparison of.
/// tag   owning
struct TyreLattice
{
    std::vector<TyreParticleRecord>  Particles            = {};     // [-] - Rings × Segments per tyre
    std::vector<TyreMotionRecord>    Motions              = {};     // [-] - one per particle
    std::vector<TyreVelocityRecord>  Velocities           = {};     // [-] - one per particle
    std::vector<TyreRestRecord>      Rests                = {};     // [-] - one per particle
    std::vector<TyreGradientRecord>  Gradients            = {};     // [-] - one per particle
    std::vector<TyreCordRecord>      Cords                = {};     // [-] - colour-sorted
    std::vector<Real32>              CordMultipliers      = {};     // [-] - one per cord, this substep
    std::vector<TyreHubRecord>       Hubs                 = {};     // [-] - one per tyre
    std::vector<Signed32>            Accumulators         = {};     // [-] - TyreAccumulatorStride per tyre
    std::vector<TyreShapeRecord>     ShapeRows            = {};     // [-] - ShapeRowLimit per tyre; rest values and compliances
    std::vector<Signed32>            ShapeSums            = {};     // [-] - ShapeSumStride per tyre; the block's fixed-point sums
    std::vector<Real32>              ShapeMultipliers     = {};     // [-] - ShapeRowLimit per tyre; λ per row, this substep
    std::vector<Real32>              ShapeAdvances        = {};     // [-] - ShapeRowLimit per tyre; Δλ per row, this pass
    std::vector<Real32>              ShapeMatrix          = {};     // [-] - ShapeMatrixStride per tyre; the factor's scratch
    std::vector<CordColour>          Colours              = {};     // [-] - in dispatch order
    std::uint32_t                    Rings                = 0u;     // [-]
    std::uint32_t                    Segments             = 0u;     // [-]
    std::uint32_t                    TyreCount            = 0u;     // [-]
    double                           RestVolume           = 0.0;    // [m³] - the moulded carcass, before inflation
    double                           Spacing              = 0.0;    // [m]  - the shortest rest cord; the convergence yardstick

    std::uint32_t ParticleCount() const;
    std::uint32_t ParticlesPerTyre() const;
    std::uint32_t ShapeOrder() const;   // [-] - 1 + Rings + Segments, the live rows of the shape block
};

/// 🧩 Builds the lattice for a declared count of identical tyres, every rim at the origin, uninflated and at rest.
/// in    Declared   [-]  the tyre
/// in    TyreCount  [-]  how many; at least one
/// out   Result    [-]  carries the specification's refusal, and ContentUnsupported for a zero count
/// note  📐 The carcass profile is ρ(θ) = r_rim + (R − r_rim)·((sin θ − sin θ₀)/(1 − sin θ₀))^f, z(θ) = ½W cos θ
///        for θ from θ₀ to π − θ₀ with cos θ₀ = W_bead / W. The beads land on the rim radius, the crown on the
///        outer radius, and f below unity flattens the tread as a belt package does.
/// cost  🔴
/// tag   api, nonthrowing
Deliver<TyreLattice> ConstructTyreLattice(const TyreSpecification& Declared, std::uint32_t TyreCount);
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded);

/// 🧩 Places one whole tyre rigidly at a pose — every particle at its rest offset, at rest, with the hub there too.
/// in    TyreIndex  [-]  below the lattice's tyre count
/// out   Result    [-]  refuses with ContentUnsupported for an index outside the lattice
/// note  🔴 The construction builds every tyre about the origin. Declaring a distant hub for the first frame
///        would drag the beads there and leave the carcass behind, a metre of strain the cords resolve in a
///        burst; place the tyre first, then declare its motion frame by frame. Carried cord and inflation
///        multipliers are kept — they belong to the shape, not the place.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
Deliver<bool> PlaceTyre(TyreLattice& Lattice, std::uint32_t TyreIndex, const HubPose& Pose);

/// 🧩 Declares one tyre's gauge pressure — at construction the specification's, and afterwards whatever the caller wants.
/// in    GaugePressure  [Pa]  above ambient; zero is a flat tyre, and a ramp from zero over a second inflates one
/// note  📝 Rewrites the hub record's pressure and the volume row's target and compliance together, which are
///        the only three places the pressure lives.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
Deliver<bool> DeclareInflation(TyreLattice& Lattice, std::uint32_t TyreIndex, double GaugePressure);

/// 🧩 Declares one rim's motion for the coming frame — where it starts, where it ends, and how fast it moves.
/// in    TyreIndex  [-]  below the lattice's tyre count
/// out   Result    [-]  refuses with ContentUnsupported for an index outside the lattice
/// note  📝 The inflation, friction and ground scalars in the hub record are filled from the specification by
///        the lattice construction and left alone here; this writes the motion alone.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
Deliver<bool> DeclareHubMotion(TyreLattice& Lattice, std::uint32_t TyreIndex, const HubPose& Begin, const HubPose& End);

/// 🧩 Declares the ground plane height one tyre meets.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
Deliver<bool> DeclareGround(TyreLattice& Lattice, std::uint32_t TyreIndex, double GroundHeight);

/// 🧩 Decodes one tyre's accumulators into the load the vehicle reads.
/// in    Accumulated  [-]  TyreAccumulatorStride integers, as the device or the host form left them
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
ContactLoad DecodeContactLoad(const Signed32* Accumulated);

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE HOST FORM
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 How one frame is subdivided.
/// note  📐 Small steps (Macklin et al. 2019): many substeps of one sweep converge faster per projection than one
///        step of many sweeps, because the error of a substep is quadratic in Δt. Thirty-two substeps at sixty
///        hertz is 0.52 ms per substep; the note below on the mass scale says why that many.
/// tag   nonallocating, nonthrowing
struct TyreStepSpecification
{
    double         FrameSeconds   = 1.0 / 60.0;   // [s] - the frame
    std::uint32_t  SubstepCount   = 32u;          // [-] - substeps per frame; 1920 Hz at sixty frames
    std::uint32_t  IterationCount = 1u;           // [-] - projection sweeps per substep
    double         Tolerance      = 2.0e-3;       // [m] - a final sweep moving nothing further than this has converged
};

// 📐 🔴 WHY THIRTY-TWO SUBSTEPS AND A MASS SCALE OF THREE. Every projection in a sweep moves a particle by
//    Δt² w F for the force F it is out of balance by, and the shape block, the cords and the contact are swept
//    one after another, so between any two of them a particle sloshes by Δt² F / m before the next one answers.
//    A carcass particle carries roughly a hundred newtons of pressure against a hundred newtons of cord tension;
//    at nine grams and 480 Hz that slosh is seven centimetres, which is five lattice spacings, and the sweep
//    never converges — the tyre balloons by fifteen percent. The slosh does not depend on the lattice
//    resolution, only on Δt² p / σ for the areal density σ, so the only two knobs are the step and the mass.
//    At 1920 Hz and three times the real mass the slosh is half a millimetre, a thirtieth of the spacing, the
//    residual settles there and the inflated radius is within a sixth of a percent of the moulded one; at
//    2880 Hz the real mass does the same. Three times the mass puts the belt's first mode at about a third of
//    its real frequency, which is still above anything a sixty-hertz frame can show, and leaves the hub load —
//    which is what the vehicle feels — within a percent of the quasi-static value in the proofs.

/// 🧩 Steps every tyre of the lattice through one frame on the host — the reference the device form is compared to.
/// in    Stepping  [-]  the subdivision
/// out   Result   [-]  the largest correction the final sweep applied, in metres, against the declared tolerance;
///                     the approximation is the lead tyre's load
/// note  🔴 Every arithmetic step is the shared projection in the shared order: predict, cords by colour, shape
///        gather, shape solve, shape advance and contact, velocity. The fixed-point sums are taken through the
///        same encoding the device uses, so the block both forms factor is the same integers — which is what
///        makes the parity bound attainable rather than aspirational.
/// note  ⏱️ This is the bottleneck the brief named. At sixteen by sixty-four and thirty-two substeps it is
///        roughly a hundred and twenty thousand cord projections, thirty thousand gathers and thirty-two
///        eighty-one-row factorisations per frame per tyre, ten milliseconds on one thread; the device form runs
///        the same count in thirteen dispatches per substep of a few microseconds each, for every tyre at once.
/// cost  🔴
/// tag   api, nonthrowing
ConvergentResult<ContactLoad> SolveTyreFrame(TyreLattice& Lattice, const TyreStepSpecification& Stepping);
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Convergent, PrecisionGuarantee::Bounded);

}   // namespace Slate
