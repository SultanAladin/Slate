//============================================================================================================================================
//                                                        SYMBOLSPECIFICATION.CPP
//============================================================================================================================================
// 🧩 The eight declared figures, transcribed from the source's own path data, plus the mark everything else draws as.

#include "SlateUI/Interface/SymbolSpecification/Api/SymbolSpecification.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     DECLARED ARTWORK
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📝 κ scaled to the radius-two corners every Lucide container uses, so the control offsets below read as the
//    literal ordinates they are rather than as a product spelled out sixteen times.
constexpr float TwoUnitControl = 2.0f * QuarterArcControl;   // [-] - 1.10457

constexpr StrokeStep ChevronDownSteps[] =
{
    {  StrokeCommand::Origin,   6.0f,  9.0f },
    {  StrokeCommand::Segment, 12.0f, 15.0f },
    {  StrokeCommand::Segment, 18.0f,  9.0f }
};

constexpr StrokeStep ChevronRightSteps[] =
{
    {  StrokeCommand::Origin,   9.0f, 18.0f },
    {  StrokeCommand::Segment, 15.0f, 12.0f },
    {  StrokeCommand::Segment,  9.0f,  6.0f }
};

// 📝 lucide `grid-3x3` — a rounded enclosure and four interior segments. Exact; no arc approximation anywhere.
constexpr StrokeStep LatticeSteps[] =
{
    {  StrokeCommand::Enclosure,  3.0f,  3.0f, 21.0f, 21.0f, 2.0f },
    {  StrokeCommand::Origin,     3.0f,  9.0f },
    {  StrokeCommand::Segment,   21.0f,  9.0f },
    {  StrokeCommand::Origin,     3.0f, 15.0f },
    {  StrokeCommand::Segment,   21.0f, 15.0f },
    {  StrokeCommand::Origin,     9.0f,  3.0f },
    {  StrokeCommand::Segment,    9.0f, 21.0f },
    {  StrokeCommand::Origin,    15.0f,  3.0f },
    {  StrokeCommand::Segment,   15.0f, 21.0f }
};

// 📝 lucide `list` — three dot segments and three rules. The dots are zero-length segments; a round cap turns
//    each into a disc of the stroke's own diameter, which is exactly what the browser draws.
constexpr StrokeStep ColumnSteps[] =
{
    {  StrokeCommand::Origin,   3.0f,  5.0f },
    {  StrokeCommand::Segment,  3.01f, 5.0f },
    {  StrokeCommand::Origin,   3.0f, 12.0f },
    {  StrokeCommand::Segment,  3.01f,12.0f },
    {  StrokeCommand::Origin,   3.0f, 19.0f },
    {  StrokeCommand::Segment,  3.01f,19.0f },
    {  StrokeCommand::Origin,   8.0f,  5.0f },
    {  StrokeCommand::Segment, 21.0f,  5.0f },
    {  StrokeCommand::Origin,   8.0f, 12.0f },
    {  StrokeCommand::Segment, 21.0f, 12.0f },
    {  StrokeCommand::Origin,   8.0f, 19.0f },
    {  StrokeCommand::Segment, 21.0f, 19.0f }
};

// 📝 lucide `search` — a disc of radius eight at (11, 11) and the handle segment. Exact.
constexpr StrokeStep MagnifierSteps[] =
{
    {  StrokeCommand::Disc,    11.0f, 11.0f, 8.0f },
    {  StrokeCommand::Origin,  21.0f, 21.0f },
    {  StrokeCommand::Segment, 16.66f,16.66f }
};

// 📐 lucide `folder`. Every `a2 2 0 0 0` in the source path is an axis-aligned quarter arc of radius two and
//    is transcribed as one cubic with the κ control offset above. The single arc that is **not** a quarter —
//    `a2 2 0 0 1 -1.69 -.9`, subtending about 57° — is transcribed as a segment: at the 16 px this figure is
//    drawn at, that arc's sagitta is under a fifteenth of a pixel, and a wrong cubic there would be a larger
//    error than the straight chord.
// 🚧 Restore it as a cubic when the figures are re-derived against a real path intake.
constexpr StrokeStep FolderSteps[] =
{
    {  StrokeCommand::Origin,  20.0f, 20.0f },
    {  StrokeCommand::Curve,   22.0f, 18.0f, 20.0f + TwoUnitControl, 20.0f, 22.0f, 18.0f + TwoUnitControl },
    {  StrokeCommand::Segment, 22.0f,  8.0f },
    {  StrokeCommand::Curve,   20.0f,  6.0f, 22.0f, 8.0f - TwoUnitControl, 20.0f + TwoUnitControl,  6.0f },
    {  StrokeCommand::Segment, 12.10f, 6.0f },
    {  StrokeCommand::Segment, 10.41f, 5.10f },
    {  StrokeCommand::Segment,  9.60f, 3.90f },
    {  StrokeCommand::Curve,    7.93f, 3.0f,  9.60f - 0.92f,          3.90f - 0.50f, 7.93f + 0.92f, 3.0f },
    {  StrokeCommand::Segment,  4.0f,  3.0f },
    {  StrokeCommand::Curve,    2.0f,  5.0f,  4.0f - TwoUnitControl,  3.0f,  2.0f,  5.0f - TwoUnitControl },
    {  StrokeCommand::Segment,  2.0f, 18.0f },
    {  StrokeCommand::Curve,    4.0f, 20.0f,  2.0f, 18.0f + TwoUnitControl,  4.0f - TwoUnitControl, 20.0f },
    {  StrokeCommand::Close }
};

