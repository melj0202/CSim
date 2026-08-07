#include "CellGameModule.h"
#include "Rulesets/WireworldRuleSet.h"
#include "Services/Logger.h"
#include "Services/SaveLoad.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>

static bool
parseIntegerArgument(const std::string& text, int* value)
{
  if (value == nullptr || text.empty()) {
    return false;
  }
  try {
    std::size_t consumed = 0;
    long long parsed = std::stoll(text, &consumed);
    if (consumed != text.size() || parsed < -2147483648LL ||
        parsed > 2147483647LL) {
      return false;
    }
    *value = static_cast<int>(parsed);
    return true;
  } catch (...) {
    return false;
  }
}

static bool
parseFloatingArgument(const std::string& text, double* value)
{
  if (value == nullptr || text.empty()) {
    return false;
  }
  try {
    std::size_t consumed = 0;
    double parsed = std::stod(text, &consumed);
    if (consumed != text.size()) {
      return false;
    }
    *value = parsed;
    return true;
  } catch (...) {
    return false;
  }
}

static std::string
withIllumoExtension(const std::string& filename)
{
  const std::string extension = ".illumo";
  if (filename.size() >= extension.size()) {
    std::string ending = filename.substr(filename.size() - extension.size());
    for (std::size_t i = 0; i < ending.size(); ++i) {
      ending[i] =
        static_cast<char>(std::tolower(static_cast<unsigned char>(ending[i])));
    }
    if (ending == extension) {
      return filename;
    }
  }
  return filename + extension;
}

CellGameModule::CellGameModule()
  : cellContext(nullptr)
  , currentState(CellState::EDIT)
  , simAccum(0.0)
  , simStepSeconds(1.0 / 30.0)
  , wireworldBrush(WireworldRuleSet::CELL_CONDUCTOR)
  , modeSplash(nullptr)
{
  ic = nullptr;
}

CellGameModule::~CellGameModule() {}

bool
CellGameModule::Start(IllumoContext* context)
{
  // D-E5: fail loud if the frozen service bag is incomplete.
  if (!IllumoContextHasGameCore(context)) {
    Logger::LogError(
      "CellGameModule::Start: IllumoContext missing required services "
      "(envVars, window, camera, renderer, inputManager, commandLine, "
      "commandRegistry, scene)");
    ic = context;
    return false;
  }
  ic = context;

  // Prefer ModeString from envvars / previous console command.
  std::string startMode = ic->envVars->getVar("ModeString").value;
  if (startMode.empty()) {
    startMode = "GAME_OF_LIFE";
  }
  this->cellContext = new CellContext(
    startMode, ic->envVars, ic->window, ic->camera, ic->renderer);

  // Simulation step rate comes from env (tps * speedFactor). Re-read live in
  // Normal().
  simAccum = 0.0;
  syncSimRateFromEnv();

  InputEvent ac;
  ac.keyCode = KeyCode::MouseMiddle;
  ac.inputAction = InputAction::Press;
  this->inputContext.bindAction("CameraPan", ac);

  ac.keyCode = KeyCode::MouseRight;
  ac.inputAction = InputAction::Press;
  this->inputContext.bindAction("CameraRotate", ac);

  ac.keyCode = KeyCode::C;
  ac.inputAction = InputAction::Press;
  this->inputContext.bindAction("ClearCanvas", ac);

  ac.keyCode = KeyCode::E;
  ac.inputAction = InputAction::Press;
  this->inputContext.bindAction("ToggleState", ac);

  ac.keyCode = KeyCode::MouseLeft;
  ac.inputAction = InputAction::Press;
  this->inputContext.bindAction("PaintCanvas", ac);

  long contextId = ic->inputManager->registerInputContext(this->inputContext);
  ic->inputManager->setActiveInputContext(contextId);

  currentState = CellState::EDIT;
  wireworldBrush = WireworldRuleSet::CELL_CONDUCTOR;

  // Ruleset-aware startup seed (GoL glider, Wireworld electron-on-wire, …).
  seedInitialPattern();

  // Initial palette from active ruleset; seed cells already mark upload dirty.
  cellContext->getCellCanvas()->rebuildPalette(cellContext->getRuleSet());
  updateVisualTargets();
  registerConsoleCommands();

  // Mode splash label (top-left corner). Shown briefly when toggling
  // EDIT/NORMAL with E. Owned by this module (not a translation-unit global).
  if (modeSplash == nullptr && ic->renderer != nullptr) {
    modeSplash = std::make_unique<SplashText>(
      "EDIT", 255, 230, 120, 255, 32, 16, 48, ic->renderer);
    modeSplash->setVisible(false);
  }

  return true;
}

