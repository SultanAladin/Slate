//============================================================================================================================================
//                                                          WORKSPACESEQUENCE.H
//============================================================================================================================================
// 🧩 Ordered bring-up, the ten-step tick, and teardown as its exact reverse — the whole host, with no concrete workspace named.

#pragma once

#include "Contract/OutcomeContract.h"
#include "Contract/PrecisionContract.h"
#include "SlateMath/Numeric/ReportSequence/Api/ReportSequence.h"
#include "SlateMath/Platform/InputExchange/Api/InputExchange.h"
#include "SlateMath/Platform/TickSequence/Api/TickSequence.h"
#include "SlateMath/Platform/WindowInterchange/Api/WindowInterchange.h"
#include "SlateUI/Interface/DrawerPanel/Api/DrawerPanel.h"
#include "SlateUI/Interface/InterfaceExchange/Api/InterfaceExchange.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/PanelIndex.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/WorkspaceSpace.h"
#include "SlateVulkan/Device/AttachmentIndex/Api/AttachmentIndex.h"
#include "SlateVulkan/Device/ByteSpace/Api/ByteSpace.h"
#include "SlateVulkan/Device/CommandSequence/Api/CommandSequence.h"
#include "SlateVulkan/Device/CycleScheduler/Api/CycleScheduler.h"
#include "SlateVulkan/Device/DiagnosticExtension/Api/DiagnosticExtension.h"
#include "SlateVulkan/Device/DisplayScheduler/Api/DisplayScheduler.h"
#include "SlateVulkan/Device/ImageSpace/Api/ImageSpace.h"
#include "SlateVulkan/Device/RenderSchedule/Api/RenderSchedule.h"
#include "SlateVulkan/Device/VulkanExchange/Api/VulkanExchange.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  ONE REGISTERED WORKSPACE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The routine one workspace declares its panels through, handed the ledger to fill and its own context.
/// note  🔴 Called at activation and at nothing else. A workspace that declared its panels every tick would rebuild
///        the ledger under a drag in flight, and the box being resized would name a slot that moved beneath it.
/// tag   contract
using WorkspaceDeclareRoutine = void (*)(PanelIndex& Ledger, void* WorkspaceContext);

/// 🧩 The routine one workspace declares its two edge drawers through, handed the index to fill and its own context.
/// note  🔴 Separate from the panel declaration and not folded into it. A drawer is not a panel: it docks nowhere,
///        takes no share of the desk, and is presented above everything the desk paints. One routine filling both
///        would put the drawer's record inside the ledger that `ReclaimPanelIndex` empties, and the reveal an artist
///        dragged out would close every time a panel was minted.
/// note  ⚠️ Called at activation, exactly as the panel routine is, and for the same reason — both address storage
///        the workspace owns, so a declaration retained across an activation names a record the departing workspace
///        has already released.
/// tag   contract
using WorkspaceDrawerRoutine = void (*)(DrawerIndex& Drawers, void* WorkspaceContext);

/// 🧩 The routine one workspace presents its own body content through, beneath every panel the desk paints.
/// note  ⚠️ Optional. A workspace whose whole surface is panels declares none, and the centre remainder is left
///        to whatever the render schedule composited beneath the interface.
/// tag   contract
using WorkspacePresentRoutine = void (*)(const ThemeSpecification& Theme,
                                         const WorkspaceRectangle& CentreArea,
                                         void*                     WorkspaceContext);

