#pragma once
#include "IllumoContext.h"
#include "Scene.h"
class IModule
{
public:
  IModule() {}

  virtual ~IModule() = default;

protected:
  IllumoContext* ic;

public:
  virtual bool Start(IllumoContext* context) = 0;
  virtual void Update(double dt) = 0;
  virtual void DispatchDrawables(Scene* scene) = 0;
  virtual void Exit() = 0;
};