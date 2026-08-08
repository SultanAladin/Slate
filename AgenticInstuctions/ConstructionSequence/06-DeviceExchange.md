# 06 — DeviceExchange

`Layer2_Device` is the whole of `SlateVulkan.lib`. It abstracts a device and nothing else: it holds handles,
allocations, descriptors and recording rotation, and it has no opinion about what is drawn. It does not know what
a layer stack is, what a brush impression is, or what an outliner row is — the link partition makes that
structural, because `SlateVulkan` and `SlateDocument` are peers and neither links the other.

The vendor spelling is preserved verbatim at the API surface. `VkDevice`, `VkImage`, `VkPipeline` and
`VkDescriptorSet` are correct inside a call; Slate's own identifiers wrapping them are not permitted to reuse the
banned words those spellings contain.

## Position In The Sequence

| Field       | Value                                                                        |
|-------------|-------------------------------------------------------------------------------|
| Unit        | `SlateVulkan.lib`                                                             |
| Layer       | `Layer2_Device`                                                               |
| Upstream    | `02` (transforms, extents), `04` (`WindowInterchange` native handle)          |
| Downstream  | `08` orders it; every recording document records through it                   |
| Unblocks    | A device, an allocation, a descriptor, an image on screen                     |

## 1. The Components

| Component              | Mechanism                                                       |
|------------------------|------------------------------------------------------------------|
| `VulkanExchange`       | Loader C-ABI, instance and device handles crossing the vendor edge|
| `VendorClassifier`     | Scores vendor implementations into a capability set               |
| `DiagnosticExtension`  | Holds `VK_EXT_debug_utils`, queried and enabled                   |
| `WindowExchange`       | Native window handle ⇄ `VkSurfaceKHR`                             |
| `DisplayScheduler`     | Paces image transitions against a latency target                  |
| `ByteSpace`            | Raw device byte extents, sliced and reclaimed                     |
| `ImageSpace`           | Device image extents with layout tracking                         |
| `DescriptorIndex`      | Slot ledger over descriptor sets and their layouts                |
| `CycleScheduler`       | Orders reuse of N cyclic recording slots                          |
| `CommandSequence`      | Ordered recording of device commands                              |
| `HardwareMetrics`      | Measures hardware execution duration and depth                    |
| `ShaderCodec`          | Compiled shader stream layout and specialisation                  |

## 2. Device Bring-Up

① `VulkanExchange` loads the loader and creates the instance, with `DiagnosticExtension` enabled in Debug only.
② `VendorClassifier` enumerates devices and scores them into a capability set.
③ One device is created with the capability set fixed at creation and immutable afterwards.
④ One graphics queue is taken. Transfers are ordered inside it.
⑤ `ByteSpace` and `ImageSpace` claim their backing extents.
⑥ `WindowExchange` converts the native handle from `04` into a surface and establishes the presentation chain.

The capability set is queried once and consulted thereafter. Re-querying a capability at a recording site is how
a code path becomes conditional on something that cannot change, and those conditionals are never all removed.

### 2.1 Settled device decisions

| Decision                | Choice                                    | Why                                          |
|-------------------------|-------------------------------------------|-----------------------------------------------|
| Queue families          | One graphics queue                        | Donor spine assumes it; no measured need yet  |
| Descriptor strategy     | Explicit sets per rotation slot           | Bindless not adopted; see `00` §5             |
| Render target discipline| Classic render construct, not dynamic     | Donor spine assumes it                        |
| Presentation            | Paced against a latency target            | `DisplayScheduler` owns the choice            |
| Allocation              | Sub-allocated from large device extents   | One allocation per resource exhausts the count|

## 3. Allocation

`ByteSpace` and `ImageSpace` sub-allocate from a small number of large device extents. Neither is a general
allocator; both are extent slicers with explicit reclamation, and reclamation is deferred by the rotation depth so
that an extent is never reclaimed while a recording slot still references it.

Device extents are declared by *access shape*, not by contents:

