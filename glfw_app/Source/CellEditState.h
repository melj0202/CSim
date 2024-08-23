#pragma once 
#include "CellState.h"


class CellEditState : public CellState {
	~CellEditState() override = default;
	void iterate(CellCanvas& lifeCanvas, CellRuleSet& ruleSet, RenderWindow& window) override;
};