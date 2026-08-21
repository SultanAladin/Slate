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
| Validation prototype shell | `GlobalShellPanel` — validation host only |
