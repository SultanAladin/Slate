//============================================================================================================================================
//                                                              TERRAINPANEL.JS
//============================================================================================================================================
// 🧩 Planet-scale volumetric CSG authoring with bounded local isosurface extraction and Texture Workspace-style controls.

import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { ConstructProject, ValidateProject, ConstructFrame, ProjectAnchor, ProjectPosition, CompileSolid, TraceRadial, Clamp, Dot, MaximumImpressions, MaximumLandforms, SourceRevision } from './PlanetProjection.js';
import { ResolveSampling, ProjectSection } from './SurfaceProjection.js';
import { RevisionSequence } from './RevisionSequence.js';
import { SliderMarkup, ConnectSlider, ReflectSlider } from './ControlPanel.js';
import './TerrainPanel.css';

const Glyphs = {
    Mountain: '<path d="m2 19 7-13 4 7 3-5 6 11Z"/><path d="m7 10 2 3 2-3"/>',
    Cursor: '<path d="m5 3 14 9-7 1-3 7Z"/>',
    Raise: '<path d="M4 18h16M6 13l6-8 6 8M12 5v14"/>',
    Lower: '<path d="M4 18h16M6 7l6 8 6-8M12 3v12"/>',
    Smooth: '<path d="M3 16c5-17 13 8 18-9M3 20h18"/>',
    Flatten: '<path d="M3 13h18M8 6l4 4 4-4M8 20l4-4 4 4"/>',
    Island: '<path d="M2 17c4-4 5 4 10 0s6 4 10 0M6 14l5-9 6 9"/>',
    Focus: '<path d="M3 9V3h6m6 0h6v6m0 6v6h-6M9 21H3v-6"/>',
    Undo: '<path d="m8 3-5 5 5 5M3 8h11a7 7 0 0 1 0 14"/>',
    Redo: '<path d="m16 3 5 5-5 5m5-5H10a7 7 0 0 0 0 14"/>',
    Folder: '<path d="M3 6h7l2 3h9v11H3Z"/>',
    Save: '<path d="M5 3h12l4 4v14H3V3Zm2 0v7h10V3M7 21v-7h10v7"/>',
    Globe: '<circle cx="12" cy="12" r="9"/><ellipse cx="12" cy="12" rx="4" ry="9"/><path d="M3 12h18"/>',
    Sliders: '<path d="M4 6h16M4 12h16M4 18h16"/><circle cx="9" cy="6" r="2"/><circle cx="16" cy="12" r="2"/><circle cx="8" cy="18" r="2"/>',
    Eye: '<path d="M2 12s4-7 10-7 10 7 10 7-4 7-10 7-10-7-10-7Z"/><circle cx="12" cy="12" r="3"/>',
};
function Icon(Name)
{
    return `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">${Glyphs[Name] || Glyphs.Mountain}</svg>`;
}

