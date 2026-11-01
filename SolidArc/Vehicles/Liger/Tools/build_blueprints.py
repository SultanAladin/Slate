"""Generate registered orthographic tracing references from the delivered CAD mesh.
No perspective, invented wheels, surface edits, or automatic modelling features.
"""
import json
from pathlib import Path
from collections import defaultdict
import numpy as np
from numba import njit
from PIL import Image, ImageDraw, ImageFont
from scipy.ndimage import binary_erosion, maximum_filter
from scipy.interpolate import BSpline
from fit_macros import ROOT
from fair_surfaces import read_nets, PARTS, geometry
from tessellate_shared import load_binary, side_uv
from setup_blueprint_project import setup_project

PPM = 1500  # same physical sampling for every view: pixels/metre
VIEWS = [
    ('Top', [0,0,1], [1,0,0], [0,1,0], 'Top'),
    ('Bottom', [0,0,-1], [1,0,0], [0,-1,0], 'Bottom'),
    ('Left', [0,1,0], [-1,0,0], [0,0,1], 'Back'),
    ('Right', [0,-1,0], [1,0,0], [0,0,1], 'Front'),
    ('Front', [1,0,0], [0,1,0], [0,0,1], 'Right'),
    ('Rear', [-1,0,0], [0,-1,0], [0,0,1], 'Left')]


@njit(cache=True)
def zbuffer(screen, tri, W, H):
    depth = np.full((H,W), -1e20, dtype=np.float32)
    for inds in tri:
        A,B,C = screen[inds[0]],screen[inds[1]],screen[inds[2]]
        den=(B[1]-C[1])*(A[0]-C[0])+(C[0]-B[0])*(A[1]-C[1])
        if abs(den)<1e-9: continue
        lx=max(0,int(min(A[0],B[0],C[0]))); hx=min(W-1,int(max(A[0],B[0],C[0]))+1)
        ly=max(0,int(min(A[1],B[1],C[1]))); hy=min(H-1,int(max(A[1],B[1],C[1]))+1)
        for y in range(ly,hy+1):
            for x in range(lx,hx+1):
                u=((B[1]-C[1])*(x-C[0])+(C[0]-B[0])*(y-C[1]))/den
                v=((C[1]-A[1])*(x-C[0])+(A[0]-C[0])*(y-C[1]))/den
                w=1-u-v
                if min(u,v,w)<-1e-6: continue
                z=u*A[2]+v*B[2]+w*C[2]
                if z>depth[y,x]:depth[y,x]=z
    return depth


@njit(cache=True)
def visible_lines(screen, segments, kinds, depth):
    H,W=depth.shape; result=np.zeros((H,W),np.uint8)
    for idx in range(len(segments)):
        a,b=screen[segments[idx,0]],screen[segments[idx,1]]
        count=max(1,int(max(abs(a[0]-b[0]),abs(a[1]-b[1]))*2)+1)
        for j in range(count+1):
            t=j/count; p=a*(1-t)+b*t; x=int(round(p[0]));y=int(round(p[1]))
            if 0<=x<W and 0<=y<H:
                # Interpolate the z-buffer at the projected curve sample, rather
                # than comparing with an offset pixel centre. A broad millimetre
                # slack would incorrectly expose the canopy insert through skin.
                ix=int(np.floor(p[0])); iy=int(np.floor(p[1])); front=0.; weight=0.
                for dy in range(2):
                    for dx in range(2):
                        xx=ix+dx; yy=iy+dy
                        if 0<=xx<W and 0<=yy<H and depth[yy,xx]>-1e19:
                            w=(1-abs(p[0]-xx))*(1-abs(p[1]-yy))
                            front+=w*depth[yy,xx];weight+=w
                if weight>1e-6 and p[2]>=front/weight-.0002:
                    result[y,x]=max(result[y,x],kinds[idx])
    return result


