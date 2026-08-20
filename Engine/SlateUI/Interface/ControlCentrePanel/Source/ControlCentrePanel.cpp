//============================================================================================================================================
//                                                      CONTROLCENTREPANEL.CPP
//============================================================================================================================================
// 🧩 Every rendered route of the notch Control Centre, using the default
// ImGui typeface and placeholder symbol.

#include "SlateUI/Interface/ControlCentrePanel/Api/ControlCentrePanel.h"

#include "SlateUI/Interface/SymbolSpecification/Api/SymbolSpecification.h"

#include <cmath>
#include <cstdio>

namespace Slate
{

namespace
{

constexpr ThemeToken White = Covering(0xFFFFFFu);
constexpr ThemeToken Black = Covering(0x000000u);
constexpr ThemeToken QuietDark = Partial(0xFFFFFFu, .08);
constexpr ThemeToken QuietLight = Partial(0x000000u, .08);
constexpr float PagePad = 32.0f;
constexpr float HeaderHeight = 64.0f;
constexpr float CardGap = 16.0f;
constexpr float RowHeight = 76.0f;
constexpr float TileHeight = 130.0f;
constexpr float DragDuration = 300.0f;

float CentreText(RecordingSurface& Surface, const PlaneExtent& Extent, const char* Text, float Size)
{
    return Extent.MinimumX + (Extent.Width() - Surface.MeasureRun(Text, Size)) * 0.5f;
}

float CentredY(const PlaneExtent& Extent, float Size)
{
    return Extent.MinimumY + (Extent.Height() - Size) * 0.5f;
}

std::uint32_t WrappedText(RecordingSurface& Surface, const PlaneExtent& Extent, ThemeToken Colour,
                          const char* Text, float Size, bool Record)
{
    char Line[256] = {};
    std::uint32_t LineLength = 0u;
    std::uint32_t LineCount = 0u;
    std::uint32_t Cursor = 0u;

    while (Text[Cursor] != '\0')
    {
        while (Text[Cursor] == ' ') ++Cursor;
        const std::uint32_t WordBegin = Cursor;
        while (Text[Cursor] != '\0' && Text[Cursor] != ' ') ++Cursor;
        const std::uint32_t WordLength = Cursor - WordBegin;
        if (WordLength == 0u) break;

        char Candidate[256] = {};
        std::uint32_t CandidateLength = 0u;
        for (std::uint32_t Ordinal = 0u; Ordinal < LineLength && CandidateLength + 1u < 256u; ++Ordinal)
            Candidate[CandidateLength++] = Line[Ordinal];
        if (CandidateLength > 0u && CandidateLength + 1u < 256u) Candidate[CandidateLength++] = ' ';
        for (std::uint32_t Ordinal = 0u; Ordinal < WordLength && CandidateLength + 1u < 256u; ++Ordinal)
            Candidate[CandidateLength++] = Text[WordBegin + Ordinal];
        Candidate[CandidateLength] = '\0';

        if (LineLength > 0u && Surface.MeasureRun(Candidate, Size) > Extent.Width())
        {
            if (Record)
                Surface.TextRunTruncated(Extent.MinimumX,
                                         Extent.MinimumY + static_cast<float>(LineCount) * (Size + 4.0f),
                                         Extent.MaximumX, Colour, Line, Size);
            ++LineCount;
            LineLength = 0u;
        }

        if (LineLength > 0u && LineLength + 1u < 256u) Line[LineLength++] = ' ';
        for (std::uint32_t Ordinal = 0u; Ordinal < WordLength && LineLength + 1u < 256u; ++Ordinal)
            Line[LineLength++] = Text[WordBegin + Ordinal];
        Line[LineLength] = '\0';
    }

    if (LineLength > 0u)
    {
        if (Record)
            Surface.TextRunTruncated(Extent.MinimumX,
                                     Extent.MinimumY + static_cast<float>(LineCount) * (Size + 4.0f),
                                     Extent.MaximumX, Colour, Line, Size);
        ++LineCount;
    }
    return LineCount;
}

ThemeToken WithOpacity(ThemeToken Colour, float Fraction)
{
    Colour.Opacity = static_cast<std::uint8_t>(static_cast<float>(Colour.Opacity) * Fraction + .5f);
    return Colour;
}

ThemeToken Between(ThemeToken From, ThemeToken To, float Fraction)
{
    const auto Mix = [Fraction](std::uint8_t First, std::uint8_t Second)
    {
        return static_cast<std::uint8_t>(static_cast<float>(First) +
                                         (static_cast<float>(Second) - static_cast<float>(First)) * Fraction + .5f);
    };
    return {Mix(From.Red, To.Red), Mix(From.Green, To.Green), Mix(From.Blue, To.Blue), Mix(From.Opacity, To.Opacity)};
}

const char* PageCaption(ControlCentrePage Page)
{
    switch (Page)
    {
    case ControlCentrePage::Settings:
        return "Settings";
    case ControlCentrePage::Notifications:
        return "Apps & Notifications";
    case ControlCentrePage::Display:
        return "Display Settings";
    case ControlCentrePage::Input:
        return "Input Devices";
    default:
        return "Control Center";
    }
}

// 📝 The live per-role weight, read straight from the configuration the artist writes. The panel never
//    reads `ThemeProfile::Fonts` for its own chrome, because that copy is refreshed by the host only when a
//    display factor or the theme moves — the strip's choice must land on the same tick as the press.
FontWeight RoleWeightOf(const std::uint32_t (&Weights)[8], std::uint32_t Role)
{
    return static_cast<FontWeight>(Weights[Role < 8u ? Role : 0u]);
}

// 📝 The weight faces every role strip offers, in ascending order. `FontLoader::Face` falls back to the
//    regular face when the selected family lacks the requested weight, so a strip never offers a tile that
//    cannot draw — the ones the family does not carry are skipped, exactly as the main family carousel
//    skips nothing because every family carries a regular face.
constexpr FontWeight CandidateFaces[] = {FontWeight::Thin,    FontWeight::ExtraLight, FontWeight::Light,
                                         FontWeight::Regular, FontWeight::Medium,    FontWeight::Semibold,
                                         FontWeight::Bold,    FontWeight::ExtraBold, FontWeight::Black};
constexpr const char* FaceNames[] = {"Thin",  "ExtraLight", "Light",   "Regular", "Medium",
                                     "Semibold", "Bold",    "ExtraBold", "Black"};

// 📝 Control ordinals for the eight per-role family strips. Each strip owns ten: two arrows and up to eight
//    pressable tile positions. They sit above the panel's historical ceiling of 192, which is why the
//    ceiling is 256 — a press past the ceiling was drawn but never registered, and a tile that can never be
//    grabbed is exactly the dead square this page previously presented.
constexpr std::uint32_t RoleArrowBase = 192u;   // [role * 2 + 0] left arrow, [role * 2 + 1] right arrow
constexpr std::uint32_t RoleTileBase = 208u;    // [role * 8 + visible position]
constexpr std::uint32_t RoleTilePositions = 8u;

} // namespace

Outcome<bool> ControlCentrePanel::Construct(MotionIntegrator& IncomingMotion, RecordingSurface& IncomingSurface,
                                            const ThemeProfile& IncomingAppearance)
{
    if (Motion != nullptr)
        return Outcome<bool>::Refuse(
            {RefusalReason::ContentUnsupported, "a Control Centre construction already stands"});

    Motion = &IncomingMotion;
    Surface = &IncomingSurface;
    Appearance = &IncomingAppearance;

    if (!Interaction.Construct(IncomingMotion).Resolved)
        return Outcome<bool>::Refuse(
            {RefusalReason::ExtentExhausted, "the Control Centre interaction index was rejected"});

    if (!SharedControls.Construct(Interaction, IncomingSurface, IncomingAppearance).Resolved)
        return Outcome<bool>::Refuse(
            {RefusalReason::ContentUnsupported, "the shared Control Centre controls were rejected"});

    for (std::uint32_t Ordinal = 0u; Ordinal < ControlCapacity; ++Ordinal)
    {
        const Outcome<ControlIdentity> Registered = Interaction.Register();
        if (!Registered.Resolved) return Outcome<bool>::Refuse(Registered.Error);
        Controls[Ordinal] = Registered.Resolve();
    }

    const Outcome<std::uint32_t> PageRegistered = IncomingMotion.RegisterEased(1.0);
    const Outcome<std::uint32_t> TabRegistered = IncomingMotion.RegisterEased(1.0);
    const Outcome<std::uint32_t> ThemeRegistered = IncomingMotion.RegisterEased(1.0);
    const Outcome<std::uint32_t> FontRegistered = IncomingMotion.RegisterEased(1.0);
    if (!PageRegistered.Resolved || !TabRegistered.Resolved || !ThemeRegistered.Resolved ||
        !FontRegistered.Resolved)
        return Outcome<bool>::Refuse({RefusalReason::ExtentExhausted, "the Control Centre carousel was rejected"});

    PageMotion = PageRegistered.Resolve();
    TabMotion = TabRegistered.Resolve();
    ThemeMotion = ThemeRegistered.Resolve();
    FontMotion = FontRegistered.Resolve();

    for (std::uint32_t Ordinal = 0u;
         Ordinal < static_cast<std::uint32_t>(ControlCentrePage::PageCount); ++Ordinal)
    {
        const Outcome<std::uint32_t> ScrollRegistered = IncomingMotion.RegisterEased(1.0);
        if (!ScrollRegistered.Resolved)
            return Outcome<bool>::Refuse({RefusalReason::ExtentExhausted,
                                          "the Control Centre scroll motion was rejected"});
        ScrollMotion[Ordinal] = ScrollRegistered.Resolve();
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < 8u; ++Ordinal)
    {
        const Outcome<std::uint32_t> RoleRegistered = IncomingMotion.RegisterEased(1.0);
        if (!RoleRegistered.Resolved)
            return Outcome<bool>::Refuse({RefusalReason::ExtentExhausted,
                                          "the typography strip motion was rejected"});
        RoleFontMotion[Ordinal] = RoleRegistered.Resolve();
    }

    return Outcome<bool>::Result(true);
}

void ControlCentrePanel::Advance(const PointerCondition& Sampled, double Elapsed)
{
    Pointer = Sampled;
    Interaction.Advance(Sampled, Elapsed);
    SharedControls.Sample(Sampled);
}

void ControlCentrePanel::RetainExclusion(const PlaneExtent& Extent)
{
    if (ExclusionCount < ControlCapacity)
        Exclusions[ExclusionCount++] = Extent;
}

void ControlCentrePanel::Exclude(DrawerSpace& Drawers) const
{
    for (std::uint32_t Ordinal = 0u; Ordinal < ExclusionCount; ++Ordinal)
        Drawers.Exclude(DrawerBearing::North, Exclusions[Ordinal]);
}

bool ControlCentrePanel::Pressed(std::uint32_t Ordinal, const PlaneExtent& Extent)
{
    if (Ordinal >= ControlCapacity) return false;

    RetainExclusion(Extent);
    const ControlIdentity Target = Controls[Ordinal];
    const bool Hovered = Extent.Encloses(Pointer.PositionX, Pointer.PositionY);
    if (Hovered && Pointer.ContactPressed && !Interaction.AnyDisclosed()) Interaction.Grab(Target, ControlPart::Body);

    Interaction.DeclareHovered(Target, Hovered, 130.0);
    const bool Quick = Hovered && Pointer.ContactPressed && Pointer.ContactReleased;
    return (Interaction.Released(Target) && Hovered) || Quick;
}

bool ControlCentrePanel::Slider(std::uint32_t Ordinal, const PlaneExtent& Extent, std::uint32_t Minimum,
                                std::uint32_t Maximum, std::uint32_t& Reading, const char* UnitGlyph,
                                ThemeToken Rail, ThemeToken Accent)
{
    if (Ordinal >= ControlCapacity || Maximum <= Minimum) return false;

    RetainExclusion(Extent);
    MagnitudeDeclaration Declared;
    Declared.Caption = "";
    Declared.UnitGlyph = UnitGlyph;
    Declared.Minimum = static_cast<double>(Minimum);
    Declared.Maximum = static_cast<double>(Maximum);

    double Coordinate = static_cast<double>(Reading);
    const ControlVerdict Verdict = SharedControls.MagnitudeRow(Controls[Ordinal], Extent, Declared, Coordinate, true);
    Reading = static_cast<std::uint32_t>(std::round(Coordinate));
    static_cast<void>(Rail);
    static_cast<void>(Accent);
    return Verdict.ReadingAltered;
}

void ControlCentrePanel::Toggle(std::uint32_t Ordinal, const PlaneExtent& Extent, bool& Enabled, ThemeToken Quiet,
                                ThemeToken Accent)
{
    if (Pressed(Ordinal, Extent)) Enabled = !Enabled;

    Interaction.DeclareTaken(Controls[Ordinal], Enabled, 150.0);
    const float Fraction = Interaction.TakenFraction(Controls[Ordinal]);
    Surface->Ground(Extent, Enabled ? Accent : Quiet, Extent.Height() * .5f, CornerAll);
    Surface->Medallion(Extent.MinimumX + 12.0f + (Extent.Width() - 24.0f) * Fraction,
                       Extent.MinimumY + Extent.Height() * .5f, 8.0f, White);
}

void ControlCentrePanel::Symbol(const PlaneExtent& Extent, ThemeToken Colour)
{
    Surface->Stroke(SymbolSubject::PlaceholderMark, Extent, Colour);
}

void ControlCentrePanel::Navigate(ControlCentrePage Incoming)
{
    if (Incoming == CurrentPage || Motion == nullptr) return;
    PreviousPage = CurrentPage;
    PageForward = static_cast<std::uint32_t>(Incoming) >= static_cast<std::uint32_t>(CurrentPage);
    CurrentPage = Incoming;
    Motion->Eased(PageMotion).Depart(0.0, 1.0, DragDuration, 0.0, EaseCurve::Carousel);
}

Outcome<bool> ControlCentrePanel::Record(const PlaneExtent& Interior, ControlCentreConfiguration& Configuration)
{
    if (Surface == nullptr || Motion == nullptr)
        return Outcome<bool>::Refuse({RefusalReason::CapabilityAbsent, "no Control Centre construction stands"});

    if (Interior.Width() <= 0.0f || Interior.Height() <= 0.0f || Surface->Excluded(Interior))
        return Outcome<bool>::Result(true);

    ExclusionCount = 0u;
    if (Configuration.Page != CurrentPage) Navigate(Configuration.Page);

    if (Configuration.Theme != CurrentTheme)
    {
        PreviousTheme = CurrentTheme;
        CurrentTheme = Configuration.Theme;
        Motion->Eased(ThemeMotion).Depart(0.0, 1.0, 500.0, 0.0, EaseCurve::Standard);
    }

    const ThemeDeclaration& FromTheme = ThemeSpecification::Theme(PreviousTheme);
    const ThemeDeclaration& ToTheme = ThemeSpecification::Theme(CurrentTheme);
    const float ThemeFraction = static_cast<float>(Motion->Eased(ThemeMotion).Current());
    ThemeDeclaration Theme = ToTheme;
    Theme.Ground = Between(FromTheme.Ground, ToTheme.Ground, ThemeFraction);
    Theme.Panel = Between(FromTheme.Panel, ToTheme.Panel, ThemeFraction);
    Theme.Primary = Between(FromTheme.Primary, ToTheme.Primary, ThemeFraction);
    Theme.Secondary = Between(FromTheme.Secondary, ToTheme.Secondary, ThemeFraction);
    Theme.Edge = Between(FromTheme.Edge, ToTheme.Edge, ThemeFraction);
    Theme.Card = Between(FromTheme.Card, ToTheme.Card, ThemeFraction);
    const ThemeToken Accent = ThemeSpecification::Accent(Configuration.Primary).Colour;
    Surface->Ground(Interior, Theme.Panel, 0.0f, CornerNone);

    const PlaneExtent SettingsButton = Spanning(Interior.MaximumX - 68.0f, Interior.MinimumY + 24.0f, 44.0f, 44.0f);
    Surface->Ground(SettingsButton, Theme.Card, 22.0f, CornerAll);
    Surface->Edge(SettingsButton, Theme.Edge, 1.0f, 22.0f, CornerAll);
    Symbol(Spanning(SettingsButton.MinimumX + 10.0f, SettingsButton.MinimumY + 10.0f, 24.0f, 24.0f),
           Theme.Primary);
    if (Pressed(0u, SettingsButton))
    {
        Configuration.Page = ControlCentrePage::Settings;
        Navigate(Configuration.Page);
    }

    const PlaneExtent PageExtent = {Interior.MinimumX + PagePad, Interior.MinimumY + 88.0f,
                                    Interior.MaximumX - PagePad, Interior.MaximumY - 24.0f};
    const std::uint32_t PageOrdinal = static_cast<std::uint32_t>(CurrentPage);
    // 📝 The Fonts page ceiling follows the page's own content: the eight role strips and the sections
    //    below them stand about 1700px past the viewport, and a ceiling shorter than the content parks
    //    the wheel short of the antialiasing section at the foot.
    const float ScrollCeiling[5] = {120.0f, 80.0f, 260.0f, 1900.0f, 520.0f};
    const float ScrollFraction = static_cast<float>(Motion->Eased(ScrollMotion[PageOrdinal]).Current());
    Scroll[PageOrdinal] = ScrollFrom[PageOrdinal] +
                          (ScrollTarget[PageOrdinal] - ScrollFrom[PageOrdinal]) * ScrollFraction;

    if (PageExtent.Encloses(Pointer.PositionX, Pointer.PositionY) && Pointer.WheelY != 0.0f)
    {
        ScrollFrom[PageOrdinal] = Scroll[PageOrdinal];
        ScrollTarget[PageOrdinal] -= Pointer.WheelY * 72.0f;
        if (ScrollTarget[PageOrdinal] < 0.0f) ScrollTarget[PageOrdinal] = 0.0f;
        if (ScrollTarget[PageOrdinal] > ScrollCeiling[PageOrdinal])
            ScrollTarget[PageOrdinal] = ScrollCeiling[PageOrdinal];
        Motion->Eased(ScrollMotion[PageOrdinal]).Depart(0.0, 1.0, 180.0, 0.0, EaseCurve::CssEase);
    }
    const double Travel = Motion->Eased(PageMotion).Current();

    auto RenderPage = [&](ControlCentrePage Page, const PlaneExtent& Extent)
    {
        PlaneExtent Scrolled = Extent;
        if (Page != ControlCentrePage::Display)
        {
            const float Offset = Scroll[static_cast<std::uint32_t>(Page)];
            Scrolled.MinimumY -= Offset;
            Scrolled.MaximumY -= Offset;
        }
        switch (Page)
        {
        case ControlCentrePage::Settings:
            SettingsPage(Scrolled, Configuration, Theme, Accent);
            break;
        case ControlCentrePage::Notifications:
            NotificationsPage(Scrolled, Configuration, Theme, Accent);
            break;
        case ControlCentrePage::Display:
            DisplayPage(Scrolled, Configuration, Theme, Accent);
            break;
        case ControlCentrePage::Input:
            InputPage(Scrolled, Configuration, Theme, Accent);
            break;
        default:
            DashboardPage(Scrolled, Configuration, Theme, Accent);
            break;
        }
    };

    Surface->Confine(PageExtent);
    if (!Motion->Eased(PageMotion).Settled)
    {
        const float Direction = PageForward ? 1.0f : -1.0f;
        PlaneExtent Departing = PageExtent;
        PlaneExtent Incoming = PageExtent;
        Departing.MinimumX -= Direction * static_cast<float>(Travel) * PageExtent.Width();
        Departing.MaximumX -= Direction * static_cast<float>(Travel) * PageExtent.Width();
        Incoming.MinimumX += Direction * static_cast<float>(1.0 - Travel) * PageExtent.Width();
        Incoming.MaximumX += Direction * static_cast<float>(1.0 - Travel) * PageExtent.Width();
        RenderPage(PreviousPage, Departing);
        RenderPage(CurrentPage, Incoming);
    }
    else
    {
        RenderPage(CurrentPage, PageExtent);
    }
    Surface->Release();
    return Outcome<bool>::Result(true);
}

void ControlCentrePanel::DashboardPage(const PlaneExtent& Extent, ControlCentreConfiguration& Configuration,
                                       const ThemeDeclaration& Theme, ThemeToken Accent)
{
    const float ContentX = (Extent.Width() < 1024.0f) ? Extent.Width() : 1024.0f;
    const float Start = Extent.MinimumX + (Extent.Width() - ContentX) * .5f;
    const float LeftX = ContentX / 3.0f - 20.0f;
    const PlaneExtent Left = Spanning(Start, Extent.MinimumY, LeftX, Extent.Height());
    const PlaneExtent Right =
        Spanning(Start + LeftX + 48.0f, Extent.MinimumY, ContentX - LeftX - 48.0f, Extent.Height());
    Surface->TextRun(Left.MinimumX + 8.0f, Left.MinimumY, Theme.Primary, "Control Center", 20.0f, 0.0f, true);

    const char* QualityNames[5] = {"Low", "Medium", "High", "Epic", "Cinematic"};
    const char* AntialiasNames[3] = {"TSAA", "Basic", "None"};
    char LabelRuns[5][64] = {};
    std::snprintf(LabelRuns[0], sizeof(LabelRuns[0]), "Quality: %s", QualityNames[Configuration.Quality % 5u]);
    std::snprintf(LabelRuns[1], sizeof(LabelRuns[1]), "VSync: %s", Configuration.VsyncEnabled ? "ON" : "OFF");
    std::snprintf(LabelRuns[2], sizeof(LabelRuns[2]), "Global Illumination: %s",
                  Configuration.IlluminationEnabled ? "ON" : "OFF");
    std::snprintf(LabelRuns[3], sizeof(LabelRuns[3]), "Notifications: %s",
                  Configuration.NotificationsEnabled ? "ON" : "OFF");
    std::snprintf(LabelRuns[4], sizeof(LabelRuns[4]), "AA: %s", AntialiasNames[Configuration.Antialiasing % 3u]);
    for (std::uint32_t Ordinal = 0u; Ordinal < 5u; ++Ordinal)
    {
        const float Column = static_cast<float>(Ordinal % 2u);
        const float Row = static_cast<float>(Ordinal / 2u);
        const PlaneExtent Tile =
            Spanning(Left.MinimumX + Column * (Left.Width() * .5f + 4.0f),
                     Left.MinimumY + 42.0f + Row * (TileHeight + 16.0f), Left.Width() * .5f - 8.0f, TileHeight);
        const bool Active = Ordinal == 0u ||
                            (Ordinal == 1u && Configuration.VsyncEnabled) ||
                            (Ordinal == 2u && Configuration.IlluminationEnabled) ||
                            (Ordinal == 3u && Configuration.NotificationsEnabled) ||
                            (Ordinal == 4u && Configuration.Antialiasing != 2u);
        Surface->Ground(Tile, Active ? Accent : Theme.Card, static_cast<float>(Configuration.Radius), CornerAll);
        Surface->Edge(Tile, Theme.Edge, 1.0f, static_cast<float>(Configuration.Radius), CornerAll);
        Symbol(Spanning(Tile.MinimumX + Tile.Width() * .5f - 18.0f, Tile.MinimumY + 24.0f, 36.0f, 36.0f),
               Active ? White : Theme.Secondary);
        Surface->TextRunTruncated(Tile.MinimumX + 10.0f, Tile.MaximumY - 32.0f, Tile.MaximumX - 10.0f,
                                  Active ? White : Theme.Secondary, LabelRuns[Ordinal], 12.0f, true);
        if (Pressed(10u + Ordinal, Tile))
        {
            if (Ordinal == 0u) Configuration.Quality = (Configuration.Quality + 1u) % 5u;
            if (Ordinal == 1u) Configuration.VsyncEnabled = !Configuration.VsyncEnabled;
            if (Ordinal == 2u) Configuration.IlluminationEnabled = !Configuration.IlluminationEnabled;
            if (Ordinal == 3u) Configuration.NotificationsEnabled = !Configuration.NotificationsEnabled;
            if (Ordinal == 4u) Configuration.Antialiasing = (Configuration.Antialiasing + 1u) % 3u;
        }
    }

    const PlaneExtent Monitor =
        Spanning(Left.MinimumX, Left.MinimumY + 42.0f + 3.0f * (TileHeight + 16.0f), Left.Width(), 64.0f);
    Surface->Ground(Monitor, Theme.Card, static_cast<float>(Configuration.Radius), CornerAll);
    Symbol(Spanning(Monitor.MinimumX + 22.0f, Monitor.MinimumY + 22.0f, 20.0f, 20.0f), Theme.Secondary);
    Slider(21u, Spanning(Monitor.MinimumX + 58.0f, Monitor.MinimumY + 12.0f,
                         Monitor.Width() - 76.0f, 40.0f),
           0u, 100u, Configuration.MonitorLevel, "%", Theme.Edge, Accent);

    Surface->TextRun(Right.MinimumX + 8.0f, Right.MinimumY, Theme.Primary, "Notifications", 20.0f, 0.0f, true);
    const PlaneExtent Clear = Spanning(Right.MaximumX - 110.0f, Right.MinimumY - 4.0f, 110.0f, 30.0f);
    Surface->TextRun(Clear.MinimumX, CentredY(Clear, 12.0f), Accent, "Clear messages", 12.0f);
    if (Pressed(20u, Clear)) Configuration.NotificationsPresent = false;

    if (!Configuration.NotificationsPresent)
    {
        Symbol(Spanning(Right.MinimumX + Right.Width() * .5f - 24.0f, Right.MinimumY + 120.0f, 48.0f, 48.0f),
               WithOpacity(Theme.Secondary, .25f));
        Surface->TextRun(CentreText(*Surface, Right, "You're all caught up.", 14.0f), Right.MinimumY + 184.0f,
                         Theme.Secondary, "You're all caught up.", 14.0f);
        return;
    }

    const char* Titles[4] = {"Storage Almost Full", "High Memory Usage", "System Update", "New Message"};
    const char* Times[4] = {"Just now", "2m ago", "10m ago", "1h ago"};
    const char* Descriptions[4] = {"You have used 95% of your allocated cloud storage. Please upgrade your "
                                   "plan to avoid data loss.",
                                   "System memory is running high. Consider closing unused applications to "
                                   "improve performance.",
                                   "A new software update is available for your workspace. This includes "
                                   "security patches and performance improvements.",
                                   "Hey, are we still on for the design review tomorrow? I have some new "
                                   "mockups to share."};
    float NotificationCursor = Right.MinimumY + 42.0f;
    for (std::uint32_t Ordinal = 0u; Ordinal < 4u; ++Ordinal)
    {
        const PlaneExtent DescriptionMeasure = {Right.MinimumX + 76.0f, 0.0f,
                                                Right.MaximumX - 20.0f, 0.0f};
        const std::uint32_t DescriptionLines = WrappedText(*Surface, DescriptionMeasure, Theme.Secondary,
                                                           Descriptions[Ordinal], 13.0f, false);
        const float RequiredHeight = 71.0f + static_cast<float>(DescriptionLines) * 17.0f;
        const float CardHeight = RequiredHeight > 102.0f ? RequiredHeight : 102.0f;
        const PlaneExtent Card = Spanning(Right.MinimumX, NotificationCursor, Right.Width(), CardHeight);
        Surface->Ground(Card, Theme.Card, static_cast<float>(Configuration.Radius), CornerAll);
        Surface->Edge(Card, Theme.Edge, 1.0f, static_cast<float>(Configuration.Radius), CornerAll);
        Surface->Medallion(Card.MinimumX + 36.0f, Card.MinimumY + 38.0f, 24.0f, WithOpacity(Accent, .12f));
        Symbol(Spanning(Card.MinimumX + 24.0f, Card.MinimumY + 26.0f, 24.0f, 24.0f), Accent);

        const float TimeX = Surface->MeasureRun(Times[Ordinal], 12.0f);
        Surface->TextRunTruncated(Card.MinimumX + 76.0f, Card.MinimumY + 18.0f,
                                  Card.MaximumX - TimeX - 36.0f,
                                  Ordinal < 3u ? Accent : Theme.Primary, Titles[Ordinal], 16.0f, true);
        Surface->TextRun(Card.MaximumX - TimeX - 20.0f, Card.MinimumY + 20.0f,
                         Theme.Secondary, Times[Ordinal], 12.0f);

        const PlaneExtent DescriptionClip = {Card.MinimumX + 76.0f, Card.MinimumY + 51.0f,
                                             Card.MaximumX - 20.0f, Card.MaximumY - 16.0f};
        Surface->Confine(DescriptionClip);
        WrappedText(*Surface, DescriptionClip, Theme.Secondary, Descriptions[Ordinal], 13.0f, true);
        Surface->Release();
        NotificationCursor = Card.MaximumY + 16.0f;
    }
}

void ControlCentrePanel::SettingsPage(const PlaneExtent& Extent, ControlCentreConfiguration& Configuration,
                                      const ThemeDeclaration& Theme, ThemeToken Accent)
{
    const float Width = (Extent.Width() < 672.0f) ? Extent.Width() : 672.0f;
    const float X = Extent.MinimumX + (Extent.Width() - Width) * .5f;
    const PlaneExtent Back = Spanning(X, Extent.MinimumY, 42.0f, 42.0f);
    Symbol(Spanning(Back.MinimumX + 9.0f, Back.MinimumY + 9.0f, 24.0f, 24.0f), Theme.Primary);
    if (Pressed(30u, Back))
    {
        Configuration.Page = ControlCentrePage::Dashboard;
        Navigate(Configuration.Page);
    }
    Surface->TextRun(X + 58.0f, Extent.MinimumY + 8.0f, Theme.Primary, "Settings", 24.0f, 0.0f,
                     false, RoleWeightOf(Configuration.TypographyWeight, 0u));

    const PlaneExtent Card = Spanning(X, Extent.MinimumY + 74.0f, Width, 5.0f * RowHeight);
    Surface->Ground(Card, Theme.Card, static_cast<float>(Configuration.Radius < 16u ? 16u : Configuration.Radius), CornerAll);
    Surface->Edge(Card, Theme.Edge, 1.0f, static_cast<float>(Configuration.Radius < 16u ? 16u : Configuration.Radius),
                  CornerAll);
    const char* Titles[5] = {"Display Settings", "Display & Workspace", "Input Devices", "Privacy & Security",
                             "Apps & Notifications"};
    const char* Subs[5] = {"Appearance, theme, fonts, and system colors", "Resolution, scaling, multiple displays",
                           "Keyboard, mouse, and touch settings", "Permissions, camera access, firewall",
                           "Do not disturb, app permissions"};
    for (std::uint32_t Ordinal = 0u; Ordinal < 5u; ++Ordinal)
    {
        const PlaneExtent Row = Spanning(Card.MinimumX, Card.MinimumY + RowHeight * static_cast<float>(Ordinal),
                                         Card.Width(), RowHeight);
        if (Ordinal < 4u)
            Surface->Ground(Spanning(Row.MinimumX + 20.0f, Row.MaximumY - 1.0f, Row.Width() - 40.0f, 1.0f),
                            Theme.Edge, 0.0f, CornerNone);
        Surface->Medallion(Row.MinimumX + 44.0f, Row.MinimumY + 38.0f, 24.0f, Theme.Ground);
        Symbol(Spanning(Row.MinimumX + 32.0f, Row.MinimumY + 26.0f, 24.0f, 24.0f),
               Ordinal == 0u ? Accent : Theme.Secondary);
        Surface->TextRun(Row.MinimumX + 82.0f, Row.MinimumY + 18.0f, Theme.Primary, Titles[Ordinal], 16.0f, 0.0f,
                         true);
        Surface->TextRun(Row.MinimumX + 82.0f, Row.MinimumY + 43.0f, Theme.Secondary, Subs[Ordinal], 13.0f);
        Symbol(Spanning(Row.MaximumX - 40.0f, Row.MinimumY + 28.0f, 20.0f, 20.0f),
               WithOpacity(Theme.Secondary, .5f));
        if (Pressed(31u + Ordinal, Row))
        {
            if (Ordinal <= 1u)
            {
                Configuration.Page = ControlCentrePage::Display;
                Configuration.DisplayPage = Ordinal == 0u ? DisplayPreferencePage::Theme : DisplayPreferencePage::Display;
            }
            else if (Ordinal == 2u)
                Configuration.Page = ControlCentrePage::Input;
            else if (Ordinal == 4u)
                Configuration.Page = ControlCentrePage::Notifications;
            Navigate(Configuration.Page);
        }
    }
}

void ControlCentrePanel::NotificationsPage(const PlaneExtent& Extent, ControlCentreConfiguration& Configuration,
                                           const ThemeDeclaration& Theme, ThemeToken Accent)
{
    const float Width = (Extent.Width() < 768.0f) ? Extent.Width() : 768.0f;
    const float X = Extent.MinimumX + (Extent.Width() - Width) * .5f;
    Surface->TextRun(X, Extent.MinimumY, Theme.Primary, "Apps & Notifications", 29.0f, 0.0f,
                     false, RoleWeightOf(Configuration.TypographyWeight, 0u));
    const PlaneExtent Back = Spanning(X + Width - 44.0f, Extent.MinimumY, 40.0f, 40.0f);
    Surface->Ground(Back, Theme.Card, 20.0f, CornerAll);
    Surface->Edge(Back, Theme.Edge, 1.0f, 20.0f, CornerAll);
    Symbol(Spanning(Back.MinimumX + 10.0f, Back.MinimumY + 10.0f, 20.0f, 20.0f), Theme.Primary);
    if (Pressed(40u, Back))
    {
        Configuration.Page = ControlCentrePage::Settings;
        Navigate(Configuration.Page);
    }

    const PlaneExtent Global = Spanning(X, Extent.MinimumY + 66.0f, Width, 176.0f);
    Surface->Ground(Global, Theme.Card, static_cast<float>(Configuration.Radius < 16u ? 16u : Configuration.Radius), CornerAll);
    Surface->Edge(Global, Theme.Edge, 1.0f, 20.0f, CornerAll);
    const char* Titles[2] = {"Do Not Disturb", "Notification Sounds"};
    const char* Subs[2] = {"Silence all notifications and alerts", "Play sounds for incoming alerts"};
    bool* Conditions[2] = {&Configuration.DisturbanceWithheld, &Configuration.SoundEnabled};
    for (std::uint32_t Ordinal = 0u; Ordinal < 2u; ++Ordinal)
    {
        const PlaneExtent Row =
            Spanning(Global.MinimumX + 24.0f, Global.MinimumY + 12.0f + 80.0f * static_cast<float>(Ordinal),
                     Global.Width() - 48.0f, 72.0f);
        Surface->Medallion(Row.MinimumX + 24.0f, Row.MinimumY + 36.0f, 20.0f, WithOpacity(Accent, .10f));
        Symbol(Spanning(Row.MinimumX + 14.0f, Row.MinimumY + 26.0f, 20.0f, 20.0f), Accent);
        Surface->TextRun(Row.MinimumX + 62.0f, Row.MinimumY + 17.0f, Theme.Primary, Titles[Ordinal], 18.0f, 0.0f,
                         true);
        Surface->TextRun(Row.MinimumX + 62.0f, Row.MinimumY + 43.0f, Theme.Secondary, Subs[Ordinal], 13.0f);
        Toggle(41u + Ordinal, Spanning(Row.MaximumX - 48.0f, Row.MinimumY + 24.0f, 48.0f, 24.0f),
               *Conditions[Ordinal], Theme.Edge, Accent);
    }

    Surface->TextRun(X + 8.0f, Global.MaximumY + 36.0f, Theme.Primary, "App Permissions", 24.0f, 0.0f,
                     false, RoleWeightOf(Configuration.TypographyWeight, 0u));
    Surface->TextRun(X + 8.0f, Global.MaximumY + 68.0f, Theme.Secondary,
                     "Choose which apps can send you notifications", 14.0f, 0.0f,
                     false, RoleWeightOf(Configuration.TypographyWeight, 5u));
    const PlaneExtent Apps = Spanning(X, Global.MaximumY + 100.0f, Width, 4.0f * RowHeight);
    Surface->Ground(Apps, Theme.Card, static_cast<float>(Configuration.Radius < 16u ? 16u : Configuration.Radius), CornerAll);
    Surface->Edge(Apps, Theme.Edge, 1.0f, 20.0f, CornerAll);
    const char* AppTitles[4] = {"Mail", "Calendar", "Messages", "System Alerts"};
    const char* AppSubs[4] = {"New emails and calendar invites", "Upcoming events and reminders",
                              "Direct messages and mentions", "Critical system and security updates"};
    for (std::uint32_t Ordinal = 0u; Ordinal < 4u; ++Ordinal)
    {
        const PlaneExtent Row =
            Spanning(Apps.MinimumX + 20.0f, Apps.MinimumY + RowHeight * static_cast<float>(Ordinal),
                     Apps.Width() - 40.0f, RowHeight);
        Symbol(Spanning(Row.MinimumX + 8.0f, Row.MinimumY + 26.0f, 24.0f, 24.0f), Theme.Secondary);
        Surface->TextRun(Row.MinimumX + 52.0f, Row.MinimumY + 17.0f, Theme.Primary, AppTitles[Ordinal], 16.0f,
                         0.0f, true);
        Surface->TextRun(Row.MinimumX + 52.0f, Row.MinimumY + 43.0f, Theme.Secondary, AppSubs[Ordinal], 13.0f);
        Toggle(45u + Ordinal, Spanning(Row.MaximumX - 48.0f, Row.MinimumY + 26.0f, 48.0f, 24.0f),
               Configuration.AppNotifications[Ordinal], Theme.Edge, Accent);
    }
}

void ControlCentrePanel::DisplayPage(const PlaneExtent& Extent, ControlCentreConfiguration& Configuration,
                                     const ThemeDeclaration& Theme, ThemeToken Accent)
{
    const PlaneExtent Back = Spanning(Extent.MinimumX, Extent.MinimumY, 42.0f, 42.0f);
    Surface->Ground(Back, Theme.Card, 21.0f, CornerAll);
    Surface->Edge(Back, Theme.Edge, 1.0f, 21.0f, CornerAll);
    Symbol(Spanning(Back.MinimumX + 9.0f, Back.MinimumY + 9.0f, 24.0f, 24.0f), Theme.Primary);
    if (Pressed(50u, Back))
    {
        Configuration.Page = ControlCentrePage::Settings;
        Navigate(Configuration.Page);
    }
    Surface->TextRun(Extent.MinimumX + 58.0f, Extent.MinimumY + 3.0f, Theme.Primary, "Display Settings", 29.0f,
                     0.0f, false, RoleWeightOf(Configuration.TypographyWeight, 0u));
    Surface->TextRun(Extent.MinimumX + 58.0f, Extent.MinimumY + 40.0f, Theme.Secondary, "Appearance & typography",
                     14.0f, 0.0f, false, RoleWeightOf(Configuration.TypographyWeight, 5u));

    if (Configuration.DisplayPage != CurrentTab)
    {
        PreviousTab = CurrentTab;
        TabForward = static_cast<std::uint32_t>(Configuration.DisplayPage) >= static_cast<std::uint32_t>(CurrentTab);
        CurrentTab = Configuration.DisplayPage;
        Motion->Eased(TabMotion).Depart(0.0, 1.0, 220.0, 0.0, EaseCurve::Carousel);
    }

    const char* Tabs[3] = {"Display", "Fonts", "Theme"};
    float TabX = Extent.MinimumX + 58.0f;
    for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
    {
        const float Width = Surface->MeasureRun(Tabs[Ordinal], 24.0f) + 8.0f;
        const PlaneExtent Tab = Spanning(TabX, Extent.MinimumY + 78.0f, Width, 42.0f);
        Surface->TextRun(Tab.MinimumX + 4.0f, Tab.MinimumY + 4.0f,
                         Configuration.DisplayPage == static_cast<DisplayPreferencePage>(Ordinal) ? Theme.Primary
                                                                                              : Theme.Secondary,
                         Tabs[Ordinal], 24.0f, 0.0f, false, RoleWeightOf(Configuration.TypographyWeight, 2u));
        if (Configuration.DisplayPage == static_cast<DisplayPreferencePage>(Ordinal))
            Surface->Ground(Spanning(Tab.MinimumX, Tab.MaximumY - 3.0f, Tab.Width(), 3.0f), Accent, 1.5f,
                            CornerAll);
        if (Pressed(51u + Ordinal, Tab)) Configuration.DisplayPage = static_cast<DisplayPreferencePage>(Ordinal);
        TabX += Width + 24.0f;
    }

    const PlaneExtent Viewport = {Extent.MinimumX + 58.0f, Extent.MinimumY + 136.0f, Extent.MaximumX - 16.0f,
                                  Extent.MaximumY};
    const auto RenderTab = [&](DisplayPreferencePage Page, PlaneExtent Content)
    {
        Content.MinimumY -= Scroll[static_cast<std::uint32_t>(ControlCentrePage::Display)];
        Content.MaximumY -= Scroll[static_cast<std::uint32_t>(ControlCentrePage::Display)];
        if (Page == DisplayPreferencePage::Display)
            DisplayHardwarePage(Content, Configuration, Theme, Accent);
        else if (Page == DisplayPreferencePage::Theme)
            ThemePage(Content, Configuration, Theme, Accent);
        else
            FontsPage(Content, Configuration, Theme, Accent);
    };

    Surface->Confine(Viewport);
    if (!Motion->Eased(TabMotion).Settled)
    {
        const float Travel = static_cast<float>(Motion->Eased(TabMotion).Current());
        const float Direction = TabForward ? 1.0f : -1.0f;
        PlaneExtent Departing = Viewport;
        PlaneExtent Incoming = Viewport;
        Departing.MinimumX -= Direction * Travel * Viewport.Width();
        Departing.MaximumX -= Direction * Travel * Viewport.Width();
        Incoming.MinimumX += Direction * (1.0f - Travel) * Viewport.Width();
        Incoming.MaximumX += Direction * (1.0f - Travel) * Viewport.Width();
        RenderTab(PreviousTab, Departing);
        RenderTab(CurrentTab, Incoming);
    }
    else
    {
        RenderTab(CurrentTab, Viewport);
    }
    Surface->Release();
}

void ControlCentrePanel::DisplayHardwarePage(const PlaneExtent& Extent, ControlCentreConfiguration& Configuration,
                                             const ThemeDeclaration& Theme, ThemeToken Accent)
{
    const PlaneExtent Card = Spanning(Extent.MinimumX, Extent.MinimumY, Extent.Width(), 440.0f);
    Surface->Ground(Card, Theme.Card, static_cast<float>(Configuration.Radius < 16u ? 16u : Configuration.Radius), CornerAll);
    Surface->Edge(Card, Theme.Edge, 1.0f, 20.0f, CornerAll);
    const char* Headings[4] = {"Resolution", "UI Scaling", "Refresh Rate", "Multiple Displays"};
    for (std::uint32_t Ordinal = 0u; Ordinal < 4u; ++Ordinal)
        Surface->TextRun(Card.MinimumX + 28.0f, Card.MinimumY + 25.0f + 105.0f * static_cast<float>(Ordinal),
                         Theme.Primary, Headings[Ordinal], 22.0f, 0.0f, true);
    const char* Res[3] = {"1920x1080", "2560x1440", "3840x2160"};
    for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
    {
        const PlaneExtent Button = Spanning(Card.MinimumX + 28.0f + 125.0f * static_cast<float>(Ordinal),
                                            Card.MinimumY + 58.0f, 114.0f, 38.0f);
        Surface->Ground(Button, Configuration.Resolution == Ordinal ? Accent : Theme.Panel, 12.0f, CornerAll);
        Surface->Edge(Button, Theme.Edge, 1.0f, 12.0f, CornerAll);
        Surface->TextRun(CentreText(*Surface, Button, Res[Ordinal], 13.0f), CentredY(Button, 13.0f),
                         Configuration.Resolution == Ordinal ? White : Theme.Secondary, Res[Ordinal], 13.0f);
        if (Pressed(60u + Ordinal, Button)) Configuration.Resolution = Ordinal;
    }
    Slider(63u, Spanning(Card.MinimumX + 28.0f, Card.MinimumY + 165.0f, Card.Width() - 56.0f, 24.0f), 100u,
           200u, Configuration.Scaling, "%", Theme.Edge, Accent);
    const char* Rates[3] = {"60Hz", "120Hz", "144Hz"};
    const char* Modes[3] = {"Mirror", "Extend", "Second Only"};
    for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
    {
        const PlaneExtent Rate = Spanning(Card.MinimumX + 28.0f + 92.0f * static_cast<float>(Ordinal),
                                          Card.MinimumY + 270.0f, 82.0f, 38.0f);
        Surface->Ground(Rate, Configuration.RefreshRate == Ordinal ? QuietDark : Theme.Panel, 12.0f, CornerAll);
        Surface->Edge(Rate, Theme.Edge, 1.0f, 12.0f, CornerAll);
        Surface->TextRun(CentreText(*Surface, Rate, Rates[Ordinal], 13.0f), CentredY(Rate, 13.0f), Theme.Primary,
                         Rates[Ordinal], 13.0f);
        if (Pressed(64u + Ordinal, Rate)) Configuration.RefreshRate = Ordinal;
        const PlaneExtent Mode =
            Spanning(Card.MinimumX + 28.0f + (Card.Width() - 56.0f) / 3.0f * static_cast<float>(Ordinal),
                     Card.MinimumY + 375.0f, (Card.Width() - 56.0f) / 3.0f, 42.0f);
        Surface->Ground(Mode, Configuration.MultipleDisplays == Ordinal ? Theme.Card : QuietDark, 12.0f, CornerAll);
        Surface->TextRun(CentreText(*Surface, Mode, Modes[Ordinal], 13.0f), CentredY(Mode, 13.0f),
                         Configuration.MultipleDisplays == Ordinal ? Theme.Primary : Theme.Secondary, Modes[Ordinal],
                         13.0f);
        if (Pressed(67u + Ordinal, Mode)) Configuration.MultipleDisplays = Ordinal;
    }
}

void ControlCentrePanel::ThemePage(const PlaneExtent& Extent, ControlCentreConfiguration& Configuration,
                                   const ThemeDeclaration& Theme, ThemeToken Accent)
{
    const PlaneExtent Section = Spanning(Extent.MinimumX, Extent.MinimumY, Extent.Width(), 1340.0f);
    Surface->Ground(Section, WithOpacity(Theme.Card, .72f), static_cast<float>(Configuration.Radius < 24u ? 24u : Configuration.Radius), CornerAll);
    Surface->Edge(Section, Theme.Edge, 1.0f, static_cast<float>(Configuration.Radius < 24u ? 24u : Configuration.Radius), CornerAll);
    const float Inset = 28.0f;
    const float ContentLeft = Extent.MinimumX + Inset;
    const float ContentRight = Extent.MaximumX - Inset;
    Surface->TextRun(ContentLeft, Extent.MinimumY + Inset, Theme.Primary, "Theme", 24.0f, 0.0f, true);
    Surface->TextRun(ContentLeft, Extent.MinimumY + Inset + 32.0f, Theme.Secondary, "Customize UI colors", 14.0f);
    const float AvailableTileWidth = (ContentRight - ContentLeft - 40.0f) / 3.0f;
    const float TileWidth = AvailableTileWidth < 300.0f ? AvailableTileWidth : 300.0f;
    const float TileHeight = TileWidth * (250.0f / 300.0f);
    const float GridWidth = TileWidth * 3.0f + 40.0f;
    const float GridTop = ContentLeft + (ContentRight - ContentLeft - GridWidth) * 0.5f;
    const ThemeToken SelectionColour = Covering(0x7B42F6u);

    for (std::uint32_t Ordinal = 0u; Ordinal < 6u; ++Ordinal)
    {
        const ThemeSubject PreviewSubject = static_cast<ThemeSubject>(Ordinal);
        const ThemeDeclaration& Preview = ThemeSpecification::Theme(PreviewSubject);
        const bool WhitePreview = PreviewSubject == ThemeSubject::CleanWhite;
        const ThemeToken SidebarQuiet = WhitePreview ? Covering(0xDADAE0u) : Preview.PreviewSidebarQuiet;
        const ThemeToken SidebarStrong = WhitePreview ? Covering(0xC8C8CEu) : Preview.PreviewSidebarStrong;
        const ThemeToken MainQuiet = WhitePreview ? Covering(0xF0F0F0u) : Preview.PreviewQuiet;
        const ThemeToken MainStrong = WhitePreview ? Covering(0xE0E0E0u) : Preview.PreviewStrong;
        const float Column = static_cast<float>(Ordinal % 3u);
        const float Row = static_cast<float>(Ordinal / 3u);
        const PlaneExtent Tile = Spanning(GridTop + Column * (TileWidth + 20.0f),
                                          Extent.MinimumY + Inset + 64.0f + Row * (TileHeight + 20.0f),
                                          TileWidth, TileHeight);
        const float XScale = TileWidth / 300.0f;
        const float YScale = TileHeight / 250.0f;
        const float OuterRadius = static_cast<float>(Configuration.Radius) * (18.0f / 24.0f) * XScale;
        const bool Selected = Configuration.Theme == static_cast<ThemeSubject>(Ordinal);
        const PlaneExtent Outer = Spanning(Tile.MinimumX + 15.0f * XScale,
                                           Tile.MinimumY + 15.0f * YScale,
                                           270.0f * XScale, 195.0f * YScale);

        if (Selected)
            Surface->Edge(Outer, WithOpacity(SelectionColour, .25f), 4.0f * XScale,
                          OuterRadius, CornerAll);
        Surface->Ground(Outer, Preview.PreviewGround, OuterRadius, CornerAll);
        Surface->Edge(Outer, Selected ? SelectionColour : Preview.Edge,
                      (Selected ? 1.5f : 1.0f) * XScale, OuterRadius, CornerAll);

        const PlaneExtent Window = Spanning(Tile.MinimumX + 45.0f * XScale,
                                            Tile.MinimumY + 40.0f * YScale,
                                            210.0f * XScale, 150.0f * YScale);
        const float WindowRadius = static_cast<float>(Configuration.Radius) * (14.0f / 24.0f) * XScale;
        Surface->Ground(Window, Preview.PreviewSidebar, WindowRadius, CornerAll);

        const PlaneExtent RightPanel = Spanning(Tile.MinimumX + 110.0f * XScale,
                                                Tile.MinimumY + 40.0f * YScale,
                                                145.0f * XScale, 150.0f * YScale);
        Surface->Ground(RightPanel, Preview.PreviewWindow, WindowRadius, CornerAll);
        Surface->Edge(Window, Preview.Edge, 1.0f, WindowRadius, CornerAll);

        for (std::uint32_t Dot = 0u; Dot < 3u; ++Dot)
            Surface->Medallion(Tile.MinimumX + (60.0f + 9.0f * static_cast<float>(Dot)) * XScale,
                               Tile.MinimumY + 55.0f * YScale, 2.5f * XScale,
                               SidebarStrong);

        const float SidebarWidths[3] = {40.0f, 28.0f, 18.0f};
        for (std::uint32_t Line = 0u; Line < 3u; ++Line)
            Surface->Ground(Spanning(Tile.MinimumX + 58.0f * XScale,
                                     Tile.MinimumY + (72.0f + 14.0f * static_cast<float>(Line)) * YScale,
                                     SidebarWidths[Line] * XScale, 6.0f * YScale),
                            SidebarQuiet, 3.0f * XScale, CornerAll);

        Surface->Medallion(Tile.MinimumX + 63.0f * XScale,
                           Tile.MinimumY + 175.0f * YScale, 5.0f * XScale,
                           SidebarStrong);
        Surface->Ground(Spanning(Tile.MinimumX + 74.0f * XScale,
                                 Tile.MinimumY + 172.0f * YScale,
                                 16.0f * XScale, 6.0f * YScale),
                        SidebarQuiet, 3.0f * XScale, CornerAll);

        Surface->Ground(Spanning(Tile.MinimumX + 123.0f * XScale,
                                 Tile.MinimumY + 60.0f * YScale,
                                 45.0f * XScale, 6.0f * YScale),
                        MainStrong, 3.0f * XScale, CornerAll);
        Surface->Ground(Spanning(Tile.MinimumX + 123.0f * XScale,
                                 Tile.MinimumY + 75.0f * YScale,
                                 42.0f * XScale, 6.0f * YScale),
                        MainStrong, 3.0f * XScale, CornerAll);

        for (std::uint32_t Cell = 0u; Cell < 3u; ++Cell)
            Surface->Ground(Spanning(Tile.MinimumX + (123.0f + 44.0f * static_cast<float>(Cell)) * XScale,
                                     Tile.MinimumY + 95.0f * YScale,
                                     32.0f * XScale, 32.0f * YScale),
                            MainQuiet, 8.0f * XScale, CornerAll);

        Surface->Ground(Spanning(Tile.MinimumX + 123.0f * XScale,
                                 Tile.MinimumY + 172.0f * YScale,
                                 26.0f * XScale, 6.0f * YScale),
                        MainStrong, 3.0f * XScale, CornerAll);

        Surface->TextRun(CentreText(*Surface, Tile, Preview.Caption, 13.0f * XScale),
                         Tile.MinimumY + 222.0f * YScale,
                         Selected ? Theme.Primary : Theme.Secondary,
                         Preview.Caption, 13.0f * XScale, .04f, true);
        if (Pressed(75u + Ordinal, Tile)) Configuration.Theme = static_cast<ThemeSubject>(Ordinal);
    }

    const float Below = Extent.MinimumY + Inset + 64.0f + 2.0f * (TileHeight + 20.0f) + 16.0f;
    Surface->TextRun(ContentLeft, Below, Theme.Primary, "Corner Radius", 22.0f, 0.0f, true);
    Slider(82u, Spanning(ContentLeft, Below + 48.0f, ContentRight - ContentLeft, 40.0f), 0u, 48u,
           Configuration.Radius, "px", Theme.Edge, Accent);
    Surface->TextRun(ContentLeft, Below + 100.0f, Theme.Primary, "Sidebar", 22.0f, 0.0f, true);
    Surface->TextRun(ContentLeft, Below + 130.0f, Theme.Secondary, "Make the sidebar transparent", 14.0f);
    Toggle(83u, Spanning(ContentRight - 48.0f, Below + 104.0f, 48.0f, 24.0f), Configuration.TransparentSidebar,
           Theme.Edge, Accent);

    const float ColoursTop = Below + 184.0f;
    Surface->TextRun(ContentLeft, ColoursTop, Theme.Primary, "System Colors", 24.0f, 0.0f, true);
    Surface->TextRun(ContentLeft, ColoursTop + 32.0f, Theme.Secondary, "Semantic colors for UI elements", 14.0f);
    const char* Names[5] = {"Primary", "Secondary", "Info", "Warning", "Alert"};
    const char* Descriptions[5] = {"Main interactive elements and accents", "Alternative interactive elements",
                                   "Informational messages and badges", "Non-critical alerts and warnings",
                                   "Critical errors and destructive actions"};
    float Cursor = ColoursTop + 70.0f;
    for (std::uint32_t Ordinal = 0u; Ordinal < 5u; ++Ordinal)
    {
        bool Open = OpenPalette == Ordinal;
        const PlaneExtent Header = Spanning(ContentLeft, Cursor, ContentRight - ContentLeft, 58.0f);
        if (Pressed(84u + Ordinal, Header))
        {
            OpenPalette = Open ? 5u : Ordinal;
            Open = OpenPalette == Ordinal;
        }

        Interaction.DeclareTaken(Controls[84u + Ordinal], Open, 220.0, EaseCurve::CssEase);
        const float Disclosure = Interaction.TakenFraction(Controls[84u + Ordinal]);
        const float Height = 58.0f + 68.0f * Disclosure;
        const PlaneExtent Row = Spanning(ContentLeft, Cursor, ContentRight - ContentLeft, Height);
        Surface->Ground(Row, Theme.Card, Ordinal == 0u || Ordinal == 4u ? 16.0f : 0.0f, CornerAll);
        Surface->TextRun(Header.MinimumX + 20.0f, Header.MinimumY + 20.0f, Theme.Primary, Names[Ordinal],
                         14.0f, 0.0f, true);
        Surface->Medallion(Header.MaximumX - 48.0f, Header.MinimumY + 28.0f, 10.0f,
                           ThemeSpecification::Accent(Configuration.SemanticColours[Ordinal]).Colour);
        Symbol(Spanning(Header.MaximumX - 26.0f, Header.MinimumY + 20.0f, 16.0f, 16.0f), Theme.Secondary);

        if (Disclosure > 0.0f)
        {
            const PlaneExtent Revealed = {Row.MinimumX, Header.MaximumY,
                                          Row.MaximumX, Header.MaximumY + 68.0f * Disclosure};
            Surface->Confine(Revealed);
            Surface->TextRun(Row.MinimumX + 20.0f, Header.MaximumY + 6.0f, Theme.Secondary,
                             Descriptions[Ordinal], 12.0f);
            for (std::uint32_t Colour = 0u; Colour < 8u; ++Colour)
            {
                const PlaneExtent Swatch = Spanning(Row.MinimumX + 22.0f + 44.0f * static_cast<float>(Colour),
                                                    Header.MaximumY + 26.0f, 32.0f, 32.0f);
                Surface->Ground(Swatch, ThemeSpecification::Accent(static_cast<AccentSubject>(Colour)).Colour,
                                16.0f, CornerAll);
                if (Configuration.SemanticColours[Ordinal] == static_cast<AccentSubject>(Colour))
                    Surface->Edge(Spanning(Swatch.MinimumX - 3.0f, Swatch.MinimumY - 3.0f, 38.0f, 38.0f),
                                  WithOpacity(White, .55f), 2.0f, 19.0f, CornerAll);
                if (Disclosure > .95f && Pressed(90u + Ordinal * 8u + Colour, Swatch))
                {
                    Configuration.SemanticColours[Ordinal] = static_cast<AccentSubject>(Colour);
                    if (Ordinal == 0u) Configuration.Primary = static_cast<AccentSubject>(Colour);
                }
            }
            Surface->Release();
        }
        Cursor += Height;
    }
}

void ControlCentrePanel::SetFontFamilies(FontLoader& Loader)
{
    FontArchive = &Loader;
}

void ControlCentrePanel::FontsPage(const PlaneExtent& Extent, ControlCentreConfiguration& Configuration,
                                   const ThemeDeclaration& Theme, ThemeToken Accent)
{
    const char* DefaultFamily = "Inter";
    const std::uint32_t FontCount = (FontArchive != nullptr && FontArchive->FamilyCount() > 0u)
                                  ? FontArchive->FamilyCount() : 1u;
    const auto FamilyAt = [&](std::uint32_t Ordinal) -> const char*
    {
        return (FontArchive != nullptr && FontArchive->FamilyCount() > 0u)
             ? FontArchive->FamilyName(Ordinal)
             : DefaultFamily;
    };
    const float Inset = 28.0f;
    const float ContentLeft = Extent.MinimumX + Inset;
    const float ContentRight = Extent.MaximumX - Inset;
    const float SpecimenTop = Extent.MinimumY + Inset + 230.0f;

    // 📐 The section card is sized from its content rather than from a fixed figure, because every role row
    //    now carries a family strip: the eight strips add a constant 40px each, and the tallest sample text
    //    still decides the row. A card that ended mid-content would draw the icon and antialiasing sections
    //    on the bare page ground.
    const auto EntryHeightOf = [](std::uint32_t Size) -> float
    {
        const float Sample = static_cast<float>(Size);
        return (Sample + 180.0f > 190.0f) ? Sample + 180.0f : 190.0f;
    };
    float ContentBottom = SpecimenTop + 176.0f + 30.0f;
    for (std::uint32_t Ordinal = 0u; Ordinal < 8u; ++Ordinal)
        ContentBottom += EntryHeightOf(Configuration.TypographySize[Ordinal]) + 12.0f;
    ContentBottom += 340.0f;   // the icon style, icon font and antialiasing sections below the roles

    const PlaneExtent Section = Spanning(Extent.MinimumX, Extent.MinimumY, Extent.Width(),
                                         ContentBottom - Extent.MinimumY + 24.0f);
    Surface->Ground(Section, WithOpacity(Theme.Card, .72f),
                    static_cast<float>(Configuration.Radius < 24u ? 24u : Configuration.Radius), CornerAll);
    Surface->Edge(Section, Theme.Edge, 1.0f,
                  static_cast<float>(Configuration.Radius < 24u ? 24u : Configuration.Radius), CornerAll);
    Surface->TextRun(ContentLeft, Extent.MinimumY + Inset, Theme.Primary, "Typography", 24.0f, 0.0f,
                     false, RoleWeightOf(Configuration.TypographyWeight, 0u));
    Surface->TextRun(ContentLeft, Extent.MinimumY + Inset + 32.0f, Theme.Secondary, "Typeface & scale",
                     14.0f, 0.0f, false, RoleWeightOf(Configuration.TypographyWeight, 5u));
    const float RailY = Extent.MinimumY + Inset + 64.0f;
    const PlaneExtent Left = Spanning(ContentLeft, RailY + 46.0f, 44.0f, 44.0f);
    const PlaneExtent Right = Spanning(ContentRight - 44.0f, RailY + 46.0f, 44.0f, 44.0f);
    const PlaneExtent FontRail = {Left.MaximumX + 12.0f, RailY,
                                  Right.MinimumX - 12.0f, RailY + 136.0f};

    const float FontFraction = static_cast<float>(Motion->Eased(FontMotion).Current());
    FontScroll = FontFrom + (FontTarget - FontFrom) * FontFraction;
    Surface->Confine(FontRail);
    for (std::uint32_t Ordinal = 0u; Ordinal < FontCount; ++Ordinal)
    {
        const PlaneExtent Tile = Spanning(FontRail.MinimumX + 4.0f +
                                              208.0f * static_cast<float>(Ordinal) - FontScroll,
                                          RailY, 192.0f, 132.0f);
        Surface->ApplyFontPreview(FontArchive != nullptr ? FontArchive->Preview(FamilyAt(Ordinal), 1.0f) : nullptr);
        Surface->Ground(Tile, Configuration.Font == Ordinal ? Theme.Card : Theme.Panel, 16.0f, CornerAll);
        Surface->Edge(Tile, Configuration.Font == Ordinal ? Theme.Edge : WithOpacity(Theme.Edge, 0.0f), 1.0f,
                      16.0f, CornerAll);
        Surface->TextRun(Tile.MinimumX + 18.0f, Tile.MinimumY + 18.0f, Theme.Primary, "Aa", 30.0f);
        Surface->TextRun(Tile.MinimumX + 18.0f, Tile.MinimumY + 66.0f, Theme.Primary, FamilyAt(Ordinal),
                         14.0f, 0.0f, true);
        Surface->TextRun(Tile.MinimumX + 18.0f, Tile.MinimumY + 92.0f, Theme.Secondary,
                         "The quick brown fox", 12.0f);
        const PlaneExtent TileContact = {
            Tile.MinimumX > FontRail.MinimumX ? Tile.MinimumX : FontRail.MinimumX,
            Tile.MinimumY,
            Tile.MaximumX < FontRail.MaximumX ? Tile.MaximumX : FontRail.MaximumX,
            Tile.MaximumY
        };
        if (TileContact.MaximumX > TileContact.MinimumX && 130u + Ordinal < ControlCapacity &&
            Pressed(130u + Ordinal, TileContact))
            Configuration.Font = Ordinal;
    }
    Surface->Release();
    Surface->ApplyFontPreview(nullptr);

    Surface->Ground(Left, Theme.Card, 22.0f, CornerAll);
    Surface->Ground(Right, Theme.Card, 22.0f, CornerAll);
    Surface->Edge(Left, Theme.Edge, 1.0f, 22.0f, CornerAll);
    Surface->Edge(Right, Theme.Edge, 1.0f, 22.0f, CornerAll);
    Surface->TextRun(CentreText(*Surface, Left, "<", 20.0f), CentredY(Left, 20.0f),
                     Theme.Primary, "<", 20.0f, 0.0f, true);
    Surface->TextRun(CentreText(*Surface, Right, ">", 20.0f), CentredY(Right, 20.0f),
                     Theme.Primary, ">", 20.0f, 0.0f, true);

    const float FontMaximum = static_cast<float>(FontCount) * 208.0f - FontRail.Width();
    if (Pressed(142u, Left))
    {
        FontFrom = FontScroll;
        FontTarget = FontScroll - 250.0f;
        if (FontTarget < 0.0f) FontTarget = 0.0f;
        Motion->Eased(FontMotion).Depart(0.0, 1.0, 250.0, 0.0, EaseCurve::Carousel);
    }
    if (Pressed(143u, Right))
    {
        FontFrom = FontScroll;
        FontTarget = FontScroll + 250.0f;
        if (FontTarget > FontMaximum) FontTarget = FontMaximum;
        Motion->Eased(FontMotion).Depart(0.0, 1.0, 250.0, 0.0, EaseCurve::Carousel);
    }

    const PlaneExtent Specimen = Spanning(ContentLeft, SpecimenTop, ContentRight - ContentLeft, 176.0f);
    Surface->Ground(Specimen, Theme.Card, static_cast<float>(Configuration.Radius < 24u ? 24u : Configuration.Radius),
                    CornerAll);
    Surface->Edge(Specimen, Theme.Edge, 1.0f, 24.0f, CornerAll);
    Surface->TextRun(Specimen.MinimumX + 32.0f, Specimen.MinimumY + 25.0f, Theme.Secondary, "TYPEFACE & COLORS",
                     12.0f, .12f, false, RoleWeightOf(Configuration.TypographyWeight, 5u));
    Surface->TextRun(Specimen.MinimumX + 32.0f, Specimen.MinimumY + 58.0f, Theme.Primary, FamilyAt(Configuration.Font),
                     48.0f, 0.0f, true);
    Surface->TextRun(Specimen.MaximumX - 330.0f, Specimen.MinimumY + 45.0f, Theme.Secondary,
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 13.0f);
    Surface->TextRun(Specimen.MaximumX - 330.0f, Specimen.MinimumY + 70.0f, Theme.Secondary,
                     "abcdefghijklmnopqrstuvwxyz", 13.0f);
    Surface->TextRun(Specimen.MaximumX - 330.0f, Specimen.MinimumY + 95.0f, Theme.Secondary, "0123456789", 13.0f);

    static const char* Roles[8] = {"Title", "Header", "Subheader", "Body", "Label", "Caption", "Warning", "Alert"};
    static const std::uint32_t Minimum[8] = {20u, 16u, 12u, 10u, 8u, 8u, 10u, 10u};
    static const std::uint32_t Maximum[8] = {64u, 40u, 32u, 24u, 20u, 16u, 24u, 24u};
    float Cursor = Specimen.MaximumY + 30.0f;
    for (std::uint32_t Ordinal = 0u; Ordinal < 8u; ++Ordinal)
    {
        const float PreviewText = static_cast<float>(Configuration.TypographySize[Ordinal]);
        const float EntryHeight = EntryHeightOf(Configuration.TypographySize[Ordinal]);
        const PlaneExtent Entry = Spanning(ContentLeft, Cursor, ContentRight - ContentLeft, EntryHeight);
        Surface->Ground(Entry, Theme.Card, 16.0f, CornerAll);
        Surface->Edge(Entry, Theme.Edge, 1.0f, 16.0f, CornerAll);

        // 📝 The role name is drawn in the role's own face, so the strip's choice is legible in the label
        //    that owns it — a Title row whose strip applied Black names itself in Black.
        const FontWeight RoleWeight = RoleWeightOf(Configuration.TypographyWeight, Ordinal);
        Surface->TextRun(Entry.MinimumX + 18.0f, Entry.MinimumY + 12.0f, Theme.Primary, Roles[Ordinal],
                         16.0f, 0.0f, false, RoleWeight);
        const float LabelWidth = Surface->MeasureRun(Roles[Ordinal], 16.0f, 0.0f, RoleWeight);
        Surface->TextRun(Entry.MinimumX + 18.0f + LabelWidth + 14.0f, Entry.MinimumY + 17.0f,
                         Theme.Secondary, FamilyAt(Configuration.Font), 12.0f);

        // 📝 The same carousel the family rail above presents, driven by the family the main rail applies:
        //    one tile per available weight of that family, every tile drawing "Aa" in its own face. The
        //    press applies the weight for the role and nothing else, and the strip scrolls like the main rail.
        const float StripY = Entry.MinimumY + 40.0f;
        const PlaneExtent LeftArrow = Spanning(Entry.MinimumX + 18.0f, StripY + 21.0f, 26.0f, 30.0f);
        const PlaneExtent RightArrow = Spanning(Entry.MaximumX - 44.0f, StripY + 21.0f, 26.0f, 30.0f);
        const PlaneExtent RoleRail = {LeftArrow.MaximumX + 8.0f, StripY,
                                      RightArrow.MinimumX - 8.0f, StripY + 72.0f};

        const std::uint32_t WeightCount = [&]() -> std::uint32_t
        {
            std::uint32_t Count = 0u;
            for (const FontWeight Candidate : CandidateFaces)
            {
                if (FontArchive != nullptr && !FontArchive->HasFace(Candidate, FontSlant::Upright))
                    continue;
                ++Count;
            }
            return Count;
        }();
        constexpr float TileStep = 132.0f;
        constexpr float TileSpan = 120.0f;
        const float RoleMaximum = (static_cast<float>(WeightCount) * TileStep > RoleRail.Width())
                                ? static_cast<float>(WeightCount) * TileStep - RoleRail.Width() : 0.0f;

        const float RoleFraction = static_cast<float>(Motion->Eased(RoleFontMotion[Ordinal]).Current());
        RoleFontScroll[Ordinal] = RoleFontFrom[Ordinal] +
                                  (RoleFontTarget[Ordinal] - RoleFontFrom[Ordinal]) * RoleFraction;

        if (Pressed(RoleArrowBase + Ordinal * 2u, LeftArrow) && RoleFontTarget[Ordinal] > 0.0f)
        {
            RoleFontFrom[Ordinal] = RoleFontScroll[Ordinal];
            RoleFontTarget[Ordinal] = RoleFontScroll[Ordinal] - TileStep;
            if (RoleFontTarget[Ordinal] < 0.0f) RoleFontTarget[Ordinal] = 0.0f;
            Motion->Eased(RoleFontMotion[Ordinal]).Depart(0.0, 1.0, 250.0, 0.0, EaseCurve::Carousel);
        }
        if (Pressed(RoleArrowBase + Ordinal * 2u + 1u, RightArrow) && RoleFontTarget[Ordinal] < RoleMaximum)
        {
            RoleFontFrom[Ordinal] = RoleFontScroll[Ordinal];
            RoleFontTarget[Ordinal] = RoleFontScroll[Ordinal] + TileStep;
            if (RoleFontTarget[Ordinal] > RoleMaximum) RoleFontTarget[Ordinal] = RoleMaximum;
            Motion->Eased(RoleFontMotion[Ordinal]).Depart(0.0, 1.0, 250.0, 0.0, EaseCurve::Carousel);
        }

        Surface->Ground(LeftArrow, Theme.Card, 15.0f, CornerAll);
        Surface->Ground(RightArrow, Theme.Card, 15.0f, CornerAll);
        Surface->Edge(LeftArrow, Theme.Edge, 1.0f, 15.0f, CornerAll);
        Surface->Edge(RightArrow, Theme.Edge, 1.0f, 15.0f, CornerAll);
        Surface->TextRun(CentreText(*Surface, LeftArrow, "<", 14.0f), CentredY(LeftArrow, 14.0f),
                         Theme.Primary, "<", 14.0f, 0.0f, true);
        Surface->TextRun(CentreText(*Surface, RightArrow, ">", 14.0f), CentredY(RightArrow, 14.0f),
                         Theme.Primary, ">", 14.0f, 0.0f, true);

        Surface->Confine(RoleRail);
        std::uint32_t FaceOrdinal = 0u;
        for (std::uint32_t Candidate = 0u; Candidate < 9u; ++Candidate)
        {
            if (FontArchive != nullptr && !FontArchive->HasFace(CandidateFaces[Candidate], FontSlant::Upright))
                continue;
            const float FaceX = RoleRail.MinimumX + 4.0f +
                                    TileStep * static_cast<float>(FaceOrdinal) - RoleFontScroll[Ordinal];
            const PlaneExtent Tile = Spanning(FaceX, StripY, TileSpan, 72.0f);
            const bool Selected = Configuration.TypographyWeight[Ordinal] ==
                                  static_cast<std::uint32_t>(CandidateFaces[Candidate]);
            const PlaneExtent TileContact = {
                Tile.MinimumX > RoleRail.MinimumX ? Tile.MinimumX : RoleRail.MinimumX,
                Tile.MinimumY,
                Tile.MaximumX < RoleRail.MaximumX ? Tile.MaximumX : RoleRail.MaximumX,
                Tile.MaximumY
            };
            if (TileContact.MaximumX > TileContact.MinimumX)
            {
                if (FontArchive != nullptr)
                    Surface->ApplyFontPreview(FontArchive->Face(CandidateFaces[Candidate], FontSlant::Upright));
                Surface->Ground(Tile, Selected ? Theme.Card : Theme.Panel, 10.0f, CornerAll);
                Surface->Edge(Tile, Selected ? Theme.Edge : WithOpacity(Theme.Edge, 0.0f), 1.0f, 10.0f, CornerAll);
                Surface->TextRun(Tile.MinimumX + 10.0f, Tile.MinimumY + 7.0f, Theme.Primary, "Aa", 22.0f);
                Surface->TextRun(Tile.MinimumX + 10.0f, Tile.MinimumY + 38.0f, Theme.Primary,
                                 FaceNames[Candidate], 10.5f, 0.0f, true);
                Surface->TextRun(Tile.MinimumX + 10.0f, Tile.MinimumY + 54.0f, Theme.Secondary,
                                 FamilyAt(Configuration.Font), 9.5f);
                // 📝 The ordinal is the tile's VISIBLE slot, not its index in the face run: the strip scrolls
                //    a whole tile at a time, so slot arithmetic is exact, and a slot's identity must not move
                //    when the run beneath it changes.
                const std::uint32_t Slot = FaceOrdinal -
                                           static_cast<std::uint32_t>(RoleFontScroll[Ordinal] / TileStep + 0.5f);
                if (Slot < RoleTilePositions &&
                    Pressed(RoleTileBase + Ordinal * RoleTilePositions + Slot, TileContact))
                    Configuration.TypographyWeight[Ordinal] = static_cast<std::uint32_t>(CandidateFaces[Candidate]);
                Surface->ApplyFontPreview(nullptr);
            }
            ++FaceOrdinal;
        }
        Surface->Release();

        Slider(144u + Ordinal,
               Spanning(Entry.MinimumX + 18.0f, Entry.MinimumY + 130.0f,
                        420.0f, 34.0f),
               Minimum[Ordinal], Maximum[Ordinal], Configuration.TypographySize[Ordinal], "px", Theme.Edge, Accent);

        const PlaneExtent PreviewClip = {Entry.MinimumX + 18.0f, Entry.MinimumY + 170.0f,
                                         Entry.MaximumX - 18.0f, Entry.MaximumY - 10.0f};
        const float PreviewHeight = PreviewClip.MinimumY +
                                    (PreviewClip.Height() - PreviewText) * 0.5f;
        Surface->Confine(PreviewClip);
        Surface->TextRunTruncated(PreviewClip.MinimumX, PreviewHeight, PreviewClip.MaximumX,
                                  Ordinal == 6u   ? ThemeSpecification::Accent(Configuration.Warning).Colour
                                  : Ordinal == 7u ? ThemeSpecification::Accent(Configuration.Alert).Colour
                                                  : Theme.Primary,
                                  Ordinal == 4u   ? "METADATA · 10:42 AM · SYSTEM"
                                  : Ordinal == 5u ? "* This is a small caption text"
                                                  : "The quick brown fox jumps over the lazy dog",
                                  PreviewText, false, RoleWeight);
        Surface->Release();
        Cursor = Entry.MaximumY + 12.0f;
    }

    const PlaneExtent IconSection = Spanning(ContentLeft, Cursor - 4.0f,
                                             ContentRight - ContentLeft, 224.0f);
    Surface->Ground(IconSection, Theme.Card, 20.0f, CornerAll);
    Surface->Edge(IconSection, Theme.Edge, 1.0f, 20.0f, CornerAll);
    Surface->TextRun(IconSection.MinimumX + 20.0f, Cursor + 10.0f, Theme.Primary,
                     "Icon Style", 24.0f, 0.0f, false, RoleWeightOf(Configuration.TypographyWeight, 1u));
    const char* Styles[3] = {"Monotone", "Duotone", "Coloured"};
    for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
    {
        const PlaneExtent B = Spanning(IconSection.MinimumX + 20.0f +
                                           (IconSection.Width() - 40.0f) / 3.0f * Ordinal,
                                       Cursor + 52.0f, (IconSection.Width() - 40.0f) / 3.0f, 42.0f);
        Surface->Ground(B, Configuration.Icons == static_cast<IconAppearance>(Ordinal) ? Theme.Card : QuietDark, 12.0f,
                        CornerAll);
        Surface->TextRun(CentreText(*Surface, B, Styles[Ordinal], 13.0f), CentredY(B, 13.0f), Theme.Primary,
                         Styles[Ordinal], 13.0f);
        if (Pressed(160u + Ordinal, B)) Configuration.Icons = static_cast<IconAppearance>(Ordinal);
    }
    Cursor += 118.0f;
    Surface->TextRun(IconSection.MinimumX + 20.0f, Cursor, Theme.Primary, "Icon Font", 24.0f, 0.0f,
                     false, RoleWeightOf(Configuration.TypographyWeight, 1u));
    Slider(164u, Spanning(IconSection.MinimumX + 20.0f, Cursor + 40.0f, 420.0f, 40.0f), 16u, 48u,
           Configuration.IconSize, "px", Theme.Edge, Accent);
    for (std::uint32_t Icon = 0u; Icon < 4u; ++Icon)
        Symbol(Spanning(Extent.MaximumX - 220.0f + 50.0f * Icon, Cursor + 30.0f,
                        static_cast<float>(Configuration.IconSize), static_cast<float>(Configuration.IconSize)),
               Theme.Primary);
    Cursor += 124.0f;
    const PlaneExtent AntialiasSection = Spanning(ContentLeft, Cursor - 18.0f,
                                                  ContentRight - ContentLeft, 116.0f);
    Surface->Ground(AntialiasSection, Theme.Card, 20.0f, CornerAll);
    Surface->Edge(AntialiasSection, Theme.Edge, 1.0f, 20.0f, CornerAll);
    Surface->TextRun(AntialiasSection.MinimumX + 20.0f, Cursor, Theme.Primary,
                     "Font Antialiasing", 24.0f, 0.0f, false, RoleWeightOf(Configuration.TypographyWeight, 1u));
    const char* Aa[3] = {"Subpixel (Auto)", "Grayscale", "None"};
    for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
    {
        const PlaneExtent B = Spanning(AntialiasSection.MinimumX + 20.0f +
                                           (AntialiasSection.Width() - 40.0f) / 3.0f * Ordinal,
                                       Cursor + 45.0f, (AntialiasSection.Width() - 40.0f) / 3.0f, 42.0f);
        Surface->Ground(B, Configuration.Antialiasing == Ordinal ? Theme.Card : QuietDark, 12.0f, CornerAll);
        Surface->TextRun(CentreText(*Surface, B, Aa[Ordinal], 13.0f), CentredY(B, 13.0f), Theme.Primary,
                         Aa[Ordinal], 13.0f);
        if (Pressed(168u + Ordinal, B)) Configuration.Antialiasing = Ordinal;
    }
}


void ControlCentrePanel::InputPage(const PlaneExtent& Extent, ControlCentreConfiguration& Configuration,
                                   const ThemeDeclaration& Theme, ThemeToken Accent)
{
    const float Width = (Extent.Width() < 768.0f) ? Extent.Width() : 768.0f;
    const float X = Extent.MinimumX + (Extent.Width() - Width) * .5f;
    Surface->TextRun(X, Extent.MinimumY, Theme.Primary, "Input Devices", 29.0f, 0.0f,
                     false, RoleWeightOf(Configuration.TypographyWeight, 0u));
    Surface->TextRun(X, Extent.MinimumY + 38.0f, Theme.Secondary, "Keyboard, mouse, and touch settings", 14.0f);
    const PlaneExtent Back = Spanning(X + Width - 44.0f, Extent.MinimumY, 40.0f, 40.0f);
    Surface->Ground(Back, Theme.Card, 20.0f, CornerAll);
    Symbol(Spanning(Back.MinimumX + 10.0f, Back.MinimumY + 10.0f, 20.0f, 20.0f), Theme.Primary);
    if (Pressed(172u, Back))
    {
        Configuration.Page = ControlCentrePage::Settings;
        Navigate(Configuration.Page);
    }
    const PlaneExtent Hotkeys = Spanning(X, Extent.MinimumY + 76.0f, Width, 620.0f);
    Surface->Ground(Hotkeys, Theme.Card, static_cast<float>(Configuration.Radius < 16u ? 16u : Configuration.Radius),
                    CornerAll);
    Surface->Edge(Hotkeys, Theme.Edge, 1.0f, 20.0f, CornerAll);
    Surface->TextRun(Hotkeys.MinimumX + 28.0f, Hotkeys.MinimumY + 24.0f, Theme.Primary, "Global Hotkeys", 24.0f,
                     0.0f, false, RoleWeightOf(Configuration.TypographyWeight, 1u));
    const PlaneExtent Preset = Spanning(Hotkeys.MaximumX - 184.0f, Hotkeys.MinimumY + 19.0f, 156.0f, 34.0f);
    Surface->Ground(Preset, QuietDark, 8.0f, CornerAll);
    Surface->TextRun(Preset.MinimumX + 12.0f, CentredY(Preset, 12.0f), Theme.Primary,
                     ShortcutSpecification::Caption(Configuration.InputPreset), 12.0f);
    if (Pressed(173u, Preset)) InputPresetOpen = !InputPresetOpen;
    std::uint32_t Count = 0u;
    const ShortcutDeclaration* Shortcuts = ShortcutSpecification::Shortcuts(Configuration.InputPreset, Count);
    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
    {
        const PlaneExtent Row = Spanning(Hotkeys.MinimumX + 24.0f, Hotkeys.MinimumY + 76.0f + 62.0f * Ordinal,
                                         Hotkeys.Width() - 48.0f, 54.0f);
        Surface->Ground(Row, Partial(0xFFFFFFu, .02), 12.0f, CornerAll);
        Surface->Edge(Row, Theme.Edge, 1.0f, 12.0f, CornerAll);
        Surface->TextRun(Row.MinimumX + 14.0f, Row.MinimumY + 10.0f, Theme.Primary, Shortcuts[Ordinal].Action,
                         14.0f, 0.0f, true);
        Surface->TextRun(Row.MinimumX + 14.0f, Row.MinimumY + 31.0f, Theme.Secondary, Shortcuts[Ordinal].Grouping,
                         11.0f);
        float KeyX = Row.MaximumX - 160.0f;
        if (Shortcuts[Ordinal].Chord.ControlEnabled)
        {
            Surface->Ground(Spanning(KeyX, Row.MinimumY + 11.0f, 38.0f, 32.0f), QuietDark, 8.0f, CornerAll);
            Surface->TextRun(KeyX + 7.0f, Row.MinimumY + 21.0f, Theme.Secondary, "Ctrl", 11.0f);
            KeyX += 44.0f;
        }
        const PlaneExtent Key = Spanning(KeyX, Row.MinimumY + 11.0f, 96.0f, 32.0f);
        Surface->Ground(Key, QuietDark, 8.0f, CornerAll);
        Surface->Edge(Key, Theme.Edge, 1.0f, 8.0f, CornerAll);
        Surface->TextRun(
            CentreText(*Surface, Key,
                       Configuration.ListeningShortcut == Ordinal ? "Listening..." : Shortcuts[Ordinal].Chord.Key, 11.0f),
            CentredY(Key, 11.0f), Theme.Primary,
            Configuration.ListeningShortcut == Ordinal ? "Listening..." : Shortcuts[Ordinal].Chord.Key, 11.0f);
        if (!InputPresetOpen && Pressed(174u + Ordinal, Key))
            Configuration.ListeningShortcut = Configuration.ListeningShortcut == Ordinal ? 0xFFFFFFFFu : Ordinal;
    }
    const float MouseTop = Hotkeys.MaximumY + 24.0f;
    const PlaneExtent Mouse = Spanning(X, MouseTop, Width, 150.0f);
    Surface->Ground(Mouse, Theme.Card, 20.0f, CornerAll);
    Surface->TextRun(Mouse.MinimumX + 28.0f, Mouse.MinimumY + 24.0f, Theme.Primary, "Mouse Settings", 24.0f, 0.0f,
                     false, RoleWeightOf(Configuration.TypographyWeight, 1u));
    Surface->TextRun(Mouse.MinimumX + 28.0f, Mouse.MinimumY + 72.0f, Theme.Primary, "Invert Scroll Direction",
                     14.0f);
    Toggle(184u, Spanning(Mouse.MaximumX - 76.0f, Mouse.MinimumY + 64.0f, 48.0f, 24.0f), Configuration.InvertScroll,
           Theme.Edge, Accent);
    Surface->TextRun(Mouse.MinimumX + 28.0f, Mouse.MinimumY + 116.0f, Theme.Primary, "Pointer Speed", 14.0f);
    Slider(185u, Spanning(Mouse.MaximumX - 388.0f, Mouse.MinimumY + 100.0f, 360.0f, 40.0f), 1u, 10u,
           Configuration.PointerSpeed, "", Theme.Edge, Accent);
    const PlaneExtent Touch = Spanning(X, Mouse.MaximumY + 24.0f, Width, 190.0f);
    Surface->Ground(Touch, Theme.Card, 20.0f, CornerAll);
    Surface->TextRun(Touch.MinimumX + 28.0f, Touch.MinimumY + 24.0f, Theme.Primary, "Touch & Stylus", 24.0f, 0.0f,
                     false, RoleWeightOf(Configuration.TypographyWeight, 1u));
    Surface->TextRun(Touch.MinimumX + 28.0f, Touch.MinimumY + 72.0f, Theme.Primary, "Enable Touch Gestures",
                     14.0f);
    Toggle(186u, Spanning(Touch.MaximumX - 76.0f, Touch.MinimumY + 64.0f, 48.0f, 24.0f), Configuration.TouchGestures,
           Theme.Edge, Accent);
    Surface->TextRun(Touch.MinimumX + 28.0f, Touch.MinimumY + 114.0f, Theme.Primary, "Stylus Pressure Sensitivity",
                     14.0f);
    Toggle(187u, Spanning(Touch.MaximumX - 76.0f, Touch.MinimumY + 106.0f, 48.0f, 24.0f), Configuration.PressureEnabled,
           Theme.Edge, Accent);
    const char* Actions[3] = {"Orbit", "Pan", "Select"};
    for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
    {
        const PlaneExtent B =
            Spanning(Touch.MaximumX - 260.0f + 78.0f * Ordinal, Touch.MinimumY + 146.0f, 72.0f, 32.0f);
        Surface->Ground(B, Configuration.TouchAction == Ordinal ? Theme.Card : QuietDark, 8.0f, CornerAll);
        Surface->TextRun(CentreText(*Surface, B, Actions[Ordinal], 11.0f), CentredY(B, 11.0f), Theme.Primary,
                         Actions[Ordinal], 11.0f);
        if (Pressed(188u + Ordinal, B)) Configuration.TouchAction = Ordinal;
    }

    if (InputPresetOpen)
    {
        const PlaneExtent Menu = Spanning(Preset.MinimumX, Preset.MaximumY + 6.0f, Preset.Width(), 108.0f);
        Surface->Ground(Menu, Theme.Card, 10.0f, CornerAll);
        Surface->Edge(Menu, Theme.Edge, 1.0f, 10.0f, CornerAll);
        for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
        {
            const PlaneExtent Option = Spanning(Menu.MinimumX + 4.0f, Menu.MinimumY + 4.0f + 34.0f * Ordinal,
                                                Menu.Width() - 8.0f, 32.0f);
            if (Configuration.InputPreset == static_cast<ShortcutPreset>(Ordinal))
                Surface->Ground(Option, QuietDark, 7.0f, CornerAll);
            Surface->TextRun(Option.MinimumX + 10.0f, CentredY(Option, 11.0f), Theme.Primary,
                             ShortcutSpecification::Caption(static_cast<ShortcutPreset>(Ordinal)), 11.0f);
            if (Pressed(120u + Ordinal, Option))
            {
                Configuration.InputPreset = static_cast<ShortcutPreset>(Ordinal);
                InputPresetOpen = false;
            }
        }
    }
}

void ControlCentrePanel::Reset()
{
    Interaction.Reset();
    Motion = nullptr;
    Surface = nullptr;
    Appearance = nullptr;
    Pointer = {};
    CurrentPage = ControlCentrePage::Dashboard;
    PreviousPage = CurrentPage;
    PageMotion = 0u;
    TabMotion = 0u;
    ThemeMotion = 0u;
    FontMotion = 0u;
    CurrentTheme = ThemeSubject::Oled;
    PreviousTheme = ThemeSubject::Oled;
    for (std::uint32_t Ordinal = 0u;
         Ordinal < static_cast<std::uint32_t>(ControlCentrePage::PageCount); ++Ordinal)
    {
        ScrollMotion[Ordinal] = 0u;
        Scroll[Ordinal] = 0.0f;
        ScrollFrom[Ordinal] = 0.0f;
        ScrollTarget[Ordinal] = 0.0f;
    }
    FontScroll = 0.0f;
    FontFrom = 0.0f;
    FontTarget = 0.0f;
    for (std::uint32_t Ordinal = 0u; Ordinal < 8u; ++Ordinal)
    {
        RoleFontMotion[Ordinal] = 0u;
        RoleFontScroll[Ordinal] = 0.0f;
        RoleFontFrom[Ordinal] = 0.0f;
        RoleFontTarget[Ordinal] = 0.0f;
    }
    OpenPalette = 5u;
    InputPresetOpen = false;
}

} // namespace Slate
