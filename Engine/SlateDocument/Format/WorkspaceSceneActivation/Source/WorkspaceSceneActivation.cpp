#include "SlateDocument/Format/WorkspaceSceneActivation/Api/WorkspaceSceneActivation.h"

#include "SlateDocument/Format/CodexInterchange/Api/CodexInterchange.h"

namespace Slate
{

Outcome<ActivatedWorkspaceScene> WorkspaceSceneActivation::Open(const std::string& CodexPath,
                                                                  const std::string& EngineContentPath) const
{
    (void)EngineContentPath;

    CodexInterchange Stream;
    WorkspaceCodexInterchange Typed;
    const Outcome<CodexDocument> Document = Stream.Open(CodexPath);
    if (!Document.Resolved) return Outcome<ActivatedWorkspaceScene>::Refuse(Document.Error);
    const Outcome<WorkspaceCodex> Decoded = Typed.DecodeWorkspace(Document.Resolve());
    if (!Decoded.Resolved) return Outcome<ActivatedWorkspaceScene>::Refuse(Decoded.Error);

    ActivatedWorkspaceScene Activated;
    Activated.Workspace = Decoded.Resolve();
    std::uint32_t SunCount = 0u;
    std::uint32_t SkyCount = 0u;
    std::uint32_t AtmosphereCount = 0u;

    for (const CodexSceneEntry& Entry : Activated.Workspace.Scene)
    {
        if (Entry.Subject == CodexSceneSubject::Sun) ++SunCount;
        else if (Entry.Subject == CodexSceneSubject::Sky) ++SkyCount;
        else if (Entry.Subject == CodexSceneSubject::Atmosphere) ++AtmosphereCount;
        else if (Entry.Subject == CodexSceneSubject::Geometry)
        {
            if (Entry.GeometryReference.empty() || Entry.MaterialReference.empty())
                return Outcome<ActivatedWorkspaceScene>::Refuse(
                    { RefusalReason::ContentUnsupported, "a workspace geometry entry lacks a source or material reference" });
            if (Activated.SharedMaterialReference.empty()) Activated.SharedMaterialReference = Entry.MaterialReference;
            if (Activated.SharedMaterialReference != Entry.MaterialReference)
                return Outcome<ActivatedWorkspaceScene>::Refuse(
                    { RefusalReason::ContentUnsupported, "the White Tea Service workspace must retain one shared material reference" });

            ActivatedGeometryEntry Resolved;
            Resolved.Entry = Entry;
            // Geometry is carried by the binary workspace stream. Runtime activation must not resolve
            // WhiteTeaService entries back to editable OBJ files under EngineContent.
            Resolved.SourcePath = CodexPath + "#" + Entry.GeometryReference;
            Activated.Geometry.push_back(std::move(Resolved));
        }
    }

    if (SunCount != 1u || SkyCount != 1u || AtmosphereCount != 1u || Activated.Geometry.size() != 6u ||
        Activated.SharedMaterialReference.empty())
    {
        return Outcome<ActivatedWorkspaceScene>::Refuse(
            { RefusalReason::ContentUnsupported, "the workspace does not declare one environment, six geometry entries, and one shared material" });
    }

    return Outcome<ActivatedWorkspaceScene>::Result(Activated);
}

} // namespace Slate
