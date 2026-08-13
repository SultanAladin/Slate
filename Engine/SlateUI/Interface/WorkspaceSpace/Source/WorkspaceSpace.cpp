//============================================================================================================================================
//                                                           WORKSPACESPACE.CPP
//============================================================================================================================================
// 🧩 Minting, withdrawal and layout for the desk — the recursion itself is templated and lives in the header.

#include "SlateUI/Interface/WorkspaceSpace/Api/WorkspaceSpace.h"

#include <cstdio>
#include <cstring>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                      TITLE MINTING
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📝 A bounded copy into the fixed title extent. A stem longer than the extent is truncated rather than refused:
//    the stem is the application's own literal, and refusing a host's catalogue at bring-up over a caption is a
//    worse answer than a shortened tab.
void CarryTitle(char (&Destination)[WorkspaceTitleExtent], const char* Stem, std::uint32_t Ordinal)
{
    std::snprintf(Destination, WorkspaceTitleExtent, "%s %u", Stem != nullptr ? Stem : "Document", Ordinal);
}

// 📝 How many standing documents already carry a stem, counted rather than remembered. A counter held per
//    catalogue entry would mint "Paint Workspace 4" after three were closed, which reads as three lost documents.
std::uint32_t StemOccurrences(const WorkspaceSpace& Space, const char* Stem)
{
    if (Stem == nullptr)
        return 0u;

    const std::size_t StemExtent = std::strlen(Stem);
    std::uint32_t     Counted    = 0u;

    for (const WorkspaceDocument& Standing : Space.Documents)
    {
        if (!Standing.SlotOccupied)
            continue;

        if (std::strncmp(Standing.Title, Stem, StemExtent) == 0)
            ++Counted;
    }

    return Counted;
}

std::size_t Located(const WorkspaceSpace& Space, WorkspaceDocumentIdentity Subject)
{
    if (!Subject.IdentityDeclared() || Subject.SlotOrdinal >= Space.Documents.size())
        return Space.Documents.size();

    const WorkspaceDocument& Standing = Space.Documents[Subject.SlotOrdinal];

    if (!Standing.SlotOccupied || Standing.Identity != Subject)
        return Space.Documents.size();

    return Subject.SlotOrdinal;
}

// 📝 🔴 Removes one document from whatever carries it and reports the leaf it emptied **without reclaiming it**.
//    Every caller reclaims last: a reclaim lifts a counterpart into the division's own slot and frees two pool
//    slots, so a leaf index resolved before the reclaim names a freed slot after it. Tear and dock both hold
//    such an index across the detach, and this is the single place that ordering is enforced.
std::int32_t Detach(WorkspaceSpace& Space, WorkspaceDocumentIdentity Subject)
{
    const std::int32_t Carrying = LocateLeafCarrying(Space, Subject);

    if (Carrying >= 0)
    {
        WorkspacePartition<WorkspaceDocumentIdentity>& Leaf = Space.Partitions[static_cast<std::size_t>(Carrying)];

        for (std::size_t Ordinal = 0u; Ordinal < Leaf.Occupants.size(); ++Ordinal)
        {
            if (Leaf.Occupants[Ordinal] == Subject)
            {
                Leaf.Occupants.erase(Leaf.Occupants.begin() + static_cast<std::ptrdiff_t>(Ordinal));
                break;
            }
        }

        if (Leaf.Occupants.empty())
        {
            Leaf.ActiveOccupant = {};
            return Carrying;
        }

        if (Leaf.ActiveOccupant == Subject)
            Leaf.ActiveOccupant = Leaf.Occupants.back();

        return -1;
    }

    for (WorkspaceFloatingWindow& Window : Space.Floating)
    {
        for (std::size_t Ordinal = 0u; Ordinal < Window.Documents.size(); ++Ordinal)
        {
            if (Window.Documents[Ordinal] != Subject)
                continue;

            Window.Documents.erase(Window.Documents.begin() + static_cast<std::ptrdiff_t>(Ordinal));

            if (Window.ActiveDocument == Subject)
            {
                Window.ActiveDocument = Window.Documents.empty() ? WorkspaceDocumentIdentity{}
                                                                 : Window.Documents.back();
            }
            break;
        }
    }

    // 📝 A window with nothing left in it is not a window. It is removed here rather than presented as an empty
    //    rectangle the artist has to close by hand.
    for (std::size_t Ordinal = Space.Floating.size(); Ordinal > 0u; --Ordinal)
    {
        if (Space.Floating[Ordinal - 1u].Documents.empty())
            Space.Floating.erase(Space.Floating.begin() + static_cast<std::ptrdiff_t>(Ordinal - 1u));
    }

    return -1;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE CATALOGUE
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> DeclareCatalogue(WorkspaceSpace&                       Space,
                               const WorkspaceDocumentSpecification* Offering,
                               std::uint32_t                         Count,
                               std::uint32_t                         DefaultOrdinal)
{
    if (Offering == nullptr || Count == 0u)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the catalogue offers nothing" });

    if (DefaultOrdinal >= Count)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "the default ordinal lies outside the catalogue" });
    }

    Space.Catalogue.assign(Offering, Offering + Count);
    Space.DefaultCatalogueOrdinal = DefaultOrdinal;

    return Outcome<bool>::Deliver(true);
}

