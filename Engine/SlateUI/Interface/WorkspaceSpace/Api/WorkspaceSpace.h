//============================================================================================================================================
//                                                            WORKSPACESPACE.H
//============================================================================================================================================
// 🧩 The recursive partition of the desk — leaves holding ordered occupants, partitions holding two links and a draggable gutter.

#pragma once

#include "Contract/IdentityContract.h"
#include "Contract/OutcomeContract.h"
#include "Contract/PrecisionContract.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"

#include <cstdint>
#include <vector>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE TWO OCCUPANT SUBJECTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Declared here rather than in `Contract/IdentityContract.h` because nothing beneath `SlateUI` addresses either
//    of them. `00` §2 keeps an identity beside the one unit that issues it until a second unit needs it.
struct WorkspaceDocumentSubject {};
struct WorkspacePanelSubject    {};

using WorkspaceDocumentIdentity = Identity<WorkspaceDocumentSubject>;   // [-] - one document, wherever it lives
using WorkspacePanelIdentity    = Identity<WorkspacePanelSubject>;      // [-] - one panel box inside one body

inline constexpr std::uint32_t WorkspaceTitleExtent = 64u;   // [-] - characters a title accepts, terminator included

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE DISCIPLINES
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What a document is for — what the catalogue offers and what an unfilled body prints.
/// note  ⚠️ Spelled `Discipline` and not `Subject`. `Module.toml` already uses `subject` to mean a source folder
///        that becomes a link target, and Phase 5 mints three executables from exactly that meaning; two unrelated
///        meanings on one word inside one build is the collision this avoids.
/// note  📝 The roster is complete before the workspaces are. A discipline with no workspace behind it costs one
///        enumerator and lets `WorkspaceSpace` never learn a concrete workspace's name.
/// tag   contract
enum class WorkspaceDiscipline : std::uint32_t
{
    Empty           = 0u,   // [-] - a blank document
    Painting        = 1u,   // [-] - surface painting over a resolved domain
    Modelling       = 2u,   // [-] - boundary authoring
    Draughting      = 3u,   // [-] - two-dimensional constrained sketching
    Simulation      = 4u,   // [-] - dynamics authoring
    UV              = 5u,   // [-] - parametric domain authoring
    Baking          = 6u,   // [-] - high-to-low transfer
    DisciplineCount = 7u    // [-] - the closed count, never a discipline
};

/// 🧩 One document the catalogue offers, supplied by the application and never spelled by this component.
/// note  🔴 `32` §5: a standalone host hands one of these and the editor hands all of them. Nothing here names a
///        concrete workspace, which is the whole reason one tree delivers separate editors and one editor both.
/// tag   nonallocating, nonthrowing
struct WorkspaceDocumentSpecification
{
    const char*          Label      = "New Document";              // [-] - the catalogue row's caption
    const char*          NameStem   = "Document";                  // [-] - minted titles read "<NameStem> N"
    WorkspaceDiscipline  Discipline = WorkspaceDiscipline::Empty;   // [-] - what the minted document is for
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     GEOMETRY AND AXIS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One rectangle in interface pixels — what a leaf, a gutter and a drop preview all resolve to.
/// tag   nonallocating, nonthrowing
struct WorkspaceRectangle
{
    float  PositionX = 0.0f;   // [px] - top-left horizontal
    float  PositionY = 0.0f;   // [px] - top-left vertical
    float  Width     = 0.0f;   // [px] - horizontal extent
    float  Height    = 0.0f;   // [px] - vertical extent
};

/// 🧩 Whether a point lies inside a rectangle, edges included.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
constexpr bool RectangleCovers(const WorkspaceRectangle& Area, float PositionX, float PositionY)
{
    return PositionX >= Area.PositionX && PositionX <= Area.PositionX + Area.Width
        && PositionY >= Area.PositionY && PositionY <= Area.PositionY + Area.Height;
}

/// 🧩 Which way a partition divides the area it was handed.
/// note  Row places the two links side by side with a vertical gutter between them; Column stacks them with a
///        horizontal gutter. The gutter is part of the partition and not of either link.
/// tag   contract
enum class WorkspacePartitionAxis : std::uint32_t
{
    Row    = 0u,   // [-] - links side by side, vertical gutter
    Column = 1u    // [-] - links stacked, horizontal gutter
};

// 📝 A leaf narrower or shorter than this cannot present a tab strip and a body at once, so a split that would
//    produce one is refused rather than producing a leaf the artist cannot aim at.
inline constexpr float MinimumLeafExtent = 96.0f;   // [px] - the smallest leaf a split may leave behind

//------------------------------------------------------------------------------------------------------------------------
//                                                      ONE PARTITION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One node of the recursive partition: either a leaf carrying ordered occupants, or a division into two links.
/// note  🔴 Addressed by index into a pool carrying an occupancy bit, never by pointer. A partition that split
///        while a reference into the pool was held would dangle, and the defect presents as a leaf drawn at
///        another leaf's rectangle rather than as a crash.
/// note  ⚠️ `FirstLink` and `SecondLink` are the two halves of the division. They are not kinship terms and the
///        kinship spellings are banned outright — a division has two halves, not a family.
/// note  📝 Templated on what a leaf holds so that the document partition and the in-body panel partition are one
///        implementation. Frontier carries two copies of this and two copies of the layout recursion beneath it,
///        which is two places a gutter arithmetic amendment has to land and one of them will be missed.
/// tag   owning
template <typename OccupantIdentity>
struct WorkspacePartition
{
    bool                             LeafDeclared   = true;    // [-]  - true selects the occupant fields below
    bool                             SlotOccupied   = true;    // [-]  - false marks a reclaimed pool slot

