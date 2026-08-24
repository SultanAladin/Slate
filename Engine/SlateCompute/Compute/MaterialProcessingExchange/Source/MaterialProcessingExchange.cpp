//============================================================================================================================================
//                                                MATERIALPROCESSINGEXCHANGE.CPP
//============================================================================================================================================

#include "SlateCompute/Compute/MaterialProcessingExchange/Api/MaterialProcessingExchange.h"

#include <algorithm>

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

} // namespace Slate
