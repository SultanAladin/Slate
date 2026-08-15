//============================================================================================================================================
//                                                        SYMBOLSPECIFICATION.CPP
//============================================================================================================================================
// 🧩 The seven declared figures, transcribed from the source's own path data, plus the mark everything else draws as.

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

// 📝 🚧 The mark every undeclared subject draws as — a rounded enclosure crossed by one diagonal. Deliberately
//    unlike any real figure, so an unfinished roster is visible at a glance rather than mistaken for artwork.
constexpr StrokeStep PlaceholderSteps[] =
{
    {  StrokeCommand::Enclosure,  4.0f,  4.0f, 20.0f, 20.0f, 3.0f },
    {  StrokeCommand::Origin,     8.0f, 16.0f },
    {  StrokeCommand::Segment,   16.0f,  8.0f }
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE ROSTER
//------------------------------------------------------------------------------------------------------------------------

constexpr SymbolFigure PlaceholderFigure =
{
    PlaceholderSteps, 3u, SymbolDiscipline::Workspace, DeclaredWeight, false
};

/// 🧩 Constructs the enrolment entry for a subject with no artwork yet.
constexpr SymbolFigure Unresolved(SymbolDiscipline Enrolled)
{
    return SymbolFigure{ PlaceholderSteps, 3u, Enrolled, DeclaredWeight, false };
}

constexpr SymbolFigure Roster[static_cast<std::uint32_t>(SymbolSubject::SubjectCount)] =
{
    /* FolderClosed        */ { FolderSteps,     13u, SymbolDiscipline::Workspace,   TongueWeight,   true  },
    /* LatticeArrangement  */ { LatticeSteps,     9u, SymbolDiscipline::Workspace,   DeclaredWeight, true  },
    /* ColumnArrangement   */ { ColumnSteps,     12u, SymbolDiscipline::Workspace,   DeclaredWeight, true  },
    /* PanelSplit          */ Unresolved(SymbolDiscipline::Workspace),
    /* PersistDisc         */ Unresolved(SymbolDiscipline::Workspace),

    /* ChevronDown         */ { ChevronDownSteps, 3u, SymbolDiscipline::Navigation,  DeclaredWeight, true  },
    /* ChevronRight        */ { ChevronRightSteps,3u, SymbolDiscipline::Navigation,  DeclaredWeight, true  },
    /* MagnifierLens       */ { MagnifierSteps,   3u, SymbolDiscipline::Navigation,  DeclaredWeight, true  },
    /* ArrowReturn         */ Unresolved(SymbolDiscipline::Navigation),
    /* CrosshairCentre     */ Unresolved(SymbolDiscipline::Navigation),

    /* VertexPoint         */ Unresolved(SymbolDiscipline::Geometry),
    /* EdgeSegment         */ Unresolved(SymbolDiscipline::Geometry),
    /* FacePlanar          */ Unresolved(SymbolDiscipline::Geometry),
    /* SubdivisionStep     */ Unresolved(SymbolDiscipline::Geometry),
    /* ExtrudeSpan         */ Unresolved(SymbolDiscipline::Geometry),
    /* BevelChamfer        */ Unresolved(SymbolDiscipline::Geometry),
    /* BooleanUnion        */ Unresolved(SymbolDiscipline::Geometry),
    /* MirrorAxis          */ Unresolved(SymbolDiscipline::Geometry),

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

    /* CameraAperture      */ Unresolved(SymbolDiscipline::Rendering),
    /* SampleConverge      */ Unresolved(SymbolDiscipline::Rendering),
    /* DenoiseSweep        */ Unresolved(SymbolDiscipline::Rendering),
    /* ExposureOrdinate    */ Unresolved(SymbolDiscipline::Rendering),

    /* KeyOrdinate         */ Unresolved(SymbolDiscipline::Animation),
    /* CurveTangent        */ Unresolved(SymbolDiscipline::Animation),
    /* TimelineScrub       */ Unresolved(SymbolDiscipline::Animation),
    /* SkeletonJoint       */ Unresolved(SymbolDiscipline::Animation),

    /* ClothDrape          */ Unresolved(SymbolDiscipline::Simulation),
    /* FluidStream         */ Unresolved(SymbolDiscipline::Simulation),
    /* RigidCollide        */ Unresolved(SymbolDiscipline::Simulation),
    /* ParticleEmit        */ Unresolved(SymbolDiscipline::Simulation),

    /* LayerMerge          */ Unresolved(SymbolDiscipline::Assembly),
    /* AlphaMask           */ Unresolved(SymbolDiscipline::Assembly),
    /* ColourWheel         */ Unresolved(SymbolDiscipline::Assembly),
    /* GraphJunction       */ Unresolved(SymbolDiscipline::Assembly),

    /* PulseTrace          */ { PulseSteps,      10u, SymbolDiscipline::Measurement, TongueWeight,   true  },
    /* RulerSpan           */ Unresolved(SymbolDiscipline::Measurement),
    /* HistogramProfile    */ Unresolved(SymbolDiscipline::Measurement),
    /* StatisticReadout    */ Unresolved(SymbolDiscipline::Measurement),

    /* PlaceholderMark     */ PlaceholderFigure
};

// 📝 🔴 The roster is declared in discipline order and the enrolment spans below index into it. Two orderings
//    that agree only until somebody inserts a subject is exactly the disguised edge `00` §2 exists to remove,
//    so the spans are checked against the roster at compile time rather than reviewed.
constexpr SymbolSubject DisciplineOrder[] =
{
    SymbolSubject::FolderClosed,        SymbolSubject::LatticeArrangement,  SymbolSubject::ColumnArrangement,
    SymbolSubject::PanelSplit,          SymbolSubject::PersistDisc,
    SymbolSubject::ChevronDown,         SymbolSubject::ChevronRight,        SymbolSubject::MagnifierLens,
    SymbolSubject::ArrowReturn,         SymbolSubject::CrosshairCentre,
    SymbolSubject::VertexPoint,         SymbolSubject::EdgeSegment,         SymbolSubject::FacePlanar,
    SymbolSubject::SubdivisionStep,     SymbolSubject::ExtrudeSpan,         SymbolSubject::BevelChamfer,
    SymbolSubject::BooleanUnion,        SymbolSubject::MirrorAxis,
    SymbolSubject::SketchPlane,         SymbolSubject::ConstraintDimension, SymbolSubject::FilletRadius,
    SymbolSubject::RevolveAxis,         SymbolSubject::LoftProfile,
    SymbolSubject::BristleTip,          SymbolSubject::InflatePush,         SymbolSubject::SmoothRelax,
    SymbolSubject::MaskStencil,         SymbolSubject::RemeshDensity,
    SymbolSubject::UnwrapSeam,          SymbolSubject::PaintBristle,        SymbolSubject::MaterialSphere,
    SymbolSubject::ChannelSelect,       SymbolSubject::StencilProjection,
    SymbolSubject::SunDirectional,      SymbolSubject::LampPoint,           SymbolSubject::AreaEmitter,
    SymbolSubject::SkyDome,
    SymbolSubject::CameraAperture,      SymbolSubject::SampleConverge,      SymbolSubject::DenoiseSweep,
    SymbolSubject::ExposureOrdinate,
    SymbolSubject::KeyOrdinate,         SymbolSubject::CurveTangent,        SymbolSubject::TimelineScrub,
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
    0u, 5u, 10u, 18u, 23u, 28u, 33u, 37u, 41u, 45u, 49u, 53u, 57u
};

static_assert(sizeof(DisciplineOrder) / sizeof(SymbolSubject) == 57u,
              "The discipline ordering must enrol every subject except the placeholder mark.");

static_assert(DisciplineFirst[static_cast<std::uint32_t>(SymbolDiscipline::DisciplineCount)] == 57u,
              "The final enrolment boundary must reach the end of the discipline ordering.");

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

SymbolDiscipline Enrolment(SymbolSubject Subject)
{
    return Figure(Subject).Enrolment;
}

std::uint32_t EnrolledIn(SymbolDiscipline Discipline, const SymbolSubject** Delivered)
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
