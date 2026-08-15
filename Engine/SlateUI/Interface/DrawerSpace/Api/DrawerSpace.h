//============================================================================================================================================
//                                                              DRAWERSPACE.H
//============================================================================================================================================
// 🧩 Two drawers over one display extent — their drag, their snap arbitration, their tongues and their interiors.

#pragma once

#include "Contract/OutcomeContract.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"
#include "SlateUI/Interface/MotionIntegrator/Api/MotionIntegrator.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    BEARING AND POSE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Which edge a drawer is anchored to and travels from.
/// tag   contract
enum class DrawerBearing : std::uint32_t
{
    North        = 0u,   // [-] - enters from the upper edge; the Control Center
    South        = 1u,   // [-] - enters from the lower edge; the Asset Browser
    BearingCount = 2u    // [-] - the closed count, never a bearing
};

/// 🧩 Where a drawer rests when nothing is dragging it.
/// note  🔴 The north drawer takes `Closed` and `Open` only — it has no intermediate pose, and the six-row
///       arbitration recorded for the south drawer does not apply to it. The south drawer takes all three,
///       and its `Open` is the source's "full": the drawer covering the whole display extent.
/// tag   contract
enum class DrawerPose : std::uint32_t
{
    Closed    = 0u,   // [-] - wholly outside the display extent
    Half      = 1u,   // [-] - half the extent; south drawer only
    Open      = 2u,   // [-] - covering the whole extent
    PoseCount = 3u    // [-] - the closed count, never a pose
};

/// 🧩 What one drawer is declared with at bring-up.
/// note  Static text and one figure. Nothing here is ever allocated and nothing is re-read after Construct.
/// tag   contract, nonallocating, nonthrowing
struct DrawerDeclaration
{
    const char*    Caption       = "";                          // [-] - the tongue's run, in small capitals
    SymbolSubject  TongueSubject = SymbolSubject::FolderClosed;  // [-] - the figure left of the caption
    std::uint32_t  PoseCount     = 2u;                           // [-] - two or three; anything else is two
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE ARRANGEMENT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The two drawers, the pointer arbitration between them, and the extents their panels record inside.
/// note  🔴 📐 The snap arbitration is evaluated on the **drag displacement since contact arrived**, never on
///       the drawer's absolute ordinate. The source states its conditions against Framer's `info.offset.y`,
///       which is a displacement; against an absolute ordinate the closed drawer's `y > h/4` can never hold
///       and the drawer never opens by drag at all.
/// note  🔴 The whole drawer body initiates an across drag, minus whatever a panel has excluded. That is the
///       source's arrangement: the drag is declared on the drawer and the two scroll extents stop
///       propagation at capture. Without `Exclude`, scrolling a library rail drags the drawer out from under
///       the artist's thumb.
/// note  ⚠️ This component raises no mark of its own. `WorkspaceSequence` reads `Moving` and marks, because
///       a drawer that marked directly would need the scheduler and the scheduler's first consumer would
///       then be the thing it schedules.
/// tag   owning
class DrawerSpace
{
public:

    static constexpr std::uint32_t ExclusionCapacity = 8u;   // [-] - extents one drawer may withhold from drag

    DrawerSpace()                              = default;
    DrawerSpace(const DrawerSpace&)            = delete;
    DrawerSpace& operator=(const DrawerSpace&) = delete;
    ~DrawerSpace()                             = default;

    /// 🧩 Enrols four springs and seats both drawers closed at the arrived display extent.
    /// in    Motion      [-]  the one integrator; borrowed and outlives this component
    /// in    Appearance  [-]  already resolved against the display scale; borrowed and outlives this
    /// in    North       [-]  what the upper drawer's tongue carries
    /// in    South       [-]  what the lower drawer's tongue carries
    /// in    Arrived     [-]  the display extent this tick reported
    /// out   Outcome     [-]  refuses with ContentUnsupported for a display extent at or below zero, and
    ///                        with ExtentExhausted when the integrator declines a spring
    /// post  both drawers stand Closed and settled; nothing moves until a pointer arrives
    /// note  🔴 Refused in full. Two of four springs enrolled leaves ordinals pointing at slot zero, and a
    ///        drag of the south drawer would then move the north one.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    Outcome<bool> Construct(MotionIntegrator&              Motion,
                            const AppearanceSpecification& Appearance,
                            const DrawerDeclaration&       North,
                            const DrawerDeclaration&       South,
                            const DisplayCondition&        Arrived);

    /// 🧩 Re-solves both drawers against a new display extent, holding each pose.
    /// note  🔴 The seated ordinate is re-derived from the pose rather than carried across, because every
    ///        ordinate is a fraction of the extent that just changed. Carrying it leaves a half-open drawer
    ///        at the previous extent's half, which after a resize is not half of anything.
    /// note  Every exclusion is released. A panel that re-solves its own extents re-declares them.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Rearrange(const DisplayCondition& Arrived);

    /// 🧩 Advances one tick of pointer arbitration — grab, drag, release, snap, tongue travel.
    /// in    Arrived   [-]   what `RecordingSurface::Pointer` sampled this tick
    /// in    Elapsed   [ms]  what the same tick's display condition measured
    /// out   Taken     [-]   true when the drawer chrome consumed the pointer; the interior must then ignore it
    /// note  📐 The release rate is smoothed across ticks rather than taken from the last one. A single
    ///        stalled tick produces an instantaneous rate of several thousand pixels a second from a
    ///        stationary thumb, and the drawer flies to a pose the artist did not ask for.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    bool Advance(const PointerCondition& Arrived, double Elapsed);

