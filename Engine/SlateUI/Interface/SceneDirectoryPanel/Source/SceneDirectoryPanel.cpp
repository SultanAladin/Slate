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

    // 📐 The leaf's page travel. Registered here, never mid-tick.
    {
        const Outcome<std::uint32_t> Eased = Integrator.RegisterEased(1.0);

        if (!Eased.Resolved)
            return Outcome<bool>::Refuse(Eased.Error);

        OutlineMotion = Eased.Resolve();
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

    // 📐 Tab walks the outliner leaf's pages: Directory → Properties → History → Directory. The key
    //    is the seam's Summon (Tab), edge-triggered there and unrepeated, so one press is one page.
    if (TabPressed)
        Applied.OutlinePage = (Applied.OutlinePage + 1u) % 3u;

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
    constexpr std::uint32_t VertexCount = (MeshColumns + 1u) * (MeshRows + 1u);
    constexpr std::uint32_t IndexCount  = MeshColumns * MeshRows * 6u;

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

    // 📐 The camera basis, the same convention the fly rig integrates: forward along the view, right
    //    across it, up the cross product.
    const double Forward[3] = { CosP * SinY, SinP, CosP * CosY };
    const double Right[3]   = { CosY, 0.0, -SinY };
    const double Up[3]      = { -SinP * SinY, CosP, -SinP * CosY };

    float Positions[VertexCount * 2u];
    float UVs[VertexCount * 2u];
    std::uint32_t Indices[IndexCount];

    std::uint32_t Vertex = 0u;

    for (std::uint32_t Row = 0u; Row <= MeshRows; ++Row)
    {
        for (std::uint32_t Column = 0u; Column <= MeshColumns; ++Column)
        {
            const float ScreenX = Extent.MinimumX + static_cast<float>(Column) / static_cast<float>(MeshColumns)
                                * Extent.Width();
            const float ScreenY = Extent.MinimumY + static_cast<float>(Row) / static_cast<float>(MeshRows)
                                * Extent.Height();

            const double NdcX = (static_cast<double>(ScreenX) - CentreX) / (Extent.Width()  * 0.5);
            const double NdcY = (static_cast<double>(CentreY) - ScreenY) / (Extent.Height() * 0.5);

            double Ray[3] = { NdcX * TanHalfH, NdcY * TanHalfV, 1.0 };
            const double Length = std::sqrt(Ray[0] * Ray[0] + Ray[1] * Ray[1] + Ray[2] * Ray[2]);
            Ray[0] /= Length;
            Ray[1] /= Length;
            Ray[2] /= Length;

            const double DirectionX = Right[0] * Ray[0] + Up[0] * Ray[1] + Forward[0] * Ray[2];
            const double DirectionY = Right[1] * Ray[0] + Up[1] * Ray[1] + Forward[1] * Ray[2];
            const double DirectionZ = Right[2] * Ray[0] + Up[2] * Ray[1] + Forward[2] * Ray[2];

            // 📐 Dome coordinates: U = azimuth/2π + 0.5 with azimuth measured from +Z toward +X (the
            //    dome generator's own convention), V = (90 − elevation)/180.
            // 🔴 U is ABSOLUTE and the sampler wraps it (REPEAT on U, clamp on V): the dome's azimuth
            //    is periodic, and a camera whose frustum crosses the seam (yaw near ±180°) spans U
            //    from ~0.93 to ~0.07 — with a clamped sampler the edge texels smear across the seam
            //    as a stretched band, which was the reported pixelation. Shifting the mesh's U by the
            //    yaw would break the texture content (a shift of yaw/2π is not a whole period), so the
            //    wrap belongs to the sampler, not the coordinates.
            const double Azimuth   = std::atan2(DirectionX, DirectionZ);
            const double Elevation = std::asin(std::clamp(DirectionY, -1.0, 1.0));

            Positions[Vertex * 2u]     = ScreenX;
            Positions[Vertex * 2u + 1u] = ScreenY;
            UVs[Vertex * 2u]     = static_cast<float>(Azimuth / (2.0 * 3.14159265358979323846) + 0.5);
            UVs[Vertex * 2u + 1u] = static_cast<float>(std::clamp(0.5 - Elevation / 3.14159265358979323846, 0.0, 1.0));

            ++Vertex;
        }
    }

    std::uint32_t Index = 0u;

    for (std::uint32_t Row = 0u; Row < MeshRows; ++Row)
    {
        for (std::uint32_t Column = 0u; Column < MeshColumns; ++Column)
        {
            const std::uint32_t A = Row * (MeshColumns + 1u) + Column;
            const std::uint32_t B = A + 1u;
            const std::uint32_t C = A + (MeshColumns + 1u);
            const std::uint32_t D = C + 1u;

            Indices[Index++] = A;
            Indices[Index++] = C;
            Indices[Index++] = B;
            Indices[Index++] = B;
            Indices[Index++] = C;
            Indices[Index++] = D;
        }
    }

    Surface->ImageMesh(Applied.SkyTextureIdentity, Positions, UVs, VertexCount, Indices, IndexCount);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE GROUND GRID
//------------------------------------------------------------------------------------------------------------------------

// 🔴 THE CPU GROUND LATTICE IS DELETED — 1828 lines carrying NINETEEN RED markers.
//    It marched the plane as line SEGMENTS on the CPU and handed screen-space
//    records to the overlay pass, and every one of its defects was a consequence
//    of that choice rather than a mistake inside it:
//
//      near-plane splitting, `Behind` runs   the CPU clipping what the GPU clips free
//      `float X[64]`, `Tally`                the CPU batching polylines
//      the over-aggressive cull              the CPU cheaply rejecting off-screen work
//      `LineCeiling` exhaustion              the CPU fitting a fixed budget
//      sample step versus cell size          the CPU guessing a tessellation
//      a finite extent with a visible edge   the CPU unable to afford an infinite one
//
//    The last of those is what the artist reported: the lattice's far boundary
//    moved with the eye height, so the ground appeared to tilt on pitch and to
//    slide the wrong way on Q/E. Measured, the topmost ground line climbed from
//    y=57.6 to y=83.1 as the eye went 1.5 m to 20 m, while a true horizon does
//    not move with height at all.
//
//    The replacement is `OverlayFragment.slang` mode 3: one quad, the plane
//    solved per pixel, the level of detail read from `fwidth` rather than
//    guessed. There is no tessellation, no budget, no extent and therefore no
//    edge — the failure modes above cease to be questions instead of being
//    patched a twentieth time.

void SceneDirectoryPanel::RecordGizmo(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                                        OverlayGeometry& Overlay)
{
    // 📐 The world-origin translation gizmo — the reference fly-cam convention: X red, Y green (up),
    //    Z blue, each an arrow with a filled head, and a centre handle. The colours are FULL-OPACITY
    //    straight alpha: the overlay's own GPU pass blends them vivid, where the interface's
    //    premultiplied read washed the same hues out over a bright sky.
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

    const double Forward[3] = { CosP * SinY, SinP, CosP * CosY };
    const double Right[3]   = { CosY, 0.0, -SinY };
    const double Up[3]      = { -SinP * SinY, CosP, -SinP * CosY };

    const double& CameraX = Applied.CameraPosition[0];
    const double& CameraY = Applied.CameraPosition[1];
    const double& CameraZ = Applied.CameraPosition[2];

    const auto Project = [&](double WorldX, double WorldY, double WorldZ,
                             float& ScreenX, float& ScreenY, bool& Behind) -> void
    {
        const double DX = WorldX - CameraX;
        const double DY = WorldY - CameraY;
        const double DZ = WorldZ - CameraZ;

        const double CameraZDepth = DX * Forward[0] + DY * Forward[1] + DZ * Forward[2];

        if (CameraZDepth < 0.25)
        {
            Behind = true;
            return;
        }

        const double CameraXDepth = DX * Right[0] + DY * Right[1] + DZ * Right[2];
        const double CameraYDepth = DX * Up[0]    + DY * Up[1]    + DZ * Up[2];

        ScreenX = CentreX + static_cast<float>((CameraXDepth / CameraZDepth) / TanHalfH * (Extent.Width()  * 0.5));
        ScreenY = CentreY - static_cast<float>((CameraYDepth / CameraZDepth) / TanHalfV * (Extent.Height() * 0.5));
        Behind  = false;
    };

    constexpr double Reach = 40.0;   // [m] - the gizmo's extent along each axis
    constexpr double Head  = 8.0;    // [m] - the arrowhead's length
    constexpr double Wing  = 3.0;    // [m] - the arrowhead's half width

    const std::uint32_t Red   = PackOverlayColour(0xE5u, 0x48u, 0x4Du, 0xFFu);
    const std::uint32_t Green = PackOverlayColour(0x46u, 0xA7u, 0x58u, 0xFFu);
    const std::uint32_t Blue  = PackOverlayColour(0x3Eu, 0x63u, 0xDDu, 0xFFu);

    float OriginX = 0.0f, OriginY = 0.0f;
    bool  OriginBehind = false;
    Project(0.0, 0.0, 0.0, OriginX, OriginY, OriginBehind);

    const auto RecordAxis = [&](double EndX, double EndY, double EndZ, std::uint32_t Packed)
    {
        float EndScreenX = 0.0f, EndScreenY = 0.0f;
        bool  EndBehind  = false;
        Project(EndX, EndY, EndZ, EndScreenX, EndScreenY, EndBehind);

        if (EndBehind)
            return;

        // 📐 The shaft, then the head: a filled triangle whose base is `Head` back along the axis and
        //    whose wings sit `Wing` either side, in screen space.
        Overlay.AddLine(OriginX, OriginY, EndScreenX, EndScreenY, Packed, 2.0f);

        if (!OriginBehind)
        {
            const float Length = std::sqrt((EndScreenX - OriginX) * (EndScreenX - OriginX)
                                         + (EndScreenY - OriginY) * (EndScreenY - OriginY));

            if (Length > 2.0f)
            {
                const float DirectionX = (EndScreenX - OriginX) / Length;
                const float DirectionY = (EndScreenY - OriginY) / Length;

                const float HeadLength = Head * Length / Reach;
                const float WingSpan   = Wing * Length / Reach;

                const float BaseX = EndScreenX - DirectionX * HeadLength;
                const float BaseY = EndScreenY - DirectionY * HeadLength;
                const float NormalX = -DirectionY;
                const float NormalY =  DirectionX;

                Overlay.AddTriangle(EndScreenX, EndScreenY,
                                    BaseX + NormalX * WingSpan, BaseY + NormalY * WingSpan,
                                    BaseX - NormalX * WingSpan, BaseY - NormalY * WingSpan,
                                    Packed);
            }
        }
    };

    if (!OriginBehind)
    {
        RecordAxis(Reach, 0.0, 0.0, Red);
        RecordAxis(0.0, Reach, 0.0, Green);
        RecordAxis(0.0, 0.0, Reach, Blue);

        // 📐 The centre handle: a small filled square, twice the shaft's width.
        const float Handle = 5.0f;
        const std::uint32_t White = PackOverlayColour(0xF0u, 0xF0u, 0xF0u, 0xFFu);

        Overlay.AddTriangle(OriginX - Handle, OriginY - Handle,
                            OriginX + Handle, OriginY - Handle,
                            OriginX - Handle, OriginY + Handle, White);
        Overlay.AddTriangle(OriginX + Handle, OriginY + Handle,
                            OriginX + Handle, OriginY - Handle,
                            OriginX - Handle, OriginY + Handle, White);
    }
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
