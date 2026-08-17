//============================================================================================================================================
//                                                       THEMESPECIFICATION.CPP
//============================================================================================================================================
// 🧩 Exact theme and semantic-colour constants from
// References/remix-notch-ui/src/App.tsx.

#include "SlateUI/Interface/ThemeSpecification/Api/ThemeSpecification.h"

namespace Slate
{

namespace
{

constexpr ThemeDeclaration Themes[] = {
    {"OLED", Covering(0x000000u), Partial(0x09090Bu, .95), Covering(0xF4F4F5u), Covering(0x71717Au),
     Partial(0x27272Au, .80), Covering(0x121214u), Covering(0x000000u), Covering(0x121214u), Partial(0xFFFFFFu, .05),
     Partial(0xFFFFFFu, .10)},
    {"Dark", Covering(0x0A0A0Au), Partial(0x18181Bu, .95), Covering(0xF4F4F5u), Covering(0xA1A1AAu),
     Covering(0x27272Au), Covering(0x1F1F22u), Covering(0x18181Bu), Covering(0x27272Au), Partial(0xFFFFFFu, .10),
     Partial(0xFFFFFFu, .20)},
    {"Clean White", Covering(0xF4F4F5u), Partial(0xFFFFFFu, .95), Covering(0x18181Bu), Covering(0x71717Au),
     Covering(0xE4E4E7u), Covering(0xFAFAFAu), Covering(0xE5E5EAu), Covering(0xFFFFFFu), Partial(0x000000u, .10),
     Partial(0x000000u, .20)},
    {"Desert Sand", Covering(0xE8D5B5u), Partial(0xF2E5CCu, .95), Covering(0x4A3B2Cu), Covering(0x8A7356u),
     Covering(0xCFAE7Eu), Covering(0xFAEED9u), Covering(0xDCB679u), Covering(0xF4E4C4u), Partial(0xDCB679u, .40),
     Partial(0xDCB679u, .60)},
    {"Purplish", Covering(0x0F0A1Cu), Partial(0x17102Bu, .95), Covering(0xF3E8FFu), Covering(0xC084FCu),
     Partial(0x581C87u, .50), Covering(0x1D1438u), Covering(0x1F163Du), Covering(0x2D2054u), Partial(0xD8B4FEu, .20),
     Partial(0xD8B4FEu, .40)},
    {"Bluish", Covering(0x09111Cu), Partial(0x0F1B2Du, .95), Covering(0xDBEAFEu), Covering(0x60A5FAu),
     Partial(0x1E3A8Au, .50), Covering(0x15253Du), Covering(0x1A2D4Au), Covering(0x264066u), Partial(0x93C5FDu, .20),
     Partial(0x93C5FDu, .40)}};

constexpr AccentDeclaration Accents[] = {{"Blue", Covering(0x3B82F6u)},  {"Cyan", Covering(0x06B6D4u)},
                                         {"Teal", Covering(0x14B8A6u)},  {"Emerald", Covering(0x10B981u)},
                                         {"Amber", Covering(0xF59E0Bu)}, {"Orange", Covering(0xF97316u)},
                                         {"Rose", Covering(0xF43F5Eu)},  {"Violet", Covering(0x8B5CF6u)}};

} // namespace

const ThemeDeclaration &ThemeSpecification::Theme(ThemeSubject Subject)
{
    const std::uint32_t Ordinal = static_cast<std::uint32_t>(Subject);
    return Themes[(Ordinal < static_cast<std::uint32_t>(ThemeSubject::SubjectCount)) ? Ordinal : 0u];
}

const AccentDeclaration &ThemeSpecification::Accent(AccentSubject Subject)
{
    const std::uint32_t Ordinal = static_cast<std::uint32_t>(Subject);
    return Accents[(Ordinal < static_cast<std::uint32_t>(AccentSubject::SubjectCount)) ? Ordinal : 0u];
}

} // namespace Slate
