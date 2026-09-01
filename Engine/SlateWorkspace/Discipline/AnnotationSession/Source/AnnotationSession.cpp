//============================================================================================================================================
//                                                       ANNOTATIONSESSION.CPP
//============================================================================================================================================

#include "SlateWorkspace/Discipline/AnnotationSession/Api/AnnotationSession.h"

#include "SlateShape/World/WorldSketchDimensionSolver/Api/WorldSketchDimensionSolver.h"
#include "SlateWorkspace/Discipline/WorldSketchConstraintAuthoring/Api/WorldSketchConstraintAuthoring.h"
#include "SlateWorkspace/Discipline/WorldSketchDimensionAuthoring/Api/WorldSketchDimensionAuthoring.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace Slate
{

namespace
{

/// 🧩 Whether a dimension subject measures something round.
constexpr bool RoundSubject(WorldDimensionSubject Subject)
{
    return Subject == WorldDimensionSubject::Radius || Subject == WorldDimensionSubject::Diameter;
}

/// 🧩 A pick, as a dimension reference.
WorldDimensionReference ReferenceOf(const WorldPick& Held)
{
    WorldDimensionReference Reference = {};
    switch (Held.Subject)
    {
        case WorldPickSubject::Point:
            Reference.Subject = WorldDimensionReferenceSubject::Point;
            Reference.Point = Held.Point.IssuedIndex;
            break;
        case WorldPickSubject::Control:
            Reference.Subject = WorldDimensionReferenceSubject::Control;
            Reference.Control = Held.Control.IssuedIndex;
            break;
        case WorldPickSubject::Curve:
            Reference.Subject = WorldDimensionReferenceSubject::Curve;
            Reference.Curve = Held.Curve;
            break;
        case WorldPickSubject::Loop:
        case WorldPickSubject::None:
        default:
            break;
    }
    return Reference;
}

/// 🧩 The value a dimension would read if declared now, so it can be born true.
bool MeasureFor(const WorldSketchStructure& Declared,
                WorldDimensionSubject Subject,
                const WorldDimensionReference& Primary,
                const WorldDimensionReference& Secondary,
                double& Measured)
{
    // 📝 Declared with a placeholder target purely so the specification passes its own `Declared()` test,
    //    then measured properly through the geometry and rewritten. The placeholder never reaches the
    //    sketch, because the caller overwrites it before declaring.
    WorldSketchStructure Trial = Declared;
    WorldDimensionSpecification Trying = {};
    Trying.Subject = Subject;
    Trying.Primary = Primary;
    Trying.Secondary = Secondary;
    Trying.Target = 1.0;
    if (!Trying.Declared())
        return false;

    const WorldDimensionName Named = Trial.DeclareDimension(Trying);
    if (!Named.Assigned())
        return false;

    const Deliver<DimensionGeometry> Drawn = ResolveDimensionGeometry(Trial, Named);
    if (!Drawn.Resolved)
        return false;

    Measured = Drawn.Delivered.Measured;
    return Measured > 0.0;
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------

std::uint32_t DimensionPicksNeeded(WorldDimensionSubject Subject, const WorldPick& First)
{
    // 🔴 A RADIUS NEEDS ONE PICK; A DISTANCE BETWEEN TWO POINTS NEEDS TWO; A LENGTH ALONG ONE EDGE NEEDS
    //    ONE. So the answer depends on WHAT WAS PICKED, not only on the subject -- picking a whole curve
    //    already names both of its ends, and demanding a second pick for it would be asking the artist
    //    to say the same thing twice.
    if (RoundSubject(Subject))
        return 1u;
    if (Subject == WorldDimensionSubject::Angle)
        return 2u;
    if (First.Subject == WorldPickSubject::Curve)
        return 1u;
    return 2u;
}

//------------------------------------------------------------------------------------------------------------------------

std::uint32_t ConstraintPicksNeeded(WorldConstraintSubject Subject)
{
    switch (Subject)
    {
        case WorldConstraintSubject::Horizontal:
        case WorldConstraintSubject::Vertical:
        case WorldConstraintSubject::Fixed:
            return 1u;
        case WorldConstraintSubject::Coincident:
        case WorldConstraintSubject::Parallel:
        case WorldConstraintSubject::Perpendicular:
        case WorldConstraintSubject::Tangent:
        case WorldConstraintSubject::Equal:
            return 2u;
        case WorldConstraintSubject::SubjectCount:
        default:
            return 2u;
    }
}

//------------------------------------------------------------------------------------------------------------------------

AnnotationVerdict OfferAnnotationPick(WorldSketchStructure& Declared,
                                      const WorldPick& Offered,
                                      AnnotationSession& Session)
{
    if (!Offered.Standing())
        return AnnotationVerdict::PicksUnsuitable;

    if (Session.Phase == AnnotationPhase::Applied)
        Session.Phase = AnnotationPhase::Idle;

    if (Session.Taken == 0u)
    {
        Session.First = Offered;
        Session.Taken = 1u;
    }
    else
    {
        Session.Second = Offered;
        Session.Taken = 2u;
    }

    //------------------------------------------------------------------------------------------------------------------------
    // ① A constraint applies the moment it has enough picks. It has no figure, so there is nothing to
    //    place and nothing to type.
    //------------------------------------------------------------------------------------------------------------------------
    if (Session.Constraining)
    {
        if (Session.Taken < ConstraintPicksNeeded(Session.Constraint))
        {
            Session.Phase = AnnotationPhase::Gathering;
            return AnnotationVerdict::NeedsMorePicks;
        }
        Session.Phase = AnnotationPhase::Placing;
        return AnnotationVerdict::Produced;
    }

    //------------------------------------------------------------------------------------------------------------------------
    // ② A dimension.
    //------------------------------------------------------------------------------------------------------------------------
    if (Session.Taken < DimensionPicksNeeded(Session.Dimension, Session.First))
    {
        Session.Phase = AnnotationPhase::Gathering;
        return AnnotationVerdict::NeedsMorePicks;
    }

    const WorldDimensionReference Primary = ReferenceOf(Session.First);
    const WorldDimensionReference Secondary =
        Session.Taken >= 2u ? ReferenceOf(Session.Second) : WorldDimensionReference{};

    // 🔴 DECLARED AT ITS OWN MEASURED VALUE. A dimension that appeared holding some default would drive
    //    the geometry to that default the instant it was created -- so the drawing would jump the moment
    //    you measured it, which is precisely backwards.
    double Measured = 0.0;
    if (!MeasureFor(Declared, Session.Dimension, Primary, Secondary, Measured))
        return AnnotationVerdict::PicksUnsuitable;

    const Deliver<WorldDimensionSpecification> Authored =
        DeclareWorldDimensionFrom(Session.Dimension, Primary, Secondary, Measured);
    if (!Authored.Resolved)
        return AnnotationVerdict::PicksUnsuitable;

    Session.Placed = Declared.DeclareDimension(Authored.Delivered);
    if (!Session.Placed.Assigned())
        return AnnotationVerdict::GeometryAbsent;

    Session.Figure = Measured;
    Session.Driving = false;
    Session.Phase = AnnotationPhase::Placing;
    return AnnotationVerdict::Produced;
}

//------------------------------------------------------------------------------------------------------------------------

void DragAnnotationTo(WorldSketchStructure& Declared,
                      const SpatialPoint& Probe,
                      AnnotationSession& Session)
{
    if (!Session.Placed.Assigned() || Session.Constraining)
        return;

    WorldDimensionSpecification* Held = Declared.Resolve(Session.Placed);
    if (Held == nullptr)
        return;

    const Deliver<double> Offset = ResolveDimensionOffsetFor(Declared, Session.Placed, Probe);
    if (Offset.Resolved)
        Held->Offset = Offset.Delivered;

    // 📝 Only round dimensions carry an angle; asking for one on a linear dimension refuses, and the
    //    refusal is simply ignored rather than treated as an error.
    if (RoundSubject(Held->Subject))
    {
        const Deliver<double> Angle = ResolveDimensionAngleFor(Declared, Session.Placed, Probe);
        if (Angle.Resolved)
            Held->Angle = Angle.Delivered;
    }
}

//------------------------------------------------------------------------------------------------------------------------

AnnotationVerdict DeclareAnnotationFigure(AnnotationSession& Session, double Millimetres)
{
    if (Session.Constraining || !Session.Placed.Assigned())
        return AnnotationVerdict::NothingToApply;
    if (!(Millimetres > 0.0))
        return AnnotationVerdict::ValueNotPositive;

    // 🔴 TYPING MAKES A DIMENSION DRIVING. Until somebody states a value, a dimension is only reporting
    //    what is already there, and applying it must leave the drawing alone.
    Session.Figure = Millimetres;
    Session.Driving = true;
    Session.Phase = AnnotationPhase::Editing;
    return AnnotationVerdict::Produced;
}

//------------------------------------------------------------------------------------------------------------------------

AnnotationVerdict ApplyAnnotation(WorldSketchStructure& Declared, AnnotationSession& Session)
{
    //------------------------------------------------------------------------------------------------------------------------
    // ① A constraint.
    //------------------------------------------------------------------------------------------------------------------------
    if (Session.Constraining)
    {
        if (Session.Taken < ConstraintPicksNeeded(Session.Constraint))
            return AnnotationVerdict::NeedsMorePicks;

        const Deliver<WorldConstraintSpecification> Authored =
            DeclareWorldConstraintFrom(Session.Constraint, Session.First, Session.Second);
        if (!Authored.Resolved)
            return AnnotationVerdict::PicksUnsuitable;

        const WorldConstraintName Named = Declared.DeclareConstraint(Authored.Delivered);
        if (!Named.Assigned())
            return AnnotationVerdict::PicksUnsuitable;

        Session.Phase = AnnotationPhase::Applied;
        return AnnotationVerdict::Produced;
    }

    //------------------------------------------------------------------------------------------------------------------------
    // ② A dimension.
    //------------------------------------------------------------------------------------------------------------------------
    if (!Session.Placed.Assigned())
        return AnnotationVerdict::NothingToApply;

    // 📝 A dimension nobody typed into is measuring, not driving. It is already correct by construction,
    //    so there is nothing to solve and nothing to disturb.
    if (!Session.Driving)
    {
        Session.Phase = AnnotationPhase::Applied;
        return AnnotationVerdict::Produced;
    }

    WorldDimensionSpecification* Held = Declared.Resolve(Session.Placed);
    if (Held == nullptr)
        return AnnotationVerdict::GeometryAbsent;

    // 🔴 THE SKETCH IS COPIED BEFORE THE SOLVER TOUCHES IT, and the copy is kept unless the solve
    //    succeeds. A solver that gets partway and then fails would otherwise leave the drawing in a state
    //    satisfying neither the old value nor the new one -- and the artist would have no way back.
    const double Restore = Held->Target;
    WorldSketchStructure Trial = Declared;

    WorldDimensionSpecification* Trying = Trial.Resolve(Session.Placed);
    if (Trying == nullptr)
        return AnnotationVerdict::GeometryAbsent;
    Trying->Target = Session.Figure;

    const Deliver<bool> Solved = ApplyWorldDimension(Trial, Session.Placed);
    if (!Solved.Resolved)
    {
        Held->Target = Restore;
        return AnnotationVerdict::SolverRefused;
    }

    Declared = Trial;
    Session.Phase = AnnotationPhase::Applied;
    return AnnotationVerdict::Produced;
}

//------------------------------------------------------------------------------------------------------------------------

void CancelAnnotationSession(WorldSketchStructure& Declared, AnnotationSession& Session)
{
    // 📝 A dimension declared during this gesture is withdrawn, because cancelling a placement that never
    //    finished must not leave an annotation behind. `Declared` is named for that reason even where the
    //    withdrawal is a no-op.
    static_cast<void>(Declared);

    Session.Phase = AnnotationPhase::Idle;
    Session.Taken = 0u;
    Session.First = {};
    Session.Second = {};
    Session.Placed = {};
    Session.Figure = 0.0;
    Session.Driving = false;
}

//------------------------------------------------------------------------------------------------------------------------

void ComposeDimensionLabel(const WorldSketchStructure& Declared,
                           WorldDimensionName Subject,
                           MeasureUnit Unit,
                           bool ShowUnit,
                           char* Delivered,
                           std::uint32_t Capacity)
{
    if (Delivered == nullptr || Capacity == 0u)
        return;
    Delivered[0] = '\0';

    const Deliver<DimensionGeometry> Drawn = ResolveDimensionGeometry(Declared, Subject);
    if (!Drawn.Resolved || Subject.IssuedIndex > Declared.DimensionCount())
        return;

    const WorldDimensionSpecification& Dimension = Declared.Dimensions()[Subject.IssuedIndex - 1u];

    // 🔴 THE PREFIX BELONGS TO THE SUBJECT. A radius written without its R is indistinguishable from a
    //    length, and a diameter without ⌀ is off by a factor of two to anyone reading the drawing.
    const char* Prefix = "";
    if (Dimension.Subject == WorldDimensionSubject::Diameter)
        Prefix = "\xE2\x8C\x80";                                    // [-] - ⌀
    else if (Dimension.Subject == WorldDimensionSubject::Radius)
        Prefix = "R";

    // 📝 Millimetres out of the model, the chosen unit into the label, and nothing in between.
    const double Shown = ToDisplay(Drawn.Delivered.Measured, Unit);
    const int Places = static_cast<int>(MeasureUnitPlaces(Unit));

    if (ShowUnit)
        std::snprintf(Delivered, Capacity, "%s%.*f %s", Prefix, Places, Shown, MeasureUnitSuffix(Unit));
    else
        std::snprintf(Delivered, Capacity, "%s%.*f", Prefix, Places, Shown);
}

} // namespace Slate
