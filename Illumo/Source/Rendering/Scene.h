#pragma once
#include "Rendering/Camera.h"
#include "Rendering/Drawable.h"
#include <vector>

class IRenderWindow;

// Per-frame contribution list for Renderer::RenderScene.
// Intentionally NOT a scene graph (node hierarchy removed — D-E4).
// Owns no GPU resources; drawables are non-owning pointers for one frame.
class Scene
{
public:
  Scene(IRenderWindow* window, Camera* camera)
    : window(window)
    , activeCamera(camera)
  {
  }

  ~Scene() = default;

  void AddDrawable(DrawableBase* drawable) { drawables.push_back(drawable); }

  void ClearDrawables() { drawables.clear(); }

  IRenderWindow* window;
  Camera* activeCamera;
  std::vector<DrawableBase*> drawables;
};
