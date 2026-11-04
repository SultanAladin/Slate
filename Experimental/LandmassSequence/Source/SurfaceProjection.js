//============================================================================================================================================
//                                                            SURFACEPROJECTION.JS
//============================================================================================================================================
// 🧩 Bounded planet overview extraction and full three-dimensional marching-tetrahedra local volumes.

import { CompileSolid, TraceRadial, ConstructFrame, Normalize, Dot, ValidateProject } from './PlanetProjection.js';

export function ResolveSampling(Mode, RamMiB, DeviceMiB, Requested = 48, RevisionBytes = 0)
{
    if (![RamMiB, DeviceMiB, Requested, RevisionBytes].every(Number.isFinite) || RamMiB < 8 || DeviceMiB < 8 || RevisionBytes < 0) throw new Error('Invalid geometry reservation.');
    const Available = RamMiB * 1048576 - RevisionBytes * 4 - 1048576;
    if (Mode === 'Planet')
    {
        const Resolution = Math.min(64, Math.max(12, Math.floor(Requested / 2)));
        const Triangles = 12 * Resolution * Resolution;
        const WorkingBytes = Triangles * 384 + RevisionBytes * 4 + 1048576;
        if (WorkingBytes > RamMiB * 1048576 || Triangles * 240 + 1048576 > DeviceMiB * 1048576) throw new Error('Planet preview exceeds geometry reservations.');
        return { Mode, Resolution, TriangleLimit: Triangles, WorkingBytes, DeviceBytes: Triangles * 240 + 1048576, VoxelCount: 0 };
    }
    if (Mode !== 'Local') throw new Error('Unknown sampling mode.');
    const Resolution = Math.floor(Math.min(96, Math.max(16, Requested)));
    const VoxelCount = (Resolution + 1) ** 3;
    const TriangleLimit = Math.floor(Math.min(80000, (Available - VoxelCount * 4) / 384, (DeviceMiB * 1048576 - 1048576) / 240));
    if (TriangleLimit < 2048) throw new Error('Local volume exceeds the available reservation.');
    return { Mode, Resolution, VoxelCount, TriangleLimit, WorkingBytes: VoxelCount * 4 + TriangleLimit * 384 + RevisionBytes * 4 + 1048576, DeviceBytes: TriangleLimit * 240 + 1048576 };
}

function ColourAt(Altitude)
{
    if (Altitude < -100) return [0.09, 0.19, 0.22];
    if (Altitude < 500) return [0.65, 0.60, 0.43];
    if (Altitude < 6000) return [0.28, 0.38, 0.24];
    if (Altitude < 14000) return [0.39, 0.40, 0.30];
    return [0.60, 0.58, 0.51];
}

function ProjectPlanet(Document, Sampling)
{
    const Solid = CompileSolid(Document);
    const Positions = new Float32Array(Sampling.TriangleLimit * 9);
    const Colours = new Float32Array(Sampling.TriangleLimit * 9);
    const Resolution = Sampling.Resolution;
    let Written = 0;
    for (let Face = 0; Face < 6; Face++)
    {
        const Directions = [];
        for (let Row = 0; Row <= Resolution; Row++)
        {
            for (let Column = 0; Column <= Resolution; Column++)
            {
                const Along = Column / Resolution * 2 - 1;
                const Cross = Row / Resolution * 2 - 1;
                const Cube = [[1, Along, Cross], [-1, Along, Cross], [Along, 1, Cross], [Along, -1, Cross], [Along, Cross, 1], [Along, Cross, -1]][Face];
                const Direction = Normalize(Cube);
                const Radius = TraceRadial(Document, Solid, Direction);
                Directions.push(Direction.map((Coordinate) => Coordinate * Radius));
            }
        }
        for (let Row = 0; Row < Resolution; Row++)
        {
            for (let Column = 0; Column < Resolution; Column++)
            {
                const Ordinal = Row * (Resolution + 1) + Column;
                for (const Triple of [[Ordinal, Ordinal + 1, Ordinal + Resolution + 1], [Ordinal + 1, Ordinal + Resolution + 2, Ordinal + Resolution + 1]])
                {
                    const Points = Triple.map((Index) => Directions[Index]);
                    const Along = Points[1].map((Coordinate, Axis) => Coordinate - Points[0][Axis]);
                    const Cross = Points[2].map((Coordinate, Axis) => Coordinate - Points[0][Axis]);
                    const Normal = [Along[1] * Cross[2] - Along[2] * Cross[1], Along[2] * Cross[0] - Along[0] * Cross[2], Along[0] * Cross[1] - Along[1] * Cross[0]];
                    if (Dot(Normal, Points[0]) < 0) [Points[1], Points[2]] = [Points[2], Points[1]];
                    for (const Position of Points)
                    {
                        Positions.set(Position.map((Coordinate) => Coordinate / 1000), Written);
                        Colours.set(ColourAt(Math.hypot(...Position) - Document.Radius - Document.SeaLevel), Written);
                        Written += 3;
                    }
                }
            }
        }
    }
    return { Positions, Colours, Count: Written / 3, Triangles: Written / 9, VoxelCount: 0 };
}

