//============================================================================================================================================
//                                                             SHADERCODEC.CPP
//============================================================================================================================================
// 🧩 The whole-file read, the stream verification that refuses before the vendor sees it, and the held specialisation.

#include "SlateVulkan/Device/ShaderCodec/Api/ShaderCodec.h"

#include <cstdio>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                     CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> ShaderCodec::Construct(const VulkanExchange& Exchange, const std::string& StreamDirectory)
{
    if (Exchange.ActiveDevice() == VK_NULL_HANDLE)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no device is active" });

    DeviceEdge = &Exchange;
    StreamRoot = StreamDirectory;

    // 📝 A trailing separator is appended once here rather than at every path assembly below, so that a caller
    //    passing either spelling reaches the same file.
    if (!StreamRoot.empty() && StreamRoot.back() != '\\' && StreamRoot.back() != '/')
        StreamRoot.push_back('\\');

    return Outcome<bool>::Deliver(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE READ
//------------------------------------------------------------------------------------------------------------------------

Outcome<std::vector<std::uint32_t>> ShaderCodec::ReadStream(const std::string& StreamPath) const
{
    // 🚧 Read through the C stream surface rather than `04` §1's `FileInterchange`, which is declared and not
    //    yet built. This is a whole-file read of a build product with no seek, no partial read and no
    //    encoding, so it carries over to that surface unchanged the moment it exists.
    std::FILE* Stream = nullptr;

#if defined(_MSC_VER)
    if (::fopen_s(&Stream, StreamPath.c_str(), "rb") != 0)
        Stream = nullptr;
#else
    Stream = std::fopen(StreamPath.c_str(), "rb");
#endif

    if (Stream == nullptr)
    {
        return Outcome<std::vector<std::uint32_t>>::Refuse(
            { RefusalReason::HostDenied, "the lowered stream could not be opened; was the shader stage run" });
    }

    std::fseek(Stream, 0, SEEK_END);

    const long Spanned = std::ftell(Stream);

    std::fseek(Stream, 0, SEEK_SET);

    if (Spanned <= 0)
    {
        std::fclose(Stream);
        return Outcome<std::vector<std::uint32_t>>::Refuse(
            { RefusalReason::ContentUnsupported, "the lowered stream is empty" });
    }

    // 🔴 A whole word count or nothing. SPIR-V is a run of 32-bit words by definition, and a stream whose
    //    length is not a multiple of four was truncated — the vendor reads the partial word as an instruction
    //    and reports a malformed module, which names the driver rather than the truncated file.
    if ((static_cast<std::size_t>(Spanned) % sizeof(std::uint32_t)) != 0u)
    {
        std::fclose(Stream);
        return Outcome<std::vector<std::uint32_t>>::Refuse(
            { RefusalReason::ContentUnsupported, "the lowered stream is not a whole count of words" });
    }

    std::vector<std::uint32_t> Words(static_cast<std::size_t>(Spanned) / sizeof(std::uint32_t), 0u);

    const std::size_t Read = std::fread(Words.data(), 1u, static_cast<std::size_t>(Spanned), Stream);

    std::fclose(Stream);

    if (Read != static_cast<std::size_t>(Spanned))
    {
        return Outcome<std::vector<std::uint32_t>>::Refuse(
            { RefusalReason::HostDenied, "the lowered stream was read short" });
    }

    if (Words[0] != SpirvStreamMarker)
    {
        return Outcome<std::vector<std::uint32_t>>::Refuse(
            { RefusalReason::ContentUnsupported, "the stream carries no SPIR-V marker" });
    }

    return Outcome<std::vector<std::uint32_t>>::Deliver(Words);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

Outcome<std::uint32_t> ShaderCodec::Resolve(const std::string& UnitName, const std::string& StreamStem)
{
    if (DeviceEdge == nullptr)
        return Outcome<std::uint32_t>::Refuse({ RefusalReason::CapabilityAbsent, "no device is active" });

    if (UnitName.empty() || StreamStem.empty())
        return Outcome<std::uint32_t>::Refuse({ RefusalReason::ContentUnsupported, "a stream naming no unit or no stem" });

    // 📝 A stream already read is delivered rather than read again. Several programs are constructed against
    //    one module — `16`'s two raster paths share their entry point — and reading it per program would
    //    construct a second vendor module carrying the same instructions.
    for (std::size_t Ordinal = 0u; Ordinal < Modules.size(); ++Ordinal)
    {
        if (Modules[Ordinal].UnitName == UnitName && Modules[Ordinal].StreamStem == StreamStem)
            return Outcome<std::uint32_t>::Deliver(static_cast<std::uint32_t>(Ordinal));
    }

    const std::string StreamPath = StreamRoot + UnitName + "\\" + StreamStem + ".spv";

    const Outcome<std::vector<std::uint32_t>> Words = ReadStream(StreamPath);

    if (!Words.ContentPresent)
        return Outcome<std::uint32_t>::Refuse(Words.Declined);

    const std::vector<std::uint32_t> Stream = Words.Resolve();

    VkShaderModuleCreateInfo ModuleDeclaration = {};
    ModuleDeclaration.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ModuleDeclaration.codeSize                 = Stream.size() * sizeof(std::uint32_t);
    ModuleDeclaration.pCode                    = Stream.data();

    HeldModule Arriving;
    Arriving.UnitName   = UnitName;
    Arriving.StreamStem = StreamStem;

    if (vkCreateShaderModule(DeviceEdge->ActiveDevice(), &ModuleDeclaration, nullptr, &Arriving.Constructed)
        != VK_SUCCESS)
    {
        return Outcome<std::uint32_t>::Refuse(
            { RefusalReason::ContentUnsupported, "the device declined the lowered stream as a module" });
    }

    Modules.push_back(Arriving);

    return Outcome<std::uint32_t>::Deliver(static_cast<std::uint32_t>(Modules.size() - 1u));
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE STAGE
//------------------------------------------------------------------------------------------------------------------------

Outcome<VkPipelineShaderStageCreateInfo> ShaderCodec::Stage(std::uint32_t                            ModuleOrdinal,
                                                            VkShaderStageFlagBits                    Reading,
                                                            const std::vector<SpecialisedConstant>&  Fixed)
{
    if (static_cast<std::size_t>(ModuleOrdinal) >= Modules.size())
    {
        return Outcome<VkPipelineShaderStageCreateInfo>::Refuse(
            { RefusalReason::ContentUnsupported, "no module stands at that ordinal" });
    }

    VkPipelineShaderStageCreateInfo StageDeclaration = {};
    StageDeclaration.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    StageDeclaration.stage                           = Reading;
    StageDeclaration.module                          = Modules[ModuleOrdinal].Constructed;

    // 📝 🔴 The entry point is the one slangc emitted, and slangc emits the name the `[shader(...)]` attribute
    //    carried. Slate's entry points sit inside `namespace Slate`, which is exactly why the build passes no
    //    `-entry` — the attribute names them, and the emitted name is the unqualified one.
    StageDeclaration.pName = "main";

    if (Fixed.empty())
        return Outcome<VkPipelineShaderStageCreateInfo>::Deliver(StageDeclaration);

    HeldSpecialisation Held;
    Held.Declared.reserve(Fixed.size());
    Held.Fixed.reserve(Fixed.size());

    for (const SpecialisedConstant& Constant : Fixed)
    {
        VkSpecializationMapEntry Entry = {};
        Entry.constantID               = Constant.ConstantOrdinal;
        Entry.offset                   = static_cast<std::uint32_t>(Held.Fixed.size() * sizeof(std::uint32_t));
        Entry.size                     = sizeof(std::uint32_t);

        Held.Declared.push_back(Entry);
        Held.Fixed.push_back(Constant.Fixed);
    }

    Specialisations.push_back(Held);

    HeldSpecialisation& Standing = Specialisations.back();

    Standing.Read.mapEntryCount = static_cast<std::uint32_t>(Standing.Declared.size());
    Standing.Read.pMapEntries   = Standing.Declared.data();
    Standing.Read.dataSize      = Standing.Fixed.size() * sizeof(std::uint32_t);
    Standing.Read.pData         = Standing.Fixed.data();

    StageDeclaration.pSpecializationInfo = &Standing.Read;

    return Outcome<VkPipelineShaderStageCreateInfo>::Deliver(StageDeclaration);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

std::uint32_t ShaderCodec::ResolvedCount() const
{
    return static_cast<std::uint32_t>(Modules.size());
}

void ShaderCodec::Reclaim()
{
    if (DeviceEdge != nullptr && DeviceEdge->ActiveDevice() != VK_NULL_HANDLE)
    {
        for (HeldModule& Held : Modules)
        {
            if (Held.Constructed != VK_NULL_HANDLE)
            {
                vkDestroyShaderModule(DeviceEdge->ActiveDevice(), Held.Constructed, nullptr);
                Held.Constructed = VK_NULL_HANDLE;
            }
        }
    }

    Modules.clear();
    Specialisations.clear();
}

ShaderCodec::~ShaderCodec()
{
    Reclaim();
}

}   // namespace Slate
