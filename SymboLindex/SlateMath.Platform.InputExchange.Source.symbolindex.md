//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Bounded cyclic arrival ordering over pointer samples.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Platform/InputExchange/Source
%layer      SlateMath
%sources    1
%symbols    4
%annotated  0/4
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S InputExchange.cpp | 52 lines | 148be718 | 4 sym | Bounded cyclic arrival ordering over pointer samples.

//------------------------------------------------------------------------------------------------------------------------
//                                                        ARRIVAL
//------------------------------------------------------------------------------------------------------------------------

F InputExchange::Record    | InputExchange.cpp | 15-30 | - | - | ?
    in    Arriving  const PointerSample&  [-]  ?
    out   -         void                  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                         DRAIN
//------------------------------------------------------------------------------------------------------------------------

F InputExchange::Sample    | InputExchange.cpp | 36-39 | - | - | ?
    in    ArrivalOrdinal  std::uint32_t         [-]  ?
    out   -               const PointerSample&  [-]  ?

F InputExchange::HeldCount | InputExchange.cpp | 41-44 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F InputExchange::Reclaim   | InputExchange.cpp | 46-50 | - | - | ?
    out   -  void  [-]  ?