const Application = document.querySelector('#Application');
Application.innerHTML = `
<header class="Topbar"><div class="Brand"><span class="Brandmark">${Icon('Globe')}</span><strong>FRONTIER</strong><span class="Divider"></span><span>Planet Studio</span><span class="Badge">EXPERIMENTAL</span></div><div class="HeaderActions"><button id="NewProject">${Icon('Globe')}<span>New</span></button><button id="OpenProject">${Icon('Folder')}<span>Open</span></button><button id="SaveProject">${Icon('Save')}<span>Save project</span></button><button id="HelpButton" class="Round" aria-label="Usage and scope">?</button></div></header>
<nav class="Workspacebar"><span class="WorkspaceActive">${Icon('Globe')} Volumetric planet</span><span class="StageLabel">3D CSG · NO HEIGHTMAP</span><span class="ProjectTitle" id="ProjectTitle"></span></nav>
<main class="Workspace">
<aside class="Outliner Panel"><div class="PanelHeading"><span>Outliner</span><button id="AddLandform" class="Round" aria-label="Add landmass">+</button></div><div class="SummaryCards"><div><span id="LandformCount">4</span><small>LANDFORMS</small></div><div><span id="WorldSize">3,390</span><small>KM RADIUS</small></div></div><div class="SectionCaption">PLANET SOLIDS <span>⌄</span></div><button class="WorldRow" id="SelectWorld">${Icon('Globe')}<span>Spherical bedrock</span></button><div id="LandformRows" class="LandformRows"></div><div class="DatumRow">${Icon('Eye')}<span>Spherical sea datum</span></div><div class="SculptureSummary" id="SculptureSummary"></div><div class="OutlinerFooter"><span class="Dot"></span>3D field · planet voxels never allocated</div></aside>
<section class="Viewport Panel" id="Viewport"><div class="ViewportHeading"><div><span class="Dot"></span><span id="PreviewCaption">Planet overview</span><span class="SmallTag">SDF</span></div><div><button id="HomeView" title="Frame view">${Icon('Focus')}</button><button id="TopView">Top</button><button id="ToggleWire">Wire</button><button id="Cutaway" title="Local section exposes buried cavities">Section</button></div></div><canvas id="TerrainCanvas" aria-label="Interactive volumetric planet viewport"></canvas><div class="ViewInformation"><span id="ExtentLabel"></span><small id="SpacingLabel"></small></div><div class="BrushReadout" id="BrushReadout" hidden></div><div class="ViewportBottom"><div class="SculptToolbar" role="toolbar" aria-label="Volumetric tools"><button data-tool="Navigate" class="Selected">${Icon('Cursor')}<span>Select</span></button><span class="ToolDivider"></span><button data-tool="Island">${Icon('Island')}<span>Landmass</span></button><button data-tool="Union">${Icon('Raise')}<span>Add solid</span></button><button data-tool="Difference">${Icon('Lower')}<span>Subtract</span></button><button data-tool="Cave">${Icon('Smooth')}<span>Cave</span></button></div><div class="NavigationHint" id="NavigationHint">Drag to orbit · right-drag to pan · double-click terrain to enter local volume</div></div><div class="Busy" id="Busy" hidden><span class="Spinner"></span>Extracting 3D field</div><div class="RendererFailure" id="RendererFailure" hidden></div></section>
<aside class="Inspector Panel"><div class="PanelHeading"><span>Inspector</span><div class="InlineActions"><button id="Undo" aria-label="Undo">${Icon('Undo')}</button><button id="Redo" aria-label="Redo">${Icon('Redo')}</button></div></div><div class="InspectorScroll">
<section class="InspectorCard"><div class="CardCaption">${Icon('Mountain')}<span>VOLUMETRIC LANDFORM</span></div><input id="LandformName" class="TitleInput" aria-label="Landform name" maxlength="80"/><p class="Muted" id="SelectionDescription"></p><div id="LandformParameters">${SliderMarkup('Height', 'Radial crest', 0.001, 100, 0.001, 22, 'km')}<div class="TwoColumns"><label>Longitude / °<input id="Longitude" type="number" min="-180" max="180" step="1"/></label><label>Latitude / °<input id="Latitude" type="number" min="-90" max="90" step="1"/></label></div>${SliderMarkup('Along', 'Along radius', 0.01, 5000, 0.01, 1550, 'km')}${SliderMarkup('Across', 'Across radius', 0.01, 5000, 0.01, 1050, 'km')}${SliderMarkup('Roughness', '3D rock relief', 0, 0.5, 0.01, 0.12, '')}<div class="ButtonPair"><button id="FocusLandform">${Icon('Focus')}Local volume</button><button id="DeleteLandform" class="Danger">Remove</button></div></div></section>
<section class="InspectorCard"><div class="CardCaption">${Icon('Sliders')}<span>VOLUMETRIC BRUSH</span></div>${SliderMarkup('BrushRadius', 'Sphere radius', 0.001, 500, 0.001, 120, 'km')}${SliderMarkup('Burial', 'Inward placement', 0, 2, 0.05, 0.6, '×r')}<p class="Muted" id="BrushWarning">Add/subtract real 3D solid. Cave places a buried void; Section reveals it. One drag = one undo.</p></section>
<section class="InspectorCard"><div class="CardCaption">${Icon('Globe')}<span>PLANET & LOCAL VOLUME</span></div><label>Project name<input id="ProjectName" maxlength="80"/></label><label>Planet radius <span><input id="PlanetRadius" type="number" min="100" max="100000" step="10"/> km</span></label><p class="Muted" id="Circumference"></p><div class="ButtonPair"><button id="MarsPreset">3,390 km</button><button id="EarthSizePreset">6,371 km</button></div><label>Terrain seed<input id="Seed" type="number" step="1"/></label><label>Sea datum <span><input id="SeaLevel" type="number" step="10"/> m</span></label><div class="Segmented"><button id="WorldPreview" class="Selected">Planet</button><button id="DetailPreview">Local 3D</button></div>${SliderMarkup('LocalSpan', 'Local volume width', 0.001, 1000, 0.001, 120, 'km')}<div class="TwoColumns"><label>Focus longitude / °<input id="FocusLongitude" type="number" value="25" min="-180" max="180" step="1"/></label><label>Focus latitude / °<input id="FocusLatitude" type="number" value="12" min="-90" max="90" step="1"/></label></div><button id="GoFocus" class="FullWidth">Enter local volume</button><p class="Muted">Planet size and local cell spacing are independent. Double-click a surface to edit it locally. Planet size presets change radius, not existing brush radii.</p></section>
<section class="InspectorCard BudgetCard"><div class="CardCaption">${Icon('Sliders')}<span>GEOMETRY RESERVATIONS</span></div><select id="BudgetProfile" aria-label="Geometry budget"><option value="Conservative">Conservative · 8 GB / 4 GB PC</option><option value="Balanced">Balanced</option><option value="Detailed">Detailed</option></select><div class="TwoColumns"><label>CPU / MiB<input id="RamBudget" type="number" min="8" max="1024" value="32"/></label><label>GPU / MiB<input id="DeviceBudget" type="number" min="8" max="1024" value="32"/></label></div><div class="Meter"><i id="BudgetMeter"></i></div><div class="BudgetNumbers"><span id="BudgetUsed"></span><span id="FacetCount"></span></div><p class="Muted">Conservative geometry estimates, not whole-process limits. Browser/driver/framebuffer overhead is additional. Local extraction refuses overflow rather than silently dropping geometry.</p></section>
</div></aside></main><footer class="Statusbar"><span id="Status">Ready · volumetric planet, not a heightmap</span><div><span id="RecipeSize"></span><span id="BuildDuration"></span><span>METRES · 3D SOLID</span></div></footer><input id="ProjectFile" type="file" accept=".json" hidden/>
<dialog id="HelpDialog"><button id="CloseHelp" class="DialogClose" aria-label="Close help">×</button><span class="Badge">VOLUMETRIC FOUNDATION</span><h1>One planet. Any direction.</h1><p>The authoring document is a three-dimensional signed-distance CSG field: spherical bedrock, curved volumetric ellipsoids and union/subtraction spheres. It is not a heightmap or a single elevation per location. Subtraction can create buried cavities, undercuts and overhangs.</p><h3>Planet scale versus local resolution</h3><p>The default radius is 3,390 km, about 21,300 km around. Change the radius independently of local resolution. Planet overview extracts only the coarse outer envelope from the same 3D field. Double-click a surface or enter longitude/latitude to extract a bounded 3D volume with marching tetrahedra, including interior surfaces.</p><h3>Sculpt in 3D</h3><p>Landmass places a closed volumetric landform. Add solid unions spheres; Subtract removes spheres. Cave puts the sphere below the surface. Section clips half of the local preview to reveal buried voids. A stroke is one undo. Sea is a reference surface, not collision geometry or an ocean simulation. Very small edits may be invisible in the coarse planet overview: enter a small local volume to resolve them.</p><h3>Storage, accuracy and limits</h3><p>No planet-wide voxel allocation. Only the current local volume is sampled, under explicit geometry reservations. Ellipsoidal and warped CSG fields are signed-distance estimates, not an exact reinitialised Euclidean distance solution. Isosurfaces use field signs; no unsafe sphere-tracing assumption. Coarse sampling can miss sub-cell features. This base is not yet sparse-brick persistence, continuous planet LOD streaming, erosion, fracture or realistic ocean simulation. HDD/SSD parity is not claimed.</p><h3>Controls and files</h3><p>Pill sliders follow the C++ Texture Workspace dimensions and pointer travel: filled 28 px rail, 26 px thumb, numeric/unit cells, drag/release and direct typing. These are browser counterparts, not linked ImGui code. Ctrl+S saves the recipe. Import is validated atomically. Legacy revision-1 height-graph projects refuse rather than silently changing meaning.</p><p class="Muted" id="SourceAttribution"></p></dialog>`;

