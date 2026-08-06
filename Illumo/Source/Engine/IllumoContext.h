#pragma once

#include "CommandLine.h"
#include "CommandRegistry.h"
#include "EnvVars.h"
#include "Foundation/MacroDefs.h"
#include "IEnvVars.h"
#include "IRenderWindow.h"
#include "Rendering/AssetManager.h"
#include "Rendering/Renderer.h"
#include "Scene.h"
#include "Services/InputManager.h"

// Non-owning service bag passed to IModule::Start.
//
// D-E5 — FROZEN shape for the current product (two modules only):
//   CellGameModule needs: envVars, window, camera, renderer, inputManager,
//   scene DebugModule needs:    envVars, window, renderer, inputManager,
//   commandLine, commandRegistry
//
// Do NOT add new members for convenience. Prefer explicit constructor
// dependencies if a third module needs a different subset. Missing pointers
// fail at Start() with a log error (not a silent null-deref later).
struct IllumoContext
{
  Scene* scene;
  IRenderWindow* window;
  CommandLine* commandLine;
  InputManager* inputManager;
  Renderer* renderer;
  AssetManager* assetManager;
  EnvVars* envVars;
  Camera* camera;
  CommandRegistry* commandRegistry;
};

// Required wiring for CellGameModule (game + canvas + input + sim).
inline bool
IllumoContextHasGameCore(const IllumoContext* c)
{
  return c != nullptr && c->envVars != nullptr && c->window != nullptr &&
         c->camera != nullptr && c->renderer != nullptr &&
         c->inputManager != nullptr && c->commandLine != nullptr &&
         c->commandRegistry != nullptr && c->scene != nullptr;
}

// Required wiring for DebugModule (console, FPS overlay, env flags).
inline bool
IllumoContextHasDebugCore(const IllumoContext* c)
{
  return c != nullptr && c->envVars != nullptr && c->window != nullptr &&
         c->renderer != nullptr && c->inputManager != nullptr &&
         c->commandLine != nullptr && c->commandRegistry != nullptr;
}
