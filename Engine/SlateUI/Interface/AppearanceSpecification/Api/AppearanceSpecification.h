//============================================================================================================================================
//                                                        APPEARANCESPECIFICATION.H
//============================================================================================================================================
// 🧩 Every ink and every measured extent the interface draws with — resolved once against the display scale, then read.

#pragma once

#include <cstdint>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                          INK
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 One display-referred ink, packed at eight bits per component.
/// note  ⚠️ Display-referred. `08` §3.1 places the interface after the tone projection, so nothing declared
///       here is ever tone-mapped a second time.
/// tag   contract, nonallocating, nonthrowing
struct InkOrdinate
{
    std::uint8_t  Red     = 0u;     // [-] - sRGB-encoded, never linear
    std::uint8_t  Green   = 0u;     // [-]
    std::uint8_t  Blue    = 0u;     // [-]
    std::uint8_t  Opacity = 255u;   // [-] - 255 is fully covering
};

/// 🧩 Constructs a fully covering ink from a packed 0xRRGGBB literal.
/// cost  ✔️
constexpr InkOrdinate Covering(std::uint32_t Packed)
{
    return InkOrdinate{ static_cast<std::uint8_t>((Packed >> 16) & 0xFFu),
                        static_cast<std::uint8_t>((Packed >>  8) & 0xFFu),
                        static_cast<std::uint8_t>( Packed        & 0xFFu),
                        255u };
}

/// 🧩 Constructs an ink at a declared coverage, matching CSS `color-mix(… n%, transparent)`.
/// in    Packed    [-]  0xRRGGBB
/// in    Coverage  [-]  zero is invisible, one is fully covering
/// cost  ✔️
constexpr InkOrdinate Partial(std::uint32_t Packed, double Coverage)
{
    InkOrdinate Constructed = Covering(Packed);
    Constructed.Opacity     = static_cast<std::uint8_t>(Coverage * 255.0 + 0.5);
    return Constructed;
}

/// 🧩 Scales an ink's coverage without disturbing its chromaticity — what a fade interpolant drives.
/// cost  ✔️
constexpr InkOrdinate Faded(InkOrdinate Standing, double Coverage)
{
    Standing.Opacity = static_cast<std::uint8_t>(static_cast<double>(Standing.Opacity) * Coverage + 0.5);
    return Standing;
}

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE NEUTRAL LADDER
//------------------------------------------------------------------------------------------------------------------------

// 📐 🔴 The source declares its greys as `oklch(L 0 none)` — chroma exactly zero. For a neutral, the Oklab
//    coefficients sum to 0.9999999935, so L = Y^(1/3) to within a part in 10⁸ and Y = L³ exactly enough. The
//    sRGB transfer then gives the ordinates below. Nine of the ten agree with the hex table everyone recalls;
//    `NeutralFourHundred` does **not** — it resolves to 0xA1A1A1 and not to 0xA3A3A3, because the source is
//    Tailwind v4, whose palette was re-declared in Oklch rather than converted from the earlier hexes.
// ⚠️ Transcribing that one value from memory is precisely the sixteenth-place seam `ToleranceContract.h`
//    exists to prevent — two panels disagreeing by two ordinates with nothing in the build comparing them.
inline constexpr std::uint32_t NeutralOneHundred    = 0xF5F5F5u;   // [-] - oklch(97%   0 none)
inline constexpr std::uint32_t NeutralTwoHundred    = 0xE5E5E5u;   // [-] - oklch(92.2% 0 none)
inline constexpr std::uint32_t NeutralThreeHundred  = 0xD4D4D4u;   // [-] - oklch(87%   0 none)
inline constexpr std::uint32_t NeutralFourHundred   = 0xA1A1A1u;   // [-] - oklch(70.8% 0 none)  🔴 not A3A3A3
inline constexpr std::uint32_t NeutralFiveHundred   = 0x737373u;   // [-] - oklch(55.6% 0 none)
inline constexpr std::uint32_t NeutralSixHundred    = 0x525252u;   // [-] - oklch(43.9% 0 none)
inline constexpr std::uint32_t NeutralSevenHundred  = 0x404040u;   // [-] - oklch(37.1% 0 none)
inline constexpr std::uint32_t NeutralEightHundred  = 0x262626u;   // [-] - oklch(26.9% 0 none)
inline constexpr std::uint32_t NeutralNineHundred   = 0x171717u;   // [-] - oklch(20.5% 0 none)
inline constexpr std::uint32_t NeutralNineFifty     = 0x0A0A0Au;   // [-] - oklch(14.5% 0 none)
inline constexpr std::uint32_t AbsoluteBlack        = 0x000000u;   // [-] - --color-black

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE RESOLVED INKS
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every ink the interface draws with, named by the responsibility it carries rather than by its ladder rung.
/// note  A second appearance is a second filled instance of this record and nothing else — no call site names
///       a ladder rung directly, so no call site has to be revisited to add one.
/// tag   contract, nonallocating, nonthrowing
struct SurfaceInk
{
    InkOrdinate  SurfaceGround       = Covering(NeutralNineFifty);        // [-] - workspace ground, preview rail
    InkOrdinate  SurfaceStanding     = Covering(NeutralNineHundred);      // [-] - drawer body, preview box
    InkOrdinate  SurfaceSunken       = Covering(AbsoluteBlack);           // [-] - library rail, tab tongue
    InkOrdinate  SurfaceRaised       = Covering(NeutralEightHundred);     // [-] - text entry, active toggle, bar
    InkOrdinate  SurfaceLifted       = Covering(NeutralFiveHundred);      // [-] - roused medallion

