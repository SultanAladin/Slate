//============================================================================================================================================
//                                                    LAYERSTACKSPECIFICATION.CPP
//============================================================================================================================================
// 🧩 The seated layer arrangement and every closed run behind it, transcribed from `References/LayerstackV1.html`.

#include "SlateUI/Interface/LayerStackSpecification/Api/LayerStackSpecification.h"

#include <cstdio>
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

// 📝 Claims one placement record and points the entry at it. An arrangement past the pool's ceiling seats
//    no record and leaves `Placement` absent, which records every other section and omits this one.
static PlacementRun* OpenPlacement(LayerArrangement& Arrangement, LayerEntry& Entry)
{
    if (Arrangement.PlacementCount >= LayerStackCeiling::PlacementRecords)
        return nullptr;

    Entry.Placement = Arrangement.PlacementCount;
    ++Arrangement.PlacementCount;

    return &Arrangement.Placements[Entry.Placement];
}

// 📐 `DECAL` — one selection, eleven ranges and five switches, in the reference's own order.
static void SeatDecalPlacement(LayerArrangement& Arrangement, LayerEntry& Entry)
{
    PlacementRun* const Run = OpenPlacement(Arrangement, Entry);

    if (Run == nullptr)
        return;

    Run->Parameters[ 0] = { "Projection",     0.0,    0.0,   0.0, "",  false, "Planar" };
    Run->Parameters[ 1] = { "Position X",     0.0, -100.0, 100.0, "",  false, nullptr };
    Run->Parameters[ 2] = { "Position Y",     0.0, -100.0, 100.0, "",  false, nullptr };
    Run->Parameters[ 3] = { "Position Z",     0.0, -100.0, 100.0, "",  false, nullptr };
    Run->Parameters[ 4] = { "Rotate X",       0.0,    0.0, 360.0, "\xC2\xB0", false, nullptr };
    Run->Parameters[ 5] = { "Rotate Y",       0.0,    0.0, 360.0, "\xC2\xB0", false, nullptr };
    Run->Parameters[ 6] = { "Rotate Z",       0.0,    0.0, 360.0, "\xC2\xB0", false, nullptr };
    Run->Parameters[ 7] = { "Scale",        100.0,    5.0, 600.0, "%", false, nullptr };
    Run->Parameters[ 8] = { "Aspect",       100.0,   25.0, 400.0, "%", false, nullptr };
    Run->Parameters[ 9] = { "Edge Fade",     12.0,    0.0, 100.0, "%", false, nullptr };
    Run->Parameters[10] = { "Parallax Depth", 0.0,    0.0, 100.0, "%", false, nullptr };
    Run->Parameters[11] = { "Height Amount", 35.0,    0.0, 100.0, "%", false, nullptr };

    Run->Parameters[12] = { "Snap to Surface", 1.0, 0.0, 1.0, "", true, nullptr };
    Run->Parameters[13] = { "Wrap Edges",      0.0, 0.0, 1.0, "", true, nullptr };
    Run->Parameters[14] = { "Height Blending", 1.0, 0.0, 1.0, "", true, nullptr };
    Run->Parameters[15] = { "Depth Test",      1.0, 0.0, 1.0, "", true, nullptr };
    Run->Parameters[16] = { "Show Gizmo",      1.0, 0.0, 1.0, "", true, nullptr };

    Run->ParameterCount = 17u;
}

