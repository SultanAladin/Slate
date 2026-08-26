//============================================================================================================================================
//                                                             EDITORHOST.CPP
//============================================================================================================================================
// 🧩 The combined editor — every workspace subject in one host, with every device concern held by HostLifecycle.
//
// 🔴 LAYOUT RULE, READ BEFORE EDITING THIS HOST. The editor's layout is:
//      workspace windows (WorkspacePanel + WorkspaceIndex + the vendor dock)
//        → splittable panels (EditorPanel + PanelStructure: viewport | UV |
//          outliner | properties leaves, each with chrome and a footer)
//          → leaf content (SceneDirectoryPanel: the sky in a viewport leaf,
//            the outliner | details column in an outliner leaf, the
//            properties / camera-bookmark pages in a properties leaf).
//    The retired validation-shell prototype once duplicated the options rail,
//    texture-paint stack, drafting directory, and inspector. Runtime UI belongs
//    only to the standing panels named above; the editor's sky lives in the
//    viewport LEAF.

#define SLATE_EDITOR_HOST 1
#include "Foundation/DeliveryOutcome.h"
#include "Application/Api/SharedViewportHostBridge.h"
#include "Application/Api/SharedCadDrawingController.h"
#include "Application/Api/SharedCadWorkspaceRuntime.h"
#include "Application/Api/ParametricWorkspaceBridge.h"
#include "Application/Api/MaterialLayerStackBridge.h"
#include "Application/Api/SketchSceneDirectoryBridge.h"
#include "SlateScene/Scene/EditorCameraComponent/Api/EditorCameraComponent.h"
#include "SlateScene/Scene/AtmosphereComponent/Api/AtmosphereComponent.h"
#include "SlateScene/Scene/DirectionalLightComponent/Api/DirectionalLightComponent.h"
#include "SlateUI/Interface/ContentBrowserPanel/Api/ContentBrowserPanel.h"
#include "SlateUI/Interface/ControlCentrePanel/Api/ControlCentrePanel.h"
#include "SlateUI/Interface/ThemeInterchange/Api/ThemeInterchange.h"
#include "SlateUI/Interface/EditorPanel/Api/EditorPanel.h"
#include "SlateUI/Interface/TexturePaintPanel/Api/TexturePaintPanel.h"
#include "SlateUI/Interface/ParametricWorkspace/Api/ParametricWorkspacePanel.h"
#include "SlateUI/Interface/ParametricTools/Api/ParametricToolsPanel.h"
#include "SlateUI/Interface/SceneDirectoryPanel/Api/SceneDirectoryPanel.h"
#include "SlateUI/Interface/ViewportSequence/Api/ViewportSequence.h"
#include "SlateUI/Interface/WorkspacePanel/Api/WorkspaceIndex.h"
#include "SlateVulkan/Device/HostLifecycle/Api/HostLifecycle.h"
#include "SlateVulkan/Device/WorkspaceOverlayPass/Api/WorkspaceOverlayPass.h"
#include "SlateVulkan/Device/ShaderCodec/Api/ShaderCodec.h"
#include "SlateVulkan/Device/AtmospherePresentationSurface/Api/AtmospherePresentationSurface.h"
#include "SlateCompute/Compute/MaterialTextureExport/Api/MaterialTextureExport.h"
#include "SlateCompute/Compute/GeometryDeviceExchange/Api/GeometryDeviceExchange.h"
#include "SlateCompute/Compute/GeometryRenderingExchange/Api/GeometryRenderingExchange.h"
#include "SlateCompute/Compute/VisibilityIndex/Api/VisibilityIndex.h"
#include "SlateDocument/Document/GeometryInterchange/Api/GeometryFileInterchange.h"
#include "SlateDocument/Document/IntakeIndex/Api/IntakeIndex.h"
#include "SlateDocument/Document/MaterialSpecification/Api/MaterialSpecification.h"
#include "SlateDocument/Document/PopulationIndex/Api/PopulationIndex.h"
#include "SlateDocument/Format/CodexInterchange/Api/CodexInterchange.h"
#include "SlateDocument/Format/CodexInterchange/Api/WorkspaceCodex.h"
#include "SlateDocument/Format/WorkspaceSceneActivation/Api/WorkspaceSceneActivation.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <vector>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <unistd.h>
#endif
#include <cstring>
#include <system_error>

//------------------------------------------------------------------------------------------------------------------------
//                                                          FIGURES
//------------------------------------------------------------------------------------------------------------------------

namespace
{

using namespace Slate;

constexpr std::uint32_t InitialWidth  = 1280u;   // [px]
constexpr std::uint32_t InitialHeight = 720u;    // [px]

constexpr const char* WindowTitle = "Slate \u2014 Editor";
constexpr const char* HostName    = "EditorHost";

std::filesystem::path HomeProfilePath()
{
#if defined(_WIN32)
    char* Home = nullptr;
    std::size_t Count = 0u;
    if (_dupenv_s(&Home, &Count, "USERPROFILE") == 0 && Home != nullptr && Count > 1u)
    {
        std::filesystem::path Result = Home;
        std::free(Home);
        return Result;
    }
    if (Home != nullptr) std::free(Home);
    return {};
#else
    const char* Home = std::getenv("HOME");
    return (Home != nullptr && Home[0] != '\0') ? std::filesystem::path(Home) : std::filesystem::path{};
#endif
}

/// 🧩 Enumerates a host-approved import folder for the browser; SlateUI receives names only, never filesystem authority.
void PopulateImportDirectory(ContentBrowserConfiguration& Browser, const std::filesystem::path& Requested)
{
    std::error_code Error;
    std::filesystem::path Resolved = Requested;
    if (Requested == "Home")
    {
        const std::filesystem::path Home = HomeProfilePath();
        if (!Home.empty()) Resolved = Home;
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
        Written.Supported = Written.Directory || Extension == ".codex" || Extension == ".sketch" || Extension == ".pigment";
    }
}

// 📝 The workspace ground the interface is recorded over. Stated here because it is the one visual decision
//    this host makes; everything else it presents belongs to a panel.
//------------------------------------------------------------------------------------------------------------------------
//                                                      THE THREE CEILINGS
//------------------------------------------------------------------------------------------------------------------------

// 🔴 Applying the content browser in the south drawer crossed all three of the budgets that have each, at
//    least once, taken a host down with no window and no log line. They are asserted here so a fourth panel
//    cannot repeat any of them silently.

// ① EASED INTERPOLANTS. `ControlIndex::Register` draws two fades per control, and the integrator's supply
//    is shared by every index in the process — the browser's private index does not get its own pool.
constexpr std::uint32_t EasesPerControl = 2u;
constexpr std::uint32_t CentreControls  = ControlCentrePanel::ControlCapacity;
constexpr std::uint32_t BrowserControls = ContentBrowserPanel::RegistrationDemand;
constexpr std::uint32_t EditorControls  = PanelStructure::RecordLimit * EditorPanel::ControlsPerRecord;
constexpr std::uint32_t SceneControls   = SceneDirectoryPanel::RegistrationDemand
                                        + TexturePaintPanel::RegistrationDemand;
constexpr std::uint32_t ParametricControls = 4u + ParametricWorkspaceContext::RowLimit * 2u
                                           + 1u + ParametricToolsContext::BandLimit
                                           + ParametricToolsContext::TileLimit;
constexpr std::uint32_t BareEases       = 9u + 1u + 1u + 4u; // [-] - centre, shell, and transfer/export rails

constexpr std::uint32_t DemandedEases =
    ((CentreControls + BrowserControls + EditorControls + SceneControls + ParametricControls)
     * EasesPerControl) + BareEases;

static_assert(DemandedEases <= MotionIntegrator::EaseCapacity,
              "this host's panels demand more eased interpolants than the integrator holds — the panel "
              "constructed last is rejected mid-registration and the host exits before its first frame; raise "
              "MotionIntegrator::EaseCapacity or reduce a panel's control count");

// ② INDEX SLOTS. Counted per index, not per host. The browser is the only owner of its own
//    `BrowserInteraction`, so only its demand is weighed here; the Control Centre answers to its private index.
static_assert(BrowserControls <= ControlIndex::ControlCapacity,
              "the content browser registers more controls than one ControlIndex holds — Construct is "
              "rejected with \"no further control slot\" and the south drawer opens onto blank ground");

static_assert(SceneControls <= ControlIndex::ControlCapacity,
              "the scene directory registers more controls than one ControlIndex holds — Construct is "
              "rejected with \"no further control slot\" and the editor opens without its scene directory");

static_assert(ParametricControls <= ControlIndex::ControlCapacity,
              "the sketch directory and parametric tools register more controls than one ControlIndex holds — "
              "the editor cannot add the sketch panels to its dropdown safely");

// ③ AUTOMATIC STORAGE. A Windows thread is given one megabyte and a refusal here is not a refusal at all:
//    the guard page is touched in the prologue, so the process dies before a statement can report anything.
//    Linux hands out eight megabytes, which is exactly why no gate here can catch it.
constexpr std::size_t WindowsThreadStack = 1048576u;                  // [B] - the shipped linker default
constexpr std::size_t AutomaticLimit   = WindowsThreadStack / 4u;   // [B] - a quarter, leaving room to call

constexpr std::size_t AutomaticUiBytes = sizeof(ShaderCodec) + sizeof(WorkspaceOverlayPass);

static_assert(AutomaticUiBytes <= AutomaticLimit,
              "this host's automatic UI members no longer fit a quarter of a Windows thread stack — the "
              "prologue's stack probe will fault before main runs a statement and the host will exit with "
              "no window and no log line; move the largest member to static storage");

constexpr float WorkspaceGround[4] = { 0.06f, 0.06f, 0.08f, 1.0f };   // [-]

/// 🧩 Copies the device handles across the layer seam into the attachment the interface declares.
/// note  🔴 `SlateVulkan` cannot name `InterfaceAttachment` — it lives one layer above — so `HostLifecycle`
///        offers the same handles as `DeviceOffering` and the host performs the copy. The copy IS the seam:
///        it happens in the one translation unit that is allowed to see both sides.
/// 🧩 Where the build lowered its shader streams, resolved from the EXECUTABLE's own location: the
///    hosts ship in `<OutputRoot>/Binary`, and the streams live at `<OutputRoot>/Shader` — one hop up
///    from wherever the executable actually sits, not from the working directory. A host launched
///    from a shortcut, a debugger or a console at another directory used to resolve `current_path()`,
///    which pointed at the wrong `Shader` folder — the overlay pass refused on the missing streams
///    and the grid, the axes and the gizmo silently never drew (the reported missing overlay).
/// note  🔴 A build that never lowered shaders (the sandbox) leaves this directory absent; the
///        overlay pass refuses on the missing streams and the editor runs without the GPU overlay —
///        and now falls back to the interface-drawn overlay instead of drawing nothing.
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

    const std::filesystem::path Shader = Binary / ".." / "Shader";
    return Shader.lexically_normal().string();
}

bool ProjectWorkspaceCodexPoint(const SceneDirectoryContext& Scene,
                                const PlaneExtent& Extent,
                                double WorldX,
                                double WorldY,
                                double WorldZ,
                                float& ScreenX,
                                float& ScreenY)
{
    const double Yaw   = Scene.ViewportSkyCamera.AzimuthDegrees * 3.14159265358979323846 / 180.0;
    const double Pitch = Scene.ViewportSkyCamera.ElevationDegrees * 3.14159265358979323846 / 180.0;
    const double CosP = std::cos(Pitch), SinP = std::sin(Pitch);
    const double SinY = std::sin(Yaw),   CosY = std::cos(Yaw);
    const double ForwardX = CosP * SinY;
    const double ForwardY = SinP;
    const double ForwardZ = CosP * CosY;
    const double RightX = CosY;
    const double RightZ = -SinY;
    const double UpX = -SinP * SinY;
    const double UpY = CosP;
    const double UpZ = -SinP * CosY;

    const double DX = WorldX - Scene.CameraPosition[0];
    const double DY = WorldY - Scene.CameraPosition[1];
    const double DZ = WorldZ - Scene.CameraPosition[2];
    const double CameraX = DX * RightX + DZ * RightZ;
    const double CameraY = DX * UpX + DY * UpY + DZ * UpZ;
    const double CameraZ = DX * ForwardX + DY * ForwardY + DZ * ForwardZ;
    if (CameraZ <= 0.01)
        return false;

    const double HalfV = Scene.ViewportSkyCamera.FieldOfViewDegrees * 0.5 * 3.14159265358979323846 / 180.0;
    const double TanV = std::tan(HalfV);
    const double Aspect = Extent.Height() > 0.0f ? static_cast<double>(Extent.Width()) / static_cast<double>(Extent.Height()) : 1.0;
    const double TanH = TanV * Aspect;
    ScreenX = static_cast<float>((CameraX / (CameraZ * TanH) * 0.5 + 0.5) * Extent.Width() + Extent.MinimumX);
    ScreenY = static_cast<float>((-CameraY / (CameraZ * TanV) * 0.5 + 0.5) * Extent.Height() + Extent.MinimumY);
    return true;
}

void RecordWorkspaceCodexProxy(RecordingSurface& Surface,
                               const PlaneExtent& Extent,
                               const SceneDirectoryContext& SceneApplied,
                               const WorkspaceCodex& Scene,
                               bool SceneStanding,
                               PanelShading Shading)
{
    if (!SceneStanding)
        return;

    Surface.Confine(Extent);
    const ThemeToken Edge = Shading == PanelShading::SourceWire
                           ? Partial(0x8AB4D8u, 0.92f)
                           : Partial(0xFFFFFFu, 0.82f);
    for (const CodexSceneEntry& Entry : Scene.Scene)
    {
        if (Entry.Subject != CodexSceneSubject::Geometry)
            continue;
        const CodexSceneMesh* Mesh = nullptr;
        for (const CodexSceneMesh& Candidate : Scene.SceneMeshes)
            if (Candidate.Naming == Entry.GeometryReference)
            {
                Mesh = &Candidate;
                break;
            }
        if (Mesh == nullptr)
            continue;
        for (std::uint32_t Index = 0u; Index + 2u < Mesh->Indices.size(); Index += 3u)
        {
            float SX[3] = {};
            float SY[3] = {};
            double WorldX[3] = {};
            double WorldY[3] = {};
            double WorldZ[3] = {};
            bool Standing = true;
            for (std::uint32_t Corner = 0u; Corner < 3u; ++Corner)
            {
                const std::uint32_t Vertex = Mesh->Indices[Index + Corner];
                if (Vertex * 3u + 2u >= Mesh->Positions.size())
                {
                    Standing = false;
                    break;
                }
                WorldX[Corner] = Entry.Position[0] + Mesh->Positions[Vertex * 3u + 0u] * Entry.Scale[0];
                WorldY[Corner] = Entry.Position[1] + Mesh->Positions[Vertex * 3u + 1u] * Entry.Scale[1];
                WorldZ[Corner] = Entry.Position[2] + Mesh->Positions[Vertex * 3u + 2u] * Entry.Scale[2];
                Standing = ProjectWorkspaceCodexPoint(SceneApplied, Extent,
                    WorldX[Corner], WorldY[Corner], WorldZ[Corner], SX[Corner], SY[Corner]) && Standing;
            }
            if (Standing)
            {
                const float Corners[6] = { SX[0], SY[0], SX[1], SY[1], SX[2], SY[2] };
                const double EdgeX1 = WorldX[1] - WorldX[0];
                const double EdgeY1 = WorldY[1] - WorldY[0];
                const double EdgeZ1 = WorldZ[1] - WorldZ[0];
                const double EdgeX2 = WorldX[2] - WorldX[0];
                const double EdgeY2 = WorldY[2] - WorldY[0];
                const double EdgeZ2 = WorldZ[2] - WorldZ[0];
                const double NormalX = EdgeY1 * EdgeZ2 - EdgeZ1 * EdgeY2;
                const double NormalY = EdgeZ1 * EdgeX2 - EdgeX1 * EdgeZ2;
                const double NormalZ = EdgeX1 * EdgeY2 - EdgeY1 * EdgeX2;
                const double NormalLength = std::sqrt(NormalX * NormalX + NormalY * NormalY + NormalZ * NormalZ);
                const double LightX = 0.35;
                const double LightY = 0.82;
                const double LightZ = 0.44;
                const double Diffuse = NormalLength > 1.0e-9
                    ? std::max(0.0, (NormalX * LightX + NormalY * LightY + NormalZ * LightZ) / NormalLength)
                    : 0.0;
                const std::uint32_t LitLevel = static_cast<std::uint32_t>(
                    std::clamp(150.0 + Diffuse * 105.0, 0.0, 255.0));
                const ThemeToken Lit = Covering((LitLevel << 16u) | (LitLevel << 8u) | LitLevel);

                if (Shading == PanelShading::Lit)
                    Surface.Tongue(Corners, 3u, Lit);

                if (Shading == PanelShading::SourceWire || Shading == PanelShading::TriangulatedWire)
                {
                    const float X0[2] = { SX[0], SX[1] }; const float Y0[2] = { SY[0], SY[1] };
                    const float X1[2] = { SX[1], SX[2] }; const float Y1[2] = { SY[1], SY[2] };
                    const float X2[2] = { SX[2], SX[0] }; const float Y2[2] = { SY[2], SY[0] };
                    Surface.Polyline(X0, Y0, 2u, Edge, 0.7f);
                    Surface.Polyline(X1, Y1, 2u, Edge, 0.7f);
                    Surface.Polyline(X2, Y2, 2u, Edge, 0.7f);
                }
            }
        }
    }
    Surface.Release();
}



InterfaceAttachment Attach(const DeviceOffering& Offered)
{
    InterfaceAttachment Incoming = {};

    Incoming.Instance                 = Offered.Instance;
    Incoming.ScoredDevice             = Offered.ScoredDevice;
    Incoming.ActiveDevice             = Offered.ActiveDevice;
    Incoming.GraphicsQueue            = Offered.GraphicsQueue;
    Incoming.GraphicsFamilyIndex    = Offered.GraphicsFamilyIndex;
    Incoming.ColourTargetFormat       = Offered.ColourTargetFormat;
    Incoming.MinimumDisplayImageCount = Offered.MinimumDisplayImageCount;
    Incoming.DisplayImageCount        = Offered.DisplayImageCount;
    Incoming.NativeWindowSlot         = Offered.NativeWindowSlot;

    return Incoming;
}

enum class EditorCadKind : std::uint32_t
{
    Point,
    Line,
    Polyline,
    Arc,
    Circle,
    Polygon,
    Slot,
    Ellipse,
    Rectangle,
    Curve
};

struct EditorCadItem
{
    EditorCadKind Kind = EditorCadKind::Rectangle;
    char Name[48] = {};
    double PlaneNormal[3] = {};
    double AxisU[3] = {};
    double AxisV[3] = {};
    double RectCorners[4][3] = {};
    double Center[3] = {};
    double RadiusMajor = 0.0;
    double RadiusMinor = 0.0;
    double LineStart[3] = {};
    double LineEnd[3] = {};
    double ArcMid[3] = {};
    std::uint32_t PolygonSides = 6u;
    double SlotThickness = 0.8;
    static constexpr std::uint32_t MaxPoints = 64u;
    double Points[MaxPoints][3] = {};
    std::uint32_t PointCount = 0u;
    std::uint32_t EntityRowIndex = 0u;
    Slate::WorkspaceRecordName CadRecordName = {};
};

inline bool IsCadDrawSubject(Slate::ParametricToolSubject Subject)
{
    return Subject == Slate::ParametricToolSubject::Point ||
           Subject == Slate::ParametricToolSubject::Line ||
           Subject == Slate::ParametricToolSubject::Polyline ||
           Subject == Slate::ParametricToolSubject::Arc ||
           Subject == Slate::ParametricToolSubject::Circle ||
           Subject == Slate::ParametricToolSubject::Polygon ||
           Subject == Slate::ParametricToolSubject::Slot ||
           Subject == Slate::ParametricToolSubject::Ellipse ||
           Subject == Slate::ParametricToolSubject::Rectangle ||
           Subject == Slate::ParametricToolSubject::RationalSpline ||
           Subject == Slate::ParametricToolSubject::BasisSpline ||
           Subject == Slate::ParametricToolSubject::HermiteCurve ||
           Subject == Slate::ParametricToolSubject::BezierCurve ||
           Subject == Slate::ParametricToolSubject::CenterRectangle;
}

[[maybe_unused]] static const auto CadDraftSubjectMapping = Slate::ResolveSharedCadDraftSubject;

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                            MAIN
//------------------------------------------------------------------------------------------------------------------------