// 📐 lucide `activity` — the pulse trace. Its `a2 2` shoulders and `a.25 .25` apexes are all far below one
//    pixel at 16 px, so the figure is transcribed as the polyline through its own path ordinates. The round
//    join the stroker applies reproduces the shoulders to within the same tolerance.
constexpr StrokeStep PulseSteps[] =
{
    {  StrokeCommand::Origin,  22.0f, 12.0f },
    {  StrokeCommand::Segment, 19.52f,12.0f },
    {  StrokeCommand::Segment, 17.59f,13.46f },
    {  StrokeCommand::Segment, 15.24f,21.82f },
    {  StrokeCommand::Segment, 14.76f,21.82f },
    {  StrokeCommand::Segment,  9.24f, 2.18f },
    {  StrokeCommand::Segment,  8.76f, 2.18f },
    {  StrokeCommand::Segment,  6.41f,10.54f },
    {  StrokeCommand::Segment,  4.49f,12.0f },
    {  StrokeCommand::Segment,  2.0f, 12.0f }
};

// 📐 lucide `lightbulb` — the tooltip trigger `Controls.html` states as
//    `M15 14c.2-1 .7-1.7 1.5-2.5 1-.9 1.5-2.2 1.5-3.5A6 6 0 0 0 6 8c0 1 .2 2.2 1.5 3.5.7.9 1.2 1.5 1.5 2.5`,
//    plus the two rules `M9 18h6` and `M10 22h4`. Every relative control offset is resolved to the absolute
//    coordinate below, so the figure can be read without carrying a pen position in the reader's head.
// 📐 The `A6 6 0 0 0` is a true semicircle of radius six about (12, 8) and is the one arc here that is not
//    already a cubic. It is transcribed as **two** quarter cubics at the declared κ rather than one — a single
//    cubic across 180° is wrong by about a fortieth of the radius at its midpoint, which is a visible flat at
//    the 28 px this figure is drawn at, where the quarter-arc error is under a thousandth of a pixel.
// 💡 The glass closes symmetrically: the stream ends at (9, 14), the mirror of its own origin (15, 14) about
//    the axis x = 12. A transcription error in any one of the four shoulder cubics breaks that symmetry.
constexpr StrokeStep BulbSteps[] =
{
    {  StrokeCommand::Origin,  15.0f,      14.0f      },
    {  StrokeCommand::Curve,   16.5f,      11.5f,      15.2f,      13.0f,      15.7f,      12.3f      },
    {  StrokeCommand::Curve,   18.0f,       8.0f,      17.5f,      10.6f,      18.0f,       9.3f      },
    {  StrokeCommand::Curve,   12.0f,       2.0f,      18.0f,       4.686292f, 15.313708f,  2.0f      },
    {  StrokeCommand::Curve,    6.0f,       8.0f,       8.686292f,  2.0f,       6.0f,       4.686292f },
    {  StrokeCommand::Curve,    7.5f,      11.5f,       6.0f,       9.0f,       6.2f,      10.2f      },
    {  StrokeCommand::Curve,    9.0f,      14.0f,       8.2f,      12.4f,       8.7f,      13.0f      },
    {  StrokeCommand::Origin,   9.0f,      18.0f },
    {  StrokeCommand::Segment, 15.0f,      18.0f },
    {  StrokeCommand::Origin,  10.0f,      22.0f },
    {  StrokeCommand::Segment, 14.0f,      22.0f }
};

