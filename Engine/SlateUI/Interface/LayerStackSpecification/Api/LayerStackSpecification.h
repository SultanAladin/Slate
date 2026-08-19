//============================================================================================================================================
//                                                     LAYERSTACKSPECIFICATION.H
//============================================================================================================================================
// 🧩 The parametric schema behind the texture-paint layer stack — a nested arrangement of layers, their attached masks and their channels.

#pragma once

#include "Contract/DeliveryContract.h"
#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE CLOSED COUNTS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every ceiling the stack is bounded by, stated once so no recorder invents its own.
/// note  📝 The arrangement is nested, so it is held as one flat run of records carrying an enclosing
///        ordinal rather than as linked storage. Nothing in the stack allocates: the whole schema is one
///        fixed extent the host owns, which is what keeps a tick free of `new`.
/// tag   contract
struct LayerStackCeiling
{
    static constexpr std::uint32_t Entries        = 128u;   // [-] - layers plus folders, whole arrangement
    static constexpr std::uint32_t Depth          =   8u;   // [-] - how deep folders may nest
    static constexpr std::uint32_t Channels       =   8u;   // [-] - the reference's fixed channel run
    static constexpr std::uint32_t Effects        =   8u;   // [-] - effects retained per layer or mask
    static constexpr std::uint32_t Parameters     =  12u;   // [-] - parameters one source or generator states
    static constexpr std::uint32_t MeshMaps       =   6u;   // [-] - mesh maps one generator reads
    static constexpr std::uint32_t NamingCeiling  =  48u;   // [-] - characters retained for one naming
    static constexpr std::uint32_t Revisions      =  64u;   // [-] - the revision run the inspector presents
    static constexpr std::uint32_t AtlasTotal     =   5u;   // [-] - ATLAS_TOTAL, what the channel panel foots
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  WHAT ONE ENTRY IS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What one entry of the arrangement is, which decides its badge, its summary run and its card.
/// note  📐 `LayerstackV1`'s own `TYPE` record. The reference spells the discriminator `type`, which is a
///        banned spelling; the discriminating mechanism is what the entry *contains*, so it is spelled
///        `LayerContent` here and the enumerators keep the reference's own words.
/// tag   contract
enum class LayerContent : std::uint32_t
{
    Folder       = 0u,   // [-] - encloses further entries; blends passthrough
    Paint        = 1u,   // [-] - accepts brush deposits
    Fill         = 2u,   // [-] - floods the whole atlas
    Adjustment   = 3u,   // [-] - re-maps what lies beneath
    Retention    = 4u,   // [-] - the reference's `filter` entry, spelled by its mechanism
    Decal        = 5u,   // [-] - a placed three-dimensional impression
    Pattern      = 6u,   // [-] - a procedural repeat
    ContentCount = 7u    // [-] - the closed count, never a content
};

/// 🧩 Which half of one entry the artist has taken — the reference's `selMask`.
/// tag   contract
enum class LayerTaken : std::uint32_t
{
    Layer = 0u,   // [-] - the entry itself; its channels reach the Channel panel
    Mask  = 1u    // [-] - its attached mask; its parameters reach the Mask panel
};

/// 🧩 Where one mask draws its coverage from — the reference's `SRC` record.
/// tag   contract
enum class MaskSource : std::uint32_t
{
    Paint          = 0u,   // [-] - brush deposits
    Bitmap         = 1u,   // [-] - a sampled picture
    BakedMap       = 2u,   // [-] - one transferred mesh map
    PolygonFill    = 3u,   // [-] - a selected run of polygons
    ColourSelection = 4u,  // [-] - matched identity colour
    AnchorPoint    = 5u,   // [-] - another entry's resolved coverage
    Generator      = 6u,   // [-] - a procedural generator
    SourceCount    = 7u    // [-] - the closed count, never a source
};

/// 🧩 One channel's own arrangement within an entry — the reference's `defCh()` record.
/// tag   contract, nonallocating, nonthrowing
struct ChannelOrdinate
{
    bool           Enabled = true;        // [-] - the dot; a disabled channel ignores this entry
    const char*    Blend   = "Normal";    // [-] - borrowed; the blend mode's own name
    std::uint32_t  Opacity = 100u;        // [%] - 0…100
};

/// 🧩 One parameter a source or generator states, resolved for presentation.
/// note  📝 The reference carries three parameter shapes — a range, a toggle and a selection. One record
///        with a discriminating extent states all three without a variant, which keeps the run flat.
/// tag   contract, nonallocating, nonthrowing
struct ParameterOrdinate
{
    const char*    Naming    = "";       // [-] - borrowed; the parameter's presented label
    double         Standing  = 0.0;      // [-] - the seated reading
    double         Least     = 0.0;      // [-] - the range floor
    double         Most      = 100.0;    // [-] - the range ceiling
    const char*    Unit      = "%";      // [-] - borrowed; presented after the reading
    bool           Toggling  = false;    // [-] - presented as a switch rather than a range
    const char*    Selected  = nullptr;  // [-] - borrowed; non-null presents a selection instead
};

/// 🧩 One mask attached to one entry, exactly as the reference's `MASK()` declares it.
/// tag   contract, nonallocating, nonthrowing
struct MaskOrdinate
{
    bool           Declared    = false;                // [-] - whether a mask stands at all
    bool           Shown       = true;                 // [-] - the mask row's own eye
    MaskSource     Source      = MaskSource::Paint;    // [-] - src
    const char*    Generator   = "Metal Edge Wear";    // [-] - borrowed; meaningful while Source is Generator
    const char*    Blend       = "Multiply";           // [-] - borrowed; how the coverage lands
    std::uint32_t  Density     = 100u;                 // [%] - den
    bool           Inverted    = false;                // [-] - invert
    bool           Unfolded    = false;                // [-] - its card stands open
    std::uint32_t  Resolution  = 2048u;                // [px] - square
    bool           BaseWhite   = true;                 // [-] - the base coverage; false seats black

