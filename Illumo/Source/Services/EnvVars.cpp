#include "EnvVars.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <nlohmann/json.hpp>

void
EnvVars::load()
{
  std::ifstream file("envvars.json");
  if (!file.is_open()) {
    return;
  }
  nlohmann::json j;
  try {
    file >> j;
    for (auto it = j.begin(); it != j.end(); ++it) {
      std::string key = it.key();
      auto item = it.value();
      if (item.is_string()) {
        setVar(key, item.get<std::string>());
      } else if (item.is_object()) {
        setVar(key, item.value("value", ""));
      }
    }
  } catch (...) {
    // Ignore JSON parsing errors
  }
  file.close();
}

void
EnvVars::save()
{
  nlohmann::json j;
  for (const auto& pair : m_vars) {
    j[pair.first] = pair.second.value;
  }
  std::ofstream file("envvars.json");
  if (file.is_open()) {
    file << j.dump(1);
    file.close();
  }
}

void
EnvVars::setVar(const std::string& key, const std::string& value)
{
  EnvVar var;
  var.value = value;

  try {
    var.valueAsLong = std::stol(value);
  } catch (...) {
    var.valueAsLong = 0L;
  }

  try {
    var.valueAsDouble = std::stod(value);
  } catch (...) {
    var.valueAsDouble = 0.0;
  }

  std::string lowerValue = value;
  std::transform(lowerValue.begin(),
                 lowerValue.end(),
                 lowerValue.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  var.valueAsBool = (lowerValue == "true" || lowerValue == "1" ||
                     lowerValue == "yes" || lowerValue == "on");

  m_vars[key] = var;
}

void
EnvVars::setVar(const std::string& key, const double& value)
{
  setVar(key, std::to_string(value));
}

void
EnvVars::setVar(const std::string& key, const int& value)
{
  setVar(key, std::to_string(value));
}

void
EnvVars::setVar(const std::string& key, const long& value)
{
  setVar(key, std::to_string(value));
}

void
EnvVars::setVar(const std::string& key, const bool& value)
{
  setVar(key, std::to_string(value));
}

void
EnvVars::setVar(const std::string& key, const unsigned int& value)
{
  setVar(key, std::to_string(value));
}

void
EnvVars::setVar(const std::string& key, const unsigned long& value)
{
  setVar(key, std::to_string(value));
}

void
EnvVars::setVar(const std::string& key, const unsigned long long& value)
{
  setVar(key, std::to_string(value));
}

void
EnvVars::setVar(const std::string& key, const char& value)
{
  setVar(key, std::to_string(value));
}

void
EnvVars::setVar(const std::string& key, const char* value)
{
  setVar(key, std::string(value));
}
const EnvVar&
EnvVars::getVar(const std::string& key)
{
  auto it = m_vars.find(key);
  if (it != m_vars.end()) {
    return it->second;
  }
  static const EnvVar defaultVar = { "", 0L, 0.0, false };
  return defaultVar;
}
