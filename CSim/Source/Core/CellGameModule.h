#pragma once
#include "System/IModule.h"
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
        //void GenerateRenderCmds() override;
        void DispatchDrawables(Scene* scene) override;
        void Exit() override;
        
    private:
        void Normal();
        void Edit();
        void SaveCellGame(std::string filename);
        void LoadCellGame(std::string filename);
        void CameraPan();
        void CameraRotate();
        CellContext* cellContext;
        CellState currentState;
        InputContext inputContext;
};