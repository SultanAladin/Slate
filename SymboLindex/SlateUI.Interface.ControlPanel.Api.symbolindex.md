//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Every primitive control as one immediate-mode free function over a rectangle — no widget owns anything, and the theme arrives by reference.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateUI/Interface/ControlPanel/Api
%layer      SlateUI
%sources    1
%symbols    37
%annotated  35/37
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ControlPanel.h | 450 lines | eb4714b1 | 37 sym | Every primitive control as one immediate-mode free function over a rectangle — no widget owns anything, and the theme arrives by reference.

//------------------------------------------------------------------------------------------------------------------------
//                                                 WHAT A CONTROL REPORTS
//------------------------------------------------------------------------------------------------------------------------

T ControlInteraction       | ControlPanel.h | 30-37   | contract,nonallocating,nonthrowing | -  | What one control did this tick — the pointer over it, and the three points of an edit's lifecycle. on every `EditDeclared`, and seals on `EditSealed`. A control that reported only "the reading changed" would leave the caller unable to tell a drag from a run of separate edits, and the defect presents as a revision row per pixel of pointer travel — which is exactly `84`'s merge requirement failing. `EditDeclared` may be false on that tick; sealing is not conditional on it.
    has   PointerOver    bool  [-]  ?
    has   EditOpened     bool  [-]  ?
    has   EditDeclared   bool  [-]  ?
    has   EditSealed     bool  [-]  ?
    has   EditAbandoned  bool  [-]  ?
    by    Source/ChannelPanel.cpp, Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlInterior.h, Source/ControlLayout.cpp, Source/ControlNumeric.cpp, (+5 more)
    note  🔴 `10` §2.4: a drag is **one** transaction and not one per tick. A caller opens on `EditOpened`, amends
    note  ⚠️ `EditSealed` arrives on the tick the pointer is released and carries no further amendment with it.

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE STROKE ALPHABET
//------------------------------------------------------------------------------------------------------------------------

E ControlStroke            | ControlPanel.h | 47-65   | contract                           | -  | The chrome glyphs a control paints from line and arc primitives when no uploaded glyph is named. rasteriser. A panel that wants authored art names a depot slot instead and the stroke is not consulted.
    has   None     ControlStroke  [-]  ?
    has   Twisty   ControlStroke  [-]  ?
    has   Chevron  ControlStroke  [-]  ?
    has   Caret    ControlStroke  [-]  ?
    has   Plus     ControlStroke  [-]  ?
    has   Cross    ControlStroke  [-]  ?
    has   Check    ControlStroke  [-]  ?
    has   Eye      ControlStroke  [-]  ?
    has   Trash    ControlStroke  [-]  ?
    has   Search   ControlStroke  [-]  ?
    has   Cog      ControlStroke  [-]  ?
    has   Image    ControlStroke  [-]  ?
    has   Brush    ControlStroke  [-]  ?
    has   Reload   ControlStroke  [-]  ?
    has   Circle   ControlStroke  [-]  ?
    has   Grip     ControlStroke  [-]  ?
    by    Source/ChannelPanel.cpp, Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp, (+1 more)
    note  📝 Flat single-colour chrome is a dozen strokes, and a stroke costs no upload, no descriptor and no

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE ROW SPLIT
//------------------------------------------------------------------------------------------------------------------------

