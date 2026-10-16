# TyreSolverPreview — the GPU XPBD tyre fleet in a browser

A WebGPU page that runs **the engine's own six tyre kernels** — `Engine/SlateCompute/Compute/TyreSolver/Shader/*.slang`,
lowered to WGSL instead of SPIR-V — over a fleet of cars driving a ring road, with every wheel's load read back to a
CPU vehicle sim two frames late exactly as `TyreDeviceExchange` does it in Vulkan.

## Run it

```
python3 -m http.server 8000            # from the repository root
# then open  http://localhost:8000/Tools/TyreSolverPreview/
```

Needs a WebGPU browser: Chrome/Edge 113+, Safari 18+, Firefox 141+. The page requests `timestamp-query` when the adapter
offers it, so the HUD shows the compute pass's GPU time directly.

## What is in here

| File | Origin |
|---|---|
| `Shader/*.wgsl` | **Generated** by `BuildShaders.py` from the engine's `.slang` with `-DSLATE_WEBGPU_TOOLCHAIN=1`. Do not edit; rebuild. |
| `TyreLattice.bin` | **Generated** by `ExportTyreLattice.cpp` — one tyre as `ConstructTyreLattice` built it (prestressed at 2.2 bar, cords coloured). The page tiles it into the fleet. |
| `TyreSolverPreview.js` | The dispatch loop (TyreDeviceExchange::Record's order), the readback ring, a quarter-car-per-wheel vehicle sim on a rigid body, the road, the renderer. |
| `index.html` | The page and its HUD. |

Regenerate after touching the kernels or the lattice construction:

```
python3 Tools/TyreSolverPreview/BuildShaders.py                    # finds slangc on PATH / $VULKAN_SDK / $SLANGC
g++ -std=c++20 -O2 -IEngine -o ExportTyreLattice Tools/TyreSolverPreview/ExportTyreLattice.cpp \
    Engine/SlateCompute/Compute/TyreSolver/Source/TyreSolver.cpp && ./ExportTyreLattice Tools/TyreSolverPreview/TyreLattice.bin
```

## The two toolchain differences, and nothing else

* WGSL has no `double`. `Foundation/Toolchain.h` narrows `Real64` to `float` under `SLATE_WEBGPU_TOOLCHAIN`; every tyre
  routine that widens does so for an intermediate and narrows back before storing, so the Bounded guarantee stands at a
  wider bound.
* WebGPU has no push constants. `TyreDeclarations.slang` binds the step block as a uniform at slot 14 under the same
  define; the page lays out one 256-byte block per dispatch and reaches each by dynamic offset.

Both the integer spans are `RWStructuredBuffer<Atomic<Signed32>>` on every target now, because WGSL admits no atomic on a
plain integer and Slang's `Atomic<T>` is the spelling both targets accept.

## Does it scale to twenty cars?

Yes, and the reason is structural rather than hopeful. The frame is **32 substeps × 13 dispatches = 416 dispatches**
regardless of how many tyres there are — every dispatch covers the whole fleet, and the one per-tyre kernel (the shape
solve) is one workgroup per tyre. Twenty cars is 80 tyres, 81 920 particles, 297 k cords; each dispatch is a few hundred
workgroups, which is finally enough to occupy a discrete GPU. Memory is about 20 MB. On a desktop GPU expect the compute
pass in the 3–6 ms range at 80 tyres; an integrated GPU will want fewer substeps or fewer cars, and the sliders are there
for exactly that. The cost that does not scale with tyres is the 416 inter-dispatch barriers, which is why the Vulkan
form belongs on an async compute queue.

The readback is 64 bytes per tyre into a three-deep ring of mapped buffers; the vehicle integrates against the secant
stiffness each load implies, so the two-frame latency does not ring (the rule is written up on `ContactLoad` in
`TyreSolver.h`). `Σ Fy / weight` in the HUD should sit at 1.00 for the watched car on flat road and swing through the hump.
