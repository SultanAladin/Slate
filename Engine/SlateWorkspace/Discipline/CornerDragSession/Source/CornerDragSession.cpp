//============================================================================================================================================
//                                                       CORNERDRAGSESSION.CPP
//============================================================================================================================================

#include "SlateWorkspace/Discipline/CornerDragSession/Api/CornerDragSession.h"

#include <algorithm>
#include <cmath>

namespace Slate
{

namespace
{

/// 🧩 The pointer's distance from the corner being worked, which is the radius the artist is asking for.
double AskedRadius(const CornerDragSession& Session, const SpatialPoint& Probe)
{
    return std::sqrt(LengthSquared(Difference(Probe, Session.Target.Position)));
}

/// 🧩 Holds a radius inside what the corner can take, recording whether it had to.
double HoldWithinLimit(double Asked, double Limit, bool& Clamped)
{
    // 📐 A floor as well as a limit. Zero is not a fillet, and a drag that begins exactly on the corner
    //    would otherwise spend its first frames producing refusals rather than a very small fillet.
    Clamped = Asked > Limit;
    return std::clamp(Asked, 0.0, Limit);
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------

void AdvanceCornerDragSession(const WorldSketchStructure& Declared,
                              const CornerPointerFrame& Pointer,
                              CornerDragSession& Session)
{
    // 🔴 `Applied` LASTS ONE FRAME. The caller reads it, and the session then returns to looking for the
    //    next corner. Leaving it standing would make every subsequent frame look like a fresh commit.
    if (Session.Phase == CornerPhase::Applied)
        Session.Phase = CornerPhase::Idle;

    // ① While the popup holds a pending operation, the pointer no longer steers it. The artist is typing.
    if (Session.Phase == CornerPhase::Pending)
        return;

    // ② A drag in progress: the radius follows the pointer, held at the corner's limit.
    if (Session.Phase == CornerPhase::Dragging)
    {
        Session.Radius = HoldWithinLimit(AskedRadius(Session, Pointer.Probe),
                                         Session.Limit, Session.Clamped);

        // 🔴 THE RELEASE DOES NOT APPLY. It hands the figure to the popup, which is what gives the artist
        //    somewhere to type an exact number before anything is written.
        if (Pointer.Released)
            Session.Phase = CornerPhase::Pending;
        return;
    }

    // ③ Otherwise the tool is looking for a corner under the pointer.
    const Deliver<WorldCornerTarget> Found =
        ResolveWorldCornerNear(Declared, Pointer.Probe, CornerProbeReach);

    if (!Found)
    {
        Session.Phase   = CornerPhase::Idle;
        Session.Target  = {};
        Session.Limit   = 0.0;
        Session.Radius  = 0.0;
        Session.Clamped = false;
        return;
    }

    Session.Target = Found.Resolve();
    Session.Limit  = Session.Target.Limit;
    Session.Phase  = CornerPhase::Hovering;

    if (Pointer.Pressed || Pointer.Held)
    {
        Session.Phase  = CornerPhase::Dragging;
        Session.Radius = HoldWithinLimit(AskedRadius(Session, Pointer.Probe),
                                         Session.Limit, Session.Clamped);
    }
}

void DeclareCornerRadius(CornerDragSession& Session, double Radius)
{
    // 📝 Clamped on the way in, so a figure typed past the limit behaves exactly as a drag past it does.
    //    Two different answers for the same number would be indefensible.
    Session.Radius = HoldWithinLimit(Radius, Session.Limit, Session.Clamped);
}

CornerVerdict ApplyCornerDragSession(WorldSketchStructure& Declared,
                                     CornerDragSession& Session,
                                     WorldCurveName& Produced)
{
    Produced = {};
    if (!Session.PopupStanding())
        return CornerVerdict::NoSharedEndpoint;

    const CornerVerdict Verdict = ApplyWorldCorner(Declared, Session.Target.First, Session.Target.Second,
                                                   Session.Radius,
                                                   Session.Manner == CornerManner::Chamfer,
                                                   Produced);

    // 🔴 A REFUSAL LEAVES THE POPUP OPEN. The artist asked for something the corner cannot take; closing
    //    the tool and discarding their figure would make them start the whole gesture again to change it.
    if (Verdict != CornerVerdict::Produced)
        return Verdict;

    Session.Phase = CornerPhase::Applied;
    return Verdict;
}

void CancelCornerDragSession(CornerDragSession& Session)
{
    Session.Phase   = CornerPhase::Idle;
    Session.Target  = {};
    Session.Radius  = 0.0;
    Session.Limit   = 0.0;
    Session.Clamped = false;
}

} // namespace Slate
