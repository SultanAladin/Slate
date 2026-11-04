//============================================================================================================================================
//                                                          LANDMASSSEQUENCE.TEST.JS
//============================================================================================================================================
// 🧩 Volumetric topology, planet-scale coordinates, bounded 3D extraction and reversible CSG recipes.

import Test from 'node:test';
import Assert from 'node:assert/strict';
import { ConstructProject, CompileSolid, ConstructFrame, ProjectPosition, ProjectAnchor, TraceRadial, ValidateProject } from '../Source/PlanetProjection.js';
import { ResolveSampling, ProjectSurface, ProjectSection } from '../Source/SurfaceProjection.js';
import { RevisionSequence } from '../Source/RevisionSequence.js';

function CavityPlanet()
{
    const Document = ConstructProject(); Document.Landforms = [];
    Document.Impressions = [{ Operation: 'Difference', Longitude: 0, Latitude: 0, Altitude: -Document.CrustDepth - 3000, Radius: 2000 }];
    return Document;
}

Test('default is a Mars-size sphere, not a 512 km square', () =>
{
    const Document = ConstructProject();
    Assert.equal(Document.Radius, 3390000);
    Assert(Math.PI * 2 * Document.Radius > 21000000);
    Assert.equal(Document.Format, 'frontier-sdf-planet'); Assert.equal(Document.Revision, 2);
    Assert.equal(Document.Span, undefined);
});

Test('pure spherical bedrock has exact radial signed distance in metres', () =>
{
    const Document = ConstructProject(); Document.Landforms = [];
    const Solid = CompileSolid(Document); const Radius = Document.Radius - Document.CrustDepth;
    Assert.equal(Solid.Sample([Radius + 123, 0, 0]), 123);
    Assert.equal(Solid.Sample([0, Radius - 123, 0]), -123);
    Assert.equal(Solid.Sample([0, 0, -Radius - 123]), 123);
    Assert.equal(Solid.Sample([0, 0, 0]), -Radius);
});

Test('a single radial line has air/solid/cavity/solid transitions: impossible for a heightmap', () =>
{
    const Document = CavityPlanet(); const Solid = CompileSolid(Document);
    const Surface = Document.Radius - Document.CrustDepth;
    Assert(Solid.Sample([Surface + 1000, 0, 0]) > 0);
    Assert(Solid.Sample([Surface - 500, 0, 0]) < 0);
    Assert(Solid.Sample([Surface - 3000, 0, 0]) > 0);
    Assert(Solid.Sample([Surface - 6000, 0, 0]) < 0);
});

Test('union can place a detached 3D solid, not merely raise one elevation', () =>
{
    const Document = ConstructProject(); Document.Landforms = [];
    Document.Impressions.push({ Operation: 'Union', Longitude: 0, Latitude: 0, Altitude: 12000, Radius: 2000 });
    const Solid = CompileSolid(Document);
    Assert(Solid.Sample([Document.Radius + 12000, 0, 0]) < 0);
    Assert(Solid.Sample([Document.Radius + 5000, 0, 0]) > 0);
    Assert(Solid.Sample([Document.Radius - Document.CrustDepth - 100, 0, 0]) < 0);
});

Test('geographic anchors roundtrip at equator, poles and date line', () =>
{
    const Document = ConstructProject();
    for (const [Longitude, Latitude] of [[0, 0], [179.99, -45], [-179.99, 65], [22, 89.99]])
    {
        const Anchor = { Longitude, Latitude, Altitude: -1234.5 };
        const Restored = ProjectAnchor(Document, ProjectPosition(Document, Anchor));
        Assert(Math.abs(Restored.Longitude - Longitude) < 1e-7);
        Assert(Math.abs(Restored.Latitude - Latitude) < 1e-7);
        Assert(Math.abs(Restored.Altitude - Anchor.Altitude) < 1e-7);
    }
});

Test('3D landform field and seed are deterministic', () =>
{
    const First = ConstructProject(); const Last = ConstructProject();
    const Position = ProjectPosition(First, { Longitude: 25, Latitude: 12, Altitude: 21000 });
    Assert.equal(CompileSolid(First).Sample(Position), CompileSolid(Last).Sample(Position));
    Last.Seed++;
    Assert.notEqual(CompileSolid(First).Sample(Position), CompileSolid(Last).Sample(Position));
});

Test('radial overview roots are extracted from the same 3D solid', () =>
{
    const Document = ConstructProject(); const Solid = CompileSolid(Document); const Direction = ConstructFrame(25, 12).Up;
    const Radius = TraceRadial(Document, Solid, Direction);
    Assert(Math.abs(Solid.Sample(Direction.map((Coordinate) => Coordinate * Radius))) < 1);
    Assert(Radius > Document.Radius);
});

