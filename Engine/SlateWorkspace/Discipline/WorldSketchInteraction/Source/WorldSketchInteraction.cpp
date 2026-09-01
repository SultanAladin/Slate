//============================================================================================================================================
//                                                    WORLDSKETCHINTERACTION.CPP
//============================================================================================================================================

#include "SlateWorkspace/Discipline/WorldSketchInteraction/Api/WorldSketchInteraction.h"

#include "SlateWorkspace/Discipline/TransformSequence/Api/TransformSequence.h"
#include "SlateWorkspace/Discipline/WorldSketchPicking/Api/WorldSketchScreenPicking.h"

#include <cmath>

namespace Slate
{

namespace
{

bool SamePickIdentity(const WorldPick& Left,
                      const WorldPick& Right)
{
    if (Left.Subject != Right.Subject)
        return false;

    switch (Left.Subject)
    {
        case WorldPickSubject::Point:
            return Left.Point.IssuedIndex == Right.Point.IssuedIndex;
        case WorldPickSubject::Control:
            return Left.Control.IssuedIndex == Right.Control.IssuedIndex;
        case WorldPickSubject::Curve:
            return Left.Curve.IssuedIndex == Right.Curve.IssuedIndex;
        case WorldPickSubject::Loop:
            return Left.Loop.IssuedIndex == Right.Loop.IssuedIndex;
        case WorldPickSubject::None:
            return true;
    }
    return false;
}

GizmoHandle ResolveUniversalGizmoHandle(const GizmoScreenBasis& Screen,
                                        TransformManner Manner,
                                        float PointerX,
                                        float PointerY)
{
    return ResolveGizmoHandle(Screen, Manner, PointerX, PointerY);
}

WorldPick ResolveActiveSelection(const WorldPick& SemanticSelection)
{
    return SemanticSelection;
}

} // namespace

bool RefreshWorldSketchPick(const WorldSketchStructure& Declared,
                           WorldPick& Pick)
{
    if (!Pick.Standing())
        return false;

    switch (Pick.Subject)
    {
        case WorldPickSubject::Point:
            return ResolveWorldSketchPointPosition(Declared, Pick.Point, Pick.Position);

        case WorldPickSubject::Control:
        {
            std::vector<WorldControlPlacement> Controls;
            if (!Pick.Curve.Assigned() || !ResolveWorldSketchControls(Declared, Pick.Curve, Controls))
                return false;
            for (const WorldControlPlacement& Control : Controls)
                if (Control.Name.IssuedIndex == Pick.Control.IssuedIndex)
                {
                    Pick.Position = Control.Position;
                    return true;
                }
            return false;
        }

        case WorldPickSubject::Curve:
        case WorldPickSubject::Loop:
            return true;

        case WorldPickSubject::None:
            return false;
    }

    return false;
}

bool ResolveCurvePolyline(const WorldSketchStructure& Declared,
                          WorldCurveName Curve,
                          std::vector<SpatialPoint>& Polyline)
{
    const DeclaredWorldCurve* Held = Declared.Resolve(Curve);
    if (Held == nullptr || !Held->Geometry.Declared())
        return false;

    Polyline.clear();
    AppendCurvePolyline(Held->Geometry, Polyline, 48u);
    return !Polyline.empty();
}

void RefreshWorldSelectionSet(const WorldSketchStructure& Declared, WorldSelectionSet& Set)
{
    for (auto It = Set.Items.begin(); It != Set.Items.end(); )
    {
        if (!RefreshWorldSketchPick(Declared, *It))
            It = Set.Items.erase(It);
        else
            ++It;
    }
}

void DriveWorldSketchSelectionAndTransform(const PlaneExtent& Extent,
                                          const PointerCondition& Pointer,
                                          const TextInputCondition& TextInput,
                                          const ModifierCondition& Modifiers,
                                          const SelectionOptions& Selection,
                                          const GizmoOptions& Gizmo,
                                          const ResolvedCamera& Camera,
                                          WorldSketchStructure& Declared,
                                          WorldSelectionSet& SelectionSet,
                                          WorldPick& SemanticSelection,
                                          WorldPick& HoveredSelection,
                                          WorldSketchTransformSession& Transform,
                                          bool& PointerTaken,
                                          double SessionMilliseconds,
                                          double& LastGPressedMilliseconds,
                                          GizmoHandle* HoveredHandle)
{
    HoveredSelection = {};
    if (HoveredHandle != nullptr)
        *HoveredHandle = GizmoHandle::None;

    if (Extent.Encloses(Pointer.PositionX, Pointer.PositionY))
        ResolveWorldSketchPickForElement(Declared, Camera, Extent,
                                        Pointer.PositionX, Pointer.PositionY,
                                        Selection.ResolvedTolerance(),
                                        Selection.Element, HoveredSelection);

    if (SemanticSelection.Standing() && SelectionSet.Empty())
        SetWorldPick(SelectionSet, SemanticSelection, false);

    RefreshWorldSelectionSet(Declared, SelectionSet);

    const WorldPick* Active = SelectionSet.Active();
    WorldPick ActiveSelection = Active != nullptr ? *Active : WorldPick{};
    SemanticSelection = ActiveSelection;

    if (SelectionSet.Items.size() > 1u)
    {
        SpatialPoint Centroid = {};
        for (const WorldPick& Item : SelectionSet.Items)
        {
            Centroid.Left += Item.Position.Left;
            Centroid.Up += Item.Position.Up;
            Centroid.Forward += Item.Position.Forward;
        }
        const double Count = static_cast<double>(SelectionSet.Items.size());
        Centroid.Left /= Count;
        Centroid.Up /= Count;
        Centroid.Forward /= Count;
        ActiveSelection.Position = Centroid;
    }

    GizmoHandle ResolvedHandle = GizmoHandle::None;
    const TransformManner ActiveGizmoManner = Transform.Engaged() ? Transform.Manner() : static_cast<TransformManner>(Gizmo.Manner);
    if (Gizmo.Shown && !Transform.Engaged() && ActiveSelection.Standing())
    {
        GizmoScreenBasis Screen = {};
        if (ResolveGizmoScreenBasis(Camera, Extent, ActiveSelection.Position, Screen))
            ResolvedHandle = ResolveUniversalGizmoHandle(Screen, ActiveGizmoManner, Pointer.PositionX, Pointer.PositionY);
    }
    if (HoveredHandle != nullptr)
        *HoveredHandle = ResolvedHandle;

    if (ResolvedHandle != GizmoHandle::None)
        HoveredSelection = {};

    // 🔴 Clicking empty viewport space is the standard CAD deselect gesture.
    if (!PointerTaken && !Transform.Engaged() && Pointer.ContactPressed &&
        ResolvedHandle == GizmoHandle::None && !HoveredSelection.Standing())
    {
        if (!Modifiers.Shifted)
        {
            SelectionSet.Clear();
            SemanticSelection = {};
            ActiveSelection = {};
            PointerTaken = true;
        }
    }

    if (!PointerTaken && !Transform.Engaged() && Pointer.ContactPressed &&
        ResolvedHandle == GizmoHandle::None && HoveredSelection.Standing())
    {
        SetWorldPick(SelectionSet, HoveredSelection, Modifiers.Shifted);
        const WorldPick* UpdatedActive = SelectionSet.Active();
        SemanticSelection = UpdatedActive != nullptr ? *UpdatedActive : WorldPick{};
        ActiveSelection = SemanticSelection;
        PointerTaken = true;
    }

    const TransformCommandIntake Command =
        ResolveTransformCommand(TextInput.Intake, TextInput.IntakeCount, Transform.Engaged(), Transform.Manner());

    if (!Transform.Engaged() && !SelectionSet.Empty() && Extent.Encloses(Pointer.PositionX, Pointer.PositionY))
    {
        if (ResolvedHandle == GizmoHandle::None && !PointerTaken && Pointer.ContactHeld &&
            !Pointer.ContactPressed && (std::fabs(Pointer.TravelX) + std::fabs(Pointer.TravelY)) > 0.0f &&
            HoveredSelection.Standing())
        {
            PointerTaken = StartWorldSketchTransformSession(Declared, Camera, Extent,
                                                           Pointer.PositionX, Pointer.PositionY,
                                                           SelectionSet,
                                                           TransformRestriction::Free,
                                                           false, Transform, true);
        }

        if (ResolvedHandle != GizmoHandle::None && Pointer.ContactPressed)
        {
            PointerTaken = StartWorldSketchTransformSession(Declared, Camera, Extent,
                                                           Pointer.PositionX, Pointer.PositionY,
                                                           SelectionSet,
                                                           ResolveHandleRestriction(ResolvedHandle),
                                                           false, Transform, true,
                                                           ResolveHandleManner(ResolvedHandle));
        }
        else if (Command.StartRequested)
        {
            const_cast<GizmoOptions&>(Gizmo).Manner = static_cast<GizmoManner>(Command.StartManner);
            const bool Slide = Command.StartManner == TransformManner::Move
                            && ResolveSlideRequested(Command.MoveTapCount,
                                                     SessionMilliseconds,
                                                     LastGPressedMilliseconds,
                                                     ActiveSelection.Standing());
            if (Command.MoveTapCount > 0u)
                LastGPressedMilliseconds = SessionMilliseconds;
            const TransformRestriction Restriction =
                Slide ? TransformRestriction::Curve
                      : (Command.StartManner == TransformManner::Rotate
                           ? TransformRestriction::Screen : TransformRestriction::Free);
            PointerTaken = StartWorldSketchTransformSession(Declared, Camera, Extent,
                                                           Pointer.PositionX, Pointer.PositionY,
                                                           SelectionSet, Restriction, Slide,
                                                           Transform, false, Command.StartManner);
        }
    }

    if (Transform.Engaged())
    {
        const bool SlideRequested = ResolveSlideRequested(Command.MoveTapCount,
                                                          SessionMilliseconds,
                                                          LastGPressedMilliseconds,
                                                          Transform.Target.Standing());
        if (Command.MoveTapCount > 0u)
            LastGPressedMilliseconds = SessionMilliseconds;

        if (SlideRequested)
        {
            Transform.Restriction() = TransformRestriction::Curve;
            Transform.SlideAlongCurve() = true;
            if (Transform.Target.Standing())
                Transform.AxisDirection = ResolveWorldCurveSlideDirection(
                    Declared, Transform.Target.Curve, Transform.Target.Position);
        }
        else if (Command.RestrictionRequested)
        {
            Transform.Restriction() = Command.Restriction;
            Transform.SlideAlongCurve() = false;
        }

        if (Command.NumericAppend[0] != '\0')
            AppendTransformNumericRun(Transform.Standing.Numeric, TransformNumericLimit, Command.NumericAppend);
        if (TextInput.BackspacePressed)
            RetractTransformCommand(Transform.Standing);
        if (TextInput.DeletePressed)
            ClearTransformNumeric(Transform.Standing);

        if (TextInput.CancelPressed)
        {
            CancelWorldSketchTransformSession(Declared, Transform);
            RefreshWorldSelectionSet(Declared, SelectionSet);
            const WorldPick* CurrentActive = SelectionSet.Active();
            SemanticSelection = CurrentActive != nullptr ? *CurrentActive : WorldPick{};
            PointerTaken = true;
        }
        else
        {
            UpdateWorldSketchTransformSession(Camera, Extent,
                                             Pointer.PositionX, Pointer.PositionY,
                                             Declared, Transform);
            RefreshWorldSelectionSet(Declared, SelectionSet);
            const WorldPick* CurrentActive = SelectionSet.Active();
            SemanticSelection = CurrentActive != nullptr ? *CurrentActive : WorldPick{};
            PointerTaken = true;

            if (Transform.AwaitingRelease)
            {
                if (Pointer.ContactReleased)
                    CommitWorldSketchTransformSession(Transform);
            }
            else if (TextInput.AcceptPressed || Pointer.ContactPressed)
            {
                CommitWorldSketchTransformSession(Transform);
            }
        }
    }
}

} // namespace Slate
