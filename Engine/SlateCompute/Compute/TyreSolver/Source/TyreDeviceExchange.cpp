//============================================================================================================================================
//                                                        TYREDEVICEEXCHANGE.CPP
//============================================================================================================================================
// 🧩 Fourteen spans, six programs, one descriptor set — and a frame that is some four hundred dispatches long.

#include "SlateCompute/Compute/TyreSolver/Api/TyreDeviceExchange.h"

#include <cstring>

namespace Slate
{

namespace
{

/// 🧩 How many bytes one host vector occupies, for the span claims and the staging transfers.
template <typename Record>
VkDeviceSize BytesOf(const std::vector<Record>& Held)
{
    return static_cast<VkDeviceSize>(Held.size() * sizeof(Record));
}

/// 🧩 Workgroups for a count of invocations at a group extent.
std::uint32_t GroupsOf(std::uint32_t Invocations, std::uint32_t PerGroup)
{
    return (Invocations + PerGroup - 1u) / PerGroup;
}

const char* const ProgramStems[] =
{
    "TyrePrediction", "TyreCordProjection", "TyreShapeGather", "TyreShapeSolve", "TyreContactProjection", "TyreVelocityRecovery"
};

}   // namespace

TyreDeviceExchange::~TyreDeviceExchange()
{
    Reclaim();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Deliver<bool> TyreDeviceExchange::ConstructTyreDeviceExchange(const VulkanExchange&      Exchange,
                                                              const DiagnosticExtension& Naming,
                                                              const char*                ShaderLocation,
                                                              const TyreLattice&         Lattice)
{
    if (Constructed)
        return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "the tyre device exchange already stands" });

