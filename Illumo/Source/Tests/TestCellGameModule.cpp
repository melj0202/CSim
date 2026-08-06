#include "Engine/IllumoContext.h"
#include "Game/CellGameModule.h"
#include "Services/CommandLine.h"
#include "Services/CommandRegistry.h"
#include "Services/InputManager.h"
#include "Services/SaveLoad.h"
#include "Tests/TestAccess.h"
#include "Tests/TestHarness.h"
#include "Tests/TestHelpers.h"
#include "Tests/TestRegistry.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

static TestCounters g;
static std::string gSaveDialogResult;
static std::string gLoadDialogResult;

std::string
SaveLoad::GetSaveLocation()
{
  return gSaveDialogResult;
}

std::string
SaveLoad::GetLoadLocation()
{
  return gLoadDialogResult;
}

static bool
historyContains(const CommandLine& console, const std::string& text)
{
  const std::vector<CommandLine::historyBuffer>& history = console.getHistory();
  for (const CommandLine::historyBuffer& entry : history) {
    if (entry.content.find(text) != std::string::npos) {
      return true;
    }
  }
  return false;
}

struct CellGameFixture
{
  NullRenderWindow window;
  EnvVars env;
  Camera camera;
  MockBackend mock;
  Renderer renderer;
  CommandRegistry registry;
  CommandLine console;
  InputManager input;
  Scene scene;
  IllumoContext context;
  CellGameModule module;
  bool started;

  CellGameFixture(int width = 8, int height = 6)
    : window(640, 480)
    , env()
    , camera(glm::vec2(1.0f, 1.0f), 1.0f, &env)
    , mock()
    , renderer(&window, &env, &camera, &mock, false)
    , registry()
    , console(&env, &registry, &window, &renderer)
    , input(nullptr)
    , scene(&window, &camera)
    , context{ &scene,  &window, &console, &input,   &renderer,
               nullptr, &env,    &camera,  &registry }
    , module()
    , started(false)
  {
    env.setVar("WinX", 640);
    env.setVar("WinY", 480);
    env.setVar("CanvasX", width);
    env.setVar("CanvasY", height);
    env.setVar("ModeString", "GAME_OF_LIFE");
    env.setVar("tps", 30);
    env.setVar("speedFactor", 1.0);
    env.setVar("cellFadeSpeed", 8.0);
    mock.Initialize();
    module.Start(&context);
    started = CellGameModuleTestAccess::getCellContext(module) != nullptr;
  }

  ~CellGameFixture()
  {
    if (started) {
      module.Exit();
    }
  }

  void execute(const std::string& command,
               const std::vector<std::string>& args = {})
  {
    const bool queued = registry.QueueCommand(command, args);
    testTrue(g, queued, ("registered command queues: " + command).c_str());
    registry.ExecuteQueue();
  }
};

static void
writeSaveFile(const std::filesystem::path& path,
              const std::string& rule,
              int width,
              int height,
              const std::vector<unsigned char>& cells)
{
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  char tag[MAX_RULETAG_SIZE] = {};
  const std::size_t count =
    rule.size() < static_cast<std::size_t>(MAX_RULETAG_SIZE - 1)
      ? rule.size()
      : static_cast<std::size_t>(MAX_RULETAG_SIZE - 1);
  std::memcpy(tag, rule.data(), count);
  output.write(tag, MAX_RULETAG_SIZE);
  output.write(reinterpret_cast<const char*>(&width), sizeof(width));
  output.write(reinterpret_cast<const char*>(&height), sizeof(height));
  if (!cells.empty()) {
    output.write(reinterpret_cast<const char*>(cells.data()),
                 static_cast<std::streamsize>(cells.size()));
  }
}

static void
testStartRegistersGameFeatures()
{
  testSection("CellGameModule: start and command registration");
  CellGameFixture fixture;
  testTrue(g, fixture.started, "valid headless context starts the game module");
  testTrue(g,
           CellGameModuleTestAccess::getState(fixture.module) ==
             CellState::EDIT,
           "module starts in edit mode");
  testEqSize(g,
             fixture.registry.GetCommandNames().size(),
             17,
             "all game commands are registered");
  testTrue(
    g, fixture.registry.HasCommand("ruleset"), "ruleset command registered");
  testTrue(g, fixture.registry.HasCommand("save"), "save command registered");
  testTrue(g, fixture.registry.HasCommand("load"), "load command registered");
  testTrue(g,
           fixture.registry.GetCommandUsage("setcell") ==
             "setcell <x> <y> <state>",
           "command usage metadata registered");
  testEqSize(g,
             fixture.registry.GetCommandCompletions("ruleset").size(),
             7,
             "ruleset completion candidates registered");

  fixture.scene.ClearDrawables();
  fixture.module.DispatchDrawables(&fixture.scene);
  testEqSize(g,
             fixture.scene.drawables.size(),
             1,
             "canvas is dispatched while splash is hidden");

  fixture.module.Exit();
  fixture.started = false;
  testEqSize(g,
             fixture.registry.GetCommandNames().size(),
             2,
             "Exit unregisters every game command");
}

