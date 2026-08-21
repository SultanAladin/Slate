//============================================================================================================================================
//                                                       SCENEDIRECTORYPANEL.H
//============================================================================================================================================
// 🧩 The editor's scene directory — the content drawn INSIDE the editor's workspace
//    leaves, never over the whole display.
//
//    This is the editor twin of the validation shell's scene-directory strip. The
//    difference is where the content lives: the shell records its own fullscreen
//    rail + viewport + inspector over the display (it is a PROTOTYPE of the whole
//    reference sheet), while this panel records only leaf content — the sky in a
//    viewport leaf, the outliner | details in an outliner leaf, and the
//    properties | history in a properties leaf — inside whatever partition the
//    artist has built with the workspace machinery.
//
//    🔴 FUTURE AGENTS, READ THIS. `GlobalShellPanel` is the ValidationHost
//       prototype and is NOT to be recorded by the editor host. The editor's
//       layout is: workspace windows → splittable panels (EditorPanel +
//       PanelStructure) → leaf content. This panel is where editor content is
//       built. Do not port the shell's rail, layer stack, or fullscreen
//       inspector into the editor; the validation host keeps them, its viewport
//       stays black, and the editor never sees them.

#pragma once

#include "Contract/DeliveryContract.h"
#include "SlateUI/Interface/ComponentSpecification/Api/ComponentSpecification.h"
#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"
#include "Shared/OverlayGeometry.slang.h"
#include "SlateUI/Interface/EditorPanel/Api/EditorPanel.h"
#include "SlateUI/Interface/InteractionIndex/Api/InteractionIndex.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"
#include "SlateUI/Interface/MotionIntegrator/Api/MotionIntegrator.h"
#include "SlateUI/Interface/SceneDirectoryPanel/Api/SceneDirectoryContract.h"
#include "SlateUI/Interface/SymbolSpecification/Api/SymbolSpecification.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   WHAT THE HOST OWNS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every datum the scene-directory panel presents, owned by the host and written through by the panel.
/// note  🔴 `14` §1: the panel presents what it is handed and retains none of it. Every condition the artist
///        can alter lives here, so the host — and only the host — is the home of the scene directory's
///        content. This is the editor twin of the shell's `ShellContext`, holding only what the editor's
///        leaves present.
/// tag   contract
struct SceneDirectoryContext
{
    static constexpr std::uint32_t EntityCeiling = 16u;   // [-] - outline rows, the reference declares fourteen
    static constexpr std::uint32_t CardCeiling   =  4u;   // [-] - property cards, the reference states four

    // 📐 The editor's environment. `EnvironmentPresented` gates every environment branch, so a host that
    //    never sets it (the validation host) renders no environment anywhere.
    bool                       EnvironmentPresented = false;   // [-] - the sun/sky/atmosphere is presented
    EnvironmentConfiguration   Environment          = {};      // [-] - host-owned; the sliders write it
    RevisionDemand             RevisionDemandSlot   = {};      // [-] - one drag-end history demand

    std::uint32_t              EntityTaken = 2u;              // [-] - which outline row is taken (the Sun)

    // 📝 The GPU sky texture the viewport leaf draws, uploaded by the host. Opaque on purpose: the panel
    //    names no vendor, so the identity is an integer the recording surface resolves.
    std::uintptr_t             SkyTextureIdentity = 0u;       // [-] - zero draws no sky at all

    // 📝 The sky's own camera, declared by the host each tick it regenerates. The dome is
    //    direction-indexed, and the viewport leaf crops it to this camera's field of view.
    SkyViewCamera              ViewportSkyCamera  = {};       // [-] - the dome crop the viewport draws

    // 📐 The Properties | History strip, and the folds the reference applies empty so every card and
    //    every revision group arrives disclosed.
    std::uint32_t              InspectorTab    = 0u;          // [-] - 0 Properties, 1 History
    bool                       CardFolded[CardCeiling]       = {};
    bool                       RevisionFolded[EntityCeiling] = {};

    // 📐 The OUTLINER leaf's own pages: 0 the directory (outliner | details), 1 the selected record's
    //    properties, 2 its history. Tab cycles them; the Inspect button in the outliner header jumps
    //    to 1. `OutlineInspectorTab` is the properties leaf's strip selection INSIDE the outliner, so
    //    the two leaves never fight over one tab state.
    std::uint32_t              OutlinePage        = 0u;      // [-] - 0 Directory, 1 Properties, 2 History
    std::uint32_t              OutlineInspectorTab = 0u;     // [-] - 0 Properties, 1 History (page 1/2)

    // 📝 The disclosure and presence conditions of the outline rows, exactly as the shell's own declare
    //    them: the level, Lighting, Environment and Systems arrive expanded and everything else folded.
    bool  EntityExpanded[EntityCeiling] = { true, true, false, false, false, false,
                                            true, false, false, false, true, false, false, false };
    bool  EntityPresent[EntityCeiling]  = { true, true, true, true, true, true,
                                            true, true, true, true, true, true, true, true };

