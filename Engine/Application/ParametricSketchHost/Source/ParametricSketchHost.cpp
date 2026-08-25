//============================================================================================================================================
//                                                        PARAMETRICSKETCHHOST.CPP
//============================================================================================================================================
// 🧩 Dedicated bring-up host for the parametric workspace path: docked workspace chrome, the dedicated CAD
//    outliner and Properties | Revision leaves, and host-owned exact-record / revision state bridged into the
//    UI guarantee. The CAD render pass remains a later phase; viewport leaves are placeholders for now.

#include "Foundation/DeliveryOutcome.h"
#include "Application/Api/ParametricWorkspaceBridge.h"
#include "SlateFeature/Feature/WorkspaceDirectoryProjection/Api/WorkspaceDirectoryProjection.h"
#include "SlateFeature/Feature/WorkspaceNameIndex/Api/WorkspaceNameIndex.h"
#include "SlateFeature/Feature/WorkspacePropertyProjection/Api/WorkspacePropertyProjection.h"
#include "SlateFeature/Feature/WorkspaceRecordStructure/Api/WorkspaceRecordStructure.h"
#include "SlateFeature/Feature/WorkspaceRevisionSequence/Api/WorkspaceRevisionSequence.h"
#include "SlateFeature/Sketch/SketchEditing/Api/SketchEditing.h"
#include "SlateFeature/Sketch/SketchPolyline/Api/SketchPolyline.h"
#include "SlateFeature/Sketch/SketchRenderingProjection/Api/SketchRenderingProjection.h"
#include "SlateFeature/Sketch/SketchSelection/Api/SketchSelection.h"
#include "SlateFeature/Sketch/SketchStructure/Api/SketchStructure.h"
#include "SlateUI/Interface/ContentBrowserPanel/Api/ContentBrowserPanel.h"
#include "SlateUI/Interface/ControlCentrePanel/Api/ControlCentrePanel.h"
#include "SlateUI/Interface/EditorPanel/Api/EditorPanel.h"
#include "SlateUI/Interface/ParametricWorkspace/Api/ParametricWorkspacePanel.h"
#include "SlateUI/Interface/ThemeInterchange/Api/ThemeInterchange.h"
#include "SlateUI/Interface/ViewportSequence/Api/ViewportSequence.h"
#include "SlateUI/Interface/WorkspacePanel/Api/WorkspaceIndex.h"
#include "Shared/OverlayGeometry.slang.h"
#include "SlateVulkan/Device/HostLifecycle/Api/HostLifecycle.h"
#include "SlateVulkan/Device/ShaderCodec/Api/ShaderCodec.h"
#include "SlateVulkan/Device/WorkspaceCadPass/Api/WorkspaceCadPass.h"
#include "SlateVulkan/Device/WorkspaceOverlayPass/Api/WorkspaceOverlayPass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <utility>
#if defined(_WIN32)
    #include <windows.h>
#else
    #include <unistd.h>
#endif
#include <vector>

namespace
{

using namespace Slate;

constexpr std::uint32_t InitialWidth  = 1440u;
constexpr std::uint32_t InitialHeight = 900u;
constexpr const char* WindowTitle = "Slate — Parametric Sketch";
constexpr const char* HostName = "ParametricSketchHost";
constexpr float WorkspaceGround[4] = { 0.06f, 0.06f, 0.08f, 1.0f };

constexpr double Pi = 3.14159265358979323846;
constexpr double CadPerspectiveFieldOfViewDegrees = 42.0;

struct SpatialBasis
{
    SpatialPoint Origin = {};
    SpatialDirection Along = { 1.0, 0.0, 0.0 };
    SpatialDirection Across = { 0.0, 0.0, 1.0 };
    SpatialDirection Normal = { 0.0, 1.0, 0.0 };
};

enum class ParametricViewOrientation : std::uint32_t
{
    Top = 0u,
    Bottom = 1u,
    Front = 2u,
    Back = 3u,
    Left = 4u,
    Right = 5u,
    Isometric = 6u
};

struct ParametricViewportState
{
    ParametricViewOrientation Orientation = ParametricViewOrientation::Top;
    SpatialPoint Focus = {};
    double Distance = 240.0;
    double OrthoScale = 3.0;
    double OrbitYaw = 45.0;
    double OrbitPitch = 30.0;
};

enum class ParametricDraftSubject : std::uint32_t
{
    None = 0u,
    Line = 1u,
    Rectangle = 2u,
    Circle = 3u
};

struct ParametricDraftState
{
    ParametricDraftSubject Subject = ParametricDraftSubject::None;
    std::vector<SpatialPoint> Anchors = {};
    bool HoverStanding = false;
    SpatialPoint Hover = {};
    SketchSnapPlacement Snap = {};
};

enum class ParametricSelectionSubject : std::uint32_t
{
    None = 0u,
    Point = 1u,
    Control = 2u,
    Curve = 3u,
    Record = 4u
};

enum class ParametricTransformMode : std::uint32_t
{
    Move = 0u,
    Rotate = 1u,
    Scale = 2u
};

enum class ParametricTransformConstraint : std::uint32_t
{
    Free = 0u,
    AxisX = 1u,
    AxisZ = 2u,
    Screen = 3u,
    Curve = 4u
};

struct ParametricViewportSelection
{
    ParametricSelectionSubject Subject = ParametricSelectionSubject::None;
    WorkspaceRecordName Record = {};
    SketchPointName Point = {};
    SketchControlName Control = {};
    SketchCurveName Curve = {};
    SpatialPoint Position = {};

