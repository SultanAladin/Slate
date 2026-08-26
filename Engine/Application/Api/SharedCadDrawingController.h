//============================================================================================================================================
//                                                SHAREDCADDRAWINGCONTROLLER.H
//============================================================================================================================================
// 🧩 Shared CAD drawing controller vocabulary used by EditorHost and ParametricSketchHost.
//    ParametricSketchHost owns the exact sketch document today; EditorHost consumes the same controller mapping so the
//    combined editor does not grow a divergent CAD tool dispatch table while the remaining exact-record state is being
//    lifted behind this seam.

#pragma once

#include "SlateUI/Interface/ParametricTools/Api/ParametricToolsSpecification.h"
#include "Application/Api/SharedCadWorkspaceRuntime.h"
#include "SlateFeature/Feature/WorkspaceNameIndex/Api/WorkspaceNameIndex.h"

#include <cstdint>
#include <string>

namespace Slate
{

enum class SharedCadDraftSubject : std::uint32_t
{
    None = 0u,
    Line = 1u,
    Rectangle = 2u,
    Circle = 3u,
    Arc = 4u,
    Polyline = 5u,
    LinearDimension = 6u,
    Point = 7u,
    Ellipse = 8u,
    Bezier = 9u,
    EllipticalArc = 10u,
    BasisSpline = 11u,
    CenterRectangle = 12u,
    ThreePointRectangle = 13u,
    DiameterCircle = 14u,
    ThreePointCircle = 15u,
    CenterStartEndArc = 16u,
    TangentArc = 17u,
    Polygon = 18u,
    Slot = 19u,
    Hermite = 20u,
    RationalSpline = 21u
};

inline SharedCadDraftSubject ResolveSharedCadDraftSubject(ParametricToolSubject Subject)
{
    switch (Subject)
    {
        case ParametricToolSubject::Line:                 return SharedCadDraftSubject::Line;
        case ParametricToolSubject::Polyline:             return SharedCadDraftSubject::Polyline;
        case ParametricToolSubject::Rectangle:            return SharedCadDraftSubject::Rectangle;
        case ParametricToolSubject::Circle:               return SharedCadDraftSubject::Circle;
        case ParametricToolSubject::Arc:                  return SharedCadDraftSubject::Arc;
        case ParametricToolSubject::LinearDimension:      return SharedCadDraftSubject::LinearDimension;
        case ParametricToolSubject::Point:                return SharedCadDraftSubject::Point;
        case ParametricToolSubject::Ellipse:              return SharedCadDraftSubject::Ellipse;
        case ParametricToolSubject::EllipticalArc:        return SharedCadDraftSubject::EllipticalArc;
        case ParametricToolSubject::BezierCurve:
        case ParametricToolSubject::Interpolate:
        case ParametricToolSubject::Approximate:          return SharedCadDraftSubject::Bezier;
        case ParametricToolSubject::HermiteCurve:         return SharedCadDraftSubject::Hermite;
        case ParametricToolSubject::BasisSpline:          return SharedCadDraftSubject::BasisSpline;
        case ParametricToolSubject::RationalSpline:       return SharedCadDraftSubject::RationalSpline;
        case ParametricToolSubject::ConstructionLine:     return SharedCadDraftSubject::Line;
        case ParametricToolSubject::CenterRectangle:      return SharedCadDraftSubject::CenterRectangle;
        case ParametricToolSubject::ThreePointRectangle:  return SharedCadDraftSubject::ThreePointRectangle;
        case ParametricToolSubject::DiameterCircle:       return SharedCadDraftSubject::DiameterCircle;
        case ParametricToolSubject::ThreePointCircle:     return SharedCadDraftSubject::ThreePointCircle;
        case ParametricToolSubject::CenterStartEndArc:    return SharedCadDraftSubject::CenterStartEndArc;
        case ParametricToolSubject::TangentArc:           return SharedCadDraftSubject::TangentArc;
        case ParametricToolSubject::Polygon:              return SharedCadDraftSubject::Polygon;
        case ParametricToolSubject::Slot:                 return SharedCadDraftSubject::Slot;
        default:                                          return SharedCadDraftSubject::None;
    }
}

inline bool SharedCadDraftProducesClosedProfile(SharedCadDraftSubject Subject)
{
    switch (Subject)
    {
        case SharedCadDraftSubject::Rectangle:
        case SharedCadDraftSubject::CenterRectangle:
        case SharedCadDraftSubject::ThreePointRectangle:
        case SharedCadDraftSubject::Circle:
        case SharedCadDraftSubject::DiameterCircle:
        case SharedCadDraftSubject::ThreePointCircle:
        case SharedCadDraftSubject::Ellipse:
        case SharedCadDraftSubject::Polygon:
        case SharedCadDraftSubject::Slot:
            return true;
        default:
            return false;
    }
}

inline std::uint32_t SharedCadDraftRequiredAnchors(SharedCadDraftSubject Subject)
{
    switch (Subject)
    {
        case SharedCadDraftSubject::Line:
        case SharedCadDraftSubject::Rectangle:
        case SharedCadDraftSubject::CenterRectangle:
        case SharedCadDraftSubject::DiameterCircle:
        case SharedCadDraftSubject::Polygon:
            return 2u;
        case SharedCadDraftSubject::Circle:
        case SharedCadDraftSubject::Ellipse:
            return 1u;
        case SharedCadDraftSubject::Arc:
        case SharedCadDraftSubject::EllipticalArc:
        case SharedCadDraftSubject::ThreePointRectangle:
        case SharedCadDraftSubject::ThreePointCircle:
        case SharedCadDraftSubject::CenterStartEndArc:
        case SharedCadDraftSubject::TangentArc:
        case SharedCadDraftSubject::Slot:
        case SharedCadDraftSubject::BasisSpline:
        case SharedCadDraftSubject::RationalSpline:
            return 3u;
        case SharedCadDraftSubject::Hermite:
            return 4u;
        case SharedCadDraftSubject::Bezier:
        case SharedCadDraftSubject::Polyline:
        case SharedCadDraftSubject::LinearDimension:
            return 2u;
        case SharedCadDraftSubject::Point:
            return 1u;
        case SharedCadDraftSubject::None:
            return 0u;
    }
    return 0u;
}

inline bool SharedCadDraftIsMultiClickCurve(SharedCadDraftSubject Subject)
{
    return Subject == SharedCadDraftSubject::Polyline ||
           Subject == SharedCadDraftSubject::Bezier ||
           Subject == SharedCadDraftSubject::BasisSpline ||
           Subject == SharedCadDraftSubject::Hermite ||
           Subject == SharedCadDraftSubject::RationalSpline;
}

struct SharedCadAuthoringRequest
{
    SharedCadDraftSubject Subject = SharedCadDraftSubject::None;
    SpatialPoint Hover = {};
    bool HoverStanding = false;
    bool ContactPressed = false;
    bool CommitRequested = false;
    bool CancelPressed = false;
    bool Construction = false;
};

/// Advances the shared draft state and commits the common two-point line tool. More specialised
/// tools remain in the host adapter until their existing feature helpers are moved here as well.
inline bool SharedCadAuthoringDispatch(SharedCadWorkspaceRuntime& Runtime,
                                       WorkspaceNameIndex& Naming,
                                       const SharedCadAuthoringRequest& Request)
{
    if (Request.CancelPressed || Request.Subject == SharedCadDraftSubject::None)
    {
        Runtime.Draft = ParametricDraftState{};
        return Request.CancelPressed;
    }

    if (Runtime.Draft.Subject != static_cast<ParametricDraftSubject>(Request.Subject))
    {
        Runtime.Draft = ParametricDraftState{};
        Runtime.Draft.Subject = static_cast<ParametricDraftSubject>(Request.Subject);
    }
    Runtime.Draft.Construction = Request.Construction;
    Runtime.Draft.HoverStanding = Request.HoverStanding;
    Runtime.Draft.Hover = Request.Hover;
    if (!Request.ContactPressed || !Request.HoverStanding)
        return false;

    Runtime.Draft.Anchors.push_back(Request.Hover);
    const ParametricDraftSubject Subject = Runtime.Draft.Subject;
    const bool TwoPoint = Runtime.Draft.Anchors.size() >= 2u;
    const bool Closed = Subject == ParametricDraftSubject::Rectangle ||
                        Subject == ParametricDraftSubject::CenterRectangle;
    const bool Circle = Subject == ParametricDraftSubject::Circle ||
                        Subject == ParametricDraftSubject::DiameterCircle;
    const bool Polyline = Subject == ParametricDraftSubject::Polyline;
    if (!TwoPoint || (Polyline && !Request.CommitRequested))
        return true;

    if (!Runtime.Sketch.Declared())
        Runtime.Sketch.DeclarePlane({ { 0.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 1.0, 0.0, 0.0 } });

    std::vector<WorkspaceRecordName> Written;
    const auto DeclareCurve = [&](SketchCurveName Curve)
    {
        WorkspaceRecord Record = {};
        Record.Subject = WorkspaceRecordSubject::OpenCurve;
        Record.Naming = Naming.Issue(WorkspaceRecordSubject::OpenCurve);
        Record.SketchCurve = Curve;
        Record.ConstructionSemantic = Request.Construction;
        const WorkspaceRecordName Name = Runtime.Records.Declare(Record);
        if (Name.Assigned())
            Written.push_back(Name);
    };

    if (Polyline)
    {
        std::vector<SketchCurveName> Curves;
        const Outcome<bool> Declared = Runtime.Sketch.DeclarePolyline(Runtime.Draft.Anchors, Curves);
        if (!Declared.Resolved)
            return true;
        for (const SketchCurveName Curve : Curves)
            DeclareCurve(Curve);
    }
    else if (Closed)
    {
        const SpatialPoint& A = Runtime.Draft.Anchors[0];
        const SpatialPoint& B = Runtime.Draft.Anchors[1];
        const SpatialPoint C = { B.Left, A.Up, A.Forward };
        const SpatialPoint D = { A.Left, A.Up, B.Forward };
        DeclareCurve(Runtime.Sketch.DeclareLine(A, C));
        DeclareCurve(Runtime.Sketch.DeclareLine(C, B));
        DeclareCurve(Runtime.Sketch.DeclareLine(B, D));
        DeclareCurve(Runtime.Sketch.DeclareLine(D, A));
    }
    else if (Circle)
    {
        const SpatialPoint& Centre = Runtime.Draft.Anchors[0];
        const SpatialDirection Along = { Request.Hover.Left - Centre.Left,
                                         Request.Hover.Up - Centre.Up,
                                         Request.Hover.Forward - Centre.Forward };
        const double Radius = std::sqrt((Request.Hover.Left - Centre.Left) * (Request.Hover.Left - Centre.Left) +
                                        (Request.Hover.Forward - Centre.Forward) * (Request.Hover.Forward - Centre.Forward));
        if (Radius > 1.0e-6)
        {
            const CircularArcCurve Arc = { Centre, { 0.0, 1.0, 0.0 }, Along, {}, false, Radius, 2.0 * 3.14159265358979323846 };
            DeclareCurve(Runtime.Sketch.DeclareCurve(CurveSpecification::DeclareCircularArc(Arc, { 0.0, 1.0 })));
        }
    }
    else if (Subject == ParametricDraftSubject::Line)
    {
        DeclareCurve(Runtime.Sketch.DeclareLine(Runtime.Draft.Anchors[0], Runtime.Draft.Anchors[1]));
    }
    else
    {
        return true;
    }

    if (!Written.empty())
    {
        Runtime.PendingSelection = Written.front();
        Runtime.Revisions.Seal("Declared CAD geometry", "Create CAD geometry", Written,
                               Runtime.Revisions.DeclaredCount() + 1u);
    }
    Runtime.Draft = ParametricDraftState{};
    return true;
}

inline const char* SharedCadDraftSubjectName(SharedCadDraftSubject Subject)
{
    switch (Subject)
    {
        case SharedCadDraftSubject::Line: return "Line";
        case SharedCadDraftSubject::Rectangle: return "Rectangle";
        case SharedCadDraftSubject::Circle: return "Circle";
        case SharedCadDraftSubject::Arc: return "Arc";
        case SharedCadDraftSubject::Polyline: return "Polyline";
        case SharedCadDraftSubject::LinearDimension: return "Dimension";
        case SharedCadDraftSubject::Point: return "Point";
        case SharedCadDraftSubject::Ellipse: return "Ellipse";
        case SharedCadDraftSubject::Bezier: return "Bezier";
        case SharedCadDraftSubject::EllipticalArc: return "Elliptical Arc";
        case SharedCadDraftSubject::BasisSpline: return "Basis Spline";
        case SharedCadDraftSubject::CenterRectangle: return "Center Rectangle";
        case SharedCadDraftSubject::ThreePointRectangle: return "3-Point Rectangle";
        case SharedCadDraftSubject::DiameterCircle: return "Diameter Circle";
        case SharedCadDraftSubject::ThreePointCircle: return "3-Point Circle";
        case SharedCadDraftSubject::CenterStartEndArc: return "Center Arc";
        case SharedCadDraftSubject::TangentArc: return "Tangent Arc";
        case SharedCadDraftSubject::Polygon: return "Polygon";
        case SharedCadDraftSubject::Slot: return "Slot";
        case SharedCadDraftSubject::Hermite: return "Hermite";
        case SharedCadDraftSubject::RationalSpline: return "NURBS Curve";
        case SharedCadDraftSubject::None: return "";
    }
    return "";
}

} // namespace Slate
