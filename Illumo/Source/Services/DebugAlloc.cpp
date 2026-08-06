#include "DebugAlloc.h"
#include <exception>
#include <malloc.h>
#include <tracy/TracyC.h>

void*
operator new(std::size_t size)
{
  void* ptr = std::malloc(size);
  if (!ptr) {
    throw std::bad_alloc();
  }
  TracyCAlloc(ptr, size);
  return ptr;
}

void
operator delete(void* ptr) noexcept
{
  TracyCFree(ptr);
  std::free(ptr);
}

void*
operator new[](std::size_t size)
{
  void* ptr = std::malloc(size);
  if (!ptr) {
    throw std::bad_alloc();
  }
  TracyCAlloc(ptr, size);
  return ptr;
}

void
operator delete[](void* ptr) noexcept
{
  TracyCFree(ptr);
  std::free(ptr);
}
