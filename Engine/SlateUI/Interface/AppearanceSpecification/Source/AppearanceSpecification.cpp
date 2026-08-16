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

/// 🧩 Scales only members measured in display pixels.
/// note  Tracking is measured in em, TongueClipFraction is dimensionless, and DisplayScale records the factor.
///       Every newly declared metric must therefore choose explicitly whether it enters this function.
/// cost  ✔️
void ScaleLengths(MetricScale& Measure, float AppliedScale)
{
    Measure.SpacingUnit             *= AppliedScale;
    Measure.RadiusFine              *= AppliedScale;
    Measure.RadiusSmall             *= AppliedScale;
    Measure.RadiusMedium            *= AppliedScale;
    Measure.RadiusGrand             *= AppliedScale;
    Measure.TextFine                *= AppliedScale;
    Measure.TextSmall               *= AppliedScale;
    Measure.TextBody                *= AppliedScale;
    Measure.TextTitle               *= AppliedScale;
    Measure.LeadingFine             *= AppliedScale;
    Measure.LeadingSmall            *= AppliedScale;
    Measure.LeadingBody             *= AppliedScale;
    Measure.LeadingTitle            *= AppliedScale;
    Measure.WheelTravel             *= AppliedScale;
    Measure.TongueAlong             *= AppliedScale;
    Measure.TongueAcross            *= AppliedScale;
    Measure.TongueGapAlong          *= AppliedScale;
    Measure.TonguePadAlong          *= AppliedScale;
    Measure.GripAlong               *= AppliedScale;
    Measure.GripAcross              *= AppliedScale;
    Measure.GripStripAcross         *= AppliedScale;
    Measure.GripLiftNorth           *= AppliedScale;
    Measure.RailAcross              *= AppliedScale;
    Measure.SymbolChevron           *= AppliedScale;
    Measure.SymbolTongue            *= AppliedScale;
    Measure.SymbolToggle            *= AppliedScale;
    Measure.SymbolVacant            *= AppliedScale;
    Measure.MedallionLattice        *= AppliedScale;
    Measure.MedallionColumn         *= AppliedScale;
    Measure.MedallionPreview        *= AppliedScale;
    Measure.LibraryAlongMedium      *= AppliedScale;
    Measure.LibraryAlongLarge       *= AppliedScale;
    Measure.PreviewAlongMedium      *= AppliedScale;
    Measure.PreviewAlongLarge       *= AppliedScale;
    Measure.LibraryPadAlong         *= AppliedScale;
    Measure.LibraryCaptionAcross    *= AppliedScale;
    Measure.GroupPadAcross          *= AppliedScale;
    Measure.GroupGapAcross          *= AppliedScale;
    Measure.SubjectIndentAlong      *= AppliedScale;
    Measure.SubjectPadTrailing      *= AppliedScale;
    Measure.SubjectStripPad         *= AppliedScale;
    Measure.ContentPad              *= AppliedScale;
    Measure.ContentPadLeading       *= AppliedScale;
    Measure.ContentHeadAcross       *= AppliedScale;
    Measure.ContentHeadPadAlong     *= AppliedScale;
    Measure.ContentHeadGap          *= AppliedScale;
    Measure.ContentTrailingPad      *= AppliedScale;
    Measure.ContentScrollPad        *= AppliedScale;
    Measure.EntryAlongCeiling       *= AppliedScale;
    Measure.EntryPadAlong           *= AppliedScale;
    Measure.EntryPadAcross          *= AppliedScale;
    Measure.TogglePad               *= AppliedScale;
    Measure.ToggleGap               *= AppliedScale;
    Measure.CardGapLattice          *= AppliedScale;
    Measure.CardGapColumn           *= AppliedScale;
    Measure.CardPadColumn           *= AppliedScale;
    Measure.CardGapColumnInner      *= AppliedScale;
    Measure.CardScrimAcross         *= AppliedScale;
    Measure.CardMetaGap             *= AppliedScale;
    Measure.CardMetaLift            *= AppliedScale;
    Measure.CardMetaDot             *= AppliedScale;
    Measure.PreviewGap              *= AppliedScale;
    Measure.PreviewPad              *= AppliedScale;
    Measure.PreviewBoxFloor         *= AppliedScale;
    Measure.PreviewBoxCeiling       *= AppliedScale;
    Measure.SkeletonGapUpper        *= AppliedScale;
    Measure.SkeletonGapLower        *= AppliedScale;
    Measure.SkeletonLeading         *= AppliedScale;
    Measure.BreakpointSmall         *= AppliedScale;
    Measure.BreakpointMedium        *= AppliedScale;
    Measure.BreakpointLarge         *= AppliedScale;
}

}   // namespace

AppearanceSpecification Resolve(double DisplayScale)
{
    AppearanceSpecification Resolved;

    const float AppliedScale = (DisplayScale > 0.0) ? static_cast<float>(DisplayScale) : 1.0f;

    ScaleLengths(Resolved.Measure, AppliedScale);
    Resolved.Measure.DisplayScale = AppliedScale;

    // 📝 🔴 The three snap rates are the only figures outside `MetricScale` carrying a length, and they are
    //    scaled explicitly here rather than enrolled with its pixel measurements. Scaling the whole motion
    //    declaration would also multiply its fractions and elasticity, changing drawer arbitration.
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
