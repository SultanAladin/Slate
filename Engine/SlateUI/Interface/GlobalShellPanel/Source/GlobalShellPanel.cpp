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

constexpr double RouseOver         = 120.0;   // [ms] - the reference's transition-colors duration
constexpr double ContextArriveOver = 100.0;   // [ms] - the context card's `transition={{ duration: 0.1 }}`
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

/// 🧩 The same colour at a declared fraction of its own coverage.
constexpr ThemeToken Faded(ThemeToken Declared, float Fraction)
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

ThemeToken EntityHue(EntitySubject Subject)
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

ThemeToken RevisionHue(RevisionSubject Classified)
{
    // 📐 The reference's `REVISION_HUE` record, transcribed verbatim from `components/Inspector.tsx`.
    switch (Classified)
    {
        case RevisionSubject::Start:     return Covering(0x7EC8FFu);
        case RevisionSubject::Feature:   return Covering(0xFFB24Du);
        case RevisionSubject::Parameter: return Covering(0x4FD18Bu);
        case RevisionSubject::Sketch:    return Covering(0x37D6D6u);
        case RevisionSubject::Relocate:  return Covering(0x5B8CFFu);
        case RevisionSubject::Grouped:   return Covering(0xB98BFFu);
        case RevisionSubject::Created:   return Covering(0x7EC8FFu);
        case RevisionSubject::Amended:   return Covering(0xC99B6Au);
        case RevisionSubject::Dropped:   return Covering(0xFF6B6Bu);
        default:                          return Covering(0xC99B6Au);
    }
}

const char* RevisionText(RevisionSubject Classified)
{
    // 📐 The `label` of the reference's `REVISION_CLASS` record, verbatim.
    switch (Classified)
    {
        case RevisionSubject::Start:     return "Start";
        case RevisionSubject::Feature:   return "Feature";
        case RevisionSubject::Parameter: return "Params";
        case RevisionSubject::Sketch:    return "Sketch";
        case RevisionSubject::Relocate:  return "Relocate";
        case RevisionSubject::Grouped:   return "Group";
        case RevisionSubject::Created:   return "Create";
        case RevisionSubject::Amended:   return "Edit";
        case RevisionSubject::Dropped:   return "Drop";
        default:                          return "Edit";
    }
}

ThemeToken ClassificationTint(LayerClassification Classified)
{
    // 📐 The reference's `KINDS` record, transcribed verbatim from `components/TexturePaint.tsx`.
    switch (Classified)
    {
        case LayerClassification::Paint:     return Covering(0xF97316u);
        case LayerClassification::Material:  return Covering(0x8B5CF6u);
        case LayerClassification::Generator: return Covering(0x10B981u);
        default:                             return Covering(0x8A8A8Au);
    }
}

