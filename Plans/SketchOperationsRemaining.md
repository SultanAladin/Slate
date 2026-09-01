# The 2D sketch operations still to build

The first batch — **Fillet, Chamfer, Cut, Trim, Extend, Offset, Fill** — is built, proven and wired.
This is the list of what is *not* built. No logic exists for anything below; it is recorded so the
shape of the remaining work is visible rather than remembered.

Booleans are deliberately absent. They are a different kind of operation — area arithmetic on closed
regions rather than editing of curves — and clipper2 is already vendored for them.

---

## The nine

| # | Operation | What it does | The hard part |
|---|-----------|--------------|---------------|
| 1 | **Mirror** | Reflects a selection across a line or an axis | Whether the mirror is a *copy* or a *constraint*. A constrained mirror has to stay mirrored when the original moves, which means it is a relation in the solver rather than a one-off transform. Decide before building. |
| 2 | **Linear array** | Repeats a selection along a direction, *n* times at a spacing | Same question as Mirror, plus: the count and spacing want to be editable after the fact, which makes it parametric rather than a stamp. |
| 3 | **Circular array** | Repeats around a centre, *n* times through a sweep | As above. Additionally, whether the copies rotate with the array or stay upright — both are wanted, so it is an option, not a decision. |
| 4 | **Split at intersection** | Divides *every* curve at *every* crossing, in one action | Mostly built already: `CutWorldCurveAtCrossings` does one curve. The remaining work is doing it across a whole selection without invalidating the names it is iterating over. |
| 5 | **Join** | Merges collinear or tangent-continuous neighbours into one curve | Deciding what may merge. Two collinear lines are obvious; a line and an arc that happen to be tangent are not, because the result is not expressible as a single curve. Probably restricted to same-subject pairs. |
| 6 | **Close / bridge** | Draws the curve that closes an open loop | Which end pairs with which, when a loop has more than one gap. Nearest-endpoint is the obvious rule and is wrong for a `C` whose ends are far apart but which is clearly one shape. |
| 7 | **Project** | Copies edges from other geometry onto the active workplane | Needs the 3D side to exist first. It is the seam between the sketch and the solid, and building it before there are solids to project would fix the wrong interface. |
| 8 | **Convert to construction** | Flips curves between real geometry and reference-only | Cheap and self-contained — a flag on `DeclaredWorldCurve`, honoured by the renderer, the picker and the loop analysis. The likeliest next one to do. |
| 9 | **Scale / rotate about a point** | Transforms a selection about a chosen pivot rather than its own centre | The gesture, not the maths. Choosing the pivot is a click that has to be distinguishable from the click that starts the drag, and the transform session currently assumes the centre of the selection. |
| 10 | **Explode** | Breaks a loop, array or joined curve back into its parts | Knowing what the parts *were*. Once a Join has merged two lines, nothing records that it did — so Explode either needs a record of the merge or is limited to structures that are still visibly composite. |

---

## Notes that apply to all of them

- **Names must survive.** Every operation in the first batch keeps the subject's `WorldCurveName` where
  it possibly can, because loops, constraints, dimensions and selections all refer to curves by name. An
  operation that replaces rather than edits silently invalidates all four.
- **Preview with the commit's own code.** Each of the seven asks the same function for its preview that
  it asks to perform, so the two cannot drift. Anything added here should do the same.
- **A figure means a readout.** The gesture shape follows from whether the operation has a number:
  those that do drag, clamp and raise the readout; those that do not perform on release. Mirror has no
  figure; the arrays have two each.
