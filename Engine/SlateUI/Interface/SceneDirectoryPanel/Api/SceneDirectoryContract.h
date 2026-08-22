//============================================================================================================================================
//                                                     SCENEDIRECTORYCONTRACT.H
//============================================================================================================================================
// 🧩 The shared scene-directory contracts: what an entity is, what the environment is,
//    and what one revision is. Owned here — beside the editor's scene-directory panel —
//    so the validation shell and the editor's outliner/properties leaves describe the
//    same things with the same names.
//
//    🔴 THESE LIVE OUTSIDE THE SHELL ON PURPOSE. The validation shell
//       (`GlobalShellPanel`) is a PROTOTYPE: it presents the full reference sheet
//       (options rail, texture-paint layer stack, CAD drafting) and must NEVER be
//       recorded by the editor host. The editor presents workspaces and their
//       panels; it shares only the scene-directory data with the shell. Future
//       agents: build editor content in SceneDirectoryPanel, never by porting the
//       shell into the editor.

#pragma once

#include "SlateUI/Interface/AppearanceSpecification/Api/AppearanceSpecification.h"
#include "SlateUI/Interface/SymbolSpecification/Api/SymbolSpecification.h"

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE REFERENCE EXTENTS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every measured extent the reference states, before the display and artist factors are applied.
/// note  📐 Each figure is the literal from `app/page.tsx` or its component, named rather than repeated. The
///        reference writes them as Tailwind arbitrary values — `h-[36px]`, `w-[220px]` — so the transcription
///        can be checked against the sheet line by line.
/// tag   contract, nonallocating, nonthrowing
struct ShellMetric
{
    float  TopBarHeight    =  36.0f;   // [px] - h-[36px]
    float  TopBarPadX  =  14.0f;   // [px] - px-[14px]
    float  OptionsX    = 220.0f;   // [px] - w-[220px]
    float  HeaderHeight    =  46.0f;   // [px] - h-[46px], every pane header
    float  HeaderPadX  =  10.0f;   // [px] - px-[10px]
    float  RowHeight       =  32.0f;   // [px] - h-[32px], an outline row
    float  RowStepX    =  15.0f;   // [px] - depth * 15
    float  RowLeadX    =   8.0f;   // [px] - paddingLeft 8 + depth * 15
    float  ChevronExtent   =  15.0f;   // [px] - the w-[15px] disclosure cell
    float  GlyphExtent     =  18.0f;   // [px] - the w-[18px] classification cell
    float  RailX       =   3.0f;   // [px] - the w-[3px] selection rail
    float  RailY      =  15.0f;   // [px] - h-[15px]
    float  RailOffsetX =   7.0f;   // [px] - left-[-7px]
    float  SearchHeight    =  30.0f;   // [px] - h-[30px], the filter field
    float  FooterHeight    =  26.0f;   // [px] - h-[26px]
    float  InspectorX  = 700.0f;   // [px] - w-[700px], docked and summoned alike
    float  SummonedY  = 400.0f;   // [px] - h-[400px], the summoned card
    float  OutlinerX   = 350.0f;   // [px] - grid-cols-[350px_minmax(0,1fr)]
    float  MedallionExtent =  24.0f;   // [px] - the w-6 h-6 header medallion
    float  CardRadius      =  12.0f;   // [px] - --r-tile
    float  MenuRadius      =  18.0f;   // [px] - --r-menu
    float  FieldRadius     =   6.0f;   // [px] - rounded-md
    float  StatusY    =  28.0f;   // [px] - the viewport's h-[28px] hint strip
    float  WeaveFineStep   =  28.0f;   // [px] - backgroundSize 28px
    float  WeaveCoarseStep = 140.0f;   // [px] - backgroundSize 140px
    float  ComponentY =  31.0f;   // [px] - h-[31px], a component card header

    // 📐 The metadata pane, from `components/MetadataPane.tsx`.
    float  HeroCrest       =  34.0f;   // [px] - the w-[34px] hero tile
    float  HeroPad         =  10.0f;   // [px] - p-2.5 around the hero
    float  StatY      =  28.0f;   // [px] - h-[28px], one hairline stat row
    float  AdvanceY   =  32.0f;   // [px] - h-[32px], the Properties & History call
    float  ActionY    =  29.0f;   // [px] - h-[29px], one inline action
    float  ActionGlyph     =  15.0f;   // [px] - the w-[15px] leading glyph cell
    float  ChipExtent      =   8.0f;   // [px] - the w-2 h-2 footer hue chip
    float  SwatchExtent    =  16.0f;   // [px] - the w-4 h-4 albedo disc
    float  PillPadX    =   8.0f;   // [px] - px-2 inside the Tab pill
    float  RunFiner        =   9.5f;   // [px] - text-[9.5px]

