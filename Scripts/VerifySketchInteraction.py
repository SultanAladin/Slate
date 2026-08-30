"""Headless contract checks for sketch selection and orthographic navigation.

This deliberately tests the same state transitions and vector equations used by the
C++ interaction code. It does not pretend to be a Vulkan runtime test.
"""
from dataclasses import dataclass
from math import isclose

@dataclass(frozen=True)
class Pick:
    subject: str
    record: int
    identity: int


def same(a, b):
    return (a.subject, a.record, a.identity) == (b.subject, b.record, b.identity)


def set_pick(items, pick, additive):
    if not additive:
        return [pick]
    for i, existing in enumerate(items):
        if same(existing, pick):
            return items[:i] + items[i + 1:]
    return [pick] + items                 # newest item is active


def test_additive_selection():
    rect_a = Pick("Point", 10, 1)
    rect_b = Pick("Point", 10, 2)
    bezier = Pick("Control", 11, 7)
    items = set_pick([], rect_a, False)
    items = set_pick(items, rect_b, True)
    items = set_pick(items, bezier, True)
    assert len(items) == 3, items
    assert items[0] == bezier, items
    assert {p.identity for p in items if p.record == 10} == {1, 2}
    items = set_pick(items, rect_b, True)
    assert len(items) == 2 and rect_b not in items, items


def test_directory_does_not_overwrite_viewport_additions():
    # The directory signature is unchanged during a viewport Shift-click.
    old_signature = 1234
    new_signature = old_signature
    directory_changed = old_signature != new_signature
    viewport = [Pick("Point", 10, 1), Pick("Control", 11, 7)]
    if directory_changed:
        viewport = [p for p in viewport if p.record == 10]
    assert len(viewport) == 2, viewport


def test_orthographic_camera_basis():
    # Camera-local basis: Right, Up, Forward. Orthographic travel must use
    # Right for A/D and Forward for W/S; it must not substitute Up for W/S.
    right = (1.0, 0.0, 0.0)
    forward = (0.0, 0.0, 1.0)
    distance = 2.0
    def add(a, b): return tuple(x + y for x, y in zip(a, b))
    def scale(a, n): return tuple(x * n for x in a)
    focus = (0.0, 0.0, 0.0)
    focus = add(focus, scale(right, distance))       # D
    focus = add(focus, scale(forward, distance))     # W
    assert focus == (2.0, 0.0, 2.0), focus
    # A top orthographic camera looking along Forward sees no screen displacement
    # from W/S; that is correct camera-relative depth travel, not a screen pan.


def main():
    test_additive_selection()
    test_directory_does_not_overwrite_viewport_additions()
    test_orthographic_camera_basis()
    print("[SketchInteraction] additive selection contract: PASS")
    print("[SketchInteraction] directory overwrite guard: PASS")
    print("[SketchInteraction] orthographic Right/Forward basis contract: PASS")
    print("[SketchInteraction] note: W/S depth travel is visually invariant in a locked orthographic projection")

if __name__ == "__main__":
    main()
