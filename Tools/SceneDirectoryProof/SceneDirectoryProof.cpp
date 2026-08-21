//============================================================================================================================================
//                                                          SCENEDIRECTORYPROOF.CPP
//============================================================================================================================================
// 🧩 Headless proof renderer for the EDITOR's scene directory — the editor's
//    real layout: a workspace window split into viewport / outliner /
//    properties leaves (WorkspacePanel + EditorPanel + PanelStructure), with
//    the scene-directory content (the GPU sky in the viewport leaf, the
//    outliner | details column, the properties | history pages) recorded by
//    SceneDirectoryPanel, and the history demand that fires ONCE per slider
//    drag (not per tick).
//
//    The harness drives the REAL panels through the REAL RecordingSurface and
//    rasterizes the recorded ImDrawList on the CPU, exactly like
//    Tools/TypographyProof — same vendor, same atlas, same pixels the
//    windowed hosts upload. The vendor dock is the one thing the harness
//    cannot run (it lives in the Vulkan-facing InterfaceExchange), so the
//    workspace tab strip is drawn as a static bar; everything below it is the
//    same recording the windowed host makes.
//
//    Scenarios (each writes one PNG under VisualProof/EditorScene/):
//      --shot=editor-overview    one workspace split into a viewport leaf (the
//                                real sky) and an outliner leaf (the scene
//                                directory with its details pane)
//      --shot=editor-sun-props   the workspace split three ways: viewport,
//                                outliner, and a properties leaf showing the
//                                Sun/Sky/Atmosphere slider cards
//      --shot=editor-after-drag  after one elevation drag: the sun rose in the
//                                viewport, and exactly ONE history demand was
//                                raised
//      --shot=editor-camera-fly  after W + right-drag look: the camera moved
//                                and yawed, the sun shifted across the viewport,
//                                and the lag toggle changed the displacement
//
//    Build (repository root, after ApplyImGuiPatches.py):
//      g++ -std=c++20 -O2 -DNDEBUG -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DGLFW_DLL \
//          -DGLFW_INCLUDE_NONE -I Engine -I . -I ExternalPackages/imgui \
//          -I ExternalPackages/glfw/include -I ExternalPackages/thorvg/inc \
//          -I _AgentScratch/Vulkan-Headers/include \
//          Tools/SceneDirectoryProof/SceneDirectoryProof.cpp \
//          Engine/SlateUI/Interface/WorkspacePanel/Source/WorkspacePanel.cpp \
//          Engine/SlateUI/Interface/EditorPanel/Source/EditorPanel.cpp \
//          Engine/SlateUI/Interface/PanelStructure/Source/PanelStructure.cpp \
//          Engine/SlateUI/Interface/SceneDirectoryPanel/Source/SceneDirectoryPanel.cpp \
//          Engine/SlateUI/Interface/ControlPanel/Source/ControlPanel.cpp \
//          Engine/SlateUI/Interface/ComponentSpecification/Source/ComponentSpecification.cpp \
//          Engine/SlateUI/Interface/InterfaceExchange/Source/RecordingSurface.cpp \
//          Engine/SlateUI/Interface/AppearanceSpecification/Source/AppearanceSpecification.cpp \
//          Engine/SlateUI/Interface/InteractionIndex/Source/InteractionIndex.cpp \
//          Engine/SlateUI/Interface/MotionIntegrator/Source/MotionIntegrator.cpp \
//          Engine/SlateUI/Interface/ThemeSpecification/Source/ThemeSpecification.cpp \
//          Engine/SlateUI/Interface/SymbolSpecification/Source/SymbolSpecification.cpp \
//          Engine/SlateUI/Interface/TextComponent/Source/FontLoader.cpp \
//          Engine/Application/EditorHost/Source/CameraRig.cpp \
//          Engine/Application/EditorHost/Source/SkyImage.cpp \
//          Engine/SlateCompute/Compute/AtmosphereIntegrator/Source/AtmosphereIntegrator.cpp \
//          Engine/SlateMath/Numeric/QuadratureIntegrator/Source/QuadratureIntegrator.cpp \
//          Engine/SlateMath/Numeric/ColourProjection/Source/ColourProjection.cpp \
//          Engine/SlateMath/Numeric/SpectralProjection/Source/SpectralProjection.cpp \
//          ExternalPackages/imgui/imgui.cpp ExternalPackages/imgui/imgui_draw.cpp \
//          ExternalPackages/imgui/imgui_tables.cpp ExternalPackages/imgui/imgui_widgets.cpp \
//          -o _AgentScratch/SceneDirectoryProof

#include "imgui.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ExternalPackages/stb/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "ExternalPackages/stb/stb_image.h"

#include "Application/EditorHost/Api/CameraRig.h"
#include "Application/EditorHost/Api/SkyImage.h"
#include "Contract/DeliveryContract.h"
#include "SlateCompute/Compute/AtmosphereIntegrator/Api/AtmosphereIntegrator.h"
#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "SlateUI/Interface/ComponentSpecification/Api/ComponentSpecification.h"
#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"
#include "SlateUI/Interface/EditorPanel/Api/EditorPanel.h"
#include "SlateUI/Interface/InteractionIndex/Api/InteractionIndex.h"
#include "SlateUI/Interface/PanelStructure/Api/PanelStructure.h"
#include "SlateUI/Interface/SceneDirectoryPanel/Api/SceneDirectoryPanel.h"
#include "SlateUI/Interface/WorkspacePanel/Api/WorkspacePanel.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"
#include "SlateUI/Interface/MotionIntegrator/Api/MotionIntegrator.h"
#include "SlateUI/Interface/SymbolSpecification/Api/SymbolSpecification.h"
#include "SlateUI/Interface/TextComponent/Api/FontLoader.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

using namespace Slate;

