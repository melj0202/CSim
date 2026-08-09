#include "Game/CanvasView.h"
#include "Game/Cursor.h"
#include "Game/SparseCellGrid.h"
#include "Rendering/Camera.h"
#include "Rendering/Mock/MockBackend.h"
#include "Rendering/Renderer.h"
#include "Rulesets/BrainsBrainRuleSet.h"
#include "Rulesets/DayAndNightRuleSet.h"
#include "Rulesets/GameOfLifeRuleSet.h"
#include "Rulesets/HighlifeRuleSet.h"
#include "Rulesets/LifeWithoutDeathRuleSet.h"
#include "Rulesets/RuleSet.h"
#include "Rulesets/SeedsRuleSet.h"
#include "Rulesets/WireworldRuleSet.h"
#include "Tests/TestHarness.h"
#include "Tests/TestHelpers.h"
#include "Tests/TestRegistry.h"
#include <array>
#include <cstdint>
#include <vector>

static TestCounters g;

static void
testNegativeChunkMapping()
{
  testSection("SparseCellGrid: negative coordinates and chunk boundaries");
  testEqInt(g,
            static_cast<int>(SparseCellGrid::floorDivide(-1, 16)),
            -1,
            "-1 floors into chunk -1");
  testEqInt(g,
            static_cast<int>(SparseCellGrid::floorModulo(-1, 16)),
            15,
            "-1 maps to local 15");
  testEqInt(g,
            static_cast<int>(SparseCellGrid::floorDivide(-16, 16)),
            -1,
            "-16 stays in chunk -1");
  testEqInt(g,
            static_cast<int>(SparseCellGrid::floorModulo(-16, 16)),
            0,
            "-16 maps to local 0");

  SparseCellGrid grid;
  grid.setCell(CellAddress{ -1, -1 }, 0);
  grid.setCell(CellAddress{ -16, -16 }, 2);
  grid.setCell(CellAddress{ -17, -17 }, 3);
  testEqUChar(g, grid.getCell(CellAddress{ -1, -1 }), 0, "negative cell read");
  testEqUChar(
    g, grid.getCell(CellAddress{ -16, -16 }), 2, "boundary cell read");
  testEqUChar(
    g, grid.getCell(CellAddress{ -17, -17 }), 3, "next negative chunk read");
  testEqSize(
    g, grid.getAllocatedChunkCount(), 2, "negative writes allocate chunks");
}

static void
testUnboundedChunks()
{
  testSection("SparseCellGrid: no fixed chunk pool cap");
  SparseCellGrid grid;
  for (std::int64_t i = 0; i < 256; ++i) {
    grid.setCell(CellAddress{ i * 32, -i * 32 }, 0);
  }
  testTrue(g,
           grid.getAllocatedChunkCount() > 64,
           "far-apart chunks exceed the old pool capacity");
  testEqUChar(g,
              grid.getCell(CellAddress{ 255 * 32, -255 * 32 }),
              0,
              "far chunk remains addressable");
}

static void
testSparseSimulationBoundaries()
{
  testSection("SparseCellGrid: serial halo stepping");
  GameOfLifeRuleSet rules(nullptr);
  SparseCellGrid grid;
  grid.setCell(CellAddress{ 15, 0 }, 0);
  grid.setCell(CellAddress{ 15, 1 }, 0);
  grid.setCell(CellAddress{ 15, 2 }, 0);
  testTrue(g, grid.advance(rules), "cross-boundary generation advances");
  testEqUChar(
    g, grid.getCell(CellAddress{ 14, 1 }), 0, "left birth across chunk");
  testEqUChar(g, grid.getCell(CellAddress{ 15, 1 }), 0, "boundary survivor");
  testEqUChar(
    g, grid.getCell(CellAddress{ 16, 1 }), 0, "right birth across chunk");
  testEqUChar(g,
              grid.getCell(CellAddress{ 0, 1 }),
              SparseCellGrid::BackgroundState,
              "no toroidal wrapping");

  SparseCellGrid cornerGrid;
  cornerGrid.setCell(CellAddress{ 15, 15 }, 0);
  cornerGrid.setCell(CellAddress{ 16, 15 }, 0);
  cornerGrid.setCell(CellAddress{ 17, 15 }, 0);
  testTrue(g, cornerGrid.advance(rules), "corner-crossing generation advances");
  testEqUChar(g,
              cornerGrid.getCell(CellAddress{ 16, 14 }),
              0,
              "upper birth crosses chunks");
  testEqUChar(g,
              cornerGrid.getCell(CellAddress{ 16, 15 }),
              0,
              "center survivor crosses chunks");
  testEqUChar(g,
              cornerGrid.getCell(CellAddress{ 16, 16 }),
              0,
              "lower birth crosses chunks");

  SparseCellGrid first;
  SparseCellGrid second;
  first.setCell(CellAddress{ -17, 0 }, 0);
  first.setCell(CellAddress{ -16, 0 }, 0);
  first.setCell(CellAddress{ -15, 0 }, 0);
  second.setCell(CellAddress{ -17, 0 }, 0);
  second.setCell(CellAddress{ -16, 0 }, 0);
  second.setCell(CellAddress{ -15, 0 }, 0);
  first.advance(rules);
  second.advance(rules);
  const std::vector<SparseChunkRecord> firstRecords =
    first.collectChunkRecords();
  const std::vector<SparseChunkRecord> secondRecords =
    second.collectChunkRecords();
  testTrue(g,
           firstRecords.size() == secondRecords.size(),
           "serial output size deterministic");
  bool same = firstRecords.size() == secondRecords.size();
  if (same) {
    for (std::size_t i = 0; i < firstRecords.size(); ++i) {
      same = same && firstRecords[i].chunkX == secondRecords[i].chunkX &&
             firstRecords[i].chunkY == secondRecords[i].chunkY &&
             firstRecords[i].cells == secondRecords[i].cells;
    }
  }
  testTrue(g, same, "serial output bytes deterministic");
}

