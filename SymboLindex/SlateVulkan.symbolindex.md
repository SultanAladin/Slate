//============================================================================================================================================
//                                                          SLATEVULKAN.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Symbol roll for SlateVulkan — The classic render constructs `06` §2.1 settled on, declared over the shared targets and re-derived on an extent change.

%format   symbolindex 1.0
%scope    layer
%path     Engine/SlateVulkan
%folders  26
%symbols  283

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
I Api    | Api/Api.symbolindex       | 18 sym | Device image extents — claimed against a declared shape, viewed once, and carrying the layout each one stands in.
I Source | Source/Source.symbolindex | 13 sym | The image claim, the one place a layout transition is recorded, and the reclamation that returns both.
I Api    | Api/Api.symbolindex       | 15 sym | Graphics and compute programs constructed once at bring-up, against the layouts and modules already declared.
I Source | Source/Source.symbolindex | 8 sym  | The layout every program reaches through, the two construction routes, and the reclamation that returns both.
I Api    | Api/Api.symbolindex       | 16 sym | What is recorded in a rotation slot, in what order, and against which shared targets.
I Source | Source/Source.symbolindex | 10 sym | Contribution gating and the ordering derived from declared reads and writes.
I Api    | Api/Api.symbolindex       | 13 sym | Lowered shader streams — read once, verified as SPIR-V, held as vendor modules and specialised at construction.
I Source | Source/Source.symbolindex | 7 sym  | The whole-file read, the stream verification that refuses before the vendor sees it, and the held specialisation.
I Api    | Api/Api.symbolindex       | 17 sym | Device linear extents, each sliced out of ByteSpace and each declaring what the device is permitted to read it as.
I Source | Source/Source.symbolindex | 11 sym | The claim, the host write, the recorded transfer and the release of every linear device extent the engine holds.
I Api    | Api/Api.symbolindex       | 2 sym  | Scores vendor implementations into a capability set, once, at bring-up and at recovery.
I Source | Source/Source.symbolindex | 1 sym  | Enumerated device scored into a capability set and a ranking.
I Api    | Api/Api.symbolindex       | 11 sym | Loader C-ABI, instance and device handles crossing the vendor edge.
I Source | Source/Source.symbolindex | 9 sym  | Instance construction, device scoring and the one graphics queue.
I Api    | Api/Api.symbolindex       | 2 sym  | Native window handle ⇄ VkSurfaceKHR — the one place the window system meets the vendor edge.
I Source | Source/Source.symbolindex | 2 sym  | The surface conversion, taken through the window system that produced the handle.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

