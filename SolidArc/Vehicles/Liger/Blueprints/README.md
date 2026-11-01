# Liger — scratch-modelling reference project

High-resolution **CAD-derived orthographic schematics**, with an empty SolidArc model for rebuilding the same exterior body yourself. Based on the latest refined Liger (`ea9cb13`); **no further vehicle modelling is performed in this project**.

[Six-view overview](Liger_Orthographic_Overview.png)

## Start here

1. Download and extract the **matching full runtime + project ZIP** from `Packages/`.
2. Windows: double-click **`Start_Liger.bat`**. Linux/X11: run **`./Start_Liger.sh`** (or `sh Start_Liger.sh` if your unzip tool loses executable permissions).
3. The launchers open **`Liger.solidarc`**, frame the references, and initialise the existing **four-pane layout**: top, isometric reference boards, vehicle right side, and vehicle front.
4. The feature tree is **empty**. The six registered image planes are locked to prevent accidental movement. Start your own curves/sketches/surfaces; unlock a reference through the Outliner only if you intentionally want to reposition it.
The grid starts hidden for clear linework; enable it from Display when needed. Use the mouse wheel in each pane to choose your preferred tracing zoom.

5. Save your modelling work into **`Liger.solidarc`**. The individual launchers under `Views/` all reopen that **same file**, not separate copies of your model. Close/save the current session before opening another launcher. If you Save As another filename, update the launchers' `--open` path.

The sidecar options configure the startup cameras/layout; they are not secretly embedded camera fields in the `.solidarc` format. Opening only the document with some other executable does not automatically apply those startup options. Use the supplied launchers and keep the project folder intact.

### Downloads

- **`Packages/Liger-Reference-Project.zip`** — project, registered images, printable schematics, optional patch layouts and view launchers; no executable.
- **`Packages/SolidArc-Liger-Windows-x64.zip`** — all of the above **plus freshly rebuilt Windows desktop runtime**, fonts, SVG icons and notices.
- **`Packages/SolidArc-Liger-Linux-x64.zip`** — all of the above **plus freshly rebuilt Linux/X11 desktop runtime**, fonts, SVG icons and notices.

These are new builds, not renamed copies of the earlier W95 packages. The original package downloads and CAD reviews remain unchanged.

## Schematics / scale

| Vehicle view | Registered tracing image | Printable schematic | SolidArc cube preset used by launcher |
|---|---|---|---|
| Top | [7023 × 3844](References/Top.png) | [Top](Schematics/Liger_Top.png) | Top |
| Bottom | [7023 × 3844](References/Bottom.png) | [Bottom](Schematics/Liger_Bottom.png) | Bottom |
| Left side | [7023 × 2102](References/Left.png) | [Left](Schematics/Liger_Left.png) | Back |
| Right side | [7023 × 2102](References/Right.png) | [Right](Schematics/Liger_Right.png) | Front |
| Front / nose | [3844 × 2102](References/Front.png) | [Front](Schematics/Liger_Front.png) | Right |
| Rear / tail | [3844 × 2102](References/Rear.png) | [Rear](Schematics/Liger_Rear.png) | Left |

**Original CAD axes are preserved:** vehicle nose **+X**, vehicle left **+Y**, up **+Z**. SolidArc's generic cube calls the −Y-facing view “Front” and +X-facing view “Right”; this is why the vehicle-view names and cube labels differ. The named launchers select the correct vehicle view for you. Opposite profiles are genuinely projected from their respective directions, not a mislabeled reused picture.