Test('planet and local geometry reservations are independent of planet radius', () =>
{
    const Small = ConstructProject(); const Large = ConstructProject(8173, 6371000);
    for (const Mode of ['Planet', 'Local'])
    {
        Assert.deepEqual(ResolveSampling(Mode, 32, 32), ResolveSampling(Mode, 32, 32));
        Assert.notEqual(Small.Radius, Large.Radius);
    }
    const First = ProjectSurface(Small, {}, ResolveSampling('Planet', 32, 32, 24));
    const Last = ProjectSurface(Large, {}, ResolveSampling('Planet', 32, 32, 24));
    Assert.equal(First.Positions.byteLength, Last.Positions.byteLength);
    Assert.equal(First.VoxelCount, 0); Assert.equal(Last.VoxelCount, 0);
});

Test('CPU/GPU reservations include a cubic field and bounded output capacity', () =>
{
    for (const Budget of [8, 16, 32, 64])
    {
        const Sampling = ResolveSampling('Local', Budget, 8, 48, 10000);
        Assert.equal(Sampling.VoxelCount, 49 ** 3);
        Assert(Sampling.WorkingBytes <= Budget * 1048576);
        Assert(Sampling.DeviceBytes <= 8 * 1048576);
    }
    Assert.throws(() => ResolveSampling('Local', 8, 8, 96, 8 * 1048576));
    Assert.throws(() => ResolveSampling('Local', NaN, 8));
});

Test('local marching tetrahedra extracts BOTH the ground and buried cavity', () =>
{
    const Document = CavityPlanet();
    const Sampling = ResolveSampling('Local', 32, 32, 32);
    const Surface = ProjectSurface(Document, { Longitude: 0, Latitude: 0, Altitude: -Document.CrustDepth, Span: 16000 }, Sampling);
    Assert(Surface.Triangles > 1000); Assert.equal(Surface.Count, Surface.Triangles * 3);
    const Positions = Surface.Positions.subarray(0, Surface.Count * 3);
    Assert(Positions.every(Number.isFinite)); Assert(Surface.Colours.every(Number.isFinite));
    let Ground = false; let Cavity = false;
    for (let Ordinal = 0; Ordinal < Positions.length; Ordinal += 3)
    {
        const Along = Positions[Ordinal]; const Up = Positions[Ordinal + 1]; const North = Positions[Ordinal + 2];
        if (Up > -0.1) Ground = true;
        if (Up < -1 && Math.abs(Math.hypot(Along, Up + 3, North) - 2) < 0.1) Cavity = true;
    }
    Assert(Ground); Assert(Cavity);
});

Test('cavity facet normals point into air, not into remaining rock', () =>
{
    const Document = CavityPlanet();
    const Surface = ProjectSurface(Document, { Longitude: 0, Latitude: 0, Altitude: -Document.CrustDepth, Span: 16000 }, ResolveSampling('Local', 32, 32, 24));
    let Checked = 0;
    for (let Ordinal = 0; Ordinal < Surface.Count * 3; Ordinal += 9)
    {
        const Vertices = [0, 3, 6].map((Offset) => Array.from(Surface.Positions.subarray(Ordinal + Offset, Ordinal + Offset + 3)));
        const Centre = [0, 1, 2].map((Axis) => Vertices.reduce((Sum, Position) => Sum + Position[Axis], 0) / 3);
        if (Centre[1] > -1) continue;
        const Along = Vertices[1].map((Coordinate, Axis) => Coordinate - Vertices[0][Axis]);
        const Across = Vertices[2].map((Coordinate, Axis) => Coordinate - Vertices[0][Axis]);
        const Normal = [Along[1] * Across[2] - Along[2] * Across[1], Along[2] * Across[0] - Along[0] * Across[2], Along[0] * Across[1] - Along[1] * Across[0]];
        const TowardAir = [-Centre[0], -3 - Centre[1], -Centre[2]];
        Assert(Normal.reduce((Sum, Coordinate, Axis) => Sum + Coordinate * TowardAir[Axis], 0) >= -1e-10);
        Checked++;
    }
    Assert(Checked > 100);
});

Test('local coordinates retain metre edits on a 100,000 km radius planet', () =>
{
    const Document = ConstructProject(8173, 100000000); Document.Landforms = [];
    const Surface = ProjectSurface(Document, { Longitude: 0, Latitude: 0, Altitude: -Document.CrustDepth, Span: 1000 }, ResolveSampling('Local', 32, 32, 16));
    const Positions = Surface.Positions.subarray(0, Surface.Count * 3);
    Assert(Positions.every(Number.isFinite));
    Assert(Math.max(...Positions) <= 0.5); Assert(Math.min(...Positions) >= -0.5);
});