namespace
{

constexpr float ViewportWidth = 1280.0f;    // [px] - the editor's window
constexpr float ViewportHeight = 900.0f;    // [px]
constexpr double TickMilliseconds = 16.6;   // [ms] - a 60 Hz tick

//------------------------------------------------------------------------------------------------------------------------
//                                                       CPU RASTERIZER
//------------------------------------------------------------------------------------------------------------------------
// The same triangle rasterizer as Tools/TypographyProof.

struct Rasterizer
{
    std::vector<unsigned char> Pixels;
    int Width = 0;
    int Height = 0;

    bool Begin(int W, int H)
    {
        Width = W;
        Height = H;
        Pixels.assign(static_cast<std::size_t>(W) * static_cast<std::size_t>(H) * 4u, 0u);
        for (std::size_t Pixel = 0u; Pixel < Pixels.size(); Pixel += 4u)
        {
            Pixels[Pixel + 0u] = 6u;
            Pixels[Pixel + 1u] = 6u;
            Pixels[Pixel + 2u] = 8u;
            Pixels[Pixel + 3u] = 255u;
        }
        return true;
    }

    static void Blend(std::vector<unsigned char>& Target, std::size_t Offset,
                      float Red, float Green, float Blue, float Alpha)
    {
        const float Over = Alpha;
        const float Under = 1.0f - Alpha;
        unsigned char* Dst = &Target[Offset];
        const float DR = static_cast<float>(Dst[0]) / 255.0f;
        const float DG = static_cast<float>(Dst[1]) / 255.0f;
        const float DB = static_cast<float>(Dst[2]) / 255.0f;
        const float DA = static_cast<float>(Dst[3]) / 255.0f;
        Dst[0] = static_cast<unsigned char>(Red * Over * 255.0f + DR * Under * 255.0f + 0.5f);
        Dst[1] = static_cast<unsigned char>(Green * Over * 255.0f + DG * Under * 255.0f + 0.5f);
        Dst[2] = static_cast<unsigned char>(Blue * Over * 255.0f + DB * Under * 255.0f + 0.5f);
        Dst[3] = static_cast<unsigned char>((Alpha + DA * Under) * 255.0f + 0.5f);
    }

    void Triangle(const ImDrawVert& A, const ImDrawVert& B, const ImDrawVert& C,
                  bool Textured, const unsigned char* Atlas, int AtlasWidth, int AtlasHeight,
                  int ClipX0, int ClipY0, int ClipX1, int ClipY1)
    {
        const float Ax = A.pos.x, Ay = A.pos.y;
        const float Bx = B.pos.x, By = B.pos.y;
        const float Cx = C.pos.x, Cy = C.pos.y;

        const float Area = (Bx - Ax) * (Cy - Ay) - (By - Ay) * (Cx - Ax);
        if (Area == 0.0f)
            return;
        const float InvArea = 1.0f / Area;

        int MinX = static_cast<int>(std::floor(std::fmin(Ax, std::fmin(Bx, Cx))));
        int MinY = static_cast<int>(std::floor(std::fmin(Ay, std::fmin(By, Cy))));
        int MaxX = static_cast<int>(std::ceil(std::fmax(Ax, std::fmax(Bx, Cx))));
        int MaxY = static_cast<int>(std::ceil(std::fmax(Ay, std::fmax(By, Cy))));
        if (MinX < ClipX0) MinX = ClipX0;
        if (MinY < ClipY0) MinY = ClipY0;
        if (MaxX > ClipX1) MaxX = ClipX1;
        if (MaxY > ClipY1) MaxY = ClipY1;
        if (MinX >= MaxX || MinY >= MaxY)
            return;

        for (int Y = MinY; Y < MaxY; ++Y)
        {
            const float Py = static_cast<float>(Y) + 0.5f;
            for (int X = MinX; X < MaxX; ++X)
            {
                const float Px = static_cast<float>(X) + 0.5f;
                const float W0 = (Bx - Ax) * (Py - Ay) - (By - Ay) * (Px - Ax);
                const float W1 = (Cx - Bx) * (Py - By) - (Cy - By) * (Px - Bx);
                const float W2 = (Ax - Cx) * (Py - Cy) - (Ay - Cy) * (Px - Cx);
                const bool Inside = (W0 >= 0.0f && W1 >= 0.0f && W2 >= 0.0f) ||
                                    (W0 <= 0.0f && W1 <= 0.0f && W2 <= 0.0f);
                if (!Inside)
                    continue;

                const float T0 = W1 * InvArea;
                const float T1 = W2 * InvArea;
                const float T2 = W0 * InvArea;

                float Red = 0.0f, Green = 0.0f, Blue = 0.0f, Alpha = 0.0f;
                for (std::uint32_t V = 0u; V < 3u; ++V)
                {
                    const ImDrawVert& Vtx = (V == 0u) ? A : (V == 1u) ? B : C;
                    const float T = (V == 0u) ? T0 : (V == 1u) ? T1 : T2;
                    Red += static_cast<float>((Vtx.col >> 0) & 0xFFu) / 255.0f * T;
                    Green += static_cast<float>((Vtx.col >> 8) & 0xFFu) / 255.0f * T;
                    Blue += static_cast<float>((Vtx.col >> 16) & 0xFFu) / 255.0f * T;
                    Alpha += static_cast<float>((Vtx.col >> 24) & 0xFFu) / 255.0f * T;
                }

                if (Textured)
                {
                    float U = A.uv.x * T0 + B.uv.x * T1 + C.uv.x * T2;
                    float V = A.uv.y * T0 + B.uv.y * T1 + C.uv.y * T2;
                    const float SX = U * static_cast<float>(AtlasWidth) - 0.5f;
                    const float SY = V * static_cast<float>(AtlasHeight) - 0.5f;
                    const int IX = static_cast<int>(std::floor(SX));
                    const int IY = static_cast<int>(std::floor(SY));
                    const float FX = SX - static_cast<float>(IX);
                    const float FY = SY - static_cast<float>(IY);
                    const int PX0 = IX < 0 ? 0 : (IX > AtlasWidth - 2 ? AtlasWidth - 2 : IX);
                    const int PY0 = IY < 0 ? 0 : (IY > AtlasHeight - 2 ? AtlasHeight - 2 : IY);
                    const auto Texel = [&](int TX, int TY) -> std::uint32_t
                    {
                        const std::size_t Offset =
                            (static_cast<std::size_t>(TY) * static_cast<std::size_t>(AtlasWidth) +
                             static_cast<std::size_t>(TX)) * 4u;
                        return (static_cast<std::uint32_t>(Atlas[Offset + 3u]) << 24u) |
                               (static_cast<std::uint32_t>(Atlas[Offset + 2u]) << 16u) |
                               (static_cast<std::uint32_t>(Atlas[Offset + 1u]) << 8u) |
                               static_cast<std::uint32_t>(Atlas[Offset + 0u]);
                    };
                    const std::uint32_t T00 = Texel(PX0, PY0);
                    const std::uint32_t T10 = Texel(PX0 + 1, PY0);
                    const std::uint32_t T01 = Texel(PX0, PY0 + 1);
                    const std::uint32_t T11 = Texel(PX0 + 1, PY0 + 1);
                    const auto MixByte = [&](std::uint32_t First, std::uint32_t Second, float F) -> float
                    {
                        return (static_cast<float>(First) + (static_cast<float>(Second) - static_cast<float>(First)) * F) / 255.0f;
                    };
                    const auto MixUnit = [&](float First, float Second, float F) -> float
                    {
                        return First + (Second - First) * F;
                    };
                    const float TR = MixUnit(MixByte((T00 >> 0) & 0xFFu, (T10 >> 0) & 0xFFu, FX),
                                             MixByte((T01 >> 0) & 0xFFu, (T11 >> 0) & 0xFFu, FX), FY);
                    const float TG = MixUnit(MixByte((T00 >> 8) & 0xFFu, (T10 >> 8) & 0xFFu, FX),
                                             MixByte((T01 >> 8) & 0xFFu, (T11 >> 8) & 0xFFu, FX), FY);
                    const float TB = MixUnit(MixByte((T00 >> 16) & 0xFFu, (T10 >> 16) & 0xFFu, FX),
                                             MixByte((T01 >> 16) & 0xFFu, (T11 >> 16) & 0xFFu, FX), FY);
                    const float TA = MixUnit(MixByte((T00 >> 24) & 0xFFu, (T10 >> 24) & 0xFFu, FX),
                                             MixByte((T01 >> 24) & 0xFFu, (T11 >> 24) & 0xFFu, FX), FY);
                    Red *= TR;
                    Green *= TG;
                    Blue *= TB;
                    Alpha *= TA;
                }

                const std::size_t Offset =
                    (static_cast<std::size_t>(Y) * static_cast<std::size_t>(Width) +
                     static_cast<std::size_t>(X)) * 4u;
                Blend(Pixels, Offset, Red, Green, Blue, Alpha);
            }
        }
    }

