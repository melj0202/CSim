#pragma once

#include "PipelineState.h"
#include "ResourceHandle.h"

// Registered draw styles owned by Renderer. A style pairs a shader handle with
// pipeline defaults. GameVisual custom shaders may consume uMVP, uUsePixels,
// u_resolution and, for sprites, uTexture on texture unit zero.
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
  ShaderHandle shaderHandle{};
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
