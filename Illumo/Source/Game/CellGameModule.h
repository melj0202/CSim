#pragma once
#include "CellContext.h"
#include "Engine/IModule.h"
#include "Rendering/Scene.h"
#include "Rendering/SplashText.h"
#include <memory>
#include <tracy/Tracy.hpp>

enum class CellState
{
  NORMAL,
  EDIT,
  EXIT
};

class CellGameModule : public IModule
{
  friend class CellGameModuleTestAccess;

public:
  CellGameModule();
  ~CellGameModule();
  virtual bool Start(IllumoContext* context) override;
  void Update(double dt) override;
  void DispatchDrawables(Scene* scene) override;
  void Exit() override;

private:
  void Normal(double dt);
  void Edit(double dt);
  void updateVisualTargets();
  void syncSimRateFromEnv();
  void registerConsoleCommands();
  void unregisterConsoleCommands();
  bool SaveCellGame(std::string filename);
  bool LoadCellGame(std::string filename);
  void setRunning(bool running);
  void stepSimulation(int generations);
  void printStatus() const;
  void CameraPan();
  void CameraRotate();
  void seedInitialPattern();
  void updateWireworldBrushFromInput();
  void showModeSplash(const char* label);
  CellContext* cellContext;
  CellState currentState;
  InputContext inputContext;
  double simAccum;
  double simStepSeconds;
  // Wireworld left-paint state: 0 head, 1 empty, 2 tail, 3 conductor.
  // Selected with keys 1/H, 2, 3/T, 4 while the console is closed.
  unsigned char wireworldBrush;
  // Module-owned mode label (EDIT/NORMAL); not a file-scope global.
  std::unique_ptr<SplashText> modeSplash;
};
