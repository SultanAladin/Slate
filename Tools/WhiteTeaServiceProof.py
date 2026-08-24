#!/usr/bin/env python3
"""CPU one-frame raster proof of WhiteTeaService.codex's current fixed-material viewport resolve."""
from pathlib import Path
import math

ROOT=Path(__file__).resolve().parents[1]; W,H=960,640
scene=[("ServiceTeapot.obj",(0,.035,0)),("Saucer.obj",(.31,0,.05)),("Teacup.obj",(.31,.035,.05)),("SugarBowl.obj",(-.30,0,.08)),("MilkJug.obj",(-.16,0,-.28))]
base=ROOT/'EngineContent/GeometryArchives/WhiteTeaService'

def parse(path):
 v=[]; tris=[]
 for line in path.read_text().splitlines():
  a=line.split()
  if a and a[0]=='v': v.append(tuple(map(float,a[1:4])))
  if a and a[0]=='f':
   q=[int(x.split('/')[0])-1 for x in a[1:]]
   for i in range(1,len(q)-1): tris.append((q[0],q[i],q[i+1]))
 return v,tris
# Camera matching proof composition.
eye=(0,.62,-1.45); target=(0,.12,0); up=(0,1,0)
def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def norm(a):
 l=math.sqrt(dot(a,a)); return tuple(x/l for x in a)
f=norm(sub(target,eye)); r=norm(cross(f,up)); u=cross(r,f)
def project(p):
 q=sub(p,eye); x,y,z=dot(q,r),dot(q,u),dot(q,f)
 if z<=.01:return None
 return (W*(.5+x/z*.72),H*(.52-y/z*.72),z)
pixels=[(0,0,0)]*(W*H); depth=[1e9]*(W*H)
# Sky radiance composition.
for y in range(H):
 t=y/(H-1); c=(int(80*(1-t)+202*t),int(130*(1-t)+220*t),int(190*(1-t)+231*t))
 for x in range(W): pixels[y*W+x]=c
# Current basic shader material colour, front-normal lighting.
light=(-.42,.68,.60); ll=math.sqrt(sum(x*x for x in light)); diffuse=max(light[2]/ll,0); rough=.32; f0=.04
highlight=max(2,128*(1-rough)); spec=f0*(max((light[2]/ll+1)/math.sqrt((light[0]/ll)**2+(light[1]/ll)**2+(light[2]/ll+1)**2),0)**highlight)
colour=tuple(int(255*min(1,.96*(.12+.88*diffuse)+spec)) for _ in range(3))
def tri(a,b,c,col,write_depth=True):
 minx=max(0,int(min(a[0],b[0],c[0])));maxx=min(W-1,int(max(a[0],b[0],c[0])));miny=max(0,int(min(a[1],b[1],c[1])));maxy=min(H-1,int(max(a[1],b[1],c[1])))
 den=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
 if abs(den)<1e-8:return
 for y in range(miny,maxy+1):
  for x in range(minx,maxx+1):
   w1=((b[1]-c[1])*(x-c[0])+(c[0]-b[0])*(y-c[1]))/den;w2=((c[1]-a[1])*(x-c[0])+(a[0]-c[0])*(y-c[1]))/den;w3=1-w1-w2
   if w1>=0 and w2>=0 and w3>=0:
    z=w1*a[2]+w2*b[2]+w3*c[2];i=y*W+x
    if (not write_depth) or z<depth[i]:
     if write_depth: depth[i]=z
     pixels[i]=col
# floor
floor=[(-1.2,-.003,-.9),(1.2,-.003,-.9),(1.2,-.003,1.1),(-1.2,-.003,1.1)];p=[project(x) for x in floor];tri(p[0],p[1],p[2],(184,184,184),False);tri(p[0],p[2],p[3],(184,184,184),False)
for name,offset in scene:
 v,t=parse(base/name);p=[project((x+offset[0],y+offset[1],z+offset[2])) for x,y,z in v]
 for a,b,c in t:
  if p[a] and p[b] and p[c]:tri(p[a],p[b],p[c],colour)
out=ROOT/'VisualProof/WhiteTeaService';out.mkdir(parents=True,exist_ok=True)
with (out/'white-tea-service-raster.ppm').open('wb') as stream:
 stream.write(f'P6\n{W} {H}\n255\n'.encode())
 stream.write(bytes(component for pixel in pixels for component in pixel))
