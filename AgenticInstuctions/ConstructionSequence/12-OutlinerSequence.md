# 12 — OutlinerSequence

An outliner is a depth-first linearisation of a nesting relation over a generationally versioned slot population.
It is not a widget with rows. The rows are the visible consequence; the mechanism is the linearisation, and it is
a first-class engine subsystem that the interface happens to display.

The central design decision, and the one that cannot be deferred: there are **two nesting relations over the same
population, not one**. Organisational containment — how the artist has grouped things — and kinematic containment
— what moves when something moves — are different relations. Deriving either from the other terminates in a
transform-override bolt-on, and that bolt-on is a rewrite rather than a patch.

## Position In The Sequence

| Field       | Value                                                                            |
|-------------|-----------------------------------------------------------------------------------|
| Units       | `SlateDocument.lib` (the relations and linearisation), `SlateUI.lib` (presentation) |
| Layers      | `Layer3_Document`, `Layer5_Interface`                                             |
| Upstream    | `02` (transforms, containment predicate), `10` (population, properties, revisions)  |
| Downstream  | `14` presents rows; `16` culls by the kinematic relation; `26` outlines the selection |
| Unblocks    | Scene navigation, selection, grouping, anything that moves with something else     |

## 1. The Two Relations

| Relation           | Meaning                                     | Governs                                    |
|--------------------|---------------------------------------------|---------------------------------------------|
| `EnclosureContains`| Organisational containment                  | Row order, visibility inheritance, grouping |
| `AttachmentFollows`| Kinematic containment                       | Transform composition, motion propagation   |

An occupant is enclosed by at most one enclosure and attached to at most one attachment. The two are independent:
an occupant may be organised into one group while moving with something in another, which is exactly the case
that a single-relation design cannot express without an override.

🔴 `AttachmentFollows` is the transform-composition relation. `EnclosureContains` never composes a transform. A
row indented under another row does not, by that fact alone, move with it.

⚠️ Kinship vocabulary is banned throughout: no `Parent`, `Child`, `Sibling`, `Ancestor`, `Descendant`, `Orphan`.
Say enclosing occupant, enclosed occupant, enclosure depth, attachment root.

## 2. Linearisation

Depth-first traversal of `EnclosureContains` produces `RowSequence`, the ordered row list. Two mechanisms keep it
from being rebuilt whenever anything changes.

### 2.1 Gapped interval labelling

Each occupant carries an interval label, and enclosure is answered by interval containment — a comparison, not a
traversal. Labels are assigned with gaps so that an insertion consumes a gap rather than relabelling. Relabelling
happens only when a local gap is exhausted, and then only across the exhausted span.

This makes "is A enclosed by B, at any depth" a Tier A integer comparison. `16` and `26` both ask it per occupant
per rotation, and neither can afford a traversal to answer it.

### 2.2 Counted ordering

`RowSequence` is paired with `RankIndex`, a counted structure answering two questions in logarithmic time:

- Which occupant is at visible row N — for scrolling to an arbitrary position.
- What visible row is occupant X at — for scrolling to a selection.

Collapsed enclosures and filtered-out occupants are excluded from the count without being removed from the
sequence, so expanding a collapsed enclosure is a count adjustment rather than a rebuild.

## 3. Subsets

`EnrollmentIndex` records which slots are enrolled in a named subset — selection, visibility exclusion, isolation,
lock. Subsets are compressed by interval rather than stored per occupant, because a subset over a scene is
overwhelmingly contiguous in row order and storing it densely wastes both memory and comparison time.

`TrigramIndex` supports name search over the population, returning candidate slots which are then confirmed
exactly. Approximate index, exact confirmation — an index that answers alone will eventually answer wrongly.

⚠️ `MembershipRegion` and `MembershipIndex` are retired spellings. `Region` is banned; the mechanism is enrollment.

## 4. Tick Order

Fixed, and every ordering is load-bearing.

