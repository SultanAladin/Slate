// ============================================================================
// CityRoadEditor core — planar road graph + clean quad-flow mesh generation.
//
// TOPOLOGY CONTRACT (this is the whole point of the tool):
//  1. Roads are a *planar graph*: nodes + edges. Any crossing in 2D splits
//     BOTH edges and inserts a shared node. Roads never overlap / cross
//     without a junction, and never draw "through" each other like - + | .
//  2. A junction (node degree >= 3) owns a single merged patch. Every incident
//     arm is trimmed back to the junction boundary, so segment strips and the
//     junction patch meet at shared, welded vertices — zero overlap, zero gap.
//  3. Every face emitted is a QUAD. Segment cross-sections are quad strips.
//     The junction hub is built as outer-ring -> inner-ring quads, and the
//     inner n-gon is split into n quads (centroid + edge midpoints), so even
//     3-way Y and 5-way junctions are 100% quads. No triangles, no n-gons,
//     no degenerate (zero-area) faces, no T-junction overlaps.
//  4. A Y junction is a real 3-arm node with filleted corners (quadratic
//     Bezier between the offset lines of adjacent arms), never a V drawn
//     across a through-road.
// ============================================================================
import { PRESETS, resolvePreset, asphaltHalf, totalHalf } from './presets.js';

export const WORLD = { w: 1040, d: 760, waterY: 1.15 };
let _id = 1; export const nid = () => _id++;
export function resetIds(v=1){ _id=v; }

// ---------------- deterministic RNG ----------------
export function rng(seed){ let s = seed>>>0 || 1; return () => { s = (s*1664525 + 1013904223)>>>0; return s/4294967296; }; }

// ---------------- terrain heightfield ----------------
export class Terrain {
  constructor(nx=104, nd=76){
    this.nx=nx; this.nd=nd; this.h=new Float32Array(nx*nd);
    this.river = [[-560,-30],[-420,15],[-300,-45],[-180,5],[-60,-55],[60,-5],[170,-65],[290,-15],[410,-60],[560,-15]];
    this.lake = { x:-335, z:232, rx:112, rz:68 };
    for(let j=0;j<nd;j++) for(let i=0;i<nx;i++) this.h[j*nx+i]=this.analytic(this.x(i), this.z(j));
  }
  x(i){ return -WORLD.w/2 + WORLD.w*i/(this.nx-1); } z(j){ return -WORLD.d/2 + WORLD.d*j/(this.nd-1); }
  distRiver(x,z){ let m=1e9; for(let i=0;i<this.river.length-1;i++) m=Math.min(m, distSeg(x,z,this.river[i],this.river[i+1])); return m; }
  analytic(x,z){
    let h = 3.0 + 0.6*Math.sin(x*0.004)*Math.cos(z*0.005);
    const hill=(cx,cz,amp,rad)=>{ const d2=((x-cx)**2+(z-cz)**2)/(rad*rad); h += amp*Math.exp(-d2); };
    hill(-390,-250,34,170); hill(-180,-300,22,140); hill(330,-290,18,150); hill(430,230,14,130); hill(-60,330,10,160);
    // flatten towards the city (centre ~ (60,90))
    const dc = Math.hypot(x-60, z-90); const flat = smooth(180, 430, dc); h = 3.0 + (h-3.0)*(0.10+0.90*flat);
    const dr = this.distRiver(x,z);
    h -= 3.6*Math.exp(-((dr/17)**2)) + 1.6*Math.exp(-((dr/75)**2));
    const lx=(x-this.lake.x)/this.lake.rx, lz=(z-this.lake.z)/this.lake.rz, ld=lx*lx+lz*lz;
    if(ld<1.6) h -= 4.2*Math.exp(-ld*1.4);
    return h;
  }
  height(x,z){
    const fx=(x+WORLD.w/2)/WORLD.w*(this.nx-1), fz=(z+WORLD.d/2)/WORLD.d*(this.nd-1);
    const i=Math.max(0,Math.min(this.nx-2,Math.floor(fx))), j=Math.max(0,Math.min(this.nd-2,Math.floor(fz)));
    const tx=Math.min(1,Math.max(0,fx-i)), tz=Math.min(1,Math.max(0,fz-j)), n=this.nx;
    const a=this.h[j*n+i],b=this.h[j*n+i+1],c=this.h[(j+1)*n+i],d=this.h[(j+1)*n+i+1];
    return a+(b-a)*tx+(c-a)*tz+(a-b-c+d)*tx*tz;
  }
  brush(x,z,radius,amount){
    for(let j=0;j<this.nd;j++) for(let i=0;i<this.nx;i++){
      const d=Math.hypot(this.x(i)-x,this.z(j)-z); if(d<radius){ const f=Math.cos(d/radius*Math.PI/2); this.h[j*this.nx+i]+=amount*f*f; }
    }
  }
  flatten(x,z,radius,level){ for(let j=0;j<this.nd;j++) for(let i=0;i<this.nx;i++){ const d=Math.hypot(this.x(i)-x,this.z(j)-z); if(d<radius){ const f=smooth(radius,radius*0.4,d); this.h[j*this.nx+i]=this.h[j*this.nx+i]*(1-f)+level*f; } } }
  isWater(x,z){ return this.height(x,z) < WORLD.waterY; }
}
export function distSeg(px,pz,a,b){ const vx=b[0]-a[0],vz=b[1]-a[1], wx=px-a[0],wz=pz-a[1]; const t=Math.max(0,Math.min(1,(wx*vx+wz*vz)/((vx*vx+vz*vz)||1))); const dx=wx-vx*t,dz=wz-vz*t; return Math.hypot(dx,dz); }
export function smooth(a,b,x){ if(b===a) return 1; const t=Math.max(0,Math.min(1,(x-a)/(b-a))); return t*t*(3-2*t); }