    InkOrdinate  CardGround          = Partial(NeutralEightHundred, 0.40);// [-] - bg-neutral-800/40
    InkOrdinate  CardGroundRoused    = Partial(NeutralSevenHundred, 0.60);// [-] - hover:bg-neutral-700/60
    InkOrdinate  CardEdge            = Partial(NeutralSevenHundred, 0.50);// [-] - border-neutral-700/50
    InkOrdinate  CardEdgeRoused      = Covering(NeutralFiveHundred);      // [-] - hover:border-neutral-500
    InkOrdinate  MedallionGround     = Partial(NeutralSevenHundred, 0.50);// [-] - the 32 px and 40 px discs
    InkOrdinate  MedallionRoused     = Covering(NeutralFiveHundred);      // [-] - group-hover:bg-neutral-500

    InkOrdinate  GroupGroundTaken    = Partial(NeutralNineHundred, 0.40); // [-] - bg-neutral-900/40
    InkOrdinate  GroupGroundRoused   = Partial(NeutralNineHundred, 0.20); // [-] - hover:bg-neutral-900/20
    InkOrdinate  SubjectGroundTint   = Partial(NeutralNineHundred, 0.10); // [-] - bg-neutral-900/10
    InkOrdinate  SubjectGroundTaken  = Partial(NeutralNineHundred, 0.60); // [-] - bg-neutral-900/60

    InkOrdinate  EdgeQuiet           = Covering(NeutralEightHundred);     // [-] - every 1 px divider
    InkOrdinate  EdgeFaint           = Partial(NeutralEightHundred, 0.50);// [-] - border-neutral-800/50
    InkOrdinate  GripPill            = Covering(NeutralSixHundred);       // [-] - the 48 × 6 pill
    InkOrdinate  MeterDot            = Covering(NeutralSevenHundred);     // [-] - the 4 px meta separator

    InkOrdinate  InkPrimary          = Covering(NeutralOneHundred);       // [-] - titles, taken rows
    InkOrdinate  InkRoused           = Covering(NeutralTwoHundred);       // [-] - hovered group row
    InkOrdinate  InkTertiary         = Covering(NeutralThreeHundred);     // [-] - card caption, hovered subject
    InkOrdinate  InkMuted            = Covering(NeutralFourHundred);      // [-] - quiet group row, quiet toggle
    InkOrdinate  InkFaint            = Covering(NeutralFiveHundred);      // [-] - quiet subject, meta, captions
    InkOrdinate  InkGhost            = Covering(NeutralSixHundred);       // [-] - the LIBRARY caption

