#pragma once
#include "State.h"

class LoadState : public State {
public:
      ~LoadState() override = default;
    State* iterate(RuleSet* ruleSet, const char* filename, State* prevState) override; //DO NOT USE THIS
};