    std::vector<OccupantIdentity>    Occupants      = {};      // [-]  - leaf only: ordered, last is topmost
    OccupantIdentity                 ActiveOccupant = {};      // [-]  - leaf only: the one presented in the body

    WorkspacePartitionAxis           Axis        = WorkspacePartitionAxis::Row;   // [-]  - division only
    float                            Ratio       = 0.5f;       // [-]  - division only: FirstLink's share, bounded
    std::int32_t                     FirstLink   = -1;         // [-]  - division only: pool index, -1 for absent
    std::int32_t                     SecondLink  = -1;         // [-]  - division only: pool index, -1 for absent

    WorkspaceRectangle               Area           = {};      // [px] - resolved every tick before any input
    WorkspaceRectangle               Gutter         = {};      // [px] - division only: the draggable band
    bool                             LayoutResolved = false;   // [-]  - Area and Gutter are current this tick
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PARTITION TRAVERSAL
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Visits every occupied partition beneath one link, first half before second, division before its halves.
/// in    Partitions  [-]  the pool
/// in    Origin      [-]  where to begin; a negative index visits nothing
/// in    Visiting    [-]  invoked as (std::int32_t Link, const WorkspacePartition&)
/// note  Order matters to the presenter and not to the layout: a division is visited before its halves so a
///        painter can fill the area beneath before the leaves paint over it.
/// cost  🚩
/// tag   api, nonallocating, nonthrowing
template <typename OccupantIdentity, typename Visitor>
void Traverse(const std::vector<WorkspacePartition<OccupantIdentity>>& Partitions,
              std::int32_t                                            Origin,
              Visitor&&                                               Visiting)
{
    if (Origin < 0 || static_cast<std::size_t>(Origin) >= Partitions.size())
        return;

    const WorkspacePartition<OccupantIdentity>& Standing = Partitions[static_cast<std::size_t>(Origin)];

    if (!Standing.SlotOccupied)
        return;

    Visiting(Origin, Standing);

    if (Standing.LeafDeclared)
        return;

    Traverse(Partitions, Standing.FirstLink,  Visiting);
    Traverse(Partitions, Standing.SecondLink, Visiting);
}

/// 🧩 Resolves the rectangle of every partition beneath one link, and the gutter of every division.
/// in    Partitions       [-]   the pool, amended in place
/// in    Origin           [-]   where to begin
/// in    Area             [px]  the rectangle the origin occupies
/// in    GutterThickness  [px]  the draggable band between two halves, taken from the theme by the caller
/// post  every visited partition carries a current Area, and LayoutResolved holds
/// note  🔴 Run once a tick **before** any input is resolved. A division dragged against last tick's rectangles
///        moves the gutter to where the pointer was rather than to where it is, and the drag reads as sticky.
/// note  ⚠️ The gutter is subtracted from the divided span before the ratio is applied, so a chain of divisions
///        does not accumulate a drift of half a gutter per level.
/// cost  🚩
/// tag   api, nonallocating, nonthrowing
template <typename OccupantIdentity>
void ResolveLayout(std::vector<WorkspacePartition<OccupantIdentity>>& Partitions,
                   std::int32_t                                      Origin,
                   WorkspaceRectangle                                Area,
                   float                                             GutterThickness)
{
    if (Origin < 0 || static_cast<std::size_t>(Origin) >= Partitions.size())
        return;

    WorkspacePartition<OccupantIdentity>& Standing = Partitions[static_cast<std::size_t>(Origin)];

    if (!Standing.SlotOccupied)
        return;

    Standing.Area           = Area;
    Standing.LayoutResolved = true;

    if (Standing.LeafDeclared)
    {
        Standing.Gutter = {};
        return;
    }

    const float BoundedRatio = Standing.Ratio < 0.05f ? 0.05f : (Standing.Ratio > 0.95f ? 0.95f : Standing.Ratio);

    WorkspaceRectangle FirstArea  = Area;
    WorkspaceRectangle SecondArea = Area;
    WorkspaceRectangle Gutter     = Area;

    if (Standing.Axis == WorkspacePartitionAxis::Row)
    {
        const float DividedSpan = Area.Width - GutterThickness;
        const float FirstSpan   = DividedSpan * BoundedRatio;

        FirstArea.Width      = FirstSpan;
        Gutter.PositionX     = Area.PositionX + FirstSpan;
        Gutter.Width         = GutterThickness;
        SecondArea.PositionX = Gutter.PositionX + GutterThickness;
        SecondArea.Width     = DividedSpan - FirstSpan;
    }
    else
    {
        const float DividedSpan = Area.Height - GutterThickness;
        const float FirstSpan   = DividedSpan * BoundedRatio;

        FirstArea.Height     = FirstSpan;
        Gutter.PositionY     = Area.PositionY + FirstSpan;
        Gutter.Height        = GutterThickness;
        SecondArea.PositionY = Gutter.PositionY + GutterThickness;
        SecondArea.Height    = DividedSpan - FirstSpan;
    }

    Standing.Gutter = Gutter;

    // 📝 The halves are resolved after this partition's own fields are written, so a recursion that refuses part
    //    way still leaves every partition above the refusal carrying a current rectangle.
    const std::int32_t FirstLink  = Standing.FirstLink;
    const std::int32_t SecondLink = Standing.SecondLink;

    ResolveLayout(Partitions, FirstLink,  FirstArea,  GutterThickness);
    ResolveLayout(Partitions, SecondLink, SecondArea, GutterThickness);
}

/// 🧩 The occupied leaf whose resolved rectangle covers a point, or -1 when none does.
/// pre   ResolveLayout ran this tick
/// cost  🚩
/// tag   api, nonallocating, nonthrowing
template <typename OccupantIdentity>
std::int32_t LocateLeafCovering(const std::vector<WorkspacePartition<OccupantIdentity>>& Partitions,
                                std::int32_t                                            Origin,
                                float                                                   PositionX,
                                float                                                   PositionY)
{
    std::int32_t Covering = -1;

    Traverse(Partitions, Origin,
             [&Covering, PositionX, PositionY](std::int32_t Link, const WorkspacePartition<OccupantIdentity>& Standing)
             {
                 if (Standing.LeafDeclared && Standing.LayoutResolved
                  && RectangleCovers(Standing.Area, PositionX, PositionY))
                 {
                     Covering = Link;
                 }
             });

    return Covering;
}

/// 🧩 The division that holds one link as one of its two halves, or -1 when the link is the origin itself.
/// note  Named for what it does rather than for a kinship relation, which the naming rules bar outright.
/// cost  🚩
/// tag   api, nonallocating, nonthrowing
template <typename OccupantIdentity>
std::int32_t LocateHolding(const std::vector<WorkspacePartition<OccupantIdentity>>& Partitions,
                           std::int32_t                                            Origin,
                           std::int32_t                                            HeldLink)
{
    std::int32_t Holding = -1;

    Traverse(Partitions, Origin,
             [&Holding, HeldLink](std::int32_t Link, const WorkspacePartition<OccupantIdentity>& Standing)
             {
                 if (!Standing.LeafDeclared && (Standing.FirstLink == HeldLink || Standing.SecondLink == HeldLink))
                     Holding = Link;
             });

    return Holding;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  CLAIMING AND DIVIDING
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Claims one pool slot as an empty leaf, reusing a reclaimed slot before growing the pool.
/// out   Link  [-]  the claimed index; never negative
/// note  💾 Reuse before growth is what keeps the pool's extent proportional to the leaves standing at once rather
///        than to how many splits the artist has ever made in a session.
/// cost  ✔️
/// tag   api, nonthrowing
template <typename OccupantIdentity>
std::int32_t ClaimPartitionSlot(std::vector<WorkspacePartition<OccupantIdentity>>& Partitions)
{
    for (std::size_t Ordinal = 0u; Ordinal < Partitions.size(); ++Ordinal)
    {
        if (!Partitions[Ordinal].SlotOccupied)
        {
            Partitions[Ordinal] = WorkspacePartition<OccupantIdentity>{};
            return static_cast<std::int32_t>(Ordinal);
        }
    }

    Partitions.push_back(WorkspacePartition<OccupantIdentity>{});

    return static_cast<std::int32_t>(Partitions.size() - 1u);
}

/// 🧩 Divides one leaf in two, moving its occupants into one half and leaving the other empty.
/// in    Partitions      [-]   the pool
/// in    RootLink        [-]   the tree top, amended when the desk was empty
/// in    TargetLeaf      [-]   the leaf to divide; negative claims a root instead
/// in    Axis            [-]   which way the gutter runs
/// in    Ratio           [-]   the first half's share, bounded to [0.05, 0.95]
/// in    ArrivingFirst   [-]   whether the **new empty** half takes the first position
/// out   Outcome         [-]   the new empty leaf's index; refuses with ExtentExhausted when the target's
///                             resolved area cannot carry two leaves at MinimumLeafExtent, and with
///                             IdentityStale when the target is not an occupied leaf
/// post  the target index becomes the division; the former occupants sit in a freshly claimed leaf
/// note  🔴 The target keeps its own index and becomes the division. Every reference held elsewhere — a drag
///         target, a preview, a floating window's return address — therefore stays valid across a split, which
///         is what a scheme that claimed a new index for the division would silently break.
/// cost  🚩
/// tag   api, nonthrowing
template <typename OccupantIdentity>
Outcome<std::int32_t> Partition(std::vector<WorkspacePartition<OccupantIdentity>>& Partitions,
                                std::int32_t&                                     RootLink,
                                std::int32_t                                      TargetLeaf,
                                WorkspacePartitionAxis                            Axis,
                                float                                             Ratio,
                                bool                                              ArrivingFirst)
{
    if (TargetLeaf < 0)
    {
        // 📝 An empty desk is not a refusal. The first document to arrive claims the root, which is how a host
        //    that configures a catalogue and nothing else still comes up with somewhere to put it.
        if (RootLink >= 0)
            return Outcome<std::int32_t>::Refuse({ RefusalReason::IdentityStale, "the desk already carries a root" });

        RootLink = ClaimPartitionSlot(Partitions);

        return Outcome<std::int32_t>::Deliver(RootLink);
    }

    if (static_cast<std::size_t>(TargetLeaf) >= Partitions.size()
     || !Partitions[static_cast<std::size_t>(TargetLeaf)].SlotOccupied
     || !Partitions[static_cast<std::size_t>(TargetLeaf)].LeafDeclared)
    {
        return Outcome<std::int32_t>::Refuse({ RefusalReason::IdentityStale, "the target is not an occupied leaf" });
    }

    {
        const WorkspacePartition<OccupantIdentity>& Target = Partitions[static_cast<std::size_t>(TargetLeaf)];

        if (Target.LayoutResolved)
        {
            const float DividedSpan = Axis == WorkspacePartitionAxis::Row ? Target.Area.Width : Target.Area.Height;

            if (DividedSpan < MinimumLeafExtent * 2.0f)
            {
                return Outcome<std::int32_t>::Refuse(
                    { RefusalReason::ExtentExhausted, "the leaf cannot carry two leaves at the minimum extent" });
            }
        }
    }

    // 📝 Both slots are claimed before either is written. A claim may grow the pool, and a reference taken into
    //    it before the growth is a reference into storage the growth has already released.
    const std::int32_t CarriedLink  = ClaimPartitionSlot(Partitions);
    const std::int32_t ArrivingLink = ClaimPartitionSlot(Partitions);

    Partitions[static_cast<std::size_t>(CarriedLink)].LeafDeclared   = true;
    Partitions[static_cast<std::size_t>(CarriedLink)].Occupants      =
        Partitions[static_cast<std::size_t>(TargetLeaf)].Occupants;
    Partitions[static_cast<std::size_t>(CarriedLink)].ActiveOccupant =
        Partitions[static_cast<std::size_t>(TargetLeaf)].ActiveOccupant;

    Partitions[static_cast<std::size_t>(ArrivingLink)].LeafDeclared = true;

    WorkspacePartition<OccupantIdentity>& Divided = Partitions[static_cast<std::size_t>(TargetLeaf)];

    Divided.LeafDeclared   = false;
    Divided.Occupants.clear();
    Divided.ActiveOccupant = {};
    Divided.Axis           = Axis;
    Divided.Ratio          = Ratio < 0.05f ? 0.05f : (Ratio > 0.95f ? 0.95f : Ratio);
    Divided.FirstLink      = ArrivingFirst ? ArrivingLink : CarriedLink;
    Divided.SecondLink     = ArrivingFirst ? CarriedLink  : ArrivingLink;
    Divided.LayoutResolved = false;

    return Outcome<std::int32_t>::Deliver(ArrivingLink);
}

/// 🧩 Reclaims one emptied leaf, lifting the other half of its division into the division's own slot.
/// in    Partitions  [-]  the pool
/// in    RootLink    [-]  the tree top, cleared when the last leaf leaves
/// in    EmptiedLeaf [-]  the leaf to reclaim
/// out   Outcome     [-]  refuses with IdentityStale when the index is not an occupied leaf, and with
///                        ContentUnsupported when the leaf still carries occupants
/// post  the division that held it carries what the other half carried; both reclaimed slots are free
/// note  🔴 Lifting into the division's **own** index rather than replacing the division with the other half's
///         index is again what keeps every outside reference valid. It costs one copy of a small record.
/// cost  🚩
/// tag   api, nonthrowing
template <typename OccupantIdentity>
Outcome<bool> ReclaimLeaf(std::vector<WorkspacePartition<OccupantIdentity>>& Partitions,
                          std::int32_t&                                     RootLink,
                          std::int32_t                                      EmptiedLeaf)
{
    if (EmptiedLeaf < 0 || static_cast<std::size_t>(EmptiedLeaf) >= Partitions.size()
     || !Partitions[static_cast<std::size_t>(EmptiedLeaf)].SlotOccupied
     || !Partitions[static_cast<std::size_t>(EmptiedLeaf)].LeafDeclared)
    {
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "the index is not an occupied leaf" });
    }

