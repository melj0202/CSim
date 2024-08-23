#pragma once 
#include "CellState.h"


class CellNormalState : public CellState {
	~CellNormalState() override = default;
	void iterate(CellCanvas& lifeCanvas, CellRuleSet& ruleSet, RenderWindow& window) override;
};