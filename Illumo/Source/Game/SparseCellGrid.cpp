#include "SparseCellGrid.h"
#include "Rulesets/RuleSet.h"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <thread>
#include <tracy/Tracy.hpp>
#include <unordered_set>

static std::size_t
mixAddress(std::uint64_t value)
{
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
  return static_cast<std::size_t>(value ^ (value >> 31));
}

SparseCellGrid::SparseCellGrid() = default;

struct SparseCellGrid::TargetResult
{
  ChunkAddress address;
  CellArray cells{};
  OccupancyMask occupied{};
  OccupancyMask counted{};
  bool hasNonBackground = false;
};

class SparseWorkerPool
{
public:
  SparseWorkerPool() = default;

  ~SparseWorkerPool()
  {
    {
      std::lock_guard<std::mutex> lock(mutex);
      stopping = true;
      workGeneration += 1;
    }
    workReady.notify_all();
    for (std::thread& worker : workers) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }

  void evaluate(const SparseCellGrid* grid,
                const unsigned char* transitions,
                const std::vector<ChunkAddress>* targets,
                std::vector<SparseCellGrid::TargetResult>* results,
                unsigned int workerCount)
  {
    if (grid == nullptr || transitions == nullptr || targets == nullptr ||
        results == nullptr || workerCount <= 1u || targets->empty()) {
      return;
    }

    const unsigned int workerThreads = workerCount - 1u;
    ensureWorkerCount(workerThreads);

    {
      std::lock_guard<std::mutex> lock(mutex);
      activeGrid = grid;
      activeTransitions = transitions;
      activeTargets = targets;
      activeResults = results;
      activeCandidateScratch = nullptr;
      activeCandidateRanges = nullptr;
      nextWorkItem.store(0u);
      availableWorkerSlots.store(workerThreads);
      completedWorkers = 0u;
      requiredWorkers = workerThreads;
      workGeneration += 1;
    }
    workReady.notify_all();

    executeAvailableWork();

    std::unique_lock<std::mutex> lock(mutex);
    workComplete.wait(lock,
                      [this]() { return completedWorkers >= requiredWorkers; });
  }

  void evaluateCandidates(
    const SparseCellGrid* grid,
    const unsigned char* transitions,
    const std::vector<SparseCellGrid::CandidateScratchChunk>* scratch,
    const std::vector<SparseCellGrid::CandidateWorkRange>* ranges,
    std::vector<SparseCellGrid::TargetResult>* results,
    unsigned int workerCount)
  {
    if (grid == nullptr || transitions == nullptr || scratch == nullptr ||
        ranges == nullptr || results == nullptr || workerCount <= 1u ||
        ranges->empty()) {
      return;
    }

    const unsigned int workerThreads = workerCount - 1u;
    ensureWorkerCount(workerThreads);

    {
      std::lock_guard<std::mutex> lock(mutex);
      activeGrid = grid;
      activeTransitions = transitions;
      activeTargets = nullptr;
      activeCandidateScratch = scratch;
      activeCandidateRanges = ranges;
      activeResults = results;
      nextWorkItem.store(0u);
      availableWorkerSlots.store(workerThreads);
      completedWorkers = 0u;
      requiredWorkers = workerThreads;
      workGeneration += 1;
    }
    workReady.notify_all();

    executeAvailableWork();

    std::unique_lock<std::mutex> lock(mutex);
    workComplete.wait(lock,
                      [this]() { return completedWorkers >= requiredWorkers; });
  }

private:
  std::mutex mutex;
  std::condition_variable workReady;
  std::condition_variable workComplete;
  std::vector<std::thread> workers;
  std::atomic<std::size_t> nextWorkItem{ 0u };
  std::atomic<unsigned int> availableWorkerSlots{ 0u };
  const SparseCellGrid* activeGrid = nullptr;
  const unsigned char* activeTransitions = nullptr;
  const std::vector<ChunkAddress>* activeTargets = nullptr;
  const std::vector<SparseCellGrid::CandidateScratchChunk>*
    activeCandidateScratch = nullptr;
  const std::vector<SparseCellGrid::CandidateWorkRange>* activeCandidateRanges =
    nullptr;
  std::vector<SparseCellGrid::TargetResult>* activeResults = nullptr;
  std::size_t workGeneration = 0u;
  unsigned int requiredWorkers = 0u;
  unsigned int completedWorkers = 0u;
  bool stopping = false;

  void ensureWorkerCount(unsigned int requiredCount)
  {
    while (workers.size() < static_cast<std::size_t>(requiredCount)) {
      workers.emplace_back(&SparseWorkerPool::workerLoop, this);
    }
  }

  bool claimWorkerSlot()
  {
    unsigned int remaining = availableWorkerSlots.load();
    while (remaining > 0u) {
      if (availableWorkerSlots.compare_exchange_weak(remaining,
                                                     remaining - 1u)) {
        return true;
      }
    }
    return false;
  }

  void executeAvailableWork()
  {
    for (;;) {
      const std::size_t index = nextWorkItem.fetch_add(1u);
      if (activeResults == nullptr || activeGrid == nullptr ||
          activeTransitions == nullptr) {
        return;
      }
      if (activeCandidateScratch != nullptr &&
          activeCandidateRanges != nullptr) {
        if (index >= activeCandidateRanges->size()) {
          return;
        }
        const SparseCellGrid::CandidateWorkRange& range =
          (*activeCandidateRanges)[index];
        for (std::size_t scratchIndex = range.begin; scratchIndex < range.end;
             ++scratchIndex) {
          activeGrid->evaluateCandidateChunk(
            (*activeCandidateScratch)[scratchIndex],
            activeTransitions,
            &(*activeResults)[scratchIndex]);
        }
        continue;
      }
      if (activeTargets == nullptr || index >= activeTargets->size()) {
        return;
      }
      activeGrid->evaluateTargetChunk(
        (*activeTargets)[index], activeTransitions, &(*activeResults)[index]);
    }
  }

  void workerLoop()
  {
    std::size_t observedGeneration = 0u;
    for (;;) {
      {
        std::unique_lock<std::mutex> lock(mutex);
        workReady.wait(lock, [this, &observedGeneration]() {
          return stopping || workGeneration != observedGeneration;
        });
        if (stopping) {
          return;
        }
        observedGeneration = workGeneration;
      }

      if (!claimWorkerSlot()) {
        continue;
      }
      executeAvailableWork();

      {
        std::lock_guard<std::mutex> lock(mutex);
        completedWorkers += 1u;
        if (completedWorkers >= requiredWorkers) {
          workComplete.notify_one();
        }
      }
    }
  }
};

SparseCellGrid::~SparseCellGrid() = default;

int SparseCellGrid::workerOverride = 0;
int SparseCellGrid::cellCandidateOverride = 0;
bool SparseCellGrid::chunkNodeReuseOverride = true;

std::size_t
ChunkAddressHash::operator()(const ChunkAddress& address) const noexcept
{
  const std::size_t hx = mixAddress(static_cast<std::uint64_t>(address.x));
  const std::size_t hy = mixAddress(static_cast<std::uint64_t>(address.y));
  return hx ^
         (hy + static_cast<std::size_t>(0x9e3779b9) + (hx << 6) + (hx >> 2));
}