static void
testInvalidContextStartIsContained()
{
  testSection("CellGameModule: invalid start context");
  CellGameModule module;
  module.Start(nullptr);
  testTrue(g,
           CellGameModuleTestAccess::getCellContext(module) == nullptr,
           "null context does not create game state");
  module.Exit();

  CellGameFixture fixture;
  IllumoContext incomplete = fixture.context;
  incomplete.commandRegistry = nullptr;
  CellGameModule incompleteModule;
  incompleteModule.Start(&incomplete);
  testTrue(g,
           CellGameModuleTestAccess::getCellContext(incompleteModule) ==
             nullptr,
           "missing service rejects startup");
  incompleteModule.Exit();
}

static void
testSaveLoadRoundTrip()
{
  testSection("CellGameModule: save/load round trip");
  CellGameFixture fixture(5, 4);
  CellContext* cellContext =
    CellGameModuleTestAccess::getCellContext(fixture.module);
  Canvas* canvas = cellContext->getCellCanvas();
  cellContext->setRuleSet("WIREWORLD");
  canvas->clearCanvas();
  canvas->setCanvasPixel(0, 0, WireworldRuleSet::CELL_HEAD);
  canvas->setCanvasPixel(2, 1, WireworldRuleSet::CELL_TAIL);
  canvas->setCanvasPixel(4, 3, WireworldRuleSet::CELL_CONDUCTOR);

  const std::filesystem::path savePath = "roundtrip.illumo";
  testTrue(g,
           CellGameModuleTestAccess::save(fixture.module, savePath.string()),
           "valid canvas saves");
  canvas->clearCanvas();
  cellContext->setRuleSet("SEEDS");
  testTrue(g,
           CellGameModuleTestAccess::load(fixture.module, savePath.string()),
           "saved canvas loads");
  testTrue(g,
           cellContext->getModeString() == "WIREWORLD",
           "saved ruleset is restored");
  testEqUChar(g,
              canvas->getCanvasPixel(0, 0),
              WireworldRuleSet::CELL_HEAD,
              "head state round-trips");
  testEqUChar(g,
              canvas->getCanvasPixel(2, 1),
              WireworldRuleSet::CELL_TAIL,
              "tail state round-trips");
  testEqUChar(g,
              canvas->getCanvasPixel(4, 3),
              WireworldRuleSet::CELL_CONDUCTOR,
              "conductor state round-trips");
  testEqSize(g,
             std::filesystem::file_size(savePath),
             static_cast<std::size_t>(MAX_RULETAG_SIZE + sizeof(int) * 2 + 20),
             "save has the expected dense binary size");
}

static void
testLoadRejectsInvalidFiles()
{
  testSection("CellGameModule: invalid save validation");
  CellGameFixture fixture;
  testTrue(g,
           !CellGameModuleTestAccess::load(fixture.module, ""),
           "empty load path rejected");
  testTrue(g,
           !CellGameModuleTestAccess::load(fixture.module, "missing.illumo"),
           "missing file rejected");

  std::ofstream("short.illumo", std::ios::binary).write("short", 5);
  testTrue(g,
           !CellGameModuleTestAccess::load(fixture.module, "short.illumo"),
           "truncated header rejected");

  writeSaveFile("unknown-rule.illumo", "NOT_A_RULE", 2, 2, { 1, 1, 1, 1 });
  testTrue(
    g,
    !CellGameModuleTestAccess::load(fixture.module, "unknown-rule.illumo"),
    "unknown ruleset rejected");

  writeSaveFile("invalid-size.illumo", "SEEDS", 0, 4, {});
  testTrue(
    g,
    !CellGameModuleTestAccess::load(fixture.module, "invalid-size.illumo"),
    "zero dimension rejected");
  writeSaveFile("oversized.illumo", "SEEDS", 100000001, 1, {});
  testTrue(g,
           !CellGameModuleTestAccess::load(fixture.module, "oversized.illumo"),
           "oversized canvas rejected before allocation");
  writeSaveFile("short-cells.illumo", "SEEDS", 2, 2, { 0, 1, 0 });
  testTrue(
    g,
    !CellGameModuleTestAccess::load(fixture.module, "short-cells.illumo"),
    "truncated cell data rejected");

  testTrue(g,
           !CellGameModuleTestAccess::save(fixture.module, ""),
           "empty save path rejected");
  testTrue(g,
           !CellGameModuleTestAccess::save(fixture.module, "."),
           "directory cannot be opened as a save file");
}

