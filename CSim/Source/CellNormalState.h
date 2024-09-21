#pragma once 
#include "CellState.h"


class CellNormalState : public CellState {
public:
	void iterate(CellRuleSet& ruleSet) override;
	~CellNormalState() override = default;
};