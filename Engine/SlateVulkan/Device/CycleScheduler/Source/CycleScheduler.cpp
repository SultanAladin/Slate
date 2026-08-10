//============================================================================================================================================
//                                                            CYCLESCHEDULER.CPP
//============================================================================================================================================
// 🧩 The ordering points of every cyclic slot, the bounded wait that reclaims one, and the advance that cycles them.

#include "SlateVulkan/Device/CycleScheduler/Api/CycleScheduler.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> CycleScheduler::Construct(const VulkanExchange& Exchange)
{
    if (Exchange.ActiveDevice() == VK_NULL_HANDLE)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no device is active" });

    DeviceEdge = &Exchange;

    const VkDevice Active = Exchange.ActiveDevice();

    VkFenceCreateInfo CompletionDeclaration = {};
    CompletionDeclaration.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    // 🔴 Signalled at construction. The first rotation waits before it has ever submitted, and an unsignalled
    //    completion makes that wait one for a submission that was never made.
    CompletionDeclaration.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo OrderingDeclaration = {};
    OrderingDeclaration.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    Slots.assign(RecordingRotationDepth, RotationSlot{});

    for (RotationSlot& Slot : Slots)
    {
        const bool Constructed =
            vkCreateFence(Active, &CompletionDeclaration, nullptr, &Slot.Completion)        == VK_SUCCESS &&
            vkCreateSemaphore(Active, &OrderingDeclaration, nullptr, &Slot.ImageArrived)    == VK_SUCCESS &&
            vkCreateSemaphore(Active, &OrderingDeclaration, nullptr, &Slot.RecordingDone)   == VK_SUCCESS;

        // 📝 🔴 Refused in full. A rotation half-constructed leaves some slots orderable and some not, and the
        //    defect surfaces on whichever rotation first reaches the unordered slot rather than at bring-up.
        if (!Constructed)
        {
            Reclaim();
            return Outcome<bool>::Refuse(
                { RefusalReason::ExtentExhausted, "the device declined an ordering point of the rotation" });
        }
    }

    SlotStanding = 0u;
    Rotations    = 0u;

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE WAIT
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> CycleScheduler::Await()
{
    if (DeviceEdge == nullptr || Slots.empty())
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no rotation is constructed" });

    const VkResult Reached = vkWaitForFences(DeviceEdge->ActiveDevice(),
                                             1u,
                                             &Slots[SlotStanding].Completion,
                                             VK_TRUE,
                                             CompletionCeilingNanoseconds);

    if (Reached == VK_TIMEOUT)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::HostDenied, "the slot did not complete within the ceiling; the device is unresponsive" });
    }

    // 🔴 `06` §7: device loss is reported upward before anything is destroyed. Reported here as the refusal
    //    rather than acted on, because what to destroy and in what order is `06` §4.2's recovery and not this
    //    component's — a wait that tore down its own device would remove the operand the recovery re-scores.
    if (Reached != VK_SUCCESS)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the device declined the wait; it may be lost" });

    return Outcome<bool>::Deliver(true);
}

Outcome<bool> CycleScheduler::Arm()
{
    if (DeviceEdge == nullptr || Slots.empty())
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no rotation is constructed" });

    // 📝 Cleared immediately before the submission that signals it, never immediately after the wait. A slot
    //    cleared early and then refused before submitting is a slot no submission will ever signal, and the
    //    next rotation to reach it waits the whole ceiling out for nothing.
    if (vkResetFences(DeviceEdge->ActiveDevice(), 1u, &Slots[SlotStanding].Completion) != VK_SUCCESS)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the device declined to clear the completion" });

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE CYCLE
//------------------------------------------------------------------------------------------------------------------------

void CycleScheduler::Advance()
{
    if (Slots.empty())
        return;

    SlotStanding = (SlotStanding + 1u) % static_cast<std::uint32_t>(Slots.size());
    ++Rotations;
}

Outcome<RotationSlot> CycleScheduler::Standing() const
{
    if (Slots.empty())
        return Outcome<RotationSlot>::Refuse({ RefusalReason::CapabilityAbsent, "no rotation is constructed" });

    return Outcome<RotationSlot>::Deliver(Slots[SlotStanding]);
}

std::uint32_t CycleScheduler::StandingOrdinal() const
{
    return SlotStanding;
}

std::uint64_t CycleScheduler::CompletedRotations() const
{
    return Rotations;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

void CycleScheduler::Reclaim()
{
    if (DeviceEdge != nullptr && DeviceEdge->ActiveDevice() != VK_NULL_HANDLE)
    {
        const VkDevice Active = DeviceEdge->ActiveDevice();

        for (RotationSlot& Slot : Slots)
        {
            if (Slot.Completion != VK_NULL_HANDLE)
            {
                vkDestroyFence(Active, Slot.Completion, nullptr);
                Slot.Completion = VK_NULL_HANDLE;
            }

            if (Slot.ImageArrived != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(Active, Slot.ImageArrived, nullptr);
                Slot.ImageArrived = VK_NULL_HANDLE;
            }

            if (Slot.RecordingDone != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(Active, Slot.RecordingDone, nullptr);
                Slot.RecordingDone = VK_NULL_HANDLE;
            }
        }
    }

    Slots.clear();
    SlotStanding = 0u;
}

CycleScheduler::~CycleScheduler()
{
    Reclaim();
}

}   // namespace Slate