- All six raw images: **1500 pixels per metre** in both axes; **1 pixel = 0.6667 mm**. This is a registration/sample scale, **not a CAD accuracy guarantee**.
- Body envelope from the sampled CAD: **4440.8 mm long × 2322.3 mm wide × 1001.8 mm tall**. The model top is at Z ≈ 1160.9 mm; its lowest body point is at Z ≈ 159.1 mm. “Body height” excludes ground clearance and is not wheel-to-roof height.
- Registered images have transparent backgrounds and **no title/legend margins**. Their exact world-space placement, pixel dimensions and camera bases are in [registration.json](registration.json).
- Printable sheets add title/legend margins and world datum guides. **Do not replace a registered tracing image with its printable sheet without adjusting the crop**, or the scale will be wrong.
- PNGs include 300 DPI metadata for convenience. Print applications must preserve the intended scale; the physical truth for CAD is the world mapping in the project/registration manifest, not automatic “fit to page”.

### What the highlighted lines mean

- **Dark silhouette** on paper / pale blue in the project: projected body boundary.
- **Cyan:** visible free rims and openings.
- **Orange:** visible CAD crease rails; adjacent sampled normals differ by at least 8° at the p95 screening level.
- **Dashed paper guides:** world datums, not styling lines or inferred wheel centres.
- **Smooth patch seams are omitted** from the main schematics. [PatchLayouts/](PatchLayouts/) contains optional visible all-rail views if you want to investigate the original surface division. They are not loaded into your modelling project.

**Orange does not mean every line is an intentional styling crease.** The source CAD still has tight folds/imperfections; use these as reference geometry, not a recommendation to reproduce every flaw. No new design curves, wheels, tyres, cabin/interior, underfloor or mechanical dimensions were invented. The bottom view is the reverse projection of the existing open exterior sheets, **not an engineered underside drawing**. These are modelling references, not manufacturer blueprints or manufacturing-tolerance drawings.

Hidden curves are suppressed with a triangulated CAD depth buffer, bilinear depth interpolation and a 0.2 mm numerical allowance. Silhouettes come from the coverage of the same evaluated CAD triangles. This is a computed high-resolution line-art projection, not an exact analytic hidden-line-removal certificate or AI-generated interpretation.

## View setup / native support

The main document contains **six image planes, zero modelling steps, zero imported CAD bodies**. Each view is positioned behind the body envelope in its own viewing direction. Image opacity is 0.8; colours and transparency are retained.

The rebuilt runtime adds only the support needed to make this reference setup behave correctly:

- Optional startup **view preset, single/quad layout, and reference-image framing** through the existing options file.
- Optional **one-sided reference images**, evaluated independently for each viewport. Thus top/bottom or front/rear images do not overprint each other as you switch views or use quad panes. Old projects remain two-sided by default.
- **Space / frame-document** can frame a reference-only document without an existing solid.
- **Alpha-aware mip filtering** keeps high-resolution reference ink continuous when zoomed out; registered reference strokes are bolder than the printable sheets for viewport readability.

All existing modelling tools remain available; no car surfaces or tracing curves were pre-modelled for you. The CAD importer fixes from the earlier Liger work are also included in these new binaries. Use the supplied runtime rather than an old W95 executable: older versions ignore the new one-sided/startup settings and can overlay opposite reference images incorrectly.

## Runtime requirements / honest testing limits

- **Windows x64:** Unsigned cross-build for Windows with Universal CRT and a working Vulkan 1.3-capable graphics driver. The cross-build statically links the C++ runtime; no separate compiler installation is needed. **Windows interactive execution is not tested in this Linux sandbox.**
- **Linux x64:** built on Debian 12 with GCC 12, using the real **GLFW X11 backend**, not GLFW's null-platform fallback. Requires a usable X11/XWayland desktop and Vulkan driver/system loader. System C/C++ libraries are not bundled. It is not a native Wayland-only build. Desktop window presentation was not exercised on this headless host; the real offscreen Vulkan UI was tested.
- Six full-resolution reference textures plus their filtered mip levels require roughly **535 MB** before staging/rendering overhead. A normal desktop GPU is recommended. The runtime ZIPs do not bundle a graphics driver.

Native project decoding, image decoding, view-facing logic, startup cameras, existing document regressions and the actual Vulkan reference viewport are checked separately. An offscreen capture is not a Windows interactive desktop test.

## Audit

