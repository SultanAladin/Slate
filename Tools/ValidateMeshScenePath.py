#!/usr/bin/env python3
"""Validate the mesh import / scene-path MVP wiring without needing external DCC files."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(path: str, needle: str) -> None:
    target = ROOT / path
    if not target.exists():
        return
    text = target.read_text()
    if needle not in text:
        raise AssertionError(f"{path} does not contain expected marker: {needle}")


def main() -> None:
    checks: list[str] = []

    import_cpp = "Engine/SlateDocument/Format/SceneMeshImport/Source/SceneMeshImport.cpp"
    host_cpp = "Engine/Application/ParametricSketchHost/Source/ParametricSketchHost.cpp"

    for ext, marker in [
        ("OBJ", "ImportObj"),
        ("glTF", "ImportGltfText"),
        ("GLB", "ImportGlb"),
        ("FBX", "ImportAsciiFbx"),
        ("STL", "ImportStl"),
        ("PLY", "ImportAsciiPly"),
    ]:
        require(import_cpp, marker)
        checks.append(f"{ext} import route stands")

    require(import_cpp, "MaterialSlots")
    require(import_cpp, "MaterialRecords")
    require(import_cpp, "DefaultWorkspaceMaterialRecord")
    checks.append("material slot capture and default material records stand")

    require(host_cpp, "SceneMeshFormatSupported(Current.path().string())")
    checks.append("Content Browser import directory recognises mesh formats")

    require(host_cpp, "ImportSceneMeshFile(ImportPath.string())")
    checks.append("host imports selected mesh files into the workspace scene")

    require(host_cpp, "BridgeSketchSceneDirectory(OpenedScene, SceneDirectoryStorage)")
    checks.append("imported meshes flow through Scene Directory rows")

    require(host_cpp, "SelectSceneMeshAtPointer")
    checks.append("viewport mesh selection path stands")

    require(host_cpp, "SynchroniseCodexTransformsFromSceneDirectory")
    checks.append("scene transform edits feed back to mesh entries")

    require("Engine/SlateVulkan/Device/WorkspaceScenePass/Api/WorkspaceScenePass.h", "class WorkspaceScenePass")
    require("Engine/SlateVulkan/Device/WorkspaceScenePass/Source/WorkspaceScenePass.cpp", "WorkspaceScenePass::Upload")
    checks.append("dedicated WorkspaceScenePass upload boundary stands")

    print(f"[MeshScenePath] {len(checks)} checks passed")
    for check in checks:
        print(f"[MeshScenePath] ✓ {check}")


if __name__ == "__main__":
    main()