/// 🧩 One workspace as an application registers it — its caption, its panels, and its own opaque context.
/// note  🔴 `32` §5: the roster is the application's and this component never spells an entry of it. That is what
///        lets one tree deliver `PaintHost` with one entry and `EditorHost` with every entry, from the same
///        `WorkspaceSequence` — the difference between the two executables is the length of an array.
/// note  ⚠️ Both text pointers and the context are the application's storage. Nothing here is copied, so a roster
///        built from a local buffer dangles the first time the declaring scope closes.
/// tag   contract, nonallocating, nonthrowing
struct WorkspaceSpecification
{
    const char*              Caption          = "Workspace";                     // [-] - the strip trapezoid's text
    const char*              NameStem         = "Workspace";                     // [-] - minted titles read "<stem> N"
    WorkspaceDiscipline      Discipline       = WorkspaceDiscipline::Empty;      // [-] - what its documents are for
    WorkspaceDeclareRoutine  DeclarePanels    = nullptr;                          // [-] - fills the ledger at activation
    WorkspaceDrawerRoutine   DeclareDrawers   = nullptr;                          // [-] - fills the drawer index, likewise
    WorkspacePresentRoutine  PresentCentre    = nullptr;                          // [-] - optional body content
    void*                    WorkspaceContext = nullptr;                          // [-] - the application's storage
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  WHAT THE HOST DECLARES
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Everything an application declares once, before bring-up.
/// note  🔴 The roster and the display extent are the only two things a host chooses. Everything else the sequence
///        brings up is settled by the documents, so a host with a knob for it would be a host that can bring the
///        engine up in an arrangement no document describes.
/// tag   contract, nonallocating, nonthrowing
struct WorkspaceDeclaration
{
    const char*                    WindowTitle     = "Slate";                    // [-]  - what the window system shows
    const WorkspaceSpecification*  Roster          = nullptr;                     // [-]  - the registered workspaces
    std::uint32_t                  RosterCount     = 0u;                          // [-]  - how many
    std::uint32_t                  StandingOrdinal = 0u;                          // [-]  - which one is active at bring-up
    std::uint32_t                  DisplayWidth    = 1600u;                       // [px] - the extent asked of the window
    std::uint32_t                  DisplayHeight   = 900u;                         // [px]
    float                          DensityScale    = 1.0f;                         // [-]  - folded into every extent once
    LatencyIntent                  Intent          = LatencyIntent::SteadyPacing;  // [-]  - what pacing optimises for
    bool                           DiagnosticRequested = false;                    // [-]  - the vendor's own reporting
};

// 📝 The roster is walked into a bounded automatic extent every tick — the captions the strip is handed. Bounded
//    rather than allocated so one tick of the host allocates nothing at all, and refused at bring-up rather than
//    truncated: a workspace registered and silently absent from the strip is one the artist cannot reach.
inline constexpr std::uint32_t WorkspaceRosterCeiling = 16u;   // [-] - registered workspaces one host may carry

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE ARBITRATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Who owns the pointer, in `14` §4.2's precedence order.
/// note  🔴 `14` §4.2 orders four claimants and the order is not negotiable: the interface first, then a manipulator
///        drag, then a canvas stroke, then the workspace itself. A stroke that outranked the interface would paint
///        through a panel the artist was aiming at.
/// tag   contract
enum class PointerClaimant : std::uint32_t
{
    Unclaimed     = 0u,   // [-] - nothing has taken it this tick
    Interface     = 1u,   // [-] - a panel, a strip, a band or a control
    Manipulator   = 2u,   // [-] - a transform manipulator drag over the canvas
    Stroke        = 3u,   // [-] - the canvas stroke path
    Workspace     = 4u,   // [-] - the workspace's own navigation
    ClaimantCount = 5u    // [-] - the closed count, never a claimant
};

/// 🧩 What one tick's arbitration settled — who holds the pointer, and whether text entry is taken.
/// note  🔴 **Capture persists for the whole drag.** The claimant is re-arbitrated only on the tick the primary
///        control goes down; every later tick of the same hold returns the standing claimant unchanged. Frontier
///        re-arbitrated each tick and the defect presented as a stroke that stopped the moment the cursor crossed a
///        floating panel — the artist reads that as the brush failing, not as an arbitration.
/// note  ⚠️ `KeyboardTaken` is separate and is arbitrated every tick. A shortcut consumed while a rename caret is
///        open is the defect it exists to prevent, and text entry opens and closes without a drag.
/// tag   contract, nonallocating, nonthrowing
struct PointerArbitration
{
    PointerClaimant  Holder        = PointerClaimant::Unclaimed;   // [-]  - who owns the pointer this tick
    bool             DragStanding  = false;                        // [-]  - a hold is open and is not re-arbitrated
    bool             KeyboardTaken = false;                        // [-]  - text entry is open somewhere
    TickPoint        Arrival       = {};                           // [ns] - when the hold that opened it arrived
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT A TICK DID
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What one tick of the sequence resolved, for a host that reports rather than one that decides.
/// note  ⚠️ `RotationSkipped` is not a failure. A chain the display retired delivers no image to record into, so the
///        rotation is skipped and the next one proceeds — a host treating it as a refusal would stop on a resize.
/// tag   contract, nonallocating, nonthrowing
struct TickReport
{
    std::uint32_t       StandingWorkspace = 0u;      // [-]  - which roster entry is active after this tick
    PointerArbitration  Arbitrated        = {};      // [-]  - who held the pointer
    double              PacedInterval     = 0.0;     // [ms] - measured between the last two arrivals
    bool                RotationSkipped   = false;   // [-]  - no image was available to record into
    bool                ExtentReclaimed   = false;   // [-]  - the chain and every target were re-established
    bool                ClosureRequested  = false;   // [-]  - the artist asked the window system to close
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SEQUENCE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The whole host: `32` §1's ordered bring-up, `32` §2's ten-step tick, and teardown as the exact reverse.
/// note  🔴 `32` §1's order is ① `SlateMath` ② `WindowInterchange` ③ `SlateVulkan` ④ `SlateDocument`
///        ⑤ `SlateCompute` ⑥ `SlateUI` ⑦ `RenderSchedule`, and two of the steps are constrained by documents
///        rather than by convenience: ⑤ before ⑥ is `12` invariant 10, and ④ before ⑤ is `22` §1 — a transaction
///        addressing a surface position needs that position resolved before the compute that reads it stands.
/// note  🔴 Step ⑦ **validates and does not repair**. A schedule whose ordering cannot be derived is a refusal
///        naming the target, appended to the register; a bring-up that quietly reordered it would present the
///        artist with an image composited in an order no document describes.
/// note  🔴 Teardown is the exact reverse **with the rotation drained first**. Reclaiming a component while a
///        recording that reads it is still executing is a vendor object destroyed under the device, and the
///        report that follows names whichever object the driver noticed rather than the one that was early.
/// note  ⚠️ This component owns the bring-up and owns none of the documents. A workspace's own storage is the
///        application's, threaded through `WorkspaceSpecification::WorkspaceContext` and never held here.
/// tag   owning
class WorkspaceSequence
{
public:

    WorkspaceSequence()                                    = default;
    WorkspaceSequence(const WorkspaceSequence&)            = delete;
    WorkspaceSequence& operator=(const WorkspaceSequence&) = delete;
    ~WorkspaceSequence();

    /// 🧩 Brings the whole host up in `32` §1's order, refusing at the first step that declines.
    /// in    Declaring  [-]  the roster, the extent and the pacing intent
    /// out   Outcome    [-]  refuses with ContentUnsupported for an empty roster or a standing ordinal outside it,
    ///                       HostDenied when the window system or the interface declines, and whatever the
    ///                       declining step refused — carried verbatim, never re-spelled
    /// post  delivered leaves every step standing and the standing workspace's ledger declared; refused leaves
    ///       nothing standing — `Reclaim` has already run over whatever had been constructed
    /// note  🔴 Refused in full. A half-brought-up host is one whose teardown reclaims components that were never
    ///        constructed, and the vendor reports that as an invalid handle rather than as a bring-up that stopped.
    /// note  ⚠️ The refusal is the declining step's own, carried up unchanged. Re-spelling it here would put one
    ///        sentence in front of every distinct failure the seven steps can report.
    /// cost  🔴
    /// tag   api, nonthrowing
    Outcome<bool> Construct(const WorkspaceDeclaration& Declaring);

