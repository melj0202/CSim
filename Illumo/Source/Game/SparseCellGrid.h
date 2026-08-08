#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class RuleSet;

// Signed world-space cell coordinate.
struct CellAddress
{
  std::int64_t x = 0;
  std::int64_t y = 0;

  bool operator==(const CellAddress& other) const
  {
    return x == other.x && y == other.y;
  }
};

struct ChunkAddress
{
  std::int64_t x = 0;
  std::int64_t y = 0;

  bool operator==(const ChunkAddress& other) const
  {
    return x == other.x && y == other.y;
  }
};

struct SparseChunkRecord
{
  std::int64_t chunkX = 0;
  std::int64_t chunkY = 0;
  std::array<unsigned char, 16 * 16> cells{};
};

struct CellAddressHash
{
  std::size_t operator()(const CellAddress& address) const noexcept;
};

struct ChunkAddressHash
{
  std::size_t operator()(const ChunkAddress& address) const noexcept;
};

// Sparse cellular-automata domain. Only chunks containing a non-background
// state are stored. Each generation reads authoritative chunks through an
// 18x18 temporary halo and writes a new set of 16x16 authoritative chunks.
class SparseCellGrid
{
public:
  static constexpr unsigned char BackgroundState = 1;
  static constexpr unsigned char CountedNeighborState = 0;
  static constexpr int kChunkDim = 16;
  static constexpr int kHaloDim = 18;
  static constexpr std::size_t kChunkCellCount = 16u * 16u;

  SparseCellGrid();
  ~SparseCellGrid() = default;

  SparseCellGrid(const SparseCellGrid&) = delete;
  SparseCellGrid& operator=(const SparseCellGrid&) = delete;

  unsigned char getCell(const CellAddress& address) const;
  bool setCell(const CellAddress& address, unsigned char state);
  void clear();
  bool advance(const RuleSet& ruleSet);

  void swap(SparseCellGrid& other) noexcept;
  bool assignChunk(const SparseChunkRecord& record);
  std::vector<SparseChunkRecord> collectChunkRecords() const;
  void collectChunkRecords(std::vector<SparseChunkRecord>* records) const;

  static std::int64_t floorDivide(std::int64_t value, std::int64_t divisor);
  static std::int64_t floorModulo(std::int64_t value, std::int64_t divisor);
  static ChunkAddress chunkAddressForCell(const CellAddress& address);
  static int localIndexForCell(const CellAddress& address);

  std::uint64_t getRevision() const { return revision; }
  std::size_t getAllocatedChunkCount() const { return chunks.size(); }
  // Compatibility name for old diagnostics; this is the live map size, not
  // a fixed allocator pool or capacity limit.
  std::size_t getPoolChunks() const { return chunks.size(); }

private:
  using CellArray = std::array<unsigned char, kChunkCellCount>;
  using ChunkMap =
    std::unordered_map<ChunkAddress, CellArray, ChunkAddressHash>;

  ChunkMap chunks;
  std::uint64_t revision = 0;

  CellArray* findChunk(const ChunkAddress& address);
  const CellArray* findChunk(const ChunkAddress& address) const;
  CellArray* findOrCreateChunk(const ChunkAddress& address);
  static bool hasNonBackgroundState(const CellArray& cells);
  static std::int64_t cellCoordinate(std::int64_t chunk, int localCoordinate);
};
