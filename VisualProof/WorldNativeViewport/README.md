# World-native viewport projection proof

These images are deterministic mathematical projections of the same world rectangle and axes used by the C++ viewport path. They are not screenshots from a running Vulkan/Windows build. They document the expected result that still requires live Windows/Vulkan validation.

- `viewport-three-orthographic-views.svg`: Top, Front, and Right views, each with its own flat view plane and a closed rectangle.
- `viewport-perspective-floor.svg`: perspective camera looking at the world floor `Y = 0`; the floor grid remains the only authoring surface.
