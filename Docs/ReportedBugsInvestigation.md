# Reported Bugs: Code Investigation

**Date:** 2026-08-30
**Scope:** Investigation only; no bug fixes implemented in this report.

## Executive summary

The reports were investigated against the current source rather than only copied into a plan.

| Issue | Finding | Confidence |
|---|---|---:|
| Outliner blank/broken | Parametric directory logic exists and is wired, but it is a separate `SketchDirectory` panel, not embedded into `SceneDirectory`. The source does not currently prove that it is blank at runtime; it does prove the requested unified scene-tree structure is not implemented. | High |
| Correct shape folders | Generic folder/category support exists (`ParentFolder`, `FolderCategory`, folder rows), but there is no evidence of the requested dedicated folders for curves, polygons, points, lines, circles, and ellipses. Shape subjects are broader (`OpenCurve`, `ClosedProfile`, `Point`, etc.). | High |
| Properties/history | Property projection and revision presentation exist and are connected to parametric records. This is not merely absent. It needs runtime verification for all shape types and selection/history navigation. | High |
| Mouse drifting to divider | There is no cursor-warp call in the searched editor/UI source. Divider hover/capture logic exists, and `PointerCaptured()` requires contact state, so the symptom is more likely coordinate/input acquisition or stale divider geometry than intentional cursor movement. | Medium |
| Curve close-zoom polygonisation | The renderer has adaptive *world/control-polygon* step selection, but the final subdivision is still a fixed number of parameter steps. It is not adaptive to projected screen-space distance/error. | High |
| Transparency | Confirmed: both Scene Directory and Parametric Workspace use `Faded(...)` for panel/menu-related colours. Parametric menu ground is opaque in at least one path, but several row/search/details/tooltip/edge paths intentionally reduce alpha. | High |
| Vulkan flicker | Static source confirms mapped-buffer upload and later command recording, but static checks cannot establish fence/submission lifetime. This remains unresolved without the requested Windows/MSVC/Vulkan run. | High |

## 1. Outliner and Parametric Directory

### What exists

The code has a working conceptual parametric-directory pipeline:

```text
WorkspaceRecordStructure
    -> ProjectWorkspaceDirectory
    -> WorkspaceDirectoryProjection
    -> SketchDirectoryPresentation
    -> ParametricWorkspacePanel::RecordOutliner
```

The host updates it through `SynchroniseParametricPresentation(...)`, and the parametric panel is constructed and advanced by the host.

The record model already contains:

```text
WorkspaceRecordName stable-ish issued index
ParentFolder
FolderCategory
Naming
WorkspaceRecordSubject
visibility/lock flags
property projection
revision sequence
```

The current supported semantic subjects are:

```text
Point
OpenCurve
ClosedProfile
ThinSurface
Solid
Dimension
Constraint
Pattern
Mirror
Folder
```

### What is missing or structurally wrong

The host has two separate panel paths:

```text
SceneDirectory -> SceneDirectoryPanel
SketchDirectory -> ParametricWorkspacePanel
```

The scene directory is recorded through `SceneDirectory.RecordOutliner(...)`; the parametric directory is recorded through `SketchDirectory.RecordOutliner(...)` only when a leaf has `PanelSubject::SketchDirectory`.

Therefore the requested behavior:

```text
Scene Directory
└── Parametric
    ├── Profiles
    ├── Polygons
    └── ...
```

is not currently represented as one embedded tree. The current implementation is a separate parametric workspace/outliner panel. This is the strongest confirmed explanation for the report that the Outliner was removed, blank, or rewired: the logic exists, but its presentation is not the requested unified scene-directory location.

`ProjectWorkspaceDirectory` does bucket records and emits rows, but the inspected model does not show canonical per-family folders for curves, polygons, lines, points, circles, and ellipses. The hierarchy currently depends on declared `ParentFolder` records and broad subject categories.

### Properties and history

These are present in source:

```text
ProjectWorkspaceProperty
BuildInspectorPresentation
WorkspaceRevisionSequence
RevisionRows
RecordPropertyPage
RecordRevisionPage/transfer presentation
```

The selection path resolves a selected directory row to a `WorkspaceRecordName`, then projects properties and revisions. This means the feature should be repaired/connected rather than restarted from zero, unless runtime tests show these paths are not reached.

The missing acceptance proof is shape-family coverage and end-to-end validation:

```text
create -> directory row -> select -> properties -> history -> back/forward -> viewport selection
```

## 2. Mouse moving toward the divider

### Confirmed source findings

The editor/UI source search found no obvious cursor-warp API such as `SetCursorPos`, `WarpCursor`, or equivalent in the relevant editor/UI paths.

The divider logic is present in `EditorPanel`:

```cpp
DeferredDivider
PointerCaptured(PresentationIndex)
CapturedPresentation
DisclosedPresentation
```

