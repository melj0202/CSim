#pragma once

#include <string>
#include <unordered_map>

struct EnvVar
{
  std::string value;
  long valueAsLong;
  double valueAsDouble;
  bool valueAsBool;
};

class IEnvVars
{
public:
  IEnvVars() = default;
  virtual ~IEnvVars() = default;

  virtual void load() = 0;
  virtual void save() = 0;

  virtual void setVar(const std::string& key, const std::string& value) = 0;
  virtual void setVar(const std::string& key, const long& value) = 0;
  virtual void setVar(const std::string& key, const double& value) = 0;
  virtual void setVar(const std::string& key, const bool& value) = 0;
  virtual void setVar(const std::string& key, const unsigned int& value) = 0;
  virtual void setVar(const std::string& key, const unsigned long& value) = 0;
  virtual void setVar(const std::string& key,
                      const unsigned long long& value) = 0;
  virtual void setVar(const std::string& key, const char& value) = 0;
  virtual void setVar(const std::string& key, const char* value) = 0;
  virtual void setVar(const std::string& key, const int& value) = 0;
  virtual const EnvVar& getVar(const std::string& key) = 0;
  virtual const std::unordered_map<std::string, EnvVar>& getVars() const = 0;
};