    InkOrdinate  RailTaken           = Covering(NeutralOneHundred);       // [-] - the 3 px selection rail
    InkOrdinate  RailGlow            = Partial(0xFFFFFFu, 0.80);          // [-] - 0 0 8px rgba(255,255,255,.8)
    InkOrdinate  RailQuiet           = Partial(AbsoluteBlack, 0.00);      // [-] - bg-transparent

    InkOrdinate  ScrimTop            = Partial(NeutralNineHundred, 0.80); // [-] - from-neutral-900/80
    InkOrdinate  ScrimBottom         = Partial(NeutralNineHundred, 0.00); // [-] - to-transparent
    InkOrdinate  DrawerShadow        = Partial(AbsoluteBlack, 0.50);      // [-] - 0 ±20px 60px
    InkOrdinate  TongueShadowNorth   = Partial(AbsoluteBlack, 0.12);      // [-] - drop-shadow-md, 0 3px 3px
    InkOrdinate  TongueShadowSouth   = Partial(AbsoluteBlack, 0.25);      // [-] - 0 -4px 10px
    InkOrdinate  RecessShadow        = Partial(AbsoluteBlack, 0.05);      // [-] - inset 0 2px 4px
    InkOrdinate  FocusRing           = Covering(NeutralFiveHundred);      // [-] - focus:ring-1 ring-neutral-500
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE MEASURED SCALE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every extent the source states, already multiplied by the display scale.
/// note  🔴 Multiplied **once**, at resolve. A call site that scales again produces a panel correct at exactly
///       one display scale, and the defect only appears on a second machine.
/// tag   contract, nonallocating, nonthrowing
struct MetricScale
{
    float  SpacingUnit          =   4.0f;   // [px] - --spacing, 0.25rem; every padding is a multiple

    float  RadiusFine           =   4.0f;   // [px] - rounded
    float  RadiusSmall          =   8.0f;   // [px] - rounded-lg
    float  RadiusMedium         =  12.0f;   // [px] - rounded-xl
    float  RadiusGrand          =  16.0f;   // [px] - rounded-2xl

    float  TextFine             =  10.0f;   // [px] - text-[10px]
    float  TextSmall            =  12.0f;   // [px] - text-xs
    float  TextBody             =  14.0f;   // [px] - text-sm
    float  TextTitle            =  24.0f;   // [px] - text-2xl

    // 📐 The sheet's own line heights. `html` declares 1.5, and three size utilities override it:
    //    text-xs is calc(1 / .75), text-sm is calc(1.25 / .875), text-2xl is calc(2 / 1.5). text-[10px]
    //    is arbitrary and therefore inherits the 1.5. A row height derived from the point size alone is
    //    short by four pixels on every group row, and fourteen groups accumulate that into a visible drift.
    float  LeadingFine          =  15.0f;   // [px] - text-[10px] at the inherited 1.5
    float  LeadingSmall         =  16.0f;   // [px] - text-xs
    float  LeadingBody          =  20.0f;   // [px] - text-sm
    float  LeadingTitle         =  32.0f;   // [px] - text-2xl

    float  WheelTravel          = 100.0f;   // [px] - one wheel notch, as the host reports it

    float  TrackingTight        =  -0.025f; // [em] - tracking-tight
    float  TrackingWide         =   0.025f; // [em] - tracking-wide
    float  TrackingWider        =   0.05f;  // [em] - tracking-wider
    float  TrackingWidest       =   0.20f;  // [em] - tracking-[0.2em]

    float  TongueAlong          = 220.0f;   // [px] - the drawer tab
    float  TongueAcross         =  36.0f;   // [px]
    float  TongueClipFraction   =   0.08f;  // [-]  - polygon inset, 8 % of TongueAlong per side
    float  TongueGapAlong       =  10.0f;   // [px] - gap-2.5 between symbol and caption
    float  TonguePadAlong       =  24.0f;   // [px] - px-6

