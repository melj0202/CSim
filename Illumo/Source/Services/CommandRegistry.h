#pragma once
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

using CommandFn = std::function<void(const std::vector<std::string>&)>;

class CommandRegistry
{
  struct QueuedCommand
  {
    CommandFn fn;
    std::vector<std::string> args;
  };

  std::unordered_map<std::string, CommandFn> commands;
  std::unordered_map<std::string, std::string> usages;
  std::unordered_map<std::string, std::string> descriptions;
  std::unordered_map<std::string, std::vector<std::string>> completions;
  std::vector<QueuedCommand> queue;

public:
  CommandRegistry() {}
  ~CommandRegistry() = default;

  void RegisterCommand(
    const std::string& name,
    CommandFn fn,
    const std::string& usage = "",
    const std::string& description = "",
    const std::vector<std::string>& completionCandidates = {})
  {
    commands[name] = fn;
    usages[name] = usage;
    descriptions[name] = description;
    completions[name] = completionCandidates;
  }
  void UnregisterCommand(const std::string& name)
  {
    commands.erase(name);
    usages.erase(name);
    descriptions.erase(name);
    completions.erase(name);
  }
  bool QueueCommand(const std::string& name,
                    const std::vector<std::string>& args = {})
  {
    std::unordered_map<std::string, CommandFn>::iterator it =
      commands.find(name);
    if (it != commands.end()) {
      queue.push_back({ it->second, args });
      return true;
    }
    return false;
  }
  void ClearQueue() { queue.clear(); }
  void ExecuteQueue()
  {
    for (auto& qc : queue) {
      if (qc.fn)
        qc.fn(qc.args);
    }
    queue.clear();
  }
  bool HasCommand(const std::string& name) const
  {
    return commands.count(name) > 0;
  }
  std::string GetCommandUsage(const std::string& name) const
  {
    std::unordered_map<std::string, std::string>::const_iterator it =
      usages.find(name);
    return it != usages.end() ? it->second : "";
  }
  std::string GetCommandDescription(const std::string& name) const
  {
    std::unordered_map<std::string, std::string>::const_iterator it =
      descriptions.find(name);
    return it != descriptions.end() ? it->second : "";
  }
  std::vector<std::string> GetCommandCompletions(const std::string& name) const
  {
    std::unordered_map<std::string, std::vector<std::string>>::const_iterator
      it = completions.find(name);
    return it != completions.end() ? it->second : std::vector<std::string>();
  }
  std::vector<std::string> GetCommandNames() const
  {
    std::vector<std::string> names;
    for (const std::pair<const std::string, CommandFn>& command : commands) {
      names.push_back(command.first);
    }
    std::sort(names.begin(), names.end());
    return names;
  }
};
