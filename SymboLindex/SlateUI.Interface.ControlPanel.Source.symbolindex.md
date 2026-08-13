//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The four choosing controls — a crossing nub, a run of exclusive pills, a dropped list, and a row of independent switches.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateUI/Interface/ControlPanel/Source
%layer      SlateUI
%sources    6
%symbols    94
%annotated  17/94
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ControlChoice.cpp  | 410 lines | 612fbbbd | 9 sym  | The four choosing controls — a crossing nub, a run of exclusive pills, a dropped list, and a row of independent switches.
S ControlChrome.cpp  | 510 lines | ec083455 | 18 sym | The five remaining primitives — a colour bar and its coordinate tracks, a menu pill, a glyph square, an accordion header, and a slide.
S ControlInterior.h  | 136 lines | 337ea190 | 25 sym | What the control translation units share — the recording, the pointer, and the shapes every control is assembled from.
S ControlLayout.cpp  | 620 lines | ac66c70a | 28 sym | The row split, the shared shapes, and the stroke alphabet every control paints its chrome from.
S ControlNumeric.cpp | 217 lines | c8de5ccb | 4 sym  | The three numeric entries — a bounded slider, an unbounded scalar drag, and three components side by side.
S ControlText.cpp    | 433 lines | 746bf063 | 10 sym | The three text controls and the caret accumulator beneath them — a field, a bare inline edit, and a path with its browse cap.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

V ControlInterior         | ControlChoice.cpp  | 11      | - | - | ?
    by    Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlLayout.cpp, Source/ControlNumeric.cpp, Source/ControlText.cpp

V ControlInterior         | ControlChrome.cpp  | 13      | - | - | ?
    by    Source/ControlChoice.cpp, Source/ControlInterior.h, Source/ControlLayout.cpp, Source/ControlNumeric.cpp, Source/ControlText.cpp

V QuarterTurn             | ControlChrome.cpp  | 20      | - | - | ?
    by    Source/ConsoleHost.cpp

F Coded255                | ControlChrome.cpp  | 23-28   | - | - | One coordinate printed as the eight-bit code an artist reads it as.
    in    Coordinate  double         [-]  ?
    out   -           std::uint32_t  [-]  ?

V ControlInterior         | ControlNumeric.cpp | 13      | - | - | ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlLayout.cpp, Source/ControlText.cpp

V ControlInterior         | ControlText.cpp    | 11      | - | - | ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlLayout.cpp, Source/ControlNumeric.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                     SHARED SHAPES
//------------------------------------------------------------------------------------------------------------------------

V PillCaptionInset        | ControlChoice.cpp  | 22      | - | - | ?

F PillWidth               | ControlChoice.cpp  | 24-29   | - | - | ?
    in    Caption  const char*  [-]  ?
    out   -        float        [-]  ?

F PaintPill               | ControlChoice.cpp  | 32-48   | - | - | One pill: the face, the hover wash and the caption, in whichever of the two conditions it stands.
    in    Theme    const ThemeSpecification&  [-]  ?
    in    Area     const WorkspaceRectangle&  [-]  ?
    in    Caption  const char*                [-]  ?
    in    Chosen   bool                       [-]  ?
    in    Covered  bool                       [-]  ?
    out   -        void                       [-]  ?

F OrdinalAdmitted         | ControlChoice.cpp  | 51-54   | - | - | Whether one ordinal names a choice in a run at all.
    in    Ordinal      std::uint32_t  [-]  ?
    in    ChoiceCount  std::uint32_t  [-]  ?
    out   -            bool           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE BOOLEAN ENTRY
//------------------------------------------------------------------------------------------------------------------------

F PresentBooleanEntry     | ControlChoice.cpp  | 63-100  | - | - | ?
    in    Theme    const ThemeSpecification&    [-]  ?
    in    Area     const WorkspaceRectangle&    [-]  ?
    in    Caption  const char*                  [-]  ?
    in    Carried  bool&                        [-]  ?
    out   -        Outcome<ControlInteraction>  [-]  ?
    by    Api/ControlPanel.h, Source/PropertyPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE SELECTION ENTRY
//------------------------------------------------------------------------------------------------------------------------

F PresentSelectionEntry   | ControlChoice.cpp  | 107-181 | - | - | ?
    in    Theme           const ThemeSpecification&    [-]  ?
    in    Area            const WorkspaceRectangle&    [-]  ?
    in    Caption         const char*                  [-]  ?
    in    Choices         const char* const*           [-]  ?
    in    ChoiceCount     std::uint32_t                [-]  ?
    in    CarriedOrdinal  std::uint32_t&               [-]  ?
    out   -               Outcome<ControlInteraction>  [-]  ?
    by    Api/ControlPanel.h

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE DROPDOWN
//------------------------------------------------------------------------------------------------------------------------

