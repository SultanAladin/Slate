//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Tool declaration, the arbitration both units ask, and the capture that persists for a whole drag.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/ToolSequence/Source
%layer      SlateDocument
%sources    1
%symbols    27
%annotated  0/27
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ToolSequence.cpp | 247 lines | bad22a6b | 27 sym | Tool declaration, the arbitration both units ask, and the capture that persists for a whole drag.

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE TOOLS
//------------------------------------------------------------------------------------------------------------------------

F ToolIndex::Declare               | ToolSequence.cpp | 15-48   | - | - | ?
    in    Declaring  const ToolSpecification&  [-]  ?
    out   -          Deliver<std::uint32_t>    [-]  ?

F ToolIndex::Resolve               | ToolSequence.cpp | 50-56   | - | - | ?
    in    ToolOrdinal  std::uint32_t                      [-]  ?
    out   -            Deliver<const ToolSpecification*>  [-]  ?

F ToolIndex::Amend                 | ToolSequence.cpp | 58-64   | - | - | ?
    in    ToolOrdinal  std::uint32_t                [-]  ?
    out   -            Deliver<ToolSpecification*>  [-]  ?

F ToolIndex::Located               | ToolSequence.cpp | 66-75   | - | - | ?
    in    Identity  const std::string&      [-]  ?
    out   -         Deliver<std::uint32_t>  [-]  ?

F ToolIndex::DeclaredCount         | ToolSequence.cpp | 77-80   | - | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DECLARATIONS
//------------------------------------------------------------------------------------------------------------------------

F ToolSequence::Tools              | ToolSequence.cpp | 86      | - | - | ?
    out   -  ToolIndex&  [-]  ?

F ToolSequence::Tools              | ToolSequence.cpp | 87      | - | - | ?
    out   -  const ToolIndex&  [-]  ?

F ToolSequence::Brushes            | ToolSequence.cpp | 88      | - | - | ?
    out   -  BrushIndex&  [-]  ?

F ToolSequence::Brushes            | ToolSequence.cpp | 89      | - | - | ?
    out   -  const BrushIndex&  [-]  ?

F ToolSequence::DeclareTool        | ToolSequence.cpp | 91-99   | - | - | ?
    in    ToolOrdinal_  std::uint32_t  [-]  ?
    out   -             Deliver<bool>  [-]  ?

F ToolSequence::DeclareBrush       | ToolSequence.cpp | 101-109 | - | - | ?
    in    BrushOrdinal_  std::uint32_t  [-]  ?
    out   -              Deliver<bool>  [-]  ?

F ToolSequence::DeclareColour      | ToolSequence.cpp | 111-121 | - | - | ?
    in    Declaring  const ColourSpecification&  [-]  ?
    out   -          Deliver<bool>               [-]  ?

F ToolSequence::DeclareDisplay     | ToolSequence.cpp | 123-131 | - | - | ?
    in    Declaring  DisplaySubject  [-]  ?
    out   -          Deliver<bool>   [-]  ?

F ToolSequence::DeclareChannel     | ToolSequence.cpp | 133-141 | - | - | ?
    in    Declaring  ChannelSubject  [-]  ?
    out   -          Deliver<bool>   [-]  ?

F ToolSequence::DeclareOverlay     | ToolSequence.cpp | 143-151 | - | - | ?
    in    Declaring        OverlaySubject  [-]  ?
    in    PresenceEnabled  bool            [-]  ?
    out   -                Deliver<bool>   [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  POINTER ARBITRATION
//------------------------------------------------------------------------------------------------------------------------

F ToolSequence::Arbitrate          | ToolSequence.cpp | 157-177 | - | - | ?
    in    InterfaceReported  bool               [-]  ?
    in    ManipulatorOpen    bool               [-]  ?
    in    StrokeOpen         bool               [-]  ?
    out   -                  PointerPrecedence  [-]  ?

F ToolSequence::OpenCapture        | ToolSequence.cpp | 179-195 | - | - | ?
    in    Claiming  PointerPrecedence       [-]  ?
    in    Opened    const ResolvedPointer&  [-]  ?
    out   -         Deliver<bool>           [-]  ?

F ToolSequence::ReleaseCapture     | ToolSequence.cpp | 197-205 | - | - | ?
    out   -  Deliver<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

F ToolSequence::Colour             | ToolSequence.cpp | 211     | - | - | ?
    out   -  const ColourSpecification&  [-]  ?

F ToolSequence::Capture            | ToolSequence.cpp | 212     | - | - | ?
    out   -  const PointerCapture&  [-]  ?

F ToolSequence::ActiveTool         | ToolSequence.cpp | 214-223 | - | - | ?
    out   -  Deliver<const ToolSpecification*>  [-]  ?

F ToolSequence::ActiveBrush        | ToolSequence.cpp | 225-234 | - | - | ?
    out   -  Deliver<const BrushSpecification*>  [-]  ?

F ToolSequence::ActiveToolOrdinal  | ToolSequence.cpp | 236     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F ToolSequence::ActiveBrushOrdinal | ToolSequence.cpp | 237     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F ToolSequence::Display            | ToolSequence.cpp | 238     | - | - | ?
    out   -  DisplaySubject  [-]  ?

F ToolSequence::IsolatedChannel    | ToolSequence.cpp | 239     | - | - | ?
    out   -  ChannelSubject  [-]  ?

F ToolSequence::OverlayStanding    | ToolSequence.cpp | 241-245 | - | - | ?
    in    Subject  OverlaySubject  [-]  ?
    out   -        bool            [-]  ?
