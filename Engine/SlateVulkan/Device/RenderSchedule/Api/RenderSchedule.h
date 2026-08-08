//============================================================================================================================================
//                                                             RENDERSCHEDULE.H
//============================================================================================================================================
// 🧩 What is recorded in a rotation slot, in what order, and against which shared targets.

#pragma once

#include "Contract/IdentityContract.h"
#include "Contract/OutcomeContract.h"
#include "SlateVulkan/Device/VulkanExchange/Api/VulkanExchange.h"

#include <cstdint>
#include <vector>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    SHARED TARGETS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every shared target the schedule declares. Nothing invents a target another already produces.
/// note  Declared as one closed enumeration so that the producer and amender lists below are total.
/// tag   contract
enum class SharedTarget : std::uint32_t
{
    DepthSurface           = 0u,   // [-] - D32, display extent
    VisibilityIndex        = 1u,   // [-] - R32G32 unsigned integer, display extent
    OccupancySurface       = 2u,   // [-] - R8, display extent
    MotionSurface          = 3u,   // [-] - R16G16 real, display extent
    OcclusionSurface       = 4u,   // [-] - R8, half display extent
    DirectOcclusionSurface = 5u,   // [-] - RGBA8; four illuminants, per DirectOcclusionCapacity
    TransmissionIndex      = 6u,   // [-] - R32G32 unsigned integer × TransmissionDepth, display extent
    RadianceSurface        = 7u,   // [-] - RGBA16 real, display extent
    ReflectionSurface      = 8u,   // [-] - RGBA16 real, half display extent
    AccumulationSurface    = 9u,   // [-] - RGBA16 real, display extent
    DisplaySurface         = 10u,  // [-] - display format and extent
    OutlineSurface         = 11u,  // [-] - R8, display extent
    TransmittanceSurface   = 12u,  // [-] - RGBA16 real, 256 × 64 — resident, 128 KiB
    MultiScatterSurface    = 13u,  // [-] - RGBA16 real, 32 × 32 — resident, 8 KiB
    SkyViewSurface         = 14u,  // [-] - RGBA16 real, 192 × 108 — resident, 162 KiB
    TargetCount            = 15u   // [-] - the closed count, never a target
};

/// 🧩 How a target's extent relates to the display extent.
/// note  ⚠️ A display-relative target is reclaimed and re-claimed on every extent change; an absolute one
///       is never touched by a resize. `06` §4.1 gates that both ways.
/// tag   contract
enum class ExtentRelation : std::uint32_t
{
    DisplayRelative  = 0u,   // [-] - exactly the display extent
    FractionOfDisplay = 1u,  // [-] - a declared fraction of it
    Absolute         = 2u    // [-] - a fixed extent, independent of the display
};

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECORDINGS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What a recording contributes. Authored once by the contributing document, consulted by the orderer.
/// tag   contract
enum class RecordingCommand : std::uint32_t
{
    GraphicsRecording = 0u,   // [-] - a graphics recording
    ComputeDispatch   = 1u    // [-] - a compute dispatch
};

/// 🧩 One declared recording.
/// note  🔴 A recording with a capability requirement and no substitution is rejected at bring-up. An
///       absent capability must degrade to something, and choosing that something belongs to the
///       contributing document rather than to a branch invented at the recording site.
/// tag   owning
struct DeclaredRecording
{
    const char*                Identity            = "";                              // [-] - unique; used in metrics
    std::vector<SharedTarget>  Reads               = {};                              // [-] - consumed targets
    std::vector<SharedTarget>  Produces            = {};                              // [-] - targets written whole
    std::vector<SharedTarget>  Amends              = {};                              // [-] - targets modified in place
    RecordingCommand           Command             = RecordingCommand::GraphicsRecording;
    bool                       CapabilityRequired  = false;                           // [-] - needs a scored capability
    const char*                Substitution        = "";                              // [-] - what runs instead
    bool                       DisplayReferred     = false;                           // [-] - recorded after the tone line
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SCHEDULE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The ordered recordings of one rotation, fixed at bring-up and merely executed per rotation.
/// note  🔴 The term is `RenderSchedule`. "Frame graph" is not a synonym: `Frame` is banned and "graph"
///       implies a solved dependency structure Slate does not build. The recordings and the target set are
///       both known at bring-up, so the ordering is fixed there too.
/// tag   owning
class RenderSchedule
{
public:

    /// 🧩 Contributes one recording to the schedule.
    /// in    Arriving [-]  the declaration the contributing document authored
    /// out   Outcome  [-]  refuses when a capability is required with no substitution, or when the
    ///                     contribution produces a target another recording already produces
    /// cost  ✔️
    /// tag   api, nonthrowing
    Outcome<bool> Contribute(const DeclaredRecording& Arriving);

    /// 🧩 Fixes the ordering. Derived from the declared reads and writes, never hand-written.
    /// out   Outcome  [-]  refuses when a target is read by a recording ordered before its producer, or
    ///                     when anything scene-referred is ordered after the display-referred line
    /// post  the ordering is immutable until the next bring-up
    /// cost  🚩
    /// tag   api, nonthrowing
    Outcome<bool> Fix();

    /// 🧩 The recordings, in the order Fix derived.
    /// pre   Fix delivered
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    const std::vector<DeclaredRecording>& Ordered() const;

private:

    std::vector<DeclaredRecording>  ContributedOrder;                            // [-] - as contributed
    std::vector<DeclaredRecording>  OrderedRecordings;                           // [-] - as Fix derived
    RecordingIdentity               ProducerOf[static_cast<std::size_t>(SharedTarget::TargetCount)] = {};
    bool                            OrderingFixed = false;                       // [-] - Fix has delivered
};

}   // namespace Slate