    // 📐 The detail pane's small option switches for the taken row: bit 0 Locked, bit 1 Cast Shadows.
    //    `Visible` is `EntityPresent`, which the eye already owns. For the CAMERA row the bits read
    //    differently: bit 1 is the camera lag, bit 2 the inverted pitch — the settings the camera's
    //    details pane presents.
    std::uint32_t              DetailBits[EntityCeiling] = {};

    // 📝 The camera's own ordinates, owned by the host and written every tick: the fly speed the
    //    properties leaf's Fly Speed card edits (with a drag-end history demand, like the
    //    environment), and the pose the details pane states.
    double                     CameraSpeed = 50.0;      // [m/s] - the fly camera's rate
    double                     CameraPosition[3] = { 0.0, 1.5, 0.0 };   // [m] - host-written
    double                     CameraRotation[3] = { 100.0, 15.0, 0.0 }; // [deg] - yaw, pitch, roll

    // 📝 The viewport's overlay record — the grid and the gizmo, filled by the panel and drawn by
    //    the GPU overlay pass. The host uploads it when its generation changes; the pass draws it
    //    in its own straight-alpha pass so the CPU never tessellates and the colours stay vivid.
    OverlayGeometry            Overlay = {};             // [-] - the GPU pass's input
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE PANEL
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Records the scene directory's leaf content — the viewport sky, the outliner | details column and the
///    properties | history pages — inside the extents the editor's panel chrome hands over.
/// note  🔴 The panel draws ONLY leaf content. It never draws a rail, a top bar or a fullscreen shell; the
///        workspace and the panel chrome belong to `WorkspacePanel` and `EditorPanel`, and this panel fills
///        the leaves they leave.
/// tag   owning
class SceneDirectoryPanel
{
public:

    /// 🧩 Exactly how many control identities `Construct` claims, stated where they are claimed.
    /// note  🔴 The arithmetic lives beside the registrations it describes so the two can only disagree by
    ///        an edit that touches both.
    static constexpr std::uint32_t RegistrationDemand =
          SceneDirectoryContext::EntityCeiling * 3u   // [-] - contact, disclosure and presence per outline row
        + SceneDirectoryContext::EntityCeiling * 3u   // [-] - three detail option rows per outline row
        + SceneDirectoryContext::CardCeiling          // [-] - one fold per property card
        + SceneDirectoryContext::EntityCeiling        // [-] - one fold per grouped revision header
        + 1u                                          // [-] - the Properties | History strip
        + 2u                                          // [-] - the outliner's page strip and the Inspect call
        + 6u;                                         // [-] - the six environment slider rows

    SceneDirectoryPanel()                                   = default;
    SceneDirectoryPanel(const SceneDirectoryPanel&)            = delete;
    SceneDirectoryPanel& operator=(const SceneDirectoryPanel&) = delete;
    ~SceneDirectoryPanel()                                  = default;

    /// 🧩 Borrows the recording facilities and registers every identity and interpolant the panel needs.
    /// out   Result  [-]  refuses with ContentUnsupported when a construction already stands, and with
    ///                     ExtentExhausted when the ledger or the integrator declines an registration
    /// cost  🚩
    /// tag   api, nonthrowing
    Outcome<bool> Construct(InteractionIndex&              Interaction,
                            MotionIntegrator&              Integrator,
                            RecordingSurface&              Surface,
                            const ThemeProfile& Resolved);

    /// 🧩 Samples the contact for this tick, after the tick owner has advanced the shared ledger once.
    /// note  🔴 This does not advance the ledger; several panels share it and the tick owner advances it once.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Advance(const PointerCondition& Sampled, double Elapsed,
                 SceneDirectoryContext& Applied, bool TabPressed = false);

    /// 🧩 Re-applies every scaled extent after the appearance was resolved against a new display extent.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void Reapply(const ThemeProfile& Resolved);

    /// 🧩 Returns the panel to its unconstructed condition.
    /// cost  ✔️
    /// tag   api, nonthrowing
    void Reset();

    /// 🧩 Records the uploaded sky dome across one viewport leaf, cropped to the camera's field of view.
    /// in    Extent   [px]  the leaf body the sky fills
    /// in    Applied  [-]   the environment and the sky identity; written through only for the crop
    /// note  🔴 The identity is the host's; a zero identity records nothing and the leaf's own ground shows.
    /// cost  🚩
    /// tag   api, nonallocating, nonthrowing
    void RecordViewportSky(const PlaneExtent& Extent, const SceneDirectoryContext& Applied);

