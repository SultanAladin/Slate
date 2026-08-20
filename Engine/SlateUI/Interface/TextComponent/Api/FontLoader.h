#pragma once

#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "Contract/DeliveryContract.h"

#include <array>
#include <string>
#include <vector>

struct ImFont;

namespace Slate
{

/// Loads the selected typeface faces into the active ImGui atlas.
class FontLoader
{
public:
    Outcome<bool> Discover(const char* FontRoot);
    Outcome<bool> Load(const char* FontRoot, const FontProfile& Profile, float DisplayScale);
    std::uint32_t FamilyCount() const { return static_cast<std::uint32_t>(Families.size()); }
    const char* FamilyName(std::uint32_t Ordinal) const;
    ImFont* Active() const { return Face(FontWeight::Regular, FontSlant::Upright); }
    ImFont* Face(FontWeight Weight, FontSlant Slant) const;
    bool HasFace(FontWeight Weight, FontSlant Slant) const;
    ImFont* Preview(const char* Family, float DisplayScale);

private:
    std::array<ImFont*, 18u> Faces{};
    std::vector<std::string> Families;
    std::string Root;
};

} // namespace Slate