    if (ShaderLocation == nullptr || ShaderLocation[0] == '\0')
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "no tyre shader stream location was supplied" });

    if (Lattice.TyreCount == 0u || Lattice.Particles.empty() || Lattice.Cords.empty())
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "the tyre lattice is not constructed" });

    DeviceEdge = &Exchange;
    TyreCount  = Lattice.TyreCount;

    if (!Bytes.ReserveByteSpace(Exchange, Naming).Resolved ||
        !Spans.ConstructSpanSpace(Exchange, Bytes, Naming).Resolved ||
        !Streams.AttachShaderStreams(Exchange, ShaderLocation).Resolved ||
        !Descriptors.ConstructDescriptorIndex(Exchange, Naming).Resolved ||
        !Programs.ConstructProgramIndex(Exchange, Streams, Descriptors, Naming).Resolved)
    {
        Reclaim();
        return Deliver<bool>::Refuse({ RefusalReason::CapabilityAbsent, "the tyre device estate was rejected" });
    }

    // 📝 The fourteen spans in binding order — the order `TyreDeclarations.slang` spells, and the order Admit
    //    stages them in. Every one is written by some entry point except the rests, the cords, the hubs and the
    //    shape rows, and the accumulators alone are read back.
    LatticeBytes =
    {
        BytesOf(Lattice.Particles),        BytesOf(Lattice.Motions),          BytesOf(Lattice.Velocities),
        BytesOf(Lattice.Rests),            BytesOf(Lattice.Gradients),        BytesOf(Lattice.Cords),
        BytesOf(Lattice.CordMultipliers),  BytesOf(Lattice.Hubs),             BytesOf(Lattice.Accumulators),
        BytesOf(Lattice.ShapeRows),        BytesOf(Lattice.ShapeSums),        BytesOf(Lattice.ShapeMultipliers),
        BytesOf(Lattice.ShapeAdvances),    BytesOf(Lattice.ShapeMatrix)
    };

    VkDeviceSize StagingBytes = 0u;

    for (std::uint32_t Slot = 0u; Slot < SpanCount; ++Slot)
    {
        SpanShape Shape;
        Shape.SpanBytes = LatticeBytes[Slot];
        Shape.Intent    = SpanIntent::StorageWritten;
        Shape.Residency = ExtentResidency::DeviceLocal;

        const Deliver<SpanReservation> Reserved = Spans.Reserve(Shape);

        if (!Reserved.Resolved)
        {
            Reclaim();
            return Deliver<bool>::Refuse({ RefusalReason::ExtentExhausted, "a tyre lattice span was rejected" });
        }

        LatticeSpans[Slot] = Reserved.Resolve().SpanIndex;
        StagingBytes      += LatticeBytes[Slot];
    }

    {
        SpanShape Shape;
        Shape.SpanBytes = StagingBytes;
        Shape.Intent    = SpanIntent::TransferSource;
        Shape.Residency = ExtentResidency::HostWritable;

        const Deliver<SpanReservation> Reserved = Spans.Reserve(Shape);

        if (!Reserved.Resolved)
        {
            Reclaim();
            return Deliver<bool>::Refuse({ RefusalReason::ExtentExhausted, "the tyre admission staging was rejected" });
        }

        AdmissionStaging = Reserved.Resolve().SpanIndex;
    }

    for (std::uint32_t Slot = 0u; Slot < RecordingSlotCount; ++Slot)
    {
        SpanShape HubShape;
        HubShape.SpanBytes = BytesOf(Lattice.Hubs);
        HubShape.Intent    = SpanIntent::TransferSource;
        HubShape.Residency = ExtentResidency::HostWritable;

        // 📝 Host-writable and reached as a transfer target alone: the device copies the accumulators into it at
        //    the frame's end and the host reads the coherent bytes two frames later. No shader touches it.
        SpanShape LoadShape;
        LoadShape.SpanBytes = BytesOf(Lattice.Accumulators);
        LoadShape.Intent    = SpanIntent::StorageRead;
        LoadShape.Residency = ExtentResidency::HostWritable;

        const Deliver<SpanReservation> Hubs  = Spans.Reserve(HubShape);
        const Deliver<SpanReservation> Loads = Spans.Reserve(LoadShape);

        if (!Hubs.Resolved || !Loads.Resolved)
        {
            Reclaim();
            return Deliver<bool>::Refuse({ RefusalReason::ExtentExhausted, "a tyre per-slot span was rejected" });
        }

        HubStaging[Slot]   = Hubs.Resolve().SpanIndex;
        LoadReadback[Slot] = Loads.Resolve().SpanIndex;
        SlotRecorded[Slot] = false;
    }

    std::vector<DescriptorSlot> Declared(SpanCount);

    for (std::uint32_t Slot = 0u; Slot < SpanCount; ++Slot)
    {
        Declared[Slot].SlotIndex      = Slot;
        Declared[Slot].Carried        = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        Declared[Slot].ReachingStages = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    const Deliver<std::uint32_t> Layout = Descriptors.Declare(Declared);

    if (!Layout.Resolved)
    {
        Reclaim();
        return Deliver<bool>::Refuse({ RefusalReason::CapabilityAbsent, "the tyre descriptor layout was rejected" });
    }

    LayoutIndex = Layout.Resolve();

    for (std::uint32_t Program = 0u; Program < static_cast<std::uint32_t>(TyreProgram::Count); ++Program)
    {
        const Deliver<std::uint32_t> Module = Streams.Resolve("SlateCompute", ProgramStems[Program]);

        if (!Module.Resolved)
        {
            Reclaim();
            return Deliver<bool>::Refuse({ RefusalReason::CapabilityAbsent, "a tyre shader stream is absent" });
        }

        ComputeDeclaration Declaring;
        Declaring.ModuleIndex   = Module.Resolve();
        Declaring.LayoutIndexs  = { LayoutIndex };
        Declaring.ConstantBytes = static_cast<std::uint32_t>(sizeof(TyreStepPush));

        const Deliver<std::uint32_t> Constructing = Programs.DeclareCompute(Declaring);

        if (!Constructing.Resolved)
        {
            Reclaim();
            return Deliver<bool>::Refuse({ RefusalReason::CapabilityAbsent, "a tyre compute program was rejected" });
        }

        ProgramIndexs[Program] = Constructing.Resolve();
    }

    // 📝 One set, amended identically for every cycle slot at construction. The spans never move, so nothing is
    //    amended per frame; the slots exist because the index allocates per slot, not because the content differs.
    const Deliver<std::uint32_t> Reserved = Descriptors.Fix(1u).Resolved ? Descriptors.Reserve(LayoutIndex)
                                                                         : Deliver<std::uint32_t>::Refuse(
                                                                               { RefusalReason::CapabilityAbsent,
                                                                                 "the tyre descriptor pool was rejected" });

    if (!Reserved.Resolved)
    {
        Reclaim();
        return Deliver<bool>::Refuse(Reserved.Error);
    }

    Reservation = Reserved.Resolve();

    std::vector<DescriptorContent> Contents(SpanCount);

    for (std::uint32_t Slot = 0u; Slot < SpanCount; ++Slot)
    {
        const Deliver<SpanReservation> Current = Spans.Current(LatticeSpans[Slot]);

        if (!Current.Resolved)
        {
            Reclaim();
            return Deliver<bool>::Refuse(Current.Error);
        }

        Contents[Slot].SlotIndex  = Slot;
        Contents[Slot].SpanExtent = Current.Resolve().Extent;
        Contents[Slot].SpanOffset = 0u;
        Contents[Slot].SpanBytes  = LatticeBytes[Slot];
    }

    for (std::uint32_t Slot = 0u; Slot < RecordingSlotCount; ++Slot)
    {
        if (!Descriptors.Amend(Reservation, Slot, Contents).Resolved)
        {
            Reclaim();
            return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "the tyre descriptors were rejected" });
        }
    }

    Constructed = true;
    return Deliver<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       ADMISSION
