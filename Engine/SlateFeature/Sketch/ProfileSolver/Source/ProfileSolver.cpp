//============================================================================================================================================
//                                                        PROFILESOLVER.CPP
//============================================================================================================================================

#include "SlateFeature/Sketch/ProfileSolver/Api/ProfileSolver.h"

namespace Slate
{

namespace
{
    bool ProfileResolvedAgainst(const ProfileSpecification& Declared,
                                std::uint32_t CurveCount)
    {
        if (!Declared.Declared())
            return false;

        for (const ProfileLoop& Loop : Declared.HeldLoops())
            for (const ProfileCurveUse& Use : Loop.Traversal)
                if (!Use.TraversedCurve.Assigned() || Use.TraversedCurve.IssuedIndex > CurveCount)
                    return false;

        return true;
    }
}

ProfileDisposition EvaluateProfiles(const SketchStructure& Declared)
{
    if (Declared.Curves().empty() && Declared.Profiles().empty())
        return ProfileDisposition::NotRequested;
    if (!Declared.Declared())
        return ProfileDisposition::InvalidSketch;
    if (Declared.Profiles().empty())
        return ProfileDisposition::NotRequested;

    for (const ProfileSpecification& Profile : Declared.Profiles())
        if (!ProfileResolvedAgainst(Profile, static_cast<std::uint32_t>(Declared.Curves().size())))
            return ProfileDisposition::InvalidSketch;

    return ProfileDisposition::Produced;
}

Outcome<ResolvedProfileSet> ResolveProfiles(const SketchStructure& Declared)
{
    if (!Declared.Declared())
        return Outcome<ResolvedProfileSet>::Refuse({ RefusalReason::ContentUnsupported, "the sketch is not declared" });
    if (Declared.Profiles().empty())
        return Outcome<ResolvedProfileSet>::Refuse({ RefusalReason::ContentUnsupported, "the sketch declares no profile" });

    ResolvedProfileSet Resolved;
    Resolved.Curves.reserve(Declared.Curves().size());
    for (const DeclaredSketchCurve& Curve : Declared.Curves())
        Resolved.Curves.push_back(Curve.Geometry);

    Resolved.Profiles.reserve(Declared.Profiles().size());
    for (const ProfileSpecification& Profile : Declared.Profiles())
    {
        if (!ProfileResolvedAgainst(Profile, static_cast<std::uint32_t>(Resolved.Curves.size())))
        {
            return Outcome<ResolvedProfileSet>::Refuse(
                { RefusalReason::ContentUnsupported, "the profile names a curve the sketch does not hold" });
        }
        Resolved.Profiles.push_back(Profile);
    }

    return Outcome<ResolvedProfileSet>::Result(Resolved);
}

Outcome<const ProfileSpecification*> ResolveProfile(const SketchStructure& Declared,
                                                    ProfileNameInFeature Profile)
{
    if (!Declared.Declared())
        return Outcome<const ProfileSpecification*>::Refuse({ RefusalReason::ContentUnsupported, "the sketch is not declared" });
    if (!Profile.Assigned() || Profile.IssuedIndex > Declared.Profiles().size())
        return Outcome<const ProfileSpecification*>::Refuse({ RefusalReason::ContentUnsupported, "no such profile is declared" });

    const ProfileSpecification* Held = &Declared.Profiles()[Profile.IssuedIndex - 1u];
    if (!ProfileResolvedAgainst(*Held, static_cast<std::uint32_t>(Declared.Curves().size())))
        return Outcome<const ProfileSpecification*>::Refuse({ RefusalReason::ContentUnsupported, "the profile names an absent curve" });

    return Outcome<const ProfileSpecification*>::Result(Held);
}

} // namespace Slate
