//============================================================================================================================================
//                                                        PROPERTYSPECIFICATION.CPP
//============================================================================================================================================
// 🧩 The validation each measure declares, the bounding offered beside it, and the write that refuses.

#include "SlateDocument/Document/PropertySpecification/Api/PropertySpecification.h"

#include <cmath>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE VALIDATION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> Validate(const PropertyDeclaration& Declared, const PropertyValue& Offered)
{
    if (Declared.Measured != Offered.Measured)
    {
        return Outcome<bool>::Refuse(
            { RefusalReason::ContentUnsupported, "the value does not measure what the property declares" });
    }

    switch (Declared.Measured)
    {
        case PropertyMeasure::Truth:
            return Outcome<bool>::Result(true);

        case PropertyMeasure::Ordinal:
        {
            if (Declared.UpperOrdinal != 0u && Offered.OrdinalHeld > Declared.UpperOrdinal)
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the ordinal exceeds its ceiling" });

            return Outcome<bool>::Result(true);
        }

        case PropertyMeasure::Signed:
        {
            if (!Declared.BoundsDeclared)
                return Outcome<bool>::Result(true);

            if (Offered.SignedHeld < Declared.LowerSigned)
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "below the declared lower bound" });

            if (Offered.SignedHeld > Declared.UpperSigned)
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "above the declared upper bound" });

            return Outcome<bool>::Result(true);
        }

        case PropertyMeasure::Magnitude:
        {
            // 🔴 A magnitude that is not a number is refused whether or not bounds are declared. `02` §2 admits
            //    unbounded emission but never an unrepresentable value, and one written here propagates through
            //    every integration that reads it without ever naming its origin.
            if (std::isnan(Offered.MagnitudeHeld))
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the magnitude is not a number" });

            if (!Declared.BoundsDeclared)
                return Outcome<bool>::Result(true);

            if (Offered.MagnitudeHeld < Declared.LowerMagnitude)
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "below the declared interval" });

            if (Offered.MagnitudeHeld > Declared.UpperMagnitude)
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "above the declared interval" });

            return Outcome<bool>::Result(true);
        }

        case PropertyMeasure::Text:
        {
            if (Declared.TextExtent != 0u && Offered.TextHeld.size() > Declared.TextExtent)
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the text exceeds its extent" });

            return Outcome<bool>::Result(true);
        }

        case PropertyMeasure::Colour:
        {
            // 🔴 `36` §1: a colour without its space is refused rather than assumed to be in the working space.
            //    An assumed space is the defect `36` exists to prevent, and assuming it here would place the
            //    assumption where no report can name it.
            if (!Offered.ColourHeld.ColourDeclared())
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the colour declares no space" });

            if (Declared.RequiredSpace != 0u && Offered.ColourHeld.SpaceIdentity != Declared.RequiredSpace)
            {
                return Outcome<bool>::Refuse(
                    { RefusalReason::ContentUnsupported, "the colour is not in the space the property requires" });
            }

            return Outcome<bool>::Result(true);
        }

        case PropertyMeasure::Enrolment:
        {
            if (Offered.OrdinalHeld >= Declared.EnrolledOptions.size())
                return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such declared option" });

            return Outcome<bool>::Result(true);
        }

        case PropertyMeasure::Occupant:
        {
            if (!Offered.OccupantHeld.IdentityDeclared())
            {
                return Outcome<bool>::Refuse(
                    { RefusalReason::IdentityStale, "an undeclared identity names no occupant" });
            }

            // 📝 Resolution against the population is the caller's, not this unit's. A property index that held
            //    a `PopulationIndex` would make every property declaration depend on the population that
            //    happens to be open, and `10` §2.1's generational compare already refuses a stale reference
            //    wherever it is resolved.
            return Outcome<bool>::Result(true);
        }

        case PropertyMeasure::MeasureCount:
            break;
    }

    return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the declared measure has no validation" });
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE BOUNDING
//------------------------------------------------------------------------------------------------------------------------

