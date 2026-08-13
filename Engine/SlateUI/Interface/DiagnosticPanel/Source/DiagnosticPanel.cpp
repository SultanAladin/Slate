//============================================================================================================================================
//                                                          DIAGNOSTICPANEL.CPP
//============================================================================================================================================
// 🧩 Seven declared classes presented at three weights, and the measures beneath them — nothing inferred, nothing held.

#include "SlateUI/Interface/DiagnosticPanel/Api/DiagnosticPanel.h"

#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"

#include "imgui.h"

#include <cstdio>
#include <vector>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE CLASS TABLE
//------------------------------------------------------------------------------------------------------------------------

ReportWeight WeightOf(ReportDisposition Declared)
{
    switch (Declared)
    {
        // 🔴 `86` §5's table, verbatim. A refusal produced nothing and a failure completed nothing; both are the
        //    artist's problem. Everything above them is the engine narrating its own ordinary operation.
        case ReportDisposition::Refused:
        case ReportDisposition::Failed:
            return ReportWeight::Problem;

        // ⚠️ `02` §5 requires a Convergent computation to report which of criterion and ceiling ended it, and a
        //    ceiling means the result is the best available rather than wrong. Presented as a problem it makes
        //    artists avoid the operation entirely, which is worse than the termination it warned about.
        case ReportDisposition::Terminated:
            return ReportWeight::Ambiguous;

        case ReportDisposition::Measured:
        case ReportDisposition::Assumed:
        case ReportDisposition::Amended:
        case ReportDisposition::Truncated:
        default:
            return ReportWeight::Information;
    }
}

std::uint32_t PresentedRankOf(ReportDisposition Declared)
{
    switch (Declared)
    {
        case ReportDisposition::Failed:     return 0u;
        case ReportDisposition::Refused:    return 1u;
        case ReportDisposition::Terminated: return 2u;
        // 📝 ⚠️ Third, above every other informational class. `56` §3.1's resampling report is an amendment and is
        //    the one operation that resamples authored content; ranked with the residency totals it is a line the
        //    artist scrolls past before discovering their paint softened.
        case ReportDisposition::Amended:    return 3u;
        case ReportDisposition::Truncated:  return 4u;
        case ReportDisposition::Assumed:    return 5u;
        case ReportDisposition::Measured:   return 6u;
        default:                            return 7u;
    }
}

const char* CaptionOf(ReportDisposition Declared)
{
    switch (Declared)
    {
        case ReportDisposition::Measured:   return "Measures";
        case ReportDisposition::Assumed:    return "Assumptions";
        case ReportDisposition::Amended:    return "Amendments";
        case ReportDisposition::Truncated:  return "Truncations";
        case ReportDisposition::Refused:    return "Refusals";
        case ReportDisposition::Terminated: return "Terminations";
        case ReportDisposition::Failed:     return "Failures";
        default:                            return "Undeclared";
    }
}

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE THREE HUES
//------------------------------------------------------------------------------------------------------------------------

// 📝 Three colours for three weights, all read from the palette. A fourth hue for a fourth weight would be a
//    colour literal spelled in a panel, which is the defect `ThemeSpecification` exists to prevent.
ThemeColour HueOf(const ThemePalette& Palette, ReportWeight Weighted)
{
    if (Weighted == ReportWeight::Problem)
        return Palette.DangerPrimary;

    if (Weighted == ReportWeight::Ambiguous)
        return Palette.AccentPrimary;

    return Palette.TextMuted;
}

constexpr float EntryHeight   = 38.0f;   // [px] - one report entry, origin line and detail line together
constexpr float MeasureHeight = 22.0f;   // [px] - one sampled measure, a single line
constexpr float CountBadge    = 28.0f;   // [px] - the occurrence count cap at an entry's right

//------------------------------------------------------------------------------------------------------------------------
//                                                       ONE ENTRY
//------------------------------------------------------------------------------------------------------------------------

