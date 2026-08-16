# ScheduleAmendment — What `08` Becomes

`08` declares fifteen shared targets, one ordering, and one substitution table. This branch adds four targets, six
recording positions, two substitution rows, and two fields to `DeclaredRecording`. It retires nothing.

This document carries the replacement text. It is the authority every other document in the branch defers to for
ordering — `IlluminationGroundwork` §4, `94` §8 and `98` §6 all point here — and it is where one tension between
two already-written documents is resolved rather than left for the recording site to discover.

## Position In The Sequence

| Field       | Value                                                                        |
|-------------|-------------------------------------------------------------------------------|
| Unit        | None — this is an amendment to `08` and to `RenderSchedule.h`, not a component |
| Layer       | `Layer2_Device`, with consequences at `Layer4_Compute`                        |
| Upstream    | `IlluminationGroundwork`, `90` (the capability), `92`, `94`, `96`, `98`, `100`, `102` |
| Downstream  | Every contribution in the branch, and `18`'s amended read set                 |
| Unblocks    | A schedule that orders the branch by derivation rather than by contribution accident |

## 1. What Is Amended

| Amended                          | Section here | Nature                                                  |
|----------------------------------|--------------|----------------------------------------------------------|
| `08` §2 — the target table       | §2           | Four targets appended; the existing fifteen untouched    |
| `08` §3 — the ordering           | §3           | Six positions inserted below ④, none renumbered          |
| `08` §5 — substitution           | §5           | Two rows, both authored by `90` §5                       |
| `RenderSchedule.h`               | §4           | Two fields on `DeclaredRecording`                        |
| `94` §3 and §7                   | 🔴 §3.1      | The direct recording separates from `18`                 |
| `18`, `64`, `86`, `02`, `14`, `90` | §6         | The amendment surface, one row each                      |
| `30`, `44`, `60`, `62`           | §6           | 🔴 Not amended at all — stated so it is not assumed      |

## 2. `08` §2 — The Target Table

Replacement text for the enumeration. 🔴 The four new members are **appended** and the ordinals of the existing
fifteen are unchanged.

```cpp
enum class SharedTarget : std::uint32_t
{
    DepthSurface            = 0u,   // [-] - D32, display extent
    VisibilityIndex         = 1u,   // [-] - R32G32 unsigned integer, display extent
    OccupancySurface        = 2u,   // [-] - R8, display extent
    MotionSurface           = 3u,   // [-] - R16G16 real, display extent
    OcclusionSurface        = 4u,   // [-] - R8, half display extent
    DirectOcclusionSurface  = 5u,   // [-] - RGBA8; four illuminants, per DirectOcclusionCapacity
    TransmissionIndex       = 6u,   // [-] - R32G32 unsigned integer × TransmissionDepth, display extent
    RadianceSurface         = 7u,   // [-] - RGBA16 real, display extent
    ReflectionSurface       = 8u,   // [-] - RGBA16 real, half display extent
    AccumulationSurface     = 9u,   // [-] - RGBA16 real, display extent
    DisplaySurface          = 10u,  // [-] - display format and extent
    OutlineSurface          = 11u,  // [-] - R8, display extent
    TransmittanceSurface    = 12u,  // [-] - RGBA16 real, 256 × 64 — resident, 128 KiB
    MultiScatterSurface     = 13u,  // [-] - RGBA16 real, 32 × 32 — resident, 8 KiB
    SkyViewSurface          = 14u,  // [-] - RGBA16 real, 192 × 108 — resident, 162 KiB
    DirectDiffuseSurface    = 15u,  // [-] - RGBA16 real, display extent — demodulated; `94` §3
    DirectSpecularSurface   = 16u,  // [-] - RGBA16 real, display extent — demodulated; α carries roughness
    IndirectDiffuseSurface  = 17u,  // [-] - RGBA16 real, half display extent — demodulated
    IndirectSpecularSurface = 18u,  // [-] - RGBA16 real, half display extent — demodulated
    TargetCount             = 19u   // [-] - the closed count, never a target
};
```

