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
        &OutlineStrip,
        &InspectCall,
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

void SceneDirectoryPanel::Advance(const PointerCondition& Contact, double Elapsed,
                                   SceneDirectoryContext& Applied, bool TabPressed)
{
    Sampled = Contact;
    Controls.Advance(Contact, Elapsed);
    // 📝 Sampled, never advanced: the tick owner advances the shared ledger exactly once, and a
    //    second advance would retire the release before the panel reads it.
    EnvironmentControls.Sample(Contact);

    // 📐 Tab walks the outliner leaf's pages: Directory → Properties → History → Directory. The key
    //    is the seam's Summon (Tab), edge-triggered there and unrepeated, so one press is one page.
    if (TabPressed)
        Applied.OutlinePage = (Applied.OutlinePage + 1u) % 3u;
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

void SceneDirectoryPanel::RecordGroundGrid(const PlaneExtent& Extent, SceneDirectoryContext& Applied,
                                              const EditorPanelConfiguration& Configuration,
                                              OverlayGeometry& Overlay)
{
    // 📐 The world the camera travels: a lattice on the Y = 0 plane, projected through the same
    //    pinhole as the sky mesh. Without it a fly camera over a pure skybox reads as static — there
    //    is nothing to move past — and the grid is what makes W/S/A/D/E/Q visibly travel the world,
    //    in metres. It is driven by the viewport's OWN grid settings (the footer's "Grid settings"
    //    popup): the presentation (none / lines / dots / both), the cell size, the extent, and the
    //    axis lines — the same configuration the skeletal lattice in the pre-editor viewport read.
    const PanelLatticePresentation Presentation = Configuration.Lattice;

    constexpr double BaseCell = 20.0;                       // [m] - one lattice cell at scale 1
    const double Cell = BaseCell * static_cast<double>(Configuration.LatticeScale);
    const std::uint32_t Cells = std::max(2u, std::min(128u, Configuration.Subdivisions));
    const double Half = Cell * static_cast<double>(Cells);  // [m] - the lattice's half extent
    constexpr std::uint32_t Samples = 48u;

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

    // 📐 The projector: a world point to screen, or "behind" when it sits on the wrong side of the
    //    camera. Nearer than a quarter metre the division explodes, so the near plane is explicit.
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

    const bool DrawLines = Presentation == PanelLatticePresentation::Lines
                        || Presentation == PanelLatticePresentation::LinesAndDots;
    const bool DrawDots  = Presentation == PanelLatticePresentation::Dots
                        || Presentation == PanelLatticePresentation::LinesAndDots;

    const auto RecordLatticeLine = [&](bool AlongZ, double Offset, bool Coarse) -> void
    {
        float X[64];
        float Y[64];
        std::uint32_t Tally = 0u;

        // 📐 One polyline per contiguous front-run: a line crossing behind the camera is split, so the
        //    near plane never draws a segment across the viewport.
        const auto Flush = [&](std::uint32_t Count) -> void
        {
            // 🔴 The lattice is recorded into the OVERLAY GEOMETRY, not the interface: the GPU pass
            //    draws it in its own pass, straight-alpha, and the CPU record is two points per
            //    segment — the interface's ImGui path tessellated every segment on the CPU.
            const std::uint32_t PackedFine   = PackOverlayColour(0x9A, 0xA6, 0xB8, 0x47u);
            const std::uint32_t PackedCoarse = PackOverlayColour(0x9A, 0xA6, 0xB8, 0x8Cu);

            for (std::uint32_t Ordinal = 1u; Ordinal < Count; ++Ordinal)
            {
                // 🔴 Segments whose bounding box misses the leaf entirely are discarded here — a
                //    camera looking up projects most of the lattice below the frame, and an
                //    unbounded run of those off-screen segments would exhaust the overlay's line
                //    ceiling and starve the axes and the gizmo (the reported missing gizmo).
                const float AX = X[Ordinal - 1u], AY = Y[Ordinal - 1u];
                const float BX = X[Ordinal],     BY = Y[Ordinal];

                if ((AX < Extent.MinimumX && BX < Extent.MinimumX) ||
                    (AX > Extent.MaximumX && BX > Extent.MaximumX) ||
                    (AY < Extent.MinimumY && BY < Extent.MinimumY) ||
                    (AY > Extent.MaximumY && BY > Extent.MaximumY))
                    continue;

                Overlay.AddLine(AX, AY, BX, BY,
                                Coarse ? PackedCoarse : PackedFine,
                                Coarse ? 1.5f : 1.0f);
            }
        };

        for (std::uint32_t Sample = 0u; Sample <= Samples; ++Sample)
        {
            const double T = -Half + (2.0 * Half) * static_cast<double>(Sample) / static_cast<double>(Samples);
            const double WorldX = AlongZ ? Offset : T;
            const double WorldZ = AlongZ ? T : Offset;

            float ScreenX = 0.0f;
            float ScreenY = 0.0f;
            bool  Behind  = false;

            Project(WorldX, 0.0, WorldZ, ScreenX, ScreenY, Behind);

            if (Behind)
            {
                Flush(Tally);
                Tally = 0u;
                continue;
            }

            X[Tally] = ScreenX;
            Y[Tally] = ScreenY;
            ++Tally;
        }

        Flush(Tally);
    };

    const std::uint32_t LineCount = std::max(2u, Cells);

    for (std::uint32_t Ordinal = 0u; Ordinal <= LineCount; ++Ordinal)
    {
        const double Offset = -Half + Cell * static_cast<double>(Ordinal);
        const bool Coarse = (Ordinal % 5u) == 0u;

        if (DrawLines)
        {
            RecordLatticeLine(true,  Offset, Coarse);
            RecordLatticeLine(false, Offset, Coarse);
        }

        // 📐 The dotted presentation: a node at every intersection, drawn as a small disc. Only the
        //    intersections that project in front of the camera are drawn, so a camera at the edge of
        //    the lattice never fills the screen with the far half of it.
        if (DrawDots && (Ordinal % 2u) == 0u)
        {
            const double WorldX = Offset;
            const double WorldZ = Offset;

            float ScreenX = 0.0f;
            float ScreenY = 0.0f;
            bool  Behind  = false;

            Project(WorldX, 0.0, WorldZ, ScreenX, ScreenY, Behind);

            if (!Behind)
            {
                const std::uint32_t Packed = Coarse ? PackOverlayColour(0x9A, 0xA6, 0xB8, 0x99u)
                                                 : PackOverlayColour(0x9A, 0xA6, 0xB8, 0x57u);
                Overlay.AddDot(ScreenX, ScreenY, Packed, Coarse ? 2.6f : 1.8f);
            }
        }
    }

    // 📐 The axis lines — the world's own X (red), Y (green, vertical) and Z (blue), each spanning
    //    the WHOLE lattice, exactly as a real editor's axes do: not a stub from the origin, but a
    //    line that runs the grid's full extent in both directions, so it reads as the world's axes
    //    no matter where the camera stands. Toggled by the grid settings' axis switches, and drawn
    //    even when the lattice presentation is None — the two are independent.
    if (Configuration.AxisX || Configuration.AxisY || Configuration.AxisZ)
    {
        constexpr std::uint32_t AxisSamples = 48u;
        const float AxisWeight = 2.0f;

        const auto RecordAxis = [&](bool AlongX, bool AlongY, bool AlongZ, std::uint32_t Packed)
        {
            float X[AxisSamples + 1u];
            float Y[AxisSamples + 1u];
            std::uint32_t Tally = 0u;

            // 📐 The axis is SAMPLED across the full span (-Half .. +Half) and split at the near
            //    plane, exactly as the lattice lines are: a line that required both endpoints in
            //    front of the camera would vanish the moment the camera crossed the axis — the
            //    reported missing axes. Sampling keeps the front run visible from anywhere.
            const auto Flush = [&](std::uint32_t Count) -> void
            {
                const std::uint32_t PackedColour = PackOverlayColour((Packed >> 16u) & 0xFFu,
                                                                  (Packed >> 8u) & 0xFFu,
                                                                  Packed & 0xFFu, 0xFFu);

                for (std::uint32_t Ordinal = 1u; Ordinal < Count; ++Ordinal)
                {
                    const float AX = X[Ordinal - 1u], AY = Y[Ordinal - 1u];
                    const float BX = X[Ordinal],     BY = Y[Ordinal];

                    if ((AX < Extent.MinimumX && BX < Extent.MinimumX) ||
                        (AX > Extent.MaximumX && BX > Extent.MaximumX) ||
                        (AY < Extent.MinimumY && BY < Extent.MinimumY) ||
                        (AY > Extent.MaximumY && BY > Extent.MaximumY))
                        continue;

                    Overlay.AddLine(AX, AY, BX, BY, PackedColour, AxisWeight);
                }
            };

            for (std::uint32_t Sample = 0u; Sample <= AxisSamples; ++Sample)
            {
                const double T = -Half + (2.0 * Half) * static_cast<double>(Sample) / static_cast<double>(AxisSamples);

                float ScreenX = 0.0f;
                float ScreenY = 0.0f;
                bool  Behind  = false;

                Project(AlongX ? T : 0.0, AlongY ? T : 0.0, AlongZ ? T : 0.0, ScreenX, ScreenY, Behind);

                if (Behind)
                {
                    Flush(Tally);
                    Tally = 0u;
                    continue;
                }

                X[Tally] = ScreenX;
                Y[Tally] = ScreenY;
                ++Tally;
            }

            Flush(Tally);
        };

        if (Configuration.AxisX)
            RecordAxis(true,  false, false, 0xE5484Du);   // [-] - X, red
        if (Configuration.AxisY)
            RecordAxis(false, true,  false, 0x46A758u);   // [-] - Y, green (up)
        if (Configuration.AxisZ)
            RecordAxis(false, false, true,  0x3E63DDu);   // [-] - Z, blue
    }
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

    // 📐 The outliner leaf's pages: 0 the directory (outliner | details), 1 the selected record's
    //    properties, 2 its history. Tab cycles them, the strip below selects them, and the header's
    //    Inspect call jumps to the properties.
    if (Applied.OutlinePage != 0u)
    {
        RecordProperties(Extent, Applied, Rows, RowCount, Revisions, RevisionCount,
                         Applied.OutlineInspectorTab);
        return;
    }

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

    // 📐 The Inspect call at the header's trailing edge — jumps the leaf to the selected record's
    //    properties, the same travel Tab performs one step at a time.
    {
        const char* Caption = "Inspect";
        const float Run     = Scaled.RunSecondary;
        const float PadX    = Scaled.HeaderPadX * 0.8f;
        const float CallSpan = PadX * 2.0f + Surface->MeasureRun(Caption, Run, 0.0f);

        const PlaneExtent Call = Spanning(Header.MaximumX - PadX - CallSpan,
                                          Header.MinimumY + (Header.Height() - 24.0f) * 0.5f,
                                          CallSpan, 24.0f);

        const bool OnCall = Call.Encloses(Sampled.PositionX, Sampled.PositionY);

        if (Sampled.ContactPressed && OnCall && !Ledger->AnyDisclosed())
            Ledger->Grab(InspectCall, ControlPart::Body);

        if (OnCall && Ledger->Released(InspectCall))
            Applied.OutlinePage = 1u;

        Ledger->DeclareHovered(InspectCall, OnCall, HoverOver);

        if (OnCall)
            Surface->Ground(Call, Tinted.TileHovered, Scaled.FieldRadius, CornerAll);

        Surface->TextRun(Call.MinimumX + PadX,
                         Call.MinimumY + (Call.Height() - Run) * 0.5f,
                         OnCall ? Tinted.Primary : Tinted.Muted, Caption, Run);
    }

    const PlaneExtent Footer = Spanning(Outlining.MinimumX, Outlining.MaximumY - Scaled.FooterHeight,
                                        Outlining.Width(), Scaled.FooterHeight);

    // 📐 The page strip between the body and the footer: Directory | Properties | History. The strip
    //    writes the same `OutlinePage` Tab cycles, so the two can never disagree.
    const PlaneExtent Strip = Spanning(Outlining.MinimumX, Footer.MinimumY - Scaled.ComponentY,
                                       Outlining.Width(), Scaled.ComponentY);

    static const char* const PageCaptions[3] = { "Directory", "Properties", "History" };
    const TabDeclaration PageDeclared{ PageCaptions, 3u };
    static_cast<void>(Controls.TabStrip(OutlineStrip, Strip, PageDeclared, Applied.OutlinePage));

    const PlaneExtent Body = Spanning(Outlining.MinimumX + Pad, Header.MaximumY + Pad,
                                      Outlining.Width() - Pad * 2.0f,
                                      Strip.MinimumY - Header.MaximumY - Pad);

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
                                           const EntityRevision* Revisions, std::uint32_t RevisionCount,
                                           std::uint32_t& InspectorTab)
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

    static_cast<void>(Controls.TabStrip(InspectorStrip, Strip, Declared, InspectorTab));

    const PlaneExtent Pages = Spanning(Extent.MinimumX, Strip.MaximumY, Extent.Width(),
                                       Extent.MaximumY - Strip.MaximumY - Scaled.FooterHeight);

    // 📐 The two pages sit side by side and the strip travels between them, exactly as the shell's
    //    inspector does — the travel is one whole page extent.
    const float Carried = (InspectorTab == 1u) ? -Pages.Width() : 0.0f;

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
                         (InspectorTab == 0u) ? "Properties" : "History", FooterRun);
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

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE GIZMO
//------------------------------------------------------------------------------------------------------------------------

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

}   // namespace Slate
