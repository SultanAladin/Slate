//============================================================================================================================================
//                                                     EDITORCAMERACOMPONENT.H
//============================================================================================================================================
// 🧩 Editor-specialised camera identity. It uses CameraComponent's common pose, movement and positional
//    lag today; editor-only selection, bookmarks and UI remain in EditorHost / SceneDirectoryPanel.
//    PlayerCameraComponent and SpectatorCameraComponent can later provide different controllers without
//    inheriting editor behavior or duplicating the base camera law.

#pragma once

#include "Application/CameraComponent/Api/CameraComponent.h"

namespace Slate
{

class EditorCameraComponent final : public CameraComponent
{
public:
    EditorCameraComponent() = default;
    ~EditorCameraComponent() = default;
};

} // namespace Slate