void
CellGameModule::showModeSplash(const char* label)
{
  if (modeSplash == nullptr || label == nullptr) {
    return;
  }
  modeSplash->setContent(label);
  modeSplash->Wake();
}

void
CellGameModule::seedInitialPattern()
{
  Canvas* canvas = cellContext->getCellCanvas();
  const int w = canvas->canvasWidth;
  const int h = canvas->canvasHeight;
  const int ox = w / 2 - 1;
  const int oy = h / 2 - 1;

  if (cellContext->getModeString() == "WIREWORLD") {
    // Horizontal conductor with a head+tail pair so one electron travels right.
    if (w < 8 || h < 3) {
      Logger::LogWarning(
        "Canvas too small for Wireworld seed; skipping initial pattern");
      return;
    }
    const int y = h / 2;
    const int startX = w / 2 - 4;
    for (int i = 0; i < 8; ++i) {
      canvas->setCanvasPixel(startX + i, y, WireworldRuleSet::CELL_CONDUCTOR);
    }
    canvas->setCanvasPixel(startX, y, WireworldRuleSet::CELL_HEAD);
    canvas->setCanvasPixel(startX + 1, y, WireworldRuleSet::CELL_TAIL);
    return;
  }

  // Classic Game-of-Life glider (pointing down-right). Works for most binary
  // life-like rules as a visible non-empty startup.
  if (w >= 5 && h >= 5) {
    canvas->setCanvasPixel(ox + 1, oy + 0, 0);
    canvas->setCanvasPixel(ox + 2, oy + 1, 0);
    canvas->setCanvasPixel(ox + 0, oy + 2, 0);
    canvas->setCanvasPixel(ox + 1, oy + 2, 0);
    canvas->setCanvasPixel(ox + 2, oy + 2, 0);
  } else {
    Logger::LogWarning(
      "Canvas too small for glider seed; skipping initial pattern");
  }
}

void
CellGameModule::updateWireworldBrushFromInput()
{
  if (ic == nullptr || ic->inputManager == nullptr ||
      ic->commandLine == nullptr || ic->commandLine->isOpen) {
    return;
  }
  // Sticky brush: last selected key wins until another is pressed.
  // 1/H = head, 2 = empty, 3/T = tail, 4 = conductor (default).
  if (ic->inputManager->isKeyPressed(KeyCode::Num1) ||
      ic->inputManager->isKeyPressed(KeyCode::H)) {
    wireworldBrush = WireworldRuleSet::CELL_HEAD;
  } else if (ic->inputManager->isKeyPressed(KeyCode::Num2)) {
    wireworldBrush = WireworldRuleSet::CELL_EMPTY;
  } else if (ic->inputManager->isKeyPressed(KeyCode::Num3) ||
             ic->inputManager->isKeyPressed(KeyCode::T)) {
    wireworldBrush = WireworldRuleSet::CELL_TAIL;
  } else if (ic->inputManager->isKeyPressed(KeyCode::Num4)) {
    wireworldBrush = WireworldRuleSet::CELL_CONDUCTOR;
  }
}

void
CellGameModule::updateVisualTargets()
{
  ZoneScopedN("Visual.updateTargets");
  // life → palette target colors (sparse); tickVisual eases displayRgb toward
  // them.
  cellContext->getCellCanvas()->rebuildTargetsFromLife();
}

