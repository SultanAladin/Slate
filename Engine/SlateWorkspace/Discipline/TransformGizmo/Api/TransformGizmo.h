//============================================================================================================================================
//                                                         TRANSFORMGIZMO.H
//============================================================================================================================================
// 🧩 The handles the artist grabs to move, turn and resize a selection.
//
// 🔴 A GIZMO IS A SCREEN-SPACE OBJECT. It is a constant size in pixels however far the camera stands off,
//    because it is a control, not part of the drawing. An arrow the artist can no longer hit after zooming
//    out is a broken control; an arrow that swallows the whole viewport after zooming in is worse.
//
// ⚠️ THE HOST HAD THE HIT TEST AND THE DRAWING MEASURED IN DIFFERENT UNITS. `ResolveGizmoHandle` tested a
//    44-PIXEL axis while `RecordViewportGizmo` drew a 78-WORLD-UNIT shaft with a cone out at 102 and a
//    scale box at 94, then projected them. Those agree at exactly one zoom level and nowhere else:
//
//        OrthoScale   drawn shaft      hit-test reach
//        0.5           39 px            44 px          (near enough)
//        1.0           78 px            44 px
//        4.0          312 px            44 px          the arrow is seven times longer than its hit box
//        32.0        2496 px            44 px          the arrow crosses the display; only its root grabs
//
//    Zoomed in, the artist grabs empty space near the pivot and gets the arrow; zoomed out, the arrow is
//    smaller than its own hit box and clicking beside it still grabs. The measurements below are the ONE
//    table both halves read, in pixels, so the two cannot drift apart again.
//
// 📝 The drawing converts each pixel measurement to world at the pivot through `WorldPerPixel`, so the
//    projected result comes back the size the table asked for.

#pragma once

#include "SlateWorkspace/Discipline/TransformSession/Api/TransformSession.h"

namespace Slate
{

/// 🧩 Which handle the pointer is over.
enum class GizmoHandle : std::uint32_t
{
    None      = 0u,
    MoveFree  = 1u,
    MoveX     = 2u,
    MoveZ     = 3u,
    Rotate    = 4u,
    ScaleFree = 5u,
    ScaleX    = 6u,
    ScaleZ    = 7u
};

/// 🧩 The one table of measurements, in screen pixels.
///
/// 🔴 Both the hit test and the drawing read THESE and nothing else. A measurement that appears in only
///    one of the two is the defect this unit was written to remove.
///
/// 📝 The values below mirror the shipped HTML reference: 82 px axis reach, 18 px move cones,
///    22 px plane quads, 96 px rotation ring and 11 px scale boxes. The hit test reads the same table,
///    in the same order, so the handle that is drawn is the handle that answers.
struct GizmoMeasure
{
    /// The shared axis footprint from `References/Cad/js/gizmo.js`: an 82 px axis with an 18 px cone.
    static constexpr double ShaftStart    = 10.0;
    static constexpr double AxisEnd       = 82.0;
    static constexpr double ArrowBase     = 64.0;
    static constexpr double ArrowRadius   = 6.0;
    static constexpr double ShaftRadius   = 2.1;
    static constexpr double ShaftGrab     = 11.0;

    /// The free-move plane quad, centred 39 px out with a 22 px edge.
    static constexpr double PlaneOffset   = 39.0;
    static constexpr double PlaneHalf     = 11.0;

    /// The centre handle, and the rotation ring just outside the axis tips.
    static constexpr double CentreGrab    = 8.0;
    static constexpr double RingRadius    = 96.0;
    static constexpr double RingGrab      = 8.0;

    /// The scale box at the shared axis tip.
    static constexpr double ScaleBox      = 82.0;
    static constexpr double ScaleBoxHalf  = 5.5;
    static constexpr double ScaleGrab     = 11.0;
};

/// 🧩 Where the gizmo sits on screen and which way its axes run from there.
struct GizmoScreenBasis
{
    float PivotX = 0.0f;
    float PivotY = 0.0f;
    float AlongX = 1.0f;
    float AlongY = 0.0f;
    float AcrossX = 0.0f;
    float AcrossY = -1.0f;

    /// 🔴 How many world units one screen pixel covers AT THE PIVOT. This is what lets the drawing express
    ///    the pixel table above in the world units it has to build geometry from.
    /// note ⚠️ Measured at the pivot, not globally. Under perspective the conversion differs across the
    ///       display, and a gizmo forty pixels wide at the centre would be sixty at the edge if a single
    ///       factor were used everywhere.
    double WorldPerPixel = 1.0;
};

/// 🧩 Reads where the gizmo stands from the camera and the pivot.
/// out   Named  [-]  false when the pivot is behind the camera and there is nothing to draw or grab
bool ResolveGizmoScreenBasis(const SpatialBasis& Basis,
                             const ViewportStanding& View,
                             bool Perspective,
                             const PlaneExtent& Extent,
                             const SpatialPoint& Pivot,
                             GizmoScreenBasis& Resolved);

/// 🧩 Which handle a pointer position is over.
/// note 📝 The hit test follows the reference order: centre, then the plane quad, then the axis, then
///       the rotation ring. The handles now deliberately share the same 82 px footprint the HTML proof
///       uses, so priority is part of the design rather than an accident of spacing.
GizmoHandle ResolveGizmoHandle(const GizmoScreenBasis& Screen,
                               TransformManner Manner,
                               float PointerX,
                               float PointerY);

/// 🧩 A pixel measurement expressed in world units at the pivot, for building the drawn geometry.
/// note 📝 One line, but it is the line that keeps the two halves honest. Everything the gizmo draws goes
///       through it, so the drawn size is the table's size by construction rather than by coincidence.
inline double GizmoWorld(const GizmoScreenBasis& Screen, double Pixels)
{
    return Pixels * Screen.WorldPerPixel;
}

/// 🧩 Which manner a handle belongs to, and what it restricts the drag to.
/// note ⚠️ A handle names both at once — grabbing the X arrow is "move, along X" in one gesture — so the
///       two are read from it together rather than inferred separately at each call site.
TransformManner      ResolveHandleManner(GizmoHandle Handle);
TransformRestriction ResolveHandleRestriction(GizmoHandle Handle);

}   // namespace Slate