Outcome<WorkspaceDocumentIdentity> ConstructWorkspaceSpace(WorkspaceSpace& Space)
{
    if (Space.Catalogue.empty())
    {
        return Outcome<WorkspaceDocumentIdentity>::Refuse(
            { RefusalReason::ContentUnsupported, "no catalogue has been declared" });
    }

    Space.Documents.clear();
    Space.Partitions.clear();
    Space.Floating.clear();
    Space.RootLink      = -1;
    Space.MintedWindows = 0u;

    return DeclareDocument(Space, Space.DefaultCatalogueOrdinal, -1);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   MINTING AND WITHDRAWAL
//------------------------------------------------------------------------------------------------------------------------

Outcome<WorkspaceDocumentIdentity> DeclareDocument(WorkspaceSpace& Space,
                                                   std::uint32_t   CatalogueOrdinal,
                                                   std::int32_t    TargetLeaf)
{
    if (CatalogueOrdinal >= Space.Catalogue.size())
    {
        return Outcome<WorkspaceDocumentIdentity>::Refuse(
            { RefusalReason::ContentUnsupported, "the ordinal lies outside the catalogue" });
    }

    std::int32_t Landing = TargetLeaf;

    if (Landing < 0)
    {
        if (Space.RootLink >= 0)
        {
            Landing = Space.RootLink;
        }
        else
        {
            const Outcome<std::int32_t> Claimed =
                Partition(Space.Partitions, Space.RootLink, -1, WorkspacePartitionAxis::Row, 0.5f, false);

            if (!Claimed.ContentPresent)
                return Outcome<WorkspaceDocumentIdentity>::Refuse(Claimed.Declined);

            Landing = Claimed.Resolve();
        }
    }

    if (static_cast<std::size_t>(Landing) >= Space.Partitions.size()
     || !Space.Partitions[static_cast<std::size_t>(Landing)].SlotOccupied
     || !Space.Partitions[static_cast<std::size_t>(Landing)].LeafDeclared)
    {
        return Outcome<WorkspaceDocumentIdentity>::Refuse(
            { RefusalReason::IdentityStale, "the target is not an occupied leaf" });
    }

    const WorkspaceDocumentSpecification& Offered = Space.Catalogue[CatalogueOrdinal];

    // 📝 A reclaimed slot is reused with its generation advanced, so an identity held across a withdrawal names
    //    the slot it was issued for and never the document that later occupies it.
    std::size_t Claimed = Space.Documents.size();

    for (std::size_t Ordinal = 0u; Ordinal < Space.Documents.size(); ++Ordinal)
    {
        if (!Space.Documents[Ordinal].SlotOccupied)
        {
            Claimed = Ordinal;
            break;
        }
    }

    if (Claimed == Space.Documents.size())
        Space.Documents.push_back(WorkspaceDocument{});

    WorkspaceDocument& Minted = Space.Documents[Claimed];

    const std::uint32_t Generation = Minted.Identity.SlotGeneration + 1u;

    Minted                          = WorkspaceDocument{};
    Minted.Identity.SlotOrdinal     = static_cast<std::uint32_t>(Claimed);
    Minted.Identity.SlotGeneration  = Generation;
    Minted.Discipline               = Offered.Discipline;
    Minted.SlotOccupied             = true;

    CarryTitle(Minted.Title, Offered.NameStem, StemOccurrences(Space, Offered.NameStem) + 1u);

    WorkspacePartition<WorkspaceDocumentIdentity>& Leaf = Space.Partitions[static_cast<std::size_t>(Landing)];

    Leaf.Occupants.push_back(Minted.Identity);
    Leaf.ActiveOccupant = Minted.Identity;

    return Outcome<WorkspaceDocumentIdentity>::Deliver(Minted.Identity);
}

Outcome<bool> WithdrawDocument(WorkspaceSpace& Space, WorkspaceDocumentIdentity Subject)
{
    const std::size_t Standing = Located(Space, Subject);

    if (Standing == Space.Documents.size())
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "no document resolves to that identity" });

    // 📝 The detach reports the leaf it emptied and leaves it standing; a withdrawal has nothing to hold across
    //    the reclaim, so it reclaims immediately.
    const std::int32_t Emptied = Detach(Space, Subject);

    if (Emptied >= 0)
        ReclaimLeaf(Space.Partitions, Space.RootLink, Emptied);

    Space.Documents[Standing].SlotOccupied = false;
    Space.Documents[Standing].PanelBoxes.clear();
    Space.Documents[Standing].PanelPartitions.clear();
    Space.Documents[Standing].PanelRoot = -1;

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

Outcome<const WorkspaceDocument*> ResolveDocument(const WorkspaceSpace& Space, WorkspaceDocumentIdentity Subject)
{
    const std::size_t Standing = Located(Space, Subject);

    if (Standing == Space.Documents.size())
    {
        return Outcome<const WorkspaceDocument*>::Refuse(
            { RefusalReason::IdentityStale, "no document resolves to that identity" });
    }

    return Outcome<const WorkspaceDocument*>::Deliver(&Space.Documents[Standing]);
}

Outcome<WorkspaceDocument*> AmendDocument(WorkspaceSpace& Space, WorkspaceDocumentIdentity Subject)
{
    const std::size_t Standing = Located(Space, Subject);

    if (Standing == Space.Documents.size())
    {
        return Outcome<WorkspaceDocument*>::Refuse(
            { RefusalReason::IdentityStale, "no document resolves to that identity" });
    }

    return Outcome<WorkspaceDocument*>::Deliver(&Space.Documents[Standing]);
}

