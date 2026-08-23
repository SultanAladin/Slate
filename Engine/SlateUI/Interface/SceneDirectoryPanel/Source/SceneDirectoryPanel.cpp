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

/// 🧩 One tone travelling toward another, for a hover that grows rather than switching.
constexpr std::uint8_t BlendChannel(std::uint8_t Previous, std::uint8_t Incoming, float Fraction)
{
    return static_cast<std::uint8_t>(static_cast<float>(Previous) +
                                     (static_cast<float>(Incoming) -
                                      static_cast<float>(Previous)) * Fraction + 0.5f);
}

constexpr ThemeToken Blend(ThemeToken Previous, ThemeToken Incoming, float Fraction)
{
    const float Bounded = (Fraction < 0.0f) ? 0.0f : (Fraction > 1.0f) ? 1.0f : Fraction;

    return ThemeToken{ BlendChannel(Previous.Red,     Incoming.Red,     Bounded),
                       BlendChannel(Previous.Green,   Incoming.Green,   Bounded),
                       BlendChannel(Previous.Blue,    Incoming.Blue,    Bounded),
                       BlendChannel(Previous.Opacity, Incoming.Opacity, Bounded) };
}

/// 🧩 The same colour at a declared fraction of its own coverage.
constexpr ThemeToken Faded(ThemeToken Declared, float Fraction)
{
    const float Bounded = Held(Fraction, 0.0f, 1.0f);
    Declared.Opacity    = static_cast<std::uint8_t>(static_cast<float>(Declared.Opacity) * Bounded + 0.5f);
    return Declared;
}

/// 🧩 Whether one run holds another as a case-insensitive subsequence — the reference's own
///    `name.toLowerCase().includes(filterText.toLowerCase())`.
bool RunHolds(const char* Subject, const char* Sought)
{
    if (Sought == nullptr || Sought[0] == '\0')
        return true;

    if (Subject == nullptr)
        return false;

    const auto Lowered = [](char Letter) -> char
    {
        return (Letter >= 'A' && Letter <= 'Z') ? static_cast<char>(Letter - 'A' + 'a') : Letter;
    };

    for (std::uint32_t Departure = 0u; Subject[Departure] != '\0'; ++Departure)
    {
        std::uint32_t Advanced = 0u;

        while (Sought[Advanced] != '\0' &&
               Lowered(Subject[Departure + Advanced]) == Lowered(Sought[Advanced]))
        {
            ++Advanced;
        }

        if (Sought[Advanced] == '\0')
            return true;
    }

    return false;
}

// 📐 The editor's filter categories, and the subject each entity maps to. The category ordinals are
//    the FacetPanel's option ordinals; `EditorFacetOf` is what makes a facet a filter rather than a
//    label — a row is shown only when its category's facet is enabled.
constexpr std::uint32_t EditorFacetCount = 9u;   // [-] - mirrors SceneDirectoryContext::FacetCount

const char* const EditorFacetOptions[EditorFacetCount] =
{
    "Objects", "Lights", "Cameras", "Folders", "Audio",
    "Particles", "Triggers", "Environment", "Layers"
};

const ThemeToken EditorFacetColours[EditorFacetCount] =
{
    Covering(0x3B82F6u),   // [-] - Objects, blue
    Covering(0xF59E0Bu),   // [-] - Lights, amber
    Covering(0xEC4899u),   // [-] - Cameras, pink
    Covering(0x8A8A8Au),   // [-] - Folders, grey
    Covering(0x8B5CF6u),   // [-] - Audio, violet
    Covering(0x10B981u),   // [-] - Particles, green
    Covering(0xEF4444u),   // [-] - Triggers, red
    Covering(0x38BDF8u),   // [-] - Environment, cyan
    Covering(0x14B8A6u)    // [-] - Layers, teal
};

std::uint32_t EditorFacetOf(EntitySubject Subject)
{
    switch (Subject)
    {
        case EntitySubject::Level:
        case EntitySubject::Grouping: return 3u;   // [-] - Folders
        case EntitySubject::Actor:
        case EntitySubject::Script:   return 0u;   // [-] - Objects
        case EntitySubject::Camera:   return 2u;   // [-] - Cameras
        case EntitySubject::Illuminant:
        case EntitySubject::Sun:      return 1u;   // [-] - Lights
        case EntitySubject::Audio:    return 4u;   // [-] - Audio
        case EntitySubject::Particle: return 5u;   // [-] - Particles
        case EntitySubject::Trigger:  return 6u;   // [-] - Triggers
        case EntitySubject::Sky:      return 7u;   // [-] - Environment
        default:                      return 0u;
    }
}

/// 🧩 Whether the search and the facets jointly retain one row.
bool RowRetained(const SceneDirectoryContext& Applied, const EntityRow& Row)
{
    const bool Searching = Applied.EntityRetention[0] != '\0';

    if (Searching)
    {
        if (!RunHolds(Row.Naming, Applied.EntityRetention) &&
            !RunHolds(Row.Tagged, Applied.EntityRetention))
            return false;
    }

    for (std::uint32_t Facet = 0u; Facet < SceneDirectoryContext::FacetCount; ++Facet)
    {
        if (Applied.FacetEnabled[Facet])
            return Applied.FacetEnabled[EditorFacetOf(Row.Subject)];
    }

    return true;
}

