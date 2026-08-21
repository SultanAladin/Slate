//============================================================================================================================================
//                                                         TEXTUREPAINTCONTRACT.H
//============================================================================================================================================
// 🧩 The editor's texture-paint layer stack — the shared contract between the
//    host's data and the TexturePaintPanel's presentation: what a layer is,
//    what a mask is, and which filter category each belongs to.
//
//    This is the EDITOR's layer stack (the LayerstackV1 / ChannelPropertyPanel
//    references), a dedicated sibling of SceneDirectoryPanel: the stack page
//    shows every layer's small details (badge, name, blend, opacity, chips),
//    and Tab slides to a selection-driven properties page — channel properties
//    for a layer, the mask panel for a mask, decal/pattern/generator settings
//    for those kinds, and the combined stack properties for a folder. No
//    history panel: the properties page is where the details live.
//
//    🔴 The validation shell keeps its OWN layer stack in LayerStackPanel.h —
//    this contract is the editor's, deliberately richer (folders, tags,
//    sources, per-row detail runs). Do not merge the two; the validation
//    layer stack is the prototype and this is the editor's home.

#pragma once

#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "SlateUI/Interface/SymbolSpecification/Api/SymbolSpecification.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE CLASSIFICATIONS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What one layer in the stack is, which decides its glyph, its hue and its filter category.
/// note  📐 The reference's `TYPE` record from `LayerstackV1.html`, in its own order: paint, fill,
///        adjustment, filter, folder, decal, pattern — with `Generator` and `Material` appended, as
///        the ChannelPropertyPanel reference presents them.
/// tag   contract
enum class TextureLayerClassification : std::uint32_t
{
    Paint       = 0u,   // [-] - brush strokes over the atlas
    Fill        = 1u,   // [-] - a solid or gradient fill
    Decal       = 2u,   // [-] - a 3D-placed decal entity
    Pattern     = 3u,   // [-] - a tiled procedural pattern
    Generator   = 4u,   // [-] - a mesh-map driven generator
    Adjustment  = 5u,   // [-] - a colour/effect adjustment
    Filter      = 6u,   // [-] - a blur/level/effect filter
    Folder      = 7u,   // [-] - a group holding layers
    Material    = 8u,   // [-] - a whole material fill
    SubjectCount = 9u   // [-] - the closed count, never a classification
};

/// 🧩 The glyph one layer classification is drawn with (dummy icons, like the references').
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
SymbolSubject TextureLayerGlyph(TextureLayerClassification Classified);

/// 🧩 The hue one layer classification carries, from the reference's swatches.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
ThemeToken TextureLayerHue(TextureLayerClassification Classified);

/// 🧩 The run naming one layer classification.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* TextureLayerText(TextureLayerClassification Classified);

/// 🧩 Which of the stack's filter categories a layer belongs to.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
std::uint32_t TextureLayerFacetOf(TextureLayerClassification Classified);

/// 🧩 Which half of a layer the artist has taken — the layer itself or its attached mask.
/// tag   contract
enum class TextureLayerTarget : std::uint32_t
{
    Layer = 0u,   // [-] - the layer row
    Mask  = 1u    // [-] - the mask row attached beneath it
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE ROWS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The eight texture channels the ChannelPropertyPanel reference presents, in its own order.
/// tag   contract
inline constexpr std::uint32_t TextureChannelCeiling = 8u;   // [-] - Base Color … Opacity

/// 🧩 One row of the layer stack — the editor's own shape, richer than the shell's: folders carry
///    depth and enclosure, layers carry a small detail run, tags feed the search, and the mask's
///    source and density are stated on the row.
/// note  📝 Borrowed for the tick, exactly as `EntityRow` is: names, blends, channels, details and
///        tags are pointer runs the host owns and keeps alive.
/// tag   contract, nonallocating, nonthrowing
struct TextureLayerRow
{
    const char*         Naming       = "";                     // [-] - borrowed; outlives the tick
    TextureLayerClassification Classified = TextureLayerClassification::Paint;
    const char*         Blend        = "Normal";               // [-] - borrowed
    std::uint32_t       Opacity      = 100u;                   // [%] - 0…100
    std::uint32_t       PaintHue     = 0xF97316u;              // [-] - the swatch, 0xRRGGBB
    std::uint32_t       TagHue       = 0xF97316u;              // [-] - the spine and its badge
    bool                MaskDeclared = false;                  // [-] - a mask row is attached
    std::uint32_t       MaskStrength = 100u;                   // [%] - the mask's density
    bool                MaskInverted = false;                  // [-] - the mask is inverted
    const char*         Source       = "";                     // [-] - borrowed; mask source or generator name
    const char*         Detail       = "";                     // [-] - borrowed; the small sub-line, e.g. "2048px · RGBA 8"
    const char*         Channels[TextureChannelCeiling] = {};
    std::uint32_t       ChannelCount = 0u;                     // [-] - active channels
    std::uint32_t       Depth        = 0u;                     // [-] - folder nesting
    std::uint32_t       Enclosing    = 0xFFFFFFFFu;            // [-] - the row holding it; absent for the level
    std::uint32_t       EnclosedCount = 0u;                    // [-] - zero presents no disclosure mark
    bool                Expanded     = true;                   // [-] - a folder's disclosure
    const char*         Tagged       = "";                     // [-] - borrowed; search tags, space-separated
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE CHANNELS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The run naming one channel ordinal.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* TextureChannelText(std::uint32_t Ordinal);

/// 🧩 Which of the properties page's channel groups a channel belongs to (0 Base, 1 Maps, 2 Output).
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
std::uint32_t TextureChannelGroup(std::uint32_t Ordinal);

/// 🧩 The group captions for the properties page's channel filter.
/// tag   contract
inline constexpr std::uint32_t TextureChannelGroupCount = 3u;   // [-] - Base, Maps, Output

} // namespace Slate
