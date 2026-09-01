//============================================================================================================================================
//                                                      SKETCHSTRUCTURE.CPP
//============================================================================================================================================

#include "SlateShape/Geometry/CurveSpecification/Api/CurveSpecification.h"
#include "SlateShape/Sketch/SketchStructure/Api/SketchStructure.h"

#include <algorithm>
#include <cmath>

namespace Slate
{

namespace
{
    CurveName CurveReferenceOf(SketchCurveName Name)
    {
        return { Name.IssuedIndex };
    }

    constexpr double SlotHalfTurn = 3.141592653589793;

    // 📝 Squared, so a length test never takes a square root it does not need.
    constexpr double SlotDegenerateLengthSquared = 1.0e-12;

    // 📝 Below this the two runs are parallel and the corner is not a corner at all.
    constexpr double SlotCollinearRadians = 1.0e-9;

    // 🔴 A MITRE ON A FOLD-BACK RUNS AWAY TO INFINITY. The inner corner sits at `Radius / cos(Turn/2)`
    //    from the spine vertex, which diverges as the turn approaches a full reversal — a spine that
    //    doubles back on itself would place that corner kilometres away and blow the shape's extent.
    //    Eight radii is the same limit a stroke offset uses, and past it the corner is reported as a
    //    join rather than pretended to be exact.
    constexpr double SlotMitreLimit = 8.0;
}

bool SketchPlane::Declared() const
{
    return LengthSquared(Normal) > 0.0 && LengthSquared(AlongDirection) > 0.0;
}

SketchCurveName SketchStructure::DeclareCurve(const CurveSpecification& Incoming)
{
    HeldCurves.push_back({ Incoming });
    return { static_cast<std::uint32_t>(HeldCurves.size()) };
}

ProfileNameInFeature SketchStructure::DeclareProfile(const ProfileSpecification& Incoming)
{
    HeldProfiles.push_back(Incoming);
    return { static_cast<std::uint32_t>(HeldProfiles.size()) };
}

ConstraintName SketchStructure::DeclareConstraint(const ConstraintSpecification& Incoming)
{
    HeldConstraints.push_back(Incoming);
    return { static_cast<std::uint32_t>(HeldConstraints.size()) };
}

DimensionName SketchStructure::DeclareDimension(const DimensionSpecification& Incoming)
{
    HeldDimensions.push_back(Incoming);
    return { static_cast<std::uint32_t>(HeldDimensions.size()) };
}

SketchCurveName SketchStructure::DeclareLine(const SpatialPoint& Origin, const SpatialPoint& Terminus)
{
    return DeclareCurve(CurveSpecification::DeclareLine(Origin, Terminus));
}

SketchCurveName SketchStructure::DeclareThreePointArc(const SpatialPoint& StartPoint,
                                                      const SpatialPoint& ThroughPoint,
                                                      const SpatialPoint& EndPoint)
{
    return DeclareCurve(CurveSpecification::DeclareThreePointArc(StartPoint, ThroughPoint, EndPoint));
}

SketchCurveName SketchStructure::DeclareCircle(const CircleCurve& Declared)
{
    return DeclareCurve(CurveSpecification::DeclareCircle(Declared));
}

SketchCurveName SketchStructure::DeclareEllipse(const EllipseCurve& Declared)
{
    return DeclareCurve(CurveSpecification::DeclareEllipse(Declared));
}

SketchCurveName SketchStructure::DeclareOval(const EllipseCurve& Declared)
{
    return DeclareCurve(CurveSpecification::DeclareOval(Declared));
}

SketchCurveName SketchStructure::DeclareBezier(const std::vector<SpatialPoint>& ControlPoints)
{
    return DeclareCurve(CurveSpecification::DeclareBezier(ControlPoints, { 0.0, 1.0 }));
}

SketchCurveName SketchStructure::DeclareBasisSpline(const BasisSplineCurve& Declared)
{
    return DeclareCurve(CurveSpecification::DeclareBasisSpline(Declared, { 0.0, 1.0 }));
}

SketchCurveName SketchStructure::DeclareRationalSpline(const RationalSplineCurve& Declared)
{
    return DeclareCurve(CurveSpecification::DeclareRationalSpline(Declared, { 0.0, 1.0 }));
}

SketchCurveName SketchStructure::DeclareHermite(const HermiteCurve& Declared)
{
    return DeclareCurve(CurveSpecification::DeclareHermite(Declared, { 0.0, 1.0 }));
}

Deliver<bool> SketchStructure::DeclarePolyline(const std::vector<SpatialPoint>& Positions,
                                               std::vector<SketchCurveName>& DeclaredCurves)
{
    DeclaredCurves.clear();
    if (Positions.size() < 2u)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "a polyline requires at least two positions" });

    DeclaredCurves.reserve(Positions.size() - 1u);
    for (std::size_t PositionIndex = 0u; PositionIndex + 1u < Positions.size(); ++PositionIndex)
    {
        // 🔴 A ZERO-LENGTH SEGMENT IS NOT A LINE, AND ONE OF THEM BLANKED THE WHOLE SKETCH. Closing a
        //    polyline anchors the start point a second time, so the final pair was coincident and
        //    `DeclareLine` produced an UNDECLARED curve. `SketchStructure::Declared()` is all-or-
        //    nothing across every curve, and `ProjectSketchRendering` refuses outright on an
        //    undeclared sketch -- so closing a shape and pressing Enter made every shape already
        //    drawn disappear at once. Coincident neighbours are skipped rather than declared.
        const SpatialDirection Span = Difference(Positions[PositionIndex],
                                                 Positions[PositionIndex + 1u]);
        if (LengthSquared(Span) <= 0.0)
            continue;

        DeclaredCurves.push_back(DeclareLine(Positions[PositionIndex], Positions[PositionIndex + 1u]));
    }

    // ⚠️ Every pair coincident means the artist clicked one spot repeatedly; there is no polyline.
    if (DeclaredCurves.empty())
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported,
                                       "a polyline requires two distinct positions" });

    return Deliver<bool>::Result(true);
}

