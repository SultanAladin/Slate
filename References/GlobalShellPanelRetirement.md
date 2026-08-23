# GlobalShellPanel retirement plan

## Purpose

`GlobalShellPanel` was a validation prototype used to converge the full interface reference. It is not the editor's
runtime surface. Once the real directory and texture-paint presentations have absorbed the approved behaviour,
the prototype and its dedicated validation host should be retired rather than maintained as a third implementation.

This plan records the retirement only. No `GlobalShellPanel` source is removed during the current layer-stack work.

## Standing replacements

| Prototype responsibility | Runtime replacement |
|--------------------------|---------------------|
| General-purpose directory and hierarchy | `SceneDirectoryPanel` |
| Directory details, properties, and history | `SceneDirectoryPanel` |
| Texture-paint rows, masks, properties, and menus | `TexturePaintPanel` |
| Reusable leaf chrome, split, and withdrawal | `EditorPanel` and planned reusable chrome declarations |
| Workspace windows and tabs | `WorkspacePanel` and `WorkspaceIndex` |
| Real viewport content and overlays | Editor viewport leaf, `ViewportSkySurface`, and `OverlayPass` |

The retirement must not copy `GlobalShellPanel` into either replacement. Approved behaviour is re-expressed through
the runtime panels' own contracts, host-owned data, reusable controls, and request slots.

## Gates before retirement

### Scene directory gate

- One general-purpose hierarchy replaces the drafting/game distinction.
- Directory and details share the first outer slide.
- Properties and history share the second outer slide as two inner pages.
- The outer transition travels continuously in both directions.
- Search, facets, visibility, disclosure, selection, context actions, editable names, property cards, and revision
  history have runtime proofs.
- Record classifications and property schemas are host-supplied rather than game-world assumptions.

### Texture paint gate

- Reviewed layer-stack appearance corrections are implemented and raster-proved.
- Material, folder, and layer naming use the reusable editable text field.
- The reusable footer can host the `Export Flattened` pill.
- Layer operations, masks, property pages, menus, filtering, and flattened-export requests have runtime proofs.
- No required texture-paint behaviour remains available only through `GlobalShellPanel` or the prototype
  `LayerStackPanel`.

### Shared chrome and control gate

- Reusable editable text-field behaviour serves search, material naming, layer naming, and directory naming.
- Reusable header and footer composition supports text fields, pill buttons, icon buttons, readouts, separators,
  menu anchors, clipping, and narrow-width withdrawal.
- Existing subject, divide, split, withdraw, grid, shading, and workspace actions remain intact.
- Popup deferral and stable interaction identities are proven after the chrome extraction.

## Retirement sequence

1. Freeze `GlobalShellPanel`: no new runtime behaviour is added to it.
2. Complete and raster-prove `SceneDirectoryPanel` against the approved directory model.
3. Complete and raster-prove `TexturePaintPanel`, including shared text editing and export intent.
4. Inventory every `GlobalShellPanel` behaviour and map each still-required item to a standing runtime proof.
5. Move any generally useful declarations out of the prototype only when a runtime consumer requires them.
6. Remove `InterfaceValidationHost` dependencies on `GlobalShellPanel` and retire proofs that exercise only the
   prototype.
7. Remove `Engine/SlateUI/Interface/GlobalShellPanel/Api/GlobalShellPanel.h` and its source.
8. Remove stale comments, registration counts, build declarations, validation scripts, and proof assertions.
9. Update the authoritative engine directory-structure document for the removed subsystem and host.
10. Run partition, translation, runtime interaction, and raster gates before accepting the retirement.

## Deletion boundaries

The retirement must not remove:

- `SceneDirectoryContract.h` records still consumed by runtime panels;
- `TexturePaintPanel` or its contract;
- shared control, appearance, symbol, motion, interaction, or recording facilities;
- real editor workspace, panel, viewport, overlay, or host behaviour;
- historical proof images required to explain an approved migration.

Any declaration shared only because `GlobalShellPanel` currently includes it must be classified by mechanism before
moving. A bulk move of the prototype header into another panel is explicitly rejected.

## Completion evidence

Retirement is complete only when:

- no source or build declaration references `GlobalShellPanel`;
- no application host constructs it;
- all retained directory and layer-stack gestures are proven through the runtime panels;
- committed raster proofs show the real editor leaves rather than the validation shell;
- the repository's structure documentation and validation commands describe only standing systems.
