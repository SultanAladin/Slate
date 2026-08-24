//============================================================================================================================================
//                                                         SKETCHSNAP.CPP
//============================================================================================================================================

#include "SlateFeature/Sketch/SketchSnap/Api/SketchSnap.h"
#include "SlateFeature/Sketch/SketchPolyline/Api/SketchPolyline.h"

#include <algorithm>
#include <cmath>

namespace Slate
{

namespace
{
    double LengthSquared(const SpatialDirection& Direction)
    {
        return Direction.Left * Direction.Left
             + Direction.Up * Direction.Up
             + Direction.Forward * Direction.Forward;
    }

    SpatialDirection Difference(const SpatialPoint& LeftPoint,
                                const SpatialPoint& RightPoint)
    {
        return { RightPoint.Left - LeftPoint.Left,
                 RightPoint.Up - LeftPoint.Up,
                 RightPoint.Forward - LeftPoint.Forward };
    }

    SpatialDirection Scaled(const SpatialDirection& Direction,
                            double Amount)
    {
        return { Direction.Left * Amount, Direction.Up * Amount, Direction.Forward * Amount };
    }

    SpatialPoint Added(const SpatialPoint& Position,
                       const SpatialDirection& Offset)
    {
        return { Position.Left + Offset.Left,
                 Position.Up + Offset.Up,
                 Position.Forward + Offset.Forward };
    }

    SpatialDirection Normalize(const SpatialDirection& Direction)
    {
        const double Length = std::sqrt(LengthSquared(Direction));
        return { Direction.Left / Length, Direction.Up / Length, Direction.Forward / Length };
    }

    SpatialDirection Cross(const SpatialDirection& LeftDirection,
                           const SpatialDirection& RightDirection)
    {
        return {
            LeftDirection.Up * RightDirection.Forward - LeftDirection.Forward * RightDirection.Up,
            LeftDirection.Forward * RightDirection.Left - LeftDirection.Left * RightDirection.Forward,
            LeftDirection.Left * RightDirection.Up - LeftDirection.Up * RightDirection.Left
        };
    }

    SpatialDirection RotateAroundAxis(const SpatialDirection& Subject,
                                      const SpatialDirection& Axis,
                                      double Radians)
    {
        const SpatialDirection UnitAxis = Normalize(Axis);
        const double Cosine = std::cos(Radians);
        const double Sine = std::sin(Radians);
        const double Projection = UnitAxis.Left * Subject.Left + UnitAxis.Up * Subject.Up + UnitAxis.Forward * Subject.Forward;
        const SpatialDirection Parallel = Scaled(UnitAxis, Projection);
        const SpatialDirection Perpendicular = { Subject.Left - Parallel.Left,
                                                 Subject.Up - Parallel.Up,
                                                 Subject.Forward - Parallel.Forward };
        const SpatialDirection Crossed = Cross(UnitAxis, Subject);
        return { Perpendicular.Left * Cosine + Crossed.Left * Sine + Parallel.Left,
                 Perpendicular.Up * Cosine + Crossed.Up * Sine + Parallel.Up,
                 Perpendicular.Forward * Cosine + Crossed.Forward * Sine + Parallel.Forward };
    }

    double DistanceSquared(const SpatialPoint& LeftPoint,
                           const SpatialPoint& RightPoint)
    {
        return LengthSquared(Difference(LeftPoint, RightPoint));
    }

    const DeclaredSketchCurve* ResolveCurve(const SketchStructure& Declared,
                                            SketchCurveName SourceCurve)
    {
        if (!SourceCurve.Assigned() || SourceCurve.IssuedIndex > Declared.Curves().size())
            return nullptr;
        return &Declared.Curves()[SourceCurve.IssuedIndex - 1u];
    }

    void AppendCurvePolylineLocal(const CurveSpecification& Geometry,
                                  std::vector<SpatialPoint>& Polyline)
    {
        Slate::AppendCurvePolyline(Geometry, Polyline, 48u);
    }

