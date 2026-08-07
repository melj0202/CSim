#pragma once

#include "PrimitiveTypes.h"

// Value-type textured quad. textureHandle is a backend table ID enrolled by
// the owner (game object / asset path); the primitive only borrows it.
struct SpritePrimitive
{
  Rect2 rect;
  unsigned long textureHandle = 0;
  float u0 = 0.0f;
  float v0 = 0.0f;
  float u1 = 1.0f;
  float v1 = 1.0f;
  ColorRgba tint;
  bool visible = true;
};