std::int32_t LocateLeafCarrying(const WorkspaceSpace& Space, WorkspaceDocumentIdentity Subject)
{
    std::int32_t Carrying = -1;

    Traverse(Space.Partitions, Space.RootLink,
             [&Carrying, Subject](std::int32_t Link, const WorkspacePartition<WorkspaceDocumentIdentity>& Standing)
             {
                 if (!Standing.LeafDeclared)
                     return;

                 for (const WorkspaceDocumentIdentity& Occupant : Standing.Occupants)
                 {
                     if (Occupant == Subject)
                         Carrying = Link;
                 }
             });

    return Carrying;
}

std::uint32_t LocateWindowCarrying(const WorkspaceSpace& Space, WorkspaceDocumentIdentity Subject)
{
    for (const WorkspaceFloatingWindow& Window : Space.Floating)
    {
        for (const WorkspaceDocumentIdentity& Occupant : Window.Documents)
        {
            if (Occupant == Subject)
                return Window.Identifier;
        }
    }

    return 0u;
}

Outcome<WorkspaceFloatingWindow*> AmendFloatingWindow(WorkspaceSpace& Space, std::uint32_t WindowKey)
{
    for (WorkspaceFloatingWindow& Window : Space.Floating)
    {
        if (Window.Identifier == WindowKey && WindowKey != 0u)
            return Outcome<WorkspaceFloatingWindow*>::Deliver(&Window);
    }

    return Outcome<WorkspaceFloatingWindow*>::Refuse(
        { RefusalReason::IdentityStale, "no floating window carries that key" });
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  TEARING AND DOCKING
//------------------------------------------------------------------------------------------------------------------------

Outcome<std::uint32_t> TearDocument(WorkspaceSpace&           Space,
                                    WorkspaceDocumentIdentity Subject,
                                    float                     PositionX,
                                    float                     PositionY)
{
    if (Located(Space, Subject) == Space.Documents.size())
    {
        return Outcome<std::uint32_t>::Refuse(
            { RefusalReason::IdentityStale, "no document resolves to that identity" });
    }

    const std::int32_t Emptied = Detach(Space, Subject);

    ++Space.MintedWindows;

    WorkspaceFloatingWindow Minted;

    Minted.Identifier     = Space.MintedWindows;
    Minted.PositionX      = PositionX;
    Minted.PositionY      = PositionY;
    Minted.ActiveDocument = Subject;
    Minted.Documents.push_back(Subject);

    Space.Floating.push_back(Minted);

    if (Emptied >= 0)
        ReclaimLeaf(Space.Partitions, Space.RootLink, Emptied);

    return Outcome<std::uint32_t>::Deliver(Minted.Identifier);
}

Outcome<bool> DockDocument(WorkspaceSpace&           Space,
                           WorkspaceDocumentIdentity Subject,
                           std::int32_t              TargetLeaf,
                           WorkspaceDropZone         Zone)
{
    if (Located(Space, Subject) == Space.Documents.size())
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "no document resolves to that identity" });

    if (Zone == WorkspaceDropZone::None)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the landing names no zone" });

    if (TargetLeaf < 0 || static_cast<std::size_t>(TargetLeaf) >= Space.Partitions.size()
     || !Space.Partitions[static_cast<std::size_t>(TargetLeaf)].SlotOccupied
     || !Space.Partitions[static_cast<std::size_t>(TargetLeaf)].LeafDeclared)
    {
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "the target is not an occupied leaf" });
    }

    // 📝 A document already alone in the target leaf has nowhere to go. Divided against itself it would produce
    //    an empty half that the reclaim below immediately undoes, and the artist sees a flicker for nothing.
    if (LocateLeafCarrying(Space, Subject) == TargetLeaf
     && Space.Partitions[static_cast<std::size_t>(TargetLeaf)].Occupants.size() == 1u)
    {
        return Outcome<bool>::Deliver(true);
    }

    std::int32_t Landing = TargetLeaf;

    if (Zone != WorkspaceDropZone::Strip && Zone != WorkspaceDropZone::Centre)
    {
        const bool Vertical      = Zone == WorkspaceDropZone::Top  || Zone == WorkspaceDropZone::Bottom;
        const bool ArrivingFirst = Zone == WorkspaceDropZone::Left || Zone == WorkspaceDropZone::Top;

        const Outcome<std::int32_t> Divided =
            Partition(Space.Partitions, Space.RootLink, TargetLeaf,
                      Vertical ? WorkspacePartitionAxis::Column : WorkspacePartitionAxis::Row,
                      0.5f, ArrivingFirst);

        if (!Divided.ContentPresent)
            return Outcome<bool>::Refuse(Divided.Declined);

        Landing = Divided.Resolve();
    }

    // 🔴 Placed before it is detached. The division above may have moved the subject's own former leaf, so the
    //    detach is resolved fresh against the tree as it now stands rather than against an index taken earlier.
    std::int32_t Emptied = -1;

    {
        // 📝 The detach must not see the copy about to be placed, so the placement happens after it and the
        //    landing index is the one the division just handed back, which a detach never invalidates.
        Emptied = Detach(Space, Subject);

        WorkspacePartition<WorkspaceDocumentIdentity>& Placed = Space.Partitions[static_cast<std::size_t>(Landing)];

        Placed.Occupants.push_back(Subject);
        Placed.ActiveOccupant = Subject;
    }

    if (Emptied >= 0 && Emptied != Landing)
        ReclaimLeaf(Space.Partitions, Space.RootLink, Emptied);

    return Outcome<bool>::Deliver(true);
}