① Apply committed transactions from `RevisionSequence`.
② Reconcile the population — resolve additions, and retire slots whose generation advanced.
③ Reconcile `AttachmentFollows`, then compose transforms downward from each attachment root.
④ Reconcile `EnclosureContains`, then repair interval labels where gaps were exhausted.
⑤ Rebuild the affected span of `RowSequence` and adjust `RankIndex`.
⑥ Re-derive subsets whose enrollment changed.
⑦ Re-derive `TrigramIndex` entries for occupants whose name changed.

Step ⑦ is not optional and was previously absent: `TrigramIndex` is declared in §3 and appeared in no step, so a
renamed occupant kept its former name in search until something else forced a rebuild. Search that answers with a
name the artist has already changed is worse than search that finds nothing.

Attachment before enclosure (③ before ④) is deliberate: transforms must be final before anything spatial is
derived from them. Enclosure repair before row rebuild (④ before ⑤) is deliberate: rebuilding rows against stale
labels produces an order that is briefly wrong and is displayed.

## 5. Invariants

| # | Invariant                                                                     |
|---|--------------------------------------------------------------------------------|
| 1 | Every occupant is enclosed by at most one enclosure                            |
| 2 | Every occupant is attached to at most one attachment                           |
| 3 | Neither relation contains a cycle                                              |
| 4 | Interval labels are strictly nested and never overlap between disjoint enclosures |
| 5 | `RankIndex` counts agree with the visible subset of `RowSequence`              |
| 6 | Every enrolled slot in every subset is occupied at the current generation      |
| 7 | An occupant's composed transform depends only on its attachment root path      |
| 8 | Retiring a slot removes it from both relations and from every subset           |
| 9 | Row order is fully determined by `EnclosureContains` and enclosure ordering    |
| 10| No linearisation is observed between ④ and ⑤ within a tick                     |

Invariants 3 and 4 are checked in Debug on every reconciliation. The remainder are checked on transaction commit.

## 6. Cost

| Storage                                        | Bytes per occupant |
|------------------------------------------------|---------------------|
| Slot, generation, both relations, interval label| 108                |
| Without the attachment relation                 | 92                 |
| Row and rank participation only                 | 66                 |

📝 108 bytes per occupant is the design point. At one million occupants that is 108 MB of relation data, which is
why subsets are interval-compressed and why enclosure is answered by comparison rather than traversal.

## 7. Presentation

The `SlateUI` half renders `RowSequence` through `RankIndex` — only the visible span is ever touched, and the
scroll position is a row index resolved by count. The presentation half holds no relation state of its own. It
reads the linearisation and writes intent (expand, collapse, select, reorder) back as transactions.

🔴 Reordering rows is a transaction against `EnclosureContains`, committed through `RevisionSequence` like any
other edit. Drag-reordering that mutates the relation directly bypasses undo, and its absence from the revision
sequence is discovered by the artist rather than by a test.

## 8. Build Order

| Step | Delivers                                                        |
|------|-------------------------------------------------------------------|
| A    | Population reconciliation over `10`'s slot ledger                |
| B    | `EnclosureContains` with interval labelling and gap repair       |
| C    | Depth-first `RowSequence`                                        |
| D    | `RankIndex` counted ordering                                     |
| E    | `AttachmentFollows` and downward transform composition           |
| F    | `EnrollmentIndex` with interval compression                      |
| G    | `TrigramIndex` name search with exact confirmation               |
| H    | Presentation in `SlateUI`, reading only                          |
| I    | Intent as transactions — expand, collapse, select, reorder       |

## 9. Gates

- **Gate:** The two relations are separately stored and separately reconciled.
- **Gate:** `EnclosureContains` composes no transform.
- **Gate:** Enclosure containment is answered by interval comparison, not traversal.
- **Gate:** All ten invariants hold at every tick boundary.
- **Gate:** The presentation half stores no relation state.
- **Gate:** Every mutation arrives as a transaction — document subsets through `RevisionSequence`, selection
  through `SelectionSequence`. There is no unrecorded mutation.
