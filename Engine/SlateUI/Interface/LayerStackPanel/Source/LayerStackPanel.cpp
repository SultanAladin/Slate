//============================================================================================================================================
//                                                         LAYERSTACKPANEL.CPP
//============================================================================================================================================
// 🧩 Records the layer stack, the channel panel and the mask panel, primitive for primitive against `References/LayerstackV1.html`.

#include "SlateUI/Interface/LayerStackPanel/Api/LayerStackPanel.h"

#include <cstdio>
#include <cstring>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       SMALL HELPERS
//------------------------------------------------------------------------------------------------------------------------

// 📐 The reference tracks its small capitals at .14em on the header and .1em on a section head.
static constexpr float TrackingHead    = 0.14f;
static constexpr float TrackingSection = 0.10f;

static PlaneExtent Squared(float CentreX, float CentreY, float Extent)
{
    const float Half = Extent * 0.5f;
    return PlaneExtent{ CentreX - Half, CentreY - Half, CentreX + Half, CentreY + Half };
}

static PlaneExtent Inset(const PlaneExtent& Extent, float X, float Y)
{
    return PlaneExtent{ Extent.MinimumX + X, Extent.MinimumY + Y,
                        Extent.MaximumX  - X, Extent.MaximumY  - Y };
}

// 📐 What the reference's own transitions run at. `.row` states `transition:background .12s`, and the
//    popup and the card both state `.16s`.
static constexpr double HoverOver = 120.0;   // [ms]
static constexpr double TakeOver  = 160.0;   // [ms]

// 📐 `.stack` scrolls three lines a notch, which at a 45 px row is what the reference's own wheel gives.
static constexpr float NotchHeight = 48.0f;   // [px]

// 📐 A contact that has travelled beyond this is a carry and never a take — `GestureTolerance` states the
//    same six pixels for the same reason, and the reference's HTML drag has the window system's own.
static constexpr float CarryFloor = 6.0f;   // [px]

// 📐 `renderHistory()`'s own columns and heights. The card is `min-height: 44px`; the two fixed columns to
//    its left are 32 px of medallion and 15 px of spine, which puts the spine's centre at 39 and the card's
//    leading edge at 47. The fold is the comment field and the value field over the author line.
static constexpr float RevisionCardHeight = 44.0f;   // [px] - one folded card
static constexpr float RevisionFoldHeight = 96.0f;   // [px] - author line, comment field, value field
static constexpr float RevisionGapY  =  4.0f;   // [px] - `pb-[4px]`
static constexpr float RevisionLeadX  = 55.0f;   // [px] - 32 + 15 + `pl-[8px]`
static constexpr float RevisionSpineX = 39.0f;   // [px] - 32 + 15/2, the spine's own centre

//------------------------------------------------------------------------------------------------------------------------
//                                                        CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::Reapply(const ThemeProfile& Resolved)
{
    Tinted = Resolved.LayerStack;
}

Outcome<bool> LayerStackPanel::Construct(InteractionIndex& Interaction, RecordingSurface& Recording,
                                        const ThemeProfile& Appearance)
{
    if (Ledger != nullptr)
    {
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported,
                                       "the layer stack panel is already constructed" });
    }

    Ledger  = &Interaction;
    Surface = &Recording;

    // 🔴 Every identity claimed here and none inside a tick. The three runs are claimed in one pass so a
    //    refusal partway through retires the whole construction rather than leaving half a panel registered.
    const auto Reserve = [&](ControlIdentity* Written, std::uint32_t Count) -> Outcome<bool>
    {
        for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
        {
            const Outcome<ControlIdentity> Registered = Interaction.Register();

            if (!Registered.Resolved)
            {
                Reset();
                return Outcome<bool>::Refuse(Registered.Error);
            }

            Written[Ordinal] = Registered.Resolve();
        }

        return Outcome<bool>::Result(true);
    };

    if (const auto Verdict = Reserve(RowCells, RowCeiling * CellsPerRow); !Verdict.Resolved)
        return Verdict;

    if (const auto Verdict = Reserve(ChromeCells, ChromeCeiling); !Verdict.Resolved)
        return Verdict;

    if (const auto Verdict = Reserve(PopupEntries, PopupEntryCeiling); !Verdict.Resolved)
        return Verdict;

    if (const auto Verdict = Reserve(RevisionCells, RevisionCellCeiling); !Verdict.Resolved)
        return Verdict;

    if (const auto Verdict = Reserve(CardControls, CardControlCeiling); !Verdict.Resolved)
        return Verdict;

    // 🔴 Constructed AFTER the claims above, so a refusal partway through the runs retires the whole panel
    //    before a component is bound to a ledger it will outlive.
    if (const auto Verdict = CardComponents.Construct(Interaction, Recording, Appearance);
        !Verdict.Resolved)
    {
        Reset();
        return Outcome<bool>::Refuse(Verdict.Error);
    }

    return Outcome<bool>::Result(true);
}

void LayerStackPanel::Advance(const PointerCondition& Contact, double)
{
    Sampled = Contact;

    // 🔴 `Sample` and never `Advance`. The component's own `Advance` would advance the SHARED ledger a
    //    second time, which retires a release before the rows that grabbed on it have observed it.
    CardComponents.Sample(Contact);
}

ControlIdentity LayerStackPanel::NextCardControl()
{
    if (CardControlsSpent >= CardControlCeiling)
        return ControlIdentity{};

    return CardControls[CardControlsSpent++];
}

