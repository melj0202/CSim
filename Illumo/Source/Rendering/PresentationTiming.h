#pragma once

#include "Services/IEnvVars.h"
#include <string>

inline bool
isVsyncRequested(IEnvVars* envVars)
{
  if (envVars == nullptr) {
    return true;
  }

  const EnvVar& configured = envVars->getVar("vsync");
  return configured.value.empty() ? true : configured.valueAsBool;
}

inline std::string
buildFrameRateLabel(bool framePaced, int pacedFps, int submitFps)
{
  const std::string pacedValue =
    framePaced ? std::to_string(pacedFps) : std::string("off");
  return "Paced FPS: " + pacedValue +
         " | Submit FPS: " + std::to_string(submitFps);
}
