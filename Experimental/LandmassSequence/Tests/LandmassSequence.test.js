//============================================================================================================================================
//                                                          LANDMASSSEQUENCE.TEST.JS
//============================================================================================================================================
// 🧩 Determinism, bounded sampling, versioned recipe refusal and reversible authoring checks.

import Test from 'node:test';
import Assert from 'node:assert/strict';
import { ConstructProject, ProjectElevation, ProjectDistance, ValidateProject, MaximumImpressions } from '../Source/ElevationProjection.js';
import { ResolveSampling, ProjectSurface } from '../Source/SurfaceProjection.js';
import { RevisionSequence } from '../Source/RevisionSequence.js';

Test('seeded landmass recipe is deterministic and not an Earth dataset', () =>
{
    const First = ConstructProject();
    const Last = ConstructProject();
    Assert.deepEqual(First, Last);
    for (const East of [-90000, -1000, 60000]) Assert.equal(ProjectElevation(First, East, 1000), ProjectElevation(Last, East, 1000));
    Last.Seed++;
    Assert.notEqual(ProjectElevation(First, -42000, 1000), ProjectElevation(Last, -42000, 1000));
});

Test('document roundtrip retains metre-space geometry', () =>
{
    const First = ConstructProject();
    Assert.deepEqual(ValidateProject(JSON.parse(JSON.stringify(First))), First);
    Assert.equal(ProjectElevation(First, 250000, 250000), First.Seabed);
    Assert(ProjectElevation(First, -48000, 16000) > 0);
});

Test('distance estimate is zero on surface, signed above/below and exact for a horizontal datum', () =>
{
    const First = ConstructProject();
    const Surface = ProjectElevation(First, -40000, 20000);
    Assert.equal(ProjectDistance(First, -40000, Surface, 20000), 0);
    Assert(ProjectDistance(First, -40000, Surface + 30, 20000) > 0);
    Assert(ProjectDistance(First, -40000, Surface - 30, 20000) < 0);
    First.Landforms = [];
    Assert.equal(ProjectDistance(First, 0, First.Seabed + 100, 0), 100);
});

Test('ordered Raise/Lower impressions affect only their support', () =>
{
    const First = ConstructProject();
    const Height = ProjectElevation(First, 0, 0);
    First.Impressions.push({ Operation: 'Raise', East: 0, North: 0, Radius: 1000, Amount: 200, Target: 0, Strength: 0.5 });
    Assert.equal(ProjectElevation(First, 0, 0), Height + 200);
    const Far = ConstructProject();
    Assert.equal(ProjectElevation(First, 2000, 0), ProjectElevation(Far, 2000, 0));
    First.Impressions.push({ ...First.Impressions[0], Operation: 'Lower', Amount: -200 });
    Assert(Math.abs(ProjectElevation(First, 0, 0) - Height) < 1e-9);
});

Test('flatten converges to a stored metre-space target', () =>
{
    const First = ConstructProject();
    First.Impressions.push({ Operation: 'Flatten', East: 0, North: 0, Radius: 1000, Amount: 200, Target: 123, Strength: 1 });
    Assert.equal(ProjectElevation(First, 0, 0), 123);
});

Test('world size does not increase sampled allocation', () =>
{
    const Small = ResolveSampling(16000, 32, 32);
    const Large = ResolveSampling(20000000, 32, 32);
    Assert.equal(Small.WorkingBytes, Large.WorkingBytes);
    Assert.equal(Small.DeviceBytes, Large.DeviceBytes);
    Assert.equal(Small.Triangles, Large.Triangles);
    Assert(Large.Spacing > Small.Spacing);
});

Test('preview respects both working reservations including revision bytes', () =>
{
    for (const Ram of [8, 16, 32, 128])
    {
        const Sampling = ResolveSampling(512000, Ram, 8, 384, 2 * 1048576);
        Assert(Sampling.WorkingBytes <= Ram * 1048576);
        Assert(Sampling.DeviceBytes <= 8 * 1048576);
        Assert(Sampling.Resolution <= 384);
    }
    Assert.throws(() => ResolveSampling(1, 8, 8, 384, 8 * 1048576));
    Assert.throws(() => ResolveSampling(1, NaN, 8));
});

