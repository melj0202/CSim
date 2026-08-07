#pragma once

#include <cstddef>

// Minimal allocator interface for interchangeable backends (frame scratch,
// pools, or thin OS malloc wrappers). Not a general mimalloc replacement.
class IAllocator
{
public:
  virtual ~IAllocator() = default;

  // Returns nullptr on failure. size == 0 is implementation-defined; callers
  // should treat a null return as failure either way.
  virtual void* Allocate(size_t size) = 0;
  virtual void Free(void* ptr) = 0;
};