const Find = (Identity) => document.getElementById(Identity);
const Sequence = new RevisionSequence(ConstructProject(), 4 * 1048576);
let Selection = 1;
let Tool = 'Navigate';
let Mode = 'Planet';
let RequestedResolution = 48;
let Extent = {};
let DisplayedExtent;
let Sampling;
let Generation = 0;
let Pending = false;
let WorkerActive = false;
let Terrain;
let Ocean;
let SectionCap;
let Dirty = false;
let Stroke;
let LastHit;
let RebuildTimer;
let Failure;
const SolidWorker = new Worker(new URL('./IsosurfaceQueue.js', import.meta.url), { type: 'module' });

function Announce(Message) { Find('Status').textContent = Message; }
function Protect(Action) { try { Action(); } catch (Refusal) { Announce(`Not applied: ${Refusal.message}`); RefreshInspector(); } }
function SelectedLandform() { return Sequence.Document.Landforms.find((Landform) => Landform.Identity === Selection); }

function RefreshOutliner()
{
    Find('LandformCount').textContent = Sequence.Document.Landforms.length;
    Find('WorldSize').textContent = (Sequence.Document.Radius / 1000).toLocaleString();
    Find('ProjectTitle').textContent = `${Sequence.Document.Name}${Dirty ? ' · unsaved' : ''}`;
    Find('LandformRows').replaceChildren();
    for (const Landform of Sequence.Document.Landforms)
    {
        const Row = document.createElement('div'); Row.className = `LandformRow ${Selection === Landform.Identity ? 'Active' : ''}`;
        const Select = document.createElement('button'); Select.innerHTML = Icon('Mountain');
        const Label = document.createElement('span'); Label.textContent = Landform.Name; Select.append(Label);
        Select.onclick = () => { Selection = Landform.Identity; RefreshOutliner(); RefreshInspector(); };
        const Visibility = document.createElement('input'); Visibility.type = 'checkbox'; Visibility.checked = Landform.Enabled;
        Visibility.setAttribute('aria-label', `Show ${Landform.Name}`);
        Visibility.onchange = () => ApplyEdit((Document) => { Document.Landforms.find((Candidate) => Candidate.Identity === Landform.Identity).Enabled = Visibility.checked; }, 'Solid visibility changed');
        Row.append(Select, Visibility); Find('LandformRows').append(Row);
    }
    Find('RecipeSize').textContent = `${(new TextEncoder().encode(JSON.stringify(Sequence.Document)).byteLength / 1024).toFixed(1)} KiB recipe`;
    Find('SculptureSummary').textContent = `${Sequence.Document.Impressions.length} volumetric brush operations`;
    Find('Undo').disabled = !Sequence.Before.length; Find('Redo').disabled = !Sequence.After.length;
}

function RefreshInspector()
{
    const Landform = SelectedLandform();
    Find('LandformName').value = Landform?.Name || 'Planet solid'; Find('LandformName').disabled = !Landform;
    Find('LandformParameters').hidden = !Landform;
    Find('SelectionDescription').textContent = Landform ? 'Closed 3D solid · curved geographic anchor' : 'Select or place a volumetric landmass.';
    if (Landform)
    {
        for (const Field of ['Height', 'Along', 'Across'])
        {
            const Maximum = Sequence.Document.Radius / 1000 * (Field === 'Height' ? 0.1 : 1.5);
            Find(Field).max = Maximum; Find(`${Field}Range`).max = Maximum;
        }
        for (const Field of ['Height', 'Along', 'Across']) ReflectSlider(Field, Landform[Field] / 1000);
        ReflectSlider('Roughness', Landform.Roughness);
        Find('Longitude').value = Landform.Longitude.toFixed(3); Find('Latitude').value = Landform.Latitude.toFixed(3);
    }
    Find('BrushRadius').max = Sequence.Document.Radius / 4000; Find('BrushRadiusRange').max = Sequence.Document.Radius / 4000;
    Find('ProjectName').value = Sequence.Document.Name;
    Find('PlanetRadius').value = Sequence.Document.Radius / 1000;
    Find('Circumference').textContent = `${(Math.PI * 2 * Sequence.Document.Radius / 1000).toLocaleString(undefined, { maximumFractionDigits: 0 })} km circumference · spherical coordinates`;
    Find('Seed').value = Sequence.Document.Seed; Find('SeaLevel').value = Sequence.Document.SeaLevel;
}

