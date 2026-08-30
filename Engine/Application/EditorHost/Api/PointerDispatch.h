//============================================================================================================================================
//                                                        POINTERDISPATCH.H
//============================================================================================================================================
// One explicit ownership decision for a frame's pointer. UI and viewport code may still expose their
// established bool reference while they are migrated, but the owner is recorded at the boundary instead
// of being inferred repeatedly from unrelated `PointerTaken` checks.

#pragma once

#include <cstdint>

namespace Slate
{

enum class PointerOwner : std::uint32_t
{
    None = 0u,
    Drawer,
    Popup,
    PanelControl,
    OrientationWidget,
    Gizmo,
    DrawingTool,
    Selection,
    Scene,
    EmptyViewport
};

struct PointerDispatchResult
{
    PointerOwner Owner = PointerOwner::None;
    bool Consumed = false;

    void Reset()
    {
        Owner = PointerOwner::None;
        Consumed = false;
    }

    bool Claim(PointerOwner Candidate)
    {
        if (Consumed)
            return false;
        Owner = Candidate;
        Consumed = true;
        return true;
    }
};

} // namespace Slate
