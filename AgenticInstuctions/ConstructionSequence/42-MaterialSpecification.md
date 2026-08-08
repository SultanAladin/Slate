# 42 — MaterialSpecification

A material declares what a surface's channels are and where each one's value comes from. `18` integrates twenty
channels across eight reflectance models; this document is what tells it which twenty values to read and which of
the eight to run.

It also owns the resolution `00` §10 conflict 15 recorded: `16`'s `VisibilityIndex` holds a **partition identity**,
not an occupant. Turning that identity into an occupant, a material and a domain position is stated here.

## Position In The Sequence

| Field       | Value                                                                        |
|-------------|-------------------------------------------------------------------------------|
| Unit        | `SlateDocument.lib`                                                           |
| Layer       | `Layer3_Document`                                                             |
| Upstream    | `10` (`PropertySpecification`), `36` (colour)                                |
| Downstream  | `16` (partition identity), `18` (every channel), `56`, `58`, `62`, `50`      |
| Unblocks    | Channels that `16` can classify and `18` can read                            |

## 1. The Components

| Component                   | What it owns                                                        |
|-----------------------------|----------------------------------------------------------------------|
| `MaterialSpecification`     | One material — its model selection and its channel declarations     |
| `ChannelSpecification`      | One channel: its source, its measure, its default                   |
| `PartitionResolutionIndex`  | Partition identity → occupant, material and domain — §4             |
| `MaterialIndex`             | Every material in the document, addressed by identity               |

## 2. A Channel Declares Its Source

| Source        | Meaning                                                | Resolved by             |
|---------------|---------------------------------------------------------|--------------------------|
| Constant      | One value over the whole surface                       | Read directly           |
| Layered       | `56`'s layer sequence for this channel                 | `20`'s resident tiles   |
| Analytic      | `54` tiling or `52` outlines, resolved at promotion    | `70`                    |
| Imported      | An image from `50`, addressed through the domain       | `20`'s resident tiles   |
| Absent        | The channel's declared default                         | Read directly           |

🔴 `Absent` is not zero. A material with no occlusion channel is fully unoccluded, and a material with no
transmission channel is opaque; both defaults are declared per channel here and neither is the number zero. A
channel defaulted to zero produces surfaces that are black or invisible, and the artist reads that as a broken
material rather than as a missing declaration.

⚠️ Layered and Analytic are not alternatives at the material level. `56` is where they interleave — an analytic
source is a layer in the sequence like any other. The distinction survives to here only because `70` resolves one
of them and `20` transfers the other.

## 3. A Channel Declares Its Measure

`36` §4 needs this and reads nothing else. The measure also fixes the channel's tier and its permitted range.

| Measure       | Colour-converted | Range          | Example                        |
|---------------|------------------|----------------|---------------------------------|
| Reflectance   | Yes              | Zero to one    | Base colour                    |
| Emission      | Yes              | Unbounded      | Emissive radiance              |
| Scalar        | No               | Declared       | Roughness, occlusion           |
| Direction     | No               | Unit           | Tangent-space perturbation     |
| Enrollment    | No               | An identity    | Which model, which subset      |

🔴 There is no inference anywhere in the intake path. `36` §4 says the conversion decision reads this declaration
and nothing else — not the image's encoding, not its channel count, and not its file name. Name-based inference is
the mechanism by which one artist's naming convention silently becomes a requirement of the program.

## 4. Partition Identity Becomes An Occupant

`00` §10 conflict 15 records that `16` writes what it can compute cheaply — a partition identity — and that
resolving it belongs here.

| Held per partition identity | Read by                                  |
|-----------------------------|-------------------------------------------|
| Occupant identity           | `26` outlining, `18` transform            |
| Material identity           | `18` model and channel selection          |
| Face range                  | Domain reconstruction at the pixel        |

`18` §1 reconstructs attributes analytically from the identity plus the depth `16` wrote, and this index is the
first read in that reconstruction. `26` §5 reads the same index so that outlining and shading agree about which
occupant a pixel belongs to.

🔴 The index is a **projection of the document**, rebuilt when the population changes, and it is never authored.
Two sources of truth about which occupant a partition belongs to would disagree exactly when an occupant was
added, which is the moment the artist is looking at it.

## 5. Model Selection

A material selects one of `18`'s eight models, and the selection is an enrollment, not a per-pixel decision.
`16`'s partitions carry the identity; `18` reads the model once per partition.

| Selection is fixed at       | Why                                                            |
|-----------------------------|-----------------------------------------------------------------|
| The material                | So `18` reads it once per partition, not once per pixel        |
| Never per texel             | A per-texel model selection is a divergent branch in shading   |

⚠️ A channel declared for a model the material does not select is retained, not discarded. The artist who switches
a material from one model to another and back expects to find their work; discarding on switch is a destructive
edit disguised as a settings change.

## 6. Materials Are Shared

A material is addressed by identity through `MaterialIndex` and may be enrolled by many occupants. Editing it
changes every occupant enrolled in it, in one transaction.

The `56` layer sequence beneath a **layered** channel belongs to the surface, not to the material. Two occupants
sharing a material and each painted differently is the ordinary case; sharing the paint as well would make the
second occupant's stroke appear on the first.

## 7. Precision

| Computation              | Tier | Reason                                                 |
|--------------------------|------|---------------------------------------------------------|
| Identity comparison      | A    | Wrong occupant means wrong material and wrong outline  |
| Channel value transfer   | B    | Continuous; `18` integrates it                         |
| Default substitution     | A    | An integer selection, not a value                      |

## 8. Gates

- **Gate:** Every channel declares a source, a measure and a default.
- **Gate:** An absent channel resolves to its declared default, which is not assumed to be zero.
- **Gate:** Colour conversion reads the declared measure and never infers from encoding or file name.
- **Gate:** `PartitionResolutionIndex` is derived from the population and is never authored.
- **Gate:** `18` and `26` resolve partition identity through the same index.
- **Gate:** Model selection is per material, never per texel.
- **Gate:** Channels for an unselected model are retained.
- **Gate:** A material is shared by identity; painted content is not shared with it.

## 9. Open

| Open question                                                        | Blocks                          |
|-----------------------------------------------------------------------|----------------------------------|
| PBR channel bit depths and slot layout                                | `00` §12; `18` implementation    |
| Whether a material may declare channels beyond `18`'s twenty          | `18` §10                         |
| Whether material presets ship, and where they live                    | `50` and `48` only               |
