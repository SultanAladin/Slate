//============================================================================================================================================
//                                             SHAREDCADWORKSPACERUNTIME.H
//============================================================================================================================================
// Shared CAD workspace state used by the Editor and Parametric Sketch hosts.

#pragma once

#include "SlateFeature/Sketch/SketchSnap/Api/SketchSnap.h"
#include "SlateFeature/Sketch/SketchEditing/Api/SketchEditing.h"
#include "SlateFeature/Sketch/SketchStructure/Api/SketchStructure.h"
#include "SlateFeature/Feature/WorkspaceRecordStructure/Api/WorkspaceRecordStructure.h"
#include "SlateFeature/Feature/WorkspaceRevisionSequence/Api/WorkspaceRevisionSequence.h"
#include "Application/Api/SharedViewportHostBridge.h"

#include <cstdint>
#include <vector>

namespace Slate
{

struct SpatialBasis
{
    SpatialPoint Origin = {};
    SpatialDirection Along = { 1.0, 0.0, 0.0 };
    SpatialDirection Across = { 0.0, 0.0, 1.0 };
    SpatialDirection Normal = { 0.0, 1.0, 0.0 };
};

enum class ParametricViewOrientation : std::uint32_t
{
    Top = 0u,
    Bottom = 1u,
    Front = 2u,
    Back = 3u,
    Left = 4u,
    Right = 5u,
    Isometric = 6u
};

struct ParametricViewportState
{
    ParametricViewOrientation Orientation = ParametricViewOrientation::Top;
    SpatialPoint Focus = {};
    double Distance = 240.0;
    double OrthoScale = 3.0;
    double OrbitYaw = 45.0;
    double OrbitPitch = 30.0;
};

enum class ParametricDraftSubject : std::uint32_t
{
    None = 0u, Line = 1u, Rectangle = 2u, Circle = 3u, Arc = 4u, Polyline = 5u,
    LinearDimension = 6u, Point = 7u, Ellipse = 8u, Bezier = 9u, EllipticalArc = 10u,
    BasisSpline = 11u, CenterRectangle = 12u, ThreePointRectangle = 13u,
    DiameterCircle = 14u, ThreePointCircle = 15u, CenterStartEndArc = 16u,
    TangentArc = 17u, Polygon = 18u, Slot = 19u, Hermite = 20u, RationalSpline = 21u
};

struct ParametricDraftState
{
    ParametricDraftSubject Subject = ParametricDraftSubject::None;
    std::vector<SpatialPoint> Anchors = {};
    std::vector<SketchSnapPlacement> AnchorSnaps = {};
    bool HoverStanding = false;
    SpatialPoint Hover = {};
    SketchSnapPlacement Snap = {};
    bool Construction = false;
};

enum class ParametricSelectionSubject : std::uint32_t
{
    None = 0u, Point = 1u, Control = 2u, Curve = 3u, Record = 4u
};

enum class ParametricTransformMode : std::uint32_t
{
    Move = 0u, Rotate = 1u, Scale = 2u
};

enum class ParametricTransformConstraint : std::uint32_t
{
    Free = 0u, AxisX = 1u, AxisZ = 2u, Screen = 3u, Curve = 4u
};

struct ParametricViewportSelection
{
    ParametricSelectionSubject Subject = ParametricSelectionSubject::None;
    WorkspaceRecordName Record = {};
    SketchPointName Point = {};
    SketchControlName Control = {};
    SketchCurveName Curve = {};
    SpatialPoint Position = {};

    bool Standing() const { return Subject != ParametricSelectionSubject::None; }
};

struct ParametricTransformPlacement
{
    bool ControlPlacement = false;
    SketchPointName Point = {};
    SketchControlName Control = {};
    SpatialPoint Position = {};
};

struct ParametricTransformState
{
    ParametricTransformMode Mode = ParametricTransformMode::Move;
    bool Engaged = false;
    bool AwaitingRelease = false;
    bool Changed = false;
    bool SlideAlongCurve = false;
    ParametricTransformConstraint Constraint = ParametricTransformConstraint::Free;
    ParametricViewportSelection Target = {};
    WorkspaceRecordName Record = {};
    std::vector<ParametricTransformPlacement> Placements = {};
    std::vector<SpatialPoint> Origins = {};
    SpatialPoint Pivot = {};
    SpatialPoint StartReference = {};
    double PivotAlong = 0.0;
    double PivotAcross = 0.0;
    double StartAlong = 0.0;
    double StartAcross = 0.0;
    double StartDistance = 1.0;
    double StartAngle = 0.0;
    SpatialDirection CurveDirection = { 1.0, 0.0, 0.0 };
    char Numeric[32] = {};
    double PreviewValue = 0.0;
};

enum class ParametricGizmoHandle : std::uint32_t
{
    None = 0u, MoveFree = 1u, MoveX = 2u, MoveZ = 3u, Rotate = 4u,
    ScaleFree = 5u, ScaleX = 6u, ScaleZ = 7u
};

struct ParametricTransformCommandInput
{
    std::uint32_t MoveTapCount = 0u;
    bool StartRequested = false;
    ParametricTransformMode StartMode = ParametricTransformMode::Move;
    bool ConstraintRequested = false;
    ParametricTransformConstraint Constraint = ParametricTransformConstraint::Free;
    char NumericAppend[32] = {};
};

struct SharedCadWorkspaceRuntime
{
    SketchStructure Sketch;
    WorkspaceRecordStructure Records;
    WorkspaceRevisionSequence Revisions;
    SharedViewportCameraState Camera;
    ParametricViewportState View;
    ParametricDraftState Draft;
    ParametricViewportSelection SemanticSelection;
    ParametricViewportSelection HoveredSelection;
    ParametricTransformState Transform;
    WorkspaceRecordName PendingSelection = {};
};

} // namespace Slate