static void
testSparseRevisionAndBoundedVisit()
{
  testSection("SparseCellGrid: revisions and bounded chunk visits");
  GameOfLifeRuleSet rules(nullptr);
  SparseCellGrid grid;
  const std::uint64_t emptyRevision = grid.getRevision();
  testTrue(g, grid.advance(rules), "empty generation advances");
  testTrue(g,
           grid.getRevision() == emptyRevision,
           "empty generation does not change revision");

  grid.setCell(CellAddress{ 0, 0 }, 0);
  grid.setCell(CellAddress{ 1, 0 }, 0);
  grid.setCell(CellAddress{ 0, 1 }, 0);
  grid.setCell(CellAddress{ 1, 1 }, 0);
  const std::uint64_t stillLifeRevision = grid.getRevision();
  testTrue(g, grid.advance(rules), "still life generation advances");
  testTrue(g,
           grid.getRevision() == stillLifeRevision,
           "still life generation does not change revision");

  grid.setCell(CellAddress{ -17, 0 }, 0);
  grid.setCell(CellAddress{ 160, 0 }, 0);
  int visited = 0;
  bool visitedOutsideBounds = false;
  grid.visitChunksInBounds(
    ChunkAddress{ -2, 0 },
    ChunkAddress{ 0, 0 },
    [&visited, &visitedOutsideBounds](const ChunkAddress& address,
                                      const SparseCellGrid::ChunkCells& cells) {
      visited += 1;
      visitedOutsideBounds = visitedOutsideBounds || address.x < -2 ||
                             address.x > 0 || cells[0] == 255;
    });
  testEqInt(
    g, visited, 2, "bounded visit returns only allocated chunks in range");
  testTrue(g, !visitedOutsideBounds, "bounded visit excludes distant chunk");
}

static void
seedSparseRandom(SparseCellGrid* grid, int dimension, unsigned int seed)
{
  if (grid == nullptr) {
    return;
  }

  unsigned int state = seed;
  const int first = -dimension / 2;
  const int last = first + dimension;
  for (int y = first; y < last; ++y) {
    for (int x = first; x < last; ++x) {
      state = state * 1664525u + 1013904223u;
      if ((state >> 29) < 2u) {
        grid->setCell(CellAddress{ x, y }, 0);
      }
    }
  }
}

static bool
sameSparseRecords(const std::vector<SparseChunkRecord>& left,
                  const std::vector<SparseChunkRecord>& right)
{
  if (left.size() != right.size()) {
    return false;
  }
  for (std::size_t index = 0; index < left.size(); ++index) {
    if (left[index].chunkX != right[index].chunkX ||
        left[index].chunkY != right[index].chunkY ||
        left[index].cells != right[index].cells) {
      return false;
    }
  }
  return true;
}

static void
testSparseParallelDeterminism()
{
  testSection("SparseCellGrid: serial and parallel target stepping");
  GameOfLifeRuleSet rules(nullptr);
  SparseCellGrid serial;
  SparseCellGrid parallel;
  seedSparseRandom(&serial, 160, 17u);
  seedSparseRandom(&parallel, 160, 17u);

  SparseCellGrid::setCellCandidateOverrideForTesting(-1);
  SparseCellGrid::setWorkerOverrideForTesting(1);
  for (int generation = 0; generation < 6; ++generation) {
    testTrue(g, serial.advance(rules), "serial sparse generation advances");
  }
  testEqInt(g,
            static_cast<int>(serial.getLastAdvanceStats().workerCount),
            1,
            "serial override uses one worker");

  SparseCellGrid::setWorkerOverrideForTesting(4);
  for (int generation = 0; generation < 6; ++generation) {
    testTrue(g, parallel.advance(rules), "parallel sparse generation advances");
    if (generation == 0) {
      testEqInt(g,
                static_cast<int>(parallel.getLastAdvanceStats().workerCount),
                4,
                "dense sparse generation uses four workers");
    }
  }
  testTrue(g,
           parallel.getLastAdvanceStats().targetChunkCount >= 32u,
           "large region crosses parallel target threshold");
  testTrue(g,
           sameSparseRecords(serial.collectChunkRecords(),
                             parallel.collectChunkRecords()),
           "parallel sparse results are byte-identical to serial");

  SparseCellGrid small;
  small.setCell(CellAddress{ 0, 0 }, 0);
  small.setCell(CellAddress{ 1, 0 }, 0);
  small.setCell(CellAddress{ 2, 0 }, 0);
  SparseCellGrid::setWorkerOverrideForTesting(0);
  SparseCellGrid::setCellCandidateOverrideForTesting(0);
  testTrue(g, small.advance(rules), "small sparse generation advances");
  testEqInt(g,
            static_cast<int>(small.getLastAdvanceStats().workerCount),
            1,
            "small target set stays on the serial path");
}

static void
seedWideBlinkers(SparseCellGrid* grid, int count)
{
  if (grid == nullptr) {
    return;
  }
  for (int index = 0; index < count; ++index) {
    const std::int64_t x = static_cast<std::int64_t>(index) * 32;
    grid->setCell(CellAddress{ x, -1 }, 0);
    grid->setCell(CellAddress{ x, 0 }, 0);
    grid->setCell(CellAddress{ x, 1 }, 0);
  }
}

