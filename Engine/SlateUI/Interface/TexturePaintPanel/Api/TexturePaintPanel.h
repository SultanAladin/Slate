//============================================================================================================================================
//                                                           TEXTUREPAINTPANEL.H
//============================================================================================================================================
// 🧩 The editor's texture-paint layer stack — a dedicated sibling of
//    SceneDirectoryPanel, presenting the LayerstackV1 and
//    ChannelPropertyPanel references inside a workspace leaf.
//
//    🔴 WHAT THIS PANEL IS. The stack page records every layer's SMALL details
//       (badge, name, blend, opacity bar, chips, attached mask row) and nothing
//       more; the full details live on the PROPERTIES page, reached with Tab
//       exactly as the user's flow describes:
//         - a layer row   + Tab  → Channel Properties (the per-channel
//           panels of ChannelPropertyPanel.html: dot, name, blend, opacity)
//         - a mask row    + Tab  → the Mask panel (source, density, invert,
//           applies-to channels)
//         - a decal       + Tab  → Decal settings   (placement sliders)
//         - a pattern     + Tab  → Pattern settings (tiling, jitter, seed)
//         - a generator   + Tab  → Generator settings
//         - a FOLDER      + Tab  → the COMBINED stack properties (counts,
//           mask count, channel union, passthrough) — one summary, not per
//           child, exactly as the user asked.
//       The two pages slide as a carousel (the leaf is a 200 %-wide strip),
//       and the properties page carries a strip of the tabs the selection
//       offers. NO history panel — the reference's undo/redo spine is not
//       ported; the properties page is where the details live.
//
//    The SAME filter as the scene directory sits on both pages: a search pill
//    and the reusable FacetPanel — layer categories on the stack page, channel
//    groups (Base / Maps / Output) on the properties page.

#pragma once

#include "Contract/DeliveryContract.h"
#include "Shared/OverlayGeometry.slang.h"
#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"
#include "SlateUI/Interface/FacetPanel/Api/FacetPanel.h"
#include "SlateUI/Interface/InteractionIndex/Api/InteractionIndex.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"
#include "SlateUI/Interface/MotionIntegrator/Api/MotionIntegrator.h"
#include "SlateUI/Interface/SceneDirectoryPanel/Api/SceneDirectoryContract.h"
#include "SlateUI/Interface/TexturePaintPanel/Api/TexturePaintContract.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   WHAT THE HOST OWNS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every datum the texture-paint panel presents, owned by the host and written through by the panel.
/// tag   contract
struct TexturePaintContext
{
    static constexpr std::uint32_t TextureLayerCeiling    = 16u;   // [-] - rows, folders included
    static constexpr std::uint32_t TextureChannelCeiling  = 8u;    // [-] - matches TextureChannelCeiling
    static constexpr std::uint32_t TextureRetentionCeiling = 48u;  // [-] - the search run, terminator included
    static constexpr std::uint32_t TextureFacetCount      = 8u;    // [-] - Paint … Filter
    static constexpr std::uint32_t TextureChannelFacetCount = 3u;  // [-] - Base, Maps, Output

    // 📐 The selection and the pages. `StackPage` is the carousel: 0 the stack, 1 the properties.
    //    `PropertyTab` is which properties panel the strip shows — 0 Channels, 1 Mask, 2 Settings
    //    (decal / pattern / generator / the folder's combined stack). Tab toggles the stack page,
    //    then the property tabs the selection offers.
    std::uint32_t              LayerTaken    = 0u;           // [-] - which row is taken
    bool                       MaskTaken     = false;        // [-] - the taken row's mask is taken
    std::uint32_t              StackPage     = 0u;           // [-] - 0 Stack, 1 Properties
    std::uint32_t              PropertyTab   = 0u;           // [-] - 0 Channels, 1 Mask, 2 Settings

    // 📝 The search and the filters — the same pair the scene directory carries.
    char                       Retention[TextureRetentionCeiling] = {};   // [-] - the search run
    bool                       SearchTaken   = false;       // [-] - the search pill holds the contact
    bool                       FacetEnabled[TextureFacetCount]     = {};  // [-] - layer categories
    bool                       ChannelFacet[TextureChannelFacetCount] = {};  // [-] - channel groups

    // 📝 The rows' own conditions: disclosure, presence, the per-row detail card, and the channel
    //    each layer is showing on the properties page.
    bool                       LayerExpanded[TextureLayerCeiling]  = {};
    bool                       LayerPresent[TextureLayerCeiling]   = {};
    bool                       LayerUnfolded[TextureLayerCeiling]  = {};
    std::uint32_t              ChannelTaken[TextureLayerCeiling]   = {};
    bool                       ChannelFolded[TextureChannelCeiling] = {};
    bool                       MaskFolded    = false;       // [-] - the mask panel's sections
    bool                       SettingFolded = false;       // [-] - the settings panel's sections

