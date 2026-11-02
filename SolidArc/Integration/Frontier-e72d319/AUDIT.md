# SolidArc-only Frontier integration audit

**2026-11-02 · Target: `streamlinkinbox/Frontier`, `arena/88b72faa-frontier`, exact commit `e72d319239dffdb961629150b9de6f1e7dc618c6`.**

**Status: source integration prepared and locally verified; remote Frontier merge is pending.**
This session remains on Slate `arena/4f335fa8-slate`. It has not pushed to the destination or changed `main`.

## Scope / completion

| Status | Requirement | Evidence / implementation |
|---|---|---|
| ✔️ | Use the explicitly requested revision, not main | Baseline object fetched and archived at e72d319; patch and per-file preconditions recorded in SourceManifest. |
| ✔️ | Bring only SolidArc and necessary dependencies | 498 source/artwork/fixture/document paths: 484 additions, 14 selectively changed existing files. No unrelated Slate framework, applications, web prototypes, downloaded dependencies or product ZIPs in the patch. |
| ✔️ | Reuse existing Frontier functionality | Existing FrontierImGui, FrontierInterface, FrontierEditorUI, content directory, fonts and dependency recovery retained. Existing third-party pins unchanged; only volk appended. |
| ✔️ | Resolve the split CAD source | CAD B-rep/document/interface/viewport and current Slate fixes seated in the destination's Engine/UI/Experimental/Exhibits layout. No duplicate general-purpose interface framework imported. |
| ✔️ | Integrate with the existing browser | Native CAD decoding, actual tessellated previews, damaged-file refusal, Open in SolidArc, workspace discovery and process launch. Existing ordinary-project flow preserved. |
| ✔️ | Preserve newer host startup | Two three-way overlaps resolved by retaining Everest includes/startup and adding an optional CAD browser connection. No replacement of the modern Windows host with the old host. |
| ✔️ | Preserve renderer and requested measurement changes | All **4,562 untouched baseline files** match byte-for-byte; includes **399 renderer/measurement/Everest files** explicitly listed in ScopeIntegrity. Existing shader source files are not replaced. |
| ✔️ | Preserve current CAD behavior | Latest Shift drawing, multiselection/Delete, component safety, tracing, geodesic links and inward surface cutters carried over and retested. |
| ✔️ | Optional CAD and packaging connection | FRONTIER_BUILD_SOLIDARC can be disabled. Preview packager detects CAD-enabled Frontier, requires a current x64 CAD companion and checks CAD resources. Signing/AV policy not weakened. |
| ✔️ | Keep vehicle and deferred task untouched | Supplied cage SHA remains exact. It is not added by this source integration. No T-spline/SubD work or automatic vehicle reconstruction. |
| ⚠️ | Windows verification | Windows host source was ported, but neither compiled nor interactively executed in this integration run. No new Windows product binary is provided. |
| 🚧 | Actual destination merge | **Not performed.** Apply/review in the destination repository/session, then build and complete its commit/PR/merge. Do not wholesale-merge Slate. |

## Fresh verification

| Status | Check | Result | Evidence |
|---|---|---|---|
| ✔️ | Native CAD / UI features | **506**: Shift 185; cutters 112; selection 17; cage 49; editing 51; tracing 92 | Native* logs in Validation |
| ✔️ | Curve workflow revision | **45 passed**: exact pointer split, shape preservation, atomic refusal, stale component cleanup, target-only distance editing, real tab drag, reopen/reset and implicit-window safety | [NativeCurveWorkflow](Validation/NativeCurveWorkflow.txt) |
| ✔️ | New browser CPU checks | **17 passed**: real decoding/geometry, mixed-case extension, spaced paths, exact launch callback, refusals, ordinary-project access, source preservation | [Browser](Validation/Browser.txt) |
| ✔️ | Existing rendered CAD workflows | **313 / 313** expectations in 18 scripts | [RenderedSummary](Validation/RenderedSummary.json) and corresponding logs |
| ✔️ | Browser rendered integration | **15 / 15** checks; actual CAD thumbnails/live reader, damaged-file refusal, ordinary sample flow and child-process workspace hand-off | [BrowserVulkan](Validation/BrowserVulkan.txt), [report](Validation/BrowserVulkanReport.md) |
| ✔️ | Full CAD suite | **869 green rows, zero red**, exit 0 | [DocumentRegression](Validation/DocumentRegression.txt) |
| ✔️ | Conditional Windows packaging logic | **9 unit tests**, using synthetic PE headers; missing/stale/wrong-architecture companions refused | [WindowsPackaging](Validation/WindowsPackaging.txt) |
| ✔️ | Build / disabled configuration | Release native app, capture, document, CAD verification and browser targets built; CAD-off config succeeds without the CAD app target | Build / Configure / DisabledConfigure logs |
| ✔️ | Dependency closure | Every retained non-vendor compile input belongs to the pinned baseline or delivered source patch | [FinalChecks](Validation/FinalChecks.txt) |
| ✔️ | Patch integrity | Dry-run, actual application in a clean source test directory, and every resulting source SHA matched | [SourceManifest](SourceManifest.json), [ScopeIntegrity](Validation/ScopeIntegrity.json) |

