#pragma once 
#include "CellState.h"

/*
	CellEditState
		Pauses the simulation, and allows the user to paint the canvas with cells or erase cells of the canvas.
*/
class CellEditState : public CellState {
public:
	~CellEditState() override = default;
	void iterate(CellRuleSet& ruleSet) override;
	bool first = true;
};