# 10 — DocumentStructure

`SlateDocument.lib` holds everything the engine can contain, name, persist and revise — and it holds none of it on
a device. It is a peer of `SlateVulkan`, linking neither it nor anything above it. That peer relationship is
the load-bearing property of the whole partition: a document model that cannot compile without a `VkDevice` has
already merged rendering into authoring, and no later discipline separates them again.

This is one of the two units `04-UnitDirectoryStructure.md` omits. It exists because `Layer2_Format` and
`Layer3_Document` are device-free by law and need a link target that enforces the law.

## Position In The Sequence

| Field       | Value                                                                          |
|-------------|---------------------------------------------------------------------------------|
| Unit        | `SlateDocument.lib`                                                             |
| Layers      | `Layer2_Format`, `Layer3_Document`                                              |
| Upstream    | `02` (transforms, tolerances), `04` (streams)                                   |
| Downstream  | `12` linearises it; `14` presents it; `20`, `22`, `24` author into it           |
| Unblocks    | Anything the engine can hold, name, save or undo                                |

## 1. Layer2_Format — Streams

| Component        | Mechanism                                                       |
|------------------|------------------------------------------------------------------|
| `ImageCodec`     | Image stream translation, decoded to a declared colour space     |
| `VectorCodec`    | Vector stream translation — consumed by `52`                     |
| `TypefaceCodec`  | Typeface stream translation; glyph outlines — consumed by `52`   |
| `TopologyCodec`  | Polygon topology stream translation — conditioned by `38`        |
| `FormatCodec`    | Versioned document stream layout and migration                   |

🔴 A codec translates a stream and does nothing else. It does not condition what it decoded, and it does not
decide whether the result is fit to use. `TopologyCodec` produces exactly what the file contained — including
n-gons, duplicate vertices, degenerate faces and absent orientation — and `38` is what makes it paintable. A codec
that silently repairs is a codec whose output cannot be trusted to describe the file.

⚠️ `VectorCodec` was declared in the first draft of this document and no document in the series consumed it. `52`
is its consumer. Recorded as `00` §10 conflict 17.

`FormatCodec` owns the document's own persisted layout, including its version and its migration path. A document
written by an earlier version is migrated on read, and the migration is a declared transformation between versions
— never a conditional inside a reader. Conditionals inside readers are how a format acquires cases nobody can
enumerate.

Codecs read through `StorageExchange` from `04`, so a decode can be driven by byte-range arrival rather than by
whole-file completion. Every decoded image declares its colour space; an image with an assumed colour space is a
future colour defect with no traceable origin.

## 2. Layer3_Document — The Population

The document is a **generationally versioned slot population**. Occupancy is a slot ledger, and identity is a slot
index paired with a generation counter.

| Component               | Mechanism                                                          |
|-------------------------|---------------------------------------------------------------------|
| `PopulationIndex`       | Slot ledger with generational identity                             |
| `OccupancyIndex`        | Which slots are occupied; the free set is its complement           |
| `PropertySpecification` | Typed, named, validated property declarations                      |
| `TopologyStructure`     | Polygon topology — vertices, edges, faces, attributes              |
| `SurfaceStructure`      | Parametric surface topology                                        |
| `SceneStructure`        | The two nesting relations over the population — see `12`           |
| `EnrollmentIndex`       | Which slots are enrolled in a named subset                         |
| `RevisionSequence`      | Ordered, scrubbable sequence of committed transactions             |
| `SelectionSequence`     | Selection ordering, revised separately, **session-scoped** — `48` §2 |

⚠️ `SelectionSequence` is a component of this unit and is **not** written into the document. `48` §2 owns that
ruling and `12` §11 is amended to agree; the three documents previously gave three answers, recorded as `00`
§10 conflict 34.

Six further components extend this population and are specified in their own documents rather than here, because
each carries more mechanism than a row: `MaterialSpecification` (`42`), `IlluminantPopulation` (`44`),
`CameraProjection` (`46`), `SurfaceLayerSequence` (`56`), `BrushSpecification` (`58`) and the spatial
subdivisions in `40`. All are occupants or properties of occupants in this same slot population, and all obey the
generational identity rule below.

