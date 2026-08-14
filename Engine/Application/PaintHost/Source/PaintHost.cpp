//============================================================================================================================================
//                                                              PAINTHOST.CPP
//============================================================================================================================================
// 🧩 The standalone painting executable — one roster entry, the documents it presents against, and no engine logic at all.

#include "SlateDocument/Document/MaterialSpecification/Api/MaterialSpecification.h"
#include "SlateDocument/Document/OutlinerSequence/Api/OutlinerSequence.h"
#include "SlateDocument/Document/PropertySpecification/Api/PropertySpecification.h"
#include "SlateDocument/Document/RevisionSequence/Api/RevisionSequence.h"
#include "SlateDocument/Document/SurfaceLayerSequence/Api/SurfaceLayerSequence.h"
#include "SlateUI/Interface/TexturePaintSpecification/Api/TexturePaintSpecification.h"
#include "SlateUI/Interface/WorkspaceSequence/Api/WorkspaceSequence.h"

#include <cstdio>

// 📝 🔴 `32` §5's gate reads on this file rather than on any other: a host declares storage, registers a roster and
//    ticks. Nothing here decides an ordering, derives a rectangle, resolves a colour or touches a vendor spelling.
//    Every line that would is already inside `WorkspaceSequence`, which is why `EditorHost` is this file with a
//    longer array.

int main()
{
    // ④ SlateDocument — the documents the workspace presents against, owned here and nowhere beneath.
    Slate::SurfaceLayerSequence  Layers;
    Slate::MaterialIndex         Materials;
    Slate::PropertyIndex         Declarations;
    Slate::RevisionSequence      Revisions;
    Slate::OutlinerSequence      Outlined;

    Slate::WorkspaceSequence     Standing;

    // 🔴 The register and the measures are the sequence's own, addressed before bring-up and never copied. Two
    //    registers is a diagnostic panel presenting an empty list beside a host that refused, which is the defect
    //    `WorkspaceSequence::Reports` exists to make impossible.
    Slate::PaintWorkspaceContext Painting;
    Painting.Layers       = &Layers;
    Painting.Materials    = &Materials;
    Painting.Declarations = &Declarations;
    Painting.Revisions    = &Revisions;
    Painting.Reports      = &Standing.Reports();
    Painting.Measures     = &Standing.Measures();
    Painting.Outlined     = &Outlined;

    // ⚠️ The roster outlives the sequence because both are automatic in this scope and the roster is declared
    //    first. An entry built from a temporary is the dangling roster `WorkspaceSpecification` warns about.
    const Slate::WorkspaceSpecification Roster[] = { Slate::ResolveTexturePaintWorkspace(Painting) };

    Slate::WorkspaceDeclaration Declaring;
    Declaring.WindowTitle     = "Slate — Texture Paint";
    Declaring.Roster          = Roster;
    Declaring.RosterCount     = 1u;
    Declaring.StandingOrdinal = 0u;

    const Slate::Outcome<bool> BroughtUp = Standing.Construct(Declaring);

    if (!BroughtUp.ContentPresent)
    {
        std::printf("bring-up refused: %s\n", BroughtUp.Declined.Detail);
        return 1;
    }

    // 📝 The tick's own refusals are already in the register the diagnostic panel presents, so a refused tick ends
    //    the run rather than being reported twice. A skipped rotation and a re-established chain are delivered
    //    outcomes and never reach this branch.
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