    // 📝 The harness's extra textures: identity -> RGBA8. The host registers the sky texture through
    //    the interface's Vulkan backend; the rasterizer resolves the same identity here.
    std::unordered_map<std::uintptr_t, std::vector<unsigned char>> ExtraTextures;

    bool Draw(const ImDrawList* List, const unsigned char* Atlas, int AtlasWidth, int AtlasHeight)
    {
        const ImDrawCmd* Commands = List->CmdBuffer.Data;
        const int CommandCount = static_cast<int>(List->CmdBuffer.Size);
        const ImDrawVert* Vertices = List->VtxBuffer.Data;
        const ImDrawIdx* Indices = List->IdxBuffer.Data;

        for (int CommandOrdinal = 0; CommandOrdinal < CommandCount; ++CommandOrdinal)
        {
            const ImDrawCmd& Command = Commands[CommandOrdinal];
            if (Command.UserCallback != nullptr)
                continue;

            const int ClipX0 = static_cast<int>(Command.ClipRect.x > 0.0f ? Command.ClipRect.x : 0.0f);
            const int ClipY0 = static_cast<int>(Command.ClipRect.y > 0.0f ? Command.ClipRect.y : 0.0f);
            const int ClipX1 = static_cast<int>(Command.ClipRect.z < static_cast<float>(Width)
                                                    ? Command.ClipRect.z : static_cast<float>(Width));
            const int ClipY1 = static_cast<int>(Command.ClipRect.w < static_cast<float>(Height)
                                                    ? Command.ClipRect.w : static_cast<float>(Height));
            if (ClipX0 >= ClipX1 || ClipY0 >= ClipY1)
                continue;

            const ImTextureID Identity = Command.GetTexID();
            const bool Textured = (Identity != (ImTextureID)0);
            const std::uint32_t PrimitiveCount = Command.ElemCount / 3u;

            // 🔴 Resolve the sampled texture: the font atlas by default, an extra texture by identity.
            const unsigned char* CommandAtlas = Atlas;
            int CommandAtlasWidth = AtlasWidth;
            int CommandAtlasHeight = AtlasHeight;
            const auto Extra = ExtraTextures.find(static_cast<std::uintptr_t>(Identity));
            if (Extra != ExtraTextures.end())
            {
                CommandAtlas = Extra->second.data();
                CommandAtlasWidth = 1024;
                CommandAtlasHeight = 576;
            }
            for (std::uint32_t Primitive = 0u; Primitive < PrimitiveCount; ++Primitive)
            {
                const ImDrawIdx I0 = Indices[static_cast<std::size_t>(Command.IdxOffset) +
                                             static_cast<std::size_t>(Primitive) * 3u + 0u];
                const ImDrawIdx I1 = Indices[static_cast<std::size_t>(Command.IdxOffset) +
                                             static_cast<std::size_t>(Primitive) * 3u + 1u];
                const ImDrawIdx I2 = Indices[static_cast<std::size_t>(Command.IdxOffset) +
                                             static_cast<std::size_t>(Primitive) * 3u + 2u];
                const ImDrawVert& A = Vertices[Command.VtxOffset + I0];
                const ImDrawVert& B = Vertices[Command.VtxOffset + I1];
                const ImDrawVert& C = Vertices[Command.VtxOffset + I2];
                Triangle(A, B, C, Textured, CommandAtlas, CommandAtlasWidth, CommandAtlasHeight,
                         ClipX0, ClipY0, ClipX1, ClipY1);
            }
        }
        return true;
    }
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE DRIVER
//------------------------------------------------------------------------------------------------------------------------
// The harness replicates the editor host's tick: the shared ledger advances once,
// the scene directory samples it, the workspace panel records its strip and body,
// the editor panel records the partition chrome, and the host's own content loop
// fills each leaf body — sky in the viewport leaf, outliner, properties.

struct SceneDriver
{
    ImGuiIO& IO;
    MotionIntegrator Motion;
    InteractionIndex Ledger;
    RecordingSurface Surface;
    WorkspacePanel Workspace;
    EditorPanel Editor;
    PanelStructure Partition;
    EditorPanelConfiguration Configuration;
    SceneDirectoryPanel SceneDirectory;
    SceneDirectoryContext Applied;
    FontLoader Fonts;

