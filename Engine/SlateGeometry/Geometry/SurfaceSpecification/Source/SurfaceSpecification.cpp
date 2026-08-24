//============================================================================================================================================
//                                                    SURFACESPECIFICATION.CPP
//============================================================================================================================================

#include "SlateGeometry/Geometry/SurfaceSpecification/Api/SurfaceSpecification.h"

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

    bool PatchDeclared(const PatchSurface& Declared, bool Rational)
    {
        if (Declared.ControlRows.size() < 2u)
            return false;
        if (Declared.DegreeAlong < 1u || Declared.DegreeAcross < 1u)
            return false;

        const std::size_t ColumnCount = Declared.ControlRows.front().ControlPoints.size();
        if (ColumnCount < 2u)
            return false;

        for (const PatchControlRow& Row : Declared.ControlRows)
        {
            if (Row.ControlPoints.size() != ColumnCount)
                return false;
            if (Rational && Row.Weights.size() != ColumnCount)
                return false;
            if (!Rational && !Row.Weights.empty())
                return false;
        }

        return Declared.DegreeAlong < Declared.ControlRows.size()
            && Declared.DegreeAcross < ColumnCount;
    }
}

SurfaceSpecification SurfaceSpecification::DeclarePlane(const PlaneSurface& Declared,
                                                        const SurfaceParameterRange& Range)
{
    SurfaceSpecification Held;
    Held.HeldSubject = SurfaceKind::Plane;
    Held.HeldRange = Range;
    Held.Plane = Declared;
    return Held;
}

SurfaceSpecification SurfaceSpecification::DeclareCylinder(const CylinderSurface& Declared,
                                                           const SurfaceParameterRange& Range)
{
    SurfaceSpecification Held;
    Held.HeldSubject = SurfaceKind::Cylinder;
    Held.HeldRange = Range;
    Held.Cylinder = Declared;
    return Held;
}

SurfaceSpecification SurfaceSpecification::DeclareCone(const ConeSurface& Declared,
                                                       const SurfaceParameterRange& Range)
{
    SurfaceSpecification Held;
    Held.HeldSubject = SurfaceKind::Cone;
    Held.HeldRange = Range;
    Held.Cone = Declared;
    return Held;
}

SurfaceSpecification SurfaceSpecification::DeclareSphere(const SphereSurface& Declared,
                                                         const SurfaceParameterRange& Range)
{
    SurfaceSpecification Held;
    Held.HeldSubject = SurfaceKind::Sphere;
    Held.HeldRange = Range;
    Held.Sphere = Declared;
    return Held;
}

SurfaceSpecification SurfaceSpecification::DeclareTorus(const TorusSurface& Declared,
                                                        const SurfaceParameterRange& Range)
{
    SurfaceSpecification Held;
    Held.HeldSubject = SurfaceKind::Torus;
    Held.HeldRange = Range;
    Held.Torus = Declared;
    return Held;
}

SurfaceSpecification SurfaceSpecification::DeclareLinearExtrusion(const LinearExtrusionSurface& Declared,
                                                                  const SurfaceParameterRange& Range)
{
    SurfaceSpecification Held;
    Held.HeldSubject = SurfaceKind::LinearExtrusion;
    Held.HeldRange = Range;
    Held.Extrusion = Declared;
    return Held;
}

SurfaceSpecification SurfaceSpecification::DeclareBezierPatch(const PatchSurface& Declared,
                                                              const SurfaceParameterRange& Range)
{
    SurfaceSpecification Held;
    Held.HeldSubject = SurfaceKind::BezierPatch;
    Held.HeldRange = Range;
    Held.Patch = Declared;
    return Held;
}

SurfaceSpecification SurfaceSpecification::DeclareRationalPatch(const PatchSurface& Declared,
                                                                const SurfaceParameterRange& Range)
{
    SurfaceSpecification Held;
    Held.HeldSubject = SurfaceKind::RationalPatch;
    Held.HeldRange = Range;
    Held.Patch = Declared;
    return Held;
}

bool SurfaceSpecification::Declared() const
{
    if (!HeldRange.Declared())
        return false;

    switch (HeldSubject)
    {
        case SurfaceKind::Plane:
            return LengthSquared(Plane.Normal) > 0.0 && LengthSquared(Plane.AlongDirection) > 0.0;

        case SurfaceKind::Cylinder:
            return Cylinder.Radius > 0.0
                && LengthSquared(Cylinder.Axis) > 0.0
                && LengthSquared(Cylinder.RadialDirection) > 0.0;

        case SurfaceKind::Cone:
            return LengthSquared(Cone.Axis) > 0.0 && Cone.HalfAngleRadians > 0.0;

        case SurfaceKind::Sphere:
            return Sphere.Radius > 0.0;

        case SurfaceKind::Torus:
            return LengthSquared(Torus.Axis) > 0.0
                && Torus.MajorRadius > 0.0
                && Torus.MinorRadius > 0.0;

        case SurfaceKind::LinearExtrusion:
            return Extrusion.SectionCurve.Declared() && LengthSquared(Extrusion.Direction) > 0.0;

        case SurfaceKind::BezierPatch:
            return PatchDeclared(Patch, false);

        case SurfaceKind::RationalPatch:
            return PatchDeclared(Patch, true);

        case SurfaceKind::SubjectCount:
            return false;
    }

    return false;
}

} // namespace Slate
