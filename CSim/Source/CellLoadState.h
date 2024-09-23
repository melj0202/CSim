#include "CellState.h"

class CellLoadState : public CellState {
      ~CellLoadState() override = default;
    CellState* iterate(CellRuleSet* ruleSet, const char* filename, CellState* prevState) override; //DO NOT USE THIS
};