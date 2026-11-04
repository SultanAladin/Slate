//============================================================================================================================================
//                                                            PLANETPROJECTION.JS
//============================================================================================================================================
// 🧩 Spherical metre-space CSG solid with volumetric ellipsoids, subtractive cavities and geographic anchors.

export const SourceRevision = 'c7egoist/Frontier@394fc140c33c2f2150a6343c927f23df36f8e12c';
export const MaximumLandforms = 96;
export const MaximumImpressions = 1024;
export const Clamp = (Quantity, Minimum, Maximum) => Math.max(Minimum, Math.min(Maximum, Quantity));
export const Dot = (First, Last) => First[0] * Last[0] + First[1] * Last[1] + First[2] * Last[2];
export const Normalize = (Direction) => { const Length = Math.hypot(...Direction); return Direction.map((Coordinate) => Coordinate / Length); };

export function ConstructFrame(Longitude, Latitude)
{
    const Along = Longitude * Math.PI / 180;
    const North = Latitude * Math.PI / 180;
    return {
        Up: [Math.cos(North) * Math.cos(Along), Math.sin(North), Math.cos(North) * Math.sin(Along)],
        East: [-Math.sin(Along), 0, Math.cos(Along)],
        North: [-Math.sin(North) * Math.cos(Along), Math.cos(North), -Math.sin(North) * Math.sin(Along)],
    };
}

export function ProjectAnchor(Document, Position)
{
    const Radial = Math.hypot(...Position);
    return {
        Longitude: Math.atan2(Position[2], Position[0]) * 180 / Math.PI,
        Latitude: Math.asin(Clamp(Position[1] / Radial, -1, 1)) * 180 / Math.PI,
        Altitude: Radial - Document.Radius,
    };
}

export function ProjectPosition(Document, Anchor)
{
    return ConstructFrame(Anchor.Longitude, Anchor.Latitude).Up.map((Coordinate) => Coordinate * (Document.Radius + Anchor.Altitude));
}

export function ConstructProject(Seed = 8173, Radius = 3390000)
{
    return {
        Format: 'frontier-sdf-planet', Revision: 2, SourceRevision,
        Name: 'Unnamed planet', Seed, Radius, CrustDepth: 24000, SeaLevel: 0,
        Landforms: [
            { Identity: 1, Name: 'Veyra continent', Longitude: 25, Latitude: 12, Along: 1550000, Across: 1050000, Height: 22000, Angle: -0.4, Roughness: 0.12, Enabled: true },
            { Identity: 2, Name: 'Northern reach', Longitude: 13, Latitude: 41, Along: 800000, Across: 1200000, Height: 18000, Angle: 0.4, Roughness: 0.14, Enabled: true },
            { Identity: 3, Name: 'Eastern island', Longitude: 72, Latitude: 0, Along: 610000, Across: 920000, Height: 14000, Angle: -0.6, Roughness: 0.16, Enabled: true },
            { Identity: 4, Name: 'Southern island', Longitude: -18, Latitude: -32, Along: 430000, Across: 670000, Height: 12000, Angle: 0.8, Roughness: 0.14, Enabled: true },
        ],
        Impressions: [],
    };
}

function EllipsoidDistance(Along, Up, North, Width, Height, Depth)
{
    const First = Math.hypot(Along / Width, Up / Height, North / Depth);
    const Last = Math.hypot(Along / (Width * Width), Up / (Height * Height), North / (Depth * Depth));
    return Last > 1e-20 ? First * (First - 1) / Last : -Math.min(Width, Height, Depth);
}

export function CompileSolid(Document)
{
    const Landforms = Document.Landforms.filter((Landform) => Landform.Enabled).map((Landform) => ({
        ...Landform, Frame: ConstructFrame(Landform.Longitude, Landform.Latitude),
        Cosine: Math.cos(Landform.Angle), Sine: Math.sin(Landform.Angle),
        RadialRadius: (Landform.Height + Document.CrustDepth) * 1.3,
    }));
    const Impressions = Document.Impressions.map((Impression) => ({
        ...Impression, Centre: ProjectPosition(Document, Impression),
    }));
    // 📝 This is a 3D CSG distance field, NOT a sampled elevation function. Ellipsoid/warped fields are
    // distance estimates (not an exact reinitialised Euclidean SDF); extraction uses signs, not unsafe sphere tracing.
    const Sample = (Position) =>
    {
        const Radial = Math.hypot(...Position);
        let Distance = Radial - (Document.Radius - Document.CrustDepth);
        if (Radial > 1)
        {
            const Direction = Position.map((Coordinate) => Coordinate / Radial);
            for (const Landform of Landforms)
            {
                const Cosine = Clamp(Dot(Direction, Landform.Frame.Up), -1, 1);
                if (Cosine < 0.15) continue;
                const Arc = Math.acos(Cosine);
                const Scale = Arc < 1e-8 ? Document.Radius : Document.Radius * Arc / Math.sqrt(Math.max(1e-16, 1 - Cosine * Cosine));
                const East = Dot(Direction, Landform.Frame.East) * Scale;
                const North = Dot(Direction, Landform.Frame.North) * Scale;
                const Along = East * Landform.Cosine + North * Landform.Sine;
                const Across = -East * Landform.Sine + North * Landform.Cosine;
                if (Math.abs(Along) > Landform.Along * 1.5 || Math.abs(Across) > Landform.Across * 1.5) continue;
                const Up = Radial - Document.Radius - (Landform.Height - Landform.RadialRadius);
                let Rock = EllipsoidDistance(Along, Up, Across, Landform.Along, Landform.RadialRadius, Landform.Across);
                const Noise = Math.sin(Along / Landform.Along * 19 + Document.Seed * 0.13) * Math.sin(Across / Landform.Across * 23 - Document.Seed * 0.17) * Math.cos(Up / Landform.RadialRadius * 6);
                Rock += Noise * Landform.Roughness * Landform.RadialRadius * 0.35;
                Distance = Math.min(Distance, Rock);
            }
        }
        for (const Impression of Impressions)
        {
            const Sphere = Math.hypot(Position[0] - Impression.Centre[0], Position[1] - Impression.Centre[1], Position[2] - Impression.Centre[2]) - Impression.Radius;
            Distance = Impression.Operation === 'Union' ? Math.min(Distance, Sphere) : Math.max(Distance, -Sphere);
        }
        return Distance;
    };
    const OuterAltitude = Math.max(1000, ...Landforms.map((Landform) => Landform.Height + Landform.RadialRadius * Landform.Roughness), ...Impressions.filter((Impression) => Impression.Operation === 'Union').map((Impression) => Impression.Altitude + Impression.Radius));
    return { Sample, OuterAltitude };
}

