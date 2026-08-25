//============================================================================================================================================
//                                                   SHAREDVIEWPORTHOSTBRIDGE.H
//============================================================================================================================================
// 🧩 Shared host-side viewport support used by EditorHost, PaintHost and ParametricSketchHost.
//    The hosts stay standalone executables; this header keeps their common runtime decisions in one place.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "SlateDocument/Format/WorkspaceSceneActivation/Api/WorkspaceSceneActivation.h"
#include "SlateUI/Interface/ContentBrowserPanel/Api/ContentBrowserPanel.h"

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
