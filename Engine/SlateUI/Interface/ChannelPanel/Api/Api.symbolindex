//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 One material's twenty channels made visible — every row read from `MaterialIndex`, nothing held here.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateUI/Interface/ChannelPanel/Api
%layer      SlateUI
%sources    1
%symbols    10
%annotated  7/10
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ChannelPanel.h | 123 lines | 86b8f9f0 | 10 sym | One material's twenty channels made visible — every row read from `MaterialIndex`, nothing held here.

//------------------------------------------------------------------------------------------------------------------------
//                                                 WHAT THE PANEL CARRIES
//------------------------------------------------------------------------------------------------------------------------

V ChannelNoticeExtent      | ChannelPanel.h | 25    | -                             | -  | ?
    by    Source/ChannelPanel.cpp

V ChannelSectionCount      | ChannelPanel.h | 28    | -                             | -  | ?
    by    Source/ChannelPanel.cpp

T ChannelPanelCarry        | ChannelPanel.h | 38-49 | owning                        | -  | What the panel carries between ticks — presentation only, and the caller owns all of it. the open picker and the notice are layout, and layout inside the document would make opening a section an undoable edit. It is not a copy of anything the material holds — a refused write leaves the material exactly as it was, so there is nothing here that the document could drift from.
    has   VisibleOffset   float                      [-]  ?
    has   SectionOpen     bool[ChannelSectionCount]  [-]  ?
    has   SelectionCarry  DropdownCarry              [-]  ?
    has   Notice          char[ChannelNoticeExtent]  [-]  ?
    has   NoticeDeclared  bool                       [-]  ?
    has   PresentedTicks  std::uint32_t              [-]  ?
    by    Source/ChannelPanel.cpp
    note  🔴 `14` §4.1 and `42` §5: nothing here is a channel, a selection or a threshold. The folds, the offset,
    note  ⚠️ The notice is the text of the **last refusal**, kept so the artist reads why a write did not land.

T ChannelPanelContext      | ChannelPanel.h | 59-64 | nonallocating,nonthrowing     | -  | What the panel presents against — the ledger it reads and the carry it writes. ordinal against the ledger every tick, so a material withdrawn beneath it presents as absent rather than as a stale copy of channels that no longer exist. amended through a call the ledger offers, the row presents the reading and refuses the edit — it does not hold a local value and write it back later.
    has   Materials        MaterialIndex*      [-]  ?
    has   MaterialOrdinal  std::uint32_t       [-]  ?
    has   Carry            ChannelPanelCarry*  [-]  ?
    by    Source/ChannelPanel.cpp
    note  🔴 `14` §1's gate: the panel stores neither, and it does not store the material either. It resolves the
    note  ⚠️ `MaterialSpecification`'s API is presented **against** and never copied. Where a channel cannot be

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT A ROW READS
//------------------------------------------------------------------------------------------------------------------------

F CaptionOfChannel         | ChannelPanel.h | 76    | api,nonallocating,nonthrowing | ✔️ | The caption one channel is presented under. a report class. `18` §2's order is the presented order, so the artist reads the channels in the order the document declares them.
    in    Channel  ChannelSubject  [-]  ?
    out   -        const char*     [-]  ?
    by    Source/ChannelPanel.cpp
    note  🔴 Read from the declared subject and never derived from the value — the same rule `86` §4.1 applies to

F CaptionOfReflectance     | ChannelPanel.h | 81    | api,nonallocating,nonthrowing | ✔️ | The caption one reflectance selection is presented under.
    in    Selected  ReflectanceSelection  [-]  ?
    out   -         const char*           [-]  ?
    by    Source/ChannelPanel.cpp

F CaptionOfChannelSource   | ChannelPanel.h | 89    | api,nonallocating,nonthrowing | ✔️ | The caption one channel source is presented under. one is where a channel's value comes from and the other is what a layer's content is — and one name over both would read as a shared vocabulary they do not have.
    in    Source  ChannelSource  [-]  ?
    out   -       const char*    [-]  ?
    by    Source/ChannelPanel.cpp
    note  ⚠️ Spelled apart from `LayerPanel`'s `CaptionOfSource` deliberately. The two enumerations are unrelated —

F CaptionOfChannelMeasure  | ChannelPanel.h | 94    | api,nonallocating,nonthrowing | ✔️ | The caption one channel measure is presented under.
    in    Measured  ChannelMeasure  [-]  ?
    out   -         const char*     [-]  ?
    by    Source/ChannelPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

F PresentChannelPanel      | ChannelPanel.h | 114   | api,nonthrowing               | 🚩 | Presents one tick of one material's channels into the rectangle the desk resolved for it. learns what a channel is. is presented in its own section rather than hidden. Hiding it is what makes an artist believe that switching a selection destroyed their work, and they then avoid switching at all. `ChannelConverted`. Neither is re-derived here — `18` §9's inventory and `36` §4's declaration are the material's answers, and a panel that recomputed them could disagree with the dispatch that reads them.
    in    Theme           const ThemeSpecification&  [-]   read by const reference; no colour or extent is spelled in this component
    in    Area            const WorkspaceRectangle&  [px]  the interior the panel layer handed it, header band included
    in    PresentContext  void*                      [-]   a `ChannelPanelContext*`; a null context or an unresolved ordinal presents empty
    out   -               void                       [-]   ?
    by    Source/ChannelPanel.cpp
    note  🔴 Matches `PanelPresentRoutine` exactly so a workspace declares it into `PanelIndex` and the desk never
    note  🔴 `42` §5: a channel declared for a reflectance the material no longer selects is **retained**, and it
    note  ⚠️ Whether a channel is sampled is read from `ChannelSampled` and whether it is converted from

F SLATE_DECLARES_PRECISION | ChannelPanel.h | 121   | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Exact    PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChartPartition.h, (+50 more)
