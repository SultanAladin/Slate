//============================================================================================================================================
//                                                         GLOBALSHELLPANEL.CPP
//============================================================================================================================================
// 🧩 The reference shell recorded as primitives into one command list, in one order, with no vendor widget anywhere.

#include "SlateUI/Interface/GlobalShellPanel/Api/GlobalShellPanel.h"

#include <cmath>
#include <cstdio>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     ENTITY CLASSIFICATION
//------------------------------------------------------------------------------------------------------------------------

namespace
{

constexpr double RouseOver     = 120.0;   // [ms] - the reference's transition-colors duration
constexpr float  RunLeading    =   1.30f; // [-]  - leading-tight, for a two-run header
constexpr float  MedallionEdge =   6.0f;  // [px] - rounded-md on the header medallion

/// 🧩 Holds an ordinate between two bounds.
constexpr float Held(float Ordinate, float Least, float Most)
{
    return (Ordinate < Least) ? Least : (Ordinate > Most) ? Most : Ordinate;
}

/// 🧩 One ordinate of the way from a departed figure to an arriving one.
constexpr float Between(float Departed, float Arriving, float Fraction)
{
    return Departed + (Arriving - Departed) * Fraction;
}

/// 🧩 The same ink at a declared fraction of its own coverage.
constexpr InkOrdinate Faded(InkOrdinate Declared, float Fraction)
{
    const float Bounded = Held(Fraction, 0.0f, 1.0f);
    Declared.Opacity    = static_cast<std::uint8_t>(static_cast<float>(Declared.Opacity) * Bounded + 0.5f);
    return Declared;
}

/// 🧩 Whether a run holds another as a subsequence, without regard to capitalisation.
/// note  📝 The reference filters with `name.toLowerCase().includes(filterText.toLowerCase())`. This is the
///       same predicate over ASCII, written without allocating the two lowered copies that call would make.
bool RunHolds(const char* Subject, const char* Sought)
{
    if (Sought == nullptr || Sought[0] == '\0')
        return true;

    if (Subject == nullptr)
        return false;

    const auto Lowered = [](char Letter) -> char
    {
        return (Letter >= 'A' && Letter <= 'Z') ? static_cast<char>(Letter - 'A' + 'a') : Letter;
    };

    for (std::uint32_t Departure = 0u; Subject[Departure] != '\0'; ++Departure)
    {
        std::uint32_t Advanced = 0u;

        while (Sought[Advanced] != '\0' &&
               Lowered(Subject[Departure + Advanced]) == Lowered(Sought[Advanced]))
        {
            ++Advanced;
        }

        if (Sought[Advanced] == '\0')
            return true;
    }

    return false;
}

}   // namespace

SymbolSubject EntityGlyph(EntitySubject Subject)
{
    switch (Subject)
    {
        case EntitySubject::Level:      return SymbolSubject::CubeSolid;
        case EntitySubject::Grouping:   return SymbolSubject::FolderClosed;
        case EntitySubject::Actor:      return SymbolSubject::CubeSolid;
        case EntitySubject::Camera:     return SymbolSubject::CameraAperture;
        case EntitySubject::Illuminant: return SymbolSubject::BulbFilament;
        case EntitySubject::Audio:      return SymbolSubject::SpeakerCone;
        case EntitySubject::Particle:   return SymbolSubject::ParticleEmit;
        case EntitySubject::Trigger:    return SymbolSubject::CrosshairCentre;
        case EntitySubject::Script:     return SymbolSubject::CodeBrackets;
        default:                        return SymbolSubject::CubeSolid;
    }
}

InkOrdinate EntityHue(EntitySubject Subject)
{
    // 📐 The reference's `COLORS` record, transcribed verbatim from `components/GameOutliner.tsx`.
    switch (Subject)
    {
        case EntitySubject::Level:      return Covering(0xEAB308u);
        case EntitySubject::Grouping:   return Covering(0x8A8A8Au);
        case EntitySubject::Actor:      return Covering(0x3B82F6u);
        case EntitySubject::Camera:     return Covering(0xEC4899u);
        case EntitySubject::Illuminant: return Covering(0xF59E0Bu);
        case EntitySubject::Audio:      return Covering(0x8B5CF6u);
        case EntitySubject::Particle:   return Covering(0x10B981u);
        case EntitySubject::Trigger:    return Covering(0xEF4444u);
        case EntitySubject::Script:     return Covering(0x06B6D4u);
        default:                        return Covering(0x8A8A8Au);
    }
}

const char* EntityText(EntitySubject Subject)
{
    switch (Subject)
    {
        case EntitySubject::Level:      return "Level";
        case EntitySubject::Grouping:   return "Folder";
        case EntitySubject::Actor:      return "Actor";
        case EntitySubject::Camera:     return "Camera";
        case EntitySubject::Illuminant: return "Light";
        case EntitySubject::Audio:      return "Audio";
        case EntitySubject::Particle:   return "Particle";
        case EntitySubject::Trigger:    return "Trigger";
        case EntitySubject::Script:     return "Script";
        default:                        return "Entity";
    }
}

ShellMetric ScaleShellLengths(float Factor)
{
    const float Applied = (Factor > 0.0f) ? Factor : 1.0f;
    ShellMetric Scaled;

    // 📝 Every member is a length, so the whole record is scaled uniformly. The two run figures that are
    //    durations live outside it precisely so that this stays true and no member has to be excepted.
    float* const Lengths = &Scaled.TopBarAcross;
    const std::uint32_t Count = static_cast<std::uint32_t>(sizeof(ShellMetric) / sizeof(float));

    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
        Lengths[Ordinal] *= Applied;

    return Scaled;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       BRING-UP
//------------------------------------------------------------------------------------------------------------------------

Deliver<bool> GlobalShellPanel::Construct(InteractionIndex&              Interaction,
                                          MotionIntegrator&              Integrator,
                                          RecordingSurface&              Surface,
                                          const AppearanceSpecification& Resolved)
{
    if (Ledger != nullptr)
    {
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported,
                                       "the shell panel is already constructed" });
    }

    Ledger     = &Interaction;
    Motion     = &Integrator;
    this->Surface = &Surface;
    Appearance = &Resolved;

    if (!Controls.Construct(Interaction, Surface, Resolved).ContentPresent)
    {
        Reset();
        return Deliver<bool>::Refuse({ RefusalReason::CapabilityAbsent,
                                       "the shared inspector controls were refused" });
    }

    // 🔴 Every identity is claimed here and none inside a tick. A control enrolled mid-tick receives a fresh
    //    fade and reads as though the pointer had just arrived over it, once per tick, forever.
    ControlIdentity* const Every[] =
    {
        &DockSwitch,     &ModeButtons[0], &ModeButtons[1], &ModeButtons[2],
        &RetentionField,    &VeilContact
    };

    for (ControlIdentity* Claiming : Every)
    {
        const Deliver<ControlIdentity> Issued = Interaction.Enrol();

        if (!Issued.ContentPresent)
        {
            Reset();
            return Deliver<bool>::Refuse(Issued.Declined);
        }

        *Claiming = Issued.Resolve();
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < RowCeiling; ++Ordinal)
    {
        ControlIdentity* const PerRow[] =
        {
            &RowContacts[Ordinal], &RowDisclosures[Ordinal], &RowPresences[Ordinal]
        };

        for (ControlIdentity* Claiming : PerRow)
        {
            const Deliver<ControlIdentity> Issued = Interaction.Enrol();

            if (!Issued.ContentPresent)
            {
                Reset();
                return Deliver<bool>::Refuse(Issued.Declined);
            }

            *Claiming = Issued.Resolve();
        }
    }

    // 📐 The two-slide strip. The reference translates it by `-translate-x-1/2` over 300 ms on
    //    cubic-bezier(.5,.05,.2,1), which `EaseCurve::Carousel` already names exactly.
    const Deliver<std::uint32_t> Enrolled = Integrator.EnrolEased(0.0);

    if (!Enrolled.ContentPresent)
    {
        Reset();
        return Deliver<bool>::Refuse(Enrolled.Declined);
    }

    CarouselSlide = Enrolled.Resolve();

    Reseat(Resolved);

    return Deliver<bool>::Deliver(true);
}