static void
testLoadCopiesOverlap()
{
  testSection("CellGameModule: different-size save overlap");
  CellGameFixture fixture(4, 3);
  writeSaveFile("small.illumo", "SEEDS", 2, 2, { 0, 1, 1, 0 });
  testTrue(g,
           CellGameModuleTestAccess::load(fixture.module, "small.illumo"),
           "different-size valid save loads");
  Canvas* canvas =
    CellGameModuleTestAccess::getCellContext(fixture.module)->getCellCanvas();
  testEqUChar(g, canvas->getCanvasPixel(0, 0), 0, "overlap row zero copied");
  testEqUChar(
    g, canvas->getCanvasPixel(1, 0), 1, "overlap row zero width preserved");
  testEqUChar(g, canvas->getCanvasPixel(1, 1), 0, "overlap row one copied");
  testEqUChar(
    g, canvas->getCanvasPixel(3, 2), 1, "outside overlap remains empty");
  testTrue(g,
           historyContains(fixture.console, "Loaded overlap from 2 x 2"),
           "size mismatch is reported");
}

static void
testConsoleSimulationCommands()
{
  testSection("CellGameModule: simulation console commands");
  CellGameFixture fixture(6, 6);
  fixture.execute("ruleset", { "SEEDS" });
  testTrue(
    g,
    CellGameModuleTestAccess::getCellContext(fixture.module)->getModeString() ==
      "SEEDS",
    "ruleset command switches mode");
  fixture.execute("mode", { "not_real" });
  testTrue(g,
           historyContains(fixture.console, "Unknown ruleset"),
           "unknown mode is reported");

  fixture.execute("setcell", { "1", "2", "0" });
  Canvas* canvas =
    CellGameModuleTestAccess::getCellContext(fixture.module)->getCellCanvas();
  testEqUChar(
    g, canvas->getCanvasPixel(1, 2), 0, "setcell writes a valid cell");
  fixture.execute("setcell", { "99", "2", "0" });
  fixture.execute("setcell", { "1", "2", "999" });
  testTrue(g,
           historyContains(fixture.console, "outside the canvas"),
           "setcell bounds error is reported");
  testTrue(g,
           historyContains(fixture.console, "Usage: setcell"),
           "setcell state validation is reported");

  fixture.execute("clear_canvas");
  testEqUChar(
    g, canvas->getCanvasPixel(1, 2), 1, "clear command empties cells");
  fixture.execute("randomize", { "100" });
  testEqUChar(g,
              canvas->getCanvasPixel(0, 0),
              0,
              "100 percent binary randomize fills alive cells");
  fixture.execute("randomize", { "0" });
  testEqUChar(
    g, canvas->getCanvasPixel(0, 0), 1, "zero percent randomize empties cells");
  fixture.execute("randomize", { "101" });
  testTrue(g,
           historyContains(fixture.console, "percentage from 0 to 100"),
           "randomize validates density");

  fixture.execute("pause");
  testTrue(g,
           CellGameModuleTestAccess::getState(fixture.module) ==
             CellState::EDIT,
           "pause enters edit state");
  fixture.execute("step", { "2" });
  testTrue(g,
           historyContains(fixture.console, "Advanced 2 generations"),
           "step advances requested generations");
  fixture.execute("step", { "0" });
  testTrue(g,
           historyContains(fixture.console, "integer from 1 to 1000"),
           "step validates generation count");
  fixture.execute("run");
  testTrue(g,
           CellGameModuleTestAccess::getState(fixture.module) ==
             CellState::NORMAL,
           "run enters normal state");
  fixture.execute("status");
  testTrue(g,
           historyContains(fixture.console, "State: RUNNING"),
           "status reports simulation state");
}

static void
testConsoleCameraAndFiles()
{
  testSection("CellGameModule: camera and file console commands");
  CellGameFixture fixture(4, 4);
  fixture.execute("camera", { "10.5", "20.5", "2.5" });
  testTrue(g,
           fixture.camera.GetPosition() == glm::vec2(10.5f, 20.5f),
           "camera command updates position");
  testTrue(g, fixture.camera.GetZoom() == 2.5f, "camera command updates zoom");
  fixture.execute("camera", { "bad", "20" });
  testTrue(g,
           historyContains(fixture.console, "Usage: camera"),
           "camera validates numeric arguments");
  fixture.execute("camera_reset");
  testTrue(g,
           historyContains(fixture.console, "Camera reset"),
           "camera reset command reports success");

  fixture.execute("save", { "console-save" });
  testTrue(g,
           std::filesystem::exists("console-save.illumo"),
           "save command adds extension");
  fixture.execute("load", { "console-save" });
  testTrue(
    g,
    historyContains(fixture.console, "Loaded canvas from console-save.illumo"),
    "load command falls back to extension");
  fixture.execute("save", {});
  fixture.execute("load", {});
  testTrue(g,
           historyContains(fixture.console, "Usage: save"),
           "save command validates arguments");
  testTrue(g,
           historyContains(fixture.console, "Usage: load"),
           "load command validates arguments");

  gSaveDialogResult.clear();
  gLoadDialogResult.clear();
  fixture.execute("save_dialog");
  fixture.execute("load_dialog");
  testTrue(g,
           historyContains(fixture.console, "Save cancelled"),
           "cancelled save dialog is reported");
  testTrue(g,
           historyContains(fixture.console, "Load cancelled"),
           "cancelled load dialog is reported");

  gSaveDialogResult = "dialog-save";
  fixture.execute("save_dialog");
  testTrue(g,
           std::filesystem::exists("dialog-save.illumo"),
           "save dialog path gains extension");
  gLoadDialogResult = "dialog-save.illumo";
  fixture.execute("load_dialog");
  testTrue(
    g,
    historyContains(fixture.console, "Loaded canvas from dialog-save.illumo"),
    "load dialog uses selected path");
}

