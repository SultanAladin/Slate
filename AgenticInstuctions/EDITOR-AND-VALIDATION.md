# The Editor vs The Validation Shell — READ BEFORE TOUCHING EITHER

> This note exists because a previous agent ported the validation shell into the
> editor host (recorded `GlobalShellPanel` fullscreen over the editor window) and
> drew the sky in the shell's own viewport. That was wrong on both counts and was
> reverted. It must not happen again.

## The two surfaces are different products

```
VALIDATION HOST (InterfaceValidationHost)                 EDITOR HOST (EditorHost)
────────────────────────────────────────                  ────────────────────────
prototype of the WHOLE reference sheet                    the real editor layout
                                                          ┌─ Workspace 1 ──────────────┐
┌──────────────────────────────────────┐                  │ ┌───────────┬─────────────┐ │
│ top bar                              │                  │ │ VIEWPORT  │ OUTLINER    │ │
├──────┬───────────────────────────────┤                  │ │ leaf      │ leaf        │ │
│OPTION│ GlobalShellPanel viewport     │                  │ │ (sky)     │ (directory) │ │
│ S    │  ┌─────────────────────────┐  │                  │ │           │ ┌───┬─────┐ │ │
│ rail │  │ BLACK viewport (stays   │  │                  │ ├───────────┤ │out│det. │ │ │
│ CAD… │  │ black — keep it black)  │  │                  │ │ footer    │ ├───┴─────┤ │ │
├──────┴──┴─────────────────────────┴──┤                  │ │           │ │ props   │ │ │
│ fullscreen inspector strip:           │                  │ │           │ │ history │ │ │
│ [outliner|details]→[properties|hist]  │                  │ └───────────┴─┴─────────┴─┘ │
│ + texture-paint layer stack           │                  │   split / + / resize /      │
└──────────────────────────────────────┘                  │   withdraw via EditorPanel  │
                                                          └──────────────────────────────┘
```

## Rules

1. **`GlobalShellPanel` is recorded ONLY by `InterfaceValidationHost`.** It is a
   prototype of the full reference sheet (options rail, texture-paint layers,
   CAD drafting, fullscreen two-slide inspector). Its viewport stays **black**
   (that is the prototype's own look) — do not draw a sky in it.
2. **The editor host NEVER records `GlobalShellPanel`.** The editor's layout is:
   - `WorkspacePanel` + `WorkspaceIndex` + the vendor dock → workspace windows
   - `EditorPanel` + `PanelStructure` → splittable panels (viewport | UV |
     outliner | properties), each with its own chrome (header, footer, subject
     menu, divide menu) — this is how panels are added, split, resized, withdrawn
   - `SceneDirectoryPanel` → the CONTENT inside the leaves: the sky in a
     viewport leaf, the outliner | details column in an outliner leaf, the
     properties | history pages in a properties leaf
3. **Drafting and the game editor are the same thing** — one general-purpose
   outliner. The editor's outliner leaf presents the scene directory; there is
   no mode switcher and no texture-paint in the editor.
4. **The sky belongs in the editor's viewport LEAF** (the GPU sky texture via
   `SceneDirectoryPanel::RecordViewportSky`), never in a fullscreen overlay.
5. Shared scene-directory contracts (entities, environment, revisions) live in
   `SceneDirectoryPanel/Api/SceneDirectoryContract.h` — both surfaces include
   that one header; neither one owns the other.
6. History records once per slider drag (start → end) via `RevisionDemandSlot`
   — the host drains it exactly once per drag. Do not regress this to per-tick.
7. Linux/Windows parity: the sandbox (`ConstructSandbox.py`) is the POSIX twin
   of `Build/Construct.ps1`; `.toml` files and `.slang` shaders must stay in
   sync with renamed C++ definitions.
8. Proofs: real pixel renders under `VisualProof/EditorScene/` — never ASCII or
   stubs.

## Where things live

| Concern | Component |
|---|---|
| Workspace windows, tabs, dock | `WorkspacePanel`, `WorkspaceIndex` |
| Split/resize/withdraw chrome | `EditorPanel`, `PanelStructure`, `LeafPanel` |
| Editor leaf content (sky, outliner, properties) | `SceneDirectoryPanel` |
| Sky GPU texture (upload, device rebuild) | `ViewportSkySurface` |
| Sky evaluation (dome + sun disc) | `GenerateSkyImage` (`Application/EditorHost`) |
| Fly camera (WASD + look + lag) | `CameraRig` (`Application/EditorHost`); input via `InterfaceExchange::CameraInput` |
| Validation prototype shell | `GlobalShellPanel` — validation host only |

## The editor camera

- **Registered in the scene directory** as the "Editor Camera" row (last row, so
  the Sun/Sky ordinals and their history ordinals never move). Its details pane
  shows the pose (position, yaw/pitch, speed) and its toggles; its properties
  leaf has a live **Fly Speed** slider (1–500 m/s) with a once-per-drag history
  demand like the environment sliders.
- **Movement is Unreal-fly**: W/S along the view direction (pitch included),
  A/D strafe, E/Q world up/down, **Shift to boost the fly speed 3x**; **hold
  the right mouse button and drag to look**. The look gesture CAPTURES the
  cursor: while held, the OS cursor is warped to the display centre every tick,
  so the turn is unbounded — it never stops at the window edge.
  🔴 The look reads the vendor's ACCUMULATED mouse delta, never the pointer's
  departure from the centre (with the warp in place the departure is non-zero
  only on the press frame — the "turns then stops" defect), and it is NOT gated
  on `WantCaptureKeyboard` (a hovered window raises it, which would stop the
  camera over any panel). Only text input gates the movement keys.