void GlobalShellPanel::Advance(const PointerCondition& Contact, double Elapsed)
{
    Sampled = Contact;
    Controls.Advance(Contact, Elapsed);
}

void GlobalShellPanel::Reseat(const AppearanceSpecification& Resolved)
{
    Appearance = &Resolved;

    // 📝 The shell is authored at engine density, exactly as the workspace strip is, so it takes the display
    //    and artist factors rather than the control sheet's authored reduction.
    const float Applied = static_cast<float>(Resolved.Measure.DisplayScale)
                        * Resolved.ControlMeasure.ArtistFactor;

    Scaled = ScaleShellLengths(Applied);
}

void GlobalShellPanel::Reset()
{
    Controls.Reset();

    Ledger        = nullptr;
    Motion        = nullptr;
    Surface       = nullptr;
    Appearance    = nullptr;
    Sampled       = {};
    CarouselSlide = 0u;
    InspectorAt   = {};
    InspectorStood = false;
    DockSwitch    = {};
    RetentionField   = {};
    VeilContact   = {};

    for (ControlIdentity& Claimed : ModeButtons)
        Claimed = {};

    for (std::uint32_t Ordinal = 0u; Ordinal < RowCeiling; ++Ordinal)
    {
        RowContacts[Ordinal]    = {};
        RowDisclosures[Ordinal] = {};
        RowPresences[Ordinal]   = {};
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE KEY RULES
//------------------------------------------------------------------------------------------------------------------------

bool GlobalShellPanel::AdvanceSummoning(ShellOrdinates& Seated, bool Summoned, bool Withdrawn)
{
    bool Altered = false;

    // 📐 `app/page.tsx`, verbatim. Docked, Tab is only ever the slide. Undocked, the FIRST Tab opens the
    //    menu and does nothing else — the inspector is not flipped by the same press that summoned the card,
    //    which is what makes the second Tab read as "and now slide".
    if (Summoned)
    {
        if (Seated.InspectorDocked)
        {
            Seated.InspectorShown = !Seated.InspectorShown;
        }
        else if (!Seated.MenuOpened)
        {
            Seated.MenuOpened = true;
        }
        else
        {
            Seated.InspectorShown = !Seated.InspectorShown;
        }

        Altered = true;
    }

    // 📐 Escape is guarded by `menuOpen && !isDocked` in the reference, so a docked inspector is NOT closed
    //    by it. The inspector goes first and the menu second, so one press never dismisses both.
    if (Withdrawn && Seated.MenuOpened && !Seated.InspectorDocked)
    {
        if (Seated.InspectorShown)
            Seated.InspectorShown = false;
        else
            Seated.MenuOpened = false;

        Altered = true;
    }

    if (Altered && Motion != nullptr)
    {
        EasedInterpolant& Travelling = Motion->Eased(CarouselSlide);

        Travelling.Depart(Travelling.Standing(), Seated.InspectorShown ? 1.0 : 0.0,
                          CarouselTravelOver, 0.0, EaseCurve::Carousel);
    }

    return Altered;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE SHARED CHROME
//------------------------------------------------------------------------------------------------------------------------

void GlobalShellPanel::RecordPaneHeader(const PlaneExtent& Extent, SymbolSubject Glyph, InkOrdinate GlyphInk,
                                        InkOrdinate MedallionInk, const char* Title, const char* Secondary)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    // 📝 The hairline is recorded as a one-pixel ground at the lower edge rather than as an Edge, because
    //    `border-b` is one side and `Edge` strokes four.
    const PlaneExtent Hairline = Spanning(Extent.LeastAlong, Extent.MostAcross - 1.0f,
                                          Extent.SpanAlong(), 1.0f);
    Surface->Ground(Hairline, Tinted.Hairline, 0.0f, CornerNone);

    const float Medallion = Scaled.MedallionExtent;
    const float Centred   = Extent.LeastAcross + (Extent.SpanAcross() - Medallion) * 0.5f;

    const PlaneExtent Seat = Spanning(Extent.LeastAlong + Scaled.HeaderPadAlong, Centred,
                                      Medallion, Medallion);

    Surface->Ground(Seat, MedallionInk, MedallionEdge, CornerAll);

    // 📐 The figure sits in a 14 px square inside a 24 px medallion, centred on both axes.
    const float FigureExtent = Medallion * (14.0f / 24.0f);
    const float FigureLead   = Seat.LeastAlong  + (Medallion - FigureExtent) * 0.5f;
    const float FigureAcross = Seat.LeastAcross + (Medallion - FigureExtent) * 0.5f;

    Surface->Stroke(Glyph, Spanning(FigureLead, FigureAcross, FigureExtent, FigureExtent), GlyphInk);

    const float RunLead = Seat.MostAlong + Scaled.HeaderPadAlong * 0.8f;

    if (Secondary != nullptr && Secondary[0] != '\0')
    {
        // 📝 Two runs, `leading-tight`, seated as a pair about the header's centre rather than each about
        //    its own — which is what `flex-col` inside a centred row actually produces.
        const float PrimaryRun   = Scaled.RunPrimary;
        const float SecondaryRun = Scaled.RunFine;
        const float PairAcross   = PrimaryRun * RunLeading + SecondaryRun * RunLeading;
        const float PairLead     = Extent.LeastAcross + (Extent.SpanAcross() - PairAcross) * 0.5f;

        Surface->TextRunTruncated(RunLead, PairLead, Extent.MostAlong - RunLead - Scaled.HeaderPadAlong,
                                  Tinted.Primary, Title, PrimaryRun, true);
        Surface->TextRunTruncated(RunLead, PairLead + PrimaryRun * RunLeading,
                                  Extent.MostAlong - RunLead - Scaled.HeaderPadAlong,
                                  Tinted.Faint, Secondary, SecondaryRun, false);
    }
    else
    {
        const float SoleRun  = Scaled.RunPrimary;
        const float SoleLead = Extent.LeastAcross + (Extent.SpanAcross() - SoleRun) * 0.5f;

        Surface->TextRunTruncated(RunLead, SoleLead, Extent.MostAlong - RunLead - Scaled.HeaderPadAlong,
                                  Tinted.Primary, Title, SoleRun, true);
    }
}

void GlobalShellPanel::RecordTopBar(const PlaneExtent& Extent, const ShellOrdinates& Seated)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    const PlaneExtent Hairline = Spanning(Extent.LeastAlong, Extent.MostAcross - 1.0f,
                                          Extent.SpanAlong(), 1.0f);
    Surface->Ground(Hairline, Tinted.Hairline, 0.0f, CornerNone);

    const float Glyph  = 18.0f * (Scaled.TopBarAcross / 36.0f);
    const float Middle = Extent.LeastAcross + (Extent.SpanAcross() - Glyph) * 0.5f;
    float       Cursor = Extent.LeastAlong + Scaled.TopBarPadAlong;

    // 📝 The accent cube, `w-[18px] h-[18px] text-[var(--accent)]`.
    Surface->Stroke(SymbolSubject::CubeSolid, Spanning(Cursor, Middle, Glyph, Glyph), Tinted.Accent);
    Cursor += Glyph + Scaled.TopBarPadAlong * 0.85f;

    const char* Naming = (Seated.Mode == WorkspaceMode::Drafting)     ? "DraftingWorkspace"
                       : (Seated.Mode == WorkspaceMode::TexturePaint) ? "Texture Paint"
                                                                      : "World Editor";
    const char* Record = (Seated.Mode == WorkspaceMode::WorldEditor)  ? "Level_01_City.map"
                                                                      : "Bracket_Rev4.wsdoc";

    const float TitleRun = Scaled.RunPrimary;
    const float TitleTop = Extent.LeastAcross + (Extent.SpanAcross() - TitleRun) * 0.5f;

    // 📐 `tracking-wide` is 0.025em, which this seam takes in em directly.
    Surface->TextRun(Cursor, TitleTop, Tinted.Primary, Naming, TitleRun, 0.025f, true);
    Cursor += Surface->MeasureRun(Naming, TitleRun, 0.025f) + Scaled.TopBarPadAlong * 0.85f;

    const float RecordRun = Scaled.RunSecondary * (11.0f / 11.5f);
    const float RecordTop = Extent.LeastAcross + (Extent.SpanAcross() - RecordRun) * 0.5f;

    Surface->TextRun(Cursor, RecordTop, Tinted.Faint, Record, RecordRun);

    // 📐 The trailing pill: `Tab  summon inspector`, rounded-full at px-2.5 py-1.
    const char* Chord   = "Tab";
    const char* Trailing = "  summon inspector";
    const float PillRun = Scaled.RunSmall;
    const float PadAlong = 10.0f * (Scaled.TopBarAcross / 36.0f);
    const float PillAlong = Surface->MeasureRun(Chord, PillRun, 0.0f)
                          + Surface->MeasureRun(Trailing, PillRun, 0.0f) + PadAlong * 2.0f;
    const float PillAcross = PillRun * 2.0f;
    const float PillLead   = Extent.MostAlong - Scaled.TopBarPadAlong - PillAlong;
    const float PillTop    = Extent.LeastAcross + (Extent.SpanAcross() - PillAcross) * 0.5f;

    const PlaneExtent Pill = Spanning(PillLead, PillTop, PillAlong, PillAcross);

    Surface->Ground(Pill, Tinted.Menu, PillAcross * 0.5f, CornerAll);
    Surface->Edge(Pill, Tinted.Hairline, 1.0f, PillAcross * 0.5f, CornerAll);

    const float ChordTop = PillTop + (PillAcross - PillRun) * 0.5f;

    Surface->TextRun(PillLead + PadAlong, ChordTop, Tinted.Primary, Chord, PillRun, 0.0f, true);
    Surface->TextRun(PillLead + PadAlong + Surface->MeasureRun(Chord, PillRun, 0.0f), ChordTop,
                     Tinted.Muted, Trailing, PillRun);
}

void GlobalShellPanel::RecordOptionsRail(const PlaneExtent& Extent, ShellOrdinates& Seated)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    // 📝 `border-r`, the one trailing side.
    const PlaneExtent Hairline = Spanning(Extent.MostAlong - 1.0f, Extent.LeastAcross,
                                          1.0f, Extent.SpanAcross());
    Surface->Ground(Hairline, Tinted.HairlineFirm, 0.0f, CornerNone);

    const PlaneExtent Header = Spanning(Extent.LeastAlong, Extent.LeastAcross,
                                        Extent.SpanAlong(), Scaled.HeaderAcross);

    const float HeaderRun = Scaled.RunPrimary;
    const float HeaderTop = Header.LeastAcross + (Header.SpanAcross() - HeaderRun) * 0.5f;
    const float HeaderPad = Scaled.HeaderPadAlong * 1.6f;

    Surface->TextRun(Header.LeastAlong + HeaderPad, HeaderTop, Tinted.Primary, "Options",
                     HeaderRun, 0.025f, true);

    const PlaneExtent HeaderEdge = Spanning(Header.LeastAlong, Header.MostAcross - 1.0f,
                                            Header.SpanAlong(), 1.0f);
    Surface->Ground(HeaderEdge, Tinted.Hairline, 0.0f, CornerNone);

    const float BodyPad = Scaled.HeaderPadAlong * 1.6f;
    float       Cursor  = Header.MostAcross + BodyPad;

    // ① Dock Inspector — the reference's `.switch` and `.nub`, which `ControlPanel` already reproduces.
    const float  SwitchAcross = Scaled.RowAcross * 0.72f;
    const PlaneExtent SwitchRow = Spanning(Extent.LeastAlong + BodyPad, Cursor,
                                           Extent.SpanAlong() - BodyPad * 2.0f, SwitchAcross);

    SwitchDeclaration Docking;
    Docking.Caption = "Dock Inspector";

    const bool DockedBefore = Seated.InspectorDocked;

    const ControlVerdict Docked = Controls.SwitchToggle(DockSwitch, SwitchRow, Docking,
                                                       Seated.InspectorDocked);
    static_cast<void>(Docked);

    // 🔴 Docking and undocking re-seats the slide rather than letting it travel. The reference swaps which
    //    container holds the very same element, so the strip is mounted fresh at its current offset — a
    //    travel here would slide a panel the artist never asked to move.
    if (DockedBefore != Seated.InspectorDocked && Motion != nullptr)
        Motion->Eased(CarouselSlide).Seat(Seated.InspectorShown ? 1.0 : 0.0);

    Cursor += SwitchAcross + BodyPad * 0.5f;

    const char* Explaining = Seated.InspectorDocked
                           ? "Inspector is docked to the right side of the screen."
                           : "Inspector is hidden. Press Tab to summon it.";

    // 📐 `text-[11px] leading-relaxed` — 1.625 line height, wrapped inside the rail's own extent.
    const float ProseRun  = Scaled.RunSecondary * (11.0f / 11.5f);
    const float ProseStep = ProseRun * 1.625f;
    const float ProseAlong = Extent.SpanAlong() - BodyPad * 2.0f;

    // 📝 Wrapped here rather than truncated: the reference wraps, and a truncated sentence would read as a
    //    defect. The break is taken at the last space that still fits.
    const char* Standing = Explaining;

    while (Standing != nullptr && *Standing != '\0')
    {
        std::uint32_t Admitted = 0u;
        std::uint32_t Breaking = 0u;
        char          Measured[96] = {};

        while (Standing[Admitted] != '\0' && Admitted + 1u < sizeof(Measured))
        {
            Measured[Admitted] = Standing[Admitted];
            Measured[Admitted + 1u] = '\0';

            if (Surface->MeasureRun(Measured, ProseRun, 0.0f) > ProseAlong)
                break;

            if (Standing[Admitted] == ' ')
                Breaking = Admitted;

            ++Admitted;
        }

        const bool Whole = Standing[Admitted] == '\0';
        const std::uint32_t Taken = Whole ? Admitted : (Breaking > 0u ? Breaking : Admitted);

        for (std::uint32_t Ordinal = 0u; Ordinal < Taken; ++Ordinal)
            Measured[Ordinal] = Standing[Ordinal];

        Measured[Taken] = '\0';

        Surface->TextRun(Extent.LeastAlong + BodyPad, Cursor, Tinted.Faint, Measured, ProseRun);
        Cursor += ProseStep;

        Standing += Taken;

        while (*Standing == ' ')
            ++Standing;
    }

    Cursor += BodyPad * 1.5f;

    // ② Workspace Mode — a caption, then the three buttons at `h-8` with `gap-2`.
    const float CaptionRun = Scaled.RunPrimary;

    Surface->TextRun(Extent.LeastAlong + BodyPad, Cursor, Tinted.Muted, "Workspace Mode", CaptionRun);
    Cursor += CaptionRun * 1.6f + BodyPad * 0.75f;

    const char* const ModeCaptions[3] = { "Drafting", "Texture Paint", "Game Editor" };
    const float ButtonAcross = 32.0f * (Scaled.RowAcross / 32.0f);
    const float ButtonGap    = 8.0f  * (Scaled.RowAcross / 32.0f);

    for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
    {
        const PlaneExtent Button = Spanning(Extent.LeastAlong + BodyPad, Cursor,
                                            Extent.SpanAlong() - BodyPad * 2.0f, ButtonAcross);

        const bool Taken  = static_cast<std::uint32_t>(Seated.Mode) == Ordinal;
        const bool Roused = Button.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

        if (Roused && Sampled.ContactArrived && !Ledger->AnyDisclosed())
            Ledger->Seize(ModeButtons[Ordinal], ControlPart::Body);

        if (Roused && Ledger->Released(ModeButtons[Ordinal]))
            Seated.Mode = static_cast<WorkspaceMode>(Ordinal);

        Ledger->DeclareRoused(ModeButtons[Ordinal], Roused, RouseOver);

        Surface->Ground(Button, Taken ? Tinted.AccentSoft : Tinted.Tile, Scaled.FieldRadius, CornerAll);
        Surface->Edge(Button, Taken ? Tinted.Accent
                                    : (Roused ? Covering(0x444444u) : Tinted.Hairline),
                      1.0f, Scaled.FieldRadius, CornerAll);

        const float ButtonRun = Scaled.RunSecondary * (11.0f / 11.5f);
        const float RunAlong  = Surface->MeasureRun(ModeCaptions[Ordinal], ButtonRun, 0.0f);

        Surface->TextRun(Button.LeastAlong + (Button.SpanAlong() - RunAlong) * 0.5f,
                         Button.LeastAcross + (Button.SpanAcross() - ButtonRun) * 0.5f,
                         Taken ? Tinted.Primary : Tinted.Muted,
                         ModeCaptions[Ordinal], ButtonRun, 0.0f, true);

        Cursor += ButtonAcross + ButtonGap;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE VIEWPORT
//------------------------------------------------------------------------------------------------------------------------

void GlobalShellPanel::RecordViewport(const PlaneExtent& Extent, const ShellOrdinates& Seated)
{
    Surface->Ground(Extent, Tinted.Desk, 0.0f, CornerNone);

    // 🔴 Confined before the weave. The lattice is recorded as discrete rules and the last one before each
    //    bound would otherwise overhang the viewport into the rail and the inspector.
    Surface->Confine(Extent);

    // 📐 Two lattices, exactly as the reference declares: 28 px at 0.028 coverage, then 140 px at 0.055.
    //    Each is one-pixel rules, which is what a `linear-gradient(… 1px, transparent 1px)` produces.
    const float Steps[2]        = { Scaled.WeaveFineStep, Scaled.WeaveCoarseStep };
    const InkOrdinate Inks[2]   = { Tinted.WeaveFine, Tinted.WeaveCoarse };

    for (std::uint32_t Pass = 0u; Pass < 2u; ++Pass)
    {
        const float Step = Steps[Pass];

        if (Step < 1.0f)
            continue;

        for (float Along = Extent.LeastAlong; Along < Extent.MostAlong; Along += Step)
        {
            Surface->Ground(Spanning(Along, Extent.LeastAcross, 1.0f, Extent.SpanAcross()),
                            Inks[Pass], 0.0f, CornerNone);
        }

        for (float Across = Extent.LeastAcross; Across < Extent.MostAcross; Across += Step)
        {
            Surface->Ground(Spanning(Extent.LeastAlong, Across, Extent.SpanAlong(), 1.0f),
                            Inks[Pass], 0.0f, CornerNone);
        }
    }

    // 📐 `radial-gradient(ellipse at 50% 45%, transparent 40%, rgba(0,0,0,.55) 100%)`. A radial ramp is not a
    //    surface primitive, so it is approximated by concentric rings stepping the coverage — sixteen rings
    //    is below the ordinate quantum of an eight-bit channel over this range, so no banding is visible.
    constexpr std::uint32_t RingCount = 16u;

    const float CentreAlong  = Extent.LeastAlong  + Extent.SpanAlong()  * 0.50f;
    const float CentreAcross = Extent.LeastAcross + Extent.SpanAcross() * 0.45f;
    const float Reach        = std::sqrt(Extent.SpanAlong()  * Extent.SpanAlong()
                                       + Extent.SpanAcross() * Extent.SpanAcross()) * 0.5f;

    for (std::uint32_t Ring = 0u; Ring < RingCount; ++Ring)
    {
        // 📝 Recorded outermost inward, each ring covering the whole disc at its own radius, so the coverage
        //    accumulates toward the edge exactly as a ramp from 40 % outward does.
        const float Fraction = 1.0f - static_cast<float>(Ring) / static_cast<float>(RingCount);
        const float Radius   = Reach * Between(0.40f, 1.0f, Fraction);
        const float Coverage = 0.55f / static_cast<float>(RingCount);

        Surface->Medallion(CentreAlong, CentreAcross, Radius,
                           Faded(Covering(0x000000u), Coverage));
    }

    // 📐 The centred hint, `press Tab to …`, with the second run present only while undocked.
    const char* Leading  = "press ";
    const char* Chord    = "Tab";
    const char* Trailing = Seated.InspectorDocked
                         ? ((Seated.Mode == WorkspaceMode::Drafting)     ? " to slide through properties"
                          : (Seated.Mode == WorkspaceMode::TexturePaint) ? " to slide through channels"
                                                                         : " to slide through components")
                         : ((Seated.Mode == WorkspaceMode::Drafting)     ? " to summon the scene directory"
                          : (Seated.Mode == WorkspaceMode::TexturePaint) ? " to summon layers"
                                                                         : " to summon the outliner");

    const float HintRun   = Scaled.RunSecondary * (12.0f / 11.5f);
    const float ChordPad  = 8.0f * (Scaled.RowAcross / 32.0f);
    const float ChordRun  = Scaled.RunSecondary * (11.0f / 11.5f);
    const float ChordAlong = Surface->MeasureRun(Chord, ChordRun, 0.0f) + ChordPad * 2.0f;
    const float HintAlong = Surface->MeasureRun(Leading, HintRun, 0.0f) + ChordAlong
                          + Surface->MeasureRun(Trailing, HintRun, 0.0f);

    float HintLead  = CentreAlong - HintAlong * 0.5f;
    const float HintTop = Extent.LeastAcross + Extent.SpanAcross() * 0.5f - HintRun;

    Surface->TextRun(HintLead, HintTop, Tinted.Faint, Leading, HintRun);
    HintLead += Surface->MeasureRun(Leading, HintRun, 0.0f);

    // 📐 The `kbd` cap: `border-b-2`, so the lower edge is drawn twice.
    const float ChordAcross = ChordRun * 1.9f;
    const PlaneExtent Cap   = Spanning(HintLead, HintTop + (HintRun - ChordAcross) * 0.5f,
                                       ChordAlong, ChordAcross);

    Surface->Ground(Cap, Tinted.Menu, Scaled.FieldRadius, CornerAll);
    Surface->Edge(Cap, Tinted.HairlineFirm, 1.0f, Scaled.FieldRadius, CornerAll);
    Surface->Ground(Spanning(Cap.LeastAlong, Cap.MostAcross - 2.0f, Cap.SpanAlong(), 2.0f),
                    Tinted.HairlineFirm, 0.0f, CornerNone);
    Surface->TextRun(Cap.LeastAlong + ChordPad, Cap.LeastAcross + (ChordAcross - ChordRun) * 0.5f,
                     Tinted.Primary, Chord, ChordRun, 0.0f, true);

    HintLead += ChordAlong;
    Surface->TextRun(HintLead, HintTop, Tinted.Faint, Trailing, HintRun);

    if (!Seated.InspectorDocked)
    {
        const char* SecondLeading  = "Tab";
        const char* SecondTrailing = (Seated.Mode == WorkspaceMode::Drafting)
                                   ? " again slides through to properties"
                                   : (Seated.Mode == WorkspaceMode::TexturePaint)
                                   ? " again slides through to channels"
                                   : " again slides through to components";

        const float SecondAlong = Surface->MeasureRun(SecondLeading, ChordRun, 0.0f) + ChordPad * 2.0f
                                + Surface->MeasureRun(SecondTrailing, HintRun, 0.0f);
        float       SecondLead  = CentreAlong - SecondAlong * 0.5f;
        const float SecondTop   = HintTop + HintRun * 1.9f;

        const PlaneExtent SecondCap = Spanning(SecondLead, SecondTop + (HintRun - ChordAcross) * 0.5f,
                                               Surface->MeasureRun(SecondLeading, ChordRun, 0.0f)
                                               + ChordPad * 2.0f, ChordAcross);

        Surface->Ground(SecondCap, Tinted.Menu, Scaled.FieldRadius, CornerAll);
        Surface->Edge(SecondCap, Tinted.HairlineFirm, 1.0f, Scaled.FieldRadius, CornerAll);
        Surface->Ground(Spanning(SecondCap.LeastAlong, SecondCap.MostAcross - 2.0f,
                                 SecondCap.SpanAlong(), 2.0f), Tinted.HairlineFirm, 0.0f, CornerNone);
        Surface->TextRun(SecondCap.LeastAlong + ChordPad,
                         SecondCap.LeastAcross + (ChordAcross - ChordRun) * 0.5f,
                         Tinted.Primary, SecondLeading, ChordRun, 0.0f, true);

        SecondLead += SecondCap.SpanAlong();
        Surface->TextRun(SecondLead, SecondTop, Tinted.Faint, SecondTrailing, HintRun);
    }

    // 📐 The lower hint strip: a scrim from transparent to rgba(0,0,0,.5), then the four navigation runs.
    const PlaneExtent Strip = Spanning(Extent.LeastAlong, Extent.MostAcross - Scaled.StatusAcross,
                                       Extent.SpanAlong(), Scaled.StatusAcross);

    Surface->Scrim(Strip, Faded(Covering(0x000000u), 0.0f), Faded(Covering(0x000000u), 0.5f),
                   ScrimAxis::Across);

    const char* const Navigating[4] = { "Orbit LMB", "Pan MMB", "Zoom Wheel", "Inspector Tab" };
    const float StatusRun = Scaled.RunSmall;
    const float StatusTop = Strip.LeastAcross + (Strip.SpanAcross() - StatusRun) * 0.5f;
    const float StatusGap = 9.0f * (Scaled.RowAcross / 32.0f);
    float       StatusLead = Strip.LeastAlong + 13.0f * (Scaled.RowAcross / 32.0f);

    for (std::uint32_t Ordinal = 0u; Ordinal < 4u; ++Ordinal)
    {
        Surface->TextRun(StatusLead, StatusTop, Tinted.Faint, Navigating[Ordinal], StatusRun);
        StatusLead += Surface->MeasureRun(Navigating[Ordinal], StatusRun, 0.0f) + StatusGap;

        if (Ordinal < 3u)
        {
            Surface->TextRun(StatusLead, StatusTop, Tinted.Unit, "\u00B7", StatusRun);
            StatusLead += Surface->MeasureRun("\u00B7", StatusRun, 0.0f) + StatusGap;
        }
    }

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE OUTLINER
//------------------------------------------------------------------------------------------------------------------------

bool GlobalShellPanel::RowPresented(const ShellOrdinates& Seated, const EntityRow* Rows,
                                    std::uint32_t RowCount, std::uint32_t Ordinal) const
{
    if (Ordinal >= RowCount)
        return false;

    const bool Retaining = Seated.EntityRetention[0] != '\0';

    // 📐 The reference retains a row when it matches, or when any row it holds matches. Retention also forces
    //    every branch open — `const isExpanded = node.expanded || !!filterText` — so disclosure is not
    //    consulted at all while a filter stands.
    if (Retaining)
    {
        if (RunHolds(Rows[Ordinal].Naming, Seated.EntityRetention))
            return true;

        for (std::uint32_t Enclosed = 0u; Enclosed < RowCount; ++Enclosed)
        {
            if (Rows[Enclosed].Enclosing == Ordinal && RowPresented(Seated, Rows, RowCount, Enclosed))
                return true;
        }

        return false;
    }

    // 📝 Walked outward rather than recursed inward, and bounded by the row count so a malformed enclosure
    //    cannot spin. A row is presented only when every enclosure above it stands disclosed.
    std::uint32_t Walking = Rows[Ordinal].Enclosing;
    std::uint32_t Walked  = 0u;

    while (Walking < RowCount && Walked++ <= RowCount)
    {
        if (!Seated.EntityExpanded[Walking])
            return false;

        Walking = Rows[Walking].Enclosing;
    }

    return true;
}

void GlobalShellPanel::RecordRetentionField(const PlaneExtent& Extent, ShellOrdinates& Seated)
{
    const bool Roused = Extent.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

    if (Roused && Sampled.ContactArrived)
        Ledger->Seize(RetentionField, ControlPart::Body);

    const bool Taken = Ledger->Holding(RetentionField) || Ledger->Disclosed(RetentionField);

    Surface->Ground(Extent, Tinted.MenuLower, Scaled.FieldRadius, CornerAll);
    Surface->Edge(Extent, Taken ? Partial(0xFFFFFFu, 0.22) : Tinted.Hairline,
                  1.0f, Scaled.FieldRadius, CornerAll);

    const float GlyphExtent = 14.0f * (Scaled.SearchAcross / 30.0f);
    const float GlyphLead   = Extent.LeastAlong + 10.0f * (Scaled.SearchAcross / 30.0f);
    const float GlyphTop    = Extent.LeastAcross + (Extent.SpanAcross() - GlyphExtent) * 0.5f;

    Surface->Stroke(SymbolSubject::MagnifierLens,
                    Spanning(GlyphLead, GlyphTop, GlyphExtent, GlyphExtent), Tinted.Faint);

    const float RunLead = GlyphLead + GlyphExtent + 8.0f * (Scaled.SearchAcross / 30.0f);
    const float FieldRun = Scaled.RunSecondary * (12.0f / 11.5f);
    const float RunTop   = Extent.LeastAcross + (Extent.SpanAcross() - FieldRun) * 0.5f;

    const bool Empty = Seated.EntityRetention[0] == '\0';

    Surface->TextRunTruncated(RunLead, RunTop, Extent.MostAlong - RunLead - 8.0f,
                              Empty ? Tinted.Faint : Tinted.Primary,
                              Empty ? "Filter Entities\u2026" : Seated.EntityRetention, FieldRun);
}

void GlobalShellPanel::RecordOutliner(const PlaneExtent& Extent, ShellOrdinates& Seated,
                                      const EntityRow* Rows, std::uint32_t RowCount)
{
    Surface->Ground(Extent, Tinted.Menu, 0.0f, CornerNone);
    Surface->Ground(Spanning(Extent.MostAlong - 1.0f, Extent.LeastAcross, 1.0f, Extent.SpanAcross()),
                    Tinted.Hairline, 0.0f, CornerNone);

    const PlaneExtent Header = Spanning(Extent.LeastAlong, Extent.LeastAcross,
                                        Extent.SpanAlong(), Scaled.HeaderAcross);

    RecordPaneHeader(Header, SymbolSubject::GearCog, Covering(0xFFFFFFu), Tinted.EntityAccent,
                     "World Outliner", "Level_01_City");

    // 📝 The header is declared `bg-[var(--menu)]` in the outliner and `--menu-2` elsewhere, so the shared
    //    header's ground is corrected here rather than parameterised into it for one caller.
    const float Pad = Scaled.PanePad;

    const PlaneExtent Retention = Spanning(Extent.LeastAlong + Pad, Header.MostAcross + Pad,
                                        Extent.SpanAlong() - Pad * 2.0f, Scaled.SearchAcross);

    RecordRetentionField(Retention, Seated);

    const PlaneExtent Footer = Spanning(Extent.LeastAlong, Extent.MostAcross - Scaled.FooterAcross,
                                        Extent.SpanAlong(), Scaled.FooterAcross);

    const PlaneExtent Body = Spanning(Extent.LeastAlong + Pad, Retention.MostAcross + Pad * 0.5f,
                                      Extent.SpanAlong() - Pad * 2.0f,
                                      Footer.LeastAcross - Retention.MostAcross - Pad);

    Surface->Confine(Body);

    float Cursor = Body.LeastAcross;

    for (std::uint32_t Ordinal = 0u; Ordinal < RowCount && Ordinal < RowCeiling; ++Ordinal)
    {
        if (!RowPresented(Seated, Rows, RowCount, Ordinal))
            continue;

        const EntityRow&  Presented = Rows[Ordinal];
        const PlaneExtent Row       = Spanning(Body.LeastAlong, Cursor,
                                               Body.SpanAlong(), Scaled.RowAcross);

        Cursor += Scaled.RowAcross;

        if (Surface->Excluded(Row))
            continue;

        const bool Taken  = Seated.EntityTaken == Ordinal;
        const bool Roused = Row.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);
        const bool Absent = !Seated.EntityPresent[Ordinal];
        const bool Branch = Presented.EnclosedCount > 0u;

        const float LeadAlong = Row.LeastAlong + Scaled.RowLeadAlong
                              + static_cast<float>(Presented.Depth) * Scaled.RowStepAlong;

        // ① The disclosure cell, which takes the contact before the row does.
        const PlaneExtent Chevron = Spanning(LeadAlong,
                                             Row.LeastAcross + (Row.SpanAcross() - Scaled.ChevronExtent) * 0.5f,
                                             Scaled.ChevronExtent, Scaled.ChevronExtent);

        const bool OnChevron = Branch && Chevron.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

        // ② The presence action at the trailing edge, which also outranks the row.
        const float PresenceExtent = Scaled.GlyphExtent * (20.0f / 18.0f);
        const PlaneExtent Presence = Spanning(Row.MostAlong - PresenceExtent - Scaled.PanePad,
                                              Row.LeastAcross + (Row.SpanAcross() - PresenceExtent) * 0.5f,
                                              PresenceExtent, PresenceExtent);

        const bool OnPresence = Presence.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

        if (Sampled.ContactArrived && !Ledger->AnyDisclosed())
        {
            if (OnChevron)
                Ledger->Seize(RowDisclosures[Ordinal], ControlPart::Chevron);
            else if (OnPresence)
                Ledger->Seize(RowPresences[Ordinal], ControlPart::Body);
            else if (Roused)
                Ledger->Seize(RowContacts[Ordinal], ControlPart::Body);
        }

        if (OnChevron && Ledger->Released(RowDisclosures[Ordinal]))
            Seated.EntityExpanded[Ordinal] = !Seated.EntityExpanded[Ordinal];

        if (OnPresence && Ledger->Released(RowPresences[Ordinal]))
        {
            // 📐 The reference applies the new presence to the row AND everything it holds, walking inward.
            const bool Arriving = !Seated.EntityPresent[Ordinal];

            Seated.EntityPresent[Ordinal] = Arriving;

            for (std::uint32_t Inward = Ordinal + 1u; Inward < RowCount && Inward < RowCeiling; ++Inward)
            {
                if (Rows[Inward].Depth <= Presented.Depth)
                    break;

                Seated.EntityPresent[Inward] = Arriving;
            }
        }

        if (Roused && !OnChevron && !OnPresence && Ledger->Released(RowContacts[Ordinal]))
            Seated.EntityTaken = Ordinal;

        Ledger->DeclareRoused(RowContacts[Ordinal], Roused, RouseOver);

        // ③ The row ground, then its rail. `opacity-50` for a withheld row is applied to every ink it draws.
        const float Coverage = Absent ? 0.5f : 1.0f;

        if (Taken)
            Surface->Ground(Row, Faded(Tinted.EntityTaken, Coverage), Scaled.FieldRadius, CornerAll);
        else if (Roused)
            Surface->Ground(Row, Faded(Tinted.RowRoused, Coverage), Scaled.FieldRadius, CornerAll);

        if (Taken)
        {
            const PlaneExtent Rail = Spanning(Row.LeastAlong - Scaled.RailOffsetAlong,
                                              Row.LeastAcross + (Row.SpanAcross() - Scaled.RailAcross) * 0.5f,
                                              Scaled.RailAlong, Scaled.RailAcross);

            Surface->Ground(Rail, Faded(Tinted.EntityAccent, Coverage), 2.0f,
                            CornerTrailingUpper | CornerTrailingLower);
        }

        if (Branch)
        {
            // 📐 `-rotate-90` while collapsed, which is the chevron-right figure rather than a turned one.
            Surface->Stroke(Seated.EntityExpanded[Ordinal] || Seated.EntityRetention[0] != '\0'
                            ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                            Chevron, Faded(Tinted.Faint, Coverage));
        }

        const float GlyphLead = LeadAlong + Scaled.ChevronExtent + Scaled.PanePad;
        const PlaneExtent Glyph = Spanning(GlyphLead,
                                           Row.LeastAcross + (Row.SpanAcross() - Scaled.GlyphExtent) * 0.5f,
                                           Scaled.GlyphExtent, Scaled.GlyphExtent);

        Surface->Stroke(EntityGlyph(Presented.Subject), Glyph,
                        Faded(EntityHue(Presented.Subject), Coverage));

        const float NamingRun  = Scaled.RunPrimary;
        const float NamingLead = Glyph.MostAlong + Scaled.PanePad;
        const float NamingTop  = Row.LeastAcross + (Row.SpanAcross() - NamingRun) * 0.5f;

        // 📝 The count and the presence action both sit to the trailing side, so the naming run is truncated
        //    against whichever of them stands.
        float NamingCeiling = Presence.LeastAlong - Scaled.PanePad;

        if (Branch)
        {
            char Counted[12] = {};
            std::snprintf(Counted, sizeof(Counted), "%u",
                          static_cast<unsigned>(Presented.EnclosedCount));

            const float CountRun  = Scaled.RunFine;
            const float CountLead = NamingCeiling - Surface->MeasureRun(Counted, CountRun, 0.0f);

            Surface->TextRun(CountLead, Row.LeastAcross + (Row.SpanAcross() - CountRun) * 0.5f,
                             Faded(Tinted.Faint, Coverage), Counted, CountRun);

            NamingCeiling = CountLead - Scaled.PanePad;
        }

        Surface->TextRunTruncated(NamingLead, NamingTop, NamingCeiling,
                                  Faded(Taken ? Tinted.Primary : (Roused ? Tinted.Primary : Tinted.Muted),
                                        Coverage),
                                  Presented.Naming, NamingRun);

        // 📐 The eye is `opacity-0` until the row is roused, and always present once the row is withheld.
        if (Roused || Absent)
        {
            if (OnPresence)
                Surface->Ground(Presence, Tinted.TileRoused, 3.0f, CornerAll);

            const float EyeExtent = PresenceExtent * (14.0f / 20.0f);
            const PlaneExtent Eye = Spanning(Presence.LeastAlong + (PresenceExtent - EyeExtent) * 0.5f,
                                             Presence.LeastAcross + (PresenceExtent - EyeExtent) * 0.5f,
                                             EyeExtent, EyeExtent);

            Surface->Stroke(Absent ? SymbolSubject::EyeClosed : SymbolSubject::EyeOpen, Eye,
                            OnPresence ? Tinted.Primary : Tinted.Faint);
        }
    }

    Surface->Release();

    // ④ The footer, `{count} entities`.
    Surface->Ground(Footer, Tinted.MenuLower, 0.0f, CornerNone);
    Surface->Ground(Spanning(Footer.LeastAlong, Footer.LeastAcross, Footer.SpanAlong(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    char Counted[16] = {};
    std::snprintf(Counted, sizeof(Counted), "%u", static_cast<unsigned>(RowCount));

    const float FooterRun = Scaled.RunFine;
    const float FooterTop = Footer.LeastAcross + (Footer.SpanAcross() - FooterRun) * 0.5f;
    const float FooterLead = Footer.LeastAlong + Scaled.HeaderPadAlong;

    Surface->TextRun(FooterLead, FooterTop, Tinted.Primary, Counted, FooterRun, 0.0f, true);
    Surface->TextRun(FooterLead + Surface->MeasureRun(Counted, FooterRun, 0.0f) + 4.0f, FooterTop,
                     Tinted.Muted, " entities", FooterRun);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE COMPONENTS
//------------------------------------------------------------------------------------------------------------------------

void GlobalShellPanel::RecordComponents(const PlaneExtent& Extent, const ShellOrdinates& Seated,
                                        const EntityRow* Rows, std::uint32_t RowCount)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    const bool Selected = Seated.EntityTaken < RowCount && Seated.EntityTaken < RowCeiling;

    const PlaneExtent Header = Spanning(Extent.LeastAlong, Extent.LeastAcross,
                                        Extent.SpanAlong(), Scaled.HeaderAcross);

    if (!Selected)
    {
        RecordPaneHeader(Header, SymbolSubject::CubeSolid, Tinted.Faint, Covering(0x111111u),
                         "Nothing selected", nullptr);

        const float ProseRun = Scaled.RunSecondary;
        const char* Prose    = "Select an entity to inspect its components.";
        const float ProseLead = Extent.LeastAlong
                              + (Extent.SpanAlong() - Surface->MeasureRun(Prose, ProseRun, 0.0f)) * 0.5f;

        Surface->TextRun(ProseLead, Header.MostAcross + Scaled.HeaderAcross, Tinted.Faint, Prose, ProseRun);
        return;
    }

    const EntityRow& Presented = Rows[Seated.EntityTaken];
    const InkOrdinate Hue      = EntityHue(Presented.Subject);

    char Classified[48] = {};
    std::snprintf(Classified, sizeof(Classified), "%s Entity", EntityText(Presented.Subject));

    RecordPaneHeader(Header, EntityGlyph(Presented.Subject), Hue, Covering(0x111111u),
                     Presented.Naming, Classified);

    // 📝 The header's secondary run carries the entity hue rather than the shared faint ink, so it is
    //    recorded again over the shared header's own.
    const float SecondaryRun = Scaled.RunFine;
    const float PairAcross   = Scaled.RunPrimary * RunLeading + SecondaryRun * RunLeading;
    const float PairLead     = Header.LeastAcross + (Header.SpanAcross() - PairAcross) * 0.5f;
    const float RunLead      = Header.LeastAlong + Scaled.HeaderPadAlong + Scaled.MedallionExtent
                             + Scaled.HeaderPadAlong * 0.8f;

    Surface->TextRunTruncated(RunLead, PairLead + Scaled.RunPrimary * RunLeading,
                              Header.MostAlong - RunLead - Scaled.HeaderPadAlong,
                              Hue, Classified, SecondaryRun, false);

    const float Pad = Scaled.PanePad;
    float       Cursor = Header.MostAcross + Pad;

    Surface->Confine(Spanning(Extent.LeastAlong, Header.MostAcross,
                              Extent.SpanAlong(), Extent.MostAcross - Header.MostAcross));

    // 📐 One card per component. The Transform card is presented for every subject except a level, a
    //    grouping and a script, exactly as the reference gates it.
    const auto RecordCard = [&](const char* Caption, const char* const* Rows2, std::uint32_t RowCount2)
    {
        const float BodyAcross = static_cast<float>(RowCount2) * Scaled.RowAcross + Pad * 2.0f;
        const PlaneExtent Card = Spanning(Extent.LeastAlong + Pad, Cursor,
                                          Extent.SpanAlong() - Pad * 2.0f,
                                          Scaled.ComponentAcross + BodyAcross);

        Surface->Ground(Card, Covering(0x0A0A0Bu), Scaled.CardRadius, CornerAll);
        Surface->Edge(Card, Tinted.Hairline, 1.0f, Scaled.CardRadius, CornerAll);

        const PlaneExtent CardHeader = Spanning(Card.LeastAlong, Card.LeastAcross,
                                                Card.SpanAlong(), Scaled.ComponentAcross);

        Surface->Ground(CardHeader, Tinted.MenuLower, Scaled.CardRadius,
                        CornerLeadingUpper | CornerTrailingUpper);
        Surface->Ground(Spanning(CardHeader.LeastAlong, CardHeader.MostAcross - 1.0f,
                                 CardHeader.SpanAlong(), 1.0f), Tinted.Hairline, 0.0f, CornerNone);

        const float CaptionRun = Scaled.RunSmall;

        // 📐 `uppercase tracking-wide` at 10.5 px.
        Surface->TextRunCapitalised(CardHeader.LeastAlong + Scaled.HeaderPadAlong,
                                    CardHeader.LeastAcross + (CardHeader.SpanAcross() - CaptionRun) * 0.5f,
                                    Tinted.Muted, Caption, CaptionRun, 0.025f, true);

        float RowCursor = CardHeader.MostAcross + Pad;

        for (std::uint32_t Ordinal = 0u; Ordinal < RowCount2; ++Ordinal)
        {
            const float LabelRun = Scaled.RunSecondary;
            const float LabelTop = RowCursor + (Scaled.RowAcross - LabelRun) * 0.5f;

            Surface->TextRun(Card.LeastAlong + Pad * 1.5f, LabelTop, Tinted.Muted,
                             Rows2[Ordinal], LabelRun);

            // 📝 The value cell, `--value-bg` at `--value-radius`, presented but not yet editable — the
            //    reference's own handlers are `onChange={()=>{}}` for every component field.
            const float CellAlong = Card.SpanAlong() * 0.45f;
            const PlaneExtent Cell = Spanning(Card.MostAlong - Pad * 1.5f - CellAlong,
                                              RowCursor + (Scaled.RowAcross - Scaled.SearchAcross * 0.8f) * 0.5f,
                                              CellAlong, Scaled.SearchAcross * 0.8f);

            Surface->Ground(Cell, Covering(0x232326u), Scaled.FieldRadius, CornerAll);

            RowCursor += Scaled.RowAcross;
        }

        Cursor = Card.MostAcross + Pad * 0.85f;
    };

    const bool Transforms = Presented.Subject != EntitySubject::Level
                         && Presented.Subject != EntitySubject::Grouping
                         && Presented.Subject != EntitySubject::Script;

    if (Transforms)
    {
        const char* const TransformRows[3] = { "Position", "Rotation", "Scale" };
        RecordCard("Transform", TransformRows, 3u);
    }

    char ComponentCaption[48] = {};
    std::snprintf(ComponentCaption, sizeof(ComponentCaption), "%s Component",
                  EntityText(Presented.Subject));

    // 📐 The per-subject field sets, transcribed from the reference's own component branch.
    switch (Presented.Subject)
    {
        case EntitySubject::Illuminant:
        {
            const char* const Fields[3] = { "Intensity", "Cast Shadows", "Light Color" };
            RecordCard(ComponentCaption, Fields, 3u);
            break;
        }
        case EntitySubject::Camera:
        {
            const char* const Fields[4] = { "Projection", "Field of View", "Near Clip", "Far Clip" };
            RecordCard(ComponentCaption, Fields, 4u);
            break;
        }
        case EntitySubject::Audio:
        {
            const char* const Fields[3] = { "Volume", "Looping", "Spatial 3D" };
            RecordCard(ComponentCaption, Fields, 3u);
            break;
        }
        case EntitySubject::Particle:
        {
            const char* const Fields[3] = { "Emit Rate", "Life Time", "Looping" };
            RecordCard(ComponentCaption, Fields, 3u);
            break;
        }
        case EntitySubject::Trigger:
        {
            const char* const Fields[2] = { "Event Tag", "Radius" };
            RecordCard(ComponentCaption, Fields, 2u);
            break;
        }
        case EntitySubject::Script:
        {
            const char* const Fields[2] = { "State", "Difficulty" };
            RecordCard(ComponentCaption, Fields, 2u);
            break;
        }
        case EntitySubject::Actor:
        {
            const char* const Fields[3] = { "Static Mesh", "Simulate Physics", "Generate Overlaps" };
            RecordCard(ComponentCaption, Fields, 3u);
            break;
        }
        case EntitySubject::Level:
        {
            const char* const Fields[2] = { "Level Name", "World Partition" };
            RecordCard(ComponentCaption, Fields, 2u);
            break;
        }
        default:
        {
            const char* const Fields[2] = { "Folder Name", "Is Editor Only" };
            RecordCard(ComponentCaption, Fields, 2u);
            break;
        }
    }

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE TWO-SLIDE STRIP
//------------------------------------------------------------------------------------------------------------------------

void GlobalShellPanel::RecordInspector(const PlaneExtent& Extent, ShellOrdinates& Seated,
                                       const EntityRow* Rows, std::uint32_t RowCount)
{
    // 📐 The reference lays a 200 %-wide strip inside the inspector and translates it by half its own extent.
    //    The strip is therefore two panes of the inspector's extent, and the travel is one whole extent.
    const float Travelled = (Motion != nullptr)
                          ? static_cast<float>(Motion->Eased(CarouselSlide).Standing())
                          : (Seated.InspectorShown ? 1.0f : 0.0f);

    const float Carried = -Held(Travelled, 0.0f, 1.0f) * Extent.SpanAlong();

    // 🔴 Confined to the inspector's own extent. The trailing slide begins one whole extent to the trailing
    //    side and would otherwise be recorded over the viewport and the rail beside it.
    Surface->Confine(Extent);

    const PlaneExtent Leading = Spanning(Extent.LeastAlong + Carried, Extent.LeastAcross,
                                         Extent.SpanAlong(), Extent.SpanAcross());
    const PlaneExtent Trailing = Spanning(Leading.MostAlong, Extent.LeastAcross,
                                          Extent.SpanAlong(), Extent.SpanAcross());

    // ① Slide one — the outliner and, beside it, the prompt the reference presents in Game Editor.
    if (!Surface->Excluded(Leading))
    {
        const float OutlinerAlong = (Scaled.OutlinerAlong < Leading.SpanAlong() * 0.6f)
                                  ? Scaled.OutlinerAlong : Leading.SpanAlong() * 0.6f;

        const PlaneExtent Outlining = Spanning(Leading.LeastAlong, Leading.LeastAcross,
                                               OutlinerAlong, Leading.SpanAcross());

        RecordOutliner(Outlining, Seated, Rows, RowCount);

        const PlaneExtent Prompting = Spanning(Outlining.MostAlong, Leading.LeastAcross,
                                               Leading.SpanAlong() - OutlinerAlong, Leading.SpanAcross());

        Surface->Ground(Prompting, Tinted.MenuLower, 0.0f, CornerNone);

        const char* Prose[3] =
        {
            "Select an entity in the Outliner and",
            "press Tab or double-click to view its",
            "properties in the Inspector slide."
        };

        const float ProseRun  = Scaled.RunSecondary;
        const float ProseStep = ProseRun * 1.625f;
        float       ProseTop  = Prompting.LeastAcross + (Prompting.SpanAcross() - ProseStep * 3.0f) * 0.5f;

        for (const char* Line : Prose)
        {
            const float LineAlong = Surface->MeasureRun(Line, ProseRun, 0.0f);

            Surface->TextRun(Prompting.LeastAlong + (Prompting.SpanAlong() - LineAlong) * 0.5f,
                             ProseTop, Tinted.Faint, Line, ProseRun);
            ProseTop += ProseStep;
        }
    }

    // ② Slide two — the component inspector for whatever the outliner has taken.
    if (!Surface->Excluded(Trailing))
        RecordComponents(Trailing, Seated, Rows, RowCount);

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE WHOLE SHELL
//------------------------------------------------------------------------------------------------------------------------

Deliver<bool> GlobalShellPanel::Record(const PlaneExtent&  Extent,
                                       ShellOrdinates&     Seated,
                                       const EntityRow*    Rows,
                                       std::uint32_t       RowCount)
{
    if (Ledger == nullptr || Surface == nullptr || Appearance == nullptr)
        return Deliver<bool>::Refuse({ RefusalReason::CapabilityAbsent, "the shell panel is unconstructed" });

    if (!Surface->Recording())
        return Deliver<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no tick stands adopted" });

    if (Rows == nullptr)
        RowCount = 0u;

    if (RowCount > RowCeiling)
        RowCount = RowCeiling;

    // 📝 The strip is carried by the same traverse whether it was moved by a key or by a press, so the
    //    target is restated every tick and the traverse departs only when it actually disagrees.
    if (Motion != nullptr)
    {
        EasedInterpolant& Travelling = Motion->Eased(CarouselSlide);
        const double      Wanted     = Seated.InspectorShown ? 1.0 : 0.0;

        if (Travelling.Settled && Travelling.Standing() != Wanted)
            Travelling.Depart(Travelling.Standing(), Wanted, CarouselTravelOver, 0.0, EaseCurve::Carousel);
    }

    // ① The desk, beneath everything.
    Surface->Ground(Extent, Tinted.Desk, 0.0f, CornerNone);

    // ② The top bar.
    const PlaneExtent TopBar = Spanning(Extent.LeastAlong, Extent.LeastAcross,
                                        Extent.SpanAlong(), Scaled.TopBarAcross);

    RecordTopBar(TopBar, Seated);

    // ③ The body: the Options rail, the viewport, and the docked inspector when it stands.
    const PlaneExtent Body = Spanning(Extent.LeastAlong, TopBar.MostAcross,
                                      Extent.SpanAlong(), Extent.MostAcross - TopBar.MostAcross);

    const PlaneExtent Rail = Spanning(Body.LeastAlong, Body.LeastAcross,
                                      Scaled.OptionsAlong, Body.SpanAcross());

    RecordOptionsRail(Rail, Seated);

    // 📐 The docked inspector is 700 px, but never more than what remains beside the rail — the reference's
    //    `flex-none` beside a `flex-1` viewport collapses the viewport first, and this reproduces that
    //    without letting the inspector overrun the rail on a narrow display.
    const float Remaining     = Body.MostAlong - Rail.MostAlong;
    const float InspectorSpan = Seated.InspectorDocked
                              ? ((Scaled.InspectorAlong < Remaining * 0.75f)
                                 ? Scaled.InspectorAlong : Remaining * 0.75f)
                              : 0.0f;

    const PlaneExtent Viewport = Spanning(Rail.MostAlong, Body.LeastAcross,
                                          Remaining - InspectorSpan, Body.SpanAcross());

    RecordViewport(Viewport, Seated);

    InspectorStood = false;

    if (Seated.InspectorDocked)
    {
        const PlaneExtent Docked = Spanning(Viewport.MostAlong, Body.LeastAcross,
                                            InspectorSpan, Body.SpanAcross());

        Surface->Ground(Docked, Tinted.Menu, 0.0f, CornerNone);
        Surface->Ground(Spanning(Docked.LeastAlong, Docked.LeastAcross, 1.0f, Docked.SpanAcross()),
                        Tinted.HairlineFirm, 0.0f, CornerNone);

        RecordInspector(Docked, Seated, Rows, RowCount);

        InspectorAt    = Docked;
        InspectorStood = true;
    }

    // ④ The veil and the summoned card — recorded LAST, so they are above everything written before them.
    // 🔴 This ordering IS the fix. The defect these replace painted a full-extent ground into the background
    //    command list while the panels beneath it were vendor windows, which composite above that list — so
    //    the veil darkened the display and the panels stayed legible and pressable straight through it. One
    //    list, written in order, cannot express that condition at all.
    if (!Seated.InspectorDocked && Seated.MenuOpened)
    {
        Surface->Ground(Extent, Tinted.Veil, 0.0f, CornerNone);

        const bool OnVeil = Extent.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

        if (OnVeil && Sampled.ContactArrived)
            Ledger->Seize(VeilContact, ControlPart::Body);

        const float CardAlong  = (Scaled.InspectorAlong < Extent.SpanAlong() - Scaled.OptionsAlong)
                               ? Scaled.InspectorAlong : Extent.SpanAlong() * 0.75f;
        const float CardAcross = (Scaled.SummonedAcross < Extent.SpanAcross() - Scaled.TopBarAcross * 2.0f)
                               ? Scaled.SummonedAcross : Extent.SpanAcross() * 0.75f;

        const PlaneExtent Card = Spanning(Extent.LeastAlong + (Extent.SpanAlong() - CardAlong) * 0.5f,
                                          Extent.LeastAcross + (Extent.SpanAcross() - CardAcross) * 0.5f,
                                          CardAlong, CardAcross);

        // 📝 The veil dismisses the menu only when the contact was NOT inside the card. The reference relies
        //    on the card being a later sibling that stops the click; one list has no bubbling, so the test
        //    is stated here.
        if (Ledger->Released(VeilContact) &&
            !Card.Encloses(Sampled.PositionAlong, Sampled.PositionAcross))
        {
            Seated.MenuOpened = false;
        }

        Surface->Ground(Card, Tinted.Menu, Scaled.MenuRadius, CornerAll);
        Surface->Edge(Card, Tinted.HairlineFirm, 1.0f, Scaled.MenuRadius, CornerAll);

        // 🔴 The card's content is clipped to its own rounded extent, so the outliner's square corners do
        //    not fill the radius the card just drew.
        Surface->Confine(Card);
        RecordInspector(Card, Seated, Rows, RowCount);
        Surface->Release();

        // 📝 The rounded corners are restored over the content, which was recorded square inside them.
        Surface->MaskCorners(Card, Tinted.Veil, Scaled.MenuRadius);
        Surface->Edge(Card, Tinted.HairlineFirm, 1.0f, Scaled.MenuRadius, CornerAll);

        InspectorAt    = Card;
        InspectorStood = true;
    }

    return Deliver<bool>::Deliver(true);
}

bool GlobalShellPanel::Occluding(float Along, float Across) const
{
    if (!InspectorStood)
        return false;

    return InspectorAt.Encloses(Along, Across);
}

}   // namespace Slate
