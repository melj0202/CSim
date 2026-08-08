#include "SparseCellGrid.h"
#include "Rulesets/RuleSet.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>

static std::size_t
mixAddress(std::uint64_t value)
{
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
  return static_cast<std::size_t>(value ^ (value >> 31));
}

SparseCellGrid::SparseCellGrid() = default;

std::size_t
CellAddressHash::operator()(const CellAddress& address) const noexcept
{
  const std::size_t hx = mixAddress(static_cast<std::uint64_t>(address.x));
  const std::size_t hy = mixAddress(static_cast<std::uint64_t>(address.y));
  return hx ^
         (hy + static_cast<std::size_t>(0x9e3779b9) + (hx << 6) + (hx >> 2));
}

std::size_t
ChunkAddressHash::operator()(const ChunkAddress& address) const noexcept
{
  const std::size_t hx = mixAddress(static_cast<std::uint64_t>(address.x));
  const std::size_t hy = mixAddress(static_cast<std::uint64_t>(address.y));
  return hx ^
         (hy + static_cast<std::size_t>(0x9e3779b9) + (hx << 6) + (hx >> 2));
}

std::int64_t
SparseCellGrid::floorDivide(std::int64_t value, std::int64_t divisor)
{
  if (divisor <= 0) {
    return 0;
  }
  const std::int64_t quotient = value / divisor;
  const std::int64_t remainder = value % divisor;
  if (remainder < 0) {
    return quotient - 1;
  }
  return quotient;
}

std::int64_t
SparseCellGrid::floorModulo(std::int64_t value, std::int64_t divisor)
{
  if (divisor <= 0) {
    return 0;
  }
  const std::int64_t remainder = value % divisor;
  return remainder < 0 ? remainder + divisor : remainder;
}

ChunkAddress
SparseCellGrid::chunkAddressForCell(const CellAddress& address)
{
  ChunkAddress result;
  result.x = floorDivide(address.x, kChunkDim);
  result.y = floorDivide(address.y, kChunkDim);
  return result;
}

int
SparseCellGrid::localIndexForCell(const CellAddress& address)
{
  const int localX = static_cast<int>(floorModulo(address.x, kChunkDim));
  const int localY = static_cast<int>(floorModulo(address.y, kChunkDim));
  return localY * kChunkDim + localX;
}

std::int64_t
SparseCellGrid::cellCoordinate(std::int64_t chunk, int localCoordinate)
{
  return chunk * static_cast<std::int64_t>(kChunkDim) + localCoordinate;
}

SparseCellGrid::CellArray*
SparseCellGrid::findChunk(const ChunkAddress& address)
{
  ChunkMap::iterator found = chunks.find(address);
  return found == chunks.end() ? nullptr : &found->second;
}

const SparseCellGrid::CellArray*
SparseCellGrid::findChunk(const ChunkAddress& address) const
{
  ChunkMap::const_iterator found = chunks.find(address);
  return found == chunks.end() ? nullptr : &found->second;
}

SparseCellGrid::CellArray*
SparseCellGrid::findOrCreateChunk(const ChunkAddress& address)
{
  ChunkMap::iterator found = chunks.find(address);
  if (found != chunks.end()) {
    return &found->second;
  }

  CellArray blank;
  blank.fill(BackgroundState);
  std::pair<ChunkMap::iterator, bool> inserted = chunks.emplace(address, blank);
  return &inserted.first->second;
}

bool
SparseCellGrid::hasNonBackgroundState(const CellArray& cells)
{
  for (unsigned char state : cells) {
    if (state != BackgroundState) {
      return true;
    }
  }
  return false;
}

unsigned char
SparseCellGrid::getCell(const CellAddress& address) const
{
  const ChunkAddress chunkAddress = chunkAddressForCell(address);
  const CellArray* chunk = findChunk(chunkAddress);
  if (chunk == nullptr) {
    return BackgroundState;
  }
  return (*chunk)[static_cast<std::size_t>(localIndexForCell(address))];
}

bool
SparseCellGrid::setCell(const CellAddress& address, unsigned char state)
{
  const ChunkAddress chunkAddress = chunkAddressForCell(address);
  const int index = localIndexForCell(address);
  CellArray* chunk = findChunk(chunkAddress);

  if (state == BackgroundState) {
    if (chunk == nullptr) {
      return true;
    }
    if ((*chunk)[static_cast<std::size_t>(index)] == BackgroundState) {
      return true;
    }
    (*chunk)[static_cast<std::size_t>(index)] = BackgroundState;
    if (!hasNonBackgroundState(*chunk)) {
      chunks.erase(chunkAddress);
    }
    revision += 1;
    return true;
  }

  try {
    chunk = findOrCreateChunk(chunkAddress);
  } catch (const std::bad_alloc&) {
    return false;
  }
  if (chunk == nullptr) {
    return false;
  }
  if ((*chunk)[static_cast<std::size_t>(index)] != state) {
    (*chunk)[static_cast<std::size_t>(index)] = state;
    revision += 1;
  }
  return true;
}