🔴 Appended and never inserted. `TargetSpace` indexes `ClaimedFor` and `TargetClaimed` by ordinal and
`RenderSchedule` indexes `ProducerOf` by ordinal; an insertion in the middle renumbers every one of them and every
`86` metric keyed by target, silently, in a way that compiles.

| New target                | Relation             | Produced by | Amended by | Read by                    |
|---------------------------|----------------------|-------------|------------|-----------------------------|
| `DirectDiffuseSurface`    | `DisplayRelative`    | `94` ③·iv   | `102` ③·vi | `18`                        |
| `DirectSpecularSurface`   | `DisplayRelative`    | `94` ③·iv   | `102` ③·vi | `18`                        |
| `IndirectDiffuseSurface`  | `FractionOfDisplay`  | `94` ③·v    | `102` ③·vi | `18`, bilaterally upsampled |
| `IndirectSpecularSurface` | `FractionOfDisplay`  | `94` ③·v    | `102` ③·vi | `18`, bilaterally upsampled |

⚠️ `RelationOfTarget` gains four rows and its table stays total over the enumeration. `06` §4.1's gate runs both
ways unchanged: all four are extent-derived, so all four are reclaimed on a display extent change and none is
absolute.

### 2.1 What Is **Not** A Shared Target

Stated because each is a target somebody would add on reflex, and each has exactly one reader.

| Not a shared target                     | Claimed by | Why not shared                                        |
|-----------------------------------------|------------|--------------------------------------------------------|
| `102`'s first and second moments        | `102`      | One reader; `102` §2 already keeps them out of the signals |
| `102`'s wavelet ping-pong extent        | `102`      | Scratch between iterations; nothing outside sees it   |
| `92`'s reservoirs, both signals, both slots | `92`   | Structured records, not images; `ImageSpace` claims neither |
| `92`'s traversal structure and scratch  | `92`       | Driver-shaped; `TargetSpace` claims images only        |
| `96`'s store, `98`'s cells              | Each       | World-referred and extent-independent                  |
| `100`'s maximum chain and illuminant atlas | `100`   | One reader — `94` §2's seam at `ScreenTraced`          |

🔴 A surface orientation target is **not** added and neither is a roughness target. `16` §6's gate stands: depth,
identity, coverage and motion, and nothing else. `102` reconstructs orientation from `VisibilityIndex` through the
same `Shared/` routine `18` uses, and roughness rides in `DirectSpecularSurface`'s alpha where `94` already has it
resolved. An arrangement that added either is the wide attribute target `16` §4 exists to refuse, arriving through
a reconstruction document rather than through a visibility one.

### 2.2 Extent Arithmetic — The Whole Branch At 1920 × 1080

| Claim                                        | Arithmetic                              | Claimed    |
|----------------------------------------------|------------------------------------------|------------|
| `DirectDiffuseSurface`                       | 1920 × 1080 × 8 B                        | 15.8 MiB   |
| `DirectSpecularSurface`                      | 1920 × 1080 × 8 B                        | 15.8 MiB   |
| `IndirectDiffuseSurface`                     | 960 × 540 × 8 B                          | 4.0 MiB    |
| `IndirectSpecularSurface`                    | 960 × 540 × 8 B                          | 4.0 MiB    |
| `102`'s moments, direct pair, two slots      | 1920 × 1080 × 4 B × 2                    | 15.8 MiB   |
| `102`'s moments, indirect pair, two slots    | 960 × 540 × 4 B × 2                      | 4.0 MiB    |
| `102`'s ping-pong extent                     | One display-extent RGBA16                | 15.8 MiB   |
| `92`'s reservoirs, both signals              | `92` §5                                  | 183 MiB    |
| `92`'s structure and scratch, ~500K triangles | `92` §5                                 | 16 MiB     |
| `96`'s store, `98`'s cells, `100`'s chain     | Declared extents; `96` §8, `98` §8        | ~40 MiB    |

⚠️ 💾 ≈ 314 MiB, of which `92`'s reservoirs are 58 %. The signal and moment targets together are 75 MiB and are
not where this branch's memory goes; `92` §7's open row about display-extent reservoirs above 1440p is the one that
decides whether this fits a 6 GiB device, and no amendment here changes that.

