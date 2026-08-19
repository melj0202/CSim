#pragma once

#include <Illumo/Rendering/Primitives/PrimitiveTypes.h>
#include <Illumo/Rendering/ResourceHandle.h>

// Value-type textured quad. The primitive borrows typed resource/style handles
// enrolled by its owner or acquired through AssetManager.
struct SpritePrimitive
{
  Rect2 rect;
  TextureHandle textureHandle{};
  TextureRegion region;
  Transform2D transform;
  RenderStyleHandle styleHandle{};
  ColorRgba tint;
  int drawOrder = 0;
  bool flipX = false;
  bool flipY = false;
  bool visible = true;
};
