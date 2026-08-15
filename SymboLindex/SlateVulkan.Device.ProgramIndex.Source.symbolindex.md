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

S ProgramIndex.cpp | 360 lines | 8bc78613 | 8 sym | The layout every program reaches through, the two construction routes, and the reclamation that returns both.

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F ProgramIndex::Construct       | ProgramIndex.cpp | 15-29   | -          | - | ?
    in    Exchange     const VulkanExchange&       [-]  ?
    in    Modules      ShaderCodec&                [-]  ?
    in    Descriptors  const DescriptorIndex&      [-]  ?
    in    Naming       const DiagnosticExtension&  [-]  ?
    out   -            Deliver<bool>               [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE REACH
//------------------------------------------------------------------------------------------------------------------------

F ProgramIndex::ReachLayout     | ProgramIndex.cpp | 35-73   | -          | - | ?
    in    LayoutOrdinals  const std::vector<std::uint32_t>&  [-]  ?
    in    ConstantBytes   std::uint32_t                      [-]  ?
    in    ReachingStages  VkShaderStageFlags                 [-]  ?
    out   -               Deliver<VkPipelineLayout>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE GRAPHICS ROUTE
//------------------------------------------------------------------------------------------------------------------------

F ProgramIndex::DeclareGraphics | ProgramIndex.cpp | 79-236  | -          | - | ?
    in    Declaring  const GraphicsDeclaration&  [-]  ?
    out   -          Deliver<std::uint32_t>      [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE COMPUTE ROUTE
//------------------------------------------------------------------------------------------------------------------------

F ProgramIndex::DeclareCompute  | ProgramIndex.cpp | 242-299 | -          | - | ?
    in    Declaring  const ComputeDeclaration&  [-]  ?
    out   -          Deliver<std::uint32_t>     [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

F ProgramIndex::Resolve         | ProgramIndex.cpp | 305-321 | -          | - | ?
    in    ProgramOrdinal  std::uint32_t                [-]  ?
    out   -               Deliver<ConstructedProgram>  [-]  ?

F ProgramIndex::DeclaredCount   | ProgramIndex.cpp | 323-326 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F ProgramIndex::Reclaim         | ProgramIndex.cpp | 332-353 | -          | - | ?
    out   -  void  [-]  ?

F ProgramIndex::~ProgramIndex   | ProgramIndex.cpp | 355-358 | destructor | - | ?
