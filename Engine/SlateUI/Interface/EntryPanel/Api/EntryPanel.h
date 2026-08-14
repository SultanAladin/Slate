//============================================================================================================================================
//                                                              ENTRYPANEL.H
//============================================================================================================================================
// 🧩 The control centre — every primitive in `ControlPanel` presented live under one accordion, each holding a reading the panel does not own.

#pragma once

#include "Contract/OutcomeContract.h"
#include "Contract/PrecisionContract.h"
#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/PanelIndex.h"
#include "SlateUI/Interface/WorkspaceSpace/Api/WorkspaceSpace.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE SECTIONS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The families the accordion divides `ControlPanel`'s surface into, one section apiece.
/// note  📝 Divided by what a control **does** and not by which translation unit declares it. An artist looking for
///        a switch does not know that `ControlChoice.cpp` holds it, and a panel organised by source layout is a
///        panel whose organisation is only legible to whoever wrote it.
/// tag   contract
enum class EntrySection : std::uint32_t
{
    Numeric   = 0u,   // [-] - the bounded slider, the unbounded scalars, the three components
    Choice    = 1u,   // [-] - the crossing nub, the exclusive pills, the segment row, the dropped list
    Text      = 2u,   // [-] - the free field and the path with its browse cap
    Colour    = 3u,   // [-] - the swatch bar and the four coordinate tracks it drops
    Chrome    = 4u,   // [-] - the menu pills and the glyph buttons
    Carousel  = 5u    // [-] - the two-pane slide, advanced by the declared interval
};

// 📝 The count is a constant beside the enumeration and never a `SectionCount` enumerator inside it. An enumerator
//    that is not a section is a case every switch over the enumeration has to refuse, and the one that forgets
//    presents a seventh accordion header with no controls under it.
inline constexpr std::uint32_t EntrySectionCount = 6u;   // [-] - sections the accordion presents

/// 🧩 The caption one section's header prints.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* CaptionOf(EntrySection Declared);

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE DECLARED EXTENTS
//------------------------------------------------------------------------------------------------------------------------

inline constexpr std::uint32_t EntryComponentCount = 3u;   // [-] - components one vector entry carries
inline constexpr std::uint32_t EntryBlendCount     = 4u;   // [-] - choices the exclusive pill run offers
inline constexpr std::uint32_t EntryChannelCount   = 4u;   // [-] - independent switches the segment row carries
inline constexpr std::uint32_t EntrySamplingCount  = 4u;   // [-] - choices the dropped list offers
inline constexpr std::uint32_t EntryPillCount      = 4u;   // [-] - pills the menu band carries
inline constexpr std::uint32_t EntryPaneCount      = 2u;   // [-] - panes the carousel slides between

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE ENTRY SPECIFICATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every reading the sections edit, and the presentation the artist left the panel in.
/// note  🔴 Held by value in whatever workspace declares the panel, exactly as `CanvasSpecification` and
///        `AssetSpecification` are. `14` §4.1 places what the artist chose to look at beside the document and not
///        inside it, so a host threading a pointer in would make closing a workspace a destructive edit.
/// note  🔴 Every reading here is the panel's own and names nothing in the document. This is the control centre:
///        it demonstrates and exercises `ControlPanel`, and a reading of it wired to a brush would make the same
///        control authoritative in two panels at once — which is the divergence `14` §1 forbids.
/// note  ⚠️ The two `TextCarry` fields and the `DropdownCarry` are carried here and never inside a control. Two
///        controls sharing one carry are two controls that open together, and two dropdowns doing it is the
///        defect `DropdownCarry`'s own note names.
/// tag   owning
struct EntrySpecification
{
    // what the numeric section edits ----------------------------------------------------------------
    double  Coverage      =  0.75;   // [-]  - a bounded reading, the slider's own span
    double  StrokeExtent  = 24.0;    // [px] - a bounded reading in pixels
    double  Hardness      =  0.50;   // [-]  - a bounded reading at two decimals
    double  Displacement  =  0.0;    // [px] - an unbounded reading, dragged by accumulated travel
    double  Turn          =  0.0;    // [°]  - an unbounded reading in degrees
    double  Placement[EntryComponentCount] = {};   // [px] - the three components of one vector entry

    // what the choice section edits -----------------------------------------------------------------
    bool           SmoothingEnabled                    = true;    // [-] - the crossing nub
    std::uint32_t  ChosenBlend                         = 0u;      // [-] - the exclusive pill run
    bool           ChannelDeclared[EntryChannelCount]  = { true, true, true, false };
                                                                  // [-] - the independent switches
    std::uint32_t  ChosenSampling                      = 0u;      // [-] - the dropped list
    DropdownCarry  SamplingCarry                       = {};      // [-] - its openness and anchor