const char* ClassificationText(LayerClassification Classified)
{
    switch (Classified)
    {
        case LayerClassification::Paint:     return "Paint";
        case LayerClassification::Material:  return "Material";
        case LayerClassification::Generator: return "Generator";
        default:                             return "Layer";
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

Result<bool> GlobalShellPanel::Construct(InteractionIndex&              Interaction,
                                          MotionIntegrator&              Integrator,
                                          RecordingSurface&              Surface,
                                          const ThemeProfile& Resolved)
{
    if (Ledger != nullptr)
    {
        return Result<bool>::Refuse({ RefusalReason::ContentUnsupported,
                                       "the shell panel is already constructed" });
    }

    Ledger     = &Interaction;
    Motion     = &Integrator;
    this->Surface = &Surface;
    Appearance = &Resolved;

    if (!Controls.Construct(Interaction, Surface, Resolved).Resolved)
    {
        Reset();
        return Result<bool>::Refuse({ RefusalReason::CapabilityAbsent,
                                       "the shared inspector controls were refused" });
    }

    // 🔴 Every identity is claimed here and none inside a tick. A control enrolled mid-tick receives a fresh
    //    fade and reads as though the pointer had just arrived over it, once per tick, forever.
    ControlIdentity* const Every[] =
    {
        &DockSwitch,     &ModeButtons[0], &ModeButtons[1], &ModeButtons[2],
        &RetentionField, &VeilContact,    &LayerAdd,       &LayerRetention,
        &AdvanceCall,
        &MetadataActions[0], &MetadataActions[1], &MetadataActions[2],
        &MetadataActions[3], &MetadataActions[4],
        &OutlineStrip,     &ContextVeil,        &ContextActions[0], &ContextActions[1],
        &ContextTints[0],    &ContextTints[1],    &ContextTints[2],   &ContextTints[3],
        &ContextTints[4],    &ContextTints[5],    &ContextTints[6]
    };

    for (ControlIdentity* Claiming : Every)
    {
        const Result<ControlIdentity> Issued = Interaction.Enrol();

        if (!Issued.Resolved)
        {
            Reset();
            return Result<bool>::Refuse(Issued.Error);
        }

        *Claiming = Issued.Resolve();
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < RowCeiling; ++Ordinal)
    {
        ControlIdentity* const PerRow[] =
        {
            &RowContacts[Ordinal], &RowDisclosures[Ordinal], &RowPresences[Ordinal],
            &RowKebabs[Ordinal]
        };

        for (ControlIdentity* Claiming : PerRow)
        {
            const Result<ControlIdentity> Issued = Interaction.Enrol();

            if (!Issued.Resolved)
            {
                Reset();
                return Result<bool>::Refuse(Issued.Error);
            }

            *Claiming = Issued.Resolve();
        }
    }

    // 📝 The Layer Stack's rows. Each carries two takeable halves and four actions, all enrolled here so
    //    that nothing is claimed inside a tick.
    for (std::uint32_t Ordinal = 0u; Ordinal < LayerCeiling; ++Ordinal)
    {
        ControlIdentity* const PerLayer[] =
        {
            &LayerHalves[Ordinal * 2u], &LayerHalves[Ordinal * 2u + 1u], &LayerFolds[Ordinal],
            &LayerPresences[Ordinal],   &LayerMaskEyes[Ordinal],         &LayerRetires[Ordinal]
        };

        for (ControlIdentity* Claiming : PerLayer)
        {
            const Result<ControlIdentity> Issued = Interaction.Enrol();

            if (!Issued.Resolved)
            {
                Reset();
                return Result<bool>::Refuse(Issued.Error);
            }

            *Claiming = Issued.Resolve();
        }
    }

    // 📐 The two-slide strip. The reference translates it by `-translate-x-1/2` over 300 ms on
    //    cubic-bezier(.5,.05,.2,1), which `EaseCurve::Carousel` already names exactly.
    const Result<std::uint32_t> Enrolled = Integrator.EnrolEased(0.0);

    if (!Enrolled.Resolved)
    {
        Reset();
        return Result<bool>::Refuse(Enrolled.Error);
    }

    CarouselSlide = Enrolled.Resolve();

    Reseat(Resolved);

    return Result<bool>::Result(true);
}

void GlobalShellPanel::Advance(const PointerCondition& Contact, double Elapsed)
{
    Sampled = Contact;
    Controls.Advance(Contact, Elapsed);
}

void GlobalShellPanel::Reseat(const ThemeProfile& Resolved)
{
    Appearance = &Resolved;

    // 🔴 The colours are taken from the appearance rather than left at their compiled-in declarations, which is
    //    what carries a theme into the shell. `Reseat` is already called at construction and again on every
    //    display change, so the one line below is also the whole of the shell's theme response.
    Tinted = Resolved.Shell;

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
    AdvanceCall   = {};
    LayerAdd      = {};
    LayerRetention = {};

    for (ControlIdentity& Claimed : ModeButtons)
        Claimed = {};

    for (ControlIdentity& Claimed : MetadataActions)
        Claimed = {};

    OutlineStrip = {};
    ContextVeil    = {};

    for (ControlIdentity& Claimed : ContextTints)
        Claimed = {};

    for (ControlIdentity& Claimed : ContextActions)
        Claimed = {};

    // 🔴 The layer runs were left seated by an earlier reset, so a reconstructed panel drew its stack with
    //    identities the ledger had already reissued to the outliner. Every run the panel claims is cleared.
    for (std::uint32_t Ordinal = 0u; Ordinal < LayerCeiling; ++Ordinal)
    {
        LayerHalves[Ordinal * 2u]      = {};
        LayerHalves[Ordinal * 2u + 1u] = {};
        LayerFolds[Ordinal]            = {};
        LayerPresences[Ordinal]        = {};
        LayerMaskEyes[Ordinal]         = {};
        LayerRetires[Ordinal]          = {};
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < RowCeiling; ++Ordinal)
    {
        RowContacts[Ordinal]    = {};
        RowDisclosures[Ordinal] = {};
        RowPresences[Ordinal]   = {};
        RowKebabs[Ordinal]      = {};
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE KEY RULES
//------------------------------------------------------------------------------------------------------------------------

bool GlobalShellPanel::AdvanceSummoning(ShellOrdinates& Seated, bool Summoned, bool Withdrawn,
                                        bool Reversed)
{
    bool Altered = false;

    // 📐 `app/page.tsx`, extended by the two tab strips the card carries. Every condition Tab can put the
    //    card into is one ordinal on a single cycle, so the rule is stated once as a walk along it rather
    //    than as a nest of conditions that must each be read against the other three.
    //
    //      0 · withdrawn — the card is not summoned at all; undocked only
    //      1 · slide one, Outliner
    //      2 · slide one, Context Menu
    //      3 · slide two, Properties
    //      4 · slide two, History
    //
    // 📐 Docked, the card always stands, so ordinal zero is not a station on the cycle and the walk is the
    //    remaining four. Undocked, the first Tab is the step from zero to one — the reference's "the first
    //    Tab opens the menu and does nothing else" — and the wrap returns to one rather than to zero,
    //    because Tab never dismisses the card in the reference and Escape is what does.
    if (Summoned)
    {
        constexpr std::uint32_t StationCount = 5u;   // [-] - the withdrawn station and the four presented

        std::uint32_t Standing = 0u;

        if (Seated.InspectorDocked || Seated.MenuOpened)
        {
            Standing = Seated.InspectorShown ? (3u + Seated.InspectorTab)
                                             : (1u + Seated.OutlineTab);
        }

        std::uint32_t Arriving = Standing;

        if (!Reversed)
        {
            // 📐 Four onward, then wrap to one. Zero is left behind on the first press and never re-entered.
            Arriving = (Standing >= 4u) ? 1u : (Standing + 1u);
        }
        else if (Standing <= 1u)
        {
            // 📐 The strict reverse of the opening step withdraws the card, and only while undocked; a
            //    docked card has no withdrawn station, so it wraps to the far end of its own four instead.
            Arriving = (Standing == 1u && !Seated.InspectorDocked) ? 0u : 4u;
        }
        else
        {
            Arriving = Standing - 1u;
        }

        if (Seated.InspectorDocked && Arriving == 0u)
            Arriving = 4u;

        Arriving = Arriving % StationCount;

        Seated.MenuOpened     = Seated.InspectorDocked ? Seated.MenuOpened : (Arriving != 0u);
        Seated.InspectorShown = Arriving >= 3u;

        if (Arriving == 1u || Arriving == 2u)
            Seated.OutlineTab = Arriving - 1u;
        else if (Arriving >= 3u)
            Seated.InspectorTab = Arriving - 3u;

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

void GlobalShellPanel::RecordPaneHeader(const PlaneExtent& Extent, SymbolSubject Glyph, ThemeToken GlyphColour,
                                        ThemeToken MedallionColour, const char* Title, const char* Secondary)
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

    Surface->Ground(Seat, MedallionColour, MedallionEdge, CornerAll);

    // 📐 The figure sits in a 14 px square inside a 24 px medallion, centred on both axes.
    const float FigureExtent = Medallion * (14.0f / 24.0f);
    const float FigureLead   = Seat.LeastAlong  + (Medallion - FigureExtent) * 0.5f;
    const float FigureAcross = Seat.LeastAcross + (Medallion - FigureExtent) * 0.5f;

    Surface->Stroke(Glyph, Spanning(FigureLead, FigureAcross, FigureExtent, FigureExtent), GlyphColour);

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
    const ThemeToken Colours[2]   = { Tinted.WeaveFine, Tinted.WeaveCoarse };

    for (std::uint32_t Pass = 0u; Pass < 2u; ++Pass)
    {
        const float Step = Steps[Pass];

        if (Step < 1.0f)
            continue;

        for (float Along = Extent.LeastAlong; Along < Extent.MostAlong; Along += Step)
        {
            Surface->Ground(Spanning(Along, Extent.LeastAcross, 1.0f, Extent.SpanAcross()),
                            Colours[Pass], 0.0f, CornerNone);
        }

        for (float Across = Extent.LeastAcross; Across < Extent.MostAcross; Across += Step)
        {
            Surface->Ground(Spanning(Extent.LeastAlong, Across, Extent.SpanAlong(), 1.0f),
                            Colours[Pass], 0.0f, CornerNone);
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

void GlobalShellPanel::RecordOutlineColumn(const PlaneExtent& Extent, ShellOrdinates& Seated,
                                             const EntityRow* Rows, std::uint32_t RowCount)
{
    // 📐 The strip sits over the whole column, beneath nothing — the pane header belongs to the page it
    //    heads, so each of the two pages draws its own and the strip is the only shared chrome.
    static const char* const Captions[2] = { "Outliner", "Context Menu" };

    const PlaneExtent Strip = Spanning(Extent.LeastAlong, Extent.LeastAcross,
                                       Extent.SpanAlong(), Scaled.ComponentAcross);

    const TabDeclaration Declared{ Captions, 2u };

    static_cast<void>(Controls.TabStrip(OutlineStrip, Strip, Declared, Seated.OutlineTab));

    const PlaneExtent Page = Spanning(Extent.LeastAlong, Strip.MostAcross, Extent.SpanAlong(),
                                      Extent.MostAcross - Strip.MostAcross);

    if (Seated.OutlineTab == 0u)
        RecordOutliner(Page, Seated, Rows, RowCount);
    else
        RecordContextPage(Page, Seated, Rows, RowCount);

    // 📐 The column's own trailing hairline, `border-r border-[var(--hair)]`, over both pages.
    Surface->Ground(Spanning(Extent.MostAlong - 1.0f, Extent.LeastAcross, 1.0f, Extent.SpanAcross()),
                    Tinted.Hairline, 0.0f, CornerNone);
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

        // ② The kebab at the very trailing edge, and the presence action inboard of it. Both outrank the
        //    row, and the kebab outranks the eye because it is written outboard of it.
        const PlaneExtent Kebab = Spanning(Row.MostAlong - Scaled.KebabExtent - Scaled.PanePad * 0.5f,
                                           Row.LeastAcross + (Row.SpanAcross() - Scaled.KebabExtent) * 0.5f,
                                           Scaled.KebabExtent, Scaled.KebabExtent);

        const bool OnKebab = Kebab.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

        const float PresenceExtent = Scaled.GlyphExtent * (20.0f / 18.0f);
        const PlaneExtent Presence = Spanning(Kebab.LeastAlong - PresenceExtent - Scaled.PanePad * 0.5f,
                                              Row.LeastAcross + (Row.SpanAcross() - PresenceExtent) * 0.5f,
                                              PresenceExtent, PresenceExtent);

        const bool OnPresence = Presence.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

        if (Sampled.ContactArrived && !Ledger->AnyDisclosed())
        {
            if (OnChevron)
                Ledger->Seize(RowDisclosures[Ordinal], ControlPart::Chevron);
            else if (OnKebab)
                Ledger->Seize(RowKebabs[Ordinal], ControlPart::Body);
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

        if (Roused && !OnChevron && !OnPresence && !OnKebab && Ledger->Released(RowContacts[Ordinal]))
            Seated.EntityTaken = Ordinal;

        Ledger->DeclareRoused(RowContacts[Ordinal], Roused, RouseOver);

        // ③ The row ground, then its rail. `opacity-50` for a withheld row is applied to every colour it draws.
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

        // ⑤ The kebab, which raises the floating options card over the whole shell. 🔴 Its identity is
        //    spent whether or not the dots are drawn: a row that skips the call shifts every later row's
        //    rouse onto the wrong identity and the whole column re-fades on the next scroll.
        if (RecordKebab(Kebab, RowKebabs[Ordinal], Roused))
        {
            Seated.ContextRaised = Ordinal;
            Seated.ContextAlong  = Kebab.LeastAlong;
            Seated.ContextAcross = Kebab.MostAcross;
            Seated.EntityTaken   = Ordinal;
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
//                                                    THE CONTEXT SURFACE
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📐 The six hues `handleSetColor` offers, from `remix-notch-ui`'s own Tailwind run — red, orange, yellow,
//    green, blue and purple at the 500 step, which is what `.replace('400','500')` selects.
constexpr std::uint32_t ContextTintRun[6] =
{
    0xEF4444u, 0xF97316u, 0xEAB308u, 0x22C55Eu, 0x3B82F6u, 0xA855F7u
};

}   // namespace

bool GlobalShellPanel::RecordKebab(const PlaneExtent& Extent, ControlIdentity Claimed, bool Roused)
{
    const bool Over = Extent.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

    if (Sampled.ContactArrived && Over && !Ledger->AnyDisclosed())
        Ledger->Seize(Claimed, ControlPart::Body);

    const bool Taken = Over && Ledger->Released(Claimed);

    Ledger->DeclareRoused(Claimed, Over, RouseOver);

    // 📐 `opacity-0` until the row is roused, exactly as the eye beside it is.
    if (!Roused && !Over)
        return Taken;

    if (Over)
        Surface->Ground(Extent, Tinted.TileRoused, 3.0f, CornerAll);

    // 📝 Three dots and not a glyph. `SymbolSubject` carries no ellipsis and is closed at its placeholder,
    //    so authoring one would renumber the whole roster and both of its pinning asserts; three medallions
    //    are the figure itself rather than a stand-in for it, and cost nothing.
    const float Centre  = Extent.LeastAlong + Extent.SpanAlong() * 0.5f;
    const float Spacing = Scaled.KebabDot * 2.5f;
    const float Seat    = Extent.LeastAcross + Extent.SpanAcross() * 0.5f;

    for (std::uint32_t Dot = 0u; Dot < 3u; ++Dot)
    {
        Surface->Medallion(Centre, Seat + (static_cast<float>(Dot) - 1.0f) * Spacing,
                           Scaled.KebabDot, Over ? Tinted.Primary : Tinted.Faint);
    }

    return Taken;
}

void GlobalShellPanel::RecordContextPage(const PlaneExtent& Extent, ShellOrdinates& Seated,
                                         const EntityRow* Rows, std::uint32_t RowCount)
{
    Surface->Ground(Extent, Tinted.Menu, 0.0f, CornerNone);

    const float Pad = Scaled.PanePad;

    if (Rows == nullptr || RowCount == 0u || Seated.EntityTaken >= RowCount ||
        Seated.EntityTaken >= RowCeiling)
    {
        const float Run   = Scaled.RunSecondary;
        const char* Prose = "Select a record to view its options.";

        Surface->TextRun(Extent.LeastAlong + (Extent.SpanAlong() -
                                              Surface->MeasureRun(Prose, Run, 0.0f)) * 0.5f,
                         Extent.LeastAcross + (Extent.SpanAcross() - Run) * 0.5f,
                         Tinted.Faint, Prose, Run);
        return;
    }

    const std::uint32_t Ordinal   = Seated.EntityTaken;
    const EntityRow&    Presented = Rows[Ordinal];
    const bool          Grouped   = Presented.Subject == EntitySubject::Grouping;

    float Cursor = Extent.LeastAcross + Pad;

    // ① The heading, which names which of the two option sets is standing.
    {
        const float Run = Scaled.RunSecondary;

        Surface->TextRun(Extent.LeastAlong + Pad * 1.5f, Cursor + (Scaled.ContextRow - Run) * 0.5f,
                         Tinted.Muted, Grouped ? "Folder Options" : "Object Options", Run, 0.0f, true);

        Cursor += Scaled.ContextRow;

        Surface->Ground(Spanning(Extent.LeastAlong, Cursor, Extent.SpanAlong(), 1.0f),
                        Tinted.Hairline, 0.0f, CornerNone);

        Cursor += Pad;
    }

    // ② `Set Color`, offered for a folder alone, exactly as the reference gates it.
    if (Grouped)
    {
        const float HeadRun = Scaled.RunFine;

        Surface->TextRunCapitalised(Extent.LeastAlong + Pad * 1.5f, Cursor, Tinted.Faint, "Set Color",
                                    HeadRun, 0.09f, false);

        Cursor += HeadRun * RunLeading + Pad;

        float Lead = Extent.LeastAlong + Pad * 1.5f;

        for (std::uint32_t Tint = 0u; Tint < 7u; ++Tint)
        {
            const PlaneExtent Disc = Spanning(Lead, Cursor, Scaled.ContextSwatch, Scaled.ContextSwatch);
            const bool        Over = Disc.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

            if (Sampled.ContactArrived && Over && !Ledger->AnyDisclosed())
                Ledger->Seize(ContextTints[Tint], ControlPart::Body);

            static_cast<void>(Over && Ledger->Released(ContextTints[Tint]));

            Ledger->DeclareRoused(ContextTints[Tint], Over, RouseOver);

            // 📐 `hover:scale-110`, which is a tenth added about the disc's own centre.
            const float Grown  = Over ? Scaled.ContextSwatch * 0.10f : 0.0f;
            const PlaneExtent Drawn = Spanning(Disc.LeastAlong - Grown * 0.5f, Disc.LeastAcross - Grown * 0.5f,
                                               Scaled.ContextSwatch + Grown, Scaled.ContextSwatch + Grown);

            if (Tint < 6u)
            {
                Surface->Ground(Drawn, Covering(ContextTintRun[Tint]), Drawn.SpanAlong() * 0.5f, CornerAll);
            }
            else
            {
                // 📐 The seventh disc clears the hue rather than setting one, and carries a cross.
                Surface->Edge(Drawn, Tinted.HairlineFirm, 1.0f, Drawn.SpanAlong() * 0.5f, CornerAll);

                const float Mark = Scaled.ContextSwatch * 0.5f;

                Surface->Stroke(SymbolSubject::PlusCross,
                                Spanning(Drawn.LeastAlong + (Drawn.SpanAlong() - Mark) * 0.5f,
                                         Drawn.LeastAcross + (Drawn.SpanAcross() - Mark) * 0.5f,
                                         Mark, Mark),
                                Tinted.Muted, 0.7853982f);
            }

            Lead += Scaled.ContextSwatch + Pad * 0.8f;
        }

        Cursor += Scaled.ContextSwatch + Pad * 1.5f;
    }

    // ③ Rename and Delete, the two the reference always offers.
    static_cast<void>(RecordActionRow(Spanning(Extent.LeastAlong + Pad, Cursor,
                                               Extent.SpanAlong() - Pad * 2.0f, Scaled.ContextRow),
                                      ContextActions[0], SymbolSubject::ColumnArrangement,
                                      "Rename", nullptr, Tinted.Primary, Tinted.Muted));

    Cursor += Scaled.ContextRow;

    if (RecordActionRow(Spanning(Extent.LeastAlong + Pad, Cursor,
                                 Extent.SpanAlong() - Pad * 2.0f, Scaled.ContextRow),
                        ContextActions[1], SymbolSubject::TrashBin, "Delete", nullptr,
                        Covering(0xF87171u), Covering(0xF87171u)))
    {
        // 📝 The row is presented rather than removed, because the outliner's run is the host's and is
        //    borrowed for the tick. Withholding it is the strongest answer a panel may give here.
        Seated.EntityPresent[Ordinal] = false;
    }
}

void GlobalShellPanel::RecordContextOverlay(const PlaneExtent& Extent, ShellOrdinates& Seated,
                                            const EntityRow* Rows, std::uint32_t RowCount)
{
    if (Seated.ContextRaised >= RowCeiling)
        return;

    if (Rows == nullptr || Seated.ContextRaised >= RowCount)
    {
        Seated.ContextRaised = ShellOrdinates::EntityCeiling;
        return;
    }

    // 📐 `fixed inset-0 z-[100]`, which dismisses the card wherever it is tapped. Recorded first so the
    //    card itself, written after it, stands over it.
    if (Sampled.ContactArrived && !Ledger->AnyDisclosed())
        Ledger->Seize(ContextVeil, ControlPart::Body);

    const bool Dismissed = Ledger->Released(ContextVeil);

    const EntityRow& Presented = Rows[Seated.ContextRaised];
    const bool       Grouped   = Presented.Subject == EntitySubject::Grouping;
    const float      Pad       = Scaled.ContextPad;

    // 📐 The card is measured from what it carries: a heading, an optional colour strip, and two rows.
    const float HeadAcross = Scaled.ContextRow + 1.0f + Pad;
    const float TintAcross = Grouped ? (Scaled.RunFine * RunLeading + Pad + Scaled.ContextSwatch + Pad)
                                     : 0.0f;
    const float WholeAcross = Pad + HeadAcross + TintAcross + Scaled.ContextRow * 2.0f + Pad;

    // 📐 `Math.min(contextMenu.x, window.innerWidth - 200)`, and the same rule across.
    const float Clamped = Extent.MostAlong - Scaled.ContextClamp;
    const float Along   = Held(Seated.ContextAlong, Extent.LeastAlong,
                               (Clamped > Extent.LeastAlong) ? Clamped : Extent.LeastAlong);
    const float Across  = Held(Seated.ContextAcross, Extent.LeastAcross,
                               (Extent.MostAcross - WholeAcross > Extent.LeastAcross)
                                   ? Extent.MostAcross - WholeAcross : Extent.LeastAcross);

    // 📐 `initial={{opacity:0, scale:.95}} animate={{opacity:1, scale:1}} transition={{duration:0.1}}`.
    // 🔴 The veil's own rouse traverse carries the arrival, declared standing for as long as the card is
    //    raised. Read without being declared it reports zero every tick and the card records invisibly —
    //    a fade that never departs is indistinguishable from a card that was never recorded.
    Ledger->DeclareRoused(ContextVeil, true, ContextArriveOver);

    const float Arriving = Ledger->RousedFraction(ContextVeil);
    const float Coverage = Held(Arriving, 0.0f, 1.0f);
    const float Grown    = Between(0.95f, 1.0f, Coverage);

    const float CardAlong  = Scaled.ContextAlong * Grown;
    const float CardAcross = WholeAcross * Grown;

    const PlaneExtent Card = Spanning(Along + (Scaled.ContextAlong - CardAlong) * 0.5f,
                                      Across + (WholeAcross - CardAcross) * 0.5f,
                                      CardAlong, CardAcross);

    Surface->Ground(Card, Faded(Tinted.Menu, Coverage), Scaled.CardRadius, CornerAll);
    Surface->Edge(Card, Faded(Tinted.HairlineFirm, Coverage), 1.0f, Scaled.CardRadius, CornerAll);

    Surface->Confine(Card);
    RecordContextPage(Card, Seated, Rows, RowCount);
    Surface->Release();

    // 📝 Dismissed last, so the card the artist tapped is the one they saw this tick.
    if (Dismissed && !Card.Encloses(Sampled.PositionAlong, Sampled.PositionAcross))
        Seated.ContextRaised = ShellOrdinates::EntityCeiling;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE METADATA PANE
//------------------------------------------------------------------------------------------------------------------------

float GlobalShellPanel::RecordStatRow(const PlaneExtent& Extent, const char* Caption, const char* Reading)
{
    const float Run    = Scaled.RunSecondary;
    const float Seated = Extent.LeastAcross + (Extent.SpanAcross() - Run) * 0.5f;

    Surface->TextRun(Extent.LeastAlong, Seated, Tinted.Muted, Caption, Run);

    const float ReadingAlong = Extent.MostAlong - Surface->MeasureRun(Reading, Run, 0.0f);

    Surface->TextRun(ReadingAlong, Seated, Tinted.Primary, Reading, Run);

    // 📐 `border-b border-[var(--hair)]`, one side and not four, so a ground and not an edge.
    Surface->Ground(Spanning(Extent.LeastAlong, Extent.MostAcross - 1.0f, Extent.SpanAlong(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    return Extent.SpanAcross();
}

bool GlobalShellPanel::RecordActionRow(const PlaneExtent& Extent, ControlIdentity Claimed,
                                       SymbolSubject Glyph, const char* Caption, const char* Chord,
                                       ThemeToken Colour, ThemeToken GlyphColour)
{
    const bool Roused = Extent.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

    if (Sampled.ContactArrived && Roused && !Ledger->AnyDisclosed())
        Ledger->Seize(Claimed, ControlPart::Body);

    const bool Taken = Roused && Ledger->Released(Claimed);

    Ledger->DeclareRoused(Claimed, Roused, RouseOver);

    // 📐 `hover:bg-[var(--tile-hi)]`, save for Delete which rouses to its own alert wash instead.
    if (Roused)
        Surface->Ground(Extent, Faded(Colour, 0.12f), Scaled.LayerRadius, CornerAll);

    const float GlyphCell  = Scaled.ActionGlyph;
    const float GlyphSeat  = Extent.LeastAcross + (Extent.SpanAcross() - GlyphCell) * 0.5f;
    const PlaneExtent Cell = Spanning(Extent.LeastAlong + Scaled.PanePad, GlyphSeat, GlyphCell, GlyphCell);

    // 📝 The reference draws the glyph at `--muted` and the alerting action at its own hue, so both colours
    //    are handed in rather than one derived from the other — Delete is where the two agree.
    Surface->Stroke(Glyph, Cell, GlyphColour);

    const float Run     = Scaled.RunSecondary;
    const float RunSeat = Extent.LeastAcross + (Extent.SpanAcross() - Run) * 0.5f;
    const float RunLead = Cell.MostAlong + Scaled.PanePad;

    float RunCeiling = Extent.MostAlong - Scaled.PanePad;

    if (Chord != nullptr && Chord[0] != '\0')
    {
        // 📐 `ml-auto text-[9.5px] text-[var(--faint)] font-mono`, hard against the trailing edge.
        const float ChordRun  = Scaled.RunFiner;
        const float ChordLead = RunCeiling - Surface->MeasureRun(Chord, ChordRun, 0.0f);

        Surface->TextRun(ChordLead, Extent.LeastAcross + (Extent.SpanAcross() - ChordRun) * 0.5f,
                         Tinted.Faint, Chord, ChordRun);

        RunCeiling = ChordLead - Scaled.PanePad;
    }

    Surface->TextRunTruncated(RunLead, RunSeat, RunCeiling, Colour, Caption, Run);

    return Taken;
}

void GlobalShellPanel::RecordMetadata(const PlaneExtent& Extent, ShellOrdinates& Seated,
                                      const EntityRow* Rows, std::uint32_t RowCount)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    const float Pad = Scaled.PanePad;

    // ① Nothing taken. 📐 The reference centres a 48 px crosshair tile, a heading and one wrapped line.
    if (Rows == nullptr || RowCount == 0u || Seated.EntityTaken >= RowCount ||
        Seated.EntityTaken >= RowCeiling)
    {
        const float TileExtent = Scaled.HeroCrest * (48.0f / 34.0f);
        const float Run        = Scaled.RunPrimary;
        const float Fine       = Scaled.RunSecondary;
        const float Stack      = TileExtent + Pad * 2.0f + Run * RunLeading + Fine * RunLeading;
        float       Cursor     = Extent.LeastAcross + (Extent.SpanAcross() - Stack) * 0.5f;

        const PlaneExtent Tile = Spanning(Extent.LeastAlong + (Extent.SpanAlong() - TileExtent) * 0.5f,
                                          Cursor, TileExtent, TileExtent);

        Surface->Ground(Tile, Tinted.Tile, Scaled.CardRadius, CornerAll);
        Surface->Edge(Tile, Tinted.Hairline, 1.0f, Scaled.CardRadius, CornerAll);

        const float Figure = TileExtent * 0.5f;

        Surface->Stroke(SymbolSubject::CrosshairCentre,
                        Spanning(Tile.LeastAlong + (TileExtent - Figure) * 0.5f,
                                 Tile.LeastAcross + (TileExtent - Figure) * 0.5f, Figure, Figure),
                        Tinted.Muted);

        Cursor = Tile.MostAcross + Pad * 2.0f;

        const char* Heading = "Properties Overview";
        const char* Prose   = "Select a record in the directory to view its details.";

        Surface->TextRun(Extent.LeastAlong + (Extent.SpanAlong() -
                                              Surface->MeasureRun(Heading, Run, 0.0f)) * 0.5f,
                         Cursor, Tinted.Primary, Heading, Run, 0.0f, true);

        Cursor += Run * RunLeading;

        Surface->TextRun(Extent.LeastAlong + (Extent.SpanAlong() -
                                              Surface->MeasureRun(Prose, Fine, 0.0f)) * 0.5f,
                         Cursor, Tinted.Faint, Prose, Fine);
        return;
    }

    const std::uint32_t Ordinal   = Seated.EntityTaken;
    const EntityRow&    Presented = Rows[Ordinal];
    const EntityProfile& Profiled = Seated.EntityProfiles[Ordinal];
    const ThemeToken   Hue       = EntityHue(Presented.Subject);
    const bool          Absent    = !Seated.EntityPresent[Ordinal];

    // 📐 The reference mints `g_NN`; the ordinal is that identity here, so the two read side by side.
    char Token[12] = {};
    std::snprintf(Token, sizeof(Token), "g_%02u", static_cast<unsigned>(Ordinal + 1u));

    const PlaneExtent Footer = Spanning(Extent.LeastAlong, Extent.MostAcross - Scaled.FooterAcross,
                                        Extent.SpanAlong(), Scaled.FooterAcross);

    const PlaneExtent Body = Spanning(Extent.LeastAlong + Pad * 1.5f, Extent.LeastAcross + Pad * 1.5f,
                                      Extent.SpanAlong() - Pad * 3.0f,
                                      Footer.LeastAcross - Extent.LeastAcross - Pad * 3.0f);

    Surface->Confine(Body);

    float Cursor = Body.LeastAcross;

    // ② The hero tile — a black crest, the naming, and the classification in the subject's own hue.
    {
        const float HeroAcross = Scaled.HeroCrest + Scaled.HeroPad * 2.0f;
        const PlaneExtent Hero = Spanning(Body.LeastAlong, Cursor, Body.SpanAlong(), HeroAcross);

        Surface->Ground(Hero, Tinted.Tile, Scaled.CardRadius, CornerAll);
        Surface->Edge(Hero, Tinted.Hairline, 1.0f, Scaled.CardRadius, CornerAll);

        const PlaneExtent Crest = Spanning(Hero.LeastAlong + Scaled.HeroPad, Hero.LeastAcross + Scaled.HeroPad,
                                           Scaled.HeroCrest, Scaled.HeroCrest);

        Surface->Ground(Crest, Covering(0x000000u), Scaled.LayerRadius, CornerAll);

        const float Figure = Scaled.HeroCrest * (24.0f / 34.0f);

        Surface->Stroke(EntityGlyph(Presented.Subject),
                        Spanning(Crest.LeastAlong + (Scaled.HeroCrest - Figure) * 0.5f,
                                 Crest.LeastAcross + (Scaled.HeroCrest - Figure) * 0.5f, Figure, Figure),
                        Covering(0xFFFFFFu));

        const float NamingRun = Scaled.RunPrimary + 0.5f;
        const float ClassRun  = Scaled.RunSmall;
        const float PairSpan  = NamingRun * RunLeading + ClassRun * RunLeading;
        const float PairSeat  = Hero.LeastAcross + (HeroAcross - PairSpan) * 0.5f;
        const float RunLead   = Crest.MostAlong + Scaled.HeroPad;

        Surface->TextRunTruncated(RunLead, PairSeat, Hero.MostAlong - Scaled.HeroPad,
                                  Tinted.Primary, Presented.Naming, NamingRun, true);
        Surface->TextRun(RunLead, PairSeat + NamingRun * RunLeading, Hue,
                         EntityText(Presented.Subject), ClassRun, 0.0f, true);

        Cursor = Hero.MostAcross + Pad * 1.5f;
    }

    // ③ The stat rows, exactly the run `getStats` assembles and in its own order.
    {
        char Reading[64] = {};

        const auto Stated = [&](const char* Caption, const char* Value)
        {
            Cursor += RecordStatRow(Spanning(Body.LeastAlong, Cursor, Body.SpanAlong(), Scaled.StatAcross),
                                    Caption, Value);
        };

        Stated("Token", Token);
        Stated("Visible", Absent ? "hidden" : "shown");

        if (Presented.EnclosedCount > 0u)
        {
            std::snprintf(Reading, sizeof(Reading), "%u records",
                          static_cast<unsigned>(Presented.EnclosedCount));
            Stated("Nested", Reading);
        }

        std::snprintf(Reading, sizeof(Reading), "[%.1f, %.1f, %.1f]",
                      Profiled.Position[0], Profiled.Position[1], Profiled.Position[2]);
        Stated("Position", Reading);

        if (Profiled.RadiusDeclared)
        {
            std::snprintf(Reading, sizeof(Reading), "%.2f mm", Profiled.Radius);
            Stated("Radius", Reading);
        }

        if (Profiled.HeightDeclared)
        {
            std::snprintf(Reading, sizeof(Reading), "%.2f mm", Profiled.Height);
            Stated("Height", Reading);
        }

        if (Profiled.CurvesDeclared)
        {
            std::snprintf(Reading, sizeof(Reading), "%u", static_cast<unsigned>(Profiled.CurveTally));
            Stated("Curves", Reading);
        }

        if (Profiled.ConstraintDeclared)
            Stated("Status", Profiled.FullyConstrained ? "Constrained" : "Unconstrained");

        if (Profiled.DepthDeclared)
        {
            std::snprintf(Reading, sizeof(Reading), "%.2f mm", Profiled.ExtrudeDepth);
            Stated("Depth", Reading);
        }

        // ④ The albedo row, which states its three components and a disc filled with them.
        if (Profiled.AlbedoDeclared)
        {
            const PlaneExtent Row = Spanning(Body.LeastAlong, Cursor, Body.SpanAlong(), Scaled.StatAcross);
            const float       Run = Scaled.RunSecondary;
            const float       Top = Row.LeastAcross + (Row.SpanAcross() - Run) * 0.5f;

            Surface->TextRun(Row.LeastAlong, Top, Tinted.Muted, "Albedo", Run);

            const PlaneExtent Disc =
                Spanning(Row.MostAlong - Scaled.SwatchExtent,
                         Row.LeastAcross + (Row.SpanAcross() - Scaled.SwatchExtent) * 0.5f,
                         Scaled.SwatchExtent, Scaled.SwatchExtent);

            const std::uint32_t Packed = (Profiled.Albedo[0] << 16) | (Profiled.Albedo[1] << 8)
                                       | Profiled.Albedo[2];

            Surface->Ground(Disc, Covering(Packed), Scaled.SwatchExtent * 0.5f, CornerAll);
            Surface->Edge(Disc, Tinted.HairlineFirm, 1.0f, Scaled.SwatchExtent * 0.5f, CornerAll);

            std::snprintf(Reading, sizeof(Reading), "%u, %u, %u",
                          static_cast<unsigned>(Profiled.Albedo[0]),
                          static_cast<unsigned>(Profiled.Albedo[1]),
                          static_cast<unsigned>(Profiled.Albedo[2]));

            Surface->TextRun(Disc.LeastAlong - Pad - Surface->MeasureRun(Reading, Run, 0.0f), Top,
                             Tinted.Primary, Reading, Run);

            Surface->Ground(Spanning(Row.LeastAlong, Row.MostAcross - 1.0f, Row.SpanAlong(), 1.0f),
                            Tinted.Hairline, 0.0f, CornerNone);

            Cursor += Row.SpanAcross();
        }
    }

    // ⑤ The call that carries the artist to slide two — the pointer twin of Tab, and it says so.
    {
        Cursor += Pad;

        const PlaneExtent Call = Spanning(Body.LeastAlong, Cursor, Body.SpanAlong(), Scaled.AdvanceAcross);
        const bool        Over = Call.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

        if (Sampled.ContactArrived && Over && !Ledger->AnyDisclosed())
            Ledger->Seize(AdvanceCall, ControlPart::Body);

        if (Over && Ledger->Released(AdvanceCall))
        {
            Seated.InspectorShown = true;

            if (Motion != nullptr)
            {
                EasedInterpolant& Travelling = Motion->Eased(CarouselSlide);

                Travelling.Depart(Travelling.Standing(), 1.0, CarouselTravelOver, 0.0, EaseCurve::Carousel);
            }
        }

        Ledger->DeclareRoused(AdvanceCall, Over, RouseOver);

        // 📐 `bg-[rgba(91,140,255,.13)] border-[var(--accent)]`, rousing to `.2` of the same accent.
        Surface->Ground(Call, Over ? Faded(Tinted.Accent, 0.20f) : Tinted.AccentSoft,
                        Scaled.LayerRadius, CornerAll);
        Surface->Edge(Call, Tinted.Accent, 1.0f, Scaled.LayerRadius, CornerAll);

        const char* Caption  = "Properties & History";
        const char* PillRun  = "Tab";
        const float Run      = Scaled.RunSecondary;
        const float Fine     = Scaled.RunFiner;
        const float Figure   = Scaled.ActionGlyph;
        const float PillSpan = Surface->MeasureRun(PillRun, Fine, 0.0f) + Scaled.PillPadAlong * 2.0f;
        const float Whole    = Figure + Pad + Surface->MeasureRun(Caption, Run, 0.0f) + Pad + PillSpan;

        float Lead = Call.LeastAlong + (Call.SpanAlong() - Whole) * 0.5f;

        Surface->Stroke(SymbolSubject::GearCog,
                        Spanning(Lead, Call.LeastAcross + (Call.SpanAcross() - Figure) * 0.5f,
                                 Figure, Figure),
                        Tinted.Primary);

        Lead += Figure + Pad;

        Surface->TextRun(Lead, Call.LeastAcross + (Call.SpanAcross() - Run) * 0.5f,
                         Tinted.Primary, Caption, Run, 0.0f, true);

        Lead += Surface->MeasureRun(Caption, Run, 0.0f) + Pad;

        const float PillAcross = Fine + 6.0f;
        const PlaneExtent Pill = Spanning(Lead, Call.LeastAcross + (Call.SpanAcross() - PillAcross) * 0.5f,
                                          PillSpan, PillAcross);

        Surface->Ground(Pill, Tinted.MenuLower, PillAcross * 0.5f, CornerAll);
        Surface->TextRun(Pill.LeastAlong + Scaled.PillPadAlong,
                         Pill.LeastAcross + (PillAcross - Fine) * 0.5f, Tinted.Muted, PillRun, Fine);

        Cursor = Call.MostAcross + Pad * 2.0f;
    }

    // ⑥ The five inline actions beneath their own heading.
    {
        const float HeadRun = Scaled.RunFiner;

        Surface->TextRunCapitalised(Body.LeastAlong + Pad * 0.5f, Cursor, Tinted.Faint, "Actions",
                                    HeadRun, 0.09f, true);

        Cursor += HeadRun * RunLeading + Pad * 0.5f;

        // 📝 Stand-in figures where the reference reaches for a lucide glyph this build has not authored
        //    yet — `type` and `copy` in particular. The four exact ones are plus, eye, eye-off and trash-2.
        const PlaneExtent NewRecord = Spanning(Body.LeastAlong, Cursor, Body.SpanAlong(), Scaled.ActionAcross);

        static_cast<void>(RecordActionRow(NewRecord, MetadataActions[0], SymbolSubject::PlusCross,
                                          "New record", nullptr, Tinted.Primary, Tinted.Muted));
        Cursor += Scaled.ActionAcross;

        Surface->Ground(Spanning(Body.LeastAlong + Pad, Cursor + Pad * 0.5f,
                                 Body.SpanAlong() - Pad * 2.0f, 1.0f),
                        Tinted.Hairline, 0.0f, CornerNone);
        Cursor += Pad;

        static_cast<void>(RecordActionRow(Spanning(Body.LeastAlong, Cursor, Body.SpanAlong(),
                                                   Scaled.ActionAcross),
                                          MetadataActions[1], SymbolSubject::ColumnArrangement,
                                          "Rename", "F2", Tinted.Primary, Tinted.Muted));
        Cursor += Scaled.ActionAcross;

        static_cast<void>(RecordActionRow(Spanning(Body.LeastAlong, Cursor, Body.SpanAlong(),
                                                   Scaled.ActionAcross),
                                          MetadataActions[2], SymbolSubject::LatticeArrangement,
                                          "Duplicate", "Ctrl D", Tinted.Primary, Tinted.Muted));
        Cursor += Scaled.ActionAcross;

        // 📐 The row states the action rather than the condition: a shown record offers Hide, and the eye
        //    it carries is the one the reference draws for the condition the record is IN.
        if (RecordActionRow(Spanning(Body.LeastAlong, Cursor, Body.SpanAlong(), Scaled.ActionAcross),
                            MetadataActions[3],
                            Absent ? SymbolSubject::EyeClosed : SymbolSubject::EyeOpen,
                            Absent ? "Show" : "Hide", "H", Tinted.Primary, Tinted.Muted))
        {
            const bool Arriving = Absent;

            Seated.EntityPresent[Ordinal] = Arriving;

            for (std::uint32_t Inward = Ordinal + 1u; Inward < RowCount && Inward < RowCeiling; ++Inward)
            {
                if (Rows[Inward].Depth <= Presented.Depth)
                    break;

                Seated.EntityPresent[Inward] = Arriving;
            }
        }

        Cursor += Scaled.ActionAcross;

        static_cast<void>(RecordActionRow(Spanning(Body.LeastAlong, Cursor, Body.SpanAlong(),
                                                   Scaled.ActionAcross),
                                          MetadataActions[4], SymbolSubject::TrashBin,
                                          "Delete", "Del", Covering(0xFF6B6Bu), Covering(0xFF6B6Bu)));
    }

    Surface->Release();

    // ⑦ The footer — the subject's hue as a chip, its naming, and the token hard against the trailing edge.
    Surface->Ground(Footer, Tinted.MenuLower, 0.0f, CornerNone);
    Surface->Ground(Spanning(Footer.LeastAlong, Footer.LeastAcross, Footer.SpanAlong(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    const float FooterRun = Scaled.RunFine;
    const float FooterTop = Footer.LeastAcross + (Footer.SpanAcross() - FooterRun) * 0.5f;
    const float ChipSeat  = Footer.LeastAcross + (Footer.SpanAcross() - Scaled.ChipExtent) * 0.5f;

    Surface->Ground(Spanning(Footer.LeastAlong + Scaled.HeaderPadAlong, ChipSeat,
                             Scaled.ChipExtent, Scaled.ChipExtent), Hue, 2.0f, CornerAll);

    Surface->TextRun(Footer.LeastAlong + Scaled.HeaderPadAlong + Scaled.ChipExtent + Pad, FooterTop,
                     Tinted.Muted, EntityText(Presented.Subject), FooterRun);

    Surface->TextRun(Footer.MostAlong - Scaled.HeaderPadAlong -
                     Surface->MeasureRun(Token, FooterRun, 0.0f), FooterTop,
                     Tinted.Muted, Token, FooterRun);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE COMPONENTS
//------------------------------------------------------------------------------------------------------------------------

void GlobalShellPanel::RecordPropertyCards(const PlaneExtent& Extent, ShellOrdinates& Seated,
                                           const EntityRow* Rows, std::uint32_t RowCount)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    if (Seated.EntityTaken >= RowCount || Seated.EntityTaken >= RowCeiling)
    {
        const float ProseRun = Scaled.RunSecondary;
        const char* Prose    = "Select a record to inspect its properties.";
        const float ProseLead = Extent.LeastAlong
                              + (Extent.SpanAlong() - Surface->MeasureRun(Prose, ProseRun, 0.0f)) * 0.5f;

        Surface->TextRun(ProseLead, Extent.LeastAcross + Scaled.HeaderAcross, Tinted.Faint, Prose, ProseRun);
        return;
    }

    const EntityRow& Presented = Rows[Seated.EntityTaken];

    const float Pad    = Scaled.PanePad;
    float       Cursor = Extent.LeastAcross + Pad;

    Surface->Confine(Extent);

    // 📐 One card per component, each folding over 200 ms. `CardOrdinal` is the fold identity the card
    //    claims; the reference keys its `collapsedCards` record by the card's own title, and the ordinal is
    //    that key here so the artist's fold survives a change of subject the way the reference's does.
    std::uint32_t CardOrdinal = 0u;

    const auto RecordCard = [&](const char* Caption, const char* const* Rows2, std::uint32_t RowCount2)
    {
        if (CardOrdinal >= CardCeiling)
            return;

        const std::uint32_t Claimed = CardOrdinal++;

        // 🔴 The fold is a fraction and not a flag, so the body's extent is what animates. Folded to a
        //    flag, the card would vanish between two frames and the 200 ms the reference states would be
        //    visible nowhere.
        const bool  Folded   = Seated.CardFolded[Claimed];
        const float Standing = Controls.OutlineExpansion(CardFolds[Claimed], !Folded, true);

        const float BodyAcross = (static_cast<float>(RowCount2) * Scaled.RowAcross + Pad * 2.0f) * Standing;
        const PlaneExtent Card = Spanning(Extent.LeastAlong + Pad, Cursor,
                                          Extent.SpanAlong() - Pad * 2.0f,
                                          Scaled.ComponentAcross + BodyAcross);

        Surface->Ground(Card, Covering(0x0A0A0Bu), Scaled.CardRadius, CornerAll);
        Surface->Edge(Card, Tinted.Hairline, 1.0f, Scaled.CardRadius, CornerAll);

        const PlaneExtent CardHeader = Spanning(Card.LeastAlong, Card.LeastAcross,
                                                Card.SpanAlong(), Scaled.ComponentAcross);

        Surface->Ground(CardHeader, Tinted.MenuLower, Scaled.CardRadius,
                        CornerLeadingUpper | CornerTrailingUpper);

        const bool OnHeader = CardHeader.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

        if (Sampled.ContactArrived && OnHeader && !Ledger->AnyDisclosed())
            Ledger->Seize(CardFolds[Claimed], ControlPart::Chevron);

        if (OnHeader && Ledger->Released(CardFolds[Claimed]))
            Seated.CardFolded[Claimed] = !Seated.CardFolded[Claimed];

        // 📐 `border-b border-transparent` while folded, and the hairline only once disclosed.
        if (Standing > 0.0f)
        {
            Surface->Ground(Spanning(CardHeader.LeastAlong, CardHeader.MostAcross - 1.0f,
                                     CardHeader.SpanAlong(), 1.0f),
                            Faded(Tinted.Hairline, Standing), 0.0f, CornerNone);
        }

        // 📐 The chevron turns `-rotate-90` while folded; the two figures are the turn.
        const float Mark = Scaled.ActionGlyph;

        Surface->Stroke(Folded ? SymbolSubject::ChevronRight : SymbolSubject::ChevronDown,
                        Spanning(CardHeader.LeastAlong + Scaled.HeaderPadAlong * 0.6f,
                                 CardHeader.LeastAcross + (CardHeader.SpanAcross() - Mark) * 0.5f,
                                 Mark, Mark),
                        Tinted.Faint);

        const float CaptionRun = Scaled.RunSmall;

        // 📐 `uppercase tracking-wide` at 10.5 px.
        Surface->TextRunCapitalised(CardHeader.LeastAlong + Scaled.HeaderPadAlong * 0.6f + Mark + Pad,
                                    CardHeader.LeastAcross + (CardHeader.SpanAcross() - CaptionRun) * 0.5f,
                                    OnHeader ? Tinted.Primary : Tinted.Muted, Caption, CaptionRun,
                                    0.025f, true);

        // 📐 `ml-auto text-[9.5px]`, the count of fields the card holds.
        char Tallied[8] = {};
        std::snprintf(Tallied, sizeof(Tallied), "%u", static_cast<unsigned>(RowCount2));

        const float TallyRun = Scaled.RunFiner;

        Surface->TextRun(CardHeader.MostAlong - Scaled.HeaderPadAlong
                         - Surface->MeasureRun(Tallied, TallyRun, 0.0f),
                         CardHeader.LeastAcross + (CardHeader.SpanAcross() - TallyRun) * 0.5f,
                         Tinted.Faint, Tallied, TallyRun);

        // 🔴 The body is confined to whatever the fold has opened, so a row half way through the travel
        //    is clipped rather than drawn over the card beneath it.
        if (Standing > 0.0f)
        {
            const PlaneExtent Opened = Spanning(Card.LeastAlong, CardHeader.MostAcross,
                                                Card.SpanAlong(), BodyAcross);

            Surface->Confine(Opened);

            float RowCursor = CardHeader.MostAcross + Pad;

            for (std::uint32_t Ordinal = 0u; Ordinal < RowCount2; ++Ordinal)
            {
                const float LabelRun = Scaled.RunSecondary;
                const float LabelTop = RowCursor + (Scaled.RowAcross - LabelRun) * 0.5f;

                Surface->TextRun(Card.LeastAlong + Pad * 1.5f, LabelTop, Tinted.Muted,
                                 Rows2[Ordinal], LabelRun);

                // 📝 The value cell, `--value-bg` at `--value-radius`, presented but not yet editable —
                //    the reference's own handlers are `onChange={()=>{}}` for every component field.
                const float CellAlong = Card.SpanAlong() * 0.45f;
                const PlaneExtent Cell =
                    Spanning(Card.MostAlong - Pad * 1.5f - CellAlong,
                             RowCursor + (Scaled.RowAcross - Scaled.SearchAcross * 0.8f) * 0.5f,
                             CellAlong, Scaled.SearchAcross * 0.8f);

                Surface->Ground(Cell, Covering(0x232326u), Scaled.FieldRadius, CornerAll);

                RowCursor += Scaled.RowAcross;
            }

            Surface->Release();
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

    // 🔴 Every card ordinal is spent whether the subject presented it or not. Skipped, a card that
    //    appears for one subject and not the next inherits the fold of whichever card held that ordinal
    //    before it, and the artist watches an unrelated card close.
    while (CardOrdinal < CardCeiling)
        static_cast<void>(Controls.OutlineExpansion(CardFolds[CardOrdinal++], true, true));

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE REVISION SPINE
//------------------------------------------------------------------------------------------------------------------------

void GlobalShellPanel::RecordRevisionSpine(const PlaneExtent& Extent, ShellOrdinates& Seated,
                                           const EntityRow* Rows, std::uint32_t RowCount,
                                           const EntityRevision* Revisions, std::uint32_t RevisionCount)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    if (Revisions == nullptr)
        RevisionCount = 0u;

    const bool Selected = Seated.EntityTaken < RowCount && Seated.EntityTaken < RowCeiling;

    // 📐 The reference gathers the taken record AND everything nested inside it, so a folder presents
    //    its children's revisions too. The run is linear and carries depth, so the descent is the span of
    //    rows deeper than the taken one that follow it.
    std::uint32_t Least = Seated.EntityTaken;
    std::uint32_t Most  = Seated.EntityTaken;

    if (Selected)
    {
        for (std::uint32_t Inward = Seated.EntityTaken + 1u; Inward < RowCount && Inward < RowCeiling;
             ++Inward)
        {
            if (Rows[Inward].Depth <= Rows[Seated.EntityTaken].Depth)
                break;

            Most = Inward;
        }
    }

    std::uint32_t Standing = 0u;

    if (Selected)
    {
        for (std::uint32_t Ordinal = 0u; Ordinal < RevisionCount; ++Ordinal)
        {
            if (Revisions[Ordinal].Against >= Least && Revisions[Ordinal].Against <= Most)
                ++Standing;
        }
    }

    if (!Selected || Standing == 0u)
    {
        const float Run   = Scaled.RunSecondary;
        const char* Prose = "No history events found for this selection or its children.";

        Surface->TextRun(Extent.LeastAlong + (Extent.SpanAlong()
                                              - Surface->MeasureRun(Prose, Run, 0.0f)) * 0.5f,
                         Extent.LeastAcross + Scaled.HeaderAcross, Tinted.Faint, Prose, Run);
        return;
    }

    const float Pad    = Scaled.PanePad;
    float       Cursor = Extent.LeastAcross + Pad;

    Surface->Confine(Extent);

    // 📐 One group per record that carries a revision, in the run's own order.
    for (std::uint32_t Against = Least; Against <= Most && Against < RowCeiling; ++Against)
    {
        std::uint32_t Held = 0u;

        for (std::uint32_t Ordinal = 0u; Ordinal < RevisionCount; ++Ordinal)
        {
            if (Revisions[Ordinal].Against == Against)
                ++Held;
        }

        if (Held == 0u)
            continue;

        const EntityRow&  Grouped = Rows[Against];
        const ThemeToken Hue     = EntityHue(Grouped.Subject);

        // ① The group header, `h-[32px] px-[12px]`, which folds the whole group.
        const PlaneExtent GroupHead = Spanning(Extent.LeastAlong + Pad, Cursor,
                                               Extent.SpanAlong() - Pad * 2.0f, Scaled.RowAcross);

        const bool OnHead = GroupHead.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

        if (Sampled.ContactArrived && OnHead && !Ledger->AnyDisclosed())
            Ledger->Seize(RevisionGroups[Against], ControlPart::Chevron);

        if (OnHead && Ledger->Released(RevisionGroups[Against]))
            Seated.RevisionFolded[Against] = !Seated.RevisionFolded[Against];

        const bool  GroupFolded = Seated.RevisionFolded[Against];
        const float Opened      = Controls.OutlineExpansion(RevisionGroups[Against], !GroupFolded, true);

        const float CrestExtent = Scaled.ActionGlyph + 5.0f;
        const PlaneExtent Crest = Spanning(GroupHead.LeastAlong,
                                           GroupHead.LeastAcross
                                           + (GroupHead.SpanAcross() - CrestExtent) * 0.5f,
                                           CrestExtent, CrestExtent);

        Surface->Ground(Crest, Hue, 5.0f, CornerAll);

        const float CrestFigure = CrestExtent * 0.6f;

        Surface->Stroke(EntityGlyph(Grouped.Subject),
                        Spanning(Crest.LeastAlong + (CrestExtent - CrestFigure) * 0.5f,
                                 Crest.LeastAcross + (CrestExtent - CrestFigure) * 0.5f,
                                 CrestFigure, CrestFigure),
                        Covering(0xFFFFFFu));

        const float NameRun = Scaled.RunPrimary;
        const float NameTop = GroupHead.LeastAcross + (GroupHead.SpanAcross() - NameRun) * 0.5f;

        // 📐 `{n} ops` at the trailing edge, and the chevron outboard of it.
        char Tallied[16] = {};
        std::snprintf(Tallied, sizeof(Tallied), "%u ops", static_cast<unsigned>(Held));

        const float TallyRun  = Scaled.RunFine;
        const float Mark      = Scaled.ActionGlyph;
        const float TallyLead = GroupHead.MostAlong - Mark - Pad
                              - Surface->MeasureRun(Tallied, TallyRun, 0.0f);

        Surface->TextRun(TallyLead, GroupHead.LeastAcross + (GroupHead.SpanAcross() - TallyRun) * 0.5f,
                         Tinted.Muted, Tallied, TallyRun);

        Surface->Stroke(GroupFolded ? SymbolSubject::ChevronRight : SymbolSubject::ChevronDown,
                        Spanning(GroupHead.MostAlong - Mark,
                                 GroupHead.LeastAcross + (GroupHead.SpanAcross() - Mark) * 0.5f,
                                 Mark, Mark),
                        Tinted.Faint);

        Surface->TextRunTruncated(Crest.MostAlong + Pad, NameTop, TallyLead - Pad,
                                  OnHead ? Covering(0xFFFFFFu) : Tinted.Primary,
                                  Grouped.Naming, NameRun, true);

        Cursor += Scaled.RowAcross + 4.0f;

        if (Opened <= 0.0f)
            continue;

        // ② The revisions themselves — a numbered bubble, the spine, and the card beside them.
        const float CardAcross  = Scaled.LayerHeadAcross;
        const float WholeAcross = static_cast<float>(Held) * (CardAcross + 4.0f) * Opened;
        const PlaneExtent Stack = Spanning(Extent.LeastAlong, Cursor, Extent.SpanAlong(), WholeAcross);

        Surface->Confine(Stack);

        float         Along     = Cursor;
        std::uint32_t Numbered  = 0u;

        for (std::uint32_t Ordinal = 0u; Ordinal < RevisionCount; ++Ordinal)
        {
            const EntityRevision& Revised = Revisions[Ordinal];

            if (Revised.Against != Against)
                continue;

            const bool First = Numbered == 0u;
            const bool Last  = Numbered + 1u == Held;

            const float BubbleExtent = 25.0f;
            const float BubbleLead   = Extent.LeastAlong + Pad
                                     + (32.0f - BubbleExtent) * 0.5f;
            const float BubbleMid    = Along + 7.0f + BubbleExtent * 0.5f;

            // 📐 The spine, `w-[6px]`, stopping half way at the first and last of the group so the run
            //    reads as a bracket — the same rule the layer stack's own rail follows.
            const float SpineMid  = Extent.LeastAlong + Pad + 32.0f + 15.0f * 0.5f;
            const float SpineTop  = First ? BubbleMid : Along;
            const float SpineFoot = Last  ? BubbleMid : Along + CardAcross + 4.0f;

            if (SpineFoot > SpineTop)
            {
                Surface->Ground(Spanning(SpineMid - 3.0f, SpineTop, 6.0f, SpineFoot - SpineTop),
                                Hue, 4.0f, CornerAll);
            }

            Surface->Ground(Spanning(BubbleLead, Along + 7.0f, BubbleExtent, BubbleExtent),
                            Hue, BubbleExtent * 0.5f, CornerAll);

            char Counted[4] = {};
            std::snprintf(Counted, sizeof(Counted), "%02u", static_cast<unsigned>(Numbered));

            const float CountRun = Scaled.RunFine;

            Surface->TextRun(BubbleLead + (BubbleExtent
                                           - Surface->MeasureRun(Counted, CountRun, 0.0f)) * 0.5f,
                             Along + 7.0f + (BubbleExtent - CountRun) * 0.5f,
                             Covering(0xFFFFFFu), Counted, CountRun, 0.0f, true);

            // 📐 The 7 px node, ringed by 3 px of the pane's own ground.
            Surface->Medallion(SpineMid, BubbleMid, 6.5f, Tinted.MenuLower);
            Surface->Medallion(SpineMid, BubbleMid, 3.5f, Covering(0xFFFFFFu));

            const PlaneExtent Card = Spanning(SpineMid + 15.0f * 0.5f + 8.0f, Along,
                                              Extent.MostAlong - Pad
                                              - (SpineMid + 15.0f * 0.5f + 8.0f), CardAcross);

            const bool OnCard = Card.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

            Surface->Ground(Card, OnCard ? Tinted.TileRoused : Tinted.Tile, Scaled.LayerRadius, CornerAll);
            Surface->Edge(Card, Tinted.Hairline, 1.0f, Scaled.LayerRadius, CornerAll);

            const RevisionDeclaration Declared{ Revised.Description, Revised.Secondary, Revised.TimeRun };

            Controls.RevisionRow(Card, Declared, OnCard);

            Along    += CardAcross + 4.0f;
            Numbered += 1u;
        }

        Surface->Release();

        Cursor += WholeAcross + Pad * 2.0f;
    }

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  SLIDE TWO, WHOLE
//------------------------------------------------------------------------------------------------------------------------

void GlobalShellPanel::RecordComponents(const PlaneExtent& Extent, ShellOrdinates& Seated,
                                        const EntityRow* Rows, std::uint32_t RowCount,
                                        const EntityRevision* Revisions, std::uint32_t RevisionCount)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    const bool Selected = Seated.EntityTaken < RowCount && Seated.EntityTaken < RowCeiling;

    const PlaneExtent Header = Spanning(Extent.LeastAlong, Extent.LeastAcross,
                                        Extent.SpanAlong(), Scaled.HeaderAcross);

    // ① The header, and the Back call it carries at its trailing edge.
    if (!Selected)
    {
        RecordPaneHeader(Header, SymbolSubject::CubeSolid, Tinted.Faint, Covering(0x111111u),
                         "Nothing selected", nullptr);
    }
    else
    {
        const EntityRow&  Presented = Rows[Seated.EntityTaken];
        const ThemeToken Hue       = EntityHue(Presented.Subject);

        char Classified[48] = {};
        std::snprintf(Classified, sizeof(Classified), "%s Entity", EntityText(Presented.Subject));

        RecordPaneHeader(Header, EntityGlyph(Presented.Subject), Hue, Covering(0x111111u),
                         Presented.Naming, Classified);

        // 📝 The header's secondary run carries the entity hue rather than the shared faint colour, so it
        //    is recorded again over the shared header's own.
        const float SecondaryRun = Scaled.RunFine;
        const float PairAcross   = Scaled.RunPrimary * RunLeading + SecondaryRun * RunLeading;
        const float PairLead     = Header.LeastAcross + (Header.SpanAcross() - PairAcross) * 0.5f;
        const float RunLead      = Header.LeastAlong + Scaled.HeaderPadAlong + Scaled.MedallionExtent
                                 + Scaled.HeaderPadAlong * 0.8f;

        Surface->TextRunTruncated(RunLead, PairLead + Scaled.RunPrimary * RunLeading,
                                  Header.MostAlong - RunLead - Scaled.HeaderPadAlong,
                                  Hue, Classified, SecondaryRun, false);
    }

    {
        // 📐 "Back to scene directory", `h-7` hard against the trailing edge, which returns the strip
        //    to slide one. It is the pointer twin of the Escape the reference binds to the same travel.
        const char* Caption  = "Back to scene directory";
        const float Run      = Scaled.RunSecondary;
        const float Mark     = Scaled.ActionGlyph * 0.9f;
        const float Gap      = Scaled.PanePad * 0.8f;
        const float CallSpan = Mark + Gap + Surface->MeasureRun(Caption, Run, 0.0f)
                             + Scaled.HeaderPadAlong;

        const PlaneExtent Call = Spanning(Header.MostAlong - Scaled.HeaderPadAlong - CallSpan,
                                          Header.LeastAcross + (Header.SpanAcross() - 28.0f) * 0.5f,
                                          CallSpan, 28.0f);

        const bool OnCall = Call.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

        if (Sampled.ContactArrived && OnCall && !Ledger->AnyDisclosed())
            Ledger->Seize(BackCall, ControlPart::Body);

        if (OnCall && Ledger->Released(BackCall))
        {
            Seated.InspectorShown = false;

            if (Motion != nullptr)
            {
                EasedInterpolant& Travelling = Motion->Eased(CarouselSlide);

                Travelling.Depart(Travelling.Standing(), 0.0, CarouselTravelOver, 0.0, EaseCurve::Carousel);
            }
        }

        Ledger->DeclareRoused(BackCall, OnCall, RouseOver);

        if (OnCall)
            Surface->Ground(Call, Tinted.TileRoused, Scaled.FieldRadius, CornerAll);

        Surface->Stroke(SymbolSubject::ChevronRight,
                        Spanning(Call.LeastAlong, Call.LeastAcross + (Call.SpanAcross() - Mark) * 0.5f,
                                 Mark, Mark),
                        OnCall ? Tinted.Primary : Tinted.Muted, 3.1415927f);

        Surface->TextRun(Call.LeastAlong + Mark + Gap,
                         Call.LeastAcross + (Call.SpanAcross() - Run) * 0.5f,
                         OnCall ? Tinted.Primary : Tinted.Muted, Caption, Run);
    }

    // ② The strip, and the inner carousel it drives.
    static const char* const Captions[2] = { "Properties", "History" };

    const PlaneExtent Strip = Spanning(Extent.LeastAlong, Header.MostAcross,
                                       Extent.SpanAlong(), Scaled.ComponentAcross);

    const TabDeclaration Declared{ Captions, 2u };

    static_cast<void>(Controls.TabStrip(InspectorStrip, Strip, Declared, Seated.InspectorTab));

    const PlaneExtent Pages = Spanning(Extent.LeastAlong, Strip.MostAcross, Extent.SpanAlong(),
                                       Extent.MostAcross - Strip.MostAcross - Scaled.FooterAcross);

    // 📐 The reference lays a 200 %-wide strip inside the body and translates it by half its own
    //    extent, exactly as the outer inspector does — so the inner travel is the outer one, one level in.
    const float Carried = (Seated.InspectorTab == 1u) ? -Pages.SpanAlong() : 0.0f;

    Surface->Confine(Pages);

    const PlaneExtent Leading = Spanning(Pages.LeastAlong + Carried, Pages.LeastAcross,
                                         Pages.SpanAlong(), Pages.SpanAcross());
    const PlaneExtent Trailing = Spanning(Leading.MostAlong, Pages.LeastAcross,
                                          Pages.SpanAlong(), Pages.SpanAcross());

    if (!Surface->Excluded(Leading))
        RecordPropertyCards(Leading, Seated, Rows, RowCount);

    if (!Surface->Excluded(Trailing))
        RecordRevisionSpine(Trailing, Seated, Rows, RowCount, Revisions, RevisionCount);

    Surface->Release();

    // ③ The footer, `{n} fields`.
    const PlaneExtent Footer = Spanning(Extent.LeastAlong, Extent.MostAcross - Scaled.FooterAcross,
                                        Extent.SpanAlong(), Scaled.FooterAcross);

    Surface->Ground(Footer, Tinted.MenuLower, 0.0f, CornerNone);
    Surface->Ground(Spanning(Footer.LeastAlong, Footer.LeastAcross, Footer.SpanAlong(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    if (Selected)
    {
        const ThemeToken Hue       = EntityHue(Rows[Seated.EntityTaken].Subject);
        const float       FooterRun = Scaled.RunFine;
        const float       FooterTop = Footer.LeastAcross + (Footer.SpanAcross() - FooterRun) * 0.5f;
        const float       ChipSeat  = Footer.LeastAcross
                                    + (Footer.SpanAcross() - Scaled.ChipExtent) * 0.5f;

        Surface->Ground(Spanning(Footer.LeastAlong + Scaled.HeaderPadAlong, ChipSeat,
                                 Scaled.ChipExtent, Scaled.ChipExtent), Hue, 2.0f, CornerAll);

        Surface->TextRun(Footer.LeastAlong + Scaled.HeaderPadAlong + Scaled.ChipExtent
                         + Scaled.PanePad, FooterTop, Tinted.Muted,
                         (Seated.InspectorTab == 0u) ? "Properties" : "History", FooterRun);
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE LAYER STACK
//------------------------------------------------------------------------------------------------------------------------

float GlobalShellPanel::RecordLayerRow(const PlaneExtent& Extent, ShellOrdinates& Seated,
                                       const LayerRow& Presented, std::uint32_t Ordinal,
                                       std::uint32_t LayerCount, bool Trailing)
{
    const bool Unfolded = Seated.LayerUnfolded[Ordinal];
    const bool Shown    = Seated.LayerShown[Ordinal];
    const bool Taken    = Seated.LayerTaken == Ordinal;

    // ① The spine gutter, `w-[30px]`, and the coloured rail running through it. The reference stops the
    //    rail half way at the first and last rows so the run reads as a bracket rather than a full column.
    const float SpineMid   = Extent.LeastAlong + Scaled.LayerSpineAlong * 0.5f;
    const float BadgeMid   = Extent.LeastAcross + Scaled.LayerHeadAcross * 0.5f;
    const bool  First      = Ordinal == 0u;
    const float SpineTop   = First ? BadgeMid : Extent.LeastAcross;
    const float SpineFoot  = Trailing ? BadgeMid : Extent.MostAcross;

    if (SpineFoot > SpineTop)
    {
        Surface->Ground(Spanning(SpineMid - Scaled.LayerSpineWidth * 0.5f, SpineTop,
                                 Scaled.LayerSpineWidth, SpineFoot - SpineTop),
                        Shown ? Covering(Presented.TagHue) : Covering(0x1B1B1Bu),
                        Scaled.LayerSpineWidth * 0.5f, CornerAll);
    }

    // 📐 The ordinal badge counts DOWN the stack — `String(layers.length - idx).padStart(2,'0')`.
    const float BadgeExtent = Scaled.LayerBadge;

    Surface->Medallion(SpineMid, BadgeMid, BadgeExtent * 0.5f,
                       Shown ? Covering(Presented.TagHue) : Covering(0x2A2A2Au));

    char Badge[3] = { '0', '0', '\0' };
    const std::uint32_t Counted = (LayerCount > Ordinal) ? (LayerCount - Ordinal) : 0u;

    Badge[0] = static_cast<char>('0' + ((Counted / 10u) % 10u));
    Badge[1] = static_cast<char>('0' + (Counted % 10u));

    const float BadgeRun = Scaled.RunFine * 0.95f;
    Surface->TextRun(SpineMid - Surface->MeasureRun(Badge, BadgeRun, 0.0f) * 0.5f,
                     BadgeMid - BadgeRun * 0.5f, Covering(0xFFFFFFu), Badge, BadgeRun);

    // ② The row card itself, beginning after the spine gutter.
    const PlaneExtent Card = Spanning(Extent.LeastAlong + Scaled.LayerSpineAlong, Extent.LeastAcross,
                                      Extent.SpanAlong() - Scaled.LayerSpineAlong - Scaled.HeaderPadAlong,
                                      Extent.SpanAcross());

    Surface->Ground(Card, Taken ? Tinted.AccentSoft : Tinted.Tile, Scaled.LayerRadius, CornerAll);
    Surface->Edge(Card, Taken ? Tinted.Accent : Tinted.Hairline, 1.0f, Scaled.LayerRadius, CornerAll);

    const PlaneExtent Head = Spanning(Card.LeastAlong, Card.LeastAcross,
                                      Card.SpanAlong(), Scaled.LayerHeadAcross);

    const float HalfAlong = Head.SpanAlong() * 0.5f;

    const PlaneExtent LayerHalf = Spanning(Head.LeastAlong, Head.LeastAcross,
                                           HalfAlong, Head.SpanAcross());
    const PlaneExtent MaskHalf  = Spanning(Head.LeastAlong + HalfAlong, Head.LeastAcross,
                                           HalfAlong, Head.SpanAcross());

    // 📝 The divider between the halves, `w-[1px] my-[6px]`.
    Surface->Ground(Spanning(Head.LeastAlong + HalfAlong, Head.LeastAcross + Scaled.LayerRowPad,
                             1.0f, Head.SpanAcross() - Scaled.LayerRowPad * 2.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    // 📝 The taken half carries `shadow-[inset_0_-2px_0_var(--accent)]` beneath it.
    const auto MarkTaken = [&](const PlaneExtent& Half, LayerTarget Target)
    {
        if (!Taken || Seated.TargetTaken != Target)
            return;

        Surface->Ground(Half, Partial(0xFFFFFFu, 0.06), 0.0f, CornerNone);
        Surface->Ground(Spanning(Half.LeastAlong, Half.MostAcross - 2.0f, Half.SpanAlong(), 2.0f),
                        Tinted.Accent, 0.0f, CornerNone);
    };

    MarkTaken(LayerHalf, LayerTarget::Layer);
    MarkTaken(MaskHalf,  LayerTarget::Mask);

    // ③ The left half — chevron, eye, swatch, then the naming pair.
    float Cursor = LayerHalf.LeastAlong + Scaled.LayerRowPad;

    const float ActionMid = LayerHalf.LeastAcross + (LayerHalf.SpanAcross() - Scaled.LayerAction) * 0.5f;

    const PlaneExtent Chevron = Spanning(Cursor, ActionMid, Scaled.LayerAction, Scaled.LayerAction);
    const bool OnChevron = Chevron.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

    Surface->Stroke(Unfolded ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                    Spanning(Chevron.LeastAlong + 3.0f, Chevron.LeastAcross + 3.0f,
                             Scaled.LayerAction - 6.0f, Scaled.LayerAction - 6.0f),
                    OnChevron ? Tinted.Primary : Tinted.Muted);

    Cursor += Scaled.LayerAction + Scaled.LayerGap * 0.5f;

    const PlaneExtent Presence = Spanning(Cursor, ActionMid, Scaled.LayerAction, Scaled.LayerAction);
    const bool OnPresence = Presence.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

    Surface->Stroke(Shown ? SymbolSubject::EyeOpen : SymbolSubject::EyeClosed,
                    Spanning(Presence.LeastAlong + 3.5f, Presence.LeastAcross + 3.5f,
                             Scaled.LayerAction - 7.0f, Scaled.LayerAction - 7.0f),
                    OnPresence ? Tinted.Primary : Tinted.Muted);

    Cursor += Scaled.LayerAction + Scaled.LayerGap * 0.5f;

    // 📐 The paint swatch — a black tile holding a 14 px colour square and a 4 px classification dot.
    const PlaneExtent Swatch = Spanning(Cursor, LayerHalf.LeastAcross
                                        + (LayerHalf.SpanAcross() - Scaled.LayerSwatch) * 0.5f,
                                        Scaled.LayerSwatch, Scaled.LayerSwatch);

    Surface->Ground(Swatch, Covering(0x000000u), Scaled.FieldRadius, CornerAll);
    Surface->Edge(Swatch, Covering(0x1C1C1Cu), 1.0f, Scaled.FieldRadius, CornerAll);

    const float Inner = Scaled.LayerSwatch * (14.0f / 26.0f);
    Surface->Ground(Spanning(Swatch.LeastAlong + (Scaled.LayerSwatch - Inner) * 0.5f,
                             Swatch.LeastAcross + (Scaled.LayerSwatch - Inner) * 0.5f, Inner, Inner),
                    Covering(Presented.PaintHue), 3.0f, CornerAll);

    const float Dot = Scaled.LayerSwatch * (4.0f / 26.0f);
    Surface->Medallion(Swatch.MostAlong - Dot, Swatch.LeastAcross + Dot, Dot * 0.5f,
                       ClassificationTint(Presented.Classified));

    Cursor += Scaled.LayerSwatch + Scaled.LayerGap;

    // 📐 The naming pair — the layer's name over `{blend} · {opacity}%`.
    const float NameRun    = Scaled.RunSecondary;
    const float DetailRun  = Scaled.RunFine * 0.95f;
    const float PairAcross = NameRun * 1.2f + DetailRun * 1.3f;
    const float PairTop    = LayerHalf.LeastAcross + (LayerHalf.SpanAcross() - PairAcross) * 0.5f;
    const float NameCeil   = LayerHalf.MostAlong - Cursor - Scaled.LayerRowPad;

    // 🔴 Both runs are borrowed from the host and neither is dereferenced unchecked. `TextRunTruncated`
    //    tolerates an absent run; the blend is copied byte by byte here, so it is the one that must be
    //    guarded before the walk rather than inside it.
    const char* const Blending = (Presented.Blend != nullptr) ? Presented.Blend : "Normal";

    Surface->TextRunTruncated(Cursor, PairTop, NameCeil, Tinted.Primary, Presented.Naming, NameRun, true);

    char Detail[48] = {};
    std::uint32_t Written = 0u;

    for (const char* Reading = Blending; *Reading != '\0' && Written < 30u; ++Reading)
        Detail[Written++] = *Reading;

    const char* const Separator = " \u00B7 ";

    for (const char* Reading = Separator; *Reading != '\0' && Written < 40u; ++Reading)
        Detail[Written++] = *Reading;

    const std::uint32_t Percent = (Presented.Opacity > 100u) ? 100u : Presented.Opacity;

    if (Percent >= 100u)      { Detail[Written++] = '1'; Detail[Written++] = '0'; Detail[Written++] = '0'; }
    else if (Percent >= 10u)  { Detail[Written++] = static_cast<char>('0' + Percent / 10u);
                                Detail[Written++] = static_cast<char>('0' + Percent % 10u); }
    else                      { Detail[Written++] = static_cast<char>('0' + Percent); }

    Detail[Written++] = '%';
    Detail[Written]   = '\0';

    Surface->TextRunTruncated(Cursor, PairTop + NameRun * 1.2f, NameCeil,
                              Covering(0x6A6A6Au), Detail, DetailRun, false);

    // ④ The right half — either the "No Mask" prompt or the mask's own eye, chip, pair and cross.
    float MaskCursor = MaskHalf.LeastAlong + Scaled.LayerRowPad;

    PlaneExtent MaskEye = {};
    bool        OnMaskEye = false;

    if (!Presented.MaskDeclared)
    {
        const PlaneExtent Absent = Spanning(MaskCursor, MaskHalf.LeastAcross
                                            + (MaskHalf.SpanAcross() - Scaled.LayerSwatch) * 0.5f,
                                            Scaled.LayerSwatch, Scaled.LayerSwatch);

        Surface->Edge(Absent, Partial(0x8A8A8Au, 0.40), 1.0f, 5.0f, CornerAll);
        Surface->Stroke(SymbolSubject::PlusCross,
                        Spanning(Absent.LeastAlong + 8.0f, Absent.LeastAcross + 8.0f,
                                 Scaled.LayerSwatch - 16.0f, Scaled.LayerSwatch - 16.0f),
                        Partial(0x8A8A8Au, 0.40));

        MaskCursor += Scaled.LayerSwatch + Scaled.LayerGap;

        Surface->TextRun(MaskCursor, MaskHalf.LeastAcross + (MaskHalf.SpanAcross() - Scaled.RunSmall) * 0.5f,
                         Partial(0x8A8A8Au, 0.40), "No Mask", Scaled.RunSmall);
    }
    else
    {
        MaskEye   = Spanning(MaskCursor, ActionMid, Scaled.LayerAction, Scaled.LayerAction);
        OnMaskEye = MaskEye.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

        Surface->Stroke(SymbolSubject::EyeOpen,
                        Spanning(MaskEye.LeastAlong + 3.5f, MaskEye.LeastAcross + 3.5f,
                                 Scaled.LayerAction - 7.0f, Scaled.LayerAction - 7.0f),
                        OnMaskEye ? Tinted.Primary : Tinted.Muted);

        MaskCursor += Scaled.LayerAction + Scaled.LayerGap * 0.5f;

        // 📐 The mask chip — a white square whose opacity states the mask's strength.
        const PlaneExtent Chip = Spanning(MaskCursor, MaskHalf.LeastAcross
                                          + (MaskHalf.SpanAcross() - Scaled.LayerSwatch) * 0.5f,
                                          Scaled.LayerSwatch, Scaled.LayerSwatch);

        Surface->Ground(Chip, Covering(0x000000u), Scaled.FieldRadius, CornerAll);
        Surface->Edge(Chip, Covering(0x1C1C1Cu), 1.0f, Scaled.FieldRadius, CornerAll);

        const double Strength = static_cast<double>((Presented.MaskStrength > 100u)
                                                  ? 100u : Presented.MaskStrength) / 100.0;

        Surface->Ground(Spanning(Chip.LeastAlong + 2.0f, Chip.LeastAcross + 2.0f,
                                 Scaled.LayerSwatch - 4.0f, Scaled.LayerSwatch - 4.0f),
                        Partial(0xFFFFFFu, Strength), 3.0f, CornerAll);

        MaskCursor += Scaled.LayerSwatch + Scaled.LayerGap;

        char Strong[8] = {};
        std::uint32_t Marked = 0u;
        const std::uint32_t Reading = (Presented.MaskStrength > 100u) ? 100u : Presented.MaskStrength;

        if (Reading >= 100u)     { Strong[Marked++] = '1'; Strong[Marked++] = '0'; Strong[Marked++] = '0'; }
        else if (Reading >= 10u) { Strong[Marked++] = static_cast<char>('0' + Reading / 10u);
                                   Strong[Marked++] = static_cast<char>('0' + Reading % 10u); }
        else                     { Strong[Marked++] = static_cast<char>('0' + Reading); }

        Strong[Marked++] = '%';
        Strong[Marked]   = '\0';

        Surface->TextRunTruncated(MaskCursor, PairTop, MaskHalf.MostAlong - MaskCursor - 44.0f,
                                  Tinted.Primary, "Mask", NameRun, true);
        Surface->TextRunTruncated(MaskCursor, PairTop + NameRun * 1.2f,
                                  MaskHalf.MostAlong - MaskCursor - 44.0f,
                                  Covering(0x6A6A6Au), Strong, DetailRun, false);
    }

    // 📝 The bin sits at the trailing edge with `ml-auto`, whether or not a mask stands.
    const PlaneExtent Retire = Spanning(MaskHalf.MostAlong - Scaled.LayerAction - Scaled.LayerRowPad,
                                        ActionMid, Scaled.LayerAction, Scaled.LayerAction);

    const bool OnRetire = Retire.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

    Surface->Stroke(SymbolSubject::TrashBin,
                    Spanning(Retire.LeastAlong + 3.5f, Retire.LeastAcross + 3.5f,
                             Scaled.LayerAction - 7.0f, Scaled.LayerAction - 7.0f),
                    OnRetire ? Covering(0xEF4444u) : Tinted.Muted);

    // ⑤ Every contact for this row, resolved in the reference's own precedence: the actions outrank the
    //    halves, and the halves outrank nothing else because the card takes no contact of its own.
    if (Sampled.ContactArrived && !Ledger->AnyDisclosed())
    {
        if (OnChevron)
            Ledger->Seize(LayerFolds[Ordinal], ControlPart::Chevron);
        else if (OnPresence)
            Ledger->Seize(LayerPresences[Ordinal], ControlPart::Body);
        else if (OnRetire)
            Ledger->Seize(LayerRetires[Ordinal], ControlPart::Body);
        else if (OnMaskEye)
            Ledger->Seize(LayerMaskEyes[Ordinal], ControlPart::Body);
        else if (LayerHalf.Encloses(Sampled.PositionAlong, Sampled.PositionAcross))
            Ledger->Seize(LayerHalves[Ordinal * 2u], ControlPart::Body);
        else if (MaskHalf.Encloses(Sampled.PositionAlong, Sampled.PositionAcross))
            Ledger->Seize(LayerHalves[Ordinal * 2u + 1u], ControlPart::Body);
    }

    if (OnChevron && Ledger->Released(LayerFolds[Ordinal]))
        Seated.LayerUnfolded[Ordinal] = !Seated.LayerUnfolded[Ordinal];

    if (OnPresence && Ledger->Released(LayerPresences[Ordinal]))
        Seated.LayerShown[Ordinal] = !Seated.LayerShown[Ordinal];

    if (Ledger->Released(LayerHalves[Ordinal * 2u])
        && LayerHalf.Encloses(Sampled.PositionAlong, Sampled.PositionAcross))
    {
        Seated.LayerTaken  = Ordinal;
        Seated.TargetTaken = LayerTarget::Layer;
    }

    if (Ledger->Released(LayerHalves[Ordinal * 2u + 1u])
        && MaskHalf.Encloses(Sampled.PositionAlong, Sampled.PositionAcross))
    {
        Seated.LayerTaken  = Ordinal;
        Seated.TargetTaken = LayerTarget::Mask;
    }

    if (!Unfolded)
        return Scaled.LayerHeadAcross;

    // ⑥ The folded half — two property columns beneath the head, `border-t` and a darker ground.
    const PlaneExtent Folded = Spanning(Card.LeastAlong, Head.MostAcross,
                                        Card.SpanAlong(), Extent.SpanAcross() - Scaled.LayerHeadAcross);

    Surface->Ground(Folded, Partial(0x000000u, 0.15), 0.0f, CornerNone);
    Surface->Ground(Spanning(Folded.LeastAlong, Folded.LeastAcross, Folded.SpanAlong(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    Surface->Ground(Spanning(Folded.LeastAlong + HalfAlong, Folded.LeastAcross + Scaled.LayerFoldPad,
                             1.0f, Folded.SpanAcross() - Scaled.LayerFoldPad * 2.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    // 📐 A labelled property row: a 50 px caption column and the control beside it.
    const auto RecordField = [&](float Along, float Across, float Along2, const char* Caption,
                                 const char* Reading)
    {
        Surface->TextRun(Along, Across + (Scaled.LayerFieldRow - Scaled.RunFine) * 0.5f,
                         Covering(0x8A8A8Au), Caption, Scaled.RunFine);

        const float ValueAlong = Along + Scaled.LayerLabelAlong;
        const PlaneExtent Well = Spanning(ValueAlong, Across + 2.0f,
                                          Along2 - Scaled.LayerLabelAlong, Scaled.LayerFieldRow - 4.0f);

        Surface->Ground(Well, Tinted.MenuLower, Scaled.FieldRadius, CornerAll);
        Surface->Edge(Well, Tinted.Hairline, 1.0f, Scaled.FieldRadius, CornerAll);
        Surface->TextRunTruncated(Well.LeastAlong + 6.0f,
                                  Well.LeastAcross + (Well.SpanAcross() - Scaled.RunFine) * 0.5f,
                                  Well.SpanAlong() - 12.0f, Tinted.Primary, Reading, Scaled.RunFine);
    };

    const float ColumnAlong = HalfAlong - Scaled.LayerFoldPad * 2.0f;
    float       FoldCursor  = Folded.LeastAcross + Scaled.LayerFoldPad;

    RecordField(Folded.LeastAlong + Scaled.LayerFoldPad, FoldCursor, ColumnAlong, "Blend", Blending);

    FoldCursor += Scaled.LayerFieldRow + Scaled.LayerGap;

    char Opac[8] = {};
    std::uint32_t Marked = 0u;

    if (Percent >= 100u)     { Opac[Marked++] = '1'; Opac[Marked++] = '0'; Opac[Marked++] = '0'; }
    else if (Percent >= 10u) { Opac[Marked++] = static_cast<char>('0' + Percent / 10u);
                               Opac[Marked++] = static_cast<char>('0' + Percent % 10u); }
    else                     { Opac[Marked++] = static_cast<char>('0' + Percent); }

    Opac[Marked++] = '%';
    Opac[Marked]   = '\0';

    RecordField(Folded.LeastAlong + Scaled.LayerFoldPad, FoldCursor, ColumnAlong, "Opac", Opac);

    FoldCursor += Scaled.LayerFieldRow + Scaled.LayerGap;

    Surface->TextRunCapitalised(Folded.LeastAlong + Scaled.LayerFoldPad, FoldCursor,
                                Covering(0x6A6A6Au), "Channels", Scaled.RunFine);

    FoldCursor += Scaled.RunFine * 1.6f;

    // 📐 The channel pills, wrapped as the reference's `flex-wrap` wraps them.
    float PillAlong = Folded.LeastAlong + Scaled.LayerFoldPad;

    for (std::uint32_t Channel = 0u;
         Channel < Presented.ChannelCount && Channel < LayerRow::ChannelCeiling; ++Channel)
    {
        const char* Naming = Presented.Channels[Channel];

        if (Naming == nullptr || Naming[0] == '\0')
            continue;

        const float PillRun   = Surface->MeasureRun(Naming, Scaled.RunFine, 0.0f);
        const float PillWidth = PillRun + 16.0f;

        if (PillAlong + PillWidth > Folded.LeastAlong + Scaled.LayerFoldPad + ColumnAlong)
        {
            PillAlong   = Folded.LeastAlong + Scaled.LayerFoldPad;
            FoldCursor += Scaled.LayerPillAcross + 4.0f;
        }

        const PlaneExtent Pill = Spanning(PillAlong, FoldCursor, PillWidth, Scaled.LayerPillAcross);

        Surface->Ground(Pill, Covering(0x1C1C1Cu), 4.0f, CornerAll);
        Surface->Edge(Pill, Covering(0x2A2A2Au), 1.0f, 4.0f, CornerAll);
        Surface->TextRun(Pill.LeastAlong + 8.0f,
                         Pill.LeastAcross + (Scaled.LayerPillAcross - Scaled.RunFine) * 0.5f,
                         Covering(0x8A8A8Au), Naming, Scaled.RunFine);

        PillAlong += PillWidth + 4.0f;
    }

    // ⑦ The mask column, which either offers "Add Mask" or states the mask's own properties.
    const float MaskColumnAlong = Folded.LeastAlong + HalfAlong + Scaled.LayerFoldPad;
    float       MaskFold        = Folded.LeastAcross + Scaled.LayerFoldPad;

    if (!Presented.MaskDeclared)
    {
        const float ButtonAlong = 74.0f;
        const PlaneExtent Add = Spanning(MaskColumnAlong + (ColumnAlong - ButtonAlong) * 0.5f,
                                         Folded.LeastAcross + (Folded.SpanAcross() - Scaled.LayerFieldRow) * 0.5f,
                                         ButtonAlong, Scaled.LayerFieldRow);

        Surface->Ground(Add, Tinted.Accent, Scaled.FieldRadius, CornerAll);
        Surface->TextRun(Add.LeastAlong + (ButtonAlong
                                           - Surface->MeasureRun("Add Mask", Scaled.RunFine, 0.0f)) * 0.5f,
                         Add.LeastAcross + (Scaled.LayerFieldRow - Scaled.RunFine) * 0.5f,
                         Covering(0xFFFFFFu), "Add Mask", Scaled.RunFine);
    }
    else
    {
        char Strong[8] = {};
        std::uint32_t Stated = 0u;
        const std::uint32_t Reading = (Presented.MaskStrength > 100u) ? 100u : Presented.MaskStrength;

        if (Reading >= 100u)     { Strong[Stated++] = '1'; Strong[Stated++] = '0'; Strong[Stated++] = '0'; }
        else if (Reading >= 10u) { Strong[Stated++] = static_cast<char>('0' + Reading / 10u);
                                   Strong[Stated++] = static_cast<char>('0' + Reading % 10u); }
        else                     { Strong[Stated++] = static_cast<char>('0' + Reading); }

        Strong[Stated++] = '%';
        Strong[Stated]   = '\0';

        RecordField(MaskColumnAlong, MaskFold, ColumnAlong, "Str", Strong);

        MaskFold += Scaled.LayerFieldRow + Scaled.LayerGap;

        Surface->TextRun(MaskColumnAlong, MaskFold + (Scaled.LayerFieldRow - Scaled.RunFine) * 0.5f,
                         Covering(0x8A8A8Au), "Invert", Scaled.RunFine);

        // 📐 The invert switch, `w-[26px] h-[14px]` with a 10 px nub.
        const PlaneExtent Switch = Spanning(MaskColumnAlong + Scaled.LayerLabelAlong,
                                            MaskFold + (Scaled.LayerFieldRow - Scaled.LayerSwitchAcross) * 0.5f,
                                            Scaled.LayerSwitchAlong, Scaled.LayerSwitchAcross);

        Surface->Ground(Switch, Presented.MaskInverted ? Tinted.Accent : Covering(0x2A2A2Au),
                        Scaled.LayerSwitchAcross * 0.5f, CornerAll);
        Surface->Edge(Switch, Presented.MaskInverted ? Tinted.Accent : Covering(0x3A3A3Au),
                      1.0f, Scaled.LayerSwitchAcross * 0.5f, CornerAll);

        const float Nub = Scaled.LayerSwitchAcross - 4.0f;
        Surface->Medallion(Presented.MaskInverted ? Switch.MostAlong - 2.0f - Nub * 0.5f
                                                  : Switch.LeastAlong + 2.0f + Nub * 0.5f,
                           Switch.LeastAcross + Scaled.LayerSwitchAcross * 0.5f,
                           Nub * 0.5f, Covering(0xEDEDEDu));

        MaskFold += Scaled.LayerFieldRow + Scaled.LayerGap;

        Surface->TextRunCapitalised(MaskColumnAlong, MaskFold, Covering(0x6A6A6Au), "Sources", Scaled.RunFine);

        MaskFold += Scaled.RunFine * 1.6f;

        const PlaneExtent Source = Spanning(MaskColumnAlong, MaskFold, ColumnAlong, 24.0f);

        Surface->Ground(Source, Tinted.MenuLower, Scaled.FieldRadius, CornerAll);
        Surface->Edge(Source, Tinted.Hairline, 1.0f, Scaled.FieldRadius, CornerAll);
        Surface->Ground(Spanning(Source.LeastAlong + 6.0f, Source.LeastAcross + 6.0f, 12.0f, 12.0f),
                        Covering(0x10B981u), 2.0f, CornerAll);
        Surface->TextRun(Source.LeastAlong + 24.0f, Source.LeastAcross + (24.0f - Scaled.RunFine) * 0.5f,
                         Tinted.Primary,
                         (Presented.Classified == LayerClassification::Generator) ? "Generator" : "Generated",
                         Scaled.RunFine);
    }

    return Extent.SpanAcross();
}

void GlobalShellPanel::RecordLayerStack(const PlaneExtent& Extent, ShellOrdinates& Seated,
                                        const LayerRow* Layers, std::uint32_t LayerCount)
{
    // 📝 The pane's own ground is `bg-[#0b0b0b]`, which is darker than `--menu`; the reference states it as
    //    a literal rather than through a custom property, so it is a literal here too.
    Surface->Ground(Extent, Covering(0x0B0B0Bu), 0.0f, CornerNone);
    Surface->Ground(Spanning(Extent.MostAlong - 1.0f, Extent.LeastAcross, 1.0f, Extent.SpanAcross()),
                    Tinted.Hairline, 0.0f, CornerNone);

    const std::uint32_t Presented = (LayerCount < LayerCeiling) ? LayerCount : LayerCeiling;

    // ① The header — a medallion, "Layer Stack", its subtitle, and the count pill at the trailing edge.
    const PlaneExtent Header = Spanning(Extent.LeastAlong, Extent.LeastAcross,
                                        Extent.SpanAlong(), Scaled.HeaderAcross);

    RecordPaneHeader(Header, SymbolSubject::LayerMerge, Covering(0x8A8A8Au), Covering(0x000000u),
                     "Layer Stack", "Suzanne \u00B7 one material + paint");

    char Counted[4] = {};
    std::uint32_t Marked = 0u;

    if (Presented >= 10u) Counted[Marked++] = static_cast<char>('0' + (Presented / 10u) % 10u);
    Counted[Marked++] = static_cast<char>('0' + Presented % 10u);
    Counted[Marked]   = '\0';

    const float PillRun   = Surface->MeasureRun(Counted, Scaled.RunFine, 0.0f);
    const float PillAlong = PillRun + 12.0f;

    const PlaneExtent CountPill = Spanning(Header.MostAlong - Scaled.HeaderPadAlong - PillAlong,
                                           Header.LeastAcross + (Header.SpanAcross() - 20.0f) * 0.5f,
                                           PillAlong, 20.0f);

    Surface->Ground(CountPill, Covering(0x1B1B1Bu), 10.0f, CornerAll);
    Surface->Edge(CountPill, Covering(0x2A2A2Au), 1.0f, 10.0f, CornerAll);
    Surface->TextRun(CountPill.LeastAlong + 6.0f, CountPill.LeastAcross + (20.0f - Scaled.RunFine) * 0.5f,
                     Covering(0x8A8A8Au), Counted, Scaled.RunFine);

    // ② The toolbar — the full-width "Add layer" button and the retention field beneath it.
    const float Pad = Scaled.PanePad;

    const PlaneExtent Add = Spanning(Extent.LeastAlong + Pad, Header.MostAcross + Pad,
                                     Extent.SpanAlong() - Pad * 2.0f, Scaled.LayerToolAcross);

    const bool OnAdd = Add.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

    if (OnAdd && Sampled.ContactArrived && !Ledger->AnyDisclosed())
        Ledger->Seize(LayerAdd, ControlPart::Body);

    Surface->Ground(Add, Covering(0x141414u), 7.0f, CornerAll);
    Surface->Edge(Add, OnAdd ? Covering(0x3A3A3Au) : Covering(0x242424u), 1.0f, 7.0f, CornerAll);

    const float AddRun   = Surface->MeasureRun("Add layer", Scaled.RunSecondary, 0.0f);
    const float AddLead  = Add.LeastAlong + (Add.SpanAlong() - AddRun - 18.0f) * 0.5f;
    const float AddMid   = Add.LeastAcross + (Add.SpanAcross() - Scaled.RunSecondary) * 0.5f;

    Surface->Stroke(SymbolSubject::PlusCross,
                    Spanning(AddLead, Add.LeastAcross + (Add.SpanAcross() - 13.0f) * 0.5f, 13.0f, 13.0f),
                    OnAdd ? Tinted.Primary : Covering(0x8A8A8Au));

    Surface->TextRun(AddLead + 18.0f, AddMid, OnAdd ? Tinted.Primary : Covering(0x8A8A8Au),
                     "Add layer", Scaled.RunSecondary);

    // 📝 The reference's filter field carries no border until it is roused or taken.
    const PlaneExtent Retaining = Spanning(Extent.LeastAlong + Pad, Add.MostAcross + 5.0f,
                                           Extent.SpanAlong() - Pad * 2.0f, Scaled.SearchAcross * (26.0f / 30.0f));

    const bool OnRetaining = Retaining.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

    if (OnRetaining && Sampled.ContactArrived)
        Ledger->Seize(LayerRetention, ControlPart::Body);

    const bool Retained = Ledger->Holding(LayerRetention) || Ledger->Disclosed(LayerRetention);

    if (OnRetaining || Retained)
    {
        Surface->Ground(Retaining, Covering(0x121214u), Scaled.FieldRadius, CornerAll);
        Surface->Edge(Retaining, Retained ? Tinted.Accent : Covering(0x242424u),
                      1.0f, Scaled.FieldRadius, CornerAll);
    }

    const float SearchGlyph = 12.0f;

    Surface->Stroke(SymbolSubject::MagnifierLens,
                    Spanning(Retaining.LeastAlong + 8.0f,
                             Retaining.LeastAcross + (Retaining.SpanAcross() - SearchGlyph) * 0.5f,
                             SearchGlyph, SearchGlyph), Covering(0x5A5A5Au));

    Surface->TextRun(Retaining.LeastAlong + 26.0f,
                     Retaining.LeastAcross + (Retaining.SpanAcross() - Scaled.RunSecondary) * 0.5f,
                     Covering(0x4A4A4Au), "Filter layers\u2026", Scaled.RunSecondary);

    // ③ The stack itself, confined so a tall row cannot record over the footer.
    const PlaneExtent Footer = Spanning(Extent.LeastAlong, Extent.MostAcross - Scaled.FooterAcross,
                                        Extent.SpanAlong(), Scaled.FooterAcross);

    const PlaneExtent Body = Spanning(Extent.LeastAlong + 3.0f, Retaining.MostAcross + Pad,
                                      Extent.SpanAlong() - 6.0f,
                                      Footer.LeastAcross - Retaining.MostAcross - Pad);

    Surface->Confine(Body);

    float Cursor = Body.LeastAcross;

    for (std::uint32_t Ordinal = 0u; Ordinal < Presented && Layers != nullptr; ++Ordinal)
    {

        // 📐 An unfolded row stands taller by its two property columns. The reference sizes that half by
        //    its content; the tallest arrangement is the mask column's four stated rows.
        const float FoldedAcross = Seated.LayerUnfolded[Ordinal]
                                 ? (Scaled.LayerFoldPad * 2.0f + Scaled.LayerFieldRow * 2.0f
                                    + Scaled.LayerGap * 2.0f + Scaled.RunFine * 1.6f
                                    + Scaled.LayerPillAcross + 8.0f)
                                 : 0.0f;

        const PlaneExtent Row = Spanning(Body.LeastAlong, Cursor, Body.SpanAlong(),
                                         Scaled.LayerHeadAcross + FoldedAcross);

        Cursor += Row.SpanAcross() + Scaled.LayerRowGap;

        if (Surface->Excluded(Row))
            continue;

        static_cast<void>(RecordLayerRow(Row, Seated, Layers[Ordinal], Ordinal, Presented,
                                         Ordinal + 1u == Presented));
    }

    Surface->Release();

    // ④ The footer — shown and hidden counts, and the reorder hint.
    Surface->Ground(Footer, Covering(0x121214u), 0.0f, CornerNone);
    Surface->Ground(Spanning(Footer.LeastAlong, Footer.LeastAcross, Footer.SpanAlong(), 1.0f),
                    Covering(0x1C1C1Cu), 0.0f, CornerNone);

    std::uint32_t Standing = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < Presented; ++Ordinal)
    {
        if (Seated.LayerShown[Ordinal])
            ++Standing;
    }

    const float FootRun = Scaled.RunFine;
    const float FootMid = Footer.LeastAcross + (Footer.SpanAcross() - FootRun) * 0.5f;
    float       FootAt  = Footer.LeastAlong + 11.0f;

    Surface->Ground(Spanning(FootAt, FootMid + FootRun * 0.25f, 7.0f, 7.0f),
                    Covering(0x94A3B8u), 2.0f, CornerAll);

    FootAt += 14.0f;

    char Tally[24] = {};
    std::uint32_t Placed = 0u;

    if (Standing >= 10u) Tally[Placed++] = static_cast<char>('0' + (Standing / 10u) % 10u);
    Tally[Placed++] = static_cast<char>('0' + Standing % 10u);
    Tally[Placed]   = '\0';

    Surface->TextRun(FootAt, FootMid, Tinted.Primary, Tally, FootRun);
    FootAt += Surface->MeasureRun(Tally, FootRun, 0.0f) + 4.0f;
    Surface->TextRun(FootAt, FootMid, Covering(0x8A8A8Au), "shown", FootRun);
    FootAt += Surface->MeasureRun("shown", FootRun, 0.0f) + 7.0f;
    Surface->TextRun(FootAt, FootMid, Covering(0x2C2C2Cu), "\u00B7", FootRun);
    FootAt += 7.0f;

    const std::uint32_t Withheld = Presented - Standing;

    Placed = 0u;
    if (Withheld >= 10u) Tally[Placed++] = static_cast<char>('0' + (Withheld / 10u) % 10u);
    Tally[Placed++] = static_cast<char>('0' + Withheld % 10u);
    Tally[Placed]   = '\0';

    Surface->TextRun(FootAt, FootMid, Tinted.Primary, Tally, FootRun);
    FootAt += Surface->MeasureRun(Tally, FootRun, 0.0f) + 4.0f;
    Surface->TextRun(FootAt, FootMid, Covering(0x8A8A8Au), "hidden", FootRun);

    const float HintRun = Surface->MeasureRun("drag to reorder", FootRun, 0.0f);

    Surface->TextRun(Footer.MostAlong - 11.0f - HintRun, FootMid,
                     Covering(0x8A8A8Au), "drag to reorder", FootRun);
}

void GlobalShellPanel::RecordLayerInspector(const PlaneExtent& Extent, const ShellOrdinates& Seated,
                                            const LayerRow* Layers, std::uint32_t LayerCount)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    const std::uint32_t Presented = (LayerCount < LayerCeiling) ? LayerCount : LayerCeiling;
    const bool          Standing  = (Layers != nullptr) && (Seated.LayerTaken < Presented);

    // 📐 `LayerInspectorPane` delegates by `target`: a mask target presents `MaskPropertyPanel`, and every
    //    other presents `ChannelPropertyPanel`. Both open with the same header naming the taken layer.
    const bool Masking = Seated.TargetTaken == LayerTarget::Mask;

    const PlaneExtent Header = Spanning(Extent.LeastAlong, Extent.LeastAcross,
                                        Extent.SpanAlong(), Scaled.HeaderAcross);

    RecordPaneHeader(Header, Masking ? SymbolSubject::LayerMerge : SymbolSubject::PaintBristle,
                     Covering(0xFFFFFFu),
                     Standing ? Covering(Layers[Seated.LayerTaken].PaintHue) : Tinted.Tile,
                     Masking ? "Mask Properties" : "Channel Properties",
                     Standing ? Layers[Seated.LayerTaken].Naming : "No layer taken");

    if (!Standing)
        return;

    const LayerRow& Taken = Layers[Seated.LayerTaken];
    const float     Pad   = Scaled.PanePad * 1.5f;

    float Cursor = Header.MostAcross + Pad;

    // 📝 One card per stated property, on the same terms as the component inspector's cards.
    const auto RecordCard = [&](const char* Caption, const char* Reading, ThemeToken Marker)
    {
        const PlaneExtent Card = Spanning(Extent.LeastAlong + Pad, Cursor,
                                          Extent.SpanAlong() - Pad * 2.0f, Scaled.ComponentAcross);

        Surface->Ground(Card, Tinted.Tile, Scaled.FieldRadius, CornerAll);
        Surface->Edge(Card, Tinted.Hairline, 1.0f, Scaled.FieldRadius, CornerAll);

        Surface->Ground(Spanning(Card.LeastAlong + 8.0f,
                                 Card.LeastAcross + (Card.SpanAcross() - 8.0f) * 0.5f, 8.0f, 8.0f),
                        Marker, 2.0f, CornerAll);

        Surface->TextRun(Card.LeastAlong + 22.0f,
                         Card.LeastAcross + (Card.SpanAcross() - Scaled.RunSecondary) * 0.5f,
                         Tinted.Primary, Caption, Scaled.RunSecondary);

        const float ReadRun = Surface->MeasureRun(Reading, Scaled.RunFine, 0.0f);

        Surface->TextRun(Card.MostAlong - 10.0f - ReadRun,
                         Card.LeastAcross + (Card.SpanAcross() - Scaled.RunFine) * 0.5f,
                         Tinted.Muted, Reading, Scaled.RunFine);

        Cursor += Scaled.ComponentAcross + 6.0f;
    };

    char Reading[8] = {};
    std::uint32_t Placed = 0u;
    const std::uint32_t Stated = Masking
                               ? ((Taken.MaskStrength > 100u) ? 100u : Taken.MaskStrength)
                               : ((Taken.Opacity > 100u) ? 100u : Taken.Opacity);

    if (Stated >= 100u)     { Reading[Placed++] = '1'; Reading[Placed++] = '0'; Reading[Placed++] = '0'; }
    else if (Stated >= 10u) { Reading[Placed++] = static_cast<char>('0' + Stated / 10u);
                              Reading[Placed++] = static_cast<char>('0' + Stated % 10u); }
    else                    { Reading[Placed++] = static_cast<char>('0' + Stated); }

    Reading[Placed++] = '%';
    Reading[Placed]   = '\0';

    if (Masking)
    {
        RecordCard("Strength", Reading, Tinted.Accent);
        RecordCard("Invert", Taken.MaskInverted ? "On" : "Off", Covering(0x8B5CF6u));
        RecordCard("Source", Taken.MaskDeclared ? "Generator" : "None", Covering(0x10B981u));
        RecordCard("Base", "White", Covering(0xFFFFFFu));
    }
    else
    {
        RecordCard("Classification", ClassificationText(Taken.Classified),
                   ClassificationTint(Taken.Classified));
        RecordCard("Blend", Taken.Blend, Tinted.Accent);
        RecordCard("Opacity", Reading, Covering(0x3B82F6u));

        Cursor += 4.0f;

        Surface->TextRunCapitalised(Extent.LeastAlong + Pad, Cursor, Tinted.Faint, "Channels",
                                    Scaled.RunFine);

        Cursor += Scaled.RunFine * 1.8f;

        for (std::uint32_t Channel = 0u;
             Channel < Taken.ChannelCount && Channel < LayerRow::ChannelCeiling; ++Channel)
        {
            if (Taken.Channels[Channel] == nullptr)
                continue;

            RecordCard(Taken.Channels[Channel], "Texture", Covering(0xB87333u));
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE TWO-SLIDE STRIP
//------------------------------------------------------------------------------------------------------------------------

void GlobalShellPanel::RecordInspector(const PlaneExtent& Extent, ShellOrdinates& Seated,
                                       const EntityRow* Rows, std::uint32_t RowCount,
                                       const LayerRow* Layers, std::uint32_t LayerCount,
                                       const EntityRevision* Revisions, std::uint32_t RevisionCount)
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

    // ① Slide one. 🔴 Which pane stands here is decided by the mode, exactly as the reference's ternary in
    //    `app/page.tsx` decides it: Texture Paint presents `LayersPane` alone, filling the whole slide, and
    //    every other mode presents an outliner beside a prompt.
    if (Seated.Mode == WorkspaceMode::TexturePaint)
    {
        if (!Surface->Excluded(Leading))
            RecordLayerStack(Leading, Seated, Layers, LayerCount);

        if (!Surface->Excluded(Trailing))
            RecordLayerInspector(Trailing, Seated, Layers, LayerCount);

        Surface->Release();
        return;
    }

    if (!Surface->Excluded(Leading))
    {
        const float OutlinerAlong = (Scaled.OutlinerAlong < Leading.SpanAlong() * 0.6f)
                                  ? Scaled.OutlinerAlong : Leading.SpanAlong() * 0.6f;

        const PlaneExtent Outlining = Spanning(Leading.LeastAlong, Leading.LeastAcross,
                                               OutlinerAlong, Leading.SpanAcross());

        RecordOutlineColumn(Outlining, Seated, Rows, RowCount);

        const PlaneExtent Prompting = Spanning(Outlining.MostAlong, Leading.LeastAcross,
                                               Leading.SpanAlong() - OutlinerAlong, Leading.SpanAcross());

        // 📐 Which pane stands here is the mode's, exactly as the reference's ternary decides it. Drafting
        //    presents `MetadataPane` under its own header; World Editor presents three lines of prose and
        //    nothing else, and the reference states them as one wrapped paragraph.
        if (Seated.Mode == WorkspaceMode::Drafting)
        {
            Surface->Ground(Prompting, Tinted.MenuLower, 0.0f, CornerNone);

            const PlaneExtent Crown = Spanning(Prompting.LeastAlong, Prompting.LeastAcross,
                                               Prompting.SpanAlong(), Scaled.HeaderAcross);

            // 📐 The header is itself the call: `onClick={() => setShowInspector(true)}`, rousing to
            //    `#292930`, and it carries a chevron at the trailing edge to say so.
            const bool OnCrown = Crown.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);

            RecordPaneHeader(Crown, SymbolSubject::CrosshairCentre, Covering(0xFFFFFFu),
                             Covering(0x000000u), "Properties & Actions", nullptr);

            if (OnCrown)
                Surface->Ground(Crown, Faded(Tinted.TileRoused, 0.5f), 0.0f, CornerNone);

            const float Mark = Scaled.MedallionExtent * (13.0f / 24.0f);

            Surface->Stroke(SymbolSubject::ChevronRight,
                            Spanning(Crown.MostAlong - Scaled.HeaderPadAlong - Mark,
                                     Crown.LeastAcross + (Crown.SpanAcross() - Mark) * 0.5f, Mark, Mark),
                            OnCrown ? Tinted.Primary : Tinted.Muted);

            RecordMetadata(Spanning(Prompting.LeastAlong, Crown.MostAcross, Prompting.SpanAlong(),
                                    Prompting.MostAcross - Crown.MostAcross),
                           Seated, Rows, RowCount);
        }
        else
        {
            Surface->Ground(Prompting, Tinted.MenuLower, 0.0f, CornerNone);

            const char* Prose[3] =
            {
                "Select an entity in the Outliner and",
                "press Tab or double-click to view its",
                "properties in the Inspector slide."
            };

            const float ProseRun  = Scaled.RunSecondary;
            const float ProseStep = ProseRun * 1.625f;
            float       ProseTop  = Prompting.LeastAcross
                                  + (Prompting.SpanAcross() - ProseStep * 3.0f) * 0.5f;

            for (const char* Line : Prose)
            {
                const float LineAlong = Surface->MeasureRun(Line, ProseRun, 0.0f);

                Surface->TextRun(Prompting.LeastAlong + (Prompting.SpanAlong() - LineAlong) * 0.5f,
                                 ProseTop, Tinted.Faint, Line, ProseRun);
                ProseTop += ProseStep;
            }
        }
    }

    // ② Slide two — the component inspector for whatever the outliner has taken.
    if (!Surface->Excluded(Trailing))
        RecordComponents(Trailing, Seated, Rows, RowCount, Revisions, RevisionCount);

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE WHOLE SHELL
//------------------------------------------------------------------------------------------------------------------------

Result<bool> GlobalShellPanel::Record(const PlaneExtent&     Extent,
                                       ShellOrdinates&        Seated,
                                       const EntityRow*       Rows,
                                       std::uint32_t          RowCount,
                                       const LayerRow*        Layers,
                                       std::uint32_t          LayerCount,
                                       const EntityRevision*  Revisions,
                                       std::uint32_t          RevisionCount)
{
    if (Ledger == nullptr || Surface == nullptr || Appearance == nullptr)
        return Result<bool>::Refuse({ RefusalReason::CapabilityAbsent, "the shell panel is unconstructed" });

    if (!Surface->Recording())
        return Result<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no tick stands adopted" });

    if (Rows == nullptr)
        RowCount = 0u;

    if (RowCount > RowCeiling)
        RowCount = RowCeiling;

    if (Layers == nullptr)
        LayerCount = 0u;

    if (LayerCount > LayerCeiling)
        LayerCount = LayerCeiling;

    if (Revisions == nullptr)
        RevisionCount = 0u;

    // 🔴 The taken layer is clamped before anything reads it. A host that hands fewer layers than the
    //    ordinal the artist last took would otherwise index past the run it was given.
    if (LayerCount > 0u && Seated.LayerTaken >= LayerCount)
        Seated.LayerTaken = LayerCount - 1u;

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

        RecordInspector(Docked, Seated, Rows, RowCount, Layers, LayerCount, Revisions, RevisionCount);

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
        RecordInspector(Card, Seated, Rows, RowCount, Layers, LayerCount, Revisions, RevisionCount);
        Surface->Release();

        // 📝 The rounded corners are restored over the content, which was recorded square inside them.
        Surface->MaskCorners(Card, Tinted.Veil, Scaled.MenuRadius);
        Surface->Edge(Card, Tinted.HairlineFirm, 1.0f, Scaled.MenuRadius, CornerAll);

        InspectorAt    = Card;
        InspectorStood = true;
    }

    // ⑤ The floating context card, recorded after the veil and the summoned card both — it is the reference's
    //    `z-[101]`, which is one above the overlay that dismisses it, and therefore above everything here.
    RecordContextOverlay(Extent, Seated, Rows, RowCount);

    return Result<bool>::Result(true);
}

bool GlobalShellPanel::Occluding(float Along, float Across) const
{
    if (!InspectorStood)
        return false;

    return InspectorAt.Encloses(Along, Across);
}

}   // namespace Slate
