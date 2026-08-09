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
#include "Rulesets/Rule90RuleSet.h"
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
  testEqInt(g,
            static_cast<int>(parallelStats.candidatePreparationWorkerCount),
            4,
            "candidate preparation uses the requested worker count");
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
seedChunkBoundaryBlocks(SparseCellGrid* grid, int count)
{
  if (grid == nullptr) {
    return;
  }
  for (int index = 0; index < count; ++index) {
    const std::int64_t boundaryX = static_cast<std::int64_t>(index) * 64;
    const std::int64_t boundaryY = index % 2 == 0 ? 0 : -16;
    grid->setCell(CellAddress{ boundaryX - 1, boundaryY - 1 }, 0);
    grid->setCell(CellAddress{ boundaryX, boundaryY - 1 }, 0);
    grid->setCell(CellAddress{ boundaryX - 1, boundaryY }, 0);
    grid->setCell(CellAddress{ boundaryX, boundaryY }, 0);
  }
}

static void
testSparseCandidatePreparation()
{
  testSection("SparseCellGrid: parallel candidate preparation");
  GameOfLifeRuleSet rules(nullptr);
  SparseCellGrid serial;
  SparseCellGrid parallel;
  SparseCellGrid fullChunks;
  seedChunkBoundaryBlocks(&serial, 128);
  seedChunkBoundaryBlocks(&parallel, 128);
  seedChunkBoundaryBlocks(&fullChunks, 128);

  SparseCellGrid::setCellCandidateOverrideForTesting(1);
  SparseCellGrid::setWorkerOverrideForTesting(1);
  for (int generation = 0; generation < 3; ++generation) {
    testTrue(g, serial.advance(rules), "serial preparation advances");
  }
  testEqInt(g,
            static_cast<int>(
              serial.getLastAdvanceStats().candidatePreparationWorkerCount),
            1,
            "serial preparation remains direct");

  SparseCellGrid::setWorkerOverrideForTesting(4);
  for (int generation = 0; generation < 3; ++generation) {
    testTrue(g, parallel.advance(rules), "parallel preparation advances");
  }
  testEqInt(g,
            static_cast<int>(
              parallel.getLastAdvanceStats().candidatePreparationWorkerCount),
            4,
            "target-centric preparation uses four workers");

  SparseCellGrid::setCellCandidateOverrideForTesting(-1);
  SparseCellGrid::setWorkerOverrideForTesting(1);
  for (int generation = 0; generation < 3; ++generation) {
    testTrue(g, fullChunks.advance(rules), "full-halo reference advances");
  }
  testTrue(g,
           sameSparseRecords(serial.collectChunkRecords(),
                             parallel.collectChunkRecords()),
           "parallel preparation matches serial preparation at boundaries");
  testTrue(g,
           sameSparseRecords(serial.collectChunkRecords(),
                             fullChunks.collectChunkRecords()),
           "candidate preparation matches full halos at boundaries");
  SparseCellGrid::setCellCandidateOverrideForTesting(0);
  SparseCellGrid::setWorkerOverrideForTesting(0);
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
           stable.getLastAdvanceStats().usedChangedFrontier ||
             stable.getLastAdvanceStats().frontierEstimatedWork >
               stable.getLastAdvanceStats().completeEstimatedWork,
           "small edited region is evaluated or cost-rejected");
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
           (stable.getLastAdvanceStats().usedChangedFrontier &&
            stable.getLastAdvanceStats().frontierTargetCount > 0u) ||
             stable.getLastAdvanceStats().frontierEstimatedWork >
               stable.getLastAdvanceStats().completeEstimatedWork,
           "editing evaluates or cost-rejects the local frontier");

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
           ruleChange.getLastAdvanceStats().usedChangedFrontier ||
             ruleChange.getLastAdvanceStats().frontierEstimatedWork >
               ruleChange.getLastAdvanceStats().completeEstimatedWork,
           "ruleset change evaluates or cost-rejects the invalidated region");
  testTrue(g,
           ruleChange.getRevision() > lifeRevision,
           "ruleset change produces a new generation");
}

