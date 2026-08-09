#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <typeinfo>
#include <unordered_map>
#include <vector>

class RuleSet;
class SparseWorkerPool;

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

struct SparseAdvanceStats
{
  std::size_t activeChunkCount = 0;
  std::size_t activeCellCount = 0;
  std::size_t countedCellCount = 0;
  std::size_t candidatePreferredChunkCount = 0;
  std::size_t targetChunkCount = 0;
  std::size_t candidateCellCount = 0;
  std::size_t candidateTargetCount = 0;
  std::size_t haloTargetCount = 0;
  std::size_t allocatedChunkNodeCount = 0;
  std::size_t reusedChunkNodeCount = 0;
  std::size_t retainedChunkNodeCount = 0;
  std::size_t candidateWorkRangeCount = 0;
  std::size_t changedChunkCount = 0;
  std::size_t frontierTargetCount = 0;
  std::size_t frontierSourceChunkCount = 0;
  std::size_t frontierEstimatedWork = 0;
  std::size_t completeEstimatedWork = 0;
  unsigned int candidatePreparationWorkerCount = 1;
  unsigned int workerCount = 1;
  bool usedCellCandidates = false;
  bool usedMixedTargets = false;
  bool usedChangedFrontier = false;
};

struct ChunkAddressHash
{
  std::size_t operator()(const ChunkAddress& address) const noexcept;
};

// Sparse cellular-automata domain. Only chunks containing a non-background
// state are stored. Each generation reads authoritative chunk masks and cells
// and writes a new set of 16x16 authoritative chunks.
class SparseCellGrid
{
public:
  static constexpr unsigned char BackgroundState = 1;
  static constexpr unsigned char CountedNeighborState = 0;
  static constexpr int kChunkDim = 16;
  static constexpr int kHaloDim = 18;
  static constexpr std::size_t kChunkCellCount = 16u * 16u;
  using ChunkCells = std::array<unsigned char, kChunkCellCount>;
  using ChunkVisitor =
    std::function<void(const ChunkAddress&, const ChunkCells&)>;
  using ChangedChunkVisitor =
    std::function<void(const ChunkAddress&, const ChunkCells*)>;

  SparseCellGrid();
  ~SparseCellGrid();

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
  void visitChunksInBounds(const ChunkAddress& minimum,
                           const ChunkAddress& maximum,
                           const ChunkVisitor& visitor) const;
  bool visitChangedChunksSince(std::uint64_t previousRevision,
                               const ChangedChunkVisitor& visitor) const;

  static std::int64_t floorDivide(std::int64_t value, std::int64_t divisor);
  static std::int64_t floorModulo(std::int64_t value, std::int64_t divisor);
  static ChunkAddress chunkAddressForCell(const CellAddress& address);
  static int localIndexForCell(const CellAddress& address);

  // Test-only override: 0 selects the adaptive production worker count.
  static void setWorkerOverrideForTesting(int workers);
  static int getWorkerOverrideForTesting();
  // Test-only override: -1 forces full chunks, 0 selects adaptively, and 1
  // forces cell candidates.
  static void setCellCandidateOverrideForTesting(int mode);
  static int getCellCandidateOverrideForTesting();
  static void setChunkNodeReuseOverrideForTesting(bool enabled);
  static bool getChunkNodeReuseOverrideForTesting();
  std::size_t getCandidateIndexCapacityForTesting() const
  {
    return m_candidateIndex.size();
  }
  std::size_t getCandidateScratchCapacityForTesting() const
  {
    return m_candidateScratch.capacity();
  }
  std::size_t getCompleteTargetCapacityForTesting() const;
  std::size_t getCompleteTargetIndexCapacityForTesting() const;
  std::size_t getCompleteResultCapacityForTesting() const;

