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

        // 📝 The occupant that becomes active is the one now last in order — the tab to the left of the closed
        //    one under the presentation `2c` gives the strip. An empty leaf is reclaimed rather than left blank.
        if (Leaf.Occupants.empty())
        {
            Leaf.ActiveOccupant = {};
            ReclaimLeaf(Space.Partitions, Space.RootLink, Carrying);
        }
        else if (Leaf.ActiveOccupant == Subject)
        {
            Leaf.ActiveOccupant = Leaf.Occupants.back();
        }
    }
    else
    {
        for (WorkspaceFloatingWindow& Window : Space.Floating)
        {
            for (std::size_t Ordinal = 0u; Ordinal < Window.Documents.size(); ++Ordinal)
            {
                if (Window.Documents[Ordinal] == Subject)
                {
                    Window.Documents.erase(Window.Documents.begin() + static_cast<std::ptrdiff_t>(Ordinal));

                    if (Window.ActiveDocument == Subject)
                        Window.ActiveDocument = Window.Documents.empty() ? WorkspaceDocumentIdentity{}
                                                                         : Window.Documents.back();
                    break;
                }
            }
        }

        for (std::size_t Ordinal = Space.Floating.size(); Ordinal > 0u; --Ordinal)
        {
            if (Space.Floating[Ordinal - 1u].Documents.empty())
                Space.Floating.erase(Space.Floating.begin() + static_cast<std::ptrdiff_t>(Ordinal - 1u));
        }
    }

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

void ResolveSpaceLayout(WorkspaceSpace& Space, WorkspaceRectangle DeskArea, float GutterThickness)
{
    // 📝 Every partition is marked unresolved first. A leaf that was reclaimed this tick then cannot be hit by a
    //    pointer test against a rectangle it kept from the tick before it left the desk.
    for (WorkspacePartition<WorkspaceDocumentIdentity>& Standing : Space.Partitions)
        Standing.LayoutResolved = false;

    ResolveLayout(Space.Partitions, Space.RootLink, DeskArea, GutterThickness);
}

}   // namespace Slate
