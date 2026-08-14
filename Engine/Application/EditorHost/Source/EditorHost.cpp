//============================================================================================================================================
//                                                             EDITORHOST.CPP
//============================================================================================================================================
// 🧩 The combined editor — the same tree, the same tick, and a roster whose length is the only thing that separates it from PaintHost.

#include "SlateDocument/Document/MaterialSpecification/Api/MaterialSpecification.h"
#include "SlateDocument/Document/OutlinerSequence/Api/OutlinerSequence.h"
#include "SlateDocument/Document/PropertySpecification/Api/PropertySpecification.h"
#include "SlateDocument/Document/RevisionSequence/Api/RevisionSequence.h"
#include "SlateDocument/Document/SurfaceLayerSequence/Api/SurfaceLayerSequence.h"
#include "SlateUI/Interface/TexturePaintSpecification/Api/TexturePaintSpecification.h"
#include "SlateUI/Interface/WorkspaceSequence/Api/WorkspaceSequence.h"

#include <cstdint>
#include <cstdio>

// 📝 🔴 `32` §5: the difference between this executable and `PaintHost` is the length of one array, and that is the
//    whole claim the two-host arrangement makes. A second workspace arrives here as one more `Resolve…Workspace`
//    call in the roster and one more storage record above it — never as a branch, a registration call or a
//    conditional inside the engine.
// 📝 🚧 One workspace is built, so the roster carries one entry today. The modelling, draughting, simulation, UV and
//    baking disciplines are declared in `WorkspaceDiscipline` and unbuilt; each lands here as a line when it does.
//    A roster padded with entries whose declaration routines present nothing would put empty tabs in the strip and
//    read to the artist as workspaces that are broken rather than as workspaces that do not exist yet.

int main()
{
    // ④ SlateDocument — the documents every registered workspace presents against, owned here and nowhere beneath.
    Slate::SurfaceLayerSequence  Layers;
    Slate::MaterialIndex         Materials;
    Slate::PropertyIndex         Declarations;
    Slate::RevisionSequence      Revisions;
    Slate::OutlinerSequence      Outlined;

    Slate::WorkspaceSequence     Standing;

    // 🔴 The outliner sequence is shared across every roster entry deliberately — `12` makes it the scene's, not a
    //    workspace's. A second copy per workspace is the arrangement where switching a tab presents a different
    //    scene, which is the defect `14` §1 forbids by giving the panel no storage of its own.
    Slate::PaintWorkspaceContext Painting;
    Painting.Layers       = &Layers;
    Painting.Materials    = &Materials;
    Painting.Declarations = &Declarations;
    Painting.Revisions    = &Revisions;
    Painting.Reports      = &Standing.Reports();
    Painting.Measures     = &Standing.Measures();
    Painting.Outlined     = &Outlined;

    // ⚠️ Every entry points at storage declared above it in this scope, so the roster is outlived by nothing it
    //    addresses. An entry built from a temporary presents as panels drawn against released storage.
    const Slate::WorkspaceSpecification Roster[] = { Slate::ResolveTexturePaintWorkspace(Painting) };

    Slate::WorkspaceDeclaration Declaring;
    Declaring.WindowTitle     = "Slate";
    Declaring.Roster          = Roster;
    Declaring.RosterCount     = static_cast<std::uint32_t>(sizeof(Roster) / sizeof(Roster[0]));
    Declaring.StandingOrdinal = 0u;

    const Slate::Outcome<bool> BroughtUp = Standing.Construct(Declaring);

    if (!BroughtUp.ContentPresent)
    {
        std::printf("bring-up refused: %s\n", BroughtUp.Declined.Detail);
        return 1;
    }

    while (!Standing.ClosureRequested())
    {
        const Slate::Outcome<Slate::TickReport> Advanced = Standing.Advance();

        if (!Advanced.ContentPresent)
        {
            std::printf("tick refused: %s\n", Advanced.Declined.Detail);
            break;
        }

        if (Advanced.Resolve().ClosureRequested)
            break;
    }

    Standing.Reclaim();

    return 0;
}