    // 📐 The Context Menu surface, from `References/remix-notch-ui/src/components/Outliner.tsx`.
    float  ContextX    = 192.0f;   // [px] - w-48, the floating card's extent along
    float  ContextRow      =  30.0f;   // [px] - px-2 py-1.5 text-sm, one action row
    float  ContextSwatch   =  20.0f;   // [px] - the w-5 h-5 colour disc
    float  ContextPad      =   4.0f;   // [px] - p-1 inside the card
    float  ContextClamp    = 200.0f;   // [px] - `window.innerWidth - 200`, the overlay's own clamp
    float  KebabDot        =   2.0f;   // [px] - one dot of the per-row kebab
    float  KebabExtent     =  18.0f;   // [px] - the cell the three dots are centred in
    float  PanePad         =   7.0f;   // [px] - p-[7px]
    float  RunPrimary      =  12.5f;   // [px] - text-[12.5px]
    float  RunSecondary    =  11.5f;   // [px] - text-[11.5px]
    float  RunSmall        =  10.5f;   // [px] - text-[10.5px]
    float  RunFine         =  10.0f;   // [px] - text-[10px]

    // 📐 The Layer Stack, from `components/TexturePaint.tsx`.
    float  LayerHeadHeight =  44.0f;   // [px] - h-[44px], the row's top half
    float  LayerSpineX =  30.0f;   // [px] - w-[30px], the spine gutter
    float  LayerSpineWidth =   3.0f;   // [px] - w-[3px], the spine itself
    float  LayerBadge      =  20.0f;   // [px] - the w-[20px] ordinal badge
    float  LayerSwatch     =  26.0f;   // [px] - the w-[26px] paint swatch
    float  LayerAction     =  20.0f;   // [px] - a w-[20px] eye, cross or bin
    float  LayerGap        =   6.0f;   // [px] - gap-[6px]
    float  LayerRowPad     =   6.0f;   // [px] - px-[6px]
    float  LayerRowGap     =   5.0f;   // [px] - pb-[5px] between rows
    float  LayerToolHeight =  28.0f;   // [px] - h-[28px], the Add layer button
    float  LayerFoldPad    =   8.0f;   // [px] - p-[8px] inside the folded half
    float  LayerFieldRow   =  26.0f;   // [px] - min-h-[26px], a folded property row
    float  LayerLabelX =  50.0f;   // [px] - grid-cols-[50px_minmax(0,1fr)]
    float  LayerPillY =  20.0f;   // [px] - h-[20px], a channel pill
    float  LayerSwitchX=  26.0f;   // [px] - the w-[26px] invert switch
    float  LayerSwitchHeight= 14.0f;   // [px] - h-[14px]
    float  LayerRadius     =   8.0f;   // [px] - rounded-[8px]

    // 📐 The LayerstackV1 reference's own metrics, from `References/LayerstackV1.html`.
    float  LayerRowY       =  45.0f;   // [px] - min-height:45px, one stack row
    float  LayerMaskY      =  37.0f;   // [px] - min-height:37px, the attached mask row
    float  LayerThumbY     =  35.0f;   // [px] - w-[35px] square preview
    float  LayerBadgeY     =  15.0f;   // [px] - the w-[15px] badge on the thumb's corner
    float  LayerChipY      =  18.0f;   // [px] - h-[18px], one chip on a row
    float  LayerTagX       =   3.0f;   // [px] - the w-[3px] colour tag on the entry's edge
    // 📐 The attached mask entry's dotted rail — LayerstackV1.html `.tag.dot`:
    //        repeating-linear-gradient(180deg, var(--c) 0 3px, transparent 3px 7px)
    //    🔴 These were raw literals at the two call sites. Every OTHER length here is
    //       multiplied by the display factor in ScaleShellLengths, so at any scale but
    //       1.0 the rail grew while its dots kept their 1.0 rhythm — the dotting drifted
    //       out of step with the row it marks, which is why it still looked wrong on a
    //       scaled display while a 1.0 harness render measured correct.
    float  LayerTagDotOn   =   3.0f;   // [px] - colour carried by each dot
    float  LayerTagDotStep =   7.0f;   // [px] - the gradient's period
    float  LayerKidsX      =  15.0f;   // [px] - the folder children's margin-left
    float  LayerMaskIndent =  26.0f;   // [px] - padding-left of the attached mask row
    float  LayerFootCrumb  =  18.0f;   // [px] - the footer's crumb line
    float  LayerFootProp   =  35.0f;   // [px] - the footer's blend + opacity row
    float  LayerFootBar    =  38.0f;   // [px] - the footer's action bar
};