F PresentDropdown         | ControlChoice.cpp  | 188-325 | - | - | ?
    in    Theme           const ThemeSpecification&    [-]  ?
    in    Area            const WorkspaceRectangle&    [-]  ?
    in    Choices         const char* const*           [-]  ?
    in    ChoiceCount     std::uint32_t                [-]  ?
    in    CarriedOrdinal  std::uint32_t&               [-]  ?
    in    Carry           DropdownCarry&               [-]  ?
    in    PresentedTick   std::uint32_t                [-]  ?
    out   -               Outcome<ControlInteraction>  [-]  ?
    by    Api/ControlPanel.h, Source/PropertyPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE SEGMENT ROW
//------------------------------------------------------------------------------------------------------------------------

F PresentSegmentRow       | ControlChoice.cpp  | 332-408 | - | - | ?
    in    Theme         const ThemeSpecification&    [-]  ?
    in    Area          const WorkspaceRectangle&    [-]  ?
    in    Captions      const char* const*           [-]  ?
    in    Carried       bool*                        [-]  ?
    in    SegmentCount  std::uint32_t                [-]  ?
    out   -             Outcome<ControlInteraction>  [-]  ?
    by    Api/ControlPanel.h

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE COLOUR ENTRY
//------------------------------------------------------------------------------------------------------------------------

F PresentColourEntry      | ControlChrome.cpp  | 37-176  | - | - | ?
    in    Theme       const ThemeSpecification&    [-]  ?
    in    Area        const WorkspaceRectangle&    [-]  ?
    in    Caption     const char*                  [-]  ?
    in    Carried     ThemeColour&                 [-]  ?
    in    PickerOpen  bool&                        [-]  ?
    out   -           Outcome<ControlInteraction>  [-]  ?
    by    Api/ControlPanel.h, Source/ChannelPanel.cpp, Source/PropertyPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE MENU PILL
//------------------------------------------------------------------------------------------------------------------------

F PresentMenuPill         | ControlChrome.cpp  | 183-204 | - | - | ?
    in    Theme        const ThemeSpecification&    [-]  ?
    in    Area         const WorkspaceRectangle&    [-]  ?
    in    Caption      const char*                  [-]  ?
    in    Highlighted  bool                         [-]  ?
    out   -            Outcome<ControlInteraction>  [-]  ?
    by    Api/ControlPanel.h, Source/DiagnosticPanel.cpp, Source/RevisionPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE GLYPH BUTTON
//------------------------------------------------------------------------------------------------------------------------

F PresentGlyphButton      | ControlChrome.cpp  | 211-256 | - | - | ?
    in    Theme        const ThemeSpecification&    [-]  ?
    in    Area         const WorkspaceRectangle&    [-]  ?
    in    Stroke       ControlStroke                [-]  ?
    in    DepotSlot    std::uint64_t                [-]  ?
    in    Highlighted  bool                         [-]  ?
    out   -            Outcome<ControlInteraction>  [-]  ?
    by    Api/ControlPanel.h

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE SECTION HEADER
//------------------------------------------------------------------------------------------------------------------------

F PresentSectionHeader    | ControlChrome.cpp  | 263-323 | - | - | ?
    in    Theme        const ThemeSpecification&    [-]  ?
    in    Area         const WorkspaceRectangle&    [-]  ?
    in    Caption      const char*                  [-]  ?
    in    SectionOpen  bool&                        [-]  ?
    in    Trailing     const char*                  [-]  ?
    out   -            Outcome<ControlInteraction>  [-]  ?
    by    Api/ControlPanel.h, Source/ChannelPanel.cpp, Source/DiagnosticPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE CONTENT CAROUSEL
//------------------------------------------------------------------------------------------------------------------------

F AdvanceContentCarousel  | ControlChrome.cpp  | 330-378 | - | - | ?
    in    Theme            const ThemeSpecification&  [-]  ?
    in    Carry            CarouselCarry&             [-]  ?
    in    ElapsedInterval  float                      [-]  ?
    out   -                Outcome<float>             [-]  ?
    by    Api/ControlPanel.h

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE PAINTING SEAM
//------------------------------------------------------------------------------------------------------------------------

F PresentSurfaceFill      | ControlChrome.cpp  | 385-397 | - | - | ?
    in    Area      const WorkspaceRectangle&  [-]  ?
    in    Colour    const ThemeColour&         [-]  ?
    in    Rounding  float                      [-]  ?
    out   -         void                       [-]  ?
    by    Api/ControlPanel.h, Source/ChannelPanel.cpp, Source/DiagnosticPanel.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp, Source/RevisionPanel.cpp

