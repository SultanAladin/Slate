//============================================================================================================================================
//                                                   SHAREDVIEWPORTHOSTBRIDGE.H
//============================================================================================================================================
// 🧩 Shared host-side viewport support used by EditorHost, PaintHost and ParametricSketchHost.
//    The hosts stay standalone executables; this header keeps their common runtime decisions in one place.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "SlateDocument/Format/WorkspaceSceneActivation/Api/WorkspaceSceneActivation.h"
#include "SlateUI/Interface/ContentBrowserPanel/Api/ContentBrowserPanel.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                  HOST FEATURE SEPARATION
//------------------------------------------------------------------------------------------------------------------------

// 📝 Compile-time feature switches. Hosts pass one of these masks into the shared helpers rather than
//    cloning the helper bodies. The standalone boundaries remain explicit: Paint does not ask for CAD,
//    ParametricSketch does not ask for texture-paint layer editing, and Editor may ask for both.
constexpr std::uint32_t SharedViewportFeatureSceneEnvironment = 1u << 0u;
constexpr std::uint32_t SharedViewportFeatureCodexScene       = 1u << 1u;
constexpr std::uint32_t SharedViewportFeatureCadSketch        = 1u << 2u;
constexpr std::uint32_t SharedViewportFeaturePaintWorkspace   = 1u << 3u;

#if defined(SLATE_EDITOR_HOST)
constexpr std::uint32_t SharedViewportHostFeatures = SharedViewportFeatureSceneEnvironment |
                                                     SharedViewportFeatureCodexScene |
                                                     SharedViewportFeatureCadSketch |
                                                     SharedViewportFeaturePaintWorkspace;
#elif defined(SLATE_PAINT_HOST)
constexpr std::uint32_t SharedViewportHostFeatures = SharedViewportFeatureSceneEnvironment |
                                                     SharedViewportFeatureCodexScene |
                                                     SharedViewportFeaturePaintWorkspace;
#elif defined(SLATE_PARAMETRIC_SKETCH_HOST)
constexpr std::uint32_t SharedViewportHostFeatures = SharedViewportFeatureCodexScene |
                                                     SharedViewportFeatureCadSketch;
#else
constexpr std::uint32_t SharedViewportHostFeatures = 0u;
#endif

enum class SharedViewportOrientation : std::uint32_t
{
    None = 0u,
    Top,
    Bottom,
    Front,
    Back,
    Right,
    Left,
    Iso
};

struct SharedViewportBasis
{
    double Right[3] = { 1.0, 0.0, 0.0 };
    double Up[3] = { 0.0, 1.0, 0.0 };
    double Forward[3] = { 0.0, 0.0, 1.0 };
};

struct SharedViewportCameraSeed
{
    double Position[3] = { 0.0, 1.2, -4.0 };
    double YawDegrees = 0.0;
    double PitchDegrees = -8.0;
    double FieldOfViewDegrees = 60.0;
};

inline SharedViewportCameraSeed SharedViewportDefaultCamera()
{
    return SharedViewportCameraSeed{};
}

inline bool SharedViewportHasFeature(std::uint32_t FeatureMask)
{
    return (SharedViewportHostFeatures & FeatureMask) != 0u;
}

inline SharedViewportBasis SharedViewportBasisFromYawPitch(double YawDegrees, double PitchDegrees)
{
    const double Yaw = YawDegrees * 3.14159265358979323846 / 180.0;
    const double Pitch = PitchDegrees * 3.14159265358979323846 / 180.0;
    const double CosP = std::cos(Pitch);
    const double SinP = std::sin(Pitch);
    const double SinY = std::sin(Yaw);
    const double CosY = std::cos(Yaw);

    SharedViewportBasis Basis;
    Basis.Forward[0] = CosP * SinY;
    Basis.Forward[1] = SinP;
    Basis.Forward[2] = CosP * CosY;
    Basis.Right[0] = CosY;
    Basis.Right[1] = 0.0;
    Basis.Right[2] = -SinY;
    Basis.Up[0] = -SinP * SinY;
    Basis.Up[1] = CosP;
    Basis.Up[2] = -SinP * CosY;
    return Basis;
}

inline const char* SharedViewportOrientationName(SharedViewportOrientation Orientation)
{
    switch (Orientation)
    {
        case SharedViewportOrientation::Top: return "Top";
        case SharedViewportOrientation::Bottom: return "Bottom";
        case SharedViewportOrientation::Front: return "Front";
        case SharedViewportOrientation::Back: return "Back";
        case SharedViewportOrientation::Right: return "Right";
        case SharedViewportOrientation::Left: return "Left";
        case SharedViewportOrientation::Iso: return "Iso";
        case SharedViewportOrientation::None: return "";
    }
    return "";
}

