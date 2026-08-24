//============================================================================================================================================
//                                                MATERIALPROCESSINGEXCHANGE.CPP
//============================================================================================================================================

#include "SlateCompute/Compute/MaterialProcessingExchange/Api/MaterialProcessingExchange.h"

#include <algorithm>
#include <cstring>

namespace Slate
{
namespace
{
constexpr std::uint32_t ChannelBit(ChannelSubject Channel)
{
    return 1u << static_cast<std::uint32_t>(Channel);
}

ChannelSpecification Scalar(double Value, double Default)
{
    ChannelSpecification Declared;
    Declared.Source = ChannelSource::Constant;
    Declared.Measured = ChannelMeasure::Scalar;
    Declared.ConstantScalar = Value;
    Declared.DefaultScalar = Default;
    Declared.LowerMagnitude = 0.0;
    Declared.UpperMagnitude = 1.0;
    return Declared;
}

ChannelSpecification Colour(double Red, double Green, double Blue)
{
    ChannelSpecification Declared;
    Declared.Source = ChannelSource::Constant;
    Declared.Measured = ChannelMeasure::Reflectance;
    Declared.ConstantColour = { Red, Green, Blue, WorkingSpaceIdentity };
    Declared.DefaultColour = Declared.ConstantColour;
    Declared.LowerMagnitude = 0.0;
    Declared.UpperMagnitude = 1.0;
    return Declared;
}

constexpr std::uint64_t HashSeed = 1469598103934665603ull;
constexpr std::uint64_t HashPrime = 1099511628211ull;

void HashBytes(std::uint64_t& Hash, const void* Data, std::size_t Span)
{
    const auto* Bytes = static_cast<const unsigned char*>(Data);
    for (std::size_t ByteIndex = 0u; ByteIndex < Span; ++ByteIndex)
    {
        Hash ^= Bytes[ByteIndex];
        Hash *= HashPrime;
    }
}

template <typename ValueType>
void HashValue(std::uint64_t& Hash, const ValueType& Value)
{
    HashBytes(Hash, &Value, sizeof(Value));
}

void HashColour(std::uint64_t& Hash, const ColourSpecification& Colour)
{
    HashValue(Hash, Colour.RedCoordinate);
    HashValue(Hash, Colour.GreenCoordinate);
    HashValue(Hash, Colour.BlueCoordinate);
    HashValue(Hash, Colour.SpaceIdentity);
}

std::uint64_t HashChannel(const ChannelSpecification& Channel)
{
    std::uint64_t Hash = HashSeed;
    HashValue(Hash, Channel.Source);
    HashValue(Hash, Channel.Measured);
    HashValue(Hash, Channel.ConstantScalar);
    HashColour(Hash, Channel.ConstantColour);
    HashValue(Hash, Channel.DefaultScalar);
    HashColour(Hash, Channel.DefaultColour);
    HashValue(Hash, Channel.LowerMagnitude);
    HashValue(Hash, Channel.UpperMagnitude);
    HashValue(Hash, Channel.ChannelDeclared);
    return Hash;
}

void HashPhysicalDeclaration(std::uint64_t& Hash, const PhysicalSurfaceDeclaration& Declaration)
{
    HashValue(Hash, Declaration.Closure);
    HashValue(Hash, Declaration.Features);
    HashValue(Hash, Declaration.Coverage);
    HashValue(Hash, Declaration.Wall);
    HashValue(Hash, Declaration.Interface);
    HashValue(Hash, Declaration.TwoSided);
}

void HashPainted(std::uint64_t& Hash, const PaintedContent& Painted)
{
    HashValue(Hash, Painted.ExtentTexels);
    HashValue(Hash, Painted.ComponentCount);
    const std::uint64_t TexelCount = static_cast<std::uint64_t>(Painted.Texels.size());
    HashValue(Hash, TexelCount);
    if (!Painted.Texels.empty())
        HashBytes(Hash, Painted.Texels.data(), Painted.Texels.size() * sizeof(float));
}

void HashLayer(std::uint64_t& Hash, const MaterialProcessingLayerSnapshot& Snapshot)
{
    const LayerSpecification& Layer = Snapshot.Layer;
    HashValue(Hash, Snapshot.Depth);
    HashValue(Hash, Snapshot.Position);
    HashValue(Hash, Layer.Identity.SlotIndex);
    HashValue(Hash, Layer.Identity.SlotGeneration);
    HashValue(Hash, Layer.Source);
    HashValue(Hash, Layer.SourceIndex);
    HashValue(Hash, Layer.NestedIndex);
    HashValue(Hash, Layer.ChannelMask);
    HashValue(Hash, Layer.Combination);
    HashValue(Hash, Layer.Coverage.Source);
    HashValue(Hash, Layer.Coverage.SourceIndex);
    HashPainted(Hash, Layer.Coverage.Painted);
    HashValue(Hash, Layer.Coverage.UniformStrength);
    HashValue(Hash, Layer.Coverage.CoverageDeclared);
    HashPainted(Hash, Layer.Painted);
    const std::uint64_t NameLength = static_cast<std::uint64_t>(Layer.Name.size());
    HashValue(Hash, NameLength);
    HashBytes(Hash, Layer.Name.data(), Layer.Name.size());
    HashValue(Hash, Layer.PresenceEnabled);
    HashValue(Hash, Layer.Mandatory);
    HashValue(Hash, Layer.ResampleOwed);
}

void CaptureLayers(const SurfaceLayerSequence& Sequence,
                   std::uint32_t Depth,
                   std::vector<MaterialProcessingLayerSnapshot>& Captured)
{
    const std::vector<LayerSpecification>& Entries = Sequence.Entries();
    for (std::size_t Position = 0u; Position < Entries.size(); ++Position)
    {
        Captured.push_back({ Entries[Position], Depth, static_cast<std::uint32_t>(Position) });
        if (Entries[Position].Source != LayerContentSource::NestedSequence) continue;

        const Outcome<const SurfaceLayerSequence*> Nested = Sequence.Nested(Entries[Position].NestedIndex);
        if (Nested.Resolved) CaptureLayers(*Nested.Resolve(), Depth + 1u, Captured);
    }
}
}

Outcome<LayerIdentity> MaterialProcessingExchange::InitialiseDielectric(MaterialSpecification& Material,
                                                                        SurfaceLayerSequence& Layers) const
{
    if (Layers.EntryCount() != 0u)
        return Outcome<LayerIdentity>::Refuse(
            { RefusalReason::HostDenied, "a material layer sequence already stands" });

    Material.DeclareReflectance(ReflectanceSelection::Standard);
    const struct ChannelDeclaration
    {
        ChannelSubject Subject;
        ChannelSpecification Specification;
    } Declared[] = {
        { ChannelSubject::AlbedoColour, Colour(1.0, 1.0, 1.0) },
        { ChannelSubject::Metallic, Scalar(0.0, 0.0) },
        { ChannelSubject::Roughness, Scalar(0.5, 0.5) },
        { ChannelSubject::NormalIncidenceReflectance, Scalar(0.04, 0.04) },
        { ChannelSubject::AmbientOcclusion, Scalar(1.0, 1.0) },
        { ChannelSubject::Emission, Colour(0.0, 0.0, 0.0) },
        { ChannelSubject::Opacity, Scalar(1.0, 1.0) }
    };

    std::uint32_t Mask = 0u;
    for (const ChannelDeclaration& Channel : Declared)
    {
        const Outcome<bool> Accepted = Material.DeclareChannel(Channel.Subject, Channel.Specification);
        if (!Accepted.Resolved)
            return Outcome<LayerIdentity>::Refuse(Accepted.Error);
        Mask |= ChannelBit(Channel.Subject);
    }

    LayerSpecification Base;
    Base.Source = LayerContentSource::MaterialConstants;
    Base.ChannelMask = Mask;
    Base.Combination = CombineSpecification::Over;
    Base.Name = "Base Material";
    Base.PresenceEnabled = true;
    Base.Mandatory = true;
    return Layers.Append(Base);
}

Outcome<const LayerSpecification*> MaterialProcessingExchange::BaseLayer(const SurfaceLayerSequence& Layers,
                                                                         LayerIdentity Layer) const
{
    const Outcome<const LayerSpecification*> Resolved = Layers.Resolve(Layer);
    if (!Resolved.Resolved) return Resolved;
    if (!Resolved.Resolve()->Mandatory ||
        Resolved.Resolve()->Source != LayerContentSource::MaterialConstants)
    {
        return Outcome<const LayerSpecification*>::Refuse(
            { RefusalReason::HostDenied, "material constants may only be edited through the base material layer" });
    }
    return Resolved;
}

Outcome<bool> MaterialProcessingExchange::DeclareScalar(MaterialSpecification& Material,
                                                        const SurfaceLayerSequence& Layers,
                                                        LayerIdentity Layer, ChannelSubject Channel,
                                                        double Value) const
{
    const Outcome<const LayerSpecification*> Base = BaseLayer(Layers, Layer);
    if (!Base.Resolved) return Outcome<bool>::Refuse(Base.Error);
    if (!EntryWritesChannel(*Base.Resolve(), Channel))
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the base layer does not declare this channel" });

    ChannelSpecification Amended = Material.Channel(Channel);
    if (Amended.Measured != ChannelMeasure::Scalar)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the channel is not scalar" });
    Amended.Source = ChannelSource::Constant;
    Amended.ConstantScalar = std::clamp(Value, Amended.LowerMagnitude, Amended.UpperMagnitude);
    return Material.DeclareChannel(Channel, Amended);
}

Outcome<bool> MaterialProcessingExchange::DeclareColour(MaterialSpecification& Material,
                                                        const SurfaceLayerSequence& Layers,
                                                        LayerIdentity Layer, ChannelSubject Channel,
                                                        ColourSpecification Value) const
{
    const Outcome<const LayerSpecification*> Base = BaseLayer(Layers, Layer);
    if (!Base.Resolved) return Outcome<bool>::Refuse(Base.Error);
    if (!EntryWritesChannel(*Base.Resolve(), Channel))
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the base layer does not declare this channel" });
    if (!Value.ColourDeclared())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the colour declares no space" });

    ChannelSpecification Amended = Material.Channel(Channel);
    if (!MeasureCarriesColour(Amended.Measured))
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the channel does not carry colour" });
    Amended.Source = ChannelSource::Constant;
    Amended.ConstantColour = Value;
    return Material.DeclareChannel(Channel, Amended);
}

MaterialProcessingSnapshot MaterialProcessingExchange::Capture(const MaterialSpecification& Material,
                                                               const SurfaceLayerSequence& Layers,
                                                               const PhysicalSurfaceDeclaration& PhysicalDeclaration) const
{
    MaterialProcessingSnapshot Captured;
    Captured.Material = Material;
    Captured.PhysicalDeclaration = PhysicalDeclaration;
    const Outcome<CompiledPhysicalSurface> Physical = PhysicalSurfaceExchange().Compile(Material, PhysicalDeclaration);
    if (Physical.Resolved)
    {
        Captured.PhysicalSurface = Physical.Resolve();
        Captured.PhysicalSurfaceResolved = true;
    }
    CaptureLayers(Layers, 0u, Captured.Layers);

    Captured.DirtyKey.Reflectance = HashSeed;
    const ReflectanceSelection Selected = Material.Reflectance();
    HashValue(Captured.DirtyKey.Reflectance, Selected);

    Captured.DirtyKey.PhysicalSurface = HashSeed;
    HashPhysicalDeclaration(Captured.DirtyKey.PhysicalSurface, PhysicalDeclaration);
    HashValue(Captured.DirtyKey.PhysicalSurface, Captured.PhysicalSurfaceResolved);

    for (std::size_t ChannelIndex = 0u; ChannelIndex < MaterialProcessingDirtyKey::ChannelSpan; ++ChannelIndex)
    {
        const ChannelSubject Subject = static_cast<ChannelSubject>(ChannelIndex);
        Captured.DirtyKey.Channels[ChannelIndex] = HashChannel(Material.Channel(Subject));
    }

    Captured.DirtyKey.Layers = HashSeed;
    for (const MaterialProcessingLayerSnapshot& Layer : Captured.Layers)
        HashLayer(Captured.DirtyKey.Layers, Layer);

    Captured.DirtyKey.Combined = HashSeed;
    HashValue(Captured.DirtyKey.Combined, Captured.DirtyKey.Reflectance);
    HashValue(Captured.DirtyKey.Combined, Captured.DirtyKey.PhysicalSurface);
    for (std::uint64_t ChannelKey : Captured.DirtyKey.Channels)
        HashValue(Captured.DirtyKey.Combined, ChannelKey);
    HashValue(Captured.DirtyKey.Combined, Captured.DirtyKey.Layers);
    return Captured;
}

MaterialProcessingDirtySet MaterialProcessingExchange::Compare(const MaterialProcessingSnapshot& Previous,
                                                               const MaterialProcessingSnapshot& Current) const
{
    MaterialProcessingDirtySet Dirty;
    Dirty.ReflectanceChanged = Previous.DirtyKey.Reflectance != Current.DirtyKey.Reflectance;
    Dirty.PhysicalSurfaceChanged = Previous.DirtyKey.PhysicalSurface != Current.DirtyKey.PhysicalSurface;
    Dirty.LayersChanged = Previous.DirtyKey.Layers != Current.DirtyKey.Layers;
    for (std::size_t ChannelIndex = 0u; ChannelIndex < MaterialProcessingDirtyKey::ChannelSpan; ++ChannelIndex)
    {
        if (Previous.DirtyKey.Channels[ChannelIndex] != Current.DirtyKey.Channels[ChannelIndex])
            Dirty.ChannelMask |= 1u << static_cast<std::uint32_t>(ChannelIndex);
    }
    return Dirty;
}

MaterialProcessingCapabilities MaterialProcessingExchange::Capabilities() const
{
    return {};
}

} // namespace Slate
