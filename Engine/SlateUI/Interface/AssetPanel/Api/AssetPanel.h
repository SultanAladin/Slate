//============================================================================================================================================
//                                                              ASSETPANEL.H
//============================================================================================================================================
// 🧩 The offered content made browsable — a folder column, a tile area, and every band's control live before one asset is declared.

#pragma once

#include "Contract/OutcomeContract.h"
#include "Contract/PrecisionContract.h"
#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/PanelIndex.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/WorkspaceSpace.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE DECLARED EXTENTS
//------------------------------------------------------------------------------------------------------------------------

// 📝 Fixed extents rather than growing runs. The panel presents what a host declared and never appends, so `00`'s
//    prohibition on allocation outside an extent slicer holds here without a slicer being involved at all.
inline constexpr std::uint32_t AssetEntryCapacity   = 128u;   // [-] - entries one folder run may offer
inline constexpr std::uint32_t AssetFolderCapacity  =  24u;   // [-] - folders the column may present
inline constexpr std::uint32_t AssetCaptionExtent   =  64u;   // [-] - characters a caption carries, terminator included

//------------------------------------------------------------------------------------------------------------------------
//                                                   WHAT AN ENTRY CARRIES
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What one offered entry is, resolved from the declaration and never inferred from its caption.
/// note  🔴 Declared by whoever offers the entry. A panel that read the content out of a file suffix would be the
///        component deciding what a surface is, and the first authored extension nobody listed presents as an
///        entry the artist cannot open for a reason the panel invented.
/// tag   contract
enum class AssetContent : std::uint32_t
{
    Surface   = 0u,   // [-] - an image the paint sequence can resolve onto
    Material  = 1u,   // [-] - a channel run addressed through `42`
    Brush     = 2u,   // [-] - an authored stroke specification
    Geometry  = 3u,   // [-] - a mesh the outliner can carry
    Document  = 4u,   // [-] - a whole session, openable rather than placeable
    Undeclared = 5u   // [-] - offered without a content declaration; presented and never opened
};

/// 🧩 The caption one content is presented under.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* CaptionOf(AssetContent Declared);

/// 🧩 The stroke one content is drawn with where no thumbnail has been uploaded.
/// note  📝 A stroke and not a colour. The tile face already carries the palette's tile hue, and a per-content
///        colour would be a second palette spelled inside a panel.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
ControlStroke StrokeOf(AssetContent Declared);

