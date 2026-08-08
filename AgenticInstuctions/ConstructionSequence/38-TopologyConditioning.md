# 38 — TopologyConditioning

Slate paints; it does not model. `00` §5 declares topology editing absent and names this document as what stands
in its place. Imported topology arrives in whatever state its author left it, and painting on it requires
properties the author had no reason to provide.

🔴 Conditioning **derives**; it never mutates. The imported topology is retained exactly as it arrived, and every
property here is a derived companion to it. An importer that repairs its input is an importer whose output the
artist cannot reconcile with the file they gave it.

## Position In The Sequence

| Field       | Value                                                                        |
|-------------|-------------------------------------------------------------------------------|
| Unit        | `SlateDocument.lib`                                                           |
| Layer       | `Layer3_Document`                                                             |
| Upstream    | `02` (Tier A predicates), `10` (`TopologyStructure`), `34` (off the tick)     |
| Downstream  | `40`, `68`, `16`, `18`, `74`                                                  |
| Unblocks    | Imported topology that is fit to paint on                                    |

## 1. What Is Derived

| Derived                | Needed by                | Consequence when absent                             |
|------------------------|--------------------------|------------------------------------------------------|
| Adjacency              | `40`, `68`               | No traversal; every solve is quadratic              |
| Position welding index | `68`                     | Charts split at a seam that is not a seam           |
| Face orientation       | `16`, `18`               | Surfaces black or invisible from one side           |
| Vertex perpendicular   | `18`, `68`               | Faceted shading on a surface authored smooth        |
| Tangent basis          | `18` §1.1                | Channels 5, 10 and 13 resolve against nothing       |
| Degeneracy enrollment  | `40`, `68`, `74`         | Intersection and unwrap both fail on the same faces |
| Extents                | `40`, `16`, `74`         | No culling; no subdivision                          |

Every entry is a **companion**. The imported arrays are untouched, and an index into the original addresses the
same face after conditioning as before it.

## 2. Welding Is By Position, Not By Index

Two vertices at the same position with different indices are one point on the surface and two points in the file.
Formats produce this constantly, because a format that stores a texture coordinate per corner has already split
every vertex where its coordinates differ.

`10`'s topology keeps both: the imported indexing, and a welding index mapping imported vertices onto positions.
`68` unwraps against the welded positions so a chart does not split at every texture-coordinate discontinuity, and
`18` reads the imported indexing so authored coordinates survive.

Coincidence is decided by the Tier A predicate in `02` §4 at a declared tolerance relative to the extent, never by
comparing coordinates for equality. Exact equality is the test that welds nothing on a file that was written
through a text format.

## 3. Degeneracy Is Enrolled, Never Removed

| Condition                              | Enrolled as | Behaviour                                          |
|----------------------------------------|-------------|-----------------------------------------------------|
| Zero-extent face                       | Degenerate  | Excluded from `40`, `68`, `74`; retained in `10`   |
| Repeated index within one face         | Degenerate  | As above                                           |
| Face with no consistent orientation    | Unoriented  | Rendered both-sided; reported                      |
| Non-manifold edge                      | Non-manifold| `68` cuts a chart boundary at it                   |
| Isolated vertex                        | Isolated    | Ignored by everything but export                   |

Enrollment uses `12` §3's interval mechanism, so testing "is this face degenerate" is an interval comparison and
an excluded population costs nothing to skip.

⚠️ Removal is what an editor does. Removing a face renumbers everything after it, and every index the artist's
file carried — their own selections, their authored coordinates, their material assignment — now addresses the
wrong face. Slate does not own that file and must not do this to it.

## 4. The Tangent Basis

`00` §10 conflict 27 / conflict 39 assigns the per-vertex tangent basis storage and handedness here. Channels 5, 10 and 13 in `18` are tangent-space and resolve against nothing without it.

This document derives and **stores** the per-vertex basis and its handedness, or retains the imported one. `18` §1.1 **interpolates** and re-orthonormalises that stored basis per pixel across the triangle. Handedness is stored per vertex because a domain that mirrors across a seam inverts handedness on one side, which cannot be recomputed per pixel.

Where the artist supplied a basis in the file, the supplied one is retained and stored, and the derived one is not computed. An imported basis that disagrees with the imported perpendicular is the author's decision, and reproducing their appearance requires reproducing it.

## 5. When It Runs

Conditioning is a long solve and runs through `34` at `Interactive` priority, because the artist is waiting and
the workspace shows the occupant unusable until it finishes.

| Trigger                            | Re-derived                                  |
|------------------------------------|----------------------------------------------|
| Topology imported                  | Everything in §1                            |
| `68` re-partitioned the domain     | The tangent basis only                      |
| An imported basis was overridden   | The tangent basis only                      |
| The occupant moved                 | Nothing — every property is in object space |

🔴 Nothing here is in document space. The last row is the reason: an artist arranging a scene must not pay for a
re-derivation per move, and `00` §10.1 ② depends on the same property one layer up.

## 6. Precision

| Computation             | Tier | Reason                                                     |
|-------------------------|------|-------------------------------------------------------------|
| Coincidence             | A    | A wrong answer welds two surfaces or splits one            |
| Orientation             | A    | `02` §4's predicate; a sign error inverts a face           |
| Degeneracy              | A    | As above                                                   |
| Perpendicular, tangent  | B    | Continuous; a small error is a small shading error         |
| Extents                 | B    | Conservative — rounded outward, never inward               |

⚠️ Extents round **outward**. An extent rounded inward excludes geometry from `16`'s culling and from `40`'s
traversal, and the symptom is a surface that disappears at one camera angle.

## 7. Gates

- **Gate:** Imported topology is never mutated; everything here is a derived companion.
- **Gate:** An index into the imported arrays means the same thing after conditioning as before.
- **Gate:** Welding is by position at a relative tolerance, through `02` §4, never by index or exact equality.
- **Gate:** Degenerate geometry is enrolled and excluded, never removed.
- **Gate:** The tangent basis is derived from `68`'s domain, with handedness stored per vertex.
- **Gate:** An imported basis is used as supplied and not overridden by a derived one.
- **Gate:** Every derived property is in object space.
- **Gate:** Conditioning runs through `34`, and its result is applied on the tick as one transaction.
- **Gate:** Extents are conservative outward.

## 8. Open

| Open question                                                          | Blocks                        |
|-------------------------------------------------------------------------|--------------------------------|
| The welding tolerance relative to extent, and whether the artist sets it | `50` intake presentation      |
| Whether a smoothing declaration in the file overrides the derived one    | `50` format coverage          |
| Whether conditioning results are stored in the document or re-derived    | `48` document size            |
