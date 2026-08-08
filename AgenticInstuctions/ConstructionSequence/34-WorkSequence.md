# 34 — WorkSequence

Some things Slate must do take longer than one tick and cannot be made shorter. Unwrapping a chart partition,
conditioning imported topology, decoding a large image, resolving an export at an extent no one has resident —
each is seconds of work, and each must happen while the artist keeps painting.

This document is the one mechanism that runs such work off the tick. Nothing else in the series is permitted to
create a thread.

## Position In The Sequence

| Field       | Value                                                                        |
|-------------|-------------------------------------------------------------------------------|
| Unit        | `SlateMath.lib`                                                               |
| Layer       | `Layer1_Numeric`                                                              |
| Upstream    | `00`, `04` (threads, `TickSequence`)                                          |
| Downstream  | `38`, `50`, `68`, `20`, `24`, `70`, `82` — every long solve in the series     |
| Unblocks    | Long solves off the tick; cancellation and progress                           |

## 1. The Components

| Component            | What it owns                                                          |
|----------------------|------------------------------------------------------------------------|
| `WorkSequence`       | The workers, their lifetime, and the dispatch order                    |
| `WorkQueue`          | Pending work at one priority level                                     |
| `ProgressMetrics`    | What a long solve reports while it runs                                |
| `CompletionExchange` | Results crossing back to the tick, and only there                      |

Worker count is chosen once at bring-up from the host report and recorded, so `HardwareMetrics` can attribute a
measurement to the concurrency that produced it.

## 2. Declaring Work

Work is declared, not spawned. A declaration names its priority, its cancellation points, and what it produces.

| Declared          | Meaning                                                            |
|-------------------|---------------------------------------------------------------------|
| Priority          | `Interactive`, `Background` or `Deferred` — §4                     |
| Inputs            | Read-only for the whole run; nothing here reads mutable state       |
| Produces          | A result crossing back through `CompletionExchange`                 |
| Cancellation      | The declared points at which cancellation is observed               |
| Progress          | Whether the work reports a fraction, a count, or nothing            |

🔴 A work item reads inputs that are **immutable for its whole run**. It may not read the document, the tick's
state, or anything in `76`. The requester captures what the work needs at declaration and hands it over.

⚠️ This is the rule that makes every other rule here unnecessary. Work that reads live state needs a lock; a lock
held by a background solve stalls the tick, which is the exact outcome this document exists to prevent.

## 3. Results Cross Back On The Tick

🔴 A result is applied by the **requester**, on the tick, after `CompletionExchange` delivers it. A work item never
mutates the document, never commits a transaction, and never records into a device recording.

| Producer          | What crosses back                    | Applied by                        |
|-------------------|--------------------------------------|------------------------------------|
| `38`              | A conditioned topology                | Intake, as one transaction         |
| `68`              | A chart partition and its domain      | Intake, as one transaction         |
| `50`              | Decoded topology or imagery           | Intake, as one transaction         |
| `20`              | A resolved tile's texels              | Promotion, into the physical extent |
| `70`              | Resolved analytic content for a tile  | Promotion, as above                |

⚠️ A work item that mutated the document directly would linearise against `RevisionSequence` from a thread that
does not observe the tick's ordering — `12` invariant 10 forbids exactly this, and it forbids it because the
outliner would reconcile against a population that changed underneath it.

## 4. Priority

| Priority      | Meaning                                                        | Starves |
|---------------|-----------------------------------------------------------------|---------|
| `Interactive` | The artist is waiting and the workspace shows a gap             | Never   |
| `Background`  | Wanted soon; the artist has not asked for it directly           | Yields  |
| `Deferred`    | Speculative — resolved because it is likely, not because asked  | Yields  |

At least one worker is reserved for `Interactive`. A residency promotion under the cursor and a full-document
export are both long solves, and the export must not be able to occupy every worker.

🔴 A work item may not wait on another work item. With a bounded worker count, waiting is a deadlock that appears
only under load, on someone else's machine. Work that has phases declares the phases as separate items chained by
`CompletionExchange`, on the tick, where the chaining is visible.

## 5. Cancellation

Cancellation is cooperative and observed only at declared points. A cancelled item produces no result, and its
requester is told it was cancelled rather than being left waiting.

| Cause                                        | Behaviour                                      |
|----------------------------------------------|-------------------------------------------------|
| The requester withdrew the request            | Cancelled at the next declared point           |
| The document it read was closed               | Cancelled; the result is discarded             |
| A newer request supersedes it                 | Cancelled; supersession is reported            |
| The item failed                               | Reported through `86` with its origin          |

⚠️ Cancellation is not abandonment. An item whose result is discarded still runs to its next declared point and
releases what it holds. A worker that is simply never joined leaks its inputs, and the leak is proportional to how
often the artist changes their mind.

## 6. Determinism

🔴 A result must not depend on how many workers ran it or in what order they finished. Where a solve is split
across workers, the split is by a declared index and the parts are recombined by index, never by completion.

This is not tidiness. `68`'s unwrap keys every painted texel in the document; `02` §7's parity proves agreement at
a declared tier. Both stop meaning anything if the same input produces two results on two machines.

Accumulation across workers uses the ordered recombination in `02` §5, not addition in arrival order — a sum in
arrival order is a different number each run at Tier B, and the difference shows up as a seam.

## 7. Progress

`ProgressMetrics` reports a fraction or a count, sampled by the tick and never pushed. A long solve that pushes
progress at its own rate contends with the tick for the very state the tick is presenting.

Progress reaching a presented panel is `86`'s concern. What is reported here is the measure only.

## 8. Gates

- **Gate:** No thread is created anywhere in the repository except by `WorkSequence`.
- **Gate:** A work item's inputs are immutable for its whole run.
- **Gate:** No work item mutates the document, commits a transaction, or records into a device recording.
- **Gate:** Every result is applied by its requester, on the tick.
- **Gate:** No work item waits on another work item.
- **Gate:** At least one worker is reserved for `Interactive` work.
- **Gate:** A split solve recombines by declared index, never by completion order.
- **Gate:** A cancelled item reports cancellation; it never leaves its requester waiting.
- **Gate:** Progress is sampled by the tick, never pushed to it.

## 9. Open

| Open question                                                          | Blocks                       |
|-------------------------------------------------------------------------|-------------------------------|
| Whether worker count is host-derived or declared in settings            | Tuning only                   |
| Whether `Deferred` work runs at all on a machine under memory pressure  | `20` budget policy            |
| Whether a failed item retries, and how often                            | `86` reporting volume         |