Outcome<PropertyValue> Bounded(const PropertyDeclaration& Declared, const PropertyValue& Offered)
{
    if (Declared.Measured != Offered.Measured)
    {
        return Outcome<PropertyValue>::Refuse(
            { RefusalReason::ContentUnsupported, "no bounding reconciles two different measures" });
    }

    PropertyValue Bounding = Offered;

    if (Declared.Measured == PropertyMeasure::Magnitude && Declared.BoundsDeclared)
    {
        if (std::isnan(Bounding.MagnitudeHeld))
        {
            return Outcome<PropertyValue>::Refuse(
                { RefusalReason::ContentUnsupported, "the magnitude is not a number" });
        }

        if (Bounding.MagnitudeHeld < Declared.LowerMagnitude)
            Bounding.MagnitudeHeld = Declared.LowerMagnitude;
        else if (Bounding.MagnitudeHeld > Declared.UpperMagnitude)
            Bounding.MagnitudeHeld = Declared.UpperMagnitude;
    }
    else if (Declared.Measured == PropertyMeasure::Signed && Declared.BoundsDeclared)
    {
        if (Bounding.SignedHeld < Declared.LowerSigned)
            Bounding.SignedHeld = Declared.LowerSigned;
        else if (Bounding.SignedHeld > Declared.UpperSigned)
            Bounding.SignedHeld = Declared.UpperSigned;
    }
    else if (Declared.Measured == PropertyMeasure::Ordinal && Declared.UpperOrdinal != 0u)
    {
        if (Bounding.OrdinalHeld > Declared.UpperOrdinal)
            Bounding.OrdinalHeld = Declared.UpperOrdinal;
    }

    return Outcome<PropertyValue>::Result(Bounding);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE DECLARATIONS
//------------------------------------------------------------------------------------------------------------------------

std::size_t PropertyIndex::Located(const std::string& Identity) const
{
    for (std::size_t Ordinal = 0u; Ordinal < DeclaredProperties.size(); ++Ordinal)
    {
        if (DeclaredProperties[Ordinal].Identity == Identity)
            return Ordinal;
    }

    return DeclaredProperties.size();
}

Outcome<bool> PropertyIndex::Declare(const PropertyDeclaration& Declaring)
{
    if (Declaring.Identity.empty())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "a property declares no identity" });

    // 🔴 The declaration's own default is validated against it. A default outside its bounds presents an invalid
    //    value on every occupant that never wrote the property, which is every occupant at the moment it arrives.
    const Outcome<bool> Defaulted = Validate(Declaring, Declaring.Defaulted);

    if (!Defaulted.Resolved)
        return Defaulted;

    const std::size_t Located_ = Located(Declaring.Identity);

    if (Located_ == DeclaredProperties.size())
    {
        DeclaredProperties.push_back(Declaring);
        HeldValues.push_back(Declaring.Defaulted);
        ValueDeclared.push_back(false);

        return Outcome<bool>::Result(true);
    }

    // 📝 A redeclaration returns the value to the new default rather than retaining the old one. The former
    //    value was validated against a declaration that no longer stands, and retaining it would hold a value
    //    no current declaration admits.
    DeclaredProperties[Located_] = Declaring;
    HeldValues[Located_]         = Declaring.Defaulted;
    ValueDeclared[Located_]      = false;

    return Outcome<bool>::Result(true);
}

Outcome<bool> PropertyIndex::Write(const std::string& Identity, const PropertyValue& Offered)
{
    const std::size_t Located_ = Located(Identity);

    if (Located_ == DeclaredProperties.size())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "nothing declares that property" });

    const Outcome<bool> Validated = Validate(DeclaredProperties[Located_], Offered);

    if (!Validated.Resolved)
        return Validated;

    HeldValues[Located_]    = Offered;
    ValueDeclared[Located_] = true;

    return Outcome<bool>::Result(true);
}

Outcome<PropertyValue> PropertyIndex::Resolve(const std::string& Identity) const
{
    const std::size_t Located_ = Located(Identity);

    if (Located_ == DeclaredProperties.size())
    {
        return Outcome<PropertyValue>::Refuse(
            { RefusalReason::ContentUnsupported, "nothing declares that property" });
    }

    return Outcome<PropertyValue>::Result(HeldValues[Located_]);
}

Outcome<PropertyDeclaration> PropertyIndex::Declared(const std::string& Identity) const
{
    const std::size_t Located_ = Located(Identity);

    if (Located_ == DeclaredProperties.size())
    {
        return Outcome<PropertyDeclaration>::Refuse(
            { RefusalReason::ContentUnsupported, "nothing declares that property" });
    }

    return Outcome<PropertyDeclaration>::Result(DeclaredProperties[Located_]);
}

const std::vector<PropertyDeclaration>& PropertyIndex::Declarations() const
{
    return DeclaredProperties;
}

bool PropertyIndex::ValueWritten(const std::string& Identity) const
{
    const std::size_t Located_ = Located(Identity);

    return Located_ != DeclaredProperties.size() && ValueDeclared[Located_];
}

Outcome<bool> PropertyIndex::Reclaim(const std::string& Identity)
{
    const std::size_t Located_ = Located(Identity);

    if (Located_ == DeclaredProperties.size())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "nothing declares that property" });

    HeldValues[Located_]    = DeclaredProperties[Located_].Defaulted;
    ValueDeclared[Located_] = false;

    return Outcome<bool>::Result(true);
}

bool PropertyIndex::ValuesValid() const
{
    for (std::size_t Ordinal = 0u; Ordinal < DeclaredProperties.size(); ++Ordinal)
    {
        if (!Validate(DeclaredProperties[Ordinal], HeldValues[Ordinal]).Resolved)
            return false;
    }

    return true;
}

}   // namespace Slate
