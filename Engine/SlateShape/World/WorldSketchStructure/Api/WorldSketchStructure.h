//============================================================================================================================================
//                                                    WORLDSKETCHSTRUCTURE.H
//============================================================================================================================================
// 🧩 World-space authoring authority for the sketch replacement path — exact curves live in true 3D coordinates,
//    while workplanes survive only as placement metadata rather than as one global basis that reinterprets
//    every shape already drawn.

#pragma once

#include "SlateShape/Geometry/CurveSpecification/Api/CurveSpecification.h"

#include <cstdint>
#include <vector>

namespace Slate
{

struct WorldCurveName
{
    std::uint32_t IssuedIndex = 0u;
    bool Assigned() const { return IssuedIndex != 0u; }
};

struct WorldLoopName
{
    std::uint32_t IssuedIndex = 0u;
    bool Assigned() const { return IssuedIndex != 0u; }
};

struct WorldConstraintName
{
    std::uint32_t IssuedIndex = 0u;
    bool Assigned() const { return IssuedIndex != 0u; }
};

enum class WorldConstraintSubject : std::uint32_t
{
    Coincident = 0u,
    Horizontal = 1u,
    Vertical = 2u,
    Parallel = 3u,
    Perpendicular = 4u,
    Tangent = 5u,
    Equal = 6u,
    Fixed = 7u,
    SubjectCount = 8u
};

enum class WorldConstraintReferenceSubject : std::uint32_t
{
    None = 0u,
    Curve = 1u,
    Point = 2u
};

struct WorldConstraintReference
{
    WorldConstraintReferenceSubject Subject = WorldConstraintReferenceSubject::None;
    WorldCurveName Curve = {};
    std::uint32_t Point = 0u;

    bool Declared() const
    {
        return (Subject == WorldConstraintReferenceSubject::Curve && Curve.Assigned())
            || (Subject == WorldConstraintReferenceSubject::Point && Point != 0u);
    }
};

struct WorldConstraintSpecification
{
    WorldConstraintSubject Subject = WorldConstraintSubject::Fixed;
    WorldConstraintReference Primary = {};
    WorldConstraintReference Secondary = {};

    bool Declared() const
    {
        switch (Subject)
        {
            case WorldConstraintSubject::Coincident:
                return Primary.Subject == WorldConstraintReferenceSubject::Point
                    && Secondary.Subject == WorldConstraintReferenceSubject::Point
                    && Primary.Declared() && Secondary.Declared();
            case WorldConstraintSubject::Horizontal:
            case WorldConstraintSubject::Vertical:
            case WorldConstraintSubject::Fixed:
                return Primary.Declared();
            case WorldConstraintSubject::Parallel:
            case WorldConstraintSubject::Perpendicular:
            case WorldConstraintSubject::Tangent:
            case WorldConstraintSubject::Equal:
                return Primary.Declared() && Secondary.Declared();
            case WorldConstraintSubject::SubjectCount:
                return false;
        }
        return false;
    }
};

struct WorldPlacementFrame
{
    SpatialPoint Origin = {};
    SpatialDirection Normal = {};
    SpatialDirection AlongDirection = {};
    bool Declared() const;
};

struct DeclaredWorldCurve
{
    CurveSpecification Geometry = {};
    WorldPlacementFrame SupportFrame = {};
    bool SupportFrameStanding = false;
};

struct WorldCurveUse
{
    WorldCurveName TraversedCurve = {};
    bool SameSense = true;
};

struct DeclaredWorldLoop
{
    std::vector<WorldCurveUse> Traversal = {};
};

class WorldSketchStructure
{
public:
    WorldCurveName DeclareCurve(const CurveSpecification& Incoming);
    WorldCurveName DeclareCurve(const CurveSpecification& Incoming,
                                const WorldPlacementFrame& SupportFrame);
    WorldLoopName DeclareLoop(const DeclaredWorldLoop& Incoming);
    WorldConstraintName DeclareConstraint(const WorldConstraintSpecification& Incoming);

    bool DeclareCurveSupportFrame(WorldCurveName Subject,
                                  const WorldPlacementFrame& SupportFrame);