// 📝 lucide `eye` — the lid is two mirrored cubics meeting at the leading and trailing canthus, and the iris
//    is the concentric disc. The control ordinates are Lucide's own, not a re-fitted approximation.
constexpr StrokeStep EyeOpenSteps[] =
{
    {  StrokeCommand::Origin,   2.0f, 12.0f },
    {  StrokeCommand::Curve,   22.0f, 12.0f,  5.0f,  5.0f, 19.0f,  5.0f },
    {  StrokeCommand::Curve,    2.0f, 12.0f, 19.0f, 19.0f,  5.0f, 19.0f },
    {  StrokeCommand::Close,    0.0f,  0.0f },
    {  StrokeCommand::Disc,    12.0f, 12.0f,  3.0f }
};

// 📝 lucide `eye-off` — the same lid opened at both ends so the strike-through reads through the gap, plus
//    the cancelling diagonal. Drawn as three open outlines rather than one, which is how Lucide states it.
constexpr StrokeStep EyeClosedSteps[] =
{
    {  StrokeCommand::Origin,  10.7f,  5.1f },
    {  StrokeCommand::Curve,   22.0f, 12.0f, 11.1f,  5.0f, 18.4f,  6.9f },
    {  StrokeCommand::Origin,   6.6f,  6.6f },
    {  StrokeCommand::Curve,    2.0f, 12.0f,  4.5f,  8.0f,  3.0f, 10.0f },
    {  StrokeCommand::Curve,   17.1f, 17.1f,  5.0f, 15.0f, 10.0f, 19.0f },
    {  StrokeCommand::Origin,   9.9f,  9.9f },
    {  StrokeCommand::Curve,   14.1f, 14.1f,  9.1f, 10.7f,  9.1f, 13.3f },
    {  StrokeCommand::Origin,   2.0f,  2.0f },
    {  StrokeCommand::Segment, 22.0f, 22.0f }
};

// 📝 lucide `plus` — the two centred bars.
constexpr StrokeStep PlusCrossSteps[] =
{
    {  StrokeCommand::Origin,   5.0f, 12.0f },
    {  StrokeCommand::Segment, 19.0f, 12.0f },
    {  StrokeCommand::Origin,  12.0f,  5.0f },
    {  StrokeCommand::Segment, 12.0f, 19.0f }
};

// 📝 lucide `trash-2` — lid bar, handle, body outline and the two interior staves.
constexpr StrokeStep TrashBinSteps[] =
{
    {  StrokeCommand::Origin,   3.0f,  6.0f },
    {  StrokeCommand::Segment, 21.0f,  6.0f },
    {  StrokeCommand::Origin,   8.0f,  6.0f },
    {  StrokeCommand::Segment,  8.0f,  4.0f },
    {  StrokeCommand::Segment, 16.0f,  4.0f },
    {  StrokeCommand::Segment, 16.0f,  6.0f },
    {  StrokeCommand::Origin,   5.0f,  6.0f },
    {  StrokeCommand::Segment,  6.0f, 20.0f },
    {  StrokeCommand::Segment, 18.0f, 20.0f },
    {  StrokeCommand::Segment, 19.0f,  6.0f },
    {  StrokeCommand::Origin,  10.0f, 11.0f },
    {  StrokeCommand::Segment, 10.0f, 17.0f },
    {  StrokeCommand::Origin,  14.0f, 11.0f },
    {  StrokeCommand::Segment, 14.0f, 17.0f }
};

// 📝 lucide `settings` — the twelve-toothed cog reduced to an octagonal rosette plus the hub disc. The full
//    Lucide path carries twenty-four segments; eight teeth resolve identically at the 14 px this is drawn at.
constexpr StrokeStep GearCogSteps[] =
{
    {  StrokeCommand::Origin,  10.3f,  2.5f },
    {  StrokeCommand::Segment, 13.7f,  2.5f },
    {  StrokeCommand::Segment, 14.3f,  5.4f },
    {  StrokeCommand::Segment, 17.0f,  6.5f },
    {  StrokeCommand::Segment, 19.5f,  5.0f },
    {  StrokeCommand::Segment, 21.5f,  8.5f },
    {  StrokeCommand::Segment, 19.4f, 10.5f },
    {  StrokeCommand::Segment, 19.4f, 13.5f },
    {  StrokeCommand::Segment, 21.5f, 15.5f },
    {  StrokeCommand::Segment, 19.5f, 19.0f },
    {  StrokeCommand::Segment, 17.0f, 17.5f },
    {  StrokeCommand::Segment, 14.3f, 18.6f },
    {  StrokeCommand::Segment, 13.7f, 21.5f },
    {  StrokeCommand::Segment, 10.3f, 21.5f },
    {  StrokeCommand::Segment,  9.7f, 18.6f },
    {  StrokeCommand::Segment,  7.0f, 17.5f },
    {  StrokeCommand::Segment,  4.5f, 19.0f },
    {  StrokeCommand::Segment,  2.5f, 15.5f },
    {  StrokeCommand::Segment,  4.6f, 13.5f },
    {  StrokeCommand::Segment,  4.6f, 10.5f },
    {  StrokeCommand::Segment,  2.5f,  8.5f },
    {  StrokeCommand::Segment,  4.5f,  5.0f },
    {  StrokeCommand::Segment,  7.0f,  6.5f },
    {  StrokeCommand::Segment,  9.7f,  5.4f },
    {  StrokeCommand::Close,    0.0f,  0.0f },
    {  StrokeCommand::Disc,    12.0f, 12.0f,  3.0f }
};

