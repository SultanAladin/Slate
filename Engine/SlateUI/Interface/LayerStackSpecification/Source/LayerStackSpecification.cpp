//============================================================================================================================================
//                                                    LAYERSTACKSPECIFICATION.CPP
//============================================================================================================================================
// 🧩 The seated layer arrangement and every closed run behind it, transcribed from `References/LayerstackV1.html`.

#include "SlateUI/Interface/LayerStackSpecification/Api/LayerStackSpecification.h"

#include <cstring>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE CHANNEL RUN
//------------------------------------------------------------------------------------------------------------------------

// 📝 `CHANNELS` from the reference, in its own order. The Channel panel presents these eight; the
//    fourteen-slot arrangement the property panel states is a superset presented there, not here.
static const char* const SeatedChannels[LayerStackCeiling::Channels] =
{
    "Base Color", "Metallic", "Roughness", "Normal",
    "Height", "Ambient Occlusion", "Emissive", "Opacity"
};

// 📝 One tint per channel, matching the property panel's own `CHANNEL_SLOTS` hues.
static const std::uint32_t SeatedChannelTints[LayerStackCeiling::Channels] =
{
    0xB87333u, 0x8B5CF6u, 0x3B82F6u, 0x10B981u,
    0x8A8A8Au, 0x6B7280u, 0xF59E0Bu, 0x94A3B8u
};

const char* const* ChannelNaming()
{
    return SeatedChannels;
}