def curves():
    layout=json.loads((ROOT/'Refined/CAD/patch_layout.json').read_text())['after']
    points=[];segments=[];kinds=[];offset=0;counts=defaultdict(int)
    for name in PARTS:
        top=layout[name];nets=read_nets(ROOT/'Refined/CAD'/f'{name}.step');owners=defaultdict(list)
        for net,p in zip(nets,top['patches']):
            if name==PARTS[0] and p['id']==137: continue  # deliberately detached branch
            for side,refs in enumerate(p['sides']):
                for ref in refs:
                    t=np.linspace(.05,.95,41); s=1-t if ref['reverse'] else t
                    uv=side_uv(side,ref['start']+(ref['end']-ref['start'])*s)
                    owners[ref['rail']].append(geometry(net,uv)[1])
        for rid,rail in enumerate(top['rails']):
            item=owners[rid];kind=1
            if len(item)==1: kind=3  # free boundary / opening
            elif len(item)==2:
                angle=np.degrees(np.arccos(np.clip(np.sum(item[0]*item[1],axis=1),-1,1)))
                if np.percentile(angle,95)>=8:kind=2  # actual crease, not every patch seam
            if not item:continue
            P=np.array(rail['P']);K=np.array(rail['K']);degree=len(K)-len(P)-1
            samples=max(33,int(np.linalg.norm(np.diff(P,axis=0),axis=1).sum()*PPM/2)+2)
            q=BSpline(K,P,degree)(np.linspace(K[degree],K[-degree-1],samples))
            counts[{1:'smooth_patch_rails',2:'crease_rails',3:'opening_rails'}[kind]]+=1
            for mirror in [1,-1]:
                xyz=q.copy();xyz[:,1]*=mirror;points.extend(xyz)
                segments.extend(np.column_stack([np.arange(len(q)-1)+offset,np.arange(1,len(q))+offset]));
                kinds.extend([kind]*(len(q)-1));offset+=len(q)
    return np.array(points),np.array(segments,np.int32),np.array(kinds,np.uint8),dict(counts)




