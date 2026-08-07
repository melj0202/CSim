#pragma once
#include "Rendering/Camera.h"
#include "Rendering/Drawable.h"
#include <vector>

class IRenderWindow;

// FrameRenderList (kept as type name Scene for source stability).
//
// Role: ordered, non-owning list of drawables rebuilt every frame by modules
// via IModule::DispatchDrawables. This is NOT a retained scene graph, spatial
// hierarchy, or world container (D-E4). If a retained world model is added
// later, extract into a separate type and keep this as the transient
// submission list consumed by Renderer::RenderScene.
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
