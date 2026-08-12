//============================================================================================================================================
//                                                              GLYPHDEPOT.H
//============================================================================================================================================
// 🧩 Vector glyphs rasterised once and keyed by content — two names sharing bytes share one texture, and no vendor identity escapes.

#pragma once

#include "Contract/OutcomeContract.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE GLYPH HANDLE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 An opaque reference to one uploaded glyph, resolved to a vendor texture identity inside the source file alone.
/// note  🔴 `14` §7: no vendor spelling crosses a public header, and a texture identity is a vendor spelling. This is
///        an integer whose meaning only `GlyphDepot.cpp` knows, exactly as `InterfaceExchange` holds its context.
/// tag   contract, nonallocating, nonthrowing
struct GlyphHandle
{
    std::uint64_t  DepotSlot = 0u;   // [-] - zero declares the handle absent; issued values begin at one

    /// 🧩 Whether this handle names an uploaded glyph at all.
    /// cost  ✔️
    constexpr bool HandleDeclared() const { return DepotSlot != 0u; }
};

//------------------------------------------------------------------------------------------------------------------------
//                                                   WHAT THE DEPOT ATTACHES TO
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The device handles one upload needs, supplied once at bring-up.
/// note  🔴 The depot never re-queries these and never owns them. `14` §6 forbids allocating a device resource per
///        recording, and every allocation here happens inside `Declare` — which is bring-up, never a tick.
/// tag   nonallocating, nonthrowing
struct GlyphAttachment
{
    VkPhysicalDevice  ScoredDevice          = VK_NULL_HANDLE;   // [-]  - the device VendorClassifier won
    VkDevice          ActiveDevice          = VK_NULL_HANDLE;   // [-]  - the created device
    VkQueue           GraphicsQueue         = VK_NULL_HANDLE;   // [-]  - the queue the staging transfer is taken on
    std::uint32_t     GraphicsFamilyOrdinal = 0u;               // [-]  - the family that queue sits in
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    ONE DECLARED GLYPH
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One glyph as a tier declares it — its key, its vector source, and the square it rasterises into.
/// note  The source is a complete vector document in bytes. It is read during `Declare` and never retained, so the
///        span may name a string literal in the declaring translation unit.
/// tag   nonallocating, nonthrowing
struct GlyphDeclaration
{
    const char*    GlyphKey      = nullptr;   // [-]  - the name a panel resolves; never empty
    const char*    VectorSource  = nullptr;   // [-]  - a complete vector document, read during Declare only
    std::uint32_t  SourceExtent  = 0u;        // [-]  - bytes of VectorSource, terminator excluded
    std::uint32_t  RasterEdge    = 0u;        // [px] - the square edge; zero takes the depot's declared edge
};

/// 🧩 A named set of glyphs one workspace declares at activation.
/// note  🔴 `14` §1: a workspace's own glyphs are declared beside its panels, so adding a workspace adds a tier and
///        edits nothing shared. The chrome tier is declared once and read by every workspace; content-hash dedup
///        means a glyph two tiers share uploads once and is torn down when the second releases it.
/// tag   nonallocating, nonthrowing
struct GlyphTier
{
    const char*              TierName      = nullptr;   // [-] - what the workspace calls its set
    const GlyphDeclaration*  Declarations  = nullptr;   // [-] - the declarations, read during Declare only
    std::uint32_t            DeclaredCount = 0u;        // [-] - how many
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE DEPOT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 A store of rasterised, evictable, reconstructible glyphs keyed by the content that produced them.
/// note  🔴 The two-level indirection — key to content hash, content hash to uploaded texture — is what lets two
///        tiers share a glyph while each keeps its own name. Collapsing it to key-to-texture uploads the chrome
///        twisty once per workspace, and the count grows with the workspace count rather than with the art.
/// note  ⚠️ Every declaration rasterises and uploads. `14` §6 forbids that during recording, so a tier is declared
///        at workspace activation and a resolution during a tick never allocates.
/// tag   owning
class GlyphDepot
{
public:

    GlyphDepot()                             = default;
    GlyphDepot(const GlyphDepot&)            = delete;
    GlyphDepot& operator=(const GlyphDepot&) = delete;
    ~GlyphDepot();

    /// 🧩 Attaches the depot to a device and brings the vector rasteriser up.
    /// in    Arriving  [-]  the device handles the uploads use
    /// out   Outcome   [-]  refuses with CapabilityAbsent when a required handle is absent, and with HostDenied
    ///                      when the rasteriser declines to start
    /// post  the depot holds nothing; every tier is declared afterwards
    /// cost  🔴
    /// tag   api, nonthrowing
    Outcome<bool> Construct(const GlyphAttachment& Arriving);

    /// 🧩 Destroys every uploaded glyph and releases the rasteriser.
    /// note  The device is waited idle first: a texture identity a recording still references is one the vendor
    ///        reads after it was freed, and the read succeeds often enough to look like a different defect.
    /// cost  🔴
    /// tag   api, nonthrowing
    void Reclaim();

