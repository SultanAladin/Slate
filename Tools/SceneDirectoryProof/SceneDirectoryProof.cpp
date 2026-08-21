//============================================================================================================================================
//                                                          SCENEDIRECTORYPROOF.CPP
//============================================================================================================================================
// 🧩 Headless proof renderer for the editor's scene directory: the shell's
//    viewport with the sun/sky/atmosphere, the outliner with the sun and sky
//    registered, the inspector's environment slider cards, and the history
//    demand that fires ONCE per slider drag (not per tick).
//
//    The harness drives the REAL GlobalShellPanel through the REAL
//    RecordingSurface and rasterizes the recorded ImDrawList on the CPU,
//    exactly like Tools/TypographyProof — same vendor, same atlas, same
//    pixels the windowed hosts upload.
//
//    Scenarios (each writes one PNG under VisualProof/EditorScene/):
//      --shot=editor-overview    the editor: viewport with sky + sun, outliner
//      --shot=editor-sun-props   the Sun row taken, inspector docked with the
//                                four sun slider cards
//      --shot=editor-after-drag  after one elevation drag: the sun moved, and
//                                exactly ONE history demand was raised
//
//    Build (repository root, after ApplyImGuiPatches.py):
//      g++ -std=c++20 -O2 -DNDEBUG -DWIN32_LEAN_AND_MEAN -DNOMINMAX -DGLFW_DLL \
//          -DGLFW_INCLUDE_NONE -I Engine -I . -I ExternalPackages/imgui \
//          -I ExternalPackages/glfw/include -I ExternalPackages/thorvg/inc \
//          Tools/SceneDirectoryProof/SceneDirectoryProof.cpp \
//          Engine/SlateUI/Interface/GlobalShellPanel/Source/GlobalShellPanel.cpp \
//          Engine/SlateUI/Interface/ControlPanel/Source/ControlPanel.cpp \
//          Engine/SlateUI/Interface/ComponentSpecification/Source/ComponentSpecification.cpp \
//          Engine/SlateUI/Interface/InterfaceExchange/Source/RecordingSurface.cpp \
//          Engine/SlateUI/Interface/AppearanceSpecification/Source/AppearanceSpecification.cpp \
//          Engine/SlateUI/Interface/InteractionIndex/Source/InteractionIndex.cpp \
//          Engine/SlateUI/Interface/MotionIntegrator/Source/MotionIntegrator.cpp \
//          Engine/SlateUI/Interface/ThemeSpecification/Source/ThemeSpecification.cpp \
//          Engine/SlateUI/Interface/SymbolSpecification/Source/SymbolSpecification.cpp \
//          Engine/SlateUI/Interface/TextComponent/Source/FontLoader.cpp \
//          ExternalPackages/imgui/imgui.cpp ExternalPackages/imgui/imgui_draw.cpp \
//          ExternalPackages/imgui/imgui_tables.cpp ExternalPackages/imgui/imgui_widgets.cpp \
//          -o _AgentScratch/SceneDirectoryProof

#include "imgui.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ExternalPackages/stb/stb_image_write.h"

#include "Contract/DeliveryContract.h"
#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "SlateUI/Interface/ComponentSpecification/Api/ComponentSpecification.h"
#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"
#include "SlateUI/Interface/GlobalShellPanel/Api/GlobalShellPanel.h"
#include "SlateUI/Interface/InteractionIndex/Api/InteractionIndex.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"
#include "SlateUI/Interface/MotionIntegrator/Api/MotionIntegrator.h"
#include "SlateUI/Interface/SymbolSpecification/Api/SymbolSpecification.h"
#include "SlateUI/Interface/TextComponent/Api/FontLoader.h"
#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
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

            const bool Textured = (Command.GetTexID() != (ImTextureID)0);
            const std::uint32_t PrimitiveCount = Command.ElemCount / 3u;
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
                Triangle(A, B, C, Textured, Atlas, AtlasWidth, AtlasHeight,
                         ClipX0, ClipY0, ClipX1, ClipY1);
            }
        }
        return true;
    }
};

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE DRIVER
//------------------------------------------------------------------------------------------------------------------------

struct SceneDriver
{
    ImGuiIO& IO;
    MotionIntegrator Motion;
    InteractionIndex Ledger;
    RecordingSurface Surface;
    GlobalShellPanel Shell;
    ShellContext Applied;
    FontLoader Fonts;

    static constexpr EntityRow EditorEntities[6] =
    {
        { "Level_01_City",           EntitySubject::Level,      0u, 0xFFFFFFFFu, 2u },
        { "Lighting",                EntitySubject::Grouping,   1u,  0u,         2u },
        { "Directional Light (Sun)", EntitySubject::Sun,        2u,  1u,         0u },
        { "Sky Atmosphere",          EntitySubject::Sky,        2u,  1u,         0u },
        { "Environment",             EntitySubject::Grouping,   1u,  0u,         1u },
        { "Post Process Volume",     EntitySubject::Actor,      2u,  4u,         0u }
    };

