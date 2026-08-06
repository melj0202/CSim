#pragma once
#include <cstring>
#include <fstream>
#include <string>

/*
    This class describes a logger class that writes messages to a file,
    and to a terminal if there is one present in the build
    (aka if it is a debug build)

    Log Levels
        0 = No logging
        1 = Error logging
        2 = Error and warning logging
        3 = Error, warning, and info logging
        4 = Error, warning, info, and trace logging

*/

class IEnvVars;
class CommandLine;

class Logger
{
public:
  Logger(IEnvVars* ev, CommandLine* cl);
  ~Logger();

  void operator=(const Logger&) = delete;
  Logger(const Logger&) = delete;

  [[nodiscard]] static bool LogError(const char* message);
  [[nodiscard]] static bool LogWarning(const char* message);
  [[nodiscard]] static bool LogInfo(const char* message);
  [[nodiscard]] static bool Log(const char* message);
  [[nodiscard]] static bool LogTrace(const char* message);

  [[nodiscard]] static bool LogError(char* message);
  [[nodiscard]] static bool LogWarning(char* message);
  [[nodiscard]] static bool LogInfo(char* message);
  [[nodiscard]] static bool Log(char* message);
  [[nodiscard]] static bool LogTrace(char* message);

  [[nodiscard]] static bool LogError(const std::string& message)
  {
    return LogError(message.c_str());
  }
  [[nodiscard]] static bool LogWarning(const std::string& message)
  {
    return LogWarning(message.c_str());
  }
  [[nodiscard]] static bool LogInfo(const std::string& message)
  {
    return LogInfo(message.c_str());
  }
  [[nodiscard]] static bool Log(const std::string& message)
  {
    return Log(message.c_str());
  }
  [[nodiscard]] static bool LogTrace(const std::string& message)
  {
    return LogTrace(message.c_str());
  }

  // Wide character logging functions
  static bool LogWError(const wchar_t* /*message*/) { return true; };
  static bool LogWWarning(const wchar_t* /*message*/) { return true; };
  static bool LogWInfo(const wchar_t* /*message*/) { return true; };
  static bool LogW(const wchar_t* /*message*/) { return true; };
  /*
      All logging functions return a integer which represents the number of
     bytes written. This allows for checking for any apparent errors. Just check
     if the returned value is 0.
  */
  static bool initLogger(IEnvVars* ev = nullptr, CommandLine* cl = nullptr);
  static void setContext(IEnvVars* ev, CommandLine* cl);
  static void shutdownLogger();
  static CommandLine* getCommandLine()
  {
    return instance ? instance->commandLine : nullptr;
  }

  std::ofstream logFileStream;

private:
  static long getSafeLogLevel();
  static Logger* instance;
  IEnvVars* envVars;
  CommandLine* commandLine;
};