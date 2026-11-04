//============================================================================================================================================
//                                                           ELEVATIONPROJECTION.JS
//============================================================================================================================================
// 🧩 Deterministic metre-space landforms, ordered sculpture and on-demand approximate surface distances.

export const MaximumImpressions = 2048;
export const MaximumLandforms   = 96;
export const SourceRevision     = 'c7egoist/Frontier@394fc140c33c2f2150a6343c927f23df36f8e12c';

export function Clamp(Quantity, Minimum, Maximum)
{
    return Math.min(Maximum, Math.max(Minimum, Quantity));
}

function HashCoordinate(East, North, Seed)
{
    let Bits = Math.imul(East, 374761393) ^ Math.imul(North, 668265263) ^ Math.imul(Seed, 1274126177);
    Bits = Math.imul(Bits ^ (Bits >>> 13), 1274126177);
    return ((Bits ^ (Bits >>> 16)) >>> 0) / 4294967295;
}

function InterpolateNoise(East, North, Seed)
{
    const West  = Math.floor(East);
    const South = Math.floor(North);
    const Along = East - West;
    const Cross = North - South;
    const EaseAlong = Along * Along * (3 - 2 * Along);
    const EaseCross = Cross * Cross * (3 - 2 * Cross);
    const First = HashCoordinate(West, South, Seed) * (1 - EaseAlong) + HashCoordinate(West + 1, South, Seed) * EaseAlong;
    const Last  = HashCoordinate(West, South + 1, Seed) * (1 - EaseAlong) + HashCoordinate(West + 1, South + 1, Seed) * EaseAlong;
    return (First * (1 - EaseCross) + Last * EaseCross) * 2 - 1;
}

export function ConstructProject(Seed = 8173, Span = 512000)
{
    const Unit = Span / 512000;
    return {
        Format: 'frontier-landmass', Revision: 1, SourceRevision,
        Name: 'Unnamed coast', Seed, Span, SeaLevel: 0, Seabed: -3200,
        Landforms: [
            { Identity: 1, Name: 'Western mainland', East: -48000 * Unit, North: 16000 * Unit, Along: 109000 * Unit, Across: 79000 * Unit, Height: 4600, Angle: -0.42, Roughness: 0.28, Enabled: true },
            { Identity: 2, Name: 'Northern peninsula', East: -6000 * Unit, North: -55000 * Unit, Along: 54000 * Unit, Across: 88000 * Unit, Height: 3100, Angle: 0.55, Roughness: 0.34, Enabled: true },
            { Identity: 3, Name: 'Eastern island', East: 101000 * Unit, North: 36000 * Unit, Along: 40000 * Unit, Across: 65000 * Unit, Height: 2400, Angle: -0.7, Roughness: 0.3, Enabled: true },
            { Identity: 4, Name: 'Outer island', East: 136000 * Unit, North: -58000 * Unit, Along: 21000 * Unit, Across: 29000 * Unit, Height: 1700, Angle: 0.4, Roughness: 0.23, Enabled: true },
        ],
        Impressions: [],
    };
}

export function ProjectElevation(Document, East, North)
{
    let Elevation = Document.Seabed;
    for (const Landform of Document.Landforms)
    {
        if (!Landform.Enabled) continue;
        const Cosine = Math.cos(Landform.Angle);
        const Sine   = Math.sin(Landform.Angle);
        const Along  = ((East - Landform.East) * Cosine + (North - Landform.North) * Sine) / Landform.Along;
        const Across = (-(East - Landform.East) * Sine + (North - Landform.North) * Cosine) / Landform.Across;
        const Radius = Along * Along + Across * Across;
        if (Radius > 5) continue;
        const Broad = InterpolateNoise(Along * 3.1 + 18, Across * 3.1 + 36, Document.Seed + Landform.Identity);
        const Fine  = InterpolateNoise(Along * 12.3 - 5, Across * 12.3 + 41, Document.Seed + Landform.Identity + 17);
        const Crest = InterpolateNoise(Along * 27, Across * 27, Document.Seed + 71);
        const Profile = Math.exp(-1.65 * Math.max(0, Radius + Broad * Landform.Roughness * 0.72));
        const Relief  = (Broad * 0.68 + Fine * 0.24 + Crest * 0.08) * Landform.Height * Landform.Roughness;
        Elevation = Math.max(Elevation, Document.Seabed + (Landform.Height - Document.Seabed) * Profile + Relief * Profile);
    }
    for (const Impression of Document.Impressions)
    {
        const Along = (East - Impression.East) / Impression.Radius;
        const Cross = (North - Impression.North) / Impression.Radius;
        const Squared = Along * Along + Cross * Cross;
        if (Squared >= 1) continue;
        const Weight = (1 - Squared) ** 3;
        if (Impression.Operation === 'Raise' || Impression.Operation === 'Lower') Elevation += Impression.Amount * Weight;
        else Elevation += (Impression.Target - Elevation) * Weight * Impression.Strength;
    }
    return Elevation;
}

