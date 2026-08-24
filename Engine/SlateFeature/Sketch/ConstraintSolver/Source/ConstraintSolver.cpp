//============================================================================================================================================
//                                                      CONSTRAINTSOLVER.CPP
//============================================================================================================================================

#include "SlateFeature/Sketch/ConstraintSolver/Api/ConstraintSolver.h"

#include "SlateFeature/Sketch/SketchAnalysis/Api/SketchAnalysis.h"
#include "SlateFeature/Sketch/SketchEditing/Api/SketchEditing.h"
#include "SlateFeature/Sketch/SketchSelection/Api/SketchSelection.h"
#include "SlateFeature/Sketch/SketchPolyline/Api/SketchPolyline.h"

#include <algorithm>
#include <cmath>

namespace Slate
{

namespace
{
    double LengthSquared(const SpatialDirection& Direction)
    {
        return Direction.Left * Direction.Left + Direction.Up * Direction.Up + Direction.Forward * Direction.Forward;
    }

    SpatialDirection Difference(const SpatialPoint& LeftPoint, const SpatialPoint& RightPoint)
    {
        return { RightPoint.Left - LeftPoint.Left,
                 RightPoint.Up - LeftPoint.Up,
                 RightPoint.Forward - LeftPoint.Forward };
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

    double Dot(const SpatialDirection& LeftDirection,
               const SpatialDirection& RightDirection)
    {
        return LeftDirection.Left * RightDirection.Left + LeftDirection.Up * RightDirection.Up + LeftDirection.Forward * RightDirection.Forward;
    }

    SpatialDirection Scaled(const SpatialDirection& Direction,
                            double Amount)
    {
        return { Direction.Left * Amount, Direction.Up * Amount, Direction.Forward * Amount };
    }

    SpatialPoint Added(const SpatialPoint& Position,
                       const SpatialDirection& Offset)
    {
        return { Position.Left + Offset.Left, Position.Up + Offset.Up, Position.Forward + Offset.Forward };
    }

    PlanarPoint Flatten(const SketchPlane& Plane,
                        const SpatialPoint& Position)
    {
        const SpatialDirection AlongDirection = Normalize(Plane.AlongDirection);
        const SpatialDirection AcrossDirection = Normalize(Cross(Plane.Normal, AlongDirection));
        const SpatialDirection Offset = { Position.Left - Plane.Origin.Left,
                                          Position.Up - Plane.Origin.Up,
                                          Position.Forward - Plane.Origin.Forward };
        return { Dot(Offset, AlongDirection), Dot(Offset, AcrossDirection) };
    }

    SpatialPoint Lift(const SketchPlane& Plane,
                      const PlanarPoint& Position)
    {
        const SpatialDirection AlongDirection = Normalize(Plane.AlongDirection);
        const SpatialDirection AcrossDirection = Normalize(Cross(Plane.Normal, AlongDirection));
        return { Plane.Origin.Left + AlongDirection.Left * Position.Along + AcrossDirection.Left * Position.Across,
                 Plane.Origin.Up + AlongDirection.Up * Position.Along + AcrossDirection.Up * Position.Across,
                 Plane.Origin.Forward + AlongDirection.Forward * Position.Along + AcrossDirection.Forward * Position.Across };
    }

    bool ResolveConstraintPoint(const SketchStructure& Declared,
                                const ReferenceSpecification& Reference,
                                SketchPointPlacement& Resolved)
    {
        if (Reference.Subject != ReferenceSubject::SketchPoint)
            return false;
        std::uint32_t CurveIndex = Reference.SketchPoint.IssuedIndex >> 8u;
        if (CurveIndex == 0u || CurveIndex > Declared.Curves().size())
            return false;
        std::vector<SketchPointPlacement> Points;
        if (!ResolveSketchPoints(Declared, { CurveIndex }, Points))
            return false;
        for (const SketchPointPlacement& Point : Points)
            if (Point.Name.IssuedIndex == Reference.SketchPoint.IssuedIndex)
            {
                Resolved = Point;
                return true;
            }
        return false;
    }

    bool ResolveConstraintCurve(const SketchStructure& Declared,
                                const ReferenceSpecification& Reference,
                                SketchCurveName& Resolved)
    {
        if (Reference.Subject != ReferenceSubject::SketchCurve)
            return false;
        if (!Reference.SketchCurve.Assigned() || Reference.SketchCurve.IssuedIndex > Declared.Curves().size())
            return false;
        Resolved = Reference.SketchCurve;
        return true;
    }

    bool ResolveLine(const SketchStructure& Declared,
                     SketchCurveName Subject,
                     SpatialPoint& StartPoint,
                     SpatialPoint& EndPoint)
    {
        if (!Subject.Assigned() || Subject.IssuedIndex > Declared.Curves().size())
            return false;
        const CurveSpecification& Geometry = Declared.Curves()[Subject.IssuedIndex - 1u].Geometry;
        if (Geometry.Subject() != CurveSubject::Line || !Geometry.Declared())
            return false;
        StartPoint = Geometry.HeldLine().Origin;
        EndPoint = Geometry.HeldLine().Terminus;
        return true;
    }

    void AppendCurvePolylineLocal(const CurveSpecification& Geometry,
                                  std::vector<SpatialPoint>& Polyline)
    {
        Slate::AppendCurvePolyline(Geometry, Polyline, 96u);
    }

    bool ResolveCurveTangentAtPoint(const SketchStructure& Declared,
                                    SketchCurveName Subject,
                                    const SpatialPoint& Contact,
                                    SpatialDirection& Tangent)
    {
        if (!Subject.Assigned() || Subject.IssuedIndex > Declared.Curves().size())
            return false;
        const CurveSpecification& Geometry = Declared.Curves()[Subject.IssuedIndex - 1u].Geometry;
        if (!Geometry.Declared())
            return false;

        std::vector<SpatialPoint> Polyline;
        AppendCurvePolylineLocal(Geometry, Polyline);
        if (Polyline.size() < 2u)
            return false;

        double BestDistance = 1.0e300;
        SpatialDirection BestDirection = {};
        for (std::size_t Index = 0u; Index + 1u < Polyline.size(); ++Index)
        {
            const SpatialPoint& StartPoint = Polyline[Index];
            const SpatialPoint& EndPoint = Polyline[Index + 1u];
            const SpatialDirection Span = Difference(StartPoint, EndPoint);
            const SpatialDirection Offset = Difference(StartPoint, Contact);
            const double SpanLengthSquared = LengthSquared(Span);
            const double Parameter = SpanLengthSquared > 1.0e-18
                ? std::clamp(Dot(Offset, Span) / SpanLengthSquared, 0.0, 1.0)
                : 0.0;
            const SpatialPoint Closest = Added(StartPoint, Scaled(Span, Parameter));
            const double Distance = std::sqrt(LengthSquared(Difference(Closest, Contact)));
            if (Distance < BestDistance)
            {
                BestDistance = Distance;
                BestDirection = Normalize(Span);
            }
        }

        if (BestDistance > 1.0e-3)
            return false;
        Tangent = BestDirection;
        return true;
    }
}

ConstraintDisposition EvaluateConstraints(const SketchStructure& Declared)
{
    if (Declared.Constraints().empty())
        return ConstraintDisposition::NotRequested;
    if (!Declared.Declared())
        return ConstraintDisposition::InvalidSketch;

    const Outcome<SketchAnalysis> Analysed = AnalyseSketch(Declared);
    if (!Analysed)
        return ConstraintDisposition::InvalidSketch;
    for (const ConstraintFinding& Finding : Analysed.Resolve().Findings)
    {
        if (Finding.Conflicting)
            return ConstraintDisposition::ConflictingConstraint;
        if (Finding.Repeated)
            return ConstraintDisposition::RepeatedConstraint;
    }

    for (const ConstraintSpecification& Constraint : Declared.Constraints())
    {
        if (!Constraint.Declared())
            return ConstraintDisposition::InvalidSketch;
        switch (Constraint.Subject)
        {
            case ConstraintSubject::Coincident:
                if (Constraint.Primary.Subject != ReferenceSubject::SketchPoint || Constraint.Secondary.Subject != ReferenceSubject::SketchPoint)
                    return ConstraintDisposition::UnsupportedConstraint;
                break;
            case ConstraintSubject::Horizontal:
            case ConstraintSubject::Vertical:
            case ConstraintSubject::Fixed:
                if (Constraint.Primary.Subject != ReferenceSubject::SketchCurve && Constraint.Primary.Subject != ReferenceSubject::SketchPoint)
                    return ConstraintDisposition::UnsupportedConstraint;
                break;
            case ConstraintSubject::Parallel:
            case ConstraintSubject::Perpendicular:
            case ConstraintSubject::Equal:
                if (Constraint.Primary.Subject != ReferenceSubject::SketchCurve || Constraint.Secondary.Subject != ReferenceSubject::SketchCurve)
                    return ConstraintDisposition::UnsupportedConstraint;
                break;
            case ConstraintSubject::Tangent:
                if (Constraint.Primary.Subject != ReferenceSubject::SketchCurve || Constraint.Secondary.Subject != ReferenceSubject::SketchCurve)
                    return ConstraintDisposition::UnsupportedConstraint;
                break;
            case ConstraintSubject::SubjectCount:
                return ConstraintDisposition::UnsupportedConstraint;
        }
    }

    return ConstraintDisposition::Produced;
}

Outcome<bool> ResolveConstraintConflict(const SketchStructure& Declared,
                                        ConstraintName Subject)
{
    if (!Subject.Assigned() || Subject.IssuedIndex > Declared.Constraints().size())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such constraint is declared" });
    const Outcome<SketchAnalysis> Analysed = AnalyseSketch(Declared);
    if (!Analysed)
        return Outcome<bool>::Refuse(Analysed.Error);
    const ConstraintFinding& Finding = Analysed.Resolve().Findings[Subject.IssuedIndex - 1u];
    return Outcome<bool>::Result(Finding.Conflicting || Finding.Repeated);
}

Outcome<bool> ApplyConstraints(SketchStructure& Declared)
{
    if (EvaluateConstraints(Declared) == ConstraintDisposition::NotRequested)
        return Outcome<bool>::Result(true);
    if (EvaluateConstraints(Declared) != ConstraintDisposition::Produced)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the sketch constraints are unsupported" });

    for (std::uint32_t ConstraintIndex = 1u; ConstraintIndex <= Declared.Constraints().size(); ++ConstraintIndex)
    {
        const Outcome<bool> Applied = ApplyConstraint(Declared, { ConstraintIndex });
        if (!Applied)
            return Applied;
    }
    return Outcome<bool>::Result(true);
}

Outcome<bool> ApplyConstraint(SketchStructure& Declared,
                              ConstraintName Subject)
{
    if (!Subject.Assigned() || Subject.IssuedIndex > Declared.Constraints().size())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such constraint is declared" });
    if (!Declared.Declared())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the sketch is not declared" });

    const ConstraintSpecification& Constraint = Declared.Constraints()[Subject.IssuedIndex - 1u];
    const SketchPlane& Plane = Declared.HeldPlane();

    switch (Constraint.Subject)
    {
        case ConstraintSubject::Coincident:
        {
            SketchPointPlacement First = {};
            SketchPointPlacement Second = {};
            if (!ResolveConstraintPoint(Declared, Constraint.Primary, First)
             || !ResolveConstraintPoint(Declared, Constraint.Secondary, Second))
            {
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the coincident points are not resolved" });
            }
            return EnforceSketchPoint(Declared, Second.Name, First.Position);
        }

        case ConstraintSubject::Horizontal:
        case ConstraintSubject::Vertical:
        {
            if (Constraint.Primary.Subject == ReferenceSubject::SketchPoint)
                return Outcome<bool>::Result(true);

            SketchCurveName Curve = {};
            if (!ResolveConstraintCurve(Declared, Constraint.Primary, Curve))
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the constrained curve is not resolved" });

            SpatialPoint StartPoint = {};
            SpatialPoint EndPoint = {};
            if (!ResolveLine(Declared, Curve, StartPoint, EndPoint))
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "horizontal and vertical constraints currently accept line curves only" });

            const PlanarPoint FlatStart = Flatten(Plane, StartPoint);
            PlanarPoint FlatEnd = Flatten(Plane, EndPoint);
            if (Constraint.Subject == ConstraintSubject::Horizontal)
                FlatEnd.Across = FlatStart.Across;
            else
                FlatEnd.Along = FlatStart.Along;

            return EnforceSketchPoint(Declared, { (Curve.IssuedIndex << 8u) | 2u }, Lift(Plane, FlatEnd));
        }

