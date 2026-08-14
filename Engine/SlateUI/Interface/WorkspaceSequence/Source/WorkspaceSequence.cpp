//============================================================================================================================================
//                                                          WORKSPACESEQUENCE.CPP
//============================================================================================================================================
// 🧩 Bring-up in `32` §1's order, the ten-step tick, and teardown as its exact reverse with the rotation drained first.

#include "SlateUI/Interface/WorkspaceSequence/Api/WorkspaceSequence.h"

#include "SlateVulkan/Device/WindowExchange/Api/WindowExchange.h"

#include <vector>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT A STEP REPORTS
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📝 Every origin below is static text naming the document and the step, so the register sorts by where a refusal
//    happened rather than by when. `86` §2.2 wants the origin at the occurrence and this is the only place the
//    bring-up's step ordinals are known.
constexpr const char* BringUpOrigin = "32 §1 WorkspaceSequence";
constexpr const char* TickOrigin    = "32 §2 WorkspaceSequence";

// 📝 🔴 The arrived chain image is the display's and not `ImageSpace`'s, so `ImageSpace::Transition` cannot carry
//    it — that component amends the record of an image it claimed, and it claimed none of these. The two barriers
//    are therefore issued here, which is the one place the chain image and the recording are both in hand.
void CarryDisplayImage(VkCommandBuffer  Recorded,
                       VkImage          Extent,
                       VkImageLayout    Departing,
                       VkImageLayout    Arriving,
                       VkAccessFlags    DepartingAccess,
                       VkAccessFlags    ArrivingAccess,
                       VkPipelineStageFlags DepartingStage,
                       VkPipelineStageFlags ArrivingStage)
{
    VkImageMemoryBarrier Carried = {};
    Carried.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Carried.srcAccessMask                   = DepartingAccess;
    Carried.dstAccessMask                   = ArrivingAccess;
    Carried.oldLayout                       = Departing;
    Carried.newLayout                       = Arriving;
    Carried.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    Carried.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    Carried.image                           = Extent;
    Carried.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    Carried.subresourceRange.levelCount     = 1u;
    Carried.subresourceRange.layerCount     = 1u;

    vkCmdPipelineBarrier(Recorded, DepartingStage, ArrivingStage, 0u, 0u, nullptr, 0u, nullptr, 1u, &Carried);
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE REGISTER
//------------------------------------------------------------------------------------------------------------------------

void WorkspaceSequence::Record(const char* Origin, const char* Subject, const Refusal& Declining)
{
    ReportSpecification Arriving;

    Arriving.Origin      = Origin;
    Arriving.Subject     = Subject;
    Arriving.Detail      = Declining.Detail;
    Arriving.Disposition = ReportDisposition::Refused;
    Arriving.Arrival     = HostTimeline.Advance();

    Register.Append(Arriving);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       BRING-UP
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> WorkspaceSequence::Construct(const WorkspaceDeclaration& Declaring)
{
    if (SequenceStanding)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "the sequence already stands" });

    if (Declaring.Roster == nullptr || Declaring.RosterCount == 0u)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the roster registers no workspace" });

    if (Declaring.RosterCount > WorkspaceRosterCeiling)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "the roster exceeds the declared workspace ceiling" });
    }

    if (Declaring.StandingOrdinal >= Declaring.RosterCount)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "the standing ordinal lies outside the roster" });
    }

    if (Declaring.DisplayWidth == 0u || Declaring.DisplayHeight == 0u
     || Declaring.DisplayWidth > DisplayExtentCeiling || Declaring.DisplayHeight > DisplayExtentCeiling)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "the declared display extent is zero or beyond the ceiling" });
    }

    Roster        = Declaring.Roster;
    RosterCount   = Declaring.RosterCount;
    StandingEntry = Declaring.StandingOrdinal;

    // 📝 A refusal from any step below leaves nothing standing. The lambda reclaims and carries the declining
    //    step's own refusal up unchanged, so one sentence is not put in front of every distinct failure.
    const auto Decline = [this](const char* Subject, const Refusal& Declining)
    {
        Record(BringUpOrigin, Subject, Declining);
        Reclaim();

        return Outcome<bool>::Refuse(Declining);
    };

    //--- ② WindowInterchange -----------------------------------------------------------------------------------------
    // 📝 ① `SlateMath` is the timeline, the register and the measures, and all three are constructed by their own
    //    member initialisers before this function is entered. There is no step to take for it and none is invented.

    DisplayExtent Asked;
    Asked.Width  = Declaring.DisplayWidth;
    Asked.Height = Declaring.DisplayHeight;

    {
        const Outcome<bool> Opened = Window.Open(Asked, Declaring.WindowTitle);

        if (!Opened.ContentPresent)
            return Decline("WindowInterchange::Open", Opened.Declined);
    }

    {
        // 🔴 `04` §3: the pointer stream is attached here so every sample carries an **arrival** stamp. A host that
        //    read the window's accumulated condition instead would stamp each sample where it was drained, which is
        //    the display rate wearing an arrival's name, and `22` reconstructs the artist's path from these.
        const Outcome<bool> Attached = Pointing.Attach(Window.NativeHandle(), HostTimeline);

        if (!Attached.ContentPresent)
            return Decline("InputExchange::Attach", Attached.Declined);
    }

    //--- ③ SlateVulkan -----------------------------------------------------------------------------------------------
    {
        const Outcome<bool> Loaded = DeviceEdge.ConstructInstance(Declaring.DiagnosticRequested);

        if (!Loaded.ContentPresent)
            return Decline("VulkanExchange::ConstructInstance", Loaded.Declined);
    }

    {
        // 🔴 A refusal here is recorded and is **not** a bring-up failure. `VK_EXT_debug_utils` is optional by
        //    declaration, and `DiagnosticExtension::Declare` delivers as a no-op when nothing negotiated it — so no
        //    claim site below branches on the configuration, and this is the only place the absence is noticed.
        const Outcome<bool> Negotiated = Naming.Construct(DeviceEdge, Register, HostTimeline);

        if (!Negotiated.ContentPresent)
            Record(BringUpOrigin, "DiagnosticExtension::Construct", Negotiated.Declined);
    }

    {
        const Outcome<VkSurfaceKHR> Converted = ::Slate::Convert(DeviceEdge.Instance(), Window.NativeHandle());

        if (!Converted.ContentPresent)
            return Decline("WindowExchange::Convert", Converted.Declined);

        DisplaySurface = Converted.Resolve();
    }

    {
        const Outcome<bool> Created = DeviceEdge.ConstructDevice(DisplaySurface);

        if (!Created.ContentPresent)
            return Decline("VulkanExchange::ConstructDevice", Created.Declined);
    }

    {
        const Outcome<bool> Sliced = BackingSpace.Construct(DeviceEdge, Naming);

        if (!Sliced.ContentPresent)
            return Decline("ByteSpace::Construct", Sliced.Declined);
    }

    {
        const Outcome<bool> Claimed = Images.Construct(DeviceEdge, BackingSpace, Naming);

        if (!Claimed.ContentPresent)
            return Decline("ImageSpace::Construct", Claimed.Declined);
    }

    {
        // 🔴 The chain stands **before** the targets are claimed. `08` §2 gives `DisplaySurface` the format of the
        //    display rather than a declared one, and `DisplayScheduler::Carries` is the only place that is known —
        //    claiming first would mean re-deriving the format at the claim site, with nothing comparing the two.
        const Outcome<bool> Established = Display.Construct(DeviceEdge, Naming, DisplaySurface,
                                                            Declaring.DisplayWidth, Declaring.DisplayHeight,
                                                            Declaring.Intent);

        if (!Established.ContentPresent)
            return Decline("DisplayScheduler::Construct", Established.Declined);
    }

    {
        const Outcome<bool> Claimed = Targets.Claim(Images, Display.StandingWidth(), Display.StandingHeight(),
                                                    Display.Carries());

        if (!Claimed.ContentPresent)
            return Decline("TargetSpace::Claim", Claimed.Declined);
    }

    {
        const Outcome<bool> Declared = Attachments.Construct(DeviceEdge, Targets);

        if (!Declared.ContentPresent)
            return Decline("AttachmentIndex::Construct", Declared.Declined);
    }

    {
        // 📝 Derived over whatever constructs stand, which is none until a contributing document declares one. A
        //    derivation over an empty index delivers, and calling it here rather than at the first resize keeps
        //    `06` §7's order — chain, targets, spans — spelled once, in the order both this and `ReclaimExtent` use.
        const Outcome<bool> Covered = Attachments.Derive(Display.StandingWidth(), Display.StandingHeight());

        if (!Covered.ContentPresent)
            return Decline("AttachmentIndex::Derive", Covered.Declined);
    }

    {
        const Outcome<bool> Ordered = Rotation.Construct(DeviceEdge, Naming);

        if (!Ordered.ContentPresent)
            return Decline("CycleScheduler::Construct", Ordered.Declined);
    }

    {
        const Outcome<bool> Written = Recordings.Construct(DeviceEdge, Naming);

        if (!Written.ContentPresent)
            return Decline("CommandSequence::Construct", Written.Declined);
    }

    //--- ④ SlateDocument and ⑤ SlateCompute --------------------------------------------------------------------------
    // 📝 🔴 Neither is constructed here and neither is skipped by oversight. `32` §1's ④ and ⑤ are the document
    //    session and the compute products, and both are the **application's** to own — a sequence that constructed
    //    a document session would decide what an editor opens with, which is the one decision `32` §5 leaves to the
    //    host. The ordering constraints they carry — ⑤ after ④ per `22` §1, ⑥ after ⑤ per `12` invariant 10 — are
    //    honoured by their arriving through the workspace context before `DeclarePanels` is called at ⑥ below.

    //--- ⑥ SlateUI ---------------------------------------------------------------------------------------------------
    {
        const Outcome<ThemeSpecification> Resolved = ResolveActiveTheme(Declaring.DensityScale);

        if (!Resolved.ContentPresent)
            return Decline("ResolveActiveTheme", Resolved.Declined);

        ActiveTheme = Resolved.Resolve();
    }

    {
        InterfaceAttachment Attaching;

        Attaching.Instance              = DeviceEdge.Instance();
        Attaching.ScoredDevice          = DeviceEdge.ScoredDevice();
        Attaching.ActiveDevice          = DeviceEdge.ActiveDevice();
        Attaching.GraphicsQueue         = DeviceEdge.GraphicsQueue();
        Attaching.GraphicsFamilyOrdinal = DeviceEdge.Capability().GraphicsFamilyOrdinal;
        Attaching.ColourTargetFormat    = Display.Carries();
        Attaching.RotationDepth         = RecordingRotationDepth;
        Attaching.NativeWindowSlot      = Window.NativeHandle();

        const Outcome<bool> Seamed = Interface.Construct(Attaching);

        if (!Seamed.ContentPresent)
            return Decline("InterfaceExchange::Construct", Seamed.Declined);
    }

    {
        // 📝 🔴 The catalogue is **derived from the roster** rather than declared a second time. One registered
        //    workspace is one thing the artist can mint, so a host declaring both would be a host in which the two
        //    lists can disagree — and the disagreement presents as a catalogue row that mints a document no
        //    workspace presents.
        WorkspaceDocumentSpecification Offering[WorkspaceRosterCeiling] = {};

        for (std::uint32_t Ordinal = 0u; Ordinal < RosterCount; ++Ordinal)
        {
            Offering[Ordinal].Label      = Roster[Ordinal].Caption;
            Offering[Ordinal].NameStem   = Roster[Ordinal].NameStem;
            Offering[Ordinal].Discipline = Roster[Ordinal].Discipline;
        }

        const Outcome<bool> Offered = DeclareCatalogue(Space, Offering, RosterCount, StandingEntry);

        if (!Offered.ContentPresent)
            return Decline("DeclareCatalogue", Offered.Declined);
    }

    {
        const Outcome<WorkspaceDocumentIdentity> Seeded = ConstructWorkspaceSpace(Space);

        if (!Seeded.ContentPresent)
            return Decline("ConstructWorkspaceSpace", Seeded.Declined);
    }

    DeclareStanding();

    //--- ⑦ RenderSchedule --------------------------------------------------------------------------------------------
    {
        // 🔴 Validated and **not** repaired. A schedule whose ordering cannot be derived is a refusal naming the
        //    target; a bring-up that quietly reordered it would present the artist with an image composited in an
        //    order no document describes, and nothing downstream could tell that had happened.
        const Outcome<bool> Fixed = Schedule.Fix();

        if (!Fixed.ContentPresent)
            return Decline("RenderSchedule::Fix", Fixed.Declined);
    }

    SequenceStanding = true;

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE STANDING LEDGER
//------------------------------------------------------------------------------------------------------------------------

