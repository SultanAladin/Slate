//============================================================================================================================================
//                                                            CONTROLPANEL.CPP
//============================================================================================================================================
// 🧩 Reference inspector controls recorded from fixed figures and arbitrated through one interaction index.

#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"

#include <cmath>
#include <cstdio>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   REFERENCE APPEARANCE
//------------------------------------------------------------------------------------------------------------------------

namespace
{

constexpr ThemeToken PanelGround     = Covering(0x101012u);
constexpr ThemeToken FieldGround     = Covering(0x0A0A0Bu);
constexpr ThemeToken ValueGround     = Covering(0x232326u);
constexpr ThemeToken NumberGround    = Covering(0x131315u);
constexpr ThemeToken UnitGround      = Covering(0x33333Au);
constexpr ThemeToken TileGround      = Covering(0x1D1D21u);
constexpr ThemeToken TileHovered      = Covering(0x26262Bu);
constexpr ThemeToken HairColour         = Partial(0xFFFFFFu, 0.06);
constexpr ThemeToken StrongHairColour   = Partial(0xFFFFFFu, 0.10);
constexpr ThemeToken AccentColour       = Covering(0x4A90E2u);
constexpr ThemeToken AccentSoftColour   = Partial(0xFFFFFFu, 0.12);
constexpr ThemeToken TrackTakenColour   = Covering(0x8A8A8Eu);
constexpr ThemeToken PrimaryColour      = Covering(0xECECF0u);
constexpr ThemeToken MutedColour        = Covering(0x7B7B82u);
constexpr ThemeToken FaintColour        = Covering(0x55555Du);
constexpr ThemeToken WhiteColour        = Covering(0xFFFFFFu);
constexpr ThemeToken AbsentColour       = Covering(0x303036u);

constexpr float ReferenceText         = 12.0f;   // [px] - default ImGui typeface presentation size
constexpr float SmallText             = 10.5f;   // [px] - metadata and counts
constexpr float ControlRadius         = 8.0f;    // [px] - tile and segment corner radius
constexpr double HoverDuration        = 120.0;   // [ms] - reference transition-colors duration
constexpr double TakeDuration         = 160.0;   // [ms] - switch and selection transition duration
constexpr double DiscloseDuration     = 200.0;   // [ms] - card and menu disclosure duration
constexpr double CarouselDuration     = 300.0;   // [ms] - inspector page transition duration
constexpr float FoldHeaderHeight      = 31.0f;   // [px] - folding card header
constexpr float FoldRowHeight         = 30.0f;   // [px] - one disclosed property row
constexpr float DropdownHeadHeight    = 32.0f;   // [px] - selection field
constexpr float DropdownOptionHeight  = 26.0f;   // [px] - one menu option
constexpr float DropdownGapY     = 6.0f;    // [px] - field to menu separation
constexpr float ColourHeadHeight      = 40.0f;   // [px] - colour field
constexpr float ColourGapY       = 9.0f;    // [px] - field to picker separation
constexpr float ColourPickerY    = 269.0f;  // [px] - picker card including its padding
constexpr float SaturationY      = 140.0f;  // [px] - saturation and brightness square
constexpr float ColourBarY       = 16.0f;   // [px] - hue and opacity rails

constexpr float Between(float Previous, float Incoming, float Fraction)
{
    return Previous + (Incoming - Previous) * Fraction;
}

constexpr double Held(double Coordinate, double Minimum, double Maximum)
{
    return (Coordinate < Minimum) ? Minimum : (Coordinate > Maximum) ? Maximum : Coordinate;
}

constexpr std::uint8_t BlendCoordinate(std::uint8_t Previous, std::uint8_t Incoming, float Fraction)
{
    return static_cast<std::uint8_t>(Between(static_cast<float>(Previous),
                                              static_cast<float>(Incoming), Fraction) + 0.5f);
}

constexpr ThemeToken Blend(ThemeToken Previous, ThemeToken Incoming, float Fraction)
{
    const float HeldFraction = (Fraction < 0.0f) ? 0.0f : (Fraction > 1.0f) ? 1.0f : Fraction;

    return ThemeToken{ BlendCoordinate(Previous.Red,     Incoming.Red,     HeldFraction),
                        BlendCoordinate(Previous.Green,   Incoming.Green,   HeldFraction),
                        BlendCoordinate(Previous.Blue,    Incoming.Blue,    HeldFraction),
                        BlendCoordinate(Previous.Opacity, Incoming.Opacity, HeldFraction) };
}

constexpr ThemeToken Faded(ThemeToken Declared, float Fraction)
{
    const float HeldFraction = (Fraction < 0.0f) ? 0.0f : (Fraction > 1.0f) ? 1.0f : Fraction;
    Declared.Opacity = static_cast<std::uint8_t>(static_cast<float>(Declared.Opacity) * HeldFraction + 0.5f);
    return Declared;
}

struct HsvCoordinate
{
    float  Hue        = 0.0f;   // [deg] - zero through 360
    float  Saturation = 0.0f;   // [-]   - unit interval
    float  Brightness = 0.0f;   // [-]   - unit interval
};

HsvCoordinate ToHsv(const PickerColour& Colour)
{
    const float Red     = static_cast<float>(Colour.Red)   / 255.0f;
    const float Green   = static_cast<float>(Colour.Green) / 255.0f;
    const float Blue    = static_cast<float>(Colour.Blue)  / 255.0f;
    const float Maximum = std::fmax(Red, std::fmax(Green, Blue));
    const float Minimum    = std::fmin(Red, std::fmin(Green, Blue));
    const float Distance = Maximum - Minimum;

    HsvCoordinate Converted;
    Converted.Brightness = Maximum;
    Converted.Saturation = (Maximum > 0.0f) ? Distance / Maximum : 0.0f;

    if (Distance > 0.0f)
    {
        if (Maximum == Red)        Converted.Hue = 60.0f * std::fmod((Green - Blue) / Distance, 6.0f);
        else if (Maximum == Green) Converted.Hue = 60.0f * ((Blue - Red) / Distance + 2.0f);
        else                        Converted.Hue = 60.0f * ((Red - Green) / Distance + 4.0f);

        if (Converted.Hue < 0.0f)
            Converted.Hue += 360.0f;
    }

    return Converted;
}

PickerColour FromHsv(const HsvCoordinate& Hsv, std::uint8_t Opacity)
{
    const float Chroma = Hsv.Brightness * Hsv.Saturation;
    const float Intermediate = Chroma * (1.0f - std::fabs(std::fmod(Hsv.Hue / 60.0f, 2.0f) - 1.0f));
    const float Added = Hsv.Brightness - Chroma;
    float Red = 0.0f;
    float Green = 0.0f;
    float Blue = 0.0f;

    if (Hsv.Hue < 60.0f)       { Red = Chroma; Green = Intermediate; }
    else if (Hsv.Hue < 120.0f) { Red = Intermediate; Green = Chroma; }
    else if (Hsv.Hue < 180.0f) { Green = Chroma; Blue = Intermediate; }
    else if (Hsv.Hue < 240.0f) { Green = Intermediate; Blue = Chroma; }
    else if (Hsv.Hue < 300.0f) { Red = Intermediate; Blue = Chroma; }
    else                       { Red = Chroma; Blue = Intermediate; }

    PickerColour Converted;
    Converted.Red     = static_cast<std::uint8_t>(std::round((Red + Added) * 255.0f));
    Converted.Green   = static_cast<std::uint8_t>(std::round((Green + Added) * 255.0f));
    Converted.Blue    = static_cast<std::uint8_t>(std::round((Blue + Added) * 255.0f));
    Converted.Opacity = Opacity;
    return Converted;
}

constexpr float CentredY(const PlaneExtent& Extent, float PointSize)
{
    return Extent.MinimumY + (Extent.Height() - PointSize) * 0.5f;
}

void UnsignedRun(char* Delivered, std::uint32_t Extent, std::uint32_t Reading)
{
    if (Delivered == nullptr || Extent < 2u)
        return;

    char          Reversed[12] = {};
    std::uint32_t Count        = 0u;

    do
    {
        Reversed[Count++] = static_cast<char>('0' + Reading % 10u);
        Reading /= 10u;
    }
    while (Reading > 0u && Count < 11u);

    std::uint32_t Written = 0u;

    while (Count > 0u && Written + 1u < Extent)
        Delivered[Written++] = Reversed[--Count];

    Delivered[Written] = '\0';
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                       CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> ControlPanel::Construct(InteractionIndex&              IncomingInteraction,
                                      RecordingSurface&              IncomingRecording,
                                      const ThemeProfile& IncomingAppearance)
{
    if (Interaction != nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "a control panel construction already stands" });

    Interaction = &IncomingInteraction;
    Recording   = &IncomingRecording;
    Appearance  = &IncomingAppearance;

    return Outcome<bool>::Result(true);
}

void ControlPanel::Advance(const PointerCondition& Incoming, double Elapsed)
{
    if (Interaction == nullptr)
        return;

    Sampled = Incoming;
    static_cast<void>(Elapsed);
}

ControlVerdict ControlPanel::ResolveTap(ControlIdentity Target, const PlaneExtent& Extent, bool& Altered)
{
    ControlVerdict Verdict;

    if (Interaction == nullptr || Recording == nullptr || !Interaction->Resolves(Target))
        return Verdict;

    const bool Hovered = Extent.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Hovered && Sampled.ContactPressed && !Interaction->AnyDisclosed())
        Interaction->Grab(Target, ControlPart::Body);