| Shape             | Access                                     | Reclamation                 |
|-------------------|---------------------------------------------|------------------------------|
| Persistent        | Written rarely, read every recording        | Explicit, on document change |
| Rotational        | Written once per recording slot             | Automatic, by rotation depth |
| Transient         | Written and consumed within one recording   | Automatic, at slot end       |
| Uploaded          | Host-visible, host-written, device-read     | Automatic, by rotation depth |

### 3.1 The memory budget

Device memory is finite and Slate's largest consumer — `20`'s tile residency — is demand-driven, so exhaustion is
reached by ordinary use rather than by a defect. A policy is declared here because the alternative is each
consumer discovering exhaustion separately and failing differently.

| Claim         | Claimed by            | Yields under pressure                          |
|---------------|-----------------------|-------------------------------------------------|
| Reserved      | Spine targets from `08`, atmosphere, rotation | Never                       |
| Committed     | Topology, partitions, resident materials      | Never while its occupant lives |
| Discretionary | `20` tile residency, `24` depot results       | First, by `20` §5's policy  |

`ByteSpace` and `ImageSpace` report their claimed and available totals per shape through `86`, every rotation.

🔴 Reserved and committed claims are taken at bring-up and at document change, and both are refused **before** they
are partially satisfied. A claim that half-succeeds leaves the engine in a state no gate describes.

⚠️ Exhaustion of the discretionary claim is not an error and is not reported as one. It is the residency policy
operating: `20` evicts and the artist sees coarser tiles resolve. Exhaustion of a reserved or committed claim is a
refusal reported through `86`, naming the shape and the amount, never a silent smaller allocation.

## 4. Recording Rotation

`CycleScheduler` owns N cyclic recording slots. N is fixed at bring-up and every rotational and transient extent,
every descriptor set and every synchronisation primitive is sized against it. Nothing anywhere in the engine
allocates a per-recording resource outside this rotation — a resource allocated per recording and freed per
recording is a leak with a slow fuse.

`CommandSequence` records into the current slot. `DisplayScheduler` decides when a completed slot is presented,
against a latency target rather than against a queue depth.

### 4.1 Display extent change

The window is resized by the artist continuously — a drag produces a new extent many times a second — and every
display-extent target in `08` §2 is invalid the moment it changes. Declared here because `08` names the targets
and this document owns their backing.

① `DisplayScheduler` observes the extent change from `04`. Nothing is recorded for the new extent yet.
② The rotation is drained to its depth. Every slot holding a reference to a display-extent target completes.
③ The presentation chain is recreated against the new surface extent.
④ Every display-extent and fraction-of-display extent in `08` §2 is reclaimed and re-claimed at the new extent.
⑤ Descriptor sets referencing them are rewritten — the layouts are unchanged, so nothing is constructed.
⑥ Recording resumes. `16`'s first rotation after ⑥ has no previous depth to cull against and runs its second
   phase over everything, per `16` §2.

🔴 Nothing in ①–⑥ is per-resize discretionary. A resize that recreated only some targets leaves the schedule
reading one extent and writing another, which validation reports at a recording far from the resize.

⚠️ A drag produces extents faster than ①–⑥ completes. The extent is therefore taken at ① and the intermediate
extents are discarded, not queued — a queue of stale extents recreates the chain once per discarded extent and
the window lags the pointer by the whole queue.

Persistent extents holding topology, partitions and residency are **not** touched. They do not depend on the
display extent, and reclaiming them on resize would evict the artist's resident tiles every time they moved a
window edge.

### 4.2 Device loss

Device loss is not a defect to be prevented. It is reported by the driver on hardware fault, on driver update
while running, and on timeout under load, and an application without a declared response terminates.

| Stage | Response                                                                     |
|-------|-------------------------------------------------------------------------------|
| ①     | Every recording stops. No further submission is attempted.                    |
| ②     | The loss is reported upward as a declared loss report. `06` writes nothing.   |
| ③     | Every device object is destroyed and the instance is retained.                |
| ④     | Bring-up §2 ②–⑥ runs again, re-scoring the capability set.                   |
| ⑤     | Persistent extents are refilled from the document; discretionary claims start empty. |
| ⑥     | Recording resumes. The artist has lost nothing but their resident tiles.      |

