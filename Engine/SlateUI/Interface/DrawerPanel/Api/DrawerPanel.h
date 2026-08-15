//============================================================================================================================================
//                                                             DRAWERPANEL.H
//============================================================================================================================================
// 🧩 The two edge drawers — a trapezoidal notch dragged out of the display's edge, settling on a damped spring, above every panel.

#pragma once

#include "Contract/PrecisionContract.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/PanelIndex.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE SLIDE SPRING
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One damped harmonic displacement, stepped by semi-implicit Euler until it comes to rest.
/// note  🔴 The reveal offset is a spring and never an interpolation over a fixed duration. A release carries the
///        pointer's own velocity into `Velocity`, so a flick and a slow let-go settle at visibly different rates —
///        an eased duration discards the flick and every release then reads identically.
/// note  ⚠️ `SettledStatus` is what makes the step free once the drawer is at rest, and it is what a drag clears.
///        A spring integrated while the pointer holds it would fight the pointer for the same offset.
/// tag   contract, nonallocating, nonthrowing
struct SlideIntegrator
{
    float  Offset        = 0.0f;     // [px]   - current displacement, the value the drawer's geometry reads
    float  Velocity      = 0.0f;     // [px/s] - current velocity, seeded by the release flick
    float  RestTarget    = 0.0f;     // [px]   - the displacement it settles toward
    float  Stiffness     = 0.0f;     // [-]    - the spring constant, declared from the extents
    float  Damping       = 0.0f;     // [-]    - the viscous coefficient, declared from the extents
    bool   SettledStatus = true;     // [-]    - at rest; false re-arms the step
};

/// 🧩 Arms the spring toward a rest target, seeded with the velocity the release carried.
/// in    Slide            [-]     the spring, amended in place
/// in    RestTarget       [px]    where it settles
/// in    ReleaseVelocity  [px/s]  the pointer's smoothed speed at the release
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
void EnableSlide(SlideIntegrator& Slide, float RestTarget, float ReleaseVelocity);

/// 🧩 Steps one armed spring by one tick and settles it once both thresholds are met.
/// in    Slide         [-]  the spring; a settled one returns immediately
/// in    DeltaSeconds  [s]  the tick's interval, bounded against `SlideMaximumStep`
/// post  a spring within both rest thresholds is snapped onto its target and marked settled
/// note  📐 Hooke's law with viscous damping — 𝑎 = −k·x − c·𝑣 — integrated velocity first. Position-first Euler
///        gains energy at the stiffnesses the drawer uses and the sheet then oscillates instead of arriving.
/// note  ⚠️ The step is bounded rather than trusted. A tick that stalled behind a device reclaim hands in an
///        interval large enough to overshoot the target by more than the displacement it started with.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
void IntegrateSlide(SlideIntegrator& Slide, float DeltaSeconds, const LayoutExtents& Extents);

/// 🧩 Places the spring at one offset outright and marks it settled, which is what a drag does every tick.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
void AlignSlide(SlideIntegrator& Slide, float Offset);

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE EDGE AND THE PIVOT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Which edge of the display the drawer is dragged out of.
/// note  🔴 The two are mirrors of one geometry and one drag, and every asymmetry between them is declared rather
///        than duplicated: the bottom drawer reveals a bounded fraction of the display, the top drawer reveals all
///        of it, and the two wash the desk behind them at different coverages.
/// tag   contract
enum class DrawerEdge : std::uint32_t
{
    Bottom = 0u,   // [-] - slides up out of the bottom edge; the notch's shelf faces up
    Top    = 1u    // [-] - pulls down out of the top edge; the notch's shelf faces down
};

