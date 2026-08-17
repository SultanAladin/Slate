//============================================================================================================================================
//                                                           INTERFACEEXCHANGE.H
//============================================================================================================================================
// 🧩 The one seam the interface library crosses — device handles in, recorded commands out, no ImGui spelling.

#pragma once

#include "Contract/DeliveryContract.h"
#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"
#include "Contract/ToleranceContract.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                            WHAT THE INTERFACE ATTACHES TO
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every device handle the interface library needs, supplied once at bring-up.
/// note  🔴 Vendor spellings are verbatim here because this is the vendor surface. Nothing in this struct
///       is an ImGui spelling: the whole point of the seam is that a host including this header links the
///       interface without acquiring ImGui's declarations. `00` §2.2 makes a host that includes `imgui.h`
///       a defect, and a defect that cannot be spelled cannot be committed.
/// tag   nonallocating, nonthrowing
struct InterfaceAttachment
{
    VkInstance        Instance              = VK_NULL_HANDLE;          // [-]  - the loaded instance
    VkPhysicalDevice  ScoredDevice          = VK_NULL_HANDLE;          // [-]  - the device VendorClassifier won
    VkDevice          ActiveDevice          = VK_NULL_HANDLE;          // [-]  - the created device
    VkQueue           GraphicsQueue         = VK_NULL_HANDLE;          // [-]  - the one queue taken
    std::uint32_t     GraphicsFamilyOrdinal = 0u;                      // [-]  - the family that queue sits in
    VkFormat          ColourTargetFormat       = VK_FORMAT_UNDEFINED;  // [-] - format of DisplaySurface
    std::uint32_t     MinimumDisplayImageCount = 0u;                   // [-] - minimum requested of the chain
    std::uint32_t     DisplayImageCount        = 0u;                   // [-] - actual images the chain holds
    void*             NativeWindowSlot         = nullptr;              // [-] - WindowInterchange's handle
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE INTERFACE SEAM
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Holds the interface context and the two vendor attachments that feed it.
/// note  🔴 This is the only component in the engine that names ImGui, and it names it only inside its
///       source file. `00` §2.2: exactly one copy of ImGui exists, compiled inside `SlateUI`.
/// note  ⚠️ Everything recorded here is display-referred. `08` §3.1 places the interface after the tone
///       projection, so nothing recorded by this component is ever tone-mapped a second time.
/// tag   owning
class InterfaceExchange
{
public:

    InterfaceExchange()                                    = default;
    InterfaceExchange(const InterfaceExchange&)            = delete;
    InterfaceExchange& operator=(const InterfaceExchange&) = delete;
    ~InterfaceExchange();

    /// 🧩 Constructs the interface context over the supplied device handles.
    /// in    Arriving [-]  the device handles and the window the interface reads from
    /// out   Deliver  [-]  refuses with CapabilityAbsent when any required handle is absent, and with
    ///                     HostDenied when the vendor attachment declines
    /// note  🚧 Recording is declared against dynamic rendering, so `06`'s bring-up must negotiate
    ///       `VK_KHR_dynamic_rendering` or a device at Vulkan 1.3. Construct refuses rather than
    ///       recording into a target the device never agreed to.
    /// cost  🔴
    /// tag   api, nonthrowing
    Deliver<bool> Construct(const InterfaceAttachment& Arriving);

    /// 🧩 Destroys the interface context and both vendor attachments.
    /// cost  🚩
    /// tag   api, nonthrowing
    void Reclaim();

    /// 🧩 Opens one interface tick and reads the window system's accumulated condition.
    /// out   Deliver  [-]  refuses when no context is constructed, or when a tick is already open
    /// cost  ✔️
    /// tag   api, nonthrowing
    Deliver<bool> Advance();

    /// 🧩 Closes the open tick and assembles its command content, ready to record.
    /// out   Deliver  [-]  refuses when no tick is open
    /// post  the assembled content stays valid until the next Advance
    /// cost  🚩
    /// tag   api, nonthrowing
    Deliver<bool> Seal();

