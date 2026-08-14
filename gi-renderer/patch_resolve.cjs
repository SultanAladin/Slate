const fs = require('fs');
let code = fs.readFileSync('src/webgpu/wgsl.ts', 'utf8');

const replacement = `@compute @workgroup_size(8, 8)
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
`;

const startIndex = code.indexOf('@compute @workgroup_size(8, 8)\nfn pass_resolve');
const endIndex = code.indexOf('@compute @workgroup_size(8, 8)\nfn pass_postprocess');

if (startIndex !== -1 && endIndex !== -1) {
    code = code.substring(0, startIndex) + replacement + '\n' + code.substring(endIndex);
    fs.writeFileSync('src/webgpu/wgsl.ts', code);
    console.log('patched successfully');
} else {
    console.log('failed to find indices');
}
