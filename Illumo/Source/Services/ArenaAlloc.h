#pragma once
#define MAX_STACK_CHUNKS 4
class ArenaAlloc
{

private:
  struct Chunk
  {
    char* data;
    size_t wastedBytes;
    Chunk* next;
  };

  Chunk* chunk;
  Chunk* chunkHead;
  size_t offset;
  size_t chunkSize; // Type is not singular in this context
  size_t numChunks;

public:
  ArenaAlloc(size_t chunkSize)
  {
    this->chunk = new Chunk();
    this->offset = 0;
    this->chunkSize = chunkSize;
    this->numChunks = 0;
    this->chunk->data = new char[chunkSize];
    this->chunk->next = nullptr;
    this->chunkHead = this->chunk;
    this->numChunks++;
  };
  ~ArenaAlloc()
  {
    Chunk* currentChunk = chunkHead;
    while (currentChunk != nullptr) {
      Chunk* next = currentChunk->next;
      delete[] currentChunk->data;
      delete currentChunk;
      currentChunk = next;
    }
  };

  template<typename T, typename... Args>
  T* Allocate(Args&&... args)
  {
    size_t size = sizeof(T);
    if (chunkSize - offset < size) {
      if (numChunks == MAX_STACK_CHUNKS) {
        return nullptr;
      }
      this->chunk->next = new Chunk();
      this->chunk->next->data = new char[chunkSize];
      this->chunk->next->next = nullptr;
      this->chunk->wastedBytes = chunkSize - offset;
      this->chunk = this->chunk->next;
      this->offset = 0;
      this->numChunks++;
    }
    void* address = this->chunk->data + offset;
    offset += size;
    T* instance = ::new (address) T(std::forward<Args>(args)...);
    return instance;
  }

  void Deallocate() { this->Clear(); }

  void Clear()
  {
    offset = 0;
    numChunks = 1;

    // Delete all chunks after the head
    Chunk* currentChunk = chunkHead->next;
    while (currentChunk != nullptr) {
      Chunk* nextChunk = currentChunk->next;
      delete[] currentChunk->data;
      delete currentChunk;
      currentChunk = nextChunk;
    }

    // Reset the head chunk
    chunkHead->next = nullptr;
    chunkHead->wastedBytes = 0;
    chunk = chunkHead;
  }
};