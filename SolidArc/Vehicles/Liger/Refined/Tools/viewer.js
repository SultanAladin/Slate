import * as THREE from 'three';
import {OrbitControls} from 'three/addons/controls/OrbitControls.js';
const $=s=>document.querySelector(s),canvas=$('#viewport');
const renderer=new THREE.WebGLRenderer({canvas,antialias:true,preserveDrawingBuffer:true});
renderer.setPixelRatio(Math.min(devicePixelRatio,1.5));renderer.setClearColor(0x0e1520);
const scene=new THREE.Scene(),camera=new THREE.PerspectiveCamera(36,1,.01,100);camera.up.set(0,0,1);
const controls=new OrbitControls(camera,canvas);controls.enableDamping=true;controls.dampingFactor=.12;
const uniforms={mode:{value:0}};
const material=new THREE.ShaderMaterial({side:THREE.DoubleSide,uniforms,
vertexShader:`attribute float deviation;attribute float change;attribute float panel;attribute float part;varying vec3 n;varying float e;varying float c;varying float f;varying float p;
void main(){n=normalMatrix*normal;e=deviation;c=change;f=panel;p=part;gl_Position=projectionMatrix*modelViewMatrix*vec4(position,1.);}`,
fragmentShader:`precision highp float;uniform int mode;varying vec3 n;varying float e;varying float c;varying float f;varying float p;
void main(){vec3 N=normalize(n);if(!gl_FrontFacing)N=-N;float light=max(0.,dot(N,normalize(vec3(-.35,.55,.76))));float shade=.24+.65*light+.16*pow(max(0.,N.z),24.);vec3 col=p<.5?vec3(.72,.79,.84):p<1.5?vec3(.37,.44,.51):vec3(.17,.23,.31);
if(mode==1)col=.3+.6*fract(sin(vec3(17.1,33.7,57.3)*(f+1.))*43758.54);
if(mode==2||mode==4){float t=clamp((mode==4?c/5.:e/2.),0.,1.);col=vec3(clamp(4.*t-2.,0.,1.),clamp(2.-abs(4.*t-2.),0.,1.),clamp(2.-4.*t,0.,1.));if(mode==2&&e>5.)col=vec3(1.,.1,.65);shade=.78+.2*light;}
if(mode==3){float stripe=sin((N.x*.9+N.z*.3+N.y*.15)*100.);col=vec3(smoothstep(-.04,.04,stripe)*.94+.04);shade=.9;}gl_FragColor=vec4(col*shade,1.);}`});
let revisions={},active='after',center,radius,source;
function current(){return revisions[active];}
function view(v,target=center,scale=radius*2.9){if(!center)return;camera.position.copy(target).add(new THREE.Vector3(...v).normalize().multiplyScalar(scale*Math.max(1,1/camera.aspect)));controls.target.copy(target);controls.update();}
function resize(){const r=canvas.getBoundingClientRect();renderer.setSize(r.width,r.height,false);camera.aspect=r.width/r.height;camera.updateProjectionMatrix();}
window.addEventListener('resize',resize);resize();
function update(){for(const [key,r] of Object.entries(revisions)){r.group.visible=key===active;for(const m of r.meshes)m.visible=document.querySelector(`[data-part="${m.userData.part}"]`).checked;r.lines.visible=($('#edges').checked||uniforms.mode.value===1)&&r.meshes.every(m=>m.visible);}$('#status').textContent=`${active==='after'?'306 refined':'312 previous'} faces · shared-edge tessellation`;}
async function compressed(url){const response=await fetch(url);if(!response.ok)throw Error(`${url}: ${response.status}`);if(!globalThis.DecompressionStream)throw Error('Use a current browser supporting gzip DecompressionStream');return new Response(response.body.pipeThrough(new DecompressionStream('gzip'))).arrayBuffer();}
async function load(which){const [buffer,meta]=await Promise.all([compressed(`${which}.bin.gz`),fetch(`${which}_metadata.json`).then(r=>{if(!r.ok)throw Error('Missing panel metadata');return r.json();})]);const h=new Uint32Array(buffer,0,3),nv=h[0],ni=h[1],nl=h[2],data=new Float32Array(buffer,12,nv*10);let off=12+nv*40;const index=new Uint32Array(buffer,off,ni);off+=ni*4;const segments=new Float32Array(buffer,off,nl*6);
 const arrays={position:new Float32Array(nv*3),normal:new Float32Array(nv*3),deviation:new Float32Array(nv),panel:new Float32Array(nv),part:new Float32Array(nv),change:new Float32Array(nv)};
 for(let i=0;i<nv;i++){arrays.position.set(data.subarray(i*10,i*10+3),i*3);arrays.normal.set(data.subarray(i*10+3,i*10+6),i*3);for(const [a,j] of [['deviation',6],['panel',7],['part',8],['change',9]])arrays[a][i]=data[i*10+j];}
 const group=new THREE.Group(),meshes=[];
 const attributes=Object.fromEntries(Object.entries(arrays).map(([k,v])=>[k,new THREE.BufferAttribute(v,['position','normal'].includes(k)?3:1)]));
 for(let p=0;p<3;p++){const g=new THREE.BufferGeometry();for(const [k,a] of Object.entries(attributes))g.setAttribute(k,a);const ids=[];for(let i=0;i<ni;i+=3)if(arrays.part[index[i]]===p)ids.push(index[i],index[i+1],index[i+2]);g.setIndex(ids);const m=new THREE.Mesh(g,material);m.userData.part=p;group.add(m);meshes.push(m);}
 const lg=new THREE.BufferGeometry();lg.setAttribute('position',new THREE.BufferAttribute(segments,3));const lines=new THREE.LineSegments(lg,new THREE.LineBasicMaterial({color:0x24394a,transparent:true,opacity:.65}));group.add(lines);scene.add(group);return {group,meshes,lines,meta,position:attributes.position};}
