//============================================================================================================================================
//                                                            SURFACEPROJECTION.JS
//============================================================================================================================================
// 🧩 Budget-bounded surface sampling with double-precision geographic coordinates and camera-relative facets.

import { ProjectElevation, Clamp } from './ElevationProjection.js';

export function ResolveSampling(
    Span,
    RamMiB,
    DeviceMiB,
    Requested     = 192,
    RevisionBytes = 0)
{
    if (![Span, RamMiB, DeviceMiB, Requested, RevisionBytes].every(Number.isFinite) || Span <= 0 || RamMiB < 8 || DeviceMiB < 8 || RevisionBytes < 0) throw new Error('Invalid preview budget.');
    // 📝 Conservative geometry working-set reservations, not total browser/driver memory accounting.
    const Available = Math.max(0, RamMiB * 1048576 - RevisionBytes);
    const RamLimit = Math.floor(Math.sqrt(Available / 192)) - 1;
    const DeviceLimit = Math.floor(Math.sqrt(DeviceMiB * 1048576 / 80)) - 1;
    const Resolution = Math.min(Math.floor(Requested), 384, RamLimit, DeviceLimit);
    if (Resolution < 32) throw new Error('Insufficient preview reservation after undo storage.');
    return {
        Resolution, Spacing: Span / Resolution,
        WorkingBytes: (Resolution + 1) ** 2 * 192 + RevisionBytes,
        DeviceBytes: (Resolution + 1) ** 2 * 80,
        Triangles: Resolution * Resolution * 2,
    };
}

export function ProjectSurface(Document, Extent, Sampling)
{
    const Resolution = Sampling.Resolution;
    if (!Number.isInteger(Resolution) || Resolution < 32 || Resolution > 384) throw new Error('Unsupported sample resolution.');
    if (![Extent.Span, Extent.East, Extent.North].every(Number.isFinite) || Extent.Span <= 0 || Extent.Span > Document.Span) throw new Error('Invalid preview extent.');
    const Count = (Resolution + 1) ** 2;
    const Positions = new Float32Array(Count * 3);
    const Colours   = new Float32Array(Count * 3);
    const Indices   = new Uint32Array(Resolution * Resolution * 6);
    let Minimum = Infinity;
    let Maximum = -Infinity;
    for (let Row = 0; Row <= Resolution; Row++)
    {
        for (let Column = 0; Column <= Resolution; Column++)
        {
            const Ordinal = Row * (Resolution + 1) + Column;
            const East = (Column / Resolution - 0.5) * Extent.Span;
            const North = (Row / Resolution - 0.5) * Extent.Span;
            const Height = ProjectElevation(Document, East + Extent.East, North + Extent.North);
            Minimum = Math.min(Minimum, Height);
            Maximum = Math.max(Maximum, Height);
            Positions[Ordinal * 3]     = East / 1000;
            Positions[Ordinal * 3 + 1] = Height / 1000;
            Positions[Ordinal * 3 + 2] = North / 1000;
            const AboveSea = Height - Document.SeaLevel;
            let Colour;
            if (AboveSea < -100) Colour = [0.10, 0.23, 0.25];
            else if (AboveSea < 100) Colour = [0.64, 0.59, 0.43];
            else if (AboveSea < 1100) Colour = [0.28, 0.37, 0.24];
            else if (AboveSea < 2400) Colour = [0.37, 0.39, 0.29];
            else if (AboveSea < 3400) Colour = [0.48, 0.45, 0.37];
            else Colour = [0.72, 0.71, 0.66];
            const Variation = Clamp(1 + Math.sin(Height * 0.001) * 0.08, 0.85, 1.15);
            Colours.set(Colour.map((Channel) => Channel * Variation), Ordinal * 3);
            if (Column < Resolution && Row < Resolution)
            {
                const Offset = (Row * Resolution + Column) * 6;
                Indices.set([Ordinal, Ordinal + Resolution + 1, Ordinal + 1, Ordinal + 1, Ordinal + Resolution + 1, Ordinal + Resolution + 2], Offset);
            }
        }
    }
    return { Positions, Colours, Indices, Minimum, Maximum };
}
