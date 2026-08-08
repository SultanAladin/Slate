# 86 — DiagnosticPanel

Thirteen documents in this series discharge an obligation by writing "reported through `86`". Every one of them
was right to: a solve that stops at its ceiling, a claim that cannot be satisfied, a source that declared no
colour space, an illuminant that did not fit the packed capacity — each is a decision the engine made on the
artist's behalf, and each is invisible until something presents it.

This document is that something. It is the last of the forty-four and it exists because the alternative is
thirteen mechanisms each inventing its own way to be quiet.

## Position In The Sequence

| Field       | Value                                                                        |
|-------------|-------------------------------------------------------------------------------|
| Units       | `SlateMath.lib` (the register), `SlateUI.lib` (the panel)                      |
| Layers      | `Layer1_Numeric`, `Layer5_Interface`                                          |
| Upstream    | Every document that reports — §4's register names them                         |
| Downstream  | `14` presents it as `DiagnosticPanel` — `14` §1                               |
| Unblocks    | Somewhere for Tier C reports and misses to land                               |

## 1. The Components

| Component               | What it owns                                                            |
|-------------------------|--------------------------------------------------------------------------|
| `ReportSequence`        | The session's appended reports, in arrival order — §2.2                 |
| `ReportSpecification`   | What one report declares: origin, class, measure, position              |
| `MeasureIndex`          | The current value of every sampled measure, keyed by origin — §2.1      |
| `ReportClassifier`      | Scores a report into one of §4.1's seven classes                        |
| `DiagnosticPanel`       | Presents both, and stores neither                                       |

🔴 `DiagnosticPanel` **stores nothing**, per `14` §1. It presents `MeasureIndex` and `ReportSequence`, and a panel
that held its own copy of either would present a residency total from the rotation before last.

## 2. A Measure And A Report Are Not The Same Thing

This is the distinction the whole document rests on, and conflating the two is the defect that makes a diagnostic
panel useless within thirty seconds of being opened.

`06` §3 reports claimed and available totals **every rotation**. At a display rate that is thousands of entries a
minute from one origin. If those entries append, `ReportSequence` is a scrolling wall of numbers inside which the
one report that mattered — a refused claim, a solve that hit its ceiling — is unfindable.

### 2.1 Measures

A measure is a **sampled quantity with a current value**. It is written into `MeasureIndex` under its origin, it
overwrites the previous value, and it is never appended.

| Measure                                    | Origin        | Sampled          |
|--------------------------------------------|---------------|-------------------|
| Claimed and available totals per shape     | `06` §3       | Every rotation    |
| Resident tile totals and deferred promotion| `20` §2.2     | Every rotation    |
| Hardware execution duration and depth      | `06` §6       | Every rotation    |
| Projection count and rebuild count         | `60`          | Every rotation    |
| Transmissive occupant count and layer depth| `62`          | Every rotation    |
| Accumulated sample count and rejection rate| `64`          | Every rotation    |
| Chart distortion and domain occupancy      | `68`          | Per partition     |
| Sequence length and extent held            | `84`          | Per transaction   |
| A long solve's progress                    | `34` §7       | By the tick       |

🔴 Measures are **sampled by the tick, never pushed**. `34` §7 states the rule for progress and it holds for every
row above: a producer that pushes its measure at its own rate contends with the tick for the state the tick is
presenting, and `06`'s totals would be written from inside a recording.

### 2.2 Reports

A report is a **single event with a cause**. It is appended to `ReportSequence` once, it is retained for the
session, and it does not overwrite anything.

🔴 A report is appended **exactly once per occurrence**, at the moment of the occurrence, and it carries its
origin. A report reconstructed later from a measure that changed is a report about the wrong instant.

## 3. The Register Does Not Live In The Interface

🔴 `ReportSequence` and `MeasureIndex` live in `SlateMath.lib` — the unit every other unit links. This is not
where a diagnostic panel's storage would naturally be put, and it is the only place it can be.

`00` §2's partition is the reason. `SlateVulkan` and `SlateDocument` are peers; `SlateCompute` cannot link
`SlateUI`. Every one of §4's origins is beneath `SlateUI`, so a register held in `SlateUI` could not be written by
a single one of the mechanisms obliged to write it.

⚠️ This is `76`'s reasoning arriving a second time from the opposite direction. `76` holds non-document state
where both `SlateCompute` and `SlateUI` can reach it; `86` holds reports where **all four** units can reach them,
which puts it one layer lower still. Two documents needing the same escape from the same partition is the
partition working, not a weakness in it.

### 3.1 Appending from a work item

`34` §3 rules that a work item never mutates the document and that results cross back on the tick. A report is
not a result and is not document state, and the rule does not reach it.

🔴 `ReportSequence` accepts an append from **any** thread, and it is the one structure in the engine that does. A
report about a failure has to survive the failure: `34` §5's failed item produces no result to carry it back, and
a report that crossed back through `CompletionExchange` would be discarded with the cancellation.

