//============================================================================================================================================
//                                                WORKSPACEPROPERTYPROJECTION.CPP
//============================================================================================================================================

#include "SlateFeature/Feature/WorkspacePropertyProjection/Api/WorkspacePropertyProjection.h"

namespace Slate
{

Outcome<WorkspacePropertyProjection> ProjectWorkspaceProperty(const WorkspaceRecordStructure& Records,
                                                              const WorkspaceRevisionSequence& Revisions,
                                                              WorkspaceRecordName Subject)
{
    const WorkspaceRecord* Record = Records.Resolve(Subject);
    if (Record == nullptr)
        return Outcome<WorkspacePropertyProjection>::Refuse({ RefusalReason::ContentUnsupported, "no such workspace record is declared" });

    WorkspacePropertyProjection Projection;
    Projection.Subject = Record;
    Revisions.ResolveForRecord(Subject, Projection.RevisionSet);
    return Outcome<WorkspacePropertyProjection>::Result(Projection);
}

} // namespace Slate