/// 🧩 Whether the search or any facet is active at all.
bool RetentionActive(const SceneDirectoryContext& Applied)
{
    if (Applied.EntityRetention[0] != '\0')
        return true;

    for (std::uint32_t Facet = 0u; Facet < SceneDirectoryContext::FacetCount; ++Facet)
    {
        if (Applied.FacetEnabled[Facet])
            return true;
    }

    return false;
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
    if (!Facets.Construct(Integrator, Surface, Resolved).Resolved)
    {
        Reset();
        return Outcome<bool>::Refuse({ RefusalReason::CapabilityAbsent,
                                       "the scene directory filter was rejected" });
    }

    ControlIdentity* const Every[] =
    {
        &InspectorStrip,
        &OutlineStrip,
        &InspectCall,
        &DirectoryCall,
        &TransferArrows[0], &TransferArrows[1],
        &TransferChoices[0], &TransferChoices[1], &TransferChoices[2], &TransferChoices[3],
        &TransferChoices[4], &TransferChoices[5], &TransferChoices[6], &TransferChoices[7],
        &TransferChoices[8], &TransferChoices[9],
        &BookmarkSave,
        &BookmarkRecall,
        &BookmarkRetire,
        &SearchField,
        &EnvironmentSliders[0], &EnvironmentSliders[1], &EnvironmentSliders[2],
        &EnvironmentSliders[3], &EnvironmentSliders[4], &EnvironmentSliders[5],
        &CardFolds[0], &CardFolds[1], &CardFolds[2], &CardFolds[3],
        // 🔴 A control that is never registered resolves to nothing, so its row
        //    would draw but refuse every contact — the axis would look editable
        //    and silently ignore the drag.
        &CardFields[0][0], &CardFields[0][1], &CardFields[0][2], &CardFields[0][3],
        &CardFields[1][0], &CardFields[1][1], &CardFields[1][2], &CardFields[1][3],
        &CardFields[2][0], &CardFields[2][1], &CardFields[2][2], &CardFields[2][3],
        &CardFields[3][0], &CardFields[3][1], &CardFields[3][2], &CardFields[3][3]
    };

    for (ControlIdentity* Identity : Every)
    {
        const Outcome<ControlIdentity> Registered = Interaction.Register();
        if (!Registered.Resolved)
            return Outcome<bool>::Refuse(Registered.Error);

        *Identity = Registered.Resolve();
    }

    for (ControlIdentity& Identity : BookmarkNames)
    {
        const Outcome<ControlIdentity> Registered = Interaction.Register();
        if (!Registered.Resolved)
            return Outcome<bool>::Refuse(Registered.Error);
        Identity = Registered.Resolve();
    }

    // 📐 The leaf's page travel. Registered here, never mid-tick.
    {
        const Outcome<std::uint32_t> Eased = Integrator.RegisterEased(1.0);

        if (!Eased.Resolved)
            return Outcome<bool>::Refuse(Eased.Error);

        OutlineMotion = Eased.Resolve();
    }

    {
        const Outcome<std::uint32_t> Eased = Integrator.RegisterEased(1.0);
        if (!Eased.Resolved)
            return Outcome<bool>::Refuse(Eased.Error);
        TransferMotion = Eased.Resolve();
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < 2u; ++Ordinal)
    {
        const Outcome<std::uint32_t> Eased = Integrator.RegisterEased(1.0);

        if (!Eased.Resolved)
            return Outcome<bool>::Refuse(Eased.Error);

        InspectorMotion[Ordinal] = Eased.Resolve();
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

void SceneDirectoryPanel::Advance(const PointerCondition& Contact, double Elapsed,
                                   SceneDirectoryContext& Applied, bool TabPressed)
{
    Sampled = Contact;
    Controls.Advance(Contact, Elapsed);
    // 📝 Sampled, never advanced: the tick owner advances the shared ledger exactly once, and a
    //    second advance would retire the release before the panel reads it.
    EnvironmentControls.Sample(Contact);
    Facets.Advance(Contact, Elapsed);

    // 📐 Two outer slides, with Properties and History nested inside the inspector. The old three-page
    //    outer state mapped both inspector destinations to the same -width coordinate, so its second
    //    transition had zero distance and appeared broken.
    if (TabPressed)
    {
        if (Applied.OutlinePage == 0u)
        {
            Applied.OutlinePage = 1u;
            Applied.OutlineInspectorTab = 0u;
        }
        else if (Applied.OutlineInspectorTab == 0u)
        {
            Applied.OutlineInspectorTab = 1u;
        }
        else
        {
            Applied.OutlinePage = 0u;
            Applied.OutlineInspectorTab = 0u;
        }
    }

    // 📝 The search field's taken state, reported to the host so it feeds the seam's typed run only
    //    while the field actually holds the contact — the validation shell's filter captured every
    //    keystroke unconditionally, which is the "search box not working" a gate fixes.
    Applied.SearchTaken = Ledger->Holding(SearchField) || Ledger->Disclosed(SearchField);
}

void SceneDirectoryPanel::Reset()
{
    Controls.Reset();
    Facets.Reset();

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

    // 📐 The dome is direction-indexed, and the viewport reads it through a PERSPECTIVE mesh rather
    //    than a single cropped quad: a quad maps azimuth and elevation linearly onto the leaf, which
    //    stretches the sun into an ellipse the moment the leaf's aspect differs from the camera's and
    //    compresses the horizon where perspective should widen it. The mesh samples the dome per
    //    screen vertex along the true pinhole ray, so the sun stays round and the horizon reads at any
    //    leaf aspect — the same projection the grid below uses, which is what keeps the two aligned.
    constexpr std::uint32_t MeshColumns = 64u;
    constexpr std::uint32_t MeshRows    = 36u;
    constexpr std::uint32_t QuadCount   = MeshColumns * MeshRows;
    constexpr std::uint32_t VertexCount = QuadCount * 4u;
    constexpr std::uint32_t IndexCount  = QuadCount * 6u;

    const float CentreX = Extent.MinimumX + Extent.Width()  * 0.5f;
    const float CentreY = Extent.MinimumY + Extent.Height() * 0.5f;

    const double HalfV = Applied.ViewportSkyCamera.FieldOfViewDegrees * 0.5 * 3.14159265358979323846 / 180.0;
    const double Aspect = static_cast<double>(Extent.Width()) / static_cast<double>(Extent.Height());
    const double TanHalfV = std::tan(HalfV);
    const double TanHalfH = TanHalfV * Aspect;

    const double Yaw   = Applied.ViewportSkyCamera.AzimuthDegrees   * 3.14159265358979323846 / 180.0;
    const double Pitch = Applied.ViewportSkyCamera.ElevationDegrees * 3.14159265358979323846 / 180.0;
    const double CosP = std::cos(Pitch);
    const double SinP = std::sin(Pitch);
    const double SinY = std::sin(Yaw);
    const double CosY = std::cos(Yaw);

    // 📐 The camera basis, the same convention CameraComponent integrates: forward along the view, right
    //    across it, up the cross product.
    const double Forward[3] = { CosP * SinY, SinP, CosP * CosY };
    const double Right[3]   = { CosY, 0.0, -SinY };
    const double Up[3]      = { -SinP * SinY, CosP, -SinP * CosY };

    float Positions[VertexCount * 2u];
    float UVs[VertexCount * 2u];
    std::uint32_t Indices[IndexCount];

    std::uint32_t VertexOffset = 0u;
    std::uint32_t IndexOffset  = 0u;

    for (std::uint32_t Row = 0u; Row < MeshRows; ++Row)
    {
        const float RowY0 = Extent.MinimumY + static_cast<float>(Row)       / static_cast<float>(MeshRows) * Extent.Height();
        const float RowY1 = Extent.MinimumY + static_cast<float>(Row + 1u)  / static_cast<float>(MeshRows) * Extent.Height();

        for (std::uint32_t Column = 0u; Column < MeshColumns; ++Column)
        {
            const float ColX0 = Extent.MinimumX + static_cast<float>(Column)      / static_cast<float>(MeshColumns) * Extent.Width();
            const float ColX1 = Extent.MinimumX + static_cast<float>(Column + 1u) / static_cast<float>(MeshColumns) * Extent.Width();

            const float CornerScreenX[4] = { ColX0, ColX1, ColX0, ColX1 };
            const float CornerScreenY[4] = { RowY0, RowY0, RowY1, RowY1 };

            float CornerU[4];
            float CornerV[4];

            for (std::uint32_t Corner = 0u; Corner < 4u; ++Corner)
            {
                const double NdcX = (static_cast<double>(CornerScreenX[Corner]) - CentreX) / (Extent.Width()  * 0.5);
                const double NdcY = (static_cast<double>(CentreY) - CornerScreenY[Corner]) / (Extent.Height() * 0.5);

                double Ray[3] = { NdcX * TanHalfH, NdcY * TanHalfV, 1.0 };
                const double Length = std::sqrt(Ray[0] * Ray[0] + Ray[1] * Ray[1] + Ray[2] * Ray[2]);
                Ray[0] /= Length;
                Ray[1] /= Length;
                Ray[2] /= Length;

                const double DirectionX = Right[0] * Ray[0] + Up[0] * Ray[1] + Forward[0] * Ray[2];
                const double DirectionY = Right[1] * Ray[0] + Up[1] * Ray[1] + Forward[1] * Ray[2];
                const double DirectionZ = Right[2] * Ray[0] + Up[2] * Ray[1] + Forward[2] * Ray[2];

                const double Azimuth   = std::atan2(DirectionX, DirectionZ);
                const double Elevation = std::asin(std::clamp(DirectionY, -1.0, 1.0));

                CornerU[Corner] = static_cast<float>(Azimuth / (2.0 * 3.14159265358979323846) + 0.5);
                CornerV[Corner] = static_cast<float>(std::clamp(0.5 - Elevation / 3.14159265358979323846, 0.0, 1.0));
            }

            // 📐 Locally unwrap U coordinates for this quad relative to Corner 0 so that no quad
            //    interpolates across the ±180° azimuth seam backwards. Being per-quad local, this
            //    completely prevents branch-cut line artifacts near the zenith / pole.
            for (std::uint32_t Corner = 1u; Corner < 4u; ++Corner)
            {
                while (CornerU[Corner] - CornerU[0] > 0.5f)
                    CornerU[Corner] -= 1.0f;
                while (CornerU[Corner] - CornerU[0] < -0.5f)
                    CornerU[Corner] += 1.0f;
            }

            const std::uint32_t Base = VertexOffset;

            for (std::uint32_t Corner = 0u; Corner < 4u; ++Corner)
            {
                Positions[(VertexOffset + Corner) * 2u]      = CornerScreenX[Corner];
                Positions[(VertexOffset + Corner) * 2u + 1u]  = CornerScreenY[Corner];
                UVs[(VertexOffset + Corner) * 2u]            = CornerU[Corner];
                UVs[(VertexOffset + Corner) * 2u + 1u]        = CornerV[Corner];
            }
            VertexOffset += 4u;

            Indices[IndexOffset++] = Base + 0u;
            Indices[IndexOffset++] = Base + 2u;
            Indices[IndexOffset++] = Base + 1u;
            Indices[IndexOffset++] = Base + 1u;
            Indices[IndexOffset++] = Base + 2u;
            Indices[IndexOffset++] = Base + 3u;
        }
    }

    Surface->ImageMesh(Applied.SkyTextureIdentity, Positions, UVs, VertexCount, Indices, IndexCount);
}


// 🔴 RecordGroundGrid is withdrawn. The ground lattice is solved per pixel
//    by OverlayFragment.slang mode 3 — see the note in OverlayFragment.slang.
//    The host pushes the camera to the overlay pass through OverlayGroundPose
//    instead of handing it screen-space line segments.

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

void SceneDirectoryPanel::RecordTransfer(const PlaneExtent& Extent, SceneDirectoryContext& Applied)
{
    Surface->Ground(Extent, Tinted.Menu, 0.0f, CornerNone);
    const float Pad = Scaled.PanePad;
    const PlaneExtent Header = Spanning(Extent.MinimumX, Extent.MinimumY, Extent.Width(), Scaled.HeaderHeight);
    RecordLeafHeader(Header, SymbolSubject::FolderClosed, Tinted.EntityAccent,
                     Applied.TransferMode == 0u ? "Import Scene" : "Export Scene",
                     "Scene transfer setup");

    const PlaneExtent Back = Spanning(Extent.MinimumX + Pad, Header.MaximumY + Pad, 82.0f, 28.0f);
    const bool OnBack = Back.Encloses(Sampled.PositionX, Sampled.PositionY);
    if (Sampled.ContactPressed && OnBack && !Ledger->AnyDisclosed()) Ledger->Grab(DirectoryCall, ControlPart::Body);
    if (OnBack && Ledger->Released(DirectoryCall)) Applied.OutlinePage = 0u;
    Ledger->DeclareHovered(DirectoryCall, OnBack, HoverOver);
    Surface->Ground(Back, OnBack ? Tinted.TileHovered : Tinted.Tile, 14.0f, CornerAll);
    Surface->Edge(Back, Tinted.HairlineFirm, 1.0f, 14.0f, CornerAll);
    Surface->TextRun(Back.MinimumX + 18.0f, Back.MinimumY + 7.0f, Tinted.Primary, "Back", Scaled.RunSecondary);

    float Y = Back.MaximumY + 28.0f;
    Surface->TextRun(Extent.MinimumX + Pad, Y, Tinted.Primary,
                     Applied.TransferMode == 0u ? "Import format" : "Export format", Scaled.RunPrimary);
    Y += 28.0f;

    static const char* const ImportFormats[] =
        { "FBX", "glTF", "GLB", "OBJ", "USD", "USDZ", "DAE", "STL", "PLY", "ABC" };
    static const char* const ExportFormats[] =
        { "FBX", "glTF", "GLB", "OBJ", "USD", "USDZ", "DAE", "STL", "ABC" };
    const char* const* Formats = Applied.TransferMode == 0u ? ImportFormats : ExportFormats;
    const std::uint32_t FormatCount = Applied.TransferMode == 0u ? 10u : 9u;
    if (Applied.TransferFormat >= FormatCount)
        Applied.TransferFormat = FormatCount - 1u;

    const PlaneExtent Left = Spanning(Extent.MinimumX + Pad, Y + 20.0f, 42.0f, 42.0f);
    const PlaneExtent Right = Spanning(Extent.MaximumX - Pad - 42.0f, Y + 20.0f, 42.0f, 42.0f);
    const PlaneExtent Rail = { Left.MaximumX + 12.0f, Y, Right.MinimumX - 12.0f, Y + 84.0f };
    const double Fraction = Motion->Eased(TransferMotion).Current();
    const double Scroll = TransferFrom + (TransferTarget - TransferFrom) * Fraction;

    const auto Arrow = [&](std::uint32_t Ordinal, const PlaneExtent& Cell, const char* Mark)
    {
        const bool On = Cell.Encloses(Sampled.PositionX, Sampled.PositionY);
        if (Sampled.ContactPressed && On && !Ledger->AnyDisclosed()) Ledger->Grab(TransferArrows[Ordinal], ControlPart::Body);
        if (On && Ledger->Released(TransferArrows[Ordinal]))
        {
            if (Ordinal == 0u && Applied.TransferFormat > 0u) --Applied.TransferFormat;
            if (Ordinal == 1u && Applied.TransferFormat + 1u < FormatCount) ++Applied.TransferFormat;
            TransferFrom = Scroll;
            TransferTarget = static_cast<double>(Applied.TransferFormat) * 144.0;
            Motion->Eased(TransferMotion).Depart(0.0, 1.0, 250.0, 0.0, EaseCurve::Carousel);
        }
        Ledger->DeclareHovered(TransferArrows[Ordinal], On, HoverOver);
        Surface->Ground(Cell, On ? Tinted.TileHovered : Tinted.Tile, 21.0f, CornerAll);
        Surface->Edge(Cell, Tinted.HairlineFirm, 1.0f, 21.0f, CornerAll);
        Surface->TextRun(Cell.MinimumX + 16.0f, Cell.MinimumY + 11.0f, Tinted.Primary, Mark, Scaled.RunPrimary);
    };
    Arrow(0u, Left, "<");
    Arrow(1u, Right, ">");

    Surface->Confine(Rail);
    for (std::uint32_t Ordinal = 0u; Ordinal < FormatCount; ++Ordinal)
    {
        const PlaneExtent Choice = Spanning(Rail.MinimumX + 4.0f + Ordinal * 144.0f - static_cast<float>(Scroll),
                                            Rail.MinimumY, 132.0f, 80.0f);
        const bool On = Choice.Encloses(Sampled.PositionX, Sampled.PositionY);
        if (Sampled.ContactPressed && On && !Ledger->AnyDisclosed()) Ledger->Grab(TransferChoices[Ordinal], ControlPart::Body);
        if (On && Ledger->Released(TransferChoices[Ordinal]))
        {
            Applied.TransferFormat = Ordinal;
            TransferFrom = Scroll;
            TransferTarget = static_cast<double>(Ordinal) * 144.0;
            Motion->Eased(TransferMotion).Depart(0.0, 1.0, 250.0, 0.0, EaseCurve::Carousel);
        }
        Ledger->DeclareHovered(TransferChoices[Ordinal], On, HoverOver);
        const bool Taken = Applied.TransferFormat == Ordinal;
        Surface->Ground(Choice, Taken ? Tinted.RowTaken : (On ? Tinted.TileHovered : Tinted.Tile), 12.0f, CornerAll);
        Surface->Edge(Choice, Taken ? Tinted.EntityAccent : Tinted.Hairline, 1.0f, 12.0f, CornerAll);
        Surface->TextRun(Choice.MinimumX + 16.0f, Choice.MinimumY + 16.0f, Tinted.Primary, Formats[Ordinal], Scaled.RunPrimary);
        Surface->TextRun(Choice.MinimumX + 16.0f, Choice.MinimumY + 48.0f, Tinted.Muted,
                         Applied.TransferMode == 0u ? "Scene input" : "Scene output", Scaled.RunFine);
    }
    Surface->Release();

    Y += 112.0f;
    Surface->TextRun(Extent.MinimumX + Pad, Y, Tinted.Muted,
                     Applied.TransferMode == 0u ? "Choose a scene format to import." : "Choose a scene format to export.",
                     Scaled.RunSecondary);
    Surface->TextRun(Extent.MinimumX + Pad, Y + 24.0f, Tinted.Faint,
                     "File selection and transfer will be connected in a later increment.", Scaled.RunFine);
}

void SceneDirectoryPanel::RecordOutliner(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                                         const EntityRow* Rows, std::uint32_t RowCount,
                                         const EntityRevision* Revisions, std::uint32_t RevisionCount)
{
    if (Rows == nullptr)
        RowCount = 0u;

    if (RowCount > SceneDirectoryContext::EntityCeiling)
        RowCount = SceneDirectoryContext::EntityCeiling;

    if (Revisions == nullptr)
        RevisionCount = 0u;

    Surface->Ground(Extent, Tinted.Menu, 0.0f, CornerNone);

    const float Pad = Scaled.PanePad;

    // 📐 One three-page carousel: Directory + Details leads, the Properties | History inspector trails.
    //    Both pages are always positioned from the same carried coordinate, so departure and arrival
    //    remain visible throughout travel in either direction.
    if (Applied.OutlinePage != OutlineArriving)
    {
        OutlineDeparted = OutlineArriving;
        OutlineArriving = Applied.OutlinePage;
        Motion->Eased(OutlineMotion).Depart(0.0, 1.0, 260.0, 0.0, EaseCurve::Carousel);
    }

    const float Travelled  = static_cast<float>(Motion->Eased(OutlineMotion).Current());
    const float DepartedAt = -static_cast<float>(OutlineDeparted) * Extent.Width();
    const float ArrivingAt = -static_cast<float>(OutlineArriving) * Extent.Width();
    const float Carried    = DepartedAt + (ArrivingAt - DepartedAt) * Travelled;

    const PlaneExtent DirectoryExtent = Spanning(Extent.MinimumX + Carried, Extent.MinimumY,
                                                  Extent.Width(), Extent.Height());
    const PlaneExtent InspectorExtent = Spanning(DirectoryExtent.MaximumX, Extent.MinimumY,
                                                  Extent.Width(), Extent.Height());
    const PlaneExtent TransferExtent = Spanning(InspectorExtent.MaximumX, Extent.MinimumY,
                                                 Extent.Width(), Extent.Height());

    if (!Surface->Excluded(TransferExtent))
    {
        Surface->Confine(Extent);
        RecordTransfer(TransferExtent, Applied);
        Surface->Release();
    }

    if (!Surface->Excluded(InspectorExtent))
    {
        Surface->Confine(Extent);
        RecordProperties(InspectorExtent, Applied, Rows, RowCount, Revisions, RevisionCount,
                         Applied.OutlineInspectorTab, true);
        Surface->Release();
    }

    if (Surface->Excluded(DirectoryExtent))
        return;

    Surface->Confine(Extent);

    // 📐 The directory and its immediate details use the validation drafting split, constrained to 60%
    //    on narrow leaves so both panes remain readable.
    const float OutlinerX = (Scaled.OutlinerX < DirectoryExtent.Width() * 0.6f)
                          ? Scaled.OutlinerX : DirectoryExtent.Width() * 0.6f;

    const PlaneExtent Outlining = Spanning(DirectoryExtent.MinimumX, DirectoryExtent.MinimumY,
                                           OutlinerX, DirectoryExtent.Height());

    const PlaneExtent Header = Spanning(Outlining.MinimumX, Outlining.MinimumY,
                                        Outlining.Width(), Scaled.HeaderHeight);

    RecordLeafHeader(Header, SymbolSubject::GearCog, Tinted.EntityAccent, "Scene Directory",
                     "Document Directory");

    // 📐 The Inspect call at the header's trailing edge — jumps the leaf to the selected record's
    //    properties, the same travel Tab performs one step at a time.
    {
        const char* Caption = "Inspect";
        const float Run     = Scaled.RunSecondary;
        const float PadX    = Scaled.HeaderPadX * 0.8f;
        const float CallSpan = PadX * 2.0f + Surface->MeasureRun(Caption, Run, 0.0f) + 12.0f;

        const PlaneExtent Call = Spanning(Header.MaximumX - PadX - CallSpan,
                                          Header.MinimumY + (Header.Height() - 24.0f) * 0.5f,
                                          CallSpan, 24.0f);

        const bool OnCall = Call.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Sampled.ContactPressed && OnCall && !Ledger->AnyDisclosed())
            Ledger->Grab(InspectCall, ControlPart::Body);

        if (OnCall && Ledger->Released(InspectCall))
            Applied.OutlinePage = 1u;

        Ledger->DeclareHovered(InspectCall, OnCall, HoverOver);

        // 🔴 THIS DID NOT LOOK LIKE A BUTTON. It drew a bare run of text with a ground
        //    only while hovered, so at rest it was indistinguishable from the header's
        //    own labels — the artist had no way to know the thing was pressable, which
        //    is why it read as decoration. It carries a ground, an edge and a chevron
        //    at rest now, and lifts on hover like every other action in the shell.
        const float Lit = Ledger->HoveredFraction(InspectCall);

        Surface->Ground(Call, Blend(Tinted.Tile, Tinted.TileHovered, Lit),
                        Call.Height() * 0.5f, CornerAll);
        Surface->Edge(Call, Blend(Tinted.Hairline, Tinted.HairlineFirm, Lit), 1.0f,
                      Call.Height() * 0.5f, CornerAll);

        Surface->TextRun(Call.MinimumX + PadX,
                         Call.MinimumY + (Call.Height() - Run) * 0.5f,
                         OnCall ? Tinted.Primary : Tinted.Muted, Caption, Run);

        // 📐 A trailing chevron, so the button states that it travels somewhere.
        const float Mark = 10.0f;

        Surface->Stroke(SymbolSubject::ChevronRight,
                        Spanning(Call.MaximumX - PadX - Mark * 0.6f,
                                 Call.MinimumY + (Call.Height() - Mark) * 0.5f, Mark, Mark),
                        OnCall ? Tinted.Primary : Tinted.Faint);
    }

    // 📐 One footer belongs to the whole Directory destination, not only to its outliner column. The
    //    details pane now terminates above the same band, so the page has a complete baseline before
    //    it slides to Properties / History.
    const PlaneExtent Footer = Spanning(DirectoryExtent.MinimumX,
                                        DirectoryExtent.MaximumY - Scaled.FooterHeight,
                                        DirectoryExtent.Width(), Scaled.FooterHeight);

    // 🔴 THE DIRECTORY | PROPERTIES | HISTORY STRIP IS WITHDRAWN, as asked. It was a
    //    third route to a page that Tab already cycles and that the header's Inspect
    //    call already jumps to, and it spent a whole band restating navigation the
    //    leaf has twice over. The inspector's own Properties | History strip stays —
    //    that one chooses between two pages nothing else reaches.
    const PlaneExtent Strip = Spanning(Outlining.MinimumX, Footer.MinimumY,
                                       Outlining.Width(), 0.0f);

    // 📝 The search field, between the header and the rows — the scene directory's own filter box,
    //    drawn as a PILL: the radius is half the field's height, so both ends are fully rounded. The
    //    host feeds the typed run through the seam's `AcceptTyped` while `SearchTaken` stands
    //    (reported by `Advance`), and Backspace/Escape clear it.
    const PlaneExtent Search = Spanning(Outlining.MinimumX + Pad, Header.MaximumY + Pad,
                                        Outlining.Width() - Pad * 2.0f, Scaled.SearchHeight);

    {
        const bool Hovered = Search.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Hovered && Sampled.ContactPressed && !Ledger->AnyDisclosed())
            Ledger->Grab(SearchField, ControlPart::Body);

        const bool Taken = Ledger->Holding(SearchField) || Ledger->Disclosed(SearchField);

        // 🔴 A pill: `Search.Height() * 0.5f` corners, never the card radius — the reported render
        //    showed the search box with the field's small radius, reading as a squashed input.
        const float PillRadius = Search.Height() * 0.5f;

        Surface->Ground(Search, Tinted.MenuLower, PillRadius, CornerAll);
        Surface->Edge(Search, Taken ? Faded(Covering(0xFFFFFFu), 0.22f) : Tinted.Hairline,
                      1.0f, PillRadius, CornerAll);

        const float GlyphExtent = 14.0f;
        const float GlyphLead   = Search.MinimumX + 10.0f;
        const float GlyphTop    = Search.MinimumY + (Search.Height() - GlyphExtent) * 0.5f;

        Surface->Stroke(SymbolSubject::MagnifierLens,
                        Spanning(GlyphLead, GlyphTop, GlyphExtent, GlyphExtent), Tinted.Faint);

        const float RunLead = GlyphLead + GlyphExtent + 8.0f;
        const float FieldRun = Scaled.RunSecondary * (12.0f / 11.5f);
        const float RunTop   = Search.MinimumY + (Search.Height() - FieldRun) * 0.5f;

        const bool Empty = Applied.EntityRetention[0] == '\0';

        Surface->TextRunTruncated(RunLead, RunTop, Search.MaximumX - RunLead - 8.0f,
                                  Empty ? Tinted.Faint : Tinted.Primary,
                                  Empty ? "Filter records\u2026" : Applied.EntityRetention, FieldRun);
    }

    // 📝 The filter card — the validation UI's generic facet filter, ported to the editor: wrapped
    //    category chips, individual removal, clear-all, and the shared dropdown. It sits after the
    //    search box, and both together decide which rows the directory presents.
    const FacetDeclaration EditorFacets =
    {
        "Filters",
        EditorFacetOptions,
        EditorFacetColours,
        EditorFacetCount,
        0xFFFFFFFFu   // [-] - no locked facet
    };

    const float FacetY = Facets.MeasureHeight(Outlining.Width() - Pad * 2.0f, EditorFacets,
                                              Applied.FacetEnabled);

    const PlaneExtent FacetCard = Spanning(Outlining.MinimumX + Pad, Search.MaximumY + Pad,
                                           Outlining.Width() - Pad * 2.0f, FacetY);

    Discard(Facets.Record(FacetCard, EditorFacets, Applied.FacetEnabled));

    const PlaneExtent Body = Spanning(Outlining.MinimumX + Pad, FacetCard.MaximumY + Pad,
                                      Outlining.Width() - Pad * 2.0f,
                                      Strip.MinimumY - FacetCard.MaximumY - Pad);

    if (Body.MaximumY <= Body.MinimumY)
    {
        Surface->Release();
        return;
    }

    Surface->Confine(Body);

    float Sweep = Body.MinimumY;

    // 📝 The search and the facets decide every row's presence: a row is presented when it matches
    //    (name or tags, within an enabled category) or when any row it holds matches, and while the
    //    filter stands every branch is forced open — the shell's own rule.
    const bool Filtering = RetentionActive(Applied);

    for (std::uint32_t Ordinal = 0u; Ordinal < RowCount; ++Ordinal)
    {
        if (Filtering)
        {
            if (!RowRetained(Applied, Rows[Ordinal]))
            {
                bool DescendantRetained = false;

                for (std::uint32_t Inward = Ordinal + 1u; Inward < RowCount; ++Inward)
                {
                    if (Rows[Inward].Depth <= Rows[Ordinal].Depth)
                        break;

                    if (RowRetained(Applied, Rows[Inward]))
                    {
                        DescendantRetained = true;
                        break;
                    }
                }

                if (!DescendantRetained)
                    continue;
            }
        }
        else
        {
            // 📐 Walked outward: a row is presented only when every enclosure above it stands disclosed.
            std::uint32_t Walking = Rows[Ordinal].Enclosing;
            std::uint32_t Walked  = 0u;

            while (Walking < RowCount && Walked++ <= RowCount)
            {
                if (!Applied.EntityExpanded[Walking])
                    break;

                Walking = Rows[Walking].Enclosing;
            }

            if (Walking < RowCount)
                continue;
        }

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
            Surface->Stroke((Applied.EntityExpanded[Ordinal] || Filtering)
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

    // 📝 The empty state: the filter stands but nothing matched.
    if (Filtering && Sweep <= Body.MinimumY + 0.5f)
    {
        const float Run = Scaled.RunSecondary;
        const char* Prose = "No records match the search or filters.";

        Surface->TextRun(Body.MinimumX + (Body.Width()
                                          - Surface->MeasureRun(Prose, Run, 0.0f)) * 0.5f,
                         Body.MinimumY + Scaled.PanePad * 2.0f, Tinted.Faint, Prose, Run);
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
                     Tinted.Muted, " records", FooterRun);

    // ⑤ The details pane — the small metadata and options card for the taken row.
    const PlaneExtent Detailing = Spanning(Outlining.MaximumX, DirectoryExtent.MinimumY,
                                           DirectoryExtent.MaximumX - Outlining.MaximumX,
                                           Footer.MinimumY - DirectoryExtent.MinimumY);

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
        Surface->Release();
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
    Surface->Release();

    // 🔴 The filter card's dropdown is a deferred popup, exactly like the editor panel's menus: it
    //    must record AFTER the rows and the details pane so it composites above them, never under.
    Facets.RecordDeferred();
}

void SceneDirectoryPanel::RecordDetailOptions(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                                              std::uint32_t Ordinal, const EntityRow& Current)
{
    // 📐 The camera row's options are the camera's own settings: the lag and the pitch direction,
    //    beside the visibility every row carries. Every other row keeps the reference's generic
    //    options. The bits are the same slots — bit 1 is lag on the camera, Locked elsewhere.
    const bool Camera = Current.Subject == EntitySubject::Camera;

    const char* const CameraCaptions[3]    = { "Visible", "Position Lag", "Invert Pitch" };
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

        // 🔴 `ChipExtent * 2.5` is 8 * 2.5 = 20 px across and 10 px tall — barely half
        //    the pill every other switch in the editor draws, so the three Options
        //    toggles read as dots rather than as switches and did not match the layer
        //    stack's or the channel card's. The shared pill is 14 px tall at the
        //    reference's 50:32 ratio, which is what those spend; these spend it too.
        const float ToggleY = 14.0f;
        const float ToggleX = ToggleY * (50.0f / 32.0f);
        const PlaneExtent Switch = Spanning(Row.MaximumX - ToggleX - Scaled.PanePad * 1.5f,
                                            Row.MinimumY + (Row.Height() - ToggleY) * 0.5f,
                                            ToggleX, ToggleY);

        // 🔴 This drew its own pill: the nub was placed by a ternary, so it
        //    jumped between the two ends instead of travelling, and its radius
        //    was Toggle*0.5-2 rather than the shared proportion. The same switch
        //    animated in the validation host and snapped here.
        Controls.SwitchTrack(DetailOptions[Ordinal][Option], Switch, State,
                             Tinted.EntityAccent, Tinted.Hairline, Covering(0xFFFFFFu));

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
                                           const EntityRevision* Revisions, std::uint32_t RevisionCount,
                                           std::uint32_t& InspectorTab, bool OutlinePresentation)
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

        RecordLeafHeader(Header, EntityGlyph(Current.Subject), Hue,
                         Current.Naming, EntityText(Current.Subject));
    }

    if (OutlinePresentation)
    {
        const char* Caption = "Directory";
        const float Run = Scaled.RunSecondary;
        const float PadX = 10.0f;
        const float CallSpan = PadX * 2.0f + Surface->MeasureRun(Caption, Run, 0.0f) + 12.0f;
        const PlaneExtent Call = Spanning(Header.MaximumX - Scaled.HeaderPadX - CallSpan,
                                          Header.MinimumY + (Header.Height() - 24.0f) * 0.5f,
                                          CallSpan, 24.0f);
        const bool OnCall = Call.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Sampled.ContactPressed && OnCall && !Ledger->AnyDisclosed())
            Ledger->Grab(DirectoryCall, ControlPart::Body);

        if (OnCall && Ledger->Released(DirectoryCall))
            Applied.OutlinePage = 0u;

        Ledger->DeclareHovered(DirectoryCall, OnCall, HoverOver);
        const float Lit = Ledger->HoveredFraction(DirectoryCall);

        Surface->Ground(Call, Blend(Tinted.Tile, Tinted.TileHovered, Lit),
                        Call.Height() * 0.5f, CornerAll);
        Surface->Edge(Call, Blend(Tinted.Hairline, Tinted.HairlineFirm, Lit), 1.0f,
                      Call.Height() * 0.5f, CornerAll);
        Surface->TextRun(Call.MinimumX + PadX * 0.7f,
                         Call.MinimumY + (Call.Height() - Run) * 0.5f,
                         OnCall ? Tinted.Primary : Tinted.Faint, "<", Run, 0.0f, true);
        Surface->TextRun(Call.MinimumX + PadX + 12.0f,
                         Call.MinimumY + (Call.Height() - Run) * 0.5f,
                         OnCall ? Tinted.Primary : Tinted.Muted, Caption, Run);
    }

    // ① The strip, and the inner pages it drives. Cameras own bookmarks rather than revisions:
    //    saved viewpoints are camera data, while History remains meaningful for every other entity.
    const bool CameraSelected = Selected && Rows[Applied.EntityTaken].Subject == EntitySubject::Camera;
    const char* const Captions[2] = { "Properties", CameraSelected ? "Bookmarks" : "History" };

    const PlaneExtent Strip = Spanning(Extent.MinimumX, Header.MaximumY,
                                       Extent.Width(), Scaled.ComponentY);

    const TabDeclaration Declared{ Captions, 2u };

    static_cast<void>(Controls.TabStrip(InspectorStrip, Strip, Declared, InspectorTab));

    const PlaneExtent Pages = Spanning(Extent.MinimumX, Strip.MaximumY, Extent.Width(),
                                       Extent.MaximumY - Strip.MaximumY - Scaled.FooterHeight);

    // 📐 The two pages sit side by side and travel on an eased interpolant. A hard ternary here was
    //    the broken carousel: it teleported between Properties and History despite drawing both pages.
    const std::uint32_t MotionOrdinal = OutlinePresentation ? 0u : 1u;

    if (InspectorTab != InspectorArriving[MotionOrdinal])
    {
        InspectorDeparted[MotionOrdinal] = InspectorArriving[MotionOrdinal];
        InspectorArriving[MotionOrdinal] = InspectorTab;
        Motion->Eased(InspectorMotion[MotionOrdinal]).Depart(0.0, 1.0, 240.0, 0.0,
                                                             EaseCurve::Carousel);
    }

    const float Travelled = static_cast<float>(Motion->Eased(InspectorMotion[MotionOrdinal]).Current());
    const float DepartedAt = (InspectorDeparted[MotionOrdinal] == 1u) ? -Pages.Width() : 0.0f;
    const float ArrivingAt = (InspectorArriving[MotionOrdinal] == 1u) ? -Pages.Width() : 0.0f;
    const float Carried = DepartedAt + (ArrivingAt - DepartedAt) * Travelled;

    Surface->Confine(Pages);

    const PlaneExtent Leading = Spanning(Pages.MinimumX + Carried, Pages.MinimumY,
                                         Pages.Width(), Pages.Height());
    const PlaneExtent Trailing = Spanning(Leading.MaximumX, Pages.MinimumY,
                                          Pages.Width(), Pages.Height());

    if (!Surface->Excluded(Leading))
        RecordPropertyCards(Leading, Applied, Rows, RowCount);

    if (!Surface->Excluded(Trailing))
    {
        if (CameraSelected)
            RecordCameraBookmarks(Trailing, Applied);
        else
            RecordRevisionSpine(Trailing, Applied, Rows, RowCount, Revisions, RevisionCount);
    }

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
                         (InspectorTab == 0u) ? "Properties"
                                              : (CameraSelected ? "Bookmarks" : "History"), FooterRun);
    }

    EnvironmentControls.RecordDeferred();
}

