//============================================================================================================================================
//                                                           CHANNELPANEL.CPP
//============================================================================================================================================
// 🧩 The twenty channels as three sections — consumed, retained and undeclared — over an API that already exists.

#include "SlateUI/Interface/ChannelPanel/Api/ChannelPanel.h"

#include <cstdio>
#include <cstring>

// 📝 🔴 No vendor spelling appears in this file and none may. Every fill, outline, run of text and clip goes through
//    `ControlPanel`'s painting seam, which is the one component that knows which recording the interface paints on.

namespace Slate
{
namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE REFERENCE GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

// 📝 What this panel spells that the theme does not. Everything the theme already carries is read from
//    `LayoutExtents` and is not repeated.
constexpr float ChannelRowHeight = 30.0f;   // [px] - one channel's own row, before its editor row
constexpr float NoticeBandHeight = 24.0f;   // [px] - the refusal band above the footer
constexpr float BadgeInset       =  6.0f;   // [px] - inset of a class badge inside its pill

constexpr std::uint32_t RowTextExtent = 96u;   // [-] - the readout buffer every row prints into

constexpr std::uint32_t ChannelSpan = static_cast<std::uint32_t>(ChannelSubject::ChannelCount);

// 📝 The three sections, in presented order. Consumed first because it is what the artist is editing; undeclared
//    last and closed, because twenty rows of "not declared" is a wall that hides the two rows that matter.
constexpr std::uint32_t SectionConsumed   = 0u;
constexpr std::uint32_t SectionRetained   = 1u;
constexpr std::uint32_t SectionUndeclared = 2u;

//------------------------------------------------------------------------------------------------------------------------
//                                                      SMALL GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

WorkspaceRectangle BandOf(const WorkspaceRectangle& Area, float Offset, float Height)
{
    return { Area.PositionX, Area.PositionY + Offset, Area.Width, Height };
}

WorkspaceRectangle InsetBy(const WorkspaceRectangle& Area, float Margin)
{
    return { Area.PositionX + Margin,
             Area.PositionY + Margin,
             Area.Width  - Margin * 2.0f > 0.0f ? Area.Width  - Margin * 2.0f : 0.0f,
             Area.Height - Margin * 2.0f > 0.0f ? Area.Height - Margin * 2.0f : 0.0f };
}

WorkspaceRectangle LeftOf(const WorkspaceRectangle& Area, float Width)
{
    return { Area.PositionX, Area.PositionY, Width < Area.Width ? Width : Area.Width, Area.Height };
}

WorkspaceRectangle RightOf(const WorkspaceRectangle& Area, float Width)
{
    const float Taken = Width < Area.Width ? Width : Area.Width;

    return { Area.PositionX + Area.Width - Taken, Area.PositionY, Taken, Area.Height };
}

WorkspaceRectangle SquareCentred(const WorkspaceRectangle& Area, float Edge)
{
    return { Area.PositionX + (Area.Width  - Edge) * 0.5f,
             Area.PositionY + (Area.Height - Edge) * 0.5f,
             Edge,
             Edge };
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE NOTICE
//------------------------------------------------------------------------------------------------------------------------

// 📝 🔴 A refusal is presented and never swallowed. `42` §2 and `10` §2.2 both put the reason in static text so
//    the artist can act on it; a panel that dropped it would leave a slider that visibly moves and a material
//    that visibly does not, with nothing on screen to explain the difference.
void RecordNotice(ChannelPanelCarry& Carry, const char* Reason)
{
    if (Reason == nullptr || Reason[0] == '\0')
        return;

    std::snprintf(Carry.Notice, ChannelNoticeExtent, "%s", Reason);

    Carry.NoticeDeclared = true;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE BANDS
//------------------------------------------------------------------------------------------------------------------------

void PresentHeaderBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       const char*                MaterialName,
                       ReflectanceSelection       Selected)
{
    PresentSurfaceFill(Band, Theme.Palette.PanelHeader, 0.0f);
    PresentSurfaceFill({ Band.PositionX, Band.PositionY + Band.Height - Theme.Extents.BorderThickness,
                         Band.Width, Theme.Extents.BorderThickness },
                       Theme.Palette.PanelBorder, 0.0f);

    const WorkspaceRectangle Interior = InsetBy(Band, Theme.Extents.PanelPadding);
    const WorkspaceRectangle GlyphBox = LeftOf(Interior, 24.0f);

    PresentSurfaceFill(GlyphBox, Theme.Palette.TileBackground, Theme.Extents.PillRounding);
    PresentControlStroke(SquareCentred(GlyphBox, Theme.Extents.GlyphEdge),
                         ControlStroke::Circle, Theme.Palette.TextPrimary, 1.5f, 0.0f);

    WorkspaceRectangle Titles = Interior;

    Titles.PositionX += 24.0f + Theme.Extents.ControlSpacing;
    Titles.Width     -= 24.0f + Theme.Extents.ControlSpacing;

    const WorkspaceRectangle TitleRun    = { Titles.PositionX, Titles.PositionY,
                                             Titles.Width, Titles.Height * 0.55f };
    const WorkspaceRectangle SubtitleRun = { Titles.PositionX, Titles.PositionY + Titles.Height * 0.55f,
                                             Titles.Width, Titles.Height * 0.45f };

    // 📝 The material's own name, from the ledger. Empty is presented as empty and never as a minted stand-in —
    //    an artist who left a material unnamed should see that, not a name the panel invented for them.
    PresentTextRun(TitleRun,
                   MaterialName != nullptr && MaterialName[0] != '\0' ? MaterialName : "Unnamed material",
                   Theme.Palette.TextPrimary, TextPlacement::Leading, 1.0f);
    PresentTextRun(SubtitleRun, CaptionOfReflectance(Selected),
                   Theme.Palette.TextMuted, TextPlacement::Leading, 0.85f);
}

void PresentNoticeBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       ChannelPanelCarry&         Carry)
{
    PresentSurfaceFill(Band, Attenuate(Theme.Palette.DangerPrimary, 0.14), Theme.Extents.PillRounding);

    const WorkspaceRectangle Interior = InsetBy(Band, BadgeInset * 0.5f);
    const WorkspaceRectangle Dismiss  = RightOf(Interior, Theme.Extents.GlyphButtonSmallEdge);

    PresentControlStroke(SquareCentred(LeftOf(Interior, Theme.Extents.GlyphButtonSmallEdge),
                                       Theme.Extents.GlyphEdge),
                         ControlStroke::Cross, Theme.Palette.DangerPrimary, 1.4f, 0.0f);

    WorkspaceRectangle Lettering = Interior;

    Lettering.PositionX += Theme.Extents.GlyphButtonSmallEdge + Theme.Extents.ControlSpacing;
    Lettering.Width     -= Theme.Extents.GlyphButtonSmallEdge * 2.0f + Theme.Extents.ControlSpacing;

    PresentTextRun(Lettering, Carry.Notice, Theme.Palette.TextPrimary, TextPlacement::Leading, 0.85f);

    if (ResolveAreaPress(Dismiss).EditSealed)
        Carry.NoticeDeclared = false;

    PresentControlStroke(SquareCentred(Dismiss, Theme.Extents.GlyphEdge), ControlStroke::Cross,
                         Theme.Palette.TextMuted, 1.4f, 0.0f);
}

void PresentFooterBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       std::uint32_t              SampledCount,
                       std::uint32_t              RetainedCount,
                       bool                       CutoutEnrolled)
{
    PresentSurfaceFill(Band, Theme.Palette.PanelHeader, 0.0f);
    PresentSurfaceFill({ Band.PositionX, Band.PositionY, Band.Width, Theme.Extents.BorderThickness },
                       Theme.Palette.PanelBorder, 0.0f);

    char Counted[RowTextExtent] = {};

    std::snprintf(Counted, RowTextExtent, "%u sampled \xC2\xB7 %u retained \xC2\xB7 %s",
                  SampledCount, RetainedCount, CutoutEnrolled ? "cutout" : "opaque");

    PresentTextRun(InsetBy(Band, Theme.Extents.PanelPadding), Counted,
                   Theme.Palette.TextMuted, TextPlacement::Leading, 0.9f);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      ONE CHANNEL ROW
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What one row's tick asked the material for, applied after the whole run has been walked.
/// note  🔴 Deferred for the same reason `LayerPanel`'s is: `DeclareChannel` amends the very declarations the walk
///        is reading, and amending inside the walk would present half a tick against one reading and half against
///        another.
struct ChannelIntent
{
    bool                  ChannelDeclared = false;                         // [-] - a row asked for an amendment
    ChannelSubject        Subject         = ChannelSubject::AlbedoColour;  // [-] - which channel it names
    ChannelSpecification  Declaring       = {};                            // [-] - the whole amended declaration
};

// 🧩 The class badge a row carries — sampled, retained or undeclared, read and never inferred.
void PresentClassBadge(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Area,
                       const char*                Caption,
                       const ThemeColour&         Wash)
{
    const float              Width = MeasuredTextExtent(Caption, 0.8f) + BadgeInset * 2.0f;
    const WorkspaceRectangle Badge = RightOf(Area, Width);

    PresentSurfaceFill(Badge, Wash, Theme.Extents.EntryRounding);
    PresentTextRun(Badge, Caption, Theme.Palette.TextPrimary, TextPlacement::Centred, 0.8f);
}

void PresentChannelRow(const ThemeSpecification&    Theme,
                       const WorkspaceRectangle&    Row,
                       ChannelSubject               Subject,
                       const MaterialSpecification& Material,
                       ChannelPanelCarry&           Carry,
                       ChannelIntent&               Arriving)
{
    const ChannelSpecification& Standing = Material.Channel(Subject);
    const bool                  Sampled  = Material.ChannelSampled(Subject);

    const ControlInteraction RowPress = ResolveAreaPress(Row);

    PresentSurfaceFill(Row,
                       RowPress.PointerOver ? Theme.Palette.RowHovered : Theme.Palette.TileBackground,
                       Theme.Extents.PillRounding);

    WorkspaceRectangle Interior = InsetBy(Row, Theme.Extents.ControlSpacing * 0.5f);

    // 📝 The name reads in the muted colour where the channel is not sampled. A dispatch does not read it —
    //    `18` §9 — and presenting it at full weight would put an unread channel beside a read one at equal
    //    prominence, which is the reading the artist then acts on.
    PresentTextRun(LeftOf(Interior, Theme.Extents.LabelColumnWidth * 1.4f),
                   CaptionOfChannel(Subject),
                   Sampled ? Theme.Palette.TextPrimary : Theme.Palette.TextMuted,
                   TextPlacement::Leading, 1.0f);

    // 📝 The source and the measure are printed rather than offered as controls. Amending either is a material
    //    edit `42` admits only as a whole `ChannelSpecification`, and the two that an artist changes by hand —
    //    the constant and the interval — are the editor row beneath. A dropdown here would suggest the panel can
    //    re-source a channel, which `50` and `56` own and this panel does not.
    char Reading[RowTextExtent] = {};

    std::snprintf(Reading, RowTextExtent, "%s \xC2\xB7 %s",
                  CaptionOfChannelSource(Standing.Source),
                  CaptionOfChannelMeasure(Standing.Measured));

    WorkspaceRectangle Reads = Interior;

    Reads.PositionX += Theme.Extents.LabelColumnWidth * 1.4f;
    Reads.Width     -= Theme.Extents.LabelColumnWidth * 1.4f;

    PresentTextRun(Reads, Reading, Theme.Palette.TextMuted, TextPlacement::Leading, 0.8f);

    if (!Standing.ChannelDeclared)
    {
        PresentClassBadge(Theme, Interior, "undeclared", Theme.Palette.ControlBackground);
        return;
    }

    if (!Sampled)
        PresentClassBadge(Theme, Interior, "retained", Attenuate(Theme.Palette.AccentPrimary, 0.18));
    else if (Material.ChannelConverted(Subject))
        PresentClassBadge(Theme, Interior, "converted", Attenuate(Theme.Palette.SelectionMarker, 0.18));
}

// 🧩 The editor beneath a declared constant channel — a slider at a scalar measure, a colour bar at a colour one.
// out  true where the row amended the declaration
bool PresentChannelEditor(const ThemeSpecification&    Theme,
                          const WorkspaceRectangle&    Row,
                          ChannelSubject               Subject,
                          const MaterialSpecification& Material,
                          ChannelPanelCarry&           Carry,
                          ChannelIntent&               Arriving)
{
    const ChannelSpecification& Standing = Material.Channel(Subject);

    if (Standing.Source != ChannelSource::Constant)
        return false;

    // 📝 🔴 The declaration is copied into a local, amended, and handed back whole through `DeclareChannel`. That
    //    is not the panel holding what it presents: the copy lives for one call and the material validates it
    //    before it lands. `42` admits a channel only as a whole specification, so there is no narrower amendment
    //    to make — and a refused one leaves the material exactly as it was.
    ChannelSpecification Amending = Standing;

    if (MeasureCarriesColour(Standing.Measured))
    {
        const std::size_t Ordinal = static_cast<std::size_t>(Subject);

        ThemeColour Carried;

        Carried.Coordinate = Amending.ConstantColour;
        Carried.Coverage   = 1.0;

        const Outcome<ControlInteraction> Reported =
            PresentColourEntry(Theme, Row, "constant", Carried, Carry.PickerOpen[Ordinal]);

        if (!Reported.ContentPresent)
            return false;

        if (Reported.Resolve().EditDeclared)
        {
            // 📝 🔴 The coordinate is written back and the space is carried through untouched. `36` §1 and
            //    `14` §5: no transfer is spelled anywhere in `SlateUI`, and a projection here would be the second
            //    one the whole arrangement exists to prevent.
            Amending.ConstantColour = Carried.Coordinate;

            Arriving.ChannelDeclared = true;
            Arriving.Subject         = Subject;
            Arriving.Declaring       = Amending;

            return true;
        }

        return false;
    }

    // 📝 A slider over the channel's **declared** interval and never over a presumed unit one. `18` §2's channels
    //    are not all bounded to one — a thickness is millimetres — and a slider that assumed otherwise would
    //    refuse every value the material considers valid.
    double Carried = Amending.ConstantScalar;

    const Outcome<ControlInteraction> Reported =
        PresentValueSlider(Theme, Row, "constant", Carried,
                           Amending.LowerMagnitude, Amending.UpperMagnitude, "\xC2\xB7", 3u);

    if (!Reported.ContentPresent)
    {
        // 📝 A refused slider is presented as its reason rather than as a control that silently did not appear.
        //    `Floor == Ceiling` is the ordinary case here — a channel whose interval was never widened — and the
        //    artist needs to be told that, not left with a blank row.
        RecordNotice(Carry, Reported.Declined.Detail);
        return false;
    }

    if (Reported.Resolve().EditDeclared)
    {
        Amending.ConstantScalar = Carried;

        Arriving.ChannelDeclared = true;
        Arriving.Subject         = Subject;
        Arriving.Declaring       = Amending;

        return true;
    }

    return false;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT A ROW READS
//------------------------------------------------------------------------------------------------------------------------

const char* CaptionOfChannel(ChannelSubject Channel)
{
    switch (Channel)
    {
        case ChannelSubject::AlbedoColour:               return "Albedo";
        case ChannelSubject::Metallic:                   return "Metallic";
        case ChannelSubject::Roughness:                  return "Roughness";
        case ChannelSubject::NormalIncidenceReflectance: return "Normal reflectance";
        case ChannelSubject::SurfaceOrientation:         return "Orientation";
        case ChannelSubject::AmbientOcclusion:           return "Occlusion";
        case ChannelSubject::Emission:                   return "Emission";
        case ChannelSubject::Opacity:                    return "Opacity";
        case ChannelSubject::Anisotropy:                 return "Anisotropy";
        case ChannelSubject::AnisotropyDirection:        return "Anisotropy direction";
        case ChannelSubject::ClearCoat:                  return "Clear coat";
        case ChannelSubject::ClearCoatRoughness:         return "Clear coat roughness";
        case ChannelSubject::ClearCoatOrientation:       return "Clear coat orientation";
        case ChannelSubject::SheenColour:                return "Sheen";
        case ChannelSubject::SheenRoughness:             return "Sheen roughness";
        case ChannelSubject::SubsurfaceColour:           return "Subsurface";
        case ChannelSubject::SubsurfaceThickness:        return "Subsurface thickness";
        case ChannelSubject::Transmission:               return "Transmission";
        case ChannelSubject::RefractionRatio:            return "Refraction ratio";
        case ChannelSubject::Displacement:               return "Displacement";
        default:                                         return "Unknown";
    }
}

const char* CaptionOfReflectance(ReflectanceSelection Selected)
{
    switch (Selected)
    {
        case ReflectanceSelection::Standard:     return "Standard";
        case ReflectanceSelection::Anisotropic:  return "Anisotropic";
        case ReflectanceSelection::ClearCoated:  return "Clear coated";
        case ReflectanceSelection::Cloth:        return "Cloth";
        case ReflectanceSelection::Subsurface:   return "Subsurface";
        case ReflectanceSelection::Transmissive: return "Transmissive";
        case ReflectanceSelection::EmissiveOnly: return "Emissive only";
        case ReflectanceSelection::Unlit:        return "Unlit";
        default:                                 return "Unknown";
    }
}

const char* CaptionOfChannelSource(ChannelSource Source)
{
    switch (Source)
    {
        case ChannelSource::Constant: return "Constant";
        case ChannelSource::Layered:  return "Layered";
        case ChannelSource::Analytic: return "Analytic";
        case ChannelSource::Imported: return "Imported";
        case ChannelSource::Absent:   return "Absent";
        default:                      return "Unknown";
    }
}

const char* CaptionOfChannelMeasure(ChannelMeasure Measured)
{
    switch (Measured)
    {
        case ChannelMeasure::Reflectance: return "reflectance";
        case ChannelMeasure::Emission:    return "emission";
        case ChannelMeasure::Scalar:      return "scalar";
        case ChannelMeasure::Direction:   return "direction";
        case ChannelMeasure::Enrolment:   return "enrolment";
        default:                          return "unknown";
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

void PresentChannelPanel(const ThemeSpecification&  Theme,
                         const WorkspaceRectangle&  Area,
                         void*                      PresentContext)
{
    ChannelPanelContext* Standing = static_cast<ChannelPanelContext*>(PresentContext);

    PresentSurfaceFill(Area, Theme.Palette.PanelBackground, Theme.Extents.CornerRounding);

    if (Standing == nullptr || Standing->Materials == nullptr || Standing->Carry == nullptr)
    {
        PresentTextRun(Area, "No material", Theme.Palette.TextMuted, TextPlacement::Centred, 1.0f);
        return;
    }

    MaterialIndex&     Materials = *Standing->Materials;
    ChannelPanelCarry& Carry     = *Standing->Carry;

    ++Carry.PresentedTicks;

    // 📝 🔴 Resolved fresh every tick, and the refusal is presented rather than assumed away. A material withdrawn
    //    beneath an open panel is the ordinary case in a session, and the panel that held a pointer to it instead
    //    would present twenty channels of released storage.
    const Outcome<const MaterialSpecification*> Resolved = Materials.Resolve(Standing->MaterialOrdinal);

    if (!Resolved.ContentPresent || Resolved.Resolve() == nullptr)
    {
        PresentTextRun(Area, "No material", Theme.Palette.TextMuted, TextPlacement::Centred, 1.0f);
        return;
    }

    const MaterialSpecification& Material = *Resolved.Resolve();
    const ReflectanceSelection   Selected = Material.Reflectance();

    //----------------------------------------------------------------------------------------------------------------
    // the bands
    //----------------------------------------------------------------------------------------------------------------

    const float NoticeHeight = Carry.NoticeDeclared ? NoticeBandHeight + Theme.Extents.ControlSpacing : 0.0f;

    const WorkspaceRectangle HeaderBand = BandOf(Area, 0.0f, Theme.Extents.PanelHeaderHeight);
    const WorkspaceRectangle FooterBand = BandOf(Area, Area.Height - Theme.Extents.PanelFooterHeight,
                                                 Theme.Extents.PanelFooterHeight);

    std::uint32_t SampledCount  = 0u;
    std::uint32_t RetainedCount = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < ChannelSpan; ++Ordinal)
    {
        const ChannelSubject Subject = static_cast<ChannelSubject>(Ordinal);

        if (Material.ChannelSampled(Subject))
            ++SampledCount;
        else if (Material.Channel(Subject).ChannelDeclared)
            ++RetainedCount;
    }

    // 📝 The name arrives as a `std::string` the index owns and outlives this call, so the header band is handed
    //    its characters directly rather than a copy made to satisfy the signature.
    PresentHeaderBand(Theme, HeaderBand, Materials.DeclaredName(Standing->MaterialOrdinal).c_str(), Selected);

    if (Carry.NoticeDeclared)
    {
        PresentNoticeBand(Theme,
                          { Area.PositionX + Theme.Extents.PanelPadding,
                            Area.PositionY + Theme.Extents.PanelHeaderHeight + Theme.Extents.ControlSpacing,
                            Area.Width - Theme.Extents.PanelPadding * 2.0f,
                            NoticeBandHeight },
                          Carry);
    }

    const WorkspaceRectangle ListBand =
        { Area.PositionX,
          Area.PositionY + Theme.Extents.PanelHeaderHeight + NoticeHeight,
          Area.Width,
          Area.Height - Theme.Extents.PanelHeaderHeight - NoticeHeight - Theme.Extents.PanelFooterHeight };

    if (ListBand.Height <= 0.0f)
    {
        PresentFooterBand(Theme, FooterBand, SampledCount, RetainedCount, Material.CutoutEnrolled());
        return;
    }

    //----------------------------------------------------------------------------------------------------------------
    // the three sections
    //----------------------------------------------------------------------------------------------------------------

    // 📝 The content span is measured before the walk so the wheel is bounded against what is actually there. A
    //    span measured after would bound this tick's wheel against last tick's content.
    float ContentSpan = 0.0f;

    for (std::uint32_t Section = 0u; Section < ChannelSectionCount; ++Section)
    {
        ContentSpan += Theme.Extents.SectionHeaderHeight + Theme.Extents.ControlSpacing;

        if (!Carry.SectionOpen[Section])
            continue;

        for (std::uint32_t Ordinal = 0u; Ordinal < ChannelSpan; ++Ordinal)
        {
            const ChannelSubject        Subject  = static_cast<ChannelSubject>(Ordinal);
            const ChannelSpecification& Declared = Material.Channel(Subject);
            const bool                  Sampled  = Material.ChannelSampled(Subject);

            const bool Belongs = Section == SectionConsumed   ? Sampled
                               : Section == SectionRetained   ? (!Sampled && Declared.ChannelDeclared)
                                                              : !Declared.ChannelDeclared;

            if (!Belongs)
                continue;

            ContentSpan += ChannelRowHeight + Theme.Extents.CardGap;

            if (Section != SectionUndeclared && Declared.Source == ChannelSource::Constant)
                ContentSpan += Theme.Extents.EntryRowHeight + Theme.Extents.CardGap;
        }
    }

    AdvanceVisibleOffset(Carry.VisibleOffset, ListBand, ContentSpan);

    ChannelIntent Arriving = {};

    DeclareClip(ListBand);

    float Walking = ListBand.PositionY - Carry.VisibleOffset;

    static const char* const SectionCaptions[ChannelSectionCount] =
        { "Sampled", "Retained", "Undeclared" };

    for (std::uint32_t Section = 0u; Section < ChannelSectionCount; ++Section)
    {
        char Counted[RowTextExtent] = {};

        std::uint32_t Belonging = 0u;

        for (std::uint32_t Ordinal = 0u; Ordinal < ChannelSpan; ++Ordinal)
        {
            const ChannelSubject        Subject  = static_cast<ChannelSubject>(Ordinal);
            const ChannelSpecification& Declared = Material.Channel(Subject);
            const bool                  Sampled  = Material.ChannelSampled(Subject);

            Belonging += (Section == SectionConsumed ? Sampled
                        : Section == SectionRetained ? (!Sampled && Declared.ChannelDeclared)
                                                     : !Declared.ChannelDeclared) ? 1u : 0u;
        }

        std::snprintf(Counted, RowTextExtent, "%u", Belonging);

        PresentSectionHeader(Theme,
                             { ListBand.PositionX + Theme.Extents.PanelPadding, Walking,
                               ListBand.Width - Theme.Extents.PanelPadding * 2.0f,
                               Theme.Extents.SectionHeaderHeight },
                             SectionCaptions[Section], Carry.SectionOpen[Section], Counted);

        Walking += Theme.Extents.SectionHeaderHeight + Theme.Extents.ControlSpacing;

        if (!Carry.SectionOpen[Section])
            continue;

        for (std::uint32_t Ordinal = 0u; Ordinal < ChannelSpan; ++Ordinal)
        {
            const ChannelSubject        Subject  = static_cast<ChannelSubject>(Ordinal);
            const ChannelSpecification& Declared = Material.Channel(Subject);
            const bool                  Sampled  = Material.ChannelSampled(Subject);

            const bool Belongs = Section == SectionConsumed   ? Sampled
                               : Section == SectionRetained   ? (!Sampled && Declared.ChannelDeclared)
                                                              : !Declared.ChannelDeclared;

            if (!Belongs)
                continue;

            const WorkspaceRectangle Row = { ListBand.PositionX + Theme.Extents.PanelPadding, Walking,
                                             ListBand.Width - Theme.Extents.PanelPadding * 2.0f,
                                             ChannelRowHeight };

            PresentChannelRow(Theme, Row, Subject, Material, Carry, Arriving);

            Walking += ChannelRowHeight + Theme.Extents.CardGap;

            if (Section != SectionUndeclared && Declared.Source == ChannelSource::Constant)
            {
                PresentChannelEditor(Theme,
                                     { Row.PositionX + Theme.Extents.IndentWidth, Walking,
                                       Row.Width - Theme.Extents.IndentWidth,
                                       Theme.Extents.EntryRowHeight },
                                     Subject, Material, Carry, Arriving);

                Walking += Theme.Extents.EntryRowHeight + Theme.Extents.CardGap;
            }
        }
    }

    ReclaimClip();

    //----------------------------------------------------------------------------------------------------------------
    // the deferred intent
    //----------------------------------------------------------------------------------------------------------------

    if (Arriving.ChannelDeclared)
    {
        const Outcome<MaterialSpecification*> Amendable = Materials.Amend(Standing->MaterialOrdinal);

        if (!Amendable.ContentPresent || Amendable.Resolve() == nullptr)
        {
            RecordNotice(Carry, Amendable.ContentPresent ? "the material could not be amended"
                                                         : Amendable.Declined.Detail);
        }
        else
        {
            // 📝 🔴 The material validates the amendment and refuses it whole. The panel does not pre-validate and
            //    does not correct — `10` §2.2 puts validation in the declaration, and a panel that bounded the
            //    value first would hide from the artist that their reading was out of bounds at all.
            const Outcome<bool> Landed = Amendable.Resolve()->DeclareChannel(Arriving.Subject, Arriving.Declaring);

            if (!Landed.ContentPresent)
                RecordNotice(Carry, Landed.Declined.Detail);
        }
    }

    PresentFooterBand(Theme, FooterBand, SampledCount, RetainedCount, Material.CutoutEnrolled());
}

}   // namespace Slate