    // 📝 The properties page's editable scratch — the panel writes these, the host seeds them from
    //    the rows at bring-up. Per-layer channel state, mask state and the settings sliders.
    bool                       ChannelOn[TextureLayerCeiling][TextureChannelCeiling] = {};
    std::uint32_t              ChannelAmount[TextureLayerCeiling][TextureChannelCeiling] = {};
    std::uint32_t              ChannelBlendTaken[TextureLayerCeiling][TextureChannelCeiling] = {};
    std::uint32_t              MaskDensity[TextureLayerCeiling]    = {};
    bool                       MaskInverted[TextureLayerCeiling]   = {};
    std::uint32_t              MaskSourceTaken[TextureLayerCeiling] = {};
    std::uint32_t              SettingAmount[TextureLayerCeiling][4] = {};
    std::uint32_t              SettingToggle[TextureLayerCeiling]  = {};
    std::uint32_t              SettingChoice[TextureLayerCeiling]  = {};
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE PANEL
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Records the editor's layer stack — the stack page and the selection-driven properties page —
///    inside the extent the editor's panel chrome hands over.
/// tag   owning
class TexturePaintPanel
{
public:

    /// 🧩 Exactly how many control identities `Construct` claims, stated where they are claimed.
    static constexpr std::uint32_t RegistrationDemand =
          TexturePaintContext::TextureLayerCeiling * 4u    // [-] - contact, chevron, eye, unfold per row
        + TexturePaintContext::TextureLayerCeiling         // [-] - one mask contact per row
        + 4u                                               // [-] - the page strip, the property strip,
                                                           //       the search field and the Add button
        + FacetPanel::FacetCapacity + 2u                   // [-] - the stack page's filter card
        + FacetPanel::FacetCapacity + 2u                   // [-] - the properties page's channel filter
        + TexturePaintContext::TextureChannelCeiling * 4u  // [-] - fold, dot, blend, opacity per channel
        + 6u;                                              // [-] - the mask panel's rows and toggles
                                                           //       plus the settings panel's rows

    TexturePaintPanel()                                   = default;
    TexturePaintPanel(const TexturePaintPanel&)           = delete;
    TexturePaintPanel& operator=(const TexturePaintPanel&) = delete;
    ~TexturePaintPanel()                                  = default;

    /// 🧩 Borrows the recording facilities and registers every identity and interpolant the panel needs.
    /// out   Result  [-]  refuses with ContentUnsupported when a construction already stands
    /// cost  🚩
    /// tag   api, nonthrowing
    Outcome<bool> Construct(InteractionIndex&              Interaction,
                            MotionIntegrator&              Integrator,
                            RecordingSurface&              Surface,
                            const ThemeProfile& Resolved);

    /// 🧩 Samples the contact, advances the shared controls and the two filter cards, and toggles
    ///    the carousel when Tab arrives.
    /// in    Applied   [-]  the host's context; `StackPage` and `PropertyTab` are written here
    /// in    TabPressed [-]  the seam's Summon (Tab), edge-triggered and unrepeated
    /// note  🔴 This does not advance the ledger; the tick owner advances it once.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Advance(const PointerCondition& Sampled, double Elapsed,
                 TexturePaintContext& Applied,
                 const TextureLayerRow* Rows, std::uint32_t RowCount,
                 bool TabPressed);

    /// 🧩 Re-applies every scaled extent after the appearance was resolved against a new display.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Reapply(const ThemeProfile& Resolved);

    /// 🧩 Records the whole leaf: the carousel's two pages, the stack and the properties.
    /// in    Rows     [-]  the layer rows, borrowed for the tick
    /// in    RowCount [-]  how many stand
    /// cost  🚩
    /// tag   api, nonallocating, nonthrowing
    void Record(const PlaneExtent& Extent, TexturePaintContext& Applied,
                const TextureLayerRow* Rows, std::uint32_t RowCount);

    /// 🧩 Returns the panel to its unconstructed condition.
    /// cost  ✔️
    /// tag   api, nonthrowing
    void Reset();

