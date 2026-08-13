//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The twenty channels as three sections — consumed, retained and undeclared — over an API that already exists.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateUI/Interface/ChannelPanel/Source
%layer      SlateUI
%sources    1
%symbols    25
%annotated  1/25
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ChannelPanel.cpp | 646 lines | 02e1429a | 25 sym | The twenty channels as three sections — consumed, retained and undeclared — over an API that already exists.

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE REFERENCE GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

V ChannelRowHeight        | ChannelPanel.cpp | 25      | - | - | ?

V NoticeBandHeight        | ChannelPanel.cpp | 26      | - | - | ?
    by    Source/PropertyPanel.cpp

V BadgeInset              | ChannelPanel.cpp | 27      | - | - | ?
    by    Source/PropertyPanel.cpp

V RowTextExtent           | ChannelPanel.cpp | 29      | - | - | ?
    by    Source/LayerPanel.cpp, Source/PropertyPanel.cpp

V SectionConsumed         | ChannelPanel.cpp | 35      | - | - | ?

V SectionRetained         | ChannelPanel.cpp | 36      | - | - | ?

V SectionUndeclared       | ChannelPanel.cpp | 37      | - | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     SMALL GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

F BandOf                  | ChannelPanel.cpp | 43-46   | - | - | ?
    in    Area    const WorkspaceRectangle&  [-]  ?
    in    Offset  float                      [-]  ?
    in    Height  float                      [-]  ?
    out   -       WorkspaceRectangle         [-]  ?
    by    Source/LayerPanel.cpp, Source/PropertyPanel.cpp

F InsetBy                 | ChannelPanel.cpp | 48-54   | - | - | ?
    in    Area    const WorkspaceRectangle&  [-]  ?
    in    Margin  float                      [-]  ?
    out   -       WorkspaceRectangle         [-]  ?
    by    Source/LayerPanel.cpp, Source/PropertyPanel.cpp

F LeftOf                  | ChannelPanel.cpp | 56-59   | - | - | ?
    in    Area   const WorkspaceRectangle&  [-]  ?
    in    Width  float                      [-]  ?
    out   -      WorkspaceRectangle         [-]  ?
    by    Source/LayerPanel.cpp, Source/PropertyPanel.cpp

F RightOf                 | ChannelPanel.cpp | 61-66   | - | - | ?
    in    Area   const WorkspaceRectangle&  [-]  ?
    in    Width  float                      [-]  ?
    out   -      WorkspaceRectangle         [-]  ?
    by    Source/LayerPanel.cpp, Source/PropertyPanel.cpp

F SquareCentred           | ChannelPanel.cpp | 68-74   | - | - | ?
    in    Area  const WorkspaceRectangle&  [-]  ?
    in    Edge  float                      [-]  ?
    out   -     WorkspaceRectangle         [-]  ?
    by    Source/LayerPanel.cpp, Source/PropertyPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE NOTICE
//------------------------------------------------------------------------------------------------------------------------

F RecordNotice            | ChannelPanel.cpp | 83-91   | - | - | ?
    in    Carry   ChannelPanelCarry&  [-]  ?
    in    Reason  const char*         [-]  ?
    out   -       void                [-]  ?
    by    Source/PropertyPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE BANDS
//------------------------------------------------------------------------------------------------------------------------

F PresentHeaderBand       | ChannelPanel.cpp | 97-131  | - | - | ?
    in    Theme         const ThemeSpecification&  [-]  ?
    in    Band          const WorkspaceRectangle&  [-]  ?
    in    MaterialName  const char*                [-]  ?
    in    Selected      ReflectanceSelection       [-]  ?
    out   -             void                       [-]  ?
    by    Source/LayerPanel.cpp, Source/PropertyPanel.cpp, Source/RevisionPanel.cpp

F PresentNoticeBand       | ChannelPanel.cpp | 133-158 | - | - | ?
    in    Theme  const ThemeSpecification&  [-]  ?
    in    Band   const WorkspaceRectangle&  [-]  ?
    in    Carry  ChannelPanelCarry&         [-]  ?
    out   -      void                       [-]  ?
    by    Source/PropertyPanel.cpp

