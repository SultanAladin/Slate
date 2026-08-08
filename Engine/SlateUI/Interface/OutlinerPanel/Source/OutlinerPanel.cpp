//============================================================================================================================================
//                                                            OUTLINERPANEL.CPP
//============================================================================================================================================
// 🧩 The counted span presented, and every gesture over it turned into a declared intent.

#include "SlateUI/Interface/OutlinerPanel/Api/OutlinerPanel.h"

#include "imgui.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                THE REORDERING PAYLOAD
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📝 The vendor payload spelling, at most thirty-two characters and not beginning with an underscore.
constexpr const char* ReorderPayload = "SlateOutlinerRow";   // [-] - names the payload a drop will accept

// 📝 A dragged occupant travels as its two identity halves rather than as an OccupantIdentity, because the
//    payload is a byte copy and a plain pair of ordinals says so without relying on the identity's layout. The
//    generation travels with it, so a drop onto a retired occupant refuses at ① instead of hitting its slot.
struct DraggedRow
{
    std::uint32_t  SlotOrdinal    = 0u;   // [-] - the dragged occupant's slot
    std::uint32_t  SlotGeneration = 0u;   // [-] - the generation it was dragged at
};

OccupantIdentity DraggedIdentity(const DraggedRow& Carried)
{
    OccupantIdentity Dragged;
    Dragged.SlotOrdinal    = Carried.SlotOrdinal;
    Dragged.SlotGeneration = Carried.SlotGeneration;

    return Dragged;
}

