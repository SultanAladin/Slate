#include "SlateWorkspace/Discipline/WorldDraftSketchBridge/Api/WorldDraftSketchBridge.h"

#include <cstdio>
#include <cmath>

using namespace Slate;

namespace
{
std::uint32_t Claims = 0u;
std::uint32_t Failures = 0u;

void Claim(bool Held, const char* Sentence)
{
    ++Claims;
    if (!Held)
    {
        ++Failures;
        std::printf("  FAILED  %s\n", Sentence);
    }
}

struct Bench
{
    WorkspaceNameIndex Naming = {};
    SketchStructure Sketch = {};
    WorkspaceRecordStructure Records = {};
    WorkspaceRevisionSequence Revisions = {};

    Bench()
    {
        Sketch.DeclarePlane({ { 0.0, 40.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 1.0, 0.0, 0.0 } });

        WorkspaceRecord SketchFolder = {};
        SketchFolder.Subject = WorkspaceRecordSubject::Folder;
        SketchFolder.FolderCategory = WorkspaceCategory::Sketch;
        SketchFolder.Naming = "Sketch";
        Records.Declare(SketchFolder);

        WorkspaceRecord AnnotationFolder = {};
        AnnotationFolder.Subject = WorkspaceRecordSubject::Folder;
        AnnotationFolder.FolderCategory = WorkspaceCategory::Annotation;
        AnnotationFolder.Naming = "Annotation";
        Records.Declare(AnnotationFolder);
    }
};

void ProveRectangleCommitsAsWorldBackedProfile()
{
    std::printf("\n1. A closed draw placement commits through world geometry and comes back as a profile\n");

    Bench Stage;
    SealedPlacement Rectangle = {};
    Rectangle.Subject = SketchSubject::Rectangle;
    Rectangle.Method = PlacementMethod::Extent;
    Rectangle.Anchors = { { 0.0, 40.0, 0.0 }, { 100.0, 40.0, 80.0 } };
    Rectangle.ClosedProfile = true;

    WorkspaceRecordName Selected = {};
    Claim(CommitPlacementWorldBacked(Stage.Naming, Stage.Sketch, Stage.Records, Stage.Revisions,
                                     Rectangle, Selected),
          "the rectangle placement commits successfully through the world-backed path");
    const WorkspaceRecord* Record = Stage.Records.Resolve(Selected);
    Claim(Record != nullptr && Record->Subject == WorkspaceRecordSubject::ClosedProfile,
          "and it selects a closed-profile record rather than four loose edge rows");
    Claim(Stage.Sketch.Curves().size() == 4u && Stage.Sketch.Profiles().size() == 1u,
          "with four mirrored world curves and one sketch profile declared");
    Claim(Stage.Revisions.DeclaredCount() == 1u,
          "and the whole placement seals exactly one revision");
}

void ProveConstructionPolylineStaysWire()
{
    std::printf("\n2. Construction geometry stays wire-only when committed through the world path\n");

    Bench Stage;
    SealedPlacement Polyline = {};
    Polyline.Subject = SketchSubject::Polyline;
    Polyline.Method = PlacementMethod::Extent;
    Polyline.Construction = true;
    Polyline.Anchors = { { 0.0, 40.0, 0.0 }, { 50.0, 40.0, 0.0 }, { 50.0, 40.0, 40.0 } };

    WorkspaceRecordName Selected = {};
    Claim(CommitPlacementWorldBacked(Stage.Naming, Stage.Sketch, Stage.Records, Stage.Revisions,
                                     Polyline, Selected),
          "the construction polyline commits successfully");
    Claim(Stage.Sketch.Curves().size() == 2u && Stage.Sketch.Profiles().empty(),
          "and it appends only its wire spans, with no closed profile created");
    const WorkspaceRecord* First = Stage.Records.Resolve(Selected);
    Claim(First != nullptr && First->Subject == WorkspaceRecordSubject::OpenCurve && First->ConstructionSemantic,
          "the returned selection is the first construction curve record");
    Claim(Stage.Revisions.DeclaredCount() == 1u,
          "and the multi-span construction draw is still one undo step");
}

void ProvePointAndWorldBackedRendering()
{
    std::printf("\n3. Point placements still land, and off-plane edits render through the world projection\n");

    Bench Stage;
    SealedPlacement Point = {};
    Point.Subject = SketchSubject::Point;
    Point.Method = PlacementMethod::Extent;
    Point.Anchors = { { 10.0, 40.0, 15.0 } };

    WorkspaceRecordName Selected = {};
    Claim(CommitPlacementWorldBacked(Stage.Naming, Stage.Sketch, Stage.Records, Stage.Revisions,
                                     Point, Selected),
          "a point placement also commits through the world-backed path");
    const WorkspaceRecord* Record = Stage.Records.Resolve(Selected);
    Claim(Record != nullptr && Record->Subject == WorkspaceRecordSubject::Point,
          "and it returns a point record for the placed point");

    Stage.Sketch.Curves()[0u].Geometry.HeldLine().Origin.Forward = 100.0;
    Stage.Sketch.Curves()[0u].Geometry.HeldLine().Terminus.Forward = 100.001;

    WorkspaceCadPacket Packet = {};
    const ResolvedCamera Camera = ResolveFreeCamera({ 0.0, 50.0, -300.0 }, 0.0, 0.0, 60.0, true, 1.0);
    Discard(ProjectWorldBackedSketchRendering(Stage.Sketch, Camera,
                                              { 0.0f, 0.0f, 800.0f, 600.0f }, { 1.0 }, Packet));
    Claim(Packet.SegmentCount > 0u,
          "the committed point's supporting geometry still renders through the world-backed projection");
}

} // namespace

int main()
{
    std::printf("=========================================================================\n");
    std::printf("WORLD DRAFT PLACEMENT COMMIT PROOF\n");
    std::printf("=========================================================================\n");

    ProveRectangleCommitsAsWorldBackedProfile();
    ProveConstructionPolylineStaysWire();
    ProvePointAndWorldBackedRendering();

    std::printf("\n=========================================================================\n");
    std::printf("%u claims, %u failures -> %s\n", Claims, Failures,
                Failures == 0u ? "PROVEN" : "REFUTED");
    std::printf("=========================================================================\n");
    return Failures == 0u ? 0 : 1;
}
