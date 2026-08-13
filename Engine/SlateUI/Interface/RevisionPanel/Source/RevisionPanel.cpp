//============================================================================================================================================
//                                                           REVISIONPANEL.CPP
//============================================================================================================================================
// 🧩 `Inspector.tsx`'s revision timeline over the real sequence — bubble column, spine, card, fold.

#include "SlateUI/Interface/RevisionPanel/Api/RevisionPanel.h"

#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"

#include "imgui.h"

#include <cstdio>

namespace Slate
{
namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE REFERENCE GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

// 📝 Every extent here that the theme carries is read from the theme. What remains is the geometry `Inspector.tsx`
//    spells inside one row and nowhere else — a bubble column, a rail and a node. Promoting them to the theme
//    would offer a density profile control over the interior of one panel, which is not what a density is.
constexpr float BubbleColumnWidth = 32.0f;   // [px] - the ordinal column left of the spine
constexpr float BubbleEdge        = 25.0f;   // [px] - the circular ordinal
constexpr float SpineWidth        = 15.0f;   // [px] - the column the rail runs down
constexpr float RailWidth         =  6.0f;   // [px] - the hued rail itself
constexpr float NodeEdge          =  7.0f;   // [px] - the white node on the rail
constexpr float NodeShadowReach   =  3.0f;   // [px] - `0 0 0 3px` around the node
constexpr float FoldRowHeight     = 18.0f;   // [px] - one line of an opened fold
constexpr float TwistyEdge        = 14.0f;   // [px]

// 📝 A row's fold state is remembered by position, and positions past the carry's extent simply do not remember.
//    Bounding the memory rather than the sequence is what keeps a thousand-transaction session presentable.
bool FoldStanding(const RevisionPanelCarry& Carry, std::size_t Position)
{
    return Position < RevisionFoldCapacity && Carry.FoldOpen[Position];
}

WorkspaceRectangle Inset(const WorkspaceRectangle& Area, float Reach)
{
    WorkspaceRectangle Narrowed;

    Narrowed.PositionX = Area.PositionX + Reach;
    Narrowed.PositionY = Area.PositionY + Reach;
    Narrowed.Width     = Area.Width  - Reach * 2.0f;
    Narrowed.Height    = Area.Height - Reach * 2.0f;

    return Narrowed;
}

// 📝 🔴 The description, or the operation name where none was supplied — `84` §2. The fallback is legible and is
//    not an acceptable default: every row that reaches it is a missing description at the `Open` site, which is
//    why the two are distinguishable in the presentation rather than blended into one string.
const char* PresentedDescription(const CommittedTransaction& Standing, bool& FallbackDeclared)
{
    FallbackDeclared = Standing.Description.empty();

    if (!FallbackDeclared)
        return Standing.Description.c_str();

    return Standing.OperationName.empty() ? "unnamed operation" : Standing.OperationName.c_str();
}

// 📝 The stamp is nanoseconds since the session's own origin, so it is printed as an interval and never as a wall
//    clock. A wall clock here would be the panel inventing a calendar the sequence does not carry.
void PresentedStamp(std::uint64_t SealedAt, char* Printed, std::size_t Extent)
{
    const std::uint64_t Seconds = SealedAt / 1000000000ull;

    std::snprintf(Printed, Extent, "%llu:%02llu", static_cast<unsigned long long>(Seconds / 60ull),
                                                  static_cast<unsigned long long>(Seconds % 60ull));
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE DESTRUCTIVE FACT
//------------------------------------------------------------------------------------------------------------------------

std::uint64_t DiscardCountStanding(const RevisionSequence& Sequence)
{
    const std::uint64_t Committed = static_cast<std::uint64_t>(Sequence.Committed().size());
    const std::uint64_t Position  = Sequence.ScrubPosition();

    // 📝 A position beyond the committed count is not arithmetic this panel corrects. It cannot arise from the
    //    sequence's own calls, and subtracting it would present a discard count of four billion.
    return Position >= Committed ? 0u : Committed - Position;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE SCRUBBING
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> ScrubToPosition(RevisionSequence& Sequence, std::uint64_t Arriving)
{
    const std::uint64_t Committed = static_cast<std::uint64_t>(Sequence.Committed().size());
    const std::uint64_t Sought    = Arriving > Committed ? Committed : Arriving;

    while (Sequence.ScrubPosition() > Sought)
    {
        // 📝 🔴 One inverse at a time, in order — `84` §3. The loop is bounded by the position rather than by a
        //    count of its own, so a `Retreat` that refuses stops it rather than spinning against a sequence that
        //    will not move.
        if (!Sequence.Retreat().ContentPresent)
            return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "a replayed inverse refused" });
    }

    while (Sequence.ScrubPosition() < Sought)
    {
        if (!Sequence.Advance().ContentPresent)
            return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "a replayed operation refused" });
    }

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE HEADER BAND
//------------------------------------------------------------------------------------------------------------------------

