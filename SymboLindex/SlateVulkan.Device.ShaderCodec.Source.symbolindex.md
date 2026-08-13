//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The whole-file read, the stream verification that refuses before the vendor sees it, and the held specialisation.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/ShaderCodec/Source
%layer      SlateVulkan
%sources    1
%symbols    7
%annotated  0/7
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ShaderCodec.cpp | 229 lines | a457180e | 7 sym | The whole-file read, the stream verification that refuses before the vendor sees it, and the held specialisation.

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F ShaderCodec::Construct     | ShaderCodec.cpp | 16-30   | -          | - | ?
    in    Exchange         const VulkanExchange&  [-]  ?
    in    StreamDirectory  const std::string&     [-]  ?
    out   -                Outcome<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE READ
//------------------------------------------------------------------------------------------------------------------------

F ShaderCodec::ReadStream    | ShaderCodec.cpp | 36-88   | -          | - | ?
    in    StreamPath  const std::string&                   [-]  ?
    out   -           Outcome<std::vector<std::uint32_t>>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

F ShaderCodec::Resolve       | ShaderCodec.cpp | 94-139  | -          | - | ?
    in    UnitName    const std::string&      [-]  ?
    in    StreamStem  const std::string&      [-]  ?
    out   -           Outcome<std::uint32_t>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE STAGE
//------------------------------------------------------------------------------------------------------------------------

F ShaderCodec::Stage         | ShaderCodec.cpp | 145-195 | -          | - | ?
    in    ModuleOrdinal  std::uint32_t                             [-]  ?
    in    Reading        VkShaderStageFlagBits                     [-]  ?
    in    Fixed          const std::vector<SpecialisedConstant>&   [-]  ?
    out   -              Outcome<VkPipelineShaderStageCreateInfo>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F ShaderCodec::ResolvedCount | ShaderCodec.cpp | 201-204 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F ShaderCodec::Reclaim       | ShaderCodec.cpp | 206-222 | -          | - | ?
    out   -  void  [-]  ?

F ShaderCodec::~ShaderCodec  | ShaderCodec.cpp | 224-227 | destructor | - | ?
