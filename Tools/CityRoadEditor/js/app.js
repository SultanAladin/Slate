import * as THREE from 'three';
import { OrbitControls } from '../vendor/OrbitControls.js';
import { Project, Terrain, buildNetwork, validateNetwork, exportOBJ, WORLD, sampleCurve } from './core.js';
import { PRESETS, resolvePreset, asphaltHalf, totalHalf } from './presets.js';
import { buildDemoCity } from './demo.js';

const $=s=>document.querySelector(s);
let project = buildDemoCity();
let tool='select', curPreset='collector';
let selectedEdge=null, selectedNode=null, draft=null, draftKind='road';
let history=[];
let built=null;
let cam2d={x:0,z:0,scale:0.62};
let mouse={x:0,z:0,inside:false};

// ---------------- history ----------------
function snapshot(){ history.push(JSON.stringify(project.toJSON())); if(history.length>60) history.shift(); }
function undo(){ const s=history.pop(); if(!s) return; project=Project.fromJSON(JSON.parse(s)); selectedEdge=null; rebuild('Undone.'); }
$('#btnUndo').onclick=undo;
addEventListener('keydown',e=>{ if((e.ctrlKey||e.metaKey)&&e.key==='z') undo(); if(e.key==='Enter'&&draft) finishDraft(); if(e.key==='Escape'){draft=null;draw2d();} if(e.key==='Delete'&&selectedEdge){ snapshot(); project.deleteEdge(selectedEdge); selectedEdge=null; rebuild('Road deleted.'); } });

// ---------------- 3D scene ----------------
const renderer=new THREE.WebGLRenderer({antialias:true, preserveDrawingBuffer:true});
renderer.setPixelRatio(Math.min(devicePixelRatio,2)); renderer.shadowMap.enabled=true; renderer.shadowMap.type=THREE.PCFSoftShadowMap;
$('#gl').appendChild(renderer.domElement);
const scene=new THREE.Scene(); scene.background=new THREE.Color(0x87b5d9); scene.fog=new THREE.Fog(0x9dbfd9, 700, 1800);
const camera=new THREE.PerspectiveCamera(50,1,0.5,4000); camera.position.set(-260,300,420);
const controls=new OrbitControls(camera,renderer.domElement); controls.target.set(40,0,70); controls.maxPolarAngle=Math.PI*0.49; controls.enableDamping=true;
scene.add(new THREE.HemisphereLight(0xcfe6ff,0x3d5231,1.05));
const sun=new THREE.DirectionalLight(0xfff2d8,2.2); sun.position.set(-320,420,180); sun.castShadow=true; sun.shadow.mapSize.set(2048,2048); Object.assign(sun.shadow.camera,{left:-560,right:560,top:460,bottom:-460,far:1400}); sun.shadow.camera.updateProjectionMatrix(); scene.add(sun);

const MATS={
  asphalt:new THREE.MeshStandardMaterial({color:0x34383f,roughness:.94}), asphaltDark:new THREE.MeshStandardMaterial({color:0x272a2f,roughness:.94}),
  asphaltLight:new THREE.MeshStandardMaterial({color:0x41464e,roughness:.95}), asphaltOld:new THREE.MeshStandardMaterial({color:0x514a41,roughness:.97}),
  asphaltJunction:new THREE.MeshStandardMaterial({color:0x30343b,roughness:.94}),
  dirt:new THREE.MeshStandardMaterial({color:0x8a6a48,roughness:1}), shoulder:new THREE.MeshStandardMaterial({color:0x7c6a52,roughness:1}),
  pavement:new THREE.MeshStandardMaterial({color:0x9aa0a3,roughness:.95}), curb:new THREE.MeshStandardMaterial({color:0xb9bcbe,roughness:.9}),
  median:new THREE.MeshStandardMaterial({color:0x5d8a4a,roughness:1}), concretePier:new THREE.MeshStandardMaterial({color:0x8f9296,roughness:.85}),
  railMetal:new THREE.MeshStandardMaterial({color:0xd7dde2,roughness:.35,metalness:.7}), railSteel:new THREE.MeshStandardMaterial({color:0xc9d2d9,roughness:.25,metalness:.85}),
  ballast:new THREE.MeshStandardMaterial({color:0x5c5c58,roughness:1}), monorailBeam:new THREE.MeshStandardMaterial({color:0xb9bdc0,roughness:.8}),
  markWhite:new THREE.MeshStandardMaterial({color:0xf2f4f2,roughness:.6}), markYellow:new THREE.MeshStandardMaterial({color:0xf2c230,roughness:.6}),
  bikeGreen:new THREE.MeshStandardMaterial({color:0x2f9e63,roughness:.7}), asphaltBump:new THREE.MeshStandardMaterial({color:0x3a3e45,roughness:.9}),
};
const MARK_MATS=new Set(['markWhite','markYellow','bikeGreen','asphaltBump']);
let cityGroup=null;