    static constexpr EntityRow EditorEntities[7] =
    {
        { "Level_01_City",           EntitySubject::Level,      0u, 0xFFFFFFFFu, 3u },
        { "Lighting",                EntitySubject::Grouping,   1u,  0u,         2u },
        { "Directional Light (Sun)", EntitySubject::Sun,        2u,  1u,         0u },
        { "Sky Atmosphere",          EntitySubject::Sky,        2u,  1u,         0u },
        { "Environment",             EntitySubject::Grouping,   1u,  0u,         1u },
        { "Post Process Volume",     EntitySubject::Actor,      2u,  4u,         0u },
        { "Editor Camera",           EntitySubject::Camera,     1u,  0u,         0u }
    };

    EntityRevision Revisions[8] = {};
    std::uint32_t RevisionCount = 0u;

    // 📝 The sky texture the host would upload: generated from the environment, registered in the
    //    rasterizer under a fixed identity, and handed to the scene directory as its texture identity.
    static constexpr std::uintptr_t SkyIdentity = 0x534B5931u;   // [-] - "SKY1", a fake descriptor handle
    AtmosphereIntegrator SkyIntegrator;
    std::vector<std::uint8_t> SkyPixels;
    EnvironmentConfiguration SkyPrevious;
    SkyCamera SkyCam;
    bool SkyReady = false;
    bool SkyEverGenerated = false;

    // 📝 The camera the host owns. The harness cannot run the seam, so it feeds the rig the same
    //    CameraCondition the seam would deliver: the simulation flags below stand in for held keys
    //    and the right-button look gesture.
    CameraRig FlyRig;
    bool  SimForwardHeld = false;
    bool  SimLookHeld    = false;
    float SimLookDeltaX  = 0.0f;
    float SimLookDeltaY  = 0.0f;

    SceneDriver() : IO(ImGui::GetIO()) {}

    bool Construct()
    {
        const char* FontRoot = "EngineContent/FontArchives";
        if (!Fonts.Discover(FontRoot).Resolved)
            return false;
        FontProfile Profile;
        std::strncpy(Profile.Family, "Inter", sizeof(Profile.Family) - 1u);
        if (!Fonts.Load(FontRoot, Profile, 1.0f).Resolved)
            return false;

        IO.Fonts->TexData->SetTexID((ImTextureID)(intptr_t)1);
        IO.Fonts->TexRef._TexData = IO.Fonts->TexData;

        ThemeSelection Selected;
        Selected.Current = ThemeSubject::Oled;
        ThemeProfile Appearance = ResolveTinted(1.0, 1.0, ViewportWidth, Selected);
        Surface.ApplyFontLoader(Fonts);
        Surface.ApplyTypographyScale(Appearance.TextScale);
        Surface.ApplyCornerScale(Appearance.CornerScale);

        if (!Ledger.Construct(Motion).Resolved)
            return false;
        if (!SceneDirectory.Construct(Ledger, Motion, Surface, Appearance).Resolved)
            return false;
        if (!Editor.Construct(Motion, Surface, Appearance).Resolved)
            return false;
        if (!Workspace.Construct(Surface, Appearance).Resolved)
            return false;

        Applied.EnvironmentPresented = true;
        Applied.Environment.SunElevation   = 35.0;
        Applied.Environment.SunAzimuth     = 120.0;
        Applied.Environment.SunIntensity   = 4.8;
        Applied.Environment.SunTemperature = 5500.0;
        Applied.Environment.SkyIntensity   = 1.0;
        Applied.Environment.SkyTurbidity   = 2.0;
        Applied.Environment.AtmosphereDensity = 1.0;
        Applied.Environment.AtmosphereScaleHeight = 1.0;
        Applied.EntityTaken = 2u;   // the sun, taken at bring-up

        // 📝 The camera row's options and the rig, exactly as the host declares them.
        Applied.DetailBits[6u] = 2u;
        Applied.CameraSpeed = 50.0;
        FlyRig.YawDegrees   = Applied.Environment.SunAzimuth - 20.0;
        FlyRig.PitchDegrees = 15.0;
        FlyRig.Snap();
        Applied.CameraPosition[0] = 0.0;
        Applied.CameraPosition[1] = 1.5;
        Applied.CameraPosition[2] = 0.0;
        Applied.CameraRotation[0] = FlyRig.YawDegrees;
        Applied.CameraRotation[1] = FlyRig.PitchDegrees;

        Revisions[0] = { "Level created", "Bracket_Rev4", "09:12", "A. Marner", 0u, RevisionSubject::Start };
        Revisions[1] = { "Sun angle relocated", "Pitch 35 deg", "10:05", "A. Marner", 2u, RevisionSubject::Relocate };
        RevisionCount = 2u;

        return true;
    }