Outcome<bool> StackDocumentInWindow(WorkspaceSpace&           Space,
                                    WorkspaceDocumentIdentity Subject,
                                    std::uint32_t             WindowKey)
{
    if (Located(Space, Subject) == Space.Documents.size())
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "no document resolves to that identity" });

    if (LocateWindowCarrying(Space, Subject) == WindowKey && WindowKey != 0u)
        return Outcome<bool>::Deliver(true);

    // 🔴 The key is admitted **before** the detach and resolved again after it. Detaching first and refusing on a
    //    stale key leaves the document carried by nothing at all — not a leaf, not a window — which presents as a
    //    tab that vanished rather than as the refusal it is. The second resolution is not redundant: the detach may
    //    have emptied and erased a window, and a pointer taken across that is a pointer into a moved extent.
    if (!AmendFloatingWindow(Space, WindowKey).ContentPresent)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::IdentityStale, "no floating window carries that key" });
    }

    const std::int32_t Emptied = Detach(Space, Subject);

    const Outcome<WorkspaceFloatingWindow*> Standing = AmendFloatingWindow(Space, WindowKey);

    if (!Standing.ContentPresent)
        return Outcome<bool>::Refuse(Standing.Declined);

    Standing.Resolve()->Documents.push_back(Subject);
    Standing.Resolve()->ActiveDocument = Subject;

    if (Emptied >= 0)
        ReclaimLeaf(Space.Partitions, Space.RootLink, Emptied);

    return Outcome<bool>::Deliver(true);
}

Outcome<bool> ReorderOccupant(WorkspaceSpace&           Space,
                              WorkspaceDocumentIdentity Subject,
                              std::int32_t              CarryingLeaf,
                              std::uint32_t             CarryingWindow,
                              std::uint32_t             Position)
{
    std::vector<WorkspaceDocumentIdentity>* Ordered = nullptr;

    if (CarryingLeaf >= 0 && static_cast<std::size_t>(CarryingLeaf) < Space.Partitions.size()
     && Space.Partitions[static_cast<std::size_t>(CarryingLeaf)].SlotOccupied
     && Space.Partitions[static_cast<std::size_t>(CarryingLeaf)].LeafDeclared)
    {
        Ordered = &Space.Partitions[static_cast<std::size_t>(CarryingLeaf)].Occupants;
    }
    else if (CarryingWindow != 0u)
    {
        const Outcome<WorkspaceFloatingWindow*> Standing = AmendFloatingWindow(Space, CarryingWindow);

        if (Standing.ContentPresent)
            Ordered = &Standing.Resolve()->Documents;
    }

    if (Ordered == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "nothing carries that document there" });

    std::size_t Standing = Ordered->size();

    for (std::size_t Ordinal = 0u; Ordinal < Ordered->size(); ++Ordinal)
    {
        if ((*Ordered)[Ordinal] == Subject)
            Standing = Ordinal;
    }

    if (Standing == Ordered->size())
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "the document is not among those occupants" });

    Ordered->erase(Ordered->begin() + static_cast<std::ptrdiff_t>(Standing));

    const std::size_t Bounded = Position < Ordered->size() ? Position : Ordered->size();

    Ordered->insert(Ordered->begin() + static_cast<std::ptrdiff_t>(Bounded), Subject);

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE PANEL BOXES
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📝 Bounded once here so the floor and the ceiling are spelled in one place. A band dragged past either bound is
//    held at it rather than refused: a drag is a continuous gesture and refusing part way through one reads as a
//    band that stopped following the pointer for no reason the artist can see.
float BoundedFraction(float Asked, float Floor, float Ceiling)
{
    return Asked < Floor ? Floor : (Asked > Ceiling ? Ceiling : Asked);
}

std::size_t LocatedPanel(const WorkspaceDocument& Standing, WorkspacePanelIdentity Subject)
{
    if (!Subject.IdentityDeclared())
        return Standing.PanelBoxes.size();

    for (std::size_t Ordinal = 0u; Ordinal < Standing.PanelBoxes.size(); ++Ordinal)
    {
        const WorkspacePanelBox& Box = Standing.PanelBoxes[Ordinal];

        if (Box.SlotOccupied && Box.Identity == Subject)
            return Ordinal;
    }

    return Standing.PanelBoxes.size();
}

// 📝 How many occupied boxes already name one side, which is what a newly docked panel's share is resolved against.
std::uint32_t SideOccurrences(const WorkspaceDocument& Standing, WorkspacePanelSide Side)
{
    std::uint32_t Counted = 0u;

    for (const WorkspacePanelBox& Box : Standing.PanelBoxes)
    {
        if (Box.SlotOccupied && Box.Side == Side)
            ++Counted;
    }

    return Counted;
}

}   // namespace

