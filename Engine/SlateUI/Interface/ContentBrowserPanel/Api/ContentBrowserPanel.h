//============================================================================================================================================
//                                                        CONTENTBROWSERPANEL.H
//============================================================================================================================================
// 🧩 Records the content browser exactly as `AsstbrowsrBasic.html` presents it — sources aside, a lattice of records, and an inspector.

#pragma once

#include "Contract/DeliveryContract.h"
#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "SlateUI/Interface/DrawerSpace/Api/DrawerSpace.h"
#include "SlateUI/Interface/InteractionIndex/Api/InteractionIndex.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE SEATED INKS
//------------------------------------------------------------------------------------------------------------------------

// 📝 `ContentBrowserInk` now lives in `AppearanceSpecification.h`, beside every other ink the interface draws
//    with. It moved so the appearance file can reach it: a token run declared in a panel header is one the
//    Control Centre cannot theme. The spellings are unchanged.

/// 🧩 Every length `AsstbrowsrBasic` states, at the artist's own scale.
/// tag   contract, nonallocating, nonthrowing
struct ContentBrowserMetric
{
    float  AsideAlong        = 240.0f;   // [px] - w-60, the sources column
    float  InspectorAlong    = 320.0f;   // [px] - w-80, the inspector column
    float  TopRailAcross     =  56.0f;   // [px] - h-14, the seek rail
    float  BreadcrumbAcross  =  41.0f;   // [px] - p-4 pb-2 over a 12px run
    float  SeekAcross        =  32.0f;   // [px] - h-8, the seek field and both actions
    float  SeekAlong         = 448.0f;   // [px] - max-w-md
    float  SourceRowAcross   =  30.0f;   // [px] - py-1.5 over a 12px run
    float  SourceStepAlong   =  24.0f;   // [px] - pl-6, one nesting step
    float  CaptionAcross     =  24.0f;   // [px] - the tracking-widest section caption
    float  CardGap           =  12.0f;   // [px] - gap-3
    float  CardPad           =  16.0f;   // [px] - p-4, the lattice's own inset
    float  CardCaptionAcross =  46.0f;   // [px] - p-2.5 over two runs
    float  PreviewAcross     = 192.0f;   // [px] - h-48, the inspector's preview
    float  TongueAcross      =  48.0f;   // [px] - p-2 over an h-8 tongue
    float  CrestAlong        =  40.0f;   // [px] - w-10 h-10, the inspector's crest
    float  ChipAcross        =  20.0f;   // [px] - the extension chip and every tag
    float  ImportAcross      =  36.0f;   // [px] - h-9, the inspector's import action
    float  RadiusCard        =  12.0f;   // [px] - rounded-xl
    float  RadiusPlate       =   8.0f;   // [px] - rounded-lg
    float  RadiusSoft        =   6.0f;   // [px] - rounded-md
    float  RunCaption        =  10.0f;   // [pt] - text-[10px]
    float  RunBody           =  12.0f;   // [pt] - text-xs
    float  RunCrest          =  13.0f;   // [pt] - text-[13px]
};

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SEATED RECORDS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What a content record is classed under. The reference's `CAT_INFO` run, in its own order.
/// note  📐 The spelling is `Archive` rather than the reference's `cat`, because the drawn caption is a
///        heading over a run of records and the identifier must state what it discriminates.
/// tag   contract
enum class ContentArchive : std::uint32_t
{
    Topology     = 0u,   // [-] - CAT_INFO.mesh, "Meshes"
    Draughting   = 1u,   // [-] - CAT_INFO.cad, "CAD"
    Arrangement  = 2u,   // [-] - CAT_INFO.scene, "Scenes"
    Material     = 3u,   // [-] - CAT_INFO.material, "Materials"
    Typeface     = 4u,   // [-] - CAT_INFO.font, "Fonts"
    Vector       = 5u,   // [-] - CAT_INFO.svg, "SVG / Vectors"
    ArchiveCount = 6u    // [-] - the closed count, never an archive
};

/// 🧩 One record in the library, as `ASSETS` declares it.
/// tag   contract, nonallocating, nonthrowing
struct ContentRecord
{
    const char*     Naming      = nullptr;                      // [-] - borrowed; `name`, without its extension
    const char*     Extension   = nullptr;                      // [-] - borrowed; `ext`
    const char*     Subheading  = nullptr;                      // [-] - borrowed; `subcat`, nullptr when none
    const char*     Tags[3]     = {};                           // [-] - borrowed; `tags`, NUL-terminated run
    std::uint32_t   TagCount    = 0u;                           // [-] - how many of Tags stand
    double          Octets      = 0.0;                          // [B] - `size`, stated in bytes as the reference does
    ContentArchive  Archive     = ContentArchive::Topology;     // [-] - `cat`
};

