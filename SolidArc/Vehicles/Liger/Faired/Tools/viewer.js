import * as THREE from 'three';
import {OrbitControls} from 'three/addons/controls/OrbitControls.js';
const $=s=>document.querySelector(s),canvas=$('#viewport');
const renderer=new THREE.WebGLRenderer({canvas,antialias:true,preserveDrawingBuffer:true});
renderer.setPixelRatio(Math.min(devicePixelRatio,2));renderer.setClearColor(0x0e1520);
const scene=new THREE.Scene(),camera=new THREE.PerspectiveCamera(36,1,.01,100);
camera.up.set(0,0,1);const controls=new OrbitControls(camera,canvas);controls.enableDamping=true;controls.dampingFactor=.12;
const uniforms={mode:{value:0},errorScale:{value:2}};
const material=new THREE.ShaderMaterial({side:THREE.DoubleSide,uniforms,
vertexShader:`attribute float change;attribute float deviation;attribute float panel;attribute float part;varying vec3 n;varying vec3 worldN;varying float c;varying float e;varying float f;varying float p;
void main(){n=normalize(normalMatrix*normal);worldN=normal;e=deviation;c=change;f=panel;p=part;gl_Position=projectionMatrix*modelViewMatrix*vec4(position,1.);}`,
fragmentShader:`precision highp float;uniform int mode;uniform float errorScale;varying vec3 n;varying vec3 worldN;varying float c;varying float e;varying float f;varying float p;
void main(){vec3 N=normalize(n);if(!gl_FrontFacing)N=-N;float light=max(0.,dot(N,normalize(vec3(-.35,.55,.76))));float shade=.24+.65*light+.16*pow(max(0.,N.z),24.);vec3 col=p<.5?vec3(.72,.79,.84):p<1.5?vec3(.37,.44,.51):vec3(.17,.23,.31);
if(mode==1){col=.3+.6*fract(sin(vec3(17.1,33.7,57.3)*(f+1.))*43758.54);}
if(mode==2||mode==4){float value=mode==4?c:e;float scale=mode==4?8.:errorScale;float t=clamp(value/scale,0.,1.);col=vec3(clamp(4.*t-2.,0.,1.),clamp(2.-abs(4.*t-2.),0.,1.),clamp(2.-4.*t,0.,1.));if(mode==2&&e>5.)col=vec3(1.,.1,.65);shade=.78+.2*light;}
if(mode==3){float z=sin((N.x*.9+N.z*.3+N.y*.15)*100.);col=vec3(smoothstep(-.04,.04,z)*.94+.04);shade=.9;}
gl_FragColor=vec4(col*shade,1.);}`});
let meshes=[],edges,meta,box,center,radius,source;
function view(v){if(!center)return;camera.position.copy(center).add(new THREE.Vector3(...v).normalize().multiplyScalar(radius*2.9));controls.target.copy(center);controls.update();}
function resize(){const r=canvas.getBoundingClientRect();renderer.setSize(r.width,r.height,false);camera.aspect=r.width/r.height;camera.updateProjectionMatrix();}
window.addEventListener('resize',resize);resize();
(async()=>{try{
 const [buffer,metadata,changeBuffer]=await Promise.all([fetch('geometry.bin').then(r=>{if(!r.ok)throw Error('Geometry not found');return r.arrayBuffer()}),fetch('viewer_metadata.json').then(r=>r.json()),fetch('change.bin').then(r=>{if(!r.ok)throw Error('Change map not found');return r.arrayBuffer()})]);meta=metadata;
 const h=new Uint32Array(buffer,0,3),nv=h[0],ni=h[1],nl=h[2];const data=new Float32Array(buffer,12,nv*9);let off=12+nv*36;const index=new Uint32Array(buffer,off,ni);off+=ni*4;const line=new Float32Array(buffer,off,nl*6);
 const pos=new Float32Array(nv*3),nor=new Float32Array(nv*3),err=new Float32Array(nv),face=new Float32Array(nv),part=new Float32Array(nv);
 for(let i=0;i<nv;i++){pos.set(data.subarray(i*9,i*9+3),i*3);nor.set(data.subarray(i*9+3,i*9+6),i*3);err[i]=data[i*9+6];face[i]=data[i*9+7];part[i]=data[i*9+8];}
 for(let p=0;p<3;p++){const g=new THREE.BufferGeometry();g.setAttribute('position',new THREE.BufferAttribute(pos,3));g.setAttribute('normal',new THREE.BufferAttribute(nor,3));g.setAttribute('change',new THREE.BufferAttribute(new Float32Array(changeBuffer),1));g.setAttribute('deviation',new THREE.BufferAttribute(err,1));g.setAttribute('panel',new THREE.BufferAttribute(face,1));g.setAttribute('part',new THREE.BufferAttribute(part,1));let ids=[];for(let i=0;i<ni;i+=3)if(part[index[i]]===p)ids.push(index[i],index[i+1],index[i+2]);g.setIndex(ids);let mesh=new THREE.Mesh(g,material);mesh.userData.part=p;scene.add(mesh);meshes.push(mesh);}
 const lg=new THREE.BufferGeometry();lg.setAttribute('position',new THREE.BufferAttribute(line,3));edges=new THREE.LineSegments(lg,new THREE.LineBasicMaterial({color:0x24394a,transparent:true,opacity:.7,depthTest:true}));edges.visible=false;scene.add(edges);
 box=new THREE.Box3().setFromBufferAttribute(new THREE.BufferAttribute(pos,3));center=box.getCenter(new THREE.Vector3());radius=box.getSize(new THREE.Vector3()).length()/2;view([7,-9,5]);$('#loading').hidden=true;$('#status').textContent=`56 faired faces / 312 total · ${(ni/3).toLocaleString()} display triangles`;
 document.querySelectorAll('[data-view]').forEach(b=>b.onclick=()=>view(JSON.parse(b.dataset.view)));
 document.querySelectorAll('[name=mode]').forEach(b=>b.onchange=()=>{uniforms.mode.value=+b.value;edges.visible=$('#edges').checked||+b.value===1;$('#heatlegend').hidden=![2,4].includes(+b.value);$('#heatlegend').innerHTML=+b.value===4?'Change from original CAD<div class="gradient"></div>0 mm <span style="float:right">≥8 mm</span><div>Nearest original patch triangle · sampled</div>':'CAD → L3 source triangle<div class="gradient"></div>0 mm <span style="float:right">≥2 mm</span><div>Magenta >5 mm · not a smoothness measure</div>';});
 $('#edges').onchange=()=>edges.visible=$('#edges').checked;
 document.querySelectorAll('[data-part]').forEach(b=>b.onchange=()=>{meshes[+b.dataset.part].visible=b.checked;edges.visible=$('#edges').checked&&meshes.every(m=>m.visible);});
 const fairPosition=pos.slice(),fairNormal=nor.slice(),fairError=err.slice();let previous;
 $('#before').onchange=async()=>{try{
  $('#before').disabled=true;
  if($('#before').checked&&!previous){const response=await fetch('../../Proofs/geometry.bin');if(!response.ok)throw Error('Original geometry missing');const b=await response.arrayBuffer();const h0=new Uint32Array(b,0,3);if(h0[0]!==nv||h0[1]!==ni)throw Error('Original topology mismatch');previous=new Float32Array(b,12,nv*9);}
  if($('#before').checked){for(let i=0;i<nv;i++){pos.set(previous.subarray(i*9,i*9+3),i*3);nor.set(previous.subarray(i*9+3,i*9+6),i*3);err[i]=previous[i*9+6];}}else{pos.set(fairPosition);nor.set(fairNormal);err.set(fairError);}
  for(const m of meshes){for(const a of ['position','normal','deviation'])m.geometry.getAttribute(a).needsUpdate=true;m.geometry.computeBoundingSphere();}
  $('#revision').textContent=$('#before').checked?'BEFORE / original CAD':'AFTER / faired CAD';$('#selection').textContent='Click a panel to inspect the displayed revision.';
  if($('#before').checked&&uniforms.mode.value===4){document.querySelector('[name=mode][value="3"]').click();}
  document.querySelector('[name=mode][value="4"]').disabled=$('#before').checked;
 }catch(e){$('#before').checked=false;$('#status').textContent=e.message;console.error(e);}finally{$('#before').disabled=false;}};
 $('#screenshot').onclick=()=>{renderer.render(scene,camera);const a=document.createElement('a');a.download='Liger_inspection.png';a.href=canvas.toDataURL();a.click();};
 $('#reference').onchange=async()=>{if(!source){const b=await fetch('../../Proofs/reference_points.bin').then(r=>r.arrayBuffer());const g=new THREE.BufferGeometry();g.setAttribute('position',new THREE.BufferAttribute(new Float32Array(b),3));source=new THREE.Points(g,new THREE.PointsMaterial({color:0xff9f70,size:.007,transparent:true,opacity:.5,depthTest:false}));scene.add(source);}source.visible=$('#reference').checked;};
 const ray=new THREE.Raycaster(),pointer=new THREE.Vector2();let down;
 canvas.onpointerdown=e=>down=[e.clientX,e.clientY];
 canvas.onpointerup=e=>{if(!down||Math.hypot(e.clientX-down[0],e.clientY-down[1])>5)return;const r=canvas.getBoundingClientRect();pointer.set((e.clientX-r.left)/r.width*2-1,-(e.clientY-r.top)/r.height*2+1);ray.setFromCamera(pointer,camera);const hits=ray.intersectObjects(meshes.filter(m=>m.visible));if(!hits.length)return;const hit=hits[0],g=hit.object.geometry;const id=g.getAttribute('panel').getX(hit.face.a);const p=($('#before').checked?meta.before_faces:meta.faces)[id];$('#selection').innerHTML=`<strong>${p.name.replaceAll('_',' ')}</strong><div class="mini">${p.mirror?'Left':'Right'} · patch ${p.patch} (0-based)</div><dl><dt>Control net</dt><dd>${p.poles.join(' × ')}</dd><dt>Source distance RMS</dt><dd>${p.rms.toFixed(3)} mm</dd><dt>Source distance max</dt><dd>${p.max.toFixed(3)} mm</dd></dl>`;};
 }catch(e){$('#loading').textContent='Could not load geometry: '+e.message;console.error(e);}})();
renderer.setAnimationLoop(()=>{controls.update();renderer.render(scene,camera);});
