//============================================================================================================================================
//                                                       APPEARANCESPECIFICATION.CPP
//============================================================================================================================================
// 🧩 Multiplies every declared extent by the display scale exactly once.

#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE RESOLVE
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    // 📝 Every extent in MetricScale is a length except the four that are not — the two tracking figures are
    //    in em, the clip fraction is dimensionless, and the display scale is what we are multiplying by. Those
    //    four are restored after the sweep rather than excluded from it, because a sweep with four holes in it
    //    is a sweep somebody will eventually add a fifth field beside without noticing.
    constexpr std::uint32_t MetricFieldCount = sizeof(MetricScale) / sizeof(float);
}

AppearanceSpecification Resolve(double DisplayScale)
{
    AppearanceSpecification Resolved;

    const float  AppliedScale   = (DisplayScale > 0.0) ? static_cast<float>(DisplayScale) : 1.0f;
    const float  TrackingTight  = Resolved.Measure.TrackingTight;
    const float  TrackingWide   = Resolved.Measure.TrackingWide;
    const float  TrackingWider  = Resolved.Measure.TrackingWider;
    const float  TrackingWidest = Resolved.Measure.TrackingWidest;
    const float  ClipFraction   = Resolved.Measure.TongueClipFraction;

    float* Sweeping = reinterpret_cast<float*>(&Resolved.Measure);

    for (std::uint32_t FieldOrdinal = 0u; FieldOrdinal < MetricFieldCount; ++FieldOrdinal)
        Sweeping[FieldOrdinal] *= AppliedScale;

    Resolved.Measure.TrackingTight      = TrackingTight;
    Resolved.Measure.TrackingWide       = TrackingWide;
    Resolved.Measure.TrackingWider      = TrackingWider;
    Resolved.Measure.TrackingWidest     = TrackingWidest;
    Resolved.Measure.TongueClipFraction = ClipFraction;
    Resolved.Measure.DisplayScale       = AppliedScale;

    // 📝 🔴 The three snap rates are the only figures outside `MetricScale` carrying a length, and they are
    //    scaled by hand here rather than moved into the swept record. Moving them would make the sweep
    //    multiply the two fractions and the elasticity as well, and a drawer whose quarter-extent threshold
    //    is half the extent on a 2× display opens when the artist meant to nudge it.
    Resolved.Motion.SnapRateSoft *= static_cast<double>(AppliedScale);
    Resolved.Motion.SnapRateFirm *= static_cast<double>(AppliedScale);
    Resolved.Motion.SnapRateHard *= static_cast<double>(AppliedScale);

    return Resolved;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    RESPONSIVE ARRANGEMENT
//------------------------------------------------------------------------------------------------------------------------

// 📝 🔴 The source's breakpoints are evaluated against the **viewport**, not against the content column. Slate
//    has no viewport media query, so they are evaluated against the extent the lattice actually occupies. At
//    the source's own proportions the two agree; a panel torn off into its own window is where they part, and
//    the content-relative reading is the one that stays correct there.
std::uint32_t LatticeColumns(const MetricScale& Measure, float ContentAlong)
{
    if (ContentAlong >= Measure.BreakpointLarge)
        return 5u;

    if (ContentAlong >= Measure.BreakpointMedium)
        return 4u;

    if (ContentAlong >= Measure.BreakpointSmall)
        return 3u;

    return 2u;
}

}   // namespace Slate
