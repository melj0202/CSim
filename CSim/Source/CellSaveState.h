#pragma once
#include "CellState.h"

class CellSaveState : public CellState {
public:
    ~CellSaveState() override = default;
    CellState* iterate(CellRuleSet* ruleSet, const char* filename, CellState* prevState) override; //DO NOT USE THIS
};