#include "SlateUI/Interface/TextComponent/Api/FontLoader.h"

#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

namespace Slate
{
namespace
{
std::string Lower(std::string Text)
{
    for (char& Letter : Text)
        Letter = static_cast<char>(std::tolower(static_cast<unsigned char>(Letter)));
    return Text;
}

std::size_t Slot(FontWeight Weight, FontSlant Slant)
{
    const std::size_t Step = (static_cast<std::uint32_t>(Weight) - 100u) / 100u;
    return Step * 2u + static_cast<std::uint32_t>(Slant);
}

bool Matches(const std::string& Name, FontWeight Weight, FontSlant Slant)
{
    const std::string LowerName = Lower(Name);
    const char* Word = (Weight == FontWeight::Thin) ? "thin" :
                       (Weight == FontWeight::ExtraLight) ? "extralight" :
                       (Weight == FontWeight::Light) ? "light" :
                       (Weight == FontWeight::Medium) ? "medium" :
                       (Weight == FontWeight::Semibold) ? "semibold" :
                       (Weight == FontWeight::Bold) ? "bold" :
                       (Weight == FontWeight::ExtraBold) ? "extrabold" :
                       (Weight == FontWeight::Black) ? "black" : "regular";
    return LowerName.find(Word) != std::string::npos &&
           (Slant == FontSlant::Italic ? LowerName.find("italic") != std::string::npos
                                       : LowerName.find("italic") == std::string::npos);
}
}

ImFont* FontLoader::Face(FontWeight Weight, FontSlant Slant) const
{
    ImFont* Loaded = Faces[Slot(Weight, Slant)];
    if (Loaded != nullptr)
        return Loaded;
    return Faces[Slot(FontWeight::Regular, FontSlant::Upright)];
}

Outcome<bool> FontLoader::Load(const char* FontRoot, const FontProfile& Profile, float DisplayScale)
{
    if (FontRoot == nullptr || ImGui::GetCurrentContext() == nullptr || ImGui::GetIO().Fonts == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "font context is unavailable" });

    const char* Family = (Profile.Family == FontFamily::OpenSans) ? "OpenSans" :
                         (Profile.Family == FontFamily::Archivo) ? "Archivo" :
                         (Profile.Family == FontFamily::JetBrainsMono) ? "JetBrainsMono" : "Inter";
    const std::filesystem::path Root = std::filesystem::path(FontRoot) / Family;
    if (!std::filesystem::exists(Root))
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "selected font family is not installed" });

    const float Size = 16.0f * ((DisplayScale > 0.0f) ? DisplayScale : 1.0f);
    Faces.fill(nullptr);
    for (std::uint32_t Weight = 100u; Weight <= 900u; Weight += 100u)
    {
        for (std::uint32_t Slant = 0u; Slant < 2u; ++Slant)
        {
            const FontWeight FaceWeight = static_cast<FontWeight>(Weight);
            const FontSlant FaceSlant = static_cast<FontSlant>(Slant);
            for (const auto& Entry : std::filesystem::directory_iterator(Root))
            {
                if (!Entry.is_regular_file() || !Matches(Entry.path().filename().string(), FaceWeight, FaceSlant))
                    continue;
                ImFont* Loaded = ImGui::GetIO().Fonts->AddFontFromFileTTF(Entry.path().string().c_str(), Size);
                if (Loaded != nullptr)
                {
                    Faces[Slot(FaceWeight, FaceSlant)] = Loaded;
                    break;
                }
            }
        }
    }

    if (Face(FontWeight::Regular, FontSlant::Upright) == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "selected font family has no regular face" });

    ImGui::GetIO().Fonts->Build();
    return Outcome<bool>::Result(true);
}

} // namespace Slate