    /// 🧩 Declares one whole tier, rasterising and uploading what is not already held.
    /// in    Declaring  [-]  the tier, its declarations read here and never retained
    /// out   Outcome    [-]  refuses with ContentUnsupported naming the first key whose source would not rasterise,
    ///                       and with ExtentExhausted when an upload could not be claimed
    /// post  every key that delivered resolves; a refused tier leaves the keys before the refusal standing
    /// note  🔴 The refusal names **which** key failed. A bare refusal over a partly-filled depot sends the reader
    ///        to compare a tier's declaration list against what resolves, one key at a time.
    /// cost  🔴
    /// tag   api, nonthrowing
    Outcome<bool> Declare(const GlyphTier& Declaring);

    /// 🧩 Releases one tier's claim on its glyphs, tearing down those no other tier still names.
    /// in    Releasing  [-]  the tier as it was declared
    /// out   Outcome    [-]  refuses with IdentityStale when the tier was never declared
    /// post  a glyph another standing tier shares is untouched
    /// cost  🚩
    /// tag   api, nonthrowing
    Outcome<bool> Release(const GlyphTier& Releasing);

    /// 🧩 Resolves one key to its handle.
    /// in    GlyphKey  [-]  the key a tier declared
    /// out   Outcome   [-]  refuses with ContentUnsupported when nothing declares that key
    /// note  What a row calls each time it presents. Never uploads and never mutates the depot.
    /// cost  🚩
    /// tag   api, nonthrowing
    Outcome<GlyphHandle> Resolve(const std::string& GlyphKey) const;

    /// 🧩 Whether one key resolves to an uploaded glyph.
    /// cost  🚩
    /// tag   api, nonthrowing
    bool GlyphHeld(const std::string& GlyphKey) const;

    /// 🧩 The square edge a declaration that names none rasterises into.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    std::uint32_t DeclaredEdge() const;

    /// 🧩 Declares the square edge a declaration that names none rasterises into.
    /// in    RasterEdge  [px] the edge; refused at zero and above the depot's ceiling
    /// out   Outcome     [-]  refuses with ContentUnsupported outside the admitted interval
    /// note  Applies to declarations made after it. A glyph already uploaded keeps the edge it was rasterised at.
    /// cost  ✔️
    /// tag   api, nonthrowing
    Outcome<bool> DeclareEdge(std::uint32_t RasterEdge);

    /// 🧩 How many distinct textures the depot holds — what dedup is measured against.
    /// note  🔍 Fewer than the resolvable key count exactly when two tiers share a glyph, which is the arrangement
    ///        the two-level indirection exists for.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    std::uint32_t UploadedCount() const;

    /// 🧩 How many keys resolve.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    std::uint32_t KeyCount() const;

private:

    /// 🧩 One uploaded glyph — the device chain and the vendor descriptor that names it as a texture.
    /// note  Several keys may name one of these, so it counts the keys that do and is torn down at zero.
    struct UploadedGlyph
    {
        VkImage          DeviceImage    = VK_NULL_HANDLE;   // [-]  - holds the rasterised texels
        VkDeviceMemory   ImageExtent    = VK_NULL_HANDLE;   // [-]  - backing allocation for DeviceImage
        VkImageView      ColourView     = VK_NULL_HANDLE;   // [-]  - the view the descriptor binds
        VkSampler        LinearSampler  = VK_NULL_HANDLE;   // [-]  - linear, clamped to the edge
        VkDescriptorSet  DescriptorSlot = VK_NULL_HANDLE;   // [-]  - what the vendor identity resolves to
        std::uint64_t    TextureSlot    = 0u;               // [-]  - DescriptorSlot as the vendor's identity
        std::uint32_t    RasterEdge     = 0u;               // [px] - the square edge it was rasterised at
        std::uint32_t    NamingCount    = 0u;               // [-]  - keys naming it; torn down at zero
    };

    std::uint64_t                                       ContentHash(const char*   VectorSource,
                                                                    std::uint32_t SourceExtent,
                                                                    std::uint32_t RasterEdge) const;
    Outcome<std::uint64_t>                              Upload(const GlyphDeclaration& Declaring,
                                                               std::uint32_t           RasterEdge);
    void                                                Withdraw(std::uint64_t ContentIdentity);

    GlyphAttachment                                     Attached          = {};    // [-]  - as supplied, never re-queried
    std::unordered_map<std::string, std::uint64_t>      KeyedContent      = {};    // [-]  - key -> content hash
    std::unordered_map<std::uint64_t, UploadedGlyph>    HeldGlyphs        = {};    // [-]  - content hash -> texture
    std::unordered_map<std::string, std::vector<std::string>>
                                                        TieredKeys        = {};    // [-]  - tier name -> its keys
    std::uint32_t                                       DefaultEdge       = 32u;   // [px] - edge a declaration omitting one takes
    bool                                                RasteriserStanding = false; // [-] - the vector engine is up
};

}   // namespace Slate
