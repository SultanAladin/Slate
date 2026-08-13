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