- **Gate:** Retiring an occupant commits its entire cascade as one transaction.
- **Gate:** A rename re-derives `TrigramIndex` within the same tick.
- **Gate:** A relation change that would create a cycle is rejected at commit and reported through `86` as a
  Refusal, naming both occupants, never applied.
- **Gate:** No kinship word appears in any identifier.

## 10. Open

Carried from `02-OutlinerPlan.md` §18 with the recommendation stated. Each is a real decision, not a placeholder.

| Open question                                             | Recommendation                          | Blocks       |
|-----------------------------------------------------------|------------------------------------------|---------------|
| Gap size for interval labelling                          | Sized to expected enclosure width       | Tuning only   |
| Whether attachment may cross enclosure boundaries        | Yes — the reason for two relations      | Nothing       |
| Multi-enrollment in mutually exclusive subsets           | Rejected at commit, not resolved        | `14` feedback |
| Whether row narrowing is a subset or a predicate         | Subset — already interval-shaped        | `14` only     |

⚠️ "Whether subsets are revisioned" is **closed**, not open — see §11. Carrying it as a recommendation while §9
gated every mutation through `RevisionSequence` was a contradiction between two sections of this document.

## 11. Subsets And Revision

🔴 Every subset mutation is a transaction, without exception. §9's gate says so and it is not qualified here.
What differs between subsets is **where the transaction is recorded**, not whether it is one.

| Subset               | Recorded in         | Scrubbed by undo |
|----------------------|---------------------|-------------------|
| Visibility exclusion | `RevisionSequence`  | Yes               |
| Lock                 | `RevisionSequence`  | Yes               |
| Isolation            | `RevisionSequence`  | Yes               |
| Selection            | `SelectionSequence` | Separately        |

Selection is recorded in its own ordered sequence rather than the document's. It survives save and load, it has
its own backward and forward traversal, and it is **restored alongside** any transaction that depends on it —
so undoing a move restores both the transform and the selection that transform was applied to.

🔴 Amended: selection survives **for the session**, not across save and load. `48` §2 rules it session state and
gives the reason — a document reopening with someone else's selection has restored a decision the artist had
already finished making, and the first stroke lands on the wrong occupant. Nothing this section needs requires
disk persistence: what is load-bearing is that a scrub restores the selection its transaction applied to, and
that holds within the session where the scrub happens. Recorded as `00` §10 conflict 34.

⚠️ The naive reading of "selection is not revisioned" produces a defect the artist finds in under a minute: move
three occupants, undo, and the transforms revert while the selection does not, so the next action applies to
something other than what the undo appeared to restore. Selection is not in the document's revision sequence, but
it is not unrevisioned either.

## 12. Retirement Cascade

Invariant 8 retires a slot from both relations and from every subset. That is necessary and not sufficient — it
says nothing about what the retired occupant contained or owned, and both are load-bearing.

| On retiring an occupant                    | Cascade                                                     |
|--------------------------------------------|--------------------------------------------------------------|
| It encloses other occupants                | Declared policy — see below; never left undefined            |
| It is an attachment root for others        | Attached occupants retain their composed transform, reattach to its attachment |
| It owns surface content in `56`            | Layers retire with it; the extents they touched are invalidated |
| It owns resident tiles in `20`             | Tiles are reclaimed after the rotation depth, per `20` §5    |
| It owns device partitions from `16`        | Partitions and `42`'s resolution entries are derived again   |
| It has placed content enclosed under it    | Placements retire with it — `00` §10.1                       |

🔴 Enclosure retirement policy: enclosed occupants are **re-enclosed by the retiring occupant's enclosure**, not
retired with it. Deleting a group deletes the group, not the work inside it. Deleting the contents is a separate
instruction the artist gives deliberately, and conflating the two loses work that undo then has to rescue.

⚠️ Retirement is one transaction including its whole cascade. A cascade committed as several transactions is
undone in pieces, and the intermediate pieces are states the document was never actually in.
