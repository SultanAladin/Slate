# SolidArc-only integration for Frontier e72d319

**Prepared and locally tested. The remote Frontier branch has NOT been merged by this delivery.**
This session is tracked in `SultanAladin/Slate` on `arena/4f335fa8-slate`; apply/review the change in a session
attached to the destination Frontier repository to complete its commit/PR/merge.

| Destination | Exact reference |
|---|---|
| Repository | `streamlinkinbox/Frontier` |
| Branch | `arena/88b72faa-frontier` — **not main** |
| Baseline | `e72d319239dffdb961629150b9de6f1e7dc618c6` |
| SolidArc fixes | Slate `c3a1879` plus the curve-workflow revision of 2026-11-02; exact installed source hashes in SourceManifest.json |

## Apply safely

Extract the complete integration ZIP together. It contains source changes, not product binaries.
In an existing checkout of the requested Frontier branch:

```sh
python /path/to/integration/ApplySolidArc.py --target /path/to/Frontier
python /path/to/integration/ApplySolidArc.py --target /path/to/Frontier --apply
```

The first command is a dry run. The second writes **uncommitted** changes; neither moves a branch nor pushes or merges.
The tool checks the patch hash, destination branch, baseline ancestry, clean tracked files, every touched-file
precondition and patch applicability. A newer destination revision is accepted only when every touched existing file
is unchanged and no added CAD path collides. Otherwise it stops for a reviewed re-port, rather than overwriting work.

Review `git diff`, follow `Exhibits/SolidArc/README.md` in the patched checkout, build/test, then commit and merge using
the destination repository's normal process. **Do not merge the entire Slate branch into Frontier**: this patch
intentionally excludes Slate's unrelated framework, applications, demonstrations and historical binary archives.

## What it integrates

- The reusable CAD B-rep library and current SolidArc document, interface, viewport, snapping and feature fixes.
- The existing Frontier project browser: real `.solidarc` thumbnails/live previews and **Open in SolidArc**.
- Existing ImGui, interface controls, content directory, fonts, GLFW, toml++, stb, ThorVG and Vulkan headers.
  Only the checksummed volk dependency is added.
- Optional `FRONTIER_BUILD_SOLIDARC` CMake switch, the CAD executable and verification targets.
- Editor-preview packaging checks for the CAD companion, CAD options, SVGs, font, sample and volk notice.

CAD editing remains a **separate workspace process launched by the browser**, not tools embedded in the Everest
scene viewport. Existing scene startup, renderer/measurement sources and game project files are preserved.
The supplied vehicle file is neither imported by this source patch nor modified. T-spline/CAD Form work is on hold.

## Evidence and limits

See [AUDIT.md](AUDIT.md), [source manifest](SourceManifest.json), [scope integrity](Validation/ScopeIntegrity.json)
and the actual application captures under [Images](Images/).

Fresh checks: **506 native CAD + 17 browser CPU; 300 CAD + 15 browser Vulkan expectations; 869 CAD rows;
9 conditional packaging unit tests**. Linux tests use headless Vulkan/SwiftShader. The compiled GLFW null-window host
is not a Linux desktop product. Windows host compilation, Windows runtime and signed distribution were not tested.
The existing two-application signature/antivirus distribution policy is unchanged, not bypassed.

## Curve-workflow revision

- **Split** in the viewport toolbar now divides authored 3D NURBS curves at the pointer ring, preserving shape.
- **Live surface distance** is at the top of Geometry; a linked target alone exposes editable settings and persistent applied/refusal feedback.
- **Outliner / Viewport / Inspector** use the existing Frontier dockspace, tab chrome and Texture Workspace authoring controls.
- **Curve guide** and **Reset panels** are visible in the status bar. Reset also reopens hidden primary panels.
- [Usage guide](CurveRebuildingGuide.md); [fresh verification](Validation/CurveWorkflowReport.json).

Fresh verification: **568 native checks**, **869 green CAD rows**, **328 rendered expectations**, **9 packaging unit tests**.
These are Linux headless checks, not a Windows runtime verification. The Frontier browser starts the exact selected
CAD project in the companion workspace; no BAT is needed in an integrated installation. The destination branch still
needs this source change applied/reviewed there. This ZIP contains **source, not a Windows executable**.
