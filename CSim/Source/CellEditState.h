#pragma once 
#include "CellState.h"


class CellEditState : public CellState {
public:
	~CellEditState() override = default;
	void iterate(CellRuleSet& ruleSet) override;
	bool first = true;
};