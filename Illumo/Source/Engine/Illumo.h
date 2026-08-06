#pragma once

#include "DebugAlloc.h"
#include "IModule.h"
#include "IllumoContext.h"
#include "Logger.h"
#include "Rendering/RenderWindow.h"
#include <memory>
#include <vector>

// Engine host: owns services and a list of IModule subsystems.
// Game modules are registered by App (or tests), not hard-coded here (D-E1).
class Illumo
{
public:
  Illumo(int argc, char** argv);
  ~Illumo();

  // Register a module. Call StartModules() after Init() once the set is
  // complete.
  void addModule(std::unique_ptr<IModule> module);

  // Construct services, window, renderer, scene, and fill IllumoContext.
  // Does not register or start game modules.
  void Init();

  // Call Start() on every registered module (after Init + addModule).
  void StartModules();

  void Update(double dt);
  void Render();
  void Shutdown();

  IllumoContext* GetContext() { return &context; }
  Scene* GetScene() { return context.scene; }
  RenderWindow* GetWindow() { return window.get(); }
  InputManager* GetInputManager() { return context.inputManager; }
  Renderer* GetRenderer() { return context.renderer; }
  AssetManager* GetAssetManager() { return context.assetManager; }
  EnvVars* GetEnvVars() { return context.envVars; }
  Camera* GetCamera() { return context.camera; }
  CommandRegistry* GetCommandRegistry() { return context.commandRegistry; }
  CommandLine* GetCommandLine() { return context.commandLine; }
  bool ShouldClose() { return context.window->shouldWindowClose(); }

private:
  std::unique_ptr<EnvVars> envVars;
  std::unique_ptr<Camera> camera;
  std::unique_ptr<RenderWindow> window;
  std::unique_ptr<Renderer> renderer;
  std::unique_ptr<AssetManager> assetManager;
  std::unique_ptr<CommandRegistry> commandRegistry;
  std::unique_ptr<CommandLine> commandLine;
  std::unique_ptr<InputManager> inputManager;
  std::unique_ptr<Scene> scene;
  IllumoContext context;
  std::vector<std::unique_ptr<IModule>> modules;
  bool modulesStarted;
};