`PointerCaptured()` checks:

```text
pointer contact is held/released
and presentation captured OR popup disclosed
and pointer is within DeferredDivider
```

The divider is also used as a gate in `Pressed()`. That means the blue line is probably a hover/visual state being left active by a coordinate or stale-extent problem, not necessarily an active resize operation.

### Most likely fault locations

1. Input sampling or logical-to-physical conversion before `PointerCondition` reaches the panels.
2. `DeferredDivider` being a whole leaf extent in some deferred popup paths rather than the actual divider line. The source assigns `DeferredDivider = CurrentLeafExtent` for deferred menus, so its name is overloaded and its hit area may be broader/staler than expected.
3. Pointer capture/release state surviving presentation changes.
4. The preview/browser environment may be reporting edge-constrained pointer coordinates; this cannot be confirmed statically.

### Required next diagnostic

Log one frame of:

```text
raw OS/client pointer
PointerCondition position
drawable scale
active presentation
DeferredDivider extent
actual divider extent
PointerCaptured result
resize gesture state
```

No code fix should be chosen until these are separated. Hiding the blue divider would mask the symptom rather than fix pointer ownership.

## 3. Curve subdivision

The curve pipeline already has a partial quality improvement:

```text
ResolveCurveStepCount
StepsForSweep
StepsForControlPolygon
```

It selects a step floor based on radius, sweep, or control-polygon size. However, `AppendCurvePolyline` then evaluates uniformly from parameter 0 to 1 using that integer step count.

This is not true close-zoom adaptation because it does not use:

```text
projected pixel length
projected midpoint deviation
active viewport scale
camera distance
```

Confirmed fix direction:

```text
adaptive recursive subdivision
stop when projected midpoint error <= pixel tolerance
and projected segment length <= pixel limit
with bounded minimum/maximum depth
```

The implementation must be shared by committed rendering and preview to prevent them from displaying different tessellation.

## 4. Theme transparency

Transparency is confirmed in code.

`SceneDirectoryPanel.cpp` defines `Faded(...)` and uses it for:

```text
search edge
selected/hovered entity rows
row rail
chevrons
labels
details headers
revision/history accents
```

`ParametricWorkspacePanel.cpp` also defines `Faded(...)` and uses it for:

```text
search edge
disclosure chevrons
```

The appearance specification itself provides `Faded(ThemeToken, double Fraction)`, so alpha reduction is an established theme mechanism rather than an isolated mistake.

The correct fix is not a global alpha replacement. For panel/menu fills, resolve the intended blended colour against the known parent background and emit it with alpha 1.0. Preserve intentional text/disabled/anti-alias treatment where appropriate.

## 5. Vulkan flicker

The CAD pass has a persistently mapped `MappedSlot`. `Upload()` writes directly to it, and `Record()` binds the same GPU buffer for draws. The host uploads conditionally using a packet fingerprint.

The static source does not expose enough of the frame submission/fence path to prove whether a later upload can overlap GPU reads. The previous per-leaf upload experiment was correctly reverted because repeatedly overwriting one mapped buffer before command submission could cause all recorded draws to see the final contents.

The correct next investigation is the frame-resource contract:

```text
Acquire frame
-> wait fence for that frame slot
-> reset/re-record command buffer
-> upload mapped resources for that slot
-> submit
-> present
-> fence ownership
```

If the buffer is shared across in-flight frames, the fix needs frame-ring buffers, staging copies, or an explicit upload/read synchronization contract. This cannot be declared fixed from the current static checks.

## Recommended implementation order

1. **Outliner audit and runtime proof:** preserve the existing parametric presentation logic, then embed its rows into the Scene Directory or explicitly unify the two panel models.
2. **Canonical folders:** add stable folder/category mapping for requested shape families without duplicating records.
3. **Properties/history coverage:** test and repair stable-ID selection and revision traversal for all shape families.
4. **Pointer diagnostics:** instrument raw/client/logical coordinates and divider/capture state before changing resize behavior.
5. **Adaptive curve tessellation:** implement screen-space error subdivision shared by preview and committed rendering.
6. **Opaque theme fills:** replace only panel/menu fill alpha with pre-composited opaque colours.
7. **Vulkan runtime validation:** run the Windows/MSVC/Vulkan build and capture, then fix frame-resource lifetime if confirmed.

## Conclusion

The earlier plan was not purely speculative: the source now confirms that several requested systems already exist partially, especially parametric directory presentation, properties, revision presentation, broad record categorisation, and non-fixed curve step selection.

The largest confirmed structural mismatch is that the parametric directory is a separate panel path rather than being embedded into the Scene Directory tree, and the largest confirmed visual issue is that the theme system deliberately applies faded/translucent tokens. The mouse issue and Vulkan lifetime issue require runtime instrumentation/validation before a responsible code fix can be selected.
