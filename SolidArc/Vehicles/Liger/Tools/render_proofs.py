"""CPU z-buffer proofs directly from make_proofs.py's evaluated NURBS geometry."""
import json,math
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFont
from numba import njit
ROOT=Path(__file__).resolve().parents[1]

@njit
def raster(screen,normal,tri,face,part,error,mode,W,H):
    image=np.empty((H,W,3),np.uint8);depth=np.full((H,W),-1e20)
    for y in range(H):
        for x in range(W):image[y,x]=np.array([15,22,33],dtype=np.uint8)
    for inds in tri:
        a,b,c=inds;A=screen[a];B=screen[b];C=screen[c]
        den=(B[1]-C[1])*(A[0]-C[0])+(C[0]-B[0])*(A[1]-C[1])
        if abs(den)<1e-8:continue
        loX=max(0,int(min(A[0],B[0],C[0])));hiX=min(W-1,int(max(A[0],B[0],C[0]))+1)
        loY=max(0,int(min(A[1],B[1],C[1])));hiY=min(H-1,int(max(A[1],B[1],C[1]))+1)
        f=int(face[a]);p=int(part[a])
        for y in range(loY,hiY+1):
            for x in range(loX,hiX+1):
                u=((B[1]-C[1])*(x-C[0])+(C[0]-B[0])*(y-C[1]))/den
                v=((C[1]-A[1])*(x-C[0])+(A[0]-C[0])*(y-C[1]))/den
                w=1-u-v
                if min(u,v,w)<-1e-6:continue
                z=u*A[2]+v*B[2]+w*C[2]
                if z<=depth[y,x]:continue
                depth[y,x]=z
                n=u*normal[a]+v*normal[b]+w*normal[c];n=n/max(1e-10,np.sqrt(np.sum(n*n)))
                # Two-sided sheet rendering, no artificial thickness.
                if n[2]<0:n=-n
                light=max(0.,n[0]*-0.35+n[1]*0.55+n[2]*0.76)
                shade=0.22+0.65*light+0.22*max(0.,n[2])**20
                if mode==1:
                    r=((f*71+53)%151+74)/255.;g=((f*97+61)%141+80)/255.;bcol=((f*43+91)%151+80)/255.
                elif mode==2 or mode==4:
                    e=u*error[a]+v*error[b]+w*error[c];t=min(e/2.,1.)
                    if e>5:r=1.;g=0.1;bcol=0.65
                    else:r=min(1.,t*2);g=min(1.,(1-t)*2);bcol=max(0.,1-t*2)
                    if mode==4:
                        r=min(1.,max(0.,4*t-2));g=min(1.,max(0.,2-abs(4*t-2)));bcol=min(1.,max(0.,2-4*t))
                        if e>5:r=1.;g=0.1;bcol=0.65
                    shade=0.7+0.25*light
                elif mode==3:
                    phase=(n[0]*0.9+n[2]*0.3+n[1]*0.15)*32.
                    stripe=(math.sin(phase*math.pi)+1)*0.5
                    stripe=1. if stripe>0.5 else 0.06
                    r=stripe;g=stripe;bcol=stripe;shade=0.85
                else:
                    if p==0:r=0.72;g=0.79;bcol=0.84
                    elif p==1:r=0.37;g=0.44;bcol=0.51
                    else:r=0.17;g=0.23;bcol=0.31
                image[y,x,0]=min(255,int(r*shade*255));image[y,x,1]=min(255,int(g*shade*255));image[y,x,2]=min(255,int(bcol*shade*255))
    return image,depth

@njit
def lines_overlay(img,depth,lines):
    H,W,_=img.shape
    for seg in lines:
        a,b=seg
        steps=int(max(abs(a[0]-b[0]),abs(a[1]-b[1])))+1
        for i in range(steps+1):
            t=i/max(1,steps);p=a*(1-t)+b*t;x=int(p[0]);y=int(p[1])
            if 0<=x<W and 0<=y<H and p[2]>=depth[y,x]-0.005:
                img[y,x]=np.array([22,33,42],dtype=np.uint8)
    return img

def main():
    D=np.load(ROOT/'Proofs/inspection_geometry.npz');pos=D['pos'];nor=D['nor'];tris=D['tri'];segs=D['lines']
    low=pos.min(0);high=pos.max(0);center=(low+high)/2
    print('Proof bounds',low,high,flush=True)
    font='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    big=ImageFont.truetype(font,33);small=ImageFont.truetype(font,17);tiny=ImageFont.truetype(font,14)
    specs=[('01_Front_Quarter','FRONT QUARTER',(7,-9,5),0),('02_Patch_Layout','PATCH LAYOUT',(7,-9,5),1),
           ('03_Rear_Quarter','REAR QUARTER',(-8,-10,5),0),('04_Side','SIDE / ORTHOGRAPHIC',(0,-10,0.8),1),
           ('05_Top','TOP / ORTHOGRAPHIC',(0,0,10),1),('06_Deviation','SAMPLED REVERSE DEVIATION',(7,-9,5),2),
           ('07_Zebra','ZEBRA / NORMAL SCREENING',(7,-9,5),3)]
    W,H=1600,960
    for filename,title,eye,mode in specs:
        forward=np.array(eye,dtype=float);forward/=np.linalg.norm(forward)
        up=np.array([0.,0,1.])
        if abs(np.dot(up,forward))>0.99:up=np.array([1.,0,0.])
        right=np.cross(up,forward);right/=np.linalg.norm(right);up=np.cross(forward,right)
        R=np.stack([right,up,forward],axis=1)
        p=(pos-center)@R
        scale=min((W-160)/np.ptp(p[:,0]),(H-230)/np.ptp(p[:,1]))
        def proj(q):
            s=(q-center)@R;s[...,0]=s[...,0]*scale+W/2;s[...,1]=H/2+48-s[...,1]*scale;return s
        image,depth=raster(proj(pos),nor@R,tris,D['face'],D['part'],D['error_mm'],mode,W,H)
        if mode in (1,2):image=lines_overlay(image,depth,proj(segs))
        im=Image.fromarray(image);d=ImageDraw.Draw(im)
        d.text((48,26),'LIGER / '+title,font=big,fill='#f0f4f7')
        d.text((49,75),'312 NURBS faces  ·  3 exterior sheet parts  ·  93.1% fewer faces than 4,528-patch baseline',font=small,fill='#9fb1c7')
        d.line((48,111,1552,111),fill='#2b3d51',width=1)
        d.text((48,888),'COMPUTED GEOMETRY PROOF  /  01 NOV 2026  /  REVIEW BUILD',font=small,fill='#b6cadc')
        note='Source: Liger_named.blend. No wheels, interior, aero extras, thickness or underside included.'
        if mode==2:note='Blue 0 mm → green 1 mm → red ≥2 mm; magenta >5 mm. CAD grid → nearest L3 reference triangle.'
        if mode==3:note='Zebra highlights normal irregularities. G1/G2 continuity and Class-A quality are NOT certified.'
        d.text((48,921),note,font=tiny,fill='#8298af')
        im.save(ROOT/'Proofs'/f'{filename}.png');print(filename,flush=True)
if __name__=='__main__':main()