### 2.3 The Claim Stays Total

🔴 `TargetSpace::Claim` gains **no capability argument**. Every one of the nineteen targets is claimed at every
capability, including `DirectOcclusionSurface` where `60` is substituted away and the indirect pair where no
indirect term is produced.

⚠️ The cost is real and small — `DirectOcclusionSurface` is 8 MiB at 1080p and unread above `ScreenTraced`. The
alternative costs more than it saves: a conditional claim makes `Resolve` refuse at a site that never expected a
refusal, and `TargetSpace::Claim`'s own contract is that the claim is granted in full or refused in full. A partial
claim set is the arrangement that note exists to forbid, arriving with a capability attached.

## 3. `08` §3 — The Ordering

Six positions are inserted **between ③ and ④**, using `08` §3's existing sub-ordinal idiom — the one
`TransmissionSequence` already writes as ⑤·i and ⑤·ii. 🔴 Nothing is renumbered. Every existing amendment ordinal,
every `08` §3 reference in a source comment, and every `86` metric identity stands.

| Position | Recording                       | Produces / amends                                              | Recorded at            |
|----------|---------------------------------|-----------------------------------------------------------------|-------------------------|
| ①        | Atmosphere                      | The three resident targets                                      | every capability        |
| ②        | `16` — visibility               | Depth, index, occupancy, motion                                 | every capability        |
| ③        | `60` — occlusion projections    | `OcclusionSurface`, `DirectOcclusionSurface`                    | 🔴 `ScreenTraced` only  |
| ③·i      | `100` — maximum chain, atlas tiles | Produces no shared target; its chain is its own claim        | `ScreenTraced` only     |
| ③·ii     | `92` — structure refit          | Produces no shared target; serial on the one queue              | `ComputeTraced` upward  |
| ③·iii    | `98` — cell reservoir refresh   | Produces no shared target; round-robin, bounded                 | every capability        |
| ③·iv     | `94` direct — ordinal 4         | Produces `DirectDiffuseSurface`, `DirectSpecularSurface`        | every capability        |
| ③·v      | `94` indirect — ordinal 6       | Produces the indirect pair; injects `96`                        | `ComputeTraced` upward, or `100` §5 |
| ③·vi     | `102` — ordinal 8               | Amends all four signal targets                                  | every capability        |
| ④        | `18`                            | Produces `RadianceSurface`; 🔴 reads the four signals           | every capability        |
| ⑤·i ·ii  | `62` — ordinals 10, 20          | Unchanged                                                       | every capability        |
| ⑥        | `30` — ordinal 30               | Unchanged                                                       | every capability        |
| ⑦        | `64` — ordinal 40               | Unchanged                                                       | every capability        |
| ⑧        | `DisplayProjection` — ordinal 50 | 🔴 `08` §3.1's display-referred line                            | every capability        |
| ⑨        | `IntersectionOutline` — ordinal 60 | Unchanged                                                     | every capability        |
| ⑩ … ⑬    | Unchanged                       | 🔴 This branch records nothing after ⑧                          | every capability        |

🔴 Exactly one of ③·i and ③·ii records. They are the two answers to `94` §2's `ClassifyOcclusion` seam and a
schedule that recorded both would build a traversal structure nothing queries, or march a chain nothing reads.

⚠️ Everything this branch adds sits **above** ⑧. `08` §3.1's line is not approached, let alone crossed, and the
gate that `Fix` runs — nothing scene-referred ordered after the display-referred line — is satisfied without
argument for all six insertions.

### 3.1 🔴 The Direct Recording Is Its Own Dispatch

`94` §3 says direct resampling runs *inside* `18`'s per-material dispatch, and `102` §4 says reconstruction records
*before* `18`. Both cannot hold: a signal produced inside `18` cannot be reconstructed before `18` consumes it.
This document owns the ordering, so this document rules.

**`94`'s direct recording is a separate compute dispatch at ③·iv.** `94` §3's step list is unchanged in every
other respect — same compacted pixel lists, same eight steps, same single `ClassifyOcclusion` at ⑥ — but it
dispatches on its own and writes the two direct signal targets rather than shading into `RadianceSurface`.