def main():
    out=ROOT/'Blueprints';out.mkdir(exist_ok=True)
    for folder in ['References','Schematics','PatchLayouts','Views','Validation']:(out/folder).mkdir(exist_ok=True)
    D=load_binary(ROOT/'Refined/Proofs/after.bin.gz');P=D['pos'];lo=P.min(0).astype(float);hi=P.max(0).astype(float)
    Q,segments,kinds,counts=curves();rows=[]
    font='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    for name,eye,right,up,preset in VIEWS:
        E,R,U=map(lambda x:np.array(x,float),[eye,right,up]);basis=np.stack([R,U,E],axis=1)
        bounds=np.array([[x,y,z] for x in [lo[0],hi[0]] for y in [lo[1],hi[1]] for z in [0,hi[2]]])@basis
        low=np.floor((bounds.min(0)-.12)*PPM)/PPM;high=np.ceil((bounds.max(0)+.12)*PPM)/PPM
        W,H=int(round((high[0]-low[0])*PPM)),int(round((high[1]-low[1])*PPM))
        def screen(q):
            p=q@basis;p[:,0]=(p[:,0]-low[0])*PPM;p[:,1]=(high[1]-p[:,1])*PPM
            return p
        depth=zbuffer(screen(P),D['tri'],W,H);fill=depth>-1e19
        outline=fill & ~binary_erosion(fill,iterations=3)
        projected=screen(Q)
        selected=kinds>=2
        lines=visible_lines(projected,segments[selected],kinds[selected],depth)
        lines=maximum_filter(lines,size=3)
        # Separate registered transparent image, paper sheet, and optional all-seam layout.
        rgba=np.zeros((H,W,4),np.uint8)
        # Bolder registered ink stays legible in a quad viewport. Printable
        # sheets below retain the original finer strokes and exact same mapping.
        bold_outline=maximum_filter(outline,size=5);bold_lines=maximum_filter(lines,size=5)
        rgba[bold_outline]=[180,223,255,255];rgba[bold_lines==2]=[255,174,70,255];rgba[bold_lines==3]=[82,219,234,255]
        ref=Image.fromarray(rgba);ref.save(out/'References'/f'{name}.png',dpi=(300,300))
        paper=np.full((H,W,3),255,np.uint8);paper[fill]=[247,249,251]
        paper[outline]=[27,42,59];paper[lines==2]=[198,96,18];paper[lines==3]=[15,118,151]
        im=Image.fromarray(paper);draw=ImageDraw.Draw(im)
        # Datum guides are on paper only, not interpreted as CAD feature curves.
        origin=screen(np.zeros((1,3)))[0]
        for axis,val in [(0,origin[0]),(1,origin[1])]:
            v=int(round(val));limit=H if axis==0 else W
            if (axis==0 and 0<=v<W) or (axis==1 and 0<=v<H):
                for j in range(0,limit,38):
                    draw.line([(v,j),(v,min(j+20,H-1))] if axis==0 else [(j,v),(min(j+20,W-1),v)],fill='#9cb4ca',width=2)
        sheet=Image.new('RGB',(W+160,H+350),'white');sheet.paste(im,(80,170));d=ImageDraw.Draw(sheet)
        d.text((80,25),f'LIGER / {name.upper()} / ORTHOGRAPHIC',font=ImageFont.truetype(font,54),fill='#172b42')
        d.text((80,104),'CAD-derived exterior body reference • front +X / left +Y / up +Z • no perspective',font=ImageFont.truetype(font,28),fill='#4e6378')
        d.text((80,H+197),'DARK: silhouette  /  CYAN: openings & free rims  /  ORANGE: crease rails  /  DASHED: world datums',font=ImageFont.truetype(font,27),fill='#385069')
        d.text((80,H+242),f'1500 pixels/metre • body envelope {(hi[0]-lo[0])*1000:.1f} × {(hi[1]-lo[1])*1000:.1f} × {(hi[2]-lo[2])*1000:.1f} mm',font=ImageFont.truetype(font,27),fill='#385069')
        d.text((80,H+285),'Reference only. No wheels/interior/underside invented. Smooth patch seams omitted; optional layout supplied.',font=ImageFont.truetype(font,25),fill='#6d7c8e')
        sheet.save(out/'Schematics'/f'Liger_{name}.png',dpi=(300,300))
        all_lines=visible_lines(projected,segments,kinds,depth);all_lines=maximum_filter(all_lines,size=2)
        layout=paper.copy();layout[all_lines==1]=[150,167,184]
        Image.fromarray(layout).save(out/'PatchLayouts'/f'{name}.png',dpi=(300,300))
        # Put the image on the far side of the model for this view; exact physical span.
        plane_depth=float(min(0.,np.min(P@E))-.025)
        corner=R*low[0]+U*high[1]+E*plane_depth
        row=dict(name=name,image=f'References/{name}.png',pixels=[W,H],pixels_per_metre=PPM,
                 eye=E.tolist(),screen_right=R.tolist(),screen_up=U.tolist(),native_preset=preset,
                 corner=corner.tolist(),right=(R*W/PPM).tolist(),down=(-U*H/PPM).tolist(),
                 near_depth=float(np.max(P@E)),plane_depth=plane_depth)
        rows.append(row)
        del rgba,paper,depth,fill,outline,lines,all_lines,layout,projected
        print(name,W,H,flush=True)
    (out/'registration.json').write_text(json.dumps(dict(source_commit='ea9cb133220aa171b6002703ff7bb88ced5673f2',
        source='Refined/CAD/Liger_Refined_CAD.step',units='metres',bounds=[lo.tolist(),hi.tolist()],
        crease_threshold_degrees=8,visibility_depth_tolerance_mm=.2,rail_counts_half=counts,views=rows),indent=2))
    setup_project(out, rows)

    thumbs=[]
    for row in rows:
        pic=Image.open(out/'Schematics'/f"Liger_{row['name']}.png");pic.thumbnail((1600,950));thumbs.append(pic.copy())
    heights=[max(p.height for p in thumbs[i:i+2])+24 for i in range(0,6,2)]
    contact=Image.new('RGB',(3200,sum(heights)),'#edf2f7')
    y=0
    for row in range(3):
        for col in range(2):contact.paste(thumbs[row*2+col],(col*1600,y))
        y+=heights[row]
    contact.save(out/'Liger_Orthographic_Overview.png')


if __name__=='__main__':main()
