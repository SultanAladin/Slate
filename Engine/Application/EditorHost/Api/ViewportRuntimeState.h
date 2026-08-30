//============================================================================================================================================
//                                                   VIEWPORTRUNTIMESTATE.H
//============================================================================================================================================
// Per-leaf viewport state. Application-wide frame inputs live in EditorFrameContext; this type owns the
// state that must not leak between split viewport leaves.

#pragma once

#include "Shared/OverlayGeometry.slang.h"
#include "SlateWorkspace/Discipline/ViewportProjection/Api/ViewportProjection.h"
#include "SlateWorkspace/Discipline/ViewportProjection/Api/CadProjection.h"
#include "SlateWorkspace/Discipline/WorkplaneCatalogue/Api/WorkplaneCatalogue.h"

namespace Slate
{

struct ViewportRuntimeState
{
    ProjectionTransit       Projection = {};
    ViewportStanding        Standing = {};
    WorkplaneName           ActiveWorkplane = {};
    ResolvedCamera          Camera = {};
    OverlayGeometry         Overlay = {};
    WorkspaceCadProjection  CadProjection = {};
    bool                    WasParallel = false;
    std::uint32_t           UploadedOverlayGeneration = 0u;
    float                   UploadedOverlayScale = 0.0f;
};

} // namespace Slate