function decalTexture(kind){
  const c=document.createElement('canvas'); c.width=c.height=128; const g=c.getContext('2d'); g.clearRect(0,0,128,128); g.fillStyle='#fff'; g.strokeStyle='#fff';
  if(kind==='arrow'){ g.beginPath(); g.moveTo(64,8); g.lineTo(104,58); g.lineTo(76,58); g.lineTo(76,120); g.lineTo(52,120); g.lineTo(52,58); g.lineTo(24,58); g.closePath(); g.fill(); }
  else if(kind==='bike'){ g.lineWidth=7; g.beginPath(); g.arc(38,88,20,0,7); g.arc(92,88,20,0,7); g.stroke(); g.lineWidth=6; g.beginPath(); g.moveTo(38,88); g.lineTo(58,58); g.lineTo(88,58); g.lineTo(92,88); g.moveTo(58,58); g.lineTo(70,88); g.moveTo(52,50); g.lineTo(66,50); g.stroke(); }
  else if(kind==='chevron'){ g.fillStyle='#f2c230'; for(let i=-2;i<4;i++){ g.save(); g.translate(i*36,0); g.rotate(0.6); g.fillRect(0,-30,16,190); g.restore(); } g.fillStyle='rgba(0,0,0,.25)'; }
  else if(kind==='bus'){ g.fillStyle='#f2c230'; g.font='bold 30px sans-serif'; g.textAlign='center'; g.fillText('BUS',64,52); g.fillText('STOP',64,90); }
  const t=new THREE.CanvasTexture(c); t.anisotropy=4; return t;
}
const DECAL_TEX={}; ['arrow','bike','chevron','bus'].forEach(k=>DECAL_TEX[k]=decalTexture(k));
const windowTex=(()=>{ const c=document.createElement('canvas'); c.width=64;c.height=128; const g=c.getContext('2d'); g.fillStyle='#6b7280'; g.fillRect(0,0,64,128); for(let y=6;y<122;y+=10) for(let x=5;x<58;x+=10){ g.fillStyle=Math.random()>0.45?'#ffd98a':'#2c3e50'; g.fillRect(x,y,6,6);} const t=new THREE.CanvasTexture(c); return t; })();

function terrainMesh(){
  const t=project.terrain, nx=t.nx, nd=t.nd; const geo=new THREE.BufferGeometry(); const pos=[],col=[],idx=[];
  const cGrass=new THREE.Color(0x63904e), cDry=new THREE.Color(0x9aa063), cRock=new THREE.Color(0x7d7a72), cSand=new THREE.Color(0xc2b088), cDeep=new THREE.Color(0x51683c);
  for(let j=0;j<nd;j++) for(let i=0;i<nx;i++){ const x=t.x(i),z=t.z(j),h=t.h[j*nx+i]; pos.push(x,h-0.12,z); const c= h<WORLD.waterY+0.5?cSand : h>16?cRock : h>7?cDry : (Math.sin(x*0.05)*Math.cos(z*0.045)>0.35?cDeep:cGrass); col.push(c.r,c.g,c.b); }
  for(let j=0;j<nd-1;j++) for(let i=0;i<nx-1;i++){ const a=j*nx+i,b=a+1,c=a+nx,d=c+1; idx.push(a,c,b,b,c,d); }
  geo.setAttribute('position',new THREE.Float32BufferAttribute(pos,3)); geo.setAttribute('color',new THREE.Float32BufferAttribute(col,3)); geo.setIndex(idx); geo.computeVertexNormals();
  const m=new THREE.Mesh(geo,new THREE.MeshStandardMaterial({vertexColors:true,roughness:1,side:THREE.DoubleSide})); m.receiveShadow=true; return m;
}