    bool Standing() const
    {
        return Subject != ParametricSelectionSubject::None;
    }
};

struct ParametricTransformPlacement
{
    bool ControlPlacement = false;
    SketchPointName Point = {};
    SketchControlName Control = {};
    SpatialPoint Position = {};
};

struct ParametricTransformState
{
    ParametricTransformMode Mode = ParametricTransformMode::Move;
    bool Engaged = false;
    bool AwaitingRelease = false;
    bool Changed = false;
    bool SlideAlongCurve = false;
    ParametricTransformConstraint Constraint = ParametricTransformConstraint::Free;
    ParametricViewportSelection Target = {};
    WorkspaceRecordName Record = {};
    std::vector<ParametricTransformPlacement> Placements = {};
    std::vector<SpatialPoint> Origins = {};
    SpatialPoint Pivot = {};
    SpatialPoint StartReference = {};
    double PivotAlong = 0.0;
    double PivotAcross = 0.0;
    double StartAlong = 0.0;
    double StartAcross = 0.0;
    double StartDistance = 1.0;
    double StartAngle = 0.0;
    SpatialDirection CurveDirection = { 1.0, 0.0, 0.0 };
    char Numeric[32] = {};
    double PreviewValue = 0.0;
};

struct ParametricTransformCommandInput
{
    std::uint32_t MoveTapCount = 0u;
    bool StartRequested = false;
    ParametricTransformMode StartMode = ParametricTransformMode::Move;
    bool ConstraintRequested = false;
    ParametricTransformConstraint Constraint = ParametricTransformConstraint::Free;
    char NumericAppend[32] = {};
};

std::string ShaderStreamDirectory()
{
    std::error_code Error;
    std::filesystem::path Binary;

#if defined(_WIN32)
    {
        wchar_t Executable[32768] = {};
        const DWORD Written = GetModuleFileNameW(nullptr, Executable, 32768);
        if (Written > 0u && Written < 32768u)
            Binary = std::filesystem::path(Executable).parent_path();
    }
#endif

    if (Binary.empty())
    {
#if !defined(_WIN32)
        std::vector<char> Executable(32768, '\0');
        const std::size_t Written = readlink("/proc/self/exe", Executable.data(), Executable.size());
        if (Written > 0u && Written < Executable.size())
            Binary = std::filesystem::path(std::string(Executable.data(), Written)).parent_path();
#endif
    }

    if (Binary.empty())
        Binary = std::filesystem::current_path(Error);

    return (Binary / ".." / "Shader").lexically_normal().string();
}

InterfaceAttachment Attach(const DeviceOffering& Offered)
{
    InterfaceAttachment Incoming = {};
    Incoming.Instance = Offered.Instance;
    Incoming.ScoredDevice = Offered.ScoredDevice;
    Incoming.ActiveDevice = Offered.ActiveDevice;
    Incoming.GraphicsQueue = Offered.GraphicsQueue;
    Incoming.GraphicsFamilyIndex = Offered.GraphicsFamilyIndex;
    Incoming.ColourTargetFormat = Offered.ColourTargetFormat;
    Incoming.MinimumDisplayImageCount = Offered.MinimumDisplayImageCount;
    Incoming.DisplayImageCount = Offered.DisplayImageCount;
    Incoming.NativeWindowSlot = Offered.NativeWindowSlot;
    return Incoming;
}

void PopulateImportDirectory(ContentBrowserConfiguration& Browser, const std::filesystem::path& Requested)
{
    std::error_code Error;
    std::filesystem::path Resolved = Requested;
    if (Requested == "Home")
    {
#if defined(_WIN32)
        const char* Home = std::getenv("USERPROFILE");
#else
        const char* Home = std::getenv("HOME");
#endif
        if (Home != nullptr && Home[0] != '\0') Resolved = Home;
    }
    if (Resolved.empty()) Resolved = std::filesystem::current_path(Error);

    Browser.ImportEntryCount = 0u;
    Browser.ImportTaken = ContentLibrary::AbsentIndex;
    std::snprintf(Browser.ImportLocation, sizeof(Browser.ImportLocation), "%s", Resolved.generic_string().c_str());
    if (Error || !std::filesystem::is_directory(Resolved, Error) || Error) return;

    std::vector<std::filesystem::directory_entry> Entries;
    for (std::filesystem::directory_iterator Current(Resolved, Error), End; !Error && Current != End; Current.increment(Error))
        Entries.push_back(*Current);
    std::sort(Entries.begin(), Entries.end(), [](const auto& Left, const auto& Right)
    {
        const bool LeftDirectory = Left.is_directory();
        const bool RightDirectory = Right.is_directory();
        return LeftDirectory != RightDirectory ? LeftDirectory : Left.path().filename() < Right.path().filename();
    });

    for (const auto& Current : Entries)
    {
        if (Browser.ImportEntryCount >= 128u) break;
        ContentImportEntry& Written = Browser.ImportEntries[Browser.ImportEntryCount++];
        const std::string Name = Current.path().filename().string();
        const std::string Extension = Current.path().extension().string();
        Written.Directory = Current.is_directory(Error) && !Error;
        Written.Octets = Written.Directory ? 0u : Current.file_size(Error);
        if (Error) { Error.clear(); Written.Octets = 0u; }
        std::snprintf(Written.Naming, sizeof(Written.Naming), "%s", Name.c_str());
        std::snprintf(Written.Extension, sizeof(Written.Extension), "%s", Extension.c_str());
        Written.Supported = Written.Directory || Extension == ".obj" || Extension == ".codex" || Extension == ".pigment";
    }
}

double LengthSquared(const SpatialDirection& Direction)
{
    return Direction.Left * Direction.Left + Direction.Up * Direction.Up + Direction.Forward * Direction.Forward;
}

SpatialDirection Normalize(const SpatialDirection& Direction)
{
    const double Length = std::sqrt(LengthSquared(Direction));
    return Length > 0.0 ? SpatialDirection{ Direction.Left / Length,
                                            Direction.Up / Length,
                                            Direction.Forward / Length }
                        : SpatialDirection{ 1.0, 0.0, 0.0 };
}

SpatialDirection Cross(const SpatialDirection& LeftDirection,
                       const SpatialDirection& RightDirection)
{
    return {
        LeftDirection.Up * RightDirection.Forward - LeftDirection.Forward * RightDirection.Up,
        LeftDirection.Forward * RightDirection.Left - LeftDirection.Left * RightDirection.Forward,
        LeftDirection.Left * RightDirection.Up - LeftDirection.Up * RightDirection.Left
    };
}

SpatialDirection Scaled(const SpatialDirection& Direction, double Amount)
{
    return { Direction.Left * Amount, Direction.Up * Amount, Direction.Forward * Amount };
}

SpatialDirection Added(const SpatialDirection& LeftDirection,
                       const SpatialDirection& RightDirection)
{
    return { LeftDirection.Left + RightDirection.Left,
             LeftDirection.Up + RightDirection.Up,
             LeftDirection.Forward + RightDirection.Forward };
}

SpatialDirection Negated(const SpatialDirection& Direction)
{
    return { -Direction.Left, -Direction.Up, -Direction.Forward };
}

SpatialPoint Added(const SpatialPoint& Position, const SpatialDirection& Offset)
{
    return { Position.Left + Offset.Left, Position.Up + Offset.Up, Position.Forward + Offset.Forward };
}

SpatialDirection Difference(const SpatialPoint& LeftPoint, const SpatialPoint& RightPoint)
{
    return { RightPoint.Left - LeftPoint.Left,
             RightPoint.Up - LeftPoint.Up,
             RightPoint.Forward - LeftPoint.Forward };
}

double Dot(const SpatialDirection& LeftDirection,
           const SpatialDirection& RightDirection)
{
    return LeftDirection.Left * RightDirection.Left
         + LeftDirection.Up * RightDirection.Up
         + LeftDirection.Forward * RightDirection.Forward;
}

SpatialBasis ResolveSketchBasis(const SketchStructure& Sketch)
{
    const SketchPlane& Plane = Sketch.HeldPlane();
    const SpatialDirection Along = Normalize(Plane.AlongDirection);
    const SpatialDirection Normal = Normalize(Plane.Normal);
    const SpatialDirection Across = Normalize(Cross(Normal, Along));
    return { Plane.Origin, Along, Across, Normal };
}

SpatialPoint ResolvePlanarPoint(const SpatialBasis& Basis, double Along, double Across)
{
    return Added(Basis.Origin,
                 Added(Scaled(Basis.Along, Along),
                       Scaled(Basis.Across, Across)));
}

void ApplyViewportOrientation(ParametricViewportState& View,
                              ParametricViewOrientation Orientation,
                              bool Perspective)
{
    View.Orientation = Orientation;
    if (Perspective)
    {
        if (Orientation == ParametricViewOrientation::Isometric)
        {
            View.OrbitYaw = 45.0;
            View.OrbitPitch = 30.0;
        }
        else if (Orientation == ParametricViewOrientation::Top)
        {
            View.OrbitYaw = 0.0;
            View.OrbitPitch = 89.0;
        }
        else if (Orientation == ParametricViewOrientation::Bottom)
        {
            View.OrbitYaw = 0.0;
            View.OrbitPitch = -89.0;
        }
        else if (Orientation == ParametricViewOrientation::Front)
        {
            View.OrbitYaw = 0.0;
            View.OrbitPitch = 0.0;
        }
        else if (Orientation == ParametricViewOrientation::Back)
        {
            View.OrbitYaw = 180.0;
            View.OrbitPitch = 0.0;
        }
        else if (Orientation == ParametricViewOrientation::Left)
        {
            View.OrbitYaw = -90.0;
            View.OrbitPitch = 0.0;
        }
        else if (Orientation == ParametricViewOrientation::Right)
        {
            View.OrbitYaw = 90.0;
            View.OrbitPitch = 0.0;
        }
    }
}

const char* OrientationText(ParametricViewOrientation Orientation)
{
    switch (Orientation)
    {
        case ParametricViewOrientation::Top:       return "Top";
        case ParametricViewOrientation::Bottom:    return "Bottom";
        case ParametricViewOrientation::Front:     return "Front";
        case ParametricViewOrientation::Back:      return "Back";
        case ParametricViewOrientation::Left:      return "Left";
        case ParametricViewOrientation::Right:     return "Right";
        case ParametricViewOrientation::Isometric: return "Perspective";
    }
    return "Top";
}

struct ViewFrame
{
    SpatialPoint Eye = {};
    SpatialDirection Right = { 1.0, 0.0, 0.0 };
    SpatialDirection Up = { 0.0, 0.0, 1.0 };
    SpatialDirection Forward = { 0.0, -1.0, 0.0 };
};

ViewFrame ResolveViewportFrame(const SpatialBasis& Basis,
                               const ParametricViewportState& View,
                               bool Perspective)
{
    if (!Perspective)
    {
        switch (View.Orientation)
        {
            case ParametricViewOrientation::Top:
                return { Added(View.Focus, Scaled(Basis.Normal, 100.0)), Basis.Along, Basis.Across, Negated(Basis.Normal) };
            case ParametricViewOrientation::Bottom:
                return { Added(View.Focus, Scaled(Basis.Normal, -100.0)), Basis.Along, Negated(Basis.Across), Basis.Normal };
            case ParametricViewOrientation::Front:
                return { Added(View.Focus, Scaled(Basis.Across, -100.0)), Basis.Along, Basis.Normal, Basis.Across };
            case ParametricViewOrientation::Back:
                return { Added(View.Focus, Scaled(Basis.Across, 100.0)), Basis.Along, Negated(Basis.Normal), Negated(Basis.Across) };
            case ParametricViewOrientation::Left:
                return { Added(View.Focus, Scaled(Basis.Along, -100.0)), Basis.Across, Basis.Normal, Basis.Along };
            case ParametricViewOrientation::Right:
                return { Added(View.Focus, Scaled(Basis.Along, 100.0)), Negated(Basis.Across), Basis.Normal, Negated(Basis.Along) };
            case ParametricViewOrientation::Isometric:
                break;
        }
    }

    const double Yaw = View.OrbitYaw * Pi / 180.0;
    const double Pitch = View.OrbitPitch * Pi / 180.0;
    const SpatialDirection Forward = Normalize(Added(
        Added(Scaled(Basis.Along, std::sin(Yaw) * std::cos(Pitch)),
              Scaled(Basis.Normal, std::sin(Pitch))),
        Scaled(Negated(Basis.Across), std::cos(Yaw) * std::cos(Pitch))));
    const SpatialDirection Right = Normalize(Cross(Forward, Basis.Normal));
    const SpatialDirection Up = Normalize(Cross(Right, Forward));
    return { Added(View.Focus, Scaled(Forward, -View.Distance)), Right, Up, Forward };
}

bool ProjectViewportPoint(const SpatialBasis& Basis,
                          const ParametricViewportState& View,
                          bool Perspective,
                          const PlaneExtent& Extent,
                          double Along,
                          double Across,
                          float& ScreenX,
                          float& ScreenY)
{
    const ViewFrame Frame = ResolveViewportFrame(Basis, View, Perspective);
    const SpatialPoint Position = ResolvePlanarPoint(Basis, Along, Across);

    if (!Perspective)
    {
        const SpatialDirection Offset = Difference(View.Focus, Position);
        const double X = Dot(Offset, Frame.Right);
        const double Y = Dot(Offset, Frame.Up);
        ScreenX = static_cast<float>(Extent.MinimumX + Extent.Width() * 0.5 + X * View.OrthoScale);
        ScreenY = static_cast<float>(Extent.MinimumY + Extent.Height() * 0.5 - Y * View.OrthoScale);
        return true;
    }

    const SpatialDirection EyeToPoint = Difference(Frame.Eye, Position);
    const double CameraX = Dot(EyeToPoint, Frame.Right);
    const double CameraY = Dot(EyeToPoint, Frame.Up);
    const double CameraZ = Dot(EyeToPoint, Frame.Forward);
    if (CameraZ <= 0.01)
        return false;

    const double TanHalf = std::tan(CadPerspectiveFieldOfViewDegrees * 0.5 * Pi / 180.0);
    const double Focal = (Extent.Height() * 0.5) / TanHalf;
    ScreenX = static_cast<float>(Extent.MinimumX + Extent.Width() * 0.5 + CameraX / CameraZ * Focal);
    ScreenY = static_cast<float>(Extent.MinimumY + Extent.Height() * 0.5 - CameraY / CameraZ * Focal);
    return true;
}

WorkspaceCadProjection ResolveCadProjection(const SpatialBasis& Basis,
                                            const ParametricViewportState& View,
                                            bool Perspective,
                                            const PlaneExtent& Extent,
                                            std::uint32_t DisplayWidth,
                                            std::uint32_t DisplayHeight)
{
    const ViewFrame Frame = ResolveViewportFrame(Basis, View, Perspective);
    const float CentreX = Extent.MinimumX + Extent.Width() * 0.5f;
    const float CentreY = Extent.MinimumY + Extent.Height() * 0.5f;

    const auto ScreenProjection = [&](const SpatialPoint& Origin,
                                      const SpatialDirection& Along,
                                      const SpatialDirection& Across,
                                      float* Projection0,
                                      float* Projection1,
                                      float* Projection2)
    {
        if (!Perspective)
        {
            const SpatialDirection FocusToOrigin = Difference(View.Focus, Origin);
            const double BaseX = Dot(FocusToOrigin, Frame.Right);
            const double BaseY = Dot(FocusToOrigin, Frame.Up);
            const double AlongX = Dot(Along, Frame.Right);
            const double AlongY = Dot(Along, Frame.Up);
            const double AcrossX = Dot(Across, Frame.Right);
            const double AcrossY = Dot(Across, Frame.Up);

            Projection0[0] = CentreX + static_cast<float>(BaseX * View.OrthoScale);
            Projection0[1] = CentreY - static_cast<float>(BaseY * View.OrthoScale);
            Projection0[2] = 0.0f;
            Projection0[3] = 1.0f;

            Projection1[0] = static_cast<float>(AlongX * View.OrthoScale);
            Projection1[1] = static_cast<float>(-AlongY * View.OrthoScale);
            Projection1[2] = 0.0f;
            Projection1[3] = 0.0f;

            Projection2[0] = static_cast<float>(AcrossX * View.OrthoScale);
            Projection2[1] = static_cast<float>(-AcrossY * View.OrthoScale);
            Projection2[2] = 0.0f;
            Projection2[3] = 0.0f;
            return;
        }

        const double TanHalf = std::tan(CadPerspectiveFieldOfViewDegrees * 0.5 * Pi / 180.0);
        const double Focal = (Extent.Height() * 0.5) / TanHalf;
        const SpatialDirection EyeToOrigin = Difference(Frame.Eye, Origin);
        const double BaseX = Dot(EyeToOrigin, Frame.Right);
        const double BaseY = Dot(EyeToOrigin, Frame.Up);
        const double BaseZ = Dot(EyeToOrigin, Frame.Forward);
        const double AlongX = Dot(Along, Frame.Right);
        const double AlongY = Dot(Along, Frame.Up);
        const double AlongZ = Dot(Along, Frame.Forward);
        const double AcrossX = Dot(Across, Frame.Right);
        const double AcrossY = Dot(Across, Frame.Up);
        const double AcrossZ = Dot(Across, Frame.Forward);

        Projection0[0] = static_cast<float>(CentreX * BaseZ + Focal * BaseX);
        Projection0[1] = static_cast<float>(CentreY * BaseZ - Focal * BaseY);
        Projection0[2] = 0.0f;
        Projection0[3] = static_cast<float>(BaseZ);

        Projection1[0] = static_cast<float>(CentreX * AlongZ + Focal * AlongX);
        Projection1[1] = static_cast<float>(CentreY * AlongZ - Focal * AlongY);
        Projection1[2] = 0.0f;
        Projection1[3] = static_cast<float>(AlongZ);

        Projection2[0] = static_cast<float>(CentreX * AcrossZ + Focal * AcrossX);
        Projection2[1] = static_cast<float>(CentreY * AcrossZ - Focal * AcrossY);
        Projection2[2] = 0.0f;
        Projection2[3] = static_cast<float>(AcrossZ);
    };

    WorkspaceCadProjection Projection = {};
    Projection.DisplayWidth = static_cast<float>(DisplayWidth);
    Projection.DisplayHeight = static_cast<float>(DisplayHeight);
    ScreenProjection(Basis.Origin, Basis.Along, Basis.Across,
                     Projection.Projection0, Projection.Projection1, Projection.Projection2);
    return Projection;
}

bool WithinViewportOrientationButton(const PlaneExtent& Button, const PointerCondition& Pointer)
{
    return Button.Encloses(Pointer.PositionX, Pointer.PositionY);
}

void RecordViewportOrientationHud(RecordingSurface& Surface,
                                  const PlaneExtent& Extent,
                                  const PointerCondition& Pointer,
                                  ParametricViewportState& View,
                                  bool& Perspective,
                                  bool& PointerTaken)
{
    const float Pad = 10.0f;
    const float ButtonX = 34.0f;
    const float ButtonY = 24.0f;
    const float Top = Extent.MinimumY + Pad;
    const float Right = Extent.MaximumX - Pad;

    struct FaceButton
    {
        ParametricViewOrientation Orientation;
        const char* Text;
        PlaneExtent Extent;
        bool PerspectiveReading;
    };

    FaceButton Faces[] =
    {
        { ParametricViewOrientation::Top, "T", Spanning(Right - 72.0f, Top, ButtonX, ButtonY), false },
        { ParametricViewOrientation::Front, "F", Spanning(Right - 108.0f, Top + 26.0f, ButtonX, ButtonY), false },
        { ParametricViewOrientation::Back, "B", Spanning(Right - 72.0f, Top + 26.0f, ButtonX, ButtonY), false },
        { ParametricViewOrientation::Left, "L", Spanning(Right - 144.0f, Top + 26.0f, ButtonX, ButtonY), false },
        { ParametricViewOrientation::Right, "R", Spanning(Right - 36.0f, Top + 26.0f, ButtonX, ButtonY), false },
        { ParametricViewOrientation::Bottom, "D", Spanning(Right - 72.0f, Top + 52.0f, ButtonX, ButtonY), false },
        { ParametricViewOrientation::Isometric, "P", Spanning(Right - 72.0f, Top + 78.0f, ButtonX, ButtonY), true },
    };

    for (FaceButton& Face : Faces)
    {
        const bool Hovered = WithinViewportOrientationButton(Face.Extent, Pointer);
        const bool Current = Face.PerspectiveReading ? Perspective : (!Perspective && View.Orientation == Face.Orientation);
        if (Hovered && Pointer.ContactPressed)
        {
            ApplyViewportOrientation(View, Face.Orientation, Face.PerspectiveReading);
            Perspective = Face.PerspectiveReading;
            PointerTaken = true;
        }
        if (Hovered)
            PointerTaken = true;

        Surface.Ground(Face.Extent, Current ? Covering(0x5B8CFFu) : (Hovered ? Covering(0x26262Bu) : Covering(0x17171Au)), 7.0f, CornerAll);
        Surface.Edge(Face.Extent, Covering(0x1C1C20u), 1.0f, 7.0f, CornerAll);
        const float Width = Surface.MeasureRun(Face.Text, 11.0f, 0.0f);
        Surface.TextRun(Face.Extent.MinimumX + (Face.Extent.Width() - Width) * 0.5f,
                        Face.Extent.MinimumY + 7.0f,
                        Current ? Covering(0xFFFFFFu) : Covering(0xD8D8DCu),
                        Face.Text, 11.0f, 0.0f, true);
    }

    const PlaneExtent PerspectiveButton = Spanning(Extent.MinimumX + Pad, Extent.MinimumY + Pad, 86.0f, 24.0f);
    const PlaneExtent OrthographicButton = Spanning(PerspectiveButton.MaximumX + 6.0f, PerspectiveButton.MinimumY, 98.0f, 24.0f);
    const bool PerspectiveHovered = WithinViewportOrientationButton(PerspectiveButton, Pointer);
    const bool OrthographicHovered = WithinViewportOrientationButton(OrthographicButton, Pointer);
    if (PerspectiveHovered && Pointer.ContactPressed)
    {
        Perspective = true;
        ApplyViewportOrientation(View, ParametricViewOrientation::Isometric, true);
        PointerTaken = true;
    }
    if (OrthographicHovered && Pointer.ContactPressed)
    {
        Perspective = false;
        PointerTaken = true;
    }
    if (PerspectiveHovered || OrthographicHovered)
        PointerTaken = true;

    Surface.Ground(PerspectiveButton, Perspective ? Covering(0x5B8CFFu) : Covering(0x17171Au), 7.0f, CornerAll);
    Surface.Edge(PerspectiveButton, Covering(0x1C1C20u), 1.0f, 7.0f, CornerAll);
    Surface.TextRun(PerspectiveButton.MinimumX + 10.0f, PerspectiveButton.MinimumY + 7.0f,
                    Perspective ? Covering(0xFFFFFFu) : Covering(0xD8D8DCu),
                    "Perspective", 11.0f, 0.0f, true);

    Surface.Ground(OrthographicButton, !Perspective ? Covering(0x5B8CFFu) : Covering(0x17171Au), 7.0f, CornerAll);
    Surface.Edge(OrthographicButton, Covering(0x1C1C20u), 1.0f, 7.0f, CornerAll);
    Surface.TextRun(OrthographicButton.MinimumX + 10.0f, OrthographicButton.MinimumY + 7.0f,
                    !Perspective ? Covering(0xFFFFFFu) : Covering(0xD8D8DCu),
                    "Orthographic", 11.0f, 0.0f, true);
}

void DriveViewport(const PlaneExtent& Extent,
                   const PointerCondition& Pointer,
                   const ModifierCondition& Modifiers,
                   ParametricViewportState& View,
                   bool Perspective)
{
    if (!Extent.Encloses(Pointer.PositionX, Pointer.PositionY))
        return;

    if (Pointer.WheelY != 0.0f)
    {
        if (Perspective)
            View.Distance = std::clamp(View.Distance * (Pointer.WheelY > 0.0f ? 0.9 : 1.1), 20.0, 4000.0);
        else
            View.OrthoScale = std::clamp(View.OrthoScale * (Pointer.WheelY > 0.0f ? 1.1 : 0.9), 0.05, 40.0);
    }

    if (!Pointer.SecondaryHeld)
        return;

    const SpatialBasis Basis = { {}, {1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 1.0, 0.0} };
    const ViewFrame Frame = ResolveViewportFrame(Basis, View, Perspective);

    if (Perspective && !Modifiers.Shifted)
    {
        View.OrbitYaw -= static_cast<double>(Pointer.TravelX) * 0.35;
        View.OrbitPitch = std::clamp(View.OrbitPitch + static_cast<double>(Pointer.TravelY) * 0.25, -89.0, 89.0);
        View.Orientation = ParametricViewOrientation::Isometric;
        return;
    }

    const double Scale = Perspective ? (View.Distance * 0.0025) : (1.0 / std::max(View.OrthoScale, 0.001));
    const SpatialDirection Pan = Added(Scaled(Frame.Right, -static_cast<double>(Pointer.TravelX) * Scale),
                                       Scaled(Frame.Up, static_cast<double>(Pointer.TravelY) * Scale));
    View.Focus = Added(View.Focus, Pan);
}

void RecordViewportGrid(RecordingSurface& Surface,
                        const PlaneExtent& Extent,
                        const SketchStructure& Sketch,
                        const ParametricViewportState& View,
                        bool Perspective,
                        const EditorPanelConfiguration& Configuration)
{
    if (!Sketch.Declared() || Configuration.Lattice == PanelLatticePresentation::None)
        return;

    const SpatialBasis Basis = ResolveSketchBasis(Sketch);
    const double Step = std::max(Configuration.LatticeCellMetres * static_cast<double>(Configuration.LatticeScale), 1.0);
    const std::uint32_t Subdivisions = std::max(Configuration.Subdivisions, 2u);
    const std::int32_t Count = Perspective ? 40 : 80;

    for (std::int32_t Index = -Count; Index <= Count; ++Index)
    {
        const bool Major = (std::abs(Index) % static_cast<std::int32_t>(Subdivisions)) == 0;
        const ThemeToken Colour = Major ? Faded(Covering(0xC4C8D6u), 0.22f) : Faded(Covering(0xC4C8D6u), 0.10f);
        const float Weight = Major ? Configuration.LatticeLineWeight + 0.25f : Configuration.LatticeLineWeight;

        float X0 = 0.0f, Y0 = 0.0f, X1 = 0.0f, Y1 = 0.0f;
        if (ProjectViewportPoint(Basis, View, Perspective, Extent, static_cast<double>(Index) * Step, static_cast<double>(-Count) * Step, X0, Y0) &&
            ProjectViewportPoint(Basis, View, Perspective, Extent, static_cast<double>(Index) * Step, static_cast<double>( Count) * Step, X1, Y1))
        {
            const float PointsX[2] = { X0, X1 };
            const float PointsY[2] = { Y0, Y1 };
            Surface.Polyline(PointsX, PointsY, 2u, Colour, Weight);
        }

        if (ProjectViewportPoint(Basis, View, Perspective, Extent, static_cast<double>(-Count) * Step, static_cast<double>(Index) * Step, X0, Y0) &&
            ProjectViewportPoint(Basis, View, Perspective, Extent, static_cast<double>( Count) * Step, static_cast<double>(Index) * Step, X1, Y1))
        {
            const float PointsX[2] = { X0, X1 };
            const float PointsY[2] = { Y0, Y1 };
            Surface.Polyline(PointsX, PointsY, 2u, Colour, Weight);
        }
    }

    float X0 = 0.0f, Y0 = 0.0f, X1 = 0.0f, Y1 = 0.0f;
    if (Configuration.AxisX &&
        ProjectViewportPoint(Basis, View, Perspective, Extent, -Count * Step, 0.0, X0, Y0) &&
        ProjectViewportPoint(Basis, View, Perspective, Extent, Count * Step, 0.0, X1, Y1))
    {
        const float PointsX[2] = { X0, X1 };
        const float PointsY[2] = { Y0, Y1 };
        Surface.Polyline(PointsX, PointsY, 2u, Faded(Covering(0xFC5A5Au), 0.80f), 1.6f);
    }

    if (Configuration.AxisZ &&
        ProjectViewportPoint(Basis, View, Perspective, Extent, 0.0, -Count * Step, X0, Y0) &&
        ProjectViewportPoint(Basis, View, Perspective, Extent, 0.0, Count * Step, X1, Y1))
    {
        const float PointsX[2] = { X0, X1 };
        const float PointsY[2] = { Y0, Y1 };
        Surface.Polyline(PointsX, PointsY, 2u, Faded(Covering(0x5A8BFCu), 0.80f), 1.6f);
    }
}

void RecordCadFallback(RecordingSurface& Surface,
                       const PlaneExtent& Extent,
                       const SketchStructure& Sketch,
                       const ParametricViewportState& View,
                       bool Perspective,
                       const WorkspaceCadPacket& Packet)
{
    if (!Packet.ExtentStanding || !Sketch.Declared())
        return;

    const SpatialBasis Basis = ResolveSketchBasis(Sketch);
    Surface.Confine(Extent);

    for (std::uint32_t Index = 0u; Index < Packet.FillCount; ++Index)
    {
        const WorkspaceCadFillTriangle& Fill = Packet.Fills[Index];
        float X0 = 0.0f, Y0 = 0.0f, X1 = 0.0f, Y1 = 0.0f, X2 = 0.0f, Y2 = 0.0f;
        if (!ProjectViewportPoint(Basis, View, Perspective, Extent, Fill.Along0, Fill.Across0, X0, Y0) ||
            !ProjectViewportPoint(Basis, View, Perspective, Extent, Fill.Along1, Fill.Across1, X1, Y1) ||
            !ProjectViewportPoint(Basis, View, Perspective, Extent, Fill.Along2, Fill.Across2, X2, Y2))
            continue;
        const float Corners[6] = { X0, Y0, X1, Y1, X2, Y2 };
        Surface.Tongue(Corners, 3u, ThemeToken{
            static_cast<std::uint8_t>((Fill.Packed >> 16u) & 0xFFu),
            static_cast<std::uint8_t>((Fill.Packed >> 8u) & 0xFFu),
            static_cast<std::uint8_t>((Fill.Packed >> 0u) & 0xFFu),
            static_cast<std::uint8_t>((Fill.Packed >> 24u) & 0xFFu) });
    }

    for (std::uint32_t Index = 0u; Index < Packet.SegmentCount; ++Index)
    {
        const WorkspaceCadSegment& Segment = Packet.Segments[Index];
        float X0 = 0.0f, Y0 = 0.0f, X1 = 0.0f, Y1 = 0.0f;
        if (!ProjectViewportPoint(Basis, View, Perspective, Extent, Segment.Along0, Segment.Across0, X0, Y0) ||
            !ProjectViewportPoint(Basis, View, Perspective, Extent, Segment.Along1, Segment.Across1, X1, Y1))
            continue;
        const float PointsX[2] = { X0, X1 };
        const float PointsY[2] = { Y0, Y1 };
        Surface.Polyline(PointsX, PointsY, 2u,
            ThemeToken{ static_cast<std::uint8_t>((Segment.Packed >> 16u) & 0xFFu),
                        static_cast<std::uint8_t>((Segment.Packed >> 8u) & 0xFFu),
                        static_cast<std::uint8_t>((Segment.Packed >> 0u) & 0xFFu),
                        static_cast<std::uint8_t>((Segment.Packed >> 24u) & 0xFFu) },
            Segment.Thickness);
    }

    for (std::uint32_t Index = 0u; Index < Packet.MarkerCount; ++Index)
    {
        const WorkspaceCadMarker& Marker = Packet.Markers[Index];
        float X = 0.0f, Y = 0.0f;
        if (!ProjectViewportPoint(Basis, View, Perspective, Extent, Marker.Along, Marker.Across, X, Y))
            continue;
        Surface.Medallion(X, Y, Marker.Radius,
            ThemeToken{ static_cast<std::uint8_t>((Marker.Packed >> 16u) & 0xFFu),
                        static_cast<std::uint8_t>((Marker.Packed >> 8u) & 0xFFu),
                        static_cast<std::uint8_t>((Marker.Packed >> 0u) & 0xFFu),
                        static_cast<std::uint8_t>((Marker.Packed >> 24u) & 0xFFu) });
    }

    Surface.Release();
}

void RecordViewportStateReadout(RecordingSurface& Surface,
                                const PlaneExtent& Extent,
                                const ParametricViewportState& View,
                                bool Perspective,
                                const WorkspaceCadPacket& Packet)
{
    char Detail[192] = {};
    std::snprintf(Detail, sizeof(Detail),
                  "%s • %s • %u segments • %u fills • %u markers",
                  Perspective ? "Perspective" : "Orthographic",
                  OrientationText(View.Orientation),
                  static_cast<unsigned>(Packet.SegmentCount),
                  static_cast<unsigned>(Packet.FillCount),
                  static_cast<unsigned>(Packet.MarkerCount));
    Surface.TextRun(Extent.MinimumX + 16.0f,
                    Extent.MaximumY - 24.0f,
                    Faded(Covering(0xE5E7EBu), 0.75f),
                    Detail, 11.0f);
}

ParametricDraftSubject ResolveDraftSubject(ParametricToolSubject Subject)
{
    switch (Subject)
    {
        case ParametricToolSubject::Line:      return ParametricDraftSubject::Line;
        case ParametricToolSubject::Rectangle: return ParametricDraftSubject::Rectangle;
        case ParametricToolSubject::Circle:    return ParametricDraftSubject::Circle;
        default:                               return ParametricDraftSubject::None;
    }
}

bool ResolveViewportPlaneIntersection(const SpatialBasis& Basis,
                                      const ParametricViewportState& View,
                                      bool Perspective,
                                      const PlaneExtent& Extent,
                                      float ScreenX,
                                      float ScreenY,
                                      SpatialPoint& Position)
{
    const ViewFrame Frame = ResolveViewportFrame(Basis, View, Perspective);
    const double NdcX = (static_cast<double>(ScreenX) - (Extent.MinimumX + Extent.Width() * 0.5)) / (Extent.Width() * 0.5);
    const double NdcY = ((Extent.MinimumY + Extent.Height() * 0.5) - static_cast<double>(ScreenY)) / (Extent.Height() * 0.5);

    SpatialPoint RayOrigin = {};
    SpatialDirection RayDirection = {};

    if (!Perspective)
    {
        const double Along = NdcX * (Extent.Width() * 0.5) / View.OrthoScale;
        const double Across = NdcY * (Extent.Height() * 0.5) / View.OrthoScale;
        RayOrigin = Added(View.Focus, Added(Scaled(Frame.Right, Along), Scaled(Frame.Up, Across)));
        RayDirection = Frame.Forward;
    }
    else
    {
        const double TanHalf = std::tan(CadPerspectiveFieldOfViewDegrees * 0.5 * Pi / 180.0);
        const double Aspect = Extent.Width() / Extent.Height();
        RayOrigin = Frame.Eye;
        RayDirection = Normalize(Added(Added(Scaled(Frame.Right, NdcX * TanHalf * Aspect),
                                              Scaled(Frame.Up, NdcY * TanHalf)),
                                        Frame.Forward));
    }

    const SpatialDirection EyeToPlane = Difference(RayOrigin, Basis.Origin);
    const double Denominator = Dot(RayDirection, Basis.Normal);
    if (std::fabs(Denominator) <= 1.0e-6)
        return false;

    const double Distance = -Dot(EyeToPlane, Basis.Normal) / Denominator;
    if (Distance < 0.0)
        return false;

    Position = Added(RayOrigin, Scaled(RayDirection, Distance));
    return true;
}

double ResolveSnapTolerance(const ParametricViewportState& View,
                            bool Perspective)
{
    return Perspective ? std::max(View.Distance * 0.02, 2.0)
                       : std::max(10.0 / std::max(View.OrthoScale, 0.001), 0.25);
}

WorkspaceRecordName ResolveCategoryFolder(const WorkspaceRecordStructure& Records,
                                          WorkspaceCategory Category)
{
    for (std::uint32_t Index = 1u; Index <= Records.DeclaredCount(); ++Index)
    {
        const WorkspaceRecord* Record = Records.Resolve({ Index });
        if (Record != nullptr && Record->Subject == WorkspaceRecordSubject::Folder &&
            Record->FolderCategory == Category && !Record->ParentFolder.Assigned())
            return { Index };
    }
    return {};
}

WorkspaceRecordName DeclareWorkspaceCurve(WorkspaceNameIndex& Naming,
                                          WorkspaceRecordStructure& Records,
                                          SketchCurveName Curve)
{
    WorkspaceRecord Record = {};
    Record.Subject = WorkspaceRecordSubject::OpenCurve;
    Record.ParentFolder = ResolveCategoryFolder(Records, WorkspaceCategory::Sketch);
    Record.Naming = Naming.Issue(WorkspaceRecordSubject::OpenCurve);
    Record.SketchCurve = Curve;
    return Records.Declare(Record);
}

WorkspaceRecordName DeclareWorkspaceProfile(WorkspaceNameIndex& Naming,
                                            WorkspaceRecordStructure& Records,
                                            ProfileNameInFeature Profile)
{
    WorkspaceRecord Record = {};
    Record.Subject = WorkspaceRecordSubject::ClosedProfile;
    Record.ParentFolder = ResolveCategoryFolder(Records, WorkspaceCategory::Sketch);
    Record.Naming = Naming.Issue(WorkspaceRecordSubject::ClosedProfile);
    Record.Profile = Profile;
    Record.ClosedSemantic = true;
    return Records.Declare(Record);
}

Outcome<WorkspaceRecordName> CommitDraft(WorkspaceNameIndex& Naming,
                                         SketchStructure& Sketch,
                                         WorkspaceRecordStructure& Records,
                                         WorkspaceRevisionSequence& Revisions,
                                         const ParametricDraftState& Draft)
{
    if (Draft.Subject == ParametricDraftSubject::Line && Draft.Anchors.size() >= 2u)
    {
        const SketchCurveName Curve = Sketch.DeclareLine(Draft.Anchors[0], Draft.Anchors[1]);
        const WorkspaceRecordName Record = DeclareWorkspaceCurve(Naming, Records, Curve);
        Revisions.Seal("Declared " + std::string(Records.Resolve(Record)->Naming), "Create Curve", { Record },
                       Revisions.DeclaredCount() + 1u);
        return Outcome<WorkspaceRecordName>::Result(Record);
    }

    if (Draft.Subject == ParametricDraftSubject::Circle && Draft.Anchors.size() >= 1u && Draft.HoverStanding)
    {
        const SpatialDirection Radius = Difference(Draft.Anchors[0], Draft.Hover);
        const double RadiusLength = std::sqrt(LengthSquared(Radius));
        if (RadiusLength <= 1.0e-6)
            return Outcome<WorkspaceRecordName>::Refuse({ RefusalReason::ContentUnsupported,
                                                          "the circle radius is too small" });

        const Outcome<ProfileNameInFeature> Profile = Sketch.DeclareCircleProfile(
            { Draft.Anchors[0], Sketch.HeldPlane().Normal, Normalize(Radius), RadiusLength });
        if (!Profile.Resolved)
            return Outcome<WorkspaceRecordName>::Refuse(Profile.Error);
        const WorkspaceRecordName Record = DeclareWorkspaceProfile(Naming, Records, Profile.Resolve());
        Revisions.Seal("Declared " + std::string(Records.Resolve(Record)->Naming), "Create Profile", { Record },
                       Revisions.DeclaredCount() + 1u);
        return Outcome<WorkspaceRecordName>::Result(Record);
    }

    if (Draft.Subject == ParametricDraftSubject::Rectangle && Draft.Anchors.size() >= 1u && Draft.HoverStanding)
    {
        const SpatialPoint A = Draft.Anchors[0];
        const SpatialPoint C = Draft.Hover;
        const SpatialPoint B = { C.Left, A.Up, A.Forward };
        const SpatialPoint D = { A.Left, A.Up, C.Forward };

        ProfileSpecification Profile;
        Profile.DeclarePlane({ Sketch.HeldPlane().Origin, Sketch.HeldPlane().Normal, Sketch.HeldPlane().AlongDirection });
        ProfileLoop Loop;
        Loop.Orientation = ProfileLoopOrientation::Outer;
        const SketchCurveName AB = Sketch.DeclareLine(A, B);
        const SketchCurveName BC = Sketch.DeclareLine(B, C);
        const SketchCurveName CD = Sketch.DeclareLine(C, D);
        const SketchCurveName DA = Sketch.DeclareLine(D, A);
        Loop.Traversal = { { { AB.IssuedIndex }, true }, { { BC.IssuedIndex }, true },
                           { { CD.IssuedIndex }, true }, { { DA.IssuedIndex }, true } };
        Profile.DeclareLoop(Loop);
        const ProfileNameInFeature ProfileName = Sketch.DeclareProfile(Profile);
        const WorkspaceRecordName Record = DeclareWorkspaceProfile(Naming, Records, ProfileName);
        Revisions.Seal("Declared " + std::string(Records.Resolve(Record)->Naming), "Create Profile", { Record },
                       Revisions.DeclaredCount() + 1u);
        return Outcome<WorkspaceRecordName>::Result(Record);
    }

    return Outcome<WorkspaceRecordName>::Refuse({ RefusalReason::ContentUnsupported,
                                                  "the active draft is not ready to commit" });
}

void CancelDraft(ParametricDraftState& Draft)
{
    Draft = {};
}

void RecordDraftPreview(RecordingSurface& Surface,
                        const PlaneExtent& Extent,
                        const SketchStructure& Sketch,
                        const ParametricViewportState& View,
                        bool Perspective,
                        const ParametricDraftState& Draft)
{
    if (Draft.Subject == ParametricDraftSubject::None || !Draft.HoverStanding || !Sketch.Declared())
        return;

    const SpatialBasis Basis = ResolveSketchBasis(Sketch);
    const auto Projected = [&](const SpatialPoint& Position, float& X, float& Y) -> bool
    {
        const SpatialDirection Offset = Difference(Basis.Origin, Position);
        const double Along = Dot(Offset, Basis.Along);
        const double Across = Dot(Offset, Basis.Across);
        return ProjectViewportPoint(Basis, View, Perspective, Extent, Along, Across, X, Y);
    };

    const ThemeToken Preview = Covering(0x5B8CFFu);
    const ThemeToken SnapTone = Covering(0xFBBF24u);

    if (Draft.Subject == ParametricDraftSubject::Line && Draft.Anchors.size() == 1u)
    {
        float X0 = 0.0f, Y0 = 0.0f, X1 = 0.0f, Y1 = 0.0f;
        if (Projected(Draft.Anchors[0], X0, Y0) && Projected(Draft.Hover, X1, Y1))
        {
            const float PointsX[2] = { X0, X1 };
            const float PointsY[2] = { Y0, Y1 };
            Surface.Polyline(PointsX, PointsY, 2u, Preview, 1.8f);
        }
    }
    else if (Draft.Subject == ParametricDraftSubject::Rectangle && Draft.Anchors.size() == 1u)
    {
        const SpatialPoint A = Draft.Anchors[0];
        const SpatialPoint C = Draft.Hover;
        const SpatialPoint B = { C.Left, A.Up, A.Forward };
        const SpatialPoint D = { A.Left, A.Up, C.Forward };
        float X[4] = {}, Y[4] = {};
        if (Projected(A, X[0], Y[0]) && Projected(B, X[1], Y[1]) &&
            Projected(C, X[2], Y[2]) && Projected(D, X[3], Y[3]))
        {
            for (std::uint32_t Index = 0u; Index < 4u; ++Index)
            {
                const std::uint32_t Next = (Index + 1u) % 4u;
                const float PointsX[2] = { X[Index], X[Next] };
                const float PointsY[2] = { Y[Index], Y[Next] };
                Surface.Polyline(PointsX, PointsY, 2u, Preview, 1.8f);
            }
        }
    }
    else if (Draft.Subject == ParametricDraftSubject::Circle && Draft.Anchors.size() == 1u)
    {
        const SpatialDirection Radius = Difference(Draft.Anchors[0], Draft.Hover);
        const double RadiusLength = std::sqrt(LengthSquared(Radius));
        if (RadiusLength > 1.0e-6)
        {
            float PointsX[49] = {};
            float PointsY[49] = {};
            std::uint32_t Count = 0u;
            for (std::uint32_t Step = 0u; Step <= 48u; ++Step)
            {
                const double Angle = (static_cast<double>(Step) / 48.0) * (2.0 * Pi);
                const SpatialPoint Position = { Draft.Anchors[0].Left + std::cos(Angle) * RadiusLength,
                                                Draft.Anchors[0].Up,
                                                Draft.Anchors[0].Forward + std::sin(Angle) * RadiusLength };
                float X = 0.0f, Y = 0.0f;
                if (!Projected(Position, X, Y))
                    continue;
                PointsX[Count] = X;
                PointsY[Count] = Y;
                ++Count;
            }
            if (Count >= 2u)
                Surface.Polyline(PointsX, PointsY, Count, Preview, 1.8f);
        }
    }

    float MarkerX = 0.0f, MarkerY = 0.0f;
    if (Projected(Draft.Hover, MarkerX, MarkerY))
        Surface.Medallion(MarkerX, MarkerY, 4.0f, Draft.Snap.Resolved() ? SnapTone : Preview);
}

void DriveDrawing(const PlaneExtent& Extent,
                  const PointerCondition& Pointer,
                  const SpatialBasis& Basis,
                  const ParametricViewportState& View,
                  bool Perspective,
                  ParametricToolSubject Tool,
                  WorkspaceNameIndex& Naming,
                  SketchStructure& Sketch,
                  WorkspaceRecordStructure& Records,
                  WorkspaceRevisionSequence& Revisions,
                  WorkspaceRecordName& PendingSelection,
                  ParametricDraftState& Draft,
                  bool& PointerTaken)
{
    const ParametricDraftSubject Desired = ResolveDraftSubject(Tool);
    if (Desired == ParametricDraftSubject::None)
    {
        if (Draft.Subject != ParametricDraftSubject::None)
            CancelDraft(Draft);
        return;
    }

    if (!Extent.Encloses(Pointer.PositionX, Pointer.PositionY))
        return;

    if (Draft.Subject != Desired)
        CancelDraft(Draft);
    Draft.Subject = Desired;

    SpatialPoint Raw = {};
    if (!ResolveViewportPlaneIntersection(Basis, View, Perspective, Extent,
                                          Pointer.PositionX, Pointer.PositionY, Raw))
        return;

    Draft.HoverStanding = true;
    Draft.Hover = Raw;
    Draft.Snap = ResolveNearestSnap(Sketch, Raw, ResolveSnapTolerance(View, Perspective));
    if (Draft.Snap.Resolved())
        Draft.Hover = Draft.Snap.Position;

    if (Pointer.ContactPressed)
    {
        PointerTaken = true;

        if (Draft.Subject == ParametricDraftSubject::Line)
        {
            Draft.Anchors.push_back(Draft.Hover);
            if (Draft.Anchors.size() >= 2u)
            {
                const Outcome<WorkspaceRecordName> Record = CommitDraft(Naming, Sketch, Records, Revisions, Draft);
                if (Record.Resolved)
                    PendingSelection = Record.Resolve();
                CancelDraft(Draft);
            }
        }
        else if (Draft.Subject == ParametricDraftSubject::Rectangle || Draft.Subject == ParametricDraftSubject::Circle)
        {
            if (Draft.Anchors.empty())
                Draft.Anchors.push_back(Draft.Hover);
            else
            {
                const Outcome<WorkspaceRecordName> Record = CommitDraft(Naming, Sketch, Records, Revisions, Draft);
                if (Record.Resolved)
                    PendingSelection = Record.Resolve();
                CancelDraft(Draft);
            }
        }
    }
}

namespace
{

std::uint32_t OverlayPacked(std::uint32_t Red,
                            std::uint32_t Green,
                            std::uint32_t Blue,
                            std::uint32_t Alpha = 255u)
{
    return PackOverlayColour(Red, Green, Blue, Alpha);
}

ThemeToken TokenFromPacked(std::uint32_t Packed)
{
    return ThemeToken{
        static_cast<std::uint8_t>((Packed >> 16u) & 0xFFu),
        static_cast<std::uint8_t>((Packed >> 8u) & 0xFFu),
        static_cast<std::uint8_t>((Packed >> 0u) & 0xFFu),
        static_cast<std::uint8_t>((Packed >> 24u) & 0xFFu)
    };
}

const char* TransformModeText(ParametricTransformMode Mode)
{
    switch (Mode)
    {
        case ParametricTransformMode::Move:   return "Move";
        case ParametricTransformMode::Rotate: return "Rotate";
        case ParametricTransformMode::Scale:  return "Scale";
    }
    return "Move";
}

const char* TransformConstraintText(ParametricTransformConstraint Constraint)
{
    switch (Constraint)
    {
        case ParametricTransformConstraint::Free:   return "Free";
        case ParametricTransformConstraint::AxisX:  return "X";
        case ParametricTransformConstraint::AxisZ:  return "Z";
        case ParametricTransformConstraint::Screen: return "Screen";
        case ParametricTransformConstraint::Curve:  return "Curve";
    }
    return "Free";
}

const char* TransformCommandToken(ParametricTransformMode Mode)
{
    switch (Mode)
    {
        case ParametricTransformMode::Move:   return "G";
        case ParametricTransformMode::Rotate: return "R";
        case ParametricTransformMode::Scale:  return "S";
    }
    return "G";
}

bool NumericCharacter(char Character)
{
    return (Character >= '0' && Character <= '9') || Character == '.' || Character == '-';
}

void AppendTransformNumericRun(char* Target,
                               std::size_t Capacity,
                               const char* Source)
{
    std::size_t Occupied = 0u;
    while (Occupied + 1u < Capacity && Target[Occupied] != '\0')
        ++Occupied;

    for (std::size_t Index = 0u; Source[Index] != '\0' && Occupied + 1u < Capacity; ++Index)
        if (NumericCharacter(Source[Index]))
            Target[Occupied++] = Source[Index];
    Target[Occupied] = '\0';
}

ParametricTransformCommandInput ResolveTransformCommandInput(const TextInputCondition& Input,
                                                             bool Engaged,
                                                             ParametricTransformMode CurrentMode)
{
    ParametricTransformCommandInput Resolved = {};
    ParametricTransformMode WorkingMode = Engaged ? CurrentMode : ParametricTransformMode::Move;
    bool ModeStanding = Engaged;
    std::size_t NumericTaken = 0u;

    for (std::uint32_t Index = 0u; Index < Input.IntakeCount; ++Index)
    {
        const char Character = Input.Intake[Index];
        if (Character == 'g' || Character == 'G')
        {
            ++Resolved.MoveTapCount;
            if (!Engaged && !Resolved.StartRequested)
            {
                Resolved.StartRequested = true;
                Resolved.StartMode = ParametricTransformMode::Move;
                WorkingMode = ParametricTransformMode::Move;
                ModeStanding = true;
            }
            continue;
        }

        if ((Character == 'r' || Character == 'R') && !Engaged && !Resolved.StartRequested)
        {
            Resolved.StartRequested = true;
            Resolved.StartMode = ParametricTransformMode::Rotate;
            WorkingMode = ParametricTransformMode::Rotate;
            ModeStanding = true;
            continue;
        }

        if ((Character == 's' || Character == 'S') && !Engaged && !Resolved.StartRequested)
        {
            Resolved.StartRequested = true;
            Resolved.StartMode = ParametricTransformMode::Scale;
            WorkingMode = ParametricTransformMode::Scale;
            ModeStanding = true;
            continue;
        }

        if ((Character == 'x' || Character == 'X') && ModeStanding &&
            (WorkingMode == ParametricTransformMode::Move || WorkingMode == ParametricTransformMode::Scale))
        {
            Resolved.ConstraintRequested = true;
            Resolved.Constraint = ParametricTransformConstraint::AxisX;
            continue;
        }

        if ((Character == 'z' || Character == 'Z') && ModeStanding &&
            (WorkingMode == ParametricTransformMode::Move || WorkingMode == ParametricTransformMode::Scale))
        {
            Resolved.ConstraintRequested = true;
            Resolved.Constraint = ParametricTransformConstraint::AxisZ;
            continue;
        }

        if (NumericCharacter(Character) && NumericTaken + 1u < sizeof(Resolved.NumericAppend))
            Resolved.NumericAppend[NumericTaken++] = Character;
    }

    Resolved.NumericAppend[NumericTaken] = '\0';
    return Resolved;
}

void RetractTransformCommand(ParametricTransformState& Transform)
{
    std::size_t Occupied = 0u;
    while (Occupied + 1u < sizeof(Transform.Numeric) && Transform.Numeric[Occupied] != '\0')
        ++Occupied;

    if (Occupied > 0u)
    {
        Transform.Numeric[Occupied - 1u] = '\0';
        return;
    }

    if (Transform.Constraint != ParametricTransformConstraint::Free &&
        Transform.Constraint != ParametricTransformConstraint::Screen)
    {
        Transform.Constraint = ParametricTransformConstraint::Free;
        Transform.SlideAlongCurve = false;
    }
}

void ClearTransformNumeric(ParametricTransformState& Transform)
{
    Transform.Numeric[0] = '\0';
}

bool ResolveNumericOverride(const ParametricTransformState& Transform,
                            double& Value)
{
    if (Transform.Numeric[0] == '\0')
        return false;

    char* End = nullptr;
    Value = std::strtod(Transform.Numeric, &End);
    return End != Transform.Numeric;
}

bool ResolveMoveSlideRequested(std::uint32_t MoveTapCount,
                              double SessionMilliseconds,
                              double LastGPressedMilliseconds,
                              bool CurveAvailable)
{
    return CurveAvailable
        && (MoveTapCount >= 2u
         || (MoveTapCount > 0u && (SessionMilliseconds - LastGPressedMilliseconds) <= 350.0));
}

void FormatTransformCommand(const ParametricTransformState& Transform,
                            char* Delivered,
                            std::size_t Capacity)
{
    if (Capacity == 0u)
        return;
    Delivered[0] = '\0';

    const bool ShowAxisConstraint = Transform.Constraint == ParametricTransformConstraint::AxisX
                                 || Transform.Constraint == ParametricTransformConstraint::AxisZ;
    if (Transform.Mode == ParametricTransformMode::Move && Transform.SlideAlongCurve)
    {
        if (Transform.Numeric[0] != '\0')
            std::snprintf(Delivered, Capacity, "G G %s", Transform.Numeric);
        else
            std::snprintf(Delivered, Capacity, "G G");
        return;
    }

    if (Transform.Numeric[0] != '\0' && ShowAxisConstraint)
        std::snprintf(Delivered, Capacity, "%s %s %s",
                      TransformCommandToken(Transform.Mode),
                      TransformConstraintText(Transform.Constraint),
                      Transform.Numeric);
    else if (Transform.Numeric[0] != '\0')
        std::snprintf(Delivered, Capacity, "%s %s",
                      TransformCommandToken(Transform.Mode),
                      Transform.Numeric);
    else if (ShowAxisConstraint)
        std::snprintf(Delivered, Capacity, "%s %s",
                      TransformCommandToken(Transform.Mode),
                      TransformConstraintText(Transform.Constraint));
    else
        std::snprintf(Delivered, Capacity, "%s",
                      TransformCommandToken(Transform.Mode));
}

bool ResolvePlanarCoordinates(const SpatialBasis& Basis,
                              const SpatialPoint& Position,
                              double& Along,
                              double& Across)
{
    const SpatialDirection Offset = Difference(Basis.Origin, Position);
    Along = Dot(Offset, Basis.Along);
    Across = Dot(Offset, Basis.Across);
    return true;
}

SpatialPoint ResolvePlanarPointLocal(const SpatialBasis& Basis,
                                     double Along,
                                     double Across)
{
    return ResolvePlanarPoint(Basis, Along, Across);
}

bool ProjectSpatialPoint(const SpatialBasis& Basis,
                         const ParametricViewportState& View,
                         bool Perspective,
                         const PlaneExtent& Extent,
                         const SpatialPoint& Position,
                         float& ScreenX,
                         float& ScreenY)
{
    double Along = 0.0;
    double Across = 0.0;
    ResolvePlanarCoordinates(Basis, Position, Along, Across);
    return ProjectViewportPoint(Basis, View, Perspective, Extent, Along, Across, ScreenX, ScreenY);
}

WorkspaceRecordName ResolveSelectedRecord(const WorkspaceDirectoryProjection& Directory,
                                          const ParametricWorkspaceContext& Applied)
{
    if (Applied.RowTaken >= Directory.Rows.size())
        return {};
    const WorkspaceDirectoryRow& Row = Directory.Rows[Applied.RowTaken];
    return Row.Kind == WorkspaceDirectoryRowKind::Record ? Row.Record : WorkspaceRecordName{};
}

bool ResolveSketchPointPositionLocal(const SketchStructure& Sketch,
                                     SketchPointName Subject,
                                     SpatialPoint& Position)
{
    if (!Subject.Assigned())
        return false;

    const std::uint32_t CurveIndex = Subject.IssuedIndex >> 8u;
    if (CurveIndex == 0u)
        return false;

    std::vector<SketchPointPlacement> Points;
    if (!ResolveSketchPoints(Sketch, { CurveIndex }, Points))
        return false;

    for (const SketchPointPlacement& Current : Points)
        if (Current.Name.IssuedIndex == Subject.IssuedIndex)
        {
            Position = Current.Position;
            return true;
        }

    return false;
}

bool ProfileContainsCurve(const ProfileSpecification& Profile,
                          SketchCurveName Curve)
{
    for (const ProfileLoop& Loop : Profile.HeldLoops())
        for (const ProfileCurveUse& Use : Loop.Traversal)
            if (Use.TraversedCurve.IssuedIndex == Curve.IssuedIndex)
                return true;
    return false;
}

WorkspaceRecordName ResolveRecordForPoint(const WorkspaceRecordStructure& Records,
                                          SketchPointName Point)
{
    for (std::uint32_t Index = 1u; Index <= Records.DeclaredCount(); ++Index)
    {
        const WorkspaceRecord* Record = Records.Resolve({ Index });
        if (Record != nullptr && Record->SketchPoint.IssuedIndex == Point.IssuedIndex)
            return { Index };
    }

    const std::uint32_t CurveIndex = Point.IssuedIndex >> 8u;
    if (CurveIndex != 0u)
        for (std::uint32_t Index = 1u; Index <= Records.DeclaredCount(); ++Index)
        {
            const WorkspaceRecord* Record = Records.Resolve({ Index });
            if (Record != nullptr && Record->SketchCurve.IssuedIndex == CurveIndex)
                return { Index };
        }

    return {};
}

WorkspaceRecordName ResolveRecordForCurve(const SketchStructure& Sketch,
                                          const WorkspaceRecordStructure& Records,
                                          SketchCurveName Curve)
{
    for (std::uint32_t Index = 1u; Index <= Records.DeclaredCount(); ++Index)
    {
        const WorkspaceRecord* Record = Records.Resolve({ Index });
        if (Record != nullptr && Record->SketchCurve.IssuedIndex == Curve.IssuedIndex)
            return { Index };
    }

    for (std::uint32_t Index = 1u; Index <= Records.DeclaredCount(); ++Index)
    {
        const WorkspaceRecord* Record = Records.Resolve({ Index });
        if (Record == nullptr || !Record->Profile.Assigned() ||
            Record->Profile.IssuedIndex > Sketch.Profiles().size())
            continue;
        if (ProfileContainsCurve(Sketch.Profiles()[Record->Profile.IssuedIndex - 1u], Curve))
            return { Index };
    }

    return {};
}

bool ResolveCurvePivot(const SketchStructure& Sketch,
                       SketchCurveName Curve,
                       SpatialPoint& Pivot)
{
    if (!Curve.Assigned() || Curve.IssuedIndex > Sketch.Curves().size())
        return false;

    const CurveSpecification& Geometry = Sketch.Curves()[Curve.IssuedIndex - 1u].Geometry;
    switch (Geometry.Subject())
    {
        case CurveSubject::Line:
        {
            const LineCurve& Line = Geometry.HeldLine();
            Pivot = { (Line.Origin.Left + Line.Terminus.Left) * 0.5,
                      (Line.Origin.Up + Line.Terminus.Up) * 0.5,
                      (Line.Origin.Forward + Line.Terminus.Forward) * 0.5 };
            return true;
        }
        case CurveSubject::CircularArc:
            Pivot = Geometry.HeldCircularArc().Centre;
            return true;
        case CurveSubject::Circle:
            Pivot = Geometry.HeldCircle().Centre;
            return true;
        case CurveSubject::EllipticalArc:
            Pivot = Geometry.HeldEllipticalArc().Centre;
            return true;
        case CurveSubject::Ellipse:
            Pivot = Geometry.HeldEllipse().Centre;
            return true;
        case CurveSubject::Bezier:
            if (!Geometry.HeldBezier().ControlPoints.empty())
            {
                Pivot = Geometry.HeldBezier().ControlPoints[Geometry.HeldBezier().ControlPoints.size() / 2u];
                return true;
            }
            return false;
        case CurveSubject::BasisSpline:
            if (!Geometry.HeldBasisSpline().ControlPoints.empty())
            {
                Pivot = Geometry.HeldBasisSpline().ControlPoints[Geometry.HeldBasisSpline().ControlPoints.size() / 2u];
                return true;
            }
            return false;
        case CurveSubject::RationalSpline:
            if (!Geometry.HeldRationalSpline().ControlPoints.empty())
            {
                Pivot = Geometry.HeldRationalSpline().ControlPoints[Geometry.HeldRationalSpline().ControlPoints.size() / 2u];
                return true;
            }
            return false;
        case CurveSubject::Hermite:
        {
            const HermiteCurve& Hermite = Geometry.HeldHermite();
            Pivot = { (Hermite.StartPoint.Left + Hermite.EndPoint.Left) * 0.5,
                      (Hermite.StartPoint.Up + Hermite.EndPoint.Up) * 0.5,
                      (Hermite.StartPoint.Forward + Hermite.EndPoint.Forward) * 0.5 };
            return true;
        }
        case CurveSubject::SubjectCount:
            return false;
    }
    return false;
}

bool ResolveProfilePivot(const SketchStructure& Sketch,
                         ProfileNameInFeature Profile,
                         SpatialPoint& Pivot)
{
    if (!Profile.Assigned() || Profile.IssuedIndex > Sketch.Profiles().size())
        return false;

    const ProfileSpecification& Source = Sketch.Profiles()[Profile.IssuedIndex - 1u];
    std::uint32_t Count = 0u;
    Pivot = {};

    for (const ProfileLoop& Loop : Source.HeldLoops())
        for (const ProfileCurveUse& Use : Loop.Traversal)
        {
            SpatialPoint CurvePivot = {};
            if (!ResolveCurvePivot(Sketch, { Use.TraversedCurve.IssuedIndex }, CurvePivot))
                continue;
            Pivot = { Pivot.Left + CurvePivot.Left,
                      Pivot.Up + CurvePivot.Up,
                      Pivot.Forward + CurvePivot.Forward };
            ++Count;
        }

    if (Count == 0u)
        return false;
    const double Scale = 1.0 / static_cast<double>(Count);
    Pivot = { Pivot.Left * Scale, Pivot.Up * Scale, Pivot.Forward * Scale };
    return true;
}

void AppendPlacementUnique(std::vector<ParametricTransformPlacement>& Placements,
                           const ParametricTransformPlacement& Placement)
{
    for (const ParametricTransformPlacement& Existing : Placements)
    {
        if (Placement.ControlPlacement == Existing.ControlPlacement)
        {
            if (!Placement.ControlPlacement && Placement.Point.IssuedIndex == Existing.Point.IssuedIndex)
                return;
            if (Placement.ControlPlacement && Placement.Control.IssuedIndex == Existing.Control.IssuedIndex)
                return;
        }
    }
    Placements.push_back(Placement);
}

void CollectCurvePlacements(const SketchStructure& Sketch,
                            SketchCurveName Curve,
                            std::vector<ParametricTransformPlacement>& Placements)
{
    std::vector<SketchPointPlacement> Points;
    if (ResolveSketchPoints(Sketch, Curve, Points))
        for (const SketchPointPlacement& Point : Points)
            AppendPlacementUnique(Placements, { false, Point.Name, {}, Point.Position });

    std::vector<SketchControlPlacement> Controls;
    if (ResolveSketchControls(Sketch, Curve, Controls))
        for (const SketchControlPlacement& Control : Controls)
            AppendPlacementUnique(Placements, { true, {}, Control.Name, Control.Position });
}

void CollectProfilePlacements(const SketchStructure& Sketch,
                              ProfileNameInFeature Profile,
                              std::vector<ParametricTransformPlacement>& Placements)
{
    if (!Profile.Assigned() || Profile.IssuedIndex > Sketch.Profiles().size())
        return;

    for (const ProfileLoop& Loop : Sketch.Profiles()[Profile.IssuedIndex - 1u].HeldLoops())
        for (const ProfileCurveUse& Use : Loop.Traversal)
            CollectCurvePlacements(Sketch, { Use.TraversedCurve.IssuedIndex }, Placements);
}

ParametricViewportSelection ResolveViewportPick(const SketchStructure& Sketch,
                                                const WorkspaceRecordStructure& Records,
                                                const SpatialPoint& Probe,
                                                double MaximumDistance)
{
    SketchPointPlacement Point = {};
    double Distance = MaximumDistance;
    if (ResolveNearestSketchPoint(Sketch, Probe, MaximumDistance, Point, Distance))
    {
        ParametricViewportSelection Pick = {};
        Pick.Subject = ParametricSelectionSubject::Point;
        Pick.Point = Point.Name;
        Pick.Curve = Point.SourceCurve;
        Pick.Position = Point.Position;
        Pick.Record = ResolveRecordForPoint(Records, Point.Name);
        if (Pick.Record.Assigned())
            return Pick;
    }

    SketchControlPlacement Control = {};
    Distance = MaximumDistance;
    if (ResolveNearestSketchControl(Sketch, Probe, MaximumDistance, Control, Distance))
    {
        ParametricViewportSelection Pick = {};
        Pick.Subject = ParametricSelectionSubject::Control;
        Pick.Control = Control.Name;
        Pick.Curve = Control.SourceCurve;
        Pick.Position = Control.Position;
        Pick.Record = ResolveRecordForCurve(Sketch, Records, Control.SourceCurve);
        if (Pick.Record.Assigned())
            return Pick;
    }

    SketchCurveName Curve = {};
    Distance = MaximumDistance;
    if (ResolveNearestSketchCurve(Sketch, Probe, MaximumDistance, Curve, Distance))
    {
        ParametricViewportSelection Pick = {};
        Pick.Subject = ParametricSelectionSubject::Curve;
        Pick.Curve = Curve;
        Pick.Record = ResolveRecordForCurve(Sketch, Records, Curve);
        ResolveCurvePivot(Sketch, Curve, Pick.Position);
        return Pick;
    }

    return {};
}

bool ResolveRecordSelection(const SketchStructure& Sketch,
                            const WorkspaceRecordStructure& Records,
                            WorkspaceRecordName RecordName,
                            ParametricViewportSelection& Selection)
{
    Selection = {};
    const WorkspaceRecord* Record = Records.Resolve(RecordName);
    if (Record == nullptr)
        return false;

    Selection.Record = RecordName;
    switch (Record->Subject)
    {
        case WorkspaceRecordSubject::Point:
            Selection.Subject = ParametricSelectionSubject::Point;
            Selection.Point = Record->SketchPoint;
            return ResolveSketchPointPositionLocal(Sketch, Record->SketchPoint, Selection.Position);
        case WorkspaceRecordSubject::OpenCurve:
            Selection.Subject = ParametricSelectionSubject::Curve;
            Selection.Curve = Record->SketchCurve;
            return ResolveCurvePivot(Sketch, Record->SketchCurve, Selection.Position);
        case WorkspaceRecordSubject::ClosedProfile:
        case WorkspaceRecordSubject::ThinSurface:
        case WorkspaceRecordSubject::Solid:
            Selection.Subject = ParametricSelectionSubject::Record;
            return ResolveProfilePivot(Sketch, Record->Profile, Selection.Position);
        default:
            return false;
    }
}

ParametricViewportSelection ResolveEditableSelection(const SketchStructure& Sketch,
                                                     const WorkspaceRecordStructure& Records,
                                                     WorkspaceRecordName SelectedRecord,
                                                     WorkspaceRecordName PendingSelection,
                                                     const ParametricViewportSelection& SemanticSelection)
{
    if (SemanticSelection.Standing() &&
        ((!SelectedRecord.Assigned() || SemanticSelection.Record.IssuedIndex == SelectedRecord.IssuedIndex) ||
         (PendingSelection.Assigned() && PendingSelection.IssuedIndex == SemanticSelection.Record.IssuedIndex)))
        return SemanticSelection;

    ParametricViewportSelection Selection = {};
    ResolveRecordSelection(Sketch, Records, SelectedRecord, Selection);
    return Selection;
}

bool ResolveTransformPlacements(const SketchStructure& Sketch,
                                const WorkspaceRecordStructure& Records,
                                const ParametricViewportSelection& Target,
                                WorkspaceRecordName& RecordName,
                                SpatialPoint& Pivot,
                                std::vector<ParametricTransformPlacement>& Placements)
{
    Placements.clear();
    RecordName = Target.Record;

    if (Target.Subject == ParametricSelectionSubject::Point)
    {
        Placements.push_back({ false, Target.Point, {}, Target.Position });
        Pivot = Target.Position;
        return true;
    }

    if (Target.Subject == ParametricSelectionSubject::Control)
    {
        Placements.push_back({ true, {}, Target.Control, Target.Position });
        Pivot = Target.Position;
        return true;
    }

    if (Target.Subject == ParametricSelectionSubject::Curve)
    {
        CollectCurvePlacements(Sketch, Target.Curve, Placements);
        if (!ResolveCurvePivot(Sketch, Target.Curve, Pivot))
            Pivot = Target.Position;
        return !Placements.empty();
    }

    if (Target.Subject == ParametricSelectionSubject::Record)
    {
        const WorkspaceRecord* Record = Records.Resolve(Target.Record);
        if (Record == nullptr)
            return false;
        if (Record->SketchCurve.Assigned())
            CollectCurvePlacements(Sketch, Record->SketchCurve, Placements);
        else if (Record->Profile.Assigned())
            CollectProfilePlacements(Sketch, Record->Profile, Placements);
        Pivot = Target.Position;
        return !Placements.empty();
    }

    return false;
}

SpatialDirection ResolveCurveSlideDirection(const SpatialBasis& Basis,
                                            const SketchStructure& Sketch,
                                            SketchCurveName Curve,
                                            const SpatialPoint& NearPosition)
{
    if (!Curve.Assigned() || Curve.IssuedIndex > Sketch.Curves().size())
        return Basis.Along;

    std::vector<SpatialPoint> Polyline;
    AppendCurvePolyline(Sketch.Curves()[Curve.IssuedIndex - 1u].Geometry, Polyline, 48u);
    if (Polyline.size() < 2u)
        return Basis.Along;

    double BestDistanceSquared = 1.0e30;
    SpatialDirection BestDirection = Basis.Along;
    for (std::size_t Index = 0u; Index + 1u < Polyline.size(); ++Index)
    {
        const SpatialPoint& StartPoint = Polyline[Index];
        const SpatialPoint& EndPoint = Polyline[Index + 1u];
        const SpatialDirection Segment = Difference(StartPoint, EndPoint);
        const double SegmentLengthSquared = LengthSquared(Segment);
        if (SegmentLengthSquared <= 1.0e-12)
            continue;

        const SpatialDirection Offset = Difference(StartPoint, NearPosition);
        const double Parameter = std::clamp(Dot(Offset, Segment) / SegmentLengthSquared, 0.0, 1.0);
        const SpatialPoint Closest = Added(StartPoint, Scaled(Segment, Parameter));
        const double CandidateDistanceSquared = LengthSquared(Difference(Closest, NearPosition));
        if (CandidateDistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = CandidateDistanceSquared;
            BestDirection = Normalize(Segment);
        }
    }

    return BestDirection;
}

void ApplyTransformPlacements(SketchStructure& Sketch,
                              const SpatialBasis& Basis,
                              const ParametricTransformState& Transform,
                              double AlongOffset,
                              double AcrossOffset,
                              double AngleRadians,
                              double ScaleFactor)
{
    for (std::size_t Index = 0u; Index < Transform.Placements.size(); ++Index)
    {
        const SpatialPoint& Origin = Transform.Origins[Index];
        double Along = 0.0;
        double Across = 0.0;
        ResolvePlanarCoordinates(Basis, Origin, Along, Across);

        if (Transform.Mode == ParametricTransformMode::Move)
        {
            Along += AlongOffset;
            Across += AcrossOffset;
        }
        else if (Transform.Mode == ParametricTransformMode::Rotate)
        {
            const double LocalAlong = Along - Transform.PivotAlong;
            const double LocalAcross = Across - Transform.PivotAcross;
            const double Cosine = std::cos(AngleRadians);
            const double Sine = std::sin(AngleRadians);
            Along = Transform.PivotAlong + LocalAlong * Cosine - LocalAcross * Sine;
            Across = Transform.PivotAcross + LocalAlong * Sine + LocalAcross * Cosine;
        }
        else if (Transform.Mode == ParametricTransformMode::Scale)
        {
            if (Transform.Constraint == ParametricTransformConstraint::AxisX)
                Along = Transform.PivotAlong + (Along - Transform.PivotAlong) * ScaleFactor;
            else if (Transform.Constraint == ParametricTransformConstraint::AxisZ)
                Across = Transform.PivotAcross + (Across - Transform.PivotAcross) * ScaleFactor;
            else
            {
                Along = Transform.PivotAlong + (Along - Transform.PivotAlong) * ScaleFactor;
                Across = Transform.PivotAcross + (Across - Transform.PivotAcross) * ScaleFactor;
            }
        }

        const SpatialPoint Position = ResolvePlanarPointLocal(Basis, Along, Across);
        const ParametricTransformPlacement& Placement = Transform.Placements[Index];
        if (Placement.ControlPlacement)
            Discard(EnforceSketchControl(Sketch, Placement.Control, Position));
        else
            Discard(EnforceSketchPoint(Sketch, Placement.Point, Position));
    }
}

void RestoreTransformPlacements(SketchStructure& Sketch,
                                const ParametricTransformState& Transform)
{
    for (std::size_t Index = 0u; Index < Transform.Placements.size(); ++Index)
    {
        const ParametricTransformPlacement& Placement = Transform.Placements[Index];
        const SpatialPoint& Origin = Transform.Origins[Index];
        if (Placement.ControlPlacement)
            Discard(EnforceSketchControl(Sketch, Placement.Control, Origin));
        else
            Discard(EnforceSketchPoint(Sketch, Placement.Point, Origin));
    }
}

void ClearTransformSession(ParametricTransformState& Transform)
{
    Transform.Engaged = false;
    Transform.AwaitingRelease = false;
    Transform.Changed = false;
    Transform.SlideAlongCurve = false;
    Transform.Constraint = ParametricTransformConstraint::Free;
    Transform.Target = {};
    Transform.Record = {};
    Transform.Placements.clear();
    Transform.Origins.clear();
    Transform.Numeric[0] = '\0';
    Transform.PreviewValue = 0.0;
}

bool StartTransformSession(const PlaneExtent& Extent,
                           const PointerCondition& Pointer,
                           const SpatialBasis& Basis,
                           const ParametricViewportState& View,
                           bool Perspective,
                           const SketchStructure& Sketch,
                           const WorkspaceRecordStructure& Records,
                           const ParametricViewportSelection& Target,
                           ParametricTransformMode Mode,
                           ParametricTransformConstraint Constraint,
                           bool SlideAlongCurve,
                           bool MouseDriven,
                           ParametricTransformState& Transform)
{
    WorkspaceRecordName RecordName = {};
    SpatialPoint Pivot = {};
    std::vector<ParametricTransformPlacement> Placements;
    if (!ResolveTransformPlacements(Sketch, Records, Target, RecordName, Pivot, Placements))
        return false;

    ClearTransformSession(Transform);
    Transform.Mode = Mode;
    Transform.Engaged = true;
    Transform.AwaitingRelease = MouseDriven;
    Transform.Constraint = Constraint;
    Transform.SlideAlongCurve = SlideAlongCurve;
    Transform.Target = Target;
    Transform.Record = RecordName;
    Transform.Pivot = Pivot;
    ResolvePlanarCoordinates(Basis, Pivot, Transform.PivotAlong, Transform.PivotAcross);
    Transform.Placements = Placements;
    Transform.Origins.reserve(Placements.size());
    for (const ParametricTransformPlacement& Placement : Placements)
        Transform.Origins.push_back(Placement.Position);
    Transform.CurveDirection = SlideAlongCurve || Constraint == ParametricTransformConstraint::Curve
                             ? ResolveCurveSlideDirection(Basis, Sketch, Target.Curve, Target.Position)
                             : Basis.Along;

    SpatialPoint Probe = Pivot;
    if (ResolveViewportPlaneIntersection(Basis, View, Perspective, Extent,
                                         Pointer.PositionX, Pointer.PositionY, Probe))
        ResolvePlanarCoordinates(Basis, Probe, Transform.StartAlong, Transform.StartAcross);
    else
    {
        Transform.StartAlong = Transform.PivotAlong;
        Transform.StartAcross = Transform.PivotAcross;
    }

    const double OffsetAlong = Transform.StartAlong - Transform.PivotAlong;
    const double OffsetAcross = Transform.StartAcross - Transform.PivotAcross;
    Transform.StartDistance = std::sqrt(OffsetAlong * OffsetAlong + OffsetAcross * OffsetAcross);
    if (Transform.StartDistance < 1.0e-4)
        Transform.StartDistance = 1.0;
    Transform.StartAngle = std::atan2(OffsetAcross, OffsetAlong);
    return true;
}

void UpdateTransformSession(const PlaneExtent& Extent,
                            const PointerCondition& Pointer,
                            const ModifierCondition& Modifiers,
                            const SpatialBasis& Basis,
                            const ParametricViewportState& View,
                            bool Perspective,
                            SketchStructure& Sketch,
                            ParametricTransformState& Transform)
{
    if (!Transform.Engaged)
        return;

    SpatialPoint Probe = Transform.Pivot;
    if (!ResolveViewportPlaneIntersection(Basis, View, Perspective, Extent,
                                          Pointer.PositionX, Pointer.PositionY, Probe))
        return;

    double Along = 0.0;
    double Across = 0.0;
    ResolvePlanarCoordinates(Basis, Probe, Along, Across);

    double AlongOffset = Along - Transform.StartAlong;
    double AcrossOffset = Across - Transform.StartAcross;
    if (Transform.Constraint == ParametricTransformConstraint::AxisX)
        AcrossOffset = 0.0;
    else if (Transform.Constraint == ParametricTransformConstraint::AxisZ)
        AlongOffset = 0.0;
    else if (Transform.Constraint == ParametricTransformConstraint::Curve)
    {
        const double AlongDirection = Dot(Transform.CurveDirection, Basis.Along);
        const double AcrossDirection = Dot(Transform.CurveDirection, Basis.Across);
        const double Projection = AlongOffset * AlongDirection + AcrossOffset * AcrossDirection;
        AlongOffset = AlongDirection * Projection;
        AcrossOffset = AcrossDirection * Projection;
    }

    double Angle = std::atan2(Across - Transform.PivotAcross, Along - Transform.PivotAlong) - Transform.StartAngle;
    while (Angle > Pi) Angle -= 2.0 * Pi;
    while (Angle < -Pi) Angle += 2.0 * Pi;
    double Scale = std::sqrt((Along - Transform.PivotAlong) * (Along - Transform.PivotAlong)
                           + (Across - Transform.PivotAcross) * (Across - Transform.PivotAcross)) / Transform.StartDistance;
    if (Scale < 0.05)
        Scale = 0.05;

    double Numeric = 0.0;
    const bool HasNumeric = ResolveNumericOverride(Transform, Numeric);

    if (Transform.Mode == ParametricTransformMode::Move)
    {
        if (HasNumeric)
        {
            if (Transform.Constraint == ParametricTransformConstraint::AxisZ)
            {
                AlongOffset = 0.0;
                AcrossOffset = Numeric;
            }
            else if (Transform.Constraint == ParametricTransformConstraint::Curve)
            {
                AlongOffset = Dot(Transform.CurveDirection, Basis.Along) * Numeric;
                AcrossOffset = Dot(Transform.CurveDirection, Basis.Across) * Numeric;
            }
            else
            {
                Transform.Constraint = ParametricTransformConstraint::AxisX;
                AlongOffset = Numeric;
                AcrossOffset = 0.0;
            }
        }
        if (Modifiers.Commanded)
        {
            const SpatialPoint SnappedProbe = ResolvePlanarPointLocal(Basis,
                Transform.StartAlong + AlongOffset,
                Transform.StartAcross + AcrossOffset);
            const SketchSnapPlacement Snap = ResolveNearestSnap(Sketch, SnappedProbe,
                ResolveSnapTolerance(View, Perspective));
            if (Snap.Resolved())
            {
                double SnapAlong = 0.0;
                double SnapAcross = 0.0;
                ResolvePlanarCoordinates(Basis, Snap.Position, SnapAlong, SnapAcross);
                AlongOffset += SnapAlong - (Transform.StartAlong + AlongOffset);
                AcrossOffset += SnapAcross - (Transform.StartAcross + AcrossOffset);
            }
        }
        Transform.PreviewValue = std::sqrt(AlongOffset * AlongOffset + AcrossOffset * AcrossOffset);
    }
    else if (Transform.Mode == ParametricTransformMode::Rotate)
    {
        if (HasNumeric)
            Angle = Numeric * Pi / 180.0;
        else if (Modifiers.Commanded)
            Angle = std::round(Angle * 180.0 / Pi / 5.0) * 5.0 * Pi / 180.0;
        Transform.PreviewValue = Angle * 180.0 / Pi;
    }
    else
    {
        if (HasNumeric)
            Scale = Numeric;
        else if (Modifiers.Commanded)
            Scale = std::max(0.05, std::round(Scale * 10.0) / 10.0);
        Transform.PreviewValue = Scale;
    }

    ApplyTransformPlacements(Sketch, Basis, Transform, AlongOffset, AcrossOffset, Angle, Scale);
    Transform.Changed = true;
}

void CommitTransformSession(const WorkspaceRecordStructure& Records,
                            WorkspaceRevisionSequence& Revisions,
                            ParametricTransformState& Transform)
{
    if (Transform.Changed && Transform.Record.Assigned())
    {
        const WorkspaceRecord* Record = Records.Resolve(Transform.Record);
        if (Record != nullptr)
            Revisions.Seal(std::string(TransformModeText(Transform.Mode)) + " " + Record->Naming,
                           "Edit Sketch", { Transform.Record }, Revisions.DeclaredCount() + 1u);
    }
    ClearTransformSession(Transform);
}

void CancelTransformSession(SketchStructure& Sketch,
                            ParametricTransformState& Transform)
{
    RestoreTransformPlacements(Sketch, Transform);
    ClearTransformSession(Transform);
}

void RecordViewportGridOverlay(OverlayGeometry& Overlay,
                               const PlaneExtent& Extent,
                               const SketchStructure& Sketch,
                               const ParametricViewportState& View,
                               bool Perspective,
                               const EditorPanelConfiguration& Configuration)
{
    if (!Sketch.Declared() || Configuration.Lattice == PanelLatticePresentation::None)
        return;

    const SpatialBasis Basis = ResolveSketchBasis(Sketch);
    const double Step = std::max(Configuration.LatticeCellMetres * static_cast<double>(Configuration.LatticeScale), 1.0);
    const std::uint32_t Subdivisions = std::max(Configuration.Subdivisions, 2u);
    const std::int32_t Count = Perspective ? 40 : 80;

    for (std::int32_t Index = -Count; Index <= Count; ++Index)
    {
        const bool Major = (std::abs(Index) % static_cast<std::int32_t>(Subdivisions)) == 0;
        const std::uint32_t Packed = Major ? OverlayPacked(0xC4u, 0xC8u, 0xD6u, 56u)
                                           : OverlayPacked(0xC4u, 0xC8u, 0xD6u, 26u);
        const float Weight = Major ? Configuration.LatticeLineWeight + 0.25f : Configuration.LatticeLineWeight;

        float X0 = 0.0f, Y0 = 0.0f, X1 = 0.0f, Y1 = 0.0f;
        if (ProjectViewportPoint(Basis, View, Perspective, Extent, static_cast<double>(Index) * Step, static_cast<double>(-Count) * Step, X0, Y0) &&
            ProjectViewportPoint(Basis, View, Perspective, Extent, static_cast<double>(Index) * Step, static_cast<double>( Count) * Step, X1, Y1))
            Overlay.AddLine(X0, Y0, X1, Y1, Packed, Weight);

        if (ProjectViewportPoint(Basis, View, Perspective, Extent, static_cast<double>(-Count) * Step, static_cast<double>(Index) * Step, X0, Y0) &&
            ProjectViewportPoint(Basis, View, Perspective, Extent, static_cast<double>( Count) * Step, static_cast<double>(Index) * Step, X1, Y1))
            Overlay.AddLine(X0, Y0, X1, Y1, Packed, Weight);
    }

    float X0 = 0.0f, Y0 = 0.0f, X1 = 0.0f, Y1 = 0.0f;
    if (Configuration.AxisX &&
        ProjectViewportPoint(Basis, View, Perspective, Extent, -Count * Step, 0.0, X0, Y0) &&
        ProjectViewportPoint(Basis, View, Perspective, Extent, Count * Step, 0.0, X1, Y1))
        Overlay.AddLine(X0, Y0, X1, Y1, OverlayPacked(0xFCu, 0x5Au, 0x5Au, 208u), 1.6f);

    if (Configuration.AxisZ &&
        ProjectViewportPoint(Basis, View, Perspective, Extent, 0.0, -Count * Step, X0, Y0) &&
        ProjectViewportPoint(Basis, View, Perspective, Extent, 0.0, Count * Step, X1, Y1))
        Overlay.AddLine(X0, Y0, X1, Y1, OverlayPacked(0x5Au, 0x8Bu, 0xFCu, 208u), 1.6f);
}

void RecordViewportOverlayFallback(RecordingSurface& Surface,
                                   const PlaneExtent& Extent,
                                   const OverlayGeometry& Overlay)
{
    Surface.Confine(Extent);
    for (std::uint32_t Index = 0u; Index < Overlay.LineCount; ++Index)
    {
        const OverlayLine& Line = Overlay.Lines[Index];
        const float PointsX[2] = { Line.X0, Line.X1 };
        const float PointsY[2] = { Line.Y0, Line.Y1 };
        Surface.Polyline(PointsX, PointsY, 2u, TokenFromPacked(Line.Packed), Line.Thickness);
    }
    for (std::uint32_t Index = 0u; Index < Overlay.DotCount; ++Index)
    {
        const OverlayDot& Dot = Overlay.Dots[Index];
        Surface.Medallion(Dot.X, Dot.Y, Dot.Radius, TokenFromPacked(Dot.Packed));
    }
    for (std::uint32_t Index = 0u; Index < Overlay.TriangleCount; ++Index)
    {
        const OverlayTriangle& Triangle = Overlay.Triangles[Index];
        const float Corners[6] = { Triangle.X0, Triangle.Y0, Triangle.X1, Triangle.Y1, Triangle.X2, Triangle.Y2 };
        Surface.Tongue(Corners, 3u, TokenFromPacked(Triangle.Packed));
    }
    Surface.Release();
}

void AppendOverlayCircle(OverlayGeometry& Overlay,
                         float CentreX,
                         float CentreY,
                         float Radius,
                         std::uint32_t Packed,
                         float Thickness,
                         std::uint32_t SegmentCount = 20u)
{
    for (std::uint32_t Index = 0u; Index < SegmentCount; ++Index)
    {
        const double A0 = (static_cast<double>(Index) / static_cast<double>(SegmentCount)) * 2.0 * Pi;
        const double A1 = (static_cast<double>(Index + 1u) / static_cast<double>(SegmentCount)) * 2.0 * Pi;
        Overlay.AddLine(CentreX + static_cast<float>(std::cos(A0) * Radius),
                        CentreY + static_cast<float>(std::sin(A0) * Radius),
                        CentreX + static_cast<float>(std::cos(A1) * Radius),
                        CentreY + static_cast<float>(std::sin(A1) * Radius),
                        Packed, Thickness);
    }
}

void AppendOverlayArrow(OverlayGeometry& Overlay,
                        float X0,
                        float Y0,
                        float X1,
                        float Y1,
                        std::uint32_t Packed,
                        float Thickness)
{
    Overlay.AddLine(X0, Y0, X1, Y1, Packed, Thickness);
    const float DX = X1 - X0;
    const float DY = Y1 - Y0;
    const float Length = std::sqrt(DX * DX + DY * DY);
    if (Length <= 1.0e-4f)
        return;
    const float UX = DX / Length;
    const float UY = DY / Length;
    const float PX = -UY;
    const float PY = UX;
    const float BaseX = X1 - UX * 11.0f;
    const float BaseY = Y1 - UY * 11.0f;
    Overlay.AddTriangle(X1, Y1,
                        BaseX + PX * 5.0f, BaseY + PY * 5.0f,
                        BaseX - PX * 5.0f, BaseY - PY * 5.0f,
                        Packed);
}

void AppendOverlaySquare(OverlayGeometry& Overlay,
                         float CentreX,
                         float CentreY,
                         float HalfExtent,
                         std::uint32_t Packed,
                         float Thickness)
{
    Overlay.AddLine(CentreX - HalfExtent, CentreY - HalfExtent, CentreX + HalfExtent, CentreY - HalfExtent, Packed, Thickness);
    Overlay.AddLine(CentreX + HalfExtent, CentreY - HalfExtent, CentreX + HalfExtent, CentreY + HalfExtent, Packed, Thickness);
    Overlay.AddLine(CentreX + HalfExtent, CentreY + HalfExtent, CentreX - HalfExtent, CentreY + HalfExtent, Packed, Thickness);
    Overlay.AddLine(CentreX - HalfExtent, CentreY + HalfExtent, CentreX - HalfExtent, CentreY - HalfExtent, Packed, Thickness);
}

void AppendOverlayArc(OverlayGeometry& Overlay,
                      float CentreX,
                      float CentreY,
                      float Radius,
                      float StartRadians,
                      float SweepRadians,
                      std::uint32_t Packed,
                      float Thickness,
                      std::uint32_t SegmentCount = 18u)
{
    for (std::uint32_t Index = 0u; Index < SegmentCount; ++Index)
    {
        const double A0 = static_cast<double>(StartRadians)
                        + static_cast<double>(SweepRadians) * (static_cast<double>(Index) / static_cast<double>(SegmentCount));
        const double A1 = static_cast<double>(StartRadians)
                        + static_cast<double>(SweepRadians) * (static_cast<double>(Index + 1u) / static_cast<double>(SegmentCount));
        Overlay.AddLine(CentreX + static_cast<float>(std::cos(A0) * Radius),
                        CentreY + static_cast<float>(std::sin(A0) * Radius),
                        CentreX + static_cast<float>(std::cos(A1) * Radius),
                        CentreY + static_cast<float>(std::sin(A1) * Radius),
                        Packed, Thickness);
    }
}

enum class ParametricGizmoHandle : std::uint32_t
{
    None = 0u,
    MoveFree = 1u,
    MoveX = 2u,
    MoveZ = 3u,
    Rotate = 4u,
    ScaleFree = 5u,
    ScaleX = 6u,
    ScaleZ = 7u
};

double DistanceSquared2(float X0, float Y0, float X1, float Y1)
{
    const double DX = static_cast<double>(X1 - X0);
    const double DY = static_cast<double>(Y1 - Y0);
    return DX * DX + DY * DY;
}

double DistancePointSegmentSquared2(float PX, float PY,
                                    float AX, float AY,
                                    float BX, float BY)
{
    const double DX = static_cast<double>(BX - AX);
    const double DY = static_cast<double>(BY - AY);
    const double LengthSquared = DX * DX + DY * DY;
    if (LengthSquared <= 1.0e-12)
        return DistanceSquared2(PX, PY, AX, AY);

    const double Parameter = std::clamp(((static_cast<double>(PX - AX) * DX)
                                       + (static_cast<double>(PY - AY) * DY)) / LengthSquared,
                                        0.0, 1.0);
    const double X = static_cast<double>(AX) + DX * Parameter;
    const double Y = static_cast<double>(AY) + DY * Parameter;
    return DistanceSquared2(PX, PY, static_cast<float>(X), static_cast<float>(Y));
}

ParametricGizmoHandle ResolveGizmoHandle(const PointerCondition& Pointer,
                                         const GizmoScreenBasis& Basis,
                                         ParametricTransformMode Mode)
{
    const float AxisLength = 44.0f;
    const float XEndX = Basis.PivotX + Basis.XDirX * AxisLength;
    const float XEndY = Basis.PivotY + Basis.XDirY * AxisLength;
    const float ZEndX = Basis.PivotX + Basis.ZDirX * AxisLength;
    const float ZEndY = Basis.PivotY + Basis.ZDirY * AxisLength;

    const double CentreDistanceSquared = DistanceSquared2(Pointer.PositionX, Pointer.PositionY, Basis.PivotX, Basis.PivotY);
    if (Mode == ParametricTransformMode::Move)
    {
        const float PlaneOffset = 16.0f;
        const float PlaneHalf = 7.0f;
        const float PlaneCentreX = Basis.PivotX + (Basis.XDirX + Basis.ZDirX) * PlaneOffset;
        const float PlaneCentreY = Basis.PivotY + (Basis.XDirY + Basis.ZDirY) * PlaneOffset;
        const float LocalX = (Pointer.PositionX - PlaneCentreX) * Basis.XDirX + (Pointer.PositionY - PlaneCentreY) * Basis.XDirY;
        const float LocalZ = (Pointer.PositionX - PlaneCentreX) * Basis.ZDirX + (Pointer.PositionY - PlaneCentreY) * Basis.ZDirY;
        if (std::fabs(LocalX) <= PlaneHalf && std::fabs(LocalZ) <= PlaneHalf)
            return ParametricGizmoHandle::MoveFree;
        if (DistancePointSegmentSquared2(Pointer.PositionX, Pointer.PositionY,
                                         Basis.PivotX, Basis.PivotY, XEndX, XEndY) <= 8.0 * 8.0)
            return ParametricGizmoHandle::MoveX;
        if (DistancePointSegmentSquared2(Pointer.PositionX, Pointer.PositionY,
                                         Basis.PivotX, Basis.PivotY, ZEndX, ZEndY) <= 8.0 * 8.0)
            return ParametricGizmoHandle::MoveZ;
        if (CentreDistanceSquared <= 12.0 * 12.0)
            return ParametricGizmoHandle::MoveFree;
    }
    else if (Mode == ParametricTransformMode::Rotate)
    {
        const double Distance = std::sqrt(CentreDistanceSquared);
        if (Distance >= 28.0 && Distance <= 44.0)
            return ParametricGizmoHandle::Rotate;
        if (CentreDistanceSquared <= 12.0 * 12.0)
            return ParametricGizmoHandle::Rotate;
    }
    else
    {
        if (DistanceSquared2(Pointer.PositionX, Pointer.PositionY, XEndX, XEndY) <= 10.0 * 10.0)
            return ParametricGizmoHandle::ScaleX;
        if (DistanceSquared2(Pointer.PositionX, Pointer.PositionY, ZEndX, ZEndY) <= 10.0 * 10.0)
            return ParametricGizmoHandle::ScaleZ;
        if (CentreDistanceSquared <= 12.0 * 12.0)
            return ParametricGizmoHandle::ScaleFree;
    }

    return ParametricGizmoHandle::None;
}

void RecordViewportSelectionOverlay(OverlayGeometry& Overlay,
                                    const PlaneExtent& Extent,
                                    const SpatialBasis& Basis,
                                    const ParametricViewportState& View,
                                    bool Perspective,
                                    const SketchStructure& Sketch,
                                    const WorkspaceRecordStructure& Records,
                                    const ParametricViewportSelection& Hovered,
                                    const ParametricViewportSelection& Selected)
{
    const auto RecordCurve = [&](SketchCurveName Curve, std::uint32_t Packed, float Thickness)
    {
        if (!Curve.Assigned() || Curve.IssuedIndex > Sketch.Curves().size())
            return;
        std::vector<SpatialPoint> Polyline;
        AppendCurvePolyline(Sketch.Curves()[Curve.IssuedIndex - 1u].Geometry, Polyline, 48u);
        for (std::size_t Index = 0u; Index + 1u < Polyline.size(); ++Index)
        {
            float X0 = 0.0f, Y0 = 0.0f, X1 = 0.0f, Y1 = 0.0f;
            if (ProjectSpatialPoint(Basis, View, Perspective, Extent, Polyline[Index], X0, Y0) &&
                ProjectSpatialPoint(Basis, View, Perspective, Extent, Polyline[Index + 1u], X1, Y1))
                Overlay.AddLine(X0, Y0, X1, Y1, Packed, Thickness);
        }
    };

    if (Selected.Standing())
    {
        if (Selected.Subject == ParametricSelectionSubject::Curve)
            RecordCurve(Selected.Curve, OverlayPacked(0x5Bu, 0x8Cu, 0xFFu, 255u), 2.4f);
        else if (Selected.Subject == ParametricSelectionSubject::Record)
        {
            const WorkspaceRecord* Record = Records.Resolve(Selected.Record);
            if (Record != nullptr && Record->Profile.Assigned() && Record->Profile.IssuedIndex <= Sketch.Profiles().size())
                for (const ProfileLoop& Loop : Sketch.Profiles()[Record->Profile.IssuedIndex - 1u].HeldLoops())
                    for (const ProfileCurveUse& Use : Loop.Traversal)
                        RecordCurve({ Use.TraversedCurve.IssuedIndex }, OverlayPacked(0x5Bu, 0x8Cu, 0xFFu, 255u), 2.2f);
        }

    }

    const auto RecordPoint = [&](const ParametricViewportSelection& Subject, std::uint32_t Outer, std::uint32_t Inner)
    {
        float X = 0.0f;
        float Y = 0.0f;
        if (!ProjectSpatialPoint(Basis, View, Perspective, Extent, Subject.Position, X, Y))
            return;
        Overlay.AddDot(X, Y, Inner, 4.5f);
        AppendOverlayCircle(Overlay, X, Y, 8.0f, Outer, 1.6f);
    };

    if (Selected.Subject == ParametricSelectionSubject::Point || Selected.Subject == ParametricSelectionSubject::Control)
        RecordPoint(Selected, OverlayPacked(0xFFu, 0xFFu, 0xFFu, 224u), OverlayPacked(0x5Bu, 0x8Cu, 0xFFu, 255u));

    if (Hovered.Subject == ParametricSelectionSubject::Curve)
        RecordCurve(Hovered.Curve, OverlayPacked(0xFBu, 0xBFu, 0x24u, 208u), 1.8f);
    else if (Hovered.Standing())
        RecordPoint(Hovered, OverlayPacked(0xFBu, 0xBFu, 0x24u, 208u), OverlayPacked(0xFBu, 0xBFu, 0x24u, 180u));
}

struct GizmoScreenBasis
{
    float PivotX = 0.0f;
    float PivotY = 0.0f;
    float XDirX = 1.0f;
    float XDirY = 0.0f;
    float ZDirX = 0.0f;
    float ZDirY = -1.0f;
};

bool ResolveGizmoScreenBasis(const SpatialBasis& Basis,
                             const ParametricViewportState& View,
                             bool Perspective,
                             const PlaneExtent& Extent,
                             const SpatialPoint& Pivot,
                             GizmoScreenBasis& Resolved)
{
    if (!ProjectSpatialPoint(Basis, View, Perspective, Extent, Pivot, Resolved.PivotX, Resolved.PivotY))
        return false;

    float X0 = 0.0f, Y0 = 0.0f;
    if (ProjectSpatialPoint(Basis, View, Perspective, Extent, Added(Pivot, Scaled(Basis.Along, 24.0)), X0, Y0))
    {
        const float DX = X0 - Resolved.PivotX;
        const float DY = Y0 - Resolved.PivotY;
        const float Length = std::sqrt(DX * DX + DY * DY);
        if (Length > 1.0e-4f)
        {
            Resolved.XDirX = DX / Length;
            Resolved.XDirY = DY / Length;
        }
    }

    if (ProjectSpatialPoint(Basis, View, Perspective, Extent, Added(Pivot, Scaled(Basis.Across, 24.0)), X0, Y0))
    {
        const float DX = X0 - Resolved.PivotX;
        const float DY = Y0 - Resolved.PivotY;
        const float Length = std::sqrt(DX * DX + DY * DY);
        if (Length > 1.0e-4f)
        {
            Resolved.ZDirX = DX / Length;
            Resolved.ZDirY = DY / Length;
        }
    }

    return true;
}

void RecordViewportGizmo(OverlayGeometry& Overlay,
                         const PlaneExtent& Extent,
                         const SpatialBasis& Basis,
                         const ParametricViewportState& View,
                         bool Perspective,
                         const ParametricViewportSelection& Selected,
                         ParametricGizmoHandle HoveredHandle,
                         const ParametricTransformState& Transform)
{
    if (!Selected.Standing())
        return;

    GizmoScreenBasis Screen = {};
    if (!ResolveGizmoScreenBasis(Basis, View, Perspective, Extent, Selected.Position, Screen))
        return;

    const std::uint32_t XPacked = OverlayPacked(0xE0u, 0x14u, 0x14u, 255u);
    const std::uint32_t ZPacked = OverlayPacked(0x15u, 0x60u, 0xE0u, 255u);
    const std::uint32_t White = OverlayPacked(0xFFu, 0xFFu, 0xFFu, 255u);
    const std::uint32_t Highlight = OverlayPacked(0xFBu, 0xBFu, 0x24u, 255u);
    const std::uint32_t Guide = OverlayPacked(0xFFu, 0xFFu, 0xFFu, 160u);
    const float AxisLength = 44.0f;
    const float XEndX = Screen.PivotX + Screen.XDirX * AxisLength;
    const float XEndY = Screen.PivotY + Screen.XDirY * AxisLength;
    const float ZEndX = Screen.PivotX + Screen.ZDirX * AxisLength;
    const float ZEndY = Screen.PivotY + Screen.ZDirY * AxisLength;

    Overlay.AddLine(Screen.PivotX, Screen.PivotY, XEndX, XEndY,
                    HoveredHandle == ParametricGizmoHandle::MoveX || HoveredHandle == ParametricGizmoHandle::ScaleX ? Highlight : XPacked,
                    1.6f);
    Overlay.AddLine(Screen.PivotX, Screen.PivotY, ZEndX, ZEndY,
                    HoveredHandle == ParametricGizmoHandle::MoveZ || HoveredHandle == ParametricGizmoHandle::ScaleZ ? Highlight : ZPacked,
                    1.6f);

    if (Transform.Mode == ParametricTransformMode::Move)
    {
        const std::uint32_t PlanePacked = HoveredHandle == ParametricGizmoHandle::MoveFree
                                        ? OverlayPacked(0xFBu, 0xBFu, 0x24u, 96u)
                                        : OverlayPacked(0x1Fu, 0xC7u, 0xC7u, 70u);
        const float PlaneOffset = 16.0f;
        const float PlaneHalf = 7.0f;
        const float PlaneCentreX = Screen.PivotX + (Screen.XDirX + Screen.ZDirX) * PlaneOffset;
        const float PlaneCentreY = Screen.PivotY + (Screen.XDirY + Screen.ZDirY) * PlaneOffset;
        const float X0 = PlaneCentreX - Screen.XDirX * PlaneHalf - Screen.ZDirX * PlaneHalf;
        const float Y0 = PlaneCentreY - Screen.XDirY * PlaneHalf - Screen.ZDirY * PlaneHalf;
        const float X1 = PlaneCentreX + Screen.XDirX * PlaneHalf - Screen.ZDirX * PlaneHalf;
        const float Y1 = PlaneCentreY + Screen.XDirY * PlaneHalf - Screen.ZDirY * PlaneHalf;
        const float X2 = PlaneCentreX + Screen.XDirX * PlaneHalf + Screen.ZDirX * PlaneHalf;
        const float Y2 = PlaneCentreY + Screen.XDirY * PlaneHalf + Screen.ZDirY * PlaneHalf;
        const float X3 = PlaneCentreX - Screen.XDirX * PlaneHalf + Screen.ZDirX * PlaneHalf;
        const float Y3 = PlaneCentreY - Screen.XDirY * PlaneHalf + Screen.ZDirY * PlaneHalf;
        Overlay.AddTriangle(X0, Y0, X1, Y1, X2, Y2, PlanePacked);
        Overlay.AddTriangle(X0, Y0, X2, Y2, X3, Y3, PlanePacked);
        Overlay.AddLine(X0, Y0, X1, Y1, HoveredHandle == ParametricGizmoHandle::MoveFree ? Highlight : White, 1.2f);
        Overlay.AddLine(X1, Y1, X2, Y2, HoveredHandle == ParametricGizmoHandle::MoveFree ? Highlight : White, 1.2f);
        Overlay.AddLine(X2, Y2, X3, Y3, HoveredHandle == ParametricGizmoHandle::MoveFree ? Highlight : White, 1.2f);
        Overlay.AddLine(X3, Y3, X0, Y0, HoveredHandle == ParametricGizmoHandle::MoveFree ? Highlight : White, 1.2f);

        AppendOverlayArrow(Overlay, Screen.PivotX, Screen.PivotY, XEndX, XEndY,
                           HoveredHandle == ParametricGizmoHandle::MoveX ? Highlight : XPacked, 2.2f);
        AppendOverlayArrow(Overlay, Screen.PivotX, Screen.PivotY, ZEndX, ZEndY,
                           HoveredHandle == ParametricGizmoHandle::MoveZ ? Highlight : ZPacked, 2.2f);
        AppendOverlayCircle(Overlay, Screen.PivotX, Screen.PivotY, 10.0f,
                            HoveredHandle == ParametricGizmoHandle::MoveFree ? Highlight : White, 1.8f);
    }
    else if (Transform.Mode == ParametricTransformMode::Rotate)
    {
        const float XAngle = std::atan2(Screen.XDirY, Screen.XDirX);
        const float ZAngle = std::atan2(Screen.ZDirY, Screen.ZDirX);
        AppendOverlayArc(Overlay, Screen.PivotX, Screen.PivotY, 34.0f, XAngle - 0.55f, 1.10f,
                         HoveredHandle == ParametricGizmoHandle::Rotate ? Highlight : XPacked, 2.0f, 16u);
        AppendOverlayArc(Overlay, Screen.PivotX, Screen.PivotY, 40.0f, ZAngle - 0.55f, 1.10f,
                         HoveredHandle == ParametricGizmoHandle::Rotate ? Highlight : ZPacked, 1.8f, 16u);
        AppendOverlayCircle(Overlay, Screen.PivotX, Screen.PivotY, 10.0f, White, 1.8f);
    }
    else
    {
        Overlay.AddLine(Screen.PivotX, Screen.PivotY, XEndX, XEndY,
                        HoveredHandle == ParametricGizmoHandle::ScaleX ? Highlight : XPacked, 2.1f);
        Overlay.AddLine(Screen.PivotX, Screen.PivotY, ZEndX, ZEndY,
                        HoveredHandle == ParametricGizmoHandle::ScaleZ ? Highlight : ZPacked, 2.1f);
        AppendOverlaySquare(Overlay, XEndX, XEndY, 4.5f,
                            HoveredHandle == ParametricGizmoHandle::ScaleX ? Highlight : XPacked, 1.8f);
        AppendOverlaySquare(Overlay, ZEndX, ZEndY, 4.5f,
                            HoveredHandle == ParametricGizmoHandle::ScaleZ ? Highlight : ZPacked, 1.8f);
        AppendOverlayCircle(Overlay, Screen.PivotX, Screen.PivotY, 10.0f,
                            HoveredHandle == ParametricGizmoHandle::ScaleFree ? Highlight : White, 1.8f);
    }

    if (Transform.Engaged)
    {
        float GuideX = 0.0f;
        float GuideY = 0.0f;
        bool Guided = false;
        if (Transform.Constraint == ParametricTransformConstraint::AxisX)
        {
            GuideX = Screen.XDirX;
            GuideY = Screen.XDirY;
            Guided = true;
        }
        else if (Transform.Constraint == ParametricTransformConstraint::AxisZ)
        {
            GuideX = Screen.ZDirX;
            GuideY = Screen.ZDirY;
            Guided = true;
        }
        else if (Transform.Constraint == ParametricTransformConstraint::Curve)
        {
            GuideX = static_cast<float>(Dot(Transform.CurveDirection, Basis.Along) * Screen.XDirX
                                      + Dot(Transform.CurveDirection, Basis.Across) * Screen.ZDirX);
            GuideY = static_cast<float>(Dot(Transform.CurveDirection, Basis.Along) * Screen.XDirY
                                      + Dot(Transform.CurveDirection, Basis.Across) * Screen.ZDirY);
            const float Length = std::sqrt(GuideX * GuideX + GuideY * GuideY);
            if (Length > 1.0e-4f)
            {
                GuideX /= Length;
                GuideY /= Length;
                Guided = true;
            }
        }

        if (Guided)
        {
            const float Span = std::max(Extent.Width(), Extent.Height());
            Overlay.AddLine(Screen.PivotX - GuideX * Span, Screen.PivotY - GuideY * Span,
                            Screen.PivotX + GuideX * Span, Screen.PivotY + GuideY * Span,
                            Guide, 1.5f);
        }
    }
}

void RecordViewportTransformReadout(RecordingSurface& Surface,
                                    const PlaneExtent& Extent,
                                    const ParametricTransformState& Transform)
{
    if (!Transform.Engaged)
        return;

    char Detail[160] = {};
    char Command[64] = {};
    FormatTransformCommand(Transform, Command, sizeof(Command));

    if (Transform.Mode == ParametricTransformMode::Rotate)
        std::snprintf(Detail, sizeof(Detail), "%s • %.1f° • %s",
                      Command,
                      Transform.PreviewValue,
                      Transform.SlideAlongCurve ? "curve slide" : TransformModeText(Transform.Mode));
    else if (Transform.Mode == ParametricTransformMode::Scale)
        std::snprintf(Detail, sizeof(Detail), "%s • %.3fx • %s",
                      Command,
                      Transform.PreviewValue,
                      TransformModeText(Transform.Mode));
    else
        std::snprintf(Detail, sizeof(Detail), "%s • %.2f • %s",
                      Command,
                      Transform.PreviewValue,
                      Transform.SlideAlongCurve ? "curve slide" : TransformModeText(Transform.Mode));

    const float Width = Surface.MeasureRun(Detail, 11.0f, 0.0f);
    Surface.TextRun(Extent.MinimumX + (Extent.Width() - Width) * 0.5f,
                    Extent.MinimumY + 42.0f,
                    Covering(0xE5E7EBu),
                    Detail, 11.0f, 0.0f, true);
}

void DriveDrawingWithModifiers(const PlaneExtent& Extent,
                               const PointerCondition& Pointer,
                               const ModifierCondition& Modifiers,
                               const SpatialBasis& Basis,
                               const ParametricViewportState& View,
                               bool Perspective,
                               ParametricToolSubject Tool,
                               WorkspaceNameIndex& Naming,
                               SketchStructure& Sketch,
                               WorkspaceRecordStructure& Records,
                               WorkspaceRevisionSequence& Revisions,
                               WorkspaceRecordName& PendingSelection,
                               ParametricDraftState& Draft,
                               bool& PointerTaken)
{
    const ParametricDraftSubject Desired = ResolveDraftSubject(Tool);
    if (Desired == ParametricDraftSubject::None)
    {
        if (Draft.Subject != ParametricDraftSubject::None)
            CancelDraft(Draft);
        return;
    }

    if (!Extent.Encloses(Pointer.PositionX, Pointer.PositionY))
        return;

    if (Draft.Subject != Desired)
        CancelDraft(Draft);
    Draft.Subject = Desired;

    SpatialPoint Raw = {};
    if (!ResolveViewportPlaneIntersection(Basis, View, Perspective, Extent,
                                          Pointer.PositionX, Pointer.PositionY, Raw))
        return;

    Draft.HoverStanding = true;
    Draft.Hover = Raw;
    Draft.Snap = {};
    if (Modifiers.Commanded)
    {
        Draft.Snap = ResolveNearestSnap(Sketch, Raw, ResolveSnapTolerance(View, Perspective));
        if (Draft.Snap.Resolved())
            Draft.Hover = Draft.Snap.Position;
    }

    if (Pointer.ContactPressed)
    {
        PointerTaken = true;
        if (Draft.Subject == ParametricDraftSubject::Line)
        {
            Draft.Anchors.push_back(Draft.Hover);
            if (Draft.Anchors.size() >= 2u)
            {
                const Outcome<WorkspaceRecordName> Record = CommitDraft(Naming, Sketch, Records, Revisions, Draft);
                if (Record.Resolved)
                    PendingSelection = Record.Resolve();
                CancelDraft(Draft);
            }
        }
        else if (Draft.Subject == ParametricDraftSubject::Rectangle || Draft.Subject == ParametricDraftSubject::Circle)
        {
            if (Draft.Anchors.empty())
                Draft.Anchors.push_back(Draft.Hover);
            else
            {
                const Outcome<WorkspaceRecordName> Record = CommitDraft(Naming, Sketch, Records, Revisions, Draft);
                if (Record.Resolved)
                    PendingSelection = Record.Resolve();
                CancelDraft(Draft);
            }
        }
    }
}

void DriveViewportSelectionAndTransform(const PlaneExtent& Extent,
                                        const PointerCondition& Pointer,
                                        const TextInputCondition& TextInput,
                                        const ModifierCondition& Modifiers,
                                        const SpatialBasis& Basis,
                                        const ParametricViewportState& View,
                                        bool Perspective,
                                        ParametricToolSubject ActiveTool,
                                        const WorkspaceDirectoryProjection& Directory,
                                        const ParametricWorkspaceContext& WorkspaceApplied,
                                        SketchStructure& Sketch,
                                        WorkspaceRecordStructure& Records,
                                        WorkspaceRevisionSequence& Revisions,
                                        WorkspaceRecordName& PendingSelection,
                                        ParametricViewportSelection& SemanticSelection,
                                        ParametricViewportSelection& HoveredSelection,
                                        ParametricTransformState& Transform,
                                        OverlayGeometry& Overlay,
                                        bool& PointerTaken,
                                        double SessionMilliseconds,
                                        double& LastGPressedMilliseconds)
{
    const WorkspaceRecordName SelectedRecord = ResolveSelectedRecord(Directory, WorkspaceApplied);
    if (SemanticSelection.Standing() && SelectedRecord.Assigned() &&
        SemanticSelection.Record.IssuedIndex != SelectedRecord.IssuedIndex &&
        (!PendingSelection.Assigned() || PendingSelection.IssuedIndex != SemanticSelection.Record.IssuedIndex))
        SemanticSelection = {};

    HoveredSelection = {};
    SpatialPoint Probe = {};
    const bool Probed = ResolveViewportPlaneIntersection(Basis, View, Perspective, Extent,
                                                         Pointer.PositionX, Pointer.PositionY, Probe);
    if (Probed)
        HoveredSelection = ResolveViewportPick(Sketch, Records, Probe, ResolveSnapTolerance(View, Perspective));

    const ParametricViewportSelection ActiveSelection =
        ResolveEditableSelection(Sketch, Records, SelectedRecord, PendingSelection, SemanticSelection);

    ParametricGizmoHandle HoveredHandle = ParametricGizmoHandle::None;
    if (!Transform.Engaged && ActiveSelection.Standing() && ResolveDraftSubject(ActiveTool) == ParametricDraftSubject::None)
    {
        GizmoScreenBasis Screen = {};
        if (ResolveGizmoScreenBasis(Basis, View, Perspective, Extent, ActiveSelection.Position, Screen))
            HoveredHandle = ResolveGizmoHandle(Pointer, Screen, Transform.Mode);
    }

    if (!Transform.Engaged && ResolveDraftSubject(ActiveTool) == ParametricDraftSubject::None &&
        Pointer.ContactPressed && HoveredHandle == ParametricGizmoHandle::None && HoveredSelection.Standing())
    {
        SemanticSelection = HoveredSelection;
        PendingSelection = HoveredSelection.Record;
        PointerTaken = true;
    }

    const ParametricTransformCommandInput Command =
        ResolveTransformCommandInput(TextInput, Transform.Engaged, Transform.Mode);

    if (!Transform.Engaged && ActiveSelection.Standing() && Extent.Encloses(Pointer.PositionX, Pointer.PositionY))
    {
        if (HoveredHandle != ParametricGizmoHandle::None && Pointer.ContactPressed)
        {
            ParametricTransformMode Mode = ParametricTransformMode::Move;
            ParametricTransformConstraint Constraint = ParametricTransformConstraint::Free;
            bool Slide = false;
            switch (HoveredHandle)
            {
                case ParametricGizmoHandle::MoveX:
                    Mode = ParametricTransformMode::Move;
                    Constraint = ParametricTransformConstraint::AxisX;
                    break;
                case ParametricGizmoHandle::MoveZ:
                    Mode = ParametricTransformMode::Move;
                    Constraint = ParametricTransformConstraint::AxisZ;
                    break;
                case ParametricGizmoHandle::MoveFree:
                    Mode = ParametricTransformMode::Move;
                    Constraint = ParametricTransformConstraint::Free;
                    break;
                case ParametricGizmoHandle::Rotate:
                    Mode = ParametricTransformMode::Rotate;
                    Constraint = ParametricTransformConstraint::Screen;
                    break;
                case ParametricGizmoHandle::ScaleX:
                    Mode = ParametricTransformMode::Scale;
                    Constraint = ParametricTransformConstraint::AxisX;
                    break;
                case ParametricGizmoHandle::ScaleZ:
                    Mode = ParametricTransformMode::Scale;
                    Constraint = ParametricTransformConstraint::AxisZ;
                    break;
                case ParametricGizmoHandle::ScaleFree:
                    Mode = ParametricTransformMode::Scale;
                    Constraint = ParametricTransformConstraint::Screen;
                    break;
                case ParametricGizmoHandle::None:
                    break;
            }

            PointerTaken = StartTransformSession(Extent, Pointer, Basis, View, Perspective,
                                                 Sketch, Records, ActiveSelection,
                                                 Mode, Constraint, Slide, true, Transform);
        }
        else if (Command.StartRequested)
        {
            const bool Slide = Command.StartMode == ParametricTransformMode::Move
                            && ResolveMoveSlideRequested(Command.MoveTapCount,
                                                         SessionMilliseconds,
                                                         LastGPressedMilliseconds,
                                                         ActiveSelection.Curve.Assigned());
            if (Command.MoveTapCount > 0u)
                LastGPressedMilliseconds = SessionMilliseconds;
            PointerTaken = StartTransformSession(Extent, Pointer, Basis, View, Perspective,
                                                 Sketch, Records, ActiveSelection,
                                                 Command.StartMode,
                                                 Slide ? ParametricTransformConstraint::Curve
                                                       : (Command.StartMode == ParametricTransformMode::Rotate
                                                            ? ParametricTransformConstraint::Screen
                                                            : ParametricTransformConstraint::Free),
                                                 Slide, false, Transform);
        }
    }

    if (Transform.Engaged)
    {
        const bool SlideRequested = Transform.Mode == ParametricTransformMode::Move
                                 && ResolveMoveSlideRequested(Command.MoveTapCount,
                                                              SessionMilliseconds,
                                                              LastGPressedMilliseconds,
                                                              Transform.Target.Curve.Assigned());
        if (Transform.Mode == ParametricTransformMode::Move && Command.MoveTapCount > 0u)
            LastGPressedMilliseconds = SessionMilliseconds;

        if (SlideRequested)
        {
            Transform.Constraint = ParametricTransformConstraint::Curve;
            Transform.SlideAlongCurve = true;
        }
        else if (Command.ConstraintRequested)
        {
            Transform.Constraint = Command.Constraint;
            Transform.SlideAlongCurve = false;
        }

        if (Command.NumericAppend[0] != '\0')
            AppendTransformNumericRun(Transform.Numeric, sizeof(Transform.Numeric), Command.NumericAppend);
        if (TextInput.BackspacePressed)
            RetractTransformCommand(Transform);
        if (TextInput.DeletePressed)
            ClearTransformNumeric(Transform);

        if (TextInput.CancelPressed)
        {
            CancelTransformSession(Sketch, Transform);
            PointerTaken = true;
        }
        else
        {
            UpdateTransformSession(Extent, Pointer, Modifiers, Basis, View, Perspective, Sketch, Transform);
            PointerTaken = true;

            if (Transform.AwaitingRelease)
            {
                if (Pointer.ContactReleased)
                    CommitTransformSession(Records, Revisions, Transform);
            }
            else if (TextInput.AcceptPressed || Pointer.ContactPressed)
            {
                CommitTransformSession(Records, Revisions, Transform);
            }
        }
    }

    RecordViewportSelectionOverlay(Overlay, Extent, Basis, View, Perspective,
                                   Sketch, Records, HoveredSelection, ActiveSelection);
    RecordViewportGizmo(Overlay, Extent, Basis, View, Perspective,
                        ActiveSelection, HoveredHandle, Transform);
}

} // namespace

void ClearInspectorBridge(ParametricWorkspaceBridgeStorage& Bridge)
{
    Bridge.Property = ParametricPropertyPresentation{};
    Bridge.PropertyNaming.clear();
    Bridge.PropertySecondary.clear();
    for (std::string& Run : Bridge.PropertyCaptions) Run.clear();
    for (std::string& Run : Bridge.PropertyValues)   Run.clear();
    for (std::string& Run : Bridge.PropertyTrails)   Run.clear();
    Bridge.RevisionRows.clear();
    Bridge.RevisionBacking.clear();
}

void SeedParametricWorkspace(WorkspaceNameIndex& Naming,
                             SketchStructure& Sketch,
                             WorkspaceRecordStructure& Records,
                             WorkspaceRevisionSequence& Revisions)
{
    if (Records.DeclaredCount() != 0u)
        return;

    Sketch.DeclarePlane({ { 0.0, 0.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 1.0, 0.0, 0.0 } });

    const SketchCurveName CurveName = Sketch.DeclareLine({ -120.0, 0.0, -40.0 },
                                                         {  120.0, 0.0, -40.0 });
    const SketchPointName PointName = { (CurveName.IssuedIndex << 8u) | 1u };

    const Outcome<ProfileNameInFeature> ProfileOutcome =
        Sketch.DeclareRegularPolygon({ 0.0, 0.0, 60.0 }, 70.0, 5u);
    const ProfileNameInFeature DeclaredProfile = ProfileOutcome.Resolved ? ProfileOutcome.Resolve() : ProfileNameInFeature{};

    ReferenceSpecification PointReference = {};
    PointReference.Subject = ReferenceSubject::SketchPoint;
    PointReference.SketchPoint = { (CurveName.IssuedIndex << 8u) | 1u };

    ReferenceSpecification PointReference2 = {};
    PointReference2.Subject = ReferenceSubject::SketchPoint;
    PointReference2.SketchPoint = { (CurveName.IssuedIndex << 8u) | 2u };

    ReferenceSpecification CurveReference = {};
    CurveReference.Subject = ReferenceSubject::SketchCurve;
    CurveReference.SketchCurve = CurveName;

    DimensionSpecification DeclaredDimension = {};
    DeclaredDimension.Subject = DimensionSubject::Horizontal;
    DeclaredDimension.Primary = PointReference;
    DeclaredDimension.Secondary = PointReference2;
    DeclaredDimension.Target = 240.0;
    const DimensionName DeclaredDimensionName = Sketch.DeclareDimension(DeclaredDimension);

    ConstraintSpecification DeclaredConstraint = {};
    DeclaredConstraint.Subject = ConstraintSubject::Horizontal;
    DeclaredConstraint.Primary = CurveReference;
    const ConstraintName DeclaredConstraintName = Sketch.DeclareConstraint(DeclaredConstraint);

    const WorkspaceRecordName FolderSketch = Records.Declare(
        WorkspaceRecord{ WorkspaceRecordSubject::Folder, {}, WorkspaceCategory::Sketch,
                         Naming.Issue(WorkspaceRecordSubject::Folder) });
    Discard(Records.SetFolderCategory(FolderSketch, WorkspaceCategory::Sketch));

    const WorkspaceRecordName FolderGeometry = Records.Declare(
        WorkspaceRecord{ WorkspaceRecordSubject::Folder, {}, WorkspaceCategory::Sketch,
                         Naming.Issue(WorkspaceRecordSubject::Folder) });
    Discard(Records.SetFolderCategory(FolderGeometry, WorkspaceCategory::Geometry));

    const WorkspaceRecordName FolderAnnotation = Records.Declare(
        WorkspaceRecord{ WorkspaceRecordSubject::Folder, {}, WorkspaceCategory::Sketch,
                         Naming.Issue(WorkspaceRecordSubject::Folder) });
    Discard(Records.SetFolderCategory(FolderAnnotation, WorkspaceCategory::Annotation));

    const WorkspaceRecordName FolderOperation = Records.Declare(
        WorkspaceRecord{ WorkspaceRecordSubject::Folder, {}, WorkspaceCategory::Sketch,
                         Naming.Issue(WorkspaceRecordSubject::Folder) });
    Discard(Records.SetFolderCategory(FolderOperation, WorkspaceCategory::Operation));

    WorkspaceRecord PointRecord = {};
    PointRecord.Subject = WorkspaceRecordSubject::Point;
    PointRecord.ParentFolder = FolderSketch;
    PointRecord.Naming = Naming.Issue(WorkspaceRecordSubject::Point);
    PointRecord.SketchPoint = PointName;
    const WorkspaceRecordName Point = Records.Declare(PointRecord);

    WorkspaceRecord CurveRecord = {};
    CurveRecord.Subject = WorkspaceRecordSubject::OpenCurve;
    CurveRecord.ParentFolder = FolderSketch;
    CurveRecord.Naming = Naming.Issue(WorkspaceRecordSubject::OpenCurve);
    CurveRecord.SketchCurve = CurveName;
    const WorkspaceRecordName Curve = Records.Declare(CurveRecord);

    WorkspaceRecord ProfileRecord = {};
    ProfileRecord.Subject = WorkspaceRecordSubject::ClosedProfile;
    ProfileRecord.ParentFolder = FolderSketch;
    ProfileRecord.Naming = Naming.Issue(WorkspaceRecordSubject::ClosedProfile);
    ProfileRecord.ClosedSemantic = true;
    ProfileRecord.Profile = DeclaredProfile;
    const WorkspaceRecordName Profile = Records.Declare(ProfileRecord);

    WorkspaceRecord SurfaceRecord = {};
    SurfaceRecord.Subject = WorkspaceRecordSubject::ThinSurface;
    SurfaceRecord.ParentFolder = FolderGeometry;
    SurfaceRecord.Naming = Naming.Issue(WorkspaceRecordSubject::ThinSurface);
    SurfaceRecord.Profile = DeclaredProfile;
    const WorkspaceRecordName Surface = Records.Declare(SurfaceRecord);

    WorkspaceRecord SolidRecord = {};
    SolidRecord.Subject = WorkspaceRecordSubject::Solid;
    SolidRecord.ParentFolder = FolderGeometry;
    SolidRecord.Naming = Naming.Issue(WorkspaceRecordSubject::Solid);
    SolidRecord.ClosedSemantic = true;
    SolidRecord.Profile = DeclaredProfile;
    SolidRecord.Solid = { 1u };
    const WorkspaceRecordName Solid = Records.Declare(SolidRecord);

    WorkspaceRecord DimensionRecord = {};
    DimensionRecord.Subject = WorkspaceRecordSubject::Dimension;
    DimensionRecord.ParentFolder = FolderAnnotation;
    DimensionRecord.Naming = Naming.Issue(WorkspaceRecordSubject::Dimension);
    DimensionRecord.Dimension = DeclaredDimensionName;
    const WorkspaceRecordName Dimension = Records.Declare(DimensionRecord);

    WorkspaceRecord ConstraintRecord = {};
    ConstraintRecord.Subject = WorkspaceRecordSubject::Constraint;
    ConstraintRecord.ParentFolder = FolderAnnotation;
    ConstraintRecord.Naming = Naming.Issue(WorkspaceRecordSubject::Constraint);
    ConstraintRecord.Constraint = DeclaredConstraintName;
    const WorkspaceRecordName Constraint = Records.Declare(ConstraintRecord);

    WorkspaceRecord PatternRecord = {};
    PatternRecord.Subject = WorkspaceRecordSubject::Pattern;
    PatternRecord.ParentFolder = FolderOperation;
    PatternRecord.Naming = Naming.Issue(WorkspaceRecordSubject::Pattern);
    PatternRecord.Feature = { 1u };
    const WorkspaceRecordName Pattern = Records.Declare(PatternRecord);

    WorkspaceRecord MirrorRecord = {};
    MirrorRecord.Subject = WorkspaceRecordSubject::Mirror;
    MirrorRecord.ParentFolder = FolderOperation;
    MirrorRecord.Naming = Naming.Issue(WorkspaceRecordSubject::Mirror);
    MirrorRecord.Feature = { 2u };
    const WorkspaceRecordName Mirror = Records.Declare(MirrorRecord);

    Revisions.Seal("Declared Point_1", "Create Point", { Point }, 1u);
    Revisions.Seal("Declared Line_1", "Create Curve", { Curve }, 2u);
    Revisions.Seal("Closed Profile_1", "Resolve Profile", { Curve, Profile }, 3u);
    Revisions.Seal("Raised ThinSurface_1", "Extrude Surface", { Profile, Surface }, 4u);
    Revisions.Seal("Raised Solid_1", "Extrude Solid", { Profile, Solid }, 5u);
    Revisions.Seal("Added Dimension_1", "Measure", { Dimension, Profile }, 6u);
    Revisions.Seal("Added Constraint_1", "Constrain", { Constraint, Curve, Point }, 7u);
    Revisions.Seal("Patterned Solid_1", "Pattern", { Pattern, Solid }, 8u);
    Revisions.Seal("Mirrored Solid_1", "Mirror", { Mirror, Solid }, 9u);
}

void ConstructParametricLayout(PanelStructure& Partition)
{
    Partition.ConstructPanelPartition(PanelSubject::Viewport);

    if (!Partition.Divide(PanelStructure::RootIndex, PanelDivisionAxis::X,
                          PanelDivisionSide::Minimum).Resolved)
        return;

    const Outcome<PanelRecord> Root = Partition.Current(PanelStructure::RootIndex);
    if (!Root.Resolved)
        return;

    Discard(Partition.Assign(Root.Resolve().Minimum, PanelSubject::Outliner));

    const std::uint32_t Right = Root.Resolve().Maximum;
    if (!Partition.Divide(Right, PanelDivisionAxis::X, PanelDivisionSide::Minimum).Resolved)
        return;

    const Outcome<PanelRecord> RightBranch = Partition.Current(Right);
    if (!RightBranch.Resolved)
        return;

    Discard(Partition.Assign(RightBranch.Resolve().Minimum, PanelSubject::ParametricTools));
    Discard(Partition.Assign(RightBranch.Resolve().Maximum, PanelSubject::Viewport));
    Discard(Partition.Proportion(PanelStructure::RootIndex, 0.27f));
    Discard(Partition.Proportion(Right, 0.33f));
}

bool AnySelectedRow(const ParametricWorkspaceContext& Applied, std::uint32_t RowCount)
{
    for (std::uint32_t Index = 0u; Index < RowCount; ++Index)
        if (Applied.RowSelected[Index])
            return true;
    return false;
}

std::uint32_t ResolveInitialRow(const WorkspaceDirectoryProjection& Directory)
{
    for (std::uint32_t Index = 0u; Index < Directory.Rows.size(); ++Index)
        if (Directory.Rows[Index].Kind == WorkspaceDirectoryRowKind::Record &&
            Directory.Rows[Index].Subject != WorkspaceRecordSubject::Folder)
            return Index;

    for (std::uint32_t Index = 0u; Index < Directory.Rows.size(); ++Index)
        if (Directory.Rows[Index].Kind == WorkspaceDirectoryRowKind::Record)
            return Index;

    return 0u;
}

void SeatParametricContext(const WorkspaceDirectoryProjection& Directory,
                           ParametricWorkspaceContext& Applied,
                           bool& Seeded)
{
    const std::uint32_t RowCount = std::min<std::uint32_t>(
        static_cast<std::uint32_t>(Directory.Rows.size()), ParametricWorkspaceContext::RowLimit);

    for (std::uint32_t Index = RowCount; Index < ParametricWorkspaceContext::RowLimit; ++Index)
    {
        Applied.RowExpanded[Index] = false;
        Applied.RowSelected[Index] = false;
    }

    if (RowCount == 0u)
    {
        Applied.RowTaken = 0u;
        Applied.RowSelectionAnchor = 0u;
        return;
    }

    if (!Seeded)
    {
        for (std::uint32_t Index = 0u; Index < ParametricWorkspaceContext::RowLimit; ++Index)
            Applied.RowSelected[Index] = false;

        for (std::uint32_t Index = 0u; Index < RowCount; ++Index)
            if (Directory.Rows[Index].Subject == WorkspaceRecordSubject::Folder)
                Applied.RowExpanded[Index] = true;

        const std::uint32_t Initial = ResolveInitialRow(Directory);
        Applied.RowTaken = Initial;
        Applied.RowSelectionAnchor = Initial;
        Applied.RowSelected[Initial] = true;
        Seeded = true;
        return;
    }

    if (Applied.RowTaken >= RowCount || !AnySelectedRow(Applied, RowCount))
    {
        for (std::uint32_t Index = 0u; Index < RowCount; ++Index)
            Applied.RowSelected[Index] = false;

        const std::uint32_t Initial = ResolveInitialRow(Directory);
        Applied.RowTaken = Initial;
        Applied.RowSelectionAnchor = Initial;
        Applied.RowSelected[Initial] = true;
    }
}

Outcome<bool> SynchroniseParametricPresentation(const WorkspaceRecordStructure& Records,
                                                const WorkspaceRevisionSequence& Revisions,
                                                WorkspaceDirectoryProjection& Directory,
                                                ParametricWorkspaceBridgeStorage& Bridge,
                                                ParametricWorkspaceContext& Applied,
                                                WorkspaceRecordName& PendingSelection,
                                                bool& Seeded)
{
    ProjectWorkspaceDirectory(Records, Directory);

    const Outcome<bool> DirectoryBridge = BridgeParametricDirectory(Directory, Bridge);
    if (!DirectoryBridge.Resolved)
        return DirectoryBridge;

    SeatParametricContext(Directory, Applied, Seeded);

    if (PendingSelection.Assigned())
    {
        const Outcome<std::uint32_t> Row = ResolveWorkspaceDirectoryRow(Directory, PendingSelection);
        if (Row.Resolved && Row.Resolve() < ParametricWorkspaceContext::RowLimit)
        {
            for (std::uint32_t Index = 0u; Index < ParametricWorkspaceContext::RowLimit; ++Index)
                Applied.RowSelected[Index] = false;
            Applied.RowTaken = Row.Resolve();
            Applied.RowSelectionAnchor = Row.Resolve();
            Applied.RowSelected[Row.Resolve()] = true;
        }
        PendingSelection = {};
    }

    const std::uint32_t RowCount = static_cast<std::uint32_t>(Directory.Rows.size());
    if (Applied.RowTaken >= RowCount || Directory.Rows.empty() ||
        Directory.Rows[Applied.RowTaken].Kind != WorkspaceDirectoryRowKind::Record)
    {
        ClearInspectorBridge(Bridge);
        return Outcome<bool>::Result(true);
    }

    const WorkspaceRecordName Selected = Directory.Rows[Applied.RowTaken].Record;
    const Outcome<WorkspacePropertyProjection> Property =
        ProjectWorkspaceProperty(Records, Revisions, Selected);
    if (!Property.Resolved)
        return Outcome<bool>::Refuse(Property.Error);

    return BridgeParametricInspector(Records, Selected, Property.Resolve(), Revisions, Bridge);
}

Outcome<bool> SynchroniseCadPacket(const SketchStructure& Sketch,
                                   const WorkspaceRecordStructure& Records,
                                   WorkspaceCadPacket& Delivered)
{
    return ProjectSketchRendering(Sketch, Records, Delivered, {});
}

void SynchroniseToolContext(const WorkspaceDirectoryProjection& Directory,
                            const WorkspaceRecordStructure& Records,
                            const SketchStructure& Sketch,
                            const ParametricWorkspaceContext& WorkspaceApplied,
                            ParametricToolsContext& ToolsApplied)
{
    ToolsApplied.WorkplaneActivation = Sketch.Declared();
    ToolsApplied.ReferencePlaneCondition = Sketch.Declared();
    ToolsApplied.PlanarProfileCondition = true;
    ToolsApplied.UniformClosureCondition = true;
    ToolsApplied.PendingGeometryCondition = false;
    ToolsApplied.SourceImageryCondition = false;

    ToolsApplied.SelectedCount = 0u;
    for (std::uint32_t Index = 0u; Index < ParametricWorkspaceContext::RowLimit; ++Index)
        if (WorkspaceApplied.RowSelected[Index])
            ++ToolsApplied.SelectedCount;

    ToolsApplied.ProfileCount = 0u;
    ToolsApplied.PerimeterEdgeCount = 0u;
    ToolsApplied.ExistingCircleCount = 0u;
    ToolsApplied.SolidCount = 0u;
    ToolsApplied.AxisAvailability = false;
    ToolsApplied.PathAvailability = false;
    ToolsApplied.SupportMaterialCondition = false;
    ToolsApplied.TangentEndpointCondition = false;
    ToolsApplied.OpeningCondition = false;
    ToolsApplied.MeasurableCondition = false;
    ToolsApplied.ClosedProfileCondition = false;
    ToolsApplied.ActiveDimension = ParametricToolDimension::Nothing;

    if (WorkspaceApplied.RowTaken >= Directory.Rows.size())
        return;

    const WorkspaceDirectoryRow& Row = Directory.Rows[WorkspaceApplied.RowTaken];
    if (Row.Kind != WorkspaceDirectoryRowKind::Record)
        return;

    const WorkspaceRecord* Record = Records.Resolve(Row.Record);
    if (Record == nullptr)
        return;

    switch (Record->Subject)
    {
        case WorkspaceRecordSubject::Point:
            ToolsApplied.ActiveDimension = ParametricToolDimension::Vertex;
            ToolsApplied.MeasurableCondition = true;
            break;
        case WorkspaceRecordSubject::OpenCurve:
            ToolsApplied.ActiveDimension = ParametricToolDimension::Edge;
            ToolsApplied.PathAvailability = true;
            ToolsApplied.AxisAvailability = true;
            ToolsApplied.MeasurableCondition = true;
            ToolsApplied.TangentEndpointCondition = true;
            ToolsApplied.PerimeterEdgeCount = 1u;
            break;
        case WorkspaceRecordSubject::ClosedProfile:
            ToolsApplied.ActiveDimension = ParametricToolDimension::Wire;
            ToolsApplied.ProfileCount = 1u;
            ToolsApplied.AxisAvailability = true;
            ToolsApplied.PathAvailability = true;
            ToolsApplied.ClosedProfileCondition = true;
            ToolsApplied.MeasurableCondition = true;
            ToolsApplied.PerimeterEdgeCount = 5u;
            break;
        case WorkspaceRecordSubject::ThinSurface:
            ToolsApplied.ActiveDimension = ParametricToolDimension::Shell;
            ToolsApplied.ProfileCount = 1u;
            ToolsApplied.SupportMaterialCondition = true;
            ToolsApplied.OpeningCondition = true;
            ToolsApplied.MeasurableCondition = true;
            break;
        case WorkspaceRecordSubject::Solid:
            ToolsApplied.ActiveDimension = ParametricToolDimension::Solid;
            ToolsApplied.SolidCount = 1u;
            ToolsApplied.SupportMaterialCondition = true;
            ToolsApplied.ReferencePlaneCondition = true;
            ToolsApplied.AxisAvailability = true;
            ToolsApplied.MeasurableCondition = true;
            ToolsApplied.ClosedProfileCondition = true;
            break;
        case WorkspaceRecordSubject::Dimension:
        case WorkspaceRecordSubject::Constraint:
            ToolsApplied.ActiveDimension = ParametricToolDimension::Edge;
            ToolsApplied.MeasurableCondition = true;
            break;
        case WorkspaceRecordSubject::Pattern:
        case WorkspaceRecordSubject::Mirror:
            ToolsApplied.ActiveDimension = ParametricToolDimension::Face;
            ToolsApplied.ReferencePlaneCondition = true;
            ToolsApplied.MeasurableCondition = true;
            break;
        case WorkspaceRecordSubject::Folder:
        case WorkspaceRecordSubject::SubjectCount:
            break;
    }
}

void RecordCadFallback(RecordingSurface& Surface, const PlaneExtent& Extent,
                       const WorkspaceCadPacket& Packet)
{
    if (!Packet.ExtentStanding)
        return;

    const float Pad = 18.0f;
    const float SpanAlong = Packet.MaximumAlong - Packet.MinimumAlong;
    const float SpanAcross = Packet.MaximumAcross - Packet.MinimumAcross;
    const float SafeAlong = SpanAlong > 1.0e-4f ? SpanAlong : 1.0f;
    const float SafeAcross = SpanAcross > 1.0e-4f ? SpanAcross : 1.0f;
    const float ScaleX = (Extent.Width() - Pad * 2.0f) / SafeAlong;
    const float ScaleY = (Extent.Height() - Pad * 2.0f) / SafeAcross;
    const float Scale = std::min(ScaleX, ScaleY);
    const float OccupiedX = SafeAlong * Scale;
    const float OccupiedY = SafeAcross * Scale;
    const float OriginX = Extent.MinimumX + (Extent.Width() - OccupiedX) * 0.5f;
    const float OriginY = Extent.MinimumY + (Extent.Height() - OccupiedY) * 0.5f;

    const auto Screen = [&](float Along, float Across) -> std::pair<float, float>
    {
        const float X = OriginX + (Along - Packet.MinimumAlong) * Scale;
        const float Y = OriginY + OccupiedY - (Across - Packet.MinimumAcross) * Scale;
        return { X, Y };
    };

    Surface.Confine(Extent);

    for (std::uint32_t Index = 0u; Index < Packet.FillCount; ++Index)
    {
        const WorkspaceCadFillTriangle& Fill = Packet.Fills[Index];
        const auto P0 = Screen(Fill.Along0, Fill.Across0);
        const auto P1 = Screen(Fill.Along1, Fill.Across1);
        const auto P2 = Screen(Fill.Along2, Fill.Across2);
        const float Corners[6] = { P0.first, P0.second, P1.first, P1.second, P2.first, P2.second };
        Surface.Tongue(Corners, 3u, ThemeToken{
            static_cast<std::uint8_t>((Fill.Packed >> 16u) & 0xFFu),
            static_cast<std::uint8_t>((Fill.Packed >> 8u) & 0xFFu),
            static_cast<std::uint8_t>((Fill.Packed >> 0u) & 0xFFu),
            static_cast<std::uint8_t>((Fill.Packed >> 24u) & 0xFFu) });
    }

    for (std::uint32_t Index = 0u; Index < Packet.SegmentCount; ++Index)
    {
        const WorkspaceCadSegment& Segment = Packet.Segments[Index];
        const auto P0 = Screen(Segment.Along0, Segment.Across0);
        const auto P1 = Screen(Segment.Along1, Segment.Across1);
        const float PointsX[2] = { P0.first, P1.first };
        const float PointsY[2] = { P0.second, P1.second };
        Surface.Polyline(PointsX, PointsY, 2u,
            ThemeToken{ static_cast<std::uint8_t>((Segment.Packed >> 16u) & 0xFFu),
                        static_cast<std::uint8_t>((Segment.Packed >> 8u) & 0xFFu),
                        static_cast<std::uint8_t>((Segment.Packed >> 0u) & 0xFFu),
                        static_cast<std::uint8_t>((Segment.Packed >> 24u) & 0xFFu) },
            Segment.Thickness);
    }

    for (std::uint32_t Index = 0u; Index < Packet.MarkerCount; ++Index)
    {
        const WorkspaceCadMarker& Marker = Packet.Markers[Index];
        const auto P = Screen(Marker.Along, Marker.Across);
        Surface.Medallion(P.first, P.second, Marker.Radius,
            ThemeToken{ static_cast<std::uint8_t>((Marker.Packed >> 16u) & 0xFFu),
                        static_cast<std::uint8_t>((Marker.Packed >> 8u) & 0xFFu),
                        static_cast<std::uint8_t>((Marker.Packed >> 0u) & 0xFFu),
                        static_cast<std::uint8_t>((Marker.Packed >> 24u) & 0xFFu) });
    }

    Surface.Release();
}

} // namespace

