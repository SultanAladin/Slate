#pragma once

#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "Contract/DeliveryContract.h"

#include <array>

struct ImFont;

namespace Slate
{

/// Loads the selected typeface faces into the active ImGui atlas.
class FontLoader
{
public:
    Outcome<bool> Load(const char* FontRoot, const FontProfile& Profile, float DisplayScale);
    ImFont* Active() const { return Face(FontWeight::Regular, FontSlant::Upright); }
    ImFont* Face(FontWeight Weight, FontSlant Slant) const;

private:
    std::array<ImFont*, 18u> Faces{};
};

} // namespace Slate