    if (!Partitions[static_cast<std::size_t>(EmptiedLeaf)].Occupants.empty())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the leaf still carries occupants" });

    const std::int32_t Holding = LocateHolding(Partitions, RootLink, EmptiedLeaf);

    if (Holding < 0)
    {
        // 📝 The last leaf on the desk. The desk goes empty rather than keeping a leaf nothing occupies, and the
        //    next document to arrive claims a fresh root through Partition.
        Partitions[static_cast<std::size_t>(EmptiedLeaf)].SlotOccupied = false;
        RootLink = -1;

        return Outcome<bool>::Deliver(true);
    }

    const std::int32_t CounterpartLink =
        Partitions[static_cast<std::size_t>(Holding)].FirstLink == EmptiedLeaf
            ? Partitions[static_cast<std::size_t>(Holding)].SecondLink
            : Partitions[static_cast<std::size_t>(Holding)].FirstLink;

    if (CounterpartLink < 0 || static_cast<std::size_t>(CounterpartLink) >= Partitions.size())
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "the division holds no counterpart" });

    WorkspacePartition<OccupantIdentity> Lifted = Partitions[static_cast<std::size_t>(CounterpartLink)];

    Lifted.SlotOccupied   = true;
    Lifted.LayoutResolved = false;

    Partitions[static_cast<std::size_t>(Holding)]         = Lifted;
    Partitions[static_cast<std::size_t>(CounterpartLink)].SlotOccupied = false;
    Partitions[static_cast<std::size_t>(EmptiedLeaf)].SlotOccupied     = false;

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE IN-BODY PANEL BOX
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Where a panel box is anchored inside a document's body.
/// note  The four docked sides reserve a band: Left and Right take a full-height column, Top and Bottom take a row
///        spanning what those leave, and Centre fills the remainder. Several panels sharing a side split the band
///        along its long axis in list order.
/// tag   contract
enum class WorkspacePanelSide : std::uint32_t
{
    Floating = 0u,   // [-] - a free rectangle overlaid on the body
    Left     = 1u,   // [-] - a reserved full-height column at the left
    Right    = 2u,   // [-] - a reserved full-height column at the right
    Top      = 3u,   // [-] - a reserved row along the top, between the columns
    Bottom   = 4u,   // [-] - a reserved row along the bottom, between the columns
    Centre   = 5u    // [-] - whatever the reserved bands leave
};