int main(int ArgumentCount, char** ArgumentValues)
{
    using namespace Slate;

    HostDeclaration Declared;
    Declared.Naming = HostName;
    Declared.WindowCaption = WindowTitle;
    Declared.InitialWidth = InitialWidth;
    Declared.InitialHeight = InitialHeight;
    Declared.Pacing = LatencyIntent::SteadyPacing;
#ifdef SLATE_DEBUG
    Declared.DiagnosticRequested = true;
#endif

    HostLifecycle Lifetime;
    if (!Lifetime.ConstructHost(Declared).Resolved)
        return 1;

    ViewportSequence Viewport;
    DrawerDeclaration NorthDrawer = { "ControlCentre", SymbolSubject::PulseTrace, 2u };
    DrawerDeclaration SouthDrawer = { "AssetBrowser", SymbolSubject::FolderClosed, 3u };
    if (!Viewport.ConstructViewportSequence(Attach(Lifetime.Offering()), NorthDrawer, SouthDrawer).Resolved)
    {
        std::printf("%s — the viewport sequence was rejected\n", HostName);
        return 1;
    }

    WorkspaceIndex Workspaces;
    WorkspacePanel Workspace;
    EditorPanel WorkspacePanels;
    PanelStructure PanelPartitions[WorkspaceIndex::WorkspaceLimit];
    EditorPanelConfiguration PanelConfiguration[WorkspaceIndex::WorkspaceLimit];
    ParametricViewportState ViewStates[WorkspaceIndex::WorkspaceLimit] = {};
    ControlCentrePanel ControlCentre;
    ControlCentreConfiguration ControlCentreValues;
    FontLoader Fonts;
    ControlIndex BrowserInteraction;
    ContentBrowserPanel ContentBrowser;
    ContentBrowserConfiguration ContentBrowserApplied;
    ContentLibrary ContentApplied;
    ControlIndex ParametricInteraction;
    ParametricWorkspacePanel ParametricPanel;
    ParametricToolsContext ToolsApplied = {};
    ParametricToolsPanel ToolPanel;
    ShaderCodec CadCodec;
    WorkspaceCadPass CadPass;
    WorkspaceOverlayPass OverlayPass;

    WorkspaceNameIndex Naming;
    SketchStructure Sketch;
    WorkspaceRecordStructure Records;
    WorkspaceRevisionSequence Revisions;
    WorkspaceDirectoryProjection Directory;
    ParametricWorkspaceBridgeStorage Bridge;
    static WorkspaceCadPacket CadPacket;
    static OverlayGeometry ViewportOverlays[PanelStructure::RecordLimit];
    ParametricWorkspaceContext ParametricApplied = {};
    ParametricDraftState Draft = {};
    ParametricViewportSelection SemanticSelection = {};
    ParametricViewportSelection HoveredSelection = {};
    ParametricTransformState Transform = {};
    WorkspaceRecordName PendingSelection = {};
    bool ParametricSeeded = false;
    bool ProjectionWarned = false;
    bool CadPacketWarned = false;
    bool CadPassWarned = false;
    bool OverlayPassWarned = false;
    std::uint32_t UploadedCadGeneration = 0xFFFFFFFFu;
    std::uint32_t RegisterIntoNode = 0u;
    double SessionMilliseconds = 0.0;
    double LastGPressedMilliseconds = -1000.0;

    const char* const InvokedAs = (ArgumentCount > 0) ? ArgumentValues[0] : "";
    const std::filesystem::path ExecutablePath = InvokedAs[0] != '\0'
                                               ? std::filesystem::absolute(InvokedAs)
                                               : std::filesystem::current_path();
    const std::string FontRoot = (ExecutablePath.parent_path() / "EngineContent" / "FontArchives").string();

    {
        ThemeSelection Recorded;
        if (ThemeInterchange::AdoptBeside(InvokedAs, Recorded))
        {
            ControlCentreValues.Theme = Recorded.Current;
            ControlCentreValues.Primary = Recorded.Primary;
            ControlCentreValues.Secondary = Recorded.Secondary;
            ControlCentreValues.Information = Recorded.Information;
            ControlCentreValues.Warning = Recorded.Warning;
            ControlCentreValues.Alert = Recorded.Alert;
        }
    }

    ThemeSelection InscribedSelection;
    InscribedSelection.Current = ControlCentreValues.Theme;
    InscribedSelection.Primary = ControlCentreValues.Primary;
    InscribedSelection.Secondary = ControlCentreValues.Secondary;
    InscribedSelection.Information = ControlCentreValues.Information;
    InscribedSelection.Warning = ControlCentreValues.Warning;
    InscribedSelection.Alert = ControlCentreValues.Alert;
    Viewport.Retint(InscribedSelection);

    Viewport.Surface().ApplyFontLoader(Fonts);
    Discard(Fonts.Discover(FontRoot.c_str()));
    Discard(Fonts.PreparePreviews(1.0f));
    Discard(Fonts.Load(FontRoot.c_str(), Viewport.Appearance().Fonts, 1.0f));
    ControlCentre.SetFontFamilies(Fonts);
    for (std::uint32_t Index = 0u; Index < Fonts.FamilyCount(); ++Index)
        if (Fonts.FamilyName(Index) != nullptr &&
            std::strcmp(Fonts.FamilyName(Index), Viewport.Appearance().Fonts.Family) == 0)
        {
            ControlCentreValues.Font = Index;
            break;
        }

    SeedParametricWorkspace(Naming, Sketch, Records, Revisions);

    if (!Workspace.ConstructWorkspacePanel(Viewport.Surface(), Viewport.Appearance()).Resolved)
    {
        std::printf("%s — the workspace panel was rejected\n", HostName);
        return 1;
    }

    if (!WorkspacePanels.ConstructEditorPanel(Viewport.MotionSource(), Viewport.Surface(), Viewport.Appearance()).Resolved)
    {
        std::printf("%s — the editor panels were rejected\n", HostName);
        return 1;
    }

    if (!ControlCentre.ConstructControlCentrePanel(Viewport.MotionSource(), Viewport.Surface(), Viewport.Appearance()).Resolved)
    {
        std::printf("%s — the Control Centre panel was rejected\n", HostName);
        return 1;
    }

    if (!BrowserInteraction.AttachMotion(Viewport.MotionSource()).Resolved)
    {
        std::printf("%s — the content browser index was rejected\n", HostName);
        return 1;
    }

    if (!ContentBrowser.ConstructContentBrowserPanel(BrowserInteraction, Viewport.Surface(), Viewport.Appearance()).Resolved)
    {
        std::printf("%s — the content browser was rejected\n", HostName);
        return 1;
    }

    ContentBrowser.Reapply(Viewport.Appearance());
    ApplyReferenceContent(ContentApplied);
    PopulateImportDirectory(ContentBrowserApplied, std::filesystem::path("EngineContent"));

    if (!ParametricInteraction.AttachMotion(Viewport.MotionSource()).Resolved)
    {
        std::printf("%s — the parametric workspace index was rejected\n", HostName);
        return 1;
    }

    if (!ParametricPanel.ConstructParametricWorkspacePanel(ParametricInteraction,
                                                           Viewport.MotionSource(),
                                                           Viewport.Surface(),
                                                           Viewport.Appearance()).Resolved)
    {
        std::printf("%s — the parametric workspace panel was rejected\n", HostName);
        return 1;
    }

    if (!ToolPanel.ConstructParametricToolsPanel(ParametricInteraction,
                                                 Viewport.MotionSource(),
                                                 Viewport.Surface(),
                                                 Viewport.Appearance()).Resolved)
    {
        std::printf("%s — the parametric tools panel was rejected\n", HostName);
        return 1;
    }

    const Outcome<bool> CodecOutcome =
        CadCodec.AttachShaderStreams(Lifetime.DeviceExchange(), ShaderStreamDirectory());
    if (!CodecOutcome.Resolved)
    {
        std::printf("%s — the CAD shader streams were not attached (reason %u: %s)\n",
                    HostName,
                    static_cast<unsigned>(CodecOutcome.Error.DeclaredReason),
                    CodecOutcome.Error.Detail);
    }
    else
    {
        const Outcome<bool> PassOutcome = CadPass.ConstructWorkspaceCadPass(Lifetime.DeviceExchange(),
                                                                            Lifetime.DiagnosticsExtension(),
                                                                            CadCodec,
                                                                            Lifetime.Offering().ColourTargetFormat);
        if (!PassOutcome.Resolved)
        {
            CadPassWarned = true;
            std::printf("%s — the CAD pass was not standing (reason %u: %s); using fallback presentation\n",
                        HostName,
                        static_cast<unsigned>(PassOutcome.Error.DeclaredReason),
                        PassOutcome.Error.Detail);
        }
    }

    if (!Viewport.Seam().ApplyWorkspaceStyle(Viewport.Appearance().WorkspaceMeasure,
                                             Viewport.Appearance().Workspace).Resolved)
    {
        std::printf("%s — the workspace style was not applied\n", HostName);
    }

    const Outcome<std::uint32_t> DefaultWorkspace = Workspaces.Register(WorkspaceSubject::Parametric);
    if (!DefaultWorkspace.Resolved)
    {
        std::printf("%s — the default workspace could not be opened\n", HostName);
        return 1;
    }
    ConstructParametricLayout(PanelPartitions[DefaultWorkspace.Resolve()]);
    std::printf("%s — opened %s\n", HostName, Workspaces.ActiveTitle());

    while (Lifetime.Active())
    {
        const TickPass Pass = Lifetime.Await(WorkspaceGround);
        Discard(Fonts.FlushPending());
        SessionMilliseconds += Pass.ElapsedMilliseconds;

        if (Pass.Current == TickCondition::Closed)
            break;

        if (Pass.DeviceRetiring)
        {
            Viewport.Reclaim();
            OverlayPass.Reclaim();
            CadPass.Reclaim();
            CadCodec.Reclaim();
            UploadedCadGeneration = 0xFFFFFFFFu;
            continue;
        }

        if (Lifetime.DeviceRecovered())
        {
            if (!Viewport.ConstructViewportSequence(Attach(Lifetime.Offering()), NorthDrawer, SouthDrawer).Resolved)
            {
                std::printf("%s — the interface could not be rebuilt on the recovered device\n", HostName);
                break;
            }
            static_cast<void>(Lifetime.DisplayRecovered());
            static_cast<void>(Viewport.Seam().ApplyWorkspaceStyle(Viewport.Appearance().WorkspaceMeasure,
                                                                  Viewport.Appearance().Workspace));
            ParametricPanel.Reapply(Viewport.Appearance());
            ToolPanel.Reapply(Viewport.Appearance());
            ContentBrowser.Reapply(Viewport.Appearance());

            const Outcome<bool> Reattached =
                CadCodec.AttachShaderStreams(Lifetime.DeviceExchange(), ShaderStreamDirectory());
            if (Reattached.Resolved)
            {
                const Outcome<bool> Rebuilt = CadPass.ConstructWorkspaceCadPass(Lifetime.DeviceExchange(),
                                                                                Lifetime.DiagnosticsExtension(),
                                                                                CadCodec,
                                                                                Lifetime.Offering().ColourTargetFormat);
                if (!Rebuilt.Resolved && !CadPassWarned)
                {
                    CadPassWarned = true;
                    std::printf("%s — the CAD pass could not be rebuilt (reason %u: %s)\n",
                                HostName,
                                static_cast<unsigned>(Rebuilt.Error.DeclaredReason),
                                Rebuilt.Error.Detail);
                }
                else if (Rebuilt.Resolved)
                {
                    CadPassWarned = false;
                }
            }
            else if (!CadPassWarned)
            {
                CadPassWarned = true;
                std::printf("%s — the CAD shader streams could not be reattached (reason %u: %s)\n",
                            HostName,
                            static_cast<unsigned>(Reattached.Error.DeclaredReason),
                            Reattached.Error.Detail);
            }

            UploadedCadGeneration = 0xFFFFFFFFu;
        }
        else if (Lifetime.DisplayRecovered())
        {
            const DeviceOffering Offered = Lifetime.Offering();
            if (!Viewport.Renegotiate(Offered.MinimumDisplayImageCount, Offered.DisplayImageCount))
                std::printf("%s — the interface rejected the restated image counts\n", HostName);
        }

        if (Pass.Current != TickCondition::Recording)
            continue;

        if (!Viewport.Advance(Pass.ElapsedMilliseconds).Resolved)
        {
            Discard(Viewport.Abandon());
            continue;
        }

        const Outcome<bool> Presented = SynchroniseParametricPresentation(Records, Revisions,
                                                                          Directory, Bridge,
                                                                          ParametricApplied,
                                                                          PendingSelection,
                                                                          ParametricSeeded);
        if (!Presented.Resolved && !ProjectionWarned)
        {
            std::printf("%s — the parametric presentation bridge refused (reason %u: %s)\n",
                        HostName,
                        static_cast<unsigned>(Presented.Error.DeclaredReason),
                        Presented.Error.Detail);
            ProjectionWarned = true;
        }
        else if (Presented.Resolved)
        {
            ProjectionWarned = false;
        }
        else
        {
            Directory.Reclaim();
            Bridge.Reclaim();
        }

        const Outcome<bool> PacketProjected = SynchroniseCadPacket(Sketch, Records, CadPacket);
        if (!PacketProjected.Resolved && !CadPacketWarned)
        {
            std::printf("%s — the CAD packet projection refused (reason %u: %s)\n",
                        HostName,
                        static_cast<unsigned>(PacketProjected.Error.DeclaredReason),
                        PacketProjected.Error.Detail);
            CadPacketWarned = true;
        }
        else if (PacketProjected.Resolved)
        {
            CadPacketWarned = false;
        }
        else
        {
            CadPacket.Reset();
        }

        SynchroniseToolContext(Directory, Records, Sketch, ParametricApplied, ToolsApplied);

        const PlaneExtent Whole = Spanning(0.0f, 0.0f,
                                           static_cast<float>(Pass.Width),
                                           static_cast<float>(Pass.Height));

        const PointerCondition& ForegroundPointer = Viewport.Surface().Pointer();
        const PlaneExtent NorthInterior = Viewport.Drawers().Interior(DrawerBearing::North);
        const PlaneExtent SouthInterior = Viewport.Drawers().Interior(DrawerBearing::South);
        const bool ForegroundDrawerStanding =
            (NorthInterior.MaximumY > 0.0f && NorthInterior.MinimumY < static_cast<float>(Pass.Height)) ||
            (SouthInterior.MaximumY > 0.0f && SouthInterior.MinimumY < static_cast<float>(Pass.Height));
        const bool PointerBehindDrawer =
            NorthInterior.Encloses(ForegroundPointer.PositionX, ForegroundPointer.PositionY) ||
            SouthInterior.Encloses(ForegroundPointer.PositionX, ForegroundPointer.PositionY);
        PointerCondition BackgroundPointer = ForegroundPointer;
        if (PointerBehindDrawer)
        {
            BackgroundPointer.PositionX = BackgroundPointer.PositionY = -1000000.0f;
            BackgroundPointer.TravelX = BackgroundPointer.TravelY = BackgroundPointer.WheelY = 0.0f;
            BackgroundPointer.ContactHeld = BackgroundPointer.ContactPressed = false;
            BackgroundPointer.ContactDoublePressed = BackgroundPointer.ContactReleased = false;
            BackgroundPointer.SecondaryHeld = BackgroundPointer.SecondaryPressed = false;
            BackgroundPointer.SecondaryReleased = false;
        }

        const ModifierCondition Modifiers = Viewport.Seam().Modifiers();

        Discard(Workspace.Record(Whole, Workspaces.ActiveTitle()));
        Viewport.Seam().RecordDockSpace(Whole);

        const std::uint32_t OpenCount = Workspaces.OpenCount();
        std::uint32_t Withdrawing = OpenCount;
        const std::uint32_t ApplyInto = RegisterIntoNode;
        RegisterIntoNode = 0u;

        PlaneExtent ViewportLeafRects[PanelStructure::RecordLimit] = {};
        WorkspaceCadProjection ViewportCadProjections[PanelStructure::RecordLimit] = {};
        std::uint32_t ViewportLeafTally = 0u;
        PlaneExtent ToolLeafRects[PanelStructure::RecordLimit] = {};
        std::uint32_t ToolLeafTally = 0u;

        WorkspacePanels.Advance(BackgroundPointer, Pass.ElapsedMilliseconds);

        for (std::uint32_t Index = 0u; Index < OpenCount; ++Index)
        {
            const char* Titled = Workspaces.Titled(Index);
            if (Titled == nullptr)
                continue;

            bool Current = true;
            const PlaneExtent PanelExtent = Viewport.Seam().EnterWorkspaceWindow(
                Titled, !Workspaces.Applied(Index), ApplyInto, Current);
            Workspaces.Apply(Index);

            if (PanelExtent.Width() > 0.0f && PanelExtent.Height() > 0.0f)
            {
                Discard(Viewport.Surface().SwitchToWindow());
                Discard(WorkspacePanels.Record(PanelExtent,
                                               PanelPartitions[Index],
                                               PanelConfiguration[Index],
                                               Index,
                                               true));

                for (std::uint32_t Leaf = 0u; Leaf < WorkspacePanels.LeafCount(); ++Leaf)
                {
                    const PlaneExtent LeafBody = WorkspacePanels.LeafBody(Leaf);
                    switch (WorkspacePanels.LeafSubject(Leaf))
                    {
                        case PanelSubject::Outliner:
                        {
                            const bool PropertyPresented = Bridge.Property.Naming != nullptr
                                                        && Bridge.Property.Naming[0] != '\0';
                            ParametricPanel.RecordOutliner(LeafBody, ParametricApplied,
                                Bridge.DirectoryRows.empty() ? nullptr : Bridge.DirectoryRows.data(),
                                static_cast<std::uint32_t>(Bridge.DirectoryRows.size()),
                                PropertyPresented ? &Bridge.Property : nullptr,
                                Bridge.RevisionRows.empty() ? nullptr : Bridge.RevisionRows.data(),
                                static_cast<std::uint32_t>(Bridge.RevisionRows.size()));
                            break;
                        }

                        case PanelSubject::ParametricTools:
                            if (ToolLeafTally < PanelStructure::RecordLimit)
                                ToolLeafRects[ToolLeafTally++] = LeafBody;
                            ToolPanel.Record(LeafBody, ToolsApplied);
                            break;

                        case PanelSubject::Viewport:
                        {
                            OverlayGeometry* LeafOverlay = nullptr;
                            if (ViewportLeafTally < PanelStructure::RecordLimit)
                            {
                                ViewportLeafRects[ViewportLeafTally] = LeafBody;
                                LeafOverlay = &ViewportOverlays[ViewportLeafTally];
                                LeafOverlay->Reset();
                            }

                            ParametricViewportState& View = ViewStates[Index];
                            const bool PointerInside = LeafBody.Encloses(BackgroundPointer.PositionX, BackgroundPointer.PositionY);
                            bool PointerTaken = false;
                            Viewport.Surface().Confine(LeafBody);
                            RecordViewportOrientationHud(Viewport.Surface(), LeafBody, BackgroundPointer,
                                                         View, PanelConfiguration[Index].Perspective,
                                                         PointerTaken);
                            if (PointerInside && !PointerTaken && !Transform.Engaged)
                                DriveViewport(LeafBody, BackgroundPointer, Modifiers,
                                              View, PanelConfiguration[Index].Perspective);

                            const SpatialBasis Basis = ResolveSketchBasis(Sketch);
                            if (LeafOverlay != nullptr)
                                RecordViewportGridOverlay(*LeafOverlay, LeafBody, Sketch, View,
                                                          PanelConfiguration[Index].Perspective,
                                                          PanelConfiguration[Index]);

                            DriveViewportSelectionAndTransform(LeafBody, BackgroundPointer,
                                                               Viewport.Surface().TextInput(), Modifiers,
                                                               Basis, View,
                                                               PanelConfiguration[Index].Perspective,
                                                               ToolsApplied.ActiveSubject,
                                                               Directory, ParametricApplied,
                                                               Sketch, Records, Revisions,
                                                               PendingSelection,
                                                               SemanticSelection, HoveredSelection,
                                                               Transform,
                                                               LeafOverlay != nullptr ? *LeafOverlay : ViewportOverlays[0],
                                                               PointerTaken,
                                                               SessionMilliseconds,
                                                               LastGPressedMilliseconds);

                            if (!Transform.Engaged)
                                DriveDrawingWithModifiers(LeafBody, BackgroundPointer, Modifiers,
                                                         Basis, View,
                                                         PanelConfiguration[Index].Perspective,
                                                         ToolsApplied.ActiveSubject,
                                                         Naming, Sketch, Records, Revisions,
                                                         PendingSelection, Draft, PointerTaken);

                            Discard(SynchroniseCadPacket(Sketch, Records, CadPacket));
                            if (!CadPass.Standing())
                                RecordCadFallback(Viewport.Surface(), LeafBody, Sketch, View,
                                                  PanelConfiguration[Index].Perspective, CadPacket);
                            RecordDraftPreview(Viewport.Surface(), LeafBody, Sketch, View,
                                               PanelConfiguration[Index].Perspective, Draft);
                            RecordViewportStateReadout(Viewport.Surface(), LeafBody, View,
                                                       PanelConfiguration[Index].Perspective, CadPacket);
                            RecordViewportTransformReadout(Viewport.Surface(), LeafBody, Transform);
                            if (LeafOverlay != nullptr && !OverlayPass.Standing())
                                RecordViewportOverlayFallback(Viewport.Surface(), LeafBody, *LeafOverlay);
                            Viewport.Surface().Release();
                            if (PointerTaken)
                                Viewport.Seam().WithholdPointer();

                            if (ViewportLeafTally < PanelStructure::RecordLimit)
                            {
                                ViewportCadProjections[ViewportLeafTally] = ResolveCadProjection(
                                    Basis, View,
                                    PanelConfiguration[Index].Perspective,
                                    LeafBody, Pass.Width, Pass.Height);
                                ++ViewportLeafTally;
                            }
                            break;
                        }

                        default:
                            break;
                    }
                }

                WorkspacePanels.RecordDeferredPopups(PanelPartitions[Index], PanelConfiguration[Index]);
                if (WorkspacePanels.PointerCaptured(Index))
                    Viewport.Seam().WithholdPointer();
            }

            Viewport.Seam().LeaveWorkspaceWindow();
            Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Beneath));
            if (!Current)
                Withdrawing = Index;
        }

        if (Withdrawing < OpenCount)
        {
            Discard(Workspaces.Withdraw(Withdrawing));
            WorkspacePanels.WithdrawPresentation(Withdrawing);
            for (std::uint32_t Moving = Withdrawing; Moving + 1u < OpenCount; ++Moving)
            {
                PanelPartitions[Moving] = PanelPartitions[Moving + 1u];
                PanelConfiguration[Moving] = PanelConfiguration[Moving + 1u];
            }
            PanelPartitions[OpenCount - 1u].Reset();
            PanelConfiguration[OpenCount - 1u] = EditorPanelConfiguration{};
        }

        std::uint32_t AskingNode = 0u;
        if (Viewport.Seam().RecordWorkspaceAddition(Workspace.Strip(), OpenCount, AskingNode))
        {
            RegisterIntoNode = AskingNode;
            const Outcome<std::uint32_t> RegisteredWorkspace = Workspaces.Register(WorkspaceSubject::Parametric);
            if (RegisteredWorkspace.Resolved)
                ConstructParametricLayout(PanelPartitions[RegisteredWorkspace.Resolve()]);
        }

        if (OpenCount == 0u && Viewport.Seam().VacantPressed(Whole))
        {
            const Outcome<std::uint32_t> RegisteredWorkspace = Workspaces.Register(WorkspaceSubject::Parametric);
            if (RegisteredWorkspace.Resolved)
                ConstructParametricLayout(PanelPartitions[RegisteredWorkspace.Resolve()]);
        }

        Viewport.RecordDrawers();
        Viewport.DrawerPanels();

        const PlaneExtent BrowserInterior = Viewport.Drawers().Interior(DrawerBearing::South);
        BrowserInteraction.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);
        ContentBrowser.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);
        if (BrowserInterior.Width() > 0.0f && BrowserInterior.Height() > 0.0f)
        {
            Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Above));
            Viewport.Surface().Ground(BrowserInterior, Viewport.Appearance().Colour.SurfaceCurrent,
                                      0.0f, CornerNone);
            ContentBrowser.RecordBrowser(BrowserInterior, ContentApplied, ContentBrowserApplied);
            ContentBrowser.RecordDeferred();

            if (ContentBrowserApplied.ImportBrowseRequested)
            {
                std::filesystem::path Destination(ContentBrowserApplied.ImportLocation);
                if (ContentBrowserApplied.ImportTaken < ContentBrowserApplied.ImportEntryCount &&
                    ContentBrowserApplied.ImportEntries[ContentBrowserApplied.ImportTaken].Directory)
                {
                    Destination /= ContentBrowserApplied.ImportEntries[ContentBrowserApplied.ImportTaken].Naming;
                }
                PopulateImportDirectory(ContentBrowserApplied, Destination);
                ContentBrowserApplied.ImportBrowseRequested = false;
            }

            ContentBrowser.Exclude(Viewport.Drawers(), DrawerBearing::South);
            Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Beneath));
        }

        const PlaneExtent ControlInterior = Viewport.Drawers().Interior(DrawerBearing::North);
        ControlCentre.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);
        Viewport.ApplyTypographyRoles(ControlCentreValues.TypographySize,
                                      ControlCentreValues.TypographyWeight);
        Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Above));
        if (ControlInterior.Width() > 0.0f && ControlInterior.Height() > 0.0f)
            Viewport.Surface().Ground(ControlInterior, Viewport.Appearance().Colour.SurfaceCurrent,
                                      0.0f, CornerNone);
        Discard(ControlCentre.Record(ControlInterior, ControlCentreValues));

        if (Viewport.ApplyInterfaceScale(ControlCentreValues.Scaling))
        {
            Discard(Viewport.Seam().ApplyWorkspaceStyle(Viewport.Appearance().WorkspaceMeasure,
                                                        Viewport.Appearance().Workspace));
            ContentBrowser.Reapply(Viewport.Appearance());
            ParametricPanel.Reapply(Viewport.Appearance());
            ToolPanel.Reapply(Viewport.Appearance());
        }
        Discard(Viewport.Seam().ApplyInterfaceAntialiasing(ControlCentreValues.GeometryAntialiasing));

        {
            ThemeSelection Chosen;
            Chosen.Current = ControlCentreValues.Theme;
            Chosen.Primary = ControlCentreValues.Primary;
            Chosen.Secondary = ControlCentreValues.Secondary;
            Chosen.Information = ControlCentreValues.Information;
            Chosen.Warning = ControlCentreValues.Warning;
            Chosen.Alert = ControlCentreValues.Alert;
            if (ControlCentreValues.Font < Fonts.FamilyCount() && Fonts.FamilyName(ControlCentreValues.Font) != nullptr)
                std::strncpy(Chosen.FontFamily, Fonts.FamilyName(ControlCentreValues.Font), sizeof(Chosen.FontFamily) - 1u);

            const bool FamilyAltered = std::strcmp(Chosen.FontFamily, InscribedSelection.FontFamily) != 0;
            const bool Altered = Chosen.Current != InscribedSelection.Current
                              || Chosen.Primary != InscribedSelection.Primary
                              || Chosen.Secondary != InscribedSelection.Secondary
                              || Chosen.Information != InscribedSelection.Information
                              || Chosen.Warning != InscribedSelection.Warning
                              || Chosen.Alert != InscribedSelection.Alert
                              || std::strcmp(Chosen.FontFamily, InscribedSelection.FontFamily) != 0;

            if (Altered)
            {
                Discard(ThemeInterchange::RecordBeside(InvokedAs, Chosen));
                InscribedSelection = Chosen;
                Viewport.Retint(Chosen);
                Discard(Viewport.Seam().ApplyWorkspaceStyle(Viewport.Appearance().WorkspaceMeasure,
                                                            Viewport.Appearance().Workspace));
                ContentBrowser.Reapply(Viewport.Appearance());
                ParametricPanel.Reapply(Viewport.Appearance());
                ToolPanel.Reapply(Viewport.Appearance());
                if (FamilyAltered)
                    Fonts.RequestLoad(FontRoot.c_str(), Viewport.Appearance().Fonts, 1.0f);
            }
        }
        ControlCentre.Exclude(Viewport.Drawers());
        Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Beneath));

        const bool TabPressed = Viewport.Seam().KeyPressed(KeySubject::Summon);
        const PointerCondition& Hovered = Viewport.Surface().Pointer();
        bool PointerInTools = false;
        for (std::uint32_t Index = 0u; Index < ToolLeafTally; ++Index)
            if (ToolLeafRects[Index].Encloses(Hovered.PositionX, Hovered.PositionY))
            {
                PointerInTools = true;
                break;
            }

        ParametricInteraction.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);
        ParametricPanel.Advance(BackgroundPointer, Pass.ElapsedMilliseconds,
                                ParametricApplied,
                                TabPressed && !PointerInTools && !PointerBehindDrawer,
                                Viewport.Seam().Modifiers());
        ToolPanel.Advance(BackgroundPointer, Pass.ElapsedMilliseconds,
                          ToolsApplied,
                          TabPressed && PointerInTools && !PointerBehindDrawer);

        if (ParametricApplied.SearchTaken)
        {
            static_cast<void>(Viewport.Seam().AcceptTyped(ParametricApplied.RowRetention,
                                                          ParametricWorkspaceContext::SearchLimit));

            if (Viewport.Seam().KeyPressed(KeySubject::Retract))
            {
                std::uint32_t Occupied = 0u;
                while (Occupied + 1u < ParametricWorkspaceContext::SearchLimit &&
                       ParametricApplied.RowRetention[Occupied] != '\0')
                    ++Occupied;
                if (Occupied > 0u)
                    ParametricApplied.RowRetention[Occupied - 1u] = '\0';
            }

            if (Viewport.Seam().KeyPressed(KeySubject::Withdraw))
                ParametricApplied.RowRetention[0] = '\0';
        }
        else if (Viewport.Seam().KeyPressed(KeySubject::Withdraw))
        {
            if (Transform.Engaged)
                CancelTransformSession(Sketch, Transform);
            else
                CancelDraft(Draft);
        }

        if (Viewport.SealPanels().Resolved)
        {
            Discard(Lifetime.BeginDisplay());
            if (!Viewport.Record(Pass.Recording))
                std::printf("%s — the interface content was not recorded\n", HostName);

            if (CadPass.Standing())
            {
                if (UploadedCadGeneration != CadPacket.Generation)
                {
                    CadPass.Upload(CadPacket);
                    UploadedCadGeneration = CadPacket.Generation;
                }

                for (std::uint32_t ViewportIndex = 0u; ViewportIndex < ViewportLeafTally; ++ViewportIndex)
                {
                    const PlaneExtent& LeafRect = ViewportLeafRects[ViewportIndex];
                    CadPass.Record(Pass.Recording, ViewportCadProjections[ViewportIndex],
                                   LeafRect.MinimumX, LeafRect.MinimumY,
                                   LeafRect.MaximumX, LeafRect.MaximumY);
                }
            }

            if (OverlayPass.Standing())
            {
                for (std::uint32_t ViewportIndex = 0u;
                     !ForegroundDrawerStanding && ViewportIndex < ViewportLeafTally;
                     ++ViewportIndex)
                {
                    const PlaneExtent& LeafRect = ViewportLeafRects[ViewportIndex];
                    OverlayPass.Upload(ViewportOverlays[ViewportIndex]);
                    OverlayPass.Record(Pass.Recording, Pass.Width, Pass.Height,
                                       LeafRect.MinimumX, LeafRect.MinimumY,
                                       LeafRect.MaximumX, LeafRect.MaximumY);
                }
            }
        }
        else
        {
            Discard(Viewport.Abandon());
        }

        if (!Lifetime.Complete().Resolved)
            break;
    }

    const std::uint32_t Serious = Lifetime.StateDiagnostics();
    ControlCentre.Reset();
    ContentBrowser.Reset();
    ParametricPanel.Reset();
    ToolPanel.Reset();
    WorkspacePanels.Reset();
    for (std::uint32_t Index = 0u; Index < WorkspaceIndex::WorkspaceLimit; ++Index)
        PanelPartitions[Index].Reset();
    Workspace.Reset();
    Workspaces.Reset();
    OverlayPass.Reclaim();
    CadPass.Reclaim();
    CadCodec.Reclaim();
    Viewport.Reclaim();
    Lifetime.Reclaim();

    std::printf("%s — exited cleanly\n", HostName);
    return (Serious == 0u) ? 0 : 1;
}
