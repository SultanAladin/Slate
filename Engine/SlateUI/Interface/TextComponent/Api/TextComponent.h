#pragma once

#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "SlateUI/Interface/InterfaceExchange/Api/RecordingSurface.h"

namespace Slate
{

enum class TextWeight : std::uint32_t
{
    Regular = 0u,
    Medium = 1u,
    Semibold = 2u,
    Bold = 3u
};

struct TextStyle
{
    float Size = 14.0f;
    float Tracking = 0.0f;
    float LineHeight = 0.0f;
    TextWeight Weight = TextWeight::Regular;
};

struct TextMetrics
{
    float Width = 0.0f;
    float Height = 0.0f;
    float Baseline = 0.0f;
};

/// Shared measurement and drawing helpers for text-dependent controls.
class TextComponent
{
public:
    static TextMetrics Measure(const RecordingSurface& Surface,
                               const char* Text,
                               const TextStyle& Style);

    static PlaneExtent Fit(const PlaneExtent& Origin,
                           const TextMetrics& Metrics,
                           float PaddingAlong,
                           float PaddingAcross);

    static void Draw(RecordingSurface& Surface,
                     const PlaneExtent& Bounds,
                     ThemeToken Colour,
                     const char* Text,
                     const TextStyle& Style);
};

} // namespace Slate
