#pragma once

#include <cstdint>

// Shared value types for render primitives (D-R12).
// Top-left origin for Rect2. Primitives never own GPU objects.

struct ColorRgba
{
  unsigned char r = 255;
  unsigned char g = 255;
  unsigned char b = 255;
  unsigned char a = 255;
};

// Axis-aligned rect; origin is top-left, size is width/height.
struct Rect2
{
  float x = 0.0f;
  float y = 0.0f;
  float w = 0.0f;
  float h = 0.0f;
};

// How GameVisual interprets primitive coordinates when emitting tokens.
enum class PrimitiveSpace : unsigned char
{
  Pixels = 0, // window pixels (y down, top-left origin) — UI/debug default
  World = 1   // world units via camera MVP
};