// 📐 The slide travel, which is a duration and not a length and so is never scaled.

/// 🧩 Applies the resolved display and artist factors to every extent the reference declares.
/// in    Factor  [-]  the same product `ThemeProfile` applies to its own interface lengths
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
ShellMetric ScaleShellLengths(float Factor);

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE OUTLINE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 What one entity in the scene directory is, which decides its glyph and its hue.
/// note  🔴 The reference spells this `GameNodeType`, which carries two banned spellings. The discriminating
///        mechanism is the entity's discipline, so it is named for that. Ordinals follow the reference's own
///        declaration order so the two records can be read side by side.
/// tag   contract
enum class EntitySubject : std::uint32_t
{
    Level        = 0u,   // [-] - the level root
    Grouping     = 1u,   // [-] - a folder in the outliner
    Actor        = 2u,   // [-] - a placed prefab
    Camera       = 3u,   // [-] - a view
    Illuminant   = 4u,   // [-] - a light
    Audio        = 5u,   // [-] - an emitter
    Particle     = 6u,   // [-] - a visual effect
    Trigger      = 7u,   // [-] - a volume
    Script       = 8u,   // [-] - a behaviour
    // 📐 The editor's environment, appended rather than inserted: the reference's ordinals above are the
    //    validation sheet's g_NN identities, so they must not move. `Sun` and `Sky` are what the editor
    //    registers under Lighting so the inspector can branch its cards on them.
    Sun          = 9u,   // [-] - the directional illuminant the sky responds to
    Sky          = 10u,  // [-] - the atmosphere shell and its aerial perspective
    SubjectCount = 11u   // [-] - the closed count, never a subject
};

/// 🧩 The glyph one entity subject is drawn with, from the reference's own `ICONS` record.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
SymbolSubject EntityGlyph(EntitySubject Subject);

/// 🧩 The hue one entity subject carries, from the reference's own `COLORS` record.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
ThemeToken EntityHue(EntitySubject Subject);

/// 🧩 The run naming one entity subject, as the reference's `capitalize` rule presents it.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* EntityText(EntitySubject Subject);

/// 🧩 One row of the scene directory, linearised and carrying its own depth.
/// note  📝 The reference holds a nested graph and renders it recursively. A linear sequence carrying depth
///       records identically and needs no recursion inside a tick, which is what keeps the panel allocation
///       free; the enclosing ordinal is retained so disclosure and presence still propagate inward.
/// tag   contract, nonallocating, nonthrowing
struct EntityRow
{
    const char*    Naming           = "";                     // [-] - borrowed; outlives the tick
    EntitySubject  Subject          = EntitySubject::Actor;   // [-] - what it is
    std::uint32_t  Depth            = 0u;                     // [-] - indentation steps from the level
    std::uint32_t  Enclosing        = 0xFFFFFFFFu;            // [-] - the row holding it; absent for the level
    std::uint32_t  EnclosedCount    = 0u;                     // [-] - zero presents no disclosure mark
    // 📝 The row's search tags — a space-separated run, borrowed like `Naming`. The scene
    //    directory's filter matches the name OR the tags, so an artist can find "the fly cam" by
    //    searching "fly" even though the row is named "Editor Camera". The run is empty by default.
    const char*    Tagged           = "";                     // [-] - borrowed; "sun light directional"
};

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE ENVIRONMENT
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every parameter the editor's sun, sky and atmosphere present, owned by the host and written through
///    by the inspector's slider cards.
/// note  🔴 Phase 1 presents these as editable ordinates and renders a stylised sky from them. The values are
///        exactly what the atmosphere shaders will read in phase 2, so the property surface does not move
///        when the renderer stops being a placeholder.
/// tag   contract, nonallocating, nonthrowing
struct EnvironmentConfiguration
{
    double SunElevation = 35.0;       // [deg] - above the horizon, 0…90
    double SunAzimuth   = 120.0;      // [deg] - clockwise from north, 0…360
    double SunIntensity = 4.8;        // [lx]  - the directional illuminant's illuminance
    double SunTemperature = 5500.0;   // [K]   - the sun's colour temperature, 1000…12000
    double SunDiscRadius = 8.0;       // [-]   - multiplier on SunAngularRadius for the baked disc
    double SunDiscIntensity = 0.95;   // [-]   - the direct term's strength, 0…4
    double SkyIntensity = 1.0;        // [-]   - the sky dome's luminance scale, 0…3
    double SkyTurbidity = 2.0;        // [-]   - the atmosphere's turbidity, 1…10
    double AtmosphereDensity = 1.0;   // [-]   - the Rayleigh density scale, 0…3
    double AtmosphereScaleHeight = 1.0; // [-] - the density fall-off height, 0.2…3
    double MieDensity = 1.0;          // [-]   - the Mie scattering scale, 0…4
    double MieAsymmetry = 0.80;       // [-]   - the Mie forward-scattering asymmetry, -0.95…0.95
    double DomeResolution = 1.0;      // [-]   - 0 = 512, 1 = 1024, 2 = 2048; selects the bake extent
};