/// 📝 A slope-normalised implicit distance estimate, not an exact/reinitialised Euclidean SDF.
/// The macro surface is a height graph; caves, overhangs and erosion are intentionally not represented here.
export function ProjectDistance(
    Document,
    East,
    Elevation,
    North,
    Interval = 10)
{
    const Surface = ProjectElevation(Document, East, North);
    const EastSlope = (ProjectElevation(Document, East + Interval, North) - ProjectElevation(Document, East - Interval, North)) / (2 * Interval);
    const NorthSlope = (ProjectElevation(Document, East, North + Interval) - ProjectElevation(Document, East, North - Interval)) / (2 * Interval);
    return (Elevation - Surface) / Math.hypot(1, EastSlope, NorthSlope);
}

export function ValidateProject(Candidate)
{
    const Refuse = (Reason) => { throw new Error(Reason); };
    const Finite = (Quantity, Minimum, Maximum) => Number.isFinite(Quantity) && Quantity >= Minimum && Quantity <= Maximum;
    if (!Candidate || Candidate.Format !== 'frontier-landmass' || Candidate.Revision !== 1) Refuse('Unsupported landscape format or revision.');
    if (!Finite(Candidate.Span, 16000, 20000000)) Refuse('World extent must be 16–20,000 km.');
    if (!Number.isInteger(Candidate.Seed) || Candidate.Seed < -2147483648 || Candidate.Seed > 2147483647) Refuse('Seed must be a signed 32-bit integer.');
    if (typeof Candidate.Name !== 'string' || Candidate.Name.length > 80) Refuse('Project name must contain at most 80 characters.');
    if (!Finite(Candidate.Seabed, -20000, -10) || !Finite(Candidate.SeaLevel, -10000, 10000)) Refuse('Invalid seabed or sea level.');
    if (!Array.isArray(Candidate.Landforms) || Candidate.Landforms.length > MaximumLandforms) Refuse('At most 96 landforms are supported in this stage.');
    if (!Array.isArray(Candidate.Impressions) || Candidate.Impressions.length > MaximumImpressions) Refuse('Sculpture recipe limit exceeded.');
    const Identities = new Set();
    for (const Landform of Candidate.Landforms)
    {
        if (!Number.isSafeInteger(Landform.Identity) || Landform.Identity < 1 || Identities.has(Landform.Identity)) Refuse('Landform identities must be positive and unique.');
        Identities.add(Landform.Identity);
        if (typeof Landform.Name !== 'string' || Landform.Name.length > 80 || typeof Landform.Enabled !== 'boolean') Refuse('Invalid landform description.');
        for (const Axis of ['East', 'North']) if (!Finite(Landform[Axis], -Candidate.Span / 2, Candidate.Span / 2)) Refuse('Landform centre is outside this world.');
        for (const Axis of ['Along', 'Across']) if (!Finite(Landform[Axis], 100, Candidate.Span * 2)) Refuse('Invalid landform radius.');
        if (!Finite(Landform.Height, 1, 20000) || !Finite(Landform.Angle, -Math.PI * 2, Math.PI * 2) || !Finite(Landform.Roughness, 0, 1)) Refuse('Invalid landform elevation, angle or relief.');
    }
    for (const Impression of Candidate.Impressions)
    {
        if (!['Raise', 'Lower', 'Flatten', 'Smooth'].includes(Impression.Operation)) Refuse('Unknown sculpture operation.');
        if (!Finite(Impression.East, -Candidate.Span / 2, Candidate.Span / 2) || !Finite(Impression.North, -Candidate.Span / 2, Candidate.Span / 2)) Refuse('Sculpture centre is outside this world.');
        if (!Finite(Impression.Radius, 10, Candidate.Span) || !Finite(Impression.Amount, -2000, 2000) || !Finite(Impression.Strength, 0, 1) || !Finite(Impression.Target, -20000, 20000)) Refuse('Invalid sculpture parameters.');
    }
    // 📝 Copy only the versioned schema; unrecognised imported properties never enter the live document.
    return {
        Format: Candidate.Format, Revision: 1, SourceRevision,
        Name: Candidate.Name, Seed: Candidate.Seed, Span: Candidate.Span,
        SeaLevel: Candidate.SeaLevel, Seabed: Candidate.Seabed,
        Landforms: Candidate.Landforms.map((Landform) => ({
            Identity: Landform.Identity, Name: Landform.Name, East: Landform.East, North: Landform.North,
            Along: Landform.Along, Across: Landform.Across, Height: Landform.Height,
            Angle: Landform.Angle, Roughness: Landform.Roughness, Enabled: Landform.Enabled,
        })),
        Impressions: Candidate.Impressions.map((Impression) => ({
            Operation: Impression.Operation, East: Impression.East, North: Impression.North,
            Radius: Impression.Radius, Amount: Impression.Amount, Strength: Impression.Strength, Target: Impression.Target,
        })),
    };
}