void
SparseCellGrid::beginAddressSet(std::vector<ChunkAddress>* addresses,
                                std::vector<AddressIndexSlot>* index,
                                std::uint64_t* generation)
{
  if (addresses == nullptr || index == nullptr || generation == nullptr) {
    return;
  }
  addresses->clear();
  *generation += 1u;
  if (*generation == 0u) {
    for (AddressIndexSlot& slot : *index) {
      slot.generation = 0u;
    }
    *generation = 1u;
  }
  if (index->empty()) {
    index->resize(16u);
  }
}

void
SparseCellGrid::ensureAddressSetCapacity(
  std::size_t requiredEntries,
  const std::vector<ChunkAddress>& addresses,
  std::vector<AddressIndexSlot>* index,
  std::uint64_t generation)
{
  if (index == nullptr) {
    return;
  }
  if (index->empty()) {
    index->resize(16u);
  }
  std::size_t newCapacity = index->size();
  while (requiredEntries > newCapacity - newCapacity / 4u) {
    if (newCapacity > std::numeric_limits<std::size_t>::max() / 2u) {
      throw std::bad_alloc();
    }
    newCapacity *= 2u;
  }
  if (newCapacity == index->size()) {
    return;
  }

  std::vector<AddressIndexSlot> replacement;
  replacement.resize(newCapacity);
  const std::size_t mask = newCapacity - 1u;
  for (const ChunkAddress& address : addresses) {
    std::size_t slotIndex = ChunkAddressHash{}(address)&mask;
    while (replacement[slotIndex].generation == generation) {
      slotIndex = (slotIndex + 1u) & mask;
    }
    replacement[slotIndex].address = address;
    replacement[slotIndex].generation = generation;
  }
  index->swap(replacement);
}

bool
SparseCellGrid::insertAddressSet(const ChunkAddress& address,
                                 std::vector<ChunkAddress>* addresses,
                                 std::vector<AddressIndexSlot>* index,
                                 std::uint64_t generation)
{
  if (addresses == nullptr || index == nullptr || generation == 0u) {
    return false;
  }
  ensureAddressSetCapacity(
    addresses->size() + 1u, *addresses, index, generation);
  const std::size_t mask = index->size() - 1u;
  std::size_t slotIndex = ChunkAddressHash{}(address)&mask;
  for (;;) {
    AddressIndexSlot& slot = (*index)[slotIndex];
    if (slot.generation != generation) {
      slot.address = address;
      slot.generation = generation;
      addresses->push_back(address);
      return true;
    }
    if (slot.address == address) {
      return false;
    }
    slotIndex = (slotIndex + 1u) & mask;
  }
}

bool
SparseCellGrid::markChangedChunk(const ChunkAddress& address)
{
  if (m_frontierInvalid) {
    return false;
  }
  try {
    if (m_changedChunkGeneration == 0u) {
      beginAddressSet(
        &m_changedChunks, &m_changedChunkIndex, &m_changedChunkGeneration);
    }
    const bool inserted = insertAddressSet(address,
                                           &m_changedChunks,
                                           &m_changedChunkIndex,
                                           m_changedChunkGeneration);
    if (m_changedChunks.size() > kFrontierTrackingLimit) {
      m_frontierInvalid = true;
      m_changedChunks.clear();
      return false;
    }
    return inserted;
  } catch (const std::bad_alloc&) {
    m_frontierInvalid = true;
    m_changedChunks.clear();
    return false;
  }
}

void
SparseCellGrid::beginNextChangedChunks()
{
  beginAddressSet(&m_nextChangedChunks,
                  &m_nextChangedChunkIndex,
                  &m_nextChangedChunkGeneration);
}

bool
SparseCellGrid::markNextChangedChunk(const ChunkAddress& address)
{
  return insertAddressSet(address,
                          &m_nextChangedChunks,
                          &m_nextChangedChunkIndex,
                          m_nextChangedChunkGeneration);
}

void
SparseCellGrid::commitNextChangedChunks()
{
  m_changedChunks.swap(m_nextChangedChunks);
  m_changedChunkIndex.swap(m_nextChangedChunkIndex);
  std::swap(m_changedChunkGeneration, m_nextChangedChunkGeneration);
}

void
SparseCellGrid::collectChangedChunksBetweenMaps()
{
  beginNextChangedChunks();
  for (ChunkMap::const_reference entry : chunks) {
    const ChunkMap::const_iterator next = m_nextChunks.find(entry.first);
    if (next == m_nextChunks.end() ||
        next->second.cells != entry.second.cells) {
      markNextChangedChunk(entry.first);
      if (m_nextChangedChunks.size() > kFrontierTrackingLimit) {
        m_frontierInvalid = true;
        return;
      }
    }
  }
  for (ChunkMap::const_reference entry : m_nextChunks) {
    if (chunks.find(entry.first) == chunks.end()) {
      markNextChangedChunk(entry.first);
      if (m_nextChangedChunks.size() > kFrontierTrackingLimit) {
        m_frontierInvalid = true;
        return;
      }
    }
  }
}

