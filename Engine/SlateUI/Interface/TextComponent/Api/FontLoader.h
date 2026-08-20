#pragma once

#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "Contract/DeliveryContract.h"

struct ImFont;

namespace Slate
{

/// Loads the selected typeface faces into the active ImGui atlas.
class FontLoader
{
public:
    Outcome<bool> Load(const char* FontRoot, const FontProfile& Profile, float DisplayScale);
    ImFont* Active() const { return ActiveFont; }

private:
    ImFont* ActiveFont = nullptr;
};

} // namespace Slate
