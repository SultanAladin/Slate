//============================================================================================================================================
//                                                       SCENEDIRECTORYPANEL.CPP
//============================================================================================================================================
// 🧩 The editor's scene directory — leaf content for the editor's workspace panels.
//    See SceneDirectoryPanel.h for the boundary this panel observes: it records
//    ONLY inside the leaf extents the editor's panel chrome hands over, and it
//    never draws the validation shell's rail, top bar or layer stack.

#include "SlateUI/Interface/SceneDirectoryPanel/Api/SceneDirectoryPanel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace Slate
{

namespace
{

constexpr double HoverOver = 120.0;   // [ms] - the reference's transition-colors duration
constexpr float  RunLeading = 1.30f;  // [-]  - leading-tight, for a two-run header

/// 🧩 Holds an coordinate between two bounds.
constexpr float Held(float Coordinate, float Minimum, float Maximum)
{
    return (Coordinate < Minimum) ? Minimum : (Coordinate > Maximum) ? Maximum : Coordinate;
}

/// 🧩 One coordinate of the way from a departed figure to an incoming one.
constexpr float Between(float Previous, float Incoming, float Fraction)
{
    return Previous + (Incoming - Previous) * Fraction;
}

/// 🧩 The same colour at a declared fraction of its own coverage.
constexpr ThemeToken Faded(ThemeToken Declared, float Fraction)
{
    const float Bounded = Held(Fraction, 0.0f, 1.0f);
    Declared.Opacity    = static_cast<std::uint8_t>(static_cast<float>(Declared.Opacity) * Bounded + 0.5f);
    return Declared;
}

}   // namespace

ShellMetric ScaleShellLengths(float Factor)
{
    const float Applied = (Factor > 0.0f) ? Factor : 1.0f;
    ShellMetric Scaled;

    // 📝 Every member is a length, so the whole record is scaled uniformly. The two run figures that are
    //    durations live outside it precisely so that this stays true and no member has to be excepted.
    float* const Lengths = &Scaled.TopBarHeight;
    const std::uint32_t Count = static_cast<std::uint32_t>(sizeof(ShellMetric) / sizeof(float));

    for (std::uint32_t Ordinal = 0u; Ordinal < Count; ++Ordinal)
        Lengths[Ordinal] *= Applied;

    return Scaled;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    ENTITY CLASSIFICATION
//------------------------------------------------------------------------------------------------------------------------

SymbolSubject EntityGlyph(EntitySubject Subject)
{
    switch (Subject)
    {
        case EntitySubject::Level:      return SymbolSubject::CubeSolid;
        case EntitySubject::Grouping:   return SymbolSubject::FolderClosed;
        case EntitySubject::Actor:      return SymbolSubject::CubeSolid;
        case EntitySubject::Camera:     return SymbolSubject::CameraAperture;
        case EntitySubject::Illuminant: return SymbolSubject::BulbFilament;
        case EntitySubject::Audio:      return SymbolSubject::SpeakerCone;
        case EntitySubject::Particle:   return SymbolSubject::ParticleEmit;
        case EntitySubject::Trigger:    return SymbolSubject::CrosshairCentre;
        case EntitySubject::Script:     return SymbolSubject::CodeBrackets;
        case EntitySubject::Sun:        return SymbolSubject::SunDirectional;
        case EntitySubject::Sky:        return SymbolSubject::SunDirectional;
        default:                        return SymbolSubject::CubeSolid;
    }
}

ThemeToken EntityHue(EntitySubject Subject)
{
    // 📐 The reference's `COLORS` record, transcribed verbatim from `components/GameOutliner.tsx`.
    switch (Subject)
    {
        case EntitySubject::Level:      return Covering(0xEAB308u);
        case EntitySubject::Grouping:   return Covering(0x8A8A8Au);
        case EntitySubject::Actor:      return Covering(0x3B82F6u);
        case EntitySubject::Camera:     return Covering(0xEC4899u);
        case EntitySubject::Illuminant: return Covering(0xF59E0Bu);
        case EntitySubject::Audio:      return Covering(0x8B5CF6u);
        case EntitySubject::Particle:   return Covering(0x10B981u);
        case EntitySubject::Trigger:    return Covering(0xEF4444u);
        case EntitySubject::Script:     return Covering(0x06B6D4u);
        case EntitySubject::Sun:        return Covering(0xF59E0Bu);
        case EntitySubject::Sky:        return Covering(0x38BDF8u);
        default:                        return Covering(0x8A8A8Au);
    }
}

const char* EntityText(EntitySubject Subject)
{
    switch (Subject)
    {
        case EntitySubject::Level:      return "Level";
        case EntitySubject::Grouping:   return "Folder";
        case EntitySubject::Actor:      return "Actor";
        case EntitySubject::Camera:     return "Camera";
        case EntitySubject::Illuminant: return "Light";
        case EntitySubject::Audio:      return "Audio";
        case EntitySubject::Particle:   return "Particle";
        case EntitySubject::Trigger:    return "Trigger";
        case EntitySubject::Script:     return "Script";
        case EntitySubject::Sun:        return "Sun";
        case EntitySubject::Sky:        return "Sky";
        default:                        return "Entity";
    }
}

ThemeToken RevisionHue(RevisionSubject Classified)
{
    // 📐 The reference's `REVISION_HUE` record, transcribed verbatim from `components/Inspector.tsx`.
    switch (Classified)
    {
        case RevisionSubject::Start:     return Covering(0x7EC8FFu);
        case RevisionSubject::Feature:   return Covering(0xFFB24Du);
        case RevisionSubject::Parameter: return Covering(0x4FD18Bu);
        case RevisionSubject::Sketch:    return Covering(0x37D6D6u);
        case RevisionSubject::Relocate:  return Covering(0x5B8CFFu);
        case RevisionSubject::Grouped:   return Covering(0xB98BFFu);
        case RevisionSubject::Created:   return Covering(0x7EC8FFu);
        case RevisionSubject::Amended:   return Covering(0xC99B6Au);
        case RevisionSubject::Dropped:   return Covering(0xFF6B6Bu);
        default:                          return Covering(0xC99B6Au);
    }
}

const char* RevisionText(RevisionSubject Classified)
{
    // 📐 The `label` of the reference's `REVISION_CLASS` record, verbatim.
    switch (Classified)
    {
        case RevisionSubject::Start:     return "Start";
        case RevisionSubject::Feature:   return "Feature";
        case RevisionSubject::Parameter: return "Params";
        case RevisionSubject::Sketch:    return "Sketch";
        case RevisionSubject::Relocate:  return "Relocate";
        case RevisionSubject::Grouped:   return "Group";
        case RevisionSubject::Created:   return "Create";
        case RevisionSubject::Amended:   return "Edit";
        case RevisionSubject::Dropped:   return "Drop";
        default:                          return "Edit";
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

Outcome<bool> SceneDirectoryPanel::Construct(InteractionIndex& Interaction,
                                             MotionIntegrator& Integrator,
                                             RecordingSurface& Surface,
                                             const ThemeProfile& Resolved)
{
    if (Ledger != nullptr)
    {
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported,
                                       "the scene directory panel is already constructed" });
    }

    Ledger     = &Interaction;
    Motion     = &Integrator;
    this->Surface = &Surface;
    Appearance = &Resolved;

    if (!Controls.Construct(Interaction, Surface, Resolved).Resolved)
    {
        Reset();
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent,
                                       "the shared inspector controls were rejected" });
    }

    if (!EnvironmentControls.Construct(Interaction, Surface, Resolved).Resolved)
    {
        Reset();
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent,
                                       "the environment slider controls were rejected" });
    }

    // 🔴 Every identity is claimed here and none inside a tick. A control registered mid-tick receives a fresh
    //    fade and reads as though the pointer had just arrived over it, once per tick, forever.
    ControlIdentity* const Every[] =
    {
        &InspectorStrip,
        &EnvironmentSliders[0], &EnvironmentSliders[1], &EnvironmentSliders[2],
        &EnvironmentSliders[3], &EnvironmentSliders[4], &EnvironmentSliders[5],
        &CardFolds[0], &CardFolds[1], &CardFolds[2], &CardFolds[3]
    };

    for (ControlIdentity* Identity : Every)
    {
        const Outcome<ControlIdentity> Registered = Interaction.Register();
        if (!Registered.Resolved)
            return Outcome<bool>::Refuse(Registered.Error);

        *Identity = Registered.Resolve();
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < SceneDirectoryContext::EntityCeiling; ++Ordinal)
    {
        ControlIdentity* const Rows[] =
        {
            &RowContacts[Ordinal], &RowDisclosures[Ordinal], &RowPresences[Ordinal],
            &DetailOptions[Ordinal][0], &DetailOptions[Ordinal][1], &DetailOptions[Ordinal][2],
            &RevisionGroups[Ordinal]
        };

        for (ControlIdentity* Identity : Rows)
        {
            const Outcome<ControlIdentity> Registered = Interaction.Register();
            if (!Registered.Resolved)
                return Outcome<bool>::Refuse(Registered.Error);

            *Identity = Registered.Resolve();
        }
    }

    Reapply(Resolved);

    return Outcome<bool>::Result(true);
}

