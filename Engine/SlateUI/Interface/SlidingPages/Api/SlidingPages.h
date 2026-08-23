//============================================================================================================================================
//                                                          SLIDINGPAGES.H
//============================================================================================================================================
// 🧩 Deterministic page placement shared by every horizontally travelling interface page pair.

#pragma once

#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"

namespace Slate
{

/// 🧩 The two extents participating in one page transition.
/// note  Departing and Incoming retain the supplied page size; the caller confines drawing to Viewport.
struct SlidingPagePlacement
{
    PlaneExtent Departing = {};
    PlaneExtent Incoming  = {};
    bool        Travelling = false;
};

/// 🧩 Resolves page geometry without owning panel state, rendering, or interaction.
class SlidingPages
{
public:

    /// 🧩 Places the departing and incoming pages across one viewport.
    /// in    Progress [-]  zero at the old page, one at the new page; values outside are clamped
    /// in    Forward  [-]  true moves content toward the leading edge, false toward the trailing edge
    /// out   Placement [-] deterministic full-size page extents and whether travel remains in progress
    /// tag   api, guarantee, nonallocating, nonthrowing
    static constexpr SlidingPagePlacement Place(const PlaneExtent& Viewport, float Progress, bool Forward)
    {
        const float Travel = Progress < 0.0f ? 0.0f : (Progress > 1.0f ? 1.0f : Progress);
        const float Bearing = Forward ? 1.0f : -1.0f;
        const float Span = Viewport.Width();
        const float DepartingX = Viewport.MinimumX - Bearing * Span * Travel;
        const float IncomingX = Viewport.MinimumX + Bearing * Span * (1.0f - Travel);

        return SlidingPagePlacement{
            Spanning(DepartingX, Viewport.MinimumY, Span, Viewport.Height()),
            Spanning(IncomingX, Viewport.MinimumY, Span, Viewport.Height()),
            Travel > 0.0f && Travel < 1.0f
        };
    }
};

}   // namespace Slate