function buildThree(){
  if(cityGroup){ scene.remove(cityGroup); cityGroup.traverse(o=>{ if(o.geometry) o.geometry.dispose(); }); }
  cityGroup=new THREE.Group();
  cityGroup.add(terrainMesh());
  if($('#lyrWater').checked){ const w=new THREE.Mesh(new THREE.PlaneGeometry(WORLD.w,WORLD.d), new THREE.MeshStandardMaterial({color:0x2e7fc2,transparent:true,opacity:.82,roughness:.15,metalness:.1})); w.rotation.x=-Math.PI/2; w.position.y=WORLD.waterY; w.receiveShadow=true; w.name='water'; cityGroup.add(w); }
  // road quads -> one mesh per material (GPU triangulates quads internally; source faces stay quads)
  const byMat=new Map(); for(const f of built.mesh.faces){ if(!byMat.has(f.mat)) byMat.set(f.mat,[]); byMat.get(f.mat).push(f); }
  const P=built.mesh.pos;
  for(const [mat,faces] of byMat){
    if(!$('#lyrMarks').checked && MARK_MATS.has(mat)) continue;
    if(!$('#lyrRails').checked && ['ballast','railSteel','monorailBeam'].includes(mat)) continue;
    const idx=[]; for(const f of faces){ idx.push(f.v[0],f.v[2],f.v[1], f.v[0],f.v[3],f.v[2]); }
    const g=new THREE.BufferGeometry(); g.setAttribute('position',new THREE.Float32BufferAttribute(P.slice(),3)); g.setIndex(idx); g.computeVertexNormals();
    const m=new THREE.Mesh(g, MATS[mat]||MATS.asphalt); m.castShadow=false; m.receiveShadow=true; cityGroup.add(m);
  }
  // decals
  if($('#lyrMarks').checked) for(const d of built.mesh.decals){ const m=new THREE.Mesh(new THREE.PlaneGeometry(d.size,d.kind==='bus'?d.size*1.6:d.size*1.4), new THREE.MeshBasicMaterial({map:DECAL_TEX[d.kind],transparent:true,depthWrite:false})); m.rotation.set(-Math.PI/2,0,-d.rot+Math.PI/2); m.position.set(d.x,d.y,d.z); m.renderOrder=3; cityGroup.add(m); }
  // bridge piers (boxes from core)
  if(built.mesh.boxes.length){ const inst=new THREE.InstancedMesh(new THREE.BoxGeometry(1,1,1), MATS.concretePier, built.mesh.boxes.length); const M=new THREE.Matrix4(),Q=new THREE.Quaternion(),E=new THREE.Euler(),S=new THREE.Vector3();
    built.mesh.boxes.forEach((b,i)=>{ E.set(0,-b.rot,0); Q.setFromEuler(E); S.set(b.sx,Math.max(0.5,b.sy),b.sz); M.compose(new THREE.Vector3(b.cx,b.cy,b.cz),Q,S); inst.setMatrixAt(i,M); }); inst.castShadow=true; cityGroup.add(inst); }
  // sleepers + pylons
  if($('#lyrRails').checked){
    if(built.sleepers.length){ const inst=new THREE.InstancedMesh(new THREE.BoxGeometry(2.3,0.12,0.55), new THREE.MeshStandardMaterial({color:0x4a3a28,roughness:1}), built.sleepers.length); const M=new THREE.Matrix4(),Q=new THREE.Quaternion(),E=new THREE.Euler();
      built.sleepers.forEach((s,i)=>{ E.set(0,-s.rot,0); Q.setFromEuler(E); M.compose(new THREE.Vector3(s.x,s.y,s.z),Q,new THREE.Vector3(1,1,1)); inst.setMatrixAt(i,M); }); cityGroup.add(inst); }
    if(built.pylons.length){ const inst=new THREE.InstancedMesh(new THREE.CylinderGeometry(0.55,0.75,1,8), MATS.concretePier, built.pylons.length); const M=new THREE.Matrix4();
      built.pylons.forEach((s,i)=>{ const h=Math.max(1,s.top-s.ground); M.makeScale(1,h,1).setPosition(s.x,s.ground+h/2,s.z); inst.setMatrixAt(i,M); }); inst.castShadow=true; cityGroup.add(inst); }
  }
  // buildings: simple blocks with lit-window texture (AAA dressing comes from roads/terrain; buildings stay clean blocks)
  if($('#lyrBuildings').checked){ const geo=new THREE.BoxGeometry(1,1,1);
    built.buildings.forEach(b=>{ const mat=new THREE.MeshStandardMaterial({map:windowTex,color:new THREE.Color().setHSL(0.08+b.tone*0.06,0.12,0.55+b.tone*0.25),roughness:.85}); const m=new THREE.Mesh(geo,mat); m.position.set(b.x,b.y+b.h/2-0.2,b.z); m.scale.set(b.w,b.h,b.d); m.rotation.y=-b.rot; m.castShadow=true; m.receiveShadow=true; cityGroup.add(m);
      const roof=new THREE.Mesh(geo,new THREE.MeshStandardMaterial({color:0x3c4147})); roof.position.set(b.x,b.y+b.h+0.15,b.z); roof.scale.set(b.w*1.02,0.3,b.d*1.02); roof.rotation.y=-b.rot; cityGroup.add(roof); }); }
  // low-poly trees: trunk + icosahedron canopy, instanced
  if($('#lyrTrees').checked && built.trees.length){
    const trunk=new THREE.InstancedMesh(new THREE.CylinderGeometry(0.14,0.2,1.6,5), new THREE.MeshStandardMaterial({color:0x6b4a2e,roughness:1}), built.trees.length);
    const canopy=new THREE.InstancedMesh(new THREE.IcosahedronGeometry(1.7,0), new THREE.MeshStandardMaterial({color:0x3f7a36,roughness:1,flatShading:true}), built.trees.length);
    const M=new THREE.Matrix4(),Q=new THREE.Quaternion(),E=new THREE.Euler();
    built.trees.forEach((t,i)=>{ E.set(0,i*1.7,0); Q.setFromEuler(E); M.compose(new THREE.Vector3(t.x,t.y+0.8,t.z),Q,new THREE.Vector3(t.s,t.s,t.s)); trunk.setMatrixAt(i,M); M.compose(new THREE.Vector3(t.x,t.y+1.6+1.3*t.s,t.z),Q,new THREE.Vector3(t.s,t.s*1.15,t.s)); canopy.setMatrixAt(i,M); });
    trunk.castShadow=canopy.castShadow=true; cityGroup.add(trunk,canopy);
  }
  scene.add(cityGroup);
}