    void Tick(float MouseX, float MouseY, bool Held, bool Arrived, bool Released)
    {
        IO.MousePos = ImVec2(MouseX, MouseY);
        IO.MouseDelta = ImVec2(0.0f, 0.0f);
        IO.MouseWheel = 0.0f;
        IO.DeltaTime = static_cast<float>(TickMilliseconds / 1000.0);
        if (Arrived)
            IO.AddMouseButtonEvent(0, true);
        else if (Released)
            IO.AddMouseButtonEvent(0, false);

        ImGui::NewFrame();

        Discard(Surface.Adopt(RecordingSurface::ShellLayer::Beneath));
        Motion.Advance(TickMilliseconds);
        Ledger.Advance(Surface.Pointer(), TickMilliseconds);
        SceneDirectory.Advance(Surface.Pointer(), TickMilliseconds);
        Editor.Advance(Surface.Pointer(), TickMilliseconds);

        // 📝 The fly camera — the host's own step, fed by the simulation flags instead of the seam.
        {
            CameraCondition FlyInput;
            FlyInput.ForwardHeld  = SimForwardHeld;
            FlyInput.LookHeld     = SimLookHeld;
            FlyInput.LookDeltaX   = SimLookDeltaX;
            FlyInput.LookDeltaY   = SimLookDeltaY;

            CameraSettings FlySettings;
            FlySettings.FlySpeed    = Applied.CameraSpeed;
            FlySettings.LagEnabled  = (Applied.DetailBits[6u] & 2u) != 0u;
            FlySettings.InvertPitch = (Applied.DetailBits[6u] & 4u) != 0u;

            FlyRig.Advance(TickMilliseconds / 1000.0, FlyInput, FlySettings);

            Applied.ViewportSkyCamera.AzimuthDegrees    = static_cast<float>(FlyRig.LaggedYawDegrees);
            Applied.ViewportSkyCamera.ElevationDegrees  = static_cast<float>(FlyRig.LaggedPitchDegrees);
            Applied.ViewportSkyCamera.FieldOfViewDegrees = 60.0f;
            Applied.CameraPosition[0] = FlyRig.LaggedPosition[0];
            Applied.CameraPosition[1] = FlyRig.LaggedPosition[1];
            Applied.CameraPosition[2] = FlyRig.LaggedPosition[2];
            Applied.CameraRotation[0] = FlyRig.LaggedYawDegrees;
            Applied.CameraRotation[1] = FlyRig.LaggedPitchDegrees;
        }

        // 📝 The host regenerates the sky when the environment changed (at most once per drag) and
        //    hands the identity to the scene directory; the viewport leaf draws it.
        if (Applied.EnvironmentPresented &&
            (!SkyEverGenerated ||
             std::memcmp(&SkyPrevious, &Applied.Environment, sizeof(EnvironmentConfiguration)) != 0))
        {
            SkyCam.AzimuthDegrees   = Applied.Environment.SunAzimuth - 20.0;
            SkyCam.ElevationDegrees = 15.0;
            if (GenerateSkyImage(SkyIntegrator, Applied.Environment, SkyCam,
                                 1024u, 576u, SkyPixels).Resolved)
            {
                Applied.SkyTextureIdentity = SkyIdentity;
                Applied.ViewportSkyCamera.AzimuthDegrees = static_cast<float>(SkyCam.AzimuthDegrees);
                Applied.ViewportSkyCamera.ElevationDegrees = static_cast<float>(SkyCam.ElevationDegrees);
                Applied.ViewportSkyCamera.FieldOfViewDegrees = static_cast<float>(SkyCam.FieldOfViewDegrees);
                SkyReady = true;
            }
            SkyPrevious = Applied.Environment;
            SkyEverGenerated = true;
        }

        // 📝 The workspace: strip and body first, then the editor chrome, then the leaf content —
        //    the same order the editor host records.
        const PlaneExtent Whole = Spanning(0.0f, 0.0f, ViewportWidth, ViewportHeight);
        Discard(Workspace.Record(Whole, "Workspace 1"));

        // 📐 A static tab strip over the workspace's own strip, standing in for the vendor dock the
        //    harness cannot run.
        const PlaneExtent WorkspaceStrip = Workspace.Strip();
        if (WorkspaceStrip.Height() > 0.0f)
        {
            Surface.TextRun(WorkspaceStrip.MinimumX + 12.0f,
                             WorkspaceStrip.MinimumY + (WorkspaceStrip.Height()
                                                        - 12.5f) * 0.5f,
                             Covering(0xE6E6E6u), "Workspace 1", 12.5f, 0.0f, true);
        }

        // 🔴 The popups are deferred, exactly as the editor host defers them, so the leaf content
        //    records beneath the split/subject menus instead of painting over them.
        Discard(Editor.Record(Workspace.Body(), Partition, Configuration, 0u, true));

        for (std::uint32_t Leaf = 0u; Leaf < Editor.LeafCount(); ++Leaf)
        {
            const PlaneExtent LeafBody = Editor.LeafBody(Leaf);

            switch (Editor.LeafSubject(Leaf))
            {
                case PanelSubject::Viewport:
                    SceneDirectory.RecordViewportSky(LeafBody, Applied);
                    SceneDirectory.RecordGroundGrid(LeafBody, Applied);
                    break;
                case PanelSubject::Outliner:
                    SceneDirectory.RecordOutliner(LeafBody, Applied, EditorEntities, 7u);
                    break;
                case PanelSubject::Properties:
                    SceneDirectory.RecordProperties(LeafBody, Applied, EditorEntities, 7u,
                                                    Revisions, RevisionCount);
                    break;
                default:
                    break;
            }
        }

        Editor.RecordDeferredPopups(Partition, Configuration);

        Surface.Retire();

        // 📝 The host drains the drag-end demand exactly once per drag.
        if (Applied.RevisionDemandSlot.Standing && RevisionCount < 8u)
        {
            EntityRevision& Written = Revisions[RevisionCount++];
            Written.Description = Applied.RevisionDemandSlot.Caption;
            Written.Secondary   = Applied.RevisionDemandSlot.Secondary;
            Written.TimeRun     = "now";
            Written.Author      = "Artist";
            Written.Against     = Applied.RevisionDemandSlot.Against;
            Written.Classified  = RevisionSubject::Parameter;
            Applied.RevisionDemandSlot = {};
        }

        ImGui::Render();
    }

