#!/usr/bin/env python3
"""Validation for host build-budget regressions found by the Windows build."""
from __future__ import annotations

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    motion = read("Engine/SlateUI/Interface/MotionIntegrator/Api/MotionIntegrator.h")
    match = re.search(r"EaseCapacity\s*=\s*(\d+)u", motion)
    require(match is not None, "MotionIntegrator must declare EaseCapacity")
    require(int(match.group(1)) >= 8192, "MotionIntegrator eased capacity must cover runtime editor startup registrations")
    require("static storage in the windowed hosts" in motion, "MotionIntegrator comment must record why the reserve is safe")

    editor = read("Engine/Application/EditorHost/Source/EditorHost.cpp")
    require("constexpr std::size_t AutomaticUiBytes = sizeof(ShaderCodec) + sizeof(WorkspaceOverlayPass);" in editor,
            "EditorHost stack assertion must only count members still on automatic storage")
    for needle in [
        "static ViewportSequence Viewport;",
        "static WorkspaceIndex          Workspaces;",
        "static WorkspacePanel          Workspace;",
        "static EditorPanel             WorkspacePanels;",
        "static ControlCentrePanel      ControlCentre;",
        "static SceneDirectoryPanel     SceneDirectory;",
        "static TexturePaintPanel        TexturePaint;",
        "static ParametricWorkspacePanel SketchDirectory;",
        "static ParametricToolsPanel    ParametricTools;",
    ]:
        require(needle in editor, f"EditorHost missing static storage move {needle!r}")
    require("_dupenv_s(&Home" in editor, "EditorHost must avoid MSVC getenv warning on Windows")
    require("std::strncpy" not in editor, "EditorHost must avoid MSVC strncpy warning")
    require("ResizedGeometryOffering" in editor and "ResizedGeometryOutcome" in editor,
            "EditorHost resize path should not shadow geometry construction locals")
    require("ConsumeSharedCodexActivation" in editor, "EditorHost activation must use the shared codex activation helper")

    validation = read("Engine/Application/InterfaceValidationHost/Source/InterfaceValidationHost.cpp")
    require("std::strncpy" not in validation, "InterfaceValidationHost must avoid MSVC strncpy warning")

    shared_cad = read("Engine/Application/Api/SharedCadDrawingController.h")
    require("ResolveSharedCadDraftSubject" in shared_cad and "SharedCadDraftRequiredAnchors" in shared_cad,
            "shared CAD drawing controller must own the tool-to-draft dispatch")
    require("SharedCadDrawingController.h" in editor and "ResolveSharedCadDraftSubject" in editor,
            "EditorHost must consume the shared CAD drawing controller dispatch")

    shared_viewport = read("Engine/Application/Api/SharedViewportHostBridge.h")
    require("RecordSharedViewportGizmo" in shared_viewport and "HitSharedViewportGizmo" in shared_viewport,
            "shared viewport bridge must own the one-at-a-time gizmo dispatch")
    require("Extent.MinimumX + 52.0f" not in shared_viewport and "Extent.MaximumX - 70.0f" in shared_viewport,
            "both Blender and CAD viewport gizmos must be anchored at the top-right of the viewport")
    require("SharedViewportCameraDepth" in shared_viewport,
            "shared viewport gizmos must use the HTML-reference camera-forward depth ordering")
    require("DrawFaceLabel" in shared_viewport and "TextRun(Face" not in shared_viewport,
            "CAD cube face labels must be projected face strokes, not hovering screen-space text")
    require("const float Scale = 40.0f" in shared_viewport and "Surface.Tongue(Face.Corners, 4u" in shared_viewport,
            "CAD cube must stay large and fill each face as one quad, not visible triangle halves")
    require("ThemeToken{ 255u, 255u, 255u, 235u }" in shared_viewport,
            "CAD cube projected face labels must render white")
    require("CenterActivatedSceneAtWorldOrigin" in shared_viewport and "CenterActivatedSceneAtWorldOrigin(Loaded)" in shared_viewport,
            "codex scene activation must recenter loaded geometry at the world origin")

    editor_panel = read("Engine/SlateUI/Interface/EditorPanel/Source/EditorPanel.cpp")
    editor_panel_api = read("Engine/SlateUI/Interface/EditorPanel/Api/EditorPanel.h")
    require("PopupOpen" in editor_panel and "PopupOpen" in editor_panel_api,
            "Editor popup disclosure state must be queryable so hosts can block background pointer paths")
    require("Covering(0x18191Eu)" in editor_panel and "Pointer.ContactPressed" in editor_panel,
            "Editor dropdown and grid popup menus must be opaque and capture press/release contact")
    overlay_pass = read("Engine/SlateVulkan/Device/WorkspaceOverlayPass/Source/WorkspaceOverlayPass.cpp")
    overlay_api = read("Engine/SlateVulkan/Device/WorkspaceOverlayPass/Api/WorkspaceOverlayPass.h")
    require("LeafRect.MinimumX" in editor and "LeafRect.MaximumX" in editor and "!ForegroundDrawerStanding" not in editor,
            "Editor viewport overlay grid must remain visible outside open drawers instead of being globally suppressed")
    require("LeafX0" in overlay_api and "Push.LeafRect[0] = LeafX0" in overlay_pass,
            "Drawer clipping must not change the grid projection/aspect; scissor and logical viewport rect stay separate")

    tea_generator = read("Tools/CreateWhiteTeaServiceCodex.py")
    require('b"".join(channels) + u32(0) + u32(1) + material_layer()' in tea_generator,
            "WhiteTeaService.codex material section must write zero images before its layer count")

    content_browser = read("Engine/SlateUI/Interface/ContentBrowserPanel/Source/ContentBrowserPanel.cpp")
    require("ActivationRequested = Library.Taken" in content_browser and "ActivationRequested = Index" in content_browser,
            "Content Browser card and Import button must request codex activation")
    require("ImportPressed" in content_browser and "Sampled.ContactReleased" in content_browser,
            "Content Browser import button must visibly press and activate on click release")

    tools = read("Engine/SlateUI/Interface/ParametricTools/Source/ParametricToolsPanel.cpp")
    require("static_cast<void>(Elapsed);" in tools, "ParametricToolsPanel must mark Elapsed as intentionally unused")
    require("Bezier" in tools and "Hermite" in tools and "NURBS Curve" in tools,
            "Sketch Draw must expose the full primary curve set")

    parametric_spec = read("Engine/SlateUI/Interface/ParametricWorkspace/Api/ParametricWorkspaceSpecification.h")
    parametric_panel = read("Engine/SlateUI/Interface/ParametricWorkspace/Source/ParametricWorkspacePanel.cpp")
    parametric_bridge = read("Engine/Application/Api/ParametricWorkspaceBridge.h")
    require("ExtrusionCapToggleDemand" in parametric_spec and "Extrude Caps" in parametric_panel,
            "closed profile properties must expose a capped/wall extrusion toggle")
    require("Curve Closure" in parametric_bridge and "Extrude Result" in parametric_bridge and "Extrude Caps" in parametric_bridge,
            "closed profile inspector data must distinguish closed-loop rendering from capped solid extrusion")

    theme = read("Engine/SlateUI/Interface/ThemeInterchange/Source/ThemeInterchange.cpp")
    require("std::strncpy" not in theme, "ThemeInterchange must avoid MSVC strncpy warning")

    print("[HostBuildBudgets] editor build budgets and warning fixes hold")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"[HostBuildBudgets] failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
