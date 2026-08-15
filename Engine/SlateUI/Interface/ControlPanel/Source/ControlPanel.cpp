//============================================================================================================================================
//                                                      CONTROLPANEL.CPP
//============================================================================================================================================
// 🧩 Top-centre slide-down control centre — a per-workspace property inspector. The notch mirrors the bottom asset shelf, settling
//    via spring dynamics. Pass one surfaces properties and settings; outliner, channels and layers paint a stub empty-state.

#include "SlateUI/Interface/ControlPanel/Api/ControlPanel.h"

#include <algorithm>
#include <cmath>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                            CONSTANTS
//------------------------------------------------------------------------------------------------------------------------

constexpr float ControlGripWidth       = 200.0f;   // [px]   - notch silhouette width (mirrors the asset shelf grip)
constexpr float ControlGripHeight      = 36.0f;    // [px]   - notch silhouette height
constexpr float ControlFinalizeOffset  = 120.0f;   // [px]   - pull past this on release -> snap open (else fall back to latch)
constexpr float ControlFlickVelocity   = 1000.0f;  // [px/s] - a fast flick overrides the distance test
constexpr float ControlBodyMinOpen     = 50.0f;    // [px]   - below this the drawer body is not yet a drag surface
constexpr int   ControlCurveSegments   = 24;       // [-]    - bezier subdivision per silhouette shoulder
constexpr float ControlMaxWidth        = 980.0f;   // [px]   - centred content column width cap

//------------------------------------------------------------------------------------------------------------------------
//                                                      INTERNAL FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

namespace
{
    // 📝 Rebuild the rail tabs for the active workspace. Properties and Settings are populated in pass one; other families
    //    paint a stub empty-state.
    void ResolveControlTabs(ControlShelf& Shelf, int ActiveWorkspace)
    {
        Shelf.Tabs.clear();
        auto Push = [&](const char* Name, int Family, bool Populated)
        {
            ControlCategoryTab Tab;
            Tab.Name      = Name;
            Tab.Family    = Family;
            Tab.Populated = Populated;
            Shelf.Tabs.push_back(Tab);
        };

        switch (ActiveWorkspace)
        {
            case 0: // Painting
                Push("Properties", ControlFamilyProperties, true);
                Push("Settings",   ControlFamilySettings,   true);
                Push("Channels",   ControlFamilyChannels,   false);
                Push("Layers",     ControlFamilyLayers,     false);
                break;
            case 1: // Modeling
                Push("Properties", ControlFamilyProperties, true);
                Push("Settings",   ControlFamilySettings,   true);
                Push("Outliner",   ControlFamilyOutliner,   false);
                break;
            case 2: // Draughting
                Push("Properties", ControlFamilyProperties, true);
                Push("Settings",   ControlFamilySettings,   true);
                Push("History",    ControlFamilyHistory,    false);
                break;
            case 3: // GameWorld
                Push("Properties", ControlFamilyProperties, true);
                Push("Settings",   ControlFamilySettings,   true);
                Push("Outliner",   ControlFamilyOutliner,   false);
                Push("Layers",     ControlFamilyLayers,     false);
                break;
            default:
                Push("Properties", ControlFamilyProperties, true);
                Push("Settings",   ControlFamilySettings,   true);
                break;
        }

        // 📝 Keep the selected family valid across a workspace switch: fall back to the first tab when the old family is gone.
        bool ActiveStillPresent = false;
        for (const ControlCategoryTab& Tab : Shelf.Tabs)
            if (Tab.Family == Shelf.ActiveFamily) ActiveStillPresent = true;
        if (!ActiveStillPresent && !Shelf.Tabs.empty())
            Shelf.ActiveFamily = Shelf.Tabs.front().Family;
    }

    // 📝 The notch silhouette for the top shelf: flat face DOWN (unlike the asset shelf which faces UP). The bracket path on a
    //    200x36 viewBox is M100 0 L0 0 C..C.. L.. C.. 200 0; the flat edge is at y = 0 (top), concave shoulders curve downward.
    void TraceControlSilhouette(float OriginX, float OriginY, float Width, float Height)
    {
        // 📝 Placeholder — actual draw list calls will be implemented when the Vulkan foreground draw list is wired.
        (void)OriginX;
        (void)OriginY;
        (void)Width;
        (void)Height;
    }

