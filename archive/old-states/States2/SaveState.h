#pragma once
#include "State.h"

class SaveState : public State {
public:
    ~SaveState() override = default;
    State* iterate(RuleSet* ruleSet, const char* filename, State* prevState) override; //DO NOT USE THIS
};