(async()=>{try{
 const after=await load('after');revisions.after=after;$('#loading').textContent='Loading previous CAD with matched shared-edge tessellation…';revisions.before=await load('before');
 const box=new THREE.Box3().setFromBufferAttribute(after.position);center=box.getCenter(new THREE.Vector3());radius=box.getSize(new THREE.Vector3()).length()/2;update();view([7,-9,5]);document.querySelectorAll('aside input').forEach(i=>i.disabled=false);$('#loading').hidden=true;
 document.querySelectorAll('[data-view]').forEach(b=>b.onclick=()=>view(JSON.parse(b.dataset.view)));
 document.querySelectorAll('[data-region]').forEach(b=>b.onclick=()=>{const r={upper:[[-.85,0,.94],[-4,-9,7],2.7],sill:[[.3,-1.1,.44],[1,-10,2],2.7],arch:[[2.17,-1.04,.6],[5,-10,4],1.15]}[b.dataset.region];view(r[1],new THREE.Vector3(...r[0]),r[2]);});
 document.querySelectorAll('[name=mode]').forEach(b=>b.onchange=()=>{uniforms.mode.value=+b.value;$('#heatlegend').hidden=![2,4].includes(+b.value);$('#heatlegend').innerHTML=+b.value===4?'Change from previous CAD<div class="gradient"></div>0 mm <span style="float:right">≥5 mm</span><div>Nearest previous CAD region · sampled</div>':'Distance to polygon source<div class="gradient"></div>0 mm <span style="float:right">≥2 mm</span><div>Magenta >5 mm · NOT a smoothness score</div>';update();});
 $('#before').onchange=()=>{active=$('#before').checked?'before':'after';$('#revision').textContent=active==='before'?'BEFORE / previous faired CAD':'AFTER / strips + clearance refined';if(active==='before'&&uniforms.mode.value===4)document.querySelector('[name=mode][value="3"]').click();document.querySelector('[name=mode][value="4"]').disabled=active==='before';$('#selection').textContent='Click a panel to inspect the displayed CAD revision.';update();};
 $('#edges').onchange=update;document.querySelectorAll('[data-part]').forEach(b=>b.onchange=update);
 $('#reference').onchange=async()=>{try{if(!source){const response=await fetch('../../Proofs/reference_points.bin');if(!response.ok)throw Error('Missing source overlay');const buffer=await response.arrayBuffer();const g=new THREE.BufferGeometry();g.setAttribute('position',new THREE.BufferAttribute(new Float32Array(buffer),3));source=new THREE.Points(g,new THREE.PointsMaterial({color:0xff9f70,size:.007,transparent:true,opacity:.5,depthTest:false}));scene.add(source);}source.visible=$('#reference').checked;}catch(e){$('#reference').checked=false;$('#status').textContent=e.message;console.error(e);}};
 $('#screenshot').onclick=()=>{renderer.render(scene,camera);const a=document.createElement('a');a.download='Liger_refined_inspection.png';a.href=canvas.toDataURL();a.click();};
 const ray=new THREE.Raycaster(),pointer=new THREE.Vector2();let down;
 canvas.onpointerdown=e=>down=[e.clientX,e.clientY];canvas.onpointerup=e=>{if(!down||Math.hypot(e.clientX-down[0],e.clientY-down[1])>5)return;const rect=canvas.getBoundingClientRect();pointer.set((e.clientX-rect.left)/rect.width*2-1,-(e.clientY-rect.top)/rect.height*2+1);ray.setFromCamera(pointer,camera);const hits=ray.intersectObjects(current().meshes.filter(m=>m.visible));if(!hits.length)return;const hit=hits[0],id=hit.object.geometry.getAttribute('panel').getX(hit.face.a),p=current().meta.faces[id];$('#selection').innerHTML=`<strong>${p.name.replaceAll('_',' ')}</strong><div class="mini">${p.mirror?'Left':'Right'} · patch ${p.patch} (0-based)</div><dl><dt>Previous patches</dt><dd>${p.source_ids.join(' + ')}</dd><dt>Control net</dt><dd>${p.poles.join(' × ')}</dd><dt>Source distance RMS</dt><dd>${p.rms.toFixed(3)} mm</dd><dt>Source distance max</dt><dd>${p.max.toFixed(3)} mm</dd></dl>`;};
 }catch(e){$('#loading').textContent='Could not load geometry: '+e.message;console.error(e);}})();
renderer.setAnimationLoop(()=>{controls.update();renderer.render(scene,camera);});
