#include "CommandLine.h"
#include "CommandRegistry.h"
#include "DebugModule.h"
#include "EnvVars.h"
#include "IModule.h"
#include "Illumo.h"
#include "InputManager.h"
#include <AssetManager.h>
#include <Camera.h>
#include <CellGameModule.h>
#include <RenderWindow.h>
#include <Renderer.h>
#include <Scene.h>
#include <glm/fwd.hpp>
#include <memory>
#include <tracy/Tracy.hpp>
#include <utility>


Illumo::Illumo(int argc, char** argv)
{

}

Illumo::~Illumo()
{
	Shutdown();
}

void Illumo::addModule(std::unique_ptr<IModule> module)
{
	modules.push_back(std::move(module));
}

void Illumo::Init()
{

	addModule(std::make_unique<CellGameModule>());
#ifndef NDEBUG
	addModule(std::make_unique<DebugModule>());
#endif

	envVars = std::make_unique<EnvVars>();
	window = std::make_unique<RenderWindow>(1280, 720, "CSim", envVars.get());
	camera = std::make_unique<Camera>(glm::vec2(0.0f, 0.0f), 1.0f, envVars.get());
	renderer = std::make_unique<Renderer>(window.get(), envVars.get(), camera.get());
	assetManager = std::make_unique<AssetManager>(renderer.get());
	commandRegistry = std::make_unique<CommandRegistry>();
	commandLine = std::make_unique<CommandLine>(envVars.get(), commandRegistry.get(), window.get());
	inputManager = std::make_unique<InputManager>(window->getWindowInstance());
	scene = std::make_unique<Scene>(window.get(), camera.get());
	if (envVars->getVar("speedFactor").value == "")
	{
		envVars->setVar("speedFactor", "1");
	}
	if (envVars->getVar("tps").value == "")
	{
		envVars->setVar("tps", "30");
	}
	if (envVars->getVar("fps").value == "")
	{
		envVars->setVar("fps", "60");
	}
	if (envVars->getVar("CanvasX").value == "")
	{
		envVars->setVar("CanvasX", 80);
	}
	if (envVars->getVar("CanvasY").value == "")
	{
		envVars->setVar("CanvasY", 60);
	}
	if (envVars->getVar("WinX").value == "")
	{
		envVars->setVar("WinX", 1280);
	}
	if (envVars->getVar("WinY").value == "")
	{
		envVars->setVar("WinY", 720);
	}
	if (envVars->getVar("ModeString").value == "")
	{
		envVars->setVar("ModeString", "GAME_OF_LIFE");
	}
	if (envVars->getVar("showFPS").value == "")
	{
		envVars->setVar("showFPS", 0);
	}
	if (envVars->getVar("logLevel").value == "")
	{
		envVars->setVar("logLevel", 2);
	}
	if (envVars->getVar("fullscreen").value == "")
	{
		envVars->setVar("fullscreen", false);
	}
	if (envVars->getVar("enableInfCanvas").valueAsBool == false)
	{
		envVars->setVar("enableInfCanvas", false);
	}
	context.envVars = envVars.get();
	context.window = window.get();
	context.commandLine = commandLine.get();
	context.inputManager = inputManager.get();
	context.renderer = renderer.get();
	context.assetManager = assetManager.get();
	context.camera = camera.get();
	context.commandRegistry = commandRegistry.get();
	context.scene = scene.get();
	for (auto& module : modules)
	{
		module->Start(&context);
	}
}

void Illumo::Update(double dt)
{
	ZoneScoped;
	context.inputManager->update();
	camera->Update(static_cast<float>(dt));
	for (auto& module : modules)
	{
		module->Update(dt);
	}
	
}

void Illumo::Render()
{
	context.scene->ClearDrawables();
	for (auto& module : modules)
	{
		module->DispatchDrawables(context.scene);
	}
	scene->Update();
	window->swapBuffers();
	
}

void Illumo::Shutdown()
{
	for (auto& module : modules)
	{
		if (module)
		{
			module->Exit();
		}
	}
	modules.clear();
}