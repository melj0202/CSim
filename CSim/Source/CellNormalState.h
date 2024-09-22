#pragma once 
#include "CellState.h"

/*
	CellNormalState
		The normal state valuates the cell canvas and calculates the next generation of cells based on the currents cells in the canvas.
*/
class CellNormalState : public CellState {
public:
	void iterate(CellRuleSet& ruleSet) override;
	~CellNormalState() override = default;
};