void
CellGameModule::registerConsoleCommands()
{
  if (ic == nullptr || ic->commandRegistry == nullptr ||
      ic->commandLine == nullptr) {
    return;
  }

  const std::vector<std::string> rulesets = CellContext::GetKnownModeStrings();
  CommandFn rulesetCommand = [this](const std::vector<std::string>& args) {
    if (args.empty()) {
      ic->commandLine->logNormal("Current ruleset: " +
                                 cellContext->getModeString());
      ic->commandLine->logNormal("Usage: ruleset <name>");
      return;
    }
    if (args.size() != 1) {
      ic->commandLine->logError("Usage: ruleset <name>");
      return;
    }

    const std::string mode = CellContext::NormalizeModeString(args[0]);
    if (!CellContext::IsKnownModeString(mode)) {
      ic->commandLine->logError("Unknown ruleset '" + args[0] + "'");
      return;
    }

    if (cellContext->setRuleSet(mode)) {
      cellContext->getCellCanvas()->rebuildPalette(cellContext->getRuleSet());
      updateVisualTargets();
    }
    ic->commandLine->logSuccess("Ruleset: " + cellContext->getModeString());
  };
  ic->commandRegistry->RegisterCommand(
    "ruleset",
    rulesetCommand,
    "ruleset [name]",
    "Show or change the cellular-automaton ruleset",
    rulesets);
  ic->commandRegistry->RegisterCommand(
    "mode", rulesetCommand, "mode [name]", "Alias for ruleset", rulesets);

  ic->commandRegistry->RegisterCommand(
    "save",
    [this](const std::vector<std::string>& args) {
      if (args.size() != 1) {
        ic->commandLine->logError("Usage: save <filename>");
        return;
      }
      const std::string filename = withIllumoExtension(args[0]);
      if (SaveCellGame(filename)) {
        ic->commandLine->logSuccess("Saved canvas to " + filename);
      }
    },
    "save <filename>",
    "Save the current canvas; .illumo is added when omitted");

  ic->commandRegistry->RegisterCommand(
    "load",
    [this](const std::vector<std::string>& args) {
      if (args.size() != 1) {
        ic->commandLine->logError("Usage: load <filename>");
        return;
      }
      std::string filename = args[0];
      std::ifstream exactFile(filename, std::ios::binary);
      if (!exactFile.is_open()) {
        filename = withIllumoExtension(filename);
      }
      exactFile.close();
      if (LoadCellGame(filename)) {
        ic->commandLine->logSuccess("Loaded canvas from " + filename);
      }
    },
    "load <filename>",
    "Load a canvas and activate its saved ruleset");

  ic->commandRegistry->RegisterCommand(
    "save_dialog",
    [this](const std::vector<std::string>& args) {
      if (!args.empty()) {
        ic->commandLine->logError("Usage: save_dialog");
        return;
      }
      const std::string selectedPath = SaveLoad::GetSaveLocation();
      if (selectedPath.empty()) {
        ic->commandLine->logWarning("Save cancelled");
        return;
      }
      const std::string filename = withIllumoExtension(selectedPath);
      if (SaveCellGame(filename)) {
        ic->commandLine->logSuccess("Saved canvas to " + filename);
      }
    },
    "save_dialog",
    "Open the native save-file picker");

  ic->commandRegistry->RegisterCommand(
    "load_dialog",
    [this](const std::vector<std::string>& args) {
      if (!args.empty()) {
        ic->commandLine->logError("Usage: load_dialog");
        return;
      }
      const std::string filename = SaveLoad::GetLoadLocation();
      if (filename.empty()) {
        ic->commandLine->logWarning("Load cancelled");
        return;
      }
      if (LoadCellGame(filename)) {
        ic->commandLine->logSuccess("Loaded canvas from " + filename);
      }
    },
    "load_dialog",
    "Open the native load-file picker");

  ic->commandRegistry->RegisterCommand(
    "pause",
    [this](const std::vector<std::string>& args) {
      if (!args.empty()) {
        ic->commandLine->logError("Usage: pause");
        return;
      }
      setRunning(false);
    },
    "pause",
    "Pause simulation and enter edit mode");

  ic->commandRegistry->RegisterCommand(
    "run",
    [this](const std::vector<std::string>& args) {
      if (!args.empty()) {
        ic->commandLine->logError("Usage: run");
        return;
      }
      setRunning(true);
    },
    "run",
    "Resume continuous simulation");

  ic->commandRegistry->RegisterCommand(
    "step",
    [this](const std::vector<std::string>& args) {
      int generations = 1;
      if ((!args.empty() && !parseIntegerArgument(args[0], &generations)) ||
          args.size() > 1 || generations < 1 || generations > 1000) {
        ic->commandLine->logError(
          "step count must be an integer from 1 to 1000");
        return;
      }
      stepSimulation(generations);
      ic->commandLine->logSuccess(
        "Advanced " + std::to_string(generations) +
        (generations == 1 ? " generation" : " generations"));
    },
    "step [count]",
    "Advance a paused canvas by one or more generations");

  ic->commandRegistry->RegisterCommand(
    "clear_canvas",
    [this](const std::vector<std::string>& args) {
      if (!args.empty()) {
        ic->commandLine->logError("Usage: clear_canvas");
        return;
      }
      cellContext->getCellCanvas()->clearCanvas();
      ic->commandLine->logSuccess("Canvas cleared");
    },
    "clear_canvas",
    "Set every cell to the empty state");

  ic->commandRegistry->RegisterCommand(
    "randomize",
    [this](const std::vector<std::string>& args) {
      double density = 25.0;
      if ((!args.empty() && !parseFloatingArgument(args[0], &density)) ||
          args.size() > 1 || density < 0.0 || density > 100.0) {
        ic->commandLine->logError(
          "randomize density must be a percentage from 0 to 100");
        return;
      }

      Canvas* canvas = cellContext->getCellCanvas();
      std::mt19937 generator(std::random_device{}());
      std::uniform_real_distribution<double> distribution(0.0, 100.0);
      const bool wireworld = cellContext->getModeString() == "WIREWORLD";
      for (int y = 0; y < canvas->canvasHeight; ++y) {
        for (int x = 0; x < canvas->canvasWidth; ++x) {
          const bool selected = distribution(generator) < density;
          const unsigned char state =
            wireworld ? (selected ? WireworldRuleSet::CELL_CONDUCTOR
                                  : WireworldRuleSet::CELL_EMPTY)
                      : (selected ? static_cast<unsigned char>(0)
                                  : static_cast<unsigned char>(1));
          canvas->setCanvasPixel(x, y, state);
        }
      }
      ic->commandLine->logSuccess("Randomized canvas at " +
                                  std::to_string(density) + "% density");
    },
    "randomize [density-percent]",
    "Fill the canvas randomly; Wireworld creates conductors");

  ic->commandRegistry->RegisterCommand(
    "setcell",
    [this](const std::vector<std::string>& args) {
      int x = 0;
      int y = 0;
      int state = 0;
      if (args.size() != 3 || !parseIntegerArgument(args[0], &x) ||
          !parseIntegerArgument(args[1], &y) ||
          !parseIntegerArgument(args[2], &state) || state < 0 || state > 255) {
        ic->commandLine->logError("Usage: setcell <x> <y> <state 0..255>");
        return;
      }
      if (!cellContext->getCellCanvas()->setCanvasPixel(
            x, y, static_cast<unsigned char>(state))) {
        ic->commandLine->logError("Cell coordinates are outside the canvas");
        return;
      }
      ic->commandLine->logSuccess("Cell (" + std::to_string(x) + ", " +
                                  std::to_string(y) +
                                  ") = " + std::to_string(state));
    },
    "setcell <x> <y> <state>",
    "Set one cell state directly, including Wireworld head/tail states");

  ic->commandRegistry->RegisterCommand(
    "camera_reset",
    [this](const std::vector<std::string>& args) {
      if (!args.empty()) {
        ic->commandLine->logError("Usage: camera_reset");
        return;
      }
      ic->camera->Reset();
      ic->commandLine->logSuccess("Camera reset");
    },
    "camera_reset",
    "Center the canvas and restore 1x zoom");

  ic->commandRegistry->RegisterCommand(
    "camera",
    [this](const std::vector<std::string>& args) {
      if (args.empty()) {
        glm::vec2 position = ic->camera->GetPosition();
        ic->commandLine->logNormal(
          "Camera: x=" + std::to_string(position.x) +
          " y=" + std::to_string(position.y) +
          " zoom=" + std::to_string(ic->camera->GetZoom()));
        return;
      }
      double x = 0.0;
      double y = 0.0;
      double zoom = static_cast<double>(ic->camera->GetZoom());
      if ((args.size() != 2 && args.size() != 3) ||
          !parseFloatingArgument(args[0], &x) ||
          !parseFloatingArgument(args[1], &y) ||
          (args.size() == 3 && !parseFloatingArgument(args[2], &zoom)) ||
          zoom < 0.1 || zoom > 100.0) {
        ic->commandLine->logError("Usage: camera <x> <y> [zoom 0.1..100]");
        return;
      }
      ic->camera->SetPosition(
        glm::vec2(static_cast<float>(x), static_cast<float>(y)));
      ic->camera->SetZoom(static_cast<float>(zoom));
      ic->commandLine->logSuccess("Camera updated");
    },
    "camera [x y [zoom]]",
    "Show or set camera position and zoom");

  ic->commandRegistry->RegisterCommand(
    "status",
    [this](const std::vector<std::string>& args) {
      if (!args.empty()) {
        ic->commandLine->logError("Usage: status");
        return;
      }
      printStatus();
    },
    "status",
    "Show simulation, canvas, ruleset, and camera state");
}

