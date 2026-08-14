export const computeWGSL = `
// PRNG
fn hash(seed_val: u32) -> u32 {
    var x = seed_val;
    x = x ^ (x >> 16u);
    x = x * 0x7feb352du;
    x = x ^ (x >> 15u);
    x = x * 0x846ca68bu;
    x = x ^ (x >> 16u);
    return x;
}

fn randf(seed: ptr<function, u32>) -> f32 {
    *seed = hash(*seed);
    return f32(*seed) / 4294967295.0;
}

struct Uniforms {
    resolution: vec2<f32>,
    frame_count: u32,
    temporal_weight: f32,
    spatial_radius: f32,
    spatial_samples: u32,
    enable_temporal: u32,
    enable_spatial: u32,
    enable_denoise: u32,
    moving_spp: u32,
    is_moving: u32,
    enable_reprojection: u32,
    cam_pos: vec4<f32>,
    cam_dir: vec4<f32>,
    cam_up: vec4<f32>,
    cam_right: vec4<f32>,
    prev_cam_pos: vec4<f32>,
    prev_cam_dir: vec4<f32>,
    prev_cam_up: vec4<f32>,
    prev_cam_right: vec4<f32>,
    sunAzimuth: f32,
    sunElevation: f32,
    sunIntensity: f32,
    turbidity: f32,
    skyIntensity: f32,
    sunAngularRadius: f32,
    maxHistory: f32,
    initialCandidates: u32,
    normalThreshold: f32,
    depthThreshold: f32,
    exposure: f32,
    denoiserType: u32,
    enable_reflections: u32,
    reflection_bounces: u32,
    reflection_gi: u32,
    diffuse_bounces: u32,
}

struct GBufferData {
    pos_t: vec4<f32>,
    norm_mat: vec4<f32>,
    albedo_prev: vec4<f32>,
    depth_hist: vec4<f32>, // x = depth, y = histLen, z = pad, w = pad
}

struct Reservoir {
    xs_wSum: vec4<f32>,
    ns_M: vec4<f32>,
    Lo_W: vec4<f32>,
}

@group(0) @binding(0) var<uniform> u: Uniforms;
@group(0) @binding(1) var tex_out: texture_storage_2d<rgba16float, write>;
@group(0) @binding(2) var<storage, read_write> accum_prev: array<vec4<f32>>;
@group(0) @binding(3) var<storage, read_write> accum_curr: array<vec4<f32>>;
@group(0) @binding(4) var<storage, read_write> gbuffer_prev: array<GBufferData>;
@group(0) @binding(5) var<storage, read_write> gbuffer_curr: array<GBufferData>;
@group(0) @binding(6) var<storage, read_write> res_prev: array<Reservoir>;
@group(0) @binding(7) var<storage, read_write> res_curr: array<Reservoir>;
@group(0) @binding(8) var<storage, read_write> res_spatial: array<Reservoir>;

struct Ray { origin: vec3<f32>, dir: vec3<f32> }
struct Hit { t: f32, n: vec3<f32>, albedo: vec3<f32>, emission: vec3<f32>, mat_type: u32, roughness: f32 }

struct Triangle {
    v0: vec3<f32>,
    pad0: f32,
    v1: vec3<f32>,
    pad1: f32,
    v2: vec3<f32>,
    pad2: f32,
    n: vec3<f32>,
    pad3: f32,
    albedo: vec3<f32>,
    roughness: f32,
    emission: vec3<f32>,
    mat_type: u32,
}

struct BVHNode {
    aabb_min: vec3<f32>,
    left_first: u32,
    aabb_max: vec3<f32>,
    count_and_miss: u32,
}

@group(0) @binding(9) var<storage, read> triangles: array<Triangle>;
@group(0) @binding(10) var<storage, read> bvh: array<BVHNode>;

fn intersect_triangle_fast(r: Ray, tri: Triangle, max_t: f32) -> f32 {
    let edge1 = tri.v1 - tri.v0;
    let edge2 = tri.v2 - tri.v0;
    let h = cross(r.dir, edge2);
    let a = dot(edge1, h);
    if (a > -0.00001 && a < 0.00001) { return 1e30; }
    
    let f = 1.0 / a;
    let s = r.origin - tri.v0;
    let u_coord = f * dot(s, h);
    if (u_coord < 0.0 || u_coord > 1.0) { return 1e30; }
    
    let q = cross(s, edge1);
    let v_coord = f * dot(r.dir, q);
    if (v_coord < 0.0 || u_coord + v_coord > 1.0) { return 1e30; }
    
    let t = f * dot(edge2, q);
    if (t > 0.001 && t < max_t) {
        return t;
    }
    return 1e30;
}

fn sun_dir() -> vec3<f32> {
    let ce = cos(u.sunElevation);
    return normalize(vec3(ce * sin(u.sunAzimuth), sin(u.sunElevation), ce * cos(u.sunAzimuth)));
}

fn intersect_scene(r: Ray, hit: ptr<function, Hit>) -> bool {
    var best_t = 1e30;
    var best_tri = 0xFFFFFFFFu;
    var sun_found = false;
    var ground_found = false;

    // Analytic Ground Plane
    let t_p = -r.origin.y / r.dir.y;
    if (t_p > 0.001 && t_p < best_t) {
        best_t = t_p;
        ground_found = true;
    }

    // Direct sun check
    let s_dir = sun_dir();
    let s_t = 1e4;
    let s_rad = 500.0;
    let oc_s = r.origin - (s_dir * s_t);
    let b_s = dot(oc_s, r.dir);
    let c_s = dot(oc_s, oc_s) - s_rad * s_rad;
    let h_s = b_s * b_s - c_s;
    if (h_s > 0.0) {
        let t = -b_s - sqrt(h_s);
        if (t > 0.001 && t < best_t) {
            best_t = t;
            sun_found = true;
            ground_found = false;
        }
    }

    var node_idx = 0u;
    while (node_idx != 0x00FFFFFFu) {
        let node = bvh[node_idx];
        let invD = 1.0 / r.dir;
        let t0 = (node.aabb_min - r.origin) * invD;
        let t1 = (node.aabb_max - r.origin) * invD;
        
        let tsmall = min(t0, t1);
        let tbig = max(t0, t1);
        let tboxmin = max(max(tsmall.x, tsmall.y), max(tsmall.z, 0.001));
        let tboxmax = min(min(tbig.x, tbig.y), min(tbig.z, best_t));
        
        if (tboxmin <= tboxmax) {
            let tri_count = node.count_and_miss >> 24u;
            if (tri_count > 0u) {
                // Leaf Node
                for (var i = 0u; i < tri_count; i++) {
                    let tri_idx = node.left_first + i;
                    let t = intersect_triangle_fast(r, triangles[tri_idx], best_t);
                    if (t < best_t) {
                        best_t = t;
                        best_tri = tri_idx;
                        sun_found = false;
                        ground_found = false;
                    }
                }
                node_idx = node.count_and_miss & 0x00FFFFFFu; // Jump to miss link (next in DFS)
            } else {
                // Internal Node
                node_idx = node.left_first; // Left child is strictly the next node in our flattened array, but let's use left_first
            }
        } else {
            // Missed box
            node_idx = node.count_and_miss & 0x00FFFFFFu; // Jump to miss link
        }
    }
    
    if (sun_found) {
        (*hit).t = best_t;
        (*hit).n = normalize(r.origin + r.dir * best_t - (s_dir * s_t));
        (*hit).albedo = vec3(0.);
        (*hit).emission = vec3(u.sunIntensity);
        (*hit).mat_type = 1u;
        (*hit).roughness = 0.0;
        return true;
    }

    if (ground_found) {
        (*hit).t = best_t;
        (*hit).n = vec3(0.0, 1.0, 0.0);
        let px = floor(r.origin.x + r.dir.x * best_t);
        let pz = floor(r.origin.z + r.dir.z * best_t);
        let check = abs((px + pz) % 2.0);
        if (check < 0.5) {
            (*hit).albedo = vec3(0.9, 0.9, 0.9);
        } else {
            (*hit).albedo = vec3(0.6, 0.6, 0.6);
        }
        (*hit).emission = vec3(0.0);
        (*hit).mat_type = 0u;
        (*hit).roughness = 0.8;
        return true;
    }
    
    if (best_tri != 0xFFFFFFFFu) {
        let tri = triangles[best_tri];
        (*hit).t = best_t;
        var n = tri.n;
        if (dot(n, r.dir) > 0.0) {
            n = -n;
        }
        (*hit).n = n;
        (*hit).albedo = tri.albedo;
        (*hit).emission = tri.emission;
        (*hit).mat_type = tri.mat_type;
        (*hit).roughness = tri.roughness;
        return true;
    }
    
    return false;
}

fn trace_occluded(origin: vec3<f32>, dir: vec3<f32>, max_dist: f32) -> bool {
    let t_p = -origin.y / dir.y;
    if (t_p > 0.001 && t_p < max_dist) {
        return true;
    }

    let r = Ray(origin, dir);
    var node_idx = 0u;
    
    while (node_idx != 0x00FFFFFFu) {
        let node = bvh[node_idx];
        
        let invD = 1.0 / r.dir;
        let t0 = (node.aabb_min - r.origin) * invD;
        let t1 = (node.aabb_max - r.origin) * invD;
        let tsmall = min(t0, t1);
        let tbig = max(t0, t1);
        let tboxmin = max(max(tsmall.x, tsmall.y), max(tsmall.z, 0.001));
        let tboxmax = min(min(tbig.x, tbig.y), min(tbig.z, max_dist));
        
        if (tboxmin <= tboxmax) {
            let tri_count = node.count_and_miss >> 24u;
            if (tri_count > 0u) {
                for (var i = 0u; i < tri_count; i++) {
                    let t = intersect_triangle_fast(r, triangles[node.left_first + i], max_dist);
                    if (t < max_dist) {
                        return true;
                    }
                }
                node_idx = node.count_and_miss & 0x00FFFFFFu;
            } else {
                node_idx = node.left_first;
            }
        } else {
            node_idx = node.count_and_miss & 0x00FFFFFFu;
        }
    }
    return false;
}

fn cone_sample(rng: ptr<function, u32>, max_angle: f32) -> vec3<f32> {
    let r1 = randf(rng);
    let r2 = randf(rng);
    let z = cos(max_angle) + (1.0 - cos(max_angle)) * r1;
    let phi = 2.0 * 3.14159265 * r2;
    let r = sqrt(1.0 - z * z);
    return vec3(r * cos(phi), r * sin(phi), z);
}

fn sky_radiance(dir: vec3<f32>, includeSunDisk: bool) -> vec3<f32> {
    let sd = sun_dir();
    let up = max(dir.y, 0.0);
    let cosT = clamp(dot(dir, sd), -1.0, 1.0);

    let rayleigh = 3.0 / (16.0 * 3.14159265) * (1.0 + cosT * cosT);
    let g = 0.76;
    let mie = (1.0 - g * g) / (4.0 * 3.14159265 * pow(1.0 + g * g - 2.0 * g * cosT, 1.5));

    let betaR = vec3(5.8e-6, 13.5e-6, 33.1e-6) * 4.0e5;
    let betaM = vec3(21e-6) * 4.0e5 * (u.turbidity * 0.2);

    let airmass = 1.0 / (up + 0.15 * pow(max(0.0, 93.885 - degrees(acos(up))), -1.253));
    let sunAM  = 1.0 / (max(sd.y,0.0) + 0.15 * pow(max(0.0, 93.885 - degrees(acos(clamp(sd.y,0.0,1.0)))), -1.253) + 1e-3);

    let extinct = exp(-(betaR + betaM) * sunAM * 0.35);
    var col = (betaR * rayleigh + betaM * mie) * airmass * extinct * u.sunIntensity;

    col = mix(vec3(0.12, 0.11, 0.10) * u.sunIntensity * 0.05, col, smoothstep(-0.05, 0.05, dir.y));

    if (includeSunDisk && cosT > cos(u.sunAngularRadius)) {
        col += vec3(1.0, 0.96, 0.9) * u.sunIntensity * 60.0;
    }
    return col * u.skyIntensity;
}

fn luma(c: vec3<f32>) -> f32 { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

fn target_pdf(r: Reservoir, gb: GBufferData) -> f32 {
    let d  = r.xs_wSum.xyz - gb.pos_t.xyz;
    let l2 = max(dot(d, d), 1e-6);
    let wi = d * inverseSqrt(l2);
    let cv = dot(gb.norm_mat.xyz, wi);
    if (cv <= 0.0) { return 0.0; }
    return luma(r.Lo_W.xyz) * cv;
}

fn shift_jacobian(xNew: vec3<f32>, xOld: vec3<f32>, r: Reservoir) -> f32 {
    let dn = r.xs_wSum.xyz - xNew;  let ln2 = max(dot(dn, dn), 1e-6);
    let do_ = r.xs_wSum.xyz - xOld; let lo2 = max(dot(do_, do_), 1e-6);
    let cn = abs(dot(r.ns_M.xyz, -dn * inverseSqrt(ln2)));
    let co = abs(dot(r.ns_M.xyz, -do_ * inverseSqrt(lo2)));
    return clamp((cn * lo2) / (max(co, 1e-6) * ln2), 0.0, 8.0);
}

fn update_reservoir(r: ptr<function, Reservoir>, s: Reservoir, w: f32, c: f32, rnd: f32) -> bool {
    (*r).xs_wSum.w += w;
    (*r).ns_M.w += c;
    if (w > 0.0 && rnd * (*r).xs_wSum.w < w) {
        (*r).xs_wSum = vec4(s.xs_wSum.xyz, (*r).xs_wSum.w);
        (*r).ns_M = vec4(s.ns_M.xyz, (*r).ns_M.w);
        (*r).Lo_W = vec4(s.Lo_W.xyz, (*r).Lo_W.w);
        return true;
    }
    return false;
}

fn generate_candidate(gb: GBufferData, rng: ptr<function, u32>, cand: ptr<function, Reservoir>, pdf: ptr<function, f32>) {
    let r1 = randf(rng);
    let r2 = randf(rng);
    
    // Sometimes sample sun, sometimes cosine hemisphere
    if (randf(rng) < 0.3) {
        let sd = sun_dir();
        let a = u.sunAngularRadius;
        
        // orthonormal basis around sd
        let t1 = normalize(cross(sd, vec3(0.0, 1.0, 0.0)));
        let t1_valid = select(t1, normalize(cross(sd, vec3(1.0, 0.0, 0.0))), abs(sd.y) > 0.99);
        let t2 = cross(sd, t1_valid);
        
        let c_samp = cone_sample(rng, a);
        let d = normalize(t1_valid * c_samp.x + t2 * c_samp.y + sd * c_samp.z);
        
        if (dot(d, gb.norm_mat.xyz) > 0.0 && !trace_occluded(gb.pos_t.xyz + gb.norm_mat.xyz*0.001, d, 1e4)) {
            (*cand).xs_wSum = vec4(gb.pos_t.xyz + d * 1e4, 0.0);
            (*cand).ns_M = vec4(-d, 0.0);
            (*cand).Lo_W = vec4(sky_radiance(d, true), 0.0);
            *pdf = 1.0 / (2.0 * 3.14159265 * (1.0 - cos(a)));
            return;
        }
    }
    
    let phi = 2.0 * 3.14159265 * r1;
    let cos_theta = sqrt(1.0 - r2);
    let sin_theta = sqrt(r2);
    let t1 = normalize(cross(gb.norm_mat.xyz, vec3(0.0, 1.0, 0.0)));
    let t1_valid = select(t1, normalize(cross(gb.norm_mat.xyz, vec3(1.0, 0.0, 0.0))), abs(gb.norm_mat.y) > 0.99);
    let t2 = cross(gb.norm_mat.xyz, t1_valid);
    let d = normalize(t1_valid * (sin_theta * cos(phi)) + t2 * (sin_theta * sin(phi)) + gb.norm_mat.xyz * cos_theta);
    
    *pdf = cos_theta / 3.14159265;
    if (*pdf <= 0.0) { return; }
    
    var hit: Hit;
    var current_ray = Ray(gb.pos_t.xyz + gb.norm_mat.xyz * 0.001, d);
    var throughput = vec3(1.0);
    var radiance = vec3(0.0);
    var hit_dist = 1000.0;
    var final_n = -d;
    var valid_hit = false;

    var b: u32 = 0u;
    loop {
        if (b >= u.diffuse_bounces) {
            break;
        }
        
        if (intersect_scene(current_ray, &hit)) {
            if (b == 0u) {
                hit_dist = hit.t;
                final_n = hit.n;
                valid_hit = true;
            }
            
            if (length(hit.emission) > 0.0) {
                radiance += throughput * hit.emission;
                break;
            } else {
                let next_origin = current_ray.origin + current_ray.dir * hit.t + hit.n * 0.001;
                let sd = sun_dir();
                let shadow_ray = Ray(next_origin, sd);
                if (!trace_occluded(shadow_ray.origin, shadow_ray.dir, 1e4)) {
                    radiance += throughput * hit.albedo * max(0.0, dot(hit.n, sd)) * u.sunIntensity;
                }
                radiance += throughput * hit.albedo * sky_radiance(hit.n, false) * 0.1;
                
                let r1_b = randf(rng);
                let r2_b = randf(rng);
                let phi_b = 2.0 * 3.14159265 * r1_b;
                let cos_theta_b = sqrt(1.0 - r2_b);
                let sin_theta_b = sqrt(r2_b);
                let t1_b = normalize(cross(hit.n, vec3(0.0, 1.0, 0.0)));
                let t1_valid_b = select(t1_b, normalize(cross(hit.n, vec3(1.0, 0.0, 0.0))), abs(hit.n.y) > 0.99);
                let t2_b = cross(hit.n, t1_valid_b);
                let next_dir = normalize(t1_valid_b * (sin_theta_b * cos(phi_b)) + t2_b * (sin_theta_b * sin(phi_b)) + hit.n * cos_theta_b);
                
                throughput = throughput * hit.albedo;
                
                // Russian Roulette (Efficiency Upgrade)
                let p = max(throughput.r, max(throughput.g, throughput.b));
                let rr_prob = clamp(p, 0.05, 0.95);
                if (randf(rng) > rr_prob) {
                    break;
                }
                throughput = throughput / rr_prob;
                
                current_ray = Ray(next_origin, next_dir);
            }
        } else {
            radiance += throughput * sky_radiance(current_ray.dir, true);
            break;
        }
        b = b + 1u;
    }
    
    if (valid_hit) {
        (*cand).xs_wSum = vec4(gb.pos_t.xyz + gb.norm_mat.xyz * 0.001 + d * hit_dist, 0.0);
        (*cand).ns_M = vec4(final_n, 0.0);
        (*cand).Lo_W = vec4(radiance, 0.0);
    } else {
        (*cand).xs_wSum = vec4(gb.pos_t.xyz + d * 1000.0, 0.0);
        (*cand).ns_M = vec4(-d, 0.0);
        (*cand).Lo_W = vec4(radiance, 0.0);
    }
}

@compute @workgroup_size(8, 8)
fn pass_temporal(@builtin(global_invocation_id) id: vec3<u32>) {
    let dims = vec2<u32>(u.resolution);
    if (id.x >= dims.x || id.y >= dims.y) { return; }
    let idx = id.y * dims.x + id.x;

    var rng = hash(idx ^ hash(u.frame_count));
    
    let uv = vec2<f32>(id.xy) / u.resolution;
    let ndc = uv * 2.0 - 1.0;
    let aspect = u.resolution.x / u.resolution.y;

    let p_local = vec3(ndc.x * aspect, -ndc.y, 2.0);
    let dir = normalize(p_local.x * u.cam_right.xyz + p_local.y * u.cam_up.xyz + p_local.z * u.cam_dir.xyz);
    let ray = Ray(u.cam_pos.xyz, dir);

    var hit: Hit;
    if (intersect_scene(ray, &hit)) {
        let x_pos = ray.origin + ray.dir * hit.t;
        let gb = GBufferData(vec4(x_pos, 1.0), vec4(hit.n, f32(hit.mat_type)), vec4(hit.albedo, 0.0), vec4(hit.t, 0.0, 0.0, 0.0));
        
        if (length(hit.emission) > 0.0) {
            res_curr[idx] = Reservoir(vec4(0.), vec4(0.), vec4(hit.emission, -1.0));
            gbuffer_curr[idx] = gb;
            return;
        }

        if (u.enable_reflections == 1u && hit.mat_type == 2u) {
            var current_ray = ray;
            var current_hit = hit;
            var current_albedo = vec3(1.0);
            var ref_col = vec3(0.0);
            
            var b: u32 = 0u;
            loop {
                if (b >= u.reflection_bounces || current_hit.mat_type != 2u) {
                    break;
                }
                
                var ref_dir = reflect(current_ray.dir, current_hit.n);
                if (current_hit.roughness > 0.0) {
                    let jitter = cone_sample(&rng, current_hit.roughness * 0.5);
                    let t1 = normalize(cross(ref_dir, vec3(0.0, 1.0, 0.0)));
                    let t1_v = select(t1, normalize(cross(ref_dir, vec3(1.0, 0.0, 0.0))), abs(ref_dir.y) > 0.99);
                    let t2 = cross(ref_dir, t1_v);
                    ref_dir = normalize(t1_v * jitter.x + t2 * jitter.y + ref_dir * jitter.z);
                }
                
                let next_origin = current_ray.origin + current_ray.dir * current_hit.t + current_hit.n * 0.001;
                current_ray = Ray(next_origin, ref_dir);
                current_albedo = current_albedo * current_hit.albedo;
                
                if (intersect_scene(current_ray, &current_hit)) {
                    // continue
                } else {
                    ref_col = current_albedo * sky_radiance(ref_dir, true);
                    current_hit.mat_type = 999u;
                    break;
                }
                b = b + 1u;
            }
            
            if (current_hit.mat_type != 999u) {
                if (length(current_hit.emission) > 0.0) {
                    ref_col = current_albedo * current_hit.emission;
                } else {
                    let next_origin = current_ray.origin + current_ray.dir * current_hit.t + current_hit.n * 0.001;
                    let sd = sun_dir();
                    let shadow_ray = Ray(next_origin, sd);
                    if (!trace_occluded(shadow_ray.origin, shadow_ray.dir, 1e4)) {
                        ref_col = current_albedo * current_hit.albedo * max(0.0, dot(current_hit.n, sd)) * u.sunIntensity;
                    }
                    ref_col += current_albedo * current_hit.albedo * sky_radiance(current_hit.n, false) * 0.1;
                    
                    if (u.reflection_gi == 1u && current_hit.mat_type != 2u) {
                        let r1 = randf(&rng);
                        let r2 = randf(&rng);
                        let phi = 2.0 * 3.14159265 * r1;
                        let cos_theta = sqrt(1.0 - r2);
                        let sin_theta = sqrt(r2);
                        let t1 = normalize(cross(current_hit.n, vec3(0.0, 1.0, 0.0)));
                        let t1_valid = select(t1, normalize(cross(current_hit.n, vec3(1.0, 0.0, 0.0))), abs(current_hit.n.y) > 0.99);
                        let t2 = cross(current_hit.n, t1_valid);
                        let gi_dir = normalize(t1_valid * (sin_theta * cos(phi)) + t2 * (sin_theta * sin(phi)) + current_hit.n * cos_theta);
                        
                        var gi_hit: Hit;
                        if (intersect_scene(Ray(next_origin, gi_dir), &gi_hit)) {
                            if (length(gi_hit.emission) > 0.0) {
                                ref_col += current_albedo * current_hit.albedo * gi_hit.emission * cos_theta;
                            } else {
                                let gi_origin = next_origin + gi_dir * gi_hit.t + gi_hit.n * 0.001;
                                let shadow_ray_gi = Ray(gi_origin, sd);
                                if (!trace_occluded(shadow_ray_gi.origin, shadow_ray_gi.dir, 1e4)) {
                                    ref_col += current_albedo * current_hit.albedo * gi_hit.albedo * max(0.0, dot(gi_hit.n, sd)) * u.sunIntensity * cos_theta;
                                }
                                ref_col += current_albedo * current_hit.albedo * gi_hit.albedo * sky_radiance(gi_hit.n, false) * 0.1 * cos_theta;
                            }
                        } else {
                            ref_col += current_albedo * current_hit.albedo * sky_radiance(gi_dir, true) * cos_theta;
                        }
                    }
                }
            }
            
            res_curr[idx] = Reservoir(vec4(0.), vec4(0.), vec4(ref_col, -1.0));
            gbuffer_curr[idx] = gb;
            return;
        }

        var r: Reservoir;
        r.xs_wSum = vec4(0.0); r.ns_M = vec4(0.0); r.Lo_W = vec4(0.0);
        
        let N = max(1u, select(u.initialCandidates, max(u.moving_spp, 2u), u.is_moving == 1u));
        for (var i = 0u; i < N; i = i + 1u) {
            var cand: Reservoir; var pdf: f32 = 0.0;
            cand.xs_wSum = vec4(0.0); cand.ns_M = vec4(0.0); cand.Lo_W = vec4(0.0);
            generate_candidate(gb, &rng, &cand, &pdf);
            let p = target_pdf(cand, gb);
            update_reservoir(&r, cand, select(0.0, p / pdf, pdf > 0.0), 1.0, randf(&rng));
        }

        var histLen = 0.0;
        if (u.enable_temporal == 1u && u.enable_reprojection == 1u && u.frame_count > 0u) {
            let v = x_pos - u.prev_cam_pos.xyz;
            let dist_z = dot(v, u.prev_cam_dir.xyz);
            let dist_x = dot(v, u.prev_cam_right.xyz);
            let dist_y = dot(v, u.prev_cam_up.xyz);
            
            if (dist_z > 0.0) {
                let p_local_x = dist_x * 2.0 / dist_z;
                let p_local_y = dist_y * 2.0 / dist_z;
                let prev_ndc_x = p_local_x / aspect;
                let prev_ndc_y = -p_local_y;
                let prev_uv = vec2(prev_ndc_x, prev_ndc_y) * 0.5 + 0.5;
                
                if (prev_uv.x >= 0.0 && prev_uv.x <= 1.0 && prev_uv.y >= 0.0 && prev_uv.y <= 1.0) {
                    let px = u32(prev_uv.x * u.resolution.x);
                    let py = u32(prev_uv.y * u.resolution.y);
                    let pi = py * u32(u.resolution.x) + px;
                    
                    let pgb = gbuffer_prev[pi];
                    let prev = res_prev[pi];
                    
                    if (pgb.pos_t.w > 0.0) {
                        let normalOk = dot(pgb.norm_mat.xyz, gb.norm_mat.xyz) > u.normalThreshold;
                        let depthOk = abs(pgb.depth_hist.x - gb.depth_hist.x) <= u.depthThreshold * max(gb.depth_hist.x, 1e-3);
                        let planeOk = abs(dot(gb.pos_t.xyz - pgb.pos_t.xyz, gb.norm_mat.xyz)) < 0.02 * max(gb.depth_hist.x, 1e-3);
                        
                        if (normalOk && depthOk && planeOk && prev.ns_M.w > 0.0) {
                            let J = shift_jacobian(gb.pos_t.xyz, pgb.pos_t.xyz, prev);
                            let pn = target_pdf(prev, gb) * J;
                            let maxHist = select(u.maxHistory, 8.0, u.is_moving == 1u);
                            let Mc = min(prev.ns_M.w, maxHist * f32(N));
                            update_reservoir(&r, prev, pn * prev.Lo_W.w * Mc, Mc, randf(&rng));
                            histLen = min(pgb.depth_hist.y, u.maxHistory);
                        }
                    }
                }
            }
        }

        let pFinal = target_pdf(r, gb);
        r.Lo_W.w = select(0.0, r.xs_wSum.w / (r.ns_M.w * pFinal), pFinal > 0.0 && r.ns_M.w > 0.0);
        res_curr[idx] = r;
        
        var gbCur = gb;
        gbCur.depth_hist.y = histLen + 1.0;
        gbuffer_curr[idx] = gbCur;
        
    } else {
        gbuffer_curr[idx] = GBufferData(vec4(0.0), vec4(0.0), vec4(0.0), vec4(0.0));
        res_curr[idx] = Reservoir(vec4(0.0), vec4(0.0), vec4(sky_radiance(dir, true), -1.0));
    }
}

@compute @workgroup_size(8, 8)
fn pass_spatial(@builtin(global_invocation_id) id: vec3<u32>) {
    let dims = vec2<u32>(u.resolution);
    if (id.x >= dims.x || id.y >= dims.y) { return; }
    let idx = id.y * dims.x + id.x;
    var rng = hash(idx ^ hash(u.frame_count + 1337u));

    let gb = gbuffer_curr[idx];
    if (gb.pos_t.w == 0.0) {
        res_spatial[idx] = res_curr[idx];
        return;
    }

    var r = res_curr[idx];
    if (r.Lo_W.w < 0.0) {
        res_spatial[idx] = r;
        return; 
    }

    if (u.enable_spatial > 0u) {
        let ang_start = randf(&rng) * 6.2831853 + f32(u.frame_count) * 2.39996;
        for (var i = 0u; i < u.spatial_samples; i = i + 1u) {
            let rad = sqrt(randf(&rng)) * u.spatial_radius;
            let ang = ang_start + f32(i) * 2.39996;
            let nx = i32(id.x) + i32(cos(ang) * rad);
            let ny = i32(id.y) + i32(sin(ang) * rad);

            if (nx >= 0 && nx < i32(dims.x) && ny >= 0 && ny < i32(dims.y)) {
                let nidx = u32(ny) * dims.x + u32(nx);
                let ngb = gbuffer_curr[nidx];
                
                if (ngb.pos_t.w > 0.0) {
                    let normalOk = dot(ngb.norm_mat.xyz, gb.norm_mat.xyz) > u.normalThreshold;
                    let depthOk = abs(ngb.depth_hist.x - gb.depth_hist.x) <= u.depthThreshold * max(gb.depth_hist.x, 1e-3);
                    let planeOk = abs(dot(gb.pos_t.xyz - ngb.pos_t.xyz, gb.norm_mat.xyz)) < 0.02 * max(gb.depth_hist.x, 1e-3);
                    
                    if (normalOk && depthOk && planeOk) {
                        let nres = res_curr[nidx];
                        let J = shift_jacobian(gb.pos_t.xyz, ngb.pos_t.xyz, nres);
                        var pn = target_pdf(nres, gb) * J;
                        
                        if (pn > 0.0) {
                            let d = normalize(nres.xs_wSum.xyz - gb.pos_t.xyz);
                            if (trace_occluded(gb.pos_t.xyz + gb.norm_mat.xyz * 1e-3, d, length(nres.xs_wSum.xyz - gb.pos_t.xyz) - 2e-3)) {
                                pn = 0.0;
                            }
                        }
                        update_reservoir(&r, nres, pn * nres.Lo_W.w * nres.ns_M.w, nres.ns_M.w, randf(&rng));
                    }
                }
            }
        }
        let pFinal = target_pdf(r, gb);
        r.Lo_W.w = select(0.0, r.xs_wSum.w / (r.ns_M.w * pFinal), pFinal > 0.0 && r.ns_M.w > 0.0);
    }
    res_spatial[idx] = r;
}

@compute @workgroup_size(8, 8)
fn pass_resolve(@builtin(global_invocation_id) id: vec3<u32>) {
    let dims = vec2<u32>(u.resolution);
    if (id.x >= dims.x || id.y >= dims.y) { return; }
    let idx = id.y * dims.x + id.x;
    
    let gb = gbuffer_curr[idx];
    if (gb.pos_t.w == 0.0) {
        let r = res_spatial[idx];
        accum_curr[idx] = vec4(r.Lo_W.xyz, 1.0);
        return;
    }

    let r_final = res_spatial[idx];
    var final_radiance = vec3(0.0);

    if (r_final.Lo_W.w < 0.0) {
        final_radiance = r_final.Lo_W.xyz;
    } else {
        let p_hat = target_pdf(r_final, gb);
        if (p_hat > 0.0 && r_final.Lo_W.w > 0.0) {
            let brdf = gb.albedo_prev.xyz / 3.14159265;
            final_radiance = r_final.Lo_W.xyz * brdf * r_final.Lo_W.w;
        }
    }

    // --- REPROJECTION (MOTION VECTORS) ---
    var prev_idx = idx;
    var histLen = gb.depth_hist.y - 1.0;
    
    if (u.enable_temporal == 1u && u.enable_reprojection == 1u && histLen > 0.0 && u.frame_count > 0u) {
        let aspect = u.resolution.x / u.resolution.y;
        let v = gb.pos_t.xyz - u.prev_cam_pos.xyz;
        let dist_z = dot(v, u.prev_cam_dir.xyz);
        let dist_x = dot(v, u.prev_cam_right.xyz);
        let dist_y = dot(v, u.prev_cam_up.xyz);
        
        if (dist_z > 0.0) {
            let prev_ndc_x = (dist_x * 2.0 / dist_z) / aspect;
            let prev_ndc_y = -(dist_y * 2.0 / dist_z);
            let prev_uv_x = (prev_ndc_x + 1.0) * 0.5;
            let prev_uv_y = (prev_ndc_y + 1.0) * 0.5;

            if (prev_uv_x >= 0.0 && prev_uv_x <= 1.0 && prev_uv_y >= 0.0 && prev_uv_y <= 1.0) {
                let p_x = u32(prev_uv_x * f32(dims.x));
                let p_y = u32(prev_uv_y * f32(dims.y));
                prev_idx = p_y * dims.x + p_x;
            } else {
                histLen = 0.0;
            }
        } else {
            histLen = 0.0;
        }
    } else if (u.enable_reprojection == 1u) {
        histLen = 0.0; // Break history if reprojection fails validation from ReSTIR pass
    }

    let n = max(histLen + 1.0, 1.0);
    let mix_factor = max(1.0 - u.temporal_weight, 1.0 / n); // Allow UI to control the max history weight!
    
    var acc = mix(accum_prev[prev_idx].xyz, final_radiance, mix_factor);
    if (u.enable_temporal == 0u) {
        acc = final_radiance;
        histLen = 0.0;
    }
    accum_curr[idx] = vec4(acc, histLen + 1.0);
}

@compute @workgroup_size(8, 8)
fn pass_postprocess(@builtin(global_invocation_id) id: vec3<u32>) {
    let dims = vec2<u32>(u.resolution);
    if (id.x >= dims.x || id.y >= dims.y) { return; }
    let idx = id.y * dims.x + id.x;
    
    var col = accum_curr[idx].xyz;
    let center_g = gbuffer_curr[idx];
    
    if (u.enable_denoise > 0u && accum_curr[idx].w > 0.0 && center_g.pos_t.w > 0.0) {
        if (u.denoiserType == 1u) {
            // 1. Basic Bilateral
            var sum_col = vec3(0.0); var sum_w = 0.0;
            for (var dy = -2; dy <= 2; dy++) {
                for (var dx = -2; dx <= 2; dx++) {
                    let nx = i32(id.x) + dx; let ny = i32(id.y) + dy;
                    if (nx >= 0 && nx < i32(dims.x) && ny >= 0 && ny < i32(dims.y)) {
                        let nidx = u32(ny) * dims.x + u32(nx);
                        let ng = gbuffer_curr[nidx];
                        if (ng.pos_t.w > 0.0) {
                            let dist2 = distance(ng.pos_t.xyz, center_g.pos_t.xyz);
                            let norm_dot = dot(ng.norm_mat.xyz, center_g.norm_mat.xyz);
                            let w = exp(-dist2 * 10.0) * pow(max(0.0, norm_dot), 32.0);
                            sum_col += accum_curr[nidx].xyz * w;
                            sum_w += w;
                        }
                    }
                }
            }
            if (sum_w > 0.0) { col = sum_col / sum_w; }
            
        } else if (u.denoiserType == 2u) {
            // 2. A-Trous Wavelet Approximation (Single Pass Unrolled)
            var sum_col = col; var sum_w = 1.0;
            let step_sizes = array<i32, 3>(1, 2, 4);
            let kernel = array<f32, 2>(0.5, 0.125);
            
            for (var s = 0; s < 3; s++) {
                let step_s = step_sizes[s];
                for (var dy = -1; dy <= 1; dy++) {
                    for (var dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) { continue; }
                        let nx = i32(id.x) + dx * step_s; let ny = i32(id.y) + dy * step_s;
                        if (nx >= 0 && nx < i32(dims.x) && ny >= 0 && ny < i32(dims.y)) {
                            let nidx = u32(ny) * dims.x + u32(nx);
                            let ng = gbuffer_curr[nidx];
                            if (ng.pos_t.w > 0.0) {
                                let dist2 = distance(ng.pos_t.xyz, center_g.pos_t.xyz);
                                let norm_dot = dot(ng.norm_mat.xyz, center_g.norm_mat.xyz);
                                let k_w = kernel[max(abs(dx), abs(dy))];
                                let w = k_w * exp(-dist2 * 5.0) * pow(max(0.0, norm_dot), 16.0);
                                sum_col += accum_curr[nidx].xyz * w;
                                sum_w += w;
                            }
                        }
                    }
                }
            }
            col = sum_col / sum_w;
            
        } else if (u.denoiserType == 3u) {
            // 3. SVGF Approximation (Variance Guided)
            var mean_l = 0.0; var mean_l2 = 0.0; var v_w = 0.0;
            for (var dy = -1; dy <= 1; dy++) {
                for (var dx = -1; dx <= 1; dx++) {
                    let nx = i32(id.x) + dx; let ny = i32(id.y) + dy;
                    if (nx >= 0 && nx < i32(dims.x) && ny >= 0 && ny < i32(dims.y)) {
                        let nidx = u32(ny) * dims.x + u32(nx);
                        let l = luma(accum_curr[nidx].xyz);
                        mean_l += l; mean_l2 += l * l; v_w += 1.0;
                    }
                }
            }
            mean_l /= v_w; mean_l2 /= v_w;
            let variance = max(0.0, mean_l2 - mean_l * mean_l);
            let center_luma = luma(col);
            
            var sum_col = vec3(0.0); var sum_w = 0.0;
            for (var dy = -2; dy <= 2; dy++) {
                for (var dx = -2; dx <= 2; dx++) {
                    let nx = i32(id.x) + dx; let ny = i32(id.y) + dy;
                    if (nx >= 0 && nx < i32(dims.x) && ny >= 0 && ny < i32(dims.y)) {
                        let nidx = u32(ny) * dims.x + u32(nx);
                        let ng = gbuffer_curr[nidx];
                        if (ng.pos_t.w > 0.0) {
                            let n_luma = luma(accum_curr[nidx].xyz);
                            let luma_diff = abs(center_luma - n_luma);
                            let w_l = exp(-luma_diff / (variance + 1e-4));
                            let dist2 = distance(ng.pos_t.xyz, center_g.pos_t.xyz);
                            let norm_dot = dot(ng.norm_mat.xyz, center_g.norm_mat.xyz);
                            let w = w_l * exp(-dist2 * 10.0) * pow(max(0.0, norm_dot), 32.0);
                            sum_col += accum_curr[nidx].xyz * w;
                            sum_w += w;
                        }
                    }
                }
            }
            if (sum_w > 0.0) { col = sum_col / sum_w; }
            
        } else if (u.denoiserType == 4u) {
            // 4. IGN (Pseudo Blue Noise) + TAA
            let ign = fract(52.9829189 * fract(dot(vec2<f32>(id.xy), vec2<f32>(0.06711056, 0.00583715))));
            let offset_x = i32(cos(ign * 6.28) * 3.0);
            let offset_y = i32(sin(ign * 6.28) * 3.0);
            var sum_col = col; var sum_w = 1.0;
            
            let nx = i32(id.x) + offset_x; let ny = i32(id.y) + offset_y;
            if (nx >= 0 && nx < i32(dims.x) && ny >= 0 && ny < i32(dims.y)) {
                let nidx = u32(ny) * dims.x + u32(nx);
                let ng = gbuffer_curr[nidx];
                if (ng.pos_t.w > 0.0) {
                    let norm_dot = dot(ng.norm_mat.xyz, center_g.norm_mat.xyz);
                    if (norm_dot > 0.9) {
                        sum_col += accum_curr[nidx].xyz;
                        sum_w += 1.0;
                    }
                }
            }
            col = sum_col / sum_w;
            
        } else if (u.denoiserType == 5u) {
            // 5. AMD FFX-style Compute Optimized
            var sum_col = col; var sum_w = 1.0;
            let offsets = array<vec2<i32>, 4>(vec2(0, 3), vec2(0, -3), vec2(3, 0), vec2(-3, 0));
            for(var i=0; i<4; i++) {
                let nx = i32(id.x) + offsets[i].x; let ny = i32(id.y) + offsets[i].y;
                if (nx >= 0 && nx < i32(dims.x) && ny >= 0 && ny < i32(dims.y)) {
                    let nidx = u32(ny) * dims.x + u32(nx);
                    let ng = gbuffer_curr[nidx];
                    if (ng.pos_t.w > 0.0) {
                        let norm_dot = dot(ng.norm_mat.xyz, center_g.norm_mat.xyz);
                        let dist2 = distance(ng.pos_t.xyz, center_g.pos_t.xyz);
                        let w = exp(-dist2 * 15.0) * pow(max(0.0, norm_dot), 64.0);
                        sum_col += accum_curr[nidx].xyz * w;
                        sum_w += w;
                    }
                }
            }
            col = sum_col / sum_w;
        }
    }
    
    col = col * u.exposure;
    col = col / (col + vec3(1.0));
    col = pow(col, vec3(1.0 / 2.2));
    textureStore(tex_out, id.xy, vec4(col, 1.0));
}
`

export const renderWGSL = `
struct VertexOut {
    @builtin(position) position : vec4<f32>,
    @location(0) uv : vec2<f32>,
}

@vertex
fn vs_main(@builtin(vertex_index) VertexIndex : u32) -> VertexOut {
    var pos = array<vec2<f32>, 6>(
        vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0),
        vec2(-1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, 1.0)
    );
    var uv = array<vec2<f32>, 6>(
        vec2(0.0, 1.0), vec2(1.0, 1.0), vec2(0.0, 0.0),
        vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(1.0, 0.0)
    );
    var output : VertexOut;
    output.position = vec4(pos[VertexIndex], 0.0, 1.0);
    output.uv = uv[VertexIndex];
    return output;
}

@group(0) @binding(0) var myTexture: texture_2d<f32>;

@fragment
fn fs_main(@location(0) uv : vec2<f32>) -> @location(0) vec4<f32> {
    let size = textureDimensions(myTexture);
    let coords = vec2<u32>(uv * vec2<f32>(size));
    return textureLoad(myTexture, coords, 0);
}
`
