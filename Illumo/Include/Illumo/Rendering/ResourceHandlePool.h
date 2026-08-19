#pragma once

#include <cstdint>
#include <vector>

// Slot/generation allocator shared by concrete and mock backends. The pool
// owns handle validity; the backend owns the resource stored at each slot.
template<typename Handle>
class ResourceHandlePool
{
public:
  ResourceHandlePool() { generations.push_back(0); }

  Handle allocate()
  {
    uint32_t slot = 0;
    if (!freeSlots.empty()) {
      slot = freeSlots.back();
      freeSlots.pop_back();
    } else {
      slot = static_cast<uint32_t>(generations.size());
      generations.push_back(1);
    }
    Handle handle;
    handle.slot = slot;
    handle.generation = generations[slot];
    return handle;
  }

  bool isCurrent(Handle handle) const
  {
    return handle.slot != 0 && handle.slot < generations.size() &&
           handle.generation == generations[handle.slot];
  }

  bool release(Handle handle)
  {
    if (!isCurrent(handle)) {
      return false;
    }
    uint32_t next = generations[handle.slot] + 1;
    if (next == 0) {
      next = 1;
    }
    generations[handle.slot] = next;
    freeSlots.push_back(handle.slot);
    return true;
  }

  void clear()
  {
    generations.clear();
    generations.push_back(0);
    freeSlots.clear();
  }

private:
  std::vector<uint32_t> generations;
  std::vector<uint32_t> freeSlots;
};