static void
seedAdaptiveFrontierWorld(SparseCellGrid* grid,
                          int stableBlockCount,
                          int blinkerCount)
{
  if (grid == nullptr) {
    return;
  }
  for (int block = 0; block < stableBlockCount; ++block) {
    const std::int64_t x = static_cast<std::int64_t>(block) * 64 + 4;
    const std::int64_t y = 64;
    grid->setCell(CellAddress{ x, y }, 0);
    grid->setCell(CellAddress{ x + 1, y }, 0);
    grid->setCell(CellAddress{ x, y + 1 }, 0);
    grid->setCell(CellAddress{ x + 1, y + 1 }, 0);
  }
  for (int blinker = 0; blinker < blinkerCount; ++blinker) {
    const std::int64_t x = static_cast<std::int64_t>(blinker) * 64 + 15;
    grid->setCell(CellAddress{ x, -1 }, 0);
    grid->setCell(CellAddress{ x, 0 }, 0);
    grid->setCell(CellAddress{ x, 1 }, 0);
  }
}

static void
testSparseAdaptiveFrontierCost()
{
  testSection("SparseCellGrid: adaptive frontier cost and candidates");
  GameOfLifeRuleSet life(nullptr);
  SparseCellGrid optimized;
  SparseCellGrid reference;
  seedAdaptiveFrontierWorld(&optimized, 1024, 8);
  seedAdaptiveFrontierWorld(&reference, 1024, 8);

  SparseCellGrid::setCellCandidateOverrideForTesting(0);
  testTrue(g, optimized.advance(life), "adaptive baseline generation advances");
  SparseCellGrid::setCellCandidateOverrideForTesting(1);
  testTrue(g, reference.advance(life), "complete baseline generation advances");

  SparseCellGrid::setCellCandidateOverrideForTesting(0);
  testTrue(g, optimized.advance(life), "wide local frontier advances");
  const SparseAdvanceStats frontierStats = optimized.getLastAdvanceStats();
  testTrue(g,
           frontierStats.usedChangedFrontier,
           "cost model keeps the localized frontier");
  testTrue(g,
           frontierStats.frontierTargetCount > 64u,
           "localized frontier crosses the former 64-target cliff");
  testTrue(g,
           frontierStats.candidateTargetCount > 0u &&
             frontierStats.usedCellCandidates,
           "sparse frontier evaluates candidate masks");
  testTrue(
    g,
    frontierStats.frontierEstimatedWork <= frontierStats.completeEstimatedWork,
    "selected frontier has no more estimated work than complete stepping");
  testTrue(g,
           frontierStats.frontierSourceChunkCount <
             frontierStats.activeChunkCount,
           "frontier candidate preparation visits only local source chunks");

  SparseCellGrid::setCellCandidateOverrideForTesting(1);
  testTrue(
    g, reference.advance(life), "complete comparison generation advances");
  testTrue(g,
           sameSparseRecords(optimized.collectChunkRecords(),
                             reference.collectChunkRecords()),
           "adaptive candidate frontier matches complete candidate stepping");

  SparseCellGrid broad;
  seedWideBlinkers(&broad, 128);
  SparseCellGrid::setCellCandidateOverrideForTesting(0);
  testTrue(g, broad.advance(life), "broad-change generation advances");
  const SparseAdvanceStats broadStats = broad.getLastAdvanceStats();
  testTrue(g,
           !broadStats.usedChangedFrontier,
           "cost model rejects a broad separated frontier");
  testTrue(g,
           broadStats.frontierEstimatedWork > broadStats.completeEstimatedWork,
           "broad frontier rejection is explained by estimated work");
}

