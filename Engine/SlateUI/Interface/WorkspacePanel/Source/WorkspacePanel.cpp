//============================================================================================================================================
//                                                           WORKSPACEPANEL.CPP
//============================================================================================================================================
// 🧩 The strip ground, the body, the footer and the vacant run — the parts the vendor's tab bar does not draw.

#include "SlateUI/Interface/WorkspacePanel/Api/WorkspacePanel.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Deliver<bool> WorkspacePanel::Construct(RecordingSurface& Recording, const AppearanceSpecification& Declared)
{
    if (Surface != nullptr)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "a construction already stands" });

    Surface    = &Recording;
    Appearance = &Declared;

    return Deliver<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE RECORDING
//------------------------------------------------------------------------------------------------------------------------

Deliver<bool> WorkspacePanel::Record(const PlaneExtent& Extent, const char* Titled)
{
    if (Surface == nullptr || Appearance == nullptr)
        return Deliver<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no construction stands" });

    const WorkspaceMetric& Measure = Appearance->WorkspaceMeasure;
    const WorkspaceInk&    Ink     = Appearance->Workspace;

    // 🔴 With NO workspace open the shell is plain black: no strip, no footer, one invitation in the
    //    middle. A grey strip and footer framing an empty panel present chrome for content that is not
    //    there, which reads as a broken workspace rather than as a fresh one.
    if (Titled == nullptr)
    {
        Surface->Ground(Extent, Ink.BodyGround);

        BodyExtent  = Extent;
        StripExtent = {};

        const float VacantTracking = Measure.VacantText * Measure.VacantTracking;

        Surface->TextRunCapitalised(Extent.LeastAlong  + Extent.SpanAlong()  * 0.5f,
                                    Extent.LeastAcross + Extent.SpanAcross() * 0.5f,
                                    Ink.VacantInk,
                                    "CREATE PANEL",
                                    Measure.VacantText,
                                    VacantTracking,
                                    true);

        return Deliver<bool>::Deliver(true);
    }

    // ① The strip ground. 🔴 Only the GROUND. The tabs are the VENDOR'S, drawn as trapezoids by
    //    `Patches/`'s PatchA from `Style.TabSlant`, on the dock node each workspace is docked into.
    //    A strip drawn by hand here would not interlock, would not carry PatchB's z-order, and would not
    //    answer the vendor's hover and drag arbitration — it would merely look similar.
    const PlaneExtent Strip = { Extent.LeastAlong,
                                Extent.LeastAcross,
                                Extent.MostAlong,
                                Extent.LeastAcross + Measure.StripAcross };

    StripExtent = Strip;

    Surface->Ground(Strip, Ink.StripGround);

    // ② The body. 🔴 `.panelbody` and `.content` are both `--panel`, which is absolute black — the
    //    sheet's OLED ground. Recorded before the footer so the footer's edge lands on top of it.
    const float FooterAcross = Extent.MostAcross - Measure.FooterAcross;

    BodyExtent = { Extent.LeastAlong,
                   Extent.LeastAcross + Measure.StripAcross,
                   Extent.MostAlong,
                   FooterAcross };

    // ⚠️ An extent too short to carry the strip and the footer together yields an inverted body. It is
    //    collapsed rather than recorded inverted, which the vendor fills across the whole panel.
    if (BodyExtent.MostAcross < BodyExtent.LeastAcross)
        BodyExtent.MostAcross = BodyExtent.LeastAcross;

    Surface->Ground(BodyExtent, Ink.BodyGround);

    // ④ The footer. 🔴 The edge is recorded after the ground and after the body, because the sheet
    //    gives `.panelfooter` a `border-top` and `z-index: 2` — a body painted over it loses the one line
    //    separating the workspace from whatever sits below.
    const PlaneExtent Footer = { Extent.LeastAlong, FooterAcross, Extent.MostAlong, Extent.MostAcross };

    Surface->Ground(Footer, Ink.FooterGround);

    const PlaneExtent FooterEdge = { Footer.LeastAlong,
                                     Footer.LeastAcross,
                                     Footer.MostAlong,
                                     Footer.LeastAcross + Measure.FooterEdgeWeight };

    Surface->Ground(FooterEdge, Ink.FooterEdge);

    return Deliver<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE READINGS
//------------------------------------------------------------------------------------------------------------------------

PlaneExtent WorkspacePanel::Body() const
{
    return BodyExtent;
}

PlaneExtent WorkspacePanel::Strip() const
{
    return StripExtent;
}

void WorkspacePanel::Reset()
{
    Surface    = nullptr;
    Appearance = nullptr;
    BodyExtent  = {};
    StripExtent = {};
}

}   // namespace Slate
