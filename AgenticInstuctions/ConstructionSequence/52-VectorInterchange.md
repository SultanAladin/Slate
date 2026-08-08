# 52 — VectorInterchange

Vector outlines and typeface outlines are the same thing at intake: closed and open planar paths with a fill rule.
A typeface glyph is an outline with a name and metrics attached, and treating them as two subsystems produces two
path solvers that disagree on the same curve.

## Position In The Sequence

| Field       | Value                                                                        |
|-------------|-------------------------------------------------------------------------------|
| Unit        | `SlateDocument.lib`                                                           |
| Layer       | `Layer3_Document`                                                             |
| Upstream    | `02` (`CurveSolver`, `PlanarClassifier`, `IncircleClassifier`), `10` (codecs), `36` |
| Downstream  | `70` resolves outlines into the domain; `72` places them; `82` previews them  |
| Unblocks    | Vector outlines and typeface outlines as content                              |

## 1. Two Intake Routes, One Result

🔴 A vector source arrives either as a file or as **source text supplied directly** — pasted markup. Both routes
produce the identical `OutlineSpecification` and nothing downstream can tell which was used.

| Route         | Arrives through                                      | Held afterwards as                     |
|---------------|------------------------------------------------------|-----------------------------------------|
| File          | `StorageExchange` → `VectorCodec`                    | The specification plus the origin path |
| Supplied text | `ClipboardExchange` or a panel field → `VectorCodec` | The specification plus the source text |

⚠️ The supplied-text route holds the text because there is no file to re-read. A source whose only copy was a
clipboard is unrecoverable after a reopen, and the artist reads that as the document having lost their work.

Both routes are decoded by `VectorCodec` from `10` §1, which translates the stream and does nothing else — a codec
that repaired malformed geometry would produce a specification that no longer describes what was supplied.

## 2. The Accepted Subset

`00` §5.2 closes this. Restated as the intake contract rather than as an absence:

| Accepted                                                | Held as                                            |
|---------------------------------------------------------|-----------------------------------------------------|
| Path geometry — lines, quadratic and cubic curves, arcs | Control positions at Tier B                        |
| Both fill rules — non-zero, even-odd                    | A declared property of each closed path            |
| Stroke geometry                                         | Converted to outline at intake, never as a width   |
| Transforms                                              | Composed into the path at intake                   |
| Gradients — linear, radial                              | A declared colour progression in `36`'s space      |

Everything outside this is refused at intake with its reason reported through `86`. 🔴 A refusal names the construct
and the position in the source. "Unsupported" with no position sends the artist to search a file they did not
write.

Stroke conversion happens at intake for one reason: a stroke width is a distance in the source's own space, and a
placement scales that space. A stroke held as a width thins when the placement shrinks, which is correct for a
drawing program and wrong for content placed onto a surface at a chosen size.

## 3. Typefaces

A typeface supplies named glyph outlines and metrics. `TypefaceCodec` from `10` §1 produces them; this document
holds them as outlines identical in structure to §2's.

| Held                | Meaning                                                        |
|---------------------|-----------------------------------------------------------------|
| Glyph outlines      | Paths, indexed by glyph identity, not by character              |
| Advance and bearing | Per glyph, in the typeface's own units                          |
| Substitution        | Character sequence to glyph sequence, as the typeface declares  |
| Positioning         | Pair adjustment between adjacent glyphs                         |

🔴 Text is resolved to a **glyph sequence** at intake and the glyph sequence is what is stored. The character
string is stored beside it so the text stays editable. Storing only characters means every resolution re-runs
substitution, and substitution depends on the typeface — so replacing a typeface silently changes the shaping of
text the artist already positioned.

⚠️ Whether a typeface is embedded on save or referenced is open in `00` §12 and is not decided here. Both are
compatible with this section: what is stored is the glyph sequence and the typeface identity either way.

## 4. Evaluation

Outlines are resolved, never stored as texels. Two `02` components carry it, both declared with `52` as their
consumer:

| Component          | Tier | Answers                                                    |
|--------------------|------|-------------------------------------------------------------|
| `CurveSolver`      | B    | Position along a path; flattening to a tolerance            |
| `PlanarClassifier` | A    | Whether a position is inside, on, or outside a closed path  |

🔴 `PlanarClassifier` is Tier A and therefore parity-proven, because the same outline is classified on the host
for `82`'s preview and on the device for `70`'s resolution. A winding test that disagrees between the two gives a
preview with a different silhouette from the result, and the artist attributes the difference to the preview being
approximate rather than to the classification being wrong.

Flattening tolerance is **resolution-relative**, not fixed. `70` resolves an outline at whatever reduction level a
tile was promoted to, so a fixed tolerance is either wasteful at coarse levels or visibly polygonal at fine ones.

## 5. Gates

- **Gate:** A file source and a supplied-text source produce identical specifications.
- **Gate:** A supplied-text source stores its text; nothing depends on a clipboard surviving.
- **Gate:** Strokes are converted to outline at intake; no stroke width is stored.
- **Gate:** Every refusal names the construct and its position in the source, through `86`.
- **Gate:** Text stores a glyph sequence and its characters, never characters alone.
- **Gate:** Interior classification is Tier A and parity-proven.
- **Gate:** Flattening tolerance is relative to the reduction level being resolved.
- **Gate:** No outline is stored as texels at any resolution.

## 6. Open

| Open question                                                       | Blocks                        |
|----------------------------------------------------------------------|--------------------------------|
| Whether right-to-left and vertical text are in the accepted subset   | Shaping only; `72` unaffected |
| Whether gradient interpolation is in working space or source space   | `36` decides                  |
| Whether a typeface is embedded or referenced on save                 | `00` §12 carries this         |
