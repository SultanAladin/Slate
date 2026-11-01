#include "Engine/GeometryProcessing/Audit/ModelAudit.h"
#include "Engine/GeometryProcessing/Exchange/StepExchange.h"
#include "UI/SolidArc/Document/SolidArcDocument.h"
#include "UI/SolidArc/Document/SolidArcDocumentCodec.h"
#include "UI/SolidArc/Document/SolidArcScene.h"
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using namespace Frontier;
using namespace Frontier::GeometryProcessing;
using namespace Frontier::SolidArc;

int main(int argc, char** argv)
{
    if (argc < 2) { std::cerr << "Usage: CadCertifier <file.step | file.solidarc> [--dump-scene prefix]\n"; return 1; }
    std::string path = argv[1];
    if (path.size() >= 9 && path.substr(path.size() - 9) == ".solidarc")
    {
        SolidArcDocumentFile file = LoadSolidArcDocumentFile(path);
        if (!file.Refusal.empty()) { std::cerr << "DecodeRefusal: " << file.Refusal << "\n"; return 2; }
        SolidArcDocument doc;
        doc.Title        = file.Title;
        doc.Tree         = std::move(file.Tree);
        doc.Captions     = std::move(file.Captions);
        doc.ImagePlanes  = std::move(file.ImagePlanes);
        doc.Appearance   = std::move(file.Appearance);
        doc.DrawingSheet = std::move(file.DrawingSheet);
        doc.Dimensions   = std::move(file.Dimensions);
        RegenerateSolidArcDocument(doc);
        int refused = doc.Regeneration.Refused;
        for (const auto& r : doc.Regeneration.Steps)
        {
            if (r.Status == FeatureStepStatus::Refused)
                std::cerr << "StepRefused [" << r.Label << "]: " << r.Refusal << "\n";
        }
        std::cout << "SOLIDARC_OK bodies=" << doc.Regeneration.Bodies.size() << " refused=" << refused << "\n";
        int auditIssues = 0;
        for (const auto& b : doc.Regeneration.Bodies)
        {
            if (!b.Body.PresentCondition()) continue;
            auto inv = doc.Partition->CollectInventory(b.Body);
            auto audit = AuditBody(*doc.Partition, b.Body);
            auditIssues += (int)audit.Findings.size();
            std::map<uint32_t, int> edgeUses;
            for (FaceToken f : inv.Faces)
                for (LoopToken l : doc.Partition->Faces.Resolve(f).Loops)
                    doc.Partition->EnumerateLoopFins(l, [&](FinToken fin) { edgeUses[doc.Partition->Fins.Resolve(fin).Edge.Slot]++; });
            int laminar = 0, sewn = 0, nonManifold = 0;
            for (EdgeToken e : inv.Edges)
            {
                int u = edgeUses[e.Slot];
                if (u == 1) ++laminar;
                else if (u == 2) ++sewn;
                else ++nonManifold;
            }
            std::cout << "  Body[" << b.Key << "] caption='" << (doc.Captions.count(b.Key) ? doc.Captions[b.Key] : b.Key) << "'"
                      << " cat=" << (doc.Partition->Bodies.Resolve(b.Body).Category == BodyCategory::Sheet ? "Sheet" : "Solid")
                      << " faces=" << inv.Faces.size()
                      << " edges=" << inv.Edges.size() << " (sewn=" << sewn << ", laminar=" << laminar << ", nonManifold=" << nonManifold << ")"
                      << " verts=" << inv.Vertices.size()
                      << " sound=" << (audit.SoundCondition() ? "YES" : "NO") << "\n";
            for (const auto& f : audit.Findings)
                std::cout << "    AuditFinding " << f.Code << " " << f.Subject << ": " << f.Detail << "\n";
        }
        if (argc < 4) return (refused == 0 && auditIssues == 0) ? 0 : 3;
        SolidArcScene scene = BuildSolidArcScene(doc, 0.0004, true);
        if (!scene.Refusal.empty()) { std::cerr << "SceneRefusal: " << scene.Refusal << "\n"; return 4; }
        std::cout << "  SolidArcScene triangles=" << scene.Corners.size() / 3
                  << " faces=" << scene.Faces.size()
                  << " edges=" << scene.Edges.size()
                  << " segments=" << scene.Segments.size()
                  << " ms=" << scene.Milliseconds << "\n";
        if (argc >= 4 && std::string(argv[2]) == "--dump-scene")
        {
            std::string prefix = argv[3];
            std::ofstream fbin(prefix + "_scene.bin", std::ios::binary);
            uint32_t nc = (uint32_t)scene.Corners.size();
            uint32_t ns = (uint32_t)scene.Segments.size();
            uint32_t nf = (uint32_t)scene.Faces.size();
            uint32_t ne = (uint32_t)scene.Edges.size();
            fbin.write((const char*)&nc, 4);
            fbin.write((const char*)&ns, 4);
            fbin.write((const char*)&nf, 4);
            fbin.write((const char*)&ne, 4);
            fbin.write((const char*)scene.Corners.data(), nc * sizeof(SolidArcSceneCorner));
            fbin.write((const char*)scene.Segments.data(), ns * sizeof(SolidArcSceneSegment));
            for (const auto& f : scene.Faces) { uint32_t b = f.Body; fbin.write((const char*)&b, 4); }
            for (const auto& e : scene.Edges) { uint32_t b = e.Body; fbin.write((const char*)&b, 4); }
        }
        return (refused == 0 && auditIssues == 0) ? 0 : 3;
    }
    else
    {
        ModelPartition part;
        StepImportRequest req;
        req.ModelUnitMillimetres = 1000.0;
        req.Label = "CertifySTEP";
        StepImport imp = ImportStepFile(part, path, req);
        if (!imp.Refusal.empty()) { std::cerr << "StepRefusal: " << imp.Refusal << "\n"; return 2; }
        std::cout << "STEP_OK solids=" << imp.Solids << " sheets=" << imp.Sheets
                  << " faces=" << imp.Faces << " edges=" << imp.Edges << " verts=" << imp.Vertices
                  << " tolerantEdges=" << imp.TolerantEdges << " tolerantVerts=" << imp.TolerantVertices
                  << " reorientedFaces=" << imp.ReorientedFaces << " largestGap_mm=" << (imp.LargestGap * 1000.0) << "\n";
        for (const auto& n : imp.Notes) std::cout << "  Note: " << n << "\n";
        return 0;
    }
}
