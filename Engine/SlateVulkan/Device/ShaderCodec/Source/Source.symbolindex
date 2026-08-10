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

S ShaderCodec.cpp | 240 lines | 479060c9 | 7 sym | The whole-file read, the stream verification that refuses before the vendor sees it, and the held specialisation.

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F ShaderCodec::Construct     | ShaderCodec.cpp | 17-31   | -          | - | ?
    in    Exchange         const VulkanExchange&  [-]  ?
    in    StreamDirectory  const std::string&     [-]  ?
    out   -                Outcome<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE READ
//------------------------------------------------------------------------------------------------------------------------

F ShaderCodec::ReadStream    | ShaderCodec.cpp | 37-99   | -          | - | ?
    in    StreamPath  const std::string&                   [-]  ?
    out   -           Outcome<std::vector<std::uint32_t>>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

F ShaderCodec::Resolve       | ShaderCodec.cpp | 105-150 | -          | - | ?
    in    UnitName    const std::string&      [-]  ?
    in    StreamStem  const std::string&      [-]  ?
    out   -           Outcome<std::uint32_t>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE STAGE
//------------------------------------------------------------------------------------------------------------------------

F ShaderCodec::Stage         | ShaderCodec.cpp | 156-206 | -          | - | ?
    in    ModuleOrdinal  std::uint32_t                             [-]  ?
    in    Reading        VkShaderStageFlagBits                     [-]  ?
    in    Fixed          const std::vector<SpecialisedConstant>&   [-]  ?
    out   -              Outcome<VkPipelineShaderStageCreateInfo>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F ShaderCodec::ResolvedCount | ShaderCodec.cpp | 212-215 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F ShaderCodec::Reclaim       | ShaderCodec.cpp | 217-233 | -          | - | ?
    out   -  void  [-]  ?

F ShaderCodec::~ShaderCodec  | ShaderCodec.cpp | 235-238 | destructor | - | ?
