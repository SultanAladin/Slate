const fs = require('fs');
let code = fs.readFileSync('src/webgpu/wgsl.ts', 'utf8');

const replacement = `
struct Sphere { center: vec3<f32>, radius: f32, mat: Material }
struct Box { bmin: vec3<f32>, bmax: vec3<f32>, mat: Material }

fn intersect_scene(r: Ray, hit: ptr<function, Hit>) -> bool {
    (*hit).t = 1e20;
    var found = false;

    // 1. Analytic Ground Plane
    let t_p = -r.origin.y / r.dir.y;
    if (t_p > 0.001 && t_p < (*hit).t) {
        (*hit).t = t_p;
        (*hit).n = vec3(0.0, 1.0, 0.0);
        
        let px = floor(r.origin.x + r.dir.x * t_p);
        let pz = floor(r.origin.z + r.dir.z * t_p);
        let check = abs((px + pz) % 2.0);
        if (check < 0.5) {
            (*hit).albedo = vec3(0.6, 0.6, 0.6);
        } else {
            (*hit).albedo = vec3(0.4, 0.4, 0.4);
        }
        (*hit).emission = vec3(0.0);
        (*hit).mat_type = 0u;
        found = true;
    }

    // 2. Analytic Spheres
    let spheres = array<Sphere, 9>(
        Sphere(vec3(500.0, 200.0, 800.0), 30.0, Material(vec3(0.), vec3(200.0, 180.0, 140.0), 1., 1u)),
        Sphere(vec3(0.0, 3.0, 0.0), 1.5, Material(vec3(0.), vec3(20.0, 40.0, 80.0), 1., 1u)), // glowing core
        Sphere(vec3(-5.0, 5.0, -10.0), 3.0, Material(vec3(0.), vec3(80.0, 10.0, 10.0), 1., 1u)),

        Sphere(vec3(-4.0, 1.5, -4.0), 1.5, Material(vec3(0.9, 0.1, 0.1), vec3(0.), 0.1, 0u)),
        Sphere(vec3(4.0, 1.5, -4.0), 1.5, Material(vec3(0.1, 0.9, 0.1), vec3(0.), 0.3, 0u)),
        Sphere(vec3(-4.0, 1.5, 4.0), 1.5, Material(vec3(0.1, 0.1, 0.9), vec3(0.), 0.5, 0u)),
        Sphere(vec3(4.0, 1.5, 4.0), 1.5, Material(vec3(0.9, 0.9, 0.1), vec3(0.), 0.7, 0u)),
        
        Sphere(vec3(-8.0, 2.5, 0.0), 2.5, Material(vec3(0.8, 0.8, 0.8), vec3(0.), 0.9, 0u)),
        Sphere(vec3(8.0, 2.5, 0.0), 2.5, Material(vec3(0.2, 0.2, 0.2), vec3(0.), 0.0, 0u))
    );

    for (var i = 0u; i < 9u; i = i + 1u) {
        let oc = r.origin - spheres[i].center;
        let b = dot(oc, r.dir);
        let c = dot(oc, oc) - spheres[i].radius * spheres[i].radius;
        let d = b * b - c;
        if (d > 0.0) {
            let t = -b - sqrt(d);
            if (t > 0.001 && t < (*hit).t) {
                (*hit).t = t;
                (*hit).n = normalize((r.origin + r.dir * t) - spheres[i].center);
                (*hit).albedo = spheres[i].mat.albedo;
                (*hit).emission = spheres[i].mat.emission;
                (*hit).mat_type = spheres[i].mat.type_;
                found = true;
            }
        }
    }

    // 3. Analytic Boxes (AABBs) - explicit walls & blocks
    let boxes = array<Box, 4>(
        Box(vec3(-15.0, 0.0, -10.0), vec3(-13.0, 8.0, 10.0), Material(vec3(0.8, 0.2, 0.2), vec3(0.), 0.1, 0u)), // Red Wall
        Box(vec3(13.0, 0.0, -10.0), vec3(15.0, 8.0, 10.0), Material(vec3(0.2, 0.8, 0.2), vec3(0.), 0.1, 0u)), // Green Wall
        Box(vec3(-13.0, 0.0, -12.0), vec3(13.0, 8.0, -10.0), Material(vec3(0.9, 0.9, 0.9), vec3(0.), 0.1, 0u)), // Back Wall
        
        Box(vec3(-1.0, 0.0, -1.0), vec3(1.0, 1.5, 1.0), Material(vec3(0.4, 0.4, 0.4), vec3(0.), 0.8, 0u)) // Pedestal
    );

    for (var i = 0u; i < 4u; i = i + 1u) {
        let invDir = 1.0 / r.dir;
        let t0 = (boxes[i].bmin - r.origin) * invDir;
        let t1 = (boxes[i].bmax - r.origin) * invDir;
        let tmin = min(t0, t1);
        let tmax = max(t0, t1);
        let tnear = max(max(tmin.x, tmin.y), tmin.z);
        let tfar = min(min(tmax.x, tmax.y), tmax.z);
        
        if (tnear < tfar && tfar > 0.001 && tnear < (*hit).t) {
            let t = select(tfar, tnear, tnear > 0.001);
            if (t > 0.001 && t < (*hit).t) {
                (*hit).t = t;
                let p = r.origin + r.dir * t;
                
                let center = (boxes[i].bmax + boxes[i].bmin) * 0.5;
                let extents = (boxes[i].bmax - boxes[i].bmin) * 0.5;
                let local_p = (p - center) / extents;
                let abs_p = abs(local_p);
                
                var n = vec3(0.0);
                if (abs_p.x > abs_p.y && abs_p.x > abs_p.z) { n.x = sign(local_p.x); }
                else if (abs_p.y > abs_p.z) { n.y = sign(local_p.y); }
                else { n.z = sign(local_p.z); }
                
                (*hit).n = n;
                (*hit).albedo = boxes[i].mat.albedo;
                (*hit).emission = boxes[i].mat.emission;
                (*hit).mat_type = boxes[i].mat.type_;
                found = true;
            }
        }
    }

    return found;
}
`;

const startIndex = code.indexOf('struct Sphere { center: vec3<f32>, radius: f32, mat: Material }');
const endIndex = code.indexOf('fn trace_occluded');

if (startIndex !== -1 && endIndex !== -1) {
    code = code.substring(0, startIndex) + replacement + '\n' + code.substring(endIndex);
    fs.writeFileSync('src/webgpu/wgsl.ts', code);
    console.log('patched successfully');
} else {
    console.log('failed to find indices');
}