//------------------------------------------------------------------------------------------------------------------------

Deliver<bool> TyreDeviceExchange::Admit(const TyreLattice& Lattice, VkCommandBuffer Recorded)
{
    if (!Constructed || Recorded == VK_NULL_HANDLE)
        return Deliver<bool>::Refuse({ RefusalReason::CapabilityAbsent, "the tyre device exchange does not stand" });

    if (Lattice.TyreCount != TyreCount)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "the lattice is not the one the estate was sized for" });

    const void* const Sources[SpanCount] =
    {
        Lattice.Particles.data(),       Lattice.Motions.data(),    Lattice.Velocities.data(),       Lattice.Rests.data(),
        Lattice.Gradients.data(),       Lattice.Cords.data(),      Lattice.CordMultipliers.data(),  Lattice.Hubs.data(),
        Lattice.Accumulators.data(),    Lattice.ShapeRows.data(),  Lattice.ShapeSums.data(),        Lattice.ShapeMultipliers.data(),
        Lattice.ShapeAdvances.data(),   Lattice.ShapeMatrix.data()
    };

    const VkDeviceSize Sizes[SpanCount] =
    {
        BytesOf(Lattice.Particles),        BytesOf(Lattice.Motions),          BytesOf(Lattice.Velocities),
        BytesOf(Lattice.Rests),            BytesOf(Lattice.Gradients),        BytesOf(Lattice.Cords),
        BytesOf(Lattice.CordMultipliers),  BytesOf(Lattice.Hubs),             BytesOf(Lattice.Accumulators),
        BytesOf(Lattice.ShapeRows),        BytesOf(Lattice.ShapeSums),        BytesOf(Lattice.ShapeMultipliers),
        BytesOf(Lattice.ShapeAdvances),    BytesOf(Lattice.ShapeMatrix)
    };

    const Deliver<SpanReservation> Staging = Spans.Current(AdmissionStaging);

    if (!Staging.Resolved)
        return Deliver<bool>::Refuse(Staging.Error);

    VkDeviceSize Offset = 0u;

    for (std::uint32_t Slot = 0u; Slot < SpanCount; ++Slot)
    {
        if (Sizes[Slot] != LatticeBytes[Slot])
            return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "the lattice has changed shape since construction" });

        if (!Spans.Amend(AdmissionStaging, Sources[Slot], Sizes[Slot], Offset).Resolved)
            return Deliver<bool>::Refuse({ RefusalReason::ExtentExhausted, "the tyre admission staging write was refused" });

        const Deliver<SpanReservation> Target = Spans.Current(LatticeSpans[Slot]);

        if (!Target.Resolved)
            return Deliver<bool>::Refuse(Target.Error);

        VkBufferCopy Carrying = {};
        Carrying.srcOffset = Offset;
        Carrying.dstOffset = 0u;
        Carrying.size      = Sizes[Slot];

        vkCmdCopyBuffer(Recorded, Staging.Resolve().Extent, Target.Resolve().Extent, 1u, &Carrying);

        Offset += Sizes[Slot];
    }

    Order(Recorded, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

    Admitted = true;
    return Deliver<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE FRAME
//------------------------------------------------------------------------------------------------------------------------

void TyreDeviceExchange::Order(VkCommandBuffer Recorded, VkPipelineStageFlags ReadStages, VkAccessFlags ReadAccess)
{
    // 📝 A global memory barrier, for `OcclusionScheduler`'s reason: every ordering here is "everything the last
    //    dispatch or transfer wrote, before everything the next one reads", and fourteen buffer barriers would say
    //    the same thing with fourteen places to forget one.
    VkMemoryBarrier Ordered = {};
    Ordered.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    Ordered.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    Ordered.dstAccessMask = ReadAccess;

    vkCmdPipelineBarrier(Recorded, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, ReadStages,
                         0u, 1u, &Ordered, 0u, nullptr, 0u, nullptr);
}

Deliver<bool> TyreDeviceExchange::Dispatch(VkCommandBuffer     Recorded,
                                           std::uint32_t       SlotIndex,
                                           TyreProgram         Program,
                                           const TyreStepPush& Push,
                                           std::uint32_t       Invocations,
                                           std::uint32_t       PerGroup)
{
    const Deliver<ConstructedProgram> Resolved = Programs.Resolve(ProgramIndexs[static_cast<std::size_t>(Program)]);
    const Deliver<VkDescriptorSet>    Reached  = Descriptors.Resolve(Reservation, SlotIndex);

    if (!Resolved.Resolved)
        return Deliver<bool>::Refuse(Resolved.Error);

    if (!Reached.Resolved)
        return Deliver<bool>::Refuse(Reached.Error);

    const VkDescriptorSet Set = Reached.Resolve();

    vkCmdBindPipeline(Recorded, VK_PIPELINE_BIND_POINT_COMPUTE, Resolved.Resolve().Constructed);
    vkCmdBindDescriptorSets(Recorded, VK_PIPELINE_BIND_POINT_COMPUTE, Resolved.Resolve().ReachedLayout, 0u, 1u, &Set, 0u, nullptr);
    vkCmdPushConstants(Recorded, Resolved.Resolve().ReachedLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0u,
                       static_cast<std::uint32_t>(sizeof(TyreStepPush)), &Push);
    vkCmdDispatch(Recorded, GroupsOf(Invocations, PerGroup), 1u, 1u);

    Order(Recorded, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

    return Deliver<bool>::Result(true);
}

Deliver<bool> TyreDeviceExchange::Record(const TyreLattice&           Lattice,
                                         const TyreStepSpecification& Stepping,
                                         VkCommandBuffer              Recorded,
                                         std::uint32_t                SlotIndex)
{
    if (!Constructed || !Admitted || Recorded == VK_NULL_HANDLE || SlotIndex >= RecordingSlotCount)
        return Deliver<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no tyre frame can be recorded into that slot" });

    if (Lattice.TyreCount != TyreCount || Stepping.SubstepCount == 0u || Stepping.IterationCount == 0u || Stepping.FrameSeconds <= 0.0)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "the tyre step specification is unusable" });

    // ① The hubs cross: the vehicle's declared rim motion for this frame, staged in this slot's host span.
    {
        const Deliver<SpanReservation> Staged = Spans.Current(HubStaging[SlotIndex]);
        const Deliver<SpanReservation> Target = Spans.Current(LatticeSpans[7u]);

        if (!Staged.Resolved || !Target.Resolved ||
            !Spans.Amend(HubStaging[SlotIndex], Lattice.Hubs.data(), BytesOf(Lattice.Hubs), 0u).Resolved)
        {
            return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "the tyre hub records could not be staged" });
        }

        VkBufferCopy Carrying = {};
        Carrying.size = BytesOf(Lattice.Hubs);

        vkCmdCopyBuffer(Recorded, Staged.Resolve().Extent, Target.Resolve().Extent, 1u, &Carrying);
        Order(Recorded, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
    }

    // ② The substeps, in the host form's exact order; the prediction of the first substep clears the accumulators.
    TyreStepPush Push = {};
    Push.ParticleCount  = Lattice.ParticleCount();
    Push.Rings          = Lattice.Rings;
    Push.Segments       = Lattice.Segments;
    Push.TyreCount      = Lattice.TyreCount;
    Push.SubstepSeconds = static_cast<Real32>(Stepping.FrameSeconds / static_cast<double>(Stepping.SubstepCount));
    Push.SubstepCount   = Stepping.SubstepCount;

    const std::uint32_t Particles = Lattice.ParticleCount();

    for (std::uint32_t Substep = 0u; Substep < Stepping.SubstepCount; ++Substep)
    {
        Push.SubstepIndex = Substep;

        Deliver<bool> Recording = Dispatch(Recorded, SlotIndex, TyreProgram::Prediction, Push, Particles, TyreGroupExtent);

        if (!Recording.Resolved)
            return Recording;

        for (std::uint32_t Iteration = 0u; Iteration < Stepping.IterationCount; ++Iteration)
        {
            Push.IterationIndex = Iteration;
            Push.PassIndex      = Substep * Stepping.IterationCount + Iteration;

            for (const CordColour& Colour : Lattice.Colours)
            {
                Push.CordOffset = Colour.Offset;
                Push.CordCount  = Colour.Count;

                Recording = Dispatch(Recorded, SlotIndex, TyreProgram::Cord, Push, Colour.Count, TyreGroupExtent);

                if (!Recording.Resolved)
                    return Recording;
            }

            // 📝 The solve is one workgroup per tyre — the group extent is the whole dispatch per tyre — so its
            //    invocation count is the tyre count times the extent.
            const std::uint32_t SolveInvocations = TyreCount * TyreGroupExtent;

            if (!(Recording = Dispatch(Recorded, SlotIndex, TyreProgram::Gather,  Push, Particles,        TyreGroupExtent)).Resolved ||
                !(Recording = Dispatch(Recorded, SlotIndex, TyreProgram::Solve,   Push, SolveInvocations, TyreGroupExtent)).Resolved ||
                !(Recording = Dispatch(Recorded, SlotIndex, TyreProgram::Contact, Push, Particles,        TyreGroupExtent)).Resolved)
            {
                return Recording;
            }
        }

        Recording = Dispatch(Recorded, SlotIndex, TyreProgram::Recovery, Push, Particles, TyreGroupExtent);

        if (!Recording.Resolved)
            return Recording;
    }

    // ③ The accumulators cross back — sixteen integers per tyre, into this slot's host-coherent span.
    {
        const Deliver<SpanReservation> Source = Spans.Current(LatticeSpans[8u]);
        const Deliver<SpanReservation> Target = Spans.Current(LoadReadback[SlotIndex]);

        if (!Source.Resolved || !Target.Resolved)
            return Deliver<bool>::Refuse({ RefusalReason::HostDenied, "the tyre load readback spans are unavailable" });

        Order(Recorded, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);

        VkBufferCopy Carrying = {};
        Carrying.size = LatticeBytes[8u];

        vkCmdCopyBuffer(Recorded, Source.Resolve().Extent, Target.Resolve().Extent, 1u, &Carrying);

        VkMemoryBarrier Visible = {};
        Visible.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        Visible.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        Visible.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

        vkCmdPipelineBarrier(Recorded, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                             0u, 1u, &Visible, 0u, nullptr, 0u, nullptr);
    }

    SlotRecorded[SlotIndex] = true;
    return Deliver<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE READBACK