Deliver<ProfileNameInFeature> SketchStructure::DeclareCircleProfile(const CircleCurve& Declared)
{
    return DeclareCircleProfile(Declared, Plane);
}

Deliver<ProfileNameInFeature> SketchStructure::DeclareCircleProfile(const CircleCurve& Declared,
                                                                     const SketchPlane& ActivePlane)
{
    if (!ActivePlane.Declared())
        return Deliver<ProfileNameInFeature>::Refuse({ RefusalReason::ContentUnsupported, "the sketch plane is not declared" });
    if (Declared.Radius <= 0.0)
        return Deliver<ProfileNameInFeature>::Refuse({ RefusalReason::ContentUnsupported, "the circle requires a positive radius" });

    ProfileSpecification Profile;
    Profile.DeclarePlane({ ActivePlane.Origin, ActivePlane.Normal, ActivePlane.AlongDirection });
    ProfileLoop Loop;
    Loop.Orientation = ProfileLoopOrientation::Outer;

    const SpatialDirection StartDirection = Normalize(Declared.StartDirection);
    const SpatialDirection QuarterDirection = Normalize(Cross(Declared.Normal, StartDirection));
    for (std::uint32_t QuarterIndex = 0u; QuarterIndex < 4u; ++QuarterIndex)
    {
        const double StartRadians = 1.5707963267948966 * static_cast<double>(QuarterIndex);
        const SpatialDirection QuarterStart = Added(Scaled(StartDirection, std::cos(StartRadians)),
                                                    Scaled(QuarterDirection, std::sin(StartRadians)));
        // ⚠️ EVERY FIELD, NAMED BY POSITION. `CircularArcCurve` carries a `ThroughPoint` and a
        //    `ThroughDeclared` between the start direction and the radius. A five-element brace list
        //    silently slid the radius into `ThroughPoint` and the sweep into `ThroughDeclared`, leaving
        //    the real radius at zero — so every circle profile in the application collapsed to its own
        //    centre. It still declared, still selected, still appeared in the outliner, and drew nothing.
        CircularArcCurve Quarter = {};
        Quarter.Centre         = Declared.Centre;
        Quarter.Normal         = Declared.Normal;
        Quarter.StartDirection = QuarterStart;
        Quarter.Radius         = Declared.Radius;
        Quarter.SweepRadians   = 1.5707963267948966;

        const SketchCurveName DeclaredCurve = DeclareCurve(
            CurveSpecification::DeclareCircularArc(Quarter, { 0.0, 1.0 }));
        Loop.Traversal.push_back({ CurveReferenceOf(DeclaredCurve), true });
    }

    Profile.DeclareLoop(Loop);
    return Deliver<ProfileNameInFeature>::Result(DeclareProfile(Profile));
}