    void Settle(int Frames)
    {
        for (int Ordinal = 0; Ordinal < Frames; ++Ordinal)
            Tick(640.0f, 450.0f, false, false, false);
    }

    void Tap(float MouseX, float MouseY)
    {
        Tick(MouseX, MouseY, true, true, false);
        Tick(MouseX, MouseY, true, false, false);
        Tick(MouseX, MouseY, false, false, true);
    }

    // 📐 The workspace partition each scenario presents:
    //    overview  :  viewport | outliner
    //    props     :  viewport | (outliner over properties)
    void ApplyPartition(bool WithProperties)
    {
        Partition.Construct(PanelSubject::Viewport);
        Discard(Partition.Divide(PanelStructure::RootOrdinal, PanelDivisionAxis::X,
                                 PanelDivisionSide::Maximum));
        const Outcome<PanelRecord> Right = Partition.Current(PanelStructure::RootOrdinal);
        const std::uint32_t RightLeaf = Right.Resolved ? Right.Resolve().Maximum : 1u;
        Discard(Partition.Assign(RightLeaf, PanelSubject::Outliner));

        if (WithProperties)
        {
            Discard(Partition.Divide(RightLeaf, PanelDivisionAxis::Y, PanelDivisionSide::Maximum));
            const Outcome<PanelRecord> RightRecord = Partition.Current(RightLeaf);
            const std::uint32_t LowerLeaf = RightRecord.Resolved ? RightRecord.Resolve().Maximum : 3u;
            Discard(Partition.Assign(LowerLeaf, PanelSubject::Properties));
        }
    }

