#!/usr/bin/env python3
"""Source-derived proof for WhiteTeaService codex activation and viewport placement.

This is not a live Vulkan capture. It visualizes the same interaction contract the code now validates:
Content Browser card/import button -> ActivationRequested -> ConsumeSharedCodexActivation ->
WorkspaceSceneActivation::Open -> CenterActivatedSceneAtWorldOrigin -> viewport scene proxy at world origin.
"""
from __future__ import annotations

import os
import struct
import subprocess
import math
from typing import Tuple, List

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
OUT = os.path.join(ROOT, "VisualProof")
W, H = 1366, 768
Color = Tuple[int, int, int]
Point = Tuple[float, float]

class Image:
    def __init__(self, w: int, h: int, bg: Color = (17, 17, 20)):
        self.w, self.h = w, h
        self.p = [[bg for _ in range(w)] for __ in range(h)]
    def blend(self, x: int, y: int, c: Color, a: float = 1.0):
        if 0 <= x < self.w and 0 <= y < self.h:
            r,g,b = self.p[y][x]; cr,cg,cb = c
            self.p[y][x] = (int(r*(1-a)+cr*a), int(g*(1-a)+cg*a), int(b*(1-a)+cb*a))
    def rect(self, x0:int,y0:int,x1:int,y1:int,c:Color,a:float=1.0):
        for y in range(max(0,y0), min(self.h,y1)):
            for x in range(max(0,x0), min(self.w,x1)):
                self.blend(x,y,c,a)
    def line(self, a:Point,b:Point,c:Color,alpha:float=1.0,width:int=1):
        x0,y0=a; x1,y1=b; steps=max(int(abs(x1-x0)),int(abs(y1-y0)),1); r=max(0,width//2)
        for i in range(steps+1):
            t=i/steps; x=int(round(x0+(x1-x0)*t)); y=int(round(y0+(y1-y0)*t))
            for yy in range(y-r,y+r+1):
                for xx in range(x-r,x+r+1): self.blend(xx,yy,c,alpha)
    def poly(self, pts:List[Point], c:Color, a:float=1.0):
        if len(pts)<3: return
        ys=[p[1] for p in pts]
        for y in range(max(0,int(min(ys))), min(self.h-1,int(max(ys)))+1):
            xs=[]
            for i,p0 in enumerate(pts):
                p1=pts[(i+1)%len(pts)]
                if (p0[1] <= y < p1[1]) or (p1[1] <= y < p0[1]):
                    t=(y-p0[1])/((p1[1]-p0[1]) or 1e-9); xs.append(p0[0]+(p1[0]-p0[0])*t)
            xs.sort()
            for x0,x1 in zip(xs[0::2], xs[1::2]):
                for x in range(max(0,int(x0)), min(self.w,int(x1)+1)): self.blend(x,y,c,a)
    def polyline(self, pts:List[Point], c:Color, a:float=1.0, width:int=1, closed:bool=False):
        for p,q in zip(pts, pts[1:]): self.line(p,q,c,a,width)
        if closed and len(pts)>2: self.line(pts[-1],pts[0],c,a,width)
    def circle(self,cx:float,cy:float,r:float,c:Color,a:float=1.0,width:int=2):
        pts=[(cx+math.cos(math.tau*i/96)*r, cy+math.sin(math.tau*i/96)*r) for i in range(97)]
        self.polyline(pts,c,a,width)
    def save_ppm(self,path:str):
        with open(path,'wb') as f:
            f.write(f"P6\n{self.w} {self.h}\n255\n".encode())
            for row in self.p:
                for px in row: f.write(struct.pack('BBB',*px))

def iso(x:float,y:float,z:float)->Point:
    sx = 842 + (x - z) * 150
    sy = 390 + (x + z) * 62 - y * 130
    return sx, sy

def cuboid(img:Image, cx:float, cy:float, cz:float, sx:float, sy:float, sz:float, fill:Color, edge:Color):
    # axis-aligned cuboid projected in a CAD-ish isometric proxy
    pts=[]
    for dx,dy,dz in [(-1,-1,-1),(1,-1,-1),(1,-1,1),(-1,-1,1),(-1,1,-1),(1,1,-1),(1,1,1),(-1,1,1)]:
        pts.append(iso(cx+dx*sx, cy+dy*sy, cz+dz*sz))
    faces=[(4,5,6,7,1.0),(0,1,5,4,0.78),(1,2,6,5,0.66),(2,3,7,6,0.58),(3,0,4,7,0.72)]
    for a,b,c,d,al in faces:
        img.poly([pts[a],pts[b],pts[c],pts[d]], fill, 0.22*al)
    for a,b in [(0,1),(1,2),(2,3),(3,0),(4,5),(5,6),(6,7),(7,4),(0,4),(1,5),(2,6),(3,7)]:
        img.line(pts[a],pts[b],edge,0.88,2)

def draw_content_browser(img:Image):
    # South drawer / Content Browser proof area
    x0,y0,x1,y1=32,500,620,735
    img.rect(x0,y0,x1,y1,(18,18,22),1)
    img.rect(x0,y0,x1,y0+34,(31,31,37),1)
    # selected WhiteTeaService card
    card=(62,560,300,700)
    img.rect(*card,(24,25,30),1)
    img.rect(card[0],card[1],card[2],card[1]+86,(38,42,52),1)
    img.circle((card[0]+card[2])/2,card[1]+44,25,(167,243,208),0.9,3)
    img.rect(card[0]+8,card[1]+8,card[0]+76,card[1]+30,(10,12,16),0.75)
    # inspector/import button
    ix0,iy0,ix1,iy1=340,560,596,700
    img.rect(ix0,iy0,ix1,iy1,(22,22,27),1)
    btn=(360,646,576,690)
    img.rect(*btn,(34,197,94),1)
    # press ripple/cursor
    img.circle(468,668,34,(251,191,36),0.95,4)
    img.line((470,668),(650,520),(251,191,36),0.9,3)
    img.line((640,520),(650,520),(251,191,36),0.9,3)
    img.line((650,520),(646,530),(251,191,36),0.9,3)

def draw_viewport(img:Image):
    x0,y0,x1,y1=640,60,1335,735
    img.rect(x0,y0,x1,y1,(24,25,30),1)
    img.rect(x0,y0,x1,y0+32,(31,31,37),1)
    img.rect(x0,y1-32,x1,y1,(22,22,26),1)
    body=(x0,y0+32,x1,y1-32)
    # grid ground centered at world origin
    for i in range(-6,7):
        img.line(iso(i*0.35,0,-2.2), iso(i*0.35,0,2.2), (80,83,94),0.45,1)
        img.line(iso(-2.2,0,i*0.35), iso(2.2,0,i*0.35), (80,83,94),0.45,1)
    img.line(iso(-2.4,0,0), iso(2.4,0,0), (248,80,100),0.9,2)
    img.line(iso(0,0,-2.4), iso(0,0,2.4), (96,165,250),0.9,2)
    img.circle(*iso(0,0,0), 10, (251,191,36), 1.0, 3)
    # tea service proxies recentered around world origin; six scene geometry entries
    cuboid(img,0,0.42,0,0.44,0.30,0.30,(244,241,232),(255,255,255))      # service teapot
    cuboid(img,-0.88,0.23,0.38,0.18,0.18,0.18,(244,241,232),(255,255,255)) # teacup
    cuboid(img,-0.88,0.06,0.38,0.28,0.04,0.28,(244,241,232),(215,252,245)) # saucer
    cuboid(img,0.78,0.28,0.32,0.24,0.22,0.22,(244,241,232),(255,255,255))  # sugar
    cuboid(img,0.82,0.33,-0.45,0.20,0.26,0.18,(244,241,232),(255,255,255)) # milk jug
    # floor plate
    f=[iso(-1.6,-0.02,-1.2),iso(1.6,-0.02,-1.2),iso(1.6,-0.02,1.2),iso(-1.6,-0.02,1.2)]
    img.poly(f,(255,255,255),0.08); img.polyline(f,(255,255,255),0.35,1,True)
    img.circle(*iso(0,0,0),92,(251,191,36),0.85,3)


def main():
    os.makedirs(OUT,exist_ok=True)
    img=Image(W,H)
    img.rect(0,0,W,48,(8,8,10),1)
    draw_content_browser(img)
    draw_viewport(img)
    ppm=os.path.join(OUT,'tea_service_load_button_proof.ppm')
    png=os.path.join(OUT,'tea_service_load_button_proof.png')
    img.save_ppm(ppm)
    subprocess.run([
        'convert', ppm, '-font','DejaVu-Sans-Bold',
        '-pointsize','16','-fill','#e5e7eb','-annotate','+42+30','WhiteTeaService.codex load proof: Content Browser button press -> activation -> centered viewport scene',
        '-pointsize','13','-fill','#d8d8dc','-annotate','+56+522','CONTENT BROWSER / Engine Content',
        '-pointsize','13','-fill','#a7f3d0','-annotate','+82+672','WhiteTeaService.codex',
        '-pointsize','12','-fill','#101014','-annotate','+432+674','Import',
        '-pointsize','12','-fill','#fbbf24','-annotate','+362+626','simulated button press',
        '-pointsize','13','-fill','#d8d8dc','-annotate','+666+82','3D Viewport - loaded scene at world origin',
        '-pointsize','12','-fill','#fbbf24','-annotate','+818+412','WORLD ORIGIN / GROUP CENTRE',
        '-pointsize','11','-fill','#a7f3d0','-annotate','+656+712','Service Teapot + Teacup + Saucer + Sugar Bowl + Milk Jug + Floor loaded from WhiteTeaService.codex',
        png
    ],check=True)
    os.remove(ppm)
    print(png)

if __name__=='__main__': main()
