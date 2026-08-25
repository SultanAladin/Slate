//============================================================================================================================================
//                                                  WORKSPACEPROPERTYPROJECTION.H
//============================================================================================================================================
// 🧩 The backend projection the future properties panel reads: selected semantic record plus the filtered
//    revision subset that belongs in the Properties | Revision pages.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "SlateFeature/Feature/WorkspaceRecordStructure/Api/WorkspaceRecordStructure.h"
#include "SlateFeature/Feature/WorkspaceRevisionSequence/Api/WorkspaceRevisionSequence.h"

#include <vector>

namespace Slate
{

struct WorkspacePropertyProjection
{
    const WorkspaceRecord* Subject = nullptr;
    std::vector<WorkspaceRevisionName> RevisionSet = {};
};

Outcome<WorkspacePropertyProjection> ProjectWorkspaceProperty(const WorkspaceRecordStructure& Records,
                                                              const WorkspaceRevisionSequence& Revisions,
                                                              WorkspaceRecordName Subject);

} // namespace Slate