// ---------------- validation / inspector ----------------
function rebuild(status){
  const t0=performance.now();
  built=buildNetwork(project,{buildings:$('#lyrBuildings').checked, trees:$('#lyrTrees').checked});
  buildThree(); draw2d(); renderInspector();
  const v=validateNetwork(project,built);
  $('#topology').innerHTML=
    `<div class="kv"><span>Quad faces</span><b>${v.quads.toLocaleString()}</b></div>`+
    `<div class="kv"><span>Quad purity</span><b>${v.quadPct}%</b></div>`+
    `<div class="kv"><span>Triangles / n-gons</span><b>${v.tris} / ${v.ngons}</b></div>`+
    `<div class="kv"><span>Degenerate faces</span><b>${v.degenerate}</b></div>`+
    `<div class="kv"><span>Non-manifold edges</span><b>${v.nonManifold}</b></div>`+
    `<div class="kv"><span>Welded vertices</span><b>${v.verts.toLocaleString()}</b></div>`+
    `<div class="kv"><span>Merged junctions</span><b>${v.junctions}</b></div>`+
    `<div class="kv"><span>Graph</span><b>${v.graphNodes} nodes · ${v.graphEdges} roads · ${project.tracks.length} tracks</b></div>`+
    `<div class="kv"><span>Buildings / trees</span><b>${built.buildings.length} / ${built.trees.length}</b></div>`+
    (v.issues.length? `<div class="bad">⚠ ${v.issues.join('<br>⚠ ')}</div>` : `<div><b>✓ No overlaps, no degenerate topology, every junction merged.</b></div>`);
  if(status) $('#status').textContent=status+`  (rebuild ${Math.round(performance.now()-t0)} ms)`;
}
function renderInspector(){
  const el=$('#inspector');
  if(selectedNode){ const d=project.degree(selectedNode); el.innerHTML=`<div class="kv"><span>Node</span><b>#${selectedNode}</b></div><div class="kv"><span>Arms (degree)</span><b>${d}</b></div><p class="hint">${d>=3?'Merged junction patch — filleted, all-quad hub.':'Endpoint / bend node.'} Drag it in 2D to move.</p>`; return; }
  if(!selectedEdge||!project.edges.find(e=>e.id===selectedEdge)){ el.innerHTML='<p class="hint">Select a road in 2D to change its type, lanes and furniture.</p>'; return; }
  const e=project.edges.find(x=>x.id===selectedEdge), p=resolvePreset(e);
  el.innerHTML=`
   <div class="kv"><span>Road</span><b>#${e.id}</b></div>
   <label>Type / preset<select id="inspPreset">${Object.entries(PRESETS).map(([k,v])=>`<option value="${k}" ${k===e.preset?'selected':''}>${v.label}</option>`).join('')}</select></label>
   <label>Lanes (total) <span id="inspLanesVal">${p.lanes}</span><input type="range" id="inspLanes" min="1" max="6" step="1" value="${p.lanes}"></label>
   <label>Median width (m)<input type="range" id="inspMedian" min="0" max="6" step="0.5" value="${p.median||0}"></label>
   <label>Sidewalk / pavement (m)<input type="range" id="inspWalk" min="0" max="4" step="0.2" value="${p.sidewalk||0}"></label>
   <label>Bike lane (m)<input type="range" id="inspBike" min="0" max="2.4" step="0.2" value="${p.bike||0}"></label>
   <label>Parking (m)<input type="range" id="inspPark" min="0" max="2.6" step="0.2" value="${p.parking||0}"></label>
   <label><input type="checkbox" id="inspBump" ${p.bump?'checked':''}> Speed bumps</label>
   <div class="kv"><span>Asphalt half-width</span><b>${asphaltHalf(p).toFixed(1)} m</b></div>
   <div class="kv"><span>Total corridor</span><b>${(totalHalf(p)*2).toFixed(1)} m</b></div>
   <button id="inspDelete">Delete this road</button>`;
  const apply=(patch)=>{ snapshot(); Object.assign(e.overrides=e.overrides||{}, patch); rebuild('Road updated — junctions re-merged.'); };
  $('#inspPreset').onchange=ev=>{ snapshot(); e.preset=ev.target.value; e.overrides={}; rebuild('Preset applied.'); };
  $('#inspLanes').oninput=ev=>{ $('#inspLanesVal').textContent=ev.target.value; apply({lanes:+ev.target.value}); };
  $('#inspMedian').onchange=ev=>apply({median:+ev.target.value});
  $('#inspWalk').onchange=ev=>apply({sidewalk:+ev.target.value});
  $('#inspBike').onchange=ev=>apply({bike:+ev.target.value});
  $('#inspPark').onchange=ev=>apply({parking:+ev.target.value});
  $('#inspBump').onchange=ev=>apply({bump:ev.target.checked});
  $('#inspDelete').onclick=()=>{ snapshot(); project.deleteEdge(e.id); selectedEdge=null; rebuild('Road deleted.'); };
}