    const char*        Effects[LayerStackCeiling::Effects] = {};      // [-] - borrowed
    std::uint32_t      EffectCount = 0u;                              // [-] - how many stand

    ParameterOrdinate  Parameters[LayerStackCeiling::Parameters] = {};   // [-] - the source's own
    std::uint32_t      ParameterCount = 0u;                              // [-] - how many stand

    const char*        MeshMaps[LayerStackCeiling::MeshMaps] = {};     // [-] - borrowed; required inputs
    bool               MeshMapTransferred[LayerStackCeiling::MeshMaps] = {};   // [-] - whether each stands
    std::uint32_t      MeshMapCount = 0u;                              // [-] - how many are required

    bool           ChannelApplied[LayerStackCeiling::Channels] = { true, true, true, true,
                                                                   true, true, true, true };   // [-] - chan
};

/// 🧩 One entry of the arrangement — a layer or a folder — with its mask and its channels.
/// note  📝 The nesting is carried by `Enclosing` and `Depth` rather than by held storage, so the whole
///        arrangement is one flat run a recorder walks in order.
/// tag   contract, nonallocating, nonthrowing
struct LayerEntry
{
    char           Naming[LayerStackCeiling::NamingCeiling] = {};   // [-] - retained, not borrowed
    LayerContent   Content     = LayerContent::Paint;    // [-] - what the entry holds
    const char*    Blend       = "Normal";               // [-] - borrowed; the blend mode's own name
    std::uint32_t  Opacity     = 100u;                   // [%] - 0…100
    std::uint32_t  ColourTag   = 0x8AB4D8u;              // [-] - the spine and the card share it
    bool           Shown       = true;                   // [-] - vis
    bool           Secured     = false;                  // [-] - lock
    bool           Unfolded    = false;                  // [-] - its own card stands open
    bool           Opened      = true;                   // [-] - a folder presenting what it encloses
    std::uint32_t  Depth       = 0u;                     // [-] - nesting steps from the outermost
    std::uint32_t  Enclosing   = 0xFFFFFFFFu;            // [-] - the folder holding it; absent when outermost
    std::uint32_t  Resolution  = 2048u;                  // [px] - square
    const char*    Format      = "RGBA 8";               // [-] - borrowed; bit depth's own name
    const char*    Modified    = "2026-08-19 14:02";     // [-] - borrowed; presented in the info section

    ChannelOrdinate  Channels[LayerStackCeiling::Channels] = {};    // [-] - one per declared channel

    const char*      Effects[LayerStackCeiling::Effects] = {};      // [-] - borrowed
    std::uint32_t    EffectCount = 0u;                              // [-] - how many stand

    MaskOrdinate     Mask;                                          // [-] - attached; Declared gates it

