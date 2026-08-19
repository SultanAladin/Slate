//============================================================================================================================================
//                                                          LAYERSTACKPANEL.H
//============================================================================================================================================
// 🧩 Records the texture-paint layer stack, the channel property panel and the mask property panel exactly as their references present them.

#pragma once

#include "Contract/DeliveryContract.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"
#include "SlateUI/Interface/LayerStackSpecification/Api/LayerStackSpecification.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE SEATED INKS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every ink `LayerstackV1` declares in its `:root`, named rather than repeated.
/// note  📐 The reference's own OLED-neutral token run. Each field carries the custom property it
///        transcribes, so the sheet can be checked line by line.
/// tag   contract, nonallocating, nonthrowing
struct LayerStackInk
{
    InkOrdinate  Ground        = Covering(0x000000u);        // [-] - --bg
    InkOrdinate  Panel         = Covering(0x050505u);        // [-] - --panel
    InkOrdinate  PanelRaised   = Covering(0x0A0A0Au);        // [-] - --panel-2
    InkOrdinate  Row           = Covering(0x0D0D0Du);        // [-] - --row
    InkOrdinate  RowHovered    = Covering(0x161616u);        // [-] - --row-h
    InkOrdinate  RowTaken      = Covering(0x202020u);        // [-] - --row-a
    InkOrdinate  Detail        = Covering(0x080808u);        // [-] - --detail
    InkOrdinate  Stroke        = Partial(0xFFFFFFu, 0.075);   // [-] - --stroke, .075 coverage
    InkOrdinate  StrokeStrong  = Partial(0xFFFFFFu, 0.18) ;   // [-] - --stroke-2, .18 coverage
    InkOrdinate  Primary       = Covering(0xEFEFEFu);        // [-] - --tx
    InkOrdinate  Secondary     = Covering(0x9A9A9Au);        // [-] - --tx-2
    InkOrdinate  Faint         = Covering(0x5E5E5Eu);        // [-] - --tx-3
    InkOrdinate  Accent        = Covering(0xFFFFFFu);        // [-] - --acc
    InkOrdinate  Danger        = Covering(0xFF6B63u);        // [-] - --danger
    InkOrdinate  Affirm        = Covering(0x59D499u);        // [-] - --ok
    InkOrdinate  Caution       = Covering(0xFFD24Au);        // [-] - --warn
};

/// 🧩 Every length `LayerstackV1` states, at the artist's own scale.
/// tag   contract, nonallocating, nonthrowing
struct LayerStackMetric
{
    float  PanelAlong       = 392.0f;   // [px] - --w, the panel's own extent
    float  HeadAcross       =  40.0f;   // [px] - .head, 11px over and 9px under an 20px run
    float  HeadPadAlong     =  14.0f;   // [px] - .head padding-left
    float  ToolsAcross      =  44.0f;   // [px] - .tools, 8px over and under a 28px field
    float  ToolsPadAlong    =  10.0f;   // [px] - .tools padding
    float  SearchAcross     =  28.0f;   // [px] - .search height
    float  RowAcross        =  45.0f;   // [px] - .row min-height
    float  MaskRowAcross    =  37.0f;   // [px] - .row.msk min-height
    float  RowPadAlong      =  10.0f;   // [px] - .row padding-left
    float  RowGapAlong      =   8.0f;   // [px] - .row gap
    float  RowStepAlong     =  27.0f;   // [px] - .kids margin-left 15 + padding-left 12
    float  MaskLeadAlong    =  26.0f;   // [px] - .attach padding-left
    float  TagAlong         =   3.0f;   // [px] - .tag width
    float  DiscloseAlong    =  14.0f;   // [px] - .tw width
    float  ActionExtent     =  23.0f;   // [px] - .ico.sm
    float  ButtonExtent     =  28.0f;   // [px] - .ico
    float  ThumbExtent      =  35.0f;   // [px] - .thumb
    float  ThumbMini        =  27.0f;   // [px] - .thumb.mini
    float  BadgeExtent      =  15.0f;   // [px] - .thumb .badge
    float  ColumnsLeast     = 580.0f;   // [px] - the extent at which `wide` seats the columns
    float  BlendColumnAlong = 118.0f;   // [px] - .col-blend width
    float  OpacityColumnAlong = 110.0f; // [px] - .col-op width
    float  OpacityReadAlong =  32.0f;   // [px] - .opn width
    float  ColumnGapAlong   =   7.0f;   // [px] - .col-op gap
    float  MiniAcross       =   4.0f;   // [px] - .mini height
    float  ChipAcross       =  18.0f;   // [px] - .chip height
    float  StackPadAlong    =   8.0f;   // [px] - .stack padding
    float  StackPadAcross   =   6.0f;   // [px] - .stack padding-top
    float  ScrollAlong      =  10.0f;   // [px] - .stack::-webkit-scrollbar width
    float  SectionAcross    =  30.0f;   // [px] - .sech, 8px over and under a 14px run
    float  CardPadAlong     =  12.0f;   // [px] - .cbody padding
    float  FieldAcross      =  26.0f;   // [px] - one folded property row
    float  FootAcross       =  96.0f;   // [px] - .foot, crumb over properties over the action bar
    float  FootPadAlong     =  10.0f;   // [px] - .foot padding
    float  RadiusStandard   =  10.0f;   // [px] - --r
    float  RadiusSmall      =   7.0f;   // [px] - --r-s
    float  RadiusPill       = 999.0f;   // [px] - --pill
    float  RunHead          =  11.5f;   // [px] - .head h1
    float  RunRow           =  12.5f;   // [px] - .name
    float  RunSub           =  10.5f;   // [px] - .sub
    float  RunFine          =  10.0f;   // [px] - .count, .chip
    float  RunSection       =   9.5f;   // [px] - .sech
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT THE HOST OWNS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Everything the stack retains between ticks, which the host owns and the panel amends.
/// tag   contract, nonallocating, nonthrowing
struct LayerStackOrdinates
{
    float          StackOffset   = 0.0f;    // [px] - how far the stack is scrolled
    float          StackSpan     = 0.0f;    // [px] - the recorded extent, resolved each tick
    std::uint32_t  Hovered       = 0xFFFFFFFFu;   // [-] - which entry the pointer is over
    bool           HoveredMask   = false;         // [-] - whether it is over that entry's mask row
    bool           ContactPrior  = false;         // [-] - the previous tick's contact, for edge detection
};

//------------------------------------------------------------------------------------------------------------------------
//                                                         THE PANEL
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Records the layer stack and its two property panels exactly as `LayerstackV1` presents them.
/// note  🔴 Every primitive is recorded through `RecordingSurface` in one order; the panel opens no vendor
///        window. The stack scrolls by offsetting its own cursor inside a confined extent, because the
///        recording seam carries a wheel reading but no scrolling primitive of its own.
/// tag   owning
class LayerStackPanel
{
public:

