#pragma once

#include "Game/CellGameModule.h"
#include "Services/InputManager.h"

class CellGameModuleTestAccess
{
public:
  static CellContext* getCellContext(CellGameModule& module)
  {
    return module.cellContext;
  }

  static CellState getState(const CellGameModule& module)
  {
    return module.currentState;
  }

  static int getLastSimulationSteps(const CellGameModule& module)
  {
    return module.lastSimulationSteps;
  }

  static bool getSimulationDebtDropped(const CellGameModule& module)
  {
    return module.simulationDebtDropped;
  }

  static bool save(CellGameModule& module, const std::string& filename)
  {
    return module.SaveCellGame(filename);
  }

  static bool load(CellGameModule& module, const std::string& filename)
  {
    return module.LoadCellGame(filename);
  }

  static unsigned char getWireworldBrush(const CellGameModule& module)
  {
    return module.wireworldBrush;
  }

  static void setWireworldBrush(CellGameModule& module, unsigned char state)
  {
    module.wireworldBrush = state;
  }
};

class InputManagerTestAccess
{
public:
  static void setAction(InputManager& input, KeyCode key, InputAction action)
  {
    input.inputStatesCurrent[key] = action;
  }

  static void setPreviousAction(InputManager& input,
                                KeyCode key,
                                InputAction action)
  {
    input.inputStatesPrevious[key] = action;
  }

  static int toGlfw(InputManager& input, KeyCode key)
  {
    return input.TranslateKeyCodeFromGLFW(key);
  }

  static KeyCode fromGlfw(InputManager& input, int key)
  {
    return input.TranslateKeyCodeToGLFW(key);
  }

  static InputAction actionFromGlfw(InputManager& input, int action)
  {
    return input.TranslateInputActionGLFW(action);
  }
};