    /// 🧩 Advances one tick — `32` §2's ten steps, in order, exactly once each.
    /// out   Outcome  [-]  refuses with CapabilityAbsent before Construct delivered, and with DeviceLost when the
    ///                     device was lost; a skipped rotation and a re-established chain are delivered outcomes
    /// post  one image was presented, or the report names why none was
    /// note  🔴 The ten steps are ① drain the window system ② arbitrate the pointer ③ apply the document's pending
    ///        intent ④ advance the interface tick ⑤ present the bracket ⑥ await the rotation slot ⑦ take the
    ///        display image ⑧ record the schedule and the interface ⑨ surrender the recording ⑩ present the image.
    ///        The order is `32` §2's and the two constrained adjacencies are recorded on the class above.
    /// note  🔴 An arrived extent is applied in `06` §7's order — re-establish the chain, re-claim every
    ///        display-relative target at the same extent, then derive every attachment span over the re-claimed
    ///        views. Any other order presents one target at the extent the display had before the drag.
    /// note  ⚠️ Input carries **arrival** stamps and never consumption stamps. `22` reconstructs the path the
    ///        artist drew from them, and a sample restamped where it was consumed carries the display rate.
    /// cost  🔴
    /// tag   api, nonthrowing
    Outcome<TickReport> Advance();

    /// 🧩 Reclaims every step in the exact reverse of bring-up, with the rotation drained first.
    /// post  nothing stands; a second call does nothing
    /// note  🔴 The rotation is drained before the first reclaim and not between them. Draining per component would
    ///        idle the device seven times over a teardown, and the first drain is already sufficient — no recording
    ///        is opened after it.
    /// cost  🔴
    /// tag   api, nonthrowing
    void Reclaim();

    /// 🧩 Whether the artist has asked the window system to close the window.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    bool ClosureRequested() const;

    /// 🧩 The resolved theme every panel of this host reads.
    /// pre   Construct delivered
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    const ThemeSpecification& Theme() const;

    /// 🧩 The desk, for a host that seeds a second document or reads what the artist arranged.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    WorkspaceSpace&       Desk();
    const WorkspaceSpace& Desk() const;

    /// 🧩 The register every step of bring-up and every tick appends its refusals to.
    /// note  🔴 Held here rather than in the host so that `DiagnosticPanel` presents the same register the
    ///        bring-up wrote into. Two registers is one panel presenting an empty list beside a host that refused.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    ReportSequence&       Reports();
    const ReportSequence& Reports() const;

    /// 🧩 The measures the tick samples — the pacing, the rotation count, the arrival interval.
    /// note  🔴 `86` §10: sampled **by the tick** and never pushed by a producer. A producer that declared its own
    ///        measure would declare it at whatever rate it runs at, and the panel would present a figure whose
    ///        interval nothing states.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    MeasureIndex&       Measures();
    const MeasureIndex& Measures() const;

    /// 🧩 The monotonic timeline every arrival is stamped against.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    const TickSequence& Timeline() const;

    /// 🧩 The ledger the standing workspace declared, for the desk and for nothing that retains it.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    const PanelIndex& Declared() const;

    /// 🧩 The two edge drawers the standing workspace declared, on the same terms.
    /// note  ⚠️ A workspace declaring no drawer routine reports an index naming neither, which the bracket presents
    ///        as nothing at all. That is not a refusal — a workspace with nothing to browse has no bottom drawer.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    const DrawerIndex& DeclaredDrawers() const;

    /// 🧩 How many rotations have completed since bring-up.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    std::uint64_t CompletedRotations() const;

private:

    /// 🧩 Declares the standing workspace's panels into the ledger, emptying whatever the previous one left.
    /// note  🔴 The ledger is emptied before the arriving workspace declares into it. A ledger carried across an
    ///        activation presents the departing workspace's panels against a context it has already released.
    void DeclareStanding();

    /// 🧩 Applies an arrived display extent in `06` §7's order — chain, then targets, then attachment spans.
    Outcome<bool> ReclaimExtent(std::uint32_t ArrivedWidth, std::uint32_t ArrivedHeight);

    /// 🧩 Arbitrates the pointer in `14` §4.2's precedence, holding the claimant for the length of a drag.
    PointerArbitration Arbitrate();

    /// 🧩 Appends one refusal to the register, stamped at the arrival this tick was taken at.
    void Record(const char* Origin, const char* Subject, const Refusal& Declining);

    // -- ① SlateMath ----------------------------------------------------------------------------------------------------
    TickSequence         HostTimeline    = {};        // [-] - constructed first; every stamp is against it
    ReportSequence       Register        = {};        // [-] - every refusal from every step below
    MeasureIndex         Measured        = {};        // [-] - sampled by the tick, never pushed
    InputExchange        Pointing        = {};        // [-] - arrival-stamped device samples

    // -- ② WindowInterchange --------------------------------------------------------------------------------------------
    WindowInterchange    Window          = {};        // [-] - the native window; surrenders its handle and no more

    // -- ③ SlateVulkan --------------------------------------------------------------------------------------------------
    VulkanExchange       DeviceEdge      = {};        // [-] - the instance and the created device
    DiagnosticExtension  Naming          = {};        // [-] - names every vendor object the steps below construct
    VkSurfaceKHR         DisplaySurface  = VK_NULL_HANDLE;   // [-] - converted from the window's handle
    ByteSpace            BackingSpace    = {};        // [-] - where every claimed image's bytes come from
    ImageSpace           Images          = {};        // [-] - the claimed images themselves
    TargetSpace          Targets         = {};        // [-] - the fifteen declared targets
    AttachmentIndex      Attachments     = {};        // [-] - the constructs and the spans over them
    CycleScheduler       Rotation        = {};        // [-] - the cyclic ordering points
    CommandSequence      Recordings      = {};        // [-] - one primary recording per rotation slot
    DisplayScheduler     Display         = {};        // [-] - the presentation chain and its pacing

    // -- ⑥ SlateUI -----------------------------------------------------------------------------------------------------
    InterfaceExchange    Interface       = {};        // [-] - the one seam the interface library crosses
    ThemeSpecification   ActiveTheme     = {};        // [-] - resolved once, at the declared density
    WorkspaceSpace       Space           = {};        // [-] - the desk
    PanelIndex           Ledger          = {};        // [-] - the standing workspace's declarations
    DrawerIndex          EdgeDrawers     = {};        // [-] - and its two edge drawers, above every panel of them

    // -- ⑦ RenderSchedule ---------------------------------------------------------------------------------------------
    RenderSchedule       Schedule        = {};        // [-] - validated at bring-up, never repaired

    // -- what the roster and the arbitration carry ---------------------------------------------------------------------
    const WorkspaceSpecification*  Roster        = nullptr;   // [-] - the application's; never owned
    std::uint32_t                  RosterCount   = 0u;        // [-] - how many were registered
    std::uint32_t                  StandingEntry = 0u;        // [-] - which one is active
    PointerArbitration             Arbitrated    = {};        // [-] - carried across ticks for the drag's length
    bool                           SequenceStanding = false;  // [-] - Construct delivered and Reclaim has not
};

// 📐 Identities, rotation ordinals and report counts are Exact. The pacing interval and every rectangle are
//    Bounded. The component claims Bounded, per `00` §3's transitivity rule.
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded, PrecisionGuarantee::Exact);

}   // namespace Slate