V AbsentConstruct                     | Api/AttachmentIndex.h       | 26      | ?
V DepthTargetAbsent                   | Api/AttachmentIndex.h       | 31      | ?
T ConstructDeclaration                | Api/AttachmentIndex.h       | 42-46   | One render construct, as the recording that draws through it declares it. this run lists third writes it to another target entirely, and both targets then carry plausible imagery — which is the reason the run is declared rather than derived from the produced set, whose order is the contributing document's convenience. ordinal of a `SharedTarget` otherwise, and `RelationOfTarget` is not consulted here — every target a construct spans stands at the display extent by declaration.
T ConstructedSpan                     | Api/AttachmentIndex.h       | 57-63   | What a recording is handed — the construct it opens, the span it opens over, and that span's extent. ordinates it records and a second derivation of one extent is `00` §2's case. It is the extent the last `Derive` was performed against and not what the display currently reports.
T AttachmentIndex                     | Api/AttachmentIndex.h       | 81-177  | Every render construct the engine declares, and the span each one covers the claimed targets with. constructed against a construct declared here, and a recording reaching for a dynamic rendering declaration instead has taken a decision `06` already took. vendor performs no implicit transition. `ImageSpace::Transition` stays the one place a layout changes — a construct that transitioned on its own would leave `ImageSpace`'s record naming a layout the image is not in, and the next barrier would then be issued from the wrong one. display-relative target on a resize, which invalidates every view a span was derived over; the construct describes formats alone and is untouched. `Derive` is what re-covers them.
F AttachmentIndex::~AttachmentIndex   | Api/AttachmentIndex.h       | 88      | ?
F AttachmentIndex::Construct          | Api/AttachmentIndex.h       | 97      | Takes the device and the claimed target set every construct is declared over.
F AttachmentIndex::Declare            | Api/AttachmentIndex.h       | 109     | Declares one render construct, returning the ordinal every later resolution names it by. naming an unclaimed target, and with HostDenied when the device declines it The table is what `TargetSpace` claimed against, and re-reading it here would let a construct and a claim come to disagree about one target's format with nothing comparing them.
F AttachmentIndex::Derive             | Api/AttachmentIndex.h       | 123     | Covers every declared construct's targets at one display extent, replacing whatever stood before. and with HostDenied when the device declines a span `06` §7's gate is that no persistent extent survives a resize, and a span retained because its construct "looked unaffected" is exactly such an extent.
F AttachmentIndex::Resolve            | Api/AttachmentIndex.h       | 131     | The construct and the span one ordinal names, for the recording that opens it. with ExtentExhausted before Derive has covered it
F AttachmentIndex::ConstructOf        | Api/AttachmentIndex.h       | 140     | The construct alone, for `ProgramIndex` constructing a program before any span is derived. is known, and only the formats enter that construction. Requiring a derived span to construct a program would order the two the wrong way round.
F AttachmentIndex::Surrender          | Api/AttachmentIndex.h       | 146     | Destroys every span and leaves the constructs standing, ahead of a re-claim at a new extent.
F AttachmentIndex::Reclaim            | Api/AttachmentIndex.h       | 152     | Destroys every span and every construct.
F AttachmentIndex::DeclaredCount      | Api/AttachmentIndex.h       | 154     | ?
F AttachmentIndex::SpansDerived       | Api/AttachmentIndex.h       | 155     | ?
T AttachmentIndex::HeldConstruct      | Api/AttachmentIndex.h       | 159-165 | ?
F AttachmentIndex::LayoutOf           | Api/AttachmentIndex.h       | 169     | Where one attachment stands throughout the construct, so that the vendor transitions nothing.
F AttachmentIndex::LayoutOf           | Source/AttachmentIndex.cpp  | 15-19   | ?
F AttachmentIndex::Construct          | Source/AttachmentIndex.cpp  | 21-30   | ?
F AttachmentIndex::Declare            | Source/AttachmentIndex.cpp  | 36-151  | ?
F AttachmentIndex::Derive             | Source/AttachmentIndex.cpp  | 157-242 | ?
F AttachmentIndex::Resolve            | Source/AttachmentIndex.cpp  | 248-271 | ?
F AttachmentIndex::ConstructOf        | Source/AttachmentIndex.cpp  | 273-282 | ?
F AttachmentIndex::DeclaredCount      | Source/AttachmentIndex.cpp  | 284-287 | ?
F AttachmentIndex::SpansDerived       | Source/AttachmentIndex.cpp  | 289-292 | ?
F AttachmentIndex::Surrender          | Source/AttachmentIndex.cpp  | 298-316 | ?
F AttachmentIndex::Reclaim            | Source/AttachmentIndex.cpp  | 318-336 | ?
F AttachmentIndex::~AttachmentIndex   | Source/AttachmentIndex.cpp  | 338-341 | ?
V AbsentExtent                        | Api/ByteSpace.h             | 25      | ?
E ExtentResidency                     | Api/ByteSpace.h             | 32-37   | Where a claimed span lives, which is the only distinction the caller makes. device declares, and it differs per device, per driver and per configuration. A caller naming the vendor ordinal directly has hard-coded one machine's declaration into a claim site.
E ClaimStanding                       | Api/ByteSpace.h             | 45-50   | What the claimant promises about the span, and therefore what exhaustion means for it. exhaustion is residency policy rather than a reported failure. The distinction cannot be derived from the span — a hundred megabytes is the working set for one caller and an optional prefetch for the next — so it is declared at the claim and carried into the refusal.
T ByteClaim                           | Api/ByteSpace.h             | 60-67   | One sliced byte span — where it sits, how far it runs, and which extent it came out of. is a no-op, which is what makes the caller's reclamation path unconditional.
V DeviceLocalExtentBytes              | Api/ByteSpace.h             | 76      | ?
V HostWritableExtentBytes             | Api/ByteSpace.h             | 77      | ?
T ByteSpace                           | Api/ByteSpace.h             | 87-176  | Every device byte the engine holds, sliced from a small number of large vendor allocations. span is `16`'s; both arrive here as a span and an alignment and leave as an offset. with its neighbours, but no extent is ever handed back to the vendor until Reclaim — an extent returned while any claim still stands is a use-after-free the validation layer reports somewhere else entirely.
F ByteSpace::~ByteSpace               | Api/ByteSpace.h             | 94      | ?
F ByteSpace::Construct                | Api/ByteSpace.h             | 102     | Takes the device and reads the vendor declaration every later claim is scored against.
F ByteSpace::Claim                    | Api/ByteSpace.h             | 115     | Slices one span of the requested residency, taking a further extent when none can satisfy it. cannot use and cannot release, and the release path is the one nobody exercises.
F ByteSpace::Release                  | Api/ByteSpace.h             | 128     | Returns one span to its extent's free list, coalescing it with whatever it now adjoins. quarantined by its **owner** — `20` §5 does exactly that over its own slots — because only the owner knows which rotation last recorded against it.
F ByteSpace::Reclaim                  | Api/ByteSpace.h             | 134     | Destroys every vendor allocation and forgets every slice.
F ByteSpace::ClaimedBytes             | Api/ByteSpace.h             | 139     | What is claimed and what is held, per residency — the two halves `86` reports separately.
F ByteSpace::BackingBytes             | Api/ByteSpace.h             | 140     | ?
F ByteSpace::ExtentCount              | Api/ByteSpace.h             | 141     | ?
T ByteSpace::FreeSpan                 | Api/ByteSpace.h             | 147-151 | ?
T ByteSpace::SlicedExtent             | Api/ByteSpace.h             | 153-162 | ?
F ByteSpace::ClassifyResidency        | Api/ByteSpace.h             | 166     | Scores what the device declares for the one entry that satisfies a residency.
F ByteSpace::ConstructExtent          | Api/ByteSpace.h             | 170     | Takes one further vendor allocation, at least as large as the span that could not be satisfied.
F PowerOfTwo                          | Source/ByteSpace.cpp        | 20-23   | ?
F RaiseToAlignment                    | Source/ByteSpace.cpp        | 25-28   | ?
F ByteSpace::Construct                | Source/ByteSpace.cpp        | 35-56   | ?
F ByteSpace::ClassifyResidency        | Source/ByteSpace.cpp        | 62-90   | ?
F ByteSpace::ConstructExtent          | Source/ByteSpace.cpp        | 96-159  | ?
F ByteSpace::Claim                    | Source/ByteSpace.cpp        | 165-266 | ?
F ByteSpace::Release                  | Source/ByteSpace.cpp        | 272-318 | ?
F ByteSpace::Reclaim                  | Source/ByteSpace.cpp        | 320-344 | ?
F ByteSpace::~ByteSpace               | Source/ByteSpace.cpp        | 346-349 | ?
F ByteSpace::ClaimedBytes             | Source/ByteSpace.cpp        | 355-366 | ?
F ByteSpace::BackingBytes             | Source/ByteSpace.cpp        | 368-379 | ?
F ByteSpace::ExtentCount              | Source/ByteSpace.cpp        | 381-384 | ?
T SurrenderOrdering                   | Api/CommandSequence.h       | 30-36   | What one surrender to the queue waits on and what it signals. that waits at the top of the ordering serialises against a point it only needs before it writes colour, and the display stall that produces reads as a device too slow for the extent.
T CommandSequence                     | Api/CommandSequence.h       | 50-134  | The rotation-deep recordings every contributing document writes its commands into. rather than a queue arbitration. `08` §3's diagram is therefore the submission order verbatim, and nothing here reorders what `RenderSchedule::Ordered` fixed. vendor a per-recording allocator it must then keep, and `06` §7 sizes every per-recording resource against the depth precisely so the whole slot can be reset at once.
F CommandSequence::~CommandSequence   | Api/CommandSequence.h       | 57      | ?
F CommandSequence::Construct          | Api/CommandSequence.h       | 66      | Constructs the per-slot recording extents and the one primary recording each holds. device declines an extent or a recording; refused in full
F CommandSequence::Open               | Api/CommandSequence.h       | 76      | Resets one rotation slot's recording extent and opens its recording for writing. and HostDenied when the device declines the reset or the open
F CommandSequence::Recording          | Api/CommandSequence.h       | 82      | The recording one rotation slot holds, for a document contributing commands to an open slot.
F CommandSequence::Surrender          | Api/CommandSequence.h       | 95      | Closes one rotation slot's recording and surrenders it to the one graphics queue. the device declines the close or the surrender — a component clearing an ordering point it does not own is one that clears it at the wrong moment for every other reader of it.
F CommandSequence::OpenImmediate      | Api/CommandSequence.h       | 103     | Opens a recording outside the rotation, for the one-off transfers bring-up records. a rotation's — an immediate wait inside a rotation is the whole device serialised on the host.
F CommandSequence::SurrenderImmediate | Api/CommandSequence.h       | 110     | Closes an immediate recording, surrenders it, waits for it, and returns it.
F CommandSequence::Reclaim            | Api/CommandSequence.h       | 116     | Destroys every recording and every extent.
T CommandSequence::RecordingSlot      | Api/CommandSequence.h       | 120-125 | ?
F CommandSequence::Construct          | Source/CommandSequence.cpp  | 15-70   | ?
F CommandSequence::Open               | Source/CommandSequence.cpp  | 76-110  | ?
F CommandSequence::Recording          | Source/CommandSequence.cpp  | 112-124 | ?
F CommandSequence::Surrender          | Source/CommandSequence.cpp  | 130-176 | ?
F CommandSequence::OpenImmediate      | Source/CommandSequence.cpp  | 182-213 | ?
F CommandSequence::SurrenderImmediate | Source/CommandSequence.cpp  | 215-270 | ?
F CommandSequence::Reclaim            | Source/CommandSequence.cpp  | 276-304 | ?
F CommandSequence::~CommandSequence   | Source/CommandSequence.cpp  | 306-309 | ?
T RotationSlot                        | Api/CycleScheduler.h        | 29-34   | What one cyclic slot holds — the ordering points a recording against it waits on and signals. ever been submitted, and an unsignalled one waits for a submission that will never arrive — a bring-up that stops before its first image, with no operand and no error.
T CycleScheduler                      | Api/CycleScheduler.h        | 47-121  | The cyclic ordering every per-rotation resource is sized against and every recording is written into. is declared in `Contract/` because `SlateVulkan` sizes against it and `SlateCompute` quarantines against it — one number, two units, and the depth is 🚧 open at `06` §9 between two and three. advancing it separately produces two rotations that agree for exactly as long as nothing refuses.
F CycleScheduler::~CycleScheduler     | Api/CycleScheduler.h        | 54      | ?
F CycleScheduler::Construct           | Api/CycleScheduler.h        | 63      | Constructs the ordering points for every slot in the depth. device declines an ordering point; refused in full, with nothing half-constructed
F CycleScheduler::Await               | Api/CycleScheduler.h        | 73      | Waits until the slot the standing ordinal names is no longer read, and makes it writable again. host that stops with no report, and `06` §7 requires the loss to be reported upward before anything is destroyed — which cannot happen from inside a wait that never returns.
F CycleScheduler::Arm                 | Api/CycleScheduler.h        | 80      | Clears the completion of the standing slot, immediately before the submission that signals it.
F CycleScheduler::Advance             | Api/CycleScheduler.h        | 86      | Carries the standing ordinal to the next slot in the cycle.
F CycleScheduler::Standing            | Api/CycleScheduler.h        | 92      | The slot the standing ordinal names, for the recording and the display that read it.
F CycleScheduler::StandingOrdinal     | Api/CycleScheduler.h        | 97      | Which slot of the depth is standing — what every per-rotation claim is addressed by.
F CycleScheduler::CompletedRotations  | Api/CycleScheduler.h        | 102     | How many rotations have been advanced through since bring-up, for `86`'s pacing report.
F CycleScheduler::Reclaim             | Api/CycleScheduler.h        | 108     | Destroys every ordering point.
F CycleScheduler::Construct           | Source/CycleScheduler.cpp   | 15-57   | ?
F CycleScheduler::Await               | Source/CycleScheduler.cpp   | 63-87   | ?
F CycleScheduler::Arm                 | Source/CycleScheduler.cpp   | 89-101  | ?
F CycleScheduler::Advance             | Source/CycleScheduler.cpp   | 107-114 | ?
F CycleScheduler::Standing            | Source/CycleScheduler.cpp   | 116-122 | ?
F CycleScheduler::StandingOrdinal     | Source/CycleScheduler.cpp   | 124-127 | ?
F CycleScheduler::CompletedRotations  | Source/CycleScheduler.cpp   | 129-132 | ?
F CycleScheduler::Reclaim             | Source/CycleScheduler.cpp   | 138-168 | ?
F CycleScheduler::~CycleScheduler     | Source/CycleScheduler.cpp   | 170-173 | ?
V AbsentDescriptor                    | Api/DescriptorIndex.h       | 25      | ?
T DescriptorSlot                      | Api/DescriptorIndex.h       | 31-37   | What one descriptor slot in a layout carries, as the shader declares it. layout must state, and a layout stating a larger one leaves the shader indexing beyond what is written.
T DescriptorContent                   | Api/DescriptorIndex.h       | 44-53   | What one descriptor set is written with — one entry per slot the recording amends. sampled image into a slot the layout declares as a span is a validation error at the write rather than at the declaration that disagreed.
T DescriptorIndex                     | Api/DescriptorIndex.h       | 67-166  | Every descriptor set layout the engine declares, and the rotation-deep sets claimed against them. declared at bring-up, and `Declare` refuses once `Fix` has been resolved — the gate is a refusal at the call rather than a remark in a review. therefore yields `RecordingRotationDepth` sets, and the recording writes the one its slot names — amending a set the device is still reading is the defect the depth exists to remove.
F DescriptorIndex::~DescriptorIndex   | Api/DescriptorIndex.h       | 74      | ?
F DescriptorIndex::Construct          | Api/DescriptorIndex.h       | 82      | Takes the device against which every layout and every set is constructed.
F DescriptorIndex::Declare            | Api/DescriptorIndex.h       | 90      | Declares one layout from its slots, returning the ordinal every later claim names it by. and with RelationCyclic once the declaration set has been fixed
F DescriptorIndex::Fix                | Api/DescriptorIndex.h       | 100     | Closes the declaration and constructs the one descriptor extent every later claim is sliced from. that reallocates invalidates every set sliced from it, including the ones a rotation still reads.
F DescriptorIndex::Claim              | Api/DescriptorIndex.h       | 108     | Claims one set per rotation slot against a declared layout, returning the claim's ordinal.
F DescriptorIndex::Amend              | Api/DescriptorIndex.h       | 119     | Writes the content of one claimed set for one rotation slot. or above the depth, or a slot the layout does not declare
F DescriptorIndex::Resolve            | Api/DescriptorIndex.h       | 127     | The set one claim names for one rotation slot, for the recording that reads it.
F DescriptorIndex::Layout             | Api/DescriptorIndex.h       | 133     | The layout one ordinal names, for the recording that constructs a program against it.
F DescriptorIndex::Reclaim            | Api/DescriptorIndex.h       | 139     | Destroys every set, every layout and the extent they were sliced from.
F DescriptorIndex::DeclaredCount      | Api/DescriptorIndex.h       | 141     | ?
F DescriptorIndex::ClaimedCount       | Api/DescriptorIndex.h       | 142     | ?
T DescriptorIndex::DeclaredLayout     | Api/DescriptorIndex.h       | 146-150 | ?
T DescriptorIndex::ClaimedSet         | Api/DescriptorIndex.h       | 152-156 | ?
F DescriptorIndex::SlotOf             | Api/DescriptorIndex.h       | 159     | Which declared slot carries an ordinal, or nothing when the layout does not declare it.
F DescriptorIndex::Construct          | Source/DescriptorIndex.cpp  | 15-23   | ?
F DescriptorIndex::Declare            | Source/DescriptorIndex.cpp  | 29-93   | ?
F DescriptorIndex::Fix                | Source/DescriptorIndex.cpp  | 99-163  | ?
F DescriptorIndex::Claim              | Source/DescriptorIndex.cpp  | 169-201 | ?
F DescriptorIndex::SlotOf             | Source/DescriptorIndex.cpp  | 207-216 | ?
F DescriptorIndex::Amend              | Source/DescriptorIndex.cpp  | 218-311 | ?
F DescriptorIndex::Resolve            | Source/DescriptorIndex.cpp  | 317-329 | ?
F DescriptorIndex::Layout             | Source/DescriptorIndex.cpp  | 331-340 | ?
F DescriptorIndex::DeclaredCount      | Source/DescriptorIndex.cpp  | 342-345 | ?
F DescriptorIndex::ClaimedCount       | Source/DescriptorIndex.cpp  | 347-350 | ?
F DescriptorIndex::Reclaim            | Source/DescriptorIndex.cpp  | 356-383 | ?
F DescriptorIndex::~DescriptorIndex   | Source/DescriptorIndex.cpp  | 385-388 | ?
V AbsentImage                         | Api/ImageSpace.h            | 25      | ?
E ImageIntent                         | Api/ImageSpace.h            | 32-39   | What a claimant intends to do with an image, which is what its usage and its aspect follow from. source for `60`, and an image claimed for one and used for the other is a validation error at the recording site rather than at the claim — a long way from the declaration that caused it.
T ImageShape                          | Api/ImageSpace.h            | 45-53   | One image's declared shape, authored by the claimant and never inferred here. the claim because the chain's extents are the image's, and an image claimed flat cannot grow one.
T ImageClaim                          | Api/ImageSpace.h            | 64-71   | What a claimant is handed — the image, the whole-image view, and where it currently stands. alone, because a recording that issues its own barrier has made a claim the record cannot see and the next transition then barriers from a layout the image is not in.
T ImageSpace                          | Api/ImageSpace.h            | 84-175  | Every device image the engine holds, each sliced out of `ByteSpace` and each carrying its own layout. target set declares fifteen formats and their extent relations, and until something walked that declaration and claimed against it the table was a document rather than a target. one — `06` §7's gate, enforced by the caller passing the relation, never re-derived here.
F ImageSpace::~ImageSpace             | Api/ImageSpace.h            | 91      | ?
F ImageSpace::Construct               | Api/ImageSpace.h            | 99      | Takes the device and the byte extents every claimed image is sliced from.
F ImageSpace::Claim                   | Api/ImageSpace.h            | 110     | Claims one image of the declared shape, slices its bytes, and constructs its whole-image view. zero extent or a format the device declines for the declared intent vendor allocation nothing holds a reference to, and it is reclaimed only at device teardown.
F ImageSpace::Transition              | Api/ImageSpace.h            | 122     | Records the barrier that carries one image from where it stands to where it is next read. derived from the declared reads and writes, and this is the one place it is recorded.
F ImageSpace::Standing                | Api/ImageSpace.h            | 128     | The current record for one claimed image, including the layout the last transition left it in.
F ImageSpace::LevelView               | Api/ImageSpace.h            | 138     | Constructs a view over one reduction level, for the chain `16` §2 walks a level at a time. holding a handle the vendor has already reused.
F ImageSpace::Release                 | Api/ImageSpace.h            | 144     | Destroys one image, every view over it, and returns its bytes.
F ImageSpace::Reclaim                 | Api/ImageSpace.h            | 150     | Releases every claimed image.
F ImageSpace::ClaimedCount            | Api/ImageSpace.h            | 152     | ?
F ImageSpace::ClaimedBytes            | Api/ImageSpace.h            | 153     | ?
T ImageSpace::HeldImage               | Api/ImageSpace.h            | 157-166 | ?
F ImageSpace::UsageOf                 | Api/ImageSpace.h            | 169     | What the declared intent requires of the image, as the vendor spells it.
F ImageSpace::AspectOf                | Api/ImageSpace.h            | 170     | ?
F ImageSpace::UsageOf                 | Source/ImageSpace.cpp       | 15-37   | ?
F ImageSpace::AspectOf                | Source/ImageSpace.cpp       | 39-44   | ?
F ImageSpace::Construct               | Source/ImageSpace.cpp       | 50-59   | ?
F ImageSpace::Claim                   | Source/ImageSpace.cpp       | 65-203  | ?
F ReachedAt                           | Source/ImageSpace.cpp       | 214-260 | ?
F ImageSpace::Transition              | Source/ImageSpace.cpp       | 263-310 | ?
F ImageSpace::Standing                | Source/ImageSpace.cpp       | 316-331 | ?
F ImageSpace::LevelView               | Source/ImageSpace.cpp       | 333-375 | ?
F ImageSpace::ClaimedCount            | Source/ImageSpace.cpp       | 377-388 | ?
F ImageSpace::ClaimedBytes            | Source/ImageSpace.cpp       | 390-401 | ?
F ImageSpace::Release                 | Source/ImageSpace.cpp       | 407-443 | ?
F ImageSpace::Reclaim                 | Source/ImageSpace.cpp       | 445-451 | ?
F ImageSpace::~ImageSpace             | Source/ImageSpace.cpp       | 453-456 | ?
V AbsentProgram                       | Api/ProgramIndex.h          | 27      | ?
T DepthDeclaration                    | Api/ProgramIndex.h          | 36-41   | How one graphics program resolves depth — tested, written, and against which comparison. and `FarPlaneDepth` is nought. A program declaring the ordinary less-than comparison against a reversed target resolves the furthest surface at every pixel, and the image is the inside of the object rather than an image that sorts wrongly — which is the failure `02` §6 declares the two constants to prevent.
T GraphicsDeclaration                 | Api/ProgramIndex.h          | 54-67   | One graphics program, as the recording that constructs it declares it. declared spans by the vertex ordinal, which is what lets one program serve every partitioning without a per-topology input declaration — and what keeps the attribute set out of a document that writes no attribute. and a program that combined its output with what stood there would be reading a target it produces. here. A program naming a second would be constructed against a division `AttachmentIndex` does not declare, and the vendor reports that at the draw rather than at the construction.
T ComputeDeclaration                  | Api/ProgramIndex.h          | 74-80   | One compute program, as the dispatching document declares it. specialised constant supplied through `Fixed` — `06` §2.1's reason for admitting specialisation at all, and either way it is the shader's declaration rather than a second one held beside it.
T ConstructedProgram                  | Api/ProgramIndex.h          | 91-96   | What a recording is handed — the program, the layout its descriptors reach through, and how it is recorded. recording writes is written **through** it. A recording resolving the program alone would then reach for a layout it derived separately, and two derivations of one layout is `00` §2's case exactly.
T ProgramIndex                        | Api/ProgramIndex.h          | 114-189 | Every program the engine constructs, resolved by the ordinal its declaration returned. is a constructed program — modules, layouts and fixed constants resolved into one object the device executes. The vendor spelling stays verbatim inside every call, per `06`'s opening rule. streams and the layouts, both of which `06` §7 fixes before the first rotation; constructing one inside a recording is a device stall of unbounded duration in the middle of an image. set layouts, which are what a **set** is written against; this is the program layout, which is what a **program** reaches its sets and its constants through. They are different vendor objects and the two lifetimes differ — the set layouts outlive every program constructed from them.
F ProgramIndex::~ProgramIndex         | Api/ProgramIndex.h          | 121     | ?
F ProgramIndex::Construct             | Api/ProgramIndex.h          | 131     | Takes the device, the modules every program is constructed from, and the layouts it reaches through.
F ProgramIndex::DeclareGraphics       | Api/ProgramIndex.h          | 142     | Constructs one graphics program, returning the ordinal every later resolution names it by. an absent render construct, and with HostDenied when the device declines it specialisation the vendor reads at construction is held where its addresses stay put.
F ProgramIndex::DeclareCompute        | Api/ProgramIndex.h          | 150     | Constructs one compute program, returning the ordinal every later resolution names it by.
F ProgramIndex::Resolve               | Api/ProgramIndex.h          | 157     | The program one ordinal names, for the recording that records against it.
F ProgramIndex::Reclaim               | Api/ProgramIndex.h          | 163     | Destroys every program and every layout constructed for one.
F ProgramIndex::DeclaredCount         | Api/ProgramIndex.h          | 165     | ?
T ProgramIndex::HeldProgram           | Api/ProgramIndex.h          | 169-174 | ?
F ProgramIndex::ReachLayout           | Api/ProgramIndex.h          | 181     | Constructs the layout one program reaches its declared sets and its constant run through.
F ProgramIndex::Construct             | Source/ProgramIndex.cpp     | 15-27   | ?
F ProgramIndex::ReachLayout           | Source/ProgramIndex.cpp     | 33-71   | ?
F ProgramIndex::DeclareGraphics       | Source/ProgramIndex.cpp     | 77-221  | ?
F ProgramIndex::DeclareCompute        | Source/ProgramIndex.cpp     | 227-272 | ?
F ProgramIndex::Resolve               | Source/ProgramIndex.cpp     | 278-294 | ?
F ProgramIndex::DeclaredCount         | Source/ProgramIndex.cpp     | 296-299 | ?
F ProgramIndex::Reclaim               | Source/ProgramIndex.cpp     | 305-326 | ?
F ProgramIndex::~ProgramIndex         | Source/ProgramIndex.cpp     | 328-331 | ?
E SharedTarget                        | Api/RenderSchedule.h        | 26-44   | Every shared target the schedule declares. Nothing invents a target another already produces.
E ExtentRelation                      | Api/RenderSchedule.h        | 50-55   | How a target's extent relates to the display extent. is never touched by a resize. `06` §4.1 gates that both ways.
F RelationOfTarget                    | Api/RenderSchedule.h        | 62      | The relation one target's extent bears to the display extent. a relation from a format or an extent has derived it from the wrong operand.
T TargetSpace                         | Api/RenderSchedule.h        | 75-142  | What the fifteen declared targets are claimed as — one image ordinal each, against one display extent. fifteen formats and their extent relations and nothing walked it; `TargetSpace` is what walks it. and fraction-of-display target and touches no absolute one. `Reclaim` refuses to hold a persistent extent across the change, and an intermediate drag extent is discarded by the caller not queued here.
F TargetSpace::Claim                  | Api/RenderSchedule.h        | 95      | Claims every declared target against one display extent and one display format. whatever `ImageSpace` refused when a target could not be claimed was never claimed, and the recording site meets it as a null view rather than as this refusal.
F TargetSpace::Reclaim                | Api/RenderSchedule.h        | 110     | Re-claims every display-relative and fraction-of-display target against a new display extent. persistent extent is carried across. Re-claiming a subset is how one target keeps the previous extent and reads as a shifted image nobody attributes to the resize.
F TargetSpace::Resolve                | Api/RenderSchedule.h        | 116     | The image one declared target was claimed as.
F TargetSpace::OrdinalOf              | Api/RenderSchedule.h        | 122     | The image ordinal one target was claimed as, for the transition `ImageSpace` records.
F TargetSpace::Surrender              | Api/RenderSchedule.h        | 128     | Releases every claimed target and forgets the display extent they were claimed against.
F TargetSpace::ShapeOf                | Api/RenderSchedule.h        | 134     | The shape one target is claimed at, derived from its relation and the standing display extent.
E RecordingCommand                    | Api/RenderSchedule.h        | 150-154 | What a recording contributes. Authored once by the contributing document, consulted by the orderer.
T DeclaredRecording                   | Api/RenderSchedule.h        | 161-171 | One declared recording. absent capability must degrade to something, and choosing that something belongs to the contributing document rather than to a branch invented at the recording site.
T RenderSchedule                      | Api/RenderSchedule.h        | 182-214 | The ordered recordings of one rotation, fixed at bring-up and merely executed per rotation. implies a solved dependency structure Slate does not build. The recordings and the target set are both known at bring-up, so the ordering is fixed there too.
F RenderSchedule::Contribute          | Api/RenderSchedule.h        | 192     | Contributes one recording to the schedule. contribution produces a target another recording already produces
F RenderSchedule::Fix                 | Api/RenderSchedule.h        | 200     | Fixes the ordering. Derived from the declared reads and writes, never hand-written. when anything scene-referred is ordered after the display-referred line
F RenderSchedule::Ordered             | Api/RenderSchedule.h        | 206     | The recordings, in the order Fix derived.
F RelationOfTarget                    | Source/RenderSchedule.cpp   | 115-122 | ?
F TargetSpace::ShapeOf                | Source/RenderSchedule.cpp   | 128-183 | ?
F TargetSpace::Claim                  | Source/RenderSchedule.cpp   | 189-241 | ?
F TargetSpace::Reclaim                | Source/RenderSchedule.cpp   | 243-297 | ?
F TargetSpace::Resolve                | Source/RenderSchedule.cpp   | 303-311 | ?
F TargetSpace::OrdinalOf              | Source/RenderSchedule.cpp   | 313-324 | ?
F TargetSpace::Surrender              | Source/RenderSchedule.cpp   | 326-350 | ?
F RenderSchedule::Contribute          | Source/RenderSchedule.cpp   | 356-390 | ?
F RenderSchedule::Fix                 | Source/RenderSchedule.cpp   | 396-466 | ?
F RenderSchedule::Ordered             | Source/RenderSchedule.cpp   | 468-471 | ?
V AbsentModule                        | Api/ShaderCodec.h           | 26      | ?
V SpirvStreamMarker                   | Api/ShaderCodec.h           | 31      | ?
T SpecialisedConstant                 | Api/ShaderCodec.h           | 38-42   | One constant the module is specialised with at construction rather than read at execution. cannot fold into its own scheduling. `16`'s partition extent and `20`'s tile extent are both of that character — fixed for the run and read on every invocation.
T ShaderCodec                         | Api/ShaderCodec.h           | 56-133  | Every lowered shader the engine holds, read from what the build lowered and constructed once. and nothing has yet built. The read is a whole-file one of a build product and is replaced by that surface the moment it exists — 🚧 recorded rather than left as an assumption. run regardless, because `06` §4.2's recovery reconstructs every program and a module discarded at bring-up would have to be read from disk again inside the recovery.
F ShaderCodec::~ShaderCodec           | Api/ShaderCodec.h           | 63      | ?
F ShaderCodec::Construct              | Api/ShaderCodec.h           | 71      | Takes the device and the directory the build lowered its streams into.
F ShaderCodec::Resolve                | Api/ShaderCodec.h           | 81      | Reads one lowered stream, verifies it, and constructs the vendor module from it. ContentUnsupported when it is not SPIR-V or its length is not a whole word count
F ShaderCodec::Stage                  | Api/ShaderCodec.h           | 93      | The stage declaration one module supplies to a program, with its specialisation folded in. at the program's construction, and a declaration returned by value is read after the call that produced it has already surrendered its stack.
F ShaderCodec::Reclaim                | Api/ShaderCodec.h           | 101     | Destroys every module and every specialisation held for it.
F ShaderCodec::ResolvedCount          | Api/ShaderCodec.h           | 103     | ?
T ShaderCodec::HeldSpecialisation     | Api/ShaderCodec.h           | 107-112 | ?
T ShaderCodec::HeldModule             | Api/ShaderCodec.h           | 114-119 | ?
F ShaderCodec::ReadStream             | Api/ShaderCodec.h           | 123     | Reads one whole file into a word run, refusing rather than truncating.
F ShaderCodec::Construct              | Source/ShaderCodec.cpp      | 17-31   | ?
F ShaderCodec::ReadStream             | Source/ShaderCodec.cpp      | 37-99   | ?
F ShaderCodec::Resolve                | Source/ShaderCodec.cpp      | 105-150 | ?
F ShaderCodec::Stage                  | Source/ShaderCodec.cpp      | 156-206 | ?
F ShaderCodec::ResolvedCount          | Source/ShaderCodec.cpp      | 212-215 | ?
F ShaderCodec::Reclaim                | Source/ShaderCodec.cpp      | 217-233 | ?
F ShaderCodec::~ShaderCodec           | Source/ShaderCodec.cpp      | 235-238 | ?
V AbsentSpan                          | Api/SpanSpace.h             | 25      | ?
E SpanIntent                          | Api/SpanSpace.h             | 35-43   | What the device is permitted to read one span as, which the vendor requires at its creation. span created without the indirect read is one the recording meets as a validation error at the draw — four calls and one ordering away from the declaration that omitted it. way. The transfer **out** is declared only where something reads it back, since `06` §3 sizes the host-writable extent against what is staged rather than against what is claimed.
T SpanShape                           | Api/SpanSpace.h             | 50-55   | The shape one span is claimed at — how far it runs, what reads it, and where it lives. device-local for `16`'s partitioning and host-writable for a per-rotation uniform, and the two claim sites know which they are while a table keyed on the intent could not.
T SpanClaim                           | Api/SpanSpace.h             | 62-68   | One claimed span — the vendor object, how far it runs, and where the host may write it. convenience. A caller writing through it unconditionally has written through a null address on the residency that carries the working set, which is every residency but staging.
T SpanSpace                           | Api/SpanSpace.h             | 83-185  | Every linear device extent the engine holds, each sliced out of `ByteSpace` and resolved by its ordinal. what occupies them; `ImageSpace` occupies them with images and this occupies them with spans. Until this stood, `16` §4's "positions and ordinals read from declared spans" named a vendor object nothing the ordinal it recorded. Erasing would renumber every claim above the released one, and the document holding the ordinal has no way to observe that it was renumbered.
F SpanSpace::~SpanSpace               | Api/SpanSpace.h             | 90      | ?
F SpanSpace::Construct                | Api/SpanSpace.h             | 98      | Takes the device and the byte extents every claimed span is sliced from.
F SpanSpace::Claim                    | Api/SpanSpace.h             | 109     | Claims one span of the declared shape and binds the bytes it occupies. a zero span or an intent outside the declared set allocation nothing holds a reference to, reclaimed only at device teardown.
F SpanSpace::Amend                    | Api/SpanSpace.h             | 122     | Writes host-supplied bytes into one host-writable span. span, and with ExtentExhausted when the write would run past the claim extent as coherent precisely so that a caller cannot forget the flush at one of its write sites.
F SpanSpace::Transfer                 | Api/SpanSpace.h             | 139     | Records the transfer that carries one span's bytes into another. ExtentExhausted when the transfer would run past either span knows which stage reads them. A transfer recorded without one is read by the device at whatever moment its scheduling reaches it, which is a partitioning that is correct on one driver.
F SpanSpace::Standing                 | Api/SpanSpace.h             | 149     | The current record for one claimed span.
F SpanSpace::Release                  | Api/SpanSpace.h             | 156     | Destroys one span and returns its bytes.
F SpanSpace::Reclaim                  | Api/SpanSpace.h             | 162     | Releases every claimed span.
F SpanSpace::ClaimedCount             | Api/SpanSpace.h             | 164     | ?
F SpanSpace::ClaimedBytes             | Api/SpanSpace.h             | 165     | ?
T SpanSpace::HeldSpan                 | Api/SpanSpace.h             | 169-175 | ?
F SpanSpace::UsageOf                  | Api/SpanSpace.h             | 180     | What the declared intent permits the device to read a span as, as the vendor spells it.
F SpanSpace::~SpanSpace               | Source/SpanSpace.cpp        | 17-20   | ?
F SpanSpace::Construct                | Source/SpanSpace.cpp        | 22-31   | ?
F SpanSpace::UsageOf                  | Source/SpanSpace.cpp        | 33-60   | ?
F SpanSpace::Claim                    | Source/SpanSpace.cpp        | 66-155  | ?
F SpanSpace::Amend                    | Source/SpanSpace.cpp        | 161-188 | ?
F SpanSpace::Transfer                 | Source/SpanSpace.cpp        | 190-222 | ?
F SpanSpace::Standing                 | Source/SpanSpace.cpp        | 228-242 | ?
F SpanSpace::ClaimedCount             | Source/SpanSpace.cpp        | 244-255 | ?
F SpanSpace::ClaimedBytes             | Source/SpanSpace.cpp        | 257-268 | ?
F SpanSpace::Release                  | Source/SpanSpace.cpp        | 274-291 | ?
F SpanSpace::Reclaim                  | Source/SpanSpace.cpp        | 293-299 | ?
T ScoredCandidate                     | Api/VendorClassifier.h      | 19-24   | One scored candidate device.
F Classify                            | Api/VendorClassifier.h      | 34      | Scores one enumerated device against the presentation surface it must serve. graphics family cannot serve Slate at all and is never chosen as a least-bad option.
F Classify                            | Source/VendorClassifier.cpp | 17-89   | ?
T CapabilitySet                       | Api/VulkanExchange.h        | 25-33   | What the created device is capable of, scored once and consulted thereafter. path becomes conditional on something that cannot change, and those conditionals never all leave.
T VulkanExchange                      | Api/VulkanExchange.h        | 43-86   | Holds the instance, the physical device, the created device and the one graphics queue. own identifiers wrapping them do not reuse the banned words those spellings contain.
F VulkanExchange::~VulkanExchange     | Api/VulkanExchange.h        | 50      | ?
F VulkanExchange::ConstructInstance   | Api/VulkanExchange.h        | 57      | Loads the loader and creates the instance, with the diagnostic capability enabled in Debug only.
F VulkanExchange::ConstructDevice     | Api/VulkanExchange.h        | 65      | Enumerates devices, scores them, and creates one with its capability set fixed at creation.
F VulkanExchange::ReclaimDevice       | Api/VulkanExchange.h        | 70      | Destroys every device object and retains the instance, for the recovery in `06` §4.2 ③.
F VulkanExchange::Instance            | Api/VulkanExchange.h        | 72      | ?
F VulkanExchange::ScoredDevice        | Api/VulkanExchange.h        | 73      | ?
F VulkanExchange::ActiveDevice        | Api/VulkanExchange.h        | 74      | ?
F VulkanExchange::GraphicsQueue       | Api/VulkanExchange.h        | 75      | ?
F VulkanExchange::Capability          | Api/VulkanExchange.h        | 76      | ?
F VulkanExchange::ConstructInstance   | Source/VulkanExchange.cpp   | 19-60   | ?
F VulkanExchange::ConstructDevice     | Source/VulkanExchange.cpp   | 66-122  | ?
F VulkanExchange::ReclaimDevice       | Source/VulkanExchange.cpp   | 128-140 | ?
F VulkanExchange::~VulkanExchange     | Source/VulkanExchange.cpp   | 142-151 | ?
F VulkanExchange::Instance            | Source/VulkanExchange.cpp   | 157     | ?
F VulkanExchange::ScoredDevice        | Source/VulkanExchange.cpp   | 158     | ?
F VulkanExchange::ActiveDevice        | Source/VulkanExchange.cpp   | 159     | ?
F VulkanExchange::GraphicsQueue       | Source/VulkanExchange.cpp   | 160     | ?
F VulkanExchange::Capability          | Source/VulkanExchange.cpp   | 161     | ?
F Convert                             | Api/WindowExchange.h        | 27      | Converts a native window handle from `04` into a presentation surface. The split is what keeps `SlateMath` free of a Vulkan header.
F Reclaim                             | Api/WindowExchange.h        | 32      | Destroys a surface previously converted.
F Convert                             | Source/WindowExchange.cpp   | 20-36   | ?
F Reclaim                             | Source/WindowExchange.cpp   | 42-48   | ?