/// 🧩 One panel box inside one document's body — a floating overlay until it is docked to a side.
/// note  ⚠️ Offsets are relative to the body's top-left, so a floating box tracks the body when the desk is
///        re-divided rather than staying where the screen was.
/// note  🚧 The presentation of these is `2e` and is severable. The record is declared now because
///        `WorkspaceDocument` holds it and the document's extent should not change when `2e` lands.
/// tag   owning
struct WorkspacePanelBox
{
    WorkspacePanelIdentity  Identity                     = {};                              // [-]
    char                    Title[WorkspaceTitleExtent]  = {};                              // [-] - header caption
    WorkspacePanelSide      Side         = WorkspacePanelSide::Floating;                    // [-] - the anchor
    float                   OffsetX      = 0.0f;    // [px] - floating top-left, relative to the body
    float                   OffsetY      = 0.0f;    // [px] - floating top-left, relative to the body
    float                   Width        = 220.0f;  // [px] - floating width
    float                   Height       = 160.0f;  // [px] - floating height
    float                   DockExtent   = 0.28f;   // [-]  - docked band depth, a fraction of the body span
    float                   SlotFraction = 1.0f;    // [-]  - share of the band when panels share a side
    bool                    SlotOccupied = true;    // [-]  - false marks a reclaimed record
};

