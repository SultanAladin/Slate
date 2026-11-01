"""Deterministic geometry proofs, not generated imagery.
Evaluates exported cubic pole nets and compares CAD grid points to the nearest
triangle of the L3 sampled reference. This is a sampled reverse deviation,
not a global distance certificate. Run in bpy environment (BVHTree required).
"""
import json,pickle,sys,struct
from pathlib import Path
import numpy as np
from scipy.interpolate import BSpline
import bpy
from mathutils.bvhtree import BVHTree
from mathutils import Vector
from fit_macros import Fitter,evaluate,ROOT
sys.modules['__main__'].Fitter=Fitter

PARTS=['Body_Main_Shell','Body_Front_Cowl','Body_Roof_Glass_Frame']
positions=[];normals=[];errors=[];faceids=[];partids=[];triangles=[];boundaries=[];boundpart=[]
reverse_stats=[];offset=0;face=0
metadata={"faces":[]};source_points=[]
for part,name in enumerate(PARTS):
    F=pickle.load(open(ROOT/'CAD'/f'{name}_fit.pkl','rb'))
    S=F.L.S
    sample=S[:,::2,::2].reshape(-1,3)
    source_points.extend((sample,sample*np.array([1,-1,1])))
    # L3 reference triangulation, same limit samples as the extraction.
    rv=S.reshape(-1,3)
    rt=[]
    for i in range(len(S)):
        for u in range(4):
            for v in range(4):
                a=i*25+u*5+v;b=a+5;c=b+1;d=a+1
                rt.extend(((a,b,c),(a,c,d)))
    tree=BVHTree.FromPolygons(rv.tolist(),rt,all_triangles=True)
    part_errors=[];badnorm=0;totalnorm=0
    for patch in F.patches:
        P=patch['P'];ku=patch['ku'];kv=patch['kv']
        nu=max(14,min(88,(len(ku)-4)*2));nv=max(14,min(88,(len(kv)-4)*2))
        u=np.linspace(0,1,nu);v=np.linspace(0,1,nv)
        U,V=np.meshgrid(u,v,indexing='ij');uv=np.c_[U.ravel(),V.ravel()]
        pos=evaluate(P,ku,kv,uv)
        bu=BSpline(ku,np.eye(len(ku)-4),3);bv=BSpline(kv,np.eye(len(kv)-4),3)
        du=np.einsum('ni,nj,ijc->nc',bu(uv[:,0],nu=1),bv(uv[:,1]),P,optimize=True)
        dv=np.einsum('ni,nj,ijc->nc',bu(uv[:,0]),bv(uv[:,1],nu=1),P,optimize=True)
        normal=np.cross(du,dv);normal/=np.maximum(np.linalg.norm(normal,axis=1)[:,None],1e-20)
        errs=[]
        for p,n in zip(pos,normal):
            near=tree.find_nearest(Vector(p))
            if near is None:raise ValueError('Missing reference projection')
            errs.append(near[3]*1000)
            # Disagreement to reference triangle normal is a screening metric,
            # not a proof of self-intersection or a sign-correctness certificate.
            if np.dot(n,near[1])<0:badnorm+=1
            totalnorm+=1
        errs=np.array(errs);part_errors.extend(errs)
        ii=np.arange(nu*nv).reshape(nu,nv)
        t=np.stack((ii[:-1,:-1],ii[1:,:-1],ii[1:,1:],ii[:-1,:-1],ii[1:,1:],ii[:-1,1:]),axis=-1).reshape(-1,3)
        for mirror in (False,True):
            h=patch['history'][-1]
            metadata['faces'].append({'name':name,'patch':patch['id'],'mirror':mirror,'poles':list(P.shape[:2]),'rms':h['heldout_rms_mm'],'max':h['heldout_max_mm']})
            sign=np.array([1,-1 if mirror else 1,1])
            positions.append(pos*sign);normals.append(normal*sign);errors.append(errs)
            faceids.append(np.full(len(pos),face,dtype=np.uint32));partids.append(np.full(len(pos),part,dtype=np.uint32))
            triangles.append((t[:,::-1] if mirror else t)+offset)
            for side in (np.c_[u,np.zeros_like(u)],np.c_[u,np.ones_like(u)],np.c_[np.zeros_like(v),v],np.c_[np.ones_like(v),v]):
                p=evaluate(P,ku,kv,side)*sign
                boundaries.extend(np.stack((p[:-1],p[1:]),axis=1));boundpart.extend([part]*(len(p)-1))
            offset+=len(pos);face+=1
    d=np.array(part_errors)
    reverse_stats.append({'name':name,'cad_grid_points_half':len(d),'cad_to_L3_triangle_rms_mm':float(np.sqrt(np.mean(d*d))),
                         'cad_to_L3_triangle_p99_mm':float(np.percentile(d,99)),
                         'cad_to_L3_triangle_max_mm':float(d.max()),'normal_disagreement_samples':badnorm,'normal_samples':totalnorm})
    print(reverse_stats[-1],flush=True)

pos=np.concatenate(positions).astype('float32');nor=np.concatenate(normals).astype('float32');err=np.concatenate(errors).astype('float32')
fid=np.concatenate(faceids);pid=np.concatenate(partids);tris=np.concatenate(triangles).astype('uint32');lines=np.array(boundaries,dtype='float32')
np.savez_compressed(ROOT/'Proofs/inspection_geometry.npz',pos=pos,nor=nor,error_mm=err,face=fid,part=pid,tri=tris,lines=lines,line_part=np.array(boundpart,dtype='uint32'))
# Browser binary layout: counts; 3f pos,3f normal,1f error,1f face,1f part;
# uint32 triangle indices; 6f line endpoints.
with open(ROOT/'Proofs/geometry.bin','wb') as s:
    s.write(struct.pack('<III',len(pos),len(tris)*3,len(lines)))
    s.write(np.c_[pos,nor,err,fid.astype('float32'),pid.astype('float32')].astype('<f4').tobytes())
    s.write(tris.astype('<u4').tobytes());s.write(lines.astype('<f4').tobytes())
(ROOT/'Proofs/reverse_deviation.json').write_text(json.dumps({'parts':reverse_stats,'note':'CAD grid to nearest L3 sampled-reference triangle; finite sampled check, not a certified global bound.'},indent=2)+'\n')
print('Proof geometry:',len(pos),'vertices',len(tris),'triangles',flush=True)

(ROOT/"Proofs/viewer_metadata.json").write_text(json.dumps(metadata))
np.concatenate(source_points).astype("<f4").tofile(ROOT/"Proofs/reference_points.bin")
