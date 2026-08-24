//============================================================================================================================================
//                                                  WORKSPACESCENEACTIVATION.H
//============================================================================================================================================
// 🧩 Validated workspace-Codex activation data, independent of editor, UI, GPU, and scene lifetime.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "SlateDocument/Format/CodexInterchange/Api/WorkspaceCodex.h"

#include <string>
#include <vector>

namespace Slate
{

/// 🧩 One geometry entry after its workspace-relative source has been resolved against Engine Content.
struct ActivatedGeometryEntry
{
    CodexSceneEntry Entry = {};
    std::string SourcePath = {};
};

/// 🧩 Immutable result a host may commit atomically into its Outliner, layer model, and geometry intake queue.
struct ActivatedWorkspaceScene
{
    WorkspaceCodex Workspace = {};
    std::vector<ActivatedGeometryEntry> Geometry = {};
    std::string SharedMaterialReference = {};
};

/// 🧩 Opens and validates a WorkspaceCodex before a host changes its presented scene.
class WorkspaceSceneActivation
{
public:
    Outcome<ActivatedWorkspaceScene> Open(const std::string& CodexPath,
                                          const std::string& EngineContentPath) const;
};

} // namespace Slate