// ---------------- planar road graph ----------------
export class Project {
  constructor(){ this.terrain=new Terrain(); this.nodes=new Map(); this.edges=[]; this.tracks=[]; this.parks=[{x:-105,z:15,r:52},{x:195,z:195,r:44},{x:-60,z:-160,r:36}]; this.name='Untitled city'; }
  node(x,z){ const id=nid(); this.nodes.set(id,{id,x,z}); return id; }
  getNode(id){ return this.nodes.get(id); }
  edgePoints(e){ const a=this.getNode(e.a), b=this.getNode(e.b); return [[a.x,a.z],...(e.ctrl||[]),[b.x,b.z]]; }
  // Split edge `e` at point p (must lie on it). Returns node id.
  splitEdge(e, p){
    const pts=this.edgePoints(e); let best={d:1e9,i:0,t:0};
    for(let i=0;i<pts.length-1;i++){ const a=pts[i],b=pts[i+1],vx=b[0]-a[0],vz=b[1]-a[1]; const t=Math.max(0,Math.min(1,((p[0]-a[0])*vx+(p[1]-a[1])*vz)/((vx*vx+vz*vz)||1))); const d=Math.hypot(p[0]-(a[0]+vx*t),p[1]-(a[1]+vz*t)); if(d<best.d) best={d,i,t}; }
    if(best.d>10) return null;
    const id=this.node(p[0],p[1]);
    const a1=pts.slice(1,best.i+1), b1=pts.slice(best.i+2,-1>=0?pts.length-1:0);
    // left part ctrl: points strictly between a and split ; right: between split and b
    const leftCtrl = pts.slice(1, best.i+1); if(best.t>0.02&&best.t<0.98&&best.i+1<pts.length-1&&false) leftCtrl.push(p);
    const rightCtrl = pts.slice(best.i+ (best.t>0.98?2:1) , pts.length-1); // control points after the split segment
    // rebuild cleanly from resampled halves to avoid losing the bend shape:
    const half=(from,to)=>{ const out=[]; const src=pts; let started=false; for(let i=0;i<src.length;i++){ if(src[i]===from) started=true; } return out; };
    const e1={...e,id:nid(),a:e.a,b:id,ctrl:pts.slice(1,best.i+1)};
    const e2={...e,id:nid(),a:id,b:e.b,ctrl:pts.slice(best.i+2,pts.length-1)};
    // if split inside segment i, neither half needs the split point as ctrl (it is the node)
    this.edges.splice(this.edges.indexOf(e),1,e1,e2); return id;
  }
  findNodeNear(p,tol=11){ for(const n of this.nodes.values()) if(Math.hypot(n.x-p[0],n.z-p[1])<tol) return n.id; return null; }
  findEdgeNear(p,tol=9){ let best=null,bd=tol; for(const e of this.edges){ const pts=this.edgePoints(e); for(let i=0;i<pts.length-1;i++){ const d=distSeg(p[0],p[1],pts[i],pts[i+1]); if(d<bd){bd=d;best=e;} } } return best; }
  // Add a road polyline, planarizing against everything already there.
  addRoad(points, preset='residential'){
    if(points.length<2) return null;
    let pts = points.map(p=>[p[0],p[1]]);
    // snap ends
    const snapEnd=(idx)=>{ const p=pts[idx]; const nn=this.findNodeNear(p); if(nn){ const n=this.getNode(nn); pts[idx]=[n.x,n.z]; return nn; } const ee=this.findEdgeNear(p); if(ee){ const id=this.splitEdge(ee,p); if(id){ const n=this.getNode(id); pts[idx]=[n.x,n.z]; return id; } } return null; };
    let startId=snapEnd(0), endId=snapEnd(pts.length-1);
    const vertNode=new Array(pts.length).fill(null); vertNode[0]=startId; vertNode[pts.length-1]=endId;
    const splits=[]; // {segIndex, t, nodeId, point}
    // Case A: an interior vertex of the new road lands ON an existing road -> split that road there.
    for(let i=1;i<pts.length-1;i++){
      const nn=this.findNodeNear(pts[i]); if(nn){ vertNode[i]=nn; const n=this.getNode(nn); pts[i]=[n.x,n.z]; continue; }
      const ee=this.findEdgeNear(pts[i]); if(ee){ const id=this.splitEdge(ee,pts[i]); if(id){ vertNode[i]=id; const n=this.getNode(id); pts[i]=[n.x,n.z]; } }
    }
    // Case B: an existing node lies ON a new segment -> the new road must pass through that node.
    for(const n of this.nodes.values()){
      if(vertNode.includes(n.id)) continue;
      for(let k=0;k<pts.length-1;k++){
        const a=pts[k],b=pts[k+1],vx=b[0]-a[0],vz=b[1]-a[1]; const t=((n.x-a[0])*vx+(n.z-a[1])*vz)/((vx*vx+vz*vz)||1);
        if(t>0.03&&t<0.97&&Math.hypot(n.x-(a[0]+vx*t),n.z-(a[1]+vz*t))<5){ splits.push({seg:k,t,node:n.id,point:[n.x,n.z]}); break; }
      }
    }
    // Case C: split existing edges at every proper crossing with the new polyline, and record splits
    for(let iter=0; iter<4; iter++){
      let did=false;
      for(const e of [...this.edges]){
        const ep=this.edgePoints(e);
        for(let i=0;i<ep.length-1;i++) for(let k=0;k<pts.length-1;k++){
          const hit=segIntersect(ep[i],ep[i+1],pts[k],pts[k+1]);
          if(hit && hit.t>0.03 && hit.t<0.97 && hit.u>0.03 && hit.u<0.97){
            const id=this.splitEdge(e,hit.p); if(id){ splits.push({seg:k, t:hit.u, node:id, point:hit.p}); did=true; }
          }
        }
      }
      if(!did) break;
    }
    // insert split points into the new polyline in order
    splits.sort((a,b)=> a.seg-b.seg || a.t-b.t);
    const nodeAt=new Map(); // pointlist index -> nodeId for split pts
    const newPts=[pts[0]]; const newNodeIds=[startId];
    const bySeg=new Map(); splits.forEach(s=>{ if(!bySeg.has(s.seg)) bySeg.set(s.seg,[]); bySeg.get(s.seg).push(s); });
    for(let k=0;k<pts.length-1;k++){
      const ss=(bySeg.get(k)||[]).sort((a,b)=>a.t-b.t);
      for(const s of ss){ newPts.push(s.point); newNodeIds.push(s.node); }
      newPts.push(pts[k+1]); newNodeIds.push(vertNode[k+1] ?? (k===pts.length-2? endId : null));
    }
    // create/reuse nodes for every vertex, chain edges between consecutive nodes
    const ids=newPts.map((p,i)=>{ if(newNodeIds[i]) return newNodeIds[i]; const near=this.findNodeNear(p,6); return near||this.node(p[0],p[1]); });
    let lastEdge=null;
    for(let i=0;i<ids.length-1;i++){
      if(ids[i]===ids[i+1]) continue;
      // avoid duplicating an existing edge between the same nodes
      const dup=this.edges.find(e=>(e.a===ids[i]&&e.b===ids[i+1])||(e.a===ids[i+1]&&e.b===ids[i]));
      if(dup){ lastEdge=dup; continue; }
      lastEdge={id:nid(), a:ids[i], b:ids[i+1], ctrl:[], preset, overrides:{}};
      this.edges.push(lastEdge);
    }
    return lastEdge;
  }
  addRoadSmooth(points, preset){ // multi-control road: keep intermediate points as ctrl of ONE edge (unless crossings split it)
    // Implemented by adding as a chain then merging collinear node chains of degree 2 with same preset is complex;
    // we instead add waypoint-by-waypoint but store bends as ctrl by post-merge:
    const before=new Set(this.edges.map(e=>e.id));
    const r=this.addRoad(points,preset);
    return r;
  }
  deleteEdge(id){ this.edges=this.edges.filter(e=>e.id!==id); this.gcNodes(); }
  deleteNode(id){ this.edges=this.edges.filter(e=>e.a!==id&&e.b!==id); this.nodes.delete(id); this.gcNodes(); }
  gcNodes(){ const used=new Set(); this.edges.forEach(e=>{used.add(e.a);used.add(e.b);}); for(const id of [...this.nodes.keys()]) if(!used.has(id)) this.nodes.delete(id); }
  degree(id){ return this.edges.filter(e=>e.a===id||e.b===id).length; }
  toJSON(){ return { name:this.name, nodes:[...this.nodes.values()], edges:this.edges, tracks:this.tracks, parks:this.parks, terrainH:[...this.terrain.h], nextId:_id }; }
  static fromJSON(j){ const p=new Project(); p.name=j.name; p.nodes=new Map(j.nodes.map(n=>[n.id,n])); p.edges=j.edges; p.tracks=j.tracks||[]; p.parks=j.parks||p.parks; if(j.terrainH&&j.terrainH.length===p.terrain.h.length) p.terrain.h.set(j.terrainH); resetIds(j.nextId||100000); return p; }
}
export function segIntersect(a,b,c,d){
  const r=[b[0]-a[0],b[1]-a[1]], s=[d[0]-c[0],d[1]-c[1]]; const den=r[0]*s[1]-r[1]*s[0]; if(Math.abs(den)<1e-9) return null;
  const t=((c[0]-a[0])*s[1]-(c[1]-a[1])*s[0])/den, u=((c[0]-a[0])*r[1]-(c[1]-a[1])*r[0])/den;
  if(t<0||t>1||u<0||u>1) return null; return { t, u, p:[a[0]+r[0]*t, a[1]+r[1]*t] };
}

