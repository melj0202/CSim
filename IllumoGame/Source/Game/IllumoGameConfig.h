#pragma once

class IEnvVars;

class IllumoGameConfig
{
public:
  static void ApplyDefaults(IEnvVars* environment);
};