/// 🧩 The seated library and what the artist has taken from it.
/// note  🔴 `AbsentOrdinal` and not a signed ordinal. Every ordinal in this unit is unsigned, so an absent
///        take must be a stated sentinel rather than a negative that cannot be represented.
/// tag   contract, nonallocating, nonthrowing
struct ContentLibrary
{
    static constexpr std::uint32_t RecordCeiling  = 64u;          // [-] - seated records; never allocated
    static constexpr std::uint32_t AbsentOrdinal  = 0xFFFFFFFFu;  // [-] - nothing taken, nothing traversed

    ContentRecord   Records[RecordCeiling] = {};                  // [-] - the library in its declared order
    std::uint32_t   RecordCount            = 0u;                  // [-] - how many of Records stand
    std::uint32_t   Taken                  = AbsentOrdinal;       // [-] - `state.selectedId`
    std::uint32_t   TraversedArchive       = AbsentOrdinal;       // [-] - `state.cat`
    const char*     TraversedSubheading    = nullptr;             // [-] - `state.subcat`, borrowed
};

/// 🧩 Seats the reference's own `ASSETS` run into a library, so the panel presents what the prototype does.
/// in    Seating  [-]  written whole; whatever stood in it is replaced
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
void SeatReferenceContent(ContentLibrary& Seating);

/// 🧩 What the archive is captioned and which symbol crests it, as `CAT_INFO` pairs them.
/// in    Archive  [-]  a stated archive; ArchiveCount yields the placeholder pair
/// out   Naming   [-]  borrowed static text, never null
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char*   ArchiveNaming(ContentArchive Archive);
SymbolSubject ArchiveCrest(ContentArchive Archive);

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PANEL'S ORDINATES
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Everything the panel carries between ticks that is not the library itself.
/// tag   contract, nonallocating, nonthrowing
struct ContentBrowserOrdinates
{
    static constexpr std::uint32_t SeekCeiling = 64u;   // [-] - the seek run, in octets

    char           Seek[SeekCeiling] = {};      // [-] - `state.search`, NUL-terminated
    bool           SeekHolding       = false;   // [-] - the seek field holds the keyboard
    float          LatticeOffset     = 0.0f;    // [px] - how far the lattice is scrolled
    float          LatticeSpan       = 0.0f;    // [px] - what the lattice measured last tick
    float          AsideOffset       = 0.0f;    // [px] - how far the sources column is scrolled
    float          AsideSpan         = 0.0f;    // [px] - what the sources column measured last tick
    std::uint32_t  InspectorTongue   = 0u;      // [-] - 0 Details, 1 Create
    const char*    Tooltip           = nullptr; // [-] - borrowed; recorded in the deferred sweep
    float          TooltipAlong      = 0.0f;    // [px]
    float          TooltipAcross     = 0.0f;    // [px]
};

//------------------------------------------------------------------------------------------------------------------------
//                                                          THE PANEL
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Records the content browser and arbitrates every contact it presents.
/// note  🔴 The panel enrols into a ledger it does not own. The host that constructs it must weigh
///        `EnrolmentDemand` against the ledger's remaining capacity before it constructs anything else.
/// tag   api, nonallocating, nonthrowing
class ContentBrowserPanel
{
public:

    static constexpr std::uint32_t SourceCeiling  = 24u;   // [-] - source rows, archives and subheadings
    static constexpr std::uint32_t LatticeCeiling = 64u;   // [-] - lattice records, one control each
    static constexpr std::uint32_t ChromeCeiling  = 12u;   // [-] - seek, both actions, both tongues, import, thumbs

    /// 🔴 What this panel takes out of the shared ledger. The host's own static_assert weighs the page's
    ///    whole demand against the ledger's capacity; this constant is the panel's contribution to it.
    static constexpr std::uint32_t EnrolmentDemand = SourceCeiling + LatticeCeiling + ChromeCeiling;

    /// 🧩 Claims every identity the panel will ever arbitrate, once, before the first tick.
    /// in    Interaction  [-]  borrowed; must outlive the panel
    /// in    Recording    [-]  borrowed; must outlive the panel
    /// out   Deliver      [-]  refuses with ContentUnsupported when a construction already stands, and
    ///                          carries the ledger's own refusal when a slot cannot be claimed
    /// err   a refusal leaves nothing enrolled; the panel records nothing until Construct is delivered
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    Deliver<bool> Construct(InteractionIndex& Interaction, RecordingSurface& Recording);

    /// 🧩 Samples the tick's pointer before anything is recorded against it.
    /// in    Arrived  [-]  this tick's pointer, as the host built it
    /// in    Elapsed  [s]  since the previous tick
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Advance(const PointerCondition& Arrived, double Elapsed);