    float  GripAlong            =  48.0f;   // [px] - w-12
    float  GripAcross           =   6.0f;   // [px] - h-1.5
    float  GripStripAcross      =  40.0f;   // [px] - h-10, the south drawer's grip strip
    float  GripLiftNorth        =  24.0f;   // [px] - bottom-6, the north drawer's grip

    float  RailAcross           =   3.0f;   // [px] - w-[3px]
    float  RailGlowSpread       =   8.0f;   // [px] - 0 0 8px

    float  SymbolChevron        =  16.0f;   // [px] - w-4 h-4
    float  SymbolTongue         =  16.0f;   // [px] - w-4 h-4, stroked at 2.5
    float  SymbolToggle         =  20.0f;   // [px] - w-5 h-5
    float  SymbolVacant         =  32.0f;   // [px] - w-8 h-8, the empty-result magnifier

    float  MedallionLattice     =  32.0f;   // [px] - w-8 h-8
    float  MedallionColumn      =  40.0f;   // [px] - w-10 h-10
    float  MedallionPreview     =  48.0f;   // [px] - w-12 h-12

    float  LibraryAlongMedium   = 224.0f;   // [px] - md:w-56
    float  LibraryAlongLarge    = 256.0f;   // [px] - lg:w-64
    float  PreviewAlongMedium   = 192.0f;   // [px] - w-48
    float  PreviewAlongLarge    = 256.0f;   // [px] - lg:w-64

    float  LibraryPadAlong      =  24.0f;   // [px] - px-6
    float  LibraryCaptionAcross =  24.0f;   // [px] - py-6
    float  GroupPadAcross       =  10.0f;   // [px] - py-2.5
    float  GroupGapAcross       =   4.0f;   // [px] - gap-1
    float  SubjectIndentAlong   =  40.0f;   // [px] - pl-10
    float  SubjectPadTrailing   =  24.0f;   // [px] - pr-6
    float  SubjectStripPad      =   6.0f;   // [px] - py-1.5

    float  ContentPad           =  24.0f;   // [px] - p-6
    float  ContentPadLeading    =  16.0f;   // [px] - pt-4
    float  ContentHeadAcross    =  40.0f;   // [px] - h-10
    float  ContentHeadPadAlong  =   8.0f;   // [px] - px-2
    float  ContentHeadGap       =  24.0f;   // [px] - mb-6
    float  ContentTrailingPad   =  48.0f;   // [px] - pb-12
    float  ContentScrollPad     =   8.0f;   // [px] - pr-2

    float  EntryAlongCeiling    = 320.0f;   // [px] - max-w-xs, 20rem
    float  EntryPadAlong        =  16.0f;   // [px] - px-4
    float  EntryPadAcross       =   6.0f;   // [px] - py-1.5
    float  TogglePad            =   8.0f;   // [px] - p-2
    float  ToggleGap            =   8.0f;   // [px] - gap-2

    float  CardGapLattice       =  16.0f;   // [px] - gap-4
    float  CardGapColumn        =   8.0f;   // [px] - gap-2
    float  CardPadColumn        =  12.0f;   // [px] - p-3
    float  CardGapColumnInner   =  16.0f;   // [px] - gap-4
    float  CardScrimAcross      =  36.0f;   // [px] - p-3 above and below a 12 px caption
    float  CardMetaGap          =   8.0f;   // [px] - gap-2
    float  CardMetaLift         =   2.0f;   // [px] - mt-0.5
    float  CardMetaDot          =   4.0f;   // [px] - w-1 h-1

    float  PreviewGap           =  24.0f;   // [px] - gap-6
    float  PreviewPad           =  24.0f;   // [px] - p-6
    float  PreviewBoxFloor      =  80.0f;   // [px] - min-h-[80px]
    float  PreviewBoxCeiling    = 240.0f;   // [px] - max-h-[240px]
    float  SkeletonGapUpper     =  12.0f;   // [px] - space-y-3
    float  SkeletonGapLower     =   8.0f;   // [px] - space-y-2
    float  SkeletonLeading      =  16.0f;   // [px] - pt-4 above the lower group

