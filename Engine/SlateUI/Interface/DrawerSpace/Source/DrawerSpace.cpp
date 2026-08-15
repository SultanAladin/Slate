//============================================================================================================================================
//                                                             DRAWERSPACE.CPP
//============================================================================================================================================
// 🧩 Elastic constraint, smoothed release rate, the two arbitrations, and the clipped tongue.

#include "SlateUI/Interface/DrawerSpace/Api/DrawerSpace.h"

#include <cmath>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE FIGURES
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📐 The rate estimator's retention. At a 16 ms tick this averages a release over roughly the last five
//    ticks — long enough that one stalled tick cannot decide a pose, short enough that a flick still reads
//    as a flick rather than as the mean of the whole drag.
constexpr double RateRetention = 0.60;   // [-] - carried from the previous tick's estimate

/// 🧩 Admits travel beyond a constraint at the declared elasticity.
/// cost  ✔️
double Constrain(double Ordinate, double Least, double Most, double Elasticity)
{
    if (Ordinate < Least)
        return Least - (Least - Ordinate) * Elasticity;

    if (Ordinate > Most)
        return Most + (Ordinate - Most) * Elasticity;

    return Ordinate;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> DrawerSpace::Construct(MotionIntegrator&              Integrator,
                                     const AppearanceSpecification& Resolved,
                                     const DrawerDeclaration&       North,
                                     const DrawerDeclaration&       South,
                                     const DisplayCondition&        Arrived)
{
    if (Arrived.ExtentAlong <= 0.0f || Arrived.ExtentAcross <= 0.0f)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the display extent is not positive" });

    Motion       = &Integrator;
    Appearance   = &Resolved;
    ExtentAlong  = Arrived.ExtentAlong;
    ExtentAcross = Arrived.ExtentAcross;

    Slots[0]          = {};
    Slots[1]          = {};
    Slots[0].Declared = North;
    Slots[1].Declared = South;

    // 📝 🔴 All four enrolments are attempted before any ordinal is retained. An integrator that declines
    //    the third delivers slot zero for it, and the south tongue would then drive the north drawer's
    //    across ordinate — a defect with no operand and no error, visible only as one drawer moving another.
    const Outcome<std::uint32_t> NorthAcross = Integrator.EnrolSpring(Resolved.Motion, 0.0);
    const Outcome<std::uint32_t> NorthTongue = Integrator.EnrolSpring(Resolved.Motion, 0.0);
    const Outcome<std::uint32_t> SouthAcross = Integrator.EnrolSpring(Resolved.Motion, 0.0);
    const Outcome<std::uint32_t> SouthTongue = Integrator.EnrolSpring(Resolved.Motion, 0.0);

    if (!NorthAcross.ContentPresent || !NorthTongue.ContentPresent ||
        !SouthAcross.ContentPresent || !SouthTongue.ContentPresent)
    {
        Motion     = nullptr;
        Appearance = nullptr;
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "the integrator declined a drawer spring" });
    }

    Slots[0].AcrossSpring = NorthAcross.Resolve();
    Slots[0].TongueSpring = NorthTongue.Resolve();
    Slots[1].AcrossSpring = SouthAcross.Resolve();
    Slots[1].TongueSpring = SouthTongue.Resolve();

    Seat(DrawerBearing::North, DrawerPose::Closed);
    Seat(DrawerBearing::South, DrawerPose::Closed);

    return Outcome<bool>::Deliver(true);
}

