#include "SlateUI/Interface/TextComponent/Api/FontLoader.h"

#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <cstring>
#include <cstdio>

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

Outcome<bool> FontLoader::Discover(const char* FontRoot)
{
    Root = FontRoot != nullptr ? FontRoot : "";
    Families.clear();
    if (FontRoot == nullptr || !std::filesystem::exists(FontRoot))
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "font archive directory is unavailable" });

    for (const auto& Entry : std::filesystem::directory_iterator(FontRoot))
    {
        if (Entry.is_directory())
            Families.push_back(Entry.path().filename().string());
    }
    std::sort(Families.begin(), Families.end());
    std::fprintf(stderr, "[Fonts] discovered %u families in %s\n",
                 static_cast<unsigned>(Families.size()), FontRoot);
    return Outcome<bool>::Result(true);
}

const char* FontLoader::FamilyName(std::uint32_t Ordinal) const
{
    return Ordinal < Families.size() ? Families[Ordinal].c_str() : nullptr;
}

bool FontLoader::HasFace(FontWeight Weight, FontSlant Slant) const
{
    return Faces[Slot(Weight, Slant)] != nullptr;
}

ImFont* FontLoader::Face(FontWeight Weight, FontSlant Slant) const
{
    ImFont* Loaded = Faces[Slot(Weight, Slant)];
    if (Loaded != nullptr)
        return Loaded;
    return Faces[Slot(FontWeight::Regular, FontSlant::Upright)];
}

ImFont* FontLoader::Preview(const char* Family, float DisplayScale)
{
    if (Family == nullptr || Root.empty() || ImGui::GetCurrentContext() == nullptr)
        return nullptr;
    const std::filesystem::path Directory = std::filesystem::path(Root) / Family;
    if (!std::filesystem::exists(Directory)) return nullptr;
    const float Size = 16.0f * ((DisplayScale > 0.0f) ? DisplayScale : 1.0f);
    for (const auto& Entry : std::filesystem::directory_iterator(Directory))
    {
        const std::string Name = Lower(Entry.path().filename().string());
        if (Entry.is_regular_file() && Name.find("regular") != std::string::npos && Name.find("italic") == std::string::npos)
            return ImGui::GetIO().Fonts->AddFontFromFileTTF(Entry.path().string().c_str(), Size);
    }
    for (const auto& Entry : std::filesystem::directory_iterator(Directory))
    {
        const std::string Name = Lower(Entry.path().filename().string());
        if (Entry.is_regular_file() && Name.find("italic") == std::string::npos &&
            (Name.ends_with(".ttf") || Name.ends_with(".otf")))
            return ImGui::GetIO().Fonts->AddFontFromFileTTF(Entry.path().string().c_str(), Size);
    }
    return nullptr;
}

Outcome<bool> FontLoader::Load(const char* FontRoot, const FontProfile& Profile, float DisplayScale)
{
    if (FontRoot == nullptr || ImGui::GetCurrentContext() == nullptr || ImGui::GetIO().Fonts == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "font context is unavailable" });

    const char* Family = (Profile.Family[0] != '\0') ? Profile.Family : "Inter";
    const std::filesystem::path Root = std::filesystem::path(FontRoot) / Family;
    if (!std::filesystem::exists(Root))
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "selected font family is not installed" });

    const float Size = 16.0f * ((DisplayScale > 0.0f) ? DisplayScale : 1.0f);
    Faces.fill(nullptr);
    std::uint32_t LoadedCount = 0u;
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
                    ++LoadedCount;
                    break;
                }
            }
        }
    }

    // Variable fonts often publish one upright file instead of a file named Regular.
    // Use that file as the regular face rather than falling back to ImGui.
    if (Face(FontWeight::Regular, FontSlant::Upright) == nullptr)
    {
        for (const auto& Entry : std::filesystem::directory_iterator(Root))
        {
            const std::string Name = Lower(Entry.path().filename().string());
            if (!Entry.is_regular_file() || Name.find("italic") != std::string::npos)
                continue;
            if (Name.ends_with(".ttf") || Name.ends_with(".otf"))
            {
                ImFont* Loaded = ImGui::GetIO().Fonts->AddFontFromFileTTF(Entry.path().string().c_str(), Size);
                if (Loaded != nullptr)
                {
                    Faces[Slot(FontWeight::Regular, FontSlant::Upright)] = Loaded;
                    ++LoadedCount;
                    break;
                }
            }
        }
    }

    if (Face(FontWeight::Regular, FontSlant::Upright) == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "selected font family has no usable upright face" });

    ImGui::GetIO().Fonts->Build();
    std::fprintf(stderr, "[Fonts] loaded %u faces for %s\n",
                 static_cast<unsigned>(LoadedCount), Family);
    return Outcome<bool>::Result(true);
}

} // namespace Slate