Test('output overflow refuses instead of returning a partial mesh', () =>
{
    const Document = CavityPlanet();
    Assert.throws(() => ProjectSurface(Document, { Longitude: 0, Latitude: 0, Altitude: -Document.CrustDepth, Span: 16000 }, { Mode: 'Local', Resolution: 16, TriangleLimit: 1 }), /No partial surface/);
});

Test('unbounded voxel or planet resolution is refused before allocation', () =>
{
    Assert.throws(() => ProjectSurface(ConstructProject(), {}, { Mode: 'Local', Resolution: 10000, TriangleLimit: 1 }));
    Assert.throws(() => ProjectSurface(ConstructProject(), {}, { Mode: 'Planet', Resolution: 10000, TriangleLimit: 1 }));
    Assert.throws(() => ProjectSurface(ConstructProject(), {}, { Mode: 'Unknown', Resolution: 24, TriangleLimit: 1 }));
});

Test('sphere radius changes preserve authored brush dimensions', () =>
{
    const Document = CavityPlanet(); const Before = JSON.stringify(Document.Impressions);
    Document.Radius = 6371000;
    Assert.equal(JSON.stringify(ValidateProject(Document).Impressions), Before);
});

Test('versioned roundtrip and undo/redo preserve volumetric operations', () =>
{
    const First = ConstructProject(); const Sequence = new RevisionSequence(First);
    const Edited = CavityPlanet(); Sequence.Replace(Edited);
    Assert(Sequence.Rewind()); Assert.deepEqual(Sequence.Document, First);
    Assert(Sequence.Advance()); Assert.deepEqual(Sequence.Document, Edited);
    Assert.deepEqual(ValidateProject(JSON.parse(JSON.stringify(Edited))), Edited);
});

Test('undo storage stays bounded', () =>
{
    const Sequence = new RevisionSequence(ConstructProject(), 6000);
    for (let Ordinal = 0; Ordinal < 50; Ordinal++) { const Edited = structuredClone(Sequence.Document); Edited.Seed++; Sequence.Replace(Edited); }
    Assert(Sequence.MeasureBytes() <= 6000); Assert(Sequence.Before.length < 24);
});

Test('legacy height-graph import refuses atomically', () =>
{
    const Sequence = new RevisionSequence(ConstructProject()); const Before = JSON.stringify(Sequence.Document);
    Assert.throws(() => Sequence.Replace({ Format: 'frontier-landmass', Revision: 1 }), /height-graph/);
    Assert.equal(JSON.stringify(Sequence.Document), Before); Assert.equal(Sequence.Before.length, 0);
});

for (const [Description, Change] of [
    ['non-finite radius', (Document) => { Document.Radius = Infinity; }],
    ['non-spherical revision', (Document) => { Document.Revision = 1; }],
    ['invalid latitude', (Document) => { Document.Landforms[0].Latitude = 91; }],
    ['duplicate identity', (Document) => { Document.Landforms[1].Identity = 1; }],
    ['non-finite axis', (Document) => { Document.Landforms[0].Along = NaN; }],
    ['negative axis', (Document) => { Document.Landforms[0].Across = -1; }],
    ['invalid seed', (Document) => { Document.Seed = 0.5; }],
    ['invalid brush', (Document) => { Document.Impressions = [{ Operation: 'Height', Longitude: 0, Latitude: 0, Altitude: 0, Radius: 100 }]; }],
    ['too many operations', (Document) => { Document.Impressions = new Array(1025); }],
])
{
    Test(`recipe refuses ${Description}`, () => { const Document = ConstructProject(); Change(Document); Assert.throws(() => ValidateProject(Document)); });
}

Test('section cap retains rock and excludes the true cavity interior', () =>
{
    const Document = CavityPlanet();
    const Positions = ProjectSection(Document, { Longitude: 0, Latitude: 0, Altitude: -Document.CrustDepth, Span: 16000 }, 0, 48);
    Assert(Positions.length > 0); Assert(Positions.every(Number.isFinite));
    for (let Ordinal = 0; Ordinal < Positions.length; Ordinal += 9)
    {
        const Up = (Positions[Ordinal + 1] + Positions[Ordinal + 4] + Positions[Ordinal + 7]) / 3;
        const North = (Positions[Ordinal + 2] + Positions[Ordinal + 5] + Positions[Ordinal + 8]) / 3;
        Assert(Math.hypot(Up + 3, North) > 1.95);
    }
});
