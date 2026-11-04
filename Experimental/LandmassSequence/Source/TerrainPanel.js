//============================================================================================================================================
//                                                              TERRAINPANEL.JS
//============================================================================================================================================
// 🧩 Experimental continental landform authoring; bounded surface preview, reversible sculpture and versioned recipe files.

import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { Clamp, ConstructProject, ProjectElevation, ValidateProject, MaximumImpressions, MaximumLandforms, SourceRevision } from './ElevationProjection.js';
import { ResolveSampling } from './SurfaceProjection.js';
import { RevisionSequence } from './RevisionSequence.js';
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
<header class="Topbar">
    <div class="Brand"><span class="Brandmark">${Icon('Mountain')}</span><strong>FRONTIER</strong><span class="Divider"></span><span>Landmass Studio</span><span class="Badge">EXPERIMENTAL</span></div>
    <div class="HeaderActions"><button id="NewProject">${Icon('Globe')}<span>New</span></button><button id="OpenProject">${Icon('Folder')}<span>Open</span></button><button id="SaveProject">${Icon('Save')}<span>Save project</span></button><button id="HelpButton" class="Round" aria-label="Usage and scope">?</button></div>
</header>
<nav class="Workspacebar"><span class="WorkspaceActive">${Icon('Globe')} World authoring</span><span class="StageLabel">01 / LANDFORM FOUNDATION</span><span class="ProjectTitle" id="ProjectTitle"></span></nav>
<main class="Workspace">
    <aside class="Outliner Panel">
        <div class="PanelHeading"><span>Outliner</span><button id="AddLandform" class="Round" title="Add an island at the preview centre" aria-label="Add island">+</button></div>
        <div class="SummaryCards"><div><span id="LandformCount">4</span><small>LANDFORMS</small></div><div><span id="WorldSize">512</span><small>KM EXTENT</small></div></div>
        <div class="SectionCaption">WORLD CONTENT <span>⌄</span></div>
        <button class="WorldRow" id="SelectWorld">${Icon('Globe')} <span>World settings</span></button>
        <div id="LandformRows" class="LandformRows"></div>
        <div class="DatumRow">${Icon('Eye')} <span>Sea-level reference</span><span class="SmallTag">DATUM</span></div>
        <div class="OutlinerFooter"><span class="Dot"></span>Procedural recipe · no world voxel volume</div>
    </aside>
    <section class="Viewport Panel" id="Viewport">
        <div class="ViewportHeading"><div><span class="Dot"></span><span id="PreviewCaption">World overview</span><span class="SmallTag">3D</span></div><div><button id="HomeView" title="Frame the current extent">${Icon('Focus')}</button><button id="TopView">Top</button><button id="ToggleWire" title="Toggle triangle edges">Wire</button></div></div>
        <canvas id="TerrainCanvas" aria-label="Interactive landmass viewport"></canvas>
        <div class="ViewInformation"><span id="ExtentLabel">512 × 512 km</span><small id="SpacingLabel"></small></div>
        <div class="NorthIndicator"><span>N</span><i></i><small>Y UP</small></div>
        <div class="BrushReadout" id="BrushReadout" hidden></div>
        <div class="ViewportBottom"><div class="SculptToolbar" role="toolbar" aria-label="Landmass tools">
            <button data-tool="Navigate" class="Selected" title="Orbit / select · V">${Icon('Cursor')}<span>Select</span></button><span class="ToolDivider"></span>
            <button data-tool="Island" title="Place a new island · I">${Icon('Island')}<span>Island</span></button>
            <button data-tool="Raise" title="Raise land · R">${Icon('Raise')}<span>Raise</span></button>
            <button data-tool="Lower" title="Lower / cut an inlet · L">${Icon('Lower')}<span>Lower</span></button>
            <button data-tool="Smooth" title="Relax local elevation · S">${Icon('Smooth')}<span>Smooth</span></button>
            <button data-tool="Flatten" title="Flatten to sampled height · F">${Icon('Flatten')}<span>Flatten</span></button>
        </div><div class="NavigationHint" id="NavigationHint">Drag to orbit · right-drag to pan · wheel to zoom · click a landform to select</div></div>
        <div class="Busy" id="Busy" hidden><span class="Spinner"></span>Rebuilding preview</div>
        <div class="RendererFailure" id="RendererFailure" hidden></div>
    </section>
    <aside class="Inspector Panel">
        <div class="PanelHeading"><span>Inspector</span><div class="InlineActions"><button id="Undo" title="Undo · Ctrl+Z" aria-label="Undo">${Icon('Undo')}</button><button id="Redo" title="Redo · Ctrl+Shift+Z" aria-label="Redo">${Icon('Redo')}</button></div></div>
        <div class="InspectorScroll">
            <section class="InspectorCard"><div class="CardCaption">${Icon('Mountain')}<span>LANDFORM</span></div><input id="LandformName" class="TitleInput" aria-label="Landform name" maxlength="80"/><p class="Muted" id="SelectionDescription"></p><div id="LandformParameters">
                <label>Peak elevation <span><input id="Height" type="number" min="1" max="20000" step="100"/> m</span></label>
                <input id="HeightRange" type="range" min="100" max="8000" step="50" aria-label="Peak elevation"/>
                <div class="TwoColumns"><label>East / km<input id="East" type="number" step="1"/></label><label>North / km<input id="North" type="number" step="1"/></label></div>
                <div class="TwoColumns"><label>Along radius / km<input id="Along" type="number" min="0.1" step="1"/></label><label>Across radius / km<input id="Across" type="number" min="0.1" step="1"/></label></div>
                <label>Rotation <span><input id="Angle" type="number" min="-360" max="360" step="5"/> °</span></label>
                <label>Coastal relief <span id="RoughnessLabel"></span></label><input id="Roughness" type="range" min="0" max="1" step="0.01" aria-label="Coastal relief"/>
                <div class="ButtonPair"><button id="FocusLandform">${Icon('Focus')} Frame</button><button id="DeleteLandform" class="Danger">Remove</button></div>
            </div></section>
            <section class="InspectorCard"><div class="CardCaption">${Icon('Sliders')}<span>SCULPT BRUSH</span></div>
                <label>Radius <span><input id="BrushRadius" type="number" min="0.01" max="512" step="0.5" value="18"/> km</span></label><input id="RadiusRange" type="range" min="0.25" max="60" step="0.25" value="18" aria-label="Brush radius"/>
                <label>Strength <span><input id="BrushAmount" type="number" min="10" max="2000" step="10" value="350"/> m</span></label>
                <p class="Muted" id="BrushWarning">Choose a sculpt tool, then drag on the terrain.</p>
            </section>
            <section class="InspectorCard"><div class="CardCaption">${Icon('Globe')}<span>WORLD & PREVIEW</span></div>
                <label>Project name <input id="ProjectName" maxlength="80" aria-label="Project name"/></label>
                <label>World extent <span><input id="WorldSpan" type="number" min="16" max="20000" step="16"/> km</span></label>
                <label>Terrain seed <input id="Seed" type="number" min="-2147483648" max="2147483647" step="1"/></label>
                <label>Sea level <span><input id="SeaLevel" type="number" min="-10000" max="10000" step="10"/> m</span></label>
                <label>Relief display <span id="ExaggerationLabel">5×</span></label><input id="Exaggeration" type="range" min="1" max="12" step="0.5" value="5" aria-label="Vertical exaggeration"/>
                <div class="Segmented"><button id="WorldPreview" class="Selected">World</button><button id="DetailPreview">Local detail</button></div>
                <label>Local extent <span><input id="LocalSpan" type="number" min="0.25" max="20000" step="1" value="32"/> km</span></label>
                <div class="TwoColumns"><label>Focus east / km<input id="FocusEast" type="number" value="-48" step="1"/></label><label>Focus north / km<input id="FocusNorth" type="number" value="16" step="1"/></label></div>
                <button id="GoFocus" class="FullWidth">Go to coordinates</button>
                <p class="Muted">Flat-world coordinates. Local detail samples a smaller area; it does not allocate extra world voxels.</p>
            </section>
            <section class="InspectorCard BudgetCard"><div class="CardCaption">${Icon('Sliders')}<span>PREVIEW RESERVATIONS</span></div>
                <select id="BudgetProfile" aria-label="Geometry budget"><option value="Conservative">Conservative · 8 GB / 4 GB PC</option><option value="Balanced">Balanced</option><option value="Detailed">Detailed</option></select>
                <div class="TwoColumns"><label>CPU / MiB<input id="RamBudget" type="number" min="8" max="1024" value="32"/></label><label>GPU / MiB<input id="DeviceBudget" type="number" min="8" max="1024" value="32"/></label></div>
                <div class="Meter"><i id="BudgetMeter"></i></div><div class="BudgetNumbers"><span id="BudgetUsed"></span><span id="FacetCount"></span></div>
                <p class="Muted">Geometry working-set estimates only. Browser, renderer and driver overhead are additional. No erosion or water solver is running.</p>
            </section>
        </div>
    </aside>
