//============================================================================================================================================
//                                                         GLOBALSHELLPANEL.CPP
//============================================================================================================================================
// 🧩 The reference shell recorded as primitives into one command list, in one order, with no vendor widget anywhere.

#include "SlateUI/Interface/GlobalShellPanel/Api/GlobalShellPanel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     ENTITY CLASSIFICATION
//------------------------------------------------------------------------------------------------------------------------

namespace
{

constexpr double HoverOver         = 120.0;   // [ms] - the reference's transition-colors duration
constexpr double ContextArriveOver = 100.0;   // [ms] - the context card's `transition={{ duration: 0.1 }}`
constexpr float  RunLeading    =   1.30f; // [-]  - leading-tight, for a two-run header
constexpr float  MedallionEdge =   6.0f;  // [px] - rounded-md on the header medallion

/// 🧩 Holds an coordinate between two bounds.
constexpr float Held(float Coordinate, float Minimum, float Maximum)
{
    return (Coordinate < Minimum) ? Minimum : (Coordinate > Maximum) ? Maximum : Coordinate;
}

/// 🧩 One coordinate of the way from a departed figure to an incoming one.
constexpr float Between(float Previous, float Incoming, float Fraction)
{
    return Previous + (Incoming - Previous) * Fraction;
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


//------------------------------------------------------------------------------------------------------------------------
//                                                       BRING-UP
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> GlobalShellPanel::Construct(InteractionIndex&              Interaction,
                                          MotionIntegrator&              Integrator,
                                          RecordingSurface&              Surface,
                                          const ThemeProfile& Resolved)
{
    if (Ledger != nullptr)
    {
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported,
                                       "the shell panel is already constructed" });
    }

    Ledger     = &Interaction;
    Motion     = &Integrator;
    this->Surface = &Surface;
    Appearance = &Resolved;

    if (!Controls.Construct(Interaction, Surface, Resolved).Resolved)
    {
        Reset();
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent,
                                       "the shared inspector controls were rejected" });
    }

    if (!EnvironmentControls.Construct(Interaction, Surface, Resolved).Resolved)
    {
        Reset();
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent,
                                       "the environment slider controls were rejected" });
    }

    // 🔴 Every identity is claimed here and none inside a tick. A control registered mid-tick receives a fresh
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

    for (ControlIdentity* Target : Every)
    {
        const Outcome<ControlIdentity> Registered = Interaction.Register();

        if (!Registered.Resolved)
        {
            Reset();
            return Outcome<bool>::Refuse(Registered.Error);
        }

        *Target = Registered.Resolve();
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < RowCeiling; ++Ordinal)
    {
        ControlIdentity* const PerRow[] =
        {
            &RowContacts[Ordinal], &RowDisclosures[Ordinal], &RowPresences[Ordinal],
            &RowKebabs[Ordinal]
        };

        for (ControlIdentity* Target : PerRow)
        {
            const Outcome<ControlIdentity> Registered = Interaction.Register();

            if (!Registered.Resolved)
            {
                Reset();
                return Outcome<bool>::Refuse(Registered.Error);
            }

            *Target = Registered.Resolve();
        }
    }

    // 📝 The Layer Stack's rows. Each carries two takeable halves and four actions, all registered here so
    //    that nothing is claimed inside a tick.
    for (std::uint32_t Ordinal = 0u; Ordinal < LayerCeiling; ++Ordinal)
    {
        ControlIdentity* const PerLayer[] =
        {
            &LayerHalves[Ordinal * 2u], &LayerHalves[Ordinal * 2u + 1u], &LayerFolds[Ordinal],
            &LayerPresences[Ordinal],   &LayerMaskEyes[Ordinal],         &LayerRetires[Ordinal]
        };

        for (ControlIdentity* Target : PerLayer)
        {
            const Outcome<ControlIdentity> Registered = Interaction.Register();

            if (!Registered.Resolved)
            {
                Reset();
                return Outcome<bool>::Refuse(Registered.Error);
            }

            *Target = Registered.Resolve();
        }
    }

    // 📝 The six environment slider rows. Registered always, drawn only while `EnvironmentPresented`,
    //    on the same terms as every other control here — a control registered mid-tick would fade as
    //    though the pointer had just arrived over it, once per tick, forever.
    for (ControlIdentity& Slider : EnvironmentSliders)
    {
        const Outcome<ControlIdentity> Registered = Interaction.Register();

        if (!Registered.Resolved)
        {
            Reset();
            return Outcome<bool>::Refuse(Registered.Error);
        }

        Slider = Registered.Resolve();
    }

    // 📐 The two-slide strip. The reference translates it by `-translate-x-1/2` over 300 ms on
    //    cubic-bezier(.5,.05,.2,1), which `EaseCurve::Carousel` already names exactly.
    const Outcome<std::uint32_t> Registered = Integrator.RegisterEased(0.0);

    if (!Registered.Resolved)
    {
        Reset();
        return Outcome<bool>::Refuse(Registered.Error);
    }

    CarouselSlide = Registered.Resolve();

    Reapply(Resolved);

    return Outcome<bool>::Result(true);
}

void GlobalShellPanel::Advance(const PointerCondition& Contact, double Elapsed)
{
    Sampled = Contact;
    Controls.Advance(Contact, Elapsed);
    // 📝 Sampled, never advanced: the tick owner advances the shared ledger exactly once, and a
    //    second advance would retire the release before the inspector reads it.
    EnvironmentControls.Sample(Contact);
}