    // 📝 The height-into-normal re-integration the reference states on every non-folder card.
    bool           HeightIntegrated = true;                  // [-] - h2n.on
    const char*    HeightBlend      = "Normal Map Detail";   // [-] - borrowed; h2n.mode
    std::uint32_t  HeightIntensity  = 100u;                  // [%] - h2n.int, 0…200
    bool           HeightTessellated = false;                // [-] - h2n.tess
};

/// 🧩 One committed revision the inspector's second pane presents.
/// note  📐 `Inspector.tsx` calls these `revisions`; the naming record retires `HistoryStack` to
///        `RevisionSequence`, so the same spelling is used here. The pane's visible caption stays as the
///        reference draws it.
/// tag   contract, nonallocating, nonthrowing
struct RevisionOrdinate
{
    const char*    Naming   = "";   // [-] - borrowed; what changed
    const char*    Moment   = "";   // [-] - borrowed; when it was sealed
    const char*    Detail   = "";   // [-] - borrowed; the reading it moved to
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE WHOLE ARRANGEMENT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The whole layer arrangement the host owns and every recorder borrows for one tick.
/// note  💾 Pointer-free and fixed in extent, so it copies by assignment — which is exactly how the
///        revision ring retains a restorable reading without allocating.
/// tag   contract, nonallocating, nonthrowing
struct LayerArrangement
{
    LayerEntry     Entries[LayerStackCeiling::Entries];   // [-] - the flat run, outermost first
    std::uint32_t  EntryCount = 0u;                       // [-] - how many stand

    std::uint32_t  Taken       = 0u;                      // [-] - which entry the artist has taken
    LayerTaken     TakenHalf   = LayerTaken::Layer;       // [-] - which half of it
    std::uint32_t  Soloed      = 0xFFFFFFFFu;             // [-] - absent when nothing is soloed
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE CLOSED RUNS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The channel naming run, in the reference's own order.
/// out   const char* const*  [-]  borrowed; exactly LayerStackCeiling::Channels entries
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* const* ChannelNaming();

/// 🧩 The tint one channel carries, from the reference's own record.
/// in    Ordinal  [-]  0…LayerStackCeiling::Channels-1; out of range resolves to the first
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
InkOrdinate ChannelTint(std::uint32_t Ordinal);

/// 🧩 The tint one content carries, from `LayerstackV1`'s own `TYPE` record.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
InkOrdinate ContentTint(LayerContent Content);

/// 🧩 The run naming one content, as the reference presents it.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* ContentNaming(LayerContent Content);

/// 🧩 The single-character badge one content carries in its thumbnail.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* ContentBadge(LayerContent Content);

/// 🧩 The run naming one mask source, as the reference presents it.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* SourceNaming(MaskSource Source);

/// 🧩 The blend-mode run one channel accepts — Normal and Height state their own shorter runs.
/// in    ChannelOrdinal  [-]  which channel is asking
/// out   Count           [-]  how many entries the returned run carries
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* const* BlendNaming(std::uint32_t ChannelOrdinal, std::uint32_t& Count);

//------------------------------------------------------------------------------------------------------------------------
//                                                   WALKING THE NESTING
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Whether one entry stands presented — every folder enclosing it is open.
/// in    Arrangement  [-]  the whole run
/// in    Ordinal      [-]  which entry
/// out   bool         [-]  false when any enclosing folder is closed
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
bool EntryPresented(const LayerArrangement& Arrangement, std::uint32_t Ordinal);

/// 🧩 How many entries one folder encloses, at every depth beneath it.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
std::uint32_t EnclosedCount(const LayerArrangement& Arrangement, std::uint32_t Ordinal);

/// 🧩 How many channels of one entry stand enabled.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
std::uint32_t ChannelsEnabled(const LayerEntry& Entry);

/// 🧩 Seats the arrangement the reference's own `tree` declares, so every host opens on one reading.
/// out   Deliver  [-]  refuses with ExtentExhausted when the declared run exceeds the ceiling
/// cost  🚩
/// tag   api, nonallocating, nonthrowing
Deliver<bool> SeatReferenceArrangement(LayerArrangement& Arrangement);

/// 🧩 Seats the revision run the inspector's second pane presents.
/// out   Count  [-]  how many revisions were seated
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
void SeatReferenceRevisions(const RevisionOrdinate*& Revisions, std::uint32_t& Count);

}   // namespace Slate