inline void SharedViewportOrientationPreset(SharedViewportOrientation Orientation,
                                            double& YawDegrees,
                                            double& PitchDegrees)
{
    switch (Orientation)
    {
        case SharedViewportOrientation::Top:    YawDegrees = 0.0;   PitchDegrees = 80.0;  break;
        case SharedViewportOrientation::Bottom: YawDegrees = 0.0;   PitchDegrees = -80.0; break;
        case SharedViewportOrientation::Front:  YawDegrees = 0.0;   PitchDegrees = 0.0;   break;
        case SharedViewportOrientation::Back:   YawDegrees = 180.0; PitchDegrees = 0.0;   break;
        case SharedViewportOrientation::Right:  YawDegrees = 90.0;  PitchDegrees = 0.0;   break;
        case SharedViewportOrientation::Left:   YawDegrees = -90.0; PitchDegrees = 0.0;   break;
        case SharedViewportOrientation::Iso:    YawDegrees = 52.0;  PitchDegrees = 24.0;  break;
        case SharedViewportOrientation::None: break;
    }
}

inline void SharedViewportOrientationPoint(const SharedViewportBasis& Basis,
                                           const PlaneExtent& Extent,
                                           const double Axis[3],
                                           float& X,
                                           float& Y,
                                           double& Depth)
{
    constexpr float Radius = 34.0f;
    const float CentreX = Extent.MinimumX + 52.0f;
    const float CentreY = Extent.MinimumY + 58.0f;
    const double SX = Axis[0] * Basis.Right[0] + Axis[1] * Basis.Right[1] + Axis[2] * Basis.Right[2];
    const double SY = Axis[0] * Basis.Up[0] + Axis[1] * Basis.Up[1] + Axis[2] * Basis.Up[2];
    Depth = Axis[0] * Basis.Forward[0] + Axis[1] * Basis.Forward[1] + Axis[2] * Basis.Forward[2];
    X = CentreX + static_cast<float>(SX) * Radius;
    Y = CentreY - static_cast<float>(SY) * Radius;
}

inline void RecordSharedViewportOrientationGizmo(RecordingSurface& Surface,
                                                 const PlaneExtent& Extent,
                                                 const SharedViewportBasis& Basis)
{
    struct AxisRecord
    {
        double Axis[3];
        ThemeToken Colour;
        bool Positive;
        const char* Label;
    };

    const AxisRecord Axes[6] =
    {
        { {  1.0,  0.0,  0.0 }, ThemeToken{ 0xFCu, 0x5Au, 0x5Au, 255u }, true,  "X" },
        { { -1.0,  0.0,  0.0 }, ThemeToken{ 0xFCu, 0x5Au, 0x5Au, 255u }, false, ""  },
        { {  0.0,  1.0,  0.0 }, ThemeToken{ 0x7Bu, 0xD6u, 0x6Au, 255u }, true,  "Y" },
        { {  0.0, -1.0,  0.0 }, ThemeToken{ 0x7Bu, 0xD6u, 0x6Au, 255u }, false, ""  },
        { {  0.0,  0.0,  1.0 }, ThemeToken{ 0x5Au, 0x8Bu, 0xFCu, 255u }, true,  "Z" },
        { {  0.0,  0.0, -1.0 }, ThemeToken{ 0x5Au, 0x8Bu, 0xFCu, 255u }, false, ""  },
    };

    struct ProjectedAxis
    {
        AxisRecord Axis;
        float X;
        float Y;
        double Depth;
    };

    ProjectedAxis Projected[6] = {};
    for (std::uint32_t Index = 0u; Index < 6u; ++Index)
    {
        Projected[Index].Axis = Axes[Index];
        SharedViewportOrientationPoint(Basis, Extent, Axes[Index].Axis,
                                       Projected[Index].X, Projected[Index].Y, Projected[Index].Depth);
    }

    std::sort(Projected, Projected + 6u, [](const ProjectedAxis& Left, const ProjectedAxis& Right)
    {
        return Left.Depth < Right.Depth;
    });

    const float CentreX = Extent.MinimumX + 52.0f;
    const float CentreY = Extent.MinimumY + 58.0f;
    Surface.Confine(Extent);
    for (const ProjectedAxis& Point : Projected)
    {
        if (!Point.Axis.Positive)
            continue;
        const float X[2] = { CentreX, Point.X };
        const float Y[2] = { CentreY, Point.Y };
        Surface.Polyline(X, Y, 2u, ThemeToken{ 255u, 255u, 255u, 36u }, 2.0f);
    }

    for (const ProjectedAxis& Point : Projected)
    {
        const float Radius = Point.Axis.Positive ? 9.0f : 7.0f;
        if (Point.Axis.Positive)
        {
            ThemeToken Fill = Point.Axis.Colour;
            Fill.Opacity = Point.Depth > 0.0 ? 255u : 190u;
            Surface.Medallion(Point.X, Point.Y, Radius, Fill);
            Surface.TextRun(Point.X - 3.2f, Point.Y - 5.1f,
                            ThemeToken{ 0u, 0u, 0u, 218u }, Point.Axis.Label, 10.0f, 0.0f, true);
        }
        else
        {
            Surface.Medallion(Point.X, Point.Y, Radius, ThemeToken{ 10u, 12u, 16u, 230u });
            Surface.Medallion(Point.X, Point.Y, Radius - 3.0f, Point.Axis.Colour);
        }
    }
    Surface.Release();
}

