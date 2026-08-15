//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Contribution gating and the ordering derived from declared reads and writes.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/RenderSchedule/Source
%layer      SlateVulkan
%sources    1
%symbols    10
%annotated  0/10
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S RenderSchedule.cpp | 490 lines | 198ca8e2 | 10 sym | Contribution gating and the ordering derived from declared reads and writes.

//------------------------------------------------------------------------------------------------------------------------
//                                                   TARGET DECLARATION
//------------------------------------------------------------------------------------------------------------------------

F RelationOfTarget           | RenderSchedule.cpp | 115-122 | - | - | ?
    in    Target  SharedTarget    [-]  ?
    out   -       ExtentRelation  [-]  ?
    by    Api/RenderSchedule.h

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE SHAPES
//------------------------------------------------------------------------------------------------------------------------

F TargetSpace::ShapeOf       | RenderSchedule.cpp | 128-183 | - | - | ?
    in    Target  SharedTarget         [-]  ?
    out   -       Deliver<ImageShape>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE CLAIM
//------------------------------------------------------------------------------------------------------------------------

F TargetSpace::Claim         | RenderSchedule.cpp | 189-241 | - | - | ?
    in    Images         ImageSpace&    [-]  ?
    in    DisplayWidth   std::uint32_t  [-]  ?
    in    DisplayHeight  std::uint32_t  [-]  ?
    in    DisplayFormat  VkFormat       [-]  ?
    out   -              Deliver<bool>  [-]  ?

F TargetSpace::Reclaim       | RenderSchedule.cpp | 243-297 | - | - | ?
    in    DisplayWidth   std::uint32_t  [-]  ?
    in    DisplayHeight  std::uint32_t  [-]  ?
    out   -              Deliver<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT IS CLAIMED
//------------------------------------------------------------------------------------------------------------------------

F TargetSpace::Resolve       | RenderSchedule.cpp | 303-311 | - | - | ?
    in    Target  SharedTarget         [-]  ?
    out   -       Deliver<ImageClaim>  [-]  ?

F TargetSpace::OrdinalOf     | RenderSchedule.cpp | 313-324 | - | - | ?
    in    Target  SharedTarget            [-]  ?
    out   -       Deliver<std::uint32_t>  [-]  ?

F TargetSpace::Surrender     | RenderSchedule.cpp | 326-350 | - | - | ?
    out   -  void  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONTRIBUTION
//------------------------------------------------------------------------------------------------------------------------

F RenderSchedule::Contribute | RenderSchedule.cpp | 356-390 | - | - | ?
    in    Arriving  const DeclaredRecording&  [-]  ?
    out   -         Deliver<bool>             [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        ORDERING
//------------------------------------------------------------------------------------------------------------------------

F RenderSchedule::Fix        | RenderSchedule.cpp | 396-483 | - | - | ?
    out   -  Deliver<bool>  [-]  ?

F RenderSchedule::Ordered    | RenderSchedule.cpp | 485-488 | - | - | ?
    out   -  const std::vector<DeclaredRecording>&  [-]  ?