//------------------------------------------------------------------------------------------------------------------------
//                                                      ONE DOCUMENT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One workspace document — what a tab represents, wherever the tab currently lives.
/// note  🔴 `14` §4.1: everything here is beside the document and never inside it. Its own panel arrangement is
///        layout, and layout inside the document would make moving a panel an undoable edit and would make one
///        document open differently on a machine with a different display.
/// tag   owning
struct WorkspaceDocument
{
    WorkspaceDocumentIdentity                          Identity                    = {};    // [-]
    char                                               Title[WorkspaceTitleExtent] = {};    // [-] - renamable
    WorkspaceDiscipline                                Discipline = WorkspaceDiscipline::Empty;
    std::vector<WorkspacePanelBox>                     PanelBoxes      = {};   // [-] - every box this body holds
    std::vector<WorkspacePartition<WorkspacePanelIdentity>>
                                                       PanelPartitions = {};   // [-] - the body's docked arrangement
    std::int32_t                                       PanelRoot   = -1;       // [-] - -1 while nothing is docked
    std::uint32_t                                      MintedPanels = 0u;      // [-] - source of "Panel N"
    bool                                               SlotOccupied = true;    // [-] - false marks a reclaimed slot
};

/// 🧩 A torn-out rectangle carrying its own ordered documents and its own tab strip.
/// note  🚧 Moved, resized and docked in `2d`. Declared now because the desk holds the list and a document may
///         already sit in one before any drag exists.
/// tag   owning
struct WorkspaceFloatingWindow
{
    std::uint32_t                           Identifier     = 0u;      // [-]  - unique within the desk; zero absent
    std::vector<WorkspaceDocumentIdentity>  Documents      = {};      // [-]  - ordered; last is topmost
    WorkspaceDocumentIdentity               ActiveDocument = {};      // [-]  - the one its body presents
    float                                   PositionX      = 0.0f;    // [px] - top-left, in interface pixels
    float                                   PositionY      = 0.0f;    // [px]
    float                                   Width          = 380.0f;  // [px]
    float                                   Height         = 280.0f;  // [px]
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DRAG IN FLIGHT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What the one drag in flight is addressing. Exactly one is open at a time.
/// note  🚧 `2c` writes only None and the pending press. The remaining modes are resolved by `2d`; they are
///        declared now because the record is one struct and adding a mode later would re-extent the desk.
/// tag   contract
enum class WorkspaceDragMode : std::uint32_t
{
    None            = 0u,   // [-] - nothing is held
    Reorder         = 1u,   // [-] - a strip trapezoid is held and sliding
    Window          = 2u,   // [-] - a floating window is being moved
    Resize          = 3u,   // [-] - a floating window's grip is held
    Partition       = 4u,   // [-] - a division's gutter is held
    PanelBox        = 5u,   // [-] - a panel box is being moved inside a body
    PanelResize     = 6u,   // [-] - a floating panel box's handle is held
    PanelBandResize = 7u    // [-] - a docked band's depth or slot share is held
};

/// 🧩 What a Window-mode drag was torn from — the drop resolution differs by origin.
/// note  🔴 A torn **tab** floats by default and docks only on a distinct edge or corner band, so a careless
///        release leaves it floating. A moved **panel** uses the five-zone cross and always docks over a leaf.
/// tag   contract
enum class WorkspaceDragOrigin : std::uint32_t
{
    Tab   = 0u,   // [-] - torn from a document strip
    Panel = 1u    // [-] - a panel box moved bodily
};

/// 🧩 The landing a released drag would take, resolved from the pointer each tick.
/// tag   contract
enum class WorkspaceDropZone : std::uint32_t
{
    None   = 0u,   // [-] - over no valid target
    Strip  = 1u,   // [-] - back onto a strip
    Centre = 2u,   // [-] - stack onto the covered leaf
    Left   = 3u,   // [-] - divide, arriving on the left
    Right  = 4u,   // [-] - divide, arriving on the right
    Top    = 5u,   // [-] - divide, arriving above
    Bottom = 6u    // [-] - divide, arriving below
};

/// 🧩 Where a release would land, and the rectangle that says so before the artist commits to it.
/// note  Resolved fresh every tick from the pointer, never accumulated. A landing carried across a tick is a
///        landing that survives the leaf it named being reclaimed underneath it.
/// tag   contract, nonallocating, nonthrowing
struct WorkspaceDropLanding
{
    WorkspaceDropZone   Zone        = WorkspaceDropZone::None;   // [-]  - what a release would do
    std::int32_t        Link        = -1;                        // [-]  - the leaf it would act on
    std::uint32_t       Window      = 0u;                        // [-]  - the window it would stack onto
    WorkspaceRectangle  PreviewArea = {};                        // [px] - what the accent wash covers
};

// 📝 🔴 The band a tab must reach to dock rather than float. Frontier's fraction, ported: below about a fifth the
//    artist cannot aim at it against a small leaf, and above about a third the centre of a leaf stops being a
//    place a tab can be dropped without splitting something.
inline constexpr float DockBandFraction = 0.22f;   // [-] - share of a body span the edge band claims

// 📝 A floating window narrower than this cannot carry a strip and a grip at once.
inline constexpr float WindowMinimumExtent = 160.0f;   // [px] - the smallest a resize may leave a window

/// 🧩 The one drag in flight, and the press that has not yet become one.
/// note  🔴 A click **activates** a tab. Only movement past `TearThreshold` while still held tears it out, which
///        is why the press is recorded apart from the drag: without it every activation would tear.
/// tag   owning
struct WorkspaceDragRecord
{
    WorkspaceDragMode          Mode           = WorkspaceDragMode::None;
    WorkspaceDragOrigin        Origin         = WorkspaceDragOrigin::Tab;
    WorkspaceDocumentIdentity  HeldDocument   = {};      // [-]  - reorder and tear
    std::uint32_t              HeldWindow     = 0u;      // [-]  - window and resize; zero absent
    std::int32_t               HeldLink       = -1;      // [-]  - partition: the division whose gutter is held
    float                      GrabOffsetX    = 0.0f;    // [px] - pointer offset inside the held thing
    float                      GrabOffsetY    = 0.0f;    // [px]

