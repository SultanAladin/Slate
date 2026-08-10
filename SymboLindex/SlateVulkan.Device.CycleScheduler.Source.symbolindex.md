//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The ordering points of every cyclic slot, the bounded wait that reclaims one, and the advance that cycles them.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/CycleScheduler/Source
%layer      SlateVulkan
%sources    1
%symbols    9
%annotated  0/9
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S CycleScheduler.cpp | 175 lines | 4fc315b6 | 9 sym | The ordering points of every cyclic slot, the bounded wait that reclaims one, and the advance that cycles them.

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F CycleScheduler::Construct          | CycleScheduler.cpp | 15-57   | -          | - | ?
    in    Exchange  const VulkanExchange&  [-]  ?
    out   -         Outcome<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE WAIT
//------------------------------------------------------------------------------------------------------------------------

F CycleScheduler::Await              | CycleScheduler.cpp | 63-87   | -          | - | ?
    out   -  Outcome<bool>  [-]  ?

F CycleScheduler::Arm                | CycleScheduler.cpp | 89-101  | -          | - | ?
    out   -  Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE CYCLE
//------------------------------------------------------------------------------------------------------------------------

F CycleScheduler::Advance            | CycleScheduler.cpp | 107-114 | -          | - | ?
    out   -  void  [-]  ?

F CycleScheduler::Standing           | CycleScheduler.cpp | 116-122 | -          | - | ?
    out   -  Outcome<RotationSlot>  [-]  ?

F CycleScheduler::StandingOrdinal    | CycleScheduler.cpp | 124-127 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F CycleScheduler::CompletedRotations | CycleScheduler.cpp | 129-132 | -          | - | ?
    out   -  std::uint64_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F CycleScheduler::Reclaim            | CycleScheduler.cpp | 138-168 | -          | - | ?
    out   -  void  [-]  ?

F CycleScheduler::~CycleScheduler    | CycleScheduler.cpp | 170-173 | destructor | - | ?