/// 🧩 The sky dome's own camera, stated in plain numbers so the panel never names an Application type.
/// note  📐 The dome is direction-indexed: azimuth across the width, elevation down the height. The
///        viewport crops it to this camera's field of view.
/// tag   contract, nonallocating, nonthrowing
struct SkyViewCamera
{
    float AzimuthDegrees    = 0.0f;   // [deg] - the view direction's azimuth
    float ElevationDegrees  = -6.0f;  // [deg] - above the horizon, negative looks down
    float FieldOfViewDegrees = 60.0f; // [deg] - the vertical field of view
};

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE HISTORY
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Which class one revision belongs to, which decides its bubble hue and its secondary glyph.
/// note  📐 `REVISION_CLASS` and `REVISION_HUE` from `components/Inspector.tsx`, in the reference's own
///        declaration order. It spells the discriminator `category`, which `SKILL-Naming` retires along
///        with `Kind` and `Type`; the discriminating mechanism is what the revision DID, so it is named
///        for that, exactly as `EntitySubject` beside it already is.
/// tag   contract
enum class RevisionSubject : std::uint32_t
{
    Start         = 0u,   // [-] - #7ec8ff
    Feature       = 1u,   // [-] - #ffb24d
    Parameter     = 2u,   // [-] - #4fd18b, the reference's `param`
    Sketch        = 3u,   // [-] - #37d6d6
    Relocate      = 4u,   // [-] - #5b8cff, the reference's `transform`
    Grouped       = 5u,   // [-] - #b98bff, the reference's `body`
    Created       = 6u,   // [-] - #7ec8ff, the reference's `add`
    Amended       = 7u,   // [-] - #c99b6a, the reference's `edit`
    Dropped       = 8u,   // [-] - #ff6b6b, the reference's `drop`
    SubjectCount  = 9u    // [-] - the closed count, never a subject
};

/// 🧩 The hue one revision subject carries, from the reference's own `REVISION_HUE` record.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
ThemeToken RevisionHue(RevisionSubject Classified);

/// 🧩 The run naming one revision subject, from the reference's own `REVISION_CLASS` record.
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
const char* RevisionText(RevisionSubject Classified);

/// 🧩 One revision against one outline row, exactly as the reference's `revisions` run declares them.
/// note  📝 Borrowed for the tick on the same terms as `EntityRow`. The reference sorts by date; the run is
///        presented in the order it is handed over, so a host states it already ordered.
/// tag   contract, nonallocating, nonthrowing
struct EntityRevision
{
    const char*       Description = "";                          // [-] - borrowed; rev.title
    const char*       Secondary   = "";                          // [-] - borrowed; rev.subtitle
    const char*       TimeRun     = "";                          // [-] - borrowed; the formatted time
    const char*       Author      = "System";                    // [-] - borrowed; rev.author
    std::uint32_t     Against     = 0u;                          // [-] - which outline row it belongs to
    RevisionSubject  Classified = RevisionSubject::Amended;   // [-] - rev.category
};

/// 🧩 One slider drag's history demand, written by the inspector at drag END and drained by the host.
/// note  🔴 The demand is a single slot rather than an array because exactly one slider can end its drag in
///        one tick. The host appends it to its own revision run and clears the slot; the panel never owns
///        the run.
/// tag   contract, nonallocating, nonthrowing
struct RevisionDemand
{
    bool           Standing  = false;   // [-] - a drag ended and awaits the host's append
    std::uint32_t  Against   = 0u;      // [-] - which outline row the revision belongs to
    char           Caption[64] = {};    // [-] - the composed description, e.g. "Sun elevation"
    char           Secondary[64] = {};  // [-] - the composed before→after, e.g. "35.0° → 42.0°"
};

} // namespace Slate