static void
testSparseCellCandidates()
{
  testSection("SparseCellGrid: adaptive cell candidates");
  GameOfLifeRuleSet life(nullptr);
  SparseCellGrid candidates;
  SparseCellGrid fullChunks;
  seedWideBlinkers(&candidates, 96);
  seedWideBlinkers(&fullChunks, 96);

  SparseCellGrid::setWorkerOverrideForTesting(1);
  SparseCellGrid::setCellCandidateOverrideForTesting(1);
  for (int generation = 0; generation < 6; ++generation) {
    testTrue(g, candidates.advance(life), "cell-candidate generation advances");
  }
  const SparseAdvanceStats candidateStats = candidates.getLastAdvanceStats();
  testTrue(g,
           candidateStats.usedCellCandidates,
           "wide sparse colony uses cell candidates");
  testTrue(g,
           candidateStats.candidateCellCount <
             candidateStats.targetChunkCount * SparseCellGrid::kChunkCellCount,
           "wide sparse colony avoids whole-chunk cell evaluation");

  SparseCellGrid::setCellCandidateOverrideForTesting(-1);
  for (int generation = 0; generation < 6; ++generation) {
    testTrue(g, fullChunks.advance(life), "full-chunk generation advances");
  }
  const SparseAdvanceStats fullChunkStats = fullChunks.getLastAdvanceStats();
  testTrue(g,
           !fullChunkStats.usedCellCandidates,
           "test override retains full-chunk evaluator");
  testTrue(g,
           sameSparseRecords(candidates.collectChunkRecords(),
                             fullChunks.collectChunkRecords()),
           "cell candidates remain byte-identical to full chunks");

  WireworldRuleSet wireworld(nullptr);
  SparseCellGrid dense;
  SparseCellGrid denseReference;
  bool denseAssigned = true;
  for (int chunkY = 0; chunkY < 9; ++chunkY) {
    for (int chunkX = 0; chunkX < 9; ++chunkX) {
      SparseChunkRecord record;
      record.chunkX = chunkX;
      record.chunkY = chunkY;
      record.cells.fill(WireworldRuleSet::CELL_CONDUCTOR);
      denseAssigned = dense.assignChunk(record) && denseAssigned;
      denseAssigned = denseReference.assignChunk(record) && denseAssigned;
    }
  }
  testTrue(g, denseAssigned, "dense conductor chunks assign");
  SparseCellGrid::setCellCandidateOverrideForTesting(0);
  SparseCellGrid::setWorkerOverrideForTesting(4);
  testTrue(g, dense.advance(wireworld), "dense generation advances");
  testTrue(g,
           dense.getLastAdvanceStats().usedCellCandidates,
           "dense conductors use cell candidates without counted heads");
  testEqSize(g,
             dense.getLastAdvanceStats().countedCellCount,
             0u,
             "Wireworld conductors are stored but not neighbor-counted");
  testEqSize(g,
             dense.getLastAdvanceStats().haloTargetCount,
             0u,
             "conductor-only targets avoid halo evaluation");
  testEqInt(g,
            static_cast<int>(dense.getLastAdvanceStats().workerCount),
            4,
            "dense conductor candidates retain bounded parallel workers");
  SparseCellGrid::setCellCandidateOverrideForTesting(-1);
  testTrue(g,
           denseReference.advance(wireworld),
           "dense reference generation advances");
  testTrue(g,
           sameSparseRecords(dense.collectChunkRecords(),
                             denseReference.collectChunkRecords()),
           "conductor candidate output matches full chunks");
}

static void
seedMixedTargetWorld(SparseCellGrid* grid)
{
  if (grid == nullptr) {
    return;
  }
  SparseChunkRecord dense;
  dense.chunkX = 0;
  dense.chunkY = 0;
  dense.cells.fill(0);
  dense.cells[0] = 2;
  grid->assignChunk(dense);
  for (int index = 0; index < 64; ++index) {
    const std::int64_t x = static_cast<std::int64_t>(index + 2) * 32;
    grid->setCell(CellAddress{ x, -1 }, 0);
    grid->setCell(CellAddress{ x, 0 }, 0);
    grid->setCell(CellAddress{ x, 1 }, 0);
  }
}

static void
testSparsePerTargetAdaptiveEvaluation()
{
  testSection("SparseCellGrid: per-target adaptive evaluation");
  GameOfLifeRuleSet rules(nullptr);
  SparseCellGrid adaptive;
  SparseCellGrid fullChunks;
  seedMixedTargetWorld(&adaptive);
  seedMixedTargetWorld(&fullChunks);

  SparseCellGrid::setWorkerOverrideForTesting(1);
  SparseCellGrid::setCellCandidateOverrideForTesting(0);
  testTrue(g, adaptive.advance(rules), "mixed adaptive generation advances");
  const SparseAdvanceStats adaptiveStats = adaptive.getLastAdvanceStats();
  testTrue(g,
           adaptiveStats.usedMixedTargets,
           "mixed world selects candidate and halo targets independently");
  testTrue(g,
           adaptiveStats.candidateTargetCount > 0u,
           "sparse targets select candidate evaluation");
  testTrue(g,
           adaptiveStats.haloTargetCount > 0u,
           "dense targets select halo evaluation");
  testTrue(g,
           adaptiveStats.countedCellCount < adaptiveStats.activeCellCount,
           "counted-state diagnostics remain distinct from stored cells");

  SparseCellGrid::setCellCandidateOverrideForTesting(-1);
  testTrue(g, fullChunks.advance(rules), "mixed full generation advances");
  testTrue(g,
           sameSparseRecords(adaptive.collectChunkRecords(),
                             fullChunks.collectChunkRecords()),
           "per-target adaptive output matches complete halo evaluation");
  SparseCellGrid::setCellCandidateOverrideForTesting(0);
}

