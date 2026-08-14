//============================================================================================================================================
//                                                           PROPERTYPANEL.CPP
//============================================================================================================================================
// 🧩 One row per declaration, dispatched on the declared measure — the panel names no property and never will.

#include "SlateUI/Interface/PropertyPanel/Api/PropertyPanel.h"

#include <cstdio>
#include <cstring>

// 📝 🔴 No vendor spelling appears in this file and none may. Every fill, run of text and clip goes through
//    `ControlPanel`'s painting seam, which is the one component that knows which recording the interface paints on.

namespace Slate
{
namespace
{

constexpr float NoticeBandHeight = 24.0f;   // [px] - the refusal band beneath the header
constexpr float BadgeInset       =  6.0f;   // [px] - inset of a measure badge inside its pill

constexpr std::uint32_t RowTextExtent = 128u;   // [-] - the readout buffer every row prints into

//------------------------------------------------------------------------------------------------------------------------
//                                                      SMALL GEOMETRY
//------------------------------------------------------------------------------------------------------------------------

WorkspaceRectangle BandOf(const WorkspaceRectangle& Area, float Offset, float Height)
{
    return { Area.PositionX, Area.PositionY + Offset, Area.Width, Height };
}

WorkspaceRectangle InsetBy(const WorkspaceRectangle& Area, float Margin)
{
    return { Area.PositionX + Margin,
             Area.PositionY + Margin,
             Area.Width  - Margin * 2.0f > 0.0f ? Area.Width  - Margin * 2.0f : 0.0f,
             Area.Height - Margin * 2.0f > 0.0f ? Area.Height - Margin * 2.0f : 0.0f };
}

WorkspaceRectangle LeftOf(const WorkspaceRectangle& Area, float Width)
{
    return { Area.PositionX, Area.PositionY, Width < Area.Width ? Width : Area.Width, Area.Height };
}

WorkspaceRectangle RightOf(const WorkspaceRectangle& Area, float Width)
{
    const float Taken = Width < Area.Width ? Width : Area.Width;

    return { Area.PositionX + Area.Width - Taken, Area.PositionY, Taken, Area.Height };
}

WorkspaceRectangle SquareCentred(const WorkspaceRectangle& Area, float Edge)
{
    return { Area.PositionX + (Area.Width  - Edge) * 0.5f,
             Area.PositionY + (Area.Height - Edge) * 0.5f,
             Edge,
             Edge };
}

void RecordNotice(PropertyPanelCarry& Carry, const char* Reason)
{
    if (Reason == nullptr || Reason[0] == '\0')
        return;

    std::snprintf(Carry.Notice, PropertyNoticeExtent, "%s", Reason);

    Carry.NoticeDeclared = true;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    WRITING ONE VALUE
//------------------------------------------------------------------------------------------------------------------------

// 🧩 Bounds one offered value against its declaration and then writes it, reporting whichever refused.
// note 🔴 The order is the header's requirement and not a preference. `Write` refuses rather than correcting, and
//      `Bounded` is offered separately precisely so a presenter can bring a drag inside its interval first. A row
//      that wrote first would refuse on every drag that reached an endpoint, and the artist would read that as the
//      slider being broken at exactly the two readings they most often want.
bool WriteBounded(PropertyIndex&              Declarations,
                  const PropertyDeclaration&  Declared,
                  const PropertyValue&        Offered,
                  PropertyPanelCarry&         Carry)
{
    const Outcome<PropertyValue> Brought = Bounded(Declared, Offered);

    if (!Brought.ContentPresent)
    {
        RecordNotice(Carry, Brought.Declined.Detail);
        return false;
    }

    const Outcome<bool> Landed = Declarations.Write(Declared.Identity, Brought.Resolve());

    if (!Landed.ContentPresent)
    {
        RecordNotice(Carry, Landed.Declined.Detail);
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE BANDS
//------------------------------------------------------------------------------------------------------------------------

void PresentHeaderBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       std::uint32_t              DeclaredCount)
{
    PresentSurfaceFill(Band, Theme.Palette.PanelHeader, 0.0f);
    PresentSurfaceFill({ Band.PositionX, Band.PositionY + Band.Height - Theme.Extents.BorderThickness,
                         Band.Width, Theme.Extents.BorderThickness },
                       Theme.Palette.PanelBorder, 0.0f);

    const WorkspaceRectangle Interior = InsetBy(Band, Theme.Extents.PanelPadding);
    const WorkspaceRectangle GlyphBox = LeftOf(Interior, 24.0f);

    PresentSurfaceFill(GlyphBox, Theme.Palette.TileBackground, Theme.Extents.PillRounding);
    PresentControlStroke(SquareCentred(GlyphBox, Theme.Extents.GlyphEdge),
                         ControlStroke::Cog, Theme.Palette.TextPrimary, 1.5f, 0.0f);

    char Counted[RowTextExtent] = {};

    std::snprintf(Counted, RowTextExtent, "%u", DeclaredCount);

    const float              CountWidth = MeasuredTextExtent(Counted, 1.0f) + Theme.Extents.PanelPadding * 2.0f;
    const WorkspaceRectangle CountBadge = RightOf(Interior, CountWidth);

    PresentSurfaceFill(CountBadge, Theme.Palette.TileBackground, Theme.Extents.EntryRounding);
    PresentTextRun(CountBadge, Counted, Theme.Palette.TextMuted, TextPlacement::Centred, 1.0f);

    WorkspaceRectangle Titles = Interior;

    Titles.PositionX += 24.0f + Theme.Extents.ControlSpacing;
    Titles.Width     -= 24.0f + Theme.Extents.ControlSpacing + CountWidth;

    PresentTextRun({ Titles.PositionX, Titles.PositionY, Titles.Width, Titles.Height * 0.55f },
                   "Properties", Theme.Palette.TextPrimary, TextPlacement::Leading, 1.0f);
    PresentTextRun({ Titles.PositionX, Titles.PositionY + Titles.Height * 0.55f,
                     Titles.Width, Titles.Height * 0.45f },
                   "Declared", Theme.Palette.TextMuted, TextPlacement::Leading, 0.85f);
}

void PresentNoticeBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       PropertyPanelCarry&        Carry)
{
    PresentSurfaceFill(Band, Attenuate(Theme.Palette.DangerPrimary, 0.14), Theme.Extents.PillRounding);

    const WorkspaceRectangle Interior = InsetBy(Band, BadgeInset * 0.5f);
    const WorkspaceRectangle Dismiss  = RightOf(Interior, Theme.Extents.GlyphButtonSmallEdge);

    WorkspaceRectangle Lettering = Interior;

    Lettering.Width -= Theme.Extents.GlyphButtonSmallEdge;

    // 📝 🔴 The refusal's own text, verbatim. `10` §2.2 requires it to name which bound was exceeded, and `86` §4
    //    presents that text unaltered for the same reason: a notice reading "invalid" sends the artist to guess.
    PresentTextRun(Lettering, Carry.Notice, Theme.Palette.TextPrimary, TextPlacement::Leading, 0.85f);

    if (ResolveAreaPress(Dismiss).EditSealed)
        Carry.NoticeDeclared = false;

    PresentControlStroke(SquareCentred(Dismiss, Theme.Extents.GlyphEdge), ControlStroke::Cross,
                         Theme.Palette.TextMuted, 1.4f, 0.0f);
}

void PresentFooterBand(const ThemeSpecification&  Theme,
                       const WorkspaceRectangle&  Band,
                       std::uint32_t              DeclaredCount,
                       std::uint32_t              WrittenCount)
{
    PresentSurfaceFill(Band, Theme.Palette.PanelHeader, 0.0f);
    PresentSurfaceFill({ Band.PositionX, Band.PositionY, Band.Width, Theme.Extents.BorderThickness },
                       Theme.Palette.PanelBorder, 0.0f);

    char Counted[RowTextExtent] = {};

    std::snprintf(Counted, RowTextExtent, "%u declared \xC2\xB7 %u written \xC2\xB7 %u defaulted",
                  DeclaredCount, WrittenCount, DeclaredCount - WrittenCount);

    PresentTextRun(InsetBy(Band, Theme.Extents.PanelPadding), Counted,
                   Theme.Palette.TextMuted, TextPlacement::Leading, 0.9f);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                        ONE ROW
//------------------------------------------------------------------------------------------------------------------------

// 🧩 Presents one declaration at whatever measure it declared, and writes through the ledger when it is amended.
// out  the height the row consumed
float PresentPropertyRow(const ThemeSpecification&   Theme,
                         const WorkspaceRectangle&   Row,
                         PropertyIndex&              Declarations,
                         const PropertyDeclaration&  Declared,
                         std::uint32_t               RowOrdinal,
                         PropertyPanelCarry&         Carry)
{
    const Outcome<PropertyValue> Standing = Declarations.Resolve(Declared.Identity);

    if (!Standing.ContentPresent)
    {
        PresentTextRun(Row, Declared.Presented.c_str(), Theme.Palette.TextMuted, TextPlacement::Leading, 1.0f);
        return Theme.Extents.EntryRowHeight;
    }

    // 📝 The caption the artist reads, falling back to the mechanism's own spelling. A declaration with no
    //    presented name is a defect in whoever declared it, and presenting the identity makes that visible rather
    //    than presenting a blank label the artist cannot report.
    const char* const Caption = !Declared.Presented.empty() ? Declared.Presented.c_str()
                                                            : Declared.Identity.c_str();

    const std::size_t Slot = static_cast<std::size_t>(RowOrdinal) < PropertyRowCapacity
                           ? static_cast<std::size_t>(RowOrdinal)
                           : PropertyRowCapacity - 1u;

    PropertyValue Offered = Standing.Resolve();

    ControlInteraction Reported = {};
    bool               Amended  = false;

    switch (Declared.Measured)
    {
    case PropertyMeasure::Truth:
    {
        bool Carried = Offered.TruthDeclared;

        const Outcome<ControlInteraction> Answered = PresentBooleanEntry(Theme, Row, Caption, Carried);

        if (Answered.ContentPresent)
        {
            Reported = Answered.Resolve();

            if (Reported.EditDeclared && Carried != Offered.TruthDeclared)
            {
                Offered.TruthDeclared = Carried;
                Amended               = true;
            }
        }
        break;
    }

    case PropertyMeasure::Magnitude:
    {
        double Carried = Offered.MagnitudeHeld;

        // 📝 A slider where the declaration carries an interval and an accumulating entry where it does not. A
        //    slider over an undeclared interval would have to invent one, and the invented endpoints become the
        //    bounds the artist believes the property has.
        const Outcome<ControlInteraction> Answered =
            Declared.BoundsDeclared
                ? PresentValueSlider(Theme, Row, Caption, Carried,
                                     Declared.LowerMagnitude, Declared.UpperMagnitude, "\xC2\xB7", 3u)
                : PresentScalarEntry(Theme, Row, Caption, Carried, 0.01, "\xC2\xB7", 3u);

        if (!Answered.ContentPresent)
        {
            RecordNotice(Carry, Answered.Declined.Detail);
            break;
        }

        Reported = Answered.Resolve();

        if (Reported.EditDeclared && Carried != Offered.MagnitudeHeld)
        {
            Offered.MagnitudeHeld = Carried;
            Amended               = true;
        }
        break;
    }

    case PropertyMeasure::Signed:
    {
        double Carried = static_cast<double>(Offered.SignedHeld);

        const Outcome<ControlInteraction> Answered =
            PresentScalarEntry(Theme, Row, Caption, Carried, 1.0, "\xC2\xB7", 0u);

        if (!Answered.ContentPresent)
        {
            RecordNotice(Carry, Answered.Declined.Detail);
            break;
        }

        Reported = Answered.Resolve();

        const std::int64_t Rounded = static_cast<std::int64_t>(Carried < 0.0 ? Carried - 0.5 : Carried + 0.5);

        if (Reported.EditDeclared && Rounded != Offered.SignedHeld)
        {
            Offered.SignedHeld = Rounded;
            Amended            = true;
        }
        break;
    }

    case PropertyMeasure::Ordinal:
    {
        double Carried = static_cast<double>(Offered.OrdinalHeld);

        const Outcome<ControlInteraction> Answered =
            PresentScalarEntry(Theme, Row, Caption, Carried, 1.0, "\xC2\xB7", 0u);

        if (!Answered.ContentPresent)
        {
            RecordNotice(Carry, Answered.Declined.Detail);
            break;
        }

        Reported = Answered.Resolve();

        // 📝 Negative travel is floored at nought here rather than wrapping. An unsigned count that wrapped
        //    presents as a colossal number the artist reads as corruption, and `Bounded` cannot undo it because
        //    the wrap has already happened by the time it is offered.
        const std::uint64_t Rounded = Carried <= 0.0 ? 0u : static_cast<std::uint64_t>(Carried + 0.5);

        if (Reported.EditDeclared && Rounded != Offered.OrdinalHeld)
        {
            Offered.OrdinalHeld = Rounded;
            Amended             = true;
        }
        break;
    }

    case PropertyMeasure::Enrolment:
    {
        const std::uint32_t ChoiceCount = static_cast<std::uint32_t>(Declared.EnrolledOptions.size());

        if (ChoiceCount == 0u)
        {
            PresentControlLabel(Theme, Row, Caption);
            break;
        }

        // 📝 The captions are gathered into a fixed run of pointers because the control takes an array and the
        //    declaration holds a vector of strings. Nothing is copied — the pointers name the declaration's own
        //    storage, which outlives this call.
        const char* Choices[16] = {};

        const std::uint32_t Presented = ChoiceCount < 16u ? ChoiceCount : 16u;

        for (std::uint32_t Ordinal = 0u; Ordinal < Presented; ++Ordinal)
            Choices[Ordinal] = Declared.EnrolledOptions[Ordinal].c_str();

        std::uint32_t Carried = static_cast<std::uint32_t>(Offered.OrdinalHeld);

        if (Carried >= Presented)
            Carried = 0u;

        const ControlRowSplit Split = ResolveControlRow(Theme, Row);

        PresentControlLabel(Theme, Split.LabelArea, Caption);

        const Outcome<ControlInteraction> Answered =
            PresentDropdown(Theme, Split.FieldArea, Choices, Presented, Carried,
                            Carry.ChoiceCarries[Slot], Carry.PresentedTicks);

        if (!Answered.ContentPresent)
        {
            RecordNotice(Carry, Answered.Declined.Detail);
            break;
        }

        Reported = Answered.Resolve();

        if (Reported.EditDeclared && static_cast<std::uint64_t>(Carried) != Offered.OrdinalHeld)
        {
            Offered.OrdinalHeld = Carried;
            Amended             = true;
        }
        break;
    }

    case PropertyMeasure::Text:
    {
        TextCarry& Editing = Carry.TextCarries[Slot];

        // 📝 The carry is seeded from the held value only while no edit is open. Seeding during an edit would
        //    overwrite what the artist is typing with what the document still holds, one character at a time.
        if (!Editing.EditOpen)
        {
            std::snprintf(Editing.Carried, ControlTextExtent, "%s", Offered.TextHeld.c_str());
            Editing.CarryExtent = static_cast<std::uint32_t>(std::strlen(Editing.Carried));
        }

        const Outcome<ControlInteraction> Answered =
            PresentTextEntry(Theme, Row, Caption, Editing, "");

        if (!Answered.ContentPresent)
        {
            RecordNotice(Carry, Answered.Declined.Detail);
            break;
        }

        Reported = Answered.Resolve();

        // 📝 🔴 Written on the seal and never on every keystroke. `10` §2.4 makes the whole edit one transaction,
        //    and a write per character would put a revision row on each one — which is exactly the merge failure
        //    `84` §2 describes, arriving through the property panel instead of through the sequence.
        if (Reported.EditSealed && Offered.TextHeld != Editing.Carried)
        {
            Offered.TextHeld = Editing.Carried;
            Amended          = true;
        }
        break;
    }

    case PropertyMeasure::Colour:
    {
        ThemeColour Carried;

        Carried.Coordinate = Offered.ColourHeld;
        Carried.Coverage   = 1.0;

        const Outcome<ControlInteraction> Answered =
            PresentColourEntry(Theme, Row, Caption, Carried, Carry.PickerOpen[Slot]);

        if (!Answered.ContentPresent)
        {
            RecordNotice(Carry, Answered.Declined.Detail);
            break;
        }

        Reported = Answered.Resolve();

        if (Reported.EditDeclared)
        {
            // 📝 🔴 The coordinate keeps its declared space across the edit — `36` §1. `RequiredSpace` is the
            //    declaration's own gate and `Validate` enforces it on the write; the panel does not project into
            //    it, because a projection here would be a transfer spelled inside `SlateUI`.
            Offered.ColourHeld = Carried.Coordinate;
            Amended            = true;
        }
        break;
    }

    case PropertyMeasure::Occupant:
    default:
    {
        // 📝 🔴 Presented as a reading with no editor. Choosing an occupant is a picking interaction `74` owns,
        //    and a control invented here could only offer a raw ordinal the artist cannot map to anything on
        //    screen. Presenting a control that cannot act is worse than presenting none.
        const ControlRowSplit Split = ResolveControlRow(Theme, Row);

        PresentControlLabel(Theme, Split.LabelArea, Caption);

        char Reading[RowTextExtent] = {};

        std::snprintf(Reading, RowTextExtent, "%s \xC2\xB7 read only",
                      CaptionOfPropertyMeasure(Declared.Measured));

        PresentTextRun(Split.FieldArea, Reading, Theme.Palette.TextMuted, TextPlacement::Leading, 0.85f);
        break;
    }
    }

    //--------------------------------------------------------------------------------------------------------------
    // the write, and the lifecycle the caller brackets its transaction with
    //--------------------------------------------------------------------------------------------------------------

    if (Amended)
    {
        Offered.Measured = Declared.Measured;

        if (WriteBounded(Declarations, Declared, Offered, Carry))
        {
            Carry.Reported.ValueWritten = true;
            Carry.Reported.RowOrdinal   = RowOrdinal;
        }
    }

    if (Reported.EditOpened)
    {
        Carry.Reported.EditOpened = true;
        Carry.Reported.RowOrdinal = RowOrdinal;
    }

    if (Reported.EditSealed)
    {
        Carry.Reported.EditSealed = true;
        Carry.Reported.RowOrdinal = RowOrdinal;
    }

    // 📝 A colour row that has its picker down occupies four tracks beneath the bar, so the row is taller for as
    //    long as it is open. Measured here rather than assumed, so the walk below stays in step with the paint.
    if (Declared.Measured == PropertyMeasure::Colour && Carry.PickerOpen[Slot])
        return Theme.Extents.EntryRowHeight
             + Theme.Extents.SwitchHeight * 4.0f + Theme.Extents.ControlSpacing * 3.0f
             + Theme.Extents.PanelPadding * 2.0f + Theme.Extents.ControlSpacing;

    return Theme.Extents.EntryRowHeight;
}

}   // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT A ROW READS
//------------------------------------------------------------------------------------------------------------------------

const char* CaptionOfPropertyMeasure(PropertyMeasure Measured)
{
    switch (Measured)
    {
        case PropertyMeasure::Truth:     return "truth";
        case PropertyMeasure::Ordinal:   return "ordinal";
        case PropertyMeasure::Signed:    return "signed";
        case PropertyMeasure::Magnitude: return "magnitude";
        case PropertyMeasure::Text:      return "text";
        case PropertyMeasure::Colour:    return "colour";
        case PropertyMeasure::Enrolment: return "enrolment";
        case PropertyMeasure::Occupant:  return "occupant";
        default:                         return "unknown";
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE PRESENTATION
//------------------------------------------------------------------------------------------------------------------------

void PresentPropertyPanel(const ThemeSpecification&  Theme,
                          const WorkspaceRectangle&  Area,
                          void*                      PresentContext)
{
    PropertyPanelContext* Standing = static_cast<PropertyPanelContext*>(PresentContext);

    PresentSurfaceFill(Area, Theme.Palette.PanelBackground, Theme.Extents.CornerRounding);

    if (Standing == nullptr || Standing->Declarations == nullptr || Standing->Carry == nullptr)
    {
        PresentTextRun(Area, "No properties", Theme.Palette.TextMuted, TextPlacement::Centred, 1.0f);
        return;
    }

    PropertyIndex&      Declarations = *Standing->Declarations;
    PropertyPanelCarry& Carry        = *Standing->Carry;

    ++Carry.PresentedTicks;

    // 📝 🔴 The report is cleared at the top of the tick. A caller reads it after the present call and brackets
    //    its transaction with it; a report that accumulated across ticks would seal a transaction the artist
    //    opened three drags ago.
    Carry.Reported = {};

    const std::vector<PropertyDeclaration>& Declared = Declarations.Declarations();
    const std::uint32_t                     Counted  = static_cast<std::uint32_t>(Declared.size());

    std::uint32_t WrittenCount = 0u;

    for (const PropertyDeclaration& Entry : Declared)
        WrittenCount += Declarations.ValueWritten(Entry.Identity) ? 1u : 0u;

    //----------------------------------------------------------------------------------------------------------------
    // the bands
    //----------------------------------------------------------------------------------------------------------------

    const float NoticeHeight = Carry.NoticeDeclared ? NoticeBandHeight + Theme.Extents.ControlSpacing : 0.0f;

    const WorkspaceRectangle HeaderBand = BandOf(Area, 0.0f, Theme.Extents.PanelHeaderHeight);
    const WorkspaceRectangle FooterBand = BandOf(Area, Area.Height - Theme.Extents.PanelFooterHeight,
                                                 Theme.Extents.PanelFooterHeight);

    PresentHeaderBand(Theme, HeaderBand, Counted);

    if (Carry.NoticeDeclared)
    {
        PresentNoticeBand(Theme,
                          { Area.PositionX + Theme.Extents.PanelPadding,
                            Area.PositionY + Theme.Extents.PanelHeaderHeight + Theme.Extents.ControlSpacing,
                            Area.Width - Theme.Extents.PanelPadding * 2.0f,
                            NoticeBandHeight },
                          Carry);
    }

    const WorkspaceRectangle ListBand =
        { Area.PositionX,
          Area.PositionY + Theme.Extents.PanelHeaderHeight + NoticeHeight,
          Area.Width,
          Area.Height - Theme.Extents.PanelHeaderHeight - NoticeHeight - Theme.Extents.PanelFooterHeight };

    if (ListBand.Height <= 0.0f || Counted == 0u)
    {
        if (Counted == 0u)
            PresentTextRun(ListBand, "Nothing declared", Theme.Palette.TextMuted, TextPlacement::Centred, 0.9f);

        PresentFooterBand(Theme, FooterBand, Counted, WrittenCount);
        return;
    }

    //----------------------------------------------------------------------------------------------------------------
    // the scrolled run of rows, in declaration order
    //----------------------------------------------------------------------------------------------------------------

    // 📝 The span is measured against the closed height of every row plus whatever the one open picker adds, so a
    //    picker opened at the bottom of a long list can still be scrolled to rather than sitting past the end.
    float ContentSpan = 0.0f;

    for (std::uint32_t Ordinal = 0u; Ordinal < Counted; ++Ordinal)
    {
        const std::size_t Slot = static_cast<std::size_t>(Ordinal) < PropertyRowCapacity
                               ? static_cast<std::size_t>(Ordinal)
                               : PropertyRowCapacity - 1u;

        ContentSpan += Theme.Extents.EntryRowHeight + Theme.Extents.ControlSpacing;

        if (Declared[Ordinal].Measured == PropertyMeasure::Colour && Carry.PickerOpen[Slot])
        {
            ContentSpan += Theme.Extents.SwitchHeight * 4.0f + Theme.Extents.ControlSpacing * 3.0f
                         + Theme.Extents.PanelPadding * 2.0f + Theme.Extents.ControlSpacing;
        }
    }

    AdvanceVisibleOffset(Carry.VisibleOffset, ListBand, ContentSpan);

    DeclareClip(ListBand);

    float Walking = ListBand.PositionY - Carry.VisibleOffset;

    for (std::uint32_t Ordinal = 0u; Ordinal < Counted; ++Ordinal)
    {
        const WorkspaceRectangle Row = { ListBand.PositionX + Theme.Extents.PanelPadding,
                                         Walking,
                                         ListBand.Width - Theme.Extents.PanelPadding * 2.0f,
                                         Theme.Extents.EntryRowHeight };

        // 📝 A row entirely outside the visible band is stepped over rather than presented. `12` §7's measurement
        //    applies here too: the rows touched stay proportional to the panel's height and not to how many
        //    properties a tool declared.
        const bool Visible = Walking + Theme.Extents.EntryRowHeight >= ListBand.PositionY
                          && Walking <= ListBand.PositionY + ListBand.Height;

        if (Visible)
        {
            Walking += PresentPropertyRow(Theme, Row, Declarations, Declared[Ordinal], Ordinal, Carry)
                     + Theme.Extents.ControlSpacing;
        }
        else
        {
            Walking += Theme.Extents.EntryRowHeight + Theme.Extents.ControlSpacing;
        }
    }

    ReclaimClip();

    PresentFooterBand(Theme, FooterBand, Counted, WrittenCount);
}

}   // namespace Slate
