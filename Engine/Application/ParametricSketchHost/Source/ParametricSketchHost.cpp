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
#include "SlateFeature/Sketch/SketchRenderingProjection/Api/SketchRenderingProjection.h"
#include "SlateFeature/Sketch/SketchStructure/Api/SketchStructure.h"
#include "SlateUI/Interface/ContentBrowserPanel/Api/ContentBrowserPanel.h"
#include "SlateUI/Interface/ControlCentrePanel/Api/ControlCentrePanel.h"
#include "SlateUI/Interface/EditorPanel/Api/EditorPanel.h"
#include "SlateUI/Interface/ParametricWorkspace/Api/ParametricWorkspacePanel.h"
#include "SlateUI/Interface/ThemeInterchange/Api/ThemeInterchange.h"
#include "SlateUI/Interface/ViewportSequence/Api/ViewportSequence.h"
#include "SlateUI/Interface/WorkspacePanel/Api/WorkspaceIndex.h"
#include "SlateVulkan/Device/HostLifecycle/Api/HostLifecycle.h"
#include "SlateVulkan/Device/ShaderCodec/Api/ShaderCodec.h"
#include "SlateVulkan/Device/WorkspaceCadPass/Api/WorkspaceCadPass.h"

#include <algorithm>
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
    Discard(Partition.Assign(Root.Resolve().Maximum, PanelSubject::Viewport));
    Discard(Partition.Proportion(PanelStructure::RootIndex, 0.30f));
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
                                                bool& Seeded)
{
    ProjectWorkspaceDirectory(Records, Directory);

    const Outcome<bool> DirectoryBridge = BridgeParametricDirectory(Directory, Bridge);
    if (!DirectoryBridge.Resolved)
        return DirectoryBridge;

    SeatParametricContext(Directory, Applied, Seeded);

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

void RecordViewportPlaceholder(RecordingSurface& Surface, const PlaneExtent& Extent,
                               const WorkspaceCadPacket& Packet)
{
    const char* Title = "2D CAD pass shell standing";
    char Detail[192] = {};
    std::snprintf(Detail, sizeof(Detail),
                  "%u segments  •  %u fills  •  %u markers  •  GPU CAD pass pending",
                  static_cast<unsigned>(Packet.SegmentCount),
                  static_cast<unsigned>(Packet.FillCount),
                  static_cast<unsigned>(Packet.MarkerCount));
    const float TitleRun = 18.0f;
    const float DetailRun = 12.0f;
    const float TitleWidth = Surface.MeasureRun(Title, TitleRun, 0.0f);
    const float DetailWidth = Surface.MeasureRun(Detail, DetailRun, 0.0f);

    Surface.TextRun(Extent.MinimumX + (Extent.Width() - TitleWidth) * 0.5f,
                    Extent.MinimumY + Extent.Height() * 0.5f - 18.0f,
                    Covering(0xE5E7EBu), Title, TitleRun, 0.0f, true);
    Surface.TextRun(Extent.MinimumX + (Extent.Width() - DetailWidth) * 0.5f,
                    Extent.MinimumY + Extent.Height() * 0.5f + 6.0f,
                    Covering(0xA1A1AAu), Detail, DetailRun);
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
    ControlCentrePanel ControlCentre;
    ControlCentreConfiguration ControlCentreValues;
    FontLoader Fonts;
    ControlIndex BrowserInteraction;
    ContentBrowserPanel ContentBrowser;
    ContentBrowserConfiguration ContentBrowserApplied;
    ContentLibrary ContentApplied;
    ControlIndex ParametricInteraction;
    ParametricWorkspacePanel ParametricPanel;
    ShaderCodec CadCodec;
    WorkspaceCadPass CadPass;

    WorkspaceNameIndex Naming;
    SketchStructure Sketch;
    WorkspaceRecordStructure Records;
    WorkspaceRevisionSequence Revisions;
    WorkspaceDirectoryProjection Directory;
    ParametricWorkspaceBridgeStorage Bridge;
    static WorkspaceCadPacket CadPacket;
    ParametricWorkspaceContext ParametricApplied = {};
    bool ParametricSeeded = false;
    bool ProjectionWarned = false;
    bool CadPacketWarned = false;
    bool CadPassWarned = false;
    std::uint32_t UploadedCadGeneration = 0xFFFFFFFFu;
    std::uint32_t RegisterIntoNode = 0u;

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

        if (Pass.Current == TickCondition::Closed)
            break;

        if (Pass.DeviceRetiring)
        {
            Viewport.Reclaim();
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
            BackgroundPointer.PositionX = BackgroundPointer.PositionY = -1000000.0f;
            BackgroundPointer.TravelX = BackgroundPointer.TravelY = BackgroundPointer.WheelY = 0.0f;
            BackgroundPointer.ContactHeld = BackgroundPointer.ContactPressed = false;
            BackgroundPointer.ContactDoublePressed = BackgroundPointer.ContactReleased = false;
            BackgroundPointer.SecondaryHeld = BackgroundPointer.SecondaryPressed = false;
            BackgroundPointer.SecondaryReleased = false;
        }

        Discard(Workspace.Record(Whole, Workspaces.ActiveTitle()));
        Viewport.Seam().RecordDockSpace(Whole);

        const std::uint32_t OpenCount = Workspaces.OpenCount();
        std::uint32_t Withdrawing = OpenCount;
        const std::uint32_t ApplyInto = RegisterIntoNode;
        RegisterIntoNode = 0u;

        PlaneExtent ViewportLeafRects[PanelStructure::RecordLimit] = {};
        std::uint32_t ViewportLeafTally = 0u;

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

                        case PanelSubject::Viewport:
                            if (ViewportLeafTally < PanelStructure::RecordLimit)
                                ViewportLeafRects[ViewportLeafTally++] = LeafBody;
                            if (!CadPass.Standing())
                            {
                                RecordCadFallback(Viewport.Surface(), LeafBody, CadPacket);
                                RecordViewportPlaceholder(Viewport.Surface(), LeafBody, CadPacket);
                            }
                            break;

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
                if (FamilyAltered)
                    Fonts.RequestLoad(FontRoot.c_str(), Viewport.Appearance().Fonts, 1.0f);
            }
        }
        ControlCentre.Exclude(Viewport.Drawers());
        Discard(Viewport.Surface().SwitchLayer(RecordingSurface::ShellLayer::Beneath));

        ParametricInteraction.Advance(Viewport.Surface().Pointer(), Pass.ElapsedMilliseconds);
        ParametricPanel.Advance(BackgroundPointer, Pass.ElapsedMilliseconds,
                                ParametricApplied,
                                Viewport.Seam().KeyPressed(KeySubject::Summon) && !PointerBehindDrawer,
                                Viewport.Seam().Modifiers());

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
                    CadPass.Record(Pass.Recording, Pass.Width, Pass.Height,
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
    WorkspacePanels.Reset();
    for (std::uint32_t Index = 0u; Index < WorkspaceIndex::WorkspaceLimit; ++Index)
        PanelPartitions[Index].Reset();
    Workspace.Reset();
    Workspaces.Reset();
    CadPass.Reclaim();
    CadCodec.Reclaim();
    Viewport.Reclaim();
    Lifetime.Reclaim();

    std::printf("%s — exited cleanly\n", HostName);
    return (Serious == 0u) ? 0 : 1;
}