</main>
<footer class="Statusbar"><span id="Status"><i class="Dot"></i>Ready to shape your world</span><div><span id="RecipeSize"></span><span id="BuildDuration"></span><span>METRES · Y UP</span></div></footer>
<input id="ProjectFile" type="file" accept=".json,.landscape" hidden/>
<dialog id="HelpDialog"><button id="CloseHelp" class="DialogClose" aria-label="Close help">×</button><span class="Badge">EXPERIMENTAL · FIRST STAGE</span><h1>A world starts with a coastline.</h1><p>Shape original continents, islands, peninsulas and inlets. The document is a compact procedural recipe, not a continent-sized voxel allocation.</p><h3>Working in the viewport</h3><p>Select a landform in the Outliner to change its dimensions, position, elevation and coastal relief. Choose Island to place one. Raise/Lower/Smooth/Flatten operate in metre space; one pointer drag is one undo. Right-drag pans and the wheel zooms. Esc returns to Select.</p><h3>Large worlds, bounded previews</h3><p>World mode samples the entire extent at a fixed budget. Local detail samples a smaller area around your focus coordinates. World size does not increase the sample count. The visible mesh is replaceable preview geometry; project files retain the authoring recipe. Vertical exaggeration affects display only.</p><h3>What this stage is not</h3><p>This is the macro landform foundation: a height-graph solid with a slope-normalised distance estimate, not a reinitialised volumetric SDF. Detailed SDF bricks, caves, overhangs, erosion, fractures, sediment, chemistry, snow, mesh export and realistic ocean simulation are deferred. Blue water is a sea-level reference plane only. A flat coordinate domain does not simulate a spherical planet.</p><h3>Saving and storage</h3><p>Save project downloads a versioned recipe file; Open validates and restores it. There is no implicit autosave. No terrain tiles are streamed from disk in this stage, so HDD/SSD parity has not been benchmarked. Geometry budgets do not limit total process memory.</p><p class="Muted" id="SourceAttribution"></p></dialog>`;

const Find = (Identity) => document.getElementById(Identity);
const Sequence = new RevisionSequence(ConstructProject());
let Selection = 1;
let Tool = 'Navigate';
let Detail = false;
let RequestedResolution = 144;
let Exaggeration = 5;
let Extent = { East: 0, North: 0, Span: Sequence.Document.Span };
let Sampling;
let Generation = 0;
let WorkerActive = false;
let Pending = false;
let RenderedRevision = -1;
let DisplayedExtent = null;
let Terrain = null;
let Dirty = false;
let Stroke = null;
let Hover = null;
let LastHit = null;
let RebuildTimer;
let Failure = null;
const ElevationWorker = new Worker(new URL('./ElevationQueue.js', import.meta.url), { type: 'module' });

function Announce(Message)
{
    Find('Status').textContent = Message;
}

function Protect(Action)
{
    try { Action(); }
    catch (Refusal) { Announce(`Not applied: ${Refusal.message}`); RefreshInspector(); }
}

function SelectedLandform()
{
    return Sequence.Document.Landforms.find((Landform) => Landform.Identity === Selection);
}

function RefreshOutliner()
{
    Find('LandformCount').textContent = Sequence.Document.Landforms.length;
    Find('WorldSize').textContent = (Sequence.Document.Span / 1000).toLocaleString();
    Find('ProjectTitle').textContent = `${Sequence.Document.Name}${Dirty ? ' · unsaved' : ''}`;
    Find('LandformRows').replaceChildren();
    for (const Landform of Sequence.Document.Landforms)
    {
        const Row = document.createElement('div');
        Row.className = `LandformRow ${Selection === Landform.Identity ? 'Active' : ''}`;
        const Select = document.createElement('button');
        Select.innerHTML = Icon('Mountain');
        const Label = document.createElement('span');
        Label.textContent = Landform.Name;
        Select.append(Label);
        Select.onclick = () => { Selection = Landform.Identity; RefreshOutliner(); RefreshInspector(); };
        const Visibility = document.createElement('input');
        Visibility.type = 'checkbox';
        Visibility.checked = Landform.Enabled;
        Visibility.title = 'Include landform in the surface';
        Visibility.setAttribute('aria-label', `Show ${Landform.Name}`);
        Visibility.onchange = () => ApplyEdit((Document) => { Document.Landforms.find((Candidate) => Candidate.Identity === Landform.Identity).Enabled = Visibility.checked; }, 'Landform visibility changed');
        Row.append(Select, Visibility);
        Find('LandformRows').append(Row);
    }
    Find('RecipeSize').textContent = `${(new TextEncoder().encode(JSON.stringify(Sequence.Document)).byteLength / 1024).toFixed(1)} KiB recipe`;
    Find('Undo').disabled = !Sequence.Before.length;
    Find('Redo').disabled = !Sequence.After.length;
}

function RefreshInspector()
{
    const Landform = SelectedLandform();
    Find('LandformName').value = Landform?.Name || 'World settings';
    Find('LandformName').disabled = !Landform;
    Find('LandformParameters').hidden = !Landform;
    Find('SelectionDescription').textContent = Landform ? 'Procedural landform · editable at any scale' : 'Select a landform or place a new island.';
    if (Landform)
    {
        for (const Field of ['Height', 'Roughness']) Find(Field).value = Landform[Field];
        Find('HeightRange').value = Landform.Height;
        for (const Field of ['East', 'North', 'Along', 'Across']) Find(Field).value = (Landform[Field] / 1000).toFixed(2);
        Find('Angle').value = (Landform.Angle * 180 / Math.PI).toFixed(1);
        Find('RoughnessLabel').textContent = `${Math.round(Landform.Roughness * 100)}%`;
    }
    Find('ProjectName').value = Sequence.Document.Name;
    Find('WorldSpan').value = Sequence.Document.Span / 1000;
    Find('Seed').value = Sequence.Document.Seed;
    Find('SeaLevel').value = Sequence.Document.SeaLevel;
}

function ApplyEdit(Change, Message)
{
    Protect(() =>
    {
        const Document = structuredClone(Sequence.Document);
        Change(Document);
        if (!Sequence.Replace(Document)) return;
        Dirty = true;
        RefreshOutliner();
        RefreshInspector();
        RequestSurface();
        Announce(Message);
    });
}

function ResolveExtent()
{
    const Span = Sequence.Document.Span;
    const LocalSpan = Clamp(Number(Find('LocalSpan').value) * 1000 || 32000, 250, Span);
    Find('LocalSpan').value = LocalSpan / 1000;
    const CurrentSpan = Detail ? LocalSpan : Span;
    const Limit = (Span - CurrentSpan) / 2;
    Extent = {
        East: Detail ? Clamp(Number(Find('FocusEast').value) * 1000 || 0, -Limit, Limit) : 0,
        North: Detail ? Clamp(Number(Find('FocusNorth').value) * 1000 || 0, -Limit, Limit) : 0,
        Span: CurrentSpan,
    };
    Find('FocusEast').value = (Extent.East / 1000).toFixed(2);
    Find('FocusNorth').value = (Extent.North / 1000).toFixed(2);
    Find('WorldPreview').classList.toggle('Selected', !Detail);
    Find('DetailPreview').classList.toggle('Selected', Detail);
    Find('PreviewCaption').textContent = Detail ? 'Local detail' : 'World overview';
    Find('ExtentLabel').textContent = `${(Extent.Span / 1000).toLocaleString()} × ${(Extent.Span / 1000).toLocaleString()} km`;
}

function RequestSurface()
{
    Generation++;
    Pending = true;
    Find('Busy').hidden = false;
    Find('Viewport').setAttribute('aria-busy', 'true');
    clearTimeout(RebuildTimer);
    RebuildTimer = setTimeout(DispatchSurface, Stroke ? 120 : 30);
}

function DispatchSurface()
{
    if (WorkerActive || !Pending || Failure) return;
    Protect(() =>
    {
        const Ram = Number(Find('RamBudget').value);
        const Device = Number(Find('DeviceBudget').value);
        if (Ram > 1024 || Device > 1024) throw new Error('Preview reservations are limited to 1024 MiB each.');
        Sampling = ResolveSampling(Extent.Span, Ram, Device, RequestedResolution, Sequence.MeasureBytes());
        WorkerActive = true;
        Pending = false;
        ElevationWorker.postMessage({ Revision: Generation, Document: Sequence.Document, Extent: { ...Extent }, Sampling });
        Find('SpacingLabel').textContent = `${Sampling.Spacing >= 1000 ? (Sampling.Spacing / 1000).toFixed(2) + ' km' : Sampling.Spacing.toFixed(0) + ' m'} sample spacing · display relief ${Exaggeration}×`;
        Find('BudgetUsed').textContent = `${(Sampling.WorkingBytes / 1048576).toFixed(1)} / ${Ram} MiB CPU`;
        Find('FacetCount').textContent = `${(Sampling.Triangles / 1000).toFixed(0)}k triangles`;
        Find('BudgetMeter').style.width = `${Math.min(100, Sampling.WorkingBytes / 1048576 / Ram * 100)}%`;
        RefreshBrushWarning();
    });
    if (!WorkerActive) { Find('Busy').hidden = true; Find('Viewport').setAttribute('aria-busy', 'false'); }
}

ElevationWorker.onmessage = ({ data: Response }) =>
{
    WorkerActive = false;
    if (Response.Revision === Generation)
    {
        if (Response.Refusal) Announce(`Preview refused: ${Response.Refusal}`);
        else
        {
            InstallSurface(Response.Surface);
            RenderedRevision = Response.Revision;
            Find('BuildDuration').textContent = `${Response.Duration.toFixed(0)} ms sample`;
        }
    }
    if (Pending) DispatchSurface();
    else { Find('Busy').hidden = true; Find('Viewport').setAttribute('aria-busy', 'false'); }
};
ElevationWorker.onerror = (Event) =>
{
    WorkerActive = false;
    Find('Busy').hidden = true;
    Announce(`Preview worker stopped: ${Event.message}. Reload to retry.`);
};

const Scene = new THREE.Scene();
Scene.background = new THREE.Color('#172025');
const Camera = new THREE.PerspectiveCamera(38, 1, 0.05, 100000);
const Canvas = Find('TerrainCanvas');
let Renderer;
let Controls;
try
{
    Renderer = new THREE.WebGLRenderer({ canvas: Canvas, antialias: false, powerPreference: 'high-performance' });
    Renderer.setPixelRatio(Math.min(devicePixelRatio, 1.5));
    Renderer.outputColorSpace = THREE.SRGBColorSpace;
    Renderer.setClearColor('#172025');
    Controls = new OrbitControls(Camera, Canvas);
    Controls.enableDamping = false;
    Controls.maxPolarAngle = Math.PI * 0.49;
    Controls.addEventListener('change', Render);
}
catch (Refusal)
{
    Failure = Refusal;
    Find('RendererFailure').hidden = false;
    Find('RendererFailure').textContent = 'WebGL2 is unavailable. Enable hardware acceleration or use a compatible browser. Your project can still be opened and saved.';
}
Scene.add(new THREE.HemisphereLight('#d7e2df', '#353b28', 1.4));
const Sun = new THREE.DirectionalLight('#fff0cf', 2.0);
Sun.position.set(-100, 180, 60);
Scene.add(Sun);
const Ocean = new THREE.Mesh(new THREE.PlaneGeometry(1, 1), new THREE.MeshStandardMaterial({ color: '#285360', roughness: 0.65, metalness: 0.16 }));
Ocean.rotation.x = -Math.PI / 2;
Scene.add(Ocean);
const Cursor = new THREE.LineLoop(new THREE.BufferGeometry().setFromPoints(Array.from({ length: 96 }, (_, Ordinal) => new THREE.Vector3(Math.cos(Ordinal / 96 * Math.PI * 2), 0, Math.sin(Ordinal / 96 * Math.PI * 2)))), new THREE.LineBasicMaterial({ color: '#dfc28c', depthTest: false }));
Cursor.renderOrder = 10;
Cursor.visible = false;
Scene.add(Cursor);
const Raycaster = new THREE.Raycaster();
const Pointer = new THREE.Vector2();

function Render()
{
    if (Renderer) Renderer.render(Scene, Camera);
}

function FrameExtent()
{
    if (!Controls) return;
    const Span = Extent.Span / 1000;
    Camera.position.set(Span * 0.56, Span * 0.65, Span * 0.72);
    Controls.target.set(0, 0, 0);
    Controls.minDistance = Math.max(0.1, Span / 1000);
    Controls.maxDistance = Span * 6;
    Controls.update();
    Render();
}

function InstallSurface(Surface)
{
    DisplayedExtent = { ...Extent };
    if (!Renderer) return;
    const Geometry = new THREE.BufferGeometry();
    Geometry.setAttribute('position', new THREE.BufferAttribute(Surface.Positions, 3));
    Geometry.setAttribute('color', new THREE.BufferAttribute(Surface.Colours, 3));
    Geometry.setIndex(new THREE.BufferAttribute(Surface.Indices, 1));
    Geometry.computeVertexNormals();
    Geometry.computeBoundingSphere();
    if (Terrain)
    {
        Scene.remove(Terrain);
        Terrain.geometry.dispose();
        Terrain.material.dispose();
    }
    Terrain = new THREE.Mesh(Geometry, new THREE.MeshStandardMaterial({ vertexColors: true, roughness: 0.94, wireframe: Find('ToggleWire').classList.contains('Selected') }));
    Terrain.scale.y = Exaggeration;
    Scene.add(Terrain);
    Ocean.scale.set(Extent.Span / 1000, Extent.Span / 1000, 1);
    Ocean.position.y = Sequence.Document.SeaLevel / 1000 * Exaggeration;
    Render();
}

function Resize()
{
    if (!Renderer) return;
    const Rectangle = Canvas.getBoundingClientRect();
    Renderer.setSize(Rectangle.width, Rectangle.height, false);
    Camera.aspect = Rectangle.width / Math.max(1, Rectangle.height);
    Camera.updateProjectionMatrix();
    Render();
}
new ResizeObserver(Resize).observe(Find('Viewport'));

function PickSurface(Event)
{
    if (!Renderer || !Terrain || RenderedRevision < 0 || !DisplayedExtent) return null;
    if (DisplayedExtent.East !== Extent.East || DisplayedExtent.North !== Extent.North || DisplayedExtent.Span !== Extent.Span) return null;
    const Rectangle = Canvas.getBoundingClientRect();
    Pointer.set((Event.clientX - Rectangle.left) / Rectangle.width * 2 - 1, -(Event.clientY - Rectangle.top) / Rectangle.height * 2 + 1);
    Raycaster.setFromCamera(Pointer, Camera);
    // 📝 Sculpt the solid seabed as well as land; the reference water plane is not a collision surface.
    const Hits = Raycaster.intersectObject(Terrain);
    if (!Hits.length) return null;
    const Position = Hits[0].point;
    return { East: Position.x * 1000 + Extent.East, North: Position.z * 1000 + Extent.North, Height: Position.y * 1000 / Exaggeration };
}

function RefreshBrushWarning()
{
    const Radius = Number(Find('BrushRadius').value) * 1000;
    Find('BrushWarning').textContent = Sampling && Radius < Sampling.Spacing * 2 ? 'Brush is below overview resolution. Switch to Local detail to see the edit.' : 'One drag = one undo. Lower carves bays and inlets; changes remain in the recipe.';
}

function SelectTool(Selected)
{
    FinishStroke();
    Tool = Selected;
    if (Controls) Controls.mouseButtons.LEFT = Tool === 'Navigate' ? THREE.MOUSE.ROTATE : -1;
    document.querySelectorAll('[data-tool]').forEach((Button) => Button.classList.toggle('Selected', Button.dataset.tool === Tool));
    Find('NavigationHint').textContent = Tool === 'Navigate' ? 'Drag to orbit · right-drag to pan · wheel to zoom · click a landform to select' : `${Tool}: left-click / drag to author · right-drag to pan · Esc to select`;
    Canvas.style.cursor = Tool === 'Navigate' ? 'default' : 'crosshair';
    Cursor.visible = Tool !== 'Navigate' && Hover !== null;
    Render();
}

function PlaceIsland(Hit)
{
    ApplyEdit((Document) =>
    {
        if (Document.Landforms.length >= MaximumLandforms) throw new Error('This stage supports at most 96 procedural landforms.');
        const Identity = Math.max(0, ...Document.Landforms.map((Landform) => Landform.Identity)) + 1;
        Document.Landforms.push({ Identity, Name: `Island ${Identity}`, East: Hit.East, North: Hit.North, Along: Document.Span / 14, Across: Document.Span / 20, Height: 2300, Angle: 0.2, Roughness: 0.3, Enabled: true });
        Selection = Identity;
    }, 'Island added · adjust its shape in Inspector');
}

function AppendImpression(Hit)
{
    if (!Stroke) return;
    const Radius = Clamp(Number(Find('BrushRadius').value) * 1000 || 18000, 10, Sequence.Document.Span);
    if (LastHit && Math.hypot(Hit.East - LastHit.East, Hit.North - LastHit.North) < Radius * 0.22) return;
    if (Sequence.Document.Impressions.length >= MaximumImpressions)
    {
        Announce('Recipe limit reached (2048 impressions). Save this project before continuing elsewhere.');
        return;
    }
    const Amount = Clamp(Number(Find('BrushAmount').value) || 350, 10, 2000);
    let Target = Stroke.Target;
    if (Tool === 'Smooth')
    {
        Target = (ProjectElevation(Sequence.Document, Hit.East - Radius / 2, Hit.North) + ProjectElevation(Sequence.Document, Hit.East + Radius / 2, Hit.North) + ProjectElevation(Sequence.Document, Hit.East, Hit.North - Radius / 2) + ProjectElevation(Sequence.Document, Hit.East, Hit.North + Radius / 2)) / 4;
    }
    Sequence.Document.Impressions.push({ Operation: Tool, East: Hit.East, North: Hit.North, Radius, Amount: Tool === 'Lower' ? -Amount : Amount, Strength: Amount / 2000, Target: Clamp(Target, -20000, 20000) });
    LastHit = Hit;
    RequestSurface();
}

function FinishStroke(Cancel = false)
{
    if (!Stroke) return;
    const Authored = Sequence.Document;
    Sequence.Document = Stroke.Before;
    Stroke = null;
    LastHit = null;
    if (!Cancel && Sequence.Replace(Authored))
    {
        Dirty = true;
        Announce(`${Tool} stroke applied · Ctrl+Z restores the previous surface`);
    }
    else Announce(Cancel ? 'Stroke cancelled' : 'No surface change');
    RefreshOutliner();
    RequestSurface();
}

Canvas.addEventListener('pointermove', (Event) =>
{
    Hover = PickSurface(Event);
    if (!Hover)
    {
        Cursor.visible = false;
        Find('BrushReadout').hidden = true;
        Render();
        return;
    }
    if (Stroke) AppendImpression(Hover);
    const Radius = Clamp(Number(Find('BrushRadius').value) || 18, 0.01, Sequence.Document.Span / 1000);
    Cursor.position.set((Hover.East - Extent.East) / 1000, ProjectElevation(Sequence.Document, Hover.East, Hover.North) / 1000 * Exaggeration + 0.03, (Hover.North - Extent.North) / 1000);
    Cursor.scale.set(Radius, 1, Radius);
    Cursor.visible = Tool !== 'Navigate';
    Find('BrushReadout').hidden = false;
    Find('BrushReadout').textContent = `E ${(Hover.East / 1000).toFixed(1)} · N ${(Hover.North / 1000).toFixed(1)} km · ${ProjectElevation(Sequence.Document, Hover.East, Hover.North).toFixed(0)} m`;
    Render();
});
Canvas.addEventListener('pointerdown', (Event) =>
{
    if (Event.button !== 0) return;
    const Hit = PickSurface(Event);
    if (!Hit) return;
    if (Tool === 'Navigate')
    {
        const Landforms = Sequence.Document.Landforms.filter((Landform) => Landform.Enabled);
        const Nearest = Landforms.sort((First, Last) => Math.hypot(Hit.East - First.East, Hit.North - First.North) - Math.hypot(Hit.East - Last.East, Hit.North - Last.North))[0];
        if (Nearest) { Selection = Nearest.Identity; RefreshOutliner(); RefreshInspector(); }
        return;
    }
    if (Tool === 'Island') { PlaceIsland(Hit); return; }
    Canvas.setPointerCapture(Event.pointerId);
    Stroke = { Before: structuredClone(Sequence.Document), Target: Clamp(ProjectElevation(Sequence.Document, Hit.East, Hit.North), -20000, 20000) };
    LastHit = null;
    AppendImpression(Hit);
});
Canvas.addEventListener('pointerup', () => FinishStroke());
Canvas.addEventListener('pointercancel', () => FinishStroke(true));
Canvas.addEventListener('lostpointercapture', () => FinishStroke());
Canvas.addEventListener('pointerleave', () => { if (!Stroke) { Cursor.visible = false; Find('BrushReadout').hidden = true; Render(); } });

for (const Button of document.querySelectorAll('[data-tool]')) Button.onclick = () => SelectTool(Button.dataset.tool);
for (const Field of ['East', 'North', 'Along', 'Across', 'Height', 'Angle', 'Roughness'])
{
    Find(Field).onchange = () => ApplyEdit((Document) =>
    {
        const Landform = Document.Landforms.find((Candidate) => Candidate.Identity === Selection);
        if (!Landform) return;
        const Scale = ['East', 'North', 'Along', 'Across'].includes(Field) ? 1000 : Field === 'Angle' ? Math.PI / 180 : 1;
        Landform[Field] = Number(Find(Field).value) * Scale;
    }, 'Landform parameters applied');
}
Find('HeightRange').oninput = () => { Find('Height').value = Find('HeightRange').value; };
Find('HeightRange').onchange = () => Find('Height').onchange();
Find('LandformName').onchange = () => ApplyEdit((Document) => { Document.Landforms.find((Landform) => Landform.Identity === Selection).Name = Find('LandformName').value; }, 'Landform renamed');
Find('SelectWorld').onclick = () => { Selection = null; RefreshOutliner(); RefreshInspector(); };
Find('AddLandform').onclick = () => PlaceIsland({ East: Extent.East, North: Extent.North });
Find('DeleteLandform').onclick = () => ApplyEdit((Document) => { Document.Landforms = Document.Landforms.filter((Landform) => Landform.Identity !== Selection); Selection = null; }, 'Landform removed · sculpt impressions remain independent');
Find('FocusLandform').onclick = () =>
{
    const Landform = SelectedLandform();
    if (!Landform) return;
    Find('FocusEast').value = Landform.East / 1000;
    Find('FocusNorth').value = Landform.North / 1000;
    SwitchPreview(true);
};
Find('RadiusRange').oninput = () => { Find('BrushRadius').value = Find('RadiusRange').value; RefreshBrushWarning(); };
Find('BrushRadius').onchange = () => { Find('BrushRadius').value = Clamp(Number(Find('BrushRadius').value) || 18, 0.01, Sequence.Document.Span / 1000); Find('RadiusRange').value = Find('BrushRadius').value; RefreshBrushWarning(); };
Find('Exaggeration').oninput = () =>
{
    Exaggeration = Number(Find('Exaggeration').value);
    Find('ExaggerationLabel').textContent = `${Exaggeration}×`;
    if (Terrain) Terrain.scale.y = Exaggeration;
    Ocean.position.y = Sequence.Document.SeaLevel / 1000 * Exaggeration;
    Render();
};
Find('ProjectName').onchange = () => ApplyEdit((Document) => { Document.Name = Find('ProjectName').value; }, 'Project renamed');
Find('SeaLevel').onchange = () => ApplyEdit((Document) => { Document.SeaLevel = Number(Find('SeaLevel').value); }, 'Sea-level datum changed');
Find('Seed').onchange = () => ApplyEdit((Document) => { Document.Seed = Number(Find('Seed').value); }, 'Terrain seed changed · sculpt recipe preserved');
Find('WorldSpan').onchange = () =>
{
    ApplyEdit((Document) =>
    {
        Document.Span = Number(Find('WorldSpan').value) * 1000;
    }, 'World extent changed · authored metre coordinates preserved');
    ResolveExtent();
    FrameExtent();
    RequestSurface();
};
function SwitchPreview(Local)
{
    FinishStroke();
    Detail = Local;
    ResolveExtent();
    FrameExtent();
    RequestSurface();
}
Find('WorldPreview').onclick = () => SwitchPreview(false);
Find('DetailPreview').onclick = () => SwitchPreview(true);
Find('GoFocus').onclick = () => SwitchPreview(true);
Find('LocalSpan').onchange = () => { if (Detail) SwitchPreview(true); };
Find('HomeView').onclick = FrameExtent;
Find('TopView').onclick = () =>
{
    if (!Controls) return;
    Camera.position.set(0, Extent.Span / 1000 * 1.4, 0.01);
    Controls.target.set(0, 0, 0);
    Controls.update();
    Render();
};
Find('ToggleWire').onclick = () =>
{
    Find('ToggleWire').classList.toggle('Selected');
    if (Terrain) Terrain.material.wireframe = Find('ToggleWire').classList.contains('Selected');
    Render();
};
Find('BudgetProfile').onchange = () =>
{
    const Profiles = { Conservative: [32, 32, 144], Balanced: [64, 64, 256], Detailed: [128, 128, 384] };
    const Profile = Profiles[Find('BudgetProfile').value];
    Find('RamBudget').value = Profile[0]; Find('DeviceBudget').value = Profile[1]; RequestedResolution = Profile[2];
    RequestSurface();
};
Find('RamBudget').onchange = RequestSurface;
Find('DeviceBudget').onchange = RequestSurface;
function TraverseRevision(Forward)
{
    FinishStroke(true);
    if (!(Forward ? Sequence.Advance() : Sequence.Rewind())) return;
    Dirty = true;
    ResolveExtent();
    RefreshOutliner();
    RefreshInspector();
    RequestSurface();
    Announce(Forward ? 'Redo' : 'Undo');
}
Find('Undo').onclick = () => TraverseRevision(false);
Find('Redo').onclick = () => TraverseRevision(true);
Find('SaveProject').onclick = () =>
{
    FinishStroke();
    const Text = JSON.stringify(Sequence.Document, null, 2);
    const Url = URL.createObjectURL(new Blob([Text], { type: 'application/json' }));
    const Link = document.createElement('a');
    Link.href = Url; Link.download = 'Frontier.landscape.json'; Link.click();
    setTimeout(() => URL.revokeObjectURL(Url), 1000);
    Dirty = false;
    RefreshOutliner();
    Announce('Project download requested · recipe remains editable');
};
Find('NewProject').onclick = () =>
{
    if (Dirty && !confirm('Start a blank world? Save the current project first to keep it.')) return;
    FinishStroke(true);
    const Blank = ConstructProject(Sequence.Document.Seed, Sequence.Document.Span);
    Blank.Landforms = [];
    Blank.Name = 'New landscape';
    Sequence.Replace(Blank);
    Selection = null;
    Dirty = true;
    Detail = false;
    ResolveExtent(); RefreshOutliner(); RefreshInspector(); FrameExtent(); RequestSurface();
    SelectTool('Island');
    Announce('Blank world ready · click the viewport to place your first island');
};
Find('OpenProject').onclick = () => Find('ProjectFile').click();
Find('ProjectFile').onchange = async () =>
{
    const File = Find('ProjectFile').files[0];
    Find('ProjectFile').value = '';
    if (!File) return;
    if (File.size > 2 * 1048576) { Announce('Open refused: project exceeds the 2 MiB recipe limit.'); return; }
    try
    {
        const Accepted = ValidateProject(JSON.parse(await File.text()));
        if (Dirty && !confirm('Replace unsaved edits? Save the current project first if you need them.')) return;
        FinishStroke(true);
        Sequence.Replace(Accepted);
        Selection = Accepted.Landforms[0]?.Identity ?? null;
        Dirty = false;
        Detail = false;
        ResolveExtent(); RefreshOutliner(); RefreshInspector(); FrameExtent(); RequestSurface();
        Announce('Project opened · no simulation started');
    }
    catch (Refusal) { Announce(`Open refused: ${Refusal.message}`); }
};
Find('HelpButton').onclick = () => Find('HelpDialog').showModal();
Find('CloseHelp').onclick = () => Find('HelpDialog').close();
Find('SourceAttribution').textContent = `Reference studio inspected at ${SourceRevision}. This isolated macro-authoring experiment does not yet port its erosion, geology or ocean algorithms.`;
window.addEventListener('keydown', (Event) =>
{
    if ((Event.ctrlKey || Event.metaKey) && Event.key.toLowerCase() === 's' && !Find('HelpDialog').open)
    {
        Event.preventDefault();
        document.activeElement?.blur();
        Find('SaveProject').click();
        return;
    }
    if (['INPUT', 'TEXTAREA', 'SELECT'].includes(document.activeElement?.tagName) || Find('HelpDialog').open) return;
    if (Event.key === 'Escape') { FinishStroke(true); SelectTool('Navigate'); }
    if ((Event.ctrlKey || Event.metaKey) && Event.key.toLowerCase() === 'z') { Event.preventDefault(); TraverseRevision(Event.shiftKey); return; }
    if (Event.ctrlKey || Event.metaKey || Event.altKey) return;
    const Shortcuts = { v: 'Navigate', i: 'Island', r: 'Raise', l: 'Lower', s: 'Smooth', f: 'Flatten' };
    if (Shortcuts[Event.key.toLowerCase()]) SelectTool(Shortcuts[Event.key.toLowerCase()]);
});
window.addEventListener('beforeunload', (Event) => { if (Dirty) { Event.preventDefault(); Event.returnValue = ''; } });
Canvas.addEventListener('webglcontextlost', (Event) => { Event.preventDefault(); Failure = new Error('WebGL context lost'); Announce('Graphics context lost. Save the recipe, then reload.'); });

RefreshOutliner();
RefreshInspector();
ResolveExtent();
FrameExtent();
Resize();
RequestSurface();