    float  BreakpointSmall      = 640.0f;   // [px] - 40rem
    float  BreakpointMedium     = 768.0f;   // [px] - 48rem
    float  BreakpointLarge      = 1024.0f;  // [px] - 64rem

    float  DisplayScale         =   1.0f;   // [-]  - what every extent above was multiplied by
};

// 📝 The five preview bars, as the source states them: two in the upper group at 16 px and 12 px, three in the
//    lower at 8 px. The fractions are of the rail's inner extent. Declared here rather than at the recording
//    site because `AssetPanel` records them and the next appearance will re-measure them.
inline constexpr float SkeletonBarAcross[5]   = { 16.0f, 12.0f,  8.0f,  8.0f,  8.0f };   // [px]
inline constexpr float SkeletonBarFraction[5] = {  0.75f, 0.50f, 1.00f, 1.00f, 0.80f };  // [-]

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE MOTION SCALE
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 Every duration and every spring coefficient the source declares.
/// note  📐 ζ = 35 / (2√350) ≈ 0.9354, so every drawer transition overshoots slightly before settling. A
///       linear ease at the same duration reads as a different product, which is why the coefficients travel
///       rather than a duration.
/// tag   contract, nonallocating, nonthrowing
struct MotionScale
{
    double  DrawerStiffness      = 350.0;   // [-]  - spring, mass one
    double  DrawerDamping        =  35.0;   // [-]
    double  DragElasticity       =   0.05;  // [-]  - travel admitted beyond a constraint
    double  DiscloseDuration     = 150.0;   // [ms] - accordion height, colour fade
    double  RouseDuration        = 200.0;   // [ms] - whileHover
    double  CardArrivalDuration  = 400.0;   // [ms] - the entry motion
    double  CardArrivalStagger   =  30.0;   // [ms] - multiplied by (ordinal mod 10)
    double  CardArrivalLift      =  10.0;   // [px] - y: 10 → 0
    double  CardArrivalScale     =   0.95;  // [-]  - scale: 0.95 → 1
    double  CardRouseScale       =   1.05;  // [-]  - lattice hover
    double  CardRouseTravel      =   4.0;   // [px] - column hover, x: 4
    double  ArrivalMargin        =  50.0;   // [px] - viewport margin the entry motion fires at

    // 📐 🔴 The arbitration's own figures, transcribed literally. Three rates and two fractions, and the
    //    three rates are **not** one rate scaled: the source states 300 for the north drawer and for the
    //    south drawer's half pose, 500 for the outer gate of closed and full, and 1000 for their inner
    //    gate. A single rate with multipliers agrees with the source at exactly one of the five sites.
    double  SnapRateSoft         = 300.0;   // [px/s] - north; south half, both directions
    double  SnapRateFirm         = 500.0;   // [px/s] - south closed and full, outer gate
    double  SnapRateHard         = 1000.0;  // [px/s] - south closed and full, inner gate
    double  SnapFractionNear     =   0.25;  // [-]    - h/4, and h*.25
    double  SnapFractionFar      =   0.75;  // [-]    - h*.75
};

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE RESOLVED RECORD
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The one record every panel reads. Resolved at bring-up and again only when the display scale changes.
/// tag   contract, nonallocating, nonthrowing
struct AppearanceSpecification
{
    SurfaceInk   Ink     = {};
    MetricScale  Measure = {};
    MotionScale  Motion  = {};
};

/// 🧩 Resolves the appearance against one display scale.
/// in    DisplayScale  [-]  what the window system reports; values at or below zero resolve at one
/// out   Appearance    [-]  every extent already multiplied; nothing downstream multiplies again
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
AppearanceSpecification Resolve(double DisplayScale);

/// 🧩 How many lattice columns the content extent admits, from the source's four breakpoints.
/// in    ContentAlong  [px] the extent the lattice is arranged inside
/// out   Columns       [-]  two below 640 px, then three, four, and five at and above 1024 px
/// cost  ✔️
/// tag   api, nonallocating, nonthrowing
std::uint32_t LatticeColumns(const MetricScale& Measure, float ContentAlong);

}   // namespace Slate