| Consequence                                  | Cost or gain                                                    |
|----------------------------------------------|------------------------------------------------------------------|
| Position and orientation reconstructed twice | Cost: once at ③·iv for `p̂`, once at ④ for shading                |
| Radiometric channels resolved twice          | 🔴 **Avoided** — §3.2; `94` resolves far fewer channels than `18` |
| The direct term is reconstructible           | Gain: `102` §1's four-signal split becomes real rather than two   |
| `94` §3's one-visibility-query economy       | Unchanged — the economy is per pixel, not per dispatch            |
| `IlluminationGroundwork` §4's claims          | Unchanged — `16` still writes no new target, `30` still composites, `64` still accumulates unfiltered |

⚠️ The alternative — fusing direct into `18` and reconstructing only the indirect pair — was considered and
refused. Resampled direct light at one sample per pixel is the noisiest signal this branch produces, contact
shadows are where the artist looks, and a fused arrangement is one where the noisiest signal is the only one that
cannot be reconstructed. `102` §5's decay would then sharpen an image whose direct term never had a filter to decay
out of.

🔴 `94` §3's opening line and §7's table are superseded by this section, and `94` §10's fourth gate reads "writes
no new **resolved** target" — `RadianceSurface` is still produced whole by `18` and by nothing else.

### 3.2 Demodulation — Why The Signals Are Not Radiance

🔴 The four signal targets carry radiance **divided by the surface's reflectance**, and `18` re-modulates.

For a texture painting application this is not an optimisation, it is the difference between a usable image and an
unusable one. A reconstruction kernel run over modulated radiance averages albedo across its footprint, and albedo
is exactly the quantity the artist just painted. The brushwork blurs, and it blurs *more* where the signal is
noisiest — so the artist's strokes soften in shadow and sharpen in light, which reads as the paint tool being
broken rather than the renderer being smooth.

| Signal            | Divided by                                        | Re-modulated at ④ by                        |
|-------------------|---------------------------------------------------|----------------------------------------------|
| Direct diffuse    | Diffuse reflectance — channel 1, resolved          | The same channel, resolved again             |
| Direct specular   | The directional specular reflectance at this angle | The same                                     |
| Indirect diffuse  | Diffuse reflectance                                | The same                                     |
| Indirect specular | Directional specular reflectance                   | The same                                     |

⚠️ The divisor is bounded away from nothing and the bound lives in `Contract/`, never at the site — `02` §8. A
demodulation bound written at four sites is tuned at four sites and disagrees at three, and it disagrees precisely
on near-black materials where the quotient is largest.

🔴 `94`'s dispatch resolves what `p̂` needs and no more: position, orientation, the two reflectance quantities
above, and roughness. `18` continues to resolve every channel through `20` and `70`. The two dispatches overlap on
a small subset, which is why §3.1's second row reads as a gain — a fused arrangement would have had `94` carry
`18`'s whole channel resolve into a recording that only needed a fraction of it.

### 3.3 Where `18` Reads Them

Superseding `94` §7's table.

| `AmbientContribution` member | Source today                        | Source at ④ after this branch                       |
|------------------------------|-------------------------------------|------------------------------------------------------|
| `DiffuseComponent`           | `SkyViewSurface`, cosine-convolved  | `IndirectDiffuseSurface`, upsampled, re-modulated; the sky where absent |
| `SpecularComponent`          | `SkyViewSurface` at the reflection direction | `IndirectSpecularSurface`, likewise           |
| `EmissiveComponent`          | Channel 7                           | Unchanged — never attenuated, never resampled        |
| `Attenuation`                | Channel 6 × `60`                    | Channel 6 only above `ScreenTraced`                  |
| The direct term              | Integrate `44` §5's reaching set    | `DirectDiffuseSurface` + `DirectSpecularSurface`, re-modulated |

⚠️ `18`'s ambient specular is still **pre-added** before `30` subtracts it. `30` §1's exact composite has the same
operand it always had, computed from a different source, and `SpecularProjection::Compose` is not amended. This is
`IlluminationGroundwork` §4's first consequence and it survives §3.1's separation intact.

