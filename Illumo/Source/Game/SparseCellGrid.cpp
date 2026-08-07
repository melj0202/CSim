#include "SparseCellGrid.h"
#include "Rulesets/RuleSet.h"
#include <cstring>

SparseCellGrid::SparseCellGrid()
  : chunkPool(kBlocksPerPoolChunk)
{
}

void
SparseCellGrid::chunkCoords(int cellX,
                            int cellY,
                            int* outChunkX,
                            int* outChunkY)
{
  // Floor division that works for negatives.
  int cx = cellX / kChunkDim;
  int cy = cellY / kChunkDim;
  if (cellX < 0 && (cellX % kChunkDim) != 0) {
    cx -= 1;
  }
  if (cellY < 0 && (cellY % kChunkDim) != 0) {
    cy -= 1;
  }
  *outChunkX = cx;
  *outChunkY = cy;
}

int
SparseCellGrid::localIndex(int cellX, int cellY)
{
  int lx = cellX % kChunkDim;
  int ly = cellY % kChunkDim;
  if (lx < 0) {
    lx += kChunkDim;
  }
  if (ly < 0) {
    ly += kChunkDim;
  }
  return ly * kChunkDim + lx;
}

SparseCellGrid::SparseChunk*
SparseCellGrid::findChunk(int chunkX, int chunkY) const
{
  for (std::size_t i = 0; i < activeChunks.size(); ++i) {
    SparseChunk* chunk = activeChunks[i];
    if (chunk != nullptr && chunk->chunkX == chunkX &&
        chunk->chunkY == chunkY) {
      return chunk;
    }
  }
  return nullptr;
}

SparseCellGrid::SparseChunk*
SparseCellGrid::findOrCreateChunk(int chunkX, int chunkY)
{
  SparseChunk* existing = findChunk(chunkX, chunkY);
  if (existing != nullptr) {
    return existing;
  }

  SparseChunk* storage = chunkPool.Allocate();
  if (storage == nullptr) {
    return nullptr;
  }

  SparseChunk* chunk = ::new (storage) SparseChunk();
  chunk->chunkX = chunkX;
  chunk->chunkY = chunkY;
  std::memset(chunk->cells, BackgroundState, sizeof(chunk->cells));
  activeChunks.push_back(chunk);
  return chunk;
}

bool
SparseCellGrid::chunkHasNonBackground(const SparseChunk& chunk) const
{
  for (int i = 0; i < kChunkDim * kChunkDim; ++i) {
    if (chunk.cells[i] != BackgroundState) {
      return true;
    }
  }
  return false;
}

unsigned char
SparseCellGrid::getCell(const CellAddress& address) const
{
  int chunkX = 0;
  int chunkY = 0;
  chunkCoords(address.x, address.y, &chunkX, &chunkY);
  const SparseChunk* chunk = findChunk(chunkX, chunkY);
  if (chunk == nullptr) {
    return BackgroundState;
  }
  return chunk->cells[localIndex(address.x, address.y)];
}

bool
SparseCellGrid::setCell(const CellAddress& address, unsigned char state)
{
  int chunkX = 0;
  int chunkY = 0;
  chunkCoords(address.x, address.y, &chunkX, &chunkY);

  if (state == BackgroundState) {
    SparseChunk* chunk = findChunk(chunkX, chunkY);
    if (chunk == nullptr) {
      return true;
    }
    chunk->cells[localIndex(address.x, address.y)] = BackgroundState;
    if (!chunkHasNonBackground(*chunk)) {
      for (std::size_t i = 0; i < activeChunks.size(); ++i) {
        if (activeChunks[i] == chunk) {
          activeChunks[i] = activeChunks.back();
          activeChunks.pop_back();
          break;
        }
      }
      chunk->~SparseChunk();
      chunkPool.Deallocate(chunk);
    }
    revision += 1;
    return true;
  }

  SparseChunk* chunk = findOrCreateChunk(chunkX, chunkY);
  if (chunk == nullptr) {
    return false;
  }
  chunk->cells[localIndex(address.x, address.y)] = state;
  revision += 1;
  return true;
}

void
SparseCellGrid::clear()
{
  for (std::size_t i = 0; i < activeChunks.size(); ++i) {
    SparseChunk* chunk = activeChunks[i];
    if (chunk != nullptr) {
      chunk->~SparseChunk();
      chunkPool.Deallocate(chunk);
    }
  }
  activeChunks.clear();
  chunkPool.Clear();
  revision += 1;
}

unsigned char
SparseCellGrid::countAliveNeighbors(int x, int y) const
{
  unsigned char count = 0;
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      CellAddress neighbor;
      neighbor.x = x + dx;
      neighbor.y = y + dy;
      if (getCell(neighbor) == CountedNeighborState) {
        count = static_cast<unsigned char>(count + 1);
      }
    }
  }
  return count;
}

bool
SparseCellGrid::advance(const RuleSet& ruleSet)
{
  // Collect candidate cells: all stored cells plus their Moore neighbors.
  struct Candidate
  {
    int x;
    int y;
  };
  std::vector<Candidate> candidates;
  candidates.reserve(activeChunks.size() * kChunkDim * kChunkDim * 2);

  auto pushUnique = [&candidates](int x, int y) {
    for (std::size_t i = 0; i < candidates.size(); ++i) {
      if (candidates[i].x == x && candidates[i].y == y) {
        return;
      }
    }
    Candidate c;
    c.x = x;
    c.y = y;
    candidates.push_back(c);
  };

  for (std::size_t ci = 0; ci < activeChunks.size(); ++ci) {
    SparseChunk* chunk = activeChunks[ci];
    if (chunk == nullptr) {
      continue;
    }
    for (int ly = 0; ly < kChunkDim; ++ly) {
      for (int lx = 0; lx < kChunkDim; ++lx) {
        const int cellX = chunk->chunkX * kChunkDim + lx;
        const int cellY = chunk->chunkY * kChunkDim + ly;
        const unsigned char state = chunk->cells[ly * kChunkDim + lx];
        if (state == BackgroundState) {
          continue;
        }
        for (int dy = -1; dy <= 1; ++dy) {
          for (int dx = -1; dx <= 1; ++dx) {
            pushUnique(cellX + dx, cellY + dy);
          }
        }
      }
    }
  }

  struct Change
  {
    int x;
    int y;
    unsigned char next;
  };
  std::vector<Change> changes;
  changes.reserve(candidates.size());

  for (std::size_t i = 0; i < candidates.size(); ++i) {
    const int x = candidates[i].x;
    const int y = candidates[i].y;
    CellAddress address;
    address.x = x;
    address.y = y;
    const unsigned char current = getCell(address);
    const unsigned char neighbors = countAliveNeighbors(x, y);
    const unsigned char next = ruleSet.nextState(current, neighbors);
    if (next != current) {
      Change change;
      change.x = x;
      change.y = y;
      change.next = next;
      changes.push_back(change);
    }
  }

  bool ok = true;
  for (std::size_t i = 0; i < changes.size(); ++i) {
    CellAddress address;
    address.x = changes[i].x;
    address.y = changes[i].y;
    if (!setCell(address, changes[i].next)) {
      ok = false;
    }
  }
  return ok;
}