// 📝 lucide `volume-2` — the driver wedge and the two radiating arcs.
constexpr StrokeStep SpeakerConeSteps[] =
{
    {  StrokeCommand::Origin,  11.0f,  5.0f },
    {  StrokeCommand::Segment,  6.0f,  9.0f },
    {  StrokeCommand::Segment,  2.0f,  9.0f },
    {  StrokeCommand::Segment,  2.0f, 15.0f },
    {  StrokeCommand::Segment,  6.0f, 15.0f },
    {  StrokeCommand::Segment, 11.0f, 19.0f },
    {  StrokeCommand::Close,    0.0f,  0.0f },
    {  StrokeCommand::Origin,  15.5f,  8.5f },
    {  StrokeCommand::Curve,   15.5f, 15.5f, 17.4f, 10.4f, 17.4f, 13.6f },
    {  StrokeCommand::Origin,  19.1f,  5.0f },
    {  StrokeCommand::Curve,   19.1f, 19.0f, 22.9f,  8.8f, 22.9f, 15.2f }
};

// 📝 lucide `sparkles` — the principal four-point star plus the two smaller ones, each stated as four cubics
//    into the star's waist. The waist control is κ at the inner radius, which is what gives the concave arm.
constexpr StrokeStep ParticleEmitSteps[] =
{
    {  StrokeCommand::Origin,   9.9f,  3.0f },
    {  StrokeCommand::Curve,   16.0f,  9.1f, 10.9f,  7.3f, 11.7f,  8.1f },
    {  StrokeCommand::Curve,    9.9f, 15.2f, 11.7f, 10.1f, 10.9f, 10.9f },
    {  StrokeCommand::Curve,    3.8f,  9.1f,  8.1f, 10.9f,  7.3f, 10.1f },
    {  StrokeCommand::Curve,    9.9f,  3.0f,  7.3f,  8.1f,  8.1f,  7.3f },
    {  StrokeCommand::Close,    0.0f,  0.0f },
    {  StrokeCommand::Origin,  18.0f,  3.0f },
    {  StrokeCommand::Segment, 19.0f,  6.0f },
    {  StrokeCommand::Segment, 22.0f,  7.0f },
    {  StrokeCommand::Segment, 19.0f,  8.0f },
    {  StrokeCommand::Segment, 18.0f, 11.0f },
    {  StrokeCommand::Segment, 17.0f,  8.0f },
    {  StrokeCommand::Segment, 14.0f,  7.0f },
    {  StrokeCommand::Segment, 17.0f,  6.0f },
    {  StrokeCommand::Close,    0.0f,  0.0f },
    {  StrokeCommand::Origin,  18.0f, 15.0f },
    {  StrokeCommand::Segment, 18.8f, 17.2f },
    {  StrokeCommand::Segment, 21.0f, 18.0f },
    {  StrokeCommand::Segment, 18.8f, 18.8f },
    {  StrokeCommand::Segment, 18.0f, 21.0f },
    {  StrokeCommand::Segment, 17.2f, 18.8f },
    {  StrokeCommand::Segment, 15.0f, 18.0f },
    {  StrokeCommand::Segment, 17.2f, 17.2f },
    {  StrokeCommand::Close,    0.0f,  0.0f }
};

// 📝 lucide `code` — the two angle brackets.
constexpr StrokeStep CodeBracketsSteps[] =
{
    {  StrokeCommand::Origin,  16.0f, 18.0f },
    {  StrokeCommand::Segment, 22.0f, 12.0f },
    {  StrokeCommand::Segment, 16.0f,  6.0f },
    {  StrokeCommand::Origin,   8.0f,  6.0f },
    {  StrokeCommand::Segment,  2.0f, 12.0f },
    {  StrokeCommand::Segment,  8.0f, 18.0f }
};

