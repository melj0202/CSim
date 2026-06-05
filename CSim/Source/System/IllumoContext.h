#pragma once

#include "Scene.h"
#include "IRenderWindow.h"
#include "RenderWindow.h"
#include "CommandLine.h"
#include "System/InputManager.h"
#include "Rendering/AssetManager.h"
#include "IEnvVars.h"
#include "EnvVars.h"
#include "Rendering/Renderer.h"
#include "CommandRegistry.h"
#include "EntityTable.h"
#include "Init/MacroDefs.h"

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
        EntityTable* entityTable;
    };