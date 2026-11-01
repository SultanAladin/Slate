"""Extract mirrored Catmull–Clark reference, not render-time shrinkwrap/solidify.
Run in Blender Python (bpy 5.0.1). Coordinates: metres, X forward, Z up.
One finite subdivision is applied with limit projection OFF, then two levels
with limit projection ON. Mirror is applied BEFORE subdivision for seam stencils.
"""
from pathlib import Path
import sys, json, hashlib
import bpy
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'Source/Liger_named.blend'
bpy.ops.wm.open_mainfile(filepath=str(SOURCE))
parts = ['Body_Main_Shell', 'Body_Front_Cowl', 'Body_Roof_Glass_Frame']
meta = {'source_sha256': hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
        'bpy': bpy.app.version_string, 'units': 'm', 'parts': [],
        'reference': 'Mirror before Catmull-Clark, level 3 limit-position samples; no shrinkwrap, mask or thickness'}
for name in parts:
    obj = bpy.data.objects[name]
    bpy.context.view_layer.objects.active = obj
    obj.hide_set(False)
    mirror_y = obj.matrix_world.translation.y
    base = {'name': name, 'original_faces_half': len(obj.data.polygons),
            'original_modifiers': [(m.name, m.type, bool(m.show_viewport)) for m in obj.modifiers]}
    for md in list(obj.modifiers):
        if md.type not in ('MIRROR', 'SUBSURF'): obj.modifiers.remove(md)
    for md in list(obj.modifiers):
        if md.type == 'MIRROR':
            md.show_viewport = True
            bpy.ops.object.modifier_apply(modifier=md.name)
    for md in list(obj.modifiers):
        obj.modifiers.remove(md)
    sub = obj.modifiers.new('Finite step', 'SUBSURF')
    sub.levels = 1; sub.use_limit_surface = False; sub.uv_smooth = 'NONE'
    bpy.ops.object.modifier_apply(modifier=sub.name)
    mesh = obj.data
    assert all(len(f.vertices) == 4 for f in mesh.polygons)
    world = np.array(obj.matrix_world)
    def positions(mesh):
        p = np.empty(len(mesh.vertices)*3, dtype=np.float64)
        mesh.vertices.foreach_get('co', p)
        p = p.reshape(-1,3) @ world[:3,:3].T + world[:3,3]
        p[:,1] -= mirror_y
        return p * 0.01
    verts = positions(mesh)
    faces = np.array([list(f.vertices) for f in mesh.polygons], dtype=np.int32)
    right = np.where(verts[faces,:,][...,1].mean(axis=1) >= -1e-7)[0]
    # All target half faces receive a private UV tile. UV interpolation NONE
    # makes the 5x5 L3 samples unambiguous even around extraordinary vertices.
    uv = mesh.uv_layers.new(name='CAD_PrivateFaceUV')
    for f in mesh.polygons:
        x,y = f.index%128, f.index//128
        for li, (u,v) in zip(f.loop_indices, ((0,0),(1,0),(1,1),(0,1))):
            uv.data[li].uv = (x*2+u,y*2+v)
    creases = np.zeros(len(mesh.edges))
    if 'crease_edge' in mesh.attributes:
        creases = np.array([d.value for d in mesh.attributes['crease_edge'].data])
    edges = np.array([list(e.vertices) for e in mesh.edges], dtype=np.int32)
    sub = obj.modifiers.new('Limit sample', 'SUBSURF')
    sub.levels=2; sub.render_levels=2; sub.use_limit_surface=True; sub.uv_smooth='NONE'
    deps=bpy.context.evaluated_depsgraph_get()
    ev=obj.evaluated_get(deps)
    em=ev.to_mesh(preserve_all_data_layers=True, depsgraph=deps)
    ep = positions(em)
    euv=em.uv_layers['CAD_PrivateFaceUV']
    uvs=np.empty(len(em.loops)*2, dtype=np.float64)
    euv.data.foreach_get('uv',uvs);uvs=uvs.reshape(-1,2)
    vi=np.empty(len(em.loops),dtype=np.int32)
    em.loops.foreach_get('vertex_index',vi)
    tx=np.floor((uvs[:,0]+1e-4)/2).astype(int)
    ty=np.floor((uvs[:,1]+1e-4)/2).astype(int)
    fi=ty*128+tx
    u=np.rint((uvs[:,0]-2*tx)*4).astype(int)
    v=np.rint((uvs[:,1]-2*ty)*4).astype(int)
    assert np.all((u>=0)&(u<=4)&(v>=0)&(v<=4))
    samples=np.full((len(faces),5,5,3),np.nan)
    samples[fi,u,v]=ep[vi]
    assert np.isfinite(samples[right]).all()
    # Shared L1 corners evaluated once on the same full mirrored limit mesh.
    limit_vertices=verts.copy()
    for corner,(a,b) in enumerate(((0,0),(4,0),(4,4),(0,4))):
        limit_vertices[faces[:,corner]]=samples[:,a,b]
    half_samples=samples[right]
    half_faces=faces[right]
    np.savez_compressed(ROOT/'Source'/f'{name}_reference.npz',
                        V=limit_vertices,Q=half_faces,S=half_samples,
                        E=edges,crease=creases,source_face=right)
    base.update({'l1_quads_half': len(right), 'mirror_plane_source_cm': mirror_y,
                 'limit_samples_half': int(len(right)*25)})
    meta['parts'].append(base)
    print(name, base, flush=True)
    ev.to_mesh_clear()
(ROOT/'Source/reference.json').write_text(json.dumps(meta,indent=2)+'\n')