🔴 `SurfaceLayerSequence` in `56` is where painted texels live. `20` §4 asserted they "live in `10`" while this
section declared nothing that could hold them — recorded as `00` §10 conflict 16.

### 2.1 Generational identity

An occupant reference is a slot index plus the generation the slot held when the reference was taken. Resolving a
reference compares generations; a mismatch resolves to absent rather than to a different occupant that has since
taken the slot. This is what makes a reference held across a deletion safe without reference counting.

🔴 Occupant identity is Tier A. It is an unsigned integer pair, never a real number, never hashed into a smaller
width for convenience. An identity that collides is not an identity.

### 2.2 Properties

`PropertySpecification` declares typed, named, validated properties. Validation is part of the declaration, not a
separate step performed by whoever writes the value — a property that can hold an invalid value between the write
and the check has an invalid state, and something will observe it.

### 2.3 Revisions

`RevisionSequence` is an ordered, scrubbable sequence of committed transactions. A transaction records the
inverse operation alongside the forward one, so scrubbing backwards is replay of inverses rather than restoration
of snapshots. `22` depends on this: a paint stroke is a transaction, and its inverse is bounded by the surface
extents the stroke touched rather than by the whole surface.

⚠️ `HistoryStack` is the retired spelling. `History` is banned and "stack" understates it — the sequence is
scrubbable in both directions, not merely popped.

### 2.4 Transaction lifecycle

A transaction is not only a forward operation and its inverse. Every interactive edit in the engine is a **drag**
— a gizmo axis, a slider, an outliner row, a stroke — and a drag has a shape the forward-and-inverse pair does not
describe. Declared once here because `22`, `72`, `78` and `84` all need it and four local inventions would behave
four ways.

| Stage    | Meaning                                                              |
|----------|------------------------------------------------------------------------|
| Open     | The edit begins; the prior state of the extent it will touch is held  |
| Amend    | The edit's parameters change; nothing is recorded                     |
| Abandon  | The edit ends with no effect; the prior state is restored             |
| Seal     | The edit ends; one transaction enters the sequence                    |

🔴 An open transaction is **not** in `RevisionSequence` and is not scrubbable. A drag that recorded a transaction
per pointer sample would fill the sequence with states the artist never intended to stop at, and undo would step
back one pixel at a time.

Two transactions **merge** into one when they are adjacent in the sequence, address the same extent, and arrive
within a declared interval — typed characters, a repeated nudge. Merging is declared per operation, never
inferred: an operation that merges when the artist expected two steps is as wrong as one that does not merge when
they expected one.

Every transaction carries a description supplied at Open, which is what `84` presents. A transaction with no
description is presented by its operation name, which is the mechanism's spelling rather than the artist's.

## 3. What Never Appears Here

| Absent                       | Because                                              |
|------------------------------|-------------------------------------------------------|
| Any Vulkan type or header    | The peer relationship is enforced by the linker       |
| Any ImGui type               | ImGui exists only inside `SlateUI`                    |
| Device residency knowledge   | `20` owns residency; the document owns the source     |
| Presentation-order knowledge | `12` derives order; the document holds the relations  |

## 4. Gates

- **Gate:** `SlateDocument` compiles with no include path to `SlateVulkan`, `SlateCompute` or `SlateUI`.
- **Gate:** Every occupant reference carries a generation, and resolution compares it.
- **Gate:** Occupant identity is an integer pair at Tier A.
- **Gate:** Every decoded image declares a colour space.
- **Gate:** Every format version has a declared migration, not a reader conditional.
- **Gate:** Every property declares its validation.
- **Gate:** Every transaction records its inverse.
- **Gate:** Every interactive edit uses the §2.4 lifecycle; no document invents its own.
- **Gate:** An open transaction is absent from `RevisionSequence` until it is sealed.
- **Gate:** Merging is declared per operation, never inferred.
- **Gate:** A codec translates only; conditioning and repair happen in `38`.

## 5. Open

| Open question                                                        | Blocks                     |
|------------------------------------------------------------------------|-----------------------------|
| Whether `RevisionSequence` is bounded, and by what — count or extent    | `22` memory, not design     |
| Which image formats ship in the first `ImageCodec`                      | Nothing structural          |
| Whether `SurfaceStructure` is needed before `24`                        | `24` scheduling only        |