🔴 Ordering is by the arrival timestamp from `04` §3, and **no engine behaviour depends on report order**. This is
what makes the concession above safe. `34` §6's determinism rule is about results; two machines may order two
simultaneous reports differently and nothing downstream reads the difference.

## 4. The Register

Every obligation made to this document in the series, discharged. Almost every row is a promise another document
already made, which is what closes the list — it is bounded by the series rather than by judgement about what an
artist might want to see.

| Origin        | Reported                                                            | Class       |
|---------------|----------------------------------------------------------------------|--------------|
| `00` §5.2     | A vector source refused rather than silently losing content          | Refusal      |
| `02` §5       | A Tier C solve terminated by its ceiling, not by its criterion       | Termination  |
| `06` §3       | A reserved or committed claim refused, naming shape and amount       | Refusal      |
| `06` §4.2     | Device loss, and which of stages ①–⑥ recovery reached                | Failure      |
| `12` §9       | A relation change that would create a cycle rejected, naming occupants | Refusal      |
| `24` §4       | A transfer's termination cause and its miss count per channel        | Termination  |
| `34` §5       | A work item failed, with its origin                                  | Failure      |
| `36` §3       | An assumed content colour space, naming what was assumed             | Assumption   |
| `38` §3       | A face with no consistent orientation reported                       | Amendment    |
| `44` §9       | Illuminants beyond what `IlluminantIndex` carries per partition      | Truncation   |
| `48` §4       | A document that cannot be read at all; no partial population opens   | Refusal      |
| `48` §5       | A missing external reference, presented as an enrolled absence       | Failure      |
| `48` §7       | A document migrated on read, naming both versions                    | Amendment    |
| `50` §3       | An intake assumption recorded in `IntakeIndex`                       | Assumption   |
| `50` §6       | A construct that does not survive emission, named at intake          | Refusal      |
| `52` §2       | A refusal naming the construct and its position in the source        | Refusal      |
| `58` §5       | A brush that reached the declared spacing floor                      | Amendment    |
| `60` §3.1     | Illuminants integrated unattenuated because the index truncated      | Truncation   |
| `62` §3.1     | Transmissive layers discarded at the per-pixel ceiling               | Truncation   |
| `68` §2       | A seam derived rather than authored                                  | Amendment    |
| `68` §4       | A flattening that terminated at its ceiling, with its distortion     | Termination  |
| `56` §3.1     | A re-partition that resampled painted texels                         | Amendment    |

Two rows are added here rather than discharged, and both are named as additions so the register stays auditable
against the series:

- `06` §4.2 declares the device-loss response and promises no report. A recovery that completed silently leaves
  the artist unable to explain why their resident tiles went coarse, so the stage reached is reported.
- `48` §5 requires a missing reference to report what it was looking for and does not say where. Here.

⚠️ `56` §3.1's row is the most consequential entry in the table and the easiest to present as though it were not. It is the one operation in the engine that resamples authored content. Presented at the same weight as a residency total, it is a line the artist scrolls past before discovering their paint softened.

### 4.1 Report classes

| Class        | Meaning                                                              | Artist's question       |
|--------------|----------------------------------------------------------------------|--------------------------|
| Measure      | A sampled quantity with a current value — §2.1                       | How much?                |
| Assumption   | The source declared nothing and something was chosen                 | What did you guess?      |
| Amendment    | Content was changed on the way in, out, or between partitions        | What changed?            |
| Truncation   | Content was dropped at a declared capacity                           | What is missing?         |
| Refusal      | Declined outright; nothing partial was produced                      | Why did nothing happen?  |
| Termination  | A Tier C solve ended at its ceiling instead of its criterion         | Is this result good?     |
| Failure      | The mechanism did not complete and there is no result                | What broke?              |

🔴 The class is declared by the reporting mechanism, never inferred by `ReportClassifier` from the text. An
inferred class is a presentation that disagrees with the document that made the promise.

## 5. A Report Is Not A Failure

Five of the seven classes describe **normal operation**, and a panel that presents all seven as problems teaches
the artist to ignore it.

| Situation                                     | Reported | A problem |
|-----------------------------------------------|----------|------------|
| Discretionary claim exhausted — `06` §3       | No       | No         |
| Promotion deferred against budget — `20` §2.2 | Measure  | No         |
| A coarse tile resolving under the cursor      | No       | No         |
| A superseded work item cancelled — `34` §5    | No       | No         |
| A reserved claim refused — `06` §3            | Refusal  | Yes        |
| A Tier C ceiling reached — `02` §5            | Termination | Sometimes |

🔴 Exhaustion of the discretionary claim is **not reported at all**, per `06` §3. It is the residency policy
operating as designed, it happens continuously during ordinary painting, and reporting it would mean the panel is
never quiet.

