#pragma once
#include "CanvasView.h"
#include "Rendering/Camera.h"
#include "Rendering/IRenderWindow.h"
#include "Rulesets/AllSets.h"
#include "Services/CommandLine.h"
#include "Services/IEnvVars.h"
#include "Services/Logger.h"
#include "SparseCellGrid.h"
#include <cctype>
#include <string>
#include <vector>

class Renderer;

class CellContext
{
public:
  CellContext(std::string modeString,
              IEnvVars* envVars = nullptr,
              IRenderWindow* window = nullptr,
              Camera* camera = nullptr,
              Renderer* renderer = nullptr)
  {
    this->envVars = envVars;
    this->window = window;
    this->camera = camera;
    this->renderer = renderer;
    this->commandLine = nullptr;

    long cx = 80;
    long cy = 60;
    if (envVars) {
      cx = envVars->getVar("CanvasX").valueAsLong;
      cy = envVars->getVar("CanvasY").valueAsLong;
      if (cx < 1)
        cx = 80;
      if (cy < 1)
        cy = 60;
    }
    grid = new SparseCellGrid();
    spareGrid = new SparseCellGrid();
    canvasView = new CanvasView(static_cast<int>(cx),
                                static_cast<int>(cy),
                                grid,
                                window,
                                camera,
                                renderer);
    ruleSet = nullptr;
    ModeString = "";
    setRuleSet(modeString);
  }
  ~CellContext()
  {
    delete ruleSet;
    delete canvasView;
    delete grid;
    delete spareGrid;
  }

  static std::string NormalizeModeString(std::string modeString)
  {
    for (size_t i = 0; i < modeString.size(); ++i) {
      modeString[i] = static_cast<char>(
        std::toupper(static_cast<unsigned char>(modeString[i])));
    }
    return modeString;
  }

  static bool IsKnownModeString(const std::string& modeString)
  {
    return modeString == "GAME_OF_LIFE" || modeString == "BRIANS_BRAIN" ||
           modeString == "DAY_AND_NIGHT" || modeString == "HIGHLIFE" ||
           modeString == "LIFE_WITHOUT_DEATH" || modeString == "SEEDS" ||
           modeString == "WIREWORLD";
  }

  static std::vector<std::string> GetKnownModeStrings()
  {
    return { "GAME_OF_LIFE",       "BRIANS_BRAIN", "DAY_AND_NIGHT", "HIGHLIFE",
             "LIFE_WITHOUT_DEATH", "SEEDS",        "WIREWORLD" };
  }

  // Returns true if the active ruleset instance changed.
  bool setRuleSet(std::string modeString)
  {
    modeString = NormalizeModeString(modeString);
    if (modeString.empty()) {
      modeString = "GAME_OF_LIFE";
    }

    // Avoid thrashing if console/env re-asserts the same mode.
    if (ruleSet != nullptr && modeString == ModeString) {
      return false;
    }

    delete ruleSet;
    ruleSet = nullptr;

    if (modeString == "GAME_OF_LIFE") {
      ruleSet = new GameOfLifeRuleSet(nullptr);
    } else if (modeString == "BRIANS_BRAIN") {
      ruleSet = new BrainsBrainRuleSet(nullptr);
    } else if (modeString == "DAY_AND_NIGHT") {
      ruleSet = new DayAndNightRuleSet(nullptr);
    } else if (modeString == "HIGHLIFE") {
      ruleSet = new HighlifeRuleSet(nullptr);
    } else if (modeString == "LIFE_WITHOUT_DEATH") {
      ruleSet = new LifeWithoutDeathRuleSet(nullptr);
    } else if (modeString == "SEEDS") {
      ruleSet = new SeedsRuleSet(nullptr);
    } else if (modeString == "WIREWORLD") {
      ruleSet = new WireworldRuleSet(nullptr);
    } else {
      Logger::LogError("Invalid rule set name: " + modeString);
      ruleSet = new GameOfLifeRuleSet(nullptr);
      modeString = "GAME_OF_LIFE";
    }

    ModeString = modeString;
    if (envVars) {
      envVars->setVar("ModeString", ModeString);
    }
    return true;
  }

  CanvasView* getCanvas() const { return canvasView; }
  CanvasView* getCellCanvas() const { return canvasView; }
  CanvasView* getCanvasView() const { return canvasView; }
  SparseCellGrid* getGrid() const { return grid; }
  SparseCellGrid* getSpareGrid() const { return spareGrid; }
  void publishSpareGrid(const SparseGenerationDelta& delta)
  {
    SparseCellGrid* previous = grid;
    grid = spareGrid;
    spareGrid = previous;
    canvasView->adoptGrid(grid, delta);
  }
  RuleSet* getRuleSet() const { return ruleSet; }
  std::string getModeString() const { return ModeString; }
  CommandLine* getCommandLine() const { return commandLine; }

private:
  RuleSet* ruleSet;
  SparseCellGrid* grid;
  SparseCellGrid* spareGrid;
  CanvasView* canvasView;
  std::string ModeString;
  CommandLine* commandLine;
  IEnvVars* envVars;
  IRenderWindow* window;
  Camera* camera;
  Renderer* renderer;
};