F PresentSurfaceOutline   | ControlChrome.cpp  | 399-414 | - | - | ?
    in    Area       const WorkspaceRectangle&  [-]  ?
    in    Colour     const ThemeColour&         [-]  ?
    in    Rounding   float                      [-]  ?
    in    Thickness  float                      [-]  ?
    out   -          void                       [-]  ?
    by    Api/ControlPanel.h, Source/LayerPanel.cpp, Source/RevisionPanel.cpp

F PresentTextRun          | ControlChrome.cpp  | 416-448 | - | - | ?
    in    Area       const WorkspaceRectangle&  [-]  ?
    in    Text       const char*                [-]  ?
    in    Colour     const ThemeColour&         [-]  ?
    in    Placement  TextPlacement              [-]  ?
    in    FontScale  float                      [-]  ?
    out   -          void                       [-]  ?
    by    Api/ControlPanel.h, Source/ChannelPanel.cpp, Source/DiagnosticPanel.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp, Source/RevisionPanel.cpp

F DeclareClip             | ControlChrome.cpp  | 450-454 | - | - | ?
    in    Area  const WorkspaceRectangle&  [-]  ?
    out   -     void                       [-]  ?
    by    Api/ControlPanel.h, Source/ChannelPanel.cpp, Source/DiagnosticPanel.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp, Source/RevisionPanel.cpp

F ReclaimClip             | ControlChrome.cpp  | 456-459 | - | - | ?
    out   -  void  [-]  ?
    by    Api/ControlPanel.h, Source/ChannelPanel.cpp, Source/DiagnosticPanel.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp, Source/RevisionPanel.cpp

F MeasuredTextExtent      | ControlChrome.cpp  | 461-469 | - | - | ?
    in    Text       const char*  [-]  ?
    in    FontScale  float        [-]  ?
    out   -          float        [-]  ?
    by    Api/ControlPanel.h, Source/ChannelPanel.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE POINTER SEAM
//------------------------------------------------------------------------------------------------------------------------

F ResolveAreaPress        | ControlChrome.cpp  | 479-482 | - | - | ?
    in    Area  const WorkspaceRectangle&  [-]  ?
    out   -     ControlInteraction         [-]  ?
    by    Api/ControlPanel.h, Source/ChannelPanel.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp

F ResolvePointerPosition  | ControlChrome.cpp  | 484-490 | - | - | ?
    in    PositionX  float&  [-]  ?
    in    PositionY  float&  [-]  ?
    out   -          void    [-]  ?
    by    Api/ControlPanel.h, Source/LayerPanel.cpp

F PointerHeld             | ControlChrome.cpp  | 492-495 | - | - | ?
    out   -  bool  [-]  ?
    by    Api/ControlPanel.h, Source/LayerPanel.cpp

F AdvanceVisibleOffset    | ControlChrome.cpp  | 497-508 | - | - | ?
    in    Carried        float&                     [-]  ?
    in    Area           const WorkspaceRectangle&  [-]  ?
    in    ContentExtent  float                      [-]  ?
    out   -              Outcome<float>             [-]  ?
    by    Api/ControlPanel.h, Source/ChannelPanel.cpp, Source/DiagnosticPanel.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp, Source/RevisionPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                   CODES AND GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

