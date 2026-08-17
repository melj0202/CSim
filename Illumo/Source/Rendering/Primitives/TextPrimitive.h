#pragma once

#include "PrimitiveTypes.h"
#include "Rendering/ResourceHandle.h"
#include <string>

// Screen/world text run expanded via stb_easy_font into colored quads on the
// shape mesh (absolute positions). Not a Scene Drawable by itself.
struct TextPrimitive
{
  std::string content;
  float x = 0.0f;
  float y = 0.0f;
  float sizePt = 12.0f; // 12 pt → scale 1.0 (matches historical GLString)
  ColorRgba color;
  RenderStyleHandle styleHandle{};
  int drawOrder = 0;
  bool visible = true;
};
