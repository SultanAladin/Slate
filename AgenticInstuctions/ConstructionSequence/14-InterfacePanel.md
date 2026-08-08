# 14 — InterfacePanel

`SlateUI.lib` is the whole of `Layer5_Interface`. It links all four units beneath it and is linked only by a host.
It owns exactly one copy of ImGui, and that copy does not escape: no ImGui spelling appears in any signature the
host can see, in any other unit, or in any shared header.

This is the seam the source documents are most specific about, and the one most easily eroded. Erosion is always
the same shape — one `ImVec2` in a public signature, then an `ImDrawData` pointer, then the host owns an ImGui
context. Each step looks small and the sum is that ImGui became the application's UI framework rather than
`SlateUI`'s implementation choice.

## Position In The Sequence

| Field       | Value                                                                      |
|-------------|-----------------------------------------------------------------------------|
| Unit        | `SlateUI.lib`                                                               |
| Layer       | `Layer5_Interface`                                                          |
| Upstream    | `04` (window, input, clipboard), `06` (device), `08` (final recording), `10`, `12`, `42`, `46`, `56`, `58`, `66` (`DisplaySurface`), `68`, `76`, `84`, `86` |
| Downstream  | `32` assembles it into a host                                              |
| Unblocks    | A visible, interactive application                                          |

## 1. The Panels

| Component         | Mechanism                                                          | Presents |
|-------------------|---------------------------------------------------------------------|-----------|
| `WorkspacePanel`  | The workspace surface the artist works inside                       | —        |
| `CameraPanel`     | Assembles multiple projections into one image                       | `46`     |
| `DisplayPanel`    | What is shown, and the pacing that shows it                         | `66`     |
| `OutlinerPanel`   | Presents `RowSequence` through `RankIndex` — see `12` §7            | `12`     |
| `PropertyPanel`   | Presents `PropertySpecification` declarations for the selection     | `10`     |
| `ToolPanel`       | Presents the active tool's parameters                               | `76`     |
| `RevisionPanel`   | The transaction sequence, scrubbable in both directions             | `84`     |
| `LayerPanel`      | The surface layer sequence for the selected occupant                | `56`     |
| `BrushPanel`      | Brush declarations, presets, and the active colour                  | `58`     |
| `MaterialPanel`   | Material declarations and their channel assignments                 | `42`     |
| `ChannelPanel`    | Which of `18`'s channels is displayed, and how                      | `76`     |
| `DomainPanel`     | The parametric domain — chart layout, seams, placement in domain    | `68`     |
| `DiagnosticPanel` | Residency, budget, timing, and refusals — see `86`                  | `86`     |

🔴 Every panel in this table presents state owned elsewhere and stores none of it. The column names the owner
precisely so that no panel can quietly become the home of the thing it displays. A panel that holds the only copy
of what it shows is the defect in §4.1 wearing a different name.

⚠️ `ViewportPanel` is a retired spelling and was ambiguous across two units. In `SlateVulkan` it became
`CameraPanel`; in `SlateUI` it became `WorkspacePanel`. Both exist and they are different things.

⚠️ The first six rows were the whole of this table when the series was written. The seven below them had no panel
at all, which meant the artist had no route to undo history, layers, brushes, materials, channel display, the
domain, or diagnostics — while `10`, `42`, `56`, `58` and `84` all assumed something presented them. Recorded as
`00` §10 conflict 24.

## 2. The Host Seam

The host sees `SlateUI` through ordinary C++ across a static link. `std::string`, `std::string_view`, `std::span`
and POD structures cross freely, because one invocation with identical switches and one CRT builds both sides.

🔴 What never crosses: `ImGuiContext`, `ImDrawData`, `ImVec2`, `ImVec4`, `ImGuiID`, `ImFont`, or any other ImGui
spelling. A host translation unit that includes `imgui.h` is a defect regardless of whether it links.

The host supplies a window handle and a device, and receives input intent and a request to record. It does not
drive ImGui, does not own its context, and does not know it exists.

## 3. Configuration Constraints

| Constraint                          | Consequence if violated                              |
|-------------------------------------|-------------------------------------------------------|
| Exactly one ImGui copy, in `SlateUI`| Two contexts, split state, unattributable input loss  |
| `/MD` in Debug and Release both     | Allocator mismatch against `ExternalPackages`         |
| 🔴 `_DEBUG` never defined            | Selects the debug CRT; same mismatch, harder to trace |
| `SLATE_DEBUG` selects debug builds  | —                                                     |
| No CMake                            | `Module.toml` plus scripts, as everywhere else        |

⚠️ If anything under `ExternalPackages` does not match the layout the build expects, **stop and report the actual
layout**. Do not adjust silently. A silently adjusted vendored dependency is a defect that reproduces on one
machine only.

## 4. Input Intent

`SlateUI` consumes timestamped samples from `InputExchange` and produces *intent*, not events. Intent is what the
artist meant — select this occupant, begin a stroke at this surface position with this pressure, reorder this row
under that enclosure — and it is expressed in document terms.

The arrival timestamps from `04` §3 survive into intent. `22` reconstructs stroke geometry from them, so an intent
carrying consumption timestamps has the display rate baked into the stroke.

Intent that mutates **the document** is committed as a transaction through `RevisionSequence`. There is no other
path for document mutation.

### 4.1 Intent that mutates nothing in the document

