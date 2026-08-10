//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The layout every program reaches through, the two construction routes, and the reclamation that returns both.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/ProgramIndex/Source
%layer      SlateVulkan
%sources    1
%symbols    8
%annotated  0/8
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ProgramIndex.cpp | 333 lines | 3d95ed1b | 8 sym | The layout every program reaches through, the two construction routes, and the reclamation that returns both.

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F ProgramIndex::Construct       | ProgramIndex.cpp | 15-27   | -          | - | ?
    in    Exchange     const VulkanExchange&   [-]  ?
    in    Modules      ShaderCodec&            [-]  ?
    in    Descriptors  const DescriptorIndex&  [-]  ?
    out   -            Outcome<bool>           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE REACH
//------------------------------------------------------------------------------------------------------------------------

F ProgramIndex::ReachLayout     | ProgramIndex.cpp | 33-71   | -          | - | ?
    in    LayoutOrdinals  const std::vector<std::uint32_t>&  [-]  ?
    in    ConstantBytes   std::uint32_t                      [-]  ?
    in    ReachingStages  VkShaderStageFlags                 [-]  ?
    out   -               Outcome<VkPipelineLayout>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE GRAPHICS ROUTE
//------------------------------------------------------------------------------------------------------------------------

F ProgramIndex::DeclareGraphics | ProgramIndex.cpp | 77-221  | -          | - | ?
    in    Declaring  const GraphicsDeclaration&  [-]  ?
    out   -          Outcome<std::uint32_t>      [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE COMPUTE ROUTE
//------------------------------------------------------------------------------------------------------------------------

F ProgramIndex::DeclareCompute  | ProgramIndex.cpp | 227-272 | -          | - | ?
    in    Declaring  const ComputeDeclaration&  [-]  ?
    out   -          Outcome<std::uint32_t>     [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

F ProgramIndex::Resolve         | ProgramIndex.cpp | 278-294 | -          | - | ?
    in    ProgramOrdinal  std::uint32_t                [-]  ?
    out   -               Outcome<ConstructedProgram>  [-]  ?

F ProgramIndex::DeclaredCount   | ProgramIndex.cpp | 296-299 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F ProgramIndex::Reclaim         | ProgramIndex.cpp | 305-326 | -          | - | ?
    out   -  void  [-]  ?

F ProgramIndex::~ProgramIndex   | ProgramIndex.cpp | 328-331 | destructor | - | ?
