//============================================================================================================================================
//                                                            DRAWERPANEL.CPP
//============================================================================================================================================
// 🧩 One silhouette, one drag and one release, mirrored across two edges — the sheet, its wash, and the notch painted over both.

#include "SlateUI/Interface/DrawerPanel/Api/DrawerPanel.h"

#include "imgui.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE SLIDE SPRING
//------------------------------------------------------------------------------------------------------------------------

void EnableSlide(SlideIntegrator& Slide, float RestTarget, float ReleaseVelocity)
{
    Slide.RestTarget    = RestTarget;
    Slide.Velocity      = ReleaseVelocity;
    Slide.SettledStatus = false;
}

void IntegrateSlide(SlideIntegrator& Slide, float DeltaSeconds, const LayoutExtents& Extents)
{
    if (Slide.SettledStatus)
        return;

    // 📝 The two constants are declared into the record the first time it is stepped rather than at construction,
    //    because a default-constructed drawer has no theme in hand and a zero stiffness never leaves the edge.
    if (!(Slide.Stiffness > 0.0f))
        Slide.Stiffness = Extents.SlideStiffness;

    if (!(Slide.Damping > 0.0f))
        Slide.Damping = Extents.SlideDamping;

    const float Step = DeltaSeconds > Extents.SlideMaximumStep ? Extents.SlideMaximumStep : DeltaSeconds;

    // 📐 Hooke's law with viscous damping — 𝑎 = −k·x − c·𝑣 — stepped velocity first. Position-first Euler gains
    //    energy at this stiffness and the sheet then rings around its target instead of arriving at it.
    const float Displacement = Slide.Offset - Slide.RestTarget;
    const float Acceleration = -Slide.Stiffness * Displacement - Slide.Damping * Slide.Velocity;

    Slide.Velocity += Acceleration * Step;
    Slide.Offset   += Slide.Velocity * Step;

    const float Remaining = Slide.Offset - Slide.RestTarget;

    // 📝 🔴 Both thresholds, and not either one. An offset within half a pixel of the target while the velocity is
    //    still large is a spring crossing its target at speed, and settling it there discards the overshoot the
    //    artist can see coming.
    if ((Remaining < Extents.SlideOffsetRest    && Remaining      > -Extents.SlideOffsetRest)
     && (Slide.Velocity < Extents.SlideVelocityRest && Slide.Velocity > -Extents.SlideVelocityRest))
    {
        Slide.Offset        = Slide.RestTarget;
        Slide.Velocity      = 0.0f;
        Slide.SettledStatus = true;
    }
}

void AlignSlide(SlideIntegrator& Slide, float Offset)
{
    Slide.Offset        = Offset;
    Slide.Velocity      = 0.0f;
    Slide.SettledStatus = true;
}

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE LOCAL SHAPES
//------------------------------------------------------------------------------------------------------------------------

// 📝 The silhouette's authored path, in the coordinates it was drawn in. Every extent below is scaled onto whatever
//    the theme declares, so the shape is one authored curve and the theme decides only how large it is drawn.
//    `M100 0 L0 0 C20 0 20 36 40 36 L160 36 C180 36 180 0 200 0 Z` — two concave shoulders into a flat shelf.
constexpr float AuthoredGripWidth  = 200.0f;   // [px] - the path's own horizontal extent
constexpr float AuthoredGripHeight =  36.0f;   // [px] - and its vertical extent

ImU32 Coded(const ThemeColour& Colour)
{
    return static_cast<ImU32>(Quantize(Colour));
}

bool CursorInside(ImVec2 Cursor, ImVec2 Lower, ImVec2 Upper)
{
    return Cursor.x >= Lower.x && Cursor.x <= Upper.x && Cursor.y >= Lower.y && Cursor.y <= Upper.y;
}

float Narrowed(float Reading, float Floor, float Ceiling)
{
    return Reading < Floor ? Floor : (Reading > Ceiling ? Ceiling : Reading);
}

