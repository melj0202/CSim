#pragma once

#include <Illumo/Services/IAllocator.h>
#include <cstdlib>

// Thin std::malloc / std::free wrapper. Useful as a baseline IAllocator and for
// tests; not intended as a high-performance general-purpose heap.
class MallocAlloc : public IAllocator
{
public:
  MallocAlloc() = default;
  ~MallocAlloc() override = default;

  void* Allocate(size_t size) override
  {
    if (size == 0) {
      return nullptr;
    }
    return std::malloc(size);
  }

  void Free(void* ptr) override { std::free(ptr); }
};
