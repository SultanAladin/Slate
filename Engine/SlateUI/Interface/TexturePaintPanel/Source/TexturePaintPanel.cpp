//============================================================================================================================================
//                                                           TEXTUREPAINTPANEL.CPP
//============================================================================================================================================
// 🧩 The editor's texture-paint layer stack — the stack page and the
//    selection-driven properties page. See TexturePaintPanel.h for the flow:
//    a layer row + Tab → channel properties, a mask + Tab → the mask panel,
//    a decal/pattern/generator + Tab → its settings, a folder + Tab → the
//    combined stack properties. No history panel.

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

// 📐 The blend roster the channel rows cycle.
const char* const BlendOptions[6] =
{
    "Normal", "Multiply", "Screen", "Overlay", "Linear Dodge", "Replace"
};

// 📐 The mask source roster.
const char* const MaskSourceOptions[5] =
{
    "Paint", "Bitmap", "Baked Map", "Polygon Fill", "Generator"
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

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE CLASSIFICATIONS
//------------------------------------------------------------------------------------------------------------------------

SymbolSubject TextureLayerGlyph(TextureLayerClassification Classified)
{
    switch (Classified)
    {
        case TextureLayerClassification::Paint:      return SymbolSubject::PaintBristle;
        case TextureLayerClassification::Fill:       return SymbolSubject::MaterialSphere;
        case TextureLayerClassification::Decal:      return SymbolSubject::StencilProjection;
        case TextureLayerClassification::Pattern:    return SymbolSubject::LatticeArrangement;
        case TextureLayerClassification::Generator:  return SymbolSubject::ChannelSelect;
        case TextureLayerClassification::Adjustment: return SymbolSubject::PulseTrace;
        case TextureLayerClassification::Filter:     return SymbolSubject::GearCog;
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

const char* TextureChannelText(std::uint32_t Ordinal)
{
    static const char* const Channels[TextureChannelCeiling] =
    {
        "Base Color", "Metallic", "Roughness", "Normal",
        "Height", "Ambient Occlusion", "Emissive", "Opacity"
    };

    return Ordinal < TextureChannelCeiling ? Channels[Ordinal] : "";
}

std::uint32_t TextureChannelGroup(std::uint32_t Ordinal)
{
    // 📐 Base Color alone; Metallic..AO are maps; Emissive and Opacity are output.
    if (Ordinal == 0u) return 0u;
    if (Ordinal >= 6u) return 2u;
    return 1u;
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
        &StackStrip, &PropertyStrip, &SearchField, &AddLayer,
        &MaskRows[0], &MaskRows[1], &MaskRows[2], &MaskRows[3],
        &SettingRows[0], &SettingRows[1], &SettingRows[2], &SettingRows[3]
    };

    for (ControlIdentity* Identity : Every)
    {
        const Outcome<ControlIdentity> Registered = Interaction.Register();
        if (!Registered.Resolved)
            return Outcome<bool>::Refuse(Registered.Error);

        *Identity = Registered.Resolve();
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < TexturePaintContext::TextureLayerCeiling; ++Ordinal)
    {
        ControlIdentity* const Rows[] =
        {
            &LayerContacts[Ordinal], &LayerChevrons[Ordinal],
            &LayerEyes[Ordinal], &LayerUnfolds[Ordinal], &MaskContacts[Ordinal]
        };

        for (ControlIdentity* Identity : Rows)
        {
            const Outcome<ControlIdentity> Registered = Interaction.Register();
            if (!Registered.Resolved)
                return Outcome<bool>::Refuse(Registered.Error);

            *Identity = Registered.Resolve();
        }
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < TexturePaintContext::TextureChannelCeiling; ++Ordinal)
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

    if (RowCount > TexturePaintContext::TextureLayerCeiling)
        RowCount = TexturePaintContext::TextureLayerCeiling;

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

    Surface->Ground(Extent, Tinted.MenuLower, PillRadius, CornerAll);
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

void TexturePaintPanel::RecordStackPage(const PlaneExtent& Extent, TexturePaintContext& Applied,
                                        const TextureLayerRow* Rows, std::uint32_t RowCount)
{
    Surface->Ground(Extent, Tinted.Menu, 0.0f, CornerNone);

    const float Pad = Scaled.PanePad;

    const PlaneExtent Header = Spanning(Extent.MinimumX, Extent.MinimumY,
                                        Extent.Width(), Scaled.HeaderHeight);

    char Secondary[48] = {};
    std::snprintf(Secondary, sizeof(Secondary), "texture paint \u00B7 %u layers", RowCount);

    RecordLeafHeader(Header, SymbolSubject::LayerMerge, Covering(0x8A8A8Au),
                     "Layer Stack", Secondary);

    // 📐 The Add button and the search pill — the reference's tools row.
    const float ToolY = Scaled.LayerToolHeight;

    const PlaneExtent Add = Spanning(Extent.MinimumX + Pad, Header.MaximumY + Pad,
                                     ToolY * 2.2f, ToolY);

    const bool OnAdd = Add.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (OnAdd && Sampled.ContactPressed && !Ledger->AnyDisclosed())
        Ledger->Grab(AddLayer, ControlPart::Body);

    Surface->Ground(Add, Covering(0x141414u), ToolY * 0.5f, CornerAll);
    Surface->Edge(Add, OnAdd ? Covering(0x3A3A3Au) : Covering(0x242424u), 1.0f,
                  ToolY * 0.5f, CornerAll);

    const float Plus = 13.0f;
    Surface->Stroke(SymbolSubject::PlusCross,
                    Spanning(Add.MinimumX + 10.0f, Add.MinimumY + (ToolY - Plus) * 0.5f,
                             Plus, Plus),
                    OnAdd ? Tinted.Primary : Covering(0x8A8A8Au));

    Surface->TextRun(Add.MinimumX + 28.0f, Add.MinimumY + (ToolY - Scaled.RunSecondary) * 0.5f,
                     OnAdd ? Tinted.Primary : Covering(0x8A8A8Au), "Add layer", Scaled.RunSecondary);

    const PlaneExtent Search = Spanning(Add.MaximumX + Pad, Header.MaximumY + Pad,
                                        Extent.MaximumX - Add.MaximumX - Pad * 2.0f,
                                        ToolY);

    RecordSearchPill(Search, Applied);

    // 📐 The stack's filter card — the SAME FacetPanel the scene directory carries, with the layer
    //    categories.
    const FacetDeclaration StackFacetCard =
    {
        "Filters", StackFacetOptions, StackFacetColours,
        TexturePaintContext::TextureFacetCount, 0xFFFFFFFFu
    };

    const float FacetY = StackFacets.MeasureHeight(Extent.Width() - Pad * 2.0f, StackFacetCard,
                                                   Applied.FacetEnabled);

    const PlaneExtent FacetCard = Spanning(Extent.MinimumX + Pad, Search.MaximumY + Pad,
                                           Extent.Width() - Pad * 2.0f, FacetY);

    Discard(StackFacets.Record(FacetCard, StackFacetCard, Applied.FacetEnabled));

    // 📐 The page strip: Stack | Properties. The strip writes the same page Tab toggles.
    const float StripY = Scaled.ComponentY;

    const PlaneExtent Strip = Spanning(Extent.MinimumX, Extent.MaximumY - Scaled.FooterHeight - StripY,
                                       Extent.Width(), StripY);

    static const char* const PageCaptions[2] = { "Stack", "Properties" };
    const TabDeclaration PageDeclared{ PageCaptions, 2u };
    static_cast<void>(Controls.TabStrip(StackStrip, Strip, PageDeclared, Applied.StackPage));

    const PlaneExtent Footer = Spanning(Extent.MinimumX, Extent.MaximumY - Scaled.FooterHeight,
                                        Extent.Width(), Scaled.FooterHeight);

    const PlaneExtent Body = Spanning(Extent.MinimumX + 3.0f, FacetCard.MaximumY + Pad,
                                      Extent.Width() - 6.0f,
                                      Strip.MinimumY - FacetCard.MaximumY - Pad);

    if (Body.MaximumY <= Body.MinimumY)
    {
        StackFacets.RecordDeferred();
        return;
    }

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
        const float MaskY = Current.MaskDeclared ? (Scaled.RowHeight * 0.82f + Scaled.LayerRowGap) : 0.0f;
        const float RowY = Scaled.RowHeight + MaskY;

        const PlaneExtent Row = Spanning(Body.MinimumX, Sweep, Body.Width(), RowY);

        Sweep += RowY + Scaled.LayerRowGap;

        if (Surface->Excluded(Row))
            continue;

        if (RowTally < TexturePaintContext::TextureLayerCeiling)
        {
            RowRects[RowTally] = Row;
            ++RowTally;
        }

        RecordStackRow(Row, Applied, Current, Ordinal);

        if (Current.MaskDeclared)
        {
            const PlaneExtent Mask = Spanning(Body.MinimumX + Scaled.LayerSpineX * 0.8f,
                                              Row.MinimumY + Scaled.RowHeight,
                                              Body.Width() - Scaled.LayerSpineX * 0.8f,
                                              Scaled.RowHeight * 0.82f);
            RecordMaskRow(Mask, Applied, Current, Ordinal);
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

    // 📐 The footer: shown / hidden counts and the reorder hint.
    Surface->Ground(Footer, Tinted.MenuLower, 0.0f, CornerNone);
    Surface->Ground(Spanning(Footer.MinimumX, Footer.MinimumY, Footer.Width(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    std::uint32_t Shown = 0u;
    std::uint32_t Masks = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < RowCount; ++Ordinal)
    {
        if (Applied.LayerPresent[Ordinal])
            ++Shown;

        if (Rows[Ordinal].MaskDeclared)
            ++Masks;
    }

    char Tallied[32] = {};
    std::snprintf(Tallied, sizeof(Tallied), "%u shown \u00B7 %u masks", Shown, Masks);

    const float FootRun = Scaled.RunFine;
    const float FootTop = Footer.MinimumY + (Footer.Height() - FootRun) * 0.5f;

    Surface->TextRun(Footer.MinimumX + Scaled.HeaderPadX, FootTop, Tinted.Muted, Tallied, FootRun);

    const float HintRun = Surface->MeasureRun("Tab for properties", FootRun, 0.0f);

    Surface->TextRun(Footer.MaximumX - Scaled.HeaderPadX - HintRun, FootTop,
                     Tinted.Faint, "Tab for properties", FootRun);

    // 🔴 The filter card's dropdown is deferred, exactly as the scene directory's is.
    StackFacets.RecordDeferred();
}

void TexturePaintPanel::RecordStackRow(const PlaneExtent& Row, TexturePaintContext& Applied,
                                       const TextureLayerRow& Current, std::uint32_t Ordinal)
{
    const bool Taken   = Applied.LayerTaken == Ordinal && !Applied.MaskTaken;
    // 🔴 The layer row's own hover excludes the mask strip beneath it: the row's extent carries the
    //    attached mask row too, and a click there must address the MASK, never the layer — the
    //    reported defect where the mask row could not be taken.
    const bool Hovered = Row.Encloses(Sampled.PositionX, Sampled.PositionY) &&
                         !(Current.MaskDeclared &&
                           Sampled.PositionY >= Row.MinimumY + Scaled.RowHeight);
    const bool Absent  = !Applied.LayerPresent[Ordinal];
    const bool Branch  = Current.EnclosedCount > 0u;

    const float Coverage = Absent ? 0.5f : 1.0f;

    // ① The interaction cells: the chevron, the eye, the row, and the unfold.
    const float LeadX = Row.MinimumX + Scaled.RowLeadX
                      + static_cast<float>(Current.Depth) * Scaled.RowStepX;

    const PlaneExtent Chevron = Spanning(LeadX,
                                         Row.MinimumY + (Scaled.RowHeight - Scaled.ChevronExtent) * 0.5f,
                                         Scaled.ChevronExtent, Scaled.ChevronExtent);
    const float EyeExtent = Scaled.GlyphExtent * (16.0f / 18.0f);
    const PlaneExtent Eye = Spanning(Row.MaximumX - Scaled.KebabExtent - Scaled.PanePad * 2.0f - EyeExtent,
                                     Row.MinimumY + (Scaled.RowHeight - EyeExtent) * 0.5f,
                                     EyeExtent, EyeExtent);
    const PlaneExtent Unfold = Spanning(Row.MaximumX - Scaled.KebabExtent - Scaled.PanePad * 0.5f,
                                        Row.MinimumY + (Scaled.RowHeight - Scaled.ChevronExtent) * 0.5f,
                                        Scaled.ChevronExtent, Scaled.ChevronExtent);

    const bool OnChevron = Branch && Chevron.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool OnEye     = Eye.Encloses(Sampled.PositionX, Sampled.PositionY);
    const bool OnUnfold  = Unfold.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Sampled.ContactPressed && !Ledger->AnyDisclosed())
    {
        if (OnChevron)
            Ledger->Grab(LayerChevrons[Ordinal], ControlPart::Chevron);
        else if (OnEye)
            Ledger->Grab(LayerEyes[Ordinal], ControlPart::Body);
        else if (OnUnfold)
            Ledger->Grab(LayerUnfolds[Ordinal], ControlPart::Body);
        else if (Hovered)
            Ledger->Grab(LayerContacts[Ordinal], ControlPart::Body);
    }

    if (OnChevron && Ledger->Released(LayerChevrons[Ordinal]))
        Applied.LayerExpanded[Ordinal] = !Applied.LayerExpanded[Ordinal];

    if (OnEye && Ledger->Released(LayerEyes[Ordinal]))
        Applied.LayerPresent[Ordinal] = !Applied.LayerPresent[Ordinal];

    if (OnUnfold && Ledger->Released(LayerUnfolds[Ordinal]))
        Applied.LayerUnfolded[Ordinal] = !Applied.LayerUnfolded[Ordinal];

    if (Hovered && !OnChevron && !OnEye && !OnUnfold && Ledger->Released(LayerContacts[Ordinal]))
    {
        Applied.LayerTaken = Ordinal;
        Applied.MaskTaken  = false;
    }

    Ledger->DeclareHovered(LayerContacts[Ordinal], Hovered, HoverOver);

    // ② The spine, the ground and the row.
    Surface->Ground(Spanning(Row.MinimumX, Row.MinimumY, 3.0f, Scaled.RowHeight),
                    Faded(Covering(Current.TagHue), Absent ? 0.3f : 0.9f), 0.0f, CornerNone);

    if (Taken)
        Surface->Ground(Spanning(Row.MinimumX + 3.0f, Row.MinimumY,
                                 Row.Width() - 3.0f, Scaled.RowHeight),
                        Faded(Tinted.EntityTaken, Coverage), Scaled.FieldRadius, CornerAll);
    else if (Hovered)
        Surface->Ground(Spanning(Row.MinimumX + 3.0f, Row.MinimumY,
                                 Row.Width() - 3.0f, Scaled.RowHeight),
                        Faded(Tinted.RowHovered, Coverage), Scaled.FieldRadius, CornerAll);

    if (Branch)
        Surface->Stroke(Applied.LayerExpanded[Ordinal]
                        ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                        Chevron, Faded(Tinted.Faint, Coverage));

    // ③ The thumb: a square with the classification glyph badge.
    const float ThumbExtent = 32.0f;
    const PlaneExtent Thumb = Spanning(LeadX + Scaled.ChevronExtent + Scaled.PanePad,
                                       Row.MinimumY + (Scaled.RowHeight - ThumbExtent) * 0.5f,
                                       ThumbExtent, ThumbExtent);

    Surface->Ground(Thumb, Faded(Covering(0x141414u), Coverage), 2.0f, CornerAll);
    Surface->Edge(Thumb, Faded(Covering(0x242424u), Coverage), 1.0f, 2.0f, CornerAll);

    const float Figure = ThumbExtent * 0.55f;

    Surface->Stroke(TextureLayerGlyph(Current.Classified),
                    Spanning(Thumb.MinimumX + (ThumbExtent - Figure) * 0.5f,
                             Thumb.MinimumY + (ThumbExtent - Figure) * 0.5f,
                             Figure, Figure),
                    Faded(TextureLayerHue(Current.Classified), Coverage));

    // ④ The meta: name and the small detail run.
    const float NamingRun  = Scaled.RunPrimary;
    const float NamingTop  = Row.MinimumY + (Scaled.RowHeight * 0.5f - NamingRun * 1.3f) * 0.5f;
    const float MetaLead   = Thumb.MaximumX + Scaled.PanePad;
    const float NamingCeiling = Eye.MinimumX - Scaled.PanePad;

    Surface->TextRunTruncated(MetaLead, NamingTop, NamingCeiling,
                              Faded(Taken ? Tinted.Primary : Tinted.Muted, Coverage),
                              Current.Naming, NamingRun);

    const float SubRun = Scaled.RunFine;
    const float SubTop = NamingTop + NamingRun * 1.3f;

    char Sub[96] = {};
    std::snprintf(Sub, sizeof(Sub), "%s \u00B7 %s \u00B7 %u%%",
                  TextureLayerText(Current.Classified),
                  Current.Detail[0] != '\0' ? Current.Detail : Current.Blend,
                  Current.Opacity);

    Surface->TextRunTruncated(MetaLead, SubTop, NamingCeiling,
                              Faded(Tinted.Faint, Coverage), Sub, SubRun);

    // ⑤ The chips: mask, channels.
    float ChipLead = NamingCeiling;

    if (Current.MaskDeclared)
    {
        const char* MaskChip = "MASK";
        const float ChipRun = Scaled.RunFiner;
        const float ChipX = Scaled.RunFiner * 0.6f;

        Surface->Ground(Spanning(ChipLead - ChipX - Surface->MeasureRun(MaskChip, ChipRun, 0.0f) - 10.0f,
                                 Row.MinimumY + (Scaled.RowHeight - 17.0f) * 0.5f,
                                 Surface->MeasureRun(MaskChip, ChipRun, 0.0f) + 10.0f, 17.0f),
                        Faded(Covering(0x2A2A2Au), Coverage), 9.0f, CornerAll);

        Surface->TextRun(ChipLead - ChipX - Surface->MeasureRun(MaskChip, ChipRun, 0.0f) - 5.0f,
                         Row.MinimumY + (Scaled.RowHeight - ChipRun) * 0.5f,
                         Faded(Covering(0xDCDCDCu), Coverage), MaskChip, ChipRun, 0.0f, true);

        ChipLead -= Surface->MeasureRun(MaskChip, ChipRun, 0.0f) + ChipX * 2.0f + 10.0f;
    }

    if (Current.ChannelCount > 0u)
    {
        char ChannelChip[12] = {};
        std::snprintf(ChannelChip, sizeof(ChannelChip), "%u/8 CH", Current.ChannelCount);

        const float ChipRun = Scaled.RunFiner;

        Surface->Ground(Spanning(ChipLead - ChipRun * 0.6f - Surface->MeasureRun(ChannelChip, ChipRun, 0.0f) - 10.0f,
                                 Row.MinimumY + (Scaled.RowHeight - 17.0f) * 0.5f,
                                 Surface->MeasureRun(ChannelChip, ChipRun, 0.0f) + 10.0f, 17.0f),
                        Faded(Covering(0x1F1F1Fu), Coverage), 9.0f, CornerAll);

        Surface->TextRun(ChipLead - ChipRun * 0.6f - Surface->MeasureRun(ChannelChip, ChipRun, 0.0f) - 5.0f,
                         Row.MinimumY + (Scaled.RowHeight - ChipRun) * 0.5f,
                         Faded(Covering(0x9A9A9Au), Coverage), ChannelChip, ChipRun, 0.0f, true);
    }

    // ⑥ The eye and the unfold.
    if (Hovered || Absent || Taken)
    {
        if (OnEye)
            Surface->Ground(Eye, Tinted.TileHovered, 3.0f, CornerAll);

        Surface->Stroke(Absent ? SymbolSubject::EyeClosed : SymbolSubject::EyeOpen, Eye,
                        OnEye ? Tinted.Primary : Tinted.Faint);
    }

    Surface->Stroke(Applied.LayerUnfolded[Ordinal] ? SymbolSubject::ChevronDown
                                                   : SymbolSubject::ChevronRight,
                    Unfold, OnUnfold ? Tinted.Primary : Tinted.Faint);

    // ⑦ The inline unfolded card: the blend and the opacity mini-bar — the small details the user
    //    wants on the row; everything else lives on the properties page.
    if (Applied.LayerUnfolded[Ordinal])
    {
        const float FoldY = Scaled.LayerFoldPad * 2.0f + Scaled.LayerFieldRow * 2.0f;
        const PlaneExtent Fold = Spanning(Row.MinimumX + 3.0f, Row.MinimumY + Scaled.RowHeight,
                                          Row.Width() - 3.0f, FoldY);

        Surface->Ground(Fold, Covering(0x0B0B0Bu), 0.0f, CornerNone);

        const float RowRun = Scaled.RunFine;
        const float RowTop = Fold.MinimumY + Scaled.LayerFoldPad;

        Surface->TextRun(Fold.MinimumX + Scaled.PanePad * 2.0f, RowTop, Tinted.Muted,
                         "Blend", RowRun);
        Surface->TextRun(Fold.MinimumX + Scaled.PanePad * 2.0f + 64.0f, RowTop,
                         Tinted.Primary, Current.Blend, RowRun);

        // 📐 The opacity mini-bar.
        const float BarX = Fold.MaximumX - 150.0f;
        const float BarY = RowTop + 3.0f;

        Surface->Ground(Spanning(BarX, BarY, 100.0f, 4.0f), Covering(0x242424u), 2.0f, CornerAll);
        Surface->Ground(Spanning(BarX, BarY, 100.0f * static_cast<float>(Current.Opacity) / 100.0f, 4.0f),
                        Covering(0xFFFFFFu), 2.0f, CornerAll);

        char Opacity[16] = {};
        std::snprintf(Opacity, sizeof(Opacity), "%u%%", Current.Opacity);

        Surface->TextRun(Fold.MaximumX - Scaled.PanePad * 2.0f - 44.0f, RowTop,
                         Tinted.Primary, Opacity, RowRun);

        Surface->TextRun(Fold.MinimumX + Scaled.PanePad * 2.0f,
                         RowTop + Scaled.LayerFieldRow, Tinted.Muted, "Channels", RowRun);
        Surface->TextRun(Fold.MinimumX + Scaled.PanePad * 2.0f + 64.0f,
                         RowTop + Scaled.LayerFieldRow, Tinted.Primary,
                         Current.ChannelCount > 0u ? Current.Channels[0] : "none", RowRun);
    }
}

void TexturePaintPanel::RecordMaskRow(const PlaneExtent& Row, TexturePaintContext& Applied,
                                      const TextureLayerRow& Current, std::uint32_t Ordinal)
{
    const bool Taken   = Applied.LayerTaken == Ordinal && Applied.MaskTaken;
    const bool Hovered = Row.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Sampled.ContactPressed && Hovered && !Ledger->AnyDisclosed())
        Ledger->Grab(MaskContacts[Ordinal], ControlPart::Body);

    if (Hovered && Ledger->Released(MaskContacts[Ordinal]))
    {
        Applied.LayerTaken = Ordinal;
        Applied.MaskTaken  = true;
    }

    Ledger->DeclareHovered(MaskContacts[Ordinal], Hovered, HoverOver);

    Surface->Ground(Row, Taken ? Faded(Tinted.EntityTaken, 1.0f) : Covering(0x101010u),
                    Scaled.FieldRadius, CornerAll);
    Surface->Edge(Row, Taken ? Tinted.Accent : Tinted.Hairline, 1.0f, Scaled.FieldRadius, CornerAll);

    const float GlyphExtent = 14.0f;
    const PlaneExtent Glyph = Spanning(Row.MinimumX + Scaled.PanePad * 1.5f,
                                       Row.MinimumY + (Row.Height() - GlyphExtent) * 0.5f,
                                       GlyphExtent, GlyphExtent);

    Surface->Stroke(SymbolSubject::MaskStencil, Glyph,
                    Faded(Covering(0xDCDCDCu), Applied.LayerPresent[Ordinal] ? 1.0f : 0.4f));

    const float Run = Scaled.RunSecondary;

    Surface->TextRun(Glyph.MaximumX + Scaled.PanePad,
                     Row.MinimumY + (Row.Height() - Run) * 0.5f,
                     Taken ? Tinted.Primary : Covering(0xDCDCDCu), "Mask", Run, 0.0f, true);

    char Sub[64] = {};
    std::snprintf(Sub, sizeof(Sub), "%s \u00B7 %u%%%s",
                  Current.Source[0] != '\0' ? Current.Source : "Paint",
                  Current.MaskStrength,
                  Current.MaskInverted ? " \u00B7 INV" : "");

    Surface->TextRun(Row.MinimumX + Scaled.PanePad * 1.5f + 64.0f,
                     Row.MinimumY + (Row.Height() - Scaled.RunFine) * 0.5f,
                     Tinted.Faint, Sub, Scaled.RunFine);
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
                  TextureLayerText(Current.Classified), Current.Blend);

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
                  Current.Opacity, Applied.MaskTaken ? "mask" : TextureLayerText(Current.Classified));

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

        const PlaneExtent Row = Spanning(Body.MinimumX, Sweep, Body.Width(), RowY);

        Sweep += RowY + Scaled.LayerRowGap;

        if (Sweep > Body.MaximumY)
            break;

        RecordChannelRow(Row, Applied, Channel);
    }

    Surface->Release();
}

void TexturePaintPanel::RecordChannelRow(const PlaneExtent& Row, TexturePaintContext& Applied,
                                         std::uint32_t Channel)
{
    const std::uint32_t Layer = Applied.LayerTaken;
    const bool On = Applied.ChannelOn[Layer][Channel];
    const bool Folded = Applied.ChannelFolded[Channel];

    // 📐 The fold: the row's leading chevron.
    const PlaneExtent Fold = Spanning(Row.MinimumX,
                                      Row.MinimumY + (Row.Height() - Scaled.ChevronExtent) * 0.5f,
                                      Scaled.ChevronExtent, Scaled.ChevronExtent);

    if (Sampled.ContactPressed && Fold.Encloses(Sampled.PositionX, Sampled.PositionY) &&
        !Ledger->AnyDisclosed())
    {
        Ledger->Grab(ChannelFolds[Channel], ControlPart::Chevron);
    }

    if (Fold.Encloses(Sampled.PositionX, Sampled.PositionY) &&
        Ledger->Released(ChannelFolds[Channel]))
    {
        Applied.ChannelFolded[Channel] = !Folded;
    }

    Surface->Stroke(Folded ? SymbolSubject::ChevronRight : SymbolSubject::ChevronDown,
                    Fold, Tinted.Faint);

    // 📐 The dot: the channel's on/off.
    const float DotExtent = 18.0f;
    const PlaneExtent Dot = Spanning(Fold.MaximumX + Scaled.PanePad,
                                     Row.MinimumY + (Row.Height() - DotExtent) * 0.5f,
                                     DotExtent, DotExtent);

    const bool OnDot = Dot.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Sampled.ContactPressed && OnDot && !Ledger->AnyDisclosed())
        Ledger->Grab(ChannelDots[Channel], ControlPart::Body);

    if (OnDot && Ledger->Released(ChannelDots[Channel]))
        Applied.ChannelOn[Layer][Channel] = !On;

    Surface->Medallion(Dot.MinimumX + DotExtent * 0.5f, Dot.MinimumY + DotExtent * 0.5f,
                       DotExtent * 0.5f,
                       On ? Covering(0xFFFFFFu) : Covering(0x111111u));

    // 📐 The name and the blend + opacity.
    const float NameRun = Scaled.RunPrimary;
    const float NameTop = Row.MinimumY + (Row.Height() - NameRun) * 0.5f;

    Surface->TextRun(Dot.MaximumX + Scaled.PanePad, NameTop,
                     On ? Tinted.Primary : Tinted.Muted, TextureChannelText(Channel), NameRun);

    const char* Blend = BlendOptions[Applied.ChannelBlendTaken[Layer][Channel] % 6u];

    const float BlendRun = Scaled.RunFine;
    const float BlendX = Row.MaximumX - 190.0f;

    Surface->TextRun(BlendX, Row.MinimumY + (Row.Height() - BlendRun) * 0.5f,
                     Tinted.Muted, Blend, BlendRun);

    // 📐 The opacity mini-bar and its value.
    const float BarX = Row.MaximumX - 96.0f;
    const float BarY = Row.MinimumY + (Row.Height() - 4.0f) * 0.5f;
    const std::uint32_t Amount = Applied.ChannelAmount[Layer][Channel];

    Surface->Ground(Spanning(BarX, BarY, 56.0f, 4.0f), Covering(0x242424u), 2.0f, CornerAll);
    Surface->Ground(Spanning(BarX, BarY, 56.0f * static_cast<float>(Amount) / 100.0f, 4.0f),
                    Covering(0xFFFFFFu), 2.0f, CornerAll);

    char Value[8] = {};
    std::snprintf(Value, sizeof(Value), "%u", Amount);

    Surface->TextRun(Row.MaximumX - 34.0f, Row.MinimumY + (Row.Height() - BlendRun) * 0.5f,
                     Tinted.Primary, Value, BlendRun);

    // 📐 The blend and amount interactions (dummy but live: click cycles the blend, drag arms set
    //    the amount by press position).
    const PlaneExtent BlendCell = Spanning(BlendX, Row.MinimumY, 86.0f, Row.Height());

    if (Sampled.ContactPressed && BlendCell.Encloses(Sampled.PositionX, Sampled.PositionY) &&
        !Ledger->AnyDisclosed())
    {
        Ledger->Grab(ChannelBlends[Channel], ControlPart::Body);
    }

    if (BlendCell.Encloses(Sampled.PositionX, Sampled.PositionY) &&
        Ledger->Released(ChannelBlends[Channel]))
    {
        Applied.ChannelBlendTaken[Layer][Channel] =
            (Applied.ChannelBlendTaken[Layer][Channel] + 1u) % 6u;
    }

    if (Sampled.ContactPressed && BarX < Sampled.PositionX && Sampled.PositionX < BarX + 56.0f &&
        !Ledger->AnyDisclosed())
    {
        Ledger->Grab(ChannelOps[Channel], ControlPart::Body);
    }

    if (Ledger->Holding(ChannelOps[Channel]))
    {
        const float Fraction = Held((Sampled.PositionX - BarX) / 56.0f, 0.0f, 1.0f);
        Applied.ChannelAmount[Layer][Channel] = static_cast<std::uint32_t>(Fraction * 100.0f + 0.5f);
    }
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
                    Covering(0x0A0A0Bu), Scaled.CardRadius, CornerAll);
    Surface->Edge(Spanning(Extent.MinimumX + Pad, Sweep, Extent.Width() - Pad * 2.0f,
                           Scaled.ComponentY),
                  Tinted.Hairline, 1.0f, Scaled.CardRadius, CornerAll);

    char MaskCaption[64] = {};
    std::snprintf(MaskCaption, sizeof(MaskCaption), "Mask \u00B7 %s",
                  Current.Source[0] != '\0' ? Current.Source : "Paint");

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
    SourceDeclared.Options     = MaskSourceOptions;
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

    Surface->Ground(Switch, Inverted ? Tinted.EntityAccent : Tinted.Hairline,
                    Toggle * 0.25f, CornerAll);

    const float Knob = Toggle * 0.5f - 2.0f;
    Surface->Medallion(Inverted ? Switch.MaximumX - Knob - 1.0f : Switch.MinimumX + Knob + 1.0f,
                       Switch.MinimumY + Toggle * 0.25f, Knob, Covering(0xFFFFFFu));

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
                    Covering(0x0A0A0Bu), Scaled.CardRadius, CornerAll);
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
                  Current.Opacity);

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

        if (Rows[Ordinal].MaskDeclared)
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

    Row("Blend", Folder.Blend);

    std::snprintf(Tallied, sizeof(Tallied), "%u%%", Folder.Opacity);
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