/// 🧩 What the pointer took hold of when it went down, captured once and cleared on release.
/// note  🔴 Classified on the click and never re-read while the hold is open. Re-classifying each tick would move
///        the drawer from grip to body the moment the silhouette slid out from under a cursor that never moved.
/// note  ⚠️ The centred content column is deliberately **not** a pivot. That is what lets a control inside the
///        drawer own its own press — a body that claimed the whole sheet would drag the drawer instead.
/// tag   contract
enum class DrawerPivot : std::uint32_t
{
    Dormant = 0u,   // [-] - nothing is held
    Grip    = 1u,   // [-] - the notch silhouette is held; horizontal drift is permitted
    Body    = 2u    // [-] - the sheet's side margins are held; drift is locked
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE DRAWER SPECIFICATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One drawer — where it is, what it latched, and everything one drag of it carries.
/// note  🔴 Held by value beside the specification whose body it presents. What the artist pulled out is layout and
///        never document, exactly as `14` §4.1 places every other carry, so closing a workspace forgets an open
///        drawer and amends nothing the document holds.
/// note  ⚠️ `OpenEnabled` is the **latched intent** and not the current offset. The release consults it as its own
///        fallback when neither the flick test nor the distance test decides, which is what makes a press that
///        travelled nowhere leave the drawer exactly as it was found.
/// tag   owning
struct DrawerSpecification
{
    SlideIntegrator  Slide        = {};                      // [-]    - the reveal offset, 0 closed → reveal open
    DrawerEdge       Edge         = DrawerEdge::Bottom;      // [-]    - which edge it is dragged out of
    const char*      GripCaption  = nullptr;                 // [-]    - printed on the notch; static storage, never copied