void
SparseCellGrid::clear()
{
  if (!chunks.empty()) {
    chunks.clear();
    revision += 1;
  }
}

bool
SparseCellGrid::assignChunk(const SparseChunkRecord& record)
{
  if (!hasNonBackgroundState(record.cells)) {
    return true;
  }
  try {
    chunks[ChunkAddress{ record.chunkX, record.chunkY }] = record.cells;
  } catch (const std::bad_alloc&) {
    return false;
  }
  revision += 1;
  return true;
}

std::vector<SparseChunkRecord>
SparseCellGrid::collectChunkRecords() const
{
  std::vector<SparseChunkRecord> records;
  records.reserve(chunks.size());
  for (ChunkMap::const_reference entry : chunks) {
    SparseChunkRecord record;
    record.chunkX = entry.first.x;
    record.chunkY = entry.first.y;
    record.cells = entry.second;
    records.push_back(record);
  }
  std::sort(records.begin(),
            records.end(),
            [](const SparseChunkRecord& left, const SparseChunkRecord& right) {
              if (left.chunkY != right.chunkY) {
                return left.chunkY < right.chunkY;
              }
              return left.chunkX < right.chunkX;
            });
  return records;
}

void
SparseCellGrid::collectChunkRecords(
  std::vector<SparseChunkRecord>* records) const
{
  if (records == nullptr) {
    return;
  }
  *records = collectChunkRecords();
}

void
SparseCellGrid::swap(SparseCellGrid& other) noexcept
{
  chunks.swap(other.chunks);
  const std::uint64_t oldRevision = revision;
  revision = other.revision;
  other.revision = oldRevision;
}

bool
SparseCellGrid::advance(const RuleSet& ruleSet)
{
  std::unordered_set<ChunkAddress, ChunkAddressHash> targets;
  std::vector<SparseChunkRecord> nextRecords;

  try {
    targets.reserve(chunks.size() * 9u + 1u);
    for (ChunkMap::const_reference entry : chunks) {
      for (int offsetY = -1; offsetY <= 1; ++offsetY) {
        for (int offsetX = -1; offsetX <= 1; ++offsetX) {
          targets.insert(
            ChunkAddress{ entry.first.x + offsetX, entry.first.y + offsetY });
        }
      }
    }
    nextRecords.reserve(targets.size());

    for (const ChunkAddress& target : targets) {
      std::array<unsigned char, kHaloDim * kHaloDim> halo;
      for (int haloY = 0; haloY < kHaloDim; ++haloY) {
        for (int haloX = 0; haloX < kHaloDim; ++haloX) {
          const CellAddress address{ cellCoordinate(target.x, haloX - 1),
                                     cellCoordinate(target.y, haloY - 1) };
          halo[static_cast<std::size_t>(haloY * kHaloDim + haloX)] =
            getCell(address);
        }
      }

      SparseChunkRecord next;
      next.chunkX = target.x;
      next.chunkY = target.y;
      next.cells.fill(BackgroundState);
      for (int localY = 0; localY < kChunkDim; ++localY) {
        for (int localX = 0; localX < kChunkDim; ++localX) {
          const int haloIndex = (localY + 1) * kHaloDim + localX + 1;
          unsigned char aliveNeighbors = 0;
          for (int neighborY = -1; neighborY <= 1; ++neighborY) {
            for (int neighborX = -1; neighborX <= 1; ++neighborX) {
              if (neighborX == 0 && neighborY == 0) {
                continue;
              }
              const unsigned char neighbor = halo[static_cast<std::size_t>(
                haloIndex + neighborY * kHaloDim + neighborX)];
              if (neighbor == CountedNeighborState) {
                aliveNeighbors = static_cast<unsigned char>(aliveNeighbors + 1);
              }
            }
          }
          const unsigned char current =
            halo[static_cast<std::size_t>(haloIndex)];
          next.cells[static_cast<std::size_t>(localY * kChunkDim + localX)] =
            ruleSet.nextState(current, aliveNeighbors);
        }
      }
      if (hasNonBackgroundState(next.cells)) {
        nextRecords.push_back(next);
      }
    }
  } catch (const std::bad_alloc&) {
    return false;
  }

  ChunkMap nextChunks;
  try {
    nextChunks.reserve(nextRecords.size());
    for (const SparseChunkRecord& record : nextRecords) {
      nextChunks.emplace(ChunkAddress{ record.chunkX, record.chunkY },
                         record.cells);
    }
  } catch (const std::bad_alloc&) {
    return false;
  }
  chunks.swap(nextChunks);
  revision += 1;
  return true;
}