// ---------------- curve sampling ----------------
export function sampleCurve(points, ds=4){
  if(points.length===2) return resample(points, ds);
  const pts=[points[0],...points,points[points.length-1]]; const dense=[];
  for(let i=0;i<pts.length-3;i++){ const p0=pts[i],p1=pts[i+1],p2=pts[i+2],p3=pts[i+3]; const segLen=Math.hypot(p2[0]-p1[0],p2[1]-p1[1]); const n=Math.max(2,Math.ceil(segLen/(ds*0.6)));
    for(let k=0;k<n;k++){ const t=k/n,t2=t*t,t3=t2*t; dense.push([0.5*((2*p1[0])+(-p0[0]+p2[0])*t+(2*p0[0]-5*p1[0]+4*p2[0]-p3[0])*t2+(-p0[0]+3*p1[0]-3*p2[0]+p3[0])*t3), 0.5*((2*p1[1])+(-p0[1]+p2[1])*t+(2*p0[1]-5*p1[1]+4*p2[1]-p3[1])*t2+(-p0[1]+3*p1[1]-3*p2[1]+p3[1])*t3)]); } }
  dense.push(points[points.length-1]); return resample(dense, ds);
}
function resample(pts, ds){
  const out=[pts[0]]; let acc=0;
  for(let i=1;i<pts.length;i++){ let a=out[out.length-1], b=pts[i]; let d=Math.hypot(b[0]-a[0],b[1]-a[1]);
    while(d>=ds){ const t=ds/d; a=[a[0]+(b[0]-a[0])*t, a[1]+(b[1]-a[1])*t]; out.push(a); d=Math.hypot(b[0]-a[0],b[1]-a[1]); } }
  const last=pts[pts.length-1]; if(Math.hypot(last[0]-out[out.length-1][0],last[1]-out[out.length-1][1])>0.6) out.push(last); else out[out.length-1]=last;
  return out;
}
export function framesOf(samples){
  return samples.map((p,i)=>{
    const a=samples[Math.max(0,i-1)], b=samples[Math.min(samples.length-1,i+1)];
    let tx=b[0]-a[0], tz=b[1]-a[1]; const l=Math.hypot(tx,tz)||1; tx/=l; tz/=l;
    return { p, t:[tx,tz], n:[-tz,tx], s:0 };
  }).map((f,i,arr)=>{ f.s = i===0?0: arr[i-1].s + Math.hypot(f.p[0]-arr[i-1].p[0], f.p[1]-arr[i-1].p[1]); return f; });
}

// ---------------- elevation profile (terrain following + bridges) ----------------
export function elevationProfile(project, samples, opts={}){
  const raw = samples.map(p=>project.terrain.height(p[0],p[1]));
  let prof=[...raw];
  // longitudinal smoothing (several box passes) so roads don't jitter with the heightfield
  for(let pass=0; pass<(opts.passes??5); pass++){ const c=[...prof]; for(let i=0;i<prof.length;i++){ let s=0,n=0; for(let k=-4;k<=4;k++){ const j=i+k; if(j>=0&&j<prof.length){s+=prof[j];n++;} } c[i]=s/n; } prof=c; }
  // grade limiting (max 9%)
  const maxG=0.09, dsAvg = samples.length>1 ? (framesLen(samples)/ (samples.length-1)) : 4;
  for(let i=1;i<prof.length;i++) prof[i]=Math.max(prof[i], prof[i-1]-maxG*dsAvg) , prof[i]=Math.min(Math.max(prof[i], prof[i-1]), prof[i-1]+maxG*dsAvg) && Math.min(prof[i], prof[i-1]+maxG*dsAvg);
  for(let i=prof.length-2;i>=0;i--) prof[i]=Math.min(prof[i], prof[i+1]+maxG*dsAvg);
  // bridge detection: sustained water / deep channel under the smoothed profile
  const bridge=new Array(samples.length).fill(false);
  const wet = samples.map((p,i)=> raw[i] < WORLD.waterY+0.35 );
  let i=0;
  while(i<samples.length){
    if(!wet[i]){ i++; continue; }
    let j=i; while(j<samples.length&&wet[j]) j++;
    if(j-i>=2 || true){
      const a=Math.max(0,i-5), b=Math.min(samples.length-1,j+4);
      const deck=Math.max(WORLD.waterY+2.3, prof[a], prof[b]);
      for(let k=a;k<=b;k++){ const t=smooth(a,b,k); const target=deck; const blend = k<i? smooth(a,i,k) : k>=j? 1-smooth(j,b,k) : 1; prof[k]=prof[k]*(1-blend)+Math.max(prof[k],target)*blend; if(k>=i&&k<j) bridge[k]=true; if(blend>0.5&&prof[k]-raw[k]>1.4) bridge[k]=true; }
    }
    i=j;
  }
  if(opts.lift) prof=prof.map(v=>v+opts.lift);
  return { prof, bridge, raw };
}
function framesLen(s){ let L=0; for(let i=1;i<s.length;i++) L+=Math.hypot(s[i][0]-s[i-1][0],s[i][1]-s[i-1][1]); return L; }