function ApplyEdit(Change, Message)
{
    Protect(() =>
    {
        if (Stroke) FinishStroke();
        const Document = structuredClone(Sequence.Document); Change(Document);
        if (!Sequence.Replace(Document)) return;
        Dirty = true; RefreshOutliner(); RefreshInspector(); RequestSurface(); Announce(Message);
    });
}

function ResolveExtent()
{
    const Longitude = Number(Find('FocusLongitude').value);
    const Latitude = Number(Find('FocusLatitude').value);
    if (!Number.isFinite(Longitude) || !Number.isFinite(Latitude) || Math.abs(Longitude) > 180 || Math.abs(Latitude) > 90) throw new Error('Invalid focus longitude/latitude.');
    const Span = Clamp(Number(Find('LocalSpan').value) * 1000, 1, Math.min(1000000, Sequence.Document.Radius));
    const Direction = ConstructFrame(Longitude, Latitude).Up;
    const Altitude = Mode === 'Local' ? TraceRadial(Sequence.Document, CompileSolid(Sequence.Document), Direction) - Sequence.Document.Radius : 0;
    Extent = { Mode, Longitude, Latitude, Span, Altitude, PlanetRadius: Sequence.Document.Radius };
    Find('WorldPreview').classList.toggle('Selected', Mode === 'Planet'); Find('DetailPreview').classList.toggle('Selected', Mode === 'Local');
    Find('PreviewCaption').textContent = Mode === 'Planet' ? 'Planet overview · outer envelope' : 'Local volumetric SDF';
    Find('ExtentLabel').textContent = Mode === 'Planet' ? `${(Sequence.Document.Radius * 2 / 1000).toLocaleString()} km diameter · ${(Math.PI * 2 * Sequence.Document.Radius / 1000).toLocaleString(undefined, { maximumFractionDigits: 0 })} km around` : `${Span / 1000} × ${Span / 1000} × ${Span / 1000} km volume`;
    Find('Cutaway').disabled = Mode !== 'Local';
}

function RequestSurface()
{
    Generation++; Pending = true; Find('Busy').hidden = false; Find('Viewport').setAttribute('aria-busy', 'true');
    clearTimeout(RebuildTimer); RebuildTimer = setTimeout(DispatchSurface, Stroke ? 150 : 30);
}
function DispatchSurface()
{
    if (WorkerActive || !Pending || Failure) return;
    Protect(() =>
    {
        const Ram = Number(Find('RamBudget').value); const Device = Number(Find('DeviceBudget').value);
        if (Ram > 1024 || Device > 1024) throw new Error('Geometry reservations exceed this stage’s 1024 MiB ceiling.');
        Sampling = ResolveSampling(Mode, Ram, Device, RequestedResolution, Sequence.MeasureBytes());
        Pending = false; WorkerActive = true;
        SolidWorker.postMessage({ Revision: Generation, Document: Sequence.Document, Extent: { ...Extent }, Sampling });
        Find('SpacingLabel').textContent = Mode === 'Planet' ? '3D field → coarse outer isosurface · double-click for local detail' : `${(Extent.Span / Sampling.Resolution).toFixed(0)} m cells · ${Sampling.Resolution}³ cells · full 3D extraction`;
        Find('BudgetUsed').textContent = `${(Sampling.WorkingBytes / 1048576).toFixed(1)} / ${Ram} MiB CPU reserved`;
        Find('BudgetMeter').style.width = `${Sampling.WorkingBytes / (Ram * 1048576) * 100}%`;
    });
    if (!WorkerActive) { Pending = false; Find('Busy').hidden = true; Find('Viewport').setAttribute('aria-busy', 'false'); }
}
SolidWorker.onmessage = ({ data: Response }) =>
{
    WorkerActive = false;
    if (Response.Revision === Generation)
    {
        if (Response.Refusal) Announce(`Extraction refused: ${Response.Refusal}`);
        else
        {
            DisplayedExtent = Response.Extent; InstallSurface(Response.Surface);
            Find('BuildDuration').textContent = `${Response.Duration.toFixed(0)} ms extraction`;
            Find('FacetCount').textContent = `${(Response.Surface.Triangles / 1000).toFixed(1)}k triangles`;
        }
    }
    if (Pending) DispatchSurface();
    else { Find('Busy').hidden = true; Find('Viewport').setAttribute('aria-busy', 'false'); }
};
SolidWorker.onerror = (Event) => { WorkerActive = false; Failure = new Error(Event.message); Find('Busy').hidden = true; Find('Viewport').setAttribute('aria-busy', 'false'); Announce(`Extraction worker stopped: ${Event.message}. Save and reload.`); };

