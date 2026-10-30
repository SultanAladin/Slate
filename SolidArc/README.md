# SolidArc CAD Audit, Features & Binary Packages

All deliverables for `unassignedinbox/Frontier` (`arena/d90f0539-frontier`, base `82ed2512`) are staged in this folder
on session branch `arena/4f335fa8-slate` so they appear directly in the Arena Workspace viewer and on GitHub.

---

## 1. Pre-Built Release Binaries

| Platform    | Package                                    | Contents                                                        |
|-------------|--------------------------------------------|-----------------------------------------------------------------|
| Windows x64 | `Package/SolidArc-Windows-x64.zip` (4.1M)  | `SolidArcWorkspace.exe`, `SolidArcWorkspace.toml`, fonts, icons |
| Windows x64 | `Package/Frontier-Windows-x64.zip` (3.9M)  | `Frontier.exe` (with SolidArc browser link), fonts, icons       |
| Linux x64   | `Package/SolidArc-Linux-x64.zip` (7.6M)    | `SolidArcWorkspace`, `SolidArcDocumentValidation`, fonts, icons |

Direct download URLs on GitHub (`arena/4f335fa8-slate`):
- `https://github.com/SultanAladin/Slate/raw/arena/4f335fa8-slate/SolidArc/Package/SolidArc-Windows-x64.zip`
- `https://github.com/SultanAladin/Slate/raw/arena/4f335fa8-slate/SolidArc/Package/Frontier-Windows-x64.zip`
- `https://github.com/SultanAladin/Slate/raw/arena/4f335fa8-slate/SolidArc/Package/SolidArc-Linux-x64.zip`

---

## 2. Applying Source Changes to `unassignedinbox/Frontier`

Apply the unified patch cleanly onto `unassignedinbox/Frontier` branch `arena/d90f0539-frontier` (`82ed2512`):

```bash
git checkout arena/d90f0539-frontier
git apply /path/to/SolidArc/Patches/SolidArc-Booleans-Variables-Modifiers.patch
cp /path/to/SolidArc/Package/SolidArc-Windows-x64.zip Exhibits/SolidArc/Package/SolidArc-Windows-x64.zip
```

Every modified and newly created C++ source file is also mirrored verbatim under `SolidArc/Source/` at its exact
`Frontier` repository path:
- `Source/Engine/GeometryProcessing/` — Boolean acceleration, carrier preservation, `BodyModifiers.{h,cpp}`,
  parametric variables, edge groups, modifier stack, and `ModifierCheckpoint` replay cache.
- `Source/UI/SolidArc/` — Live Boolean cutters, Variables card, Edge Weights card, Blender-style Modifiers stack UI,
  32-entry scene mesh LRU cache, `StageGizmoPreview` throttle fix, and `.solidarc` TOML v1 serialization.
- `Source/Exhibits/` — `SolidArcDocumentValidation.cpp` (`W72`–`W78`) and updated standalone kernel checks.

---

## 3. Audit & Validation Reports

- `ProductionAudit.md` — Full CAD completeness/correctness audit report with emoji matrix and root-cause analysis.
- `Results/DocumentValidation.md` — Complete `W1`–`W78` run (**794 🟢 / 0 🔴**).
