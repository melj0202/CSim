#pragma once

#include <cstdint>

// Shared value types for render primitives (D-R15).
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

struct Transform2D
{
  float x = 0.0f;
  float y = 0.0f;
  float scaleX = 1.0f;
  float scaleY = 1.0f;
  float rotationRadians = 0.0f;
  // Normalized within the transformed primitive or GameVisual content bounds.
  float pivotX = 0.0f;
  float pivotY = 0.0f;
};

struct TextureRegion
{
  float u0 = 0.0f;
  float v0 = 0.0f;
  float u1 = 1.0f;
  float v1 = 1.0f;

  static TextureRegion gridCell(unsigned int columns,
                                unsigned int rows,
                                unsigned int column,
                                unsigned int row)
  {
    TextureRegion region;
    if (columns == 0 || rows == 0 || column >= columns || row >= rows) {
      return region;
    }
    const float cellWidth = 1.0f / static_cast<float>(columns);
    const float cellHeight = 1.0f / static_cast<float>(rows);
    region.u0 = static_cast<float>(column) * cellWidth;
    region.v0 = static_cast<float>(row) * cellHeight;
    region.u1 = region.u0 + cellWidth;
    region.v1 = region.v0 + cellHeight;
    return region;
  }
};

// How GameVisual interprets primitive coordinates when emitting tokens.
enum class PrimitiveSpace : unsigned char
{
  Pixels = 0, // window pixels (y down, top-left origin) — UI/debug default
  World = 1   // world units via camera MVP
};
