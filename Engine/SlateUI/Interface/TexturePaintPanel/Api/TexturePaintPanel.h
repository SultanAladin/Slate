//============================================================================================================================================
//                                                           TEXTUREPAINTPANEL.H
//============================================================================================================================================
// 🧩 The editor's texture-paint layer stack — a dedicated sibling of
//    SceneDirectoryPanel, presenting the LayerstackV1 reference inside a
//    workspace leaf, appearance and interactions faithful to the HTML:
//
//    HEADER   "LAYERS" + the "N · Mm" count chip + the SOLO chip (while a row
//             is solo'd) + undo/redo (drawn disabled — no history spine) +
//             the expand toggle + the solid Add button.
//    TOOLS    the search pill ("Filter layers…") + the folder, mask and
//             collapse-all tools, exactly the reference's tools row.
//    ROWS     45 px entries with the 3 px colour tag (dotted on a mask), the
//             disclosure chevron, the eye, the 35 px square thumb with the
//             type badge, name + sub run, the chips (3D / L / MASK / n FX /
//             x/8 CH), the details chevron and the "more" menu.
//    MASKS    the attached 37 px mask row with the connector elbow, the
//             dashed border, the uppercase MASK name, the source · Gray 8 ·
//             density · INV sub run, chips and menu.
//    FOLDERS  children indented with the colour guide line.
//    FOOTER   the crumb, the blend pill + opacity slider, and the action bar
//             (paint / fill / adjustment / filter / decal / pattern · group /
//             duplicate / lock · move up / move down · delete).
//
//    🔴 WHAT THE PANEL IS. The stack page is the HTML's; the full details live
//       on the PROPERTIES page, reached with Tab or with a row's details
//       chevron exactly as the user's flow describes:
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
//       The two pages slide as a carousel, and the properties page carries a
//       strip of the tabs the selection offers. NO history panel — the
//       reference's undo/redo spine is not ported (the two header buttons
//       draw disabled); the properties page is where the details live.
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
///    The per-row working copies (opacity, blend, lock, mask, tag hue) are seeded from the rows at
///    bring-up and synchronised back through `TexturePaintStack::ApplyRequest` — the rows stay the model.
/// tag   contract
struct TexturePaintContext
{
    static constexpr std::uint32_t TextureRetentionCeiling = 48u;  // [-] - the search run, terminator included
    static constexpr std::uint32_t TextureFacetCount      = 8u;    // [-] - Paint … Filter
    static constexpr std::uint32_t TextureChannelFacetCount = 3u;  // [-] - Base, Maps, Output
    static constexpr std::uint32_t TextureSwatchCount     = 10u;   // [-] - the reference's COLORS run

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

    // 📝 The rows' own conditions: disclosure, presence, the details chevron's page travel, and the
    //    channel each layer is showing on the properties page.
    bool                       LayerExpanded[TextureLayerCeiling]  = {};
    bool                       LayerPresent[TextureLayerCeiling]   = {};
    std::uint32_t              ChannelTaken[TextureLayerCeiling]   = {};
    bool                       ChannelFolded[TextureChannelCeiling] = {};
    bool                       MaskFolded    = false;       // [-] - the mask panel's sections
    bool                       SettingFolded = false;       // [-] - the settings panel's sections

    // 📝 The per-row working copies — the fields the artist edits on the stack page itself, seeded
    //    from the rows at bring-up and written back by `TexturePaintStack::ApplyRequest`.
    std::uint32_t              LayerOpacity[TextureLayerCeiling]   = {};   // [%] - the footer slider
    std::uint32_t              LayerBlendTaken[TextureLayerCeiling] = {};  // [-] - into the blend roster
    bool                       LayerLocked[TextureLayerCeiling]    = {};
    bool                       MaskAttached[TextureLayerCeiling]   = {};
    bool                       MaskVisible[TextureLayerCeiling]    = {};
    std::uint32_t              LayerTagHue[TextureLayerCeiling]    = {};   // [-] - 0xRRGGBB, the entry tag
    std::uint32_t              SoloTaken     = 0xFFFFFFFFu;      // [-] - the solo'd row; absent for none
    bool                       WideRows      = false;        // [-] - the expand toggle's wide columns

    // 📝 The open menu. `MenuOpen` is 0 none, 1 the Add menu, 2 the layer menu, 3 the mask menu,
    //    4 the blend menu; `MenuRow` is the row the layer/mask menu hangs from.
    std::uint32_t              MenuOpen      = 0u;
    std::uint32_t              MenuRow       = 0u;

