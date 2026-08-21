//============================================================================================================================================
//                                                           EDITORPANEL.CPP
//============================================================================================================================================
// 🧩 Exact editor chrome and bounded split interaction around skeletal workspace render targets.

#include "SlateUI/Interface/EditorPanel/Api/EditorPanel.h"

#include <cmath>

namespace Slate
{

namespace
{

const char* SubjectTitle(PanelSubject Subject)
{
    switch (Subject)
    {
        case PanelSubject::Viewport:   return "3D Viewport";
        case PanelSubject::Uv:         return "UV Editor";
        case PanelSubject::Outliner:   return "Outliner";
        case PanelSubject::Properties: return "Properties";
        default:                       return "Choose Panel Type";
    }
}

const char* ShadingTitle(PanelShading Shading)
{
    switch (Shading)
    {
        case PanelShading::Wireframe:    return "wireframe";
        case PanelShading::Matcap:       return "matcap";
        case PanelShading::Normal:       return "normal";
        case PanelShading::Metallic:     return "metallic";
        case PanelShading::Illumination: return "gi";
        default:                         return "solid";
    }
}

const char* GizmoTitle(PanelGizmo Gizmo)
{
    return Gizmo == PanelGizmo::Cad ? "cad" : "blender";
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                       CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> EditorPanel::Construct(MotionIntegrator& IncomingMotion,
                                     RecordingSurface& IncomingSurface,
                                     const ThemeProfile& IncomingAppearance)
{
    if (Motion != nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "an editor panel construction stands" });

    Motion     = &IncomingMotion;
    Surface    = &IncomingSurface;
    Appearance = &IncomingAppearance;

    if (!Interaction.Construct(IncomingMotion).Resolved)
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "editor panel interaction was rejected" });

    if (!SharedControls.Construct(Interaction, IncomingSurface, IncomingAppearance).Resolved)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "shared editor controls were rejected" });

    if (!ScenePresentation.Construct(IncomingSurface, IncomingAppearance, LeafSubject::Scene).Resolved ||
        !UvPresentation.Construct(IncomingSurface, IncomingAppearance, LeafSubject::Uv).Resolved ||
        !OutlinerPresentation.Construct(IncomingSurface, IncomingAppearance, LeafSubject::Outliner).Resolved ||
        !PropertyPresentation.Construct(IncomingSurface, IncomingAppearance, LeafSubject::Property).Resolved)
    {
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "an editor leaf panel was rejected" });
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < ControlCapacity; ++Ordinal)
    {
        const Outcome<ControlIdentity> Registered = Interaction.Register();
        if (!Registered.Resolved)
            return Outcome<bool>::Refuse(Registered.Error);

        Controls[Ordinal] = Registered.Resolve();
    }

    return Outcome<bool>::Result(true);
}

void EditorPanel::Advance(const PointerCondition& Sampled, double Elapsed)
{
    Pointer = Sampled;
    Interaction.Advance(Sampled, Elapsed);
    SharedControls.Sample(Sampled);

    if (!Sampled.ContactHeld && !Sampled.ContactReleased)
        CapturedPresentation = AbsentPresentation;
}

std::uint32_t EditorPanel::ControlOrdinal(std::uint32_t RecordOrdinal, ControlRole Role) const
{
    return RecordOrdinal * ControlsPerRecord + static_cast<std::uint32_t>(Role);
}

bool EditorPanel::Pressed(std::uint32_t Ordinal, const PlaneExtent& Extent, bool PopupAction)
{
    if (Ordinal >= ControlCapacity)
        return false;

    const ControlIdentity Target = Controls[Ordinal];
    const bool BoundaryPresent = DeferredBoundary.Width() > 0.0f && DeferredBoundary.Height() > 0.0f;
    const bool WithinBoundary = !BoundaryPresent ||
                                DeferredBoundary.Encloses(Pointer.PositionX, Pointer.PositionY);
    const bool Hovered = WithinBoundary && Extent.Encloses(Pointer.PositionX, Pointer.PositionY);
    if (Hovered && Pointer.ContactPressed && (PopupAction || !Interaction.AnyDisclosed()) &&
        Interaction.Grab(Target, ControlPart::Body))
    {
        CapturedPresentation = CurrentPresentation;
    }

    Interaction.DeclareHovered(Target, Hovered, 130.0);
    return CapturedPresentation == CurrentPresentation && Interaction.Released(Target) && Hovered;
}

bool EditorPanel::Disclosed(ControlIdentity Target) const
{
    return DisclosedPresentation == CurrentPresentation && Interaction.Disclosed(Target);
}

void EditorPanel::Disclose(ControlIdentity Target)
{
    if (Interaction.Disclose(Target))
        DisclosedPresentation = CurrentPresentation;
}

void EditorPanel::CloseDisclosure()
{
    Interaction.Withdraw();
    DisclosedPresentation = AbsentPresentation;
}