    /// 🧩 The extent of one row the last `Record` drew, between the filter card and the page strip.
    /// note  ⚠️ Valid only until the next `Record`, and empty when the ordinal is out of range. The
    ///        extent includes the layer row AND its attached mask row when one stands — the same
    ///        block the panel drew.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    PlaneExtent RowExtent(std::uint32_t Ordinal) const
    {
        return Ordinal < RowTally ? RowRects[Ordinal] : PlaneExtent{};
    }

    /// 🧩 How many rows the last `Record` drew, for the host's probe.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    std::uint32_t DrawnRows() const { return RowTally; }

private:

    void RecordStackPage(const PlaneExtent& Extent, TexturePaintContext& Applied,
                         const TextureLayerRow* Rows, std::uint32_t RowCount);
    void RecordStackRow(const PlaneExtent& Row, TexturePaintContext& Applied,
                        const TextureLayerRow& Current, std::uint32_t Ordinal);
    void RecordMaskRow(const PlaneExtent& Row, TexturePaintContext& Applied,
                       const TextureLayerRow& Current, std::uint32_t Ordinal);
    void RecordPropertiesPage(const PlaneExtent& Extent, TexturePaintContext& Applied,
                              const TextureLayerRow* Rows, std::uint32_t RowCount);
    void RecordSearchPill(const PlaneExtent& Extent, TexturePaintContext& Applied);
    void RecordChannelCard(const PlaneExtent& Extent, TexturePaintContext& Applied,
                           const TextureLayerRow& Current);
    void RecordChannelRow(const PlaneExtent& Row, TexturePaintContext& Applied,
                          std::uint32_t Channel);
    void RecordMaskCard(const PlaneExtent& Extent, TexturePaintContext& Applied,
                        const TextureLayerRow& Current);
    void RecordSettingsCard(const PlaneExtent& Extent, TexturePaintContext& Applied,
                            const TextureLayerRow& Current);
    void RecordFolderCard(const PlaneExtent& Extent, TexturePaintContext& Applied,
                          const TextureLayerRow* Rows, std::uint32_t RowCount);
    void RecordLeafHeader(const PlaneExtent& Extent, SymbolSubject Glyph,
                          const ThemeToken& Hue, const char* Titled, const char* Secondary);
    std::uint32_t PropertyTabCount(const TexturePaintContext& Applied,
                                   const TextureLayerRow& Current) const;

    InteractionIndex*           Ledger = nullptr;        // [-] - borrowed; never owned
    MotionIntegrator*           Motion = nullptr;        // [-] - borrowed; never owned
    RecordingSurface*           Surface = nullptr;       // [-] - borrowed; never owned
    const ThemeProfile*         Appearance = nullptr;    // [-] - borrowed; never owned
    ShellColour                 Tinted = {};             // [-] - the shell's own colour record
    ShellMetric                 Scaled = {};             // [-] - re-applied on every appearance resolve

    ControlPanel                Controls = {};           // [-] - strips, sliders, fold animation
    ComponentSpecification      SharedControls = {};     // [-] - magnitude rows, toggles, selection
    FacetPanel                  StackFacets = {};        // [-] - the stack page's filter card
    FacetPanel                  ChannelFacets = {};      // [-] - the properties page's channel filter

    PointerCondition            Sampled = {};            // [-] - this tick's contact

    ControlIdentity LayerContacts[TexturePaintContext::TextureLayerCeiling]   = {};
    ControlIdentity LayerChevrons[TexturePaintContext::TextureLayerCeiling]  = {};
    ControlIdentity LayerEyes[TexturePaintContext::TextureLayerCeiling]      = {};
    ControlIdentity LayerUnfolds[TexturePaintContext::TextureLayerCeiling]   = {};
    ControlIdentity MaskContacts[TexturePaintContext::TextureLayerCeiling]   = {};
    ControlIdentity StackStrip      = {};
    ControlIdentity PropertyStrip   = {};
    ControlIdentity SearchField     = {};
    ControlIdentity AddLayer        = {};
    ControlIdentity ChannelFolds[TexturePaintContext::TextureChannelCeiling] = {};
    ControlIdentity ChannelDots[TexturePaintContext::TextureChannelCeiling]  = {};
    ControlIdentity ChannelBlends[TexturePaintContext::TextureChannelCeiling] = {};
    ControlIdentity ChannelOps[TexturePaintContext::TextureChannelCeiling]   = {};
    ControlIdentity MaskRows[4]     = {};
    ControlIdentity SettingRows[4]  = {};

    PlaneExtent RowRects[TexturePaintContext::TextureLayerCeiling] = {};   // [-] - the last Record's rows
    std::uint32_t RowTally = 0u;                                          // [-] - how many stood
};

} // namespace Slate