// ---------------- preset library UI ----------------
const presetsEl=$('#presets');
Object.entries(PRESETS).forEach(([key,p])=>{
  const b=document.createElement('button'); b.className='preset'+(key===curPreset?' on':'');
  const total=(p.lanes*p.laneW+(p.median||0)+(p.bike||0)*2+(p.parking||0)*2).toFixed(0);
  b.innerHTML=`${p.label}<small>${p.lanes} lanes · ${total} m asphalt${p.sidewalk?` · ${p.sidewalk} m pavements`:''}${p.bike?' · bike':''}${p.parking?' · parking':''}</small><div class="diagram">${Array.from({length:Math.max(0,p.lanes-1)},(_,i)=>`<i style="left:${(i+1)*100/(p.lanes)}%"></i>`).join('')}</div>`;
  b.onclick=()=>{ curPreset=key; document.querySelectorAll('.preset').forEach(x=>x.classList.remove('on')); b.classList.add('on'); if(selectedEdge){ const e=project.edges.find(x=>x.id===selectedEdge); if(e){ snapshot(); e.preset=key; e.overrides={}; rebuild('Preset applied to selected road.'); } } else $('#status').textContent='Preset armed: '+p.label+' — now draw with ✏️ Draw road.'; };
  presetsEl.appendChild(b);
});
document.querySelectorAll('#tools button').forEach(b=>b.onclick=()=>{ tool=b.dataset.tool; draft=null; document.querySelectorAll('#tools button').forEach(x=>x.classList.toggle('on',x===b));
  $('#toolHint').textContent={select:'Drag a gold node to move a junction. Click a road to inspect it.',road:'Click to add points, double-click / Enter to finish. Crossing any road auto-creates a merged junction; ending on a road splits it (T/Y).',rail:'Click points, double-click to finish the rail line. It cuts across roads on level crossings and bridges the river.',monorail:'Click points for an elevated monorail (beam + pylons auto-generated).',raise:'Drag to raise terrain — roads re-grade, water floods low ground.',lower:'Drag to lower terrain / carve a valley or lake.',flatten:'Drag to flatten terrain to the clicked height.',delete:'Click a road or track to delete it.'}[tool]; });

