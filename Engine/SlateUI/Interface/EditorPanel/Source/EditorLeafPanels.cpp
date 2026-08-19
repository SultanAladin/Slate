//============================================================================================================================================
//                                                       EDITORLEAFPANELS.CPP
//============================================================================================================================================
// 🧩 One skeletal leaf body, seated four ways, isolated from the shared editor chrome and partition.

#include "SlateUI/Interface/EditorPanel/Api/EditorLeafPanels.h"

namespace Slate
{

namespace
{

/// 🧩 The three ordinates a leaf differs by. Everything else about a leaf is shared.
struct LeafAppearance
{
    InkOrdinate  Ground   = {};        // [-]  - the ground ink the body is filled with
    const char*  Caption  = nullptr;   // [-]  - the centred caption, never owned
    float        TextSize = 0.0f;      // [px] - the caption's text size
};

/// 🧩 Reads the arriving leaf's three distinguishing ordinates out of the appearance declarations.
/// in    Appearance  [-]  the appearance declarations
/// in    Subject     [-]  which leaf is being presented
/// out   LeafAppearance  [-]  the ground ink, caption and text size for that leaf
/// cost  ✔️
LeafAppearance AppearanceFor(const AppearanceSpecification& Appearance, LeafSubject Subject)
{
    switch (Subject)
    {
        case LeafSubject::Scene:
            return { Appearance.EditorPanel.ViewGround,
                     "3D VIEWPORT RENDER TARGET",
                     Appearance.EditorPanelMeasure.TextSmall };

        case LeafSubject::Uv:
            return { Appearance.EditorPanel.ViewGround,
                     "UV EDITOR RENDER TARGET",
                     Appearance.EditorPanelMeasure.TextSmall };

        case LeafSubject::Outliner:
        case LeafSubject::Property:
        default:
            return { Appearance.EditorPanel.BodyGround,
                     "Empty",
                     Appearance.EditorPanelMeasure.TextBody };
    }
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                       LEAF TARGET
//------------------------------------------------------------------------------------------------------------------------

Deliver<bool> LeafPanel::Construct(RecordingSurface& ArrivingSurface,
                                   const AppearanceSpecification& ArrivingAppearance,
                                   LeafSubject ArrivingSubject)
{
    if (Surface != nullptr)
        return Deliver<bool>::Refuse({ RefusalReason::ContentUnsupported, "a leaf panel construction stands" });

    Surface    = &ArrivingSurface;
    Appearance = &ArrivingAppearance;
    Subject    = ArrivingSubject;
    return Deliver<bool>::Deliver(true);
}

void LeafPanel::Record(const PlaneExtent& Extent)
{
    if (Surface == nullptr || Appearance == nullptr)
        return;

    const LeafAppearance Seated = AppearanceFor(*Appearance, Subject);

    Surface->Ground(Extent, Seated.Ground);
    Surface->TextRun(Extent.LeastAlong + Extent.SpanAlong() * 0.5f,
                     Extent.LeastAcross + Extent.SpanAcross() * 0.5f,
                     Appearance->EditorPanel.InkGhost,
                     Seated.Caption,
                     Seated.TextSize,
                     0.0f,
                     true);
}

void LeafPanel::Reset()
{
    Surface    = nullptr;
    Appearance = nullptr;
    Subject    = LeafSubject::Scene;
}

}   // namespace Slate