void
CellGameModule::unregisterConsoleCommands()
{
  if (ic == nullptr || ic->commandRegistry == nullptr) {
    return;
  }
  const char* commandNames[] = { "camera",  "camera_reset", "clear_canvas",
                                 "load",    "load_dialog",  "mode",
                                 "pause",   "randomize",    "ruleset",
                                 "run",     "save",         "save_dialog",
                                 "setcell", "status",       "step" };
  for (const char* commandName : commandNames) {
    ic->commandRegistry->UnregisterCommand(commandName);
  }
}

void
CellGameModule::setRunning(bool running)
{
  currentState = running ? CellState::NORMAL : CellState::EDIT;
  simAccum = 0.0;
  showModeSplash(running ? "NORMAL" : "EDIT");
  ic->commandLine->logSuccess(running ? "Simulation running"
                                      : "Simulation paused in edit mode");
}

void
CellGameModule::stepSimulation(int generations)
{
  currentState = CellState::EDIT;
  simAccum = 0.0;
  showModeSplash("EDIT");
  Canvas* canvas = cellContext->getCellCanvas();
  for (int i = 0; i < generations; ++i) {
    cellContext->getRuleSet()->calcGeneration(
      0, 0, canvas->canvasWidth, canvas->canvasHeight);
  }
  updateVisualTargets();
}