static void
testSparseCandidateParallelDeterminism()
{
  testSection("SparseCellGrid: coarse parallel candidate evaluation");
  GameOfLifeRuleSet rules(nullptr);
  SparseCellGrid serial;
  SparseCellGrid parallel;
  const int colonyCount = 512;
  seedWideBlinkers(&serial, colonyCount);
  seedWideBlinkers(&parallel, colonyCount);

  SparseCellGrid::setCellCandidateOverrideForTesting(1);
  SparseCellGrid::setWorkerOverrideForTesting(1);
  for (int generation = 0; generation < 4; ++generation) {
    testTrue(g, serial.advance(rules), "serial candidate generation advances");
  }

  SparseCellGrid::setWorkerOverrideForTesting(4);
  for (int generation = 0; generation < 4; ++generation) {
    testTrue(
      g, parallel.advance(rules), "parallel candidate generation advances");
  }
  const SparseAdvanceStats parallelStats = parallel.getLastAdvanceStats();
  testEqInt(g,
            static_cast<int>(parallelStats.workerCount),
            4,
            "candidate evaluation uses the requested worker count");
  testTrue(g,
           parallelStats.candidateWorkRangeCount >= 4u,
           "candidate work provides at least one coarse range per worker");
  testTrue(g,
           parallelStats.candidateWorkRangeCount * 32u <
             parallelStats.targetChunkCount,
           "candidate workers claim coarse ranges instead of target chunks");
  testTrue(g,
           sameSparseRecords(serial.collectChunkRecords(),
                             parallel.collectChunkRecords()),
           "parallel candidates remain byte-identical to serial candidates");
}

static void
seedStableBlocksAndBlinker(SparseCellGrid* grid, int blockCount)
{
  if (grid == nullptr) {
    return;
  }
  for (int block = 0; block < blockCount; ++block) {
    const std::int64_t x = static_cast<std::int64_t>(block) * 64 + 4;
    const std::int64_t y = 64;
    grid->setCell(CellAddress{ x, y }, 0);
    grid->setCell(CellAddress{ x + 1, y }, 0);
    grid->setCell(CellAddress{ x, y + 1 }, 0);
    grid->setCell(CellAddress{ x + 1, y + 1 }, 0);
  }
  grid->setCell(CellAddress{ 0, -1 }, 0);
  grid->setCell(CellAddress{ 0, 0 }, 0);
  grid->setCell(CellAddress{ 0, 1 }, 0);
}

static void
testSparseChangedFrontier()
{
  testSection("SparseCellGrid: retained changed-region frontier");
  GameOfLifeRuleSet life(nullptr);
  SparseCellGrid stable;
  stable.setCell(CellAddress{ 3, 3 }, 0);
  stable.setCell(CellAddress{ 4, 3 }, 0);
  stable.setCell(CellAddress{ 3, 4 }, 0);
  stable.setCell(CellAddress{ 4, 4 }, 0);

  SparseCellGrid::setCellCandidateOverrideForTesting(0);
  testTrue(g, stable.advance(life), "still life frontier settles");
  testTrue(g,
           stable.getLastAdvanceStats().usedChangedFrontier,
           "small edited region uses the frontier evaluator");
  const std::uint64_t stableRevision = stable.getRevision();
  testTrue(g, stable.advance(life), "settled frontier advances");
  testTrue(g,
           stable.getLastAdvanceStats().usedChangedFrontier,
           "settled world stays on the frontier path");
  testEqSize(g,
             stable.getLastAdvanceStats().frontierTargetCount,
             0u,
             "settled world evaluates zero target chunks");
  testEqSize(g,
             static_cast<std::size_t>(stable.getRevision()),
             static_cast<std::size_t>(stableRevision),
             "settled frontier keeps its revision");

  SparseCellGrid optimized;
  SparseCellGrid reference;
  seedStableBlocksAndBlinker(&optimized, 128);
  seedStableBlocksAndBlinker(&reference, 128);
  bool observedLocalFrontier = false;
  for (int generation = 0; generation < 6; ++generation) {
    SparseCellGrid::setCellCandidateOverrideForTesting(0);
    testTrue(g, optimized.advance(life), "frontier generation advances");
    if (generation > 0 && optimized.getLastAdvanceStats().usedChangedFrontier) {
      observedLocalFrontier = true;
      testTrue(g,
               optimized.getLastAdvanceStats().frontierTargetCount <
                 optimized.getLastAdvanceStats().activeChunkCount,
               "localized oscillator evaluates less than the static world");
    }
    SparseCellGrid::setCellCandidateOverrideForTesting(-1);
    testTrue(g, reference.advance(life), "full reference generation advances");
  }
  testTrue(g,
           observedLocalFrontier,
           "localized activity enters the changed-region frontier");
  testTrue(g,
           sameSparseRecords(optimized.collectChunkRecords(),
                             reference.collectChunkRecords()),
           "frontier output matches complete full-chunk stepping");

  SparseCellGrid::setCellCandidateOverrideForTesting(0);
  stable.setCell(CellAddress{ 20, 20 }, 0);
  testTrue(g, stable.advance(life), "edited settled world advances");
  testTrue(g,
           stable.getLastAdvanceStats().usedChangedFrontier &&
             stable.getLastAdvanceStats().frontierTargetCount > 0u,
           "editing repopulates the local frontier");

  SparseCellGrid ruleChange;
  ruleChange.setCell(CellAddress{ 0, 0 }, 0);
  ruleChange.setCell(CellAddress{ 1, 0 }, 0);
  ruleChange.setCell(CellAddress{ 0, 1 }, 0);
  ruleChange.setCell(CellAddress{ 1, 1 }, 0);
  testTrue(g, ruleChange.advance(life), "life block settles");
  SeedsRuleSet seeds(nullptr);
  const std::uint64_t lifeRevision = ruleChange.getRevision();
  testTrue(g, ruleChange.advance(seeds), "ruleset change advances");
  testTrue(g,
           ruleChange.getLastAdvanceStats().usedChangedFrontier,
           "ruleset change invalidates the stable region locally");
  testTrue(g,
           ruleChange.getRevision() > lifeRevision,
           "ruleset change produces a new generation");
}