    /// 🧩 Closes an open tick without assembling it, so that nothing downstream may record it.
    /// out   Deliver  [-]  delivers true when no tick was open; abandoning nothing is not a defect
    /// post  no tick is open and Record refuses until the next Advance and Seal
    /// note  🔴 The escape a host takes when anything between Advance and Seal declines. A tick left open
    ///       makes every subsequent Advance refuse with "a tick is already open" for the life of the
    ///       process, and the interface stops responding with no error anywhere.
    /// cost  ✔️
    /// tag   api, nonthrowing
    Deliver<bool> Abandon();

    /// 🧩 Restates the display image counts after a presentation chain was re-established.
    /// in    MinimumImageCount  [-]  minimum image count requested when the chain was created
    /// in    ImageCount         [-]  actual image count the vendor returned
    /// out   Deliver            [-]  refuses when no context stands or either count is inconsistent
    /// note  🔴 These are properties of the presentation chain, never `RecordingSlotCount`. Dear ImGui's
    ///       Vulkan attachment sizes against display images independently of Slate's reusable command slots.
    /// cost  🚩
    /// tag   api, nonthrowing
    Deliver<bool> Renegotiate(std::uint32_t MinimumImageCount, std::uint32_t ImageCount);

    /// 🧩 Seats the sheet's tab figures into the vendor's style, including the four `Patches/` adds.
    /// out   Deliver  [-]  refuses with CapabilityAbsent before Construct
    /// note  🔴 The four patched members default to 0.0f, at which a patched build rasterises exactly as an
    ///        unpatched one. Seating them is what turns the trapezoid on — a build that never called this
    ///        drew stock rectangular tabs and read as though the patches had failed to apply.
    /// note  ⚠️ `TabPadAlong` and `TabOverlap` are coupled: the 38 px padding clears the slant plus the
    ///        overlap, and raising one without the other runs adjacent tabs together.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    Deliver<bool> SeatWorkspaceStyle(const WorkspaceMetric& Measure, const WorkspaceInk& Tinted);

    /// 🧩 Records the workspace tab strip with the vendor's patched tab bar, and reports what the artist did.
    /// in    Titles   [-]  one static run per open workspace, in presentation order
    /// out   Chosen   [-]  the ordinal selected this tick, or `Count` when none
    /// out   Closed   [-]  the ordinal closed this tick, or `Count` when none
    /// note  🔴 The TRAPEZOID is the vendor's, from PatchA and `Style.TabSlant`. Nothing here draws a tab:
    ///        a hand-drawn strip would not interlock, would not carry PatchB's z-order, and would not
    ///        answer the vendor's own hover and drag arbitration.
    /// note  ⚠️ A closure is REPORTED and never acted on here. Withdrawing inside this sweep would edit the
    ///        set the tab bar is walking; the caller withdraws it after the strip is sealed.
    /// cost  🚩
    /// tag   api, nonthrowing
    void RecordWorkspaceTabs(const PlaneExtent& Extent, const char* const* Titles, std::uint32_t Count,
                             std::uint32_t Active, std::uint32_t& Chosen, std::uint32_t& Closed);

    /// 🧩 Records the assembled content into a command recording of the current cycle slot.
    /// in    CommandRecording [-]  a recording already inside a dynamic rendering scope over DisplaySurface
    /// out   Deliver          [-]  refuses when nothing has been sealed since the last Advance
    /// pre   Seal delivered
    /// cost  🚩
    /// tag   api, nonthrowing
    Deliver<bool> Record(VkCommandBuffer CommandRecording);

    /// 🧩 Whether the interface has taken the pointer, so that `22` must not treat it as a canvas stroke.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    bool PointerCaptured() const;

    /// 🧩 Whether the interface has taken text entry, so that no shortcut consumes the same key.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    bool KeyboardCaptured() const;

private:

    InterfaceAttachment  Attached          = {};               // [-] - as supplied, never re-queried
    VkDescriptorPool     DescriptorSlot    = VK_NULL_HANDLE;   // [-] - sized for the interface's own imagery
    void*                ContextSlot       = nullptr;          // [-] - opaque; the ImGui spelling stays in
                                                               //       the source file
    bool                 TickOpen          = false;            // [-] - Advance delivered, Seal has not
    bool                 ContentAssembled  = false;            // [-] - Seal delivered, Record has not
    bool                 WindowAttached    = false;            // [-] - the window system attachment stands
    bool                 VendorAttached    = false;            // [-] - the vendor attachment stands
};

}   // namespace Slate