// ---------------- 2D editor ----------------
const cv=$('#cv2d'), ctx=cv.getContext('2d');
function resize(){ const r=cv.parentElement.getBoundingClientRect(); cv.width=r.width*devicePixelRatio; cv.height=r.height*devicePixelRatio; const g=$('#gl').getBoundingClientRect(); renderer.setSize(g.width,g.height); camera.aspect=g.width/g.height; camera.updateProjectionMatrix(); draw2d(); }
new ResizeObserver(resize).observe($('#center'));
const W2S=(x,z)=>[ (x-cam2d.x)*cam2d.scale + cv.width/devicePixelRatio/2, (z-cam2d.z)*cam2d.scale + cv.height/devicePixelRatio/2 ];
const S2W=(px,py)=>[ (px-cv.width/devicePixelRatio/2)/cam2d.scale+cam2d.x, (py-cv.height/devicePixelRatio/2)/cam2d.scale+cam2d.z ];
function draw2d(){
  if(!cv.width) return; const W=cv.width/devicePixelRatio,H=cv.height/devicePixelRatio; ctx.setTransform(devicePixelRatio,0,0,devicePixelRatio,0,0);
  ctx.fillStyle='#20301f'; ctx.fillRect(0,0,W,H);
  // terrain shading (coarse cells)
  const t=project.terrain, step=4;
  for(let j=0;j<t.nd-1;j+=step) for(let i=0;i<t.nx-1;i+=step){ const h=t.h[j*t.nx+i]; const [x,y]=W2S(t.x(i),t.z(j)); const s=(t.x(step)-t.x(0))*cam2d.scale+1;
    ctx.fillStyle= h<WORLD.waterY? '#2e6ea8' : h<WORLD.waterY+0.6?'#c2b088': h>16?'#7d7a72': h>7?'#77894f':'#597d43'; ctx.fillRect(x,y,s,s); }
  // parks
  for(const pk of project.parks){ const [x,y]=W2S(pk.x,pk.z); ctx.fillStyle='rgba(70,140,60,.55)'; ctx.beginPath(); ctx.arc(x,y,pk.r*cam2d.scale,0,7); ctx.fill(); }
  // buildings
  if(built&&$('#lyrBuildings').checked){ ctx.fillStyle='#aab4bf'; for(const b of built.buildings){ const [x,y]=W2S(b.x,b.z); ctx.fillRect(x-3,y-3,6,6); } }
  // roads: casing -> asphalt -> centreline (graph truth, no overdraw crossings: junctions drawn as filled circles)
  const drawRoad=(pts,width,fill)=>{ ctx.strokeStyle=fill; ctx.lineWidth=Math.max(1,width*cam2d.scale); ctx.lineJoin=ctx.lineCap='round'; ctx.beginPath(); pts.forEach((p,i)=>{ const [x,y]=W2S(p[0],p[1]); i?ctx.lineTo(x,y):ctx.moveTo(x,y); }); ctx.stroke(); };
  for(const e of project.edges){ const p=resolvePreset(e), pts=sampleCurve(project.edgePoints(e),7); const sel=e.id===selectedEdge;
    drawRoad(pts,totalHalf(p)*2,'#14161a'); drawRoad(pts,asphaltHalf(p)*2, sel?'#7a4a20':(p.dirt?'#8a6a48':'#3a3f46'));
    if(p.sidewalk) drawRoad(pts,(asphaltHalf(p)+p.sidewalk*0.5)*2,'rgba(170,175,178,.28)'); }
  // junctions merged disc
  for(const [id,n] of project.nodes){ if(project.degree(id)>=3){ const [x,y]=W2S(n.x,n.z); ctx.fillStyle='#30343b'; ctx.beginPath(); ctx.arc(x,y,9*cam2d.scale+2,0,7); ctx.fill(); } }
  for(const e of project.edges){ const p=resolvePreset(e); if(p.dirt||p.center==='none') continue; ctx.setLineDash(p.center==='doubleYellow'?[]:[7,6]); drawRoad(sampleCurve(project.edgePoints(e),7),0.0, p.center.includes('Yellow')||p.center==='doubleYellow'?'#e8c33a':'#dfe3e6'); ctx.setLineDash([]); }
  // tracks
  for(const tr of project.tracks){ const pts=sampleCurve(tr.points,7); drawRoad(pts, tr.kind==='monorail'?2.2:3.8, tr.kind==='monorail'?'#37c4d8':'#5c5c58'); ctx.setLineDash([3,5]); drawRoad(pts,0.4,'#dfe6ee'); ctx.setLineDash([]); }
  if(built&&$('#lyrTrees').checked){ ctx.fillStyle='#2f6e2a'; for(const tr of built.trees){ const [x,y]=W2S(tr.x,tr.z); ctx.beginPath(); ctx.arc(x,y,2.2,0,7); ctx.fill(); } }
  // nodes
  for(const [id,n] of project.nodes){ const [x,y]=W2S(n.x,n.z); ctx.fillStyle= id===selectedNode?'#fff':'#f2c230'; ctx.beginPath(); ctx.arc(x,y,id===selectedNode?6:3.6,0,7); ctx.fill(); ctx.strokeStyle='#111'; ctx.stroke(); }
  // draft
  if(draft&&draft.length){ ctx.strokeStyle='#37c4d8'; ctx.lineWidth=2; ctx.setLineDash([6,4]); ctx.beginPath(); draft.forEach((p,i)=>{ const [x,y]=W2S(p[0],p[1]); i?ctx.lineTo(x,y):ctx.moveTo(x,y); }); const [mx,my]=W2S(mouse.x,mouse.z); ctx.lineTo(mx,my); ctx.stroke(); ctx.setLineDash([]); draft.forEach(p=>{ const [x,y]=W2S(p[0],p[1]); ctx.fillStyle='#37c4d8'; ctx.fillRect(x-3,y-3,6,6); }); }
  // snap indicator
  if(mouse.inside&&(tool==='road')){ const nn=project.findNodeNear([mouse.x,mouse.z]); const ee=!nn&&project.findEdgeNear([mouse.x,mouse.z]); if(nn||ee){ const [x,y]=W2S(mouse.x,mouse.z); ctx.strokeStyle='#7ee2a0'; ctx.lineWidth=2; ctx.beginPath(); ctx.arc(x,y,10,0,7); ctx.stroke(); } }
}
let dragNode=null, panning=null, brushing=false, moved=false;
cv.addEventListener('contextmenu',e=>e.preventDefault());
cv.addEventListener('wheel',e=>{ e.preventDefault(); const f=e.deltaY<0?1.12:0.89; cam2d.scale=Math.max(0.18,Math.min(4,cam2d.scale*f)); draw2d(); },{passive:false});
cv.addEventListener('pointerdown',e=>{ cv.setPointerCapture(e.pointerId); const r=cv.getBoundingClientRect(); const [wx,wz]=S2W(e.clientX-r.left,e.clientY-r.top); moved=false;
  if(e.button===1||e.button===2){ panning={x:e.clientX,z:e.clientY,cx:cam2d.x,cz:cam2d.z}; return; }
  if(tool==='select'){ const nn=project.findNodeNear([wx,wz],12/Math.max(1,cam2d.scale)); if(nn){ dragNode=nn; selectedNode=nn; snapshot(); } else { const ee=project.findEdgeNear([wx,wz],10/Math.max(1,cam2d.scale)); selectedEdge=ee?ee.id:null; selectedNode=null; panning={x:e.clientX,z:e.clientY,cx:cam2d.x,cz:cam2d.z}; renderInspector(); draw2d(); } }
  else if(tool==='road'||tool==='rail'||tool==='monorail'){ if(!draft){ draft=[]; draftKind=tool; } draft.push([wx,wz]); draw2d(); }
  else if(tool==='delete'){ const ee=project.findEdgeNear([wx,wz],10); if(ee){ snapshot(); project.deleteEdge(ee.id); if(selectedEdge===ee.id) selectedEdge=null; rebuild('Road deleted.'); return; }
    let bi=-1,bd=12; project.tracks.forEach((tr,i)=>{ const pts=sampleCurve(tr.points,6); for(let k=0;k<pts.length;k++) { const d=Math.hypot(pts[k][0]-wx,pts[k][1]-wz); if(d<bd){bd=d;bi=i;} } }); if(bi>=0){ snapshot(); project.tracks.splice(bi,1); rebuild('Track deleted.'); } }
  else if(['raise','lower','flatten'].includes(tool)){ brushing=true; applyBrush(wx,wz); }
});
cv.addEventListener('pointermove',e=>{ const r=cv.getBoundingClientRect(); const [wx,wz]=S2W(e.clientX-r.left,e.clientY-r.top); mouse={x:wx,z:wz,inside:true}; moved=true;
  $('#coords').textContent=`x ${wx.toFixed(0)} · z ${wz.toFixed(0)} · terrain ${project.terrain.height(wx,wz).toFixed(1)} m`;
  if(panning){ cam2d.x=panning.cx-(e.clientX-panning.x)/cam2d.scale; cam2d.z=panning.cz-(e.clientY-panning.z)/cam2d.scale; draw2d(); return; }
  if(dragNode){ const n=project.getNode(dragNode); n.x=wx; n.z=wz; draw2d(); return; }
  if(brushing) applyBrush(wx,wz); else draw2d(); });
