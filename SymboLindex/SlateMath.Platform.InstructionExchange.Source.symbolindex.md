//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The host query, the operating-system consent a wide specialisation additionally needs, and the selection.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Platform/InstructionExchange/Source
%layer      SlateMath
%sources    1
%symbols    11
%annotated  0/11
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S InstructionExchange.cpp | 239 lines | c451bc58 | 11 sym | The host query, the operating-system consent a wide specialisation additionally needs, and the selection.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

K SLATE_INSTRUCTION_QUERY      | InstructionExchange.cpp | 12      | - | - | ?

K SLATE_INSTRUCTION_QUERY      | InstructionExchange.cpp | 14      | - | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE HOST QUERY
//------------------------------------------------------------------------------------------------------------------------

F QueryHost                    | InstructionExchange.cpp | 39-50   | - | - | ?
    in    Leaf      std::uint32_t  [-]  ?
    in    Subleaf   std::uint32_t  [-]  ?
    in    Reported  std::uint32_t  [-]  ?
    out   -         void           [-]  ?

F ExtendedConsent              | InstructionExchange.cpp | 56-68   | - | - | ?
    out   -  std::uint64_t  [-]  ?

F ResolveSupported             | InstructionExchange.cpp | 72-135  | - | - | ?
    out   -  InstructionWidth  [-]  ?

F ResolveCacheLine             | InstructionExchange.cpp | 137-161 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F StandingReport               | InstructionExchange.cpp | 166-183 | - | - | ?
    out   -  InstructionReport&  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE SELECTION
//------------------------------------------------------------------------------------------------------------------------

F InstructionExchange::Report  | InstructionExchange.cpp | 191-194 | - | - | ?
    out   -  const InstructionReport&  [-]  ?

F InstructionExchange::Fix     | InstructionExchange.cpp | 196-210 | - | - | ?
    in    Fixed  InstructionWidth  [-]  ?
    out   -      InstructionWidth  [-]  ?

F InstructionExchange::Release | InstructionExchange.cpp | 212-218 | - | - | ?
    out   -  void  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT IS NAMED
//------------------------------------------------------------------------------------------------------------------------

F InstructionExchange::Naming  | InstructionExchange.cpp | 224-237 | - | - | ?
    in    Reported  InstructionWidth  [-]  ?
    out   -         const char*       [-]  ?