    bool Capture(const char* Path, const unsigned char* Atlas, int AtlasWidth, int AtlasHeight)
    {
        const unsigned char* LiveAtlas = static_cast<const unsigned char*>(IO.Fonts->TexData->Pixels);
        const int LiveAtlasWidth = IO.Fonts->TexData->Width;
        const int LiveAtlasHeight = IO.Fonts->TexData->Height;

        Rasterizer Out;
        Out.Begin(static_cast<int>(ViewportWidth), static_cast<int>(ViewportHeight));
        if (SkyReady)
            Out.ExtraTextures[SkyIdentity] = SkyPixels;
        const ImDrawData* Data = ImGui::GetDrawData();
for (int ListOrdinal = 0; ListOrdinal < Data->CmdListsCount; ++ListOrdinal)
        {
            if (!Out.Draw(Data->CmdLists[ListOrdinal], LiveAtlas, LiveAtlasWidth, LiveAtlasHeight))
                return false;
        }
        if (stbi_write_png(Path, Out.Width, Out.Height, 4, Out.Pixels.data(), Out.Width * 4) == 0)
            return false;
        std::fprintf(stderr, "[proof] wrote %s (%dx%d)\n", Path, Out.Width, Out.Height);
        return true;
    }
};

bool RunShot(SceneDriver& Driver, const char* OutputPath, const char* Scenario,
             const unsigned char* Atlas, int AtlasWidth, int AtlasHeight)
{
    std::fprintf(stderr, "\n== %s ==\n", Scenario);

    if (std::strcmp(Scenario, "editor-overview") == 0)
    {
        Driver.ApplyPartition(false);
        Driver.Settle(20);
    }
    else if (std::strcmp(Scenario, "editor-sun-props") == 0)
    {
        Driver.ApplyPartition(true);
        Driver.Settle(20);
    }
    else if (std::strcmp(Scenario, "editor-camera-fly") == 0)
    {
        Driver.ApplyPartition(false);
        Driver.Settle(20);

        const double YawBefore   = Driver.FlyRig.LaggedYawDegrees;
        const double PitchBefore = Driver.FlyRig.LaggedPitchDegrees;
        const double PositionBefore[3] = { Driver.FlyRig.LaggedPosition[0],
                                           Driver.FlyRig.LaggedPosition[1],
                                           Driver.FlyRig.LaggedPosition[2] };

        // 📐 Forty ticks of W + right-drag look: the camera flies forward along its yaw and turns with
        //    the look gesture. The drag is 2 px per tick rightward and 0.5 px per tick downward.
        Driver.SimForwardHeld = true;
        Driver.SimLookHeld    = true;
        Driver.SimLookDeltaX  = 2.0f;
        Driver.SimLookDeltaY  = 0.5f;
        for (int Step = 0; Step < 40; ++Step)
            Driver.Tick(640.0f, 450.0f, false, false, false);
        Driver.SimForwardHeld = false;
        Driver.SimLookHeld    = false;
        Driver.SimLookDeltaX  = 0.0f;
        Driver.SimLookDeltaY  = 0.0f;
        Driver.Settle(10);

        const double YawAfter   = Driver.FlyRig.LaggedYawDegrees;
        const double PitchAfter = Driver.FlyRig.LaggedPitchDegrees;
        const double PositionAfter[3] = { Driver.FlyRig.LaggedPosition[0],
                                          Driver.FlyRig.LaggedPosition[1],
                                          Driver.FlyRig.LaggedPosition[2] };
        const double Travelled = std::sqrt((PositionAfter[0] - PositionBefore[0])
                                         * (PositionAfter[0] - PositionBefore[0])
                                         + (PositionAfter[1] - PositionBefore[1])
                                         * (PositionAfter[1] - PositionBefore[1])
                                         + (PositionAfter[2] - PositionBefore[2])
                                         * (PositionAfter[2] - PositionBefore[2]));

        std::fprintf(stderr, "[assert] yaw %.1f -> %.1f, pitch %.1f -> %.1f\n",
                     YawBefore, YawAfter, PitchBefore, PitchAfter);
        std::fprintf(stderr, "[assert] travelled %.1f m (lag on)\n", Travelled);

        if (YawAfter <= YawBefore + 4.0)
        {
            std::fprintf(stderr, "[FAIL] the look gesture did not yaw the camera\n");
            return false;
        }
        if (Travelled < 5.0)
        {
            std::fprintf(stderr, "[FAIL] W did not move the camera\n");
            return false;
        }

        // 📐 The lag's own proof: with the lag DISABLED, the same forty ticks must travel further —
        //    the lagged camera is still catching up, the unlagged one is at the target.
        Driver.Applied.DetailBits[6u] &= ~2u;   // [-] - camera lag off
        const double LaglessBefore[3] = { Driver.FlyRig.LaggedPosition[0],
                                          Driver.FlyRig.LaggedPosition[1],
                                          Driver.FlyRig.LaggedPosition[2] };

        Driver.SimForwardHeld = true;
        for (int Step = 0; Step < 40; ++Step)
            Driver.Tick(640.0f, 450.0f, false, false, false);
        Driver.SimForwardHeld = false;
        Driver.Settle(10);

        const double LaglessAfter[3] = { Driver.FlyRig.LaggedPosition[0],
                                         Driver.FlyRig.LaggedPosition[1],
                                         Driver.FlyRig.LaggedPosition[2] };
        const double LaglessTravelled = std::sqrt((LaglessAfter[0] - LaglessBefore[0])
                                                * (LaglessAfter[0] - LaglessBefore[0])
                                                + (LaglessAfter[1] - LaglessBefore[1])
                                                * (LaglessAfter[1] - LaglessBefore[1])
                                                + (LaglessAfter[2] - LaglessBefore[2])
                                                * (LaglessAfter[2] - LaglessBefore[2]));

        std::fprintf(stderr, "[assert] travelled %.1f m (lag off)\n", LaglessTravelled);
        if (LaglessTravelled <= Travelled)
        {
            std::fprintf(stderr, "[FAIL] the camera lag did not lag\n");
            return false;
        }

        // 📐 The ground grid's own proof: the rendered leaf must contain the lattice — light grey-blue
        //    lines over the dark ground, drawn by the panel's perspective projector. The capture is
        //    repeated here so the PNG stands for the check, and the PNG itself is scanned for the ink.
        if (!Driver.Capture(OutputPath, Atlas, AtlasWidth, AtlasHeight))
            return false;
        {
            int ReadWidth = 0;
            int ReadHeight = 0;
            int ReadChannels = 0;
            unsigned char* ReadPixels = stbi_load(OutputPath, &ReadWidth, &ReadHeight, &ReadChannels, 4);

            if (ReadPixels == nullptr)
            {
                std::fprintf(stderr, "[FAIL] the captured PNG would not read back\n");
                return false;
            }

            // 📐 The lattice ink (0x9AA6B8 at 0.28/0.55 coverage over the (10,8,7) ground) resolves to
            //    ~(50,52,56) fine and ~(89,95,104) coarse — grey-blue, brighter than the ground,
            //    never confused with the sky's saturated blues or the panels' chrome.
            std::uint32_t GridPixels = 0u;
            for (int Ordinal = 0; Ordinal < ReadWidth * ReadHeight; ++Ordinal)
            {
                const int R = ReadPixels[Ordinal * 4u + 0u];
                const int G = ReadPixels[Ordinal * 4u + 1u];
                const int B = ReadPixels[Ordinal * 4u + 2u];

                if (R > 38 && R < 130 && B > R && (B - R) < 22 &&
                    std::abs(R - G) < 8 && std::abs(G - B) < 14)
                    ++GridPixels;
            }

            stbi_image_free(ReadPixels);

            std::fprintf(stderr, "[assert] grid pixels in render: %u\n", GridPixels);
            if (GridPixels < 500u)
            {
                std::fprintf(stderr, "[FAIL] the ground grid did not draw\n");
                return false;
            }
        }
    }
    else if (std::strcmp(Scenario, "editor-after-drag") == 0)
    {
        Driver.ApplyPartition(true);
        Driver.Settle(20);

        // 📐 The elevation slider: the first row of the Sun card inside the properties leaf. The
        //    geometry is the panel's own — the leaf body, then the pane header, the tab strip, the
        //    card header, and the row padding — so the drag lands on the real slider wherever the
        //    partition placed it.
        const PlaneExtent Body = Driver.Editor.LeafBody(2u);
        ThemeSelection Sel;
        Sel.Current = ThemeSubject::Oled;
        const ThemeProfile Resolved = ResolveTinted(1.0, 1.0, ViewportWidth, Sel);
        const float AppliedFactor = static_cast<float>(Resolved.Measure.DisplayScale)
                                  * Resolved.ControlMeasure.ArtistFactor;
        const ShellMetric Scaled = ScaleShellLengths(AppliedFactor);

        const float Pad = Scaled.PanePad;

        // 📐 The Sun card is the SECOND card: the Transform card (three rows) precedes it, so the
        //    elevation row sits one card deeper than the naive layout suggests. The geometry below
        //    walks the panel's own sweep: pages begin after the pane header and the tab strip, the
        //    Transform card occupies one card height, then the Sun card's first row is the elevation.
        const float PagesY = Body.MinimumY + Scaled.HeaderHeight + Scaled.ComponentY;
        float Sweep = PagesY + Pad;
        const float TransformBottom = Sweep + Scaled.ComponentY + (3.0f * Scaled.RowHeight + Pad * 2.0f);
        Sweep = TransformBottom + Pad * 0.85f;

        const float SliderY = Sweep + Scaled.ComponentY + Pad + Scaled.RowHeight * 0.5f;
        const float TrackX0 = Body.MinimumX + Pad * 1.5f + 6.0f;
        const float TrackX1 = Body.MaximumX - Pad * 1.5f - 6.0f;

        // 📐 The press lands on the thumb's own position (elevation 35 of 0…90), so the drag reads as
        //    a continuous move rather than a jump; the drag then runs to 80 % of the track.
        const float ThumbX = TrackX0 + (35.0 / 90.0) * (TrackX1 - TrackX0);
        const float ReleaseX = TrackX0 + 0.55f * (TrackX1 - TrackX0);

        const std::uint32_t Before = Driver.RevisionCount;

        Driver.Tick(ThumbX, SliderY, true, true, false);
        for (int Step = 0; Step < 40; ++Step)
        {
            const float T = static_cast<float>(Step) / 39.0f;
            Driver.Tick(ThumbX + (ReleaseX - ThumbX) * T, SliderY, true, false, false);
        }
        Driver.Tick(ReleaseX, SliderY, false, false, true);
        Driver.Settle(10);

        std::fprintf(stderr, "[assert] revisions before=%u after=%u demand=%s\n",
                     Before, Driver.RevisionCount,
                     Driver.Applied.RevisionDemandSlot.Standing ? "still standing" : "drained");
        std::fprintf(stderr, "[assert] sun elevation after drag: %.1f\n",
                     Driver.Applied.Environment.SunElevation);
        if (Driver.RevisionCount != Before + 1u)
        {
            std::fprintf(stderr, "[FAIL] expected exactly one revision per drag\n");
            return false;
        }
        if (Driver.Applied.Environment.SunElevation < 50.0)
        {
            std::fprintf(stderr, "[FAIL] the elevation slider did not move the sun enough\n");
            return false;
        }

        // 📐 The raised sun now stands ABOVE the camera's 60° frustum (61.4° vs the 15° pitch) — the
        //    physically correct answer to a real fly camera. The artist looks up to see it: the look
        //    gesture drags the pointer UP (negative Y) and the camera pitches up, bringing the sun
        //    back into the viewport. This doubles as the look-gesture's own proof.
        const double PitchBefore = Driver.FlyRig.LaggedPitchDegrees;

        Driver.SimLookHeld = true;
        Driver.SimLookDeltaX = 0.0f;
        Driver.SimLookDeltaY = -6.0f;
        for (int Step = 0; Step < 35; ++Step)
            Driver.Tick(640.0f, 450.0f, false, false, false);
        Driver.SimLookHeld = false;
        Driver.SimLookDeltaY = 0.0f;
        Driver.Settle(10);

        const double PitchAfter = Driver.FlyRig.LaggedPitchDegrees;

        std::fprintf(stderr, "[assert] look gesture pitched the camera %.1f -> %.1f\n",
                     PitchBefore, PitchAfter);

        if (PitchAfter <= PitchBefore + 15.0)
        {
            std::fprintf(stderr, "[FAIL] the look gesture did not pitch the camera up\n");
            return false;
        }

        if (!Driver.Capture(OutputPath, Atlas, AtlasWidth, AtlasHeight))
            return false;

        // 📐 The sun must be back in the viewport leaf: warm, bright, in the upper sky.
        {
            int ReadWidth = 0;
            int ReadHeight = 0;
            int ReadChannels = 0;
            unsigned char* ReadPixels = stbi_load(OutputPath, &ReadWidth, &ReadHeight, &ReadChannels, 4);

            if (ReadPixels == nullptr)
            {
                std::fprintf(stderr, "[FAIL] the captured PNG would not read back\n");
                return false;
            }

            std::uint32_t SunPixels = 0u;
            for (int Y = 0; Y < ReadHeight; ++Y)
            {
                for (int X = 0; X < 637; ++X)
                {
                    const std::size_t Offset = (static_cast<std::size_t>(Y) * ReadWidth + X) * 4u;
                    const int R = ReadPixels[Offset + 0u];
                    const int G = ReadPixels[Offset + 1u];
                    const int B = ReadPixels[Offset + 2u];

                    if (R > 200 && G > 195 && B < 225 && (R - B) > 40)
                        ++SunPixels;
                }
            }

            stbi_image_free(ReadPixels);

            std::fprintf(stderr, "[assert] sun pixels in viewport after look-up: %u\n", SunPixels);
            if (SunPixels < 100u)
            {
                std::fprintf(stderr, "[FAIL] the sun is not in the viewport after looking up\n");
                return false;
            }
        }
    }
    else
    {
        std::fprintf(stderr, "unknown scenario %s\n", Scenario);
        return false;
    }

    return Driver.Capture(OutputPath, Atlas, AtlasWidth, AtlasHeight);
}

} // namespace

int main(int ArgumentCount, char** Arguments)
{
    std::string OutputDirectory = "VisualProof/EditorScene";
    std::string Scenario = "";

    for (int Ordinal = 1; Ordinal < ArgumentCount; ++Ordinal)
    {
        const std::string Arg = Arguments[Ordinal];
        if (Arg == "--out" && Ordinal + 1 < ArgumentCount)
            OutputDirectory = Arguments[++Ordinal];
        else if (Arg.rfind("--shot=", 0) == 0)
            Scenario = Arg.substr(7);
    }

    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO();
    IO.DisplaySize = ImVec2(ViewportWidth, ViewportHeight);
    IO.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    IO.IniFilename = nullptr;
    IO.LogFilename = nullptr;
    IO.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

    SceneDriver Driver;
    if (!Driver.Construct())
    {
        std::fprintf(stderr, "refused: harness construction\n");
        return 1;
    }
    IO.FontDefault = IO.Fonts->Fonts[0];

    unsigned char* AtlasPixels = nullptr;
    int AtlasWidth = 0;
    int AtlasHeight = 0;
    IO.Fonts->GetTexDataAsRGBA32(&AtlasPixels, &AtlasWidth, &AtlasHeight);
    IO.Fonts->TexData->SetTexID((ImTextureID)(intptr_t)1);
    IO.Fonts->TexRef._TexData = IO.Fonts->TexData;

    const char* Shots[] = {"editor-overview", "editor-sun-props", "editor-after-drag", "editor-camera-fly"};

    int Rendered = 0;
    for (const char* Shot : Shots)
    {
        if (!Scenario.empty() && Scenario != Shot)
            continue;
        const std::string Path = OutputDirectory + "/" + Shot + ".png";
        if (!RunShot(Driver, Path.c_str(), Shot, AtlasPixels, AtlasWidth, AtlasHeight))
            return 1;
        ++Rendered;
    }
    if (Rendered == 0)
    {
        std::fprintf(stderr, "no scenario matched; pass --shot=<name>\n");
        return 1;
    }

    std::fprintf(stderr, "\n[done] %d shot(s) rendered\n", Rendered);
    return 0;
}