// 🧩 The properties column's wheel scroll, eased. Answers where it stands.
float SceneDirectoryPanel::AdvanceOutlineScroll(SceneDirectoryContext& Applied,
                                                const PlaneExtent& Viewport)
{
    // 📐 The content's height is not known until the cards have been laid out, so the
    //    ceiling is taken from the PREVIOUS tick's sweep. A column whose height
    //    changes settles in one tick, and a wheel notch never travels past content
    //    that is no longer there.
    const float Travel = (PropertyContent > Viewport.Height())
                       ? (PropertyContent - Viewport.Height()) : 0.0f;

    if (Travel <= 0.0f)
    {
        PropertyWanted = 0.0f;
        PropertyShown  = 0.0f;
        return 0.0f;
    }

    if (Viewport.Encloses(Sampled.PositionX, Sampled.PositionY) && Sampled.WheelY != 0.0f &&
        !Ledger->AnyDisclosed())
    {
        PropertyWanted -= Sampled.WheelY * 56.0f;
    }

    if (PropertyWanted < 0.0f)     PropertyWanted = 0.0f;
    if (PropertyWanted > Travel)   PropertyWanted = Travel;

    const float Remaining = PropertyWanted - PropertyShown;

    if (Remaining > 0.35f || Remaining < -0.35f)
        PropertyShown += Remaining * 0.26f;
    else
        PropertyShown = PropertyWanted;

    static_cast<void>(Applied);
    return PropertyShown;
}

