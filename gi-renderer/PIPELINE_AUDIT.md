# ReSTIR GI Renderer — Pipeline Audit

Audit of the WebGPU ReSTIR GI renderer in `gi-renderer/`, covering the full pipeline from primary
ray generation through to presentation.

**Sources read:** `src/webgpu/wgsl.ts`, `src/webgpu/renderer.ts`, `src/webgpu/mesh.ts`, `src/App.tsx`,
`vite.config.ts`, `package.json`.

**Status:** verification only. HTML single-file conversion is deferred and not yet performed.

---

## Table 1 — Accuracy audit, primary ray to present

| # | Stage | Verdict | Finding |
|---|---|---|---|
| 1 | Uniform buffer layout (`renderer.ts:301`) | **PASS** | All 35 fields verified byte-for-byte against WGSL std140 (240 B in a 256 B buffer). vec4 alignment, u32/f32 interleaving, `cam_right.y = 0` padding — all correct. |
| 2 | Storage layouts (Tri 96 B, BVH 32 B, Reservoir 48 B, GBuffer 64 B) | **PASS** | Struct sizes and JS packing offsets all match. |
| 3 | Ping-pong / pass barriers | **PASS** | Bind group parity is correct; WebGPU guarantees inter-dispatch hazard sync inside one compute pass, so no missing barriers. Buffers are spec-zero-init, so the uncleared reset is safe. |
| 4 | BVH build + miss-link flatten (`mesh.ts:356`) | **PASS** | DFS order, `leftFirst = i+1` for internal nodes, `missLink = i + subtreeSize`, root to sentinel. Traversal logic is sound. |
| 5 | Camera basis + reprojection math | **PASS** | Focal `z=2.0` in the primary ray matches the `x2.0/dist_z` in both reprojection sites. `prev*` updated after the write. Consistent. |
| 6 | **Missing cosine at resolve** (`wgsl.ts:717`) | **BIAS — severe** | `final = Li*(albedo/pi)*W`. RIS requires the *full* integrand `f = Li*albedo/pi*cos(theta)`. With M=1 this yields `Li*rho/cos(theta)` instead of `Li*rho` — **overbright by 1/cos(theta), diverging at grazing angles**. The cosine is in `target_pdf` but never in `f`. |
| 7 | **Mixture PDF not applied** (`generate_candidate:358`) | **BIAS — severe** | Strategy is picked 30/70 (sun cone vs cosine) but the returned `pdf` is the *conditional* pdf, not `0.3*p_cone + 0.7*p_cos`. Sun is under-weighted ~3.3x, diffuse ~1.43x. Worse: an occluded sun sample silently falls through to the cosine branch, so visibility is baked into the sampling decision — you cannot reject on the integrand and keep the pdf. |
| 8 | **Three inconsistent sun radiances** | **BIAS — severe** | (a) BSDF hit on the sun sphere gives `emission = vec3(I)`; (b) `sky_radiance(d, true)` disk gives `I*60*(1,.96,.9)`; (c) NEE at each bounce gives `albedo*cos*I`, **missing the `/pi` BRDF normalisation**. Path (b) is what ReSTIR's sun strategy stores, so the sun is literally 60x brighter through one estimator than the other. |
| 9 | **Sun double-counting** (`generate_candidate:418` + `:446`) | **BIAS** | NEE fires a sun shadow ray *and* the escape branch adds `sky_radiance(dir, true)` including the disk. No MIS weight anywhere. The same path counts the sun twice. |
| 10 | Ambient hack (`wgsl.ts:421`) | **NON-PHYSICAL** | `radiance += throughput * albedo * sky_radiance(n, false) * 0.1` — magic 0.1, sky radiance in the normal direction used as if it were irradiance, no cosine integral, no visibility, added at *every* bounce. |
| 11 | **No light sampling of emissive geometry** | **GAP — major** | The glowing sphere (`emission [20,40,80]`, ~1100 triangles) is the scene's main light and has **zero NEE**. It is found only by chance BSDF hits. A ReSTIR *GI* demo with no ReSTIR DI over emitters. |
| 12 | ReSTIR combiner (temporal + spatial) | **BIAS — known** | Raw `w = p_hat*W*M` M-weighting from the 2020 paper, no generalized/pairwise MIS (GRIS). Produces boundary bias wherever reuse domains disagree. `M` is capped at `maxHistory*N = 480`, so at least it is bounded. |
| 13 | **No temporal visibility revalidation** | **BIAS** | The spatial pass shadow-tests every accepted neighbour (`:681`); the temporal pass does not. Stale reservoirs survive on normal/depth/plane tests alone, producing light leaks and shadow ghosting. No periodic sample validation either. |
| 14 | Spatial result never fed back | **DESIGN GAP** | `pass_spatial` writes `res_spatial`; `pass_temporal` reads `res_prev` (= last frame's `res_curr`, pre-spatial). Spatial reuse is discarded every frame — reuse never compounds, which is the entire point of ReSTIR. |
| 15 | Jacobian (`shift_jacobian:333`) | **PASS (clamped)** | `(cos_new*d_old^2)/(cos_old*d_new^2)` is the correct reconnection Jacobian. Clamp to `[0,8]` is standard-practice bias-for-stability. |
| 16 | WRS `update_reservoir` | **PASS** | Correct weighted reservoir sampling; `W` field correctly preserved and recomputed downstream. |
| 17 | **Reprojection off kills accumulation** (`pass_resolve:749`) | **BUG** | With `enable_reprojection == 0`, `pass_temporal` never writes `histLen`, so `depth_hist.y` stays 1, resolve `histLen = 0`, `mix_factor = 1`. Turning off the *Reprojection* toggle silently disables *Accumulation* even on a static camera. |
| 18 | Resolve reprojects without validation | **BUG** | `pass_resolve` redoes the whole reprojection but only bounds-checks the UV — no normal/depth/plane test at `prev_idx`, unlike `pass_temporal`. Direct ghosting source. Also nearest-neighbour history fetch, giving stair-stepping under motion. |
| 19 | Sun sphere intersection (`:150`) | **PRECISION** | Sphere at t=1e4, r=500 gives `dot(oc,oc) ~ 1e8`, and `h = b^2 - c` with `b^2 ~ 1e8`. Catastrophic f32 cancellation; the disk edge is numerically ragged. An analytic `dot(dir, sd) > cos(a)` cone test is exact and cheaper. |
| 20 | Sun angular size | **NON-PHYSICAL** | `asin(500/1e4) = 0.05 rad ~ 2.9 deg` — about **11x the real sun** (0.53 deg). Consistent with `sunAngularRadius`, so at least self-consistent. |
| 21 | Sky model (`sky_radiance:296`) | **NON-PHYSICAL** | Kasten-Young airmass is the right *shape*, but `betaR*4e5`, `betaM*turbidity*0.2`, `exp(-...*0.35)` are fitted magic. Single-scatter only, so the horizon is too dark, with no ozone and no aerial perspective. Not Preetham/Hosek. |
| 22 | Metals (`pass_temporal:490`) | **NON-PHYSICAL** | No Fresnel/Schlick, no GGX — roughness is a uniform cone jitter, which is the wrong lobe shape. No specular on dielectrics at all; a surface is binary metal-or-diffuse. No energy conservation between the two lobes. |
| 23 | Reflections bypass ReSTIR | **GAP** | Metal pixels are flagged `Lo_W.w = -1` and shaded by a raw path trace — no reuse, pure per-frame noise, smoothed only by temporal accumulation. |
| 24 | Reflections denoised with primary geometry | **BUG** | Metal pixels write the *primary* surface's normal/depth into the G-buffer, so every denoiser filters reflections using the mirror's own normals. Reflections get over-smoothed. |
| 25 | Denoiser 2 "A-Trous" | **MISNAMED** | Real a-trous is N sequential passes each filtering the previous output. This reads `accum_curr` at 3 step sizes in one pass — a sparse 3-ring blur. Also `kernel[0] = 0.5` is **dead code**: `max(abs(dx),abs(dy))` is always 1 in a 3x3 with the centre `continue`d. |
| 26 | Denoiser 3 "SVGF" | **MISNAMED** | Variance is estimated from a 3x3 spatial window of the already-accumulated signal, not temporally accumulated luminance moments. No history-length-driven kernel widening. |
| 27 | Denoisers 4 / 5 "IGN+TAA" / "FFX" | **MISNAMED** | #4 is a single tap at radius 3 with a *static* (non-frame-varying) IGN offset, giving structured artifacts, and there is no TAA in it. #5 is a 4-tap cross. Neither resembles its label. |
| 28 | All denoisers filter albedo x irradiance | **QUALITY BUG** | No albedo demodulation, so texture and colour detail is blurred along with the noise. |
| 29 | No firefly clamp, no NaN guard | **BUG** | RR divides by `rr_prob >= 0.05`, giving 20x throughput spikes with nothing catching them. Reinhard `col/(col+1)` on a negative or NaN value diverges; no `max(col, 0)`. |
| 30 | No sub-pixel jitter (`pass_temporal:471`) | **GAP** | `uv = id.xy / resolution` — no `+0.5` pixel-centre offset (half-pixel image shift) and no jitter. A full temporal accumulation buffer exists and delivers **zero anti-aliasing**. |
| 31 | RR from bounce 0 (`:436`) | **INEFFICIENT** | `clamp(p, 0.05, 0.95)` means a 5%-or-greater kill chance even at throughput 1.0, on the very first bounce. Pure added variance at 1-2 bounce budgets. |
| 32 | `is_moving` clobbered (`renderer.ts:374`) | **BUG** | Mouse-look sets `isMoving = true` at `:257`, then `render()` overwrites it with `moved` (keyboard only) at `:374`. Rotating the camera never triggers the `movingSpp` fast path. |
| 33 | `resetAccumulation` over-fires (`renderer.ts:434`) | **BUG** | Any settings change resets history — including **exposure** and **denoiser type**, which are pure post-process. Nudging exposure throws away 30 frames of convergence. |
| 34 | 9 storage buffers in one stage | **PORTABILITY RISK** | Bindings 2-10 are 9 storage buffers; the WebGPU default `maxStorageBuffersPerShaderStage` is **8** (compat mode: 4). This is why `requiredLimits` is there — it will hard-fail at pipeline creation on any adapter reporting 8. |
| 35 | No resize / DPR handling | **BUG** | Resolution is hard-clamped to 800x600 at init and never rebuilt. Resizing the window breaks the aspect ratio (CSS-stretched, `aspect` uniform stale). |
| 36 | `playing` starts `false` but engine starts | **UX BUG** | `init()` calls `start()`; the settings effect would `stop()` it but `rendererRef.current` is still null at that point. Button reads "Resume Engine" while it is running. |

**Bottom line on accuracy:** the *plumbing* is correct — layouts, ping-pong, BVH, Jacobian, WRS and
camera math all verify clean. The *radiometry* does not. Items 6, 7, 8 and 9 are four independent,
compounding energy errors on the same estimator.

---

## Table 2 — Improvements: correctness and image quality

| # | Change | Fixes | Effort | Impact |
|---|---|---|---|---|
| 1 | Multiply `final_radiance` by `max(dot(n, wi), 0)` in `pass_resolve` | #6 | 1 line | **Critical** — removes the 1/cos(theta) divergence |
| 2 | Return the true mixture pdf `0.3*p_cone + 0.7*p_cos`; on sun-sample rejection, return the *cosine* strategy pdf, not a fall-through | #7 | ~10 lines | **Critical** |
| 3 | Single `sun_radiance()` helper used by the sphere hit, the sky disk, and NEE; NEE gains its `/pi` | #8 | ~15 lines | **Critical** |
| 4 | Power-heuristic MIS between NEE and BSDF sampling; skip the disk term on escape when NEE already fired | #9 | ~25 lines | **Critical** |
| 5 | Emissive-triangle light sampling: build a power-weighted alias table on the CPU, sample it as a third strategy in `generate_candidate` | #11 | ~80 lines CPU + 30 WGSL | **Critical** — the main light currently has no NEE |
| 6 | Pairwise/generalized MIS in the reservoir combiner (GRIS) instead of raw M-weighting | #12 | ~40 lines | High |
| 7 | Temporal sample validation: re-trace each reservoir's stored sample every 4-8 frames, drop it if radiance changed by more than 2x | #13 | ~30 lines | High |
| 8 | Feed `res_spatial` into next frame's temporal read (add a 4th reservoir buffer or swap roles) | #14 | small | High — makes reuse actually compound |
| 9 | Validate the prev G-buffer in `pass_resolve`'s reprojection; use a 2x2 bilinear history fetch with per-tap validity weights | #18 | ~30 lines | High — kills ghosting and stair-stepping |
| 10 | Replace the sun sphere with an analytic cone test `dot(dir, sd) > cos(a)`; drop `sunAngularRadius` to ~0.0047 rad | #19, #20 | ~10 lines | Medium |
| 11 | Halton(2,3) sub-pixel jitter + `+0.5` pixel centre, with neighbourhood-clamped TAA resolve | #30 | ~25 lines | **High** — free AA, the accumulation buffer already exists |
| 12 | Owen-scrambled Sobol or a blue-noise texture instead of the hash PRNG | — | ~40 lines | High — large convergence-per-sample win |
| 13 | Albedo demodulation: filter irradiance only, remodulate after denoise | #28 | ~20 lines | **High** — biggest visible sharpness gain |
| 14 | Real multi-pass a-trous: 5 ping-pong iterations, proper B3-spline 5-tap kernel | #25 | ~60 lines | High |
| 15 | Real SVGF: temporally accumulate luma moments in the G-buffer's spare `depth_hist.zw`, drive kernel width from variance and history length | #26 | ~80 lines | High |
| 16 | GGX/VNDF sampling + Schlick Fresnel for metals; add a dielectric specular lobe with proper energy split | #22 | ~70 lines | High |
| 17 | ReSTIR (or at least spatial reuse) in a reflection domain, with its own hit-distance G-buffer for roughness-aware filtering | #23, #24 | ~100 lines | High |
| 18 | Firefly clamp (percentile or `min(L, 8*mean_luma)`), plus `max(col, 0)` and a NaN guard before tonemap | #29 | ~8 lines | Medium — cheap, very visible |
| 19 | AgX or ACES filmic curve instead of Reinhard | — | ~15 lines | Medium |
| 20 | Precomputed sky LUT (Hosek-Wilkie or Bruneton multi-scatter) + a CDF for sky importance sampling | #21 | ~150 lines | Medium |
| 21 | Gate accumulation on `enable_temporal` alone; make Reprojection purely a reuse-mode switch | #17 | ~5 lines | Medium |
| 22 | Split `resetAccumulation` — post-process settings (exposure, denoiser, tonemap) must not reset history | #33 | ~10 lines | Medium |
| 23 | Fix `is_moving` (OR the mouse and keyboard flags rather than overwriting) | #32 | 1 line | Low |
| 24 | Expose `initialCandidates`, `maxHistory`, `normalThreshold`, `depthThreshold` in the UI — these are the knobs that actually move the image | — | ~20 lines | Low |
| 25 | Canvas resize + `devicePixelRatio`: rebuild buffers, textures and bind groups on a `ResizeObserver` | #35 | ~40 lines | Medium |
| 26 | Rename the denoiser dropdown honestly, or implement what the labels claim | #25-27 | — | Low |

---

## Table 3 — Improvements: performance (raster-guided first)

| # | Change | Removes | Est. win |
|---|---|---|---|
| 1 | **Raster the G-buffer.** Replace the primary `intersect_scene` with a real render pipeline writing pos/normal/albedo/depth. | ~480k full BVH traversals/frame | **30-50% of total frame time** — the single biggest item |
| 2 | **Sun shadow map.** Raster a cascaded/single shadow map from `sun_dir()`; a texture fetch replaces `trace_occluded` everywhere except the penumbra band, where the BVH ray is still fired. | The most-fired ray in the shader (1 per bounce per candidate) | **20-35%** |
| 3 | **Hi-Z screen-space pre-march.** Build a depth pyramid from the raster pass; march GI/shadow rays in screen space first, fall back to BVH only on screen-exit or thickness ambiguity. | 60-80% of short-range rays | **15-30%** |
| 4 | **Cull rays by `dot(n, sun_dir)` before tracing** (`wgsl.ts:418`, `:532`). Currently the shadow ray is traced *then* multiplied by `max(0, dot)`. | ~50% of all shadow rays | **10-15%**, one `if` |
| 5 | **Drop `initialCandidates` 16 to 2-4.** Sixteen multi-bounce path traces per pixel per frame defeats the entire premise of ReSTIR; the reuse chain is supposed to supply the sample count. | 12-14 path traces/pixel/frame | **~3-5x** on the ReSTIR pass |
| 6 | **Indirect dispatch over a compacted pixel list** from the raster coverage mask — separate kernels for sky / metal / diffuse. | Whole-warp divergence between the reflection and ReSTIR branches | **10-20%** |
| 7 | **Ordered BVH traversal** (near child first via split axis + ray sign) with a small local stack, instead of pure miss-link DFS. | Far-subtree traversal that a closer hit would have culled | **15-25%** on trace cost |
| 8 | **Pack the G-buffer.** 336 B/pixel total (161 MB of traffic at 800x600). Octahedral normals (u32), RGB9E5 radiance, position reconstructed from depth, giving ~64-96 B/pixel. | ~70% of memory bandwidth | **High** — this is likely bandwidth-bound |
| 9 | **Denoisers read a slim normal+depth buffer (8 B)**, not the full 64 B `GBufferData`. A 5x5 bilateral currently re-reads 1.6 KB/pixel. | 8x denoiser tap bandwidth | **10-20%** of post cost |
| 10 | **Workgroup-shared tile cache** for the denoiser taps (`var<workgroup>`, load a 12x12 halo once). | Redundant global loads | **10-15%** |
| 11 | **Merge the 9 storage buffers to 8 or fewer** (suballocate prev/curr G-buffer from one buffer with a dynamic offset). | Portability failure on adapters with the default limit of 8 | Correctness on more hardware |
| 12 | **Store `prev_idx` in `depth_hist.zw`** in `pass_temporal`; `pass_resolve` currently recomputes the entire reprojection from scratch. | A duplicate full reprojection per pixel | 2-4% |
| 13 | **Hoist `sun_dir()` to a CPU-computed uniform** — it is 2 sin + 2 cos, recomputed per call, per bounce, per candidate. | ~50 transcendentals/pixel/frame | 2-5% |
| 14 | **Hoist `1.0/r.dir`** out of the BVH loop body (`wgsl.ts:168`, `:259`) — currently recomputed at every node. | 3 divides x node count | 3-6% |
| 15 | **Start Russian roulette at bounce 2 or later**, not bounce 0. | 5% of paths killed before contributing | 3-5% |
| 16 | **Cap sky-sample visibility rays** at scene-bounds distance, not `1e4` (`wgsl.ts:681`). | Absurdly long spatial-reuse shadow rays | 3-8% |
| 17 | **`dot(e,e) > 0` or a material flag** instead of `length(hit.emission) > 0.0` — that is a `sqrt` per bounce. | ~50 sqrt/pixel/frame | 1-2% |
| 18 | **Raster motion vectors** (prev-MVP per vertex) instead of world-position reprojection math, in both passes. | ~20 ALU x 2 per pixel; also handles dynamic objects | 2-4% plus correctness |
| 19 | **BVH build off the main thread**; compute subtree sizes during `flatten` instead of the recursive `countNodes` per node (`mesh.ts:370`). Also hoist the `new Uint32Array(...)` out of the pack loop (`:344`). | O(n^2) build hitch plus ~5100 array allocations | Startup only |
| 20 | **Merge `pass_resolve` and `pass_postprocess`** into one kernel; drop the redundant `accum_curr[].w` history copy. | One full-screen dispatch and one read/write round trip | 3-5% |
| 21 | **Dynamic resolution scaling** driven by a frame-time target, with the raster G-buffer at full res and ReSTIR at half. | — | 2-4x when needed |
| 22 | **RSM / probe grid from the raster pass** to terminate GI paths after 1 bounce instead of 2-5. | 50-70% of secondary bounces | **High**, larger effort |

---

## Recommended first five

1. Table 2 #1 — cosine at resolve
2. Table 2 #2 — mixture pdf
3. Table 2 #3 — unified sun radiance

   Three small edits that together fix an overbright, mis-weighted, 60x-inconsistent estimator.

4. Table 3 #1 — raster G-buffer
5. Table 3 #2 — sun shadow map

   This pair is the "use the raster to avoid firing rays" idea, and it is where the frame time is.
