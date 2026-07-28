#pragma once
#include <unordered_map>
#include <vector>
#include <functional>
#include <string>

using CommandFn =
    std::function<void(
        const std::vector<std::string>&
    )>;

class CommandRegistry {
    struct QueuedCommand {
        CommandFn fn;
        std::vector<std::string> args;
    };

    std::unordered_map<std::string, CommandFn> commands;
    std::vector<QueuedCommand> queue;

    public:
        CommandRegistry() {}
        ~CommandRegistry() = default;

        void RegisterCommand(const std::string& name, CommandFn fn) { commands[name] = fn; }
        void UnregisterCommand(const std::string& name) { commands.erase(name); }
        void QueueCommand(const std::string& name, const std::vector<std::string>& args = {}) {
            auto it = commands.find(name);
            if (it != commands.end()) {
                queue.push_back({it->second, args});
            }
        }
        void ClearQueue() { queue.clear(); }
        void ExecuteQueue() {
            for (auto& qc : queue) {
                if (qc.fn) qc.fn(qc.args);
            }
            queue.clear();
        }
        bool HasCommand(const std::string& name) const { return commands.count(name) > 0; }
};