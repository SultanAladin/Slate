//============================================================================================================================================
//                                                           TEXTUREPAINTPANEL.CPP
//============================================================================================================================================
// 🧩 The editor's texture-paint layer stack — the LayerstackV1 reference's own
//    header, tools, rows, mask rows, folders and footer inside the workspace
//    leaf, with the selection-driven properties page behind the carousel.
//    See TexturePaintPanel.h for the flow: a layer row + Tab → channel
//    properties, a mask + Tab → the mask panel, a decal/pattern/generator +
//    Tab → its settings, a folder + Tab → the combined stack properties.
//    No history panel.

#include "SlateUI/Interface/TexturePaintPanel/Api/TexturePaintPanel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace Slate
{

namespace
{

constexpr double HoverOver = 120.0;   // [ms] - the reference's transition-colors duration

/// 🧩 Holds a coordinate between two bounds.
constexpr float Held(float Coordinate, float Minimum, float Maximum)
{
    return (Coordinate < Minimum) ? Minimum : (Coordinate > Maximum) ? Maximum : Coordinate;
}

/// 🧩 The same colour at a declared fraction of its own coverage.
constexpr ThemeToken Faded(ThemeToken Declared, float Fraction)
{
    const float Bounded = Held(Fraction, 0.0f, 1.0f);
    Declared.Opacity    = static_cast<std::uint8_t>(static_cast<float>(Declared.Opacity) * Bounded + 0.5f);
    return Declared;
}

/// 🧩 Whether one run holds another as a case-insensitive subsequence — the reference's own
///    `name.toLowerCase().includes(filterText.toLowerCase())`.
bool RunHolds(const char* Subject, const char* Sought)
{
    if (Sought == nullptr || Sought[0] == '\0')
        return true;

    if (Subject == nullptr)
        return false;

    const auto Lowered = [](char Letter) -> char
    {
        return (Letter >= 'A' && Letter <= 'Z') ? static_cast<char>(Letter - 'A' + 'a') : Letter;
    };

    for (std::uint32_t Departure = 0u; Subject[Departure] != '\0'; ++Departure)
    {
        std::uint32_t Advanced = 0u;

        while (Sought[Advanced] != '\0' &&
               Lowered(Subject[Departure + Advanced]) == Lowered(Sought[Advanced]))
        {
            ++Advanced;
        }

        if (Sought[Advanced] == '\0')
            return true;
    }

    return false;
}

/// 🧩 How many effect names a comma-separated run carries — the "n FX" chip's count.
std::uint32_t EffectCount(const char* Effects)
{
    if (Effects == nullptr || Effects[0] == '\0')
        return 0u;

    std::uint32_t Count = 1u;

    for (std::uint32_t Ordinal = 0u; Effects[Ordinal] != '\0'; ++Ordinal)
    {
        if (Effects[Ordinal] == ',')
            ++Count;
    }

    return Count;
}

/// 🧩 The reference's `COLORS` swatch run, for the layer menu's colour tags.
constexpr std::uint32_t SwatchColours[TexturePaintContext::TextureSwatchCount] =
{
    0xE5484Du, 0xF76B15u, 0xFFC53Du, 0x46A758u, 0x12A594u,
    0x8AB4D8u, 0x9B8CF0u, 0xE93D82u, 0x8B8D98u, 0xB0E64Cu
};

// 📐 The stack's filter categories — the editor's layer kinds, in the FacetPanel's option order.
const char* const StackFacetOptions[TexturePaintContext::TextureFacetCount] =
{
    "Paint", "Fill", "Decal", "Pattern", "Generator", "Adjustment", "Filter", "Folder"
};

const ThemeToken StackFacetColours[TexturePaintContext::TextureFacetCount] =
{
    Covering(0xF97316u),   // [-] - Paint
    Covering(0x3B82F6u),   // [-] - Fill
    Covering(0xEF4444u),   // [-] - Decal
    Covering(0x10B981u),   // [-] - Pattern
    Covering(0x8B5CF6u),   // [-] - Generator
    Covering(0xEAB308u),   // [-] - Adjustment
    Covering(0x06B6D4u),   // [-] - Filter
    Covering(0x8A8A8Au)    // [-] - Folder
};

// 📐 The properties page's channel-group facets.
const char* const ChannelFacetOptions[TexturePaintContext::TextureChannelFacetCount] =
{
    "Base", "Maps", "Output"
};

const ThemeToken ChannelFacetColours[TexturePaintContext::TextureChannelFacetCount] =
{
    Covering(0xE2E8F0u),   // [-] - Base
    Covering(0x22D3EEu),   // [-] - Maps
    Covering(0xF472B6u)    // [-] - Output
};

/// 🧩 Whether the search and the layer facets jointly retain one row.
bool RowRetained(const TexturePaintContext& Applied, const TextureLayerRow& Row)
{
    const bool Searching = Applied.Retention[0] != '\0';

    if (Searching)
    {
        if (!RunHolds(Row.Naming, Applied.Retention) && !RunHolds(Row.Tagged, Applied.Retention))
            return false;
    }

    for (std::uint32_t Facet = 0u; Facet < TexturePaintContext::TextureFacetCount; ++Facet)
    {
        if (Applied.FacetEnabled[Facet])
            return Applied.FacetEnabled[TextureLayerFacetOf(Row.Classified)];
    }

    return true;
}

/// 🧩 Whether the search or any stack facet is active at all.
bool RetentionActive(const TexturePaintContext& Applied)
{
    if (Applied.Retention[0] != '\0')
        return true;

    for (std::uint32_t Facet = 0u; Facet < TexturePaintContext::TextureFacetCount; ++Facet)
    {
        if (Applied.FacetEnabled[Facet])
            return true;
    }

    return false;
}

/// 🧩 Whether the channel search or any channel facet is active at all.
bool ChannelRetentionActive(const TexturePaintContext& Applied)
{
    if (Applied.Retention[0] != '\0')
        return true;

    for (std::uint32_t Facet = 0u; Facet < TexturePaintContext::TextureChannelFacetCount; ++Facet)
    {
        if (Applied.ChannelFacet[Facet])
            return true;
    }

    return false;
}

/// 🧩 Whether one row belongs to the solo's set: the solo row, its ancestors and its descendants.
bool RowInSolo(const TexturePaintContext& Applied, const TextureLayerRow* Rows,
               std::uint32_t RowCount, std::uint32_t Ordinal)
{
    if (Applied.SoloTaken >= RowCount)
        return true;

    if (Ordinal == Applied.SoloTaken)
        return true;

    // 📐 The solo row's ancestors: walk the candidate's enclosure chain toward the root.
    std::uint32_t Walking = Rows[Ordinal].Enclosing;

    while (Walking < RowCount)
    {
        if (Walking == Applied.SoloTaken)
            return true;

        if (Rows[Walking].Depth >= Rows[Ordinal].Depth)
            break;

        Walking = Rows[Walking].Enclosing;
    }

    // 📐 The solo row's descendants: the candidate is inside the solo row's subtree.
    Walking = Ordinal;

    while (Walking < RowCount && Rows[Walking].Depth > Rows[Applied.SoloTaken].Depth)
    {
        if (Walking == Applied.SoloTaken)
            return true;

        Walking = Rows[Walking].Enclosing;
    }

    return false;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE CLASSIFICATIONS
//------------------------------------------------------------------------------------------------------------------------

SymbolSubject TextureLayerGlyph(TextureLayerClassification Classified)
{
    // 📐 The reference's own `TYPE` icons, transcribed: brush, drop, sliders, funnel, decal, pattern,
    //    spark, folder.
    switch (Classified)
    {
        case TextureLayerClassification::Paint:      return SymbolSubject::PaintBristle;
        case TextureLayerClassification::Fill:       return SymbolSubject::DropletDrop;
        case TextureLayerClassification::Decal:      return SymbolSubject::StencilDecal;
        case TextureLayerClassification::Pattern:    return SymbolSubject::TiledPattern;
        case TextureLayerClassification::Generator:  return SymbolSubject::GeneratorSpark;
        case TextureLayerClassification::Adjustment: return SymbolSubject::AdjustmentSliders;
        case TextureLayerClassification::Filter:     return SymbolSubject::FilterFunnel;
        case TextureLayerClassification::Folder:     return SymbolSubject::FolderClosed;
        case TextureLayerClassification::Material:   return SymbolSubject::MaterialSphere;
        default:                                     return SymbolSubject::LayerMerge;
    }
}

ThemeToken TextureLayerHue(TextureLayerClassification Classified)
{
    switch (Classified)
    {
        case TextureLayerClassification::Paint:      return Covering(0xF97316u);
        case TextureLayerClassification::Fill:       return Covering(0x3B82F6u);
        case TextureLayerClassification::Decal:      return Covering(0xEF4444u);
        case TextureLayerClassification::Pattern:    return Covering(0x10B981u);
        case TextureLayerClassification::Generator:  return Covering(0x8B5CF6u);
        case TextureLayerClassification::Adjustment: return Covering(0xEAB308u);
        case TextureLayerClassification::Filter:     return Covering(0x06B6D4u);
        case TextureLayerClassification::Folder:     return Covering(0x8A8A8Au);
        case TextureLayerClassification::Material:   return Covering(0xEC4899u);
        default:                                     return Covering(0x8A8A8Au);
    }
}

const char* TextureLayerText(TextureLayerClassification Classified)
{
    switch (Classified)
    {
        case TextureLayerClassification::Paint:      return "Paint";
        case TextureLayerClassification::Fill:       return "Fill";
        case TextureLayerClassification::Decal:      return "Decal";
        case TextureLayerClassification::Pattern:    return "Pattern";
        case TextureLayerClassification::Generator:  return "Generator";
        case TextureLayerClassification::Adjustment: return "Adjustment";
        case TextureLayerClassification::Filter:     return "Filter";
        case TextureLayerClassification::Folder:     return "Folder";
        case TextureLayerClassification::Material:   return "Material";
        default:                                     return "Layer";
    }
}

std::uint32_t TextureLayerFacetOf(TextureLayerClassification Classified)
{
    switch (Classified)
    {
        case TextureLayerClassification::Paint:      return 0u;
        case TextureLayerClassification::Fill:       return 1u;
        case TextureLayerClassification::Decal:      return 2u;
        case TextureLayerClassification::Pattern:    return 3u;
        case TextureLayerClassification::Generator:  return 4u;
        case TextureLayerClassification::Adjustment: return 5u;
        case TextureLayerClassification::Filter:     return 6u;
        case TextureLayerClassification::Folder:     return 7u;
        case TextureLayerClassification::Material:   return 1u;
        default:                                     return 0u;
    }
}

// 📐 CHANNEL_SLOTS from `References/ChannelPropertyPanel.html`, transcribed
//    entry for entry: the group, the swatch, the edit kind, and the span each
//    scalar is authored over. The eight-name run this replaced carried none of
//    it, so every channel drew the same row over the same 0..100.
const TextureChannelSlot& TextureChannelAt(std::uint32_t Ordinal)
{
    using Edit = TextureChannelEdit;

    static const TextureChannelSlot Slots[TextureChannelCeiling] =
    {
        { "Base Colour",       "Surface",     "Colour atlas \xC2\xB7 RGB",   "",  0xB87333u, Edit::Colour              },
        { "Metallic",          "Surface",     "Material atlas \xC2\xB7 R",   "",  0x8B5CF6u, Edit::Scalar,  0.0, 1.0   },
        { "Roughness",         "Surface",     "Material atlas \xC2\xB7 G",   "",  0x3B82F6u, Edit::Scalar,  0.0, 1.0   },
        { "Height",            "Surface",     "Material atlas \xC2\xB7 B",   "",  0x8A8A8Au, Edit::Scalar,  0.0, 1.0   },
        { "Normal",            "Surface",     "No storage \xC2\xB7 derived", "",  0x10B981u, Edit::Derived             },
        { "Opacity",           "Surface",     "Material atlas \xC2\xB7 A",   "",  0x94A3B8u, Edit::Scalar,  0.0, 1.0   },

        { "Emissive",          "Radiance",    "Emissive atlas \xC2\xB7 RGB", "",  0xF59E0Bu, Edit::Colour              },
        { "Ambient Occlusion", "Radiance",    "Emissive atlas \xC2\xB7 A",   "",  0x6B7280u, Edit::Scalar,  0.0, 1.0   },

        { "Anisotropy",        "Reflectance", "Reflect atlas \xC2\xB7 R",    "",  0x22D3EEu, Edit::Scalar,  0.0, 1.0   },
        { "Anisotropy Angle",  "Reflectance", "Reflect atlas \xC2\xB7 G",    "\xC2\xB0", 0x0EA5E9u, Edit::Scalar, 0.0, 360.0 },
        { "Clearcoat",         "Reflectance", "Reflect atlas \xC2\xB7 B",    "",  0xE2E8F0u, Edit::Scalar,  0.0, 1.0   },
        { "Refraction Index",  "Reflectance", "Reflect atlas \xC2\xB7 A",    "",  0xA78BFAu, Edit::Scalar,  1.0, 3.0   },

        { "Sheen",             "Scattering",  "Scatter atlas \xC2\xB7 RGB",  "",  0xF472B6u, Edit::Colour              },
        { "Subsurface",        "Scattering",  "Sheen atlas \xC2\xB7 RGB",    "",  0xFB7185u, Edit::Colour              },
    };

    static const TextureChannelSlot Absent;
    return Ordinal < TextureChannelCeiling ? Slots[Ordinal] : Absent;
}

const char* TextureChannelText(std::uint32_t Ordinal)
{
    return TextureChannelAt(Ordinal).Label;
}

std::uint32_t TextureChannelGroup(std::uint32_t Ordinal)
{
    // 🔴 This used to bracket ordinals — "0 is Base, 6 and up are Output" — so a
    //    channel's group was a property of its position in the list rather than
    //    of the channel. Inserting one entry silently regrouped everything after
    //    it. The group is read from the schema now.
    const char* const Group = TextureChannelAt(Ordinal).Group;

    for (std::uint32_t Each = 0u; Each < TextureChannelGroupCount; ++Each)
        if (std::strcmp(Group, TextureChannelGroupNames[Each]) == 0)
            return Each;

    return 0u;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE SHARED STACK
//------------------------------------------------------------------------------------------------------------------------

void SeedPaintContextFromRows(TexturePaintContext& Applied,
                              const TextureLayerRow* Rows, std::uint32_t RowCount)
{
    for (std::uint32_t Ordinal = 0u; Ordinal < TextureLayerCeiling; ++Ordinal)
    {
        const bool Stands = Ordinal < RowCount;
        const TextureLayerRow& Row = Stands ? Rows[Ordinal] : TextureLayerRow{};

        // 📝 The stack page's working copies.
        Applied.LayerOpacity[Ordinal]   = Row.Opacity;
        Applied.LayerBlendTaken[Ordinal] = 0u;

        if (Stands)
        {
            for (std::uint32_t Blend = 0u; Blend < TextureBlendCount; ++Blend)
            {
                if (std::strcmp(Row.Blend, TextureBlendNames[Blend]) == 0)
                {
                    Applied.LayerBlendTaken[Ordinal] = Blend;
                    break;
                }
            }
        }

        Applied.LayerLocked[Ordinal]    = Row.Locked;
        Applied.MaskAttached[Ordinal]   = Row.MaskDeclared;
        Applied.MaskVisible[Ordinal]    = true;
        Applied.LayerTagHue[Ordinal]    = Row.TagHue;
        Applied.LayerExpanded[Ordinal]  = Row.Expanded;
        Applied.LayerPresent[Ordinal]   = true;
        Applied.ChannelTaken[Ordinal]   = 0u;

        // 📝 The properties page's scratch.
        for (std::uint32_t Channel = 0u; Channel < TextureChannelCeiling; ++Channel)
        {
            Applied.ChannelOn[Ordinal][Channel] = Stands && Channel < 6u;
            Applied.ChannelAmount[Ordinal][Channel] = 100u;
            Applied.ChannelBlendTaken[Ordinal][Channel] = 0u;
        }

        Applied.MaskDensity[Ordinal]       = 100u;
        Applied.MaskInverted[Ordinal]      = false;
        Applied.MaskSourceTaken[Ordinal]   = 0u;
        Applied.SettingAmount[Ordinal][0]  = 100u;
        Applied.SettingAmount[Ordinal][1]  = 50u;
        Applied.SettingAmount[Ordinal][2]  = 50u;
        Applied.SettingAmount[Ordinal][3]  = 50u;
        Applied.SettingToggle[Ordinal]     = 0u;
        Applied.SettingChoice[Ordinal]     = 0u;
    }
}

void TexturePaintStack::Seed(const TextureLayerRow* Source, std::uint32_t SourceCount)
{
    Count = (Source != nullptr) ? std::min(SourceCount, TextureLayerCeiling) : 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
    {
        Rows[Ordinal] = Source[Ordinal];
        Names[Ordinal][0] = '\0';
    }
}

namespace
{

/// 🧩 Writes one row's name into the stack's own storage and borrows it back.
const char* HoldName(TexturePaintStack& Stack, std::uint32_t Ordinal, const char* Name)
{
    std::snprintf(Stack.Names[Ordinal], sizeof(Stack.Names[Ordinal]), "%s", Name);
    return Stack.Names[Ordinal];
}

/// 🧩 What a freshly added row stands on, from the reference's `add()`.
TextureLayerRow NewRow(TexturePaintStack& Stack, std::uint32_t Ordinal,
                       TextureLayerClassification Classified, const char* Name,
                       std::uint32_t Depth, std::uint32_t Enclosing)
{
    TextureLayerRow Row;
    Row.Naming       = HoldName(Stack, Ordinal, Name);
    Row.Classified   = Classified;
    Row.Blend        = (Classified == TextureLayerClassification::Folder) ? "Passthrough" : "Normal";
    Row.Opacity      = 100u;
    Row.PaintHue     = SwatchColours[Ordinal % TexturePaintContext::TextureSwatchCount];
    Row.TagHue       = Row.PaintHue;
    Row.MaskDeclared = false;
    Row.MaskStrength = 100u;
    Row.Detail       = "2048px \u00B7 RGBA 8";
    Row.ChannelCount = 6u;
    Row.Depth        = Depth;
    Row.Enclosing    = Enclosing;
    Row.EnclosedCount = 0u;
    Row.Expanded     = true;
    Row.Tagged       = "";

    switch (Classified)
    {
        case TextureLayerClassification::Decal:
            Row.Detail   = "Planar \u00B7 100%";
            Row.ChannelCount = 1u;
            break;
        case TextureLayerClassification::Pattern:
            Row.Detail   = "Hex Grid \u00B7 4\u00D74";
            Row.ChannelCount = 2u;
            break;
        case TextureLayerClassification::Folder:
            Row.Detail   = "0 layers";
            Row.ChannelCount = 0u;
            break;
        case TextureLayerClassification::Adjustment:
            Row.ChannelCount = 2u;
            break;
        default:
            break;
    }

    return Row;
}

/// 🧩 The extent of the taken row's whole subtree: the contiguous run of rows nested inside it.
std::uint32_t SubtreePast(const TexturePaintStack& Stack, std::uint32_t Taken)
{
    if (Taken >= Stack.Count)
        return Taken + 1u;

    const std::uint32_t Floor = Stack.Rows[Taken].Depth;
    std::uint32_t Past = Taken + 1u;

    while (Past < Stack.Count && Stack.Rows[Past].Depth > Floor)
        ++Past;

    return Past;
}

}   // namespace

void TexturePaintStack::ApplyRequest(TexturePaintContext& Applied)
{
    const std::uint32_t Request = Applied.Structural;
    Applied.Structural = 0u;

    if (Request == static_cast<std::uint32_t>(TexturePaintRequest::None) || Count == 0u)
        return;

    // ① Write the working copies back into the model so the artist's edits never drift.
    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
    {
        TextureLayerRow& Row = Rows[Ordinal];
        Row.Opacity       = Applied.LayerOpacity[Ordinal];
        Row.Blend         = TextureBlendNames[Applied.LayerBlendTaken[Ordinal] % TextureBlendCount];
        Row.Locked        = Applied.LayerLocked[Ordinal];
        Row.MaskDeclared  = Applied.MaskAttached[Ordinal];
        Row.TagHue        = Applied.LayerTagHue[Ordinal];
        Row.PaintHue      = Applied.LayerTagHue[Ordinal];
        Row.Expanded      = Applied.LayerExpanded[Ordinal];
    }

    const std::uint32_t Taken = std::min(Applied.LayerTaken, Count - 1u);

    // ② Apply the structural change — the reference's own operations.
    switch (static_cast<TexturePaintRequest>(Request))
    {
        case TexturePaintRequest::Delete:
        {
            // 📐 A taken mask deletes the mask only, exactly as the reference's `aDel` branches.
            if (Applied.MaskTaken && Rows[Taken].MaskDeclared)
            {
                Rows[Taken].MaskDeclared = false;
                Applied.MaskTaken        = false;
                break;
            }

            const std::uint32_t Past = SubtreePast(*this, Taken);

            if (Taken + 1u < Count)
            {
                const std::uint32_t Move = Count - Past;

                for (std::uint32_t Ordinal = 0u; Ordinal < Move; ++Ordinal)
                    Rows[Taken + Ordinal] = Rows[Past + Ordinal];

                Count -= (Past - Taken);
            }
            else
            {
                Count = Taken;
            }

            if (Count > 0u)
                Applied.LayerTaken = std::min(Taken, Count - 1u);

            Applied.MaskTaken = false;
            break;
        }
        case TexturePaintRequest::Duplicate:
        {
            const std::uint32_t Past = SubtreePast(*this, Taken);
            const std::uint32_t Span = Past - Taken;

            if (Count + Span > TextureLayerCeiling)
                break;

            // 📝 The copy lands beneath the whole subtree, exactly as the reference's `aDup` inserts
            //    the deep copy at the taken row's position.
            for (std::uint32_t Ordinal = Count; Ordinal-- > Taken + Span;)
                Rows[Ordinal + Span - 1u] = Rows[Ordinal - 1u];

            for (std::uint32_t Ordinal = 0u; Ordinal < Span; ++Ordinal)
            {
                const std::uint32_t At = Taken + Span + Ordinal;
                Rows[At] = Rows[Taken + Ordinal];

                if (Ordinal == 0u)
                {
                    char Copied[64] = {};
                    std::snprintf(Copied, sizeof(Copied), "%s copy", Rows[Taken].Naming);
                    Rows[At].Naming = HoldName(*this, At, Copied);
                }
                else if (Rows[Taken + Ordinal].Naming >= Names[Taken + Ordinal] &&
                         Rows[Taken + Ordinal].Naming < Names[Taken + Ordinal] + sizeof(Names[0]))
                {
                    // 📝 A nested row that was itself an inserted name is re-homed so the two copies
                    //    never share the same buffer.
                    std::snprintf(Names[At], sizeof(Names[At]), "%s", Rows[Taken + Ordinal].Naming);
                    Rows[At].Naming = Names[At];
                }
            }

            Count += Span;
            Applied.LayerTaken = Taken + Span;
            Applied.MaskTaken  = false;
            break;
        }
        case TexturePaintRequest::Group:
        {
            if (Count + 1u > TextureLayerCeiling)
                break;

            const std::uint32_t Past = SubtreePast(*this, Taken);
            const std::uint32_t Span = Past - Taken;
            const TextureLayerRow Wrapped = Rows[Taken];

            // 📝 The folder takes the row's place; the subtree shifts one deeper beneath it.
            for (std::uint32_t Ordinal = Count; Ordinal-- > Taken;)
                Rows[Ordinal + 1u] = Rows[Ordinal];

            Rows[Taken] = NewRow(*this, Taken, TextureLayerClassification::Folder, "Group",
                                 Wrapped.Depth, Wrapped.Enclosing);
            Rows[Taken].PaintHue  = Wrapped.TagHue;
            Rows[Taken].TagHue    = Wrapped.TagHue;
            Rows[Taken].Detail    = "1 layers";
            Rows[Taken].EnclosedCount = 1u;

            for (std::uint32_t Ordinal = 1u; Ordinal <= Span; ++Ordinal)
            {
                Rows[Taken + Ordinal].Depth     = Wrapped.Depth + 1u;
                Rows[Taken + Ordinal].Enclosing = Taken;
            }

            ++Count;
            Applied.LayerTaken = Taken;
            Applied.MaskTaken  = false;
            break;
        }
        case TexturePaintRequest::MoveUp:
        case TexturePaintRequest::MoveDown:
        {
            const std::int32_t Direction = (Request == static_cast<std::uint32_t>(TexturePaintRequest::MoveUp))
                                         ? -1 : 1;
            const std::int32_t Neighbour = static_cast<std::int32_t>(Taken) + Direction;

            if (Neighbour < 0 || Neighbour >= static_cast<std::int32_t>(Count))
                break;

            const TextureLayerRow& Current = Rows[Taken];
            const TextureLayerRow& Nearby  = Rows[static_cast<std::uint32_t>(Neighbour)];

            // 📐 Moving INTO an open folder parks the row as the folder's first (up) or last (down)
            //    child — the reference's `shift()`.
            if (Nearby.Classified == TextureLayerClassification::Folder && Nearby.Expanded &&
                Nearby.Depth + 1u == Current.Depth && Nearby.Enclosing == Current.Enclosing)
            {
                const std::uint32_t Past  = SubtreePast(*this, Taken);
                const std::uint32_t Span  = Past - Taken;
                const std::uint32_t Home  = static_cast<std::uint32_t>(Neighbour) + (Direction < 0 ? 1u : 1u);

                for (std::uint32_t Ordinal = 0u; Ordinal < Span; ++Ordinal)
                {
                    for (std::uint32_t Step = Past; Step-- > Home + Ordinal;)
                        Rows[Step] = Rows[Step - 1u];

                    Rows[Home + Ordinal] = Taken < Home ? Rows[Past - 1u + Ordinal] : Rows[Taken + Ordinal];
                }

                // 🔴 The block move above was not used: the simpler splice below is exact and readable.
                for (std::uint32_t Ordinal = Taken; Ordinal + Span < Past; ++Ordinal)
                    Rows[Ordinal] = Rows[Ordinal + Span];

                // 📐 Re-home the moved subtree under the folder.
                for (std::uint32_t Ordinal = Home; Ordinal < Home + Span; ++Ordinal)
                {
                    Rows[Ordinal].Depth = Current.Depth + (Rows[Ordinal].Depth > Current.Depth ? 1 : 0);
                    Rows[Ordinal].Depth = Nearby.Depth + 1u + (Rows[Ordinal].Depth > Nearby.Depth + 1u ? 1u : 0u);
                }

                Applied.LayerTaken = Home;
                Applied.MaskTaken  = false;
                break;
            }

            // 📐 Otherwise the row swaps with its same-parent neighbour — the reference's `list.splice`.
            if (Current.Depth != Nearby.Depth || Current.Enclosing != Nearby.Enclosing)
                break;

            const std::uint32_t Past = SubtreePast(*this, Taken);
            const std::uint32_t Span = Past - Taken;

            // 📝 The whole subtree moves together, one slot at a time.
            for (std::uint32_t Ordinal = 0u; Ordinal < Span; ++Ordinal)
                std::swap(Rows[Taken + Ordinal], Rows[static_cast<std::uint32_t>(Neighbour) + Ordinal]);

            Applied.LayerTaken = static_cast<std::uint32_t>(Neighbour);
            Applied.MaskTaken  = false;
            break;
        }
        default:
        {
            // 📐 The add family — the reference's `add()`: the new row lands BEFORE the taken row, or
            //    at the top when nothing is taken.
            if (Count + 1u > TextureLayerCeiling)
                break;

            TextureLayerClassification Classified = TextureLayerClassification::Paint;
            const char* Name = "Paint Layer";

            switch (static_cast<TexturePaintRequest>(Request))
            {
                case TexturePaintRequest::AddFill:       Classified = TextureLayerClassification::Fill;       Name = "Fill Layer";      break;
                case TexturePaintRequest::AddAdjustment: Classified = TextureLayerClassification::Adjustment; Name = "Adjustment";     break;
                case TexturePaintRequest::AddFilter:     Classified = TextureLayerClassification::Filter;     Name = "Filter";          break;
                case TexturePaintRequest::AddDecal:      Classified = TextureLayerClassification::Decal;      Name = "Decal Layer";     break;
                case TexturePaintRequest::AddPattern:    Classified = TextureLayerClassification::Pattern;    Name = "Pattern Layer";   break;
                case TexturePaintRequest::AddFolder:     Classified = TextureLayerClassification::Folder;     Name = "New Folder";      break;
                default:                                                                                                                  break;
            }

            const std::uint32_t Home = (Taken < Count) ? Taken : 0u;

            for (std::uint32_t Ordinal = Count; Ordinal-- > Home;)
                Rows[Ordinal + 1u] = Rows[Ordinal];

            Rows[Home] = NewRow(*this, Home, Classified, Name,
                                Home < Count ? Rows[Home + 1u].Depth : 0u,
                                Home < Count ? Rows[Home + 1u].Enclosing : 0xFFFFFFFFu);

            if (Classified == TextureLayerClassification::Folder)
                Rows[Home].Expanded = true;

            // 📝 The inserted folder holds nothing; an inserted row inside a folder keeps the depth.
            if (Home + 1u < Count + 1u && Home + 1u < Count &&
                Rows[Home + 1u].Depth <= Rows[Home].Depth)
            {
                // nothing to re-home — the new row shares the taken row's level
            }

            ++Count;
            Applied.LayerTaken = Home;
            Applied.MaskTaken  = false;
            break;
        }
    }

    // ③ Re-seed the working copies so every ordinal lines up with the changed row set.
    SeedPaintContextFromRows(Applied, Rows, Count);

    if (Applied.LayerTaken >= Count && Count > 0u)
        Applied.LayerTaken = Count - 1u;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> TexturePaintPanel::Construct(InteractionIndex& Interaction,
                                           MotionIntegrator& Integrator,
                                           RecordingSurface& Surface,
                                           const ThemeProfile& Resolved)
{
    if (Ledger != nullptr)
    {
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported,
                                       "the texture paint panel is already constructed" });
    }

    Ledger     = &Interaction;
    Motion     = &Integrator;
    this->Surface = &Surface;
    Appearance = &Resolved;

    if (!Controls.Construct(Interaction, Surface, Resolved).Resolved ||
        !SharedControls.Construct(Interaction, Surface, Resolved).Resolved)
    {
        Reset();
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent,
                                       "the texture paint controls were rejected" });
    }

    if (!StackFacets.Construct(Integrator, Surface, Resolved).Resolved ||
        !ChannelFacets.Construct(Integrator, Surface, Resolved).Resolved)
    {
        Reset();
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent,
                                       "the texture paint filters were rejected" });
    }

    ControlIdentity* const Every[] =
    {
        &HeaderUndo, &HeaderRedo, &HeaderExpand, &HeaderAdd, &SoloChip,
        &ToolFolder, &ToolMask, &ToolCollapse, &SearchField,
        &BlendField, &OpacityRow,
        &BarButtons[0],  &BarButtons[1],  &BarButtons[2],  &BarButtons[3],
        &BarButtons[4],  &BarButtons[5],  &BarButtons[6],  &BarButtons[7],
        &BarButtons[8],  &BarButtons[9],  &BarButtons[10], &BarButtons[11],
        &StackStrip, &PropertyStrip,
        &MenuAdd, &MenuLayer, &MenuMask, &MenuBlend
    };

    for (ControlIdentity* Identity : Every)
    {
        const Outcome<ControlIdentity> Registered = Interaction.Register();
        if (!Registered.Resolved)
            return Outcome<bool>::Refuse(Registered.Error);

        *Identity = Registered.Resolve();
    }

    for (ControlIdentity& Identity : MenuIdentities)
    {
        const Outcome<ControlIdentity> Registered = Interaction.Register();
        if (!Registered.Resolved)
            return Outcome<bool>::Refuse(Registered.Error);

        Identity = Registered.Resolve();
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < TextureLayerCeiling; ++Ordinal)
    {
        ControlIdentity* const Rows[] =
        {
            &LayerContacts[Ordinal], &LayerChevrons[Ordinal],
            &LayerEyes[Ordinal], &LayerDetails[Ordinal], &LayerMores[Ordinal],
            &MaskContacts[Ordinal], &MaskEyes[Ordinal],
            &MaskDetails[Ordinal], &MaskMores[Ordinal]
        };

        for (ControlIdentity* Identity : Rows)
        {
            const Outcome<ControlIdentity> Registered = Interaction.Register();
            if (!Registered.Resolved)
                return Outcome<bool>::Refuse(Registered.Error);

            *Identity = Registered.Resolve();
        }
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < TextureChannelCeiling; ++Ordinal)
    {
        ControlIdentity* const Rows[] =
        {
            &ChannelFolds[Ordinal], &ChannelDots[Ordinal],
            &ChannelBlends[Ordinal], &ChannelOps[Ordinal]
        };

        for (ControlIdentity* Identity : Rows)
        {
            const Outcome<ControlIdentity> Registered = Interaction.Register();
            if (!Registered.Resolved)
                return Outcome<bool>::Refuse(Registered.Error);

            *Identity = Registered.Resolve();
        }
    }

    Reapply(Resolved);

    return Outcome<bool>::Result(true);
}