  std::uint64_t getRevision() const { return revision; }
  std::size_t getAllocatedChunkCount() const { return chunks.size(); }
  const SparseAdvanceStats& getLastAdvanceStats() const
  {
    return lastAdvanceStats;
  }
  // Compatibility name for old diagnostics; this is the live map size, not
  // a fixed allocator pool or capacity limit.
  std::size_t getPoolChunks() const { return chunks.size(); }

private:
  using CellArray = ChunkCells;
  using OccupancyMask = std::array<std::uint64_t, kChunkCellCount / 64u>;
  struct ChunkData
  {
    CellArray cells{};
    OccupancyMask occupied{};
    OccupancyMask counted{};
    std::uint16_t occupiedCellCount = 0u;
    std::uint16_t countedCellCount = 0u;
  };
  struct ChunkStatistics
  {
    std::size_t activeCellCount = 0u;
    std::size_t countedCellCount = 0u;
    std::size_t candidatePreferredChunkCount = 0u;
  };
  struct CandidateScratchChunk
  {
    CandidateScratchChunk() noexcept
      : address{}
      , candidates{}
      , candidateCellCount(0u)
      , neighborContributionCount(0u)
      , useCellCandidates(true)
    {
    }
    CandidateScratchChunk(const CandidateScratchChunk&) = delete;
    CandidateScratchChunk& operator=(const CandidateScratchChunk&) = delete;
    CandidateScratchChunk(CandidateScratchChunk&& other) noexcept
      : address(other.address)
      , candidates(other.candidates)
      , candidateCellCount(other.candidateCellCount)
      , neighborContributionCount(other.neighborContributionCount)
      , useCellCandidates(other.useCellCandidates)
    {
      if (candidateCellCount == 0u) {
        return;
      }
      for (std::size_t index = 0u; index < kChunkCellCount; ++index) {
        const std::size_t wordIndex = index / 64u;
        const std::uint64_t bit = static_cast<std::uint64_t>(1u)
                                  << static_cast<unsigned int>(index % 64u);
        if ((candidates[wordIndex] & bit) != 0u) {
          neighborCounts[index] = other.neighborCounts[index];
        }
      }
    }

    ChunkAddress address;
    OccupancyMask candidates{};
    // Counts are initialized by markCandidate before any candidate is read.
    std::array<unsigned char, kChunkCellCount> neighborCounts;
    std::size_t candidateCellCount = 0u;
    std::size_t neighborContributionCount = 0u;
    bool useCellCandidates = true;
  };
  struct CandidateIndexSlot
  {
    ChunkAddress address;
    std::size_t scratchIndex = 0u;
    std::uint64_t generation = 0u;
  };
  struct CandidateWorkRange
  {
    std::size_t begin = 0u;
    std::size_t end = 0u;
  };
  struct AddressIndexSlot
  {
    ChunkAddress address;
    std::uint64_t generation = 0u;
  };
  using ChunkMap =
    std::unordered_map<ChunkAddress, ChunkData, ChunkAddressHash>;
  using ChunkNode = ChunkMap::node_type;
  struct TargetResult;

  static constexpr std::size_t kParallelTargetThreshold = 32u;
  static constexpr unsigned int kMaxParallelWorkers = 8u;
  static constexpr unsigned int kMaxCandidateWorkers = 4u;
  static constexpr std::size_t kCandidateCellsPerChunkThreshold = 48u;
  static constexpr std::size_t kCandidateNeighborContributionThreshold =
    kCandidateCellsPerChunkThreshold * 8u;
  static constexpr std::size_t kParallelCandidateCellThreshold = 16384u;
  static constexpr std::size_t kCandidateCellsPerWorkRange = 2048u;
  static constexpr std::size_t kFrontierTrackingLimit = 4096u;

  ChunkMap chunks;
  ChunkMap m_nextChunks;
  ChunkStatistics m_chunkStatistics;
  ChunkStatistics m_nextChunkStatistics;
  std::vector<ChunkNode> m_recycledChunkNodes;
  std::vector<CandidateScratchChunk> m_candidateScratch;
  std::vector<CandidateIndexSlot> m_candidateIndex;
  std::vector<TargetResult> m_candidateResults;
  std::vector<CandidateWorkRange> m_candidateWorkRanges;
  std::vector<ChunkAddress> m_changedChunks;
  std::vector<AddressIndexSlot> m_changedChunkIndex;
  std::uint64_t m_changedChunkGeneration = 0u;
  std::vector<ChunkAddress> m_nextChangedChunks;
  std::vector<AddressIndexSlot> m_nextChangedChunkIndex;
  std::uint64_t m_nextChangedChunkGeneration = 0u;
  std::vector<ChunkAddress> m_presentationChangedChunks;
  std::vector<AddressIndexSlot> m_presentationChangedChunkIndex;
  std::uint64_t m_presentationChangedChunkGeneration = 0u;
  std::vector<ChunkAddress> m_frontierTargets;
  std::vector<AddressIndexSlot> m_frontierTargetIndex;
  std::uint64_t m_frontierTargetGeneration = 0u;
  std::vector<ChunkAddress> m_frontierSourceChunks;
  std::vector<AddressIndexSlot> m_frontierSourceChunkIndex;
  std::uint64_t m_frontierSourceChunkGeneration = 0u;
  std::vector<TargetResult> m_frontierResults;
  std::vector<ChunkAddress> m_completeTargets;
  std::vector<AddressIndexSlot> m_completeTargetIndex;
  std::uint64_t m_completeTargetGeneration = 0u;
  std::vector<TargetResult> m_completeResults;
  std::uint64_t m_candidateIndexGeneration = 0u;
  std::uint64_t revision = 0;
  std::uint64_t m_changedChunksRevision = 0u;
  bool m_changedChunksRevisionValid = false;
  const std::type_info* lastRuleType = nullptr;
  bool m_frontierInvalid = false;
  std::unique_ptr<SparseWorkerPool> workerPool;
  SparseAdvanceStats lastAdvanceStats;