static void
testSparseCandidateScratchReuse()
{
  testSection("SparseCellGrid: retained candidate scratch storage");
  const int colonyCount = 64;
  GameOfLifeRuleSet rules(nullptr);
  SparseCellGrid grid;
  seedWideBlinkers(&grid, colonyCount);

  SparseCellGrid::setWorkerOverrideForTesting(1);
  SparseCellGrid::setCellCandidateOverrideForTesting(1);
  testTrue(g, grid.advance(rules), "first candidate generation advances");
  const SparseAdvanceStats firstStats = grid.getLastAdvanceStats();
  const std::size_t firstIndexCapacity =
    grid.getCandidateIndexCapacityForTesting();
  const std::size_t firstScratchCapacity =
    grid.getCandidateScratchCapacityForTesting();
  testEqSize(g,
             firstStats.targetChunkCount,
             static_cast<std::size_t>(colonyCount) * 4u,
             "wide blinkers use exact candidate chunks");
  testEqSize(g,
             firstStats.candidateCellCount,
             static_cast<std::size_t>(colonyCount) * 15u,
             "wide blinkers use exact candidate cells");
  testTrue(g,
           firstIndexCapacity >= firstStats.targetChunkCount,
           "candidate flat index owns retained capacity");
  testTrue(g,
           firstScratchCapacity >= firstStats.targetChunkCount,
           "candidate scratch vector owns retained capacity");

  testTrue(g, grid.advance(rules), "second candidate generation advances");
  testEqSize(g,
             grid.getCandidateIndexCapacityForTesting(),
             firstIndexCapacity,
             "candidate flat-index capacity is reused");
  testEqSize(g,
             grid.getCandidateScratchCapacityForTesting(),
             firstScratchCapacity,
             "candidate scratch capacity is reused");
  testEqSize(g,
             grid.getLastAdvanceStats().candidateCellCount,
             static_cast<std::size_t>(colonyCount) * 15u,
             "reused scratch is reset before the next generation");
}

static void
testSparseCandidateFlatIndex()
{
  testSection("SparseCellGrid: flat candidate index generations");
  GameOfLifeRuleSet rules(nullptr);
  SparseCellGrid candidates;
  SparseCellGrid fullChunks;
  seedWideBlinkers(&candidates, 128);

  SparseCellGrid::setWorkerOverrideForTesting(1);
  SparseCellGrid::setCellCandidateOverrideForTesting(1);
  testTrue(g, candidates.advance(rules), "large candidate generation advances");
  const std::size_t retainedCapacity =
    candidates.getCandidateIndexCapacityForTesting();

  candidates.clear();
  const std::int64_t x = -1024;
  for (std::int64_t y = -1; y <= 1; ++y) {
    candidates.setCell(CellAddress{ x, y }, 0);
    fullChunks.setCell(CellAddress{ x, y }, 0);
  }
  testTrue(g,
           candidates.advance(rules),
           "new negative-coordinate index generation advances");
  testEqSize(g,
             candidates.getLastAdvanceStats().targetChunkCount,
             4u,
             "old generation slots do not remain active");
  testEqSize(g,
             candidates.getLastAdvanceStats().candidateCellCount,
             15u,
             "flat index deduplicates negative-coordinate candidates");
  testEqSize(g,
             candidates.getCandidateIndexCapacityForTesting(),
             retainedCapacity,
             "flat index retains its high-water capacity");

  SparseCellGrid::setCellCandidateOverrideForTesting(-1);
  testTrue(g, fullChunks.advance(rules), "full-chunk reference advances");
  testTrue(g,
           sameSparseRecords(candidates.collectChunkRecords(),
                             fullChunks.collectChunkRecords()),
           "flat-index generation matches full chunks");
}

static void
testSparseChunkNodeReuse()
{
  testSection("SparseCellGrid: retained generation chunk nodes");
  GameOfLifeRuleSet rules(nullptr);
  SparseCellGrid candidates;
  candidates.setCell(CellAddress{ 4, 3 }, 0);
  candidates.setCell(CellAddress{ 4, 4 }, 0);
  candidates.setCell(CellAddress{ 4, 5 }, 0);

  SparseCellGrid::setWorkerOverrideForTesting(1);
  SparseCellGrid::setCellCandidateOverrideForTesting(1);
  testTrue(g, candidates.advance(rules), "first candidate step advances");
  testEqSize(g,
             candidates.getLastAdvanceStats().allocatedChunkNodeCount,
             1u,
             "first candidate output allocates one chunk node");
  testTrue(g, candidates.advance(rules), "second candidate step advances");
  testEqSize(g,
             candidates.getLastAdvanceStats().allocatedChunkNodeCount,
             0u,
             "steady candidate output allocates no chunk nodes");
  testEqSize(g,
             candidates.getLastAdvanceStats().reusedChunkNodeCount,
             1u,
             "steady candidate output reuses an inactive chunk node");
  testEqSize(g,
             candidates.getLastAdvanceStats().retainedChunkNodeCount,
             1u,
             "candidate path retains the inactive generation node");

  SparseCellGrid stable;
  stable.setCell(CellAddress{ -2, -2 }, 0);
  stable.setCell(CellAddress{ -1, -2 }, 0);
  stable.setCell(CellAddress{ -2, -1 }, 0);
  stable.setCell(CellAddress{ -1, -1 }, 0);
  const std::uint64_t stableRevision = stable.getRevision();
  testTrue(g, stable.advance(rules), "unchanged candidate step advances");
  testEqSize(g,
             static_cast<std::size_t>(stable.getRevision()),
             static_cast<std::size_t>(stableRevision),
             "unchanged generation keeps its revision");
  testTrue(g, stable.advance(rules), "reused unchanged step advances");
  testEqSize(g,
             stable.getLastAdvanceStats().allocatedChunkNodeCount,
             0u,
             "unchanged steady generation allocates no chunk nodes");
  testEqSize(g,
             stable.getLastAdvanceStats().reusedChunkNodeCount,
             1u,
             "unchanged steady generation reuses its retained node");

  SparseCellGrid dense;
  dense.setCell(CellAddress{ 7, 7 }, 0);
  dense.setCell(CellAddress{ 8, 7 }, 0);
  dense.setCell(CellAddress{ 7, 8 }, 0);
  dense.setCell(CellAddress{ 8, 8 }, 0);
  SparseCellGrid::setCellCandidateOverrideForTesting(-1);
  testTrue(g, dense.advance(rules), "first full-chunk step advances");
  testTrue(g, dense.advance(rules), "second full-chunk step advances");
  testEqSize(g,
             dense.getLastAdvanceStats().allocatedChunkNodeCount,
             0u,
             "steady full-chunk output allocates no chunk nodes");
  testEqSize(g,
             dense.getLastAdvanceStats().reusedChunkNodeCount,
             1u,
             "full-chunk output uses the shared node recycler");
}