const Scene = new THREE.Scene(); Scene.background = new THREE.Color('#121a20');
const Camera = new THREE.PerspectiveCamera(38, 1, 0.01, 1000000);
const Canvas = Find('TerrainCanvas');
let Renderer; let Controls;
try
{
    Renderer = new THREE.WebGLRenderer({ canvas: Canvas, antialias: false, powerPreference: 'high-performance' });
    Renderer.setPixelRatio(Math.min(devicePixelRatio, 1.5)); Renderer.outputColorSpace = THREE.SRGBColorSpace; Renderer.localClippingEnabled = true;
    Controls = new OrbitControls(Camera, Canvas); Controls.enableDamping = false; Controls.addEventListener('change', Render);
}
catch (Refusal) { Failure = Refusal; Find('RendererFailure').hidden = false; Find('RendererFailure').textContent = 'WebGL2 unavailable. Enable hardware acceleration. Recipe save/open remains available.'; }
Scene.add(new THREE.HemisphereLight('#d7e2df', '#343a28', 1.4));
const Sun = new THREE.DirectionalLight('#fff1d7', 2.2); Sun.position.set(100, 150, 100); Scene.add(Sun);
const Cursor = new THREE.Mesh(new THREE.SphereGeometry(1, 16, 12), new THREE.MeshBasicMaterial({ color: '#dcc28b', wireframe: true, transparent: true, opacity: 0.35, depthTest: false }));
Cursor.visible = false; Cursor.renderOrder = 10; Scene.add(Cursor);
const Raycaster = new THREE.Raycaster(); const Pointer = new THREE.Vector2();
const SectionPlane = new THREE.Plane(new THREE.Vector3(1, 0, 0), 0);
function Render()
{
    if (!Renderer) return;
    if (Mode === 'Planet')
    {
        const Radius = Sequence.Document.Radius / 1000;
        Camera.near = Math.max(0.1, Math.min(Radius * 0.03, (Camera.position.length() - Radius * 1.25) * 0.25));
        Camera.updateProjectionMatrix();
    }
    Renderer.render(Scene, Camera);
}
function FrameExtent()
{
    if (!Controls) return;
    const Span = Mode === 'Planet' ? Sequence.Document.Radius / 1000 * 2 : Extent.Span / 1000;
    Camera.position.set(Span * 1.1, Span * 0.75, Span * 1.25); Controls.target.set(0, 0, 0);
    Camera.near = Math.max(0.00001, Span / 1000); Camera.far = Span * 30; Camera.updateProjectionMatrix();
    Controls.minDistance = Mode === 'Planet' ? Sequence.Document.Radius / 1000 * 1.3 : Span / 10000; Controls.maxDistance = Span * 8; Controls.update(); Render();
}
function InstallSurface(Surface)
{
    if (!Renderer) return;
    const Geometry = new THREE.BufferGeometry();
    Geometry.setAttribute('position', new THREE.BufferAttribute(Surface.Positions.subarray(0, Surface.Count * 3), 3));
    Geometry.setAttribute('color', new THREE.BufferAttribute(Surface.Colours.subarray(0, Surface.Count * 3), 3));
    Geometry.computeVertexNormals(); Geometry.computeBoundingSphere();
    if (Terrain) { Scene.remove(Terrain); Terrain.geometry.dispose(); Terrain.material.dispose(); }
    Terrain = new THREE.Mesh(Geometry, new THREE.MeshStandardMaterial({ vertexColors: true, roughness: 0.94, side: THREE.DoubleSide, wireframe: Find('ToggleWire').classList.contains('Selected'), clippingPlanes: Mode === 'Local' && Find('Cutaway').classList.contains('Selected') ? [SectionPlane] : [] }));
    Scene.add(Terrain);
    if (Ocean) { Scene.remove(Ocean); Ocean.geometry.dispose(); Ocean.material.dispose(); }
    const OceanRadius = (Sequence.Document.Radius + Sequence.Document.SeaLevel) / 1000;
    if (Mode === 'Planet') Ocean = new THREE.Mesh(new THREE.SphereGeometry(OceanRadius, 96, 64), new THREE.MeshStandardMaterial({ color: '#204452', roughness: 0.72, metalness: 0.1 }));
    else
    {
        const Geometry = new THREE.PlaneGeometry(Extent.Span / 1000, Extent.Span / 1000, 16, 16); Geometry.rotateX(-Math.PI / 2);
        const Points = Geometry.attributes.position;
        for (let Ordinal = 0; Ordinal < Points.count; Ordinal++) Points.setY(Ordinal, Math.sqrt(Math.max(0, OceanRadius ** 2 - Points.getX(Ordinal) ** 2 - Points.getZ(Ordinal) ** 2)) - (Sequence.Document.Radius + Extent.Altitude) / 1000);
        Geometry.computeVertexNormals();
        Ocean = new THREE.Mesh(Geometry, new THREE.MeshStandardMaterial({ color: '#204452', roughness: 0.72, transparent: true, opacity: 0.6, depthWrite: false, side: THREE.DoubleSide }));
    }
    Ocean.visible = !(Mode === 'Local' && Find('Cutaway').classList.contains('Selected')); Scene.add(Ocean); RefreshSection(); Render();
}
function RefreshSection()
{
    if (SectionCap) { Scene.remove(SectionCap); SectionCap.geometry.dispose(); SectionCap.material.dispose(); SectionCap = null; }
    if (!Renderer || Mode !== 'Local' || !Find('Cutaway').classList.contains('Selected')) return;
    const Positions = ProjectSection(Sequence.Document, Extent, -SectionPlane.constant * 1000);
    const Geometry = new THREE.BufferGeometry(); Geometry.setAttribute('position', new THREE.BufferAttribute(Positions, 3)); Geometry.computeVertexNormals();
    SectionCap = new THREE.Mesh(Geometry, new THREE.MeshStandardMaterial({ color: '#67624e', roughness: 1, side: THREE.DoubleSide }));
    Scene.add(SectionCap);
}
function Resize()
{
    if (!Renderer) return;
    const Rectangle = Canvas.getBoundingClientRect(); Renderer.setSize(Rectangle.width, Rectangle.height, false);
    Camera.aspect = Rectangle.width / Math.max(1, Rectangle.height); Camera.updateProjectionMatrix(); Render();
}
new ResizeObserver(Resize).observe(Find('Viewport'));

