export interface Material {
    albedo: number[];
    emission?: number[];
    roughness?: number;
    type?: number; // 0=diffuse, 1=emissive, 2=metal
}

export interface Tri {
    v0: number[];
    v1: number[];
    v2: number[];
    albedo: number[];
    emission: number[];
    roughness: number;
    type: number;
    centroid: number[];
}

export interface BVHNode {
    aabbMin: number[];
    aabbMax: number[];
    leftFirst: number;
    triCount: number;
}

export function buildScene(): { triBuffer: Float32Array, bvhBuffer: Float32Array } {
    const triangles: Tri[] = [];

    function pushTri(v0: number[], v1: number[], v2: number[], mat: Material) {
        triangles.push({
            v0, v1, v2,
            albedo: mat.albedo,
            emission: mat.emission || [0, 0, 0],
            roughness: mat.roughness !== undefined ? mat.roughness : 0.9,
            type: mat.type || 0,
            centroid: [
                (v0[0] + v1[0] + v2[0]) / 3,
                (v0[1] + v1[1] + v2[1]) / 3,
                (v0[2] + v1[2] + v2[2]) / 3
            ]
        });
    }

    function pushBox(min: number[], max: number[], mat: Material) {
        const v = [
            [min[0], min[1], min[2]], [max[0], min[1], min[2]], [max[0], max[1], min[2]], [min[0], max[1], min[2]],
            [min[0], min[1], max[2]], [max[0], min[1], max[2]], [max[0], max[1], max[2]], [min[0], max[1], max[2]]
        ];
        const indices = [
            0, 1, 2, 0, 2, 3, // back
            5, 4, 7, 5, 7, 6, // front
            4, 0, 3, 4, 3, 7, // left
            1, 5, 6, 1, 6, 2, // right
            4, 5, 1, 4, 1, 0, // bottom
            3, 2, 6, 3, 6, 7  // top
        ];
        for (let i = 0; i < indices.length; i += 3) {
            pushTri(v[indices[i]], v[indices[i + 1]], v[indices[i + 2]], mat);
        }
    }

    function pushSphere(c: number[], r: number, segments: number, mat: Material) {
        for (let i = 0; i < segments; i++) {
            for (let j = 0; j < segments; j++) {
                let theta1 = (i / segments) * Math.PI;
                let theta2 = ((i + 1) / segments) * Math.PI;
                let phi1 = (j / segments) * 2 * Math.PI;
                let phi2 = ((j + 1) / segments) * 2 * Math.PI;

                let p1 = [c[0] + r * Math.sin(theta1) * Math.cos(phi1), c[1] + r * Math.cos(theta1), c[2] + r * Math.sin(theta1) * Math.sin(phi1)];
                let p2 = [c[0] + r * Math.sin(theta2) * Math.cos(phi1), c[1] + r * Math.cos(theta2), c[2] + r * Math.sin(theta2) * Math.sin(phi1)];
                let p3 = [c[0] + r * Math.sin(theta2) * Math.cos(phi2), c[1] + r * Math.cos(theta2), c[2] + r * Math.sin(theta2) * Math.sin(phi2)];
                let p4 = [c[0] + r * Math.sin(theta1) * Math.cos(phi2), c[1] + r * Math.cos(theta1), c[2] + r * Math.sin(theta1) * Math.sin(phi2)];

                // Don't push degenerate triangles at poles
                if (i > 0) pushTri(p1, p2, p4, mat);
                if (i < segments - 1) pushTri(p2, p3, p4, mat);
            }
        }
    }

    function pushCone(c: number[], r: number, h: number, segments: number, mat: Material) {
        let top = [c[0], c[1] + h, c[2]];
        for (let i = 0; i < segments; i++) {
            let theta1 = (i / segments) * 2 * Math.PI;
            let theta2 = ((i + 1) / segments) * 2 * Math.PI;
            let p1 = [c[0] + r * Math.cos(theta1), c[1], c[2] + r * Math.sin(theta1)];
            let p2 = [c[0] + r * Math.cos(theta2), c[1], c[2] + r * Math.sin(theta2)];
            pushTri(p1, p2, top, mat);
            pushTri(p2, p1, c, mat); // base
        }
    }

    function pushTorus(c: number[], rMain: number, rTube: number, segMain: number, segTube: number, mat: Material) {
        for (let i = 0; i < segMain; i++) {
            for (let j = 0; j < segTube; j++) {
                let phi1 = (i / segMain) * 2 * Math.PI;
                let phi2 = ((i + 1) / segMain) * 2 * Math.PI;
                let theta1 = (j / segTube) * 2 * Math.PI;
                let theta2 = ((j + 1) / segTube) * 2 * Math.PI;

                let p1 = [
                    c[0] + (rMain + rTube * Math.cos(theta1)) * Math.cos(phi1),
                    c[1] + rTube * Math.sin(theta1),
                    c[2] + (rMain + rTube * Math.cos(theta1)) * Math.sin(phi1)
                ];
                let p2 = [
                    c[0] + (rMain + rTube * Math.cos(theta1)) * Math.cos(phi2),
                    c[1] + rTube * Math.sin(theta1),
                    c[2] + (rMain + rTube * Math.cos(theta1)) * Math.sin(phi2)
                ];
                let p3 = [
                    c[0] + (rMain + rTube * Math.cos(theta2)) * Math.cos(phi2),
                    c[1] + rTube * Math.sin(theta2),
                    c[2] + (rMain + rTube * Math.cos(theta2)) * Math.sin(phi2)
                ];
                let p4 = [
                    c[0] + (rMain + rTube * Math.cos(theta2)) * Math.cos(phi1),
                    c[1] + rTube * Math.sin(theta2),
                    c[2] + (rMain + rTube * Math.cos(theta2)) * Math.sin(phi1)
                ];

                pushTri(p1, p2, p4, mat);
                pushTri(p2, p3, p4, mat);
            }
        }
    }

    // --- Create Mesh Scene ---
    // Room
    pushBox([-35, 0, -30], [-33, 16, 30], { albedo: [0.8, 0.2, 0.2] }); // Red wall
    pushBox([33, 0, -30], [35, 16, 30], { albedo: [0.2, 0.8, 0.2] }); // Green wall
    pushBox([-33, 0, -32], [33, 16, -30], { albedo: [0.9, 0.9, 0.9] }); // Back wall
    pushBox([-35, 16, -32], [35, 17, 32], { albedo: [0.9, 0.9, 0.9] }); // Ceiling

    // Meshed Boxes
    pushBox([12, 0, -10], [16, 4, -6], { albedo: [0.1, 0.6, 0.8] });
    pushBox([-18, 0, 4], [-15, 6, 7], { albedo: [0.8, 0.5, 0.1] });
    
    // Meshed Mirror Pillar
    pushBox([-8, 0, 18], [-2, 8, 24], { albedo: [1.0, 1.0, 1.0], roughness: 0.0, type: 2 });
    
    // Meshed Spheres
    pushSphere([0, 3.5, 0], 2.5, 24, { albedo: [0, 0, 0], emission: [20, 40, 80], type: 1 }); // Glowing Core
    pushSphere([15, 3, 10], 3, 24, { albedo: [1.0, 0.85, 0.57], roughness: 0.1, type: 2 }); // Gold sphere
    pushSphere([-15, 4, -15], 4, 24, { albedo: [0.8, 0.5, 0.8], roughness: 0.8, type: 0 }); // Diffuse sphere
    pushSphere([-5, 2, -10], 2, 16, { albedo: [0.91, 0.92, 0.92], roughness: 0.2, type: 2 }); // Aluminium sphere

    // New Objects (Cones, Toruses, more boxes, all diffuse no reflection)
    pushCone([10, 0, -15], 3, 6, 16, { albedo: [0.9, 0.3, 0.3], type: 0 });
    pushCone([-20, 0, -5], 4, 8, 16, { albedo: [0.3, 0.9, 0.3], type: 0 });
    pushCone([15, 0, 18], 2.5, 5, 16, { albedo: [0.8, 0.5, 0.8], type: 0 });
    pushCone([-5, 0, 25], 3.5, 9, 16, { albedo: [0.4, 0.7, 0.9], type: 0 });

    pushTorus([-5, 1.5, 12], 4.0, 1.0, 16, 8, { albedo: [0.8, 0.2, 0.8], type: 0 });
    pushTorus([15, 3.0, 2], 3.0, 0.8, 16, 8, { albedo: [0.9, 0.9, 0.2], type: 0 });
    pushTorus([-22, 4.0, 8], 3.5, 1.2, 16, 8, { albedo: [0.2, 0.8, 0.9], type: 0 });
    pushTorus([8, 2.0, -22], 2.5, 0.6, 16, 8, { albedo: [0.9, 0.5, 0.2], type: 0 });

    pushBox([2, 0, 15], [6, 5, 19], { albedo: [0.7, 0.7, 0.2], type: 0 });
    pushBox([-22, 0, -18], [-16, 8, -14], { albedo: [0.3, 0.2, 0.8], type: 0 });
    pushBox([22, 0, -22], [28, 6, -18], { albedo: [0.2, 0.8, 0.4], type: 0 });
    pushBox([-28, 0, 20], [-20, 4, 25], { albedo: [0.8, 0.3, 0.6], type: 0 });
    pushBox([20, 0, 4], [28, 10, 12], { albedo: [0.5, 0.2, 0.8], type: 0 });

    // --- Build BVH (Binned SAH) ---
    class BVHNode {
        aabbMin = [Infinity, Infinity, Infinity];
        aabbMax = [-Infinity, -Infinity, -Infinity];
        leftFirst = 0;
        triCount = 0;
        leftChild: BVHNode | null = null;
        rightChild: BVHNode | null = null;
    }

    function updateBounds(node: BVHNode, first: number, count: number) {
        let min = [Infinity, Infinity, Infinity];
        let max = [-Infinity, -Infinity, -Infinity];
        for (let i = 0; i < count; i++) {
            let tri = triangles[first + i];
            for (let j = 0; j < 3; j++) {
                min[j] = Math.min(min[j], tri.v0[j], tri.v1[j], tri.v2[j]);
                max[j] = Math.max(max[j], tri.v0[j], tri.v1[j], tri.v2[j]);
            }
        }
        node.aabbMin = min;
        node.aabbMax = max;
    }

    function surfaceArea(min: number[], max: number[]) {
        let e = [Math.max(0, max[0] - min[0]), Math.max(0, max[1] - min[1]), Math.max(0, max[2] - min[2])];
        return 2.0 * (e[0] * e[1] + e[1] * e[2] + e[2] * e[0]);
    }

    function subdivide(node: BVHNode, first: number, count: number) {
        updateBounds(node, first, count);
        
        if (count <= 2) {
            node.leftFirst = first;
            node.triCount = count;
            return;
        }

        let bestCost = count * 1.0; 
        let bestAxis = -1;
        let bestPos = 0;
        
        const BINS = 8;
        for (let axis = 0; axis < 3; axis++) {
            let minC = Infinity, maxC = -Infinity;
            for (let i = 0; i < count; i++) {
                let c = triangles[first + i].centroid[axis];
                minC = Math.min(minC, c);
                maxC = Math.max(maxC, c);
            }
            if (minC === maxC) continue;
            
            let binCounts = new Int32Array(BINS);
            let binMin = Array.from({length: BINS}, () => [Infinity, Infinity, Infinity]);
            let binMax = Array.from({length: BINS}, () => [-Infinity, -Infinity, -Infinity]);
            
            for (let i = 0; i < count; i++) {
                let tri = triangles[first + i];
                let c = tri.centroid[axis];
                let binIdx = Math.floor(BINS * ((c - minC) / (maxC - minC)));
                binIdx = Math.min(BINS - 1, Math.max(0, binIdx));
                binCounts[binIdx]++;
                for(let j=0; j<3; j++) {
                    binMin[binIdx][j] = Math.min(binMin[binIdx][j], tri.v0[j], tri.v1[j], tri.v2[j]);
                    binMax[binIdx][j] = Math.max(binMax[binIdx][j], tri.v0[j], tri.v1[j], tri.v2[j]);
                }
            }
            
            for (let i = 0; i < BINS - 1; i++) {
                let leftCount = 0, rightCount = 0;
                let leftMin = [Infinity, Infinity, Infinity], leftMax = [-Infinity, -Infinity, -Infinity];
                let rightMin = [Infinity, Infinity, Infinity], rightMax = [-Infinity, -Infinity, -Infinity];
                
                for (let j = 0; j <= i; j++) {
                    leftCount += binCounts[j];
                    for(let k=0;k<3;k++) { leftMin[k] = Math.min(leftMin[k], binMin[j][k]); leftMax[k] = Math.max(leftMax[k], binMax[j][k]); }
                }
                for (let j = i + 1; j < BINS; j++) {
                    rightCount += binCounts[j];
                    for(let k=0;k<3;k++) { rightMin[k] = Math.min(rightMin[k], binMin[j][k]); rightMax[k] = Math.max(rightMax[k], binMax[j][k]); }
                }
                
                if (leftCount === 0 || rightCount === 0) continue;
                
                let rootArea = surfaceArea(node.aabbMin, node.aabbMax);
                let cost = 1.0 + (leftCount * surfaceArea(leftMin, leftMax) + rightCount * surfaceArea(rightMin, rightMax)) / rootArea;
                if (cost < bestCost) {
                    bestCost = cost;
                    bestAxis = axis;
                    bestPos = minC + (maxC - minC) * (i + 1) / BINS;
                }
            }
        }
        
        if (bestAxis === -1) {
            if (count > 15) {
                // forced split for count overflow
                let leftCount = Math.floor(count / 2);
                node.leftChild = new BVHNode();
                node.rightChild = new BVHNode();
                subdivide(node.leftChild, first, leftCount);
                subdivide(node.rightChild, first + leftCount, count - leftCount);
                return;
            }
            node.leftFirst = first;
            node.triCount = count;
            return;
        }
        
        // Partition
        let i = first;
        let j = first + count - 1;
        while (i <= j) {
            if (triangles[i].centroid[bestAxis] < bestPos) {
                i++;
            } else {
                let temp = triangles[i];
                triangles[i] = triangles[j];
                triangles[j] = temp;
                j--;
            }
        }
        
        let leftCount = i - first;
        if (leftCount === 0 || leftCount === count) {
            if (count > 15) {
                leftCount = Math.floor(count / 2);
                node.leftChild = new BVHNode();
                node.rightChild = new BVHNode();
                subdivide(node.leftChild, first, leftCount);
                subdivide(node.rightChild, first + leftCount, count - leftCount);
                return;
            }
            node.leftFirst = first;
            node.triCount = count;
            return;
        }
        
        node.leftChild = new BVHNode();
        node.rightChild = new BVHNode();
        subdivide(node.leftChild, first, leftCount);
        subdivide(node.rightChild, i, count - leftCount);
    }

    let root = new BVHNode();
    subdivide(root, 0, triangles.length);

    let flatNodes: BVHNode[] = [];
    function flatten(node: BVHNode) {
        flatNodes.push(node);
        if (node.triCount === 0) {
            flatten(node.leftChild!);
            flatten(node.rightChild!);
        }
    }
    flatten(root);

    // --- Pack Buffers ---
    // Triangle Buffer (96 bytes = 24 floats per tri)
    const triBuffer = new Float32Array(triangles.length * 24);
    for (let i = 0; i < triangles.length; i++) {
        let t = triangles[i];
        let off = i * 24;
        triBuffer.set(t.v0, off);
        triBuffer.set(t.v1, off + 4);
        triBuffer.set(t.v2, off + 8);
        
        // Face normal
        let e1 = [t.v1[0] - t.v0[0], t.v1[1] - t.v0[1], t.v1[2] - t.v0[2]];
        let e2 = [t.v2[0] - t.v0[0], t.v2[1] - t.v0[1], t.v2[2] - t.v0[2]];
        let n = [e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]];
        let l = Math.hypot(n[0], n[1], n[2]);
        if (l > 0) { n[0] /= l; n[1] /= l; n[2] /= l; }
        
        triBuffer.set(n, off + 12);
        triBuffer.set(t.albedo, off + 16);
        triBuffer[off + 19] = t.roughness;
        triBuffer.set(t.emission, off + 20);
        new Uint32Array(triBuffer.buffer, triBuffer.byteOffset)[off + 23] = t.type;
    }

    // BVH Buffer (32 bytes = 8 floats per node)
    const bvhBuffer = new Float32Array(flatNodes.length * 8);
    const bvhU32 = new Uint32Array(bvhBuffer.buffer);
    
    function countNodes(node: BVHNode): number {
        if (node.triCount > 0) return 1;
        return 1 + countNodes(node.leftChild!) + countNodes(node.rightChild!);
    }

    for (let i = 0; i < flatNodes.length; i++) {
        let n = flatNodes[i];
        let off = i * 8;
        let padding = 0.001;
        
        bvhBuffer[off + 0] = n.aabbMin[0] - padding;
        bvhBuffer[off + 1] = n.aabbMin[1] - padding;
        bvhBuffer[off + 2] = n.aabbMin[2] - padding;
        bvhU32[off + 3] = n.triCount > 0 ? n.leftFirst : (i + 1);
        
        bvhBuffer[off + 4] = n.aabbMax[0] + padding;
        bvhBuffer[off + 5] = n.aabbMax[1] + padding;
        bvhBuffer[off + 6] = n.aabbMax[2] + padding;
        
        let subtreeSize = countNodes(n);
        let missLink = i + subtreeSize;
        if (missLink >= flatNodes.length) missLink = 0x00FFFFFF;
        
        let count = Math.min(255, n.triCount);
        bvhU32[off + 7] = (count << 24) | (missLink & 0x00FFFFFF);
    }

    return { triBuffer, bvhBuffer };
}
