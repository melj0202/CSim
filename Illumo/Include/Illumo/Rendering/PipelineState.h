#pragma once
#include <cstdint>
// 1. Define custom, clean enums instead of raw GLenums
enum class BlendFactor : uint8_t
{
  Zero,
  One,
  SrcAlpha,
  OneMinusSrcAlpha,
  SrcColor,
  OneMinusSrcColor
};

enum class CullMode : uint8_t
{
  Front,
  Back,
  FrontAndBack
};

enum class WindingOrder : uint8_t
{
  Clockwise,
  CounterClockwise
};

enum class Primitives : uint8_t
{
  Points,
  Lines,
  Triangles
};

// 2. The completely pure, cross-platform Pipeline State Struct
struct PipelineState
{
  bool depthTestEnabled = true;

  bool blendEnabled = false;
  BlendFactor blendSrc = BlendFactor::SrcAlpha;
  BlendFactor blendDst = BlendFactor::OneMinusSrcAlpha;

  bool faceCullingEnabled = false;
  CullMode cullFace = CullMode::Back;
  WindingOrder frontFace = WindingOrder::CounterClockwise;

  bool wireframe = false;
  Primitives primitives = Primitives::Triangles;
};