void EditorPanel::Symbol(const PlaneExtent& Extent, ThemeToken Colour)
{
    Surface->Stroke(SymbolSubject::PlaceholderMark, Extent, Colour);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     PARTITION RECORDING
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> EditorPanel::Record(const PlaneExtent& Extent,
                                  PanelStructure& Partition,
                                  EditorPanelConfiguration& Configuration,
                                  std::uint32_t PresentationOrdinal,
                                  bool DeferPopups)
{
    if (Surface == nullptr || Appearance == nullptr || Motion == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no editor panel construction stands" });

    if (!Partition.Current(PanelStructure::RootOrdinal).Resolved)
        Partition.Construct();

    CurrentPresentation = PresentationOrdinal;
    DeferredAnchor       = {};
    DeferredBoundary     = {};
    DeferredRecord       = PanelStructure::RecordCeiling;
    DeferredRole         = ControlRole::RoleCount;
    LeafTally            = 0u;

    Surface->Ground(Extent, Appearance->EditorPanel.WindowGround);
    RecordBranch(PanelStructure::RootOrdinal, Extent, Partition, Configuration);

    // 🔴 The popups are deferred when the caller fills the leaves itself: recorded before the leaf
    //    content, a split or subject menu is painted over by the caller's sky quad and becomes
    //    unreadable. The host records its content between the two calls.
    if (!DeferPopups)
        RecordDeferred(Partition, Configuration);

    return Outcome<bool>::Result(true);
}

void EditorPanel::RecordDeferredPopups(PanelStructure& Partition, EditorPanelConfiguration& Configuration)
{
    RecordDeferred(Partition, Configuration);
}

bool EditorPanel::PointerCaptured(std::uint32_t PresentationOrdinal) const
{
    const bool ContactCurrent = Pointer.ContactHeld || Pointer.ContactReleased;
    const bool PresentationCaptured = CapturedPresentation == PresentationOrdinal;
    const bool PointerWithinPresentation = DeferredBoundary.Encloses(Pointer.PositionX, Pointer.PositionY);
    const bool PopupCaptured = DisclosedPresentation == PresentationOrdinal && Interaction.AnyDisclosed() &&
                               PointerWithinPresentation;
    return ContactCurrent && (PresentationCaptured || PopupCaptured);
}

void EditorPanel::WithdrawPresentation(std::uint32_t PresentationOrdinal)
{
    if (DisclosedPresentation == PresentationOrdinal)
        CloseDisclosure();
    else if (DisclosedPresentation != AbsentPresentation && DisclosedPresentation > PresentationOrdinal)
        --DisclosedPresentation;

    if (CapturedPresentation == PresentationOrdinal)
    {
        Interaction.Abandon();
        CapturedPresentation = AbsentPresentation;
    }
    else if (CapturedPresentation != AbsentPresentation && CapturedPresentation > PresentationOrdinal)
    {
        --CapturedPresentation;
    }
}

void EditorPanel::RecordBranch(std::uint32_t RecordOrdinal,
                               const PlaneExtent& Extent,
                               PanelStructure& Partition,
                               EditorPanelConfiguration& Configuration)
{
    const Outcome<PanelRecord> Delivered = Partition.Current(RecordOrdinal);
    if (!Delivered.Resolved)
        return;

    const PanelRecord Declared = Delivered.Resolve();
    if (!Declared.Divided)
    {
        RecordLeaf(RecordOrdinal, Declared, Extent, Partition, Configuration);
        return;
    }

    const EditorPanelMetric& Measure = Appearance->EditorPanelMeasure;
    const EditorPanelColour&    Colour     = Appearance->EditorPanel;
    const bool X = Declared.Axis == PanelDivisionAxis::X;
    const float Span = X ? Extent.Width() : Extent.Height();
    const float Available = (Span > Measure.SplitterHeight) ? Span - Measure.SplitterHeight : 0.0f;
    const float MinimumSpan = Available * Declared.MinimumFraction;

    PlaneExtent MinimumExtent = Extent;
    PlaneExtent SplitExtent = Extent;
    PlaneExtent MaximumExtent  = Extent;

    if (X)
    {
        MinimumExtent.MaximumX = Extent.MinimumX + MinimumSpan;
        SplitExtent.MinimumX = MinimumExtent.MaximumX;
        SplitExtent.MaximumX = SplitExtent.MinimumX + Measure.SplitterHeight;
        MaximumExtent.MinimumX = SplitExtent.MaximumX;
    }
    else
    {
        MinimumExtent.MaximumY = Extent.MinimumY + MinimumSpan;
        SplitExtent.MinimumY = MinimumExtent.MaximumY;
        SplitExtent.MaximumY = SplitExtent.MinimumY + Measure.SplitterHeight;
        MaximumExtent.MinimumY = SplitExtent.MaximumY;
    }

    const std::uint32_t SplitControl = ControlOrdinal(RecordOrdinal, ControlRole::DivisionMenu);
    const ControlIdentity Target = Controls[SplitControl];
    const bool Hovered = SplitExtent.Encloses(Pointer.PositionX, Pointer.PositionY);
    if (Hovered && Pointer.ContactPressed && !Interaction.AnyDisclosed() &&
        Interaction.Grab(Target, ControlPart::Body))
    {
        Interaction.RecordInitial(Target, Declared.MinimumFraction);
        CapturedPresentation = CurrentPresentation;
        DraggedDivision      = RecordOrdinal;
        DraggedExtent        = Extent;
    }

    const bool DivisionCaptured = CapturedPresentation == CurrentPresentation;
    const bool DivisionHeld = DivisionCaptured && Interaction.Holding(Target);
    const bool DivisionReleased = DivisionCaptured && Interaction.Released(Target);
    Interaction.DeclareHovered(Target, Hovered || DivisionHeld, 130.0);

    float RequestedFraction = Declared.MinimumFraction;
    if (DivisionHeld || DivisionReleased)
    {
        RequestedFraction = X
                          ? (Pointer.PositionX - DraggedExtent.MinimumX) / DraggedExtent.Width()
                          : (Pointer.PositionY - DraggedExtent.MinimumY) / DraggedExtent.Height();
    }

    if (DivisionReleased)
    {
        if (RequestedFraction < 0.05f)
        {
            Discard(Partition.Withdraw(Declared.Minimum));
            RecordBranch(RecordOrdinal, Extent, Partition, Configuration);
            return;
        }

        if (RequestedFraction > 0.95f)
        {
            Discard(Partition.Withdraw(Declared.Maximum));
            RecordBranch(RecordOrdinal, Extent, Partition, Configuration);
            return;
        }
    }

    if (DivisionHeld)
        Discard(Partition.Proportion(RecordOrdinal, RequestedFraction));

    Surface->Ground(SplitExtent, Hovered || DivisionHeld ? Colour.Accent : Colour.ChromeGround);
    Surface->Edge(SplitExtent, Colour.Edge, Measure.EdgeWeight);

    RecordBranch(Declared.Minimum, MinimumExtent, Partition, Configuration);
    RecordBranch(Declared.Maximum, MaximumExtent, Partition, Configuration);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        LEAF CHROME
//------------------------------------------------------------------------------------------------------------------------

void EditorPanel::RecordLeaf(std::uint32_t RecordOrdinal,
                             const PanelRecord& Declared,
                             const PlaneExtent& Extent,
                             PanelStructure& Partition,
                             EditorPanelConfiguration& Configuration)
{
    CurrentLeafExtent = Extent;

    if (Declared.Subject == PanelSubject::Vacant)
    {
        RecordVacant(RecordOrdinal, Extent, Partition);
        return;
    }

    const EditorPanelMetric& Measure = Appearance->EditorPanelMeasure;
    const PlaneExtent Header = Spanning(Extent.MinimumX, Extent.MinimumY,
                                        Extent.Width(), Measure.HeaderHeight);
    const PlaneExtent Footer = Spanning(Extent.MinimumX, Extent.MaximumY - Measure.FooterHeight,
                                        Extent.Width(), Measure.FooterHeight);
    const PlaneExtent Body = { Extent.MinimumX, Header.MaximumY, Extent.MaximumX, Footer.MinimumY };

    // 📝 The leaf is delivered to the host, which fills its body with the leaf's own content — the sky in
    //    a viewport leaf, the scene directory in an outliner or properties leaf. The tally grows in
    //    depth-first order and resets at the top of every `Record`.
    if (LeafTally < PanelStructure::RecordCeiling)
    {
        LeafBodies[LeafTally]   = Body;
        LeafSubjects[LeafTally] = Declared.Subject;
        ++LeafTally;
    }

    RecordHeader(RecordOrdinal, Declared.Subject, Header, Partition);

    // 📝 GPU scene and UV rendering are intentionally absent in this skeleton. Each focused panel owns its
    //    render-target body while `EditorPanel` owns only shared chrome and partition interaction.
    switch (Declared.Subject)
    {
        case PanelSubject::Viewport:   ScenePresentation.Record(Body);    break;
        case PanelSubject::Uv:         UvPresentation.Record(Body);       break;
        case PanelSubject::Outliner:   OutlinerPresentation.Record(Body); break;
        case PanelSubject::Properties: PropertyPresentation.Record(Body); break;
        default:                       break;
    }

    RecordFooter(RecordOrdinal, Declared.Subject, Footer, Configuration);
}

void EditorPanel::RecordHeader(std::uint32_t RecordOrdinal,
                               PanelSubject Subject,
                               const PlaneExtent& Extent,
                               PanelStructure& Partition)
{
    const EditorPanelMetric& Measure = Appearance->EditorPanelMeasure;
    const EditorPanelColour&    Colour     = Appearance->EditorPanel;
    Surface->Ground(Extent, Colour.ChromeGround);
    Surface->Ground(Spanning(Extent.MinimumX, Extent.MaximumY - Measure.EdgeWeight,
                             Extent.Width(), Measure.EdgeWeight), Colour.Edge);

    const PlaneExtent SubjectButton = Spanning(Extent.MinimumX + Measure.HeaderPadX,
                                               Extent.MinimumY + 2.0f,
                                               44.0f,
                                               Measure.HeaderAction);
    const std::uint32_t SubjectControl = ControlOrdinal(RecordOrdinal, ControlRole::SubjectMenu);
    const bool SubjectOpen = Disclosed(Controls[SubjectControl]);
    if (SubjectOpen)
        Surface->Ground(SubjectButton, Colour.Hovered, 4.0f, CornerAll);

    Symbol(Spanning(SubjectButton.MinimumX + 2.0f,
                    SubjectButton.MinimumY + 7.0f,
                    Measure.HeaderSymbol,
                    Measure.HeaderSymbol), Colour.ColourQuiet);
    Surface->TextRun(SubjectButton.MaximumX - 10.0f,
                     SubjectButton.MinimumY + 8.0f,
                     Colour.ColourFaint,
                     "v",
                     Measure.TextSmall,
                     0.0f,
                     true);

    if (Pressed(SubjectControl, SubjectButton, true))
    {
        if (SubjectOpen)
            CloseDisclosure();
        else
            Disclose(Controls[SubjectControl]);
    }

    if (Disclosed(Controls[SubjectControl]))
    {
        DeferredAnchor   = SubjectButton;
        DeferredBoundary = CurrentLeafExtent;
        DeferredRecord   = RecordOrdinal;
        DeferredRole     = ControlRole::SubjectMenu;
    }

    const bool CanRemove = Partition.RemovalAccepted();
    const float ActionCount = CanRemove ? 2.0f : 1.0f;
    const PlaneExtent DivisionButton = Spanning(Extent.MaximumX - Measure.HeaderPadX -
                                                   Measure.HeaderAction * ActionCount,
                                               Extent.MinimumY + 2.0f,
                                               Measure.HeaderAction,
                                               Measure.HeaderAction);
    const PlaneExtent TitleClip = { SubjectButton.MaximumX + Measure.HeaderTitleGap,
                                    Extent.MinimumY,
                                    DivisionButton.MinimumX - 4.0f,
                                    Extent.MaximumY };
    if (TitleClip.Width() > 0.0f)
    {
        Surface->Confine(TitleClip);
        Surface->TextRunTruncated(TitleClip.MinimumX,
                                  Extent.MinimumY + 10.0f,
                                  TitleClip.MaximumX,
                                  Colour.ColourSecondary,
                                  SubjectTitle(Subject),
                                  Measure.TextSmall,
                                  false);
        Surface->Release();
    }

    const std::uint32_t DivisionControl = ControlOrdinal(RecordOrdinal, ControlRole::DivisionMenu);
    const bool DivisionOpen = Disclosed(Controls[DivisionControl]);
    if (DivisionOpen)
        Surface->Ground(DivisionButton, Colour.Hovered, 6.0f, CornerAll);

    Symbol(Spanning(DivisionButton.MinimumX + 7.0f,
                    DivisionButton.MinimumY + 7.0f,
                    Measure.HeaderSymbol,
                    Measure.HeaderSymbol), Colour.ColourQuiet);

    if (Pressed(DivisionControl, DivisionButton, true))
    {
        if (DivisionOpen)
            CloseDisclosure();
        else
            Disclose(Controls[DivisionControl]);
    }

    if (Disclosed(Controls[DivisionControl]))
    {
        DeferredAnchor   = DivisionButton;
        DeferredBoundary = CurrentLeafExtent;
        DeferredRecord   = RecordOrdinal;
        DeferredRole     = ControlRole::DivisionMenu;
    }

    if (CanRemove)
    {
        const PlaneExtent WithdrawalButton = Spanning(DivisionButton.MaximumX,
                                                       DivisionButton.MinimumY,
                                                       Measure.HeaderAction,
                                                       Measure.HeaderAction);
        Symbol(Spanning(WithdrawalButton.MinimumX + 7.0f,
                        WithdrawalButton.MinimumY + 7.0f,
                        Measure.HeaderSymbol,
                        Measure.HeaderSymbol), Colour.ColourQuiet);
        if (Pressed(ControlOrdinal(RecordOrdinal, ControlRole::Withdrawal), WithdrawalButton))
            Discard(Partition.Withdraw(RecordOrdinal));
    }
}

void EditorPanel::RecordFooter(std::uint32_t RecordOrdinal,
                               PanelSubject Subject,
                               const PlaneExtent& Extent,
                               EditorPanelConfiguration& Configuration)
{
    const EditorPanelMetric& Measure = Appearance->EditorPanelMeasure;
    const EditorPanelColour&    Colour     = Appearance->EditorPanel;
    Surface->Ground(Extent, Colour.ChromeGround);
    Surface->Ground(Spanning(Extent.MinimumX, Extent.MinimumY,
                             Extent.Width(), Measure.EdgeWeight), Colour.Edge);

    if (Subject == PanelSubject::Outliner || Subject == PanelSubject::Properties)
    {
        Surface->TextRun(Extent.MinimumX + Measure.FooterPadX,
                         Extent.MinimumY + 18.0f,
                         Colour.ColourFaint,
                         Subject == PanelSubject::Outliner ? "0 items" : "No active object",
                         Measure.TextSmall,
                         0.0f,
                         false);
        return;
    }

    float Cursor = Extent.MinimumX + Measure.FooterPadX;
    const auto Pill = [&](const char* Caption, float X) -> PlaneExtent
    {
        const PlaneExtent Button = Spanning(Cursor, Extent.MinimumY + 10.0f, X, Measure.PillY);
        Surface->Ground(Button, Colour.BodyGround, Measure.PillRadius, CornerAll);
        Surface->Edge(Button, Colour.Edge, Measure.EdgeWeight, Measure.PillRadius, CornerAll);
        Symbol(Spanning(Button.MinimumX + 10.0f, Button.MinimumY + 7.0f,
                        Measure.HeaderSymbol, Measure.HeaderSymbol), Colour.ColourQuiet);
        Surface->TextRun(Button.MinimumX + 30.0f, Button.MinimumY + 8.0f,
                         Colour.ColourQuiet, Caption, Measure.TextSmall, 0.0f, false);
        Cursor = Button.MaximumX + Measure.FooterGap;
        return Button;
    };

    if (Subject == PanelSubject::Viewport)
    {
        const PlaneExtent Cameras = Pill("Cameras", 92.0f);
        const std::uint32_t CameraControl = ControlOrdinal(RecordOrdinal, ControlRole::CameraMenu);
        const bool CameraOpen = Disclosed(Controls[CameraControl]);
        if (Pressed(CameraControl, Cameras, true))
        {
            if (CameraOpen)
                CloseDisclosure();
            else
                Disclose(Controls[CameraControl]);
        }
        if (Disclosed(Controls[CameraControl]))
        {
            DeferredAnchor   = Cameras;
        DeferredBoundary = CurrentLeafExtent;
            DeferredRecord = RecordOrdinal;
            DeferredRole   = ControlRole::CameraMenu;
        }
    }

    const PlaneExtent LatticeButton = Pill("Grid", 72.0f);
    const std::uint32_t LatticeControl = ControlOrdinal(RecordOrdinal, ControlRole::LatticeMenu);
    const bool LatticeOpen = Disclosed(Controls[LatticeControl]);
    if (Pressed(LatticeControl, LatticeButton, true))
    {
        if (LatticeOpen)
            CloseDisclosure();
        else
            Disclose(Controls[LatticeControl]);
    }

    if (Disclosed(Controls[LatticeControl]))
    {
        DeferredAnchor   = LatticeButton;
        DeferredBoundary = CurrentLeafExtent;
        DeferredRecord   = RecordOrdinal;
        DeferredRole     = ControlRole::LatticeMenu;
    }

    const float TrailingFloor = Cursor + 20.0f;
    float Trailing = Extent.MaximumX - Measure.FooterPadX;
    const auto TrailingPill = [&](const char* Caption, float X, ThemeToken Accent) -> PlaneExtent
    {
        Trailing -= X;
        const PlaneExtent Button = Spanning(Trailing, Extent.MinimumY + 10.0f, X, Measure.PillY);
        Surface->Ground(Button, Colour.BodyGround, Measure.PillRadius, CornerAll);
        Surface->Edge(Button, Accent, Measure.EdgeWeight, Measure.PillRadius, CornerAll);
        Surface->TextRun(Button.MinimumX + Button.Width() * 0.5f,
                         Button.MinimumY + 8.0f,
                         Colour.ColourQuiet,
                         Caption,
                         Measure.TextSmall,
                         0.0f,
                         true);
        Trailing = Button.MinimumX - Measure.FooterGap;
        return Button;
    };

    if (Subject == PanelSubject::Viewport && Trailing > TrailingFloor + 250.0f)
    {
        const PlaneExtent GizmoButton = TrailingPill(GizmoTitle(Configuration.Gizmo), 78.0f, Colour.Edge);
        const std::uint32_t GizmoControl = ControlOrdinal(RecordOrdinal, ControlRole::Gizmo);
        if (Pressed(GizmoControl, GizmoButton, true))
            Disclose(Controls[GizmoControl]);
        if (Disclosed(Controls[GizmoControl]))
        {
            DeferredAnchor   = GizmoButton;
        DeferredBoundary = CurrentLeafExtent;
            DeferredRecord = RecordOrdinal;
            DeferredRole   = ControlRole::Gizmo;
        }

        const PlaneExtent ShadingButton = TrailingPill(ShadingTitle(Configuration.Shading), 86.0f, Colour.Edge);
        const std::uint32_t ShadingControl = ControlOrdinal(RecordOrdinal, ControlRole::Shading);
        if (Pressed(ShadingControl, ShadingButton, true))
            Disclose(Controls[ShadingControl]);
        if (Disclosed(Controls[ShadingControl]))
        {
            DeferredAnchor   = ShadingButton;
        DeferredBoundary = CurrentLeafExtent;
            DeferredRecord = RecordOrdinal;
            DeferredRole   = ControlRole::Shading;
        }

        const PlaneExtent CameraButton = TrailingPill(Configuration.Perspective ? "Persp" : "Ortho", 64.0f, Colour.Edge);
        if (Pressed(ControlOrdinal(RecordOrdinal, ControlRole::LatticePresentation), CameraButton))
            Configuration.Perspective = !Configuration.Perspective;

        const bool OverlaysTaken = Configuration.FpsOverlay || Configuration.StorageOverlay || Configuration.RendererOverlay;
        const PlaneExtent OverlayButton = TrailingPill("Overlays", 80.0f,
                                                       OverlaysTaken ? Colour.Positive : Colour.Edge);
        const std::uint32_t OverlayControl = ControlOrdinal(RecordOrdinal, ControlRole::OverlayMenu);
        if (Pressed(OverlayControl, OverlayButton, true))
            Disclose(Controls[OverlayControl]);
        if (Disclosed(Controls[OverlayControl]))
        {
            DeferredAnchor   = OverlayButton;
        DeferredBoundary = CurrentLeafExtent;
            DeferredRecord = RecordOrdinal;
            DeferredRole   = ControlRole::OverlayMenu;
        }
    }
    else if (Subject == PanelSubject::Uv && Trailing > TrailingFloor + 100.0f)
    {
        static_cast<void>(TrailingPill("2D View", 72.0f, Colour.Edge));
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    VACANT PANEL CHOOSER
//------------------------------------------------------------------------------------------------------------------------

void EditorPanel::RecordVacant(std::uint32_t RecordOrdinal,
                               const PlaneExtent& Extent,
                               PanelStructure& Partition)
{
    const EditorPanelMetric& Measure = Appearance->EditorPanelMeasure;
    const EditorPanelColour&    Colour     = Appearance->EditorPanel;
    Surface->Ground(Extent, Colour.WindowGround);

    if (Partition.RemovalAccepted())
    {
        const PlaneExtent Close = Spanning(Extent.MaximumX - 46.0f, Extent.MinimumY + 16.0f, 30.0f, 30.0f);
        Surface->Ground(Close, Colour.ViewGround, 8.0f, CornerAll);
        Surface->Edge(Close, Colour.Edge, Measure.EdgeWeight, 8.0f, CornerAll);
        Symbol(Spanning(Close.MinimumX + 7.0f, Close.MinimumY + 7.0f,
                        16.0f, 16.0f), Colour.ColourFaint);
        if (Pressed(ControlOrdinal(RecordOrdinal, ControlRole::Withdrawal), Close))
            Discard(Partition.Withdraw(RecordOrdinal));
    }

    const float HorizontalPad = 16.0f;
    const float AvailableX = (Extent.Width() > HorizontalPad * 2.0f)
                               ? Extent.Width() - HorizontalPad * 2.0f : Extent.Width();
    const std::uint32_t Columns = (AvailableX >= Measure.ChooserButtonX * 4.0f +
                                                     Measure.ChooserGap * 3.0f) ? 4u
                                : (AvailableX >= 212.0f) ? 2u : 1u;
    const std::uint32_t Rows = (4u + Columns - 1u) / Columns;
    const float ButtonX = (Measure.ChooserButtonX <
                              (AvailableX - Measure.ChooserGap * static_cast<float>(Columns - 1u)) /
                                  static_cast<float>(Columns))
                            ? Measure.ChooserButtonX
                            : (AvailableX - Measure.ChooserGap * static_cast<float>(Columns - 1u)) /
                                  static_cast<float>(Columns);
    const float AvailableY = (Extent.Height() > 96.0f)
                                ? Extent.Height() - 96.0f : Extent.Height();
    const float ButtonY = (Measure.ChooserButtonHeight <
                               (AvailableY - Measure.ChooserGap * static_cast<float>(Rows - 1u)) /
                                   static_cast<float>(Rows))
                             ? Measure.ChooserButtonHeight
                             : (AvailableY - Measure.ChooserGap * static_cast<float>(Rows - 1u)) /
                                   static_cast<float>(Rows);
    const float TotalX = ButtonX * static_cast<float>(Columns) +
                             Measure.ChooserGap * static_cast<float>(Columns - 1u);
    const float TotalY = ButtonY * static_cast<float>(Rows) +
                              Measure.ChooserGap * static_cast<float>(Rows - 1u);
    const float MinimumX = Extent.MinimumX + (Extent.Width() - TotalX) * 0.5f;
    const float MinimumY = Extent.MinimumY + (Extent.Height() - TotalY) * 0.5f + 14.0f;

    Surface->Confine(Extent);
    Surface->TextRun(Extent.MinimumX + Extent.Width() * 0.5f,
                     MinimumY - 34.0f,
                     Colour.ColourSecondary,
                     "Choose Panel Type",
                     Measure.TextBody,
                     0.0f,
                     true);

    const PanelSubject Subjects[4] = { PanelSubject::Viewport, PanelSubject::Uv,
                                       PanelSubject::Outliner, PanelSubject::Properties };
    const ControlRole Roles[4] = { ControlRole::ChooseViewport, ControlRole::ChooseUv,
                                   ControlRole::ChooseOutliner, ControlRole::ChooseProperties };

    for (std::uint32_t Ordinal = 0u; Ordinal < 4u; ++Ordinal)
    {
        const std::uint32_t Column = Ordinal % Columns;
        const std::uint32_t Row = Ordinal / Columns;
        const PlaneExtent Button = Spanning(MinimumX + static_cast<float>(Column) *
                                                        (ButtonX + Measure.ChooserGap),
                                            MinimumY + static_cast<float>(Row) *
                                                        (ButtonY + Measure.ChooserGap),
                                            ButtonX,
                                            ButtonY);
        Surface->Ground(Button, Colour.BodyGround, Measure.ChooserRadius, CornerAll);
        Surface->Edge(Button, Colour.Edge, Measure.EdgeWeight, Measure.ChooserRadius, CornerAll);
        const float SymbolY = Button.MinimumY + ((ButtonY > 64.0f) ? 18.0f : 8.0f);
        Symbol(Spanning(Button.MinimumX + Button.Width() * 0.5f - 12.0f,
                        SymbolY,
                        24.0f,
                        24.0f), Colour.ColourFaint);
        const PlaneExtent CaptionClip = { Button.MinimumX + 6.0f,
                                          Button.MinimumY,
                                          Button.MaximumX - 6.0f,
                                          Button.MaximumY };
        Surface->Confine(CaptionClip);
        Surface->TextRunTruncated(Button.MinimumX + Button.Width() * 0.5f,
                                  Button.MaximumY - 27.0f,
                                  CaptionClip.MaximumX,
                                  Colour.ColourQuiet,
                                  SubjectTitle(Subjects[Ordinal]),
                                  Measure.TextSmall,
                                  true);
        Surface->Release();

        if (Pressed(ControlOrdinal(RecordOrdinal, Roles[Ordinal]), Button))
            Discard(Partition.Assign(RecordOrdinal, Subjects[Ordinal]));
    }

    Surface->Release();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     DEFERRED MENUS
//------------------------------------------------------------------------------------------------------------------------

void EditorPanel::RecordDeferred(PanelStructure& Partition, EditorPanelConfiguration& Configuration)
{
    if (DeferredRecord >= PanelStructure::RecordCeiling)
        return;

    const bool BoundaryPresent = DeferredBoundary.Width() > 0.0f && DeferredBoundary.Height() > 0.0f;
    if (BoundaryPresent && Pointer.ContactPressed &&
        !DeferredBoundary.Encloses(Pointer.PositionX, Pointer.PositionY))
    {
        CloseDisclosure();
        return;
    }

    if (BoundaryPresent)
        Surface->Confine(DeferredBoundary);

    switch (DeferredRole)
    {
        case ControlRole::SubjectMenu:
            RecordSubjectMenu(DeferredRecord, DeferredAnchor, Partition);
            break;
        case ControlRole::DivisionMenu:
            RecordDivisionMenu(DeferredRecord, DeferredAnchor, Partition);
            break;
        case ControlRole::LatticeMenu:
            RecordLatticeMenu(DeferredRecord, DeferredAnchor, Configuration);
            break;
        case ControlRole::CameraMenu:
        case ControlRole::OverlayMenu:
        case ControlRole::Shading:
        case ControlRole::Gizmo:
            RecordFooterMenu(DeferredRecord, DeferredAnchor, DeferredRole, Configuration);
            break;
        default:
            break;
    }

    if (BoundaryPresent)
        Surface->Release();
}

void EditorPanel::RecordSubjectMenu(std::uint32_t RecordOrdinal,
                                    const PlaneExtent& Anchor,
                                    PanelStructure& Partition)
{
    const EditorPanelMetric& Measure = Appearance->EditorPanelMeasure;
    const EditorPanelColour&    Colour     = Appearance->EditorPanel;
    const float MenuX = (Measure.MenuX < DeferredBoundary.Width())
                          ? Measure.MenuX : DeferredBoundary.Width();
    const float DesiredMinimum = Anchor.MinimumX;
    const float MenuTop = (DesiredMinimum + MenuX > DeferredBoundary.MaximumX)
                          ? DeferredBoundary.MaximumX - MenuX
                          : (DesiredMinimum < DeferredBoundary.MinimumX)
                          ? DeferredBoundary.MinimumX : DesiredMinimum;
    const PlaneExtent Menu = Spanning(MenuTop,
                                      Anchor.MaximumY + Measure.MenuLift,
                                      MenuX,
                                      Measure.MenuPadY * 2.0f + Measure.MenuRowHeight * 4.0f);
    Surface->Ground(Menu, Colour.ChromeGround, Measure.MenuRadius, CornerAll);
    Surface->Edge(Menu, Colour.Edge, Measure.EdgeWeight, Measure.MenuRadius, CornerAll);

    const PanelSubject Subjects[4] = { PanelSubject::Viewport, PanelSubject::Uv,
                                       PanelSubject::Outliner, PanelSubject::Properties };
    const ControlRole Roles[4] = { ControlRole::ChooseViewport, ControlRole::ChooseUv,
                                   ControlRole::ChooseOutliner, ControlRole::ChooseProperties };

    for (std::uint32_t Ordinal = 0u; Ordinal < 4u; ++Ordinal)
    {
        const PlaneExtent Row = Spanning(Menu.MinimumX + Measure.MenuPadY,
                                         Menu.MinimumY + Measure.MenuPadY +
                                             static_cast<float>(Ordinal) * Measure.MenuRowHeight,
                                         Menu.Width() - Measure.MenuPadY * 2.0f,
                                         Measure.MenuRowHeight);
        Symbol(Spanning(Row.MinimumX + 8.0f, Row.MinimumY + 7.0f,
                        Measure.HeaderSymbol, Measure.HeaderSymbol), Colour.ColourQuiet);
        Surface->TextRun(Row.MinimumX + 30.0f, Row.MinimumY + 7.0f,
                         Colour.ColourQuiet, SubjectTitle(Subjects[Ordinal]), Measure.TextBody, 0.0f, false);
        if (Pressed(ControlOrdinal(RecordOrdinal, Roles[Ordinal]), Row, true))
        {
            Discard(Partition.Assign(RecordOrdinal, Subjects[Ordinal]));
            CloseDisclosure();
        }
    }
}

void EditorPanel::RecordDivisionMenu(std::uint32_t RecordOrdinal,
                                     const PlaneExtent& Anchor,
                                     PanelStructure& Partition)
{
    const EditorPanelMetric& Measure = Appearance->EditorPanelMeasure;
    const EditorPanelColour&    Colour     = Appearance->EditorPanel;
    const float MenuX = (Measure.SplitMenuX < DeferredBoundary.Width())
                          ? Measure.SplitMenuX : DeferredBoundary.Width();
    const float DesiredMinimum = Anchor.MaximumX - MenuX;
    const float MenuTop = (DesiredMinimum + MenuX > DeferredBoundary.MaximumX)
                          ? DeferredBoundary.MaximumX - MenuX
                          : (DesiredMinimum < DeferredBoundary.MinimumX)
                          ? DeferredBoundary.MinimumX : DesiredMinimum;
    const PlaneExtent Menu = Spanning(MenuTop,
                                      Anchor.MaximumY + Measure.MenuLift,
                                      MenuX,
                                      Measure.MenuPadY * 2.0f + Measure.MenuRowHeight * 4.0f + 1.0f);
    Surface->Ground(Menu, Colour.ChromeGround, Measure.MenuRadius, CornerAll);
    Surface->Edge(Menu, Colour.Edge, Measure.EdgeWeight, Measure.MenuRadius, CornerAll);

    const char* Captions[4] = { "Split Left", "Split Right", "Split Top", "Split Bottom" };
    const ControlRole Roles[4] = { ControlRole::DivideLeft, ControlRole::DivideRight,
                                   ControlRole::DivideUpper, ControlRole::DivideLower };

    for (std::uint32_t Ordinal = 0u; Ordinal < 4u; ++Ordinal)
    {
        const PlaneExtent Row = Spanning(Menu.MinimumX + Measure.MenuPadY,
                                         Menu.MinimumY + Measure.MenuPadY +
                                             static_cast<float>(Ordinal) * Measure.MenuRowHeight,
                                         Menu.Width() - Measure.MenuPadY * 2.0f,
                                         Measure.MenuRowHeight);
        Symbol(Spanning(Row.MinimumX + 8.0f, Row.MinimumY + 7.0f,
                        Measure.HeaderSymbol, Measure.HeaderSymbol), Colour.ColourQuiet);
        Surface->TextRun(Row.MinimumX + 30.0f, Row.MinimumY + 7.0f,
                         Colour.ColourQuiet, Captions[Ordinal], Measure.TextBody, 0.0f, false);
        if (Pressed(ControlOrdinal(RecordOrdinal, Roles[Ordinal]), Row, true))
        {
            const PanelDivisionAxis Axis = Ordinal < 2u ? PanelDivisionAxis::X : PanelDivisionAxis::Y;
            const PanelDivisionSide Side = (Ordinal == 0u || Ordinal == 2u)
                                         ? PanelDivisionSide::Minimum : PanelDivisionSide::Maximum;
            Discard(Partition.Divide(RecordOrdinal, Axis, Side));
            CloseDisclosure();
        }
    }
}

void EditorPanel::RecordLatticeMenu(std::uint32_t RecordOrdinal,
                                 const PlaneExtent& Anchor,
                                 EditorPanelConfiguration& Configuration)
{
    const EditorPanelMetric& Measure = Appearance->EditorPanelMeasure;
    const EditorPanelColour&    Colour     = Appearance->EditorPanel;
    const float MenuX = (360.0f < DeferredBoundary.Width()) ? 360.0f : DeferredBoundary.Width();
    const float DesiredMinimum = Anchor.MinimumX;
    const float MenuTop = (DesiredMinimum + MenuX > DeferredBoundary.MaximumX)
                          ? DeferredBoundary.MaximumX - MenuX
                          : (DesiredMinimum < DeferredBoundary.MinimumX)
                          ? DeferredBoundary.MinimumX : DesiredMinimum;
    const PlaneExtent Menu = Spanning(MenuTop,
                                      Anchor.MinimumY - 316.0f,
                                      MenuX,
                                      304.0f);
    Surface->Ground(Menu, Colour.ChromeGround, 12.0f, CornerAll);
    Surface->Edge(Menu, Colour.Edge, Measure.EdgeWeight, 12.0f, CornerAll);
    Surface->TextRun(Menu.MinimumX + 20.0f, Menu.MinimumY + 18.0f,
                     Colour.ColourPrimary, "Grid settings", Measure.TextBody, 0.0f, false);
    Surface->Ground(Spanning(Menu.MinimumX + 20.0f, Menu.MinimumY + 44.0f,
                             Menu.Width() - 40.0f, 1.0f), Colour.Edge);

    const char* LatticeOptions[4] = { "None", "Lines", "Dotted", "Lines + Dots" };
    SelectionDeclaration LatticeDeclaration;
    LatticeDeclaration.Caption     = "Grid";
    LatticeDeclaration.Options     = LatticeOptions;
    LatticeDeclaration.OptionCount = 4u;
    std::uint32_t LatticeReading = static_cast<std::uint32_t>(Configuration.Lattice);
    SharedControls.SelectionField(Controls[ControlOrdinal(RecordOrdinal, ControlRole::LatticePresentation)],
                                  Spanning(Menu.MinimumX + 20.0f, Menu.MinimumY + 58.0f,
                                           Menu.Width() - 40.0f, 36.0f),
                                  LatticeDeclaration,
                                  LatticeReading);
    Configuration.Lattice = static_cast<PanelLatticePresentation>(LatticeReading);

    MagnitudeDeclaration ScaleDeclaration;
    ScaleDeclaration.Caption     = "Scale";
    ScaleDeclaration.UnitGlyph   = "m";
    ScaleDeclaration.Minimum= 1.0;
    ScaleDeclaration.Maximum = 10.0;
    double ScaleReading = static_cast<double>(Configuration.LatticeScale);
    SharedControls.MagnitudeRow(Controls[ControlOrdinal(RecordOrdinal, ControlRole::LatticeScale)],
                                Spanning(Menu.MinimumX + 20.0f, Menu.MinimumY + 106.0f,
                                         Menu.Width() - 40.0f, 36.0f),
                                ScaleDeclaration,
                                ScaleReading,
                                true);
    Configuration.LatticeScale = static_cast<std::uint32_t>(std::round(ScaleReading));

    MagnitudeDeclaration SubdivisionDeclaration;
    SubdivisionDeclaration.Caption      = "Subdivisions";
    SubdivisionDeclaration.UnitGlyph    = "";
    SubdivisionDeclaration.Minimum = 1.0;
    SubdivisionDeclaration.Maximum  = 100.0;
    double SubdivisionReading = static_cast<double>(Configuration.Subdivisions);
    SharedControls.MagnitudeRow(Controls[ControlOrdinal(RecordOrdinal, ControlRole::Subdivisions)],
                                Spanning(Menu.MinimumX + 20.0f, Menu.MinimumY + 154.0f,
                                         Menu.Width() - 40.0f, 36.0f),
                                SubdivisionDeclaration,
                                SubdivisionReading,
                                true);
    Configuration.Subdivisions = static_cast<std::uint32_t>(std::round(SubdivisionReading));

    ToggleDeclaration AxisDeclarations[3];
    AxisDeclarations[0].Caption = "X axis";
    AxisDeclarations[1].Caption = "Y axis";
    AxisDeclarations[2].Caption = "Z axis";
    bool* AxisReadings[3] = { &Configuration.AxisX, &Configuration.AxisY, &Configuration.AxisZ };
    const ControlRole AxisRoles[3] = { ControlRole::AxisX, ControlRole::AxisY, ControlRole::AxisZ };
    for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
    {
        SharedControls.ToggleRow(Controls[ControlOrdinal(RecordOrdinal, AxisRoles[Ordinal])],
                                 Spanning(Menu.MinimumX + 20.0f + static_cast<float>(Ordinal) * 106.0f,
                                          Menu.MinimumY + 214.0f,
                                          96.0f,
                                          44.0f),
                                 AxisDeclarations[Ordinal],
                                 *AxisReadings[Ordinal]);
    }
}

void EditorPanel::RecordFooterMenu(std::uint32_t RecordOrdinal,
                                   const PlaneExtent& Anchor,
                                   ControlRole Role,
                                   EditorPanelConfiguration& Configuration)
{
    const EditorPanelMetric& Measure = Appearance->EditorPanelMeasure;
    const EditorPanelColour&    Colour     = Appearance->EditorPanel;
    const auto FitExtent = [&](float DesiredMinimum,
                               float DesiredX,
                               float MinimumY,
                               float Height) -> PlaneExtent
    {
        const float Width = (DesiredX < DeferredBoundary.Width())
                                ? DesiredX : DeferredBoundary.Width();
        const float MinimumX = (DesiredMinimum + Width > DeferredBoundary.MaximumX)
                               ? DeferredBoundary.MaximumX - Width
                               : (DesiredMinimum < DeferredBoundary.MinimumX)
                               ? DeferredBoundary.MinimumX : DesiredMinimum;
        return Spanning(MinimumX, MinimumY, Width, Height);
    };

    if (Role == ControlRole::CameraMenu)
    {
        const PlaneExtent Menu = FitExtent(Anchor.MinimumX, 240.0f, Anchor.MinimumY - 116.0f, 104.0f);
        Surface->Ground(Menu, Colour.ChromeGround, 12.0f, CornerAll);
        Surface->Edge(Menu, Colour.Edge, Measure.EdgeWeight, 12.0f, CornerAll);
        Surface->TextRun(Menu.MinimumX + 12.0f, Menu.MinimumY + 14.0f,
                         Colour.ColourSecondary, "Saved Cameras", Measure.TextSmall, 0.0f, false);
        Surface->TextRun(Menu.MaximumX - 12.0f, Menu.MinimumY + 14.0f,
                         Colour.Accent, "+ Save", Measure.TextSmall, 0.0f, true);
        Surface->Ground(Spanning(Menu.MinimumX + 12.0f, Menu.MinimumY + 38.0f,
                                 Menu.Width() - 24.0f, 1.0f), Colour.Edge);
        Surface->TextRun(Menu.MinimumX + Menu.Width() * 0.5f, Menu.MinimumY + 70.0f,
                         Colour.ColourFaint, "No saved cameras", Measure.TextSmall, 0.0f, true);
        return;
    }

    if (Role == ControlRole::OverlayMenu)
    {
        const PlaneExtent Menu = FitExtent(Anchor.MaximumX - 200.0f,
                                           200.0f,
                                           Anchor.MinimumY - 132.0f,
                                           120.0f);
        Surface->Ground(Menu, Colour.ChromeGround, 12.0f, CornerAll);
        Surface->Edge(Menu, Colour.Edge, Measure.EdgeWeight, 12.0f, CornerAll);

        ToggleDeclaration Declarations[3];
        Declarations[0].Caption = "FPS Monitor";
        Declarations[1].Caption = "Storage Allocation";
        Declarations[2].Caption = "GPU Renderer";
        bool* Readings[3] = { &Configuration.FpsOverlay, &Configuration.StorageOverlay, &Configuration.RendererOverlay };
        const ControlRole Roles[3] = { ControlRole::AxisX, ControlRole::AxisY, ControlRole::AxisZ };
        for (std::uint32_t Ordinal = 0u; Ordinal < 3u; ++Ordinal)
        {
            SharedControls.ToggleRow(Controls[ControlOrdinal(RecordOrdinal, Roles[Ordinal])],
                                     Spanning(Menu.MinimumX + 8.0f,
                                              Menu.MinimumY + 6.0f + static_cast<float>(Ordinal) * 36.0f,
                                              Menu.Width() - 16.0f,
                                              34.0f),
                                     Declarations[Ordinal],
                                     *Readings[Ordinal]);
        }
        return;
    }

    const std::uint32_t OptionCount = Role == ControlRole::Shading ? 6u : 2u;
    const float MenuX = Role == ControlRole::Shading ? 160.0f : 130.0f;
    const PlaneExtent Menu = FitExtent(Anchor.MaximumX - MenuX,
                                       MenuX,
                                       Anchor.MinimumY - Measure.MenuPadY * 2.0f -
                                           Measure.MenuRowHeight * static_cast<float>(OptionCount) - 12.0f,
                                       Measure.MenuPadY * 2.0f +
                                           Measure.MenuRowHeight * static_cast<float>(OptionCount));
    Surface->Ground(Menu, Colour.ChromeGround, Measure.MenuRadius, CornerAll);
    Surface->Edge(Menu, Colour.Edge, Measure.EdgeWeight, Measure.MenuRadius, CornerAll);

    const char* ShadingOptions[6] = { "solid", "wireframe", "matcap", "normal", "metallic", "gi" };
    const char* GizmoOptions[2] = { "blender", "cad" };
    const ControlRole OptionRoles[6] = { ControlRole::DivideLeft, ControlRole::DivideRight,
                                         ControlRole::DivideUpper, ControlRole::DivideLower,
                                         ControlRole::ChooseViewport, ControlRole::ChooseUv };
    for (std::uint32_t Ordinal = 0u; Ordinal < OptionCount; ++Ordinal)
    {
        const PlaneExtent Row = Spanning(Menu.MinimumX + Measure.MenuPadY,
                                         Menu.MinimumY + Measure.MenuPadY +
                                             static_cast<float>(Ordinal) * Measure.MenuRowHeight,
                                         Menu.Width() - Measure.MenuPadY * 2.0f,
                                         Measure.MenuRowHeight);
        const bool Taken = Role == ControlRole::Shading
                         ? static_cast<std::uint32_t>(Configuration.Shading) == Ordinal
                         : static_cast<std::uint32_t>(Configuration.Gizmo) == Ordinal;
        if (Taken)
            Surface->Ground(Row, Colour.Hovered, 4.0f, CornerAll);
        Surface->TextRun(Row.MinimumX + 12.0f, Row.MinimumY + 7.0f,
                         Taken ? Colour.ColourPrimary : Colour.ColourQuiet,
                         Role == ControlRole::Shading ? ShadingOptions[Ordinal] : GizmoOptions[Ordinal],
                         Measure.TextBody,
                         0.0f,
                         false);
        if (Pressed(ControlOrdinal(RecordOrdinal, OptionRoles[Ordinal]), Row, true))
        {
            if (Role == ControlRole::Shading)
                Configuration.Shading = static_cast<PanelShading>(Ordinal);
            else
                Configuration.Gizmo = static_cast<PanelGizmo>(Ordinal);
            CloseDisclosure();
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

void EditorPanel::Reset()
{
    PropertyPresentation.Reset();
    OutlinerPresentation.Reset();
    UvPresentation.Reset();
    ScenePresentation.Reset();
    SharedControls.Reset();
    Interaction.Reset();
    Motion          = nullptr;
    Surface         = nullptr;
    Appearance      = nullptr;
    Pointer                = {};
    CurrentLeafExtent      = {};
    DeferredAnchor         = {};
    DeferredBoundary       = {};
    DeferredRecord         = PanelStructure::RecordCeiling;
    DeferredRole           = ControlRole::RoleCount;
    CurrentPresentation    = 0u;
    CapturedPresentation   = AbsentPresentation;
    DisclosedPresentation  = AbsentPresentation;
    DraggedDivision        = PanelStructure::RecordCeiling;
    DraggedExtent          = {};

    for (std::uint32_t Ordinal = 0u; Ordinal < ControlCapacity; ++Ordinal)
        Controls[Ordinal] = {};
}

}   // namespace Slate
