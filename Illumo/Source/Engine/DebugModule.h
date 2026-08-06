#pragma once
#include "CommandLine.h"
#include "IModule.h"
#include "Rendering/GLString.h"
#include <tracy/Tracy.hpp>

class DebugModule : public IModule
{
public:
  DebugModule();
  ~DebugModule();
  virtual bool Start(IllumoContext* context) override;
  void Update(double dt) override;
  void DispatchDrawables(Scene* scene) override;
  void Exit() override;

private:
  bool isShowFpsEnabled() const;
  void updateFpsCounter(double dt);

  GLString* fpsLabel;
  double fpsAccum;
  int fpsFrames;
  int fpsDisplay;
};