cv.addEventListener('pointerup',e=>{ if(dragNode){ dragNode=null; rebuild('Junction moved — topology re-merged.'); } if(brushing){ brushing=false; snapshot(); rebuild('Terrain edited — roads re-graded / bridges updated.'); } panning=null; });
cv.addEventListener('pointerleave',()=>{ mouse.inside=false; });
cv.addEventListener('dblclick',e=>{ if(draft) finishDraft(); });
function finishDraft(){ if(!draft||draft.length<2){ draft=null; draw2d(); return; } snapshot();
  if(draftKind==='road'){ const e=project.addRoad(draft,curPreset); if(e) selectedEdge=e.id; rebuild('Road added — crossings split & junctions merged.'); }
  else { project.tracks.push({kind:draftKind, points:draft}); rebuild(draftKind==='rail'?'Rail line added.':'Monorail added.'); }
  draft=null; }
let brushTimer=null;
function applyBrush(x,z){ const r=+$('#brushSize').value; if(tool==='raise') project.terrain.brush(x,z,r,0.55); else if(tool==='lower') project.terrain.brush(x,z,r,-0.55); else project.terrain.flatten(x,z,r,project.terrain.height(x,z)); draw2d(); clearTimeout(brushTimer); brushTimer=setTimeout(()=>rebuild('Terrain edited.'),180); }

