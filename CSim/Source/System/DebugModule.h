#pragma once
#include "IModule.h"
#include "CommandLine.h"
#include "Rendering/GLString.h"
#include <tracy/Tracy.hpp>

class DebugModule : public IModule {
    public:
        DebugModule();
        ~DebugModule() = default;
        void Start(IllumoContext* context) override;
        void Update(double dt) override;
        //void GenerateRenderCmds() override;
        void DispatchDrawables(Scene* scene) override;
        void Exit() override;
    private:
        GLString* glString;
};