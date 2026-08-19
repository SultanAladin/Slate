//============================================================================================================================================
//                                                       CONTENTBROWSERPANEL.CPP
//============================================================================================================================================
// 🧩 The content browser's recording and its arbitration, transcribed from `AsstbrowsrBasic.html` element by element.

#include "SlateUI/Interface/ContentBrowserPanel/Api/ContentBrowserPanel.h"

#include <cstdio>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SEATED CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

namespace
{

constexpr double RouseOver   = 0.120;   // [s] - the reference's `transition-colors` duration
constexpr float  NotchAcross =  48.0f;  // [px] - one wheel notch
constexpr double OctetsPerMegaOctet = 1048576.0;   // [B] - the reference's own divisor

/// 🔴 The lattice's column count is not a media query here. The reference steps 2→6 columns across five
///    Tailwind breakpoints against the VIEWPORT; a panel inside a page has its own extent, so the count is
///    resolved from the extent actually handed to the lattice rather than from the display.
std::uint32_t ColumnsWithin(float SpanAlong, float CardGap, float CardPad)
{
    constexpr float CardIdeal = 168.0f;   // [px] - what a lattice card wants to occupy

    const float Usable = SpanAlong - CardPad * 2.0f;

    if (Usable <= CardIdeal)
        return 1u;

    const auto Resolved = static_cast<std::uint32_t>((Usable + CardGap) / (CardIdeal + CardGap));

    return (Resolved < 1u) ? 1u : ((Resolved > 6u) ? 6u : Resolved);
}

/// 🔴 A lowercase fold that reaches no locale. The reference's `toLowerCase` is applied to ASCII record
///    namings only, and a locale-aware fold would disagree with it on the very octets it is asked about.
constexpr char Folded(char Arrived)
{
    return (Arrived >= 'A' && Arrived <= 'Z') ? static_cast<char>(Arrived - 'A' + 'a') : Arrived;
}

/// 🧩 Whether Sought appears anywhere within Subject, folded, as `String.includes` decides it.
bool Within(const char* Subject, const char* Sought)
{
    if (Subject == nullptr || Sought == nullptr || Sought[0] == '\0')
        return true;

    for (std::uint32_t Origin = 0u; Subject[Origin] != '\0'; ++Origin)
    {
        std::uint32_t Ordinal = 0u;

        while (Sought[Ordinal] != '\0' &&
               Folded(Subject[Origin + Ordinal]) == Folded(Sought[Ordinal]))
        {
            ++Ordinal;
        }

        if (Sought[Ordinal] == '\0')
            return true;
    }

    return false;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE ARCHIVE ROSTER
//------------------------------------------------------------------------------------------------------------------------

const char* ArchiveNaming(ContentArchive Archive)
{
    switch (Archive)
    {
        case ContentArchive::Topology:    return "Meshes";
        case ContentArchive::Draughting:  return "CAD";
        case ContentArchive::Arrangement: return "Scenes";
        case ContentArchive::Material:    return "Materials";
        case ContentArchive::Typeface:    return "Fonts";
        case ContentArchive::Vector:      return "SVG / Vectors";
        default:                          return "Unclassed";
    }
}

SymbolSubject ArchiveCrest(ContentArchive Archive)
{
    // 📝 The reference names lucide `box`, `box-select`, `layout`, `palette`, `type` and `pen-tool`. Four
    //    of the six are unresolved in the symbol roster, so they crest as the placeholder the roster
    //    declares for exactly this — a dummy crest, which is what was asked for at this stage.
    switch (Archive)
    {
        case ContentArchive::Topology:    return SymbolSubject::CubeSolid;
        case ContentArchive::Draughting:  return SymbolSubject::SketchPlane;
        case ContentArchive::Arrangement: return SymbolSubject::PanelSplit;
        case ContentArchive::Material:    return SymbolSubject::MaterialSphere;
        case ContentArchive::Typeface:    return SymbolSubject::CodeBrackets;
        case ContentArchive::Vector:      return SymbolSubject::UnwrapSeam;
        default:                          return SymbolSubject::PlaceholderMark;
    }
}

void SeatReferenceContent(ContentLibrary& Seating)
{
    Seating = ContentLibrary{};

    // 📐 `ASSETS` in its declared order. The reference states each size as a megaoctet count times
    //    1048576, so the product is stated here rather than the rounded figure the inspector prints.
    const auto Seat = [&](const char* Naming, const char* Extension, double MegaOctets,
                          const char* FirstTag, const char* SecondTag,
                          ContentArchive Archive, const char* Subheading)
    {
        if (Seating.RecordCount >= ContentLibrary::RecordCeiling)
            return;

        ContentRecord& Written = Seating.Records[Seating.RecordCount];

        Written.Naming     = Naming;
        Written.Extension  = Extension;
        Written.Subheading = Subheading;
        Written.Octets     = MegaOctets * OctetsPerMegaOctet;
        Written.Archive    = Archive;
        Written.Tags[0]    = FirstTag;
        Written.TagCount   = 1u;

        if (SecondTag != nullptr)
        {
            Written.Tags[1] = SecondTag;
            Written.TagCount = 2u;
        }

        ++Seating.RecordCount;
    };

    Seat("Turbine_Housing_A",  "step",  48.2, "mech",    nullptr, ContentArchive::Draughting,  nullptr);
    Seat("Hangar_Interior",    "fbx",  214.9, "env",     "hero",  ContentArchive::Arrangement, nullptr);
    Seat("Character_Base_Mesh","obj",   18.6, "char",    nullptr, ContentArchive::Topology,    nullptr);
    Seat("Polished_Copper",    "mat",    1.2, "metal",   nullptr, ContentArchive::Material,    "Metals");
    Seat("Brushed_Aluminium",  "mat",    2.1, "metal",   nullptr, ContentArchive::Material,    "Metals");
    Seat("Matte_Red_Plastic",  "mat",    0.5, "plastic", nullptr, ContentArchive::Material,    "Plastics");
    Seat("Neue_Haas_Grotesk",  "otf",    1.9, "ui",      nullptr, ContentArchive::Typeface,    nullptr);
    Seat("Company_Logo",       "svg",   0.05, "vector",  nullptr, ContentArchive::Vector,      nullptr);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      BRING-UP AND SAMPLING
//------------------------------------------------------------------------------------------------------------------------

Deliver<bool> ContentBrowserPanel::Construct(InteractionIndex& Interaction, RecordingSurface& Recording)
{
    if (Ledger != nullptr)
    {
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported,
                                       "the content browser panel is already constructed" });
    }

    Ledger  = &Interaction;
    Surface = &Recording;

    // 🔴 Every identity claimed here and none inside a tick. A refusal partway through retires the whole
    //    construction rather than leaving half a panel enrolled against a ledger it cannot fill.
    const auto Claim = [&](ControlIdentity* Written, std::uint32_t Count) -> Deliver<bool>
    {
        for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
        {
            const Deliver<ControlIdentity> Issued = Interaction.Enrol();

            if (!Issued.ContentPresent)
            {
                Reset();
                return Deliver<bool>::Refuse(Issued.Declined);
            }

            Written[Ordinal] = Issued.Resolve();
        }

        return Deliver<bool>::Deliver(true);
    };

    if (const auto Verdict = Claim(SourceRows, SourceCeiling); !Verdict.ContentPresent)
        return Verdict;

    if (const auto Verdict = Claim(LatticeCards, LatticeCeiling); !Verdict.ContentPresent)
        return Verdict;

    if (const auto Verdict = Claim(ChromeCells, ChromeCeiling); !Verdict.ContentPresent)
        return Verdict;

    return Deliver<bool>::Deliver(true);
}

void ContentBrowserPanel::Advance(const PointerCondition& Arrived, double Elapsed)
{
    static_cast<void>(Elapsed);
    Sampled = Arrived;
}

void ContentBrowserPanel::Reset()
{
    Ledger  = nullptr;
    Surface = nullptr;

    for (auto& Written : SourceRows)   Written = ControlIdentity{};
    for (auto& Written : LatticeCards) Written = ControlIdentity{};
    for (auto& Written : ChromeCells)  Written = ControlIdentity{};
}

bool ContentBrowserPanel::Roused(const PlaneExtent& Extent) const
{
    if (Surface == nullptr || Surface->Excluded(Extent))
        return false;

    return Extent.Encloses(Sampled.PositionAlong, Sampled.PositionAcross);
}

bool ContentBrowserPanel::Pressed(ControlIdentity Claimed, const PlaneExtent& Extent,
                                  ContentBrowserOrdinates& Seated, const char* Tooltip)
{
    if (Ledger == nullptr)
        return false;

    const bool Over = Roused(Extent);

    if (Over && Tooltip != nullptr)
    {
        Seated.Tooltip       = Tooltip;
        Seated.TooltipAlong  = (Extent.LeastAlong + Extent.MostAlong) * 0.5f;
        Seated.TooltipAcross = Extent.LeastAcross;
    }

    if (Over && Sampled.ContactArrived && !Ledger->AnyDisclosed())
        Ledger->Seize(Claimed, ControlPart::Body);

    Ledger->DeclareRoused(Claimed, Over, RouseOver);

    return Over && Ledger->Released(Claimed);
}

bool ContentBrowserPanel::AdmitTyped(char Arrived, ContentBrowserOrdinates& Seated)
{
    if (!Seated.SeekHolding || Arrived < 0x20)
        return false;

    std::uint32_t Occupied = 0u;

    while (Occupied + 1u < ContentBrowserOrdinates::SeekCeiling && Seated.Seek[Occupied] != '\0')
        ++Occupied;

    if (Occupied + 1u >= ContentBrowserOrdinates::SeekCeiling)
        return false;

    Seated.Seek[Occupied]      = Arrived;
    Seated.Seek[Occupied + 1u] = '\0';

    return true;
}

bool ContentBrowserPanel::RetractTyped(ContentBrowserOrdinates& Seated)
{
    if (!Seated.SeekHolding)
        return false;

    std::uint32_t Occupied = 0u;

    while (Occupied + 1u < ContentBrowserOrdinates::SeekCeiling && Seated.Seek[Occupied] != '\0')
        ++Occupied;

    if (Occupied == 0u)
        return false;

    Seated.Seek[Occupied - 1u] = '\0';
    return true;
}

bool ContentBrowserPanel::Retained(const ContentRecord& Record, const ContentLibrary& Library,
                                   const ContentBrowserOrdinates& Seated) const
{
    // 📐 `renderGrid` narrows by archive, then by subheading, then by the seek run — in that order, and
    //    each against the run the previous one left rather than against the whole library.
    if (Library.TraversedArchive != ContentLibrary::AbsentOrdinal &&
        static_cast<std::uint32_t>(Record.Archive) != Library.TraversedArchive)
    {
        return false;
    }

    if (Library.TraversedSubheading != nullptr &&
        !(Record.Subheading != nullptr && Within(Record.Subheading, Library.TraversedSubheading) &&
          Within(Library.TraversedSubheading, Record.Subheading)))
    {
        return false;
    }

    if (Seated.Seek[0] == '\0')
        return true;

    // 📐 The reference seeks the naming, the extension and the archive spelling joined by spaces, so a
    //    run spanning the join matches there and must match here.
    return Within(Record.Naming, Seated.Seek)
        || Within(Record.Extension, Seated.Seek)
        || Within(ArchiveNaming(Record.Archive), Seated.Seek);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      SHARED FRAGMENTS
//------------------------------------------------------------------------------------------------------------------------

void ContentBrowserPanel::RecordHatch(const PlaneExtent& Extent)
{
    // 📐 `repeating-linear-gradient(45deg, rgba(255,255,255,.02) 0 1px, transparent 1px 7px)` — a 1px rule
    //    every 7px along the 45° diagonal. Each rule is one four-corner tongue confined to the plate, not a
    //    shader: the whole figure is two coverage steps, and a shader would be a second pipeline for it.
    // 🔴 One tongue per RULE and not one ground per pixel. Stepping the diagonal a pixel at a time recorded
    //    an upright 1×1 quad for every covered pixel, which on a lattice of plates put a quarter of a million
    //    vertices into a single command list and tripped ImGui's 16-bit index ceiling outright. A rule is
    //    convex, so it costs four corners however long it is.
    constexpr float Period = 7.0f;

    Surface->Confine(Extent);

    // 📐 The rule runs at 45°, so a plate SpanAcross tall shifts it a whole SpanAcross along, end to end.
    //    Origins therefore run from one full drop BEFORE the leading edge to one full drop PAST the
    //    trailing one — a rule seated at the trailing edge up top has walked a drop to the left by the
    //    time it reaches the bottom, so the origins beyond the edge are what covers the lower trailing
    //    corner. Ending at the trailing edge left that corner bare in a widening wedge.
    const float Drop  = Extent.SpanAcross();
    const float First = -Drop;
    const float Last  = Extent.SpanAlong() + Drop + Period;

    for (float Origin = First; Origin < Last; Origin += Period)
    {
        const float Upper = Extent.LeastAlong + Origin;
        const float Lower = Upper - Drop;

        const float Corners[8] =
        {
            Upper,        Extent.LeastAcross,
            Upper + 1.0f, Extent.LeastAcross,
            Lower + 1.0f, Extent.MostAcross,
            Lower,        Extent.MostAcross
        };

        Surface->Tongue(Corners, 4u, Ink.Hatch);
    }

    Surface->Release();
}

void ContentBrowserPanel::RecordScrollbar(const PlaneExtent& Extent, ControlIdentity Claimed,
                                          float Span, float& Offset)
{
    // 📐 `::-webkit-scrollbar` — a 6px trough with a #333 thumb at radius 4, presented only when the run
    //    is longer than the extent that holds it.
    const float Visible = Extent.SpanAcross();

    if (Span <= Visible || Visible <= 0.0f)
    {
        Offset = 0.0f;
        return;
    }

    const float Ceiling     = Span - Visible;
    const float ThumbAcross = (Visible * Visible / Span < 28.0f) ? 28.0f : (Visible * Visible / Span);
    const float Travel      = Visible - ThumbAcross;

    const bool Holding = Ledger->Holding(Claimed);

    // 📐 The wheel reaches the run whenever the pointer is over the extent, whether or not the bar itself
    //    is roused — the reference scrolls the container and not its scrollbar.
    if (Roused(Extent) && !Ledger->AnyDisclosed() && Sampled.WheelAcross != 0.0f)
        Offset -= Sampled.WheelAcross * NotchAcross;

    const PlaneExtent Trough = Spanning(Extent.MostAlong - 6.0f, Extent.LeastAcross, 6.0f, Visible);

    if (Roused(Trough) && Sampled.ContactArrived && !Ledger->AnyDisclosed())
    {
        Ledger->Seize(Claimed, ControlPart::Thumb);
        Ledger->DepartFrom(Claimed, Offset);
    }

    if (Holding && Travel > 0.0f)
    {
        const Deliver<float> Departed = Ledger->DepartedOrdinate(Claimed);

        if (Departed.ContentPresent)
        {
            const float Moved = Sampled.PositionAcross - Ledger->OriginAcross();
            Offset = Departed.Resolve() + Moved * (Ceiling / Travel);
        }
    }

    // 🔴 Clamped last and always. Every reach above may carry the offset past either end, and a run
    //    recorded from a past-the-end offset presents an empty extent that reads as a panel that failed.
    if (Offset < 0.0f)       Offset = 0.0f;
    if (Offset > Ceiling)    Offset = Ceiling;

    const float ThumbSeat = Extent.LeastAcross + (Ceiling > 0.0f ? (Offset / Ceiling) * Travel : 0.0f);

    Surface->Ground(Spanning(Extent.MostAlong - 6.0f + 3.0f, ThumbSeat, 3.0f, ThumbAcross),
                    Partial(0xFFFFFFu, Holding ? 0.30 : 0.15), 2.0f);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE SOURCES ASIDE
//------------------------------------------------------------------------------------------------------------------------

void ContentBrowserPanel::RecordSources(const PlaneExtent& Extent, ContentLibrary& Library,
                                        ContentBrowserOrdinates& Seated)
{
    Surface->Ground(Extent, Ink.Aside);
    Surface->Ground(Spanning(Extent.MostAlong - 1.0f, Extent.LeastAcross, 1.0f, Extent.SpanAcross()),
                    Ink.Stroke);

    // 📐 `p-4 pb-2` over a `text-[10px] tracking-widest uppercase` caption.
    Surface->TextRunCapitalised(Extent.LeastAlong + 16.0f, Extent.LeastAcross + 16.0f,
                                Ink.Faint, "Sources", Measure.RunCaption, 1.6f, true);

    // 📐 The library cache foot is pinned to the lower edge, so the traversable run is what remains.
    constexpr float FootAcross = 69.0f;   // [px] - border-t p-4 over two runs and a 4px meter

    const PlaneExtent Traversable = Spanning(Extent.LeastAlong, Extent.LeastAcross + 40.0f,
                                             Extent.SpanAlong(),
                                             Extent.SpanAcross() - 40.0f - FootAcross);

    Surface->Confine(Traversable);

    float Cursor = Traversable.LeastAcross - Seated.AsideOffset + 8.0f;
    std::uint32_t Claimed = 0u;

    const auto SourceRow = [&](const char* Naming, std::uint32_t Count, float Step,
                               bool Standing, SymbolSubject Crest, bool Crested)
    {
        if (Claimed >= SourceCeiling)
            return false;

        const PlaneExtent Row = Spanning(Extent.LeastAlong + 8.0f + Step, Cursor,
                                         Extent.SpanAlong() - 16.0f - Step, Measure.SourceRowAcross);

        const bool Over  = Roused(Row);
        const bool Taken = Pressed(SourceRows[Claimed], Row, Seated);

        if (Standing)
            Surface->Ground(Row, Ink.Taken, Measure.RadiusSoft);
        else if (Over)
            Surface->Ground(Row, Ink.Roused, Measure.RadiusSoft);

        const InkOrdinate Run = (Standing || Over) ? Ink.Primary : Ink.Secondary;

        float Along = Row.LeastAlong + 8.0f;

        // 📐 A crested row strokes its chevron or its folder; an uncrested one holds the same 14px of
        //    space empty, exactly as the reference's `<span class="w-3.5 h-3.5">` does.
        if (Crested)
        {
            Surface->Stroke(Crest, Spanning(Along, Row.LeastAcross + 8.0f, 14.0f, 14.0f), Run);
        }

        Along += 22.0f;

        char Tally[16] = {};
        std::snprintf(Tally, sizeof(Tally), "%u", Count);

        const float TallyAlong = Surface->MeasureRun(Tally, Measure.RunCaption);

        Surface->TextRunTruncated(Along, Row.LeastAcross + 9.0f,
                                  Row.MostAlong - Along - TallyAlong - 12.0f, Run,
                                  Naming, Measure.RunBody);

        Surface->TextRun(Row.MostAlong - TallyAlong - 8.0f, Row.LeastAcross + 10.0f,
                         Run, Tally, Measure.RunCaption);

        Cursor += Measure.SourceRowAcross + 2.0f;
        ++Claimed;

        return Taken;
    };

    // 📐 `Project` — the section caption above the library row.
    Surface->TextRunCapitalised(Extent.LeastAlong + 16.0f, Cursor + 4.0f,
                                Ink.Faint, "Project", Measure.RunCaption, 1.6f, true);
    Cursor += Measure.CaptionAcross;

    // 📐 `Project Library` — standing whenever no archive is traversed, as `!state.cat` decides it.
    if (SourceRow("Project Library", Library.RecordCount, 0.0f,
                  Library.TraversedArchive == ContentLibrary::AbsentOrdinal,
                  SymbolSubject::FolderClosed, true))
    {
        Library.TraversedArchive    = ContentLibrary::AbsentOrdinal;
        Library.TraversedSubheading = nullptr;
        Library.Taken               = ContentLibrary::AbsentOrdinal;
    }

    // 📐 The archives, in the order the library first presents them, exactly as `Array.from(new Set(...))`
    //    yields them rather than in the enum's own order.
    constexpr std::uint32_t ArchiveCeiling = static_cast<std::uint32_t>(ContentArchive::ArchiveCount);

    bool Presented[ArchiveCeiling] = {};

    for (std::uint32_t Ordinal = 0u; Ordinal < Library.RecordCount; ++Ordinal)
    {
        const auto Archive = static_cast<std::uint32_t>(Library.Records[Ordinal].Archive);

        if (Archive >= static_cast<std::uint32_t>(ContentArchive::ArchiveCount) || Presented[Archive])
            continue;

        Presented[Archive] = true;

        std::uint32_t Beneath = 0u;

        for (std::uint32_t Scan = 0u; Scan < Library.RecordCount; ++Scan)
        {
            if (static_cast<std::uint32_t>(Library.Records[Scan].Archive) == Archive)
                ++Beneath;
        }

        // 📐 A chevron only where subheadings stand beneath, which is what `subcats.length > 0` states.
        bool Subheaded = false;

        for (std::uint32_t Scan = 0u; Scan < Library.RecordCount; ++Scan)
        {
            if (static_cast<std::uint32_t>(Library.Records[Scan].Archive) == Archive &&
                Library.Records[Scan].Subheading != nullptr)
            {
                Subheaded = true;
            }
        }

        const bool Standing = Library.TraversedArchive == Archive &&
                              Library.TraversedSubheading == nullptr;

        if (SourceRow(ArchiveNaming(static_cast<ContentArchive>(Archive)), Beneath,
                      Measure.SourceStepAlong, Standing, SymbolSubject::ChevronDown, Subheaded))
        {
            Library.TraversedArchive    = Archive;
            Library.TraversedSubheading = nullptr;
            Library.Taken               = ContentLibrary::AbsentOrdinal;
        }

        if (!Subheaded)
            continue;

        // 📐 The subheadings beneath, each in first-presented order and stepped one further nesting in.
        for (std::uint32_t Scan = 0u; Scan < Library.RecordCount; ++Scan)
        {
            const ContentRecord& Record = Library.Records[Scan];

            if (static_cast<std::uint32_t>(Record.Archive) != Archive || Record.Subheading == nullptr)
                continue;

            bool Repeated = false;

            for (std::uint32_t Prior = 0u; Prior < Scan; ++Prior)
            {
                if (static_cast<std::uint32_t>(Library.Records[Prior].Archive) == Archive &&
                    Library.Records[Prior].Subheading != nullptr &&
                    Within(Library.Records[Prior].Subheading, Record.Subheading) &&
                    Within(Record.Subheading, Library.Records[Prior].Subheading))
                {
                    Repeated = true;
                }
            }

            if (Repeated)
                continue;

            std::uint32_t Tallied = 0u;

            for (std::uint32_t Count = 0u; Count < Library.RecordCount; ++Count)
            {
                const ContentRecord& Weighed = Library.Records[Count];

                if (static_cast<std::uint32_t>(Weighed.Archive) == Archive &&
                    Weighed.Subheading != nullptr &&
                    Within(Weighed.Subheading, Record.Subheading) &&
                    Within(Record.Subheading, Weighed.Subheading))
                {
                    ++Tallied;
                }
            }

            const bool SubStanding = Library.TraversedArchive == Archive &&
                                     Library.TraversedSubheading != nullptr &&
                                     Within(Library.TraversedSubheading, Record.Subheading) &&
                                     Within(Record.Subheading, Library.TraversedSubheading);

            if (SourceRow(Record.Subheading, Tallied, Measure.SourceStepAlong * 2.0f,
                          SubStanding, SymbolSubject::PlaceholderMark, false))
            {
                Library.TraversedArchive    = Archive;
                Library.TraversedSubheading = Record.Subheading;
                Library.Taken               = ContentLibrary::AbsentOrdinal;
            }
        }
    }

    Seated.AsideSpan = (Cursor + Seated.AsideOffset) - Traversable.LeastAcross;

    Surface->Release();

    RecordScrollbar(Traversable, ChromeCells[8], Seated.AsideSpan, Seated.AsideOffset);

    // 📐 The library cache foot — a caption pair over a three-part meter at 38 / 24 / 12 percent.
    const PlaneExtent Foot = Spanning(Extent.LeastAlong, Extent.MostAcross - FootAcross,
                                      Extent.SpanAlong(), FootAcross);

    Surface->Ground(Spanning(Foot.LeastAlong, Foot.LeastAcross, Foot.SpanAlong(), 1.0f), Ink.Stroke);

    Surface->TextRun(Foot.LeastAlong + 16.0f, Foot.LeastAcross + 16.0f,
                     Ink.Faint, "Library cache", Measure.RunCaption);

    const float RetainedAlong = Surface->MeasureRun("12.4 GB", Measure.RunCaption);

    Surface->TextRun(Foot.MostAlong - 16.0f - RetainedAlong, Foot.LeastAcross + 16.0f,
                     Ink.Primary, "12.4 GB", Measure.RunCaption);

    const PlaneExtent Meter = Spanning(Foot.LeastAlong + 16.0f, Foot.LeastAcross + 38.0f,
                                       Foot.SpanAlong() - 32.0f, 4.0f);

    Surface->Ground(Meter, Partial(0xFFFFFFu, 0.10), 2.0f);

    const float MeterSpan = Meter.SpanAlong();

    Surface->Ground(Spanning(Meter.LeastAlong, Meter.LeastAcross, MeterSpan * 0.38f, 4.0f),
                    Covering(0xFFFFFFu), 2.0f);
    Surface->Ground(Spanning(Meter.LeastAlong + MeterSpan * 0.38f, Meter.LeastAcross,
                             MeterSpan * 0.24f, 4.0f), Covering(0x737373u));
    Surface->Ground(Spanning(Meter.LeastAlong + MeterSpan * 0.62f, Meter.LeastAcross,
                             MeterSpan * 0.12f, 4.0f), Partial(0xFFFFFFu, 0.20));
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE SEEK RAIL
//------------------------------------------------------------------------------------------------------------------------

void ContentBrowserPanel::RecordSeekRail(const PlaneExtent& Extent, ContentBrowserOrdinates& Seated)
{
    Surface->Ground(Extent, Ink.Aside);
    Surface->Ground(Spanning(Extent.LeastAlong, Extent.MostAcross - 1.0f, Extent.SpanAlong(), 1.0f),
                    Ink.Stroke);

    // 📐 `flex-1 max-w-md h-8 ... rounded-full` — the seek field, pinned to the leading edge.
    const float FieldAlong = (Extent.SpanAlong() - 32.0f - 200.0f < Measure.SeekAlong)
                           ? (Extent.SpanAlong() - 32.0f - 200.0f) : Measure.SeekAlong;

    const PlaneExtent SeekField = Spanning(Extent.LeastAlong + 16.0f,
                                           Extent.LeastAcross + 12.0f,
                                           (FieldAlong < 120.0f) ? 120.0f : FieldAlong,
                                           Measure.SeekAcross);

    const bool SeekPressed = Pressed(ChromeCells[0], SeekField, Seated);

    // 📐 `focus-within:border-white/40` — the field holds the keyboard until a contact lands off it.
    if (SeekPressed)
        Seated.SeekHolding = true;
    else if (Sampled.ContactArrived && !Roused(SeekField))
        Seated.SeekHolding = false;

    Surface->Ground(SeekField, Ink.Field, Measure.SeekAcross * 0.5f);
    Surface->Edge(SeekField, Seated.SeekHolding ? Partial(0xFFFFFFu, 0.40) : Ink.Stroke,
                  1.0f, Measure.SeekAcross * 0.5f);

    Surface->Stroke(SymbolSubject::MagnifierLens,
                    Spanning(SeekField.LeastAlong + 12.0f, SeekField.LeastAcross + 9.0f, 14.0f, 14.0f),
                    Ink.Faint);

    const bool Sought = Seated.Seek[0] != '\0';

    Surface->TextRunTruncated(SeekField.LeastAlong + 34.0f, SeekField.LeastAcross + 10.0f,
                              SeekField.SpanAlong() - 70.0f,
                              Sought ? Ink.Primary : Ink.Faintest,
                              Sought ? Seated.Seek : "Search assets, formats, tags...",
                              Measure.RunBody);

    // 📐 The caret sits at the run's own trailing edge while the field holds the keyboard.
    if (Seated.SeekHolding)
    {
        const float Caret = SeekField.LeastAlong + 34.0f +
                            Surface->MeasureRun(Seated.Seek, Measure.RunBody);

        Surface->Ground(Spanning(Caret, SeekField.LeastAcross + 8.0f, 1.0f, 16.0f), Ink.Primary);
    }

    // 📐 The `/` chip — the reference's own keyboard hint, at the field's trailing edge.
    const PlaneExtent Hint = Spanning(SeekField.MostAlong - 26.0f, SeekField.LeastAcross + 8.0f,
                                      18.0f, 16.0f);

    Surface->Ground(Hint, Partial(0xFFFFFFu, 0.05), 4.0f);
    Surface->Edge(Hint, Ink.Stroke, 1.0f, 4.0f);
    Surface->TextRun(Hint.LeastAlong + 6.0f, Hint.LeastAcross + 2.0f, Ink.Faint, "/", Measure.RunCaption);

    // 📐 `Create` and `Import`, pinned to the trailing edge with `ml-auto`. Import is the filled action.
    const float ImportAlong = 84.0f;
    const float CreateAlong = 82.0f;

    const PlaneExtent Import = Spanning(Extent.MostAlong - 16.0f - ImportAlong,
                                        Extent.LeastAcross + 12.0f, ImportAlong, Measure.SeekAcross);

    const PlaneExtent Create = Spanning(Import.LeastAlong - 8.0f - CreateAlong,
                                        Extent.LeastAcross + 12.0f, CreateAlong, Measure.SeekAcross);

    static_cast<void>(Pressed(ChromeCells[1], Create, Seated, "Create a new record"));
    static_cast<void>(Pressed(ChromeCells[2], Import, Seated, "Import into the library"));

    const bool CreateOver = Roused(Create);

    Surface->Ground(Create, CreateOver ? Partial(0xFFFFFFu, 0.10) : Partial(0xFFFFFFu, 0.05),
                    Measure.SeekAcross * 0.5f);
    Surface->Edge(Create, Ink.Stroke, 1.0f, Measure.SeekAcross * 0.5f);
    Surface->Stroke(SymbolSubject::PlusCross,
                    Spanning(Create.LeastAlong + 12.0f, Create.LeastAcross + 9.0f, 14.0f, 14.0f),
                    CreateOver ? Ink.Primary : Ink.Secondary);
    Surface->TextRun(Create.LeastAlong + 32.0f, Create.LeastAcross + 10.0f,
                     CreateOver ? Ink.Primary : Ink.Secondary, "Create", Measure.RunBody);

    const bool ImportOver = Roused(Import);

    Surface->Ground(Import, ImportOver ? Covering(0xE5E5E5u) : Covering(0xFFFFFFu),
                    Measure.SeekAcross * 0.5f);
    Surface->Stroke(SymbolSubject::PersistDisc,
                    Spanning(Import.LeastAlong + 12.0f, Import.LeastAcross + 9.0f, 14.0f, 14.0f),
                    Covering(0x000000u));
    Surface->TextRun(Import.LeastAlong + 32.0f, Import.LeastAcross + 10.0f,
                     Covering(0x000000u), "Import", Measure.RunBody, 0.0f, true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE LATTICE
//------------------------------------------------------------------------------------------------------------------------

void ContentBrowserPanel::RecordLattice(const PlaneExtent& Extent, ContentLibrary& Library,
                                        ContentBrowserOrdinates& Seated)
{
    Surface->Ground(Extent, Ink.Ground);

    // 📐 The breadcrumb rail — `sticky top-0`, so it is recorded before the run and never scrolls with it.
    const PlaneExtent Rail = Spanning(Extent.LeastAlong, Extent.LeastAcross,
                                      Extent.SpanAlong(), Measure.BreadcrumbAcross);

    Surface->Ground(Rail, Ink.Rail);
    Surface->Ground(Spanning(Rail.LeastAlong, Rail.MostAcross - 1.0f, Rail.SpanAlong(), 1.0f), Ink.Stroke);

    float Crumb = Rail.LeastAlong + 16.0f;

    Surface->TextRun(Crumb, Rail.LeastAcross + 14.0f, Ink.Faint, "Harbor", Measure.RunBody);
    Crumb += Surface->MeasureRun("Harbor", Measure.RunBody) + 8.0f;
    Surface->TextRun(Crumb, Rail.LeastAcross + 14.0f, Ink.Faint, "/", Measure.RunBody);
    Crumb += Surface->MeasureRun("/", Measure.RunBody) + 8.0f;

    if (Library.TraversedArchive == ContentLibrary::AbsentOrdinal)
    {
        Surface->TextRun(Crumb, Rail.LeastAcross + 14.0f, Ink.Primary, "Project Library",
                         Measure.RunBody, 0.0f, true);
    }
    else
    {
        const char* Naming = ArchiveNaming(static_cast<ContentArchive>(Library.TraversedArchive));

        if (Library.TraversedSubheading == nullptr)
        {
            Surface->TextRun(Crumb, Rail.LeastAcross + 14.0f, Ink.Primary, Naming,
                             Measure.RunBody, 0.0f, true);
        }
        else
        {
            Surface->TextRun(Crumb, Rail.LeastAcross + 14.0f, Ink.Faint, Naming, Measure.RunBody);
            Crumb += Surface->MeasureRun(Naming, Measure.RunBody) + 8.0f;
            Surface->TextRun(Crumb, Rail.LeastAcross + 14.0f, Ink.Faint, "/", Measure.RunBody);
            Crumb += Surface->MeasureRun("/", Measure.RunBody) + 8.0f;
            Surface->TextRun(Crumb, Rail.LeastAcross + 14.0f, Ink.Primary,
                             Library.TraversedSubheading, Measure.RunBody, 0.0f, true);
        }
    }

    // 📐 The run itself, beneath the rail and clipped to what remains.
    const PlaneExtent Run = Spanning(Extent.LeastAlong, Rail.MostAcross,
                                     Extent.SpanAlong(), Extent.SpanAcross() - Measure.BreadcrumbAcross);

    Surface->Confine(Run);

    const std::uint32_t Columns = ColumnsWithin(Run.SpanAlong(), Measure.CardGap, Measure.CardPad);

    const float CardAlong = (Run.SpanAlong() - Measure.CardPad * 2.0f -
                             Measure.CardGap * static_cast<float>(Columns - 1u)) /
                            static_cast<float>(Columns);

    // 📐 `aspect-4-3` over the plate, and `p-2.5` over two runs beneath it.
    const float PlateAcross = CardAlong * 0.75f;
    const float CardAcross  = PlateAcross + Measure.CardCaptionAcross;

    std::uint32_t Seat    = 0u;
    std::uint32_t Claimed = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < Library.RecordCount && Claimed < LatticeCeiling; ++Ordinal)
    {
        const ContentRecord& Record = Library.Records[Ordinal];

        if (!Retained(Record, Library, Seated))
            continue;

        const std::uint32_t Column = Seat % Columns;
        const std::uint32_t Course = Seat / Columns;

        const float Along  = Run.LeastAlong + Measure.CardPad +
                             static_cast<float>(Column) * (CardAlong + Measure.CardGap);
        const float Across = Run.LeastAcross + Measure.CardPad +
                             static_cast<float>(Course) * (CardAcross + Measure.CardGap) -
                             Seated.LatticeOffset;

        const PlaneExtent Card = Spanning(Along, Across, CardAlong, CardAcross);

        const bool Standing = Library.Taken == Ordinal;
        const bool Over     = Roused(Card);

        if (Pressed(LatticeCards[Claimed], Card, Seated))
            Library.Taken = Ordinal;

        // 📐 `hover:-translate-y-0.5` — the roused card lifts two pixels, which is the whole of the
        //    reference's hover motion apart from its shadow.
        const PlaneExtent Lifted = Over
            ? Spanning(Card.LeastAlong, Card.LeastAcross - 2.0f, CardAlong, CardAcross)
            : Card;

        // 📐 `bg-gradient-to-b from-[#131316] to-[#0f0f12] rounded-xl` — Scrim carries no radius of its
        //    own, so the corners are cut back to the page ground after it, which is what MaskCorners is for.
        Surface->Scrim(Lifted, Ink.CardUpper, Ink.CardLower);
        Surface->MaskCorners(Lifted, Ink.Ground, Measure.RadiusCard);

        // 📐 The plate — the hatched square the crest stands on, clipped to the card's upper corners.
        const PlaneExtent Plate = Spanning(Lifted.LeastAlong, Lifted.LeastAcross, CardAlong, PlateAcross);

        Surface->Ground(Plate, Ink.Plate, Measure.RadiusCard,
                        CornerLeadingUpper | CornerTrailingUpper);

        RecordHatch(Plate);

        Surface->Ground(Spanning(Plate.LeastAlong, Plate.MostAcross - 1.0f, CardAlong, 1.0f), Ink.Stroke);

        const float CrestSpan = Over ? 34.0f : 32.0f;

        Surface->Stroke(ArchiveCrest(Record.Archive),
                        Spanning(Plate.LeastAlong + (CardAlong - CrestSpan) * 0.5f,
                                 Plate.LeastAcross + (PlateAcross - CrestSpan) * 0.5f,
                                 CrestSpan, CrestSpan),
                        Over ? Ink.Secondary : Ink.Faintest);

        // 📐 The extension chip — `absolute left-2 bottom-2`, a medallion and the run beside it.
        char Extension[16] = {};
        std::snprintf(Extension, sizeof(Extension), "%s", Record.Extension);

        for (std::uint32_t Step = 0u; Extension[Step] != '\0'; ++Step)
        {
            if (Extension[Step] >= 'a' && Extension[Step] <= 'z')
                Extension[Step] = static_cast<char>(Extension[Step] - 'a' + 'A');
        }

        const float ChipAlong = Surface->MeasureRun(Extension, 9.0f) + 24.0f;

        const PlaneExtent Chip = Spanning(Plate.LeastAlong + 8.0f,
                                          Plate.MostAcross - 8.0f - Measure.ChipAcross,
                                          ChipAlong, Measure.ChipAcross);

        Surface->Ground(Chip, Partial(0x000000u, 0.70), Measure.RadiusSoft);
        Surface->Edge(Chip, Ink.Stroke, 1.0f, Measure.RadiusSoft);
        Surface->Medallion(Chip.LeastAlong + 9.0f, Chip.LeastAcross + Measure.ChipAcross * 0.5f,
                           3.0f, Ink.Secondary);
        Surface->TextRun(Chip.LeastAlong + 16.0f, Chip.LeastAcross + 5.0f,
                         Ink.Secondary, Extension, 9.0f, 0.6f);

        // 📐 The caption pair — `name.ext` truncated, and the size beneath it.
        char Titled[96] = {};
        std::snprintf(Titled, sizeof(Titled), "%s.%s", Record.Naming, Record.Extension);

        Surface->TextRunTruncated(Lifted.LeastAlong + 10.0f, Plate.MostAcross + 8.0f,
                                  CardAlong - 20.0f, Ink.Primary, Titled, Measure.RunBody, true);

        char Sized[32] = {};
        std::snprintf(Sized, sizeof(Sized), "%.1f MB", Record.Octets / OctetsPerMegaOctet);

        Surface->TextRun(Lifted.LeastAlong + 10.0f, Plate.MostAcross + 26.0f,
                         Ink.Faint, Sized, Measure.RunCaption);

        // 📐 `border ... overflow-hidden` — the border box clips its children, so the edge is in FRONT of
        //    everything the card holds. Recorded last for that reason.
        // 🔴 Recorded before the plate it reads over, the taken card's white/60 edge was overpainted along
        //    its whole upper run by the plate's own ground, and a taken card was then indistinguishable
        //    from an untaken one down the three sides the caption did not cover.
        Surface->Edge(Lifted, Standing ? Ink.EdgeTaken : (Over ? Ink.EdgeRoused : Ink.Stroke),
                      1.0f, Measure.RadiusCard);

        ++Seat;
        ++Claimed;
    }

    // 📐 What the run occupied, so the next tick's scroll is held against a measured span.
    const std::uint32_t Courses = (Seat + Columns - 1u) / Columns;

    Seated.LatticeSpan = Measure.CardPad * 2.0f +
                         static_cast<float>(Courses) * (CardAcross + Measure.CardGap);

    // 📐 An empty run states why rather than presenting nothing, which reads as a panel that failed.
    if (Seat == 0u)
    {
        Surface->TextRun(Run.LeastAlong + Measure.CardPad, Run.LeastAcross + Measure.CardPad + 8.0f,
                         Ink.Faint, "No record answers this search.", Measure.RunBody);
    }

    Surface->Release();

    RecordScrollbar(Run, ChromeCells[9], Seated.LatticeSpan, Seated.LatticeOffset);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE INSPECTOR
//------------------------------------------------------------------------------------------------------------------------

void ContentBrowserPanel::RecordInspector(const PlaneExtent& Extent, ContentLibrary& Library,
                                          ContentBrowserOrdinates& Seated)
{
    Surface->Ground(Extent, Ink.Aside);
    Surface->Ground(Spanning(Extent.LeastAlong, Extent.LeastAcross, 1.0f, Extent.SpanAcross()), Ink.Stroke);

    // 📐 The tongue pair — `Details` and `Create`, `p-2` over an `h-8` each.
    const PlaneExtent Tongues = Spanning(Extent.LeastAlong, Extent.LeastAcross,
                                         Extent.SpanAlong(), Measure.TongueAcross);

    Surface->Ground(Tongues, Ink.Rail);
    Surface->Ground(Spanning(Tongues.LeastAlong, Tongues.MostAcross - 1.0f, Tongues.SpanAlong(), 1.0f),
                    Ink.Stroke);

    const float TongueAlong = (Tongues.SpanAlong() - 12.0f) * 0.5f;

    const char*         TongueNaming[2] = { "Details", "Create" };
    const SymbolSubject TongueCrest[2]  = { SymbolSubject::BulbFilament, SymbolSubject::PlusCross };

    for (std::uint32_t Ordinal = 0u; Ordinal < 2u; ++Ordinal)
    {
        const PlaneExtent Tongue = Spanning(Tongues.LeastAlong + 8.0f +
                                            static_cast<float>(Ordinal) * (TongueAlong + 4.0f),
                                            Tongues.LeastAcross + 8.0f, TongueAlong, 32.0f);

        if (Pressed(ChromeCells[3u + Ordinal], Tongue, Seated))
            Seated.InspectorTongue = Ordinal;

        const bool Standing = Seated.InspectorTongue == Ordinal;

        if (Standing)
            Surface->Ground(Tongue, Ink.Taken, Measure.RadiusSoft);

        const InkOrdinate Run = Standing ? Ink.Primary
                                         : (Roused(Tongue) ? Ink.Secondary : Ink.Faint);

        const float Titled = Surface->MeasureRun(TongueNaming[Ordinal], Measure.RunBody);
        const float Origin = Tongue.LeastAlong + (TongueAlong - Titled - 20.0f) * 0.5f;

        Surface->Stroke(TongueCrest[Ordinal],
                        Spanning(Origin, Tongue.LeastAcross + 9.0f, 14.0f, 14.0f), Run);
        Surface->TextRun(Origin + 20.0f, Tongue.LeastAcross + 10.0f, Run,
                         TongueNaming[Ordinal], Measure.RunBody);
    }

    // 📐 The preview — `h-48`, always presented, whether or not a record stands taken.
    const PlaneExtent Preview = Spanning(Extent.LeastAlong, Tongues.MostAcross,
                                         Extent.SpanAlong(), Measure.PreviewAcross);

    Surface->Ground(Preview, Ink.Ground);
    Surface->Ground(Spanning(Preview.LeastAlong, Preview.MostAcross - 1.0f, Preview.SpanAlong(), 1.0f),
                    Ink.Stroke);

    const bool Taken = Library.Taken != ContentLibrary::AbsentOrdinal &&
                       Library.Taken < Library.RecordCount;

    if (!Taken)
    {
        const float Titled = Surface->MeasureRun("No preview available", Measure.RunBody);

        Surface->TextRun(Preview.LeastAlong + (Preview.SpanAlong() - Titled) * 0.5f,
                         Preview.LeastAcross + Measure.PreviewAcross * 0.5f - 8.0f,
                         Ink.Faintest, "No preview available", Measure.RunBody);

        const float Nothing = Surface->MeasureRun("Nothing selected", Measure.RunBody);

        Surface->TextRun(Extent.LeastAlong + (Extent.SpanAlong() - Nothing) * 0.5f,
                         Preview.MostAcross + 32.0f, Ink.Primary, "Nothing selected",
                         Measure.RunBody, 0.0f, true);

        Surface->TextRun(Extent.LeastAlong + 24.0f, Preview.MostAcross + 54.0f, Ink.Faint,
                         "Select an asset to inspect its", Measure.RunBody);
        Surface->TextRun(Extent.LeastAlong + 24.0f, Preview.MostAcross + 72.0f, Ink.Faint,
                         "metadata and import options.", Measure.RunBody);
        return;
    }

    const ContentRecord& Record = Library.Records[Library.Taken];

    // 🔴 The 3D preview is deliberately absent. The reference reaches Three.js for a rotating solid, and
    //    a viewport is a separate mechanism from a panel recording — it was excluded from this pass by
    //    instruction, so the preview states what it would present rather than pretending to present it.
    RecordHatch(Preview);

    Surface->Stroke(ArchiveCrest(Record.Archive),
                    Spanning(Preview.LeastAlong + Preview.SpanAlong() * 0.5f - 28.0f,
                             Preview.LeastAcross + Measure.PreviewAcross * 0.5f - 34.0f, 56.0f, 56.0f),
                    Ink.Faintest);

    Surface->TextRunCapitalised(Preview.LeastAlong + 8.0f, Preview.LeastAcross + 8.0f,
                                Ink.Faint, "2d preview", 9.0f, 1.2f);

    const float Extension = Surface->MeasureRun(Record.Extension, 9.0f);

    Surface->TextRunCapitalised(Preview.MostAlong - 8.0f - Extension, Preview.LeastAcross + 8.0f,
                                Ink.Faint, Record.Extension, 9.0f, 1.2f);

    const float Awaiting = Surface->MeasureRun("dimensional preview withheld", Measure.RunCaption);

    Surface->TextRun(Preview.LeastAlong + (Preview.SpanAlong() - Awaiting) * 0.5f,
                     Preview.MostAcross - 24.0f, Ink.Faintest,
                     "dimensional preview withheld", Measure.RunCaption);

    // 📐 The crest row — a 40px medallion, the naming beside it and its tags beneath.
    const PlaneExtent Crest = Spanning(Extent.LeastAlong, Preview.MostAcross, Extent.SpanAlong(), 68.0f);

    Surface->Ground(Spanning(Crest.LeastAlong, Crest.MostAcross - 1.0f, Crest.SpanAlong(), 1.0f),
                    Ink.Stroke);

    const PlaneExtent Plate = Spanning(Crest.LeastAlong + 12.0f, Crest.LeastAcross + 12.0f,
                                       Measure.CrestAlong, Measure.CrestAlong);

    Surface->Ground(Plate, Ink.Medallion, Measure.RadiusPlate);
    Surface->Edge(Plate, Ink.Stroke, 1.0f, Measure.RadiusPlate);
    Surface->Stroke(ArchiveCrest(Record.Archive),
                    Spanning(Plate.LeastAlong + 10.0f, Plate.LeastAcross + 10.0f, 20.0f, 20.0f),
                    Ink.Secondary);

    char Titled[96] = {};
    std::snprintf(Titled, sizeof(Titled), "%s.%s", Record.Naming, Record.Extension);

    Surface->TextRunTruncated(Plate.MostAlong + 12.0f, Crest.LeastAcross + 12.0f,
                              Crest.MostAlong - Plate.MostAlong - 24.0f, Ink.Primary,
                              Titled, Measure.RunCrest, true);

    float TagAlong = Plate.MostAlong + 12.0f;

    for (std::uint32_t Ordinal = 0u; Ordinal < Record.TagCount; ++Ordinal)
    {
        const float Span = Surface->MeasureRun(Record.Tags[Ordinal], Measure.RunCaption) + 12.0f;

        const PlaneExtent Tag = Spanning(TagAlong, Crest.LeastAcross + 34.0f, Span, Measure.ChipAcross);

        Surface->Ground(Tag, Partial(0xFFFFFFu, 0.02), 4.0f);
        Surface->Edge(Tag, Ink.Stroke, 1.0f, 4.0f);
        Surface->TextRun(Tag.LeastAlong + 6.0f, Tag.LeastAcross + 5.0f, Ink.Secondary,
                         Record.Tags[Ordinal], Measure.RunCaption);

        TagAlong += Span + 4.0f;
    }

    // 📐 The properties section — a caption over a run of name/reading pairs.
    const PlaneExtent Properties = Spanning(Extent.LeastAlong, Crest.MostAcross,
                                            Extent.SpanAlong(), 118.0f);

    Surface->Ground(Spanning(Properties.LeastAlong, Properties.MostAcross - 1.0f,
                             Properties.SpanAlong(), 1.0f), Ink.Stroke);

    Surface->TextRunCapitalised(Properties.LeastAlong + 12.0f, Properties.LeastAcross + 12.0f,
                                Ink.Faint, "Properties", Measure.RunCaption, 1.6f, true);

    float Pair = Properties.LeastAcross + 36.0f;

    const auto RecordPair = [&](const char* Naming, const char* Reading)
    {
        Surface->TextRun(Properties.LeastAlong + 12.0f, Pair, Ink.Faint, Naming, Measure.RunBody);

        const float Span = Surface->MeasureRun(Reading, Measure.RunBody);

        Surface->TextRun(Properties.MostAlong - 12.0f - Span, Pair, Ink.Primary,
                         Reading, Measure.RunBody);

        Pair += 20.0f;
    };

    char Extended[16] = {};
    std::snprintf(Extended, sizeof(Extended), "%s", Record.Extension);

    for (std::uint32_t Step = 0u; Extended[Step] != '\0'; ++Step)
    {
        if (Extended[Step] >= 'a' && Extended[Step] <= 'z')
            Extended[Step] = static_cast<char>(Extended[Step] - 'a' + 'A');
    }

    RecordPair("Format",   Extended);
    RecordPair("Category", ArchiveNaming(Record.Archive));

    if (Record.Subheading != nullptr)
        RecordPair("Subcategory", Record.Subheading);

    char Sized[32] = {};
    std::snprintf(Sized, sizeof(Sized), "%.1f MB", Record.Octets / OctetsPerMegaOctet);

    RecordPair("Size", Sized);

    // 📐 `mt-auto` — the import action is pinned to the inspector's own lower edge.
    const PlaneExtent Import = Spanning(Extent.LeastAlong + 12.0f,
                                        Extent.MostAcross - 12.0f - Measure.ImportAcross,
                                        Extent.SpanAlong() - 24.0f, Measure.ImportAcross);

    static_cast<void>(Pressed(ChromeCells[5], Import, Seated, "Import this record"));

    const bool ImportOver = Roused(Import);

    Surface->Ground(Import, ImportOver ? Covering(0xE5E5E5u) : Covering(0xFFFFFFu), Measure.RadiusSoft);

    const float Span = Surface->MeasureRun("Import", Measure.RunBody);

    Surface->Stroke(SymbolSubject::PersistDisc,
                    Spanning(Import.LeastAlong + (Import.SpanAlong() - Span - 20.0f) * 0.5f,
                             Import.LeastAcross + 11.0f, 14.0f, 14.0f),
                    Covering(0x000000u));

    Surface->TextRun(Import.LeastAlong + (Import.SpanAlong() - Span - 20.0f) * 0.5f + 20.0f,
                     Import.LeastAcross + 12.0f, Covering(0x000000u), "Import",
                     Measure.RunBody, 0.0f, true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE WHOLE BROWSER
//------------------------------------------------------------------------------------------------------------------------

void ContentBrowserPanel::RecordBrowser(const PlaneExtent& Extent, ContentLibrary& Library,
                                        ContentBrowserOrdinates& Seated)
{
    if (Ledger == nullptr || Surface == nullptr)
        return;

    Surface->Ground(Extent, Ink.Ground);

    // 📐 `h-screen w-full flex` — the sources aside, then the main column, then the inspector aside. Both
    //    asides are `flex-none`, so the lattice takes whatever the two of them leave.
    const PlaneExtent Aside = Spanning(Extent.LeastAlong, Extent.LeastAcross,
                                       Measure.AsideAlong, Extent.SpanAcross());

    const float MainAlong = Extent.SpanAlong() - Measure.AsideAlong - Measure.InspectorAlong;

    const PlaneExtent Rail = Spanning(Aside.MostAlong, Extent.LeastAcross,
                                      MainAlong, Measure.TopRailAcross);

    const PlaneExtent Lattice = Spanning(Aside.MostAlong, Rail.MostAcross,
                                         MainAlong, Extent.SpanAcross() - Measure.TopRailAcross);

    const PlaneExtent Inspector = Spanning(Extent.MostAlong - Measure.InspectorAlong, Extent.LeastAcross,
                                           Measure.InspectorAlong, Extent.SpanAcross());

    RecordSources(Aside, Library, Seated);
    RecordSeekRail(Rail, Seated);
    RecordLattice(Lattice, Library, Seated);
    RecordInspector(Inspector, Library, Seated);
}

void ContentBrowserPanel::RecordDeferred(ContentBrowserOrdinates& Seated)
{
    if (Surface == nullptr || Seated.Tooltip == nullptr)
        return;

    // 📐 The tooltip card, above everything the tick recorded, seated at the roused control's upper edge.
    const float Span = Surface->MeasureRun(Seated.Tooltip, Measure.RunCaption);

    const PlaneExtent Card = Spanning(Seated.TooltipAlong - (Span + 16.0f) * 0.5f,
                                      Seated.TooltipAcross - 30.0f, Span + 16.0f, 24.0f);

    Surface->Ground(Card, Covering(0x17171Bu), Measure.RadiusSoft);
    Surface->Edge(Card, Ink.Stroke, 1.0f, Measure.RadiusSoft);
    Surface->TextRun(Card.LeastAlong + 8.0f, Card.LeastAcross + 6.0f, Ink.Secondary,
                     Seated.Tooltip, Measure.RunCaption);

    Seated.Tooltip = nullptr;
}

}   // namespace Slate