F PresentFooterBand       | ChannelPanel.cpp | 160-177 | - | - | ?
    in    Theme           const ThemeSpecification&  [-]  ?
    in    Band            const WorkspaceRectangle&  [-]  ?
    in    SampledCount    std::uint32_t              [-]  ?
    in    RetainedCount   std::uint32_t              [-]  ?
    in    CutoutEnrolled  bool                       [-]  ?
    out   -               void                       [-]  ?
    by    Source/LayerPanel.cpp, Source/PropertyPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    ONE CHANNEL ROW
//------------------------------------------------------------------------------------------------------------------------

T ChannelIntent           | ChannelPanel.cpp | 187-192 | - | - | What one row's tick asked the material for, applied after the whole run has been walked. is reading, and amending inside the walk would present half a tick against one reading and half against another.
    has   ChannelDeclared  bool                  [-]  ?
    has   Subject          ChannelSubject        [-]  ?
    has   Declaring        ChannelSpecification  [-]  ?
    note  🔴 Deferred for the same reason `LayerPanel`'s is: `DeclareChannel` amends the very declarations the walk

F PresentClassBadge       | ChannelPanel.cpp | 195-205 | - | - | ?
    in    Theme    const ThemeSpecification&  [-]  ?
    in    Area     const WorkspaceRectangle&  [-]  ?
    in    Caption  const char*                [-]  ?
    in    Wash     const ThemeColour&         [-]  ?
    out   -        void                       [-]  ?

F PresentChannelRow       | ChannelPanel.cpp | 207-260 | - | - | ?
    in    Theme     const ThemeSpecification&     [-]  ?
    in    Row       const WorkspaceRectangle&     [-]  ?
    in    Subject   ChannelSubject                [-]  ?
    in    Material  const MaterialSpecification&  [-]  ?
    in    Carry     ChannelPanelCarry&            [-]  ?
    in    Arriving  ChannelIntent&                [-]  ?
    out   -         void                          [-]  ?

F PresentChannelEditor    | ChannelPanel.cpp | 264-344 | - | - | ?
    in    Theme     const ThemeSpecification&     [-]  ?
    in    Row       const WorkspaceRectangle&     [-]  ?
    in    Subject   ChannelSubject                [-]  ?
    in    Material  const MaterialSpecification&  [-]  ?
    in    Carry     ChannelPanelCarry&            [-]  ?
    in    Arriving  ChannelIntent&                [-]  ?
    out   -         bool                          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT A ROW READS
//------------------------------------------------------------------------------------------------------------------------

F CaptionOfChannel        | ChannelPanel.cpp | 352-378 | - | - | ?
    in    Channel  ChannelSubject  [-]  ?
    out   -        const char*     [-]  ?
    by    Api/ChannelPanel.h

F CaptionOfReflectance    | ChannelPanel.cpp | 380-394 | - | - | ?
    in    Selected  ReflectanceSelection  [-]  ?
    out   -         const char*           [-]  ?
    by    Api/ChannelPanel.h

F CaptionOfChannelSource  | ChannelPanel.cpp | 396-407 | - | - | ?
    in    Source  ChannelSource  [-]  ?
    out   -       const char*    [-]  ?
    by    Api/ChannelPanel.h

F CaptionOfChannelMeasure | ChannelPanel.cpp | 409-420 | - | - | ?
    in    Measured  ChannelMeasure  [-]  ?
    out   -         const char*     [-]  ?
    by    Api/ChannelPanel.h

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

F PresentChannelPanel     | ChannelPanel.cpp | 426-644 | - | - | ?
    in    Theme           const ThemeSpecification&  [-]  ?
    in    Area            const WorkspaceRectangle&  [-]  ?
    in    PresentContext  void*                      [-]  ?
    out   -               void                       [-]  ?
    by    Api/ChannelPanel.h