int main(int ArgumentCount, char** ArgumentValues)
{
    using namespace Slate;

    // ① The five lifetimes — window, instance, surface, diagnostic, device, chain, slots, recordings.
    HostDeclaration Declared;
    Declared.Naming        = HostName;
    Declared.WindowCaption = WindowTitle;
    Declared.InitialWidth  = InitialWidth;
    Declared.InitialHeight = InitialHeight;
    Declared.Pacing        = LatencyIntent::SteadyPacing;

#ifdef SLATE_DEBUG
    Declared.DiagnosticRequested = true;
#endif

    HostLifecycle Lifetime;

    if (!Lifetime.ConstructHost(Declared).Resolved)
        return 1;

    // ② The viewport sequence — springs, drawers, and the assembled recording.
    static ViewportSequence Viewport;

    DrawerDeclaration NorthDrawer;
    NorthDrawer.Caption       = "ControlCentre";
    NorthDrawer.TongueSubject = SymbolSubject::PulseTrace;
    NorthDrawer.PoseCount     = 2u;

    DrawerDeclaration SouthDrawer;
    SouthDrawer.Caption       = "ContentBrowser";
    SouthDrawer.TongueSubject = SymbolSubject::FolderClosed;
    SouthDrawer.PoseCount     = 3u;

    if (!Viewport.ConstructViewportSequence(Attach(Lifetime.Offering()), NorthDrawer, SouthDrawer).Resolved)
    {
        std::printf("%s \u2014 the viewport sequence was rejected\n", HostName);
        return 1;
    }

    // ─────────────────────────────────────────────────────────────────────────────────────────────────────
    //                                                       THE TICK LOOP
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────

    // 📝 🔴 Every refusal is resolved inside Await, before a display image is acquired. A tick that reports
    //    Recording has a recording open and cannot be abandoned; a tick that reports Idle has opened
    //    nothing. That is what makes the `continue` below safe — the arrangement this host previously got
    //    wrong five times over, returning to the top of the loop with a command buffer still recording.

    // ③ The workspaces this host opens. 🔴 The INDEX owns them and the panel presents them — `14` §1
    //    forbids a panel from holding what it displays, and separating the two is the whole reason there
    //    are two components here rather than one.
    // 📝 The subject this host opens by default, named once so the startup registration and the strip's `+`
    //    cannot disagree about what a new workspace is.
    constexpr WorkspaceSubject DefaultSubject = WorkspaceSubject::Vacant;

    static WorkspaceIndex          Workspaces;
    static WorkspacePanel          Workspace;
    static EditorPanel             WorkspacePanels;
    static PanelStructure          PanelPartitions[WorkspaceIndex::WorkspaceLimit];
    static EditorPanelConfiguration    PanelConfiguration[WorkspaceIndex::WorkspaceLimit];
    static ControlCentrePanel      ControlCentre;
    static ControlCentreConfiguration  ControlCentreValues;
    static SceneDirectoryPanel     SceneDirectory;
    static SceneDirectoryContext   SceneApplied;
    static ControlIndex        SceneInteraction;
    static ParametricWorkspacePanel SketchDirectory;
    static ParametricWorkspaceContext SketchDirectoryApplied;
    static ParametricToolsPanel    ParametricTools;
    static ParametricToolsContext  ParametricToolsApplied;
    static ControlIndex            ParametricInteraction;
    static TexturePaintPanel        TexturePaint;
    static TexturePaintContext     TexturePaintApplied;
    TexturePaintStack        StackRows;                 // [-] - the mutable row set; the panel borrows it
    MaterialSpecification     EditorMaterialDocument;
    SurfaceLayerSequence      EditorMaterialLayers;
    MaterialProcessingExchange EditorMaterialExchange;
    MaterialProcessingSnapshot EditorMaterialSnapshot;
    bool                      EditorMaterialSnapshotReady = false;
    AtmospherePresentationSurface AtmosphereSurface;
    AtmosphereComponent       DynamicAtmosphere;
    DirectionalLightComponent SunLight;
    EditorCameraComponent     EditorCamera;
    // Shared CAD state is kept beside the editor camera so the Editor host can consume the same
    // sketch records and draft lifecycle as the standalone Parametric Sketch host.
    static SharedCadWorkspaceRuntime CadRuntime;
    static WorkspaceNameIndex CadNaming;
    ShaderCodec             OverlayCodec;
    WorkspaceOverlayPass             Overlay;
    GeometryDeviceExchange           GeometryDevice = {};
    GeometryFileInterchange          GeometryTransfer = {};
    GeometryInterchange              ImportedGeometry = {};
    GeometryRenderingExchange        ImportedRendering = {};
    IntakeIndex                      ImportedIntake = {};
    PopulationIndex                  ImportedOwners = {};
    PartitionResolutionIndex         ImportedPartitions = {};
    VisibilityIndex                  ImportedVisibility = {};
    GeometryRenderingIdentity        PendingRendering = {};
    std::uint32_t                    PendingVisibilityRegistration = 0u;
    std::uint32_t                    PendingRegistrationBase = 0u;
    bool                             GeometryAdmissionPending = false;
    bool                             EditorCameraLookLatched = false;
    std::uint32_t           OverlayGeneration[PanelStructure::RecordLimit] = {};   // [-] - per viewport leaf

    // 📝 One overlay record per viewport leaf, in STATIC storage: each record is ~70 KB and the
    //    automatic-storage budget (a quarter of a Windows thread stack) cannot hold eleven of them.
    static OverlayGeometry   ViewportOverlays[PanelStructure::RecordLimit];
    std::uint32_t            ViewportLeafIndexs[PanelStructure::RecordLimit] = {};
    PlaneExtent              ViewportLeafRects[PanelStructure::RecordLimit]    = {};
    std::uint32_t            ViewportLeafTally = 0u;

    // 📝 The texture-paint leaves, for the Tab arbitration: the layer stack consumes Tab only when
    //    the pointer is over one of its leaves.
    PlaneExtent              LayerLeafRects[PanelStructure::RecordLimit] = {};
    std::uint32_t            LayerLeafTally = 0u;
    bool                    SkyEverGenerated = false;
    std::uint32_t           SkyQuality = 0xFFFFFFFFu;
    std::uintptr_t          SkyTextureIdentity = 0u;
    bool                    SkyRegistered = false;

    static constexpr std::uint32_t EditorCadLimit = 64u;
    static EditorCadItem EditorCadItems[EditorCadLimit] = {};
    static std::uint32_t EditorCadCount = 0u;

    static bool CadDrawActive = false;
    static std::uint32_t CadDrawPhase = 0u;
    static double CadStartWorld[3] = {};
    static double CadCurrentWorld[3] = {};
    static double CadAuxWorld[3] = {};
    static double CadDrawPlaneNormal[3] = { 0.0, 1.0, 0.0 };
    static double CadDrawPlaneOrigin[3] = { 0.0, 0.0, 0.0 };
    static double CadDrawAxisU[3] = { 1.0, 0.0, 0.0 };
    static double CadDrawAxisV[3] = { 0.0, 0.0, 1.0 };
    static ParametricToolSubject CadActiveTool = ParametricToolSubject::Select;
    static std::uint32_t CadPolygonSides = 6u;
    static double CadSlotThickness = 0.8;
    static double CadMultiPoints[64][3] = {};
    static std::uint32_t CadMultiPointCount = 0u;
    static std::uint32_t CadShapeIndex = 1u;
    static ParametricWorkspaceBridgeStorage CadBridgeStorage = {};
    static WorkspaceDirectoryProjection CadDirectoryProjection = {};

    // 📐 The editor's scene directory — the sun and sky the viewport renders, registered under the
    //    Lighting grouping. `Sun` and `Sky` are the two appended `EntitySubject` ordinals, so the
    //    inspector's slider cards branch on them while every reference entity keeps its g_NN identity.
    static EntityRow EditorEntities[SceneDirectoryContext::EntityLimit] =
    {
        { "Lighting",                EntitySubject::Grouping,   0u, 0xFFFFFFFFu, 2u, "folder lighting", CameraRole::Absent, 1002u },
        { "Directional Light (Sun)", EntitySubject::Sun,        1u,  0u,         0u, "sun light directional", CameraRole::Absent, 1003u },
        { "Sky Atmosphere",          EntitySubject::Sky,        1u,  0u,         0u, "sky atmosphere dome", CameraRole::Absent, 1004u },
        { "Environment",             EntitySubject::Grouping,   0u, 0xFFFFFFFFu, 1u, "folder environment", CameraRole::Absent, 1005u },
        { "Post Process Volume",     EntitySubject::Actor,      1u,  3u,         0u, "post volume effects", CameraRole::Absent, 1006u },
        { "Editor Camera",           EntitySubject::Camera,     0u, 0xFFFFFFFFu, 0u, "camera fly view", CameraRole::Editor, 1007u }
    };
    static SketchSceneDirectoryStorage WorkspaceSceneRows = {};
    static WorkspaceCodex OpenedScene = {};
    static bool OpenedSceneStanding = false;
    static const char* const WhiteDielectricChannels[] = { "Base Color", "Metallic", "Roughness", "Opacity" };
    static TextureLayerRow WhiteDielectricLayer = {
        "White Dielectric", TextureLayerClassification::Material, "Normal", 100u, 0xE7E3D8u, 0xE7E3D8u,
        false, 100u, false, "WhiteDielectric.pigment", "Shared Engine Content material",
        { WhiteDielectricChannels[0], WhiteDielectricChannels[1], WhiteDielectricChannels[2], WhiteDielectricChannels[3] },
        4u, 0u, 0xFFFFFFFFu, 0u, true, "white dielectric shared tea service", false, "", true, 4001u
    };
    EntityRow* PresentedEntities = EditorEntities;
    std::uint32_t PresentedEntityCount = 6u;

    FontLoader                  Fonts;

    // The full catalogue and its 356 arbitrated controls are process-lifetime UI state. Keep them out
    // of main's Windows-sized automatic frame; ordinary host setup reinitialises their presented state.
    static ControlIndex                 BrowserInteraction;

    // 📝 The south drawer's owner. The library is the HOST's, not the panel's — `14` §1 forbids a panel
    //    from holding what it displays, which is the same separation WorkspaceIndex and WorkspacePanel keep.
    static ContentBrowserPanel          ContentBrowser;
    static ContentBrowserConfiguration  ContentBrowserApplied;
    static ContentLibrary               ContentApplied;

    // 📝 The appearance file sits beside the executable and is read once, before any panel is recorded. A
    //    first run has no file yet, which is the ordinary case and not a fault — the build's own appearance
    //    stands and the first colour the artist changes writes the file.
    const char* const InvokedAs = (ArgumentCount > 0) ? ArgumentValues[0] : "";
    const std::filesystem::path ExecutablePath = InvokedAs[0] != '\0'
                                               ? std::filesystem::absolute(InvokedAs)
                                               : std::filesystem::current_path();
    const std::filesystem::path EngineContentRoot = ResolveEngineContentRoot(ExecutablePath);
    const std::string FontRoot = (EngineContentRoot / "FontArchives").string();

    {
        ThemeSelection Recorded;

        if (ThemeInterchange::AdoptBeside(InvokedAs, Recorded))
        {
            ControlCentreValues.Theme       = Recorded.Current;
            ControlCentreValues.Primary     = Recorded.Primary;
            ControlCentreValues.Secondary   = Recorded.Secondary;
            ControlCentreValues.Information = Recorded.Information;
            ControlCentreValues.Warning     = Recorded.Warning;
            ControlCentreValues.Alert       = Recorded.Alert;
        }
    }

    // 🔴 What was last written, so the file is inscribed when a colour actually changes and not every tick.
    //    A write per frame would rewrite the whole appearance sixty times a second for as long as the
    //    Control Centre is open, which is a disk cost no artist asked for.
    ThemeSelection InscribedSelection;
    InscribedSelection.Current   = ControlCentreValues.Theme;
    InscribedSelection.Primary     = ControlCentreValues.Primary;
    InscribedSelection.Secondary   = ControlCentreValues.Secondary;
    InscribedSelection.Information = ControlCentreValues.Information;
    InscribedSelection.Warning     = ControlCentreValues.Warning;
    InscribedSelection.Alert       = ControlCentreValues.Alert;

    // 🔴 Declared BEFORE any panel is constructed. Panels that copy their inks do so out of the appearance the
    //    viewport hands them at Construct, so a selection declared afterwards would leave the first frames
    //    drawn in the transcription's own theme and only correct itself on the artist's first colour change.
    Viewport.Retint(InscribedSelection);

    // 📝 Which dock node the next registered workspace is applied into; zero means the main dock space.
    std::uint32_t  RegisterIntoNode = 0u;

    Viewport.Surface().ApplyFontLoader(Fonts);
    Discard(Fonts.Discover(FontRoot.c_str()));
    // 📝 The family carousel's preview faces are added to the atlas BEFORE the first tick records. Added
    //    during recording instead, the faces would land in an atlas the renderer had already uploaded and
    //    the preview tiles would draw from stale texture data.
    Discard(Fonts.PreparePreviews(1.0f));
    Discard(Fonts.Load(FontRoot.c_str(), Viewport.Appearance().Fonts, 1.0f));
    ControlCentre.SetFontFamilies(Fonts);
    // 📝 Seat the family carousel on the family the appearance names. Without this the carousel opened
    //    on ordinal zero (the alphabetically first family) while the loaded faces were the appearance's
    //    own — and the role strips draw the LOADED family's faces, so the two have to agree at bring-up.
    for (std::uint32_t Index = 0u; Index < Fonts.FamilyCount(); ++Index)
        if (Fonts.FamilyName(Index) != nullptr &&
            std::strcmp(Fonts.FamilyName(Index), Viewport.Appearance().Fonts.Family) == 0)
        {
            ControlCentreValues.Font = Index;
            break;
        }


    if (!Workspace.ConstructWorkspacePanel(Viewport.Surface(), Viewport.Appearance()).Resolved)
    {
        std::printf("%s \u2014 the workspace panel was rejected\n", HostName);
        return 1;
    }

    if (!WorkspacePanels.ConstructEditorPanel(Viewport.MotionSource(), Viewport.Surface(), Viewport.Appearance()).Resolved)
    {
        std::printf("%s \u2014 the editor panels were rejected\n", HostName);
        return 1;
    }

    if (!ControlCentre.ConstructControlCentrePanel(Viewport.MotionSource(), Viewport.Surface(), Viewport.Appearance()).Resolved)
    {
        std::printf("%s \u2014 the Control Centre panel was rejected\n", HostName);
        return 1;
    }

    if (!BrowserInteraction.AttachMotion(Viewport.MotionSource()).Resolved)
    {
        std::printf("%s \u2014 the content browser index was rejected\n", HostName);
        return 1;
    }

    // 📝 The editor's sun and sky arrive presented, so the viewport draws the sky from the very first
    //    frame and the inspector edits the same ordinates.
    SceneApplied.EnvironmentPresented = true;
    SceneApplied.Environment.SunElevation    = 35.0;
    SceneApplied.Environment.SunAzimuth      = 120.0;
    SceneApplied.Environment.SunIntensity    = 4.8;
    SceneApplied.Environment.SunTemperature  = 5500.0;
    SceneApplied.Environment.SkyIntensity    = 1.0;
    SceneApplied.Environment.SkyTurbidity    = 2.0;
    SceneApplied.Environment.AtmosphereDensity = 1.0;
    SceneApplied.Environment.AtmosphereScaleHeight = 1.0;
    SceneApplied.EntityTaken = 2u;   // [-] - the sun, taken at bring-up

    // 📝 The texture-paint layer stack — the reference's own tree from LayerstackV1.html, seeded as
    //    the editor's mock: a folder holding an adjustment, a decal and two fills (one with a
    //    generator mask), then a fill with a paint mask, a pattern, and a second folder of materials.
    //    The row detail runs are the small sub-lines the stack page shows; the full settings live on
    //    the properties page.
    static const char* const StackChannels[TextureChannelLimit] =
    {
        "Base Color", "Metallic", "Roughness", "Normal",
        "Height", "Ambient Occlusion", "Emissive", "Opacity"
    };

    static TextureLayerRow StackSeed[TextureLayerLimit] =
    {
        { "Surface Detail",  TextureLayerClassification::Folder,  "Passthrough", 100u, 0x9B8CF0u, 0x9B8CF0u,
          false, 100u, false, "", "4 layers", { StackChannels[0], StackChannels[1], StackChannels[2] }, 3u,
          0u, 0xFFFFFFFFu, 4u, true, "folder detail group", false, "", false, 2001u },
        { "Levels",          TextureLayerClassification::Adjustment, "Overlay",   64u, 0x8B8D98u, 0x8B8D98u,
          false, 100u, false, "", "2048px \u00B7 RGBA 8", { StackChannels[0], StackChannels[3] }, 2u,
          1u, 0u, 0u, true, "adjust levels", false, "", false, 2002u },
        { "Warning Stencil", TextureLayerClassification::Decal,   "Normal",    100u, 0xE5484Du, 0xE5484Du,
          true,  100u, false, "Bitmap", "Planar \u00B7 100%", { StackChannels[0] }, 1u,
          1u, 0u, 0u, true, "decal stencil warning", false, "", false, 2003u },
        { "Scratches",       TextureLayerClassification::Paint,    "Screen",     38u, 0xB0E64Cu, 0xB0E64Cu,
          true,   88u, false, "Generator", "2048px \u00B7 RGBA 8", { StackChannels[0], StackChannels[2] }, 2u,
          1u, 0u, 0u, true, "paint scratches grunge", false, "Blur", false, 2004u },
        { "Edge Wear",       TextureLayerClassification::Fill,     "Multiply",   82u, 0xF76B15u, 0xF76B15u,
          true,  100u, false, "Generator", "2048px \u00B7 RGBA 8", { StackChannels[1], StackChannels[2] }, 2u,
          1u, 0u, 0u, true, "fill edge wear rust", false, "", false, 2005u },
        { "Emissive Trim",   TextureLayerClassification::Fill,     "Normal",    100u, 0xFFC53Du, 0xFFC53Du,
          true,  100u, false, "Paint", "2048px \u00B7 RGBA 8", { StackChannels[6] }, 1u,
          0u, 0xFFFFFFFFu, 0u, true, "fill emissive trim", false, "", false, 2006u },
        { "Hex Panelling",   TextureLayerClassification::Pattern,  "Normal",    100u, 0x8AB4D8u, 0x8AB4D8u,
          true,  100u, false, "Generator", "Hex Grid \u00B7 4\u00D74", { StackChannels[2], StackChannels[4] }, 2u,
          0u, 0xFFFFFFFFu, 0u, true, "pattern hex panel", false, "", false, 2007u },
        { "Base Materials",  TextureLayerClassification::Folder,   "Passthrough", 100u, 0x12A594u, 0x12A594u,
          false, 100u, false, "", "4 layers", { StackChannels[0], StackChannels[1] }, 2u,
          0u, 0xFFFFFFFFu, 4u, true, "folder materials base", false, "", false, 2008u },
        { "Brushed Steel",   TextureLayerClassification::Fill,     "Normal",    100u, 0x8AB4D8u, 0x8AB4D8u,
          true,  100u, false, "Generator", "4096px \u00B7 RGBA 8", { StackChannels[0], StackChannels[1] }, 2u,
          1u, 7u, 0u, true, "fill brushed steel metal", false, "", false, 2009u },
        { "Gold Inlay",      TextureLayerClassification::Fill,     "Normal",    100u, 0xE5484Du, 0xE5484Du,
          true,   50u, true,  "Color Selection", "2048px \u00B7 RGBA 8", { StackChannels[0] }, 1u,
          1u, 7u, 0u, true, "fill gold inlay", false, "Levels, HSL Shift", false, 2010u },
        { "Oak Panel",       TextureLayerClassification::Material, "Normal",    100u, 0xF76B15u, 0xF76B15u,
          false, 100u, false, "", "2048px \u00B7 RGBA 8", { StackChannels[0], StackChannels[2] }, 2u,
          1u, 7u, 0u, true, "material oak wood", true, "", false, 2011u },
        { "Canvas Weave",    TextureLayerClassification::Material, "Normal",     90u, 0xE93D82u, 0xE93D82u,
          false, 100u, false, "", "2048px \u00B7 RGBA 8", { StackChannels[0], StackChannels[3] }, 2u,
          1u, 7u, 0u, false, "material canvas fabric", false, "", false, 2012u }
    };

    TexturePaintApplied.LayerTaken = 1u;

    // 📝 The shared stack helper seeds the mutable row set and every working copy, exactly as the
    //    harness drives it — the two can never drift.
    StackRows.Seed(StackSeed, 12u);
    SeedPaintContextFromRows(TexturePaintApplied, StackRows.Rows, StackRows.Count);

    for (std::uint32_t Index = 0u; Index < TextureLayerLimit; ++Index)
    {
        TexturePaintApplied.MaskSourceTaken[Index] =
            (Index == 2u || Index == 3u || Index == 4u) ? 4u : 0u;
        TexturePaintApplied.MaskDensity[Index] = (Index == 3u) ? 88u : 100u;
        TexturePaintApplied.MaskInverted[Index] = (Index == 9u);
    }

    MaterialLayerStackBridgeReport InitialMaterialBridge = RebuildMaterialLayersFromTextureStack(
        EditorMaterialDocument, EditorMaterialLayers, StackRows, TexturePaintApplied,
        EditorMaterialExchange, nullptr);
    EditorMaterialSnapshot = InitialMaterialBridge.Snapshot;
    EditorMaterialSnapshotReady = true;

    // 📝 The editor camera, registered as the seventh row. Its details' options are the camera's own:
    //    bit 1 is the camera lag, bit 2 the inverted pitch — the lag arrives enabled so the camera
    //    eases out of the gate, and the pitch arrives un-inverted (the standard fly-cam convention).
    SceneApplied.DetailBits[6u] = 2u;
    SceneApplied.CameraSpeed = 50.0;
    EditorCamera.YawDegrees   = SceneApplied.Environment.SunAzimuth - 20.0;
    // 📐 The fly camera looks slightly DOWN at bring-up, matching the reference editors: the ground
    //    lattice fills the lower frame rather than a sliver at the horizon. A +15 degree default
    //    pointed above the horizon and crushed the perspective grid into the bottom ~100 px.
    EditorCamera.PitchDegrees = -15.0;
    EditorCamera.Position[0]  = 0.0;
    EditorCamera.Position[1]  = 1.5;
    EditorCamera.Position[2]  = 0.0;
    EditorCamera.Snap();
    SceneApplied.CameraPosition[0] = 0.0;
    SceneApplied.CameraPosition[1] = 1.5;
    SceneApplied.CameraPosition[2] = 0.0;
    SceneApplied.CameraRotation[0] = EditorCamera.YawDegrees;
    SceneApplied.CameraRotation[1] = EditorCamera.PitchDegrees;
    // The Editor Camera row's Transform card is the camera component's authored pose, not a disconnected
    // entity mirror. Other rows keep their ordinary scene transforms.
    for (std::uint32_t Axis = 0u; Axis < 3u; ++Axis)
    {
        SceneApplied.EntityPosition[6u][Axis] = SceneApplied.CameraPosition[Axis];
        SceneApplied.EntityRotation[6u][Axis] = SceneApplied.CameraRotation[Axis];
    }
    EditorCamera.PublishTransform(SceneApplied.EntityPosition[6u], SceneApplied.EntityRotation[6u]);

    if (!SceneInteraction.AttachMotion(Viewport.MotionSource()).Resolved)
    {
        std::printf("%s \u2014 the scene directory index was rejected\n", HostName);
        return 1;
    }

    if (!SceneDirectory.ConstructSceneDirectoryPanel(SceneInteraction, Viewport.MotionSource(), Viewport.Surface(),
                                  Viewport.Appearance()).Resolved)
    {
        std::printf("%s \u2014 the scene directory was rejected\n", HostName);
        return 1;
    }

    if (!TexturePaint.ConstructTexturePaintPanel(SceneInteraction, Viewport.MotionSource(), Viewport.Surface(),
                              Viewport.Appearance()).Resolved)
    {
        std::printf("%s \u2014 the texture paint panel was rejected\n", HostName);
        return 1;
    }

    if (!ParametricInteraction.AttachMotion(Viewport.MotionSource()).Resolved)
    {
        std::printf("%s \u2014 the sketch-directory index was rejected\n", HostName);
        return 1;
    }

    if (!SketchDirectory.ConstructParametricWorkspacePanel(ParametricInteraction, Viewport.MotionSource(),
                                                           Viewport.Surface(), Viewport.Appearance()).Resolved)
    {
        std::printf("%s \u2014 the sketch directory was rejected\n", HostName);
        return 1;
    }

    if (!ParametricTools.ConstructParametricToolsPanel(ParametricInteraction, Viewport.MotionSource(),
                                                       Viewport.Surface(), Viewport.Appearance()).Resolved)
    {
        std::printf("%s \u2014 the parametric tools panel was rejected\n", HostName);
        return 1;
    }

    // One shader stream index feeds both the dynamic atmosphere compute pass and the overlay pass.
    const Outcome<bool> CodecOutcome =
        OverlayCodec.AttachShaderStreams(Lifetime.DeviceExchange(), ShaderStreamDirectory());

    if (CodecOutcome.Resolved)
    {
        const Outcome<bool> AtmosphereOutcome = AtmosphereSurface.ConstructAtmosphereSurface(
            Lifetime.DeviceExchange(), Lifetime.DiagnosticsExtension(), OverlayCodec);
        if (AtmosphereOutcome.Resolved)
        {
            SkyTextureIdentity = Viewport.Surface().RegisterSampledImage(
                AtmosphereSurface.Sampler(), AtmosphereSurface.View());
            SkyRegistered = SkyTextureIdentity != 0u;
        }
        else
        {
            std::printf("%s \u2014 the GPU atmosphere presentation was rejected (reason %u: %s)\n", HostName,
                        static_cast<unsigned>(AtmosphereOutcome.Error.DeclaredReason),
                        AtmosphereOutcome.Error.Detail);
        }
    }

    // The overlay pass shares the lowered-stream index. A sandbox with no SPIR-V keeps the interface
    // functional, but a packaged editor uses the compute sky and GPU overlay paths.


    if (!CodecOutcome.Resolved)
    {
        std::printf("%s \u2014 the overlay shader streams were not found (reason %u: %s); "
                    "drawing the grid and axes through the interface fallback\n",
                    HostName,
                    static_cast<unsigned>(CodecOutcome.Error.DeclaredReason),
                    CodecOutcome.Error.Detail);
    }
    else
    {
        const Outcome<bool> PassOutcome = Overlay.ConstructWorkspaceOverlayPass(Lifetime.DeviceExchange(),
                                                            Lifetime.DiagnosticsExtension(),
                                                            OverlayCodec,
                                                            Lifetime.Offering().ColourTargetFormat);

        if (!PassOutcome.Resolved)
        {
            std::printf("%s \u2014 the overlay pass was rejected (reason %u: %s); "
                        "drawing the grid and axes through the interface fallback\n",
                        HostName,
                        static_cast<unsigned>(PassOutcome.Error.DeclaredReason),
                        PassOutcome.Error.Detail);
        }
        else
        {
            std::printf("%s \u2014 overlay pass standing: the grid, the axes and the gizmo draw on the GPU\n",
                        HostName);
        }
    }

    // 🔴 The renderer's device estate is deliberately brought up before any geometry is admitted. The next
    //    geometry increment supplies a selected imported packet; until then it owns no residency and records no
    //    geometry. Keeping the estate separate makes device recovery and display-sized target reclamation testable
    //    without inventing a placeholder surface.
    const DeviceOffering GeometryOffering = Lifetime.Offering();
    const Outcome<bool> GeometryOutcome = GeometryDevice.ConstructGeometryDeviceExchange(
        Lifetime.DeviceExchange(), Lifetime.DiagnosticsExtension(), ShaderStreamDirectory().c_str(),
        InitialWidth, InitialHeight, GeometryOffering.ColourTargetFormat);
    if (!GeometryOutcome.Resolved)
    {
        std::printf("%s \u2014 the geometry device estate was rejected (reason %u: %s)\n", HostName,
                    static_cast<unsigned>(GeometryOutcome.Error.DeclaredReason), GeometryOutcome.Error.Detail);
    }
    if (!ImportedVisibility.ConstructVisibilityIndex(InitialWidth, InitialHeight).Resolved)
    {
        std::printf("%s — imported topology partition visibility could not be prepared\n", HostName);
    }

    // 🔴 The browser carries its OWN index, as every panel here does, so its registration cannot exhaust the
    //    Control Centre's. Read — an registration refusal is silent at the call site and a browser that was
    //    rejected records nothing at all, which reads as a drawer that opens onto blank ground.
    const Outcome<bool> BrowserOutcome = ContentBrowser.ConstructContentBrowserPanel(BrowserInteraction, Viewport.Surface(), Viewport.Appearance());
    if (!BrowserOutcome.Resolved)
    {
        std::printf("%s \u2014 the content browser was rejected (reason %u: %s)\n", HostName,
                    static_cast<unsigned>(BrowserOutcome.Error.DeclaredReason), BrowserOutcome.Error.Detail);
        return 1;
    }

    // 🔴 The browser takes no appearance at Construct — it is applied here, once the viewport has resolved one.
    ContentBrowser.Reapply(Viewport.Appearance());

    ApplyReferenceContent(ContentApplied);
    PopulateImportDirectory(ContentBrowserApplied, EngineContentRoot);

    // 📝 🔴 The editor opens a VACANT workspace, where the painting host opens a canvas. This is the one
    //    thing that distinguishes the two hosts, and it is the reason there are two: the editor carries
    //    every subject and cannot presume which the artist wants, so it presents a blank one and lets them
    //    say. A host that guessed would open a canvas for someone who came to sketch.
    const Outcome<std::uint32_t> DefaultWorkspace = Workspaces.Register(DefaultSubject);
    if (!DefaultWorkspace.Resolved)
    {
        std::printf("%s \u2014 the default workspace could not be opened\n", HostName);
        return 1;
    }
    PanelPartitions[DefaultWorkspace.Resolve()].ConstructPanelPartition(PanelSubject::Viewport);

    // 🔴 The sheet's tab figures applied into the vendor's style, including the four `Patches/` adds. They
    //    default to 0.0f, at which a patched build draws stock rectangular tabs — so this call is what
    //    turns the trapezoid on.
    if (!Viewport.Seam().ApplyWorkspaceStyle(Viewport.Appearance().WorkspaceMeasure,
                                                 Viewport.Appearance().Workspace).Resolved)
    {
        std::printf("%s \u2014 the workspace style was not applied\n", HostName);
    }

    std::printf("%s \u2014 opened %s\n", HostName, Workspaces.ActiveTitle());

    while (Lifetime.Active())
    {
        const TickPass Pass = Lifetime.Await(WorkspaceGround);
        Discard(Fonts.FlushPending());

        if (Pass.Current == TickCondition::Closed)
            break;

        // 🔴 Phase one of a device rebuild. The device STILL STANDS here, so this is the one moment the
        //    interface can release its descriptor pool, font atlas and pipelines against a live handle.
        //    Reclaiming after the rebuild idles a device the vendor has already destroyed, which the
        //    loader reports as VUID-vkDeviceWaitIdle-device-parameter.
        if (Pass.DeviceRetiring)
        {
            Viewport.Reclaim();
            GeometryDevice.Reclaim();
            AtmosphereSurface.Reclaim();
            SkyRegistered = false;
            SkyTextureIdentity = 0u;
            Overlay.Reclaim();
            OverlayCodec.Reclaim();
            continue;
        }

        // ③·i 🔴 The DEVICE was rebuilt, so every device handle the interface holds names an object the
        //      vendor has returned — its font atlas, its descriptor sets, its pipelines. Renegotiating the
        //      image counts would restate figures against a device that no longer exists, so the interface
        //      is reclaimed and reconstructed against the handles the rebuilt device offers.
        //      Tested before DisplayRecovered because a device rebuild raises both.
        if (Lifetime.DeviceRecovered())
        {
            // 📝 Not reclaimed here: the retiring tick above already did it, while the device lived.
            if (!Viewport.ConstructViewportSequence(Attach(Lifetime.Offering()), NorthDrawer, SouthDrawer).Resolved)
            {
                std::printf("%s \u2014 the interface could not be rebuilt on the recovered device\n", HostName);
                break;
            }

            // The shader modules, dynamic atmosphere image and overlay all died with the old device.
            // Reattach streams first because both passes resolve their modules from that index.
            if (OverlayCodec.AttachShaderStreams(Lifetime.DeviceExchange(), ShaderStreamDirectory()).Resolved)
            {
                if (AtmosphereSurface.ConstructAtmosphereSurface(Lifetime.DeviceExchange(), Lifetime.DiagnosticsExtension(),
                                                   OverlayCodec).Resolved)
                {
                    SkyTextureIdentity = Viewport.Surface().RegisterSampledImage(
                        AtmosphereSurface.Sampler(), AtmosphereSurface.View());
                    SkyRegistered = SkyTextureIdentity != 0u;
                    SkyEverGenerated = false;
                }

                static_cast<void>(Overlay.ConstructWorkspaceOverlayPass(Lifetime.DeviceExchange(),
                                                    Lifetime.DiagnosticsExtension(),
                                                    OverlayCodec,
                                                    Lifetime.Offering().ColourTargetFormat));
                for (std::uint32_t Index = 0u; Index < PanelStructure::RecordLimit; ++Index)
                    OverlayGeneration[Index] = 0u;
            }

            const DeviceOffering ResizedGeometryOffering = Lifetime.Offering();
            const Outcome<bool> ResizedGeometryOutcome = GeometryDevice.ConstructGeometryDeviceExchange(
                Lifetime.DeviceExchange(), Lifetime.DiagnosticsExtension(), ShaderStreamDirectory().c_str(),
                Pass.Width, Pass.Height, ResizedGeometryOffering.ColourTargetFormat);
            if (!ResizedGeometryOutcome.Resolved)
            {
                std::printf("%s \u2014 the geometry device estate could not be rebuilt (reason %u: %s)\n", HostName,
                            static_cast<unsigned>(ResizedGeometryOutcome.Error.DeclaredReason), ResizedGeometryOutcome.Error.Detail);
            }

            // 📝 The display recovery this rebuild also raised is consumed here. The reconstruction above
            //    already took the counts the new chain holds, and renegotiating them again would restate
            //    what was just constructed.
            static_cast<void>(Lifetime.DisplayRecovered());
        }

        // ③ The chain was re-established. The interface is told the counts it now holds, exactly once.
        else if (Lifetime.DisplayRecovered())
        {
            const DeviceOffering Offered = Lifetime.Offering();
            // 🔴 Read, not discarded. An interface still holding the previous image counts records
            //    against a chain depth that no longer exists, and the vendor reports that as a
            //    descriptor mismatch several ticks later rather than as the resize that caused it.
            if (!Viewport.Renegotiate(Offered.MinimumDisplayImageCount, Offered.DisplayImageCount))
            {
                std::printf("%s \u2014 the interface rejected the restated image counts\n", HostName);
            }

            if (GeometryDevice.Standing() && !GeometryDevice.ReclaimDisplay(Pass.Width, Pass.Height).Resolved)
            {
                std::printf("%s \u2014 the geometry targets could not be re-derived after display recovery\n", HostName);
            }
        }

        if (Pass.Current != TickCondition::Recording)
            continue;

        // ④ Build the interface tick. A refusal here abandons the tick's content, and the recording is
        //    still surrendered — an empty rendering scope presents the cleared ground, which is correct
        //    and is what the artist sees for one tick.
        if (Viewport.Advance(Pass.ElapsedMilliseconds).Resolved)
        {
            // 🔴 The workspace is recorded FIRST and the drawers over it. One background draw list, so
            //    the order of recording IS the z-order — and the previous arrangement recorded the
            //    workspace after `RecordDrawers`, which painted the whole surface over the control
            //    centre and the asset browser.
            const PlaneExtent Whole = Spanning(0.0f, 0.0f,
                                               static_cast<float>(Pass.Width),
                                               static_cast<float>(Pass.Height));

            const PointerCondition& ForegroundPointer = Viewport.Surface().Pointer();
            const PlaneExtent NorthInterior = Viewport.Drawers().Interior(DrawerBearing::North);
            const PlaneExtent SouthInterior = Viewport.Drawers().Interior(DrawerBearing::South);
            const bool PointerBehindDrawer =
                NorthInterior.Encloses(ForegroundPointer.PositionX, ForegroundPointer.PositionY) ||
                SouthInterior.Encloses(ForegroundPointer.PositionX, ForegroundPointer.PositionY);
            PointerCondition BackgroundPointer = ForegroundPointer;
            if (PointerBehindDrawer)
            {
                BackgroundPointer.PositionX = -1000000.0f;
                BackgroundPointer.PositionY = -1000000.0f;
                BackgroundPointer.TravelX = BackgroundPointer.TravelY = BackgroundPointer.WheelY = 0.0f;
                BackgroundPointer.ContactHeld = BackgroundPointer.ContactPressed = false;
                BackgroundPointer.ContactDoublePressed = BackgroundPointer.ContactReleased = false;
                BackgroundPointer.SecondaryHeld = BackgroundPointer.SecondaryPressed = false;
                BackgroundPointer.SecondaryReleased = false;
            }

            Discard(Workspace.Record(Whole, Workspaces.ActiveTitle()));

            // 🔴 The dock space FIRST, over the whole panel. Every workspace below docks into it, and the
            //    vendor draws their tabs with PatchA's trapezoid — which is what makes a tab draggable out
            //    into a floating window. A hand-recorded tab bar cannot be undocked: the vendor's docking
            //    operates on WINDOWS, so a workspace has to be one.
            Viewport.Seam().RecordDockSpace(Whole);

            const std::uint32_t OpenCount = Workspaces.OpenCount();

            // 🔴 Titles are read through `Titled`, which points into the index's own storage. The delivered
            //    form copies the entry, so a pointer taken from it dangles at the semicolon — every label
            //    then decayed to the same garbage and ImGui reported four conflicting IDs.
            std::uint32_t Withdrawing = OpenCount;

            // 📝 The node the previous tick's `+` named, so the workspace it registered is applied into the
            //    strip the artist actually pressed rather than always into the main dock space.
            const std::uint32_t ApplyInto = RegisterIntoNode;

            RegisterIntoNode = 0u;

            WorkspacePanels.Advance(BackgroundPointer, Pass.ElapsedMilliseconds);

            // 📐 The fly camera is integrated BEFORE any leaf is recorded, so the sky geometry, the
            //    ground lattice and the gizmo are all projected through the SAME current-tick pose.
            //    The previous order advanced the camera AFTER recording, which left every overlay
            //    one frame behind the artist's input — the lattice and axes trailed the camera while
            //    panning or flying. The sky regeneration below depends only on the environment and is
            //    intentionally left where it stands.
            {
                bool PointerOverViewport = false;
                const PointerCondition& Pointer = Viewport.Surface().Pointer();

                for (std::uint32_t Leaf = 0u; Leaf < WorkspacePanels.LeafCount(); ++Leaf)
                {
                    if (WorkspacePanels.LeafSubject(Leaf) == PanelSubject::Viewport &&
                        WorkspacePanels.LeafBody(Leaf).Encloses(Pointer.PositionX, Pointer.PositionY))
                    {
                        PointerOverViewport = true;
                        break;
                    }
                }

                if (Pointer.SecondaryPressed)
                    EditorCameraLookLatched = PointerOverViewport && !PointerBehindDrawer;
                if (Pointer.SecondaryReleased || !Pointer.SecondaryHeld)
                    EditorCameraLookLatched = false;

                CameraCondition FlyInput = Viewport.Seam().CameraInput(
                    (PointerOverViewport || EditorCameraLookLatched) && !PointerBehindDrawer);

                // A direct XYZ edit in the Editor Camera's Transform card is consumed before navigation.
                // The transform synchronizer distinguishes it from the values the camera published last tick,
                // so WASD movement is never reset by a stale UI mirror.
                static_cast<void>(EditorCamera.ConsumeTransform(SceneApplied.EntityPosition[6u],
                                                                SceneApplied.EntityRotation[6u]));

                EditorCamera.FlySpeed = std::clamp(SceneApplied.CameraSpeed, 1.0, 5000.0);
                EditorCamera.FieldOfViewDegrees = std::clamp(SceneApplied.CameraFieldOfView, 20.0, 150.0);
                EditorCamera.NearClipMetres = std::clamp(SceneApplied.CameraNearClip, 0.01, 10.0);
                EditorCamera.FarClipMetres = std::clamp(SceneApplied.CameraFarClip,
                                                        EditorCamera.NearClipMetres + 0.01, 100000.0);

                if (FlyInput.SpeedSteps != 0.0f)
                {
                    // 📐 Unreal-style fly-speed gearing: while right-button look owns the viewport,
                    //    each wheel notch changes the persistent Editor Camera speed by one 25% step.
                    //    It changes speed, not FOV or position, and the outliner control reflects it.
                    EditorCamera.AdjustFlySpeed(static_cast<double>(FlyInput.SpeedSteps));
                }

                SceneApplied.CameraSpeed       = EditorCamera.FlySpeed;
                SceneApplied.CameraFieldOfView = EditorCamera.FieldOfViewDegrees;
                SceneApplied.CameraNearClip    = EditorCamera.NearClipMetres;
                SceneApplied.CameraFarClip     = EditorCamera.FarClipMetres;

                CameraSettings FlySettings;
                FlySettings.FlySpeed    = EditorCamera.FlySpeed;
                FlySettings.LagEnabled  = (SceneApplied.DetailBits[6u] & 2u) != 0u;
                FlySettings.InvertPitch = (SceneApplied.DetailBits[6u] & 4u) != 0u;

                EditorCamera.Advance(Pass.ElapsedMilliseconds / 1000.0, FlyInput, FlySettings);

                SceneApplied.ViewportSkyCamera.AzimuthDegrees    = static_cast<float>(EditorCamera.LaggedYawDegrees);
                SceneApplied.ViewportSkyCamera.ElevationDegrees  = static_cast<float>(EditorCamera.LaggedPitchDegrees);
                SceneApplied.ViewportSkyCamera.FieldOfViewDegrees =
                    static_cast<float>(EditorCamera.FieldOfViewDegrees);
                SceneApplied.CameraPosition[0] = EditorCamera.LaggedPosition[0];
                SceneApplied.CameraPosition[1] = EditorCamera.LaggedPosition[1];
                SceneApplied.CameraPosition[2] = EditorCamera.LaggedPosition[2];
                SceneApplied.CameraRotation[0] = EditorCamera.LaggedYawDegrees;
                SceneApplied.CameraRotation[1] = EditorCamera.LaggedPitchDegrees;
                SceneApplied.CameraRotation[2] = 0.0;
                EditorCamera.PublishTransform(SceneApplied.EntityPosition[6u],
                                              SceneApplied.EntityRotation[6u]);

                // Publish the same camera payload consumed by the shared CAD authoring runtime.
                CadRuntime.Camera.Position[0] = EditorCamera.LaggedPosition[0];
                CadRuntime.Camera.Position[1] = EditorCamera.LaggedPosition[1];
                CadRuntime.Camera.Position[2] = EditorCamera.LaggedPosition[2];
                CadRuntime.Camera.YawDegrees = EditorCamera.LaggedYawDegrees;
                CadRuntime.Camera.PitchDegrees = EditorCamera.LaggedPitchDegrees;
                CadRuntime.Camera.FieldOfViewDegrees = EditorCamera.FieldOfViewDegrees;
                CadRuntime.Camera.Perspective = true;
            }

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
                    // 🔴 The popups are deferred: the chrome's split/subject menus must record AFTER
                    //    the leaf content, or the sky quad paints over them and the menus become
                    //    unreadable — the reported defect when splitting a panel.
                    Discard(WorkspacePanels.Record(PanelExtent,
                                                      PanelPartitions[Index],
                                                      PanelConfiguration[Index],
                                                      Index,
                                                      true));

                    PointerCondition LeafPointer = BackgroundPointer;
                    if (WorkspacePanels.PopupOpen(Index))
                    {
                        LeafPointer.PositionX = LeafPointer.PositionY = -1000000.0f;
                        LeafPointer.TravelX = LeafPointer.TravelY = LeafPointer.WheelY = 0.0f;
                        LeafPointer.ContactHeld = LeafPointer.ContactPressed = false;
                        LeafPointer.ContactDoublePressed = LeafPointer.ContactReleased = false;
                        LeafPointer.SecondaryHeld = LeafPointer.SecondaryPressed = false;
                        LeafPointer.SecondaryReleased = false;
                    }

                    // 📝 The leaf content — the editor's scene directory inside the workspace's own
                    //    panels. Recorded into the same window the panel chrome was, so it clips and
                    //    orders with it: the sky fills a viewport leaf, the outliner | details fills
                    //    an outliner leaf, and the properties / camera bookmarks fill a properties leaf.
                    //    Panels draw their content only while they exist in the partition; there is
                    //    no fullscreen scene directory in this host (see the header's layout rule).
                    // 📝 Each viewport leaf owns its overlay record: the panel empties the leaf's
                    //    record and refills it with the grid, the axes and the gizmo projected for
                    //    THAT leaf. The host uploads each record when its generation changed and
                    //    the GPU pass draws each one clipped to its own leaf's box — so with two
                    //    viewports, each shows its own grid and neither leaks onto the panels.
                    ViewportLeafTally = 0u;
                    LayerLeafTally = 0u;

                    for (std::uint32_t Leaf = 0u; Leaf < WorkspacePanels.LeafCount(); ++Leaf)
                    {
                        const PlaneExtent LeafBody = WorkspacePanels.LeafBody(Leaf);

                        switch (WorkspacePanels.LeafSubject(Leaf))
                        {
                            case PanelSubject::Viewport:
                            {
                                CadRuntime.Camera.Perspective = PanelConfiguration[Index].Perspective;
                                OverlayGeometry& LeafOverlay = ViewportOverlays[Leaf];
                                LeafOverlay.Reset();

                                SceneDirectory.RecordViewportSky(LeafBody, SceneApplied);
                                RecordWorkspaceCodexProxy(Viewport.Surface(), LeafBody, SceneApplied,
                                                          OpenedScene, OpenedSceneStanding,
                                                          PanelConfiguration[Index].Shading);

                                const float CentreX = LeafBody.MinimumX + LeafBody.Width() * 0.5f;
                                const float CentreY = LeafBody.MinimumY + LeafBody.Height() * 0.5f;

                                const double CamHalfV = SceneApplied.ViewportSkyCamera.FieldOfViewDegrees * 0.5 * 3.14159265358979323846 / 180.0;
                                const double CamAspect = (LeafBody.Height() > 0.0f) ? static_cast<double>(LeafBody.Width()) / static_cast<double>(LeafBody.Height()) : 1.0;
                                const double CamTanHalfV = std::tan(CamHalfV);
                                const double CamTanHalfH = CamTanHalfV * CamAspect;

                                const double CamYaw = SceneApplied.ViewportSkyCamera.AzimuthDegrees * 3.14159265358979323846 / 180.0;
                                const double CamPitch = SceneApplied.ViewportSkyCamera.ElevationDegrees * 3.14159265358979323846 / 180.0;
                                const double CamCosP = std::cos(CamPitch), CamSinP = std::sin(CamPitch);
                                const double CamSinY = std::sin(CamYaw), CamCosY = std::cos(CamYaw);

                                const double CamForward[3] = { CamCosP * CamSinY, CamSinP, CamCosP * CamCosY };
                                const double CamRight[3]   = { CamCosY, 0.0, -CamSinY };
                                const double CamUp[3]      = { -CamSinP * CamSinY, CamCosP, -CamSinP * CamCosY };
                                const double CamEye[3]     = { SceneApplied.CameraPosition[0],
                                                               SceneApplied.CameraPosition[1],
                                                               SceneApplied.CameraPosition[2] };
                                const bool IsPerspective   = PanelConfiguration[Index].Perspective;

                                double ActivePlaneNormal[3] = { 0.0, 1.0, 0.0 };
                                double ActivePlaneOrigin[3] = { 0.0, 0.0, 0.0 };
                                double ActiveAxisU[3]       = { 1.0, 0.0, 0.0 };
                                double ActiveAxisV[3]       = { 0.0, 0.0, 1.0 };

                                if (std::abs(CamForward[1]) >= 0.45)
                                {
                                    ActivePlaneNormal[0] = 0.0; ActivePlaneNormal[1] = 1.0; ActivePlaneNormal[2] = 0.0;
                                    ActiveAxisU[0] = 1.0; ActiveAxisU[1] = 0.0; ActiveAxisU[2] = 0.0;
                                    ActiveAxisV[0] = 0.0; ActiveAxisV[1] = 0.0; ActiveAxisV[2] = 1.0;
                                }
                                else if (std::abs(CamForward[2]) >= std::abs(CamForward[0]))
                                {
                                    ActivePlaneNormal[0] = 0.0; ActivePlaneNormal[1] = 0.0; ActivePlaneNormal[2] = 1.0;
                                    ActiveAxisU[0] = 1.0; ActiveAxisU[1] = 0.0; ActiveAxisU[2] = 0.0;
                                    ActiveAxisV[0] = 0.0; ActiveAxisV[1] = 1.0; ActiveAxisV[2] = 0.0;
                                }
                                else
                                {
                                    ActivePlaneNormal[0] = 1.0; ActivePlaneNormal[1] = 0.0; ActivePlaneNormal[2] = 0.0;
                                    ActiveAxisU[0] = 0.0; ActiveAxisU[1] = 0.0; ActiveAxisU[2] = 1.0;
                                    ActiveAxisV[0] = 0.0; ActiveAxisV[1] = 1.0; ActiveAxisV[2] = 0.0;
                                }

                                const auto UnprojectPoint = [&](float ScreenX, float ScreenY, double OutWorld[3]) -> bool
                                {
                                    const double NdcX = (static_cast<double>(ScreenX) - CentreX) / (LeafBody.Width() * 0.5);
                                    const double NdcY = (static_cast<double>(CentreY) - ScreenY) / (LeafBody.Height() * 0.5);

                                    double RayOrig[3];
                                    double RayDir[3];

                                    if (IsPerspective)
                                    {
                                        RayOrig[0] = CamEye[0]; RayOrig[1] = CamEye[1]; RayOrig[2] = CamEye[2];
                                        RayDir[0] = CamForward[0] + CamRight[0] * (NdcX * CamTanHalfH) + CamUp[0] * (NdcY * CamTanHalfV);
                                        RayDir[1] = CamForward[1] + CamRight[1] * (NdcX * CamTanHalfH) + CamUp[1] * (NdcY * CamTanHalfV);
                                        RayDir[2] = CamForward[2] + CamRight[2] * (NdcX * CamTanHalfH) + CamUp[2] * (NdcY * CamTanHalfV);
                                    }
                                    else
                                    {
                                        const double SpanH = 20.0 * CamAspect;
                                        const double SpanV = 20.0;
                                        RayOrig[0] = CamEye[0] + CamRight[0] * (NdcX * SpanH) + CamUp[0] * (NdcY * SpanV);
                                        RayOrig[1] = CamEye[1] + CamRight[1] * (NdcX * SpanH) + CamUp[1] * (NdcY * SpanV);
                                        RayOrig[2] = CamEye[2] + CamRight[2] * (NdcX * SpanH) + CamUp[2] * (NdcY * SpanV);
                                        RayDir[0] = CamForward[0]; RayDir[1] = CamForward[1]; RayDir[2] = CamForward[2];
                                    }

                                    const double DirLen = std::sqrt(RayDir[0] * RayDir[0] + RayDir[1] * RayDir[1] + RayDir[2] * RayDir[2]);
                                    if (DirLen < 1.0e-9) return false;
                                    RayDir[0] /= DirLen; RayDir[1] /= DirLen; RayDir[2] /= DirLen;

                                    const double Denom = RayDir[0] * ActivePlaneNormal[0] + RayDir[1] * ActivePlaneNormal[1] + RayDir[2] * ActivePlaneNormal[2];
                                    if (std::abs(Denom) < 1.0e-6) return false;

                                    const double Diff[3] = { ActivePlaneOrigin[0] - RayOrig[0],
                                                             ActivePlaneOrigin[1] - RayOrig[1],
                                                             ActivePlaneOrigin[2] - RayOrig[2] };
                                    const double Numer = Diff[0] * ActivePlaneNormal[0] + Diff[1] * ActivePlaneNormal[1] + Diff[2] * ActivePlaneNormal[2];
                                    const double t = Numer / Denom;
                                    if (IsPerspective && t <= 0.0) return false;

                                    OutWorld[0] = RayOrig[0] + RayDir[0] * t;
                                    OutWorld[1] = RayOrig[1] + RayDir[1] * t;
                                    OutWorld[2] = RayOrig[2] + RayDir[2] * t;
                                    return true;
                                };

                                const auto ProjectPoint = [&](const double W[3], float& OutScreenX, float& OutScreenY) -> bool
                                {
                                    const double RelX = W[0] - CamEye[0];
                                    const double RelY = W[1] - CamEye[1];
                                    const double RelZ = W[2] - CamEye[2];
                                    const double ViewX = RelX * CamRight[0]   + RelY * CamRight[1]   + RelZ * CamRight[2];
                                    const double ViewY = RelX * CamUp[0]      + RelY * CamUp[1]      + RelZ * CamUp[2];
                                    const double ViewZ = RelX * CamForward[0] + RelY * CamForward[1] + RelZ * CamForward[2];

                                    if (IsPerspective)
                                    {
                                        if (ViewZ <= 0.01) return false;
                                        OutScreenX = CentreX + static_cast<float>((ViewX / (ViewZ * CamTanHalfH)) * (LeafBody.Width() * 0.5));
                                        OutScreenY = CentreY - static_cast<float>((ViewY / (ViewZ * CamTanHalfV)) * (LeafBody.Height() * 0.5));
                                    }
                                    else
                                    {
                                        const double SpanH = 20.0 * CamAspect;
                                        const double SpanV = 20.0;
                                        OutScreenX = CentreX + static_cast<float>((ViewX / SpanH) * (LeafBody.Width() * 0.5));
                                        OutScreenY = CentreY - static_cast<float>((ViewY / SpanV) * (LeafBody.Height() * 0.5));
                                    }
                                    return true;
                                };

                                const auto SafeOverlayLine = [&](float X0, float Y0, float X1, float Y1, std::uint32_t Color, float Width = 2.0f)
                                {
                                    if (std::isfinite(X0) && std::isfinite(Y0) && std::isfinite(X1) && std::isfinite(Y1))
                                    {
                                        if (std::abs(X0) < 40000.0f && std::abs(Y0) < 40000.0f &&
                                            std::abs(X1) < 40000.0f && std::abs(Y1) < 40000.0f)
                                        {
                                            LeafOverlay.AddLine(X0, Y0, X1, Y1, Color, Width);
                                        }
                                    }
                                };

                                const auto SafeOverlayDot = [&](float X, float Y, std::uint32_t Color, float Radius = 3.0f)
                                {
                                    if (std::isfinite(X) && std::isfinite(Y))
                                    {
                                        if (std::abs(X) < 40000.0f && std::abs(Y) < 40000.0f)
                                        {
                                            LeafOverlay.AddDot(X, Y, Color, Radius);
                                        }
                                    }
                                };

                                const auto SyncPresentedSceneEntities = [&]()
                                {
                                    if (OpenedSceneStanding)
                                    {
                                        BridgeSketchSceneDirectory(OpenedScene, WorkspaceSceneRows);
                                        AppendSketchCadReferences(CadRuntime.Records, WorkspaceSceneRows);
                                        PresentedEntities = WorkspaceSceneRows.Rows;
                                        PresentedEntityCount = WorkspaceSceneRows.RowCount;
                                        for (std::uint32_t i = 0u; i < PresentedEntityCount && i < SceneDirectoryContext::EntityLimit; ++i)
                                        {
                                            SceneApplied.EntityPresent[i] = true;
                                        }
                                    }
                                    else
                                    {
                                        std::uint32_t Count = 6u;
                                        for (std::uint32_t i = 0u; i < EditorCadCount && Count < SceneDirectoryContext::EntityLimit; ++i)
                                        {
                                            EditorCadItem& Item = EditorCadItems[i];
                                            EntityRow& Row = EditorEntities[Count];
                                            Row = EntityRow{
                                                Item.Name,
                                                EntitySubject::Actor,
                                                1u,
                                                0xFFFFFFFFu,
                                                0u,
                                                "cad geometry actor",
                                                CameraRole::Absent,
                                                2000u + Count
                                            };
                                            SceneApplied.EntityPresent[Count] = true;
                                            SceneApplied.EntityExpanded[Count] = false;
                                            for (int a = 0; a < 3; ++a)
                                            {
                                                SceneApplied.EntityPosition[Count][a] = Item.Center[a];
                                                SceneApplied.EntityRotation[Count][a] = 0.0;
                                                SceneApplied.EntityScale[Count][a] = 1.0;
                                            }
                                            Item.EntityRowIndex = Count;
                                            Count++;
                                        }
                                        PresentedEntities = EditorEntities;
                                        PresentedEntityCount = Count;
                                    }
                                };

                                if (LeafPointer.SecondaryPressed || Viewport.Surface().TextInput().CancelPressed)
                                {
                                    if (CadDrawActive)
                                    {
                                        CadDrawActive = false;
                                        CadDrawPhase = 0u;
                                        CadMultiPointCount = 0u;
                                    }
                                }

                                const ParametricToolSubject ActiveSubj = ParametricToolsApplied.ActiveSubject;
                                if (CadActiveTool != ActiveSubj)
                                {
                                    CadDrawActive = false;
                                    CadDrawPhase = 0u;
                                    CadMultiPointCount = 0u;
                                    CadActiveTool = ActiveSubj;
                                }

                                const bool IsCadTool = IsCadDrawSubject(ActiveSubj);
                                if (IsCadTool && LeafBody.Encloses(LeafPointer.PositionX, LeafPointer.PositionY))
                                {
                                    double WorldPt[3] = {};
                                    if (UnprojectPoint(LeafPointer.PositionX, LeafPointer.PositionY, WorldPt))
                                    {
                                        for (int a = 0; a < 3; ++a)
                                            CadCurrentWorld[a] = WorldPt[a];

                                        CadDrawPlaneNormal[0] = ActivePlaneNormal[0];
                                        CadDrawPlaneNormal[1] = ActivePlaneNormal[1];
                                        CadDrawPlaneNormal[2] = ActivePlaneNormal[2];
                                        CadDrawPlaneOrigin[0] = ActivePlaneOrigin[0];
                                        CadDrawPlaneOrigin[1] = ActivePlaneOrigin[1];
                                        CadDrawPlaneOrigin[2] = ActivePlaneOrigin[2];
                                        CadDrawAxisU[0] = ActiveAxisU[0];
                                        CadDrawAxisU[1] = ActiveAxisU[1];
                                        CadDrawAxisU[2] = ActiveAxisU[2];
                                        CadDrawAxisV[0] = ActiveAxisV[0];
                                        CadDrawAxisV[1] = ActiveAxisV[1];
                                        CadDrawAxisV[2] = ActiveAxisV[2];

                                        if (ActiveSubj == ParametricToolSubject::Polygon && CadDrawActive)
                                        {
                                            if (LeafPointer.WheelY > 0.05f)
                                                CadPolygonSides = std::min(CadPolygonSides + 1u, 32u);
                                            else if (LeafPointer.WheelY < -0.05f)
                                                CadPolygonSides = std::max(CadPolygonSides - 1u, 3u);
                                        }
                                        else if (ActiveSubj == ParametricToolSubject::Slot && CadDrawActive)
                                        {
                                            if (LeafPointer.WheelY > 0.05f)
                                                CadSlotThickness = std::clamp(CadSlotThickness + 0.15, 0.1, 20.0);
                                            else if (LeafPointer.WheelY < -0.05f)
                                                CadSlotThickness = std::clamp(CadSlotThickness - 0.15, 0.1, 20.0);
                                        }

                                        if (ActiveSubj == ParametricToolSubject::Point)
                                        {
                                            if (LeafPointer.ContactPressed && EditorCadCount < EditorCadLimit)
                                            {
                                                EditorCadItem& Item = EditorCadItems[EditorCadCount++];
                                                Item.Kind = EditorCadKind::Point;
                                                std::snprintf(Item.Name, sizeof(Item.Name), "Point %u", CadShapeIndex++);
                                                for (int a = 0; a < 3; ++a)
                                                {
                                                    Item.PlaneNormal[a] = ActivePlaneNormal[a];
                                                    Item.AxisU[a] = ActiveAxisU[a];
                                                    Item.AxisV[a] = ActiveAxisV[a];
                                                    Item.Center[a] = WorldPt[a];
                                                }
                                                if (!CadRuntime.Sketch.Declared())
                                                    CadRuntime.Sketch.DeclarePlane({ { 0.0, 0.0, 0.0 }, { ActivePlaneNormal[0], ActivePlaneNormal[1], ActivePlaneNormal[2] }, { ActiveAxisU[0], ActiveAxisU[1], ActiveAxisU[2] } });

                                                std::vector<WorkspaceRecordName> Written;
                                                WorkspaceRecord PointRec = {};
                                                PointRec.Subject = WorkspaceRecordSubject::Point;
                                                PointRec.Naming = CadNaming.Issue(WorkspaceRecordSubject::Point);
                                                Item.CadRecordName = CadRuntime.Records.Declare(PointRec);
                                                if (Item.CadRecordName.Assigned()) Written.push_back(Item.CadRecordName);
                                                CadRuntime.Revisions.Seal("Declared point", "Create Point", Written, CadRuntime.Revisions.DeclaredCount() + 1u);
                                                SyncPresentedSceneEntities();
                                            }
                                        }
                                        else if (ActiveSubj == ParametricToolSubject::Line)
                                        {
                                            if (LeafPointer.ContactPressed)
                                            {
                                                CadDrawActive = true;
                                                CadDrawPhase = 1u;
                                                for (int a = 0; a < 3; ++a)
                                                {
                                                    CadStartWorld[a] = WorldPt[a];
                                                    CadCurrentWorld[a] = WorldPt[a];
                                                }
                                            }
                                            else if (CadDrawActive && (LeafPointer.ContactReleased || !LeafPointer.ContactHeld))
                                            {
                                                const double Dist = std::sqrt(
                                                    (CadCurrentWorld[0] - CadStartWorld[0]) * (CadCurrentWorld[0] - CadStartWorld[0]) +
                                                    (CadCurrentWorld[1] - CadStartWorld[1]) * (CadCurrentWorld[1] - CadStartWorld[1]) +
                                                    (CadCurrentWorld[2] - CadStartWorld[2]) * (CadCurrentWorld[2] - CadStartWorld[2]));
                                                if (Dist > 0.02 && EditorCadCount < EditorCadLimit)
                                                {
                                                    EditorCadItem& Item = EditorCadItems[EditorCadCount++];
                                                    Item.Kind = EditorCadKind::Line;
                                                    std::snprintf(Item.Name, sizeof(Item.Name), "Line %u", CadShapeIndex++);
                                                    for (int a = 0; a < 3; ++a)
                                                    {
                                                        Item.PlaneNormal[a] = CadDrawPlaneNormal[a];
                                                        Item.AxisU[a] = CadDrawAxisU[a];
                                                        Item.AxisV[a] = CadDrawAxisV[a];
                                                        Item.LineStart[a] = CadStartWorld[a];
                                                        Item.LineEnd[a] = CadCurrentWorld[a];
                                                        Item.Center[a] = (CadStartWorld[a] + CadCurrentWorld[a]) * 0.5;
                                                    }
                                                    if (!CadRuntime.Sketch.Declared())
                                                        CadRuntime.Sketch.DeclarePlane({ { 0.0, 0.0, 0.0 }, { CadDrawPlaneNormal[0], CadDrawPlaneNormal[1], CadDrawPlaneNormal[2] }, { CadDrawAxisU[0], CadDrawAxisU[1], CadDrawAxisU[2] } });

                                                    const SpatialPoint SP0 = { Item.LineStart[0], Item.LineStart[1], Item.LineStart[2] };
                                                    const SpatialPoint SP1 = { Item.LineEnd[0], Item.LineEnd[1], Item.LineEnd[2] };
                                                    const SketchCurveName LineCurve = CadRuntime.Sketch.DeclareLine(SP0, SP1);
                                                    std::vector<WorkspaceRecordName> Written;
                                                    WorkspaceRecord CurveRec = {};
                                                    CurveRec.Subject = WorkspaceRecordSubject::OpenCurve;
                                                    CurveRec.Naming = CadNaming.Issue(WorkspaceRecordSubject::OpenCurve);
                                                    CurveRec.SketchCurve = LineCurve;
                                                    const WorkspaceRecordName CN = CadRuntime.Records.Declare(CurveRec);
                                                    if (CN.Assigned()) Written.push_back(CN);
                                                    CadRuntime.Revisions.Seal("Declared line", "Create Line", Written, CadRuntime.Revisions.DeclaredCount() + 1u);
                                                    SyncPresentedSceneEntities();
                                                }
                                                CadDrawActive = false;
                                                CadDrawPhase = 0u;
                                            }
                                        }
                                        else if (ActiveSubj == ParametricToolSubject::Rectangle || ActiveSubj == ParametricToolSubject::CenterRectangle)
                                        {
                                            if (LeafPointer.ContactPressed)
                                            {
                                                CadDrawActive = true;
                                                CadDrawPhase = 1u;
                                                for (int a = 0; a < 3; ++a)
                                                {
                                                    CadStartWorld[a] = WorldPt[a];
                                                    CadCurrentWorld[a] = WorldPt[a];
                                                }
                                            }
                                            else if (CadDrawActive && (LeafPointer.ContactReleased || !LeafPointer.ContactHeld))
                                            {
                                                const double Diff[3] = { CadCurrentWorld[0] - CadStartWorld[0],
                                                                         CadCurrentWorld[1] - CadStartWorld[1],
                                                                         CadCurrentWorld[2] - CadStartWorld[2] };
                                                const double DU = Diff[0] * CadDrawAxisU[0] + Diff[1] * CadDrawAxisU[1] + Diff[2] * CadDrawAxisU[2];
                                                const double DV = Diff[0] * CadDrawAxisV[0] + Diff[1] * CadDrawAxisV[1] + Diff[2] * CadDrawAxisV[2];

                                                if (std::abs(DU) > 0.02 && std::abs(DV) > 0.02 && EditorCadCount < EditorCadLimit)
                                                {
                                                    EditorCadItem& Item = EditorCadItems[EditorCadCount++];
                                                    Item.Kind = EditorCadKind::Rectangle;
                                                    std::snprintf(Item.Name, sizeof(Item.Name), "Rectangle %u", CadShapeIndex++);
                                                    for (int a = 0; a < 3; ++a)
                                                    {
                                                        Item.PlaneNormal[a] = CadDrawPlaneNormal[a];
                                                        Item.AxisU[a] = CadDrawAxisU[a];
                                                        Item.AxisV[a] = CadDrawAxisV[a];
                                                        Item.RectCorners[0][a] = CadStartWorld[a];
                                                        Item.RectCorners[1][a] = CadStartWorld[a] + CadDrawAxisU[a] * DU;
                                                        Item.RectCorners[2][a] = CadCurrentWorld[a];
                                                        Item.RectCorners[3][a] = CadStartWorld[a] + CadDrawAxisV[a] * DV;
                                                        Item.Center[a] = (Item.RectCorners[0][a] + Item.RectCorners[2][a]) * 0.5;
                                                    }
                                                    if (!CadRuntime.Sketch.Declared())
                                                        CadRuntime.Sketch.DeclarePlane({ { 0.0, 0.0, 0.0 }, { CadDrawPlaneNormal[0], CadDrawPlaneNormal[1], CadDrawPlaneNormal[2] }, { CadDrawAxisU[0], CadDrawAxisU[1], CadDrawAxisU[2] } });

                                                    const SpatialPoint SP0 = { Item.RectCorners[0][0], Item.RectCorners[0][1], Item.RectCorners[0][2] };
                                                    const SpatialPoint SP1 = { Item.RectCorners[1][0], Item.RectCorners[1][1], Item.RectCorners[1][2] };
                                                    const SpatialPoint SP2 = { Item.RectCorners[2][0], Item.RectCorners[2][1], Item.RectCorners[2][2] };
                                                    const SpatialPoint SP3 = { Item.RectCorners[3][0], Item.RectCorners[3][1], Item.RectCorners[3][2] };

                                                    std::vector<WorkspaceRecordName> Written;
                                                    const auto DeclareCurve = [&](SketchCurveName Curve)
                                                    {
                                                        WorkspaceRecord Rec = {};
                                                        Rec.Subject = WorkspaceRecordSubject::OpenCurve;
                                                        Rec.Naming = CadNaming.Issue(WorkspaceRecordSubject::OpenCurve);
                                                        Rec.SketchCurve = Curve;
                                                        const WorkspaceRecordName N = CadRuntime.Records.Declare(Rec);
                                                        if (N.Assigned()) Written.push_back(N);
                                                    };
                                                    DeclareCurve(CadRuntime.Sketch.DeclareLine(SP0, SP1));
                                                    DeclareCurve(CadRuntime.Sketch.DeclareLine(SP1, SP2));
                                                    DeclareCurve(CadRuntime.Sketch.DeclareLine(SP2, SP3));
                                                    DeclareCurve(CadRuntime.Sketch.DeclareLine(SP3, SP0));

                                                    WorkspaceRecord ProfileRec = {};
                                                    ProfileRec.Subject = WorkspaceRecordSubject::ClosedProfile;
                                                    ProfileRec.Naming = CadNaming.Issue(WorkspaceRecordSubject::ClosedProfile);
                                                    ProfileRec.ClosedSemantic = true;
                                                    ProfileRec.CappedExtrusionSemantic = true;
                                                    Item.CadRecordName = CadRuntime.Records.Declare(ProfileRec);
                                                    if (Item.CadRecordName.Assigned()) Written.push_back(Item.CadRecordName);

                                                    CadRuntime.Revisions.Seal("Declared rectangle", "Create Rectangle", Written,
                                                                              CadRuntime.Revisions.DeclaredCount() + 1u);
                                                    SyncPresentedSceneEntities();
                                                }
                                                CadDrawActive = false;
                                                CadDrawPhase = 0u;
                                            }
                                        }
                                        else if (ActiveSubj == ParametricToolSubject::Circle)
                                        {
                                            if (LeafPointer.ContactPressed)
                                            {
                                                CadDrawActive = true;
                                                CadDrawPhase = 1u;
                                                for (int a = 0; a < 3; ++a)
                                                {
                                                    CadStartWorld[a] = WorldPt[a];
                                                    CadCurrentWorld[a] = WorldPt[a];
                                                }
                                            }
                                            else if (CadDrawActive && (LeafPointer.ContactReleased || !LeafPointer.ContactHeld))
                                            {
                                                const double Diff[3] = { CadCurrentWorld[0] - CadStartWorld[0],
                                                                         CadCurrentWorld[1] - CadStartWorld[1],
                                                                         CadCurrentWorld[2] - CadStartWorld[2] };
                                                const double Radius = std::sqrt(Diff[0] * Diff[0] + Diff[1] * Diff[1] + Diff[2] * Diff[2]);

                                                if (Radius > 0.02 && EditorCadCount < EditorCadLimit)
                                                {
                                                    EditorCadItem& Item = EditorCadItems[EditorCadCount++];
                                                    Item.Kind = EditorCadKind::Circle;
                                                    std::snprintf(Item.Name, sizeof(Item.Name), "Circle %u", CadShapeIndex++);
                                                    for (int a = 0; a < 3; ++a)
                                                    {
                                                        Item.PlaneNormal[a] = CadDrawPlaneNormal[a];
                                                        Item.AxisU[a] = CadDrawAxisU[a];
                                                        Item.AxisV[a] = CadDrawAxisV[a];
                                                        Item.Center[a] = CadStartWorld[a];
                                                    }
                                                    Item.RadiusMajor = Radius;

                                                    if (!CadRuntime.Sketch.Declared())
                                                        CadRuntime.Sketch.DeclarePlane({ { 0.0, 0.0, 0.0 }, { CadDrawPlaneNormal[0], CadDrawPlaneNormal[1], CadDrawPlaneNormal[2] }, { CadDrawAxisU[0], CadDrawAxisU[1], CadDrawAxisU[2] } });

                                                    const SpatialPoint C = { Item.Center[0], Item.Center[1], Item.Center[2] };
                                                    const CircularArcCurve Arc = { C, { Item.PlaneNormal[0], Item.PlaneNormal[1], Item.PlaneNormal[2] },
                                                                                  { Item.AxisU[0], Item.AxisU[1], Item.AxisU[2] }, {}, false, Item.RadiusMajor,
                                                                                  2.0 * 3.14159265358979323846 };
                                                    const SketchCurveName Curve = CadRuntime.Sketch.DeclareCurve(CurveSpecification::DeclareCircularArc(Arc, { 0.0, 1.0 }));

                                                    std::vector<WorkspaceRecordName> Written;
                                                    WorkspaceRecord CurveRec = {};
                                                    CurveRec.Subject = WorkspaceRecordSubject::OpenCurve;
                                                    CurveRec.Naming = CadNaming.Issue(WorkspaceRecordSubject::OpenCurve);
                                                    CurveRec.SketchCurve = Curve;
                                                    const WorkspaceRecordName CN = CadRuntime.Records.Declare(CurveRec);
                                                    if (CN.Assigned()) Written.push_back(CN);

                                                    WorkspaceRecord ProfileRec = {};
                                                    ProfileRec.Subject = WorkspaceRecordSubject::ClosedProfile;
                                                    ProfileRec.Naming = CadNaming.Issue(WorkspaceRecordSubject::ClosedProfile);
                                                    ProfileRec.ClosedSemantic = true;
                                                    ProfileRec.CappedExtrusionSemantic = true;
                                                    Item.CadRecordName = CadRuntime.Records.Declare(ProfileRec);
                                                    if (Item.CadRecordName.Assigned()) Written.push_back(Item.CadRecordName);

                                                    CadRuntime.Revisions.Seal("Declared circle", "Create Circle", Written,
                                                                              CadRuntime.Revisions.DeclaredCount() + 1u);
                                                    SyncPresentedSceneEntities();
                                                }
                                                CadDrawActive = false;
                                                CadDrawPhase = 0u;
                                            }
                                        }
                                        else if (ActiveSubj == ParametricToolSubject::Polygon)
                                        {
                                            if (LeafPointer.ContactPressed)
                                            {
                                                CadDrawActive = true;
                                                CadDrawPhase = 1u;
                                                for (int a = 0; a < 3; ++a)
                                                {
                                                    CadStartWorld[a] = WorldPt[a];
                                                    CadCurrentWorld[a] = WorldPt[a];
                                                }
                                            }
                                            else if (CadDrawActive && (LeafPointer.ContactReleased || !LeafPointer.ContactHeld))
                                            {
                                                const double Diff[3] = { CadCurrentWorld[0] - CadStartWorld[0],
                                                                         CadCurrentWorld[1] - CadStartWorld[1],
                                                                         CadCurrentWorld[2] - CadStartWorld[2] };
                                                const double Radius = std::sqrt(Diff[0] * Diff[0] + Diff[1] * Diff[1] + Diff[2] * Diff[2]);

                                                if (Radius > 0.02 && EditorCadCount < EditorCadLimit)
                                                {
                                                    EditorCadItem& Item = EditorCadItems[EditorCadCount++];
                                                    Item.Kind = EditorCadKind::Polygon;
                                                    std::snprintf(Item.Name, sizeof(Item.Name), "Polygon %u", CadShapeIndex++);
                                                    for (int a = 0; a < 3; ++a)
                                                    {
                                                        Item.PlaneNormal[a] = CadDrawPlaneNormal[a];
                                                        Item.AxisU[a] = CadDrawAxisU[a];
                                                        Item.AxisV[a] = CadDrawAxisV[a];
                                                        Item.Center[a] = CadStartWorld[a];
                                                    }
                                                    Item.RadiusMajor = Radius;
                                                    Item.PolygonSides = CadPolygonSides;
                                                    Item.PointCount = CadPolygonSides;

                                                    const double DU = Diff[0] * CadDrawAxisU[0] + Diff[1] * CadDrawAxisU[1] + Diff[2] * CadDrawAxisU[2];
                                                    const double DV = Diff[0] * CadDrawAxisV[0] + Diff[1] * CadDrawAxisV[1] + Diff[2] * CadDrawAxisV[2];
                                                    const double Theta0 = std::atan2(DV, DU);

                                                    for (std::uint32_t s = 0u; s < Item.PolygonSides; ++s)
                                                    {
                                                        const double Ang = Theta0 + static_cast<double>(s) * 2.0 * 3.14159265358979323846 / static_cast<double>(Item.PolygonSides);
                                                        for (int a = 0; a < 3; ++a)
                                                            Item.Points[s][a] = Item.Center[a] + Item.AxisU[a] * (Radius * std::cos(Ang)) + Item.AxisV[a] * (Radius * std::sin(Ang));
                                                    }

                                                    if (!CadRuntime.Sketch.Declared())
                                                        CadRuntime.Sketch.DeclarePlane({ { 0.0, 0.0, 0.0 }, { CadDrawPlaneNormal[0], CadDrawPlaneNormal[1], CadDrawPlaneNormal[2] }, { CadDrawAxisU[0], CadDrawAxisU[1], CadDrawAxisU[2] } });

                                                    std::vector<WorkspaceRecordName> Written;
                                                    for (std::uint32_t s = 0u; s < Item.PolygonSides; ++s)
                                                    {
                                                        const std::uint32_t Next = (s + 1u) % Item.PolygonSides;
                                                        const SpatialPoint P0 = { Item.Points[s][0], Item.Points[s][1], Item.Points[s][2] };
                                                        const SpatialPoint P1 = { Item.Points[Next][0], Item.Points[Next][1], Item.Points[Next][2] };
                                                        const SketchCurveName LineC = CadRuntime.Sketch.DeclareLine(P0, P1);
                                                        WorkspaceRecord Rec = {};
                                                        Rec.Subject = WorkspaceRecordSubject::OpenCurve;
                                                        Rec.Naming = CadNaming.Issue(WorkspaceRecordSubject::OpenCurve);
                                                        Rec.SketchCurve = LineC;
                                                        const WorkspaceRecordName N = CadRuntime.Records.Declare(Rec);
                                                        if (N.Assigned()) Written.push_back(N);
                                                    }

                                                    WorkspaceRecord ProfileRec = {};
                                                    ProfileRec.Subject = WorkspaceRecordSubject::ClosedProfile;
                                                    ProfileRec.Naming = CadNaming.Issue(WorkspaceRecordSubject::ClosedProfile);
                                                    ProfileRec.ClosedSemantic = true;
                                                    ProfileRec.CappedExtrusionSemantic = true;
                                                    Item.CadRecordName = CadRuntime.Records.Declare(ProfileRec);
                                                    if (Item.CadRecordName.Assigned()) Written.push_back(Item.CadRecordName);

                                                    CadRuntime.Revisions.Seal("Declared polygon", "Create Polygon", Written,
                                                                              CadRuntime.Revisions.DeclaredCount() + 1u);
                                                    SyncPresentedSceneEntities();
                                                }
                                                CadDrawActive = false;
                                                CadDrawPhase = 0u;
                                            }
                                        }
                                        else if (ActiveSubj == ParametricToolSubject::Arc)
                                        {
                                            if (CadDrawPhase == 0u)
                                            {
                                                if (LeafPointer.ContactPressed)
                                                {
                                                    CadDrawActive = true;
                                                    CadDrawPhase = 1u;
                                                    for (int a = 0; a < 3; ++a)
                                                    {
                                                        CadStartWorld[a] = WorldPt[a];
                                                        CadCurrentWorld[a] = WorldPt[a];
                                                    }
                                                }
                                            }
                                            else if (CadDrawPhase == 1u)
                                            {
                                                if (LeafPointer.ContactReleased)
                                                {
                                                    const double Dist = std::sqrt(
                                                        (CadCurrentWorld[0] - CadStartWorld[0]) * (CadCurrentWorld[0] - CadStartWorld[0]) +
                                                        (CadCurrentWorld[1] - CadStartWorld[1]) * (CadCurrentWorld[1] - CadStartWorld[1]) +
                                                        (CadCurrentWorld[2] - CadStartWorld[2]) * (CadCurrentWorld[2] - CadStartWorld[2]));
                                                    if (Dist > 0.05)
                                                    {
                                                        for (int a = 0; a < 3; ++a)
                                                            CadAuxWorld[a] = CadCurrentWorld[a];
                                                        CadDrawPhase = 2u;
                                                    }
                                                }
                                            }
                                            else if (CadDrawPhase == 2u)
                                            {
                                                if (LeafPointer.ContactPressed)
                                                {
                                                    if (EditorCadCount < EditorCadLimit)
                                                    {
                                                        EditorCadItem& Item = EditorCadItems[EditorCadCount++];
                                                        Item.Kind = EditorCadKind::Arc;
                                                        std::snprintf(Item.Name, sizeof(Item.Name), "Arc %u", CadShapeIndex++);
                                                        for (int a = 0; a < 3; ++a)
                                                        {
                                                            Item.PlaneNormal[a] = CadDrawPlaneNormal[a];
                                                            Item.AxisU[a] = CadDrawAxisU[a];
                                                            Item.AxisV[a] = CadDrawAxisV[a];
                                                            Item.LineStart[a] = CadStartWorld[a];
                                                            Item.LineEnd[a] = CadAuxWorld[a];
                                                            Item.ArcMid[a] = CadCurrentWorld[a];
                                                            Item.Center[a] = (Item.LineStart[a] + Item.LineEnd[a]) * 0.5;
                                                        }
                                                        if (!CadRuntime.Sketch.Declared())
                                                            CadRuntime.Sketch.DeclarePlane({ { 0.0, 0.0, 0.0 }, { CadDrawPlaneNormal[0], CadDrawPlaneNormal[1], CadDrawPlaneNormal[2] }, { CadDrawAxisU[0], CadDrawAxisU[1], CadDrawAxisU[2] } });

                                                        const SpatialPoint SP0 = { Item.LineStart[0], Item.LineStart[1], Item.LineStart[2] };
                                                        const SpatialPoint SP1 = { Item.ArcMid[0], Item.ArcMid[1], Item.ArcMid[2] };
                                                        const SpatialPoint SP2 = { Item.LineEnd[0], Item.LineEnd[1], Item.LineEnd[2] };

                                                        const SketchCurveName ArcC0 = CadRuntime.Sketch.DeclareLine(SP0, SP1);
                                                        const SketchCurveName ArcC1 = CadRuntime.Sketch.DeclareLine(SP1, SP2);

                                                        std::vector<WorkspaceRecordName> Written;
                                                        WorkspaceRecord Rec0 = {};
                                                        Rec0.Subject = WorkspaceRecordSubject::OpenCurve;
                                                        Rec0.Naming = CadNaming.Issue(WorkspaceRecordSubject::OpenCurve);
                                                        Rec0.SketchCurve = ArcC0;
                                                        const WorkspaceRecordName N0 = CadRuntime.Records.Declare(Rec0);
                                                        if (N0.Assigned()) Written.push_back(N0);

                                                        WorkspaceRecord Rec1 = {};
                                                        Rec1.Subject = WorkspaceRecordSubject::OpenCurve;
                                                        Rec1.Naming = CadNaming.Issue(WorkspaceRecordSubject::OpenCurve);
                                                        Rec1.SketchCurve = ArcC1;
                                                        const WorkspaceRecordName N1 = CadRuntime.Records.Declare(Rec1);
                                                        if (N1.Assigned()) Written.push_back(N1);

                                                        Item.CadRecordName = N0;
                                                        CadRuntime.Revisions.Seal("Declared arc", "Create Arc", Written,
                                                                                  CadRuntime.Revisions.DeclaredCount() + 1u);
                                                        SyncPresentedSceneEntities();
                                                    }
                                                    CadDrawActive = false;
                                                    CadDrawPhase = 0u;
                                                }
                                            }
                                        }
                                        else if (ActiveSubj == ParametricToolSubject::Ellipse)
                                        {
                                            if (LeafPointer.ContactPressed)
                                            {
                                                CadDrawActive = true;
                                                CadDrawPhase = 1u;
                                                for (int a = 0; a < 3; ++a)
                                                {
                                                    CadStartWorld[a] = WorldPt[a];
                                                    CadCurrentWorld[a] = WorldPt[a];
                                                }
                                            }
                                            else if (CadDrawActive && (LeafPointer.ContactReleased || !LeafPointer.ContactHeld))
                                            {
                                                const double Diff[3] = { CadCurrentWorld[0] - CadStartWorld[0],
                                                                         CadCurrentWorld[1] - CadStartWorld[1],
                                                                         CadCurrentWorld[2] - CadStartWorld[2] };
                                                double DU = std::abs(Diff[0] * CadDrawAxisU[0] + Diff[1] * CadDrawAxisU[1] + Diff[2] * CadDrawAxisU[2]);
                                                double DV = std::abs(Diff[0] * CadDrawAxisV[0] + Diff[1] * CadDrawAxisV[1] + Diff[2] * CadDrawAxisV[2]);
                                                const double Dist = std::sqrt(Diff[0] * Diff[0] + Diff[1] * Diff[1] + Diff[2] * Diff[2]);

                                                if (DU < 0.05 && DV < 0.05) { DU = DV = Dist; }
                                                else { DU = std::max(DU, 0.05); DV = std::max(DV, 0.05); }

                                                if (DU > 0.02 && DV > 0.02 && EditorCadCount < EditorCadLimit)
                                                {
                                                    EditorCadItem& Item = EditorCadItems[EditorCadCount++];
                                                    Item.Kind = EditorCadKind::Ellipse;
                                                    std::snprintf(Item.Name, sizeof(Item.Name), "Ellipse %u", CadShapeIndex++);
                                                    for (int a = 0; a < 3; ++a)
                                                    {
                                                        Item.PlaneNormal[a] = CadDrawPlaneNormal[a];
                                                        Item.AxisU[a] = CadDrawAxisU[a];
                                                        Item.AxisV[a] = CadDrawAxisV[a];
                                                        Item.Center[a] = CadStartWorld[a];
                                                    }
                                                    Item.RadiusMajor = DU;
                                                    Item.RadiusMinor = DV;

                                                    if (!CadRuntime.Sketch.Declared())
                                                        CadRuntime.Sketch.DeclarePlane({ { 0.0, 0.0, 0.0 }, { CadDrawPlaneNormal[0], CadDrawPlaneNormal[1], CadDrawPlaneNormal[2] }, { CadDrawAxisU[0], CadDrawAxisU[1], CadDrawAxisU[2] } });

                                                    std::vector<WorkspaceRecordName> Written;
                                                    constexpr int EllipseSegs = 24;
                                                    for (int s = 0; s < EllipseSegs; ++s)
                                                    {
                                                        const double A0 = static_cast<double>(s) * 2.0 * 3.14159265358979323846 / static_cast<double>(EllipseSegs);
                                                        const double A1 = static_cast<double>((s + 1) % EllipseSegs) * 2.0 * 3.14159265358979323846 / static_cast<double>(EllipseSegs);
                                                        const SpatialPoint P0 = {
                                                            Item.Center[0] + Item.AxisU[0] * (DU * std::cos(A0)) + Item.AxisV[0] * (DV * std::sin(A0)),
                                                            Item.Center[1] + Item.AxisU[1] * (DU * std::cos(A0)) + Item.AxisV[1] * (DV * std::sin(A0)),
                                                            Item.Center[2] + Item.AxisU[2] * (DU * std::cos(A0)) + Item.AxisV[2] * (DV * std::sin(A0))
                                                        };
                                                        const SpatialPoint P1 = {
                                                            Item.Center[0] + Item.AxisU[0] * (DU * std::cos(A1)) + Item.AxisV[0] * (DV * std::sin(A1)),
                                                            Item.Center[1] + Item.AxisU[1] * (DU * std::cos(A1)) + Item.AxisV[1] * (DV * std::sin(A1)),
                                                            Item.Center[2] + Item.AxisU[2] * (DU * std::cos(A1)) + Item.AxisV[2] * (DV * std::sin(A1))
                                                        };
                                                        const SketchCurveName LineC = CadRuntime.Sketch.DeclareLine(P0, P1);
                                                        WorkspaceRecord Rec = {};
                                                        Rec.Subject = WorkspaceRecordSubject::OpenCurve;
                                                        Rec.Naming = CadNaming.Issue(WorkspaceRecordSubject::OpenCurve);
                                                        Rec.SketchCurve = LineC;
                                                        const WorkspaceRecordName N = CadRuntime.Records.Declare(Rec);
                                                        if (N.Assigned()) Written.push_back(N);
                                                    }

                                                    WorkspaceRecord ProfileRec = {};
                                                    ProfileRec.Subject = WorkspaceRecordSubject::ClosedProfile;
                                                    ProfileRec.Naming = CadNaming.Issue(WorkspaceRecordSubject::ClosedProfile);
                                                    ProfileRec.ClosedSemantic = true;
                                                    ProfileRec.CappedExtrusionSemantic = true;
                                                    Item.CadRecordName = CadRuntime.Records.Declare(ProfileRec);
                                                    if (Item.CadRecordName.Assigned()) Written.push_back(Item.CadRecordName);

                                                    CadRuntime.Revisions.Seal("Declared ellipse", "Create Ellipse", Written,
                                                                              CadRuntime.Revisions.DeclaredCount() + 1u);
                                                    SyncPresentedSceneEntities();
                                                }
                                                CadDrawActive = false;
                                                CadDrawPhase = 0u;
                                            }
                                        }
                                        else if (ActiveSubj == ParametricToolSubject::Slot)
                                        {
                                            if (!CadDrawActive)
                                            {
                                                if (LeafPointer.ContactPressed)
                                                {
                                                    CadDrawActive = true;
                                                    CadDrawPhase = 1u;
                                                    CadMultiPointCount = 1u;
                                                    for (int a = 0; a < 3; ++a)
                                                    {
                                                        CadMultiPoints[0][a] = WorldPt[a];
                                                        CadStartWorld[a] = WorldPt[a];
                                                        CadCurrentWorld[a] = WorldPt[a];
                                                    }
                                                    CadSlotThickness = 0.8;
                                                }
                                            }
                                            else
                                            {
                                                if (LeafPointer.ContactDoublePressed)
                                                {
                                                    if (CadMultiPointCount < 64u)
                                                    {
                                                        for (int a = 0; a < 3; ++a)
                                                            CadMultiPoints[CadMultiPointCount][a] = WorldPt[a];
                                                        CadMultiPointCount++;
                                                    }
                                                    if (CadMultiPointCount >= 2u && EditorCadCount < EditorCadLimit)
                                                    {
                                                        EditorCadItem& Item = EditorCadItems[EditorCadCount++];
                                                        Item.Kind = EditorCadKind::Slot;
                                                        std::snprintf(Item.Name, sizeof(Item.Name), "Slot %u", CadShapeIndex++);
                                                        for (int a = 0; a < 3; ++a)
                                                        {
                                                            Item.PlaneNormal[a] = CadDrawPlaneNormal[a];
                                                            Item.AxisU[a] = CadDrawAxisU[a];
                                                            Item.AxisV[a] = CadDrawAxisV[a];
                                                            Item.Center[a] = CadMultiPoints[0][a];
                                                        }
                                                        Item.SlotThickness = CadSlotThickness;
                                                        Item.PointCount = CadMultiPointCount;
                                                        for (std::uint32_t p = 0u; p < CadMultiPointCount; ++p)
                                                            for (int a = 0; a < 3; ++a)
                                                                Item.Points[p][a] = CadMultiPoints[p][a];

                                                        if (!CadRuntime.Sketch.Declared())
                                                            CadRuntime.Sketch.DeclarePlane({ { 0.0, 0.0, 0.0 }, { CadDrawPlaneNormal[0], CadDrawPlaneNormal[1], CadDrawPlaneNormal[2] }, { CadDrawAxisU[0], CadDrawAxisU[1], CadDrawAxisU[2] } });

                                                        std::vector<WorkspaceRecordName> Written;
                                                        for (std::uint32_t p = 0u; p + 1u < Item.PointCount; ++p)
                                                        {
                                                            const SpatialPoint P0 = { Item.Points[p][0], Item.Points[p][1], Item.Points[p][2] };
                                                            const SpatialPoint P1 = { Item.Points[p + 1u][0], Item.Points[p + 1u][1], Item.Points[p + 1u][2] };
                                                            const SketchCurveName LineC = CadRuntime.Sketch.DeclareLine(P0, P1);
                                                            WorkspaceRecord Rec = {};
                                                            Rec.Subject = WorkspaceRecordSubject::OpenCurve;
                                                            Rec.Naming = CadNaming.Issue(WorkspaceRecordSubject::OpenCurve);
                                                            Rec.SketchCurve = LineC;
                                                            const WorkspaceRecordName N = CadRuntime.Records.Declare(Rec);
                                                            if (N.Assigned()) Written.push_back(N);
                                                        }

                                                        WorkspaceRecord ProfileRec = {};
                                                        ProfileRec.Subject = WorkspaceRecordSubject::ClosedProfile;
                                                        ProfileRec.Naming = CadNaming.Issue(WorkspaceRecordSubject::ClosedProfile);
                                                        ProfileRec.ClosedSemantic = true;
                                                        ProfileRec.CappedExtrusionSemantic = true;
                                                        Item.CadRecordName = CadRuntime.Records.Declare(ProfileRec);
                                                        if (Item.CadRecordName.Assigned()) Written.push_back(Item.CadRecordName);

                                                        CadRuntime.Revisions.Seal("Declared slot", "Create Slot", Written,
                                                                                  CadRuntime.Revisions.DeclaredCount() + 1u);
                                                        SyncPresentedSceneEntities();
                                                    }
                                                    CadDrawActive = false;
                                                    CadDrawPhase = 0u;
                                                    CadMultiPointCount = 0u;
                                                }
                                                else if (LeafPointer.ContactPressed)
                                                {
                                                    const double Dist = std::sqrt(
                                                        (WorldPt[0] - CadMultiPoints[CadMultiPointCount - 1u][0]) * (WorldPt[0] - CadMultiPoints[CadMultiPointCount - 1u][0]) +
                                                        (WorldPt[1] - CadMultiPoints[CadMultiPointCount - 1u][1]) * (WorldPt[1] - CadMultiPoints[CadMultiPointCount - 1u][1]) +
                                                        (WorldPt[2] - CadMultiPoints[CadMultiPointCount - 1u][2]) * (WorldPt[2] - CadMultiPoints[CadMultiPointCount - 1u][2]));
                                                    if (Dist > 0.05 && CadMultiPointCount < 64u)
                                                    {
                                                        for (int a = 0; a < 3; ++a)
                                                            CadMultiPoints[CadMultiPointCount][a] = WorldPt[a];
                                                        CadMultiPointCount++;
                                                    }
                                                }
                                            }
                                        }
                                        else if (ActiveSubj == ParametricToolSubject::Polyline)
                                        {
                                            if (!CadDrawActive)
                                            {
                                                if (LeafPointer.ContactPressed)
                                                {
                                                    CadDrawActive = true;
                                                    CadDrawPhase = 1u;
                                                    CadMultiPointCount = 1u;
                                                    for (int a = 0; a < 3; ++a)
                                                    {
                                                        CadMultiPoints[0][a] = WorldPt[a];
                                                        CadCurrentWorld[a] = WorldPt[a];
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                if (LeafPointer.ContactDoublePressed)
                                                {
                                                    if (CadMultiPointCount < 64u)
                                                    {
                                                        for (int a = 0; a < 3; ++a)
                                                            CadMultiPoints[CadMultiPointCount][a] = WorldPt[a];
                                                        CadMultiPointCount++;
                                                    }
                                                    if (CadMultiPointCount >= 2u && EditorCadCount < EditorCadLimit)
                                                    {
                                                        EditorCadItem& Item = EditorCadItems[EditorCadCount++];
                                                        Item.Kind = EditorCadKind::Polyline;
                                                        std::snprintf(Item.Name, sizeof(Item.Name), "Polyline %u", CadShapeIndex++);
                                                        for (int a = 0; a < 3; ++a)
                                                        {
                                                            Item.PlaneNormal[a] = CadDrawPlaneNormal[a];
                                                            Item.AxisU[a] = CadDrawAxisU[a];
                                                            Item.AxisV[a] = CadDrawAxisV[a];
                                                            Item.Center[a] = CadMultiPoints[0][a];
                                                        }
                                                        Item.PointCount = CadMultiPointCount;
                                                        for (std::uint32_t p = 0u; p < CadMultiPointCount; ++p)
                                                            for (int a = 0; a < 3; ++a)
                                                                Item.Points[p][a] = CadMultiPoints[p][a];

                                                        if (!CadRuntime.Sketch.Declared())
                                                            CadRuntime.Sketch.DeclarePlane({ { 0.0, 0.0, 0.0 }, { CadDrawPlaneNormal[0], CadDrawPlaneNormal[1], CadDrawPlaneNormal[2] }, { CadDrawAxisU[0], CadDrawAxisU[1], CadDrawAxisU[2] } });

                                                        std::vector<WorkspaceRecordName> Written;
                                                        for (std::uint32_t p = 0u; p + 1u < Item.PointCount; ++p)
                                                        {
                                                            const SpatialPoint P0 = { Item.Points[p][0], Item.Points[p][1], Item.Points[p][2] };
                                                            const SpatialPoint P1 = { Item.Points[p + 1u][0], Item.Points[p + 1u][1], Item.Points[p + 1u][2] };
                                                            const SketchCurveName LineC = CadRuntime.Sketch.DeclareLine(P0, P1);
                                                            WorkspaceRecord Rec = {};
                                                            Rec.Subject = WorkspaceRecordSubject::OpenCurve;
                                                            Rec.Naming = CadNaming.Issue(WorkspaceRecordSubject::OpenCurve);
                                                            Rec.SketchCurve = LineC;
                                                            const WorkspaceRecordName N = CadRuntime.Records.Declare(Rec);
                                                            if (N.Assigned()) Written.push_back(N);
                                                        }
                                                        CadRuntime.Revisions.Seal("Declared polyline", "Create Polyline", Written,
                                                                                  CadRuntime.Revisions.DeclaredCount() + 1u);
                                                        SyncPresentedSceneEntities();
                                                    }
                                                    CadDrawActive = false;
                                                    CadDrawPhase = 0u;
                                                    CadMultiPointCount = 0u;
                                                }
                                                else if (LeafPointer.ContactPressed)
                                                {
                                                    const double Dist = std::sqrt(
                                                        (WorldPt[0] - CadMultiPoints[CadMultiPointCount - 1u][0]) * (WorldPt[0] - CadMultiPoints[CadMultiPointCount - 1u][0]) +
                                                        (WorldPt[1] - CadMultiPoints[CadMultiPointCount - 1u][1]) * (WorldPt[1] - CadMultiPoints[CadMultiPointCount - 1u][1]) +
                                                        (WorldPt[2] - CadMultiPoints[CadMultiPointCount - 1u][2]) * (WorldPt[2] - CadMultiPoints[CadMultiPointCount - 1u][2]));
                                                    if (Dist > 0.05 && CadMultiPointCount < 64u)
                                                    {
                                                        for (int a = 0; a < 3; ++a)
                                                            CadMultiPoints[CadMultiPointCount][a] = WorldPt[a];
                                                        CadMultiPointCount++;
                                                    }
                                                }
                                            }
                                        }
                                        else if (ActiveSubj == ParametricToolSubject::RationalSpline ||
                                                 ActiveSubj == ParametricToolSubject::BasisSpline ||
                                                 ActiveSubj == ParametricToolSubject::HermiteCurve ||
                                                 ActiveSubj == ParametricToolSubject::BezierCurve)
                                        {
                                            if (!CadDrawActive)
                                            {
                                                if (LeafPointer.ContactPressed)
                                                {
                                                    CadDrawActive = true;
                                                    CadDrawPhase = 1u;
                                                    CadMultiPointCount = 1u;
                                                    for (int a = 0; a < 3; ++a)
                                                    {
                                                        CadMultiPoints[0][a] = WorldPt[a];
                                                        CadCurrentWorld[a] = WorldPt[a];
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                if (LeafPointer.ContactDoublePressed)
                                                {
                                                    if (CadMultiPointCount < 64u)
                                                    {
                                                        for (int a = 0; a < 3; ++a)
                                                            CadMultiPoints[CadMultiPointCount][a] = WorldPt[a];
                                                        CadMultiPointCount++;
                                                    }
                                                    if (CadMultiPointCount >= 2u && EditorCadCount < EditorCadLimit)
                                                    {
                                                        EditorCadItem& Item = EditorCadItems[EditorCadCount++];
                                                        Item.Kind = EditorCadKind::Curve;
                                                        const char* CurveLabel = (ActiveSubj == ParametricToolSubject::BezierCurve) ? "Bezier" :
                                                                                 (ActiveSubj == ParametricToolSubject::HermiteCurve) ? "Hermite" :
                                                                                 (ActiveSubj == ParametricToolSubject::BasisSpline) ? "B-Spline" : "NURBS";
                                                        std::snprintf(Item.Name, sizeof(Item.Name), "%s %u", CurveLabel, CadShapeIndex++);
                                                        for (int a = 0; a < 3; ++a)
                                                        {
                                                            Item.PlaneNormal[a] = CadDrawPlaneNormal[a];
                                                            Item.AxisU[a] = CadDrawAxisU[a];
                                                            Item.AxisV[a] = CadDrawAxisV[a];
                                                            Item.Center[a] = CadMultiPoints[0][a];
                                                        }
                                                        Item.PointCount = CadMultiPointCount;
                                                        for (std::uint32_t p = 0u; p < CadMultiPointCount; ++p)
                                                            for (int a = 0; a < 3; ++a)
                                                                Item.Points[p][a] = CadMultiPoints[p][a];

                                                        if (!CadRuntime.Sketch.Declared())
                                                            CadRuntime.Sketch.DeclarePlane({ { 0.0, 0.0, 0.0 }, { CadDrawPlaneNormal[0], CadDrawPlaneNormal[1], CadDrawPlaneNormal[2] }, { CadDrawAxisU[0], CadDrawAxisU[1], CadDrawAxisU[2] } });

                                                        std::vector<WorkspaceRecordName> Written;
                                                        for (std::uint32_t p = 0u; p + 1u < Item.PointCount; ++p)
                                                        {
                                                            const SpatialPoint P0 = { Item.Points[p][0], Item.Points[p][1], Item.Points[p][2] };
                                                            const SpatialPoint P1 = { Item.Points[p + 1u][0], Item.Points[p + 1u][1], Item.Points[p + 1u][2] };
                                                            const SketchCurveName LineC = CadRuntime.Sketch.DeclareLine(P0, P1);
                                                            WorkspaceRecord Rec = {};
                                                            Rec.Subject = WorkspaceRecordSubject::OpenCurve;
                                                            Rec.Naming = CadNaming.Issue(WorkspaceRecordSubject::OpenCurve);
                                                            Rec.SketchCurve = LineC;
                                                            const WorkspaceRecordName N = CadRuntime.Records.Declare(Rec);
                                                            if (N.Assigned()) Written.push_back(N);
                                                        }
                                                        CadRuntime.Revisions.Seal("Declared curve", "Create Curve", Written,
                                                                                  CadRuntime.Revisions.DeclaredCount() + 1u);
                                                        SyncPresentedSceneEntities();
                                                    }
                                                    CadDrawActive = false;
                                                    CadDrawPhase = 0u;
                                                    CadMultiPointCount = 0u;
                                                }
                                                else if (LeafPointer.ContactPressed)
                                                {
                                                    const double Dist = std::sqrt(
                                                        (WorldPt[0] - CadMultiPoints[CadMultiPointCount - 1u][0]) * (WorldPt[0] - CadMultiPoints[CadMultiPointCount - 1u][0]) +
                                                        (WorldPt[1] - CadMultiPoints[CadMultiPointCount - 1u][1]) * (WorldPt[1] - CadMultiPoints[CadMultiPointCount - 1u][1]) +
                                                        (WorldPt[2] - CadMultiPoints[CadMultiPointCount - 1u][2]) * (WorldPt[2] - CadMultiPoints[CadMultiPointCount - 1u][2]));
                                                    if (Dist > 0.05 && CadMultiPointCount < 64u)
                                                    {
                                                        for (int a = 0; a < 3; ++a)
                                                            CadMultiPoints[CadMultiPointCount][a] = WorldPt[a];
                                                        CadMultiPointCount++;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                {
                                    SharedViewportBasis GizmoBasis = SharedViewportBasisFromYawPitch(
                                        SceneApplied.ViewportSkyCamera.AzimuthDegrees,
                                        SceneApplied.ViewportSkyCamera.ElevationDegrees);
                                    const EditorPanelConfiguration& PanelDeclaredForGizmo = PanelConfiguration[Index];
                                    if (LeafPointer.ContactPressed &&
                                        LeafBody.Encloses(LeafPointer.PositionX, LeafPointer.PositionY))
                                    {
                                        const SharedViewportOrientation Hit = HitSharedViewportGizmo(
                                            LeafBody, GizmoBasis, LeafPointer.PositionX, LeafPointer.PositionY,
                                            PanelDeclaredForGizmo.Gizmo == PanelGizmo::Cad);
                                        if (Hit != SharedViewportOrientation::None)
                                        {
                                            double Yaw = EditorCamera.YawDegrees;
                                            double Pitch = EditorCamera.PitchDegrees;
                                            SharedViewportOrientationPreset(Hit, Yaw, Pitch);
                                            EditorCamera.YawDegrees = Yaw;
                                            EditorCamera.PitchDegrees = Pitch;
                                            EditorCamera.Snap();
                                            GizmoBasis = SharedViewportBasisFromYawPitch(Yaw, Pitch);
                                        }
                                    }
                                    RecordSharedViewportGizmo(Viewport.Surface(), LeafBody, GizmoBasis,
                                                              PanelDeclaredForGizmo.Gizmo == PanelGizmo::Cad);
                                }

                                // 📐 The ground lattice is no longer recorded here. It is solved per
                                //    pixel in the overlay pass's mode 3, from the camera pushed below,
                                //    so the CPU hands over a pose rather than a thousand segments.
                                SceneDirectory.RecordGizmo(LeafBody, SceneApplied, LeafOverlay);

                                // Render committed and preview CAD geometry into the viewport overlay
                                const std::uint32_t CommittedColor = PackOverlayColour(52u, 211u, 153u, 255u);
                                const std::uint32_t PreviewColor   = PackOverlayColour(0u, 229u, 255u, 255u);
                                const std::uint32_t HandleColor    = PackOverlayColour(255u, 255u, 255u, 255u);
                                const std::uint32_t MutedColor     = PackOverlayColour(0u, 229u, 255u, 120u);

                                for (std::uint32_t I = 0u; I < EditorCadCount; ++I)
                                {
                                    const EditorCadItem& Item = EditorCadItems[I];
                                    if (Item.Kind == EditorCadKind::Point)
                                    {
                                        float SX = 0.0f, SY = 0.0f;
                                        if (ProjectPoint(Item.Center, SX, SY))
                                            SafeOverlayDot(SX, SY, CommittedColor, 4.0f);
                                    }
                                    else if (Item.Kind == EditorCadKind::Line)
                                    {
                                        float S0X = 0.0f, S0Y = 0.0f, S1X = 0.0f, S1Y = 0.0f;
                                        if (ProjectPoint(Item.LineStart, S0X, S0Y) && ProjectPoint(Item.LineEnd, S1X, S1Y))
                                        {
                                            SafeOverlayLine(S0X, S0Y, S1X, S1Y, CommittedColor, 2.0f);
                                            SafeOverlayDot(S0X, S0Y, HandleColor, 3.0f);
                                            SafeOverlayDot(S1X, S1Y, HandleColor, 3.0f);
                                        }
                                    }
                                    else if (Item.Kind == EditorCadKind::Rectangle)
                                    {
                                        float Sc[4][2] = {};
                                        bool AllVis = true;
                                        for (int c = 0; c < 4; ++c)
                                        {
                                            if (!ProjectPoint(Item.RectCorners[c], Sc[c][0], Sc[c][1]))
                                                AllVis = false;
                                        }
                                        if (AllVis)
                                        {
                                            for (int c = 0; c < 4; ++c)
                                            {
                                                SafeOverlayLine(Sc[c][0], Sc[c][1], Sc[(c + 1) % 4][0], Sc[(c + 1) % 4][1], CommittedColor, 2.0f);
                                                SafeOverlayDot(Sc[c][0], Sc[c][1], HandleColor, 3.0f);
                                            }
                                        }
                                    }
                                    else if (Item.Kind == EditorCadKind::Circle)
                                    {
                                        float PrevSX = 0.0f, PrevSY = 0.0f;
                                        bool First = true;
                                        constexpr int Segments = 48;
                                        for (int s = 0; s <= Segments; ++s)
                                        {
                                            const double Ang = static_cast<double>(s % Segments) * 2.0 * 3.14159265358979323846 / static_cast<double>(Segments);
                                            double WP[3] = {};
                                            for (int a = 0; a < 3; ++a)
                                                WP[a] = Item.Center[a] + Item.AxisU[a] * (Item.RadiusMajor * std::cos(Ang)) + Item.AxisV[a] * (Item.RadiusMajor * std::sin(Ang));
                                            float CurSX = 0.0f, CurSY = 0.0f;
                                            if (ProjectPoint(WP, CurSX, CurSY))
                                            {
                                                if (!First)
                                                    SafeOverlayLine(PrevSX, PrevSY, CurSX, CurSY, CommittedColor, 2.0f);
                                                else
                                                    First = false;
                                                PrevSX = CurSX; PrevSY = CurSY;
                                            }
                                        }
                                        float CSX = 0.0f, CSY = 0.0f;
                                        if (ProjectPoint(Item.Center, CSX, CSY))
                                            SafeOverlayDot(CSX, CSY, HandleColor, 3.0f);
                                    }
                                    else if (Item.Kind == EditorCadKind::Polygon)
                                    {
                                        for (std::uint32_t s = 0u; s < Item.PolygonSides; ++s)
                                        {
                                            const std::uint32_t Next = (s + 1u) % Item.PolygonSides;
                                            float S0X = 0.0f, S0Y = 0.0f, S1X = 0.0f, S1Y = 0.0f;
                                            if (ProjectPoint(Item.Points[s], S0X, S0Y) && ProjectPoint(Item.Points[Next], S1X, S1Y))
                                            {
                                                SafeOverlayLine(S0X, S0Y, S1X, S1Y, CommittedColor, 2.0f);
                                                SafeOverlayDot(S0X, S0Y, HandleColor, 3.0f);
                                            }
                                        }
                                    }
                                    else if (Item.Kind == EditorCadKind::Arc)
                                    {
                                        float PrevSX = 0.0f, PrevSY = 0.0f;
                                        bool First = true;
                                        constexpr int Segments = 32;
                                        for (int s = 0; s <= Segments; ++s)
                                        {
                                            const double t = static_cast<double>(s) / static_cast<double>(Segments);
                                            double WP[3] = {};
                                            for (int a = 0; a < 3; ++a)
                                                WP[a] = (1.0 - t) * (1.0 - t) * Item.LineStart[a] + 2.0 * (1.0 - t) * t * Item.ArcMid[a] + t * t * Item.LineEnd[a];
                                            float CurSX = 0.0f, CurSY = 0.0f;
                                            if (ProjectPoint(WP, CurSX, CurSY))
                                            {
                                                if (!First)
                                                    SafeOverlayLine(PrevSX, PrevSY, CurSX, CurSY, CommittedColor, 2.0f);
                                                else
                                                    First = false;
                                                PrevSX = CurSX; PrevSY = CurSY;
                                            }
                                        }
                                        float S0X = 0.0f, S0Y = 0.0f, S1X = 0.0f, S1Y = 0.0f;
                                        if (ProjectPoint(Item.LineStart, S0X, S0Y)) SafeOverlayDot(S0X, S0Y, HandleColor, 3.0f);
                                        if (ProjectPoint(Item.LineEnd, S1X, S1Y)) SafeOverlayDot(S1X, S1Y, HandleColor, 3.0f);
                                    }
                                    else if (Item.Kind == EditorCadKind::Ellipse)
                                    {
                                        float PrevSX = 0.0f, PrevSY = 0.0f;
                                        bool First = true;
                                        constexpr int Segments = 48;
                                        for (int s = 0; s <= Segments; ++s)
                                        {
                                            const double Ang = static_cast<double>(s % Segments) * 2.0 * 3.14159265358979323846 / static_cast<double>(Segments);
                                            double WP[3] = {};
                                            for (int a = 0; a < 3; ++a)
                                                WP[a] = Item.Center[a] + Item.AxisU[a] * (Item.RadiusMajor * std::cos(Ang)) + Item.AxisV[a] * (Item.RadiusMinor * std::sin(Ang));
                                            float CurSX = 0.0f, CurSY = 0.0f;
                                            if (ProjectPoint(WP, CurSX, CurSY))
                                            {
                                                if (!First)
                                                    SafeOverlayLine(PrevSX, PrevSY, CurSX, CurSY, CommittedColor, 2.0f);
                                                else
                                                    First = false;
                                                PrevSX = CurSX; PrevSY = CurSY;
                                            }
                                        }
                                        float CSX = 0.0f, CSY = 0.0f;
                                        if (ProjectPoint(Item.Center, CSX, CSY))
                                            SafeOverlayDot(CSX, CSY, HandleColor, 3.0f);
                                    }
                                    else if (Item.Kind == EditorCadKind::Slot || Item.Kind == EditorCadKind::Polyline || Item.Kind == EditorCadKind::Curve)
                                    {
                                        for (std::uint32_t p = 0u; p + 1u < Item.PointCount; ++p)
                                        {
                                            float S0X = 0.0f, S0Y = 0.0f, S1X = 0.0f, S1Y = 0.0f;
                                            if (ProjectPoint(Item.Points[p], S0X, S0Y) && ProjectPoint(Item.Points[p + 1u], S1X, S1Y))
                                            {
                                                SafeOverlayLine(S0X, S0Y, S1X, S1Y, CommittedColor, 2.0f);
                                                SafeOverlayDot(S0X, S0Y, HandleColor, 3.0f);
                                            }
                                        }
                                        if (Item.PointCount > 0u)
                                        {
                                            float LastSX = 0.0f, LastSY = 0.0f;
                                            if (ProjectPoint(Item.Points[Item.PointCount - 1u], LastSX, LastSY))
                                                SafeOverlayDot(LastSX, LastSY, HandleColor, 3.0f);
                                        }
                                    }
                                }

                                if (CadDrawActive)
                                {
                                    if (CadActiveTool == ParametricToolSubject::Line)
                                    {
                                        float S0X = 0.0f, S0Y = 0.0f, S1X = 0.0f, S1Y = 0.0f;
                                        if (ProjectPoint(CadStartWorld, S0X, S0Y) && ProjectPoint(CadCurrentWorld, S1X, S1Y))
                                        {
                                            SafeOverlayLine(S0X, S0Y, S1X, S1Y, PreviewColor, 2.0f);
                                            SafeOverlayDot(S0X, S0Y, HandleColor, 4.0f);
                                            SafeOverlayDot(S1X, S1Y, HandleColor, 4.0f);
                                        }
                                    }
                                    else if (CadActiveTool == ParametricToolSubject::Rectangle || CadActiveTool == ParametricToolSubject::CenterRectangle)
                                    {
                                        const double Diff[3] = { CadCurrentWorld[0] - CadStartWorld[0],
                                                                 CadCurrentWorld[1] - CadStartWorld[1],
                                                                 CadCurrentWorld[2] - CadStartWorld[2] };
                                        const double DU = Diff[0] * CadDrawAxisU[0] + Diff[1] * CadDrawAxisU[1] + Diff[2] * CadDrawAxisU[2];
                                        const double DV = Diff[0] * CadDrawAxisV[0] + Diff[1] * CadDrawAxisV[1] + Diff[2] * CadDrawAxisV[2];

                                        double Corners[4][3] = {};
                                        for (int a = 0; a < 3; ++a)
                                        {
                                            Corners[0][a] = CadStartWorld[a];
                                            Corners[1][a] = CadStartWorld[a] + CadDrawAxisU[a] * DU;
                                            Corners[2][a] = CadCurrentWorld[a];
                                            Corners[3][a] = CadStartWorld[a] + CadDrawAxisV[a] * DV;
                                        }
                                        float Sc[4][2] = {};
                                        bool AllVis = true;
                                        for (int c = 0; c < 4; ++c)
                                        {
                                            if (!ProjectPoint(Corners[c], Sc[c][0], Sc[c][1]))
                                                AllVis = false;
                                        }
                                        if (AllVis)
                                        {
                                            for (int c = 0; c < 4; ++c)
                                            {
                                                SafeOverlayLine(Sc[c][0], Sc[c][1], Sc[(c + 1) % 4][0], Sc[(c + 1) % 4][1], PreviewColor, 2.0f);
                                                SafeOverlayDot(Sc[c][0], Sc[c][1], HandleColor, 4.0f);
                                            }
                                        }
                                    }
                                    else if (CadActiveTool == ParametricToolSubject::Circle)
                                    {
                                        const double Diff[3] = { CadCurrentWorld[0] - CadStartWorld[0],
                                                                 CadCurrentWorld[1] - CadStartWorld[1],
                                                                 CadCurrentWorld[2] - CadStartWorld[2] };
                                        const double Radius = std::sqrt(Diff[0] * Diff[0] + Diff[1] * Diff[1] + Diff[2] * Diff[2]);

                                        float PrevSX = 0.0f, PrevSY = 0.0f;
                                        bool First = true;
                                        constexpr int Segments = 48;
                                        for (int s = 0; s <= Segments; ++s)
                                        {
                                            const double Ang = static_cast<double>(s % Segments) * 2.0 * 3.14159265358979323846 / static_cast<double>(Segments);
                                            double WP[3] = {};
                                            for (int a = 0; a < 3; ++a)
                                                WP[a] = CadStartWorld[a] + CadDrawAxisU[a] * (Radius * std::cos(Ang)) + CadDrawAxisV[a] * (Radius * std::sin(Ang));
                                            float CurSX = 0.0f, CurSY = 0.0f;
                                            if (ProjectPoint(WP, CurSX, CurSY))
                                            {
                                                if (!First)
                                                    SafeOverlayLine(PrevSX, PrevSY, CurSX, CurSY, PreviewColor, 2.0f);
                                                else
                                                    First = false;
                                                PrevSX = CurSX; PrevSY = CurSY;
                                            }
                                        }
                                        float CenterSX = 0.0f, CenterSY = 0.0f;
                                        float CurSX = 0.0f, CurSY = 0.0f;
                                        if (ProjectPoint(CadStartWorld, CenterSX, CenterSY))
                                            SafeOverlayDot(CenterSX, CenterSY, HandleColor, 4.0f);
                                        if (ProjectPoint(CadStartWorld, CenterSX, CenterSY) && ProjectPoint(CadCurrentWorld, CurSX, CurSY))
                                            SafeOverlayLine(CenterSX, CenterSY, CurSX, CurSY, MutedColor, 1.0f);
                                    }
                                    else if (CadActiveTool == ParametricToolSubject::Polygon)
                                    {
                                        const double Diff[3] = { CadCurrentWorld[0] - CadStartWorld[0],
                                                                 CadCurrentWorld[1] - CadStartWorld[1],
                                                                 CadCurrentWorld[2] - CadStartWorld[2] };
                                        const double Radius = std::sqrt(Diff[0] * Diff[0] + Diff[1] * Diff[1] + Diff[2] * Diff[2]);
                                        const double DU = Diff[0] * CadDrawAxisU[0] + Diff[1] * CadDrawAxisU[1] + Diff[2] * CadDrawAxisU[2];
                                        const double DV = Diff[0] * CadDrawAxisV[0] + Diff[1] * CadDrawAxisV[1] + Diff[2] * CadDrawAxisV[2];
                                        const double Theta0 = std::atan2(DV, DU);

                                        float PrevSX = 0.0f, PrevSY = 0.0f;
                                        float FirstSX = 0.0f, FirstSY = 0.0f;
                                        for (std::uint32_t s = 0u; s < CadPolygonSides; ++s)
                                        {
                                            const double Ang = Theta0 + static_cast<double>(s) * 2.0 * 3.14159265358979323846 / static_cast<double>(CadPolygonSides);
                                            double WP[3] = {};
                                            for (int a = 0; a < 3; ++a)
                                                WP[a] = CadStartWorld[a] + CadDrawAxisU[a] * (Radius * std::cos(Ang)) + CadDrawAxisV[a] * (Radius * std::sin(Ang));
                                            float CurSX = 0.0f, CurSY = 0.0f;
                                            if (ProjectPoint(WP, CurSX, CurSY))
                                            {
                                                SafeOverlayDot(CurSX, CurSY, HandleColor, 3.5f);
                                                if (s > 0u)
                                                    SafeOverlayLine(PrevSX, PrevSY, CurSX, CurSY, PreviewColor, 2.0f);
                                                else
                                                {
                                                    FirstSX = CurSX;
                                                    FirstSY = CurSY;
                                                }
                                                PrevSX = CurSX; PrevSY = CurSY;
                                            }
                                        }
                                        SafeOverlayLine(PrevSX, PrevSY, FirstSX, FirstSY, PreviewColor, 2.0f);
                                        float CSX = 0.0f, CSY = 0.0f;
                                        if (ProjectPoint(CadStartWorld, CSX, CSY))
                                            SafeOverlayDot(CSX, CSY, HandleColor, 4.0f);
                                    }
                                    else if (CadActiveTool == ParametricToolSubject::Arc)
                                    {
                                        const double* P0 = CadStartWorld;
                                        const double* P1 = (CadDrawPhase == 2u) ? CadAuxWorld : CadCurrentWorld;
                                        double Pmid[3] = {};
                                        if (CadDrawPhase == 2u)
                                        {
                                            for (int a = 0; a < 3; ++a)
                                                Pmid[a] = CadCurrentWorld[a];
                                        }
                                        else
                                        {
                                            const double D[3] = { P1[0] - P0[0], P1[1] - P0[1], P1[2] - P0[2] };
                                            const double L = std::sqrt(D[0] * D[0] + D[1] * D[1] + D[2] * D[2]);
                                            if (L > 0.01)
                                            {
                                                const double InvL = 1.0 / L;
                                                const double Dx = D[0] * InvL, Dy = D[1] * InvL, Dz = D[2] * InvL;
                                                const double Nx = CadDrawPlaneNormal[1] * Dz - CadDrawPlaneNormal[2] * Dy;
                                                const double Ny = CadDrawPlaneNormal[2] * Dx - CadDrawPlaneNormal[0] * Dz;
                                                const double Nz = CadDrawPlaneNormal[0] * Dy - CadDrawPlaneNormal[1] * Dx;
                                                for (int a = 0; a < 3; ++a)
                                                    Pmid[a] = (P0[a] + P1[a]) * 0.5;
                                                Pmid[0] += Nx * (L * 0.28);
                                                Pmid[1] += Ny * (L * 0.28);
                                                Pmid[2] += Nz * (L * 0.28);
                                            }
                                            else
                                            {
                                                for (int a = 0; a < 3; ++a)
                                                    Pmid[a] = P0[a];
                                            }
                                        }

                                        float PrevSX = 0.0f, PrevSY = 0.0f;
                                        bool First = true;
                                        constexpr int Segments = 32;
                                        for (int s = 0; s <= Segments; ++s)
                                        {
                                            const double t = static_cast<double>(s) / static_cast<double>(Segments);
                                            double WP[3] = {};
                                            for (int a = 0; a < 3; ++a)
                                                WP[a] = (1.0 - t) * (1.0 - t) * P0[a] + 2.0 * (1.0 - t) * t * Pmid[a] + t * t * P1[a];
                                            float CurSX = 0.0f, CurSY = 0.0f;
                                            if (ProjectPoint(WP, CurSX, CurSY))
                                            {
                                                if (!First)
                                                    SafeOverlayLine(PrevSX, PrevSY, CurSX, CurSY, PreviewColor, 2.0f);
                                                else
                                                    First = false;
                                                PrevSX = CurSX; PrevSY = CurSY;
                                            }
                                        }
                                        float S0X = 0.0f, S0Y = 0.0f, S1X = 0.0f, S1Y = 0.0f;
                                        if (ProjectPoint(P0, S0X, S0Y)) SafeOverlayDot(S0X, S0Y, HandleColor, 4.0f);
                                        if (ProjectPoint(P1, S1X, S1Y)) SafeOverlayDot(S1X, S1Y, HandleColor, 4.0f);
                                    }
                                    else if (CadActiveTool == ParametricToolSubject::Ellipse)
                                    {
                                        const double Diff[3] = { CadCurrentWorld[0] - CadStartWorld[0],
                                                                 CadCurrentWorld[1] - CadStartWorld[1],
                                                                 CadCurrentWorld[2] - CadStartWorld[2] };
                                        double DU = std::abs(Diff[0] * CadDrawAxisU[0] + Diff[1] * CadDrawAxisU[1] + Diff[2] * CadDrawAxisU[2]);
                                        double DV = std::abs(Diff[0] * CadDrawAxisV[0] + Diff[1] * CadDrawAxisV[1] + Diff[2] * CadDrawAxisV[2]);
                                        const double Dist = std::sqrt(Diff[0] * Diff[0] + Diff[1] * Diff[1] + Diff[2] * Diff[2]);
                                        if (DU < 0.05 && DV < 0.05) { DU = DV = Dist; }
                                        else { DU = std::max(DU, 0.05); DV = std::max(DV, 0.05); }

                                        float PrevSX = 0.0f, PrevSY = 0.0f;
                                        bool First = true;
                                        constexpr int Segments = 48;
                                        for (int s = 0; s <= Segments; ++s)
                                        {
                                            const double Ang = static_cast<double>(s % Segments) * 2.0 * 3.14159265358979323846 / static_cast<double>(Segments);
                                            double WP[3] = {};
                                            for (int a = 0; a < 3; ++a)
                                                WP[a] = CadStartWorld[a] + CadDrawAxisU[a] * (DU * std::cos(Ang)) + CadDrawAxisV[a] * (DV * std::sin(Ang));
                                            float CurSX = 0.0f, CurSY = 0.0f;
                                            if (ProjectPoint(WP, CurSX, CurSY))
                                            {
                                                if (!First)
                                                    SafeOverlayLine(PrevSX, PrevSY, CurSX, CurSY, PreviewColor, 2.0f);
                                                else
                                                    First = false;
                                                PrevSX = CurSX; PrevSY = CurSY;
                                            }
                                        }
                                        float CSX = 0.0f, CSY = 0.0f;
                                        if (ProjectPoint(CadStartWorld, CSX, CSY))
                                            SafeOverlayDot(CSX, CSY, HandleColor, 4.0f);
                                    }
                                    else if (CadActiveTool == ParametricToolSubject::Slot ||
                                             CadActiveTool == ParametricToolSubject::Polyline ||
                                             CadActiveTool == ParametricToolSubject::RationalSpline ||
                                             CadActiveTool == ParametricToolSubject::BasisSpline ||
                                             CadActiveTool == ParametricToolSubject::HermiteCurve ||
                                             CadActiveTool == ParametricToolSubject::BezierCurve)
                                    {
                                        for (std::uint32_t p = 0u; p < CadMultiPointCount; ++p)
                                        {
                                            float SX = 0.0f, SY = 0.0f;
                                            if (ProjectPoint(CadMultiPoints[p], SX, SY))
                                                SafeOverlayDot(SX, SY, HandleColor, 3.5f);
                                            if (p + 1u < CadMultiPointCount)
                                            {
                                                float S0X = 0.0f, S0Y = 0.0f, S1X = 0.0f, S1Y = 0.0f;
                                                if (ProjectPoint(CadMultiPoints[p], S0X, S0Y) && ProjectPoint(CadMultiPoints[p + 1u], S1X, S1Y))
                                                    SafeOverlayLine(S0X, S0Y, S1X, S1Y, PreviewColor, 2.0f);
                                            }
                                        }
                                        if (CadMultiPointCount > 0u)
                                        {
                                            float S0X = 0.0f, S0Y = 0.0f, S1X = 0.0f, S1Y = 0.0f;
                                            if (ProjectPoint(CadMultiPoints[CadMultiPointCount - 1u], S0X, S0Y) && ProjectPoint(CadCurrentWorld, S1X, S1Y))
                                                SafeOverlayLine(S0X, S0Y, S1X, S1Y, PreviewColor, 2.0f);
                                        }
                                    }
                                }
                                // 📐 The pose the analytic ground reads. Assembled here because the
                                //    host owns the EditorCameraComponent and the leaf's extent both.
                                {
                                    OverlayGroundPose& Pose = LeafOverlay.Ground;
                                    const EditorPanelConfiguration& PanelDeclared = PanelConfiguration[Index];

                                    Pose.Standing = PanelDeclared.Lattice != PanelLatticePresentation::None;

                                    const double Yaw   = SceneApplied.ViewportSkyCamera.AzimuthDegrees
                                                       * 3.14159265358979323846 / 180.0;
                                    const double Pitch = SceneApplied.ViewportSkyCamera.ElevationDegrees
                                                       * 3.14159265358979323846 / 180.0;
                                    const double CosP = std::cos(Pitch), SinP = std::sin(Pitch);
                                    const double SinY = std::sin(Yaw),   CosY = std::cos(Yaw);

                                    Pose.EyeX = static_cast<float>(SceneApplied.CameraPosition[0]);
                                    Pose.EyeY = static_cast<float>(SceneApplied.CameraPosition[1]);
                                    Pose.EyeZ = static_cast<float>(SceneApplied.CameraPosition[2]);

                                    Pose.ForwardX = static_cast<float>(CosP * SinY);
                                    Pose.ForwardY = static_cast<float>(SinP);
                                    Pose.ForwardZ = static_cast<float>(CosP * CosY);
                                    Pose.RightX   = static_cast<float>(CosY);
                                    Pose.RightY   = 0.0f;
                                    Pose.RightZ   = static_cast<float>(-SinY);
                                    Pose.UpX      = static_cast<float>(-SinP * SinY);
                                    Pose.UpY      = static_cast<float>(CosP);
                                    Pose.UpZ      = static_cast<float>(-SinP * CosY);

                                    const double HalfV = SceneApplied.ViewportSkyCamera.FieldOfViewDegrees
                                                       * 0.5 * 3.14159265358979323846 / 180.0;
                                    const double Aspect = (LeafBody.Height() > 0.0f)
                                                        ? static_cast<double>(LeafBody.Width())
                                                        / static_cast<double>(LeafBody.Height()) : 1.0;

                                    Pose.TanHalfV = static_cast<float>(std::tan(HalfV));
                                    Pose.TanHalfH = static_cast<float>(std::tan(HalfV) * Aspect);

                                    const double DeclaredCell = PanelDeclared.LatticeCellMetres > 0.0
                                                              ? PanelDeclared.LatticeCellMetres : 1.0;
                                    Pose.Cell = static_cast<float>(DeclaredCell
                                              * static_cast<double>(PanelDeclared.LatticeScale));

                                    Pose.Presentation = static_cast<std::uint32_t>(PanelDeclared.Lattice);
                                    Pose.LineWeight   = PanelDeclared.LatticeLineWeight;
                                    Pose.DotRadius    = PanelDeclared.LatticeDotRadius;
                                    Pose.Subdivisions = PanelDeclared.Subdivisions > 0u
                                                      ? static_cast<float>(PanelDeclared.Subdivisions) : 10.0f;
                                    Pose.ExtentMetres = static_cast<float>(
                                        std::max(PanelDeclared.LatticeExtentMetres, DeclaredCell));
                                    Pose.FadeRadiusMetres = static_cast<float>(
                                        std::max(PanelDeclared.LatticeFadeRadiusMetres, DeclaredCell));

                                    std::uint32_t Mask = 0u;
                                    if (PanelDeclared.AxisX) Mask |= 1u;
                                    if (PanelDeclared.AxisY) Mask |= 2u;
                                    if (PanelDeclared.AxisZ) Mask |= 4u;
                                    Pose.AxisMask = Mask;
                                }

                                // 🔴 When the GPU overlay pass could not stand (a build that lowered no
                                //    shaders, or a device that refused it), the SAME record is drawn
                                //    through the interface so the grid, the axes and the gizmo are
                                //    ALWAYS visible — the editor must never silently lose its overlay.
                                //    The upload/record block below then has no pass to draw and skips.
                                if (!Overlay.Standing())
                                    SceneDirectory.RecordOverlayFallback(LeafBody, LeafOverlay);

                                if (ViewportLeafTally < PanelStructure::RecordLimit)
                                {
                                    ViewportLeafIndexs[ViewportLeafTally] = Leaf;
                                    ViewportLeafRects[ViewportLeafTally]    = LeafBody;
                                    ++ViewportLeafTally;
                                }
                                break;
                            }
                            case PanelSubject::SketchDirectory:
                            {
                                CadDirectoryProjection.Reclaim();
                                ProjectWorkspaceDirectory(CadRuntime.Records, CadDirectoryProjection);
                                Discard(BridgeParametricDirectory(CadDirectoryProjection, CadBridgeStorage));
                                SketchDirectory.RecordOutliner(LeafBody, SketchDirectoryApplied,
                                                              CadBridgeStorage.DirectoryRows.data(),
                                                              static_cast<std::uint32_t>(CadBridgeStorage.DirectoryRows.size()),
                                                              nullptr, nullptr, 0u);
                                break;
                            }

                            case PanelSubject::ParametricTools:
                                ParametricTools.Record(LeafBody, ParametricToolsApplied);
                                break;

                            case PanelSubject::Outliner:
                                if (PanelConfiguration[Index].FooterDemand == EditorFooterDemand::SceneImport ||
                                    PanelConfiguration[Index].FooterDemand == EditorFooterDemand::SceneExport)
                                {
                                    SceneApplied.TransferMode =
                                        PanelConfiguration[Index].FooterDemand == EditorFooterDemand::SceneExport ? 1u : 0u;
                                    SceneApplied.OutlinePage = 2u;
                                    PanelConfiguration[Index].FooterDemand = EditorFooterDemand::None;
                                }
                                SceneDirectory.RecordOutliner(LeafBody, SceneApplied, PresentedEntities, PresentedEntityCount);
                                if (SceneApplied.TransferDemand == SceneTransferDemand::Import)
                                {
                                    const Outcome<GeometryAssetView> Imported = GeometryTransfer.Import(
                                        SceneApplied.TransferLocation, SceneApplied.TransferName,
                                        ImportedGeometry, ImportedIntake);
                                    if (!Imported.Resolved)
                                    {
                                        std::printf("%s — geometry import refused (reason %u: %s)\n", HostName,
                                                    static_cast<unsigned>(Imported.Error.DeclaredReason), Imported.Error.Detail);
                                    }
                                    else
                                    {
                                        const Outcome<OwnerIdentity> Owner = ImportedOwners.Register();
                                        const std::uint32_t Base = ImportedVisibility.DeclaredPartitionCount();
                                        const Outcome<std::uint32_t> Registered = Owner.Resolved
                                            ? ImportedVisibility.Register(Owner.Resolve(), *Imported.Resolve().Topology,
                                                                         *Imported.Resolve().Conditioning, ImportedPartitions)
                                            : Outcome<std::uint32_t>::Refuse(Owner.Error);
                                        const Outcome<GeometryRenderingIdentity> Rendered = Registered.Resolved
                                            ? ImportedRendering.Synchronise(Imported.Resolve())
                                            : Outcome<GeometryRenderingIdentity>::Refuse(Registered.Error);
                                        if (!Rendered.Resolved)
                                        {
                                            std::printf("%s — imported topology could not prepare visibility (reason %u: %s)\n",
                                                        HostName, static_cast<unsigned>(Rendered.Error.DeclaredReason),
                                                        Rendered.Error.Detail);
                                        }
                                        else
                                        {
                                            PendingRendering = Rendered.Resolve();
                                            PendingVisibilityRegistration = Registered.Resolve();
                                            PendingRegistrationBase = Base;
                                            GeometryAdmissionPending = true;
                                        }
                                    }
                                    SceneApplied.TransferDemand = SceneTransferDemand::None;
                                }
                                break;
                            case PanelSubject::Properties:
                                SceneDirectory.RecordProperties(LeafBody, SceneApplied,
                                                                PresentedEntities, PresentedEntityCount, SceneApplied.InspectorTab);
                                break;
                            case PanelSubject::TexturePaint:
                                if (PanelConfiguration[Index].FooterDemand == EditorFooterDemand::ExportFlattened ||
                                    PanelConfiguration[Index].FooterDemand == EditorFooterDemand::LayerExport)
                                {
                                    TexturePaintApplied.ExportMode =
                                        PanelConfiguration[Index].FooterDemand == EditorFooterDemand::LayerExport ? 1u : 0u;
                                    TexturePaintApplied.StackPage = 2u;

                                    WorkspaceMaterialRecord ExportMaterial;
                                    ExportMaterial.Reference = TexturePaintApplied.ExportName;
                                    ExportMaterial.Material = EditorMaterialDocument;
                                    ExportMaterial.Layers = EditorMaterialLayers;
                                    MaterialExportOptions ExportOptions;
                                    ExportOptions.OutputName = TexturePaintApplied.ExportName;
                                    ExportOptions.OutputDirectory = TexturePaintApplied.ExportLocation;
                                    ExportOptions.Target = static_cast<MaterialExportTarget>(
                                        std::min(TexturePaintApplied.ExportPreset,
                                                 static_cast<std::uint32_t>(MaterialExportTarget::TargetCount) - 1u));
                                    ExportOptions.Format = TexturePaintApplied.ExportFormat == 1u
                                        ? MaterialExportImageFormat::Tga : MaterialExportImageFormat::Png;
                                    ExportOptions.BitDepth = static_cast<MaterialExportBitDepth>(
                                        std::min(TexturePaintApplied.ExportBitDepth,
                                                 static_cast<std::uint32_t>(MaterialExportBitDepth::DepthCount) - 1u));
                                    ExportOptions.NormalConvention = TexturePaintApplied.ExportDirectXNormals
                                        ? MaterialExportNormalConvention::DirectX : MaterialExportNormalConvention::OpenGl;
                                    ExportOptions.Resolution = 128u << std::min(TexturePaintApplied.ExportResolution, 7u);
                                    ExportOptions.Dilation = TexturePaintApplied.ExportDilation;
                                    const Outcome<MaterialExportPackage> ExportPackage =
                                        BuildMaterialExportPackage(ExportMaterial, ExportOptions);
                                    if (ExportPackage.Resolved)
                                        Discard(MaterialTextureExport().WritePackage(ExportMaterial, ExportPackage.Resolve()));

                                    PanelConfiguration[Index].FooterDemand = EditorFooterDemand::None;
                                }
                                TexturePaint.Record(LeafBody, TexturePaintApplied, StackRows.Rows, StackRows.Count);

                                if (LayerLeafTally < PanelStructure::RecordLimit)
                                {
                                    LayerLeafRects[LayerLeafTally] = LeafBody;
                                    ++LayerLeafTally;
                                }
                                break;
                            default:
                                break;
                        }
                    }

                    Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Above));
                    WorkspacePanels.RecordDeferredPopups(PanelPartitions[Index],
                                                         PanelConfiguration[Index]);
                    Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Beneath));

                    if (WorkspacePanels.PointerCaptured(Index))
                        Viewport.Seam().WithholdPointer();
                }

                Viewport.Seam().LeaveWorkspaceWindow();
                Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Beneath));

                // ⚠️ Recorded, never acted on inside the sweep. Withdrawing here edits the set being walked.
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
                    PanelConfiguration[Moving]   = PanelConfiguration[Moving + 1u];
                }
                PanelPartitions[OpenCount - 1u].Reset();
                PanelConfiguration[OpenCount - 1u] = EditorPanelConfiguration{};
            }

            // 📝 The `+`, applied inside the dock node's own tab bar so the vendor lays it after the last
            //    tab — always at the end, by construction rather than by arithmetic.
            std::uint32_t AskingNode = 0u;

            if (Viewport.Seam().RecordWorkspaceAddition(Workspace.Strip(), OpenCount, AskingNode))
            {
                // 🔴 The asking node is carried to the NEXT tick, because the workspace it registers is not
                //    recorded until then. Applying it against the main space instead is what put a new
                //    workspace in the wrong window.
                RegisterIntoNode = AskingNode;
                const Outcome<std::uint32_t> RegisteredWorkspace = Workspaces.Register(DefaultSubject);
                if (RegisteredWorkspace.Resolved)
                    PanelPartitions[RegisteredWorkspace.Resolve()].ConstructPanelPartition(PanelSubject::Viewport);
            }

            // 🔴 With nothing open there is no tab bar to seat a `+` in, so the empty shell carries the
            //    invitation itself. `WorkspacePanel` draws "CREATE PANEL" on plain black; a press anywhere
            //    on that ground registers one, which is the way out of a state that otherwise has none.
            if (OpenCount == 0u && Viewport.Seam().VacantPressed(Whole))
            {
                const Outcome<std::uint32_t> RegisteredWorkspace = Workspaces.Register(DefaultSubject);
                if (RegisteredWorkspace.Resolved)
                    PanelPartitions[RegisteredWorkspace.Resolve()].ConstructPanelPartition(PanelSubject::Viewport);
            }

            // 📝 The drawers last, so they sit ABOVE the workspace as the sheet lays them.
            // ④·b The scene directory — the shared index is advanced here, once, before the panel
            //      samples it; the panel's own Advance only samples, and a second advance would retire
            //      the release before the leaves read it.
            SceneInteraction.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);
            ParametricInteraction.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);

            // 📐 Tab is shared by the scene directory's pages and the layer stack's carousel, so the
            //    key goes to whichever panel the pointer is over: a TexturePaint leaf feeds the layer
            //    stack, anything else feeds the scene directory. With no TexturePaint leaf open, the
            //    scene directory keeps Tab as before.
            const bool TabPressed = Viewport.Seam().KeyPressed(KeySubject::Summon);
            const PointerCondition& Hovered = Viewport.Surface().Pointer();
            bool PointerInLayers = LayerLeafTally > 0u;

            if (PointerInLayers)
            {
                PointerInLayers = false;

                for (std::uint32_t Index = 0u; Index < LayerLeafTally; ++Index)
                {
                    if (LayerLeafRects[Index].Encloses(Hovered.PositionX, Hovered.PositionY))
                    {
                        PointerInLayers = true;
                        break;
                    }
                }
            }

            SceneDirectory.Advance(BackgroundPointer, Pass.ElapsedMilliseconds,
                                   SceneApplied,
                                   TabPressed && !PointerInLayers && !PointerBehindDrawer,
                                   Viewport.Seam().Modifiers());
            TexturePaint.Advance(BackgroundPointer, Pass.ElapsedMilliseconds,
                               TexturePaintApplied, StackRows.Rows, StackRows.Count,
                               TabPressed && PointerInLayers && !PointerBehindDrawer,
                               Viewport.Seam().Modifiers());
            SketchDirectory.Advance(BackgroundPointer, Pass.ElapsedMilliseconds,
                                    SketchDirectoryApplied,
                                    TabPressed && !PointerBehindDrawer);
            ParametricTools.Advance(BackgroundPointer, Pass.ElapsedMilliseconds,
                                    ParametricToolsApplied,
                                    TabPressed && !PointerBehindDrawer);

            // 📝 The search field: while it holds the contact, the seam's typed run feeds the
            //    directory's retention run, and Backspace / Escape edit it. Gated on the panel's own
            //    `SearchTaken` — the validation shell captured every keystroke unconditionally,
            //    which is the "search box not working" a gate fixes.
            if (SceneApplied.SearchTaken)
            {
                static_cast<void>(Viewport.Seam().AcceptTyped(SceneApplied.EntityRetention,
                                                              SceneDirectoryContext::RetentionLimit));

                if (Viewport.Seam().KeyPressed(KeySubject::Retract))
                {
                    std::uint32_t Occupied = 0u;

                    while (Occupied + 1u < SceneDirectoryContext::RetentionLimit &&
                           SceneApplied.EntityRetention[Occupied] != '\0')
                    {
                        ++Occupied;
                    }

                    if (Occupied > 0u)
                        SceneApplied.EntityRetention[Occupied - 1u] = '\0';
                }

                if (Viewport.Seam().KeyPressed(KeySubject::Withdraw))
                    SceneApplied.EntityRetention[0] = '\0';
            }

            // 📝 The layer stack's own search pill — the same gated feed.
            if (TexturePaintApplied.SearchTaken)
            {
                static_cast<void>(Viewport.Seam().AcceptTyped(TexturePaintApplied.Retention,
                                                              TexturePaintContext::TextureRetentionLimit));

                if (Viewport.Seam().KeyPressed(KeySubject::Retract))
                {
                    std::uint32_t Occupied = 0u;

                    while (Occupied + 1u < TexturePaintContext::TextureRetentionLimit &&
                           TexturePaintApplied.Retention[Occupied] != '\0')
                    {
                        ++Occupied;
                    }

                    if (Occupied > 0u)
                        TexturePaintApplied.Retention[Occupied - 1u] = '\0';
                }

                if (Viewport.Seam().KeyPressed(KeySubject::Withdraw))
                    TexturePaintApplied.Retention[0] = '\0';
            }

            // The atmosphere is updated on the GPU in this frame's command stream. The scene component
            // classifies medium, sky-view and presentation changes; the current compatibility surface
            // composes those results directly without any CPU pixel generation or transfer submission.
            if (SkyRegistered && SceneApplied.EnvironmentPresented)
            {
                // A non-zero day-cycle rate drives the same authored azimuth the editor and game consume.
                // It therefore updates the visible disc and the directional light/shadow direction together.
                if (SceneApplied.Environment.DayCycleDegreesPerSecond != 0.0)
                {
                    SceneApplied.Environment.SunAzimuth = std::fmod(
                        SceneApplied.Environment.SunAzimuth +
                        SceneApplied.Environment.DayCycleDegreesPerSecond *
                        (Pass.ElapsedMilliseconds / 1000.0), 360.0);
                    if (SceneApplied.Environment.SunAzimuth < 0.0)
                        SceneApplied.Environment.SunAzimuth += 360.0;
                }

                AtmosphereState Authored;
                Authored.SunElevation = SceneApplied.Environment.SunElevation;
                Authored.SunAzimuth = SceneApplied.Environment.SunAzimuth;
                Authored.SunIlluminance = SceneApplied.Environment.SunIntensity;
                Authored.SunTemperature = SceneApplied.Environment.SunTemperature;
                Authored.SunAngularRadius = 0.266 * SceneApplied.Environment.SunDiscRadius;
                Authored.SunDiscIntensity = SceneApplied.Environment.SunDiscIntensity;
                Authored.SkyIntensity = SceneApplied.Environment.SkyIntensity;
                Authored.ExposureCompensation = SceneApplied.Environment.ExposureCompensation;
                Authored.GroundAlbedo = SceneApplied.Environment.GroundAlbedo;
                Authored.RayleighDensity = SceneApplied.Environment.AtmosphereDensity;
                Authored.RayleighScaleHeightKilometres = 8.0 * SceneApplied.Environment.AtmosphereScaleHeight;
                Authored.MieDensity = SceneApplied.Environment.MieDensity;
                Authored.MieScaleHeightKilometres = SceneApplied.Environment.MieScaleHeightKilometres;
                Authored.MieAsymmetry = SceneApplied.Environment.MieAsymmetry;
                Authored.OzoneDensity = SceneApplied.Environment.OzoneDensity;
                Authored.CameraAltitudeKilometres = std::max(EditorCamera.LaggedPosition[1], 0.0) * 0.001;

                const AtmosphereDirty Dirty = DynamicAtmosphere.Apply(Authored);

                SunLight.SetSolarPosition(Authored.SunAzimuth, Authored.SunElevation);
                SunLight.Illuminance = Authored.SunIlluminance;
                SunLight.TemperatureKelvin = Authored.SunTemperature;
                SunLight.AngularRadiusDegrees = Authored.SunAngularRadius;
                SunLight.ShadowStrength = SceneApplied.Environment.SunShadowStrength;
                SunLight.CastShadows = SceneApplied.Environment.SunShadowStrength > 0.0;

                DynamicSkyParameters GPU;
                GPU.SunElevationDegrees = static_cast<float>(Authored.SunElevation);
                GPU.SunAzimuthDegrees = static_cast<float>(Authored.SunAzimuth);
                GPU.SunIlluminance = static_cast<float>(Authored.SunIlluminance);
                GPU.SunTemperatureKelvin = static_cast<float>(Authored.SunTemperature);
                GPU.SunAngularRadiusDegrees = static_cast<float>(Authored.SunAngularRadius);
                GPU.SunDiscIntensity = static_cast<float>(Authored.SunDiscIntensity);
                GPU.SkyIntensity = static_cast<float>(Authored.SkyIntensity);
                GPU.ExposureCompensation = static_cast<float>(Authored.ExposureCompensation);
                GPU.GroundAlbedo = static_cast<float>(Authored.GroundAlbedo);
                GPU.RayleighDensity = static_cast<float>(Authored.RayleighDensity);
                GPU.RayleighScaleHeightKilometres = static_cast<float>(Authored.RayleighScaleHeightKilometres);
                GPU.MieDensity = static_cast<float>(Authored.MieDensity);
                GPU.MieScaleHeightKilometres = static_cast<float>(Authored.MieScaleHeightKilometres);
                GPU.MieAsymmetry = static_cast<float>(Authored.MieAsymmetry);
                GPU.OzoneDensity = static_cast<float>(Authored.OzoneDensity);
                GPU.CameraAltitudeKilometres = static_cast<float>(Authored.CameraAltitudeKilometres);
                GPU.Quality = std::min(SceneApplied.Environment.AtmosphereQuality, 3u);

                const bool Refresh = !SkyEverGenerated || Dirty != AtmosphereDirty::None ||
                                     SkyQuality != GPU.Quality;
                if (AtmosphereSurface.Record(Pass.Recording, GPU, Refresh).Resolved)
                {
                    SkyEverGenerated = true;
                    SkyQuality = GPU.Quality;
                }
                SceneApplied.SkyTextureIdentity = SkyTextureIdentity;
            }
            else
                SceneApplied.SkyTextureIdentity = 0u;

            // 📝 The layer stack's structural request is drained exactly once per tick, through the
            //    same shared helper the harness drives — the row set and the working copies stay in
            //    step with the panel's buttons and menus.
            StackRows.ApplyRequest(TexturePaintApplied);
            MaterialLayerStackBridgeReport MaterialBridge = RebuildMaterialLayersFromTextureStack(
                EditorMaterialDocument, EditorMaterialLayers, StackRows, TexturePaintApplied,
                EditorMaterialExchange, EditorMaterialSnapshotReady ? &EditorMaterialSnapshot : nullptr);
            EditorMaterialSnapshot = MaterialBridge.Snapshot;
            EditorMaterialSnapshotReady = true;

            // 📝 Bookmark recall is a request to the owning EditorCameraComponent, not a temporary write
            //    into the panel's mirrored pose (which the next camera tick would overwrite).
            if (SceneApplied.CameraBookmarkRecallRequested)
            {
                const std::uint32_t Bookmark = SceneApplied.CameraBookmarkTaken;
                if (Bookmark < SceneApplied.CameraBookmarkCount)
                {
                    for (std::uint32_t Axis = 0u; Axis < 3u; ++Axis)
                        EditorCamera.Position[Axis] = SceneApplied.CameraBookmarkPosition[Bookmark][Axis];
                    EditorCamera.YawDegrees = SceneApplied.CameraBookmarkRotation[Bookmark][0];
                    EditorCamera.PitchDegrees = SceneApplied.CameraBookmarkRotation[Bookmark][1];
                    EditorCamera.Snap();
                }
                SceneApplied.CameraBookmarkRecallRequested = false;
            }

            Viewport.RecordDrawers();
            Viewport.DrawerPanels();

            // ⑤ The south drawer's browser, recorded before the north drawer's Control Centre so the
            //     Control Centre's own exclusions are the last thing declared and the two cannot disagree.
            //     🔴 The interior is asked for every tick and not cached — the drawer is springing, so the
            //     extent it offers is a different one on almost every tick of an open or a close.
            const PlaneExtent BrowserInterior = Viewport.Drawers().Interior(DrawerBearing::South);

            BrowserInteraction.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);
            ContentBrowser.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);

            if (BrowserInterior.Width() > 0.0f && BrowserInterior.Height() > 0.0f)
            {
                Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Above));
                ThemeToken DrawerGround = Viewport.Appearance().Colour.SurfaceCurrent;
                DrawerGround.Opacity = 255u;
                Viewport.Surface().Ground(BrowserInterior, DrawerGround, 0.0f, CornerNone);
                ContentBrowser.RecordBrowser(BrowserInterior, ContentApplied, ContentBrowserApplied);
                ContentBrowser.RecordDeferred();

                const SharedCodexActivation ActivatedScene = ConsumeSharedCodexActivation(
                    ContentBrowserApplied, ContentApplied, EngineContentRoot);
                if (ActivatedScene.Requested && !ActivatedScene.Resolved)
                {
                    std::printf("%s — workspace activation refused (reason %u: %s)\n", HostName,
                                static_cast<unsigned>(ActivatedScene.Error.DeclaredReason), ActivatedScene.Error.Detail);
                }
                else if (ActivatedScene.Resolved)
                {
                    OpenedScene = ActivatedScene.Scene.Workspace;
                    OpenedSceneStanding = true;
                    BridgeSketchSceneDirectory(OpenedScene, WorkspaceSceneRows);
                    AppendSketchCadReferences(CadRuntime.Records, WorkspaceSceneRows);
                    PresentedEntities = WorkspaceSceneRows.Rows;
                    PresentedEntityCount = WorkspaceSceneRows.RowCount;
                    ApplySketchSceneEnvironment(OpenedScene, SceneApplied);
                    const SharedViewportCameraSeed CameraSeed = SharedViewportDefaultCamera();
                    EditorCamera.Position[0] = CameraSeed.Position[0];
                    EditorCamera.Position[1] = CameraSeed.Position[1];
                    EditorCamera.Position[2] = CameraSeed.Position[2];
                    EditorCamera.YawDegrees = CameraSeed.YawDegrees;
                    EditorCamera.PitchDegrees = CameraSeed.PitchDegrees;
                    EditorCamera.FieldOfViewDegrees = CameraSeed.FieldOfViewDegrees;
                    EditorCamera.Snap();

                    // The workspace names one shared pigment for every tea-service geometry entry.
                    // Present it once in the host-owned layer model rather than fabricating one layer per mesh.
                    StackRows.Seed(&WhiteDielectricLayer, 1u);
                    SeedPaintContextFromRows(TexturePaintApplied, StackRows.Rows, StackRows.Count);
                    TexturePaintApplied.LayerTaken = 0u;
                }
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

                // 🔴 Declared every tick or lost. Without it the drawer owns every contact inside its own
                //    body, so taking a record or dragging the lattice slides the drawer instead.
                ContentBrowser.Exclude(Viewport.Drawers(), DrawerBearing::South);
                Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Beneath));
            }

            const PlaneExtent ControlInterior = Viewport.Drawers().Interior(DrawerBearing::North);
            ControlCentre.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);
            // 📝 The artist's per-role weights are declared every tick so the workspace's panels read the
            //    current choice; the viewport re-states them after each resolve.
            Viewport.ApplyTypographyRoles(ControlCentreValues.TypographySize,
                                          ControlCentreValues.TypographyWeight);
            Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Above));
            if (ControlInterior.Width() > 0.0f && ControlInterior.Height() > 0.0f)
            {
                ThemeToken DrawerGround = Viewport.Appearance().Colour.SurfaceCurrent;
                DrawerGround.Opacity = 255u;
                Viewport.Surface().Ground(ControlInterior, DrawerGround, 0.0f, CornerNone);
            }
            Discard(ControlCentre.Record(ControlInterior, ControlCentreValues));

            // UI Scaling was previously only a displayed Control Centre value. It now re-resolves the shared
            // appearance, while display DPI remains an independent multiplier. Panels that cache derived
            // metrics are explicitly reseated; borrowed-theme panels observe the same profile immediately.
            if (Viewport.ApplyInterfaceScale(ControlCentreValues.Scaling))
            {
                Discard(Viewport.Seam().ApplyWorkspaceStyle(
                    Viewport.Appearance().WorkspaceMeasure,
                    Viewport.Appearance().Workspace));
                ContentBrowser.Reapply(Viewport.Appearance());
                SceneDirectory.Reapply(Viewport.Appearance());
                SketchDirectory.Reapply(Viewport.Appearance());
                ParametricTools.Reapply(Viewport.Appearance());
                TexturePaint.Reapply(Viewport.Appearance());
            }
            Discard(Viewport.Seam().ApplyInterfaceAntialiasing(
                ControlCentreValues.GeometryAntialiasing));

            // 📝 Compared rather than watched. The Control Centre writes the artist's choice straight into the
            //    ordinates, so the change is visible here as a difference and needs no callback to report it.
            {
                ThemeSelection Chosen;
                Chosen.Current   = ControlCentreValues.Theme;
                Chosen.Primary     = ControlCentreValues.Primary;
                Chosen.Secondary   = ControlCentreValues.Secondary;
                Chosen.Information = ControlCentreValues.Information;
                Chosen.Warning     = ControlCentreValues.Warning;
                Chosen.Alert       = ControlCentreValues.Alert;
                if (ControlCentreValues.Font < Fonts.FamilyCount() && Fonts.FamilyName(ControlCentreValues.Font) != nullptr)
                    std::snprintf(Chosen.FontFamily, sizeof(Chosen.FontFamily), "%s", Fonts.FamilyName(ControlCentreValues.Font));

                // 🔴 Only the family re-runs the font pipeline. The other members are colours and reach
                //    every panel through the appearance; re-loading fonts for them would re-rasterise
                //    the whole atlas on every theme or colour edit.
                const bool FamilyAltered = std::strcmp(Chosen.FontFamily, InscribedSelection.FontFamily) != 0;
                const bool Altered = Chosen.Current   != InscribedSelection.Current
                                  || Chosen.Primary     != InscribedSelection.Primary
                                  || Chosen.Secondary   != InscribedSelection.Secondary
                                  || Chosen.Information != InscribedSelection.Information
                                  || Chosen.Warning     != InscribedSelection.Warning
                                  || Chosen.Alert       != InscribedSelection.Alert
                                  || std::strcmp(Chosen.FontFamily, InscribedSelection.FontFamily) != 0;

                // 🔴 The record is advanced whether the write was delivered or rejected. A read-only folder would
                //    otherwise have every later tick retry the same rejected write for the life of the process.
                if (Altered)
                {
                    Discard(ThemeInterchange::RecordBeside(InvokedAs, Chosen));
                    InscribedSelection = Chosen;

                    // 🔴 Declared to the viewport, which re-anchors the whole appearance on the next tick, and
                    //    then pushed into the two panels that keep their own copy of the inks. The shell reads
                    //    the appearance through its own Reapply, which the viewport already calls.
                    Viewport.Retint(Chosen);
                    Discard(Viewport.Seam().ApplyWorkspaceStyle(
                        Viewport.Appearance().WorkspaceMeasure,
                        Viewport.Appearance().Workspace));
                    ContentBrowser.Reapply(Viewport.Appearance());
                    SceneDirectory.Reapply(Viewport.Appearance());
                    SketchDirectory.Reapply(Viewport.Appearance());
                    ParametricTools.Reapply(Viewport.Appearance());
                    TexturePaint.Reapply(Viewport.Appearance());
                    if (FamilyAltered)
                        Fonts.RequestLoad(FontRoot.c_str(), Viewport.Appearance().Fonts, 1.0f);
                }
            }
            ControlCentre.Exclude(Viewport.Drawers());
            Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Beneath));

            // 🧩 Admission is deliberately drained while the host command recording is open and before the
            // interface begins the display scope. The imported packet has already travelled through source drain,
            // faithful decode, document intake, Earcut rendering-packet construction, and partition registration.
            if (GeometryAdmissionPending && GeometryDevice.Standing())
            {
                const Outcome<const PartitionStructure*> Partitioned =
                    ImportedVisibility.Registered(PendingVisibilityRegistration);
                const Outcome<const GeometryRenderingSnapshot*> Rendering =
                    ImportedRendering.Resolve(PendingRendering);
                if (Partitioned.Resolved && Rendering.Resolved)
                {
                    const Outcome<std::uint32_t> Admitted = GeometryDevice.Admit(
                        *Partitioned.Resolve(), *Rendering.Resolve(), PendingRegistrationBase, Pass.Recording);
                    if (Admitted.Resolved)
                        GeometryAdmissionPending = false;
                    else
                        std::printf("%s — geometry admission refused (reason %u: %s)\n", HostName,
                                    static_cast<unsigned>(Admitted.Error.DeclaredReason), Admitted.Error.Detail);
                }
            }

            if (Viewport.SealPanels().Resolved)
            {
                // Scene compute and classic render constructs record before this boundary. The interface and
                // display-referred overlay require the dynamic display scope and therefore begin it here.
                Discard(Lifetime.BeginDisplay());

                // 🔴 Read. A rejected Record presents the cleared ground with nothing on it, which is
                //    indistinguishable from a panel that drew nothing, so the refusal is named here.
                if (!Viewport.RecordBeneath(Pass.Recording))
                {
                    std::printf("%s — the interface content was not recorded\n", HostName);
                }

                // 🔴 Overlay always records. Drawers may scissor the leaf they cover;
                //    menus sit on the foreground list submitted after this pass.

                for (std::uint32_t ViewportIndex = 0u;
                     ViewportIndex < ViewportLeafTally;
                     ++ViewportIndex)
                {
                    const std::uint32_t LeafIndex = ViewportLeafIndexs[ViewportIndex];
                    OverlayGeometry& LeafOverlay = ViewportOverlays[LeafIndex];

                    if (LeafOverlay.Generation != OverlayGeneration[LeafIndex])
                    {
                        Overlay.Upload(LeafOverlay);
                        OverlayGeneration[LeafIndex] = LeafOverlay.Generation;
                    }

                    PlaneExtent LeafRect = ViewportLeafRects[ViewportIndex];

                    if (NorthInterior.Height() > 0.0f &&
                        NorthInterior.MaximumY > LeafRect.MinimumY &&
                        NorthInterior.MinimumY <= LeafRect.MinimumY)
                    {
                        LeafRect.MinimumY = std::min(LeafRect.MaximumY, NorthInterior.MaximumY);
                    }

                    if (SouthInterior.Height() > 0.0f &&
                        SouthInterior.MinimumY < LeafRect.MaximumY &&
                        SouthInterior.MaximumY >= LeafRect.MaximumY)
                    {
                        LeafRect.MaximumY = std::max(LeafRect.MinimumY, SouthInterior.MinimumY);
                    }

                    if (LeafRect.Width() <= 0.0f || LeafRect.Height() <= 0.0f)
                        continue;

                    Overlay.Record(Pass.Recording, Pass.Width, Pass.Height,
                                   LeafRect.MinimumX, LeafRect.MinimumY,
                                   LeafRect.MaximumX, LeafRect.MaximumY,
                                   LeafRect.MinimumX, LeafRect.MinimumY,
                                   LeafRect.MaximumX, LeafRect.MaximumY);
                }

                if (!Viewport.RecordAbove(Pass.Recording))
                {
                    std::printf("%s — the interface chrome was not recorded\n", HostName);
                }
            }
            else
            {
                Discard(Viewport.Abandon());
            }
        }
        else
        {
            Discard(Viewport.Abandon());
        }

        // ⑤ Close the scope, submit, present, advance. A rejected present re-establishes the chain rather
        //    than ending the loop.
        if (!Lifetime.Complete().Resolved)
            break;
    }

    // ─────────────────────────────────────────────────────────────────────────────────────────────────────
    //                                                      RECLAMATION
    // ─────────────────────────────────────────────────────────────────────────────────────────────────────

    // 📝 The viewport is retired before the lifetimes it was constructed over. HostLifecycle idles the
    //    device inside Reclaim, so nothing here needs to.
    // 🔴 Read before Reclaim. The register is Device lifetime, and a reclaimed device has emptied it.
    const std::uint32_t Serious = Lifetime.StateDiagnostics();

    ControlCentre.Reset();
    WorkspacePanels.Reset();
    for (std::uint32_t Index = 0u; Index < WorkspaceIndex::WorkspaceLimit; ++Index)
        PanelPartitions[Index].Reset();
    Workspace.Reset();
    Workspaces.Reset();
    Viewport.Reclaim();

    // 🔴 The atmosphere presentation is reclaimed BEFORE the device: its fence wait needs the device alive,
    //    and a surface left standing past `Lifetime.Reclaim()` waited on a dead device in its destructor —
    //    the "vkWaitForFences: Invalid device" reported at shutdown.
    GeometryDevice.Reclaim();
    AtmosphereSurface.Reclaim();
    SkyRegistered = false;
    SkyTextureIdentity = 0u;
    Overlay.Reclaim();
    OverlayCodec.Reclaim();

    Lifetime.Reclaim();

    std::printf("%s \u2014 exited cleanly\n", HostName);

    // 🔴 Returned rather than only stated. A validation run needs an exit code, so that a serious arrival
    //    fails whatever invoked the host instead of scrolling past in a console nobody reads.
    return (Serious == 0u) ? 0 : 1;
}
