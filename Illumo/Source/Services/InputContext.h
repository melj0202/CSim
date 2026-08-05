#pragma once
#include "Services/KeyCode.h"
#include <unordered_map>
#include <string>

class InputContext {
    private:
        std::unordered_map<std::string, InputEvent> actions;

    public:
        InputContext()  {}
        InputEvent getActionTag(const std::string& actionTag) const { 
            return actions.at(actionTag);
        }
        const std::unordered_map<std::string, InputEvent>& getActions() const { return actions; }
        //bool containsKeyCode(const std::string& keyCode) const { return actions.count(keyCode); }

        void bindAction(const std::string& actionTag, InputEvent inputEvent) { actions[actionTag] = inputEvent; }
};