void LayerStackPanel::Reset()
{
    Ledger  = nullptr;
    Surface = nullptr;
    Sampled = {};

    for (ControlIdentity& Target : RowCells)
        Target = {};

    for (ControlIdentity& Target : ChromeCells)
        Target = {};

    for (ControlIdentity& Target : PopupEntries)
        Target = {};

    for (ControlIdentity& Target : RevisionCells)
        Target = {};

    for (ControlIdentity& Target : CardControls)
        Target = {};

    CardControlsSpent = 0u;
    CardComponents.Reset();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      ONE ARBITRATION
//------------------------------------------------------------------------------------------------------------------------

bool LayerStackPanel::Hovered(const PlaneExtent& Extent) const
{
    if (Surface == nullptr)
        return false;

    // 📐 A confined extent is what actually decides: a row scrolled past the stack's edge still encloses
    //    the pointer arithmetically, and rousing it would light a row nobody can see.
    if (Surface->Excluded(Extent))
        return false;

    return Extent.Encloses(Sampled.PositionX, Sampled.PositionY);
}

bool LayerStackPanel::Pressed(ControlIdentity Target, const PlaneExtent& Extent,
                              LayerStackContext& Applied, const char* Tooltip)
{
    if (Ledger == nullptr)
        return false;

    const bool Over = Hovered(Extent);

    // 📐 `[data-tip]` — the tooltip follows the hover and is recorded in the deferred sweep, applied at the
    //    hovered control's own upper edge exactly as the reference's `getBoundingClientRect` places it.
    if (Over && Tooltip != nullptr)
    {
        Applied.Tooltip       = Tooltip;
        Applied.TooltipX  = (Extent.MinimumX + Extent.MaximumX) * 0.5f;
        Applied.TooltipHeight = Extent.MinimumY;
    }

    // 🔴 A standing popup outranks every row beneath it. Without this the same contact that dismisses a
    //    menu also presses whatever the menu was covering.
    if (Over && Sampled.ContactPressed && !Ledger->AnyDisclosed())
        Ledger->Grab(Target, ControlPart::Body);

    Ledger->DeclareHovered(Target, Over, HoverOver);

    return Over && Ledger->Released(Target);
}

bool LayerStackPanel::Dragged(ControlIdentity Target, const PlaneExtent& Extent, std::uint32_t& Reading)
{
    if (Ledger == nullptr || Extent.Width() <= 0.0f)
        return false;

    const bool Over = Hovered(Extent);

    if (Over && Sampled.ContactPressed && !Ledger->AnyDisclosed())
    {
        Ledger->Grab(Target, ControlPart::Track);
        Ledger->RecordInitial(Target, static_cast<float>(Reading));
    }

    Ledger->DeclareHovered(Target, Over, HoverOver);

    if (!Ledger->Holding(Target))
        return false;

    // 📐 The reading follows the pointer's ABSOLUTE position along the track rather than an accumulated
    //    per-tick delta, which drifts by a pixel for every tick the pointer spent outside the extent.
    const float Fraction = (Sampled.PositionX - Extent.MinimumX) / Extent.Width();
    const float Clamped  = (Fraction < 0.0f) ? 0.0f : ((Fraction > 1.0f) ? 1.0f : Fraction);
    const auto  Resolved = static_cast<std::uint32_t>(Clamped * 100.0f + 0.5f);

    if (Resolved == Reading)
        return false;

    Reading = Resolved;
    return true;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      SHARED FRAGMENTS
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordMeter(const PlaneExtent& Extent, std::uint32_t Reading, ThemeToken Colour)
{
    // 📐 `.mini` — a 4px trough at .12 coverage with the reading filled over it and rounded to a pill.
    Surface->Ground(Extent, Partial(0xFFFFFFu, 0.12), Scaled.MiniY * 0.5f);

    const float Fraction = (Reading > 100u) ? 1.0f : static_cast<float>(Reading) * 0.01f;

    if (Fraction > 0.0f)
    {
        PlaneExtent Filled = Extent;
        Filled.MaximumX   = Extent.MinimumX + Extent.Width() * Fraction;
        Surface->Ground(Filled, Colour, Scaled.MiniY * 0.5f);
    }
}

void LayerStackPanel::RecordChip(const PlaneExtent& Extent, const char* Caption, ThemeToken Colour, bool Solid)
{
    // 📐 `.chip` — an 18px pill, either a .07-coverage ground with a stroke or a solid tint carrying black text.
    const float Radius = Extent.Height() * 0.5f;

    if (Solid)
    {
        Surface->Ground(Extent, Colour, Radius);
    }
    else
    {
        Surface->Ground(Extent, Partial(0xFFFFFFu, 0.07), Radius);
        Surface->Edge(Extent, Tinted.Stroke, 1.0f, Radius);
    }

    const ThemeToken Written = Solid ? Covering(0x000000u) : Colour;
    const float       X   = Extent.MinimumX + (Extent.Width() -
                                Surface->MeasureRun(Caption, Scaled.RunFine, 0.04f)) * 0.5f;

    Surface->TextRun(X, Extent.MinimumY + (Extent.Height() - Surface->LineHeight(Scaled.RunFine)) * 0.5f,
                     Written, Caption, Scaled.RunFine, 0.04f, true);
}

void LayerStackPanel::RecordSectionHead(const PlaneExtent& Extent, const char* Caption, const char* Reading,
                                        bool Opened)
{
    // 📐 `.sech` — a chevron, a tracked small-capital caption and an optional trailing reading.
    const float Middle = (Extent.MinimumY + Extent.MaximumY) * 0.5f;

    Surface->Stroke(Opened ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                    Squared(Extent.MinimumX + 6.0f, Middle, 11.0f), Tinted.Faint);

    Surface->TextRunCapitalised(Extent.MinimumX + 18.0f, Middle - Surface->LineHeight(Scaled.RunSection) * 0.5f,
                                Tinted.Secondary, Caption, Scaled.RunSection, TrackingSection, true);

    if (Reading != nullptr && Reading[0] != '\0')
    {
        const float X = Extent.MaximumX - Surface->MeasureRun(Reading, Scaled.RunFine);
        Surface->TextRun(X, Middle - Surface->LineHeight(Scaled.RunFine) * 0.5f,
                         Tinted.Faint, Reading, Scaled.RunFine);
    }
}

float LayerStackPanel::RecordReadingRow(const PlaneExtent& Extent, const char* Caption, const char* Reading)
{
    // 📐 `.d` — a faint caption on the leading edge and its reading on the trailing one.
    const float Middle   = Extent.MinimumY + Scaled.FieldHeight * 0.5f;
    const float Baseline = Middle - Surface->LineHeight(Scaled.RunSub) * 0.5f;

    Surface->TextRun(Extent.MinimumX, Baseline, Tinted.Faint, Caption, Scaled.RunSub);

    const float X = Extent.MaximumX - Surface->MeasureRun(Reading, Scaled.RunSub);
    Surface->TextRunTruncated(X, Baseline, Extent.Width() * 0.6f, Tinted.Primary,
                              Reading, Scaled.RunSub, true);

    return Extent.MinimumY + Scaled.FieldHeight;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE UNFOLDED CARD
//------------------------------------------------------------------------------------------------------------------------

// 📐 `.card{transition:grid-template-rows .28s var(--ease),opacity .22s}` and `.secb{transition:.24s}`.
static constexpr double CardOver    = 280.0;   // [ms] - the card's own fold
static constexpr double SectionOver = 240.0;   // [ms] - one section's body
static constexpr float  CardOpacityFloor = 0.35f;   // [-]  - `opacity .22s` runs shorter than the height

// 📐 `.cbody` and `.pad` — the card's own inner padding, and the run one control row occupies.
static constexpr float CardRowHeight   = 26.0f;   // [px] - one `.pr` / `.ps` / `.sw2` row
static constexpr float CardRowGap      =  3.0f;   // [px] - `.pgrid` gap
static constexpr float CardNoteLead    = 14.0f;   // [px] - `.note` line height
static constexpr float CardActionRow   = 28.0f;   // [px] - a `.frow` of `.dbtn`

float LayerStackPanel::CardOpening(EasedInterpolant& Fold, bool Unfolded, bool Staged, double Elapsed) const
{
    // 🔴 The staged tick renders CLOSED and departs nothing, which is the whole of `pendingOpen`: a card
    //    that appeared already open would have nothing to travel from and would simply blcolour into place.
    const double Heading = (Unfolded && !Staged) ? 1.0 : 0.0;

    if (Fold.Incoming != Heading)
        Fold.Depart(Fold.Current(), Heading, CardOver, 0.0, EaseCurve::Standard);

    if (!Fold.Settled)
        Fold.Advance(Elapsed);

    const double Current = Fold.Current();

    return (Current < 0.0) ? 0.0f : ((Current > 1.0) ? 1.0f : static_cast<float>(Current));
}

bool LayerStackPanel::RecordCardSection(const PlaneExtent& Extent, const char* Caption, const char* Reading,
                                        CardSection Section, std::uint32_t Ordinal,
                                        LayerStackContext& Applied, bool Recording, float& Y)
{
    const std::uint32_t Bit    = 1u << static_cast<std::uint32_t>(Section);
    const bool          Opened = (Applied.Sections[Ordinal] & Bit) != 0u;

    const PlaneExtent Head = Spanning(Extent.MinimumX, Y, Extent.Width(), Scaled.SectionHeight);

    if (Recording && !Surface->Excluded(Head))
    {
        RecordSectionHead(Inset(Head, Scaled.CardPadX, 0.0f), Caption, Reading, Opened);

        // 📐 `.sech` — the whole head is the control, exactly as the reference's own `<button class="sech">`.
        if (Pressed(NextCardControl(), Head, Applied, nullptr))
            Applied.Sections[Ordinal] ^= Bit;
    }
    else if (Recording)
    {
        // 🔴 The identity is still spent on a culled head, so a section scrolled out of view does not
        //    shift every control after it onto a different identity and fade them all in.
        (void) NextCardControl();
    }

    Y += Scaled.SectionHeight;

    return Opened;
}

float LayerStackPanel::RecordCardNote(const PlaneExtent& Extent, float Y, const char* Body,
                                      bool Recording)
{
    // 📐 `.note` — a faint wrapped run inside the card's padding. Wrapped against the extent at record
    //    time and never measured into storage, so the run may change between two ticks.
    const float MinimumX = Extent.MinimumX + Scaled.CardPadX;
    const float Limit      = Extent.Width() - Scaled.CardPadX * 2.0f;

    if (Body == nullptr || Body[0] == '\0' || Limit <= 0.0f)
        return Y;

    float Written = Y + 2.0f;
    std::uint32_t Opening = 0u;

    while (Body[Opening] != '\0')
    {
        // ① Reach as far along the run as fits, then retreat to the last space so no word is broken.
        std::uint32_t Reach   = Opening;
        std::uint32_t Breaking = 0u;
        char          Line[192] = {};

        while (Body[Reach] != '\0' && (Reach - Opening) < (sizeof Line - 1u))
        {
            Line[Reach - Opening] = Body[Reach];
            Line[Reach - Opening + 1u] = '\0';

            if (Surface != nullptr && Surface->MeasureRun(Line, Scaled.RunFine) > Limit)
            {
                Line[Reach - Opening] = '\0';
                break;
            }

            if (Body[Reach] == ' ')
                Breaking = Reach;

            ++Reach;
        }

        if (Body[Reach] != '\0' && Breaking > Opening)
        {
            Line[Breaking - Opening] = '\0';
            Reach = Breaking + 1u;
        }
        else if (Body[Reach] == '\0')
        {
            Reach = static_cast<std::uint32_t>(std::strlen(Body));
        }

        if (Recording)
        {
            const PlaneExtent LineExtent = Spanning(MinimumX, Written, Limit, CardNoteLead);

            if (!Surface->Excluded(LineExtent))
                Surface->TextRun(MinimumX, Written, Tinted.Faint, Line, Scaled.RunFine);
        }

        Written += CardNoteLead;

        if (Reach <= Opening)
            break;

        Opening = Reach;
    }

    return Written + 4.0f;
}

// 📐 `.pr>label` / `.ps>label` — the caption cell every card row leads with, and the extent left for
//    the control beside it. Stated once so the properties, height and placement rows cannot disagree.
static constexpr float CardCaptionTop = 74.0f;   // [px] - `minmax(74px,33%)`

PlaneExtent LayerStackPanel::RecordCardCaption(const PlaneExtent& Row, const char* Caption)
{
    const float Third   = Row.Width() * 0.33f;
    const float Leading = (Third < CardCaptionTop) ? CardCaptionTop : Third;

    if (Caption != nullptr && Caption[0] != '\0')
    {
        Surface->TextRunCapitalised(Row.MinimumX,
                                    Row.MinimumY + (Row.Height() -
                                    Surface->LineHeight(Scaled.RunSection)) * 0.5f,
                                    Tinted.Faint, Caption, Scaled.RunSection, TrackingSection, true);
    }

    return PlaneExtent{ Row.MinimumX + Leading, Row.MinimumY, Row.MaximumX, Row.MaximumY };
}

float LayerStackPanel::RecordParameterRun(const PlaneExtent& Extent, float Y,
                                          ParameterCoordinate* Parameters, std::uint32_t Count,
                                          const char* const* Options, std::uint32_t OptionCount,
                                          LayerArrangement& Arrangement, RevisionSequence& Revisions,
                                          bool Recording)
{
    // 📐 `paramsHTML` — `.pr` for a range, `.ps` for a selection, `.sw2` for a switch, in declared order.
    const float MinimumX = Extent.MinimumX + Scaled.CardPadX;
    const float Width  = Extent.Width() - Scaled.CardPadX * 2.0f;

    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
    {
        ParameterCoordinate& Parameter = Parameters[Ordinal];
        const PlaneExtent  Row       = Spanning(MinimumX, Y, Width, CardRowHeight);

        if (!Recording || Surface->Excluded(Row))
        {
            if (Recording)
                (void) NextCardControl();

            Y += CardRowHeight + CardRowGap;
            continue;
        }

        if (Parameter.Selected != nullptr)
        {
            // 📐 `.ps` — a label and a pill carrying the taken option.
            // 🔴 A run the caller declared none for still presents its standing reading. The mask's own
            //    parameters name a bitmap and a projection the reference offers no roster for; lending the
            //    field an empty run would draw a pill with nothing in it, which reads as a lost value.
            const char* const* Current = Options;
            std::uint32_t      Offered   = OptionCount;
            std::uint32_t      Taken     = 0u;

            if (Current == nullptr || Offered == 0u)
            {
                Current = &Parameter.Selected;
                Offered   = 1u;
            }
            else
            {
                for (std::uint32_t Option = 0u; Option < Offered; ++Option)
                {
                    if (Current[Option] != nullptr &&
                        std::strcmp(Current[Option], Parameter.Selected) == 0)
                    {
                        Taken = Option;
                        break;
                    }
                }
            }

            // 📐 `.ps{grid-template-columns:minmax(74px,33%) 1fr}` — the caption, then the pill.
            const PlaneExtent Field = RecordCardCaption(Row, Parameter.Naming);

            const SelectionDeclaration Declared{ "", Current, Offered };

            if (CardComponents.SelectionField(NextCardControl(), Field, Declared, Taken).ReadingAltered &&
                Taken < Offered)
            {
                Revisions.Record(Arrangement, "Parameter amended");
                Parameter.Selected = Current[Taken];
            }
        }
        else if (Parameter.Toggling)
        {
            // 📐 `.sw2` — the switch the reference draws for `T(k,l,v)`.
            bool Taken = Parameter.Current >= 0.5;

            if (CardComponents.ToggleRow(NextCardControl(), Row, ToggleDeclaration{ Parameter.Naming },
                                         Taken).ReadingAltered)
            {
                Revisions.Record(Arrangement, "Parameter amended");
                Parameter.Current = Taken ? 1.0 : 0.0;
            }
        }
        else
        {
            // 📐 `.pr` — the caption, the range, and its `.pv` readout on the trailing edge.
            const PlaneExtent Field = RecordCardCaption(Row, Parameter.Naming);

            const MagnitudeDeclaration Declared{ "", Parameter.Unit,
                                                 Parameter.Minimum, Parameter.Maximum };

            if (CardComponents.MagnitudeRow(NextCardControl(), Field, Declared,
                                            Parameter.Current, true).ReadingAltered)
            {
                Revisions.Record(Arrangement, "Parameter amended");
            }
        }

        Y += CardRowHeight + CardRowGap;
    }

    return Y;
}

// 📐 `.dbtn` — the small action button a card's `.frow` carries. Reports a press.
bool LayerStackPanel::RecordCardAction(const PlaneExtent& Extent, const char* Caption, bool Marked,
                                       bool Dangerous, LayerStackContext& Applied)
{
    const ThemeToken Ground = Marked ? Partial(0xFFFFFFu, 0.14) : Partial(0xFFFFFFu, 0.05);

    Surface->Ground(Extent, Hovered(Extent) ? Partial(0xFFFFFFu, 0.10) : Ground, Scaled.RadiusSmall);
    Surface->Edge(Extent, Tinted.Stroke, 1.0f, Scaled.RadiusSmall);

    const ThemeToken Written = Dangerous ? Tinted.Danger : (Marked ? Tinted.Primary : Tinted.Secondary);
    const float       X   = Extent.MinimumX + (Extent.Width() -
                                Surface->MeasureRun(Caption, Scaled.RunFine)) * 0.5f;

    Surface->TextRun(X, Extent.MinimumY + (Extent.Height() -
                     Surface->LineHeight(Scaled.RunFine)) * 0.5f, Written, Caption, Scaled.RunFine);

    return Pressed(NextCardControl(), Extent, Applied, nullptr);
}

float LayerStackPanel::RecordEntryCard(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                                       std::uint32_t Ordinal, LayerStackContext& Applied,
                                       RevisionSequence& Revisions, bool Recording)
{
    LayerEntry& Entry  = Arrangement.Entries[Ordinal];
    const bool  Folder = (Entry.Content == LayerContent::Folder);

    const float MinimumX = Extent.MinimumX + Scaled.CardPadX;
    const float Width  = Extent.Width() - Scaled.CardPadX * 2.0f;

    float Y = Extent.MinimumY + 6.0f;
    char  Reading[48] = {};

    // ① Info — `sec(n,'info','Info','',info,false)`, the reference's own twelve fields.
    std::snprintf(Reading, sizeof Reading, "%s \xC2\xB7 %u%%", Entry.Blend, Entry.Opacity);

    if (RecordCardSection(Extent, "Info", nullptr, CardSection::Info, Ordinal, Applied, Recording, Y))
    {
        const auto Field = [&](const char* Caption, const char* Written)
        {
            const PlaneExtent Row = Spanning(MinimumX, Y, Width, Scaled.FieldHeight);

            if (Recording && !Surface->Excluded(Row))
                RecordReadingRow(Row, Caption, Written);

            Y += Scaled.FieldHeight;
        };

        char Written[48] = {};

        Field("Type", ContentNaming(Entry.Content));
        Field("Blend", Entry.Blend);

        std::snprintf(Written, sizeof Written, "%u%%", Entry.Opacity);
        Field("Opacity", Written);

        if (Folder)
        {
            std::snprintf(Written, sizeof Written, "%u", EnclosedCount(Arrangement, Ordinal));
            Field("Children", Written);
        }
        else
        {
            std::snprintf(Written, sizeof Written, "%u \xC3\x97 %u", Entry.Resolution, Entry.Resolution);
            Field("Resolution", Written);
        }

        Field("Format", Folder ? "\xE2\x80\x94" : Entry.Format);
        Field("Effects", (Entry.EffectCount > 0u) ? Entry.Effects[0] : "none");
        Field("Mask", Entry.Mask.Declared ? SourceNaming(Entry.Mask.Source) : "none");

        std::snprintf(Written, sizeof Written, "%u / %u active", ChannelsEnabled(Entry),
                      LayerStackCeiling::Channels);
        Field("Channels", Written);

        Field("Height -> Normal", Entry.HeightIntegrated ? Entry.HeightBlend : "off");
        Field("Locked", Entry.Secured ? "yes" : "no");
        Field("Modified", Entry.Modified);
    }

    // ② Properties — `sec(n,'props','Properties',blend · op,props,true)`, open by default.
    if (RecordCardSection(Extent, "Properties", Reading, CardSection::Properties, Ordinal, Applied,
                          Recording, Y))
    {
        // 📐 `.frow` — the blend pill over the opacity range, then the three actions.
        const PlaneExtent BlendRow = Spanning(MinimumX, Y, Width, CardRowHeight);

        if (Recording && !Surface->Excluded(BlendRow))
        {
            std::uint32_t BlendCount = 0u;
            const char* const* Blends = BlendNaming(0u, BlendCount);
            std::uint32_t Taken = 0u;

            for (std::uint32_t Option = 0u; Option < BlendCount; ++Option)
            {
                if (std::strcmp(Blends[Option], Entry.Blend) == 0)
                {
                    Taken = Option;
                    break;
                }
            }

            if (CardComponents.SelectionField(NextCardControl(), RecordCardCaption(BlendRow, "Blend"),
                                              SelectionDeclaration{ "", Blends, BlendCount },
                                              Taken).ReadingAltered && Taken < BlendCount)
            {
                Revisions.Record(Arrangement, "Blend amended");
                Entry.Blend = Blends[Taken];
            }
        }
        else if (Recording)
        {
            (void) NextCardControl();
        }

        Y += CardRowHeight + CardRowGap;

        const PlaneExtent OpacityRow = Spanning(MinimumX, Y, Width, CardRowHeight);

        if (Recording && !Surface->Excluded(OpacityRow))
        {
            double Current = static_cast<double>(Entry.Opacity);

            if (CardComponents.MagnitudeRow(NextCardControl(), RecordCardCaption(OpacityRow, "Opacity"),
                                            MagnitudeDeclaration{ "", "%", 0.0, 100.0 },
                                            Current, true).ReadingAltered)
            {
                Revisions.Record(Arrangement, "Opacity amended");
                Entry.Opacity = static_cast<std::uint32_t>(Current + 0.5);
            }
        }
        else if (Recording)
        {
            (void) NextCardControl();
        }

        Y += CardRowHeight + CardRowGap;

        // 📐 The three `.dbtn` actions — lock, mask and solo.
        const PlaneExtent Actions = Spanning(MinimumX, Y, Width, CardActionRow);

        if (Recording && !Surface->Excluded(Actions))
        {
            const float Each = (Width - 12.0f) / 3.0f;

            const PlaneExtent Lock = Spanning(MinimumX, Y, Each, CardActionRow);
            const PlaneExtent Mask = Spanning(MinimumX + Each + 6.0f, Y, Each, CardActionRow);
            const PlaneExtent Solo = Spanning(MinimumX + (Each + 6.0f) * 2.0f, Y, Each, CardActionRow);

            if (RecordCardAction(Lock, Entry.Secured ? "Unlock" : "Lock", Entry.Secured, false, Applied))
            {
                Revisions.Record(Arrangement, "Lock amended");
                Entry.Secured = !Entry.Secured;
            }

            if (RecordCardAction(Mask, Entry.Mask.Declared ? "Remove mask" : "Add mask",
                                 Entry.Mask.Declared, false, Applied))
            {
                Revisions.Record(Arrangement, "Mask amended");
                Entry.Mask.Declared = !Entry.Mask.Declared;
            }

            if (RecordCardAction(Solo, (Arrangement.Soloed == Ordinal) ? "Clear solo" : "Solo",
                                 Arrangement.Soloed == Ordinal, false, Applied))
            {
                Arrangement.Soloed = (Arrangement.Soloed == Ordinal)
                                   ? LayerStackCeiling::AbsentOrdinal : Ordinal;
            }
        }
        else if (Recording)
        {
            (void) NextCardControl();
            (void) NextCardControl();
            (void) NextCardControl();
        }

        Y += CardActionRow + CardRowGap;
    }

    // ③ Channel Blending — `channelsHTML(n)`, absent on a folder.
    if (!Folder)
    {
        std::snprintf(Reading, sizeof Reading, "%u active", ChannelsEnabled(Entry));

        if (RecordCardSection(Extent, "Channel Blending", Reading, CardSection::Channels, Ordinal,
                              Applied, Recording, Y))
        {
            for (std::uint32_t Channel = 0u; Channel < LayerStackCeiling::Channels; ++Channel)
            {
                ChannelCoordinate&  Reading8 = Entry.Channels[Channel];
                const PlaneExtent Row      = Spanning(MinimumX, Y, Width, 28.0f);

                if (!Recording || Surface->Excluded(Row))
                {
                    if (Recording)
                    {
                        (void) NextCardControl();
                        (void) NextCardControl();
                        (void) NextCardControl();
                    }

                    Y += 30.0f;
                    continue;
                }

                Surface->Ground(Row, Reading8.Enabled ? Tinted.Row : Tinted.Detail, Scaled.RadiusSmall);

                const float       Middle = Row.MinimumY + 14.0f;
                const PlaneExtent Dot    = Squared(Row.MinimumX + 12.0f, Middle, 20.0f);

                if (Pressed(NextCardControl(), Dot, Applied,
                            Reading8.Enabled ? "Disable channel" : "Enable channel"))
                {
                    Revisions.Record(Arrangement, "Channel amended");
                    Reading8.Enabled = !Reading8.Enabled;
                }

                Surface->Medallion(Row.MinimumX + 12.0f, Middle, 4.0f,
                                   Reading8.Enabled ? ChannelTint(Channel) : Tinted.Faint);

                Surface->TextRunTruncated(Row.MinimumX + 22.0f,
                                          Middle - Surface->LineHeight(Scaled.RunSub) * 0.5f, 92.0f,
                                          Reading8.Enabled ? Tinted.Primary : Tinted.Faint,
                                          ChannelNaming()[Channel], Scaled.RunSub, false);

                // 📐 `select.pill.sm` — the channel's own blend run, which Normal and Height shorten.
                std::uint32_t BlendCount = 0u;
                const char* const* Blends = BlendNaming(Channel, BlendCount);
                std::uint32_t Taken = 0u;

                for (std::uint32_t Option = 0u; Option < BlendCount; ++Option)
                {
                    if (std::strcmp(Blends[Option], Reading8.Blend) == 0)
                    {
                        Taken = Option;
                        break;
                    }
                }

                const PlaneExtent Pill = Spanning(Row.MinimumX + 118.0f, Row.MinimumY + 2.0f,
                                                  Row.Width() - 190.0f, 24.0f);

                if (CardComponents.SelectionField(NextCardControl(), Pill,
                                                  SelectionDeclaration{ "", Blends, BlendCount },
                                                  Taken).ReadingAltered && Taken < BlendCount)
                {
                    Revisions.Record(Arrangement, "Channel blend amended");
                    Reading8.Blend = Blends[Taken];
                }

                // 📐 `.mrange` — the channel's own opacity, at the row's trailing edge.
                const PlaneExtent Meter = Spanning(Row.MaximumX - 66.0f, Middle - 9.0f, 60.0f, 18.0f);
                double Current = static_cast<double>(Reading8.Opacity);

                if (CardComponents.MagnitudeRow(NextCardControl(), Meter,
                                                MagnitudeDeclaration{ "", "%", 0.0, 100.0 },
                                                Current, true).ReadingAltered)
                {
                    Revisions.Record(Arrangement, "Channel opacity amended");
                    Reading8.Opacity = static_cast<std::uint32_t>(Current + 0.5);
                }

                Y += 30.0f;
            }
        }

        // ④ Height -> Normal — `sec(n,'h2n',…)`, the re-integration and its note.
        if (RecordCardSection(Extent, "Height -> Normal",
                              Entry.HeightIntegrated ? Entry.HeightBlend : "off",
                              CardSection::Height, Ordinal, Applied, Recording, Y))
        {
            const PlaneExtent OnRow = Spanning(MinimumX, Y, Width, CardRowHeight);

            if (Recording && !Surface->Excluded(OnRow))
            {
                bool Taken = Entry.HeightIntegrated;

                if (CardComponents.ToggleRow(NextCardControl(), OnRow,
                                             ToggleDeclaration{ "Re-integrate height into normal" },
                                             Taken).ReadingAltered)
                {
                    Revisions.Record(Arrangement, "Height integration amended");
                    Entry.HeightIntegrated = Taken;
                }
            }
            else if (Recording)
            {
                (void) NextCardControl();
            }

            Y += CardRowHeight + CardRowGap;

            const PlaneExtent ModeRow = Spanning(MinimumX, Y, Width, CardRowHeight);

            if (Recording && !Surface->Excluded(ModeRow))
            {
                std::uint32_t BlendCount = 0u;
                const char* const* Blends = BlendNaming(3u, BlendCount);
                std::uint32_t Taken = 0u;

                for (std::uint32_t Option = 0u; Option < BlendCount; ++Option)
                {
                    if (std::strcmp(Blends[Option], Entry.HeightBlend) == 0)
                    {
                        Taken = Option;
                        break;
                    }
                }

                if (CardComponents.SelectionField(NextCardControl(),
                                                  RecordCardCaption(ModeRow, "Normal Blend"),
                                                  SelectionDeclaration{ "", Blends, BlendCount },
                                                  Taken).ReadingAltered && Taken < BlendCount)
                {
                    Revisions.Record(Arrangement, "Normal blend amended");
                    Entry.HeightBlend = Blends[Taken];
                }
            }
            else if (Recording)
            {
                (void) NextCardControl();
            }

            Y += CardRowHeight + CardRowGap;

            const PlaneExtent IntensityRow = Spanning(MinimumX, Y, Width, CardRowHeight);

            if (Recording && !Surface->Excluded(IntensityRow))
            {
                double Current = static_cast<double>(Entry.HeightIntensity);

                if (CardComponents.MagnitudeRow(NextCardControl(),
                                                RecordCardCaption(IntensityRow, "Intensity"),
                                                MagnitudeDeclaration{ "", "%", 0.0, 200.0 },
                                                Current, true).ReadingAltered)
                {
                    Revisions.Record(Arrangement, "Height intensity amended");
                    Entry.HeightIntensity = static_cast<std::uint32_t>(Current + 0.5);
                }
            }
            else if (Recording)
            {
                (void) NextCardControl();
            }

            Y += CardRowHeight + CardRowGap;

            const PlaneExtent TessRow = Spanning(MinimumX, Y, Width, CardRowHeight);

            if (Recording && !Surface->Excluded(TessRow))
            {
                bool Taken = Entry.HeightTessellated;

                if (CardComponents.ToggleRow(NextCardControl(), TessRow,
                                             ToggleDeclaration{ "Feed displacement / tessellation" },
                                             Taken).ReadingAltered)
                {
                    Revisions.Record(Arrangement, "Tessellation amended");
                    Entry.HeightTessellated = Taken;
                }
            }
            else if (Recording)
            {
                (void) NextCardControl();
            }

            Y += CardRowHeight + CardRowGap;

            Y = RecordCardNote(Extent, Y,
                                    "This layer's Height is converted to a normal contribution and blended, "
                                    "so the normals incoming from the layers below are re-oriented against "
                                    "this layer's surface instead of being overwritten.", Recording);
        }
    }

    // ⑤ Placement — the decal's and the pattern's own extra section, open by default.
    if (Entry.Placement < Arrangement.PlacementCount &&
        (Entry.Content == LayerContent::Decal || Entry.Content == LayerContent::Pattern))
    {
        PlacementRun& Run     = Arrangement.Placements[Entry.Placement];
        const bool    IsDecal = (Entry.Content == LayerContent::Decal);

        std::uint32_t OptionCount = 0u;
        const char* const* Options = PlacementOptions(Entry.Content, OptionCount);

        if (RecordCardSection(Extent, IsDecal ? "Placement \xC2\xB7 3D Decal" : "Pattern Generator",
                              (Run.ParameterCount > 0u) ? Run.Parameters[0].Selected : nullptr,
                              CardSection::Placement, Ordinal, Applied, Recording, Y))
        {
            Y = RecordParameterRun(Extent, Y, Run.Parameters, Run.ParameterCount,
                                        Options, OptionCount, Arrangement, Revisions, Recording);

            Y = RecordCardNote(Extent, Y, IsDecal
                ? "The decal is a 3D placed entity: it keeps its own transform gizmo in the viewport and "
                  "stays listed here so it can be re-ordered, masked, grouped and channel-filtered like any "
                  "other layer."
                : "Placeholder parameter set - wire your own pattern generator to these keys.", Recording);
        }
    }

    // ⑥ Effects — `sec(n,'fx',…)`, the chips and the add action.
    if (Entry.EffectCount > 0u)
        std::snprintf(Reading, sizeof Reading, "%u active", Entry.EffectCount);

    if (RecordCardSection(Extent, "Effects", (Entry.EffectCount > 0u) ? Reading : "none",
                          CardSection::Effects, Ordinal, Applied, Recording, Y))
    {
        float ChipX = MinimumX;
        float ChipRow   = Y;

        if (Recording)
        {
            for (std::uint32_t Effect = 0u; Effect < Entry.EffectCount; ++Effect)
            {
                const char* Caption = Entry.Effects[Effect];
                const float X   = Surface->MeasureRun(Caption, Scaled.RunFine) + 20.0f;

                if (ChipX + X > Extent.MaximumX - Scaled.CardPadX)
                {
                    ChipX = MinimumX;
                    ChipRow  += Scaled.ChipHeight + 4.0f;
                }

                const PlaneExtent Chip = Spanning(ChipX, ChipRow, X, Scaled.ChipHeight);

                if (!Surface->Excluded(Chip))
                    RecordChip(Chip, Caption, Tinted.Accent, false);

                ChipX += X + 5.0f;
            }

            if (Entry.EffectCount == 0u)
            {
                const PlaneExtent Chip = Spanning(ChipX, ChipRow, 48.0f, Scaled.ChipHeight);
                RecordChip(Chip, "none", Tinted.Faint, false);
            }
        }

        Y = ChipRow + Scaled.ChipHeight + 6.0f;

        const PlaneExtent AddRow = Spanning(MinimumX, Y, Width, CardActionRow);

        if (Recording && !Surface->Excluded(AddRow))
        {
            const PlaneExtent Add = Spanning(MinimumX, Y, 96.0f, CardActionRow);

            if (RecordCardAction(Add, "Add effect", false, false, Applied))
            {
                Arrangement.Taken     = Ordinal;
                Arrangement.TakenHalf = LayerTaken::Layer;
                Applied.Popup          = StackPopup::EffectMenu;
                Applied.PopupSubject   = Ordinal;
                Applied.PopupOnMask    = false;
                Applied.PopupX     = Add.MaximumX;
                Applied.PopupHeight    = Add.MaximumY + 6.0f;
                Applied.PopupOffset    = 0.0f;
                Ledger->Disclose(ChromeCells[static_cast<std::uint32_t>(ChromeCell::PopupBody)]);
            }
        }
        else if (Recording)
        {
            (void) NextCardControl();
        }

        Y += CardActionRow + CardRowGap;
    }

    // ⑦ Colour Tag — `sec(n,'col',…)`, the ten swatches and the custom wheel.
    if (RecordCardSection(Extent, "Colour Tag", nullptr, CardSection::ColourTag, Ordinal, Applied,
                          Recording, Y))
    {
        const PlaneExtent Swatches = Spanning(MinimumX, Y, Width, CardActionRow);

        if (Recording && !Surface->Excluded(Swatches))
        {
            const std::uint32_t* Tags = AppliedColourTags();

            for (std::uint32_t Tag = 0u; Tag < LayerStackCeiling::ColourTags; ++Tag)
            {
                const PlaneExtent Swatch = Squared(MinimumX + 9.0f + static_cast<float>(Tag) * 22.0f,
                                                   Y + CardActionRow * 0.5f, 16.0f);

                Surface->Ground(Swatch, Covering(Tags[Tag]), 8.0f);

                if (Entry.ColourTag == Tags[Tag])
                    Surface->Edge(Swatch, Tinted.Primary, 2.0f, 8.0f);

                if (Pressed(NextCardControl(), Swatch, Applied, nullptr))
                {
                    Revisions.Record(Arrangement, "Colour tag amended");
                    Entry.ColourTag = Tags[Tag];
                }
            }

            const PlaneExtent Custom = Spanning(MinimumX + 9.0f +
                                                static_cast<float>(LayerStackCeiling::ColourTags) * 22.0f,
                                                Y, 74.0f, CardActionRow);

            if (RecordCardAction(Custom, "Custom...", false, false, Applied))
            {
                Arrangement.Taken     = Ordinal;
                Arrangement.TakenHalf = LayerTaken::Layer;
                Applied.Popup          = StackPopup::ColourWheel;
                Applied.PopupSubject   = Ordinal;
                Applied.PopupOnMask    = false;
                Applied.PopupX     = Custom.MaximumX;
                Applied.PopupHeight    = Custom.MaximumY + 6.0f;
                Ledger->Disclose(ChromeCells[static_cast<std::uint32_t>(ChromeCell::PopupBody)]);
            }
        }
        else if (Recording)
        {
            for (std::uint32_t Tag = 0u; Tag <= LayerStackCeiling::ColourTags; ++Tag)
                (void) NextCardControl();
        }

        Y += CardActionRow + CardRowGap;
    }

    // 📐 The trailing mask note, which the reference states only while a mask stands.
    if (Entry.Mask.Declared)
    {
        Y = RecordCardNote(Extent, Y,
                                "This mask is evaluated after the layer's effects and multiplies this "
                                "layer's alpha only - layers below are untouched.", Recording);
    }

    return Y + 6.0f - Extent.MinimumY;
}

float LayerStackPanel::RecordMaskCard(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                                      std::uint32_t Ordinal, LayerStackContext& Applied,
                                      RevisionSequence& Revisions, bool Recording)
{
    LayerEntry&   Entry = Arrangement.Entries[Ordinal];
    MaskCoordinate& Mask  = Entry.Mask;

    const float MinimumX = Extent.MinimumX + Scaled.CardPadX;
    const float Width  = Extent.Width() - Scaled.CardPadX * 2.0f;

    float Y = Extent.MinimumY + 6.0f;
    char  Reading[48] = {};

    // ① Info — `sec(m,'info','Info','',info,false)`.
    if (RecordCardSection(Extent, "Info", nullptr, CardSection::MaskInfo, Ordinal, Applied,
                          Recording, Y))
    {
        const auto Field = [&](const char* Caption, const char* Written)
        {
            const PlaneExtent Row = Spanning(MinimumX, Y, Width, Scaled.FieldHeight);

            if (Recording && !Surface->Excluded(Row))
                RecordReadingRow(Row, Caption, Written);

            Y += Scaled.FieldHeight;
        };

        char Written[48] = {};

        Field("Attached to", Entry.Naming);
        Field("Source", SourceNaming(Mask.Source));
        Field("Generator", (Mask.Source == MaskSource::Generator) ? Mask.Generator : "\xE2\x80\x94");
        Field("Channel", "Grayscale 8-bit");

        std::snprintf(Written, sizeof Written, "%u%%", Mask.Density);
        Field("Density", Written);

        Field("Mask Blend", Mask.Blend);
        Field("Inverted", Mask.Inverted ? "yes" : "no");

        std::snprintf(Written, sizeof Written, "%u \xC3\x97 %u", Mask.Resolution, Mask.Resolution);
        Field("Resolution", Written);

        Field("Effects", (Mask.EffectCount > 0u) ? Mask.Effects[0] : "none");
        Field("Enabled", Mask.Shown ? "yes" : "no");
    }

    // ② Source — `sec(m,'src','Source',…,true)`, open by default.
    if (RecordCardSection(Extent, "Source", SourceNaming(Mask.Source), CardSection::MaskSource,
                          Ordinal, Applied, Recording, Y))
    {
        const PlaneExtent DensityRow = Spanning(MinimumX, Y, Width, CardRowHeight);

        if (Recording && !Surface->Excluded(DensityRow))
        {
            double Current = static_cast<double>(Mask.Density);

            if (CardComponents.MagnitudeRow(NextCardControl(), RecordCardCaption(DensityRow, "Density"),
                                            MagnitudeDeclaration{ "", "%", 0.0, 100.0 },
                                            Current, true).ReadingAltered)
            {
                Revisions.Record(Arrangement, "Mask density amended");
                Mask.Density = static_cast<std::uint32_t>(Current + 0.5);
            }
        }
        else if (Recording)
        {
            (void) NextCardControl();
        }

        Y += CardRowHeight + CardRowGap;

        const PlaneExtent Actions = Spanning(MinimumX, Y, Width, CardActionRow);

        if (Recording && !Surface->Excluded(Actions))
        {
            const float Each = (Width - 6.0f) * 0.5f;

            const PlaneExtent Invert = Spanning(MinimumX, Y, Each, CardActionRow);
            const PlaneExtent Retire = Spanning(MinimumX + Each + 6.0f, Y, Each, CardActionRow);

            if (RecordCardAction(Invert, "Invert", Mask.Inverted, false, Applied))
            {
                Revisions.Record(Arrangement, "Mask inversion amended");
                Mask.Inverted = !Mask.Inverted;
            }

            if (RecordCardAction(Retire, "Delete mask", false, true, Applied))
            {
                Revisions.Record(Arrangement, "Mask retired");
                Mask.Declared = false;
                Mask.Unfolded = false;
            }
        }
        else if (Recording)
        {
            (void) NextCardControl();
            (void) NextCardControl();
        }

        Y += CardActionRow + CardRowGap;
    }

    // ③ Parameters — `sec(m,'p',…,true)`, the source's own run, open by default.
    std::snprintf(Reading, sizeof Reading, "%u params", Mask.ParameterCount);

    if (RecordCardSection(Extent,
                          (Mask.Source == MaskSource::Generator) ? "Generator Parameters"
                                                                 : "Source Parameters",
                          Reading, CardSection::MaskParams, Ordinal, Applied, Recording, Y))
    {
        Y = RecordParameterRun(Extent, Y, Mask.Parameters, Mask.ParameterCount,
                                    nullptr, 0u, Arrangement, Revisions, Recording);
    }

    // ④ Mesh Map Inputs — `sec(m,'maps',…,false)`, the bake chips.
    if (Mask.MeshMapCount > 0u)
        std::snprintf(Reading, sizeof Reading, "%u required", Mask.MeshMapCount);

    if (RecordCardSection(Extent, "Mesh Map Inputs", (Mask.MeshMapCount > 0u) ? Reading : "none",
                          CardSection::MaskMaps, Ordinal, Applied, Recording, Y))
    {
        float ChipX = MinimumX;
        float ChipRow   = Y;

        if (Recording)
        {
            for (std::uint32_t Map = 0u; Map < Mask.MeshMapCount; ++Map)
            {
                const char* Caption = Mask.MeshMaps[Map];
                const float X   = Surface->MeasureRun(Caption, Scaled.RunFine) + 20.0f;

                if (ChipX + X > Extent.MaximumX - Scaled.CardPadX)
                {
                    ChipX = MinimumX;
                    ChipRow  += Scaled.ChipHeight + 4.0f;
                }

                const PlaneExtent Chip = Spanning(ChipX, ChipRow, X, Scaled.ChipHeight);

                if (!Surface->Excluded(Chip))
                    RecordChip(Chip, Caption, Mask.MeshMapTransferred[Map] ? Tinted.Affirm : Tinted.Danger,
                               false);

                ChipX += X + 5.0f;
            }

            if (Mask.MeshMapCount == 0u)
                RecordChip(Spanning(ChipX, ChipRow, 128.0f, Scaled.ChipHeight),
                           "no mesh map required", Tinted.Faint, false);
        }

        Y = ChipRow + Scaled.ChipHeight + 6.0f;

        Y = RecordCardNote(Extent, Y,
                                "Generators read baked mesh maps. Missing maps are shown in red - bake them "
                                "to make the generator resolve correctly.", Recording);
    }

    // ⑤ Applies To Channels — `sec(m,'chan',…,false)`, the eight toggle chips.
    std::uint32_t AppliedChannels = 0u;

    for (std::uint32_t Channel = 0u; Channel < LayerStackCeiling::Channels; ++Channel)
        if (Mask.ChannelApplied[Channel])
            ++AppliedChannels;

    std::snprintf(Reading, sizeof Reading, "%u / %u", Applied, LayerStackCeiling::Channels);

    if (RecordCardSection(Extent, "Applies To Channels", Reading, CardSection::MaskApplies, Ordinal,
                          Applied, Recording, Y))
    {
        float ChipX = MinimumX;
        float ChipRow   = Y;

        for (std::uint32_t Channel = 0u; Channel < LayerStackCeiling::Channels; ++Channel)
        {
            const char* Caption = ChannelNaming()[Channel];
            const float X   = (Surface != nullptr)
                                ? Surface->MeasureRun(Caption, Scaled.RunFine) + 20.0f : 60.0f;

            if (ChipX + X > Extent.MaximumX - Scaled.CardPadX)
            {
                ChipX = MinimumX;
                ChipRow  += Scaled.ChipHeight + 4.0f;
            }

            const PlaneExtent Chip = Spanning(ChipX, ChipRow, X, Scaled.ChipHeight);

            if (Recording && !Surface->Excluded(Chip))
            {
                const bool Current = Mask.ChannelApplied[Channel];

                RecordChip(Chip, Caption, Current ? ChannelTint(Channel) : Tinted.Faint, Current);

                if (Pressed(NextCardControl(), Chip, Applied, nullptr))
                {
                    Revisions.Record(Arrangement, "Mask channels amended");
                    Mask.ChannelApplied[Channel] = !Current;
                }
            }
            else if (Recording)
            {
                (void) NextCardControl();
            }

            ChipX += X + 5.0f;
        }

        Y = ChipRow + Scaled.ChipHeight + 6.0f;

        Y = RecordCardNote(Extent, Y,
                                "Channel-aware masking: unticked channels ignore this mask entirely. Useful "
                                "when a generator should carve Height and Roughness but leave Base Color "
                                "intact.", Recording);
    }

    // ⑥ Mask Effects — `sec(m,'fx',…,false)`.
    if (Mask.EffectCount > 0u)
        std::snprintf(Reading, sizeof Reading, "%u active", Mask.EffectCount);

    if (RecordCardSection(Extent, "Mask Effects", (Mask.EffectCount > 0u) ? Reading : "none",
                          CardSection::MaskEffects, Ordinal, Applied, Recording, Y))
    {
        float ChipX = MinimumX;
        float ChipRow   = Y;

        if (Recording)
        {
            for (std::uint32_t Effect = 0u; Effect < Mask.EffectCount; ++Effect)
            {
                const char* Caption = Mask.Effects[Effect];
                const float X   = Surface->MeasureRun(Caption, Scaled.RunFine) + 20.0f;

                if (ChipX + X > Extent.MaximumX - Scaled.CardPadX)
                {
                    ChipX = MinimumX;
                    ChipRow  += Scaled.ChipHeight + 4.0f;
                }

                const PlaneExtent Chip = Spanning(ChipX, ChipRow, X, Scaled.ChipHeight);

                if (!Surface->Excluded(Chip))
                    RecordChip(Chip, Caption, Tinted.Accent, false);

                ChipX += X + 5.0f;
            }

            if (Mask.EffectCount == 0u)
                RecordChip(Spanning(ChipX, ChipRow, 48.0f, Scaled.ChipHeight), "none",
                           Tinted.Faint, false);
        }

        Y = ChipRow + Scaled.ChipHeight + 6.0f;
    }

    // 📐 The evaluation-order note the reference trails every mask card with.
    Y = RecordCardNote(Extent, Y,
                            "Evaluation order for this entry: source -> mask effects -> invert / density -> "
                            "mask multiplies the layer's alpha per channel -> layer opacity -> channel blend "
                            "into the layers below.", Recording);

    return Y + 6.0f - Extent.MinimumY;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                          ONE ROW
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordEntryRow(const PlaneExtent& Extent, const LayerArrangement& Arrangement,
                                     std::uint32_t Ordinal, bool Taken, bool HoveredOrdinal)
{
    const LayerEntry& Entry  = Arrangement.Entries[Ordinal];
    const bool        Folder = (Entry.Content == LayerContent::Folder);
    const float       Middle = (Extent.MinimumY + Extent.MaximumY) * 0.5f;

    // ① The ground, which the reference tints by taken, then hovered, then standing.
    // 📐 `/* ── ROW (square) ── */` — the entry row carries no radius at all.
    const ThemeToken Ground = Taken ? Tinted.RowTaken : (HoveredOrdinal ? Tinted.RowHovered : Tinted.Row);
    Surface->Ground(Extent, Ground, 0.0f);

    if (Taken)
        Surface->Edge(Extent, Tinted.StrokeStrong, 1.0f, 0.0f);

    // ② `.tag` — the colour tag, a 3px spine down the leading edge.
    PlaneExtent Tag = Extent;
    Tag.MaximumX   = Extent.MinimumX + Scaled.TagX;
    Surface->Ground(Tag, Covering(Entry.ColourTag), Scaled.TagX * 0.5f);

    float X = Extent.MinimumX + Scaled.RowPadX;

    // ③ `.tw` — the disclosure twisty, drawn only on a folder.
    if (Folder)
        Surface->Stroke(Entry.Opened ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                        Squared(X + Scaled.DiscloseX * 0.5f, Middle, 12.0f), Tinted.Secondary);

    X += Scaled.DiscloseX + Scaled.RowGapX * 0.5f;

    // ④ The eye, dimmed while the entry is hidden.
    Surface->Stroke(Entry.Shown ? SymbolSubject::EyeOpen : SymbolSubject::EyeClosed,
                    Squared(X + Scaled.ActionExtent * 0.5f, Middle, 12.5f),
                    Entry.Shown ? Tinted.Secondary : Tinted.Faint);

    X += Scaled.ActionExtent + Scaled.RowGapX;

    // ⑤ `.thumb` — the preview disc and its classification badge.
    // 📐 `.disc` states `border-radius:0` and a 1px inset ring; the fill is the whole square.
    const PlaneExtent Thumb = Squared(X + Scaled.ThumbExtent * 0.5f, Middle, Scaled.ThumbExtent);
    Surface->Ground(Thumb, ContentTint(Entry.Content), 0.0f);
    Surface->Edge(Thumb, Tinted.StrokeStrong, 1.0f, 0.0f);

    // 📐 `.badge` — 15px square hung 3px past the trailing and lower edges, on --r-s.
    const PlaneExtent Badge = Spanning(Thumb.MaximumX - Scaled.BadgeExtent + 3.0f,
                                       Thumb.MaximumY - Scaled.BadgeExtent + 3.0f,
                                       Scaled.BadgeExtent, Scaled.BadgeExtent);
    Surface->Ground(Badge, Covering(0x000000u), Scaled.RadiusSmall);
    Surface->Edge(Badge, Tinted.StrokeStrong, 1.0f, Scaled.RadiusSmall);

    const char* const BadgeRun = ContentBadge(Entry.Content);
    Surface->TextRun(Badge.MinimumX + (Scaled.BadgeExtent - Surface->MeasureRun(BadgeRun, 9.0f)) * 0.5f,
                     Badge.MinimumY + (Scaled.BadgeExtent - Surface->LineHeight(9.0f)) * 0.5f,
                     Tinted.Secondary, BadgeRun, 9.0f, 0.0f, true);

    X += Scaled.ThumbExtent + Scaled.RowGapX;

    // ⑥ `.chips` — measured before the meta, because `.meta{flex:1}` yields to whatever the chips take.
    struct ChipDeclaration { char Caption[16]; ThemeToken Colour; bool Solid; };
    ChipDeclaration Declared[5] = {};
    std::uint32_t   ChipCount   = 0u;

    const auto Declare = [&](const char* Caption, ThemeToken Colour, bool Solid)
    {
        if (ChipCount >= 5u)
            return;
        std::snprintf(Declared[ChipCount].Caption, sizeof Declared[ChipCount].Caption, "%s", Caption);
        Declared[ChipCount].Colour   = Colour;
        Declared[ChipCount].Solid = Solid;
        ++ChipCount;
    };

    // 📝 The reference orders them 3D, L, MASK, N FX, n/8 CH, leading to trailing.
    if (Entry.Content == LayerContent::Decal)
        Declare("3D", Tinted.Accent, true);

    if (Entry.Secured)
        Declare("L", Tinted.Danger, false);

    if (Entry.Mask.Declared)
        Declare("MASK", Tinted.Caution, false);

    if (Entry.EffectCount > 0u)
    {
        char Counted[16] = {};
        std::snprintf(Counted, sizeof Counted, "%u FX", Entry.EffectCount);
        Declare(Counted, Tinted.Affirm, false);
    }

    if (!Folder)
    {
        const std::uint32_t Enabled = ChannelsEnabled(Entry);

        if (Enabled != LayerStackCeiling::Channels)
        {
            char Counted[16] = {};
            std::snprintf(Counted, sizeof Counted, "%u/8 CH", Enabled);
            Declare(Counted, Tinted.Secondary, false);
        }
    }

    float ChipsX = 0.0f;

    for (std::uint32_t Chip = 0u; Chip < ChipCount; ++Chip)
        ChipsX += Surface->MeasureRun(Declared[Chip].Caption, Scaled.RunFine, 0.04f) + 14.0f + 3.0f;

    // ⑦ `body.wide .col` — the two columns seat only once the panel reaches 580px.
    const bool  Columns     = Extent.Width() >= Scaled.ColumnsTop;
    const float ColumnsSpan = Columns ? Scaled.BlendColumnX + Scaled.OpacityColumnX : 0.0f;
    const float ChipsY   = Extent.MaximumX - Scaled.ActionExtent * 2.0f - ChipsX;
    const float ColumnsY = ChipsY - ColumnsSpan;

    // ⑧ `.meta` — the naming over its reading line, both truncated to whatever extent is left.
    const float MetaCeiling = ColumnsY - X - Scaled.RowGapX;

    if (MetaCeiling > 8.0f)
    {
        Surface->TextRunTruncated(X, Middle - Surface->LineHeight(Scaled.RunRow) - 1.0f, MetaCeiling,
                                  Entry.Shown ? Tinted.Primary : Tinted.Faint, Entry.Naming,
                                  Scaled.RunRow, true);

        char Reading[96] = {};

        // 📝 `rowSub` reads the resolution and format once the columns carry blend and opacity instead.
        if (Folder)
            std::snprintf(Reading, sizeof Reading, "%u items  %s",
                          EnclosedCount(Arrangement, Ordinal), Entry.Blend);
        else if (Columns)
            std::snprintf(Reading, sizeof Reading, "%s  %upx  %s",
                          ContentNaming(Entry.Content), Entry.Resolution, Entry.Format);
        else
            std::snprintf(Reading, sizeof Reading, "%s  %s  %u%%",
                          ContentNaming(Entry.Content), Entry.Blend, Entry.Opacity);

        Surface->TextRunTruncated(X, Middle + 1.0f, MetaCeiling, Tinted.Faint, Reading, Scaled.RunSub);
    }

    if (Columns)
    {
        // ⑨ `.col-blend` — an em dash on a folder, its blend otherwise.
        Surface->TextRunTruncated(ColumnsY, Middle - Surface->LineHeight(Scaled.RunSub) * 0.5f,
                                  Scaled.BlendColumnX - 6.0f, Tinted.Secondary,
                                  Folder ? "\xE2\x80\x94" : Entry.Blend, Scaled.RunSub);

        // ⑩ `.col-op` — the meter takes the slack, the reading its stated 32px on the trailing edge.
        const float MeterY  = ColumnsY + Scaled.BlendColumnX;
        const float MeterX = Scaled.OpacityColumnX - Scaled.OpacityReadX - Scaled.ColumnGapX;
        RecordMeter(Spanning(MeterY, Middle - Scaled.MiniY * 0.5f, MeterX, Scaled.MiniY),
                    Entry.Opacity, Tinted.Accent);

        char Percent[8] = {};
        std::snprintf(Percent, sizeof Percent, "%u%%", Entry.Opacity);
        Surface->TextRun(MeterY + Scaled.OpacityColumnX - Surface->MeasureRun(Percent, Scaled.RunSub),
                         Middle - Surface->LineHeight(Scaled.RunSub) * 0.5f,
                         Tinted.Secondary, Percent, Scaled.RunSub);
    }

    // ⑪ The chips themselves, applied leading to trailing across the run just measured.
    float ChipY = ChipsY;

    for (std::uint32_t Chip = 0u; Chip < ChipCount; ++Chip)
    {
        const float ChipX = Surface->MeasureRun(Declared[Chip].Caption, Scaled.RunFine, 0.04f) + 14.0f;
        RecordChip(Spanning(ChipY, Middle - Scaled.ChipHeight * 0.5f, ChipX, Scaled.ChipHeight),
                   Declared[Chip].Caption, Declared[Chip].Colour, Declared[Chip].Solid);
        ChipY += ChipX + 3.0f;
    }

    // ⑩ The unfold chevron and the menu, both on the trailing edge.
    Surface->Stroke(Entry.Unfolded ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                    Squared(Extent.MaximumX - Scaled.ActionExtent * 1.5f, Middle, 12.0f), Tinted.Faint);

    Surface->Medallion(Extent.MaximumX - Scaled.ActionExtent * 0.5f, Middle - 4.0f, 1.3f, Tinted.Faint);
    Surface->Medallion(Extent.MaximumX - Scaled.ActionExtent * 0.5f, Middle,        1.3f, Tinted.Faint);
    Surface->Medallion(Extent.MaximumX - Scaled.ActionExtent * 0.5f, Middle + 4.0f, 1.3f, Tinted.Faint);
}

void LayerStackPanel::RecordMaskRow(const PlaneExtent& Extent, const LayerEntry& Entry, bool Taken, bool HoveredOrdinal)
{
    // 📐 `.row.msk` — attached beneath its entry, indented, shorter, and without a colour tag or twisty.
    const MaskCoordinate& Mask = Entry.Mask;
    const float      Middle = (Extent.MinimumY + Extent.MaximumY) * 0.5f;

    const ThemeToken Ground = Taken ? Tinted.RowTaken : (HoveredOrdinal ? Tinted.RowHovered : Tinted.Detail);
    Surface->Ground(Extent, Ground, 0.0f);
    Surface->Edge(Extent, Taken ? Tinted.StrokeStrong : Tinted.Stroke, 1.0f, 0.0f);

    float X = Extent.MinimumX + Scaled.RowPadX;

    Surface->Stroke(Mask.Shown ? SymbolSubject::EyeOpen : SymbolSubject::EyeClosed,
                    Squared(X + Scaled.ActionExtent * 0.5f, Middle, 11.5f),
                    Mask.Shown ? Tinted.Secondary : Tinted.Faint);

    X += Scaled.ActionExtent + Scaled.RowGapX;

    // 📐 The mini preview, which the reference inverts in place when the mask is inverted.
    const PlaneExtent Thumb = Squared(X + Scaled.ThumbMini * 0.5f, Middle, Scaled.ThumbMini);
    Surface->Ground(Thumb, Mask.Inverted ? Covering(0x2A2A2Au) : Covering(0xC8C8C8u), 0.0f);
    Surface->Edge(Thumb, Tinted.StrokeStrong, 1.0f, 0.0f);

    X += Scaled.ThumbMini + Scaled.RowGapX;

    Surface->TextRun(X, Middle - Surface->LineHeight(Scaled.RunSub) - 1.0f, Tinted.Secondary,
                     "Mask", Scaled.RunSub, 0.0f, true);

    char Reading[96] = {};
    const char* Source = (Mask.Source == MaskSource::Generator && Mask.Generator != nullptr)
                       ? Mask.Generator : SourceNaming(Mask.Source);

    if (Mask.Inverted)
        std::snprintf(Reading, sizeof Reading, "%s  Gray 8  %u%%  INV", Source, Mask.Density);
    else
        std::snprintf(Reading, sizeof Reading, "%s  Gray 8  %u%%", Source, Mask.Density);

    // 📐 `.chips` — the mask row declares at most the effect count.
    char  Counted[16] = {};
    float ChipsX  = 0.0f;

    if (Mask.EffectCount > 0u)
    {
        std::snprintf(Counted, sizeof Counted, "%u FX", Mask.EffectCount);
        ChipsX = Surface->MeasureRun(Counted, Scaled.RunFine, 0.04f) + 14.0f + 3.0f;
    }

    const bool  Columns     = Extent.Width() >= Scaled.ColumnsTop;
    const float ColumnsSpan = Columns ? Scaled.BlendColumnX + Scaled.OpacityColumnX : 0.0f;
    const float ChipsY   = Extent.MaximumX - Scaled.ActionExtent * 2.0f - ChipsX;
    const float ColumnsY = ChipsY - ColumnsSpan;
    const float MetaCeiling = ColumnsY - X - Scaled.RowGapX;

    if (MetaCeiling > 8.0f)
        Surface->TextRunTruncated(X, Middle + 1.0f, MetaCeiling, Tinted.Faint, Reading, Scaled.RunFine);

    if (Columns)
    {
        // 📐 `clips <naming>` in the blend column, truncated at fourteen characters by the reference.
        char Clips[48] = {};
        std::snprintf(Clips, sizeof Clips, "clips %.14s", Entry.Naming);
        Surface->TextRunTruncated(ColumnsY, Middle - Surface->LineHeight(Scaled.RunFine) * 0.5f,
                                  Scaled.BlendColumnX - 6.0f, Tinted.Faint, Clips, Scaled.RunFine);

        const float MeterY  = ColumnsY + Scaled.BlendColumnX;
        const float MeterX = Scaled.OpacityColumnX - Scaled.OpacityReadX - Scaled.ColumnGapX;
        RecordMeter(Spanning(MeterY, Middle - Scaled.MiniY * 0.5f, MeterX, Scaled.MiniY),
                    Mask.Density, Tinted.Secondary);

        char Percent[8] = {};
        std::snprintf(Percent, sizeof Percent, "%u%%", Mask.Density);
        Surface->TextRun(MeterY + Scaled.OpacityColumnX - Surface->MeasureRun(Percent, Scaled.RunSub),
                         Middle - Surface->LineHeight(Scaled.RunSub) * 0.5f,
                         Tinted.Secondary, Percent, Scaled.RunSub);
    }

    if (Mask.EffectCount > 0u)
        RecordChip(Spanning(ChipsY, Middle - Scaled.ChipHeight * 0.5f, ChipsX - 3.0f, Scaled.ChipHeight),
                   Counted, Tinted.Affirm, false);

    Surface->Stroke(Mask.Unfolded ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                    Squared(Extent.MaximumX - Scaled.ActionExtent * 1.5f, Middle, 11.0f), Tinted.Faint);

    Surface->Medallion(Extent.MaximumX - Scaled.ActionExtent * 0.5f, Middle - 3.5f, 1.2f, Tinted.Faint);
    Surface->Medallion(Extent.MaximumX - Scaled.ActionExtent * 0.5f, Middle,        1.2f, Tinted.Faint);
    Surface->Medallion(Extent.MaximumX - Scaled.ActionExtent * 0.5f, Middle + 3.5f, 1.2f, Tinted.Faint);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DROP MARK
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordDropMark(const PlaneExtent& Extent, DropIntent Intent)
{
    // 📐 `.drop-before` and `.drop-after` state a 2px accent rule; `.drop-into` states a ring around the
    //    whole folder row, which is what makes the two readings unmistakable at a glance.
    switch (Intent)
    {
        case DropIntent::Prior:
            Surface->Ground(PlaneExtent{ Extent.MinimumX, Extent.MinimumY - 1.0f,
                                         Extent.MaximumX,  Extent.MinimumY + 1.0f }, Tinted.Accent);
            break;

        case DropIntent::Trailing:
            Surface->Ground(PlaneExtent{ Extent.MinimumX, Extent.MaximumY - 1.0f,
                                         Extent.MaximumX,  Extent.MaximumY + 1.0f }, Tinted.Accent);
            break;

        case DropIntent::Enclosed:
            Surface->Edge(Extent, Tinted.Accent, 2.0f, Scaled.RadiusSmall);
            break;

        default:
            break;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE POPUP SHELL
//------------------------------------------------------------------------------------------------------------------------

// 📐 `.pop` — a 200px raised card on --r, strokes at .18, and 6px of padding.
static constexpr float PopupLeft  = 208.0f;   // [px]
static constexpr float PopupEntryY =  26.0f;   // [px] - one entry
static constexpr float PopupPad         =   6.0f;   // [px]
static constexpr float PopupCaption     =  22.0f;   // [px] - one `<h6>`

PlaneExtent LayerStackPanel::RecordPopupGround(LayerStackContext& Applied, float X, float Y,
                                               float Span)
{
    const DisplayCondition& Display = Surface->Display();

    // 📐 `show()` applies the card against its anchor and then clamps it inside the display on both axes,
    //    flipping it above the anchor when it would otherwise run past the lower edge.
    const float Extent  = Display.Width  > 0.0f ? Display.Width  : 1920.0f;
    const float Y0 = Display.Height > 0.0f ? Display.Height : 1080.0f;

    float ClampedX = X - PopupLeft;

    if (ClampedX < 8.0f)                            ClampedX = 8.0f;
    if (ClampedX > Extent - PopupLeft - 8.0f) ClampedX = Extent - PopupLeft - 8.0f;

    // 📐 `if(y+p.height>innerHeight-8)y=Math.max(8,at.top-p.height-6)` — flipped against the card's OWN
    //    measured height and not against a fixed probe, which left a long run hanging off the lower edge.
    float Upper = Y;

    if (Upper + Span > Y0 - 8.0f)
        Upper = (Y0 - 8.0f - Span > 8.0f) ? (Y0 - 8.0f - Span) : 8.0f;

    Applied.PopupX  = ClampedX;
    Applied.PopupY = Upper;

    return PlaneExtent{ ClampedX, Upper, ClampedX + PopupLeft, Upper };
}

bool LayerStackPanel::RecordPopupEntry(const PlaneExtent& Extent, const char* Caption, const char* Chord,
                                       bool Marked, bool Dangerous, LayerStackContext& Applied)
{
    if (Surface->Excluded(Extent))
        return false;

    // 📝 A popup entry is arbitrated against the standing disclosure rather than through `Pressed`, which
    //    refuses everything while a popup is open — that refusal is exactly what keeps rows underneath a
    //    menu from answering, and the menu's own entries have to sit on the other side of it.
    const bool Over = Extent.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Over)
        Surface->Ground(Extent, Partial(0xFFFFFFu, 0.07), Scaled.RadiusSmall);

    const float Middle = (Extent.MinimumY + Extent.MaximumY) * 0.5f;

    // 📐 The check occupies its 14px cell whether or not it is drawn, so the captions align down the run.
    if (Marked)
        Surface->Stroke(SymbolSubject::ChevronRight, Squared(Extent.MinimumX + 15.0f, Middle, 10.0f),
                        Tinted.Accent);

    Surface->TextRunTruncated(Extent.MinimumX + 26.0f, Middle - Surface->LineHeight(Scaled.RunSub) * 0.5f,
                              Extent.Width() - 34.0f - (Chord != nullptr ? 30.0f : 0.0f),
                              Dangerous ? Tinted.Danger : Tinted.Primary, Caption, Scaled.RunSub);

    if (Chord != nullptr && Chord[0] != '\0')
    {
        Surface->TextRun(Extent.MaximumX - 8.0f - Surface->MeasureRun(Chord, Scaled.RunFine),
                         Middle - Surface->LineHeight(Scaled.RunFine) * 0.5f, Tinted.Faint,
                         Chord, Scaled.RunFine);
    }

    // 📐 A popup resolves on the RELEASE, and only once it has stood for a whole tick — the contact that
    //    opened it is itself a release, and would otherwise pick whatever entry landed under the pointer.
    const bool Taken = Over && Sampled.ContactReleased && Applied.PopupSettled;

    if (Taken)
    {
        Applied.Popup = StackPopup::Absent;

        if (Ledger != nullptr)
            Ledger->Withdraw();
    }

    return Taken;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE CHORDS
//------------------------------------------------------------------------------------------------------------------------

bool LayerStackPanel::AcceptChord(KeySubject Subject, const ModifierCondition& Modifiers,
                                 LayerArrangement& Arrangement, LayerStackContext& Applied,
                                 RevisionSequence& Revisions)
{
    // 🔴 The reference's own first line — `if(e.target.tagName==='INPUT'||…)return`. A chord that reached
    //    the arrangement while the artist was typing would declare a paint layer out of the letter `p`.
    //    The revision card's comment and value fields are `INPUT` and `TEXTAREA` on exactly those grounds.
    if (Applied.RetentionHovered || Applied.Renaming != LayerStackCeiling::AbsentOrdinal ||
        Applied.RevisionField != 0u)
    {
        return false;
    }

    const bool Taken = Arrangement.Taken < Arrangement.EntryCount;

    // 📐 Every amendment records its own revision FIRST, exactly as `snap()` is called ahead of the
    //    mutation and never after it.
    const auto Amend = [&](const char* Naming) { Revisions.Record(Arrangement, Naming); };

    if (Modifiers.Commanded)
    {
        switch (Subject)
        {
            // ① The two revision chords, the only ones that READ the ring rather than record into it.
            case KeySubject::Revert:
                if (Modifiers.Shifted)
                    Revisions.Reinstate(Arrangement);
                else
                    Revisions.Revert(Arrangement);

                return true;

            case KeySubject::DeclareDecal:                      // 📐 ⌘D — duplicate, not "declare a decal"
                if (!Taken) return true;
                Amend("Entry copied");
                if (!DuplicateTaken(Arrangement)) Revisions.Revert(Arrangement);
                return true;

            case KeySubject::DeclareFolder:                     // 📐 ⌘G — group
                if (!Taken) return true;
                Amend("Entries grouped");
                if (!EncloseTaken(Arrangement)) Revisions.Revert(Arrangement);
                return true;

            case KeySubject::StepPrior:                         // 📐 ⌘↑ — carry, not step
                if (!Taken) return true;
                Amend("Entry carried");
                if (!CarryTaken(Arrangement, false)) Revisions.Revert(Arrangement);
                return true;

            case KeySubject::StepNext:                          // 📐 ⌘↓
                if (!Taken) return true;
                Amend("Entry carried");
                if (!CarryTaken(Arrangement, true)) Revisions.Revert(Arrangement);
                return true;

            default:
                // 📐 `else if(m)return` — a commanded chord this panel does not answer is swallowed rather
                //    than falling through to its unmodified reading, which would declare a layer from ⌘P.
                return false;
        }
    }

    switch (Subject)
    {
        // ② The seven declarations.
        case KeySubject::DeclarePaint:
            Amend("Paint layer declared");
            DeclareEntry(Arrangement, LayerContent::Paint, "Paint Layer");
            return true;

        case KeySubject::DeclareFill:
            Amend("Fill layer declared");
            DeclareEntry(Arrangement, LayerContent::Fill, "Fill Layer");
            return true;

        case KeySubject::DeclareAdjustment:
            Amend("Adjustment declared");
            DeclareEntry(Arrangement, LayerContent::Adjustment, "Adjustment");
            return true;

        case KeySubject::DeclareRetention:
            Amend("Retention declared");
            DeclareEntry(Arrangement, LayerContent::Retention, "Filter");
            return true;

        case KeySubject::DeclareDecal:
            Amend("Decal layer declared");
            DeclareEntry(Arrangement, LayerContent::Decal, "Decal Layer");
            return true;

        case KeySubject::DeclarePattern:
            Amend("Pattern layer declared");
            DeclareEntry(Arrangement, LayerContent::Pattern, "Pattern Layer");
            return true;

        case KeySubject::DeclareFolder:
            Amend("Folder declared");
            DeclareEntry(Arrangement, LayerContent::Folder, "New Folder");
            return true;

        // ③ What one taken entry answers.
        case KeySubject::Retire:
            if (!Taken) return true;
            Amend("Entry retired");
            if (!RetireTaken(Arrangement)) Revisions.Revert(Arrangement);
            return true;

        case KeySubject::AttachMask:
            if (!Taken) return true;
            Amend("Mask amended");
            if (!ToggleMask(Arrangement)) Revisions.Revert(Arrangement);
            return true;

        case KeySubject::Secure:
            if (!Taken) return true;
            Amend("Entry secured");
            Arrangement.Entries[Arrangement.Taken].Secured =
                !Arrangement.Entries[Arrangement.Taken].Secured;
            return true;

        case KeySubject::Conceal:
            if (!Taken) return true;
            Amend("Presence amended");
            Arrangement.Entries[Arrangement.Taken].Shown =
                !Arrangement.Entries[Arrangement.Taken].Shown;
            return true;

        // 📐 Soloing and unfolding are interaction, not content, so neither records a revision — the
        //    reference does not `snap()` on either.
        case KeySubject::Solo:
            if (!Taken) return true;
            Arrangement.Soloed = (Arrangement.Soloed == Arrangement.Taken)
                               ? LayerStackCeiling::AbsentOrdinal : Arrangement.Taken;
            return true;

        case KeySubject::Unfold:
        {
            if (!Taken) return true;

            LayerEntry& Entry = Arrangement.Entries[Arrangement.Taken];

            if (Arrangement.TakenHalf == LayerTaken::Mask && Entry.Mask.Declared)
                Entry.Mask.Unfolded = !Entry.Mask.Unfolded;
            else
                Entry.Unfolded = !Entry.Unfolded;

            return true;
        }

        case KeySubject::Rename:
            if (!Taken || Arrangement.TakenHalf == LayerTaken::Mask) return true;
            Applied.Renaming = Arrangement.Taken;
            std::snprintf(Applied.RenamingRun, sizeof Applied.RenamingRun, "%s",
                          Arrangement.Entries[Arrangement.Taken].Naming);
            return true;

        case KeySubject::Seek:
            Applied.RetentionHovered = true;
            return true;

        // ④ Folder disclosure, which the two horizontal arrows drive.
        case KeySubject::Disclose:
        case KeySubject::Withhold:
        {
            if (!Taken) return true;

            LayerEntry& Entry = Arrangement.Entries[Arrangement.Taken];

            if (Entry.Content != LayerContent::Folder)
                return true;

            Entry.Opened = (Subject == KeySubject::Disclose);
            return true;
        }

        // ⑤ Stepping through the presented run, which is what makes the arrows read as a walk.
        case KeySubject::StepPrior:
        case KeySubject::StepNext:
        {
            CurrentHalf Halves[LayerStackCeiling::Entries * 2u];
            const std::uint32_t Count = CurrentHalves(Arrangement, Applied.Retention, Halves,
                                                        LayerStackCeiling::Entries * 2u);

            if (Count == 0u)
                return true;

            std::uint32_t Current = 0u;

            for (std::uint32_t Walk = 0u; Walk < Count; ++Walk)
                if (Halves[Walk].Ordinal == Arrangement.Taken && Halves[Walk].Half == Arrangement.TakenHalf)
                {
                    Current = Walk;
                    break;
                }

            // 📐 The walk stops at both ends rather than wrapping, exactly as `flat[i+d]` yields nothing.
            if (Subject == KeySubject::StepPrior && Current == 0u)
                return true;

            if (Subject == KeySubject::StepNext && Current + 1u >= Count)
                return true;

            const std::uint32_t Stepped = (Subject == KeySubject::StepNext) ? Current + 1u : Current - 1u;

            Arrangement.Taken     = Halves[Stepped].Ordinal;
            Arrangement.TakenHalf = Halves[Stepped].Half;
            return true;
        }

        case KeySubject::Withdraw:
            // 📐 Escape closes the popup first and clears the search run second.
            if (Applied.Popup != StackPopup::Absent)
            {
                Applied.Popup = StackPopup::Absent;
                if (Ledger != nullptr) Ledger->Withdraw();
                return true;
            }

            Applied.Retention[0] = '\0';
            return true;

        default:
            return false;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE STACK
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordStack(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                                  LayerStackContext& Applied, RevisionSequence& Revisions)
{
    if (Surface == nullptr || Ledger == nullptr || !Surface->Recording())
        return;

    // 📝 The tooltip is resolved fresh every tick. A retained one outlives the control it named and hangs
    //    over the panel after the pointer has left it.
    Applied.Tooltip = nullptr;

    // 🔴 The shared card run is handed out from its beginning on every tick. Left to accumulate it would
    //    be spent within a few seconds and every card control after that would draw with an unresolved
    //    identity — a card that records perfectly and answers no contact at all.
    CardControlsSpent = 0u;

    const double Elapsed = Surface->Display().Elapsed;

    // 📐 `secOpen(o,key,def)` — every card opens on the sections the reference's own `def` arguments state,
    //    seeded once so a card the artist has folded stays folded.
    if (!Applied.SectionsSeeded)
    {
        for (std::uint32_t Ordinal = 0u; Ordinal < LayerStackCeiling::Entries; ++Ordinal)
            Applied.Sections[Ordinal] = AppliedSections;

        Applied.SectionsSeeded = true;
    }

    // 🔴 The staged card is cleared at the HEAD of the tick that follows the one which staged it, so the
    //    card renders closed for exactly one frame and departs on the next — the reference's `pendingOpen`
    //    set, which it clears at the top of its own `render`.
    const std::uint32_t Staging = Applied.CardPending;
    Applied.CardPending = LayerStackCeiling::AbsentOrdinal;

    Surface->Ground(Extent, Tinted.Panel);
    Surface->Confine(Extent);

    // ① `.head` — the caption and the entry count.
    const PlaneExtent Head = Spanning(Extent.MinimumX, Extent.MinimumY, Extent.Width(), Scaled.HeadHeight);
    Surface->Ground(Head, Tinted.PanelRaised);
    Surface->Edge(PlaneExtent{ Head.MinimumX, Head.MaximumY - 1.0f, Head.MaximumX, Head.MaximumY },
                  Tinted.Stroke);

    Surface->TextRunCapitalised(Head.MinimumX + Scaled.HeadPadX,
                                Head.MinimumY + (Scaled.HeadHeight - Surface->LineHeight(Scaled.RunHead)) * 0.5f,
                                Tinted.Secondary, "Layers", Scaled.RunHead, TrackingHead, true);

    // 📐 `$('#count').textContent=count()+' · '+maskCount()+'m'` — entries and masks, in one chip.
    std::uint32_t MaskCount = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < Arrangement.EntryCount; ++Ordinal)
        if (Arrangement.Entries[Ordinal].Mask.Declared)
            ++MaskCount;

    char Counted[24] = {};
    std::snprintf(Counted, sizeof Counted, "%u \xC2\xB7 %um", Arrangement.EntryCount, MaskCount);

    const float CountX = Surface->MeasureRun(Counted, Scaled.RunFine) + 18.0f;
    RecordChip(Spanning(Head.MaximumX - Scaled.HeadPadX - CountX,
                        Head.MinimumY + (Scaled.HeadHeight - 20.0f) * 0.5f, CountX, 20.0f),
               Counted, Tinted.Secondary, false);

    // ② `.tools` — the search field and its trailing actions.
    const PlaneExtent Tools = Spanning(Extent.MinimumX, Head.MaximumY, Extent.Width(), Scaled.ToolsY);
    Surface->Edge(PlaneExtent{ Tools.MinimumX, Tools.MaximumY - 1.0f, Tools.MaximumX, Tools.MaximumY },
                  Tinted.Stroke);

    const float       ActionsX = Scaled.ButtonExtent * 3.0f + 10.0f;
    const PlaneExtent Search       = Spanning(Tools.MinimumX + Scaled.ToolsPadX,
                                              Tools.MinimumY + (Scaled.ToolsY - Scaled.SearchHeight) * 0.5f,
                                              Tools.Width() - Scaled.ToolsPadX * 2.0f - ActionsX,
                                              Scaled.SearchHeight);

    // 📐 `#q` — the field is a PRIMITIVE and not a vendor widget, so it carries its own hover and its own
    //    caret. A contact inside it takes the keyboard; a contact anywhere else gives it back.
    if (Pressed(ChromeCells[static_cast<std::uint32_t>(ChromeCell::SearchField)], Search, Applied,
                "Search layers"))
    {
        Applied.RetentionHovered = true;
    }
    else if (Sampled.ContactPressed && !Hovered(Search))
    {
        Applied.RetentionHovered = false;
    }

    Surface->Ground(Search, Covering(0x000000u), Scaled.SearchHeight * 0.5f);
    Surface->Edge(Search, Applied.RetentionHovered ? Tinted.StrokeStrong : Tinted.Stroke, 1.0f,
                  Scaled.SearchHeight * 0.5f);
    Surface->Stroke(SymbolSubject::MagnifierLens,
                    Squared(Search.MinimumX + 17.0f, (Search.MinimumY + Search.MaximumY) * 0.5f, 13.0f),
                    Tinted.Faint);

    {
        const bool  Written  = Applied.Retention[0] != '\0';
        const float Baseline = (Search.MinimumY + Search.MaximumY) * 0.5f - Surface->LineHeight(12.0f) * 0.5f;

        Surface->TextRunTruncated(Search.MinimumX + 28.0f, Baseline, Search.Width() - 38.0f,
                                  Written ? Tinted.Primary : Tinted.Faint,
                                  Written ? Applied.Retention : "Search layers", 12.0f);

        // 📐 The caret, drawn only while the field holds the keyboard, at the run's trailing edge.
        if (Applied.RetentionHovered)
        {
            const float Caret = Search.MinimumX + 28.0f +
                                (Written ? Surface->MeasureRun(Applied.Retention, 12.0f) : 0.0f);

            Surface->Ground(Spanning(Caret + 1.0f, Search.MinimumY + 7.0f, 1.0f,
                                     Scaled.SearchHeight - 14.0f), Tinted.Primary);
        }
    }

    // ③ The three tool actions — add, group, retire.
    const float ActionMiddle = (Tools.MinimumY + Tools.MaximumY) * 0.5f;

    const PlaneExtent AddButton    = Squared(Tools.MaximumX - Scaled.ToolsPadX -
                                             Scaled.ButtonExtent * 2.5f, ActionMiddle, Scaled.ButtonExtent);
    const PlaneExtent FolderButton = Squared(Tools.MaximumX - Scaled.ToolsPadX -
                                             Scaled.ButtonExtent * 1.5f, ActionMiddle, Scaled.ButtonExtent);
    const PlaneExtent RetireButton = Squared(Tools.MaximumX - Scaled.ToolsPadX -
                                             Scaled.ButtonExtent * 0.5f, ActionMiddle, Scaled.ButtonExtent);

    const auto Action = [&](ChromeCell Cell, const PlaneExtent& Bounds, SymbolSubject Figure,
                            const char* Tooltip, ThemeToken Colour) -> bool
    {
        const bool Taken = Pressed(ChromeCells[static_cast<std::uint32_t>(Cell)], Bounds, Applied, Tooltip);

        if (Hovered(Bounds))
            Surface->Ground(Bounds, Partial(0xFFFFFFu, 0.07), Scaled.RadiusSmall);

        Surface->Stroke(Figure, Inset(Bounds, 6.5f, 6.5f), Colour);
        return Taken;
    };

    if (Action(ChromeCell::AddButton, AddButton, SymbolSubject::PlusCross, "Add layer", Tinted.Secondary))
    {
        Applied.Popup       = StackPopup::Addition;
        Applied.PopupX  = AddButton.MaximumX;
        Applied.PopupHeight = AddButton.MaximumY + 6.0f;
        Applied.PopupOffset = 0.0f;
        Ledger->Disclose(ChromeCells[static_cast<std::uint32_t>(ChromeCell::PopupBody)]);
    }

    if (Action(ChromeCell::FolderButton, FolderButton, SymbolSubject::LayerMerge, "Group", Tinted.Secondary))
    {
        Revisions.Record(Arrangement, "Folder declared");
        DeclareEntry(Arrangement, LayerContent::Folder, "New Folder");
    }

    if (Action(ChromeCell::RetireButton, RetireButton, SymbolSubject::TrashBin, "Delete", Tinted.Secondary))
    {
        Revisions.Record(Arrangement, "Entry retired");

        if (!RetireTaken(Arrangement))
            Revisions.Revert(Arrangement);
    }

    // ④ `.stack` — the scrolling run of rows, confined so an overflowing row is clipped and not drawn over
    //    the footer.
    const PlaneExtent Foot  = Spanning(Extent.MinimumX, Extent.MaximumY - Scaled.FootY,
                                       Extent.Width(), Scaled.FootY);
    const PlaneExtent Stack = PlaneExtent{ Extent.MinimumX, Tools.MaximumY,
                                           Extent.MaximumX,  Foot.MinimumY };

    Surface->Confine(Stack);

    const float RowX  = Stack.Width() - Scaled.StackPadX * 2.0f - Scaled.ScrollX;
    float       Y    = Stack.MinimumY + Scaled.StackPadY - Applied.StackOffset;
    const float Pointer   = Sampled.PositionY;
    const float PointerAt = Sampled.PositionX;

    std::uint32_t HoveredOrdinal = LayerStackCeiling::AbsentOrdinal;
    bool          HoveredMask = false;

    // 📐 What one drop would do, resolved fresh each tick against whatever the carried entry stands over.
    std::uint32_t Destination = LayerStackCeiling::AbsentOrdinal;
    DropIntent    Intent      = DropIntent::Absent;

    const bool Carrying = Applied.Carried < Arrangement.EntryCount;

    // 🔴 A contact that has not travelled the carry floor is still a candidate TAKE, not yet a carry. Only
    //    once it passes the floor does the row dim and a drop resolve — otherwise every ordinary press
    //    scrimmed the row it selected for the one tick the contact was held, which read as the row going
    //    hidden the instant it was clicked.
    const float CarryTravel = (Sampled.PositionY > Applied.CarryOrigin)
                            ? (Sampled.PositionY - Applied.CarryOrigin)
                            : (Applied.CarryOrigin - Sampled.PositionY);
    const bool  Travelled   = Carrying && Sampled.ContactHeld && CarryTravel >= CarryFloor;

    // 🔴 What the PREVIOUS tick resolved, kept before this tick overwrites it. The release tick carries no
    //    held contact, so it resolves no destination of its own — a drop that read only this tick's
    //    reading therefore always found nothing and silently discarded every reorder.
    const std::uint32_t PriorDestination = Applied.Destination;
    const DropIntent    PriorIntent      = Applied.Intent;

    // 📐 One presented row per registered run of cells. Beyond `RowCeiling` the rows still record and still
    //    take, on the row body's shared identity — a reduction, not a defect.
    std::uint32_t ShownCount = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < Arrangement.EntryCount; ++Ordinal)
    {
        // 📐 A retention run opens every folder it reaches into, so it is asked instead of the disclosure.
        const bool Retaining = Applied.Retention[0] != '\0';
        const bool Shown  = Retaining ? EntryRetained(Arrangement, Ordinal, Applied.Retention)
                                         : EntryCurrent(Arrangement, Ordinal);

        if (!Shown)
            continue;

        const LayerEntry& Entry  = Arrangement.Entries[Ordinal];
        const float       Indent = static_cast<float>(Entry.Depth) * Scaled.RowStepX;
        const std::uint32_t Cells = (ShownCount < RowCeiling) ? (ShownCount * CellsPerRow) : 0u;
        const bool          Registered = ShownCount < RowCeiling;

        ++ShownCount;

        const PlaneExtent Row = Spanning(Stack.MinimumX + Scaled.StackPadX + Indent, Y,
                                         RowX - Indent, Scaled.RowHeight);

        if (!Surface->Excluded(Row))
        {
            const bool Over = Row.Encloses(PointerAt, Pointer) && Stack.Encloses(PointerAt, Pointer);

            if (Over)
            {
                HoveredOrdinal = Ordinal;
                HoveredMask = false;
            }

            // 📐 A carried entry resolves its drop against whichever row it stands over. A folder crossed
            //    through its middle third is entered rather than passed — `y>.32&&y<.68`, verbatim.
            if (Travelled && Over && Applied.Carried != Ordinal &&
                !EntryWithin(Arrangement, Applied.Carried, Ordinal))
            {
                const float Fraction = (Pointer - Row.MinimumY) / Row.Height();

                Destination = Ordinal;
                Intent      = (Entry.Content == LayerContent::Folder && Fraction > 0.32f && Fraction < 0.68f)
                            ? DropIntent::Enclosed
                            : ((Fraction < 0.5f) ? DropIntent::Prior : DropIntent::Trailing);
            }

            const bool TakenRow = Arrangement.Taken == Ordinal && Arrangement.TakenHalf == LayerTaken::Layer;

            RecordEntryRow(Row, Arrangement, Ordinal, TakenRow, Over);

            // 📐 A carried entry is drawn at half coverage in place, which is what `.dragging{opacity:.4}`
            //    states, rather than being lifted to the pointer.
            if (Travelled && Applied.Carried == Ordinal)
                Surface->Ground(Row, Partial(0x000000u, 0.55), 0.0f);

            if (Registered)
            {
                const float Middle = (Row.MinimumY + Row.MaximumY) * 0.5f;

                // ⓐ The twisty, on a folder alone.
                if (Entry.Content == LayerContent::Folder)
                {
                    const PlaneExtent Twisty = Squared(Row.MinimumX + Scaled.RowPadX +
                                                       Scaled.DiscloseX * 0.5f, Middle, 18.0f);

                    if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::Disclosure)],
                                Twisty, Applied, Entry.Opened ? "Collapse" : "Expand"))
                    {
                        Arrangement.Entries[Ordinal].Opened = !Entry.Opened;
                    }
                }

                // ⓑ The eye. Alternate-clicking it solos, exactly as `if(e.altKey)` branches.
                const PlaneExtent Eye = Squared(Row.MinimumX + Scaled.RowPadX + Scaled.DiscloseX +
                                                Scaled.RowGapX * 0.5f + Scaled.ActionExtent * 0.5f,
                                                Middle, Scaled.ActionExtent);

                if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::Presence)], Eye, Applied,
                            Entry.Shown ? "Hide \xC2\xB7 Alt = solo" : "Show \xC2\xB7 Alt = solo"))
                {
                    Revisions.Record(Arrangement, "Presence amended");
                    Arrangement.Entries[Ordinal].Shown = !Entry.Shown;
                }

                // ⓒ The card chevron and the ellipsis, on the trailing edge.
                const PlaneExtent Unfolding = Squared(Row.MaximumX - Scaled.ActionExtent * 1.5f, Middle,
                                                      Scaled.ActionExtent);
                const PlaneExtent Menu      = Squared(Row.MaximumX - Scaled.ActionExtent * 0.5f, Middle,
                                                      Scaled.ActionExtent);

                if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::Unfolding)], Unfolding,
                            Applied, Entry.Unfolded ? "Hide details" : "Show details"))
                {
                    Arrangement.Entries[Ordinal].Unfolded = !Entry.Unfolded;

                    // 📐 `toggleCard` — opening STAGES the card so it renders closed for one frame and
                    //    departs on the next. Closing needs no stage: the fold already stands at one.
                    if (Arrangement.Entries[Ordinal].Unfolded)
                    {
                        Applied.CardPending   = Ordinal;
                        Applied.PendingOnMask = false;
                    }
                }

                if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::Menu)], Menu, Applied,
                            "Layer menu"))
                {
                    Arrangement.Taken     = Ordinal;
                    Arrangement.TakenHalf = LayerTaken::Layer;
                    Applied.Popup          = StackPopup::LayerMenu;
                    Applied.PopupSubject   = Ordinal;
                    Applied.PopupOnMask    = false;
                    Applied.PopupX     = Menu.MaximumX;
                    Applied.PopupHeight    = Menu.MaximumY + 6.0f;
                    Applied.PopupOffset    = 0.0f;
                    Ledger->Disclose(ChromeCells[static_cast<std::uint32_t>(ChromeCell::PopupBody)]);
                }
            }
        }

        Y += Scaled.RowHeight + 4.0f;

        // ⓓ `cardHTML(n)` — the unfolded card, which drops down beneath its own row.
        // 🔴 Recorded whenever the fold is OFF ITS SEAT and not merely while the flag stands, so the
        //    closing traverse is drawn to its end. A card recorded only while unfolded vanishes on the
        //    tick the flag clears and never animates shut.
        {
            const bool  Staged  = (Staging == Ordinal) && !Applied.PendingOnMask;
            const float Opening = CardOpening(Applied.CardFold[Ordinal], Entry.Unfolded, Staged, Elapsed);

            if (Opening > 0.0f)
            {
                // 🔴 The body is MEASURED by the same walk that records it, with the surface silent, so the
                //    extent the fold multiplies is the extent the content actually occupies.
                const PlaneExtent Measuring = Spanning(Stack.MinimumX + Scaled.StackPadX + Indent,
                                                       Y, RowX - Indent, 0.0f);
                const float Full = RecordEntryCard(Measuring, Arrangement, Ordinal, Applied, Revisions, false);
                const float Open = Full * Opening;

                const PlaneExtent Card = Spanning(Stack.MinimumX + Scaled.StackPadX + Indent,
                                                  Y, RowX - Indent, Open);

                if (!Surface->Excluded(Card))
                {
                    // 📐 `.card>.cin{overflow:hidden}` — the body is recorded at its whole extent and clipped
                    //    to how far the fold has travelled, which is what a `0fr→1fr` row does.
                    Surface->Ground(Card, Tinted.Detail, Scaled.RadiusStandard);
                    Surface->Confine(Card);

                    const PlaneExtent Whole = Spanning(Card.MinimumX, Y, Card.Width(), Full);
                    (void) RecordEntryCard(Whole, Arrangement, Ordinal, Applied, Revisions, true);

                    Surface->Release();
                    Surface->Edge(Card, Tinted.Stroke, 1.0f, Scaled.RadiusStandard);
                }

                Y += Open + 4.0f;
            }
        }

        // 📐 `.attach` — the mask row, drawn immediately beneath its entry and indented past the thumb.
        if (Entry.Mask.Declared)
        {
            const PlaneExtent MaskRow = Spanning(Stack.MinimumX + Scaled.StackPadX + Indent +
                                                 Scaled.MaskLeadX, Y,
                                                 RowX - Indent - Scaled.MaskLeadX, Scaled.MaskRowHeight);

            if (!Surface->Excluded(MaskRow))
            {
                const bool Over = MaskRow.Encloses(PointerAt, Pointer) && Stack.Encloses(PointerAt, Pointer);

                if (Over)
                {
                    HoveredOrdinal = Ordinal;
                    HoveredMask = true;
                }

                RecordMaskRow(MaskRow, Entry,
                              Arrangement.Taken == Ordinal && Arrangement.TakenHalf == LayerTaken::Mask, Over);

                if (Registered)
                {
                    const float Middle = (MaskRow.MinimumY + MaskRow.MaximumY) * 0.5f;

                    const PlaneExtent MaskEye = Squared(MaskRow.MinimumX + Scaled.RowPadX +
                                                        Scaled.ActionExtent * 0.5f, Middle, Scaled.ActionExtent);

                    if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::MaskPresence)],
                                MaskEye, Applied, Entry.Mask.Shown ? "Disable mask" : "Enable mask"))
                    {
                        Revisions.Record(Arrangement, "Mask presence amended");
                        Arrangement.Entries[Ordinal].Mask.Shown = !Entry.Mask.Shown;
                    }

                    const PlaneExtent MaskUnfold = Squared(MaskRow.MaximumX - Scaled.ActionExtent * 1.5f,
                                                           Middle, Scaled.ActionExtent);
                    const PlaneExtent MaskMenu   = Squared(MaskRow.MaximumX - Scaled.ActionExtent * 0.5f,
                                                           Middle, Scaled.ActionExtent);

                    if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::MaskUnfold)],
                                MaskUnfold, Applied, "Mask details"))
                    {
                        Arrangement.Entries[Ordinal].Mask.Unfolded = !Entry.Mask.Unfolded;

                        if (Arrangement.Entries[Ordinal].Mask.Unfolded)
                        {
                            Applied.CardPending   = Ordinal;
                            Applied.PendingOnMask = true;
                        }
                    }

                    if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::MaskMenu)],
                                MaskMenu, Applied, "Mask menu"))
                    {
                        Arrangement.Taken     = Ordinal;
                        Arrangement.TakenHalf = LayerTaken::Mask;
                        Applied.Popup          = StackPopup::MaskMenu;
                        Applied.PopupSubject   = Ordinal;
                        Applied.PopupOnMask    = true;
                        Applied.PopupX     = MaskMenu.MaximumX;
                        Applied.PopupHeight    = MaskMenu.MaximumY + 6.0f;
                        Applied.PopupOffset    = 0.0f;
                        Ledger->Disclose(ChromeCells[static_cast<std::uint32_t>(ChromeCell::PopupBody)]);
                    }
                }
            }

            Y += Scaled.MaskRowHeight + 4.0f;

            // ⓔ `maskCard(n)` — the mask's own card, on the same fold as the entry's.
            const bool  MaskStaged  = (Staging == Ordinal) && Applied.PendingOnMask;
            const float MaskOpening = CardOpening(Applied.MaskFold[Ordinal], Entry.Mask.Unfolded,
                                                  MaskStaged, Elapsed);

            if (MaskOpening > 0.0f)
            {
                const float MaskLead = Indent + Scaled.MaskLeadX;
                const PlaneExtent Measuring = Spanning(Stack.MinimumX + Scaled.StackPadX + MaskLead,
                                                       Y, RowX - MaskLead, 0.0f);
                const float Full = RecordMaskCard(Measuring, Arrangement, Ordinal, Applied, Revisions, false);
                const float Open = Full * MaskOpening;

                const PlaneExtent Card = Spanning(Stack.MinimumX + Scaled.StackPadX + MaskLead,
                                                  Y, RowX - MaskLead, Open);

                if (!Surface->Excluded(Card))
                {
                    Surface->Ground(Card, Tinted.Detail, Scaled.RadiusStandard);
                    Surface->Confine(Card);

                    const PlaneExtent Whole = Spanning(Card.MinimumX, Y, Card.Width(), Full);
                    (void) RecordMaskCard(Whole, Arrangement, Ordinal, Applied, Revisions, true);

                    Surface->Release();
                    Surface->Edge(Card, Tinted.Stroke, 1.0f, Scaled.RadiusStandard);
                }

                Y += Open + 4.0f;
            }
        }

        // 📐 The drop rule is recorded AFTER its row so it lies over the row's own ground.
        if (Destination == Ordinal && Intent != DropIntent::Absent)
            RecordDropMark(Row, Intent);
    }

    Applied.StackSpan   = (Y + Applied.StackOffset) - (Stack.MinimumY + Scaled.StackPadY);
    Applied.Hovered     = HoveredOrdinal;
    Applied.HoveredMask = HoveredMask;
    Applied.Destination = Destination;
    Applied.Intent      = Intent;

    Surface->Release();

    // ⑤ The scroll bar, recorded only when the run overflows its extent, and draggable in place.
    const float Visible = Stack.Height();
    const float Ceiling = (Applied.StackSpan > Visible) ? (Applied.StackSpan - Visible) : 0.0f;

    if (Ceiling > 0.0f && Visible > 0.0f)
    {
        const float Fraction    = Visible / Applied.StackSpan;
        const float ThumbHeight = (Visible * Fraction < 28.0f) ? 28.0f : Visible * Fraction;
        const float Travel      = Visible - ThumbHeight;
        const float Advanced    = Applied.StackOffset / Ceiling;

        const PlaneExtent Bar   = Spanning(Stack.MaximumX - Scaled.ScrollX, Stack.MinimumY,
                                           Scaled.ScrollX, Visible);
        const PlaneExtent Thumb = Spanning(Stack.MaximumX - Scaled.ScrollX + 3.0f,
                                           Stack.MinimumY + Travel * Advanced, 4.0f, ThumbHeight);

        ControlIdentity& Target = ChromeCells[static_cast<std::uint32_t>(ChromeCell::ScrollThumb)];

        if (Hovered(Bar) && Sampled.ContactPressed && !Ledger->AnyDisclosed())
        {
            Ledger->Grab(Target, ControlPart::Thumb);
            Ledger->RecordInitial(Target, Applied.StackOffset);
        }

        Ledger->DeclareHovered(Target, Hovered(Bar), HoverOver);

        // 📐 The bar travels `Travel` pixels while the run travels `Ceiling`, so the pointer's own travel
        //    is scaled by their ratio rather than applied to the offset directly.
        if (Ledger->Holding(Target) && Travel > 0.0f)
        {
            const Outcome<float> Previous = Ledger->InitialReading(Target);

            if (Previous.Resolved)
            {
                const float Moved = Sampled.PositionY - Ledger->OriginY();
                Applied.StackOffset = Previous.Resolve() + Moved * (Ceiling / Travel);
            }
        }

        Surface->Ground(Thumb, Partial(0xFFFFFFu, Ledger->Holding(Target) ? 0.30 : 0.15), 2.0f);
    }

    // ⑥ The wheel, which the stack answers itself because the seam carries no scrolling primitive.
    if (Stack.Encloses(PointerAt, Pointer) && Applied.Popup == StackPopup::Absent)
        Applied.StackOffset -= Sampled.WheelY * NotchHeight;

    if (Applied.StackOffset < 0.0f)      Applied.StackOffset = 0.0f;
    if (Applied.StackOffset > Ceiling)   Applied.StackOffset = Ceiling;

    // ⑦ What the artist takes, and what the artist carries. Both resolve off the SAME contact: a contact
    //    that arrived over a row and travelled beyond the carry floor is a drag, and one that did not is a
    //    take — which is exactly the separation `GestureTolerance` states and the reference gets from the
    //    window system's own drag threshold.
    if (Sampled.ContactPressed && HoveredOrdinal < Arrangement.EntryCount && !Ledger->AnyDisclosed())
    {
        Arrangement.Taken     = HoveredOrdinal;
        Arrangement.TakenHalf = HoveredMask ? LayerTaken::Mask : LayerTaken::Layer;

        // 📐 A secured entry refuses to be carried — `draggable="${n.lock?'false':'true'}"`.
        if (!HoveredMask && !Arrangement.Entries[HoveredOrdinal].Secured)
        {
            Applied.Carried     = HoveredOrdinal;
            Applied.CarryOrigin = Pointer;
        }
    }

    if (Carrying && !Sampled.ContactHeld)
    {
        // 📐 The drop, resolved against what the last held tick marked. A carry released over nothing
        //    droppable simply ends, which is what `cleanup()` does.
        if (PriorDestination < Arrangement.EntryCount && PriorIntent != DropIntent::Absent)
        {
            Revisions.Record(Arrangement, "Entry carried");

            const bool Moved = CarryEntry(Arrangement, Applied.Carried, PriorDestination,
                                          PriorIntent == DropIntent::Enclosed,
                                          PriorIntent == DropIntent::Trailing);

            if (!Moved)
                Revisions.Revert(Arrangement);
        }

        Applied.Carried     = LayerStackCeiling::AbsentOrdinal;
        Applied.Destination = LayerStackCeiling::AbsentOrdinal;
        Applied.Intent      = DropIntent::Absent;
    }

    // ⑧ `.foot` — the breadcrumb over the taken entry's blend and opacity.
    Surface->Ground(Foot, Tinted.PanelRaised);
    Surface->Edge(PlaneExtent{ Foot.MinimumX, Foot.MinimumY, Foot.MaximumX, Foot.MinimumY + 1.0f },
                  Tinted.Stroke);

    if (Arrangement.Taken < Arrangement.EntryCount)
    {
        LayerEntry& Taken = Arrangement.Entries[Arrangement.Taken];
        const bool  OnMask = Arrangement.TakenHalf == LayerTaken::Mask && Taken.Mask.Declared;

        char Crumb[128] = {};
        std::snprintf(Crumb, sizeof Crumb, "%s  /  %s%s", ContentNaming(Taken.Content), Taken.Naming,
                      OnMask ? "  /  Mask" : "");

        Surface->TextRunTruncated(Foot.MinimumX + Scaled.FootPadX, Foot.MinimumY + 9.0f,
                                  Foot.Width() - Scaled.FootPadX * 2.0f, Tinted.Faint,
                                  Crumb, Scaled.RunFine);

        // 📐 `.blend` — the pill that opens the blend menu, capped at 52% of the footer.
        const float       BlendX = Foot.Width() * 0.52f;
        const PlaneExtent Blend      = Spanning(Foot.MinimumX + Scaled.FootPadX,
                                                Foot.MinimumY + 26.0f, BlendX, 27.0f);

        if (Pressed(ChromeCells[static_cast<std::uint32_t>(ChromeCell::BlendPill)], Blend, Applied,
                    "Blend mode"))
        {
            Applied.Popup        = StackPopup::BlendMode;
            Applied.PopupSubject = Arrangement.Taken;
            Applied.PopupOnMask  = OnMask;
            Applied.PopupX   = Blend.MinimumX;
            Applied.PopupHeight  = Blend.MinimumY - 6.0f;
            Applied.PopupOffset  = 0.0f;
            Ledger->Disclose(ChromeCells[static_cast<std::uint32_t>(ChromeCell::PopupBody)]);
        }

        const bool BlendHovered = Hovered(Blend);

        Surface->Ground(Blend, Partial(0xFFFFFFu, BlendHovered ? 0.11 : 0.06), 13.5f);
        Surface->Edge(Blend, BlendHovered ? Tinted.StrokeStrong : Tinted.Stroke, 1.0f, 13.5f);
        Surface->TextRunTruncated(Blend.MinimumX + 13.0f,
                                  Blend.MinimumY + (27.0f - Surface->LineHeight(11.0f)) * 0.5f,
                                  BlendX - 32.0f, Tinted.Primary,
                                  OnMask ? Taken.Mask.Blend : Taken.Blend, 11.0f, true);
        Surface->Stroke(SymbolSubject::ChevronDown,
                        Squared(Blend.MaximumX - 12.0f, Blend.MinimumY + 13.5f, 11.0f), Tinted.Faint);

        // 📐 `#opac` — the opacity run that fills the footer's trailing half, dragged in place. The mask
        //    half moves the mask's density instead, exactly as `if(selMask&&n.mask)n.mask.den=v`.
        const float MeterTop = Blend.MaximumX + 10.0f;
        const float MeterMaximum  = Foot.MaximumX - Scaled.FootPadX - 34.0f;

        if (MeterMaximum > MeterTop)
        {
            const PlaneExtent Track = PlaneExtent{ MeterTop, Blend.MinimumY + 4.0f,
                                                   MeterMaximum,  Blend.MinimumY + 23.0f };

            std::uint32_t& Reading = OnMask ? Taken.Mask.Density : Taken.Opacity;
            const auto     Prior   = Reading;

            ControlIdentity& Target = ChromeCells[static_cast<std::uint32_t>(ChromeCell::OpacityRun)];

            // 📐 One revision per drag and not one per tick — recorded on the incoming edge alone, which
            //    is what `pointerdown → snap()` states and what keeps the ring from filling in a second.
            if (Hovered(Track) && Sampled.ContactPressed && !Ledger->AnyDisclosed())
                Revisions.Record(Arrangement, OnMask ? "Mask density moved" : "Opacity amended");

            if (Dragged(Target, Track, Reading) && Reading == Prior)
                Reading = Prior;

            RecordMeter(PlaneExtent{ MeterTop, Blend.MinimumY + 12.0f, MeterMaximum,
                                     Blend.MinimumY + 15.0f }, Reading, Tinted.Accent);

            // 📐 The thumb, drawn only while the run is hovered or held, as `.rng::-webkit-slider-thumb`
            //    is scaled from zero on hover.
            if (Hovered(Track) || Ledger->Holding(Target))
            {
                const float Bounds = MeterTop + (MeterMaximum - MeterTop) *
                                   static_cast<float>(Reading) * 0.01f;
                Surface->Medallion(Bounds, Blend.MinimumY + 13.5f, 5.0f, Tinted.Accent);
            }
        }

        char Percent[8] = {};
        std::snprintf(Percent, sizeof Percent, "%u%%", OnMask ? Taken.Mask.Density : Taken.Opacity);
        Surface->TextRun(Foot.MaximumX - Scaled.FootPadX -
                         Surface->MeasureRun(Percent, Scaled.RunSub),
                         Blend.MinimumY + 13.5f - Surface->LineHeight(Scaled.RunSub) * 0.5f,
                         Tinted.Secondary, Percent, Scaled.RunSub, 0.0f, true);
    }

    Surface->Release();
    Applied.ContactPrior = Sampled.ContactHeld;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE CHANNEL PROPERTIES
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordChannelProperties(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                                              LayerStackContext& Applied, RevisionSequence& Revisions)
{
    if (Surface == nullptr || Ledger == nullptr || !Surface->Recording())
        return;

    Surface->Ground(Extent, Tinted.Panel);
    Surface->Confine(Extent);

    const std::uint32_t Subject = (Arrangement.Taken < Arrangement.EntryCount) ? Arrangement.Taken : 0u;
    LayerEntry&         Entry   = Arrangement.Entries[Subject];

    // ① The head — the naming, its reading and the classification medallion.
    const PlaneExtent Head = Spanning(Extent.MinimumX, Extent.MinimumY, Extent.Width(), 52.0f);
    Surface->Ground(Head, Tinted.PanelRaised);
    Surface->Edge(PlaneExtent{ Head.MinimumX, Head.MaximumY - 1.0f, Head.MaximumX, Head.MaximumY },
                  Tinted.Stroke);

    const PlaneExtent Medallion = Squared(Head.MinimumX + 24.0f, Head.MinimumY + 26.0f, 26.0f);
    Surface->Ground(Medallion, ContentTint(Entry.Content), Scaled.RadiusSmall);
    Surface->Stroke(SymbolSubject::ChannelSelect, Inset(Medallion, 6.0f, 6.0f), Covering(0x000000u));

    Surface->TextRunTruncated(Head.MinimumX + 46.0f, Head.MinimumY + 11.0f,
                              Head.Width() - 60.0f, Tinted.Primary, Entry.Naming, 13.0f, true);

    char Reading[96] = {};
    std::snprintf(Reading, sizeof Reading, "%s  %upx  %s",
                  ContentNaming(Entry.Content), Entry.Resolution, Entry.Format);
    Surface->TextRun(Head.MinimumX + 46.0f, Head.MinimumY + 28.0f, Tinted.Faint, Reading, Scaled.RunFine);

    // ② The chips region — one chip per enabled channel, tinted with its own hue.
    float Y = Head.MaximumY;

    const PlaneExtent ChipsHead = Spanning(Extent.MinimumX, Y, Extent.Width(), Scaled.SectionHeight);
    char ChipsReading[24] = {};
    std::snprintf(ChipsReading, sizeof ChipsReading, "%u", ChannelsEnabled(Entry));
    RecordSectionHead(Inset(ChipsHead, Scaled.CardPadX, 0.0f), "Channels", ChipsReading, true);
    Y += Scaled.SectionHeight;

    float ChipX = Extent.MinimumX + Scaled.CardPadX;

    for (std::uint32_t Channel = 0u; Channel < LayerStackCeiling::Channels; ++Channel)
    {
        if (!Entry.Channels[Channel].Enabled)
            continue;

        const char* Caption = ChannelNaming()[Channel];
        const float X   = Surface->MeasureRun(Caption, Scaled.RunFine) + 26.0f;

        if (ChipX + X > Extent.MaximumX - Scaled.CardPadX)
        {
            ChipX = Extent.MinimumX + Scaled.CardPadX;
            Y   += 22.0f;
        }

        const PlaneExtent Chip = Spanning(ChipX, Y, X, 18.0f);
        Surface->Ground(Chip, Partial(0xFFFFFFu, 0.05), 9.0f);
        Surface->Edge(Chip, Tinted.Stroke, 1.0f, 9.0f);
        Surface->Medallion(Chip.MinimumX + 10.0f, Y + 9.0f, 3.5f, ChannelTint(Channel));
        Surface->TextRun(Chip.MinimumX + 18.0f, Y + 9.0f - Surface->LineHeight(Scaled.RunFine) * 0.5f,
                         Tinted.Secondary, Caption, Scaled.RunFine);

        ChipX += X + 5.0f;
    }

    Y += 30.0f;

    // ③ One panel per channel — a dot, its naming, its blend and its opacity meter.
    const PlaneExtent BlendingHead = Spanning(Extent.MinimumX, Y, Extent.Width(), Scaled.SectionHeight);
    RecordSectionHead(Inset(BlendingHead, Scaled.CardPadX, 0.0f), "Channel Blending", nullptr, true);
    Y += Scaled.SectionHeight + 2.0f;

    for (std::uint32_t Channel = 0u; Channel < LayerStackCeiling::Channels; ++Channel)
    {
        ChannelCoordinate&  Reading8 = Entry.Channels[Channel];
        const PlaneExtent Row      = Spanning(Extent.MinimumX + Scaled.CardPadX, Y,
                                              Extent.Width() - Scaled.CardPadX * 2.0f, 28.0f);

        if (Surface->Excluded(Row))
        {
            Y += 30.0f;
            continue;
        }

        const bool RowHovered = Hovered(Row);

        Surface->Ground(Row, RowHovered ? Tinted.RowHovered
                                       : (Reading8.Enabled ? Tinted.Row : Tinted.Detail),
                        Scaled.RadiusSmall);

        const float Middle = Row.MinimumY + 14.0f;

        // 📐 `[data-cha="on"]` — the dot toggles the channel. Its own 20px cell, not the whole row, so a
        //    contact on the blend run beside it does not silently disable the channel.
        const PlaneExtent Dot = Squared(Row.MinimumX + 12.0f, Middle, 20.0f);

        if (Pressed(RowCells[Channel * CellsPerRow + static_cast<std::uint32_t>(RowCell::Body)], Dot,
                    Applied, Reading8.Enabled ? "Disable channel" : "Enable channel"))
        {
            Revisions.Record(Arrangement, "Channel amended");
            Reading8.Enabled = !Reading8.Enabled;
        }

        Surface->Medallion(Row.MinimumX + 12.0f, Middle, 4.0f,
                           Reading8.Enabled ? ChannelTint(Channel) : Tinted.Faint);

        if (Hovered(Dot))
            Surface->Medallion(Row.MinimumX + 12.0f, Middle, 7.5f, Partial(0xFFFFFFu, 0.14));

        Surface->TextRunTruncated(Row.MinimumX + 22.0f, Middle - Surface->LineHeight(Scaled.RunSub) * 0.5f,
                                  108.0f, Reading8.Enabled ? Tinted.Primary : Tinted.Faint,
                                  ChannelNaming()[Channel], Scaled.RunSub, true);

        // 📐 The blend run opens the same twenty-nine-entry menu the footer pill does, anchored here.
        const PlaneExtent BlendRun = PlaneExtent{ Row.MinimumX + 128.0f, Row.MinimumY + 4.0f,
                                                  Row.MaximumX - 74.0f,   Row.MaximumY - 4.0f };

        if (Pressed(RowCells[Channel * CellsPerRow + static_cast<std::uint32_t>(RowCell::Menu)],
                    BlendRun, Applied, "Channel blend"))
        {
            Applied.Popup        = StackPopup::BlendMode;
            Applied.PopupSubject = Subject;
            Applied.PopupOnMask  = false;
            Applied.PopupX   = BlendRun.MaximumX;
            Applied.PopupHeight  = BlendRun.MaximumY + 6.0f;
            Applied.PopupOffset  = 0.0f;
            Ledger->Disclose(ChromeCells[static_cast<std::uint32_t>(ChromeCell::PopupBody)]);
        }

        if (Hovered(BlendRun))
            Surface->Ground(BlendRun, Partial(0xFFFFFFu, 0.05), Scaled.RadiusSmall);

        Surface->TextRunTruncated(Row.MinimumX + 132.0f, Middle - Surface->LineHeight(Scaled.RunFine) * 0.5f,
                                  Row.Width() - 132.0f - 74.0f, Tinted.Secondary,
                                  Reading8.Blend, Scaled.RunFine);

        // 📐 The channel's own opacity, dragged in place — `[data-cha="op"]`.
        {
            const PlaneExtent Track = PlaneExtent{ Row.MaximumX - 66.0f, Row.MinimumY + 4.0f,
                                                   Row.MaximumX - 30.0f, Row.MaximumY - 4.0f };

            ControlIdentity& Target =
                RowCells[Channel * CellsPerRow + static_cast<std::uint32_t>(RowCell::Opacity)];

            if (Hovered(Track) && Sampled.ContactPressed && !Ledger->AnyDisclosed())
                Revisions.Record(Arrangement, "Channel opacity amended");

            Dragged(Target, Track, Reading8.Opacity);
        }

        RecordMeter(PlaneExtent{ Row.MaximumX - 66.0f, Middle - 1.5f, Row.MaximumX - 30.0f, Middle + 1.5f },
                    Reading8.Opacity, Reading8.Enabled ? ChannelTint(Channel) : Tinted.Faint);

        char Percent[8] = {};
        std::snprintf(Percent, sizeof Percent, "%u%%", Reading8.Opacity);
        Surface->TextRun(Row.MaximumX - 26.0f, Middle - Surface->LineHeight(Scaled.RunFine) * 0.5f,
                         Tinted.Faint, Percent, Scaled.RunFine);

        Y += 30.0f;
    }

    // ④ The foot — how many channels stand against how many atlases the arrangement covers.
    const PlaneExtent Foot = Spanning(Extent.MinimumX, Extent.MaximumY - 26.0f, Extent.Width(), 26.0f);
    Surface->Ground(Foot, Tinted.PanelRaised);
    Surface->Edge(PlaneExtent{ Foot.MinimumX, Foot.MinimumY, Foot.MaximumX, Foot.MinimumY + 1.0f },
                  Tinted.Stroke);

    char Footing[48] = {};
    std::snprintf(Footing, sizeof Footing, "%u channels  \xC2\xB7  %u atlases",
                  ChannelsEnabled(Entry), LayerStackCeiling::AtlasTotal);
    Surface->TextRun(Foot.MinimumX + Scaled.FootPadX,
                     Foot.MinimumY + (26.0f - Surface->LineHeight(Scaled.RunFine)) * 0.5f,
                     Tinted.Faint, Footing, Scaled.RunFine);

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE MASK PROPERTIES
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordMaskProperties(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                                           LayerStackContext& Applied, RevisionSequence& Revisions)
{
    if (Surface == nullptr || Ledger == nullptr || !Surface->Recording())
        return;

    Surface->Ground(Extent, Tinted.Panel);
    Surface->Confine(Extent);

    const std::uint32_t  Subject = (Arrangement.Taken < Arrangement.EntryCount) ? Arrangement.Taken : 0u;
    LayerEntry&          Entry   = Arrangement.Entries[Subject];
    MaskCoordinate&        Mask    = Entry.Mask;

    // ① The head.
    const PlaneExtent Head = Spanning(Extent.MinimumX, Extent.MinimumY, Extent.Width(), 52.0f);
    Surface->Ground(Head, Tinted.PanelRaised);
    Surface->Edge(PlaneExtent{ Head.MinimumX, Head.MaximumY - 1.0f, Head.MaximumX, Head.MaximumY },
                  Tinted.Stroke);

    const PlaneExtent Medallion = Squared(Head.MinimumX + 24.0f, Head.MinimumY + 26.0f, 26.0f);
    Surface->Ground(Medallion, Mask.Inverted ? Covering(0x2A2A2Au) : Covering(0xC8C8C8u), Scaled.RadiusSmall);
    Surface->Stroke(SymbolSubject::MaskStencil, Inset(Medallion, 6.0f, 6.0f), Covering(0x000000u));

    Surface->TextRun(Head.MinimumX + 46.0f, Head.MinimumY + 11.0f, Tinted.Primary, "Mask", 13.0f, 0.0f, true);

    char Reading[96] = {};
    std::snprintf(Reading, sizeof Reading, "clips %.18s", Entry.Naming);
    Surface->TextRun(Head.MinimumX + 46.0f, Head.MinimumY + 28.0f, Tinted.Faint, Reading, Scaled.RunFine);

    if (!Mask.Declared)
    {
        Surface->TextRun(Extent.MinimumX + Scaled.CardPadX, Head.MaximumY + 18.0f, Tinted.Faint,
                         "This layer carries no mask.", Scaled.RunSub);
        Surface->Release();
        return;
    }

    float Y = Head.MaximumY + 2.0f;

    // ② Source — what the mask reads from, and the two switches beside it.
    const PlaneExtent SourceHead = Spanning(Extent.MinimumX, Y, Extent.Width(), Scaled.SectionHeight);
    RecordSectionHead(Inset(SourceHead, Scaled.CardPadX, 0.0f), "Source",
                      SourceNaming(Mask.Source), true);
    Y += Scaled.SectionHeight;

    const PlaneExtent Body = PlaneExtent{ Extent.MinimumX + Scaled.CardPadX, Y,
                                          Extent.MaximumX  - Scaled.CardPadX, Y + Scaled.FieldHeight };

    // Source, density, and blend are live controls rather than display-only rows.
    const PlaneExtent SourceControl = PlaneExtent{ Body.MinimumX + 96.0f, Body.MinimumY,
                                                   Body.MaximumX, Body.MinimumY + Scaled.FieldHeight };
    if (Pressed(RowCells[static_cast<std::uint32_t>(RowCell::MaskBody)], SourceControl, Applied,
                "Change mask source"))
    {
        Revisions.Record(Arrangement, "Mask source amended");
        const std::uint32_t Next = (static_cast<std::uint32_t>(Mask.Source) + 1u)
                                 % static_cast<std::uint32_t>(MaskSource::SourceCount);
        Mask.Source = static_cast<MaskSource>(Next);
    }
    Y = RecordReadingRow(Body, "Source", SourceNaming(Mask.Source));

    if (Mask.Generator != nullptr)
        Y = RecordReadingRow(PlaneExtent{ Body.MinimumX, Y, Body.MaximumX,
                                               Y + Scaled.FieldHeight }, "Generator", Mask.Generator);

    char Density[8] = {};
    std::snprintf(Density, sizeof Density, "%u%%", Mask.Density);
    Y = RecordReadingRow(PlaneExtent{ Body.MinimumX, Y, Body.MaximumX,
                                           Y + Scaled.FieldHeight }, "Density", Density);
    Y = RecordReadingRow(PlaneExtent{ Body.MinimumX, Y, Body.MaximumX,
                                           Y + Scaled.FieldHeight }, "Blend", Mask.Blend);

    const PlaneExtent DensityRow = PlaneExtent{ Body.MinimumX, Y, Body.MaximumX,
                                                Y + Scaled.FieldHeight };
    Surface->TextRun(Body.MinimumX, Y + 8.0f, Tinted.Faint, "Density", Scaled.RunSub);
    const PlaneExtent DensityTrack = PlaneExtent{ Body.MinimumX + 96.0f, Y + 4.0f,
                                                  Body.MaximumX - 44.0f, Y + Scaled.FieldHeight - 4.0f };
    if (Hovered(DensityTrack) && Sampled.ContactPressed && !Ledger->AnyDisclosed())
        Revisions.Record(Arrangement, "Mask density amended");
    Dragged(RowCells[static_cast<std::uint32_t>(RowCell::Opacity)], DensityTrack, Mask.Density);
    RecordMeter(DensityTrack, Mask.Density, Tinted.Accent);
    Y += Scaled.FieldHeight;

    // 📐 The invert switch — a 26×14 pill whose knob sits on whichever side the reading names.
    {
        const float       Middle = Y + Scaled.FieldHeight * 0.5f;
        const PlaneExtent Switch = Spanning(Body.MaximumX - 26.0f, Middle - 7.0f, 26.0f, 14.0f);

        Surface->TextRun(Body.MinimumX, Middle - Surface->LineHeight(Scaled.RunSub) * 0.5f,
                         Tinted.Faint, "Invert", Scaled.RunSub);

        if (Pressed(RowCells[static_cast<std::uint32_t>(RowCell::Body)], Switch, Applied,
                    Mask.Inverted ? "Stop inverting" : "Invert"))
        {
            Revisions.Record(Arrangement, "Mask inverted");
            Mask.Inverted = !Mask.Inverted;
        }

        Surface->Ground(Switch, Mask.Inverted ? Tinted.Accent : Partial(0xFFFFFFu, 0.09), 7.0f);
        Surface->Medallion(Mask.Inverted ? Switch.MaximumX - 6.0f : Switch.MinimumX + 6.0f,
                           Middle, 5.0f, Mask.Inverted ? Covering(0x000000u) : Tinted.Secondary);

        Y += Scaled.FieldHeight;
    }

    // ③ Parameters — every reading the source declares, each as a caption, a meter and its reading.
    if (Mask.ParameterCount > 0u)
    {
        const PlaneExtent ParameterHead = Spanning(Extent.MinimumX, Y + 4.0f, Extent.Width(),
                                                   Scaled.SectionHeight);
        char ParameterReading[16] = {};
        std::snprintf(ParameterReading, sizeof ParameterReading, "%u", Mask.ParameterCount);
        RecordSectionHead(Inset(ParameterHead, Scaled.CardPadX, 0.0f), "Parameters",
                          ParameterReading, true);
        Y += Scaled.SectionHeight + 4.0f;

        for (std::uint32_t Ordinal = 0u; Ordinal < Mask.ParameterCount; ++Ordinal)
        {
            ParameterCoordinate& Parameter = Mask.Parameters[Ordinal];
            const float        Middle    = Y + Scaled.FieldHeight * 0.5f;

            // 📐 One registered run of cells per parameter, beyond the eight the channel rows take.
            const std::uint32_t Cells = (LayerStackCeiling::Channels + Ordinal < RowCeiling)
                                      ? (LayerStackCeiling::Channels + Ordinal) * CellsPerRow
                                      : 0u;

            Surface->TextRunTruncated(Body.MinimumX, Middle - Surface->LineHeight(Scaled.RunSub) * 0.5f,
                                      96.0f, Tinted.Faint, Parameter.Naming, Scaled.RunSub);

            if (Parameter.Selected != nullptr)
            {
                Surface->TextRunTruncated(Body.MinimumX + 104.0f,
                                          Middle - Surface->LineHeight(Scaled.RunSub) * 0.5f,
                                          Body.Width() - 104.0f, Tinted.Primary,
                                          Parameter.Selected, Scaled.RunSub, true);
            }
            else if (Parameter.Toggling)
            {
                const PlaneExtent Switch = Spanning(Body.MaximumX - 26.0f, Middle - 7.0f, 26.0f, 14.0f);
                const bool        Current = Parameter.Current > 0.5;

                // 📐 `[data-pt]` — a parameter switch toggles in place and does not re-record the panel.
                if (Pressed(RowCells[Cells + static_cast<std::uint32_t>(RowCell::Presence)], Switch, Applied))
                {
                    Revisions.Record(Arrangement, "Parameter amended");
                    Parameter.Current = Current ? 0.0 : 1.0;
                }

                Surface->Ground(Switch, Current ? Tinted.Accent : Partial(0xFFFFFFu, 0.09), 7.0f);
                Surface->Medallion(Current ? Switch.MaximumX - 6.0f : Switch.MinimumX + 6.0f,
                                   Middle, 5.0f, Current ? Covering(0x000000u) : Tinted.Secondary);
            }
            else
            {
                const double Span     = Parameter.Maximum - Parameter.Minimum;

                // 📐 `[data-pk]` — the range is dragged in its own declared span and not in 0…100, so a
                //    size in pixels or a rotation in degrees reads its own units back.
                {
                    const PlaneExtent Track = PlaneExtent{ Body.MinimumX + 104.0f, Y + 3.0f,
                                                           Body.MaximumX - 44.0f, Y + Scaled.FieldHeight - 3.0f };

                    auto Reading = (Span > 0.0)
                                 ? static_cast<std::uint32_t>((Parameter.Current - Parameter.Minimum) / Span * 100.0)
                                 : 0u;

                    ControlIdentity& Target =
                        RowCells[Cells + static_cast<std::uint32_t>(RowCell::Opacity)];

                    if (Hovered(Track) && Sampled.ContactPressed && !Ledger->AnyDisclosed())
                        Revisions.Record(Arrangement, "Parameter amended");

                    if (Dragged(Target, Track, Reading))
                        Parameter.Current = Parameter.Minimum + Span * static_cast<double>(Reading) * 0.01;
                }

                const double Fraction = (Span > 0.0) ? ((Parameter.Current - Parameter.Minimum) / Span) : 0.0;

                RecordMeter(PlaneExtent{ Body.MinimumX + 104.0f, Middle - 1.5f,
                                         Body.MaximumX - 44.0f, Middle + 1.5f },
                            static_cast<std::uint32_t>(Fraction * 100.0), Tinted.Accent);

                char Written[24] = {};
                std::snprintf(Written, sizeof Written, "%.0f%s", Parameter.Current, Parameter.Unit);
                Surface->TextRun(Body.MaximumX - Surface->MeasureRun(Written, Scaled.RunFine),
                                 Middle - Surface->LineHeight(Scaled.RunFine) * 0.5f,
                                 Tinted.Secondary, Written, Scaled.RunFine);
            }

            Y += Scaled.FieldHeight;
        }
    }

    // ④ Mesh Map Inputs — one chip per map, marked by whether its transfer stands.
    if (Mask.MeshMapCount > 0u)
    {
        const PlaneExtent MapHead = Spanning(Extent.MinimumX, Y + 4.0f, Extent.Width(),
                                             Scaled.SectionHeight);
        RecordSectionHead(Inset(MapHead, Scaled.CardPadX, 0.0f), "Mesh Map Inputs", nullptr, true);
        Y += Scaled.SectionHeight + 4.0f;

        float ChipX  = Body.MinimumX;
        bool  AnyAbsent  = false;

        for (std::uint32_t Ordinal = 0u; Ordinal < Mask.MeshMapCount; ++Ordinal)
        {
            const bool  Transferred = Mask.MeshMapTransferred[Ordinal];
            const char* Caption     = Mask.MeshMaps[Ordinal];
            const float X       = Surface->MeasureRun(Caption, Scaled.RunFine) + 24.0f;

            if (!Transferred)
                AnyAbsent = true;

            if (ChipX + X > Body.MaximumX)
            {
                ChipX = Body.MinimumX;
                Y   += 22.0f;
            }

            const PlaneExtent Chip = Spanning(ChipX, Y, X, 18.0f);

            // 📐 `data-dact="bake"` — a contact on an untransferred chip transfers that one map. The
            //    reference offers only "transfer all missing"; per-chip is the same command at the
            //    granularity the chip already presents, and the row's own chip is where an artist aims.
            if (Pressed(RowCells[(LayerStackCeiling::Channels + Ordinal) % RowCeiling * CellsPerRow +
                                 static_cast<std::uint32_t>(RowCell::MaskBody)], Chip, Applied,
                        Transferred ? "Transferred" : "Transfer this mesh map"))
            {
                Revisions.Record(Arrangement, "Mesh map transferred");
                Mask.MeshMapTransferred[Ordinal] = !Transferred;
            }

            Surface->Ground(Chip, Partial(0xFFFFFFu, Hovered(Chip) ? 0.11 : 0.05), 9.0f);
            Surface->Edge(Chip, Transferred ? Tinted.Stroke : Partial(0xFF6B63u, 0.35), 1.0f, 9.0f);
            Surface->TextRun(Chip.MinimumX + 8.0f, Y + 9.0f - Surface->LineHeight(Scaled.RunFine) * 0.5f,
                             Transferred ? Tinted.Secondary : Tinted.Danger, Caption, Scaled.RunFine);
            Surface->Stroke(Transferred ? SymbolSubject::CubeSolid : SymbolSubject::PlusCross,
                            Squared(Chip.MaximumX - 9.0f, Y + 9.0f, 8.0f),
                            Transferred ? Tinted.Affirm : Tinted.Danger,
                            Transferred ? 0.0f : 0.785398f);

            ChipX += X + 5.0f;
        }

        Y += 24.0f;

        if (AnyAbsent)
        {
            Surface->TextRun(Body.MinimumX, Y, Tinted.Danger, "Transfer missing", Scaled.RunFine);
            Y += 18.0f;
        }
    }

    // ⑤ Applies To Channels — the eight chips, dimmed where the mask does not reach.
    const PlaneExtent AppliesHead = Spanning(Extent.MinimumX, Y + 4.0f, Extent.Width(),
                                             Scaled.SectionHeight);
    RecordSectionHead(Inset(AppliesHead, Scaled.CardPadX, 0.0f), "Applies To Channels", nullptr, true);
    Y += Scaled.SectionHeight + 4.0f;

    float AppliesX = Body.MinimumX;

    for (std::uint32_t Channel = 0u; Channel < LayerStackCeiling::Channels; ++Channel)
    {
        const bool  Reached = Mask.ChannelApplied[Channel];
        const char* Caption = ChannelNaming()[Channel];
        const float X   = Surface->MeasureRun(Caption, Scaled.RunFine) + 16.0f;

        if (AppliesX + X > Body.MaximumX)
        {
            AppliesX = Body.MinimumX;
            Y      += 22.0f;
        }

        const PlaneExtent Chip = Spanning(AppliesX, Y, X, 18.0f);

        // 📐 `data-dact="mchan"` — the chip toggles whether the mask reaches that channel.
        if (Pressed(RowCells[Channel * CellsPerRow + static_cast<std::uint32_t>(RowCell::MaskDensity)],
                    Chip, Applied, Reached ? "Stop applying" : "Apply to this channel"))
        {
            Revisions.Record(Arrangement, "Mask channels amended");
            Mask.ChannelApplied[Channel] = !Reached;
        }

        Surface->Ground(Chip, Reached ? Partial(0xFFFFFFu, 0.90) : Tinted.Detail, 9.0f);
        Surface->Edge(Chip, Hovered(Chip) ? Tinted.StrokeStrong : Tinted.Stroke, 1.0f, 9.0f);
        // 📐 `.chip.tog.on{background:rgba(255,255,255,.9);color:#000}` — a applied chip inverts its run.
        Surface->TextRun(Chip.MinimumX + 8.0f, Y + 9.0f - Surface->LineHeight(Scaled.RunFine) * 0.5f,
                         Reached ? Covering(0x000000u) : Tinted.Faint, Caption, Scaled.RunFine);

        AppliesX += X + 5.0f;
    }

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE REVISIONS
//------------------------------------------------------------------------------------------------------------------------

void LayerStackPanel::RecordRevisions(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                                     LayerStackContext& Applied, RevisionSequence& Revisions)
{
    if (Surface == nullptr || !Surface->Recording() || Ledger == nullptr)
        return;

    Surface->Ground(Extent, Tinted.Panel);
    Surface->Confine(Extent);

    // ① The head, which folds the whole run. `renderHistory` gives its group header `cursor-pointer` and
    //    `onClick={() => toggleHistoryCard(token)}`, and rotates the chevron −90° while it stands folded.
    const PlaneExtent Head = Spanning(Extent.MinimumX, Extent.MinimumY, Extent.Width(),
                                      Scaled.HeadHeight);

    const bool HeadHovered = Hovered(Head);

    if (Pressed(ChromeCells[static_cast<std::uint32_t>(ChromeCell::RevisionHead)], Head, Applied,
                Applied.RevisionsFolded ? "Show history" : "Hide history"))
    {
        Applied.RevisionsFolded = !Applied.RevisionsFolded;
        Applied.RevisionField   = 0u;
    }

    Surface->Ground(Head, HeadHovered ? Tinted.RowHovered : Tinted.PanelRaised);
    Surface->Edge(PlaneExtent{ Head.MinimumX, Head.MaximumY - 1.0f, Head.MaximumX, Head.MaximumY },
                  Tinted.Stroke);

    // 📝 The caption the reference draws, which is not the spelling the identifiers carry.
    Surface->TextRunCapitalised(Head.MinimumX + Scaled.HeadPadX,
                                Head.MinimumY + (Scaled.HeadHeight - Surface->LineHeight(Scaled.RunHead)) * 0.5f,
                                HeadHovered ? Tinted.Primary : Tinted.Secondary, "History", Scaled.RunHead,
                                TrackingHead, true);

    const RevisionCoordinate* Reference = nullptr;
    std::uint32_t           Count     = 0u;
    ApplyReferenceRevisions(Reference, Count);

    const std::uint32_t Current = Revisions.RecordedCount() + Count;

    // 📐 `{tokenRevisions.length} ops` — the count the header carries at its trailing edge, ahead of the
    //    chevron.
    {
        char Reading[24] = {};
        std::snprintf(Reading, sizeof Reading, "%u ops", Current);

        const float X = Head.MaximumX - 34.0f - Surface->MeasureRun(Reading, Scaled.RunFine);
        Surface->TextRun(X, Head.MinimumY + (Scaled.HeadHeight - Surface->LineHeight(Scaled.RunFine)) * 0.5f,
                         Tinted.Secondary, Reading, Scaled.RunFine);
    }

    Surface->Stroke(Applied.RevisionsFolded ? SymbolSubject::ChevronRight : SymbolSubject::ChevronDown,
                    Squared(Head.MaximumX - 18.0f, (Head.MinimumY + Head.MaximumY) * 0.5f, 13.0f),
                    Tinted.Faint);

    // ② The two ring actions, which the reference's own inspector does not carry but the arrangement
    //    demands: a recorded run that cannot be walked back is a log, not a history.
    const PlaneExtent Bar = Spanning(Extent.MinimumX, Head.MaximumY, Extent.Width(), 34.0f);

    if (!Applied.RevisionsFolded)
    {
        Surface->Ground(Bar, Tinted.Detail);
        Surface->Edge(PlaneExtent{ Bar.MinimumX, Bar.MaximumY - 1.0f, Bar.MaximumX, Bar.MaximumY },
                      Tinted.Stroke);

        const float Middle = (Bar.MinimumY + Bar.MaximumY) * 0.5f;

        const PlaneExtent RevertButton    = Spanning(Bar.MinimumX + 10.0f, Middle - 11.0f, 78.0f, 22.0f);
        const PlaneExtent ReinstateButton = Spanning(RevertButton.MaximumX + 6.0f, Middle - 11.0f, 88.0f, 22.0f);

        const auto RingAction = [&](ChromeCell Cell, const PlaneExtent& Bounds, const char* Caption,
                                    bool Offered, const char* Tooltip) -> bool
        {
            const bool Taken = Offered && Pressed(ChromeCells[static_cast<std::uint32_t>(Cell)], Bounds,
                                                  Applied, Tooltip);

            const bool Lit = Offered && Hovered(Bounds);

            Surface->Ground(Bounds, Partial(0xFFFFFFu, Lit ? 0.10 : 0.045), Scaled.RadiusSmall);
            Surface->Edge(Bounds, Lit ? Tinted.StrokeStrong : Tinted.Stroke, 1.0f, Scaled.RadiusSmall);

            const float X = Bounds.MinimumX +
                                (Bounds.Width() - Surface->MeasureRun(Caption, Scaled.RunFine)) * 0.5f;

            Surface->TextRun(X, Bounds.MinimumY + (22.0f - Surface->LineHeight(Scaled.RunFine)) * 0.5f,
                             Offered ? (Lit ? Tinted.Primary : Tinted.Secondary) : Tinted.Faint,
                             Caption, Scaled.RunFine);

            return Taken;
        };

        // 📐 Both read the ring rather than record into it, exactly as ⌘Z and ⇧⌘Z do in `AcceptChord`.
        if (RingAction(ChromeCell::RevertAction, RevertButton, "Revert",
                       Revisions.RecordedCount() > 0u, "Revert the last amendment"))
        {
            Revisions.Revert(Arrangement);
            Applied.RevisionShown = LayerStackCeiling::AbsentOrdinal;
            Applied.RevisionField = 0u;
        }

        if (RingAction(ChromeCell::ReinstateAction, ReinstateButton, "Reinstate",
                       Revisions.ReinstatableCount() > 0u, "Reinstate what was reverted"))
        {
            Revisions.Reinstate(Arrangement);
            Applied.RevisionShown = LayerStackCeiling::AbsentOrdinal;
            Applied.RevisionField = 0u;
        }
    }

    // 📐 `gridTemplateRows: isCollapsed ? '0fr' : '1fr'` — a folded run records nothing at all, which is
    //    what an unfolded extent of zero actually amounts to once the transition has settled.
    if (Applied.RevisionsFolded)
    {
        Applied.RevisionSpan = 0.0f;
        Surface->Release();
        return;
    }

    const PlaneExtent Run = Spanning(Extent.MinimumX, Bar.MaximumY, Extent.Width(),
                                     Extent.MaximumY - Bar.MaximumY);

    Surface->Confine(Run);

    if (Current == 0u)
    {
        // 📝 The reference's own empty state, verbatim.
        Surface->TextRunTruncated(Run.MinimumX + Scaled.CardPadX, Run.MinimumY + 16.0f,
                                  Run.Width() - Scaled.CardPadX * 2.0f, Tinted.Faint,
                                  "No history events found for this selection or its children.", 11.5f);
        Surface->Release();
        Surface->Release();
        Applied.RevisionSpan = 0.0f;
        return;
    }

    float Y = Run.MinimumY + Scaled.StackPadY - Applied.RevisionOffset;

    std::uint32_t Registered = 0u;

    // 📐 One card, recorded the same way whether its reading came from the ring or from the applied
    //    reference run. The reference draws both out of one `tokenRevisions.map`, so they share a body.
    const auto RecordCard = [&](std::uint32_t Ordinal, const char* Naming, const char* Moment,
                                const char* Detail, bool SecondCurrent) -> void
    {
        const bool  Shown = Applied.RevisionShown == Ordinal;
        const float Folded = Shown ? RevisionFoldHeight : 0.0f;
        const float Whole  = RevisionCardHeight + Folded;

        const PlaneExtent Card = Spanning(Run.MinimumX + RevisionLeadX, Y,
                                          Run.Width() - RevisionLeadX - Scaled.StackPadX,
                                          RevisionCardHeight);

        const PlaneExtent Whole2 = Spanning(Card.MinimumX, Y, Card.Width(), Whole);

        // 📐 The bubble and the spine, which the reference applies in two fixed columns of 32 and 15 to the
        //    left of every card and runs continuously between the first card and the last.
        const float Spine = Run.MinimumX + RevisionSpineX;

        if (!Surface->Excluded(Whole2))
        {
            const bool First = Ordinal == 0u;
            const bool Last  = Ordinal + 1u == Current;

            // 📐 The spine is its own flex column and runs the WHOLE row, the card's trailing padding
            //    included — so the gap between two cards carries spine and not ground. It starts at the
            //    first node and stops at the last, rounded at whichever end it terminates.
            const float SpineTop = First ? (Y + 19.0f) : Y;
            const float SpineBottom  = Last  ? (Y + 19.0f) : (Y + Whole + RevisionGapY);

            if (SpineBottom > SpineTop)
            {
                Surface->Ground(Spanning(Spine - 3.0f, SpineTop, 6.0f, SpineBottom - SpineTop),
                                Partial(0xFFFFFFu, 0.10), (First || Last) ? 3.0f : 0.0f);
            }

            // 📐 `w-[7px] h-[7px] rounded-full bg-white shadow-[0_0_0_3px_var(--menu-2)]` — the node, which
            //    sits on the spine 19px into the card and is ringed by the panel's own ground.
            Surface->Ground(Squared(Spine, Y + 19.0f, 13.0f), Tinted.PanelRaised, 6.5f);
            Surface->Ground(Squared(Spine, Y + 19.0f, 7.0f), Tinted.Accent, 3.5f);

            // 📐 The medallion — `{i.toString().padStart(2,'0')}` in a 25px disc.
            const PlaneExtent Medallion = Squared(Run.MinimumX + 16.0f, Y + 19.0f, 25.0f);
            Surface->Ground(Medallion, SecondCurrent ? Tinted.Accent : Partial(0xFFFFFFu, 0.16), 12.5f);

            char Numbered[4] = { static_cast<char>('0' + static_cast<char>((Ordinal / 10u) % 10u)),
                                 static_cast<char>('0' + static_cast<char>(Ordinal % 10u)), '\0', '\0' };

            Surface->TextRun(Medallion.MinimumX +
                             (25.0f - Surface->MeasureRun(Numbered, Scaled.RunFine)) * 0.5f,
                             Medallion.MinimumY + (25.0f - Surface->LineHeight(Scaled.RunFine)) * 0.5f,
                             SecondCurrent ? Tinted.Ground : Tinted.Primary, Numbered, Scaled.RunFine,
                             0.0f, true);
        }

        // 📐 The card itself, pressed to unfold. Only the first `RevisionCeiling` entries carry a cell;
        //    beyond that the pane still draws but no longer arbitrates, which is what a ceiling is for.
        bool Taken = false;

        if (Registered < RevisionCellCeiling)
        {
            Taken = Pressed(RevisionCells[Registered], Card, Applied,
                            Shown ? "Fold this revision" : "Unfold this revision");
            ++Registered;
        }

        if (Taken)
        {
            Applied.RevisionShown = Shown ? LayerStackCeiling::AbsentOrdinal : Ordinal;
            Applied.RevisionField = 0u;
        }

        if (Surface->Excluded(Whole2))
        {
            Y += Whole + RevisionGapY;
            return;
        }

        const bool Lit = Hovered(Card);

        // 📐 `rounded-t-[8px] border-[var(--accent)] bg-[var(--accent-soft)] border-b-transparent` while
        //    unfolded, and a plain rounded tile otherwise.
        Surface->Ground(Card, Shown ? Partial(0xFFFFFFu, 0.07)
                                    : (Lit ? Tinted.RowHovered : Tinted.Row),
                        8.0f, Shown ? (CornerLeadingUpper | CornerTrailingUpper) : CornerAll);
        Surface->Edge(Card, Shown ? Tinted.StrokeStrong : Tinted.Stroke, 1.0f, 8.0f);

        Surface->TextRunTruncated(Card.MinimumX + 8.0f, Card.MinimumY + 8.0f,
                                  Card.Width() - 76.0f, Tinted.Primary, Naming, 12.5f, true);

        if (Detail != nullptr && Detail[0] != '\0')
        {
            Surface->TextRunTruncated(Card.MinimumX + 8.0f, Card.MinimumY + 25.0f,
                                      Card.Width() - 76.0f, Tinted.Secondary, Detail, 10.0f);
        }

        // 📐 `rev.date.toLocaleTimeString(…)` — the trailing moment, in the reference's own monospaced run.
        if (Moment != nullptr && Moment[0] != '\0')
        {
            const float X = Card.MaximumX - 26.0f - Surface->MeasureRun(Moment, 10.0f);
            Surface->TextRun(X, Card.MinimumY + (RevisionCardHeight - Surface->LineHeight(10.0f)) * 0.5f,
                             Tinted.Faint, Moment, 10.0f);
        }

        // 📐 `${isRevExpanded ? 'rotate-180' : ''}` — the card's own chevron points DOWN at rest and turns
        //    a half circle when it unfolds. It is the run's head that swaps to a right chevron, not a card.
        Surface->Stroke(SymbolSubject::ChevronDown,
                        Squared(Card.MaximumX - 14.0f, (Card.MinimumY + Card.MaximumY) * 0.5f, 12.0f),
                        Lit ? Tinted.Secondary : Tinted.Faint, Shown ? 3.14159265f : 0.0f);

        if (!Shown)
        {
            Y += Whole + RevisionGapY;
            return;
        }

        // ③ The fold — author and date over a Comment field over an optional Value field, exactly as the
        //    reference lays it out inside its `grid-template-rows: 1fr` panel.
        const PlaneExtent Fold = Spanning(Card.MinimumX, Card.MaximumY - 1.0f, Card.Width(),
                                          RevisionFoldHeight + 1.0f);

        Surface->Ground(Fold, Partial(0xFFFFFFu, 0.045), 8.0f,
                        CornerTrailingLower | CornerLeadingLower);
        Surface->Edge(Fold, Tinted.StrokeStrong, 1.0f, 8.0f);

        Surface->TextRun(Fold.MinimumX + 8.0f, Fold.MinimumY + 8.0f, Tinted.Secondary,
                         "By System", 10.0f);

        if (Moment != nullptr && Moment[0] != '\0')
        {
            const float X = Fold.MaximumX - 8.0f - Surface->MeasureRun(Moment, 10.0f);
            Surface->TextRun(X, Fold.MinimumY + 8.0f, Tinted.Secondary, Moment, 10.0f);
        }

        // 📐 One field, which takes the keyboard on a press and gives it back on a contact anywhere else —
        //    the same arbitration `#q` carries, because a primitive field has no vendor focus to borrow.
        const auto RecordField = [&](std::uint32_t Which, const PlaneExtent& Bounds, const char* Caption,
                                     char* Written, const char* Absent) -> void
        {
            const bool Holding = Applied.RevisionField == Which;

            if (Registered < RevisionCellCeiling &&
                Pressed(RevisionCells[Registered], Bounds, Applied, nullptr))
            {
                Applied.RevisionField = Holding ? 0u : Which;
            }
            else if (Sampled.ContactPressed && Holding && !Hovered(Bounds))
            {
                // 📝 `onBlur` — the reference writes the reading back exactly here and nowhere else.
                Applied.RevisionField = 0u;
            }

            if (Registered < RevisionCellCeiling)
                ++Registered;

            Surface->Ground(Bounds, Tinted.PanelRaised, Scaled.RadiusSmall);
            Surface->Edge(Bounds, Holding ? Tinted.StrokeStrong : Tinted.Stroke, 1.0f, Scaled.RadiusSmall);

            Surface->TextRunCapitalised(Bounds.MinimumX + 8.0f, Bounds.MinimumY + 6.0f, Tinted.Faint,
                                        Caption, 9.0f, TrackingSection, true);

            const bool Present = Written[0] != '\0';

            Surface->TextRunTruncated(Bounds.MinimumX + 8.0f, Bounds.MinimumY + 19.0f,
                                      Bounds.Width() - 16.0f, Present ? Tinted.Primary : Tinted.Faint,
                                      Present ? Written : Absent, 11.5f);

            if (Holding)
            {
                const float Caret = Bounds.MinimumX + 8.0f +
                                    (Present ? Surface->MeasureRun(Written, 11.5f) : 0.0f);

                Surface->Ground(Spanning(Caret + 1.0f, Bounds.MinimumY + 18.0f, 1.0f, 13.0f),
                                Tinted.Primary);
            }
        };

        // 📐 The retained run is the field's key as well as its seat, so a card beyond the ceiling folds
        //    into the last retained pair rather than writing past the end of either run.
        const std::uint32_t Held = (Ordinal < LayerStackContext::RevisionCeiling)
                                 ? Ordinal : (LayerStackContext::RevisionCeiling - 1u);

        const PlaneExtent Remark = Spanning(Fold.MinimumX + 7.0f, Fold.MinimumY + 24.0f,
                                            Fold.Width() - 14.0f, 36.0f);

        RecordField(Held * 2u + 1u, Remark, "Comment", Applied.RevisionRemark[Held],
                    "Add a comment...");

        // 📐 `{rev.editValue !== undefined && …}` — the Value field stands only where the revision
        //    actually moved a reading, which is exactly where the card carries a detail.
        if (Detail != nullptr && Detail[0] != '\0')
        {
            const PlaneExtent Reading = Spanning(Fold.MinimumX + 7.0f, Remark.MaximumY + 6.0f,
                                                 Fold.Width() - 14.0f, 36.0f);

            RecordField(Held * 2u + 2u, Reading, "Value", Applied.RevisionReading[Held], Detail);
        }

        Y += Whole + RevisionGapY;
    };

    std::uint32_t Ordinal = 0u;

    // 📐 What the artist has actually amended this session stands above the applied reference run, newest
    //    first, so the pane reads as one continuous record rather than as two.
    for (std::uint32_t Recorded = 0u; Recorded < Revisions.RecordedCount(); ++Recorded, ++Ordinal)
        RecordCard(Ordinal, Revisions.RevisionNaming(Recorded), "this session", nullptr, Ordinal == 0u);

    for (std::uint32_t Bounds = 0u; Bounds < Count; ++Bounds, ++Ordinal)
    {
        RecordCard(Ordinal, Reference[Bounds].Naming, Reference[Bounds].Moment, Reference[Bounds].Detail,
                   Ordinal == 0u);
    }

    Applied.RevisionSpan = (Y + Applied.RevisionOffset) - (Run.MinimumY + Scaled.StackPadY);

    Surface->Release();

    // ④ The pane's own bar and wheel, on the same terms the stack's are — the seam carries no scrolling
    //    primitive, so a pane that overflows answers for itself.
    const float Visible = Run.Height();
    const float Ceiling = (Applied.RevisionSpan > Visible) ? (Applied.RevisionSpan - Visible) : 0.0f;

    if (Ceiling > 0.0f && Visible > 0.0f)
    {
        const float Fraction    = Visible / Applied.RevisionSpan;
        const float ThumbHeight = (Visible * Fraction < 28.0f) ? 28.0f : Visible * Fraction;
        const float Travel      = Visible - ThumbHeight;
        const float Advanced    = Applied.RevisionOffset / Ceiling;

        const PlaneExtent Track = Spanning(Run.MaximumX - Scaled.ScrollX, Run.MinimumY,
                                           Scaled.ScrollX, Visible);
        const PlaneExtent Thumb = Spanning(Run.MaximumX - Scaled.ScrollX + 3.0f,
                                           Run.MinimumY + Travel * Advanced, 4.0f, ThumbHeight);

        ControlIdentity& Target = ChromeCells[static_cast<std::uint32_t>(ChromeCell::RevisionBar)];

        if (Hovered(Track) && Sampled.ContactPressed && !Ledger->AnyDisclosed())
        {
            Ledger->Grab(Target, ControlPart::Thumb);
            Ledger->RecordInitial(Target, Applied.RevisionOffset);
        }

        Ledger->DeclareHovered(Target, Hovered(Track), HoverOver);

        if (Ledger->Holding(Target) && Travel > 0.0f)
        {
            const Outcome<float> Previous = Ledger->InitialReading(Target);

            if (Previous.Resolved)
            {
                const float Moved  = Sampled.PositionY - Ledger->OriginY();
                Applied.RevisionOffset = Previous.Resolve() + Moved * (Ceiling / Travel);
            }
        }

        Surface->Ground(Thumb, Partial(0xFFFFFFu, Ledger->Holding(Target) ? 0.30 : 0.15), 2.0f);
    }

    if (Run.Encloses(Sampled.PositionX, Sampled.PositionY) && Applied.Popup == StackPopup::Absent)
        Applied.RevisionOffset -= Sampled.WheelY * NotchHeight;

    if (Applied.RevisionOffset < 0.0f)      Applied.RevisionOffset = 0.0f;
    if (Applied.RevisionOffset > Ceiling)   Applied.RevisionOffset = Ceiling;

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DEFERRED SWEEP
//------------------------------------------------------------------------------------------------------------------------

// 📐 `hex2hsl` / `hsl2hex`, transcribed. The wheel is authored in hue-saturation-luminance because that is
//    what a ring and a radius actually are; the arrangement retains packed sRGB, so the pair converts.
static void SeparateTint(std::uint32_t Packed, float& Hue, float& Saturation, float& Luminance)
{
    const float Red   = static_cast<float>((Packed >> 16) & 0xFFu) / 255.0f;
    const float Green = static_cast<float>((Packed >>  8) & 0xFFu) / 255.0f;
    const float Blue  = static_cast<float>( Packed        & 0xFFu) / 255.0f;

    const float Maximum  = (Red > Green ? (Red > Blue ? Red : Blue) : (Green > Blue ? Green : Blue));
    const float Minimum = (Red < Green ? (Red < Blue ? Red : Blue) : (Green < Blue ? Green : Blue));

    Luminance = (Maximum + Minimum) * 0.5f;
    Hue       = 0.0f;
    Saturation = 0.0f;

    if (Maximum != Minimum)
    {
        const float Span = Maximum - Minimum;

        Saturation = (Luminance > 0.5f) ? (Span / (2.0f - Maximum - Minimum)) : (Span / (Maximum + Minimum));

        if (Maximum == Red)        Hue = (Green - Blue) / Span + (Green < Blue ? 6.0f : 0.0f);
        else if (Maximum == Green) Hue = (Blue - Red) / Span + 2.0f;
        else                    Hue = (Red - Green) / Span + 4.0f;

        Hue *= 60.0f;
    }

    Saturation *= 100.0f;
    Luminance  *= 100.0f;
}

static std::uint32_t CombineTint(float Hue, float Saturation, float Luminance)
{
    const float Sat = Saturation * 0.01f;
    const float Lum = Luminance  * 0.01f;
    const float Amplitude = Sat * ((Lum < 1.0f - Lum) ? Lum : (1.0f - Lum));

    const auto Channel = [&](float Offset) -> std::uint32_t
    {
        float Turned = Offset + Hue / 30.0f;

        while (Turned >= 12.0f) Turned -= 12.0f;
        while (Turned <    0.0f) Turned += 12.0f;

        const float Lower   = (Turned - 3.0f < 9.0f - Turned) ? (Turned - 3.0f) : (9.0f - Turned);
        const float Bounded = (Lower < 1.0f) ? Lower : 1.0f;
        const float Written = Lum - Amplitude * ((Bounded > -1.0f) ? Bounded : -1.0f);
        const float Scaled8 = Written * 255.0f + 0.5f;

        return static_cast<std::uint32_t>((Scaled8 < 0.0f) ? 0.0f : ((Scaled8 > 255.0f) ? 255.0f : Scaled8));
    };

    return (Channel(0.0f) << 16) | (Channel(8.0f) << 8) | Channel(4.0f);
}

void LayerStackPanel::RecordDeferred(LayerArrangement& Arrangement, LayerStackContext& Applied,
                                     RevisionSequence& Revisions)
{
    if (Surface == nullptr || Ledger == nullptr || !Surface->Recording())
        return;

    // ① The veil. A contact anywhere outside the standing popup dismisses it, exactly as the reference's
    //    document-level `pointerdown` does — and it dismisses it WITHOUT the contact reaching a row.
    if (Applied.Popup != StackPopup::Absent && Sampled.ContactPressed)
    {
        // 📐 Tested against where the card was actually RECORDED last tick, not against its anchor.
        const PlaneExtent Card = Spanning(Applied.PopupX, Applied.PopupY,
                                          PopupLeft, Applied.PopupSpan);

        if (!Card.Encloses(Sampled.PositionX, Sampled.PositionY))
        {
            Applied.Popup = StackPopup::Absent;
            Ledger->Withdraw();
        }
    }

    if (Applied.Popup == StackPopup::Absent)
    {
        Applied.PopupSettled = false;
    }
    else
    {
        // 📝 The card's extent is counted from its declared run BEFORE the ground is recorded, because the
        //    ground has to be laid first — a vendor command list is ordered, so a ground recorded after its
        //    entries would paint over them.

        const std::uint32_t Subject = (Applied.PopupSubject < Arrangement.EntryCount)
                                    ? Applied.PopupSubject : Arrangement.Taken;
        const bool          Present = Subject < Arrangement.EntryCount;

        std::uint32_t Entries  = 0u;
        std::uint32_t Captions = 0u;
        std::uint32_t Rules    = 0u;

        switch (Applied.Popup)
        {
            case StackPopup::Addition:    Entries = 7u;  Captions = 1u; Rules = 2u; break;
            case StackPopup::BlendMode:   Entries = 29u; Captions = 1u; Rules = 0u; break;
            case StackPopup::LayerMenu:   Entries = 9u;  Captions = 2u; Rules = 3u; break;
            case StackPopup::MaskMenu:    Entries = 20u; Captions = 4u; Rules = 3u; break;
            case StackPopup::EffectMenu:  Entries = 14u; Captions = 1u; Rules = 0u; break;
            case StackPopup::ColourWheel: Entries = 0u;  Captions = 1u; Rules = 0u; break;
            default:                                                                break;
        }

        float Measured = PopupPad * 2.0f + static_cast<float>(Entries) * PopupEntryY
                       + static_cast<float>(Captions) * PopupCaption + static_cast<float>(Rules) * 8.0f;

        if (Applied.Popup == StackPopup::LayerMenu)
            Measured += 30.0f;   // 📐 the swatch strip

        if (Applied.Popup == StackPopup::ColourWheel)
            Measured += 244.0f;  // 📐 the ring, its luminance run and its hexadecimal row

        // 📐 A run taller than the display scrolls inside its own card rather than running off it, which is
        //    what the twenty-nine blend modes need on a short display.
        const float DisplayY = (Surface->Display().Height > 0.0f)
                                  ? Surface->Display().Height : 1080.0f;
        const float Ceiling       = DisplayY - 16.0f;
        const float Current      = (Measured < Ceiling) ? Measured : ((Ceiling > 80.0f) ? Ceiling : 80.0f);

        const PlaneExtent Anchored = RecordPopupGround(Applied, Applied.PopupX, Applied.PopupHeight,
                                                       Current);
        const PlaneExtent Card     = Spanning(Anchored.MinimumX, Anchored.MinimumY,
                                              PopupLeft, Current);

        Applied.PopupSpan = Current;

        Surface->Ground(Card, Tinted.PanelRaised, Scaled.RadiusStandard);
        Surface->Edge(Card, Tinted.StrokeStrong, 1.0f, Scaled.RadiusStandard);
        Surface->Confine(Card);

        float Y = Anchored.MinimumY + PopupPad;

        const auto Caption = [&](const char* Written)
        {
            Surface->TextRunCapitalised(Anchored.MinimumX + 14.0f,
                                        Y + (PopupCaption - Surface->LineHeight(Scaled.RunSection)) * 0.5f,
                                        Tinted.Faint, Written, Scaled.RunSection, TrackingSection, true);
            Y += PopupCaption;
        };

        const auto Rule = [&]()
        {
            Surface->Ground(PlaneExtent{ Anchored.MinimumX + 6.0f, Y + 3.0f,
                                         Anchored.MaximumX - 6.0f, Y + 4.0f }, Tinted.Stroke);
            Y += 8.0f;
        };

        const auto Entry = [&](const char* Written, const char* Chord, bool Marked, bool Dangerous) -> bool
        {
            const PlaneExtent Candidate0 = Spanning(Anchored.MinimumX + PopupPad, Y,
                                                 PopupLeft - PopupPad * 2.0f, PopupEntryY);
            Y += PopupEntryY;
            return RecordPopupEntry(Candidate0, Written, Chord, Marked, Dangerous, Applied);
        };

        if (Measured > Current && Card.Encloses(Sampled.PositionX, Sampled.PositionY))
        {
            Applied.PopupOffset -= Sampled.WheelY * NotchHeight;

            const float Travel = Measured - Current;

            if (Applied.PopupOffset < 0.0f)    Applied.PopupOffset = 0.0f;
            if (Applied.PopupOffset > Travel)  Applied.PopupOffset = Travel;
        }
        else if (Measured <= Current)
        {
            Applied.PopupOffset = 0.0f;
        }

        Y -= Applied.PopupOffset;

        switch (Applied.Popup)
        {
            // ⓐ `#btnAdd` — the seven declarations, with their chords.
            case StackPopup::Addition:
            {
                struct Declaration { const char* Naming; const char* Chord; LayerContent Content; };

                static const Declaration Offered[7] =
                {
                    { "Paint layer",       "P",  LayerContent::Paint      },
                    { "Fill layer",        "F",  LayerContent::Fill       },
                    { "Adjustment",        "A",  LayerContent::Adjustment },
                    { "Filter",            "R",  LayerContent::Retention  },
                    { "Decal layer \xC2\xB7 3D", "D", LayerContent::Decal  },
                    { "Pattern layer",     "T",  LayerContent::Pattern    },
                    { "Group",             "G",  LayerContent::Folder     }
                };

                static const char* const Declared[7] =
                {
                    "Paint Layer", "Fill Layer", "Adjustment", "Filter",
                    "Decal Layer", "Pattern Layer", "New Folder"
                };

                Caption("Add");

                for (std::uint32_t Ordinal = 0u; Ordinal < 7u; ++Ordinal)
                {
                    if (Ordinal == 4u || Ordinal == 6u)
                        Rule();

                    if (Entry(Offered[Ordinal].Naming, Offered[Ordinal].Chord, false, false))
                    {
                        Revisions.Record(Arrangement, "Entry declared");
                        DeclareEntry(Arrangement, Offered[Ordinal].Content, Declared[Ordinal]);
                    }
                }

                break;
            }

            // ⓑ `#btnBlend` — the blend run the taken half accepts.
            case StackPopup::BlendMode:
            {
                if (!Present)
                    break;

                LayerEntry& Taken   = Arrangement.Entries[Subject];
                const bool  OnMask  = Applied.PopupOnMask && Taken.Mask.Declared;
                const char* CurrentBlend = OnMask ? Taken.Mask.Blend : Taken.Blend;

                std::uint32_t       Count   = 0u;
                const char* const*  Offered = BlendNaming(0xFFFFFFFFu, Count);

                Caption("Blend mode");

                for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
                {
                    const bool Marked = std::strcmp(Offered[Ordinal], CurrentBlend) == 0;

                    if (Entry(Offered[Ordinal], nullptr, Marked, false))
                    {
                        Revisions.Record(Arrangement, "Blend restated");

                        if (OnMask) Arrangement.Entries[Subject].Mask.Blend = Offered[Ordinal];
                        else        Arrangement.Entries[Subject].Blend      = Offered[Ordinal];
                    }
                }

                break;
            }

            // ⓒ One entry's own menu.
            case StackPopup::LayerMenu:
            {
                if (!Present)
                    break;

                const LayerEntry& Taken = Arrangement.Entries[Subject];

                Caption(Taken.Naming);

                if (Entry("Rename", "F2", false, false))
                {
                    Applied.Renaming = Subject;
                    std::snprintf(Applied.RenamingRun, sizeof Applied.RenamingRun, "%s", Taken.Naming);
                }

                if (Entry(Taken.Unfolded ? "Hide details" : "Show details", "Space", false, false))
                    Arrangement.Entries[Subject].Unfolded = !Taken.Unfolded;

                if (Entry(Taken.Mask.Declared ? "Remove mask" : "Add mask", "M", false, false))
                {
                    Revisions.Record(Arrangement, "Mask amended");
                    Arrangement.Taken = Subject;

                    if (!ToggleMask(Arrangement))
                        Revisions.Revert(Arrangement);
                }

                if (Entry(Taken.Secured ? "Unlock" : "Lock", "L", false, false))
                {
                    Revisions.Record(Arrangement, "Entry secured");
                    Arrangement.Entries[Subject].Secured = !Taken.Secured;
                }

                if (Entry(Arrangement.Soloed == Subject ? "Clear solo" : "Solo", "S", false, false))
                {
                    Arrangement.Soloed = (Arrangement.Soloed == Subject)
                                       ? LayerStackCeiling::AbsentOrdinal : Subject;
                }

                if (Entry("Duplicate", "Cmd D", false, false))
                {
                    Revisions.Record(Arrangement, "Entry copied");
                    Arrangement.Taken = Subject;

                    if (!DuplicateTaken(Arrangement))
                        Revisions.Revert(Arrangement);
                }

                if (Entry("Group", "Cmd G", false, false))
                {
                    Revisions.Record(Arrangement, "Entries grouped");
                    Arrangement.Taken = Subject;

                    if (!EncloseTaken(Arrangement))
                        Revisions.Revert(Arrangement);
                }

                Rule();
                Caption("Colour tag");

                // 📐 `<div class="swatches">` — ten 18px swatches, the standing one ringed.
                {
                    const std::uint32_t* const Offered = AppliedColourTags();
                    const float Step = (PopupLeft - PopupPad * 2.0f - 12.0f) /
                                       static_cast<float>(LayerStackCeiling::ColourTags);

                    for (std::uint32_t Ordinal = 0u; Ordinal < LayerStackCeiling::ColourTags; ++Ordinal)
                    {
                        const PlaneExtent Swatch = Spanning(Anchored.MinimumX + PopupPad + 6.0f +
                                                            Step * static_cast<float>(Ordinal),
                                                            Y + 4.0f, Step - 2.0f, 18.0f);

                        Surface->Ground(Swatch, Covering(Offered[Ordinal]), 4.0f);

                        if (Taken.ColourTag == Offered[Ordinal])
                            Surface->Edge(Swatch, Tinted.Accent, 1.5f, 4.0f);

                        if (Swatch.Encloses(Sampled.PositionX, Sampled.PositionY) &&
                            Sampled.ContactReleased)
                        {
                            Revisions.Record(Arrangement, "Colour tag amended");
                            Arrangement.Entries[Subject].ColourTag = Offered[Ordinal];
                            Applied.Popup = StackPopup::Absent;
                            Ledger->Withdraw();
                        }
                    }

                    Y += 30.0f;
                }

                if (Entry("Custom colour...", nullptr, false, false))
                {
                    SeparateTint(Taken.ColourTag, Applied.WheelHue, Applied.WheelSaturation,
                                 Applied.WheelLuminance);

                    Applied.Popup        = StackPopup::ColourWheel;
                    Applied.PopupSubject = Subject;
                    Applied.PopupOffset  = 0.0f;
                    Ledger->Disclose(ChromeCells[static_cast<std::uint32_t>(ChromeCell::PopupBody)]);
                }

                Rule();

                if (Entry("Delete", "Del", false, true))
                {
                    Revisions.Record(Arrangement, "Entry retired");
                    Arrangement.Taken     = Subject;
                    Arrangement.TakenHalf = LayerTaken::Layer;

                    if (!RetireTaken(Arrangement))
                        Revisions.Revert(Arrangement);
                }

                break;
            }

            // ⓓ One mask's own menu — source, generator, invert, clear.
            case StackPopup::MaskMenu:
            {
                if (!Present || !Arrangement.Entries[Subject].Mask.Declared)
                    break;

                const MaskCoordinate& Mask = Arrangement.Entries[Subject].Mask;

                Caption("Mask");

                if (Entry(Mask.Unfolded ? "Hide details" : "Show details", nullptr, false, false))
                    Arrangement.Entries[Subject].Mask.Unfolded = !Mask.Unfolded;

                if (Entry("Invert", nullptr, Mask.Inverted, false))
                {
                    Revisions.Record(Arrangement, "Mask inverted");
                    Arrangement.Entries[Subject].Mask.Inverted = !Mask.Inverted;
                }

                Rule();
                Caption("Source");

                // 📐 `SRCLIST.filter(s=>s!=='Generator')` — the generator reaches the run below instead.
                for (std::uint32_t Ordinal = 0u; Ordinal < 6u; ++Ordinal)
                {
                    const auto Offered = static_cast<MaskSource>(Ordinal);

                    if (Entry(SourceNaming(Offered), nullptr, Mask.Source == Offered, false))
                    {
                        Revisions.Record(Arrangement, "Mask source restated");
                        Arrangement.Entries[Subject].Mask.Source = Offered;
                    }
                }

                Rule();
                Caption("Generator");

                // 📐 `GENLIST` — the eight the reference declares, in its own order.
                static const char* const Generators[8] =
                {
                    "Curvature", "Ambient Occlusion", "Dirt", "Metal Edge Wear",
                    "Position", "Mask Editor", "Light", "Panel Lines"
                };

                for (const char* const Offered : Generators)
                {
                    const bool Marked = Mask.Source == MaskSource::Generator &&
                                        Mask.Generator != nullptr &&
                                        std::strcmp(Mask.Generator, Offered) == 0;

                    if (Entry(Offered, nullptr, Marked, false))
                    {
                        Revisions.Record(Arrangement, "Mask generator restated");
                        Arrangement.Entries[Subject].Mask.Source    = MaskSource::Generator;
                        Arrangement.Entries[Subject].Mask.Generator = Offered;
                    }
                }

                Rule();

                if (Entry("Clear mask effects", nullptr, false, false))
                {
                    Revisions.Record(Arrangement, "Mask effects cleared");
                    Arrangement.Entries[Subject].Mask.EffectCount = 0u;
                }

                if (Entry("Delete mask", nullptr, false, true))
                {
                    Revisions.Record(Arrangement, "Mask retired");
                    Arrangement.Entries[Subject].Mask = MaskCoordinate{};
                    Arrangement.TakenHalf             = LayerTaken::Layer;
                }

                break;
            }

            // ⓔ The effect run a card's `+` offers.
            case StackPopup::EffectMenu:
            {
                if (!Present)
                    break;

                std::uint32_t      Count   = 0u;
                const char* const* Offered = EffectNaming(Count);

                Caption("Add effect");

                for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
                {
                    if (!Entry(Offered[Ordinal], nullptr, false, false))
                        continue;

                    Revisions.Record(Arrangement, "Effect declared");

                    if (Applied.PopupOnMask && Arrangement.Entries[Subject].Mask.Declared)
                    {
                        MaskCoordinate& Mask = Arrangement.Entries[Subject].Mask;

                        if (Mask.EffectCount < LayerStackCeiling::Effects)
                            Mask.Effects[Mask.EffectCount++] = Offered[Ordinal];
                        else
                            Revisions.Revert(Arrangement);
                    }
                    else
                    {
                        LayerEntry& Taken = Arrangement.Entries[Subject];

                        if (Taken.EffectCount < LayerStackCeiling::Effects)
                            Taken.Effects[Taken.EffectCount++] = Offered[Ordinal];
                        else
                            Revisions.Revert(Arrangement);
                    }
                }

                break;
            }

            // ⓕ The colour wheel — a hue ring, a luminance run and an Apply.
            case StackPopup::ColourWheel:
            {
                Caption("Custom colour");

                // 📐 `.wheel` — a 158px disc whose angle is hue and whose radius, out to `r-9`, is
                //    saturation. It is recorded as concentric arcs of medallions rather than as a texture,
                //    because the recording seam carries no per-pixel primitive and the ring is the only
                //    thing in the panel that needs one.
                const float Ring   = 158.0f;
                const float Radius = Ring * 0.5f;
                const float Centre = Anchored.MinimumX + PopupLeft * 0.5f;
                const float Middle = Y + Radius + 4.0f;

                Surface->Ground(Squared(Centre, Middle, Ring), Covering(0x0A0A0Au), Radius);

                // 📐 Sixty spokes of ten stops each. Six hundred medallions is what a 158px ring needs to
                //    read as continuous; fewer bands and the saturation axis visibly steps.
                for (std::uint32_t Spoke = 0u; Spoke < 60u; ++Spoke)
                {
                    const float Turn = static_cast<float>(Spoke) * 6.0f;
                    const float Sine = Turn;

                    for (std::uint32_t Band = 0u; Band < 10u; ++Band)
                    {
                        const float Away = (static_cast<float>(Band) + 0.5f) * 0.1f;
                        const float Sat  = Away * 100.0f;
                        const auto  Tint = CombineTint(Sine, Sat, Applied.WheelLuminance);

                        // 📐 sin and cos from the turn, without <cmath>: the ring is sixty fixed spokes, so
                        //    the two are read from a stated quarter-turn run rather than computed.
                        static const float Quarter[16] =
                        {
                            0.0000f, 0.1045f, 0.2079f, 0.3090f, 0.4067f, 0.5000f, 0.5878f, 0.6691f,
                            0.7431f, 0.8090f, 0.8660f, 0.9135f, 0.9511f, 0.9781f, 0.9945f, 1.0000f
                        };

                        const std::uint32_t Step  = Spoke;                       // 6 degrees each
                        const std::uint32_t Ordinal = Step % 60u;
                        const auto Sines = [&](std::uint32_t Which) -> float
                        {
                            const std::uint32_t Folded = Which % 60u;

                            if (Folded <= 15u) return  Quarter[Folded];
                            if (Folded <= 30u) return  Quarter[30u - Folded];
                            if (Folded <= 45u) return -Quarter[Folded - 30u];
                            return                    -Quarter[60u - Folded];
                        };

                        const float X  = Centre + Sines((Ordinal + 15u) % 60u) * (Radius - 9.0f) * Away;
                        const float Y0 = Middle + Sines(Ordinal) * (Radius - 9.0f) * Away;

                        Surface->Medallion(X, Y0, 5.0f, Covering(Tint));
                    }
                }

                // 📐 The dot, at the standing hue and saturation.
                {
                    static const float Quarter[16] =
                    {
                        0.0000f, 0.1045f, 0.2079f, 0.3090f, 0.4067f, 0.5000f, 0.5878f, 0.6691f,
                        0.7431f, 0.8090f, 0.8660f, 0.9135f, 0.9511f, 0.9781f, 0.9945f, 1.0000f
                    };

                    const auto Sines = [&](float Degrees) -> float
                    {
                        float Turned = Degrees;
                        while (Turned >= 360.0f) Turned -= 360.0f;
                        while (Turned <    0.0f) Turned += 360.0f;

                        const auto Step = static_cast<std::uint32_t>(Turned / 6.0f) % 60u;

                        if (Step <= 15u) return  Quarter[Step];
                        if (Step <= 30u) return  Quarter[30u - Step];
                        if (Step <= 45u) return -Quarter[Step - 30u];
                        return                  -Quarter[60u - Step];
                    };

                    const float Away  = Applied.WheelSaturation * 0.01f * (Radius - 9.0f);
                    const float X = Centre + Sines(Applied.WheelHue + 90.0f) * Away;
                    const float Down  = Middle + Sines(Applied.WheelHue) * Away;
                    const auto  Tint  = CombineTint(Applied.WheelHue, Applied.WheelSaturation,
                                                    Applied.WheelLuminance);

                    Surface->Medallion(X, Down, 7.0f, Covering(0xFFFFFFu));
                    Surface->Medallion(X, Down, 5.5f, Covering(Tint));

                    // 📐 A contact inside the ring resolves hue from its angle and saturation from its
                    //    radius, and keeps resolving while it is held — which is what makes it a wheel.
                    const PlaneExtent Disc = Squared(Centre, Middle, Ring);

                    if (Disc.Encloses(Sampled.PositionX, Sampled.PositionY) &&
                        (Sampled.ContactPressed || Sampled.ContactHeld))
                    {
                        const float OffX  = Sampled.PositionX  - Centre;
                        const float OffY = Sampled.PositionY - Middle;

                        // 📐 The angle, resolved by walking the same sixty spokes rather than by an
                        //    arc-tangent — the ring has sixty stops, so sixty comparisons resolve it
                        //    exactly and pull in no further dependency.
                        float Nearest = 0.0f;
                        float Closest = 1.0e30f;

                        for (std::uint32_t Step = 0u; Step < 60u; ++Step)
                        {
                            const float Turn      = static_cast<float>(Step) * 6.0f;
                            const float SpokeDown = Sines(Turn);
                            const float SpokeHeight = Sines(Turn + 90.0f);
                            const float Away2     = OffX * SpokeHeight + OffY * SpokeDown;

                            if (Away2 <= 0.0f)
                                continue;

                            const float Off = (OffX - SpokeHeight * Away2) * (OffX - SpokeHeight * Away2)
                                            + (OffY - SpokeDown * Away2) * (OffY - SpokeDown * Away2);

                            if (Off < Closest)
                            {
                                Closest = Off;
                                Nearest = Turn;
                            }
                        }

                        const float Extent2 = OffX * OffX + OffY * OffY;
                        float       Reach   = 0.0f;

                        // 📐 The radius, by bisection over the squared extent — the same reason as above.
                        for (float Probe = 0.0f; Probe <= 1.0f; Probe += 0.01f)
                        {
                            const float Span = Probe * (Radius - 9.0f);

                            if (Span * Span <= Extent2)
                                Reach = Probe;
                        }

                        Applied.WheelHue        = Nearest;
                        Applied.WheelSaturation = Reach * 100.0f;
                    }
                }

                Y = Middle + Radius + 10.0f;

                // 📐 `#cwL` — the luminance run, 4…96.
                {
                    const PlaneExtent Track = Spanning(Anchored.MinimumX + 44.0f, Y,
                                                       PopupLeft - 56.0f, 16.0f);

                    Surface->TextRunCapitalised(Anchored.MinimumX + 12.0f,
                                                Y + 8.0f - Surface->LineHeight(Scaled.RunSection) * 0.5f,
                                                Tinted.Faint, "Lum", Scaled.RunSection, TrackingSection, true);

                    auto Reading = static_cast<std::uint32_t>(Applied.WheelLuminance);

                    if (Dragged(ChromeCells[static_cast<std::uint32_t>(ChromeCell::WheelLuma)],
                                Track, Reading))
                    {
                        const auto Bounded = (Reading < 4u) ? 4u : ((Reading > 96u) ? 96u : Reading);
                        Applied.WheelLuminance = static_cast<float>(Bounded);
                    }

                    RecordMeter(PlaneExtent{ Track.MinimumX, Y + 6.5f, Track.MaximumX, Y + 9.5f },
                                static_cast<std::uint32_t>(Applied.WheelLuminance), Tinted.Accent);

                    Y += 26.0f;
                }

                // 📐 `#cwPrev` and `#cwOk` — the resolved tint beside the Apply.
                {
                    const auto Tint = CombineTint(Applied.WheelHue, Applied.WheelSaturation,
                                                  Applied.WheelLuminance);

                    Surface->Ground(Spanning(Anchored.MinimumX + 12.0f, Y, 28.0f, 24.0f),
                                    Covering(Tint), Scaled.RadiusSmall);

                    char Written[16] = {};
                    std::snprintf(Written, sizeof Written, "#%06X", Tint);
                    Surface->TextRun(Anchored.MinimumX + 48.0f,
                                     Y + 12.0f - Surface->LineHeight(Scaled.RunSub) * 0.5f,
                                     Tinted.Secondary, Written, Scaled.RunSub);

                    const PlaneExtent Apply = Spanning(Anchored.MaximumX - 74.0f, Y, 62.0f, 24.0f);
                    const bool        Over  = Apply.Encloses(Sampled.PositionX, Sampled.PositionY);

                    Surface->Ground(Apply, Over ? Tinted.Accent : Partial(0xFFFFFFu, 0.09),
                                    Scaled.RadiusSmall);
                    Surface->TextRun(Apply.MinimumX + 16.0f,
                                     Y + 12.0f - Surface->LineHeight(Scaled.RunSub) * 0.5f,
                                     Over ? Covering(0x000000u) : Tinted.Primary, "Apply", Scaled.RunSub,
                                     0.0f, true);

                    if (Over && Sampled.ContactReleased && Present)
                    {
                        Revisions.Record(Arrangement, "Colour tag amended");
                        Arrangement.Entries[Subject].ColourTag = Tint;
                        Applied.Popup = StackPopup::Absent;
                        Ledger->Withdraw();
                    }

                    Y += 32.0f;
                }

                break;
            }

            default:
                break;
        }

        Surface->Release();

        // 🔴 Set LAST. A popup opened during this very sweep must not resolve one of its own entries under
        //    the same contact — it becomes pickable on the next tick and not before.
        if (Applied.Popup != StackPopup::Absent)
            Applied.PopupSettled = true;
    }

    // ② The tooltip, above even the popup, because it may name one of the popup's own entries.
    if (Applied.Tooltip != nullptr && Applied.Tooltip[0] != '\0' && Applied.Popup == StackPopup::Absent)
    {
        const float X  = Surface->MeasureRun(Applied.Tooltip, Scaled.RunFine) + 16.0f;
        const float Y = 22.0f;

        float Bounds = Applied.TooltipX - X * 0.5f;
        float Over = Applied.TooltipHeight - Y - 6.0f;

        const float Width = (Surface->Display().Width > 0.0f)
                                ? Surface->Display().Width : 1920.0f;

        if (Bounds < 8.0f)                     Bounds = 8.0f;
        if (Bounds > Width - X - 8.0f) Bounds = Width - X - 8.0f;
        if (Over < 8.0f)                     Over = Applied.TooltipHeight + 24.0f;

        const PlaneExtent Card = Spanning(Bounds, Over, X, Y);

        Surface->Ground(Card, Covering(0x1A1A1Au), Scaled.RadiusSmall);
        Surface->Edge(Card, Tinted.StrokeStrong, 1.0f, Scaled.RadiusSmall);
        Surface->TextRun(Bounds + 8.0f, Over + (Y - Surface->LineHeight(Scaled.RunFine)) * 0.5f,
                         Tinted.Primary, Applied.Tooltip, Scaled.RunFine);
    }
}

}   // namespace Slate