// 📝 lucide `crosshair` — the bounding disc and the four axis ticks that cross it.
constexpr StrokeStep CrosshairCentreSteps[] =
{
    {  StrokeCommand::Disc,    12.0f, 12.0f, 10.0f },
    {  StrokeCommand::Origin,  22.0f, 12.0f },
    {  StrokeCommand::Segment, 18.0f, 12.0f },
    {  StrokeCommand::Origin,   6.0f, 12.0f },
    {  StrokeCommand::Segment,  2.0f, 12.0f },
    {  StrokeCommand::Origin,  12.0f,  6.0f },
    {  StrokeCommand::Segment, 12.0f,  2.0f },
    {  StrokeCommand::Origin,  12.0f, 22.0f },
    {  StrokeCommand::Segment, 12.0f, 18.0f }
};

// 📝 lucide `box` — the isometric cube: the upper rhombus and the two falling edges from its near corner.
constexpr StrokeStep CubeSolidSteps[] =
{
    {  StrokeCommand::Origin,  21.0f,  8.0f },
    {  StrokeCommand::Segment, 12.0f,  3.0f },
    {  StrokeCommand::Segment,  3.0f,  8.0f },
    {  StrokeCommand::Segment,  3.0f, 16.0f },
    {  StrokeCommand::Segment, 12.0f, 21.0f },
    {  StrokeCommand::Segment, 21.0f, 16.0f },
    {  StrokeCommand::Close,    0.0f,  0.0f },
    {  StrokeCommand::Origin,   3.0f,  8.0f },
    {  StrokeCommand::Segment, 12.0f, 13.0f },
    {  StrokeCommand::Segment, 21.0f,  8.0f },
    {  StrokeCommand::Origin,  12.0f, 13.0f },
    {  StrokeCommand::Segment, 12.0f, 21.0f }
};

// 📝 lucide `camera` — the body with its raised shutter hood, and the lens disc.
constexpr StrokeStep CameraApertureSteps[] =
{
    {  StrokeCommand::Origin,   9.0f,  6.0f },
    {  StrokeCommand::Segment, 10.5f,  3.5f },
    {  StrokeCommand::Segment, 13.5f,  3.5f },
    {  StrokeCommand::Segment, 15.0f,  6.0f },
    {  StrokeCommand::Segment, 20.0f,  6.0f },
    {  StrokeCommand::Segment, 22.0f,  8.0f },
    {  StrokeCommand::Segment, 22.0f, 18.0f },
    {  StrokeCommand::Segment, 20.0f, 20.0f },
    {  StrokeCommand::Segment,  4.0f, 20.0f },
    {  StrokeCommand::Segment,  2.0f, 18.0f },
    {  StrokeCommand::Segment,  2.0f,  8.0f },
    {  StrokeCommand::Segment,  4.0f,  6.0f },
    {  StrokeCommand::Close,    0.0f,  0.0f },
    {  StrokeCommand::Disc,    12.0f, 13.0f,  4.0f }
};

// 📝 lucide `layers` — the three stacked lozenges, the upper one closed and the two beneath stated as the
//    visible lower chevron only, exactly as the source draws them.
constexpr StrokeStep LayerMergeSteps[] =
{
    {  StrokeCommand::Origin,  12.0f,  2.0f },
    {  StrokeCommand::Segment, 22.0f,  7.0f },
    {  StrokeCommand::Segment, 12.0f, 12.0f },
    {  StrokeCommand::Segment,  2.0f,  7.0f },
    {  StrokeCommand::Close,    0.0f,  0.0f },
    {  StrokeCommand::Origin,   2.0f, 12.0f },
    {  StrokeCommand::Segment, 12.0f, 17.0f },
    {  StrokeCommand::Segment, 22.0f, 12.0f },
    {  StrokeCommand::Origin,   2.0f, 17.0f },
    {  StrokeCommand::Segment, 12.0f, 22.0f },
    {  StrokeCommand::Segment, 22.0f, 17.0f }
};