// 📐 `PATTERN` — one selection, ten ranges and four switches, in the reference's own order.
static void SeatPatternPlacement(LayerArrangement& Arrangement, LayerEntry& Entry)
{
    PlacementRun* const Run = OpenPlacement(Arrangement, Entry);

    if (Run == nullptr)
        return;

    Run->Parameters[ 0] = { "Pattern",        0.0,   0.0,   0.0, "",  false, "Hex Grid" };
    Run->Parameters[ 1] = { "Scale",        100.0,   1.0, 800.0, "%", false, nullptr };
    Run->Parameters[ 2] = { "Rotation",       0.0,   0.0, 360.0, "\xC2\xB0", false, nullptr };
    Run->Parameters[ 3] = { "Tiling U",       4.0,   1.0,  64.0, "",  false, nullptr };
    Run->Parameters[ 4] = { "Tiling V",       4.0,   1.0,  64.0, "",  false, nullptr };
    Run->Parameters[ 5] = { "Jitter",         0.0,   0.0, 100.0, "%", false, nullptr };
    Run->Parameters[ 6] = { "Warp",           0.0,   0.0, 100.0, "%", false, nullptr };
    Run->Parameters[ 7] = { "Bevel",         20.0,   0.0, 100.0, "%", false, nullptr };
    Run->Parameters[ 8] = { "Gap",           10.0,   0.0, 100.0, "%", false, nullptr };
    Run->Parameters[ 9] = { "Height Amount", 45.0,   0.0, 100.0, "%", false, nullptr };
    Run->Parameters[10] = { "Seed",           3.0,   0.0, 999.0, "",  false, nullptr };

    Run->Parameters[11] = { "Invert",          0.0, 0.0, 1.0, "", true, nullptr };
    Run->Parameters[12] = { "Random Rotation", 0.0, 0.0, 1.0, "", true, nullptr };
    Run->Parameters[13] = { "Output to Height", 1.0, 0.0, 1.0, "", true, nullptr };
    Run->Parameters[14] = { "Tri-Planar",      0.0, 0.0, 1.0, "", true, nullptr };

    Run->ParameterCount = 15u;
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

        SeatDecalPlacement(Arrangement, Stencil);
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

        SeatPatternPlacement(Arrangement, Panelling);
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

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE MENU RUNS
//------------------------------------------------------------------------------------------------------------------------

const std::uint32_t* SeatedColourTags()
{
    // 📝 `COLORS`, verbatim and in the reference's own order.
    static const std::uint32_t Seated[LayerStackCeiling::ColourTags] =
    {
        0xE5484Du, 0xF76B15u, 0xFFC53Du, 0x46A758u, 0x12A594u,
        0x8AB4D8u, 0x9B8CF0u, 0xE93D82u, 0x8B8D98u, 0xB0E64Cu
    };

    return Seated;
}

const char* const* PlacementOptions(LayerContent Content, std::uint32_t& Count)
{
    // 📝 `DECAL`'s `proj` run, verbatim.
    static const char* const SeatedProjections[] =
    {
        "Planar", "Tri-Planar", "UV", "Spherical", "Screen Space"
    };

    // 📝 `PATTERN`'s `pat` run, verbatim.
    static const char* const SeatedPatterns[] =
    {
        "Hex Grid", "Square Grid", "Herringbone", "Dots", "Stripes", "Diamond Plate", "Rivets",
        "Camo", "Woven", "Brick", "Voronoi"
    };

    if (Content == LayerContent::Pattern)
    {
        Count = static_cast<std::uint32_t>(sizeof SeatedPatterns / sizeof SeatedPatterns[0]);
        return SeatedPatterns;
    }

    Count = static_cast<std::uint32_t>(sizeof SeatedProjections / sizeof SeatedProjections[0]);
    return SeatedProjections;
}

const char* const* EffectNaming(std::uint32_t& Count)
{
    // 📝 `EFFECTS`, verbatim.
    static const char* const Seated[] =
    {
        "Blur", "Blur Slope", "Sharpen", "Levels", "HSL Shift", "Warp", "Noise", "Curvature",
        "Anchor Point", "Clamp", "Contrast / Luminosity", "Height to Normal", "Normal to Height",
        "Matte Fill"
    };

    Count = static_cast<std::uint32_t>(sizeof Seated / sizeof Seated[0]);
    return Seated;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                 WALKING THE PRESENTED RUN
//------------------------------------------------------------------------------------------------------------------------

// 📝 One case-insensitive containment test over ASCII. The reference lowercases both sides and calls
//    `includes`; the same reading is produced here without a second copy of either run.
static bool RunCarries(const char* Within, const char* Sought)
{
    if (Sought == nullptr || Sought[0] == '\0')
        return true;

    if (Within == nullptr)
        return false;

    const auto Lowered = [](char Written) -> char
    {
        return (Written >= 'A' && Written <= 'Z') ? static_cast<char>(Written - 'A' + 'a') : Written;
    };

    for (std::uint32_t Seat = 0u; Within[Seat] != '\0'; ++Seat)
    {
        std::uint32_t Walk = 0u;

        while (Sought[Walk] != '\0' && Lowered(Within[Seat + Walk]) == Lowered(Sought[Walk]))
            ++Walk;

        if (Sought[Walk] == '\0')
            return true;
    }

    return false;
}

bool EntryRetained(const LayerArrangement& Arrangement, std::uint32_t Ordinal, const char* Retention)
{
    if (Ordinal >= Arrangement.EntryCount)
        return false;

    if (Retention == nullptr || Retention[0] == '\0')
        return true;

    if (RunCarries(Arrangement.Entries[Ordinal].Naming, Retention))
        return true;

    // 📐 A folder is retained by whatever it encloses, exactly as `match` recurses into `kids`.
    const std::uint32_t Enclosed = EnclosedCount(Arrangement, Ordinal);

    for (std::uint32_t Walk = Ordinal + 1u; Walk <= Ordinal + Enclosed; ++Walk)
        if (RunCarries(Arrangement.Entries[Walk].Naming, Retention))
            return true;

    return false;
}

std::uint32_t PresentedHalves(const LayerArrangement& Arrangement, const char* Retention,
                              PresentedHalf* Written, std::uint32_t Ceiling)
{
    if (Written == nullptr || Ceiling == 0u)
        return 0u;

    // 📐 A retention run opens every folder it reaches into — `const kOpen=(n.open||q)`. Without one the
    //    ordinary disclosure decides.
    const bool    Retaining = (Retention != nullptr) && (Retention[0] != '\0');
    std::uint32_t Occupied  = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < Arrangement.EntryCount; ++Ordinal)
    {
        const bool Presented = Retaining ? EntryRetained(Arrangement, Ordinal, Retention)
                                         : EntryPresented(Arrangement, Ordinal);

        if (!Presented)
            continue;

        if (Occupied >= Ceiling)
            break;

        Written[Occupied].Ordinal = Ordinal;
        Written[Occupied].Half    = LayerTaken::Layer;
        ++Occupied;

        if (Arrangement.Entries[Ordinal].Mask.Declared && Occupied < Ceiling)
        {
            Written[Occupied].Ordinal = Ordinal;
            Written[Occupied].Half    = LayerTaken::Mask;
            ++Occupied;
        }
    }

    return Occupied;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                 AMENDING THE ARRANGEMENT
//------------------------------------------------------------------------------------------------------------------------

// 🔴 Enclosing is DERIVED and never carried through a move. The run is held outermost-first with everything
//    a folder encloses laid immediately after it, so one entry's enclosing folder is the nearest preceding
//    entry exactly one step shallower. Deriving it after every amendment is what removes the whole class of
//    defect where a splice fixes the ordinals it can see and silently breaks the ones it cannot.
static void ResolveEnclosing(LayerArrangement& Arrangement)
{
    for (std::uint32_t Ordinal = 0u; Ordinal < Arrangement.EntryCount; ++Ordinal)
    {
        LayerEntry& Entry = Arrangement.Entries[Ordinal];

        Entry.Enclosing = LayerStackCeiling::AbsentOrdinal;

        if (Entry.Depth == 0u)
            continue;

        for (std::uint32_t Walk = Ordinal; Walk > 0u; --Walk)
        {
            if (Arrangement.Entries[Walk - 1u].Depth + 1u == Entry.Depth)
            {
                Entry.Enclosing = Walk - 1u;
                break;
            }
        }
    }
}

// 📐 How many records one entry occupies — itself and everything it encloses, contiguously.
static std::uint32_t SubrunSpan(const LayerArrangement& Arrangement, std::uint32_t Ordinal)
{
    return 1u + EnclosedCount(Arrangement, Ordinal);
}

static void ReverseRun(LayerArrangement& Arrangement, std::uint32_t Least, std::uint32_t Most)
{
    while (Least + 1u < Most)
    {
        const LayerEntry Held             = Arrangement.Entries[Least];
        Arrangement.Entries[Least]        = Arrangement.Entries[Most - 1u];
        Arrangement.Entries[Most - 1u]    = Held;

        ++Least;
        --Most;
    }
}

// 📐 Carries the records `[From, From + Span)` so that they begin at `To`, by three reversals. A temporary
//    copy of the whole subrun would be 160 KiB on the stack in the worst case; three reversals need one
//    record's worth and no allocation at all.
static void CarrySubrun(LayerArrangement& Arrangement, std::uint32_t From, std::uint32_t Span,
                        std::uint32_t To)
{
    if (Span == 0u || To == From)
        return;

    if (To > From)
    {
        ReverseRun(Arrangement, From, From + Span);
        ReverseRun(Arrangement, From + Span, To + Span);
        ReverseRun(Arrangement, From, To + Span);
    }
    else
    {
        ReverseRun(Arrangement, To, From);
        ReverseRun(Arrangement, From, From + Span);
        ReverseRun(Arrangement, To, From + Span);
    }
}

// 📐 The deepest nesting one subrun reaches, so a move can refuse before it exceeds the depth ceiling.
static std::uint32_t DeepestWithin(const LayerArrangement& Arrangement, std::uint32_t Ordinal,
                                   std::uint32_t Span)
{
    std::uint32_t Deepest = 0u;

    for (std::uint32_t Walk = Ordinal; Walk < Ordinal + Span; ++Walk)
        if (Arrangement.Entries[Walk].Depth > Deepest)
            Deepest = Arrangement.Entries[Walk].Depth;

    return Deepest;
}

bool EntryWithin(const LayerArrangement& Arrangement, std::uint32_t Enclosing, std::uint32_t Asked)
{
    if (Enclosing >= Arrangement.EntryCount || Asked >= Arrangement.EntryCount)
        return false;

    return Asked > Enclosing && Asked <= Enclosing + EnclosedCount(Arrangement, Enclosing);
}

bool DeclareEntry(LayerArrangement& Arrangement, LayerContent Content, const char* Naming)
{
    if (Arrangement.EntryCount >= LayerStackCeiling::Entries)
        return false;

    // 📐 `s.list.splice(s.i,0,n)` — the fresh entry is seated immediately above whatever stands taken, at
    //    that entry's own nesting, so an addition inside an open folder stays inside it.
    const std::uint32_t Seat  = (Arrangement.Taken < Arrangement.EntryCount) ? Arrangement.Taken : 0u;
    const std::uint32_t Depth = (Arrangement.EntryCount > 0u) ? Arrangement.Entries[Seat].Depth : 0u;

    for (std::uint32_t Walk = Arrangement.EntryCount; Walk > Seat; --Walk)
        Arrangement.Entries[Walk] = Arrangement.Entries[Walk - 1u];

    LayerEntry& Declared = Arrangement.Entries[Seat];

    Declared         = LayerEntry{};
    Declared.Content = Content;
    Declared.Depth   = Depth;

    SeatNaming(Declared, Naming);
    SeatChannels(Declared);

    if (Content == LayerContent::Folder)
    {
        Declared.Blend  = "Passthrough";
        Declared.Opened = true;
    }

    ++Arrangement.EntryCount;

    Arrangement.Taken     = Seat;
    Arrangement.TakenHalf = LayerTaken::Layer;

    ResolveEnclosing(Arrangement);
    return true;
}

bool RetireTaken(LayerArrangement& Arrangement)
{
    if (Arrangement.Taken >= Arrangement.EntryCount)
        return false;

    // 📐 The mask half retires the mask alone, exactly as `aDel` branches on `selMask`.
    if (Arrangement.TakenHalf == LayerTaken::Mask)
    {
        Arrangement.Entries[Arrangement.Taken].Mask = MaskOrdinate{};
        Arrangement.TakenHalf                       = LayerTaken::Layer;
        return true;
    }

    const std::uint32_t Seat = Arrangement.Taken;
    const std::uint32_t Span = SubrunSpan(Arrangement, Seat);

    for (std::uint32_t Walk = Seat; Walk + Span < Arrangement.EntryCount; ++Walk)
        Arrangement.Entries[Walk] = Arrangement.Entries[Walk + Span];

    Arrangement.EntryCount -= Span;

    if (Arrangement.Soloed >= Arrangement.EntryCount)
        Arrangement.Soloed = LayerStackCeiling::AbsentOrdinal;

    Arrangement.Taken     = (Arrangement.EntryCount == 0u) ? 0u
                          : ((Seat < Arrangement.EntryCount) ? Seat : Arrangement.EntryCount - 1u);
    Arrangement.TakenHalf = LayerTaken::Layer;

    ResolveEnclosing(Arrangement);
    return true;
}

bool DuplicateTaken(LayerArrangement& Arrangement)
{
    if (Arrangement.Taken >= Arrangement.EntryCount)
        return false;

    const std::uint32_t Seat = Arrangement.Taken;
    const std::uint32_t Span = SubrunSpan(Arrangement, Seat);

    if (Arrangement.EntryCount + Span > LayerStackCeiling::Entries)
        return false;

    for (std::uint32_t Walk = Arrangement.EntryCount; Walk > Seat; --Walk)
        Arrangement.Entries[Walk + Span - 1u] = Arrangement.Entries[Walk - 1u];

    for (std::uint32_t Walk = 0u; Walk < Span; ++Walk)
        Arrangement.Entries[Seat + Walk] = Arrangement.Entries[Seat + Span + Walk];

    // 📐 `c.name=s.node.name+' copy'` — only the copied head is renamed, never what it encloses.
    char Copied[LayerStackCeiling::NamingCeiling] = {};
    std::snprintf(Copied, sizeof Copied, "%.*s copy",
                  static_cast<int>(LayerStackCeiling::NamingCeiling - 7u), Arrangement.Entries[Seat].Naming);
    SeatNaming(Arrangement.Entries[Seat], Copied);

    Arrangement.EntryCount += Span;
    Arrangement.Taken       = Seat;
    Arrangement.TakenHalf   = LayerTaken::Layer;

    ResolveEnclosing(Arrangement);
    return true;
}

bool EncloseTaken(LayerArrangement& Arrangement)
{
    if (Arrangement.Taken >= Arrangement.EntryCount)
        return false;

    if (Arrangement.EntryCount >= LayerStackCeiling::Entries)
        return false;

    const std::uint32_t Seat = Arrangement.Taken;
    const std::uint32_t Span = SubrunSpan(Arrangement, Seat);

    if (DeepestWithin(Arrangement, Seat, Span) + 1u >= LayerStackCeiling::Depth)
        return false;

    const std::uint32_t Depth = Arrangement.Entries[Seat].Depth;

    for (std::uint32_t Walk = Arrangement.EntryCount; Walk > Seat; --Walk)
        Arrangement.Entries[Walk] = Arrangement.Entries[Walk - 1u];

    ++Arrangement.EntryCount;

    for (std::uint32_t Walk = Seat + 1u; Walk <= Seat + Span; ++Walk)
        ++Arrangement.Entries[Walk].Depth;

    LayerEntry& Folder = Arrangement.Entries[Seat];

    Folder           = LayerEntry{};
    Folder.Content   = LayerContent::Folder;
    Folder.Blend     = "Passthrough";
    Folder.Opened    = true;
    Folder.Depth     = Depth;
    Folder.ColourTag = Arrangement.Entries[Seat + 1u].ColourTag;

    SeatNaming(Folder, "Group");
    SeatChannels(Folder);

    Arrangement.Taken     = Seat;
    Arrangement.TakenHalf = LayerTaken::Layer;

    ResolveEnclosing(Arrangement);
    return true;
}

bool CarryTaken(LayerArrangement& Arrangement, bool Downward)
{
    if (Arrangement.Taken >= Arrangement.EntryCount)
        return false;

    const std::uint32_t Seat  = Arrangement.Taken;
    const std::uint32_t Span  = SubrunSpan(Arrangement, Seat);
    const std::uint32_t Depth = Arrangement.Entries[Seat].Depth;

    if (Downward)
    {
        const std::uint32_t Neighbour = Seat + Span;

        // 📐 Nothing follows at this nesting, so the entry steps OUT of whatever encloses it.
        if (Neighbour >= Arrangement.EntryCount || Arrangement.Entries[Neighbour].Depth < Depth)
        {
            if (Depth == 0u)
                return false;

            for (std::uint32_t Walk = Seat; Walk < Seat + Span; ++Walk)
                --Arrangement.Entries[Walk].Depth;

            ResolveEnclosing(Arrangement);
            return true;
        }

        const std::uint32_t NeighbourSpan = SubrunSpan(Arrangement, Neighbour);

        // 📐 An OPEN folder is stepped INTO rather than over, which is what makes a repeated press walk
        //    down into a folder exactly as the reference's `nb.kids.unshift(n)` does.
        if (Arrangement.Entries[Neighbour].Content == LayerContent::Folder &&
            Arrangement.Entries[Neighbour].Opened)
        {
            if (DeepestWithin(Arrangement, Seat, Span) + 1u >= LayerStackCeiling::Depth)
                return false;

            CarrySubrun(Arrangement, Seat, Span, Seat + 1u);

            for (std::uint32_t Walk = Seat + 1u; Walk < Seat + 1u + Span; ++Walk)
                ++Arrangement.Entries[Walk].Depth;

            Arrangement.Taken = Seat + 1u;
            ResolveEnclosing(Arrangement);
            return true;
        }

        CarrySubrun(Arrangement, Seat, Span, Seat + NeighbourSpan);
        Arrangement.Taken = Seat + NeighbourSpan;
        ResolveEnclosing(Arrangement);
        return true;
    }

    // 📐 Upward. The preceding neighbour is found by walking back to the nearest entry at this nesting.
    if (Seat == 0u)
        return false;

    std::uint32_t Preceding = Seat;

    while (Preceding > 0u && Arrangement.Entries[Preceding - 1u].Depth > Depth)
        --Preceding;

    if (Preceding == 0u || Arrangement.Entries[Preceding - 1u].Depth < Depth)
    {
        if (Depth == 0u)
            return false;

        // 📐 Stepping out of the enclosing folder, which now sits immediately before this subrun.
        const std::uint32_t Enclosing = Preceding - 1u;

        for (std::uint32_t Walk = Seat; Walk < Seat + Span; ++Walk)
            --Arrangement.Entries[Walk].Depth;

        CarrySubrun(Arrangement, Seat, Span, Enclosing);

        Arrangement.Taken = Enclosing;
        ResolveEnclosing(Arrangement);
        return true;
    }

    const std::uint32_t Neighbour = Preceding - 1u;

    if (Arrangement.Entries[Neighbour].Content == LayerContent::Folder &&
        Arrangement.Entries[Neighbour].Opened)
    {
        if (DeepestWithin(Arrangement, Seat, Span) + 1u >= LayerStackCeiling::Depth)
            return false;

        // 📐 `nb.kids.push(n)` — stepping up into an open folder lands at its END, not its beginning.
        for (std::uint32_t Walk = Seat; Walk < Seat + Span; ++Walk)
            ++Arrangement.Entries[Walk].Depth;

        ResolveEnclosing(Arrangement);
        return true;
    }

    CarrySubrun(Arrangement, Seat, Span, Neighbour);
    Arrangement.Taken = Neighbour;
    ResolveEnclosing(Arrangement);
    return true;
}

bool ToggleMask(LayerArrangement& Arrangement)
{
    if (Arrangement.Taken >= Arrangement.EntryCount)
        return false;

    LayerEntry& Entry = Arrangement.Entries[Arrangement.Taken];

    if (Entry.Mask.Declared)
    {
        Entry.Mask            = MaskOrdinate{};
        Arrangement.TakenHalf = LayerTaken::Layer;
        return true;
    }

    SeatPaintMask(Entry, 100u, true);
    Entry.Mask.Unfolded   = true;
    Arrangement.TakenHalf = LayerTaken::Mask;
    return true;
}

void ToggleEveryFolder(LayerArrangement& Arrangement)
{
    // 📐 `const to=!any()` — one open folder anywhere closes them all; otherwise they all open. The pass
    //    also folds every card, which is what makes the button read as "collapse everything".
    bool AnyOpened = false;

    for (std::uint32_t Ordinal = 0u; Ordinal < Arrangement.EntryCount; ++Ordinal)
        if (Arrangement.Entries[Ordinal].Content == LayerContent::Folder &&
            Arrangement.Entries[Ordinal].Opened)
        {
            AnyOpened = true;
            break;
        }

    for (std::uint32_t Ordinal = 0u; Ordinal < Arrangement.EntryCount; ++Ordinal)
    {
        LayerEntry& Entry = Arrangement.Entries[Ordinal];

        if (Entry.Content == LayerContent::Folder)
            Entry.Opened = !AnyOpened;

        Entry.Unfolded      = false;
        Entry.Mask.Unfolded = false;
    }
}

bool CarryEntry(LayerArrangement& Arrangement, std::uint32_t Carried, std::uint32_t Destined,
                bool Enclosed, bool Trailing)
{
    if (Carried >= Arrangement.EntryCount || Destined >= Arrangement.EntryCount || Carried == Destined)
        return false;

    // 🔴 A destination inside what is being carried would splice the run into itself, which orphans every
    //    record between them. The reference refuses the same case with `if(within(dn,r.dataset.id))return`.
    if (EntryWithin(Arrangement, Carried, Destined))
        return false;

    const std::uint32_t Span     = SubrunSpan(Arrangement, Carried);
    const std::uint32_t Deepest  = DeepestWithin(Arrangement, Carried, Span);
    const std::uint32_t Standing = Arrangement.Entries[Carried].Depth;

    std::uint32_t Seat  = 0u;
    std::uint32_t Depth = 0u;

    if (Enclosed)
    {
        if (Arrangement.Entries[Destined].Content != LayerContent::Folder)
            return false;

        Depth = Arrangement.Entries[Destined].Depth + 1u;
        Seat  = Destined + 1u;

        Arrangement.Entries[Destined].Opened = true;
    }
    else
    {
        Depth = Arrangement.Entries[Destined].Depth;
        Seat  = Trailing ? (Destined + SubrunSpan(Arrangement, Destined)) : Destined;
    }

    if (Deepest - Standing + Depth >= LayerStackCeiling::Depth)
        return false;

    // 📐 A seat beyond the carried subrun is measured against a run that still holds it, so it steps back
    //    by the span the removal will take out from under it.
    const std::uint32_t Resolved = (Seat > Carried) ? (Seat - Span) : Seat;

    CarrySubrun(Arrangement, Carried, Span, Resolved);

    for (std::uint32_t Walk = Resolved; Walk < Resolved + Span; ++Walk)
        Arrangement.Entries[Walk].Depth = Arrangement.Entries[Walk].Depth - Standing + Depth;

    Arrangement.Taken     = Resolved;
    Arrangement.TakenHalf = LayerTaken::Layer;

    ResolveEnclosing(Arrangement);
    return true;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE REVISION RING
//------------------------------------------------------------------------------------------------------------------------

void RevisionSequence::Record(const LayerArrangement& Standing, const char* Naming)
{
    // 📐 `snap()` — the recorded run drops its oldest reading once it is full, and every reinstatable
    //    reading is abandoned, because a fresh amendment makes the branch they belonged to unreachable.
    if (Recorded == RevisionCeiling)
    {
        for (std::uint32_t Walk = 0u; Walk + 1u < RevisionCeiling; ++Walk)
        {
            Reverting[Walk] = Reverting[Walk + 1u];
            Namings[Walk]   = Namings[Walk + 1u];
        }

        --Recorded;
    }

    Reverting[Recorded] = Standing;
    Namings[Recorded]   = (Naming != nullptr) ? Naming : "";
    ++Recorded;

    Reinstatable = 0u;
}

bool RevisionSequence::Revert(LayerArrangement& Standing)
{
    if (Recorded == 0u)
        return false;

    if (Reinstatable < RevisionCeiling)
    {
        Reinstating[Reinstatable]      = Standing;
        ReinstateNamings[Reinstatable] = Namings[Recorded - 1u];
        ++Reinstatable;
    }

    --Recorded;
    Standing = Reverting[Recorded];
    return true;
}

bool RevisionSequence::Reinstate(LayerArrangement& Standing)
{
    if (Reinstatable == 0u)
        return false;

    if (Recorded < RevisionCeiling)
    {
        Reverting[Recorded] = Standing;
        Namings[Recorded]   = ReinstateNamings[Reinstatable - 1u];
        ++Recorded;
    }

    --Reinstatable;
    Standing = Reinstating[Reinstatable];
    return true;
}

const char* RevisionSequence::RevisionNaming(std::uint32_t Ordinal) const
{
    if (Ordinal >= Recorded)
        return "";

    // 📐 Newest first, which is the order the pane presents.
    const char* const Written = Namings[Recorded - 1u - Ordinal];
    return (Written != nullptr) ? Written : "";
}

void RevisionSequence::Reset()
{
    Recorded     = 0u;
    Reinstatable = 0u;
}

}   // namespace Slate