namespace
{

void PresentHeaderBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Area,
                       const RevisionSequence&    Sequence)
{
    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    WorkspaceRectangle Band = Area;
    Band.Height = Extents.PanelHeaderHeight;

    PresentSurfaceFill(Band, Palette.PanelHeader, 0.0f);

    WorkspaceRectangle GlyphBox;
    GlyphBox.PositionX = Band.PositionX + Extents.PanelPadding;
    GlyphBox.PositionY = Band.PositionY + (Band.Height - 24.0f) * 0.5f;
    GlyphBox.Width     = 24.0f;
    GlyphBox.Height    = 24.0f;

    PresentSurfaceFill(GlyphBox, Palette.TileBackground, Extents.CornerRounding * 0.5f);
    PresentControlStroke(GlyphBox, ControlStroke::Reload, Palette.TextPrimary, 1.5f, 0.0f);

    WorkspaceRectangle Caption;
    Caption.PositionX = GlyphBox.PositionX + GlyphBox.Width + Extents.ControlSpacing;
    Caption.PositionY = Band.PositionY;
    Caption.Width     = Band.Width - (Caption.PositionX - Band.PositionX) - Extents.PanelPadding;
    Caption.Height    = Band.Height;

    PresentTextRun(Caption, "History", Palette.TextPrimary, TextPlacement::Leading, 1.0f);

    // 📝 The count badge presents the committed extent and the position together, because a sequence of thirty
    //    scrubbed back to twelve is a different situation from one of twelve, and one number cannot say which.
    char Counted[32] = {};

    std::snprintf(Counted, sizeof Counted, "%llu / %llu",
                  static_cast<unsigned long long>(Sequence.ScrubPosition()),
                  static_cast<unsigned long long>(Sequence.Committed().size()));

    PresentTextRun(Caption, Counted, Palette.TextMuted, TextPlacement::Trailing, 1.0f);

    WorkspaceRectangle Rule = Band;
    Rule.PositionY = Band.PositionY + Band.Height - Extents.BorderThickness;
    Rule.Height    = Extents.BorderThickness;

    PresentSurfaceFill(Rule, Palette.PanelBorder, 0.0f);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE DISCARD CONFIRMATION
//------------------------------------------------------------------------------------------------------------------------

// 📝 🔴 `84` §3.1's whole point, and the first thing built rather than the last. The band stands whenever the
//    position is behind the end — the artist sees what an edit would cost **before** deciding to make one, not
//    after. The prompt beneath it is what a caller gates the edit itself on.
void PresentDiscardBand(const ThemeSpecification&  Theme,
                        const WorkspaceRectangle&  Area,
                        const RevisionSequence&    Sequence,
                        RevisionPanelCarry&        Carry,
                        float&                     Travelled)
{
    const std::uint64_t Standing = DiscardCountStanding(Sequence);

    if (Standing == 0u)
    {
        // 📝 A position back at the end closes a prompt that was open. Leaving it standing would name a count of
        //    transactions that no longer exists, and accepting it would confirm a discard of nothing.
        Carry.DiscardPromptOpen  = false;
        Carry.DiscardPromptCount = 0u;

        return;
    }

    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    WorkspaceRectangle Band;
    Band.PositionX = Area.PositionX + Extents.PanelPadding;
    Band.PositionY = Travelled;
    Band.Width     = Area.Width - Extents.PanelPadding * 2.0f;
    Band.Height    = Extents.SegmentRowHeight;

    PresentSurfaceFill(Band, Attenuate(Palette.DangerPrimary, 0.18), Extents.CornerRounding * 0.5f);
    PresentSurfaceOutline(Band, Palette.DangerPrimary, Extents.CornerRounding * 0.5f, Extents.BorderThickness);

    char Warned[96] = {};

    std::snprintf(Warned, sizeof Warned, "  editing here discards %llu revision%s",
                  static_cast<unsigned long long>(Standing), Standing == 1u ? "" : "s");

    PresentTextRun(Band, Warned, Palette.DangerPrimary, TextPlacement::Leading, 1.0f);

    Travelled += Band.Height + Extents.ControlSpacing;

    if (!Carry.DiscardPromptOpen)
        return;

    // 📝 The count the prompt names is the one resolved when it opened, and is compared against the count now.
    //    A sequence that moved under an open prompt makes the confirmation stale, and confirming a stale count
    //    is confirming a discard the artist was never shown.
    WorkspaceRectangle Prompt = Band;
    Prompt.PositionY = Travelled;
    Prompt.Height    = Extents.SegmentRowHeight;

    const bool Stale = Carry.DiscardPromptCount != Standing;

    WorkspaceRectangle Accept = Prompt;
    Accept.Width     = 96.0f;
    Accept.PositionX = Prompt.PositionX + Prompt.Width - Accept.Width;

    WorkspaceRectangle Decline = Accept;
    Decline.PositionX = Accept.PositionX - Accept.Width - Extents.ControlSpacing;

    PresentTextRun(Prompt, Stale ? "  the sequence moved — confirm again" : "  discard them?",
                   Palette.TextPrimary, TextPlacement::Leading, 1.0f);

    if (PresentMenuPill(Theme, Decline, "Keep", false).ContentPresent
     && PresentMenuPill(Theme, Decline, "Keep", false).Resolve().EditSealed)
    {
        Carry.DiscardPromptOpen = false;
    }

    const Outcome<ControlInteraction> Accepted = PresentMenuPill(Theme, Accept, "Discard", true);

    if (Accepted.ContentPresent && Accepted.Resolve().EditSealed && !Stale)
    {
        // 📝 🔴 The panel records the confirmation and discards nothing itself. `10` owns the sequence, and a
        //    panel that truncated it would be the one component that both presents and mutates what it presents.
        Carry.DiscardConfirmed  = true;
        Carry.DiscardPromptOpen = false;
    }

    Carry.DiscardPromptCount = Standing;

    Travelled += Prompt.Height + Extents.ControlSpacing;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        ONE ROW
//------------------------------------------------------------------------------------------------------------------------

float RowExtentOf(const LayoutExtents& Extents, bool FoldOpen)
{
    // 📝 Three folded lines — author, date, comment. `84` §2 names the first two and the reference draws all
    //    three, so the fold's extent is fixed rather than measured; a fold that grew with its comment would make
    //    the content extent depend on text this panel does not own.
    return Extents.RevisionCardHeight + (FoldOpen ? FoldRowHeight * 3.0f + Extents.PanelPadding : 0.0f);
}

void PresentRow(const ThemeSpecification&    Theme,
                const WorkspaceRectangle&    Area,
                const CommittedTransaction&  Standing,
                std::size_t                  Position,
                bool                         Applied,
                bool                         Saved,
                RevisionPanelCarry&          Carry,
                RevisionSequence&            Sequence)
{
    const ThemePalette&  Palette   = Theme.Palette;
    const LayoutExtents& Extents   = Theme.Extents;
    const ImGuiIO&       Pointing  = ImGui::GetIO();
    const bool           FoldOpen  = FoldStanding(Carry, Position);

    // 📝 🔴 Applied-ness is `ScrubPosition()` against the row's own ordinal and is never a field of the row. A
    //    presented applied flag would be a second copy of the position, and it would disagree with the sequence
    //    for exactly as long as it took the next scrub to land.
    const ThemeColour RowText = Applied ? Palette.TextPrimary : Palette.TextMuted;
    const ThemeColour RailHue = Applied ? Palette.AccentPrimary : Attenuate(Palette.AccentPrimary, 0.25);

    // -- the bubble column ---------------------------------------------------------------------------------------------
    WorkspaceRectangle Bubble;
    Bubble.PositionX = Area.PositionX + (BubbleColumnWidth - BubbleEdge) * 0.5f;
    Bubble.PositionY = Area.PositionY + (Extents.RevisionCardHeight - BubbleEdge) * 0.5f;
    Bubble.Width     = BubbleEdge;
    Bubble.Height    = BubbleEdge;

    PresentSurfaceFill(Bubble, Applied ? Palette.TileBackground : Palette.PanelBackground, BubbleEdge * 0.5f);

    char Ordinal[8] = {};

    // 📝 Zero-padded, matching the reference, and top-down: the first transaction is `01` and the count grows
    //    downward. Numbering from the end instead would renumber every row each time one was sealed.
    std::snprintf(Ordinal, sizeof Ordinal, "%02llu", static_cast<unsigned long long>(Position + 1u));

    PresentTextRun(Bubble, Ordinal, RowText, TextPlacement::Centred, 0.85f);

    // -- the spine -----------------------------------------------------------------------------------------------------
    WorkspaceRectangle Rail;
    Rail.PositionX = Area.PositionX + BubbleColumnWidth + (SpineWidth - RailWidth) * 0.5f;
    Rail.PositionY = Area.PositionY;
    Rail.Width     = RailWidth;
    Rail.Height    = RowExtentOf(Extents, FoldOpen);

    PresentSurfaceFill(Rail, Attenuate(RailHue, Applied ? 0.35 : 0.15), RailWidth * 0.5f);

    WorkspaceRectangle Node;
    Node.PositionX = Area.PositionX + BubbleColumnWidth + (SpineWidth - NodeEdge) * 0.5f;
    Node.PositionY = Area.PositionY + (Extents.RevisionCardHeight - NodeEdge) * 0.5f;
    Node.Width     = NodeEdge;
    Node.Height    = NodeEdge;

    PresentSurfaceFill(Inset(Node, -NodeShadowReach), Palette.PanelBackground, NodeEdge);
    PresentSurfaceFill(Node, Applied ? Palette.TextPrimary : Palette.TextMuted, NodeEdge * 0.5f);

    // -- the card ------------------------------------------------------------------------------------------------------
    WorkspaceRectangle Card;
    Card.PositionX = Area.PositionX + BubbleColumnWidth + SpineWidth + Extents.CardGap;
    Card.PositionY = Area.PositionY;
    Card.Width     = Area.PositionX + Area.Width - Card.PositionX;
    Card.Height    = RowExtentOf(Extents, FoldOpen);

    const bool Covered = RectangleCovers(Card, Pointing.MousePos.x, Pointing.MousePos.y);

    PresentSurfaceFill(Card, Covered ? Palette.TileHovered : Palette.TileBackground, Extents.CornerRounding * 0.5f);

    if (!Saved)
    {
        // 📝 🔴 `84` §5's marker, presented as a hairline down the card's leading edge rather than as a badge.
        //    An artist scanning for where their save sits reads an edge in one glance and a badge in as many
        //    glances as there are rows.
        WorkspaceRectangle Unsaved = Card;
        Unsaved.Width = 2.0f;

        PresentSurfaceFill(Unsaved, Palette.AccentPrimary, 1.0f);
    }

    WorkspaceRectangle Caption;
    Caption.PositionX = Card.PositionX + Extents.PanelPadding;
    Caption.PositionY = Card.PositionY;
    Caption.Width     = Card.Width - Extents.PanelPadding * 2.0f - TwistyEdge - Extents.ControlSpacing;
    Caption.Height    = Extents.RevisionCardHeight;

    bool FallbackDeclared = false;

    const char* Described = PresentedDescription(Standing, FallbackDeclared);

    PresentTextRun(Caption, Described,
                   FallbackDeclared ? Palette.TextMuted : RowText, TextPlacement::Leading, 1.0f);

    char Stamped[24] = {};

    PresentedStamp(Standing.SealedAt, Stamped, sizeof Stamped);

    PresentTextRun(Caption, Stamped, Palette.TextMuted, TextPlacement::Trailing, 0.9f);

    WorkspaceRectangle Twisty;
    Twisty.PositionX = Card.PositionX + Card.Width - Extents.PanelPadding - TwistyEdge;
    Twisty.PositionY = Card.PositionY + (Extents.RevisionCardHeight - TwistyEdge) * 0.5f;
    Twisty.Width     = TwistyEdge;
    Twisty.Height    = TwistyEdge;

    PresentControlStroke(Twisty, ControlStroke::Twisty, Palette.TextMuted, 1.4f, FoldOpen ? 1.5708f : 0.0f);

    // -- the fold ------------------------------------------------------------------------------------------------------
    if (FoldOpen)
    {
        WorkspaceRectangle Folded;
        Folded.PositionX = Caption.PositionX;
        Folded.PositionY = Card.PositionY + Extents.RevisionCardHeight;
        Folded.Width     = Caption.Width;
        Folded.Height    = FoldRowHeight;

        // 📝 The operation name is presented in the fold even where it was the caption's fallback. A row reading
        //    its mechanism's spelling twice is legible; a row whose fold hides the one fact that would explain
        //    the caption is not.
        PresentTextRun(Folded, Standing.OperationName.c_str(), Palette.TextMuted, TextPlacement::Leading, 0.9f);

        Folded.PositionY += FoldRowHeight;

        char Detailed[96] = {};

        std::snprintf(Detailed, sizeof Detailed, "sealed at %llu ns",
                      static_cast<unsigned long long>(Standing.SealedAt));

        PresentTextRun(Folded, Detailed, Palette.TextMuted, TextPlacement::Leading, 0.9f);

        Folded.PositionY += FoldRowHeight;

        PresentTextRun(Folded, Standing.MergeDeclared ? "merged with adjacent edits" : "not mergeable",
                       Palette.TextMuted, TextPlacement::Leading, 0.9f);
    }

    if (!Covered || !ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        return;

    if (RectangleCovers(Twisty, Pointing.MousePos.x, Pointing.MousePos.y))
    {
        if (Position < RevisionFoldCapacity)
            Carry.FoldOpen[Position] = !Carry.FoldOpen[Position];

        return;
    }

    // 📝 🔴 A click on the card scrubs to just past this row, and the scrub is not a transaction. `ScrubToPosition`
    //    replays every step between here and there, which is `84` §3 in one call.
    ScrubToPosition(Sequence, static_cast<std::uint64_t>(Position) + 1u);

    // 📝 The prompt opens as soon as the position moves back, not when an edit arrives. `84` §3.1 requires the
    //    count before the discard, and the only instant that is reliably before it is the scrub itself.
    Carry.DiscardPromptCount = DiscardCountStanding(Sequence);
    Carry.DiscardPromptOpen  = Carry.DiscardPromptCount > 0u;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

void PresentRevisionPanel(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, void* PresentContext)
{
    const ThemePalette&  Palette = Theme.Palette;
    const LayoutExtents& Extents = Theme.Extents;

    PresentSurfaceFill(Area, Palette.PanelBackground, Extents.CornerRounding);

    RevisionPanelContext* Presenting = static_cast<RevisionPanelContext*>(PresentContext);

    if (Presenting == nullptr || Presenting->Sequence == nullptr || Presenting->Carry == nullptr)
    {
        // 📝 An empty state and never a refusal. A panel declared before its workspace has a document is the
        //    ordinary arrangement at bring-up, and refusing it would put a report in `86` every tick.
        PresentTextRun(Area, "no revision sequence", Palette.TextMuted, TextPlacement::Centred, 1.0f);

        return;
    }

    RevisionSequence&    Sequence = *Presenting->Sequence;
    RevisionPanelCarry&  Carry    = *Presenting->Carry;

    PresentHeaderBand(Theme, Area, Sequence);

    WorkspaceRectangle Body;
    Body.PositionX = Area.PositionX;
    Body.PositionY = Area.PositionY + Extents.PanelHeaderHeight;
    Body.Width     = Area.Width;
    Body.Height    = Area.Height - Extents.PanelHeaderHeight - Extents.PanelFooterHeight;

    if (Body.Height <= 0.0f)
        return;

    DeclareClip(Body);

    float Travelled = Body.PositionY + Extents.PanelPadding;

    PresentDiscardBand(Theme, Area, Sequence, Carry, Travelled);

    const std::vector<CommittedTransaction>& Committed = Sequence.Committed();
    const std::uint64_t                      Position  = Sequence.ScrubPosition();

    // 📝 The whole content is measured before any of it is presented, because the visible offset is bounded
    //    against the content and a bound applied after the walk is a bound applied one tick late.
    float ContentExtent = 0.0f;

    for (std::size_t Ordinal = 0u; Ordinal < Committed.size(); ++Ordinal)
        ContentExtent += RowExtentOf(Extents, FoldStanding(Carry, Ordinal)) + Extents.CardGap;

    AdvanceVisibleOffset(Carry.VisibleOffset, Body, ContentExtent + (Travelled - Body.PositionY));

    Travelled -= Carry.VisibleOffset;

    for (std::size_t Ordinal = 0u; Ordinal < Committed.size(); ++Ordinal)
    {
        const float RowExtent = RowExtentOf(Extents, FoldStanding(Carry, Ordinal));

        WorkspaceRectangle Row;
        Row.PositionX = Body.PositionX + Extents.PanelPadding;
        Row.PositionY = Travelled;
        Row.Width     = Body.Width - Extents.PanelPadding * 2.0f;
        Row.Height    = RowExtent;

        // 📝 A row entirely outside the body is stepped over rather than presented and clipped. `12` §7's rule
        //    for the outliner holds here for the same reason: the cost of a tick stays proportional to what the
        //    artist can see rather than to how long they have been working.
        const bool Presentable = Row.PositionY + Row.Height >= Body.PositionY
                              && Row.PositionY <= Body.PositionY + Body.Height;

        if (Presentable)
        {
            const bool Applied = Position > static_cast<std::uint64_t>(Ordinal);
            const bool Saved   = Carry.SavedPositionDeclared
                              && Carry.SavedPosition > static_cast<std::uint64_t>(Ordinal);

            PresentRow(Theme, Row, Committed[Ordinal], Ordinal, Applied, Saved, Carry, Sequence);
        }

        Travelled += RowExtent + Extents.CardGap;
    }

    if (Committed.empty())
        PresentTextRun(Body, "no revisions yet", Palette.TextMuted, TextPlacement::Centred, 1.0f);

    ReclaimClip();

    // -- the footer ------------------------------------------------------------------------------------------------
    WorkspaceRectangle Footer;
    Footer.PositionX = Area.PositionX + Extents.PanelPadding;
    Footer.PositionY = Area.PositionY + Area.Height - Extents.PanelFooterHeight;
    Footer.Width     = Area.Width - Extents.PanelPadding * 2.0f;
    Footer.Height    = Extents.PanelFooterHeight;

    char Counted[96] = {};

    std::snprintf(Counted, sizeof Counted, "%llu revision%s%s",
                  static_cast<unsigned long long>(Committed.size()),
                  Committed.size() == 1u ? "" : "s",
                  Sequence.TransactionOpen() ? "  ·  an edit is open" : "");

    // 📝 ⚠️ The open transaction is named in the footer and is emphatically **not** a row — `84` §4. A row would
    //    present a position the sequence cannot be scrubbed to, and it would appear and vanish per stroke.
    PresentTextRun(Footer, Counted, Palette.TextMuted, TextPlacement::Leading, 0.9f);
}

}   // namespace Slate
