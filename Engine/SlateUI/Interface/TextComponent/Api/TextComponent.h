#pragma once

#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"

namespace Slate
{

struct TextStyle
{
    float Size = 14.0f;
    float Tracking = 0.0f;
    float LineHeight = 0.0f;
    FontWeight Weight = FontWeight::Regular;
    FontSlant Slant = FontSlant::Upright;
};

struct TypographyMetrics
{
    float Width = 0.0f;
    float Height = 0.0f;
    float Baseline = 0.0f;
};

/// Shared measurement and drawing helpers for text-dependent controls.
class TextComponent
{
public:
    static TypographyMetrics Measure(const RecordingSurface& Surface,
                               const char* Text,
                               const TextStyle& Style);

    static PlaneExtent Fit(const PlaneExtent& Origin,
                           const TypographyMetrics& Metrics,
                           float PaddingAlong,
                           float PaddingAcross);

    static void Draw(RecordingSurface& Surface,
                     const PlaneExtent& Bounds,
                     ThemeToken Colour,
                     const char* Text,
                     const TextStyle& Style);
};

} // namespace Slate