🔴 The clause above was previously unqualified — "there is no other path" — which left the greater part of an
application's state with **no specified route at all**. Choosing a colour, resizing a brush, activating a tool,
orbiting the camera and switching a display mode are none of them document mutations, and none of them can be
transactions: undo must not step back through a colour change.

| State                                    | Owned by | Recorded in          |
|------------------------------------------|----------|-----------------------|
| Active tool and its parameters           | `76`     | Beside the document   |
| Active colour and the brush              | `76`     | Beside the document   |
| Camera position, projection, exposure    | `46`     | The document          |
| Display mode and overlay presence        | `76`     | Beside the document   |
| Panel layout, scroll, expansion          | `14`     | Beside the document   |
| Selection                                | `12`     | `SelectionSequence`, session-scoped |

⚠️ Exposure moved out of this table. It is an authored camera property stored in the document — `46` §6 — and
the two answers this table and `46` gave are recorded as `00` §10 conflict 33.

⚠️ This is the mechanism behind a defect the artist meets immediately: picking a colour and finding the brush
unchanged. With no declared owner for the active colour, `SlateUI` holds it, `SlateCompute` cannot read it, and
the stroke resolves against something else. `76` owns it, both units read it, and the interface presents it.

### 4.2 Pointer arbitration

Exactly one consumer holds the pointer at a time, and the decision is made **before** intent is produced, never by
two consumers each deciding they were addressed.

| Precedence | Holder                                               |
|------------|-------------------------------------------------------|
| 1          | ImGui, when it reports the pointer over interface     |
| 2          | An open manipulator drag in `78`                      |
| 3          | An open stroke in `22`                                |
| 4          | The workspace — picking, navigation                   |

🔴 Capture persists for the whole drag. A drag that begins on a manipulator handle and leaves the interface
continues to address that handle; a drag that begins in the workspace is not stolen by a panel it passes under.
Re-arbitrating mid-drag is the defect where a stroke stops the moment the cursor crosses a floating panel.

## 5. Composition And Presentation

`14` contributes the final recording in `08`'s order, and it contributes **only** that recording. It records the
interface over `DisplaySurface` and hands the slot to `DisplayScheduler` for pacing.

🔴 `14` no longer composites anything. This section previously claimed it composited `RadianceSurface` and
`OutlineSurface` into `DisplaySurface`, which placed tone mapping, transfer encoding and selection presentation
inside the interface unit. `66` produces `DisplaySurface` from `RadianceSurface`; `26` writes the selection
outline over it, display-referred; `80` writes the overlays. By the time `14` records, `DisplaySurface` is
finished and the interface is drawn on top of it.

| Reads                    | Writes           |
|--------------------------|-------------------|
| Nothing produced by `18` | `DisplaySurface` |

⚠️ The distinction is load-bearing rather than pedantic. Compositing a scene-referred surface inside `SlateUI`
would require `SlateUI` to hold the exposure and the transfer function, and the artist would then find that
changing exposure in the display panel changed the interface's own colours.

The interface is recorded into the same rotation slot as everything else. It has no separate rotation and no
separate device queue.

## 6. Never Call

Enumerated because each has a legitimate-looking call site and each breaks the seam or the rotation.

| Never                                          | Instead                                          |
|------------------------------------------------|---------------------------------------------------|
| Device idle-wait to settle interface state     | Size against the rotation depth                  |
| Construct a descriptor layout during recording | Construct at bring-up — `06` §5                  |
| Allocate a device resource per recording       | Rotational extents — `06` §3                     |
| Present outside `DisplayScheduler`             | `DisplayScheduler` owns pacing                   |
| Mutate the document outside a transaction      | `RevisionSequence` — `10` §2.3                   |
| Read the outliner relations directly           | Read `RowSequence` through `RankIndex`           |
| Define `_DEBUG`                                | `SLATE_DEBUG`                                    |

## 7. Gates

- **Gate:** No ImGui spelling appears outside `SlateUI/`, including in shared headers.
- **Gate:** Exactly one ImGui copy exists in the build.
- **Gate:** `/MD` in every configuration; `_DEBUG` in none.
- **Gate:** Every document mutation from the interface is a transaction.
- **Gate:** Input intent carries arrival timestamps, not consumption timestamps.
- **Gate:** The interface records into the shared rotation, with no rotation of its own.
- **Gate:** Nothing in the never-call list appears in `SlateUI`.
- 🔴 **Gate:** Every panel names the component that owns what it presents, and stores none of it.
- **Gate:** No non-document state is committed as a transaction — the §4.1 table is the whole route.
- **Gate:** Exactly one consumer holds the pointer, and capture persists for the whole drag.
- **Gate:** `14` composites no scene-referred surface; it records the interface over a finished `DisplaySurface`.
- **Gate:** No exposure, tone map or transfer function is spelled anywhere in `SlateUI`.

## 8. Open

| Open question                                              | Blocks                        |
|-------------------------------------------------------------|--------------------------------|
| Whether `CameraPanel` supports more than one projection now  | Nothing structural             |
| Theme and scaling policy for high-density displays           | Nothing structural             |
| Whether `LayerPanel` and `OutlinerPanel` are one panel or two| Presentation only — `00` §12   |

⚠️ "Whether panel layout persists in the document or beside it" is **closed** — beside it, owned by `14`, per
§4.1. Panel layout in the document would make rearranging a panel an undoable edit and would make a document
open differently on a machine with a different display.
