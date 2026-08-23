//============================================================================================================================================
//                                                        SURFACELAYERSEQUENCE.CPP
//============================================================================================================================================
// 🧩 Ordering by position alone, amendments bounded by what they touched, and the one resampling that is reported.

#include "SlateDocument/Document/SurfaceLayerSequence/Api/SurfaceLayerSequence.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                      LOCATION
//------------------------------------------------------------------------------------------------------------------------

std::size_t SurfaceLayerSequence::Located(LayerIdentity Subject) const
{
    if (!Subject.IdentityDeclared())
        return Sequenced.size();

    for (std::size_t Index = 0u; Index < Sequenced.size(); ++Index)
    {
        if (Sequenced[Index].Identity == Subject)
            return Index;
    }

    return Sequenced.size();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     DECLARATION
//------------------------------------------------------------------------------------------------------------------------

Outcome<LayerIdentity> SurfaceLayerSequence::Append(const LayerSpecification& Declaring)
{
    if (Sequenced.size() >= EntryLimit)
        return Outcome<LayerIdentity>::Refuse({ RefusalReason::ExtentExhausted, "the entry ceiling was reached" });

    if (Declaring.Source == LayerContentSource::NestedSequence
     && Declaring.NestedIndex >= NestedSequences.size())
    {
        return Outcome<LayerIdentity>::Refuse({ RefusalReason::ContentUnsupported, "no such nested sequence" });
    }

    // 📝 Painted content is validated against its own declared extent here rather than trusted. A span that does
    //    not match is a span every later sample reads past the end of, and the read is not detectable afterwards.
    if (Declaring.Source == LayerContentSource::PaintedImpressions)
    {
        const std::size_t Required = static_cast<std::size_t>(Declaring.Painted.ExtentTexels)
                                   * static_cast<std::size_t>(Declaring.Painted.ExtentTexels)
                                   * static_cast<std::size_t>(Declaring.Painted.ComponentCount);

        if (Declaring.Painted.ExtentTexels == 0u || Declaring.Painted.Texels.size() != Required)
        {
            return Outcome<LayerIdentity>::Refuse(
                { RefusalReason::ContentUnsupported, "the painted span does not match its declared extent" });
        }

        if (Declaring.Painted.ExtentTexels > MaximumWorkingEdge)
        {
            return Outcome<LayerIdentity>::Refuse(
                { RefusalReason::ExtentExhausted, "the working extent exceeds the declared maximum" });
        }
    }

    LayerSpecification Incoming = Declaring;

    Incoming.Identity.SlotIndex    = static_cast<std::uint32_t>(Sequenced.size());
    Incoming.Identity.SlotGeneration = RegisteredGeneration;

    Sequenced.push_back(Incoming);

    return Outcome<LayerIdentity>::Result(Incoming.Identity);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     AMENDMENTS
//------------------------------------------------------------------------------------------------------------------------

Outcome<std::uint32_t> SurfaceLayerSequence::Reorder(LayerIdentity Subject, std::uint32_t Position)
{
    const std::size_t Located_ = Located(Subject);

    if (Located_ == Sequenced.size())
        return Outcome<std::uint32_t>::Refuse({ RefusalReason::IdentityStale, "the entry no longer resolves" });

    const std::uint32_t Prior   = static_cast<std::uint32_t>(Located_);
    const LayerSpecification Held = Sequenced[Located_];

    Sequenced.erase(Sequenced.begin() + static_cast<std::ptrdiff_t>(Located_));

    const std::size_t Incoming = Position >= Sequenced.size() ? Sequenced.size()
                                                              : static_cast<std::size_t>(Position);

    Sequenced.insert(Sequenced.begin() + static_cast<std::ptrdiff_t>(Incoming), Held);

    return Outcome<std::uint32_t>::Result(Prior);
}

Outcome<bool> SurfaceLayerSequence::DeclarePresence(LayerIdentity Subject, bool PresenceEnabled)
{
    const std::size_t Located_ = Located(Subject);

    if (Located_ == Sequenced.size())
        return Outcome<bool>::Refuse({ RefusalReason::IdentityStale, "the entry no longer resolves" });

    const bool Prior = Sequenced[Located_].PresenceEnabled;

    Sequenced[Located_].PresenceEnabled = PresenceEnabled;

    return Outcome<bool>::Result(Prior);
}

Outcome<CombineSpecification> SurfaceLayerSequence::DeclareCombination(LayerIdentity        Subject,
                                                                       CombineSpecification Declaring)
{
    const std::size_t Located_ = Located(Subject);

    if (Located_ == Sequenced.size())
    {
        return Outcome<CombineSpecification>::Refuse(
            { RefusalReason::IdentityStale, "the entry no longer resolves" });
    }

    if (Declaring == CombineSpecification::CombineCount)
    {
        return Outcome<CombineSpecification>::Refuse(
            { RefusalReason::ContentUnsupported, "no such combination" });
    }

    const CombineSpecification Prior = Sequenced[Located_].Combination;

    Sequenced[Located_].Combination = Declaring;

    return Outcome<CombineSpecification>::Result(Prior);
}

Outcome<LayerSpecification> SurfaceLayerSequence::Withdraw(LayerIdentity Subject)
{
    const std::size_t Located_ = Located(Subject);

    if (Located_ == Sequenced.size())
    {
        return Outcome<LayerSpecification>::Refuse(
            { RefusalReason::IdentityStale, "the entry no longer resolves" });
    }

    LayerSpecification Departing = Sequenced[Located_];

    // 🔴 A reconstructible entry surrenders its resolved texels and keeps its description — `56` §6. Nothing in
    //    the inverse is a derivation, which is what keeps the inverse bounded by what the amendment touched.
    if (SourceReconstructible(Departing.Source))
    {
        Departing.Painted.Texels.clear();
        Departing.Coverage.Painted.Texels.clear();
    }

    Sequenced.erase(Sequenced.begin() + static_cast<std::ptrdiff_t>(Located_));

    // 📝 The generation advances on withdrawal, so an identity the caller still holds resolves to absent rather
    //    than to whichever entry later occupies the position — `10` §2.1's scheme, unchanged.
    ++RegisteredGeneration;

    return Outcome<LayerSpecification>::Result(Departing);
}

Outcome<std::uint32_t> SurfaceLayerSequence::Nest()
{
    if (Depth + 1u > LayerNestingLimit)
    {
        return Outcome<std::uint32_t>::Refuse(
            { RefusalReason::ExtentExhausted, "the declared nesting ceiling was reached" });
    }

    const std::uint32_t NestedIndex = static_cast<std::uint32_t>(NestedSequences.size());

    NestedSequences.push_back(SurfaceLayerSequence{});
    NestedSequences.back().Depth = Depth + 1u;

    return Outcome<std::uint32_t>::Result(NestedIndex);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE RESAMPLING
//------------------------------------------------------------------------------------------------------------------------

namespace
{

// 📐 Bilinear over the interleaved span, clamped at the edges. Nearest would preserve every texel exactly and
//    would also shear every diagonal the artist painted; bilinear softens uniformly, which is the failure mode
//    that reads as a resampling rather than as a defect.
float SampleBilinear(const PaintedContent& Held,
                     double                PositionX,
                     double                PositionY,
                     std::uint32_t         Component)
{
    const double Extent = static_cast<double>(Held.ExtentTexels);

    double XTexel  = PositionX  * Extent - 0.5;
    double YTexel = PositionY * Extent - 0.5;

    XTexel  = XTexel  < 0.0 ? 0.0 : (XTexel  > Extent - 1.0 ? Extent - 1.0 : XTexel);
    YTexel = YTexel < 0.0 ? 0.0 : (YTexel > Extent - 1.0 ? Extent - 1.0 : YTexel);

    const std::uint32_t MinimumX  = static_cast<std::uint32_t>(XTexel);
    const std::uint32_t MinimumY = static_cast<std::uint32_t>(YTexel);

    const std::uint32_t NextX  = MinimumX  + 1u < Held.ExtentTexels ? MinimumX  + 1u : MinimumX;
    const std::uint32_t NextY = MinimumY + 1u < Held.ExtentTexels ? MinimumY + 1u : MinimumY;

    const double FractionX  = XTexel  - static_cast<double>(MinimumX);
    const double FractionY = YTexel - static_cast<double>(MinimumY);

    const std::size_t Stride = static_cast<std::size_t>(Held.ComponentCount);

    const std::size_t LowerLeft  = (static_cast<std::size_t>(MinimumY) * Held.ExtentTexels + MinimumX) * Stride;
    const std::size_t LowerRight = (static_cast<std::size_t>(MinimumY) * Held.ExtentTexels + NextX)  * Stride;
    const std::size_t UpperLeft  = (static_cast<std::size_t>(NextY)  * Held.ExtentTexels + MinimumX) * Stride;
    const std::size_t UpperRight = (static_cast<std::size_t>(NextY)  * Held.ExtentTexels + NextX)  * Stride;

    const double Lower = static_cast<double>(Held.Texels[LowerLeft + Component])  * (1.0 - FractionX)
                       + static_cast<double>(Held.Texels[LowerRight + Component]) * FractionX;

    const double Upper = static_cast<double>(Held.Texels[UpperLeft + Component])  * (1.0 - FractionX)
                       + static_cast<double>(Held.Texels[UpperRight + Component]) * FractionX;

    return static_cast<float>(Lower * (1.0 - FractionY) + Upper * FractionY);
}

void ResampleContent(PaintedContent&                                               Held,
                     const std::function<bool(double, double, double&, double&)>&  Remapping)
{
    if (Held.ExtentTexels == 0u || Held.Texels.empty())
        return;

    std::vector<float> Incoming(Held.Texels.size(), 0.0f);

    const double Extent = static_cast<double>(Held.ExtentTexels);

    for (std::uint32_t Y = 0u; Y < Held.ExtentTexels; ++Y)
    {
        for (std::uint32_t X = 0u; X < Held.ExtentTexels; ++X)
        {
            const double PositionX  = (static_cast<double>(X)  + 0.5) / Extent;
            const double PositionY = (static_cast<double>(Y) + 0.5) / Extent;

            double FormerX  = 0.0;
            double FormerY = 0.0;

            // 📝 A position the remapping cannot answer occupied no chart in the former domain, so it is left at
            //    zero rather than filled from the nearest thing. A fabricated value here is content the artist
            //    never painted, appearing exactly where a chart boundary moved.
            if (!Remapping(PositionX, PositionY, FormerX, FormerY))
                continue;

            const std::size_t Writing = (static_cast<std::size_t>(Y) * Held.ExtentTexels + X)
                                      * static_cast<std::size_t>(Held.ComponentCount);

            for (std::uint32_t Component = 0u; Component < Held.ComponentCount; ++Component)
                Incoming[Writing + Component] = SampleBilinear(Held, FormerX, FormerY, Component);
        }
    }

    Held.Texels.swap(Incoming);
}

}   // namespace

Outcome<bool> SurfaceLayerSequence::Resample(
    std::uint64_t                                                 IncomingRevision,
    const std::function<bool(double, double, double&, double&)>&  Remapping,
    ReportSequence&                                               Reporting,
    TickPoint                                                     Sampled)
{
    if (!Remapping)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "no domain remapping was supplied" });

    if (IncomingRevision == DescribedRevision)
        return Outcome<bool>::Result(true);

    std::uint32_t ResampledCount = 0u;

    for (LayerSpecification& Held : Sequenced)
    {
        if (Held.Source == LayerContentSource::PaintedImpressions)
        {
            ResampleContent(Held.Painted, Remapping);
            Held.ResampleOwed = false;
            ++ResampledCount;
        }

        if (Held.Coverage.CoverageDeclared
         && Held.Coverage.Source == LayerContentSource::PaintedImpressions)
        {
            ResampleContent(Held.Coverage.Painted, Remapping);
            ++ResampledCount;
        }
    }

    for (SurfaceLayerSequence& Nesting : NestedSequences)
    {
        const Outcome<bool> Nested = Nesting.Resample(IncomingRevision, Remapping, Reporting, Sampled);

        if (!Nested.Resolved)
            return Nested;
    }

    DescribedRevision = IncomingRevision;

    // 🔴 `86` §4's `56` §3.1 row, and the register's most consequential entry. It is the one operation in the
    //    engine that resamples authored content, and presenting it at the same weight as a residency total is a
    //    line the artist scrolls past before discovering their paint softened.
    if (ResampledCount != 0u)
    {
        ReportSpecification Amended;
        Amended.Origin         = "56 §3.1 SurfaceLayerSequence";
        Amended.Subject        = "PaintedResampling";
        Amended.Detail         = "a re-partition moved the domain; painted texels were resampled into it";
        Amended.SubjectIndex = IncomingRevision;
        Amended.Verdict    = ReportVerdict::Amended;
        Amended.Arrival        = Sampled;

        Reporting.Append(Amended);
    }

    return Outcome<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

Outcome<const LayerSpecification*> SurfaceLayerSequence::Resolve(LayerIdentity Subject) const
{
    const std::size_t Located_ = Located(Subject);

    if (Located_ == Sequenced.size())
    {
        return Outcome<const LayerSpecification*>::Refuse(
            { RefusalReason::IdentityStale, "the entry no longer resolves" });
    }

    return Outcome<const LayerSpecification*>::Result(&Sequenced[Located_]);
}

Outcome<PaintedContent*> SurfaceLayerSequence::AmendPainted(LayerIdentity Subject)
{
    const std::size_t Located_ = Located(Subject);

    if (Located_ == Sequenced.size())
    {
        return Outcome<PaintedContent*>::Refuse(
            { RefusalReason::IdentityStale, "the entry no longer resolves" });
    }

    // 🔴 §3's classification, enforced at the one door into authored content. A reconstructible entry stores a
    //    description and `70` resolves it; texels written into one would be a second place its content lived,
    //    and the two would diverge at the first re-resolution.
    if (SourceReconstructible(Sequenced[Located_].Source))
    {
        return Outcome<PaintedContent*>::Refuse(
            { RefusalReason::ContentUnsupported, "the entry stores a description, not texels" });
    }

    return Outcome<PaintedContent*>::Result(&Sequenced[Located_].Painted);
}

const std::vector<LayerSpecification>& SurfaceLayerSequence::Entries() const
{
    return Sequenced;
}

Outcome<const SurfaceLayerSequence*> SurfaceLayerSequence::Nested(std::uint32_t NestedIndex) const
{
    if (NestedIndex >= NestedSequences.size())
    {
        return Outcome<const SurfaceLayerSequence*>::Refuse(
            { RefusalReason::ContentUnsupported, "no such nested sequence" });
    }

    return Outcome<const SurfaceLayerSequence*>::Result(&NestedSequences[NestedIndex]);
}

Outcome<SurfaceLayerSequence*> SurfaceLayerSequence::AmendNested(std::uint32_t NestedIndex)
{
    if (NestedIndex >= NestedSequences.size())
    {
        return Outcome<SurfaceLayerSequence*>::Refuse(
            { RefusalReason::ContentUnsupported, "no such nested sequence" });
    }

    return Outcome<SurfaceLayerSequence*>::Result(&NestedSequences[NestedIndex]);
}

Outcome<std::uint32_t> SurfaceLayerSequence::PositionOf(LayerIdentity Subject) const
{
    const std::size_t Located_ = Located(Subject);

    if (Located_ == Sequenced.size())
        return Outcome<std::uint32_t>::Refuse({ RefusalReason::IdentityStale, "the entry no longer resolves" });

    return Outcome<std::uint32_t>::Result(static_cast<std::uint32_t>(Located_));
}

std::uint32_t SurfaceLayerSequence::WrittenChannels() const
{
    std::uint32_t Written = 0u;

    for (const LayerSpecification& Held : Sequenced)
    {
        if (!Held.PresenceEnabled)
            continue;

        // 📝 §4.1: a nested entry writes the union of what its own entries write, restricted by its own set.
        if (Held.Source == LayerContentSource::NestedSequence && Held.NestedIndex < NestedSequences.size())
            Written |= NestedSequences[Held.NestedIndex].WrittenChannels() & Held.ChannelMask;
        else
            Written |= Held.ChannelMask;
    }

    return Written;
}

bool SurfaceLayerSequence::AuthoredContentHeld() const
{
    for (const LayerSpecification& Held : Sequenced)
    {
        if (!SourceReconstructible(Held.Source))
            return true;

        if (Held.Coverage.CoverageDeclared
         && !SourceReconstructible(Held.Coverage.Source))
        {
            return true;
        }
    }

    for (const SurfaceLayerSequence& Nesting : NestedSequences)
    {
        if (Nesting.AuthoredContentHeld())
            return true;
    }

    return false;
}

std::uint32_t SurfaceLayerSequence::NestingDepth() const      { return Depth;             }
std::uint64_t SurfaceLayerSequence::AddressedRevision() const { return DescribedRevision; }

std::uint32_t SurfaceLayerSequence::EntryCount() const
{
    return static_cast<std::uint32_t>(Sequenced.size());
}

std::uint32_t SurfaceLayerSequence::NestedCount() const
{
    return static_cast<std::uint32_t>(NestedSequences.size());
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE ENTRIES
//------------------------------------------------------------------------------------------------------------------------

Outcome<const LayerSpecification*> LayerIndex::Locate(const SurfaceLayerSequence& Sequence, LayerIdentity Subject)
{
    const Outcome<const LayerSpecification*> Held = Sequence.Resolve(Subject);

    if (Held.Resolved)
        return Held;

    for (std::uint32_t Index = 0u; Index < Sequence.NestedCount(); ++Index)
    {
        const Outcome<const SurfaceLayerSequence*> Nesting = Sequence.Nested(Index);

        if (!Nesting.Resolved)
            continue;

        const Outcome<const LayerSpecification*> Deeper = Locate(*Nesting.Resolve(), Subject);

        if (Deeper.Resolved)
            return Deeper;
    }

    return Outcome<const LayerSpecification*>::Refuse(
        { RefusalReason::IdentityStale, "nothing in the nesting holds that entry" });
}

std::uint32_t LayerIndex::SpannedCount(const SurfaceLayerSequence& Sequence)
{
    std::uint32_t Spanned = Sequence.EntryCount();

    for (std::uint32_t Index = 0u; Index < Sequence.NestedCount(); ++Index)
    {
        const Outcome<const SurfaceLayerSequence*> Nesting = Sequence.Nested(Index);

        if (Nesting.Resolved)
            Spanned += SpannedCount(*Nesting.Resolve());
    }

    return Spanned;
}

}   // namespace Slate
