//============================================================================================================================================
//                                                      GEOMETRYDEVICEEXCHANGE.CPP
//============================================================================================================================================
// 🧩 Constructs the device estate before any real geometry source is admitted to visibility residency.

#include "SlateCompute/Compute/GeometryDeviceExchange/Api/GeometryDeviceExchange.h"

#include "Foundation/NumericTolerance.h"

namespace Slate
{

GeometryDeviceExchange::~GeometryDeviceExchange()
{
    Reclaim();
}

Outcome<bool> GeometryDeviceExchange::ConstructGeometryDeviceExchange(
    const VulkanExchange&      Exchange,
    const DiagnosticExtension& Naming,
    const char*                 ShaderLocation,
    std::uint32_t               DisplayWidth,
    std::uint32_t               DisplayHeight,
    VkFormat                     DisplayFormat)
{
    if (Constructed)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::HostDenied, "the geometry device exchange already stands" });
    }

    if (ShaderLocation == nullptr || ShaderLocation[0] == '\0')
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "no geometry shader stream location was supplied" });
    }

    if (!Bytes.ReserveByteSpace(Exchange, Naming).Resolved ||
        !Images.ConstructImageSpace(Exchange, Bytes, Naming).Resolved ||
        !Targets.Reserve(Images, DisplayWidth, DisplayHeight, DisplayFormat).Resolved ||
        !Spans.ConstructSpanSpace(Exchange, Bytes, Naming).Resolved ||
        !Streams.AttachShaderStreams(Exchange, ShaderLocation).Resolved ||
        !Descriptors.ConstructDescriptorIndex(Exchange, Naming).Resolved ||
        !Programs.ConstructProgramIndex(Exchange, Streams, Descriptors, Naming).Resolved ||
        !Attachments.ConstructAttachmentIndex(Exchange, Targets).Resolved ||
        !Raster.ConstructVisibilityRaster(Spans, Streams, Descriptors, Programs, Attachments).Resolved ||
        !Descriptors.Fix(RecordingSlotCount).Resolved ||
        !Raster.Derive(DisplayWidth, DisplayHeight).Resolved)
    {
        Reclaim();
        return Outcome<bool>::Refuse(
            { RefusalReason::CapabilityAbsent, "the geometry visibility device estate was rejected" });
    }

    Constructed = true;
    return Outcome<bool>::Result(true);
}

Outcome<bool> GeometryDeviceExchange::ReclaimDisplay(std::uint32_t DisplayWidth, std::uint32_t DisplayHeight)
{
    if (!Constructed)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::CapabilityAbsent, "the geometry device exchange does not stand" });
    }

    Attachments.Release();

    const Outcome<bool> Reclaimed = Targets.Reclaim(DisplayWidth, DisplayHeight);
    if (!Reclaimed.Resolved)
        return Reclaimed;

    return Raster.Derive(DisplayWidth, DisplayHeight);
}

VisibilityRaster& GeometryDeviceExchange::Visibility()
{
    return Raster;
}

Outcome<ImageReservation> GeometryDeviceExchange::Radiance() const
{
    if (!Constructed)
    {
        return Outcome<ImageReservation>::Refuse(
            { RefusalReason::CapabilityAbsent, "the geometry device exchange does not stand" });
    }

    return Targets.Resolve(SharedTarget::RadianceSurface);
}

void GeometryDeviceExchange::Reclaim()
{
    Raster.Reclaim();
    Attachments.Reclaim();
    Programs.Reclaim();
    Descriptors.Reclaim();
    Streams.Reclaim();
    Spans.Reclaim();
    Targets.Release();
    Images.Reclaim();
    Bytes.Reclaim();
    Constructed = false;
}

bool GeometryDeviceExchange::Standing() const
{
    return Constructed;
}

}   // namespace Slate