        case ConstraintSubject::Parallel:
        case ConstraintSubject::Perpendicular:
        case ConstraintSubject::Equal:
        {
            SketchCurveName Primary = {};
            SketchCurveName Secondary = {};
            if (!ResolveConstraintCurve(Declared, Constraint.Primary, Primary)
             || !ResolveConstraintCurve(Declared, Constraint.Secondary, Secondary))
            {
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the constrained curves are not resolved" });
            }

            SpatialPoint PrimaryStart = {}, PrimaryEnd = {};
            SpatialPoint SecondaryStart = {}, SecondaryEnd = {};
            if (!ResolveLine(Declared, Primary, PrimaryStart, PrimaryEnd)
             || !ResolveLine(Declared, Secondary, SecondaryStart, SecondaryEnd))
            {
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "parallel, perpendicular and equal currently accept line curves only" });
            }

            SpatialDirection PrimaryDirection = Normalize(Difference(PrimaryStart, PrimaryEnd));
            SpatialDirection SecondaryDirection = Normalize(Difference(SecondaryStart, SecondaryEnd));
            double Length = std::sqrt(LengthSquared(Difference(SecondaryStart, SecondaryEnd)));

            if (Constraint.Subject == ConstraintSubject::Parallel)
            {
                SecondaryEnd = Added(SecondaryStart, Scaled(PrimaryDirection, Length));
            }
            else if (Constraint.Subject == ConstraintSubject::Perpendicular)
            {
                const SpatialDirection PerpendicularDirection = Normalize(Cross(Plane.Normal, PrimaryDirection));
                SecondaryEnd = Added(SecondaryStart, Scaled(PerpendicularDirection, Length));
            }
            else
            {
                const double PrimaryLength = std::sqrt(LengthSquared(Difference(PrimaryStart, PrimaryEnd)));
                SecondaryEnd = Added(SecondaryStart, Scaled(SecondaryDirection, PrimaryLength));
            }