    EntityRevision Revisions[8] = {};
    std::uint32_t RevisionCount = 0u;

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
        if (!Shell.Construct(Ledger, Motion, Surface, Appearance).Resolved)
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
        Shell.Advance(Surface.Pointer(), TickMilliseconds);
        Discard(Shell.Record(Spanning(0.0f, 0.0f, ViewportWidth, ViewportHeight), Applied,
                               EditorEntities, 6u, nullptr, 0u, Revisions, RevisionCount));
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

    bool Capture(const char* Path, const unsigned char* Atlas, int AtlasWidth, int AtlasHeight)
    {
        const unsigned char* LiveAtlas = static_cast<const unsigned char*>(IO.Fonts->TexData->Pixels);
        const int LiveAtlasWidth = IO.Fonts->TexData->Width;
        const int LiveAtlasHeight = IO.Fonts->TexData->Height;

        Rasterizer Out;
        Out.Begin(static_cast<int>(ViewportWidth), static_cast<int>(ViewportHeight));
        const ImDrawData* Data = ImGui::GetDrawData();
        for (int ListOrdinal = 0; ListOrdinal < Data->CmdListsCount; ++ListOrdinal)
        {
            if (!Out.Draw(Data->CmdLists[ListOrdinal], LiveAtlas, LiveAtlasWidth, LiveAtlasHeight))
                return false;
        }
        if (stbi_write_png(Path, Out.Width, Out.Height, 4, Out.Pixels.data(), Out.Width * 4) == 0)
            return false;
        std::fprintf(stderr, "[proof] wrote %s (%dx%d)\\n", Path, Out.Width, Out.Height);
        return true;
    }
};

bool RunShot(SceneDriver& Driver, const char* OutputPath, const char* Scenario,
             const unsigned char* Atlas, int AtlasWidth, int AtlasHeight)
{
    std::fprintf(stderr, "\\n== %s ==\\n", Scenario);

    if (std::strcmp(Scenario, "editor-overview") == 0)
    {
        Driver.Applied.InspectorDocked = false;
        Driver.Settle(20);
    }
    else if (std::strcmp(Scenario, "editor-sun-props") == 0)
    {
        Driver.Applied.InspectorDocked = true;
        Driver.Applied.InspectorShown  = true;
        Driver.Applied.InspectorTab    = 0u;   // [-] - Properties
        Driver.Settle(20);
    }
    else if (std::strcmp(Scenario, "editor-after-drag") == 0)
    {
        Driver.Applied.InspectorDocked = true;
        Driver.Applied.InspectorShown  = true;
        Driver.Applied.InspectorTab    = 0u;
        Driver.Settle(20);

        // 📐 Drag the elevation slider from 35° to ~70°: the first slider row of the Sun card sits at
        //    y≈324 inside the docked inspector, its track spanning x≈650..1000.
        const float SliderX0 = 660.0f;
        const float SliderX1 = 960.0f;
        const float SliderY  = 326.0f;
        const std::uint32_t Before = Driver.RevisionCount;

        Driver.Tick(SliderX0, SliderY, true, true, false);
        for (int Step = 0; Step < 30; ++Step)
            Driver.Tick(SliderX0 + static_cast<float>(Step) * 10.0f, SliderY, true, false, false);
        Driver.Tick(SliderX1, SliderY, false, false, true);
        Driver.Settle(10);

        std::fprintf(stderr, "[assert] revisions before=%u after=%u demand=%s\\n",
                     Before, Driver.RevisionCount,
                     Driver.Applied.RevisionDemandSlot.Standing ? "still standing" : "drained");
        std::fprintf(stderr, "[assert] sun elevation after drag: %.1f\\n",
                     Driver.Applied.Environment.SunElevation);
        if (Driver.RevisionCount != Before + 1u)
        {
            std::fprintf(stderr, "[FAIL] expected exactly one revision per drag\\n");
            return false;
        }
    }
    else
    {
        std::fprintf(stderr, "unknown scenario %s\\n", Scenario);
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
        std::fprintf(stderr, "refused: harness construction\\n");
        return 1;
    }
    IO.FontDefault = IO.Fonts->Fonts[0];

    unsigned char* AtlasPixels = nullptr;
    int AtlasWidth = 0;
    int AtlasHeight = 0;
    IO.Fonts->GetTexDataAsRGBA32(&AtlasPixels, &AtlasWidth, &AtlasHeight);
    IO.Fonts->TexData->SetTexID((ImTextureID)(intptr_t)1);
    IO.Fonts->TexRef._TexData = IO.Fonts->TexData;

    const char* Shots[] = {"editor-overview", "editor-sun-props", "editor-after-drag"};

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
        std::fprintf(stderr, "no scenario matched; pass --shot=<name>\\n");
        return 1;
    }

    std::fprintf(stderr, "\\n[done] %d shot(s) rendered\\n", Rendered);
    return 0;
}
