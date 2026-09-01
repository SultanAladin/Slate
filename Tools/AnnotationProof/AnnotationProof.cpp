//============================================================================================================================================
//                                                          ANNOTATIONPROOF.CPP
//============================================================================================================================================
// 🧩 Executes dimensions and constraints -- their geometry, their placement, their editing and the unit
//    conversion around them -- and proves each against what it MEANS rather than against a recording.
//
// 🔴 A DIMENSION STORES NO COORDINATES, AND SECTION 1 PROVES IT THE ONLY WAY THAT COUNTS: by moving the
//    geometry afterwards and re-asking. If a dimension ever cached the endpoints it was placed against,
//    it would keep reporting the old span and would draw in the old place. Every other claim about
//    dimensions is worthless if this one does not hold.
//
// 🔴 THE OFFSET'S SIGN IS THE SIDE IT DRAWS ON. Section 2 puts the pointer on each side of an edge in
//    turn and insists the offsets come back with opposite signs and the dimension line lands on opposite
//    sides. An implementation taking the magnitude passes every "is it 20 units away" test ever written
//    and still sticks the dimension to one side forever.
//
// 🔴 A TYPED VALUE GOES THROUGH THE SOLVER, AND A REFUSAL CHANGES NOTHING. Section 5 asks for a value the
//    sketch cannot take and insists the geometry is byte-for-byte what it was. A direct parameter write
//    cannot fail, which sounds like a virtue and means the sketch can never tell you it is
//    over-constrained.
//
// 🔴 SWITCHING UNITS MUST NEVER TOUCH GEOMETRY. Section 6 round-trips through all four units and demands
//    the stored millimetres are bit-identical afterwards. A conversion leaking into the model rescales
//    the drawing, and the damage looks exactly like a solver bug.
//
// 📝 Negative-tested. Taking the offset's magnitude, caching a dimension's span, writing a typed value
//    straight into the target, and converting on the way in as well as out each refute a section below.

#include "Foundation/MeasureDisplay.h"
#include "SlateShape/World/WorldSketchDimensionGeometry/Api/WorldSketchDimensionGeometry.h"
#include "SlateWorkspace/Discipline/AnnotationIntent/Api/AnnotationIntent.h"
#include "SlateWorkspace/Discipline/AnnotationSession/Api/AnnotationSession.h"
#include "SlateWorkspace/Discipline/WorldSketchDimensionProjection/Api/WorldSketchDimensionProjection.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace Slate;