    /// 🧩 Records the world's ground lattice across one viewport leaf — the same pinhole the sky mesh
    ///    uses, so the two align — giving the fly camera something to travel past.
    /// in    Extent   [px]  the leaf body the lattice is projected into
    /// in    Applied  [-]   the camera's pose and position, as the host wrote them this tick
    /// cost  🚩
    /// tag   api, nonallocating, nonthrowing
    void RecordGroundGrid(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                          const EditorPanelConfiguration& Configuration, OverlayGeometry& Overlay);

    /// 🧩 Records the world-origin translation gizmo — the three vivid axis arrows and the centre
    ///    handle — into the overlay geometry, projected through the same pinhole as the grid.
    /// in    Extent   [px]  the leaf body the gizmo is projected into
    /// in    Applied  [-]   the camera's pose and position
    /// in    Overlay  [-]   the overlay record the gizmo is written into; the host owns one per
    ///                      viewport leaf, so the pass can clip each leaf's geometry to its own box
    /// note  🔴 The gizmo colours are FULL-OPACITY straight alpha — the whole reason the overlay has
    ///        its own GPU pass is that the interface's premultiplied blend washed them out.
    /// cost  🚩
    /// tag   api, nonallocating, nonthrowing
    void RecordGizmo(const PlaneExtent& Extent, SceneDirectoryContext& Applied, OverlayGeometry& Overlay);

    /// 🧩 Records the outliner column and its details pane across one outliner leaf.
    /// in    Rows   [-]  the entity rows, borrowed for the tick
    /// in    RowCount [-]  how many of them stand
    /// cost  🚩
    /// tag   api, nonallocating, nonthrowing
    void RecordOutliner(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                        const EntityRow* Rows, std::uint32_t RowCount,
                        const EntityRevision* Revisions, std::uint32_t RevisionCount);

    /// 🧩 Records the Properties | History pages across one properties leaf.
    /// in    Rows        [-]  the entity rows, borrowed for the tick
    /// in    RowCount    [-]  how many of them stand
    /// in    Revisions   [-]  the host's revision run, borrowed for the tick
    /// in    RevisionCount [-]  how many of them stand
    /// cost  🚩
    /// tag   api, nonallocating, nonthrowing
    void RecordProperties(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                          const EntityRow* Rows, std::uint32_t RowCount,
                          const EntityRevision* Revisions, std::uint32_t RevisionCount,
                          std::uint32_t& InspectorTab);

private:

    void RecordLeafHeader(const PlaneExtent& Extent, SymbolSubject Glyph, const ThemeToken& Hue,
                          const char* Titled, const char* Secondary);
    void RecordDetailOptions(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                             std::uint32_t Ordinal, const EntityRow& Current);
    void RecordPropertyCards(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                             const EntityRow* Rows, std::uint32_t RowCount);
    void RecordEnvironmentCard(SceneDirectoryContext& Applied,
                               const PlaneExtent& Extent, float& Sweep, std::uint32_t& CardOrdinal,
                               const char* Caption,
                               const char* const* SliderCaptions,
                               const char* const* UnitGlyphs,
                               const double* Minimums, const double* Maximums,
                               double* Values, std::uint32_t SliderCount);
    void RecordRevisionSpine(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                             const EntityRow* Rows, std::uint32_t RowCount,
                             const EntityRevision* Revisions, std::uint32_t RevisionCount);

    InteractionIndex*           Ledger = nullptr;        // [-] - borrowed; never owned
    MotionIntegrator*           Motion = nullptr;        // [-] - borrowed; never owned
    RecordingSurface*           Surface = nullptr;       // [-] - borrowed; never owned
    const ThemeProfile*         Appearance = nullptr;    // [-] - borrowed; never owned
    ShellColour                 Tinted = {};             // [-] - the shell's own colour record
    ShellMetric                 Scaled = {};             // [-] - re-applied on every appearance resolve

    ControlPanel                Controls = {};           // [-] - tab strips, revision rows, fold animation
    ComponentSpecification      EnvironmentControls = {};   // [-] - the environment slider rows

    PointerCondition            Sampled = {};            // [-] - this tick's contact

    ControlIdentity RowContacts[SceneDirectoryContext::EntityCeiling]    = {};
    ControlIdentity RowDisclosures[SceneDirectoryContext::EntityCeiling] = {};
    ControlIdentity RowPresences[SceneDirectoryContext::EntityCeiling]   = {};
    ControlIdentity DetailOptions[SceneDirectoryContext::EntityCeiling][3] = {};
    ControlIdentity CardFolds[SceneDirectoryContext::CardCeiling]        = {};
    ControlIdentity RevisionGroups[SceneDirectoryContext::EntityCeiling] = {};
    ControlIdentity InspectorStrip = {};
    ControlIdentity OutlineStrip    = {};
    ControlIdentity InspectCall     = {};
    ControlIdentity EnvironmentSliders[6] = {};

    bool   EnvironmentArmed[6] = {};   // [-] - drag-start latched per slider
    double EnvironmentFrom[6]  = {};   // [-] - the value at drag start
};

} // namespace Slate