void
CellGameModule::printStatus() const
{
  const Canvas* canvas = cellContext->getCellCanvas();
  const glm::vec2 cameraPosition = ic->camera->GetPosition();
  ic->commandLine->logNormal(
    std::string("State: ") +
    (currentState == CellState::NORMAL ? "RUNNING" : "PAUSED/EDIT"));
  ic->commandLine->logNormal("Ruleset: " + cellContext->getModeString());
  ic->commandLine->logNormal("Canvas: " + std::to_string(canvas->canvasWidth) +
                             " x " + std::to_string(canvas->canvasHeight));
  ic->commandLine->logNormal("Rate: " + ic->envVars->getVar("tps").value +
                             " tps x " +
                             ic->envVars->getVar("speedFactor").value);
  ic->commandLine->logNormal("Camera: x=" + std::to_string(cameraPosition.x) +
                             " y=" + std::to_string(cameraPosition.y) +
                             " zoom=" + std::to_string(ic->camera->GetZoom()));
}

void
CellGameModule::syncSimRateFromEnv()
{
  long tps = ic->envVars->getVar("tps").valueAsLong;
  if (tps < 1)
    tps = 1;
  if (tps > 1000)
    tps = 1000;

  double speedFactor = ic->envVars->getVar("speedFactor").valueAsDouble;
  if (speedFactor <= 0.0)
    speedFactor = 1.0;
  if (speedFactor > 100.0)
    speedFactor = 100.0;

  const double effectiveTps = static_cast<double>(tps) * speedFactor;
  simStepSeconds = 1.0 / effectiveTps;

  float fadeSpeed = 8.0f;
  if (ic->envVars->getVar("cellFadeSpeed").value != "") {
    fadeSpeed =
      static_cast<float>(ic->envVars->getVar("cellFadeSpeed").valueAsDouble);
  }
  if (fadeSpeed < 0.0f)
    fadeSpeed = 0.0f;
  cellContext->getCellCanvas()->setFadeSpeed(fadeSpeed);
}

