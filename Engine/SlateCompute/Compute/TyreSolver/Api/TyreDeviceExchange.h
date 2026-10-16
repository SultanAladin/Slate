//============================================================================================================================================
//                                                         TYREDEVICEEXCHANGE.H
//============================================================================================================================================
// 🧩 Device residency, the per-frame recording and the latency-slotted load readback for the tyre solver.

#pragma once

#include "Foundation/DeliveryGuarantee.h"
#include "Foundation/NumericTolerance.h"
#include "SlateCompute/Compute/TyreSolver/Api/TyreSolver.h"
#include "SlateVulkan/Device/ByteSpace/Api/ByteSpace.h"
#include "SlateVulkan/Device/DescriptorIndex/Api/DescriptorIndex.h"
#include "SlateVulkan/Device/ProgramIndex/Api/ProgramIndex.h"
#include "SlateVulkan/Device/ShaderCodec/Api/ShaderCodec.h"
#include "SlateVulkan/Device/SpanSpace/Api/SpanSpace.h"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE DEVICE EXCHANGE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The tyre lattice made device-resident, stepped there every frame, and its hub loads read back two frames late.
/// note  🔴 There is no host path through this component. The lattice crosses to the device once at admission and
///        the only bytes that cross back are sixteen integers per tyre; the positions never return. A caller that
///        wants the carcass shape for rendering reads the particle span from a draw, not from the host.
/// note  🔴 The load is read with a latency of `RecordingSlotCount` frames, for `RequestQueue`'s reason: the slot
///        being recorded is never the slot being read, so the readback never waits on the device. The vehicle
///        receives the load the carcass carried at the hub pose it was given two frames ago, and the pose it was
///        measured at is part of the answer — `TyreSolver.h` says how the vehicle extrapolates from there.
/// tag   owning
class TyreDeviceExchange
{
public:

    TyreDeviceExchange()                                     = default;
    TyreDeviceExchange(const TyreDeviceExchange&)            = delete;
    TyreDeviceExchange& operator=(const TyreDeviceExchange&) = delete;
    ~TyreDeviceExchange();

    /// 🧩 Claims every span the lattice needs, constructs the six programs and fixes the one descriptor set.
    /// in    Exchange        [-]  the active Vulkan device
    /// in    Naming          [-]  the diagnostic naming extension
    /// in    ShaderLocation  [-]  the folder containing lowered shader streams
    /// in    Lattice         [-]  the constructed lattice; sizes every claim, and its content is uploaded by Admit
    /// out   Result          [-]  refuses at the first rejected device claim and reclaims prior claims
    /// cost  🔴
    /// tag   api, nonthrowing
    Deliver<bool> ConstructTyreDeviceExchange(const VulkanExchange&      Exchange,
                                              const DiagnosticExtension& Naming,
                                              const char*                ShaderLocation,
                                              const TyreLattice&         Lattice);

    /// 🧩 Records the transfer that carries the whole lattice to the device — once, and again after PlaceTyre.
    /// in    Lattice   [-]  the lattice as the host holds it now; every span is rewritten from it
    /// in    Recorded  [-]  open recording; the staging bytes are read when it executes, so they are not reused
    ///                      until the recording completes
    /// note  📝 Everything crosses, the multipliers and scratch included, so the device begins the frame exactly
    ///        where the host form would — which is what the parity proof compares.
    /// cost  🚩
    /// tag   api, nonthrowing
    Deliver<bool> Admit(const TyreLattice& Lattice, VkCommandBuffer Recorded);

