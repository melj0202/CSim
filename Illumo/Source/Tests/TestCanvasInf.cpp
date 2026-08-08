#include "Game/CanvasView.h"
#include "Game/SparseCellGrid.h"
#include "Rendering/Camera.h"
#include "Rendering/Mock/MockBackend.h"
#include "Rendering/Renderer.h"
#include "Rulesets/BrainsBrainRuleSet.h"
#include "Rulesets/GameOfLifeRuleSet.h"
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
           view.getVisibleCell(1, 2) == CellAddress{ 0, 0 },
           "view samples the centered origin");
  const unsigned char* pixels = view.getDisplayTexBuffer();
  testEqUChar(
    g, pixels[(2 * 4 + 1) * 3], 0, "visible alive cell is staged black");

  grid.setCell(CellAddress{ 0, 0 }, SparseCellGrid::BackgroundState);
  view.rebuildTargetsFromGrid();
  testTrue(g, view.isFadeActive(), "target changes start visible-area fade");
  view.setFadeSpeed(0.0f);
  testTrue(g, !view.isFadeActive(), "zero fade speed snaps immediately");
  camera.SetPosition(glm::vec2(32.0f, 0.0f));
  view.syncVisibleRegion();
  testTrue(g,
           view.getVisibleCell(1, 2) == CellAddress{ 2, 0 },
           "camera movement reveals a new world region");
}

static void
testCanvasTextureUsesLinearFiltering()
{
  testSection("CanvasView: linear filtering for zoomed display");
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

  bool foundCanvasTexture = false;
  bool isLinear = false;
  for (std::size_t i = 0; i < mock.getCreateCount(); ++i) {
    const MockBackend::CreateRecord& record = mock.getCreate(i);
    if (record.kind == MockBackend::CreateRecord::Kind::TextureData) {
      foundCanvasTexture = true;
      isLinear = record.filter == TextureFilter::Linear;
    }
  }
  testTrue(g, foundCanvasTexture, "CanvasView enrolls a display texture");
  testTrue(g, isLinear, "CanvasView requests linear texture filtering");
}

static int
runCanvasInfCase(void (*testFunction)())
{
  g.failures = 0;
  testFunction();
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
  registry.add("Illumo.CanvasInf.MultiStateTransitions",
               []() { return runCanvasInfCase(testMultiStateTransitions); });
  registry.add("Illumo.CanvasInf.BoundedView",
               []() { return runCanvasInfCase(testBoundedCanvasView); });
  registry.add("Illumo.CanvasInf.LinearTextureFiltering", []() {
    return runCanvasInfCase(testCanvasTextureUsesLinearFiltering);
  });
}