void
CellGameModule::Update(double dt)
{
  ZoneNamed(CellGameModuleUpdateZone, "CellGameModule Update");

  // Host erases modules that fail Start; still guard for incomplete fixtures.
  if (cellContext == nullptr || ic == nullptr) {
    return;
  }

  // Apply ruleset changes from console (`ruleset SEEDS`) or env ModeString.
  {
    std::string wanted = ic->envVars->getVar("ModeString").value;
    if (!wanted.empty() && wanted != cellContext->getModeString()) {
      if (cellContext->setRuleSet(wanted)) {
        std::string msg = "Active ruleset: " + cellContext->getModeString();
        Logger::LogInfo(msg.c_str());
        // Same life values, new colors → rebuild palette only (no cell
        // re-upload).
        cellContext->getCellCanvas()->rebuildPalette(cellContext->getRuleSet());
        if (cellContext->getModeString() == "WIREWORLD") {
          wireworldBrush = WireworldRuleSet::CELL_CONDUCTOR;
        }
      }
    }
  }

  // Common behavior: camera panning & scroll zoom (only when console is closed)
  if (!ic->commandLine->isOpen) {
    CameraPan();

    // Zoom behavior using scroll offset
    std::array<double, 2> mouseCoords = ic->window->getMouseCoords();
    glm::vec2 worldMouse =
      ic->camera->ScreenToWorld(glm::vec2(mouseCoords[0], mouseCoords[1]));
    double* scroll = ic->inputManager->getMouseScrollOffset();
    if (*scroll != 0.0f) {
      double zoomFactor = (*scroll > 0.0f) ? 1.15 : 0.85;
      ic->camera->ZoomAt(static_cast<float>(zoomFactor), worldMouse);
    }
  }

  // Toggle between NORMAL and EDIT states with 'E' key (only when console is
  // closed)
  if (!ic->commandLine->isOpen &&
      ic->inputManager->isActionActive("ToggleState")) {
    if (currentState == CellState::NORMAL) {
      currentState = CellState::EDIT;
      showModeSplash("EDIT");
      Logger::LogInfo("State changed to EDIT");
    } else {
      currentState = CellState::NORMAL;
      simAccum = 0.0;
      showModeSplash("NORMAL");
      Logger::LogInfo("State changed to NORMAL");
    }
  }

  // State dependent behavior
  switch (currentState) {
    case CellState::NORMAL:
      Normal(dt);
      break;
    case CellState::EDIT:
      Edit(dt);
      break;
    case CellState::EXIT:
      Exit();
      break;
    default:
      break;
  }

  // Map dirty life cells to palette target colors, then ease display toward
  // them.
  updateVisualTargets();
  cellContext->getCellCanvas()->tickVisual(static_cast<float>(dt));
}

void
CellGameModule::Exit()
{
  unregisterConsoleCommands();
  modeSplash.reset();
  delete cellContext;
  cellContext = nullptr;
}

void
CellGameModule::Normal(double dt)
{
  int width = this->cellContext->getCellCanvas()->canvasWidth;
  int height = this->cellContext->getCellCanvas()->canvasHeight;

  // Pick up tps / speedFactor changes from envvars.json or console (`tps 60`).
  syncSimRateFromEnv();

  // Advance simulation on the tps clock, not every render frame.
  if (dt < 0.0) {
    dt = 0.0;
  }
  // Avoid huge single-frame jumps after a breakpoint / alt-tab.
  if (dt > 0.25) {
    dt = 0.25;
  }

  simAccum += dt;

  // Allow enough steps to honor high tps; still cap so a stall can't melt the
  // CPU.
  const int maxSteps = 64;
  int steps = 0;
  while (simAccum >= simStepSeconds && steps < maxSteps) {
    this->cellContext->getRuleSet()->calcGeneration(0, 0, width, height);
    simAccum -= simStepSeconds;
    steps += 1;
  }

  // Drop leftover debt if we hit the cap so we don't forever "catch up".
  if (steps >= maxSteps && simAccum > simStepSeconds) {
    FrameMarkNamed("Sim.debtDropped");
    simAccum = 0.0;
  }
}