    WorkspaceDocumentIdentity  PendingDocument = {};     // [-]  - pressed, not yet torn
    float                      PendingPressX   = 0.0f;   // [px] - pointer at the press
    float                      PendingPressY   = 0.0f;   // [px]
    float                      PendingTabLeft  = 0.0f;   // [px] - the trapezoid's left at the press

    WorkspaceDropZone          PreviewZone    = WorkspaceDropZone::None;
    std::int32_t               PreviewLink    = -1;      // [-]  - leaf the preview targets
    std::uint32_t              PreviewWindow  = 0u;      // [-]  - window the preview stacks onto
    WorkspaceRectangle         PreviewArea    = {};      // [px] - the highlighted rectangle
};

// 📝 Below this the pointer has not moved enough to mean anything but a click. Frontier's threshold, ported
//    verbatim: a smaller one tears a tab off on an unsteady click, a larger one makes a deliberate tear feel stuck.
inline constexpr float TearThreshold = 6.0f;   // [px] - summed pointer travel that turns a press into a tear

/// 🧩 One inline title edit, opened by a double-click on a trapezoid.
/// note  The carry is the edited title, not the committed one. Abandoning the edit discards it and the document's
///        own title is never written until the edit is sealed.
/// tag   owning
struct WorkspaceRenameRecord
{
    bool                       RenameOpen                  = false;   // [-] - an edit is running
    WorkspaceDocumentIdentity  Subject                     = {};      // [-] - whose title is being edited
    char                       Carry[WorkspaceTitleExtent] = {};      // [-] - the edited text
    std::uint32_t              CarryExtent                 = 0u;      // [-] - characters held, terminator excluded
};

/// 🧩 One hand-rolled foreground overlay — the `(+)` minting list or the `(V)` panel list.
/// note  🔴 Not a vendor popup. Every trapezoid is painted on the foreground draw list, and a vendor popup sits
///        beneath that list, so a popup would be occluded by the tabs that opened it.
/// note  The tick it opened is recorded so the press that opened it does not immediately dismiss it.
/// tag   owning
struct WorkspaceOverlayRecord
{
    bool                       OverlayOpen   = false;   // [-]  - the overlay is presenting
    std::uint32_t              OpenedTick    = 0u;      // [-]  - the tick it opened on
    float                      AnchorX       = 0.0f;    // [px] - overlay top-left
    float                      AnchorY       = 0.0f;    // [px]
    std::int32_t               TargetLink    = -1;      // [-]  - leaf a chosen entry lands in
    std::uint32_t              TargetWindow  = 0u;      // [-]  - window a chosen entry lands in
    WorkspaceDocumentIdentity  TargetDocument = {};     // [-]  - document a chosen panel box lands in
};

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE DESK
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The whole desk: every document owned once, the partition of the docked area, and the floating windows.
/// note  🔴 A document is owned here exactly once and referenced everywhere else by identity. A leaf holding a
///         document by value is the defect where tearing a tab out produces two documents that drift apart.
/// tag   owning
struct WorkspaceSpace
{
    std::vector<WorkspaceDocument>                            Documents       = {};   // [-] - owned once
    std::vector<WorkspaceDocumentSpecification>               Catalogue       = {};   // [-] - what (+) offers
    std::uint32_t                                             DefaultCatalogueOrdinal = 0u;