⚠️ A Tier C termination is the ambiguous row and it is presented as ambiguous rather than resolved. `02` §5
requires a Tier C component to report which of the criterion and the ceiling terminated it; the ceiling means the
result is the best available, not that it is wrong. Presenting it as an error makes artists avoid unwrapping.

## 6. Volume

A report that recurs is presented **once with a count**, not once per occurrence.

| Recurrence                                    | Presented                                    |
|-----------------------------------------------|-----------------------------------------------|
| Same origin, same class, same subject         | One entry, with a count and the latest instant |
| Same origin, different subject                | Separate entries                              |
| Same subject, different class                 | Separate entries                              |

🔴 Coalescing is by origin, class **and** subject together. Coalescing by origin alone would present twelve
distinct refused constructs from `52` as one entry with a count of twelve, and `52` §2 promises the artist the
construct and its position — which is exactly what the count destroys.

⚠️ `ReportSequence` is bounded for the session. When the bound is reached the **oldest** entries are discarded
and the discard is itself presented, because a register that silently forgot the first report of a run is worse
than one that admits it is full. `34` §9 carries the same open row from the other side: how often a failed item
retries decides the volume this bound has to absorb.

## 7. What The Panel Presents

| Presented                | From                                                              |
|--------------------------|--------------------------------------------------------------------|
| Live measures            | `MeasureIndex`, sampled by the tick                                |
| The report register      | `ReportSequence`, newest last                                      |
| Class, origin and count  | `ReportSpecification` — never re-derived from the text             |
| Progress of long solves  | `34` §7's fraction or count, per open item                         |

`34` §7 states that progress reaching a presented panel is this document's concern. It is presented per open work
item with its priority from `34` §4, because an artist waiting on an unwrap needs to see that an export is
occupying the workers rather than that "something" is running.

🔴 Reports are recorded in **every** configuration, not in Debug only. `06` §6's `DiagnosticExtension` is
Debug-only and is a different mechanism: validation messages are for whoever builds Slate, and §4's register is
for whoever uses it. A refused claim that only reports in Debug reports on the one machine where it will not
happen.

## 8. Across A Session

| Situation           | Behaviour                                                              |
|---------------------|-------------------------------------------------------------------------|
| Document opened     | The register retains the session's reports; `48`'s open reports into it |
| Document closed     | Reports are retained; they describe the run, not the document           |
| Device loss — `06` §4.2 | The register survives; it is not device state                       |
| Application closed  | Nothing is retained                                                     |

🔴 Nothing here is written into the document. A report describes what this run of Slate did on this machine, and a
document carrying another machine's residency measures is a document whose contents depend on who opened it —
`76` §2 and `14` §8 rule the same way for the same reason.

## 9. Precision

| Computation                    | Tier | Reason                                                       |
|--------------------------------|------|---------------------------------------------------------------|
| Report ordinal and count       | A    | Integers; a count that is approximately twelve is not a count |
| Arrival timestamp              | A    | `04` §3's timestamps, carried unmodified                      |
| Claimed and available totals   | A    | Byte extents are integers — `06` §3                           |
| Distortion, rate, fraction     | B    | Presented values, at the tier their producer declared         |

🔴 A measure is presented at the tier its **producer** declared and is never re-derived here. `68`'s distortion is
Tier B because `68` says so; recomputing it for presentation would let the panel disagree with the mechanism about
whether a chart is acceptable.

## 10. Gates

- **Gate:** `DiagnosticPanel` stores nothing; `ReportSequence` and `MeasureIndex` own it.
- 🔴 **Gate:** The register lives in `SlateMath.lib`, reachable by all four units — §3.
- **Gate:** A measure overwrites; it is never appended to `ReportSequence`.
- **Gate:** Every measure is sampled by the tick and never pushed to it.
- **Gate:** A report is appended once per occurrence, carrying its origin and its declared class.
- **Gate:** The class is declared by the reporting mechanism, never inferred from the text.
- **Gate:** `ReportSequence` accepts an append from any thread; nothing reads report order.
- **Gate:** Every "reported through `86`" obligation in the series appears in §4's register.
- **Gate:** Discretionary exhaustion is not reported — `06` §3.
- **Gate:** Coalescing is by origin, class and subject together, never by origin alone.
- **Gate:** Reports are recorded in every configuration, not in Debug only.
- **Gate:** Nothing in the register is written into the document.
- **Gate:** A presented measure is at the tier its producer declared and is never re-derived.

## 11. Open

| Open question                                                             | Blocks                          |
|----------------------------------------------------------------------------|----------------------------------|
| The session bound on `ReportSequence` — by count or by extent held          | Memory only; §6 discards oldest  |
| Whether a report can be written to a stream for a defect report             | Nothing structural               |
| Whether `56` §3.1's resampling report is presented as a confirmation instead   | `56` §3.1 confirms already       |
| Whether a failed item retries — `34` §9 carries the same row                 | §6's volume                      |
| Whether measures are recorded over time or only latest                      | Presentation; `06` §6 timing     |