void PresentEntry(const ThemeSpecification&   Theme,
                  const WorkspaceRectangle&   Area,
                  const ReportSpecification&  Standing)
{
    const ThemePalette&  Palette  = Theme.Palette;
    const LayoutExtents& Extents  = Theme.Extents;
    const ReportWeight   Weighted = WeightOf(Standing.Disposition);
    const ThemeColour    Hue      = HueOf(Palette, Weighted);

    const bool Covered = RectangleCovers(Area, ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);

    PresentSurfaceFill(Area, Covered ? Palette.TileHovered : Palette.TileBackground, Extents.CornerRounding * 0.5f);

    // 📝 A problem carries a hued rail and the other two weights do not. A badge on every entry would make the
    //    quiet classes as loud as the refusals, which is `86` §5 undone by decoration.
    if (Weighted != ReportWeight::Information)
    {
        WorkspaceRectangle Rail = Area;
        Rail.Width = 3.0f;

        PresentSurfaceFill(Rail, Hue, 1.5f);
    }

    WorkspaceRectangle Leading;
    Leading.PositionX = Area.PositionX + Extents.PanelPadding;
    Leading.PositionY = Area.PositionY + 2.0f;
    Leading.Width     = Area.Width - Extents.PanelPadding * 2.0f - CountBadge;
    Leading.Height    = EntryHeight * 0.5f;

    // 📝 🔴 Origin and subject on the first line, together. `52` §2 promises the artist the construct **and** its
    //    position, and the register coalesces by origin, class and subject together precisely so that promise
    //    survives — presenting only the origin would destroy exactly what the coalescing rule preserved.
    char Named[192] = {};

    if (Standing.SubjectOrdinal != 0u)
    {
        std::snprintf(Named, sizeof Named, "%s  ·  %s  #%llu", Standing.Origin, Standing.Subject,
                      static_cast<unsigned long long>(Standing.SubjectOrdinal));
    }
    else
    {
        std::snprintf(Named, sizeof Named, "%s  ·  %s", Standing.Origin, Standing.Subject);
    }

    PresentTextRun(Leading, Named, Weighted == ReportWeight::Problem ? Hue : Palette.TextPrimary,
                   TextPlacement::Leading, 0.95f);

    WorkspaceRectangle Trailing = Leading;
    Trailing.PositionY = Area.PositionY + EntryHeight * 0.5f - 2.0f;

    // 📝 The detail is the reason verbatim, as its origin spelled it. Rewording it here would put the panel's
    //    words in front of the mechanism's, and the mechanism is the one that knows what happened.
    PresentTextRun(Trailing, Standing.Detail, Palette.TextMuted, TextPlacement::Leading, 0.9f);

    if (Standing.OccurrenceCount <= 1u)
        return;

    // 📝 ⚠️ A count and never N entries — `86` §6. The register coalesced them at append, so this is a read.
    WorkspaceRectangle Badge;
    Badge.PositionX = Area.PositionX + Area.Width - Extents.PanelPadding - CountBadge;
    Badge.PositionY = Area.PositionY + (Area.Height - Extents.SegmentRowHeight) * 0.5f;
    Badge.Width     = CountBadge;
    Badge.Height    = Extents.SegmentRowHeight;

    char Counted[16] = {};

    std::snprintf(Counted, sizeof Counted, "%u", Standing.OccurrenceCount);

    PresentSurfaceFill(Badge, Attenuate(Hue, 0.18), Extents.PillRounding);
    PresentTextRun(Badge, Counted, Hue, TextPlacement::Centred, 0.85f);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      ONE MEASURE
//------------------------------------------------------------------------------------------------------------------------

void PresentMeasure(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, const SampledMeasure& Standing)
{
    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    WorkspaceRectangle Row = Area;
    Row.PositionX += Extents.PanelPadding;
    Row.Width     -= Extents.PanelPadding * 2.0f;

    char Named[160] = {};

    std::snprintf(Named, sizeof Named, "%s  ·  %s", Standing.Origin, Standing.Measured);

    PresentTextRun(Row, Named, Palette.TextMuted, TextPlacement::Leading, 0.9f);

    char Read[48] = {};

    // 📝 🔴 `86` §9: the reading is printed in the form its producer declared and is never converted. A count
    //    printed as a real would tell the artist a residency total of eight tiles is `8.000`, which reads as a
    //    measured quantity that happens to be round rather than as the integer it is.
    if (Standing.RealDeclared)
        std::snprintf(Read, sizeof Read, "%.4g", Standing.Magnitude);
    else
        std::snprintf(Read, sizeof Read, "%llu", static_cast<unsigned long long>(Standing.Counted));

    PresentTextRun(Row, Read, Palette.ValueText, TextPlacement::Trailing, 0.95f);
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

void PresentDiagnosticPanel(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, void* PresentContext)
{
    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    PresentSurfaceFill(Area, Palette.PanelBackground, Extents.CornerRounding);

    DiagnosticPanelContext* Presenting = static_cast<DiagnosticPanelContext*>(PresentContext);

    if (Presenting == nullptr || Presenting->Carry == nullptr)
    {
        PresentTextRun(Area, "no register", Palette.TextMuted, TextPlacement::Centred, 1.0f);

        return;
    }

    DiagnosticPanelCarry& Carry = *Presenting->Carry;

    // 📝 🔴 Taken by value, once, at the top of the tick. `86` §3.1 admits an append from any thread, so a
    //    reference walked across a presentation is storage a worker may be writing halfway through the walk.
    const std::vector<ReportSpecification> Retained =
        Presenting->Reports != nullptr ? Presenting->Reports->Retained() : std::vector<ReportSpecification>{};

    // -- the header band -------------------------------------------------------------------------------------------
    WorkspaceRectangle Band = Area;
    Band.Height = Extents.PanelHeaderHeight;

    PresentSurfaceFill(Band, Palette.PanelHeader, 0.0f);

    WorkspaceRectangle Caption = Band;
    Caption.PositionX += Extents.PanelPadding;
    Caption.Width     -= Extents.PanelPadding * 2.0f;

    PresentTextRun(Caption, "Diagnostics", Palette.TextPrimary, TextPlacement::Leading, 1.0f);

    std::uint32_t Problems = 0u;

    for (const ReportSpecification& Standing : Retained)
    {
        if (WeightOf(Standing.Disposition) == ReportWeight::Problem)
            ++Problems;
    }

    char Summarised[64] = {};

    std::snprintf(Summarised, sizeof Summarised, "%u problem%s", Problems, Problems == 1u ? "" : "s");

    PresentTextRun(Caption, Summarised, Problems > 0u ? Palette.DangerPrimary : Palette.TextMuted,
                   TextPlacement::Trailing, 0.95f);

    WorkspaceRectangle Rule = Band;
    Rule.PositionY = Band.PositionY + Band.Height - Extents.BorderThickness;
    Rule.Height    = Extents.BorderThickness;

    PresentSurfaceFill(Rule, Palette.PanelBorder, 0.0f);

    // -- the body --------------------------------------------------------------------------------------------------
    WorkspaceRectangle Body;
    Body.PositionX = Area.PositionX;
    Body.PositionY = Area.PositionY + Extents.PanelHeaderHeight;
    Body.Width     = Area.Width;
    Body.Height    = Area.Height - Extents.PanelHeaderHeight - Extents.PanelFooterHeight;

    if (Body.Height <= 0.0f)
        return;

    DeclareClip(Body);

    // 📝 The content is measured before it is presented, for the same reason the revision panel measures its own:
    //    the visible offset is bounded against the content, and a bound applied afterwards lags by one tick.
    float ContentExtent = 0.0f;

    for (std::uint32_t Rank = 0u; Rank < 7u; ++Rank)
    {
        for (std::uint32_t Ordinal = 0u; Ordinal < static_cast<std::uint32_t>(ReportDisposition::DispositionCount);
             ++Ordinal)
        {
            const ReportDisposition Declared = static_cast<ReportDisposition>(Ordinal);

            if (PresentedRankOf(Declared) != Rank)
                continue;

            std::uint32_t Counted = 0u;

            for (const ReportSpecification& Standing : Retained)
                Counted += Standing.Disposition == Declared ? 1u : 0u;

            if (Counted == 0u)
                continue;

            if (WeightOf(Declared) == ReportWeight::Information && !Carry.InformationDeclared)
                continue;

            ContentExtent += Extents.SectionHeaderHeight;

            if (Carry.ClassOpen[Ordinal])
                ContentExtent += static_cast<float>(Counted) * (EntryHeight + Extents.CardGap);
        }
    }

    const std::vector<SampledMeasure>& Sampled =
        Presenting->Measures != nullptr ? Presenting->Measures->Measures() : std::vector<SampledMeasure>{};

    ContentExtent += Extents.SectionHeaderHeight;

    if (Carry.MeasuresOpen)
        ContentExtent += static_cast<float>(Sampled.size()) * MeasureHeight;

    AdvanceVisibleOffset(Carry.VisibleOffset, Body, ContentExtent + Extents.PanelPadding * 2.0f);

    float Travelled = Body.PositionY + Extents.PanelPadding - Carry.VisibleOffset;

    // 📝 🔴 Presented in rank order and never in arrival order. `86` §3 states that no engine behaviour depends
    //    on report order, which is what makes the any-thread append safe — and it is also what leaves this panel
    //    free to order by attention instead of by arrival.
    for (std::uint32_t Rank = 0u; Rank < 7u; ++Rank)
    {
        for (std::uint32_t Ordinal = 0u; Ordinal < static_cast<std::uint32_t>(ReportDisposition::DispositionCount);
             ++Ordinal)
        {
            const ReportDisposition Declared = static_cast<ReportDisposition>(Ordinal);

            if (PresentedRankOf(Declared) != Rank)
                continue;

            std::uint32_t Counted = 0u;

            for (const ReportSpecification& Standing : Retained)
                Counted += Standing.Disposition == Declared ? 1u : 0u;

            // 📝 An empty class presents no section at all. A run of seven headers reading zero is a panel that
            //    looks busy while saying nothing, which is the same erosion `86` §5 describes by another route.
            if (Counted == 0u)
                continue;

            if (WeightOf(Declared) == ReportWeight::Information && !Carry.InformationDeclared)
                continue;

            WorkspaceRectangle Header;
            Header.PositionX = Body.PositionX + Extents.PanelPadding;
            Header.PositionY = Travelled;
            Header.Width     = Body.Width - Extents.PanelPadding * 2.0f;
            Header.Height    = Extents.SectionHeaderHeight;

            char Trailing[16] = {};

            std::snprintf(Trailing, sizeof Trailing, "%u", Counted);

            PresentSectionHeader(Theme, Header, CaptionOf(Declared), Carry.ClassOpen[Ordinal], Trailing);

            Travelled += Extents.SectionHeaderHeight;

            if (!Carry.ClassOpen[Ordinal])
                continue;

            for (const ReportSpecification& Standing : Retained)
            {
                if (Standing.Disposition != Declared)
                    continue;

                WorkspaceRectangle Entry;
                Entry.PositionX = Body.PositionX + Extents.PanelPadding;
                Entry.PositionY = Travelled;
                Entry.Width     = Body.Width - Extents.PanelPadding * 2.0f;
                Entry.Height    = EntryHeight;

                const bool Presentable = Entry.PositionY + Entry.Height >= Body.PositionY
                                      && Entry.PositionY <= Body.PositionY + Body.Height;

                if (Presentable)
                    PresentEntry(Theme, Entry, Standing);

                Travelled += EntryHeight + Extents.CardGap;
            }
        }
    }

    // -- the measures ----------------------------------------------------------------------------------------------
    {
        WorkspaceRectangle Header;
        Header.PositionX = Body.PositionX + Extents.PanelPadding;
        Header.PositionY = Travelled;
        Header.Width     = Body.Width - Extents.PanelPadding * 2.0f;
        Header.Height    = Extents.SectionHeaderHeight;

        char Trailing[16] = {};

        std::snprintf(Trailing, sizeof Trailing, "%llu", static_cast<unsigned long long>(Sampled.size()));

        // 📝 🔴 A section of their own, beneath every report class. A measure is not a report — `86` §2 — and the
        //    two shown in one list is the arrangement in which the one refusal that mattered is unfindable.
        PresentSectionHeader(Theme, Header, "Sampled", Carry.MeasuresOpen, Trailing);

        Travelled += Extents.SectionHeaderHeight;

        if (Carry.MeasuresOpen)
        {
            for (const SampledMeasure& Standing : Sampled)
            {
                WorkspaceRectangle Row;
                Row.PositionX = Body.PositionX + Extents.PanelPadding;
                Row.PositionY = Travelled;
                Row.Width     = Body.Width - Extents.PanelPadding * 2.0f;
                Row.Height    = MeasureHeight;

                if (Row.PositionY + Row.Height >= Body.PositionY && Row.PositionY <= Body.PositionY + Body.Height)
                    PresentMeasure(Theme, Row, Standing);

                Travelled += MeasureHeight;
            }
        }
    }

    if (Retained.empty() && Sampled.empty())
        PresentTextRun(Body, "nothing reported", Palette.TextMuted, TextPlacement::Centred, 1.0f);

    ReclaimClip();

    // -- the footer ------------------------------------------------------------------------------------------------
    WorkspaceRectangle Footer;
    Footer.PositionX = Area.PositionX + Extents.PanelPadding;
    Footer.PositionY = Area.PositionY + Area.Height - Extents.PanelFooterHeight;
    Footer.Width     = Area.Width - Extents.PanelPadding * 2.0f;
    Footer.Height    = Extents.PanelFooterHeight;

    char Summary[128] = {};

    // 📝 ⚠️ The discard count is presented rather than hidden — `86` §6. A register that silently forgot the
    //    first report of a run is worse than one that admits it is full, and the admission belongs on screen.
    if (Presenting->Reports != nullptr)
    {
        std::snprintf(Summary, sizeof Summary, "%u retained  ·  %llu appended  ·  %llu discarded",
                      Presenting->Reports->RetainedCount(),
                      static_cast<unsigned long long>(Presenting->Reports->AppendedCount()),
                      static_cast<unsigned long long>(Presenting->Reports->DiscardedCount()));
    }

    PresentTextRun(Footer, Summary, Palette.TextMuted, TextPlacement::Leading, 0.9f);

    WorkspaceRectangle Toggle = Footer;
    Toggle.Width     = 110.0f;
    Toggle.PositionX = Footer.PositionX + Footer.Width - Toggle.Width;
    Toggle.Height    = Extents.SegmentRowHeight;
    Toggle.PositionY = Footer.PositionY + (Footer.Height - Toggle.Height) * 0.5f;

    const Outcome<ControlInteraction> Filtered =
        PresentMenuPill(Theme, Toggle, Carry.InformationDeclared ? "Problems only" : "Show all",
                        !Carry.InformationDeclared);

    if (Filtered.ContentPresent && Filtered.Resolve().EditSealed)
        Carry.InformationDeclared = !Carry.InformationDeclared;
}

}   // namespace Slate