  static int workerOverride;
  static int cellCandidateOverride;
  static bool chunkNodeReuseOverride;

  CellArray* findChunk(const ChunkAddress& address);
  const CellArray* findChunk(const ChunkAddress& address) const;
  static ChunkData makeChunkData(const CellArray& cells);
  static bool hasNonBackgroundState(const CellArray& cells);
  static std::size_t countOccupiedCells(const ChunkData& chunk);
  static std::size_t countCountedCells(const ChunkData& chunk);
  static void refreshChunkCounts(ChunkData* chunk);
  static bool isCandidatePreferredChunk(const ChunkData& chunk);
  static void addChunkStatistics(const ChunkData& chunk,
                                 ChunkStatistics* statistics);
  static void removeChunkStatistics(const ChunkData& chunk,
                                    ChunkStatistics* statistics);
  static bool hasOccupiedCells(const ChunkData& chunk);
  static void setOccupied(ChunkData* chunk, std::size_t index, bool occupied);
  static void setCounted(ChunkData* chunk, std::size_t index, bool counted);
  static bool sameChunkMaps(const ChunkMap& left, const ChunkMap& right);
  bool prepareNextChunks(std::size_t expectedChunkCount);
  void recycleNextChunks();
  ChunkMap::iterator insertNextChunk(const ChunkAddress& address,
                                     const ChunkData& chunk);
  void finishNextChunks();
  static void beginAddressSet(std::vector<ChunkAddress>* addresses,
                              std::vector<AddressIndexSlot>* index,
                              std::uint64_t* generation);
  static void ensureAddressSetCapacity(
    std::size_t requiredEntries,
    const std::vector<ChunkAddress>& addresses,
    std::vector<AddressIndexSlot>* index,
    std::uint64_t generation);
  static bool insertAddressSet(const ChunkAddress& address,
                               std::vector<ChunkAddress>* addresses,
                               std::vector<AddressIndexSlot>* index,
                               std::uint64_t generation);
  bool markChangedChunk(const ChunkAddress& address);
  void beginNextChangedChunks();
  bool markNextChangedChunk(const ChunkAddress& address);
  void commitNextChangedChunks();
  void publishChangedChunksRevision(bool valid);
  void publishChangedChunkRevision(const ChunkAddress& address, bool valid);
  void collectChangedChunksBetweenMaps();
  void buildFrontierTargets();
  void buildCompleteTargets();
  bool buildFrontierCandidateScratch(std::size_t* estimatedWork,
                                     std::size_t* evaluationWork);
  std::size_t estimateCompleteAdvanceWork() const;
  bool advanceChangedFrontier(const RuleSet& ruleSet, bool useCandidateScratch);
  static std::size_t saturatingAdd(std::size_t left, std::size_t right);
  static std::size_t saturatingMultiply(std::size_t left, std::size_t right);
  void beginCandidateIndexGeneration();
  void ensureCandidateIndexCapacity(std::size_t requiredEntries);
  CandidateScratchChunk* findCandidateScratch(const ChunkAddress& address);
  CandidateScratchChunk* findOrCreateCandidateScratch(
    const ChunkAddress& address);
  static bool markCandidate(CandidateScratchChunk* scratch, std::size_t index);
  void prepareCandidateScratchFromSource(const ChunkAddress& sourceAddress,
                                         const ChunkData& source);
  void prepareCandidateScratchChunk(CandidateScratchChunk* scratch) const;
  void buildCandidateWorkRanges();
  bool advanceCellCandidates(const RuleSet& ruleSet, bool adaptiveTargets);
  unsigned int resolveWorkerCount(std::size_t targetCount) const;
  unsigned int resolveCandidatePreparationWorkerCount(
    std::size_t targetCount,
    std::size_t estimatedWork) const;
  unsigned int resolveCandidateWorkerCount(
    std::size_t candidateCellCount) const;
  void evaluateCandidateChunk(const CandidateScratchChunk& scratch,
                              const unsigned char* transitions,
                              TargetResult* result) const;
  void evaluateTargetChunk(const ChunkAddress& target,
                           const unsigned char* transitions,
                           TargetResult* result) const;
  static void buildHorizontalNeighborRow(
    const ChunkData* const neighborhood[3][3],
    int haloY,
    std::array<unsigned char, kChunkDim>* horizontalCounts);

  friend class SparseWorkerPool;
};
