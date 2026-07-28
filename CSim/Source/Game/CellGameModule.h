#pragma once
#include "Engine/IModule.h"
#include "CellContext.h"
#include <tracy/Tracy.hpp>
#include "Rendering/Scene.h"

enum class CellState {
    NORMAL,
    EDIT,
    SAVE,
    LOAD,
    EXIT
};

class CellGameModule : public IModule {
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
        void SaveCellGame(std::string filename);
        void LoadCellGame(std::string filename);
        void CameraPan();
        void CameraRotate();
        CellContext* cellContext;
        CellState currentState;
        InputContext inputContext;
        double simAccum;
        double simStepSeconds;
};