function WorldFromLocal(Position)
{
    const Frame = ConstructFrame(DisplayedExtent.Longitude, DisplayedExtent.Latitude);
    return Frame.Up.map((Coordinate, Axis) => Coordinate * (Sequence.Document.Radius + DisplayedExtent.Altitude) + Position[0] * Frame.East[Axis] + Position[1] * Frame.Up[Axis] + Position[2] * Frame.North[Axis]);
}
function PickSurface(Event)
{
    if (!Terrain || !DisplayedExtent || DisplayedExtent.PlanetRadius !== Sequence.Document.Radius || DisplayedExtent.Mode !== Mode || DisplayedExtent.Longitude !== Extent.Longitude || DisplayedExtent.Latitude !== Extent.Latitude || DisplayedExtent.Span !== Extent.Span) return null;
    const Rectangle = Canvas.getBoundingClientRect(); Pointer.set((Event.clientX - Rectangle.left) / Rectangle.width * 2 - 1, -(Event.clientY - Rectangle.top) / Rectangle.height * 2 + 1);
    Raycaster.setFromCamera(Pointer, Camera);
    const Hit = Raycaster.intersectObject(Terrain).find((Intersection) => !(Mode === 'Local' && Find('Cutaway').classList.contains('Selected') && SectionPlane.distanceToPoint(Intersection.point) < 0));
    if (!Hit) return null;
    const Position = Hit.point.toArray().map((Coordinate) => Coordinate * 1000);
    const World = Mode === 'Planet' ? Position : WorldFromLocal(Position);
    return { World, Position, Anchor: ProjectAnchor(Sequence.Document, World) };
}
function SelectTool(Selected)
{
    FinishStroke(); Tool = Selected;
    if (Controls) Controls.mouseButtons.LEFT = Tool === 'Navigate' ? THREE.MOUSE.ROTATE : -1;
    document.querySelectorAll('[data-tool]').forEach((Button) => Button.classList.toggle('Selected', Button.dataset.tool === Tool));
    Find('NavigationHint').textContent = Tool === 'Navigate' ? 'Drag to orbit · right-drag to pan · double-click a surface for local volume' : `${Tool}: left-click / drag in 3D · Esc to select · use Local 3D for small features`;
    Canvas.style.cursor = Tool === 'Navigate' ? 'default' : 'crosshair'; Cursor.visible = false;
}
function PlaceIsland(Anchor)
{
    ApplyEdit((Document) =>
    {
        if (Document.Landforms.length >= MaximumLandforms) throw new Error('96 landform limit reached.');
        const Identity = Math.max(0, ...Document.Landforms.map((Landform) => Landform.Identity)) + 1;
        Document.Landforms.push({ Identity, Name: `Landmass ${Identity}`, Longitude: Anchor.Longitude, Latitude: Anchor.Latitude, Along: Math.min(400000, Document.Radius / 5), Across: Math.min(600000, Document.Radius / 4), Height: 14000, Angle: 0.3, Roughness: 0.15, Enabled: true }); Selection = Identity;
    }, 'Volumetric landmass added');
}
function AppendImpression(Hit)
{
    if (!Stroke) return;
    const Radius = Clamp(Number(Find('BrushRadius').value) * 1000, 1, Sequence.Document.Radius / 4);
    if (LastHit && Math.hypot(...Hit.World.map((Coordinate, Axis) => Coordinate - LastHit.World[Axis])) < Radius * 0.3) return;
    if (Sequence.Document.Impressions.length >= MaximumImpressions) { Announce('1024 volumetric operation limit reached. Save before continuing.'); return; }
    const Anchor = { ...Hit.Anchor };
    if (Tool === 'Difference' || Tool === 'Cave') Anchor.Altitude -= Radius * (Tool === 'Cave' ? Math.max(1.35, Number(Find('Burial').value)) : Number(Find('Burial').value));
    Sequence.Document.Impressions.push({ ...Anchor, Operation: Tool === 'Union' ? 'Union' : 'Difference', Radius });
    LastHit = Hit; RequestSurface();
}
function FinishStroke(Cancel = false)
{
    if (!Stroke) return;
    const Authored = Sequence.Document; Sequence.Document = Stroke.Before; Stroke = null; LastHit = null;
    if (!Cancel && Sequence.Replace(Authored)) { Dirty = true; Announce(`${Tool} 3D stroke applied · one undo restores it`); }
    RefreshOutliner(); RequestSurface();
}
Canvas.addEventListener('pointerdown', (Event) =>
{
    if (Event.button !== 0) return;
    const Hit = PickSurface(Event); if (!Hit) return;
    if (Tool === 'Navigate')
    {
        const Direction = ConstructFrame(Hit.Anchor.Longitude, Hit.Anchor.Latitude).Up;
        const Closest = Sequence.Document.Landforms.filter((Landform) => Landform.Enabled).sort((First, Last) => Dot(Direction, ConstructFrame(Last.Longitude, Last.Latitude).Up) - Dot(Direction, ConstructFrame(First.Longitude, First.Latitude).Up))[0];
        if (Closest) { Selection = Closest.Identity; RefreshOutliner(); RefreshInspector(); } return;
    }
    if (Tool === 'Island') { PlaceIsland(Hit.Anchor); return; }
    Canvas.setPointerCapture(Event.pointerId); Stroke = { Before: structuredClone(Sequence.Document) }; LastHit = null; AppendImpression(Hit);
});
Canvas.addEventListener('pointermove', (Event) =>
{
    const Hit = PickSurface(Event); if (!Hit) { Cursor.visible = false; Find('BrushReadout').hidden = true; Render(); return; }
    if (Stroke) AppendImpression(Hit);
    Cursor.position.fromArray(Hit.Position.map((Coordinate) => Coordinate / 1000)); Cursor.scale.setScalar(Number(Find('BrushRadius').value)); Cursor.visible = Tool !== 'Navigate' && Tool !== 'Island';
    Find('BrushReadout').hidden = false; Find('BrushReadout').textContent = `${Hit.Anchor.Longitude.toFixed(2)}° lon · ${Hit.Anchor.Latitude.toFixed(2)}° lat · ${Hit.Anchor.Altitude.toFixed(0)} m radial altitude`; Render();
});
Canvas.addEventListener('pointerup', () => FinishStroke()); Canvas.addEventListener('pointercancel', () => FinishStroke(true)); Canvas.addEventListener('lostpointercapture', () => FinishStroke());
Canvas.addEventListener('pointerleave', () => { if (!Stroke) { Cursor.visible = false; Find('BrushReadout').hidden = true; Render(); } });
Canvas.addEventListener('dblclick', (Event) =>
{
    if (Tool !== 'Navigate') return;
    const Hit = PickSurface(Event); if (!Hit) return;
    Find('FocusLongitude').value = Hit.Anchor.Longitude; Find('FocusLatitude').value = Hit.Anchor.Latitude; SwitchPreview('Local');
});
function SwitchPreview(Replacement)
{
    FinishStroke(); Mode = Replacement; Find('Cutaway').classList.remove('Selected');
    Protect(() => { ResolveExtent(); FrameExtent(); RequestSurface(); });
    if (Mode === 'Local' && Number(Find('BrushRadius').value) * 1000 > Extent.Span / 4) ReflectSlider('BrushRadius', Math.max(0.001, Extent.Span / 1000 * 0.12));
    Cursor.visible = false;
}
for (const Button of document.querySelectorAll('[data-tool]')) Button.onclick = () => SelectTool(Button.dataset.tool);
for (const Field of ['Height', 'Along', 'Across', 'Roughness']) ConnectSlider(Field, (Quantity) => ApplyEdit((Document) => { const Landform = Document.Landforms.find((Candidate) => Candidate.Identity === Selection); if (Landform) Landform[Field] = Quantity * (Field === 'Roughness' ? 1 : 1000); }, 'Volumetric landform changed'));
for (const Field of ['Longitude', 'Latitude']) Find(Field).onchange = () => ApplyEdit((Document) => { Document.Landforms.find((Landform) => Landform.Identity === Selection)[Field] = Number(Find(Field).value); }, 'Geographic anchor changed');
ConnectSlider('BrushRadius', () => {}); ConnectSlider('Burial', () => {}); ConnectSlider('LocalSpan', () => { if (Mode === 'Local') SwitchPreview('Local'); });
Find('LandformName').onchange = () => ApplyEdit((Document) => { Document.Landforms.find((Landform) => Landform.Identity === Selection).Name = Find('LandformName').value; }, 'Solid renamed');
Find('ProjectName').onchange = () => ApplyEdit((Document) => { Document.Name = Find('ProjectName').value; }, 'Planet renamed');
Find('Seed').onchange = () => ApplyEdit((Document) => { Document.Seed = Number(Find('Seed').value); }, 'Seed changed');
Find('SeaLevel').onchange = () => ApplyEdit((Document) => { Document.SeaLevel = Number(Find('SeaLevel').value); }, 'Sea datum changed');
Find('PlanetRadius').onchange = () => { ApplyEdit((Document) => { Document.Radius = Number(Find('PlanetRadius').value) * 1000; }, 'Planet radius changed · brush radii preserved'); Protect(() => { ResolveExtent(); FrameExtent(); RequestSurface(); }); };
Find('MarsPreset').onclick = () => { Find('PlanetRadius').value = 3390; Find('PlanetRadius').onchange(); };
Find('EarthSizePreset').onclick = () => { Find('PlanetRadius').value = 6371; Find('PlanetRadius').onchange(); };
Find('SelectWorld').onclick = () => { Selection = null; RefreshOutliner(); RefreshInspector(); };
Find('AddLandform').onclick = () => PlaceIsland({ Longitude: Number(Find('FocusLongitude').value), Latitude: Number(Find('FocusLatitude').value) });
Find('DeleteLandform').onclick = () => ApplyEdit((Document) => { Document.Landforms = Document.Landforms.filter((Landform) => Landform.Identity !== Selection); Selection = null; }, 'Landmass removed · other CSG operations retained');
Find('FocusLandform').onclick = () => { const Landform = SelectedLandform(); if (Landform) { Find('FocusLongitude').value = Landform.Longitude; Find('FocusLatitude').value = Landform.Latitude; SwitchPreview('Local'); } };
Find('WorldPreview').onclick = () => SwitchPreview('Planet'); Find('DetailPreview').onclick = () => SwitchPreview('Local'); Find('GoFocus').onclick = () => SwitchPreview('Local'); Find('HomeView').onclick = FrameExtent;
Find('TopView').onclick = () => { if (!Controls) return; const Span = Mode === 'Planet' ? Sequence.Document.Radius / 1000 * 2 : Extent.Span / 1000; Camera.position.set(0, Span * 1.6, Span * 0.0001); Controls.target.set(0, 0, 0); Controls.update(); Render(); };
Find('ToggleWire').onclick = () => { Find('ToggleWire').classList.toggle('Selected'); if (Terrain) Terrain.material.wireframe = Find('ToggleWire').classList.contains('Selected'); Render(); };
Find('Cutaway').onclick = () =>
{
    Find('Cutaway').classList.toggle('Selected');
    const Enabled = Find('Cutaway').classList.contains('Selected');
    if (Enabled && Controls)
    {
        const Latest = Sequence.Document.Impressions.at(-1);
        let Centre = [0, -Extent.Span * 0.1, 0];
        if (Latest)
        {
            const World = ProjectPosition(Sequence.Document, Latest);
            const Frame = ConstructFrame(Extent.Longitude, Extent.Latitude);
            const Relative = World.map((Coordinate, Axis) => Coordinate - Frame.Up[Axis] * (Sequence.Document.Radius + Extent.Altitude));
            const Candidate = [Dot(Relative, Frame.East), Dot(Relative, Frame.Up), Dot(Relative, Frame.North)];
            if (Candidate.every((Coordinate) => Math.abs(Coordinate) <= Extent.Span / 2)) Centre = Candidate;
        }
        SectionPlane.constant = -Centre[0] / 1000;
        const Span = Extent.Span / 1000;
        Controls.target.fromArray(Centre.map((Coordinate) => Coordinate / 1000));
        Camera.position.set(Centre[0] / 1000 - Span * 1.25, Centre[1] / 1000 + Span * 0.06, Centre[2] / 1000 + Span * 0.65);
        Controls.update();
    }
    if (Terrain) Terrain.material.clippingPlanes = Enabled ? [SectionPlane] : [];
    if (Ocean) Ocean.visible = !Enabled;
    if (!Enabled) FrameExtent();
    RefreshSection();
    Render();
};
Find('BudgetProfile').onchange = () => { const Profile = { Conservative: [32, 32, 48], Balanced: [64, 64, 72], Detailed: [128, 128, 96] }[Find('BudgetProfile').value]; Find('RamBudget').value = Profile[0]; Find('DeviceBudget').value = Profile[1]; RequestedResolution = Profile[2]; RequestSurface(); };
Find('RamBudget').onchange = RequestSurface; Find('DeviceBudget').onchange = RequestSurface;
function TraverseRevision(Forward)
{
    FinishStroke(true); if (!(Forward ? Sequence.Advance() : Sequence.Rewind())) return;
    Dirty = true; RefreshOutliner(); RefreshInspector(); Protect(() => { ResolveExtent(); RequestSurface(); }); Announce(Forward ? 'Redo' : 'Undo');
}
Find('Undo').onclick = () => TraverseRevision(false); Find('Redo').onclick = () => TraverseRevision(true);
Find('SaveProject').onclick = () =>
{
    FinishStroke(); const Url = URL.createObjectURL(new Blob([JSON.stringify(Sequence.Document, null, 2)], { type: 'application/json' }));
    const Link = document.createElement('a'); Link.href = Url; Link.download = 'Frontier.planet.json'; Link.click(); setTimeout(() => URL.revokeObjectURL(Url), 1000);
    Dirty = false; RefreshOutliner(); Announce('Volumetric planet recipe download requested');
};
Find('OpenProject').onclick = () => Find('ProjectFile').click();
Find('ProjectFile').onchange = async () =>
{
    const File = Find('ProjectFile').files[0]; Find('ProjectFile').value = ''; if (!File) return;
    if (File.size > 2 * 1048576) { Announce('Open refused: 2 MiB recipe limit.'); return; }
    try
    {
        const Accepted = ValidateProject(JSON.parse(await File.text()));
        if (Dirty && !confirm('Replace unsaved edits?')) return;
        FinishStroke(true); Sequence.Replace(Accepted); Selection = Accepted.Landforms[0]?.Identity ?? null; Dirty = false;
        Mode = 'Planet'; Find('Cutaway').classList.remove('Selected'); ResolveExtent(); RefreshOutliner(); RefreshInspector(); FrameExtent(); RequestSurface(); Announce('Volumetric planet opened');
    }
    catch (Refusal) { Announce(`Open refused: ${Refusal.message}`); }
};
Find('NewProject').onclick = () =>
{
    if (Dirty && !confirm('Start a blank spherical planet?')) return;
    FinishStroke(true); const Blank = ConstructProject(Sequence.Document.Seed, Sequence.Document.Radius); Blank.Landforms = []; Blank.Name = 'New planet'; Sequence.Replace(Blank);
    Selection = null; Dirty = true; Mode = 'Planet'; Find('Cutaway').classList.remove('Selected'); ResolveExtent(); RefreshOutliner(); RefreshInspector(); FrameExtent(); RequestSurface(); SelectTool('Island'); Announce('Blank 3D bedrock sphere ready');
};
Find('HelpButton').onclick = () => Find('HelpDialog').showModal(); Find('CloseHelp').onclick = () => Find('HelpDialog').close();
Find('SourceAttribution').textContent = `Reference erosion studio: ${SourceRevision}. Native slider reference: Frontier ControlKit::Slider and ValuePill. Erosion remains deferred.`;
window.addEventListener('keydown', (Event) =>
{
    if ((Event.ctrlKey || Event.metaKey) && Event.key.toLowerCase() === 's' && !Find('HelpDialog').open) { Event.preventDefault(); document.activeElement?.blur(); Find('SaveProject').click(); return; }
    if (['INPUT', 'TEXTAREA', 'SELECT'].includes(document.activeElement?.tagName) || Find('HelpDialog').open) return;
    if (Event.key === 'Escape') { FinishStroke(true); SelectTool('Navigate'); }
    if ((Event.ctrlKey || Event.metaKey) && Event.key.toLowerCase() === 'z') { Event.preventDefault(); TraverseRevision(Event.shiftKey); return; }
    if (Event.ctrlKey || Event.metaKey || Event.altKey) return;
    const Shortcuts = { v: 'Navigate', i: 'Island', a: 'Union', d: 'Difference', c: 'Cave' }; if (Shortcuts[Event.key.toLowerCase()]) SelectTool(Shortcuts[Event.key.toLowerCase()]);
});
window.addEventListener('beforeunload', (Event) => { if (Dirty) { Event.preventDefault(); Event.returnValue = ''; } });
Canvas.addEventListener('webglcontextlost', (Event) => { Event.preventDefault(); Failure = new Error('Graphics context lost'); Announce('Graphics context lost. Save the recipe and reload.'); });
RefreshOutliner(); RefreshInspector(); ResolveExtent(); FrameExtent(); Resize(); RequestSurface();