    LayerStackPanel()                                  = default;
    LayerStackPanel(const LayerStackPanel&)            = delete;
    LayerStackPanel& operator=(const LayerStackPanel&) = delete;
    ~LayerStackPanel()                                 = default;

    /// 🧩 Binds the panel to one recording surface for the ticks that follow.
    /// in    Recording  [-]  borrowed; must outlive the panel
    /// out   Deliver    [-]  refuses with CapabilityAbsent when the surface is absent
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    Deliver<bool> Construct(RecordingSurface& Recording);

    /// 🧩 Records the layer stack — header, tools, the nested rows and the footer.
    /// in    Extent       [-]  the pane's own extent
    /// in    Arrangement  [-]  borrowed for the tick; the panel amends what the artist takes
    /// in    Seated       [-]  retained between ticks
    /// cost  🔴
    /// tag   api, nonallocating, nonthrowing
    void RecordStack(const PlaneExtent& Extent, LayerArrangement& Arrangement,
                     LayerStackOrdinates& Seated);

    /// 🧩 Records the channel property panel for whichever entry stands taken.
    /// note  📐 Reached when the taken half is the entry itself — a material, paint, fill or decal.
    /// cost  🔴
    /// tag   api, nonallocating, nonthrowing
    void RecordChannelProperties(const PlaneExtent& Extent, const LayerArrangement& Arrangement);

    /// 🧩 Records the mask property panel for whichever entry stands taken.
    /// note  📐 Reached when the taken half is the attached mask.
    /// cost  🔴
    /// tag   api, nonallocating, nonthrowing
    void RecordMaskProperties(const PlaneExtent& Extent, const LayerArrangement& Arrangement);

    /// 🧩 Records the revision pane the inspector's second slide pairs with a property panel.
    /// cost  🚩
    /// tag   api, nonallocating, nonthrowing
    void RecordRevisions(const PlaneExtent& Extent);

    /// 🧩 The seated inks, so a host may state them in a proof.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    const LayerStackInk& Inked() const { return Tinted; }

    /// 🧩 The seated lengths.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    const LayerStackMetric& Measured() const { return Scaled; }

private:

    void RecordEntryRow(const PlaneExtent& Extent, const LayerArrangement& Arrangement,
                        std::uint32_t Ordinal, bool Taken, bool Hovered);

    void RecordMaskRow(const PlaneExtent& Extent, const LayerEntry& Entry, bool Taken, bool Hovered);

    void RecordSectionHead(const PlaneExtent& Extent, const char* Caption, const char* Reading,
                           bool Opened);

    float RecordReadingRow(const PlaneExtent& Extent, const char* Caption, const char* Reading);

    void RecordMeter(const PlaneExtent& Extent, std::uint32_t Reading, InkOrdinate Ink);

    void RecordChip(const PlaneExtent& Extent, const char* Caption, InkOrdinate Ink, bool Solid);

    RecordingSurface*  Surface = nullptr;   // [-] - borrowed
    LayerStackInk      Tinted;              // [-] - the seated inks
    LayerStackMetric   Scaled;              // [-] - the seated lengths
};

}   // namespace Slate