    const bool QuickTap = Sampled.ContactPressed && Sampled.ContactReleased && Hovered;

    if ((Interaction->Released(Target) && Hovered) || QuickTap)
    {
        Altered                  = true;
        Verdict.ReadingAltered = true;
    }

    Interaction->DeclareHovered(Target, Hovered, HoverDuration);
    Verdict.ContactTaken = Interaction->Holding(Target);
    Verdict.Mark         = (Hovered || Verdict.ContactTaken) ? RedrawMark::Recolour : RedrawMark::Quiet;

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                          SWITCH
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ControlPanel::SwitchToggle(ControlIdentity Target, const PlaneExtent& Extent,
                                          const SwitchDeclaration& Declared, bool& Taken)
{
    bool           Altered = false;
    ControlVerdict Verdict = ResolveTap(Target, Extent, Altered);

    if (Altered)
        Taken = !Taken;

    if (Interaction == nullptr || Recording == nullptr)
        return Verdict;

    Interaction->DeclareTaken(Target, Taken, TakeDuration);

    const float TakenFraction = Interaction->TakenFraction(Target);
    const float HoverFraction = Interaction->HoveredFraction(Target);
    const float CaptionX  = Extent.MinimumX;
    const float TrackX    = Extent.MaximumX - 50.0f;
    const PlaneExtent Track   = Spanning(TrackX, Extent.MinimumY, 50.0f, 32.0f);

    Recording->TextRun(CaptionX, CentredY(Extent, ReferenceText),
                       Blend(MutedColour, PrimaryColour, HoverFraction), Declared.Caption, ReferenceText);
    Recording->Ground(Track, Blend(FieldGround, TrackTakenColour, TakenFraction), 16.0f, CornerAll);
    Recording->Edge(Track, Blend(HairColour, StrongHairColour, HoverFraction), 1.0f, 16.0f, CornerAll);

    const float NubX = Between(Track.MinimumX + 16.0f, Track.MaximumX - 16.0f, TakenFraction);
    const float NubRadius = Between(11.0f, 12.0f, HoverFraction);
    Recording->Medallion(NubX, Track.MinimumY + 16.0f, NubRadius, WhiteColour);

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    SEGMENTED CHOICE
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ControlPanel::SegmentedChoice(ControlIdentity Target, const PlaneExtent& Extent,
                                             const SegmentDeclaration& Declared, std::uint32_t& TakenOrdinal)
{
    ControlVerdict Verdict;

    if (Interaction == nullptr || Recording == nullptr || Declared.Captions == nullptr || Declared.CaptionCount == 0u)
        return Verdict;

    const float SegmentX = Extent.Width() / static_cast<float>(Declared.CaptionCount);
    std::uint32_t HoveredOrdinal = Declared.CaptionCount;

    for (std::uint32_t Ordinal = 0u; Ordinal < Declared.CaptionCount; ++Ordinal)
    {
        const PlaneExtent Segment = Spanning(Extent.MinimumX + SegmentX * static_cast<float>(Ordinal),
                                             Extent.MinimumY, SegmentX, Extent.Height());

        if (Segment.Encloses(Sampled.PositionX, Sampled.PositionY))
            HoveredOrdinal = Ordinal;
    }

    const bool Hovered = HoveredOrdinal < Declared.CaptionCount;

    if (Hovered && Sampled.ContactPressed)
    {
        Interaction->Grab(Target, ControlPart::Body);
        Interaction->RecordInitial(Target, static_cast<float>(HoveredOrdinal));
    }

    if ((Interaction->Released(Target) && Hovered) ||
        (Sampled.ContactPressed && Sampled.ContactReleased && Hovered))
    {
        TakenOrdinal             = HoveredOrdinal;
        Verdict.ReadingAltered = true;
    }

    Interaction->DeclareHovered(Target, Hovered, HoverDuration);
    Interaction->DeclareTaken(Target, TakenOrdinal < Declared.CaptionCount, TakeDuration);

    const float HoverFraction = Interaction->HoveredFraction(Target);

    Recording->Ground(Extent, FieldGround, ControlRadius, CornerAll);
    Recording->Edge(Extent, HairColour, 1.0f, ControlRadius, CornerAll);

    for (std::uint32_t Ordinal = 0u; Ordinal < Declared.CaptionCount; ++Ordinal)
    {
        const PlaneExtent Segment = Spanning(Extent.MinimumX + SegmentX * static_cast<float>(Ordinal),
                                             Extent.MinimumY, SegmentX, Extent.Height());
        const bool Taken = Ordinal == TakenOrdinal;
        const PlaneExtent Inset = { Segment.MinimumX + 3.0f, Segment.MinimumY + 3.0f,
                                    Segment.MaximumX - 3.0f, Segment.MaximumY - 3.0f };

        if (Ordinal == HoveredOrdinal && !Taken)
            Recording->Ground(Inset, Blend(FieldGround, TileHovered, HoverFraction), 6.0f, CornerAll);

        if (Taken)
        {
            Recording->Ground(Inset, AccentSoftColour, 6.0f, CornerAll);
            Recording->Edge(Inset, Blend(StrongHairColour, AccentColour, 0.65f), 1.0f, 6.0f, CornerAll);
        }

        const float RunX = Recording->MeasureRun(Declared.Captions[Ordinal], ReferenceText);
        Recording->TextRun(Segment.MinimumX + (Segment.Width() - RunX) * 0.5f,
                           CentredY(Segment, ReferenceText), Taken ? PrimaryColour : MutedColour,
                           Declared.Captions[Ordinal], ReferenceText, 0.0f, Taken);
    }

    Verdict.ContactTaken = Interaction->Holding(Target);
    Verdict.Mark         = Hovered ? RedrawMark::Recolour : RedrawMark::Quiet;

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                          TAB STRIP
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ControlPanel::TabStrip(ControlIdentity Target, const PlaneExtent& Extent,
                                      const TabDeclaration& Declared, std::uint32_t& TakenOrdinal)
{
    ControlVerdict Verdict;

    if (Interaction == nullptr || Recording == nullptr || Declared.Captions == nullptr || Declared.CaptionCount == 0u)
        return Verdict;

    const float TabX = Extent.Width() / static_cast<float>(Declared.CaptionCount);
    std::uint32_t HoveredOrdinal = Declared.CaptionCount;

    for (std::uint32_t Ordinal = 0u; Ordinal < Declared.CaptionCount; ++Ordinal)
    {
        const PlaneExtent Tab = Spanning(Extent.MinimumX + TabX * static_cast<float>(Ordinal),
                                         Extent.MinimumY, TabX, Extent.Height());
        if (Tab.Encloses(Sampled.PositionX, Sampled.PositionY))
            HoveredOrdinal = Ordinal;
    }

    const bool Hovered = HoveredOrdinal < Declared.CaptionCount;

    if (Hovered && Sampled.ContactPressed)
        Interaction->Grab(Target, ControlPart::Body);

    if ((Interaction->Released(Target) && Hovered) ||
        (Sampled.ContactPressed && Sampled.ContactReleased && Hovered))
    {
        TakenOrdinal             = HoveredOrdinal;
        Verdict.ReadingAltered = true;
    }

    Interaction->DeclareHovered(Target, Hovered, HoverDuration);
    const float HoverFraction = Interaction->HoveredFraction(Target);

    Recording->Ground(Extent, PanelGround, 0.0f, CornerNone);
    Recording->Ground(Spanning(Extent.MinimumX, Extent.MaximumY - 1.0f, Extent.Width(), 1.0f), HairColour, 0.0f, CornerNone);

    for (std::uint32_t Ordinal = 0u; Ordinal < Declared.CaptionCount; ++Ordinal)
    {
        const PlaneExtent Tab = Spanning(Extent.MinimumX + TabX * static_cast<float>(Ordinal),
                                         Extent.MinimumY, TabX, Extent.Height());
        const bool Taken = Ordinal == TakenOrdinal;
        const float RunX = Recording->MeasureRun(Declared.Captions[Ordinal], ReferenceText);

        if (Ordinal == HoveredOrdinal && !Taken)
            Recording->Ground(Tab, Blend(PanelGround, TileHovered, HoverFraction), 0.0f, CornerNone);

        Recording->TextRun(Tab.MinimumX + (Tab.Width() - RunX) * 0.5f,
                           CentredY(Tab, ReferenceText),
                           Taken ? PrimaryColour : Blend(MutedColour, PrimaryColour,
                                                      (Ordinal == HoveredOrdinal) ? HoverFraction : 0.0f),
                           Declared.Captions[Ordinal], ReferenceText, 0.0f, Taken);

        if (Taken)
        {
            const float UnderlineX = Tab.Width() - 12.0f;
            Recording->Ground(Spanning(Tab.MinimumX + 6.0f, Tab.MaximumY - 2.0f,
                                       UnderlineX, 2.0f), AccentColour, 0.0f, CornerNone);
        }
    }

    Verdict.ContactTaken = Interaction->Holding(Target);
    Verdict.Mark         = Hovered ? RedrawMark::Recolour : RedrawMark::Quiet;

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     INSPECTOR CAROUSEL
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ControlPanel::CarouselPages(ControlIdentity Target, const PlaneExtent& Extent,
                                           const CarouselDeclaration& Declared, std::uint32_t TakenOrdinal)
{
    ControlVerdict Verdict;

    if (Interaction == nullptr || Recording == nullptr)
        return Verdict;

    const bool TrailingTaken = TakenOrdinal == 1u;
    Interaction->DeclareTaken(Target, TrailingTaken, CarouselDuration, EaseCurve::Carousel);

    const float Travel = Interaction->TakenFraction(Target);
    const float PageX = Extent.Width();
    const float LeadingX = Extent.MinimumX - PageX * Travel;
    const float TrailingX = Extent.MinimumX + PageX * (1.0f - Travel);

    Recording->Ground(Extent, FieldGround, ControlRadius, CornerAll);
    Recording->Edge(Extent, HairColour, 1.0f, ControlRadius, CornerAll);
    Recording->Confine(Extent);

    const auto RecordLeading = [&](float X)
    {
        const PlaneExtent Page = Spanning(X, Extent.MinimumY, PageX, Extent.Height());
        Recording->TextRunCapitalised(Page.MinimumX + 10.0f, Page.MinimumY + 9.0f,
                                      FaintColour, "Property cards", SmallText, 0.08f, true);

        if (Declared.LeadingRuns == nullptr)
            return;

        for (std::uint32_t Ordinal = 0u; Ordinal < Declared.LeadingCount; ++Ordinal)
        {
            const PlaneExtent Card = Spanning(Page.MinimumX + 8.0f,
                                              Page.MinimumY + 30.0f + 38.0f * static_cast<float>(Ordinal),
                                              Page.Width() - 16.0f, 32.0f);
            Recording->Ground(Card, PanelGround, 7.0f, CornerAll);
            Recording->Edge(Card, HairColour, 1.0f, 7.0f, CornerAll);
            Recording->Stroke(SymbolSubject::ChevronRight,
                              Spanning(Card.MinimumX + 8.0f, Card.MinimumY + 9.0f, 13.0f, 13.0f), FaintColour);
            Recording->TextRunTruncated(Card.MinimumX + 29.0f, CentredY(Card, ReferenceText),
                                        Card.MaximumX - 10.0f, PrimaryColour,
                                        Declared.LeadingRuns[Ordinal], ReferenceText, true);
        }
    };

    const auto RecordTrailing = [&](float X)
    {
        const PlaneExtent Page = Spanning(X, Extent.MinimumY, PageX, Extent.Height());
        Recording->TextRunCapitalised(Page.MinimumX + 10.0f, Page.MinimumY + 9.0f,
                                      FaintColour, "Revision sequence", SmallText, 0.08f, true);

        if (Declared.TrailingRuns == nullptr)
            return;

        const float MarkerX = Page.MinimumX + 18.0f;
        Recording->Ground(Spanning(MarkerX - 0.5f, Page.MinimumY + 30.0f, 1.0f,
                                   38.0f * static_cast<float>(Declared.TrailingCount)), HairColour,
                          0.0f, CornerNone);

        for (std::uint32_t Ordinal = 0u; Ordinal < Declared.TrailingCount; ++Ordinal)
        {
            const float Y = Page.MinimumY + 30.0f + 38.0f * static_cast<float>(Ordinal);
            Recording->Medallion(MarkerX, Y + 16.0f, Ordinal == 0u ? 4.0f : 3.0f,
                                 Ordinal == 0u ? AccentColour : FaintColour);

            const PlaneExtent Card = Spanning(Page.MinimumX + 31.0f, Y,
                                              Page.Width() - 39.0f, 32.0f);
            Recording->Ground(Card, Ordinal == 0u ? AccentSoftColour : PanelGround, 7.0f, CornerAll);
            Recording->Edge(Card, Ordinal == 0u ? AccentColour : HairColour, 1.0f, 7.0f, CornerAll);
            Recording->TextRunTruncated(Card.MinimumX + 9.0f, CentredY(Card, ReferenceText),
                                        Card.MaximumX - 9.0f, PrimaryColour,
                                        Declared.TrailingRuns[Ordinal], ReferenceText, Ordinal == 0u);
        }
    };

    RecordLeading(LeadingX);
    RecordTrailing(TrailingX);
    Recording->Release();

    Verdict.Mark = (Travel > 0.0f && Travel < 1.0f) ? RedrawMark::Rerecord : RedrawMark::Quiet;
    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       FOLDING CARD
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ControlPanel::CollapsibleCard(ControlIdentity Target, const PlaneExtent& Extent,
                                             const FoldDeclaration& Declared, bool& ExpansionEnabled)
{
    const PlaneExtent Header = { Extent.MinimumX, Extent.MinimumY, Extent.MaximumX,
                                 Extent.MinimumY + FoldHeaderHeight };
    bool           Altered = false;
    ControlVerdict Verdict = ResolveTap(Target, Header, Altered);

    if (Altered)
        ExpansionEnabled = !ExpansionEnabled;

    if (Interaction == nullptr || Recording == nullptr)
        return Verdict;

    Interaction->DeclareTaken(Target, ExpansionEnabled, DiscloseDuration);

    const float Disclosure = Interaction->TakenFraction(Target);
    const float BodyHeight = FoldRowHeight * static_cast<float>(Declared.BodyCount) + 8.0f;
    const float ContentY = FoldHeaderHeight + BodyHeight * Disclosure;
    const PlaneExtent Current = { Extent.MinimumX, Extent.MinimumY, Extent.MaximumX,
                                    Extent.MinimumY + ContentY };

    Recording->Ground(Current, PanelGround, ControlRadius, CornerAll);
    Recording->Edge(Current, HairColour, 1.0f, ControlRadius, CornerAll);
    Recording->Ground(Spanning(Header.MinimumX, Header.MaximumY - 1.0f, Header.Width(),
                               Disclosure), HairColour, 0.0f, CornerNone);

    const PlaneExtent SymbolExtent = Spanning(Header.MinimumX + 8.0f, Header.MinimumY + 8.0f, 14.0f, 14.0f);
    Recording->Stroke((Disclosure > 0.5f) ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                      SymbolExtent, Blend(FaintColour, PrimaryColour, Interaction->HoveredFraction(Target)));
    Recording->TextRun(Header.MinimumX + 30.0f, CentredY(Header, SmallText), MutedColour,
                       Declared.Caption, SmallText, 0.08f, true);

    char CountRun[12] = {};
    UnsignedRun(CountRun, 12u, Declared.BodyCount);
    const float CountX = Recording->MeasureRun(CountRun, SmallText);
    Recording->TextRun(Header.MaximumX - CountX - 10.0f, CentredY(Header, SmallText),
                       FaintColour, CountRun, SmallText);

    if (Disclosure > 0.0f && Declared.BodyRuns != nullptr)
    {
        const PlaneExtent Revealed = { Extent.MinimumX, Header.MaximumY,
                                       Extent.MaximumX, Header.MaximumY + BodyHeight * Disclosure };
        Recording->Confine(Revealed);

        for (std::uint32_t Ordinal = 0u; Ordinal < Declared.BodyCount; ++Ordinal)
        {
            const PlaneExtent Row = Spanning(Extent.MinimumX + 8.0f,
                                             Header.MaximumY + 4.0f + FoldRowHeight * static_cast<float>(Ordinal),
                                             Extent.Width() - 16.0f, FoldRowHeight - 4.0f);
            Recording->Ground(Row, FieldGround, 7.0f, CornerAll);
            Recording->TextRun(Row.MinimumX + 10.0f, CentredY(Row, ReferenceText), MutedColour,
                               Declared.BodyRuns[Ordinal], ReferenceText);
            Recording->TextRun(Row.MaximumX - 52.0f, CentredY(Row, ReferenceText), PrimaryColour,
                               (Ordinal == 0u) ? "0.00" : (Ordinal == 1u) ? "0.0" : "1.00", ReferenceText);
        }

        Recording->Release();
    }

    Verdict.Mark = (Disclosure > 0.0f && Disclosure < 1.0f) ? RedrawMark::Rearrange : Verdict.Mark;
    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       DROPDOWN CARD
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ControlPanel::DropdownCard(ControlIdentity Target, const PlaneExtent& Extent,
                                          const DropdownDeclaration& Declared, std::uint32_t& TakenOrdinal)
{
    ControlVerdict Verdict;

    if (Interaction == nullptr || Recording == nullptr || Declared.Options == nullptr || Declared.OptionCount == 0u)
        return Verdict;

    if (TakenOrdinal >= Declared.OptionCount)
        TakenOrdinal = 0u;

    const PlaneExtent Head = { Extent.MinimumX, Extent.MinimumY,
                               Extent.MaximumX, Extent.MinimumY + DropdownHeadHeight };
    const bool Open = Interaction->Disclosed(Target);
    const float MenuTop = Head.MaximumY + DropdownGapY;
    std::uint32_t HoveredOption = Declared.OptionCount;

    for (std::uint32_t Ordinal = 0u; Ordinal < Declared.OptionCount; ++Ordinal)
    {
        const PlaneExtent Option = Spanning(Extent.MinimumX, MenuTop + DropdownOptionHeight * static_cast<float>(Ordinal),
                                            Extent.Width(), DropdownOptionHeight);
        if (Open && Option.Encloses(Sampled.PositionX, Sampled.PositionY))
            HoveredOption = Ordinal;
    }

    const bool HeadHovered = Head.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool MenuHovered = HoveredOption < Declared.OptionCount;

    if ((HeadHovered || MenuHovered) && Sampled.ContactPressed)
        Interaction->Grab(Target, MenuHovered ? ControlPart::Option : ControlPart::Body);
    else if (Open && Sampled.ContactPressed && !Extent.Encloses(Sampled.PositionX, Sampled.PositionY))
        Interaction->Withdraw();

    const bool QuickTap = Sampled.ContactPressed && Sampled.ContactReleased && (HeadHovered || MenuHovered);

    if (Interaction->Released(Target) || QuickTap)
    {
        if (MenuHovered)
        {
            TakenOrdinal             = HoveredOption;
            Verdict.ReadingAltered = true;
            Interaction->Withdraw();
        }
        else if (HeadHovered)
        {
            if (Open) Interaction->Withdraw();
            else      Interaction->Disclose(Target);
        }
    }

    const bool DisclosureOpen = Interaction->Disclosed(Target);
    Interaction->DeclareHovered(Target, HeadHovered || MenuHovered, HoverDuration);
    Interaction->DeclareTaken(Target, DisclosureOpen, DiscloseDuration);

    const float HoverFraction = Interaction->HoveredFraction(Target);
    const float Disclosure = Interaction->TakenFraction(Target);
    const float CaptionX = 92.0f;
    const PlaneExtent Field = { Head.MinimumX + CaptionX, Head.MinimumY, Head.MaximumX, Head.MaximumY };

    Recording->TextRun(Head.MinimumX, CentredY(Head, ReferenceText),
                       Blend(MutedColour, PrimaryColour, HeadHovered ? HoverFraction : 0.0f),
                       Declared.Caption, ReferenceText);
    Recording->Ground(Field, FieldGround, 16.0f, CornerAll);
    Recording->Edge(Field, Blend(HairColour, StrongHairColour, HoverFraction), 1.0f, 16.0f, CornerAll);
    Recording->TextRunTruncated(Field.MinimumX + 12.0f, CentredY(Field, ReferenceText),
                                Field.MaximumX - 35.0f, PrimaryColour,
                                Declared.Options[TakenOrdinal], ReferenceText);
    Recording->Stroke(Disclosure > 0.5f ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                      Spanning(Field.MaximumX - 25.0f, Field.MinimumY + 9.0f, 13.0f, 13.0f), MutedColour);

    if (Disclosure > 0.0f)
    {
        const float MenuHeight = DropdownOptionHeight * static_cast<float>(Declared.OptionCount) + 8.0f;
        const PlaneExtent Menu = { Field.MinimumX, MenuTop, Field.MaximumX,
                                   MenuTop + MenuHeight * Disclosure };
        Recording->Ground(Menu, PanelGround, 9.0f, CornerAll);
        Recording->Edge(Menu, StrongHairColour, 1.0f, 9.0f, CornerAll);
        Recording->Confine(Menu);

        for (std::uint32_t Ordinal = 0u; Ordinal < Declared.OptionCount; ++Ordinal)
        {
            const PlaneExtent Option = Spanning(Menu.MinimumX + 4.0f,
                                                Menu.MinimumY + 4.0f + DropdownOptionHeight * static_cast<float>(Ordinal),
                                                Menu.Width() - 8.0f, DropdownOptionHeight);
            const bool Taken = Ordinal == TakenOrdinal;
            const bool Hovered = Ordinal == HoveredOption;

            if (Hovered)
            {
                Recording->Ground(Option, TileHovered, 5.0f, CornerAll);
                Recording->Ground(Spanning(Option.MinimumX, Option.MinimumY, 3.0f, Option.Height()),
                                  AccentColour, 1.0f, CornerAll);
            }

            Recording->TextRunTruncated(Option.MinimumX + 10.0f, CentredY(Option, SmallText),
                                        Option.MaximumX - 26.0f, Taken ? PrimaryColour : MutedColour,
                                        Declared.Options[Ordinal], SmallText, Taken);
            Recording->Medallion(Option.MaximumX - 11.0f,
                                 Option.MinimumY + Option.Height() * 0.5f,
                                 Taken ? 4.0f : 3.0f, Taken ? AccentColour : FaintColour);
        }

        Recording->Release();
    }

    Verdict.ContactTaken = Interaction->Holding(Target);
    Verdict.Mark = (Disclosure > 0.0f && Disclosure < 1.0f) ? RedrawMark::Rearrange
                                                            : (HeadHovered || MenuHovered) ? RedrawMark::Recolour
                                                                                         : RedrawMark::Quiet;
    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        COLOUR PICKER
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ControlPanel::ColourPicker(ControlIdentity Target, const PlaneExtent& Extent,
                                          const ColourPickerDeclaration& Declared, PickerColour& Colour)
{
    ControlVerdict Verdict;

    if (Interaction == nullptr || Recording == nullptr)
        return Verdict;

    const PlaneExtent Head = Spanning(Extent.MinimumX, Extent.MinimumY + 23.0f,
                                      Extent.Width(), ColourHeadHeight);
    const PlaneExtent Caret = { Head.MaximumX - 40.0f, Head.MinimumY, Head.MaximumX, Head.MaximumY };
    const float PickerTop = Head.MaximumY + ColourGapY;
    const PlaneExtent Picker = Spanning(Extent.MinimumX, PickerTop, Extent.Width(), ColourPickerY);
    const PlaneExtent Saturation = Spanning(Picker.MinimumX + 13.0f, Picker.MinimumY + 13.0f,
                                            Picker.Width() - 26.0f, SaturationY);
    const PlaneExtent HueRail = Spanning(Saturation.MinimumX, Saturation.MaximumY + 12.0f,
                                         Saturation.Width(), ColourBarY);
    const PlaneExtent OpacityRail = Spanning(Saturation.MinimumX, HueRail.MaximumY + 11.0f,
                                             Saturation.Width(), ColourBarY);

    const bool Open = Interaction->Disclosed(Target);
    const bool HeadHovered = Head.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool SaturationHovered = Open && Saturation.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool HueHovered = Open && HueRail.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool OpacityHovered = Open && OpacityRail.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Sampled.ContactPressed)
    {
        if (HeadHovered)             Interaction->Grab(Target, ControlPart::Body);
        else if (SaturationHovered)  Interaction->Grab(Target, ControlPart::Track);
        else if (HueHovered)         Interaction->Grab(Target, ControlPart::Strip);
        else if (OpacityHovered)     Interaction->Grab(Target, ControlPart::Thumb);
        else if (Open && !Picker.Encloses(Sampled.PositionX, Sampled.PositionY))
            Interaction->Withdraw();
    }

    HsvCoordinate Hsv = ToHsv(Colour);

    if (Interaction->Holding(Target))
    {
        const ControlPart Part = Interaction->HeldPart(Target);

        if (Part == ControlPart::Track)
        {
            Hsv.Saturation = static_cast<float>(Held((Sampled.PositionX - Saturation.MinimumX) /
                                                     Saturation.Width(), 0.0, 1.0));
            Hsv.Brightness = static_cast<float>(Held(1.0 - (Sampled.PositionY - Saturation.MinimumY) /
                                                     Saturation.Height(), 0.0, 1.0));
            Colour = FromHsv(Hsv, Colour.Opacity);
            Verdict.ReadingAltered = true;
        }
        else if (Part == ControlPart::Strip)
        {
            Hsv.Hue = static_cast<float>(Held((Sampled.PositionX - HueRail.MinimumX) /
                                              HueRail.Width(), 0.0, 1.0) * 360.0);
            Colour = FromHsv(Hsv, Colour.Opacity);
            Verdict.ReadingAltered = true;
        }
        else if (Part == ControlPart::Thumb)
        {
            Colour.Opacity = static_cast<std::uint8_t>(std::round(Held(
                (Sampled.PositionX - OpacityRail.MinimumX) / OpacityRail.Width(), 0.0, 1.0) * 255.0));
            Verdict.ReadingAltered = true;
        }
    }

    const bool QuickTap = Sampled.ContactPressed && Sampled.ContactReleased && HeadHovered;

    if ((Interaction->Released(Target) && HeadHovered) || QuickTap)
    {
        if (Open) Interaction->Withdraw();
        else      Interaction->Disclose(Target);
    }

    const bool DisclosureOpen = Interaction->Disclosed(Target);
    Interaction->DeclareHovered(Target, HeadHovered, HoverDuration);
    Interaction->DeclareTaken(Target, DisclosureOpen, DiscloseDuration);

    const float Disclosure = Interaction->TakenFraction(Target);
    const float HoverFraction = Interaction->HoveredFraction(Target);
    Hsv = ToHsv(Colour);

    Recording->TextRun(Extent.MinimumX, Extent.MinimumY,
                       Blend(MutedColour, PrimaryColour, HoverFraction), Declared.Caption, ReferenceText);
    Recording->Ground(Head, FieldGround, 20.0f, CornerAll);
    Recording->Ground(Caret, Blend(UnitGround, TileHovered, HoverFraction), 20.0f,
                      CornerTrailingUpper | CornerTrailingLower);

    ThemeToken CurrentColour{ Colour.Red, Colour.Green, Colour.Blue, Colour.Opacity };
    Recording->Medallion(Head.MinimumX + 24.0f, Head.MinimumY + 20.0f, 12.0f, CurrentColour);
    Recording->Edge(Spanning(Head.MinimumX + 12.0f, Head.MinimumY + 8.0f, 24.0f, 24.0f),
                    StrongHairColour, 1.0f, 12.0f, CornerAll);

    char RgbaRun[64] = {};
    std::snprintf(RgbaRun, sizeof(RgbaRun), "rgba(%u, %u, %u, %.2f)",
                  static_cast<unsigned>(Colour.Red), static_cast<unsigned>(Colour.Green),
                  static_cast<unsigned>(Colour.Blue), static_cast<double>(Colour.Opacity) / 255.0);
    Recording->TextRunTruncated(Head.MinimumX + 47.0f, CentredY(Head, ReferenceText),
                                Caret.MinimumX - 8.0f, PrimaryColour, RgbaRun, ReferenceText);
    Recording->Stroke(Disclosure > 0.5f ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                      Spanning(Caret.MinimumX + 13.0f, Caret.MinimumY + 13.0f, 14.0f, 14.0f), MutedColour);

    if (Disclosure > 0.0f)
    {
        const PlaneExtent Revealed = { Picker.MinimumX, Picker.MinimumY, Picker.MaximumX,
                                       Picker.MinimumY + Picker.Height() * Disclosure };
        Recording->Ground(Revealed, ValueGround, 13.0f, CornerAll);
        Recording->Confine(Revealed);

        HsvCoordinate HueOnly{ Hsv.Hue, 1.0f, 1.0f };
        const PickerColour HueColour = FromHsv(HueOnly, 255u);
        Recording->Ground(Saturation, { HueColour.Red, HueColour.Green, HueColour.Blue, 255u }, 10.0f, CornerAll);
        Recording->Scrim(Saturation, WhiteColour, { 255u, 255u, 255u, 0u }, ScrimAxis::X);
        Recording->Scrim(Saturation, { 0u, 0u, 0u, 0u }, { 0u, 0u, 0u, 255u }, ScrimAxis::Y);
        Recording->MaskCorners(Saturation, ValueGround, 10.0f);

        const float SaturationX = Saturation.MinimumX + Saturation.Width() * Hsv.Saturation;
        const float BrightnessY = Saturation.MinimumY + Saturation.Height() * (1.0f - Hsv.Brightness);
        Recording->Medallion(SaturationX, BrightnessY, 9.0f, WhiteColour);
        Recording->Medallion(SaturationX, BrightnessY, 6.0f, CurrentColour);

        constexpr ThemeToken HueStops[7] = {
            Covering(0xFF0000u), Covering(0xFFFF00u), Covering(0x00FF00u), Covering(0x00FFFFu),
            Covering(0x0000FFu), Covering(0xFF00FFu), Covering(0xFF0000u)
        };
        const float HueSegment = HueRail.Width() / 6.0f;

        for (std::uint32_t Ordinal = 0u; Ordinal < 6u; ++Ordinal)
        {
            Recording->Scrim(Spanning(HueRail.MinimumX + HueSegment * static_cast<float>(Ordinal),
                                      HueRail.MinimumY, HueSegment + 1.0f, HueRail.Height()),
                             HueStops[Ordinal], HueStops[Ordinal + 1u], ScrimAxis::X);
        }
        Recording->MaskCorners(HueRail, ValueGround, ColourBarY * 0.5f);

        const float HueX = HueRail.MinimumX + HueRail.Width() * (Hsv.Hue / 360.0f);
        Recording->Medallion(HueX, HueRail.MinimumY + 8.0f, 10.0f, Covering(0x1B1B1Eu));
        Recording->Medallion(HueX, HueRail.MinimumY + 8.0f, 8.0f, WhiteColour);

        constexpr float CheckExtent = 10.0f;
        const std::uint32_t XCount = static_cast<std::uint32_t>(
            std::ceil(OpacityRail.Width() / CheckExtent));
        const std::uint32_t YCount = static_cast<std::uint32_t>(
            std::ceil(OpacityRail.Height() / CheckExtent));
        Recording->Confine(OpacityRail);

        for (std::uint32_t YOrdinal = 0u; YOrdinal < YCount; ++YOrdinal)
        {
            for (std::uint32_t XOrdinal = 0u; XOrdinal < XCount; ++XOrdinal)
            {
                const ThemeToken CheckColour = ((XOrdinal + YOrdinal) % 2u == 0u)
                                            ? Covering(0x808080u) : Covering(0xC0C0C0u);
                Recording->Ground(Spanning(OpacityRail.MinimumX + CheckExtent * static_cast<float>(XOrdinal),
                                           OpacityRail.MinimumY + CheckExtent * static_cast<float>(YOrdinal),
                                           CheckExtent, CheckExtent), CheckColour, 0.0f, CornerNone);
            }
        }

        Recording->Scrim(OpacityRail, { Colour.Red, Colour.Green, Colour.Blue, 0u },
                          { Colour.Red, Colour.Green, Colour.Blue, 255u }, ScrimAxis::X);
        Recording->MaskCorners(OpacityRail, ValueGround, ColourBarY * 0.5f);
        Recording->Release();
        const float OpacityX = OpacityRail.MinimumX + OpacityRail.Width() *
                                  (static_cast<float>(Colour.Opacity) / 255.0f);
        Recording->Medallion(OpacityX, OpacityRail.MinimumY + 8.0f, 10.0f, Covering(0x1B1B1Eu));
        Recording->Medallion(OpacityX, OpacityRail.MinimumY + 8.0f, 8.0f, WhiteColour);

        const PlaneExtent HexField = Spanning(Picker.MinimumX + 13.0f, OpacityRail.MaximumY + 12.0f,
                                              108.0f, 36.0f);
        Recording->Ground(HexField, NumberGround, 9.0f, CornerAll);
        char HexRun[8] = {};
        std::snprintf(HexRun, sizeof(HexRun), "#%02X%02X%02X",
                      static_cast<unsigned>(Colour.Red), static_cast<unsigned>(Colour.Green),
                      static_cast<unsigned>(Colour.Blue));
        Recording->TextRun(HexField.MinimumX + 10.0f, CentredY(HexField, 14.0f),
                           PrimaryColour, HexRun, 14.0f);

        char AlphaRun[16] = {};
        std::snprintf(AlphaRun, sizeof(AlphaRun), "A %u%%",
                      static_cast<unsigned>(std::round(static_cast<double>(Colour.Opacity) / 255.0 * 100.0)));
        const float AlphaX = Recording->MeasureRun(AlphaRun, ReferenceText);
        Recording->TextRun(Picker.MaximumX - AlphaX - 13.0f, CentredY(HexField, ReferenceText),
                           MutedColour, AlphaRun, ReferenceText);
        Recording->Release();
    }

    Verdict.ContactTaken = Interaction->Holding(Target);
    Verdict.Mark = (Disclosure > 0.0f && Disclosure < 1.0f) ? RedrawMark::Rearrange
                                                            : Verdict.ReadingAltered ? RedrawMark::Recolour
                                                                                      : RedrawMark::Quiet;
    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    OUTLINE DISCLOSURE
//------------------------------------------------------------------------------------------------------------------------

float ControlPanel::OutlineExpansion(ControlIdentity Target, bool ExpansionEnabled, bool AnimationEnabled)
{
    if (Interaction == nullptr || !Interaction->Resolves(Target))
        return ExpansionEnabled ? 1.0f : 0.0f;

    if (!AnimationEnabled)
        return ExpansionEnabled ? 1.0f : 0.0f;

    Interaction->DeclareTaken(Target, ExpansionEnabled, HoverDuration, EaseCurve::CssEase);
    return Interaction->TakenFraction(Target);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        OUTLINE ROW
//------------------------------------------------------------------------------------------------------------------------

ControlVerdict ControlPanel::OutlineRow(ControlIdentity Target, const PlaneExtent& Extent,
                                        const OutlineDeclaration& Declared, bool SelectionExtended,
                                        float ExpansionFraction, OutlineDropPlacement DropPlacement,
                                        bool& ExpansionEnabled, bool& Selected, bool& PresenceEnabled)
{
    ControlVerdict Verdict;

    if (Interaction == nullptr || Recording == nullptr)
        return Verdict;

    const float Indent = 7.0f + static_cast<float>(Declared.Depth) * 17.0f;
    const PlaneExtent DisclosureExtent = Spanning(Extent.MinimumX + Indent - 2.0f,
                                                   Extent.MinimumY + 3.0f, 18.0f, 22.0f);
    const PlaneExtent PresenceExtent = Spanning(Extent.MaximumX - 27.0f, Extent.MinimumY + 3.0f, 22.0f, 22.0f);
    const bool DisclosureHovered = Declared.EnclosedCount > 0u &&
                                  DisclosureExtent.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool PresenceHovered = PresenceExtent.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool RowHovered      = Extent.Encloses(Sampled.PositionX, Sampled.PositionY);
    const float DragX     = Sampled.PositionX - Interaction->OriginX();
    const float DragY    = Sampled.PositionY - Interaction->OriginY();
    const bool BodyHeld       = Interaction->Holding(Target) &&
                                Interaction->HeldPart(Target) == ControlPart::Body;
    const bool BodyReleased   = Interaction->Released(Target) &&
                                Interaction->ReleasedControlPart(Target) == ControlPart::Body;
    const bool Dragged        = (BodyHeld || BodyReleased) &&
                                (DragX * DragX + DragY * DragY >= 16.0f);

    if (RowHovered && Sampled.ContactPressed)
    {
        Interaction->Grab(Target, (PresenceHovered || DisclosureHovered)
                                  ? ControlPart::Chevron : ControlPart::Body);
    }

    if (((Interaction->Released(Target) && RowHovered) ||
         (Sampled.ContactPressed && Sampled.ContactReleased && RowHovered)) && !Dragged)
    {
        if (DisclosureHovered)
            ExpansionEnabled = !ExpansionEnabled;
        else if (PresenceHovered)
            PresenceEnabled = !PresenceEnabled;
        else if (SelectionExtended)
            Selected = !Selected;
        else
            Selected = true;

        Verdict.ReadingAltered = true;
    }

    Interaction->DeclareHovered(Target, RowHovered, HoverDuration);
    Interaction->DeclareTaken(Target, Selected, TakeDuration);

    const float HoverFraction = Interaction->HoveredFraction(Target);
    const float SelectionFraction = Interaction->TakenFraction(Target);

    if (SelectionFraction > 0.0f)
    {
        Recording->Ground(Extent, Blend(TileGround, AccentSoftColour, SelectionFraction), 5.0f, CornerAll);
        Recording->Ground(Spanning(Extent.MinimumX, Extent.MinimumY,
                                   Between(0.0f, 2.0f, SelectionFraction), Extent.Height()), AccentColour,
                          1.0f, CornerAll);
    }
    else if (HoverFraction > 0.0f)
    {
        Recording->Ground(Extent, Blend(TileGround, TileHovered, HoverFraction), 5.0f, CornerAll);
    }

    if (DropPlacement == OutlineDropPlacement::Before)
    {
        Recording->Ground(Spanning(Extent.MinimumX, Extent.MinimumY,
                                   Extent.Width(), 2.0f), AccentColour, 1.0f, CornerAll);
    }
    else if (DropPlacement == OutlineDropPlacement::After)
    {
        Recording->Ground(Spanning(Extent.MinimumX, Extent.MaximumY - 2.0f,
                                   Extent.Width(), 2.0f), AccentColour, 1.0f, CornerAll);
    }
    else if (DropPlacement == OutlineDropPlacement::Enclosed)
    {
        Recording->Ground(Extent, AccentSoftColour, 5.0f, CornerAll);
        Recording->Edge(Extent, AccentColour, 1.0f, 5.0f, CornerAll);
    }

    if (Dragged)
        Recording->Edge(Extent, StrongHairColour, 1.0f, 5.0f, CornerAll);

    if (Declared.EnclosedCount > 0u)
    {
        constexpr float QuarterTurn = 1.5707963268f;   // [rad] - 90 degrees
        const float Turn = -(1.0f - ExpansionFraction) * QuarterTurn;
        Recording->Stroke(SymbolSubject::ChevronDown,
                          Spanning(Extent.MinimumX + Indent, Extent.MinimumY + 7.0f, 12.0f, 12.0f),
                          FaintColour, Turn);
    }

    Recording->Stroke(SymbolSubject::PlaceholderMark,
                      Spanning(Extent.MinimumX + Indent + 17.0f, Extent.MinimumY + 5.0f, 16.0f, 16.0f),
                      PresenceEnabled ? AccentColour : FaintColour);
    Recording->TextRunTruncated(Extent.MinimumX + Indent + 39.0f, CentredY(Extent, ReferenceText),
                                Extent.MaximumX - 34.0f, PresenceEnabled ? PrimaryColour : FaintColour,
                                Declared.Caption, ReferenceText, Selected);

    Recording->Edge(PresenceExtent, PresenceHovered ? StrongHairColour : HairColour, 1.0f, 5.0f, CornerAll);
    Recording->Medallion(PresenceExtent.MinimumX + 11.0f, PresenceExtent.MinimumY + 11.0f,
                         PresenceEnabled ? 3.0f : 1.5f,
                         PresenceEnabled ? MutedColour : AbsentColour);

    Verdict.ContactTaken = Interaction->Holding(Target);
    Verdict.Mark         = RowHovered ? RedrawMark::Recolour : RedrawMark::Quiet;

    return Verdict;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      REVISION MARKER
//------------------------------------------------------------------------------------------------------------------------

void ControlPanel::RevisionRow(const PlaneExtent& Extent, const RevisionDeclaration& Declared, bool Taken)
{
    if (Recording == nullptr)
        return;

    const float MarkerX = Extent.MinimumX + 12.0f;
    Recording->Ground(Spanning(MarkerX - 0.5f, Extent.MinimumY, 1.0f, Extent.Height()), HairColour,
                      0.0f, CornerNone);
    Recording->Medallion(MarkerX, Extent.MinimumY + Extent.Height() * 0.5f, Taken ? 5.0f : 4.0f,
                         Taken ? AccentColour : FaintColour);

    const PlaneExtent Card = { Extent.MinimumX + 28.0f, Extent.MinimumY + 3.0f,
                               Extent.MaximumX, Extent.MaximumY - 3.0f };
    Recording->Ground(Card, Taken ? AccentSoftColour : TileGround, 7.0f, CornerAll);
    Recording->Edge(Card, Taken ? AccentColour : HairColour, 1.0f, 7.0f, CornerAll);
    Recording->TextRunTruncated(Card.MinimumX + 9.0f, Card.MinimumY + 7.0f, Card.MaximumX - 58.0f,
                                PrimaryColour, Declared.Description, ReferenceText, true);
    Recording->TextRunTruncated(Card.MinimumX + 9.0f, Card.MinimumY + 24.0f, Card.MaximumX - 58.0f,
                                MutedColour, Declared.Secondary, SmallText);

    const float TimeX = Recording->MeasureRun(Declared.TimeRun, SmallText);
    Recording->TextRun(Card.MaximumX - TimeX - 8.0f, Card.MinimumY + 7.0f,
                       FaintColour, Declared.TimeRun, SmallText);
}

void ControlPanel::Reset()
{
    Interaction = nullptr;
    Recording   = nullptr;
    Appearance  = nullptr;
    Sampled     = {};
}

}   // namespace Slate
