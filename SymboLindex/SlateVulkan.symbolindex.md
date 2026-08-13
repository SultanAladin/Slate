//============================================================================================================================================
//                                                          SLATEVULKAN.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Symbol roll for SlateVulkan — The classic render constructs `06` §2.1 settled on, declared over the shared targets and re-derived on an extent change.

%format   symbolindex 1.0
%scope    layer
%path     Engine/SlateVulkan
%folders  32
%symbols  369

//------------------------------------------------------------------------------------------------------------------------
//                                                     FOLDER INDEXES
//------------------------------------------------------------------------------------------------------------------------

I Api    | Api/Api.symbolindex       | 17 sym | The classic render constructs `06` §2.1 settled on, declared over the shared targets and re-derived on an extent change.
I Source | Source/Source.symbolindex | 11 sym | The construct declared from the claimed formats, the span derived over the claimed views, and the two reclamations.
I Api    | Api/Api.symbolindex       | 19 sym | Raw device byte extents, claimed in a few large pieces and sliced into the spans every resource sits in.
I Source | Source/Source.symbolindex | 12 sym | Residency scoring, the first-fit slice, and the coalescing release that keeps an extent from fragmenting away.
I Api    | Api/Api.symbolindex       | 11 sym | One recording per rotation slot — where commands are written, and the ordered surrender of them to the queue.
I Source | Source/Source.symbolindex | 8 sym  | The per-slot recording extents, the open that resets one whole, and the surrender to the one graphics queue.
I Api    | Api/Api.symbolindex       | 11 sym | Orders reuse of N cyclic recording slots — the wait that makes a slot writable and the ordinal that names it.
I Source | Source/Source.symbolindex | 9 sym  | The ordering points of every cyclic slot, the bounded wait that reclaims one, and the advance that cycles them.
I Api    | Api/Api.symbolindex       | 18 sym | Descriptor set layouts constructed once at bring-up, and explicit sets claimed one per rotation slot.
I Source | Source/Source.symbolindex | 12 sym | The layout declaration that closes at bring-up, the extent it is sized against, and the per-rotation write.
I Api    | Api/Api.symbolindex       | 10 sym | `VK_EXT_debug_utils` — queried, enabled and held, so every device object Slate creates carries a name.
I Source | Source/Source.symbolindex | 8 sym  | The loader resolution, the attached sink, the arrival that appends, and the per-object name.
I Api    | Api/Api.symbolindex       | 19 sym | The presentation chain and the pacing of it — image transitions ordered against a declared latency target.
I Source | Source/Source.symbolindex | 15 sym | The scored format, the established chain, the ordered arrival, and the surrender back to the display.
I Api    | Api/Api.symbolindex       | 17 sym | Hardware execution duration, measured between recorded timestamps — and reported unavailable rather than zero.
I Source | Source/Source.symbolindex | 13 sym | The timestamp extent, the recorded pair around each declared span, and the readback that reports or refuses.
I Api    | Api/Api.symbolindex       | 19 sym | Device image extents — claimed against a declared shape, viewed once, and carrying the layout each one stands in.
I Source | Source/Source.symbolindex | 14 sym | The image claim, the one place a layout transition is recorded, and the reclamation that returns both.
I Api    | Api/Api.symbolindex       | 15 sym | Graphics and compute programs constructed once at bring-up, against the layouts and modules already declared.
I Source | Source/Source.symbolindex | 8 sym  | The layout every program reaches through, the two construction routes, and the reclamation that returns both.
I Api    | Api/Api.symbolindex       | 16 sym | What is recorded in a rotation slot, in what order, and against which shared targets.
I Source | Source/Source.symbolindex | 10 sym | Contribution gating and the ordering derived from declared reads and writes.
I Api    | Api/Api.symbolindex       | 13 sym | Lowered shader streams — read once, verified as SPIR-V, held as vendor modules and specialised at construction.
I Source | Source/Source.symbolindex | 7 sym  | The whole-file read, the stream verification that refuses before the vendor sees it, and the held specialisation.
I Api    | Api/Api.symbolindex       | 18 sym | Device linear extents, each sliced out of ByteSpace and each declaring what the device is permitted to read it as.
I Source | Source/Source.symbolindex | 12 sym | The claim, the host write, the recorded transfer and the release of every linear device extent the engine holds.
I Api    | Api/Api.symbolindex       | 2 sym  | Scores vendor implementations into a capability set, once, at bring-up and at recovery.
I Source | Source/Source.symbolindex | 1 sym  | Enumerated device scored into a capability set and a ranking.
I Api    | Api/Api.symbolindex       | 11 sym | Loader C-ABI, instance and device handles crossing the vendor edge.
I Source | Source/Source.symbolindex | 9 sym  | Instance construction, device scoring and the one graphics queue.
I Api    | Api/Api.symbolindex       | 2 sym  | Native window handle ⇄ VkSurfaceKHR — the one place the window system meets the vendor edge.
I Source | Source/Source.symbolindex | 2 sym  | The surface conversion, taken through the window system that produced the handle.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