Test('surface preview has finite facets, valid indices and upward winding', () =>
{
    const First = ConstructProject();
    const Sampling = ResolveSampling(First.Span, 8, 8, 32);
    const Surface = ProjectSurface(First, { East: 0, North: 0, Span: First.Span }, Sampling);
    Assert.equal(Surface.Positions.length, 33 * 33 * 3);
    Assert.equal(Surface.Indices.length, 32 * 32 * 6);
    Assert(Surface.Positions.every(Number.isFinite));
    Assert(Surface.Colours.every(Number.isFinite));
    Assert(Surface.Indices.every((Ordinal) => Ordinal < 33 * 33));
    const [FirstIndex, SecondIndex, ThirdIndex] = Surface.Indices;
    const EastFirst = Surface.Positions[SecondIndex * 3] - Surface.Positions[FirstIndex * 3];
    const NorthFirst = Surface.Positions[SecondIndex * 3 + 2] - Surface.Positions[FirstIndex * 3 + 2];
    const EastLast = Surface.Positions[ThirdIndex * 3] - Surface.Positions[FirstIndex * 3];
    const NorthLast = Surface.Positions[ThirdIndex * 3 + 2] - Surface.Positions[FirstIndex * 3 + 2];
    Assert(NorthFirst * EastLast - EastFirst * NorthLast > 0);
});

Test('local geometry rebases large geographic coordinates before float conversion', () =>
{
    const First = ConstructProject(8173, 20000000);
    const Surface = ProjectSurface(First, { East: 7000000, North: 6000000, Span: 16000 }, { Resolution: 32 });
    Assert.equal(Surface.Positions[0], -8);
    Assert.equal(Surface.Positions[2], -8);
    Assert.equal(Surface.Positions.at(-3), 8);
});

Test('preview refuses unbounded resolution', () =>
{
    Assert.throws(() => ProjectSurface(ConstructProject(), { East: 0, North: 0, Span: 1000 }, { Resolution: 100000 }));
});

Test('undo redo and branching preserve recipe equality', () =>
{
    const First = ConstructProject();
    const Sequence = new RevisionSequence(First);
    const Edited = structuredClone(First);
    Edited.Landforms[0].Height = 6000;
    Sequence.Replace(Edited);
    Assert(Sequence.Rewind());
    Assert.deepEqual(Sequence.Document, First);
    Assert(Sequence.Advance());
    Assert.deepEqual(Sequence.Document, Edited);
    Sequence.Rewind();
    Edited.Seed++;
    Sequence.Replace(Edited);
    Assert.equal(Sequence.Advance(), false);
});

Test('undo snapshots evict oldest edits under the byte reservation', () =>
{
    const Sequence = new RevisionSequence(ConstructProject(), 6000);
    for (let Ordinal = 0; Ordinal < 50; Ordinal++)
    {
        const Edited = structuredClone(Sequence.Document);
        Edited.Seed++;
        Sequence.Replace(Edited);
    }
    Assert(Sequence.MeasureBytes() <= 6000);
    Assert(Sequence.Before.length < 24);
});

Test('invalid import is atomic and unknown fields are not retained', () =>
{
    const Sequence = new RevisionSequence(ConstructProject());
    const Before = JSON.stringify(Sequence.Document);
    Assert.throws(() => Sequence.Replace({ ...Sequence.Document, Span: Infinity }));
    Assert.equal(JSON.stringify(Sequence.Document), Before);
    Assert.equal(Sequence.Before.length, 0);
    Assert.equal(ValidateProject({ ...Sequence.Document, Unknown: 'ignore' }).Unknown, undefined);
});

for (const [Description, Change] of [
    ['unsupported version', (Document) => { Document.Revision = 9; }],
    ['oversized extent', (Document) => { Document.Span = 20000001; }],
    ['non-finite coordinate', (Document) => { Document.Landforms[0].East = NaN; }],
    ['negative radius', (Document) => { Document.Landforms[0].Along = -1; }],
    ['duplicate identity', (Document) => { Document.Landforms[1].Identity = 1; }],
    ['invalid seed', (Document) => { Document.Seed = 1.5; }],
    ['excessive impressions', (Document) => { Document.Impressions = new Array(MaximumImpressions + 1); }],
    ['oversized label', (Document) => { Document.Name = 'x'.repeat(81); }],
])
{
    Test(`recipe refuses ${Description}`, () =>
    {
        const Document = ConstructProject();
        Change(Document);
        Assert.throws(() => ValidateProject(Document));
    });
}