static void
seedSparseRulePattern(SparseCellGrid* grid)
{
  if (grid == nullptr) {
    return;
  }
  grid->setCell(CellAddress{ -32, -1 }, 0);
  grid->setCell(CellAddress{ -32, 0 }, 0);
  grid->setCell(CellAddress{ -32, 1 }, 0);
  grid->setCell(CellAddress{ 0, 0 }, 0);
}

static void
testSparseCellCandidateRuleEquivalence()
{
  testSection("SparseCellGrid: candidate rule equivalence");
  GameOfLifeRuleSet life(nullptr);
  SeedsRuleSet seeds(nullptr);
  BrainsBrainRuleSet brains(nullptr);
  HighlifeRuleSet highlife(nullptr);
  DayAndNightRuleSet dayAndNight(nullptr);
  LifeWithoutDeathRuleSet lifeWithoutDeath(nullptr);
  WireworldRuleSet wireworld(nullptr);
  RuleSet* ruleSets[] = { &life,     &seeds,       &brains,
                          &highlife, &dayAndNight, &lifeWithoutDeath,
                          &wireworld };

  for (RuleSet* rules : ruleSets) {
    SparseCellGrid candidates;
    SparseCellGrid fullChunks;
    seedSparseRulePattern(&candidates);
    seedSparseRulePattern(&fullChunks);
    if (rules == &brains) {
      candidates.setCell(CellAddress{ 1, 0 }, 2);
      fullChunks.setCell(CellAddress{ 1, 0 }, 2);
    }
    if (rules == &wireworld) {
      candidates.setCell(CellAddress{ 1, 0 }, WireworldRuleSet::CELL_TAIL);
      candidates.setCell(CellAddress{ 2, 0 }, WireworldRuleSet::CELL_CONDUCTOR);
      fullChunks.setCell(CellAddress{ 1, 0 }, WireworldRuleSet::CELL_TAIL);
      fullChunks.setCell(CellAddress{ 2, 0 }, WireworldRuleSet::CELL_CONDUCTOR);
    }

    SparseCellGrid::setCellCandidateOverrideForTesting(1);
    for (int generation = 0; generation < 4; ++generation) {
      testTrue(
        g, candidates.advance(*rules), "candidate rules generation advances");
    }
    SparseCellGrid::setCellCandidateOverrideForTesting(-1);
    for (int generation = 0; generation < 4; ++generation) {
      testTrue(g, fullChunks.advance(*rules), "full rules generation advances");
    }
    testTrue(g,
             sameSparseRecords(candidates.collectChunkRecords(),
                               fullChunks.collectChunkRecords()),
             "candidate rules match full chunks");
  }
  SparseCellGrid::setCellCandidateOverrideForTesting(0);
}

static void
testMultiStateTransitions()
{
  testSection("SparseCellGrid: multi-state transitions");
  WireworldRuleSet rules(nullptr);
  SparseCellGrid grid;
  grid.setCell(CellAddress{ 0, 0 }, WireworldRuleSet::CELL_HEAD);
  grid.setCell(CellAddress{ 1, 0 }, WireworldRuleSet::CELL_TAIL);
  grid.setCell(CellAddress{ 2, 0 }, WireworldRuleSet::CELL_CONDUCTOR);
  grid.advance(rules);
  testEqUChar(g,
              grid.getCell(CellAddress{ 0, 0 }),
              WireworldRuleSet::CELL_TAIL,
              "head becomes tail");
  testEqUChar(g,
              grid.getCell(CellAddress{ 1, 0 }),
              WireworldRuleSet::CELL_CONDUCTOR,
              "tail becomes conductor");
  testEqUChar(g,
              grid.getCell(CellAddress{ 2, 0 }),
              WireworldRuleSet::CELL_CONDUCTOR,
              "conductor preserves state");

  BrainsBrainRuleSet brainRules(nullptr);
  SparseCellGrid brain;
  brain.setCell(CellAddress{ 0, 0 }, 0);
  brain.setCell(CellAddress{ 1, 0 }, 2);
  brain.advance(brainRules);
  testEqUChar(
    g, brain.getCell(CellAddress{ 0, 0 }), 2, "brain alive becomes dying");
  testEqUChar(
    g, brain.getCell(CellAddress{ 1, 0 }), 1, "brain dying becomes empty");
}

