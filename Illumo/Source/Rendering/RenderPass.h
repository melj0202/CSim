#pragma once
#include <array>
#include <cstdint>

// 1-byte enums are perfect for GPU state packing
enum class LoadAction : uint8_t
{
  Load,
  Clear,
  DontCare
};

enum class StoreAction : uint8_t
{
  Store,
  DontCare
};

// Tight alignment: 24 bytes total per color attachment
struct ColorAttachmentDescription
{
  unsigned int binding = 0; // Hardware slot target (e.g., layout(location = X))
  LoadAction loadAction = LoadAction::Clear;
  StoreAction storeAction = StoreAction::Store;
  uint8_t padding[2] = {
    0,
    0
  }; // Explicitly pad to maintain clean 4-byte boundaries
  std::array<float, 4> clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
};

// Depth and stencil usually share the same physical texture memory slice
struct DepthStencilDescription
{
  LoadAction depthLoadAction = LoadAction::Clear;
  StoreAction depthStoreAction = StoreAction::Store;
  LoadAction stencilLoadAction = LoadAction::Clear;
  StoreAction stencilStoreAction = StoreAction::Store;

  float clearDepth = 1.0f;
  int clearStencil = 0;
};

class RenderPass
{
public:
  // Fixed arrays prevent heap allocations during your frame loop
  // 4 to 8 color attachments covers 99% of modern AAA Deferred Renderers
  static constexpr size_t MAX_COLOR_ATTACHMENTS = 4;

  std::array<ColorAttachmentDescription, MAX_COLOR_ATTACHMENTS>
    colorAttachments;
  uint32_t activeColorAttachmentCount = 0;

  // Optional structure: use a boolean flag or a pointer to track if it's active
  bool hasDepthStencil = false;
  DepthStencilDescription depthStencilAttachment;

  uint32_t getFrameBufferID() const { return _framebufferID; }

private:
  uint32_t _framebufferID = 0;
};