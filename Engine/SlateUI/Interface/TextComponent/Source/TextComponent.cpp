#include "SlateUI/Interface/TextComponent/Api/TextComponent.h"

namespace Slate
{

TextMetrics TextComponent::Measure(const RecordingSurface& Surface,
                                   const char* Text,
                                   const TextStyle& Style)
{
    const float Height = (Style.LineHeight > 0.0f) ? Style.LineHeight : Style.Size * 1.25f;
    return { Surface.MeasureRun(Text != nullptr ? Text : "", Style.Size, Style.Tracking), Height, Style.Size };
}

PlaneExtent TextComponent::Fit(const PlaneExtent& Origin,
                               const TextMetrics& Metrics,
                               float PaddingAlong,
                               float PaddingAcross)
{
    const float Width = Metrics.Width + PaddingAlong * 2.0f;
    const float Height = Metrics.Height + PaddingAcross * 2.0f;
    return PlaneExtent{Origin.LeastAlong, Origin.LeastAcross,
                       Origin.LeastAlong + Width, Origin.LeastAcross + Height};
}

void TextComponent::Draw(RecordingSurface& Surface,
                         const PlaneExtent& Bounds,
                         ThemeToken Colour,
                         const char* Text,
                         const TextStyle& Style)
{
    const TextMetrics Metrics = Measure(Surface, Text, Style);
    Surface.TextRun(Bounds.LeastAlong + (Bounds.SpanAlong() - Metrics.Width) * 0.5f,
                    Bounds.LeastAcross + (Bounds.SpanAcross() - Metrics.Height) * 0.5f,
                    Colour, Text != nullptr ? Text : "", Style.Size, Style.Tracking, true);
}

} // namespace Slate