void DrawerSpace::Rearrange(const DisplayCondition& Arrived)
{
    if (Motion == nullptr || Arrived.ExtentAlong <= 0.0f || Arrived.ExtentAcross <= 0.0f)
        return;

    ExtentAlong  = Arrived.ExtentAlong;
    ExtentAcross = Arrived.ExtentAcross;

    for (std::uint32_t SlotOrdinal = 0u; SlotOrdinal < 2u; ++SlotOrdinal)
    {
        DrawerSlot&         Standing = Slots[SlotOrdinal];
        const DrawerBearing Bearing  = static_cast<DrawerBearing>(SlotOrdinal);

        Standing.ExcludedCount  = 0u;
        Standing.BodyDragLive   = false;
        Standing.TongueDragLive = false;

        Motion->Spring(Standing.AcrossSpring).Seat(PoseOrdinate(Bearing, Standing.Standing));

        // 📝 The tongue's travel is clamped rather than re-derived. It is the artist's own placement along
        //    the edge and carries no fraction of the extent, so a narrower display moves it only as far as
        //    the narrower display requires.
        const float TravelCeiling = (ExtentAlong - Appearance->Measure.TongueAlong) * 0.5f;
        const float Admissible    = (TravelCeiling > 0.0f) ? TravelCeiling : 0.0f;

        if (Standing.TongueTravel >  Admissible) Standing.TongueTravel =  Admissible;
        if (Standing.TongueTravel < -Admissible) Standing.TongueTravel = -Admissible;

        Motion->Spring(Standing.TongueSpring).Seat(static_cast<double>(Standing.TongueTravel));
    }

    GrabbedBy = DrawerBearing::BearingCount;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    POSES AND ORDINATES
//------------------------------------------------------------------------------------------------------------------------

const DrawerSpace::DrawerSlot& DrawerSpace::Slot(DrawerBearing Bearing) const
{
    return Slots[(Bearing == DrawerBearing::South) ? 1u : 0u];
}

DrawerSpace::DrawerSlot& DrawerSpace::Slot(DrawerBearing Bearing)
{
    return Slots[(Bearing == DrawerBearing::South) ? 1u : 0u];
}

double DrawerSpace::PoseOrdinate(DrawerBearing Bearing, DrawerPose Declared) const
{
    const double Extent = static_cast<double>(ExtentAcross);

    if (Bearing == DrawerBearing::North)
        return (Declared == DrawerPose::Open) ? 0.0 : -Extent;

    switch (Declared)
    {
        case DrawerPose::Open: return 0.0;
        case DrawerPose::Half: return Extent * 0.5;
        default:               return Extent;
    }
}

double DrawerSpace::StandingOrdinate(DrawerBearing Bearing) const
{
    if (Motion == nullptr)
        return PoseOrdinate(Bearing, DrawerPose::Closed);

    return Motion->Spring(Slot(Bearing).AcrossSpring).Standing;
}

DrawerPose DrawerSpace::Pose(DrawerBearing Bearing) const
{
    return Slot(Bearing).Standing;
}

void DrawerSpace::Seat(DrawerBearing Bearing, DrawerPose Declared)
{
    if (Motion == nullptr)
        return;

    DrawerSlot& Standing = Slot(Bearing);

    if (Standing.Declared.PoseCount < 3u && Declared == DrawerPose::Half)
        Declared = DrawerPose::Closed;

    Standing.Standing = Declared;
    Motion->Spring(Standing.AcrossSpring).Seat(PoseOrdinate(Bearing, Declared));
}

void DrawerSpace::Depart(DrawerBearing Bearing, DrawerPose Declared)
{
    if (Motion == nullptr)
        return;

    DrawerSlot& Standing = Slot(Bearing);

    if (Standing.Declared.PoseCount < 3u && Declared == DrawerPose::Half)
        Declared = DrawerPose::Closed;

    Standing.Standing = Declared;

    SpringInterpolant& Travelling = Motion->Spring(Standing.AcrossSpring);
    Travelling.Target  = PoseOrdinate(Bearing, Declared);
    Travelling.Settled = false;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE SNAP ARBITRATION
//------------------------------------------------------------------------------------------------------------------------

// 📐 🔴 Transcribed literally from the source's two release handlers. Two operands, and they are not the
//    same operand in the two drawers. The north drawer compares the release's own displacement against a
//    quarter of the extent. The south drawer compares `h + offset.y` — the extent **plus** the displacement —
//    against fractions of the extent, which is the quantity its handler names `Be`. Substituting the
//    displacement alone there inverts every one of its six conditions, and the drawer snaps to the pose
//    opposite the one the artist flicked toward.
// 📝 The nesting is the source's too. `closed` and `full` each gate an inner pair behind an outer condition,
//    so a release that clears the outer gate always leaves the pose it was in; `half` is a flat three-way.
//    Flattening the two nested rows lets a slow drag from closed reach `full` without ever passing `half`.
DrawerPose DrawerSpace::Classify(DrawerBearing Bearing) const
{
    const DrawerSlot&  Standing = Slot(Bearing);
    const MotionScale& Figures  = Appearance->Motion;

    const double Extent       = static_cast<double>(ExtentAcross);
    const double Displacement = Standing.TravelAcross;
    const double Rate         = Standing.ReleaseRate;
    const double Near         = Extent * Figures.SnapFractionNear;
    const double Far          = Extent * Figures.SnapFractionFar;

    if (Bearing == DrawerBearing::North)
    {
        if (Standing.Standing == DrawerPose::Open)
            return (Displacement < -Near || Rate < -Figures.SnapRateSoft) ? DrawerPose::Closed : DrawerPose::Open;

        return (Displacement > Near || Rate > Figures.SnapRateSoft) ? DrawerPose::Open : DrawerPose::Closed;
    }

    // 📐 The source's `Be`. The south drawer rests at `h` when closed and at zero when full, so this is the
    //    ordinate the release would have left it at had nothing been constrained — not its constrained one.
    const double Reached = Extent + Displacement;

    if (Standing.Standing == DrawerPose::Closed)
    {
        if (Rate < -Figures.SnapRateFirm || Reached < Far)
            return (Rate < -Figures.SnapRateHard || Reached < Near) ? DrawerPose::Open : DrawerPose::Half;

        return DrawerPose::Closed;
    }

    if (Standing.Standing == DrawerPose::Half)
    {
        if (Rate < -Figures.SnapRateSoft || Reached < Near) return DrawerPose::Open;
        if (Rate >  Figures.SnapRateSoft || Reached > Far)  return DrawerPose::Closed;
        return DrawerPose::Half;
    }

    if (Rate > Figures.SnapRateFirm || Reached > Near)
        return (Rate > Figures.SnapRateHard || Reached > Far) ? DrawerPose::Closed : DrawerPose::Half;

    return DrawerPose::Open;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE TICK
//------------------------------------------------------------------------------------------------------------------------

bool DrawerSpace::Advance(const PointerCondition& Arrived, double Elapsed)
{
    if (Motion == nullptr || Appearance == nullptr)
        return false;

    const MetricScale& Measure       = Appearance->Measure;
    const MotionScale& Figures       = Appearance->Motion;
    const float        TravelCeiling = (ExtentAlong - Measure.TongueAlong) * 0.5f;
    const float        Admissible    = (TravelCeiling > 0.0f) ? TravelCeiling : 0.0f;

    // ① Nothing is held — decide whether this contact grabs anything at all.
    if (GrabbedBy == DrawerBearing::BearingCount)
    {
        if (!Arrived.ContactArrived)
            return false;

        for (std::uint32_t SlotOrdinal = 0u; SlotOrdinal < 2u; ++SlotOrdinal)
        {
            const DrawerBearing Bearing  = static_cast<DrawerBearing>(SlotOrdinal);
            DrawerSlot&         Standing = Slots[SlotOrdinal];

            const bool OnTongue = Tongue(Bearing).Encloses(Arrived.PositionAlong, Arrived.PositionAcross);
            const bool OnBody   = Body(Bearing).Encloses(Arrived.PositionAlong, Arrived.PositionAcross);

            if (!OnTongue && !OnBody)
                continue;

            if (OnBody && !OnTongue)
            {
                bool Withheld = false;

                for (std::uint32_t Ordinal = 0u; Ordinal < Standing.ExcludedCount; ++Ordinal)
                {
                    if (Standing.Excluded[Ordinal].Encloses(Arrived.PositionAlong, Arrived.PositionAcross))
                    {
                        Withheld = true;
                        break;
                    }
                }

                if (Withheld)
                    return false;
            }

            Standing.BodyDragLive   = !OnTongue;
            Standing.TongueDragLive = OnTongue;
            Standing.SeatedOrdinate = StandingOrdinate(Bearing);
            Standing.TongueSeated   = Standing.TongueTravel;
            Standing.TravelAcross   = 0.0;
            Standing.TravelAlong    = 0.0;
            Standing.ReleaseRate    = 0.0;

            GrabbedBy = Bearing;
            return true;
        }

        return false;
    }

    // ② Something is held — carry the travel, constrain it, and estimate the rate.
    DrawerSlot& Standing = Slot(GrabbedBy);

    Standing.TravelAcross += static_cast<double>(Arrived.TravelAcross);
    Standing.TravelAlong  += static_cast<double>(Arrived.TravelAlong);

    if (Elapsed > 0.0)
    {
        const double Instant = static_cast<double>(Arrived.TravelAcross) * 1000.0 / Elapsed;
        Standing.ReleaseRate = Standing.ReleaseRate * RateRetention + Instant * (1.0 - RateRetention);
    }

    if (Standing.BodyDragLive)
    {
        const double Least   = (GrabbedBy == DrawerBearing::North) ? -static_cast<double>(ExtentAcross) : 0.0;
        const double Most    = (GrabbedBy == DrawerBearing::North) ? 0.0 : static_cast<double>(ExtentAcross);
        const double Dragged = Standing.SeatedOrdinate + Standing.TravelAcross;

        Motion->Spring(Standing.AcrossSpring).Seat(Constrain(Dragged, Least, Most, Figures.DragElasticity));
    }
    else if (Standing.TongueDragLive)
    {
        const double Dragged = static_cast<double>(Standing.TongueSeated) + Standing.TravelAlong;

        Standing.TongueTravel = static_cast<float>(
            Constrain(Dragged, -static_cast<double>(Admissible), Admissible, Figures.DragElasticity));

        Motion->Spring(Standing.TongueSpring).Seat(static_cast<double>(Standing.TongueTravel));
    }

    if (!Arrived.ContactReleased)
        return true;

    // ③ Released — a drag arbitrates, and the tongue does nothing but settle back inside its constraint.
    // 📝 🔴 The source's tongue declares `drag="x"`, `dragConstraints`, `dragMomentum:false` and
    //    `dragElastic:.05`, and declares **no** press handler of any description. A tap on it is a drag of
    //    zero travel and resolves to the same elastic settle as any other release.
    if (Standing.TongueDragLive)
    {
        SpringInterpolant& Travelling = Motion->Spring(Standing.TongueSpring);

        const float Settled = (Standing.TongueTravel >  Admissible) ?  Admissible
                            : (Standing.TongueTravel < -Admissible) ? -Admissible
                                                                    :  Standing.TongueTravel;

        Standing.TongueTravel = Settled;
        Travelling.Target     = static_cast<double>(Settled);
        Travelling.Settled    = (std::fabs(Travelling.Standing - Travelling.Target) < 0.1);
    }
    else
    {
        Depart(GrabbedBy, Classify(GrabbedBy));

        // 📐 The release's own rate is injected into the spring rather than discarded. Framer carries the
        //    drag's momentum into the transition, and a spring departing from rest arrives visibly later
        //    than the flick that asked for it — read as lag in the drawer rather than as a discarded rate.
        Motion->Spring(Standing.AcrossSpring).Rate = Standing.ReleaseRate / 1000.0;
    }

    Standing.BodyDragLive   = false;
    Standing.TongueDragLive = false;
    Standing.TravelAcross   = 0.0;
    Standing.TravelAlong    = 0.0;
    Standing.ReleaseRate    = 0.0;

    GrabbedBy = DrawerBearing::BearingCount;

    return true;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE EXTENTS
//------------------------------------------------------------------------------------------------------------------------

PlaneExtent DrawerSpace::Body(DrawerBearing Bearing) const
{
    return Spanning(0.0f, static_cast<float>(StandingOrdinate(Bearing)), ExtentAlong, ExtentAcross);
}

PlaneExtent DrawerSpace::Interior(DrawerBearing Bearing) const
{
    PlaneExtent Occupied = Body(Bearing);

    if (Appearance == nullptr)
        return Occupied;

    const float Strip = Appearance->Measure.GripStripAcross;

    if (Bearing == DrawerBearing::North)
        Occupied.MostAcross -= Strip;
    else
        Occupied.LeastAcross += Strip;

    return Occupied;
}

PlaneExtent DrawerSpace::Tongue(DrawerBearing Bearing) const
{
    if (Appearance == nullptr)
        return {};

    const MetricScale& Measure  = Appearance->Measure;
    const PlaneExtent  Occupied = Body(Bearing);
    const float        Centre   = ExtentAlong * 0.5f + Slot(Bearing).TongueTravel;
    const float        Leading  = Centre - Measure.TongueAlong * 0.5f;

    if (Bearing == DrawerBearing::North)
        return Spanning(Leading, Occupied.MostAcross, Measure.TongueAlong, Measure.TongueAcross);

    return Spanning(Leading, Occupied.LeastAcross - Measure.TongueAcross,
                    Measure.TongueAlong, Measure.TongueAcross);
}

bool DrawerSpace::Withdrawn(DrawerBearing Bearing) const
{
    const PlaneExtent Occupied = Body(Bearing);

    return Occupied.MostAcross <= 0.0f || Occupied.LeastAcross >= ExtentAcross;
}

bool DrawerSpace::Moving() const
{
    if (Motion == nullptr)
        return false;

    if (GrabbedBy != DrawerBearing::BearingCount)
        return true;

    return !Motion->Spring(Slots[0].AcrossSpring).Settled
        || !Motion->Spring(Slots[1].AcrossSpring).Settled
        || !Motion->Spring(Slots[0].TongueSpring).Settled
        || !Motion->Spring(Slots[1].TongueSpring).Settled;
}

void DrawerSpace::Exclude(DrawerBearing Bearing, const PlaneExtent& Extent)
{
    DrawerSlot& Standing = Slot(Bearing);

    if (Standing.ExcludedCount >= ExclusionCapacity)
        return;

    Standing.Excluded[Standing.ExcludedCount++] = Extent;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE RECORDING
//------------------------------------------------------------------------------------------------------------------------

void DrawerSpace::Record(RecordingSurface& Surface) const
{
    if (Appearance == nullptr)
        return;

    if (Slots[1].Standing == DrawerPose::Open)
    {
        RecordOne(Surface, DrawerBearing::North);
        RecordOne(Surface, DrawerBearing::South);
        return;
    }

    RecordOne(Surface, DrawerBearing::South);
    RecordOne(Surface, DrawerBearing::North);
}

void DrawerSpace::Record(RecordingSurface& Surface, DrawerBearing Bearing) const
{
    if (Appearance == nullptr)
        return;

    RecordOne(Surface, Bearing);
}

void DrawerSpace::RecordOne(RecordingSurface& Surface, DrawerBearing Bearing) const
{
    if (Withdrawn(Bearing))
        return;

    const SurfaceInk&  Ink      = Appearance->Ink;
    const MetricScale& Measure  = Appearance->Measure;
    const DrawerSlot&  Standing = Slot(Bearing);
    const PlaneExtent  Occupied = Body(Bearing);
    const PlaneExtent  Tab      = Tongue(Bearing);
    const bool         Northern = (Bearing == DrawerBearing::North);

    // ① The body's cast — 0 ±20px 60px at half coverage, travelling away from the anchored edge.
    Surface.ShadowCast(Occupied, Ink.DrawerShadow, 60.0f * Measure.DisplayScale,
                       0.0f, (Northern ? 20.0f : -20.0f) * Measure.DisplayScale, 0.0f);

    Surface.Ground(Occupied, Ink.SurfaceStanding, 0.0f, CornerNone);

    // ② The one edge the source declares — a rule on the travelling side only.
    const float EdgeAcross = Northern ? Occupied.MostAcross : Occupied.LeastAcross;

    Surface.Ground(PlaneExtent{ Occupied.LeastAlong, EdgeAcross - (Northern ? 1.0f : 0.0f),
                                Occupied.MostAlong,  EdgeAcross + (Northern ? 0.0f : 1.0f) },
                   Ink.EdgeQuiet, 0.0f, CornerNone);

    // ③ The grip pill, centred along, lifted from the travelling edge.
    const float PillAlong  = ExtentAlong * 0.5f - Measure.GripAlong * 0.5f;
    const float PillAcross = Northern
                           ? Occupied.MostAcross  - Measure.GripLiftNorth - Measure.GripAcross
                           : Occupied.LeastAcross + (Measure.GripStripAcross - Measure.GripAcross) * 0.5f;

    Surface.Ground(Spanning(PillAlong, PillAcross, Measure.GripAlong, Measure.GripAcross),
                   Ink.GripPill, Measure.GripAcross * 0.5f, CornerAll);

    // ④ The tongue's own cast, then its clipped outline.
    Surface.ShadowCast(Tab,
                       Northern ? Ink.TongueShadowNorth : Ink.TongueShadowSouth,
                       (Northern ? 3.0f : 10.0f) * Measure.DisplayScale,
                       0.0f,
                       (Northern ? 3.0f : -4.0f) * Measure.DisplayScale, 0.0f);

    // 📐 The source's clip polygon — `0 0, 100% 0, 92% 100%, 8% 100%` on the north tongue and its mirror on
    //    the south. The inset side is always the free end, so both tongues widen toward the drawer they
    //    belong to and the trapezium reads as an extension of the body rather than as a separate tab.
    const float Inset  = Tab.SpanAlong() * Measure.TongueClipFraction;
    const float Least  = Tab.LeastAlong;
    const float Most   = Tab.MostAlong;
    const float Upper  = Tab.LeastAcross;
    const float Lower  = Tab.MostAcross;

    const float NorthOutline[8] = { Least,         Upper, Most,         Upper,
                                    Most - Inset,  Lower, Least + Inset, Lower };
    const float SouthOutline[8] = { Least + Inset, Upper, Most - Inset, Upper,
                                    Most,          Lower, Least,        Lower };

    Surface.Tongue(Northern ? NorthOutline : SouthOutline, 4u, Ink.SurfaceSunken);

    // ⑤ The tongue's figure and its run, measured together and centred inside the pad.
    const char* Caption = Standing.Declared.Caption;
    const float RunSpan = Surface.MeasureRun(Caption, Measure.TextSmall, Measure.TrackingWide);
    const float Content = Measure.SymbolTongue + Measure.TongueGapAlong + RunSpan;
    const float Origin  = (Least + Most) * 0.5f - Content * 0.5f;
    const float Middle  = (Upper + Lower) * 0.5f;

    Surface.Stroke(Standing.Declared.TongueSubject,
                   Spanning(Origin, Middle - Measure.SymbolTongue * 0.5f,
                            Measure.SymbolTongue, Measure.SymbolTongue),
                   Ink.InkPrimary);

    Surface.TextRunCapitalised(Origin + Measure.SymbolTongue + Measure.TongueGapAlong,
                               Middle - Measure.TextSmall * 0.5f,
                               Ink.InkPrimary, Caption, Measure.TextSmall, Measure.TrackingWide, true);
}

}   // namespace Slate