Totals: **568 native CAD/browser checks and 328 rendered expectations**.
The 869 CAD table rows and 9 synthetic-PE unit tests are reported separately, not relabelled as native feature checks.
Linux headless ImGui + Vulkan/SwiftShader was exercised. The null-window GLFW build is compile coverage,
not proof of a usable Linux desktop package or Windows startup.

## Curve workflow changes

| Status | Request | Implemented result |
|---|---|---|
| ✔️ | Adjustable live surface/geodesic distance | Top-of-Geometry card; select one linked target to edit; apply on field release or explicitly; persistent success/refusal feedback; ambiguous multi-body references require a choice |
| ✔️ | Split at the mouse | Visible Split tool; exact NURBS knot division at refined projected parameter; both pieces kept; one undo; dependency and endpoint refusals; stale pole/span selections cleared |
| ✔️ | Three dockable primary panels | Outliner, Viewport and Inspector delegate to existing PanelDockspace; actual tab dragging, validated layout and reopen/reset exercised |
| ✔️ | Reuse Texture Workspace controls | Existing VectorAuthoringControlSurface and PanelChrome compiled once as FrontierAuthoringControls and linked by both workspaces; no copied control implementation |
| ✔️ | Guides | Native Curve guide plus [usage guide](CurveRebuildingGuide.md) |
| ✔️ | Frontier project browser | Existing browser previews, reader and exact-document companion launch retested; an integrated installation does not need BAT |
| 🚧 | Windows product / destination merge | Still not performed; this delivery is source plus Linux verification, not an installed Frontier update |

Separate OS panel windows and restart layout persistence are not claimed. Geodesic distance is tolerance-controlled,
not analytically exact. The original vehicle cage and the reference-only tracing project remain byte-identical.

## Actual application images

- [Mouse-position split preview](Images/Split_Pointer_Preview.png)
- [Exact retained split pieces](Images/Split_Exact_Pieces.png)
- [Target-only live distance edit](Images/Linked_Target_Only_Distance_Edit.png)
- [Visible refusal keeps previous geometry](Images/Distance_Refusal_Keeps_Geometry.png)
- [Floating Outliner through the existing dockspace](Images/Workspace_Floating_Outliner.png)
- [Native usage guide](Images/Curve_Workflow_Guide.png)
- [Frontier CAD library](Images/BrowserLibrary.png)
- [Frontier reader displaying decoded CAD geometry](Images/BrowserReader.png)
- [Open in SolidArc hand-off](Images/BrowserHandOff.png)
- [The child workspace opened the requested CAD document](Images/BrowserStartedWorkspace.png)
- [Shift polyline axes on the updated foundations](Images/Shift_Polyline_Per_Segment_Axes.png)
- [Aligned arc endpoints](Images/Shift_Arc_Aligned_Endpoints.png)
- [Free arc bulge](Images/Shift_Arc_Free_Bulge.png)

These are real application captures, not mockups. The bracket/assembly and drawing documents are disposable test
fixtures, not new geometry in the supplied vehicle.

## Application boundary and remaining work

- CAD editing opens in its own **SolidArcWorkspace** process, through the existing browser. This does not embed
  SolidArc commands into the Everest scene viewport or change the scene renderer.
- Windows host build/startup, native MSVC, hardware-GPU execution and signed/antivirus-verified distribution remain
  unverified. The existing restricted two-application distribution authorization remains unchanged.
- The integration preserves inherited CAD APIs/names rather than performing a broad naming or architecture rewrite.
- ApplySolidArc.py checks the exact target branch, baseline ancestry, tracked cleanliness, new-path collisions,
  every edited source precondition and patch SHA. Newer touched files cause a refusal, not silent replacement.
- Remote merge is pending because the active Arena session is tied to Slate. The source bundle is a hand-off,
  **not a claim that Frontier's linked branch has been updated**.

## Provenance

CAD source: `unassignedinbox/Frontier` `82ed251267ed8457ca3530d8228b91555572c25d` plus current Slate
`c3a187909483aa8182ba96cf4916e3542a09395e` overlays and the two NURBS importer precision patches.
Old host `SultanAladin/Frontier` `65cf0f131a6afe8bdcd722c006b10cfab0e1aafd` supplied the three-way comparison only;
it was not substituted for the destination editor. Existing source/artwork notices remain applicable.
No new licence grant is asserted.

Compiler: GCC 12.2, C++20, Release; CMake/Ninja. Existing destination ImGui patches and dependency pins reused.
Vulkan: checksummed SwiftShader from the destination's existing tool-recovery source, 1280×680 for CAD scripts.
Reproduction and safe application are in [README](README.md) and the patched `Exhibits/SolidArc/README.md`.

Original cage SHA-256: `90344c326b32e467b8298851eb8b1df2a2dba1ce879becd3c7a12303b252ec70`.
