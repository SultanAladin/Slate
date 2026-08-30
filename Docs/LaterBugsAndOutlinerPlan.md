# Slate: Later Bugs and Structural Plan

**Status:** Planning only. No implementation from this document has been started.

## Deferred note from the viewport/flicker work

> The phase is complete, but the overall flicker investigation is not yet fully complete. The remaining unresolved item is Vulkan frame-resource lifetime and synchronization validation, which requires checking the actual command-buffer submission/fence path—something the static checks cannot prove.

This remains an explicit acceptance item. Static checks and interaction proofs pass, but they are not a live Windows/MSVC/Vulkan capture.

## Current and newly reported issues

| Area | Reported/problem state | Priority | Validation needed |
|---|---|---:|---|
| Vulkan rendering | Intermittent rendered-shape flicker is not conclusively resolved. Upload lifetime, command-buffer submission, fences, and mapped-buffer reuse still need runtime validation. | P0 | Windows/MSVC/Vulkan build, GPU capture, validation layers, resize and split-view stress |
| Outliner | The Outliner/parametric directory may be blank, removed, or rewired. The current code has `SceneDirectory` and a separate `SketchDirectory`; this needs an evidence-based audit before changing it. | P0 | Add shape, inspect rows, select, expand/collapse, rename, delete/undo, reload |
| Parametric hierarchy | Curves, polygons, points, lines, circles, and ellipses need stable folders and correct insertion into their folders. | P0 | Every creation operation must produce the expected tree path |
| Parametric properties | Selecting an item must expose its properties and keep them synchronized with the selected record. | P1 | Select each supported shape and edit/read properties |
| Parametric history | History must support previewing and moving backward/forward for the selected shape without corrupting the directory or scene. | P0 | Undo/redo, selection changes, preview, branch-after-undo |
| Mouse movement | The pointer appears to drift/stick toward the left edge and highlights the resize divider in blue. It is not necessarily a rendering bug; it may be pointer capture, coordinate conversion, edge clamping, or divider hover state. | P0 | Log OS pointer position, viewport pointer position, capture owner, divider hit result, and resize activation separately |
| Curve subdivision | Curves become visibly polygonal when zoomed close to a point. | P1 | Close zoom, pan, rotate, large and small curves, compare screen-space error |
| Themes/panels | UI appears translucent where solid fills are required, including the selection menu. Transparency should be replaced with equivalent opaque colours. | P1 | Inspect all panel, popup, dropdown, drawer, and selection-menu backgrounds |
| Existing interaction requirements | Mode-specific gizmos, planar pads, 2D visibility, close-zoom gizmo visibility, complete curve controls, opaque panels, UI event isolation, and empty-space deselection must remain intact. | P0 | Existing proofs plus focused manual checks |

## Proposed Outliner structure

The exact names should follow the existing code vocabulary after the audit. The intended logical structure is:

```text
Scene
├── Parametric
│   ├── Profiles
│   │   ├── Curves
│   │   │   ├── Bezier
│   │   │   ├── Hermite
│   │   │   ├── Basis Splines
│   │   │   └── NURBS
│   │   ├── Polygons
│   │   ├── Lines
│   │   ├── Points
│   │   ├── Circles
│   │   └── Ellipses
│   ├── Modifiers
│   └── Construction
│       ├── Workplanes
│       ├── Guides
│       └── Measurements
├── Scene Objects
├── CAD References
└── Lighting
```

### Naming decision

`Parametric` is the preferred user-facing root name. `Profiles` should contain drawable profile/shape families. `ParametricDirectory` is a possible implementation name, but the user-facing tree should not expose an implementation-specific name unless it is clearer in context.

If the existing scene model already has a canonical folder/category vocabulary, that vocabulary wins. The implementation should not create duplicate parallel trees merely to make the UI look correct.

## Directory data model to verify or implement

```text
ParametricDirectory
├── Folder records
│   ├── stable ID
│   ├── parent ID
│   ├── display name
│   ├── expanded state
│   └── ordering/index
├── Shape records
│   ├── stable ID
│   ├── parent folder ID
│   ├── shape kind
│   ├── display name
│   ├── geometry/control data
│   ├── properties
│   └── history reference
└── Selection state
    ├── selected item ID
    ├── hovered item ID
    └── active edit/preview state
```

Every creation path should use one insertion API, conceptually:

```text
CreateShape(kind, properties, geometry)
    -> choose canonical folder from kind
    -> allocate stable shape ID
    -> insert shape record
    -> append history entry
    -> rebuild/synchronise directory rows
    -> select new shape
```

This avoids separate ad hoc insertion logic for curves, polygons, circles, and ellipses.

## Properties and history model

A selected shape should provide:

```text
Selection
    -> properties panel reads selected stable ID
    -> geometry and parameters are edited through a command
    -> command creates a history entry
    -> directory and viewport are rebuilt from the resulting state
```

History should be command/snapshot based rather than a UI-only list:

```text
Shape history
├── initial creation
├── property edit
├── control-point edit
├── transform
├── topology/geometry edit
└── delete/restore
```

Required behaviors:

- Backward/forward navigation affects the selected shape state.
- Preview does not destroy the current committed state.
- Selecting another shape changes the inspected history.
- Editing after going backward creates a new branch or explicitly truncates the forward branch.
- Directory selection remains stable through undo/redo where the shape still exists.
- Deleted/restored shapes retain stable identity where possible.

## Audit-first implementation sequence

### Phase A — Outliner evidence audit