            return EnforceSketchPoint(Declared, { (Secondary.IssuedIndex << 8u) | 2u }, SecondaryEnd);
        }

        case ConstraintSubject::Fixed:
            return Outcome<bool>::Result(true);

        case ConstraintSubject::Tangent:
        {
            SketchCurveName Primary = {};
            SketchCurveName Secondary = {};
            if (!ResolveConstraintCurve(Declared, Constraint.Primary, Primary)
             || !ResolveConstraintCurve(Declared, Constraint.Secondary, Secondary))
            {
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the tangent curves are not resolved" });
            }

            SpatialPoint LineStart = {}, LineEnd = {};
            if (!ResolveLine(Declared, Primary, LineStart, LineEnd))
            {
                std::swap(Primary, Secondary);
                if (!ResolveLine(Declared, Primary, LineStart, LineEnd))
                    return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "tangent currently requires one line curve" });
            }

            SpatialDirection StartTangent = {};
            SpatialDirection EndTangent = {};
            const bool StartContact = ResolveCurveTangentAtPoint(Declared, Secondary, LineStart, StartTangent);
            const bool EndContact = ResolveCurveTangentAtPoint(Declared, Secondary, LineEnd, EndTangent);
            if (!StartContact && !EndContact)
            {
                return Outcome<bool>::Refuse(
                    { RefusalReason::ContentUnsupported, "tangent currently requires one line endpoint to lie on the target curve" });
            }

            const SpatialPoint Contact = StartContact ? LineStart : LineEnd;
            const SpatialPoint Anchor = StartContact ? LineEnd : LineStart;
            const SpatialDirection TangentDirection = StartContact ? StartTangent : EndTangent;
            const double Length = std::sqrt(LengthSquared(Difference(Anchor, Contact)));
            const SpatialPoint Adjusted = Added(Contact, Scaled(TangentDirection, Length));
            return EnforceSketchPoint(Declared,
                                      { (Primary.IssuedIndex << 8u) | (StartContact ? 2u : 1u) },
                                      Adjusted);
        }

        case ConstraintSubject::SubjectCount:
            break;
    }

    return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such constraint subject is supported" });
}

} // namespace Slate