void SceneDirectoryPanel::Advance(const PointerCondition& Contact, double Elapsed)
{
    Sampled = Contact;
    Controls.Advance(Contact, Elapsed);
    // 📝 Sampled, never advanced: the tick owner advances the shared ledger exactly once, and a
    //    second advance would retire the release before the panel reads it.
    EnvironmentControls.Sample(Contact);
}

void SceneDirectoryPanel::Reset()
{
    Controls.Reset();

    Ledger     = nullptr;
    Motion     = nullptr;
    Surface    = nullptr;
    Appearance = nullptr;
    Sampled    = {};
    Tinted     = {};
    Scaled     = {};

    for (std::uint32_t Ordinal = 0u; Ordinal < 6u; ++Ordinal)
    {
        EnvironmentArmed[Ordinal] = false;
        EnvironmentFrom[Ordinal]  = 0.0;
    }
}

void SceneDirectoryPanel::Reapply(const ThemeProfile& Resolved)
{
    Appearance = &Resolved;

    // 🔴 The colours are taken from the appearance rather than left at their compiled-in declarations, which is
    //    what carries a theme into the panel. `Reapply` is already called at construction and again on every
    //    display change, so the one line below is also the whole of the panel's theme response.
    Tinted = Resolved.Shell;

    // 📝 The scene directory is authored at engine density, exactly as the shell's own is, so it takes the
    //    display and artist factors rather than the control sheet's authored reduction.
    const float Applied = static_cast<float>(Resolved.Measure.DisplayScale)
                        * Resolved.ControlMeasure.ArtistFactor;

    Scaled = ScaleShellLengths(Applied);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE VIEWPORT SKY
//------------------------------------------------------------------------------------------------------------------------

void SceneDirectoryPanel::RecordViewportSky(const PlaneExtent& Extent, const SceneDirectoryContext& Applied)
{
    if (Applied.SkyTextureIdentity == 0u)
        return;

    const EnvironmentConfiguration& Sky = Applied.Environment;

    const float HalfV = Applied.ViewportSkyCamera.FieldOfViewDegrees * 0.5f;
    const float HalfH = std::atan(std::tan(HalfV * 3.14159265f / 180.0f)
                                  * (Extent.Width() / Extent.Height())) * 180.0f / 3.14159265f;

    const float Azimuth   = Applied.ViewportSkyCamera.AzimuthDegrees;
    const float Elevation = Applied.ViewportSkyCamera.ElevationDegrees;

    // 📐 Dome coordinates: U = (azimuth + 180) / 360, V = (90 − elevation) / 180. The crop is the
    //    camera's frustum on the dome, clamped so it never leaves the texture.
    float U0 = std::clamp((Azimuth - HalfH + 180.0f) / 360.0f, 0.0f, 1.0f);
    float U1 = std::clamp((Azimuth + HalfH + 180.0f) / 360.0f, 0.0f, 1.0f);
    float V0 = std::clamp((90.0f - (Elevation + HalfV)) / 180.0f, 0.0f, 1.0f);
    float V1 = std::clamp((90.0f - (Elevation - HalfV)) / 180.0f, 0.0f, 1.0f);

    // 📐 The sun stays in frame at any viewport aspect: the camera aims twenty degrees wide of the sun,
    //    which a docked viewport's narrower frustum would otherwise crop out. When the sun's dome
    //    coordinate falls outside the frustum, the crop shifts to contain it with a small cushion.
    const float SunU = std::clamp(static_cast<float>(Sky.SunAzimuth + 180.0) / 360.0f, 0.0f, 1.0f);
    const float SunV = std::clamp(static_cast<float>(90.0 - Sky.SunElevation) / 180.0f, 0.0f, 1.0f);
    constexpr float CushionU = 0.012f;   // [-] - half a disc's width, so the disc is not glued to the edge
    constexpr float CushionV = 0.012f;   // [-]

    float ShiftU = 0.0f;
    if (SunU < U0 + CushionU)
        ShiftU = U0 + CushionU - SunU;
    else if (SunU > U1 - CushionU)
        ShiftU = U1 - CushionU - SunU;

    float ShiftV = 0.0f;
    if (SunV < V0 + CushionV)
        ShiftV = V0 + CushionV - SunV;
    else if (SunV > V1 - CushionV)
        ShiftV = V1 - CushionV - SunV;

    U0 = std::clamp(U0 - ShiftU, 0.0f, 1.0f);
    U1 = std::clamp(U1 - ShiftU, 0.0f, 1.0f);
    V0 = std::clamp(V0 - ShiftV, 0.0f, 1.0f);
    V1 = std::clamp(V1 - ShiftV, 0.0f, 1.0f);

    Surface->Image(Extent, Applied.SkyTextureIdentity, U0, V0, U1, V1);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE OUTLINER
//------------------------------------------------------------------------------------------------------------------------

void SceneDirectoryPanel::RecordLeafHeader(const PlaneExtent& Extent, SymbolSubject Glyph,
                                           const ThemeToken& Hue, const char* Titled,
                                           const char* Secondary)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);
    Surface->Ground(Spanning(Extent.MinimumX, Extent.MaximumY - 1.0f, Extent.Width(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    const float Pad      = Scaled.HeaderPadX;
    const float Medallion = Scaled.MedallionExtent;

    const PlaneExtent Crest = Spanning(Extent.MinimumX + Pad,
                                       Extent.MinimumY + (Extent.Height() - Medallion) * 0.5f,
                                       Medallion, Medallion);

    Surface->Ground(Crest, Hue, 6.0f, CornerAll);

    const float Figure = Medallion * 0.62f;
    Surface->Stroke(Glyph,
                    Spanning(Crest.MinimumX + (Medallion - Figure) * 0.5f,
                             Crest.MinimumY + (Medallion - Figure) * 0.5f, Figure, Figure),
                    Covering(0xFFFFFFu));

    const float Run        = Scaled.RunPrimary;
    const float SecondaryRun = Scaled.RunFine;
    const float PairHeight = Run * RunLeading + SecondaryRun * RunLeading;
    const float PairLead   = Extent.MinimumY + (Extent.Height() - PairHeight) * 0.5f;
    const float RunLead    = Crest.MaximumX + Pad;

    Surface->TextRunTruncated(RunLead, PairLead, Extent.MaximumX - RunLead - Pad,
                              Tinted.Primary, Titled, Run, true);
    Surface->TextRunTruncated(RunLead, PairLead + Run * RunLeading,
                              Extent.MaximumX - RunLead - Pad, Hue, Secondary, SecondaryRun, false);
}

void SceneDirectoryPanel::RecordOutliner(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                                         const EntityRow* Rows, std::uint32_t RowCount)
{
    if (Rows == nullptr)
        RowCount = 0u;

    if (RowCount > SceneDirectoryContext::EntityCeiling)
        RowCount = SceneDirectoryContext::EntityCeiling;

    Surface->Ground(Extent, Tinted.Menu, 0.0f, CornerNone);

    const float Pad = Scaled.PanePad;

    // 📐 The outliner column and the details pane beside it — the same `350px_minmax(0,1fr)` split the
    //    shell's scene directory presents. The details pane is the small metadata/options card.
    const float OutlinerX = (Scaled.OutlinerX < Extent.Width() * 0.6f)
                          ? Scaled.OutlinerX : Extent.Width() * 0.6f;

    const PlaneExtent Outlining = Spanning(Extent.MinimumX, Extent.MinimumY,
                                           OutlinerX, Extent.Height());

    const PlaneExtent Header = Spanning(Outlining.MinimumX, Outlining.MinimumY,
                                        Outlining.Width(), Scaled.HeaderHeight);

    RecordLeafHeader(Header, SymbolSubject::GearCog, Tinted.EntityAccent, "Scene Directory",
                     "World Outliner");

    const PlaneExtent Footer = Spanning(Outlining.MinimumX, Outlining.MaximumY - Scaled.FooterHeight,
                                        Outlining.Width(), Scaled.FooterHeight);

    const PlaneExtent Body = Spanning(Outlining.MinimumX + Pad, Header.MaximumY + Pad,
                                      Outlining.Width() - Pad * 2.0f,
                                      Footer.MinimumY - Header.MaximumY - Pad);

    Surface->Confine(Body);

    float Sweep = Body.MinimumY;

    for (std::uint32_t Ordinal = 0u; Ordinal < RowCount; ++Ordinal)
    {
        const bool Expanded = Applied.EntityExpanded[Ordinal];

        if (Ordinal > 0u && !Expanded && Rows[Ordinal].Depth > Rows[Ordinal - 1u].Depth &&
            !Applied.EntityPresent[Ordinal])
            continue;

        const EntityRow&  EntryRow = Rows[Ordinal];
        const PlaneExtent Row      = Spanning(Body.MinimumX, Sweep, Body.Width(), Scaled.RowHeight);

        Sweep += Scaled.RowHeight;

        if (Sweep > Body.MaximumY)
            break;

        const bool Taken   = Applied.EntityTaken == Ordinal;
        const bool Hovered = Row.Encloses(Sampled.PositionX, Sampled.PositionY);
        const bool Absent  = !Applied.EntityPresent[Ordinal];
        const bool Branch  = EntryRow.EnclosedCount > 0u;

        const float LeadX = Row.MinimumX + Scaled.RowLeadX
                          + static_cast<float>(EntryRow.Depth) * Scaled.RowStepX;

        // ① The disclosure cell, which takes the contact before the row does.
        const PlaneExtent Chevron = Spanning(LeadX,
                                             Row.MinimumY + (Row.Height() - Scaled.ChevronExtent) * 0.5f,
                                             Scaled.ChevronExtent, Scaled.ChevronExtent);

        const bool OnChevron = Branch && Chevron.Encloses(Sampled.PositionX, Sampled.PositionY);

        // ② The presence action at the trailing edge, outranking the row.
        const float PresenceExtent = Scaled.GlyphExtent * (20.0f / 18.0f);
        const PlaneExtent Presence = Spanning(Row.MaximumX - PresenceExtent - Scaled.PanePad * 0.5f,
                                              Row.MinimumY + (Row.Height() - PresenceExtent) * 0.5f,
                                              PresenceExtent, PresenceExtent);

        const bool OnPresence = Presence.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Sampled.ContactPressed && !Ledger->AnyDisclosed())
        {
            if (OnChevron)
                Ledger->Grab(RowDisclosures[Ordinal], ControlPart::Chevron);
            else if (OnPresence)
                Ledger->Grab(RowPresences[Ordinal], ControlPart::Body);
            else if (Hovered)
                Ledger->Grab(RowContacts[Ordinal], ControlPart::Body);
        }

        if (OnChevron && Ledger->Released(RowDisclosures[Ordinal]))
            Applied.EntityExpanded[Ordinal] = !Applied.EntityExpanded[Ordinal];

        if (OnPresence && Ledger->Released(RowPresences[Ordinal]))
        {
            const bool Incoming = !Applied.EntityPresent[Ordinal];

            Applied.EntityPresent[Ordinal] = Incoming;

            for (std::uint32_t Inward = Ordinal + 1u; Inward < RowCount; ++Inward)
            {
                if (Rows[Inward].Depth <= EntryRow.Depth)
                    break;

                Applied.EntityPresent[Inward] = Incoming;
            }
        }

        if (Hovered && !OnChevron && !OnPresence && Ledger->Released(RowContacts[Ordinal]))
            Applied.EntityTaken = Ordinal;

        Ledger->DeclareHovered(RowContacts[Ordinal], Hovered, HoverOver);

        // ③ The row ground, then its rail. A withheld row draws at half coverage.
        const float Coverage = Absent ? 0.5f : 1.0f;

        if (Taken)
            Surface->Ground(Row, Faded(Tinted.EntityTaken, Coverage), Scaled.FieldRadius, CornerAll);
        else if (Hovered)
            Surface->Ground(Row, Faded(Tinted.RowHovered, Coverage), Scaled.FieldRadius, CornerAll);

        if (Taken)
        {
            const PlaneExtent Rail = Spanning(Row.MinimumX - Scaled.RailOffsetX,
                                              Row.MinimumY + (Row.Height() - Scaled.RailY) * 0.5f,
                                              Scaled.RailX, Scaled.RailY);

            Surface->Ground(Rail, Faded(Tinted.EntityAccent, Coverage), 2.0f,
                            CornerTrailingUpper | CornerTrailingLower);
        }

        if (Branch)
            Surface->Stroke(Applied.EntityExpanded[Ordinal]
                            ? SymbolSubject::ChevronDown : SymbolSubject::ChevronRight,
                            Chevron, Faded(Tinted.Faint, Coverage));

        const float GlyphLead = LeadX + Scaled.ChevronExtent + Scaled.PanePad;
        const PlaneExtent Glyph = Spanning(GlyphLead,
                                           Row.MinimumY + (Row.Height() - Scaled.GlyphExtent) * 0.5f,
                                           Scaled.GlyphExtent, Scaled.GlyphExtent);

        Surface->Stroke(EntityGlyph(EntryRow.Subject), Glyph,
                        Faded(EntityHue(EntryRow.Subject), Coverage));

        const float NamingRun  = Scaled.RunPrimary;
        const float NamingLead = Glyph.MaximumX + Scaled.PanePad;
        const float NamingTop  = Row.MinimumY + (Row.Height() - NamingRun) * 0.5f;

        float NamingCeiling = Presence.MinimumX - Scaled.PanePad;

        if (Branch)
        {
            char Counted[12] = {};
            std::snprintf(Counted, sizeof(Counted), "%u",
                          static_cast<unsigned>(EntryRow.EnclosedCount));

            const float CountRun  = Scaled.RunFine;
            const float CountLead = NamingCeiling - Surface->MeasureRun(Counted, CountRun, 0.0f);

            Surface->TextRun(CountLead, Row.MinimumY + (Row.Height() - CountRun) * 0.5f,
                             Faded(Tinted.Faint, Coverage), Counted, CountRun);

            NamingCeiling = CountLead - Scaled.PanePad;
        }

        Surface->TextRunTruncated(NamingLead, NamingTop, NamingCeiling,
                                  Faded(Taken ? Tinted.Primary : (Hovered ? Tinted.Primary : Tinted.Muted),
                                        Coverage),
                                  EntryRow.Naming, NamingRun);

        if (Hovered || Absent)
        {
            if (OnPresence)
                Surface->Ground(Presence, Tinted.TileHovered, 3.0f, CornerAll);

            const float EyeExtent = PresenceExtent * (14.0f / 20.0f);
            const PlaneExtent Eye = Spanning(Presence.MinimumX + (PresenceExtent - EyeExtent) * 0.5f,
                                             Presence.MinimumY + (PresenceExtent - EyeExtent) * 0.5f,
                                             EyeExtent, EyeExtent);

            Surface->Stroke(Absent ? SymbolSubject::EyeClosed : SymbolSubject::EyeOpen, Eye,
                            OnPresence ? Tinted.Primary : Tinted.Faint);
        }
    }

    Surface->Release();

    // ④ The footer, `{count} entities`.
    Surface->Ground(Footer, Tinted.MenuLower, 0.0f, CornerNone);
    Surface->Ground(Spanning(Footer.MinimumX, Footer.MinimumY, Footer.Width(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    char Counted[16] = {};
    std::snprintf(Counted, sizeof(Counted), "%u", static_cast<unsigned>(RowCount));

    const float FooterRun = Scaled.RunFine;
    const float FooterTop = Footer.MinimumY + (Footer.Height() - FooterRun) * 0.5f;
    const float FooterLead = Footer.MinimumX + Scaled.HeaderPadX;

    Surface->TextRun(FooterLead, FooterTop, Tinted.Primary, Counted, FooterRun, 0.0f, true);
    Surface->TextRun(FooterLead + Surface->MeasureRun(Counted, FooterRun, 0.0f) + 4.0f, FooterTop,
                     Tinted.Muted, " entities", FooterRun);

    // ⑤ The details pane — the small metadata and options card for the taken row.
    const PlaneExtent Detailing = Spanning(Outlining.MaximumX, Extent.MinimumY,
                                           Extent.MaximumX - Outlining.MaximumX, Extent.Height());

    if (RowCount == 0u || Applied.EntityTaken >= RowCount)
    {
        RecordLeafHeader(Spanning(Detailing.MinimumX, Detailing.MinimumY,
                                  Detailing.Width(), Scaled.HeaderHeight),
                         SymbolSubject::CrosshairCentre, Tinted.Faint,
                         "Details", "Nothing selected");

        const float Run = Scaled.RunSecondary;
        const char* Prose = "Select a record in the directory to view its details.";

        Surface->TextRun(Detailing.MinimumX + (Detailing.Width()
                                                  - Surface->MeasureRun(Prose, Run, 0.0f)) * 0.5f,
                         Detailing.MinimumY + Scaled.HeaderHeight + Scaled.PanePad * 3.0f,
                         Tinted.Faint, Prose, Run);
        return;
    }

    const std::uint32_t Ordinal = Applied.EntityTaken;
    const EntityRow&    Current = Rows[Ordinal];
    const ThemeToken    Hue     = EntityHue(Current.Subject);

    const PlaneExtent DetailsHeader = Spanning(Detailing.MinimumX, Detailing.MinimumY,
                                               Detailing.Width(), Scaled.HeaderHeight);

    RecordLeafHeader(DetailsHeader, EntityGlyph(Current.Subject), Hue,
                     Current.Naming, EntityText(Current.Subject));

    const float DetailPad = Scaled.PanePad * 1.5f;
    const PlaneExtent DetailBody = Spanning(Detailing.MinimumX + DetailPad,
                                            DetailsHeader.MaximumY + DetailPad,
                                            Detailing.Width() - DetailPad * 2.0f,
                                            Detailing.MaximumY - DetailsHeader.MaximumY - DetailPad);

    Surface->Confine(DetailBody);

    float DetailSweep = DetailBody.MinimumY;

    // 📐 The hero tile — the crest, the token and the classification in the subject's own hue.
    {
        const float HeroHeight = Scaled.HeroCrest + Scaled.HeroPad * 2.0f;
        const PlaneExtent Hero = Spanning(DetailBody.MinimumX, DetailSweep,
                                          DetailBody.Width(), HeroHeight);

        Surface->Ground(Hero, Tinted.Tile, Scaled.CardRadius, CornerAll);
        Surface->Edge(Hero, Tinted.Hairline, 1.0f, Scaled.CardRadius, CornerAll);

        const PlaneExtent Crest = Spanning(Hero.MinimumX + Scaled.HeroPad,
                                           Hero.MinimumY + Scaled.HeroPad,
                                           Scaled.HeroCrest, Scaled.HeroCrest);

        Surface->Ground(Crest, Hue, 8.0f, CornerAll);

        const float Figure = Scaled.HeroCrest * 0.55f;

        Surface->Stroke(EntityGlyph(Current.Subject),
                        Spanning(Crest.MinimumX + (Scaled.HeroCrest - Figure) * 0.5f,
                                 Crest.MinimumY + (Scaled.HeroCrest - Figure) * 0.5f,
                                 Figure, Figure), Covering(0xFFFFFFu));

        char Token[12] = {};
        std::snprintf(Token, sizeof(Token), "g_%02u", static_cast<unsigned>(Ordinal + 1u));

        const float NameRun = Scaled.RunPrimary;
        const float PairRun = Scaled.RunFine;

        Surface->TextRunTruncated(Crest.MaximumX + Scaled.HeroPad,
                                  Hero.MinimumY + Scaled.HeroPad * 0.9f,
                                  Hero.MaximumX - Scaled.HeroPad,
                                  Tinted.Primary, Current.Naming, NameRun, true);
        Surface->TextRun(Crest.MaximumX + Scaled.HeroPad,
                         Hero.MinimumY + Scaled.HeroPad * 0.9f + NameRun * RunLeading,
                         Hue, Token, PairRun);

        DetailSweep += HeroHeight + Scaled.PanePad;
    }

    RecordDetailOptions(Spanning(DetailBody.MinimumX, DetailSweep,
                                 DetailBody.Width(), DetailBody.MaximumY - DetailSweep),
                        Applied, Ordinal, Current);

    Surface->Release();
}

void SceneDirectoryPanel::RecordDetailOptions(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                                              std::uint32_t Ordinal, const EntityRow& Current)
{
    // 📐 The camera row's options are the camera's own settings: the lag and the pitch direction,
    //    beside the visibility every row carries. Every other row keeps the reference's generic
    //    options. The bits are the same slots — bit 1 is lag on the camera, Locked elsewhere.
    const bool Camera = Current.Subject == EntitySubject::Camera;

    const char* const CameraCaptions[3]    = { "Visible", "Camera Lag", "Invert Pitch" };
    const char* const GenericCaptions[3]   = { "Visible", "Locked", "Cast Shadows" };
    const char* const* const Captions = Camera ? CameraCaptions : GenericCaptions;
    const float RowY = Scaled.RowHeight;

    float Sweep = Extent.MinimumY;

    Surface->Ground(Spanning(Extent.MinimumX, Sweep, Extent.Width(), RowY),
                    Tinted.Tile, Scaled.FieldRadius, CornerAll);
    Surface->TextRun(Extent.MinimumX + Scaled.PanePad * 2.0f,
                     Sweep + (RowY - Scaled.RunPrimary) * 0.5f,
                     Tinted.Muted, "Options", Scaled.RunPrimary);

    Sweep += RowY + Scaled.PanePad * 0.5f;

    for (std::uint32_t Option = 0u; Option < 3u; ++Option)
    {
        const PlaneExtent Row = Spanning(Extent.MinimumX, Sweep, Extent.Width(), RowY);

        if (Sweep + RowY > Extent.MaximumY)
            break;

        const bool OnRow = Row.Encloses(Sampled.PositionX, Sampled.PositionY);

        const bool State = (Option == 0u) ? Applied.EntityPresent[Ordinal]
                          : ((Applied.DetailBits[Ordinal] & (1u << Option)) != 0u);

        if (Sampled.ContactPressed && OnRow && !Ledger->AnyDisclosed())
            Ledger->Grab(DetailOptions[Ordinal][Option], ControlPart::Body);

        if (OnRow && Ledger->Released(DetailOptions[Ordinal][Option]))
        {
            if (Option == 0u)
                Applied.EntityPresent[Ordinal] = !Applied.EntityPresent[Ordinal];
            else
                Applied.DetailBits[Ordinal] ^= (1u << Option);
        }

        Ledger->DeclareHovered(DetailOptions[Ordinal][Option], OnRow, HoverOver);

        if (OnRow)
            Surface->Ground(Row, Tinted.TileHovered, Scaled.FieldRadius, CornerAll);

        const float Toggle = Scaled.ChipExtent * 2.5f;
        const PlaneExtent Switch = Spanning(Row.MaximumX - Toggle - Scaled.PanePad * 1.5f,
                                            Row.MinimumY + (Row.Height() - Toggle * 0.5f) * 0.5f,
                                            Toggle, Toggle * 0.5f);

        Surface->Ground(Switch, State ? Tinted.EntityAccent : Tinted.Hairline,
                        Toggle * 0.25f, CornerAll);

        const float Knob = Toggle * 0.5f - 2.0f;
        Surface->Medallion(State ? Switch.MaximumX - Knob - 1.0f : Switch.MinimumX + Knob + 1.0f,
                           Switch.MinimumY + Toggle * 0.25f, Knob, Covering(0xFFFFFFu));

        Surface->TextRun(Row.MinimumX + Scaled.PanePad * 2.0f,
                         Row.MinimumY + (Row.Height() - Scaled.RunPrimary) * 0.5f,
                         OnRow ? Tinted.Primary : Tinted.Muted, Captions[Option], Scaled.RunPrimary);

        Sweep += RowY + Scaled.PanePad * 0.5f;
    }

    // 📐 The camera's small metadata: the pose the rig reports and the speed the artist set, stated
    //    as plain stat rows beneath the options.
    if (Camera)
    {
        Sweep += Scaled.PanePad * 0.5f;

        const float StatY = Scaled.StatY;
        const PlaneExtent Stats = Spanning(Extent.MinimumX, Sweep, Extent.Width(),
                                           StatY * 3.0f + Scaled.PanePad);

        if (Stats.MaximumY <= Extent.MaximumY)
        {
            Surface->Ground(Stats, Tinted.Tile, Scaled.FieldRadius, CornerAll);

            const auto StateRow = [&](const char* Caption, const char* Value)
            {
                const PlaneExtent Row = Spanning(Stats.MinimumX, Sweep, Stats.Width(), StatY);

                Surface->TextRun(Row.MinimumX + Scaled.PanePad * 2.0f,
                                 Row.MinimumY + (Row.Height() - Scaled.RunFine) * 0.5f,
                                 Tinted.Muted, Caption, Scaled.RunFine);

                Surface->TextRun(Row.MaximumX - Scaled.PanePad * 2.0f
                                 - Surface->MeasureRun(Value, Scaled.RunFine, 0.0f),
                                 Row.MinimumY + (Row.Height() - Scaled.RunFine) * 0.5f,
                                 Tinted.Primary, Value, Scaled.RunFine);

                Sweep += StatY;
            };

            char Positioned[64] = {};
            std::snprintf(Positioned, sizeof(Positioned), "%.0f, %.1f, %.0f m",
                          Applied.CameraPosition[0], Applied.CameraPosition[1], Applied.CameraPosition[2]);
            StateRow("Position", Positioned);

            char Turned[64] = {};
            std::snprintf(Turned, sizeof(Turned), "yaw %.0f\u00B0  pitch %.0f\u00B0",
                          Applied.CameraRotation[0], Applied.CameraRotation[1]);
            StateRow("Rotation", Turned);

            char Stepped[64] = {};
            std::snprintf(Stepped, sizeof(Stepped), "%.0f m/s", Applied.CameraSpeed);
            StateRow("Speed", Stepped);
        }
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                   PROPERTIES | HISTORY
//------------------------------------------------------------------------------------------------------------------------

void SceneDirectoryPanel::RecordProperties(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                                           const EntityRow* Rows, std::uint32_t RowCount,
                                           const EntityRevision* Revisions, std::uint32_t RevisionCount)
{
    if (Rows == nullptr)
        RowCount = 0u;

    if (RowCount > SceneDirectoryContext::EntityCeiling)
        RowCount = SceneDirectoryContext::EntityCeiling;

    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    const bool Selected = Applied.EntityTaken < RowCount;

    const PlaneExtent Header = Spanning(Extent.MinimumX, Extent.MinimumY,
                                        Extent.Width(), Scaled.HeaderHeight);

    if (!Selected)
    {
        RecordLeafHeader(Header, SymbolSubject::CubeSolid, Tinted.Faint,
                         "Nothing selected", "No entity");
    }
    else
    {
        const EntityRow&  Current = Rows[Applied.EntityTaken];
        const ThemeToken Hue     = EntityHue(Current.Subject);

        char Classified[48] = {};
        std::snprintf(Classified, sizeof(Classified), "%s Entity", EntityText(Current.Subject));

        RecordLeafHeader(Header, EntityGlyph(Current.Subject), Hue, Current.Naming, Classified);
    }

    // ① The strip, and the inner pages it drives.
    static const char* const Captions[2] = { "Properties", "History" };

    const PlaneExtent Strip = Spanning(Extent.MinimumX, Header.MaximumY,
                                       Extent.Width(), Scaled.ComponentY);

    const TabDeclaration Declared{ Captions, 2u };

    static_cast<void>(Controls.TabStrip(InspectorStrip, Strip, Declared, Applied.InspectorTab));

    const PlaneExtent Pages = Spanning(Extent.MinimumX, Strip.MaximumY, Extent.Width(),
                                       Extent.MaximumY - Strip.MaximumY - Scaled.FooterHeight);

    // 📐 The two pages sit side by side and the strip travels between them, exactly as the shell's
    //    inspector does — the travel is one whole page extent.
    const float Carried = (Applied.InspectorTab == 1u) ? -Pages.Width() : 0.0f;

    Surface->Confine(Pages);

    const PlaneExtent Leading = Spanning(Pages.MinimumX + Carried, Pages.MinimumY,
                                         Pages.Width(), Pages.Height());
    const PlaneExtent Trailing = Spanning(Leading.MaximumX, Pages.MinimumY,
                                          Pages.Width(), Pages.Height());

    if (!Surface->Excluded(Leading))
        RecordPropertyCards(Leading, Applied, Rows, RowCount);

    if (!Surface->Excluded(Trailing))
        RecordRevisionSpine(Trailing, Applied, Rows, RowCount, Revisions, RevisionCount);

    Surface->Release();

    // ② The footer, `{n} fields` — the strip's selection stated in the entity's own hue.
    const PlaneExtent Footer = Spanning(Extent.MinimumX, Extent.MaximumY - Scaled.FooterHeight,
                                        Extent.Width(), Scaled.FooterHeight);

    Surface->Ground(Footer, Tinted.MenuLower, 0.0f, CornerNone);
    Surface->Ground(Spanning(Footer.MinimumX, Footer.MinimumY, Footer.Width(), 1.0f),
                    Tinted.Hairline, 0.0f, CornerNone);

    if (Selected)
    {
        const ThemeToken Hue       = EntityHue(Rows[Applied.EntityTaken].Subject);
        const float       FooterRun = Scaled.RunFine;
        const float       FooterTop = Footer.MinimumY + (Footer.Height() - FooterRun) * 0.5f;
        const float       ChipY     = Footer.MinimumY
                                    + (Footer.Height() - Scaled.ChipExtent) * 0.5f;

        Surface->Ground(Spanning(Footer.MinimumX + Scaled.HeaderPadX, ChipY,
                                 Scaled.ChipExtent, Scaled.ChipExtent), Hue, 2.0f, CornerAll);

        Surface->TextRun(Footer.MinimumX + Scaled.HeaderPadX + Scaled.ChipExtent
                         + Scaled.PanePad, FooterTop, Tinted.Muted,
                         (Applied.InspectorTab == 0u) ? "Properties" : "History", FooterRun);
    }
}

void SceneDirectoryPanel::RecordPropertyCards(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                                              const EntityRow* Rows, std::uint32_t RowCount)
{
    if (Extent.Width() <= 0.0f || Extent.Height() <= 0.0f)
        return;

    const float Pad = Scaled.PanePad;
    float       Sweep = Extent.MinimumY + Pad;
    std::uint32_t CardOrdinal = 0u;

    // 📝 The property card — a folding card whose rows are inert labels, from the reference's generic
    //    component cards.
    const auto RecordCard = [&](const char* Caption, const char* const* Fields, std::uint32_t FieldCount)
    {
        if (CardOrdinal >= SceneDirectoryContext::CardCeiling)
            return;

        const std::uint32_t Target = CardOrdinal++;
        const bool  Folded   = Applied.CardFolded[Target];
        const float Current  = Controls.OutlineExpansion(CardFolds[Target], !Folded, true);

        const float BodyHeight = (static_cast<float>(FieldCount) * Scaled.RowHeight + Pad * 2.0f) * Current;
        const PlaneExtent Card = Spanning(Extent.MinimumX + Pad, Sweep,
                                          Extent.Width() - Pad * 2.0f,
                                          Scaled.ComponentY + BodyHeight);

        Surface->Ground(Card, Covering(0x0A0A0Bu), Scaled.CardRadius, CornerAll);
        Surface->Edge(Card, Tinted.Hairline, 1.0f, Scaled.CardRadius, CornerAll);

        const PlaneExtent CardHeader = Spanning(Card.MinimumX, Card.MinimumY,
                                                Card.Width(), Scaled.ComponentY);

        Surface->Ground(CardHeader, Tinted.MenuLower, Scaled.CardRadius,
                        CornerLeadingUpper | CornerTrailingUpper);

        const bool OnHeader = CardHeader.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Sampled.ContactPressed && OnHeader && !Ledger->AnyDisclosed())
            Ledger->Grab(CardFolds[Target], ControlPart::Chevron);

        if (OnHeader && Ledger->Released(CardFolds[Target]))
            Applied.CardFolded[Target] = !Applied.CardFolded[Target];

        if (Current > 0.0f)
            Surface->Ground(Spanning(CardHeader.MinimumX, CardHeader.MaximumY - 1.0f,
                                     CardHeader.Width(), 1.0f),
                            Faded(Tinted.Hairline, Current), 0.0f, CornerNone);

        const float Mark = Scaled.ActionGlyph;

        Surface->Stroke(Folded ? SymbolSubject::ChevronRight : SymbolSubject::ChevronDown,
                        Spanning(CardHeader.MinimumX + Scaled.HeaderPadX * 0.6f,
                                 CardHeader.MinimumY + (CardHeader.Height() - Mark) * 0.5f,
                                 Mark, Mark), Tinted.Faint);

        const float CaptionRun = Scaled.RunSmall;

        Surface->TextRunCapitalised(CardHeader.MinimumX + Scaled.HeaderPadX * 0.6f + Mark + Pad,
                                    CardHeader.MinimumY + (CardHeader.Height() - CaptionRun) * 0.5f,
                                    OnHeader ? Tinted.Primary : Tinted.Muted, Caption, CaptionRun,
                                    0.025f, true);

        char Tallied[8] = {};
        std::snprintf(Tallied, sizeof(Tallied), "%u", static_cast<unsigned>(FieldCount));

        const float TallyRun = Scaled.RunFiner;

        Surface->TextRun(CardHeader.MaximumX - Scaled.HeaderPadX
                         - Surface->MeasureRun(Tallied, TallyRun, 0.0f),
                         CardHeader.MinimumY + (CardHeader.Height() - TallyRun) * 0.5f,
                         Tinted.Faint, Tallied, TallyRun);

        if (Current > 0.0f)
        {
            const PlaneExtent Opened = Spanning(Card.MinimumX, CardHeader.MaximumY,
                                                Card.Width(), BodyHeight);

            Surface->Confine(Opened);

            float RowCursor = CardHeader.MaximumY + Pad;

            for (std::uint32_t FieldOrdinal = 0u; FieldOrdinal < FieldCount; ++FieldOrdinal)
            {
                const PlaneExtent Row = Spanning(Card.MinimumX + Pad * 1.5f, RowCursor,
                                                 Card.Width() - Pad * 3.0f, Scaled.RowHeight);

                Surface->TextRun(Row.MinimumX + 2.0f,
                                 Row.MinimumY + (Row.Height() - Scaled.RunPrimary) * 0.5f,
                                 Tinted.Muted, Fields[FieldOrdinal], Scaled.RunPrimary);

                RowCursor += Scaled.RowHeight;
            }

            Surface->Release();
        }

        Sweep = Card.MaximumY + Pad * 0.85f;
    };

    if (RowCount == 0u || Applied.EntityTaken >= RowCount)
        return;

    const std::uint32_t Taken = Applied.EntityTaken;
    const EntityRow&    Current = Rows[Taken];

    const bool Transforms = Current.Subject != EntitySubject::Level
                         && Current.Subject != EntitySubject::Grouping
                         && Current.Subject != EntitySubject::Script;

    if (Transforms)
    {
        const char* const TransformRows[3] = { "Position", "Rotation", "Scale" };
        RecordCard("Transform", TransformRows, 3u);
    }

    char ComponentCaption[48] = {};
    std::snprintf(ComponentCaption, sizeof(ComponentCaption), "%s Component",
                  EntityText(Current.Subject));

    // 📐 The per-subject field sets, and the environment cards where the subject is the sun or the sky
    //    while the environment is presented.
    switch (Current.Subject)
    {
        case EntitySubject::Illuminant:
        {
            const char* const Fields[3] = { "Intensity", "Cast Shadows", "Light Color" };
            RecordCard(ComponentCaption, Fields, 3u);
            break;
        }
        case EntitySubject::Camera:
        {
            const char* const Fields[4] = { "Projection", "Field of View", "Near Clip", "Far Clip" };
            RecordCard(ComponentCaption, Fields, 4u);

            // 📝 The camera's own card — the fly speed, a live slider exactly like the environment's,
            //    with the same once-per-drag history demand against the camera row.
            const char* const CameraCaptions[1] = { "Fly Speed" };
            const char* const CameraUnits[1]    = { "m/s" };
            const double CameraMinimums[1]      = { 1.0 };
            const double CameraMaximums[1]      = { 500.0 };
            double CameraValues[1]              = { Applied.CameraSpeed };

            RecordEnvironmentCard(Applied, Extent, Sweep, CardOrdinal,
                                  "Camera", CameraCaptions, CameraUnits, CameraMinimums,
                                  CameraMaximums, CameraValues, 1u);

            Applied.CameraSpeed = CameraValues[0];
            break;
        }
        case EntitySubject::Audio:
        {
            const char* const Fields[3] = { "Volume", "Looping", "Spatial 3D" };
            RecordCard(ComponentCaption, Fields, 3u);
            break;
        }
        case EntitySubject::Particle:
        {
            const char* const Fields[3] = { "Emit Rate", "Life Time", "Looping" };
            RecordCard(ComponentCaption, Fields, 3u);
            break;
        }
        case EntitySubject::Trigger:
        {
            const char* const Fields[2] = { "Event Tag", "Radius" };
            RecordCard(ComponentCaption, Fields, 2u);
            break;
        }
        case EntitySubject::Script:
        {
            const char* const Fields[2] = { "State", "Difficulty" };
            RecordCard(ComponentCaption, Fields, 2u);
            break;
        }
        case EntitySubject::Actor:
        {
            const char* const Fields[3] = { "Static Mesh", "Simulate Physics", "Generate Overlaps" };
            RecordCard(ComponentCaption, Fields, 3u);
            break;
        }
        case EntitySubject::Level:
        {
            const char* const Fields[2] = { "Level Name", "World Partition" };
            RecordCard(ComponentCaption, Fields, 2u);
            break;
        }
        case EntitySubject::Sun:
        {
            // 📝 The sun card — the four ordinates the sky renderer reads, each a live slider. The card is
            //    drawn only while the environment is presented; a host that never presents it renders the
            //    reference's generic illuminant card instead.
            if (Applied.EnvironmentPresented)
            {
                const char* const SunCaptions[4] = { "Elevation", "Azimuth", "Intensity", "Temperature" };
                const char* const SunUnits[4]    = { "\u00B0", "\u00B0", "lx", "K" };
                const double SunMinimums[4]      = { 0.0, 0.0, 0.0, 1000.0 };
                const double SunMaximums[4]      = { 90.0, 360.0, 10.0, 12000.0 };
                double SunValues[4]              = { Applied.Environment.SunElevation,
                                                     Applied.Environment.SunAzimuth,
                                                     Applied.Environment.SunIntensity,
                                                     Applied.Environment.SunTemperature };

                RecordEnvironmentCard(Applied, Extent, Sweep, CardOrdinal,
                                      "Sun", SunCaptions, SunUnits, SunMinimums, SunMaximums,
                                      SunValues, 4u);

                Applied.Environment.SunElevation   = SunValues[0];
                Applied.Environment.SunAzimuth     = SunValues[1];
                Applied.Environment.SunIntensity   = SunValues[2];
                Applied.Environment.SunTemperature = SunValues[3];
            }
            else
            {
                const char* const Fields[3] = { "Intensity", "Cast Shadows", "Light Color" };
                RecordCard(ComponentCaption, Fields, 3u);
            }
            break;
        }
        case EntitySubject::Sky:
        {
            if (Applied.EnvironmentPresented)
            {
                const char* const SkyCaptions[2] = { "Sky Intensity", "Turbidity" };
                const char* const SkyUnits[2]    = { "", "" };
                const double SkyMinimums[2]      = { 0.0, 1.0 };
                const double SkyMaximums[2]      = { 3.0, 10.0 };
                double SkyValues[2]              = { Applied.Environment.SkyIntensity,
                                                     Applied.Environment.SkyTurbidity };

                RecordEnvironmentCard(Applied, Extent, Sweep, CardOrdinal,
                                      "Sky", SkyCaptions, SkyUnits, SkyMinimums, SkyMaximums,
                                      SkyValues, 2u);

                Applied.Environment.SkyIntensity = SkyValues[0];
                Applied.Environment.SkyTurbidity = SkyValues[1];

                const char* const AtmoCaptions[2] = { "Atmosphere Density", "Scale Height" };
                const char* const AtmoUnits[2]    = { "", "" };
                const double AtmoMinimums[2]      = { 0.0, 0.2 };
                const double AtmoMaximums[2]      = { 3.0, 3.0 };
                double AtmoValues[2]              = { Applied.Environment.AtmosphereDensity,
                                                      Applied.Environment.AtmosphereScaleHeight };

                RecordEnvironmentCard(Applied, Extent, Sweep, CardOrdinal,
                                      "Atmosphere", AtmoCaptions, AtmoUnits, AtmoMinimums,
                                      AtmoMaximums, AtmoValues, 2u);

                Applied.Environment.AtmosphereDensity     = AtmoValues[0];
                Applied.Environment.AtmosphereScaleHeight = AtmoValues[1];
            }
            else
            {
                const char* const Fields[3] = { "Intensity", "Cast Shadows", "Light Color" };
                RecordCard(ComponentCaption, Fields, 3u);
            }
            break;
        }
        default:
        {
            const char* const Fields[2] = { "Folder Name", "Is Editor Only" };
            RecordCard(ComponentCaption, Fields, 2u);
            break;
        }
    }

    // 🔴 Every card ordinal is spent whether the subject presented it or not. Skipped, a card that
    //    appears for one subject and not the next inherits the fold of whichever card held that ordinal
    //    before it, and the artist watches an unrelated card close.
    while (CardOrdinal < SceneDirectoryContext::CardCeiling)
        static_cast<void>(Controls.OutlineExpansion(CardFolds[CardOrdinal++], true, true));
}

void SceneDirectoryPanel::RecordEnvironmentCard(SceneDirectoryContext& Applied,
                                                const PlaneExtent& Extent, float& Sweep,
                                                std::uint32_t& CardOrdinal,
                                                const char* Caption,
                                                const char* const* SliderCaptions,
                                                const char* const* UnitGlyphs,
                                                const double* Minimums, const double* Maximums,
                                                double* Values, std::uint32_t SliderCount)
{
    if (CardOrdinal >= SceneDirectoryContext::CardCeiling)
        return;

    const float Pad = Scaled.PanePad;

    const std::uint32_t Target = CardOrdinal++;
    const bool  Folded   = Applied.CardFolded[Target];
    const float Current  = Controls.OutlineExpansion(CardFolds[Target], !Folded, true);

    const float BodyHeight = (static_cast<float>(SliderCount) * Scaled.RowHeight + Pad * 2.0f) * Current;
    const PlaneExtent Card = Spanning(Extent.MinimumX + Pad, Sweep,
                                      Extent.Width() - Pad * 2.0f,
                                      Scaled.ComponentY + BodyHeight);

    Surface->Ground(Card, Covering(0x0A0A0Bu), Scaled.CardRadius, CornerAll);
    Surface->Edge(Card, Tinted.Hairline, 1.0f, Scaled.CardRadius, CornerAll);

    const PlaneExtent CardHeader = Spanning(Card.MinimumX, Card.MinimumY,
                                            Card.Width(), Scaled.ComponentY);

    Surface->Ground(CardHeader, Tinted.MenuLower, Scaled.CardRadius,
                    CornerLeadingUpper | CornerTrailingUpper);

    const bool OnHeader = CardHeader.Encloses(Sampled.PositionX, Sampled.PositionY);

    if (Sampled.ContactPressed && OnHeader && !Ledger->AnyDisclosed())
        Ledger->Grab(CardFolds[Target], ControlPart::Chevron);

    if (OnHeader && Ledger->Released(CardFolds[Target]))
        Applied.CardFolded[Target] = !Applied.CardFolded[Target];

    if (Current > 0.0f)
        Surface->Ground(Spanning(CardHeader.MinimumX, CardHeader.MaximumY - 1.0f,
                                 CardHeader.Width(), 1.0f),
                        Faded(Tinted.Hairline, Current), 0.0f, CornerNone);

    const float Mark = Scaled.ActionGlyph;

    Surface->Stroke(Folded ? SymbolSubject::ChevronRight : SymbolSubject::ChevronDown,
                    Spanning(CardHeader.MinimumX + Scaled.HeaderPadX * 0.6f,
                             CardHeader.MinimumY + (CardHeader.Height() - Mark) * 0.5f,
                             Mark, Mark), Tinted.Faint);

    const float CaptionRun = Scaled.RunSmall;

    Surface->TextRunCapitalised(CardHeader.MinimumX + Scaled.HeaderPadX * 0.6f + Mark + Pad,
                                CardHeader.MinimumY + (CardHeader.Height() - CaptionRun) * 0.5f,
                                OnHeader ? Tinted.Primary : Tinted.Muted, Caption, CaptionRun,
                                0.025f, true);

    char Tallied[8] = {};
    std::snprintf(Tallied, sizeof(Tallied), "%u", static_cast<unsigned>(SliderCount));

    const float TallyRun = Scaled.RunFiner;

    Surface->TextRun(CardHeader.MaximumX - Scaled.HeaderPadX
                     - Surface->MeasureRun(Tallied, TallyRun, 0.0f),
                     CardHeader.MinimumY + (CardHeader.Height() - TallyRun) * 0.5f,
                     Tinted.Faint, Tallied, TallyRun);

    if (Current > 0.0f)
    {
        const PlaneExtent Opened = Spanning(Card.MinimumX, CardHeader.MaximumY,
                                            Card.Width(), BodyHeight);

        Surface->Confine(Opened);

        float RowCursor = CardHeader.MaximumY + Pad;

        for (std::uint32_t SliderOrdinal = 0u; SliderOrdinal < SliderCount; ++SliderOrdinal)
        {
            const PlaneExtent Row = Spanning(Card.MinimumX + Pad * 1.5f, RowCursor,
                                             Card.Width() - Pad * 3.0f, Scaled.RowHeight);

            MagnitudeDeclaration Declared;
            Declared.Caption     = SliderCaptions[SliderOrdinal];
            Declared.UnitGlyph   = UnitGlyphs[SliderOrdinal];
            Declared.Minimum     = Minimums[SliderOrdinal];
            Declared.Maximum     = Maximums[SliderOrdinal];

            double& Coordinate   = Values[SliderOrdinal];

            static_cast<void>(EnvironmentControls.MagnitudeRow(EnvironmentSliders[SliderOrdinal],
                                                               Row, Declared, Coordinate, true));

            // 🔴 The drag arm: latched the first tick the slider holds the contact, with the value at
            //    that moment — the "start" the history entry describes. Released with a changed value,
            //    one demand is raised; neither fires on the intermediate ticks.
            if (Ledger->Holding(EnvironmentSliders[SliderOrdinal]) && !EnvironmentArmed[SliderOrdinal])
            {
                EnvironmentArmed[SliderOrdinal] = true;
                EnvironmentFrom[SliderOrdinal]  = Coordinate;
            }

            if (Ledger->Released(EnvironmentSliders[SliderOrdinal]))
            {
                if (EnvironmentArmed[SliderOrdinal])
                {
                    if (std::abs(Coordinate - EnvironmentFrom[SliderOrdinal]) > 0.0005)
                    {
                        Applied.RevisionDemandSlot.Standing  = true;
                        Applied.RevisionDemandSlot.Against   = Applied.EntityTaken;
                        std::snprintf(Applied.RevisionDemandSlot.Caption,
                                      sizeof(Applied.RevisionDemandSlot.Caption), "%s",
                                      SliderCaptions[SliderOrdinal]);
                        std::snprintf(Applied.RevisionDemandSlot.Secondary,
                                      sizeof(Applied.RevisionDemandSlot.Secondary),
                                      "%.1f \u2192 %.1f",
                                      EnvironmentFrom[SliderOrdinal], Coordinate);
                    }

                    EnvironmentArmed[SliderOrdinal] = false;
                }
            }

            RowCursor += Scaled.RowHeight;
        }

        Surface->Release();
    }

    Sweep = Card.MaximumY + Pad * 0.85f;
}

void SceneDirectoryPanel::RecordRevisionSpine(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                                              const EntityRow* Rows, std::uint32_t RowCount,
                                              const EntityRevision* Revisions, std::uint32_t RevisionCount)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    if (Revisions == nullptr)
        RevisionCount = 0u;

    const bool Selected = Applied.EntityTaken < RowCount;

    // 📐 The reference gathers the taken record AND everything nested inside it, so a folder presents
    //    its children's revisions too.
    std::uint32_t Minimum = Applied.EntityTaken;
    std::uint32_t Maximum  = Applied.EntityTaken;

    if (Selected)
    {
        for (std::uint32_t Inward = Applied.EntityTaken + 1u; Inward < RowCount; ++Inward)
        {
            if (Rows[Inward].Depth <= Rows[Applied.EntityTaken].Depth)
                break;

            Maximum = Inward;
        }
    }

    std::uint32_t Current = 0u;

    if (Selected)
    {
        for (std::uint32_t Ordinal = 0u; Ordinal < RevisionCount; ++Ordinal)
        {
            if (Revisions[Ordinal].Against >= Minimum && Revisions[Ordinal].Against <= Maximum)
                ++Current;
        }
    }

    if (!Selected || Current == 0u)
    {
        const float Run   = Scaled.RunSecondary;
        const char* Prose = "No history events found for this selection or its children.";

        Surface->TextRun(Extent.MinimumX + (Extent.Width()
                                              - Surface->MeasureRun(Prose, Run, 0.0f)) * 0.5f,
                         Extent.MinimumY + Scaled.HeaderHeight, Tinted.Faint, Prose, Run);
        return;
    }

    const float Pad    = Scaled.PanePad;
    float       Sweep = Extent.MinimumY + Pad;

    Surface->Confine(Extent);

    // 📐 One group per record that carries a revision, in the run's own order.
    for (std::uint32_t Against = Minimum; Against <= Maximum; ++Against)
    {
        std::uint32_t Held = 0u;

        for (std::uint32_t Ordinal = 0u; Ordinal < RevisionCount; ++Ordinal)
        {
            if (Revisions[Ordinal].Against == Against)
                ++Held;
        }

        if (Held == 0u)
            continue;

        const EntityRow&  Grouped = Rows[Against];
        const ThemeToken Hue     = EntityHue(Grouped.Subject);

        // ① The group header, which folds the whole group.
        const PlaneExtent GroupHead = Spanning(Extent.MinimumX + Pad, Sweep,
                                               Extent.Width() - Pad * 2.0f, Scaled.RowHeight);

        const bool OnHead = GroupHead.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Sampled.ContactPressed && OnHead && !Ledger->AnyDisclosed())
            Ledger->Grab(RevisionGroups[Against], ControlPart::Chevron);

        if (OnHead && Ledger->Released(RevisionGroups[Against]))
            Applied.RevisionFolded[Against] = !Applied.RevisionFolded[Against];

        const bool  GroupFolded = Applied.RevisionFolded[Against];
        const float Opened      = Controls.OutlineExpansion(RevisionGroups[Against], !GroupFolded, true);

        const float CrestExtent = Scaled.ActionGlyph + 5.0f;
        const PlaneExtent Crest = Spanning(GroupHead.MinimumX,
                                           GroupHead.MinimumY
                                           + (GroupHead.Height() - CrestExtent) * 0.5f,
                                           CrestExtent, CrestExtent);

        Surface->Ground(Crest, Hue, 5.0f, CornerAll);

        const float CrestFigure = CrestExtent * 0.6f;

        Surface->Stroke(EntityGlyph(Grouped.Subject),
                        Spanning(Crest.MinimumX + (CrestExtent - CrestFigure) * 0.5f,
                                 Crest.MinimumY + (CrestExtent - CrestFigure) * 0.5f,
                                 CrestFigure, CrestFigure),
                        Covering(0xFFFFFFu));

        const float NameRun = Scaled.RunPrimary;
        const float NameTop = GroupHead.MinimumY + (GroupHead.Height() - NameRun) * 0.5f;

        // 📐 `{n} ops` at the trailing edge, and the chevron outboard of it.
        char Tallied[16] = {};
        std::snprintf(Tallied, sizeof(Tallied), "%u ops", static_cast<unsigned>(Held));

        const float TallyRun  = Scaled.RunFine;
        const float Mark      = Scaled.ActionGlyph;
        const float TallyLead = GroupHead.MaximumX - Mark - Pad
                              - Surface->MeasureRun(Tallied, TallyRun, 0.0f);

        Surface->TextRun(TallyLead, GroupHead.MinimumY + (GroupHead.Height() - TallyRun) * 0.5f,
                         Tinted.Muted, Tallied, TallyRun);

        Surface->Stroke(GroupFolded ? SymbolSubject::ChevronRight : SymbolSubject::ChevronDown,
                        Spanning(GroupHead.MaximumX - Mark,
                                 GroupHead.MinimumY + (GroupHead.Height() - Mark) * 0.5f,
                                 Mark, Mark),
                        Tinted.Faint);

        Surface->TextRunTruncated(Crest.MaximumX + Pad, NameTop, TallyLead - Pad,
                                  OnHead ? Covering(0xFFFFFFu) : Tinted.Primary,
                                  Grouped.Naming, NameRun, true);

        Sweep += Scaled.RowHeight + 4.0f;

        if (Opened <= 0.0f)
            continue;

        // ② The revisions themselves — a numbered bubble, the spine, and the card beside them.
        const float CardHeight  = Scaled.LayerHeadHeight;
        const float WholeHeight = static_cast<float>(Held) * (CardHeight + 4.0f) * Opened;
        const PlaneExtent Stack = Spanning(Extent.MinimumX, Sweep, Extent.Width(), WholeHeight);

        Surface->Confine(Stack);

        float         X        = Sweep;
        std::uint32_t Numbered = 0u;

        for (std::uint32_t Ordinal = 0u; Ordinal < RevisionCount; ++Ordinal)
        {
            const EntityRevision& Revised = Revisions[Ordinal];

            if (Revised.Against != Against)
                continue;

            const bool First = Numbered == 0u;
            const bool Last  = Numbered + 1u == Held;

            const float BubbleExtent = 25.0f;
            const float BubbleLead   = Extent.MinimumX + Pad
                                     + (32.0f - BubbleExtent) * 0.5f;
            const float BubbleMid    = X + 7.0f + BubbleExtent * 0.5f;

            // 📐 The spine, stopping half way at the first and last of the group so the run reads as a
            //    bracket — the same rule the shell's layer stack follows.
            const float SpineMid  = Extent.MinimumX + Pad + 32.0f + 15.0f * 0.5f;
            const float SpineTop  = First ? BubbleMid : X;
            const float SpineFoot = Last  ? BubbleMid : X + CardHeight + 4.0f;

            if (SpineFoot > SpineTop)
            {
                Surface->Ground(Spanning(SpineMid - 3.0f, SpineTop, 6.0f, SpineFoot - SpineTop),
                                Hue, 4.0f, CornerAll);
            }

            Surface->Ground(Spanning(BubbleLead, X + 7.0f, BubbleExtent, BubbleExtent),
                            Hue, BubbleExtent * 0.5f, CornerAll);

            char Counted[4] = {};
            std::snprintf(Counted, sizeof(Counted), "%02u", static_cast<unsigned>(Numbered));

            const float CountRun = Scaled.RunFine;

            Surface->TextRun(BubbleLead + (BubbleExtent
                                           - Surface->MeasureRun(Counted, CountRun, 0.0f)) * 0.5f,
                             X + 7.0f + (BubbleExtent - CountRun) * 0.5f,
                             Covering(0xFFFFFFu), Counted, CountRun, 0.0f, true);

            // 📐 The 7 px node, ringed by 3 px of the pane's own ground.
            Surface->Medallion(SpineMid, BubbleMid, 6.5f, Tinted.MenuLower);
            Surface->Medallion(SpineMid, BubbleMid, 3.5f, Covering(0xFFFFFFu));

            const PlaneExtent Card = Spanning(SpineMid + 15.0f * 0.5f + 8.0f, X,
                                              Extent.MaximumX - Pad
                                              - (SpineMid + 15.0f * 0.5f + 8.0f), CardHeight);

            const bool OnCard = Card.Encloses(Sampled.PositionX, Sampled.PositionY);

            Surface->Ground(Card, OnCard ? Tinted.TileHovered : Tinted.Tile, Scaled.LayerRadius, CornerAll);
            Surface->Edge(Card, Tinted.Hairline, 1.0f, Scaled.LayerRadius, CornerAll);

            const RevisionDeclaration Declared{ Revised.Description, Revised.Secondary, Revised.TimeRun };

            Controls.RevisionRow(Card, Declared, OnCard);

            X        += CardHeight + 4.0f;
            Numbered += 1u;
        }

        Surface->Release();

        Sweep += WholeHeight + Pad * 2.0f;
    }

    Surface->Release();
}

}   // namespace Slate