const Corners = [[0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 1, 0], [0, 0, 1], [1, 0, 1], [1, 1, 1], [0, 1, 1]];
const Tetrahedra = [[0, 1, 2, 6], [0, 2, 3, 6], [0, 3, 7, 6], [0, 7, 4, 6], [0, 4, 5, 6], [0, 5, 1, 6]];
const Edges = [[0, 1], [0, 2], [0, 3], [1, 2], [1, 3], [2, 3]];
const CrossProduct = (Along, Across) => [Along[1] * Across[2] - Along[2] * Across[1], Along[2] * Across[0] - Along[0] * Across[2], Along[0] * Across[1] - Along[1] * Across[0]];

function ProjectVolume(Document, Extent, Sampling)
{
    const Resolution = Sampling.Resolution;
    if (!Number.isInteger(Resolution) || Resolution < 16 || Resolution > 96 || !Number.isInteger(Sampling.TriangleLimit) || Sampling.TriangleLimit < 1 || Sampling.TriangleLimit > 80000) throw new Error('Unsupported bounded volume.');
    if (![Extent.Span, Extent.Altitude, Extent.Longitude, Extent.Latitude].every(Number.isFinite) || Extent.Span < 1 || Extent.Span > Document.Radius) throw new Error('Invalid local volume extent.');
    const Frame = ConstructFrame(Extent.Longitude, Extent.Latitude);
    const Origin = Frame.Up.map((Coordinate) => Coordinate * (Document.Radius + Extent.Altitude));
    const Solid = CompileSolid(Document);
    const Side = Resolution + 1;
    const Distances = new Float32Array(Side ** 3);
    const Spacing = Extent.Span / Resolution;
    const PositionAt = (Along, Up, North) => [(Along / Resolution - 0.5) * Extent.Span, (Up / Resolution - 0.5) * Extent.Span, (North / Resolution - 0.5) * Extent.Span];
    const WorldAt = (Position) => Origin.map((Coordinate, Axis) => Coordinate + Position[0] * Frame.East[Axis] + Position[1] * Frame.Up[Axis] + Position[2] * Frame.North[Axis]);
    for (let North = 0; North < Side; North++)
    {
        for (let Up = 0; Up < Side; Up++)
        {
            for (let Along = 0; Along < Side; Along++) Distances[Along + Side * (Up + Side * North)] = Solid.Sample(WorldAt(PositionAt(Along, Up, North)));
        }
    }
    const Positions = new Float32Array(Sampling.TriangleLimit * 9);
    const Colours = new Float32Array(Sampling.TriangleLimit * 9);
    let Written = 0;
    const Emit = (First, Second, Last) =>
    {
        if (Written + 9 > Positions.length) throw new Error('Local triangle reservation reached. Reduce volume resolution or increase geometry reservations. No partial surface was accepted.');
        for (const Position of [First, Second, Last])
        {
            Positions.set(Position.map((Coordinate) => Coordinate / 1000), Written);
            Colours.set(ColourAt(Math.hypot(...WorldAt(Position)) - Document.Radius - Document.SeaLevel), Written);
            Written += 3;
        }
    };
    for (let North = 0; North < Resolution; North++)
    {
        for (let Up = 0; Up < Resolution; Up++)
        {
            for (let Along = 0; Along < Resolution; Along++)
            {
                const Scalars = Corners.map(([East, Height, Cross]) => Distances[Along + East + Side * (Up + Height + Side * (North + Cross))]);
                if (Scalars.every((Distance) => Distance >= 0) || Scalars.every((Distance) => Distance < 0)) continue;
                const Cell = Corners.map(([East, Height, Cross]) => PositionAt(Along + East, Up + Height, North + Cross));
                for (const Tetrahedron of Tetrahedra)
                {
                    const Inside = Tetrahedron.filter((Index) => Scalars[Index] < 0);
                    const Outside = Tetrahedron.filter((Index) => Scalars[Index] >= 0);
                    if (!Inside.length || !Outside.length) continue;
                    const Polygon = [];
                    for (const [First, Last] of Edges)
                    {
                        const Start = Tetrahedron[First];
                        const End = Tetrahedron[Last];
                        if ((Scalars[Start] < 0) === (Scalars[End] < 0)) continue;
                        const Fraction = Scalars[Start] / (Scalars[Start] - Scalars[End]);
                        const Position = Cell[Start].map((Coordinate, Axis) => Coordinate + (Cell[End][Axis] - Coordinate) * Fraction);
                        if (!Polygon.some((Existing) => Math.hypot(...Existing.map((Coordinate, Axis) => Coordinate - Position[Axis])) < Spacing * 1e-9)) Polygon.push(Position);
                    }
                    if (Polygon.length < 3) continue;
                    const Centre = [0, 1, 2].map((Axis) => Polygon.reduce((Sum, Position) => Sum + Position[Axis], 0) / Polygon.length);
                    const Normal = [0, 1, 2].map((Axis) => Outside.reduce((Sum, Index) => Sum + Cell[Index][Axis], 0) / Outside.length - Inside.reduce((Sum, Index) => Sum + Cell[Index][Axis], 0) / Inside.length);
                    const FirstAxis = Normalize(Polygon[0].map((Coordinate, Axis) => Coordinate - Centre[Axis]));
                    const SecondAxis = Normalize(CrossProduct(Normal, FirstAxis));
                    Polygon.sort((First, Last) =>
                    {
                        const RelativeFirst = First.map((Coordinate, Axis) => Coordinate - Centre[Axis]);
                        const RelativeLast = Last.map((Coordinate, Axis) => Coordinate - Centre[Axis]);
                        return Math.atan2(Dot(RelativeFirst, SecondAxis), Dot(RelativeFirst, FirstAxis)) - Math.atan2(Dot(RelativeLast, SecondAxis), Dot(RelativeLast, FirstAxis));
                    });
                    for (let Ordinal = 1; Ordinal < Polygon.length - 1; Ordinal++) Emit(Polygon[0], Polygon[Ordinal], Polygon[Ordinal + 1]);
                }
            }
        }
    }
    return { Positions, Colours, Count: Written / 3, Triangles: Written / 9, VoxelCount: Distances.length, Spacing };
}