/// 🧩 One offered entry, as whoever declared it spelled it.
/// note  ⚠️ Both text pointers name storage the declarer owns. Nothing here is copied, so an entry built from a
///        local buffer leaves the panel printing a released extent the first tick after the declaring scope closes.
/// tag   contract, nonallocating, nonthrowing
struct AssetEntry
{
    const char*    Caption        = nullptr;                      // [-]  - what the tile prints
    const char*    Detail         = nullptr;                      // [-]  - the second line: extent, revision, origin
    AssetContent   Content        = AssetContent::Undeclared;     // [-]  - what it is, declared and never inferred
    std::uint32_t  FolderOrdinal  = 0u;                           // [-]  - which folder carries it
    std::uint64_t  ByteExtent     = 0u;                           // [B]  - what it occupies where it is retained
    bool           ChosenDeclared = false;                        // [-]  - the artist has it chosen
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  WHAT A FOLDER CARRIES
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One row of the folder column — its caption, its depth, and whether its fold is open.
/// note  📝 Depth is carried rather than derived from an enclosure link, because the column presents a
///        linearisation and never a relation. `12` §7 makes the same split for the outliner and for the same reason.
/// tag   contract, nonallocating, nonthrowing
struct AssetFolder
{
    const char*    Caption        = nullptr;   // [-] - the row's text
    std::uint32_t  Depth          = 0u;        // [-] - indents by `IndentWidth` per step
    std::uint32_t  CountedEntries = 0u;        // [-] - what the folder offers, printed at the row's trailing edge
    bool           FolderOpen     = true;      // [-] - the fold is open, so its deeper rows present
};

//------------------------------------------------------------------------------------------------------------------------
//                                                 HOW THE AREA PRESENTS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Whether the entries present as tiles or as one row apiece.
/// tag   contract
enum class AssetPresentation : std::uint32_t
{
    Tiles = 0u,   // [-] - a lattice of thumbnails, sized by the footer's slider
    Rows  = 1u    // [-] - one line apiece, caption and detail side by side
};

/// 🧩 What the entries are ordered by.
/// note  ⚠️ Ordering is resolved at presentation and the declared run is never permuted. A panel that sorted the
///        run it was handed would reorder storage its declarer still holds ordinals into.
/// tag   contract
enum class AssetOrdering : std::uint32_t
{
    Caption  = 0u,   // [-] - lexicographic by the printed caption
    Content  = 1u,   // [-] - grouped by declared content, caption within
    Extent   = 2u,   // [-] - largest first
    Declared = 3u    // [-] - the order the declarer offered them in, untouched
};

//------------------------------------------------------------------------------------------------------------------------
//                                                THE ASSET SPECIFICATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Everything the asset panel presents and carries — the offered runs, and the presentation the artist chose.
/// note  🔴 Held by value and not by pointer, exactly as `CanvasSpecification` is. What the artist chose to look at
///        is layout and never document — `14` §4.1 — so a host threading in a pointer would be a host deciding
///        which folder is open, and closing the workspace would become a destructive edit.
/// note  🔴 The two runs are declared **into** this record by whoever offers content. Both counts are zero here,
///        which is the state the panel is complete in: every band, every control and both empty states present and
///        answer the pointer before one asset exists.
/// note  ⚠️ `ChosenEntry` is an ordinal into `Offered` and is bounded at every read rather than trusted. A run
///        that shortens beneath a chosen ordinal is the ordinary case — a folder collapsing narrows it — and a
///        trusted ordinal would read past the count on the tick that narrowing happened.
/// tag   owning
struct AssetSpecification
{
    // what was offered ------------------------------------------------------------------------------
    AssetEntry     Offered[AssetEntryCapacity]   = {};   // [-] - the entries, declared and never appended here
    std::uint32_t  OfferedCount                  = 0u;   // [-] - how many of them are declared
    AssetFolder    Folders[AssetFolderCapacity]  = {};   // [-] - the column's rows, deepest last
    std::uint32_t  FolderCount                   = 0u;   // [-] - how many of them are declared

