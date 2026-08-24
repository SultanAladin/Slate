//============================================================================================================================================
//                                                  MATERIALPROCESSINGEXCHANGE.H
//============================================================================================================================================
// 🧩 Material-layer commands into validated material declarations; GPU processing follows behind this seam.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "Foundation/Identity.h"
#include "SlateDocument/Document/MaterialSpecification/Api/MaterialSpecification.h"
#include "SlateDocument/Document/SurfaceLayerSequence/Api/SurfaceLayerSequence.h"

namespace Slate
{

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

private:
    Outcome<const LayerSpecification*> BaseLayer(const SurfaceLayerSequence& Layers,
                                                 LayerIdentity Layer) const;
};

} // namespace Slate