Outcome<WorkspacePanelIdentity> DeclarePanelBox(WorkspaceDocument& Standing,
                                                const char*        DeclaredIdentifier,
                                                const char*        Title)
{
    if (DeclaredIdentifier == nullptr || DeclaredIdentifier[0] == '\0')
    {
        return Outcome<WorkspacePanelIdentity>::Refuse(
            { RefusalReason::ContentUnsupported, "a panel box names no ledger identifier" });
    }

    // 📝 A reclaimed record is reused with its generation advanced, exactly as a document slot is, so an identity a
    //    drag is holding across a withdrawal names the record it was issued for and never its successor.
    std::size_t Claimed = Standing.PanelBoxes.size();

    for (std::size_t Ordinal = 0u; Ordinal < Standing.PanelBoxes.size(); ++Ordinal)
    {
        if (!Standing.PanelBoxes[Ordinal].SlotOccupied)
        {
            Claimed = Ordinal;
            break;
        }
    }

    if (Claimed == Standing.PanelBoxes.size())
    {
        if (Standing.PanelBoxes.size() >= PanelBoxCapacity)
        {
            return Outcome<WorkspacePanelIdentity>::Refuse(
                { RefusalReason::ExtentExhausted, "the body already holds its declared panel capacity" });
        }

        Standing.PanelBoxes.push_back(WorkspacePanelBox{});
    }

    WorkspacePanelBox& Minted = Standing.PanelBoxes[Claimed];

    const std::uint32_t Generation = Minted.Identity.SlotGeneration + 1u;

    ++Standing.MintedPanels;

    Minted                            = WorkspacePanelBox{};
    Minted.Identity.SlotOrdinal       = static_cast<std::uint32_t>(Claimed);
    Minted.Identity.SlotGeneration    = Generation;
    Minted.DeclaredIdentifier         = DeclaredIdentifier;
    Minted.SlotOccupied               = true;

    // 📝 The offsets stagger with the mint count so two boxes declared in a row do not arrive exactly on top of one
    //    another, which is the arrangement where the artist cannot tell there are two.
    const float Stagger = static_cast<float>(Standing.MintedPanels % 6u) * 18.0f;

    Minted.OffsetX = 24.0f + Stagger;
    Minted.OffsetY = 24.0f + Stagger;

    std::snprintf(Minted.Title, WorkspaceTitleExtent, "%s",
                  Title != nullptr && Title[0] != '\0' ? Title : DeclaredIdentifier);

    return Outcome<WorkspacePanelIdentity>::Deliver(Minted.Identity);
}

Outcome<bool> WithdrawPanelBox(WorkspaceDocument& Standing, WorkspacePanelIdentity Subject)
{
    const std::size_t Resting = LocatedPanel(Standing, Subject);

    if (Resting == Standing.PanelBoxes.size())
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "no panel box resolves to that identity" });

    // 📝 The record is emptied rather than erased. Erasing would move every later box's position in the list, and
    //    the placement the tick already resolved names positions in exactly that list.
    Standing.PanelBoxes[Resting].SlotOccupied       = false;
    Standing.PanelBoxes[Resting].DeclaredIdentifier = nullptr;

    return Outcome<bool>::Deliver(true);
}

Outcome<WorkspacePanelBox*> AmendPanelBox(WorkspaceDocument& Standing, WorkspacePanelIdentity Subject)
{
    const std::size_t Resting = LocatedPanel(Standing, Subject);

    if (Resting == Standing.PanelBoxes.size())
    {
        return Outcome<WorkspacePanelBox*>::Refuse(
            { RefusalReason::IdentityStale, "no panel box resolves to that identity" });
    }

    return Outcome<WorkspacePanelBox*>::Deliver(&Standing.PanelBoxes[Resting]);
}

Outcome<bool> RaisePanelBox(WorkspaceDocument& Standing, WorkspacePanelIdentity Subject)
{
    const std::size_t Resting = LocatedPanel(Standing, Subject);

    if (Resting == Standing.PanelBoxes.size())
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "no panel box resolves to that identity" });

    // 📝 The counter only ever climbs, so the raised box is in front of every box raised before it and of every box
    //    never raised at all. Renumbering the whole body instead would need the boxes sorted, and the sort would
    //    move the records whose positions their own identities name.
    ++Standing.RaisedPanels;

    Standing.PanelBoxes[Resting].RaiseOrdinal = Standing.RaisedPanels;

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE BODY PLACEMENT
//------------------------------------------------------------------------------------------------------------------------