- **Camera lag**: the rig eases position and yaw/pitch toward the target with
  an exponential time constant (0.18 s). Toggle it in the camera's details
  (Camera Lag); Invert Pitch is the second toggle. The viewport crop reads the
  LAGGED pose, so a fast turn visibly trails the input.
- **The world is visible**: the viewport leaf draws a ground lattice on the
  Y=0 plane, projected through the same pinhole as the sky — so W/A/D/E/Q
  visibly travel the scene and the look gesture visibly turns it, in metres.
  The lattice is driven by the viewport's footer "Grid settings" popup: None /
  Lines / Dotted / Lines + Dots, cell Scale (m), Subdivisions (the half-extent
  in cells, 2-128), and the red/green/blue axis lines (X/Y/Z toggles). The
  dotted presentation draws a node at every intersection.

## The GPU overlay pass (grid, gizmo, wireframe)

The grid, the world-origin gizmo and (later) wireframe are NOT drawn through
the interface's ImGui draw lists — they are drawn by `OverlayPass`
(`SlateVulkan/Device/OverlayPass`), a dedicated graphics program recorded in
its own pass inside the host's dynamic-rendering scope, AFTER the interface.

- **The CPU never tessellates.** The panel fills `OverlayGeometry`
  (`Shared/OverlayGeometry.slang.h`, reachable from every unit) — a few hundred
  screen-space primitives with two points per line and one per dot. The host
  uploads it when its generation changes, and the vertex shader
  (`OverlayVertex.slang`) expands lines and dots from `SV_VertexID` into their
  quads; triangles pass through. ImGui's polyline path tessellated every
  segment on the CPU, which bogged the frame down on dense lattices and would
  on high-poly wireframe.
- **Vivid colours.** The pass blends STRAIGHT alpha (`src_alpha /
  one_minus_src_alpha`) with no tone mapping, so a full-opacity gizmo stays
  full-opacity — the interface's premultiplied blend washed the same hues out
  over a bright sky.
- **Lifecycle**: `Overlay.Reclaim()` on device-retiring, re-Construct on
  device-recovered, `Overlay.Reclaim()` before `Lifetime.Reclaim()` at
  shutdown. The pass refuses gracefully when the build lowered no shaders (the
  sandbox) and the editor runs without the overlay.
- **The overlay is clipped to the viewport leaf.** The pass's `Record` sets the
  scissor to the leaf's box (clamped to the display), and the host keeps ONE
  overlay record per viewport leaf (static storage) and draws each clipped to
  its own leaf — the grid, the axes and the gizmo never paint over the
  outliner, the properties or any other panel, and two viewports each show
  their own grid. The harness rasterizes with the same clip and asserts zero
  overlay ink beyond the leaf.
- **The axis lines span the whole grid**: X (red), Y (green, vertical) and Z
  (blue) each run the lattice's full extent in both directions (-Half ..
  +Half), sampled and near-plane-split like the lattice lines, and they draw
  even when the lattice presentation is None (the two toggles are
  independent).
- **Shaders**: `OverlayVertex.slang` + `OverlayFragment.slang` under
  `SlateVulkan/Device/OverlayPass/Shader/` are lowered by `Construct.ps1` to
  `<Binary>/../Shader/SlateVulkan/<Stem>.spv`; `ShaderCodec` reads them at the
  same directory the build writes.
- **Proof**: the harness rasterizes the SAME `OverlayGeometry` record on the
  CPU (its `RasterizeOverlay`) with the same straight-alpha blend, so the proof
  pixels are the pass's input; `editor-grid-settings` asserts all three axes
  (R/G/B at full opacity), the gizmo's white centre handle, and that
  lines+dots carries ~6x the lattice ink of dots-only.
- The sky dome is direction-indexed and camera-independent: looking around only
  moves the crop, never regenerates the texture. The viewport samples the dome
  through a PERSPECTIVE mesh (per-vertex UVs along the true pinhole ray), so
  the sun stays round at any leaf aspect — a plain cropped quad stretches it.
  The mesh's U is ABSOLUTE and the sampler wraps U (REPEAT, V clamps): the
  dome's azimuth is periodic, and a camera whose frustum crosses the seam must
  wrap, not shift — a shift of yaw/2π is not a whole period and reads the
  wrong texels (this was tried and reverted).
- **Dropdowns composite above the viewport**: `EditorPanel::Record` can defer
  its popups (`DeferPopups`), the host records leaf content between the two
  calls, then `RecordDeferredPopups` records the menus on top. Never record the
  leaf content AFTER the deferred call.
- **The outliner leaf has pages**: Tab (the seam's Summon) cycles Directory →
  Properties → History → Directory; the leaf's bottom strip selects the same
  page, and the "Inspect" call in its header jumps to Properties. The
  properties leaf's own Properties | History strip is a separate tab state, so
  the two leaves never fight.
- **Shutdown order matters**: `SkySurface.Reclaim()` runs BEFORE
  `Lifetime.Reclaim()` — a surface left standing past the device reclaim waits
  on a dead fence and reports `vkWaitForFences: Invalid device` at exit.