//------------------------------------------------------------------------------------------------------------------------

Deliver<ContactLoad> TyreDeviceExchange::Load(std::uint32_t SlotIndex, std::uint32_t TyreIndex) const
{
    if (!Constructed || SlotIndex >= RecordingSlotCount || !SlotRecorded[SlotIndex])
        return Deliver<ContactLoad>::Refuse({ RefusalReason::ContentUnsupported, "no tyre frame has completed in that slot" });

    if (TyreIndex >= TyreCount)
        return Deliver<ContactLoad>::Refuse({ RefusalReason::ContentUnsupported, "no tyre stands at that index" });

    const Deliver<SpanReservation> Readback = Spans.Current(LoadReadback[SlotIndex]);

    if (!Readback.Resolved || Readback.Resolve().HostAddress == nullptr)
        return Deliver<ContactLoad>::Refuse({ RefusalReason::CapabilityAbsent, "the tyre load readback span is not host-visible" });

    // 📝 Copied out before decoding rather than decoded in place: the span is host-coherent but the device may
    //    be writing the *other* slot at this moment, and a plain copy keeps the read to the slot that is ours.
    Signed32 Accumulated[TyreAccumulatorStride] = {};

    std::memcpy(Accumulated,
                static_cast<const unsigned char*>(Readback.Resolve().HostAddress) + TyreIndex * TyreAccumulatorStride * sizeof(Signed32),
                sizeof(Accumulated));

    return Deliver<ContactLoad>::Result(DecodeContactLoad(Accumulated));
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       LIFETIME
//------------------------------------------------------------------------------------------------------------------------

void TyreDeviceExchange::Reclaim()
{
    Programs.Reclaim();
    Descriptors.Reclaim();
    Streams.Reclaim();
    Spans.Reclaim();
    Bytes.Reclaim();

    LatticeSpans.fill(AbsentSpan);
    HubStaging.fill(AbsentSpan);
    LoadReadback.fill(AbsentSpan);
    SlotRecorded.fill(false);
    ProgramIndexs.fill(AbsentProgram);

    AdmissionStaging = AbsentSpan;
    LayoutIndex      = AbsentDescriptor;
    Reservation      = AbsentDescriptor;
    TyreCount        = 0u;
    Admitted         = false;
    DeviceEdge       = nullptr;
    Constructed      = false;
}

bool TyreDeviceExchange::Standing() const
{
    return Constructed;
}

}   // namespace Slate
