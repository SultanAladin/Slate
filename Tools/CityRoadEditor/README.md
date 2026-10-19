# Slate City Road Editor

A robust 2D road-graph editor with a live 3D editor view, built for game
(AAA-style) city networks. Open `index.html` via any static server — no build step:

```sh
python3 -m http.server 8901   # then open http://localhost:8901
node test-topology.mjs        # topology proof for the mesh generator
```

## Topology contract (the core of the tool)

1. **Planar graph, always.** Roads are nodes + edges. Drawing a road across
   another splits *both* and inserts a shared node — a crossing is never a
   `+` drawn over a `|`, a T/Y is never a `V` drawn across a through-road,
   and ending a road on another splits it. A vertex landing exactly on a
   road, or an existing node lying on a new segment, is handled too.
2. **Merged junctions.** Every node of degree ≥ 3 becomes one filleted
   patch (quadratic-Bézier corners between adjacent arms' offset lines;
   acute Y arms get proportionally longer trims, reconciled when two
   junctions share a short link). Arms are trimmed to the junction
   boundary at the exact boundary elevation, so segments and the patch
   meet with no overlap, no gap and no height step.
3. **100 % quads.** Segments are cross-section quad strips (pavement,
   kerb, asphalt, median, shoulder). The junction hub is outer→inner ring
   quads plus the inner n-gon split into n quads (centroid + edge
   midpoints), so 3-way Y, 4-way and 5-way junctions are all-quad.
   The builder welds vertices, rejects degenerate faces, and the live
   validator reports quads / tris / n-gons / degenerate / non-manifold
   counts on every rebuild. OBJ export keeps faces as 4-vertex `f` lines.

## What's in the scene

- **Preset library:** motorway, highway, arterial, boulevard, collector,
  residential, small street, one-way avenue, country road, offroad dirt
  track, alley, bike street — each with lanes, median, bike lane, parking,
  pavements, shoulders and its own marking rules; any selected road can be
  re-typed and re-laned live from the inspector.
- **Full marking set as geometry:** zebra crossings + stop lines + give-way
  arrows on every junction arm, double-yellow / dashed centre lines, lane
  dashes, edge lines, green bike lanes with bicycle decals, parking-bay
  ticks, direction arrows, bus-stop markings, median-nose chevrons, and
  raised, yellow-striped speed bumps on residential/bike streets.
- **Pavements** with kerbs and sloped (ramped) junction corners at crossings.
- **Terrain** heightfield with raise/lower/flatten brushes; roads re-grade
  (max 9 %), follow the smoothed profile, and **bridge automatically**
  where they cross water (raised deck, parapets, piers) — river and lake
  included. Bridges also form after you carve new water with the brushes.
- **Rail** lines with ballast, sleepers and steel rails cutting across
  roads on the level and bridging the river; elevated **monorail** beam
  on pylons. Buildings are clean window-textured blocks, trees low-poly.
- **Demo project** (loads automatically): organic ring + radial network —
  countryside country roads and farm tracks, suburbs, downtown avenues,
  offroad lakeside/hill trails — *not* a lazy grid — with a rail main line
  and a monorail loop.

## Files

| File | Role |
|---|---|
| `js/core.js` | Terrain, planar graph (splitting/planarization), quad mesh builder, bridges, validator, OBJ export |
| `js/presets.js` | Road-type preset library |
| `js/demo.js` | The full countryside→city→offroad project |
| `js/app.js` | 2D canvas editor + Three.js 3D view, inspector, history |
| `vendor/` | Three.js r160 (vendored, works offline) |