| Requirement | Status | Evidence |
|---|:---:|---|
| Six orthographic reference views | 🟢 | Registered PNGs and printable sheets above |
| Same vehicle / no invented features | 🟢 | Latest refined CAD source; no artistic reconstruction |
| Common scale / aligned project planes | 🟢 | Pixel-to-world regression and registration manifest |
| Scratch modelling project | 🟢 | `Liger.solidarc`: image references only; empty feature tree |
| Feature curves emphasized, patch clutter separated | 🟢 | Silhouette / opening / crease colours; optional layouts |
| Python regression suite | 🟢 | [32 tests](Validation/PythonTests.txt), including prior CAD checks |
| Native reference checks | 🟢 | [111 native assertions](Validation/NativeProjectTests.txt); image decoding, codec round trip and all six view-facing selections |
| Existing modelling regressions | 🟢 | [869 passed / 0 failed](Validation/DocumentRegression.txt) |
| Actual Vulkan reference viewport | 🟢 | [Quad](Validation/GPU/Liger_Quad.png) / [Top](Validation/GPU/Liger_Top.png); 13 scripted assertions passed |
| Minified reference ink | 🟢 | [98.4% direct coverage, 100% within one pixel](Validation/MinificationCheck.json) at the mapped expected ink pixels |
| New Windows/Linux binaries with full icons | 🟢 | Both desktop builds succeeded; 310 SVG icons (66 SolidArc) and 5 DM Sans faces per runtime; [build manifest](Validation/BuildManifest.json) |
| Reference web gallery | 🟢 | [18 images, all downloads, mobile layout; no JS/HTTP errors](Validation/BrowserTests.txt) |
| Windows interactive execution | 🟡 | Not available in this Linux sandbox |
| Manufacturer / Class-A / underside data | 🟡 | Not claimed; only available exterior CAD is projected |

## Rebuild / reproduce

Python: use the existing pinned `../Tools/requirements-refined.txt` environment, plus Python 3.11+ for the regression runner. Run from the parent **Liger** directory:

```sh
python Tools/build_blueprints.py
python Tools/setup_blueprint_project.py  # fast project/launcher refresh without rendering
CERTIFIER=/path/to/CadCertifier python -m unittest discover -s Tools -p 'test_*.py' -v
```

Native source: use Frontier `82ed2512`, the [W95 overlay](../../../Patches/SolidArc-Booleans-Variables-Modifiers.patch), both prior NURBS importer patches under `../Tools/Patches/`, and [SolidArc-Reference-Startup-Views.patch](../../../Patches/SolidArc-Reference-Startup-Views.patch). For the full desktop build, seat that source into the supported SultanAladin/Frontier host (`65cf0f1`) with `Experimental/SolidArc/Native/Tooling/OverlayFrontier.py --frontier ... --copy`, then prepare the pinned dependencies/fonts. The new source files are mirrored under `SolidArc/Source/` as well.

Configure the host with `-DCMAKE_PROJECT_INCLUDE=/absolute/path/to/Tools/BlueprintValidation.cmake` to add `LigerBlueprintValidation`; build it and run it with the absolute path to this project folder. The existing `SolidArcDocumentValidation` exercises the previous modelling regressions. Desktop target: `SolidArc`; Vulkan offscreen target: `SolidArcCapture`. Windows uses the existing Zig toolchain/cross-build procedure, with Zig 0.14.1 for this delivery. Linux uses GLFW X11 development headers.

The browser gallery can be served from the Liger folder with `python -m http.server 8081 --bind 0.0.0.0 --directory Blueprints`. Its regression uses the existing pinned browser dependencies in `Refined/Tools` (`npm ci` there, then `node Tools/test_blueprint_gallery.mjs` from Liger).

Packaging is reproducible with `Tools/package_blueprints.py`, using explicitly supplied installed Windows/Linux runtime directories. The package tool verifies archive CRCs, payload SHA256 hashes, SVG icon completeness and expected executable formats. No build caches, compiler SDKs or downloaded dependency trees are checked into this project.