Deliver<ProfileNameInFeature> SketchStructure::DeclareEllipseProfile(const EllipseCurve& Declared)
{
    return DeclareEllipseProfile(Declared, Plane);
}

Deliver<ProfileNameInFeature> SketchStructure::DeclareEllipseProfile(const EllipseCurve& Declared,
                                                                     const SketchPlane& ActivePlane)
{
    if (!ActivePlane.Declared())
        return Deliver<ProfileNameInFeature>::Refuse({ RefusalReason::ContentUnsupported, "the sketch plane is not declared" });
    if (Declared.MajorRadius <= 0.0 || Declared.MinorRadius <= 0.0)
        return Deliver<ProfileNameInFeature>::Refuse({ RefusalReason::ContentUnsupported, "the ellipse requires positive axes" });

    ProfileSpecification Profile;
    Profile.DeclarePlane({ ActivePlane.Origin, ActivePlane.Normal, ActivePlane.AlongDirection });
    ProfileLoop Loop;
    Loop.Orientation = ProfileLoopOrientation::Outer;

    for (std::uint32_t QuarterIndex = 0u; QuarterIndex < 4u; ++QuarterIndex)
    {
        const SketchCurveName DeclaredCurve = DeclareCurve(CurveSpecification::DeclareEllipticalArc(
            { Declared.Centre, Declared.Normal, Declared.MajorDirection,
              Declared.MajorRadius, Declared.MinorRadius,
              1.5707963267948966 * static_cast<double>(QuarterIndex),
              1.5707963267948966 },
            { 0.0, 1.0 }));
        Loop.Traversal.push_back({ CurveReferenceOf(DeclaredCurve), true });
    }

    Profile.DeclareLoop(Loop);
    return Deliver<ProfileNameInFeature>::Result(DeclareProfile(Profile));
}

Deliver<ProfileNameInFeature> SketchStructure::DeclareOvalProfile(const EllipseCurve& Declared)
{
    return DeclareEllipseProfile(Declared);
}

Deliver<ProfileNameInFeature> SketchStructure::DeclareOvalProfile(const EllipseCurve& Declared,
                                                                   const SketchPlane& ActivePlane)
{
    return DeclareEllipseProfile(Declared, ActivePlane);
}

Deliver<ProfileNameInFeature> SketchStructure::DeclareRegularPolygon(const SpatialPoint& Centre,
                                                                     double Radius,
                                                                     std::uint32_t SideCount,
                                                                     const SpatialDirection& StartDirection)
{
    return DeclareRegularPolygon(Centre, Radius, SideCount, Plane, StartDirection);
}

