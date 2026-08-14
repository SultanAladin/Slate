//============================================================================================================================================
//                                                            OUTLINERPANEL.CPP
//============================================================================================================================================
// 🧩 The counted span presented inside the rectangle the desk resolved, and every gesture over it turned into a declared intent.

#include "SlateUI/Interface/OutlinerPanel/Api/OutlinerPanel.h"

#include <cstdio>
#include <cstring>

// 📝 🔴 No vendor spelling appears in this file and none may. Every fill, outline, run of text and clip goes through
//    `ControlPanel`'s painting seam, which is the one component that knows which recording the interface paints on.
//    Before this the panel opened a window of its own and read the vendor's pointer directly, which made it the one
//    panel of the six the desk could not place — recorded against `14` §7.

namespace Slate
{
namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE REFERENCE GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

// 📝 What this panel spells that the theme does not. Everything the theme already carries — the 46 px header, the
//    paddings, the roundings, the glyph squares — is read from `LayoutExtents` and is not repeated here.
constexpr float RowHeight       = 24.0f;   // [px] - one outliner row, tighter than a layer row's 44
constexpr float IndentStep      = 14.0f;   // [px] - one enclosure of depth
constexpr float SubjectDotEdge  =  4.0f;   // [px] - the dot beside a row's name
constexpr float DropLineHeight  =  2.0f;   // [px] - the accent line over the row a release would enclose in
constexpr float ToolbarPadding  =  7.0f;   // [px] - the toolbar's own inset, narrower than a panel's

// 📝 The readout buffer every row prints into. A name reaches sixty-three characters and a count reaches ten, and a
//    fixed extent keeps the whole presentation allocation-free.
constexpr std::uint32_t RowTextExtent = 128u;

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

WorkspaceRectangle AfterLeft(const WorkspaceRectangle& Area, float Consumed)
{
    const float Taken = Consumed < Area.Width ? Consumed : Area.Width;

    return { Area.PositionX + Taken, Area.PositionY, Area.Width - Taken, Area.Height };
}

WorkspaceRectangle SquareCentred(const WorkspaceRectangle& Area, float Edge)
{
    return { Area.PositionX + (Area.Width  - Edge) * 0.5f,
             Area.PositionY + (Area.Height - Edge) * 0.5f,
             Edge,
             Edge };
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   DECLARING INTENT
//------------------------------------------------------------------------------------------------------------------------

// 📝 One subset or expansion intent, declared where the gesture arrived. A refusal from Declare is the
//    sequence's to report through `86`, so the panel neither collects it nor presents a refusal of its own.
void DeclareStanding(OutlinerSequence& Outliner,
                     OutlinerIntent    Declared,
                     OccupantIdentity  Subject,
                     bool              StandingEnabled)
{
    DeclaredIntent Arriving;
    Arriving.Declared        = Declared;
    Arriving.Subject         = Subject;
    Arriving.StandingEnabled = StandingEnabled;

    Outliner.Declare(Arriving);
}

// 📝 A selection intent carries whether it extends the standing selection, which is the modifier's only effect
//    here: the panel reports what the artist did and the sequence decides what the selection becomes.
void DeclareSelection(OutlinerSequence& Outliner, OccupantIdentity Subject, bool SelectionExtended)
{
    DeclaredIntent Arriving;
    Arriving.Declared          = OutlinerIntent::Select;
    Arriving.Subject           = Subject;
    Arriving.StandingEnabled   = true;
    Arriving.SelectionExtended = SelectionExtended;

    Outliner.Declare(Arriving);
}

// 🔴 `12` §7: reordering is a transaction against the enclosure relation, declared like any other edit. The
//    panel never touches the relation — a drag that mutated it directly would bypass undo, and its absence from
//    the revision sequence is discovered by the artist rather than by a test.
void DeclareEnclosure(OutlinerSequence& Outliner,
                      OccupantIdentity  Subject,
                      OccupantIdentity  ProposedEnclosure,
                      std::uint32_t     OrderWithinEnclosure)
{
    DeclaredIntent Arriving;
    Arriving.Declared             = OutlinerIntent::Enclose;
    Arriving.Subject              = Subject;
    Arriving.RelatedOccupant      = ProposedEnclosure;
    Arriving.OrderWithinEnclosure = OrderWithinEnclosure;

    Outliner.Declare(Arriving);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE BANDS
//------------------------------------------------------------------------------------------------------------------------

void PresentHeaderBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       std::uint32_t              CountedTotal)
{
    PresentSurfaceFill(Band, Theme.Palette.PanelHeader, 0.0f);
    PresentSurfaceFill({ Band.PositionX, Band.PositionY + Band.Height - Theme.Extents.BorderThickness,
                         Band.Width, Theme.Extents.BorderThickness },
                       Theme.Palette.PanelBorder, 0.0f);

    const WorkspaceRectangle Interior = InsetBy(Band, Theme.Extents.PanelPadding);
    const WorkspaceRectangle GlyphBox = LeftOf(Interior, 24.0f);

    PresentSurfaceFill(GlyphBox, Theme.Palette.TileBackground, Theme.Extents.PillRounding);
    PresentControlStroke(SquareCentred(GlyphBox, Theme.Extents.GlyphEdge),
                         ControlStroke::Grip, Theme.Palette.TextPrimary, 1.5f, 0.0f);

    char Counted[RowTextExtent] = {};
    std::snprintf(Counted, RowTextExtent, "%u", CountedTotal);

    const float              CountWidth = MeasuredTextExtent(Counted, 1.0f) + Theme.Extents.PanelPadding * 2.0f;
    const WorkspaceRectangle CountBadge = RightOf(Interior, CountWidth);

    PresentSurfaceFill(CountBadge, Theme.Palette.TileBackground, Theme.Extents.EntryRounding);
    PresentTextRun(CountBadge, Counted, Theme.Palette.TextMuted, TextPlacement::Centred, 1.0f);

    WorkspaceRectangle Titles = AfterLeft(Interior, 24.0f + Theme.Extents.ControlSpacing);
    Titles.Width = Titles.Width - CountWidth - Theme.Extents.ControlSpacing;

    const WorkspaceRectangle TitleRun    = { Titles.PositionX, Titles.PositionY, Titles.Width, Titles.Height * 0.55f };
    const WorkspaceRectangle SubtitleRun = { Titles.PositionX, Titles.PositionY + Titles.Height * 0.55f,
                                             Titles.Width, Titles.Height * 0.45f };

    PresentTextRun(TitleRun,    "Outliner",       Theme.Palette.TextPrimary, TextPlacement::Leading, 1.0f);
    PresentTextRun(SubtitleRun, "Scene contents", Theme.Palette.TextMuted,   TextPlacement::Leading, 0.85f);
}

// 🧩 The toolbar: the search entry, and what the standing narrowing confirmed beneath it.
void PresentToolbarBand(const ThemeSpecification&  Theme,
                        const WorkspaceRectangle&  Band,
                        OutlinerSequence&          Outliner,
                        OutlinerPanelCarry&        Carry)
{
    const WorkspaceRectangle Interior  = InsetBy(Band, ToolbarPadding);
    const WorkspaceRectangle SearchRow = { Interior.PositionX, Interior.PositionY,
                                           Interior.Width, Theme.Extents.SegmentRowHeight };

    PresentTextEntry(Theme, SearchRow, "", Carry.Sought, "Search names");
    PresentControlStroke(SquareCentred(RightOf(SearchRow, Theme.Extents.SegmentRowHeight), Theme.Extents.GlyphEdge),
                         ControlStroke::Search, Theme.Palette.TextMuted, 1.4f, 0.0f);

    // 📝 The narrowing is declared, not applied. `12` §10 rules row narrowing a subset, and a subset arrives as
    //    declared intent — narrowing the linearisation where the keystroke landed would make the presentation the
    //    owner of the thing it displays, which `14` §1 forbids outright.
    const std::string Sought = Carry.Sought.Carried;

    if (Sought != Outliner.Sought())
    {
        DeclaredIntent Narrowing;
        Narrowing.Declared   = OutlinerIntent::Narrow;
        Narrowing.SoughtText = Sought;

        Outliner.Declare(Narrowing);

        Carry.ConfirmedCount =
            Sought.empty() ? 0u : static_cast<std::uint32_t>(Outliner.Names().Narrow(Sought).size());
    }

    if (Sought.empty())
        return;

    char Confirmed[RowTextExtent] = {};
    std::snprintf(Confirmed, RowTextExtent, "%u of %u names confirmed",
                  Carry.ConfirmedCount, Outliner.Names().NamedCount());

    PresentTextRun({ Interior.PositionX,
                     Interior.PositionY + Theme.Extents.SegmentRowHeight,
                     Interior.Width,
                     Theme.Extents.ControlSpacing * 2.0f },
                   Confirmed, Theme.Palette.TextMuted, TextPlacement::Leading, 0.8f);
}

void PresentFooterBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       std::uint32_t              CountedTotal,
                       std::uint32_t              SelectedCount,
                       std::uint32_t              ExcludedCount)
{
    PresentSurfaceFill(Band, Theme.Palette.PanelHeader, 0.0f);
    PresentSurfaceFill({ Band.PositionX, Band.PositionY, Band.Width, Theme.Extents.BorderThickness },
                       Theme.Palette.PanelBorder, 0.0f);

    char Counted[RowTextExtent] = {};
    std::snprintf(Counted, RowTextExtent, "%u rows \xC2\xB7 %u selected \xC2\xB7 %u hidden",
                  CountedTotal, SelectedCount, ExcludedCount);

    PresentTextRun(InsetBy(Band, Theme.Extents.PanelPadding), Counted,
                   Theme.Palette.TextMuted, TextPlacement::Leading, 0.9f);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        ONE ROW
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What one row's tick asked the sequence for, applied after the whole span has been walked.
/// note  🔴 Deferred because `Declare` appends to the pending run the walk does not read, but the subset and
///        expansion queries beneath it do read the linearisation — and applying inside the walk would present a
///        row against relations that a later row in the same tick has already changed.
struct RowIntent
{
    bool              StandingDeclared = false;                     // [-] - a subset or expansion was pressed
    OutlinerIntent    StandingSubject  = OutlinerIntent::Expand;    // [-] - which
    OccupantIdentity  StandingOccupant = {};                        // [-] - the occupant it names
    bool              StandingArriving = false;                     // [-] - what the enrolment should become
    bool              SelectDeclared   = false;                     // [-] - a row was chosen
    OccupantIdentity  SelectOccupant   = {};                        // [-] - which
    bool              SelectExtended   = false;                     // [-] - the modifier was down
};

bool RowAdmitted(const OutlinerPanelCarry& Carry, const char* RowName)
{
    if (Carry.Sought.CarryExtent == 0u)
        return true;

    // 📝 ⚠️ A second narrowing, over the presented text, on top of the one the sequence applies. The declared
    //    narrowing lands at the next tick's ①, so the tick the artist types on would otherwise present every row
    //    while the entry already reads a name — and the panel would look unwired for exactly one tick.
    return std::strstr(RowName, Carry.Sought.Carried) != nullptr;
}

void PresentRow(const ThemeSpecification&  Theme,
                const WorkspaceRectangle&  Row,
                const SequencedRow&        Presented,
                const char*                RowName,
                const EnrollmentIndex&     Subsets,
                OutlinerPanelCarry&        Carry,
                RowIntent&                 Arriving)
{
    const bool Selected = Subsets.Enrolled(Presented.Occupant, SubsetSubject::Selection);
    const bool Excluded = Subsets.Enrolled(Presented.Occupant, SubsetSubject::VisibilityExclusion);
    const bool Locked   = Subsets.Enrolled(Presented.Occupant, SubsetSubject::Lock);

    const ControlInteraction RowPress = ResolveAreaPress(Row);

    PresentSurfaceFill(Row,
                       Selected             ? Theme.Palette.AccentSubtle
                     : RowPress.PointerOver ? Theme.Palette.RowHovered
                                            : Theme.Palette.PanelBackground,
                       Theme.Extents.PillRounding);

    // 📝 The indentation is the row's own declared depth and is never accumulated across the walk. A running
    //    indent drifts by one enclosure for every row the narrowing removed from the span.
    const float Indentation = static_cast<float>(Presented.EnclosureDepth) * IndentStep;

    WorkspaceRectangle Walking = InsetBy(Row, Theme.Extents.ControlSpacing * 0.5f);

    Walking = AfterLeft(Walking, Indentation);

    //--------------------------------------------------------------------------------------------------------------
    // the expansion twisty
    //--------------------------------------------------------------------------------------------------------------

    const WorkspaceRectangle TwistyBox = LeftOf(Walking, Theme.Extents.GlyphButtonSmallEdge);

    // 📝 An occupant that encloses nothing admits no expansion gesture, so none is offered for it and the space it
    //    would occupy is held open instead. Offering an arrow that does nothing is the interface claiming a
    //    structure the relation does not have.
    if (Presented.EnclosedCount != 0u)
    {
        if (ResolveAreaPress(TwistyBox).EditSealed)
        {
            // 🔴 `14` §4.1: expansion is not a transaction. Undo must not step back through the artist collapsing
            //    a row, so it travels as intent and lands as a count adjustment.
            Arriving.StandingDeclared = true;
            Arriving.StandingSubject  = OutlinerIntent::Expand;
            Arriving.StandingOccupant = Presented.Occupant;
            Arriving.StandingArriving = !Presented.ExpansionEnabled;
        }

        PresentControlStroke(SquareCentred(TwistyBox, Theme.Extents.GlyphEdge), ControlStroke::Twisty,
                             Theme.Palette.TextMuted, 1.4f,
                             Presented.ExpansionEnabled ? 1.5707963f : 0.0f);
    }

    Walking = AfterLeft(Walking, Theme.Extents.GlyphButtonSmallEdge);

    //--------------------------------------------------------------------------------------------------------------
    // the two subset glyphs
    //--------------------------------------------------------------------------------------------------------------

    const WorkspaceRectangle EyeBox = LeftOf(Walking, Theme.Extents.GlyphButtonSmallEdge);

    if (ResolveAreaPress(EyeBox).EditSealed)
    {
        Arriving.StandingDeclared = true;
        Arriving.StandingSubject  = OutlinerIntent::ExcludeVisibility;
        Arriving.StandingOccupant = Presented.Occupant;
        Arriving.StandingArriving = !Excluded;
    }

    PresentControlStroke(SquareCentred(EyeBox, Theme.Extents.GlyphEdge), ControlStroke::Eye,
                         Excluded ? Theme.Palette.TextMuted : Theme.Palette.TextPrimary, 1.4f, 0.0f);

    Walking = AfterLeft(Walking, Theme.Extents.GlyphButtonSmallEdge);

    const WorkspaceRectangle LockBox = LeftOf(Walking, Theme.Extents.GlyphButtonSmallEdge);

    if (ResolveAreaPress(LockBox).EditSealed)
    {
        Arriving.StandingDeclared = true;
        Arriving.StandingSubject  = OutlinerIntent::Lock;
        Arriving.StandingOccupant = Presented.Occupant;
        Arriving.StandingArriving = !Locked;
    }

    // 📝 🚧 A lock reads as a cog until the stroke alphabet carries one of its own. `ControlStroke` is a closed
    //    enumeration owned by `ControlPanel`, and minting an enumerator from here would be this panel asserting a
    //    set another component declares. Presented muted where the occupant is unlocked, so the two read apart.
    PresentControlStroke(SquareCentred(LockBox, Theme.Extents.GlyphEdge), ControlStroke::Cog,
                         Locked ? Theme.Palette.DangerPrimary : Theme.Palette.TextMuted, 1.4f, 0.0f);

    Walking = AfterLeft(Walking, Theme.Extents.GlyphButtonSmallEdge + Theme.Extents.ControlSpacing * 0.5f);

    //--------------------------------------------------------------------------------------------------------------
    // the name
    //--------------------------------------------------------------------------------------------------------------

    const WorkspaceRectangle DotBox = { Walking.PositionX,
                                        Walking.PositionY + (Walking.Height - SubjectDotEdge) * 0.5f,
                                        SubjectDotEdge, SubjectDotEdge };

    PresentSurfaceFill(DotBox,
                       Selected ? Theme.Palette.AccentPrimary : Theme.Palette.TextMuted,
                       Theme.Extents.EntryRounding);

    const WorkspaceRectangle NameRun = AfterLeft(Walking, SubjectDotEdge + Theme.Extents.ControlSpacing * 0.5f);

    DeclareClip(Row);
    PresentTextRun(NameRun, RowName,
                   Excluded ? Theme.Palette.TextMuted : Theme.Palette.TextPrimary,
                   TextPlacement::Leading, 1.0f);
    ReclaimClip();

    //--------------------------------------------------------------------------------------------------------------
    // the retirement glyph
    //--------------------------------------------------------------------------------------------------------------

    const WorkspaceRectangle RetireBox = RightOf(InsetBy(Row, Theme.Extents.ControlSpacing * 0.5f),
                                                 Theme.Extents.GlyphButtonSmallEdge);

    if (ResolveAreaPress(RetireBox).EditSealed)
    {
        // 📝 Retirement carries its whole cascade as one transaction — `12` §12. What the occupant encloses is
        //    re-enclosed rather than retired, which is why this reads as one glyph and not as a submenu.
        Arriving.StandingDeclared = true;
        Arriving.StandingSubject  = OutlinerIntent::Retire;
        Arriving.StandingOccupant = Presented.Occupant;
        Arriving.StandingArriving = false;
    }

    if (RowPress.PointerOver)
    {
        PresentControlStroke(SquareCentred(RetireBox, Theme.Extents.GlyphEdge), ControlStroke::Trash,
                             Theme.Palette.DangerPrimary, 1.4f, 0.0f);
    }

    //--------------------------------------------------------------------------------------------------------------
    // choosing and taking hold
    //--------------------------------------------------------------------------------------------------------------

    if (RowPress.EditOpened)
    {
        Arriving.SelectDeclared = true;
        Arriving.SelectOccupant = Presented.Occupant;

        // 📝 ⚠️ 🚧 The extend modifier is not read. `ControlPanel`'s pointer seam reports position and press and
        //    carries no modifier, and reading the vendor's directly is the one thing this file may not do. A drag
        //    that extends the selection arrives when the seam gains a modifier reading, as one argument here.
        Arriving.SelectExtended = false;

        Carry.ReorderOpen     = true;
        Carry.DraggedOccupant = Presented.Occupant;
        Carry.LandingOccupant = Presented.Occupant;
    }

    if (Carry.ReorderOpen && RowPress.PointerOver && Carry.DraggedOccupant != Presented.Occupant)
        Carry.LandingOccupant = Presented.Occupant;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

void PresentOutlinerPanel(const ThemeSpecification&  Theme,
                          const WorkspaceRectangle&  Area,
                          void*                      PresentContext)
{
    OutlinerPanelContext* Standing = static_cast<OutlinerPanelContext*>(PresentContext);

    PresentSurfaceFill(Area, Theme.Palette.PanelBackground, Theme.Extents.CornerRounding);

    if (Standing == nullptr || Standing->Outliner == nullptr || Standing->Carry == nullptr)
    {
        PresentTextRun(Area, "No scene", Theme.Palette.TextMuted, TextPlacement::Centred, 1.0f);
        return;
    }

    OutlinerSequence&   Outliner = *Standing->Outliner;
    OutlinerPanelCarry& Carry    = *Standing->Carry;

    // 🔴 `12` §7 and `14` §6: the rows are read through `RankIndex` and the relations are never read here. The panel
    //    asks the counted ordering which row sits at a visible position and touches nothing else, which is what
    //    keeps the cost proportional to the panel's height rather than to the population.
    const RowSequence&               Sequenced = Outliner.Sequenced();
    const RankIndex&                 Counted   = Sequenced.Counted();
    const std::vector<SequencedRow>& Rows      = Sequenced.Rows();
    const TrigramIndex&              Names     = Outliner.Names();
    const EnrollmentIndex&           Subsets   = Outliner.Enrollments();

    const std::uint32_t CountedTotal = Counted.CountedTotal();

    Carry.RowsPresented = 0u;

    //----------------------------------------------------------------------------------------------------------------
    // the four bands
    //----------------------------------------------------------------------------------------------------------------

    const float ToolbarHeight = ToolbarPadding * 2.0f + Theme.Extents.SegmentRowHeight
                              + Theme.Extents.ControlSpacing * 2.0f;

    const WorkspaceRectangle HeaderBand  = BandOf(Area, 0.0f, Theme.Extents.PanelHeaderHeight);
    const WorkspaceRectangle ToolbarBand = BandOf(Area, Theme.Extents.PanelHeaderHeight, ToolbarHeight);
    const WorkspaceRectangle FooterBand  = BandOf(Area, Area.Height - Theme.Extents.PanelFooterHeight,
                                                  Theme.Extents.PanelFooterHeight);
    const WorkspaceRectangle ListBand    = { Area.PositionX,
                                             Area.PositionY + Theme.Extents.PanelHeaderHeight + ToolbarHeight,
                                             Area.Width,
                                             Area.Height - Theme.Extents.PanelHeaderHeight - ToolbarHeight
                                                         - Theme.Extents.PanelFooterHeight };

    PresentHeaderBand(Theme, HeaderBand, CountedTotal);
    PresentToolbarBand(Theme, ToolbarBand, Outliner, Carry);

    const std::uint32_t SelectedCount = Subsets.EnrolledCount(SubsetSubject::Selection);
    const std::uint32_t ExcludedCount = Subsets.EnrolledCount(SubsetSubject::VisibilityExclusion);

    if (ListBand.Height <= 0.0f)
    {
        PresentFooterBand(Theme, FooterBand, CountedTotal, SelectedCount, ExcludedCount);
        return;
    }

    //----------------------------------------------------------------------------------------------------------------
    // the anchor, restored before the offset is read
    //----------------------------------------------------------------------------------------------------------------

    // 🔴 The occupant the span is anchored on is restored before the offset is read, so that a collapse above the
    //    view is absorbed here rather than felt as a jump. Holding the ordinal alone would keep the artist at row
    //    four hundred while the occupant that was there moved to row two.
    if (Carry.AnchoredOccupant.IdentityDeclared() && CountedTotal != Carry.CountedWhenAnchored)
    {
        const Outcome<std::uint32_t> Held = Sequenced.RowOf(Carry.AnchoredOccupant);

        if (Held.ContentPresent)
        {
            const Outcome<std::uint32_t> Restored = Counted.VisibleOfRow(Held.Resolve());

            // 📝 An anchor whose occupant left the count keeps the offset it had. Scrolling to the nearest counted
            //    row would move the view on a collapse the artist made elsewhere.
            if (Restored.ContentPresent)
                Carry.VisibleOffset = static_cast<float>(Restored.Resolve()) * RowHeight;
        }
    }

    //----------------------------------------------------------------------------------------------------------------
    // the scrolled span of rows
    //----------------------------------------------------------------------------------------------------------------

    AdvanceVisibleOffset(Carry.VisibleOffset, ListBand, static_cast<float>(CountedTotal) * RowHeight);

    const std::uint32_t Anchored = static_cast<std::uint32_t>(Carry.VisibleOffset / RowHeight);

    Carry.VisibleAnchor       = CountedTotal != 0u && Anchored >= CountedTotal ? CountedTotal - 1u : Anchored;
    Carry.CountedWhenAnchored = CountedTotal;

    // 📝 Which occupant the anchor names is recorded from the counted ordering rather than from the presented span,
    //    because the span is walked after this and a clipped first row is still the row the artist is looking at.
    const Outcome<std::uint32_t> Anchoring = Counted.RowAtVisible(Carry.VisibleAnchor);

    Carry.AnchoredOccupant = Anchoring.ContentPresent && Anchoring.Resolve() < Rows.size()
                           ? Rows[Anchoring.Resolve()].Occupant
                           : OccupantIdentity{};

    // 📝 🔴 Only the counted span the panel can show is walked. `RankIndex` turns a visible position into a row
    //    ordinal in logarithmic time — the first of the two scroll questions `12` §3 declares the counts exist to
    //    answer — and the span is bounded by the band's height, so a million occupants walk a few dozen rows.
    const std::uint32_t Spanned = static_cast<std::uint32_t>(ListBand.Height / RowHeight) + 2u;

    RowIntent Arriving = {};

    DeclareClip(ListBand);

    for (std::uint32_t Ordinal = 0u; Ordinal < Spanned; ++Ordinal)
    {
        const std::uint32_t Visible = Carry.VisibleAnchor + Ordinal;

        if (Visible >= CountedTotal)
            break;

        const Outcome<std::uint32_t> Located = Counted.RowAtVisible(Visible);

        if (!Located.ContentPresent || Located.Resolve() >= Rows.size())
            continue;

        const SequencedRow& Presented = Rows[Located.Resolve()];

        char RowName[RowTextExtent] = {};

        // 📝 An unnamed occupant presents its slot ordinal rather than an empty row. `10` issues the slot and the
        //    artist can address what they can see; an empty row is a row they cannot click.
        const std::string& Named = Names.DeclaredName(Presented.Occupant);

        if (Named.empty())
            std::snprintf(RowName, RowTextExtent, "(unnamed %u)", Presented.Occupant.SlotOrdinal);
        else
            std::snprintf(RowName, RowTextExtent, "%s", Named.c_str());

        if (!RowAdmitted(Carry, RowName))
            continue;

        const WorkspaceRectangle Row = { ListBand.PositionX + Theme.Extents.PanelPadding,
                                         ListBand.PositionY
                                             + static_cast<float>(Visible) * RowHeight - Carry.VisibleOffset,
                                         ListBand.Width - Theme.Extents.PanelPadding * 2.0f,
                                         RowHeight };

        if (Carry.ReorderOpen && Carry.LandingOccupant == Presented.Occupant
         && Carry.DraggedOccupant != Presented.Occupant)
        {
            PresentSurfaceFill({ Row.PositionX, Row.PositionY - DropLineHeight, Row.Width, DropLineHeight },
                               Theme.Palette.AccentPrimary, 0.0f);
        }

        PresentRow(Theme, Row, Presented, RowName, Subsets, Carry, Arriving);

        ++Carry.RowsPresented;
    }

    ReclaimClip();

    //----------------------------------------------------------------------------------------------------------------
    // the reorder drag, declared on release
    //----------------------------------------------------------------------------------------------------------------

    if (Carry.ReorderOpen && !PointerHeld())
    {
        // 🔴 The dragged occupant and the landing are both carried from where the drag began, so the release
        //    declares what the drag took hold of rather than what the selection has become by then. `14` §4.2 keeps
        //    the capture for the whole drag, and this is the document half of that rule.
        // 📝 A drop onto a row encloses the dragged occupant in it, first in its ordering. A drop onto an occupant
        //    that encloses nothing still encloses: `12` §1 has no separate grouping mechanism, and an occupant that
        //    encloses another is what a group is.
        if (Carry.DraggedOccupant.IdentityDeclared()
         && Carry.LandingOccupant.IdentityDeclared()
         && Carry.DraggedOccupant != Carry.LandingOccupant)
        {
            DeclareEnclosure(Outliner, Carry.DraggedOccupant, Carry.LandingOccupant, 0u);
        }

        Carry.ReorderOpen     = false;
        Carry.DraggedOccupant = OccupantIdentity{};
        Carry.LandingOccupant = OccupantIdentity{};
    }

    //----------------------------------------------------------------------------------------------------------------
    // the deferred intent
    //----------------------------------------------------------------------------------------------------------------

    if (Arriving.SelectDeclared)
        DeclareSelection(Outliner, Arriving.SelectOccupant, Arriving.SelectExtended);

    if (Arriving.StandingDeclared)
    {
        DeclareStanding(Outliner, Arriving.StandingSubject, Arriving.StandingOccupant, Arriving.StandingArriving);
    }

    PresentFooterBand(Theme, FooterBand, CountedTotal, SelectedCount, ExcludedCount);
}

}   // namespace Slate
