//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 `Inspector.tsx`'s revision timeline over the real sequence — bubble column, spine, card, fold.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateUI/Interface/RevisionPanel/Source
%layer      SlateUI
%sources    1
%symbols    19
%annotated  0/19
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S RevisionPanel.cpp | 538 lines | ec3682a8 | 19 sym | `Inspector.tsx`'s revision timeline over the real sequence — bubble column, spine, card, fold.

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE REFERENCE GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

V BubbleColumnWidth    | RevisionPanel.cpp | 26      | - | - | ?

V BubbleEdge           | RevisionPanel.cpp | 27      | - | - | ?

V SpineWidth           | RevisionPanel.cpp | 28      | - | - | ?
    by    Source/LayerPanel.cpp

V RailWidth            | RevisionPanel.cpp | 29      | - | - | ?

V NodeEdge             | RevisionPanel.cpp | 30      | - | - | ?

V NodeShadowReach      | RevisionPanel.cpp | 31      | - | - | ?

V FoldRowHeight        | RevisionPanel.cpp | 32      | - | - | ?
    by    Source/LayerPanel.cpp

V TwistyEdge           | RevisionPanel.cpp | 33      | - | - | ?

F FoldStanding         | RevisionPanel.cpp | 37-40   | - | - | ?
    in    Carry     const RevisionPanelCarry&  [-]  ?
    in    Position  std::size_t                [-]  ?
    out   -         bool                       [-]  ?
    by    Source/LayerPanel.cpp

F Inset                | RevisionPanel.cpp | 42-52   | - | - | ?
    in    Area   const WorkspaceRectangle&  [-]  ?
    in    Reach  float                      [-]  ?
    out   -      WorkspaceRectangle         [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlLayout.cpp

F PresentedDescription | RevisionPanel.cpp | 57-65   | - | - | ?
    in    Standing          const CommittedTransaction&  [-]  ?
    in    FallbackDeclared  bool&                        [-]  ?
    out   -                 const char*                  [-]  ?

F PresentedStamp       | RevisionPanel.cpp | 69-75   | - | - | ?
    in    SealedAt  std::uint64_t  [-]  ?
    in    Printed   char*          [-]  ?
    in    Extent    std::size_t    [-]  ?
    out   -         void           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE DESTRUCTIVE FACT
//------------------------------------------------------------------------------------------------------------------------

F DiscardCountStanding | RevisionPanel.cpp | 83-91   | - | - | ?
    in    Sequence  const RevisionSequence&  [-]  ?
    out   -         std::uint64_t            [-]  ?
    by    Api/RevisionPanel.h

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SCRUBBING
//------------------------------------------------------------------------------------------------------------------------

F ScrubToPosition      | RevisionPanel.cpp | 97-118  | - | - | ?
    in    Sequence  RevisionSequence&  [-]  ?
    in    Arriving  std::uint64_t      [-]  ?
    out   -         Outcome<bool>      [-]  ?
    by    Api/RevisionPanel.h

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE HEADER BAND
//------------------------------------------------------------------------------------------------------------------------

F PresentHeaderBand    | RevisionPanel.cpp | 127-171 | - | - | ?
    in    Theme     const ThemeSpecification&  [-]  ?
    in    Area      const WorkspaceRectangle&  [-]  ?
    in    Sequence  const RevisionSequence&    [-]  ?
    out   -         void                       [-]  ?
    by    Source/ChannelPanel.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                THE DISCARD CONFIRMATION
//------------------------------------------------------------------------------------------------------------------------

F PresentDiscardBand   | RevisionPanel.cpp | 180-260 | - | - | ?
    in    Theme      const ThemeSpecification&  [-]  ?
    in    Area       const WorkspaceRectangle&  [-]  ?
    in    Sequence   const RevisionSequence&    [-]  ?
    in    Carry      RevisionPanelCarry&        [-]  ?
    in    Travelled  float&                     [-]  ?
    out   -          void                       [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        ONE ROW
//------------------------------------------------------------------------------------------------------------------------

F RowExtentOf          | RevisionPanel.cpp | 266-272 | - | - | ?
    in    Extents   const LayoutExtents&  [-]  ?
    in    FoldOpen  bool                  [-]  ?
    out   -         float                 [-]  ?

F PresentRow           | RevisionPanel.cpp | 274-426 | - | - | ?
    in    Theme     const ThemeSpecification&    [-]  ?
    in    Area      const WorkspaceRectangle&    [-]  ?
    in    Standing  const CommittedTransaction&  [-]  ?
    in    Position  std::size_t                  [-]  ?
    in    Applied   bool                         [-]  ?
    in    Saved     bool                         [-]  ?
    in    Carry     RevisionPanelCarry&          [-]  ?
    in    Sequence  RevisionSequence&            [-]  ?
    out   -         void                         [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

F PresentRevisionPanel | RevisionPanel.cpp | 434-536 | - | - | ?
    in    Theme           const ThemeSpecification&  [-]  ?
    in    Area            const WorkspaceRectangle&  [-]  ?
    in    PresentContext  void*                      [-]  ?
    out   -               void                       [-]  ?
    by    Api/RevisionPanel.h