## 4. Two Amendments To `RenderSchedule.h`

Both are forced by the ordering above, and both are stated as source rather than as prose because `Fix` is
mechanical and refuses what it cannot derive.

### 4.1 A rotation-crossing read

`102` reads `AccumulationSurface` for `64`'s stored sample count — `102` §5 — and `64` produces that target at ⑦,
three positions later. `Fix` refuses when a target is read by a recording ordered before its producer, so the
declaration must say which rotation it means.

```cpp
// 🔴 Read from the **previous** cycle slot, and therefore excluded from the producer-ordering derivation.
//    `102` §5 reads `64`'s stored count and `64` §6 reads its own previous result; both are rotation-crossing
//    and both are declared, never omitted. An edge left out of `Reads` to dodge the derivation makes `Fix`
//    sound on a graph that is not the real one.
// 📝 `Contribute` refuses a target that appears in both `Reads` and `ReadsPreviousSlot`.
std::vector<SharedTarget>  ReadsPreviousSlot = {};   // [-] - consumed from the prior cycle slot
```

⚠️ `64`'s existing self-read migrates to this field. It changes no behaviour and no ordering — it makes the edge
that `64` §6 already declares in prose visible to the mechanism that checks it.

### 4.2 A capability range

`60` records only at `ScreenTraced` and `92` only above it. The existing pair — `CapabilityRequired` with a
`Substitution` — expresses "absent, so run something else" and cannot express "present, so do not run".

```cpp
// 🔴 The inclusive capability range this recording is contributed for. The defaults admit every capability, so
//    every recording contributed before this field existed is unchanged.
// 📝 The capability is fixed at device creation and `Contribute` runs at bring-up, so a contributor may equally
//    resolve its own declaration against it. This pair exists so that the common case needs no branch.
TraversalCapability  RecordedFrom    = TraversalCapability::ScreenTraced;     // [-] - inclusive floor
TraversalCapability  RecordedThrough = TraversalCapability::HardwareTraced;   // [-] - inclusive ceiling
```

| Recording       | `RecordedFrom`  | `RecordedThrough` |
|-----------------|-----------------|--------------------|
| `60` at ③       | `ScreenTraced`  | 🔴 `ScreenTraced`  |
| `100` at ③·i    | `ScreenTraced`  | `ScreenTraced`     |
| `92` at ③·ii    | `ComputeTraced` | `HardwareTraced`   |
| `94` indirect at ③·v | `ScreenTraced` where `100` §5 ships, else `ComputeTraced` | `HardwareTraced` |
| Everything else | Default         | Default            |

🔴 A recording excluded by its range is excluded from `ProducerOf` as well, so `18`'s read of
`DirectOcclusionSurface` must also be conditional above `ScreenTraced` or `Fix` refuses a read with no producer.
That conditionality is resolved at contribution time against the fixed capability — it is never a runtime branch,
which is `94` §2's rule and `VulkanExchange`'s own note.

⚠️ `RenderSchedule.h` already includes `VulkanExchange.h`, so `TraversalCapability` is in scope where the field is
declared. No include is added and no header inverts.

## 5. `08` §5 — Substitution

Two rows, authored by `90` §5 and reproduced here because `08` §5 is where the table lives.

| Capability absent          | Recordings affected     | Substitution                                                   |
|----------------------------|-------------------------|-----------------------------------------------------------------|
| Traversal group            | ③·ii, ③·v               | ③·i's extremum march answers `ClassifyOcclusion`; `96` is the only indirect source; ③ stands |
| Ray-tracing recording only | ③·iv, ③·v               | The inline ray-query form; the algorithm and every target are identical |

⚠️ The second row substitutes an execution model and not an algorithm, so it changes no target, no ordinal and no
read set. It is a row in the table because `08` §1 requires a capability requirement to name what runs instead,
not because anything downstream can tell the difference.