    // what the artist chose to look at --------------------------------------------------------------
    std::uint32_t      ChosenFolder      = 0u;                            // [-] - which folder row is standing
    std::uint32_t      ChosenEntry       = 0u;                            // [-] - which entry is standing
    bool               EntryChosen       = false;                         // [-] - an entry is standing at all
    AssetPresentation  Presentation      = AssetPresentation::Tiles;      // [-] - tiles or rows
    AssetOrdering      Ordering          = AssetOrdering::Caption;        // [-] - what the run is ordered by
    bool               ContentDeclared[6] = { true, true, true, true, true, true };
                                                                          // [-] - which contents are presented
    // the live controls' carries -------------------------------------------------------------------
    TextCarry      Sought            = {};       // [-]  - the search entry, wired and not decorative
    DropdownCarry  OrderingCarry     = {};       // [-]  - the ordering dropdown's openness and anchor
    double         TileExtent        = 96.0;     // [px] - one tile's edge, dragged by the footer track
    float          VisibleOffset     = 0.0f;     // [px] - top of the presented span of entries
    float          FolderOffset      = 0.0f;     // [px] - top of the presented span of folder rows
    float          ColumnFraction    = 0.30f;    // [-]  - the folder column's share of the body's width
    bool           ColumnHeld        = false;    // [-]  - the divider between column and area is being dragged
    bool           TileHeld          = false;    // [-]  - the footer's tile track is being dragged
    std::uint32_t  PresentedTicks    = 0u;       // [-]  - ticks presented, for dropdown dismissal
};

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE BAND ARITHMETIC
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The stacked bands, and the two halves the body between them divides into.
/// note  🔴 The search entry has a band to itself and does not share one with the ordering head. Every labelled
///        control in `ControlPanel` reserves a label column through `ResolveControlRow` — a caption of `nullptr`
///        suppresses the caption and not the column — so three controls sharing one row leaves each of them a
///        field narrower than the primitive accepts, and all three refuse at every width a docked panel takes.
/// tag   contract, nonallocating, nonthrowing
struct AssetBands
{
    WorkspaceRectangle  HeaderBand   = {};   // [px] - the caption band and its glyph buttons
    WorkspaceRectangle  SearchBand   = {};   // [px] - the search entry, a row to itself
    WorkspaceRectangle  NarrowBand   = {};   // [px] - the ordering head and the content segment row
    WorkspaceRectangle  FolderColumn = {};   // [px] - the folder rows
    WorkspaceRectangle  Divider      = {};   // [px] - the draggable band between column and area
    WorkspaceRectangle  EntryArea    = {};   // [px] - the tiles or the rows
    WorkspaceRectangle  FooterBand   = {};   // [px] - the counts and the tile track
};

/// 🧩 Resolves the bands from the rectangle the desk gave the panel.
/// in    Extents         [-]   read for the four band heights and the divider's thickness
/// in    Area            [px]  the whole panel
/// in    ColumnFraction  [-]   the folder column's share, bounded here and not by the caller
/// out   Bands           [px]  the six rectangles, none overlapping
/// note  ⚠️ Every band is bounded at zero height rather than allowed negative. A negative extent inverts
///        `RectangleCovers`, and a panel dragged below its bands would then resolve presses against rectangles
///        covering the whole display instead of none of it.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
AssetBands ResolveAssetBands(const LayoutExtents& Extents, const WorkspaceRectangle& Area, float ColumnFraction);

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE NARROWING
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Whether one entry survives the standing folder, the content narrowing and the search entry together.
/// in    Standing  [-]  the specification, read for all three narrowings
/// in    Ordinal   [-]  which entry; outside the count answers false rather than reading past it
/// note  📝 One place, so the count in the footer and the run in the area cannot disagree. Two copies of this
///        predicate is the defect where a footer reads "12 assets" over an area presenting nine.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
bool EntrySurvives(const AssetSpecification& Standing, std::uint32_t Ordinal);

/// 🧩 How many entries survive the three narrowings.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
std::uint32_t SurvivingCount(const AssetSpecification& Standing);

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Presents one tick of the asset panel into the rectangle the desk resolved.
/// in    Theme     [-]   read for the palette and the band extents; never held
/// in    Area      [px]  the interior the desk resolved, honoured exactly
/// in    Standing  [-]   the specification, amended in place by every control on every band
/// post  the panel is painted onto the foreground recording and the artist's choices are written back
/// note  🔴 Nothing here opens a vendor window and nothing here reads the vendor pointer. Every quad leaves
///        through `ControlPanel`'s seam, which `14` §7 holds to exactly one component knowing which recording the
///        interface paints on.
/// note  ⚠️ Neither empty state is a refusal. A panel offered nothing presents its bands and says so, because a
///        refusal per tick per panel would append to `86`'s register on the ordinary path of an empty folder.
/// cost  🚩
/// tag   api, nonthrowing
void PresentAssets(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, AssetSpecification& Standing);

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PANEL ROUTINE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The presentation routine matching `PanelPresentRoutine`.
/// in    PresentContext  [-]  an `AssetSpecification*`; a null context presents nothing at all
/// note  🔴 Matches `PanelPresentRoutine` exactly, so a workspace declares it into `PanelIndex` and the desk never
///        learns what an asset is.
/// cost  🚩
/// tag   api, nonthrowing
void PresentAssetPanel(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, void* PresentContext);

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE LEDGER SLOT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Resolves a panel slot for the asset panel.
/// in    Standing  [-]  the specification the slot will address for as long as the workspace stands
/// out   Slot      [-]  the resolved slot, ready for ledger registration
/// note  🔴 The returned slot points at `Standing`. Registering a slot built from a temporary is a dangling
///        context, and it presents as a panel painted against released storage.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
PanelSlot ResolveAssetSlot(const char*          PanelIdentifier,
                           const char*          PanelTitle,
                           WorkspacePanelSide   DeclaredSide,
                           AssetSpecification&  Standing);

// 📐 Ordinals, counts and byte extents are Exact integers. Every rectangle the panel is handed is Bounded and none
//    of them is derived here. The component claims Bounded, per `00` §3's transitivity rule.
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded, PrecisionGuarantee::Exact);

}   // namespace Slate