void
CellGameModule::Edit(double dt)
{
  (void)dt;
  int width = cellContext->getCellCanvas()->canvasWidth;
  int height = cellContext->getCellCanvas()->canvasHeight;
  static bool wasPressed = false;
  static int lastMouseX = -1;
  static int lastMouseY = -1;

  std::array<double, 2> mouseCoords = ic->window->getMouseCoords();
  glm::vec2 worldPos =
    ic->camera->ScreenToWorld(glm::vec2(mouseCoords[0], mouseCoords[1]));

  float cellSize = 16.0f;
  int currentX = static_cast<int>(std::floor(worldPos.x / cellSize));
  int currentY = static_cast<int>(std::floor(worldPos.y / cellSize));

  if (!ic->commandLine->isOpen) {
    if (cellContext->getModeString() == "WIREWORLD") {
      updateWireworldBrushFromInput();
    }

    bool isLeftPressed =
      ic->inputManager->isMouseButtonPressed(KeyCode::MouseLeft);
    bool isRightPressed =
      ic->inputManager->isMouseButtonPressed(KeyCode::MouseRight);

    if (isLeftPressed || isRightPressed) {
      // Binary CAs: left = alive (0), right = dead (1).
      // Wireworld: left = active brush (1/H head, 3/T tail, 4 conductor),
      // right = empty.
      unsigned char colorVal = isLeftPressed ? 0 : 1;
      if (cellContext->getModeString() == "WIREWORLD") {
        colorVal =
          isLeftPressed ? wireworldBrush : WireworldRuleSet::CELL_EMPTY;
      }

      if (wasPressed) {
        int x0 = lastMouseX;
        int y0 = lastMouseY;
        int x1 = currentX;
        int y1 = currentY;
        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;

        while (true) {
          if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
            this->cellContext->getCellCanvas()->setCanvasPixel(
              x0, y0, colorVal);
          }
          if (x0 == x1 && y0 == y1)
            break;
          int e2 = 2 * err;
          if (e2 > -dy) {
            err -= dy;
            x0 += sx;
          }
          if (e2 < dx) {
            err += dx;
            y0 += sy;
          }
        }
      } else {
        if (currentX >= 0 && currentX < width && currentY >= 0 &&
            currentY < height) {
          this->cellContext->getCellCanvas()->setCanvasPixel(
            currentX, currentY, colorVal);
        }
      }
      wasPressed = true;
      lastMouseX = currentX;
      lastMouseY = currentY;
    } else {
      wasPressed = false;
      if (ic->inputManager->isKeyPressed(KeyCode::C))
        this->cellContext->getCellCanvas()->clearCanvas();
    }
  } else {
    wasPressed = false;
  }
}

bool
CellGameModule::SaveCellGame(std::string filename)
{
  if (filename.empty()) {
    ic->commandLine->logError("Save path is empty");
    return false;
  }

  std::ofstream file(filename, std::ios::binary | std::ios::trunc);
  if (!file.is_open()) {
    ic->commandLine->logError("Failed to open for saving: " + filename);
    return false;
  }

  char ruleTag[MAX_RULETAG_SIZE] = {};
  const std::string activeTag = cellContext->getRuleSet()->getRuleTag();
  const std::size_t tagBytes =
    std::min(activeTag.size(), static_cast<std::size_t>(MAX_RULETAG_SIZE - 1));
  std::memcpy(ruleTag, activeTag.data(), tagBytes);

  Canvas* canvas = cellContext->getCellCanvas();
  const int width = canvas->canvasWidth;
  const int height = canvas->canvasHeight;
  const std::size_t cellCount =
    static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
  file.write(ruleTag, MAX_RULETAG_SIZE);
  file.write(reinterpret_cast<const char*>(&width), sizeof(width));
  file.write(reinterpret_cast<const char*>(&height), sizeof(height));
  file.write(reinterpret_cast<const char*>(canvas->lifeCanvas),
             static_cast<std::streamsize>(cellCount));
  const bool succeeded = file.good();
  file.close();
  if (!succeeded) {
    ic->commandLine->logError("Failed while writing: " + filename);
  }
  return succeeded;
}