🔴 `102` at ③·vi declares **no capability requirement**. It reconstructs whatever the four signal targets carry,
and at `ScreenTraced` with no indirect term the indirect pair carries the sky-derived fallback `18` would have read
anyway. A reconstruction that refused when a capability was absent would leave the direct signals — the noisiest
ones — unreconstructed on exactly the hardware least able to converge them.

## 6. The Amendment Surface

One row per document the branch touches outside its own numbering. 🔴 A document not listed is not amended, and
four of them are listed precisely to say so.

| Document | Amended                                                                                       |
|----------|------------------------------------------------------------------------------------------------|
| `02`     | §6 gains `104`'s two entry points; `Contract/` gains the non-finite weight bound, the demodulation bound, and the march crossing bound |
| `14`     | One panel row presenting `90`'s negotiated capability and `86`'s occupancy and truncation reports |
| `18`     | §5's ambient sources become §3.3's table; §9's first gate becomes `IlluminationGroundwork` §1's restatement; the direct term is read and re-modulated rather than integrated |
| `28`     | 🔴 Not amended. `SkyViewSurface` keeps its meaning and remains the fallback `96` §4's chain terminates at |
| `30`     | 🔴 Not amended. §3.3's last note is why — the composite's operand is unchanged                  |
| `44`     | 🔴 Not amended. §5's index still answers the primary term; `98` is a second structure, not a replacement |
| `60`     | 🔴 Not amended internally. §8's second gate holds at `ScreenTraced`; §4.2's range excludes it above |
| `62`     | 🔴 Not amended. Ordinals 10 and 20 stand and `08` §3.2's rule covers `94`'s two recordings without change |
| `64`     | §6's self-read migrates to §4.1's field; `RejectionSpecification::CountCeiling` gains a second reader in `102` §5 |
| `86`     | New reported measures: `96`'s occupancy, `92`'s refit count and invalidation cause, `100`'s atlas truncation, `94`'s bounded non-finite weights |
| `90`     | Its own §2 enumeration is the operand of §4.2's fields; no change to `90`'s text                |
| `00`     | §5's global-illumination row deleted; §5.1's socket one names `102`; §9's series list gains eight entries; §9.1 re-derived |

## 7. Gates

- **Gate:** The four new targets are appended; no existing target ordinal changes.
- **Gate:** `RelationOfTarget` stays total over the enumeration.
- **Gate:** No orientation target and no roughness target is added; `16` §6's gate is untouched.
- **Gate:** `TargetSpace::Claim` takes no capability argument and claims all nineteen at every capability.
- **Gate:** Six positions are inserted below ④ using the sub-ordinal idiom; nothing is renumbered.
- **Gate:** Exactly one of ③·i and ③·ii records.
- **Gate:** Nothing this branch contributes is ordered after ⑧.
- 🔴 **Gate:** `94`'s direct term is its own dispatch at ③·iv and produces two targets; `18` produces
  `RadianceSurface` whole and remains its only producer.
- 🔴 **Gate:** All four signal targets are demodulated; `18` re-modulates; the bound is in `Contract/`.
- **Gate:** `102` amends the four signals and produces none of them.
- **Gate:** Every rotation-crossing read is declared through `ReadsPreviousSlot`, never omitted from `Reads`.
- **Gate:** A capability-conditional read set is resolved at contribution time, never branched at a recording site.
- **Gate:** `08` §5 carries both substitution rows and `102` requires no capability.
- **Gate:** `30`, `44`, `62` and `28` are not amended by this branch.

## 8. Open

| Open question                                                                  | Blocks                              |
|---------------------------------------------------------------------------------|--------------------------------------|
| Whether the direct pair packs to one RGBA16 target with a shared luminance       | 💾 §2.2; costs a reconstruction extent |
| Whether `DirectOcclusionSurface` is ever claimed conditionally                    | 💾 8 MiB against §2.3's ruling       |
| Whether `102`'s ping-pong extent can be one of `62`'s intermediates               | 💾 15.8 MiB; `62` §4 carries the shape |
| Whether `64`'s self-read migration is done in this branch or separately            | Nothing structural; §4.1 changes no behaviour |
| Whether ③·iii is worth a position of its own or folds into ③·v's dispatch          | Cost only; `98` §3's round-robin is small |
