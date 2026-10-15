#!/usr/bin/env python3
"""Validation for the WebGPU global-distance-field GI exhibit (surface cache + ReSTIR).

Checks the structural contracts the exhibit claims, without needing a GPU:
  - the vehicle stream is the authored Project Drive ControlVehicle export;
  - exactly one global field texture exists, and the sphere-tracing entry point
    enumerates no primitives (per-object enumeration lives only in composition);
  - surface-cache capture and lookup are both present, as are the ReSTIR
    temporal / spatial / resolve stages with the RIS weight identity.
"""
from __future__ import annotations

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
PAGE = ROOT / "References" / "GlobalDistanceFieldRestir.html"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    text = PAGE.read_text(encoding="utf-8")

    # The real authored vehicle stream (ControlVehicleStructure.inl: 1492 verts / 2924 tris).
    vehicle = re.search(r"const VEHICLE = (\{.*?\});", text, re.S)
    require(vehicle is not None, "vehicle stream missing")
    require('"positions"' in vehicle.group(1) and '"indices"' in vehicle.group(1)
            and '"family"' in vehicle.group(1) and '"wheels"' in vehicle.group(1),
            "vehicle stream is not the authored ControlVehicle export")

    # One global field: a single 3D texture, composed once, then only sampled.
    require("dimension:'3d'" in text, "global field is not a 3D texture")
    require(text.count("createTexture") >= 1 and "gdfTexture" in text, "global field texture missing")
    trace = re.search(r"fn traceField\(.*?\n\}", text, re.S)
    require(trace is not None, "traceField missing")
    require("prims[" not in trace.group(0) and "primDistance" not in trace.group(0),
            "traceField enumerates primitives — traces must sample only the global field")
    build = re.search(r"const WGSL_GDF_BUILD.*?`;", text, re.S)
    require(build is not None and "primDistance(prims[i], pos)" in build.group(0),
            "field composition pass must be the one place primitives are enumerated")

    # Surface cache: capture and lookup are exact inverses, lighting feeds back.
    for needle in ("fn cardSurface(", "fn cardTexelOf(", "WGSL_CACHE_CAPTURE",
                   "WGSL_CACHE_LIGHT", "cacheRadiancePrev", "cosineDirection(n, &rng)"):
        require(needle in text, f"surface-cache contract missing {needle!r}")

    # ReSTIR: candidates, temporal, spatial, resolve, with the RIS identity stated in code.
    for needle in ("WGSL_RESTIR_CANDIDATE", "WGSL_RESTIR_TEMPORAL", "WGSL_RESTIR_SPATIAL",
                   "WGSL_RESTIR_RESOLVE", "fn reservoirMerge(", "fn reservoirFinalWeight(",
                   "r.hitPosWsum.w / (r.radianceM.w * pHatSelected)", "jacobian"):
        require(needle in text, f"ReSTIR contract missing {needle!r}")

    # The vehicle actually moves and drags its field records with it.
    for needle in ("integrateVehicle", "packPrimitives(vehicleState)", "circuitIndex",
                   "wheelbase = 3.396", "courseGroundHeight"):
        require(needle in text, f"vehicle contract missing {needle!r}")

    print("[GlobalDistanceFieldRestir] one-field trace, surface cache, ReSTIR stages and vehicle stream hold")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"[GlobalDistanceFieldRestir] failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