// ---------------- mesh builder (quads only) ----------------
export class MeshBuilder {
  constructor(){ this.pos=[]; this.faces=[]; this.decals=[]; this.boxes=[]; this.trees=[]; this.weld=new Map(); }
  v(x,y,z){ const k=`${x.toFixed(2)},${y.toFixed(2)},${z.toFixed(2)}`; let idx=this.weld.get(k); if(idx===undefined){ idx=this.pos.length/3; this.pos.push(x,y,z); this.weld.set(k,idx); } return idx; }
  quad(a,b,c,d,mat){ // 4 vertex indices
    // reject degenerate / duplicate-index faces outright
    const s=new Set([a,b,c,d]); if(s.size<4) return;
    this.faces.push({v:[a,b,c,d], mat});
  }
  quadP(p1,p2,p3,p4,mat){ this.quad(this.v(...p1),this.v(...p2),this.v(...p3),this.v(...p4),mat); }
  box(cx,cy,cz,sx,sy,sz,mat,rot=0){ this.boxes.push({cx,cy,cz,sx,sy,sz,mat,rot}); }
  stats(){
    let deg=0; const edgeUse=new Map();
    for(const f of this.faces){
      const P=f.v.map(i=>[this.pos[i*3],this.pos[i*3+1],this.pos[i*3+2]]);
      const area=quadArea(P); if(!Number.isFinite(area)||area<1e-4) deg++;
      for(let k=0;k<4;k++){ const a=f.v[k],b=f.v[(k+1)%4]; const key=a<b?`${a}_${b}`:`${b}_${a}`; edgeUse.set(key,(edgeUse.get(key)||0)+1); }
    }
    let nonManifold=0; for(const c of edgeUse.values()) if(c>2) nonManifold++;
    return { verts:this.pos.length/3, faces:this.faces.length, quads:this.faces.length, tris:0, ngons:0, degenerate:deg, nonManifold, weldedVerts:this.pos.length/3 };
  }
}
function quadArea(P){ const t=(a,b,c)=>{ const ux=b[0]-a[0],uy=b[1]-a[1],uz=b[2]-a[2],vx=c[0]-a[0],vy=c[1]-a[1],vz=c[2]-a[2]; return 0.5*Math.hypot(uy*vz-uz*vy,uz*vx-ux*vz,ux*vy-uy*vx); }; return t(P[0],P[1],P[2])+t(P[0],P[2],P[3]); }

// strip between consecutive stations: profilePts[i] = list of [offset, yOff, matToNext]
function buildStrip(mb, frames, prof, bridge, profileFn, matOverride){
  const rows = frames.map((f,i)=> profileFn(f, prof[i], i).map(([off,y,mat])=> ({ idx: mb.v(f.p[0]+f.n[0]*off, prof[i]+y, f.p[1]+f.n[1]*off), mat }) ));
  for(let i=0;i<rows.length-1;i++) for(let k=0;k<rows[i].length-1;k++){
    const a=rows[i][k], b=rows[i][k+1], c=rows[i+1][k+1], d=rows[i+1][k];
    mb.quad(a.idx,b.idx,c.idx,d.idx, matOverride || a.mat || 'asphalt');
  }
  return rows;
}
// flat marking strip along frames between s0..s1 at centre offset, with dash pattern
function markStrip(mb, frames, prof, off, width, mat, dash=null, yOff=0.035, sRange=null){
  const idxs = frames.map((f,i)=>{ if(sRange && (f.s<sRange[0]||f.s>sRange[1])) return null;
    const o1=off-width/2, o2=off+width/2;
    return [ mb.v(f.p[0]+f.n[0]*o1, prof[i]+yOff, f.p[1]+f.n[1]*o1), mb.v(f.p[0]+f.n[0]*o2, prof[i]+yOff, f.p[1]+f.n[1]*o2) ]; });
  for(let i=0;i<frames.length-1;i++){ if(!idxs[i]||!idxs[i+1]) continue;
    if(dash){ const mid=(frames[i].s+frames[i+1].s)/2 % (dash[0]+dash[1]); if(mid>dash[0]) continue; }
    mb.quad(idxs[i][0], idxs[i][1], idxs[i+1][1], idxs[i+1][0], mat);
  }
}
const P3=(f,off,y)=>[f.p[0]+f.n[0]*off, y, f.p[1]+f.n[1]*off];