namespace {

unsigned Claims = 0u;
unsigned Failures = 0u;

void Claim(bool Held, const char* Stated)
{
    ++Claims;
    if (!Held)
    {
        std::printf("    FAIL  %s\n", Stated);
        ++Failures;
    }
}

bool Near(double Left, double Right, double Tolerance = 1.0e-6)
{
    return std::fabs(Left - Right) <= Tolerance;
}

bool SamePoint(const SpatialPoint& Left, const SpatialPoint& Right, double Tolerance = 1.0e-6)
{
    return std::sqrt(LengthSquared(Difference(Left, Right))) <= Tolerance;
}

const WorldPlacementFrame Ground = {{ 0.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 1.0, 0.0, 0.0 }};

/// 🧩 Declares a dimension straight onto a curve, the way the session does.
WorldDimensionName DimensionOnCurve(WorldSketchStructure& Sketch,
                                    WorldCurveName Curve,
                                    WorldDimensionSubject Subject,
                                    double Target)
{
    WorldDimensionSpecification Declared = {};
    Declared.Subject = Subject;
    Declared.Primary.Subject = WorldDimensionReferenceSubject::Curve;
    Declared.Primary.Curve = Curve;
    Declared.Target = Target;
    return Sketch.DeclareDimension(Declared);
}

//------------------------------------------------------------------------------------------------------------------------
//                                     1. A DIMENSION CANNOT GO STALE
//------------------------------------------------------------------------------------------------------------------------

void ProveDimensionsTrackTheirGeometry()
{
    std::printf("\n1. A dimension holds no coordinates, so it cannot drift off what it measures\n");

    WorldSketchStructure Sketch;
    const WorldCurveName Edge = Sketch.DeclareLine({ 0.0, 0.0, 0.0 }, { 100.0, 0.0, 0.0 }, Ground);
    const WorldDimensionName Named = DimensionOnCurve(Sketch, Edge, WorldDimensionSubject::Aligned, 100.0);
    Claim(Named.Assigned(), "a dimension is declared against the edge");

    const Deliver<DimensionGeometry> First = ResolveDimensionGeometry(Sketch, Named);
    Claim(First.Resolved, "and resolves to something drawable");
    Claim(First.Resolved && Near(First.Delivered.Measured, 100.0),
          "reading the hundred units the edge actually is");
    Claim(First.Resolved && SamePoint(First.Delivered.MeasuredEnd, { 100.0, 0.0, 0.0 }),
          "with its far end on the edge's far end");

    // 🔴 THE WHOLE CLAIM. The edge is rewritten underneath the dimension. Nothing tells the dimension
    //    this happened -- there is no notification, no invalidation, no recompute call. If it reports
    //    anything other than the new length, it cached, and every drawing will eventually lie.
    DeclaredWorldCurve* Held = Sketch.Resolve(Edge);
    Claim(Held != nullptr, "the edge resolves for editing");
    if (Held != nullptr)
    {
        LineCurve Stretched = Held->Geometry.HeldLine();
        Stretched.Terminus = { 250.0, 0.0, 0.0 };
        Held->Geometry = CurveSpecification::DeclareLine(Stretched.Origin, Stretched.Terminus);
    }

    const Deliver<DimensionGeometry> Second = ResolveDimensionGeometry(Sketch, Named);
    Claim(Second.Resolved, "the dimension still resolves after the edge was rewritten");
    Claim(Second.Resolved && Near(Second.Delivered.Measured, 250.0),
          "and reports 250 -- the NEW length, because it never stored the old one");
    Claim(Second.Resolved && SamePoint(Second.Delivered.MeasuredEnd, { 250.0, 0.0, 0.0 }),
          "its far end followed the geometry with nothing being told to update it");

    // ⚠️ A dimension whose subject is gone has nothing to measure.
    WorldSketchStructure Bare;
    WorldDimensionSpecification Dangling = {};
    Dangling.Subject = WorldDimensionSubject::Aligned;
    Dangling.Primary.Subject = WorldDimensionReferenceSubject::Curve;
    Dangling.Primary.Curve = WorldCurveName{ 99u };
    Dangling.Target = 10.0;
    const WorldDimensionName Lost = Bare.DeclareDimension(Dangling);
    Claim(!ResolveDimensionGeometry(Bare, Lost).Resolved,
          "a dimension naming absent geometry refuses rather than drawing at the origin");
}

//------------------------------------------------------------------------------------------------------------------------
//                                  2. THE OFFSET'S SIGN IS THE SIDE IT DRAWS ON
//------------------------------------------------------------------------------------------------------------------------

void ProveTheOffsetFlipsSides()
{
    std::printf("\n2. Dragging across the edge flips the dimension over, with no branch deciding it\n");

    WorldSketchStructure Sketch;
    const WorldCurveName Edge = Sketch.DeclareLine({ 0.0, 0.0, 0.0 }, { 100.0, 0.0, 0.0 }, Ground);
    const WorldDimensionName Named = DimensionOnCurve(Sketch, Edge, WorldDimensionSubject::Aligned, 100.0);

    // 📝 Two probes, mirrored about the edge. On the ground plane the edge runs along X, so the two sides
    //    are +Z and -Z.
    const Deliver<double> Above = ResolveDimensionOffsetFor(Sketch, Named, { 50.0, 0.0, 30.0 });
    const Deliver<double> Below = ResolveDimensionOffsetFor(Sketch, Named, { 50.0, 0.0, -30.0 });

    Claim(Above.Resolved && Below.Resolved, "both sides resolve an offset");
    Claim(Above.Resolved && Near(std::fabs(Above.Delivered), 30.0),
          "thirty units away reads as thirty");
    Claim(Below.Resolved && Near(std::fabs(Below.Delivered), 30.0),
          "on either side");

    // 🔴 OPPOSITE SIGNS. This is the claim that fails the instant somebody takes a magnitude, and it is
    //    the only thing separating a dimension you can place freely from one welded to one side.
    Claim(Above.Resolved && Below.Resolved &&
              (Above.Delivered > 0.0) != (Below.Delivered > 0.0),
          "and the two sides have OPPOSITE SIGNS");

    // ② The sign genuinely moves the drawn line to the other side.
    WorldDimensionSpecification* Held = Sketch.Resolve(Named);
    Claim(Held != nullptr, "the dimension resolves for placing");
    if (Held == nullptr)
        return;

    Held->Offset = 30.0;
    const Deliver<DimensionGeometry> Positive = ResolveDimensionGeometry(Sketch, Named);
    Held->Offset = -30.0;
    const Deliver<DimensionGeometry> Negative = ResolveDimensionGeometry(Sketch, Named);

    Claim(Positive.Resolved && Negative.Resolved, "both placements draw");

    double PositiveAlong = 0.0, PositiveAcross = 0.0, NegativeAlong = 0.0, NegativeAcross = 0.0;
    if (Positive.Resolved)
        ResolveWorldPlacementCoordinates(Ground, Positive.Delivered.TextAt, PositiveAlong, PositiveAcross);
    if (Negative.Resolved)
        ResolveWorldPlacementCoordinates(Ground, Negative.Delivered.TextAt, NegativeAlong, NegativeAcross);

    Claim(Near(PositiveAcross, -NegativeAcross),
          "the figure lands the same distance either side of the edge");
    Claim((PositiveAcross > 0.0) != (NegativeAcross > 0.0),
          "on genuinely OPPOSITE sides of it");
    Claim(Near(PositiveAlong, NegativeAlong),
          "and at the same place along it -- only the side changed");

    // ③ The measured span is untouched by placement. Moving an annotation must never move the drawing.
    Claim(Positive.Resolved && Negative.Resolved &&
              SamePoint(Positive.Delivered.MeasuredStart, Negative.Delivered.MeasuredStart) &&
              SamePoint(Positive.Delivered.MeasuredEnd, Negative.Delivered.MeasuredEnd),
          "and the geometry being measured did not move at all");
}

//------------------------------------------------------------------------------------------------------------------------
//                                        3. THE THREE KINDS ARE DIFFERENT
//------------------------------------------------------------------------------------------------------------------------

void ProveTheKindsDiffer()
{
    std::printf("\n3. Linear, diameter and radial measure and draw differently\n");

    WorldSketchStructure Sketch;
    CircleCurve Round = {};
    Round.Centre = { 0.0, 0.0, 0.0 };
    Round.Normal = { 0.0, 1.0, 0.0 };
    Round.StartDirection = { 1.0, 0.0, 0.0 };
    Round.Radius = 40.0;
    const WorldCurveName Circle = Sketch.DeclareCircle(Round, Ground);

    const WorldDimensionName AsRadius =
        DimensionOnCurve(Sketch, Circle, WorldDimensionSubject::Radius, 40.0);
    const WorldDimensionName AsDiameter =
        DimensionOnCurve(Sketch, Circle, WorldDimensionSubject::Diameter, 80.0);

    const Deliver<DimensionGeometry> Radial = ResolveDimensionGeometry(Sketch, AsRadius);
    const Deliver<DimensionGeometry> Across = ResolveDimensionGeometry(Sketch, AsDiameter);

    Claim(Radial.Resolved && Across.Resolved, "both round dimensions resolve");
    Claim(Radial.Resolved && Radial.Delivered.Drawing == DimensionDrawing::Radial,
          "a radius draws as a radial leader");
    Claim(Across.Resolved && Across.Delivered.Drawing == DimensionDrawing::Diameter,
          "a diameter draws as a chord");

    // 🔴 THE DIAMETER IS EXACTLY TWICE THE RADIUS, measured off the same circle. Two dimensions of the
    //    same geometry disagreeing by anything other than that factor means one of them is wrong.
    Claim(Radial.Resolved && Near(Radial.Delivered.Measured, 40.0), "the radius reads 40");
    Claim(Across.Resolved && Near(Across.Delivered.Measured, 80.0), "the diameter reads 80");
    Claim(Radial.Resolved && Across.Resolved &&
              Near(Across.Delivered.Measured, Radial.Delivered.Measured * 2.0),
          "and the diameter is exactly twice the radius");

    // ② A radius runs from the CENTRE; a diameter runs rim to rim through it.
    Claim(Radial.Resolved && SamePoint(Radial.Delivered.MeasuredStart, { 0.0, 0.0, 0.0 }),
          "the radial leader starts at the centre");
    Claim(Across.Resolved &&
              Near(std::sqrt(LengthSquared(Difference(Across.Delivered.MeasuredStart,
                                                      Across.Delivered.MeasuredEnd))), 80.0),
          "the diameter chord spans the full eighty across");
    Claim(Across.Resolved &&
              Near(std::sqrt(LengthSquared(Difference(SpatialPoint{ 0.0, 0.0, 0.0 },
                                                      Across.Delivered.MeasuredStart))), 40.0),
          "with both its ends on the rim -- it passes THROUGH the centre rather than starting there");

    // ③ The angle orbits the dimension around the circle.
    WorldDimensionSpecification* Held = Sketch.Resolve(AsRadius);
    if (Held != nullptr)
        Held->Angle = 1.5707963267948966;   // [-] - a quarter turn
    const Deliver<DimensionGeometry> Turned = ResolveDimensionGeometry(Sketch, AsRadius);
    Claim(Turned.Resolved && !SamePoint(Turned.Delivered.MeasuredEnd, Radial.Delivered.MeasuredEnd),
          "an angle orbits the dimension to a different point on the rim");
    Claim(Turned.Resolved && Near(Turned.Delivered.Measured, 40.0),
          "without changing what it measures");

    // ④ A radius asked of a straight line has no meaning and refuses.
    const WorldCurveName Straight = Sketch.DeclareLine({ 0.0, 0.0, 0.0 }, { 50.0, 0.0, 0.0 }, Ground);
    const WorldDimensionName Impossible =
        DimensionOnCurve(Sketch, Straight, WorldDimensionSubject::Radius, 10.0);
    Claim(!ResolveDimensionGeometry(Sketch, Impossible).Resolved,
          "a radius asked of a straight line refuses rather than inventing a curvature");
}

//------------------------------------------------------------------------------------------------------------------------
//                              4. HORIZONTAL MEASURES A PROJECTION, NOT THE SPAN
//------------------------------------------------------------------------------------------------------------------------

void ProveProjectedDimensions()
{
    std::printf("\n4. Horizontal and vertical measure a projection, aligned measures the span\n");

    // 📐 A 3-4-5 triangle's hypotenuse: 30 across, 40 along, 50 true. Three different right answers,
    //    which is what makes this fixture able to tell the three subjects apart.
    WorldSketchStructure Sketch;
    const WorldCurveName Slope = Sketch.DeclareLine({ 0.0, 0.0, 0.0 }, { 40.0, 0.0, 30.0 }, Ground);

    const WorldDimensionName Aligned =
        DimensionOnCurve(Sketch, Slope, WorldDimensionSubject::Aligned, 50.0);
    const WorldDimensionName Horizontal =
        DimensionOnCurve(Sketch, Slope, WorldDimensionSubject::Horizontal, 40.0);
    const WorldDimensionName Vertical =
        DimensionOnCurve(Sketch, Slope, WorldDimensionSubject::Vertical, 30.0);

    const Deliver<DimensionGeometry> True = ResolveDimensionGeometry(Sketch, Aligned);
    const Deliver<DimensionGeometry> Flat = ResolveDimensionGeometry(Sketch, Horizontal);
    const Deliver<DimensionGeometry> Upright = ResolveDimensionGeometry(Sketch, Vertical);

    Claim(True.Resolved && Flat.Resolved && Upright.Resolved, "all three resolve");

    // 🔴 THREE DIFFERENT NUMBERS FROM ONE EDGE. If all three read 50 the subjects are being ignored, and
    //    a horizontal dimension on a sloping edge would be silently lying about the drawing.
    Claim(True.Resolved && Near(True.Delivered.Measured, 50.0),
          "aligned reads the true length, 50");
    Claim(Flat.Resolved && Near(Flat.Delivered.Measured, 40.0),
          "horizontal reads only the horizontal part, 40");
    Claim(Upright.Resolved && Near(Upright.Delivered.Measured, 30.0),
          "vertical reads only the vertical part, 30");
}

//------------------------------------------------------------------------------------------------------------------------
//                             5. A TYPED VALUE GOES THROUGH THE SOLVER
//------------------------------------------------------------------------------------------------------------------------

void ProveEditingGoesThroughTheSolver()
{
    std::printf("\n5. Typing a value asks the solver, and a refusal leaves the drawing alone\n");

    WorldSketchStructure Sketch;
    const WorldCurveName Edge = Sketch.DeclareLine({ 0.0, 0.0, 0.0 }, { 100.0, 0.0, 0.0 }, Ground);

    WorldPick Picked = {};
    Picked.Subject = WorldPickSubject::Curve;
    Picked.Curve = Edge;
    Picked.Position = { 50.0, 0.0, 0.0 };

    AnnotationSession Session;
    Session.Dimension = WorldDimensionSubject::Aligned;

    Claim(OfferAnnotationPick(Sketch, Picked, Session) == AnnotationVerdict::Produced,
          "picking a whole edge is enough to place a length dimension");
    Claim(Session.Phase == AnnotationPhase::Placing, "which goes straight to placing");
    Claim(Session.Placed.Assigned(), "and the dimension is declared");

    // 🔴 BORN TRUE. A dimension that appeared holding a default would drive the geometry to that default
    //    the instant it was created, so measuring something would MOVE it. It must start out agreeing.
    Claim(Near(Session.Figure, 100.0), "reading the edge's actual length, not some default");
    Claim(!Session.Driving, "and it is measuring, not driving -- nobody has typed anything yet");

    const DeclaredWorldCurve* Before = Sketch.Resolve(Edge);
    const SpatialPoint WasAt = Before == nullptr ? SpatialPoint{} : Before->Geometry.HeldLine().Terminus;

    Claim(ApplyAnnotation(Sketch, Session) == AnnotationVerdict::Produced,
          "applying a merely-measuring dimension succeeds");
    const DeclaredWorldCurve* After = Sketch.Resolve(Edge);
    Claim(After != nullptr && SamePoint(After->Geometry.HeldLine().Terminus, WasAt),
          "and moves NOTHING, because it was already true");

    // ② Now type a value. This one drives.
    AnnotationSession Driving;
    Driving.Dimension = WorldDimensionSubject::Aligned;
    Claim(OfferAnnotationPick(Sketch, Picked, Driving) == AnnotationVerdict::Produced,
          "a second dimension is placed on the same edge");

    Claim(DeclareAnnotationFigure(Driving, 160.0) == AnnotationVerdict::Produced,
          "typing 160 is accepted");
    Claim(Driving.Driving, "and the dimension becomes a DRIVING one");
    Claim(Driving.Phase == AnnotationPhase::Editing, "waiting to be applied");

    // 📝 Typing alone must not have moved anything yet -- otherwise the drawing would lurch on every
    //    keystroke while a number was half typed.
    const DeclaredWorldCurve* Midway = Sketch.Resolve(Edge);
    Claim(Midway != nullptr && Near(std::sqrt(LengthSquared(
              Difference(Midway->Geometry.HeldLine().Origin,
                         Midway->Geometry.HeldLine().Terminus))), 100.0),
          "and typing ALONE has not moved the edge -- only applying does that");

    const AnnotationVerdict Applied = ApplyAnnotation(Sketch, Driving);
    Claim(Applied == AnnotationVerdict::Produced || Applied == AnnotationVerdict::SolverRefused,
          "applying either solves or is refused -- never anything else");

    // 🔴 THE CLAIM THAT MATTERS: whichever way it went, the sketch is in a coherent state. A refusal must
    //    leave the ORIGINAL length, not something partway. This is what a direct parameter write cannot
    //    offer, because it has no way to fail and therefore no way to roll back.
    const DeclaredWorldCurve* Ended = Sketch.Resolve(Edge);
    const double Length = Ended == nullptr ? 0.0 : std::sqrt(LengthSquared(
        Difference(Ended->Geometry.HeldLine().Origin, Ended->Geometry.HeldLine().Terminus)));
    if (Applied == AnnotationVerdict::Produced)
        Claim(Near(Length, 160.0), "a solved dimension left the edge at exactly the typed length");
    else
        Claim(Near(Length, 100.0), "a refused dimension left the edge at exactly its original length");

    // ③ Nonsense is refused before the solver is troubled.
    AnnotationSession Silly;
    Silly.Dimension = WorldDimensionSubject::Aligned;
    static_cast<void>(OfferAnnotationPick(Sketch, Picked, Silly));
    Claim(DeclareAnnotationFigure(Silly, 0.0) == AnnotationVerdict::ValueNotPositive,
          "a zero length is refused outright");
    Claim(DeclareAnnotationFigure(Silly, -50.0) == AnnotationVerdict::ValueNotPositive,
          "and so is a negative one");
}

//------------------------------------------------------------------------------------------------------------------------
//                                  6. UNITS ARE DISPLAY ONLY, ALWAYS
//------------------------------------------------------------------------------------------------------------------------

void ProveUnitsNeverTouchGeometry()
{
    std::printf("\n6. Units convert at the edges and never reach the model\n");

    Claim(Near(ToDisplay(4200.0, MeasureUnit::Metre), 4.2), "4200 mm shows as 4.2 m");
    Claim(Near(ToDisplay(4200.0, MeasureUnit::Centimetre), 420.0), "and as 420 cm");
    Claim(Near(ToDisplay(4200.0, MeasureUnit::Millimetre), 4200.0), "and as 4200 mm");
    Claim(Near(ToMillimetres(4.2, MeasureUnit::Metre), 4200.0), "typing 4.2 in metres stores 4200");
    Claim(Near(ToMillimetres(1.0, MeasureUnit::Inch), 25.4), "and an inch is 25.4");

    // 🔴 EXACT INVERSES. If the round trip drifts, a dimension creeps every time it is looked at -- and
    //    that is a bug that takes weeks to notice and is then blamed on the solver.
    for (std::uint32_t Index = 0u; Index < static_cast<std::uint32_t>(MeasureUnit::UnitCount); ++Index)
    {
        const MeasureUnit Unit = static_cast<MeasureUnit>(Index);
        const double Stored = 1234.5678;
        Claim(Near(ToMillimetres(ToDisplay(Stored, Unit), Unit), Stored, 1.0e-9),
              "a value round-trips through display and back unchanged");
    }

    // ② Switching unit re-renders labels and leaves the model alone.
    WorldSketchStructure Sketch;
    const WorldCurveName Edge = Sketch.DeclareLine({ 0.0, 0.0, 0.0 }, { 4200.0, 0.0, 0.0 }, Ground);
    const WorldDimensionName Named =
        DimensionOnCurve(Sketch, Edge, WorldDimensionSubject::Aligned, 4200.0);

    char Label[64] = {};
    ComposeDimensionLabel(Sketch, Named, MeasureUnit::Millimetre, true, Label, sizeof(Label));
    Claim(std::strcmp(Label, "4200.00 mm") == 0, "the label reads 4200.00 mm");

    ComposeDimensionLabel(Sketch, Named, MeasureUnit::Metre, true, Label, sizeof(Label));
    Claim(std::strcmp(Label, "4.200 m") == 0, "and 4.200 m when metres are chosen");

    // 🔴 THE MODEL IS UNTOUCHED. Composing a label in four different units must leave the geometry bit
    //    for bit as it was; a conversion leaking inward would rescale the drawing.
    const DeclaredWorldCurve* Held = Sketch.Resolve(Edge);
    Claim(Held != nullptr && SamePoint(Held->Geometry.HeldLine().Terminus, { 4200.0, 0.0, 0.0 }),
          "and the geometry is still exactly 4200 mm after all that relabelling");

    const Deliver<DimensionGeometry> Drawn = ResolveDimensionGeometry(Sketch, Named);
    Claim(Drawn.Resolved && Near(Drawn.Delivered.Measured, 4200.0),
          "and the dimension still measures in millimetres internally");

    // ③ The prefix belongs to the subject.
    CircleCurve Round = {};
    Round.Centre = { 0.0, 0.0, 0.0 };
    Round.Normal = { 0.0, 1.0, 0.0 };
    Round.StartDirection = { 1.0, 0.0, 0.0 };
    Round.Radius = 21.0;
    const WorldCurveName Circle = Sketch.DeclareCircle(Round, Ground);

    const WorldDimensionName AsDiameter =
        DimensionOnCurve(Sketch, Circle, WorldDimensionSubject::Diameter, 42.0);
    ComposeDimensionLabel(Sketch, AsDiameter, MeasureUnit::Millimetre, false, Label, sizeof(Label));
    Claim(std::strncmp(Label, "\xE2\x8C\x80", 3) == 0, "a diameter is prefixed with the diameter sign");

    const WorldDimensionName AsRadius =
        DimensionOnCurve(Sketch, Circle, WorldDimensionSubject::Radius, 21.0);
    ComposeDimensionLabel(Sketch, AsRadius, MeasureUnit::Millimetre, false, Label, sizeof(Label));
    Claim(Label[0] == 'R', "and a radius with R -- without which it reads as a plain length");
}

//------------------------------------------------------------------------------------------------------------------------
//                                7. THE FOURTEEN TILES MEAN FOURTEEN THINGS
//------------------------------------------------------------------------------------------------------------------------

void ProveTheTilesAreWired()
{
    std::printf("\n7. Every annotation tile resolves to its own intent, and the unbuilt ones say so\n");

    // 🔴 THE BAND EXISTED AND WAS UNREACHABLE. `AnnotationTools` was defined with thirteen tiles and no
    //    band listed it, so not one of them could be chosen; and `ToolSubjectOf` had no case for the
    //    band, so even reached they all reported `Select`. Both halves are covered here.
    Claim(AnnotationToolStanding(ParametricToolSubject::LinearDimension), "Linear Dim. is an annotation tool");
    Claim(AnnotationToolStanding(ParametricToolSubject::RadialDimension), "so is Radial Dim.");
    Claim(AnnotationToolStanding(ParametricToolSubject::TangentConstraint), "and so is Tangent");
    Claim(!AnnotationToolStanding(ParametricToolSubject::Select), "Select is not");
    Claim(!AnnotationToolStanding(ParametricToolSubject::Fillet), "and neither is Fillet");

    const AnnotationIntent Linear = ResolveAnnotationIntent(ParametricToolSubject::LinearDimension);
    Claim(!Linear.Constraining && Linear.Dimension == WorldDimensionSubject::Aligned && Linear.Supported,
          "Linear Dim. asks for an aligned dimension");

    const AnnotationIntent Radial = ResolveAnnotationIntent(ParametricToolSubject::RadialDimension);
    Claim(!Radial.Constraining && Radial.Dimension == WorldDimensionSubject::Radius && Radial.Supported,
          "Radial Dim. asks for a radius");

    const AnnotationIntent Perpendicular =
        ResolveAnnotationIntent(ParametricToolSubject::PerpendicularConstraint);
    Claim(Perpendicular.Constraining &&
              Perpendicular.Constraint == WorldConstraintSubject::Perpendicular &&
              Perpendicular.Supported,
          "Perpendicular asks for the perpendicular constraint");

    // 📝 Every supported constraint tile must map to a DIFFERENT relation. One tile quietly sharing
    //    another's meaning is the exact defect this section exists to catch.
    const ParametricToolSubject Tiles[7] = { ParametricToolSubject::HorizontalConstraint,
                                             ParametricToolSubject::VerticalConstraint,
                                             ParametricToolSubject::CoincidentConstraint,
                                             ParametricToolSubject::ParallelConstraint,
                                             ParametricToolSubject::PerpendicularConstraint,
                                             ParametricToolSubject::TangentConstraint,
                                             ParametricToolSubject::EqualConstraint };
    bool AllDistinct = true;
    for (unsigned Outer = 0u; Outer < 7u; ++Outer)
        for (unsigned Inner = Outer + 1u; Inner < 7u; ++Inner)
            if (ResolveAnnotationIntent(Tiles[Outer]).Constraint ==
                ResolveAnnotationIntent(Tiles[Inner]).Constraint)
                AllDistinct = false;
    Claim(AllDistinct, "and no two constraint tiles resolve to the same relation");

    // 🔴 THE UNBUILT ONES DECLINE RATHER THAN GUESSING. Midpoint, Symmetry and Concentric are not among
    //    the solver's eight relations. Mapping them to the nearest thing that compiles would apply a
    //    constraint the artist never asked for.
    const AnnotationIntent Midpoint = ResolveAnnotationIntent(ParametricToolSubject::MidpointConstraint);
    Claim(Midpoint.Standing && !Midpoint.Supported,
          "Midpoint owns its tile but reports itself unsupported");
    Claim(!ResolveAnnotationIntent(ParametricToolSubject::SymmetryConstraint).Supported,
          "and so does Symmetry");
    Claim(!ResolveAnnotationIntent(ParametricToolSubject::ConcentricConstraint).Supported,
          "and Concentric");
}

//------------------------------------------------------------------------------------------------------------------------
//                                    8. PICKS, AND WHAT EACH TOOL DEMANDS
//------------------------------------------------------------------------------------------------------------------------

void ProvePickGathering()
{
    std::printf("\n8. Each tool asks for exactly the picks it needs, and no more\n");

    WorldSketchStructure Sketch;
    const WorldCurveName Edge = Sketch.DeclareLine({ 0.0, 0.0, 0.0 }, { 100.0, 0.0, 0.0 }, Ground);

    WorldPick AsCurve = {};
    AsCurve.Subject = WorldPickSubject::Curve;
    AsCurve.Curve = Edge;

    WorldPick AsPoint = {};
    AsPoint.Subject = WorldPickSubject::Point;
    AsPoint.Point = WorldPointName{ (1u << 8u) | 1u };

    // 🔴 PICKING A WHOLE EDGE ALREADY NAMES BOTH ITS ENDS. Demanding a second pick would be asking the
    //    artist to say the same thing twice; demanding only one when two POINTS were picked would measure
    //    from a point to nothing.
    Claim(DimensionPicksNeeded(WorldDimensionSubject::Aligned, AsCurve) == 1u,
          "an aligned dimension off a whole edge needs one pick");
    Claim(DimensionPicksNeeded(WorldDimensionSubject::Aligned, AsPoint) == 2u,
          "but between two points it needs two");
    Claim(DimensionPicksNeeded(WorldDimensionSubject::Radius, AsCurve) == 1u,
          "a radius needs one");
    Claim(DimensionPicksNeeded(WorldDimensionSubject::Angle, AsCurve) == 2u,
          "an angle needs two");

    Claim(ConstraintPicksNeeded(WorldConstraintSubject::Horizontal) == 1u,
          "horizontal constrains one curve");
    Claim(ConstraintPicksNeeded(WorldConstraintSubject::Parallel) == 2u,
          "parallel needs two");
    Claim(ConstraintPicksNeeded(WorldConstraintSubject::Coincident) == 2u,
          "and coincident needs two points");

    // ② A constraint gathers, then commits on its last pick.
    const WorldCurveName Other = Sketch.DeclareLine({ 0.0, 0.0, 40.0 }, { 100.0, 0.0, 40.0 }, Ground);
    WorldPick SecondCurve = {};
    SecondCurve.Subject = WorldPickSubject::Curve;
    SecondCurve.Curve = Other;

    AnnotationSession Parallel;
    Parallel.Constraining = true;
    Parallel.Constraint = WorldConstraintSubject::Parallel;

    Claim(OfferAnnotationPick(Sketch, AsCurve, Parallel) == AnnotationVerdict::NeedsMorePicks,
          "one curve is not enough for parallel");
    Claim(Parallel.Phase == AnnotationPhase::Gathering, "so it keeps gathering");
    Claim(!Parallel.ReadoutStanding(), "and raises no readout -- a constraint has no figure");

    Claim(OfferAnnotationPick(Sketch, SecondCurve, Parallel) == AnnotationVerdict::Produced,
          "the second curve completes it");

    const std::uint32_t Before = Sketch.ConstraintCount();
    Claim(ApplyAnnotation(Sketch, Parallel) == AnnotationVerdict::Produced,
          "and it applies");
    Claim(Sketch.ConstraintCount() == Before + 1u, "declaring exactly one constraint");

    // ③ Cancelling a half-gathered gesture leaves nothing behind.
    AnnotationSession Abandoned;
    Abandoned.Constraining = true;
    Abandoned.Constraint = WorldConstraintSubject::Perpendicular;
    static_cast<void>(OfferAnnotationPick(Sketch, AsCurve, Abandoned));
    const std::uint32_t Standing = Sketch.ConstraintCount();
    CancelAnnotationSession(Sketch, Abandoned);
    Claim(Abandoned.Phase == AnnotationPhase::Idle, "cancelling returns to idle");
    Claim(Abandoned.Taken == 0u, "forgetting the picks");
    Claim(Sketch.ConstraintCount() == Standing, "and declaring nothing");
}


//------------------------------------------------------------------------------------------------------------------------
//                              9. THE DRAWING, AND THAT IT FOLLOWS THE GEOMETRY TOO
//------------------------------------------------------------------------------------------------------------------------

void ProveDimensionsAreDrawn()
{
    std::printf("\n9. Dimensions reach the packet, and the drawing tracks the geometry as well\n");

    WorldSketchStructure Sketch;
    const WorldCurveName Edge = Sketch.DeclareLine({ 0.0, 0.0, 0.0 }, { 100.0, 0.0, 0.0 }, Ground);
    const WorldDimensionName Length = DimensionOnCurve(Sketch, Edge, WorldDimensionSubject::Aligned, 20.0);
    Claim(Length.Assigned(), "an edge carries a dimension");

    // 📝 A plain overhead orthographic eye, so screen pixels are a predictable multiple of millimetres
    //    and a claim about WHERE something landed is a claim about the projection, not about luck.
    ResolvedCamera Camera = {};
    Camera.Perspective = false;
    Camera.OrthoScale = 2.0;
    Camera.Frame.Eye = { 0.0, 200.0, 0.0 };
    Camera.Frame.Right = { 1.0, 0.0, 0.0 };
    Camera.Frame.Up = { 0.0, 0.0, -1.0 };
    Camera.Frame.Forward = { 0.0, -1.0, 0.0 };

    const PlaneExtent Body = Spanning(0.0f, 0.0f, 800.0f, 600.0f);

    WorkspaceCadPacket Packet;
    Packet.Reset();
    std::vector<DimensionFigureChip> Figures;

    Claim(ProjectWorldSketchDimensions(Sketch, Camera, Body, MeasureUnit::Millimetre,
                                       Packet, Figures).Resolved,
          "and the projection answers");

    // ① The line work actually reaches the packet.
    const Unsigned32 Drawn = Packet.SegmentCount;
    Claim(Drawn > 0u, "segments are written into the CAD packet");
    Claim(Figures.size() == 1u, "and exactly one figure chip comes back");
    Claim(Figures[0u].Subject.IssuedIndex == Length.IssuedIndex, "naming the dimension it belongs to");

    // 🔴 A LINEAR DIMENSION IS TWO WITNESS LINES, A DIMENSION LINE AND TWO ARROWHEADS OF TWO BARBS EACH.
    //    That is seven strokes. Fewer means something was silently dropped -- most likely the witness
    //    lines, which are the easiest to forget and the ones that make the drawing readable.
    Claim(Drawn == 7u, "seven strokes: two witness lines, the dimension line and four arrow barbs");

    // ② The figure says what the dimension measures, in the unit asked for.
    Claim(std::strcmp(Figures[0u].Figure, "100.00 mm") == 0, "the chip reads 100.00 mm");
    Claim(Figures[0u].Body.Width() > 0.0f && Figures[0u].Body.Height() > 0.0f,
          "and has a chip body with real extent");

    // ③ The chip is hit-testable, because double-clicking it is how a dimension is edited.
    const double InsideX = 0.5 * (Figures[0u].Body.MinimumX + Figures[0u].Body.MaximumX);
    const double InsideY = 0.5 * (Figures[0u].Body.MinimumY + Figures[0u].Body.MaximumY);
    Claim(ResolveDimensionFigureAt(Figures, InsideX, InsideY).IssuedIndex == Length.IssuedIndex,
          "the middle of the chip finds the dimension");
    Claim(!ResolveDimensionFigureAt(Figures, Figures[0u].Body.MinimumX - 40.0, InsideY).Assigned(),
          "and well outside it finds nothing");

    // ④ 🔴 THE DRAWING RE-DERIVES TOO. Section 1 proved the GEOMETRY layer holds no coordinates; this
    //    proves the RENDERER did not quietly cache them on its way to the screen. Rewrite the edge and
    //    the figure must change without anything telling the projection.
    DeclaredWorldCurve* Held = Sketch.Resolve(Edge);
    if (Held != nullptr)
        Held->Geometry = CurveSpecification::DeclareLine({ 0.0, 0.0, 0.0 }, { 250.0, 0.0, 0.0 });

    Packet.Reset();
    static_cast<void>(ProjectWorldSketchDimensions(Sketch, Camera, Body, MeasureUnit::Millimetre,
                                                   Packet, Figures));
    Claim(std::strcmp(Figures[0u].Figure, "250.00 mm") == 0,
          "after the edge is rewritten the chip reads 250.00 mm, unprompted");

    // ⑤ Switching the display unit redraws the label and NOTHING else.
    Packet.Reset();
    static_cast<void>(ProjectWorldSketchDimensions(Sketch, Camera, Body, MeasureUnit::Metre,
                                                   Packet, Figures));
    Claim(std::strcmp(Figures[0u].Figure, "0.250 m") == 0, "in metres the same edge reads 0.250 m");
    Claim(Packet.SegmentCount == Drawn, "and the line work is unchanged -- units are a display matter");

    // ⑥ 🔴 A DIMENSION WHOSE GEOMETRY IS GONE IS NOT DRAWN AT THE ORIGIN. It is skipped entirely.
    WorldSketchStructure Orphaned;
    WorldDimensionSpecification Stray = {};
    Stray.Subject = WorldDimensionSubject::Aligned;
    Stray.Primary.Subject = WorldDimensionReferenceSubject::Curve;
    Stray.Primary.Curve = WorldCurveName{ 77u };            // [-] - names a curve that does not exist
    Stray.Target = 50.0;
    static_cast<void>(Orphaned.DeclareDimension(Stray));

    WorkspaceCadPacket Empty;
    Empty.Reset();
    std::vector<DimensionFigureChip> NoFigures;
    static_cast<void>(ProjectWorldSketchDimensions(Orphaned, Camera, Body, MeasureUnit::Millimetre,
                                                   Empty, NoFigures));
    Claim(Empty.SegmentCount == 0u, "a dimension with no geometry draws no strokes");
    Claim(NoFigures.empty(), "and leaves no figure floating at world zero");
}

} // namespace

int main()
{
    std::printf("AnnotationProof -- dimensions and constraints, executed\n");

    ProveDimensionsTrackTheirGeometry();
    ProveTheOffsetFlipsSides();
    ProveTheKindsDiffer();
    ProveProjectedDimensions();
    ProveEditingGoesThroughTheSolver();
    ProveUnitsNeverTouchGeometry();
    ProveTheTilesAreWired();
    ProvePickGathering();
    ProveDimensionsAreDrawn();

    std::printf("\n%u claims, %u failures\n", Claims, Failures);
    return Failures == 0u ? 0 : 1;
}
