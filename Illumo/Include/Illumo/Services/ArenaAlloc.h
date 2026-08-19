#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <string>
#include <utility>

// Bump-pointer arena with chained fixed-size chunks. Individual free is not
// supported; Clear() / Deallocate() bulk-reset the arena (destructors are not
// invoked — use POD or manage lifetimes yourself).
//
// Fit: frame scratch, transient command data, parsers, load buffers.
class ArenaAlloc
{
public:
  static constexpr size_t kMaxChunks = 4;

private:
  struct Chunk
  {
    char* data = nullptr;
    size_t wastedBytes = 0;
    Chunk* next = nullptr;
  };

  Chunk* chunk = nullptr;
  Chunk* chunkHead = nullptr;
  size_t offset = 0;
  size_t chunkSize = 0;
  size_t numChunks = 0;

  static size_t alignUp(size_t value, size_t alignment)
  {
    if (alignment <= 1) {
      return value;
    }
    const size_t mask = alignment - 1;
    return (value + mask) & ~mask;
  }

  bool growChunk()
  {
    if (numChunks >= kMaxChunks) {
      return false;
    }
    Chunk* nextChunk = new Chunk();
    nextChunk->data = new char[chunkSize];
    nextChunk->next = nullptr;
    nextChunk->wastedBytes = 0;
    chunk->wastedBytes = chunkSize - offset;
    chunk->next = nextChunk;
    chunk = nextChunk;
    offset = 0;
    numChunks += 1;
    return true;
  }

  void* allocateRaw(size_t size, size_t alignment)
  {
    if (size == 0 || size > chunkSize) {
      return nullptr;
    }
    if (alignment == 0) {
      alignment = 1;
    }

    size_t alignedOffset = alignUp(offset, alignment);
    if (alignedOffset + size > chunkSize) {
      if (!growChunk()) {
        return nullptr;
      }
      alignedOffset = alignUp(offset, alignment);
      if (alignedOffset + size > chunkSize) {
        return nullptr;
      }
    }

    void* address = chunk->data + alignedOffset;
    offset = alignedOffset + size;
    return address;
  }

public:
  explicit ArenaAlloc(size_t bytesPerChunk)
    : chunkSize(bytesPerChunk)
  {
    if (chunkSize == 0) {
      chunkSize = 1;
    }
    chunk = new Chunk();
    chunk->data = new char[chunkSize];
    chunk->next = nullptr;
    chunk->wastedBytes = 0;
    chunkHead = chunk;
    numChunks = 1;
    offset = 0;
  }

  ~ArenaAlloc()
  {
    Chunk* currentChunk = chunkHead;
    while (currentChunk != nullptr) {
      Chunk* next = currentChunk->next;
      delete[] currentChunk->data;
      delete currentChunk;
      currentChunk = next;
    }
  }

  ArenaAlloc(const ArenaAlloc&) = delete;
  ArenaAlloc& operator=(const ArenaAlloc&) = delete;

  template<typename T, typename... Args>
  T* Allocate(Args&&... args)
  {
    void* address = allocateRaw(sizeof(T), alignof(T));
    if (address == nullptr) {
      return nullptr;
    }
    return ::new (address) T(std::forward<Args>(args)...);
  }

  // Uninitialized aligned bytes (or nullptr on failure / zero size).
  void* AllocateBytes(size_t size, size_t alignment = alignof(std::max_align_t))
  {
    return allocateRaw(size, alignment);
  }

  // Null-terminated copy of src, or nullptr on failure.
  char* AllocateCString(const char* src, size_t length)
  {
    char* dest = static_cast<char*>(allocateRaw(length + 1, alignof(char)));
    if (dest == nullptr) {
      return nullptr;
    }
    if (length > 0 && src != nullptr) {
      std::memcpy(dest, src, length);
    }
    dest[length] = '\0';
    return dest;
  }

  char* AllocateCString(const std::string& text)
  {
    return AllocateCString(text.data(), text.size());
  }

  // Bulk free; synonym for Clear(). Does not run destructors.
  void Deallocate() { Clear(); }

  void Clear()
  {
    offset = 0;
    numChunks = 1;

    Chunk* currentChunk = chunkHead->next;
    while (currentChunk != nullptr) {
      Chunk* nextChunk = currentChunk->next;
      delete[] currentChunk->data;
      delete currentChunk;
      currentChunk = nextChunk;
    }

    chunkHead->next = nullptr;
    chunkHead->wastedBytes = 0;
    chunk = chunkHead;
  }

  size_t getChunkSize() const { return chunkSize; }
  size_t getNumChunks() const { return numChunks; }
  size_t getOffset() const { return offset; }
};