WorkspaceBodyPlacement ResolveBodyPlacement(const WorkspaceDocument& Standing,
                                            WorkspaceRectangle       Body,
                                            float                    GutterThickness)
{
    WorkspaceBodyPlacement Placed;

    Placed.CentreArea = Body;

    if (Body.Width <= 1.0f || Body.Height <= 1.0f)
        return Placed;

    // 📝 🔴 The four sides are resolved in a fixed order — columns before rows — so a corner belongs to exactly one
    //    band. Resolving the rows first would hand the top-left to both the left column and the top row, and the
    //    panel that lost would paint underneath the other with no way for the artist to reach it.
    const WorkspacePanelSide Ordered[4] =
    {
        WorkspacePanelSide::Left,
        WorkspacePanelSide::Right,
        WorkspacePanelSide::Top,
        WorkspacePanelSide::Bottom
    };

    for (const WorkspacePanelSide Side : Ordered)
    {
        const std::uint32_t Sharing = SideOccurrences(Standing, Side);

        if (Sharing == 0u)
            continue;

        const bool  Across = Side == WorkspacePanelSide::Left || Side == WorkspacePanelSide::Right;
        const float Span   = Across ? Placed.CentreArea.Width : Placed.CentreArea.Height;

        // 📝 The band takes the depth of the first panel declared on the side. Every panel there carries its own
        //    copy, and a depth drag writes all of them, so the record stays per-panel while the band stays one band.
        float Depth = 0.28f;

        for (const WorkspacePanelBox& Box : Standing.PanelBoxes)
        {
            if (Box.SlotOccupied && Box.Side == Side)
            {
                Depth = BoundedFraction(Box.DockExtent, PanelDockFloor, PanelDockCeiling);
                break;
            }
        }

        const float BandDepth = Span * Depth;

        WorkspaceRectangle Band = Placed.CentreArea;

        if (Side == WorkspacePanelSide::Left)
        {
            Band.Width = BandDepth;

            Placed.CentreArea.PositionX += BandDepth;
            Placed.CentreArea.Width     -= BandDepth;
        }
        else if (Side == WorkspacePanelSide::Right)
        {
            Band.PositionX = Placed.CentreArea.PositionX + Placed.CentreArea.Width - BandDepth;
            Band.Width     = BandDepth;

            Placed.CentreArea.Width -= BandDepth;
        }
        else if (Side == WorkspacePanelSide::Top)
        {
            Band.Height = BandDepth;

            Placed.CentreArea.PositionY += BandDepth;
            Placed.CentreArea.Height    -= BandDepth;
        }
        else
        {
            Band.PositionY = Placed.CentreArea.PositionY + Placed.CentreArea.Height - BandDepth;
            Band.Height    = BandDepth;

            Placed.CentreArea.Height -= BandDepth;
        }

        // 📝 The shares are normalised against their own sum rather than trusted to reach one. A withdrawal leaves
        //    the survivors' shares summing to less than one, and trusting them would leave a gap in the band that
        //    nothing paints and nothing can be dragged into.
        float Summed = 0.0f;

        for (const WorkspacePanelBox& Box : Standing.PanelBoxes)
        {
            if (Box.SlotOccupied && Box.Side == Side)
                Summed += Box.SlotFraction > 0.02f ? Box.SlotFraction : 0.02f;
        }

        if (!(Summed > 0.0f))
            Summed = 1.0f;

        // 📝 The band is split along its long axis: a column splits vertically, a row horizontally.
        const float SharedSpan = Across ? Band.Height : Band.Width;

        float Travelled = 0.0f;
        std::uint32_t PlacedOnSide = 0u;

        for (std::size_t Ordinal = 0u; Ordinal < Standing.PanelBoxes.size(); ++Ordinal)
        {
            const WorkspacePanelBox& Box = Standing.PanelBoxes[Ordinal];

            if (!Box.SlotOccupied || Box.Side != Side)
                continue;

            if (Placed.DockedCount >= PanelBoxCapacity)
                break;

            const float Share  = (Box.SlotFraction > 0.02f ? Box.SlotFraction : 0.02f) / Summed;
            const float Extent = SharedSpan * Share;

            WorkspacePanelPlacement Resolved;

            Resolved.Identity = Box.Identity;
            Resolved.Area     = Band;

            if (Across)
            {
                Resolved.Area.PositionY = Band.PositionY + Travelled;
                Resolved.Area.Height    = Extent;
            }
            else
            {
                Resolved.Area.PositionX = Band.PositionX + Travelled;
                Resolved.Area.Width     = Extent;
            }

            // -- the depth grip, on the band's inner edge --------------------------------------------------------------
            Resolved.DepthGrip         = Resolved.Area;
            Resolved.DepthGripDeclared = true;

            if (Side == WorkspacePanelSide::Left)
            {
                Resolved.DepthGrip.PositionX = Resolved.Area.PositionX + Resolved.Area.Width - GutterThickness;
                Resolved.DepthGrip.Width     = GutterThickness;
            }
            else if (Side == WorkspacePanelSide::Right)
            {
                Resolved.DepthGrip.Width = GutterThickness;
            }
            else if (Side == WorkspacePanelSide::Top)
            {
                Resolved.DepthGrip.PositionY = Resolved.Area.PositionY + Resolved.Area.Height - GutterThickness;
                Resolved.DepthGrip.Height    = GutterThickness;
            }
            else
            {
                Resolved.DepthGrip.Height = GutterThickness;
            }

            // -- the share grip, only where another panel follows along the side ---------------------------------------
            if (PlacedOnSide + 1u < Sharing)
            {
                Resolved.ShareGripDeclared = true;
                Resolved.ShareGrip         = Resolved.Area;

                if (Across)
                {
                    Resolved.ShareGrip.PositionY = Resolved.Area.PositionY + Resolved.Area.Height - GutterThickness;
                    Resolved.ShareGrip.Height    = GutterThickness;
                }
                else
                {
                    Resolved.ShareGrip.PositionX = Resolved.Area.PositionX + Resolved.Area.Width - GutterThickness;
                    Resolved.ShareGrip.Width     = GutterThickness;
                }
            }

            Placed.Docked[Placed.DockedCount] = Resolved;
            ++Placed.DockedCount;
            ++PlacedOnSide;

            Travelled += Extent;
        }
    }

    // 📝 A panel declared for the centre fills what the bands left rather than reserving a band of its own, so it
    //    is resolved after every band and takes no grip: there is nothing on the other side of it to drag against.
    for (const WorkspacePanelBox& Box : Standing.PanelBoxes)
    {
        if (!Box.SlotOccupied || Box.Side != WorkspacePanelSide::Centre)
            continue;

        if (Placed.DockedCount >= PanelBoxCapacity)
            break;

        WorkspacePanelPlacement Resolved;

        Resolved.Identity = Box.Identity;
        Resolved.Area     = Placed.CentreArea;

        Placed.Docked[Placed.DockedCount] = Resolved;
        ++Placed.DockedCount;
    }

    // -- the floating run, painted last --------------------------------------------------------------------------------
    // 📝 🔴 Inserted in raise order rather than appended in list order. The list's order is fixed for the body's whole
    //    life because a record's position is the ordinal its identity carries, so raise order is the only ordering a
    //    floating box can be given — and without it a box pressed while half under another one paints under it still.
    for (const WorkspacePanelBox& Box : Standing.PanelBoxes)
    {
        if (!Box.SlotOccupied || Box.Side != WorkspacePanelSide::Floating)
            continue;

        if (Placed.OverlaidCount >= PanelBoxCapacity)
            break;

        WorkspacePanelPlacement Resolved;

        Resolved.Identity     = Box.Identity;
        Resolved.RaiseOrdinal = Box.RaiseOrdinal;

        // 📝 The offsets are relative to the body, which is what makes a floating box track its own body when the
        //    desk is re-divided under it rather than staying where the screen was.
        Resolved.Area.PositionX = Body.PositionX + Box.OffsetX;
        Resolved.Area.PositionY = Body.PositionY + Box.OffsetY;
        Resolved.Area.Width     = Box.Width  < PanelMinimumExtent ? PanelMinimumExtent : Box.Width;
        Resolved.Area.Height    = Box.Height < PanelMinimumExtent ? PanelMinimumExtent : Box.Height;

        // 📝 The insertion travels back over every entry raised **after** this one and stops at the first raised at
        //    the same tick or earlier, so two boxes never raised at all keep the order they were declared in.
        std::uint32_t Landing = Placed.OverlaidCount;

        while (Landing > 0u && Placed.Overlaid[Landing - 1u].RaiseOrdinal > Resolved.RaiseOrdinal)
        {
            Placed.Overlaid[Landing] = Placed.Overlaid[Landing - 1u];
            --Landing;
        }

        Placed.Overlaid[Landing] = Resolved;
        ++Placed.OverlaidCount;
    }

    if (Placed.CentreArea.Width  < 0.0f) Placed.CentreArea.Width  = 0.0f;
    if (Placed.CentreArea.Height < 0.0f) Placed.CentreArea.Height = 0.0f;

    return Placed;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE PANEL LANDING
//------------------------------------------------------------------------------------------------------------------------

WorkspacePanelSide ResolvePanelLanding(WorkspaceRectangle  Body,
                                       float               PointerX,
                                       float               PointerY,
                                       WorkspaceRectangle& Preview)
{
    Preview = Body;

    if (Body.Width <= 1.0f || Body.Height <= 1.0f || !RectangleCovers(Body, PointerX, PointerY))
        return WorkspacePanelSide::Floating;

    const float FractionAcross = (PointerX - Body.PositionX) / Body.Width;
    const float FractionDown   = (PointerY - Body.PositionY) / Body.Height;

    WorkspacePanelSide Landed = WorkspacePanelSide::Floating;

    if      (FractionAcross < DockBandFraction)        Landed = WorkspacePanelSide::Left;
    else if (FractionAcross > 1.0f - DockBandFraction) Landed = WorkspacePanelSide::Right;
    else if (FractionDown   < DockBandFraction)        Landed = WorkspacePanelSide::Top;
    else if (FractionDown   > 1.0f - DockBandFraction) Landed = WorkspacePanelSide::Bottom;

    // 🔴 The middle resolves to Floating and never to Centre. A panel docked over the centre covers the document the
    //    body exists to present, so Centre is a side a workspace declares deliberately and never one a release mints.

    const float Depth = 0.28f;

    switch (Landed)
    {
        case WorkspacePanelSide::Left:
            Preview.Width = Body.Width * Depth;
            break;

        case WorkspacePanelSide::Right:
            Preview.PositionX = Body.PositionX + Body.Width * (1.0f - Depth);
            Preview.Width     = Body.Width * Depth;
            break;

        case WorkspacePanelSide::Top:
            Preview.Height = Body.Height * Depth;
            break;

        case WorkspacePanelSide::Bottom:
            Preview.PositionY = Body.PositionY + Body.Height * (1.0f - Depth);
            Preview.Height    = Body.Height * Depth;
            break;

        default:
            break;
    }

    return Landed;
}

Outcome<bool> DockPanelBox(WorkspaceDocument&     Standing,
                           WorkspacePanelIdentity Subject,
                           WorkspacePanelSide     Landed,
                           WorkspaceRectangle     Body,
                           float                  ReleaseX,
                           float                  ReleaseY)
{
    const std::size_t Resting = LocatedPanel(Standing, Subject);

    if (Resting == Standing.PanelBoxes.size())
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "no panel box resolves to that identity" });

    // 📝 The count is taken **before** the side is written, so a box already on the side it is being dropped onto
    //    does not count itself and end up with a share of one over two panels.
    const std::uint32_t Sharing = SideOccurrences(Standing, Landed);

    WorkspacePanelBox& Docked = Standing.PanelBoxes[Resting];

    if (Landed == WorkspacePanelSide::Floating)
    {
        // 📝 A box returned to floating arrives where it was released rather than where it last floated, which is
        //    the one position the artist has just chosen for it.
        Docked.Side    = WorkspacePanelSide::Floating;
        Docked.OffsetX = ReleaseX - Body.PositionX;
        Docked.OffsetY = ReleaseY - Body.PositionY;

        return Outcome<bool>::Deliver(true);
    }

    // 📝 The arriving panel takes the band's current depth so the band does not jump when a second panel joins it.
    float Depth = Docked.DockExtent;

    for (const WorkspacePanelBox& Box : Standing.PanelBoxes)
    {
        if (Box.SlotOccupied && Box.Side == Landed && Box.Identity != Subject)
        {
            Depth = Box.DockExtent;
            break;
        }
    }

    Docked.Side         = Landed;
    Docked.DockExtent   = BoundedFraction(Depth, PanelDockFloor, PanelDockCeiling);

    // 📝 An equal share of the side it joined. The placement normalises against the sum, so an equal share here is
    //    an equal band whatever the panels already there were carrying.
    Docked.SlotFraction = 1.0f / static_cast<float>(Sharing + 1u);

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    DROP RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

