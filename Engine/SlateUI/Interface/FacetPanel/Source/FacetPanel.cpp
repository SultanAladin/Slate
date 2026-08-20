//============================================================================================================================================
//                                                            FACETPANEL.CPP
//============================================================================================================================================
// 🧩 Wrapped active-facet chips and shared dropdown selection inside one reusable validation card.

#include "SlateUI/Interface/FacetPanel/Api/FacetPanel.h"

#include <cstdio>

namespace Slate
{

namespace
{

constexpr float CardRadius       = 12.0f;   // [px] - --r-tile
constexpr float CardPad          = 10.0f;   // [px] - chips-region horizontal padding
constexpr float HeaderHeight     = 22.0f;   // [px] - heading and count line
constexpr float HeaderGap        = 8.0f;    // [px] - heading to chips
constexpr float ChipHeight       = 27.0f;   // [px] - chip and add button height
constexpr float ChipGap          = 6.0f;    // [px] - flex gap
constexpr float ChipPadLeading   = 10.0f;   // [px] - caption leading inset
constexpr float ChipSwatch       = 9.0f;    // [px] - classification dot
constexpr float ChipSwatchGap    = 6.0f;    // [px] - dot to caption
constexpr float ChipRemove       = 17.0f;   // [px] - circular remove action
constexpr float ChipRemoveGap    = 6.0f;    // [px] - caption to remove action
constexpr float ChipPadTrailing  = 5.0f;    // [px] - remove action trailing inset
constexpr float DropdownGap      = 10.0f;   // [px] - chips to dropdown
constexpr float CardTrailingPad  = 10.0f;   // [px] - dropdown to card edge
constexpr float CountX       = 24.0f;   // [px] - active count badge floor
constexpr float ClearX       = 48.0f;   // [px] - clear-all action

float Scaled(float Figure, const ThemeProfile& Appearance)
{
    return Figure * Appearance.Measure.DisplayScale;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                       CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> FacetPanel::Construct(MotionIntegrator& IncomingMotion,
                                    RecordingSurface& IncomingSurface,
                                    const ThemeProfile& IncomingAppearance)
{
    if (Motion != nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "a facet panel construction stands" });

    Motion     = &IncomingMotion;
    Surface    = &IncomingSurface;
    Appearance = &IncomingAppearance;

    if (!Interaction.Construct(IncomingMotion).Resolved)
        return Outcome<bool>::Refuse({ RefusalReason::ExtentExhausted, "facet interaction was rejected" });

    if (!SharedControls.Construct(Interaction, IncomingSurface, IncomingAppearance).Resolved)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "shared facet controls were rejected" });

    for (std::uint32_t Ordinal = 0u; Ordinal < FacetCapacity + 2u; ++Ordinal)
    {
        const Outcome<ControlIdentity> Registered = Interaction.Register();
        if (!Registered.Resolved)
            return Outcome<bool>::Refuse(Registered.Error);

        Controls[Ordinal] = Registered.Resolve();
    }

    return Outcome<bool>::Result(true);
}

