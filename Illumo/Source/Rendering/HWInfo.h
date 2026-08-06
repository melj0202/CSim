#pragma once
#include <string>

struct HWInfo
{
  std::string gpuVendor;
  std::string gpuRenderer; // Name of the actual graphics card
  std::string glVersion;   // Driver OpenGL version
  std::string glslVersion;

  int maxTextureSize = 0; // Max width/height of a 2D texture
  int maxTextureSlots =
    0; // How many textures can be bound at once (useful for batching!)
  int maxUniformBlockSize = 0; // Max bytes for a Uniform Buffer Object (UBO)
  int maxUniformBlocks = 0;
  int maxVertexAttributes = 0; // Max vertex inputs allowed in a shader

  int maxGeometryInputComponents = 0;
  int maxGeometryOutputComponents = 0;
  int maxGeometryTotalOutputComponents = 0;

  int maxTessControlInputComponents = 0;
  int maxTessControlOutputComponents = 0;
  int maxTessEvaluationInputComponents = 0;
  int maxTessEvaluationOutputComponents = 0;

  bool hasGeometryShaderSupport = false;
  bool hasTessellationShaderSupport = false;
  bool hasComputeShaderSupport = false;
  bool hasRayTracingSupport = false;
  bool hasMeshShaderSupport = false;

  int maxComputeWorkGroupInvocations = 0;
  int maxComputeWorkGroupCount[3] = { 0, 0, 0 };
  int maxComputeWorkGroupSize[3] = { 0, 0, 0 };
  int maxImageUnits = 0;

  // VRAM
  int vramSize = 0;
};