void SceneDirectoryPanel::RecordPropertyCards(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                                              const EntityRow* Rows, std::uint32_t RowCount)
{
    if (Extent.Width() <= 0.0f || Extent.Height() <= 0.0f)
        return;

    const float Pad = Scaled.PanePad;
    // 🔴 THE PROPERTIES PAGE COULD NOT BE SCROLLED. With the sun, sky, grid, sun-disc
    //    and atmosphere cards all unfolded the column stands far past the leaf, and
    //    there was no wheel path at all — the tail was unreachable. The page scrolls
    //    on the same eased chase the layer stack's lists use.
    const float Wheeled = AdvanceOutlineScroll(Applied, Extent);

    float       Sweep = Extent.MinimumY + Pad - Wheeled;
    std::uint32_t CardOrdinal = 0u;

    // 📝 The property card — a folding card, from the reference's generic component cards.
    // 🔴 Its rows used to be inert labels: the row said "Position" and drew no reading at all. A row
    //    may now carry three ordinates and a unit, recorded through the shared VectorRow so a
    //    transform reads as [X|Y|Z|unit] in the same pill grammar the scalar rows use.
    const auto RecordCard = [&](const char* Caption, const char* const* Fields, std::uint32_t FieldCount,
                                double (*Vectors)[3] = nullptr, const char* const* Units = nullptr)
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

        // 🔴 The card ground was the literal 0x0A0A0B, which is exactly the value
        //    behind Tinted.Desk. Spelling it as a hex pinned this one surface to
        //    the dark palette while every neighbour — the header below, the
        //    hairline around it — followed the theme, so on a light theme the card
        //    stayed a black slab. Same value, taken from the record that owns it.
        Surface->Ground(Card, Tinted.Desk, Scaled.CardRadius, CornerAll);
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

                if (Vectors != nullptr)
                {
                    VectorDeclaration Axes;
                    Axes.Caption   = Fields[FieldOrdinal];
                    Axes.UnitGlyph = (Units != nullptr && Units[FieldOrdinal] != nullptr)
                                   ? Units[FieldOrdinal] : "";

                    static_cast<void>(EnvironmentControls.VectorRow(
                        CardFields[Target][FieldOrdinal], Row, Axes, Vectors[FieldOrdinal]));
                }
                else
                {
                    Surface->TextRun(Row.MinimumX + 2.0f,
                                     Row.MinimumY + (Row.Height() - Scaled.RunPrimary) * 0.5f,
                                     Tinted.Muted, Fields[FieldOrdinal], Scaled.RunPrimary);
                }

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
        // 📐 One unit per row: metres, degrees, and a bare multiplier for scale.
        const char* const TransformRows[3]  = { "Position", "Rotation", "Scale" };
        const char* const TransformUnits[3] = { "m", "\xC2\xB0", "x" };

        // 📐 Three rows, three ordinate triples, contiguous so one pointer serves
        //    the loop. Scale is seeded to 1 once; a zero-scale default would read
        //    as a collapsed object on a fresh scene.
        if (!Applied.TransformSeeded)
        {
            for (std::uint32_t Each = 0u; Each < SceneDirectoryContext::EntityCeiling; ++Each)
                for (std::uint32_t Axis = 0u; Axis < 3u; ++Axis)
                    Applied.EntityScale[Each][Axis] = 1.0;

            Applied.TransformSeeded = true;
        }

        double Ordinates[3][3] = {};
        for (std::uint32_t Axis = 0u; Axis < 3u; ++Axis)
        {
            Ordinates[0][Axis] = Applied.EntityPosition[Taken][Axis];
            Ordinates[1][Axis] = Applied.EntityRotation[Taken][Axis];
            Ordinates[2][Axis] = Applied.EntityScale[Taken][Axis];
        }

        RecordCard("Transform", TransformRows, 3u, Ordinates, TransformUnits);

        // the rows edit in place, so carry any drag back to the record
        for (std::uint32_t Axis = 0u; Axis < 3u; ++Axis)
        {
            Applied.EntityPosition[Taken][Axis] = Ordinates[0][Axis];
            Applied.EntityRotation[Taken][Axis] = Ordinates[1][Axis];
            Applied.EntityScale[Taken][Axis]    = Ordinates[2][Axis];
        }
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

                // 📐 The sun disc is the icon drawn over the atmosphere: its radius multiplier and
                //    direct-term intensity. Same component as the other environment cards.
                const char* const DiscCaptions[2] = { "Disc Radius", "Disc Intensity" };
                const char* const DiscUnits[2]    = { "x", "" };
                const double DiscMinimums[2]      = { 1.0, 0.0 };
                const double DiscMaximums[2]      = { 32.0, 4.0 };
                double DiscValues[2]              = { Applied.Environment.SunDiscRadius,
                                                     Applied.Environment.SunDiscIntensity };

                RecordEnvironmentCard(Applied, Extent, Sweep, CardOrdinal,
                                      "Sun Disc", DiscCaptions, DiscUnits, DiscMinimums, DiscMaximums,
                                      DiscValues, 2u);

                Applied.Environment.SunDiscRadius    = DiscValues[0];
                Applied.Environment.SunDiscIntensity = DiscValues[1];
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

                const char* const AtmoCaptions[4] = { "Atmosphere Density", "Scale Height", "Mie Density", "Mie Asymmetry" };
                const char* const AtmoUnits[4]    = { "", "", "x", "" };
                const double AtmoMinimums[4]      = { 0.0, 0.2, 0.0, -0.95 };
                const double AtmoMaximums[4]      = { 3.0, 3.0, 4.0, 0.95 };
                double AtmoValues[4]              = { Applied.Environment.AtmosphereDensity,
                                                      Applied.Environment.AtmosphereScaleHeight,
                                                      Applied.Environment.MieDensity,
                                                      Applied.Environment.MieAsymmetry };

                RecordEnvironmentCard(Applied, Extent, Sweep, CardOrdinal,
                                      "Atmosphere", AtmoCaptions, AtmoUnits, AtmoMinimums,
                                      AtmoMaximums, AtmoValues, 4u);

                Applied.Environment.AtmosphereDensity     = AtmoValues[0];
                Applied.Environment.AtmosphereScaleHeight = AtmoValues[1];
                Applied.Environment.MieDensity            = AtmoValues[2];
                Applied.Environment.MieAsymmetry          = AtmoValues[3];
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

    // 📐 What the column actually came to, for next tick's scroll ceiling. Measured
    //    from the sweep rather than predicted, so a card folding or a subject
    //    changing the card set is accounted for without a second layout pass.
    PropertyContent = (Sweep + Wheeled) - Extent.MinimumY + Pad;

    // 📐 The thumb, only while there is somewhere to travel.
    if (PropertyContent > Extent.Height() && Extent.Height() > 0.0f)
    {
        const float Fraction = Extent.Height() / PropertyContent;
        const float ThumbY   = Extent.Height() * Fraction;
        const float Reach    = PropertyContent - Extent.Height();
        const float Along    = (Reach > 0.0f) ? (Wheeled / Reach) : 0.0f;

        Surface->Ground(Spanning(Extent.MaximumX - 5.0f,
                                 Extent.MinimumY + (Extent.Height() - ThumbY) * Along,
                                 3.0f, ThumbY),
                        Faded(Tinted.Muted, 0.55f), 1.5f, CornerAll);
    }
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

    // 🔴 The second card ground; same literal, same defect as above.
    Surface->Ground(Card, Tinted.Desk, Scaled.CardRadius, CornerAll);
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
            // 🔴 The same defect as the shell's copy of this card: the rows were
            //    laid out with no label at all. Label · track · readout.
            Declared.Layout      = MagnitudeDeclaration::Arrange::Measured;

            double& Coordinate   = Values[SliderOrdinal];

            static_cast<void>(EnvironmentControls.MagnitudeRow(EnvironmentSliders[SliderOrdinal],
                                                               Row, Declared, Coordinate, false));

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

void SceneDirectoryPanel::RecordCameraBookmarks(const PlaneExtent& Extent,
                                                    SceneDirectoryContext& Applied)
{
    Surface->Ground(Extent, Tinted.MenuLower, 0.0f, CornerNone);

    const float Pad = Scaled.PanePad * 1.5f;
    const float ButtonY = 26.0f;
    const float Top = Extent.MinimumY + Pad;
    const float ButtonGap = 8.0f;
    const float ButtonWidth = 108.0f;

    const PlaneExtent Save = Spanning(Extent.MaximumX - Pad - ButtonWidth, Top,
                                      ButtonWidth, ButtonY);
    const PlaneExtent Recall = Spanning(Save.MinimumX - ButtonGap - ButtonWidth, Top,
                                        ButtonWidth, ButtonY);
    const PlaneExtent Retire = Spanning(Recall.MinimumX - ButtonGap - ButtonWidth, Top,
                                        ButtonWidth, ButtonY);

    const auto Pill = [&](ControlIdentity Target, const PlaneExtent& Bounds,
                          const char* Caption, bool Enabled) -> bool
    {
        const bool Over = Enabled && Bounds.Encloses(Sampled.PositionX, Sampled.PositionY);
        if (Sampled.ContactPressed && Over && !Ledger->AnyDisclosed())
            Ledger->Grab(Target, ControlPart::Body);

        Surface->Ground(Bounds, Over ? Tinted.TileHovered : Tinted.Tile,
                        Bounds.Height() * 0.5f, CornerAll);
        Surface->Edge(Bounds, Enabled ? Tinted.HairlineFirm : Tinted.Hairline, 1.0f,
                      Bounds.Height() * 0.5f, CornerAll);
        const float Run = Scaled.RunFine;
        Surface->TextRun(Bounds.MinimumX + (Bounds.Width() - Surface->MeasureRun(Caption, Run, 0.0f)) * 0.5f,
                         Bounds.MinimumY + (Bounds.Height() - Run) * 0.5f,
                         Enabled ? Tinted.Primary : Tinted.Faint, Caption, Run);
        return Over && Ledger->Released(Target);
    };

    if (Pill(BookmarkSave, Save, "Save Current", true) &&
        Applied.CameraBookmarkCount < SceneDirectoryContext::CameraBookmarkCeiling)
    {
        const std::uint32_t Bookmark = Applied.CameraBookmarkCount++;
        Applied.CameraBookmarkTaken = Bookmark;
        std::snprintf(Applied.CameraBookmarkNames[Bookmark], 32u, "Bookmark %u",
                      static_cast<unsigned>(Bookmark + 1u));
        for (std::uint32_t Axis = 0u; Axis < 3u; ++Axis)
        {
            Applied.CameraBookmarkPosition[Bookmark][Axis] = Applied.CameraPosition[Axis];
            Applied.CameraBookmarkRotation[Bookmark][Axis] = Applied.CameraRotation[Axis];
        }
    }

    if (Pill(BookmarkRecall, Recall, "Go To", Applied.CameraBookmarkCount > 0u))
        Applied.CameraBookmarkRecallRequested = true;

    if (Pill(BookmarkRetire, Retire, "Delete", Applied.CameraBookmarkCount > 0u))
    {
        const std::uint32_t Retiring = Applied.CameraBookmarkTaken;
        for (std::uint32_t Bookmark = Retiring; Bookmark + 1u < Applied.CameraBookmarkCount; ++Bookmark)
        {
            std::memcpy(Applied.CameraBookmarkNames[Bookmark],
                        Applied.CameraBookmarkNames[Bookmark + 1u], 32u);
            for (std::uint32_t Axis = 0u; Axis < 3u; ++Axis)
            {
                Applied.CameraBookmarkPosition[Bookmark][Axis] =
                    Applied.CameraBookmarkPosition[Bookmark + 1u][Axis];
                Applied.CameraBookmarkRotation[Bookmark][Axis] =
                    Applied.CameraBookmarkRotation[Bookmark + 1u][Axis];
            }
        }
        --Applied.CameraBookmarkCount;
        if (Applied.CameraBookmarkCount == 0u)
            Applied.CameraBookmarkTaken = 0u;
        else if (Applied.CameraBookmarkTaken >= Applied.CameraBookmarkCount)
            Applied.CameraBookmarkTaken = Applied.CameraBookmarkCount - 1u;
    }

    Surface->TextRun(Extent.MinimumX + Pad, Top + (ButtonY - Scaled.RunSecondary) * 0.5f,
                     Tinted.Primary, "Camera Bookmarks", Scaled.RunSecondary, 0.0f, true);

    float Sweep = Top + ButtonY + Pad;
    if (Applied.CameraBookmarkCount == 0u)
    {
        const char* Empty = "Move the Editor Camera, then save the current viewpoint.";
        Surface->TextRun(Extent.MinimumX + Pad, Sweep + Pad, Tinted.Faint,
                         Empty, Scaled.RunFine);
        return;
    }

    for (std::uint32_t Bookmark = 0u; Bookmark < Applied.CameraBookmarkCount; ++Bookmark)
    {
        const PlaneExtent Card = Spanning(Extent.MinimumX + Pad, Sweep,
                                          Extent.Width() - Pad * 2.0f, 58.0f);
        const bool Selected = Applied.CameraBookmarkTaken == Bookmark;
        const bool Over = Card.Encloses(Sampled.PositionX, Sampled.PositionY);

        Surface->Ground(Card, Selected ? Tinted.EntityTaken : Tinted.Tile,
                        Scaled.CardRadius, CornerAll);
        Surface->Edge(Card, Selected ? Tinted.HairlineFirm : Tinted.Hairline, 1.0f,
                      Scaled.CardRadius, CornerAll);

        if (Sampled.ContactPressed && Over)
            Applied.CameraBookmarkTaken = Bookmark;

        const PlaneExtent Name = Spanning(Card.MinimumX + 8.0f, Card.MinimumY + 5.0f,
                                          Card.Width() - 16.0f, 25.0f);
        EnvironmentControls.EditableText(
            BookmarkNames[Bookmark], Name,
            EditableTextDeclaration{ "Bookmark name", false, false },
            Applied.CameraBookmarkNames[Bookmark], 32u);

        char Pose[96] = {};
        std::snprintf(Pose, sizeof Pose, "%.1f, %.1f, %.1f m   ·   yaw %.1f°   pitch %.1f°",
                      Applied.CameraBookmarkPosition[Bookmark][0],
                      Applied.CameraBookmarkPosition[Bookmark][1],
                      Applied.CameraBookmarkPosition[Bookmark][2],
                      Applied.CameraBookmarkRotation[Bookmark][0],
                      Applied.CameraBookmarkRotation[Bookmark][1]);
        Surface->TextRun(Card.MinimumX + 10.0f, Card.MinimumY + 36.0f,
                         Tinted.Faint, Pose, Scaled.RunFiner);

        Sweep += Card.Height() + 8.0f;
        if (Sweep >= Extent.MaximumY)
            break;
    }
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


//------------------------------------------------------------------------------------------------------------------------
//                                                       THE GIZMO
//------------------------------------------------------------------------------------------------------------------------

void SceneDirectoryPanel::RecordGizmo(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                                      OverlayGeometry& Overlay)
{
    // 📐 The ground grid and all 3 world axes (Red X, Green Y, Blue Z) are rendered 100%
    //    on the GPU by the overlay pass fragment shader (OverlayFragment.slang). The CPU gizmo
    //    arrows are removed.
    (void)Extent;
    (void)Applied;
    (void)Overlay;
}

void SceneDirectoryPanel::RecordOverlayFallback(const PlaneExtent& Extent,
                                                const OverlayGeometry& Overlay)
{
    if (Surface == nullptr)
        return;

    // 📐 The SAME record the GPU pass would draw, drawn through the interface instead — the fallback
    //    when the pass could not stand. Everything is confined to the leaf, exactly as the pass's
    //    scissor clips its own draw: the grid, the axes and the gizmo never paint over the panels.
    Surface->Confine(Extent);

    const auto Token = [](std::uint32_t Packed) -> ThemeToken
    {
        const float Alpha = static_cast<float>((Packed >> 24u) & 0xFFu) / 255.0f;
        return Faded(Covering(Packed & 0xFFFFFFu), Alpha);
    };

    for (std::uint32_t Ordinal = 0u; Ordinal < Overlay.LineCount; ++Ordinal)
    {
        const OverlayLine& Line = Overlay.Lines[Ordinal];
        const float PointsX[2] = { Line.X0, Line.X1 };
        const float PointsY[2] = { Line.Y0, Line.Y1 };
        Surface->Polyline(PointsX, PointsY, 2u, Token(Line.Packed), Line.Thickness);
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < Overlay.DotCount; ++Ordinal)
    {
        const OverlayDot& Dot = Overlay.Dots[Ordinal];
        Surface->Medallion(Dot.X, Dot.Y, Dot.Radius, Token(Dot.Packed));
    }

    for (std::uint32_t Ordinal = 0u; Ordinal < Overlay.TriangleCount; ++Ordinal)
    {
        const OverlayTriangle& Triangle = Overlay.Triangles[Ordinal];
        const float Corners[6] = { Triangle.X0, Triangle.Y0,
                                   Triangle.X1, Triangle.Y1,
                                   Triangle.X2, Triangle.Y2 };
        Surface->Tongue(Corners, 3u, Token(Triangle.Packed));
    }

    Surface->Release();
}

}   // namespace Slate