DraggedRow CarriedIdentity(OccupantIdentity Dragging)
{
    DraggedRow Carried;
    Carried.SlotOrdinal    = Dragging.SlotOrdinal;
    Carried.SlotGeneration = Dragging.SlotGeneration;

    return Carried;
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

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> OutlinerPanel::Present(OutlinerSequence& Outliner)
{
    if (ImGui::GetCurrentContext() == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "no interface context is current" });

    RowsPresented = 0u;

    if (!PresenceEnabled)
        return Outcome<bool>::Deliver(true);

    if (!ImGui::Begin("Outliner", &PresenceEnabled))
    {
        ImGui::End();
        return Outcome<bool>::Deliver(true);
    }

    // 🔴 `12` §7 and `14` §6: the rows are read through RankIndex and the relations are never read here. The
    //    panel asks the counted ordering which row sits at a visible position and touches nothing else, which
    //    is what keeps the cost proportional to the panel's height rather than to the population.
    const RowSequence&               Sequenced = Outliner.Sequenced();
    const RankIndex&                 Counted   = Sequenced.Counted();
    const std::vector<SequencedRow>& Rows      = Sequenced.Rows();
    const TrigramIndex&              Names     = Outliner.Names();
    const EnrollmentIndex&           Subsets   = Outliner.Enrollments();

    ImGui::TextUnformatted("Name");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##Sought", SoughtEntry, sizeof(SoughtEntry));

    // 📝 The narrowing is declared, not applied. `12` §10 rules row narrowing a subset, and a subset arrives
    //    as declared intent — narrowing the linearisation where the keystroke landed would make the
    //    presentation the owner of the thing it displays, which `14` §1 forbids outright.
    const std::string Sought = SoughtEntry;

    if (Sought != Outliner.Sought())
    {
        DeclaredIntent Narrowing;
        Narrowing.Declared   = OutlinerIntent::Narrow;
        Narrowing.SoughtText = Sought;

        Outliner.Declare(Narrowing);

        ConfirmedCount = Sought.empty() ? 0u : static_cast<std::uint32_t>(Names.Narrow(Sought).size());
    }

    if (!Sought.empty())
        ImGui::TextDisabled("%u of %u names confirmed", ConfirmedCount, Names.NamedCount());

    ImGui::Separator();

    const std::uint32_t CountedTotal = Counted.CountedTotal();

    if (ImGui::BeginChild("Rows", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders))
    {
        // 📐 The scroll position is read back as a counted ordinal rather than held as a pixel offset — `14`
        //    §4.1 places it beside the document and `12` §7 makes it a row index resolved by count.
        // 📐 The pitch is a framed row's, not a text line's. Every row carries an expansion arrow or the space
        //    one would occupy, so a text-line pitch would tell the clipper a height no row actually has — and
        //    the clipper both submits the wrong span and places every row past the first at the wrong offset.
        const float RowHeight = ImGui::GetFrameHeightWithSpacing();

        if (RowHeight > 0.0f && CountedTotal != 0u)
        {
            // 🔴 The occupant the span is anchored on is restored before the offset is read, so that a collapse
            //    above the view is absorbed here rather than felt as a jump. Holding the ordinal alone would
            //    keep the artist at row four hundred while the occupant that was there moved to row two.
            if (AnchoredOccupant.IdentityDeclared() && CountedTotal != CountedWhenAnchored)
            {
                const Outcome<std::uint32_t> Held = Sequenced.RowOf(AnchoredOccupant);

                if (Held.ContentPresent)
                {
                    const Outcome<std::uint32_t> Restored = Counted.VisibleOfRow(Held.Resolve());

                    // 📝 An anchor whose occupant left the count keeps the ordinal it had. Scrolling to the
                    //    nearest counted row would move the view on a collapse the artist made elsewhere.
                    if (Restored.ContentPresent)
                        ImGui::SetScrollY(static_cast<float>(Restored.Resolve()) * RowHeight);
                }
            }

            const std::uint32_t Anchored = static_cast<std::uint32_t>(ImGui::GetScrollY() / RowHeight);

            VisibleAnchor       = Anchored >= CountedTotal ? CountedTotal - 1u : Anchored;
            CountedWhenAnchored = CountedTotal;

            // 📝 Which occupant the anchor names is recorded from the counted ordering rather than from the
            //    presented span, because the span is submitted after this and a clipped first row is still the
            //    row the artist is looking at.
            const Outcome<std::uint32_t> Anchoring = Counted.RowAtVisible(VisibleAnchor);

            AnchoredOccupant = Anchoring.ContentPresent && Anchoring.Resolve() < Rows.size()
                             ? Rows[Anchoring.Resolve()].Occupant
                             : OccupantIdentity{};
        }
        else
        {
            VisibleAnchor       = 0u;
            CountedWhenAnchored = CountedTotal;
            AnchoredOccupant    = OccupantIdentity{};
        }

        // 📝 Only the counted span the panel can show is submitted. Every ordinal handed back is a position
        //    among counted rows, and RankIndex turns it into a row ordinal in logarithmic time — the first of
        //    the two scroll questions `12` §3 declares the counts exist to answer.
        ImGuiListClipper Presenting;
        Presenting.Begin(static_cast<int>(CountedTotal), RowHeight);

        while (Presenting.Step())
        {
            for (int Visible = Presenting.DisplayStart; Visible < Presenting.DisplayEnd; ++Visible)
            {
                const Outcome<std::uint32_t> Located = Counted.RowAtVisible(static_cast<std::uint32_t>(Visible));

                if (!Located.ContentPresent)
                    continue;

                const std::uint32_t RowOrdinal = Located.Resolve();

                if (RowOrdinal >= Rows.size())
                    continue;

                const SequencedRow& Presented = Rows[RowOrdinal];

                ++RowsPresented;

                ImGui::PushID(static_cast<int>(Presented.Occupant.SlotOrdinal));

                const float Indentation = static_cast<float>(Presented.EnclosureDepth)
                                        * ImGui::GetStyle().IndentSpacing;

                if (Indentation > 0.0f)
                    ImGui::Indent(Indentation);

                // 📝 An occupant that encloses nothing admits no expansion gesture, so none is offered for it
                //    and the space it would occupy is held open instead. Offering an arrow that does nothing is
                //    the interface claiming a structure the relation does not have.
                if (Presented.EnclosedCount != 0u)
                {
                    const ImGuiDir Facing = Presented.ExpansionEnabled ? ImGuiDir_Down : ImGuiDir_Right;

                    if (ImGui::ArrowButton("Expansion", Facing))
                    {
                        // 🔴 `14` §4.1: expansion is not a transaction. Undo must not step back through the
                        //    artist collapsing a row, so it travels as intent and lands as a count adjustment.
                        DeclareStanding(Outliner,
                                        OutlinerIntent::Expand,
                                        Presented.Occupant,
                                        !Presented.ExpansionEnabled);
                    }
                }
                else
                {
                    ImGui::Dummy(ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()));
                }

                ImGui::SameLine();

                const std::string& Named    = Names.DeclaredName(Presented.Occupant);
                const bool         Selected = Subsets.Enrolled(Presented.Occupant, SubsetSubject::Selection);

                // 📝 An unnamed occupant presents its slot ordinal rather than an empty row. `10` issues the
                //    slot and the artist can address what they can see; an empty row is a row they cannot click.
                if (Named.empty())
                    ImGui::Selectable("(unnamed)", Selected, ImGuiSelectableFlags_AllowOverlap);
                else
                    ImGui::Selectable(Named.c_str(), Selected, ImGuiSelectableFlags_AllowOverlap);

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                    DeclareSelection(Outliner, Presented.Occupant, ImGui::GetIO().KeyCtrl);

                // 🔴 The dragged occupant and its enclosure both travel through the payload, so the drop
                //    declares what the drag began rather than what the selection has become by release. `14`
                //    §4.2 keeps the capture for the whole drag, and this is the document half of that rule.
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    const DraggedRow Carried = CarriedIdentity(Presented.Occupant);

                    ImGui::SetDragDropPayload(ReorderPayload, &Carried, sizeof(Carried));
                    ImGui::TextUnformatted(Named.empty() ? "(unnamed)" : Named.c_str());
                    ImGui::EndDragDropSource();
                }

                if (ImGui::BeginDragDropTarget())
                {
                    const ImGuiPayload* Arriving = ImGui::AcceptDragDropPayload(ReorderPayload);

                    if (Arriving != nullptr && Arriving->DataSize == static_cast<int>(sizeof(DraggedRow)))
                    {
                        DraggedRow Carried = {};
                        const char* Bytes  = static_cast<const char*>(Arriving->Data);

                        for (std::size_t Ordinal = 0u; Ordinal < sizeof(DraggedRow); ++Ordinal)
                            reinterpret_cast<char*>(&Carried)[Ordinal] = Bytes[Ordinal];

                        // 📝 A drop onto a row encloses the dragged occupant in it, first in its ordering. A
                        //    drop onto an occupant that encloses nothing still encloses: `12` §1 has no separate
                        //    grouping mechanism, and an occupant that encloses another is what a group is.
                        DeclareEnclosure(Outliner, DraggedIdentity(Carried), Presented.Occupant, 0u);
                    }

                    ImGui::EndDragDropTarget();
                }

                // 📝 The three document subsets and retirement travel from one place, so no gesture reaches a
                //    subset without becoming a transaction. `12` §11 admits no unrecorded mutation.
                if (ImGui::BeginPopupContextItem("Row"))
                {
                    const bool Excluded = Subsets.Enrolled(Presented.Occupant, SubsetSubject::VisibilityExclusion);
                    const bool Isolated = Subsets.Enrolled(Presented.Occupant, SubsetSubject::Isolation);
                    const bool Locked   = Subsets.Enrolled(Presented.Occupant, SubsetSubject::Lock);

                    if (ImGui::MenuItem("Visible", nullptr, !Excluded))
                    {
                        DeclareStanding(Outliner,
                                        OutlinerIntent::ExcludeVisibility,
                                        Presented.Occupant,
                                        !Excluded);
                    }

                    // 📝 Isolation is offered while the occupant is excluded from visibility and the enrolment
                    //    is refused at ① — `12` §10 rejects multi-enrollment at commit rather than resolving it
                    //    by a precedence nobody declared, and the refusal is `86`'s to present.
                    if (ImGui::MenuItem("Isolated", nullptr, Isolated))
                        DeclareStanding(Outliner, OutlinerIntent::Isolate, Presented.Occupant, !Isolated);

                    if (ImGui::MenuItem("Locked", nullptr, Locked))
                        DeclareStanding(Outliner, OutlinerIntent::Lock, Presented.Occupant, !Locked);

                    ImGui::Separator();

                    // 📝 Retirement carries its whole cascade as one transaction — `12` §12. What the occupant
                    //    encloses is re-enclosed rather than retired, which is why this reads as one item.
                    if (ImGui::MenuItem("Retire"))
                        DeclareStanding(Outliner, OutlinerIntent::Retire, Presented.Occupant, false);

                    ImGui::EndPopup();
                }

                if (Indentation > 0.0f)
                    ImGui::Unindent(Indentation);

                ImGui::PopID();
            }
        }

        Presenting.End();
    }

    ImGui::EndChild();
    ImGui::End();

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

void OutlinerPanel::DeclarePresence(bool PresenceDeclared)
{
    PresenceEnabled = PresenceDeclared;
}

bool OutlinerPanel::PresenceStanding() const
{
    return PresenceEnabled;
}

std::uint32_t OutlinerPanel::VisiblePosition() const
{
    return VisibleAnchor;
}

OccupantIdentity OutlinerPanel::Anchored() const
{
    return AnchoredOccupant;
}

std::uint32_t OutlinerPanel::RowsTouched() const
{
    return RowsPresented;
}

std::string OutlinerPanel::Sought() const
{
    return std::string(SoughtEntry);
}

std::uint32_t OutlinerPanel::ConfirmedNames() const
{
    return ConfirmedCount;
}

}   // namespace Slate