/// 🧩 Traces the notch silhouette into the recording's path, ready to be filled.
/// in    Origin  [px]  the path's own top-left, before the mirror is applied
/// in    Flipped [-]   true mirrors the shape vertically, so the flat shelf faces up out of the bottom edge
/// note  🔴 The apex is prepended before the outline and it is not part of the authored path. `PathFillConvex`
///        fans triangles from the first point, and the silhouette is **concave** at both shoulders — starting the
///        fan at the outline's own first point leaves the two curved wedges unfilled. Starting it at the apex fans
///        every triangle across the concavity instead, which is what makes the shape close.
void TraceGripSilhouette(ImDrawList*          Recording,
                         ImVec2               Origin,
                         float                Width,
                         float                Height,
                         bool                 Flipped,
                         int                  CurveSegments)
{
    const float ScaleX = Width  / AuthoredGripWidth;
    const float ScaleY = Height / AuthoredGripHeight;

    // 📝 The mirror is applied to the authored vertical coordinate and to nothing else, so both edges share one
    //    path and one set of control points. Two hand-mirrored paths is two places every shoulder amendment lands.
    const auto At = [&](float AuthoredX, float AuthoredY)
    {
        const float Vertical = Flipped ? (AuthoredGripHeight - AuthoredY) : AuthoredY;

        return ImVec2(Origin.x + AuthoredX * ScaleX, Origin.y + Vertical * ScaleY);
    };

    Recording->PathClear();
    Recording->PathLineTo(At(100.0f, 0.0f));
    Recording->PathLineTo(At(0.0f, 0.0f));
    Recording->PathBezierCubicCurveTo(At(20.0f, 0.0f), At(20.0f, 36.0f), At(40.0f, 36.0f), CurveSegments);
    Recording->PathLineTo(At(160.0f, 36.0f));
    Recording->PathBezierCubicCurveTo(At(180.0f, 36.0f), At(180.0f, 0.0f), At(200.0f, 0.0f), CurveSegments);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE DRAWER GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every rectangle one drawer occupies at one offset — resolved twice a tick, before and after the drag steps it.
struct DrawerGeometry
{
    float   Offset      = 0.0f;    // [px] - the reveal, bounded to what the edge permits
    float   SheetEdge   = 0.0f;    // [px] - the vertical the sheet's moving edge sits at
    ImVec2  GripLower   = {};      // [px] - the notch silhouette
    ImVec2  GripUpper   = {};      // [px]
    ImVec2  SheetLower  = {};      // [px] - the whole sheet, across the display
    ImVec2  SheetUpper  = {};      // [px]
    ImVec2  ColumnLower = {};      // [px] - the centred content column, which is never a drag surface
    ImVec2  ColumnUpper = {};      // [px]
};

DrawerGeometry ResolveGeometry(const DrawerSpecification&  Drawer,
                               const LayoutExtents&        Extents,
                               float                       DisplayWidth,
                               float                       DisplayHeight,
                               float                       Reveal,
                               float                       CentreX,
                               float                       ColumnX,
                               float                       ColumnWidth)
{
    DrawerGeometry Resolved;

    Resolved.Offset = Narrowed(Drawer.Slide.Offset, 0.0f, Reveal);

    if (Drawer.Edge == DrawerEdge::Bottom)
    {
        // 📐 The bottom sheet grows upward out of the display's floor, so its moving edge is the display height
        //    less the reveal and its notch sits immediately above that edge with its shelf facing up.
        Resolved.SheetEdge  = DisplayHeight - Resolved.Offset;
        Resolved.GripLower  = ImVec2(CentreX - Extents.DrawerGripWidth * 0.5f,
                                     Resolved.SheetEdge - Extents.DrawerGripHeight);
        Resolved.GripUpper  = ImVec2(Resolved.GripLower.x + Extents.DrawerGripWidth, Resolved.SheetEdge);
        Resolved.SheetLower = ImVec2(0.0f, Resolved.SheetEdge);
        Resolved.SheetUpper = ImVec2(DisplayWidth, DisplayHeight);
        Resolved.ColumnLower = ImVec2(ColumnX, Resolved.SheetEdge);
        Resolved.ColumnUpper = ImVec2(ColumnX + ColumnWidth, DisplayHeight);
    }
    else
    {
        // 📐 The top sheet grows downward out of the ceiling, so the reveal **is** its moving edge and the notch
        //    hangs below it with its shelf facing down. The sheet is anchored at zero and never above it.
        Resolved.SheetEdge  = Resolved.Offset;
        Resolved.GripLower  = ImVec2(CentreX - Extents.DrawerGripWidth * 0.5f, Resolved.SheetEdge);
        Resolved.GripUpper  = ImVec2(Resolved.GripLower.x + Extents.DrawerGripWidth,
                                     Resolved.SheetEdge + Extents.DrawerGripHeight);
        Resolved.SheetLower = ImVec2(0.0f, 0.0f);
        Resolved.SheetUpper = ImVec2(DisplayWidth, Resolved.SheetEdge);
        Resolved.ColumnLower = ImVec2(ColumnX, 0.0f);
        Resolved.ColumnUpper = ImVec2(ColumnX + ColumnWidth, Resolved.SheetEdge);
    }

    return Resolved;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE DRAG ITSELF
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One tick of a live drag — the offset follows the pointer directly, the grip drifts, the speed is smoothed.
/// in    TravelX       [px] - the pointer's horizontal delta this tick
/// in    TravelY       [px] - its vertical delta
/// in    DeltaSeconds  [s]  - the tick's interval, which is what turns a delta into a speed
/// note  🔴 The offset is placed with `AlignSlide` and not with a target. While the pointer holds the drawer, the
///        spring is **not** integrating — a spring pulling toward a rest target under a pointer that is dragging it
///        somewhere else is two authorities on one offset, and the sheet lags the cursor by the difference.
/// note  📐 The velocity is an exponentially weighted mean over the instantaneous speed, at the arriving share the
///        extents declare. A raw last-tick speed makes the flick test a coin toss on one frame's jitter; an
///        unweighted mean over the whole drag ignores the flick entirely.
void AdvanceDrawerDrag(DrawerSpecification&  Drawer,
                       const LayoutExtents&  Extents,
                       float                 TravelX,
                       float                 TravelY,
                       float                 DeltaSeconds)
{
    if (Drawer.Pivot == DrawerPivot::Dormant)
    {
        Drawer.DragActive = false;

        return;
    }

    // 📝 🔴 The one sign in the whole component. The bottom drawer's reveal grows as the pointer travels **up**,
    //    so its offset takes the negated delta; the top drawer's grows as the pointer travels down and takes it
    //    directly. Everything downstream — the drift, the velocity, the release test — is then identical.
    const float Directed  = Drawer.Edge == DrawerEdge::Bottom ? -TravelY : TravelY;
    const float NewOffset = Drawer.Slide.Offset + Directed;

    AlignSlide(Drawer.Slide, NewOffset < 0.0f ? 0.0f : NewOffset);

    if (Drawer.Pivot == DrawerPivot::Grip)
        Drawer.Drift += TravelX;

    // 📝 The interval is floored rather than trusted. A tick reporting zero seconds — the first tick after a device
    //    reclaim does — divides the delta by nothing and the flick test then fires on a pointer that never moved.
    const float Step    = DeltaSeconds > 1.0e-4f ? DeltaSeconds : 1.0e-4f;
    const float Instant = Directed / Step;
    const float Arriving = Extents.DrawerVelocityRecent;

    Drawer.DragVelocity = Drawer.DragVelocity * (1.0f - Arriving) + Instant * Arriving;
    Drawer.DragActive   = true;
}

/// 🧩 The release — decided once, against the flick, then the travel, then the intent the drag opened with.
/// note  🔴 Consulted in that order and never re-ordered. A fast flick is what the artist means even where it
///        travelled almost nothing, and a long slow pull is what they mean even where it ended at rest. Testing
///        travel first would make a flick that crossed no threshold read as an abandoned drag.
/// note  ⚠️ The latched intent is the fallback and not a toggle. A press that travelled nowhere leaves the drawer
///        exactly as it was found, which is what makes the notch safe to press by accident.
void SealDrawerDrag(DrawerSpecification& Drawer, const LayoutExtents& Extents, float Reveal)
{
    const float Travelled = Drawer.Slide.Offset - Drawer.StartOffset;

    bool OpenDeclared = Drawer.OpenEnabled;

    if      (Drawer.DragVelocity >  Extents.DrawerFlickVelocity)  OpenDeclared = true;
    else if (Drawer.DragVelocity < -Extents.DrawerFlickVelocity)  OpenDeclared = false;
    else if (Travelled           >  Extents.DrawerFinalizeOffset) OpenDeclared = true;
    else if (Travelled           < -Extents.DrawerFinalizeOffset) OpenDeclared = false;

    Drawer.OpenEnabled = OpenDeclared;
    Drawer.Pivot       = DrawerPivot::Dormant;
    Drawer.DragActive  = false;

    // 📝 The pointer's own smoothed speed is carried into the spring rather than discarded, so a flick keeps
    //    travelling after the artist has let go. Seeding zero here makes every release look identically inert.
    EnableSlide(Drawer.Slide, OpenDeclared ? Reveal : 0.0f, Drawer.DragVelocity);

    Drawer.DragVelocity = 0.0f;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE REVEAL EXTENT
//------------------------------------------------------------------------------------------------------------------------

float ResolveDrawerReveal(const LayoutExtents& Extents, DrawerEdge Edge, float DisplayHeight)
{
    const float Bounded = DisplayHeight > 0.0f ? DisplayHeight : 0.0f;

    // 📝 🔴 The two edges reveal differently and deliberately. The top drawer is a control centre and takes the
    //    whole display, because what it presents is the thing the artist came to it for. The bottom drawer is a
    //    browser opened **beside** the work, so it takes a bounded share and leaves the canvas visible above it —
    //    a browser that covered the surface being painted would make choosing an asset a blind act.
    if (Edge == DrawerEdge::Top)
        return Bounded;

    const float Fractional = Bounded * Extents.DrawerRevealFraction;

    return Fractional < Extents.DrawerRevealCeiling ? Fractional : Extents.DrawerRevealCeiling;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE LATCHED INTENT
//------------------------------------------------------------------------------------------------------------------------

void EnableDrawer(DrawerSpecification& Drawer, const LayoutExtents& Extents, float DisplayHeight)
{
    Drawer.OpenEnabled = true;

    EnableSlide(Drawer.Slide, ResolveDrawerReveal(Extents, Drawer.Edge, DisplayHeight), 0.0f);
}

void DisableDrawer(DrawerSpecification& Drawer)
{
    Drawer.OpenEnabled = false;

    EnableSlide(Drawer.Slide, 0.0f, 0.0f);
}

void ToggleDrawer(DrawerSpecification& Drawer, const LayoutExtents& Extents, float DisplayHeight)
{
    if (Drawer.OpenEnabled)
        DisableDrawer(Drawer);
    else
        EnableDrawer(Drawer, Extents, DisplayHeight);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE POINTER CLAIM
//------------------------------------------------------------------------------------------------------------------------

bool DrawerCapturingPointer(const DrawerSpecification& Drawer,
                            const LayoutExtents&       Extents,
                            float                      DisplayWidth,
                            float                      DisplayHeight)
{
    // 📝 A live drag claims outright, wherever the cursor has travelled to. The drawer keeps following a pointer
    //    that left its own rectangle, so a claim tested against the rectangle alone would surrender the pointer
    //    mid-drag and the desk beneath would begin resolving the same travel.
    if (Drawer.Pivot != DrawerPivot::Dormant || Drawer.DragActive || Drawer.ContentHeld)
        return true;

    if (ImGui::GetCurrentContext() == nullptr)
        return false;

    const float Reveal = ResolveDrawerReveal(Extents, Drawer.Edge, DisplayHeight);
    const float Offset = Narrowed(Drawer.Slide.Offset, 0.0f, Reveal);

    if (Offset <= Extents.DrawerBodyClearance)
        return false;

    const float MaximumDrift = DisplayWidth > Extents.DrawerGripWidth
                             ? (DisplayWidth - Extents.DrawerGripWidth) * 0.5f
                             : 0.0f;

    const float Drift   = Narrowed(Drawer.Drift, -MaximumDrift, MaximumDrift);
    const float CentreX = DisplayWidth * 0.5f + Drift;

    const DrawerGeometry Placed = ResolveGeometry(Drawer, Extents, DisplayWidth, DisplayHeight,
                                                  Reveal, CentreX, 0.0f, 0.0f);

    const ImVec2 Cursor = ImGui::GetIO().MousePos;

    return CursorInside(Cursor, Placed.GripLower, Placed.GripUpper)
        || CursorInside(Cursor, Placed.SheetLower, Placed.SheetUpper);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

bool PresentDrawer(const ThemeSpecification&  Theme,
                   DrawerSpecification&       Drawer,
                   float                      DisplayWidth,
                   float                      DisplayHeight,
                   PanelPresentRoutine        Body,
                   void*                      BodyContext)
{
    if (ImGui::GetCurrentContext() == nullptr || DisplayWidth <= 0.0f || DisplayHeight <= 0.0f)
        return false;

    const LayoutExtents& Extents   = Theme.Extents;
    const ThemePalette&  Palette   = Theme.Palette;
    const ImGuiIO&       Pointing  = ImGui::GetIO();
    ImDrawList*          Recording = ImGui::GetForegroundDrawList();

    const float Reveal = ResolveDrawerReveal(Extents, Drawer.Edge, DisplayHeight);

    //--- ① step the spring, unless the pointer is holding the drawer somewhere else ------------------------------------
    IntegrateSlide(Drawer.Slide, Pointing.DeltaTime, Extents);

    //--- ② place the notch along its edge -----------------------------------------------------------------------------
    // 📐 The drift is bounded so the silhouette cannot leave the display in either direction. The bound is applied
    //    to the carried drift itself and not merely to what is drawn, because an unbounded carry accumulates a
    //    travel the artist must then drag all the way back before the notch begins to move again.
    const float MaximumDrift = DisplayWidth > Extents.DrawerGripWidth
                             ? (DisplayWidth - Extents.DrawerGripWidth) * 0.5f
                             : 0.0f;

    Drawer.Drift = Narrowed(Drawer.Drift, -MaximumDrift, MaximumDrift);

    const float CentreX = DisplayWidth * 0.5f + Drawer.Drift;

    const float ColumnCeiling = DisplayWidth - Extents.DrawerContentInset;
    const float ColumnWidth   = ColumnCeiling < Extents.DrawerContentWidth ? ColumnCeiling : Extents.DrawerContentWidth;
    const float ColumnX       = (DisplayWidth - ColumnWidth) * 0.5f;

    DrawerGeometry Placed = ResolveGeometry(Drawer, Extents, DisplayWidth, DisplayHeight,
                                            Reveal, CentreX, ColumnX, ColumnWidth);

    //--- ③ classify what the press took hold of, once, on the tick it lands --------------------------------------------
    const ImVec2 Cursor = Pointing.MousePos;

    const bool ClearanceAchieved = Placed.Offset > Extents.DrawerBodyClearance;
    const bool OverGrip          = CursorInside(Cursor, Placed.GripLower, Placed.GripUpper);
    const bool OverColumn        = CursorInside(Cursor, Placed.ColumnLower, Placed.ColumnUpper);

    // 📝 🔴 The margins either side of the content column are the body's drag surface and the column itself is not.
    //    That is the whole reason a control inside the drawer can be pressed at all — a body claiming the column
    //    would drag the drawer every time the artist reached for a slider inside it.
    const bool OverMargin = ClearanceAchieved && !OverGrip && !OverColumn
                         && CursorInside(Cursor, Placed.SheetLower, Placed.SheetUpper);

    if (Drawer.Pivot == DrawerPivot::Dormant && !Drawer.ContentHeld
     && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        if      (OverGrip)   Drawer.Pivot = DrawerPivot::Grip;
        else if (OverMargin) Drawer.Pivot = DrawerPivot::Body;

        if (Drawer.Pivot != DrawerPivot::Dormant)
        {
            Drawer.StartOffset     = Placed.Offset;
            Drawer.DragVelocity    = 0.0f;
            Drawer.Slide.Velocity  = 0.0f;
        }
    }

    //--- ④ advance the hold, or seal it, then re-place everything the offset moved -------------------------------------
    if (Drawer.Pivot != DrawerPivot::Dormant)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            AdvanceDrawerDrag(Drawer, Extents, Pointing.MouseDelta.x, Pointing.MouseDelta.y, Pointing.DeltaTime);
        }
        else
        {
            SealDrawerDrag(Drawer, Extents, Reveal);
        }

        // 🔴 Re-placed in full and not merely re-offset. Every rectangle below is derived from the offset the drag
        //    just wrote, and painting the sheet at the new offset while testing the notch at the old one is a
        //    silhouette the artist can see one tick away from the edge it is supposed to be welded to.
        Drawer.Drift = Narrowed(Drawer.Drift, -MaximumDrift, MaximumDrift);

        const float DriftedX = DisplayWidth * 0.5f + Drawer.Drift;

        Placed = ResolveGeometry(Drawer, Extents, DisplayWidth, DisplayHeight,
                                 Reveal, DriftedX, ColumnX, ColumnWidth);
    }

    //--- ⑤ paint the wash, the sheet and the body -----------------------------------------------------------------------
    if (Placed.Offset > 1.0f)
    {
        const float Fraction = Narrowed(Placed.Offset / (Reveal > 1.0f ? Reveal : 1.0f), 0.0f, 1.0f);

        // 📝 The wash is the drawer's own coverage over the desk and it deepens with the reveal, so a drawer barely
        //    pulled out barely dims what is behind it. The two edges wash at different depths because the top
        //    drawer covers the whole display at full reveal and the bottom drawer never does.
        const float Coverage = Drawer.Edge == DrawerEdge::Top ? Extents.DrawerScrimTop : Extents.DrawerScrimBottom;

        ThemeColour Scrim = Palette.DeskBackground;
        Scrim.Coverage    = static_cast<double>(Fraction * Coverage);

        Recording->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(DisplayWidth, DisplayHeight), Coded(Scrim));
        Recording->AddRectFilled(Placed.SheetLower, Placed.SheetUpper, Coded(Palette.DeskBackground));

        // 📐 The framed interior is the content column inset by the sheet's own padding on both of its ends. The
        //    frame is drawn only once there is room for it — a frame taller than the sheet inverts, and an inverted
        //    rectangle fills the whole display in a colour the artist did not ask for.
        const float FrameTop    = Placed.ColumnLower.y + Extents.DrawerContentPadding;
        const float FrameBottom = Placed.ColumnUpper.y - Extents.DrawerContentPadding;

        if (FrameBottom > FrameTop + Extents.DrawerFrameMinimum && ColumnWidth > 0.0f)
        {
            const ImVec2 FrameLower(Placed.ColumnLower.x, FrameTop);
            const ImVec2 FrameUpper(Placed.ColumnUpper.x, FrameBottom);

            Recording->AddRectFilled(FrameLower, FrameUpper, Coded(Palette.PanelBackground), Extents.CornerRounding);
            Recording->AddRect(FrameLower, FrameUpper, Coded(Palette.PanelBorder), Extents.CornerRounding,
                               0, Extents.BorderThickness);

            if (Body != nullptr)
            {
                WorkspaceRectangle Interior;
                Interior.PositionX = FrameLower.x;
                Interior.PositionY = FrameLower.y;
                Interior.Width     = FrameUpper.x - FrameLower.x;
                Interior.Height    = FrameUpper.y - FrameLower.y;

                // 🔴 The body is clipped to the frame it was handed. A panel routine laying out against a rectangle
                //    the sheet is still sliding through would otherwise paint its own rows across the desk behind.
                Recording->PushClipRect(FrameLower, FrameUpper, true);

                Body(Theme, Interior, BodyContext);

                Recording->PopClipRect();
            }
        }
    }

    //--- ⑥ the notch, last, over the sheet's own moving edge --------------------------------------------------------------
    const int CurveSegments = static_cast<int>(Extents.DrawerCurveSegments);

    TraceGripSilhouette(Recording, Placed.GripLower,
                        Extents.DrawerGripWidth, Extents.DrawerGripHeight,
                        Drawer.Edge == DrawerEdge::Bottom,
                        CurveSegments > 1 ? CurveSegments : 1);

    Recording->PathFillConvex(Coded(Palette.PanelHeader));

    if (Drawer.GripCaption != nullptr && Drawer.GripCaption[0] != '\0')
    {
        const ImVec2 Measured = ImGui::CalcTextSize(Drawer.GripCaption);

        Recording->AddText(ImVec2(CentreX - Measured.x * 0.5f,
                                  Placed.GripLower.y + (Extents.DrawerGripHeight - Measured.y) * 0.5f),
                           Coded(OverGrip ? Palette.TextPrimary : Palette.TextMuted),
                           Drawer.GripCaption);
    }

    // 📝 The report is the same predicate the coordinator consulted before the desk painted, plus whatever this
    //    tick's own classification just claimed. Reporting only the pre-paint answer would surrender the pointer
    //    on the tick a press first lands on the notch, and that press would reach the desk beneath as well.
    return Drawer.Pivot != DrawerPivot::Dormant || Drawer.DragActive || Drawer.ContentHeld
        || OverGrip || (ClearanceAchieved && CursorInside(Cursor, Placed.SheetLower, Placed.SheetUpper));
}

}   // namespace Slate