    /// 🧩 Records the whole browser — sources, seek rail, lattice and inspector — into one extent.
    /// in    Extent   [px] the page the browser occupies whole
    /// in    Library  [-]  amended in place as the artist takes and traverses
    /// in    Seated   [-]  amended in place; carries the seek run and both offsets
    /// cost  🚩
    /// tag   api, nonallocating, nonthrowing
    void RecordBrowser(const PlaneExtent& Extent, ContentLibrary& Library, ContentBrowserOrdinates& Seated);

    /// 🧩 Records the tooltip above everything the tick has already recorded.
    /// in    Seated  [-]  read for the tooltip the rouse seated, then cleared
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void RecordDeferred(ContentBrowserOrdinates& Seated);

    /// 🧩 Admits a typed octet into the seek run when the seek field holds the keyboard.
    /// in    Arrived  [-]  the octet, as the interface reported it
    /// in    Seated   [-]  amended in place
    /// out   Admitted [-]  false when the field does not hold the keyboard
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    bool AdmitTyped(char Arrived, ContentBrowserOrdinates& Seated);

    /// 🧩 Retires the last octet of the seek run when the seek field holds the keyboard.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    bool RetractTyped(ContentBrowserOrdinates& Seated);

    /// 🧩 Withholds everything the browser arbitrated this tick from initiating a drawer drag.
    /// in    Drawers  [-]  amended in place; the exclusions are declared against Bearing
    /// in    Bearing  [-]  which drawer the browser was recorded inside
    /// note  ⚠️ Declared per tick, immediately after RecordBrowser and before the drawer advances. The
    ///       standing set is cleared at the head of every RecordBrowser, so a tick that records the
    ///       browser and does not call this leaves the drawer free to be dragged by its own cards.
    /// note  🔴 Without this the drawer owns every contact inside its body, so pressing a record, dragging
    ///       the lattice thumb or typing in the seek field slides the whole drawer instead. That is what
    ///       `ControlCentrePanel::Exclude` exists for on the north drawer, and the south one needs it just
    ///       as much — more, since the browser is the denser of the two in controls.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Exclude(DrawerSpace& Drawers, DrawerBearing Bearing) const;

    /// 🧩 Returns the panel to its unconstructed condition, enrolling nothing.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Reset();

    /// 🧩 Restates the panel's inks and lengths from a resolved appearance, so a theme change reaches it.
    /// in    Resolved  [-]  the appearance the host resolved for the chosen theme
    /// note  📐 Cheap enough to call every tick, though a host need only call it when the selection moves.
    ///        Nothing is borrowed — the record is copied out, so the caller may let `Resolved` expire.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Reseat(const AppearanceSpecification& Resolved);

    ContentBrowserInk     Ink;        // [-] - the reference's own token run
    ContentBrowserMetric  Measure;    // [-] - the reference's own lengths

private:

    void  RetainExclusion(const PlaneExtent& Extent);

    bool  Roused(const PlaneExtent& Extent) const;
    bool  Pressed(ControlIdentity Claimed, const PlaneExtent& Extent,
                  ContentBrowserOrdinates& Seated, const char* Tooltip = nullptr);

    void  RecordSources(const PlaneExtent& Extent, ContentLibrary& Library, ContentBrowserOrdinates& Seated);
    void  RecordSeekRail(const PlaneExtent& Extent, ContentBrowserOrdinates& Seated);
    void  RecordLattice(const PlaneExtent& Extent, ContentLibrary& Library, ContentBrowserOrdinates& Seated);
    void  RecordInspector(const PlaneExtent& Extent, ContentLibrary& Library, ContentBrowserOrdinates& Seated);
    void  RecordHatch(const PlaneExtent& Extent);
    void  RecordScrollbar(const PlaneExtent& Extent, ControlIdentity Claimed, float Span, float& Offset);

    /// 🔴 Whether a record survives the traversal and the seek run, exactly as `renderGrid` filters it.
    bool  Retained(const ContentRecord& Record, const ContentLibrary& Library,
                   const ContentBrowserOrdinates& Seated) const;

    InteractionIndex*  Ledger  = nullptr;   // [-] - borrowed, never owned
    RecordingSurface*  Surface = nullptr;   // [-] - borrowed, never owned

    ControlIdentity  SourceRows[SourceCeiling]    = {};   // [-] - one per source row
    ControlIdentity  LatticeCards[LatticeCeiling] = {};   // [-] - one per lattice record
    ControlIdentity  ChromeCells[ChromeCeiling]   = {};   // [-] - the rail, the tongues, both thumbs

    PointerCondition  Sampled;                            // [-] - this tick's pointer

    /// 🔴 One extent per control the tick arbitrated, so a drawer can be told what not to drag by. The
    ///    ceiling is the enrolment demand because that is the most controls one tick can record.
    PlaneExtent    Exclusions[EnrolmentDemand] = {};      // [px] - display ordinates, this tick's
    std::uint32_t  ExclusionCount              = 0u;      // [-]  - cleared at the head of RecordBrowser
};

}   // namespace Slate
