#pragma once

// Ordered composition buckets for a single main pass (default framebuffer).
// World draws first, then UI, then optional debug overlays. Not GPU render
// passes (those own targets/load-store; deferred until targets diverge).
enum class RenderLayerId : unsigned char
{
  World = 0,
  UI = 1,
  Debug = 2,
  Count
};

inline unsigned
renderLayerIndex(RenderLayerId layer)
{
  return static_cast<unsigned>(layer);
}

inline unsigned
renderLayerCount()
{
  return static_cast<unsigned>(RenderLayerId::Count);
}
