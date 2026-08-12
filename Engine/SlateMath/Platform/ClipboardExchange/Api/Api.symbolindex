//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Text and imagery crossing to and from the operating system, taken as a copy that outlives what supplied it.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Platform/ClipboardExchange/Api
%layer      SlateMath
%sources    1
%symbols    8
%annotated  8/8
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ClipboardExchange.h | 104 lines | 9dbd391c | 8 sym | Text and imagery crossing to and from the operating system, taken as a copy that outlives what supplied it.

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT IS CARRIED
//------------------------------------------------------------------------------------------------------------------------

T ClipboardImage                  | ClipboardExchange.h | 28-33  | owning                        | -  | One image taken from or handed to the host clipboard, unpremultiplied and eight bits per component. statement about its alpha, and a premultiplied supply read as unpremultiplied darkens every partly transparent texel — which reads as the imagery being wrong rather than as the convention being unsaid. narrowed by whoever supplies it; nothing here widens a narrow one back and calls it precision.
    has   Texels  std::vector<std::uint8_t>  [-]  ?
    has   Width   std::uint32_t              [-]  ?
    has   Height  std::uint32_t              [-]  ?
    by    Source/ClipboardExchange.cpp
    note  🔴 Unpremultiplied, and stated rather than assumed. The host clipboard's own image carries no
    note  📝 Eight bits per component because that is what all three host clipboards carry. A wider source is

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE EXCHANGE
//------------------------------------------------------------------------------------------------------------------------

T ClipboardExchange               | ClipboardExchange.h | 47-102 | owning                        | -  | The one place clipboard content crosses the operating-system edge. nothing depends on a clipboard surviving — the artist copies something else a second later, and a source holding a reference into the host clipboard would then describe content nobody supplied. narrowing happens here rather than at each caller: a caller that narrowed it itself would narrow it through the process code page, which silently drops every character outside it.
    has   ImageTexelCeiling  static constexpr std::uint64_t  [-]  ?
    by    Source/ClipboardExchange.cpp
    note  🔴 Every read returns a **copy**. `52` §5 gates that a supplied-text source stores its text and that
    note  ⚠️ Text crosses as UTF-8 in both directions. Two of the three hosts carry wide text natively, so the

F ClipboardExchange::ReadText     | ClipboardExchange.h | 58     | api,nonthrowing               | 🚩 | Reads the host clipboard's text. HostDenied when the host declines to open it operation and `86` would otherwise report the artist's own empty clipboard as an OS failure.
    out   -  Outcome  [-]  refuses with CapabilityAbsent when the clipboard carries no text at all, and with
    by    Source/ClipboardExchange.cpp
    note  📝 Carrying no text is CapabilityAbsent rather than HostDenied. An empty clipboard is ordinary

F ClipboardExchange::WriteText    | ClipboardExchange.h | 65     | api,nonthrowing               | 🚩 | Hands text to the host clipboard, replacing whatever it carried.
    in    Supplied  const std::string&  [-]  UTF-8; an empty supply clears the clipboard rather than refusing
    out   -         Outcome             [-]  refuses with HostDenied when the host declines
    by    Source/ClipboardExchange.cpp

F ClipboardExchange::ReadImage    | ClipboardExchange.h | 76     | api,nonthrowing               | 🔴 | Reads the host clipboard's imagery. ContentUnsupported for a layout this translation does not read, and with HostDenied when the host declines to open it stores its rows bottom-up under a positive height and top-down under a negative one, and a reader that ignored the sign delivers half of all clipboard imagery vertically mirrored.
    out   -  Outcome  [-]  refuses with CapabilityAbsent when the clipboard carries no imagery, with
    by    Source/ClipboardExchange.cpp
    note  🔴 The rows are delivered top-down regardless of how the host stored them. The Windows clipboard

F ClipboardExchange::WriteImage   | ClipboardExchange.h | 84     | api,nonthrowing               | 🔴 | Hands imagery to the host clipboard, replacing whatever it carried. with HostDenied when the host declines
    in    Supplied  const ClipboardImage&  [-]  row order, top-down, RGBA, unpremultiplied
    out   -         Outcome                [-]  refuses with ContentUnsupported when the texel extent is not the stated extent, and
    by    Source/ClipboardExchange.cpp

F ClipboardExchange::TextCarried  | ClipboardExchange.h | 90     | api,nonallocating,nonthrowing | ✔️ | Whether the host clipboard currently carries text this translation can read.
    out   -  TextCarried  [-]  false while the host declines to be asked at all
    by    Source/ClipboardExchange.cpp

F ClipboardExchange::ImageCarried | ClipboardExchange.h | 96     | api,nonallocating,nonthrowing | ✔️ | Whether the host clipboard currently carries imagery this translation can read.
    out   -  ImageCarried  [-]  false while the host declines to be asked at all
    by    Source/ClipboardExchange.cpp
