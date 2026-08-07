#pragma once

#include "PipelineState.h"

// Built-in draw styles owned by Renderer (Phase A). A style pairs a shader
// table handle with pipeline defaults. Drawables keep content handles (mesh /
// texture / domain buffers) and call Renderer::bindStyle before content tokens.
enum class RenderStyleId : unsigned char
{
  Canvas = 0,
  UiText = 1,
  Console = 2,
  Shape = 3,  // solid/outline shapes (GameVisual primitives)
  Sprite = 4, // textured quads (GameVisual primitives)
  Count
};

struct RenderStyle
{
  unsigned long shaderHandle = 0;
  PipelineState pipeline;
  bool ready = false;
};

inline unsigned
renderStyleIndex(RenderStyleId id)
{
  return static_cast<unsigned>(id);
}

inline unsigned
renderStyleCount()
{
  return static_cast<unsigned>(RenderStyleId::Count);
}