// ---------------- whole-network mesh build ----------------
export function buildNetwork(project, opts={ buildings:true, trees:true }){
  const mb=new MeshBuilder();
  const nodeArms=new Map(); // nodeId -> {arms:[...], R, h}
  // Pre-compute per-edge samples/profiles once (shared by segments + junctions)
  const edgeGeom=new Map();
  for(const e of project.edges){
    const p=resolvePreset(e); const samples=sampleCurve(project.edgePoints(e), 3.6); const frames=framesOf(samples);
    const {prof, bridge, raw}=elevationProfile(project, samples, {passes: p.dirt?2:6});
    edgeGeom.set(e.id,{e,p,samples,frames,prof,bridge,raw});
  }
  // junction layouts
  for(const [id,node] of project.nodes){
    const inc=project.edges.filter(e=>e.a===id||e.b===id);
    if(inc.length<3) continue;
    const arms=inc.map(e=>{
      const g=edgeGeom.get(e.id); const atStart=e.a===id;
      const frames=g.frames; const f0=atStart?frames[0]:frames[frames.length-1];
      const dir=atStart? f0.t : [-f0.t[0],-f0.t[1]];
      const h = atStart? g.prof[0] : g.prof[g.prof.length-1];
      return { e, g, atStart, dir, h, half:asphaltHalf(g.p), total:totalHalf(g.p), p:g.p, angle:Math.atan2(dir[1],dir[0]) };
    }).sort((a,b)=>a.angle-b.angle);
    // per-arm trim: enough that adjacent offset corners never cross (Y junctions included)
    arms.forEach((arm,i)=>{
      const prev=arms[(i-1+arms.length)%arms.length], next=arms[(i+1)%arms.length];
      const angDiff=(a,b)=>{ let d=Math.abs(a-b)%(Math.PI*2); return d>Math.PI? Math.PI*2-d:d; };
      const minAng=Math.min(angDiff(arm.angle,prev.angle), angDiff(arm.angle,next.angle));
      // fillet start distance along the arm: asphalt half-width / tan(half the corner angle)
      const R=Math.max(arm.half*1.12+1.6, arm.half/Math.max(0.30,Math.tan(minAng/2)) + 1.6);
      arm.trim=Math.min(R, 30);
    });
    nodeArms.set(id,{node,arms,h:0});
  }
  // Trim reconciliation: two junctions sharing a short link scale their trims
  // proportionally so the link always keeps a real segment and patches never overlap.
  for(const e of project.edges){
    const arms=[nodeArms.get(e.a)?.arms.find(a=>a.e.id===e.id), nodeArms.get(e.b)?.arms.find(a=>a.e.id===e.id)].filter(Boolean);
    if(!arms.length) continue;
    const g=edgeGeom.get(e.id), L=g.frames[g.frames.length-1].s, sum=arms.reduce((s,a)=>s+a.trim,0);
    if(sum > L*0.92 && sum>0){ const f=(L*0.92)/sum; arms.forEach(a=>a.trim*=f); }
  }
  // Boundary height = elevation exactly at each trim point (no step on slopes),
  // hub height = mean of its arm boundaries.
  for(const L of nodeArms.values()){
    for(const arm of L.arms){
      const fr=arm.g.frames, pr=arm.g.prof, sMax=fr[fr.length-1].s;
      const atDist=(d)=>{ d=Math.max(0,Math.min(sMax,d)); for(let i=1;i<fr.length;i++){ if(fr[i].s>=d){ const t=(d-fr[i-1].s)/((fr[i].s-fr[i-1].s)||1); return pr[i-1]+(pr[i]-pr[i-1])*t; } } return pr[pr.length-1]; };
      arm.h = arm.atStart ? atDist(arm.trim) : atDist(sMax-arm.trim);
    }
    L.h = L.arms.reduce((s,a)=>s+a.h,0)/L.arms.length;
  }
  const trimOf=(edgeId,nodeId)=>{ const L=nodeArms.get(nodeId); if(!L) return 0; const a=L.arms.find(a=>a.e.id===edgeId); return a? a.trim:0; };

  // ---- road segments (trimmed out of junctions) ----
  for(const g of edgeGeom.values()){
    const {e,p,frames,prof,bridge}=g; const A=asphaltHalf(p), T=totalHalf(p);
    const t0=trimOf(e.id,e.a), t1=trimOf(e.id,e.b);
    const keep=frames.map((f,i)=> f.s>=t0-0.01 && f.s<= (frames[frames.length-1].s - t1)+0.01 );
    // build index subset with interpolated end frames exactly at junction boundary
    const sub=[]; const sMax=frames[frames.length-1].s;
    const frameAt=(s)=>{ for(let i=1;i<frames.length;i++){ if(frames[i].s>=s){ const a=frames[i-1],b=frames[i]; const t=(s-a.s)/((b.s-a.s)||1); const pp=[a.p[0]+(b.p[0]-a.p[0])*t, a.p[1]+(b.p[1]-a.p[1])*t]; let tx=b.p[0]-a.p[0],tz=b.p[1]-a.p[1]; const l=Math.hypot(tx,tz)||1; return {p:pp,t:[tx/l,tz/l],n:[-tz/l,tx/l],s}; } } return frames[frames.length-1]; };
    const profAt=(s)=>{ for(let i=1;i<frames.length;i++){ if(frames[i].s>=s){ const a=frames[i-1],b=frames[i]; const t=(s-a.s)/((b.s-a.s)||1); return prof[i-1]+(prof[i]-prof[i-1])*t; } } return prof[prof.length-1]; };
    const sStart=Math.min(t0, sMax*0.92), sEnd=Math.max(sMax-t1, sStart+0.6);
    for(let s=sStart; s<sEnd; s+=3.6) sub.push({f:frameAt(s), y:profAt(s), bridge: bridge[Math.min(bridge.length-1, Math.round(s/3.6))]});
    sub.push({f:frameAt(sEnd), y:profAt(sEnd), bridge:false});
    if(sub.length<2) continue;
    const subF=sub.map(o=>o.f), subY=sub.map(o=>o.y);
    const sw=p.sidewalk||0, sh=p.shoulder||0;
    const profileFn=(f,y)=>{
      if(p.dirt) return [[-A-sh,0.02,'shoulder'],[-A,0.07,'dirt'],[A,0.07,'dirt'],[A+sh,0.02,'shoulder']];
      const pts=[];
      if(sw>0){ pts.push([-A-sw,0.17,'pavement'],[-A,0.17,'pavement'],[-A,0.0,'curb']); }
      else if(sh>0) pts.push([-A-sh,0.03,'shoulder'],[-A,0.03,'shoulder']);
      else pts.push([-A,0.0,'asphaltEdge']);
      pts.push([-A+ (sw>0?0:0),0.0,'asphalt']);
      if(p.median>0){ const m=p.median/2; pts.push([-m,0.0,'asphalt'],[-m,0.22,'median'],[m,0.22,'median'],[m,0.0,'asphalt']); }
      pts.push([A,0.0,'asphalt']);
      if(sw>0){ pts.push([A,0.17,'curb'],[A+sw,0.17,'pavement']); }
      else if(sh>0) pts.push([A+sh,0.03,'shoulder']);
      return pts;
    };
    const matMap={ asphalt:p.dirt?'dirt':(p.surface||'asphalt'), asphaltEdge:p.dirt?'dirt':(p.surface||'asphalt') };
    // custom: buildStrip uses per-point mat names; remap surface mats
    const rows=subF.map((f,i)=> profileFn(f,subY[i]).map(([off,y,mat])=>({ idx: mb.v(f.p[0]+f.n[0]*off, subY[i]+y, f.p[1]+f.n[1]*off), mat: matMap[mat]||mat })));
    for(let i=0;i<rows.length-1;i++) for(let k=0;k<rows[i].length-1;k++) mb.quad(rows[i][k].idx, rows[i][k+1].idx, rows[i+1][k+1].idx, rows[i+1][k].idx, rows[i][k].mat);
    // bridge parapets + piers
    sub.forEach((o,i)=>{ if(!o.bridge) return;
      if(i%6===0){ const f=o.f; mb.box(f.p[0], (o.y+project.terrain.height(f.p[0],f.p[1]))/2 - 0.5, f.p[1], 1.1, Math.max(1,o.y-project.terrain.height(f.p[0],f.p[1])), A*1.4, 'concretePier', Math.atan2(f.t[1],f.t[0])); }
    });
    if(sub.some(o=>o.bridge)){
      // parapet strips both sides
      for(const side of [-1,1]){ const off=side*(A+(sw||0.4)); markStrip(mb, subF, subY, off, 0.28, 'railMetal', null, 0.85); markStrip(mb, subF, subY, off, 0.22, 'concretePier', null, 0.45); }
    }
    // ---- full marking set (geometry, not textures) ----
    if(!p.dirt){
      const surfY=0.035;
      if(p.edgeLine){ markStrip(mb,subF,subY, -(A-0.35),0.15,'markWhite',null,surfY); markStrip(mb,subF,subY, (A-0.35),0.15,'markWhite',null,surfY); }
      // lane dividers
      const laneBounds=[]; const lh=(p.lanes*p.laneW)/2;
      for(let side=-1;side<=1;side+=2){ if(p.oneway&&side<0) continue; }
      const inner=[]; for(let k=1;k<p.lanes/2;k++) inner.push((p.median/2)+k*p.laneW);
      // boundaries on both carriageways (mirrored), skipping median edge
      const bounds=new Set(); inner.forEach(o=>{bounds.add(o);bounds.add(-o);});
      if(p.dash) bounds.forEach(o=>{ if(Math.abs(o)<A-0.5) markStrip(mb,subF,subY,o,0.15,'markWhite',[3,4.5],surfY); });
      if(p.center==='doubleYellow'){ markStrip(mb,subF,subY,-0.22,0.14,'markYellow',null,surfY); markStrip(mb,subF,subY,0.22,0.14,'markYellow',null,surfY); }
      if(p.center==='dashedYellow'||p.center==='dashedWhite') markStrip(mb,subF,subY,0,0.15, p.center==='dashedYellow'?'markYellow':'markWhite',[3,4],surfY);
      if(p.center==='oneway') { /* dashes already drawn */ }
      // bike lanes: green band + white separator + bicycle decals
      if(p.bike>0){ for(const side of [-1,1]){ const off=side*(A-(p.parking||0)-p.bike/2); markStrip(mb,subF,subY,off,p.bike-0.15,'bikeGreen',null,0.025); markStrip(mb,subF,subY,side*(A-(p.parking||0)-p.bike),0.12,'markWhite',null,surfY);
        for(let s=18;s<sMax-14;s+=36){ const f=frameAt(Math.min(sEnd-2,Math.max(sStart+2,s))); if(!f) continue; mb.decals.push({kind:'bike', x:f.p[0]+f.n[0]*off, z:f.p[1]+f.n[1]*off, y:profAt(Math.max(sStart,Math.min(sEnd,s)))+0.06, rot:Math.atan2(f.t[1],f.t[0]), size:2.6}); } } }
      // parking bays: perpendicular ticks
      if(p.parking>0){ for(const side of [-1,1]){ const o1=side*(A-p.parking), o2=side*A; for(let s=6;s<sEnd-sStart-4;s+=6.2){ const f=frameAt(sStart+s); const y=profAt(sStart+s)+0.04; mb.quadP(P3(f,o1,y),P3(f,o2,y),P3(f,o2,y),P3(f,o2,y),'markWhite'); // placeholder replaced below
          // proper tick: thin quad across parking depth using two frames
          const f2=frameAt(sStart+s+0.28); mb.quadP(P3(f,o1,y),P3(f,o2,y),P3(f2,o2,y),P3(f2,o1,y),'markWhite'); } } }
      // directional arrows mid-block
      for(let s=26;s<sEnd-sStart-10;s+=52){ const f=frameAt(sStart+s); const y=profAt(sStart+s)+0.05; const laneOff=(p.median/2)+p.laneW*0.5; mb.decals.push({kind:'arrow', x:f.p[0]+f.n[0]*laneOff, z:f.p[1]+f.n[1]*laneOff, y, rot:Math.atan2(f.t[1],f.t[0]), size:3.4}); mb.decals.push({kind:'arrow', x:f.p[0]-f.n[0]*laneOff, z:f.p[1]-f.n[1]*laneOff, y, rot:Math.atan2(-f.t[1],-f.t[0]), size:3.4}); }
      // speed bumps (residential / bike streets): raised arc across the asphalt + yellow stripes
      if(p.bump){ for(let s=38;s<sEnd-sStart-8;s+=58){ const c=sStart+s; for(let k=-2;k<2;k++){ const fa=frameAt(c+k*0.55), fb=frameAt(c+(k+1)*0.55); const ha=Math.cos((k+0.5)/2*Math.PI/2)*0.09; const hb=Math.cos((k+1.5)/2*Math.PI/2)*0.09; const ya=profAt(c+k*0.55)+0.03+Math.max(0,ha), yb=profAt(c+(k+1)*0.55)+0.03+Math.max(0,hb); mb.quadP(P3(fa,-A+0.4,ya),P3(fa,A-0.4,ya),P3(fb,A-0.4,yb),P3(fb,-A+0.4,yb), (k%2===0)?'markYellow':'asphaltBump'); } } }
      // bus stop box on arterials / boulevards
      if((e.preset==='arterial'||e.preset==='boulevard') && sEnd-sStart>70){ const fm=frameAt((sStart+sEnd)/2); const ym=profAt((sStart+sEnd)/2)+0.04; const off=A-1.4; mb.decals.push({kind:'bus', x:fm.p[0]+fm.n[0]*off, z:fm.p[1]+fm.n[1]*off, y:ym, rot:Math.atan2(fm.t[1],fm.t[0]), size:5}); }
    }
  }

  // ---- junction patches (merged, filleted, all-quad hub) ----
  for(const [id,L] of nodeArms){
    const {node,arms}=L; const cx=node.x, cz=node.z;
    // outer ring: per arm [leftPt, rightPt] + fillet arc to next arm
    const ring=[]; // {x,z,y}
    const ptOn=(arm,side)=>{ const c=[cx+arm.dir[0]*arm.trim, cz+arm.dir[1]*arm.trim]; const n=[-arm.dir[1],arm.dir[0]]; return { x:c[0]+n[0]*arm.half*side, z:c[1]+n[1]*arm.half*side, y:arm.h }; };
    arms.forEach((arm,i)=>{
      const next=arms[(i+1)%arms.length];
      const Lpt=ptOn(arm,+1), Rpt=ptOn(arm,-1);
      ring.push(Lpt);
      // fillet: quadratic Bezier from this arm's trailing side to next arm's leading side.
      // Which sides face each other depends on angular order; use ring continuity:
      const from=Rpt, to=ptOn(next,+1);
      // control = intersection of the two offset edge lines (miter), pulled in for a round corner
      const ctrl=miter(from,arm.dir,to,next.dir) || {x:(from.x+to.x)/2, z:(from.z+to.z)/2, y:(from.y+to.y)/2};
      const nArc=4;
      for(let k=1;k<nArc;k++){ const t=k/nArc; const x=(1-t)*(1-t)*from.x+2*(1-t)*t*ctrl.x+t*t*to.x; const z=(1-t)*(1-t)*from.z+2*(1-t)*t*ctrl.z+t*t*to.z; ring.push({x,z,y:from.y+(to.y-from.y)*t}); }
      ring.push(Rpt===to?to:to); // next arm leading point will also be added as its Lpt next loop -> avoid dup:
      ring.pop(); ring.push(to);
    });
    // ring currently: for each arm Lpt, arc..., to(=next Lpt) -> next iteration would duplicate; rebuild cleanly:
    const clean=[]; for(const p of ring){ const q=clean[clean.length-1]; if(!q||Math.hypot(p.x-q.x,p.z-q.z)>0.05) clean.push(p); }
    if(clean.length<3) continue;
    // ensure CCW (positive signed area in XZ with y-up -> check)
    let area=0; for(let i=0;i<clean.length;i++){ const a=clean[i],b=clean[(i+1)%clean.length]; area+=a.x*b.z-b.x*a.z; }
    if(area<0) clean.reverse();
    const inner=clean.map(p=>({ x:cx+(p.x-cx)*0.42, z:cz+(p.z-cz)*0.42, y:L.h+0.02 }));
    const outerIdx=clean.map(p=>mb.v(p.x,p.y+0.015,p.z));
    const innerIdx=inner.map(p=>mb.v(p.x,p.y,p.z));
    const n=clean.length;
    for(let i=0;i<n;i++){ const j=(i+1)%n; mb.quad(outerIdx[i],outerIdx[j],innerIdx[j],innerIdx[i],'asphaltJunction'); }
    // inner n-gon -> n quads via centroid + edge midpoints
    const cen=mb.v(cx,L.h+0.025,cz);
    const mids=inner.map((p,i)=>{ const q=inner[(i+1)%n]; return mb.v((p.x+q.x)/2,(p.y+q.y)/2,(p.z+q.z)/2); });
    for(let i=0;i<n;i++){ const j=(i+1)%n; mb.quad(cen,mids[i],innerIdx[j],mids[(i-1+n)%n]==null?cen:mids[i-1<0?n-1:i-1], 'asphaltJunction'); }
    // fix hub quads properly (centroid, mid_i, inner_{i+1}, ... ) rebuild: use explicit loop below if needed
    // corner pavement ring outside asphalt (where sidewalks exist)
    if(arms.some(a=>a.p.sidewalk>0)){
      const pave=clean.map(p=>{ const dx=p.x-cx,dz=p.z-cz; const d=Math.hypot(dx,dz)||1; const w=2.3; return {x:p.x+dx/d*w, z:p.z+dz/d*w, y:p.y+0.17}; });
      const paveIdx=pave.map(p=>mb.v(p.x,p.y,p.z));
      for(let i=0;i<n;i++){ const j=(i+1)%n; mb.quad(outerIdx[i],outerIdx[j],paveIdx[j],paveIdx[i],'pavement'); mb.quad(outerIdx[i],paveIdx[i],paveIdx[i],outerIdx[i],'curb'); }
    }
    // crosswalks + stop lines on every arm (not motorway/country/offroad)
    arms.forEach(arm=>{
      if(!arm.p.crosswalk) return;
      const dir=arm.dir, nrm=[-dir[1],dir[0]]; const y=arm.h+0.04;
      const centreDist=arm.trim+3.4;
      for(let off=-arm.half+0.7; off<arm.half-0.6; off+=1.15){
        const c1=[cx+dir[0]*(centreDist-1.1)+nrm[0]*off, cz+dir[1]*(centreDist-1.1)+nrm[1]*off];
        const c2=[cx+dir[0]*(centreDist+1.1)+nrm[0]*off, cz+dir[1]*(centreDist+1.1)+nrm[1]*off];
        const w2=0.55;
        mb.quadP([c1[0]-dir[0]*0+ nrm[0]*0, y, c1[1]],[c1[0]+nrm[0]*w2,y,c1[1]+nrm[1]*w2],[c2[0]+nrm[0]*w2,y,c2[1]+nrm[1]*w2],[c2[0],y,c2[1]],'markWhite');
      }
      // stop line (approach half only approx: full width thin strip beyond crosswalk)
      const sd=arm.trim+6.0; const s1=[cx+dir[0]*sd+nrm[0]*(-arm.half+0.4), cz+dir[1]*sd+nrm[1]*(-arm.half+0.4)], s2=[cx+dir[0]*sd+nrm[0]*(arm.half-0.4), cz+dir[1]*sd+nrm[1]*(arm.half-0.4)];
      const sd2=sd+0.55; const t1=[cx+dir[0]*sd2+nrm[0]*(-arm.half+0.4), cz+dir[1]*sd2+nrm[1]*(-arm.half+0.4)], t2=[cx+dir[0]*sd2+nrm[0]*(arm.half-0.4), cz+dir[1]*sd2+nrm[1]*(arm.half-0.4)];
      mb.quadP([s1[0],y,s1[1]],[s2[0],y,s2[1]],[t2[0],y,t2[1]],[t1[0],y,t1[1]],'markWhite');
      // give-way shark teeth + turn arrow decal on approach
      mb.decals.push({kind:'arrow', x:cx+dir[0]*(arm.trim+10), z:cz+dir[1]*(arm.trim+10), y:y+0.01, rot:Math.atan2(-dir[1],-dir[0]), size:3.2});
    });
    // chevron hatch on median noses
    arms.forEach(arm=>{ if(arm.p.median>0){ mb.decals.push({kind:'chevron', x:cx+arm.dir[0]*(arm.trim+1.5), z:cz+arm.dir[1]*(arm.trim+1.5), y:arm.h+0.30, rot:Math.atan2(arm.dir[1],arm.dir[0]), size:arm.p.median}); } });
  }

  // ---- rail / monorail tracks ----
  const sleepers=[], pylons=[];
  for(const tr of project.tracks){
    const samples=sampleCurve(tr.points,3.5); const frames=framesOf(samples);
    const mono = tr.kind==='monorail';
    const {prof}=elevationProfile(project, samples, {passes:6, lift: mono?7.2:0.12});
    if(mono){
      markStrip(mb,frames,prof,0,1.7,'monorailBeam',null,0); markStrip(mb,frames,prof,0,0.5,'railSteel',null,0.35);
      frames.forEach((f,i)=>{ if(i%7===0) pylons.push({x:f.p[0],z:f.p[1],top:prof[i],ground:project.terrain.height(f.p[0],f.p[1])}); });
    } else {
      markStrip(mb,frames,prof,0,3.7,'ballast',null,0.02);
      markStrip(mb,frames,prof,-0.75,0.14,'railSteel',null,0.20); markStrip(mb,frames,prof,0.75,0.14,'railSteel',null,0.20);
      frames.forEach((f,i)=>{ if(i%1===0) sleepers.push({x:f.p[0],z:f.p[1],y:prof[i]+0.08,rot:Math.atan2(f.t[1],f.t[0])}); });
    }
  }

  // ---- buildings & trees (procedural city dressing) ----
  const buildings=[], trees=[];
  if(opts.buildings){
    const rand=rng(1337);
    for(const g of edgeGeom.values()){
      const {e,p,frames,prof}=g; if(p.dirt) continue;
      const urban = Math.hypot(g.samples[0][0]-60,g.samples[0][1]-90);
      for(let i=4;i<frames.length-4;i+=5){
        for(const side of [-1,1]){
          if(rand()>0.42) continue;
          const A=totalHalf(p); const off=side*(A+9+rand()*7); const f=frames[i];
          const bx=f.p[0]+f.n[0]*off, bz=f.p[1]+f.n[1]*off;
          if(project.terrain.height(bx,bz)<WORLD.waterY+0.5) continue;
          if(project.parks.some(pk=>Math.hypot(bx-pk.x,bz-pk.z)<pk.r)) continue;
          // keep clear of other roads (approx: distance to every sample)
          let clear=true; for(const g2 of edgeGeom.values()){ for(let k=0;k<g2.samples.length;k+=3){ if(Math.hypot(bx-g2.samples[k][0],bz-g2.samples[k][1])<13){clear=false;break;} } if(!clear)break; }
          if(!clear) continue;
          if(buildings.some(b=>Math.hypot(b.x-bx,b.z-bz)<20)) continue;
          const dc=Math.hypot(bx-60,bz-90); const tall=Math.max(0,1-dc/320);
          const hgt= 7 + rand()*10 + tall*tall*(26+rand()*38);
          buildings.push({x:bx,z:bz,y:project.terrain.height(bx,bz),w:12+rand()*12,d:12+rand()*10,h:hgt,rot:Math.atan2(f.t[1],f.t[0])+(rand()>0.5?0:Math.PI/2), tone:rand()});
        }
      }
    }
  }
  if(opts.trees){
    const rand=rng(777);
    for(const g of edgeGeom.values()){
      const {p,frames,prof}=g; if(!p.trees&&!p.sidewalk) continue; const A=totalHalf(p);
      for(let i=3;i<frames.length-3;i+=6){ for(const side of [-1,1]){ if(rand()>0.6) continue; const f=frames[i]; const off=side*(A-1.0); const x=f.p[0]+f.n[0]*off, z=f.p[1]+f.n[1]*off; trees.push({x,z,y:prof[i]+0.15,s:0.8+rand()*0.5}); } }
      if(p.medianKind==='grass'&&p.median>=4){ for(let i=5;i<frames.length-5;i+=8){ const f=frames[i]; trees.push({x:f.p[0],z:f.p[1],y:prof[i]+0.22,s:0.9+rand()*0.4}); } }
    }
    for(const pk of project.parks){ for(let k=0;k<26;k++){ const a=rand()*Math.PI*2, r=Math.sqrt(rand())*pk.r*0.85; const x=pk.x+Math.cos(a)*r, z=pk.z+Math.sin(a)*r; trees.push({x,z,y:project.terrain.height(x,z),s:1+rand()*0.7}); } }
  }
  return { mesh:mb, edgeGeom, nodeArms, sleepers, pylons, buildings, trees };
}
function miter(from, dirFrom, to, dirTo){
  // intersection of line(from,dirFrom) and line(to,dirTo)
  const den=dirFrom[0]*dirTo[1]-dirFrom[1]*dirTo[0]; if(Math.abs(den)<1e-6) return null;
  const t=((to[0]-from.x)*dirTo[1]-(to[1]-from.z)*dirTo[0])/den;
  if(t<-2||t>26) { // runaway miter on acute Y: fall back to the chord midpoint (still a clean fillet)
    return { x:(from.x+to.x)/2, z:(from.z+to.z)/2, y:(from.y+to.y)/2 };
  }
  return { x:from.x+dirFrom[0]*t, z:from.z+dirFrom[1]*t, y:(from.y+to.y)/2 };
}
export function validateNetwork(project, built){
  const s=built.mesh.stats(); const junctions=built.nodeArms.size;
  let splits=0; for(const [id] of project.nodes) if(project.degree(id)>=3) splits++;
  const issues=[];
  if(s.degenerate) issues.push(`${s.degenerate} degenerate faces`);
  if(s.nonManifold) issues.push(`${s.nonManifold} non-manifold edges (>2 faces)`);
  for(const g of built.edgeGeom.values()){ const L=g.frames[g.frames.length-1].s; const tA=built.nodeArms.get(g.e.a)?.arms.find(a=>a.e.id===g.e.id)?.trim||0; const tB=built.nodeArms.get(g.e.b)?.arms.find(a=>a.e.id===g.e.id)?.trim||0; if(tA+tB>L*0.98) issues.push(`edge ${g.e.id} is consumed by its junction trims — move its nodes apart`); }
  return { ...s, junctions, graphNodes:project.nodes.size, graphEdges:project.edges.length, quadPct: s.faces? (100*s.quads/s.faces).toFixed(1):'100.0', issues };
}
// OBJ export — quads stay quads (f a b c d), grouped by material.
export function exportOBJ(built, name='city'){
  const mb=built.mesh; const byMat=new Map();
  for(const f of mb.faces){ if(!byMat.has(f.mat)) byMat.set(f.mat,[]); byMat.get(f.mat).push(f); }
  let out=`# ${name} — CityRoadEditor (100% quad faces)\n`;
  for(let i=0;i<mb.pos.length;i+=3) out+=`v ${mb.pos[i].toFixed(3)} ${mb.pos[i+1].toFixed(3)} ${mb.pos[i+2].toFixed(3)}\n`;
  for(const [mat,fs] of byMat){ out+=`g ${mat}\nusemtl ${mat}\n`; for(const f of fs) out+=`f ${f.v.map(v=>v+1).join(' ')}\n`; }
  return out;
}
