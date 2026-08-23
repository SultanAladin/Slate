#pragma once
#include "SlateScene/Scene/CameraComponent/Api/CameraComponent.h"
namespace Slate
{
class SpectatorCameraComponent : public CameraComponent
{
public:
    SpectatorCameraComponent() = default;
    ~SpectatorCameraComponent() = default;
};
} // namespace Slate