static void
testUpdateStateAndTiming()
{
  testSection("CellGameModule: update state and timing");
  CellGameFixture fixture(5, 5);
  CellContext* cellContext =
    CellGameModuleTestAccess::getCellContext(fixture.module);
  Canvas* canvas = cellContext->getCellCanvas();
  canvas->clearCanvas();
  canvas->setCanvasPixel(1, 2, 0);
  canvas->setCanvasPixel(2, 2, 0);
  canvas->setCanvasPixel(3, 2, 0);

  InputManagerTestAccess::setAction(
    fixture.input, KeyCode::E, InputAction::Press);
  fixture.module.Update(0.0);
  testTrue(g,
           CellGameModuleTestAccess::getState(fixture.module) ==
             CellState::NORMAL,
           "toggle action enters normal state");
  InputManagerTestAccess::setAction(
    fixture.input, KeyCode::E, InputAction::None);
  fixture.module.Update(0.04);
  testEqUChar(g,
              canvas->getCanvasPixel(2, 1),
              0,
              "normal update advances simulation at configured tps");

  fixture.env.setVar("tps", 100000);
  fixture.env.setVar("speedFactor", 1000.0);
  fixture.env.setVar("cellFadeSpeed", -2.0);
  fixture.module.Update(1.0);
  testTrue(g,
           CellGameModuleTestAccess::getState(fixture.module) ==
             CellState::NORMAL,
           "large delta and rate remain bounded");

  InputManager::scrollCallback(nullptr, 0.0, 1.0);
  const float oldZoom = fixture.camera.GetZoom();
  fixture.module.Update(-1.0);
  fixture.camera.Update(1.0f);
  testTrue(
    g, fixture.camera.GetZoom() > oldZoom, "scroll input updates camera zoom");

  InputManagerTestAccess::setAction(
    fixture.input, KeyCode::E, InputAction::Press);
  fixture.module.Update(0.0);
  testTrue(g,
           CellGameModuleTestAccess::getState(fixture.module) ==
             CellState::EDIT,
           "toggle action returns to edit state");
  InputManagerTestAccess::setAction(
    fixture.input, KeyCode::E, InputAction::None);
  fixture.module.Update(0.016);
}

static int
runCellGameModuleCase(void (*testFunction)())
{
  g.failures = 0;
  gSaveDialogResult.clear();
  gLoadDialogResult.clear();
  testFunction();
  return g.failures;
}

void
registerCellGameModuleTests(IllumoTestRegistry& registry)
{
  registry.add("Illumo.CellGame.StartAndRegistration", []() {
    return runCellGameModuleCase(testStartRegistersGameFeatures);
  });
  registry.add("Illumo.CellGame.InvalidContext", []() {
    return runCellGameModuleCase(testInvalidContextStartIsContained);
  });
  registry.add("Illumo.CellGame.SaveLoadRoundTrip",
               []() { return runCellGameModuleCase(testSaveLoadRoundTrip); });
  registry.add("Illumo.CellGame.InvalidSaveFiles", []() {
    return runCellGameModuleCase(testLoadRejectsInvalidFiles);
  });
  registry.add("Illumo.CellGame.LoadOverlap",
               []() { return runCellGameModuleCase(testLoadCopiesOverlap); });
  registry.add("Illumo.CellGame.SimulationCommands", []() {
    return runCellGameModuleCase(testConsoleSimulationCommands);
  });
  registry.add("Illumo.CellGame.CameraAndFileCommands", []() {
    return runCellGameModuleCase(testConsoleCameraAndFiles);
  });
  registry.add("Illumo.CellGame.UpdateStateAndTiming", []() {
    return runCellGameModuleCase(testUpdateStateAndTiming);
  });
}