F Coded                   | ControlInterior.h  | 30      | - | - | ?
    in    Colour  const ThemeColour&  [-]  ?
    out   -       ImU32               [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/WorkspaceDrag.cpp, Source/WorkspacePanel.cpp, Source/WorkspaceStrip.cpp, (+1 more)

F Corner                  | ControlInterior.h  | 31      | - | - | ?
    in    Area  const WorkspaceRectangle&  [-]  ?
    out   -     ImVec2                     [-]  ?
    by    Source/ConsoleHost.cpp, Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/DecalProjection.cpp, Source/OcclusionProjection.cpp, (+10 more)

F Opposite                | ControlInterior.h  | 32      | - | - | ?
    in    Area  const WorkspaceRectangle&  [-]  ?
    out   -     ImVec2                     [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/WorkspaceDrag.cpp, Source/WorkspacePanel.cpp, Source/WorkspaceStrip.cpp, (+1 more)

F Centre                  | ControlInterior.h  | 33      | - | - | ?
    in    Area  const WorkspaceRectangle&  [-]  ?
    out   -     ImVec2                     [-]  ?
    by    Api/WorkspaceSpace.h, Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/ControlLayout.cpp, Source/OcclusionProjection.cpp, Source/PrimitiveStructure.cpp, (+2 more)

F Inset                   | ControlInterior.h  | 35      | - | - | ?
    in    Area    const WorkspaceRectangle&  [-]  ?
    in    Margin  float                      [-]  ?
    out   -       WorkspaceRectangle         [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/RevisionPanel.cpp

F CentredBand             | ControlInterior.h  | 36      | - | - | ?
    in    Area    const WorkspaceRectangle&  [-]  ?
    in    Height  float                      [-]  ?
    out   -       WorkspaceRectangle         [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlNumeric.cpp, Source/ControlText.cpp

F LeftSlice               | ControlInterior.h  | 37      | - | - | ?
    in    Area   const WorkspaceRectangle&  [-]  ?
    in    Width  float                      [-]  ?
    out   -      WorkspaceRectangle         [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlNumeric.cpp

F RightSlice              | ControlInterior.h  | 38      | - | - | ?
    in    Area   const WorkspaceRectangle&  [-]  ?
    in    Width  float                      [-]  ?
    out   -      WorkspaceRectangle         [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlText.cpp

F SquareIn                | ControlInterior.h  | 39      | - | - | ?
    in    Area  const WorkspaceRectangle&  [-]  ?
    in    Edge  float                      [-]  ?
    out   -     WorkspaceRectangle         [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp

F Coded                   | ControlLayout.cpp  | 20-23   | - | - | ?
    in    Colour  const ThemeColour&  [-]  ?
    out   -       ImU32               [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/WorkspaceDrag.cpp, Source/WorkspacePanel.cpp, Source/WorkspaceStrip.cpp, (+1 more)

F Corner                  | ControlLayout.cpp  | 25-28   | - | - | ?
    in    Area  const WorkspaceRectangle&  [-]  ?
    out   -     ImVec2                     [-]  ?
    by    Source/ConsoleHost.cpp, Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/DecalProjection.cpp, Source/OcclusionProjection.cpp, (+10 more)

F Opposite                | ControlLayout.cpp  | 30-33   | - | - | ?
    in    Area  const WorkspaceRectangle&  [-]  ?
    out   -     ImVec2                     [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/WorkspaceDrag.cpp, Source/WorkspacePanel.cpp, Source/WorkspaceStrip.cpp, (+1 more)

F Centre                  | ControlLayout.cpp  | 35-38   | - | - | ?
    in    Area  const WorkspaceRectangle&  [-]  ?
    out   -     ImVec2                     [-]  ?
    by    Api/WorkspaceSpace.h, Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/ControlInterior.h, Source/OcclusionProjection.cpp, Source/PrimitiveStructure.cpp, (+2 more)

F Inset                   | ControlLayout.cpp  | 40-53   | - | - | ?
    in    Area    const WorkspaceRectangle&  [-]  ?
    in    Margin  float                      [-]  ?
    out   -       WorkspaceRectangle         [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/RevisionPanel.cpp

F CentredBand             | ControlLayout.cpp  | 55-63   | - | - | ?
    in    Area    const WorkspaceRectangle&  [-]  ?
    in    Height  float                      [-]  ?
    out   -       WorkspaceRectangle         [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlNumeric.cpp, Source/ControlText.cpp

F LeftSlice               | ControlLayout.cpp  | 65-72   | - | - | ?
    in    Area   const WorkspaceRectangle&  [-]  ?
    in    Width  float                      [-]  ?
    out   -      WorkspaceRectangle         [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlNumeric.cpp

F RightSlice              | ControlLayout.cpp  | 74-82   | - | - | ?
    in    Area   const WorkspaceRectangle&  [-]  ?
    in    Width  float                      [-]  ?
    out   -      WorkspaceRectangle         [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlText.cpp

F SquareIn                | ControlLayout.cpp  | 84-94   | - | - | ?
    in    Area  const WorkspaceRectangle&  [-]  ?
    in    Edge  float                      [-]  ?
    out   -     WorkspaceRectangle         [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE POINTER
//------------------------------------------------------------------------------------------------------------------------

T PointerReading          | ControlInterior.h  | 46-55   | - | - | The pointer as one tick sees it, read once so every control in a tick agrees on where it is.
    has   PositionX     float  [-]  ?
    has   PositionY     float  [-]  ?
    has   TravelX       float  [-]  ?
    has   PressBegan    bool   [-]  ?
    has   PressHeld     bool   [-]  ?
    has   PressEnded    bool   [-]  ?
    has   PressDoubled  bool   [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlNumeric.cpp, Source/ControlText.cpp

F ResolvePointer          | ControlInterior.h  | 57      | - | - | ?
    out   -  PointerReading  [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlNumeric.cpp, Source/ControlText.cpp

F PointerCovers           | ControlInterior.h  | 58      | - | - | ?
    in    Pointer  const PointerReading&      [-]  ?
    in    Area     const WorkspaceRectangle&  [-]  ?
    out   -        bool                       [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlText.cpp

F ResolvePointer          | ControlLayout.cpp  | 100-115 | - | - | ?
    out   -  PointerReading  [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlNumeric.cpp, Source/ControlText.cpp

F PointerCovers           | ControlLayout.cpp  | 117-120 | - | - | ?
    in    Pointer  const PointerReading&      [-]  ?
    in    Area     const WorkspaceRectangle&  [-]  ?
    out   -        bool                       [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlText.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                THE PRESS AND THE TRACK
//------------------------------------------------------------------------------------------------------------------------

F ResolvePress            | ControlInterior.h  | 70      | - | - | A press with no hold — what a control that answers a click and never drags reports. any release covering it would fire when a drag started elsewhere happened to end over it, and the defect presents as a section collapsing because a slider release landed on its header. own rectangle needs one, and that control uses `ResolveTrack` instead.
    in    Area  const WorkspaceRectangle&  [-]  ?
    out   -     ControlInteraction         [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlText.cpp
    note  🔴 The release is only honoured where the press **began** over the same rectangle. A control that acted on
    note  ⚠️ This claims no active identity. Only a control that must keep amending after the pointer has left its

T TrackHold               | ControlInterior.h  | 76-81   | - | - | One grabbable track: reports the interaction and, while held, the fraction the pointer names. that left the track would otherwise stop amending the moment it did, which is the defect where a slider drops the reading if the artist's hand strays a few pixels below the row.
    has   Interaction  ControlInteraction  [-]  ?
    has   HoldOpen     bool                [-]  ?
    has   Fraction     float               [-]  ?
    by    Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlNumeric.cpp
    note  🔴 The hold is identified by the vendor's active identity and not by "the pointer is down over me". A drag

F ResolveTrack            | ControlInterior.h  | 84      | - | - | Resolves one track's hold, the claim keyed by an address the caller guarantees is stable across ticks.
    in    Area    const WorkspaceRectangle&  [-]  ?
    in    Anchor  const void*                [-]  ?
    out   -       TrackHold                  [-]  ?
    by    Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlNumeric.cpp

F PaintTrack              | ControlInterior.h  | 87      | - | - | Paints a track, its travelled fill and its knob at a declared fraction.
    in    Theme          const ThemeSpecification&  [-]  ?
    in    Area           const WorkspaceRectangle&  [-]  ?
    in    Fraction       float                      [-]  ?
    in    FillTravelled  bool                       [-]  ?
    in    Held           bool                       [-]  ?
    out   -              void                       [-]  ?
    by    Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlNumeric.cpp

F ResolvePress            | ControlLayout.cpp  | 126-150 | - | - | ?
    in    Area  const WorkspaceRectangle&  [-]  ?
    out   -     ControlInteraction         [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlText.cpp

F ResolveTrack            | ControlLayout.cpp  | 152-192 | - | - | ?
    in    Area    const WorkspaceRectangle&  [-]  ?
    in    Anchor  const void*                [-]  ?
    out   -       TrackHold                  [-]  ?
    by    Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlNumeric.cpp

F PaintTrack              | ControlLayout.cpp  | 194-228 | - | - | ?
    in    Theme          const ThemeSpecification&  [-]  ?
    in    Area           const WorkspaceRectangle&  [-]  ?
    in    Fraction       float                      [-]  ?
    in    FillTravelled  bool                       [-]  ?
    in    Held           bool                       [-]  ?
    out   -              void                       [-]  ?
    by    Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlNumeric.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    SHARED PAINTING
//------------------------------------------------------------------------------------------------------------------------

F Recording               | ControlInterior.h  | 97      | - | - | ?
    out   -  ImDrawList*  [-]  ?
    by    Api/CommandSequence.h, Api/DomainSpace.h, Source/AssetInterchange.cpp, Source/CommandSequence.cpp, Source/ControlChoice.cpp, Source/ControlChrome.cpp, (+9 more)

F PaintFill               | ControlInterior.h  | 99      | - | - | ?
    in    Area      const WorkspaceRectangle&  [-]  ?
    in    Colour    const ThemeColour&         [-]  ?
    in    Rounding  float                      [-]  ?
    out   -         void                       [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlText.cpp

F PaintOutline            | ControlInterior.h  | 100     | - | - | ?
    in    Area       const WorkspaceRectangle&  [-]  ?
    in    Colour     const ThemeColour&         [-]  ?
    in    Rounding   float                      [-]  ?
    in    Thickness  float                      [-]  ?
    out   -          void                       [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlLayout.cpp, Source/ControlText.cpp

F PaintDisc               | ControlInterior.h  | 101     | - | - | ?
    in    CentreX  float               [-]  ?
    in    CentreY  float               [-]  ?
    in    Radius   float               [-]  ?
    in    Colour   const ThemeColour&  [-]  ?
    out   -        void                [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlText.cpp

F PaintCaption            | ControlInterior.h  | 104     | - | - | Text clipped to a rectangle, aligned by two fractions — zero is left or top, one is right or bottom.
    in    Area                 const WorkspaceRectangle&  [-]  ?
    in    Caption              const char*                [-]  ?
    in    Colour               const ThemeColour&         [-]  ?
    in    HorizontalAlignment  float                      [-]  ?
    in    VerticalAlignment    float                      [-]  ?
    in    FontScale            float                      [-]  ?
    out   -                    void                       [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlText.cpp

V ReadoutExtent           | ControlInterior.h  | 113     | - | - | ?
    by    Source/ControlChrome.cpp, Source/ControlNumeric.cpp

F Bounded                 | ControlInterior.h  | 116     | - | - | One reading bounded to a closed interval, the ends included.
    in    Reading  double  [-]  ?
    in    Floor    double  [-]  ?
    in    Ceiling  double  [-]  ?
    out   -        double  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+74 more)

F PrintReading            | ControlInterior.h  | 122     | - | - | One reading printed to the declared number of decimals, into a caller-owned buffer. not a second authority over it; rounding the carry to what is shown loses precision the artist never asked to lose.
    in    Destination        char*          [-]  ?
    in    DestinationExtent  std::uint32_t  [-]  ?
    in    Reading            double         [-]  ?
    in    Decimals           std::uint32_t  [-]  ?
    out   -                  void           [-]  ?
    by    Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlNumeric.cpp
    note  ⚠️ Printed and never rounded in place. `02`'s tiers make the readout a presentation of the reading and

F PaintValueBox           | ControlInterior.h  | 127     | - | - | The value box — a black centre carrying the readout, capped by a side segment at one end or the other.
    in    Theme       const ThemeSpecification&                                                           [-]   ?
    in    Area        const WorkspaceRectangle&                                                           [-]   ?
    in    CapCaption  const char*                                                                         [-]   ?
    in    CapWidth    float                                                                               [px]  zero paints no cap
    in    CapLeading  bool                                                                                [-]   ?
    in    Readout     const char*                                                                         [-]   ?
    in    Focused     bool                                                                                [-]   ?
    out   -           the rectangle the readout occupies, for a caller that wants to place a caret in it  [-]   ?
    by    Source/ControlLayout.cpp, Source/ControlNumeric.cpp

F Recording               | ControlLayout.cpp  | 237-240 | - | - | ?
    out   -  ImDrawList*  [-]  ?
    by    Api/CommandSequence.h, Api/DomainSpace.h, Source/AssetInterchange.cpp, Source/CommandSequence.cpp, Source/ControlChoice.cpp, Source/ControlChrome.cpp, (+9 more)

F PaintFill               | ControlLayout.cpp  | 242-253 | - | - | ?
    in    Area      const WorkspaceRectangle&  [-]  ?
    in    Colour    const ThemeColour&         [-]  ?
    in    Rounding  float                      [-]  ?
    out   -         void                       [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlText.cpp

F PaintOutline            | ControlLayout.cpp  | 255-264 | - | - | ?
    in    Area       const WorkspaceRectangle&  [-]  ?
    in    Colour     const ThemeColour&         [-]  ?
    in    Rounding   float                      [-]  ?
    in    Thickness  float                      [-]  ?
    out   -          void                       [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlInterior.h, Source/ControlText.cpp

F PaintDisc               | ControlLayout.cpp  | 266-272 | - | - | ?
    in    CentreX  float               [-]  ?
    in    CentreY  float               [-]  ?
    in    Radius   float               [-]  ?
    in    Colour   const ThemeColour&  [-]  ?
    out   -        void                [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlText.cpp

F PaintCaption            | ControlLayout.cpp  | 274-296 | - | - | ?
    in    Area                 const WorkspaceRectangle&  [-]  ?
    in    Caption              const char*                [-]  ?
    in    Colour               const ThemeColour&         [-]  ?
    in    HorizontalAlignment  float                      [-]  ?
    in    VerticalAlignment    float                      [-]  ?
    in    FontScale            float                      [-]  ?
    out   -                    void                       [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlText.cpp

F Bounded                 | ControlLayout.cpp  | 298-301 | - | - | ?
    in    Reading  double  [-]  ?
    in    Floor    double  [-]  ?
    in    Ceiling  double  [-]  ?
    out   -        double  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+74 more)

F PrintReading            | ControlLayout.cpp  | 303-311 | - | - | ?
    in    Destination        char*          [-]  ?
    in    DestinationExtent  std::uint32_t  [-]  ?
    in    Reading            double         [-]  ?
    in    Decimals           std::uint32_t  [-]  ?
    out   -                  void           [-]  ?
    by    Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlNumeric.cpp

F PaintValueBox           | ControlLayout.cpp  | 313-358 | - | - | ?
    in    Theme       const ThemeSpecification&  [-]  ?
    in    Area        const WorkspaceRectangle&  [-]  ?
    in    CapCaption  const char*                [-]  ?
    in    CapWidth    float                      [-]  ?
    in    CapLeading  bool                       [-]  ?
    in    Readout     const char*                [-]  ?
    in    Focused     bool                       [-]  ?
    out   -           WorkspaceRectangle         [-]  ?
    by    Source/ControlInterior.h, Source/ControlNumeric.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE ROW SPLIT
//------------------------------------------------------------------------------------------------------------------------

F ResolveControlRow       | ControlLayout.cpp  | 367-393 | - | - | ?
    in    Theme  const ThemeSpecification&  [-]  ?
    in    Area   const WorkspaceRectangle&  [-]  ?
    out   -      ControlRowSplit            [-]  ?
    by    Api/ControlPanel.h, Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlNumeric.cpp, Source/ControlText.cpp, Source/PropertyPanel.cpp

F PresentControlLabel     | ControlLayout.cpp  | 395-398 | - | - | ?
    in    Theme    const ThemeSpecification&  [-]  ?
    in    Area     const WorkspaceRectangle&  [-]  ?
    in    Caption  const char*                [-]  ?
    out   -        void                       [-]  ?
    by    Api/ControlPanel.h, Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlNumeric.cpp, Source/ControlText.cpp, Source/PropertyPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE STROKE ALPHABET
//------------------------------------------------------------------------------------------------------------------------

T StrokeMapping           | ControlLayout.cpp  | 410-417 | - | - | ?
    has   CentreX   float  [-]  ?
    has   CentreY   float  [-]  ?
    has   HalfEdge  float  [-]  ?
    has   Cosine    float  [-]  ?
    has   Sine      float  [-]  ?

F Placed                  | ControlLayout.cpp  | 419-426 | - | - | ?
    in    Mapping  const StrokeMapping&  [-]  ?
    in    UnitX    float                 [-]  ?
    in    UnitY    float                 [-]  ?
    out   -        ImVec2                [-]  ?
    by    Api/DecalProjection.h, Api/DomainSpace.h, Source/AnalyticProjection.cpp, Source/ConsoleHost.cpp, Source/DecalProjection.cpp, Source/DomainSpace.cpp, (+12 more)

F StrokeRun               | ControlLayout.cpp  | 428-451 | - | - | ?
    in    Mapping          const StrokeMapping&  [-]  ?
    in    UnitCoordinates  const float*          [-]  ?
    in    PointCount       std::uint32_t         [-]  ?
    in    Code             ImU32                 [-]  ?
    in    Thickness        float                 [-]  ?
    in    Closed           bool                  [-]  ?
    out   -                void                  [-]  ?

F PresentControlStroke    | ControlLayout.cpp  | 455-618 | - | - | ?
    in    Area       const WorkspaceRectangle&  [-]  ?
    in    Stroke     ControlStroke              [-]  ?
    in    Colour     const ThemeColour&         [-]  ?
    in    Thickness  float                      [-]  ?
    in    Rotation   float                      [-]  ?
    out   -          void                       [-]  ?
    by    Api/ControlPanel.h, Source/ChannelPanel.cpp, Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp, (+1 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE VALUE SLIDER
//------------------------------------------------------------------------------------------------------------------------

F PresentValueSlider      | ControlNumeric.cpp | 24-88   | - | - | ?
    in    Theme     const ThemeSpecification&    [-]  ?
    in    Area      const WorkspaceRectangle&    [-]  ?
    in    Caption   const char*                  [-]  ?
    in    Carried   double&                      [-]  ?
    in    Floor     double                       [-]  ?
    in    Ceiling   double                       [-]  ?
    in    Unit      const char*                  [-]  ?
    in    Decimals  std::uint32_t                [-]  ?
    out   -         Outcome<ControlInteraction>  [-]  ?
    by    Api/ControlPanel.h, Source/ChannelPanel.cpp, Source/PropertyPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE SCALAR ENTRY
//------------------------------------------------------------------------------------------------------------------------

F PresentScalarEntry      | ControlNumeric.cpp | 95-148  | - | - | ?
    in    Theme     const ThemeSpecification&    [-]  ?
    in    Area      const WorkspaceRectangle&    [-]  ?
    in    Caption   const char*                  [-]  ?
    in    Carried   double&                      [-]  ?
    in    Step      double                       [-]  ?
    in    Unit      const char*                  [-]  ?
    in    Decimals  std::uint32_t                [-]  ?
    out   -         Outcome<ControlInteraction>  [-]  ?
    by    Api/ControlPanel.h, Source/PropertyPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE VECTOR ENTRY
//------------------------------------------------------------------------------------------------------------------------

F PresentVectorEntry      | ControlNumeric.cpp | 155-215 | - | - | ?
    in    Theme     const ThemeSpecification&    [-]  ?
    in    Area      const WorkspaceRectangle&    [-]  ?
    in    Caption   const char*                  [-]  ?
    in    Carried   double                       [-]  ?
    in    Step      double                       [-]  ?
    in    Decimals  std::uint32_t                [-]  ?
    out   -         Outcome<ControlInteraction>  [-]  ?
    by    Api/ControlPanel.h

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE CARET ACCUMULATOR
//------------------------------------------------------------------------------------------------------------------------

E EditProgress            | ControlText.cpp    | 21-27   | - | - | What one tick of an open edit produced.
    has   Continuing  EditProgress  [-]  ?
    has   Amended     EditProgress  [-]  ?
    has   Sealed      EditProgress  [-]  ?
    has   Abandoned   EditProgress  [-]  ?

F AdmitCharacter          | ControlText.cpp    | 34-53   | - | - | Inserts one printable character at the caret and advances it.
    in    Carry    TextCarry&  [-]  ?
    in    Arrived  char        [-]  ?
    out   -        bool        [-]  ?

F WithdrawCharacter       | ControlText.cpp    | 56-69   | - | - | Removes one character at a named position and draws the tail back over it.
    in    Carry     TextCarry&     [-]  ?
    in    Position  std::uint32_t  [-]  ?
    out   -         bool           [-]  ?

F AdvanceTextCarry        | ControlText.cpp    | 71-128  | - | - | ?
    in    Carry  TextCarry&    [-]  ?
    out   -      EditProgress  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE SHARED FIELD
//------------------------------------------------------------------------------------------------------------------------

F CaretAdvance            | ControlText.cpp    | 135-144 | - | - | The advance of a carry's leading characters, which is where its caret stands.
    in    Carry  const TextCarry&  [-]  ?
    out   -      float             [-]  ?

F AdvanceField            | ControlText.cpp    | 150-226 | - | - | Resolves one field's opening, typing and sealing, and paints the text and the caret inside it.
    in    Theme        const ThemeSpecification&  [-]   ?
    in    Frame        const WorkspaceRectangle&  [px]  what the pointer is resolved against; the frame's own fill is the caller's
    in    Placement    const WorkspaceRectangle&  [px]  where the text is laid, already inset from that frame
    in    Carry        TextCarry&                 [-]   ?
    in    Placeholder  const char*                [-]   ?
    in    Settled      const char*                [-]   what to present while no edit is open; the carry itself where none is named
    out   -            ControlInteraction         [-]   ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE TEXT ENTRY
//------------------------------------------------------------------------------------------------------------------------

F PresentTextEntry        | ControlText.cpp    | 235-273 | - | - | ?
    in    Theme        const ThemeSpecification&    [-]  ?
    in    Area         const WorkspaceRectangle&    [-]  ?
    in    Caption      const char*                  [-]  ?
    in    Carry        TextCarry&                   [-]  ?
    in    Placeholder  const char*                  [-]  ?
    out   -            Outcome<ControlInteraction>  [-]  ?
    by    Api/ControlPanel.h, Source/LayerPanel.cpp, Source/PropertyPanel.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE INLINE TEXT EDITOR
//------------------------------------------------------------------------------------------------------------------------

F PresentInlineTextEditor | ControlText.cpp    | 280-350 | - | - | ?
    in    Theme  const ThemeSpecification&    [-]  ?
    in    Area   const WorkspaceRectangle&    [-]  ?
    in    Carry  TextCarry&                   [-]  ?
    out   -      Outcome<ControlInteraction>  [-]  ?
    by    Api/ControlPanel.h

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PATH ENTRY
//------------------------------------------------------------------------------------------------------------------------

F PresentPathEntry        | ControlText.cpp    | 357-431 | - | - | ?
    in    Theme           const ThemeSpecification&    [-]  ?
    in    Area            const WorkspaceRectangle&    [-]  ?
    in    Caption         const char*                  [-]  ?
    in    Carry           TextCarry&                   [-]  ?
    in    BrowseDeclared  bool&                        [-]  ?
    out   -               Outcome<ControlInteraction>  [-]  ?
    by    Api/ControlPanel.h