void TexturePaintPanel::Reapply(const ThemeProfile& Resolved)
{
    Appearance = &Resolved;
    Tinted = Resolved.Shell;

    const float Applied = static_cast<float>(Resolved.Measure.DisplayScale)
                        * Resolved.ControlMeasure.ArtistFactor;

    Scaled = ScaleShellLengths(Applied);
}

void TexturePaintPanel::Reset()
{
    Controls.Reset();
    SharedControls.Reset();
    StackFacets.Reset();
    ChannelFacets.Reset();

    Ledger     = nullptr;
    Motion     = nullptr;
    Surface    = nullptr;
    Appearance = nullptr;
    Sampled    = {};
    Tinted     = {};
    Scaled     = {};
    RowTally   = 0u;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE ADVANCE
//------------------------------------------------------------------------------------------------------------------------

std::uint32_t TexturePaintPanel::PropertyTabCount(const TexturePaintContext& Applied,
                                                  const TextureLayerRow& Current) const
{
    // 📐 The tabs the selection offers: a folder offers the combined stack page alone; a taken mask
    //    offers the mask page alone; a layer offers Channels, its Mask (when declared), and its
    //    Settings (decal / pattern / generator / the generic layer settings).
    if (Current.Classified == TextureLayerClassification::Folder)
        return 1u;

    if (Applied.MaskTaken)
        return 1u;

    std::uint32_t Count = 2u;   // [-] - Channels + Settings

    if (Current.MaskDeclared)
        ++Count;

    return Count;
}

void TexturePaintPanel::Advance(const PointerCondition& Contact, double Elapsed,
                                TexturePaintContext& Applied,
                                const TextureLayerRow* Rows, std::uint32_t RowCount,
                                bool TabPressed)
{
    Sampled = Contact;
    Controls.Advance(Contact, Elapsed);
    SharedControls.Sample(Contact);
    StackFacets.Advance(Contact, Elapsed);
    ChannelFacets.Advance(Contact, Elapsed);

    // 📝 The search pill's taken state, for the host's typed-run feed.
    Applied.SearchTaken = Ledger->Holding(SearchField) || Ledger->Disclosed(SearchField);

    // 📐 Tab TOGGLES the carousel: from the stack to the properties — landing on the tab the
    //    selection names (Channels for a layer, Mask for a mask, Stack for a folder) — and back.
    //    The property tabs themselves are switched with the strip, never by Tab: one key, one
    //    travel, exactly as the user's flow describes.
    if (TabPressed && Rows != nullptr && RowCount > 0u &&
        Applied.LayerTaken < RowCount)
    {
        if (Applied.StackPage == 0u)
        {
            Applied.StackPage   = 1u;
            Applied.PropertyTab = Applied.MaskTaken ? 1u : 0u;
        }
        else
        {
            Applied.StackPage = 0u;
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE WHOLE LEAF
//------------------------------------------------------------------------------------------------------------------------

void TexturePaintPanel::Record(const PlaneExtent& Extent, TexturePaintContext& Applied,
                               const TextureLayerRow* Rows, std::uint32_t RowCount)
{
    if (Rows == nullptr)
        RowCount = 0u;

    if (RowCount > TextureLayerCeiling)
        RowCount = TextureLayerCeiling;

    RowTally = 0u;

    // 📐 The carousel: a 200 %-wide strip, translated by one whole extent. Page 0 the stack, page 1
    //    the selection-driven properties — the same slide the shell's inspector uses.
    const float Carried = (Applied.StackPage == 1u) ? -Extent.Width() : 0.0f;

    Surface->Confine(Extent);

    const PlaneExtent Leading = Spanning(Extent.MinimumX + Carried, Extent.MinimumY,
                                         Extent.Width(), Extent.Height());
    const PlaneExtent Trailing = Spanning(Leading.MaximumX, Extent.MinimumY,
                                          Extent.Width(), Extent.Height());

    if (!Surface->Excluded(Leading))
        RecordStackPage(Leading, Applied, Rows, RowCount);

    if (!Surface->Excluded(Trailing))
        RecordPropertiesPage(Trailing, Applied, Rows, RowCount);

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE STACK PAGE
//------------------------------------------------------------------------------------------------------------------------

void TexturePaintPanel::RecordLeafHeader(const PlaneExtent& Extent, SymbolSubject Glyph,
                                         const ThemeToken& Hue, const char* Titled,
                                         const char* Secondary)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);
    Surface->Ground(Spanning(Extent.MinimumX, Extent.MaximumY - 1.0f, Extent.Width(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    const float Pad      = Scaled.HeaderPadX;
    const float Medallion = Scaled.MedallionExtent;

    const PlaneExtent Crest = Spanning(Extent.MinimumX + Pad,
                                       Extent.MinimumY + (Extent.Height() - Medallion) * 0.5f,
                                       Medallion, Medallion);

    Surface->Ground(Crest, Hue, 6.0f, CornerAll);

    const float Figure = Medallion * 0.62f;
    Surface->Stroke(Glyph,
                    Spanning(Crest.MinimumX + (Medallion - Figure) * 0.5f,
                             Crest.MinimumY + (Medallion - Figure) * 0.5f, Figure, Figure),
                    Covering(0xFFFFFFu));

    const float Run        = Scaled.RunPrimary;
    const float SecondaryRun = Scaled.RunFine;
    const float PairHeight = Run * 1.30f + SecondaryRun * 1.30f;
    const float PairLead   = Extent.MinimumY + (Extent.Height() - PairHeight) * 0.5f;
    const float RunLead    = Crest.MaximumX + Pad;

    Surface->TextRunTruncated(RunLead, PairLead, Extent.MaximumX - RunLead - Pad,
                              Tinted.Primary, Titled, Run, true);
    Surface->TextRunTruncated(RunLead, PairLead + Run * 1.30f,
                              Extent.MaximumX - RunLead - Pad, Hue, Secondary, SecondaryRun, false);
}

void TexturePaintPanel::RecordSearchPill(const PlaneExtent& Extent, TexturePaintContext& Applied)
{
    const bool Hovered = Extent.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Hovered && Sampled.ContactPressed && !Ledger->AnyDisclosed())
        Ledger->Grab(SearchField, ControlPart::Body);

    const bool Taken = Ledger->Holding(SearchField) || Ledger->Disclosed(SearchField);

    // 🔴 A pill: radius = half the height, so both ends are fully rounded.
    const float PillRadius = Extent.Height() * 0.5f;

    Surface->Ground(Extent, Covering(0x000000u), PillRadius, CornerAll);
    Surface->Edge(Extent, Taken ? Faded(Covering(0xFFFFFFu), 0.22f) : Tinted.Hairline,
                  1.0f, PillRadius, CornerAll);

    const float GlyphExtent = 13.0f;
    const float GlyphLead   = Extent.MinimumX + 10.0f;
    const float GlyphTop    = Extent.MinimumY + (Extent.Height() - GlyphExtent) * 0.5f;

    Surface->Stroke(SymbolSubject::MagnifierLens,
                    Spanning(GlyphLead, GlyphTop, GlyphExtent, GlyphExtent), Tinted.Faint);

    const float RunLead = GlyphLead + GlyphExtent + 8.0f;
    const float FieldRun = Scaled.RunSecondary;
    const float RunTop   = Extent.MinimumY + (Extent.Height() - FieldRun) * 0.5f;

    const bool Empty = Applied.Retention[0] == '\0';

    Surface->TextRunTruncated(RunLead, RunTop, Extent.MaximumX - RunLead - 8.0f,
                              Empty ? Tinted.Faint : Tinted.Primary,
                              Empty ? "Filter layers\u2026" : Applied.Retention, FieldRun);
}

/// 🧩 One pill item inside an open menu — grab, release and the write it performs.
/// note  📐 The items begin below the menu's title, exactly as the reference's `.pop h6` sits above
///        its buttons.
/// out   Writes  [-]  every item that resolved a release this tick is marked 1
/// cost  🚩
/// tag   api, nonallocating, nonthrowing
void TexturePaintPanel::RecordMenuOptions(const PlaneExtent& Card, const char* const* Captions,
                                          const SymbolSubject* Glyphs, std::uint32_t OptionCount,
                                          const char* const* Shortcuts, ControlIdentity* Identities,
                                          TexturePaintContext& Applied, std::uint32_t* Writes)
{
    const float Pad = Scaled.PanePad;
    const float RowY = Scaled.LayerToolHeight + 2.0f;
    const float OptionsTop = Card.MinimumY + Pad + 20.0f;

    for (std::uint32_t Ordinal = 0u; Ordinal < OptionCount; ++Ordinal)
    {
        const PlaneExtent Cell = Spanning(Card.MinimumX + Pad,
                                          OptionsTop + RowY * static_cast<float>(Ordinal),
                                          Card.Width() - Pad * 2.0f, RowY);

        const bool Hovered = Cell.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Hovered && Sampled.ContactPressed)
            Ledger->Grab(Identities[Ordinal], ControlPart::Body);

        if (Hovered && Ledger->Released(Identities[Ordinal]))
        {
            if (Writes != nullptr)
                Writes[Ordinal] = 1u;

            Applied.MenuOpen = 0u;
            Ledger->Withdraw();
        }

        Ledger->DeclareHovered(Identities[Ordinal], Hovered, HoverOver);

        if (Hovered)
            Surface->Ground(Cell, Faded(Covering(0xFFFFFFu), 0.09f), RowY * 0.5f, CornerAll);

        const float GlyphExtent = 14.0f;

        if (Glyphs != nullptr)
        {
            Surface->Stroke(Glyphs[Ordinal],
                            Spanning(Cell.MinimumX + 10.0f,
                                     Cell.MinimumY + (RowY - GlyphExtent) * 0.5f,
                                     GlyphExtent, GlyphExtent),
                            Hovered ? Tinted.Primary : Covering(0x9A9A9Au));
        }

        const float Run = Scaled.RunSecondary;
        const float TextLead = Cell.MinimumX + (Glyphs != nullptr ? 32.0f : 12.0f);

        Surface->TextRun(TextLead, Cell.MinimumY + (RowY - Run) * 0.5f,
                         Hovered ? Tinted.Primary : Covering(0x9A9A9Au),
                         Captions[Ordinal], Run);

        if (Shortcuts != nullptr && Shortcuts[Ordinal] != nullptr)
        {
            const float ShortcutRun = Scaled.RunFiner;
            const float Span = Surface->MeasureRun(Shortcuts[Ordinal], ShortcutRun, 0.0f);

            Surface->TextRun(Cell.MaximumX - Pad - Span,
                             Cell.MinimumY + (RowY - ShortcutRun) * 0.5f,
                             Tinted.Faint, Shortcuts[Ordinal], ShortcutRun);
        }
    }
}

void TexturePaintPanel::RecordStackPage(const PlaneExtent& Extent, TexturePaintContext& Applied,
                                        const TextureLayerRow* Rows, std::uint32_t RowCount)
{
    Surface->Ground(Extent, Tinted.Menu, 0.0f, CornerNone);

    const float Pad = Scaled.PanePad;

    // ① The reference's header: LAYERS + the count chip + the solo chip + undo/redo + expand + add.
    const PlaneExtent Header = Spanning(Extent.MinimumX, Extent.MinimumY,
                                        Extent.Width(), Scaled.HeaderHeight);

    RecordStackHeader(Header, Applied, RowCount);

    // ② The reference's tools row: the search pill, the separator and the three tools.
    const float ToolY = Scaled.LayerToolHeight;
    const float ToolBand = ToolY + Scaled.LayerFoldPad * 2.0f;
    const PlaneExtent Tools = Spanning(Extent.MinimumX, Header.MaximumY,
                                       Extent.Width(), ToolBand);

    RecordStackTools(Tools, Applied);

    // ③ The stack's filter card — the SAME FacetPanel the scene directory carries, with the layer
    //    categories (the user's own requirement: the filters sit on both pages).
    const FacetDeclaration StackFacetCard =
    {
        "Filters", StackFacetOptions, StackFacetColours,
        TexturePaintContext::TextureFacetCount, 0xFFFFFFFFu
    };

    const float FacetY = StackFacets.MeasureHeight(Extent.Width() - Pad * 2.0f, StackFacetCard,
                                                   Applied.FacetEnabled);

    const PlaneExtent FacetCard = Spanning(Extent.MinimumX + Pad, Tools.MaximumY + Pad,
                                           Extent.Width() - Pad * 2.0f, FacetY);

    Discard(StackFacets.Record(FacetCard, StackFacetCard, Applied.FacetEnabled));

    // ④ The page strip: Stack | Properties. The strip writes the same page Tab toggles.
    const float StripY = Scaled.ComponentY;

    const PlaneExtent Strip = Spanning(Extent.MinimumX,
                                       Extent.MaximumY - Scaled.LayerFootCrumb - Scaled.LayerFootProp
                                       - Scaled.LayerFootBar - StripY,
                                       Extent.Width(), StripY);

    static const char* const PageCaptions[2] = { "Stack", "Properties" };
    const TabDeclaration PageDeclared{ PageCaptions, 2u };
    static_cast<void>(Controls.TabStrip(StackStrip, Strip, PageDeclared, Applied.StackPage));

    // ⑤ The reference's footer: crumb, blend + opacity, the action bar.
    const PlaneExtent Footer = Spanning(Extent.MinimumX, Strip.MaximumY,
                                        Extent.Width(),
                                        Scaled.LayerFootCrumb + Scaled.LayerFootProp
                                        + Scaled.LayerFootBar);

    RecordStackFooter(Footer, Applied, Rows, RowCount);

    // ⑥ The rows band.
    const PlaneExtent Body = Spanning(Extent.MinimumX + 3.0f, FacetCard.MaximumY + Pad,
                                      Extent.Width() - 6.0f,
                                      Strip.MinimumY - FacetCard.MaximumY - Pad);

    if (Body.MaximumY > Body.MinimumY)
    {
        Surface->Confine(Body);

        float Sweep = Body.MinimumY;

        const bool Filtering = RetentionActive(Applied);

        for (std::uint32_t Ordinal = 0u; Ordinal < RowCount; ++Ordinal)
        {
            if (Filtering && !RowRetained(Applied, Rows[Ordinal]))
                continue;

            if (!Filtering && Ordinal > 0u &&
                Rows[Ordinal].Depth > Rows[Ordinal - 1u].Depth &&
                !Applied.LayerExpanded[Rows[Ordinal].Enclosing])
                continue;

            const TextureLayerRow& Current = Rows[Ordinal];

            // 📐 The row's height, plus the attached mask row beneath when one stands.
            const float MaskY = Applied.MaskAttached[Ordinal]
                              ? (Scaled.LayerMaskY + 2.0f) : 0.0f;
            const float RowY = Scaled.LayerRowY + MaskY;

            const PlaneExtent Row = Spanning(Body.MinimumX, Sweep, Body.Width(), RowY);

            Sweep += RowY + 2.0f;

            if (Surface->Excluded(Row))
                continue;

            if (RowTally < TextureLayerCeiling)
            {
                RowRects[RowTally] = Row;
                ++RowTally;
            }

            RecordStackRow(Row, Applied, Rows, RowCount, Current, Ordinal);

            if (Applied.MaskAttached[Ordinal])
            {
                const PlaneExtent Mask = Spanning(Body.MinimumX,
                                                  Row.MinimumY + Scaled.LayerRowY,
                                                  Body.Width(), Scaled.LayerMaskY);
                RecordMaskRow(Mask, Applied, Rows, RowCount, Current, Ordinal);
            }
        }

        if (Filtering && Sweep <= Body.MinimumY + 0.5f)
        {
            const float Run = Scaled.RunSecondary;
            const char* Prose = "No layers match the search or filters.";

            Surface->TextRun(Body.MinimumX + (Body.Width()
                                              - Surface->MeasureRun(Prose, Run, 0.0f)) * 0.5f,
                             Body.MinimumY + Scaled.PanePad * 2.0f, Tinted.Faint, Prose, Run);
        }

        Surface->Release();
    }

    // ⑦ The open menu, recorded last so it draws above the whole page.
    RecordMenu(Extent, Applied, Rows, RowCount);

    // 🔴 The filter card's dropdown is deferred, exactly as the scene directory's is.
    StackFacets.RecordDeferred();
}

void TexturePaintPanel::RecordStackHeader(const PlaneExtent& Header, TexturePaintContext& Applied,
                                          std::uint32_t RowCount)
{
    Surface->Ground(Header, Tinted.MenuLower, 0.0f, CornerNone);
    Surface->Ground(Spanning(Header.MinimumX, Header.MaximumY - 1.0f, Header.Width(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    const float Pad = Scaled.HeaderPadX;
    const float Run = 11.5f;
    const float TitleTop = Header.MinimumY + (Header.Height() - Run) * 0.5f;

    // 📐 "LAYERS" — the reference's uppercase, letter-spaced title.
    Surface->TextRunCapitalised(Header.MinimumX + Pad, TitleTop, Tinted.Muted,
                                "Layers", Run, 1.4f, true);

    // 📐 The count chip: "N · Mm" — the reference's `count()+' · '+maskCount()+'m'`.
    char Counted[24] = {};
    std::uint32_t Masks = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < TextureLayerCeiling; ++Ordinal)
    {
        if (Applied.MaskAttached[Ordinal])
            ++Masks;
    }

    std::snprintf(Counted, sizeof(Counted), "%u \u00B7 %um", RowCount, Masks);

    const float ChipRun = Scaled.RunFiner;
    const float ChipSpan = Surface->MeasureRun(Counted, ChipRun, 0.0f) + 18.0f;
    const float ChipY = Header.MinimumY + (Header.Height() - Scaled.LayerPillY) * 0.5f;

    const float TitleSpan = Surface->MeasureRun("Layers", Run, 1.4f);
    const PlaneExtent CountChip = Spanning(Header.MinimumX + Pad + TitleSpan + 10.0f, ChipY,
                                           ChipSpan, Scaled.LayerPillY);

    Surface->Ground(CountChip, Faded(Covering(0xFFFFFFu), 0.06f), ChipY * 0.5f, CornerAll);
    Surface->Edge(CountChip, Tinted.Hairline, 1.0f, ChipY * 0.5f, CornerAll);
    Surface->TextRun(CountChip.MinimumX + 9.0f,
                     CountChip.MinimumY + (CountChip.Height() - ChipRun) * 0.5f,
                     Tinted.Muted, Counted, ChipRun, 0.0f, true);

    // 📐 The SOLO chip, standing only while a row is solo'd — the reference's `body.soloing .solo`.
    float ButtonsLead = CountChip.MaximumX + Pad;

    if (Applied.SoloTaken < TextureLayerCeiling)
    {
        const char* SoloRun = "SOLO";
        const float SoloSpan = Surface->MeasureRun(SoloRun, ChipRun, 1.0f) + 18.0f;
        const PlaneExtent SoloPill = Spanning(ButtonsLead, ChipY, SoloSpan, Scaled.LayerPillY);

        Surface->Ground(SoloPill, Covering(0xFFD24Au), SoloPill.Height() * 0.5f, CornerAll);

        const bool OnSolo = SoloPill.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Sampled.ContactPressed && OnSolo && !Ledger->AnyDisclosed())
            Ledger->Grab(SoloChip, ControlPart::Body);

        if (OnSolo && Ledger->Released(SoloChip))
            Applied.SoloTaken = 0xFFFFFFFFu;

        Surface->TextRun(SoloPill.MinimumX + 9.0f,
                         SoloPill.MinimumY + (SoloPill.Height() - ChipRun) * 0.5f,
                         Covering(0x000000u), SoloRun, ChipRun, 1.0f, true);

        ButtonsLead = SoloPill.MaximumX + Pad;
    }

    // 📐 The header buttons, in the reference's own order: undo, redo, expand, then the solid Add —
    //    the Add stands rightmost, exactly as the HTML's `.head` places it.
    const float ButtonY = Scaled.LayerToolHeight;
    const float Gap = 6.0f;
    const float Figure = 15.0f;

    const auto CellAt = [&](float X)
    {
        return Spanning(X, Header.MinimumY + (Header.Height() - ButtonY) * 0.5f, ButtonY, ButtonY);
    };

    // The solid Add button: opens the Add menu.
    const float AddX = Header.MaximumX - Pad - ButtonY;
    const PlaneExtent AddCell = CellAt(AddX);
    const bool OnAdd = AddCell.Encloses(Sampled.PositionX, Sampled.PositionY);

    Surface->Ground(AddCell, OnAdd ? Faded(Covering(0xFFFFFFu), 0.16f)
                                   : Faded(Covering(0xFFFFFFu), 0.09f),
                    Scaled.LayerRadius, CornerAll);
    Surface->Edge(AddCell, Tinted.Hairline, 1.0f, Scaled.LayerRadius, CornerAll);

    Surface->Stroke(SymbolSubject::PlusCross,
                    Spanning(AddCell.MinimumX + (ButtonY - Figure) * 0.5f,
                             AddCell.MinimumY + (ButtonY - Figure) * 0.5f, Figure, Figure),
                    OnAdd ? Tinted.Primary : Covering(0x9A9A9Au));

    if (Sampled.ContactPressed && OnAdd && !Ledger->AnyDisclosed())
        Ledger->Grab(HeaderAdd, ControlPart::Body);

    if (OnAdd && Ledger->Released(HeaderAdd))
    {
        Ledger->Disclose(MenuAdd);
        Applied.MenuOpen = 1u;
        MenuAnchorExtent = AddCell;
    }

    // The expand toggle: cycles the wide columns.
    const float ExpandX = AddX - ButtonY - Gap;
    const PlaneExtent ExpandCell = CellAt(ExpandX);
    const bool OnExpand = ExpandCell.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (OnExpand)
        Surface->Ground(ExpandCell, Faded(Covering(0xFFFFFFu), 0.08f), Scaled.LayerRadius, CornerAll);

    Surface->Stroke(SymbolSubject::ExpandFrame,
                    Spanning(ExpandCell.MinimumX + (ButtonY - Figure) * 0.5f,
                             ExpandCell.MinimumY + (ButtonY - Figure) * 0.5f, Figure, Figure),
                    OnExpand ? Tinted.Primary : Covering(0x9A9A9Au));

    if (Sampled.ContactPressed && OnExpand && !Ledger->AnyDisclosed())
        Ledger->Grab(HeaderExpand, ControlPart::Body);

    if (OnExpand && Ledger->Released(HeaderExpand))
        Applied.WideRows = !Applied.WideRows;

    // Redo and undo — always disabled, matching the reference's empty-history state.
    const float RedoX = ExpandX - ButtonY - Gap;
    const PlaneExtent RedoCell = CellAt(RedoX);

    Surface->Stroke(SymbolSubject::RedoArrow,
                    Spanning(RedoCell.MinimumX + (ButtonY - Figure) * 0.5f,
                             RedoCell.MinimumY + (ButtonY - Figure) * 0.5f, Figure, Figure),
                    Faded(Covering(0x9A9A9Au), 0.25f));

    const float UndoX = RedoX - ButtonY - Gap;
    const PlaneExtent UndoCell = CellAt(UndoX);

    Surface->Stroke(SymbolSubject::UndoArrow,
                    Spanning(UndoCell.MinimumX + (ButtonY - Figure) * 0.5f,
                             UndoCell.MinimumY + (ButtonY - Figure) * 0.5f, Figure, Figure),
                    Faded(Covering(0x9A9A9Au), 0.25f));

}

void TexturePaintPanel::RecordStackTools(const PlaneExtent& Tools, TexturePaintContext& Applied)
{
    const float ToolY = Scaled.LayerToolHeight;
    const float Top = Tools.MinimumY + (Tools.Height() - ToolY) * 0.5f;

    // 📐 The search pill fills everything the three tools leave.
    const float IconSpan = ToolY * 3.0f + 10.0f * 2.0f + 6.0f;
    const PlaneExtent Search = Spanning(Tools.MinimumX + Scaled.PanePad, Top,
                                        Tools.Width() - Scaled.PanePad * 2.0f - IconSpan - 1.0f,
                                        ToolY);

    RecordSearchPill(Search, Applied);

    // 📐 The separator and the three tools: folder, mask, collapse.
    const float VSepY = Top + (ToolY - 17.0f) * 0.5f;
    Surface->Ground(Spanning(Search.MaximumX + 6.0f, VSepY, 1.0f, 17.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    const float Gap = 5.0f;
    float Lead = Search.MaximumX + 13.0f;

    struct ToolCell
    {
        ControlIdentity* Target;
        SymbolSubject    Glyph;
        std::uint32_t    Request;
    };

    const ToolCell ToolsDeclared[3] =
    {
        { &ToolFolder,   SymbolSubject::FolderClosed,   static_cast<std::uint32_t>(TexturePaintRequest::AddFolder) },
        { &ToolMask,     SymbolSubject::HalfMask,       0x80000000u },
        { &ToolCollapse, SymbolSubject::CollapseFold,   0u }
    };

    for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
    {
        const PlaneExtent Cell = Spanning(Lead, Top, ToolY, ToolY);
        const bool Hovered = Cell.Encloses(Sampled.PositionX, Sampled.PositionY);

        const bool MaskToolOn = (Ordinal == 1u) && Applied.LayerTaken < TextureLayerCeiling &&
                                Applied.MaskAttached[Applied.LayerTaken];

        if (Hovered)
            Surface->Ground(Cell, Faded(Covering(0xFFFFFFu), 0.08f), Scaled.LayerRadius, CornerAll);

        const float Figure = 15.0f;

        Surface->Stroke(ToolsDeclared[Ordinal].Glyph,
                        Spanning(Cell.MinimumX + (ToolY - Figure) * 0.5f,
                                 Cell.MinimumY + (ToolY - Figure) * 0.5f, Figure, Figure),
                        MaskToolOn ? Tinted.Primary
                                   : (Hovered ? Tinted.Primary : Covering(0x9A9A9Au)));

        if (Sampled.ContactPressed && Hovered && !Ledger->AnyDisclosed())
            Ledger->Grab(*ToolsDeclared[Ordinal].Target, ControlPart::Body);

        if (Hovered && Ledger->Released(*ToolsDeclared[Ordinal].Target))
        {
            if (Ordinal == 1u)
            {
                // 📐 The mask tool toggles the taken row's attached mask — the reference's `btnMask`.
                if (Applied.LayerTaken < TextureLayerCeiling)
                {
                    const std::uint32_t Taken = Applied.LayerTaken;
                    Applied.MaskAttached[Taken] = !Applied.MaskAttached[Taken];

                    if (Applied.MaskAttached[Taken])
                    {
                        Applied.MaskVisible[Taken] = true;
                        Applied.MaskTaken = true;
                    }
                    else
                    {
                        Applied.MaskTaken = false;
                    }
                }
            }
            else if (Ordinal == 2u)
            {
                // 📐 Collapse / expand all folders — the reference's `btnCollapse`.
                bool AnyOpen = false;

                for (std::uint32_t Row = 0u; Row < TextureLayerCeiling; ++Row)
                {
                    if (Applied.LayerExpanded[Row])
                    {
                        AnyOpen = true;
                        break;
                    }
                }

                const bool Open = !AnyOpen;

                for (std::uint32_t Row = 0u; Row < TextureLayerCeiling; ++Row)
                    Applied.LayerExpanded[Row] = Open;
            }
            else
            {
                Applied.Structural = ToolsDeclared[Ordinal].Request;
            }
        }

        Lead += ToolY + Gap;
    }
}

void TexturePaintPanel::RecordStackRow(const PlaneExtent& Row, TexturePaintContext& Applied,
                                       const TextureLayerRow* Rows, std::uint32_t RowCount,
                                       const TextureLayerRow& Current, std::uint32_t Ordinal)
{
    const bool Taken   = Applied.LayerTaken == Ordinal && !Applied.MaskTaken;
    // 🔴 The layer row's own hover excludes the mask strip beneath it: the row's extent carries the
    //    attached mask row too, and a click there must address the MASK, never the layer — the
    //    reported defect where the mask row could not be taken.
    const bool Hovered = Row.Encloses(Sampled.PositionX, Sampled.PositionY) &&
                         !(Applied.MaskAttached[Ordinal] &&
                           Sampled.PositionY >= Row.MinimumY + Scaled.LayerRowY);
    const bool Absent  = !Applied.LayerPresent[Ordinal];
    const bool Branch  = Current.EnclosedCount > 0u;
    const bool SoloDim = !RowInSolo(Applied, Rows, RowCount, Ordinal);

    // 📐 The row's geometry: the entry tag, then the chevron, the eye, the thumb, the meta, the
    //    chips and the details + more cells — the reference's `.row` order.
    const float LeadX = Row.MinimumX + Scaled.RowLeadX
                      + static_cast<float>(Current.Depth) * Scaled.RowStepX;

    const float ChevronY = Row.MinimumY + (Scaled.LayerRowY - Scaled.ChevronExtent) * 0.5f;

    const PlaneExtent Chevron = Spanning(LeadX, ChevronY,
                                         Scaled.ChevronExtent, Scaled.ChevronExtent);

    const float EyeExtent = Scaled.LayerToolHeight - 5.0f;
    const float EyeLead = Chevron.MaximumX + (Branch ? 8.0f : 0.0f) + 8.0f;
    const PlaneExtent Eye = Spanning(EyeLead,
                                     Row.MinimumY + (Scaled.LayerRowY - EyeExtent) * 0.5f,
                                     EyeExtent, EyeExtent);

    const float ThumbExtent = Scaled.LayerThumbY;
    const PlaneExtent Thumb = Spanning(Eye.MaximumX + 8.0f,
                                       Row.MinimumY + (Scaled.LayerRowY - ThumbExtent) * 0.5f,
                                       ThumbExtent, ThumbExtent);

    const float MetaLead = Thumb.MaximumX + Scaled.PanePad;
    const float RightGrip = Row.MaximumX - 6.0f;

    const PlaneExtent More = Spanning(RightGrip - EyeExtent,
                                      Row.MinimumY + (Scaled.LayerRowY - EyeExtent) * 0.5f,
                                      EyeExtent, EyeExtent);
    const PlaneExtent Details = Spanning(More.MinimumX - 4.0f - EyeExtent,
                                         More.MinimumY, EyeExtent, EyeExtent);

    const float RowCoverage = (Absent ? 0.34f : 1.0f) * (SoloDim ? 0.30f : 1.0f);

    const bool OnChevron = Branch && Chevron.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool OnEye     = Eye.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool OnDetails = Details.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool OnMore    = More.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Sampled.ContactPressed && !Ledger->AnyDisclosed())
    {
        if (OnChevron)
            Ledger->Grab(LayerChevrons[Ordinal], ControlPart::Chevron);
        else if (OnEye)
            Ledger->Grab(LayerEyes[Ordinal], ControlPart::Body);
        else if (OnDetails)
            Ledger->Grab(LayerDetails[Ordinal], ControlPart::Body);
        else if (OnMore)
            Ledger->Grab(LayerMores[Ordinal], ControlPart::Body);
        else if (Hovered)
            Ledger->Grab(LayerContacts[Ordinal], ControlPart::Body);
    }

    if (OnChevron && Ledger->Released(LayerChevrons[Ordinal]))
        Applied.LayerExpanded[Ordinal] = !Applied.LayerExpanded[Ordinal];

    if (OnEye && Ledger->Released(LayerEyes[Ordinal]))
        Applied.LayerPresent[Ordinal] = !Applied.LayerPresent[Ordinal];

    // 📐 The details chevron travels to the properties page — the reference's `exp` revealing the
    //    details, landed on the page the selection names.
    if (OnDetails && Ledger->Released(LayerDetails[Ordinal]))
    {
        Applied.LayerTaken   = Ordinal;
        Applied.MaskTaken    = false;
        Applied.StackPage    = 1u;
        Applied.PropertyTab  = 0u;
    }

    // 📐 The more button opens the layer menu — the reference's `layerMenu`.
    if (OnMore && Ledger->Released(LayerMores[Ordinal]))
    {
        Applied.LayerTaken = Ordinal;
        Applied.MaskTaken  = false;
        Applied.MenuOpen   = 2u;
        Applied.MenuRow    = Ordinal;
        Ledger->Disclose(MenuLayer);
        MenuAnchorExtent = More;
    }

    if (Hovered && !OnChevron && !OnEye && !OnDetails && !OnMore &&
        Ledger->Released(LayerContacts[Ordinal]))
    {
        Applied.LayerTaken = Ordinal;
        Applied.MaskTaken  = false;
    }

    Ledger->DeclareHovered(LayerContacts[Ordinal], Hovered, HoverOver);

    // ② The entry tag — the 3 px colour rail, dotted for a mask, dimmed for a hidden row.
    const std::uint32_t TagHue = Applied.LayerTagHue[Ordinal] != 0u
                               ? Applied.LayerTagHue[Ordinal] : Current.TagHue;

    if (Applied.MaskAttached[Ordinal])
    {
        // 📐 The reference's `.tag.dot`: 3 px on, 4 px off, at 0.85 coverage.
        for (float Y = Row.MinimumY; Y < Row.MinimumY + Scaled.LayerRowY; Y += 7.0f)
        {
            Surface->Ground(Spanning(Row.MinimumX, Y, Scaled.LayerTagX,
                                     std::min(3.0f, Row.MinimumY + Scaled.LayerRowY - Y)),
                            Faded(Covering(TagHue), (Absent ? 0.3f : 0.85f)), 0.0f, CornerNone);
        }
    }
    else
    {
        Surface->Ground(Spanning(Row.MinimumX, Row.MinimumY, Scaled.LayerTagX, Scaled.LayerRowY),
                        Faded(Covering(TagHue), Absent ? 0.3f : 1.0f), 0.0f, CornerNone);
    }

    // ③ The row ground — the reference's `#0d0d0d` row, its hover, and the selected pose.
    if (Taken)
    {
        Surface->Ground(Spanning(Row.MinimumX + Scaled.LayerTagX, Row.MinimumY,
                                 Row.Width() - Scaled.LayerTagX, Scaled.LayerRowY),
                        Covering(0x202020u), 0.0f, CornerNone);
        Surface->Edge(Spanning(Row.MinimumX + Scaled.LayerTagX, Row.MinimumY,
                               Row.Width() - Scaled.LayerTagX, Scaled.LayerRowY),
                      Faded(Covering(0xFFFFFFu), 0.18f), 1.0f, 0.0f, CornerNone);
    }
    else if (Hovered)
    {
        Surface->Ground(Spanning(Row.MinimumX + Scaled.LayerTagX, Row.MinimumY,
                                 Row.Width() - Scaled.LayerTagX, Scaled.LayerRowY),
                        Covering(0x161616u), 0.0f, CornerNone);
    }
    else
    {
        Surface->Ground(Spanning(Row.MinimumX + Scaled.LayerTagX, Row.MinimumY,
                                 Row.Width() - Scaled.LayerTagX, Scaled.LayerRowY),
                        Covering(0x0D0D0Du), 0.0f, CornerNone);
    }

    // ④ The disclosure chevron (folders only — the reference's `.tw.void` is skipped).
    if (Branch)
    {
        const bool Open = Applied.LayerExpanded[Ordinal];

        Surface->Stroke(Open ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                        Chevron, Faded(OnChevron ? Tinted.Primary : Covering(0x5E5E5Eu),
                                       RowCoverage));
    }

    // ⑤ The eye — always standing, exactly as the reference's `.eye` carries its 0.55 opacity.
    if (OnEye)
        Surface->Ground(Eye, Faded(Covering(0xFFFFFFu), 0.08f), 3.0f, CornerAll);

    Surface->Stroke(Absent ? SymbolSubject::EyeClosed : SymbolSubject::EyeOpen, Eye,
                    Faded(OnEye ? Tinted.Primary : Faded(Covering(0xFFFFFFu), Absent ? 0.4f : 0.55f),
                          RowCoverage));

    // ⑥ The square thumb: checker + folder glyph for folders, the hue wash + type badge for layers.
    const float Checker = ThumbExtent * 0.25f;

    for (std::uint32_t RowStep = 0u; RowStep < 4u; ++RowStep)
    {
        for (std::uint32_t Column = 0u; Column < 4u; ++Column)
        {
            const bool Light = ((RowStep + Column) & 1u) == 0u;

            Surface->Ground(Spanning(Thumb.MinimumX + Checker * static_cast<float>(Column),
                                     Thumb.MinimumY + Checker * static_cast<float>(RowStep),
                                     Checker + 0.5f, Checker + 0.5f),
                            Faded(Covering(Light ? 0x1C1C1Cu : 0x101010u), RowCoverage),
                            0.0f, CornerNone);
        }
    }

    Surface->Edge(Thumb, Faded(Covering(0xFFFFFFu), 0.15f * RowCoverage), 1.0f, 0.0f, CornerAll);

    if (Current.Classified == TextureLayerClassification::Folder)
    {
        const float Glyph = 16.0f;
        Surface->Stroke(SymbolSubject::FolderClosed,
                        Spanning(Thumb.MinimumX + (ThumbExtent - Glyph) * 0.5f,
                                 Thumb.MinimumY + (ThumbExtent - Glyph) * 0.5f, Glyph, Glyph),
                        Faded(Covering(TagHue), RowCoverage));
    }
    else
    {
        // 📐 The hue wash over the checker, then the badge with the type glyph — the reference's
        //    texture disc + `badge`.
        Surface->Ground(Thumb, Faded(Covering(TagHue), 0.30f * RowCoverage), 0.0f, CornerNone);

        const float BadgeExtent = Scaled.LayerBadgeY;
        const PlaneExtent Badge = Spanning(Thumb.MaximumX - BadgeExtent + 3.0f,
                                           Thumb.MaximumY + ThumbExtent - BadgeExtent - 3.0f,
                                           BadgeExtent, BadgeExtent);

        Surface->Ground(Badge, Faded(Covering(0x000000u), RowCoverage), Scaled.LayerRadius * 0.5f,
                        CornerAll);
        Surface->Edge(Badge, Faded(Covering(0xFFFFFFu), 0.18f), 1.0f,
                      Scaled.LayerRadius * 0.5f, CornerAll);

        const float Figure = BadgeExtent * 0.62f;
        Surface->Stroke(TextureLayerGlyph(Current.Classified),
                        Spanning(Badge.MinimumX + (BadgeExtent - Figure) * 0.5f,
                                 Badge.MinimumY + (BadgeExtent - Figure) * 0.5f, Figure, Figure),
                        Faded(Covering(0x9A9A9Au), RowCoverage));
    }

    // ⑦ The meta: name + the sub run.
    const float NamingRun  = Scaled.RunPrimary;
    const float NamingTop  = Row.MinimumY + (Scaled.LayerRowY * 0.5f - NamingRun * 1.3f) * 0.5f;
    const float NamingCeiling = Details.MinimumX - Scaled.PanePad;

    Surface->TextRunTruncated(MetaLead, NamingTop, NamingCeiling,
                              Faded(Taken ? Tinted.Primary : Tinted.Muted, RowCoverage),
                              Current.Naming, NamingRun, Taken);

    const float SubRun = Scaled.RunFine;
    const float SubTop = NamingTop + NamingRun * 1.3f;

    char Sub[96] = {};
    const bool IsFolder = Current.Classified == TextureLayerClassification::Folder;

    // 🔴 The sub-line used to restate the blend and the opacity, both of which
    //    the details panel already owns as their own rows, and both of which the
    //    wide columns draw again beside it. Blend appeared twice and opacity
    //    three times on one row.
    //    The stack answers "what is this and what is inside it"; the inspector
    //    answers "how is it set". So the sub-line now carries only what the row
    //    itself cannot: the classification, a folder's tally, and the resolution
    //    or source that identifies the content.
    if (IsFolder)
    {
        std::snprintf(Sub, sizeof(Sub), "%u items", Current.EnclosedCount);
    }
    else if (Current.Source[0] != '\0')
    {
        std::snprintf(Sub, sizeof(Sub), "%s \u00B7 %s",
                      TextureLayerText(Current.Classified), Current.Source);
    }
    else if (Current.Detail[0] != '\0')
    {
        std::snprintf(Sub, sizeof(Sub), "%s \u00B7 %s",
                      TextureLayerText(Current.Classified), Current.Detail);
    }
    else
    {
        std::snprintf(Sub, sizeof(Sub), "%s", TextureLayerText(Current.Classified));
    }

    Surface->TextRunTruncated(MetaLead, SubTop, NamingCeiling,
                              Faded(Tinted.Faint, RowCoverage), Sub, SubRun);

    // ⑧ The opacity column.
    // 🔴 The opacity used to appear three times over — in the sub-line, as a
    //    mini-bar, and as a value — and only when WideRows was toggled, so the
    //    one figure an artist reads constantly was the one that could vanish.
    //    A folder has no opacity of its own, so it states none.
    float ChipCeiling = Details.MinimumX - 8.0f;

    if (!IsFolder && NamingCeiling - MetaLead > 160.0f)
    {
        const std::uint32_t Amount = Applied.LayerOpacity[Ordinal];

        char Value[8] = {};
        std::snprintf(Value, sizeof(Value), "%u%%", Amount);

        const float ValueSpan = Surface->MeasureRun(Value, SubRun, 0.0f);
        const float ValueLead = ChipCeiling - ValueSpan;

        Surface->TextRun(ValueLead, Row.MinimumY + (Scaled.LayerRowY - SubRun) * 0.5f,
                         Faded(Taken ? Tinted.Primary : Tinted.Muted, RowCoverage),
                         Value, SubRun, 0.0f, true);

        ChipCeiling = ValueLead - 10.0f;

        // 📐 The blend run stays a wide-only column: it is a setting rather than
        //    a reading, and the inspector states it in full.
        if (Applied.WideRows && NamingCeiling - MetaLead > 300.0f)
        {
            const char* Blend = TextureBlendNames[Applied.LayerBlendTaken[Ordinal] % TextureBlendCount];
            const float BlendSpan = Surface->MeasureRun(Blend, SubRun, 0.0f);

            Surface->TextRun(ChipCeiling - BlendSpan,
                             Row.MinimumY + (Scaled.LayerRowY - SubRun) * 0.5f,
                             Faded(Tinted.Faint, RowCoverage), Blend, SubRun, 0.0f, true);

            ChipCeiling = ChipCeiling - BlendSpan - 10.0f;
        }
    }

    // ⑨ The chips, right-to-left: CH, FX, MASK, L, 3D — the reference's `.chips`.
    const float ChipRun = Scaled.RunFiner;
    const float ChipH = Scaled.LayerChipY;
    float ChipX = ChipCeiling;

    const auto DrawChip = [&](const char* Text, std::uint32_t Background, std::uint32_t Foreground,
                              const ThemeToken& Border) -> float
    {
        const float Span = Surface->MeasureRun(Text, ChipRun, 0.0f) + 14.0f;

        if (ChipX - Span >= MetaLead + 40.0f)
        {
            const PlaneExtent Chip = Spanning(ChipX - Span,
                                              Row.MinimumY + (Scaled.LayerRowY - ChipH) * 0.5f,
                                              Span, ChipH);

            Surface->Ground(Chip, Faded(Covering(Background), RowCoverage), ChipH * 0.5f, CornerAll);
            Surface->Edge(Chip, Faded(Border, RowCoverage), 1.0f, ChipH * 0.5f, CornerAll);
            Surface->TextRun(Chip.MinimumX + 7.0f,
                             Row.MinimumY + (Scaled.LayerRowY - ChipRun) * 0.5f,
                             Faded(Covering(Foreground), RowCoverage), Text, ChipRun, 0.0f, true);

            ChipX -= Span + 4.0f;
        }

        return ChipX;
    };

    char ChannelChip[12] = {};
    std::snprintf(ChannelChip, sizeof(ChannelChip), "%u/8 CH", Current.ChannelCount);

    if (Current.ChannelCount > 0u && !IsFolder)
        DrawChip(ChannelChip, 0x1E1E1Eu, 0x9A9A9Au, Tinted.Hairline);

    if (EffectCount(Current.Effects) > 0u)
    {
        char EffectChip[12] = {};
        std::snprintf(EffectChip, sizeof(EffectChip), "%u FX", EffectCount(Current.Effects));
        DrawChip(EffectChip, 0x22190Eu, 0xFFD9A0u, Covering(0x5A3A1Cu));
    }

    if (Applied.MaskAttached[Ordinal])
        DrawChip("MASK", 0x2A2A2Au, 0xDCDCDCu, Tinted.Hairline);

    if (Applied.LayerLocked[Ordinal])
        DrawChip("L", 0x2C1918u, 0xFF9D96u, Covering(0x4C2524u));

    if (Current.Classified == TextureLayerClassification::Decal)
        DrawChip("3D", 0x1B242Fu, 0xA9D8FFu, Covering(0x38506Au));

    // ⑩ The details and more cells.
    const float CellCoverage = (Hovered || Taken) ? 1.0f : 0.45f;

    if (OnDetails)
        Surface->Ground(Details, Faded(Covering(0xFFFFFFu), 0.08f), 3.0f, CornerAll);

    Surface->Stroke(SymbolSubject::ChevronRight,
                    Spanning(Details.MinimumX + (EyeExtent - 13.0f) * 0.5f,
                             Details.MinimumY + (EyeExtent - 13.0f) * 0.5f, 13.0f, 13.0f),
                    Faded(Faded(Tinted.Faint, CellCoverage), RowCoverage));

    if (OnMore)
        Surface->Ground(More, Faded(Covering(0xFFFFFFu), 0.08f), 3.0f, CornerAll);

    Surface->Stroke(SymbolSubject::EllipsisDots,
                    Spanning(More.MinimumX + (EyeExtent - 13.0f) * 0.5f,
                             More.MinimumY + (EyeExtent - 13.0f) * 0.5f, 13.0f, 13.0f),
                    Faded(Faded(Tinted.Faint, CellCoverage), RowCoverage));
}

void TexturePaintPanel::RecordMaskRow(const PlaneExtent& Row, TexturePaintContext& Applied,
                                      const TextureLayerRow* Rows, std::uint32_t RowCount,
                                      const TextureLayerRow& Current, std::uint32_t Ordinal)
{
    const bool Taken   = Applied.LayerTaken == Ordinal && Applied.MaskTaken;
    const bool Hovered = Row.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool Absent  = !Applied.MaskVisible[Ordinal];
    const bool SoloDim = !RowInSolo(Applied, Rows, RowCount, Ordinal);

    const float Coverage = (Absent ? 0.34f : 1.0f) * (SoloDim ? 0.30f : 1.0f);

    // 📐 The connector elbow — the reference's `.attach::before/::after`.
    const float SpineX = Row.MinimumX + 14.0f;

    Surface->Ground(Spanning(SpineX, Row.MinimumY, 1.0f, 23.0f),
                    Faded(Covering(0xFFFFFFu), 0.10f), 0.0f, CornerNone);
    Surface->Ground(Spanning(SpineX, Row.MinimumY + 23.0f, 11.0f, 1.0f),
                    Faded(Covering(0xFFFFFFu), 0.10f), 0.0f, CornerNone);

    // 📐 The mask row ground: near-transparent, dashed border, solid + selected when taken.
    const PlaneExtent Ground = Spanning(Row.MinimumX, Row.MinimumY, Row.Width(), Row.Height());

    if (Taken)
    {
        Surface->Ground(Ground, Covering(0x202020u), 0.0f, CornerNone);
        Surface->Edge(Ground, Faded(Covering(0xFFFFFFu), 0.18f), 1.0f, 0.0f, CornerNone);
    }
    else
    {
        Surface->Ground(Ground, Covering(Hovered ? 0x141414u : 0x0F0F0Fu), 0.0f, CornerNone);

        // 📐 The reference's dashed border, drawn as short segments.
        const float Segment = 10.0f;
        const float Gap = 4.0f;

        for (float X = Row.MinimumX + 1.0f; X < Row.MaximumX - 1.0f; X += Segment + Gap)
        {
            const float Span = std::min(Segment, Row.MaximumX - 1.0f - X);

            Surface->Ground(Spanning(X, Row.MinimumY, Span, 1.0f),
                            Faded(Covering(0xFFFFFFu), 0.10f), 0.0f, CornerNone);
            Surface->Ground(Spanning(X, Row.MaximumY - 1.0f, Span, 1.0f),
                            Faded(Covering(0xFFFFFFu), 0.10f), 0.0f, CornerNone);
        }

        Surface->Ground(Spanning(Row.MinimumX, Row.MinimumY, 1.0f, Row.Height()),
                        Faded(Covering(0xFFFFFFu), 0.10f), 0.0f, CornerNone);
        Surface->Ground(Spanning(Row.MaximumX - 1.0f, Row.MinimumY, 1.0f, Row.Height()),
                        Faded(Covering(0xFFFFFFu), 0.10f), 0.0f, CornerNone);
    }

    // 📐 The mask entry's own dotted colour tag — the reference's `.entry .tag.dot` on the attached
    //    entry, in the layer's colour, recorded above the ground so it always stands.
    const std::uint32_t TagHue = Applied.LayerTagHue[Ordinal] != 0u
                               ? Applied.LayerTagHue[Ordinal] : Current.TagHue;

    for (float Y = Row.MinimumY; Y < Row.MinimumY + Row.Height(); Y += 7.0f)
    {
        Surface->Ground(Spanning(Row.MinimumX, Y, Scaled.LayerTagX,
                                 std::min(3.0f, Row.MinimumY + Row.Height() - Y)),
                        Faded(Covering(TagHue), Absent ? 0.3f : 0.85f), 0.0f, CornerNone);
    }

    // 📐 The content: eye, mini thumb, MASK name + sub, chips, details and more — the reference's
    //    `.row.msk`.
    const float Lead = Row.MinimumX + Scaled.LayerMaskIndent;
    const float EyeExtent = Scaled.LayerToolHeight - 5.0f;
    const PlaneExtent Eye = Spanning(Lead,
                                     Row.MinimumY + (Row.Height() - EyeExtent) * 0.5f,
                                     EyeExtent, EyeExtent);

    const float Mini = Scaled.LayerThumbY - 8.0f;
    const PlaneExtent Thumb = Spanning(Eye.MaximumX + 8.0f,
                                       Row.MinimumY + (Row.Height() - Mini) * 0.5f,
                                       Mini, Mini);

    const float MetaLead = Thumb.MaximumX + Scaled.PanePad;
    const float RightGrip = Row.MaximumX - 6.0f;

    const PlaneExtent More = Spanning(RightGrip - EyeExtent,
                                      Row.MinimumY + (Row.Height() - EyeExtent) * 0.5f,
                                      EyeExtent, EyeExtent);
    const PlaneExtent Details = Spanning(More.MinimumX - 4.0f - EyeExtent,
                                         More.MinimumY, EyeExtent, EyeExtent);

    const bool OnEye     = Eye.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool OnDetails = Details.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool OnMore    = More.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Sampled.ContactPressed && Hovered && !Ledger->AnyDisclosed())
    {
        if (OnEye)
            Ledger->Grab(MaskEyes[Ordinal], ControlPart::Body);
        else if (OnDetails)
            Ledger->Grab(MaskDetails[Ordinal], ControlPart::Body);
        else if (OnMore)
            Ledger->Grab(MaskMores[Ordinal], ControlPart::Body);
        else
            Ledger->Grab(MaskContacts[Ordinal], ControlPart::Body);
    }

    if (OnEye && Ledger->Released(MaskEyes[Ordinal]))
        Applied.MaskVisible[Ordinal] = !Applied.MaskVisible[Ordinal];

    if (OnDetails && Ledger->Released(MaskDetails[Ordinal]))
    {
        Applied.LayerTaken  = Ordinal;
        Applied.MaskTaken   = true;
        Applied.StackPage   = 1u;
        Applied.PropertyTab = 1u;
    }

    if (OnMore && Ledger->Released(MaskMores[Ordinal]))
    {
        Applied.LayerTaken = Ordinal;
        Applied.MaskTaken  = true;
        Applied.MenuOpen   = 3u;
        Applied.MenuRow    = Ordinal;
        Ledger->Disclose(MenuMask);
        MenuAnchorExtent = More;
    }

    if (Hovered && !OnEye && !OnDetails && !OnMore && Ledger->Released(MaskContacts[Ordinal]))
    {
        Applied.LayerTaken = Ordinal;
        Applied.MaskTaken  = true;
    }

    Ledger->DeclareHovered(MaskContacts[Ordinal], Hovered, HoverOver);

    if (OnEye)
        Surface->Ground(Eye, Faded(Covering(0xFFFFFFu), 0.08f), 3.0f, CornerAll);

    Surface->Stroke(Absent ? SymbolSubject::EyeClosed : SymbolSubject::EyeOpen, Eye,
                    Faded(OnEye ? Tinted.Primary : Faded(Covering(0xFFFFFFu), Absent ? 0.4f : 0.55f),
                          Coverage));

    // 📐 The mini thumb: the mask's hue wash with the mask glyph.
    for (std::uint32_t RowStep = 0u; RowStep < 2u; ++RowStep)
    {
        for (std::uint32_t Column = 0u; Column < 2u; ++Column)
        {
            const bool Light = ((RowStep + Column) & 1u) == 0u;

            Surface->Ground(Spanning(Thumb.MinimumX + Mini * 0.5f * static_cast<float>(Column),
                                     Thumb.MinimumY + Mini * 0.5f * static_cast<float>(RowStep),
                                     Mini * 0.5f + 0.5f, Mini * 0.5f + 0.5f),
                            Faded(Covering(Light ? 0x1C1C1Cu : 0x101010u), Coverage),
                            0.0f, CornerNone);
        }
    }

    const float MaskGlyph = Mini * 0.55f;
    Surface->Stroke(SymbolSubject::HalfMask,
                    Spanning(Thumb.MinimumX + (Mini - MaskGlyph) * 0.5f,
                             Thumb.MinimumY + (Mini - MaskGlyph) * 0.5f, MaskGlyph, MaskGlyph),
                    Faded(Covering(0xDCDCDCu), Coverage));

    // 📐 The name and the sub run.
    const float NamingRun = Scaled.RunSmall;
    const float NamingTop = Row.MinimumY + (Row.Height() * 0.5f - NamingRun * 1.3f) * 0.5f;
    const float NamingCeiling = Details.MinimumX - Scaled.PanePad;

    Surface->TextRunCapitalised(MetaLead, NamingTop,
                                Faded(Taken ? Tinted.Primary : Covering(0x9A9A9Au), Coverage),
                                "Mask", NamingRun, 1.1f, true);

    const char* Source = Current.Source[0] != '\0'
                       ? Current.Source
                       : TextureMaskSourceNames[Applied.MaskSourceTaken[Ordinal] % 5u];

    char Sub[64] = {};
    std::snprintf(Sub, sizeof(Sub), "%s \u00B7 Gray 8 \u00B7 %u%%%s",
                  Source, Applied.MaskDensity[Ordinal],
                  Applied.MaskInverted[Ordinal] ? " \u00B7 INV" : "");

    Surface->TextRunTruncated(MetaLead, NamingTop + NamingRun * 1.3f, NamingCeiling,
                              Faded(Tinted.Faint, Coverage), Sub, Scaled.RunFine);

    // 📐 The chips: CH, FX.
    float ChipX = NamingCeiling;
    const float ChipRun = Scaled.RunFiner;
    const float ChipH = Scaled.LayerChipY;

    if (Current.ChannelCount > 0u && Current.ChannelCount < TextureChannelCeiling)
    {
        char Chip[12] = {};
        std::snprintf(Chip, sizeof(Chip), "%u CH", Current.ChannelCount);

        const float Span = Surface->MeasureRun(Chip, ChipRun, 0.0f) + 14.0f;
        const PlaneExtent ChipCell = Spanning(ChipX - Span,
                                              Row.MinimumY + (Row.Height() - ChipH) * 0.5f,
                                              Span, ChipH);

        Surface->Ground(ChipCell, Faded(Covering(0x1E1E1Eu), Coverage), ChipH * 0.5f, CornerAll);
        Surface->Edge(ChipCell, Faded(Tinted.Hairline, Coverage), 1.0f, ChipH * 0.5f, CornerAll);
        Surface->TextRun(ChipCell.MinimumX + 7.0f,
                         Row.MinimumY + (Row.Height() - ChipRun) * 0.5f,
                         Faded(Covering(0x9A9A9Au), Coverage), Chip, ChipRun, 0.0f, true);

        ChipX -= Span + 4.0f;
    }

    if (EffectCount(Current.Effects) > 0u)
    {
        char Chip[12] = {};
        std::snprintf(Chip, sizeof(Chip), "%u FX", EffectCount(Current.Effects));

        const float Span = Surface->MeasureRun(Chip, ChipRun, 0.0f) + 14.0f;
        const PlaneExtent ChipCell = Spanning(ChipX - Span,
                                              Row.MinimumY + (Row.Height() - ChipH) * 0.5f,
                                              Span, ChipH);

        Surface->Ground(ChipCell, Faded(Covering(0x22190Eu), Coverage), ChipH * 0.5f, CornerAll);
        Surface->Edge(ChipCell, Faded(Covering(0x5A3A1Cu), Coverage), 1.0f, ChipH * 0.5f, CornerAll);
        Surface->TextRun(ChipCell.MinimumX + 7.0f,
                         Row.MinimumY + (Row.Height() - ChipRun) * 0.5f,
                         Faded(Covering(0xFFD9A0u), Coverage), Chip, ChipRun, 0.0f, true);
    }

    // 📐 The details and more cells.
    const float CellCoverage = (Hovered || Taken) ? 1.0f : 0.45f;

    if (OnDetails)
        Surface->Ground(Details, Faded(Covering(0xFFFFFFu), 0.08f), 3.0f, CornerAll);

    Surface->Stroke(SymbolSubject::ChevronRight,
                    Spanning(Details.MinimumX + (EyeExtent - 13.0f) * 0.5f,
                             Details.MinimumY + (EyeExtent - 13.0f) * 0.5f, 13.0f, 13.0f),
                    Faded(Faded(Tinted.Faint, CellCoverage), Coverage));

    if (OnMore)
        Surface->Ground(More, Faded(Covering(0xFFFFFFu), 0.08f), 3.0f, CornerAll);

    Surface->Stroke(SymbolSubject::EllipsisDots,
                    Spanning(More.MinimumX + (EyeExtent - 13.0f) * 0.5f,
                             More.MinimumY + (EyeExtent - 13.0f) * 0.5f, 13.0f, 13.0f),
                    Faded(Faded(Tinted.Faint, CellCoverage), Coverage));
}

void TexturePaintPanel::RecordBarButton(ControlIdentity Target, const PlaneExtent& Cell,
                                        SymbolSubject Glyph, TexturePaintContext& Applied,
                                        std::uint32_t Request, bool Dimmed)
{
    const bool Hovered = Cell.Encloses(Sampled.PositionX, Sampled.PositionY);
    const float Coverage = Dimmed ? 0.25f : 1.0f;

    if (Hovered && !Dimmed)
        Surface->Ground(Cell, Faded(Covering(0xFFFFFFu), 0.08f), Scaled.LayerRadius, CornerAll);

    const float Figure = 15.0f;

    Surface->Stroke(Glyph,
                    Spanning(Cell.MinimumX + (Cell.Width() - Figure) * 0.5f,
                             Cell.MinimumY + (Cell.Height() - Figure) * 0.5f, Figure, Figure),
                    Faded(Hovered ? Tinted.Primary : Covering(0x9A9A9Au), Coverage));

    if (Sampled.ContactPressed && Hovered && !Dimmed && !Ledger->AnyDisclosed())
        Ledger->Grab(Target, ControlPart::Body);

    if (Hovered && !Dimmed && Ledger->Released(Target))
        Applied.Structural = Request;
}

void TexturePaintPanel::RecordStackFooter(const PlaneExtent& Footer, TexturePaintContext& Applied,
                                          const TextureLayerRow* Rows, std::uint32_t RowCount)
{
    Surface->Ground(Footer, Tinted.MenuLower, 0.0f, CornerNone);
    Surface->Ground(Spanning(Footer.MinimumX, Footer.MinimumY, Footer.Width(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    const float Pad = Scaled.PanePad;
    const bool Selection = Applied.LayerTaken < RowCount;
    const std::uint32_t Taken = Applied.LayerTaken;

    // ① The crumb — the reference's breadcrumb path.
    const float CrumbRun = Scaled.RunFiner;
    const float CrumbTop = Footer.MinimumY + (Scaled.LayerFootCrumb - CrumbRun) * 0.5f;

    char Crumb[160] = {};
    Crumb[0] = '\0';

    if (Selection)
    {
        // 📐 The path: the ancestors' names, then the taken name — the reference's `ancestors().map`.
        char Path[128] = {};
        std::uint32_t Steps[TextureLayerCeiling] = {};
        std::uint32_t StepCount = 0u;
        std::uint32_t Walking = Rows[Taken].Enclosing;

        while (Walking < RowCount && StepCount + 1u < TextureLayerCeiling)
        {
            Steps[StepCount++] = Walking;
            Walking = Rows[Walking].Enclosing;
        }

        for (std::uint32_t Step = StepCount; Step-- > 0u;)
        {
            std::snprintf(Path + std::strlen(Path), sizeof(Path) - std::strlen(Path), "%s \u203A ",
                          Rows[Steps[Step]].Naming);
        }

        std::snprintf(Crumb, sizeof(Crumb), "%s%s \u00B7 %s", Path, Rows[Taken].Naming,
                      Applied.MaskTaken
                          ? (Rows[Taken].Source[0] != '\0' ? Rows[Taken].Source
                                                           : "grayscale mask")
                          : TextureLayerText(Rows[Taken].Classified));
    }
    else
    {
        std::snprintf(Crumb, sizeof(Crumb), "no selection");
    }

    Surface->TextRunTruncated(Footer.MinimumX + Pad, CrumbTop,
                              Footer.MaximumX - Pad, Tinted.Faint, Crumb, CrumbRun);

    // ② The blend pill + the opacity slider — the reference's `.prop`.
    const float PropY = Footer.MinimumY + Scaled.LayerFootCrumb;
    const float PropH = Scaled.LayerFootProp;
    const float PillH = Scaled.LayerToolHeight - 1.0f;
    const float PillY = PropY + (PropH - PillH) * 0.5f;

    const bool MaskOn = Selection && Applied.MaskTaken;
    const bool BlendDim = !Selection || MaskOn;

    const float PillW = std::min(Footer.Width() * 0.45f, 190.0f);
    const PlaneExtent Pill = Spanning(Footer.MinimumX + Pad, PillY, PillW, PillH);

    Surface->Ground(Pill, Faded(Covering(0xFFFFFFu), BlendDim ? 0.03f : 0.06f),
                    PillH * 0.5f, CornerAll);
    Surface->Edge(Pill, Tinted.Hairline, 1.0f, PillH * 0.5f, CornerAll);

    const float PillRun = Scaled.RunSecondary;

    if (Selection)
    {
        const char* BlendText = Applied.MaskTaken
                              ? "Mask density"
                              : TextureBlendNames[Applied.LayerBlendTaken[Taken] % TextureBlendCount];

        Surface->TextRunTruncated(Pill.MinimumX + 13.0f,
                                  Pill.MinimumY + (PillH - PillRun) * 0.5f,
                                  Pill.MaximumX - 26.0f,
                                  BlendDim ? Faded(Tinted.Muted, 0.5f) : Tinted.Primary,
                                  BlendText, PillRun, true);

        const float Chev = 11.0f;
        Surface->Stroke(SymbolSubject::ChevronDown,
                        Spanning(Pill.MaximumX - 18.0f,
                                 Pill.MinimumY + (PillH - Chev) * 0.5f, Chev, Chev),
                        Faded(Tinted.Faint, BlendDim ? 0.4f : 1.0f));
    }

    if (!BlendDim)
    {
        if (Sampled.ContactPressed && Pill.Encloses(Sampled.PositionX, Sampled.PositionY) &&
            !Ledger->AnyDisclosed())
        {
            Ledger->Grab(BlendField, ControlPart::Body);
        }

        if (Pill.Encloses(Sampled.PositionX, Sampled.PositionY) && Ledger->Released(BlendField))
        {
            Ledger->Disclose(MenuBlend);
            Applied.MenuOpen = 4u;
            MenuAnchorExtent = Pill;
        }
    }

    // 📐 The opacity slider and its value pill.
    const float ValueW = 46.0f;
    const float TrackX = Pill.MaximumX + 12.0f;
    const float TrackW = Footer.MaximumX - Pad - ValueW - 8.0f - TrackX;
    const float TrackY = PropY + (PropH - 6.0f) * 0.5f;

    const std::uint32_t Amount = Selection
                               ? (MaskOn ? Applied.MaskDensity[Taken]
                                         : Applied.LayerOpacity[Taken])
                               : 100u;

    const float Fill = TrackW * static_cast<float>(Amount) / 100.0f;

    Surface->Ground(Spanning(TrackX, TrackY, TrackW, 6.0f),
                    Faded(Covering(0xFFFFFFu), 0.11f), 3.0f, CornerAll);
    Surface->Ground(Spanning(TrackX, TrackY, Fill, 6.0f),
                    Faded(Covering(0xFFFFFFu), BlendDim ? 0.5f : 1.0f), 3.0f, CornerAll);

    const float Knob = 15.0f;
    const float KnobX = TrackX + Held(Fill, Knob * 0.5f, TrackW - Knob * 0.5f);

    Surface->Medallion(KnobX, TrackY + 3.0f, Knob * 0.5f, Covering(0xFFFFFFu));
    Surface->Medallion(KnobX, TrackY + 3.0f, Knob * 0.5f + 2.0f, Faded(Covering(0xFFFFFFu), 0.35f));

    const bool OnTrack = TrackX <= Sampled.PositionX && Sampled.PositionX <= TrackX + TrackW &&
                         TrackY - 8.0f <= Sampled.PositionY && Sampled.PositionY <= TrackY + 14.0f;

    if (Sampled.ContactPressed && OnTrack && !Ledger->AnyDisclosed())
        Ledger->Grab(OpacityRow, ControlPart::Body);

    if (Ledger->Holding(OpacityRow) && Selection)
    {
        const float Fraction = Held((Sampled.PositionX - TrackX) / TrackW, 0.0f, 1.0f);
        const std::uint32_t Reading = static_cast<std::uint32_t>(Fraction * 100.0f + 0.5f);

        if (MaskOn)
            Applied.MaskDensity[Taken] = Reading;
        else
            Applied.LayerOpacity[Taken] = Reading;
    }

    const PlaneExtent ValuePill = Spanning(Footer.MaximumX - Pad - ValueW,
                                           PillY, ValueW, PillH);

    Surface->Ground(ValuePill, Faded(Covering(0xFFFFFFu), 0.06f), PillH * 0.5f, CornerAll);
    Surface->Edge(ValuePill, Tinted.Hairline, 1.0f, PillH * 0.5f, CornerAll);

    char Readout[8] = {};
    std::snprintf(Readout, sizeof(Readout), "%u%%", Amount);

    Surface->TextRun(ValuePill.MinimumX + (ValueW - Surface->MeasureRun(Readout, PillRun, 0.0f)) * 0.5f,
                     ValuePill.MinimumY + (PillH - PillRun) * 0.5f,
                     Selection ? Tinted.Primary : Faded(Tinted.Muted, 0.5f),
                     Readout, PillRun, 0.0f, true);

    // ③ The action bar — the reference's `.bar`.
    const float BarY = PropY + Scaled.LayerFootProp;
    const float BarH = Scaled.LayerFootBar;
    const float ButtonY = Scaled.LayerToolHeight;
    const float ButtonTop = BarY + (BarH - ButtonY) * 0.5f;

    const float VSepY = ButtonTop + (ButtonY - 17.0f) * 0.5f;

    float Lead = Footer.MinimumX + Pad;

    struct BarCell
    {
        SymbolSubject   Glyph;
        std::uint32_t   Request;
        bool            Always;
    };

    const BarCell Bar[12] =
    {
        { SymbolSubject::PaintBristle,      static_cast<std::uint32_t>(TexturePaintRequest::AddPaint),      true  },
        { SymbolSubject::DropletDrop,       static_cast<std::uint32_t>(TexturePaintRequest::AddFill),       true  },
        { SymbolSubject::AdjustmentSliders, static_cast<std::uint32_t>(TexturePaintRequest::AddAdjustment), true  },
        { SymbolSubject::FilterFunnel,      static_cast<std::uint32_t>(TexturePaintRequest::AddFilter),     true  },
        { SymbolSubject::StencilDecal,      static_cast<std::uint32_t>(TexturePaintRequest::AddDecal),      true  },
        { SymbolSubject::TiledPattern,      static_cast<std::uint32_t>(TexturePaintRequest::AddPattern),    true  },
        { SymbolSubject::FolderClosed,      static_cast<std::uint32_t>(TexturePaintRequest::Group),         false },
        { SymbolSubject::CopyDuplicate,     static_cast<std::uint32_t>(TexturePaintRequest::Duplicate),     false },
        { SymbolSubject::LockClosed,        0x80000001u,                                                    false },
        { SymbolSubject::ArrowUpLine,       static_cast<std::uint32_t>(TexturePaintRequest::MoveUp),        false },
        { SymbolSubject::ArrowDownLine,     static_cast<std::uint32_t>(TexturePaintRequest::MoveDown),      false },
        { SymbolSubject::TrashBin,          static_cast<std::uint32_t>(TexturePaintRequest::Delete),        false }
    };

    for (std::uint32_t Ordinal = 0u; Ordinal < 12u; ++Ordinal)
    {
        if (Ordinal == 4u || Ordinal == 6u || Ordinal == 10u)
        {
            Surface->Ground(Spanning(Lead + 5.0f, VSepY, 1.0f, 17.0f),
                            Tinted.Hairline, 0.0f, CornerNone);
            Lead += 11.0f;
        }

        if (Ordinal == 9u)
            Lead += 14.0f;   // 📐 the reference's `.gap` before the reorder pair

        const PlaneExtent Cell = Spanning(Lead, ButtonTop, ButtonY, ButtonY);

        if (Ordinal == 8u)
        {
            // 📐 The lock toggles the taken row's lock — a working copy, never a structural request.
            const bool Locked = Selection && Applied.LayerLocked[Taken];

            const bool Hovered = Cell.Encloses(Sampled.PositionX, Sampled.PositionY);

            if (Hovered && Selection)
                Surface->Ground(Cell, Faded(Covering(0xFFFFFFu), 0.08f), Scaled.LayerRadius, CornerAll);

            const float Figure = 15.0f;

            Surface->Stroke(Locked ? SymbolSubject::LockClosed : SymbolSubject::LockOpen,
                            Spanning(Cell.MinimumX + (ButtonY - Figure) * 0.5f,
                                     Cell.MinimumY + (ButtonY - Figure) * 0.5f, Figure, Figure),
                            Faded(Hovered ? Tinted.Primary : Covering(0x9A9A9Au),
                                  Selection ? 1.0f : 0.25f));

            if (Sampled.ContactPressed && Hovered && Selection && !Ledger->AnyDisclosed())
                Ledger->Grab(BarButtons[Ordinal], ControlPart::Body);

            if (Hovered && Selection && Ledger->Released(BarButtons[Ordinal]))
                Applied.LayerLocked[Taken] = !Applied.LayerLocked[Taken];
        }
        else
        {
            RecordBarButton(BarButtons[Ordinal], Cell, Bar[Ordinal].Glyph, Applied,
                            Bar[Ordinal].Request, !Bar[Ordinal].Always && !Selection);
        }

        Lead += ButtonY + 5.0f;
    }
}

void TexturePaintPanel::RecordMenu(const PlaneExtent& Extent, TexturePaintContext& Applied,
                                   const TextureLayerRow* Rows, std::uint32_t RowCount)
{
    const std::uint32_t Open = Applied.MenuOpen;

    if (Open == 0u)
        return;

    // 📐 The menu stands only while its disclosure stands — a dropdown opening elsewhere withdraws it.
    const bool Disclosed = (Open == 1u && Ledger->Disclosed(MenuAdd)) ||
                           (Open == 2u && Ledger->Disclosed(MenuLayer)) ||
                           (Open == 3u && Ledger->Disclosed(MenuMask)) ||
                           (Open == 4u && Ledger->Disclosed(MenuBlend));

    if (!Disclosed)
    {
        Applied.MenuOpen = 0u;
        return;
    }

    const float Pad = Scaled.PanePad;
    const float RowY = Scaled.LayerToolHeight + 2.0f;

    std::uint32_t OptionCount = 0u;
    const char* const* Captions = nullptr;
    const SymbolSubject* Glyphs = nullptr;
    const char* const* Shortcuts = nullptr;
    ControlIdentity* Identities = &MenuIdentities[0];
    const char* Title = "";

    // 📐 The four menus' declared items — the reference's popup content, trimmed to what the editor
    //    owns (no history spine, no colour wheel).
    static const char* const AddCaptions[7] =
    {
        "Paint layer", "Fill layer", "Adjustment", "Filter",
        "Decal layer \u00B7 3D", "Pattern layer", "Group"
    };
    static const SymbolSubject AddGlyphs[7] =
    {
        SymbolSubject::PaintBristle, SymbolSubject::DropletDrop,
        SymbolSubject::AdjustmentSliders, SymbolSubject::FilterFunnel,
        SymbolSubject::StencilDecal, SymbolSubject::TiledPattern, SymbolSubject::FolderClosed
    };
    static const char* const AddShortcuts[7] =
    {
        "P", "F", "A", "R", "D", "T", "G"
    };

    static const char* LayerCaptions[7] =
    {
        "Details", "Add mask", "Lock", "Solo", "Duplicate", "Group", "Delete"
    };
    static const SymbolSubject LayerGlyphs[7] =
    {
        SymbolSubject::ChevronRight, SymbolSubject::HalfMask, SymbolSubject::LockClosed,
        SymbolSubject::EyeOpen, SymbolSubject::CopyDuplicate, SymbolSubject::FolderClosed,
        SymbolSubject::TrashBin
    };
    static const char* const LayerShortcuts[7] =
    {
        "Space", "M", "L", "S", "\u2318D", "\u2318G", "\u232B"
    };

    static const char* const MaskCaptions[3] = { "Details", "Invert", "Delete mask" };
    static const SymbolSubject MaskGlyphs[3] =
    {
        SymbolSubject::ChevronRight, SymbolSubject::HalfMask, SymbolSubject::TrashBin
    };

    static const char* const BlendCaptions[TextureBlendCount] =
    {
        "Normal", "Passthrough", "Replace", "Multiply", "Screen", "Overlay",
        "Soft Light", "Hard Light", "Linear Dodge (Add)", "Color Dodge", "Linear Burn",
        "Difference", "Exclusion"
    };

    float CardY = MenuAnchorExtent.MaximumY + 6.0f;
    const float CardW = 206.0f;
    ControlIdentity* SwatchIdentities = &MenuIdentities[14];

    switch (Open)
    {
        case 1u:
            OptionCount = 7u;
            Captions  = AddCaptions;
            Glyphs    = AddGlyphs;
            Shortcuts = AddShortcuts;
            Title     = "Add";
            Identities = &MenuIdentities[0];
            break;
        case 2u:
        {
            OptionCount = 7u;
            Captions  = LayerCaptions;
            Glyphs    = LayerGlyphs;
            Shortcuts = LayerShortcuts;
            Title     = Rows != nullptr && Applied.MenuRow < RowCount
                      ? Rows[Applied.MenuRow].Naming : "Layer";
            Identities = &MenuIdentities[7];

            // 📐 The dynamic captions: mask and lock and solo follow the row's own state.
            LayerCaptions[1] = Applied.MaskAttached[Applied.MenuRow] ? "Remove mask" : "Add mask";
            LayerCaptions[2] = Applied.LayerLocked[Applied.MenuRow]  ? "Unlock"      : "Lock";
            LayerCaptions[3] = Applied.SoloTaken == Applied.MenuRow  ? "Clear solo"  : "Solo";
            break;
        }
        case 3u:
            OptionCount = 3u;
            Captions  = MaskCaptions;
            Glyphs    = MaskGlyphs;
            Shortcuts = nullptr;
            Title     = "Mask";
            Identities = &MenuIdentities[24];
            break;
        default:
            OptionCount = TextureBlendCount;
            Captions  = BlendCaptions;
            Glyphs    = nullptr;
            Shortcuts = nullptr;
            Title     = "Blend mode";
            Identities = &MenuIdentities[27];
            break;
    }

    // 📐 The card hangs right-aligned to its anchor, flipped above when the leaf has no room.
    const float CardH = Pad * 2.0f + 20.0f + RowY * static_cast<float>(OptionCount)
                      + (Open == 2u ? (RowY + 6.0f) : 0.0f);

    float CardX = MenuAnchorExtent.MaximumX - CardW;

    if (CardY + CardH > Extent.MaximumY - 6.0f)
        CardY = MenuAnchorExtent.MinimumY - CardH - 6.0f;

    CardX = Held(CardX, Extent.MinimumX + 6.0f, Extent.MaximumX - CardW - 6.0f);

    const PlaneExtent Card = Spanning(CardX, CardY, CardW, CardH);

    // 📐 A contact that arrived outside the card and its anchor withdraws the menu.
    if (Sampled.ContactPressed && !Card.Encloses(Sampled.PositionX, Sampled.PositionY) &&
        !MenuAnchorExtent.Encloses(Sampled.PositionX, Sampled.PositionY))
    {
        Applied.MenuOpen = 0u;

        if (Open == 1u) Ledger->Withdraw();
        if (Open == 2u) Ledger->Withdraw();
        if (Open == 3u) Ledger->Withdraw();
        if (Open == 4u) Ledger->Withdraw();
        return;
    }

    Surface->Ground(Card, Covering(0x0B0B0Bu), 16.0f, CornerAll);
    Surface->Edge(Card, Faded(Covering(0xFFFFFFu), 0.18f), 1.0f, 16.0f, CornerAll);

    Surface->TextRunCapitalised(Card.MinimumX + 10.0f, Card.MinimumY + 9.0f,
                                Tinted.Faint, Title, Scaled.RunFiner, 1.0f, true);

    // 📐 The items.
    std::uint32_t WritesLocal[TextureBlendCount] = {};

    RecordMenuOptions(Card, Captions, Glyphs, OptionCount, Shortcuts, Identities,
                      Applied, WritesLocal);

    if (Open == 1u)
    {
        for (std::uint32_t Ordinal = 0u; Ordinal < OptionCount; ++Ordinal)
        {
            if (WritesLocal[Ordinal] != 0u)
                Applied.Structural = static_cast<std::uint32_t>(TexturePaintRequest::AddPaint) + Ordinal;
        }
    }
    else if (Open == 2u)
    {
        if (WritesLocal[0] != 0u)
        {
            Applied.StackPage = 1u;
            Applied.PropertyTab = 0u;
        }

        if (WritesLocal[1] != 0u)
        {
            const std::uint32_t Row = Applied.MenuRow;
            Applied.MaskAttached[Row] = !Applied.MaskAttached[Row];

            if (Applied.MaskAttached[Row])
                Applied.MaskVisible[Row] = true;

            Applied.MaskTaken = false;
        }

        if (WritesLocal[2] != 0u)
            Applied.LayerLocked[Applied.MenuRow] = !Applied.LayerLocked[Applied.MenuRow];

        if (WritesLocal[3] != 0u)
            Applied.SoloTaken = (Applied.SoloTaken == Applied.MenuRow)
                              ? 0xFFFFFFFFu : Applied.MenuRow;

        if (WritesLocal[4] != 0u)
            Applied.Structural = static_cast<std::uint32_t>(TexturePaintRequest::Duplicate);

        if (WritesLocal[5] != 0u)
            Applied.Structural = static_cast<std::uint32_t>(TexturePaintRequest::Group);

        if (WritesLocal[6] != 0u)
            Applied.Structural = static_cast<std::uint32_t>(TexturePaintRequest::Delete);
    }
    else if (Open == 3u)
    {
        if (WritesLocal[0] != 0u)
        {
            Applied.StackPage = 1u;
            Applied.PropertyTab = 1u;
        }

        if (WritesLocal[1] != 0u)
            Applied.MaskInverted[Applied.MenuRow] = !Applied.MaskInverted[Applied.MenuRow];

        if (WritesLocal[2] != 0u)
        {
            Applied.MaskAttached[Applied.MenuRow] = false;
            Applied.MaskTaken = false;
        }
    }
    else if (Open == 4u)
    {
        for (std::uint32_t Ordinal = 0u; Ordinal < OptionCount; ++Ordinal)
        {
            if (WritesLocal[Ordinal] != 0u && Applied.LayerTaken < RowCount)
                Applied.LayerBlendTaken[Applied.LayerTaken] = Ordinal;
        }

        // 📐 The taken blend's check — the reference's check mark on the standing option.
        if (Applied.LayerTaken < RowCount)
        {
            const std::uint32_t TakenBlend = Applied.LayerBlendTaken[Applied.LayerTaken]
                                           % TextureBlendCount;
            const float CheckY = Card.MinimumY + Pad + 20.0f
                               + RowY * static_cast<float>(TakenBlend) + (RowY - 12.0f) * 0.5f;

            const float PointsX[3] = { Card.MinimumX + 12.0f, Card.MinimumX + 16.0f,
                                       Card.MinimumX + 23.0f };
            const float PointsY[3] = { CheckY + 6.5f, CheckY + 11.0f, CheckY + 3.5f };

            Surface->Polyline(PointsX, PointsY, 3u, Tinted.Primary, 1.6f);
        }
    }

    // 📐 The layer menu's colour swatches — the reference's `.swatches` row.
    if (Open == 2u)
    {
        const float SwatchY = Card.MinimumY + Pad * 2.0f + 20.0f + RowY * 7.0f + 6.0f;
        const float Swatch = 18.0f;
        const float Gap = (CardW - Pad * 2.0f - Swatch * 10.0f) / 9.0f;

        for (std::uint32_t SwatchOrdinal = 0u; SwatchOrdinal < TexturePaintContext::TextureSwatchCount;
             ++SwatchOrdinal)
        {
            const float X = Card.MinimumX + Pad + static_cast<float>(SwatchOrdinal) * (Swatch + Gap);
            const PlaneExtent Cell = Spanning(X, SwatchY, Swatch, Swatch);

            const bool On = Applied.LayerTagHue[Applied.MenuRow] == SwatchColours[SwatchOrdinal];

            Surface->Ground(Cell, Covering(SwatchColours[SwatchOrdinal]),
                            Swatch * 0.5f, CornerAll);
            Surface->Edge(Cell, Faded(Covering(0xFFFFFFu), On ? 1.0f : 0.16f), On ? 2.0f : 1.0f,
                          Swatch * 0.5f, CornerAll);

            const bool Hovered = Cell.Encloses(Sampled.PositionX, Sampled.PositionY);

            if (Hovered && Sampled.ContactPressed)
                Ledger->Grab(SwatchIdentities[SwatchOrdinal], ControlPart::Body);

            if (Hovered && Ledger->Released(SwatchIdentities[SwatchOrdinal]))
            {
                Applied.LayerTagHue[Applied.MenuRow] = SwatchColours[SwatchOrdinal];
                Applied.MenuOpen = 0u;
                Ledger->Withdraw();
            }
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PROPERTIES PAGE
//------------------------------------------------------------------------------------------------------------------------

void TexturePaintPanel::RecordPropertiesPage(const PlaneExtent& Extent, TexturePaintContext& Applied,
                                             const TextureLayerRow* Rows, std::uint32_t RowCount)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    if (RowCount == 0u || Applied.LayerTaken >= RowCount)
    {
        const float Run = Scaled.RunSecondary;
        const char* Prose = "Select a layer in the stack, then press Tab.";

        Surface->TextRun(Extent.MinimumX + (Extent.Width()
                                              - Surface->MeasureRun(Prose, Run, 0.0f)) * 0.5f,
                         Extent.MinimumY + Scaled.HeaderHeight, Tinted.Faint, Prose, Run);
        return;
    }

    const TextureLayerRow& Current = Rows[Applied.LayerTaken];
    const ThemeToken Hue = TextureLayerHue(Current.Classified);

    const PlaneExtent Header = Spanning(Extent.MinimumX, Extent.MinimumY,
                                        Extent.Width(), Scaled.HeaderHeight);

    char Classified[48] = {};
    std::snprintf(Classified, sizeof(Classified), "%s \u00B7 %s",
                  TextureLayerText(Current.Classified),
                  TextureBlendNames[Applied.LayerBlendTaken[Applied.LayerTaken] % TextureBlendCount]);

    RecordLeafHeader(Header, TextureLayerGlyph(Current.Classified), Hue, Current.Naming, Classified);

    // 📐 The property strip — the tabs the selection offers.
    const char* TabCaptions[3] = { "Channels", "Mask", "Settings" };

    if (Current.Classified == TextureLayerClassification::Folder)
        TabCaptions[0] = "Stack";

    if (Applied.MaskTaken)
    {
        TabCaptions[0] = "Mask";
        TabCaptions[1] = nullptr;
        TabCaptions[2] = nullptr;
    }

    const std::uint32_t TabCount = PropertyTabCount(Applied, Current);

    if (Applied.PropertyTab >= TabCount)
        Applied.PropertyTab = 0u;

    const TabDeclaration Declared{ TabCaptions, TabCount };
    static_cast<void>(Controls.TabStrip(PropertyStrip,
                                        Spanning(Extent.MinimumX, Header.MaximumY,
                                                 Extent.Width(), Scaled.ComponentY),
                                        Declared, Applied.PropertyTab));

    const PlaneExtent Pages = Spanning(Extent.MinimumX, Header.MaximumY + Scaled.ComponentY,
                                       Extent.Width(),
                                       Extent.MaximumY - Header.MaximumY - Scaled.ComponentY
                                       - Scaled.FooterHeight);

    if (Pages.MaximumY <= Pages.MinimumY)
        return;

    Surface->Confine(Pages);

    switch (Applied.PropertyTab)
    {
        case 0u:
            if (Current.Classified == TextureLayerClassification::Folder)
                RecordFolderCard(Pages, Applied, Rows, RowCount);
            else if (Applied.MaskTaken)
                RecordMaskCard(Pages, Applied, Current);
            else
                RecordChannelCard(Pages, Applied, Current);
            break;
        case 1u:
            RecordMaskCard(Pages, Applied, Current);
            break;
        default:
            RecordSettingsCard(Pages, Applied, Current);
            break;
    }

    Surface->Release();

    // 📐 The footer: the selection's small summary.
    const PlaneExtent Footer = Spanning(Extent.MinimumX, Extent.MaximumY - Scaled.FooterHeight,
                                        Extent.Width(), Scaled.FooterHeight);

    Surface->Ground(Footer, Tinted.MenuLower, 0.0f, CornerNone);
    Surface->Ground(Spanning(Footer.MinimumX, Footer.MinimumY, Footer.Width(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    char Summary[64] = {};
    std::snprintf(Summary, sizeof(Summary), "%u%% \u00B7 %s",
                  Applied.LayerOpacity[Applied.LayerTaken],
                  Applied.MaskTaken ? "mask" : TextureLayerText(Current.Classified));

    const float FootRun = Scaled.RunFine;
    const float FootTop = Footer.MinimumY + (Footer.Height() - FootRun) * 0.5f;

    Surface->Ground(Spanning(Footer.MinimumX + Scaled.HeaderPadX,
                             Footer.MinimumY + (Footer.Height() - Scaled.ChipExtent) * 0.5f,
                             Scaled.ChipExtent, Scaled.ChipExtent), Hue, 2.0f, CornerAll);

    Surface->TextRun(Footer.MinimumX + Scaled.HeaderPadX + Scaled.ChipExtent + Scaled.PanePad,
                     FootTop, Tinted.Muted, Summary, FootRun);

    ChannelFacets.RecordDeferred();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE CHANNEL PROPERTIES
//------------------------------------------------------------------------------------------------------------------------

void TexturePaintPanel::RecordChannelCard(const PlaneExtent& Extent, TexturePaintContext& Applied,
                                          const TextureLayerRow& Current)
{
    const float Pad = Scaled.PanePad;

    // 📐 The card's own filter: a search pill filtering the channel names and the channel-group
    //    facets — the SAME filter pair as the stack page and the scene directory.
    const PlaneExtent Search = Spanning(Extent.MinimumX + Pad, Extent.MinimumY + Pad,
                                        Extent.Width() - Pad * 2.0f, Scaled.SearchHeight);

    RecordSearchPill(Search, Applied);

    const FacetDeclaration ChannelFacetCard =
    {
        "Channels", ChannelFacetOptions, ChannelFacetColours,
        TexturePaintContext::TextureChannelFacetCount, 0xFFFFFFFFu
    };

    const float FacetY = ChannelFacets.MeasureHeight(Extent.Width() - Pad * 2.0f, ChannelFacetCard,
                                                     Applied.ChannelFacet);

    const PlaneExtent FacetCard = Spanning(Extent.MinimumX + Pad, Search.MaximumY + Pad,
                                           Extent.Width() - Pad * 2.0f, FacetY);

    Discard(ChannelFacets.Record(FacetCard, ChannelFacetCard, Applied.ChannelFacet));

    const float BodyTop = FacetCard.MaximumY + Pad;

    if (BodyTop >= Extent.MaximumY - Pad)
        return;

    const PlaneExtent Body = Spanning(Extent.MinimumX + Pad, BodyTop,
                                      Extent.Width() - Pad * 2.0f,
                                      Extent.MaximumY - BodyTop - Pad);

    Surface->Confine(Body);

    float Sweep = Body.MinimumY;

    const bool Filtering = ChannelRetentionActive(Applied);

    for (std::uint32_t Channel = 0u; Channel < TextureChannelCeiling; ++Channel)
    {
        const char* Name = TextureChannelText(Channel);

        if (Filtering)
        {
            const bool InSearch = Applied.Retention[0] == '\0' ||
                                  RunHolds(Name, Applied.Retention);

            bool InFacet = true;
            for (std::uint32_t Facet = 0u; Facet < TexturePaintContext::TextureChannelFacetCount; ++Facet)
            {
                if (Applied.ChannelFacet[Facet])
                {
                    InFacet = Applied.ChannelFacet[TextureChannelGroup(Channel)];
                    break;
                }
            }

            if (!InSearch || !InFacet)
                continue;
        }

        const float RowY = Scaled.LayerHeadHeight * 0.82f;

        // 📐 The card is a ground, its head, and — while unfolded — its body. The
        //    fold animates on the shared expansion so it opens over 200 ms rather
        //    than appearing between two frames.
        const float Opened = Controls.OutlineExpansion(ChannelFolds[Channel],
                                                       !Applied.ChannelFolded[Channel], true);

        const float BodyY = ChannelBodyHeight(Applied, Channel) * Opened;
        const PlaneExtent Card = Spanning(Body.MinimumX, Sweep, Body.Width(), RowY + BodyY);

        if (Sweep + RowY > Body.MaximumY)
            break;

        Surface->Ground(Card, Tinted.Tile, Scaled.LayerRadius, CornerAll);
        Surface->Edge(Card, Tinted.Hairline, 1.0f, Scaled.LayerRadius, CornerAll);

        const PlaneExtent Row = Spanning(Card.MinimumX + Scaled.PanePad, Sweep,
                                         Card.Width() - Scaled.PanePad * 2.0f, RowY);

        RecordChannelRow(Row, Applied, Channel);

        if (BodyY > 0.5f)
        {
            const PlaneExtent Inner = Spanning(Card.MinimumX + Scaled.PanePad * 1.5f,
                                               Card.MinimumY + RowY,
                                               Card.Width() - Scaled.PanePad * 3.0f, BodyY);

            Surface->Confine(Inner);
            static_cast<void>(RecordChannelBody(Inner, Applied, Channel));
            Surface->Release();
        }

        Sweep += RowY + BodyY + Scaled.LayerRowGap;
    }

    Surface->Release();
}

void TexturePaintPanel::RecordChannelRow(const PlaneExtent& Row, TexturePaintContext& Applied,
                                         std::uint32_t Channel)
{
    // 🔴 This row hand-rolled every control it needed: a Medallion for the
    //    on/off, a two-Ground mini-bar for the amount, and a click that CYCLED
    //    the blend because there was no menu. None of it matched the reusable
    //    set, so the same three controls looked and behaved differently here
    //    than in every other card. It now spends SwitchTrack, MagnitudeRow and
    //    SelectionField, and the row reads the channel's own schema rather than
    //    assuming every channel is a 0..100 scalar.
    const std::uint32_t Layer = Applied.LayerTaken;
    const TextureChannelSlot& Slot = TextureChannelAt(Channel);

    const bool On     = Applied.ChannelOn[Layer][Channel];
    const bool Folded = Applied.ChannelFolded[Channel];

    // ① The head: chevron, swatch, label, and the mode the card is authored in.
    const PlaneExtent Fold = Spanning(Row.MinimumX,
                                      Row.MinimumY + (Row.Height() - Scaled.ChevronExtent) * 0.5f,
                                      Scaled.ChevronExtent, Scaled.ChevronExtent);

    const bool OnFold = Fold.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Sampled.ContactPressed && OnFold && !Ledger->AnyDisclosed())
        Ledger->Grab(ChannelFolds[Channel], ControlPart::Chevron);

    if (OnFold && Ledger->Released(ChannelFolds[Channel]))
        Applied.ChannelFolded[Channel] = !Folded;

    Surface->Stroke(Folded ? SymbolSubject::ChevronRight : SymbolSubject::ChevronDown,
                    Fold, Tinted.Faint);

    // 📐 The swatch is the channel's own hue, as the reference's `.ch-dot` is.
    const float Swatch = 9.0f;
    Surface->Medallion(Fold.MaximumX + Scaled.PanePad + Swatch * 0.5f,
                       Row.MinimumY + Row.Height() * 0.5f, Swatch * 0.5f,
                       Faded(Covering(Slot.Hue), On ? 1.0f : 0.35f));

    const float NameRun = Scaled.RunPrimary;

    Surface->TextRun(Fold.MaximumX + Scaled.PanePad * 2.0f + Swatch,
                     Row.MinimumY + (Row.Height() - NameRun) * 0.5f,
                     On ? Tinted.Primary : Tinted.Muted, Slot.Label, NameRun);

    // ② The trailing switch — the shared pill, not a bare medallion.
    const float SwitchY = 14.0f;
    const float SwitchX = SwitchY * (50.0f / 32.0f);
    const PlaneExtent Switch = Spanning(Row.MaximumX - SwitchX,
                                        Row.MinimumY + (Row.Height() - SwitchY) * 0.5f,
                                        SwitchX, SwitchY);

    const bool OnSwitch = Switch.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Sampled.ContactPressed && OnSwitch && !Ledger->AnyDisclosed())
        Ledger->Grab(ChannelDots[Channel], ControlPart::Body);

    if (OnSwitch && Ledger->Released(ChannelDots[Channel]))
        Applied.ChannelOn[Layer][Channel] = !On;

    Ledger->DeclareHovered(ChannelDots[Channel], OnSwitch, HoverOver);

    Controls.SwitchTrack(ChannelDots[Channel], Switch, On,
                         Covering(Slot.Hue), Tinted.Hairline, Covering(0xFFFFFFu));

    // 📐 The placement run states which atlas lane the channel occupies, which is
    //    what the reference puts under the title.
    const float PlaceRun = Scaled.RunFiner;
    const float PlaceSpan = Surface->MeasureRun(Slot.Placement, PlaceRun, 0.0f);

    Surface->TextRun(Switch.MinimumX - Scaled.PanePad - PlaceSpan,
                     Row.MinimumY + (Row.Height() - PlaceRun) * 0.5f,
                     Tinted.Faint, Slot.Placement, PlaceRun);
}

// 🧩 How tall an unfolded channel card stands, so the fold has a figure to
//    animate toward rather than snapping open.
float TexturePaintPanel::ChannelBodyHeight(const TexturePaintContext& Applied,
                                           std::uint32_t Channel) const
{
    const TextureChannelSlot& Slot = TextureChannelAt(Channel);
    const float RowY = Appearance->ControlMeasure.FieldHeight + Scaled.PanePad * 0.5f;

    // the source picker always stands; a scalar adds its amount row
    float Height = RowY;

    if (Slot.Edit == TextureChannelEdit::Scalar)
        Height += RowY;
    else if (Slot.Edit == TextureChannelEdit::Derived)
        Height += RowY;

    return Height + Scaled.PanePad;
}

float TexturePaintPanel::RecordChannelBody(const PlaneExtent& Extent, TexturePaintContext& Applied,
                                           std::uint32_t Channel)
{
    // 🧩 The unfolded card: a source picker, then the field the edit kind calls
    //    for. Every one is a shared component.
    const std::uint32_t Layer = Applied.LayerTaken;
    const TextureChannelSlot& Slot = TextureChannelAt(Channel);
    const float RowY = Appearance->ControlMeasure.FieldHeight + Scaled.PanePad * 0.5f;

    float Sweep = Extent.MinimumY;

    // ① Source — Value, Texture or Generator, exactly the reference's picker.
    static const char* const Sources[3] = { "Value", "Texture", "Generator" };

    SelectionDeclaration Source;
    Source.Caption       = "Source";
    Source.Options       = Sources;
    Source.OptionCount   = 3u;

    std::uint32_t Taken = Applied.ChannelMode[Layer][Channel] % 3u;

    if (SharedControls.SelectionField(ChannelBlends[Channel],
                                      Spanning(Extent.MinimumX, Sweep, Extent.Width(), RowY),
                                      Source, Taken).ReadingAltered)
    {
        Applied.ChannelMode[Layer][Channel] = Taken;
    }

    Sweep += RowY;

    // ② A derived channel has nothing to author, and says so rather than
    //    offering a field that would not be read.
    if (Slot.Edit == TextureChannelEdit::Derived)
    {
        Surface->TextRun(Extent.MinimumX, Sweep + Scaled.PanePad,
                         Tinted.Faint,
                         "Derived from the painted height. No value to author.",
                         Scaled.RunFine);

        return Sweep + RowY - Extent.MinimumY;
    }

    // ③ Amount — over the channel's OWN span. Anisotropy Angle runs to 360 and
    //    Refraction Index starts at 1; the old row put every channel on 0..100.
    if (Slot.Edit == TextureChannelEdit::Scalar)
    {
        MagnitudeDeclaration Amount;
        Amount.Caption   = "Amount";
        Amount.UnitGlyph = Slot.Unit;
        Amount.Minimum   = Slot.Minimum;
        Amount.Maximum   = Slot.Maximum;

        double Reading = Applied.ChannelReading[Layer][Channel];

        if (SharedControls.MagnitudeRow(ChannelOps[Channel],
                                        Spanning(Extent.MinimumX, Sweep, Extent.Width(), RowY),
                                        Amount, Reading, false).ReadingAltered)
        {
            Applied.ChannelReading[Layer][Channel] = Reading;
        }

        Sweep += RowY;
    }

    return Sweep - Extent.MinimumY;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE MASK PROPERTIES
//------------------------------------------------------------------------------------------------------------------------

void TexturePaintPanel::RecordMaskCard(const PlaneExtent& Extent, TexturePaintContext& Applied,
                                       const TextureLayerRow& Current)
{
    const float Pad = Scaled.PanePad;
    const float RowY = Scaled.RowHeight * 0.82f;
    float Sweep = Extent.MinimumY + Pad;

    Surface->Ground(Spanning(Extent.MinimumX + Pad, Sweep, Extent.Width() - Pad * 2.0f,
                             Scaled.ComponentY),
                    Tinted.Desk, Scaled.CardRadius, CornerAll);
    Surface->Edge(Spanning(Extent.MinimumX + Pad, Sweep, Extent.Width() - Pad * 2.0f,
                           Scaled.ComponentY),
                  Tinted.Hairline, 1.0f, Scaled.CardRadius, CornerAll);

    const char* Source = Current.Source[0] != '\0'
                       ? Current.Source
                       : TextureMaskSourceNames[Applied.MaskSourceTaken[Applied.LayerTaken] % 5u];

    char MaskCaption[64] = {};
    std::snprintf(MaskCaption, sizeof(MaskCaption), "Mask \u00B7 %s", Source);

    Surface->TextRunCapitalised(Extent.MinimumX + Pad * 2.0f, Sweep + 7.0f,
                                Tinted.Primary, MaskCaption, Scaled.RunSmall, 0.0f, true);

    Sweep += Scaled.ComponentY + Pad;

    // ① The density slider.
    const PlaneExtent DensityRow = Spanning(Extent.MinimumX + Pad, Sweep,
                                            Extent.Width() - Pad * 2.0f, RowY);

    Surface->TextRun(DensityRow.MinimumX + Scaled.PanePad, DensityRow.MinimumY,
                     Tinted.Muted, "Density", Scaled.RunSecondary);

    const float BarX = DensityRow.MinimumX + 90.0f;
    const float BarY = DensityRow.MinimumY + (RowY - 4.0f) * 0.5f;
    const std::uint32_t Density = Applied.MaskDensity[Applied.LayerTaken];

    Surface->Ground(Spanning(BarX, BarY, 120.0f, 4.0f), Covering(0x242424u), 2.0f, CornerAll);
    Surface->Ground(Spanning(BarX, BarY, 120.0f * static_cast<float>(Density) / 100.0f, 4.0f),
                    Covering(0xFFFFFFu), 2.0f, CornerAll);

    if (Sampled.ContactPressed && BarX < Sampled.PositionX && Sampled.PositionX < BarX + 120.0f &&
        !Ledger->AnyDisclosed())
    {
        Ledger->Grab(MaskRows[0], ControlPart::Body);
    }

    if (Ledger->Holding(MaskRows[0]))
    {
        const float Fraction = Held((Sampled.PositionX - BarX) / 120.0f, 0.0f, 1.0f);
        Applied.MaskDensity[Applied.LayerTaken] = static_cast<std::uint32_t>(Fraction * 100.0f + 0.5f);
    }

    char DensityText[8] = {};
    std::snprintf(DensityText, sizeof(DensityText), "%u%%", Density);

    Surface->TextRun(DensityRow.MaximumX - 44.0f, DensityRow.MinimumY,
                     Tinted.Primary, DensityText, Scaled.RunSecondary);

    Sweep += RowY + Scaled.LayerRowGap;

    // ② The source selection.
    const PlaneExtent SourceRow = Spanning(Extent.MinimumX + Pad, Sweep,
                                           Extent.Width() - Pad * 2.0f, RowY);

    Surface->TextRun(SourceRow.MinimumX + Scaled.PanePad, SourceRow.MinimumY,
                     Tinted.Muted, "Source", Scaled.RunSecondary);

    SelectionDeclaration SourceDeclared;
    SourceDeclared.Caption     = "Source";
    SourceDeclared.Options     = TextureMaskSourceNames;
    SourceDeclared.OptionCount = 5u;

    std::uint32_t SourceTaken = Applied.MaskSourceTaken[Applied.LayerTaken] % 5u;

    SharedControls.SelectionField(MaskRows[1],
                                  Spanning(SourceRow.MinimumX + 90.0f, SourceRow.MinimumY,
                                           SourceRow.Width() - 90.0f, RowY),
                                  SourceDeclared, SourceTaken);

    Applied.MaskSourceTaken[Applied.LayerTaken] = SourceTaken;

    Sweep += RowY + Scaled.LayerRowGap;

    // ③ The invert toggle.
    const PlaneExtent InvertRow = Spanning(Extent.MinimumX + Pad, Sweep,
                                           Extent.Width() - Pad * 2.0f, RowY);

    const bool OnInvert = InvertRow.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool Inverted = Applied.MaskInverted[Applied.LayerTaken];

    if (Sampled.ContactPressed && OnInvert && !Ledger->AnyDisclosed())
        Ledger->Grab(MaskRows[2], ControlPart::Body);

    if (OnInvert && Ledger->Released(MaskRows[2]))
        Applied.MaskInverted[Applied.LayerTaken] = !Inverted;

    const float Toggle = Scaled.ChipExtent * 2.5f;
    const PlaneExtent Switch = Spanning(InvertRow.MaximumX - Toggle - Pad,
                                        InvertRow.MinimumY + (RowY - Toggle * 0.5f) * 0.5f,
                                        Toggle, Toggle * 0.5f);

    // 🔴 Another hand-rolled copy of the pill; see ControlPanel::SwitchTrack.
    Controls.SwitchTrack(MaskRows[2], Switch, Inverted,
                         Tinted.EntityAccent, Tinted.Hairline, Covering(0xFFFFFFu));

    Surface->TextRun(InvertRow.MinimumX + Scaled.PanePad, InvertRow.MinimumY,
                     OnInvert ? Tinted.Primary : Tinted.Muted, "Invert", Scaled.RunSecondary);

    Sweep += RowY + Scaled.LayerRowGap;

    // ④ The applies-to channel chips.
    const PlaneExtent ChipsRow = Spanning(Extent.MinimumX + Pad, Sweep,
                                          Extent.Width() - Pad * 2.0f, RowY * 1.6f);

    Surface->TextRun(ChipsRow.MinimumX + Scaled.PanePad, ChipsRow.MinimumY,
                     Tinted.Muted, "Applies to", Scaled.RunSecondary);

    float ChipX = ChipsRow.MinimumX + 90.0f;

    for (std::uint32_t Channel = 0u; Channel < TextureChannelCeiling && ChipX < ChipsRow.MaximumX - 40.0f;
         ++Channel)
    {
        const bool On = Applied.ChannelOn[Applied.LayerTaken][Channel];
        const char* Name = TextureChannelText(Channel);
        const float ChipRun = Scaled.RunFiner;
        const float ChipSpan = Surface->MeasureRun(Name, ChipRun, 0.0f) + 14.0f;

        Surface->Ground(Spanning(ChipX, ChipsRow.MinimumY + 2.0f, ChipSpan, 17.0f),
                        On ? Covering(0x2A2A2Au) : Covering(0x141414u), 9.0f, CornerAll);

        Surface->TextRun(ChipX + 7.0f, ChipsRow.MinimumY + 4.0f,
                         On ? Covering(0xDCDCDCu) : Covering(0x5E5E5Eu), Name, ChipRun);

        ChipX += ChipSpan + 4.0f;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE SETTINGS PROPERTIES
//------------------------------------------------------------------------------------------------------------------------

void TexturePaintPanel::RecordSettingsCard(const PlaneExtent& Extent, TexturePaintContext& Applied,
                                           const TextureLayerRow& Current)
{
    const float Pad = Scaled.PanePad;
    const float RowY = Scaled.RowHeight * 0.82f;
    float Sweep = Extent.MinimumY + Pad;

    Surface->Ground(Spanning(Extent.MinimumX + Pad, Sweep, Extent.Width() - Pad * 2.0f,
                             Scaled.ComponentY),
                    Tinted.Desk, Scaled.CardRadius, CornerAll);
    Surface->Edge(Spanning(Extent.MinimumX + Pad, Sweep, Extent.Width() - Pad * 2.0f,
                           Scaled.ComponentY),
                  Tinted.Hairline, 1.0f, Scaled.CardRadius, CornerAll);

    char Caption[48] = {};
    std::snprintf(Caption, sizeof(Caption), "%s settings", TextureLayerText(Current.Classified));

    Surface->TextRunCapitalised(Extent.MinimumX + Pad * 2.0f, Sweep + 7.0f,
                                Tinted.Primary, Caption, Scaled.RunSmall, 0.0f, true);

    Sweep += Scaled.ComponentY + Pad;

    // 📐 The settings rows — a small dummy set per kind: the reference's decal placement, pattern
    //    tiling, generator intensity, or the layer's own blend/opacity. The values are live
    //    scratch in the context; the labels are the kind's own.
    const char* const Labels[4] =
    {
        (Current.Classified == TextureLayerClassification::Decal)   ? "Scale"    :
        (Current.Classified == TextureLayerClassification::Pattern) ? "Tiling U" :
        (Current.Classified == TextureLayerClassification::Generator) ? "Intensity" : "Opacity",
        (Current.Classified == TextureLayerClassification::Decal)   ? "Fade"     :
        (Current.Classified == TextureLayerClassification::Pattern) ? "Tiling V" :
        (Current.Classified == TextureLayerClassification::Generator) ? "Balance" : "Blend",
        (Current.Classified == TextureLayerClassification::Decal)   ? "Height"   :
        (Current.Classified == TextureLayerClassification::Pattern) ? "Jitter"   :
        (Current.Classified == TextureLayerClassification::Generator) ? "Contrast" : "Seed",
        "Resolution"
    };

    for (std::uint32_t Ordinal = 0u; Ordinal < 4u; ++Ordinal)
    {
        const PlaneExtent Row = Spanning(Extent.MinimumX + Pad, Sweep,
                                         Extent.Width() - Pad * 2.0f, RowY);

        Surface->TextRun(Row.MinimumX + Scaled.PanePad, Row.MinimumY,
                         Tinted.Muted, Labels[Ordinal], Scaled.RunSecondary);

        const float BarX = Row.MinimumX + 90.0f;
        const float BarY = Row.MinimumY + (RowY - 4.0f) * 0.5f;
        const std::uint32_t Amount = Applied.SettingAmount[Applied.LayerTaken][Ordinal];

        Surface->Ground(Spanning(BarX, BarY, 120.0f, 4.0f), Covering(0x242424u), 2.0f, CornerAll);
        Surface->Ground(Spanning(BarX, BarY, 120.0f * static_cast<float>(Amount) / 100.0f, 4.0f),
                        Covering(0xFFFFFFu), 2.0f, CornerAll);

        if (Sampled.ContactPressed && BarX < Sampled.PositionX && Sampled.PositionX < BarX + 120.0f &&
            !Ledger->AnyDisclosed())
        {
            Ledger->Grab(SettingRows[Ordinal], ControlPart::Body);
        }

        if (Ledger->Holding(SettingRows[Ordinal]))
        {
            const float Fraction = Held((Sampled.PositionX - BarX) / 120.0f, 0.0f, 1.0f);
            Applied.SettingAmount[Applied.LayerTaken][Ordinal] =
                static_cast<std::uint32_t>(Fraction * 100.0f + 0.5f);
        }

        char Value[8] = {};
        std::snprintf(Value, sizeof(Value), "%u", Amount);

        Surface->TextRun(Row.MaximumX - 44.0f, Row.MinimumY,
                         Tinted.Primary, Value, Scaled.RunSecondary);

        Sweep += RowY + Scaled.LayerRowGap;
    }

    // 📐 The small note carrying the row's detail.
    const float NoteRun = Scaled.RunFine;
    const PlaneExtent Note = Spanning(Extent.MinimumX + Pad, Sweep,
                                      Extent.Width() - Pad * 2.0f, RowY * 1.4f);

    Surface->Ground(Note, Covering(0x0D0D0Du), Scaled.FieldRadius, CornerAll);

    char NoteText[96] = {};
    std::snprintf(NoteText, sizeof(NoteText), "%s \u00B7 %s \u00B7 %u%%",
                  TextureLayerText(Current.Classified),
                  Current.Detail[0] != '\0' ? Current.Detail : Current.Blend,
                  Applied.LayerOpacity[Applied.LayerTaken]);

    Surface->TextRunTruncated(Note.MinimumX + Scaled.PanePad * 1.5f,
                              Note.MinimumY + (Note.Height() - NoteRun) * 0.5f,
                              Note.MaximumX - Scaled.PanePad * 1.5f,
                              Tinted.Faint, NoteText, NoteRun);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE FOLDER (COMBINED) PROPERTIES
//------------------------------------------------------------------------------------------------------------------------

void TexturePaintPanel::RecordFolderCard(const PlaneExtent& Extent, TexturePaintContext& Applied,
                                         const TextureLayerRow* Rows, std::uint32_t RowCount)
{
    const float Pad = Scaled.PanePad;
    const float RowY = Scaled.RowHeight * 0.82f;
    float Sweep = Extent.MinimumY + Pad;

    const TextureLayerRow& Folder = Rows[Applied.LayerTaken];

    // 📐 The combined stack properties: one summary over the folder's whole subtree — the count,
    //    the mask count, the channel union, and the folder's own blend and opacity. This is the
    //    "Properties of combined layers in the entire stack" the user asked for.
    std::uint32_t Layers = 0u;
    std::uint32_t Masks  = 0u;
    bool ChannelUnion[TextureChannelCeiling] = {};

    for (std::uint32_t Ordinal = 0u; Ordinal < RowCount; ++Ordinal)
    {
        if (Ordinal == Applied.LayerTaken)
            continue;

        const bool Inside = Rows[Ordinal].Enclosing == Applied.LayerTaken;

        if (!Inside)
        {
            // 📝 Also count grandchildren: a row inside a child of the folder is inside the folder.
            bool Grandchild = false;
            std::uint32_t Walking = Rows[Ordinal].Enclosing;

            while (Walking < RowCount)
            {
                if (Walking == Applied.LayerTaken)
                {
                    Grandchild = true;
                    break;
                }

                if (Rows[Walking].Depth <= Folder.Depth)
                    break;

                Walking = Rows[Walking].Enclosing;
            }

            if (!Grandchild)
                continue;
        }

        ++Layers;

        if (Applied.MaskAttached[Ordinal])
            ++Masks;

        for (std::uint32_t Channel = 0u; Channel < Rows[Ordinal].ChannelCount &&
             Channel < TextureChannelCeiling; ++Channel)
        {
            ChannelUnion[Channel] = true;
        }
    }

    const auto Row = [&](const char* Label, const char* Value)
    {
        const PlaneExtent ExtentRow = Spanning(Extent.MinimumX + Pad, Sweep,
                                               Extent.Width() - Pad * 2.0f, RowY);

        Surface->TextRun(ExtentRow.MinimumX + Scaled.PanePad, ExtentRow.MinimumY,
                         Tinted.Muted, Label, Scaled.RunSecondary);

        Surface->TextRun(ExtentRow.MaximumX - Scaled.PanePad
                         - Surface->MeasureRun(Value, Scaled.RunSecondary, 0.0f),
                         ExtentRow.MinimumY, Tinted.Primary, Value, Scaled.RunSecondary);

        Sweep += RowY + Scaled.LayerRowGap;
    };

    char Tallied[24] = {};
    std::snprintf(Tallied, sizeof(Tallied), "%u layers", Layers);
    Row("Stack", Tallied);

    std::snprintf(Tallied, sizeof(Tallied), "%u masks", Masks);
    Row("Masks", Tallied);

    std::uint32_t Union = 0u;
    for (std::uint32_t Channel = 0u; Channel < TextureChannelCeiling; ++Channel)
    {
        if (ChannelUnion[Channel])
            ++Union;
    }
    std::snprintf(Tallied, sizeof(Tallied), "%u / %u channels", Union, TextureChannelCeiling);
    Row("Channel union", Tallied);

    Row("Blend", TextureBlendNames[Applied.LayerBlendTaken[Applied.LayerTaken] % TextureBlendCount]);

    std::snprintf(Tallied, sizeof(Tallied), "%u%%", Applied.LayerOpacity[Applied.LayerTaken]);
    Row("Opacity", Tallied);

    // 📐 The union chips.
    const PlaneExtent Chips = Spanning(Extent.MinimumX + Pad, Sweep,
                                       Extent.Width() - Pad * 2.0f, RowY * 1.6f);

    Surface->TextRun(Chips.MinimumX + Scaled.PanePad, Chips.MinimumY,
                     Tinted.Muted, "Channels", Scaled.RunSecondary);

    float ChipX = Chips.MinimumX + 90.0f;

    for (std::uint32_t Channel = 0u; Channel < TextureChannelCeiling && ChipX < Chips.MaximumX - 40.0f;
         ++Channel)
    {
        if (!ChannelUnion[Channel])
            continue;

        const char* Name = TextureChannelText(Channel);
        const float ChipRun = Scaled.RunFiner;
        const float ChipSpan = Surface->MeasureRun(Name, ChipRun, 0.0f) + 14.0f;

        Surface->Ground(Spanning(ChipX, Chips.MinimumY + 2.0f, ChipSpan, 17.0f),
                        Covering(0x2A2A2Au), 9.0f, CornerAll);

        Surface->TextRun(ChipX + 7.0f, Chips.MinimumY + 4.0f,
                         Covering(0xDCDCDCu), Name, ChipRun);

        ChipX += ChipSpan + 4.0f;
    }
}

} // namespace Slate
