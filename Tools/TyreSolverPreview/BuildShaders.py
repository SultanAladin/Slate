#!/usr/bin/env python3
"""Lowers the six tyre entry points to WGSL for the WebGPU preview — the same .slang sources the Vulkan build lowers to SPIR-V.

Usage:  python3 Tools/TyreSolverPreview/BuildShaders.py [path/to/slangc]

The compiler is found on the argument, on $SLANGC, under $VULKAN_SDK/bin, or on PATH. The only difference from the
engine's own lowering is `-DSLATE_WEBGPU_TOOLCHAIN=1`, which Foundation/Toolchain.h reads to narrow Real64 to the
one width WGSL has and which TyreDeclarations.slang reads to bind the step block as a uniform at slot 14 rather
than as a push constant.
"""

import os
import shutil
import subprocess
import sys

Root        = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ShaderRoot  = os.path.join(Root, "Engine", "SlateCompute", "Compute", "TyreSolver", "Shader")
TargetRoot  = os.path.join(Root, "Tools", "TyreSolverPreview", "Shader")
EntryPoints = ["TyrePrediction", "TyreCordProjection", "TyreShapeGather",
               "TyreShapeSolve", "TyreContactProjection", "TyreVelocityRecovery"]


def FindCompiler():
    Candidates = []
    if len(sys.argv) > 1:
        Candidates.append(sys.argv[1])
    if os.environ.get("SLANGC"):
        Candidates.append(os.environ["SLANGC"])
    if os.environ.get("VULKAN_SDK"):
        Candidates.append(os.path.join(os.environ["VULKAN_SDK"], "bin", "slangc"))
        Candidates.append(os.path.join(os.environ["VULKAN_SDK"], "Bin", "slangc.exe"))
    Found = shutil.which("slangc")
    if Found:
        Candidates.append(Found)
    for Candidate in Candidates:
        if os.path.isfile(Candidate) or shutil.which(Candidate):
            return Candidate
    sys.exit("slangc not found: pass its path, set SLANGC or VULKAN_SDK, or put it on PATH")


def main():
    Compiler = FindCompiler()
    os.makedirs(TargetRoot, exist_ok=True)
    for Entry in EntryPoints:
        Source = os.path.join(ShaderRoot, Entry + ".slang")
        Target = os.path.join(TargetRoot, Entry + ".wgsl")
        Command = [Compiler, Source, "-DSLATE_SHADER_TOOLCHAIN=1", "-DSLATE_WEBGPU_TOOLCHAIN=1",
                   "-I", os.path.join(Root, "Engine"), "-target", "wgsl", "-o", Target]
        if Compiler.endswith(".py"):
            Command.insert(0, sys.executable)
        print("  " + Entry)
        subprocess.run(Command, check=True)
    print("lowered %d entry points into %s" % (len(EntryPoints), os.path.relpath(TargetRoot, Root)))


if __name__ == "__main__":
    main()
