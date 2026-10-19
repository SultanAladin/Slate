// Topology proof: run `node test-topology.mjs`
import { Project, buildNetwork, validateNetwork, exportOBJ } from './js/core.js';
import { buildDemoCity } from './js/demo.js';
import assert from 'node:assert';

// 1) Full demo city: 100% quads, nothing degenerate / non-manifold / NaN.
const p = buildDemoCity(); const b = buildNetwork(p); const v = validateNetwork(p, b);
assert.equal(v.quadPct, '100.0'); assert.equal(v.tris, 0); assert.equal(v.ngons, 0);
assert.equal(v.degenerate, 0); assert.equal(v.nonManifold, 0); assert.deepEqual(v.issues, []);
assert.ok(b.mesh.pos.every(Number.isFinite), 'NaN vertex');
assert.ok(v.junctions >= 40, 'expected many merged junctions');

// 2) Crossing roads split BOTH ways (never a "+" drawn over a "|"):
//    a vertex landing exactly on a road, and a normal crossing, both junction.
const q = new Project();
q.addRoad([[-200, 0], [200, 0]], 'arterial');
q.addRoad([[0, -150], [10, 0], [60, 140]], 'collector');
q.addRoad([[-150, 80], [150, 90]], 'residential');
assert.equal([...q.nodes.values()].filter(n => q.degree(n.id) >= 3).length, 2);
const vq = validateNetwork(q, buildNetwork(q));
assert.equal(vq.degenerate, 0); assert.deepEqual(vq.issues, []);

// 3) OBJ export is pure quads.
const obj = exportOBJ(b); const faces = obj.match(/^f .*$/gm);
assert.ok(faces.length > 20000 && faces.every(f => f.split(' ').length === 5), 'OBJ must contain only 4-vertex faces');
console.log(`PASS  demo: ${v.quads} quads, ${v.junctions} merged junctions, ${b.buildings.length} buildings, ${b.trees.length} trees, ${b.mesh.boxes.length} bridge piers`);