void GlobalShellPanel::Reapply(const ThemeProfile& Resolved)
{
    Appearance = &Resolved;

    // 🔴 The colours are taken from the appearance rather than left at their compiled-in declarations, which is
    //    what carries a theme into the shell. `Reapply` is already called at construction and again on every
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

    for (ControlIdentity& Target : ModeButtons)
        Target = {};

    for (ControlIdentity& Target : MetadataActions)
        Target = {};

    OutlineStrip = {};
    ContextVeil    = {};

    for (ControlIdentity& Target : ContextTints)
        Target = {};

    for (ControlIdentity& Target : ContextActions)
        Target = {};

    // 🔴 The layer runs were left applied by an earlier reset, so a reconstructed panel drew its stack with
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

bool GlobalShellPanel::AdvanceSummoning(ShellContext& Applied, bool Summoned, bool Dismissed,
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

        std::uint32_t StationOrdinal = 0u;

        if (Applied.InspectorDocked || Applied.MenuOpened)
        {
            StationOrdinal = Applied.InspectorShown ? (3u + Applied.InspectorTab)
                                             : (1u + Applied.OutlineTab);
        }

        std::uint32_t IncomingStation = StationOrdinal;

        if (!Reversed)
        {
            // 📐 Four onward, then wrap to one. Zero is left behind on the first press and never re-entered.
            IncomingStation = (StationOrdinal >= 4u) ? 1u : (StationOrdinal + 1u);
        }
        else if (StationOrdinal <= 1u)
        {
            // 📐 The strict reverse of the opening step withdraws the card, and only while undocked; a
            //    docked card has no withdrawn station, so it wraps to the far end of its own four instead.
            IncomingStation = (StationOrdinal == 1u && !Applied.InspectorDocked) ? 0u : 4u;
        }
        else
        {
            IncomingStation = StationOrdinal - 1u;
        }

        if (Applied.InspectorDocked && IncomingStation == 0u)
            IncomingStation = 4u;

        IncomingStation = IncomingStation % StationCount;

        Applied.MenuOpened     = Applied.InspectorDocked ? Applied.MenuOpened : (IncomingStation != 0u);
        Applied.InspectorShown = IncomingStation >= 3u;

        if (IncomingStation == 1u || IncomingStation == 2u)
            Applied.OutlineTab = IncomingStation - 1u;
        else if (IncomingStation >= 3u)
            Applied.InspectorTab = IncomingStation - 3u;

        Altered = true;
    }

    // 📐 Escape is guarded by `menuOpen && !isDocked` in the reference, so a docked inspector is NOT closed
    //    by it. The inspector goes first and the menu second, so one press never dismisses both.
    if (Dismissed && Applied.MenuOpened && !Applied.InspectorDocked)
    {
        if (Applied.InspectorShown)
            Applied.InspectorShown = false;
        else
            Applied.MenuOpened = false;

        Altered = true;
    }

    if (Altered && Motion != nullptr)
    {
        EasedInterpolant& Travelling = Motion->Eased(CarouselSlide);

        Travelling.Depart(Travelling.Current(), Applied.InspectorShown ? 1.0 : 0.0,
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
    const PlaneExtent Hairline = Spanning(Extent.MinimumX, Extent.MaximumY - 1.0f,
                                          Extent.Width(), 1.0f);
    Surface->Ground(Hairline, Tinted.Hairline, 0.0f, CornerNone);

    const float Medallion = Scaled.MedallionExtent;
    const float Centred   = Extent.MinimumY + (Extent.Height() - Medallion) * 0.5f;

    const PlaneExtent Disc = Spanning(Extent.MinimumX + Scaled.HeaderPadX, Centred,
                                      Medallion, Medallion);

    Surface->Ground(Disc, MedallionColour, MedallionEdge, CornerAll);

    // 📐 The figure sits in a 14 px square inside a 24 px medallion, centred on both axes.
    const float FigureExtent = Medallion * (14.0f / 24.0f);
    const float FigureLead   = Disc.MinimumX  + (Medallion - FigureExtent) * 0.5f;
    const float FigureY = Disc.MinimumY + (Medallion - FigureExtent) * 0.5f;

    Surface->Stroke(Glyph, Spanning(FigureLead, FigureY, FigureExtent, FigureExtent), GlyphColour);

    const float RunLead = Disc.MaximumX + Scaled.HeaderPadX * 0.8f;

    if (Secondary != nullptr && Secondary[0] != '\0')
    {
        // 📝 Two runs, `leading-tight`, applied as a pair about the header's centre rather than each about
        //    its own — which is what `flex-col` inside a centred row actually produces.
        const float PrimaryRun   = Scaled.RunPrimary;
        const float SecondaryRun = Scaled.RunFine;
        const float PairHeight   = PrimaryRun * RunLeading + SecondaryRun * RunLeading;
        const float PairLead     = Extent.MinimumY + (Extent.Height() - PairHeight) * 0.5f;

        Surface->TextRunTruncated(RunLead, PairLead, Extent.MaximumX - RunLead - Scaled.HeaderPadX,
                                  Tinted.Primary, Title, PrimaryRun, true);
        Surface->TextRunTruncated(RunLead, PairLead + PrimaryRun * RunLeading,
                                  Extent.MaximumX - RunLead - Scaled.HeaderPadX,
                                  Tinted.Faint, Secondary, SecondaryRun, false);
    }
    else
    {
        const float SoleRun  = Scaled.RunPrimary;
        const float SoleLead = Extent.MinimumY + (Extent.Height() - SoleRun) * 0.5f;

        Surface->TextRunTruncated(RunLead, SoleLead, Extent.MaximumX - RunLead - Scaled.HeaderPadX,
                                  Tinted.Primary, Title, SoleRun, true);
    }
}

void GlobalShellPanel::RecordTopBar(const PlaneExtent& Extent, const ShellContext& Applied)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    const PlaneExtent Hairline = Spanning(Extent.MinimumX, Extent.MaximumY - 1.0f,
                                          Extent.Width(), 1.0f);
    Surface->Ground(Hairline, Tinted.Hairline, 0.0f, CornerNone);

    const float Glyph  = 18.0f * (Scaled.TopBarHeight / 36.0f);
    const float Middle = Extent.MinimumY + (Extent.Height() - Glyph) * 0.5f;
    float       Sweep = Extent.MinimumX + Scaled.TopBarPadX;

    // 📝 The accent cube, `w-[18px] h-[18px] text-[var(--accent)]`.
    Surface->Stroke(SymbolSubject::CubeSolid, Spanning(Sweep, Middle, Glyph, Glyph), Tinted.Accent);
    Sweep += Glyph + Scaled.TopBarPadX * 0.85f;

    const char* Naming = (Applied.Mode == WorkspaceMode::Drafting)     ? "DraftingWorkspace"
                       : (Applied.Mode == WorkspaceMode::TexturePaint) ? "Texture Paint"
                                                                      : "World Editor";
    const char* Record = (Applied.Mode == WorkspaceMode::WorldEditor)  ? "Level_01_City.map"
                                                                      : "Bracket_Rev4.wsdoc";

    const float TitleRun = Scaled.RunPrimary;
    const float TitleTop = Extent.MinimumY + (Extent.Height() - TitleRun) * 0.5f;

    // 📐 `tracking-wide` is 0.025em, which this seam takes in em directly.
    Surface->TextRun(Sweep, TitleTop, Tinted.Primary, Naming, TitleRun, 0.025f, true);
    Sweep += Surface->MeasureRun(Naming, TitleRun, 0.025f) + Scaled.TopBarPadX * 0.85f;

    const float RecordRun = Scaled.RunSecondary * (11.0f / 11.5f);
    const float RecordTop = Extent.MinimumY + (Extent.Height() - RecordRun) * 0.5f;

    Surface->TextRun(Sweep, RecordTop, Tinted.Faint, Record, RecordRun);

    // 📐 The trailing pill: `Tab  summon inspector`, rounded-full at px-2.5 py-1.
    const char* Chord   = "Tab";
    const char* Trailing = "  summon inspector";
    const float PillRun = Scaled.RunSmall;
    const float PadX = 10.0f * (Scaled.TopBarHeight / 36.0f);
    const float PillX = Surface->MeasureRun(Chord, PillRun, 0.0f)
                          + Surface->MeasureRun(Trailing, PillRun, 0.0f) + PadX * 2.0f;
    const float PillY = PillRun * 2.0f;
    const float PillLead   = Extent.MaximumX - Scaled.TopBarPadX - PillX;
    const float PillTop    = Extent.MinimumY + (Extent.Height() - PillY) * 0.5f;

    const PlaneExtent Pill = Spanning(PillLead, PillTop, PillX, PillY);

    Surface->Ground(Pill, Tinted.Menu, PillY * 0.5f, CornerAll);
    Surface->Edge(Pill, Tinted.Hairline, 1.0f, PillY * 0.5f, CornerAll);

    const float ChordTop = PillTop + (PillY - PillRun) * 0.5f;

    Surface->TextRun(PillLead + PadX, ChordTop, Tinted.Primary, Chord, PillRun, 0.0f, true);
    Surface->TextRun(PillLead + PadX + Surface->MeasureRun(Chord, PillRun, 0.0f), ChordTop,
                     Tinted.Muted, Trailing, PillRun);
}

void GlobalShellPanel::RecordOptionsRail(const PlaneExtent& Extent, ShellContext& Applied)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    // 📝 `border-r`, the one trailing side.
    const PlaneExtent Hairline = Spanning(Extent.MaximumX - 1.0f, Extent.MinimumY,
                                          1.0f, Extent.Height());
    Surface->Ground(Hairline, Tinted.HairlineFirm, 0.0f, CornerNone);

    const PlaneExtent Header = Spanning(Extent.MinimumX, Extent.MinimumY,
                                        Extent.Width(), Scaled.HeaderHeight);

    const float HeaderRun = Scaled.RunPrimary;
    const float HeaderTop = Header.MinimumY + (Header.Height() - HeaderRun) * 0.5f;
    const float HeaderPad = Scaled.HeaderPadX * 1.6f;

    Surface->TextRun(Header.MinimumX + HeaderPad, HeaderTop, Tinted.Primary, "Options",
                     HeaderRun, 0.025f, true);

    const PlaneExtent HeaderEdge = Spanning(Header.MinimumX, Header.MaximumY - 1.0f,
                                            Header.Width(), 1.0f);
    Surface->Ground(HeaderEdge, Tinted.Hairline, 0.0f, CornerNone);

    const float BodyPad = Scaled.HeaderPadX * 1.6f;
    float       Sweep  = Header.MaximumY + BodyPad;

    // ① Dock Inspector — the reference's `.switch` and `.nub`, which `ControlPanel` already reproduces.
    const float  SwitchY = Scaled.RowHeight * 0.72f;
    const PlaneExtent SwitchRow = Spanning(Extent.MinimumX + BodyPad, Sweep,
                                           Extent.Width() - BodyPad * 2.0f, SwitchY);

    SwitchDeclaration Docking;
    Docking.Caption = "Dock Inspector";

    const bool DockedBefore = Applied.InspectorDocked;

    const ControlVerdict Docked = Controls.SwitchToggle(DockSwitch, SwitchRow, Docking,
                                                       Applied.InspectorDocked);
    static_cast<void>(Docked);

    // 🔴 Docking and undocking re-applies the slide rather than letting it travel. The reference swaps which
    //    container holds the very same element, so the strip is mounted fresh at its current offset — a
    //    travel here would slide a panel the artist never asked to move.
    if (DockedBefore != Applied.InspectorDocked && Motion != nullptr)
        Motion->Eased(CarouselSlide).Place(Applied.InspectorShown ? 1.0 : 0.0);

    Sweep += SwitchY + BodyPad * 0.5f;

    const char* Explaining = Applied.InspectorDocked
                           ? "Inspector is docked to the right side of the screen."
                           : "Inspector is hidden. Press Tab to summon it.";

    // 📐 `text-[11px] leading-relaxed` — 1.625 line height, wrapped inside the rail's own extent.
    const float ProseRun  = Scaled.RunSecondary * (11.0f / 11.5f);
    const float ProseStep = ProseRun * 1.625f;
    const float ProseX = Extent.Width() - BodyPad * 2.0f;

    // 📝 Wrapped here rather than truncated: the reference wraps, and a truncated sentence would read as a
    //    defect. The break is taken at the last space that still fits.
    const char* Walk = Explaining;

    while (Walk != nullptr && *Walk != '\0')
    {
        std::uint32_t Accepted = 0u;
        std::uint32_t Breaking = 0u;
        char          Measured[96] = {};

        while (Walk[Accepted] != '\0' && Accepted + 1u < sizeof(Measured))
        {
            Measured[Accepted] = Walk[Accepted];
            Measured[Accepted + 1u] = '\0';

            if (Surface->MeasureRun(Measured, ProseRun, 0.0f) > ProseX)
                break;

            if (Walk[Accepted] == ' ')
                Breaking = Accepted;

            ++Accepted;
        }

        const bool Whole = Walk[Accepted] == '\0';
        const std::uint32_t Taken = Whole ? Accepted : (Breaking > 0u ? Breaking : Accepted);

        for (std::uint32_t Ordinal = 0u; Ordinal < Taken; ++Ordinal)
            Measured[Ordinal] = Walk[Ordinal];

        Measured[Taken] = '\0';

        Surface->TextRun(Extent.MinimumX + BodyPad, Sweep, Tinted.Faint, Measured, ProseRun);
        Sweep += ProseStep;

        Walk += Taken;

        while (*Walk == ' ')
            ++Walk;
    }

    Sweep += BodyPad * 1.5f;

    // ② Workspace Mode — a caption, then the three buttons at `h-8` with `gap-2`.
    const float CaptionRun = Scaled.RunPrimary;

    Surface->TextRun(Extent.MinimumX + BodyPad, Sweep, Tinted.Muted, "Workspace Mode", CaptionRun);
    Sweep += CaptionRun * 1.6f + BodyPad * 0.75f;

    const char* const ModeCaptions[3] = { "Drafting", "Texture Paint", "Game Editor" };
    const float ButtonY = 32.0f * (Scaled.RowHeight / 32.0f);
    const float ButtonGap    = 8.0f  * (Scaled.RowHeight / 32.0f);

    for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
    {
        const PlaneExtent Button = Spanning(Extent.MinimumX + BodyPad, Sweep,
                                            Extent.Width() - BodyPad * 2.0f, ButtonY);

        const bool Taken  = static_cast<std::uint32_t>(Applied.Mode) == Ordinal;
        const bool Hovered = Button.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Hovered && Sampled.ContactPressed && !Ledger->AnyDisclosed())
            Ledger->Grab(ModeButtons[Ordinal], ControlPart::Body);

        if (Hovered && Ledger->Released(ModeButtons[Ordinal]))
            Applied.Mode = static_cast<WorkspaceMode>(Ordinal);

        Ledger->DeclareHovered(ModeButtons[Ordinal], Hovered, HoverOver);

        Surface->Ground(Button, Taken ? Tinted.AccentSoft : Tinted.Tile, Scaled.FieldRadius, CornerAll);
        Surface->Edge(Button, Taken ? Tinted.Accent
                                    : (Hovered ? Covering(0x444444u) : Tinted.Hairline),
                      1.0f, Scaled.FieldRadius, CornerAll);

        const float ButtonRun = Scaled.RunSecondary * (11.0f / 11.5f);
        const float RunX  = Surface->MeasureRun(ModeCaptions[Ordinal], ButtonRun, 0.0f);

        Surface->TextRun(Button.MinimumX + (Button.Width() - RunX) * 0.5f,
                         Button.MinimumY + (Button.Height() - ButtonRun) * 0.5f,
                         Taken ? Tinted.Primary : Tinted.Muted,
                         ModeCaptions[Ordinal], ButtonRun, 0.0f, true);

        Sweep += ButtonY + ButtonGap;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE VIEWPORT
//------------------------------------------------------------------------------------------------------------------------

void GlobalShellPanel::RecordViewport(const PlaneExtent& Extent, const ShellContext& Applied)
{
    Surface->Ground(Extent, Tinted.Desk, 0.0f, CornerNone);

    // 🔴 Confined before the weave. The lattice is recorded as discrete rules and the last one before each
    //    bound would otherwise overhang the viewport into the rail and the inspector.
    Surface->Confine(Extent);

    // 📝 The editor's sky, drawn in place of the weave while the environment stands presented. Phase 1
    //    renders the sun and sky from the configuration as recording-surface primitives; phase 2 replaces
    //    the dome with the atmosphere shaders reading the very same ordinates.
    if (Applied.EnvironmentPresented)
    {
        const EnvironmentConfiguration& Sky = Applied.Environment;

        // 📝 The uploaded sky dome, when the host has one: the real atmosphere evaluation, drawn
        //    through the interface's sampled-image path. The dome is direction-indexed (azimuth across
        //    the width, elevation down the height), so the viewport crops it to the camera's own field
        //    of view — which keeps the sun in frame at any viewport aspect.
        if (Applied.SkyTextureIdentity != 0u)
        {
            const float HalfV = Applied.ViewportSkyCamera.FieldOfViewDegrees * 0.5f;
            const float HalfH = std::atan(std::tan(HalfV * 3.14159265f / 180.0f)
                                          * (Extent.Width() / Extent.Height())) * 180.0f / 3.14159265f;

            const float Azimuth = Applied.ViewportSkyCamera.AzimuthDegrees;
            const float Elevation = Applied.ViewportSkyCamera.ElevationDegrees;

            // 📐 Dome coordinates: U = (azimuth + 180) / 360, V = (90 − elevation) / 180. The crop is
            //    the camera's frustum on the dome, clamped so it never leaves the texture.
            float U0 = std::clamp((Azimuth - HalfH + 180.0f) / 360.0f, 0.0f, 1.0f);
            float U1 = std::clamp((Azimuth + HalfH + 180.0f) / 360.0f, 0.0f, 1.0f);
            float V0 = std::clamp((90.0f - (Elevation + HalfV)) / 180.0f, 0.0f, 1.0f);
            float V1 = std::clamp((90.0f - (Elevation - HalfV)) / 180.0f, 0.0f, 1.0f);

            // 📐 The sun stays in frame at any viewport aspect: the camera aims twenty degrees wide of
            //    the sun, which a docked viewport's narrower frustum would otherwise crop out. When the
            //    sun's dome coordinate falls outside the frustum, the crop shifts to contain it with a
            //    small cushion — a few degrees of drift, only when the authored aim would lose the sun.
            const float SunU = std::clamp(static_cast<float>(Sky.SunAzimuth + 180.0) / 360.0f, 0.0f, 1.0f);
            const float SunV = std::clamp(static_cast<float>(90.0 - Sky.SunElevation) / 180.0f, 0.0f, 1.0f);
            constexpr float CushionU = 0.012f;   // [-] - half a disc's width, so the disc is not glued to the edge
            constexpr float CushionV = 0.012f;   // [-]

            float ShiftU = 0.0f;
            if (SunU < U0 + CushionU)
                ShiftU = U0 + CushionU - SunU;
            else if (SunU > U1 - CushionU)
                ShiftU = U1 - CushionU - SunU;

            float ShiftV = 0.0f;
            if (SunV < V0 + CushionV)
                ShiftV = V0 + CushionV - SunV;
            else if (SunV > V1 - CushionV)
                ShiftV = V1 - CushionV - SunV;

            U0 = std::clamp(U0 - ShiftU, 0.0f, 1.0f);
            U1 = std::clamp(U1 - ShiftU, 0.0f, 1.0f);
            V0 = std::clamp(V0 - ShiftV, 0.0f, 1.0f);
            V1 = std::clamp(V1 - ShiftV, 0.0f, 1.0f);

            Surface->Image(Extent, Applied.SkyTextureIdentity, U0, V0, U1, V1);
        }

        // 📐 The stylised fallback is drawn only when no uploaded texture stands — the two would
        //    otherwise stack, with the fallback's sun painted over the real one.
        if (Applied.SkyTextureIdentity == 0u)
        {
        // 📐 The dome: a zenith-to-horizon ramp, brightening with sky intensity and warming with turbidity.
        const float Intensity = static_cast<float>(Sky.SkyIntensity);
        const float Turbidity = static_cast<float>(Sky.SkyTurbidity / 10.0);
        const float ZenithMix = 0.45f + 0.30f * Intensity;
        const float HorizonMix = 0.55f + 0.30f * Intensity;

        // 📐 The dome's blue is authored here rather than read from the theme, because the reference's
        //    desk ground is deliberately dark and the sky must read as a sky in every appearance.
        const ThemeToken Zenith =
            Covering(static_cast<std::uint32_t>(
                (static_cast<unsigned>(52u + 30u * ZenithMix) << 16) |
                (static_cast<unsigned>(86u + 42u * ZenithMix) << 8) |
                static_cast<unsigned>(138u + 46u * ZenithMix)));
        const ThemeToken Horizon =
            Covering(static_cast<std::uint32_t>(
                (static_cast<unsigned>(128u + 60u * HorizonMix) << 16) |
                (static_cast<unsigned>(152u + 52u * HorizonMix) << 8) |
                static_cast<unsigned>(178u + 40u * HorizonMix)));

        Surface->Scrim(Extent, Zenith, Horizon, ScrimAxis::Y);

        // 📐 The sun disc, placed by elevation (height above the horizon) and azimuth (side along the
        //    viewport). The temperature maps 1000 K → deep red and 12000 K → blue-white.
        const float HorizonY = Extent.MinimumY + Extent.Height() * 0.62f;
        const float Elevation = std::clamp(static_cast<float>(Sky.SunElevation), 0.0f, 90.0f);
        const float Azimuth = static_cast<float>(Sky.SunAzimuth);
        const float AlongFraction = 0.5f + 0.42f * std::sin(Azimuth * 3.14159265f / 180.0f);
        const float SunX = Extent.MinimumX + Extent.Width() * AlongFraction;
        const float SunY = HorizonY - (Elevation / 90.0f) * Extent.Height() * 0.52f;

        const float Temperature = std::clamp(static_cast<float>(Sky.SunTemperature), 1000.0f, 12000.0f);
        const float T = (Temperature - 1000.0f) / 11000.0f;
        std::uint32_t SunRed = 0u;
        std::uint32_t SunGreen = 0u;
        std::uint32_t SunBlue = 0u;
        if (T < 0.5f)
        {
            SunRed = 255u;
            SunGreen = static_cast<std::uint32_t>(80.0f + T * 2.0f * 175.0f);
            SunBlue = static_cast<std::uint32_t>(30.0f + T * 2.0f * 40.0f);
        }
        else
        {
            const float U = (T - 0.5f) * 2.0f;
            SunRed = static_cast<std::uint32_t>(255.0f - U * 60.0f);
            SunGreen = static_cast<std::uint32_t>(255.0f - U * 30.0f);
            SunBlue = static_cast<std::uint32_t>(110.0f + U * 145.0f);
        }
        const ThemeToken SunColour = Covering((SunRed << 16) | (SunGreen << 8) | SunBlue);

        // 📐 The glow: three concentric halos stepping the coverage down, then the disc itself. The halo
        //    radius scales with intensity so a brighter sun reads larger.
        const float Halo = 90.0f + 40.0f * static_cast<float>(Sky.SunIntensity / 5.0);
        Surface->Medallion(SunX, SunY, Halo, Faded(SunColour, 0.10f));
        Surface->Medallion(SunX, SunY, Halo * 0.6f, Faded(SunColour, 0.16f));
        Surface->Medallion(SunX, SunY, Halo * 0.32f, Faded(SunColour, 0.30f));
        Surface->Medallion(SunX, SunY, 26.0f, SunColour);

        // 📐 The horizon line, where the ground meets the dome.
        Surface->Ground(Spanning(Extent.MinimumX, HorizonY - 1.0f, Extent.Width(), 2.0f),
                        Faded(Covering(0xFFFFFFu), 0.10f), 0.0f, CornerNone);
        Surface->Ground(Spanning(Extent.MinimumX, HorizonY, Extent.Width(),
                                 Extent.MaximumY - HorizonY),
                        Covering(0x101418u), 0.0f, CornerNone);

        // 📐 The atmosphere's aerial perspective: a band over the horizon that thickens with density.
        const float HazeHeight = 60.0f * static_cast<float>(Sky.AtmosphereDensity);
        Surface->Scrim(Spanning(Extent.MinimumX, HorizonY - HazeHeight, Extent.Width(), HazeHeight),
                       Faded(Covering(0x000000u), 0.0f),
                       Faded(Covering(0xC8D8E8u), 0.28f * static_cast<float>(Sky.AtmosphereDensity)),
                       ScrimAxis::Y);

        // 📐 The weave is retained faintly so the scene still reads as the editor's grid; it is what the
        //    artist expects under a selection.
        const float Steps[2] = { Scaled.WeaveFineStep, Scaled.WeaveCoarseStep };
        const ThemeToken WeaveColours[2] = { Tinted.WeaveFine, Tinted.WeaveCoarse };
        for (std::uint32_t Pass = 0u; Pass < 2u; ++Pass)
        {
            const float Step = Steps[Pass];
            if (Step < 1.0f)
                continue;
            for (float X = Extent.MinimumX; X < Extent.MaximumX; X += Step)
                Surface->Ground(Spanning(X, Extent.MinimumY, 1.0f, Extent.Height()),
                                WeaveColours[Pass], 0.0f, CornerNone);
            for (float Y = Extent.MinimumY; Y < Extent.MaximumY; Y += Step)
                Surface->Ground(Spanning(Extent.MinimumX, Y, Extent.Width(), 1.0f),
                                WeaveColours[Pass], 0.0f, CornerNone);
        }
        }   // [-] - the stylised fallback, only without an uploaded texture
    }       // [-] - the presented environment's viewport: the uploaded dome or the stylised fallback
    else
    {
    // 📐 The empty editor viewport, when no environment stands presented: the weave and the vignette
    //    over the desk, and the centred hint. These are deliberately exclusive with the sky above —
    //    drawing them over the uploaded dome would wash the atmosphere out to the desk's coverage.
    //    The weave is two lattices, exactly as the reference declares: 28 px at 0.028 coverage, then
    //    140 px at 0.055. Each is one-pixel rules, which is what a `linear-gradient(… 1px, transparent
    //    1px)` produces.
    const float Steps[2]        = { Scaled.WeaveFineStep, Scaled.WeaveCoarseStep };
    const ThemeToken Colours[2]   = { Tinted.WeaveFine, Tinted.WeaveCoarse };

    for (std::uint32_t Pass = 0u; Pass < 2u; ++Pass)
    {
        const float Step = Steps[Pass];

        if (Step < 1.0f)
            continue;

        for (float X = Extent.MinimumX; X < Extent.MaximumX; X += Step)
        {
            Surface->Ground(Spanning(X, Extent.MinimumY, 1.0f, Extent.Height()),
                            Colours[Pass], 0.0f, CornerNone);
        }

        for (float Y = Extent.MinimumY; Y < Extent.MaximumY; Y += Step)
        {
            Surface->Ground(Spanning(Extent.MinimumX, Y, Extent.Width(), 1.0f),
                            Colours[Pass], 0.0f, CornerNone);
        }
    }

    // 📐 `radial-gradient(ellipse at 50% 45%, transparent 40%, rgba(0,0,0,.55) 100%)`. A radial ramp is not a
    //    surface primitive, so it is approximated by concentric rings stepping the coverage — sixteen rings
    //    is below the coordinate quantum of an eight-bit channel over this range, so no banding is visible.
    constexpr std::uint32_t RingCount = 16u;

    const float CentreX  = Extent.MinimumX  + Extent.Width()  * 0.50f;
    const float CentreY = Extent.MinimumY + Extent.Height() * 0.45f;
    const float Reach        = std::sqrt(Extent.Width()  * Extent.Width()
                                       + Extent.Height() * Extent.Height()) * 0.5f;

    for (std::uint32_t Ring = 0u; Ring < RingCount; ++Ring)
    {
        // 📝 Recorded outermost inward, each ring covering the whole disc at its own radius, so the coverage
        //    accumulates toward the edge exactly as a ramp from 40 % outward does.
        const float Fraction = 1.0f - static_cast<float>(Ring) / static_cast<float>(RingCount);
        const float Radius   = Reach * Between(0.40f, 1.0f, Fraction);
        const float Coverage = 0.55f / static_cast<float>(RingCount);

        Surface->Medallion(CentreX, CentreY, Radius,
                           Faded(Covering(0x000000u), Coverage));
    }

    // 📐 The centred hint, `press Tab to …`, with the second run present only while undocked.
    const char* Leading  = "press ";
    const char* Chord    = "Tab";
    const char* Trailing = Applied.InspectorDocked
                         ? ((Applied.Mode == WorkspaceMode::Drafting)     ? " to slide through properties"
                          : (Applied.Mode == WorkspaceMode::TexturePaint) ? " to slide through channels"
                                                                         : " to slide through components")
                         : ((Applied.Mode == WorkspaceMode::Drafting)     ? " to summon the scene directory"
                          : (Applied.Mode == WorkspaceMode::TexturePaint) ? " to summon layers"
                                                                         : " to summon the outliner");

    const float HintRun   = Scaled.RunSecondary * (12.0f / 11.5f);
    const float ChordPad  = 8.0f * (Scaled.RowHeight / 32.0f);
    const float ChordRun  = Scaled.RunSecondary * (11.0f / 11.5f);
    const float ChordX = Surface->MeasureRun(Chord, ChordRun, 0.0f) + ChordPad * 2.0f;
    const float HintX = Surface->MeasureRun(Leading, HintRun, 0.0f) + ChordX
                          + Surface->MeasureRun(Trailing, HintRun, 0.0f);

    float HintLead  = CentreX - HintX * 0.5f;
    const float HintTop = Extent.MinimumY + Extent.Height() * 0.5f - HintRun;

    Surface->TextRun(HintLead, HintTop, Tinted.Faint, Leading, HintRun);
    HintLead += Surface->MeasureRun(Leading, HintRun, 0.0f);

    // 📐 The `kbd` cap: `border-b-2`, so the lower edge is drawn twice.
    const float ChordHeight = ChordRun * 1.9f;
    const PlaneExtent Cap   = Spanning(HintLead, HintTop + (HintRun - ChordHeight) * 0.5f,
                                       ChordX, ChordHeight);

    Surface->Ground(Cap, Tinted.Menu, Scaled.FieldRadius, CornerAll);
    Surface->Edge(Cap, Tinted.HairlineFirm, 1.0f, Scaled.FieldRadius, CornerAll);
    Surface->Ground(Spanning(Cap.MinimumX, Cap.MaximumY - 2.0f, Cap.Width(), 2.0f),
                    Tinted.HairlineFirm, 0.0f, CornerNone);
    Surface->TextRun(Cap.MinimumX + ChordPad, Cap.MinimumY + (ChordHeight - ChordRun) * 0.5f,
                     Tinted.Primary, Chord, ChordRun, 0.0f, true);

    HintLead += ChordX;
    Surface->TextRun(HintLead, HintTop, Tinted.Faint, Trailing, HintRun);

    if (!Applied.InspectorDocked)
    {
        const char* SecondLeading  = "Tab";
        const char* SecondTrailing = (Applied.Mode == WorkspaceMode::Drafting)
                                   ? " again slides through to properties"
                                   : (Applied.Mode == WorkspaceMode::TexturePaint)
                                   ? " again slides through to channels"
                                   : " again slides through to components";

        const float SecondX = Surface->MeasureRun(SecondLeading, ChordRun, 0.0f) + ChordPad * 2.0f
                                + Surface->MeasureRun(SecondTrailing, HintRun, 0.0f);
        float       SecondLead  = CentreX - SecondX * 0.5f;
        const float SecondTop   = HintTop + HintRun * 1.9f;

        const PlaneExtent SecondCap = Spanning(SecondLead, SecondTop + (HintRun - ChordHeight) * 0.5f,
                                               Surface->MeasureRun(SecondLeading, ChordRun, 0.0f)
                                               + ChordPad * 2.0f, ChordHeight);

        Surface->Ground(SecondCap, Tinted.Menu, Scaled.FieldRadius, CornerAll);
        Surface->Edge(SecondCap, Tinted.HairlineFirm, 1.0f, Scaled.FieldRadius, CornerAll);
        Surface->Ground(Spanning(SecondCap.MinimumX, SecondCap.MaximumY - 2.0f,
                                 SecondCap.Width(), 2.0f), Tinted.HairlineFirm, 0.0f, CornerNone);
        Surface->TextRun(SecondCap.MinimumX + ChordPad,
                         SecondCap.MinimumY + (ChordHeight - ChordRun) * 0.5f,
                         Tinted.Primary, SecondLeading, ChordRun, 0.0f, true);

        SecondLead += SecondCap.Width();
        Surface->TextRun(SecondLead, SecondTop, Tinted.Faint, SecondTrailing, HintRun);
    }

    // 📐 The lower hint strip: a scrim from transparent to rgba(0,0,0,.5), then the four navigation runs.
    const PlaneExtent Strip = Spanning(Extent.MinimumX, Extent.MaximumY - Scaled.StatusY,
                                       Extent.Width(), Scaled.StatusY);

    Surface->Scrim(Strip, Faded(Covering(0x000000u), 0.0f), Faded(Covering(0x000000u), 0.5f),
                   ScrimAxis::Y);

    const char* const Navigating[4] = { "Orbit LMB", "Pan MMB", "Zoom Wheel", "Inspector Tab" };
    const float StatusRun = Scaled.RunSmall;
    const float StatusTop = Strip.MinimumY + (Strip.Height() - StatusRun) * 0.5f;
    const float StatusGap = 9.0f * (Scaled.RowHeight / 32.0f);
    float       StatusLead = Strip.MinimumX + 13.0f * (Scaled.RowHeight / 32.0f);

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

    }

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE OUTLINER
//------------------------------------------------------------------------------------------------------------------------

bool GlobalShellPanel::RowCurrent(const ShellContext& Applied, const EntityRow* Rows,
                                    std::uint32_t RowCount, std::uint32_t Ordinal) const
{
    if (Ordinal >= RowCount)
        return false;

    const bool Retaining = Applied.EntityRetention[0] != '\0';

    // 📐 The reference retains a row when it matches, or when any row it holds matches. Retention also forces
    //    every branch open — `const isExpanded = node.expanded || !!filterText` — so disclosure is not
    //    consulted at all while a filter stands.
    if (Retaining)
    {
        if (RunHolds(Rows[Ordinal].Naming, Applied.EntityRetention))
            return true;

        for (std::uint32_t Enclosed = 0u; Enclosed < RowCount; ++Enclosed)
        {
            if (Rows[Enclosed].Enclosing == Ordinal && RowCurrent(Applied, Rows, RowCount, Enclosed))
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
        if (!Applied.EntityExpanded[Walking])
            return false;

        Walking = Rows[Walking].Enclosing;
    }

    return true;
}

void GlobalShellPanel::RecordRetentionField(const PlaneExtent& Extent, ShellContext& Applied)
{
    const bool Hovered = Extent.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Hovered && Sampled.ContactPressed)
        Ledger->Grab(RetentionField, ControlPart::Body);

    const bool Taken = Ledger->Holding(RetentionField) || Ledger->Disclosed(RetentionField);

    Surface->Ground(Extent, Tinted.MenuLower, Scaled.FieldRadius, CornerAll);
    Surface->Edge(Extent, Taken ? Partial(0xFFFFFFu, 0.22) : Tinted.Hairline,
                  1.0f, Scaled.FieldRadius, CornerAll);

    const float GlyphExtent = 14.0f * (Scaled.SearchHeight / 30.0f);
    const float GlyphLead   = Extent.MinimumX + 10.0f * (Scaled.SearchHeight / 30.0f);
    const float GlyphTop    = Extent.MinimumY + (Extent.Height() - GlyphExtent) * 0.5f;

    Surface->Stroke(SymbolSubject::MagnifierLens,
                    Spanning(GlyphLead, GlyphTop, GlyphExtent, GlyphExtent), Tinted.Faint);

    const float RunLead = GlyphLead + GlyphExtent + 8.0f * (Scaled.SearchHeight / 30.0f);
    const float FieldRun = Scaled.RunSecondary * (12.0f / 11.5f);
    const float RunTop   = Extent.MinimumY + (Extent.Height() - FieldRun) * 0.5f;

    const bool Empty = Applied.EntityRetention[0] == '\0';

    Surface->TextRunTruncated(RunLead, RunTop, Extent.MaximumX - RunLead - 8.0f,
                              Empty ? Tinted.Faint : Tinted.Primary,
                              Empty ? "Filter Entities\u2026" : Applied.EntityRetention, FieldRun);
}

void GlobalShellPanel::RecordOutlineColumn(const PlaneExtent& Extent, ShellContext& Applied,
                                             const EntityRow* Rows, std::uint32_t RowCount)
{
    // 📐 The strip sits over the whole column, beneath nothing — the pane header belongs to the page it
    //    heads, so each of the two pages draws its own and the strip is the only shared chrome.
    static const char* const Captions[2] = { "Outliner", "Context Menu" };

    const PlaneExtent Strip = Spanning(Extent.MinimumX, Extent.MinimumY,
                                       Extent.Width(), Scaled.ComponentY);

    const TabDeclaration Declared{ Captions, 2u };

    static_cast<void>(Controls.TabStrip(OutlineStrip, Strip, Declared, Applied.OutlineTab));

    const PlaneExtent Page = Spanning(Extent.MinimumX, Strip.MaximumY, Extent.Width(),
                                      Extent.MaximumY - Strip.MaximumY);

    if (Applied.OutlineTab == 0u)
        RecordOutliner(Page, Applied, Rows, RowCount);
    else
        RecordContextPage(Page, Applied, Rows, RowCount);

    // 📐 The column's own trailing hairline, `border-r border-[var(--hair)]`, over both pages.
    Surface->Ground(Spanning(Extent.MaximumX - 1.0f, Extent.MinimumY, 1.0f, Extent.Height()),
                    Tinted.Hairline, 0.0f, CornerNone);
}

void GlobalShellPanel::RecordOutliner(const PlaneExtent& Extent, ShellContext& Applied,
                                      const EntityRow* Rows, std::uint32_t RowCount)
{
    Surface->Ground(Extent, Tinted.Menu, 0.0f, CornerNone);
    Surface->Ground(Spanning(Extent.MaximumX - 1.0f, Extent.MinimumY, 1.0f, Extent.Height()),
                    Tinted.Hairline, 0.0f, CornerNone);

    const PlaneExtent Header = Spanning(Extent.MinimumX, Extent.MinimumY,
                                        Extent.Width(), Scaled.HeaderHeight);

    RecordPaneHeader(Header, SymbolSubject::GearCog, Covering(0xFFFFFFu), Tinted.EntityAccent,
                     "World Outliner", "Level_01_City");

    // 📝 The header is declared `bg-[var(--menu)]` in the outliner and `--menu-2` elsewhere, so the shared
    //    header's ground is corrected here rather than parameterised into it for one caller.
    const float Pad = Scaled.PanePad;

    const PlaneExtent Retention = Spanning(Extent.MinimumX + Pad, Header.MaximumY + Pad,
                                        Extent.Width() - Pad * 2.0f, Scaled.SearchHeight);

    RecordRetentionField(Retention, Applied);

    const PlaneExtent Footer = Spanning(Extent.MinimumX, Extent.MaximumY - Scaled.FooterHeight,
                                        Extent.Width(), Scaled.FooterHeight);

    const PlaneExtent Body = Spanning(Extent.MinimumX + Pad, Retention.MaximumY + Pad * 0.5f,
                                      Extent.Width() - Pad * 2.0f,
                                      Footer.MinimumY - Retention.MaximumY - Pad);

    Surface->Confine(Body);

    float Sweep = Body.MinimumY;

    for (std::uint32_t Ordinal = 0u; Ordinal < RowCount && Ordinal < RowCeiling; ++Ordinal)
    {
        if (!RowCurrent(Applied, Rows, RowCount, Ordinal))
            continue;

        const EntityRow&  EntryRow = Rows[Ordinal];
        const PlaneExtent Row       = Spanning(Body.MinimumX, Sweep,
                                               Body.Width(), Scaled.RowHeight);

        Sweep += Scaled.RowHeight;

        if (Surface->Excluded(Row))
            continue;

        const bool Taken  = Applied.EntityTaken == Ordinal;
        const bool Hovered = Row.Encloses(Sampled.PositionX, Sampled.PositionY);
        const bool Absent = !Applied.EntityPresent[Ordinal];
        const bool Branch = EntryRow.EnclosedCount > 0u;

        const float LeadX = Row.MinimumX + Scaled.RowLeadX
                              + static_cast<float>(EntryRow.Depth) * Scaled.RowStepX;

        // ① The disclosure cell, which takes the contact before the row does.
        const PlaneExtent Chevron = Spanning(LeadX,
                                             Row.MinimumY + (Row.Height() - Scaled.ChevronExtent) * 0.5f,
                                             Scaled.ChevronExtent, Scaled.ChevronExtent);

        const bool OnChevron = Branch && Chevron.Encloses(Sampled.PositionX, Sampled.PositionY);

        // ② The kebab at the very trailing edge, and the presence action inboard of it. Both outrank the
        //    row, and the kebab outranks the eye because it is written outboard of it.
        const PlaneExtent Kebab = Spanning(Row.MaximumX - Scaled.KebabExtent - Scaled.PanePad * 0.5f,
                                           Row.MinimumY + (Row.Height() - Scaled.KebabExtent) * 0.5f,
                                           Scaled.KebabExtent, Scaled.KebabExtent);

        const bool OnKebab = Kebab.Encloses(Sampled.PositionX, Sampled.PositionY);

        const float PresenceExtent = Scaled.GlyphExtent * (20.0f / 18.0f);
        const PlaneExtent Presence = Spanning(Kebab.MinimumX - PresenceExtent - Scaled.PanePad * 0.5f,
                                              Row.MinimumY + (Row.Height() - PresenceExtent) * 0.5f,
                                              PresenceExtent, PresenceExtent);

        const bool OnPresence = Presence.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Sampled.ContactPressed && !Ledger->AnyDisclosed())
        {
            if (OnChevron)
                Ledger->Grab(RowDisclosures[Ordinal], ControlPart::Chevron);
            else if (OnKebab)
                Ledger->Grab(RowKebabs[Ordinal], ControlPart::Body);
            else if (OnPresence)
                Ledger->Grab(RowPresences[Ordinal], ControlPart::Body);
            else if (Hovered)
                Ledger->Grab(RowContacts[Ordinal], ControlPart::Body);
        }

        if (OnChevron && Ledger->Released(RowDisclosures[Ordinal]))
            Applied.EntityExpanded[Ordinal] = !Applied.EntityExpanded[Ordinal];

        if (OnPresence && Ledger->Released(RowPresences[Ordinal]))
        {
            // 📐 The reference applies the new presence to the row AND everything it holds, walking inward.
            const bool Incoming = !Applied.EntityPresent[Ordinal];

            Applied.EntityPresent[Ordinal] = Incoming;

            for (std::uint32_t Inward = Ordinal + 1u; Inward < RowCount && Inward < RowCeiling; ++Inward)
            {
                if (Rows[Inward].Depth <= EntryRow.Depth)
                    break;

                Applied.EntityPresent[Inward] = Incoming;
            }
        }

        if (Hovered && !OnChevron && !OnPresence && !OnKebab && Ledger->Released(RowContacts[Ordinal]))
            Applied.EntityTaken = Ordinal;

        Ledger->DeclareHovered(RowContacts[Ordinal], Hovered, HoverOver);

        // ③ The row ground, then its rail. `opacity-50` for a withheld row is applied to every colour it draws.
        const float Coverage = Absent ? 0.5f : 1.0f;

        if (Taken)
            Surface->Ground(Row, Faded(Tinted.EntityTaken, Coverage), Scaled.FieldRadius, CornerAll);
        else if (Hovered)
            Surface->Ground(Row, Faded(Tinted.RowHovered, Coverage), Scaled.FieldRadius, CornerAll);

        if (Taken)
        {
            const PlaneExtent Rail = Spanning(Row.MinimumX - Scaled.RailOffsetX,
                                              Row.MinimumY + (Row.Height() - Scaled.RailY) * 0.5f,
                                              Scaled.RailX, Scaled.RailY);

            Surface->Ground(Rail, Faded(Tinted.EntityAccent, Coverage), 2.0f,
                            CornerTrailingUpper | CornerTrailingLower);
        }

        if (Branch)
        {
            // 📐 `-rotate-90` while collapsed, which is the chevron-right figure rather than a turned one.
            Surface->Stroke(Applied.EntityExpanded[Ordinal] || Applied.EntityRetention[0] != '\0'
                            ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                            Chevron, Faded(Tinted.Faint, Coverage));
        }

        const float GlyphLead = LeadX + Scaled.ChevronExtent + Scaled.PanePad;
        const PlaneExtent Glyph = Spanning(GlyphLead,
                                           Row.MinimumY + (Row.Height() - Scaled.GlyphExtent) * 0.5f,
                                           Scaled.GlyphExtent, Scaled.GlyphExtent);

        Surface->Stroke(EntityGlyph(EntryRow.Subject), Glyph,
                        Faded(EntityHue(EntryRow.Subject), Coverage));

        const float NamingRun  = Scaled.RunPrimary;
        const float NamingLead = Glyph.MaximumX + Scaled.PanePad;
        const float NamingTop  = Row.MinimumY + (Row.Height() - NamingRun) * 0.5f;

        // 📝 The count and the presence action both sit to the trailing side, so the naming run is truncated
        //    against whichever of them stands.
        float NamingCeiling = Presence.MinimumX - Scaled.PanePad;

        if (Branch)
        {
            char Counted[12] = {};
            std::snprintf(Counted, sizeof(Counted), "%u",
                          static_cast<unsigned>(EntryRow.EnclosedCount));

            const float CountRun  = Scaled.RunFine;
            const float CountLead = NamingCeiling - Surface->MeasureRun(Counted, CountRun, 0.0f);

            Surface->TextRun(CountLead, Row.MinimumY + (Row.Height() - CountRun) * 0.5f,
                             Faded(Tinted.Faint, Coverage), Counted, CountRun);

            NamingCeiling = CountLead - Scaled.PanePad;
        }

        Surface->TextRunTruncated(NamingLead, NamingTop, NamingCeiling,
                                  Faded(Taken ? Tinted.Primary : (Hovered ? Tinted.Primary : Tinted.Muted),
                                        Coverage),
                                  EntryRow.Naming, NamingRun);

        // 📐 The eye is `opacity-0` until the row is hovered, and always present once the row is withheld.
        if (Hovered || Absent)
        {
            if (OnPresence)
                Surface->Ground(Presence, Tinted.TileHovered, 3.0f, CornerAll);

            const float EyeExtent = PresenceExtent * (14.0f / 20.0f);
            const PlaneExtent Eye = Spanning(Presence.MinimumX + (PresenceExtent - EyeExtent) * 0.5f,
                                             Presence.MinimumY + (PresenceExtent - EyeExtent) * 0.5f,
                                             EyeExtent, EyeExtent);

            Surface->Stroke(Absent ? SymbolSubject::EyeClosed : SymbolSubject::EyeOpen, Eye,
                            OnPresence ? Tinted.Primary : Tinted.Faint);
        }

        // ⑤ The kebab, which raises the floating options card over the whole shell. 🔴 Its identity is
        //    spent whether or not the dots are drawn: a row that skips the call shifts every later row's
        //    hover onto the wrong identity and the whole column re-fades on the next scroll.
        if (RecordKebab(Kebab, RowKebabs[Ordinal], Hovered))
        {
            Applied.ContextRaised = Ordinal;
            Applied.ContextX  = Kebab.MinimumX;
            Applied.ContextY = Kebab.MaximumY;
            Applied.EntityTaken   = Ordinal;
        }
    }

    Surface->Release();

    // ④ The footer, `{count} entities`.
    Surface->Ground(Footer, Tinted.MenuLower, 0.0f, CornerNone);
    Surface->Ground(Spanning(Footer.MinimumX, Footer.MinimumY, Footer.Width(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    char Counted[16] = {};
    std::snprintf(Counted, sizeof(Counted), "%u", static_cast<unsigned>(RowCount));

    const float FooterRun = Scaled.RunFine;
    const float FooterTop = Footer.MinimumY + (Footer.Height() - FooterRun) * 0.5f;
    const float FooterLead = Footer.MinimumX + Scaled.HeaderPadX;

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
//    green, blue and lavender at the 500 step, which is what `.replace('400','500')` selects.
constexpr std::uint32_t ContextTintRun[6] =
{
    0xEF4444u, 0xF97316u, 0xEAB308u, 0x22C55Eu, 0x3B82F6u, 0xA855F7u
};

}   // namespace

bool GlobalShellPanel::RecordKebab(const PlaneExtent& Extent, ControlIdentity Target, bool Hovered)
{
    const bool Over = Extent.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Sampled.ContactPressed && Over && !Ledger->AnyDisclosed())
        Ledger->Grab(Target, ControlPart::Body);

    const bool Taken = Over && Ledger->Released(Target);

    Ledger->DeclareHovered(Target, Over, HoverOver);

    // 📐 `opacity-0` until the row is hovered, exactly as the eye beside it is.
    if (!Hovered && !Over)
        return Taken;

    if (Over)
        Surface->Ground(Extent, Tinted.TileHovered, 3.0f, CornerAll);

    // 📝 Three dots and not a glyph. `SymbolSubject` carries no ellipsis and is closed at its placeholder,
    //    so authoring one would renumber the whole roster and both of its pinning asserts; three medallions
    //    are the figure itself rather than a stand-in for it, and cost nothing.
    const float Centre  = Extent.MinimumX + Extent.Width() * 0.5f;
    const float Spacing = Scaled.KebabDot * 2.5f;
    const float Disc    = Extent.MinimumY + Extent.Height() * 0.5f;

    for (std::uint32_t Dot = 0u; Dot < 3u; ++Dot)
    {
        Surface->Medallion(Centre, Disc + (static_cast<float>(Dot) - 1.0f) * Spacing,
                           Scaled.KebabDot, Over ? Tinted.Primary : Tinted.Faint);
    }

    return Taken;
}

void GlobalShellPanel::RecordContextPage(const PlaneExtent& Extent, ShellContext& Applied,
                                         const EntityRow* Rows, std::uint32_t RowCount)
{
    Surface->Ground(Extent, Tinted.Menu, 0.0f, CornerNone);

    const float Pad = Scaled.PanePad;

    if (Rows == nullptr || RowCount == 0u || Applied.EntityTaken >= RowCount ||
        Applied.EntityTaken >= RowCeiling)
    {
        const float Run   = Scaled.RunSecondary;
        const char* Prose = "Select a record to view its options.";

        Surface->TextRun(Extent.MinimumX + (Extent.Width() -
                                              Surface->MeasureRun(Prose, Run, 0.0f)) * 0.5f,
                         Extent.MinimumY + (Extent.Height() - Run) * 0.5f,
                         Tinted.Faint, Prose, Run);
        return;
    }

    const std::uint32_t Ordinal   = Applied.EntityTaken;
    const EntityRow&    Current = Rows[Ordinal];
    const bool          Grouped   = Current.Subject == EntitySubject::Grouping;

    float Sweep = Extent.MinimumY + Pad;

    // ① The heading, which names which of the two option sets is standing.
    {
        const float Run = Scaled.RunSecondary;

        Surface->TextRun(Extent.MinimumX + Pad * 1.5f, Sweep + (Scaled.ContextRow - Run) * 0.5f,
                         Tinted.Muted, Grouped ? "Folder Options" : "Object Options", Run, 0.0f, true);

        Sweep += Scaled.ContextRow;

        Surface->Ground(Spanning(Extent.MinimumX, Sweep, Extent.Width(), 1.0f),
                        Tinted.Hairline, 0.0f, CornerNone);

        Sweep += Pad;
    }

    // ② `Set Color`, offered for a folder alone, exactly as the reference gates it.
    if (Grouped)
    {
        const float HeadRun = Scaled.RunFine;

        Surface->TextRunCapitalised(Extent.MinimumX + Pad * 1.5f, Sweep, Tinted.Faint, "Set Color",
                                    HeadRun, 0.09f, false);

        Sweep += HeadRun * RunLeading + Pad;

        float Lead = Extent.MinimumX + Pad * 1.5f;

        for (std::uint32_t Tint = 0u; Tint < 7u; ++Tint)
        {
            const PlaneExtent Disc = Spanning(Lead, Sweep, Scaled.ContextSwatch, Scaled.ContextSwatch);
            const bool        Over = Disc.Encloses(Sampled.PositionX, Sampled.PositionY);

            if (Sampled.ContactPressed && Over && !Ledger->AnyDisclosed())
                Ledger->Grab(ContextTints[Tint], ControlPart::Body);

            static_cast<void>(Over && Ledger->Released(ContextTints[Tint]));

            Ledger->DeclareHovered(ContextTints[Tint], Over, HoverOver);

            // 📐 `hover:scale-110`, which is a tenth added about the disc's own centre.
            const float Grown  = Over ? Scaled.ContextSwatch * 0.10f : 0.0f;
            const PlaneExtent Drawn = Spanning(Disc.MinimumX - Grown * 0.5f, Disc.MinimumY - Grown * 0.5f,
                                               Scaled.ContextSwatch + Grown, Scaled.ContextSwatch + Grown);

            if (Tint < 6u)
            {
                Surface->Ground(Drawn, Covering(ContextTintRun[Tint]), Drawn.Width() * 0.5f, CornerAll);
            }
            else
            {
                // 📐 The seventh disc clears the hue rather than setting one, and carries a cross.
                Surface->Edge(Drawn, Tinted.HairlineFirm, 1.0f, Drawn.Width() * 0.5f, CornerAll);

                const float Mark = Scaled.ContextSwatch * 0.5f;

                Surface->Stroke(SymbolSubject::PlusCross,
                                Spanning(Drawn.MinimumX + (Drawn.Width() - Mark) * 0.5f,
                                         Drawn.MinimumY + (Drawn.Height() - Mark) * 0.5f,
                                         Mark, Mark),
                                Tinted.Muted, 0.7853982f);
            }

            Lead += Scaled.ContextSwatch + Pad * 0.8f;
        }

        Sweep += Scaled.ContextSwatch + Pad * 1.5f;
    }

    // ③ Rename and Delete, the two the reference always offers.
    static_cast<void>(RecordActionRow(Spanning(Extent.MinimumX + Pad, Sweep,
                                               Extent.Width() - Pad * 2.0f, Scaled.ContextRow),
                                      ContextActions[0], SymbolSubject::ColumnArrangement,
                                      "Rename", nullptr, Tinted.Primary, Tinted.Muted));

    Sweep += Scaled.ContextRow;

    if (RecordActionRow(Spanning(Extent.MinimumX + Pad, Sweep,
                                 Extent.Width() - Pad * 2.0f, Scaled.ContextRow),
                        ContextActions[1], SymbolSubject::TrashBin, "Delete", nullptr,
                        Covering(0xF87171u), Covering(0xF87171u)))
    {
        // 📝 The row is presented rather than removed, because the outliner's run is the host's and is
        //    borrowed for the tick. Withholding it is the strongest answer a panel may give here.
        Applied.EntityPresent[Ordinal] = false;
    }
}

void GlobalShellPanel::RecordContextOverlay(const PlaneExtent& Extent, ShellContext& Applied,
                                            const EntityRow* Rows, std::uint32_t RowCount)
{
    if (Applied.ContextRaised >= RowCeiling)
        return;

    if (Rows == nullptr || Applied.ContextRaised >= RowCount)
    {
        Applied.ContextRaised = ShellContext::EntityCeiling;
        return;
    }

    // 📐 `fixed inset-0 z-[100]`, which dismisses the card wherever it is tapped. Recorded first so the
    //    card itself, written after it, stands over it.
    if (Sampled.ContactPressed && !Ledger->AnyDisclosed())
        Ledger->Grab(ContextVeil, ControlPart::Body);

    const bool Dismissed = Ledger->Released(ContextVeil);

    const EntityRow& Current = Rows[Applied.ContextRaised];
    const bool       Grouped   = Current.Subject == EntitySubject::Grouping;
    const float      Pad       = Scaled.ContextPad;

    // 📐 The card is measured from what it carries: a heading, an optional colour strip, and two rows.
    const float HeadHeight = Scaled.ContextRow + 1.0f + Pad;
    const float TintY = Grouped ? (Scaled.RunFine * RunLeading + Pad + Scaled.ContextSwatch + Pad)
                                     : 0.0f;
    const float WholeHeight = Pad + HeadHeight + TintY + Scaled.ContextRow * 2.0f + Pad;

    // 📐 `Math.min(contextMenu.x, window.innerWidth - 200)`, and the same rule across.
    const float Clamped = Extent.MaximumX - Scaled.ContextClamp;
    const float X   = Held(Applied.ContextX, Extent.MinimumX,
                               (Clamped > Extent.MinimumX) ? Clamped : Extent.MinimumX);
    const float Y  = Held(Applied.ContextY, Extent.MinimumY,
                               (Extent.MaximumY - WholeHeight > Extent.MinimumY)
                                   ? Extent.MaximumY - WholeHeight : Extent.MinimumY);

    // 📐 `initial={{opacity:0, scale:.95}} animate={{opacity:1, scale:1}} transition={{duration:0.1}}`.
    // 🔴 The veil's own hover traverse carries the arrival, declared standing for as long as the card is
    //    raised. Read without being declared it reports zero every tick and the card records invisibly —
    //    a fade that never departs is indistinguishable from a card that was never recorded.
    Ledger->DeclareHovered(ContextVeil, true, ContextArriveOver);

    const float Incoming = Ledger->HoveredFraction(ContextVeil);
    const float Coverage = Held(Incoming, 0.0f, 1.0f);
    const float Grown    = Between(0.95f, 1.0f, Coverage);

    const float CardX  = Scaled.ContextX * Grown;
    const float CardHeight = WholeHeight * Grown;

    const PlaneExtent Card = Spanning(X + (Scaled.ContextX - CardX) * 0.5f,
                                      Y + (WholeHeight - CardHeight) * 0.5f,
                                      CardX, CardHeight);

    Surface->Ground(Card, Faded(Tinted.Menu, Coverage), Scaled.CardRadius, CornerAll);
    Surface->Edge(Card, Faded(Tinted.HairlineFirm, Coverage), 1.0f, Scaled.CardRadius, CornerAll);

    Surface->Confine(Card);
    RecordContextPage(Card, Applied, Rows, RowCount);
    Surface->Release();

    // 📝 Dismissed last, so the card the artist tapped is the one they saw this tick.
    if (Dismissed && !Card.Encloses(Sampled.PositionX, Sampled.PositionY))
        Applied.ContextRaised = ShellContext::EntityCeiling;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE METADATA PANE
//------------------------------------------------------------------------------------------------------------------------

float GlobalShellPanel::RecordStatRow(const PlaneExtent& Extent, const char* Caption, const char* Reading)
{
    const float Run    = Scaled.RunSecondary;
    const float Applied = Extent.MinimumY + (Extent.Height() - Run) * 0.5f;

    Surface->TextRun(Extent.MinimumX, Applied, Tinted.Muted, Caption, Run);

    const float ReadingX = Extent.MaximumX - Surface->MeasureRun(Reading, Run, 0.0f);

    Surface->TextRun(ReadingX, Applied, Tinted.Primary, Reading, Run);

    // 📐 `border-b border-[var(--hair)]`, one side and not four, so a ground and not an edge.
    Surface->Ground(Spanning(Extent.MinimumX, Extent.MaximumY - 1.0f, Extent.Width(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    return Extent.Height();
}

bool GlobalShellPanel::RecordActionRow(const PlaneExtent& Extent, ControlIdentity Target,
                                       SymbolSubject Glyph, const char* Caption, const char* Chord,
                                       ThemeToken Colour, ThemeToken GlyphColour)
{
    const bool Hovered = Extent.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Sampled.ContactPressed && Hovered && !Ledger->AnyDisclosed())
        Ledger->Grab(Target, ControlPart::Body);

    const bool Taken = Hovered && Ledger->Released(Target);

    Ledger->DeclareHovered(Target, Hovered, HoverOver);

    // 📐 `hover:bg-[var(--tile-hi)]`, save for Delete which rouses to its own alert wash instead.
    if (Hovered)
        Surface->Ground(Extent, Faded(Colour, 0.12f), Scaled.LayerRadius, CornerAll);

    const float GlyphCell  = Scaled.ActionGlyph;
    const float GlyphY  = Extent.MinimumY + (Extent.Height() - GlyphCell) * 0.5f;
    const PlaneExtent Cell = Spanning(Extent.MinimumX + Scaled.PanePad, GlyphY, GlyphCell, GlyphCell);

    // 📝 The reference draws the glyph at `--muted` and the alerting action at its own hue, so both colours
    //    are handed in rather than one derived from the other — Delete is where the two agree.
    Surface->Stroke(Glyph, Cell, GlyphColour);

    const float Run     = Scaled.RunSecondary;
    const float RunY = Extent.MinimumY + (Extent.Height() - Run) * 0.5f;
    const float RunLead = Cell.MaximumX + Scaled.PanePad;

    float RunCeiling = Extent.MaximumX - Scaled.PanePad;

    if (Chord != nullptr && Chord[0] != '\0')
    {
        // 📐 `ml-auto text-[9.5px] text-[var(--faint)] font-mono`, hard against the trailing edge.
        const float ChordRun  = Scaled.RunFiner;
        const float ChordLead = RunCeiling - Surface->MeasureRun(Chord, ChordRun, 0.0f);

        Surface->TextRun(ChordLead, Extent.MinimumY + (Extent.Height() - ChordRun) * 0.5f,
                         Tinted.Faint, Chord, ChordRun);

        RunCeiling = ChordLead - Scaled.PanePad;
    }

    Surface->TextRunTruncated(RunLead, RunY, RunCeiling, Colour, Caption, Run);

    return Taken;
}

void GlobalShellPanel::RecordMetadata(const PlaneExtent& Extent, ShellContext& Applied,
                                      const EntityRow* Rows, std::uint32_t RowCount)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    const float Pad = Scaled.PanePad;

    // ① Nothing taken. 📐 The reference centres a 48 px crosshair tile, a heading and one wrapped line.
    if (Rows == nullptr || RowCount == 0u || Applied.EntityTaken >= RowCount ||
        Applied.EntityTaken >= RowCeiling)
    {
        const float TileExtent = Scaled.HeroCrest * (48.0f / 34.0f);
        const float Run        = Scaled.RunPrimary;
        const float Fine       = Scaled.RunSecondary;
        const float Stack      = TileExtent + Pad * 2.0f + Run * RunLeading + Fine * RunLeading;
        float       Sweep     = Extent.MinimumY + (Extent.Height() - Stack) * 0.5f;

        const PlaneExtent Tile = Spanning(Extent.MinimumX + (Extent.Width() - TileExtent) * 0.5f,
                                          Sweep, TileExtent, TileExtent);

        Surface->Ground(Tile, Tinted.Tile, Scaled.CardRadius, CornerAll);
        Surface->Edge(Tile, Tinted.Hairline, 1.0f, Scaled.CardRadius, CornerAll);

        const float Figure = TileExtent * 0.5f;

        Surface->Stroke(SymbolSubject::CrosshairCentre,
                        Spanning(Tile.MinimumX + (TileExtent - Figure) * 0.5f,
                                 Tile.MinimumY + (TileExtent - Figure) * 0.5f, Figure, Figure),
                        Tinted.Muted);

        Sweep = Tile.MaximumY + Pad * 2.0f;

        const char* Heading = "Properties Overview";
        const char* Prose   = "Select a record in the directory to view its details.";

        Surface->TextRun(Extent.MinimumX + (Extent.Width() -
                                              Surface->MeasureRun(Heading, Run, 0.0f)) * 0.5f,
                         Sweep, Tinted.Primary, Heading, Run, 0.0f, true);

        Sweep += Run * RunLeading;

        Surface->TextRun(Extent.MinimumX + (Extent.Width() -
                                              Surface->MeasureRun(Prose, Fine, 0.0f)) * 0.5f,
                         Sweep, Tinted.Faint, Prose, Fine);
        return;
    }

    const std::uint32_t Ordinal   = Applied.EntityTaken;
    const EntityRow&    Current = Rows[Ordinal];
    const EntityProfile& Profiled = Applied.EntityProfiles[Ordinal];
    const ThemeToken   Hue       = EntityHue(Current.Subject);
    const bool          Absent    = !Applied.EntityPresent[Ordinal];

    // 📐 The reference mints `g_NN`; the ordinal is that identity here, so the two read side by side.
    char Token[12] = {};
    std::snprintf(Token, sizeof(Token), "g_%02u", static_cast<unsigned>(Ordinal + 1u));

    const PlaneExtent Footer = Spanning(Extent.MinimumX, Extent.MaximumY - Scaled.FooterHeight,
                                        Extent.Width(), Scaled.FooterHeight);

    const PlaneExtent Body = Spanning(Extent.MinimumX + Pad * 1.5f, Extent.MinimumY + Pad * 1.5f,
                                      Extent.Width() - Pad * 3.0f,
                                      Footer.MinimumY - Extent.MinimumY - Pad * 3.0f);

    Surface->Confine(Body);

    float Sweep = Body.MinimumY;

    // ② The hero tile — a black crest, the naming, and the classification in the subject's own hue.
    {
        const float HeroHeight = Scaled.HeroCrest + Scaled.HeroPad * 2.0f;
        const PlaneExtent Hero = Spanning(Body.MinimumX, Sweep, Body.Width(), HeroHeight);

        Surface->Ground(Hero, Tinted.Tile, Scaled.CardRadius, CornerAll);
        Surface->Edge(Hero, Tinted.Hairline, 1.0f, Scaled.CardRadius, CornerAll);

        const PlaneExtent Crest = Spanning(Hero.MinimumX + Scaled.HeroPad, Hero.MinimumY + Scaled.HeroPad,
                                           Scaled.HeroCrest, Scaled.HeroCrest);

        Surface->Ground(Crest, Covering(0x000000u), Scaled.LayerRadius, CornerAll);

        const float Figure = Scaled.HeroCrest * (24.0f / 34.0f);

        Surface->Stroke(EntityGlyph(Current.Subject),
                        Spanning(Crest.MinimumX + (Scaled.HeroCrest - Figure) * 0.5f,
                                 Crest.MinimumY + (Scaled.HeroCrest - Figure) * 0.5f, Figure, Figure),
                        Covering(0xFFFFFFu));

        const float NamingRun = Scaled.RunPrimary + 0.5f;
        const float ClassRun  = Scaled.RunSmall;
        const float PairSpan  = NamingRun * RunLeading + ClassRun * RunLeading;
        const float PairY  = Hero.MinimumY + (HeroHeight - PairSpan) * 0.5f;
        const float RunLead   = Crest.MaximumX + Scaled.HeroPad;

        Surface->TextRunTruncated(RunLead, PairY, Hero.MaximumX - Scaled.HeroPad,
                                  Tinted.Primary, Current.Naming, NamingRun, true);
        Surface->TextRun(RunLead, PairY + NamingRun * RunLeading, Hue,
                         EntityText(Current.Subject), ClassRun, 0.0f, true);

        Sweep = Hero.MaximumY + Pad * 1.5f;
    }

    // ③ The stat rows, exactly the run `getStats` assembles and in its own order.
    {
        char Reading[64] = {};

        const auto Stated = [&](const char* Caption, const char* Value)
        {
            Sweep += RecordStatRow(Spanning(Body.MinimumX, Sweep, Body.Width(), Scaled.StatY),
                                    Caption, Value);
        };

        Stated("Token", Token);
        Stated("Visible", Absent ? "hidden" : "shown");

        if (Current.EnclosedCount > 0u)
        {
            std::snprintf(Reading, sizeof(Reading), "%u records",
                          static_cast<unsigned>(Current.EnclosedCount));
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
            const PlaneExtent Row = Spanning(Body.MinimumX, Sweep, Body.Width(), Scaled.StatY);
            const float       Run = Scaled.RunSecondary;
            const float       Top = Row.MinimumY + (Row.Height() - Run) * 0.5f;

            Surface->TextRun(Row.MinimumX, Top, Tinted.Muted, "Albedo", Run);

            const PlaneExtent Disc =
                Spanning(Row.MaximumX - Scaled.SwatchExtent,
                         Row.MinimumY + (Row.Height() - Scaled.SwatchExtent) * 0.5f,
                         Scaled.SwatchExtent, Scaled.SwatchExtent);

            const std::uint32_t Packed = (Profiled.Albedo[0] << 16) | (Profiled.Albedo[1] << 8)
                                       | Profiled.Albedo[2];

            Surface->Ground(Disc, Covering(Packed), Scaled.SwatchExtent * 0.5f, CornerAll);
            Surface->Edge(Disc, Tinted.HairlineFirm, 1.0f, Scaled.SwatchExtent * 0.5f, CornerAll);

            std::snprintf(Reading, sizeof(Reading), "%u, %u, %u",
                          static_cast<unsigned>(Profiled.Albedo[0]),
                          static_cast<unsigned>(Profiled.Albedo[1]),
                          static_cast<unsigned>(Profiled.Albedo[2]));

            Surface->TextRun(Disc.MinimumX - Pad - Surface->MeasureRun(Reading, Run, 0.0f), Top,
                             Tinted.Primary, Reading, Run);

            Surface->Ground(Spanning(Row.MinimumX, Row.MaximumY - 1.0f, Row.Width(), 1.0f),
                            Tinted.Hairline, 0.0f, CornerNone);

            Sweep += Row.Height();
        }
    }

    // ⑤ The call that carries the artist to slide two — the pointer twin of Tab, and it says so.
    {
        Sweep += Pad;

        const PlaneExtent Call = Spanning(Body.MinimumX, Sweep, Body.Width(), Scaled.AdvanceY);
        const bool        Over = Call.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Sampled.ContactPressed && Over && !Ledger->AnyDisclosed())
            Ledger->Grab(AdvanceCall, ControlPart::Body);

        if (Over && Ledger->Released(AdvanceCall))
        {
            Applied.InspectorShown = true;

            if (Motion != nullptr)
            {
                EasedInterpolant& Travelling = Motion->Eased(CarouselSlide);

                Travelling.Depart(Travelling.Current(), 1.0, CarouselTravelOver, 0.0, EaseCurve::Carousel);
            }
        }

        Ledger->DeclareHovered(AdvanceCall, Over, HoverOver);

        // 📐 `bg-[rgba(91,140,255,.13)] border-[var(--accent)]`, rousing to `.2` of the same accent.
        Surface->Ground(Call, Over ? Faded(Tinted.Accent, 0.20f) : Tinted.AccentSoft,
                        Scaled.LayerRadius, CornerAll);
        Surface->Edge(Call, Tinted.Accent, 1.0f, Scaled.LayerRadius, CornerAll);

        const char* Caption  = "Properties & History";
        const char* PillRun  = "Tab";
        const float Run      = Scaled.RunSecondary;
        const float Fine     = Scaled.RunFiner;
        const float Figure   = Scaled.ActionGlyph;
        const float PillSpan = Surface->MeasureRun(PillRun, Fine, 0.0f) + Scaled.PillPadX * 2.0f;
        const float Whole    = Figure + Pad + Surface->MeasureRun(Caption, Run, 0.0f) + Pad + PillSpan;

        float Lead = Call.MinimumX + (Call.Width() - Whole) * 0.5f;

        Surface->Stroke(SymbolSubject::GearCog,
                        Spanning(Lead, Call.MinimumY + (Call.Height() - Figure) * 0.5f,
                                 Figure, Figure),
                        Tinted.Primary);

        Lead += Figure + Pad;

        Surface->TextRun(Lead, Call.MinimumY + (Call.Height() - Run) * 0.5f,
                         Tinted.Primary, Caption, Run, 0.0f, true);

        Lead += Surface->MeasureRun(Caption, Run, 0.0f) + Pad;

        const float PillY = Fine + 6.0f;
        const PlaneExtent Pill = Spanning(Lead, Call.MinimumY + (Call.Height() - PillY) * 0.5f,
                                          PillSpan, PillY);

        Surface->Ground(Pill, Tinted.MenuLower, PillY * 0.5f, CornerAll);
        Surface->TextRun(Pill.MinimumX + Scaled.PillPadX,
                         Pill.MinimumY + (PillY - Fine) * 0.5f, Tinted.Muted, PillRun, Fine);

        Sweep = Call.MaximumY + Pad * 2.0f;
    }

    // ⑥ The five inline actions beneath their own heading.
    {
        const float HeadRun = Scaled.RunFiner;

        Surface->TextRunCapitalised(Body.MinimumX + Pad * 0.5f, Sweep, Tinted.Faint, "Actions",
                                    HeadRun, 0.09f, true);

        Sweep += HeadRun * RunLeading + Pad * 0.5f;

        // 📝 Stand-in figures where the reference reaches for a lucide glyph this build has not authored
        //    yet — `type` and `copy` in particular. The four exact ones are plus, eye, eye-off and trash-2.
        const PlaneExtent NewRecord = Spanning(Body.MinimumX, Sweep, Body.Width(), Scaled.ActionY);

        static_cast<void>(RecordActionRow(NewRecord, MetadataActions[0], SymbolSubject::PlusCross,
                                          "New record", nullptr, Tinted.Primary, Tinted.Muted));
        Sweep += Scaled.ActionY;

        Surface->Ground(Spanning(Body.MinimumX + Pad, Sweep + Pad * 0.5f,
                                 Body.Width() - Pad * 2.0f, 1.0f),
                        Tinted.Hairline, 0.0f, CornerNone);
        Sweep += Pad;

        static_cast<void>(RecordActionRow(Spanning(Body.MinimumX, Sweep, Body.Width(),
                                                   Scaled.ActionY),
                                          MetadataActions[1], SymbolSubject::ColumnArrangement,
                                          "Rename", "F2", Tinted.Primary, Tinted.Muted));
        Sweep += Scaled.ActionY;

        static_cast<void>(RecordActionRow(Spanning(Body.MinimumX, Sweep, Body.Width(),
                                                   Scaled.ActionY),
                                          MetadataActions[2], SymbolSubject::LatticeArrangement,
                                          "Duplicate", "Ctrl D", Tinted.Primary, Tinted.Muted));
        Sweep += Scaled.ActionY;

        // 📐 The row states the action rather than the condition: a shown record offers Hide, and the eye
        //    it carries is the one the reference draws for the condition the record is IN.
        if (RecordActionRow(Spanning(Body.MinimumX, Sweep, Body.Width(), Scaled.ActionY),
                            MetadataActions[3],
                            Absent ? SymbolSubject::EyeClosed : SymbolSubject::EyeOpen,
                            Absent ? "Show" : "Hide", "H", Tinted.Primary, Tinted.Muted))
        {
            const bool Incoming = Absent;

            Applied.EntityPresent[Ordinal] = Incoming;

            for (std::uint32_t Inward = Ordinal + 1u; Inward < RowCount && Inward < RowCeiling; ++Inward)
            {
                if (Rows[Inward].Depth <= Current.Depth)
                    break;

                Applied.EntityPresent[Inward] = Incoming;
            }
        }

        Sweep += Scaled.ActionY;

        static_cast<void>(RecordActionRow(Spanning(Body.MinimumX, Sweep, Body.Width(),
                                                   Scaled.ActionY),
                                          MetadataActions[4], SymbolSubject::TrashBin,
                                          "Delete", "Del", Covering(0xFF6B6Bu), Covering(0xFF6B6Bu)));
    }

    Surface->Release();

    // ⑦ The footer — the subject's hue as a chip, its naming, and the token hard against the trailing edge.
    Surface->Ground(Footer, Tinted.MenuLower, 0.0f, CornerNone);
    Surface->Ground(Spanning(Footer.MinimumX, Footer.MinimumY, Footer.Width(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    const float FooterRun = Scaled.RunFine;
    const float FooterTop = Footer.MinimumY + (Footer.Height() - FooterRun) * 0.5f;
    const float ChipY  = Footer.MinimumY + (Footer.Height() - Scaled.ChipExtent) * 0.5f;

    Surface->Ground(Spanning(Footer.MinimumX + Scaled.HeaderPadX, ChipY,
                             Scaled.ChipExtent, Scaled.ChipExtent), Hue, 2.0f, CornerAll);

    Surface->TextRun(Footer.MinimumX + Scaled.HeaderPadX + Scaled.ChipExtent + Pad, FooterTop,
                     Tinted.Muted, EntityText(Current.Subject), FooterRun);

    Surface->TextRun(Footer.MaximumX - Scaled.HeaderPadX -
                     Surface->MeasureRun(Token, FooterRun, 0.0f), FooterTop,
                     Tinted.Muted, Token, FooterRun);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE COMPONENTS
//------------------------------------------------------------------------------------------------------------------------

void GlobalShellPanel::RecordPropertyCards(const PlaneExtent& Extent, ShellContext& Applied,
                                           const EntityRow* Rows, std::uint32_t RowCount)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    if (Applied.EntityTaken >= RowCount || Applied.EntityTaken >= RowCeiling)
    {
        const float ProseRun = Scaled.RunSecondary;
        const char* Prose    = "Select a record to inspect its properties.";
        const float ProseLead = Extent.MinimumX
                              + (Extent.Width() - Surface->MeasureRun(Prose, ProseRun, 0.0f)) * 0.5f;

        Surface->TextRun(ProseLead, Extent.MinimumY + Scaled.HeaderHeight, Tinted.Faint, Prose, ProseRun);
        return;
    }

    const EntityRow& Current = Rows[Applied.EntityTaken];

    const float Pad    = Scaled.PanePad;
    float       Sweep = Extent.MinimumY + Pad;

    Surface->Confine(Extent);

    // 📐 One card per component, each folding over 200 ms. `CardOrdinal` is the fold identity the card
    //    claims; the reference keys its `collapsedCards` record by the card's own title, and the ordinal is
    //    that key here so the artist's fold survives a change of subject the way the reference's does.
    std::uint32_t CardOrdinal = 0u;

    const auto RecordCard = [&](const char* Caption, const char* const* Rows2, std::uint32_t RowCount2)
    {
        if (CardOrdinal >= CardCeiling)
            return;

        const std::uint32_t Target = CardOrdinal++;

        // 🔴 The fold is a fraction and not a flag, so the body's extent is what animates. Folded to a
        //    flag, the card would vanish between two frames and the 200 ms the reference states would be
        //    visible nowhere.
        const bool  Folded   = Applied.CardFolded[Target];
        const float Current = Controls.OutlineExpansion(CardFolds[Target], !Folded, true);

        const float BodyHeight = (static_cast<float>(RowCount2) * Scaled.RowHeight + Pad * 2.0f) * Current;
        const PlaneExtent Card = Spanning(Extent.MinimumX + Pad, Sweep,
                                          Extent.Width() - Pad * 2.0f,
                                          Scaled.ComponentY + BodyHeight);

        Surface->Ground(Card, Covering(0x0A0A0Bu), Scaled.CardRadius, CornerAll);
        Surface->Edge(Card, Tinted.Hairline, 1.0f, Scaled.CardRadius, CornerAll);

        const PlaneExtent CardHeader = Spanning(Card.MinimumX, Card.MinimumY,
                                                Card.Width(), Scaled.ComponentY);

        Surface->Ground(CardHeader, Tinted.MenuLower, Scaled.CardRadius,
                        CornerLeadingUpper | CornerTrailingUpper);

        const bool OnHeader = CardHeader.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Sampled.ContactPressed && OnHeader && !Ledger->AnyDisclosed())
            Ledger->Grab(CardFolds[Target], ControlPart::Chevron);

        if (OnHeader && Ledger->Released(CardFolds[Target]))
            Applied.CardFolded[Target] = !Applied.CardFolded[Target];

        // 📐 `border-b border-transparent` while folded, and the hairline only once disclosed.
        if (Current > 0.0f)
        {
            Surface->Ground(Spanning(CardHeader.MinimumX, CardHeader.MaximumY - 1.0f,
                                     CardHeader.Width(), 1.0f),
                            Faded(Tinted.Hairline, Current), 0.0f, CornerNone);
        }

        // 📐 The chevron turns `-rotate-90` while folded; the two figures are the turn.
        const float Mark = Scaled.ActionGlyph;

        Surface->Stroke(Folded ? SymbolSubject::ChevronRight : SymbolSubject::ChevronDown,
                        Spanning(CardHeader.MinimumX + Scaled.HeaderPadX * 0.6f,
                                 CardHeader.MinimumY + (CardHeader.Height() - Mark) * 0.5f,
                                 Mark, Mark),
                        Tinted.Faint);

        const float CaptionRun = Scaled.RunSmall;

        // 📐 `uppercase tracking-wide` at 10.5 px.
        Surface->TextRunCapitalised(CardHeader.MinimumX + Scaled.HeaderPadX * 0.6f + Mark + Pad,
                                    CardHeader.MinimumY + (CardHeader.Height() - CaptionRun) * 0.5f,
                                    OnHeader ? Tinted.Primary : Tinted.Muted, Caption, CaptionRun,
                                    0.025f, true);

        // 📐 `ml-auto text-[9.5px]`, the count of fields the card holds.
        char Tallied[8] = {};
        std::snprintf(Tallied, sizeof(Tallied), "%u", static_cast<unsigned>(RowCount2));

        const float TallyRun = Scaled.RunFiner;

        Surface->TextRun(CardHeader.MaximumX - Scaled.HeaderPadX
                         - Surface->MeasureRun(Tallied, TallyRun, 0.0f),
                         CardHeader.MinimumY + (CardHeader.Height() - TallyRun) * 0.5f,
                         Tinted.Faint, Tallied, TallyRun);

        // 🔴 The body is confined to whatever the fold has opened, so a row half way through the travel
        //    is clipped rather than drawn over the card beneath it.
        if (Current > 0.0f)
        {
            const PlaneExtent Opened = Spanning(Card.MinimumX, CardHeader.MaximumY,
                                                Card.Width(), BodyHeight);

            Surface->Confine(Opened);

            float RowCursor = CardHeader.MaximumY + Pad;

            for (std::uint32_t Ordinal = 0u; Ordinal < RowCount2; ++Ordinal)
            {
                const float LabelRun = Scaled.RunSecondary;
                const float LabelTop = RowCursor + (Scaled.RowHeight - LabelRun) * 0.5f;

                Surface->TextRun(Card.MinimumX + Pad * 1.5f, LabelTop, Tinted.Muted,
                                 Rows2[Ordinal], LabelRun);

                // 📝 The value cell, `--value-bg` at `--value-radius`, presented but not yet editable —
                //    the reference's own handlers are `onChange={()=>{}}` for every component field.
                const float CellX = Card.Width() * 0.45f;
                const PlaneExtent Cell =
                    Spanning(Card.MaximumX - Pad * 1.5f - CellX,
                             RowCursor + (Scaled.RowHeight - Scaled.SearchHeight * 0.8f) * 0.5f,
                             CellX, Scaled.SearchHeight * 0.8f);

                Surface->Ground(Cell, Covering(0x232326u), Scaled.FieldRadius, CornerAll);

                RowCursor += Scaled.RowHeight;
            }

            Surface->Release();
        }

        Sweep = Card.MaximumY + Pad * 0.85f;
    };

    // 📝 The environment slider card — the same fold as `RecordCard`, but its rows are live magnitude
    //    rows that write the configuration, and a drag that ENDS with a changed value raises the
    //    host's revision demand ONCE. The demand is what makes history record on drag start/end rather
    //    than on every tick the thumb moved.
    const auto RecordEnvironmentCard = [&](const char* Caption,
                                           const char* const* SliderCaptions,
                                           const char* const* UnitGlyphs,
                                           const double* Minimums, const double* Maximums,
                                           double* Values, std::uint32_t SliderCount)
    {
        if (CardOrdinal >= CardCeiling)
            return;

        const std::uint32_t Target = CardOrdinal++;
        const bool  Folded   = Applied.CardFolded[Target];
        const float Current  = Controls.OutlineExpansion(CardFolds[Target], !Folded, true);

        const float BodyHeight = (static_cast<float>(SliderCount) * Scaled.RowHeight + Pad * 2.0f) * Current;
        const PlaneExtent Card = Spanning(Extent.MinimumX + Pad, Sweep,
                                          Extent.Width() - Pad * 2.0f,
                                          Scaled.ComponentY + BodyHeight);

        Surface->Ground(Card, Covering(0x0A0A0Bu), Scaled.CardRadius, CornerAll);
        Surface->Edge(Card, Tinted.Hairline, 1.0f, Scaled.CardRadius, CornerAll);

        const PlaneExtent CardHeader = Spanning(Card.MinimumX, Card.MinimumY,
                                                Card.Width(), Scaled.ComponentY);

        Surface->Ground(CardHeader, Tinted.MenuLower, Scaled.CardRadius,
                        CornerLeadingUpper | CornerTrailingUpper);

        const bool OnHeader = CardHeader.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Sampled.ContactPressed && OnHeader && !Ledger->AnyDisclosed())
            Ledger->Grab(CardFolds[Target], ControlPart::Chevron);

        if (OnHeader && Ledger->Released(CardFolds[Target]))
            Applied.CardFolded[Target] = !Applied.CardFolded[Target];

        if (Current > 0.0f)
            Surface->Ground(Spanning(CardHeader.MinimumX, CardHeader.MaximumY - 1.0f,
                                     CardHeader.Width(), 1.0f),
                            Faded(Tinted.Hairline, Current), 0.0f, CornerNone);

        const float Mark = Scaled.ActionGlyph;

        Surface->Stroke(Folded ? SymbolSubject::ChevronRight : SymbolSubject::ChevronDown,
                        Spanning(CardHeader.MinimumX + Scaled.HeaderPadX * 0.6f,
                                 CardHeader.MinimumY + (CardHeader.Height() - Mark) * 0.5f,
                                 Mark, Mark),
                        Tinted.Faint);

        const float CaptionRun = Scaled.RunSmall;

        Surface->TextRunCapitalised(CardHeader.MinimumX + Scaled.HeaderPadX * 0.6f + Mark + Pad,
                                    CardHeader.MinimumY + (CardHeader.Height() - CaptionRun) * 0.5f,
                                    OnHeader ? Tinted.Primary : Tinted.Muted, Caption, CaptionRun,
                                    0.025f, true);

        char Tallied[8] = {};
        std::snprintf(Tallied, sizeof(Tallied), "%u", static_cast<unsigned>(SliderCount));

        const float TallyRun = Scaled.RunFiner;

        Surface->TextRun(CardHeader.MaximumX - Scaled.HeaderPadX
                         - Surface->MeasureRun(Tallied, TallyRun, 0.0f),
                         CardHeader.MinimumY + (CardHeader.Height() - TallyRun) * 0.5f,
                         Tinted.Faint, Tallied, TallyRun);

        if (Current > 0.0f)
        {
            const PlaneExtent Opened = Spanning(Card.MinimumX, CardHeader.MaximumY,
                                                Card.Width(), BodyHeight);

            Surface->Confine(Opened);

            float RowCursor = CardHeader.MaximumY + Pad;

            for (std::uint32_t SliderOrdinal = 0u; SliderOrdinal < SliderCount; ++SliderOrdinal)
            {
                const PlaneExtent Row = Spanning(Card.MinimumX + Pad * 1.5f, RowCursor,
                                                 Card.Width() - Pad * 3.0f, Scaled.RowHeight);

                MagnitudeDeclaration Declared;
                Declared.Caption     = SliderCaptions[SliderOrdinal];
                Declared.UnitGlyph   = UnitGlyphs[SliderOrdinal];
                Declared.Minimum     = Minimums[SliderOrdinal];
                Declared.Maximum     = Maximums[SliderOrdinal];

                double& Coordinate   = Values[SliderOrdinal];

                static_cast<void>(EnvironmentControls.MagnitudeRow(EnvironmentSliders[SliderOrdinal],
                                                                   Row, Declared, Coordinate, true));

                // 🔴 The drag arm: latched the first tick the slider holds the contact, with the value
                //    at that moment — the "start" the history entry describes. Released with a changed
                //    value, one demand is raised; neither fires on the intermediate ticks.
                if (Ledger->Holding(EnvironmentSliders[SliderOrdinal]) && !EnvironmentArmed[SliderOrdinal])
                {
                    EnvironmentArmed[SliderOrdinal] = true;
                    EnvironmentFrom[SliderOrdinal]  = Coordinate;
                }

                if (Ledger->Released(EnvironmentSliders[SliderOrdinal]))
                {
                    if (EnvironmentArmed[SliderOrdinal])
                    {

                        if (std::abs(Coordinate - EnvironmentFrom[SliderOrdinal]) > 0.0005)
                        {
                            Applied.RevisionDemandSlot.Standing  = true;
                            Applied.RevisionDemandSlot.Against   = Applied.EntityTaken;
                            std::snprintf(Applied.RevisionDemandSlot.Caption,
                                          sizeof(Applied.RevisionDemandSlot.Caption), "%s",
                                          SliderCaptions[SliderOrdinal]);
                            std::snprintf(Applied.RevisionDemandSlot.Secondary,
                                          sizeof(Applied.RevisionDemandSlot.Secondary),
                                          "%.1f \u2192 %.1f",
                                          EnvironmentFrom[SliderOrdinal], Coordinate);
                        }
                    }
                }

                RowCursor += Scaled.RowHeight;
            }

            Surface->Release();
        }

        Sweep = Card.MaximumY + Pad * 0.85f;
    };

    const bool Transforms = Current.Subject != EntitySubject::Level
                         && Current.Subject != EntitySubject::Grouping
                         && Current.Subject != EntitySubject::Script;

    if (Transforms)
    {
        const char* const TransformRows[3] = { "Position", "Rotation", "Scale" };
        RecordCard("Transform", TransformRows, 3u);
    }

    char ComponentCaption[48] = {};
    std::snprintf(ComponentCaption, sizeof(ComponentCaption), "%s Component",
                  EntityText(Current.Subject));

    // 📐 The per-subject field sets, transcribed from the reference's own component branch.
    switch (Current.Subject)
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
        case EntitySubject::Sun:
        {
            // 📝 The sun card — the four ordinates the sky renderer reads, each a live slider. The card
            //    is drawn only while the host presents the environment; the validation host never sets
            //    the flag and renders the reference's generic Illuminant card instead.
            if (Applied.EnvironmentPresented)
            {
                const char* const SunCaptions[4] = { "Elevation", "Azimuth", "Intensity", "Temperature" };
                const char* const SunUnits[4]    = { "\u00B0", "\u00B0", "lx", "K" };
                const double SunMinimums[4]      = { 0.0, 0.0, 0.0, 1000.0 };
                const double SunMaximums[4]      = { 90.0, 360.0, 10.0, 12000.0 };
                double SunValues[4]              = { Applied.Environment.SunElevation,
                                                     Applied.Environment.SunAzimuth,
                                                     Applied.Environment.SunIntensity,
                                                     Applied.Environment.SunTemperature };

                RecordEnvironmentCard("Sun", SunCaptions, SunUnits, SunMinimums, SunMaximums,
                                      SunValues, 4u);

                Applied.Environment.SunElevation   = SunValues[0];
                Applied.Environment.SunAzimuth     = SunValues[1];
                Applied.Environment.SunIntensity   = SunValues[2];
                Applied.Environment.SunTemperature = SunValues[3];
            }
            else
            {
                const char* const Fields[3] = { "Intensity", "Cast Shadows", "Light Color" };
                RecordCard(ComponentCaption, Fields, 3u);
            }
            break;
        }
        case EntitySubject::Sky:
        {
            if (Applied.EnvironmentPresented)
            {
                const char* const SkyCaptions[2] = { "Sky Intensity", "Turbidity" };
                const char* const SkyUnits[2]    = { "", "" };
                const double SkyMinimums[2]      = { 0.0, 1.0 };
                const double SkyMaximums[2]      = { 3.0, 10.0 };
                double SkyValues[2]              = { Applied.Environment.SkyIntensity,
                                                     Applied.Environment.SkyTurbidity };

                RecordEnvironmentCard("Sky", SkyCaptions, SkyUnits, SkyMinimums, SkyMaximums,
                                      SkyValues, 2u);

                Applied.Environment.SkyIntensity = SkyValues[0];
                Applied.Environment.SkyTurbidity = SkyValues[1];

                const char* const AtmoCaptions[2] = { "Atmosphere Density", "Scale Height" };
                const char* const AtmoUnits[2]    = { "", "" };
                const double AtmoMinimums[2]      = { 0.0, 0.2 };
                const double AtmoMaximums[2]      = { 3.0, 3.0 };
                double AtmoValues[2]              = { Applied.Environment.AtmosphereDensity,
                                                      Applied.Environment.AtmosphereScaleHeight };

                RecordEnvironmentCard("Atmosphere", AtmoCaptions, AtmoUnits, AtmoMinimums,
                                      AtmoMaximums, AtmoValues, 2u);

                Applied.Environment.AtmosphereDensity     = AtmoValues[0];
                Applied.Environment.AtmosphereScaleHeight = AtmoValues[1];
            }
            else
            {
                const char* const Fields[3] = { "Intensity", "Cast Shadows", "Light Color" };
                RecordCard(ComponentCaption, Fields, 3u);
            }
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

void GlobalShellPanel::RecordRevisionSpine(const PlaneExtent& Extent, ShellContext& Applied,
                                           const EntityRow* Rows, std::uint32_t RowCount,
                                           const EntityRevision* Revisions, std::uint32_t RevisionCount)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    if (Revisions == nullptr)
        RevisionCount = 0u;

    const bool Selected = Applied.EntityTaken < RowCount && Applied.EntityTaken < RowCeiling;

    // 📐 The reference gathers the taken record AND everything nested inside it, so a folder presents
    //    its children's revisions too. The run is linear and carries depth, so the descent is the span of
    //    rows deeper than the taken one that follow it.
    std::uint32_t Minimum = Applied.EntityTaken;
    std::uint32_t Maximum  = Applied.EntityTaken;

    if (Selected)
    {
        for (std::uint32_t Inward = Applied.EntityTaken + 1u; Inward < RowCount && Inward < RowCeiling;
             ++Inward)
        {
            if (Rows[Inward].Depth <= Rows[Applied.EntityTaken].Depth)
                break;

            Maximum = Inward;
        }
    }

    std::uint32_t Current = 0u;

    if (Selected)
    {
        for (std::uint32_t Ordinal = 0u; Ordinal < RevisionCount; ++Ordinal)
        {
            if (Revisions[Ordinal].Against >= Minimum && Revisions[Ordinal].Against <= Maximum)
                ++Current;
        }
    }

    if (!Selected || Current == 0u)
    {
        const float Run   = Scaled.RunSecondary;
        const char* Prose = "No history events found for this selection or its children.";

        Surface->TextRun(Extent.MinimumX + (Extent.Width()
                                              - Surface->MeasureRun(Prose, Run, 0.0f)) * 0.5f,
                         Extent.MinimumY + Scaled.HeaderHeight, Tinted.Faint, Prose, Run);
        return;
    }

    const float Pad    = Scaled.PanePad;
    float       Sweep = Extent.MinimumY + Pad;

    Surface->Confine(Extent);

    // 📐 One group per record that carries a revision, in the run's own order.
    for (std::uint32_t Against = Minimum; Against <= Maximum && Against < RowCeiling; ++Against)
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
        const PlaneExtent GroupHead = Spanning(Extent.MinimumX + Pad, Sweep,
                                               Extent.Width() - Pad * 2.0f, Scaled.RowHeight);

        const bool OnHead = GroupHead.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Sampled.ContactPressed && OnHead && !Ledger->AnyDisclosed())
            Ledger->Grab(RevisionGroups[Against], ControlPart::Chevron);

        if (OnHead && Ledger->Released(RevisionGroups[Against]))
            Applied.RevisionFolded[Against] = !Applied.RevisionFolded[Against];

        const bool  GroupFolded = Applied.RevisionFolded[Against];
        const float Opened      = Controls.OutlineExpansion(RevisionGroups[Against], !GroupFolded, true);

        const float CrestExtent = Scaled.ActionGlyph + 5.0f;
        const PlaneExtent Crest = Spanning(GroupHead.MinimumX,
                                           GroupHead.MinimumY
                                           + (GroupHead.Height() - CrestExtent) * 0.5f,
                                           CrestExtent, CrestExtent);

        Surface->Ground(Crest, Hue, 5.0f, CornerAll);

        const float CrestFigure = CrestExtent * 0.6f;

        Surface->Stroke(EntityGlyph(Grouped.Subject),
                        Spanning(Crest.MinimumX + (CrestExtent - CrestFigure) * 0.5f,
                                 Crest.MinimumY + (CrestExtent - CrestFigure) * 0.5f,
                                 CrestFigure, CrestFigure),
                        Covering(0xFFFFFFu));

        const float NameRun = Scaled.RunPrimary;
        const float NameTop = GroupHead.MinimumY + (GroupHead.Height() - NameRun) * 0.5f;

        // 📐 `{n} ops` at the trailing edge, and the chevron outboard of it.
        char Tallied[16] = {};
        std::snprintf(Tallied, sizeof(Tallied), "%u ops", static_cast<unsigned>(Held));

        const float TallyRun  = Scaled.RunFine;
        const float Mark      = Scaled.ActionGlyph;
        const float TallyLead = GroupHead.MaximumX - Mark - Pad
                              - Surface->MeasureRun(Tallied, TallyRun, 0.0f);

        Surface->TextRun(TallyLead, GroupHead.MinimumY + (GroupHead.Height() - TallyRun) * 0.5f,
                         Tinted.Muted, Tallied, TallyRun);

        Surface->Stroke(GroupFolded ? SymbolSubject::ChevronRight : SymbolSubject::ChevronDown,
                        Spanning(GroupHead.MaximumX - Mark,
                                 GroupHead.MinimumY + (GroupHead.Height() - Mark) * 0.5f,
                                 Mark, Mark),
                        Tinted.Faint);

        Surface->TextRunTruncated(Crest.MaximumX + Pad, NameTop, TallyLead - Pad,
                                  OnHead ? Covering(0xFFFFFFu) : Tinted.Primary,
                                  Grouped.Naming, NameRun, true);

        Sweep += Scaled.RowHeight + 4.0f;

        if (Opened <= 0.0f)
            continue;

        // ② The revisions themselves — a numbered bubble, the spine, and the card beside them.
        const float CardHeight  = Scaled.LayerHeadHeight;
        const float WholeHeight = static_cast<float>(Held) * (CardHeight + 4.0f) * Opened;
        const PlaneExtent Stack = Spanning(Extent.MinimumX, Sweep, Extent.Width(), WholeHeight);

        Surface->Confine(Stack);

        float         X     = Sweep;
        std::uint32_t Numbered  = 0u;

        for (std::uint32_t Ordinal = 0u; Ordinal < RevisionCount; ++Ordinal)
        {
            const EntityRevision& Revised = Revisions[Ordinal];

            if (Revised.Against != Against)
                continue;

            const bool First = Numbered == 0u;
            const bool Last  = Numbered + 1u == Held;

            const float BubbleExtent = 25.0f;
            const float BubbleLead   = Extent.MinimumX + Pad
                                     + (32.0f - BubbleExtent) * 0.5f;
            const float BubbleMid    = X + 7.0f + BubbleExtent * 0.5f;

            // 📐 The spine, `w-[6px]`, stopping half way at the first and last of the group so the run
            //    reads as a bracket — the same rule the layer stack's own rail follows.
            const float SpineMid  = Extent.MinimumX + Pad + 32.0f + 15.0f * 0.5f;
            const float SpineTop  = First ? BubbleMid : X;
            const float SpineFoot = Last  ? BubbleMid : X + CardHeight + 4.0f;

            if (SpineFoot > SpineTop)
            {
                Surface->Ground(Spanning(SpineMid - 3.0f, SpineTop, 6.0f, SpineFoot - SpineTop),
                                Hue, 4.0f, CornerAll);
            }

            Surface->Ground(Spanning(BubbleLead, X + 7.0f, BubbleExtent, BubbleExtent),
                            Hue, BubbleExtent * 0.5f, CornerAll);

            char Counted[4] = {};
            std::snprintf(Counted, sizeof(Counted), "%02u", static_cast<unsigned>(Numbered));

            const float CountRun = Scaled.RunFine;

            Surface->TextRun(BubbleLead + (BubbleExtent
                                           - Surface->MeasureRun(Counted, CountRun, 0.0f)) * 0.5f,
                             X + 7.0f + (BubbleExtent - CountRun) * 0.5f,
                             Covering(0xFFFFFFu), Counted, CountRun, 0.0f, true);

            // 📐 The 7 px node, ringed by 3 px of the pane's own ground.
            Surface->Medallion(SpineMid, BubbleMid, 6.5f, Tinted.MenuLower);
            Surface->Medallion(SpineMid, BubbleMid, 3.5f, Covering(0xFFFFFFu));

            const PlaneExtent Card = Spanning(SpineMid + 15.0f * 0.5f + 8.0f, X,
                                              Extent.MaximumX - Pad
                                              - (SpineMid + 15.0f * 0.5f + 8.0f), CardHeight);

            const bool OnCard = Card.Encloses(Sampled.PositionX, Sampled.PositionY);

            Surface->Ground(Card, OnCard ? Tinted.TileHovered : Tinted.Tile, Scaled.LayerRadius, CornerAll);
            Surface->Edge(Card, Tinted.Hairline, 1.0f, Scaled.LayerRadius, CornerAll);

            const RevisionDeclaration Declared{ Revised.Description, Revised.Secondary, Revised.TimeRun };

            Controls.RevisionRow(Card, Declared, OnCard);

            X    += CardHeight + 4.0f;
            Numbered += 1u;
        }

        Surface->Release();

        Sweep += WholeHeight + Pad * 2.0f;
    }

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  SLIDE TWO, WHOLE
//------------------------------------------------------------------------------------------------------------------------

void GlobalShellPanel::RecordComponents(const PlaneExtent& Extent, ShellContext& Applied,
                                        const EntityRow* Rows, std::uint32_t RowCount,
                                        const EntityRevision* Revisions, std::uint32_t RevisionCount)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    const bool Selected = Applied.EntityTaken < RowCount && Applied.EntityTaken < RowCeiling;

    const PlaneExtent Header = Spanning(Extent.MinimumX, Extent.MinimumY,
                                        Extent.Width(), Scaled.HeaderHeight);

    // ① The header, and the Back call it carries at its trailing edge.
    if (!Selected)
    {
        RecordPaneHeader(Header, SymbolSubject::CubeSolid, Tinted.Faint, Covering(0x111111u),
                         "Nothing selected", nullptr);
    }
    else
    {
        const EntityRow&  Current = Rows[Applied.EntityTaken];
        const ThemeToken Hue       = EntityHue(Current.Subject);

        char Classified[48] = {};
        std::snprintf(Classified, sizeof(Classified), "%s Entity", EntityText(Current.Subject));

        RecordPaneHeader(Header, EntityGlyph(Current.Subject), Hue, Covering(0x111111u),
                         Current.Naming, Classified);

        // 📝 The header's secondary run carries the entity hue rather than the shared faint colour, so it
        //    is recorded again over the shared header's own.
        const float SecondaryRun = Scaled.RunFine;
        const float PairHeight   = Scaled.RunPrimary * RunLeading + SecondaryRun * RunLeading;
        const float PairLead     = Header.MinimumY + (Header.Height() - PairHeight) * 0.5f;
        const float RunLead      = Header.MinimumX + Scaled.HeaderPadX + Scaled.MedallionExtent
                                 + Scaled.HeaderPadX * 0.8f;

        Surface->TextRunTruncated(RunLead, PairLead + Scaled.RunPrimary * RunLeading,
                                  Header.MaximumX - RunLead - Scaled.HeaderPadX,
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
                             + Scaled.HeaderPadX;

        const PlaneExtent Call = Spanning(Header.MaximumX - Scaled.HeaderPadX - CallSpan,
                                          Header.MinimumY + (Header.Height() - 28.0f) * 0.5f,
                                          CallSpan, 28.0f);

        const bool OnCall = Call.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Sampled.ContactPressed && OnCall && !Ledger->AnyDisclosed())
            Ledger->Grab(BackCall, ControlPart::Body);

        if (OnCall && Ledger->Released(BackCall))
        {
            Applied.InspectorShown = false;

            if (Motion != nullptr)
            {
                EasedInterpolant& Travelling = Motion->Eased(CarouselSlide);

                Travelling.Depart(Travelling.Current(), 0.0, CarouselTravelOver, 0.0, EaseCurve::Carousel);
            }
        }

        Ledger->DeclareHovered(BackCall, OnCall, HoverOver);

        if (OnCall)
            Surface->Ground(Call, Tinted.TileHovered, Scaled.FieldRadius, CornerAll);

        Surface->Stroke(SymbolSubject::ChevronRight,
                        Spanning(Call.MinimumX, Call.MinimumY + (Call.Height() - Mark) * 0.5f,
                                 Mark, Mark),
                        OnCall ? Tinted.Primary : Tinted.Muted, 3.1415927f);

        Surface->TextRun(Call.MinimumX + Mark + Gap,
                         Call.MinimumY + (Call.Height() - Run) * 0.5f,
                         OnCall ? Tinted.Primary : Tinted.Muted, Caption, Run);
    }

    // ② The strip, and the inner carousel it drives.
    static const char* const Captions[2] = { "Properties", "History" };

    const PlaneExtent Strip = Spanning(Extent.MinimumX, Header.MaximumY,
                                       Extent.Width(), Scaled.ComponentY);

    const TabDeclaration Declared{ Captions, 2u };

    static_cast<void>(Controls.TabStrip(InspectorStrip, Strip, Declared, Applied.InspectorTab));

    const PlaneExtent Pages = Spanning(Extent.MinimumX, Strip.MaximumY, Extent.Width(),
                                       Extent.MaximumY - Strip.MaximumY - Scaled.FooterHeight);

    // 📐 The reference lays a 200 %-wide strip inside the body and translates it by half its own
    //    extent, exactly as the outer inspector does — so the inner travel is the outer one, one level in.
    const float Carried = (Applied.InspectorTab == 1u) ? -Pages.Width() : 0.0f;

    Surface->Confine(Pages);

    const PlaneExtent Leading = Spanning(Pages.MinimumX + Carried, Pages.MinimumY,
                                         Pages.Width(), Pages.Height());
    const PlaneExtent Trailing = Spanning(Leading.MaximumX, Pages.MinimumY,
                                          Pages.Width(), Pages.Height());

    if (!Surface->Excluded(Leading))
        RecordPropertyCards(Leading, Applied, Rows, RowCount);

    if (!Surface->Excluded(Trailing))
        RecordRevisionSpine(Trailing, Applied, Rows, RowCount, Revisions, RevisionCount);

    Surface->Release();

    // ③ The footer, `{n} fields`.
    const PlaneExtent Footer = Spanning(Extent.MinimumX, Extent.MaximumY - Scaled.FooterHeight,
                                        Extent.Width(), Scaled.FooterHeight);

    Surface->Ground(Footer, Tinted.MenuLower, 0.0f, CornerNone);
    Surface->Ground(Spanning(Footer.MinimumX, Footer.MinimumY, Footer.Width(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    if (Selected)
    {
        const ThemeToken Hue       = EntityHue(Rows[Applied.EntityTaken].Subject);
        const float       FooterRun = Scaled.RunFine;
        const float       FooterTop = Footer.MinimumY + (Footer.Height() - FooterRun) * 0.5f;
        const float       ChipY  = Footer.MinimumY
                                    + (Footer.Height() - Scaled.ChipExtent) * 0.5f;

        Surface->Ground(Spanning(Footer.MinimumX + Scaled.HeaderPadX, ChipY,
                                 Scaled.ChipExtent, Scaled.ChipExtent), Hue, 2.0f, CornerAll);

        Surface->TextRun(Footer.MinimumX + Scaled.HeaderPadX + Scaled.ChipExtent
                         + Scaled.PanePad, FooterTop, Tinted.Muted,
                         (Applied.InspectorTab == 0u) ? "Properties" : "History", FooterRun);
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE LAYER STACK
//------------------------------------------------------------------------------------------------------------------------

float GlobalShellPanel::RecordLayerRow(const PlaneExtent& Extent, ShellContext& Applied,
                                       const LayerRow& Current, std::uint32_t Ordinal,
                                       std::uint32_t LayerCount, bool Trailing)
{
    const bool Unfolded = Applied.LayerUnfolded[Ordinal];
    const bool Shown    = Applied.LayerShown[Ordinal];
    const bool Taken    = Applied.LayerTaken == Ordinal;

    // ① The spine gutter, `w-[30px]`, and the coloured rail running through it. The reference stops the
    //    rail half way at the first and last rows so the run reads as a bracket rather than a full column.
    const float SpineMid   = Extent.MinimumX + Scaled.LayerSpineX * 0.5f;
    const float BadgeMid   = Extent.MinimumY + Scaled.LayerHeadHeight * 0.5f;
    const bool  First      = Ordinal == 0u;
    const float SpineTop   = First ? BadgeMid : Extent.MinimumY;
    const float SpineFoot  = Trailing ? BadgeMid : Extent.MaximumY;

    if (SpineFoot > SpineTop)
    {
        Surface->Ground(Spanning(SpineMid - Scaled.LayerSpineWidth * 0.5f, SpineTop,
                                 Scaled.LayerSpineWidth, SpineFoot - SpineTop),
                        Shown ? Covering(Current.TagHue) : Covering(0x1B1B1Bu),
                        Scaled.LayerSpineWidth * 0.5f, CornerAll);
    }

    // 📐 The ordinal badge counts DOWN the stack — `String(layers.length - idx).padStart(2,'0')`.
    const float BadgeExtent = Scaled.LayerBadge;

    Surface->Medallion(SpineMid, BadgeMid, BadgeExtent * 0.5f,
                       Shown ? Covering(Current.TagHue) : Covering(0x2A2A2Au));

    char Badge[3] = { '0', '0', '\0' };
    const std::uint32_t Counted = (LayerCount > Ordinal) ? (LayerCount - Ordinal) : 0u;

    Badge[0] = static_cast<char>('0' + ((Counted / 10u) % 10u));
    Badge[1] = static_cast<char>('0' + (Counted % 10u));

    const float BadgeRun = Scaled.RunFine * 0.95f;
    Surface->TextRun(SpineMid - Surface->MeasureRun(Badge, BadgeRun, 0.0f) * 0.5f,
                     BadgeMid - BadgeRun * 0.5f, Covering(0xFFFFFFu), Badge, BadgeRun);

    // ② The row card itself, beginning after the spine gutter.
    const PlaneExtent Card = Spanning(Extent.MinimumX + Scaled.LayerSpineX, Extent.MinimumY,
                                      Extent.Width() - Scaled.LayerSpineX - Scaled.HeaderPadX,
                                      Extent.Height());

    Surface->Ground(Card, Taken ? Tinted.AccentSoft : Tinted.Tile, Scaled.LayerRadius, CornerAll);
    Surface->Edge(Card, Taken ? Tinted.Accent : Tinted.Hairline, 1.0f, Scaled.LayerRadius, CornerAll);

    const PlaneExtent Head = Spanning(Card.MinimumX, Card.MinimumY,
                                      Card.Width(), Scaled.LayerHeadHeight);

    const float HalfX = Head.Width() * 0.5f;

    const PlaneExtent LayerHalf = Spanning(Head.MinimumX, Head.MinimumY,
                                           HalfX, Head.Height());
    const PlaneExtent MaskHalf  = Spanning(Head.MinimumX + HalfX, Head.MinimumY,
                                           HalfX, Head.Height());

    // 📝 The divider between the halves, `w-[1px] my-[6px]`.
    Surface->Ground(Spanning(Head.MinimumX + HalfX, Head.MinimumY + Scaled.LayerRowPad,
                             1.0f, Head.Height() - Scaled.LayerRowPad * 2.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    // 📝 The taken half carries `shadow-[inset_0_-2px_0_var(--accent)]` beneath it.
    const auto MarkTaken = [&](const PlaneExtent& Half, LayerTarget Target)
    {
        if (!Taken || Applied.TargetTaken != Target)
            return;

        Surface->Ground(Half, Partial(0xFFFFFFu, 0.06), 0.0f, CornerNone);
        Surface->Ground(Spanning(Half.MinimumX, Half.MaximumY - 2.0f, Half.Width(), 2.0f),
                        Tinted.Accent, 0.0f, CornerNone);
    };

    MarkTaken(LayerHalf, LayerTarget::Layer);
    MarkTaken(MaskHalf,  LayerTarget::Mask);

    // ③ The left half — chevron, eye, swatch, then the naming pair.
    float Sweep = LayerHalf.MinimumX + Scaled.LayerRowPad;

    const float ActionMid = LayerHalf.MinimumY + (LayerHalf.Height() - Scaled.LayerAction) * 0.5f;

    const PlaneExtent Chevron = Spanning(Sweep, ActionMid, Scaled.LayerAction, Scaled.LayerAction);
    const bool OnChevron = Chevron.Encloses(Sampled.PositionX, Sampled.PositionY);

    Surface->Stroke(Unfolded ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                    Spanning(Chevron.MinimumX + 3.0f, Chevron.MinimumY + 3.0f,
                             Scaled.LayerAction - 6.0f, Scaled.LayerAction - 6.0f),
                    OnChevron ? Tinted.Primary : Tinted.Muted);

    Sweep += Scaled.LayerAction + Scaled.LayerGap * 0.5f;

    const PlaneExtent Presence = Spanning(Sweep, ActionMid, Scaled.LayerAction, Scaled.LayerAction);
    const bool OnPresence = Presence.Encloses(Sampled.PositionX, Sampled.PositionY);

    Surface->Stroke(Shown ? SymbolSubject::EyeOpen : SymbolSubject::EyeClosed,
                    Spanning(Presence.MinimumX + 3.5f, Presence.MinimumY + 3.5f,
                             Scaled.LayerAction - 7.0f, Scaled.LayerAction - 7.0f),
                    OnPresence ? Tinted.Primary : Tinted.Muted);

    Sweep += Scaled.LayerAction + Scaled.LayerGap * 0.5f;

    // 📐 The paint swatch — a black tile holding a 14 px colour square and a 4 px classification dot.
    const PlaneExtent Swatch = Spanning(Sweep, LayerHalf.MinimumY
                                        + (LayerHalf.Height() - Scaled.LayerSwatch) * 0.5f,
                                        Scaled.LayerSwatch, Scaled.LayerSwatch);

    Surface->Ground(Swatch, Covering(0x000000u), Scaled.FieldRadius, CornerAll);
    Surface->Edge(Swatch, Covering(0x1C1C1Cu), 1.0f, Scaled.FieldRadius, CornerAll);

    const float Inner = Scaled.LayerSwatch * (14.0f / 26.0f);
    Surface->Ground(Spanning(Swatch.MinimumX + (Scaled.LayerSwatch - Inner) * 0.5f,
                             Swatch.MinimumY + (Scaled.LayerSwatch - Inner) * 0.5f, Inner, Inner),
                    Covering(Current.PaintHue), 3.0f, CornerAll);

    const float Dot = Scaled.LayerSwatch * (4.0f / 26.0f);
    Surface->Medallion(Swatch.MaximumX - Dot, Swatch.MinimumY + Dot, Dot * 0.5f,
                       ClassificationTint(Current.Classified));

    Sweep += Scaled.LayerSwatch + Scaled.LayerGap;

    // 📐 The naming pair — the layer's name over `{blend} · {opacity}%`.
    const float NameRun    = Scaled.RunSecondary;
    const float DetailRun  = Scaled.RunFine * 0.95f;
    const float PairHeight = NameRun * 1.2f + DetailRun * 1.3f;
    const float PairTop    = LayerHalf.MinimumY + (LayerHalf.Height() - PairHeight) * 0.5f;
    const float NameCeil   = LayerHalf.MaximumX - Sweep - Scaled.LayerRowPad;

    // 🔴 Both runs are borrowed from the host and neither is dereferenced unchecked. `TextRunTruncated`
    //    tolerates an absent run; the blend is copied byte by byte here, so it is the one that must be
    //    guarded before the walk rather than inside it.
    const char* const Blending = (Current.Blend != nullptr) ? Current.Blend : "Normal";

    Surface->TextRunTruncated(Sweep, PairTop, NameCeil, Tinted.Primary, Current.Naming, NameRun, true);

    char Detail[48] = {};
    std::uint32_t Written = 0u;

    for (const char* Reading = Blending; *Reading != '\0' && Written < 30u; ++Reading)
        Detail[Written++] = *Reading;

    const char* const Separator = " \u00B7 ";

    for (const char* Reading = Separator; *Reading != '\0' && Written < 40u; ++Reading)
        Detail[Written++] = *Reading;

    const std::uint32_t Percent = (Current.Opacity > 100u) ? 100u : Current.Opacity;

    if (Percent >= 100u)      { Detail[Written++] = '1'; Detail[Written++] = '0'; Detail[Written++] = '0'; }
    else if (Percent >= 10u)  { Detail[Written++] = static_cast<char>('0' + Percent / 10u);
                                Detail[Written++] = static_cast<char>('0' + Percent % 10u); }
    else                      { Detail[Written++] = static_cast<char>('0' + Percent); }

    Detail[Written++] = '%';
    Detail[Written]   = '\0';

    Surface->TextRunTruncated(Sweep, PairTop + NameRun * 1.2f, NameCeil,
                              Covering(0x6A6A6Au), Detail, DetailRun, false);

    // ④ The right half — either the "No Mask" prompt or the mask's own eye, chip, pair and cross.
    float MaskCursor = MaskHalf.MinimumX + Scaled.LayerRowPad;

    PlaneExtent MaskEye = {};
    bool        OnMaskEye = false;

    if (!Current.MaskDeclared)
    {
        const PlaneExtent Absent = Spanning(MaskCursor, MaskHalf.MinimumY
                                            + (MaskHalf.Height() - Scaled.LayerSwatch) * 0.5f,
                                            Scaled.LayerSwatch, Scaled.LayerSwatch);

        Surface->Edge(Absent, Partial(0x8A8A8Au, 0.40), 1.0f, 5.0f, CornerAll);
        Surface->Stroke(SymbolSubject::PlusCross,
                        Spanning(Absent.MinimumX + 8.0f, Absent.MinimumY + 8.0f,
                                 Scaled.LayerSwatch - 16.0f, Scaled.LayerSwatch - 16.0f),
                        Partial(0x8A8A8Au, 0.40));

        MaskCursor += Scaled.LayerSwatch + Scaled.LayerGap;

        Surface->TextRun(MaskCursor, MaskHalf.MinimumY + (MaskHalf.Height() - Scaled.RunSmall) * 0.5f,
                         Partial(0x8A8A8Au, 0.40), "No Mask", Scaled.RunSmall);
    }
    else
    {
        MaskEye   = Spanning(MaskCursor, ActionMid, Scaled.LayerAction, Scaled.LayerAction);
        OnMaskEye = MaskEye.Encloses(Sampled.PositionX, Sampled.PositionY);

        Surface->Stroke(SymbolSubject::EyeOpen,
                        Spanning(MaskEye.MinimumX + 3.5f, MaskEye.MinimumY + 3.5f,
                                 Scaled.LayerAction - 7.0f, Scaled.LayerAction - 7.0f),
                        OnMaskEye ? Tinted.Primary : Tinted.Muted);

        MaskCursor += Scaled.LayerAction + Scaled.LayerGap * 0.5f;

        // 📐 The mask chip — a white square whose opacity states the mask's strength.
        const PlaneExtent Chip = Spanning(MaskCursor, MaskHalf.MinimumY
                                          + (MaskHalf.Height() - Scaled.LayerSwatch) * 0.5f,
                                          Scaled.LayerSwatch, Scaled.LayerSwatch);

        Surface->Ground(Chip, Covering(0x000000u), Scaled.FieldRadius, CornerAll);
        Surface->Edge(Chip, Covering(0x1C1C1Cu), 1.0f, Scaled.FieldRadius, CornerAll);

        const double Strength = static_cast<double>((Current.MaskStrength > 100u)
                                                  ? 100u : Current.MaskStrength) / 100.0;

        Surface->Ground(Spanning(Chip.MinimumX + 2.0f, Chip.MinimumY + 2.0f,
                                 Scaled.LayerSwatch - 4.0f, Scaled.LayerSwatch - 4.0f),
                        Partial(0xFFFFFFu, Strength), 3.0f, CornerAll);

        MaskCursor += Scaled.LayerSwatch + Scaled.LayerGap;

        char Strong[8] = {};
        std::uint32_t Marked = 0u;
        const std::uint32_t Reading = (Current.MaskStrength > 100u) ? 100u : Current.MaskStrength;

        if (Reading >= 100u)     { Strong[Marked++] = '1'; Strong[Marked++] = '0'; Strong[Marked++] = '0'; }
        else if (Reading >= 10u) { Strong[Marked++] = static_cast<char>('0' + Reading / 10u);
                                   Strong[Marked++] = static_cast<char>('0' + Reading % 10u); }
        else                     { Strong[Marked++] = static_cast<char>('0' + Reading); }

        Strong[Marked++] = '%';
        Strong[Marked]   = '\0';

        Surface->TextRunTruncated(MaskCursor, PairTop, MaskHalf.MaximumX - MaskCursor - 44.0f,
                                  Tinted.Primary, "Mask", NameRun, true);
        Surface->TextRunTruncated(MaskCursor, PairTop + NameRun * 1.2f,
                                  MaskHalf.MaximumX - MaskCursor - 44.0f,
                                  Covering(0x6A6A6Au), Strong, DetailRun, false);
    }

    // 📝 The bin sits at the trailing edge with `ml-auto`, whether or not a mask stands.
    const PlaneExtent Retire = Spanning(MaskHalf.MaximumX - Scaled.LayerAction - Scaled.LayerRowPad,
                                        ActionMid, Scaled.LayerAction, Scaled.LayerAction);

    const bool OnRetire = Retire.Encloses(Sampled.PositionX, Sampled.PositionY);

    Surface->Stroke(SymbolSubject::TrashBin,
                    Spanning(Retire.MinimumX + 3.5f, Retire.MinimumY + 3.5f,
                             Scaled.LayerAction - 7.0f, Scaled.LayerAction - 7.0f),
                    OnRetire ? Covering(0xEF4444u) : Tinted.Muted);

    // ⑤ Every contact for this row, resolved in the reference's own precedence: the actions outrank the
    //    halves, and the halves outrank nothing else because the card takes no contact of its own.
    if (Sampled.ContactPressed && !Ledger->AnyDisclosed())
    {
        if (OnChevron)
            Ledger->Grab(LayerFolds[Ordinal], ControlPart::Chevron);
        else if (OnPresence)
            Ledger->Grab(LayerPresences[Ordinal], ControlPart::Body);
        else if (OnRetire)
            Ledger->Grab(LayerRetires[Ordinal], ControlPart::Body);
        else if (OnMaskEye)
            Ledger->Grab(LayerMaskEyes[Ordinal], ControlPart::Body);
        else if (LayerHalf.Encloses(Sampled.PositionX, Sampled.PositionY))
            Ledger->Grab(LayerHalves[Ordinal * 2u], ControlPart::Body);
        else if (MaskHalf.Encloses(Sampled.PositionX, Sampled.PositionY))
            Ledger->Grab(LayerHalves[Ordinal * 2u + 1u], ControlPart::Body);
    }

    if (OnChevron && Ledger->Released(LayerFolds[Ordinal]))
        Applied.LayerUnfolded[Ordinal] = !Applied.LayerUnfolded[Ordinal];

    if (OnPresence && Ledger->Released(LayerPresences[Ordinal]))
        Applied.LayerShown[Ordinal] = !Applied.LayerShown[Ordinal];

    if (Ledger->Released(LayerHalves[Ordinal * 2u])
        && LayerHalf.Encloses(Sampled.PositionX, Sampled.PositionY))
    {
        Applied.LayerTaken  = Ordinal;
        Applied.TargetTaken = LayerTarget::Layer;
    }

    if (Ledger->Released(LayerHalves[Ordinal * 2u + 1u])
        && MaskHalf.Encloses(Sampled.PositionX, Sampled.PositionY))
    {
        Applied.LayerTaken  = Ordinal;
        Applied.TargetTaken = LayerTarget::Mask;
    }

    if (!Unfolded)
        return Scaled.LayerHeadHeight;

    // ⑥ The folded half — two property columns beneath the head, `border-t` and a darker ground.
    const PlaneExtent Folded = Spanning(Card.MinimumX, Head.MaximumY,
                                        Card.Width(), Extent.Height() - Scaled.LayerHeadHeight);

    Surface->Ground(Folded, Partial(0x000000u, 0.15), 0.0f, CornerNone);
    Surface->Ground(Spanning(Folded.MinimumX, Folded.MinimumY, Folded.Width(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    Surface->Ground(Spanning(Folded.MinimumX + HalfX, Folded.MinimumY + Scaled.LayerFoldPad,
                             1.0f, Folded.Height() - Scaled.LayerFoldPad * 2.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    // 📐 A labelled property row: a 50 px caption column and the control beside it.
    const auto RecordField = [&](float X, float Y, float X2, const char* Caption,
                                 const char* Reading)
    {
        Surface->TextRun(X, Y + (Scaled.LayerFieldRow - Scaled.RunFine) * 0.5f,
                         Covering(0x8A8A8Au), Caption, Scaled.RunFine);

        const float ValueX = X + Scaled.LayerLabelX;
        const PlaneExtent Well = Spanning(ValueX, Y + 2.0f,
                                          X2 - Scaled.LayerLabelX, Scaled.LayerFieldRow - 4.0f);

        Surface->Ground(Well, Tinted.MenuLower, Scaled.FieldRadius, CornerAll);
        Surface->Edge(Well, Tinted.Hairline, 1.0f, Scaled.FieldRadius, CornerAll);
        Surface->TextRunTruncated(Well.MinimumX + 6.0f,
                                  Well.MinimumY + (Well.Height() - Scaled.RunFine) * 0.5f,
                                  Well.Width() - 12.0f, Tinted.Primary, Reading, Scaled.RunFine);
    };

    const float ColumnX = HalfX - Scaled.LayerFoldPad * 2.0f;
    float       FoldCursor  = Folded.MinimumY + Scaled.LayerFoldPad;

    RecordField(Folded.MinimumX + Scaled.LayerFoldPad, FoldCursor, ColumnX, "Blend", Blending);

    FoldCursor += Scaled.LayerFieldRow + Scaled.LayerGap;

    char Opac[8] = {};
    std::uint32_t Marked = 0u;

    if (Percent >= 100u)     { Opac[Marked++] = '1'; Opac[Marked++] = '0'; Opac[Marked++] = '0'; }
    else if (Percent >= 10u) { Opac[Marked++] = static_cast<char>('0' + Percent / 10u);
                               Opac[Marked++] = static_cast<char>('0' + Percent % 10u); }
    else                     { Opac[Marked++] = static_cast<char>('0' + Percent); }

    Opac[Marked++] = '%';
    Opac[Marked]   = '\0';

    RecordField(Folded.MinimumX + Scaled.LayerFoldPad, FoldCursor, ColumnX, "Opac", Opac);

    FoldCursor += Scaled.LayerFieldRow + Scaled.LayerGap;

    Surface->TextRunCapitalised(Folded.MinimumX + Scaled.LayerFoldPad, FoldCursor,
                                Covering(0x6A6A6Au), "Channels", Scaled.RunFine);

    FoldCursor += Scaled.RunFine * 1.6f;

    // 📐 The channel pills, wrapped as the reference's `flex-wrap` wraps them.
    float PillX = Folded.MinimumX + Scaled.LayerFoldPad;

    for (std::uint32_t Channel = 0u;
         Channel < Current.ChannelCount && Channel < LayerRow::ChannelCeiling; ++Channel)
    {
        const char* Naming = Current.Channels[Channel];

        if (Naming == nullptr || Naming[0] == '\0')
            continue;

        const float PillRun   = Surface->MeasureRun(Naming, Scaled.RunFine, 0.0f);
        const float PillWidth = PillRun + 16.0f;

        if (PillX + PillWidth > Folded.MinimumX + Scaled.LayerFoldPad + ColumnX)
        {
            PillX   = Folded.MinimumX + Scaled.LayerFoldPad;
            FoldCursor += Scaled.LayerPillY + 4.0f;
        }

        const PlaneExtent Pill = Spanning(PillX, FoldCursor, PillWidth, Scaled.LayerPillY);

        Surface->Ground(Pill, Covering(0x1C1C1Cu), 4.0f, CornerAll);
        Surface->Edge(Pill, Covering(0x2A2A2Au), 1.0f, 4.0f, CornerAll);
        Surface->TextRun(Pill.MinimumX + 8.0f,
                         Pill.MinimumY + (Scaled.LayerPillY - Scaled.RunFine) * 0.5f,
                         Covering(0x8A8A8Au), Naming, Scaled.RunFine);

        PillX += PillWidth + 4.0f;
    }

    // ⑦ The mask column, which either offers "Add Mask" or states the mask's own properties.
    const float MaskColumnX = Folded.MinimumX + HalfX + Scaled.LayerFoldPad;
    float       MaskFold        = Folded.MinimumY + Scaled.LayerFoldPad;

    if (!Current.MaskDeclared)
    {
        const float ButtonX = 74.0f;
        const PlaneExtent Add = Spanning(MaskColumnX + (ColumnX - ButtonX) * 0.5f,
                                         Folded.MinimumY + (Folded.Height() - Scaled.LayerFieldRow) * 0.5f,
                                         ButtonX, Scaled.LayerFieldRow);

        Surface->Ground(Add, Tinted.Accent, Scaled.FieldRadius, CornerAll);
        Surface->TextRun(Add.MinimumX + (ButtonX
                                           - Surface->MeasureRun("Add Mask", Scaled.RunFine, 0.0f)) * 0.5f,
                         Add.MinimumY + (Scaled.LayerFieldRow - Scaled.RunFine) * 0.5f,
                         Covering(0xFFFFFFu), "Add Mask", Scaled.RunFine);
    }
    else
    {
        char Strong[8] = {};
        std::uint32_t Stated = 0u;
        const std::uint32_t Reading = (Current.MaskStrength > 100u) ? 100u : Current.MaskStrength;

        if (Reading >= 100u)     { Strong[Stated++] = '1'; Strong[Stated++] = '0'; Strong[Stated++] = '0'; }
        else if (Reading >= 10u) { Strong[Stated++] = static_cast<char>('0' + Reading / 10u);
                                   Strong[Stated++] = static_cast<char>('0' + Reading % 10u); }
        else                     { Strong[Stated++] = static_cast<char>('0' + Reading); }

        Strong[Stated++] = '%';
        Strong[Stated]   = '\0';

        RecordField(MaskColumnX, MaskFold, ColumnX, "Str", Strong);

        MaskFold += Scaled.LayerFieldRow + Scaled.LayerGap;

        Surface->TextRun(MaskColumnX, MaskFold + (Scaled.LayerFieldRow - Scaled.RunFine) * 0.5f,
                         Covering(0x8A8A8Au), "Invert", Scaled.RunFine);

        // 📐 The invert switch, `w-[26px] h-[14px]` with a 10 px nub.
        const PlaneExtent Switch = Spanning(MaskColumnX + Scaled.LayerLabelX,
                                            MaskFold + (Scaled.LayerFieldRow - Scaled.LayerSwitchHeight) * 0.5f,
                                            Scaled.LayerSwitchX, Scaled.LayerSwitchHeight);

        Surface->Ground(Switch, Current.MaskInverted ? Tinted.Accent : Covering(0x2A2A2Au),
                        Scaled.LayerSwitchHeight * 0.5f, CornerAll);
        Surface->Edge(Switch, Current.MaskInverted ? Tinted.Accent : Covering(0x3A3A3Au),
                      1.0f, Scaled.LayerSwitchHeight * 0.5f, CornerAll);

        const float Nub = Scaled.LayerSwitchHeight - 4.0f;
        Surface->Medallion(Current.MaskInverted ? Switch.MaximumX - 2.0f - Nub * 0.5f
                                                  : Switch.MinimumX + 2.0f + Nub * 0.5f,
                           Switch.MinimumY + Scaled.LayerSwitchHeight * 0.5f,
                           Nub * 0.5f, Covering(0xEDEDEDu));

        MaskFold += Scaled.LayerFieldRow + Scaled.LayerGap;

        Surface->TextRunCapitalised(MaskColumnX, MaskFold, Covering(0x6A6A6Au), "Sources", Scaled.RunFine);

        MaskFold += Scaled.RunFine * 1.6f;

        const PlaneExtent Source = Spanning(MaskColumnX, MaskFold, ColumnX, 24.0f);

        Surface->Ground(Source, Tinted.MenuLower, Scaled.FieldRadius, CornerAll);
        Surface->Edge(Source, Tinted.Hairline, 1.0f, Scaled.FieldRadius, CornerAll);
        Surface->Ground(Spanning(Source.MinimumX + 6.0f, Source.MinimumY + 6.0f, 12.0f, 12.0f),
                        Covering(0x10B981u), 2.0f, CornerAll);
        Surface->TextRun(Source.MinimumX + 24.0f, Source.MinimumY + (24.0f - Scaled.RunFine) * 0.5f,
                         Tinted.Primary,
                         (Current.Classified == LayerClassification::Generator) ? "Generator" : "Generated",
                         Scaled.RunFine);
    }

    return Extent.Height();
}

void GlobalShellPanel::RecordLayerStack(const PlaneExtent& Extent, ShellContext& Applied,
                                        const LayerRow* Layers, std::uint32_t LayerCount)
{
    // 📝 The pane's own ground is `bg-[#0b0b0b]`, which is darker than `--menu`; the reference states it as
    //    a literal rather than through a custom property, so it is a literal here too.
    Surface->Ground(Extent, Covering(0x0B0B0Bu), 0.0f, CornerNone);
    Surface->Ground(Spanning(Extent.MaximumX - 1.0f, Extent.MinimumY, 1.0f, Extent.Height()),
                    Tinted.Hairline, 0.0f, CornerNone);

    const std::uint32_t VisibleCount = (LayerCount < LayerCeiling) ? LayerCount : LayerCeiling;

    // ① The header — a medallion, "Layer Stack", its subtitle, and the count pill at the trailing edge.
    const PlaneExtent Header = Spanning(Extent.MinimumX, Extent.MinimumY,
                                        Extent.Width(), Scaled.HeaderHeight);

    RecordPaneHeader(Header, SymbolSubject::LayerMerge, Covering(0x8A8A8Au), Covering(0x000000u),
                     "Layer Stack", "Suzanne \u00B7 one material + paint");

    char Counted[4] = {};
    std::uint32_t Marked = 0u;

    if (VisibleCount >= 10u) Counted[Marked++] = static_cast<char>('0' + (VisibleCount / 10u) % 10u);
    Counted[Marked++] = static_cast<char>('0' + VisibleCount % 10u);
    Counted[Marked]   = '\0';

    const float PillRun   = Surface->MeasureRun(Counted, Scaled.RunFine, 0.0f);
    const float PillX = PillRun + 12.0f;

    const PlaneExtent CountPill = Spanning(Header.MaximumX - Scaled.HeaderPadX - PillX,
                                           Header.MinimumY + (Header.Height() - 20.0f) * 0.5f,
                                           PillX, 20.0f);

    Surface->Ground(CountPill, Covering(0x1B1B1Bu), 10.0f, CornerAll);
    Surface->Edge(CountPill, Covering(0x2A2A2Au), 1.0f, 10.0f, CornerAll);
    Surface->TextRun(CountPill.MinimumX + 6.0f, CountPill.MinimumY + (20.0f - Scaled.RunFine) * 0.5f,
                     Covering(0x8A8A8Au), Counted, Scaled.RunFine);

    // ② The toolbar — the full-width "Add layer" button and the retention field beneath it.
    const float Pad = Scaled.PanePad;

    const PlaneExtent Add = Spanning(Extent.MinimumX + Pad, Header.MaximumY + Pad,
                                     Extent.Width() - Pad * 2.0f, Scaled.LayerToolHeight);

    const bool OnAdd = Add.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (OnAdd && Sampled.ContactPressed && !Ledger->AnyDisclosed())
        Ledger->Grab(LayerAdd, ControlPart::Body);

    Surface->Ground(Add, Covering(0x141414u), 7.0f, CornerAll);
    Surface->Edge(Add, OnAdd ? Covering(0x3A3A3Au) : Covering(0x242424u), 1.0f, 7.0f, CornerAll);

    const float AddRun   = Surface->MeasureRun("Add layer", Scaled.RunSecondary, 0.0f);
    const float AddLead  = Add.MinimumX + (Add.Width() - AddRun - 18.0f) * 0.5f;
    const float AddMid   = Add.MinimumY + (Add.Height() - Scaled.RunSecondary) * 0.5f;

    Surface->Stroke(SymbolSubject::PlusCross,
                    Spanning(AddLead, Add.MinimumY + (Add.Height() - 13.0f) * 0.5f, 13.0f, 13.0f),
                    OnAdd ? Tinted.Primary : Covering(0x8A8A8Au));

    Surface->TextRun(AddLead + 18.0f, AddMid, OnAdd ? Tinted.Primary : Covering(0x8A8A8Au),
                     "Add layer", Scaled.RunSecondary);

    // 📝 The reference's filter field carries no border until it is hovered or taken.
    const PlaneExtent Retaining = Spanning(Extent.MinimumX + Pad, Add.MaximumY + 5.0f,
                                           Extent.Width() - Pad * 2.0f, Scaled.SearchHeight * (26.0f / 30.0f));

    const bool OnRetaining = Retaining.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (OnRetaining && Sampled.ContactPressed)
        Ledger->Grab(LayerRetention, ControlPart::Body);

    const bool Retained = Ledger->Holding(LayerRetention) || Ledger->Disclosed(LayerRetention);

    if (OnRetaining || Retained)
    {
        Surface->Ground(Retaining, Covering(0x121214u), Scaled.FieldRadius, CornerAll);
        Surface->Edge(Retaining, Retained ? Tinted.Accent : Covering(0x242424u),
                      1.0f, Scaled.FieldRadius, CornerAll);
    }

    const float SearchGlyph = 12.0f;

    Surface->Stroke(SymbolSubject::MagnifierLens,
                    Spanning(Retaining.MinimumX + 8.0f,
                             Retaining.MinimumY + (Retaining.Height() - SearchGlyph) * 0.5f,
                             SearchGlyph, SearchGlyph), Covering(0x5A5A5Au));

    Surface->TextRun(Retaining.MinimumX + 26.0f,
                     Retaining.MinimumY + (Retaining.Height() - Scaled.RunSecondary) * 0.5f,
                     Covering(0x4A4A4Au), "Filter layers\u2026", Scaled.RunSecondary);

    // ③ The stack itself, confined so a tall row cannot record over the footer.
    const PlaneExtent Footer = Spanning(Extent.MinimumX, Extent.MaximumY - Scaled.FooterHeight,
                                        Extent.Width(), Scaled.FooterHeight);

    const PlaneExtent Body = Spanning(Extent.MinimumX + 3.0f, Retaining.MaximumY + Pad,
                                      Extent.Width() - 6.0f,
                                      Footer.MinimumY - Retaining.MaximumY - Pad);

    Surface->Confine(Body);

    float Sweep = Body.MinimumY;

    for (std::uint32_t Ordinal = 0u; Ordinal < VisibleCount && Layers != nullptr; ++Ordinal)
    {

        // 📐 An unfolded row stands taller by its two property columns. The reference sizes that half by
        //    its content; the tallest arrangement is the mask column's four stated rows.
        const float FoldedY = Applied.LayerUnfolded[Ordinal]
                                 ? (Scaled.LayerFoldPad * 2.0f + Scaled.LayerFieldRow * 2.0f
                                    + Scaled.LayerGap * 2.0f + Scaled.RunFine * 1.6f
                                    + Scaled.LayerPillY + 8.0f)
                                 : 0.0f;

        const PlaneExtent Row = Spanning(Body.MinimumX, Sweep, Body.Width(),
                                         Scaled.LayerHeadHeight + FoldedY);

        Sweep += Row.Height() + Scaled.LayerRowGap;

        if (Surface->Excluded(Row))
            continue;

        static_cast<void>(RecordLayerRow(Row, Applied, Layers[Ordinal], Ordinal, VisibleCount,
                                         Ordinal + 1u == VisibleCount));
    }

    Surface->Release();

    // ④ The footer — shown and hidden counts, and the reorder hint.
    Surface->Ground(Footer, Covering(0x121214u), 0.0f, CornerNone);
    Surface->Ground(Spanning(Footer.MinimumX, Footer.MinimumY, Footer.Width(), 1.0f),
                    Covering(0x1C1C1Cu), 0.0f, CornerNone);

    std::uint32_t Current = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < Current; ++Ordinal)
    {
        if (Applied.LayerShown[Ordinal])
            ++Current;
    }

    const float FootRun = Scaled.RunFine;
    const float FootMid = Footer.MinimumY + (Footer.Height() - FootRun) * 0.5f;
    float       FootAt  = Footer.MinimumX + 11.0f;

    Surface->Ground(Spanning(FootAt, FootMid + FootRun * 0.25f, 7.0f, 7.0f),
                    Covering(0x94A3B8u), 2.0f, CornerAll);

    FootAt += 14.0f;

    char Tally[24] = {};
    std::uint32_t Placed = 0u;

    if (Current >= 10u) Tally[Placed++] = static_cast<char>('0' + (Current / 10u) % 10u);
    Tally[Placed++] = static_cast<char>('0' + Current % 10u);
    Tally[Placed]   = '\0';

    Surface->TextRun(FootAt, FootMid, Tinted.Primary, Tally, FootRun);
    FootAt += Surface->MeasureRun(Tally, FootRun, 0.0f) + 4.0f;
    Surface->TextRun(FootAt, FootMid, Covering(0x8A8A8Au), "shown", FootRun);
    FootAt += Surface->MeasureRun("shown", FootRun, 0.0f) + 7.0f;
    Surface->TextRun(FootAt, FootMid, Covering(0x2C2C2Cu), "\u00B7", FootRun);
    FootAt += 7.0f;

    const std::uint32_t Withheld = Current - Current;

    Placed = 0u;
    if (Withheld >= 10u) Tally[Placed++] = static_cast<char>('0' + (Withheld / 10u) % 10u);
    Tally[Placed++] = static_cast<char>('0' + Withheld % 10u);
    Tally[Placed]   = '\0';

    Surface->TextRun(FootAt, FootMid, Tinted.Primary, Tally, FootRun);
    FootAt += Surface->MeasureRun(Tally, FootRun, 0.0f) + 4.0f;
    Surface->TextRun(FootAt, FootMid, Covering(0x8A8A8Au), "hidden", FootRun);

    const float HintRun = Surface->MeasureRun("drag to reorder", FootRun, 0.0f);

    Surface->TextRun(Footer.MaximumX - 11.0f - HintRun, FootMid,
                     Covering(0x8A8A8Au), "drag to reorder", FootRun);
}

void GlobalShellPanel::RecordLayerInspector(const PlaneExtent& Extent, const ShellContext& Applied,
                                            const LayerRow* Layers, std::uint32_t LayerCount)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    const std::uint32_t VisibleCount = (LayerCount < LayerCeiling) ? LayerCount : LayerCeiling;
    const bool          TakenWithin  = (Layers != nullptr) && (Applied.LayerTaken < VisibleCount);

    // 📐 `LayerInspectorPane` delegates by `target`: a mask target presents `MaskPropertyPanel`, and every
    //    other presents `ChannelPropertyPanel`. Both open with the same header naming the taken layer.
    const bool Masking = Applied.TargetTaken == LayerTarget::Mask;

    const PlaneExtent Header = Spanning(Extent.MinimumX, Extent.MinimumY,
                                        Extent.Width(), Scaled.HeaderHeight);

    RecordPaneHeader(Header, Masking ? SymbolSubject::LayerMerge : SymbolSubject::PaintBristle,
                     Covering(0xFFFFFFu),
                     TakenWithin ? Covering(Layers[Applied.LayerTaken].PaintHue) : Tinted.Tile,
                     Masking ? "Mask Properties" : "Channel Properties",
                     TakenWithin ? Layers[Applied.LayerTaken].Naming : "No layer taken");

    if (!TakenWithin)
        return;

    const LayerRow& Taken = Layers[Applied.LayerTaken];
    const float     Pad   = Scaled.PanePad * 1.5f;

    float Sweep = Header.MaximumY + Pad;

    // 📝 One card per stated property, on the same terms as the component inspector's cards.
    const auto RecordCard = [&](const char* Caption, const char* Reading, ThemeToken Marker)
    {
        const PlaneExtent Card = Spanning(Extent.MinimumX + Pad, Sweep,
                                          Extent.Width() - Pad * 2.0f, Scaled.ComponentY);

        Surface->Ground(Card, Tinted.Tile, Scaled.FieldRadius, CornerAll);
        Surface->Edge(Card, Tinted.Hairline, 1.0f, Scaled.FieldRadius, CornerAll);

        Surface->Ground(Spanning(Card.MinimumX + 8.0f,
                                 Card.MinimumY + (Card.Height() - 8.0f) * 0.5f, 8.0f, 8.0f),
                        Marker, 2.0f, CornerAll);

        Surface->TextRun(Card.MinimumX + 22.0f,
                         Card.MinimumY + (Card.Height() - Scaled.RunSecondary) * 0.5f,
                         Tinted.Primary, Caption, Scaled.RunSecondary);

        const float ReadRun = Surface->MeasureRun(Reading, Scaled.RunFine, 0.0f);

        Surface->TextRun(Card.MaximumX - 10.0f - ReadRun,
                         Card.MinimumY + (Card.Height() - Scaled.RunFine) * 0.5f,
                         Tinted.Muted, Reading, Scaled.RunFine);

        Sweep += Scaled.ComponentY + 6.0f;
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

        Sweep += 4.0f;

        Surface->TextRunCapitalised(Extent.MinimumX + Pad, Sweep, Tinted.Faint, "Channels",
                                    Scaled.RunFine);

        Sweep += Scaled.RunFine * 1.8f;

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

void GlobalShellPanel::RecordInspector(const PlaneExtent& Extent, ShellContext& Applied,
                                       const EntityRow* Rows, std::uint32_t RowCount,
                                       const LayerRow* Layers, std::uint32_t LayerCount,
                                       const EntityRevision* Revisions, std::uint32_t RevisionCount)
{
    // 📐 The reference lays a 200 %-wide strip inside the inspector and translates it by half its own extent.
    //    The strip is therefore two panes of the inspector's extent, and the travel is one whole extent.
    const float Travelled = (Motion != nullptr)
                          ? static_cast<float>(Motion->Eased(CarouselSlide).Current())
                          : (Applied.InspectorShown ? 1.0f : 0.0f);

    const float Carried = -Held(Travelled, 0.0f, 1.0f) * Extent.Width();

    // 🔴 Confined to the inspector's own extent. The trailing slide begins one whole extent to the trailing
    //    side and would otherwise be recorded over the viewport and the rail beside it.
    Surface->Confine(Extent);

    const PlaneExtent Leading = Spanning(Extent.MinimumX + Carried, Extent.MinimumY,
                                         Extent.Width(), Extent.Height());
    const PlaneExtent Trailing = Spanning(Leading.MaximumX, Extent.MinimumY,
                                          Extent.Width(), Extent.Height());

    // ① Slide one. 🔴 Which pane stands here is decided by the mode, exactly as the reference's ternary in
    //    `app/page.tsx` decides it: Texture Paint presents `LayersPane` alone, filling the whole slide, and
    //    every other mode presents an outliner beside a prompt.
    if (Applied.Mode == WorkspaceMode::TexturePaint)
    {
        if (!Surface->Excluded(Leading))
            RecordLayerStack(Leading, Applied, Layers, LayerCount);

        if (!Surface->Excluded(Trailing))
            RecordLayerInspector(Trailing, Applied, Layers, LayerCount);

        Surface->Release();
        return;
    }

    if (!Surface->Excluded(Leading))
    {
        const float OutlinerX = (Scaled.OutlinerX < Leading.Width() * 0.6f)
                                  ? Scaled.OutlinerX : Leading.Width() * 0.6f;

        const PlaneExtent Outlining = Spanning(Leading.MinimumX, Leading.MinimumY,
                                               OutlinerX, Leading.Height());

        RecordOutlineColumn(Outlining, Applied, Rows, RowCount);

        const PlaneExtent Prompting = Spanning(Outlining.MaximumX, Leading.MinimumY,
                                               Leading.Width() - OutlinerX, Leading.Height());

        // 📐 Which pane stands here is the mode's, exactly as the reference's ternary decides it. Drafting
        //    presents `MetadataPane` under its own header; World Editor presents three lines of prose and
        //    nothing else, and the reference states them as one wrapped paragraph.
        if (Applied.Mode == WorkspaceMode::Drafting)
        {
            Surface->Ground(Prompting, Tinted.MenuLower, 0.0f, CornerNone);

            const PlaneExtent Crown = Spanning(Prompting.MinimumX, Prompting.MinimumY,
                                               Prompting.Width(), Scaled.HeaderHeight);

            // 📐 The header is itself the call: `onClick={() => setShowInspector(true)}`, rousing to
            //    `#292930`, and it carries a chevron at the trailing edge to say so.
            const bool OnCrown = Crown.Encloses(Sampled.PositionX, Sampled.PositionY);

            RecordPaneHeader(Crown, SymbolSubject::CrosshairCentre, Covering(0xFFFFFFu),
                             Covering(0x000000u), "Properties & Actions", nullptr);

            if (OnCrown)
                Surface->Ground(Crown, Faded(Tinted.TileHovered, 0.5f), 0.0f, CornerNone);

            const float Mark = Scaled.MedallionExtent * (13.0f / 24.0f);

            Surface->Stroke(SymbolSubject::ChevronRight,
                            Spanning(Crown.MaximumX - Scaled.HeaderPadX - Mark,
                                     Crown.MinimumY + (Crown.Height() - Mark) * 0.5f, Mark, Mark),
                            OnCrown ? Tinted.Primary : Tinted.Muted);

            RecordMetadata(Spanning(Prompting.MinimumX, Crown.MaximumY, Prompting.Width(),
                                    Prompting.MaximumY - Crown.MaximumY),
                           Applied, Rows, RowCount);
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
            float       ProseTop  = Prompting.MinimumY
                                  + (Prompting.Height() - ProseStep * 3.0f) * 0.5f;

            for (const char* Line : Prose)
            {
                const float LineX = Surface->MeasureRun(Line, ProseRun, 0.0f);

                Surface->TextRun(Prompting.MinimumX + (Prompting.Width() - LineX) * 0.5f,
                                 ProseTop, Tinted.Faint, Line, ProseRun);
                ProseTop += ProseStep;
            }
        }
    }

    // ② Slide two — the component inspector for whatever the outliner has taken.
    if (!Surface->Excluded(Trailing))
        RecordComponents(Trailing, Applied, Rows, RowCount, Revisions, RevisionCount);

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE WHOLE SHELL
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> GlobalShellPanel::Record(const PlaneExtent&     Extent,
                                       ShellContext&        Applied,
                                       const EntityRow*       Rows,
                                       std::uint32_t          RowCount,
                                       const LayerRow*        Layers,
                                       std::uint32_t          LayerCount,
                                       const EntityRevision*  Revisions,
                                       std::uint32_t          RevisionCount)
{
    if (Ledger == nullptr || Surface == nullptr || Appearance == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "the shell panel is unconstructed" });

    if (!Surface->Recording())
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no tick stands adopted" });

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
    if (LayerCount > 0u && Applied.LayerTaken >= LayerCount)
        Applied.LayerTaken = LayerCount - 1u;

    // 📝 The strip is carried by the same traverse whether it was moved by a key or by a press, so the
    //    target is restated every tick and the traverse departs only when it actually disagrees.
    if (Motion != nullptr)
    {
        EasedInterpolant& Travelling = Motion->Eased(CarouselSlide);
        const double      Wanted     = Applied.InspectorShown ? 1.0 : 0.0;

        if (Travelling.Settled && Travelling.Current() != Wanted)
            Travelling.Depart(Travelling.Current(), Wanted, CarouselTravelOver, 0.0, EaseCurve::Carousel);
    }

    // ① The desk, beneath everything.
    Surface->Ground(Extent, Tinted.Desk, 0.0f, CornerNone);

    // ② The top bar.
    const PlaneExtent TopBar = Spanning(Extent.MinimumX, Extent.MinimumY,
                                        Extent.Width(), Scaled.TopBarHeight);

    RecordTopBar(TopBar, Applied);

    // ③ The body: the Options rail, the viewport, and the docked inspector when it stands.
    const PlaneExtent Body = Spanning(Extent.MinimumX, TopBar.MaximumY,
                                      Extent.Width(), Extent.MaximumY - TopBar.MaximumY);

    const PlaneExtent Rail = Spanning(Body.MinimumX, Body.MinimumY,
                                      Scaled.OptionsX, Body.Height());

    RecordOptionsRail(Rail, Applied);

    // 📐 The docked inspector is 700 px, but never more than what remains beside the rail — the reference's
    //    `flex-none` beside a `flex-1` viewport collapses the viewport first, and this reproduces that
    //    without letting the inspector overrun the rail on a narrow display.
    const float Remaining     = Body.MaximumX - Rail.MaximumX;
    const float InspectorSpan = Applied.InspectorDocked
                              ? ((Scaled.InspectorX < Remaining * 0.75f)
                                 ? Scaled.InspectorX : Remaining * 0.75f)
                              : 0.0f;

    const PlaneExtent Viewport = Spanning(Rail.MaximumX, Body.MinimumY,
                                          Remaining - InspectorSpan, Body.Height());

    RecordViewport(Viewport, Applied);

    InspectorStood = false;

    if (Applied.InspectorDocked)
    {
        const PlaneExtent Docked = Spanning(Viewport.MaximumX, Body.MinimumY,
                                            InspectorSpan, Body.Height());

        Surface->Ground(Docked, Tinted.Menu, 0.0f, CornerNone);
        Surface->Ground(Spanning(Docked.MinimumX, Docked.MinimumY, 1.0f, Docked.Height()),
                        Tinted.HairlineFirm, 0.0f, CornerNone);

        RecordInspector(Docked, Applied, Rows, RowCount, Layers, LayerCount, Revisions, RevisionCount);

        InspectorAt    = Docked;
        InspectorStood = true;
    }

    // ④ The veil and the summoned card — recorded LAST, so they are above everything written before them.
    // 🔴 This ordering IS the fix. The defect these replace painted a full-extent ground into the background
    //    command list while the panels beneath it were vendor windows, which composite above that list — so
    //    the veil darkened the display and the panels stayed legible and pressable straight through it. One
    //    list, written in order, cannot express that condition at all.
    if (!Applied.InspectorDocked && Applied.MenuOpened)
    {
        Surface->Ground(Extent, Tinted.Veil, 0.0f, CornerNone);

        const bool OnVeil = Extent.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (OnVeil && Sampled.ContactPressed)
            Ledger->Grab(VeilContact, ControlPart::Body);

        const float CardX  = (Scaled.InspectorX < Extent.Width() - Scaled.OptionsX)
                               ? Scaled.InspectorX : Extent.Width() * 0.75f;
        const float CardHeight = (Scaled.SummonedY < Extent.Height() - Scaled.TopBarHeight * 2.0f)
                               ? Scaled.SummonedY : Extent.Height() * 0.75f;

        const PlaneExtent Card = Spanning(Extent.MinimumX + (Extent.Width() - CardX) * 0.5f,
                                          Extent.MinimumY + (Extent.Height() - CardHeight) * 0.5f,
                                          CardX, CardHeight);

        // 📝 The veil dismisses the menu only when the contact was NOT inside the card. The reference relies
        //    on the card being a later sibling that stops the click; one list has no bubbling, so the test
        //    is stated here.
        if (Ledger->Released(VeilContact) &&
            !Card.Encloses(Sampled.PositionX, Sampled.PositionY))
        {
            Applied.MenuOpened = false;
        }

        Surface->Ground(Card, Tinted.Menu, Scaled.MenuRadius, CornerAll);
        Surface->Edge(Card, Tinted.HairlineFirm, 1.0f, Scaled.MenuRadius, CornerAll);

        // 🔴 The card's content is clipped to its own rounded extent, so the outliner's square corners do
        //    not fill the radius the card just drew.
        Surface->Confine(Card);
        RecordInspector(Card, Applied, Rows, RowCount, Layers, LayerCount, Revisions, RevisionCount);
        Surface->Release();

        // 📝 The rounded corners are restored over the content, which was recorded square inside them.
        Surface->MaskCorners(Card, Tinted.Veil, Scaled.MenuRadius);
        Surface->Edge(Card, Tinted.HairlineFirm, 1.0f, Scaled.MenuRadius, CornerAll);

        InspectorAt    = Card;
        InspectorStood = true;
    }

    // ⑤ The floating context card, recorded after the veil and the summoned card both — it is the reference's
    //    `z-[101]`, which is one above the overlay that dismisses it, and therefore above everything here.
    RecordContextOverlay(Extent, Applied, Rows, RowCount);

    return Outcome<bool>::Result(true);
}

bool GlobalShellPanel::Occluding(float X, float Y) const
{
    if (!InspectorStood)
        return false;

    return InspectorAt.Encloses(X, Y);
}

}   // namespace Slate