    // what the text section edits -------------------------------------------------------------------
    TextCarry  Named           = {};      // [-] - the free field
    TextCarry  Located         = {};      // [-] - the path field
    bool       BrowseDeclared  = false;   // [-] - the browse cap was pressed this tick

    // what the colour section edits -----------------------------------------------------------------
    ThemeColour  Tint          = {};      // [-] - the swatch, resolved from the palette on the first tick
    bool         TintDeclared  = false;   // [-] - the tint has been resolved and is the artist's from here
    bool         PickerOpen    = false;   // [-] - the four coordinate tracks are dropped

    // what the chrome section edits -----------------------------------------------------------------
    std::uint32_t  ChosenPill   = 0u;   // [-] - which menu pill stands highlighted
    std::uint32_t  ChosenGlyph  = 0u;   // [-] - which glyph button stands highlighted

    // what the carousel section carries -------------------------------------------------------------
    CarouselCarry  Slide             = {};          // [-] - the presented pane, the arriving one, the travel
    float          DeclaredInterval  = 0.0166667f;  // [s] - the nominal step; a host with a clock writes it

    // the panel's own presentation ------------------------------------------------------------------
    bool           SectionOpen[EntrySectionCount] = { true, true, true, true, true, true };
                                                       // [-]  - which sections are unfolded
    float          VisibleOffset    = 0.0f;            // [px] - top of the presented span of the body
    std::uint32_t  PresentedTicks   = 0u;              // [-]  - ticks presented, for dropdown dismissal
    std::uint32_t  RefusedControls  = 0u;              // [-]  - controls that refused on the previous tick
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE SECTION EXTENT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The vertical extent one section's body occupies while it is open, its own header excluded.
/// in    Extents   [-]   read for every row pitch the section stacks
/// in    Standing  [-]   read because an open colour picker makes its section taller
/// in    Declared  [-]   the section
/// note  🔴 One place, so the measured content and the presented content cannot disagree. Measured separately from
///        what the walk lays out, the body's visible offset is bounded against an extent nothing presents — and the
///        artist meets that as a list that scrolls past its own last control or stops short of it.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
float SectionExtent(const LayoutExtents& Extents, const EntrySpecification& Standing, EntrySection Declared);

/// 🧩 The whole body's extent, every header and every open section together.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
float PresentedExtent(const LayoutExtents& Extents, const EntrySpecification& Standing);

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Presents one tick of the control centre into the rectangle the desk resolved.
/// in    Theme     [-]   read for the palette and every extent; never held
/// in    Area      [px]  the interior the desk resolved, honoured exactly
/// in    Standing  [-]   the specification, amended in place by every control that answers the pointer
/// post  the panel is painted onto the foreground recording and every artist amendment is written back
/// note  🔴 Nothing here opens a vendor window and nothing here reads the vendor pointer. Every quad leaves
///        through `ControlPanel`'s seam, which `14` §7 holds to exactly one component knowing which recording the
///        interface paints on.
/// note  ⚠️ A control that refuses for want of width is counted and presented in the footer, never raised. A
///        refusal per tick per narrow control would append to `86`'s register on the ordinary path of a panel
///        docked into a narrow column.
/// cost  🚩
/// tag   api, nonthrowing
void PresentEntries(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, EntrySpecification& Standing);

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PANEL ROUTINE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The presentation routine matching `PanelPresentRoutine`.
/// in    PresentContext  [-]  an `EntrySpecification*`; a null context presents an empty state and nothing else
/// note  🔴 Matches `PanelPresentRoutine` exactly, so a workspace declares it into `PanelIndex` and the desk never
///        learns what a control is.
/// cost  🚩
/// tag   api, nonthrowing
void PresentEntryPanel(const ThemeSpecification& Theme, const WorkspaceRectangle& Area, void* PresentContext);

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE LEDGER SLOT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Resolves a panel slot for the control centre.
/// in    Standing  [-]  the specification the slot will address for as long as the workspace stands
/// out   Slot      [-]  the resolved slot, ready for registration
/// note  🔴 The returned slot points at `Standing`. A slot built from a temporary is a dangling context, and it
///        presents as a panel painted against released storage.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
PanelSlot ResolveEntrySlot(const char*          PanelIdentifier,
                           const char*          PanelTitle,
                           WorkspacePanelSide   DeclaredSide,
                           EntrySpecification&  Standing);

// 📐 Ordinals, counts and section extents are Exact. Every reading a control drags and every rectangle the panel
//    is handed is Bounded. The component claims Bounded, per `00` §3's transitivity rule.
SLATE_DECLARES_PRECISION(PrecisionGuarantee::Bounded, PrecisionGuarantee::Bounded, PrecisionGuarantee::Exact);

}   // namespace Slate