static void
testSparseCachedStatistics()
{
  testSection("SparseCellGrid: transactional cached statistics");
  Rule90RuleSet identity(nullptr);
  SparseCellGrid grid;
  grid.setCell(CellAddress{ 0, 0 }, 0);
  grid.setCell(CellAddress{ 1, 0 }, 3);
  grid.setCell(CellAddress{ 16, 0 }, 0);

  testTrue(g, grid.advance(identity), "initial identity generation advances");
  const SparseAdvanceStats initial = grid.getLastAdvanceStats();
  testEqSize(g, initial.activeChunkCount, 2u, "two source chunks are cached");
  testEqSize(g, initial.activeCellCount, 3u, "stored cells are cached");
  testEqSize(g, initial.countedCellCount, 2u, "counted cells are cached");
  testEqSize(g,
             initial.candidatePreferredChunkCount,
             2u,
             "candidate-preferred chunks are cached");

  testTrue(g, grid.advance(identity), "settled identity generation advances");
  const SparseAdvanceStats settled = grid.getLastAdvanceStats();
  testTrue(g,
           settled.usedChangedFrontier && settled.frontierTargetCount == 0u,
           "settled generation takes the zero-target frontier path");
  testEqSize(g,
             settled.activeCellCount,
             3u,
             "settled generation reports cached stored cells");
  testEqSize(g,
             settled.countedCellCount,
             2u,
             "settled generation reports cached counted cells");

  grid.setCell(CellAddress{ 0, 0 }, 3);
  grid.setCell(CellAddress{ 16, 0 }, SparseCellGrid::BackgroundState);
  testTrue(g, grid.advance(identity), "edited identity generation advances");
  const SparseAdvanceStats edited = grid.getLastAdvanceStats();
  testEqSize(g, edited.activeChunkCount, 1u, "empty chunk leaves the cache");
  testEqSize(g, edited.activeCellCount, 2u, "state edits update stored cells");
  testEqSize(
    g, edited.countedCellCount, 0u, "state edits update counted cells");
  testEqSize(g,
             edited.candidatePreferredChunkCount,
             1u,
             "state edits update candidate preference");

  SparseChunkRecord replacement;
  replacement.chunkX = 0;
  replacement.chunkY = 0;
  replacement.cells.fill(SparseCellGrid::BackgroundState);
  replacement.cells[0] = 0;
  replacement.cells[1] = 2;
  testTrue(g, grid.assignChunk(replacement), "existing chunk is replaced");

  SparseChunkRecord denseCounted;
  denseCounted.chunkX = 3;
  denseCounted.chunkY = 0;
  denseCounted.cells.fill(SparseCellGrid::BackgroundState);
  for (std::size_t index = 0u; index < 48u; ++index) {
    denseCounted.cells[index] = 0;
  }
  testTrue(
    g, grid.assignChunk(denseCounted), "dense counted chunk is assigned");
  testTrue(g, grid.advance(identity), "assigned chunks advance");
  const SparseAdvanceStats assigned = grid.getLastAdvanceStats();
  testEqSize(
    g, assigned.activeCellCount, 50u, "assignment updates stored cache");
  testEqSize(
    g, assigned.countedCellCount, 49u, "assignment updates counted cache");
  testEqSize(g,
             assigned.candidatePreferredChunkCount,
             1u,
             "threshold-equal chunk is not candidate-preferred");

  grid.setCell(CellAddress{ 48, 0 }, 2);
  testTrue(g, grid.advance(identity), "threshold crossing advances");
  testEqSize(g,
             grid.getLastAdvanceStats().candidatePreferredChunkCount,
             2u,
             "counted-cell edit updates cached candidate preference");

  SparseCellGrid other;
  other.setCell(CellAddress{ -32, 0 }, 0);
  testTrue(g, other.advance(identity), "swap peer is synchronized");
  grid.swap(other);
  testTrue(g, grid.advance(identity), "swapped small grid advances");
  testEqSize(g,
             grid.getLastAdvanceStats().activeCellCount,
             1u,
             "swap moves stored cache");
  testEqSize(g,
             grid.getLastAdvanceStats().countedCellCount,
             1u,
             "swap moves counted cache");
  testTrue(g, other.advance(identity), "swapped large grid advances");
  testEqSize(g,
             other.getLastAdvanceStats().activeCellCount,
             50u,
             "peer receives stored cache");
  testEqSize(g,
             other.getLastAdvanceStats().countedCellCount,
             48u,
             "peer receives counted cache");
  other.clear();
  testTrue(g, other.advance(identity), "cleared grid advances");
  testEqSize(g,
             other.getLastAdvanceStats().activeCellCount,
             0u,
             "clear resets stored cache");
  testEqSize(g,
             other.getLastAdvanceStats().countedCellCount,
             0u,
             "clear resets counted cache");

  GameOfLifeRuleSet life(nullptr);
  SparseCellGrid complete;
  complete.setCell(CellAddress{ 0, 0 }, 0);
  SparseCellGrid::setCellCandidateOverrideForTesting(-1);
  testTrue(g, complete.advance(life), "complete generation removes lone cell");
  SparseCellGrid::setCellCandidateOverrideForTesting(0);
  testTrue(g, complete.advance(life), "empty complete result advances");
  testEqSize(g,
             complete.getLastAdvanceStats().activeCellCount,
             0u,
             "complete-map swap publishes cached empty totals");

  SparseCellGrid frontier;
  frontier.setCell(CellAddress{ 3, 3 }, 0);
  frontier.setCell(CellAddress{ 4, 3 }, 0);
  frontier.setCell(CellAddress{ 3, 4 }, 0);
  frontier.setCell(CellAddress{ 4, 4 }, 0);
  testTrue(g, frontier.advance(life), "frontier still life synchronizes");
  frontier.setCell(CellAddress{ 64, 0 }, 0);
  testTrue(g, frontier.advance(life), "frontier removes edited lone cell");
  testTrue(g, frontier.advance(life), "frontier result settles");
  testEqSize(g,
             frontier.getLastAdvanceStats().activeCellCount,
             4u,
             "frontier-map swap publishes cached stored totals");
  testEqSize(g,
             frontier.getLastAdvanceStats().countedCellCount,
             4u,
             "frontier-map swap publishes cached counted totals");
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
testSparseCompleteHaloScratchReuse()
{
  testSection("SparseCellGrid: retained complete-halo scratch storage");
  WireworldRuleSet rules(nullptr);
  SparseCellGrid grid;
  bool assigned = true;
  for (int chunkY = 0; chunkY < 9; ++chunkY) {
    for (int chunkX = 0; chunkX < 9; ++chunkX) {
      SparseChunkRecord record;
      record.chunkX = chunkX;
      record.chunkY = chunkY;
      record.cells.fill(WireworldRuleSet::CELL_CONDUCTOR);
      assigned = grid.assignChunk(record) && assigned;
    }
  }
  testTrue(g, assigned, "complete-halo source chunks assign");

  SparseCellGrid::setWorkerOverrideForTesting(1);
  SparseCellGrid::setCellCandidateOverrideForTesting(-1);
  testTrue(g, grid.advance(rules), "first complete-halo generation advances");
  testEqSize(g,
             grid.getLastAdvanceStats().targetChunkCount,
             11u * 11u,
             "complete halo owns the exact expanded target set");
  const std::size_t targetCapacity = grid.getCompleteTargetCapacityForTesting();
  const std::size_t indexCapacity =
    grid.getCompleteTargetIndexCapacityForTesting();
  const std::size_t resultCapacity = grid.getCompleteResultCapacityForTesting();
  testTrue(g,
           targetCapacity >= grid.getLastAdvanceStats().targetChunkCount,
           "complete target vector owns retained capacity");
  testTrue(g,
           indexCapacity >= grid.getLastAdvanceStats().targetChunkCount,
           "complete target index owns retained capacity");
  testTrue(g,
           resultCapacity >= grid.getLastAdvanceStats().targetChunkCount,
           "complete result vector owns retained capacity");

  testTrue(g, grid.advance(rules), "second complete-halo generation advances");
  testEqSize(g,
             grid.getCompleteTargetCapacityForTesting(),
             targetCapacity,
             "complete target capacity is reused");
  testEqSize(g,
             grid.getCompleteTargetIndexCapacityForTesting(),
             indexCapacity,
             "complete target-index capacity is reused");
  testEqSize(g,
             grid.getCompleteResultCapacityForTesting(),
             resultCapacity,
             "complete result capacity is reused");

  grid.clear();
  SparseChunkRecord replacement;
  replacement.chunkX = -5;
  replacement.chunkY = -5;
  replacement.cells.fill(WireworldRuleSet::CELL_CONDUCTOR);
  testTrue(g, grid.assignChunk(replacement), "replacement halo chunk assigns");
  testTrue(g, grid.advance(rules), "new complete-target generation advances");
  testEqSize(g,
             grid.getLastAdvanceStats().targetChunkCount,
             9u,
             "old complete-target generation does not remain active");
  testEqSize(g,
             grid.getCompleteTargetIndexCapacityForTesting(),
             indexCapacity,
             "smaller complete world retains its target-index high-water");
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
  testEqSize(g,
             view.getLastSampledTexelCount(),
             static_cast<std::size_t>(view.getViewWidth()) *
               static_cast<std::size_t>(view.getViewHeight()),
             "overview revisions retain full density resampling");
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

static std::size_t
countTextureCreates(const MockBackend& mock)
{
  std::size_t count = 0u;
  for (std::size_t i = 0; i < mock.getCreateCount(); ++i) {
    if (mock.getCreate(i).kind ==
        MockBackend::CreateRecord::Kind::TextureData) {
      count += 1u;
    }
  }
  return count;
}

static void
testTextureCapacityAndLifecycle()
{
  testSection("CanvasView: texture capacity and lifecycle");
  NullRenderWindow window(1280, 720);
  EnvVars env;
  env.setVar("WinX", 1280);
  env.setVar("WinY", 720);
  Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
  MockBackend mock;
  mock.Initialize();
  Renderer renderer(&window, &env, &camera, &mock, false);
  SparseCellGrid grid;
  unsigned long textureHandle = 0;

  {
    CanvasView view(80, 60, &grid, &window, &camera, &renderer);
    const std::size_t initialTextureCreates = countTextureCreates(mock);
    testEqSize(
      g, initialTextureCreates, 1u, "view enrolls one initial display texture");

    view.rebuildTargetsFromGrid();
    const std::size_t grownTextureCreates = countTextureCreates(mock);
    testEqSize(g,
               grownTextureCreates,
               initialTextureCreates + 1u,
               "first capacity growth replaces the display texture once");
    testEqInt(g,
              view.getTextureWidth(),
              120,
              "small width growth reserves fifty percent headroom");

    camera.SetZoom(0.95f);
    view.syncVisibleRegion();
    camera.SetZoom(0.90f);
    view.syncVisibleRegion();
    testEqSize(g,
               countTextureCreates(mock),
               grownTextureCreates,
               "nearby smooth-zoom sizes reuse retained texture capacity");

    for (std::size_t i = 0; i < mock.getCreateCount(); ++i) {
      const MockBackend::CreateRecord& record = mock.getCreate(i);
      if (record.kind != MockBackend::CreateRecord::Kind::TextureData) {
        continue;
      }
      if (textureHandle == 0) {
        textureHandle = record.tableID;
      } else {
        testTrue(g,
                 record.tableID == textureHandle,
                 "capacity replacement preserves the opaque texture handle");
      }
    }
  }

  testEqSize(g,
             mock.getDestroyedTextureCount(),
             1u,
             "view destruction releases its backend texture");
  testTrue(g,
           mock.getDestroyedTexture(0) == textureHandle,
           "view releases the enrolled texture handle");
}

static void
testIncrementalPresentationWork()
{
  testSection("CanvasView: changed-tile sampling and active fades");
  NullRenderWindow window(640, 480);
  EnvVars env;
  env.setVar("WinX", 640);
  env.setVar("WinY", 480);
  Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
  SparseCellGrid grid;
  grid.setCell(CellAddress{ 0, 0 }, 0);
  CanvasView view(80, 60, &grid, &window, &camera, nullptr);
  view.rebuildDefaultPalette();
  view.rebuildTargetsFromGrid();
  const std::size_t fullViewTexels =
    static_cast<std::size_t>(view.getViewWidth()) *
    static_cast<std::size_t>(view.getViewHeight());
  testEqSize(g,
             view.getLastSampledTexelCount(),
             fullViewTexels,
             "initial presentation samples the complete view");

  grid.setCell(CellAddress{ 0, 0 }, SparseCellGrid::BackgroundState);
  view.rebuildTargetsFromGrid();
  testEqSize(g,
             view.getLastSampledTexelCount(),
             SparseCellGrid::kChunkCellCount,
             "one changed chunk resamples one visible tile");
  testEqSize(g,
             view.getFadingTexelCount(),
             1u,
             "one changed cell enrolls one fading texel");
  view.tickVisual(0.01f);
  testEqSize(g,
             view.getLastFadeVisitCount(),
             1u,
             "fade tick visits only the active texel");

  view.setFadeSpeed(0.0f);
  testEqSize(g,
             view.getLastSnapVisitCountForTesting(),
             1u,
             "zero fade speed snaps only the active texel");
  testEqSize(
    g, view.getFadingTexelCount(), 0u, "snap clears the active fade set");
  view.setFadeSpeed(0.0f);
  testEqSize(g,
             view.getLastSnapVisitCountForTesting(),
             1u,
             "repeated zero fade configuration performs no snap scan");

  grid.setCell(CellAddress{ 0, 0 }, 0);
  grid.setCell(CellAddress{ 20, 0 }, 0);
  view.rebuildTargetsFromGrid();
  testEqSize(g,
             view.getLastSampledTexelCount(),
             fullViewTexels,
             "multiple unseen revisions fall back to a complete resample");

  grid.setCell(CellAddress{ 20, 0 }, SparseCellGrid::BackgroundState);
  view.rebuildTargetsFromGrid();
  testEqSize(g,
             view.getLastSampledTexelCount(),
             5u * SparseCellGrid::kChunkDim,
             "single-cell removal clips changed-tile sampling to the view");

  GameOfLifeRuleSet rules(nullptr);
  testTrue(g, grid.advance(rules), "presentation source generation advances");
  view.rebuildTargetsFromGrid();
  testEqSize(g,
             view.getLastSampledTexelCount(),
             SparseCellGrid::kChunkCellCount,
             "one simulation revision publishes its changed tile");

  SparseCellGrid replacement;
  replacement.setCell(CellAddress{ 1, 1 }, 0);
  grid.swap(replacement);
  view.rebuildTargetsFromGrid();
  testEqSize(g,
             view.getLastSampledTexelCount(),
             fullViewTexels,
             "whole-grid replacement invalidates incremental sampling");
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
  registry.add("Illumo.CanvasInf.SparseCandidatePreparation", []() {
    return runCanvasInfCase(testSparseCandidatePreparation);
  });
  registry.add("Illumo.CanvasInf.SparseChangedFrontier",
               []() { return runCanvasInfCase(testSparseChangedFrontier); });
  registry.add("Illumo.CanvasInf.SparseAdaptiveFrontierCost", []() {
    return runCanvasInfCase(testSparseAdaptiveFrontierCost);
  });
  registry.add("Illumo.CanvasInf.SparseCachedStatistics",
               []() { return runCanvasInfCase(testSparseCachedStatistics); });
  registry.add("Illumo.CanvasInf.SparseCandidateScratchReuse", []() {
    return runCanvasInfCase(testSparseCandidateScratchReuse);
  });
  registry.add("Illumo.CanvasInf.SparseCandidateFlatIndex",
               []() { return runCanvasInfCase(testSparseCandidateFlatIndex); });
  registry.add("Illumo.CanvasInf.SparseCompleteHaloScratchReuse", []() {
    return runCanvasInfCase(testSparseCompleteHaloScratchReuse);
  });
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
  registry.add("Illumo.CanvasInf.TextureCapacityAndLifecycle", []() {
    return runCanvasInfCase(testTextureCapacityAndLifecycle);
  });
  registry.add("Illumo.CanvasInf.IncrementalPresentationWork", []() {
    return runCanvasInfCase(testIncrementalPresentationWork);
  });
  registry.add("Illumo.CanvasInf.WorldCellPresentation", []() {
    return runCanvasInfCase(testCanvasViewUsesWorldCellQuad);
  });
  registry.add("Illumo.CanvasInf.CursorCellAlignment",
               []() { return runCanvasInfCase(testCursorUsesCellBounds); });
}
