//============================================================================================================================================
//                                                             CANVASPANEL.H
//============================================================================================================================================
// 🧩 One viewport's tick — the band arithmetic, the three hand-rolled overlays, and the quads that frame a canvas nothing has drawn into yet.

#pragma once

#include "Contract/PrecisionContract.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/PanelIndex.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/WorkspaceSpace.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE CANVAS EXTENTS
//------------------------------------------------------------------------------------------------------------------------

inline constexpr std::uint32_t CanvasExtentCount = 4u;   // [-] - four declared image extents

/// 🧩 One offered canvas extent — resolution and caption.
struct CanvasExtent
{
    std::uint32_t  Width   = 0u;     // [px] - horizontal extent
    std::uint32_t  Height  = 0u;     // [px] - vertical extent
    const char*    Caption = "";     // [-]  - "1920 × 1080" et al
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE CANVAS PROJECTION
//------------------------------------------------------------------------------------------------------------------------

enum class CanvasProjection : std::uint8_t
{
    Perspective,
    Orthographic
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE OVERLAY RECORD
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What one overlay remembers between ticks — its open state, where it opened, and the tick it opened on.
struct CanvasOverlayRecord
{
    bool           OverlayOpen = false;   // [-]  - the overlay is standing
    float          AnchorX     = 0.0f;    // [px] - where it opened
    float          AnchorY     = 0.0f;    // [px]
    std::uint32_t  OpenedTick  = 0u;      // [-]  - which tick opened it
};

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE CANVAS SPECIFICATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Everything the viewport panel carries — declarations, overlays, and readout toggles.
/// note  📝 🔴 Held by value and not by pointer. The viewport's declarations are layout and never document, so `14` §4.1 keeps
///        them beside the document — a host threading in a pointer would be a host deciding what the canvas resolves into.
struct CanvasSpecification
{
    // the three overlay records ---------------------------------------------------------------------
    CanvasOverlayRecord  HeaderOverlay = {};   // [-] - the header band's dropdown
    CanvasOverlayRecord  CanvasOverlay = {};   // [-] - the canvas area's own
    CanvasOverlayRecord  FooterOverlay = {};   // [-] - the footer band's dropdown

    // the canvas declarations ------------------------------------------------------------------------
    CanvasProjection     Projection         = CanvasProjection::Perspective;   // [-] - perspective or orthographic
    std::uint32_t        ExtentOrdinal      = 1u;                              // [-] - which extent is declared (1920×1080 default)
    bool                 LatticeStanding    = true;                            // [-] - the lattice overlay is shown
    bool                 AxisStanding       = false;                           // [-] - the axis reference is shown
    bool                 WireframeStanding  = false;                           // [-] - wireframe is shown

    // the footer readouts ----------------------------------------------------------------------------
    bool                 ExtentReadout        = true;    // [-] - extent is shown in footer
    bool                 MagnificationReadout = false;   // [-] - magnification is shown in footer
    bool                 OffsetReadout        = false;   // [-] - offset is shown in footer
    bool                 RotationReadout      = false;   // [-] - rotation is shown in footer

    // the current view state (read-only for now) ----------------------------------------------------
    float                Magnification  = 100.0f;   // [%]  - current zoom level
    float                OffsetAlong    = 0.0f;     // [px] - pan offset horizontal
    float                OffsetAcross   = 0.0f;     // [px] - pan offset vertical
    float                RotationYaw    = 0.0f;     // [°]  - rotation around Y axis
    float                RotationPitch  = 0.0f;     // [°]  - rotation around X axis
    float                RotationRoll   = 0.0f;     // [°]  - rotation around Z axis

    // presentation state -----------------------------------------------------------------------------
    std::uint32_t        PresentedTicks = 0u;       // [-] - ticks presented, for overlay dismissal
};

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE BAND ARITHMETIC
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The three resolved bands — header, canvas area, and footer.
struct CanvasBands
{
    WorkspaceRectangle  HeaderBand = {};   // [px] - the header band
    WorkspaceRectangle  CanvasArea = {};   // [px] - the canvas rendering area
    WorkspaceRectangle  FooterBand = {};   // [px] - the footer band
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE OFFERED EXTENTS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Resolves a canvas extent from its ordinal.
/// in    ExtentOrdinal [-]  which extent to resolve (0-3)
/// out   Extent        [-]  the resolved extent, or refusal if ordinal is out of range
/// note  📝 The one place an image extent literal is permitted in this component.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
Outcome<CanvasExtent> ResolveCanvasExtent(std::uint32_t ExtentOrdinal);

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE BAND ARITHMETIC
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Resolves the three bands from a body rectangle.
/// in    Extents  [-]  layout extents from theme
/// in    Body     [px] the panel's body rectangle
/// out   Bands    [px] the three resolved bands
/// note  📝 The header takes its declared height, footer takes its declared height, canvas takes the remainder.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
CanvasBands ResolveCanvasBands(const LayoutExtents& Extents, WorkspaceRectangle Body);

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Presents one tick of the canvas panel into the rectangle the desk resolved.
/// in    Theme  [-]   read for the palette and the band extents; never held
/// in    Body   [px]  the interior the desk resolved for this panel, honoured exactly
/// in    Canvas [-]   the canvas specification, mutated for overlay state
/// post  the canvas panel is painted into the foreground list
/// note  🔴 The three overlays are hand-rolled rather than using vendor widgets.
/// cost  🚩
/// tag   api, nonthrowing
void PresentCanvas(const ThemeSpecification& Theme, WorkspaceRectangle Body, CanvasSpecification& Canvas);

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE PANEL ROUTINE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The presentation routine matching `PanelPresentRoutine`.
/// in    Theme          [-]   read for the palette and the band extents; never held
/// in    Area           [px]  the interior the desk resolved for this panel, honoured exactly
/// in    PresentContext [-]   a `CanvasSpecification*`; a null context presents nothing
/// post  the canvas panel is painted
/// note  🔴 Matches `PanelPresentRoutine` exactly, so a workspace declares it into `PanelIndex` and the desk
///        never learns what a canvas specification is.
/// cost  🚩
/// tag   api, nonthrowing
void PresentCanvasPanel(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, void* PresentContext);

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE LEDGER SLOT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Resolves a panel slot for the canvas panel.
/// in    PanelIdentifier [-]  the panel's identifier string
/// in    PanelTitle      [-]  the panel's title string
/// in    Canvas          [-]  reference to the canvas specification
/// out   Slot            [-]  the resolved panel slot, ready for ledger registration
/// note  🔴 The returned slot points at the Canvas reference. Registering a slot built from a temporary
///        is a dangling context.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
PanelSlot ResolveCanvasSlot(const char* PanelIdentifier, const char* PanelTitle, CanvasSpecification& Canvas);

// 📐 Ordinals, counts and slot positions are Exact integers. Every rectangle the panel is handed is Bounded and
//    none of them is derived here. The component claims Bounded, per `00` §3's transitivity rule.
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded, PrecisionGuarantee::Exact);

}   // namespace Slate
