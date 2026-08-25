#!/usr/bin/env python3
"""Validation for the ReSTIR GI planning pass and CAD shader push constant fix."""
from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def require_text(path: str, *needles: str) -> str:
    content = read(path)
    for needle in needles:
        require(needle in content, f"{path} missing {needle!r}")
    return content


def main() -> int:
    vertex = require_text(
        "Engine/SlateVulkan/Device/WorkspaceCadPass/Shader/WorkspaceCadVertex.slang",
        "struct CadPushConstant",
        "Unsigned32 Draw;",
        "Output.Draw = Push.Draw;",
        "if (Push.Draw == WorkspaceCadFillDraw)",
        "else if (Push.Draw == WorkspaceCadSegmentDraw)",
    )
    require("Push.Role" not in vertex and "Output.Role" not in vertex, "CAD vertex shader still references the old push field")

    fragment = require_text(
        "Engine/SlateVulkan/Device/WorkspaceCadPass/Shader/WorkspaceCadFragment.slang",
        "Unsigned32 Draw    : TEXCOORD1;",
        "if (Input.Draw == WorkspaceCadMarkerDraw)",
    )
    require("Input.Role" not in fragment, "CAD fragment shader still references the old varying field")

    require_text(
        "Engine/SlateFeature/Sketch/ProfileReshape/Source/ProfileReshape.cpp",
        "return Outcome<SketchCurveName>::Refuse({ RefusalReason::ContentUnsupported, \"the curve cannot be trimmed\" });",
    )

    plan = require_text(
        "References/RestirGIRealtimeArchitecturePlan.md",
        "GTX 1060+ compute baseline",
        "RTX / hardware ray query path",
        "RTX high path / ray pipeline path",
        "Software BVH for Tier A",
        "Hardware acceleration structures for Tier B/C",
        "Motion vector pass",
        "Temporal resampling",
        "Denoising and temporal accumulation",
        "Implementation phases recommended for Slate",
    )
    lower = plan.lower()
    require("kind" not in lower and "asset" not in lower and "role" not in lower,
            "ReSTIR GI plan must avoid banned vocabulary")

    print("[RestirGIPlan] CAD shader fix and ReSTIR GI architecture plan hold")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"[RestirGIPlan] failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