V AbsentConstruct                           | Api/AttachmentIndex.h          | 26      | ?
V DepthTargetAbsent                         | Api/AttachmentIndex.h          | 31      | ?
T ConstructDeclaration                      | Api/AttachmentIndex.h          | 42-46   | One render construct, as the recording that draws through it declares it. this run lists third writes it to another target entirely, and both targets then carry plausible imagery — which is the reason the run is declared rather than derived from the produced set, whose order is the contributing document's convenience. ordinal of a `SharedTarget` otherwise, and `RelationOfTarget` is not consulted here — every target a construct spans stands at the display extent by declaration.
T ConstructedSpan                           | Api/AttachmentIndex.h          | 57-63   | What a recording is handed — the construct it opens, the span it opens over, and that span's extent. ordinates it records and a second derivation of one extent is `00` §2's case. It is the extent the last `Derive` was performed against and not what the display currently reports.
T AttachmentIndex                           | Api/AttachmentIndex.h          | 81-177  | Every render construct the engine declares, and the span each one covers the claimed targets with. constructed against a construct declared here, and a recording reaching for a dynamic rendering declaration instead has taken a decision `06` already took. vendor performs no implicit transition. `ImageSpace::Transition` stays the one place a layout changes — a construct that transitioned on its own would leave `ImageSpace`'s record naming a layout the image is not in, and the next barrier would then be issued from the wrong one. display-relative target on a resize, which invalidates every view a span was derived over; the construct describes formats alone and is untouched. `Derive` is what re-covers them.
F AttachmentIndex::~AttachmentIndex         | Api/AttachmentIndex.h          | 88      | ?
F AttachmentIndex::Construct                | Api/AttachmentIndex.h          | 97      | Takes the device and the claimed target set every construct is declared over.
F AttachmentIndex::Declare                  | Api/AttachmentIndex.h          | 109     | Declares one render construct, returning the ordinal every later resolution names it by. naming an unclaimed target, and with HostDenied when the device declines it The table is what `TargetSpace` claimed against, and re-reading it here would let a construct and a claim come to disagree about one target's format with nothing comparing them.
F AttachmentIndex::Derive                   | Api/AttachmentIndex.h          | 123     | Covers every declared construct's targets at one display extent, replacing whatever stood before. and with HostDenied when the device declines a span `06` §7's gate is that no persistent extent survives a resize, and a span retained because its construct "looked unaffected" is exactly such an extent.
F AttachmentIndex::Resolve                  | Api/AttachmentIndex.h          | 131     | The construct and the span one ordinal names, for the recording that opens it. with ExtentExhausted before Derive has covered it
F AttachmentIndex::ConstructOf              | Api/AttachmentIndex.h          | 140     | The construct alone, for `ProgramIndex` constructing a program before any span is derived. is known, and only the formats enter that construction. Requiring a derived span to construct a program would order the two the wrong way round.
F AttachmentIndex::Surrender                | Api/AttachmentIndex.h          | 146     | Destroys every span and leaves the constructs standing, ahead of a re-claim at a new extent.
F AttachmentIndex::Reclaim                  | Api/AttachmentIndex.h          | 152     | Destroys every span and every construct.
F AttachmentIndex::DeclaredCount            | Api/AttachmentIndex.h          | 154     | ?
F AttachmentIndex::SpansDerived             | Api/AttachmentIndex.h          | 155     | ?
T AttachmentIndex::HeldConstruct            | Api/AttachmentIndex.h          | 159-165 | ?
F AttachmentIndex::LayoutOf                 | Api/AttachmentIndex.h          | 169     | Where one attachment stands throughout the construct, so that the vendor transitions nothing.
F AttachmentIndex::LayoutOf                 | Source/AttachmentIndex.cpp     | 15-19   | ?
F AttachmentIndex::Construct                | Source/AttachmentIndex.cpp     | 21-30   | ?
F AttachmentIndex::Declare                  | Source/AttachmentIndex.cpp     | 36-151  | ?
F AttachmentIndex::Derive                   | Source/AttachmentIndex.cpp     | 157-242 | ?
F AttachmentIndex::Resolve                  | Source/AttachmentIndex.cpp     | 248-271 | ?
F AttachmentIndex::ConstructOf              | Source/AttachmentIndex.cpp     | 273-282 | ?
F AttachmentIndex::DeclaredCount            | Source/AttachmentIndex.cpp     | 284-287 | ?
F AttachmentIndex::SpansDerived             | Source/AttachmentIndex.cpp     | 289-292 | ?
F AttachmentIndex::Surrender                | Source/AttachmentIndex.cpp     | 298-316 | ?
F AttachmentIndex::Reclaim                  | Source/AttachmentIndex.cpp     | 318-336 | ?
F AttachmentIndex::~AttachmentIndex         | Source/AttachmentIndex.cpp     | 338-341 | ?
V AbsentExtent                              | Api/ByteSpace.h                | 26      | ?
E ExtentResidency                           | Api/ByteSpace.h                | 33-38   | Where a claimed span lives, which is the only distinction the caller makes. device declares, and it differs per device, per driver and per configuration. A caller naming the vendor ordinal directly has hard-coded one machine's declaration into a claim site.
E ClaimStanding                             | Api/ByteSpace.h                | 46-51   | What the claimant promises about the span, and therefore what exhaustion means for it. exhaustion is residency policy rather than a reported failure. The distinction cannot be derived from the span — a hundred megabytes is the working set for one caller and an optional prefetch for the next — so it is declared at the claim and carried into the refusal.
T ByteClaim                                 | Api/ByteSpace.h                | 61-68   | One sliced byte span — where it sits, how far it runs, and which extent it came out of. is a no-op, which is what makes the caller's reclamation path unconditional.
V DeviceLocalExtentBytes                    | Api/ByteSpace.h                | 77      | ?
V HostWritableExtentBytes                   | Api/ByteSpace.h                | 78      | ?
T ByteSpace                                 | Api/ByteSpace.h                | 88-182  | Every device byte the engine holds, sliced from a small number of large vendor allocations. span is `16`'s; both arrive here as a span and an alignment and leave as an offset. with its neighbours, but no extent is ever handed back to the vendor until Reclaim — an extent returned while any claim still stands is a use-after-free the validation layer reports somewhere else entirely.
F ByteSpace::~ByteSpace                     | Api/ByteSpace.h                | 95      | ?
F ByteSpace::Construct                      | Api/ByteSpace.h                | 107     | Takes the device and reads the vendor declaration every later claim is scored against. by whoever happened to call `ConstructExtent` would be absent for the extents this component takes on its own, which is every extent after the first of each residency.
F ByteSpace::Claim                          | Api/ByteSpace.h                | 120     | Slices one span of the requested residency, taking a further extent when none can satisfy it. cannot use and cannot release, and the release path is the one nobody exercises.
F ByteSpace::Release                        | Api/ByteSpace.h                | 133     | Returns one span to its extent's free list, coalescing it with whatever it now adjoins. quarantined by its **owner** — `20` §5 does exactly that over its own slots — because only the owner knows which rotation last recorded against it.
F ByteSpace::Reclaim                        | Api/ByteSpace.h                | 139     | Destroys every vendor allocation and forgets every slice.
F ByteSpace::ClaimedBytes                   | Api/ByteSpace.h                | 144     | What is claimed and what is held, per residency — the two halves `86` reports separately.
F ByteSpace::BackingBytes                   | Api/ByteSpace.h                | 145     | ?
F ByteSpace::ExtentCount                    | Api/ByteSpace.h                | 146     | ?
T ByteSpace::FreeSpan                       | Api/ByteSpace.h                | 152-156 | ?
T ByteSpace::SlicedExtent                   | Api/ByteSpace.h                | 158-167 | ?
F ByteSpace::ClassifyResidency              | Api/ByteSpace.h                | 171     | Scores what the device declares for the one entry that satisfies a residency.
F ByteSpace::ConstructExtent                | Api/ByteSpace.h                | 175     | Takes one further vendor allocation, at least as large as the span that could not be satisfied.
F PowerOfTwo                                | Source/ByteSpace.cpp           | 20-23   | ?
F RaiseToAlignment                          | Source/ByteSpace.cpp           | 25-28   | ?
F ByteSpace::Construct                      | Source/ByteSpace.cpp           | 35-57   | ?
F ByteSpace::ClassifyResidency              | Source/ByteSpace.cpp           | 63-91   | ?
F ByteSpace::ConstructExtent                | Source/ByteSpace.cpp           | 97-171  | ?
F ByteSpace::Claim                          | Source/ByteSpace.cpp           | 177-278 | ?
F ByteSpace::Release                        | Source/ByteSpace.cpp           | 284-330 | ?
F ByteSpace::Reclaim                        | Source/ByteSpace.cpp           | 332-356 | ?
F ByteSpace::~ByteSpace                     | Source/ByteSpace.cpp           | 358-361 | ?
F ByteSpace::ClaimedBytes                   | Source/ByteSpace.cpp           | 367-378 | ?
F ByteSpace::BackingBytes                   | Source/ByteSpace.cpp           | 380-391 | ?
F ByteSpace::ExtentCount                    | Source/ByteSpace.cpp           | 393-396 | ?
T SurrenderOrdering                         | Api/CommandSequence.h          | 31-37   | What one surrender to the queue waits on and what it signals. that waits at the top of the ordering serialises against a point it only needs before it writes colour, and the display stall that produces reads as a device too slow for the extent.
T CommandSequence                           | Api/CommandSequence.h          | 51-142  | The rotation-deep recordings every contributing document writes its commands into. rather than a queue arbitration. `08` §3's diagram is therefore the submission order verbatim, and nothing here reorders what `RenderSchedule::Ordered` fixed. vendor a per-recording allocator it must then keep, and `06` §7 sizes every per-recording resource against the depth precisely so the whole slot can be reset at once.
F CommandSequence::~CommandSequence         | Api/CommandSequence.h          | 58      | ?
F CommandSequence::Construct                | Api/CommandSequence.h          | 71      | Constructs the per-slot recording extents and the one primary recording each holds. device declines an extent or a recording; refused in full driver's text needs to say — a report against an unnamed recording cannot distinguish the slot being written from the one the device is still executing, and that pair is the whole rotation.
F CommandSequence::Open                     | Api/CommandSequence.h          | 81      | Resets one rotation slot's recording extent and opens its recording for writing. and HostDenied when the device declines the reset or the open
F CommandSequence::Recording                | Api/CommandSequence.h          | 87      | The recording one rotation slot holds, for a document contributing commands to an open slot.
F CommandSequence::Surrender                | Api/CommandSequence.h          | 101     | Closes one rotation slot's recording and surrenders it to the one graphics queue. the device declines the close or the surrender, and DeviceLost when the device was lost; the slot is closed and nothing is destroyed either way — a component clearing an ordering point it does not own is one that clears it at the wrong moment for every other reader of it.
F CommandSequence::OpenImmediate            | Api/CommandSequence.h          | 109     | Opens a recording outside the rotation, for the one-off transfers bring-up records. a rotation's — an immediate wait inside a rotation is the whole device serialised on the host.
F CommandSequence::SurrenderImmediate       | Api/CommandSequence.h          | 117     | Closes an immediate recording, surrenders it, waits for it, and returns it. DeviceLost when the device was lost; the recording is returned either way
F CommandSequence::Reclaim                  | Api/CommandSequence.h          | 123     | Destroys every recording and every extent.
T CommandSequence::RecordingSlot            | Api/CommandSequence.h          | 127-132 | ?
F CommandSequence::Construct                | Source/CommandSequence.cpp     | 15-93   | ?
F CommandSequence::Open                     | Source/CommandSequence.cpp     | 99-133  | ?
F CommandSequence::Recording                | Source/CommandSequence.cpp     | 135-147 | ?
F CommandSequence::Surrender                | Source/CommandSequence.cpp     | 153-204 | ?
F CommandSequence::OpenImmediate            | Source/CommandSequence.cpp     | 210-247 | ?
F CommandSequence::SurrenderImmediate       | Source/CommandSequence.cpp     | 249-317 | ?
F CommandSequence::Reclaim                  | Source/CommandSequence.cpp     | 323-351 | ?
F CommandSequence::~CommandSequence         | Source/CommandSequence.cpp     | 353-356 | ?
T RotationSlot                              | Api/CycleScheduler.h           | 30-35   | What one cyclic slot holds — the ordering points a recording against it waits on and signals. ever been submitted, and an unsignalled one waits for a submission that will never arrive — a bring-up that stops before its first image, with no operand and no error.
T CycleScheduler                            | Api/CycleScheduler.h           | 48-128  | The cyclic ordering every per-rotation resource is sized against and every recording is written into. is declared in `Contract/` because `SlateVulkan` sizes against it and `SlateCompute` quarantines against it — one number, two units, and the depth is 🚧 open at `06` §9 between two and three. advancing it separately produces two rotations that agree for exactly as long as nothing refuses.
F CycleScheduler::~CycleScheduler           | Api/CycleScheduler.h           | 55      | ?
F CycleScheduler::Construct                 | Api/CycleScheduler.h           | 68      | Constructs the ordering points for every slot in the depth. device declines an ordering point; refused in full, with nothing half-constructed orders, because a stall reports one waiter against one signaller and the two are indistinguishable by address — an unnamed rotation makes every deadlock report the same sentence.
F CycleScheduler::Await                     | Api/CycleScheduler.h           | 79      | Waits until the slot the standing ordinal names is no longer read, and makes it writable again. with DeviceLost when the device was lost; nothing is destroyed either way host that stops with no report, and `06` §7 requires the loss to be reported upward before anything is destroyed — which cannot happen from inside a wait that never returns.
F CycleScheduler::Arm                       | Api/CycleScheduler.h           | 86      | Clears the completion of the standing slot, immediately before the submission that signals it.
F CycleScheduler::Advance                   | Api/CycleScheduler.h           | 92      | Carries the standing ordinal to the next slot in the cycle.
F CycleScheduler::Standing                  | Api/CycleScheduler.h           | 98      | The slot the standing ordinal names, for the recording and the display that read it.
F CycleScheduler::StandingOrdinal           | Api/CycleScheduler.h           | 103     | Which slot of the depth is standing — what every per-rotation claim is addressed by.
F CycleScheduler::CompletedRotations        | Api/CycleScheduler.h           | 108     | How many rotations have been advanced through since bring-up, for `86`'s pacing report.
F CycleScheduler::Reclaim                   | Api/CycleScheduler.h           | 114     | Destroys every ordering point.
F CycleScheduler::Construct                 | Source/CycleScheduler.cpp      | 15-82   | ?
F CycleScheduler::Await                     | Source/CycleScheduler.cpp      | 88-115  | ?
F CycleScheduler::Arm                       | Source/CycleScheduler.cpp      | 117-129 | ?
F CycleScheduler::Advance                   | Source/CycleScheduler.cpp      | 135-142 | ?
F CycleScheduler::Standing                  | Source/CycleScheduler.cpp      | 144-150 | ?
F CycleScheduler::StandingOrdinal           | Source/CycleScheduler.cpp      | 152-155 | ?
F CycleScheduler::CompletedRotations        | Source/CycleScheduler.cpp      | 157-160 | ?
F CycleScheduler::Reclaim                   | Source/CycleScheduler.cpp      | 166-196 | ?
F CycleScheduler::~CycleScheduler           | Source/CycleScheduler.cpp      | 198-201 | ?
V AbsentDescriptor                          | Api/DescriptorIndex.h          | 26      | ?
T DescriptorSlot                            | Api/DescriptorIndex.h          | 32-38   | What one descriptor slot in a layout carries, as the shader declares it. layout must state, and a layout stating a larger one leaves the shader indexing beyond what is written.
T DescriptorContent                         | Api/DescriptorIndex.h          | 45-54   | What one descriptor set is written with — one entry per slot the recording amends. sampled image into a slot the layout declares as a span is a validation error at the write rather than at the declaration that disagreed.
T DescriptorIndex                           | Api/DescriptorIndex.h          | 68-172  | Every descriptor set layout the engine declares, and the rotation-deep sets claimed against them. declared at bring-up, and `Declare` refuses once `Fix` has been resolved — the gate is a refusal at the call rather than a remark in a review. therefore yields `RecordingRotationDepth` sets, and the recording writes the one its slot names — amending a set the device is still reading is the defect the depth exists to remove.
F DescriptorIndex::~DescriptorIndex         | Api/DescriptorIndex.h          | 75      | ?
F DescriptorIndex::Construct                | Api/DescriptorIndex.h          | 87      | Takes the device against which every layout and every set is constructed. often — every content mismatch is reported against the set rather than against the recording that bound it — so an unnamed set turns each of those reports into an address the reader must resolve.
F DescriptorIndex::Declare                  | Api/DescriptorIndex.h          | 95      | Declares one layout from its slots, returning the ordinal every later claim names it by. and with RelationCyclic once the declaration set has been fixed
F DescriptorIndex::Fix                      | Api/DescriptorIndex.h          | 105     | Closes the declaration and constructs the one descriptor extent every later claim is sliced from. that reallocates invalidates every set sliced from it, including the ones a rotation still reads.
F DescriptorIndex::Claim                    | Api/DescriptorIndex.h          | 113     | Claims one set per rotation slot against a declared layout, returning the claim's ordinal.
F DescriptorIndex::Amend                    | Api/DescriptorIndex.h          | 124     | Writes the content of one claimed set for one rotation slot. or above the depth, or a slot the layout does not declare
F DescriptorIndex::Resolve                  | Api/DescriptorIndex.h          | 132     | The set one claim names for one rotation slot, for the recording that reads it.
F DescriptorIndex::Layout                   | Api/DescriptorIndex.h          | 138     | The layout one ordinal names, for the recording that constructs a program against it.
F DescriptorIndex::Reclaim                  | Api/DescriptorIndex.h          | 144     | Destroys every set, every layout and the extent they were sliced from.
F DescriptorIndex::DeclaredCount            | Api/DescriptorIndex.h          | 146     | ?
F DescriptorIndex::ClaimedCount             | Api/DescriptorIndex.h          | 147     | ?
T DescriptorIndex::DeclaredLayout           | Api/DescriptorIndex.h          | 151-155 | ?
T DescriptorIndex::ClaimedSet               | Api/DescriptorIndex.h          | 157-161 | ?
F DescriptorIndex::SlotOf                   | Api/DescriptorIndex.h          | 164     | Which declared slot carries an ordinal, or nothing when the layout does not declare it.
F DescriptorIndex::Construct                | Source/DescriptorIndex.cpp     | 15-24   | ?
F DescriptorIndex::Declare                  | Source/DescriptorIndex.cpp     | 30-103  | ?
F DescriptorIndex::Fix                      | Source/DescriptorIndex.cpp     | 109-179 | ?
F DescriptorIndex::Claim                    | Source/DescriptorIndex.cpp     | 185-232 | ?
F DescriptorIndex::SlotOf                   | Source/DescriptorIndex.cpp     | 238-247 | ?
F DescriptorIndex::Amend                    | Source/DescriptorIndex.cpp     | 249-342 | ?
F DescriptorIndex::Resolve                  | Source/DescriptorIndex.cpp     | 348-360 | ?
F DescriptorIndex::Layout                   | Source/DescriptorIndex.cpp     | 362-371 | ?
F DescriptorIndex::DeclaredCount            | Source/DescriptorIndex.cpp     | 373-376 | ?
F DescriptorIndex::ClaimedCount             | Source/DescriptorIndex.cpp     | 378-381 | ?
F DescriptorIndex::Reclaim                  | Source/DescriptorIndex.cpp     | 387-414 | ?
F DescriptorIndex::~DescriptorIndex         | Source/DescriptorIndex.cpp     | 416-419 | ?
T DiagnosticExtension                       | Api/DiagnosticExtension.h      | 37-156  | The one optional vendor capability `06` negotiates by name — the driver's own diagnostic text, and the per-object naming that makes that text nameable. vendor capability. `VulkanExchange::ConstructInstance` requests the extension and the validation layer; nothing there holds what the request produced, and this holds it. Release is one whose every call site is conditional, and `06` §1's note on the capability set applies verbatim — those conditionals never all leave. `Construct` refuses instead when nothing negotiated it, and `Declare` delivers as a no-op, so no call site branches on the configuration. instance-level, and a sink attached after the device exists cannot report what device creation itself rejected — which is the one message worth having on a machine where bring-up refuses.
F DiagnosticExtension::~DiagnosticExtension | Api/DiagnosticExtension.h      | 44      | ?
F DiagnosticExtension::Construct            | Api/DiagnosticExtension.h      | 59      | Resolves the capability's entry points and attaches the sink the driver writes its diagnostic text into. declare the capability, and with HostDenied when the driver declines the sink optional by declaration, so its symbols are absent from the import library on a machine whose loader does not carry it — and a link-time reference makes the whole executable unloadable there.
F DiagnosticExtension::Declare              | Api/DiagnosticExtension.h      | 82      | Names one vendor object, so the driver's text names the object rather than an address. the driver declines the name discharged at each **claim site**, not here. This is the one mechanism that can name an object; the gate is met only once `ByteSpace`, `ImageSpace`, `SpanSpace`, `DescriptorIndex`, `ProgramIndex`, `CommandSequence`, `CycleScheduler` and `DisplayScheduler` each call it. with no diagnostic capability is not a failure, and refusing would make every claim site branch on the configuration to ignore a refusal it expected. third-party exemption and beside the existing `WindowExchange::Convert(…, void* NativeHandle)`. The vendor's own field is `objectHandle` and one spelling at the edge is what the exemption is for. holds this as a borrowed const edge beside the device, and a non-const one would make every such component hold a mutable reference to a capability it only reads.
F DiagnosticExtension::Declare              | Api/DiagnosticExtension.h      | 99      | Names one vendor object by a static prefix and the ordinal its owning component holds it at. composed text does not fit the extent it is composed in own ordinal, and eight separate compositions is eight places where one of them formats the ordinal differently and the driver's text stops sorting alongside the rest. the two-operand form's contract already admits — the driver copies the text and nothing is retained here.
F DiagnosticExtension::Negotiated           | Api/DiagnosticExtension.h      | 108     | Whether the capability was negotiated, for a claim site reporting what it could not name.
F DiagnosticExtension::ArrivalCount         | Api/DiagnosticExtension.h      | 115     | How many diagnostic arrivals the driver has reported since the sink attached. arrival count is what says whether a quiet register means a clean run or a detached sink.
F DiagnosticExtension::Reclaim              | Api/DiagnosticExtension.h      | 121     | Detaches the sink and forgets every resolved entry point.
F DiagnosticExtension::Arrival              | Api/DiagnosticExtension.h      | 135     | The C-ABI arrival the driver calls, which forwards to the register the construction was given. was executing on. Nothing here allocates and nothing here takes a lock the driver does not already hold — the register's own guard is the only one. declares seven dispositions and none of them is "warning": an error and a validation warning are each a defect in Slate's own use of the vendor rather than normal operation, and `86` §5 — not a mapping here — decides what is presented as a problem. Only those two severities are subscribed to; a bounded register filled with information arrivals is one in which the error that mattered has already been discarded.
T DiagnosticExtension::ArrivalForwarding    | Api/DiagnosticExtension.h      | 142-147 | ?
F DiagnosticExtension::Construct            | Source/DiagnosticExtension.cpp | 17-79   | ?
F DiagnosticExtension::Arrival              | Source/DiagnosticExtension.cpp | 85-131  | ?
F DiagnosticExtension::Declare              | Source/DiagnosticExtension.cpp | 137-161 | ?
F DiagnosticExtension::Declare              | Source/DiagnosticExtension.cpp | 163-189 | ?
F DiagnosticExtension::Negotiated           | Source/DiagnosticExtension.cpp | 195-198 | ?
F DiagnosticExtension::ArrivalCount         | Source/DiagnosticExtension.cpp | 200-203 | ?
F DiagnosticExtension::Reclaim              | Source/DiagnosticExtension.cpp | 209-227 | ?
F DiagnosticExtension::~DiagnosticExtension | Source/DiagnosticExtension.cpp | 229-232 | ?
V AbsentDisplayImage                        | Api/DisplayScheduler.h         | 28      | ?
E LatencyIntent                             | Api/DisplayScheduler.h         | 38-43   | What the artist is optimising for, which is what the pacing is chosen against. owns the choice. The choice is declared here as two named intents rather than as a vendor mode, because the vendor's modes are three per driver and not all of them are present on every device — a caller naming one directly has named something a second machine declines. halfway down it, and the tear is read as a defect in the brush rather than as an absent wait.
T ArrivedImage                              | Api/DisplayScheduler.h         | 50-57   | What one arrival delivers — which image the recording writes and how far behind the display it is. image this rotation, and the caller re-claims **after** presenting it; refusing instead would drop the rotation that noticed, and the artist sees the resize as one stalled stroke.
T DisplayScheduler                          | Api/DisplayScheduler.h         | 78-252  | The chain of display images, the surface format every target is claimed against, and the pacing. stood, `08` §2's `DisplaySurface` named a target with no format to claim it at: `TargetSpace::Claim` takes the display format as an operand, and `Carries` is where that operand comes from. Re-deriving it at the claim site would let the chain and the targets disagree about one format with nothing comparing them. by the display and awaited by the recording, `RecordingDone` signalled by the recording and awaited by the display. Nothing here constructs an ordering point of its own; a second set would be one the rotation does not know it is waiting on. independent — the rotation is how many recordings the host may write ahead, the chain is how many images the display holds — and a claim sized against the wrong one is a per-rotation resource that aliases on whichever driver reports a different count.
F DisplayScheduler::~DisplayScheduler       | Api/DisplayScheduler.h         | 90      | ?
F DisplayScheduler::Construct               | Api/DisplayScheduler.h         | 112     | Establishes the chain against one surface, at one extent, for one declared latency intent. no format, ContentUnsupported for a zero or excessive extent, and ExtentExhausted when the device declines the chain the display has not handed back vendor holding images nothing references, and they are returned only when the surface is. re-establishes too. The chain carries the establishment ordinal rather than no ordinal, because two chains stand at once for the length of a resize — the retiring one is named as `oldSwapchain` while the arriving one is constructed — and a report against either would otherwise read alike.
F DisplayScheduler::Reclaim                 | Api/DisplayScheduler.h         | 133     | Re-establishes the chain at a new extent, retaining the format the targets were claimed at. here, then `TargetSpace::Reclaim` at the same extent, then `AttachmentIndex::Derive` over the re-claimed views. A chain re-established without the targets following is a display image at the arrived extent composited from targets at the previous one, which reads as a shifted image rather than as a resize that was half applied. stops rotating instead of establishing a chain no image can be claimed from.
F DisplayScheduler::Await                   | Api/DisplayScheduler.h         | 153     | Takes the next display image, ordering its arrival against the standing rotation slot. already taken, HostDenied when the display neither delivers an image nor reports the chain outgrown within the arrival ceiling, and DeviceLost when the device was lost; the chain is left standing for the recovery to reclaim writes colour. An arrival ordered on nothing is a recording that writes an image the display is still reading, and the artist sees the previous rotation's stroke tear through this one's. `ImageOrdinal` is what says which. A chain the display has merely outgrown delivers a usable image — present it, then re-establish. A chain it has retired delivers `AbsentDisplayImage` and no view: there is nothing to record into, so the caller re-establishes and skips the rotation. A caller reading `Reclaimed` alone would record into a null view on the second of the two.
F DisplayScheduler::Present                 | Api/DisplayScheduler.h         | 169     | Surrenders one taken image back to the display, ordered behind the recording that wrote it. HostDenied when the display declines it, and with DeviceLost when the device was lost; the ordinal is released to the display either way on it here would serialise the host against the device once per rotation — which is the whole purpose of the rotation depth, spent. was presented either way, and refusing would make the caller treat a resize as a lost rotation.
F DisplayScheduler::Carries                 | Api/DisplayScheduler.h         | 177     | What the surface carries, which is the format every display-relative target is claimed at. the format of the display rather than a declared one, and this is the only place that is known.
F DisplayScheduler::StandingWidth           | Api/DisplayScheduler.h         | 182     | The extent the chain stands at, which every display-relative target is claimed against.
F DisplayScheduler::StandingHeight          | Api/DisplayScheduler.h         | 183     | ?
F DisplayScheduler::ChainDepth              | Api/DisplayScheduler.h         | 188     | How many images the chain holds — the vendor's count, never the rotation depth.
F DisplayScheduler::PacedInterval           | Api/DisplayScheduler.h         | 197     | The interval between the last two arrivals, for the pacing report `86` presents. which is the quantity the latency intent was chosen against; `HardwareMetrics` measures what the device spent, and the two answer different questions.
F DisplayScheduler::Presented               | Api/DisplayScheduler.h         | 202     | How many images have been surrendered to the display since the chain was established.
F DisplayScheduler::Surrender               | Api/DisplayScheduler.h         | 210     | Destroys every view and the chain, and forgets the extent it stood at. that destroyed the surface it was established against would reclaim what it borrowed.
F DisplayScheduler::ScoreFormat             | Api/DisplayScheduler.h         | 225     | Scores the surface's declared formats and takes the one `08` §2's display target is claimed at. `08` §3 ⑧ is exposure, tone map and OETF in one recording — and a sRGB surface would apply it a second time in hardware, which the artist reads as a washed-out surface rather than as a double encoding.
F DisplayScheduler::ScorePacing             | Api/DisplayScheduler.h         | 230     | The vendor pacing one declared latency intent resolves to, from what the surface admits. intents and nothing here refuses over an absent mode.
F DisplayScheduler::Establish               | Api/DisplayScheduler.h         | 233     | Establishes the chain and its views at the standing extent, retiring whatever stood before.
F DisplayScheduler::ScoreFormat             | Source/DisplayScheduler.cpp    | 15-42   | ?
F DisplayScheduler::ScorePacing             | Source/DisplayScheduler.cpp    | 44-74   | ?
F DisplayScheduler::Construct               | Source/DisplayScheduler.cpp    | 80-108  | ?
F DisplayScheduler::Reclaim                 | Source/DisplayScheduler.cpp    | 110-123 | ?
F DisplayScheduler::Establish               | Source/DisplayScheduler.cpp    | 125-283 | ?
F DisplayScheduler::Await                   | Source/DisplayScheduler.cpp    | 289-367 | ?
F DisplayScheduler::Present                 | Source/DisplayScheduler.cpp    | 373-411 | ?
F DisplayScheduler::Carries                 | Source/DisplayScheduler.cpp    | 417     | ?
F DisplayScheduler::StandingWidth           | Source/DisplayScheduler.cpp    | 418     | ?
F DisplayScheduler::StandingHeight          | Source/DisplayScheduler.cpp    | 419     | ?
F DisplayScheduler::PacedInterval           | Source/DisplayScheduler.cpp    | 420     | ?
F DisplayScheduler::Presented               | Source/DisplayScheduler.cpp    | 421     | ?
F DisplayScheduler::ChainDepth              | Source/DisplayScheduler.cpp    | 423-426 | ?
F DisplayScheduler::Surrender               | Source/DisplayScheduler.cpp    | 432-459 | ?
F DisplayScheduler::~DisplayScheduler       | Source/DisplayScheduler.cpp    | 461-464 | ?
T MeasuredSpan                              | Api/HardwareMetrics.h          | 34-40   | What one declared span of device execution reads, for the rotation the reading came back from. a device with no timestamp capability, and a span whose rotation has not completed yet, both have no duration — and a zero in either place is a performance report that is confidently wrong. recorded it has completed, so the reading a tick presents is `RecordingRotationDepth` rotations old. Waiting for the current one would serialise the host against the device to measure it.
T HardwareMetrics                           | Api/HardwareMetrics.h          | 58-206  | The declared spans of device execution, the timestamps recorded around each, and the depth they nest to. declared span inside another — `08` §3's ordering is thirteen recordings and a report that timed each span reports unavailable and every recording still records. `Open` and `Close` deliver as no-ops so that no recording site branches on the capability — a conditional at the recording site is one that never leaves, per `06` §1's note on the capability set. that pushed its own measure would write from inside a recording, contending with the tick for the state the tick is presenting.
F HardwareMetrics::~HardwareMetrics         | Api/HardwareMetrics.h          | 75      | ?
F HardwareMetrics::Construct                | Api/HardwareMetrics.h          | 86      | Constructs the timestamp extent every recorded span writes into, sized against the rotation depth. substitution — metrics report unavailable, not zero — and refusing instead would make bring-up fail on a device that can draw everything Slate draws.
F HardwareMetrics::Declare                  | Api/HardwareMetrics.h          | 97      | Declares one span of device execution by name, returning the ordinal that opens and closes it. a name already declared — two spans of one name make one unreadable reading the query extent, and reallocating it invalidates the readings the standing rotations still hold.
F HardwareMetrics::Open                     | Api/HardwareMetrics.h          | 109     | Records the timestamp that opens one declared span, and enters it on the nesting depth. and with RelationCyclic when the span is already open in this rotation
F HardwareMetrics::Close                    | Api/HardwareMetrics.h          | 118     | Records the timestamp that closes one declared span, and leaves it on the nesting depth. with RelationCyclic when the span was not opened in this rotation standing has crossed the nesting, and the depth it reports then belongs to neither of them.
F HardwareMetrics::Clear                    | Api/HardwareMetrics.h          | 128     | Clears one rotation slot's timestamps, immediately before the recording that writes them. whatever the previous rotation wrote, which is a plausible duration attributed to the wrong rotation — the one failure a metric cannot be caught in, because nothing about it looks wrong.
F HardwareMetrics::Resolve                  | Api/HardwareMetrics.h          | 142     | Reads back one completed rotation's timestamps and resolves each declared span's duration. device declines the readback, and DeviceLost when the device was lost — a measurement reports the loss rather than absorbing it as absent data conditional recording of `28`, whose atmosphere spans rebuild only on change — and a zero there would report the rebuild as free rather than as absent.
F HardwareMetrics::Standing                 | Api/HardwareMetrics.h          | 150     | One declared span's last resolved reading. member says it was declared and has no reading — two different facts, and `86` presents both.
F HardwareMetrics::Report                   | Api/HardwareMetrics.h          | 160     | Declares every resolved reading into the register the tick samples. span and nothing at all is declared for an unavailable one, so `MeasureIndex::Resolve` refuses rather than reading zero, which is `08` §5 enforced at the presentation rather than promised.
F HardwareMetrics::Measuring                | Api/HardwareMetrics.h          | 165     | Whether the device declared the timestamp capability at all.
F HardwareMetrics::DeclaredCount            | Api/HardwareMetrics.h          | 170     | How many spans are declared.
F HardwareMetrics::Reclaim                  | Api/HardwareMetrics.h          | 176     | Destroys the timestamp extent and forgets every declared span.
T HardwareMetrics::DeclaredSpan             | Api/HardwareMetrics.h          | 181-186 | One declared span — its name, where its two timestamps sit, and what it last read.
T HardwareMetrics::RecordedRotation         | Api/HardwareMetrics.h          | 189-193 | What one rotation slot recorded — which spans were opened and closed in it.
F HardwareMetrics::TimestampOrdinalOf       | Api/HardwareMetrics.h          | 197     | Where one span's opening timestamp sits in the extent, for one rotation slot.
F HardwareMetrics::Construct                | Source/HardwareMetrics.cpp     | 17-55   | ?
F HardwareMetrics::TimestampOrdinalOf       | Source/HardwareMetrics.cpp     | 61-64   | ?
F HardwareMetrics::Declare                  | Source/HardwareMetrics.cpp     | 66-96   | ?
F HardwareMetrics::Clear                    | Source/HardwareMetrics.cpp     | 102-119 | ?
F HardwareMetrics::Open                     | Source/HardwareMetrics.cpp     | 121-153 | ?
F HardwareMetrics::Close                    | Source/HardwareMetrics.cpp     | 155-190 | ?
F HardwareMetrics::Resolve                  | Source/HardwareMetrics.cpp     | 196-267 | ?
F HardwareMetrics::Standing                 | Source/HardwareMetrics.cpp     | 273-279 | ?
F HardwareMetrics::Report                   | Source/HardwareMetrics.cpp     | 281-293 | ?
F HardwareMetrics::Measuring                | Source/HardwareMetrics.cpp     | 295-298 | ?
F HardwareMetrics::DeclaredCount            | Source/HardwareMetrics.cpp     | 300-303 | ?
F HardwareMetrics::Reclaim                  | Source/HardwareMetrics.cpp     | 309-322 | ?
F HardwareMetrics::~HardwareMetrics         | Source/HardwareMetrics.cpp     | 324-327 | ?
V AbsentImage                               | Api/ImageSpace.h               | 26      | ?
E ImageIntent                               | Api/ImageSpace.h               | 33-40   | What a claimant intends to do with an image, which is what its usage and its aspect follow from. source for `60`, and an image claimed for one and used for the other is a validation error at the recording site rather than at the claim — a long way from the declaration that caused it.
T ImageShape                                | Api/ImageSpace.h               | 46-54   | One image's declared shape, authored by the claimant and never inferred here. the claim because the chain's extents are the image's, and an image claimed flat cannot grow one.
T ImageClaim                                | Api/ImageSpace.h               | 65-72   | What a claimant is handed — the image, the whole-image view, and where it currently stands. alone, because a recording that issues its own barrier has made a claim the record cannot see and the next transition then barriers from a layout the image is not in.
T ImageSpace                                | Api/ImageSpace.h               | 85-186  | Every device image the engine holds, each sliced out of `ByteSpace` and each carrying its own layout. target set declares fifteen formats and their extent relations, and until something walked that declaration and claimed against it the table was a document rather than a target. one — `06` §7's gate, enforced by the caller passing the relation, never re-derived here.
F ImageSpace::~ImageSpace                   | Api/ImageSpace.h               | 92      | ?
F ImageSpace::Construct                     | Api/ImageSpace.h               | 104     | Takes the device and the byte extents every claimed image is sliced from. separate vendor objects, and the driver reports a view's error against the view — an unnamed view under a named image reports as an address beside a name, which reads as two objects.
F ImageSpace::Claim                         | Api/ImageSpace.h               | 117     | Claims one image of the declared shape, slices its bytes, and constructs its whole-image view. zero extent or a format the device declines for the declared intent vendor allocation nothing holds a reference to, and it is reclaimed only at device teardown.
F ImageSpace::Transition                    | Api/ImageSpace.h               | 129     | Records the barrier that carries one image from where it stands to where it is next read. derived from the declared reads and writes, and this is the one place it is recorded.
F ImageSpace::Standing                      | Api/ImageSpace.h               | 135     | The current record for one claimed image, including the layout the last transition left it in.
F ImageSpace::LevelView                     | Api/ImageSpace.h               | 145     | Constructs a view over one reduction level, for the chain `16` §2 walks a level at a time. holding a handle the vendor has already reused.
F ImageSpace::Release                       | Api/ImageSpace.h               | 151     | Destroys one image, every view over it, and returns its bytes.
F ImageSpace::Reclaim                       | Api/ImageSpace.h               | 157     | Releases every claimed image.
F ImageSpace::ClaimedCount                  | Api/ImageSpace.h               | 159     | ?
F ImageSpace::ClaimedBytes                  | Api/ImageSpace.h               | 160     | ?
T ImageSpace::HeldImage                     | Api/ImageSpace.h               | 164-173 | ?
F ImageSpace::UsageOf                       | Api/ImageSpace.h               | 176     | What the declared intent requires of the image, as the vendor spells it.
F ImageSpace::AspectOf                      | Api/ImageSpace.h               | 177     | ?
F ImageSpace::NameOf                        | Api/ImageSpace.h               | 180     | What one declared intent is named in the driver's text, so a claim's name states what writes it.
F ImageSpace::UsageOf                       | Source/ImageSpace.cpp          | 15-37   | ?
F ImageSpace::AspectOf                      | Source/ImageSpace.cpp          | 39-44   | ?
F ImageSpace::Construct                     | Source/ImageSpace.cpp          | 50-62   | ?
F ImageSpace::NameOf                        | Source/ImageSpace.cpp          | 64-78   | ?
F ImageSpace::Claim                         | Source/ImageSpace.cpp          | 84-235  | ?
F ReachedAt                                 | Source/ImageSpace.cpp          | 246-292 | ?
F ImageSpace::Transition                    | Source/ImageSpace.cpp          | 295-342 | ?
F ImageSpace::Standing                      | Source/ImageSpace.cpp          | 348-363 | ?
F ImageSpace::LevelView                     | Source/ImageSpace.cpp          | 365-416 | ?
F ImageSpace::ClaimedCount                  | Source/ImageSpace.cpp          | 418-429 | ?
F ImageSpace::ClaimedBytes                  | Source/ImageSpace.cpp          | 431-442 | ?
F ImageSpace::Release                       | Source/ImageSpace.cpp          | 448-484 | ?
F ImageSpace::Reclaim                       | Source/ImageSpace.cpp          | 486-492 | ?
F ImageSpace::~ImageSpace                   | Source/ImageSpace.cpp          | 494-497 | ?
V AbsentProgram                             | Api/ProgramIndex.h             | 28      | ?
T DepthDeclaration                          | Api/ProgramIndex.h             | 37-42   | How one graphics program resolves depth — tested, written, and against which comparison. and `FarPlaneDepth` is nought. A program declaring the ordinary less-than comparison against a reversed target resolves the furthest surface at every pixel, and the image is the inside of the object rather than an image that sorts wrongly — which is the failure `02` §6 declares the two constants to prevent.
T GraphicsDeclaration                       | Api/ProgramIndex.h             | 55-68   | One graphics program, as the recording that constructs it declares it. declared spans by the vertex ordinal, which is what lets one program serve every partitioning without a per-topology input declaration — and what keeps the attribute set out of a document that writes no attribute. and a program that combined its output with what stood there would be reading a target it produces. here. A program naming a second would be constructed against a division `AttachmentIndex` does not declare, and the vendor reports that at the draw rather than at the construction.
T ComputeDeclaration                        | Api/ProgramIndex.h             | 75-81   | One compute program, as the dispatching document declares it. specialised constant supplied through `Fixed` — `06` §2.1's reason for admitting specialisation at all, and either way it is the shader's declaration rather than a second one held beside it.
T ConstructedProgram                        | Api/ProgramIndex.h             | 92-97   | What a recording is handed — the program, the layout its descriptors reach through, and how it is recorded. recording writes is written **through** it. A recording resolving the program alone would then reach for a layout it derived separately, and two derivations of one layout is `00` §2's case exactly.
T ProgramIndex                              | Api/ProgramIndex.h             | 115-198 | Every program the engine constructs, resolved by the ordinal its declaration returned. is a constructed program — modules, layouts and fixed constants resolved into one object the device executes. The vendor spelling stays verbatim inside every call, per `06`'s opening rule. streams and the layouts, both of which `06` §7 fixes before the first rotation; constructing one inside a recording is a device stall of unbounded duration in the middle of an image. set layouts, which are what a **set** is written against; this is the program layout, which is what a **program** reaches its sets and its constants through. They are different vendor objects and the two lifetimes differ — the set layouts outlive every program constructed from them.
F ProgramIndex::~ProgramIndex               | Api/ProgramIndex.h             | 122     | ?
F ProgramIndex::Construct                   | Api/ProgramIndex.h             | 136     | Takes the device, the modules every program is constructed from, and the layouts it reaches through. separately and by the same ordinal, because they are two vendor objects a recording binds one after the other — and the errors the two raise read alike until the objects are told apart.
F ProgramIndex::DeclareGraphics             | Api/ProgramIndex.h             | 150     | Constructs one graphics program, returning the ordinal every later resolution names it by. an absent render construct, and with HostDenied when the device declines it specialisation the vendor reads at construction is held where its addresses stay put.
F ProgramIndex::DeclareCompute              | Api/ProgramIndex.h             | 158     | Constructs one compute program, returning the ordinal every later resolution names it by.
F ProgramIndex::Resolve                     | Api/ProgramIndex.h             | 165     | The program one ordinal names, for the recording that records against it.
F ProgramIndex::Reclaim                     | Api/ProgramIndex.h             | 171     | Destroys every program and every layout constructed for one.
F ProgramIndex::DeclaredCount               | Api/ProgramIndex.h             | 173     | ?
T ProgramIndex::HeldProgram                 | Api/ProgramIndex.h             | 177-182 | ?
F ProgramIndex::ReachLayout                 | Api/ProgramIndex.h             | 189     | Constructs the layout one program reaches its declared sets and its constant run through.
F ProgramIndex::Construct                   | Source/ProgramIndex.cpp        | 15-29   | ?
F ProgramIndex::ReachLayout                 | Source/ProgramIndex.cpp        | 35-73   | ?
F ProgramIndex::DeclareGraphics             | Source/ProgramIndex.cpp        | 79-236  | ?
F ProgramIndex::DeclareCompute              | Source/ProgramIndex.cpp        | 242-299 | ?
F ProgramIndex::Resolve                     | Source/ProgramIndex.cpp        | 305-321 | ?
F ProgramIndex::DeclaredCount               | Source/ProgramIndex.cpp        | 323-326 | ?
F ProgramIndex::Reclaim                     | Source/ProgramIndex.cpp        | 332-353 | ?
F ProgramIndex::~ProgramIndex               | Source/ProgramIndex.cpp        | 355-358 | ?
E SharedTarget                              | Api/RenderSchedule.h           | 26-44   | Every shared target the schedule declares. Nothing invents a target another already produces.
E ExtentRelation                            | Api/RenderSchedule.h           | 50-55   | How a target's extent relates to the display extent. is never touched by a resize. `06` §4.1 gates that both ways.
F RelationOfTarget                          | Api/RenderSchedule.h           | 62      | The relation one target's extent bears to the display extent. a relation from a format or an extent has derived it from the wrong operand.
T TargetSpace                               | Api/RenderSchedule.h           | 75-142  | What the fifteen declared targets are claimed as — one image ordinal each, against one display extent. fifteen formats and their extent relations and nothing walked it; `TargetSpace` is what walks it. and fraction-of-display target and touches no absolute one. `Reclaim` refuses to hold a persistent extent across the change, and an intermediate drag extent is discarded by the caller not queued here.
F TargetSpace::Claim                        | Api/RenderSchedule.h           | 95      | Claims every declared target against one display extent and one display format. whatever `ImageSpace` refused when a target could not be claimed was never claimed, and the recording site meets it as a null view rather than as this refusal.
F TargetSpace::Reclaim                      | Api/RenderSchedule.h           | 110     | Re-claims every display-relative and fraction-of-display target against a new display extent. persistent extent is carried across. Re-claiming a subset is how one target keeps the previous extent and reads as a shifted image nobody attributes to the resize.
F TargetSpace::Resolve                      | Api/RenderSchedule.h           | 116     | The image one declared target was claimed as.
F TargetSpace::OrdinalOf                    | Api/RenderSchedule.h           | 122     | The image ordinal one target was claimed as, for the transition `ImageSpace` records.
F TargetSpace::Surrender                    | Api/RenderSchedule.h           | 128     | Releases every claimed target and forgets the display extent they were claimed against.
F TargetSpace::ShapeOf                      | Api/RenderSchedule.h           | 134     | The shape one target is claimed at, derived from its relation and the standing display extent.
E RecordingCommand                          | Api/RenderSchedule.h           | 150-154 | What a recording contributes. Authored once by the contributing document, consulted by the orderer.
T DeclaredRecording                         | Api/RenderSchedule.h           | 161-180 | One declared recording. absent capability must degrade to something, and choosing that something belongs to the contributing document rather than to a branch invented at the recording site.
T RenderSchedule                            | Api/RenderSchedule.h           | 191-223 | The ordered recordings of one rotation, fixed at bring-up and merely executed per rotation. implies a solved dependency structure Slate does not build. The recordings and the target set are both known at bring-up, so the ordering is fixed there too.
F RenderSchedule::Contribute                | Api/RenderSchedule.h           | 201     | Contributes one recording to the schedule. contribution produces a target another recording already produces
F RenderSchedule::Fix                       | Api/RenderSchedule.h           | 209     | Fixes the ordering. Derived from the declared reads and writes, never hand-written. when anything scene-referred is ordered after the display-referred line
F RenderSchedule::Ordered                   | Api/RenderSchedule.h           | 215     | The recordings, in the order Fix derived.
F RelationOfTarget                          | Source/RenderSchedule.cpp      | 115-122 | ?
F TargetSpace::ShapeOf                      | Source/RenderSchedule.cpp      | 128-183 | ?
F TargetSpace::Claim                        | Source/RenderSchedule.cpp      | 189-241 | ?
F TargetSpace::Reclaim                      | Source/RenderSchedule.cpp      | 243-297 | ?
F TargetSpace::Resolve                      | Source/RenderSchedule.cpp      | 303-311 | ?
F TargetSpace::OrdinalOf                    | Source/RenderSchedule.cpp      | 313-324 | ?
F TargetSpace::Surrender                    | Source/RenderSchedule.cpp      | 326-350 | ?
F RenderSchedule::Contribute                | Source/RenderSchedule.cpp      | 356-390 | ?
F RenderSchedule::Fix                       | Source/RenderSchedule.cpp      | 396-483 | ?
F RenderSchedule::Ordered                   | Source/RenderSchedule.cpp      | 485-488 | ?
V AbsentModule                              | Api/ShaderCodec.h              | 26      | ?
V SpirvStreamMarker                         | Api/ShaderCodec.h              | 31      | ?
T SpecialisedConstant                       | Api/ShaderCodec.h              | 38-42   | One constant the module is specialised with at construction rather than read at execution. cannot fold into its own scheduling. `16`'s partition extent and `20`'s tile extent are both of that character — fixed for the run and read on every invocation.
T ShaderCodec                               | Api/ShaderCodec.h              | 55-132  | Every lowered shader the engine holds, read from what the build lowered and constructed once. product, which is what that surface delivers; nothing here opens a file itself. run regardless, because `06` §4.2's recovery reconstructs every program and a module discarded at bring-up would have to be read from disk again inside the recovery.
F ShaderCodec::~ShaderCodec                 | Api/ShaderCodec.h              | 62      | ?
F ShaderCodec::Construct                    | Api/ShaderCodec.h              | 70      | Takes the device and the directory the build lowered its streams into.
F ShaderCodec::Resolve                      | Api/ShaderCodec.h              | 80      | Reads one lowered stream, verifies it, and constructs the vendor module from it. ContentUnsupported when it is not SPIR-V or its length is not a whole word count
F ShaderCodec::Stage                        | Api/ShaderCodec.h              | 92      | The stage declaration one module supplies to a program, with its specialisation folded in. at the program's construction, and a declaration returned by value is read after the call that produced it has already surrendered its stack.
F ShaderCodec::Reclaim                      | Api/ShaderCodec.h              | 100     | Destroys every module and every specialisation held for it.
F ShaderCodec::ResolvedCount                | Api/ShaderCodec.h              | 102     | ?
T ShaderCodec::HeldSpecialisation           | Api/ShaderCodec.h              | 106-111 | ?
T ShaderCodec::HeldModule                   | Api/ShaderCodec.h              | 113-118 | ?
F ShaderCodec::ReadStream                   | Api/ShaderCodec.h              | 122     | Reads one whole file into a word run, refusing rather than truncating.
F ShaderCodec::Construct                    | Source/ShaderCodec.cpp         | 16-30   | ?
F ShaderCodec::ReadStream                   | Source/ShaderCodec.cpp         | 36-88   | ?
F ShaderCodec::Resolve                      | Source/ShaderCodec.cpp         | 94-139  | ?
F ShaderCodec::Stage                        | Source/ShaderCodec.cpp         | 145-195 | ?
F ShaderCodec::ResolvedCount                | Source/ShaderCodec.cpp         | 201-204 | ?
F ShaderCodec::Reclaim                      | Source/ShaderCodec.cpp         | 206-222 | ?
F ShaderCodec::~ShaderCodec                 | Source/ShaderCodec.cpp         | 224-227 | ?
V AbsentSpan                                | Api/SpanSpace.h                | 26      | ?
E SpanIntent                                | Api/SpanSpace.h                | 36-44   | What the device is permitted to read one span as, which the vendor requires at its creation. span created without the indirect read is one the recording meets as a validation error at the draw — four calls and one ordering away from the declaration that omitted it. way. The transfer **out** is declared only where something reads it back, since `06` §3 sizes the host-writable extent against what is staged rather than against what is claimed.
T SpanShape                                 | Api/SpanSpace.h                | 51-56   | The shape one span is claimed at — how far it runs, what reads it, and where it lives. device-local for `16`'s partitioning and host-writable for a per-rotation uniform, and the two claim sites know which they are while a table keyed on the intent could not.
T SpanClaim                                 | Api/SpanSpace.h                | 63-69   | One claimed span — the vendor object, how far it runs, and where the host may write it. convenience. A caller writing through it unconditionally has written through a null address on the residency that carries the working set, which is every residency but staging.
T SpanSpace                                 | Api/SpanSpace.h                | 84-196  | Every linear device extent the engine holds, each sliced out of `ByteSpace` and resolved by its ordinal. what occupies them; `ImageSpace` occupies them with images and this occupies them with spans. Until this stood, `16` §4's "positions and ordinals read from declared spans" named a vendor object nothing the ordinal it recorded. Erasing would renumber every claim above the released one, and the document holding the ordinal has no way to observe that it was renumbered.
F SpanSpace::~SpanSpace                     | Api/SpanSpace.h                | 91      | ?
F SpanSpace::Construct                      | Api/SpanSpace.h                | 103     | Takes the device and the byte extents every claimed span is sliced from. what a claimant resolves it by — a name carrying only the intent would be shared by every storage span the engine holds, and the driver's text would then name a set rather than a span.
F SpanSpace::Claim                          | Api/SpanSpace.h                | 116     | Claims one span of the declared shape and binds the bytes it occupies. a zero span or an intent outside the declared set allocation nothing holds a reference to, reclaimed only at device teardown.
F SpanSpace::Amend                          | Api/SpanSpace.h                | 129     | Writes host-supplied bytes into one host-writable span. span, and with ExtentExhausted when the write would run past the claim extent as coherent precisely so that a caller cannot forget the flush at one of its write sites.
F SpanSpace::Transfer                       | Api/SpanSpace.h                | 146     | Records the transfer that carries one span's bytes into another. ExtentExhausted when the transfer would run past either span knows which stage reads them. A transfer recorded without one is read by the device at whatever moment its scheduling reaches it, which is a partitioning that is correct on one driver.
F SpanSpace::Standing                       | Api/SpanSpace.h                | 156     | The current record for one claimed span.
F SpanSpace::Release                        | Api/SpanSpace.h                | 163     | Destroys one span and returns its bytes.
F SpanSpace::Reclaim                        | Api/SpanSpace.h                | 169     | Releases every claimed span.
F SpanSpace::ClaimedCount                   | Api/SpanSpace.h                | 171     | ?
F SpanSpace::ClaimedBytes                   | Api/SpanSpace.h                | 172     | ?
T SpanSpace::HeldSpan                       | Api/SpanSpace.h                | 176-182 | ?
F SpanSpace::UsageOf                        | Api/SpanSpace.h                | 187     | What the declared intent permits the device to read a span as, as the vendor spells it.
F SpanSpace::NameOf                         | Api/SpanSpace.h                | 190     | What one declared intent is named in the driver's text, so a claim's name states what reads it.
F SpanSpace::~SpanSpace                     | Source/SpanSpace.cpp           | 17-20   | ?
F SpanSpace::Construct                      | Source/SpanSpace.cpp           | 22-34   | ?
F SpanSpace::NameOf                         | Source/SpanSpace.cpp           | 36-51   | ?
F SpanSpace::UsageOf                        | Source/SpanSpace.cpp           | 53-80   | ?
F SpanSpace::Claim                          | Source/SpanSpace.cpp           | 86-183  | ?
F SpanSpace::Amend                          | Source/SpanSpace.cpp           | 189-216 | ?
F SpanSpace::Transfer                       | Source/SpanSpace.cpp           | 218-250 | ?
F SpanSpace::Standing                       | Source/SpanSpace.cpp           | 256-270 | ?
F SpanSpace::ClaimedCount                   | Source/SpanSpace.cpp           | 272-283 | ?
F SpanSpace::ClaimedBytes                   | Source/SpanSpace.cpp           | 285-296 | ?
F SpanSpace::Release                        | Source/SpanSpace.cpp           | 302-319 | ?
F SpanSpace::Reclaim                        | Source/SpanSpace.cpp           | 321-327 | ?
T ScoredCandidate                           | Api/VendorClassifier.h         | 19-24   | One scored candidate device.
F Classify                                  | Api/VendorClassifier.h         | 34      | Scores one enumerated device against the presentation surface it must serve. graphics family cannot serve Slate at all and is never chosen as a least-bad option.
F Classify                                  | Source/VendorClassifier.cpp    | 17-89   | ?
T CapabilitySet                             | Api/VulkanExchange.h           | 25-33   | What the created device is capable of, scored once and consulted thereafter. path becomes conditional on something that cannot change, and those conditionals never all leave.
T VulkanExchange                            | Api/VulkanExchange.h           | 43-86   | Holds the instance, the physical device, the created device and the one graphics queue. own identifiers wrapping them do not reuse the banned words those spellings contain.
F VulkanExchange::~VulkanExchange           | Api/VulkanExchange.h           | 50      | ?
F VulkanExchange::ConstructInstance         | Api/VulkanExchange.h           | 57      | Loads the loader and creates the instance, with the diagnostic capability enabled in Debug only.
F VulkanExchange::ConstructDevice           | Api/VulkanExchange.h           | 65      | Enumerates devices, scores them, and creates one with its capability set fixed at creation.
F VulkanExchange::ReclaimDevice             | Api/VulkanExchange.h           | 70      | Destroys every device object and retains the instance, for the recovery in `06` §4.2 ③.
F VulkanExchange::Instance                  | Api/VulkanExchange.h           | 72      | ?
F VulkanExchange::ScoredDevice              | Api/VulkanExchange.h           | 73      | ?
F VulkanExchange::ActiveDevice              | Api/VulkanExchange.h           | 74      | ?
F VulkanExchange::GraphicsQueue             | Api/VulkanExchange.h           | 75      | ?
F VulkanExchange::Capability                | Api/VulkanExchange.h           | 76      | ?
F VulkanExchange::ConstructInstance         | Source/VulkanExchange.cpp      | 19-60   | ?
F VulkanExchange::ConstructDevice           | Source/VulkanExchange.cpp      | 66-122  | ?
F VulkanExchange::ReclaimDevice             | Source/VulkanExchange.cpp      | 128-140 | ?
F VulkanExchange::~VulkanExchange           | Source/VulkanExchange.cpp      | 142-151 | ?
F VulkanExchange::Instance                  | Source/VulkanExchange.cpp      | 157     | ?
F VulkanExchange::ScoredDevice              | Source/VulkanExchange.cpp      | 158     | ?
F VulkanExchange::ActiveDevice              | Source/VulkanExchange.cpp      | 159     | ?
F VulkanExchange::GraphicsQueue             | Source/VulkanExchange.cpp      | 160     | ?
F VulkanExchange::Capability                | Source/VulkanExchange.cpp      | 161     | ?
F Convert                                   | Api/WindowExchange.h           | 27      | Converts a native window handle from `04` into a presentation surface. The split is what keeps `SlateMath` free of a Vulkan header.
F Reclaim                                   | Api/WindowExchange.h           | 32      | Destroys a surface previously converted.
F Convert                                   | Source/WindowExchange.cpp      | 20-36   | ?
F Reclaim                                   | Source/WindowExchange.cpp      | 42-48   | ?