inline SharedViewportOrientation HitSharedViewportOrientationGizmo(const PlaneExtent& Extent,
                                                                   const SharedViewportBasis& Basis,
                                                                   float PointerX,
                                                                   float PointerY)
{
    struct HitAxis
    {
        double Axis[3];
        SharedViewportOrientation Orientation;
    };

    const HitAxis Axes[6] =
    {
        { {  1.0,  0.0,  0.0 }, SharedViewportOrientation::Right },
        { { -1.0,  0.0,  0.0 }, SharedViewportOrientation::Left },
        { {  0.0,  1.0,  0.0 }, SharedViewportOrientation::Top },
        { {  0.0, -1.0,  0.0 }, SharedViewportOrientation::Bottom },
        { {  0.0,  0.0,  1.0 }, SharedViewportOrientation::Front },
        { {  0.0,  0.0, -1.0 }, SharedViewportOrientation::Back },
    };

    SharedViewportOrientation Best = SharedViewportOrientation::None;
    float BestDistance = 13.0f * 13.0f;
    for (const HitAxis& Axis : Axes)
    {
        float X = 0.0f;
        float Y = 0.0f;
        double Depth = 0.0;
        SharedViewportOrientationPoint(Basis, Extent, Axis.Axis, X, Y, Depth);
        static_cast<void>(Depth);
        const float DX = PointerX - X;
        const float DY = PointerY - Y;
        const float Distance = DX * DX + DY * DY;
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            Best = Axis.Orientation;
        }
    }
    return Best;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    ENGINE CONTENT
//------------------------------------------------------------------------------------------------------------------------

inline std::filesystem::path ResolveEngineContentRoot(const std::filesystem::path& ExecutablePath)
{
    const auto Standing = [](const std::filesystem::path& Candidate)
    {
        return std::filesystem::exists(Candidate / "WhiteTeaService.codex") ||
               std::filesystem::exists(Candidate / "FontArchives");
    };

    const std::filesystem::path Starts[3] =
    {
        std::filesystem::current_path() / "EngineContent",
        ExecutablePath.parent_path() / "EngineContent",
        ExecutablePath.parent_path().parent_path() / "EngineContent"
    };

    for (const std::filesystem::path& Candidate : Starts)
        if (Standing(Candidate))
            return Candidate.lexically_normal();

    std::filesystem::path Walk = std::filesystem::current_path();
    for (std::uint32_t Step = 0u; Step < 8u; ++Step)
    {
        const std::filesystem::path Candidate = Walk / "EngineContent";
        if (Standing(Candidate))
            return Candidate.lexically_normal();

        if (!Walk.has_parent_path() || Walk.parent_path() == Walk)
            break;
        Walk = Walk.parent_path();
    }

    return (std::filesystem::current_path() / "EngineContent").lexically_normal();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    CODEX ACTIVATION
//------------------------------------------------------------------------------------------------------------------------

struct SharedCodexActivation
{
    bool                    Requested = false;
    bool                    Resolved = false;
    ActivatedWorkspaceScene Scene = {};
    Refusal                 Error = { RefusalReason::CapabilityAbsent, "no codex activation was requested" };
    std::filesystem::path   ScenePath = {};
};

inline bool ContentRecordIsCodexScene(const ContentRecord& Record)
{
    if (Record.Archive != ContentArchive::Arrangement || Record.Extension == nullptr)
        return false;

    return std::string(Record.Extension) == ".codex" || std::string(Record.Extension) == "codex";
}

inline SharedCodexActivation ConsumeSharedCodexActivation(ContentBrowserConfiguration& Applied,
                                                          const ContentLibrary& Library,
                                                          const std::filesystem::path& EngineContentRoot)
{
    SharedCodexActivation Result;

    if (Applied.ActivationRequested >= Library.RecordCount)
        return Result;

    Result.Requested = true;
    const ContentRecord& Requested = Library.Records[Applied.ActivationRequested];
    Applied.ActivationRequested = ContentLibrary::AbsentIndex;

    if (!ContentRecordIsCodexScene(Requested))
    {
        Result.Error = { RefusalReason::ContentUnsupported, "the selected content record is not a codex scene" };
        return Result;
    }

    const std::string Extension = Requested.Extension != nullptr && Requested.Extension[0] == '.'
                                ? std::string(Requested.Extension)
                                : "." + std::string(Requested.Extension != nullptr ? Requested.Extension : "");
    Result.ScenePath = EngineContentRoot / (std::string(Requested.Naming) + Extension);

    WorkspaceSceneActivation Activating;
    const Outcome<ActivatedWorkspaceScene> Activated = Activating.Open(Result.ScenePath.string(), EngineContentRoot.string());
    if (!Activated.Resolved)
    {
        Result.Error = Activated.Error;
        return Result;
    }

    Result.Scene = Activated.Resolve();
    Result.Resolved = true;
    Result.Error = {};
    return Result;
}

} // namespace Slate