void FacetPanel::Advance(const PointerCondition& Sampled, double Elapsed)
{
    Pointer = Sampled;
    Interaction.Advance(Sampled, Elapsed);
    SharedControls.Sample(Sampled);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        ARRANGEMENT
//------------------------------------------------------------------------------------------------------------------------

FacetPanel::Arrangement FacetPanel::Arrange(float X,
                                            float Y,
                                            float Width,
                                            const FacetDeclaration& Declared,
                                            const bool* Enabled) const
{
    Arrangement Arranged;
    if (Surface == nullptr || Appearance == nullptr || Width <= 0.0f)
        return Arranged;

    const float Scale = Appearance->Measure.DisplayScale;
    const float Pad = CardPad * Scale;
    const float InteriorX = (Width > Pad * 2.0f) ? Width - Pad * 2.0f : 0.0f;
    const float TextSize = (Appearance->ControlMeasure.RowText > 11.0f * Scale)
                         ? Appearance->ControlMeasure.RowText : 11.0f * Scale;
    const float ChipHeight = ChipHeight * Scale;
    const float Gap = ChipGap * Scale;
    float ChipX = 0.0f;
    float ChipCursorY = 0.0f;
    float ChipsHeight = ChipHeight;
    bool ActivePresent = false;

    const std::uint32_t Count = (Declared.OptionCount < FacetCapacity)
                              ? Declared.OptionCount : FacetCapacity;
    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
    {
        if (Enabled == nullptr || !Enabled[Ordinal])
            continue;

        ActivePresent = true;
        const char* Caption = (Declared.Options != nullptr && Declared.Options[Ordinal] != nullptr)
                            ? Declared.Options[Ordinal] : "";
        const float CaptionX = Surface->MeasureRun(Caption, TextSize);
        const float RequiredX = (ChipPadLeading + ChipSwatch + ChipSwatchGap + ChipRemoveGap +
                                     ChipRemove + ChipPadTrailing) * Scale + CaptionX;
        if (ChipX > 0.0f && ChipX + RequiredX > InteriorX)
        {
            ChipX = 0.0f;
            ChipCursorY += ChipHeight + Gap;
            ChipsHeight += ChipHeight + Gap;
        }

        ChipX += RequiredX + Gap;
    }

    if (!ActivePresent)
        ChipsHeight = ChipHeight;

    const float HeaderHeight = HeaderHeight * Scale;
    const float HeaderToChips = HeaderGap * Scale;
    const float ChipsTop = Y + Pad + HeaderHeight + HeaderToChips;
    const float DropdownTop = ChipsTop + ChipsHeight + DropdownGap * Scale;
    const float DropdownHeight = Appearance->ControlMeasure.FieldHeight;

    Arranged.Header = Spanning(X + Pad, Y + Pad, InteriorX, HeaderHeight);
    Arranged.Chips = Spanning(X + Pad, ChipsTop, InteriorX, ChipsHeight);
    Arranged.Dropdown = Spanning(X + Pad, DropdownTop, InteriorX, DropdownHeight);
    Arranged.TotalY = Pad + HeaderHeight + HeaderToChips + ChipsHeight + DropdownGap * Scale +
                           DropdownHeight + CardTrailingPad * Scale;
    return Arranged;
}

float FacetPanel::MeasureHeight(float Width,
                                const FacetDeclaration& Declared,
                                const bool* Enabled) const
{
    return Arrange(0.0f, 0.0f, Width, Declared, Enabled).TotalY;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        INTERACTION
//------------------------------------------------------------------------------------------------------------------------

bool FacetPanel::Pressed(std::uint32_t Ordinal, const PlaneExtent& Extent)
{
    if (Ordinal >= FacetCapacity + 2u)
        return false;

    const ControlIdentity Target = Controls[Ordinal];
    const bool Hovered = Extent.Encloses(Pointer.PositionX, Pointer.PositionY);
    if (Hovered && Pointer.ContactPressed && !Interaction.AnyDisclosed())
        Interaction.Grab(Target, ControlPart::Body);

    Interaction.DeclareHovered(Target, Hovered, 130.0);
    return Hovered && Interaction.Released(Target);
}

ThemeToken FacetPanel::FacetColour(const FacetDeclaration& Declared, std::uint32_t Ordinal) const
{
    if (Declared.Colours != nullptr && Ordinal < Declared.OptionCount)
        return Declared.Colours[Ordinal];

    return Appearance != nullptr ? Appearance->Control.StopTaken : Covering(0xE8E8E8u);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                         RECORDING
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> FacetPanel::Record(const PlaneExtent& Extent,
                                 const FacetDeclaration& Declared,
                                 bool* Enabled)
{
    if (Surface == nullptr || Appearance == nullptr || Motion == nullptr)
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent, "no facet panel construction stands" });

    const std::uint32_t Count = (Declared.OptionCount < FacetCapacity)
                              ? Declared.OptionCount : FacetCapacity;
    const Arrangement Arranged = Arrange(Extent.MinimumX, Extent.MinimumY,
                                         Extent.Width(), Declared, Enabled);
    const ControlColour& Colour = Appearance->Control;
    const float Scale = Appearance->Measure.DisplayScale;
    const float Radius = CardRadius * Scale;
    const float TextSize = (Appearance->ControlMeasure.RowText > 11.0f * Scale)
                         ? Appearance->ControlMeasure.RowText : 11.0f * Scale;

    Surface->Ground(Extent, Colour.CardGround, Radius, CornerAll);
    Surface->Edge(Extent, Colour.CardEdge, Appearance->ControlMeasure.CardEdgeWeight, Radius, CornerAll);

    std::uint32_t ActiveCount = 0u;
    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
        if (Enabled != nullptr && Enabled[Ordinal]) ++ActiveCount;

    Surface->TextRunCapitalised(Arranged.Header.MinimumX,
                                Arranged.Header.MinimumY + 5.0f * Scale,
                                Colour.LabelQuiet,
                                Declared.Caption,
                                Appearance->ControlMeasure.LabelText,
                                0.08f,
                                false);

    char CountRun[12] = {};
    std::snprintf(CountRun, sizeof(CountRun), "%u", static_cast<unsigned>(ActiveCount));
    const PlaneExtent CountBadge = Spanning(Arranged.Header.MinimumX +
                                               Surface->MeasureRun(Declared.Caption,
                                                                   Appearance->ControlMeasure.LabelText,
                                                                   0.08f) + 10.0f * Scale,
                                           Arranged.Header.MinimumY + 2.0f * Scale,
                                           CountX * Scale,
                                           18.0f * Scale);
    Surface->Ground(CountBadge, Colour.StopTaken, 9.0f * Scale, CornerAll);
    Surface->TextRun(CountBadge.MinimumX + CountBadge.Width() * 0.5f,
                     CountBadge.MinimumY + 4.0f * Scale,
                     Colour.StopTakenColour,
                     CountRun,
                     Appearance->ControlMeasure.LabelText,
                     0.0f,
                     true);

    const PlaneExtent Clear = Spanning(Arranged.Header.MaximumX - ClearX * Scale,
                                       Arranged.Header.MinimumY,
                                       ClearX * Scale,
                                       Arranged.Header.Height());
    if (ActiveCount > ((Declared.LockedOrdinal < Count && Enabled != nullptr &&
                        Enabled[Declared.LockedOrdinal]) ? 1u : 0u))
    {
        Surface->TextRun(Clear.MinimumX,
                         Clear.MinimumY + 5.0f * Scale,
                         Colour.LabelQuiet,
                         "Clear all",
                         Appearance->ControlMeasure.LabelText,
                         0.0f,
                         false);
        if (Pressed(1u, Clear) && Enabled != nullptr)
        {
            for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
                Enabled[Ordinal] = Ordinal == Declared.LockedOrdinal;
        }
    }

    const float ChipHeight = ChipHeight * Scale;
    const float Gap = ChipGap * Scale;
    float CursorX = Arranged.Chips.MinimumX;
    float CursorY = Arranged.Chips.MinimumY;
    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
    {
        if (Enabled == nullptr || !Enabled[Ordinal])
            continue;

        const char* Caption = (Declared.Options != nullptr && Declared.Options[Ordinal] != nullptr)
                            ? Declared.Options[Ordinal] : "";
        const float CaptionX = Surface->MeasureRun(Caption, TextSize);
        const float RequiredX = (ChipPadLeading + ChipSwatch + ChipSwatchGap + ChipRemoveGap +
                                     ChipRemove + ChipPadTrailing) * Scale + CaptionX;
        if (CursorX > Arranged.Chips.MinimumX && CursorX + RequiredX > Arranged.Chips.MaximumX)
        {
            CursorX = Arranged.Chips.MinimumX;
            CursorY += ChipHeight + Gap;
        }

        const PlaneExtent Chip = Spanning(CursorX, CursorY, RequiredX, ChipHeight);
        Surface->Ground(Chip, Colour.FieldGround, ChipHeight * 0.5f, CornerAll);
        Surface->Edge(Chip, Colour.CardEdge, Appearance->ControlMeasure.CardEdgeWeight,
                      ChipHeight * 0.5f, CornerAll);
        Surface->Medallion(Chip.MinimumX + ChipPadLeading * Scale + ChipSwatch * Scale * 0.5f,
                           Chip.MinimumY + ChipHeight * 0.5f,
                           ChipSwatch * Scale * 0.5f,
                           FacetColour(Declared, Ordinal));
        const float CaptionTop = Chip.MinimumX + (ChipPadLeading + ChipSwatch + ChipSwatchGap) * Scale;
        Surface->TextRun(CaptionTop,
                         Chip.MinimumY + (ChipHeight - TextSize) * 0.5f,
                         Colour.FieldColour,
                         Caption,
                         TextSize,
                         0.0f,
                         false);

        const PlaneExtent Remove = Spanning(Chip.MaximumX - (ChipRemove + ChipPadTrailing) * Scale,
                                            Chip.MinimumY + (ChipHeight - ChipRemove * Scale) * 0.5f,
                                            ChipRemove * Scale,
                                            ChipRemove * Scale);
        Surface->Ground(Remove, Colour.CellGround, Remove.Height() * 0.5f, CornerAll);
        Surface->Stroke(SymbolSubject::PlaceholderMark, Spanning(Remove.MinimumX + 4.0f * Scale,
                                                       Remove.MinimumY + 4.0f * Scale,
                                                       Remove.Width() - 8.0f * Scale,
                                                       Remove.Height() - 8.0f * Scale),
                        Colour.CellColour);
        if (Ordinal != Declared.LockedOrdinal && Pressed(Ordinal + 2u, Remove))
            Enabled[Ordinal] = false;

        CursorX = Chip.MaximumX + Gap;
    }

    AvailableCount = 1u;
    AvailableOptions[0] = "Choose filter...";
    AvailableOrdinals[0] = AbsentFacet;
    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
    {
        if (Enabled != nullptr && Enabled[Ordinal])
            continue;

        AvailableOptions[AvailableCount] = Declared.Options != nullptr ? Declared.Options[Ordinal] : "";
        AvailableOrdinals[AvailableCount] = Ordinal;
        ++AvailableCount;
    }

    SelectionDeclaration Dropdown;
    Dropdown.Caption     = (AvailableCount > 1u) ? "Add filter" : "Filters";
    Dropdown.Options     = AvailableOptions;
    Dropdown.OptionCount = AvailableCount;
    if (PendingSelection >= AvailableCount)
        PendingSelection = 0u;

    const ControlVerdict Selected = SharedControls.SelectionField(Controls[0], Arranged.Dropdown,
                                                                  Dropdown, PendingSelection);
    if (Selected.ReadingAltered && PendingSelection > 0u && PendingSelection < AvailableCount && Enabled != nullptr)
    {
        const std::uint32_t FacetOrdinal = AvailableOrdinals[PendingSelection];
        if (FacetOrdinal < Count)
            Enabled[FacetOrdinal] = true;
        PendingSelection = 0u;
    }

    return Outcome<bool>::Result(true);
}

void FacetPanel::RecordDeferred()
{
    SharedControls.RecordDeferred();
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

void FacetPanel::Reset()
{
    SharedControls.Reset();
    Interaction.Reset();
    Motion           = nullptr;
    Surface          = nullptr;
    Appearance       = nullptr;
    Pointer          = {};
    AvailableCount   = 0u;
    PendingSelection = 0u;

    for (std::uint32_t Ordinal = 0u; Ordinal < FacetCapacity + 2u; ++Ordinal)
        Controls[Ordinal] = {};
    for (std::uint32_t Ordinal = 0u; Ordinal < FacetCapacity + 1u; ++Ordinal)
    {
        AvailableOptions[Ordinal] = nullptr;
        AvailableOrdinals[Ordinal] = AbsentFacet;
    }
}

}   // namespace Slate
