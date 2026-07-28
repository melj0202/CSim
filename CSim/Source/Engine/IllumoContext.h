#pragma once

#include "Scene.h"
#include "IRenderWindow.h"
#include "RenderWindow.h"
#include "CommandLine.h"
#include "Services/InputManager.h"
#include "Rendering/AssetManager.h"
#include "IEnvVars.h"
#include "EnvVars.h"
#include "Rendering/Renderer.h"
#include "CommandRegistry.h"
#include "Foundation/MacroDefs.h"

// Non-owning service bag passed to IModule::Start.
// EntityTable removed — unused experimental ECS path archived (D-E3).
struct IllumoContext {
	Scene* scene;
	RenderWindow* window;
	CommandLine* commandLine;
	InputManager* inputManager;
	Renderer* renderer;
	AssetManager* assetManager;
	EnvVars* envVars;
	Camera* camera;
	CommandRegistry* commandRegistry;
};
