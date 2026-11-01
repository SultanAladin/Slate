// Native reference-project, options, image decoding and per-pane facing regression.
#include "UI/SolidArc/Document/SolidArcDocumentCodec.h"
#include "UI/SolidArc/Interface/SolidArcInterface.h"
#include "UI/SolidArc/Interface/SolidArcOptions.h"
#include "UI/SolidArc/Viewport/SolidArcRasterization.h"
#include <imgui.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numbers>
#include <stdexcept>
using namespace Frontier;
using namespace Frontier::SolidArc;
int Checks = 0;
void Check(bool Condition, const std::string& Label)
{
    if (!Condition) throw std::runtime_error(Label);
    ++Checks; std::cout << "PASS " << Label << '\n';
}
int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    try
    {
        const std::filesystem::path Root = std::filesystem::absolute(argv[1]);
        const auto File = LoadSolidArcDocumentFile((Root/"Liger.solidarc").string());
        Check(File.Refusal.empty(), "native project decoder");
        Check(File.Tree.Steps.empty(), "no existing modelling steps");
        Check(File.ImagePlanes.size() == 6, "six reference planes");
        ImGui::CreateContext();
        SolidArcOptions Options = LoadSolidArcOptions((Root/"SolidArcWorkspace.toml").string());
        Check(Options.Refusal.empty(), "startup options decoder");
        Check(Options.InitialLayout == "Quad" && Options.FitImagesEnabled, "quad layout and reference framing requested");
        SolidArcWorkspaceState State;
        InitialiseSolidArcWorkspace(State, Options);
        State.Document.ImagePlanes = File.ImagePlanes;
        ConfigureSolidArcStartupViews(State);
        Check(SolidArcViewportPaneCount(State.SplitView.Layout) == 4, "four real viewport cameras initialised");
        Check(State.Camera.Distance > 1 && std::isfinite(State.Camera.Distance), "reference bounds produce finite model-scale camera");
        const auto Round = DecodeSolidArcDocument(EncodeSolidArcDocument(State.Document));
        Check(Round.Refusal.empty() && Round.ImagePlanes.size() == 6, "native project codec round trip");
        for (size_t i = 0; i < File.ImagePlanes.size(); ++i)
        {
            const auto& P = File.ImagePlanes[i];
            Check(P.FrontOnlyCondition && P.LockCondition && P.VisibleCondition, P.Name+" registered, locked and one-sided");
            Check(Round.ImagePlanes[i].FrontOnlyCondition && Round.ImagePlanes[i].LockCondition, P.Name+" flags survive save");
            const auto Image = DecodeSolidArcPicture(P.Path);
            Check(Image.Refusal.empty() && Image.Width >= 3800 && Image.Height >= 2100, P.Name+" native PNG decode");
        }
        const OrbitViewPreset Presets[] = {OrbitViewPreset::Top,OrbitViewPreset::Bottom,OrbitViewPreset::Back,OrbitViewPreset::Front,OrbitViewPreset::Right,OrbitViewPreset::Left};
        for (int v = 0; v < 6; ++v)
        {
            OrbitCamera Camera;
            PresentOrbitView(Camera, Presets[v]);
            const auto Frame = FrameOrbitCamera(Camera, 1200, 900);
            int Drawn = 0;
            for (size_t i = 0; i < File.ImagePlanes.size(); ++i)
            {
                const auto& P = File.ImagePlanes[i];
                SolidArcImagePlaneDraw D;
                D.Right = {P.Right.X,P.Right.Y,P.Right.Z}; D.Down = {P.Down.X,P.Down.Y,P.Down.Z}; D.FrontOnlyCondition = true;
                const bool Face = FacesSolidArcImagePlane(D, Frame); Drawn += Face;
                Check(Face == (i == size_t(v)), "view "+std::to_string(v)+" facing filter plane "+std::to_string(i));
                D.FrontOnlyCondition = false;
                Check(FacesSolidArcImagePlane(D, Frame), "legacy two-sided image behaviour");
            }
            Check(Drawn == 1, "orthographic view draws only its own reference, never its opposite");
        }
        for (const std::string Name : {"Top","Bottom","Left","Right","Front","Rear"})
        {
            const auto ViewOptions = LoadSolidArcOptions((Root/"Views"/(Name+".toml")).string());
            Check(ViewOptions.Refusal.empty() && ViewOptions.InitialLayout == "Single", Name+" launcher options");
        }
        const auto Temp = std::filesystem::temp_directory_path()/"Liger-invalid-startup-options.toml";
        {std::ofstream O(Temp); O << "[Viewport]\nInitialView = 'InvalidView'\n";}
        Check(!LoadSolidArcOptions(Temp.string()).Refusal.empty(), "invalid view is refused, not silently mislabelled");
        std::filesystem::remove(Temp);
        ImGui::DestroyContext();
        std::cout << "Native blueprint checks: " << Checks << " passed; 0 failed\n";
        return 0;
    }
    catch(const std::exception& E) { std::cerr << "FAIL " << E.what() << '\n'; return 1; }
}