    // 📝 One structural request per tick — what the action bar asked for; the host drains it through
    //    `TexturePaintStack::ApplyRequest` after the record.
    std::uint32_t              Structural    = 0u;           // [-] - TexturePaintRequest

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
          TextureLayerCeiling * 5u          // [-] - contact, chevron, eye, details, more per row
        + TextureLayerCeiling * 4u          // [-] - contact, eye, details, more per attached mask
        + 9u                                // [-] - undo, redo, expand, add, solo, folder, mask, collapse, search
        + 14u                               // [-] - the blend field, the opacity row and the twelve bar buttons
        + 2u                                // [-] - the page strips
        + (FacetPanel::FacetCapacity + 2u) * 2u   // [-] - the two filter cards
        + TextureChannelCeiling * 4u        // [-] - fold, dot, blend, opacity per channel
        + 8u                                // [-] - the mask card's rows and the settings card's rows
        + 44u;                              // [-] - the four menu anchors plus menu items: add 7,
                                            //       layer 7, swatches 10, mask 3, blend 13

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
    void RecordStackHeader(const PlaneExtent& Header, TexturePaintContext& Applied,
                           std::uint32_t RowCount);
    void RecordStackTools(const PlaneExtent& Tools, TexturePaintContext& Applied);
    void RecordStackRow(const PlaneExtent& Row, TexturePaintContext& Applied,
                        const TextureLayerRow* Rows, std::uint32_t RowCount,
                        const TextureLayerRow& Current, std::uint32_t Ordinal);
    void RecordMaskRow(const PlaneExtent& Row, TexturePaintContext& Applied,
                       const TextureLayerRow* Rows, std::uint32_t RowCount,
                       const TextureLayerRow& Current, std::uint32_t Ordinal);
    void RecordStackFooter(const PlaneExtent& Footer, TexturePaintContext& Applied,
                           const TextureLayerRow* Rows, std::uint32_t RowCount);
    void RecordBarButton(ControlIdentity Target, const PlaneExtent& Cell, SymbolSubject Glyph,
                         TexturePaintContext& Applied, std::uint32_t Request,
                         bool Dimmed = false);
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

    // 📝 The popup menus — the reference's `.pop`: a rounded card of pill items, recorded above the
    //    whole page inside the leaf.
    void RecordMenu(const PlaneExtent& Extent, TexturePaintContext& Applied,
                    const TextureLayerRow* Rows, std::uint32_t RowCount);
    void RecordMenuOptions(const PlaneExtent& Card, const char* const* Captions,
                           const SymbolSubject* Glyphs, std::uint32_t OptionCount,
                           const char* const* Shortcuts, ControlIdentity* Identities,
                           TexturePaintContext& Applied, std::uint32_t* Writes);

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

    ControlIdentity HeaderUndo    = {};
    ControlIdentity HeaderRedo    = {};
    ControlIdentity HeaderExpand  = {};
    ControlIdentity HeaderAdd     = {};
    ControlIdentity SoloChip      = {};
    ControlIdentity ToolFolder    = {};
    ControlIdentity ToolMask      = {};
    ControlIdentity ToolCollapse  = {};
    ControlIdentity SearchField   = {};
    ControlIdentity BlendField    = {};
    ControlIdentity OpacityRow    = {};
    ControlIdentity BarButtons[12] = {};
    ControlIdentity StackStrip    = {};
    ControlIdentity PropertyStrip = {};

    ControlIdentity LayerContacts[TextureLayerCeiling]   = {};
    ControlIdentity LayerChevrons[TextureLayerCeiling]   = {};
    ControlIdentity LayerEyes[TextureLayerCeiling]       = {};
    ControlIdentity LayerDetails[TextureLayerCeiling]    = {};
    ControlIdentity LayerMores[TextureLayerCeiling]      = {};
    ControlIdentity MaskContacts[TextureLayerCeiling]    = {};
    ControlIdentity MaskEyes[TextureLayerCeiling]        = {};
    ControlIdentity MaskDetails[TextureLayerCeiling]     = {};
    ControlIdentity MaskMores[TextureLayerCeiling]       = {};

    ControlIdentity ChannelFolds[TextureChannelCeiling]  = {};
    ControlIdentity ChannelDots[TextureChannelCeiling]   = {};
    ControlIdentity ChannelBlends[TextureChannelCeiling] = {};
    ControlIdentity ChannelOps[TextureChannelCeiling]    = {};
    ControlIdentity MaskRows[4]                          = {};
    ControlIdentity SettingRows[4]                       = {};

    ControlIdentity MenuAdd    = {};
    ControlIdentity MenuLayer  = {};
    ControlIdentity MenuMask   = {};
    ControlIdentity MenuBlend  = {};
    ControlIdentity MenuIdentities[40] = {};             // [-] - the pooled menu item identities

    PlaneExtent MenuAnchorExtent = {};                   // [px] - where the open menu hangs from
    PlaneExtent RowRects[TextureLayerCeiling] = {};      // [-] - the last Record's rows
    std::uint32_t RowTally = 0u;                         // [-] - how many stood
};

} // namespace Slate
