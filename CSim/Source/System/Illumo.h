#pragma once

#include "IllumoContext.h"
#include <vector>
#include "IModule.h"
#include "Core/CellGameModule.h"
#include "DebugModule.h"
#include "Logger.h"
#include "DebugAlloc.h"
class Illumo {
    public:
    Illumo(int argc, char** argv);
    ~Illumo();
    
    void addModule(std::unique_ptr<IModule> module);
    void Init();
    void Update(double dt);
    void Render();
    void Shutdown();

    IllumoContext* GetContext() { return &context; };
    Scene* GetScene() { return context.scene; };
    RenderWindow* GetWindow() { return context.window; };
    InputManager* GetInputManager() { return context.inputManager; };
    Renderer* GetRenderer() { return context.renderer; };
    AssetManager* GetAssetManager() { return context.assetManager; };
    EnvVars* GetEnvVars() { return context.envVars; };
    Camera* GetCamera() { return context.camera; };
    CommandRegistry* GetCommandRegistry() { return context.commandRegistry; };
    //std::vector<std::unique_ptr<IGame>> GetModules() { return modules; };
    CommandLine* GetCommandLine() { return context.commandLine; };
    bool ShouldClose() { return context.window->shouldWindowClose(); };
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
};