InkOrdinate ChannelTint(std::uint32_t Ordinal)
{
    const std::uint32_t Resolved = (Ordinal < LayerStackCeiling::Channels) ? Ordinal : 0u;
    return Covering(SeatedChannelTints[Resolved]);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE BLEND RUNS
//------------------------------------------------------------------------------------------------------------------------

// 📝 `BLENDS`, `NBLENDS` and `HBLENDS` verbatim. Normal and Height accept their own shorter runs, which is
//    why `blendsFor` exists in the reference and why the ordinal decides here.
static const char* const SeatedBlends[] =
{
    "Normal", "Passthrough", "Replace", "Disable", "Multiply", "Divide", "Inverse Divide", "Screen",
    "Overlay", "Soft Light", "Hard Light", "Vivid Light", "Linear Light", "Pin Light",
    "Linear Dodge (Add)", "Color Dodge", "Linear Burn", "Color Burn", "Subtract", "Inverse Subtract",
    "Difference", "Exclusion", "Signed Addition (AddSub)", "Darken (Min)", "Lighten (Max)", "Tint",
    "Saturation", "Color", "Value"
};

static const char* const SeatedNormalBlends[] =
{
    "Normal Map Combine", "Normal Map Detail", "Normal Map Inverse Detail", "Normal", "Replace", "Disable"
};

static const char* const SeatedHeightBlends[] =
{
    "Linear Dodge (Add)", "Signed Addition (AddSub)", "Normal", "Multiply", "Subtract",
    "Darken (Min)", "Lighten (Max)", "Replace", "Disable"
};

const char* const* BlendNaming(std::uint32_t ChannelOrdinal, std::uint32_t& Count)
{
    // 📐 Ordinal three is Normal and ordinal four is Height, per `SeatedChannels`.
    if (ChannelOrdinal == 3u)
    {
        Count = static_cast<std::uint32_t>(sizeof SeatedNormalBlends / sizeof SeatedNormalBlends[0]);
        return SeatedNormalBlends;
    }

    if (ChannelOrdinal == 4u)
    {
        Count = static_cast<std::uint32_t>(sizeof SeatedHeightBlends / sizeof SeatedHeightBlends[0]);
        return SeatedHeightBlends;
    }

    Count = static_cast<std::uint32_t>(sizeof SeatedBlends / sizeof SeatedBlends[0]);
    return SeatedBlends;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT ONE ENTRY IS
//------------------------------------------------------------------------------------------------------------------------

InkOrdinate ContentTint(LayerContent Content)
{
    switch (Content)
    {
        case LayerContent::Folder:     return Covering(0x9B8CF0u);
        case LayerContent::Paint:      return Covering(0xB0E64Cu);
        case LayerContent::Fill:       return Covering(0xF76B15u);
        case LayerContent::Adjustment: return Covering(0x8B8D98u);
        case LayerContent::Retention:  return Covering(0x12A594u);
        case LayerContent::Decal:      return Covering(0xE5484Du);
        case LayerContent::Pattern:    return Covering(0x8AB4D8u);
        default:                       return Covering(0x8B8D98u);
    }
}

const char* ContentNaming(LayerContent Content)
{
    switch (Content)
    {
        case LayerContent::Folder:     return "Folder";
        case LayerContent::Paint:      return "Paint";
        case LayerContent::Fill:       return "Fill";
        case LayerContent::Adjustment: return "Adjustment";
        case LayerContent::Retention:  return "Filter";
        case LayerContent::Decal:      return "Decal";
        case LayerContent::Pattern:    return "Pattern";
        default:                       return "Paint";
    }
}

const char* ContentBadge(LayerContent Content)
{
    switch (Content)
    {
        case LayerContent::Folder:     return "F";
        case LayerContent::Paint:      return "P";
        case LayerContent::Fill:       return "L";
        case LayerContent::Adjustment: return "A";
        case LayerContent::Retention:  return "R";
        case LayerContent::Decal:      return "D";
        case LayerContent::Pattern:    return "T";
        default:                       return "P";
    }
}

const char* SourceNaming(MaskSource Source)
{
    switch (Source)
    {
        case MaskSource::Paint:           return "Paint";
        case MaskSource::Bitmap:          return "Bitmap";
        case MaskSource::BakedMap:        return "Baked Map";
        case MaskSource::PolygonFill:     return "Polygon Fill";
        case MaskSource::ColourSelection: return "Color Selection";
        case MaskSource::AnchorPoint:     return "Anchor Point";
        case MaskSource::Generator:       return "Generator";
        default:                          return "Paint";
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   WALKING THE NESTING
//------------------------------------------------------------------------------------------------------------------------

bool EntryPresented(const LayerArrangement& Arrangement, std::uint32_t Ordinal)
{
    if (Ordinal >= Arrangement.EntryCount)
        return false;

    // 📐 Walk outward through the enclosing folders; one closed folder hides everything within it. The walk
    //    is bounded by the nesting ceiling so a malformed enclosing ordinal cannot spin.
    std::uint32_t Enclosing = Arrangement.Entries[Ordinal].Enclosing;
    std::uint32_t Steps     = 0u;

    while (Enclosing < Arrangement.EntryCount && Steps < LayerStackCeiling::Depth)
    {
        if (!Arrangement.Entries[Enclosing].Opened)
            return false;

        Enclosing = Arrangement.Entries[Enclosing].Enclosing;
        ++Steps;
    }

    return true;
}

std::uint32_t EnclosedCount(const LayerArrangement& Arrangement, std::uint32_t Ordinal)
{
    if (Ordinal >= Arrangement.EntryCount)
        return 0u;

    // 📐 The run is held outermost-first with everything a folder encloses laid immediately after it, so the
    //    enclosed extent ends at the first entry no deeper than the folder itself.
    const std::uint32_t Depth   = Arrangement.Entries[Ordinal].Depth;
    std::uint32_t       Counted = 0u;

    for (std::uint32_t Walk = Ordinal + 1u; Walk < Arrangement.EntryCount; ++Walk)
    {
        if (Arrangement.Entries[Walk].Depth <= Depth)
            break;

        ++Counted;
    }

    return Counted;
}

std::uint32_t ChannelsEnabled(const LayerEntry& Entry)
{
    std::uint32_t Counted = 0u;

    for (std::uint32_t Channel = 0u; Channel < LayerStackCeiling::Channels; ++Channel)
        if (Entry.Channels[Channel].Enabled)
            ++Counted;

    return Counted;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  SEATING THE REFERENCE
//------------------------------------------------------------------------------------------------------------------------

// 📝 `defCh()` — every channel arrives enabled but Emissive and Opacity, Normal arrives combining and
//    Height arrives adding, which is what makes the height-into-normal note on the card true.
static void SeatChannels(LayerEntry& Entry)
{
    for (std::uint32_t Channel = 0u; Channel < LayerStackCeiling::Channels; ++Channel)
    {
        Entry.Channels[Channel].Enabled = (Channel != 6u) && (Channel != 7u);
        Entry.Channels[Channel].Opacity = 100u;
        Entry.Channels[Channel].Blend   = "Normal";
    }

    Entry.Channels[3].Blend = "Normal Map Combine";
    Entry.Channels[4].Blend = "Linear Dodge (Add)";
}

static void SeatNaming(LayerEntry& Entry, const char* Naming)
{
    std::strncpy(Entry.Naming, Naming, LayerStackCeiling::NamingCeiling - 1u);
    Entry.Naming[LayerStackCeiling::NamingCeiling - 1u] = '\0';
}

// 📝 One paint-sourced mask, the reference's simplest `MASK()`.
static void SeatPaintMask(LayerEntry& Entry, std::uint32_t Density, bool Shown)
{
    Entry.Mask.Declared   = true;
    Entry.Mask.Shown      = Shown;
    Entry.Mask.Source     = MaskSource::Paint;
    Entry.Mask.Density    = Density;
    Entry.Mask.Blend      = "Multiply";
    Entry.Mask.Resolution = 2048u;

    Entry.Mask.Parameters[0] = { "Brush",    0.0,   0.0,   0.0, "",   false, "Basic Hard" };
    Entry.Mask.Parameters[1] = { "Flow",   100.0,   0.0, 100.0, "%",  false, nullptr };
    Entry.Mask.Parameters[2] = { "Size",    24.0,   1.0, 300.0, "px", false, nullptr };
    Entry.Mask.Parameters[3] = { "Hardness", 80.0,  0.0, 100.0, "%",  false, nullptr };
    Entry.Mask.Parameters[4] = { "Symmetry", 0.0,   0.0,   1.0, "",   true,  nullptr };
    Entry.Mask.ParameterCount = 5u;
}

// 📝 One generator-sourced mask. The mesh maps a generator reads are what the bake chips report.
static void SeatGeneratorMask(LayerEntry& Entry, const char* Generator, std::uint32_t Density)
{
    Entry.Mask.Declared   = true;
    Entry.Mask.Shown      = true;
    Entry.Mask.Source     = MaskSource::Generator;
    Entry.Mask.Generator  = Generator;
    Entry.Mask.Density    = Density;
    Entry.Mask.Blend      = "Multiply";
    Entry.Mask.Resolution = 2048u;

    Entry.Mask.Parameters[0] = { "Intensity",       60.0, 0.0, 100.0, "%", false, nullptr };
    Entry.Mask.Parameters[1] = { "Balance",         50.0, 0.0, 100.0, "%", false, nullptr };
    Entry.Mask.Parameters[2] = { "Contrast",        50.0, 0.0, 100.0, "%", false, nullptr };
    Entry.Mask.Parameters[3] = { "Blur",            10.0, 0.0, 100.0, "%", false, nullptr };
    Entry.Mask.Parameters[4] = { "Range Clamp",    100.0, 0.0, 100.0, "%", false, nullptr };
    Entry.Mask.Parameters[5] = { "Invert",           0.0, 0.0,   1.0, "",  true,  nullptr };
    Entry.Mask.Parameters[6] = { "Micro Details",    0.0, 0.0,   1.0, "",  true,  nullptr };
    Entry.Mask.ParameterCount = 7u;

    Entry.Mask.MeshMaps[0] = "Curvature";
    Entry.Mask.MeshMaps[1] = "Normal";
    Entry.Mask.MeshMaps[2] = "Height";
    Entry.Mask.MeshMapTransferred[0] = true;
    Entry.Mask.MeshMapTransferred[1] = true;
    Entry.Mask.MeshMapTransferred[2] = true;
    Entry.Mask.MeshMapCount = 3u;
}

Deliver<bool> SeatReferenceArrangement(LayerArrangement& Arrangement)
{
    // 📐 The reference's own `tree`, laid outermost-first with everything a folder encloses immediately
    //    after it. Thirteen entries stand, well inside the ceiling — the guard states the invariant anyway
    //    so a later edit to the declared run refuses rather than writing past the extent.
    static constexpr std::uint32_t Declared = 13u;

    if (Declared > LayerStackCeiling::Entries)
        return Deliver<bool>::Refuse({ RefusalReason::ExtentExhausted,
                                       "the declared arrangement exceeds the entry ceiling" });

    Arrangement = LayerArrangement{};

    const auto Open = [&](LayerContent Content, const char* Naming, std::uint32_t Depth,
                          std::uint32_t Enclosing, std::uint32_t ColourTag) -> LayerEntry&
    {
        LayerEntry& Entry = Arrangement.Entries[Arrangement.EntryCount];

        Entry           = LayerEntry{};
        Entry.Content   = Content;
        Entry.Depth     = Depth;
        Entry.Enclosing = Enclosing;
        Entry.ColourTag = ColourTag;

        SeatNaming(Entry, Naming);
        SeatChannels(Entry);

        ++Arrangement.EntryCount;
        return Entry;
    };

    // ① Surface Detail — a folder over four entries.
    {
        LayerEntry& Folder = Open(LayerContent::Folder, "Surface Detail", 0u, 0xFFFFFFFFu, 0x9B8CF0u);
        Folder.Blend  = "Passthrough";
        Folder.Opened = true;
    }

    {
        LayerEntry& Levels = Open(LayerContent::Adjustment, "Levels", 1u, 0u, 0x8B8D98u);
        Levels.Blend    = "Overlay";
        Levels.Opacity  = 64u;
        Levels.Modified = "2026-08-18 09:31";
    }

    {
        LayerEntry& Stencil = Open(LayerContent::Decal, "Warning Stencil", 1u, 0u, 0xE5484Du);
        Stencil.Blend = "Normal";
        SeatPaintMask(Stencil, 100u, true);
        Stencil.Mask.Source = MaskSource::Bitmap;
        Stencil.Mask.Parameters[0] = { "Bitmap", 0.0, 0.0, 0.0, "", false, "grunge_leaky_paint" };
        Stencil.Mask.Parameters[1] = { "Projection", 0.0, 0.0, 0.0, "", false, "UV" };
        Stencil.Mask.Parameters[2] = { "Scale", 100.0, 1.0, 800.0, "%", false, nullptr };
        Stencil.Mask.ParameterCount = 3u;
    }

    {
        LayerEntry& Scratches = Open(LayerContent::Paint, "Scratches", 1u, 0u, 0xB0E64Cu);
        Scratches.Blend      = "Screen";
        Scratches.Opacity    = 38u;
        Scratches.Effects[0] = "Blur";
        Scratches.EffectCount = 1u;
        SeatGeneratorMask(Scratches, "Metal Edge Wear", 88u);
        Scratches.Mask.Effects[0]  = "Levels";
        Scratches.Mask.Effects[1]  = "Blur";
        Scratches.Mask.EffectCount = 2u;
    }

    {
        LayerEntry& EdgeWear = Open(LayerContent::Fill, "Edge Wear", 1u, 0u, 0xF76B15u);
        EdgeWear.Blend   = "Multiply";
        EdgeWear.Opacity = 82u;
        SeatGeneratorMask(EdgeWear, "Curvature", 100u);
        EdgeWear.Mask.Effects[0]  = "Levels";
        EdgeWear.Mask.EffectCount = 1u;
    }

    // ② Three outermost entries between the two folders.
    {
        LayerEntry& Trim = Open(LayerContent::Fill, "Emissive Trim", 0u, 0xFFFFFFFFu, 0xFFC53Du);
        SeatPaintMask(Trim, 100u, true);
    }

    {
        LayerEntry& Panelling = Open(LayerContent::Pattern, "Hex Panelling", 0u, 0xFFFFFFFFu, 0x8AB4D8u);
        SeatGeneratorMask(Panelling, "Position", 100u);
    }

    // ③ Base Materials — a folder over four fills.
    {
        LayerEntry& Folder = Open(LayerContent::Folder, "Base Materials", 0u, 0xFFFFFFFFu, 0x12A594u);
        Folder.Blend  = "Passthrough";
        Folder.Opened = true;
    }

    const std::uint32_t BaseOrdinal = Arrangement.EntryCount - 1u;

    {
        LayerEntry& Steel = Open(LayerContent::Fill, "Brushed Steel", 1u, BaseOrdinal, 0x8AB4D8u);
        SeatGeneratorMask(Steel, "Mask Editor", 100u);
        Steel.Mask.Effects[0]  = "Warp";
        Steel.Mask.EffectCount = 1u;
    }

    {
        LayerEntry& Gold = Open(LayerContent::Fill, "Gold Inlay", 1u, BaseOrdinal, 0xE5484Du);
        Gold.Effects[0]  = "Levels";
        Gold.Effects[1]  = "HSL Shift";
        Gold.EffectCount = 2u;
        SeatPaintMask(Gold, 100u, false);
        Gold.Mask.Source = MaskSource::ColourSelection;
        Gold.Mask.Parameters[0] = { "ID Colour", 0.0, 0.0, 0.0, "", false, "ID_04 \xC2\xB7 red" };
        Gold.Mask.Parameters[1] = { "Tolerance", 15.0, 0.0, 100.0, "%", false, nullptr };
        Gold.Mask.Parameters[2] = { "Blur", 2.0, 0.0, 100.0, "%", false, nullptr };
        Gold.Mask.ParameterCount = 3u;
    }

    {
        LayerEntry& Oak = Open(LayerContent::Fill, "Oak Panel", 1u, BaseOrdinal, 0xF76B15u);
        Oak.Secured = true;
    }

    {
        LayerEntry& Canvas = Open(LayerContent::Fill, "Canvas Weave", 1u, BaseOrdinal, 0xE93D82u);
        Canvas.Opacity = 90u;
        Canvas.Shown   = false;
    }

    // ④ The outermost base.
    {
        LayerEntry& Concrete = Open(LayerContent::Fill, "Concrete Base", 0u, 0xFFFFFFFFu, 0x8B8D98u);
        Concrete.Resolution = 4096u;
    }

    // 📝 The reference seats `sel` on its second outermost entry — Emissive Trim — and unfolds it.
    Arrangement.Taken     = 5u;
    Arrangement.TakenHalf = LayerTaken::Layer;
    Arrangement.Entries[5].Unfolded = true;

    return Deliver<bool>::Deliver(true);
}

void SeatReferenceRevisions(const RevisionOrdinate*& Revisions, std::uint32_t& Count)
{
    // 📝 `lib/store.tsx` seats its revisions from the record it opens on. The same readings are stated here
    //    so the second pane presents a standing run rather than an empty one.
    static const RevisionOrdinate Seated[] =
    {
        // 📝 The rightwards arrow is written "->": the default typeface carries no U+2192, exactly as it
        //    carries no U+2026, and an absent glyph draws as a hollow box rather than as nothing.
        { "Opacity amended",    "2026-08-19 14:02", "82% -> 100%" },
        { "Mask density moved", "2026-08-19 11:47", "88% -> 92%"  },
        { "Blend restated",     "2026-08-18 17:20", "Screen"                },
        { "Effect declared",    "2026-08-18 09:31", "Levels"                },
        { "Entry opened",       "2026-08-17 08:05", "Paint"                 }
    };

    Revisions = Seated;
    Count     = static_cast<std::uint32_t>(sizeof Seated / sizeof Seated[0]);
}

}   // namespace Slate