🔴 Stage ② **reports and does not write**. This document cannot name `10`, `FormatCodec` or a document at all —
§7's own gate forbids it and the previous wording violated that gate three lines above it, which is recorded as
`00` §10 conflict 36. `32` observes the loss report, and `32` links both units; it instructs `48` to write the
recovery journal before it lets `06` proceed to ③.

⚠️ That the write is *possible* at all is still the peer partition paying for itself, and the reasoning is
unchanged: `SlateDocument` holds nothing on a device, so a document is writable by a process whose device has
just been lost. What was wrong was only which unit performs the write.

⚠️ The capability set is re-scored at ④ rather than reused. A driver update is one cause of loss, and the updated
driver may score differently. Reusing the previous set assumes the reason for the loss did not change anything.

## 5. Descriptors

`DescriptorIndex` is a slot ledger. Descriptor set layouts are declared with the recording they serve, constructed
at bring-up, and never constructed during a recording. Per-recording variation is expressed by which slot in the
rotation is written, not by constructing a new layout.

## 6. Diagnostics

`DiagnosticExtension` holds `VK_EXT_debug_utils` and is enabled in Debug only. Every device object Slate creates
is given a name through it, because an unnamed object in a validation message costs more to trace than naming
every object costs to write.

`HardwareMetrics` measures execution duration and depth through device timestamps and reports which instruction
specialisation from `04` was active, so that a timing anomaly is attributable.

🔴 `_DEBUG` is never defined. Debug configuration is selected by `SLATE_DEBUG`. `/MD` is used in both Debug and
Release. Defining `_DEBUG` selects the debug CRT and produces an allocator mismatch against `ExternalPackages`.

## 7. Gates

- **Gate:** `SlateVulkan` does not link, include, or name anything in `SlateDocument`.
- **Gate:** No ImGui spelling appears in `Layer2_Device`.
- **Gate:** The capability set is fixed at device creation and never re-queried.
- **Gate:** No descriptor set layout is constructed during a recording.
- **Gate:** Every per-recording resource is sized against the rotation depth.
- **Gate:** Every device object Slate creates carries a diagnostic name in Debug.
- 🔴 **Gate:** A display extent change recreates **every** display-relative target in `08` §2, and no persistent
  extent. Intermediate extents during a drag are discarded, never queued.
- **Gate:** Device loss reports upward before destroying anything; `06` names no document component.
- **Gate:** Nothing in `Layer2_Device` spells `FormatCodec`, `RevisionSequence` or any `10` identifier.
- **Gate:** Recovery re-scores the capability set rather than reusing it.
- **Gate:** Every claim declares its shape, and reserved and committed claims are refused before partial success.
- **Gate:** Discretionary exhaustion is residency policy, not a reported failure.
- **Gate:** `_DEBUG` is defined in no configuration; `/MD` is used in all of them.

## 8. What This Document Does Not Know

Restated as an inventory because `SlateVulkan` acquiring one of these is the erosion the peer partition exists to
prevent, and each has a plausible-looking reason to arrive here.

| Absent           | Because                                                             |
|------------------|----------------------------------------------------------------------|
| A layer sequence | `56` owns it, in `SlateDocument`. `06` holds the texels' backing only |
| An occupant      | `10` owns the population; `06` holds extents, not what fills them    |
| A brush          | `58` owns it                                                        |
| A selection      | `12` owns it                                                        |
| A material       | `42` owns it; `06` holds the descriptor slots it is written into     |

⚠️ The opening paragraph said `06` "does not know what a layer stack is" while nothing in the series defined a
layer stack at all. It is `56`'s `SurfaceLayerSequence`, and `Stack` is a retired spelling. Recorded as `00` §10
conflict 22.

## 9. Open

| Open question                                                             | Blocks                     |
|----------------------------------------------------------------------------|-----------------------------|
| `DOC/VulkanFolder.md` was not in the read set and `00` says it supersedes    | Folder detail inside §1     |
| Rotation depth N — two or three                                             | Extent sizing, not design   |
| Whether a dedicated transfer queue is needed for `20`'s residency traffic   | `20` throughput only        |
| Whether recovery re-resolves the domain or reloads the document entirely     | Recovery latency only       |