void
SparseCellGrid::buildFrontierTargets()
{
  ZoneScopedN("SparseCellGrid.buildFrontierTargets");
  beginAddressSet(
    &m_frontierTargets, &m_frontierTargetIndex, &m_frontierTargetGeneration);
  for (const ChunkAddress& changed : m_changedChunks) {
    for (int offsetY = -1; offsetY <= 1; ++offsetY) {
      for (int offsetX = -1; offsetX <= 1; ++offsetX) {
        insertAddressSet(
          ChunkAddress{ changed.x + offsetX, changed.y + offsetY },
          &m_frontierTargets,
          &m_frontierTargetIndex,
          m_frontierTargetGeneration);
      }
    }
  }
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

void
SparseCellGrid::setWorkerOverrideForTesting(int workers)
{
  workerOverride = workers < 0 ? 0 : workers;
}

int
SparseCellGrid::getWorkerOverrideForTesting()
{
  return workerOverride;
}

void
SparseCellGrid::setCellCandidateOverrideForTesting(int mode)
{
  if (mode < -1) {
    mode = -1;
  }
  if (mode > 1) {
    mode = 1;
  }
  cellCandidateOverride = mode;
}

int
SparseCellGrid::getCellCandidateOverrideForTesting()
{
  return cellCandidateOverride;
}

void
SparseCellGrid::setChunkNodeReuseOverrideForTesting(bool enabled)
{
  chunkNodeReuseOverride = enabled;
}

bool
SparseCellGrid::getChunkNodeReuseOverrideForTesting()
{
  return chunkNodeReuseOverride;
}

bool
SparseCellGrid::chunkAddressLess(const ChunkAddress& left,
                                 const ChunkAddress& right)
{
  if (left.y != right.y) {
    return left.y < right.y;
  }
  return left.x < right.x;
}

SparseCellGrid::CandidateScratchChunk*
SparseCellGrid::findCandidateScratch(const ChunkAddress& address)
{
  if (m_candidateIndex.empty()) {
    return nullptr;
  }
  const std::size_t mask = m_candidateIndex.size() - 1u;
  std::size_t slotIndex = ChunkAddressHash{}(address)&mask;
  for (;;) {
    CandidateIndexSlot& slot = m_candidateIndex[slotIndex];
    if (slot.generation != m_candidateIndexGeneration) {
      return nullptr;
    }
    if (slot.address == address) {
      return &m_candidateScratch[slot.scratchIndex];
    }
    slotIndex = (slotIndex + 1u) & mask;
  }
}

void
SparseCellGrid::beginCandidateIndexGeneration()
{
  m_candidateScratch.clear();
  m_candidateIndexGeneration += 1u;
  if (m_candidateIndexGeneration == 0u) {
    for (CandidateIndexSlot& slot : m_candidateIndex) {
      slot.generation = 0u;
    }
    m_candidateIndexGeneration = 1u;
  }
  if (m_candidateIndex.empty()) {
    m_candidateIndex.resize(16u);
  }
}

void
SparseCellGrid::ensureCandidateIndexCapacity(std::size_t requiredEntries)
{
  if (m_candidateIndex.empty()) {
    m_candidateIndex.resize(16u);
  }
  std::size_t newCapacity = m_candidateIndex.size();
  while (requiredEntries > newCapacity - newCapacity / 4u) {
    if (newCapacity > std::numeric_limits<std::size_t>::max() / 2u) {
      throw std::bad_alloc();
    }
    newCapacity *= 2u;
  }
  if (newCapacity == m_candidateIndex.size()) {
    return;
  }

  std::vector<CandidateIndexSlot> replacement;
  replacement.resize(newCapacity);
  const std::size_t mask = newCapacity - 1u;
  for (std::size_t scratchIndex = 0u; scratchIndex < m_candidateScratch.size();
       ++scratchIndex) {
    const ChunkAddress& address = m_candidateScratch[scratchIndex].address;
    std::size_t slotIndex = ChunkAddressHash{}(address)&mask;
    while (replacement[slotIndex].generation == m_candidateIndexGeneration) {
      slotIndex = (slotIndex + 1u) & mask;
    }
    CandidateIndexSlot& slot = replacement[slotIndex];
    slot.address = address;
    slot.scratchIndex = scratchIndex;
    slot.generation = m_candidateIndexGeneration;
  }
  m_candidateIndex.swap(replacement);
}

SparseCellGrid::CandidateScratchChunk*
SparseCellGrid::findOrCreateCandidateScratch(const ChunkAddress& address)
{
  ensureCandidateIndexCapacity(m_candidateScratch.size() + 1u);
  const std::size_t mask = m_candidateIndex.size() - 1u;
  std::size_t slotIndex = ChunkAddressHash{}(address)&mask;
  for (;;) {
    CandidateIndexSlot& slot = m_candidateIndex[slotIndex];
    if (slot.generation != m_candidateIndexGeneration) {
      const std::size_t scratchIndex = m_candidateScratch.size();
      m_candidateScratch.emplace_back();
      CandidateScratchChunk& scratch = m_candidateScratch.back();
      scratch.address = address;
      slot.address = address;
      slot.scratchIndex = scratchIndex;
      slot.generation = m_candidateIndexGeneration;
      return &scratch;
    }
    if (slot.address == address) {
      return &m_candidateScratch[slot.scratchIndex];
    }
    slotIndex = (slotIndex + 1u) & mask;
  }
}

bool
SparseCellGrid::markCandidate(CandidateScratchChunk* scratch, std::size_t index)
{
  if (scratch == nullptr || index >= kChunkCellCount) {
    return false;
  }
  const std::size_t wordIndex = index / 64u;
  const std::uint64_t bit = static_cast<std::uint64_t>(1u)
                            << static_cast<unsigned int>(index % 64u);
  if ((scratch->candidates[wordIndex] & bit) != 0u) {
    return false;
  }
  scratch->candidates[wordIndex] |= bit;
  scratch->neighborCounts[index] = 0u;
  return true;
}

unsigned int
SparseCellGrid::resolveWorkerCount(std::size_t targetCount) const
{
  if (targetCount == 0u) {
    return 1u;
  }

  if (workerOverride > 0) {
    const unsigned int requested = static_cast<unsigned int>(workerOverride);
    return std::min(std::min(requested, kMaxParallelWorkers),
                    static_cast<unsigned int>(targetCount));
  }

  if (targetCount < kParallelTargetThreshold) {
    return 1u;
  }

  unsigned int workerCount = std::thread::hardware_concurrency();
  if (workerCount < 2u) {
    return 1u;
  }
  workerCount = std::min(workerCount, kMaxParallelWorkers);
  return std::min(workerCount, static_cast<unsigned int>(targetCount));
}

unsigned int
SparseCellGrid::resolveCandidateWorkerCount(
  std::size_t candidateCellCount) const
{
  if (m_candidateWorkRanges.empty()) {
    return 1u;
  }
  if (workerOverride > 0) {
    const unsigned int requested = static_cast<unsigned int>(workerOverride);
    return std::min(std::min(requested, kMaxParallelWorkers),
                    static_cast<unsigned int>(m_candidateWorkRanges.size()));
  }
  if (candidateCellCount < kParallelCandidateCellThreshold) {
    return 1u;
  }

  unsigned int workerCount = std::thread::hardware_concurrency();
  if (workerCount < 2u) {
    return 1u;
  }
  workerCount = std::min(workerCount, kMaxCandidateWorkers);
  return std::min(workerCount,
                  static_cast<unsigned int>(m_candidateWorkRanges.size()));
}

void
SparseCellGrid::buildCandidateWorkRanges()
{
  ZoneScopedN("SparseCellGrid.buildCandidateWorkRanges");
  m_candidateWorkRanges.clear();
  std::size_t rangeBegin = 0u;
  std::size_t rangeCandidateCount = 0u;
  for (std::size_t scratchIndex = 0u; scratchIndex < m_candidateScratch.size();
       ++scratchIndex) {
    const CandidateScratchChunk& scratch = m_candidateScratch[scratchIndex];
    rangeCandidateCount +=
      scratch.useCellCandidates ? scratch.candidateCellCount : kChunkCellCount;
    if (rangeCandidateCount >= kCandidateCellsPerWorkRange) {
      m_candidateWorkRanges.push_back(
        CandidateWorkRange{ rangeBegin, scratchIndex + 1u });
      rangeBegin = scratchIndex + 1u;
      rangeCandidateCount = 0u;
    }
  }
  if (rangeBegin < m_candidateScratch.size()) {
    m_candidateWorkRanges.push_back(
      CandidateWorkRange{ rangeBegin, m_candidateScratch.size() });
  }
}

void
SparseCellGrid::evaluateCandidateChunk(const CandidateScratchChunk& scratch,
                                       const unsigned char* transitions,
                                       TargetResult* result) const
{
  if (result == nullptr || transitions == nullptr) {
    return;
  }
  result->address = scratch.address;
  result->occupied.fill(0u);
  result->counted.fill(0u);
  result->hasNonBackground = false;
  if (!scratch.useCellCandidates) {
    evaluateTargetChunk(scratch.address, transitions, result);
    return;
  }
  const ChunkMap::const_iterator source = chunks.find(scratch.address);
  for (std::size_t wordIndex = 0u; wordIndex < scratch.candidates.size();
       ++wordIndex) {
    std::uint64_t candidates = scratch.candidates[wordIndex];
    while (candidates != 0u) {
      const unsigned int offset = std::countr_zero(candidates);
      const std::size_t index = wordIndex * 64u + offset;
      candidates &= candidates - 1u;

      const unsigned char current =
        source == chunks.end() ? BackgroundState : source->second.cells[index];
      const unsigned char next = transitions[RuleSet::transitionIndex(
        current, scratch.neighborCounts[index])];
      if (next == BackgroundState) {
        continue;
      }
      result->cells[index] = next;
      const std::uint64_t bit = static_cast<std::uint64_t>(1u)
                                << static_cast<unsigned int>(index % 64u);
      result->occupied[wordIndex] |= bit;
      if (next == CountedNeighborState) {
        result->counted[wordIndex] |= bit;
      }
      result->hasNonBackground = true;
    }
  }
}

SparseCellGrid::CellArray*
SparseCellGrid::findChunk(const ChunkAddress& address)
{
  ChunkMap::iterator found = chunks.find(address);
  return found == chunks.end() ? nullptr : &found->second.cells;
}

const SparseCellGrid::CellArray*
SparseCellGrid::findChunk(const ChunkAddress& address) const
{
  ChunkMap::const_iterator found = chunks.find(address);
  return found == chunks.end() ? nullptr : &found->second.cells;
}

SparseCellGrid::ChunkData*
SparseCellGrid::findOrCreateChunk(const ChunkAddress& address)
{
  ChunkMap::iterator found = chunks.find(address);
  if (found != chunks.end()) {
    return &found->second;
  }

  CellArray blankCells;
  blankCells.fill(BackgroundState);
  std::pair<ChunkMap::iterator, bool> inserted =
    chunks.emplace(address, makeChunkData(blankCells));
  return &inserted.first->second;
}

SparseCellGrid::ChunkData
SparseCellGrid::makeChunkData(const CellArray& cells)
{
  ChunkData result;
  result.cells = cells;
  result.occupied.fill(0u);
  result.counted.fill(0u);
  for (std::size_t index = 0; index < cells.size(); ++index) {
    if (cells[index] != BackgroundState) {
      setOccupied(&result, index, true);
    }
    if (cells[index] == CountedNeighborState) {
      setCounted(&result, index, true);
    }
  }
  return result;
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

std::size_t
SparseCellGrid::countOccupiedCells(const ChunkData& chunk)
{
  std::size_t count = 0u;
  for (std::uint64_t word : chunk.occupied) {
    count += static_cast<std::size_t>(std::popcount(word));
  }
  return count;
}

std::size_t
SparseCellGrid::countCountedCells(const ChunkData& chunk)
{
  std::size_t count = 0u;
  for (std::uint64_t word : chunk.counted) {
    count += static_cast<std::size_t>(std::popcount(word));
  }
  return count;
}

bool
SparseCellGrid::hasOccupiedCells(const ChunkData& chunk)
{
  for (std::uint64_t word : chunk.occupied) {
    if (word != 0u) {
      return true;
    }
  }
  return false;
}

void
SparseCellGrid::setOccupied(ChunkData* chunk, std::size_t index, bool occupied)
{
  if (chunk == nullptr || index >= kChunkCellCount) {
    return;
  }
  const std::size_t word = index / 64u;
  const std::uint64_t bit = static_cast<std::uint64_t>(1u)
                            << static_cast<unsigned int>(index % 64u);
  if (occupied) {
    chunk->occupied[word] |= bit;
  } else {
    chunk->occupied[word] &= ~bit;
  }
}

void
SparseCellGrid::setCounted(ChunkData* chunk, std::size_t index, bool counted)
{
  if (chunk == nullptr || index >= kChunkCellCount) {
    return;
  }
  const std::size_t word = index / 64u;
  const std::uint64_t bit = static_cast<std::uint64_t>(1u)
                            << static_cast<unsigned int>(index % 64u);
  if (counted) {
    chunk->counted[word] |= bit;
  } else {
    chunk->counted[word] &= ~bit;
  }
}

bool
SparseCellGrid::sameChunkMaps(const ChunkMap& left, const ChunkMap& right)
{
  if (left.size() != right.size()) {
    return false;
  }
  for (ChunkMap::const_reference entry : left) {
    const ChunkMap::const_iterator found = right.find(entry.first);
    if (found == right.end() || found->second.cells != entry.second.cells) {
      return false;
    }
  }
  return true;
}

bool
SparseCellGrid::prepareNextChunks(std::size_t expectedChunkCount)
{
  ZoneScopedN("SparseCellGrid.prepareNextChunks");
  try {
    if (!chunkNodeReuseOverride) {
      m_nextChunks.clear();
      m_recycledChunkNodes.clear();
    }
    if (m_nextChunks.size() >
        std::numeric_limits<std::size_t>::max() - m_recycledChunkNodes.size()) {
      throw std::bad_alloc();
    }
    const std::size_t retainedChunkCount =
      m_recycledChunkNodes.size() + m_nextChunks.size();
    const std::size_t requiredNodeCapacity =
      std::max(retainedChunkCount, expectedChunkCount);
    if (m_recycledChunkNodes.capacity() < requiredNodeCapacity) {
      m_recycledChunkNodes.reserve(requiredNodeCapacity);
    }

    if (chunkNodeReuseOverride) {
      recycleNextChunks();
    }

    const double retainedBucketCapacity =
      static_cast<double>(m_nextChunks.bucket_count()) *
      static_cast<double>(m_nextChunks.max_load_factor());
    if (static_cast<double>(expectedChunkCount) > retainedBucketCapacity) {
      m_nextChunks.reserve(expectedChunkCount);
    }
  } catch (const std::bad_alloc&) {
    return false;
  }
  return true;
}

void
SparseCellGrid::recycleNextChunks()
{
  ZoneScopedN("SparseCellGrid.recycleNextChunks");
  while (!m_nextChunks.empty()) {
    const ChunkMap::iterator entry = m_nextChunks.begin();
    m_recycledChunkNodes.push_back(m_nextChunks.extract(entry));
  }
}

SparseCellGrid::ChunkMap::iterator
SparseCellGrid::insertNextChunk(const ChunkAddress& address,
                                const ChunkData& chunk)
{
  if (m_recycledChunkNodes.empty()) {
    const std::pair<ChunkMap::iterator, bool> inserted =
      m_nextChunks.emplace(address, chunk);
    if (inserted.second) {
      lastAdvanceStats.allocatedChunkNodeCount += 1u;
    }
    return inserted.first;
  }

  ChunkNode node = std::move(m_recycledChunkNodes.back());
  m_recycledChunkNodes.pop_back();
  node.key() = address;
  node.mapped() = chunk;
  ChunkMap::insert_return_type inserted = m_nextChunks.insert(std::move(node));
  if (!inserted.inserted) {
    m_recycledChunkNodes.push_back(std::move(inserted.node));
    return inserted.position;
  }
  lastAdvanceStats.reusedChunkNodeCount += 1u;
  return inserted.position;
}

void
SparseCellGrid::finishNextChunks()
{
  ZoneScopedN("SparseCellGrid.finishNextChunks");
  bool changed = false;
  try {
    m_frontierInvalid = false;
    collectChangedChunksBetweenMaps();
    changed = m_frontierInvalid || !m_nextChangedChunks.empty();
  } catch (const std::bad_alloc&) {
    changed = !sameChunkMaps(chunks, m_nextChunks);
    m_frontierInvalid = true;
  }
  if (changed) {
    chunks.swap(m_nextChunks);
    revision += 1u;
  }
  if (!m_frontierInvalid) {
    commitNextChangedChunks();
  }
  lastAdvanceStats.retainedChunkNodeCount =
    chunkNodeReuseOverride ? m_recycledChunkNodes.size() + m_nextChunks.size()
                           : 0u;
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
  const std::size_t index =
    static_cast<std::size_t>(localIndexForCell(address));
  ChunkMap::iterator found = chunks.find(chunkAddress);

  if (state == BackgroundState) {
    if (found == chunks.end()) {
      return true;
    }
    if (found->second.cells[index] == BackgroundState) {
      return true;
    }
    markChangedChunk(chunkAddress);
    found->second.cells[index] = BackgroundState;
    setOccupied(&found->second, index, false);
    setCounted(&found->second, index, false);
    if (!hasOccupiedCells(found->second)) {
      chunks.erase(found);
    }
    revision += 1;
    return true;
  }

  ChunkData* chunk = nullptr;
  try {
    chunk = findOrCreateChunk(chunkAddress);
  } catch (const std::bad_alloc&) {
    return false;
  }
  if (chunk == nullptr) {
    return false;
  }
  if (chunk->cells[index] != state) {
    markChangedChunk(chunkAddress);
    chunk->cells[index] = state;
    setOccupied(chunk, index, true);
    setCounted(chunk, index, state == CountedNeighborState);
    revision += 1;
  }
  return true;
}

void
SparseCellGrid::clear()
{
  if (!chunks.empty()) {
    for (ChunkMap::const_reference entry : chunks) {
      markChangedChunk(entry.first);
      if (m_frontierInvalid) {
        break;
      }
    }
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
    const ChunkAddress address{ record.chunkX, record.chunkY };
    ChunkMap::iterator found = chunks.find(address);
    if (found != chunks.end() && found->second.cells == record.cells) {
      return true;
    }
    markChangedChunk(address);
    chunks[address] = makeChunkData(record.cells);
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
    record.cells = entry.second.cells;
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
SparseCellGrid::visitChunksInBounds(const ChunkAddress& minimum,
                                    const ChunkAddress& maximum,
                                    const ChunkVisitor& visitor) const
{
  if (!visitor || minimum.x > maximum.x || minimum.y > maximum.y) {
    return;
  }
  for (std::int64_t chunkY = minimum.y;; ++chunkY) {
    for (std::int64_t chunkX = minimum.x;; ++chunkX) {
      const ChunkAddress address{ chunkX, chunkY };
      const CellArray* cells = findChunk(address);
      if (cells != nullptr) {
        visitor(address, *cells);
      }
      if (chunkX == maximum.x) {
        break;
      }
    }
    if (chunkY == maximum.y) {
      break;
    }
  }
}

void
SparseCellGrid::swap(SparseCellGrid& other) noexcept
{
  const bool changed = !sameChunkMaps(chunks, other.chunks);
  chunks.swap(other.chunks);
  m_nextChunks.swap(other.m_nextChunks);
  m_recycledChunkNodes.swap(other.m_recycledChunkNodes);
  m_changedChunks.swap(other.m_changedChunks);
  m_changedChunkIndex.swap(other.m_changedChunkIndex);
  std::swap(m_changedChunkGeneration, other.m_changedChunkGeneration);
  m_nextChangedChunks.swap(other.m_nextChangedChunks);
  m_nextChangedChunkIndex.swap(other.m_nextChangedChunkIndex);
  std::swap(m_nextChangedChunkGeneration, other.m_nextChangedChunkGeneration);
  std::swap(lastRuleType, other.lastRuleType);
  std::swap(m_frontierInvalid, other.m_frontierInvalid);
  if (changed) {
    revision += 1;
    other.revision += 1;
  }
}

void
SparseCellGrid::buildHorizontalNeighborRow(
  const HaloCells& halo,
  int haloY,
  std::array<unsigned char, kChunkDim>* horizontalCounts)
{
  if (horizontalCounts == nullptr || haloY < 0 || haloY >= kHaloDim) {
    return;
  }

  const std::size_t rowStart = static_cast<std::size_t>(haloY * kHaloDim);
  unsigned char rollingCount = 0u;
  for (int haloX = 0; haloX < 3; ++haloX) {
    if (halo[rowStart + static_cast<std::size_t>(haloX)] ==
        CountedNeighborState) {
      rollingCount = static_cast<unsigned char>(rollingCount + 1u);
    }
  }

  for (int localX = 0; localX < kChunkDim; ++localX) {
    (*horizontalCounts)[static_cast<std::size_t>(localX)] = rollingCount;
    if (localX + 1 >= kChunkDim) {
      continue;
    }
    if (halo[rowStart + static_cast<std::size_t>(localX)] ==
        CountedNeighborState) {
      rollingCount = static_cast<unsigned char>(rollingCount - 1u);
    }
    if (halo[rowStart + static_cast<std::size_t>(localX + 3)] ==
        CountedNeighborState) {
      rollingCount = static_cast<unsigned char>(rollingCount + 1u);
    }
  }
}

void
SparseCellGrid::evaluateTargetChunk(const ChunkAddress& target,
                                    const unsigned char* transitions,
                                    TargetResult* result) const
{
  if (result == nullptr || transitions == nullptr) {
    return;
  }

  const CellArray* neighborhood[3][3];
  for (int neighborY = -1; neighborY <= 1; ++neighborY) {
    for (int neighborX = -1; neighborX <= 1; ++neighborX) {
      const ChunkAddress neighborAddress{ target.x + neighborX,
                                          target.y + neighborY };
      neighborhood[neighborY + 1][neighborX + 1] = findChunk(neighborAddress);
    }
  }

  HaloCells halo;
  for (int haloY = 0; haloY < kHaloDim; ++haloY) {
    for (int haloX = 0; haloX < kHaloDim; ++haloX) {
      const int sourceX = haloX - 1;
      const int sourceY = haloY - 1;
      const int chunkX = sourceX < 0 ? 0 : (sourceX >= kChunkDim ? 2 : 1);
      const int chunkY = sourceY < 0 ? 0 : (sourceY >= kChunkDim ? 2 : 1);
      const int localX =
        sourceX < 0 ? kChunkDim - 1 : (sourceX >= kChunkDim ? 0 : sourceX);
      const int localY =
        sourceY < 0 ? kChunkDim - 1 : (sourceY >= kChunkDim ? 0 : sourceY);
      const CellArray* source = neighborhood[chunkY][chunkX];
      halo[static_cast<std::size_t>(haloY * kHaloDim + haloX)] =
        source == nullptr
          ? BackgroundState
          : (*source)[static_cast<std::size_t>(localY * kChunkDim + localX)];
    }
  }

  result->address = target;
  result->cells.fill(BackgroundState);
  result->occupied.fill(0u);
  result->counted.fill(0u);
  result->hasNonBackground = false;
  std::array<unsigned char, kChunkDim> previousRow;
  std::array<unsigned char, kChunkDim> currentRow;
  std::array<unsigned char, kChunkDim> nextRow;
  buildHorizontalNeighborRow(halo, 0, &previousRow);
  buildHorizontalNeighborRow(halo, 1, &currentRow);
  buildHorizontalNeighborRow(halo, 2, &nextRow);
  for (int localY = 0; localY < kChunkDim; ++localY) {
    for (int localX = 0; localX < kChunkDim; ++localX) {
      const int haloIndex = (localY + 1) * kHaloDim + localX + 1;
      const unsigned char current = halo[static_cast<std::size_t>(haloIndex)];
      unsigned char aliveNeighbors = static_cast<unsigned char>(
        previousRow[static_cast<std::size_t>(localX)] +
        currentRow[static_cast<std::size_t>(localX)] +
        nextRow[static_cast<std::size_t>(localX)]);
      if (current == CountedNeighborState) {
        aliveNeighbors = static_cast<unsigned char>(aliveNeighbors - 1u);
      }
      const std::size_t resultIndex =
        static_cast<std::size_t>(localY * kChunkDim + localX);
      const unsigned char next =
        transitions[RuleSet::transitionIndex(current, aliveNeighbors)];
      result->cells[resultIndex] = next;
      if (next != BackgroundState) {
        const std::size_t word = resultIndex / 64u;
        const std::uint64_t bit =
          static_cast<std::uint64_t>(1u)
          << static_cast<unsigned int>(resultIndex % 64u);
        result->occupied[word] |= bit;
        if (next == CountedNeighborState) {
          result->counted[word] |= bit;
        }
        result->hasNonBackground = true;
      }
    }
    if (localY + 1 < kChunkDim) {
      previousRow = currentRow;
      currentRow = nextRow;
      buildHorizontalNeighborRow(halo, localY + 3, &nextRow);
    }
  }
}

bool
SparseCellGrid::advanceChangedFrontier(const RuleSet& ruleSet)
{
  ZoneScopedN("SparseCellGrid.advanceChangedFrontier");
  try {
    const unsigned char* transitions = ruleSet.getTransitionTable().data();
    m_frontierResults.resize(m_frontierTargets.size());
    lastAdvanceStats.targetChunkCount = m_frontierTargets.size();
    lastAdvanceStats.frontierTargetCount = m_frontierTargets.size();
    lastAdvanceStats.haloTargetCount = m_frontierTargets.size();
    lastAdvanceStats.workerCount = resolveWorkerCount(m_frontierTargets.size());
    lastAdvanceStats.usedChangedFrontier = true;

    if (lastAdvanceStats.workerCount > 1u) {
      ZoneScopedN("SparseCellGrid.evaluateFrontierParallel");
      if (workerPool == nullptr) {
        workerPool = std::make_unique<SparseWorkerPool>();
      }
      workerPool->evaluate(this,
                           transitions,
                           &m_frontierTargets,
                           &m_frontierResults,
                           lastAdvanceStats.workerCount);
    } else {
      ZoneScopedN("SparseCellGrid.evaluateFrontierSerial");
      for (std::size_t index = 0u; index < m_frontierTargets.size(); ++index) {
        evaluateTargetChunk(
          m_frontierTargets[index], transitions, &m_frontierResults[index]);
      }
    }

    beginNextChangedChunks();
    for (const TargetResult& result : m_frontierResults) {
      const ChunkMap::const_iterator current = chunks.find(result.address);
      bool differs = result.hasNonBackground != (current != chunks.end());
      if (!differs && result.hasNonBackground) {
        differs = current->second.occupied != result.occupied;
        if (!differs) {
          for (std::size_t wordIndex = 0u;
               wordIndex < result.occupied.size() && !differs;
               ++wordIndex) {
            std::uint64_t occupied = result.occupied[wordIndex];
            while (occupied != 0u) {
              const unsigned int offset = std::countr_zero(occupied);
              const std::size_t cellIndex = wordIndex * 64u + offset;
              occupied &= occupied - 1u;
              if (current->second.cells[cellIndex] != result.cells[cellIndex]) {
                differs = true;
                break;
              }
            }
          }
        }
      }
      if (differs) {
        markNextChangedChunk(result.address);
      }
    }

    if (m_recycledChunkNodes.size() >
        std::numeric_limits<std::size_t>::max() - m_frontierTargets.size()) {
      throw std::bad_alloc();
    }
    m_recycledChunkNodes.reserve(m_recycledChunkNodes.size() +
                                 m_frontierTargets.size());
    if (m_nextChunks.size() >
        std::numeric_limits<std::size_t>::max() - m_frontierTargets.size()) {
      throw std::bad_alloc();
    }
    const std::size_t maximumMapSize =
      m_nextChunks.size() + m_frontierTargets.size();
    const double retainedBucketCapacity =
      static_cast<double>(m_nextChunks.bucket_count()) *
      static_cast<double>(m_nextChunks.max_load_factor());
    if (static_cast<double>(maximumMapSize) > retainedBucketCapacity) {
      m_nextChunks.reserve(maximumMapSize);
    }

    {
      ZoneScopedN("SparseCellGrid.applyFrontierResults");
      for (const TargetResult& result : m_frontierResults) {
        ChunkMap::iterator destination = m_nextChunks.find(result.address);
        if (!result.hasNonBackground) {
          if (destination != m_nextChunks.end()) {
            m_recycledChunkNodes.push_back(m_nextChunks.extract(destination));
          }
          continue;
        }

        ChunkData next;
        next.cells = result.cells;
        next.occupied = result.occupied;
        next.counted = result.counted;
        if (destination == m_nextChunks.end()) {
          insertNextChunk(result.address, next);
        } else {
          destination->second = next;
          lastAdvanceStats.reusedChunkNodeCount += 1u;
        }
      }
    }
  } catch (const std::bad_alloc&) {
    return false;
  } catch (const std::exception&) {
    return false;
  }

  const bool changed = !m_nextChangedChunks.empty();
  if (changed) {
    chunks.swap(m_nextChunks);
    revision += 1u;
  }
  commitNextChangedChunks();
  m_frontierInvalid = false;
  lastAdvanceStats.retainedChunkNodeCount =
    m_recycledChunkNodes.size() + m_nextChunks.size();
  return true;
}

bool
SparseCellGrid::advanceCellCandidates(const RuleSet& ruleSet,
                                      bool adaptiveTargets)
{
  ZoneScopedN("SparseCellGrid.advanceCellCandidates");

  try {
    const unsigned char* transitions = ruleSet.getTransitionTable().data();
    std::size_t candidateCellCount = 0u;
    {
      ZoneScopedN("SparseCellGrid.collectCandidateChunks");
      beginCandidateIndexGeneration();
      for (ChunkMap::const_reference entry : chunks) {
        findOrCreateCandidateScratch(entry.first);
        bool negativeX = false;
        bool positiveX = false;
        bool negativeY = false;
        bool positiveY = false;
        bool negativeXNegativeY = false;
        bool positiveXNegativeY = false;
        bool negativeXPositiveY = false;
        bool positiveXPositiveY = false;

        for (std::size_t wordIndex = 0u;
             wordIndex < entry.second.counted.size();
             ++wordIndex) {
          std::uint64_t counted = entry.second.counted[wordIndex];
          while (counted != 0u) {
            const unsigned int offset = std::countr_zero(counted);
            const std::size_t index = wordIndex * 64u + offset;
            counted &= counted - 1u;
            const int localX = static_cast<int>(index % kChunkDim);
            const int localY = static_cast<int>(index / kChunkDim);
            negativeX = negativeX || localX == 0;
            positiveX = positiveX || localX == kChunkDim - 1;
            negativeY = negativeY || localY == 0;
            positiveY = positiveY || localY == kChunkDim - 1;
            if (localX == 0 && localY == 0) {
              negativeXNegativeY = true;
            }
            if (localX == kChunkDim - 1 && localY == 0) {
              positiveXNegativeY = true;
            }
            if (localX == 0 && localY == kChunkDim - 1) {
              negativeXPositiveY = true;
            }
            if (localX == kChunkDim - 1 && localY == kChunkDim - 1) {
              positiveXPositiveY = true;
            }
          }
        }

        if (negativeX) {
          findOrCreateCandidateScratch(
            ChunkAddress{ entry.first.x - 1, entry.first.y });
        }
        if (positiveX) {
          findOrCreateCandidateScratch(
            ChunkAddress{ entry.first.x + 1, entry.first.y });
        }
        if (negativeY) {
          findOrCreateCandidateScratch(
            ChunkAddress{ entry.first.x, entry.first.y - 1 });
        }
        if (positiveY) {
          findOrCreateCandidateScratch(
            ChunkAddress{ entry.first.x, entry.first.y + 1 });
        }
        if (negativeXNegativeY) {
          findOrCreateCandidateScratch(
            ChunkAddress{ entry.first.x - 1, entry.first.y - 1 });
        }
        if (positiveXNegativeY) {
          findOrCreateCandidateScratch(
            ChunkAddress{ entry.first.x + 1, entry.first.y - 1 });
        }
        if (negativeXPositiveY) {
          findOrCreateCandidateScratch(
            ChunkAddress{ entry.first.x - 1, entry.first.y + 1 });
        }
        if (positiveXPositiveY) {
          findOrCreateCandidateScratch(
            ChunkAddress{ entry.first.x + 1, entry.first.y + 1 });
        }
      }
    }

    {
      ZoneScopedN("SparseCellGrid.buildCandidateScratch");
      for (ChunkMap::const_reference entry : chunks) {
        CandidateScratchChunk* neighborhood[3][3];
        for (int chunkOffsetY = -1; chunkOffsetY <= 1; ++chunkOffsetY) {
          for (int chunkOffsetX = -1; chunkOffsetX <= 1; ++chunkOffsetX) {
            const ChunkAddress address{ entry.first.x + chunkOffsetX,
                                        entry.first.y + chunkOffsetY };
            neighborhood[chunkOffsetY + 1][chunkOffsetX + 1] =
              findCandidateScratch(address);
          }
        }

        CandidateScratchChunk* center = neighborhood[1][1];
        if (center == nullptr) {
          return false;
        }
        for (std::size_t wordIndex = 0u;
             wordIndex < entry.second.occupied.size();
             ++wordIndex) {
          std::uint64_t occupied = entry.second.occupied[wordIndex];
          while (occupied != 0u) {
            const unsigned int offset = std::countr_zero(occupied);
            const std::size_t index = wordIndex * 64u + offset;
            occupied &= occupied - 1u;

            if (markCandidate(center, index)) {
              candidateCellCount += 1u;
              center->candidateCellCount += 1u;
            }
          }
        }

        for (std::size_t wordIndex = 0u;
             wordIndex < entry.second.counted.size();
             ++wordIndex) {
          std::uint64_t counted = entry.second.counted[wordIndex];
          while (counted != 0u) {
            const unsigned int offset = std::countr_zero(counted);
            const std::size_t index = wordIndex * 64u + offset;
            counted &= counted - 1u;
            const int localX = static_cast<int>(index % kChunkDim);
            const int localY = static_cast<int>(index / kChunkDim);
            for (int offsetY = -1; offsetY <= 1; ++offsetY) {
              for (int offsetX = -1; offsetX <= 1; ++offsetX) {
                if (offsetX == 0 && offsetY == 0) {
                  continue;
                }
                int neighborLocalX = localX + offsetX;
                int neighborLocalY = localY + offsetY;
                int neighborChunkX = 0;
                int neighborChunkY = 0;
                if (neighborLocalX < 0) {
                  neighborChunkX = -1;
                  neighborLocalX += kChunkDim;
                } else if (neighborLocalX >= kChunkDim) {
                  neighborChunkX = 1;
                  neighborLocalX -= kChunkDim;
                }
                if (neighborLocalY < 0) {
                  neighborChunkY = -1;
                  neighborLocalY += kChunkDim;
                } else if (neighborLocalY >= kChunkDim) {
                  neighborChunkY = 1;
                  neighborLocalY -= kChunkDim;
                }

                CandidateScratchChunk* destination =
                  neighborhood[neighborChunkY + 1][neighborChunkX + 1];
                if (destination == nullptr) {
                  return false;
                }
                const std::size_t neighborIndex = static_cast<std::size_t>(
                  neighborLocalY * kChunkDim + neighborLocalX);
                if (markCandidate(destination, neighborIndex)) {
                  candidateCellCount += 1u;
                  destination->candidateCellCount += 1u;
                }
                destination->neighborCounts[neighborIndex] =
                  static_cast<unsigned char>(
                    destination->neighborCounts[neighborIndex] + 1u);
                destination->neighborContributionCount += 1u;
              }
            }
          }
        }
      }
    }

    lastAdvanceStats.targetChunkCount = m_candidateScratch.size();
    lastAdvanceStats.candidateCellCount = candidateCellCount;
    std::size_t evaluationCellCount = 0u;
    for (CandidateScratchChunk& scratch : m_candidateScratch) {
      scratch.useCellCandidates =
        !adaptiveTargets || scratch.neighborContributionCount <
                              kCandidateNeighborContributionThreshold;
      const std::size_t targetWork = scratch.useCellCandidates
                                       ? scratch.candidateCellCount
                                       : kChunkCellCount;
      if (evaluationCellCount >
          std::numeric_limits<std::size_t>::max() - targetWork) {
        evaluationCellCount = std::numeric_limits<std::size_t>::max();
      } else {
        evaluationCellCount += targetWork;
      }
      if (scratch.useCellCandidates) {
        lastAdvanceStats.candidateTargetCount += 1u;
      } else {
        lastAdvanceStats.haloTargetCount += 1u;
      }
    }
    lastAdvanceStats.usedCellCandidates =
      lastAdvanceStats.candidateTargetCount != 0u;
    lastAdvanceStats.usedMixedTargets =
      lastAdvanceStats.candidateTargetCount != 0u &&
      lastAdvanceStats.haloTargetCount != 0u;
    const bool parallelCandidatesRequested =
      workerOverride > 1 ||
      (workerOverride == 0 &&
       evaluationCellCount >= kParallelCandidateCellThreshold);
    if (parallelCandidatesRequested) {
      buildCandidateWorkRanges();
      lastAdvanceStats.candidateWorkRangeCount = m_candidateWorkRanges.size();
      lastAdvanceStats.workerCount =
        resolveCandidateWorkerCount(evaluationCellCount);
    }
    if (parallelCandidatesRequested || lastAdvanceStats.haloTargetCount != 0u) {
      m_candidateResults.resize(m_candidateScratch.size());
    }

    if (lastAdvanceStats.workerCount > 1u ||
        lastAdvanceStats.haloTargetCount != 0u) {
      if (lastAdvanceStats.workerCount > 1u) {
        ZoneScopedN("SparseCellGrid.evaluateCellCandidates");
        if (workerPool == nullptr) {
          workerPool = std::make_unique<SparseWorkerPool>();
        }
        workerPool->evaluateCandidates(this,
                                       transitions,
                                       &m_candidateScratch,
                                       &m_candidateWorkRanges,
                                       &m_candidateResults,
                                       lastAdvanceStats.workerCount);
      } else {
        ZoneScopedN("SparseCellGrid.evaluateMixedTargetsSerial");
        for (std::size_t index = 0u; index < m_candidateScratch.size();
             ++index) {
          evaluateCandidateChunk(
            m_candidateScratch[index], transitions, &m_candidateResults[index]);
        }
      }

      {
        ZoneScopedN("SparseCellGrid.mergeCandidateResults");
        if (!prepareNextChunks(m_candidateScratch.size())) {
          return false;
        }
        for (const TargetResult& result : m_candidateResults) {
          if (!result.hasNonBackground) {
            continue;
          }
          ChunkData next;
          next.cells.fill(BackgroundState);
          next.occupied = result.occupied;
          next.counted = result.counted;
          for (std::size_t wordIndex = 0u; wordIndex < result.occupied.size();
               ++wordIndex) {
            std::uint64_t occupied = result.occupied[wordIndex];
            while (occupied != 0u) {
              const unsigned int offset = std::countr_zero(occupied);
              const std::size_t index = wordIndex * 64u + offset;
              occupied &= occupied - 1u;
              next.cells[index] = result.cells[index];
            }
          }
          insertNextChunk(result.address, next);
        }
      }
    } else {
      ZoneScopedN("SparseCellGrid.evaluateCellCandidatesSerial");
      if (!prepareNextChunks(m_candidateScratch.size())) {
        return false;
      }
      for (const CandidateScratchChunk& scratch : m_candidateScratch) {
        const ChunkMap::const_iterator source = chunks.find(scratch.address);
        ChunkMap::iterator destination = m_nextChunks.end();
        for (std::size_t wordIndex = 0u; wordIndex < scratch.candidates.size();
             ++wordIndex) {
          std::uint64_t candidates = scratch.candidates[wordIndex];
          while (candidates != 0u) {
            const unsigned int offset = std::countr_zero(candidates);
            const std::size_t index = wordIndex * 64u + offset;
            candidates &= candidates - 1u;

            const unsigned char current = source == chunks.end()
                                            ? BackgroundState
                                            : source->second.cells[index];
            const unsigned char next = transitions[RuleSet::transitionIndex(
              current, scratch.neighborCounts[index])];
            if (next == BackgroundState) {
              continue;
            }
            if (destination == m_nextChunks.end()) {
              ChunkData blank;
              blank.cells.fill(BackgroundState);
              blank.occupied.fill(0u);
              blank.counted.fill(0u);
              destination = insertNextChunk(scratch.address, blank);
            }
            destination->second.cells[index] = next;
            setOccupied(&destination->second, index, true);
            setCounted(
              &destination->second, index, next == CountedNeighborState);
          }
        }
      }
    }
  } catch (const std::bad_alloc&) {
    return false;
  } catch (const std::exception&) {
    return false;
  }

  finishNextChunks();
  return true;
}

bool
SparseCellGrid::advance(const RuleSet& ruleSet)
{
  ZoneScopedN("SparseCellGrid.advance");
  lastAdvanceStats.activeChunkCount = chunks.size();
  lastAdvanceStats.activeCellCount = 0u;
  lastAdvanceStats.countedCellCount = 0u;
  lastAdvanceStats.targetChunkCount = 0u;
  lastAdvanceStats.candidateCellCount = 0u;
  lastAdvanceStats.candidateTargetCount = 0u;
  lastAdvanceStats.haloTargetCount = 0u;
  lastAdvanceStats.allocatedChunkNodeCount = 0u;
  lastAdvanceStats.reusedChunkNodeCount = 0u;
  lastAdvanceStats.retainedChunkNodeCount =
    m_recycledChunkNodes.size() + m_nextChunks.size();
  lastAdvanceStats.candidateWorkRangeCount = 0u;
  lastAdvanceStats.changedChunkCount = m_changedChunks.size();
  lastAdvanceStats.frontierTargetCount = 0u;
  lastAdvanceStats.workerCount = 1u;
  lastAdvanceStats.usedCellCandidates = false;
  lastAdvanceStats.usedMixedTargets = false;
  lastAdvanceStats.usedChangedFrontier = false;

  bool hasCandidatePreferredChunk = false;
  for (ChunkMap::const_reference entry : chunks) {
    lastAdvanceStats.activeCellCount += countOccupiedCells(entry.second);
    const std::size_t countedCellCount = countCountedCells(entry.second);
    lastAdvanceStats.countedCellCount += countedCellCount;
    if (countedCellCount < kCandidateCellsPerChunkThreshold) {
      hasCandidatePreferredChunk = true;
    }
  }

  if (lastRuleType != &typeid(ruleSet)) {
    lastRuleType = &typeid(ruleSet);
    for (ChunkMap::const_reference entry : chunks) {
      markChangedChunk(entry.first);
      if (m_frontierInvalid) {
        break;
      }
    }
    lastAdvanceStats.changedChunkCount = m_changedChunks.size();
  }

  if (cellCandidateOverride == 0 && !m_frontierInvalid) {
    if (m_changedChunks.empty()) {
      lastAdvanceStats.usedChangedFrontier = true;
      return true;
    }
    try {
      buildFrontierTargets();
    } catch (const std::bad_alloc&) {
      m_frontierInvalid = true;
    }
    if (!m_frontierInvalid &&
        m_frontierTargets.size() <= kFrontierTargetThreshold) {
      return advanceChangedFrontier(ruleSet);
    }
  }

  if (cellCandidateOverride > 0) {
    return advanceCellCandidates(ruleSet, false);
  }
  if (cellCandidateOverride == 0 && hasCandidatePreferredChunk) {
    return advanceCellCandidates(ruleSet, true);
  }

  std::unordered_set<ChunkAddress, ChunkAddressHash> targets;
  std::vector<ChunkAddress> targetAddresses;
  std::vector<TargetResult> targetResults;

  try {
    const unsigned char* transitions = ruleSet.getTransitionTable().data();
    {
      ZoneScopedN("SparseCellGrid.collectTargets");
      targets.reserve(chunks.size() * 9u + 1u);
      for (ChunkMap::const_reference entry : chunks) {
        for (int offsetY = -1; offsetY <= 1; ++offsetY) {
          for (int offsetX = -1; offsetX <= 1; ++offsetX) {
            targets.insert(
              ChunkAddress{ entry.first.x + offsetX, entry.first.y + offsetY });
          }
        }
      }
    }
    targetAddresses.reserve(targets.size());
    for (const ChunkAddress& target : targets) {
      targetAddresses.push_back(target);
    }
    std::sort(targetAddresses.begin(),
              targetAddresses.end(),
              SparseCellGrid::chunkAddressLess);

    lastAdvanceStats.targetChunkCount = targetAddresses.size();
    lastAdvanceStats.haloTargetCount = targetAddresses.size();
    lastAdvanceStats.workerCount = resolveWorkerCount(targetAddresses.size());
    targetResults.resize(targetAddresses.size());

    if (lastAdvanceStats.workerCount > 1u) {
      ZoneScopedN("SparseCellGrid.evaluateParallel");
      if (workerPool == nullptr) {
        workerPool = std::make_unique<SparseWorkerPool>();
      }
      workerPool->evaluate(this,
                           transitions,
                           &targetAddresses,
                           &targetResults,
                           lastAdvanceStats.workerCount);
    } else {
      ZoneScopedN("SparseCellGrid.evaluateSerial");
      for (std::size_t index = 0; index < targetAddresses.size(); ++index) {
        evaluateTargetChunk(
          targetAddresses[index], transitions, &targetResults[index]);
      }
    }

    {
      ZoneScopedN("SparseCellGrid.mergeResults");
      if (!prepareNextChunks(targetResults.size())) {
        return false;
      }
      for (const TargetResult& result : targetResults) {
        if (result.hasNonBackground) {
          ChunkData next;
          next.cells = result.cells;
          next.occupied = result.occupied;
          next.counted = result.counted;
          insertNextChunk(result.address, next);
        }
      }
    }
  } catch (const std::bad_alloc&) {
    return false;
  } catch (const std::exception&) {
    return false;
  }

  finishNextChunks();
  return true;
}