export function TraceRadial(Document, Solid, Direction)
{
    const Outside = Document.Radius + Solid.OuterAltitude + 100;
    const Inside = Document.Radius - Document.CrustDepth - Math.max(10000, ...Document.Impressions.map((Impression) => Impression.Radius * 2));
    let Previous = Outside;
    for (let Ordinal = 1; Ordinal <= 64; Ordinal++)
    {
        const Radius = Outside + (Inside - Outside) * Ordinal / 64;
        if (Solid.Sample(Direction.map((Coordinate) => Coordinate * Radius)) <= 0)
        {
            let Lower = Radius;
            let Upper = Previous;
            for (let Refinement = 0; Refinement < 16; Refinement++)
            {
                const Middle = (Lower + Upper) / 2;
                if (Solid.Sample(Direction.map((Coordinate) => Coordinate * Middle)) > 0) Upper = Middle;
                else Lower = Middle;
            }
            return (Lower + Upper) / 2;
        }
        Previous = Radius;
    }
    throw new Error('Overview ray found no solid; reduce an oversized subtractive edit or inspect its local volume.');
}

export function ValidateProject(Candidate)
{
    const Refuse = (Reason) => { throw new Error(Reason); };
    const Finite = (Quantity, Minimum, Maximum) => Number.isFinite(Quantity) && Quantity >= Minimum && Quantity <= Maximum;
    if (!Candidate || Candidate.Format !== 'frontier-sdf-planet' || Candidate.Revision !== 2) Refuse('This editor requires a revision-2 volumetric planet. Revision-1 height-graph files are not silently converted.');
    if (!Finite(Candidate.Radius, 100000, 100000000)) Refuse('Planet radius must be 100–100,000 km.');
    if (!Number.isInteger(Candidate.Seed) || Candidate.Seed < -2147483648 || Candidate.Seed > 2147483647) Refuse('Invalid seed.');
    if (typeof Candidate.Name !== 'string' || Candidate.Name.length > 80) Refuse('Invalid planet name.');
    if (!Finite(Candidate.CrustDepth, 100, Candidate.Radius / 3) || !Finite(Candidate.SeaLevel, -Candidate.Radius / 10, Candidate.Radius / 10)) Refuse('Invalid crust depth or sea datum.');
    if (!Array.isArray(Candidate.Landforms) || Candidate.Landforms.length > MaximumLandforms || !Array.isArray(Candidate.Impressions) || Candidate.Impressions.length > MaximumImpressions) Refuse('Recipe complexity limit exceeded.');
    const Identities = new Set();
    const Geographic = (Anchor) => Finite(Anchor.Longitude, -180, 180) && Finite(Anchor.Latitude, -90, 90);
    for (const Landform of Candidate.Landforms)
    {
        if (!Number.isSafeInteger(Landform.Identity) || Landform.Identity < 1 || Identities.has(Landform.Identity)) Refuse('Landform identities must be positive and unique.');
        Identities.add(Landform.Identity);
        if (!Geographic(Landform) || typeof Landform.Name !== 'string' || Landform.Name.length > 80 || typeof Landform.Enabled !== 'boolean') Refuse('Invalid landform anchor.');
        if (!Finite(Landform.Along, 10, Candidate.Radius * 1.5) || !Finite(Landform.Across, 10, Candidate.Radius * 1.5) || !Finite(Landform.Height, 1, Candidate.Radius / 10) || !Finite(Landform.Angle, -7, 7) || !Finite(Landform.Roughness, 0, 0.5)) Refuse('Invalid solid dimensions.');
    }
    for (const Impression of Candidate.Impressions)
    {
        if (!Geographic(Impression) || !['Union', 'Difference'].includes(Impression.Operation) || !Finite(Impression.Altitude, -Candidate.Radius / 2, Candidate.Radius / 2) || !Finite(Impression.Radius, 1, Candidate.Radius / 4)) Refuse('Invalid volumetric brush.');
    }
    return {
        Format: Candidate.Format, Revision: 2, SourceRevision, Name: Candidate.Name,
        Seed: Candidate.Seed, Radius: Candidate.Radius, CrustDepth: Candidate.CrustDepth, SeaLevel: Candidate.SeaLevel,
        Landforms: Candidate.Landforms.map((Landform) => ({
            Identity: Landform.Identity, Name: Landform.Name, Longitude: Landform.Longitude, Latitude: Landform.Latitude,
            Along: Landform.Along, Across: Landform.Across, Height: Landform.Height, Angle: Landform.Angle, Roughness: Landform.Roughness, Enabled: Landform.Enabled,
        })),
        Impressions: Candidate.Impressions.map((Impression) => ({
            Operation: Impression.Operation, Longitude: Impression.Longitude, Latitude: Impression.Latitude,
            Altitude: Impression.Altitude, Radius: Impression.Radius,
        })),
    };
}
