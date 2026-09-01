//============================================================================================================================================
//                                                     SKETCHOPERATIONDRIVER.CPP
//============================================================================================================================================

#include "SlateWorkspace/Discipline/SketchOperationDriver/Api/SketchOperationDriver.h"

#include "SlateShape/World/WorldSketchAnalysis/Api/WorldSketchAnalysis.h"
#include "SlateUI/Interface/OptionControls/Api/OptionControls.h"

#include <algorithm>
#include <cmath>

namespace Slate
{

namespace
{

constexpr double ProjectionPi = 3.14159265358979323846;

/// 🧩 The ray under the pointer, in world space.
/// 📝 The same derivation the picker and the gizmo use. It is repeated rather than shared because the two
///    that have it are both `namespace {}` locals in units this one must not depend on; a third copy is
///    the lesser evil against an upward edge in the unit graph.
bool RayUnderPointer(const ResolvedCamera& Camera,
                     const PlaneExtent& Extent,
                     float ScreenX,
                     float ScreenY,
                     SpatialPoint& RayOrigin,
                     SpatialDirection& RayDirection)
{
    const double CentreX = Extent.MinimumX + Extent.Width() * 0.5;
    const double CentreY = Extent.MinimumY + Extent.Height() * 0.5;
    const double NdcX = (static_cast<double>(ScreenX) - CentreX)
                      / std::max(static_cast<double>(Extent.Width()) * 0.5, 1.0);
    const double NdcY = (CentreY - static_cast<double>(ScreenY))
                      / std::max(static_cast<double>(Extent.Height()) * 0.5, 1.0);

    if (!Camera.Perspective)
    {
        const double Along = NdcX / std::max(Camera.OrthoScale, 0.001) * (Extent.Width() * 0.5);
        const double Upward = NdcY / std::max(Camera.OrthoScale, 0.001) * (Extent.Height() * 0.5);
        RayOrigin = Added(Camera.Frame.Eye,
                          Added(Scaled(Camera.Frame.Right, Along), Scaled(Camera.Frame.Up, Upward)));
        RayDirection = Normalize(Camera.Frame.Forward);
        return true;
    }

    const double TanHalf = std::tan(Camera.FieldOfViewDegrees * 0.5 * ProjectionPi / 180.0);
    const double Aspect = Extent.Width() / std::max(Extent.Height(), 1.0f);
    RayOrigin = Camera.Frame.Eye;
    RayDirection = Normalize(Added(Added(Scaled(Camera.Frame.Right, NdcX * TanHalf * Aspect),
                                         Scaled(Camera.Frame.Up, NdcY * TanHalf)),
                                   Camera.Frame.Forward));
    return true;
}

/// 🧩 Where the pointer meets the active workplane.
/// 🔴 EVERY OPERATION ASKS THIS ONCE AND SHARES THE ANSWER. Two arms deriving the point separately would
///    eventually disagree by a rounding step, and the preview would name a different corner from the
///    commit -- which is a bug nobody could reproduce.
bool PointerOnWorkplane(const ResolvedCamera& Camera,
                        const PlaneExtent& Extent,
                        const PointerCondition& Pointer,
                        const WorldPlacementFrame& Workplane,
                        SpatialPoint& Position)
{
    if (!Workplane.Declared())
        return false;

    SpatialPoint RayOrigin = {};
    SpatialDirection RayDirection = {};
    if (!RayUnderPointer(Camera, Extent, Pointer.PositionX, Pointer.PositionY, RayOrigin, RayDirection))
        return false;

    return ResolveWorldPlacementIntersection(Workplane, RayOrigin, RayDirection, Position);
}

/// 🧩 How many world units one screen pixel spans, at a stated point, under this camera.
/// 🔴 THE WHOLE REASON FILLET AND CHAMFER LOOKED DEAD. A probe reach fixed in world units is hittable at
///    exactly one zoom: at metre scale a twelve-millimetre reach is a fraction of a pixel, so the pointer
///    can never land inside it, no corner is ever resolved, and the tool never leaves `Idle`. Converting a
///    constant PIXEL reach through the camera keeps the target the same size on screen however far in or
///    out the artist is, which is what the curve picker has always done.
/// 📝 Orthographic is exact -- `OrthoScale` IS pixels per world unit. Perspective varies with depth, so it
///    is measured at the probe: the span of one pixel at that distance along the view.
double WorldUnitsPerPixel(const ResolvedCamera& Camera,
                          const PlaneExtent& Extent,
                          const SpatialPoint& At)
{
    if (!Camera.Perspective)
        return 1.0 / std::max(Camera.OrthoScale, 1.0e-6);

    const double Height = std::max(static_cast<double>(Extent.Height()), 1.0);
    const double TanHalf = std::tan(Camera.FieldOfViewDegrees * 0.5 * ProjectionPi / 180.0);

    // 📐 Depth along the view direction, not the straight-line distance: a point off to the side of a
    //    perspective frustum is further from the eye than it is deep, and using the longer figure would
    //    quietly widen the reach toward the edges of the leaf.
    const double Depth = std::abs(Dot(Difference(Camera.Frame.Eye, At),
                                      Normalize(Camera.Frame.Forward)));

    return 2.0 * std::max(Depth, 1.0e-6) * TanHalf / Height;
}

/// 🧩 Which corner manner a tool asks for, and whether it asks for one at all.
bool CornerMannerFor(ParametricToolSubject Subject, CornerManner& Manner)
{
    if (Subject == ParametricToolSubject::Fillet)
    {
        Manner = CornerManner::Fillet;
        return true;
    }
    if (Subject == ParametricToolSubject::Chamfer)
    {
        Manner = CornerManner::Chamfer;
        return true;
    }
    return false;
}

/// 🧩 Which operation manner a tool asks for, and whether it asks for one at all.
bool OperationMannerFor(ParametricToolSubject Subject, OperationManner& Manner)
{
    switch (Subject)
    {
        case ParametricToolSubject::Cut:      Manner = OperationManner::Cut;    return true;
        case ParametricToolSubject::Trim:     Manner = OperationManner::Trim;   return true;
        case ParametricToolSubject::Extend:   Manner = OperationManner::Extend; return true;
        case ParametricToolSubject::Offset:   Manner = OperationManner::Offset; return true;
        case ParametricToolSubject::FillFace: Manner = OperationManner::Fill;   return true;
        default:                                                               return false;
    }
}

/// 🧩 The heading and glyph a readout shows for a tool.
void TitleFor(ParametricToolSubject Subject, const char*& Title, SymbolSubject& Glyph)
{
    switch (Subject)
    {
        case ParametricToolSubject::Fillet:
            Title = "Fillet";
            Glyph = SymbolSubject::FilletRadius;
            break;
        case ParametricToolSubject::Chamfer:
            Title = "Chamfer";
            Glyph = SymbolSubject::BevelChamfer;
            break;
        case ParametricToolSubject::Offset:
            Title = "Offset";
            Glyph = SymbolSubject::FilletRadius;
            break;
        default:
            Title = "Operation";
            Glyph = SymbolSubject::SubjectCount;
            break;
    }
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------

void DriveSketchOperations(const PlaneExtent& Bounds,
                           const PointerCondition& Pointer,
                           const ResolvedCamera& Camera,
                           ParametricToolSubject ActiveTool,
                           const WorldPlacementFrame& Workplane,
                           const std::vector<WorldCurveName>& Selection,
                           WorldSketchStructure& World,
                           SketchOperationState& State,
                           ToolContextMenu& Readout,
                           bool& PointerTaken)
{
    // 🔴 CHANGING TOOL ABANDONS WHATEVER WAS IN FLIGHT. A half-dragged fillet left standing when the
    //    artist picks Trim would apply itself the next time anything called Apply -- writing geometry the
    //    artist asked for under a different tool and then walked away from.
    if (ActiveTool != State.Prepared)
    {
        CancelCornerDragSession(State.Corner);
        CancelSketchOperationSession(State.Operation);
        Readout.Close();
        State.Prepared = ActiveTool;
    }

    if (!OperationToolStanding(ActiveTool))
        return;

    SpatialPoint Probe = {};
    const bool Reached = PointerOnWorkplane(Camera, Bounds, Pointer, Workplane, Probe);

    // 📝 The contact only counts when the pointer is actually on the plane. Off it, the operation is
    //    blind, and consuming a press it cannot place would swallow clicks for no reason.
    const bool Pressed  = Reached && Pointer.ContactPressed;
    const bool Held     = Reached && Pointer.ContactHeld;
    const bool Released = Reached && Pointer.ContactReleased;

    CornerManner Manner = CornerManner::Fillet;
    OperationManner Operating = OperationManner::Cut;

    //------------------------------------------------------------------------------------------------------------------------
    // ① Fillet and Chamfer: the corner gesture.
    //------------------------------------------------------------------------------------------------------------------------
    if (CornerMannerFor(ActiveTool, Manner))
    {
        State.Corner.Manner = Manner;

        // 🔴 A REACH THE ARTIST CAN SEE. Converted from a constant pixel target through this frame's
        //    camera, so grabbing a corner takes the same gesture whether the part is ten millimetres
        //    or ten metres across.
        const double Reach = CornerProbeReachPixels * WorldUnitsPerPixel(Camera, Bounds, Probe);

        AdvanceCornerDragSession(World, { Probe, Pressed, Held, Released, Reach }, State.Corner);

        if (State.Corner.PopupStanding() && !Readout.Standing())
            Readout.Open();
        if (!State.Corner.PopupStanding() && Readout.Standing())
            Readout.Close();

        if (State.Corner.Phase == CornerPhase::Dragging || State.Corner.Phase == CornerPhase::Pending)
            State.Figure = static_cast<float>(State.Corner.Radius);

        if (State.Corner.Phase != CornerPhase::Idle)
            PointerTaken = true;
    }
    //------------------------------------------------------------------------------------------------------------------------
    // ② Cut, Trim, Extend, Offset, Fill.
    //------------------------------------------------------------------------------------------------------------------------
    else if (OperationMannerFor(ActiveTool, Operating))
    {
        State.Operation.Manner = Operating;
        State.Chain = Selection;

        AdvanceSketchOperationSession(World, State.Chain, Workplane,
                                      { Probe, Pressed, Held, Released }, State.Operation);

        // 🔴 THE CLICK OPERATIONS COMMIT HERE AND RAISE NOTHING. Only Offset carries a figure, so only
        //    Offset reaches the readout at all -- which is why the popup is asked about the session's
        //    own answer rather than about which tool is active.
        if (State.Operation.Phase == OperationPhase::Applied)
        {
            std::vector<WorldCurveName> Produced;
            static_cast<void>(PerformSketchOperation(World, State.Chain, Workplane,
                                                     State.Operation, Produced));
        }

        if (State.Operation.ReadoutStanding() && !Readout.Standing())
            Readout.Open();
        if (!State.Operation.ReadoutStanding() && Readout.Standing())
            Readout.Close();

        if (State.Operation.Phase == OperationPhase::Dragging ||
            State.Operation.Phase == OperationPhase::Pending)
            State.Figure = static_cast<float>(State.Operation.Distance);

        if (State.Operation.Phase != OperationPhase::Idle)
            PointerTaken = true;
    }

    //------------------------------------------------------------------------------------------------------------------------
    // ③ The readout, for the two operations that carry a figure.
    //------------------------------------------------------------------------------------------------------------------------
    if (!Readout.Standing())
        return;

    const double Limit = CornerMannerFor(ActiveTool, Manner) ? State.Corner.Limit
                                                             : State.Operation.Limit;

    const char* Title = "";
    SymbolSubject Glyph = SymbolSubject::SubjectCount;
    TitleFor(ActiveTool, Title, Glyph);

    OptionDeclaration Rows[1] = {};
    Rows[0].Kind    = OptionControl::Slider;
    Rows[0].Caption = ActiveTool == ParametricToolSubject::Offset ? "Distance" : "Radius";
    Rows[0].Unit    = "mm";
    Rows[0].Reading = &State.Figure;

    // 🔴 THE SLIDER'S RANGE IS THE GESTURE'S OWN CLAMP, not a constant. A readout that let the artist
    //    type or drag past the limit would be a way around the clamp rather than a way to be precise
    //    inside it, and the operation would refuse at a number the readout had just offered.
    Rows[0].Minimum = ActiveTool == ParametricToolSubject::Offset ? -static_cast<float>(Limit) : 0.0f;
    Rows[0].Maximum = static_cast<float>(Limit > 0.0 ? Limit : 1.0);

    PopupDeclaration Declared = {};
    Declared.Title    = Title;
    Declared.Glyph    = Glyph;
    Declared.Rows     = Rows;
    Declared.RowCount = 1u;

    bool ReadoutTaken = false;
    const Deliver<PopupVerdict> Verdict = Readout.Record(Bounds, Declared, ReadoutTaken);
    if (ReadoutTaken)
        PointerTaken = true;
    if (!Verdict.Resolved)
        return;

    // 📝 The figure the artist may have typed goes back through the session's clamp before it is used,
    //    so the drag and the readout cannot disagree about what the largest legal value is.
    if (CornerMannerFor(ActiveTool, Manner))
    {
        DeclareCornerRadius(State.Corner, static_cast<double>(State.Figure));
        if (Verdict.Delivered == PopupVerdict::Applied)
        {
            WorldCurveName Produced = {};
            static_cast<void>(ApplyCornerDragSession(World, State.Corner, Produced));
        }
        else if (Verdict.Delivered == PopupVerdict::Cancelled)
        {
            CancelCornerDragSession(State.Corner);
        }
    }
    else
    {
        DeclareOperationDistance(State.Operation, static_cast<double>(State.Figure));
        if (Verdict.Delivered == PopupVerdict::Applied)
        {
            std::vector<WorldCurveName> Produced;
            static_cast<void>(PerformSketchOperation(World, State.Chain, Workplane,
                                                     State.Operation, Produced));
        }
        else if (Verdict.Delivered == PopupVerdict::Cancelled)
        {
            CancelSketchOperationSession(State.Operation);
        }
    }
}

} // namespace Slate
