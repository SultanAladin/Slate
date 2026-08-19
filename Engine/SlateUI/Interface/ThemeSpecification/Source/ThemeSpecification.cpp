//============================================================================================================================================
//                                                     THEMESPECIFICATION.CPP
//============================================================================================================================================
// 🧩 The compiled-in appearances, and the standing copy of them every panel draws from.

#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"

namespace Slate
{

namespace
{

//------------------------------------------------------------------------------------------------------------------------
//                                           WHAT THIS BUILD WAS COMPILED WITH
//------------------------------------------------------------------------------------------------------------------------

// 📝 Transcribed exactly from References/remix-notch-ui/src/App.tsx, and left exactly as transcribed. This
//    run is the answer to "what did the reference say", which is a different question from "what is the
//    interface drawing now" — an appearance file answers the second and must never be able to edit the first.
// 🔴 constexpr, so a caption too long for CaptionCeiling refuses at compile time rather than truncating
//    silently into the standing copy.
constexpr ThemeDeclaration TranscribedThemes[ThemeCeiling] = {
    {"OLED", Covering(0x000000u), Partial(0x09090Bu, .95), Covering(0xF4F4F5u), Covering(0x71717Au),
     Partial(0x27272Au, .80), Covering(0x121214u), Covering(0x000000u), Covering(0x121214u),
     Covering(0x09090Bu), Covering(0x151517u), Covering(0x222223u),
     Covering(0x1E1E20u), Covering(0x2A2A2Cu)},
    {"Dark", Covering(0x0A0A0Au), Partial(0x18181Bu, .95), Covering(0xF4F4F5u), Covering(0xA1A1AAu),
     Covering(0x27272Au), Covering(0x1F1F22u), Covering(0x18181Bu), Covering(0x27272Au),
     Covering(0x18181Bu), Covering(0x2F2F32u), Covering(0x464649u),
     Covering(0x3D3D3Fu), Covering(0x525255u)},
    {"Clean White", Covering(0xF4F4F5u), Partial(0xFFFFFFu, .95), Covering(0x18181Bu), Covering(0x71717Au),
     Covering(0xE4E4E7u), Covering(0xFAFAFAu), Covering(0xE5E5EAu), Covering(0xFFFFFFu),
     Covering(0xE5E5EAu), Covering(0xCECED3u), Covering(0xB7B7BBu),
     Covering(0xE6E6E6u), Covering(0xCCCCCCu)},
    {"Desert Sand", Covering(0xE8D5B5u), Partial(0xF2E5CCu, .95), Covering(0x4A3B2Cu), Covering(0x8A7356u),
     Covering(0xCFAE7Eu), Covering(0xFAEED9u), Covering(0xDCB679u), Covering(0xF4E4C4u),
     Covering(0xE8D5B5u), Covering(0xE3C99Du), Covering(0xE1C291u),
     Covering(0xEAD2A6u), Covering(0xE6C897u)},
    {"Purplish", Covering(0x0F0A1Cu), Partial(0x17102Bu, .95), Covering(0xF3E8FFu), Covering(0xC084FCu),
     Partial(0x581C87u, .50), Covering(0x1D1438u), Covering(0x1F163Du), Covering(0x2D2054u),
     Covering(0x23174Au), Covering(0x47366Eu), Covering(0x6B5692u),
     Covering(0x4F3E76u), Covering(0x715B98u)},
    {"Bluish", Covering(0x09111Cu), Partial(0x0F1B2Du, .95), Covering(0xDBEAFEu), Covering(0x60A5FAu),
     Partial(0x1E3A8Au, .50), Covering(0x15253Du), Covering(0x1A2D4Au), Covering(0x264066u),
     Covering(0x1C3152u), Covering(0x344F74u), Covering(0x4C6C96u),
     Covering(0x3C5B84u), Covering(0x5275A2u)}};

constexpr AccentDeclaration TranscribedAccents[AccentCeiling] = {
    {"Blue", Covering(0x3B82F6u)},  {"Cyan", Covering(0x06B6D4u)},
    {"Teal", Covering(0x14B8A6u)},  {"Emerald", Covering(0x10B981u)},
    {"Amber", Covering(0xF59E0Bu)}, {"Orange", Covering(0xF97316u)},
    {"Rose", Covering(0xF43F5Eu)},  {"Violet", Covering(0x8B5CF6u)}};

//------------------------------------------------------------------------------------------------------------------------
//                                           WHAT THE INTERFACE IS DRAWING NOW
//------------------------------------------------------------------------------------------------------------------------

// 🔴 One standing copy, seeded from the transcription and replaced whole by Adopt. Panels read through the
//    accessors rather than reaching the run directly, so an adopted appearance reaches all of them at once
//    and none of them can hold an ink the archive no longer contains.
ThemeDeclaration  StandingThemes[ThemeCeiling]   = {};
AccentDeclaration StandingAccents[AccentCeiling] = {};
bool              Seeded                         = false;

// 📝 Seeding is deferred to first read rather than done in a constructor. Static initialisation order across
//    translation units is not ordered, and a panel constructed during static initialisation would otherwise
//    read a run of zeroes — every ink fully transparent, which presents as an interface that did not draw.
void SeedOnce()
{
    if (Seeded) return;

    for (std::uint32_t Ordinal = 0u; Ordinal < ThemeCeiling; ++Ordinal)
    {
        StandingThemes[Ordinal] = TranscribedThemes[Ordinal];
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < AccentCeiling; ++Ordinal)
    {
        StandingAccents[Ordinal] = TranscribedAccents[Ordinal];
    }

    Seeded = true;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                            READING THE STANDING APPEARANCE
//------------------------------------------------------------------------------------------------------------------------

const ThemeDeclaration& ThemeSpecification::Theme(ThemeSubject Subject)
{
    SeedOnce();

    const std::uint32_t Ordinal = static_cast<std::uint32_t>(Subject);
    return StandingThemes[(Ordinal < ThemeCeiling) ? Ordinal : 0u];
}

const AccentDeclaration& ThemeSpecification::Accent(AccentSubject Subject)
{
    SeedOnce();

    const std::uint32_t Ordinal = static_cast<std::uint32_t>(Subject);
    return StandingAccents[(Ordinal < AccentCeiling) ? Ordinal : 0u];
}

//------------------------------------------------------------------------------------------------------------------------
//                                           REPLACING THE STANDING APPEARANCE
//------------------------------------------------------------------------------------------------------------------------

void ThemeSpecification::Adopt(const ThemeArchive& Arriving)
{
    // 📝 Seeded is raised before the copy rather than after. The copy writes every element of both runs, so
    //    the seed it would otherwise perform first is work whose result is immediately overwritten.
    Seeded = true;

    for (std::uint32_t Ordinal = 0u; Ordinal < ThemeCeiling; ++Ordinal)
    {
        StandingThemes[Ordinal] = Arriving.Themes[Ordinal];
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < AccentCeiling; ++Ordinal)
    {
        StandingAccents[Ordinal] = Arriving.Accents[Ordinal];
    }
}

ThemeArchive ThemeSpecification::Standing(const ThemeSelection& Selected)
{
    SeedOnce();

    ThemeArchive Produced;
    Produced.Selected = Selected;

    for (std::uint32_t Ordinal = 0u; Ordinal < ThemeCeiling; ++Ordinal)
    {
        Produced.Themes[Ordinal] = StandingThemes[Ordinal];
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < AccentCeiling; ++Ordinal)
    {
        Produced.Accents[Ordinal] = StandingAccents[Ordinal];
    }

    return Produced;
}

void ThemeSpecification::Restore()
{
    Seeded = false;
    SeedOnce();
}

}   // namespace Slate