    float            DragVelocity = 0.0f;                    // [px/s] - smoothed pointer speed, read once on release
    float            StartOffset  = 0.0f;                    // [px]   - the offset the hold opened at
    float            Drift        = 0.0f;                    // [px]   - the notch's horizontal travel along its edge
    bool             OpenEnabled  = false;                   // [-]    - the latched open intent
    DrawerPivot      Pivot        = DrawerPivot::Dormant;    // [-]    - what the hold took hold of
    bool             DragActive   = false;                   // [-]    - a drag is live this tick
    bool             ContentHeld  = false;                   // [-]    - a control inside the body owns the pointer
};

/// 🧩 One drawer as a workspace hands it to the tick — the record, and the routine its content column presents.
/// note  🔴 The record is addressed and never owned. It is a member of the workspace's own storage, exactly as every
///        panel context is, so an open drawer survives a tick and is forgotten when the workspace is.
/// note  ⚠️ A declaration naming no record presents nothing at all. That is how a workspace declares one drawer and
///        not the other, and it is not a refusal — a workspace with nothing to browse has no bottom drawer.
/// tag   contract, nonallocating, nonthrowing
struct DrawerDeclaration
{
    DrawerSpecification*  Drawer      = nullptr;   // [-] - the workspace's own record; never owned here
    PanelPresentRoutine   Body        = nullptr;   // [-] - one tick of the content column
    void*                 BodyContext = nullptr;   // [-] - threaded to the routine unread
};

/// 🧩 The two drawers one workspace declares — the bottom edge's and the top edge's.
/// note  🔴 Exactly two, and they are named rather than counted. A drawer is an edge of the display and the display
///        has two horizontal ones; an open-ended run would let a workspace declare a third with no edge to hang it
///        on. A workspace declaring neither is presented with neither, which is not a refusal.
/// note  ⚠️ The declarations are rebuilt at activation, beside the panel contexts and for the same reason — both
///        address storage the workspace owns, and a declaration retained across an activation names a record the
///        departing workspace has already released.
/// tag   contract, nonallocating, nonthrowing
struct DrawerIndex
{
    DrawerDeclaration  BottomDrawer = {};   // [-] - slides up out of the floor
    DrawerDeclaration  TopDrawer    = {};   // [-] - pulls down out of the ceiling
};

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE REVEAL EXTENT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 How far one drawer opens, which is the one asymmetry between the two edges that geometry cannot derive.
/// in    Extents        [-]   read for the bounded fraction and its ceiling
/// in    Edge           [-]   the bottom drawer reveals a fraction; the top drawer reveals the whole display
/// in    DisplayHeight  [px]  the drawable extent
/// out   Reveal         [px]  the offset an opened drawer settles at
/// note  📝 One place, so the release target and the scrim's coverage fraction cannot disagree. Two copies is the
///        defect where a drawer settles at an offset its own wash treats as fully open.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
float ResolveDrawerReveal(const LayoutExtents& Extents, DrawerEdge Edge, float DisplayHeight);

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE LATCHED INTENT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Latches the drawer open and arms the spring toward its reveal.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
void EnableDrawer(DrawerSpecification& Drawer, const LayoutExtents& Extents, float DisplayHeight);

/// 🧩 Latches the drawer closed and arms the spring back to the edge.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
void DisableDrawer(DrawerSpecification& Drawer);

/// 🧩 Reverses the latched intent — what a shortcut or a header glyph drives, never what the notch itself drives.
/// note  📝 A press on the notch that travelled nowhere deliberately does **not** toggle. The release falls back to
///        the latched intent, so an accidental tap on the silhouette leaves the drawer where it was.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
void ToggleDrawer(DrawerSpecification& Drawer, const LayoutExtents& Extents, float DisplayHeight);

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE POINTER CLAIM
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Whether this drawer must take the pointer away from everything the desk paints beneath it.
/// in    Drawer         [-]   read and never amended
/// in    Extents        [-]   read for the notch extents and the clearance below which the sheet is not a surface
/// in    DisplayWidth   [px]  the drawable extent
/// in    DisplayHeight  [px]
/// out   Claiming       [-]   true where a drag is live, a control inside the body holds, or the pointer sits over
///                            the opened sheet or its notch
/// pre   an interface tick is open
/// note  🔴 Consulted **before** the desk paints, and never after. The drawer owns no vendor window, so nothing else
///        stops a panel beneath the cursor resolving the same press — the claim is the whole mechanism by which the
///        drawer sits above every panel rather than merely in front of them.
/// note  ⚠️ A closed drawer claims nothing, its notch included. The notch still classifies its own press when it
///        paints, so the press resolves in both places exactly as it does at the source.
/// cost  ✔️
/// tag   api, nonthrowing
bool DrawerCapturingPointer(const DrawerSpecification& Drawer,
                            const LayoutExtents&       Extents,
                            float                      DisplayWidth,
                            float                      DisplayHeight);

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Presents one tick of one drawer — the drag, the release, the sheet, the body, and the notch over all of it.
/// in    Theme        [-]   read for the palette and every extent; never held
/// in    Drawer       [-]   amended in place: the offset, the drift, the pivot and the latched intent
/// in    DisplayWidth [px]  the drawable extent
/// in    DisplayHeight[px]
/// in    Body         [-]   the routine the content column is handed to; null presents the sheet and nothing in it
/// in    BodyContext  [-]   threaded to `Body` unread
/// out   Consumed     [-]   true where this drawer resolved the pointer this tick
/// post  the spring is stepped, the drag is advanced or released, and every quad is on the foreground recording
/// note  🔴 The notch paints **last**, over the sheet's own edge. Painted before it, the sheet's fill covers the
///        silhouette's concave shoulders and the notch reads as a plain rectangle sitting on a line.
/// note  🔴 The release decides once and never during the drag. A decision taken per tick would snap the sheet open
///        under a pointer that was still travelling, and the artist reads that as the drawer fighting the drag.
/// note  ⚠️ The body routine is handed the framed interior and nothing else. A routine resolving its own rectangle
///        would present at the extent the sheet carried before the drag rather than the one it carries.
/// cost  🔴
/// tag   api, nonthrowing
bool PresentDrawer(const ThemeSpecification&  Theme,
                   DrawerSpecification&       Drawer,
                   float                      DisplayWidth,
                   float                      DisplayHeight,
                   PanelPresentRoutine        Body,
                   void*                      BodyContext);

// 📐 Every offset, velocity, drift and rectangle here is Bounded — each is bounded against the display extent or a
//    declared threshold before it is read. The component claims Bounded, per `00` §3's transitivity rule.
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded, PrecisionGuarantee::Exact);

}   // namespace Slate
