//============================================================================================================================================
//                                                    PARAMETRICTOOLSSPECIFICATION.H
//============================================================================================================================================
// 🧩 Host-owned CAD construction-catalogue state: the active band/tool, the catalogue/detail slide, and the
//    document facts the tool field filters against.

#pragma once

#include <cstdint>

namespace Slate
{

enum class ParametricToolPage : std::uint32_t
{
    Catalogue = 0u,
    Settings = 1u,
    PageCount = 2u
};

enum class ParametricToolDimension : std::uint32_t
{
    Nothing = 0u,
    Vertex = 1u,
    Edge = 2u,
    Wire = 3u,
    Face = 4u,
    Shell = 5u,
    Solid = 6u,
    DimensionCount = 7u
};

enum class ParametricToolSubject : std::uint32_t
{
    Select = 0u,
    Workplane = 1u,
    Line = 2u,
    Polyline = 3u,
    Rectangle = 4u,
    Circle = 5u,
    Arc = 6u,
    Point = 7u,
    Fillet = 8u,
    Chamfer = 9u,
    Trim = 10u,
    Extend = 11u,
    Offset = 12u,
    Extrude = 13u,
    Revolve = 14u,
    Sweep = 15u,
    Loft = 16u,
    Boss = 17u,
    Interpolate = 18u,
    Approximate = 19u,
    Helix = 20u,
    PlanarFace = 21u,
    FillFace = 22u,
    Union = 23u,
    Cut = 24u,
    LinearArray = 25u,
    Mirror = 26u,
    DatumPlane = 27u,
    DatumAxis = 28u,
    ImportStep = 29u,
    MeshToSolid = 30u,
    TraceImage = 31u,
    PointLight = 32u,
    Camera = 33u,
    LinearDimension = 34u,
    LeaderNote = 35u,
    SubjectCount = 36u
};

struct ParametricToolsContext
{
    static constexpr std::uint32_t BandLimit = 16u;
    static constexpr std::uint32_t TileLimit = 32u;

    ParametricToolPage Page = ParametricToolPage::Catalogue;
    std::uint32_t ActiveBand = 0u;
    std::uint32_t ActiveTool = 0u;
    ParametricToolSubject ActiveSubject = ParametricToolSubject::Select;
    bool ShowGated = true;

    ParametricToolDimension ActiveDimension = ParametricToolDimension::Nothing;
    std::uint32_t SelectedCount = 0u;
    std::uint32_t ProfileCount = 0u;
    std::uint32_t PerimeterEdgeCount = 0u;
    std::uint32_t ExistingCircleCount = 0u;
    std::uint32_t SolidCount = 0u;

    bool WorkplaneActivation = false;
    bool ClosedProfileCondition = false;
    bool PlanarProfileCondition = true;
    bool AxisAvailability = false;
    bool PathAvailability = false;
    bool UniformClosureCondition = true;
    bool PendingGeometryCondition = false;
    bool SupportMaterialCondition = false;
    bool TangentEndpointCondition = false;
    bool OpeningCondition = false;
    bool ReferencePlaneCondition = false;
    bool SourceImageryCondition = false;
    bool MeasurableCondition = false;

    bool ConstructionGeometry = false;
    bool LineLengthAssist = false;
    bool LineAngleAssist = false;
    double LineLength = 100.0;
    double LineAngleDegrees = 0.0;
    bool RectangleDimensionAssist = false;
    double RectangleWidth = 120.0;
    double RectangleHeight = 80.0;
    bool CircleRadiusAssist = false;
    bool CircleDiameterMode = false;
    double CircleRadius = 40.0;
};

const char* ParametricToolDimensionText(ParametricToolDimension Subject);

} // namespace Slate
