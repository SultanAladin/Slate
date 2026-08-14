//============================================================================================================================================
//                                                     TEXTUREPAINTSPECIFICATION.CPP
//============================================================================================================================================
// 🧩 The six declarations one painting workspace makes, and nothing else — every panel it names is already routine-shaped.

#include "SlateUI/Interface/TexturePaintSpecification/Api/TexturePaintSpecification.h"

namespace Slate
{
namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE DECLARED EXTENT
//------------------------------------------------------------------------------------------------------------------------

// 📝 Six, and the ledger holds sixteen. The count is spelled once so the array literal and the loop that walks it
//    cannot disagree — which is the whole class of defect where a seventh panel is written and never presented.
constexpr std::uint32_t TexturePaintPanelCount = 7u;   // [-] - panels this workspace declares

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

    for (std::uint32_t SlotOrdinal = 0u; SlotOrdinal < TexturePaintPanelCount; ++SlotOrdinal)
    {
        // 📝 The outcome is deliberately not inspected. `DeclarePanel` raises exactly two refusals — a slot naming
        //    no identifier or no present routine, and a ledger already at `PanelSlotCapacity`. The literal above
        //    settles both at compile time, so a check here would test a fact the array has already decided.
        DeclarePanel(Ledger, Declaring[SlotOrdinal]);
    }

    // 📝 The viewport is declared separately using ResolveCanvasSlot. The (V) list offers whatever the standing workspace declared into its
    //    ledger, so the viewport becomes reachable the moment the paint workspace declares this slot. Two lines here, and no host changes at
    //    all — EditorHost.cpp and PaintHost.cpp only assign pointers into the context and are untouched by a by-value member.
    DeclarePanel(Ledger, ResolveCanvasSlot("Canvas", "Viewport", Standing->Canvas));
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE WORKSPACE
//------------------------------------------------------------------------------------------------------------------------

WorkspaceSpecification ResolveTexturePaintWorkspace(PaintWorkspaceContext& Standing)
{
    WorkspaceSpecification Entry;

    Entry.Caption          = "Texture Paint";
    Entry.NameStem         = "PaintWorkspace";
    Entry.Discipline       = WorkspaceDiscipline::Painting;
    Entry.DeclarePanels    = &DeclareTexturePaintPanels;
    Entry.PresentCentre    = nullptr;
    Entry.WorkspaceContext = &Standing;

    return Entry;
}

}   // namespace Slate