Deliver<ProfileNameInFeature> SketchStructure::DeclareRegularPolygon(const SpatialPoint& Centre,
                                                                     double Radius,
                                                                     std::uint32_t SideCount,
                                                                     const SketchPlane& ActivePlane,
                                                                     const SpatialDirection& StartDirection)
{
    if (!ActivePlane.Declared())
        return Deliver<ProfileNameInFeature>::Refuse({ RefusalReason::ContentUnsupported, "the sketch plane is not declared" });
    if (Radius <= 0.0 || SideCount < 3u)
        return Deliver<ProfileNameInFeature>::Refuse({ RefusalReason::ContentUnsupported, "the polygon requires positive radius and three sides" });

    // 🔴 THE FIRST CORNER GOES WHERE THE ARTIST DRAGGED. It was pinned to the plane's own
    //    `AlongDirection`, so the committed polygon was rotated away from the one just previewed --
    //    the shape visibly turned on release. An unstated direction still falls back to the plane's
    //    axis, so a polygon declared from a script behaves as it always did.
    const SpatialDirection AlongDirection = LengthSquared(StartDirection) > 1.0e-12
                                          ? Normalize(StartDirection)
                                          : Normalize(ActivePlane.AlongDirection);
    const SpatialDirection AcrossDirection = Normalize(Cross(ActivePlane.Normal, AlongDirection));

    ProfileSpecification Profile;
    Profile.DeclarePlane({ ActivePlane.Origin, ActivePlane.Normal, ActivePlane.AlongDirection });
    ProfileLoop Loop;
    Loop.Orientation = ProfileLoopOrientation::Outer;

    const double StepRadians = 6.283185307179586 / static_cast<double>(SideCount);
    std::vector<SpatialPoint> Corners;
    Corners.reserve(SideCount);
    for (std::uint32_t CornerIndex = 0u; CornerIndex < SideCount; ++CornerIndex)
    {
        const double AngleRadians = StepRadians * static_cast<double>(CornerIndex);
        const SpatialDirection Offset = {
            AlongDirection.Left * Radius * std::cos(AngleRadians) + AcrossDirection.Left * Radius * std::sin(AngleRadians),
            AlongDirection.Up * Radius * std::cos(AngleRadians) + AcrossDirection.Up * Radius * std::sin(AngleRadians),
            AlongDirection.Forward * Radius * std::cos(AngleRadians) + AcrossDirection.Forward * Radius * std::sin(AngleRadians)
        };
        Corners.push_back(Added(Centre, Offset));
    }

    for (std::uint32_t EdgeIndex = 0u; EdgeIndex < SideCount; ++EdgeIndex)
    {
        const std::uint32_t NextIndex = (EdgeIndex + 1u) % SideCount;
        const SketchCurveName DeclaredCurve = DeclareLine(Corners[EdgeIndex], Corners[NextIndex]);
        Loop.Traversal.push_back({ CurveReferenceOf(DeclaredCurve), true });
    }

    Profile.DeclareLoop(Loop);
    return Deliver<ProfileNameInFeature>::Result(DeclareProfile(Profile));
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    SLOT OUTLINE GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

double ResolveSpineDistance(const std::vector<SpatialPoint>& Spine,
                            const SpatialPoint& Reference)
{
    if (Spine.empty())
        return 0.0;

    // 📝 The perpendicular foot is clamped into the segment, so a pointer beyond either end measures to
    //    that end — which is what the round cap there actually is.
    double Nearest = std::sqrt(LengthSquared(Difference(Spine.front(), Reference)));

    for (std::size_t Index = 0u; Index + 1u < Spine.size(); ++Index)
    {
        const SpatialDirection Along = Difference(Spine[Index], Spine[Index + 1u]);
        const double           Span  = LengthSquared(Along);
        if (Span <= SlotDegenerateLengthSquared)
            continue;

        const SpatialDirection Reach    = Difference(Spine[Index], Reference);
        const double           Fraction = std::clamp(Dot(Reach, Along) / Span, 0.0, 1.0);
        const SpatialPoint     Foot     = Added(Spine[Index], Scaled(Along, Fraction));

        Nearest = std::min(Nearest, std::sqrt(LengthSquared(Difference(Foot, Reference))));
    }

    return Nearest;
}

namespace
{

/// 🧩 The spine with consecutive duplicates removed, so no segment has zero length.
std::vector<SpatialPoint> ResolveSlotSpine(const std::vector<SpatialPoint>& Spine)
{
    std::vector<SpatialPoint> Distinct;
    Distinct.reserve(Spine.size());

    for (const SpatialPoint& Point : Spine)
        if (Distinct.empty() || LengthSquared(Difference(Distinct.back(), Point)) > SlotDegenerateLengthSquared)
            Distinct.push_back(Point);

    return Distinct;
}

/// 🧩 One side of a slot: the offset run along every segment, joined at each bend.
/// in    Sign  [-]  +1 walks the spine forwards on one side, -1 walks it backwards on the other
/// note  📝 Both sides are the same construction, so writing it once is what keeps the two halves of the
///        outline consistent with each other.
void AppendSlotSide(const std::vector<SpatialPoint>& Spine,
                    double Radius,
                    const SpatialDirection& Normal,
                    double Sign,
                    std::vector<CurveSpecification>& Delivered)
{
    const std::size_t SegmentCount = Spine.size() - 1u;

    // 📝 Where the run being emitted BEGINS. An inner corner moves it onto the mitre, so the trim is
    //    applied to the segment that follows rather than by adding a span between the two.
    SpatialPoint Origin         = {};
    bool         OriginStanding = false;

    for (std::size_t Step = 0u; Step < SegmentCount; ++Step)
    {
        // 📝 `Sign` reverses the walk as well as the side, so the outline stays a single loop.
        const std::size_t Index = (Sign > 0.0) ? Step : (SegmentCount - 1u - Step);

        const SpatialPoint& From = (Sign > 0.0) ? Spine[Index] : Spine[Index + 1u];
        const SpatialPoint& To   = (Sign > 0.0) ? Spine[Index + 1u] : Spine[Index];

        const SpatialDirection Along  = Normalize(Difference(From, To));
        const SpatialDirection Offset = Scaled(Normalize(Cross(Normal, Along)), Radius);

        if (!OriginStanding)
        {
            Origin         = Added(From, Offset);
            OriginStanding = true;
        }

        SpatialPoint Terminus = Added(To, Offset);

        if (Step + 1u >= SegmentCount)
        {
            Delivered.push_back(CurveSpecification::DeclareLine(Origin, Terminus));
            continue;
        }

        // 🔴 THE CORNER. The next run is offset about the same vertex but in a different direction, so
        //    the two offset points differ and something must join them. What joins them depends on
        //    WHICH WAY the spine turns relative to this side.
        const std::size_t NextIndex = (Sign > 0.0) ? (Index + 1u) : (Index - 1u);

        const SpatialPoint& Vertex     = To;
        const SpatialPoint& NextFrom   = (Sign > 0.0) ? Spine[NextIndex] : Spine[NextIndex + 1u];
        const SpatialPoint& NextTo     = (Sign > 0.0) ? Spine[NextIndex + 1u] : Spine[NextIndex];

        const SpatialDirection NextAlong  = Normalize(Difference(NextFrom, NextTo));
        const SpatialDirection NextOffset = Scaled(Normalize(Cross(Normal, NextAlong)), Radius);

        // 📝 Signed about the plane normal, so its sign says which side of the spine bulges.
        const double Turn = std::atan2(Dot(Cross(Along, NextAlong), Normal), Dot(Along, NextAlong));

        if (std::fabs(Turn) < SlotCollinearRadians)
        {
            // 📝 A straight join: the two runs are one run, so no span is closed here at all.
            continue;
        }

        Delivered.push_back(CurveSpecification::DeclareLine(Origin, Terminus));

        // 🔴 THE OUTER SIDE IS AN ARC ABOUT THE VERTEX. Every point of a swept disc's boundary is
        //    exactly `Radius` from the spine, and about a vertex that locus IS a circular arc -- the
        //    same radius and the same centre-to-boundary relation as the end caps. A straight chord
        //    here is the "weird bevel": it cuts the corner off the outside of the bend.
        if (Turn < 0.0)
        {
            CircularArcCurve Fillet = {};
            Fillet.Centre         = Vertex;
            Fillet.Normal         = Normal;
            Fillet.StartDirection = Normalize(Offset);
            Fillet.Radius         = Radius;
            Fillet.SweepRadians   = Turn;
            Delivered.push_back(CurveSpecification::DeclareCircularArc(Fillet, { 0.0, 1.0 }));

            Origin = Added(Vertex, NextOffset);
            continue;
        }

        // 🔴 THE INNER SIDE OVERLAPS AND IS TRIMMED, NOT BRIDGED. Here the two offset runs cross, so
        //    the boundary of the swept region is their intersection -- the mitre point. Joining their
        //    endpoints instead draws a chord across the inside of the bend, which is the edge that was
        //    seen cutting through the slot body.
        const double Cosine = std::cos(0.5 * Turn);

        if (Cosine > 1.0 / SlotMitreLimit)
        {
            const SpatialDirection Bisector = Normalize(Added(Normalize(Offset), Normalize(NextOffset)));
            const SpatialPoint     Mitre    = Added(Vertex, Scaled(Bisector, Radius / Cosine));

            // 📝 Both runs are pulled back onto the mitre rather than a span being inserted between
            //    them, so the inner boundary carries no spur doubling back along itself.
            Delivered.back().HeldLine().Terminus = Mitre;
            Origin = Mitre;
            continue;
        }

        // ⚠️ Past the mitre limit the spine has all but doubled back and the mitre runs away to
        //    infinity. Both runs are pulled onto the vertex itself, which is on the boundary of the
        //    swept region and so cannot escape it.
        Delivered.back().HeldLine().Terminus = Vertex;
        Origin = Vertex;
    }
}

/// 🧩 One end cap: the half turn from one side of the spine to the other, around the end.
/// note  🔴 The sweep is NEGATIVE so the cap goes the LONG way round, over the end it belongs to. A
///        positive sweep turns it towards the other cap instead, carving both semicircles out of the
///        body rather than adding them to its ends. The outline still closes either way, so nothing
///        downstream refuses it; it simply draws as two crescents biting into the slot.
void AppendSlotCap(const SpatialPoint& Centre,
                   const SpatialDirection& Normal,
                   const SpatialDirection& StartDirection,
                   double Radius,
                   std::vector<CurveSpecification>& Delivered)
{
    // ⚠️ The five-versus-seven field slide this once had is why every field is named rather than
    //    written as a brace list; a positional initialiser here silently gave both caps a zero radius.
    CircularArcCurve Cap = {};
    Cap.Centre         = Centre;
    Cap.Normal         = Normal;
    Cap.StartDirection = StartDirection;
    Cap.Radius         = Radius;
    Cap.SweepRadians   = -SlotHalfTurn;

    Delivered.push_back(CurveSpecification::DeclareCircularArc(Cap, { 0.0, 1.0 }));
}

}   // namespace

void AppendSlotOutline(const std::vector<SpatialPoint>& Spine,
                       double Radius,
                       const SpatialDirection& Normal,
                       std::vector<CurveSpecification>& Delivered)
{
    const std::vector<SpatialPoint> Run = ResolveSlotSpine(Spine);
    if (Run.size() < 2u || !(Radius > 0.0) || !(LengthSquared(Normal) > 0.0))
        return;

    const SpatialDirection PlaneNormal = Normalize(Normal);

    const SpatialDirection FirstAlong = Normalize(Difference(Run.front(), Run[1]));
    const SpatialDirection LastAlong  = Normalize(Difference(Run[Run.size() - 2u], Run.back()));
    const SpatialDirection FirstSide  = Normalize(Cross(PlaneNormal, FirstAlong));
    const SpatialDirection LastSide   = Normalize(Cross(PlaneNormal, LastAlong));

    AppendSlotSide(Run, Radius, PlaneNormal, 1.0, Delivered);
    AppendSlotCap(Run.back(), PlaneNormal, LastSide, Radius, Delivered);
    AppendSlotSide(Run, Radius, PlaneNormal, -1.0, Delivered);
    AppendSlotCap(Run.front(), PlaneNormal, Negated(FirstSide), Radius, Delivered);
}

Deliver<ProfileNameInFeature> SketchStructure::DeclareSlot(const SpatialPoint& StartPoint,
                                                           const SpatialPoint& EndPoint,
                                                           double Radius)
{
    return DeclareSlot(StartPoint, EndPoint, Radius, Plane);
}

Deliver<ProfileNameInFeature> SketchStructure::DeclareSlot(const SpatialPoint& StartPoint,
                                                           const SpatialPoint& EndPoint,
                                                           double Radius,
                                                           const SketchPlane& ActivePlane)
{
    // 📝 A two-point slot is a one-segment spine, so it is declared through the SAME outline every
    //    longer spine uses rather than through a second construction that has to be kept in step.
    return DeclarePolylineSlot({ StartPoint, EndPoint }, Radius, ActivePlane);
}

Deliver<ProfileNameInFeature> SketchStructure::DeclarePolylineSlot(const std::vector<SpatialPoint>& Spine,
                                                                   double Radius)
{
    return DeclarePolylineSlot(Spine, Radius, Plane);
}

Deliver<ProfileNameInFeature> SketchStructure::DeclarePolylineSlot(const std::vector<SpatialPoint>& Spine,
                                                                   double Radius,
                                                                   const SketchPlane& ActivePlane)
{
    if (!ActivePlane.Declared())
        return Deliver<ProfileNameInFeature>::Refuse({ RefusalReason::ContentUnsupported, "the sketch plane is not declared" });
    if (Radius <= 0.0)
        return Deliver<ProfileNameInFeature>::Refuse({ RefusalReason::ContentUnsupported, "the slot requires a positive radius" });
    if (Spine.size() < 2u)
        return Deliver<ProfileNameInFeature>::Refuse({ RefusalReason::ContentUnsupported, "the slot requires at least two points" });

    // 🔴 THE OUTLINE IS BUILT BY THE SAME CALL THE PREVIEW USES. It was built here a second time, and
    //    the two constructions drifted in exactly the way that guarantees: both joined consecutive
    //    offset runs with a straight chord, so every bend in a spine drew a bevel that cut through
    //    the slot's own body instead of the semicircle a swept disc actually traces.
    std::vector<CurveSpecification> Outline;
    AppendSlotOutline(Spine, Radius, ActivePlane.Normal, Outline);

    if (Outline.empty())
        return Deliver<ProfileNameInFeature>::Refuse({ RefusalReason::ContentUnsupported, "the slot spine points are degenerate" });

    ProfileSpecification Profile;
    Profile.DeclarePlane({ ActivePlane.Origin, ActivePlane.Normal, ActivePlane.AlongDirection });

    ProfileLoop Loop;
    Loop.Orientation = ProfileLoopOrientation::Outer;

    // 📝 The outline already arrives in traversal order, so the loop is the curves in the order they
    //    were delivered and the profile needs no second opinion about how they join.
    for (const CurveSpecification& Span : Outline)
        Loop.Traversal.push_back({ CurveReferenceOf(DeclareCurve(Span)), true });

    Profile.DeclareLoop(Loop);
    return Deliver<ProfileNameInFeature>::Result(DeclareProfile(Profile));
}

bool SketchStructure::Declared() const
{
    if (!PlaneStanding || !Plane.Declared())
        return false;

    for (const DeclaredSketchCurve& Curve : HeldCurves)
        if (!Curve.Geometry.Declared())
            return false;

    for (const ProfileSpecification& Profile : HeldProfiles)
        if (!Profile.Declared())
            return false;

    for (const ConstraintSpecification& Constraint : HeldConstraints)
        if (!Constraint.Declared())
            return false;

    for (const DimensionSpecification& Dimension : HeldDimensions)
        if (!Dimension.Declared())
            return false;

    return true;
}

void SketchStructure::Reclaim()
{
    Plane = {};
    PlaneStanding = false;
    HeldCurves.clear();
    HeldProfiles.clear();
    HeldConstraints.clear();
    HeldDimensions.clear();
}

} // namespace Slate