static void
testBoundedCanvasView()
{
  testSection("CanvasView: visible region, snapping, and fade");
  NullRenderWindow window(64, 64);
  EnvVars env;
  env.setVar("WinX", 64);
  env.setVar("WinY", 64);
  Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
  SparseCellGrid grid;
  CanvasView view(4, 4, &grid, &window, &camera, nullptr);
  view.rebuildDefaultPalette();
  grid.setCell(CellAddress{ 0, 0 }, 0);
  view.rebuildTargetsFromGrid();
  view.snapVisualToTargets();
  testTrue(g,
           view.getVisibleCell(3, 2) == CellAddress{ 0, 0 },
           "view samples the centered origin");
  const unsigned char* pixels = view.getDisplayTexBuffer();
  testEqUChar(
    g, pixels[(2 * 6 + 3) * 3], 0, "visible alive cell is staged black");

  grid.setCell(CellAddress{ 0, 0 }, SparseCellGrid::BackgroundState);
  view.rebuildTargetsFromGrid();
  testTrue(g, view.isFadeActive(), "target changes start visible-area fade");
  view.setFadeSpeed(0.0f);
  testTrue(g, !view.isFadeActive(), "zero fade speed snaps immediately");
  grid.setCell(CellAddress{ 2, 0 }, 0);
  camera.SetPosition(glm::vec2(32.0f, 0.0f));
  view.syncVisibleRegion();
  testTrue(g,
           view.getVisibleCell(3, 2) == CellAddress{ 2, 0 },
           "camera movement reveals a new world region");
  testTrue(
    g, !view.isFadeActive(), "newly revealed cells snap instead of fading");
}

static void
testAdaptiveOverviewAndRevisionGate()
{
  testSection("CanvasView: adaptive overview and revision-gated uploads");
  NullRenderWindow window(1280, 720);
  EnvVars env;
  env.setVar("WinX", 1280);
  env.setVar("WinY", 720);
  Camera camera(glm::vec2(0.0f, 0.0f), 0.1f, &env);
  MockBackend mock;
  mock.Initialize();
  Renderer renderer(&window, &env, &camera, &mock, false);
  SparseCellGrid grid;
  grid.setCell(CellAddress{ 0, 0 }, 0);
  CanvasView view(80, 60, &grid, &window, &camera, &renderer);
  view.rebuildDefaultPalette();
  view.rebuildTargetsFromGrid();

  testEqInt(g, view.getVisibleCellWidth(), 802, "far view source width");
  testEqInt(g, view.getVisibleCellHeight(), 452, "far view source height");
  testEqInt(
    g, view.getViewWidth(), 320, "far view uses four-pixel overview width");
  testEqInt(
    g, view.getViewHeight(), 180, "far view uses four-pixel overview height");
  testEqInt(
    g, view.getTextureWidth(), 320, "overview texture width is bounded");
  testEqInt(
    g, view.getTextureHeight(), 180, "overview texture height is bounded");

  const CellAddress firstCell = view.getVisibleFirstCell();
  const int outputX = static_cast<int>((0 - firstCell.x) * view.getViewWidth() /
                                       view.getVisibleCellWidth());
  const int outputY = static_cast<int>(
    (firstCell.y - 0) * view.getViewHeight() / view.getVisibleCellHeight());
  const unsigned char* pixels = view.getDisplayTexBuffer();
  const int pixelIndex = (outputY * view.getTextureWidth() + outputX) * 3;
  testTrue(g,
           pixels[pixelIndex] > 0 && pixels[pixelIndex] < 255,
           "overview pixel contains density-weighted live color");

  view.AppendCommands(&renderer);
  mock.SubmitCommandQueue();
  mock.ClearCommandQueue();
  view.rebuildTargetsFromGrid();
  view.AppendCommands(&renderer);
  mock.SubmitCommandQueue();
  bool unchangedFrameUpdatedTexture = false;
  for (std::size_t i = 0; i < mock.getLastSubmittedCount(); ++i) {
    unchangedFrameUpdatedTexture =
      unchangedFrameUpdatedTexture ||
      mock.getLastSubmitted(i).commandType == CommandType::UpdateTexture;
  }
  testTrue(g,
           !unchangedFrameUpdatedTexture,
           "unchanged view does not upload a texture");

  grid.setCell(CellAddress{ 1, 0 }, 0);
  view.rebuildTargetsFromGrid();
  view.setFadeSpeed(0.0f);
  mock.ClearCommandQueue();
  view.AppendCommands(&renderer);
  mock.SubmitCommandQueue();
  bool foundOverviewUpload = false;
  bool usesTextureStride = false;
  for (std::size_t i = 0; i < mock.getLastSubmittedCount(); ++i) {
    const RenderCommand& command = mock.getLastSubmitted(i);
    if (command.commandType == CommandType::UpdateTexture) {
      foundOverviewUpload = true;
      usesTextureStride =
        command.updateTexture.srcRowStride == view.getTextureWidth();
    }
  }
  testTrue(g, foundOverviewUpload, "changed overview emits one texture update");
  testTrue(
    g, usesTextureStride, "overview update keeps allocated texture stride");

  camera.SetZoom(1.0f);
  view.rebuildTargetsFromGrid();
  testEqInt(g, view.getViewWidth(), 82, "near view restores exact cell texels");
  testEqInt(g, view.getViewHeight(), 47, "near view restores exact cell rows");
  testEqInt(g,
            view.getTextureWidth(),
            320,
            "near view retains allocation without expanding active work");
  testEqInt(g,
            view.getTextureHeight(),
            180,
            "near view retains allocation without expanding active rows");
}

