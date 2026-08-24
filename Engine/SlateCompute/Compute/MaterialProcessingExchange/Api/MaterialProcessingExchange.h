//============================================================================================================================================
//                                                  MATERIALPROCESSINGEXCHANGE.H
//============================================================================================================================================
// 🧩 Material-layer commands into validated material declarations; GPU processing follows behind this seam.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "Foundation/Identity.h"
#include "SlateCompute/Compute/PreviewProjection/Api/PreviewProjection.h"
#include "SlateDocument/Document/MaterialSpecification/Api/MaterialSpecification.h"
#include "SlateDocument/Document/MaterialSpecification/Api/PhysicalSurfaceSpecification.h"
#include "SlateDocument/Document/SurfaceLayerSequence/Api/SurfaceLayerSequence.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Slate
{

/// 🧩 Deterministic content fingerprints used to avoid reprocessing unchanged material and layer declarations.
struct MaterialProcessingDirtyKey
{
    static constexpr std::size_t ChannelSpan = static_cast<std::size_t>(ChannelSubject::ChannelCount);

    std::array<std::uint64_t, ChannelSpan> Channels = {};
    std::uint64_t Reflectance = 0u;
    std::uint64_t PhysicalSurface = 0u;
    std::uint64_t Layers      = 0u;
    std::uint64_t Combined    = 0u;
};

/// 🧩 Which portions changed between two immutable processing snapshots.
struct MaterialProcessingDirtySet
{
    std::uint32_t ChannelMask          = 0u;
    bool          ReflectanceChanged    = false;
    bool          PhysicalSurfaceChanged = false;
    bool          LayersChanged         = false;

    bool Empty() const
    {
        return ChannelMask == 0u && !ReflectanceChanged && !PhysicalSurfaceChanged && !LayersChanged;
    }
};

struct MaterialProcessingLayerSnapshot
{
    LayerSpecification Layer    = {};
    std::uint32_t      Depth    = 0u;
    std::uint32_t      Position = 0u;
};

/// 🧩 One live material-channel sample resolved by the existing speculative-preview resolver.
/// It carries no cache or document mutation; callers discard it after the current UI/viewport rotation.
struct MaterialLiveChannelPreview
{
    ChannelSubject         Channel = ChannelSubject::ChannelCount;
    ResolvedSample         Sample = {};
    CompiledPhysicalSurface PhysicalSurface = {};
    std::uint64_t          MaterialFingerprint = 0u;
    bool                   PhysicalSurfaceResolved = false;
};

/// 🧩 A worker-safe value snapshot; processing never reads mutable document objects asynchronously.
/// Nested entries are flattened in depth-first sequence order while retaining depth and local position.
struct MaterialProcessingSnapshot
{
    MaterialSpecification                    Material = {};
    PhysicalSurfaceDeclaration               PhysicalDeclaration = {};
    CompiledPhysicalSurface                  PhysicalSurface = {};
    bool                                     PhysicalSurfaceResolved = false;
    std::vector<MaterialProcessingLayerSnapshot> Layers = {};
    MaterialProcessingDirtyKey               DirtyKey = {};
};

/// 🧩 Honest support boundary for the current CPU milestone and the later GPU implementation.
struct MaterialProcessingCapabilities
{
    bool ConstantDielectricDeclarations = true;
    bool ImmutableSnapshots             = true;
    bool ChannelDirtyKeys               = true;
    bool PhysicalSurfaceCompilation     = true;
    bool LayerSequenceResolution        = false;
    bool ImportedImageResolution        = false;
    bool AnalyticResolution             = false;
    bool DeviceProcessing               = false;
};

/// 🧩 Creates and edits the mandatory dielectric material layer without exposing a second material authority.
class MaterialProcessingExchange
{
public:
    Outcome<LayerIdentity> InitialiseDielectric(MaterialSpecification& Material,
                                                SurfaceLayerSequence& Layers) const;
    Outcome<bool> DeclareScalar(MaterialSpecification& Material,
                                const SurfaceLayerSequence& Layers,
                                LayerIdentity Layer, ChannelSubject Channel, double Value) const;
    Outcome<bool> DeclareColour(MaterialSpecification& Material,
                                const SurfaceLayerSequence& Layers,
                                LayerIdentity Layer, ChannelSubject Channel,
                                ColourSpecification Value) const;

    /// 🧩 Copies document-owned declarations before asynchronous processing begins.
    /// 🧩 Captures a renderer-safe physical closure alongside document material and layer declarations.
    /// A failed closure compilation is retained as unresolved rather than replaced with a misleading fallback.
    MaterialProcessingSnapshot Capture(const MaterialSpecification& Material,
                                       const SurfaceLayerSequence& Layers,
                                       const PhysicalSurfaceDeclaration& PhysicalDeclaration = {}) const;

    /// 🧩 Reports dirty channels and structural changes without comparing mutable document storage.
    MaterialProcessingDirtySet Compare(const MaterialProcessingSnapshot& Previous,
                                       const MaterialProcessingSnapshot& Current) const;

    /// 🧩 Resolves one channel through PreviewProjection, preserving its one-resolver and non-mutating guarantees.
    /// This is for immediate paint/material presentation; persistent browser previews use a later bake route.
    Outcome<MaterialLiveChannelPreview> ResolveLiveChannelPreview(
        const MaterialProcessingSnapshot& Snapshot,
        const PreviewProjection& Preview,
        const SurfaceLayerSequence& Layers,
        ChannelSubject Channel,
        double PositionX,
        double PositionY,
        std::uint32_t Level) const;

    MaterialProcessingCapabilities Capabilities() const;

private:
    Outcome<const LayerSpecification*> BaseLayer(const SurfaceLayerSequence& Layers,
                                                 LayerIdentity Layer) const;
};

} // namespace Slate