bool
CellGameModule::LoadCellGame(std::string filename)
{
  if (filename.empty()) {
    ic->commandLine->logError("Load path is empty");
    return false;
  }

  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    ic->commandLine->logError("Failed to open for loading: " + filename);
    return false;
  }

  char ruleTag[MAX_RULETAG_SIZE + 1] = {};
  int fileWidth = 0;
  int fileHeight = 0;
  if (!file.read(ruleTag, MAX_RULETAG_SIZE) ||
      !file.read(reinterpret_cast<char*>(&fileWidth), sizeof(fileWidth)) ||
      !file.read(reinterpret_cast<char*>(&fileHeight), sizeof(fileHeight))) {
    ic->commandLine->logError("Invalid or truncated Illumo save header");
    return false;
  }

  const std::string ruleString(ruleTag);
  if (!CellContext::IsKnownModeString(ruleString)) {
    ic->commandLine->logError("Save uses unsupported ruleset: " + ruleString);
    return false;
  }

  const long long fileCellCount =
    static_cast<long long>(fileWidth) * static_cast<long long>(fileHeight);
  const long long maxLoadCells = 100000000LL;
  if (fileWidth < 1 || fileHeight < 1 || fileCellCount < 1 ||
      fileCellCount > maxLoadCells) {
    ic->commandLine->logError("Save contains invalid canvas dimensions");
    return false;
  }

  std::vector<unsigned char> loadedCells(
    static_cast<std::size_t>(fileCellCount));
  if (!file.read(reinterpret_cast<char*>(loadedCells.data()),
                 static_cast<std::streamsize>(loadedCells.size()))) {
    ic->commandLine->logError(
      "Save is truncated before all cell data was read");
    return false;
  }

  if (cellContext->setRuleSet(ruleString)) {
    cellContext->getCellCanvas()->rebuildPalette(cellContext->getRuleSet());
  }

  Canvas* canvas = cellContext->getCellCanvas();
  canvas->clearCanvas();
  const int copyWidth = std::min(fileWidth, canvas->canvasWidth);
  const int copyHeight = std::min(fileHeight, canvas->canvasHeight);
  for (int y = 0; y < copyHeight; ++y) {
    const unsigned char* sourceRow =
      loadedCells.data() +
      static_cast<std::size_t>(y) * static_cast<std::size_t>(fileWidth);
    unsigned char* destinationRow =
      canvas->lifeCanvas + static_cast<std::size_t>(y) *
                             static_cast<std::size_t>(canvas->canvasWidth);
    std::memcpy(destinationRow, sourceRow, static_cast<std::size_t>(copyWidth));
  }
  canvas->markCellsDirty();
  canvas->rebuildTargetsFromLife();
  canvas->snapVisualToTargets();

  if (fileWidth != canvas->canvasWidth || fileHeight != canvas->canvasHeight) {
    ic->commandLine->logWarning(
      "Loaded overlap from " + std::to_string(fileWidth) + " x " +
      std::to_string(fileHeight) + " save into " +
      std::to_string(canvas->canvasWidth) + " x " +
      std::to_string(canvas->canvasHeight) + " canvas");
  }
  return true;
}

void
CellGameModule::CameraPan()
{
  std::array<double, 2> mousePos = ic->inputManager->getMousePosition();
  glm::vec2 worldMouse =
    ic->camera->ScreenToWorld(glm::vec2(mousePos[0], mousePos[1]));
  static glm::vec2 lastMousePos = worldMouse;
  static bool wasPressed = false;

  if (ic->inputManager->isMouseButtonPressed(KeyCode::MouseMiddle)) {
    if (!wasPressed) {
      lastMousePos = worldMouse;
      wasPressed = true;
    }
    glm::vec2 delta = lastMousePos - worldMouse;
    ic->camera->Pan(delta * ic->camera->GetZoom());
    worldMouse = ic->camera->ScreenToWorld(glm::vec2(mousePos[0], mousePos[1]));
  } else {
    wasPressed = false;
  }
  lastMousePos = worldMouse;
}

void
CellGameModule::CameraRotate()
{
}

void
CellGameModule::DispatchDrawables(Scene* scene)
{
  if (cellContext == nullptr || scene == nullptr) {
    return;
  }
  scene->AddDrawable(this->cellContext->getCellCanvas());
  // Mode splash sits above the canvas (token UI path via
  // SplashText::AppendCommands).
  if (modeSplash != nullptr && modeSplash->isVisible()) {
    scene->AddDrawable(modeSplash.get());
  }
}