T ControlRowSplit          | ControlPanel.h | 73-77   | contract,nonallocating,nonthrowing | -  | One control row divided into its label column and the field beside it.
    has   LabelArea  WorkspaceRectangle  [-]  ?
    has   FieldArea  WorkspaceRectangle  [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlNumeric.cpp, Source/ControlText.cpp, Source/PropertyPanel.cpp

F ResolveControlRow        | ControlPanel.h | 88      | api,nonallocating,nonthrowing      | ✔️ | Divides one row into remix's 88 px label column and the field that takes the remainder. falls back to `LabelColumnRatio`. Frontier clipped the field to nothing instead, and the defect presents as a docked-narrow panel whose sliders are all zero pixels wide and cannot be grabbed at all.
    in    Theme  const ThemeSpecification&  [-]   read for the two column extents and the gap between them
    in    Area   const WorkspaceRectangle&  [px]  the whole row
    out   -      Split                      [px]  the two rectangles, never overlapping
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlNumeric.cpp, Source/ControlText.cpp, Source/PropertyPanel.cpp
    note  ⚠️ Below `LabelColumnWidth + LabelColumnGap + ValueColumnWidth` the fixed split cannot hold, and the row

F PresentControlLabel      | ControlPanel.h | 93      | api,nonallocating,nonthrowing      | ✔️ | Paints one row's caption in the muted text colour, clipped to its column.
    in    Theme    const ThemeSpecification&  [-]  ?
    in    Area     const WorkspaceRectangle&  [-]  ?
    in    Caption  const char*                [-]  ?
    out   -        void                       [-]  ?
    by    Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/ControlNumeric.cpp, Source/ControlText.cpp, Source/PropertyPanel.cpp

F PresentControlStroke     | ControlPanel.h | 99      | api,nonallocating,nonthrowing      | ✔️ | Paints one stroke centred in a square, at the coverage and thickness the caller declares.
    in    Area       const WorkspaceRectangle&  [-]    ?
    in    Stroke     ControlStroke              [-]    ?
    in    Colour     const ThemeColour&         [-]    ?
    in    Thickness  float                      [-]    ?
    in    Rotation   float                      [rad]  applied about the square's centre; a twisty opens at a quarter turn
    out   -          void                       [-]    ?
    by    Source/ChannelPanel.cpp, Source/ControlChoice.cpp, Source/ControlChrome.cpp, Source/ControlLayout.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp, (+1 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE PAINTING SEAM
//------------------------------------------------------------------------------------------------------------------------

E TextPlacement            | ControlPanel.h | 115-120 | contract                           | -  | Where a text run sits inside the rectangle it was given.
    has   Leading   TextPlacement  [-]  ?
    has   Centred   TextPlacement  [-]  ?
    has   Trailing  TextPlacement  [-]  ?
    by    Source/ChannelPanel.cpp, Source/ControlChrome.cpp, Source/DiagnosticPanel.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp, Source/RevisionPanel.cpp

F PresentSurfaceFill       | ControlPanel.h | 127     | api,nonallocating,nonthrowing      | ✔️ | Fills one rectangle at the declared corner radius. `999px` for exactly that and refusing it would make every pill site carry its own arithmetic.
    in    Area      const WorkspaceRectangle&  [-]  ?
    in    Colour    const ThemeColour&         [-]  ?
    in    Rounding  float                      [-]  ?
    out   -         void                       [-]  ?
    by    Source/ChannelPanel.cpp, Source/ControlChrome.cpp, Source/DiagnosticPanel.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp, Source/RevisionPanel.cpp
    note  A radius beyond half the shorter span is fully rounded rather than refused — the reference spells

F PresentSurfaceOutline    | ControlPanel.h | 132     | api,nonallocating,nonthrowing      | ✔️ | Outlines one rectangle at the declared corner radius and thickness.
    in    Area       const WorkspaceRectangle&  [-]  ?
    in    Colour     const ThemeColour&         [-]  ?
    in    Rounding   float                      [-]  ?
    in    Thickness  float                      [-]  ?
    out   -          void                       [-]  ?
    by    Source/ControlChrome.cpp, Source/LayerPanel.cpp, Source/RevisionPanel.cpp

F PresentTextRun           | ControlPanel.h | 141     | api,nonallocating,nonthrowing      | ✔️ | Prints one run of text placed inside a rectangle, clipped to it.
    in    Area       const WorkspaceRectangle&  [-]  ?
    in    Text       const char*                [-]  ?
    in    Colour     const ThemeColour&         [-]  ?
    in    Placement  TextPlacement              [-]  ?
    in    FontScale  float                      [-]  one leaves the interface font as it stands; refused values below a sixteenth are bounded
    out   -          void                       [-]  ?
    by    Source/ChannelPanel.cpp, Source/ControlChrome.cpp, Source/DiagnosticPanel.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp, Source/RevisionPanel.cpp

F DeclareClip              | ControlPanel.h | 153     | api,nonallocating,nonthrowing      | ✔️ | Narrows the recording to one rectangle until the matching reclaim. leaves the whole rest of the tick clipped to a panel's interior, and the defect presents as the tab strip vanishing rather than as anything the panel did.
    in    Area  const WorkspaceRectangle&  [-]  ?
    out   -     void                       [-]  ?
    by    Source/ChannelPanel.cpp, Source/ControlChrome.cpp, Source/DiagnosticPanel.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp, Source/RevisionPanel.cpp
    note  🔴 Every declaration is reclaimed on every path out of the scope that made it. An unmatched declaration

F ReclaimClip              | ControlPanel.h | 158     | api,nonallocating,nonthrowing      | ✔️ | Returns the recording to whatever it was clipped to before the matching declaration.
    out   -  void  [-]  ?
    by    Source/ChannelPanel.cpp, Source/ControlChrome.cpp, Source/DiagnosticPanel.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp, Source/RevisionPanel.cpp

F MeasuredTextExtent       | ControlPanel.h | 163     | api,nonallocating,nonthrowing      | ✔️ | The horizontal extent one run of text would occupy at the declared scale.
    in    Text       const char*  [-]  ?
    in    FontScale  float        [-]  ?
    out   -          float        [-]  ?
    by    Source/ChannelPanel.cpp, Source/ControlChrome.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp

F ResolveAreaPress         | ControlPanel.h | 174     | api,nonallocating,nonthrowing      | ✔️ | A press with no hold over one rectangle — what a panel row, a header glyph or a fold twisty answers. any release covering it would fire when a drag that started elsewhere happened to end over it, and the defect presents as a layer collapsing because a slider release landed on its row. component knows which recording the interface paints on, and the same argument holds for who reads the pointer: a panel with its own copy is a second authority on what "pressed" means.
    in    Area  const WorkspaceRectangle&  [-]  ?
    out   -     ControlInteraction         [-]  ?
    by    Source/ChannelPanel.cpp, Source/ControlChrome.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp
    note  🔴 The release is honoured only where the press **began** over the same rectangle. A panel that acted on
    note  ⚠️ Spelled here so a panel never reads the vendor's pointer itself. `14` §7's seam holds only while one

F ResolvePointerPosition   | ControlPanel.h | 181     | api,nonallocating,nonthrowing      | ✔️ | The pointer this tick, in the same interface pixels every rectangle is spelled in. left the row it began on is precisely the case a per-rectangle answer cannot describe.
    in    PositionX  float&  [-]  ?
    in    PositionY  float&  [-]  ?
    out   -          void    [-]  ?
    by    Source/ControlChrome.cpp, Source/LayerPanel.cpp
    note  What a reorder drag reads. `ResolveAreaPress` reports only what covers one rectangle, and a drag that has

F PointerHeld              | ControlPanel.h | 186     | api,nonallocating,nonthrowing      | ✔️ | Whether the primary control is down this tick, wherever the pointer is.
    out   -  bool  [-]  ?
    by    Source/ControlChrome.cpp, Source/LayerPanel.cpp

F AdvanceVisibleOffset     | ControlPanel.h | 197     | api,nonthrowing                    | ✔️ | Advances a hand-rolled list's visible offset by the wheel, bounded to what the content leaves. scrolled view snaps back to content rather than presenting an empty band under the last row.
    in    Carried        float&                     [px]  the offset, amended in place; zero is the top of the content
    in    Area           const WorkspaceRectangle&  [px]  the viewport the list is clipped to
    in    ContentExtent  float                      [px]  the whole content's height
    out   -              Deliver                    [px]  the bounded offset, delivered so a caller need not read the carry back
    by    Source/ChannelPanel.cpp, Source/ControlChrome.cpp, Source/DiagnosticPanel.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp, Source/RevisionPanel.cpp
    note  ⚠️ Bounded against the content **after** the wheel is applied, so a list that shortens beneath a

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE NUMERIC ENTRIES
//------------------------------------------------------------------------------------------------------------------------

F PresentValueSlider       | ControlPanel.h | 220     | api,nonthrowing                    | 🚩 | A label, a fixed 78 px value box, and a track whose knob sits at the reading's fraction of its span. ExtentExhausted when the field cannot carry the value box and a grabbable track its knob; clamping the span silently instead gives the artist a control that looks live and is not.
    in    Theme     const ThemeSpecification&  [-]   read and never held
    in    Area      const WorkspaceRectangle&  [px]  the whole row, label included
    in    Caption   const char*                [-]   the label column's text
    in    Carried   double&                    [-]   the reading, amended in place when the artist drags or types
    in    Floor     double                     [-]   the low end of the span
    in    Ceiling   double                     [-]   the high end
    in    Unit      const char*                [-]   the unit cap's text; the reference prints a middot where a quantity is dimensionless
    in    Decimals  std::uint32_t              [-]   digits after the point in the readout; zero rounds the reading to an integer
    out   -         Deliver                    [-]   refuses with ContentUnsupported when the ceiling does not exceed the floor, and with
    by    Source/ChannelPanel.cpp, Source/ControlNumeric.cpp, Source/PropertyPanel.cpp
    note  🔴 The refusal on an empty span is the point. A slider over `Floor == Ceiling` divides by zero to place

F PresentScalarEntry       | ControlPanel.h | 237     | api,nonthrowing                    | 🚩 | An unbounded scalar: the same value box, and a centre-knobbed track that accumulates travel rather than mapping position, so the reading may leave any span. readout — a knob that travelled would imply a span this control does not have.
    in    Theme     const ThemeSpecification&  [-]  ?
    in    Area      const WorkspaceRectangle&  [-]  ?
    in    Caption   const char*                [-]  ?
    in    Carried   double&                    [-]  ?
    in    Step      double                     [-]  reading amended per pixel of horizontal travel
    in    Unit      const char*                [-]  ?
    in    Decimals  std::uint32_t              [-]  ?
    out   -         Deliver                    [-]  refuses with ExtentExhausted when the field cannot carry both parts
    by    Source/ControlNumeric.cpp, Source/PropertyPanel.cpp
    note  ⚠️ The knob is painted at the track's centre always and never moves. It is a grab surface and not a

F PresentVectorEntry       | ControlPanel.h | 250     | api,nonthrowing                    | 🚩 | Three value boxes side by side, each capped with its axis letter, each dragging its own component.
    in    Theme     const ThemeSpecification&  [-]  ?
    in    Area      const WorkspaceRectangle&  [-]  ?
    in    Caption   const char*                [-]  ?
    in    Carried   double                     [-]  three components, amended in place
    in    Step      double                     [-]  ?
    in    Decimals  std::uint32_t              [-]  ?
    out   -         Deliver                    [-]  refuses with ExtentExhausted when the field cannot carry three boxes
    by    Source/ControlNumeric.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE CHOICE ENTRIES
//------------------------------------------------------------------------------------------------------------------------

F PresentBooleanEntry      | ControlPanel.h | 266     | api,nonthrowing                    | ✔️ | A 50×32 travel whose nub crosses on a click. holding its own would be retained state — which this file has none of by construction.
    in    Theme    const ThemeSpecification&    [-]  ?
    in    Area     const WorkspaceRectangle&    [-]  ?
    in    Caption  const char*                  [-]  ?
    in    Carried  bool&                        [-]  ?
    out   -        Deliver<ControlInteraction>  [-]  ?
    by    Source/ControlChoice.cpp, Source/PropertyPanel.cpp
    note  The nub's travel is not animated here. `14` §4.1 places animation carry beside the caller, and a control

F PresentSelectionEntry    | ControlPanel.h | 278     | api,nonthrowing                    | 🚩 | A wrapping run of pills, the chosen one filled with the accent and lettered in the on-accent colour.
    in    Theme           const ThemeSpecification&  [-]  ?
    in    Area            const WorkspaceRectangle&  [-]  ?
    in    Caption         const char*                [-]  ?
    in    Choices         const char* const*         [-]  the captions, read and never held
    in    ChoiceCount     std::uint32_t              [-]  how many
    in    CarriedOrdinal  std::uint32_t&             [-]  which is chosen, amended in place
    out   -               Deliver                    [-]  refuses with ContentUnsupported for no choices or an ordinal outside them
    by    Source/ControlChoice.cpp

T DropdownCarry            | ControlPanel.h | 289-295 | owning                             | -  | What a dropdown carries between ticks — its openness and the anchor its list drops from. open together, which is the defect that follows from a control holding openness of its own.
    has   ListOpen    bool           [-]  ?
    has   OpenedTick  std::uint32_t  [-]  ?
    has   AnchorX     float          [-]  ?
    has   AnchorY     float          [-]  ?
    by    Api/ChannelPanel.h, Api/PropertyPanel.h, Source/ControlChoice.cpp
    note  🔴 Owned by the caller and never by the control. Two dropdowns sharing one carry are two dropdowns that

F PresentDropdown          | ControlPanel.h | 304     | api,nonthrowing                    | 🚩 | A 26 px head with a caret cap, and a hand-rolled list beneath it. tab overlays are not: every trapezoid is already on that recording, and a popup sits beneath it.
    in    Theme           const ThemeSpecification&  [-]  ?
    in    Area            const WorkspaceRectangle&  [-]  ?
    in    Choices         const char* const*         [-]  ?
    in    ChoiceCount     std::uint32_t              [-]  ?
    in    CarriedOrdinal  std::uint32_t&             [-]  ?
    in    Carry           DropdownCarry&             [-]  ?
    in    PresentedTick   std::uint32_t              [-]  the desk's own presentation count, compared against the carry's opening tick
    out   -               Deliver                    [-]  refuses with ContentUnsupported for no choices or an ordinal outside them
    by    Source/ControlChoice.cpp, Source/PropertyPanel.cpp
    note  🔴 The list is painted on the foreground recording and is **not** a vendor popup, for the same reason the

F PresentSegmentRow        | ControlPanel.h | 317     | api,nonthrowing                    | 🚩 | A 26 px pill row — the reference's `.seg`, filled when it is on. choice, so each pill carries its own boolean and pressing one does not clear the others.
    in    Theme         const ThemeSpecification&    [-]  ?
    in    Area          const WorkspaceRectangle&    [-]  ?
    in    Captions      const char* const*           [-]  ?
    in    Carried       bool*                        [-]  ?
    in    SegmentCount  std::uint32_t                [-]  ?
    out   -             Deliver<ControlInteraction>  [-]  ?
    by    Source/ControlChoice.cpp
    note  Distinct from a selection entry: a segment row is a run of independent switches, not one exclusive

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE TEXT ENTRIES
//------------------------------------------------------------------------------------------------------------------------

V ControlTextExtent        | ControlPanel.h | 329     | -                                  | -  | ?
    by    Source/ControlText.cpp, Source/PropertyPanel.cpp

T TextCarry                | ControlPanel.h | 335-341 | owning                             | -  | One open text edit — the edited text, never the committed one. never written during the edit, so an abandoned rename leaves no trace and a refused seal leaves none.
    has   EditOpen       bool                     [-]  ?
    has   Carried        char[ControlTextExtent]  [-]  ?
    has   CarryExtent    std::uint32_t            [-]  ?
    has   CaretPosition  std::uint32_t            [-]  ?
    by    Api/LayerPanel.h, Api/PropertyPanel.h, Source/ControlText.cpp, Source/PropertyPanel.cpp
    note  🔴 Sealing writes the carry back to whatever owns the text; abandoning discards it. The owner's text is

F PresentTextEntry         | ControlPanel.h | 347     | api,nonthrowing                    | 🚩 | A rounded field the artist types into, opened by a click and sealed by Enter or by a press elsewhere.
    in    Theme        const ThemeSpecification&  [-]  ?
    in    Area         const WorkspaceRectangle&  [-]  ?
    in    Caption      const char*                [-]  ?
    in    Carry        TextCarry&                 [-]  ?
    in    Placeholder  const char*                [-]  ?
    out   -            Deliver                    [-]  refuses with ExtentExhausted when the field is narrower than one glyph
    by    Source/ControlText.cpp, Source/LayerPanel.cpp, Source/PropertyPanel.cpp

F PresentInlineTextEditor  | ControlPanel.h | 358     | api,nonthrowing                    | 🚩 | A text edit with no field of its own, painted over whatever it is renaming. the trapezoid beneath it stays visible and the artist can see what is being renamed.
    in    Theme  const ThemeSpecification&    [-]  ?
    in    Area   const WorkspaceRectangle&    [-]  ?
    in    Carry  TextCarry&                   [-]  ?
    out   -      Deliver<ControlInteraction>  [-]  ?
    by    Source/ControlText.cpp
    note  What a tab's double-click rename rides on. It paints a caret and an accent hairline and nothing else, so

F PresentPathEntry         | ControlPanel.h | 368     | api,nonthrowing                    | 🚩 | A path field and the round browse cap beside it. chooser itself would be a panel holding what it presents.
    in    Theme           const ThemeSpecification&  [-]  ?
    in    Area            const WorkspaceRectangle&  [-]  ?
    in    Caption         const char*                [-]  ?
    in    Carry           TextCarry&                 [-]  ?
    in    BrowseDeclared  bool&                      [-]  ?
    out   -               Deliver                    [-]  the interaction; `EditSealed` on the browse cap means the caller should open a chooser
    by    Source/ControlText.cpp
    note  ⚠️ This control never touches the file system. `04`'s interchange owns that, and a control that opened a

//------------------------------------------------------------------------------------------------------------------------
//                                                THE REMAINING PRIMITIVES
//------------------------------------------------------------------------------------------------------------------------

F PresentColourEntry       | ControlPanel.h | 385     | api,nonthrowing                    | 🚩 | A fully rounded bar carrying a swatch disc, the coordinate printed, and a caret cap. never spells a transfer — `14` §5 places the interface after the display projection, and a control that converted here would be the second transfer the whole arrangement exists to prevent.
    in    Theme       const ThemeSpecification&    [-]  ?
    in    Area        const WorkspaceRectangle&    [-]  ?
    in    Caption     const char*                  [-]  ?
    in    Carried     ThemeColour&                 [-]  the colour, amended in place; its space is carried through untouched
    in    PickerOpen  bool&                        [-]  ?
    out   -           Deliver<ControlInteraction>  [-]  ?
    by    Source/ChannelPanel.cpp, Source/ControlChrome.cpp, Source/PropertyPanel.cpp
    note  🔴 `36` §1: the coordinate keeps its declared space across the edit. This control never projects and

F PresentMenuPill          | ControlPanel.h | 394     | api,nonthrowing                    | ✔️ | A caption pill that reads as pressable — what a menu band is a run of.
    in    Theme        const ThemeSpecification&    [-]  ?
    in    Area         const WorkspaceRectangle&    [-]  ?
    in    Caption      const char*                  [-]  ?
    in    Highlighted  bool                         [-]  ?
    out   -            Deliver<ControlInteraction>  [-]  ?
    by    Source/ControlChrome.cpp, Source/DiagnosticPanel.cpp, Source/RevisionPanel.cpp

F PresentGlyphButton       | ControlPanel.h | 409     | api,nonthrowing                    | ✔️ | A square glyph that answers a press — the uploaded glyph when a depot slot is named, the stroke otherwise. painting nothing at all — a missing icon that still answers a press is recoverable, an invisible button is not. The stroke is therefore always supplied, even where a slot is expected to resolve. already documents it as an integer whose meaning only its own source knows.
    in    Theme        const ThemeSpecification&    [-]  ?
    in    Area         const WorkspaceRectangle&    [-]  ?
    in    Stroke       ControlStroke                [-]  what is painted when no slot is named, or when the named one has been reclaimed
    in    DepotSlot    std::uint64_t                [-]  an opaque `GlyphHandle::DepotSlot`; zero falls back to the stroke
    in    Highlighted  bool                         [-]  ?
    out   -            Deliver<ControlInteraction>  [-]  ?
    by    Source/ControlChrome.cpp
    note  🔴 The fallback is not a convenience. A depot that reclaimed a tier mid-session must not leave a panel
    note  ⚠️ The slot crosses as a bare integer so that no vendor spelling enters this header. `GlyphDepot.h`

F PresentSectionHeader     | ControlPanel.h | 419     | api,nonthrowing                    | ✔️ | A 29 px accordion header with a twisty that turns a quarter as it opens.
    in    Theme        const ThemeSpecification&    [-]  ?
    in    Area         const WorkspaceRectangle&    [-]  ?
    in    Caption      const char*                  [-]  ?
    in    SectionOpen  bool&                        [-]  amended in place by a press anywhere on the header
    in    Trailing     const char*                  [-]  ?
    out   -            Deliver<ControlInteraction>  [-]  ?
    by    Source/ChannelPanel.cpp, Source/ControlChrome.cpp, Source/DiagnosticPanel.cpp

T CarouselCarry            | ControlPanel.h | 427-432 | owning                             | -  | What a carousel carries between ticks — which pane is presented and how far the slide has travelled.
    has   PresentedPane  std::uint32_t  [-]  ?
    has   ArrivingPane   std::uint32_t  [-]  ?
    has   Travelled      float          [-]  ?
    by    Source/ControlChrome.cpp

F AdvanceContentCarousel   | ControlPanel.h | 442     | api,nonthrowing                    | ✔️ | Advances a carousel's slide and reports the horizontal offset each pane paints at. body width further along in the direction of travel caller clips its own body — a carousel that clipped for its caller would need to know what a pane is.
    in    Theme            const ThemeSpecification&  [-]   ?
    in    Carry            CarouselCarry&             [-]   ?
    in    ElapsedInterval  float                      [s]   since the previous tick, from the tick's own clock
    out   -                Deliver                    [px]  the offset to add to the presented pane's origin; the arriving pane sits one
    by    Source/ControlChrome.cpp
    note  📝 `84`'s Properties/History pair rides this. The offset is returned rather than applied so that the

F SLATE_DECLARES_PRECISION | ControlPanel.h | 448     | -                                  | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Exact    PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)
