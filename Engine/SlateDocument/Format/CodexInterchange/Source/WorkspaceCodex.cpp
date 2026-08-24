//============================================================================================================================================
//                                                         WORKSPACECODEX.CPP
//============================================================================================================================================
// 🧩 Workspace section inscription and recovery without coupling document storage to any interface implementation.

#include "SlateDocument/Format/CodexInterchange/Api/WorkspaceCodex.h"

#include <cstring>
#include <limits>
#include <utility>

namespace Slate
{

namespace
{

constexpr std::uint32_t NamingSection      = 0x4D414E57u;   // [-] - `WNAM`
constexpr std::uint32_t EnvironmentSection = 0x564E4557u;   // [-] - `WENV`
constexpr std::uint32_t SceneSection       = 0x454E4353u;   // [-] - `SCNE`
constexpr std::uint32_t EmbeddedSection    = 0x44424D45u;   // [-] - `EMBD`

void Inscribe32(std::vector<std::uint8_t>& Content, std::uint32_t Held)
{
    for (std::uint32_t Shift = 0u; Shift < 32u; Shift += 8u)
        Content.push_back(static_cast<std::uint8_t>(Held >> Shift));
}

void Inscribe64(std::vector<std::uint8_t>& Content, std::uint64_t Held)
{
    for (std::uint32_t Shift = 0u; Shift < 64u; Shift += 8u)
        Content.push_back(static_cast<std::uint8_t>(Held >> Shift));
}

void InscribeReal(std::vector<std::uint8_t>& Content, double Held)
{
    std::uint64_t Bits = 0u;
    static_assert(sizeof(Bits) == sizeof(Held));
    std::memcpy(&Bits, &Held, sizeof(Bits));
    Inscribe64(Content, Bits);
}

void InscribeRun(std::vector<std::uint8_t>& Content, const std::string& Held)
{
    Inscribe32(Content, static_cast<std::uint32_t>(Held.size()));
    Content.insert(Content.end(), Held.begin(), Held.end());
}

bool Extract32(const std::vector<std::uint8_t>& Content, std::size_t& Position, std::uint32_t& Held)
{
    if (Position > Content.size() || Content.size() - Position < 4u)
        return false;

    Held = 0u;
    for (std::uint32_t Shift = 0u; Shift < 32u; Shift += 8u)
        Held |= static_cast<std::uint32_t>(Content[Position++]) << Shift;
    return true;
}

bool Extract64(const std::vector<std::uint8_t>& Content, std::size_t& Position, std::uint64_t& Held)
{
    if (Position > Content.size() || Content.size() - Position < 8u)
        return false;

    Held = 0u;
    for (std::uint32_t Shift = 0u; Shift < 64u; Shift += 8u)
        Held |= static_cast<std::uint64_t>(Content[Position++]) << Shift;
    return true;
}

bool ExtractReal(const std::vector<std::uint8_t>& Content, std::size_t& Position, double& Held)
{
    std::uint64_t Bits = 0u;
    if (!Extract64(Content, Position, Bits))
        return false;

    std::memcpy(&Held, &Bits, sizeof(Held));
    return true;
}

bool ExtractRun(const std::vector<std::uint8_t>& Content, std::size_t& Position, std::string& Held)
{
    std::uint32_t ByteCount = 0u;
    if (!Extract32(Content, Position, ByteCount) || ByteCount > Content.size() - Position)
        return false;

    Held.assign(reinterpret_cast<const char*>(Content.data() + Position), ByteCount);
    Position += ByteCount;
    return true;
}

const CodexSection* SectionOf(const CodexDocument& Document, std::uint32_t Code)
{
    for (const CodexSection& Current : Document.Sections)
    {
        if (Current.Code == Code)
            return &Current;
    }

    return nullptr;
}

CodexSection Section(std::uint32_t Code, std::uint64_t Revision, std::vector<std::uint8_t>&& Content)
{
    CodexSection Produced;
    Produced.Code = Code;
    Produced.MajorVersion = 1u;
    Produced.MinorVersion = 0u;
    Produced.Revision = Revision;
    Produced.Content = std::move(Content);
    return Produced;
}

}

Outcome<CodexDocument> WorkspaceCodexInterchange::EncodeWorkspace(const WorkspaceCodex& Workspace,
                                                                   std::uint64_t          Identity,
                                                                   std::uint64_t          Revision) const
{
    if (Workspace.Scene.size() > std::numeric_limits<std::uint32_t>::max() ||
        Workspace.Embedded.size() > std::numeric_limits<std::uint32_t>::max())
    {
        return Outcome<CodexDocument>::Refuse(
            { RefusalReason::ExtentExhausted, "the workspace carries too many scene or embedded documents" });
    }

    std::vector<std::uint8_t> Naming;
    InscribeRun(Naming, Workspace.Naming);

    std::vector<std::uint8_t> Environment;
    InscribeReal(Environment, Workspace.Environment.SunElevation);
    InscribeReal(Environment, Workspace.Environment.SunAzimuth);
    InscribeReal(Environment, Workspace.Environment.SunIntensity);
    InscribeReal(Environment, Workspace.Environment.SunTemperature);
    InscribeReal(Environment, Workspace.Environment.SkyIntensity);
    InscribeReal(Environment, Workspace.Environment.AtmosphereDensity);
    InscribeReal(Environment, Workspace.Environment.AtmosphereScaleHeight);

    std::vector<std::uint8_t> Scene;
    Inscribe32(Scene, static_cast<std::uint32_t>(Workspace.Scene.size()));
    for (const CodexSceneEntry& Current : Workspace.Scene)
    {
        Inscribe32(Scene, static_cast<std::uint32_t>(Current.Subject));
        InscribeRun(Scene, Current.Naming);
        InscribeRun(Scene, Current.GeometryReference);
        InscribeRun(Scene, Current.MaterialReference);
        for (std::uint32_t Axis = 0u; Axis < 3u; ++Axis)
            InscribeReal(Scene, Current.Position[Axis]);
        for (std::uint32_t Axis = 0u; Axis < 3u; ++Axis)
            InscribeReal(Scene, Current.Rotation[Axis]);
        for (std::uint32_t Axis = 0u; Axis < 3u; ++Axis)
            InscribeReal(Scene, Current.Scale[Axis]);
    }

    CodexInterchange Codex;
    std::vector<std::uint8_t> Embedded;
    Inscribe32(Embedded, static_cast<std::uint32_t>(Workspace.Embedded.size()));
    for (const CodexDocument& Current : Workspace.Embedded)
    {
        const Outcome<std::vector<std::uint8_t>> Encoded = Codex.Encode(Current);
        if (!Encoded.Resolved || Encoded.Resolve().size() > std::numeric_limits<std::uint32_t>::max())
        {
            return Outcome<CodexDocument>::Refuse(
                { RefusalReason::ContentUnsupported, "an embedded Codex document could not be represented" });
        }

        Inscribe32(Embedded, static_cast<std::uint32_t>(Encoded.Resolve().size()));
        Embedded.insert(Embedded.end(), Encoded.Resolve().begin(), Encoded.Resolve().end());
    }

    CodexDocument Produced;
    Produced.Profile = CodexProfile::Workspace;
    Produced.Identity = Identity;
    Produced.CurrentRevision = Revision;
    Produced.Sections.push_back(Section(NamingSection, Revision, std::move(Naming)));
    Produced.Sections.push_back(Section(EnvironmentSection, Revision, std::move(Environment)));
    Produced.Sections.push_back(Section(SceneSection, Revision, std::move(Scene)));
    Produced.Sections.push_back(Section(EmbeddedSection, Revision, std::move(Embedded)));
    return Outcome<CodexDocument>::Result(std::move(Produced));
}

Outcome<WorkspaceCodex> WorkspaceCodexInterchange::DecodeWorkspace(const CodexDocument& Document) const
{
    if (Document.Profile != CodexProfile::Workspace)
    {
        return Outcome<WorkspaceCodex>::Refuse(
            { RefusalReason::ContentUnsupported, "the Codex document is not a workspace profile" });
    }

    const CodexSection* Naming = SectionOf(Document, NamingSection);
    const CodexSection* Environment = SectionOf(Document, EnvironmentSection);
    const CodexSection* Scene = SectionOf(Document, SceneSection);
    const CodexSection* Embedded = SectionOf(Document, EmbeddedSection);
    if (Naming == nullptr || Environment == nullptr || Scene == nullptr || Embedded == nullptr)
    {
        return Outcome<WorkspaceCodex>::Refuse(
            { RefusalReason::ContentUnsupported, "the workspace is missing one required typed section" });
    }

    WorkspaceCodex Produced;
    std::size_t Position = 0u;
    if (!ExtractRun(Naming->Content, Position, Produced.Naming) || Position != Naming->Content.size())
    {
        return Outcome<WorkspaceCodex>::Refuse(
            { RefusalReason::ContentUnsupported, "the workspace naming section is inconsistent" });
    }

    Position = 0u;
    if (!ExtractReal(Environment->Content, Position, Produced.Environment.SunElevation) ||
        !ExtractReal(Environment->Content, Position, Produced.Environment.SunAzimuth) ||
        !ExtractReal(Environment->Content, Position, Produced.Environment.SunIntensity) ||
        !ExtractReal(Environment->Content, Position, Produced.Environment.SunTemperature) ||
        !ExtractReal(Environment->Content, Position, Produced.Environment.SkyIntensity) ||
        !ExtractReal(Environment->Content, Position, Produced.Environment.AtmosphereDensity) ||
        !ExtractReal(Environment->Content, Position, Produced.Environment.AtmosphereScaleHeight) ||
        Position != Environment->Content.size())
    {
        return Outcome<WorkspaceCodex>::Refuse(
            { RefusalReason::ContentUnsupported, "the workspace environment section is inconsistent" });
    }

    Position = 0u;
    std::uint32_t SceneCount = 0u;
    if (!Extract32(Scene->Content, Position, SceneCount))
    {
        return Outcome<WorkspaceCodex>::Refuse(
            { RefusalReason::ContentUnsupported, "the workspace scene section is incomplete" });
    }

    Produced.Scene.reserve(SceneCount);
    for (std::uint32_t Index = 0u; Index < SceneCount; ++Index)
    {
        std::uint32_t Subject = 0u;
        CodexSceneEntry Current;
        if (!Extract32(Scene->Content, Position, Subject) || Subject > static_cast<std::uint32_t>(CodexSceneSubject::Geometry) ||
            !ExtractRun(Scene->Content, Position, Current.Naming) ||
            !ExtractRun(Scene->Content, Position, Current.GeometryReference) ||
            !ExtractRun(Scene->Content, Position, Current.MaterialReference))
        {
            return Outcome<WorkspaceCodex>::Refuse(
                { RefusalReason::ContentUnsupported, "a workspace scene entry is inconsistent" });
        }

        Current.Subject = static_cast<CodexSceneSubject>(Subject);
        for (std::uint32_t Axis = 0u; Axis < 3u; ++Axis)
            if (!ExtractReal(Scene->Content, Position, Current.Position[Axis]))
                return Outcome<WorkspaceCodex>::Refuse(
                    { RefusalReason::ContentUnsupported, "a workspace scene placement is incomplete" });
        for (std::uint32_t Axis = 0u; Axis < 3u; ++Axis)
            if (!ExtractReal(Scene->Content, Position, Current.Rotation[Axis]))
                return Outcome<WorkspaceCodex>::Refuse(
                    { RefusalReason::ContentUnsupported, "a workspace scene rotation is incomplete" });
        for (std::uint32_t Axis = 0u; Axis < 3u; ++Axis)
            if (!ExtractReal(Scene->Content, Position, Current.Scale[Axis]))
                return Outcome<WorkspaceCodex>::Refuse(
                    { RefusalReason::ContentUnsupported, "a workspace scene scale is incomplete" });

        Produced.Scene.push_back(std::move(Current));
    }

    if (Position != Scene->Content.size())
    {
        return Outcome<WorkspaceCodex>::Refuse(
            { RefusalReason::ContentUnsupported, "the workspace scene section has trailing content" });
    }

    Position = 0u;
    std::uint32_t EmbeddedCount = 0u;
    if (!Extract32(Embedded->Content, Position, EmbeddedCount))
    {
        return Outcome<WorkspaceCodex>::Refuse(
            { RefusalReason::ContentUnsupported, "the workspace embedded section is incomplete" });
    }

    CodexInterchange Codex;
    Produced.Embedded.reserve(EmbeddedCount);
    for (std::uint32_t Index = 0u; Index < EmbeddedCount; ++Index)
    {
        std::uint32_t ByteCount = 0u;
        if (!Extract32(Embedded->Content, Position, ByteCount) || ByteCount > Embedded->Content.size() - Position)
        {
            return Outcome<WorkspaceCodex>::Refuse(
                { RefusalReason::ContentUnsupported, "an embedded Codex extent is inconsistent" });
        }

        std::vector<std::uint8_t> Stream(Embedded->Content.begin() + Position,
                                         Embedded->Content.begin() + Position + ByteCount);
        Position += ByteCount;

        const Outcome<CodexDocument> Decoded = Codex.Decode(Stream);
        if (!Decoded.Resolved)
            return Outcome<WorkspaceCodex>::Refuse(Decoded.Error);

        Produced.Embedded.push_back(Decoded.Resolve());
    }

    if (Position != Embedded->Content.size())
    {
        return Outcome<WorkspaceCodex>::Refuse(
            { RefusalReason::ContentUnsupported, "the workspace embedded section has trailing content" });
    }

    return Outcome<WorkspaceCodex>::Result(std::move(Produced));
}

}   // namespace Slate