// 📝 The one approved placeholder icon — the Lucide image SVG reduced to the engine's stroke stream. Every
//    unresolved application symbol uses it until the discipline-specific icon sets are supplied.
constexpr StrokeStep PlaceholderSteps[] =
{
    {  StrokeCommand::Enclosure,  3.0f,  3.0f, 21.0f, 21.0f, 2.0f },
    {  StrokeCommand::Disc,       8.5f,  8.5f, 1.5f },
    {  StrokeCommand::Origin,     3.0f, 18.0f },
    {  StrokeCommand::Segment,    9.0f, 12.0f },
    {  StrokeCommand::Segment,   13.0f, 16.0f },
    {  StrokeCommand::Segment,   16.0f, 13.0f },
    {  StrokeCommand::Segment,   21.0f, 18.0f }
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE ROSTER
//------------------------------------------------------------------------------------------------------------------------

constexpr SymbolFigure PlaceholderFigure =
{
    PlaceholderSteps, 7u, SymbolDiscipline::Workspace, DeclaredWeight, false
};

/// 🧩 Constructs the registration entry for a subject with no artwork yet.
constexpr SymbolFigure Unresolved(SymbolDiscipline Registered)
{
    return SymbolFigure{ PlaceholderSteps, 7u, Registered, DeclaredWeight, false };
}

constexpr SymbolFigure Roster[static_cast<std::uint32_t>(SymbolSubject::SubjectCount)] =
{
    /* FolderClosed        */ { FolderSteps,     13u, SymbolDiscipline::Workspace,   TongueWeight,   true  },
    /* LatticeArrangement  */ { LatticeSteps,     9u, SymbolDiscipline::Workspace,   DeclaredWeight, true  },
    /* ColumnArrangement   */ { ColumnSteps,     12u, SymbolDiscipline::Workspace,   DeclaredWeight, true  },
    /* PanelSplit          */ Unresolved(SymbolDiscipline::Workspace),
    /* PersistDisc         */ Unresolved(SymbolDiscipline::Workspace),
    /* BulbFilament        */ { BulbSteps,       11u, SymbolDiscipline::Workspace,   DeclaredWeight, true  },
    /* EyeOpen             */ { EyeOpenSteps,     5u, SymbolDiscipline::Workspace,   DeclaredWeight, true  },
    /* EyeClosed           */ { EyeClosedSteps,   9u, SymbolDiscipline::Workspace,   DeclaredWeight, true  },
    /* PlusCross           */ { PlusCrossSteps,   4u, SymbolDiscipline::Workspace,   DeclaredWeight, true  },
    /* TrashBin            */ { TrashBinSteps,   14u, SymbolDiscipline::Workspace,   DeclaredWeight, true  },
    /* GearCog             */ { GearCogSteps,    26u, SymbolDiscipline::Workspace,   DeclaredWeight, true  },
    /* SpeakerCone         */ { SpeakerConeSteps,11u, SymbolDiscipline::Workspace,   DeclaredWeight, true  },
    /* CodeBrackets        */ { CodeBracketsSteps,6u, SymbolDiscipline::Workspace,   DeclaredWeight, true  },

    /* ChevronDown         */ { ChevronDownSteps, 3u, SymbolDiscipline::Navigation,  DeclaredWeight, true  },
    /* ChevronRight        */ { ChevronRightSteps,3u, SymbolDiscipline::Navigation,  DeclaredWeight, true  },
    /* MagnifierLens       */ { MagnifierSteps,   3u, SymbolDiscipline::Navigation,  DeclaredWeight, true  },
    /* ArrowReturn         */ Unresolved(SymbolDiscipline::Navigation),
    /* CrosshairCentre     */ { CrosshairCentreSteps, 9u, SymbolDiscipline::Navigation, DeclaredWeight, true },

    /* VertexPoint         */ Unresolved(SymbolDiscipline::Geometry),
    /* EdgeSegment         */ Unresolved(SymbolDiscipline::Geometry),
    /* FacePlanar          */ Unresolved(SymbolDiscipline::Geometry),
    /* SubdivisionStep     */ Unresolved(SymbolDiscipline::Geometry),
    /* ExtrudeSpan         */ Unresolved(SymbolDiscipline::Geometry),
    /* BevelChamfer        */ Unresolved(SymbolDiscipline::Geometry),
    /* BooleanUnion        */ Unresolved(SymbolDiscipline::Geometry),
    /* MirrorAxis          */ Unresolved(SymbolDiscipline::Geometry),
    /* CubeSolid           */ { CubeSolidSteps,  12u, SymbolDiscipline::Geometry,    DeclaredWeight, true  },

    /* SketchPlane         */ Unresolved(SymbolDiscipline::ComputerAidedDesign),
    /* ConstraintDimension */ Unresolved(SymbolDiscipline::ComputerAidedDesign),
    /* FilletRadius        */ Unresolved(SymbolDiscipline::ComputerAidedDesign),
    /* RevolveAxis         */ Unresolved(SymbolDiscipline::ComputerAidedDesign),
    /* LoftProfile         */ Unresolved(SymbolDiscipline::ComputerAidedDesign),

    /* BristleTip          */ Unresolved(SymbolDiscipline::Sculpting),
    /* InflatePush         */ Unresolved(SymbolDiscipline::Sculpting),
    /* SmoothRelax         */ Unresolved(SymbolDiscipline::Sculpting),
    /* MaskStencil         */ Unresolved(SymbolDiscipline::Sculpting),
    /* RemeshDensity       */ Unresolved(SymbolDiscipline::Sculpting),

    /* UnwrapSeam          */ Unresolved(SymbolDiscipline::Texturing),
    /* PaintBristle        */ Unresolved(SymbolDiscipline::Texturing),
    /* MaterialSphere      */ Unresolved(SymbolDiscipline::Texturing),
    /* ChannelSelect       */ Unresolved(SymbolDiscipline::Texturing),
    /* StencilProjection   */ Unresolved(SymbolDiscipline::Texturing),

    /* SunDirectional      */ Unresolved(SymbolDiscipline::Illumination),
    /* LampPoint           */ Unresolved(SymbolDiscipline::Illumination),
    /* AreaEmitter         */ Unresolved(SymbolDiscipline::Illumination),
    /* SkyDome             */ Unresolved(SymbolDiscipline::Illumination),

    /* CameraAperture      */ { CameraApertureSteps, 14u, SymbolDiscipline::Rendering, DeclaredWeight, true },
    /* SampleConverge      */ Unresolved(SymbolDiscipline::Rendering),
    /* DenoiseSweep        */ Unresolved(SymbolDiscipline::Rendering),
    /* ExposureCoordinate    */ Unresolved(SymbolDiscipline::Rendering),

    /* KeyCoordinate         */ Unresolved(SymbolDiscipline::Animation),
    /* CurveTangent        */ Unresolved(SymbolDiscipline::Animation),
    /* TimelineScrub       */ Unresolved(SymbolDiscipline::Animation),
    /* SkeletonJoint       */ Unresolved(SymbolDiscipline::Animation),

    /* ClothDrape          */ Unresolved(SymbolDiscipline::Simulation),
    /* FluidStream         */ Unresolved(SymbolDiscipline::Simulation),
    /* RigidCollide        */ Unresolved(SymbolDiscipline::Simulation),
    /* ParticleEmit        */ { ParticleEmitSteps, 24u, SymbolDiscipline::Simulation, DeclaredWeight, true },

    /* LayerMerge          */ { LayerMergeSteps, 11u, SymbolDiscipline::Assembly,     DeclaredWeight, true  },
    /* AlphaMask           */ Unresolved(SymbolDiscipline::Assembly),
    /* ColourWheel         */ Unresolved(SymbolDiscipline::Assembly),
    /* GraphJunction       */ Unresolved(SymbolDiscipline::Assembly),

    /* PulseTrace          */ { PulseSteps,      10u, SymbolDiscipline::Measurement, TongueWeight,   true  },
    /* RulerSpan           */ Unresolved(SymbolDiscipline::Measurement),
    /* HistogramProfile    */ Unresolved(SymbolDiscipline::Measurement),
    /* StatisticReadout    */ Unresolved(SymbolDiscipline::Measurement),

    /* PlaceholderMark     */ PlaceholderFigure
};

// 📝 🔴 The roster is declared in discipline order and the registration spans below index into it. Two orderings
//    that agree only until somebody inserts a subject is exactly the disguised edge `00` §2 exists to remove,
//    so the spans are checked against the roster at compile time rather than reviewed.
constexpr SymbolSubject DisciplineOrder[] =
{
    SymbolSubject::FolderClosed,        SymbolSubject::LatticeArrangement,  SymbolSubject::ColumnArrangement,
    SymbolSubject::PanelSplit,          SymbolSubject::PersistDisc,
    SymbolSubject::BulbFilament,        SymbolSubject::EyeOpen,             SymbolSubject::EyeClosed,
    SymbolSubject::PlusCross,           SymbolSubject::TrashBin,            SymbolSubject::GearCog,
    SymbolSubject::SpeakerCone,         SymbolSubject::CodeBrackets,
    SymbolSubject::ChevronDown,         SymbolSubject::ChevronRight,        SymbolSubject::MagnifierLens,
    SymbolSubject::ArrowReturn,         SymbolSubject::CrosshairCentre,
    SymbolSubject::VertexPoint,         SymbolSubject::EdgeSegment,         SymbolSubject::FacePlanar,
    SymbolSubject::SubdivisionStep,     SymbolSubject::ExtrudeSpan,         SymbolSubject::BevelChamfer,
    SymbolSubject::BooleanUnion,        SymbolSubject::MirrorAxis,          SymbolSubject::CubeSolid,
    SymbolSubject::SketchPlane,         SymbolSubject::ConstraintDimension, SymbolSubject::FilletRadius,
    SymbolSubject::RevolveAxis,         SymbolSubject::LoftProfile,
    SymbolSubject::BristleTip,          SymbolSubject::InflatePush,         SymbolSubject::SmoothRelax,
    SymbolSubject::MaskStencil,         SymbolSubject::RemeshDensity,
    SymbolSubject::UnwrapSeam,          SymbolSubject::PaintBristle,        SymbolSubject::MaterialSphere,
    SymbolSubject::ChannelSelect,       SymbolSubject::StencilProjection,
    SymbolSubject::SunDirectional,      SymbolSubject::LampPoint,           SymbolSubject::AreaEmitter,
    SymbolSubject::SkyDome,
    SymbolSubject::CameraAperture,      SymbolSubject::SampleConverge,      SymbolSubject::DenoiseSweep,
    SymbolSubject::ExposureCoordinate,
    SymbolSubject::KeyCoordinate,         SymbolSubject::CurveTangent,        SymbolSubject::TimelineScrub,
    SymbolSubject::SkeletonJoint,
    SymbolSubject::ClothDrape,          SymbolSubject::FluidStream,         SymbolSubject::RigidCollide,
    SymbolSubject::ParticleEmit,
    SymbolSubject::LayerMerge,          SymbolSubject::AlphaMask,           SymbolSubject::ColourWheel,
    SymbolSubject::GraphJunction,
    SymbolSubject::PulseTrace,          SymbolSubject::RulerSpan,           SymbolSubject::HistogramProfile,
    SymbolSubject::StatisticReadout
};

constexpr std::uint32_t DisciplineFirst[static_cast<std::uint32_t>(SymbolDiscipline::DisciplineCount) + 1u] =
{
    0u, 13u, 18u, 27u, 32u, 37u, 42u, 46u, 50u, 54u, 58u, 62u, 66u
};

static_assert(sizeof(DisciplineOrder) / sizeof(SymbolSubject) == 66u,
              "The discipline ordering must register every subject except the placeholder mark.");

static_assert(DisciplineFirst[static_cast<std::uint32_t>(SymbolDiscipline::DisciplineCount)] == 66u,
              "The final registration boundary must reach the end of the discipline ordering.");

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE LOOKUPS
//------------------------------------------------------------------------------------------------------------------------

const SymbolFigure& Figure(SymbolSubject Subject)
{
    const std::uint32_t Ordinal = static_cast<std::uint32_t>(Subject);

    if (Ordinal >= static_cast<std::uint32_t>(SymbolSubject::SubjectCount))
        return PlaceholderFigure;

    return Roster[Ordinal];
}

SymbolDiscipline Registration(SymbolSubject Subject)
{
    return Figure(Subject).Registration;
}

std::uint32_t RegisteredIn(SymbolDiscipline Discipline, const SymbolSubject** Delivered)
{
    const std::uint32_t Ordinal = static_cast<std::uint32_t>(Discipline);

    if (Ordinal >= static_cast<std::uint32_t>(SymbolDiscipline::DisciplineCount) || Delivered == nullptr)
        return 0u;

    const std::uint32_t First = DisciplineFirst[Ordinal];
    const std::uint32_t Past  = DisciplineFirst[Ordinal + 1u];

    *Delivered = &DisciplineOrder[First];

    return Past - First;
}

const char* DisciplineText(SymbolDiscipline Discipline)
{
    switch (Discipline)
    {
        case SymbolDiscipline::Workspace:           return "Workspace";
        case SymbolDiscipline::Navigation:          return "Navigation";
        case SymbolDiscipline::Geometry:            return "Geometry";
        case SymbolDiscipline::ComputerAidedDesign: return "Computer-aided design";
        case SymbolDiscipline::Sculpting:           return "Sculpting";
        case SymbolDiscipline::Texturing:           return "Texturing";
        case SymbolDiscipline::Illumination:        return "Illumination";
        case SymbolDiscipline::Rendering:           return "Rendering";
        case SymbolDiscipline::Animation:           return "Animation";
        case SymbolDiscipline::Simulation:          return "Simulation";
        case SymbolDiscipline::Assembly:            return "Assembly";
        case SymbolDiscipline::Measurement:         return "Measurement";
        default:                                    return "";
    }
}

}   // namespace Slate
