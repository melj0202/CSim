#include "EnvVars.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

std::filesystem::path
EnvVars::ApplicationConfigPath()
{
#ifdef _WIN32
  std::wstring executablePath(32768, L'\0');
  const unsigned long pathLength =
    GetModuleFileNameW(nullptr,
                       executablePath.data(),
                       static_cast<unsigned long>(executablePath.size()));
  if (pathLength > 0 &&
      static_cast<size_t>(pathLength) < executablePath.size()) {
    executablePath.resize(pathLength);
    return std::filesystem::path(executablePath).parent_path() / "envvars.json";
  }
#endif
  return std::filesystem::current_path() / "envvars.json";
}

void
EnvVars::load()
{
  std::ifstream file(m_filePath);
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
  std::ofstream file(m_filePath);
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
