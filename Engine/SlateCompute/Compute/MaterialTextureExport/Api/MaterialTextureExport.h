//============================================================================================================================================
//                                                       MATERIALTEXTUREEXPORT.H
//============================================================================================================================================
// 🧩 Actual flattened material texture writing from export packages. This is the pixel-producing seam after
//    MaterialExport declares target files and channel packing.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "SlateCompute/Compute/MaterialImageSampling/Api/MaterialImageSampling.h"
#include "SlateDocument/Format/MaterialExport/Api/MaterialExport.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Slate
{

struct FlattenedMaterialTexture
{
    std::string Path = {};
    MaterialExportImageDeclaration Declaration = {};
    std::uint32_t Width = 0u;
    std::uint32_t Height = 0u;
    std::vector<float> Texels = {}; // [-] RGBA float, row-major
};

struct MaterialTextureExportReport
{
    std::vector<std::string> WrittenFiles = {};
    std::string ManifestPath = {};
    std::uint32_t PixelCount = 0u;
    std::uint32_t ImageCount = 0u;
};

class MaterialTextureExport
{
public:
    Outcome<FlattenedMaterialTexture> FlattenImage(const WorkspaceMaterialRecord& Material,
                                                   const MaterialExportPackage& Package,
                                                   const MaterialExportImageDeclaration& Image) const;

    Outcome<bool> WriteImage(const FlattenedMaterialTexture& Texture) const;

    Outcome<MaterialTextureExportReport> WritePackage(const WorkspaceMaterialRecord& Material,
                                                      const MaterialExportPackage& Package) const;
};

} // namespace Slate