export function ProjectSurface(Document, Extent, Sampling)
{
    Document = ValidateProject(Document);
    if (Sampling.Mode === 'Planet')
    {
        if (!Number.isInteger(Sampling.Resolution) || Sampling.Resolution < 12 || Sampling.Resolution > 64 || Sampling.TriangleLimit !== 12 * Sampling.Resolution ** 2) throw new Error('Unsupported bounded planet preview.');
        return ProjectPlanet(Document, Sampling);
    }
    if (Sampling.Mode !== 'Local') throw new Error('Unknown extraction mode.');
    return ProjectVolume(Document, Extent, Sampling);
}

export function ProjectSection(Document, Extent, East, Resolution = 48)
{
    if (!Number.isInteger(Resolution) || Resolution < 8 || Resolution > 48) throw new Error('Unsupported section resolution.');
    const Solid = CompileSolid(Document);
    const Frame = ConstructFrame(Extent.Longitude, Extent.Latitude);
    const Positions = [];
    const Corner = (Up, North) =>
    {
        const Position = [East, (Up / Resolution - 0.5) * Extent.Span, (North / Resolution - 0.5) * Extent.Span];
        const World = Frame.Up.map((Coordinate, Axis) => Coordinate * (Document.Radius + Extent.Altitude) + Position[0] * Frame.East[Axis] + Position[1] * Frame.Up[Axis] + Position[2] * Frame.North[Axis]);
        return { Position, Distance: Solid.Sample(World) };
    };
    for (let North = 0; North < Resolution; North++)
    {
        for (let Up = 0; Up < Resolution; Up++)
        {
            const Square = [Corner(Up, North), Corner(Up + 1, North), Corner(Up + 1, North + 1), Corner(Up, North + 1)];
            for (const Triangle of [[Square[0], Square[1], Square[2]], [Square[0], Square[2], Square[3]]])
            {
                const Polygon = [];
                for (let Ordinal = 0; Ordinal < 3; Ordinal++)
                {
                    const First = Triangle[Ordinal]; const Last = Triangle[(Ordinal + 1) % 3];
                    if (First.Distance <= 0) Polygon.push(First.Position);
                    if ((First.Distance <= 0) !== (Last.Distance <= 0))
                    {
                        const Fraction = First.Distance / (First.Distance - Last.Distance);
                        Polygon.push(First.Position.map((Coordinate, Axis) => Coordinate + (Last.Position[Axis] - Coordinate) * Fraction));
                    }
                }
                for (let Ordinal = 1; Ordinal < Polygon.length - 1; Ordinal++)
                {
                    for (const Position of [Polygon[0], Polygon[Ordinal + 1], Polygon[Ordinal]]) Positions.push(...Position.map((Coordinate) => Coordinate / 1000));
                }
            }
        }
    }
    return new Float32Array(Positions);
}