void WorkspaceSequence::DeclareStanding()
{
    // 🔴 Emptied before the arriving workspace declares into it. A ledger carried across an activation presents the
    //    departing workspace's panels against a context it has already released, and the artist meets that as a
    //    panel whose contents are a different workspace's.
    ReclaimPanelIndex(Ledger);

    if (Roster == nullptr || StandingEntry >= RosterCount)
        return;

    const WorkspaceSpecification& Standing = Roster[StandingEntry];

    if (Standing.DeclarePanels != nullptr)
        Standing.DeclarePanels(Ledger, Standing.WorkspaceContext);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE ARRIVED EXTENT
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> WorkspaceSequence::ReclaimExtent(std::uint32_t ArrivedWidth, std::uint32_t ArrivedHeight)
{
    if (ArrivedWidth == 0u || ArrivedHeight == 0u)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the arrived extent is zero" });

    // 🔴 Idle before the first reclaim and not between them. Every rotation that reads the old chain and the old
    //    targets must have completed, and a component reclaimed while a recording still reads it is a vendor object
    //    destroyed under the device — reported against whichever object the driver noticed rather than the early one.
    if (DeviceEdge.ActiveDevice() != VK_NULL_HANDLE)
        vkDeviceWaitIdle(DeviceEdge.ActiveDevice());

    {
        const Outcome<bool> Established = Display.Reclaim(ArrivedWidth, ArrivedHeight);

        if (!Established.ContentPresent)
            return Established;
    }

    {
        // 🔴 The targets follow the chain and the spans follow the targets. Any other order presents one target at
        //    the extent the display carried before the drag, which reads as a shifted image rather than as a resize
        //    that was half applied — `06` §7, verbatim.
        const Outcome<bool> Claimed = Targets.Reclaim(Display.StandingWidth(), Display.StandingHeight());

        if (!Claimed.ContentPresent)
            return Claimed;
    }

    return Attachments.Derive(Display.StandingWidth(), Display.StandingHeight());
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE ARBITRATION
//------------------------------------------------------------------------------------------------------------------------

PointerArbitration WorkspaceSequence::Arbitrate()
{
    PointerArbitration Resolved = Arbitrated;

    // 📝 Text entry is arbitrated every tick and the pointer is not. A rename caret opens and closes without a drag,
    //    and a shortcut consumed while one is open is the defect this separation exists to prevent.
    Resolved.KeyboardTaken = Interface.KeyboardCaptured();

    const bool DeskHolding = Space.Dragging.Mode != WorkspaceDragMode::None;

    // 🔴 **Capture persists for the whole drag.** The claimant is settled on the tick the hold opens and is not
    //    revisited until it ends. Re-arbitrating each tick is `14` §4.2's recorded defect: a stroke stops the moment
    //    the cursor crosses a floating panel, and the artist reads that as the brush failing rather than as an
    //    arbitration they cannot see.
    if (Resolved.DragStanding && DeskHolding)
        return Resolved;

    // 📝 The interface's own claim is read from the vendor's accumulated condition and from the desk's drag record
    //    together. `PointerCaptured` describes the tick that has already been sealed, so a press that lands on a
    //    trapezoid is seen here one tick later — which is why the desk's record is consulted beside it rather than
    //    instead of it.
    const bool InterfaceClaiming = Interface.PointerCaptured() || DeskHolding
                                || Arbitrated.Holder == PointerClaimant::Interface;

    const PointerClaimant Arriving = InterfaceClaiming ? PointerClaimant::Interface : PointerClaimant::Workspace;

    // 📝 🔴 `Manipulator` and `Stroke` are declared and never resolved here, and that is not an omission. Both are
    //    claims a **document** makes, and this sequence owns no document — the host threads one through the
    //    workspace context, and the workspace that owns it narrows the claim before the canvas reads it. A claimant
    //    invented here would be one nothing can withdraw.

    if (Arriving != Resolved.Holder || (DeskHolding && !Resolved.DragStanding))
        Resolved.Arrival = HostTimeline.Advance();

    Resolved.Holder       = Arriving;
    Resolved.DragStanding = DeskHolding;

    return Resolved;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE TICK
//------------------------------------------------------------------------------------------------------------------------

Outcome<TickReport> WorkspaceSequence::Advance()
{
    if (!SequenceStanding)
        return Outcome<TickReport>::Refuse({ RefusalReason::CapabilityAbsent, "no sequence stands" });

    TickReport Reported;

    //--- ① drain the window system ------------------------------------------------------------------------------------
    Window.Drain();

    Reported.ClosureRequested = Window.ClosureRequested();

    const DisplayExtent Arrived = Window.CurrentExtent();

    // 📝 🔴 An arrived extent is applied here, before anything is assembled against it. Applying it after the
    //    interface tick had been built would present one tick of interface laid out at the extent the display no
    //    longer carries, and the artist meets that as panels that jump one frame behind the window edge.
    // ⚠️ A zero extent is a minimised window. Nothing is re-established and the rotation is skipped; establishing a
    //    chain no image can be claimed from is what `DisplayScheduler::Reclaim` refuses, and refusing every tick of
    //    a minimised window would fill the register with the artist's own window manager.
    if ((Arrived.Width != Display.StandingWidth() || Arrived.Height != Display.StandingHeight())
     && Arrived.Width != 0u && Arrived.Height != 0u)
    {
        const Outcome<bool> Reclaimed = ReclaimExtent(Arrived.Width, Arrived.Height);

        if (!Reclaimed.ContentPresent)
        {
            Record(TickOrigin, "ReclaimExtent", Reclaimed.Declined);

            if (Reclaimed.Declined.DeclaredReason == RefusalReason::DeviceLost)
                return Outcome<TickReport>::Refuse(Reclaimed.Declined);
        }
        else
        {
            Reported.ExtentReclaimed = true;
        }
    }

    //--- ② arbitrate the pointer --------------------------------------------------------------------------------------
    Arbitrated          = Arbitrate();
    Reported.Arbitrated = Arbitrated;

    //--- ③ apply the document's pending intent --------------------------------------------------------------------------
    // 📝 🔴 Nothing is applied here and the step is not skipped. `14` §4.1 routes intent that amends nothing in the
    //    document straight to its owner and never through a transaction, and every such owner sits behind the
    //    workspace context — so this step is discharged by the workspace itself, inside ⑤, where it holds what it
    //    owns. A sequence applying intent on a document's behalf would be a sequence that knows what a document is.

    //--- ④ advance the interface tick -----------------------------------------------------------------------------------
    {
        const Outcome<bool> Opened = Interface.Advance();

        if (!Opened.ContentPresent)
        {
            Record(TickOrigin, "InterfaceExchange::Advance", Opened.Declined);

            return Outcome<TickReport>::Refuse(Opened.Declined);
        }
    }

    //--- ⑤ present the bracket ------------------------------------------------------------------------------------------
    {
        const char* Captions[WorkspaceRosterCeiling] = {};

        for (std::uint32_t Ordinal = 0u; Ordinal < RosterCount; ++Ordinal)
            Captions[Ordinal] = Roster[Ordinal].Caption;

        // 📝 🚧 The centre routine is presented **before** the bracket so the desk's panels paint over it rather than
        //    under it, and the rectangle it is handed is derived from the same two theme extents the bracket
        //    derives its own from. Two derivations of one rectangle is `00` §2's case and it is open: the closing
        //    move is for the bracket to take the routine as an operand, which is a `WorkspaceSpace` amendment.
        if (Roster[StandingEntry].PresentCentre != nullptr)
        {
            const LayoutExtents& Extents = ActiveTheme.Extents;

            const float DeskTop    = Extents.TabStripHeight;
            const float DeskBottom = static_cast<float>(Display.StandingHeight()) - Extents.ViewportBandBottom;

            WorkspaceRectangle CentreArea;
            CentreArea.PositionX = 0.0f;
            CentreArea.PositionY = DeskTop;
            CentreArea.Width     = static_cast<float>(Display.StandingWidth());
            CentreArea.Height    = DeskBottom > DeskTop ? DeskBottom - DeskTop : 0.0f;

            if (CentreArea.Width > 0.0f && CentreArea.Height > 0.0f)
                Roster[StandingEntry].PresentCentre(ActiveTheme, CentreArea, Roster[StandingEntry].WorkspaceContext);
        }

        const DeploymentReport Bracketed =
            PresentDeploymentBracket(ActiveTheme, Space, &Ledger, Captions, RosterCount, StandingEntry,
                                     "",
                                     static_cast<float>(Display.StandingWidth()),
                                     static_cast<float>(Display.StandingHeight()));

        if (Bracketed.WorkspaceChoice != AbsentWorkspaceChoice
         && Bracketed.WorkspaceChoice <  RosterCount
         && Bracketed.WorkspaceChoice != StandingEntry)
        {
            StandingEntry = Bracketed.WorkspaceChoice;

            DeclareStanding();
        }

        if (Bracketed.PointerConsumed)
            Arbitrated.Holder = PointerClaimant::Interface;

        Reported.Arbitrated        = Arbitrated;
        Reported.StandingWorkspace = StandingEntry;
    }

    {
        // 🔴 Sealed here rather than at ⑧, so the interface tick is balanced on **every** path out of this function.
        //    A skipped rotation that left the tick open would make the next `Advance` refuse, and a minimised window
        //    would then stop the host permanently rather than for as long as it was minimised.
        const Outcome<bool> Assembled = Interface.Seal();

        if (!Assembled.ContentPresent)
            Record(TickOrigin, "InterfaceExchange::Seal", Assembled.Declined);
    }

    if (Arrived.Width == 0u || Arrived.Height == 0u)
    {
        Reported.RotationSkipped = true;
        Reported.PacedInterval   = Display.PacedInterval();

        return Outcome<TickReport>::Deliver(Reported);
    }

    //--- ⑥ await the rotation slot --------------------------------------------------------------------------------------
    {
        const Outcome<bool> Completed = Rotation.Await();

        if (!Completed.ContentPresent)
        {
            Record(TickOrigin, "CycleScheduler::Await", Completed.Declined);

            return Outcome<TickReport>::Refuse(Completed.Declined);
        }
    }

    const Outcome<RotationSlot> Standing = Rotation.Standing();

    if (!Standing.ContentPresent)
    {
        Record(TickOrigin, "CycleScheduler::Standing", Standing.Declined);

        return Outcome<TickReport>::Refuse(Standing.Declined);
    }

    const RotationSlot& Slot = Standing.Resolve();

    //--- ⑦ take the display image ---------------------------------------------------------------------------------------
    const Outcome<ArrivedImage> Taken = Display.Await(Slot, HostTimeline);

    if (!Taken.ContentPresent)
    {
        Record(TickOrigin, "DisplayScheduler::Await", Taken.Declined);

        if (Taken.Declined.DeclaredReason == RefusalReason::DeviceLost)
            return Outcome<TickReport>::Refuse(Taken.Declined);

        Reported.RotationSkipped = true;
        Reported.PacedInterval   = Display.PacedInterval();

        return Outcome<TickReport>::Deliver(Reported);
    }

    const ArrivedImage& Image = Taken.Resolve();

    // 🔴 `Reclaimed` is delivered in two cases and `ImageOrdinal` is what tells them apart. A chain the display has
    //    merely outgrown hands back a usable image — record it, present it, re-establish afterwards. A chain it has
    //    retired hands back `AbsentDisplayImage` and no view, so there is nothing to record into and the rotation is
    //    skipped. A host reading `Reclaimed` alone records into a null view on the second of the two.
    if (Image.ImageOrdinal == AbsentDisplayImage || Image.WholeView == VK_NULL_HANDLE)
    {
        const Outcome<bool> Reclaimed = ReclaimExtent(Arrived.Width, Arrived.Height);

        if (!Reclaimed.ContentPresent)
            Record(TickOrigin, "ReclaimExtent", Reclaimed.Declined);
        else
            Reported.ExtentReclaimed = true;

        Reported.RotationSkipped = true;
        Reported.PacedInterval   = Display.PacedInterval();

        return Outcome<TickReport>::Deliver(Reported);
    }

    //--- ⑧ record the schedule and the interface --------------------------------------------------------------------------
    const Outcome<VkCommandBuffer> Opened = Recordings.Open(Rotation.StandingOrdinal());

    if (!Opened.ContentPresent)
    {
        Record(TickOrigin, "CommandSequence::Open", Opened.Declined);

        return Outcome<TickReport>::Refuse(Opened.Declined);
    }

    VkCommandBuffer Recorded = Opened.Resolve();

    CarryDisplayImage(Recorded, Image.Extent,
                      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                      0u, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    {
        // 📝 🚧 `06` §2.1 settled on the classic render construct and `AttachmentIndex` builds them, but the arrived
        //    chain image is not a claimed target and no construct spans it — so the display recording is opened as a
        //    dynamic scope, which is also what `InterfaceExchange` was constructed against. Two mechanisms answer one
        //    question and the choice is open; the schedule's own recordings continue to open constructs.
        VkRenderingAttachmentInfo Colour = {};
        Colour.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        Colour.imageView   = Image.WholeView;
        Colour.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        Colour.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
        Colour.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;

        // 📝 The clear is the desk's own background rather than a literal, so a palette change reaches the one pixel
        //    the interface never paints over — `ThemeSpecification` stays the only place a colour is authored.
        Colour.clearValue.color.float32[0] =
            static_cast<float>(ActiveTheme.Palette.DeskBackground.Coordinate.RedCoordinate);
        Colour.clearValue.color.float32[1] =
            static_cast<float>(ActiveTheme.Palette.DeskBackground.Coordinate.GreenCoordinate);
        Colour.clearValue.color.float32[2] =
            static_cast<float>(ActiveTheme.Palette.DeskBackground.Coordinate.BlueCoordinate);
        Colour.clearValue.color.float32[3] = 1.0f;

        VkRenderingInfo Scope = {};
        Scope.sType                    = VK_STRUCTURE_TYPE_RENDERING_INFO;
        Scope.renderArea.extent.width  = Display.StandingWidth();
        Scope.renderArea.extent.height = Display.StandingHeight();
        Scope.layerCount               = 1u;
        Scope.colorAttachmentCount     = 1u;
        Scope.pColorAttachments        = &Colour;

        vkCmdBeginRendering(Recorded, &Scope);

        // 🔴 `08` §3.1 places the interface **after** the tone projection, so it is recorded last and is
        //    display-referred throughout. Nothing recorded from here is tone-mapped a second time.
        const Outcome<bool> Painted = Interface.Record(Recorded);

        if (!Painted.ContentPresent)
            Record(TickOrigin, "InterfaceExchange::Record", Painted.Declined);

        vkCmdEndRendering(Recorded);
    }

    CarryDisplayImage(Recorded, Image.Extent,
                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0u,
                      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

    //--- ⑨ surrender the recording ----------------------------------------------------------------------------------------
    {
        // 🔴 The completion is cleared here, immediately before the surrender that signals it, and by its owner. A
        //    component clearing an ordering point it does not own clears it at the wrong moment for every other
        //    reader of it.
        const Outcome<bool> Armed = Rotation.Arm();

        if (!Armed.ContentPresent)
            Record(TickOrigin, "CycleScheduler::Arm", Armed.Declined);

        SurrenderOrdering Ordering;

        // 📝 The wait applies where the recording writes colour and not at the top of the ordering. A wait at the top
        //    serialises the whole recording against an arrival it needs only at its last stage, and the stall that
        //    produces reads as a device too slow for the extent.
        Ordering.Awaited      = Slot.ImageArrived;
        Ordering.AwaitedStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Ordering.Signalled    = Slot.RecordingDone;
        Ordering.Completion   = Slot.Completion;

        const Outcome<bool> Surrendered = Recordings.Surrender(Rotation.StandingOrdinal(), Ordering);

        if (!Surrendered.ContentPresent)
        {
            Record(TickOrigin, "CommandSequence::Surrender", Surrendered.Declined);

            if (Surrendered.Declined.DeclaredReason == RefusalReason::DeviceLost)
                return Outcome<TickReport>::Refuse(Surrendered.Declined);
        }
    }

    //--- ⑩ present the image ----------------------------------------------------------------------------------------------
    {
        const Outcome<bool> Presented = Display.Present(Slot, Image.ImageOrdinal);

        if (!Presented.ContentPresent)
        {
            Record(TickOrigin, "DisplayScheduler::Present", Presented.Declined);

            if (Presented.Declined.DeclaredReason == RefusalReason::DeviceLost)
                return Outcome<TickReport>::Refuse(Presented.Declined);
        }
    }

    // 📝 The image was presented before the chain is re-established, which is the whole reason `Reclaimed` arrives
    //    beside a usable ordinal. Re-establishing first would drop the rotation that noticed the resize, and the
    //    artist sees that as one stalled stroke rather than as a resize.
    if (Image.Reclaimed)
    {
        const Outcome<bool> Reclaimed = ReclaimExtent(Arrived.Width, Arrived.Height);

        if (!Reclaimed.ContentPresent)
            Record(TickOrigin, "ReclaimExtent", Reclaimed.Declined);
        else
            Reported.ExtentReclaimed = true;
    }

    Rotation.Advance();

    //--- what the tick sampled --------------------------------------------------------------------------------------------
    // 🔴 `86` §10: sampled **by the tick** and never pushed by a producer. A producer declaring its own measure
    //    declares it at whatever rate it runs at, and the panel then presents a figure whose interval nothing states.
    {
        const TickPoint Sampled = HostTimeline.Advance();

        Measured.DeclareMagnitude("06 §3 DisplayScheduler", "PacedInterval", Display.PacedInterval(), Sampled);
        Measured.DeclareCount("06 §3 DisplayScheduler",     "Presented",     Display.Presented(),     Sampled);
        Measured.DeclareCount("06 §3 CycleScheduler",       "Rotations",     Rotation.CompletedRotations(), Sampled);
        Measured.DeclareCount("06 §3 ByteSpace",            "DeviceLocal",
                              BackingSpace.ClaimedBytes(ExtentResidency::DeviceLocal),  Sampled);
        Measured.DeclareCount("06 §3 ByteSpace",            "HostWritable",
                              BackingSpace.ClaimedBytes(ExtentResidency::HostWritable), Sampled);
        Measured.DeclareCount("06 §3 ImageSpace",           "ClaimedImages", Images.ClaimedCount(), Sampled);
        Measured.DeclareCount("86 §3 ReportSequence",       "Retained",      Register.RetainedCount(), Sampled);
    }

    Reported.PacedInterval = Display.PacedInterval();

    return Outcome<TickReport>::Deliver(Reported);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       TEARDOWN
//------------------------------------------------------------------------------------------------------------------------

void WorkspaceSequence::Reclaim()
{
    // 🔴 Drained once, before the first reclaim, and never between them. Draining per component would idle the
    //    device seven times over one teardown, and the first drain is already sufficient — no recording is opened
    //    after it.
    if (DeviceEdge.ActiveDevice() != VK_NULL_HANDLE)
        vkDeviceWaitIdle(DeviceEdge.ActiveDevice());

    //--- ⑦ RenderSchedule -------------------------------------------------------------------------------------------
    // 📝 The schedule owns no vendor object. It is listed so the reverse order reads against `32` §1's forward one
    //    without a reader having to work out which steps were silently absent.

    //--- ⑥ SlateUI --------------------------------------------------------------------------------------------------
    ReclaimPanelIndex(Ledger);

    Space = WorkspaceSpace{};

    Interface.Reclaim();

    //--- ③ SlateVulkan ----------------------------------------------------------------------------------------------
    Recordings.Reclaim();
    Rotation.Reclaim();
    Display.Surrender();
    Attachments.Reclaim();
    Targets.Surrender();
    Images.Reclaim();
    BackingSpace.Reclaim();

    Naming.Reclaim();

    // 🔴 The surface is destroyed by whoever converted it and after the chain that was established against it has
    //    been surrendered. `DisplayScheduler::Surrender` deliberately leaves it standing for exactly this line.
    if (DisplaySurface != VK_NULL_HANDLE && DeviceEdge.Instance() != VK_NULL_HANDLE)
    {
        ::Slate::Reclaim(DeviceEdge.Instance(), DisplaySurface);

        DisplaySurface = VK_NULL_HANDLE;
    }

    DeviceEdge.ReclaimDevice();

    //--- ② WindowInterchange ----------------------------------------------------------------------------------------
    // 📝 Detached before the window is reclaimed. `InputExchange::Attach` chained onto whatever the window system
    //    already held, so returning that stream after the window has gone would return it to nothing.
    Pointing.Detach();

    //--- ① SlateMath ------------------------------------------------------------------------------------------------
    // 📝 The register is **not** emptied. A host that refused during bring-up has its reason in there, and the
    //    caller reads it after this returns — `Reclaim` running inside a refused `Construct` is precisely when that
    //    matters most.
    Measured.Reclaim();

    Roster           = nullptr;
    RosterCount      = 0u;
    StandingEntry    = 0u;
    Arbitrated       = {};
    SequenceStanding = false;
}

WorkspaceSequence::~WorkspaceSequence()
{
    Reclaim();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

bool WorkspaceSequence::ClosureRequested() const
{
    return Window.ClosureRequested();
}

const ThemeSpecification& WorkspaceSequence::Theme() const   { return ActiveTheme;  }

WorkspaceSpace&           WorkspaceSequence::Desk()          { return Space;        }
const WorkspaceSpace&     WorkspaceSequence::Desk() const    { return Space;        }

ReportSequence&           WorkspaceSequence::Reports()       { return Register;     }
const ReportSequence&     WorkspaceSequence::Reports() const { return Register;     }

MeasureIndex&             WorkspaceSequence::Measures()      { return Measured;     }
const MeasureIndex&       WorkspaceSequence::Measures() const{ return Measured;     }

const TickSequence&       WorkspaceSequence::Timeline() const{ return HostTimeline; }

const PanelIndex&         WorkspaceSequence::Declared() const{ return Ledger;       }

std::uint64_t WorkspaceSequence::CompletedRotations() const
{
    return Rotation.CompletedRotations();
}

}   // namespace Slate