    WorldCurveName DeclareLine(const SpatialPoint& Origin,
                               const SpatialPoint& Terminus);
    WorldCurveName DeclareLine(const SpatialPoint& Origin,
                               const SpatialPoint& Terminus,
                               const WorldPlacementFrame& SupportFrame);
    WorldCurveName DeclareThreePointArc(const SpatialPoint& StartPoint,
                                        const SpatialPoint& ThroughPoint,
                                        const SpatialPoint& EndPoint);
    WorldCurveName DeclareThreePointArc(const SpatialPoint& StartPoint,
                                        const SpatialPoint& ThroughPoint,
                                        const SpatialPoint& EndPoint,
                                        const WorldPlacementFrame& SupportFrame);
    WorldCurveName DeclareCircle(const CircleCurve& Declared);
    WorldCurveName DeclareCircle(const CircleCurve& Declared,
                                 const WorldPlacementFrame& SupportFrame);
    WorldCurveName DeclareEllipse(const EllipseCurve& Declared);
    WorldCurveName DeclareEllipse(const EllipseCurve& Declared,
                                  const WorldPlacementFrame& SupportFrame);
    WorldCurveName DeclareBezier(const std::vector<SpatialPoint>& ControlPoints);
    WorldCurveName DeclareBezier(const std::vector<SpatialPoint>& ControlPoints,
                                 const WorldPlacementFrame& SupportFrame);
    WorldCurveName DeclareBasisSpline(const BasisSplineCurve& Declared);
    WorldCurveName DeclareBasisSpline(const BasisSplineCurve& Declared,
                                      const WorldPlacementFrame& SupportFrame);
    WorldCurveName DeclareRationalSpline(const RationalSplineCurve& Declared);
    WorldCurveName DeclareRationalSpline(const RationalSplineCurve& Declared,
                                         const WorldPlacementFrame& SupportFrame);
    WorldCurveName DeclareHermite(const HermiteCurve& Declared);
    WorldCurveName DeclareHermite(const HermiteCurve& Declared,
                                  const WorldPlacementFrame& SupportFrame);

    const DeclaredWorldCurve* Resolve(WorldCurveName Subject) const;
    DeclaredWorldCurve* Resolve(WorldCurveName Subject);
    const DeclaredWorldLoop* Resolve(WorldLoopName Subject) const;
    DeclaredWorldLoop* Resolve(WorldLoopName Subject);
    const WorldConstraintSpecification* Resolve(WorldConstraintName Subject) const;
    WorldConstraintSpecification* Resolve(WorldConstraintName Subject);

    void ResolveCurves(std::vector<CurveSpecification>& Delivered) const;

    const std::vector<DeclaredWorldCurve>& Curves() const { return HeldCurves; }
    std::vector<DeclaredWorldCurve>& Curves() { return HeldCurves; }
    const std::vector<DeclaredWorldLoop>& Loops() const { return HeldLoops; }
    std::vector<DeclaredWorldLoop>& Loops() { return HeldLoops; }
    std::uint32_t CurveCount() const { return static_cast<std::uint32_t>(HeldCurves.size()); }
    std::uint32_t LoopCount() const { return static_cast<std::uint32_t>(HeldLoops.size()); }
    std::uint32_t ConstraintCount() const { return static_cast<std::uint32_t>(HeldConstraints.size()); }
    const std::vector<WorldConstraintSpecification>& Constraints() const { return HeldConstraints; }
    std::vector<WorldConstraintSpecification>& Constraints() { return HeldConstraints; }
    bool Declared() const;
    void Reclaim();

private:
    std::vector<DeclaredWorldCurve> HeldCurves = {};
    std::vector<DeclaredWorldLoop> HeldLoops = {};
    std::vector<WorldConstraintSpecification> HeldConstraints = {};
};

void ResolveWorldPlacementCoordinates(const WorldPlacementFrame& Frame,
                                      const SpatialPoint& Position,
                                      double& Along,
                                      double& Across);
SpatialPoint ResolveWorldPlacementPosition(const WorldPlacementFrame& Frame,
                                           double Along,
                                           double Across);
double ResolveWorldPlacementOffset(const WorldPlacementFrame& Frame,
                                   const SpatialPoint& Position);
SpatialPoint ResolveWorldPlacementProjection(const WorldPlacementFrame& Frame,
                                             const SpatialPoint& Position);
bool ResolveWorldPlacementIntersection(const WorldPlacementFrame& Frame,
                                       const SpatialPoint& RayOrigin,
                                       const SpatialDirection& RayDirection,
                                       SpatialPoint& Delivered);

} // namespace Slate
