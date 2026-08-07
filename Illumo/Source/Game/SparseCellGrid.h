#pragma once

#include "Services/PoolAlloc.h"
#include <cstddef>
#include <cstdint>
#include <vector>

class RuleSet;

// Integer cell coordinate in infinite (sparse) space.
struct CellAddress
{
  int x = 0;
  int y = 0;

  bool operator==(const CellAddress& other) const
  {
    return x == other.x && y == other.y;
  }

  bool operator!=(const CellAddress& other) const { return !(*this == other); }
};

// Sparse CA domain backed by fixed-size chunks from ChainedPoolAlloc.
// Background cells are not stored; non-background cells live in 8x8 chunks.
// Does not replace dense Canvas — optional path for large / sparse worlds.
class SparseCellGrid
{
public:
  static constexpr unsigned char BackgroundState = 1;
  static constexpr unsigned char CountedNeighborState = 0;
  static constexpr int kChunkDim = 8;
  static constexpr size_t kBlocksPerPoolChunk = 16;

  SparseCellGrid();
  ~SparseCellGrid() = default;

  SparseCellGrid(const SparseCellGrid&) = delete;
  SparseCellGrid& operator=(const SparseCellGrid&) = delete;

  unsigned char getCell(const CellAddress& address) const;
  // Returns false if a new chunk was required and the pool was exhausted.
  bool setCell(const CellAddress& address, unsigned char state);
  void clear();

  // One generation using ruleSet.nextState. Returns false if a next-state
  // write needed a chunk and the pool was exhausted (grid may be partial).
  bool advance(const RuleSet& ruleSet);

  std::uint64_t getRevision() const { return revision; }
  std::size_t getAllocatedChunkCount() const { return activeChunks.size(); }
  std::size_t getPoolChunks() const { return chunkPool.getNumChunks(); }
  std::size_t getBlocksFree() const { return chunkPool.getBlocksFree(); }

private:
  struct SparseChunk
  {
    int chunkX = 0;
    int chunkY = 0;
    // Dense 8x8 storage; BackgroundState means empty for this implementation.
    unsigned char cells[kChunkDim * kChunkDim];
  };

  ChainedPoolAlloc<SparseChunk> chunkPool;
  std::vector<SparseChunk*> activeChunks;
  std::uint64_t revision = 0;

  static void chunkCoords(int cellX, int cellY, int* outChunkX, int* outChunkY);
  static int localIndex(int cellX, int cellY);
  SparseChunk* findChunk(int chunkX, int chunkY) const;
  SparseChunk* findOrCreateChunk(int chunkX, int chunkY);
  bool chunkHasNonBackground(const SparseChunk& chunk) const;
  unsigned char countAliveNeighbors(int x, int y) const;
};
