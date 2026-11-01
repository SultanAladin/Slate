import * as THREE from 'three';
import {OrbitControls} from 'three/addons/controls/OrbitControls.js';
const $=s=>document.querySelector(s),canvas=$('#viewport');
const renderer=new THREE.WebGLRenderer({canvas,antialias:true,preserveDrawingBuffer:true});
renderer.setPixelRatio(Math.min(devicePixelRatio,2));renderer.setClearColor(0x0e1520);
const scene=new THREE.Scene(),camera=new THREE.PerspectiveCamera(36,1,.01,100);
camera.up.set(0,0,1);const controls=new OrbitControls(camera,canvas);controls.enableDamping=true;controls.dampingFactor=.12;
const uniforms={mode:{value:0},errorScale:{value:2}};
const material=new THREE.ShaderMaterial({side:THREE.DoubleSide,uniforms,
vertexShader:`attribute float deviation;attribute float panel;attribute float part;varying vec3 n;varying vec3 worldN;varying float e;varying float f;varying float p;
void main(){n=normalize(normalMatrix*normal);worldN=normal;e=deviation;f=panel;p=part;gl_Position=projectionMatrix*modelViewMatrix*vec4(position,1.);}`,
fragmentShader:`precision highp float;uniform int mode;uniform float errorScale;varying vec3 n;varying vec3 worldN;varying float e;varying float f;varying float p;
void main(){vec3 N=normalize(n);if(!gl_FrontFacing)N=-N;float light=max(0.,dot(N,normalize(vec3(-.35,.55,.76))));float shade=.24+.65*light+.16*pow(max(0.,N.z),24.);vec3 col=p<.5?vec3(.72,.79,.84):p<1.5?vec3(.37,.44,.51):vec3(.17,.23,.31);
if(mode==1){col=.3+.6*fract(sin(vec3(17.1,33.7,57.3)*(f+1.))*43758.54);}
if(mode==2){float t=clamp(e/errorScale,0.,1.);col=vec3(min(1.,t*2.),min(1.,(1.-t)*2.),max(0.,1.-2.*t));if(e>5.)col=vec3(1.,.1,.65);shade=.78+.2*light;}
if(mode==3){float z=sin((N.x*.9+N.z*.3+N.y*.15)*100.);col=vec3(smoothstep(-.04,.04,z)*.94+.04);shade=.9;}
gl_FragColor=vec4(col*shade,1.);}`});
let meshes=[],edges,meta,box,center,radius,source;
function view(v){if(!center)return;camera.position.copy(center).add(new THREE.Vector3(...v).normalize().multiplyScalar(radius*2.9));controls.target.copy(center);controls.update();}
function resize(){const r=canvas.getBoundingClientRect();renderer.setSize(r.width,r.height,false);camera.aspect=r.width/r.height;camera.updateProjectionMatrix();}
window.addEventListener('resize',resize);resize();
(async()=>{try{
 const [buffer,metadata]=await Promise.all([fetch('geometry.bin').then(r=>{if(!r.ok)throw Error('Geometry not found');return r.arrayBuffer()}),fetch('viewer_metadata.json').then(r=>r.json())]);meta=metadata;
 const h=new Uint32Array(buffer,0,3),nv=h[0],ni=h[1],nl=h[2];const data=new Float32Array(buffer,12,nv*9);let off=12+nv*36;const index=new Uint32Array(buffer,off,ni);off+=ni*4;const line=new Float32Array(buffer,off,nl*6);
 const pos=new Float32Array(nv*3),nor=new Float32Array(nv*3),err=new Float32Array(nv),face=new Float32Array(nv),part=new Float32Array(nv);
 for(let i=0;i<nv;i++){pos.set(data.subarray(i*9,i*9+3),i*3);nor.set(data.subarray(i*9+3,i*9+6),i*3);err[i]=data[i*9+6];face[i]=data[i*9+7];part[i]=data[i*9+8];}
 for(let p=0;p<3;p++){const g=new THREE.BufferGeometry();g.setAttribute('position',new THREE.BufferAttribute(pos,3));g.setAttribute('normal',new THREE.BufferAttribute(nor,3));g.setAttribute('deviation',new THREE.BufferAttribute(err,1));g.setAttribute('panel',new THREE.BufferAttribute(face,1));g.setAttribute('part',new THREE.BufferAttribute(part,1));let ids=[];for(let i=0;i<ni;i+=3)if(part[index[i]]===p)ids.push(index[i],index[i+1],index[i+2]);g.setIndex(ids);let mesh=new THREE.Mesh(g,material);mesh.userData.part=p;scene.add(mesh);meshes.push(mesh);}
 const lg=new THREE.BufferGeometry();lg.setAttribute('position',new THREE.BufferAttribute(line,3));edges=new THREE.LineSegments(lg,new THREE.LineBasicMaterial({color:0x24394a,transparent:true,opacity:.7,depthTest:true}));edges.visible=false;scene.add(edges);
 box=new THREE.Box3().setFromBufferAttribute(new THREE.BufferAttribute(pos,3));center=box.getCenter(new THREE.Vector3());radius=box.getSize(new THREE.Vector3()).length()/2;view([7,-9,5]);$('#loading').hidden=true;$('#status').textContent=`312 faces · ${(ni/3).toLocaleString()} display triangles`;
 document.querySelectorAll('[data-view]').forEach(b=>b.onclick=()=>view(JSON.parse(b.dataset.view)));
 document.querySelectorAll('[name=mode]').forEach(b=>b.onchange=()=>{uniforms.mode.value=+b.value;edges.visible=$('#edges').checked||+b.value===1;$('#heatlegend').hidden=+b.value!==2;});
 $('#edges').onchange=()=>edges.visible=$('#edges').checked;
 document.querySelectorAll('[data-part]').forEach(b=>b.onchange=()=>{meshes[+b.dataset.part].visible=b.checked;edges.visible=$('#edges').checked&&meshes.every(m=>m.visible);});
 $('#screenshot').onclick=()=>{renderer.render(scene,camera);const a=document.createElement('a');a.download='Liger_inspection.png';a.href=canvas.toDataURL();a.click();};
 $('#reference').onchange=async()=>{if(!source){const b=await fetch('reference_points.bin').then(r=>r.arrayBuffer());const g=new THREE.BufferGeometry();g.setAttribute('position',new THREE.BufferAttribute(new Float32Array(b),3));source=new THREE.Points(g,new THREE.PointsMaterial({color:0xff9f70,size:.007,transparent:true,opacity:.5,depthTest:false}));scene.add(source);}source.visible=$('#reference').checked;};
 const ray=new THREE.Raycaster(),pointer=new THREE.Vector2();let down;
 canvas.onpointerdown=e=>down=[e.clientX,e.clientY];
 canvas.onpointerup=e=>{if(!down||Math.hypot(e.clientX-down[0],e.clientY-down[1])>5)return;const r=canvas.getBoundingClientRect();pointer.set((e.clientX-r.left)/r.width*2-1,-(e.clientY-r.top)/r.height*2+1);ray.setFromCamera(pointer,camera);const hits=ray.intersectObjects(meshes.filter(m=>m.visible));if(!hits.length)return;const hit=hits[0],g=hit.object.geometry;const id=g.getAttribute('panel').getX(hit.face.a);const p=meta.faces[id];$('#selection').innerHTML=`<strong>${p.name.replaceAll('_',' ')}</strong><div class="mini">${p.mirror?'Left':'Right'} · macro ${p.patch+1}</div><dl><dt>Control net</dt><dd>${p.poles.join(' × ')}</dd><dt>Held-out RMS</dt><dd>${p.rms.toFixed(3)} mm</dd><dt>Held-out max</dt><dd>${p.max.toFixed(3)} mm</dd></dl>`;};
 }catch(e){$('#loading').textContent='Could not load geometry: '+e.message;console.error(e);}})();
renderer.setAnimationLoop(()=>{controls.update();renderer.render(scene,camera);});