// ---------------- header / views / misc ----------------
$('#btnDemo').onclick=()=>{ snapshot(); project=buildDemoCity(); selectedEdge=null; rebuild('Full city loaded: countryside → suburbs → city → offroad, river & lake bridges, rail + monorail.'); };
$('#btnNew').onclick=()=>{ snapshot(); project=new Project(); selectedEdge=null; cam2d={x:0,z:0,scale:0.62}; rebuild('Empty project — draw a road.'); };
$('#btnSave').onclick=()=>download(JSON.stringify(project.toJSON(),null,1), (project.name||'city')+'.roads.json','application/json');
$('#btnOBJ').onclick=()=>download(exportOBJ(built,project.name), (project.name||'city')+'.obj','text/plain');
$('#btnPNG').onclick=()=>{ renderer.render(scene,camera); const a=document.createElement('a'); a.href=renderer.domElement.toDataURL('image/png'); a.download='city-3d.png'; a.click(); };
$('#btnLoad').onclick=()=>$('#fileLoad').click();
$('#fileLoad').onchange=e=>{ const f=e.target.files[0]; if(!f) return; f.text().then(t=>{ snapshot(); project=Project.fromJSON(JSON.parse(t)); rebuild('Project opened.'); }); };
function download(text,name,type){ const a=document.createElement('a'); a.href=URL.createObjectURL(new Blob([text],{type})); a.download=name; a.click(); setTimeout(()=>URL.revokeObjectURL(a.href),4000); }
document.querySelectorAll('#viewBtns button').forEach(b=>b.onclick=()=>{ const v=b.dataset.view;
  if(v==='city'){ camera.position.set(-260,300,420); controls.target.set(40,0,70); }
  if(v==='bridge'){ camera.position.set(-40,55,-160); controls.target.set(45,4,-40); }
  if(v==='junction'){ camera.position.set(-95,70,110); controls.target.set(30,2,45); }
  if(v==='top'){ camera.position.set(40,780,75); controls.target.set(40,0,70); } });
['lyrBuildings','lyrTrees','lyrMarks','lyrRails','lyrWater'].forEach(id=>$('#'+id).onchange=()=>rebuild());
$('#brushSize').oninput=e=>$('#brushSizeVal').textContent=e.target.value+' m';

function loop(){ requestAnimationFrame(loop); controls.update(); const g=$('#gl').getBoundingClientRect(); if(g.width&&renderer.domElement.width!==Math.round(g.width*devicePixelRatio)){ renderer.setSize(g.width,g.height); camera.aspect=g.width/g.height; camera.updateProjectionMatrix(); } renderer.render(scene,camera); }

resize(); rebuild('Full city loaded: countryside → suburbs → city → offroad.'); loop();
