#include "Illumo.h"
#include "CommandLine.h"
#include "CommandRegistry.h"
#include "EnvVars.h"
#include "GLString.h"
#include "IModule.h"
#include "InputManager.h"
#include "Rendering/OpenGL/CreateOpenGLBackend.h"
#include <AssetManager.h>
#include <Camera.h>
#include <IBackend.h>
#include <RenderWindow.h>
#include <Renderer.h>
#include <Scene.h>
#include <glm/fwd.hpp>
#include <memory>
#include <tracy/Tracy.hpp>
#include <utility>

Illumo::Illumo(int argc, char** argv)
  : modulesStarted(false)
{
  (void)argc;
  (void)argv;
}

Illumo::~Illumo()
{
  Shutdown();
  Logger::setContext(nullptr, nullptr);
}

void
Illumo::addModule(std::unique_ptr<IModule> module)
{
  modules.push_back(std::move(module));
}

void
Illumo::Init()
{

  // Load env first so flags like showFPS can influence module setup.
  envVars = std::make_unique<EnvVars>();
  if (envVars->getVar("speedFactor").value == "") {
    envVars->setVar("speedFactor", "1");
  }
  if (envVars->getVar("tps").value == "") {
    envVars->setVar("tps", "30");
  }
  if (envVars->getVar("fps").value == "") {
    envVars->setVar("fps", "60");
  }
  if (envVars->getVar("CanvasX").value == "") {
    envVars->setVar("CanvasX", 80);
  }
  if (envVars->getVar("CanvasY").value == "") {
    envVars->setVar("CanvasY", 60);
  }
  if (envVars->getVar("WinX").value == "") {
    envVars->setVar("WinX", 1280);
  }
  if (envVars->getVar("WinY").value == "") {
    envVars->setVar("WinY", 720);
  }
  if (envVars->getVar("ModeString").value == "") {
    envVars->setVar("ModeString", "GAME_OF_LIFE");
  }
  if (envVars->getVar("showFPS").value == "") {
    envVars->setVar("showFPS", 0);
  }
  // How quickly cell display colors ease toward logical state (higher =
  // snappier; 0 = instant)
  if (envVars->getVar("cellFadeSpeed").value == "") {
    envVars->setVar("cellFadeSpeed", "8");
  }
  if (envVars->getVar("logLevel").value == "") {
    envVars->setVar("logLevel", 2);
  }
  if (envVars->getVar("fullscreen").value == "") {
    envVars->setVar("fullscreen", false);
  }
  if (envVars->getVar("enableInfCanvas").valueAsBool == false) {
    envVars->setVar("enableInfCanvas", false);
  }
  window = std::make_unique<RenderWindow>(1280, 720, "Illumo", envVars.get());
  camera = std::make_unique<Camera>(glm::vec2(0.0f, 0.0f), 1.0f, envVars.get());
  // D-R11: construct the concrete backend at composition root via factory.
  // Renderer depends only on IBackend — never on GLBackend types.
  IBackend* productionBackend = CreateOpenGLBackend(window.get());
  renderer = std::make_unique<Renderer>(
    window.get(), envVars.get(), camera.get(), productionBackend, true);
  assetManager = std::make_unique<AssetManager>(renderer.get());
  commandRegistry = std::make_unique<CommandRegistry>();
  commandLine = std::make_unique<CommandLine>(
    envVars.get(), commandRegistry.get(), window.get(), renderer.get());
  Logger::setContext(envVars.get(), commandLine.get());
  inputManager = std::make_unique<InputManager>(window->getWindowInstance());
  scene = std::make_unique<Scene>(window.get(), camera.get());

  // GLString draw path needs the active window (FPS overlay, splash text, etc.)
  GLString::setRenderWindow(window.get());

  context.envVars = envVars.get();
  context.window = window.get();
  context.commandLine = commandLine.get();
  context.inputManager = inputManager.get();
  context.renderer = renderer.get();
  context.assetManager = assetManager.get();
  context.camera = camera.get();
  context.commandRegistry = commandRegistry.get();
  context.scene = scene.get();
  // Game / debug modules are registered by App (CellMain) after Init (D-E1).
  modulesStarted = false;
}

void
Illumo::StartModules()
{
  if (modulesStarted) {
    Logger::LogWarning("Illumo::StartModules called more than once; ignoring");
    return;
  }
  auto it = modules.begin();
  while (it != modules.end()) {
    if (!(*it)->Start(&context)) {
      it = modules.erase(it);
    } else {
      ++it;
    }
  }
  modulesStarted = true;
}

void
Illumo::Update(double dt)
{
  ZoneScoped;
  context.inputManager->update();
  camera->Update(static_cast<float>(dt));
  for (auto& module : modules) {
    module->Update(dt);
  }
}

void
Illumo::Render()
{
  // Single production frame path (D-R13): modules contribute drawables,
  // Renderer emits tokens + optional hybrid immediate fallback for stubs.
  // Token proof lives only as Renderer::RenderProofQuad for headless tests.
  context.scene->ClearDrawables();
  for (auto& module : modules) {
    module->DispatchDrawables(context.scene);
  }

  if (renderer) {
    renderer->BeginFrame();
    renderer->RenderScene(context.scene, context.camera);
    renderer->EndFrame();
  }
}

void
Illumo::Shutdown()
{
  for (std::unique_ptr<IModule>& module : modules) {
    if (module) {
      module->Exit();
    }
  }
  modules.clear();
  modulesStarted = false;
}