static void
testCanvasViewUsesWorldCellQuad()
{
  testSection("CanvasView: world-space cell-aligned presentation");
  NullRenderWindow window(64, 64);
  EnvVars env;
  env.setVar("WinX", 64);
  env.setVar("WinY", 64);
  Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
  MockBackend mock;
  mock.Initialize();
  Renderer renderer(&window, &env, &camera, &mock, false);
  SparseCellGrid grid;
  CanvasView view(4, 4, &grid, &window, &camera, &renderer);
  view.rebuildTargetsFromGrid();

  bool foundCanvasTexture = false;
  bool usesNearest = true;
  for (std::size_t i = 0; i < mock.getCreateCount(); ++i) {
    const MockBackend::CreateRecord& record = mock.getCreate(i);
    if (record.kind == MockBackend::CreateRecord::Kind::TextureData) {
      foundCanvasTexture = true;
      usesNearest = usesNearest && record.filter == TextureFilter::Nearest;
    }
  }
  testTrue(g, foundCanvasTexture, "CanvasView enrolls a display texture");
  testTrue(g, usesNearest, "CanvasView keeps cell texture edges sharp");
  testTrue(g,
           view.getVisual().getSpace() == PrimitiveSpace::World,
           "CanvasView uses the camera world space");
  SpritePrimitive* sprite = view.getVisual().getSprite(0);
  testTrue(g, sprite != nullptr, "CanvasView owns one display sprite");
  if (sprite != nullptr) {
    testTrue(g,
             sprite->rect.x == -56.0f && sprite->rect.y == -56.0f &&
               sprite->rect.w == 96.0f && sprite->rect.h == 96.0f,
             "display sprite follows cell boundaries");
    testTrue(g,
             sprite->v0 == 1.0f && sprite->v1 == 0.0f,
             "display sprite keeps world-up rows upright");
  }
}

static void
testCursorUsesCellBounds()
{
  testSection("CanvasView: cursor uses the same cell bounds");
  NullRenderWindow window(64, 64);
  EnvVars env;
  env.setVar("WinX", 64);
  env.setVar("WinY", 64);
  Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
  MockBackend mock;
  mock.Initialize();
  Renderer renderer(&window, &env, &camera, &mock, false);
  Cursor cursor;
  cursor.init(&renderer, &window, &camera);
  cursor.setFromCell(0, 0);

  ShapePrimitive* outline = cursor.getShape(0);
  testTrue(g, outline != nullptr, "cursor creates a cell outline");
  if (outline != nullptr) {
    testTrue(g,
             outline->rect.x == -8.0f && outline->rect.y == -8.0f &&
               outline->rect.w == 16.0f && outline->rect.h == 16.0f,
             "cursor outline is centered on its selected cell");
  }
}

static int
runCanvasInfCase(void (*testFunction)())
{
  g.failures = 0;
  SparseCellGrid::setWorkerOverrideForTesting(0);
  SparseCellGrid::setCellCandidateOverrideForTesting(0);
  SparseCellGrid::setChunkNodeReuseOverrideForTesting(true);
  testFunction();
  SparseCellGrid::setWorkerOverrideForTesting(0);
  SparseCellGrid::setCellCandidateOverrideForTesting(0);
  SparseCellGrid::setChunkNodeReuseOverrideForTesting(true);
  return g.failures;
}

void
registerCanvasInfTests(IllumoTestRegistry& registry)
{
  registry.add("Illumo.CanvasInf.NegativeChunkMapping",
               []() { return runCanvasInfCase(testNegativeChunkMapping); });
  registry.add("Illumo.CanvasInf.UnboundedChunks",
               []() { return runCanvasInfCase(testUnboundedChunks); });
  registry.add("Illumo.CanvasInf.SparseSimulationBoundaries", []() {
    return runCanvasInfCase(testSparseSimulationBoundaries);
  });
  registry.add("Illumo.CanvasInf.SparseRevisionAndBoundedVisit", []() {
    return runCanvasInfCase(testSparseRevisionAndBoundedVisit);
  });
  registry.add("Illumo.CanvasInf.SparseParallelDeterminism", []() {
    return runCanvasInfCase(testSparseParallelDeterminism);
  });
  registry.add("Illumo.CanvasInf.SparseCellCandidates",
               []() { return runCanvasInfCase(testSparseCellCandidates); });
  registry.add("Illumo.CanvasInf.SparsePerTargetAdaptiveEvaluation", []() {
    return runCanvasInfCase(testSparsePerTargetAdaptiveEvaluation);
  });
  registry.add("Illumo.CanvasInf.SparseCandidateParallelDeterminism", []() {
    return runCanvasInfCase(testSparseCandidateParallelDeterminism);
  });
  registry.add("Illumo.CanvasInf.SparseChangedFrontier",
               []() { return runCanvasInfCase(testSparseChangedFrontier); });
  registry.add("Illumo.CanvasInf.SparseCandidateScratchReuse", []() {
    return runCanvasInfCase(testSparseCandidateScratchReuse);
  });
  registry.add("Illumo.CanvasInf.SparseCandidateFlatIndex",
               []() { return runCanvasInfCase(testSparseCandidateFlatIndex); });
  registry.add("Illumo.CanvasInf.SparseChunkNodeReuse",
               []() { return runCanvasInfCase(testSparseChunkNodeReuse); });
  registry.add("Illumo.CanvasInf.SparseCellCandidateRuleEquivalence", []() {
    return runCanvasInfCase(testSparseCellCandidateRuleEquivalence);
  });
  registry.add("Illumo.CanvasInf.MultiStateTransitions",
               []() { return runCanvasInfCase(testMultiStateTransitions); });
  registry.add("Illumo.CanvasInf.BoundedView",
               []() { return runCanvasInfCase(testBoundedCanvasView); });
  registry.add("Illumo.CanvasInf.AdaptiveOverviewAndRevisionGate", []() {
    return runCanvasInfCase(testAdaptiveOverviewAndRevisionGate);
  });
  registry.add("Illumo.CanvasInf.WorldCellPresentation", []() {
    return runCanvasInfCase(testCanvasViewUsesWorldCellQuad);
  });
  registry.add("Illumo.CanvasInf.CursorCellAlignment",
               []() { return runCanvasInfCase(testCursorUsesCellBounds); });
}
