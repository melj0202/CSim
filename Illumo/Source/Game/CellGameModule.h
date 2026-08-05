#pragma once
#include "Engine/IModule.h"
#include "CellContext.h"
#include <tracy/Tracy.hpp>
#include "Rendering/Scene.h"

enum class CellState {
    NORMAL,
    EDIT,
    EXIT
};

class CellGameModule : public IModule {
        friend class CellGameModuleTestAccess;
    public:
        CellGameModule();
        ~CellGameModule();
        void Start(IllumoContext* context) override;
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
        CellContext* cellContext;
        CellState currentState;
        InputContext inputContext;
        double simAccum;
        double simStepSeconds;
};
