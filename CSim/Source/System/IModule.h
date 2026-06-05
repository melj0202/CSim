#pragma once
#include "Scene.h"
#include "IllumoContext.h"
class IModule
{
public:
    IModule()
    {
    }

    virtual ~IModule() = default;

protected:
    IllumoContext* ic;

public:
    virtual void Start(IllumoContext* context) = 0;
    virtual void Update(double dt) = 0;
    virtual void DispatchDrawables(Scene* scene) = 0;
    virtual void Exit() = 0;
};