1. Trace `SceneDirectory`, `SketchDirectory`, `SceneDirectoryRows`, `SketchDirectoryRows`, and their panel recording paths.
2. Trace where parametric records are created and where folder/category values are assigned.
3. Identify whether the logic exists but is not rendered, is rendered into the wrong panel, or was removed.
4. Add focused proof cases before rewriting anything.
5. If the logic is substantially missing or broken, replace it with one canonical parametric-directory path embedded in the scene-directory presentation.

### Phase B — Canonical hierarchy

1. Define stable folder/category IDs.
2. Map every shape kind to exactly one canonical folder.
3. Make all creation operations call the same insertion path.
4. Embed parametric rows into the Scene Directory tree rather than maintaining an isolated, invisible outliner.
5. Preserve scene objects, CAD references, and lighting as sibling scene categories.

### Phase C — Selection, properties, and history

1. Make stable IDs the common key between tree row, selection, properties, viewport, and history.
2. Make property edits commands/snapshots.
3. Add selected-shape history navigation and non-destructive preview.
4. Validate all supported shape families: curves, polygons, points, lines, circles, and ellipses.

## Mouse/divider investigation and proposed fix

The desired behavior is not to hide the divider or disable resizing. The issue is to stop a normal mouse movement from being interpreted as pointer relocation or persistent divider hover.

Instrument these values independently:

```text
OS cursor position
window/client position
logical pointer position
physical pointer position
pointer capture owner
active viewport leaf
divider rectangle hit result
resize gesture active
pointer dispatch owner
```

Likely causes to distinguish:

1. The application is warping the cursor to keep a drag inside a region.
2. Logical/physical coordinate conversion is clamping X to the client edge.
3. A resize divider is using hover geometry that is too wide or stale.
4. Pointer capture is not released after a resize or viewport drag.
5. A relative mouse-look path is active when no camera drag is engaged.

Proposed policy:

```text
Normal move:
    never warp cursor
    update hover only

Resize press on divider:
    claim DividerResize
    capture pointer
    resize while held

Resize release/cancel:
    release capture
    clear DividerResize
    recompute divider geometry
```

The divider should be highlighted only from current pointer position and should not claim a press unless the pointer is actually pressed within the divider hit area.

## Adaptive curve subdivision plan

Use screen-space error rather than a fixed world-space segment count.

Conceptually:

```text
for each curve span:
    start with one segment
    recursively split until:
        projected midpoint deviation <= pixel tolerance
        projected segment length <= maximum pixel length
        depth reaches a safe maximum
```

Recommended starting policy:

```text
pixel tolerance: 0.35–0.75 px
maximum projected segment length: 6–12 px
minimum subdivision: 4 segments for non-linear spans
maximum subdivision: bounded per curve/span
```

The tolerance should be evaluated after projection using the active viewport camera. Close zoom therefore increases tessellation automatically, while distant curves remain inexpensive. The same evaluator should feed preview and committed rendering to prevent them from looking different.

Safeguards:

- Clamp recursion/segment counts.
- Handle degenerate spans.
- Avoid infinite subdivision when projection is unstable near the camera plane.
- Cache by geometry revision, camera/projection revision, viewport size, and pixel scale.
- Invalidate when zoom, pan, rotation, geometry, or drawable scale changes.

## Opaque theme/colour plan

Do not simulate opacity by lowering alpha. Resolve the desired composited appearance into an opaque colour against the known panel background.

Examples:

```text
75% black appearance on background B:
    RGB = 0.25 * B
    alpha = 1.0

50% black appearance on background B:
    RGB = 0.50 * B
    alpha = 1.0

25% black appearance on background B:
    RGB = 0.75 * B
    alpha = 1.0
```

For a general foreground colour `F` and background `B`:

```text
opaque replacement = alpha * F + (1 - alpha) * B
alpha = 1.0
```

Implementation sequence:

1. Locate theme colours and all popup/panel background alpha assignments.
2. Separate window/background colours from text, border, hover, and active colours.
3. Resolve each translucent fill against its actual parent background.
4. Set fill alpha to fully opaque.
5. Ensure draw ordering cannot reveal the viewport behind UI panels.
6. Check selection menus, dropdowns, Content Browser, Control Centre, drawers, and properties panels.

Do not globally replace every alpha value: text anti-aliasing, shadows, disabled-state styling, and intentional hover effects may still need controlled alpha. The requirement applies to panel and menu fills.

## Validation gates before declaring completion

### Static/build gates

```text
python3 Tools/ValidateHostBuildBudgets.py
python3 Scripts/VerifyNaming.py
git diff --check
python3 Tools/WorldSketchInteractionProof/WorldSketchInteractionProof.py
```

### Required focused checks

- Outliner rows are non-empty after creating every supported shape family.
- New shapes appear in the correct folder.
- Selecting a row selects the matching viewport shape.
- Properties and history follow the stable selected ID.
- Undo/redo and preview preserve directory integrity.
- Mouse movement does not warp or clamp to the divider.
- Divider hover does not imply resize capture.
- Curves remain smooth at close zoom and bounded at distant zoom.
- Panels and menus are visibly opaque.
- UI clicks never leak into viewport interaction.
- Split viewport state and CAD rendering remain independent.
- Windows/MSVC/Vulkan runtime validation confirms command-buffer and mapped-resource lifetime.

## Definition of done

This work is complete only when:

1. The Outliner is populated and structurally canonical.
2. Parametric content is visibly embedded in the Scene Directory.
3. Every supported shape is inserted into the correct folder.
4. Properties and per-shape history work through stable IDs.
5. Pointer movement no longer drifts or falsely activates the divider.
6. Curve subdivision adapts to projected screen-space detail.
7. Panel/menu fills are opaque while preserving the intended visual shades.
8. The remaining Vulkan lifetime issue is either proven safe by runtime validation or fixed with verified synchronization/resource ownership.