    std::vector<WorkspacePartition<WorkspaceDocumentIdentity>> Partitions     = {};   // [-] - the pool
    std::int32_t                                              RootLink        = -1;   // [-] - -1 = empty desk

    std::vector<WorkspaceFloatingWindow>                      Floating        = {};   // [-] - last is topmost
    std::uint32_t                                             MintedWindows   = 0u;   // [-] - window key source

    WorkspaceDragRecord                                       Dragging        = {};   // [-] - the one drag in flight
    WorkspaceRenameRecord                                     Renaming        = {};   // [-] - the one open title edit
    WorkspaceOverlayRecord                                    MintingOverlay  = {};   // [-] - what (+) presents
    WorkspaceOverlayRecord                                    PanelOverlay    = {};   // [-] - what (V) presents

    // 📝 The desk counts its own presentations rather than reading the vendor's tick count, which keeps every
    //    field of this struct spellable without the vendor header. An overlay compares against it to ignore the
    //    press that opened it.
    std::uint32_t                                             PresentedTicks  = 0u;   // [-] - presentations so far
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     DESK OPERATIONS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Installs the catalogue the application offers and forgets whatever was offered before.
/// in    Space      [-]  the desk
/// in    Offering   [-]  the entries, copied here; caller-owned only for the duration of the call
/// in    Count      [-]  how many
/// in    DefaultOrdinal [-] which entry seeds the desk
/// out   Outcome    [-]  refuses with ContentUnsupported for an empty offering or an ordinal outside it
/// cost  🚩
/// tag   api, nonthrowing
Outcome<bool> DeclareCatalogue(WorkspaceSpace&                       Space,
                               const WorkspaceDocumentSpecification* Offering,
                               std::uint32_t                         Count,
                               std::uint32_t                         DefaultOrdinal);

/// 🧩 Seeds the desk with one document of the default catalogue entry and activates it.
/// out   Outcome  [-]  refuses with ContentUnsupported when no catalogue has been declared
/// pre   DeclareCatalogue delivered
/// cost  🚩
/// tag   api, nonthrowing
Outcome<WorkspaceDocumentIdentity> ConstructWorkspaceSpace(WorkspaceSpace& Space);

/// 🧩 Mints one document from a catalogue entry and places it in a leaf.
/// in    Space             [-]  the desk
/// in    CatalogueOrdinal  [-]  which entry
/// in    TargetLeaf        [-]  the leaf it joins; negative claims the root when the desk is empty
/// out   Outcome           [-]  refuses with ContentUnsupported outside the catalogue, and with IdentityStale
///                              when the target is not an occupied leaf
/// post  the minted document is the target leaf's active occupant
/// note  The title is minted as "<NameStem> N", where N counts the documents already carrying that stem. Counting
///        rather than holding a per-entry counter means closing document 2 and minting again produces a 2 again,
///        which is what an artist expects of a numbered tab.
/// cost  🚩
/// tag   api, nonthrowing
Outcome<WorkspaceDocumentIdentity> DeclareDocument(WorkspaceSpace& Space,
                                                   std::uint32_t   CatalogueOrdinal,
                                                   std::int32_t    TargetLeaf);

/// 🧩 Withdraws one document from the desk, reclaiming the leaf it emptied.
/// out   Outcome  [-]  refuses with IdentityStale when nothing resolves
/// cost  🚩
/// tag   api, nonthrowing
Outcome<bool> WithdrawDocument(WorkspaceSpace& Space, WorkspaceDocumentIdentity Subject);

/// 🧩 One document, by identity.
/// out   Outcome  [-]  refuses with IdentityStale
/// cost  🚩
/// tag   api, nonthrowing
Outcome<const WorkspaceDocument*> ResolveDocument(const WorkspaceSpace& Space, WorkspaceDocumentIdentity Subject);

/// 🧩 One document, for amending its title or its panel arrangement.
/// out   Outcome  [-]  refuses with IdentityStale
/// cost  🚩
/// tag   api, nonthrowing
Outcome<WorkspaceDocument*> AmendDocument(WorkspaceSpace& Space, WorkspaceDocumentIdentity Subject);

/// 🧩 Which leaf currently carries one document, or -1 when a floating window does.
/// cost  🚩
/// tag   api, nonthrowing
std::int32_t LocateLeafCarrying(const WorkspaceSpace& Space, WorkspaceDocumentIdentity Subject);

/// 🧩 Which floating window carries one document, or zero when a leaf does.
/// cost  🚩
/// tag   api, nonthrowing
std::uint32_t LocateWindowCarrying(const WorkspaceSpace& Space, WorkspaceDocumentIdentity Subject);

/// 🧩 One floating window, for moving or resizing it.
/// out   Outcome  [-]  refuses with IdentityStale when no window carries that key
/// cost  🚩
/// tag   api, nonthrowing
Outcome<WorkspaceFloatingWindow*> AmendFloatingWindow(WorkspaceSpace& Space, std::uint32_t WindowKey);

/// 🧩 Lifts one document out of wherever it sits into a floating window of its own.
/// in    Space      [-]   the desk
/// in    Subject    [-]   the document torn out
/// in    PositionX  [px]  the window's top-left, already offset by the grab
/// in    PositionY  [px]
/// out   Outcome    [-]   the minted window's key; refuses with IdentityStale when nothing resolves
/// post  the emptied leaf, if any, is reclaimed; the document is the new window's only occupant
/// note  🔴 The document is never copied. It stays owned once by `Documents` and the window names it by identity,
///        which is what keeps a torn tab and its former self from drifting apart.
/// cost  🚩
/// tag   api, nonthrowing
Outcome<std::uint32_t> TearDocument(WorkspaceSpace&           Space,
                                    WorkspaceDocumentIdentity Subject,
                                    float                     PositionX,
                                    float                     PositionY);

/// 🧩 Lands one document on a leaf — stacking it, or dividing the leaf and placing it in the new half.
/// in    TargetLeaf  [-]  the leaf the landing named
/// in    Zone        [-]  Strip and Centre stack; the four sides divide
/// out   Outcome     [-]  refuses with IdentityStale when the document or the leaf does not resolve, and with
///                        whatever `Partition` refused when the leaf is too small to divide
/// note  🔴 The document is placed **before** it is removed from where it was, and the leaf it emptied is
///        reclaimed only after both are done. Reclaiming first lifts a counterpart into the division's slot,
///        and the target index the landing resolved a moment earlier then names a freed slot.
/// cost  🚩
/// tag   api, nonthrowing
Outcome<bool> DockDocument(WorkspaceSpace&           Space,
                           WorkspaceDocumentIdentity Subject,
                           std::int32_t              TargetLeaf,
                           WorkspaceDropZone         Zone);

/// 🧩 Stacks one document onto an existing floating window and activates it there.
/// out   Outcome  [-]  refuses with IdentityStale when the document or the window does not resolve
/// cost  🚩
/// tag   api, nonthrowing
Outcome<bool> StackDocumentInWindow(WorkspaceSpace&           Space,
                                    WorkspaceDocumentIdentity Subject,
                                    std::uint32_t             WindowKey);

/// 🧩 Moves one document to a position within the ordered occupants of the leaf or window carrying it.
/// in    Position  [-]  the ordinal it takes; bounded to the occupant count
/// out   Outcome   [-]  refuses with IdentityStale when the document does not resolve where it was said to be
/// note  Reorder never changes which occupant is active. A tab slid past its neighbour keeps its body presented.
/// cost  🚩
/// tag   api, nonthrowing
Outcome<bool> ReorderOccupant(WorkspaceSpace&           Space,
                              WorkspaceDocumentIdentity Subject,
                              std::int32_t              CarryingLeaf,
                              std::uint32_t             CarryingWindow,
                              std::uint32_t             Position);

/// 🧩 Resolves what a release at one point would do, without doing any of it.
/// in    Origin         [-]   a torn tab floats unless it reaches an edge band; a moved panel always docks
/// in    StripHeight    [px]  from the theme, so a strip can be told apart from a body
/// in    IgnoredWindow  [-]   the window being dragged, which must not resolve as its own target
/// note  🔴 Pure geometry over the rectangles `ResolveLayout` wrote this tick. Nothing here mutates the desk,
///        which is what lets the same call drive the preview each tick and the landing on release.
/// cost  🚩
/// tag   api, nonallocating, nonthrowing
WorkspaceDropLanding ResolveDropLanding(const WorkspaceSpace& Space,
                                        WorkspaceDragOrigin   Origin,
                                        float                 PointerX,
                                        float                 PointerY,
                                        float                 StripHeight,
                                        std::uint32_t         IgnoredWindow);

/// 🧩 Resolves the whole desk's layout for this tick.
/// in    DeskArea         [px]  the area between the two bands, supplied by the bracket
/// in    GutterThickness  [px]  from the theme
/// pre   called once a tick, before any input is resolved
/// cost  🚩
/// tag   api, nonthrowing
void ResolveSpaceLayout(WorkspaceSpace& Space, WorkspaceRectangle DeskArea, float GutterThickness);

/// 🧩 Presents the whole desk for one tick — layout, bodies, trapezoid strips, overlays, then input.
/// in    Theme     [-]   the resolved theme, read and never held
/// in    Space     [-]   the desk, amended in place
/// in    DeskArea  [px]  the area between the two bands, supplied by the bracket
/// pre   an interface tick is open — `InterfaceExchange::Advance` delivered and `Seal` has not
/// post  layout is current, and every activation, rename and mint the tick carried has been applied
/// note  🔴 Nothing here opens a vendor window. Every quad is recorded on the foreground list, because a
///        trapezoid cannot be a vendor tab and a vendor popup would be painted beneath the tabs that opened it.
/// note  🚧 `2c`. Tear-out, docking preview and the in-body panel layer are `2d` and `2e`; the records they
///        write are declared above and this call leaves them at rest.
/// cost  🚩
/// tag   api, nonthrowing
void PresentWorkspaceSpace(const ThemeSpecification& Theme, WorkspaceSpace& Space, WorkspaceRectangle DeskArea);

// 📐 Identities, occupant counts and pool indices are Exact. Rectangles, ratios and band fractions are Bounded.
//    The component claims Bounded, per `00` §3's transitivity rule.
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded, PrecisionGuarantee::Exact);

}   // namespace Slate
