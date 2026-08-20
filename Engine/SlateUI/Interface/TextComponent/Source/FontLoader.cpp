#include "SlateUI/Interface/TextComponent/Api/FontLoader.h"

#include "imgui.h"
#include <filesystem>
#include <string>

namespace Slate
{

Outcome<bool> FontLoader::Load(const char* FontRoot, const FontProfile& Profile, float DisplayScale)
{
    if (FontRoot == nullptr || ImGui::GetCurrentContext() == nullptr || ImGui::GetIO().Fonts == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "font context is unavailable" });

    const char* Family = (Profile.Family == 1u) ? "OpenSans" :
                         (Profile.Family == 2u) ? "Archivo" :
                         (Profile.Family == 3u) ? "Inter" :
                         (Profile.Family == 4u) ? "JetBrainsMono" : "Inter";
    const std::filesystem::path Root = std::filesystem::path(FontRoot) / Family;
    if (!std::filesystem::exists(Root))
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "selected font family is not installed" });

    const float Size = 16.0f * ((DisplayScale > 0.0f) ? DisplayScale : 1.0f);
    const std::filesystem::path Regular = Root / (std::string(Family) + "-Regular.ttf");
    if (!std::filesystem::exists(Regular))
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "selected font regular face is missing" });

    ImFont* Loaded = ImGui::GetIO().Fonts->AddFontFromFileTTF(Regular.string().c_str(), Size);
    if (Loaded == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "font atlas rejected the selected face" });

    ActiveFont = Loaded;
    ImGui::GetIO().Fonts->Build();
    return Outcome<bool>::Result(true);
}

} // namespace Slate