    inline bool ControlCursorInside(float CursorX, float CursorY, float RectMinX, float RectMinY, float RectMaxX, float RectMaxY)
    {
        return CursorX >= RectMinX && CursorX <= RectMaxX &&
               CursorY >= RectMinY && CursorY <= RectMaxY;
    }
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      PUBLIC FUNCTIONS
//------------------------------------------------------------------------------------------------------------------------

void InitializeControlShelf(ControlShelf& Shelf)
{
    Shelf.SlideOffset   = 0.0f;
    Shelf.SlideVelocity = 0.0f;
    Shelf.OpenEnabled   = false;
    Shelf.ActiveFamily  = ControlFamilyProperties;
}

void EnableControlShelf(ControlShelf& Shelf, float RevealHeight)
{
    Shelf.OpenEnabled = true;
    Shelf.SlideVelocity = 0.0f;
    Shelf.SlideOffset = 0.0f;
    (void)RevealHeight;
}

void DisableControlShelf(ControlShelf& Shelf)
{
    Shelf.OpenEnabled = false;
    Shelf.SlideVelocity = 0.0f;
}

float ResolveControlShelfRevealHeight(float ScreenHeight)
{
    return std::min(ScreenHeight * 0.55f, 460.0f);
}

void ToggleControlShelf(ControlShelf& Shelf, float ScreenHeight)
{
    if (Shelf.OpenEnabled)
        DisableControlShelf(Shelf);
    else
        EnableControlShelf(Shelf, ResolveControlShelfRevealHeight(ScreenHeight));
}

bool ControlShelfCapturingPointer(const ControlShelf& Shelf, float CursorX, float CursorY)
{
    (void)Shelf;
    (void)CursorX;
    (void)CursorY;
    return false;
}

void ResolveControlShelfInteraction(ControlShelf& Shelf, float DeltaX, float DeltaY, float DeltaSeconds, bool PointerHeld)
{
    if (!PointerHeld || Shelf.PivotMode == ControlPivotDormant)
    {
        Shelf.DragActive = false;
        return;
    }

    // 📝 The control shelf grows DOWNWARD, so dragging down (DeltaY > 0) increases the offset.
    float NewOffset = Shelf.SlideOffset + DeltaY;
    if (NewOffset < 0.0f) NewOffset = 0.0f;
    Shelf.SlideOffset = NewOffset;

    if (Shelf.PivotMode == ControlPivotGrip)
        Shelf.Drift += DeltaX;

    float Step    = DeltaSeconds > 1.0e-4f ? DeltaSeconds : 1.0e-4f;
    float Instant = DeltaY / Step;   // down-positive velocity to match the down-positive offset
    Shelf.DragVelocity = Shelf.DragVelocity * 0.65f + Instant * 0.35f;
    Shelf.DragActive = true;
}

void ResolveControlShelfRelease(ControlShelf& Shelf, float RevealHeight)
{
    float Distance   = Shelf.SlideOffset - Shelf.StartOffset;
    bool  ShouldOpen = Shelf.OpenEnabled;
    if      (Shelf.DragVelocity >  ControlFlickVelocity) ShouldOpen = true;
    else if (Shelf.DragVelocity < -ControlFlickVelocity) ShouldOpen = false;
    else if (Distance >  ControlFinalizeOffset)           ShouldOpen = true;
    else if (Distance < -ControlFinalizeOffset)           ShouldOpen = false;

    Shelf.OpenEnabled = ShouldOpen;
    Shelf.PivotMode   = ControlPivotDormant;
    Shelf.DragActive  = false;
    Shelf.SlideOffset = ShouldOpen ? RevealHeight : 0.0f;
    Shelf.DragVelocity = 0.0f;
}

void ConstructControlShelf(ControlShelf&               Shelf,
                           const ThemeSpecification&    Theme,
                           int                          ActiveWorkspace)
{
    (void)Theme;

    ResolveControlTabs(Shelf, ActiveWorkspace);

    // 📝 Placeholder — actual drawing will be implemented when the Vulkan foreground draw list is wired.
    //    The structure mirrors Frontier's control centre: notch → drawer → rail → content area.
}

}   // namespace Slate
