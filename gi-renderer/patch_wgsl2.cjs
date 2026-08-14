const fs = require('fs');
let code = fs.readFileSync('src/webgpu/wgsl.ts', 'utf8');

// Replace Uniforms
code = code.replace(
    /struct Uniforms \{[\s\S]*?pad2: f32,\n\}/,
    `struct Uniforms {
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
}`
);

const postProcessStart = code.indexOf('@compute @workgroup_size(8, 8)\nfn pass_postprocess');
const postProcessEnd = code.indexOf('}\n`\n\nexport const renderWGSL');
const newPostProcess = `@compute @workgroup_size(8, 8)
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
`;

code = code.substring(0, postProcessStart) + newPostProcess + code.substring(postProcessEnd);

fs.writeFileSync('src/webgpu/wgsl.ts', code);
console.log('patched successfully');
