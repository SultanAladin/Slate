//============================================================================================================================================
//                                                     TEXTUREPAINTSPECIFICATION.CPP
//============================================================================================================================================
// 🧩 The nine declarations one painting workspace makes, and nothing else — every panel it names is already routine-shaped.

#include "SlateUI/Interface/TexturePaintSpecification/Api/TexturePaintSpecification.h"

namespace Slate
{
namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE DECLARED EXTENT
//------------------------------------------------------------------------------------------------------------------------

// 📝 🔴 Six, and the ledger holds sixteen. Six is what the array below actually initialises, and the count is what
//    the loop walks — so the two must agree by construction. They did not: the count read seven over six initialisers,
//    which handed `DeclarePanel` a zeroed seventh slot on every activation. That slot names no identifier and carries
//    no present routine, so it was refused silently by the one call site that deliberately does not inspect its outcome.
constexpr std::uint32_t TexturePaintPanelCount = 6u;   // [-] - panels declared through the array below

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE PANEL DECLARATION
//------------------------------------------------------------------------------------------------------------------------

void DeclareTexturePaintPanels(PanelIndex& Ledger, void* WorkspaceContext)
{
    PaintWorkspaceContext* Standing = static_cast<PaintWorkspaceContext*>(WorkspaceContext);

    if (Standing == nullptr)
        return;

    // 🔴 Rebuilt at every activation and never once at construction. A host that opened a second document or moved
    //    the material ordinal between activations would otherwise present the rows against whatever was standing
    //    when the record was first filled, and that reads as a panel showing another document's contents.
    Standing->DeclaredOutlinerContext   = { Standing->Outlined,     &Standing->OutlinerCarry    };
    Standing->DeclaredLayerContext      = { Standing->Layers,       &Standing->LayerCarry       };
    Standing->DeclaredChannelContext    = { Standing->Materials,     Standing->MaterialOrdinal,
                                                                    &Standing->ChannelCarry     };
    Standing->DeclaredPropertyContext   = { Standing->Declarations, &Standing->PropertyCarry    };
    Standing->DeclaredRevisionContext   = { Standing->Revisions,    &Standing->RevisionCarry    };
    Standing->DeclaredDiagnosticContext = { Standing->Reports,       Standing->Measures,
                                                                    &Standing->DiagnosticCarry  };

    // 📝 🔴 Every context addressed below is a member of the record the caller owns, never a local. The ledger
    //    retains these addresses for as long as the workspace stands, so a context built in this scope would be a
    //    released extent by the first tick that presented it.
    // 📝 The identifiers are string literals with static storage, which `PanelSlot` requires outright — it retains
    //    both text pointers and copies neither.
    const PanelSlot Declaring[TexturePaintPanelCount] =
    {
        { "TexturePaintOutliner",    "Outliner",     WorkspacePanelSide::Left,
                                                     &PresentOutlinerPanel,    &Standing->DeclaredOutlinerContext   },
        { "TexturePaintLayers",      "Layers",       WorkspacePanelSide::Right,
                                                     &PresentLayerPanel,       &Standing->DeclaredLayerContext      },
        { "TexturePaintChannels",    "Channels",     WorkspacePanelSide::Right,
                                                     &PresentChannelPanel,     &Standing->DeclaredChannelContext    },
        { "TexturePaintProperties",  "Properties",   WorkspacePanelSide::Right,
                                                     &PresentPropertyPanel,    &Standing->DeclaredPropertyContext   },
        { "TexturePaintRevisions",   "Revisions",    WorkspacePanelSide::Bottom,
                                                     &PresentRevisionPanel,    &Standing->DeclaredRevisionContext   },
        { "TexturePaintDiagnostics", "Diagnostics",  WorkspacePanelSide::Bottom,
                                                     &PresentDiagnosticPanel,  &Standing->DeclaredDiagnosticContext },
    };

    // 🔴 The agreement the count's note describes, asserted rather than trusted. A seventh initialiser added without
    //    amending the constant, or a constant raised without a matching initialiser, stops the build here — which is
    //    precisely what the previous spelling of this file failed to do while it silently declared a zeroed slot.
    static_assert(sizeof(Declaring) / sizeof(Declaring[0]) == TexturePaintPanelCount,
                  "The declared count and the array of declarations must name the same number of panels.");

    for (std::uint32_t SlotOrdinal = 0u; SlotOrdinal < TexturePaintPanelCount; ++SlotOrdinal)
    {
        // 📝 The outcome is deliberately not inspected. `DeclarePanel` raises exactly two refusals — a slot naming
        //    no identifier or no present routine, and a ledger already at `PanelSlotCapacity`. The literal above
        //    settles both at compile time, so a check here would test a fact the array has already decided.
        DeclarePanel(Ledger, Declaring[SlotOrdinal]);
    }

    // 📝 The last three are declared through their own `Resolve*Slot` calls rather than from the array, because each
    //    addresses its specification directly and has no separate context record to rebuild above. The (V) list offers
    //    whatever the standing workspace declared into its ledger, so all three become reachable the moment this runs —
    //    and `EditorHost.cpp` and `PaintHost.cpp`, which only assign document pointers, are untouched by any of it.
    DeclarePanel(Ledger, ResolveCanvasSlot("Canvas", "Viewport", Standing->Canvas));

    DeclarePanel(Ledger, ResolveAssetSlot("TexturePaintAssets", "Assets",
                                          WorkspacePanelSide::Left, Standing->Assets));

    DeclarePanel(Ledger, ResolveEntrySlot("TexturePaintEntries", "Controls",
                                          WorkspacePanelSide::Right, Standing->Entries));
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE WORKSPACE
//------------------------------------------------------------------------------------------------------------------------

WorkspaceSpecification ResolveTexturePaintWorkspace(PaintWorkspaceContext& Standing)
{
    WorkspaceSpecification Entry;

    Entry.Caption          = "Texture Paint";

    // 📝 🔴 The stem names the **document** and must not name the workspace. `CarryTitle` mints a leaf's caption from
    //    it, so a stem of "PaintWorkspace" printed "PaintWorkspace 2" on the desk's own strip — directly beneath the
    //    roster trapezoid reading "Texture Paint" — and the pair read as a workspace nested inside a workspace. What
    //    the leaf actually carries is a canvas, so that is what it is called.
    Entry.NameStem         = "Canvas";
    Entry.Discipline       = WorkspaceDiscipline::Painting;
    Entry.DeclarePanels    = &DeclareTexturePaintPanels;
    Entry.PresentCentre    = nullptr;
    Entry.WorkspaceContext = &Standing;

    return Entry;
}

}   // namespace Slate