    void ConsiderCandidate(const SpatialPoint& Probe,
                           const SpatialPoint& CandidatePosition,
                           SketchCurveName SourceCurve,
                           SketchSnapSubject Subject,
                           SketchPointName SketchPoint,
                           SketchControlName SketchControl,
                           double MaximumDistance,
                           SketchSnapPlacement& Best)
    {
        const double CandidateDistance = std::sqrt(DistanceSquared(Probe, CandidatePosition));
        if (CandidateDistance > MaximumDistance)
            return;
        if (!Best.Resolved() || CandidateDistance < Best.Distance)
            Best = { Subject, SourceCurve, SketchPoint, SketchControl, CandidatePosition, CandidateDistance };
    }
}

SketchSnapPlacement ResolveNearestSnap(const SketchStructure& Declared,
                                       const SpatialPoint& Probe,
                                       double MaximumDistance,
                                       const SketchSnapMask& Accepted)
{
    SketchSnapPlacement Best = {};
    Best.Distance = MaximumDistance;

    std::vector<SketchPointPlacement> Points;
    std::vector<SketchControlPlacement> Controls;
    std::vector<SpatialPoint> Polyline;

    for (std::uint32_t CurveIndex = 1u; CurveIndex <= Declared.Curves().size(); ++CurveIndex)
    {
        const SketchCurveName Curve = { CurveIndex };

        if (Accepted.EndpointAccepted && ResolveSketchPoints(Declared, Curve, Points))
        {
            for (const SketchPointPlacement& Point : Points)
                ConsiderCandidate(Probe, Point.Position, Curve, SketchSnapSubject::Endpoint, Point.Name, {}, MaximumDistance, Best);
        }

        if ((Accepted.CentreAccepted || Accepted.ControlAccepted) && ResolveSketchControls(Declared, Curve, Controls))
        {
            for (const SketchControlPlacement& Control : Controls)
            {
                if (Accepted.CentreAccepted && Control.Subject == SketchControlSubject::Centre)
                    ConsiderCandidate(Probe, Control.Position, Curve, SketchSnapSubject::Centre, {}, Control.Name, MaximumDistance, Best);
                else if (Accepted.ControlAccepted)
                    ConsiderCandidate(Probe, Control.Position, Curve, SketchSnapSubject::Control, {}, Control.Name, MaximumDistance, Best);
            }
        }

        const DeclaredSketchCurve* Held = ResolveCurve(Declared, Curve);
        if (Held == nullptr || !Held->Geometry.Declared())
            continue;

        AppendCurvePolylineLocal(Held->Geometry, Polyline);
        if (Polyline.size() < 2u)
            continue;

        if (Accepted.MidpointAccepted)
        {
            for (std::size_t PointIndex = 0u; PointIndex + 1u < Polyline.size(); ++PointIndex)
            {
                const SpatialPoint Midpoint = { (Polyline[PointIndex].Left + Polyline[PointIndex + 1u].Left) * 0.5,
                                                (Polyline[PointIndex].Up + Polyline[PointIndex + 1u].Up) * 0.5,
                                                (Polyline[PointIndex].Forward + Polyline[PointIndex + 1u].Forward) * 0.5 };
                ConsiderCandidate(Probe, Midpoint, Curve, SketchSnapSubject::Midpoint, {}, {}, MaximumDistance, Best);
            }
        }

        if (Accepted.AlongCurveAccepted)
        {
            for (std::size_t PointIndex = 0u; PointIndex + 1u < Polyline.size(); ++PointIndex)
            {
                const SpatialDirection Span = Difference(Polyline[PointIndex], Polyline[PointIndex + 1u]);
                const SpatialDirection Offset = Difference(Polyline[PointIndex], Probe);
                const double SpanLengthSquared = LengthSquared(Span);
                const double Parameter = SpanLengthSquared > 1.0e-18
                    ? std::clamp((Offset.Left * Span.Left + Offset.Up * Span.Up + Offset.Forward * Span.Forward) / SpanLengthSquared,
                                 0.0, 1.0)
                    : 0.0;
                const SpatialPoint Closest = Added(Polyline[PointIndex], Scaled(Span, Parameter));
                ConsiderCandidate(Probe, Closest, Curve, SketchSnapSubject::AlongCurve, {}, {}, MaximumDistance, Best);
            }
        }
    }

    return Best;
}

} // namespace Slate