    /// 🧩 Records one frame: the hubs cross, every substep is dispatched, and the accumulators are copied for readback.
    /// in    Lattice    [-]  the host lattice; only its hub records and dimensions are read
    /// in    Stepping   [-]  the subdivision; the same structure the host form takes
    /// in    Recorded   [-]  the open recording for this cycle slot
    /// in    SlotIndex  [-]  the completion-gated recording slot this command belongs to
    /// out   Result     [-]  refuses before the estate stands or Admit has been recorded
    /// note  ⏱️ Thirteen dispatches per substep at the default lattice — prediction, eight cord colours, gather, solve,
    ///        contact, recovery — so about four hundred per frame at thirty-two substeps, each a few
    ///        microseconds of work on a handful of groups and a global memory barrier between them. The barrier
    ///        cost dominates; it is the price of a sequential sweep and the reason the frame belongs on an async
    ///        compute queue alongside the frame's rendering rather than in front of it.
    /// cost  🔴
    /// tag   api, nonthrowing
    Deliver<bool> Record(const TyreLattice&           Lattice,
                         const TyreStepSpecification& Stepping,
                         VkCommandBuffer              Recorded,
                         std::uint32_t                SlotIndex);

    /// 🧩 The load one tyre carried in the frame recorded into this slot, once that recording has completed.
    /// in    SlotIndex  [-]  the slot whose recording the host has observed complete — the one about to be reused
    /// in    TyreIndex  [-]  below the lattice's tyre count
    /// out   Result     [-]  refuses for a slot nothing was recorded into yet
    /// pre   the host has waited on the slot's completion; the span is host-coherent and nothing is flushed here
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    Deliver<ContactLoad> Load(std::uint32_t SlotIndex, std::uint32_t TyreIndex) const;

    /// 🧩 Releases every device claim while the device is still alive.
    /// pre   the device is idle
    void Reclaim();

    bool Standing() const;

private:

    /// 🧩 The programs in dispatch order, one per entry point.
    enum class TyreProgram : std::uint32_t
    {
        Prediction = 0u,
        Cord       = 1u,
        Gather     = 2u,
        Solve      = 3u,
        Contact    = 4u,
        Recovery   = 5u,
        Count      = 6u
    };

    static constexpr std::uint32_t SpanCount = 14u;   // [-] - the descriptor set's slots, in binding order

    /// 🧩 Records the global ordering between one dispatch's writes and the next's reads.
    static void Order(VkCommandBuffer Recorded, VkPipelineStageFlags ReadStages, VkAccessFlags ReadAccess);

    /// 🧩 Binds one program with the slot's descriptor set, pushes the step and dispatches a count of invocations.
    Deliver<bool> Dispatch(VkCommandBuffer Recorded, std::uint32_t SlotIndex, TyreProgram Program,
                           const TyreStepPush& Push, std::uint32_t Invocations, std::uint32_t PerGroup);

    const VulkanExchange*  DeviceEdge   = nullptr;   // [-] - borrowed device; outlives every owned claim
    ByteSpace              Bytes        = {};        // [-] - backing extents for every span
    SpanSpace              Spans        = {};        // [-] - the lattice, staging and readback spans
    ShaderCodec            Streams      = {};        // [-] - the six lowered entry points
    DescriptorIndex        Descriptors  = {};        // [-] - the one layout and its per-slot sets
    ProgramIndex           Programs     = {};        // [-] - the six compute programs

    std::array<std::uint32_t, SpanCount>                      LatticeSpans  = {};   // [-] - device-local, binding order
    std::array<VkDeviceSize,  SpanCount>                      LatticeBytes  = {};   // [B] - how far each runs
    std::array<std::uint32_t, RecordingSlotCount>             HubStaging    = {};   // [-] - host-written hubs per slot
    std::array<std::uint32_t, RecordingSlotCount>             LoadReadback  = {};   // [-] - accumulators per slot
    std::array<bool,          RecordingSlotCount>             SlotRecorded  = {};   // [-] - a frame has been recorded
    std::array<std::uint32_t, static_cast<std::size_t>(TyreProgram::Count)> ProgramIndexs = {};   // [-]

    std::uint32_t  AdmissionStaging = AbsentSpan;         // [-] - the whole lattice, host-written
    std::uint32_t  LayoutIndex      = AbsentDescriptor;   // [-] - the one declared layout
    std::uint32_t  Reservation      = AbsentDescriptor;   // [-] - its per-slot sets
    std::uint32_t  TyreCount        = 0u;                 // [-] - as admitted
    bool           Admitted         = false;              // [-] - Admit has been recorded at least once
    bool           Constructed      = false;              // [-] - every dependency stands
};

}   // namespace Slate
