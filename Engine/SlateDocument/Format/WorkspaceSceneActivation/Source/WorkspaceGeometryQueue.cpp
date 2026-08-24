#include "SlateDocument/Format/WorkspaceSceneActivation/Api/WorkspaceGeometryQueue.h"
namespace Slate
{
Outcome<std::vector<WorkspaceGeometryQueueEntry>> WorkspaceGeometryQueue::Prepare(const ActivatedWorkspaceScene& Scene) const
{
 if (Scene.Geometry.size()!=6u) return Outcome<std::vector<WorkspaceGeometryQueueEntry>>::Refuse({RefusalReason::ContentUnsupported,"the activated workspace does not carry its complete geometry set"});
 std::vector<WorkspaceGeometryQueueEntry> Queue; Queue.reserve(Scene.Geometry.size());
 for(std::uint32_t Index=0u;Index<Scene.Geometry.size();++Index) Queue.push_back({Scene.Geometry[Index],Index});
 return Outcome<std::vector<WorkspaceGeometryQueueEntry>>::Result(Queue);
}
}