namespace
{

WorkspaceRectangle PreviewFor(const WorkspaceRectangle& Body, WorkspaceDropZone Zone)
{
    WorkspaceRectangle Preview = Body;

    switch (Zone)
    {
        case WorkspaceDropZone::Left:   Preview.Width  *= 0.5f;                                             break;
        case WorkspaceDropZone::Right:  Preview.PositionX += Body.Width  * 0.5f; Preview.Width  *= 0.5f;    break;
        case WorkspaceDropZone::Top:    Preview.Height *= 0.5f;                                             break;
        case WorkspaceDropZone::Bottom: Preview.PositionY += Body.Height * 0.5f; Preview.Height *= 0.5f;    break;
        default:                                                                                            break;
    }

    return Preview;
}

}   // namespace

WorkspaceDropLanding ResolveDropLanding(const WorkspaceSpace& Space,
                                        WorkspaceDragOrigin   Origin,
                                        float                 PointerX,
                                        float                 PointerY,
                                        float                 StripHeight,
                                        std::uint32_t         IgnoredWindow)
{
    WorkspaceDropLanding Landing;

    // 📝 The windows are tested topmost first, which is the reverse of the list. The one being dragged is skipped
    //    outright: a window that resolved as its own target would stack a document onto the window it lives in.
    for (std::size_t Ordinal = Space.Floating.size(); Ordinal > 0u; --Ordinal)
    {
        const WorkspaceFloatingWindow& Window = Space.Floating[Ordinal - 1u];

        if (Window.Identifier == IgnoredWindow)
            continue;

        WorkspaceRectangle Strip;

        Strip.PositionX = Window.PositionX;
        Strip.PositionY = Window.PositionY;
        Strip.Width     = Window.Width;
        Strip.Height    = StripHeight;

        if (RectangleCovers(Strip, PointerX, PointerY))
        {
            Landing.Zone        = WorkspaceDropZone::Strip;
            Landing.Window      = Window.Identifier;
            Landing.PreviewArea = Strip;

            return Landing;
        }
    }

    const std::int32_t Covering = LocateLeafCovering(Space.Partitions, Space.RootLink, PointerX, PointerY);

    if (Covering < 0)
        return Landing;

    const WorkspaceRectangle Area = Space.Partitions[static_cast<std::size_t>(Covering)].Area;

    WorkspaceRectangle Strip = Area;

    Strip.Height = StripHeight;

    if (RectangleCovers(Strip, PointerX, PointerY))
    {
        Landing.Zone        = WorkspaceDropZone::Strip;
        Landing.Link        = Covering;
        Landing.PreviewArea = Strip;

        return Landing;
    }

    WorkspaceRectangle Body = Area;

    Body.PositionY += StripHeight;
    Body.Height     = Area.Height - StripHeight;

    if (Body.Width <= 1.0f || Body.Height <= 1.0f)
        return Landing;

    const float FractionAcross = (PointerX - Body.PositionX) / Body.Width;
    const float FractionDown   = (PointerY - Body.PositionY) / Body.Height;

    if      (FractionAcross < DockBandFraction)        Landing.Zone = WorkspaceDropZone::Left;
    else if (FractionAcross > 1.0f - DockBandFraction) Landing.Zone = WorkspaceDropZone::Right;
    else if (FractionDown   < DockBandFraction)        Landing.Zone = WorkspaceDropZone::Top;
    else if (FractionDown   > 1.0f - DockBandFraction) Landing.Zone = WorkspaceDropZone::Bottom;
    else
    {
        // 🔴 The origin decides what the middle means. A torn **tab** released over the middle of a leaf stays
        //    floating, so a careless release never rearranges the desk; a moved **panel** always docks, because
        //    a panel released into open air has nowhere to be.
        Landing.Zone = Origin == WorkspaceDragOrigin::Panel ? WorkspaceDropZone::Centre : WorkspaceDropZone::None;
    }

    if (Landing.Zone != WorkspaceDropZone::None)
    {
        Landing.Link        = Covering;
        Landing.PreviewArea = PreviewFor(Body, Landing.Zone);
    }

    return Landing;
}

void ResolveSpaceLayout(WorkspaceSpace& Space, WorkspaceRectangle DeskArea, float GutterThickness)
{
    // 📝 Every partition is marked unresolved first. A leaf that was reclaimed this tick then cannot be hit by a
    //    pointer test against a rectangle it kept from the tick before it left the desk.
    for (WorkspacePartition<WorkspaceDocumentIdentity>& Standing : Space.Partitions)
        Standing.LayoutResolved = false;

    ResolveLayout(Space.Partitions, Space.RootLink, DeskArea, GutterThickness);
}

}   // namespace Slate