    /// 🧩 Records both drawers — shadows, bodies, edges, grips, tongues and tongue runs.
    /// note  🔴 The south drawer is recorded last while it stands Open, and first otherwise. The source
    ///        raises it above the north drawer at full extent only; a fixed order shows the north drawer's
    ///        edge drawn across a drawer that covers it.
    /// note  Records the drawers alone. Whatever a panel records inside an interior is recorded by that
    ///       panel, between this call and the seal.
    /// cost  🚩
    /// tag   api, nonthrowing
    void Record(RecordingSurface& Surface) const;

    /// 🧩 Records one drawer alone, so a caller may interleave a panel between the two.
    /// note  🔴 The south drawer's interior is recorded above its own body and below the north drawer
    ///        whenever the south drawer is not Open. `Record(Surface)` emits both bodies back to back and
    ///        leaves no seam between them, so a caller with interior content sequences the three pieces
    ///        itself through this overload and does not call the other.
    /// cost  🚩
    /// tag   api, nonthrowing
    void Record(RecordingSurface& Surface, DrawerBearing Bearing) const;

    /// 🧩 Withholds one extent inside a drawer from initiating an across drag.
    /// in    Extent  [px] in display ordinates, as the panel recorded it
    /// note  ⚠️ Released by `Rearrange`. A panel that solves its extents once and never re-declares them
    ///        loses its exclusion at the first resize, and its scroll extent starts dragging the drawer.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Exclude(DrawerBearing Bearing, const PlaneExtent& Extent);

    /// 🧩 The pose one drawer is settled at, or heading toward while a spring is live.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    DrawerPose Pose(DrawerBearing Bearing) const;

    /// 🧩 Places one drawer at a pose immediately, discarding any motion — a shortcut, or bring-up.
    /// note  The south drawer refuses `Half` only by taking `Closed` instead when it was declared with two
    ///       poses; the north drawer is declared with two and therefore never rests half open.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Seat(DrawerBearing Bearing, DrawerPose Declared);

    /// 🧩 Starts one drawer travelling toward a pose under its spring — what a tongue tap does.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Depart(DrawerBearing Bearing, DrawerPose Declared);

    /// 🧩 The whole extent one drawer's body occupies, including the region its grip sits in.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    PlaneExtent Body(DrawerBearing Bearing) const;

    /// 🧩 The extent a panel records inside — the body less the grip strip at its travelling edge.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    PlaneExtent Interior(DrawerBearing Bearing) const;

    /// 🧩 The tongue's extent, for a caller that wants to place something against it.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    PlaneExtent Tongue(DrawerBearing Bearing) const;

    /// 🧩 Whether one drawer's body lies wholly outside the display extent.
    /// out   Withdrawn  [-]  true when a panel inside it need not be recorded at all
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    bool Withdrawn(DrawerBearing Bearing) const;

    /// 🧩 Whether either drawer is being dragged or is still travelling under its spring.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    bool Moving() const;

private:

    /// 🧩 One drawer's own condition. Its ordinate lives in the integrator; everything else lives here.
    /// tag   nonallocating, nonthrowing
    struct DrawerSlot
    {
        DrawerDeclaration  Declared        = {};                     // [-]  - as supplied, never re-read
        std::uint32_t      AcrossSpring    = 0u;                     // [-]  - ordinal into the integrator
        std::uint32_t      TongueSpring    = 0u;                     // [-]  - the tongue's elastic release
        DrawerPose         Standing        = DrawerPose::Closed;     // [-]  - settled, or being travelled to
        float              TongueTravel    = 0.0f;                   // [px] - signed, from the along centre
        float              TongueSeated    = 0.0f;                   // [px] - where the tongue was at grab
        double             SeatedOrdinate  = 0.0;                    // [px] - the across ordinate at grab
        double             TravelAcross    = 0.0;                    // [px] - displacement since contact
        double             TravelAlong     = 0.0;                    // [px] - displacement since contact
        double             ReleaseRate     = 0.0;                    // [px/s] - smoothed, signed
        bool               BodyDragLive    = false;                  // [-]  - the body is being dragged across
        bool               TongueDragLive  = false;                  // [-]  - the tongue is being dragged along
        PlaneExtent        Excluded[ExclusionCapacity] = {};         // [px] - extents that do not start a drag
        std::uint32_t      ExcludedCount   = 0u;                     // [-]
    };

    /// 🧩 Which slot one bearing names.
    const DrawerSlot& Slot(DrawerBearing Bearing) const;
    DrawerSlot&       Slot(DrawerBearing Bearing);

    /// 🧩 The across ordinate one pose rests at, at the standing extent.
    double PoseOrdinate(DrawerBearing Bearing, DrawerPose Declared) const;

    /// 🧩 The across ordinate one drawer stands at now, read from its spring.
    double StandingOrdinate(DrawerBearing Bearing) const;

    /// 🧩 The pose a release resolves to, from the drawer's own arbitration.
    DrawerPose Classify(DrawerBearing Bearing) const;

    /// 🧩 Records one drawer in full.
    void RecordOne(RecordingSurface& Surface, DrawerBearing Bearing) const;

    MotionIntegrator*              Motion       = nullptr;   // [-]  - borrowed; never owned
    const AppearanceSpecification* Appearance   = nullptr;   // [-]  - borrowed; never owned
    DrawerSlot                     Slots[2]     = {};        // [-]  - north, then south
    float                          ExtentAlong  = 0.0f;      // [px] - the display's drawable extent
    float                          ExtentAcross = 0.0f;      // [px]
    DrawerBearing                  GrabbedBy    = DrawerBearing::BearingCount;   // [-] - which drawer holds
                                                                                 //       the pointer, if any
};

}   // namespace Slate
